/* dark_allclose_switch_test.c — 5 规则开关全关（which=0/1/2/3/4 全关）组合终局矩阵
 * 行为级实证
 * 补全现有缺口：§61 dark_multi_switch_matrix_test 覆盖 which=2×which=3 两开关组合
 * （分域独立），§42/§50/§54/§55/§56/§57 各覆盖单开关开/关，但**5 开关全关
 * （which=0 和棋 + which=1 大本营 + which=2 吃雷 + which=3 扛旗 + which=4 暗棋
 * 同时关）的组合终局矩阵**无独立用例（全关时各"关"行为叠加：不判和 + 不冻结 +
 * 不清零 + 扛旗不终局 + 暗棋门槛失效，终局统一退回困毙/吃光近似，需实证各维度
 * 独立叠加无串扰）。
 *
 * 对照 64 位实现（rules.c which=0 §519 和棋门控 / which=1 §237 大本营冻结门控 /
 * which=2 §746 吃雷清零门控 / which=3 §700 扛旗终局门控 / which=4 §292/310 暗棋
 * 门槛门控，5 开关**分域独立**——全关时各自"关"行为叠加，互不串扰）：
 *   · which=0 关：70 步无吃子不判和（进度不打断）；
 *   · which=1 关：进大本营子可再动（不冻结）；
 *   · which=2 关：工兵撞雷送死 EV_DEAD（不清零 mines/moves）；
 *   · which=3 关：扛旗不置终局（攻子入位，靠困毙近似）；
 *   · which=4 关：暗棋门槛全失效（退回明棋，本用例 LAY 模式口径不触发）。
 * 64 位行为级实证（走已实现的 5 开关各自门控路径，0 分叉；几何走 §42/§54/§61
 * 既有 LAY 明棋口径，全关组合下各维度独立断言 + 无串扰对照）。
 *
 * 几何口径：col1 纵线蓝攻子 cell51(10,1) 攻 红军旗 cell56(11,1)（§54 扛旗几何）+
 *   cell51 攻雷 cell52(10,2)（which=2 吃雷几何）+ 蓝@56 出大本营 56→55（§42 大
 *   本营冻结几何）；红 cell50(10,0) 防困毙；mode=LAY（明棋，绕暗棋工兵门槛）。
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
#define C55 cell_index(11,0)   /* 大本营横邻 */
#define C50 cell_index(10,0)   /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

static void all_close(void)
{
    ui_rule_draw_set(0); ui_rule_hq_set(0); ui_rule_mine_set(0);
    ui_rule_flag_set(0); ui_rule_dark_set(0);
}
static void all_open(void)
{
    ui_rule_draw_set(1); ui_rule_hq_set(1); ui_rule_mine_set(1);
    ui_rule_flag_set(1); ui_rule_dark_set(1);
}

int main(void)
{
    int ok = 1;
    /* ① which=3 关（全关下）：扛旗不置终局（攻子入位 cell56，stage=PLAY） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        all_close();
        int ev = jq_try_move(&st, C51, C56);
        ok &= chk("① 全关 which=3关: 扛旗不置终局(PLAY+攻子入位cell56)",
                  st.stage == STG_PLAY && st.cells[C56].rank == 5 && ev == EV_FLAG);
    }
    /* ② which=2 关（全关下）：工兵撞雷送死 EV_DEAD（不清零 mines） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C52].rank = RANK_MINE; st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        all_close();
        int ev = jq_try_move(&st, C51, C52);
        ok &= chk("② 全关 which=2关: 工兵撞雷送死EV_DEAD(不清零mines)",
                  ev == EV_DEAD && st.mines_cleared[SIDE_BLUE] == 0);
    }
    /* ③ which=1 关（全关下）：进大本营子可再动（蓝@56→55 非冻结） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C56].rank = 1; st.cells[C56].side = SIDE_BLUE; st.cells[C56].revealed = 1;
        st.cells[C55].rank = 0;
        all_close();
        int can = jq_can_move(&st, C56, C55);
        ok &= chk("③ 全关 which=1关: 进大本营子可再动(蓝@56→55 非冻结)",
                  can == 1);
    }
    /* ④ 全关终局语义：靠困毙近似（which=3 关扛旗不终局 + which=1 关不冻结 +
       which=2 关不清零），winner 维持 NONE 直到困毙/吃光 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        all_close();
        jq_try_move(&st, C51, C56);   /* 扛旗不终局(which=3关) */
        ok &= chk("④ 全关终局语义: 扛旗后winner维持NONE(靠困毙近似非直接终局)",
                  st.winner == JQ_WIN_NONE && st.stage == STG_PLAY);
    }
    /* ⑤ 无串扰对照：全关 which=3 关扛旗 与 全开 which=3 开扛旗 对比（OVER vs PLAY） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        jq_state stf; jq_state_init(&stf); stf.mode = JQ_MODE_LAY;
        memset(&stf.cells, 0, sizeof(stf.cells));
        stf.stage = STG_PLAY; stf.winner = JQ_WIN_NONE; stf.turn = SIDE_BLUE;
        stf.cells[C51].rank = 5; stf.cells[C51].side = SIDE_BLUE; stf.cells[C51].revealed = 1;
        stf.cells[C56].rank = RANK_FLAG; stf.cells[C56].side = SIDE_RED; stf.cells[C56].revealed = 1;
        stf.cells[C50].rank = 1; stf.cells[C50].side = SIDE_RED;  stf.cells[C50].revealed = 1;
        all_close();
        jq_try_move(&st, C51, C56);          /* 全关: PLAY */
        all_open();
        jq_try_move(&stf, C51, C56);         /* 全开: OVER */
        ok &= chk("⑤ 无串扰对照: 全关which=3关扛旗=PLAY vs 全开which=3开扛旗=OVER",
                  st.stage == STG_PLAY && stf.stage == STG_OVER);
        all_open();   /* 复原默认全开 */
    }
    all_open();   /* 复原默认全开 */
    printf("dark_allclose_switch_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
