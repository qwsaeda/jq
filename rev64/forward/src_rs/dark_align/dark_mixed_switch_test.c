/* dark_mixed_switch_test.c — 暗棋多开关混合组合（which=0/1/2/3/4 任意中间组合）终局
 * 矩阵行为级实证
 * 补全现有缺口：§61 dark_multi_switch_matrix_test 覆盖 which=2×which=3 两开关组合
 * （分域独立），§62 dark_allclose_switch_test 覆盖"全开/全关"两端，但**中间任意
 * 混合组合**（如 which=2 关 + which=3 开 + which=4 关 三开关混合）的终局矩阵无独
 * 立用例——混合组合下 §51 which=2×which=4 **事实交互**（which=2 关阻断 mines 累
 * 积到 3 → 阻断 which=4 开暗棋扛旗门槛）与 §61/§62 which=0/1/2/3 **分域叠加**
 * （各维度独立）的交互叠加未独立断言。
 *
 * 对照 64 位实现（rules.c which=0 §519 和棋 / which=1 §237 大本营冻结 / which=2
 * §746 吃雷清零 / which=3 §700 扛旗终局 / which=4 §292/310 暗棋门槛，5 开关分域
 * 独立 + which=2×which=4 事实交互）：
 *   · 混合A（which=4 关 + which=3 开 + which=2 关）：扛旗维度 which=3 开→终局
 *     OVER（which=4 关明棋语义不阻断 which=3 终局，which=2 关不影响扛旗维度）；
 *   · 混合B（which=4 开 + which=3 开 + which=2 关）：非工兵扛旗 暗棋门槛
 *     （which=4 开）+ mines=0（which=2 关阻断累积到 3）→ 拒（§51 事实交互：
 *     which=2 关阻断 which=4 暗棋扛旗门槛路径）；
 *   · 混合C（which=4 关 + which=3 关 + which=2 开）：工兵吃雷 which=2 开→EV_MINE
 *     清零（which=3/4 关不影响吃雷维度，分域叠加）。
 * 64 位行为级实证（走已实现的 5 开关各自门控路径，0 分叉；几何走 §49/§51/§54
 * 既有口径，混合组合下事实交互 vs 分域叠加对照）。
 *
 * 几何口径：col1 纵线蓝攻子 cell51(10,1) 攻 红军旗 cell56(11,1)（§54 扛旗几何）+
 *   cell51 工兵攻雷 cell52(10,2)（which=2 吃雷几何）；红 cell50(10,0) 防困毙；
 *   混合A/C LAY 明棋口径（绕暗棋工兵门槛），混合B FLIP 暗棋口径（触发 which=4
 *   门槛 × which=2 阻断事实交互）。
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

static void set_switches(int which2, int which3, int which4)
{
    ui_rule_draw_set(1); ui_rule_hq_set(1);
    ui_rule_mine_set(which2); ui_rule_flag_set(which3); ui_rule_dark_set(which4);
}

int main(void)
{
    int ok = 1;
    /* 混合A：which=4关 + which=3开 + which=2关 → 扛旗终局（which=3开不阻断） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        set_switches(0, 1, 0);   /* which=2关+which=3开+which=4关 */
        int ev = jq_try_move(&st, C51, C56);
        ok &= chk("混合A which=4关+3开+2关 扛旗终局(which=3开不阻断OVER)",
                  ev == EV_FLAG && st.stage == STG_OVER);
    }
    /* 混合B：which=4开 + which=3开 + which=2关 → 非工兵扛旗拒（§51 事实交互：
       which=2关 mines=0 阻断 which=4 暗棋扛旗门槛） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 0;
        set_switches(0, 1, 1);   /* which=2关+which=3开+which=4开(暗棋门槛) */
        int can = jq_can_move(&st, C51, C56);
        ok &= chk("混合B which=4开+3开+2关 非工兵扛旗拒(§51事实交互which=2关阻断门槛)",
                  can == 0);
    }
    /* 混合C：which=4关 + which=3关 + which=2开 → 工兵吃雷清零（which=2开不影响
       which=3/4 关维度，分域叠加） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C52].rank = RANK_MINE; st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        set_switches(1, 0, 0);   /* which=2开+which=3关+which=4关 */
        int ev = jq_try_move(&st, C51, C52);
        ok &= chk("混合C which=4关+3关+2开 工兵吃雷EV_MINE清零(which=2开分域独立)",
                  ev == EV_MINE && st.mines_cleared[SIDE_BLUE] == 1);
    }
    /* 混合D（对照 §51 既有 which=2×which=4 事实交互）：which=4开+which=2开+
       which=3开 非工兵扛旗 仍拒（which=2 开但 mines=0 累积未达 3，门槛仍阻断） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 0;
        set_switches(1, 1, 1);   /* which=2开+which=3开+which=4开 全开(对照) */
        int can = jq_can_move(&st, C51, C56);
        ok &= chk("混合D 全开which=4开+2开 非工兵扛旗拒(mines=0未达3门槛§51对照)",
                  can == 0);
    }
    set_switches(1, 1, 1);   /* 复原默认全开 */
    printf("dark_mixed_switch_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
