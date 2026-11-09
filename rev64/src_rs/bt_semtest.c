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
        int ret2 = jq_bt_session_on_frame(&sessr, &strr, BT_T_RESTART, 0, 0, 0, 0, 0, 0);
        CHECK(ret2 == -2, "RESTART 闸口：DISCONNECTED（RESIGN 终态后）拒收（rc=-2，B04-012）");
        {
            /* CONNECTED 态 RESTART 正例（新会话未断连，重开语义全链） */
            jq_bt_session sess2; jq_state st2; fixture_play(&st2);
            jq_bt_session_init(&sess2, &st2, 0);
            int ret3 = jq_bt_session_on_frame(&sess2, &st2, BT_T_RESTART, 0, 0, 0, 0, 0, 0);
            CHECK(ret3 == 0, "RESTART 帧接受（CONNECTED 态，rc=0）");
            CHECK(st2.stage == STG_PLAY, "RESTART 语义：stage 回 STG_PLAY");
            CHECK(st2.winner == JQ_WIN_NONE, "RESTART 语义：winner 复位未定");
            CHECK(st2.turn == SIDE_BLUE, "RESTART 语义：turn 复位蓝先手");
            int empty = 0;
            for (int i = 0; i < N_CELLS; i++) if (st2.cells[i].rank) empty = 1;
            CHECK(empty == 0, "RESTART 语义：棋盘清空（jq_state_init）");
            CHECK(sess2.state == BT_ST_OUR_TURN, "RESTART 语义：state=OUR_TURN（等本方输入）");
            CHECK(st2.total_moves == 0 && st2.moves_since_eat == 0,
                  "RESTART 语义：计数复位 0");
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
    printf("[§7] 未知帧类型拒收\n");
    {
        jq_state st; fixture_play(&st);
        jq_bt_session sess;
        jq_bt_session_init(&sess, &st, 1);
        sess.state = BT_ST_THEIR_TURN;
        int ret = jq_bt_session_on_frame(&sess, &st, BT_T_MOVE + 100, 0, 0, 0, 0, 0, 0);
        CHECK(ret == -1, "未知 type（越界）拒收（rc=-1）");
        /* ★ AUD-1000 B04-040（P2 补用例）：type=8/9（DRAW/CHAT）帧协议已定义但
           bt_session.c switch 未处理（default 拒收）——验证对端误发 DRAW/CHAT 帧
           被拒（rc=-1），不静默消费。诚实边界：DRAW/CHAT 业务逻辑未移植（需
           服务器协议），此处只验"帧协议完整拒收"非"业务处理"。 */
        CHECK(jq_bt_session_on_frame(&sess, &st, 8, 0, 0, 0, 0, 0, 0) == -1,
              "未知 type=8（DRAW，协议已定义未处理）拒收（rc=-1）");
        CHECK(jq_bt_session_on_frame(&sess, &st, 9, 0, 0, 0, 0, 0, 0) == -1,
              "未知 type=9（CHAT，协议已定义未处理）拒收（rc=-1）");
        /* ★ AUD-1000 B04-014（P1 已修）：last_type 仅合法 case 命中才记录——
           旧断言"越界 type 也写 last_type"随修复反转：init 后 memset 清零，
           越界帧不污染 last_type（诊断日志按 last_type 判"最后有效帧"不被
           非法帧误导）。初值 0 = BT_T_MOVE（memset 0）保持。 */
        CHECK(sess.last_type == 0, "越界 type 不污染 last_type（保持 init 初值 0）");
    }

    printf(rc ? "== bt_semtest 结果: 有失败（%d 条通过）==\n"
              : "=== bt_semtest 全量 %d 条 PASS ===\n", n_pass);
    return rc;
}
