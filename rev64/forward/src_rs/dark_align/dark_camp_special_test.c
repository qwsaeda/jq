/* dark_camp_special_test.c — 行营（camp）安全岛对特殊子（雷/旗）无例外行为级实证
 * 补全现有缺口：§41 dark_camp_safety_test 覆盖行营安全岛基本语义（敌方不可吃
 * 行营中普通子 + 空行营可入 + 己方可走出），但**行营安全岛对特殊子（雷/旗）
 * 是否例外**（§41 诚实边界挂起"行营中炸弹/地雷特殊子语义未独立用例"）+
 * **与大本营安全岛例外的差异对照**未覆盖。
 *
 * 对照 64 位实现（rules.c §287 行营 `if (is_camp(to) && dst->rank != RANK_EMPTY)
 * return false` 无 rank 例外 + §357 大本营 `is_base` 仅旗/雷可吃例外）：
 *   · **行营安全岛无例外**：行营中雷/旗/普通子**都不可被敌方吃**（is_camp 判定
 *     不区分 rank，dst 有任何子即拒）；
 *   · **大本营安全岛有例外**（§357）：大本营中雷仅工兵可挖 / 旗仅工兵挖满 3 雷
 *     可扛（rank 例外）；
 *   · 差异 = 行营"全拒" vs 大本营"旗/雷例外"——行营中地雷工兵也挖不了、行营中
 *     军旗工兵也扛不了（对照行营外/大本营内可挖/可扛）。
 * 64 位行为级实证（走已实现的 §287 行营无例外 + §357 大本营例外路径，0 分叉）。
 *
 * 几何口径：行营 (3,2)（col2 纵线隔断点，§40 拓扑），蓝方 cell42(4,2) 攻 行营
 *   cell32(3,2)；对照行营外雷 cell51(10,1) 蓝工兵 cell46(9,1) 攻（§32 既有口径）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C32 cell_index(3,2)    /* 行营(3,2) */
#define C42 cell_index(4,2)    /* (4,2) col2 纵线 */
#define C51 cell_index(10,1)   /* 行营外雷 cell51 */
#define C46 cell_index(9,1)    /* 蓝工兵 cell46 */

static int chk(char *name, int got, int want)
{
    int ok = (got == want);
    printf("%s: got=%d want=%d → %s\n", name, got, want, ok ? "PASS" : "FAIL");
    return ok;
}

int main(void)
{
    int ok = 1;
    /* ① 行营中地雷不可被工兵挖（行营安全岛无例外，对照行营外可挖） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = RANK_WORKER; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C32].rank = RANK_MINE;  st.cells[C32].side = SIDE_RED;  st.cells[C32].revealed = 1;
        ok &= chk("① 行营中红雷+蓝工兵攻=0(行营安全岛无例外,雷不可挖)",
                  jq_can_move(&st, C42, C32), 0);
    }
    /* ② 行营中军旗不可被工兵扛（行营安全岛无例外，对照大本营例外可扛） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = RANK_WORKER; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C32].rank = RANK_FLAG;   st.cells[C32].side = SIDE_RED;  st.cells[C32].revealed = 1;
        ok &= chk("② 行营中红军旗+蓝工兵攻=0(行营安全岛无例外,旗不可扛)",
                  jq_can_move(&st, C42, C32), 0);
    }
    /* ③ 行营中普通子不可被吃（安全岛基本语义，对照 dark_camp_safety_test ③） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = 1; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C32].rank = 3; st.cells[C32].side = SIDE_RED;  st.cells[C32].revealed = 1;
        ok &= chk("③ 行营中红rank3+蓝rank1攻=0(行营安全岛基本语义)",
                  jq_can_move(&st, C42, C32), 0);
    }
    /* ④ 对照：行营外地雷工兵可挖（§32 既有口径，row10col1 非行营） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C46].rank = RANK_WORKER; st.cells[C46].side = SIDE_BLUE; st.cells[C46].revealed = 1;
        st.cells[C51].rank = RANK_MINE;   st.cells[C51].side = SIDE_RED;  st.cells[C51].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 3;
        int can = jq_can_move(&st, C46, C51);
        ok &= chk("④ 对照行营外地雷工兵可挖(46→51)=1(对照行营内不可挖)", can, 1);
    }
    /* ⑤ 对照：大本营地雷工兵可挖（§32 既有口径，row11col1 大本营） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[cell_index(10,1)].rank = RANK_WORKER; st.cells[cell_index(10,1)].side = SIDE_BLUE; st.cells[cell_index(10,1)].revealed = 1;
        st.cells[cell_index(11,1)].rank = RANK_MINE;  st.cells[cell_index(11,1)].side = SIDE_RED;  st.cells[cell_index(11,1)].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 3;
        int can = jq_can_move(&st, cell_index(10,1), cell_index(11,1));
        ok &= chk("⑤ 对照大本营地雷工兵可挖(51→56)=1(大本营例外,对照行营无例外)", can, 1);
    }
    printf("dark_camp_special_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
