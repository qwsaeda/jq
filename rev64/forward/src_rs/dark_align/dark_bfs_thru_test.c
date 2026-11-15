/* dark_bfs_thru_test.c — 工兵（BFS）穿己方子绕行语义行为级实证
 * 补全现有缺口 + §70 既有诚实标注"工兵 BFS 能否穿过己方子语义未定案"首次
 * 行为级定案：
 *   · 工兵恒走 rail_path_engineer（铁路网 BFS，§398-399 分派），非直线穿格——
 *     中间有子格仅阻断"直接进该子格"，不阻断经其他铁路路径**绕行到达**目标
 *     （目标格须空格 + 存在某条不穿子格的铁路连通路径）；
 *   · 实证：(5,0) 工兵 + (5,2) 己方子阻断 row5 直滑，但 (5,3)/(5,4)/(6,4) 仍
 *     可达（经 col2 纵线 + row6 横线等铁路网 BFS 绕行），目标格 (5,2) 本身
 *     不可进（子格）。
 * 对照 64 位实现（rules.c rail_path_engineer BFS + §398-399 工兵恒 engineer
 * 分派）：工兵穿己方子绕行语义行为级实锤（§70 未定案 → 定案"可绕行到达"）。
 * 64 位行为级实证（走已实现的 rail_path_engineer 路径，0 分叉）。
 *
 * 几何口径：row5/row6 横铁路 + col2 纵线交点（§40 铁路拓扑）；(5,0) 工兵，
 *   (5,2) 己方子阻断 row5 直滑，绕行经铁路网到 (5,4)/(6,4)。
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
    /* ① 全空铁路：工兵 (5,0) 滑 (5,4) 可达（baseline，对照 rail_block_test） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(5,0)].rank = RANK_WORKER;
        st.cells[cell_index(5,0)].side = SIDE_BLUE; st.cells[cell_index(5,0)].revealed = 1;
        ok &= chk("① 全空工兵(5,0)→(5,4)=1(baseline)",
                  jq_can_move(&st, cell_index(5,0), cell_index(5,4)), 1);
    }
    /* ② 中间己方子阻断直滑，但绕行可达 (5,4)（§70 定案核心） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(5,0)].rank = RANK_WORKER;
        st.cells[cell_index(5,0)].side = SIDE_BLUE; st.cells[cell_index(5,0)].revealed = 1;
        st.cells[cell_index(5,2)].rank = 3;
        st.cells[cell_index(5,2)].side = SIDE_BLUE; st.cells[cell_index(5,2)].revealed = 1;
        ok &= chk("② 中间己方子(5,2)阻断直滑, (5,2)本身不可进",
                  jq_can_move(&st, cell_index(5,0), cell_index(5,2)), 0);
        ok &= chk("② 中间己方子(5,2), 绕行(5,0)→(5,4)=1(§70可绕行到达)",
                  jq_can_move(&st, cell_index(5,0), cell_index(5,4)), 1);
    }
    /* ③ 中间敌方子同样可绕行（己/敌对称，子格不可进但绕行可达） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(5,0)].rank = RANK_WORKER;
        st.cells[cell_index(5,0)].side = SIDE_BLUE; st.cells[cell_index(5,0)].revealed = 1;
        st.cells[cell_index(5,2)].rank = 3;
        st.cells[cell_index(5,2)].side = SIDE_RED; st.cells[cell_index(5,2)].revealed = 1;
        ok &= chk("③ 中间敌方子(5,2), 绕行(5,0)→(5,4)=1(己/敌对称)",
                  jq_can_move(&st, cell_index(5,0), cell_index(5,4)), 1);
    }
    /* ④ 非工兵直走不绕行（rail_path_straight 直线，中间有子=阻断，对照工兵 BFS） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(5,0)].rank = 5;   /* 非工兵 rank5 */
        st.cells[cell_index(5,0)].side = SIDE_BLUE; st.cells[cell_index(5,0)].revealed = 1;
        st.cells[cell_index(5,2)].rank = 3;
        st.cells[cell_index(5,2)].side = SIDE_BLUE; st.cells[cell_index(5,2)].revealed = 1;
        ok &= chk("④ 非工兵直走(5,0)→(5,4)中间己方子阻断=0(不绕行,对照工兵)",
                  jq_can_move(&st, cell_index(5,0), cell_index(5,4)), 0);
    }
    printf("dark_bfs_thru_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
