/* dark_32combo_matrix_test.c — 暗棋 2^5=32 子组合（which=0/1/2/3/4 全排列开/关）
 * 终局矩阵全量覆盖行为级实证
 * 补全现有缺口：§61 dark_multi_switch_matrix_test（which=2×which=3 两开关分域
 * 独立）/ §62 dark_allclose_switch_test（全开/全关两端）/ §63
 * dark_mixed_switch_test（代表中间混合组合：事实交互 vs 分域叠加 4 子组合）之外
 * 的**完整 2^5=32 子组合终局矩阵全量覆盖**（which=0/1/2/3/4 各开/关 32 种组合
 * × 扛旗 which=3 门控维度 + 吃雷 which=2 门控维度，32/32 全断言）。
 *
 * 对照 64 位实现（rules.c which=0 §519 和棋 / which=1 §237 大本营冻结 /
 * which=2 §746 吃雷清零 / which=3 §700 扛旗终局 / which=4 §292/310 暗棋门槛，
 * 5 开关分域独立 + which=2×which=4 事实交互）：
 *   · 扛旗维度（which=3 门控）：which=3 开 → 扛旗终局 OVER（winner 置）；which=3
 *     关 → 扛旗不置终局 PLAY（攻子入位，靠困毙近似）——32 组合中 which=3 位控
 *     制扛旗维度，与 which=0/1/2/4 无关（分域独立）；
 *   · 吃雷维度（which=2 门控）：which=2 开 → 工兵吃雷 EV_MINE（清零）；which=2
 *     关 → 工兵撞雷 EV_DEAD（送死不清零）——32 组合中 which=2 位控吃雷维度，
 *     与 which=0/1/3/4 无关（分域独立，which=2×which=4 事实交互在 LAY 明棋口径
 *     下不触发——which=4 暗棋门槛仅 FLIP 模式生效，本用例 LAY 口径绕开）；
 *   · 32 子组合全量遍历：每个 mask（w0..w4 5 位）断言扛旗 which=3 门控 + 吃雷
 *     which=2 门控双维度正确（32/32 全 PASS = 分域独立全量实锤）。
 * 64 位行为级实证（遍历 32 组合 + 走已实现的 which=2/3 门控路径，0 分叉；几何走
 * §54/§61 既有 LAY 明棋口径）。
 *
 * 几何口径：col1 纵线蓝攻子 cell51(10,1) 攻 红军旗 cell56(11,1)（§54/§61 扛旗
 *   几何）+ cell51 工兵攻雷 cell52(10,2)（which=2 吃雷几何）；红 cell50(10,0)
 *   防困毙；mode=LAY 明棋（绕暗棋工兵门槛 + which=4 门槛，32 组合纯 which=0/1/2/3
 *   分域叠加 + which=2/3 门控）。
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
#define C52 cell_index(10,2)   /* 红雷 */
#define C56 cell_index(11,1)   /* 红军旗大本营 */
#define C50 cell_index(10,0)   /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

/* 扛旗夹具（蓝 rank5@51 攻 旗@56），返 which=3 门控正确性 */
static int flag_dim(int w3)
{
    jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
    memset(&st.cells, 0, sizeof(st.cells));
    st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
    st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
    st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
    st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
    jq_try_move(&st, C51, C56);
    return (w3 == 1) ? (st.stage == STG_OVER) : (st.stage == STG_PLAY);
}

/* 吃雷夹具（蓝工兵@51 攻 雷@52），返 which=2 门控正确性 */
static int mine_dim(int w2)
{
    jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
    memset(&st.cells, 0, sizeof(st.cells));
    st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
    st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
    st.cells[C52].rank = RANK_MINE; st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;
    st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
    int ev = jq_try_move(&st, C51, C52);
    return (w2 == 1) ? (ev == EV_MINE) : (ev == EV_DEAD);
}

int main(void)
{
    int ok = 1;
    int pass = 0, fail = 0;
    int fail_mask = -1;
    /* 遍历 32 子组合（mask 0..31，w0=bit0..w4=bit4） */
    for (int mask = 0; mask < 32; mask++) {
        int w0 = mask & 1, w1 = (mask >> 1) & 1, w2 = (mask >> 2) & 1,
            w3 = (mask >> 3) & 1, w4 = (mask >> 4) & 1;
        ui_rule_draw_set(w0); ui_rule_hq_set(w1); ui_rule_mine_set(w2);
        ui_rule_flag_set(w3); ui_rule_dark_set(w4);
        int f_ok = flag_dim(w3);
        int m_ok = mine_dim(w2);
        if (f_ok && m_ok) pass++;
        else { fail++; if (fail_mask < 0) fail_mask = mask; }
    }
    ui_rule_draw_set(1); ui_rule_hq_set(1); ui_rule_mine_set(1);
    ui_rule_flag_set(1); ui_rule_dark_set(1);   /* 复原默认全开 */
    ok &= chk("32子组合全量矩阵(扛旗which=3门控×吃雷which=2门控 32/32 PASS)",
              pass == 32 && fail == 0);
    if (fail)
        printf("  首个 FAIL mask=%d\n", fail_mask);
    /* 对照断言：which=3 开 16 组合扛旗全 OVER + which=3 关 16 组合扛旗全 PLAY */
    {
        int w3_open_over = 0, w3_close_play = 0;
        for (int mask = 0; mask < 32; mask++) {
            int w0 = mask & 1, w1 = (mask >> 1) & 1, w2 = (mask >> 2) & 1,
                w3 = (mask >> 3) & 1, w4 = (mask >> 4) & 1;
            ui_rule_draw_set(w0); ui_rule_hq_set(w1); ui_rule_mine_set(w2);
            ui_rule_flag_set(w3); ui_rule_dark_set(w4);
            jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
            memset(&st.cells, 0, sizeof(st.cells));
            st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
            st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
            st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
            st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
            jq_try_move(&st, C51, C56);
            if (w3 && st.stage == STG_OVER) w3_open_over++;
            if (!w3 && st.stage == STG_PLAY) w3_close_play++;
        }
        ui_rule_draw_set(1); ui_rule_hq_set(1); ui_rule_mine_set(1);
        ui_rule_flag_set(1); ui_rule_dark_set(1);
        ok &= chk("对照 which=3开16组合扛旗全OVER + which=3关16组合扛旗全PLAY",
                  w3_open_over == 16 && w3_close_play == 16);
    }
    printf("dark_32combo_matrix_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
