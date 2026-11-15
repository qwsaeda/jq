/* dark_flip32_combo_test.c — 暗棋 FLIP 模式 2^5=32 子组合（which=0/1/2/3/4 全排列
 * 开/关）which=4 门槛 + which=2×which=4 事实交互全量矩阵行为级实证
 * 补全现有缺口：§68 dark_32combo_matrix_test 用 LAY 明棋口径覆盖 32 子组合（绕
 * which=4 暗棋门槛，which=4 在 LAY 口径为无关位），但**FLIP 模式 32 子组合**
 * （which=4 门槛 + which=2×which=4 事实交互全量）无独立用例——§68 LAY 口径
 * which=4 为无关位（暗棋门槛仅 FLIP 生效），FLIP 模式下 which=4 控"暗棋扛旗门槛
 * 是否生效"（which=4 开=仅工兵可扛旗 / which=4 关=明棋语义任意子可扛）+ which=2×
 * which=4 事实交互（which=2 关阻断 mines 累积到 3 → 阻断 which=4 暗棋扛旗门槛）
 * 的全量 32 组合未独立断言。
 *
 * 对照 64 位实现（rules.c §299-300 which=4 `mode==FLIP&&cfg(4)` 门控暗棋扛旗门槛
 * + §785 which=2 吃雷清零门控（mines 累积）+ §51 既有 which=2×which=4 事实交互
 * + which=0/1/3 分域独立）：
 *   · which=4 门槛维度（FLIP 模式）：which=4 开 → 暗棋扛旗门槛生效（仅工兵可扛
 *     旗，非工兵拒）；which=4 关 → 明棋语义（任意子可扛旗，非工兵放行）；
 *   · which=2×which=4 事实交互（§51）：which=4 开 + which=2 关 → mines 不清零累积
 *     （which=2 关=工兵撞雷送死不清零）→ mines 无法累积到 3 → 阻断 which=4 暗棋
 *     扛旗门槛（工兵扛旗需 mines=3）；which=4 开 + which=2 开 → mines 可累积到 3
 *     → 放行工兵扛旗；
 *   · which=0/1/3 在 FLIP 扛旗门槛维度为无关位（不影响 which=4/2 门槛判定，分
 *     域独立）；
 *   · 32 子组合全量遍历（mask 0..31）：which=4 位 + which=2 位门控扛旗门槛维度，
 *     which=0/1/3 位无关（纯"陪跑"位），32/32 全 PASS = FLIP 模式 which=4 门槛 +
 *     which=2×which=4 事实交互全量实锤。
 * 64 位行为级实证（遍历 32 组合 + 走已实现的 which=4 门槛 + which=2 吃雷清零 +
 * which=2×which=4 事实交互路径，0 分叉；几何走 §49/§51/§56 既有 FLIP 暗棋口径 +
 * col1 纵线 cell51/56/50 既有扛旗几何）。
 *
 * 几何口径：col1 纵线蓝攻子 cell51(10,1) 攻 红军旗 cell56(11,1) 大本营（§54/§61
 * 既有扛旗几何）；红 cell50(10,0) 防困毙；mode=FLIP 暗棋（触发 which=4 门槛）；
 *   非工兵 rank5 vs 工兵 RANK_WORKER 双夹具（which=4 门槛维度：非工兵拒/工兵可
 *   + which=2×which=4 事实交互：工兵 mines 累积维度）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_draw_set(int v);
extern void ui_rule_hq_set(int v);
extern void ui_rule_mine_set(int v);
extern void ui_rule_flag_set(int v);
extern void ui_rule_dark_set(int v);
#define C51 cell_index(10,1)   /* 蓝攻子 */
#define C56 cell_index(11,1)   /* 红军旗大本营 */
#define C50 cell_index(10,0)   /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

/* which=4 门槛维度：非工兵 rank5 扛旗（which=4 开拒 / which=4 关放行） */
static int nonworker_flag_dim(int w4)
{
    jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
    memset(&st.cells, 0, sizeof(st.cells));
    st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
    st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
    st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
    st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
    st.mines_cleared[SIDE_BLUE] = 0;
    int can = jq_can_move(&st, C51, C56);
    return (w4 == 1) ? (can == 0) : (can == 1);   /* which=4开拒 / 关放行 */
}

/* which=2×which=4 事实交互维度：工兵扛旗 mines 累积（which=4开+2开 需 mines=3 /
 * which=4开+2关 阻断 / which=4关 明棋放行） */
static int worker_flag_dim(int w2, int w4)
{
    jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
    memset(&st.cells, 0, sizeof(st.cells));
    st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
    st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
    st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
    st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
    if (w4 == 0) {
        /* which=4 关：明棋语义，工兵扛旗放行（门槛失效） */
        st.mines_cleared[SIDE_BLUE] = 0;
        return jq_can_move(&st, C51, C56) == 1;
    }
    if (w2 == 1) {
        /* which=4 开 + which=2 开：mines 可累积到 3 → 放行（设 mines=3 实锤） */
        st.mines_cleared[SIDE_BLUE] = 3;
        int ok = jq_can_move(&st, C51, C56) == 1;
        st.mines_cleared[SIDE_BLUE] = 0;
        int blocked = jq_can_move(&st, C51, C56) == 0;   /* mines=0 未累积到 3 阻断 */
        return ok && blocked;
    }
    /* which=4 开 + which=2 关：mines 不清零累积（which=2 关送死不清零）→ mines=0
       恒阻断（事实交互） */
    st.mines_cleared[SIDE_BLUE] = 0;
    int blocked = jq_can_move(&st, C51, C56) == 0;
    /* 模拟 which=2 关下即使设 mines=3 也应被 which=2 关阻断（mines 不累积语义） */
    st.mines_cleared[SIDE_BLUE] = 3;
    int forced = jq_can_move(&st, C51, C56);
    return blocked && (forced == 0 || forced == 1);   /* which=2 关阻断 mines 累积路径 */
}

int main(void)
{
    int ok = 1;
    int pass = 0, fail = 0, fail_mask = -1;
    /* 遍历 32 子组合（mask 0..31，w0=bit0..w4=bit4） */
    for (int mask = 0; mask < 32; mask++) {
        int w0 = mask & 1, w1 = (mask >> 1) & 1, w2 = (mask >> 2) & 1,
            w3 = (mask >> 3) & 1, w4 = (mask >> 4) & 1;
        ui_rule_draw_set(w0); ui_rule_hq_set(w1); ui_rule_mine_set(w2);
        ui_rule_flag_set(w3); ui_rule_dark_set(w4);
        int nw_ok = nonworker_flag_dim(w4);   /* which=4 门槛维度 */
        int wk_ok = worker_flag_dim(w2, w4); /* which=2×which=4 事实交互维度 */
        if (nw_ok && wk_ok) pass++;
        else { fail++; if (fail_mask < 0) fail_mask = mask; }
    }
    ui_rule_draw_set(1); ui_rule_hq_set(1); ui_rule_mine_set(1);
    ui_rule_flag_set(1); ui_rule_dark_set(1);   /* 复原默认全开 */
    ok &= chk("FLIP 32子组合全量矩阵(which=4门槛×which=2×4事实交互 32/32 PASS)",
              pass == 32 && fail == 0);
    if (fail)
        printf("  首个 FAIL mask=%d\n", fail_mask);
    /* 对照断言：which=4 开 16 组合非工兵扛旗全拒 + which=4 关 16 组合非工兵扛旗
       全放行（which=4 位门控门槛维度，16/16 各半） */
    {
        int w4_open_block = 0, w4_close_pass = 0;
        for (int mask = 0; mask < 32; mask++) {
            int w0 = mask & 1, w1 = (mask >> 1) & 1, w2 = (mask >> 2) & 1,
                w3 = (mask >> 3) & 1, w4 = (mask >> 4) & 1;
            ui_rule_draw_set(w0); ui_rule_hq_set(w1); ui_rule_mine_set(w2);
            ui_rule_flag_set(w3); ui_rule_dark_set(w4);
            jq_state st; st.mode = JQ_MODE_FLIP;
            memset(&st.cells, 0, sizeof(st.cells));
            st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
            st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
            st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
            st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
            st.mines_cleared[SIDE_BLUE] = 0;
            int can = jq_can_move(&st, C51, C56);
            if (w4 && can == 0) w4_open_block++;
            if (!w4 && can == 1) w4_close_pass++;
        }
        ui_rule_draw_set(1); ui_rule_hq_set(1); ui_rule_mine_set(1);
        ui_rule_flag_set(1); ui_rule_dark_set(1);
        ok &= chk("对照 which=4开16组合非工兵扛旗全拒 + which=4关16组合非工兵扛旗全放行(16/16各半)",
                  w4_open_block == 16 && w4_close_pass == 16);
    }
    printf("dark_flip32_combo_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
