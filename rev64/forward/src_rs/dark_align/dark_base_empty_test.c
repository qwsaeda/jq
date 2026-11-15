/* dark_base_empty_test.c — 大本营（base）目标格判定完整行为级实证
 * 补全现有缺口：dark_hq_freeze_test 覆盖"源格在大本营冻结"（开关 1 源侧），
 * 但**目标格在大本营**的判定（§352 AUD-R-071 空大本营不可进 + §357 AUD-R-029
 * 暗棋仅旗/雷例外）无独立用例。
 *
 * 对照 64 位实现（rules.c §352-357 大本营目标判定 + §32 暗棋雷/旗例外）：
 *   · 空大本营任何棋子不可进（return false，§352 AUD-R-071 几何事实——大本营格
 *     是"落点"非"子"，走子期空大本营无落点语义）；
 *   · 明棋大本营有军旗/地雷可扛（rulelay 任意子扛旗，§311）；
 *   · 暗棋（§357 AUD-R-029）大本营收窄为仅旗/雷可吃（非旗/雷拒）；
 *   · 空大本营拒进与大本营开关 1（源格侧冻结，§37 dark_hq_freeze_test）无关
 *     （§352 B01-021：开关 1 仅控"源格在大本营子可再动"，目标格侧空大本营恒拒
 *     是几何事实）。
 * 64 位行为级实证（走已实现的 §352-357 大本营目标判定，0 分叉）。
 *
 * 几何口径：红大本营 cell56(row11col1)，蓝方 cell51(row10col1) 攻 56（相邻）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"

static int chk(char *name, int got, int want)
{
    int ok = (got == want);
    printf("%s: got=%d want=%d → %s\n", name, got, want, ok ? "PASS" : "FAIL");
    return ok;
}

int main(void)
{
    int ok = 1;
    /* ① 明棋空大本营不可进（§352 AUD-R-071 几何事实，与开关 1 无关） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[51].rank = 1; st.cells[51].side = SIDE_BLUE; st.cells[51].revealed = 1;
        /* cell56 空 */
        ok &= chk("① 明棋空大本营不可进(蓝1@51攻空56)=0",
                  jq_can_move(&st, 51, 56), 0);
    }
    /* ② 明棋大本营有军旗可扛（对照 dark_flag_win_test，§311 任意子扛旗） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[51].rank = 1; st.cells[51].side = SIDE_BLUE; st.cells[51].revealed = 1;
        st.cells[56].rank = RANK_FLAG; st.cells[56].side = SIDE_RED; st.cells[56].revealed = 1;
        ok &= chk("② 明棋大本营有军旗可扛(蓝1@51攻56旗)=1",
                  jq_can_move(&st, 51, 56), 1);
    }
    /* ③ 暗棋空大本营不可进（§357 AUD-R-029 暗棋收窄仅旗/雷） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[51].rank = 5; st.cells[51].side = SIDE_BLUE; st.cells[51].revealed = 1;
        /* cell56 空 */
        ok &= chk("③ 暗棋空大本营不可进(蓝5@51攻空56)=0",
                  jq_can_move(&st, 51, 56), 0);
    }
    /* ④ 暗棋大本营有地雷工兵可挖（§32 既有口径，挖满 3 雷门槛） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[51].rank = RANK_WORKER; st.cells[51].side = SIDE_BLUE; st.cells[51].revealed = 1;
        st.cells[56].rank = RANK_MINE; st.cells[56].side = SIDE_RED; st.cells[56].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 3;
        ok &= chk("④ 暗棋大本营地雷工兵可挖(蓝工兵@51攻56雷,挖满)=1",
                  jq_can_move(&st, 51, 56), 1);
    }
    /* ⑤ 空大本营拒进与开关 1 无关（§352 B01-021）：开关 1 开/关，空大本营恒拒 */
    {
        extern void ui_rule_hq_set(int v);
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[51].rank = 1; st.cells[51].side = SIDE_BLUE; st.cells[51].revealed = 1;
        ui_rule_hq_set(1);
        int can_open = jq_can_move(&st, 51, 56);
        ui_rule_hq_set(0);
        int can_closed = jq_can_move(&st, 51, 56);
        ui_rule_hq_set(1);   /* 复原 */
        ok &= chk("⑤ 空大本营拒进与开关1无关(开/关均=0)",
                  (can_open == 0 && can_closed == 0), 1);
    }
    printf("dark_base_empty_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
