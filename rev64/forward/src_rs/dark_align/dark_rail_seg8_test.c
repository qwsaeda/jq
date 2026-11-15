/* dark_rail_seg8_test.c — col2 纵贯线跨 row8 隔断（红方视角 row4-7→row9-11）
 * + 同段放行 + 工兵跨隔断绕行不可达行为级实证
 * 补全现有缺口：§80 dark_rail_segment_test 实锤 col2 纵贯线跨 row3 隔断（蓝方
 * 视角 row0-2→row4-7）+ row11 红方底线对称 + 工兵跨 row3 隔断绕行不可达，但
 * **col2 纵贯线跨 row8 隔断（红方视角 row4-7→row9-11）**（对称 row3 隔断，红
 * 方跨段阻断 + 同段放行 + 工兵跨隔断绕行不可达）无独立用例——col2 纵贯线"三
 * 段隔断"（row0-2/4-7/9-11）在 row8 隔断（红方视角）的跨段阻断 + 同段放行未独
 * 立断言（§80 实锤 row3 隔断蓝方视角，row8 隔断红方视角对称未覆盖）。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 纵向 col2 纵贯全高）+
 * is_camp 行营隔断（row3/row8 col2 隔断成三段 §43 既有）+ rules.c rail_path_
 * straight 非工兵直走（同段放行 / 跨隔断拒）+ rail_path_engineer 工兵 BFS（跨
 * 隔断绕行不可达 §76/§80 既有口径））：
 *   · 非工兵 col2 跨 row8 隔断（(7,2)→(9,2) row4-7 段→row9-11 段）：col2 被行
 *     营 row8 隔断，跨段直走拒（can=0，对称 §80 row3 隔断蓝方视角跨段拒）；
 *   · 非工兵 col2 同段 row9-11 放行（(9,2)→(11,2)）：col2 row9-11 段全铁路无隔
 *     断，同段直走放行（can=1，对照跨段拒）；
 *   · 工兵 BFS 跨 row8 隔断绕行不可达（(7,2)→(9,2)）：col2 被行营 row8 隔断 +
 *     行营隔断双重阻断，工兵拐弯绕行不可达（can=0，§80 工兵跨 row3 隔断绕行不
 *     可达对称）；
 *   · 工兵 col2 同段 row9-11 放行（(9,2)→(11,2)）：col2 row9-11 段全铁路，工兵
 *     同段滑行放行（can=1，对照跨段绕行不可达）。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + is_camp 行营隔断 + rail_path_
 * straight/rail_path_engineer 判定，0 分叉；几何走 §43/§80 既有 col2 纵贯线 +
 * row8 行营隔断口径 + 红方视角 (7,2)/(9,2)/(11,2)）。
 *
 * 几何口径：col2 纵贯线 (7,2)↔(9,2)（跨 row8 隔断）/ (9,2)↔(11,2)（同段
 *   row9-11）；蓝 cell(0,0) 防困毙；mode=LAY 明棋；非工兵 rank5 + 工兵
 *   RANK_WORKER（红方视角）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C72 cell_index(7,2)   /* col2 纵贯线 row7（row4-7 段远端） */
#define C92 cell_index(9,2)   /* col2 纵贯线 row9（row9-11 段起点） */
#define C112 cell_index(11,2) /* col2 纵贯线 row11（row9-11 段远端） */
#define C00 cell_index(0,0)   /* 蓝防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 非工兵 col2 跨 row8 隔断（(7,2)→(9,2) 跨段直走拒，对称 §80 row3 隔断） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C72].rank = 5; st.cells[C72].side = SIDE_RED; st.cells[C72].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C72, C92);
        ok &= chk("① 非工兵col2跨row8隔断((7,2)→(9,2)跨段直走拒can=0对称§80row3)", can == 0);
    }
    /* ② 非工兵 col2 同段 row9-11 放行（(9,2)→(11,2) 对照 ① 跨段拒） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C92].rank = 5; st.cells[C92].side = SIDE_RED; st.cells[C92].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C92, C112);
        ok &= chk("② 非工兵col2同段row9-11放行((9,2)→(11,2)can=1对照①跨段拒)",
                  can == 1);
    }
    /* ③ 工兵 BFS 跨 row8 隔断绕行不可达（(7,2)→(9,2)，对称 §80 工兵跨 row3） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C72].rank = RANK_WORKER; st.cells[C72].side = SIDE_RED; st.cells[C72].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C72, C92);
        ok &= chk("③ 工兵BFS跨row8隔断绕行不可达((7,2)→(9,2)can=0对称§80工兵跨row3)",
                  can == 0);
    }
    /* ④ 工兵 col2 同段 row9-11 放行（(9,2)→(11,2) 对照 ③ 跨段绕行不可达） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C92].rank = RANK_WORKER; st.cells[C92].side = SIDE_RED; st.cells[C92].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C92, C112);
        ok &= chk("④ 工兵col2同段row9-11放行((9,2)→(11,2)can=1对照③跨段绕行不可达)",
                  can == 1);
    }
    printf("dark_rail_seg8_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
