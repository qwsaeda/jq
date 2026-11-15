/* dark_rail_walk_test.c — 走子路径分派边界（相邻走子 vs 铁路滑行 vs 非铁路远格）行为级实证
 * 补全现有缺口：dark_bfs_thru_test 覆盖工兵 BFS 穿己方子（铁路滑行穿格），
 * 但**"相邻走子（非铁路）恒放行 vs 铁路滑行（须铁路格）vs 非相邻非铁路不可走"
 * 的走子路径分派边界 + 工兵/非工兵对照**未独立用例（§404 `if (adjacent) return
 * true` 相邻恒放行 + §398-400 铁路滑行分派）。
 *
 * 对照 64 位实现（rules.c move_legal §404 adjacent 恒放行 + §398-400 铁路滑行
 * 分派：工兵 rail_path_engineer / 非工兵 rail_path_straight，§131 !is_rail→
 * continue 滑行目标须铁路格）：
 *   · 一步相邻格（`adjacent`）：恒放行（所有子，含工兵，非铁路格可走）；
 *   · 非相邻铁路格：工兵 rail_path_engineer（BFS 拐弯）/ 非工兵 rail_path_straight
 *     （直走）；
 *   · 非相邻非铁路格：不可走（rail_path_* 只对铁路格 BFS/直走，非铁路远格返 false）。
 * 64 位行为级实证（走已实现的 move_legal 路径分派，0 分叉）。
 *
 * 几何口径：(4,0) 非铁路非行营格（row4col0），(3,0) 相邻非铁路，(5,0)/(5,4)
 *   row5 横铁路，col2 纵贯线。
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
    /* ① 工兵相邻走子（非铁路格恒放行，§404 adjacent） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,0)].rank = RANK_WORKER;
        st.cells[cell_index(4,0)].side = SIDE_BLUE; st.cells[cell_index(4,0)].revealed = 1;
        ok &= chk("① 工兵相邻走子(4,0)→(3,0)非铁路=1(adjacent恒放行)",
                  jq_can_move(&st, cell_index(4,0), cell_index(3,0)), 1);
    }
    /* ② 工兵非相邻非铁路不可走（rail_path_engineer 只对铁路 BFS，非铁路远格 false） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,0)].rank = RANK_WORKER;
        st.cells[cell_index(4,0)].side = SIDE_BLUE; st.cells[cell_index(4,0)].revealed = 1;
        ok &= chk("② 工兵非相邻非铁路(4,0)→(0,0)=0(非铁路不滑行)",
                  jq_can_move(&st, cell_index(4,0), cell_index(0,0)), 0);
    }
    /* ③ 工兵铁路滑行（(5,0)→(5,4) row5 横铁路，对照 dark_bfs 穿格场景） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(5,0)].rank = RANK_WORKER;
        st.cells[cell_index(5,0)].side = SIDE_BLUE; st.cells[cell_index(5,0)].revealed = 1;
        ok &= chk("③ 工兵铁路滑行(5,0)→(5,4)=1(row5横铁路)",
                  jq_can_move(&st, cell_index(5,0), cell_index(5,4)), 1);
    }
    /* ④ 非工兵相邻走子（非铁路格恒放行，§404 adjacent，对照工兵） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,0)].rank = 5;
        st.cells[cell_index(4,0)].side = SIDE_BLUE; st.cells[cell_index(4,0)].revealed = 1;
        ok &= chk("④ 非工兵相邻走子(4,0)→(3,0)非铁路=1(adjacent恒放行)",
                  jq_can_move(&st, cell_index(4,0), cell_index(3,0)), 1);
    }
    /* ⑤ 非工兵非相邻非铁路不可走（rail_path_straight 只对铁路直走，非铁路远格 false） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,0)].rank = 5;
        st.cells[cell_index(4,0)].side = SIDE_BLUE; st.cells[cell_index(4,0)].revealed = 1;
        ok &= chk("⑤ 非工兵非相邻非铁路(4,0)→(0,0)=0(非铁路不直走)",
                  jq_can_move(&st, cell_index(4,0), cell_index(0,0)), 0);
    }
    printf("dark_rail_walk_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
