/* dark_cross_switch_test.c — 大本营开关（which=1 源格冻结）× 暗棋开关（which=4 目标例外）交叉门控行为级实证
 * 补全现有缺口：§42 dark_hq_freeze_test 覆盖 which=1 源格冻结（单开关），
 * §49 dark_flip_island_test 覆盖 which=4 暗棋例外（单开关），但**双开关交叉
 * 门控**（which=1 源格侧 × which=4 目标格侧独立判定）无独立用例（§49 挂起
 * "which=1 大本营冻结 + which=4 暗棋例外交叉门控"）。
 *
 * 对照 64 位实现（rules.c §245 which=1 源格 `if(cfg(1)&&is_base(from))return false`
 * + §301 which=4 目标 `if(mode==FLIP&&cfg(4)&&is_base(to)&&雷/旗例外)`）：
 *   · which=1（源格侧）：控"子已进大本营能否再动"（开=冻结 / 关=可再动）；
 *   · which=4（目标格侧）：控"暗棋下大本营能否被攻吃"（开=仅旗/雷例外 /
 *     关=退回明棋任意子可扛）；
 *   · **两侧判定独立**：源格侧 which=1 与目标格侧 which=4 互不影响（同一大本营
 *     格，源侧冻结 vs 目标侧例外是不同走子方向）。
 * 64 位行为级实证（走已实现的 §245 源格 + §301 目标例外双开关路径，0 分叉）。
 *
 * 几何口径：大本营 cell56(11,1)，上邻 cell51(10,1)（§32/§44/§49 既有几何）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_hq_set(int v);
extern void ui_rule_dark_set(int v);
#define C56 cell_index(11,1)   /* 大本营 */
#define C51 cell_index(10,1)   /* 上邻 */

static int chk(char *name, int got, int want)
{
    int ok = (got == want);
    printf("%s: got=%d want=%d → %s\n", name, got, want, ok ? "PASS" : "FAIL");
    return ok;
}

int main(void)
{
    int ok = 1;
    /* ① which=4开+which=1开：蓝工兵@大本营56 攻 51 → 源格冻结（which=1 生效） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C56].rank = RANK_WORKER; st.cells[C56].side = SIDE_BLUE; st.cells[C56].revealed = 1;
        st.cells[C51].rank = 0;
        ui_rule_dark_set(1); ui_rule_hq_set(1);
        ok &= chk("① which=4开+which=1开: 蓝工兵@大本营56→51=0(源格冻结)",
                  jq_can_move(&st, C56, C51), 0);
    }
    /* ② which=4开+which=1关：同几何 → 源格不冻结（which=1 关放行） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C56].rank = RANK_WORKER; st.cells[C56].side = SIDE_BLUE; st.cells[C56].revealed = 1;
        st.cells[C51].rank = 0;
        ui_rule_dark_set(1); ui_rule_hq_set(0);
        int can = jq_can_move(&st, C56, C51);
        ui_rule_hq_set(1);   /* 复原 */
        ok &= chk("② which=4开+which=1关: 蓝工兵@大本营56→51=1(源格不冻结)",
                  can, 1);
    }
    /* ③ which=4开+which=1开 目标侧例外：蓝工兵(51)挖满扛红军旗@56 → 放行（which=4 暗棋例外） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG;  st.cells[C56].side = SIDE_RED;  st.cells[C56].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 3;
        ui_rule_dark_set(1); ui_rule_hq_set(1);
        ok &= chk("③ which=4开+which=1开: 蓝工兵(51)挖满扛红军旗@56=1(目标例外放行)",
                  jq_can_move(&st, C51, C56), 1);
    }
    /* ④ 明棋+which=1开：蓝 rank5(51) 扛红军旗@56 → 任意子可扛（which=4 关退回明棋） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5;         st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED;  st.cells[C56].revealed = 1;
        ui_rule_dark_set(0); ui_rule_hq_set(1);
        int can = jq_can_move(&st, C51, C56);
        ui_rule_dark_set(1);   /* 复原 */
        ok &= chk("④ 明棋+which=1开: 蓝rank5(51)扛红军旗@56=1(明棋任意子可扛)",
                  can, 1);
    }
    /* ⑤ 双开关独立判定对照：which=1 源格冻结 与 which=4 目标例外 互不影响
       （同 cell56 大本营格，源侧攻出 vs 目标侧被攻 是不同走子方向） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C56].rank = RANK_WORKER; st.cells[C56].side = SIDE_BLUE; st.cells[C56].revealed = 1;
        st.cells[C51].rank = 0;
        ui_rule_dark_set(1); ui_rule_hq_set(1);
        int src_frozen = jq_can_move(&st, C56, C51);   /* 源侧：which=1 冻结 */
        /* 目标侧：同 cell56 有红军旗时，蓝工兵(51)攻 → which=4 例外放行 */
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 3;
        int tgt_allowed = jq_can_move(&st, C51, C56);  /* 目标侧：which=4 例外 */
        ok &= chk("⑤ 双开关独立(源侧冻结=0 且 目标侧例外放行=1)",
                  (src_frozen == 0 && tgt_allowed == 1), 1);
    }
    printf("dark_cross_switch_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
