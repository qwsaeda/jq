/* dark_rail_segment_test.c — col2 纵贯线跨行营隔断（row3/8）+ row11 红方底线对称
 * + 工兵跨隔断行为级实证
 * 补全现有缺口：§43 dark_rail_topo_test 实锤 col2 行营隔断三段 + 工兵/非工兵滑行
 * 基础拓扑，§76 dark_bfs_around_test 实锤工兵 BFS 穿己方子（直线阻断 + 绕行不可
 * 达），§77 dark_straight_rail_test 实锤非工兵直走 + 铁路横贯线完整拓扑，§79
 * dark_cross_rail_test 实锤中间交叉格阻断，但**col2 纵贯线跨行营隔断（row3/8）
 * 的独立跨段阻断 + row11 红方底线对称 + 工兵跨 row3 隔断绕行不可达**未独立用例
 * ——col2 纵贯线"三段隔断"在跨段（非工兵拒 / 工兵绕行不可达）+ 红方 row11 底线
 * 对称（两端非铁路阻断）的完整判定未独立断言。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 纵向 col2 纵贯全高 + 横向
 * row0/11 底线 + row5/6 中线）+ is_camp 行营隔断（row3/row8 col2 隔断 col2 纵
 * 贯线成三段 row0-2/4-7/9-11，§43 既有）+ rules.c rail_path_straight 非工兵直
 * 走（同段放行 / 跨隔断拒）+ rail_path_engineer 工兵 BFS（跨隔断绕行不可达，§76
 * 既有口径））：
 *   · 非工兵 col2 跨 row3 隔断（(2,2)→(4,2)）：col2 纵贯线被行营 row3 隔断成三
 *     段，(2,2) 在 row0-2 段、(4,2) 在 row4-7 段，**跨段直走拒**（can=0，同段
 *     row4-7 (4,2)→(7,2) 放行 can=1 对照）；
 *   · 非工兵 row11 红方底线横贯线（(11,0)→(11,4)）：row11 两端 col0/col4 非铁
 *     路（底角 + 边格，§77 既有 row0 两端非铁路对称）→ 横贯线滑行阻断（can=0，
 *     对称 §77 ④ row0 两端非铁路）；
 *   · 工兵 BFS 跨 row3 隔断（(2,2)→(4,2)）：col2 被行营隔断，工兵拐弯绕行不可
 *     达（can=0，§76 工兵"绕行不可达"口径——col2 隔断 + 行营隔断双重阻断）；
 *   · 非工兵 col2 同段放行（(4,2)→(7,2) row4-7 段）：col2 row4-7 段全铁路无隔
 *     断，同段直走放行（can=1，§77/§79 既有同段滑行对称）。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + is_camp 行营隔断 + rail_path_
 * straight/rail_path_engineer 判定，0 分叉；几何走 §43/§76/§77 既有 col2 纵贯线
 * + row11 红方底线 + 行营隔断口径）。
 *
 * 几何口径：col2 纵贯线 (2,2)↔(4,2)（跨 row3 隔断）/ (4,2)↔(7,2)（同段 row4-7）；
 *   row11 红方底线 (11,0)↔(11,4)（横贯线两端非铁路）；红 cell(0,0) / 蓝
 *   cell(10,0) 防困毙；mode=LAY 明棋；非工兵 rank5 + 工兵 RANK_WORKER。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C22 cell_index(2,2)   /* col2 纵贯线 row2（row0-2 段） */
#define C42 cell_index(4,2)   /* col2 纵贯线 row4（row4-7 段） */
#define C72 cell_index(7,2)   /* col2 纵贯线 row7（row4-7 段远端） */
#define C110 cell_index(11,0) /* row11 红方底线起点 */
#define C114 cell_index(11,4) /* row11 红方底线远端 */
#define C100 cell_index(10,0) /* 蓝防困毙 */
#define C00 cell_index(0,0)   /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 非工兵 col2 跨 row3 隔断（(2,2)→(4,2) 跨段拒） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C22].rank = 5; st.cells[C22].side = SIDE_BLUE; st.cells[C22].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C22, C42);
        ok &= chk("① 非工兵col2跨row3隔断((2,2)→(4,2)跨段直走拒can=0)", can == 0);
    }
    /* ② 非工兵 col2 同段 row4-7 放行（(4,2)→(7,2) 对照 ① 跨段拒） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = 5; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C42, C72);
        ok &= chk("② 非工兵col2同段row4-7放行((4,2)→(7,2)can=1对照①跨段拒)",
                  can == 1);
    }
    /* ③ 非工兵 row11 红方底线横贯线阻断（(11,0)→(11,4) 两端非铁路，对称 §77
       row0） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C110].rank = 5; st.cells[C110].side = SIDE_RED; st.cells[C110].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C110, C114);
        ok &= chk("③ 非工兵row11红方底线横贯线阻断((11,0)→(11,4)两端非铁路can=0对称§77row0)",
                  can == 0);
    }
    /* ④ 工兵 BFS 跨 row3 隔断绕行不可达（(2,2)→(4,2) col2 隔断 + 行营隔断双重
       阻断，§76 工兵"绕行不可达"口径） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C22].rank = RANK_WORKER; st.cells[C22].side = SIDE_BLUE; st.cells[C22].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C22, C42);
        ok &= chk("④ 工兵BFS跨row3隔断绕行不可达((2,2)→(4,2)can=0§76绕行不可达口径)",
                  can == 0);
    }
    /* ⑤ 工兵 col2 同段 row4-7 放行（(4,2)→(7,2) 工兵同段滑行，对照 ④ 跨段拒） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = RANK_WORKER; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C42, C72);
        ok &= chk("⑤ 工兵col2同段row4-7放行((4,2)→(7,2)can=1对照④跨段拒)",
                  can == 1);
    }
    printf("dark_rail_segment_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
