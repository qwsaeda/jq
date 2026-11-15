/* dark_hq_freeze_test.c — 大本营（base）冻结开关（which=1 g_rule_hq）开/关行为级实证
 * 补全现有缺口：§37 诚实边界挂起"大本营开关（which=1 g_rule_hq）关闭分支未覆盖"
 * （边缘场景）。§237 既有口径"开关1=进大本营棋子冻结/关=可再动"首次行为级实锤。
 *
 * 对照 64 位实现（rules.c §237 `if (ui_rule_cfg_get(1) && is_base(from)) return false`
 * + board.c is_base 4 格）：
 *   · 开关 1 开（默认，对齐原版）：当前在大本营内的棋子不能动（进大本营冻结）；
 *   · 开关 1 关：进大本营的棋子可再动。
 * 64 位行为级实证（走已实现的 §237 大本营冻结分支，0 分叉）。
 *
 * 几何口径：大本营 4 格（蓝 (0,1)(0,3)=cell1/3 + 红 (11,1)(11,3)=cell56/58，
 *   §board.c is_base AUD-R-131 对称认 4 格）。cell56(row11col1) 上邻 cell51(row10col1)。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_hq_set(int v);

static int chk(char *name, int got, int want)
{
    int ok = (got == want);
    printf("%s: got=%d want=%d → %s\n", name, got, want, ok ? "PASS" : "FAIL");
    return ok;
}

int main(void)
{
    int ok = 1;
    /* ① 大本营拓扑：is_base 4 格（蓝 cell1/3 + 红 cell56/58） */
    ok &= chk("① 大本营拓扑 is_base(11,1)=1（红 cell56）", is_base(11,1), 1);
    ok &= chk("① 大本营拓扑 is_base(0,1)=1（蓝 cell1）", is_base(0,1), 1);
    ok &= chk("① 大本营拓扑 is_base(11,3)=1（红 cell58）", is_base(11,3), 1);
    ok &= chk("① 非大本营 is_base(10,1)=0（cell51）", is_base(10,1), 0);
    /* ② 开关 1 开：大本营内子冻结（蓝@cell56 不可走到 cell51） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[56].rank = 1; st.cells[56].side = SIDE_BLUE; st.cells[56].revealed = 1;
        st.cells[51].rank = 0;
        ui_rule_hq_set(1);
        int can = jq_can_move(&st, 56, 51);
        ok &= chk("② 开关1开=大本营内子冻结(蓝@56→51)", can, 0);
    }
    /* ③ 开关 1 关：进大本营的子可再动（同几何蓝@cell56 可走到 cell51） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[56].rank = 1; st.cells[56].side = SIDE_BLUE; st.cells[56].revealed = 1;
        st.cells[51].rank = 0;
        ui_rule_hq_set(0);
        int can = jq_can_move(&st, 56, 51);
        ok &= chk("③ 开关1关=进大本营子可再动(蓝@56→51)", can, 1);
        ui_rule_hq_set(1);   /* 复原默认开 */
    }
    printf("dark_hq_freeze_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
