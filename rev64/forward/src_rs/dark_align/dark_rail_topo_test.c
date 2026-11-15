/* dark_rail_topo_test.c — 铁路（rail）拓扑 + 工兵/非工兵滑行 + 行营隔断行为级实证
 * 补全现有缺口：flip_rail_test / rail_block_test 覆盖"暗棋翻出子滑行 / 阻断 /
 * 工兵拐弯 / 非工兵直走"，但**铁路拓扑本身（col2 纵贯线被行营 row3/row8 隔断
 * 成三段）+ 非工兵直走范围（同段不可跨隔断）+ 工兵 BFS 跨段拐弯**未独立用例
 * 覆盖（§33 AUD-R-023 既有诚实标注"行营排除出铁路→中线 col2 被隔断，需人工
 * 确认"首次行为级实锤）。
 *
 * 对照 64 位实现（board.c is_rail + rules.c rail_path_engineer/rail_path_straight）：
 *   · 铁路拓扑 = col2 纵贯线（row0-2 / row4-7 / row9-11 三段）+ row0/5/6/11
 *     横贯（row3/8 行营隔断 col2，is_camp(3,2)=is_camp(8,2)=1）；
 *   · 非工兵 rail_path_straight：同段直走（(4,2)→(7,2) 同 row4-7 段），不可跨
 *     隔断（(4,2)→(9,2) 跨 row8 行营 = 0）；
 *   · 工兵 rail_path_engineer（BFS）：可拐弯跨段（(4,2)→(5,4) 纵→横拐 row5）。
 * 64 位行为级实证（走已实现的 is_rail + rail_path_* 路径，0 分叉）。
 *
 * 几何口径：col2 纵贯线 + row5/6 横贯（§board.c 拓扑唯一事实源）；行营
 * (3,2)/(8,2) 隔断点（§33 AUD-R-023）。
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
    /* ① 铁路拓扑：col2 纵贯线 row3/row8 被行营隔断（is_camp 隔断点） */
    ok &= chk("① col2 row3 被行营隔断(is_rail=0)", is_rail(3, 2), 0);
    ok &= chk("① col2 row8 被行营隔断(is_rail=0)", is_rail(8, 2), 0);
    ok &= chk("① 行营隔断点 is_camp(3,2)=1", is_camp(3, 2), 1);
    ok &= chk("① 行营隔断点 is_camp(8,2)=1", is_camp(8, 2), 1);
    ok &= chk("① col2 row4/7 同段铁路(is_rail=1)", is_rail(4, 2) && is_rail(7, 2), 1);
    ok &= chk("① 横向 row5/6 整行铁路", is_rail(5, 0) && is_rail(5, 4)
              && is_rail(6, 0) && is_rail(6, 4), 1);
    /* ② 非工兵直走：同段 (4,2)→(7,2) 可（row4-7 段），跨隔断 (4,2)→(9,2) 不可 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,2)].rank = 5;
        st.cells[cell_index(4,2)].side = SIDE_BLUE; st.cells[cell_index(4,2)].revealed = 1;
        ok &= chk("② 非工兵同段直走(4,2)→(7,2)=1",
                  jq_can_move(&st, cell_index(4,2), cell_index(7,2)), 1);
        ok &= chk("② 非工兵跨行营隔断(4,2)→(9,2)=0",
                  jq_can_move(&st, cell_index(4,2), cell_index(9,2)), 0);
    }
    /* ③ 工兵拐弯跨段：(4,2)→(5,4)（col2 纵→row5 横拐） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,2)].rank = RANK_WORKER;
        st.cells[cell_index(4,2)].side = SIDE_BLUE; st.cells[cell_index(4,2)].revealed = 1;
        ok &= chk("③ 工兵拐弯(4,2)→(5,4)=1",
                  jq_can_move(&st, cell_index(4,2), cell_index(5,4)), 1);
    }
    /* ④ 非工兵不可拐弯：同几何 (4,2)→(5,4)（rank5 非工兵） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,2)].rank = 5;
        st.cells[cell_index(4,2)].side = SIDE_BLUE; st.cells[cell_index(4,2)].revealed = 1;
        ok &= chk("④ 非工兵拐弯(4,2)→(5,4)=0",
                  jq_can_move(&st, cell_index(4,2), cell_index(5,4)), 0);
    }
    printf("dark_rail_topo_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
