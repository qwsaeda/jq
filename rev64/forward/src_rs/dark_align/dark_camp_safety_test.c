/* dark_camp_safety_test.c — 行营（camp）安全岛完整行为级实证
 * 补全现有缺口：rule_text_conformance 72-76 行仅覆盖"明棋敌方不能吃行营中子"
 * 单场景，但**行营完整拓扑（is_camp 10 格）+ 空行营可入 + 己方可走出行营 +
 * 暗棋 FLIP 行营安全岛对照**未独立覆盖。
 *   ★ 注释漂移纠正：board.c:95 AUD-R-067 标注"行营 6 格"，实际 is_camp 拓扑
 *     = 10 格（蓝方 row2-4 区 5 格 (2,1)(2,3)(3,2)(4,1)(4,3) + 红方 row7-9 区
 *     5 格对称 (7,1)(7,3)(8,2)(9,1)(9,3)）——"6 格"系旧口径，is_camp 行为
 *     唯一事实源 = 10 格（本用例实锤）。
 *
 * 对照 64 位实现（board.c is_camp + rules.c move_legal 行营安全岛判定
 * §76 行营非铁路不可穿行 / §130 目标判定并入 walkable 行营端点拦）：
 *   · 行营 = 安全岛（敌方不能**吃**行营中棋子，空行营可入，己方可走出）；
 *   · 行营非铁路（is_rail 对 camp 返 false，不可穿行 col2 隔断）。
 * 64 位行为级实证（走已实现的 is_camp + move_legal 行营安全岛路径，0 分叉）。
 *
 * 几何口径：行营 (3,2) 是 col2 纵线隔断点（§33/§40 行营隔断口径），蓝方 row2-4
 * 行营区。相邻格：(4,2)（下）/ (2,2)（上，非行营）/ (3,1)(3,3)（左右，非行营）。
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
    /* ① 行营拓扑：is_camp 10 格（蓝 row2-4 + 红 row7-9 对称），非行营对照 */
    ok &= chk("① 行营拓扑 (2,1)camp", is_camp(2,1), 1);
    ok &= chk("① 行营拓扑 (3,2)camp", is_camp(3,2), 1);
    ok &= chk("① 行营拓扑 (4,3)camp", is_camp(4,3), 1);
    ok &= chk("① 行营拓扑 (9,1)camp（红对称）", is_camp(9,1), 1);
    ok &= chk("① 非行营 (4,2)（col2 纵线，非 camp）", is_camp(4,2), 0);
    ok &= chk("① 行营非铁路 is_rail(3,2)=0（col2 隔断点）", is_rail(3,2), 0);
    /* ② 空行营可入：(4,2) 蓝 rank1 攻 (3,2) 空行营 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,2)].rank = 1;
        st.cells[cell_index(4,2)].side = SIDE_BLUE; st.cells[cell_index(4,2)].revealed = 1;
        ok &= chk("② 空行营可入(蓝1从4,2攻空3,2)",
                  jq_can_move(&st, cell_index(4,2), cell_index(3,2)), 1);
    }
    /* ③ 敌方不可吃行营中子（安全岛）：(3,2) 有红 rank1，蓝攻拒 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,2)].rank = 1; st.cells[cell_index(4,2)].side = SIDE_BLUE; st.cells[cell_index(4,2)].revealed = 1;
        st.cells[cell_index(3,2)].rank = 1; st.cells[cell_index(3,2)].side = SIDE_RED;  st.cells[cell_index(3,2)].revealed = 1;
        ok &= chk("③ 敌方不可吃行营中子(蓝攻红@行营3,2)",
                  jq_can_move(&st, cell_index(4,2), cell_index(3,2)), 0);
    }
    /* ④ 己方可走出行营：蓝 rank1 在行营 (3,2)，走出到相邻空格 (4,2) */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[cell_index(3,2)].rank = 1; st.cells[cell_index(3,2)].side = SIDE_BLUE; st.cells[cell_index(3,2)].revealed = 1;
        ok &= chk("④ 己方可走出行营(蓝@3,2→4,2空)",
                  jq_can_move(&st, cell_index(3,2), cell_index(4,2)), 1);
    }
    /* ⑤ 暗棋 FLIP 行营安全岛对照：敌方不可吃行营中未翻子（安全岛跨明/暗棋一致） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,2)].rank = 1; st.cells[cell_index(4,2)].side = SIDE_BLUE; st.cells[cell_index(4,2)].revealed = 1;
        st.cells[cell_index(3,2)].rank = 3; st.cells[cell_index(3,2)].side = SIDE_RED;  st.cells[cell_index(3,2)].revealed = 0;
        ok &= chk("⑤ 暗棋敌方不可吃行营中未翻子(蓝攻红@行营3,2)",
                  jq_can_move(&st, cell_index(4,2), cell_index(3,2)), 0);
    }
    printf("dark_camp_safety_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
