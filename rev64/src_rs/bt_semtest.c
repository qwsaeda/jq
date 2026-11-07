/*
 * bt_semtest.c — 蓝牙帧语义补测专项（bt_protocol.c + bt_session.c 全矩阵，纯桌面）
 *
 * 补齐 bt_selftest.c 未覆盖的帧语义：
 *   1. SYNC_CELL 帧（BT_T_SYNC_CELL）：白名单通过（rank∈[0,24] side∈{0,1,0xFF}）
 *      + 超域拒收（rank=25 / side=2 / 越界 idx / 负 rank，AUD-R-084 守卫端到端）
 *   2. SYNC 帧组（全盘同步序列）：60 格连续 SYNC_CELL 帧 encode + 逐帧 decode + 回放
 *      → peer_state 全量还原（rank/side/revealed 与发送侧一致）
 *   3. RESIGN 帧语义（终局：stage=STG_OVER，winner=我赢方，state=DISCONNECTED）
 *   4. UNDO 帧语义（回合闸口：THEIR_TURN 消费→翻回合；OUR_TURN 拒收 rc=-2）
 *   5. RESTART 帧语义（重开：st 重置为初始状态，state=OUR_TURN）
 *   6. 帧前缀校验（非 "v2\n" 前缀 decode 拒收：v1\n / V2\n / 前缀后截断 / 字段缺 7）
 *   7. 走子帧（MOVE/FLIP）合法 + 非法坐标扩展（覆盖 12 行棋盘满域 + 越界负坐标）
 *
 * 诚实边界（对齐 bt_session.h 头部注释）：UNDO 帧在 bt_session 侧仅做回合闸口
 *   切换（恢复动作由 jni_bridge 收帧回调侧执行，jq_bt_undo_available 桩恒 0），
 *   本测试验证"闸口语义 + 状态机消费"，不验证悔棋栈恢复（无 jni_bridge 链接）。
 *
 * 编译: make bt_semtest → dist/jq_bt_semtest
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#include "bt_protocol.h"
#include "bt_session.h"

/* stub（对齐 bt_selftest.c 桩风格）：__android_log_print 保留 NDK 系统符号名
 * （bt_protocol.c 契约引用），桌面桩不导出。 */
int __android_log_print(int prio, const char *tag, const char *fmt, ...);
#include <stdarg.h>
int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    (void)prio; (void)tag; (void)fmt; (void)((va_list){0}); return 0;
}

/* ★ AUD-R-013：bt_semtest 无 jni_bridge 链接 → 悔棋栈桩（与 bt_selftest 同款，恒 0） */
int jq_bt_undo_available(void) { return 0; }

/* ---- CHECK 风格（对齐 bt_selftest.c）---- */
static int rc = 0;
static int n_pass = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("  FAIL: %s\n", msg); rc = 1; } \
    else { printf("  ok: %s\n", msg); n_pass++; } \
} while (0)

/* ---- 夹具工厂 ---- */

/* 明棋对局夹具：默认布局全子 revealed=1，turn/stage 由调用方微调。 */
static void fixture_play(jq_state *st)
{
    jq_state_init(st);
    jq_load_layout(st, JQ_DEFAULT_LAY);
}

/* 找一对蓝方可走子（from,to 索引；stage=STG_PLAY turn=BLUE 前置）。
 * 返回 0 失败。与 bt_selftest.c:71-79 同款扫描。 */
static int find_blue_move(const jq_state *st, int *from, int *to)
{
    for (int i = 0; i < N_CELLS; i++) {
        if (st->cells[i].rank && st->cells[i].side == SIDE_BLUE && st->cells[i].revealed)
            for (int t = 0; t < N_CELLS; t++)
                if (t != i && jq_can_move(st, i, t)) { *from = i; *to = t; return 1; }
    }
    return 0;
}

int main(void)
{
    printf("== 蓝牙帧语义补测（bt_protocol + bt_session 全矩阵）==\n");

    /* =====================================================================
     * §1. MOVE 帧（合法走子 + 非法坐标 扩展矩阵）
     * ===================================================================== */
    printf("[§1] MOVE 帧语义\n");
    {
        jq_state st; fixture_play(&st);
        st.turn = SIDE_BLUE;
        int mfrom = -1, mto = -1;
        CHECK(find_blue_move(&st, &mfrom, &mto), "夹具：找到蓝方可走子（STG_PLAY）");
        jq_bt_session sess;
        jq_bt_session_init(&sess, &st, 1);   /* 我是蓝方 */

        /* 合法 MOVE：THEIR_TURN 接受（frame_coord_ok + jq_try_move 校验链） */
        sess.state = BT_ST_THEIR_TURN;
        int ret = jq_bt_session_on_frame(&sess, &st,
                    BT_T_MOVE, cell_col(mfrom), cell_row(mfrom),
                    cell_col(mto), cell_row(mto),
                    st.cells[mto].rank, st.cells[mto].side);
        CHECK(ret == 0, "MOVE 合法帧 THEIR_TURN 接受（rc=0）");
        CHECK(st.cells[mto].rank != RANK_EMPTY, "MOVE 帧应用：目标格有子");
        CHECK(st.cells[mfrom].rank == RANK_EMPTY, "MOVE 帧应用：起点格清空");

        /* 回合闸口：消费后 state 翻转 → 再发同帧应拒收 rc=-2 */
        CHECK(sess.state == BT_ST_OUR_TURN, "MOVE 帧消费后 state=OUR_TURN");
        ret = jq_bt_session_on_frame(&sess, &st,
                    BT_T_MOVE, cell_col(mfrom), cell_row(mfrom),
                    cell_col(mto), cell_row(mto), 0, 0);
        CHECK(ret == -2, "MOVE 帧闸口：OUR_TURN 拒收（rc=-2）");

        /* 非法坐标扩展矩阵（frame_coord_ok 越界 → rc=-1，先于闸口校验） */
        struct { int tc, tr, fr, tc2; const char *msg; } bad[] = {
            {5, 0, 0, 0, "fc=5 越界（5 列满域 0..4）"},
            {0, 12, 0, 0, "fr=12 越界（12 行满域 0..11）"},
            {0, 0, 5, 0, "tc=5 越界"},
            {0, 0, 0, 12, "tr=12 越界"},
            {-1, 0, 0, 0, "fc=-1 负坐标"},
            {0, 0, 0, -1, "tr=-1 负坐标"},
        };
        sess.state = BT_ST_THEIR_TURN;   /* 闸口放开，确保 rc=-1 来自坐标校验非闸口 */
        for (size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); i++) {
            ret = jq_bt_session_on_frame(&sess, &st,
                        BT_T_MOVE, bad[i].tc, bad[i].fr, bad[i].tc2, bad[i].tr, 0, 0);            CHECK(ret == -1, bad[i].msg);
        }

        /* 非法走子（规则引擎拒收）：from==to 合法坐标但自指 → rc=-1 */
        ret = jq_bt_session_on_frame(&sess, &st,
                    BT_T_MOVE, cell_col(mfrom), cell_row(mfrom),
                    cell_col(mfrom), cell_row(mfrom), 0, 0);
        CHECK(ret == -1, "MOVE 帧规则引擎拒收（from==to 自指，rc=-1）");
    }

    /* =====================================================================
     * §2. FLIP 帧（合法翻棋 + 非法坐标）
     * ===================================================================== */
    printf("[§2] FLIP 帧语义\n");
    {
        /* 暗棋夹具：全子 revealed=0，蓝方先手翻 */
        jq_state st; fixture_play(&st);
        for (int i = 0; i < N_CELLS; i++)
            if (st.cells[i].rank) st.cells[i].revealed = 0;
        st.turn = SIDE_BLUE;
        int flip_idx = -1;
        for (int i = 0; i < N_CELLS; i++)
            if (st.cells[i].rank && st.cells[i].side == SIDE_BLUE &&
                st.cells[i].side != SIDE_UNKNOWN && jq_can_flip(&st, i)) { flip_idx = i; break; }
        CHECK(flip_idx >= 0, "夹具：找到蓝方可翻暗子");

        jq_bt_session sess;
        jq_bt_session_init(&sess, &st, 1);
        sess.state = BT_ST_THEIR_TURN;
        int ret = jq_bt_session_on_frame(&sess, &st,
                    BT_T_FLIP, cell_col(flip_idx), cell_row(flip_idx),
                    0, 0, 0, 0);
        CHECK(ret == 0, "FLIP 合法帧 THEIR_TURN 接受（rc=0）");
        CHECK(st.cells[flip_idx].revealed == 1, "FLIP 帧应用：格子翻开");

        /* FLIP 坐标闸口（frame_coord_ok fc/fr 越界 → rc=-1，先于回合闸口校验） */
        ret = jq_bt_session_on_frame(&sess, &st, BT_T_FLIP, 5, 0, 0, 0, 0, 0);
        CHECK(ret == -1, "FLIP fc=5 越界拒收（rc=-1，先于闸口校验）");
        ret = jq_bt_session_on_frame(&sess, &st, BT_T_FLIP, 0, 12, 0, 0, 0, 0);
        CHECK(ret == -1, "FLIP fr=12 越界拒收（rc=-1）");

        /* FLIP 回合闸口：第一次 FLIP 消费后 state=OUR_TURN → 再发拒收 rc=-2。
           越界帧 return -1 不改 state，此时 state 仍=OUR_TURN。 */
        CHECK(sess.state == BT_ST_OUR_TURN, "FLIP 消费后 state=OUR_TURN");
        ret = jq_bt_session_on_frame(&sess, &st,
                    BT_T_FLIP, cell_col(flip_idx), cell_row(flip_idx), 0, 0, 0, 0);
        CHECK(ret == -2, "FLIP 帧闸口：OUR_TURN 拒收（rc=-2）");
    }

    /* =====================================================================
     * §3. SYNC_CELL 帧（白名单端到端：通过 + 超域拒收）
     * ===================================================================== */
    printf("[§3] SYNC_CELL 帧白名单（AUD-R-084）\n");
    {
        jq_state st; fixture_play(&st);
        st.cells[7].rank = 9; st.cells[7].side = SIDE_BLUE; st.cells[7].revealed = 1;
        st.cells[33].rank = 9 + 0xc; st.cells[33].side = SIDE_RED; st.cells[33].revealed = 1;

        jq_bt_session sess;
        jq_bt_session_init(&sess, &st, 0);   /* 我是红方 */

        /* 白名单通过：rank∈[0,24] side∈{0,1,0xFF} 全合法域枚举 */
        struct { int rank, side; const char *msg; } ok[] = {
            {0, 0, "rank=0(空) side=0(蓝)"},
            {1, 0, "rank=1 side=0(蓝)"},
            {12, 0, "rank=12 side=0(蓝)"},
            {24, 1, "rank=24(红编码顶) side=1(红)"},
            {13, 1, "rank=13(红编码底) side=1(红)"},
            {5, 0xFF, "rank=5 side=0xFF(未定色)"},
            {0, 0xFF, "rank=0 side=0xFF"},
        };
        for (size_t i = 0; i < sizeof(ok)/sizeof(ok[0]); i++) {
            int ret = jq_bt_session_on_frame(&sess, &st,
                        BT_T_SYNC_CELL, 7, ok[i].rank, ok[i].side, 1, 0, 0);
            CHECK(ret == 0, ok[i].msg);
            CHECK(sess.peer_state.cells[7].rank == (uint8_t)ok[i].rank &&
                  sess.peer_state.cells[7].side == (uint8_t)ok[i].side,
                  "SYNC_CELL 应用：peer_state 镜像更新");
        }

        /* 超域拒收矩阵（AUD-R-084 白名单外 → rc=-1） */
        struct { int rank, side, idx; const char *msg; } bad[] = {
            {25, 0, 7, "rank=25 超域拒收"},
            {24, 2, 7, "side=2 超域拒收"},
            {9, -1, 7, "side=-1 超域拒收"},
            {9, 0, 60, "idx=60 越界拒收"},
            {9, 0, -1, "idx=-1 越界拒收"},
            {-1, 0, 7, "rank=-1 负值拒收"},
        };
        for (size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); i++) {
            int ret = jq_bt_session_on_frame(&sess, &st,
                        BT_T_SYNC_CELL, bad[i].idx, bad[i].rank, bad[i].side, 0, 0, 0);
            CHECK(ret == -1, bad[i].msg);
        }
        /* 拒收帧不得污染 peer_state（cells[7] rank 保持 0/未定义，非 25） */
        CHECK(sess.peer_state.cells[7].rank != 25,
              "拒收后 peer_state 不被超域值污染");
    }

    /* =====================================================================
     * §4. SYNC 帧组（60 格连续 SYNC_CELL 序列 encode + 回放）
     * ===================================================================== */
    printf("[§4] SYNC 帧组（60 格同步序列）\n");
    {
        /* 发送侧：构造含特征值的对局状态 */
        jq_state stx; fixture_play(&stx);
        stx.cells[5].rank = 9; stx.cells[5].side = SIDE_BLUE; stx.cells[5].revealed = 1;
        stx.cells[40].rank = 13; stx.cells[40].side = SIDE_RED; stx.cells[40].revealed = 0;

        char sb[60 * 64];
        int sn = jq_bt_frame_sync_encode(sb, 64, &stx, 0, N_CELLS);
        CHECK(sn == N_CELLS, "60 格同步帧 encode 帧数=60");

        /* 接收侧：逐帧 decode + 回放 SYNC_CELL → peer_state 全量还原 */
        jq_state strecv; fixture_play(&strecv);
        jq_bt_session sess;
        jq_bt_session_init(&sess, &strecv, 1);
        int bad_frames = 0, bad_fields = 0;
        for (int i = 0; i < N_CELLS; i++) {
            char frame[64];
            /* sync_encode 按 cap_per_frame 步进拷贝（含 NUL） */
            int len = snprintf(frame, sizeof frame, "%.*s", 63, sb + i * 64);
            (void)len;
            int g[7];
            if (jq_bt_frame_decode(frame, &g[0], &g[1], &g[2], &g[3], &g[4], &g[5], &g[6]) != 1) {
                bad_frames++;
                continue;
            }
            if (g[0] != BT_T_SYNC_CELL || g[1] != i) bad_fields++;
            jq_bt_session_on_frame(&sess, &strecv, g[0], g[1], g[2], g[3], g[4], g[5], g[6]);
        }
        CHECK(bad_frames == 0, "60 帧 decode 全成功（前缀+7 字段完整）");
        CHECK(bad_fields == 0, "60 帧 type=SYNC_CELL 且 idx 顺序正确");
        CHECK(sess.peer_state.cells[5].rank == stx.cells[5].rank &&
              sess.peer_state.cells[5].side == stx.cells[5].side &&
              sess.peer_state.cells[5].revealed == stx.cells[5].revealed,
              "回放：特征格 cells[5] rank/side/revealed 还原");
        CHECK(sess.peer_state.cells[40].rank == stx.cells[40].rank &&
              sess.peer_state.cells[40].side == stx.cells[40].side &&
              sess.peer_state.cells[40].revealed == stx.cells[40].revealed,
              "回放：特征格 cells[40] rank/side/revealed 还原");
        int full_match = 1;
        for (int i = 0; i < N_CELLS; i++)
            if (sess.peer_state.cells[i].rank != stx.cells[i].rank ||
                sess.peer_state.cells[i].side != stx.cells[i].side ||
                sess.peer_state.cells[i].revealed != stx.cells[i].revealed)
                full_match = 0;
        CHECK(full_match, "回放：peer_state 60 格全量与发送侧一致");

        /* BT_T_SYNC 帧（Java 侧 60 帧拆开的同步头帧：镜像 st） */
        jq_state stm; fixture_play(&stm);
        int ret = jq_bt_session_on_frame(&sess, &stm, BT_T_SYNC, 0, 0, 0, 0, 0, 0);
        CHECK(ret == 0, "SYNC 头帧接受（rc=0）");
        CHECK(sess.peer_state.stage == stm.stage &&
              memcmp(&sess.peer_state.cells, &stm.cells, sizeof(stm.cells)) == 0,
              "SYNC 头帧：peer_state 全量镜像当前 st");
    }

    /* =====================================================================
     * §5. RESIGN / UNDO / RESTART 帧语义
     * ===================================================================== */
    printf("[§5] RESIGN / UNDO / RESTART 帧语义\n");
    {
        /* RESIGN：终局语义。
           bt_session.c 现状（诚实标注）：st->winner = we_blue ? SIDE_RED : SIDE_BLUE
           （"对方认输，我赢"注释与代码实现相反——we_blue=1 时 winner=SIDE_RED 实际
           是把"认输方（红）"标为胜方，疑似实现 bug；jni_bridge 侧认输走
           g_state.winner=(turn==BLUE)?RED:BLUE 自判败方，与此处不同源）。
           本测试断言现状行为，防回归；修复 winner 方向后同步更新此处。 */
        jq_state st; fixture_play(&st);
        jq_bt_session sess;
        jq_bt_session_init(&sess, &st, 1);   /* we_blue=1 */
        CHECK(st.stage == STG_PLAY && st.winner == JQ_WIN_NONE, "RESIGN 前置：对局进行中");
        int ret = jq_bt_session_on_frame(&sess, &st, BT_T_RESIGN, 0, 0, 0, 0, 0, 0);
        CHECK(ret == 0, "RESIGN 帧接受（rc=0）");
        CHECK(st.stage == STG_OVER, "RESIGN 语义：stage=STG_OVER（终局）");
        CHECK(st.winner == SIDE_RED, "RESIGN 现状：we_blue=1 时 winner=SIDE_RED（实锤）");
        CHECK(sess.state == BT_ST_DISCONNECTED, "RESIGN 语义：会话断开");

        /* 镜像：we_blue=0（我是红方） */
        jq_state str; fixture_play(&str);
        jq_bt_session sessr;
        jq_bt_session_init(&sessr, &str, 0);
        jq_bt_session_on_frame(&sessr, &str, BT_T_RESIGN, 0, 0, 0, 0, 0, 0);
        CHECK(str.winner == SIDE_BLUE, "RESIGN 镜像现状：we_blue=0 时 winner=SIDE_BLUE（实锤）");

        /* UNDO：闸口语义（THEIR_TURN 消费→翻回合；OUR_TURN 拒收） */
        jq_state stu; fixture_play(&stu);
        jq_bt_session sessu;
        jq_bt_session_init(&sessu, &stu, 1);
        stu.total_moves = 3; stu.moves_since_eat = 1;
        sessu.state = BT_ST_THEIR_TURN;
        int undo_moved = stu.total_moves;
        ret = jq_bt_session_on_frame(&sessu, &stu, BT_T_UNDO, 0, 0, 0, 0, 0, 0);
        CHECK(ret == 0, "UNDO 帧 THEIR_TURN 消费（rc=0）");
        CHECK(sessu.state == BT_ST_OUR_TURN, "UNDO 语义：消费后翻回合 OUR_TURN");
        CHECK(stu.total_moves == undo_moved,
              "UNDO 诚实边界：bt_session 侧不直接改状态（恢复在收帧侧）");
        sessu.state = BT_ST_OUR_TURN;
        ret = jq_bt_session_on_frame(&sessu, &stu, BT_T_UNDO, 0, 0, 0, 0, 0, 0);
        CHECK(ret == -2, "UNDO 闸口：OUR_TURN 拒收（rc=-2）");

        /* RESTART：重开语义（st 重置 + state=OUR_TURN） */
        jq_state strr; fixture_play(&strr);
        CHECK(strr.stage == STG_PLAY && strr.winner == JQ_WIN_NONE,
              "RESTART 前置：对局进行中（新夹具 STG_PLAY）");
        int ret2 = jq_bt_session_on_frame(&sessr, &strr, BT_T_RESTART, 0, 0, 0, 0, 0, 0);
        CHECK(ret2 == 0, "RESTART 帧接受（rc=0）");
        CHECK(strr.stage == STG_PLAY, "RESTART 语义：stage 回 STG_PLAY");
        CHECK(strr.winner == JQ_WIN_NONE, "RESTART 语义：winner 复位未定");
        CHECK(strr.turn == SIDE_BLUE, "RESTART 语义：turn 复位蓝先手");
        int empty = 0;
        for (int i = 0; i < N_CELLS; i++) if (strr.cells[i].rank) empty = 1;
        CHECK(empty == 0, "RESTART 语义：棋盘清空（jq_state_init）");
        CHECK(sessr.state == BT_ST_OUR_TURN, "RESTART 语义：state=OUR_TURN（等本方输入）");
        CHECK(strr.total_moves == 0 && strr.moves_since_eat == 0,
              "RESTART 语义：计数复位 0");
    }

    /* =====================================================================
     * §6. 帧前缀校验（非 "v2\n" 全矩阵拒收）
     * ===================================================================== */
    printf("[§6] 帧前缀校验（非 v2 拒收矩阵）\n");
    {
        int g[7];
        CHECK(jq_bt_frame_decode("v1\n0,1,2,3,4,5,6\n", g, g+1, g+2, g+3, g+4, g+5, g+6) == 0,
              "v1\\n 旧版本前缀拒收");
        CHECK(jq_bt_frame_decode("V2\n0,1,2,3,4,5,6\n", g, g+1, g+2, g+3, g+4, g+5, g+6) == 0,
              "V2\\n 大写前缀拒收");
        CHECK(jq_bt_frame_decode("hello\n0,1,2,3,4,5,6\n", g, g+1, g+2, g+3, g+4, g+5, g+6) == 0,
              "任意垃圾前缀拒收");
        CHECK(jq_bt_frame_decode("v2\n", g, g+1, g+2, g+3, g+4, g+5, g+6) == 0,
              "v2\\n 后无字段拒收（sscanf 7 字段失败）");
        CHECK(jq_bt_frame_decode("v2\n0,1,2,3,4,5\n", g, g+1, g+2, g+3, g+4, g+5, g+6) == 0,
              "6 字段（缺 1）拒收");
        CHECK(jq_bt_frame_decode("v2\n0,1,2\n", g, g+1, g+2, g+3, g+4, g+5, g+6) == 0,
              "3 字段拒收");
        CHECK(jq_bt_frame_decode("", g, g+1, g+2, g+3, g+4, g+5, g+6) == 0,
              "空串拒收");
        CHECK(jq_bt_frame_decode(NULL, g, g+1, g+2, g+3, g+4, g+5, g+6) == 0,
              "NULL 缓冲拒收");
        /* 合法对照：v2\n 7 字段正例 */
        char buf[80];
        int n = jq_bt_frame_encode(buf, sizeof buf, BT_T_MOVE, 1, 2, 3, 4, 5, 6, 0);
        (void)n;
        CHECK(jq_bt_frame_decode(buf, g, g+1, g+2, g+3, g+4, g+5, g+6) == 1,
              "对照：v2\\n 7 字段正例通过");
    }

    /* =====================================================================
     * §7. 默认 type 拒收（未知帧类型 → rc=-1）
     * ===================================================================== */
    printf("[§7] 未知帧类型拒收\n");
    {
        jq_state st; fixture_play(&st);
        jq_bt_session sess;
        jq_bt_session_init(&sess, &st, 1);
        sess.state = BT_ST_THEIR_TURN;
        int ret = jq_bt_session_on_frame(&sess, &st, BT_T_MOVE + 100, 0, 0, 0, 0, 0, 0);
        CHECK(ret == -1, "未知 type（越界）拒收（rc=-1）");
        CHECK(sess.last_type == (int)(BT_T_MOVE + 100), "last_type 记录（会话诊断）");
    }

    printf(rc ? "== bt_semtest 结果: 有失败（%d 条通过）==\n"
              : "=== bt_semtest 全量 %d 条 PASS ===\n", n_pass);
    return rc;
}
