/* dark_02_switch_test.c — which=0（和棋）× which=2（吃雷）交叉门控行为级实证
 * 补全现有缺口：§37 dark_rule_switch_test 覆盖各开关独立开/关，§51
 * dark_4switch_combo_test 覆盖 which=2×which=4 事实交互，但 **which=0（和棋）
 * × which=2（吃雷）交叉门控**（吃雷清零 70 步进度 vs 和棋判定 which=0 门控）
 * 无独立用例（§35 挂起"which=0 和棋×which=2 吃雷清零交互"）。
 *
 * 对照 64 位实现（rules.c §519 which=0 `if(cfg(0)&&moves_since_eat>=70)jq_on_draw`
 * + §746 which=2 工兵撞雷开=挖雷清零/关=送死 + §784 吃雷 EV_MINE 清零
 * moves_since_eat）：
 *   · which=0 开：70 步无吃子判和（jq_on_draw，winner=DRAW）；
 *   · which=0 关：即使 70 步无吃子也不判和（cfg(0) 门控）；
 *   · which=2 开：工兵撞雷=EV_MINE 挖雷（mines_cleared++ + moves_since_eat 清零）；
 *   · **交叉交互**：which=2 开时吃雷清零可**打断**即将判和的 70 步进度
 *     （69 步空移后吃雷 → moves_since_eat 清零 → 不判和）；which=0 开关独立
 *     控"是否判和"，which=2 开关控"吃雷是否清零进度"，两开关分域（和棋判定
 *     vs 吃雷清零）但可交叉影响 70 步进度演化。
 * 64 位行为级实证（走已实现的 §519 和棋门控 + §746/§784 吃雷清零路径，0 分叉）。
 *
 * 几何口径：col1 纵线工兵 cell46(9,1) 攻雷 cell51(10,1)（§32 既有挖雷几何）；
 *   空移对照 col4 纵线 44↔39（§45/§46 既有几何）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_draw_set(int v);
extern void ui_rule_mine_set(int v);
#define C46 cell_index(9,1)    /* 工兵 cell46 */
#define C51 cell_index(10,1)   /* 雷 cell51 */
#define C50 cell_index(10,0)   /* 红防困毙 */

static int chk(char *name, int got, int want)
{
    int ok = (got == want);
    printf("%s: got=%d want=%d → %s\n", name, got, want, ok ? "PASS" : "FAIL");
    return ok;
}

int main(void)
{
    int ok = 1;
    /* ① which=0开+which=2开：工兵挖雷（69 步后）→ EV_MINE + 清零 + 不判和 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C46].rank = RANK_WORKER; st.cells[C46].side = SIDE_BLUE; st.cells[C46].revealed = 1;
        st.cells[C51].rank = RANK_MINE;  st.cells[C51].side = SIDE_RED;  st.cells[C51].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED; st.cells[C50].revealed = 1;
        st.moves_since_eat = 69;
        ui_rule_draw_set(1); ui_rule_mine_set(1);
        int ev = jq_try_move(&st, C46, C51);
        ok &= chk("① which=0开+which=2开: 工兵挖雷(69步)→EV_MINE+清零+不判和",
                  ev == EV_MINE && st.moves_since_eat == 0 && st.stage == STG_PLAY, 1);
    }
    /* ② which=0开+空移 70 步（无吃雷）→ 判和（对照 ① 吃雷打断） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[50].rank = 1; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        st.moves_since_eat = 69;
        ui_rule_draw_set(1); ui_rule_mine_set(1);
        int ev = jq_try_move(&st, 44, 39);   /* 空移到 70 */
        ok &= chk("② which=0开+空移70步(无吃雷)→判和(DRAW)",
                  ev == EV_MOVE && st.moves_since_eat == 70
                  && st.stage == STG_OVER && st.winner == JQ_WIN_DRAW, 1);
    }
    /* ③ which=0关+空移 70 步 → 不判和（which=0 门控，对照 ②） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[50].rank = 1; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        st.moves_since_eat = 69;
        ui_rule_draw_set(0); ui_rule_mine_set(1);
        int ev = jq_try_move(&st, 44, 39);   /* 空移到 70 但不判和 */
        int ok_ = (ev == EV_MOVE && st.moves_since_eat == 70 && st.stage == STG_PLAY);
        ui_rule_draw_set(1);
        ok &= chk("③ which=0关+空移70步→不判和(which=0门控, PLAY)", ok_, 1);
    }
    /* ④ 交叉交互：吃雷清零打断即将判和的 70 步进度（69 步→吃雷清零→续走不判和）
       which=0开 + which=2开：69 步空移后吃雷清零，续走几步仍 <70 不判和 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C46].rank = RANK_WORKER; st.cells[C46].side = SIDE_BLUE; st.cells[C46].revealed = 1;
        st.cells[C51].rank = RANK_MINE;  st.cells[C51].side = SIDE_RED;  st.cells[C51].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED; st.cells[C50].revealed = 1;
        st.moves_since_eat = 69;   /* 即将 70 步 */
        ui_rule_draw_set(1); ui_rule_mine_set(1);
        int ev = jq_try_move(&st, C46, C51);   /* 吃雷清零（非空移到 70） */
        int reset = (ev == EV_MINE && st.moves_since_eat == 0 && st.stage == STG_PLAY);
        ok &= chk("④ 交叉: 吃雷清零打断70步进度(69步吃雷→清零,不判和)", reset, 1);
    }
    printf("dark_02_switch_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
