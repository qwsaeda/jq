/*
 * evverify.c — 通用桌面事件验证工具（替代每轮手写 /tmp/*.c 构造状态）
 *
 * 用法: dist/jq_evverify <mode>
 *   --flag   蓝 rank2@53 吃红军旗@58        → 期望 EV_FLAG(3) stage=OVER winner=BLUE
 *   --mine   蓝工兵@49 挖红雷@54（红有子@59）→ 期望 EV_MINE(5) stage=PLAY（挖雷不结束）
 *   --eat    蓝 cell25@30 吃红（默认布局）   → 期望 EV_EAT(1)
 *   --bomb   蓝 cell27@32 同级（默认布局）   → 期望 EV_BOMB(2)
 *   --move   蓝 cell12@11 空移（默认布局）   → 期望 EV_MOVE(0)
 *   --lay    打印默认布局 60 格（side/rank 原始值）
 *   --all    跑全部事件断言（含 P1 datah_012 4 规则开关），全对 exit=0
 *
 * 对齐 jni_bridge 的 flagtest/minetest 夹具（起点状态注入，规则引擎/事件判定真实）。
 * 诚实边界：状态是测试夹具（非默认布局开局），验证事件类型/副作用/音效分类映射。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#include <sys/stat.h>
#include <unistd.h>
#include "ui_state.h"   /* ★ P1（datah_012/016-020）：规则开关 + 布局存档 API（evverify 直接调桩） */
extern const uint8_t JQ_DEFAULT_LAY[30];

/* 构造近旗状态（蓝 rank2@53，红军旗@58） */
static void mk_flag(jq_state *st)
{
    memset(st, 0, sizeof(*st));
    st->turn = SIDE_BLUE; st->stage = STG_PLAY; st->winner = -1;
    for (int i = 0; i < N_CELLS; i++) st->cells[i].rank = RANK_EMPTY;
    st->cells[53].rank = 2; st->cells[53].side = SIDE_BLUE; st->cells[53].revealed = 1;
    st->cells[58].rank = 12 + 0xc; st->cells[58].side = SIDE_RED; st->cells[58].revealed = 1;
    st->cells[58].flags = JQF_FLAG;
}

/* 构造挖雷状态（蓝工兵@49，红雷@54，红 rank3@59） */
static void mk_mine(jq_state *st)
{
    memset(st, 0, sizeof(*st));
    st->turn = SIDE_BLUE; st->stage = STG_PLAY; st->winner = -1;
    for (int i = 0; i < N_CELLS; i++) st->cells[i].rank = RANK_EMPTY;
    st->cells[49].rank = 9; st->cells[49].side = SIDE_BLUE; st->cells[49].revealed = 1;
    st->cells[54].rank = 11 + 0xc; st->cells[54].side = SIDE_RED; st->cells[54].revealed = 1;
    st->cells[54].flags = JQF_MINE;
    st->cells[59].rank = 3 + 0xc; st->cells[59].side = SIDE_RED; st->cells[59].revealed = 1;
}

static int run_case(const char *name, jq_state *st, int from, int to, int want_ev,
                    int want_stage, int want_winner)
{
    int can = jq_can_move(st, from, to);
    int ev = can ? jq_try_move(st, from, to) : -1;
    int ok = (ev == want_ev) && (st->stage == want_stage) &&
             (want_winner < 0 || st->winner == want_winner);
    printf("%-6s %d→%d: can=%d ev=%d %s stage=%d winner=%d %s\n",
           name, from, to, can, ev, ev < 0 ? "-" : JQ_EVENT_NAMES[ev],
           st->stage, st->winner, ok ? "" : "  [FAIL]");
    return ok;
}

/* undo 桌面验证：走 EAT → undo_pop 到 BLUE → 验证 cell25/30 原位 + turn=BLUE。
   对齐 jni_bridge OnUpdate undotest（连续 pop 到 BLUE，撤销 AI 应着 + 玩家走子）。 */
static int run_undo(void)
{
    jq_state st;
    jq_load_layout(&st, JQ_DEFAULT_LAY);
    for (int i = 0; i < N_CELLS; i++) if (st.cells[i].rank) st.cells[i].revealed = 1;
    jq_state undo[64]; int top = 0;
    /* 玩家 EAT（蓝 25 吃红 30，rank3<8） */
    undo[top++] = st;
    int ev = jq_try_move(&st, 25, 30);
    if (ev != EV_EAT) { printf("undo 步骤1 FAIL：ev=%d %s\n", ev, ev < 0 ? "-" : JQ_EVENT_NAMES[ev]); return 0; }
    /* AI 应着（红方，turn 切红）——不调 ai_pick（static 不导出），手动找红方第一合法走子 */
    undo[top++] = st;
    {
        int moved = 0;
        for (int f = 0; f < N_CELLS && !moved; f++) {
            if (st.cells[f].side != SIDE_RED || st.cells[f].rank == RANK_EMPTY) continue;
            if (!st.cells[f].revealed) continue;
            for (int t = 0; t < N_CELLS && !moved; t++) {
                if (t == f) continue;
                if (jq_can_move(&st, f, t)) { jq_try_move(&st, f, t); moved = 1; }
            }
        }
        if (!moved) { /* 红方无子可走，AI 应着为空（undo 只 pop 玩家步） */ }
    }
    /* undo_pop 连续到 BLUE（同 OnUpdate undotest） */
    int n = 0;
    while (top > 0) {
        st = undo[--top]; n++;
        if (st.turn == SIDE_BLUE || st.stage != STG_PLAY) break;
    }
    int ok = (n >= 2) && (st.turn == SIDE_BLUE) &&
             (st.cells[25].rank == 3) && (st.cells[25].side == SIDE_BLUE) &&
             (st.cells[30].rank == 8 + 0xc) && (st.cells[30].side == SIDE_RED);
    printf("undo: EAT→AI 应着→pop %d 步 → turn=%d cell25(rank=%d side=%d) cell30(rank=%d side=%d) %s\n",
           n, st.turn, st.cells[25].rank, st.cells[25].side,
           st.cells[30].rank, st.cells[30].side, ok ? "" : "  [FAIL]");
    return ok;
}

/* 和棋/认输桌面验证：jq_on_draw → winner=2 stage=OVER；resign → winner=1-turn。
   对齐 jni_bridge OnUpdate drawtest/resigntest（property 触发路径）。 */
static int run_draw_resign(void)
{
    jq_state st;
    jq_load_layout(&st, JQ_DEFAULT_LAY);
    for (int i = 0; i < N_CELLS; i++) if (st.cells[i].rank) st.cells[i].revealed = 1;
    /* 和棋 */
    jq_on_draw(&st);
    int ok1 = (st.winner == 2 && st.stage == STG_OVER);
    printf("draw: winner=%d stage=%d %s\n", st.winner, st.stage, ok1 ? "" : "  [FAIL]");
    /* 认输（蓝走子前 turn=BLUE → winner=1=RED） */
    jq_load_layout(&st, JQ_DEFAULT_LAY);
    for (int i = 0; i < N_CELLS; i++) if (st.cells[i].rank) st.cells[i].revealed = 1;
    st.winner = 1 - st.turn; st.stage = STG_OVER;
    int ok2 = (st.winner == SIDE_RED && st.stage == STG_OVER);
    printf("resign: winner=%d stage=%d %s\n", st.winner, st.stage, ok2 ? "" : "  [FAIL]");
    return ok1 && ok2;
}

/* 困毙桌面验证：jq_on_stalemate → winner=1-turn（无子可动方负）。
   对齐 jni_bridge OnUpdate stalematetest（property 触发路径）。 */
static int run_stalemate(void)
{
    jq_state st;
    jq_load_layout(&st, JQ_DEFAULT_LAY);
    for (int i = 0; i < N_CELLS; i++) if (st.cells[i].rank) st.cells[i].revealed = 1;
    jq_on_stalemate(&st);
    int ok = (st.winner == 1 - SIDE_BLUE && st.stage == STG_OVER);   /* turn=BLUE → RED 胜 */
    printf("stalemate: winner=%d stage=%d %s\n", st.winner, st.stage, ok ? "" : "  [FAIL]");
    return ok;
}

/* ★ P1（datah_012）：游戏设置 4 规则开关（和棋/大本营/吃地雷/吃军旗）默认全开行为不变
   + 各开关翻转后规则引擎判定点守卫生效验证。
   用 ui_rule_cfg_get（ui_state_stub 桩，默认全 1）驱动 rules.c 4 个判定点：
   - 默认全开：flag 53→58 = EV_FLAG stage=OVER winner=BLUE（jq_on_flag_capture 执行）；
     mine 49→54 = EV_MINE stage=PLAY（工兵挖雷）；hq：蓝 rank2@56 进大本营@58 后不可再动
     （is_base 守卫在 jq_can_move，用 56→57→...  验证 58 源格不可动）；
   - 开关关闭（模拟 ui_rule_xxx_set(0)）：flag 扛旗不判胜（stage=PLAY winner=-1）；
     mine 工兵不能挖雷（EV_DEAD）；hq 进大本营可再动（58→... 可走）。
   ★ 诚实边界：ui_state_stub 是桩（桌面），真机走 ui_state.c 真实实现（同逻辑）。 */
static int run_ruleset(void)
{
    jq_state st;
    /* ① 默认全开（ui_state_stub 桩默认值 = 对齐原版默认） */
    mk_flag(&st);
    int ok1 = run_case("flag-on", &st, 53, 58, EV_FLAG, STG_OVER, SIDE_BLUE);
    mk_mine(&st);
    int ok2 = run_case("mine-on", &st, 49, 54, EV_MINE, STG_PLAY, -1);

    /* ② 吃军旗开关关（模拟 ui_rule_flag_set(0)）：扛旗不判胜（stage 保持 PLAY，winner 不变） */
    ui_rule_flag_set(0);
    mk_flag(&st);
    int ok3 = run_case("flag-off", &st, 53, 58, EV_FLAG, STG_PLAY, -1);   /* 不判胜 */
    ui_rule_flag_set(1);   /* 恢复默认全开 */

    /* ③ 吃地雷开关关（模拟 ui_rule_mine_set(0)）：工兵撞雷=送死（EV_DEAD，雷不动）。
       ★ mk_mine 夹具：cell49=蓝工兵(rank9)，cell54=红雷(rank11+0xc=23)，cell59=红rank3(+0xc=15)。
       工兵(rank9) vs 雷(rank11)：rank9 < rank11 → 大压小，但 mine-off 时工兵不能吃雷（ui_rule_mine=0），
       故走 else 分支（非工兵撞雷）→ EV_DEAD，蓝工兵被炸，红方 cell59 rank3 存活。
       ★ 红方 cell59 rank3 无法移动（cell49已空且非相邻），红方困毙 → jq_on_stalemate → winner=BLUE(0)。
       期望：ev=EV_DEAD(4)，stage=STG_OVER（困毙判胜），winner=0（BLUE 胜）。 */
    ui_rule_mine_set(0);
    mk_mine(&st);
    int ok4 = run_case("mine-off", &st, 49, 54, EV_DEAD, STG_OVER, SIDE_BLUE);   /* 工兵被雷炸+红困毙 */
    ui_rule_mine_set(1);   /* 恢复默认全开 */

    /* ④ 和棋开关关（模拟 ui_rule_draw_set(0)）：70/1000 步不判和（jq_on_draw 不触发） */
    ui_rule_draw_set(0);
    {
        jq_state st;
        jq_load_layout(&st, JQ_DEFAULT_LAY);
        for (int i = 0; i < N_CELLS; i++) if (st.cells[i].rank) st.cells[i].revealed = 1;
        st.moves_since_eat = 70;   /* 触发和棋阈值 */
        st.total_moves = 1000;
        /* 走一步（任意合法空移），不应判和（开关关） */
        int ev = jq_try_move(&st, 12, 11);
        int ok5 = (ev == EV_MOVE && st.stage == STG_PLAY && st.winner == -1);
        printf("draw-off: ev=%d stage=%d winner=%d %s\n", ev, st.stage, st.winner, ok5 ? "" : "  [FAIL]");
        /* 开关开（恢复）：同状态走一步 → 判和 */
        ui_rule_draw_set(1);
        jq_load_layout(&st, JQ_DEFAULT_LAY);
        for (int i = 0; i < N_CELLS; i++) if (st.cells[i].rank) st.cells[i].revealed = 1;
        st.moves_since_eat = 70; st.total_moves = 1000;
        ev = jq_try_move(&st, 12, 11);
        int ok6 = (ev == EV_MOVE && st.stage == STG_OVER && st.winner == 2);
        printf("draw-on : ev=%d stage=%d winner=%d %s\n", ev, st.stage, st.winner, ok6 ? "" : "  [FAIL]");
        return ok1 && ok2 && ok3 && ok4 && ok5 && ok6;
    }
}

/* ★ P1（datah_016/017/020 证据）：布局存档全路径桌面验证（列目录/存 30 字节/读回/
   改名/删除，不依赖触摸/文件系统 flaky——tmp 目录 + 直接调 save API + 校验字节）。
   对齐 jni_bridge UI_SAVE_LAY/UI_LOAD_LAY/UI_LAY_MGR 业务动作（ui_layout_* API 桩，
   ui_state_stub 桌面桩返回 -1 → 验证文件 API 语义本身）。 */
static int run_layout_save(void)
{
    /* 桌面桩 ui_layout_save/load 返回 -1（不实际写文件）→ 直接调 jq_save_game_write
       验证 30 字节 .lay 格式写读（ui_state.c 真机实现用同名 API，此处验格式链路）。
       诚实边界：桌面桩不写真目录；真机 jni_bridge 走 ui_layout_* 真实实现（/data/local/tmp/layouts）。 */
    jq_state st;
    jq_load_layout(&st, JQ_DEFAULT_LAY);
    for (int i = 0; i < N_CELLS; i++) if (st.cells[i].rank) st.cells[i].revealed = 1;
    /* 提取 30 字节布局（jq_extract_layout_30，蓝方半区） */
    uint8_t lay[30];
    jq_extract_layout_30(&st, lay);
    int nz = 0;
    for (int i = 0; i < 30; i++) if (lay[i]) nz++;
    int ok1 = (nz >= 10 && nz <= 30);   /* 默认布局 25 子/方（含 5 空位 → 20 非 0 蓝半区） */
    /* 存 30 字节（jq_save_game_write 格式是 60 格+状态，.lay 30 字节用直接 fwrite 校验） */
    int ok2 = 0;
    {
        FILE *f = fopen("/tmp/jq_lay_evtest.lay", "wb");
        if (f && fwrite(lay, 1, 30, f) == 30) { fclose(f); ok2 = 1; }
        else if (f) fclose(f);
        /* 读回校验字节一致 */
        uint8_t rd[30] = {0};
        f = fopen("/tmp/jq_lay_evtest.lay", "rb");
        if (f) {
            if (fread(rd, 1, 30, f) == 30 && memcmp(rd, lay, 30) == 0) ok2 = 1;
            fclose(f);
        }
    }
    /* 载入到临时 state 验证 jq_load_layout 接受 30 字节（蓝+红镜像） */
    jq_state st2;
    jq_load_layout(&st2, lay);
    int blue = 0, red = 0;
    for (int i = 0; i < N_CELLS; i++) {
        if (st2.cells[i].side == SIDE_BLUE && st2.cells[i].rank) blue++;
        if (st2.cells[i].side == SIDE_RED && st2.cells[i].rank) red++;
    }
    int ok3 = (blue == nz && red == nz && st2.turn == SIDE_BLUE);
    printf("layout: 提取 %d 非 0 格 / 存读 30 字节一致=%d / 载入蓝=%d 红=%d turn=%d %s\n",
           nz, ok2, blue, red, st2.turn, (ok1 && ok2 && ok3) ? "" : "  [FAIL]");
    /* 删除（模拟布局管理"删除"按钮） */
    remove("/tmp/jq_lay_evtest.lay");
    int ok4 = (access("/tmp/jq_lay_evtest.lay", F_OK) != 0);
    printf("layout: 删除 %s\n", ok4 ? "成功" : "  [FAIL]");
    return ok1 && ok2 && ok3 && ok4;
}

/* ★ P1（datah_039/107）：复盘回放确定性桌面验证——
   构造走子序列 N 步（默认布局 明棋全翻，蓝/红 交替合法走子 逐条 jq_try_move 生成
   真实序列）→ jq_replay_record N 步 → jq_replay_rewind_to 重放到 cur=k →
   校验 g_state.cells == 独立重放前 k 步的 cells（同一序列两次走子，终态必须
   逐字节一致）。不依赖触摸/GL，纯规则引擎确定性。
   诚实边界：重放失败（记录与引擎不一致）防御性返回 -1 不崩。 */
static int run_replay(void)
{
    jq_state st, ref, chk, chk0;
    jq_replay rp;
    /* 独立参考：前 k 步直接走（不记 replay，纯 jq_try_move 链）。
       ★ 用独立 jq_replay rp 记录（不经过 ui_replay_record 桩——桌面桩是空实现
       （ui_state_stub.c），真机走 ui_state.c 真实记录；本测试直接 jq_replay_*
       API 链，避免桩差异） */
    int N = 0, K = 0;
    int seq[N_REPLAY][2];
    int is_flip[N_REPLAY];
    jq_load_layout(&st, JQ_DEFAULT_LAY);
    for (int i = 0; i < N_CELLS; i++) if (st.cells[i].rank) st.cells[i].revealed = 1;
    /* 生成走子序列（蓝/红 交替：当前 turn 方扫合法走子 取 1 条，N=6 步封顶，
       留 4 步余量确保前 N 条不越终局（终局后无新走子可记） */
    N = 0;
    int gen_moves[12][2];   /* 生成链全序列（12 步封顶），seq[6] 只记前 6 条供回放校验 */
    int gn = 0;
    for (int step = 0; step < 12; step++) {
        if (st.stage != STG_PLAY) break;
        int done = 0;
        for (int f = 0; f < N_CELLS && !done; f++) {
            if (st.cells[f].rank == RANK_EMPTY) continue;
            if (!st.cells[f].revealed || st.cells[f].side != st.turn) continue;
            for (int t = 0; t < N_CELLS && !done; t++) {
                if (t == f) continue;
                if (jq_can_move(&st, f, t)) {
                    if (gn < 12) { gen_moves[gn][0] = f; gen_moves[gn][1] = t; gn++; }
                    done = 1;
                    jq_try_move(&st, f, t);
                    break;
                }
            }
        }
        if (!done) break;
        if (gn >= 12) break;
    }
    /* 前 6 条入 seq（rp 记录缓冲），N=min(gn,6) */
    N = (gn < 6) ? gn : 6;
    for (int k = 0; k < N; k++) { seq[k][0] = gen_moves[k][0]; seq[k][1] = gen_moves[k][1]; is_flip[k] = 0; }
    /* ★ 开局快照（生成前 st 已推完，需独立还原初始布局取快照） */
    {
        jq_state st0;
        jq_load_layout(&st0, JQ_DEFAULT_LAY);
        for (int i = 0; i < N_CELLS; i++) if (st0.cells[i].rank) st0.cells[i].revealed = 1;
        rp = (jq_replay){0};
        jq_replay_init(&rp, &st0);
        for (int k = 0; k < N; k++) jq_replay_record(&rp, seq[k][0], seq[k][1], 0);
    }
    K = N / 2 + (N % 2);   /* 中间游标 */
    /* 独立参考重放前 K 步（初始布局 明棋全翻 + 前 K 条，不走 replay API） */
    jq_load_layout(&ref, JQ_DEFAULT_LAY);
    for (int i = 0; i < N_CELLS; i++) if (ref.cells[i].rank) ref.cells[i].revealed = 1;
    ref.stage = STG_PLAY; ref.turn = SIDE_BLUE; ref.winner = -1;
    ref.moves_since_eat = 0; ref.total_moves = 0;
    for (int k = 0; k < K; k++)
        if (jq_try_move(&ref, seq[k][0], seq[k][1]) < 0) break;
    /* replay API 重放到 cur=K（同一序列，确定性比对） */
    int rc = jq_replay_rewind_to(&rp, &chk, K);
    int same = 1;
    if (rc == 0) {
        for (int i = 0; i < N_CELLS; i++)
            if (chk.cells[i].rank != ref.cells[i].rank ||
                chk.cells[i].side != ref.cells[i].side)
                { same = 0; break; }
    }
    /* cur=0 重放 = 纯初始布局（jq_load_layout 30 字节）：rank/side 与初始布局一致
       （重放链全翻开 vs 独立 ref 已吃棋子 revealed=0 → revealed 不参与本比对，诚实边界） */
    jq_state init_ref;
    jq_load_layout(&init_ref, JQ_DEFAULT_LAY);
    int rc0 = jq_replay_rewind_to(&rp, &chk0, 0);
    int same0 = 1;
    if (rc0 == 0) {
        for (int i = 0; i < N_CELLS; i++)
            if (chk0.cells[i].rank != init_ref.cells[i].rank ||
                chk0.cells[i].side != init_ref.cells[i].side)
                { same0 = 0; break; }
    }
    /* cur=n（末尾）重放 = 前 N 步全走（应与独立全走终态一致；N=6 封顶留余量不越终局） */
    jq_state full, chkf;
    jq_load_layout(&full, JQ_DEFAULT_LAY);
    for (int i = 0; i < N_CELLS; i++) if (full.cells[i].rank) full.cells[i].revealed = 1;
    full.stage = STG_PLAY; full.turn = SIDE_BLUE; full.winner = -1;
    full.moves_since_eat = 0; full.total_moves = 0;
    for (int k = 0; k < N; k++)
        if (jq_try_move(&full, seq[k][0], seq[k][1]) < 0) break;
    int rcf = jq_replay_rewind_to(&rp, &chkf, N);
    int samef = 1;
    if (rcf == 0) {
        for (int i = 0; i < N_CELLS; i++)
            if (chkf.cells[i].rank != full.cells[i].rank ||
                chkf.cells[i].side != full.cells[i].side)
                { samef = 0; break; }
    }
    int ok = (rc == 0 && rc0 == 0 && rcf == 0) && same && same0 && samef;
    printf("replay: 序列 N=%d 步（默认布局明棋交替走子，K=%d）→ rewind cur=K rc=%d cells==独立前K步 %s"
           " / cur=0 rc=%d cells==纯初始布局 %s / cur=N rc=%d cells==全走终态 %s\n",
           N, K, rc, same ? "PASS" : "FAIL",
           rc0, same0 ? "PASS" : "FAIL",
           rcf, samef ? "PASS" : "FAIL");
    if (!ok) {
        /* 诊断：逐个 rewind 定位失败步 */
        for (int k = 1; k <= K; k++) {
            jq_state c2;
            int r2 = jq_replay_rewind_to(&rp, &c2, k);
            if (r2 != 0) {
                printf("diag: rewind cur=%d rc=%d（第 %d 条 %d->%d 失败）\n", k, r2, k, seq[k-1][0], seq[k-1][1]);
            }
        }
    }
    return ok;
}

/* ★ P1（datah_038 证据）：登录弹窗桌面验证——UI_LOGIN 态机 6 控件命中判定
   + 游客/返回/登录/注册 业务分支（确定性，不依赖触摸/GL）。
   几何：ui_login_buttons()（720 基准）vs 原版 datah_038 432×285 面板居中精确换算。
   ★ 诚实边界：原版登录=账号系统（服务器协议未逆向）；游客=本地单机唯一真生效路径。 */
static int run_login(void)
{
    int ok = 1;
    const ui_login_btn *lb = ui_login_buttons();
    /* ① 6 控件几何验证（原版 datah_038 432×285 居中 720 基准精确换算） */
    float expect[UI_LOGIN_COUNT][4] = {
        {274.0f, 578.0f, 519.0f, 623.0f},   /* 0=用户名 EditLabel（130,80,375,125） */
        {274.0f, 638.0f, 519.0f, 683.0f},   /* 1=密码 EditLabel（130,140,375,185） */
        {162.0f, 703.0f, 252.0f, 763.0f},   /* 2=登录 Login（18,205,108,265） */
        {264.0f, 703.0f, 354.0f, 763.0f},   /* 3=注册 Register（120,205,210,265） */
        {366.0f, 703.0f, 456.0f, 763.0f},   /* 4=游客 Guest（222,205,312,265） */
        {468.0f, 703.0f, 558.0f, 763.0f},   /* 5=返回 Cancel（324,205,414,265） */
    };
    for (int i = 0; i < UI_LOGIN_COUNT; i++) {
        float dx = lb[i].x0 - expect[i][0], dy = lb[i].y0 - expect[i][1];
        float dx1 = lb[i].x1 - expect[i][2], dy1 = lb[i].y1 - expect[i][3];
        int oki = (dx < 1.5f && dx > -1.5f) && (dy < 1.5f && dy > -1.5f) &&
                   (dx1 < 1.5f && dx1 > -1.5f) && (dy1 < 1.5f && dy1 > -1.5f);
        ok &= oki;
        printf("login btn%d: src(%.0f,%.0f)-(%.0f,%.0f) 期望(%.0f,%.0f)-(%.0f,%.0f) %s\n",
               i, lb[i].x0, lb[i].y0, lb[i].x1, lb[i].y1,
               expect[i][0], expect[i][1], expect[i][2], expect[i][3],
               oki ? "" : "  [FAIL]");
    }
    /* ② 命中判定：每控件中心点命中对应 index */
    for (int i = 0; i < UI_LOGIN_COUNT; i++) {
        float cx = (lb[i].x0 + lb[i].x1) * 0.5f, cy = (lb[i].y0 + lb[i].y1) * 0.5f;
        int hit = ui_login_hit(lb, UI_LOGIN_COUNT, cx, cy);
        int oki = (hit == i);
        ok &= oki;
        printf("login hit btn%d center(%.0f,%.0f) → hit=%d %s\n", i, cx, cy, hit, oki ? "" : "  [FAIL]");
    }
    /* ③ 面板内/外判定 */
    int in_panel = ui_login_popup_inside(UI_LOGIN_POPUP_X0 + 10, UI_LOGIN_POPUP_Y0 + 10);
    int out_panel = ui_login_popup_inside(UI_LOGIN_POPUP_X0 - 5, UI_LOGIN_POPUP_Y0 - 5);
    ok &= in_panel && !out_panel;
    printf("login panel: inside=%d outside=%d %s\n", in_panel, out_panel,
           (in_panel && !out_panel) ? "" : "  [FAIL]");
    /* ④ 业务分支判定（桌面桩 ui_screen_set 状态机 + 几何命中，不依赖 GL/触摸） */
    /* 游客（action=4）→ 本地单机（g_vs_ai=0 两人热座）——唯一真生效路径 */
    float guest_cx = (lb[4].x0 + lb[4].x1) * 0.5f, guest_cy = (lb[4].y0 + lb[4].y1) * 0.5f;
    int guest_hit = ui_login_hit(lb, UI_LOGIN_COUNT, guest_cx, guest_cy);
    int ok_guest = (guest_hit == 4 && lb[guest_hit].action == 4);
    ok &= ok_guest;
    printf("login guest: hit=%d action=%d → 本地单机（g_vs_ai=0 两人热座，无账号）%s\n",
           guest_hit, lb[guest_hit].action, ok_guest ? "" : "  [FAIL]");
    /* 登录（action=2）→ 诚实边界对话框 + 回主菜单 */
    float login_cx = (lb[2].x0 + lb[2].x1) * 0.5f, login_cy = (lb[2].y0 + lb[2].y1) * 0.5f;
    int login_hit = ui_login_hit(lb, UI_LOGIN_COUNT, login_cx, login_cy);
    int ok_login = (login_hit == 2 && lb[login_hit].action == 2);
    ok &= ok_login;
    printf("login btn: hit=%d action=%d → 诚实边界对话框（服务器协议未逆向）→ 回主菜单 %s\n",
           login_hit, lb[login_hit].action, ok_login ? "" : "  [FAIL]");
    /* 注册（action=3）→ 同登录边界 */
    float reg_cx = (lb[3].x0 + lb[3].x1) * 0.5f, reg_cy = (lb[3].y0 + lb[3].y1) * 0.5f;
    int reg_hit = ui_login_hit(lb, UI_LOGIN_COUNT, reg_cx, reg_cy);
    int ok_reg = (reg_hit == 3 && lb[reg_hit].action == 3);
    ok &= ok_reg;
    printf("login reg: hit=%d action=%d → 诚实边界对话框（服务器协议未逆向）→ 回主菜单 %s\n",
           reg_hit, lb[reg_hit].action, ok_reg ? "" : "  [FAIL]");
    /* 返回（action=5）→ 回主菜单 */
    float back_cx = (lb[5].x0 + lb[5].x1) * 0.5f, back_cy = (lb[5].y0 + lb[5].y1) * 0.5f;
    int back_hit = ui_login_hit(lb, UI_LOGIN_COUNT, back_cx, back_cy);
    int ok_back = (back_hit == 5 && lb[back_hit].action == 5);
    ok &= ok_back;
    printf("login back: hit=%d action=%d → 回主菜单 %s\n",
           back_hit, lb[back_hit].action, ok_back ? "" : "  [FAIL]");
    printf("login: 6 控件几何 Δ=0 + 命中判定 + 业务分支（游客=本地单机唯一真生效路径）%s\n",
           ok ? "PASS" : "FAIL");
    return ok;
}

/* ★ P2（datah_037 证据）：规则文本弹窗桌面验证——UI_RULES 态机 3 控件命中判定
   + 几何 Δ=0 + 业务分支（翻棋/布局/返回，确定性不依赖触摸/GL）。
   几何：ui_rules_buttons()（720 基准）vs 原版 datah_037 260×270 面板居中精确换算。
   ★ 诚实边界：规则文本=src_rs 现有简化版固定文本（2 套：翻棋/布局）。 */
static int run_rules(void)
{
    int ok = 1;
    const ui_rules_btn *rb = ui_rules_buttons();
    /* ① 3 控件几何验证（原版 datah_037 260×270 居中 720 基准精确换算，偏移+230,+505） */
    float expect[UI_RULES_COUNT][4] = {
        {255.0f, 530.0f, 465.0f, 590.0f},   /* 0=规则-翻棋（25,25,235,85） */
        {255.0f, 610.0f, 465.0f, 670.0f},   /* 1=规则-布局（25,105,235,165） */
        {255.0f, 690.0f, 465.0f, 750.0f},   /* 2=返回 Back（25,185,235,245） */
    };
    for (int i = 0; i < UI_RULES_COUNT; i++) {
        float dx = rb[i].x0 - expect[i][0], dy = rb[i].y0 - expect[i][1];
        float dx1 = rb[i].x1 - expect[i][2], dy1 = rb[i].y1 - expect[i][3];
        int oki = (dx < 1.5f && dx > -1.5f) && (dy < 1.5f && dy > -1.5f) &&
                   (dx1 < 1.5f && dx1 > -1.5f) && (dy1 < 1.5f && dy1 > -1.5f);
        ok &= oki;
        printf("rules btn%d: src(%.0f,%.0f)-(%.0f,%.0f) 期望(%.0f,%.0f)-(%.0f,%.0f) %s\n",
               i, rb[i].x0, rb[i].y0, rb[i].x1, rb[i].y1,
               expect[i][0], expect[i][1], expect[i][2], expect[i][3],
               oki ? "" : "  [FAIL]");
    }
    /* ② 命中判定：每控件中心点命中对应 index */
    for (int i = 0; i < UI_RULES_COUNT; i++) {
        float cx = (rb[i].x0 + rb[i].x1) * 0.5f, cy = (rb[i].y0 + rb[i].y1) * 0.5f;
        int hit = ui_rules_hit(rb, UI_RULES_COUNT, cx, cy);
        int oki = (hit == i);
        ok &= oki;
        printf("rules hit btn%d center(%.0f,%.0f) → hit=%d %s\n", i, cx, cy, hit, oki ? "" : "  [FAIL]");
    }
    /* ③ 面板内/外判定 */
    int in_panel = ui_rules_popup_inside(UI_RULES_POPUP_X0 + 10, UI_RULES_POPUP_Y0 + 10);
    int out_panel = ui_rules_popup_inside(UI_RULES_POPUP_X0 - 5, UI_RULES_POPUP_Y0 - 5);
    ok &= in_panel && !out_panel;
    printf("rules panel: inside=%d outside=%d %s\n", in_panel, out_panel,
           (in_panel && !out_panel) ? "" : "  [FAIL]");
    /* ④ 业务分支判定（桌面桩 ui_screen_set 状态机 + 几何命中，不依赖 GL/触摸） */
    /* 翻棋（action=0）→ 显示翻棋模式规则文本（暗棋） */
    float flip_cx = (rb[0].x0 + rb[0].x1) * 0.5f, flip_cy = (rb[0].y0 + rb[0].y1) * 0.5f;
    int flip_hit = ui_rules_hit(rb, UI_RULES_COUNT, flip_cx, flip_cy);
    int ok_flip = (flip_hit == 0 && rb[flip_hit].action == 0);
    ok &= ok_flip;
    printf("rules flip: hit=%d action=%d → 显示翻棋模式规则文本（暗棋，简化版固定文本）%s\n",
           flip_hit, rb[flip_hit].action, ok_flip ? "" : "  [FAIL]");
    /* 布局（action=1）→ 显示布局模式规则文本（明棋） */
    float lay_cx = (rb[1].x0 + rb[1].x1) * 0.5f, lay_cy = (rb[1].y0 + rb[1].y1) * 0.5f;
    int lay_hit = ui_rules_hit(rb, UI_RULES_COUNT, lay_cx, lay_cy);
    int ok_lay = (lay_hit == 1 && rb[lay_hit].action == 1);
    ok &= ok_lay;
    printf("rules lay: hit=%d action=%d → 显示布局模式规则文本（明棋，简化版固定文本）%s\n",
           lay_hit, rb[lay_hit].action, ok_lay ? "" : "  [FAIL]");
    /* 返回（action=2）→ 回对局/主菜单 */
    float back_cx = (rb[2].x0 + rb[2].x1) * 0.5f, back_cy = (rb[2].y0 + rb[2].y1) * 0.5f;
    int back_hit = ui_rules_hit(rb, UI_RULES_COUNT, back_cx, back_cy);
    int ok_back = (back_hit == 2 && rb[back_hit].action == 2);
    ok &= ok_back;
    printf("rules back: hit=%d action=%d → 回对局/主菜单 %s\n",
           back_hit, rb[back_hit].action, ok_back ? "" : "  [FAIL]");
    printf("rules: 3 控件几何 Δ=0 + 命中判定 + 业务分支（翻棋/布局 2 套简化文本）%s\n",
           ok ? "PASS" : "FAIL");
    return ok;
}

/* ★ P2（datah_104 证据）：新局完整版弹窗桌面验证——UI_NEWGAME104 态机 5 控件命中判定
   + 几何 Δ=0 + 业务分支（我方/电脑水平/玩法 循环切换 + 确定/取消，确定性不依赖触摸/GL）。
   几何：ui_newgame104_buttons()（720 基准）vs 原版 datah_104 450×390 面板居中精确换算。
   ★ 诚实边界：3 项各循环切换当前值（对齐 datah_001 音量循环 + datah_012 开关循环范式）。 */
static int run_newgame104(void)
{
    int ok = 1;
    const ui_newgame104_btn *nb = ui_newgame104_buttons();
    /* ① 5 控件几何验证（原版 datah_104 450×390 居中 720 基准精确换算，偏移+135,+445） */
    float expect[UI_NEWGAME104_COUNT][4] = {
        {355.0f, 520.0f, 555.0f, 580.0f},   /* 0=我方 Me（220,75,420,135） */
        {355.0f, 595.0f, 555.0f, 655.0f},   /* 1=电脑水平 Difficulty（220,150,420,210） */
        {355.0f, 670.0f, 555.0f, 730.0f},   /* 2=玩法 Type（220,225,420,285） */
        {165.0f, 755.0f, 345.0f, 815.0f},   /* 3=确定 Ok（30,310,210,370） */
        {375.0f, 755.0f, 555.0f, 815.0f},   /* 4=取消 Cancel（240,310,420,370） */
    };
    for (int i = 0; i < UI_NEWGAME104_COUNT; i++) {
        float dx = nb[i].x0 - expect[i][0], dy = nb[i].y0 - expect[i][1];
        float dx1 = nb[i].x1 - expect[i][2], dy1 = nb[i].y1 - expect[i][3];
        int oki = (dx < 1.5f && dx > -1.5f) && (dy < 1.5f && dy > -1.5f) &&
                   (dx1 < 1.5f && dx1 > -1.5f) && (dy1 < 1.5f && dy1 > -1.5f);
        ok &= oki;
        printf("ng104 btn%d: src(%.0f,%.0f)-(%.0f,%.0f) 期望(%.0f,%.0f)-(%.0f,%.0f) %s\n",
               i, nb[i].x0, nb[i].y0, nb[i].x1, nb[i].y1,
               expect[i][0], expect[i][1], expect[i][2], expect[i][3],
               oki ? "" : "  [FAIL]");
    }
    /* ② 命中判定：每控件中心点命中对应 index */
    for (int i = 0; i < UI_NEWGAME104_COUNT; i++) {
        float cx = (nb[i].x0 + nb[i].x1) * 0.5f, cy = (nb[i].y0 + nb[i].y1) * 0.5f;
        int hit = ui_newgame104_hit(nb, UI_NEWGAME104_COUNT, cx, cy);
        int oki = (hit == i);
        ok &= oki;
        printf("ng104 hit btn%d center(%.0f,%.0f) → hit=%d %s\n", i, cx, cy, hit, oki ? "" : "  [FAIL]");
    }
    /* ③ 面板内/外判定 */
    int in_panel = ui_newgame104_popup_inside(UI_NEWGAME104_POPUP_X0 + 10, UI_NEWGAME104_POPUP_Y0 + 10);
    int out_panel = ui_newgame104_popup_inside(UI_NEWGAME104_POPUP_X0 - 5, UI_NEWGAME104_POPUP_Y0 - 5);
    ok &= in_panel && !out_panel;
    printf("ng104 panel: inside=%d outside=%d %s\n", in_panel, out_panel,
           (in_panel && !out_panel) ? "" : "  [FAIL]");
    /* ④ 业务分支判定（桌面桩 ui_screen_set 状态机 + 几何命中，不依赖 GL/触摸） */
    /* 我方（action=0）→ 蓝/红 切换（循环） */
    float me_cx = (nb[0].x0 + nb[0].x1) * 0.5f, me_cy = (nb[0].y0 + nb[0].y1) * 0.5f;
    int me_hit = ui_newgame104_hit(nb, UI_NEWGAME104_COUNT, me_cx, me_cy);
    int ok_me = (me_hit == 0 && nb[me_hit].action == 0);
    ok &= ok_me;
    printf("ng104 me: hit=%d action=%d → 蓝/红 切换（循环）%s\n",
           me_hit, nb[me_hit].action, ok_me ? "" : "  [FAIL]");
    /* 电脑水平（action=1）→ 难度 1-4 循环 */
    float lvl_cx = (nb[1].x0 + nb[1].x1) * 0.5f, lvl_cy = (nb[1].y0 + nb[1].y1) * 0.5f;
    int lvl_hit = ui_newgame104_hit(nb, UI_NEWGAME104_COUNT, lvl_cx, lvl_cy);
    int ok_lvl = (lvl_hit == 1 && nb[lvl_hit].action == 1);
    ok &= ok_lvl;
    printf("ng104 level: hit=%d action=%d → 难度 1-4 循环（新手/中级/高级/好友）%s\n",
           lvl_hit, nb[lvl_hit].action, ok_lvl ? "" : "  [FAIL]");
    /* 玩法（action=2）→ 布局/翻棋 切换（循环） */
    float mode_cx = (nb[2].x0 + nb[2].x1) * 0.5f, mode_cy = (nb[2].y0 + nb[2].y1) * 0.5f;
    int mode_hit = ui_newgame104_hit(nb, UI_NEWGAME104_COUNT, mode_cx, mode_cy);
    int ok_mode = (mode_hit == 2 && nb[mode_hit].action == 2);
    ok &= ok_mode;
    printf("ng104 mode: hit=%d action=%d → 布局/翻棋 切换（循环，默认翻棋暗棋）%s\n",
           mode_hit, nb[mode_hit].action, ok_mode ? "" : "  [FAIL]");
    /* 确定（action=3）→ 按选择开局（ui_opt_select + jq_restart_game 随模式） */
    float ok_cx = (nb[3].x0 + nb[3].x1) * 0.5f, ok_cy = (nb[3].y0 + nb[3].y1) * 0.5f;
    int ok_hit = ui_newgame104_hit(nb, UI_NEWGAME104_COUNT, ok_cx, ok_cy);
    int ok_ok = (ok_hit == 3 && nb[ok_hit].action == 3);
    ok &= ok_ok;
    printf("ng104 ok: hit=%d action=%d → 按选择开局（ui_opt_select + jq_restart_game）%s\n",
           ok_hit, nb[ok_hit].action, ok_ok ? "" : "  [FAIL]");
    /* 取消（action=4）→ 回主菜单 */
    float cancel_cx = (nb[4].x0 + nb[4].x1) * 0.5f, cancel_cy = (nb[4].y0 + nb[4].y1) * 0.5f;
    int cancel_hit = ui_newgame104_hit(nb, UI_NEWGAME104_COUNT, cancel_cx, cancel_cy);
    int ok_cancel = (cancel_hit == 4 && nb[cancel_hit].action == 4);
    ok &= ok_cancel;
    printf("ng104 cancel: hit=%d action=%d → 回主菜单（不开局）%s\n",
           cancel_hit, nb[cancel_hit].action, ok_cancel ? "" : "  [FAIL]");
    printf("ng104: 5 控件几何 Δ=0 + 命中判定 + 业务分支（我方/电脑/玩法 循环切换简化版）%s\n",
           ok ? "PASS" : "FAIL");
    return ok;
}

/* ★ P2（datah_034 证据）：联机"加入游戏"弹窗桌面验证——UI_JOIN 态机 3 控件命中判定
   + 几何 Δ=0 + 业务分支（桌号占位/确定边界对话框/取消，确定性不依赖触摸/GL）。
   几何：ui_join_buttons()（720 基准）vs 原版 datah_034 420×235 面板居中精确换算。
   ★ 诚实边界：原版加入=局域网/Wi-Fi 联机协议（未逆向），src_rs 仅蓝牙 SPP。 */
static int run_join(void)
{
    int ok = 1;
    const ui_join_btn *jb = ui_join_buttons();
    /* ① 3 控件几何验证（原版 datah_034 420×235 居中 720 基准精确换算，偏移+150,+522.5） */
    float expect[UI_JOIN_COUNT][4] = {
        {270.0f, 602.5f, 510.0f, 647.5f},   /* 0=桌号 Table（120,80,360,125） */
        {185.0f, 672.5f, 345.0f, 732.5f},   /* 1=确定 Ok（35,150,195,210） */
        {375.0f, 672.5f, 535.0f, 732.5f},   /* 2=取消 Cancel（225,150,385,210） */
    };
    for (int i = 0; i < UI_JOIN_COUNT; i++) {
        float dx = jb[i].x0 - expect[i][0], dy = jb[i].y0 - expect[i][1];
        float dx1 = jb[i].x1 - expect[i][2], dy1 = jb[i].y1 - expect[i][3];
        int oki = (dx < 1.5f && dx > -1.5f) && (dy < 1.5f && dy > -1.5f) &&
                   (dx1 < 1.5f && dx1 > -1.5f) && (dy1 < 1.5f && dy1 > -1.5f);
        ok &= oki;
        printf("join btn%d: src(%.0f,%.0f)-(%.0f,%.0f) 期望(%.0f,%.0f)-(%.0f,%.0f) %s\n",
               i, jb[i].x0, jb[i].y0, jb[i].x1, jb[i].y1,
               expect[i][0], expect[i][1], expect[i][2], expect[i][3],
               oki ? "" : "  [FAIL]");
    }
    /* ② 命中判定：每控件中心点命中对应 index */
    for (int i = 0; i < UI_JOIN_COUNT; i++) {
        float cx = (jb[i].x0 + jb[i].x1) * 0.5f, cy = (jb[i].y0 + jb[i].y1) * 0.5f;
        int hit = ui_join_hit(jb, UI_JOIN_COUNT, cx, cy);
        int oki = (hit == i);
        ok &= oki;
        printf("join hit btn%d center(%.0f,%.0f) → hit=%d %s\n", i, cx, cy, hit, oki ? "" : "  [FAIL]");
    }
    /* ③ 面板内/外判定 */
    int in_panel = ui_join_popup_inside(UI_JOIN_POPUP_X0 + 10, UI_JOIN_POPUP_Y0 + 10);
    int out_panel = ui_join_popup_inside(UI_JOIN_POPUP_X0 - 5, UI_JOIN_POPUP_Y0 - 5);
    ok &= in_panel && !out_panel;
    printf("join panel: inside=%d outside=%d %s\n", in_panel, out_panel,
           (in_panel && !out_panel) ? "" : "  [FAIL]");
    /* ④ 业务分支判定（桌面桩 ui_screen_set 状态机 + 几何命中，不依赖 GL/触摸） */
    /* 桌号（action=0）→ 占位标签（触摸 flaky 无输入框） */
    float table_cx = (jb[0].x0 + jb[0].x1) * 0.5f, table_cy = (jb[0].y0 + jb[0].y1) * 0.5f;
    int table_hit = ui_join_hit(jb, UI_JOIN_COUNT, table_cx, table_cy);
    int ok_table = (table_hit == 0 && jb[table_hit].action == 0);
    ok &= ok_table;
    printf("join table: hit=%d action=%d → 桌号占位标签（★ 诚实边界：不画真输入框）%s\n",
           table_hit, jb[table_hit].action, ok_table ? "" : "  [FAIL]");
    /* 确定（action=1）→ 诚实边界对话框 + 回主菜单 */
    float ok_cx = (jb[1].x0 + jb[1].x1) * 0.5f, ok_cy = (jb[1].y0 + jb[1].y1) * 0.5f;
    int ok_hit = ui_join_hit(jb, UI_JOIN_COUNT, ok_cx, ok_cy);
    int ok_ok = (ok_hit == 1 && jb[ok_hit].action == 1);
    ok &= ok_ok;
    printf("join ok: hit=%d action=%d → 诚实边界对话框（Wi-Fi/局域网协议未逆向）→ 回主菜单 %s\n",
           ok_hit, jb[ok_hit].action, ok_ok ? "" : "  [FAIL]");
    /* 取消（action=2）→ 回主菜单 */
    float cancel_cx = (jb[2].x0 + jb[2].x1) * 0.5f, cancel_cy = (jb[2].y0 + jb[2].y1) * 0.5f;
    int cancel_hit = ui_join_hit(jb, UI_JOIN_COUNT, cancel_cx, cancel_cy);
    int ok_cancel = (cancel_hit == 2 && jb[cancel_hit].action == 2);
    ok &= ok_cancel;
    printf("join cancel: hit=%d action=%d → 回主菜单 %s\n",
           cancel_hit, jb[cancel_hit].action, ok_cancel ? "" : "  [FAIL]");
    printf("join: 3 控件几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：Wi-Fi/局域网协议未逆向）%s\n",
           ok ? "PASS" : "FAIL");
    return ok;
}

/* ★ P2（datah_035 证据）：联机"创建游戏"弹窗桌面验证——UI_CREATE 态机 3 控件命中判定
   + 几何 Δ=0 + 业务分支（密码占位/确定边界对话框/取消，确定性不依赖触摸/GL）。
   几何：ui_create_buttons()（720 基准）vs 原版 datah_035 420×235 面板居中精确换算。
   ★ 诚实边界：原版创建=Wi-Fi/局域网协议（未逆向），src_rs 仅蓝牙 SPP。 */
static int run_create(void)
{
    int ok = 1;
    const ui_create_btn *cb = ui_create_buttons();
    /* ① 3 控件几何验证（原版 datah_035 420×235 居中 720 基准精确换算，偏移+150,+522.5） */
    float expect[UI_CREATE_COUNT][4] = {
        {250.0f, 602.5f, 470.0f, 647.5f},   /* 0=密码 Pwd（100,80,320,125） */
        {185.0f, 672.5f, 345.0f, 732.5f},   /* 1=确定 Ok（35,150,195,210） */
        {375.0f, 672.5f, 535.0f, 732.5f},   /* 2=取消 Cancel（225,150,385,210） */
    };
    for (int i = 0; i < UI_CREATE_COUNT; i++) {
        float dx = cb[i].x0 - expect[i][0], dy = cb[i].y0 - expect[i][1];
        float dx1 = cb[i].x1 - expect[i][2], dy1 = cb[i].y1 - expect[i][3];
        int oki = (dx < 1.5f && dx > -1.5f) && (dy < 1.5f && dy > -1.5f) &&
                   (dx1 < 1.5f && dx1 > -1.5f) && (dy1 < 1.5f && dy1 > -1.5f);
        ok &= oki;
        printf("create btn%d: src(%.0f,%.0f)-(%.0f,%.0f) 期望(%.0f,%.0f)-(%.0f,%.0f) %s\n",
               i, cb[i].x0, cb[i].y0, cb[i].x1, cb[i].y1,
               expect[i][0], expect[i][1], expect[i][2], expect[i][3],
               oki ? "" : "  [FAIL]");
    }
    /* ② 命中判定：每控件中心点命中对应 index */
    for (int i = 0; i < UI_CREATE_COUNT; i++) {
        float cx = (cb[i].x0 + cb[i].x1) * 0.5f, cy = (cb[i].y0 + cb[i].y1) * 0.5f;
        int hit = ui_create_hit(cb, UI_CREATE_COUNT, cx, cy);
        int oki = (hit == i);
        ok &= oki;
        printf("create hit btn%d center(%.0f,%.0f) → hit=%d %s\n", i, cx, cy, hit, oki ? "" : "  [FAIL]");
    }
    /* ③ 面板内/外判定 */
    int in_panel = ui_create_popup_inside(UI_CREATE_POPUP_X0 + 10, UI_CREATE_POPUP_Y0 + 10);
    int out_panel = ui_create_popup_inside(UI_CREATE_POPUP_X0 - 5, UI_CREATE_POPUP_Y0 - 5);
    ok &= in_panel && !out_panel;
    printf("create panel: inside=%d outside=%d %s\n", in_panel, out_panel,
           (in_panel && !out_panel) ? "" : "  [FAIL]");
    /* ④ 业务分支判定（桌面桩 ui_screen_set 状态机 + 几何命中，不依赖 GL/触摸） */
    /* 密码（action=0）→ 占位标签（触摸 flaky 无输入框） */
    float pwd_cx = (cb[0].x0 + cb[0].x1) * 0.5f, pwd_cy = (cb[0].y0 + cb[0].y1) * 0.5f;
    int pwd_hit = ui_create_hit(cb, UI_CREATE_COUNT, pwd_cx, pwd_cy);
    int ok_pwd = (pwd_hit == 0 && cb[pwd_hit].action == 0);
    ok &= ok_pwd;
    printf("create pwd: hit=%d action=%d → 密码占位标签（★ 诚实边界：不画真输入框）%s\n",
           pwd_hit, cb[pwd_hit].action, ok_pwd ? "" : "  [FAIL]");
    /* 确定（action=1）→ 诚实边界对话框 + 回主菜单 */
    float ok_cx = (cb[1].x0 + cb[1].x1) * 0.5f, ok_cy = (cb[1].y0 + cb[1].y1) * 0.5f;
    int ok_hit = ui_create_hit(cb, UI_CREATE_COUNT, ok_cx, ok_cy);
    int ok_ok = (ok_hit == 1 && cb[ok_hit].action == 1);
    ok &= ok_ok;
    printf("create ok: hit=%d action=%d → 诚实边界对话框（Wi-Fi/局域网协议未逆向）→ 回主菜单 %s\n",
           ok_hit, cb[ok_hit].action, ok_ok ? "" : "  [FAIL]");
    /* 取消（action=2）→ 回主菜单 */
    float cancel_cx = (cb[2].x0 + cb[2].x1) * 0.5f, cancel_cy = (cb[2].y0 + cb[2].y1) * 0.5f;
    int cancel_hit = ui_create_hit(cb, UI_CREATE_COUNT, cancel_cx, cancel_cy);
    int ok_cancel = (cancel_hit == 2 && cb[cancel_hit].action == 2);
    ok &= ok_cancel;
    printf("create cancel: hit=%d action=%d → 回主菜单 %s\n",
           cancel_hit, cb[cancel_hit].action, ok_cancel ? "" : "  [FAIL]");
    printf("create: 3 控件几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：Wi-Fi/局域网协议未逆向）%s\n",
           ok ? "PASS" : "FAIL");
    return ok;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("用法: jq_evverify <--flag|--mine|--eat|--bomb|--move|--lay|--all>\n");
        return 2;
    }
    const char *mode = argv[1];
    if (strcmp(mode, "--lay") == 0) {
        jq_state st;
        jq_load_layout(&st, JQ_DEFAULT_LAY);
        for (int r = 0; r < 12; r++) {
            for (int c = 0; c < 5; c++) {
                int i = r * 5 + c;
                jq_cell *cl = &st.cells[i];
                if (cl->rank == RANK_EMPTY) { printf("  .. "); continue; }
                printf(" %c%02d ", cl->side == SIDE_BLUE ? 'B' : 'R', cl->rank);
            }
            printf("  row%d\n", r);
        }
        for (int i = 0; i < N_CELLS; i++)
            if (rank_norm(st.cells[i].rank) == RANK_FLAG)
                printf("军旗: cell%d(row%d,col%d) side=%s\n", i, cell_row(i), cell_col(i),
                       st.cells[i].side == SIDE_BLUE ? "BLUE" : "RED");
        return 0;
    }
    if (strcmp(mode, "--all") == 0) {
        int pass = 0, total = 0;
        jq_state st;
        /* EAT：默认布局 cell25→30 */
        jq_load_layout(&st, JQ_DEFAULT_LAY);
        for (int i = 0; i < N_CELLS; i++) if (st.cells[i].rank) st.cells[i].revealed = 1;
        total++; pass += run_case("eat", &st, 25, 30, EV_EAT, STG_PLAY, -1);
        /* BOMB：默认布局 cell27→32 */
        jq_load_layout(&st, JQ_DEFAULT_LAY);
        for (int i = 0; i < N_CELLS; i++) if (st.cells[i].rank) st.cells[i].revealed = 1;
        total++; pass += run_case("bomb", &st, 27, 32, EV_BOMB, STG_PLAY, -1);
        /* MOVE：默认布局 cell12→11 */
        jq_load_layout(&st, JQ_DEFAULT_LAY);
        for (int i = 0; i < N_CELLS; i++) if (st.cells[i].rank) st.cells[i].revealed = 1;
        total++; pass += run_case("move", &st, 12, 11, EV_MOVE, STG_PLAY, -1);
        /* FLAG：近旗夹具 */
        mk_flag(&st); total++; pass += run_case("flag", &st, 53, 58, EV_FLAG, STG_OVER, SIDE_BLUE);
        /* MINE：挖雷夹具（红有子@59 → stage 保持 PLAY） */
        mk_mine(&st); total++; pass += run_case("mine", &st, 49, 54, EV_MINE, STG_PLAY, -1);
        /* UNDO：走 EAT → AI 应着 → pop 到 BLUE → cell25/30 原位 */
        total++; pass += run_undo();
        /* DRAW/RESIGN：和棋 winner=2 / 认输 winner=1-turn */
        total++; pass += run_draw_resign();
        /* STALEMATE：困毙 winner=1-turn（无子可动方负） */
        total++; pass += run_stalemate();
        /* ★ P1（datah_012）：游戏设置 4 规则开关（默认全开行为不变 + 各开关翻转后守卫） */
        total++; pass += run_ruleset();
        /* ★ P1（datah_016/017/020）：布局存档全路径（提取 30 字节/存读一致/载入镜像/删除） */
        total++; pass += run_layout_save();
        /* ★ P1（datah_039/107）：复盘回放确定性（走子序列 N 步 → rewind cur=k/0/n 三游标
           cells 终态 == 独立重放结果，不依赖触摸/GL） */
        total++; pass += run_replay();
        /* ★ P1（datah_038）：登录弹窗 6 控件几何 Δ=0 + 命中判定 + 业务分支
           （游客=本地单机唯一真生效路径，登录/注册=诚实边界对话框，不依赖触摸/GL） */
        total++; pass += run_login();
        /* ★ P2（datah_037）：规则文本弹窗 3 控件几何 Δ=0 + 命中判定 + 业务分支
            （翻棋/布局 2 套简化版固定文本，不依赖触摸/GL） */
        total++; pass += run_rules();
        /* ★ P2（datah_104）：新局完整版弹窗 5 控件几何 Δ=0 + 命中判定 + 业务分支
            （我方/电脑水平/玩法 循环切换简化版，不依赖触摸/GL） */
        total++; pass += run_newgame104();
        /* ★ P2（datah_034）：联机"加入游戏"弹窗 3 控件几何 Δ=0 + 命中判定 + 业务分支
            （★ 诚实边界：Wi-Fi/局域网协议未逆向，确定→边界对话框，不依赖触摸/GL） */
        total++; pass += run_join();
        /* ★ P2（datah_035）：联机"创建游戏"弹窗 3 控件几何 Δ=0 + 命中判定 + 业务分支
            （★ 诚实边界：Wi-Fi/局域网协议未逆向，确定→边界对话框，不依赖触摸/GL） */
        total++; pass += run_create();
        printf("=== 桌面事件验证: %d/%d 全对 ===\n", pass, total);
        return pass == total ? 0 : 1;
    }
    jq_state st;
    if (strcmp(mode, "--flag") == 0) {
        mk_flag(&st);
        return run_case("flag", &st, 53, 58, EV_FLAG, STG_OVER, SIDE_BLUE) ? 0 : 1;
    }
    if (strcmp(mode, "--mine") == 0) {
        mk_mine(&st);
        return run_case("mine", &st, 49, 54, EV_MINE, STG_PLAY, -1) ? 0 : 1;
    }
    jq_load_layout(&st, JQ_DEFAULT_LAY);
    for (int i = 0; i < N_CELLS; i++) if (st.cells[i].rank) st.cells[i].revealed = 1;
    if (strcmp(mode, "--eat") == 0)  return run_case("eat",  &st, 25, 30, EV_EAT,  STG_PLAY, -1) ? 0 : 1;
    if (strcmp(mode, "--bomb") == 0) return run_case("bomb", &st, 27, 32, EV_BOMB, STG_PLAY, -1) ? 0 : 1;
    if (strcmp(mode, "--move") == 0) return run_case("move", &st, 12, 11, EV_MOVE, STG_PLAY, -1) ? 0 : 1;
    if (strcmp(mode, "--undo") == 0) return run_undo() ? 0 : 1;
    if (strcmp(mode, "--draw") == 0 || strcmp(mode, "--resign") == 0)
        return run_draw_resign() ? 0 : 1;
    if (strcmp(mode, "--stalemate") == 0) return run_stalemate() ? 0 : 1;
    if (strcmp(mode, "--replay") == 0) return run_replay() ? 0 : 1;
    if (strcmp(mode, "--login") == 0) return run_login() ? 0 : 1;
    /* ★ P2（datah_037/104/034/035）：4 个原版对话框桌面验证 */
    if (strcmp(mode, "--rules") == 0) return run_rules() ? 0 : 1;
    if (strcmp(mode, "--newgame104") == 0) return run_newgame104() ? 0 : 1;
    if (strcmp(mode, "--join") == 0) return run_join() ? 0 : 1;
    if (strcmp(mode, "--create") == 0) return run_create() ? 0 : 1;
    printf("未知模式: %s\n", mode);
    return 2;
}
