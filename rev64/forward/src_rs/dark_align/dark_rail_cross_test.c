/* dark_rail_cross_test.c — 工兵 BFS 跨行营隔断段可达性行为级实证
 * 补全现有缺口：§40 dark_rail_topo_test 覆盖 col2 行营隔断拓扑（三段
 * row0-2/row4-7/row9-11），§45 dark_rail_walk_test 覆盖走子路径分派，
 * 但**工兵 BFS 拐弯跨行营隔断段可达性**（同段内拐弯可 / 跨隔断段不可）
 * 未独立用例（§33 AUD-R-023 行营隔断 col2 三段，工兵 BFS 跨隔断段
 * `!is_rail→continue` 阻断首次行为级实锤）。
 *
 * 对照 64 位实现（rules.c rail_path_engineer BFS + board.c is_rail 行营隔断）：
 *   · col2 纵线被行营 (3,2)/(8,2) 隔断成三段（is_rail(3,2)=is_rail(8,2)=0）；
 *   · 工兵 BFS 同段内拐弯可达（段B row4-7col2 内 (4,2)→(7,2) 直走 + 经
 *     row5/6 横线拐弯）；
 *   · 工兵 BFS 跨隔断段不可达（段B→段C 经 row8 行营，(8,2) is_rail=0 阻断，
 *     BFS `!is_rail→continue` 不扩展跨隔断节点）；
 *   · 非工兵直走不跨隔断（rail_path_straight 同段）。
 * 64 位行为级实证（走已实现的 rail_path_engineer + is_rail 隔断路径，0 分叉）。
 *
 * 几何口径：col2 纵线三段（段A row0-2 / 段B row4-7 / 段C row9-11），行营
 *   隔断点 (3,2)/(8,2)；工兵在 (4,2)（段B），目标 (7,2)（同段B）/ (9,2)（段C
 *   跨隔断）/ (11,2)（段C 跨隔断）。
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
    /* ① 拓扑复证：col2 三段 + 行营隔断点（对照 §40，本用例跨段前提） */
    ok &= chk("① col2 段B is_rail(4,2)=1", is_rail(4,2), 1);
    ok &= chk("① 行营隔断点 is_rail(3,2)=0", is_rail(3,2), 0);
    ok &= chk("① 行营隔断点 is_rail(8,2)=0", is_rail(8,2), 0);
    ok &= chk("① col2 段C is_rail(9,2)=1", is_rail(9,2), 1);
    /* ② 工兵同段B 直走可达：(4,2)→(7,2)（段B row4-7col2 内） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,2)].rank = RANK_WORKER;
        st.cells[cell_index(4,2)].side = SIDE_BLUE; st.cells[cell_index(4,2)].revealed = 1;
        ok &= chk("② 工兵同段B直走(4,2)→(7,2)=1",
                  jq_can_move(&st, cell_index(4,2), cell_index(7,2)), 1);
    }
    /* ③ 工兵跨隔断段B→段C 不可达：(4,2)→(9,2)（经 row8 行营隔断） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,2)].rank = RANK_WORKER;
        st.cells[cell_index(4,2)].side = SIDE_BLUE; st.cells[cell_index(4,2)].revealed = 1;
        ok &= chk("③ 工兵跨隔断段B→段C(4,2)→(9,2)=0(row8行营阻断)",
                  jq_can_move(&st, cell_index(4,2), cell_index(9,2)), 0);
    }
    /* ④ 工兵跨隔断到段C底线 (4,2)→(11,2) 不可达 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,2)].rank = RANK_WORKER;
        st.cells[cell_index(4,2)].side = SIDE_BLUE; st.cells[cell_index(4,2)].revealed = 1;
        ok &= chk("④ 工兵跨隔断(4,2)→(11,2)段C底线=0",
                  jq_can_move(&st, cell_index(4,2), cell_index(11,2)), 0);
    }
    /* ⑤ 非工兵对照：跨隔断直走不可（rail_path_straight 同段，对照工兵） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,2)].rank = 5;
        st.cells[cell_index(4,2)].side = SIDE_BLUE; st.cells[cell_index(4,2)].revealed = 1;
        ok &= chk("⑤ 非工兵跨隔断直走(4,2)→(9,2)=0(对照工兵,不跨隔断)",
                  jq_can_move(&st, cell_index(4,2), cell_index(9,2)), 0);
    }
    /* ⑥ 工兵同段内拐弯可达（段B 内经 row5/6 横线拐弯，(4,2)→(5,4) 对照 §45） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,2)].rank = RANK_WORKER;
        st.cells[cell_index(4,2)].side = SIDE_BLUE; st.cells[cell_index(4,2)].revealed = 1;
        ok &= chk("⑥ 工兵同段内拐弯(4,2)→(5,4)=1(row5横线拐弯,段B内)",
                  jq_can_move(&st, cell_index(4,2), cell_index(5,4)), 1);
    }
    printf("dark_rail_cross_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
