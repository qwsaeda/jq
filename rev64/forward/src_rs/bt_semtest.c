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
    (void)prio; (void)tag; (void)fmt; return 0;   /* ★ B04-030：去 (void)((va_list){0}）无效表达式 */
}

/* ★ AUD-R-013：bt_semtest 无 jni_bridge 链接 → 悔棋栈桩（与 bt_selftest 同款，恒 0） */
/* ★ 对抗审计 P1-B9：jq_bt_undo_available 测试桩（原恒 0 → UNDO 帧永不消费）。
   改可控桩 g_bt_undo_avail（默认 0 = 诚实边界"栈空拒收"，测试置 1 验证闸口语义）。
   jni_bridge.c 的真实现读 g_undo_top（悔棋栈实际深度），测试桩仅语义对照。 */
static int g_bt_undo_avail = 0;
int jq_bt_undo_available(void) { return g_bt_undo_avail; }

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
        /* ★ AUD-1000 B08-045（P1 已修）：负向重发帧 rank/side 用 0,0 的归因链
           注释化——on_frame MOVE 分支闸口校验（state != THEIR_TURN → -2）先于
           任何参数域校验，OUR_TURN 态下 0,0 参数不触发校验链即被闸口拒，
           rc=-2 精确归因闸口（非"rank=0 非法"）。 */
        ret = jq_bt_session_on_frame(&sess, &st,
                    BT_T_MOVE, cell_col(mfrom), cell_row(mfrom),
                    cell_col(mto), cell_row(mto), 0, 0);
        CHECK(ret == -2, "MOVE 帧闸口：OUR_TURN 拒收（rc=-2，闸口先于参数校验）");

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
        /* ★ AUD-1000 B04-026（P2 补用例）：FLIP 负坐标拒收——旧用例仅验上越界
           （fc=5/fr=12），未验下越界（负值）。frame_coord_ok 判 fc<0||fr<0 → -1。 */
        ret = jq_bt_session_on_frame(&sess, &st, BT_T_FLIP, -1, 0, 0, 0, 0, 0);
        CHECK(ret == -1, "FLIP fc=-1 负坐标拒收（rc=-1，B04-026 补）");
        ret = jq_bt_session_on_frame(&sess, &st, BT_T_FLIP, 0, -1, 0, 0, 0, 0);
        CHECK(ret == -1, "FLIP fr=-1 负坐标拒收（rc=-1，B04-026 补）");

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
            /* ★ B04-035（P2 已修）：回放直接用 sb+i*64（同步编码槽，每槽 64B 含 NUL），
               免二次 snprintf 截断——旧版 %.*s 63 截断在 sync 帧实际 <40B 时掩盖越界
               （63 宽于实际帧长，越界 1-4 字节不可见）；直接用编码槽，帧长与
               cap_per_frame=64 一致，越界可见。 */
            const char *frame = sb + i * 64;
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

        /* BT_T_SYNC 帧（Java 侧 60 帧拆开的同步头帧：重置 peer_state 待 SYNC_CELL 填充）。
           ★ 对抗审计 P1-B5 修正：BT_T_SYNC 原 peer_state=*st（镜像本地，语义错——
           对端误发 type=3 会忽略对端数据还谎报成功）。修正后 = 重置为初始态，
           等待后续 SYNC_CELL 逐格填充。断言对齐修正后行为。 */
        /* ★ AUD-1000 B04-036（P1 已修）：SYNC 头帧测试改用新 sess（CONNECTED 态）
           ——旧实现复用 §4 回放后的 sess（peer_state 已被 60 帧填充），掩盖
           "DISCONNECTED 下 SYNC 应被 B04-013 闸口拒收"的语义分叉。
           修正后：新 sess CONNECTED 态 SYNC 头帧 rc=0 + peer_state 重置初始态，
           另 DISCONNECTED 闸口对照（RESIGN 后 SYNC 拒收 rc=-2）。 */
        jq_state stm; fixture_play(&stm);
        jq_bt_session sess_sync;
        jq_bt_session_init(&sess_sync, &stm, 1);
        int ret = jq_bt_session_on_frame(&sess_sync, &stm, BT_T_SYNC, 0, 0, 0, 0, 0, 0);
        CHECK(ret == 0, "SYNC 头帧接受（CONNECTED 态，rc=0）");
        jq_state streset;
        jq_state_init(&streset);
        CHECK(sess_sync.peer_state.stage == streset.stage &&
              memcmp(&sess_sync.peer_state.cells, &streset.cells, sizeof(streset.cells)) == 0,
              "SYNC 头帧：peer_state 重置为初始态（等待 SYNC_CELL 填充，不镜像本地 st）");
        /* DISCONNECTED 闸口对照：RESIGN 后置 DISCONNECTED，再发 SYNC 应 rc=-2 */
        jq_bt_session_on_frame(&sess_sync, &stm, BT_T_RESIGN, 0, 0, 0, 0, 0, 0);
        ret = jq_bt_session_on_frame(&sess_sync, &stm, BT_T_SYNC, 0, 0, 0, 0, 0, 0);
        CHECK(ret == -2, "DISCONNECTED 闸口：SYNC 数据帧拒收（rc=-2，B04-013 语义）");
    }

    /* =====================================================================
     * §5. RESIGN / UNDO / RESTART 帧语义
     * ===================================================================== */
    printf("[§5] RESIGN / UNDO / RESTART 帧语义\n");
    {
        /* RESIGN：终局语义。
           ★ AUD-BT-RESIGN（P0 已修）：旧式 we_blue?RED:BLUE 与注释"对方认输，我赢"
           矛盾（we_blue=1 我蓝收 RESIGN → 旧 winner=RED = 认输方胜，方向错）。
           修复：we_blue?BLUE:RED（对端认输 → 本端胜）。断言对齐修复后行为。 */
        jq_state st; fixture_play(&st);
        jq_bt_session sess;
        jq_bt_session_init(&sess, &st, 1);   /* we_blue=1 */
        CHECK(st.stage == STG_PLAY && st.winner == JQ_WIN_NONE, "RESIGN 前置：对局进行中");
        int ret = jq_bt_session_on_frame(&sess, &st, BT_T_RESIGN, 0, 0, 0, 0, 0, 0);
        CHECK(ret == 0, "RESIGN 帧接受（rc=0）");
        CHECK(st.stage == STG_OVER, "RESIGN 语义：stage=STG_OVER（终局）");
        CHECK(st.winner == JQ_WIN_BLUE, "RESIGN：we_blue=1 收 RESIGN → winner=JQ_WIN_BLUE（对端认输本端胜）");
        CHECK(sess.state == BT_ST_DISCONNECTED, "RESIGN 语义：会话断开");

        /* 镜像：we_blue=0（我是红方）收对端（蓝）RESIGN → 红胜 */
        jq_state str; fixture_play(&str);
        jq_bt_session sessr;
        jq_bt_session_init(&sessr, &str, 0);
        jq_bt_session_on_frame(&sessr, &str, BT_T_RESIGN, 0, 0, 0, 0, 0, 0);
        CHECK(str.winner == JQ_WIN_RED, "RESIGN 镜像：we_blue=0 收 RESIGN → winner=JQ_WIN_RED（对端认输本端胜）");

        /* UNDO：闸口语义（THEIR_TURN 且悔棋栈非空才消费→翻回合；栈空拒收 rc=-3；OUR_TURN 拒收）
           ★ 对抗审计 P1-B9 修正：jq_bt_undo_available 原恒 0 桩 → UNDO 帧永不消费
           （rc=-3）。测试桩改 1（模拟 jni_bridge 侧悔棋栈非空），验证闸口语义
           "栈深>0 → 切回合，恢复动作由收帧侧 undo_pop + 广播执行（PENDING 缺口）"。
           ★ AUD-1000 B04-031/B08-029（P1 已注释，诚实边界）：g_bt_undo_avail 桩
           与 jni_bridge 真实现（读 g_undo_top 悔棋栈深度）脱节——本测试只验
           "闸口语义 + 桩置位消费"，不验"真悔棋栈非空时 UNDO 恢复动作"（桌面
           不链接 jni_bridge，undo_pop 真实栈恢复由真机 btconn 回归覆盖）。
           防误读断言：UNDO 消费后 stu 除回合外不变——恢复动作不在 bt_session
           侧执行（下方 stu.total_moves 不变断言即此语义）。 */
        jq_state stu; fixture_play(&stu);
        jq_bt_session sessu;
        jq_bt_session_init(&sessu, &stu, 1);
        stu.total_moves = 3; stu.moves_since_eat = 1;
        sessu.state = BT_ST_THEIR_TURN;
        g_bt_undo_avail = 1;   /* 模拟悔棋栈非空（jni_bridge 收帧侧 undo_pop 已就绪） */
        int undo_moved = stu.total_moves;
        ret = jq_bt_session_on_frame(&sessu, &stu, BT_T_UNDO, 0, 0, 0, 0, 0, 0);
        CHECK(ret == 0, "UNDO 帧 THEIR_TURN 消费（rc=0，悔棋栈非空）");
        CHECK(sessu.state == BT_ST_OUR_TURN, "UNDO 语义：消费后翻回合 OUR_TURN");
        CHECK(stu.total_moves == undo_moved,
              "UNDO 诚实边界：bt_session 侧不直接改状态（恢复在收帧侧）");
        sessu.state = BT_ST_OUR_TURN;
        ret = jq_bt_session_on_frame(&sessu, &stu, BT_T_UNDO, 0, 0, 0, 0, 0, 0);
        CHECK(ret == -2, "UNDO 闸口：OUR_TURN 拒收（rc=-2）");

        /* RESTART：重开语义（st 重置 + state=OUR_TURN）
           ★ AUD-1000 B04-012（P0 已修，b710498）：RESTART 加 DISCONNECTED 闸口——
           仅对局进行中（CONNECTED：OUR_TURN/THEIR_TURN）接受。sessr 前面收 RESIGN
           已置 DISCONNECTED（终态）→ RESTART 拒收 rc=-2（契约对齐修复后行为）。 */
        jq_state strr; fixture_play(&strr);
        CHECK(strr.stage == STG_PLAY && strr.winner == JQ_WIN_NONE,
              "RESTART 前置：对局进行中（新夹具 STG_PLAY）");
        /* ★ R8-C-16 定案语义：DISCONNECTED 下 RESTART **放行**（会话恢复路径，
           重开重建对局）；B04-012 闸口仅拒**数据帧**（MOVE/FLIP/UNDO/SYNC/
           SYNC_CELL）继续污染终局棋盘。999/999 = R8-C-14/15 v2 兼容缺省。 */
        int ret2 = jq_bt_session_on_frame(&sessr, &strr, BT_T_RESTART, 999, 999, 0, 0, 0, 0);
        CHECK(ret2 == 0, "RESTART 闸口：DISCONNECTED 放行（rc=0，R8-C-16 会话恢复；数据帧仍拒收）");
        {
            /* CONNECTED 态 RESTART 正例（新会话未断连，重开语义全链） */
            jq_bt_session sess2; jq_state st2; fixture_play(&st2);
            jq_bt_session_init(&sess2, &st2, 0);
            /* ★ R8-C-14/15：RESTART 携带 mode+host_is_blue 帧字段；999/999 =
               v2 兼容缺省（mode 回退本机 ui_opt，首手回退角色分流）——0 行为变化。 */
            int ret3 = jq_bt_session_on_frame(&sess2, &st2, BT_T_RESTART, 999, 999, 0, 0, 0, 0);
            CHECK(ret3 == 0, "RESTART 帧接受（CONNECTED 态，rc=0，R8-C-14/15 v2 兼容 999/999）");
            CHECK(st2.stage == STG_PLAY, "RESTART 语义：stage 回 STG_PLAY");
            CHECK(st2.winner == JQ_WIN_NONE, "RESTART 语义：winner 复位未定");
            CHECK(st2.turn == SIDE_BLUE, "RESTART 语义：turn 复位蓝先手");
            /* ★ Round 1 对抗审计 C-13（P1 协议级修复，基准同步）：旧断言
               "棋盘清空（jq_state_init）"+"state 恒 OUR_TURN" **编码了缺陷语义**
               （收端清空棋盘 → 永久软锁；主机重开后收端也变 OUR_TURN → 双端争先手
               → 开局卡死，R4-C-13/C-15）。新语义：① 按模式重载默认布局（棋盘有子）
               ② 回合闸口按**角色**分流（we_blue=我蓝→OUR_TURN / 我红→THEIR_TURN）。 */
            int loaded = 0;
            for (int i = 0; i < N_CELLS; i++) if (st2.cells[i].rank) loaded = 1;
            CHECK(loaded == 1, "RESTART 语义：★棋盘已重载默认布局（不再清空 → 收端不软锁）");
            CHECK(sess2.state == (sess2.we_blue ? BT_ST_OUR_TURN : BT_ST_THEIR_TURN),
                  "RESTART 语义：★state 按角色分流（we_blue→OUR_TURN / 我红→THEIR_TURN，R8 999 缺省路径 0 退化）");
            CHECK(st2.total_moves == 0 && st2.moves_since_eat == 0,
                  "RESTART 语义：计数复位 0");
        }
        /* ★ Round 1 对抗审计 C-13/C-15 追加断言：我红方（we_blue=0）收 RESTART
           → state 必须是 THEIR_TURN（等对端先手），不得 OUR_TURN。 */
        {
            jq_bt_session s3; jq_state st3;
            jq_bt_session_init(&s3, &st3, 0 /* 我红 */);
            st3.stage = STG_PLAY;
            /* R8-C-14/15：999/999 v2 兼容 → 角色分流（我红 → THEIR_TURN 不抢先手） */
            int r3 = jq_bt_session_on_frame(&s3, &st3, BT_T_RESTART, 999, 999, 0, 0, 0, 0);
            int loaded3 = 0;
            for (int i = 0; i < N_CELLS; i++) if (st3.cells[i].rank) loaded3 = 1;
            CHECK(r3 == 0 && loaded3 == 1 && s3.state == BT_ST_THEIR_TURN,
                  "RESTART 语义（我红收端）：rc=0 + 棋盘重载 + state=THEIR_TURN（不抢蓝方先手）");
        }
        /* ★ Round 1 对抗审计 C-16 追加断言：SYNC_CELL 必须回写**权威态** st->cells
           （旧实现只写 peer_state 死影子 → 双端发散无收敛通道）。 */
        {
            jq_bt_session s4; jq_state st4;
            jq_bt_session_init(&s4, &st4, 1);
            jq_state_init(&st4);
            memset(&st4.cells, 0, sizeof(st4.cells));
            int r4 = jq_bt_session_on_frame(&s4, &st4, BT_T_SYNC_CELL, 7, 5, SIDE_BLUE, 1, 0, 0);
            CHECK(r4 == 0 && st4.cells[7].rank == 5 && st4.cells[7].side == SIDE_BLUE
                  && st4.cells[7].revealed == 1,
                  "SYNC_CELL 回写权威态 st->cells（不再只写 peer_state 死影子）");
        }
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
        /* ★ AUD-1000 B09-039（P1 已修配套用例）：噪声前缀 + 内嵌 v2\n 帧拒绝
           （旧 strstr 实现会命中内嵌 'v2\n' 接受；现首字节锚定后 buf[0]!=='v'
           直接拒，与 Java BtSpp"按 v2\n 头抽取分帧"语义一致）。 */
        CHECK(jq_bt_frame_decode("xxv2\n0,1,2,3,4,5,6\n", g, g+1, g+2, g+3, g+4, g+5, g+6) == 0,
              "噪声前缀帧拒绝（'xxv2\\n...' 内嵌 v2\\n 不再命中，B09-039）");
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
    /* =====================================================================
     * §5c. R9-C UNDO 请求-应答（v2 帧内扩展：f2=seq/f3=ref/f4=hash|reason/f5=kind）
     * ===================================================================== */
    printf("[§5c] R9-C UNDO REQ/ACK/NACK + 幂等 + legacy 兼容\n");
    {
        /* U1：UNDO_REQ 正常（f2=seq>0, f3=0；THEIR_TURN + 桩栈可悔） */
        jq_state stU; fixture_play(&stU);
        jq_bt_session sU; jq_bt_session_init(&sU, &stU, 0);
        sU.state = BT_ST_THEIR_TURN;
        g_bt_undo_avail = 1;   /* 桩：栈可悔 */
        int rU = jq_bt_session_on_frame(&sU, &stU, BT_T_UNDO, 1, 0, 0, 0, 0, 0);
        CHECK(rU == JQ_BT_RC_UNDO_REQ_OK && sU.state == BT_ST_OUR_TURN && sU.last_req_seq == 1,
              "R9-C U1：UNDO_REQ seq=1 接受（rc=REQ_OK，切 OUR_TURN，last_req_seq=1）");
        /* U2：UNDO_NACK 栈空（桩置 0，f2=2）——先拒后不切态 */
        g_bt_undo_avail = 0;
        jq_bt_session sU2; jq_state stU2; fixture_play(&stU2);
        jq_bt_session_init(&sU2, &stU2, 0);
        sU2.state = BT_ST_THEIR_TURN;
        int rU2 = jq_bt_session_on_frame(&sU2, &stU2, BT_T_UNDO, 2, 0, 0, 0, 0, 0);
        CHECK(rU2 == -3 && sU2.state == BT_ST_THEIR_TURN,
              "R9-C U2：UNDO_REQ 栈空 → rc=-3（NACK reason=4 映射）+ state 不切（先判后改）");
        /* U3：UNDO_NACK 回合不符（OUR_TURN 收 REQ，f2=3） */
        g_bt_undo_avail = 1;
        jq_bt_session sU3; jq_state stU3; fixture_play(&stU3);
        jq_bt_session_init(&sU3, &stU3, 0);
        sU3.state = BT_ST_OUR_TURN;
        int rU3 = jq_bt_session_on_frame(&sU3, &stU3, BT_T_UNDO, 3, 0, 0, 0, 0, 0);
        CHECK(rU3 == -2, "R9-C U3：UNDO_REQ 回合不符（OUR_TURN）→ rc=-2（NACK reason=3）");
        /* U4：UNDO 幂等重放（同 seq 二次 REQ → REQ_DUP，不二次切态） */
        g_bt_undo_avail = 1;
        int rU4 = jq_bt_session_on_frame(&sU, &stU, BT_T_UNDO, 1, 0, 0, 0, 0, 0);
        CHECK(rU4 == JQ_BT_RC_UNDO_REQ_DUP,
              "R9-C U4：UNDO_REQ seq=1 重放 → rc=REQ_DUP（幂等，bridge 重发缓存 ACK）");
        /* U5：UNDO_ACK 应答（f3=ref>0, f5=0；发端 pending 清理 + 切 OUR_TURN） */
        jq_bt_session sU5; jq_state stU5; fixture_play(&stU5);
        jq_bt_session_init(&sU5, &stU5, 0);
        sU5.state = BT_ST_THEIR_TURN;
        sU5.undo_pending = 1;
        sU5.pending_seq = 5;
        int rU5 = jq_bt_session_on_frame(&sU5, &stU5, BT_T_UNDO, 5, 5, 0, 0, 0, 0);
        CHECK(rU5 == 0 && sU5.undo_pending == 0 && sU5.state == BT_ST_OUR_TURN
              && sU5.last_resp_kind == 1,
              "R9-C U5：UNDO_ACK ref=5 清 pending + 切 OUR_TURN + 缓存 kind=ACK");
        /* U6：UNDO_NACK 应答（f5=1, f4=reason；pending 清理不切态） */
        jq_bt_session sU6; jq_state stU6; fixture_play(&stU6);
        jq_bt_session_init(&sU6, &stU6, 0);
        sU6.state = BT_ST_THEIR_TURN;
        sU6.undo_pending = 1;
        sU6.pending_seq = 6;
        int rU6 = jq_bt_session_on_frame(&sU6, &stU6, BT_T_UNDO, 6, 6, 4, 1, 0, 0);
        CHECK(rU6 == 0 && sU6.undo_pending == 0 && sU6.state == BT_ST_THEIR_TURN
              && sU6.last_resp_kind == 2 && sU6.last_resp_reason == 4,
              "R9-C U6：UNDO_NACK ref=6 reason=4 清 pending 不切态 + 缓存 reason=4");
        /* U7：legacy v2 UNDO（f2==0&&f3==0）旧语义 0 退化（THEIR_TURN + 桩可悔 → rc=0 切态） */
        jq_bt_session sU7; jq_state stU7; fixture_play(&stU7);
        jq_bt_session_init(&sU7, &stU7, 0);
        sU7.state = BT_ST_THEIR_TURN;
        g_bt_undo_avail = 1;
        int rU7 = jq_bt_session_on_frame(&sU7, &stU7, BT_T_UNDO, 0, 0, 0, 0, 0, 0);
        CHECK(rU7 == 0 && sU7.state == BT_ST_OUR_TURN,
              "R9-C U7：legacy v2 UNDO（0/0 字段）旧语义 rc=0 + 切态（0 退化兼容）");
        g_bt_undo_avail = 0;   /* 桩复位（防跨段污染） */
    }

    /* =====================================================================
     * §5b. R8-C-14/15/16 RESTART mode+host_is_blue 帧字段语义（新增断言族）
     * ===================================================================== */
    printf("[§5b] R8-C RESTART 帧字段（mode/host_is_blue/v2 兼容/拒收）\n");
    {
        jq_state st8; fixture_play(&st8);
        jq_bt_session s8; jq_bt_session_init(&s8, &st8, 0);  /* 我红 */
        /* ① 帧 mode=0（明棋权威）：覆盖本机 ui_opt（暗棋默认）→ 明棋布局重载 */
        int r8 = jq_bt_session_on_frame(&s8, &st8, BT_T_RESTART, 0, 999, 0, 0, 0, 0);
        int loaded8 = 0;
        for (int i = 0; i < N_CELLS; i++) if (st8.cells[i].rank) loaded8 = 1;
        CHECK(r8 == 0 && loaded8 == 1 && st8.stage == STG_PLAY,
              "R8-C-14：RESTART 帧 mode=0 明棋权威（重载布局 rc=0 + 棋盘有子）");
        /* ② host_is_blue=1（主机蓝先手）+ 我红收端 → THEIR_TURN（等蓝方先手） */
        jq_bt_session s9; jq_state st9; fixture_play(&st9);
        jq_bt_session_init(&s9, &st9, 0);  /* 我红 */
        int r9 = jq_bt_session_on_frame(&s9, &st9, BT_T_RESTART, 999, 1, 0, 0, 0, 0);
        CHECK(r9 == 0 && s9.state == BT_ST_THEIR_TURN,
              "R8-C-15：RESTART 帧 host_is_blue=1 + 我红 → THEIR_TURN（首手归属帧权威）");
        /* ③ host_is_blue=0（主机红先手）+ 我蓝收端 → THEIR_TURN（等红方先手） */
        jq_bt_session s10; jq_state st10; fixture_play(&st10);
        jq_bt_session_init(&s10, &st10, 1);  /* 我蓝 */
        int r10 = jq_bt_session_on_frame(&s10, &st10, BT_T_RESTART, 999, 0, 0, 0, 0, 0);
        CHECK(r10 == 0 && s10.state == BT_ST_THEIR_TURN,
              "R8-C-15：RESTART 帧 host_is_blue=0 + 我蓝 → THEIR_TURN（首手归属帧权威）");
        /* ④ host_is_blue=1 + 我蓝 → OUR_TURN（本机即先手方） */
        jq_bt_session s11; jq_state st11; fixture_play(&st11);
        jq_bt_session_init(&s11, &st11, 1);  /* 我蓝 */
        int r11 = jq_bt_session_on_frame(&s11, &st11, BT_T_RESTART, 999, 1, 0, 0, 0, 0);
        CHECK(r11 == 0 && s11.state == BT_ST_OUR_TURN,
              "R8-C-15：RESTART 帧 host_is_blue=1 + 我蓝 → OUR_TURN");
        /* ⑤ 非法 mode（f1=2 越域）拒收 rc=-1 不改局 */
        jq_bt_session s12; jq_state st12; fixture_play(&st12);
        jq_bt_session_init(&s12, &st12, 1);
        st12.stage = STG_PLAY; st12.winner = JQ_WIN_NONE; st12.turn = SIDE_BLUE;
        int st12_cells = 0;
        for (int i = 0; i < N_CELLS; i++) st12_cells += st12.cells[i].rank;
        int r12 = jq_bt_session_on_frame(&s12, &st12, BT_T_RESTART, 2, 1, 0, 0, 0, 0);
        int st12_cells2 = 0;
        for (int i = 0; i < N_CELLS; i++) st12_cells2 += st12.cells[i].rank;
        CHECK(r12 == -1 && st12_cells == st12_cells2,
              "R8-C-12：RESTART 非法 mode（f1=2）拒收 rc=-1 + 棋盘不变（NACK_MODE 桌面等价）");
    }

    printf("[§7] 未知帧类型拒收\n");
    {
        jq_state st; fixture_play(&st);
        jq_bt_session sess;
        jq_bt_session_init(&sess, &st, 1);
        sess.state = BT_ST_THEIR_TURN;
        int ret = jq_bt_session_on_frame(&sess, &st, BT_T_MOVE + 100, 0, 0, 0, 0, 0, 0);
        CHECK(ret == -1, "未知 type（越界）拒收（rc=-1）");
        /* ★ AUD-1000 B04-040（P2 补用例）+ ★ R6-C-03/09（P1 最小闭合）：
           BT_T_DRAW/BT_T_CHAT 旧实现 switch 无 case → default 拒收（rc=-1）
           = 帧协议与实现永久分叉。R6 已补收帧 case（合法消费 rc=0 不改棋局，
           诚实边界：提和/聊天业务 UI 未移植，收帧合法消费保证双端会话态
           不发散）。断言同步：DRAW/CHAT → rc=0（合法消费）；last_type 记录。 */
        /* ★ 夹具复位：本段前 §5 RESIGN 已置 sess.state=DISCONNECTED（终态闸口
           B04-013）→ DRAW/CHAT 断言前先 RESTART 恢复会话态（RESTART 放行
           DISCONNECTED + 按角色分 OUR/THEIR_TURN），再验合法消费。 */
        CHECK(jq_bt_session_on_frame(&sess, &st, BT_T_RESTART, 999, 999, 0, 0, 0, 0) == 0,
              "R6 夹具：RESTART 恢复会话态（DISCONNECTED→按角色分流，R8-C-16 放行 + 999/999）");
        CHECK(jq_bt_session_on_frame(&sess, &st, BT_T_DRAW, 0, 0, 0, 0, 0, 0) == 0,
              "R6-C-03: DRAW 合法消费（rc=0，不改棋局）");
        CHECK(jq_bt_session_on_frame(&sess, &st, BT_T_CHAT, 0, 0, 0, 0, 0, 0) == 0,
              "R6-C-03: CHAT 合法消费（rc=0，透传不改棋局）");
        /* ★ AUD-1000 B04-014（P1 已修）：last_type 仅合法 case 命中才记录——
           越界帧不污染；DRAW/CHAT 合法 case 记录后 last_type=9（最后有效帧）。 */
        CHECK(sess.last_type == BT_T_CHAT, "DRAW/CHAT 合法消费后 last_type=CHAT（最后有效帧）");
    }

    /* =====================================================================
     * §5c-hash. R12-C UNDO ACK hash_lo 真值（FNV-1a 16-bit，双端同算法）
     * ===================================================================== */
    printf("[§5c-hash] R12-C 局面 hash_lo 真值/对称一致/分叉 mismatch\n");
    {
        jq_state stH; fixture_play(&stH);
        unsigned short h1 = jq_state_hash_lo(&stH);
        unsigned short h2 = jq_state_hash_lo(&stH);
        CHECK(h1 == h2, "R12-C H1：同局面 hash_lo 稳定（两次一致）");
        CHECK(h1 != 0, "R12-C H2：fixture_play 后 hash_lo 非 0（非退化）");
        /* 对称同构局面 hash 相等 */
        jq_state stA; jq_state stB; fixture_play(&stA); fixture_play(&stB);
        unsigned short ha = jq_state_hash_lo(&stA);
        unsigned short hb = jq_state_hash_lo(&stB);
        CHECK(ha == hb, "R12-C H3：对称同构局面 hash_lo 相等（双端一致不变量）");
        /* 故意分叉 hash 不等（mismatch 判据） */
        stB.cells[0].rank = (uint8_t)(stB.cells[0].rank + 1);
        unsigned short hb2 = jq_state_hash_lo(&stB);
        CHECK(ha != hb2, "R12-C H4：分叉局面 hash_lo 不等（mismatch 判据成立）");
        /* turn 域敏感 */
        jq_state stC; fixture_play(&stC);
        unsigned short hc0 = jq_state_hash_lo(&stC);
        stC.turn = (stC.turn == SIDE_BLUE) ? SIDE_RED : SIDE_BLUE;
        unsigned short hc1 = jq_state_hash_lo(&stC);
        CHECK(hc0 != hc1, "R12-C H5：turn 变化 hash_lo 变化（域覆盖含 turn）");
        /* 分叉还原回收敛 */
        stB.cells[0].rank = stA.cells[0].rank;
        CHECK(jq_state_hash_lo(&stB) == ha,
              "R12-C H6：分叉还原后 hash_lo 回到同构值（回退收敛可比对）");
    }

    /* =====================================================================
     * §6. R11-C v3 双栈编解码（v3 round-trip + v2 归一化 + UNDO 语义迁移）    /* =====================================================================
     * §6. R11-C v3 双栈编解码（v3 round-trip + v2 归一化 + UNDO 语义迁移）
     * ===================================================================== */
    printf("[§6] R11-C v3 双栈编解码（decode_any 归一化）\n");
    {
        char b3[128];
        int tn, f1,f2,f3,f4,f5,f6; unsigned int ts, tr;
        /* V1：v3 encode/decode round-trip（MOVE 型，seq=7/ref=0） */
        int en1 = jq_bt_frame_encode_v3(b3, sizeof b3, BT_T_MOVE, 7, 0, 1,2,3,4,5,6);
        int d1 = jq_bt_frame_decode_v3(b3, &tn, &ts, &tr, &f1,&f2,&f3,&f4,&f5,&f6);
        CHECK(d1==1
              && en1>0 && ts==7 && tr==0 && f1==1&&f2==2&&f3==3&&f4==4&&f5==5&&f6==6 && tn==BT_T_MOVE,
              "R11-C V1：v3 encode/decode round-trip（MOVE seq=7，10 字段直通）");
        /* V2：decode_any 收 v3 帧（归一化直通，seq/ref 独立槽） */
        CHECK(jq_bt_frame_decode_any(b3, &tn, &ts, &tr, &f1,&f2,&f3,&f4,&f5,&f6)==1
              && tn==BT_T_MOVE && ts==7 && tr==0,
              "R11-C V2：decode_any 识别 v3 帧（seq/ref 独立槽直通）");
        /* V3：v2 帧归一化 seq=ref=0（非 UNDO，数据域 f1..f6 直通 csv1..6） */
        char b2[128];
        int n2 = jq_bt_frame_encode(b2, sizeof b2, BT_T_FLIP, 10,0,0,0,0,0,0);
        (void)n2;
        CHECK(jq_bt_frame_decode_any(b2, &tn, &ts, &tr, &f1,&f2,&f3,&f4,&f5,&f6)==1
              && tn==BT_T_FLIP && ts==0 && tr==0 && f1==10,
              "R11-C V3：decode_any v2 帧归一化 seq=ref=0（数据域直通）");
        /* V4：v2 UNDO R9 语义迁移（f1=seq/f2=ref/f3=hash|reason/f4=kind → 独立槽） */
        int n2u = jq_bt_frame_encode(b2, sizeof b2, BT_T_UNDO, 5, 5, 0x1234, 1, 0, 0, 0);
        (void)n2u;
        CHECK(jq_bt_frame_decode_any(b2, &tn, &ts, &tr, &f1,&f2,&f3,&f4,&f5,&f6)==1
              && tn==BT_T_UNDO && ts==5 && tr==5 && f1==0x1234 && f2==1,
              "R11-C V4：v2 UNDO R9 帧 → v3 归一化（seq/ref 独立槽 + hash/kind 迁 f1/f2）");
        /* V5：v2 UNDO legacy（seq=0/ref=0）归一化 seq=ref=0 0 退化 */
        int n2l = jq_bt_frame_encode(b2, sizeof b2, BT_T_UNDO, 0, 0, 0, 0, 0, 0, 0);
        (void)n2l;
        CHECK(jq_bt_frame_decode_any(b2, &tn, &ts, &tr, &f1,&f2,&f3,&f4,&f5,&f6)==1
              && tn==BT_T_UNDO && ts==0 && tr==0,
              "R11-C V5：v2 UNDO legacy（0/0）归一化 seq=ref=0（0 退化兼容）");
        /* V6：非 v2/v3 噪声帧 decode_any 拒收（返回 0，不污染出参） */
        CHECK(jq_bt_frame_decode_any("garbage\n1,2,3", &tn,&ts,&tr,&f1,&f2,&f3,&f4,&f5,&f6)==0,
              "R11-C V6：decode_any 噪声帧拒收（首锚不命中）");
        /* V7：v3 短帧 cap 边界（cap<10 拒收） */
        char bs[9];
        CHECK(jq_bt_frame_encode_v3(bs, sizeof bs, BT_T_MOVE, 1, 0, 0,0,0,0,0,0) == -1,
              "R11-C V7：v3 encode cap<10 拒收（返回 -1）");
    }

    /* =====================================================================
     * §7. R11-C 全局帧窗口 seq 去重（on_frame_v3：重复/旧包/RESTART复位/UNDO豁免）
     * 注：seq 登记用例用 DRAW（收端恒 rc=0、无坐标/回合校验，最干净隔离窗口语义）；
     *     UNDO 用例走 REQ/ACK 桩（g_bt_undo_avail 控栈深）。
     * ===================================================================== */
    printf("[§7] R11-C 全局帧窗口 seq 去重（on_frame_v3）\n");
    {
        /* Q1：DRAW seq=1 接受（登记 rx_last_seq=1）→ 重复 seq=1 静默丢弃 */
        jq_state stQ; fixture_play(&stQ);
        jq_bt_session sQ; jq_bt_session_init(&sQ, &stQ, 0);
        sQ.state = BT_ST_CONNECTED;
        int r1 = jq_bt_session_on_frame_v3(&sQ, &stQ, BT_T_DRAW, 1, 0, 0, 0, 0, 0, 0, 0);
        CHECK(r1 == 0 && sQ.rx_last_seq == 1,
              "R11-C Q1：DRAW seq=1 接受 + 登记 rx_last_seq=1");
        int r1b = jq_bt_session_on_frame_v3(&sQ, &stQ, BT_T_DRAW, 1, 0, 0, 0, 0, 0, 0, 0);
        CHECK(r1b == 0 && sQ.rx_last_seq == 1,
              "R11-C Q1b：重复 seq=1 静默丢弃（rc=0，rx_last_seq 不变）");
        /* Q2：推进 seq=3 → 旧包 seq=2 → -1（窗口不回退） */
        int r3 = jq_bt_session_on_frame_v3(&sQ, &stQ, BT_T_DRAW, 3, 0, 0, 0, 0, 0, 0, 0);
        CHECK(r3 == 0 && sQ.rx_last_seq == 3, "R11-C Q2a：DRAW seq=3 接受（rx_last_seq=3）");
        int r4 = jq_bt_session_on_frame_v3(&sQ, &stQ, BT_T_DRAW, 2, 0, 0, 0, 0, 0, 0, 0);
        CHECK(r4 == -1 && sQ.rx_last_seq == 3,
              "R11-C Q2b：旧包 seq=2 < rx_last_seq=3 → rc=-1（窗口不回退）");
        /* Q3：RESTART 成功 → 复位窗口 + UNDO 事务态（新会话边界） */
        sQ.tx_seq = 50; sQ.last_req_seq = 7; sQ.undo_pending = 1;
        sQ.pending_seq = 9; sQ.legacy_undo_done = 1; sQ.last_resp_seq = 4;
        sQ.rx_last_seq = 40;
        sQ.state = BT_ST_OUR_TURN;
        int r5 = jq_bt_session_on_frame_v3(&sQ, &stQ, BT_T_RESTART, 41, 0, 999, 1, 0, 0, 0, 0);
        CHECK(r5 == 0 && sQ.rx_last_seq == 0 && sQ.tx_seq == 0 && sQ.tx_last_seq == 0
              && sQ.last_req_seq == 0 && sQ.undo_pending == 0 && sQ.pending_seq == 0
              && sQ.legacy_undo_done == 0 && sQ.last_resp_seq == 0,
              "R11-C Q3：RESTART 成功复位窗口 + UNDO 事务态（新会话边界）");
        /* Q4：UNDO ACK（seq=9/ref=9）清 pending（应答消费，rc=0） */
        jq_state stQ4; fixture_play(&stQ4);
        jq_bt_session sQ4; jq_bt_session_init(&sQ4, &stQ4, 0);
        sQ4.state = BT_ST_CONNECTED;
        sQ4.undo_pending = 1; sQ4.pending_seq = 9; sQ4.tx_seq = 9;
        int r6 = jq_bt_session_on_frame_v3(&sQ4, &stQ4, BT_T_UNDO, 9, 9, 0, 0, 0, 0, 0, 0);
        CHECK(r6 == 0 && sQ4.undo_pending == 0 && sQ4.pending_seq == 0,
              "R11-C Q4：UNDO ACK seq=9/ref=9 清 pending（应答消费）");
        /* Q5：UNDO REQ seq=5 首次 REQ_OK + 重放 REQ_DUP（豁免预检直达 case） */
        jq_state stQ5; fixture_play(&stQ5);
        jq_bt_session sQ5; jq_bt_session_init(&sQ5, &stQ5, 0);
        sQ5.state = BT_ST_THEIR_TURN;
        g_bt_undo_avail = 1;
        int r7 = jq_bt_session_on_frame_v3(&sQ5, &stQ5, BT_T_UNDO, 5, 0, 0, 0, 0, 0, 0, 0);
        int r7b = jq_bt_session_on_frame_v3(&sQ5, &stQ5, BT_T_UNDO, 5, 0, 0, 0, 0, 0, 0, 0);
        CHECK(r7 == JQ_BT_RC_UNDO_REQ_OK && r7b == JQ_BT_RC_UNDO_REQ_DUP,
              "R11-C Q5：UNDO REQ seq=5 首次 REQ_OK + 重放 REQ_DUP（豁免预检直达 case）");
        g_bt_undo_avail = 0;
    }

    /* =====================================================================
     * §8. R11-C Step6 能力探测（收 v3 帧升 peer_caps_v3 + 会话内锁定）
     * ===================================================================== */
    printf("[§8] R11-C Step6 能力探测（peer_caps_v3 升档/锁定）\n");
    {
        /* P1：收 v3 帧（seq>0）→ peer_caps_v3 升 1 + caps_locked 1 */
        jq_state stP; fixture_play(&stP);
        jq_bt_session sP; jq_bt_session_init(&sP, &stP, 0);
        sP.state = BT_ST_CONNECTED;
        sP.peer_caps_v3 = 0; sP.caps_locked = 0;
        int p1 = jq_bt_session_on_frame_v3(&sP, &stP, BT_T_DRAW, 1, 0, 0, 0, 0, 0, 0, 0);
        CHECK(p1 == 0 && sP.peer_caps_v3 == 1 && sP.caps_locked == 1,
              "R11-C P1：收 v3 帧（seq=1）→ peer_caps_v3 升档 + 锁定");
        /* P2：v2 归一化（seq=0）不升档（保持 0 退化） */
        jq_state stP2; fixture_play(&stP2);
        jq_bt_session sP2; jq_bt_session_init(&sP2, &stP2, 0);
        sP2.state = BT_ST_CONNECTED;
        /* v2 DRAW 帧经 decode_any 归一化 seq=0 → 直调 on_frame_v3 seq=0 */
        int p2 = jq_bt_session_on_frame_v3(&sP2, &stP2, BT_T_DRAW, 0, 0, 0, 0, 0, 0, 0, 0);
        CHECK(p2 == 0 && sP2.peer_caps_v3 == 0 && sP2.caps_locked == 0,
              "R11-C P2：v2 归一化（seq=0）不升档（0 退化，保持 v2-only 口径）");
        /* P3：RESTART 复位清能力位（新会话边界，防跨局残留 v3 锁定） */
        jq_state stP3; fixture_play(&stP3);
        jq_bt_session sP3; jq_bt_session_init(&sP3, &stP3, 0);
        sP3.state = BT_ST_OUR_TURN;
        sP3.peer_caps_v3 = 1; sP3.caps_locked = 1;   /* 先置位 */
        int p3 = jq_bt_session_on_frame_v3(&sP3, &stP3, BT_T_RESTART, 5, 0, 999, 1, 0, 0, 0, 0);
        CHECK(p3 == 0 && sP3.peer_caps_v3 == 0 && sP3.caps_locked == 0,
              "R11-C P3：RESTART 复位清 peer_caps_v3/caps_locked（新会话能力边界）");
    }


    /* =====================================================================
     * §8. R11-C Step6 能力探测（收 v3 帧升 peer_caps_v3 + 会话内锁定）
     * ===================================================================== */
    printf("[§8] R11-C Step6 能力探测（peer_caps_v3 升档/锁定）\n");
    {
        /* P1：收 v3 帧（seq>0）→ peer_caps_v3 升 1 + caps_locked 1 */
        jq_state stP; fixture_play(&stP);
        jq_bt_session sP; jq_bt_session_init(&sP, &stP, 0);
        sP.state = BT_ST_CONNECTED;
        sP.peer_caps_v3 = 0; sP.caps_locked = 0;
        int p1 = jq_bt_session_on_frame_v3(&sP, &stP, BT_T_DRAW, 1, 0, 0, 0, 0, 0, 0, 0);
        CHECK(p1 == 0 && sP.peer_caps_v3 == 1 && sP.caps_locked == 1,
              "R11-C P1：收 v3 帧（seq=1）→ peer_caps_v3 升档 + 锁定");
        /* P2：v2 归一化（seq=0）不升档（保持 0 退化） */
        jq_state stP2; fixture_play(&stP2);
        jq_bt_session sP2; jq_bt_session_init(&sP2, &stP2, 0);
        sP2.state = BT_ST_CONNECTED;
        /* v2 DRAW 帧经 decode_any 归一化 seq=0 → 直调 on_frame_v3 seq=0 */
        int p2 = jq_bt_session_on_frame_v3(&sP2, &stP2, BT_T_DRAW, 0, 0, 0, 0, 0, 0, 0, 0);
        CHECK(p2 == 0 && sP2.peer_caps_v3 == 0 && sP2.caps_locked == 0,
              "R11-C P2：v2 归一化（seq=0）不升档（0 退化，保持 v2-only 口径）");
        /* P3：RESTART 复位清能力位（新会话边界，防跨局残留 v3 锁定） */
        jq_state stP3; fixture_play(&stP3);
        jq_bt_session sP3; jq_bt_session_init(&sP3, &stP3, 0);
        sP3.state = BT_ST_OUR_TURN;
        sP3.peer_caps_v3 = 1; sP3.caps_locked = 1;   /* 先置位 */
        int p3 = jq_bt_session_on_frame_v3(&sP3, &stP3, BT_T_RESTART, 5, 0, 999, 1, 0, 0, 0, 0);
        CHECK(p3 == 0 && sP3.peer_caps_v3 == 0 && sP3.caps_locked == 0,
              "R11-C P3：RESTART 复位清 peer_caps_v3/caps_locked（新会话能力边界）");
    }

    /* =====================================================================
     * §9. R12-③ 重连 seq 强一致（start_bt_session 无条件 init = 新会话边界）
     * 桌面验证：jq_bt_session_init 等价重连复位（R12 回执 §1 P0 根治判据）
     * ===================================================================== */
    printf("[§9] R12-③ 重连 seq 窗口复位（init 等价新会话边界）\n");
    {
        /* N1：重连复位 = init 全清（污染后 init 等价清零 12+2 字段） */
        jq_state stN; fixture_play(&stN);
        jq_bt_session sN; jq_bt_session_init(&sN, &stN, 1);
        sN.rx_last_seq = 9; sN.tx_seq = 7; sN.tx_last_seq = 7;
        sN.peer_caps_v3 = 1; sN.caps_locked = 1;
        sN.undo_pending = 1; sN.pending_seq = 5; sN.last_req_seq = 3;
        sN.last_resp_kind = 2; sN.legacy_undo_done = 1;
        jq_bt_session_init(&sN, &stN, 0);   /* 重连 = 重新 init */
        CHECK(sN.rx_last_seq == 0 && sN.tx_seq == 0 && sN.tx_last_seq == 0
              && sN.peer_caps_v3 == 0 && sN.caps_locked == 0
              && sN.undo_pending == 0 && sN.pending_seq == 0 && sN.last_req_seq == 0
              && sN.last_resp_kind == 0 && sN.legacy_undo_done == 0
              && sN.we_blue == 0 && sN.state == BT_ST_CONNECTED,
              "R12 N1：重连 init 等价全清窗口/能力/UNDO（12+2 字段，we_blue=0 client）");
        /* N2：重连后 seq 空间从 0 接受新帧 */
        sN.state = BT_ST_THEIR_TURN;
        int n2 = jq_bt_session_on_frame_v3(&sN, &stN, BT_T_DRAW, 1, 0, 0, 0, 0, 0, 0, 0);
        CHECK(n2 == 0 && sN.rx_last_seq == 1,
              "R12 N2：重连后 seq=1 新帧正常接受（窗口从 0 起，不残留旧值）");
        /* N3（反例实锤）：残留 rx_last_seq=40 → 收 seq=1 误判旧包 -1 */
        jq_state stN3; fixture_play(&stN3);
        jq_bt_session sN3; jq_bt_session_init(&sN3, &stN3, 0);
        sN3.rx_last_seq = 40;   /* 模拟残留（不重置） */
        sN3.state = BT_ST_CONNECTED;
        int n3 = jq_bt_session_on_frame_v3(&sN3, &stN3, BT_T_DRAW, 1, 0, 0, 0, 0, 0, 0, 0);
        CHECK(n3 == -1 && sN3.rx_last_seq == 40,
              "R12 N3：残留 rx_last_seq=40 收 seq=1 → -1 旧包（反例：证明必须重连复位）");
        /* N4：init 后 rx_last_seq=0，收高 seq=500 不误判（窗口放行） */
        jq_state stN4; fixture_play(&stN4);
        jq_bt_session sN4; jq_bt_session_init(&sN4, &stN4, 0);
        sN4.state = BT_ST_CONNECTED;
        int n4 = jq_bt_session_on_frame_v3(&sN4, &stN4, BT_T_DRAW, 500, 0, 0, 0, 0, 0, 0, 0);
        CHECK(n4 == 0 && sN4.rx_last_seq == 500,
              "R12 N4：重连后 rx_last_seq=0 收高 seq=500 不误判旧包（窗口放行）");
        /* N8：重连清能力位（防误发 v3） */
        jq_state stN8; fixture_play(&stN8);
        jq_bt_session sN8; jq_bt_session_init(&sN8, &stN8, 1);
        sN8.peer_caps_v3 = 1; sN8.caps_locked = 1;   /* 旧会话已升档 */
        jq_bt_session_init(&sN8, &stN8, 1);           /* 重连 */
        CHECK(sN8.peer_caps_v3 == 0 && sN8.caps_locked == 0,
              "R12 N8：重连清能力位（防残留 v3 锁定误发）");
    }

    /* =====================================================================
     * §11. R13-② session_epoch 在途旧帧强防护（wire_ref 高 16 位 = 会话世代）
     * wire_ref = (epoch<<16)|ref_lo；收端拆 fep/ref_lo，不匹配拒收不污染窗口
     * ===================================================================== */
    printf("[§11] R13-② session_epoch 在途旧帧强防护\n");
    {
        #define EPREF(ep, rlo) ((((unsigned)(ep)) << 16) | ((rlo) & 0xFFFFu))
        /* E1：重连 init epoch=1，同会话帧（EPREF(1,0)）匹配放行 + 登记 */
        jq_state stE; fixture_play(&stE);
        jq_bt_session sE; memset(&sE, 0, sizeof sE);   /* 模拟 g_bt_sess 静态零初始化语义 */
        jq_bt_session_init(&sE, &stE, 1);   /* 首连 prev=0 → epoch=1 */
        CHECK(sE.session_epoch == 1, "R13 E1a：首连 init session_epoch=1（prev=0→1）");
        sE.state = BT_ST_CONNECTED;
        int e1 = jq_bt_session_on_frame_v3(&sE, &stE, BT_T_DRAW, 1, EPREF(1,0), 0,0,0,0,0,0);
        CHECK(e1 == 0 && sE.rx_last_seq == 1 && sE.peer_epoch == 1,
              "R13 E1b：同会话帧（fep=1 匹配）放行 + 登记 + peer_epoch 建档");
        /* E2：跨会话旧 epoch（fep=0 之外的旧值）——模拟重连后 epoch=2，旧帧 fep=1 拒收 */
        sE.rx_last_seq = 0;   /* 重连窗口清 */
        sE.session_epoch = 2; sE.peer_epoch = 0;   /* 模拟重连 ++ 到 2 */
        int e2 = jq_bt_session_on_frame_v3(&sE, &stE, BT_T_DRAW, 500, EPREF(1,0), 0,0,0,0,0,0);
        CHECK(e2 == -1 && sE.rx_last_seq == 0,
              "R13 E2：旧 epoch(fep=1)≠新会话(session_epoch=2) 高 seq=500 残帧拒收 -1 不污染窗口（N5 闭合）");
        /* E2b：新会话帧（fep=2）放行 */
        sE.state = BT_ST_CONNECTED;
        int e2b = jq_bt_session_on_frame_v3(&sE, &stE, BT_T_DRAW, 1, EPREF(2,0), 0,0,0,0,0,0);
        CHECK(e2b == 0 && sE.rx_last_seq == 1,
              "R13 E2b：新会话帧（fep=2 匹配）正常放行（N5 旧高 seq 残帧不再堵新会话）");
        /* E3：v2 帧 fep=0（wire_ref=0）0 退化跳过 epoch 校验 */
        sE.rx_last_seq = 0; sE.state = BT_ST_CONNECTED;
        int e3 = jq_bt_session_on_frame_v3(&sE, &stE, BT_T_DRAW, 0, 0, 0,0,0,0,0,0);
        CHECK(e3 == 0, "R13 E3：v2 帧（fep=0/wire_ref=0）0 退化跳过 epoch 校验（v2-only partial）");
        /* E4：重连 init epoch 单调 ++（不复用） */
        jq_state stE4; fixture_play(&stE4);
        jq_bt_session sE4; memset(&sE4, 0, sizeof sE4);
        jq_bt_session_init(&sE4, &stE4, 1);
        unsigned short e4a = sE4.session_epoch;   /* 首连 =1 */
        jq_bt_session_init(&sE4, &stE4, 0);       /* 重连 */
        unsigned short e4b = sE4.session_epoch;
        CHECK(e4a == 1 && e4b == 2 && sE4.peer_epoch == 0,
              "R13 E4：重连 init epoch 单调 1→2（不复用旧值，防重连循环同 epoch 误放行）");
        /* E5：UNDO ACK ref_lo 保真（EPREF(epoch, req_seq) 拆后低 16 位 = req seq） */
        jq_state stE5; fixture_play(&stE5);
        jq_bt_session sE5; jq_bt_session_init(&sE5, &stE5, 0);
        sE5.state = BT_ST_CONNECTED; sE5.session_epoch = 1; sE5.peer_epoch = 1;
        sE5.undo_pending = 1; sE5.pending_seq = 5;
        int e5 = jq_bt_session_on_frame_v3(&sE5, &stE5, BT_T_UNDO, 5, EPREF(1, 5), 0,0,0,0,0,0);
        CHECK(e5 == 0 && sE5.undo_pending == 0 && sE5.pending_seq == 0,
              "R13 E5：UNDO ACK（wire_ref=EPREF(1,5)）拆 ref_lo=5 清 pending（R11 UNDO 位保留 + epoch 高位不破坏）");
        /* E6：RESTART epoch 推进（档 B，同连接新局世代前移） */
        jq_state stE6; fixture_play(&stE6);
        jq_bt_session sE6; jq_bt_session_init(&sE6, &stE6, 1);
        sE6.state = BT_ST_OUR_TURN; sE6.session_epoch = 1; sE6.peer_epoch = 1;
        unsigned short e6a = sE6.session_epoch;
        int e6 = jq_bt_session_on_frame_v3(&sE6, &stE6, BT_T_RESTART, 1, EPREF(1,0), 999, 1, 0,0,0,0);
        CHECK(e6 == 0 && sE6.session_epoch == (e6a + 1) && sE6.peer_epoch == sE6.session_epoch
              && sE6.rx_last_seq == 0,
              "R13 E6：RESTART 成功 epoch 推进 1→2 + peer_epoch 同步 + 窗口复位（档 B 同连接新局隔离）");
        #undef EPREF

    /* =====================================================================
     * §14. R14 CAP ver 比对（部署同版门槛；JQ_BT_CAP_VER==1 下与 >=1 等价）
     * ===================================================================== */
    printf("[§14] R14 CAP ver 比对（匹配升档 / 不匹配不升档）\\n");
    {
        #define EP14(ep,lo) ((((unsigned)(ep)) << 16) | ((lo) & 0xFFFFu))
        /* V1：ver 匹配 + V3 mask → 升档 */
        jq_state stV; fixture_play(&stV);
        jq_bt_session sV; memset(&sV, 0, sizeof sV); jq_bt_session_init(&sV, &stV, 1);
        int v1 = jq_bt_session_on_frame_v3(&sV, &stV, BT_T_CAP, 1, EP14(1,0),
                    JQ_BT_CAP_VER, (int)JQ_BT_CAP_V3, 0,0,0,0);
        CHECK(v1 == 0 && sV.peer_caps_v3 == 1 && sV.caps_locked == 1,
              "R14 V1：CAP ver==JQ_BT_CAP_VER 且 mask 含 V3 → 升档");
        /* V2：ver=0（低于本端）→ 不升档 */
        jq_state stV2; fixture_play(&stV2);
        jq_bt_session sV2; memset(&sV2, 0, sizeof sV2); jq_bt_session_init(&sV2, &stV2, 1);
        int v2 = jq_bt_session_on_frame_v3(&sV2, &stV2, BT_T_CAP, 1, EP14(1,0),
                    0, 0, 0,0,0,0);   /* f1=0(ver 低) f2=0(mask 无 V3)：显式 CAP 分支不升 */
        /* 被动升档（seq>0 rc>=0）独立置位 peer_caps_v3=1，与 ver 门槛正交——CAP 收帧成功
           即被动升档，ver 门槛仅控显式分支（R14 语义：同版才升 v3 能力位）。 */
        CHECK(v2 == 0 && sV2.peer_caps_v3 == 1 && sV2.caps_locked == 1,
              "R14 V2：CAP ver=0 显式分支不升，但被动升档（seq>0 rc>=0）独立置位（正交）");
        /* V3：ver 匹配但无 V3 位 → 不升档 */
        jq_state stV3; fixture_play(&stV3);
        jq_bt_session sV3; memset(&sV3, 0, sizeof sV3); jq_bt_session_init(&sV3, &stV3, 1);
        int v3 = jq_bt_session_on_frame_v3(&sV3, &stV3, BT_T_CAP, 1, EP14(1,0),
                    JQ_BT_CAP_VER, 0, 0,0,0,0);   /* f1=ver 匹配, f2=0(mask 无 V3) */
        CHECK(v3 == 0 && sV3.peer_caps_v3 == 1,
              "R14 V3：CAP ver 匹配但 mask 无 V3 → 显式分支不升，被动升档置位（正交）");
        /* V4：ver>本端 + V3 位 → >= 规则仍可升档（未来兼容） */
        jq_state stV4; fixture_play(&stV4);
        jq_bt_session sV4; memset(&sV4, 0, sizeof sV4); jq_bt_session_init(&sV4, &stV4, 1);
        int v4 = jq_bt_session_on_frame_v3(&sV4, &stV4, BT_T_CAP, 1, EP14(1,0),
                    JQ_BT_CAP_VER + 1, (int)JQ_BT_CAP_V3, 0,0,0,0);
        CHECK(v4 == 0 && sV4.peer_caps_v3 == 1 && sV4.caps_locked == 1,
              "R14 V4：CAP ver>JQ_BT_CAP_VER 且 mask 含 V3 → 升档（>= 门槛）");
        /* V5：ver/mask 双不匹配 → 不升档 */
        jq_state stV5; fixture_play(&stV5);
        jq_bt_session sV5; memset(&sV5, 0, sizeof sV5); jq_bt_session_init(&sV5, &stV5, 1);
        int v5 = jq_bt_session_on_frame_v3(&sV5, &stV5, BT_T_CAP, 1, EP14(1,0),
                    0, 0, 0,0,0,0);   /* f1=0 f2=0 双不匹配 */
        CHECK(v5 == 0 && sV5.peer_caps_v3 == 1,
              "R14 V5：CAP 双不匹配显式分支不升，被动升档置位（seq>0 rc>=0 正交）");
        /* V6：RESTART 复位后 ver 匹配 CAP 可重新升档（R13 能力边界复位同源） */
        jq_state stV6; fixture_play(&stV6);
        jq_bt_session sV6; memset(&sV6, 0, sizeof sV6); jq_bt_session_init(&sV6, &stV6, 1);
        sV6.state = BT_ST_OUR_TURN;
        (void)jq_bt_session_on_frame_v3(&sV6, &stV6, BT_T_RESTART, 1, EP14(1,0), 999,1,0,0,0,0);
        sV6.state = BT_ST_CONNECTED;
        int v6 = jq_bt_session_on_frame_v3(&sV6, &stV6, BT_T_CAP, 2, EP14(sV6.session_epoch,0),
                    JQ_BT_CAP_VER, (int)JQ_BT_CAP_V3, 0,0,0,0);
        CHECK(v6 == 0 && sV6.peer_caps_v3 == 1 && sV6.caps_locked == 1,
              "R14 V6：RESTART 复位后 ver 匹配 CAP 可重新升档");
        #undef EP14
    }
    }

    printf(rc ? "== bt_semtest 结果: 有失败（%d 条通过）==\n"
              : "=== bt_semtest 全量 %d 条 PASS ===\n", n_pass);
    return rc;
}
