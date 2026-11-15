/* dark_rail_c2_fullseg_test.c — 纵贯线全高 col2"纵贯线内同段直走 vs 跨段隔断
 * 阻断"工兵/非工兵全场景对照 + col0/1/4 非全高纵贯线对照行为级实证
 * 补全现有缺口：§92 dark_rail_topology_matrix_test（实锤 is_rail 全格分布"横
 * 贯线全宽 vs 纵贯线非全高"完整拓扑矩阵，纵贯线全高仅 col2 10 格 + col0/1/3/
 * 4 非全高 4/2/2/4 格 + row3/row8 全非铁路行营隔断），§93 实锤 row5/6 全宽横
 * 贯线"横贯线内全宽滑行"工兵/非工兵全场景对照 + row0/11 非全宽横贯线对照，但
 * **纵贯线全高 col2"纵贯线内同段直走 vs 跨段隔断阻断"全场景对照 + col0/1/4 非
 * 全高纵贯线跨非铁路阻断对照**无独立用例——§92 实锤纵贯线全高仅 col2（10 格
 * 铁路 row0/1/2/4/5/6/7/9/10/11，row3/8 行营隔断非铁路），但 col2 纵贯全高 10
 * 格被 row3/8 行营隔断成 3 段（row0-2/row4-7/row9-11），"纵贯线内同段直走可达
 * （(4,2)→(7,2) / (9,2)→(11,2) / (0,2)→(2,2) 同段直走 can=1）vs 跨段（row3/8
 * 隔断）不可达（(4,2)→(0,2) / (7,2)→(9,2) / (2,2)→(4,2) 跨 row3/8 隔断 can=0
 * ）"全场景对照 + col0/1/4 非全高纵贯线跨非铁路阻断对照未独立断言。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83/§84/§85/§87/§88/§8
 * 9/§90/§91/§92/§93 全格分布实锤：col2 纵贯 10 格 + row5/6 横贯 10 格全宽 +
 * row0/11 横贯非全宽仅 3 格铁路 col0/2/4 + col0=4 格 / col1=2 格 / col3=2 格
 *  / col4=4 格非纵贯全高 + row3/row8 全非铁路行营隔断）+ rules.c rail_path_st
 * raight 非工兵直走（col2 纵贯线内同段直走（row0-2/row4-7/row9-11 段内）放行
 * ，跨段（row3/8 隔断）阻断，col0/1/4 非全高纵贯线跨非铁路阻断）+ rail_path_e
 * ngineer 工兵 BFS 拐弯路径引擎（col2 纵贯线内同段拐弯可达，跨段 row3/8 隔断
 * 阻断，col0/1/4 端拐弯到 col2 纵贯全高线受限））：
 *   · col2 纵贯全高 10 格被 row3/8 行营隔断成 3 段（row0-2/row4-7/row9-11），
 *     "纵贯线全高" ≠ "纵贯线内全高直走可达"（row3/8 隔断阻断跨段直走）；
 *   · 同段直走可达：col2 row0-2 段 (0,2)→(2,2) / row4-7 段 (4,2)→(7,2) / row9
 *     -11 段 (9,2)→(11,2) 同段直走 can=1（rail_path_straight 非工兵同段直走
 *     放行）；
 *   · 跨段隔断阻断：col2 (4,2)→(0,2) 跨 row3 / (7,2)→(9,2) 跨 row8 / (2,2)→
 *     (4,2) 跨 row3 均 can=0（row3/8 行营隔断 col2 纵贯跨段阻断，rail_path_st
 *     raight 非工兵跨隔断拒）；
 *   · 对照 col0/1/4 非全高纵贯线跨非铁路阻断：col0 (0,0)→(5,0) / col4 (0,4)→
 *     (5,4) 跨 row1/2/3/4 非铁路 can=0（col0/4 仅 row0/5/6/11 = 4 格，col1 仅
 *     row5/6 = 2 格，非纵贯全高跨非铁路阻断）。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + rail_path_straight 非工兵直
 * 走 + rail_path_engineer 工兵 BFS，0 分叉；几何走 §92 既有 col2 纵贯全高 10
 * 格被 row3/8 隔断成 3 段 + 同段直走 vs 跨段隔断口径 + col0/1/4 非全高纵贯线
 * 对照）。
 *
 * 几何口径：col2 纵贯全高 10 格（row0/1/2/4/5/6/7/9/10/11，row3/8 行营隔断成
 *   3 段 row0-2/row4-7/row9-11）；同段直走 (0,2)→(2,2) / (4,2)→(7,2) / (9,2)
 *   →(11,2)（can=1）；跨段隔断 (4,2)→(0,2) / (7,2)→(9,2) / (2,2)→(4,2)（can
 *   =0）；col0 (0,0)→(5,0) / col4 (0,4)→(5,4) 跨非铁路（can=0，col0/4 仅 row0
 *   /5/6/11 = 4 格非全高）；红 cell(10,0) 防困毙；mode=LAY 明棋；非工兵 rank
 *   5 + 工兵 RANK_WORKER。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C02 cell_index(0,2)    /* col2 row0（纵贯线全高 row0-2 段起点） */
#define C22 cell_index(2,2)    /* col2 row2（纵贯线全高 row0-2 段远端） */
#define C42 cell_index(4,2)    /* col2 row4（纵贯线全高 row4-7 段起点） */
#define C72 cell_index(7,2)    /* col2 row7（纵贯线全高 row4-7 段远端） */
#define C92 cell_index(9,2)    /* col2 row9（纵贯线全高 row9-11 段起点） */
#define C112 cell_index(11,2)  /* col2 row11（纵贯线全高 row9-11 段远端） */
#define C00 cell_index(0,0)    /* col0 row0（非全高纵贯线起点） */
#define C50 cell_index(5,0)    /* col0 row5（非全高纵贯线远端） */
#define C04 cell_index(0,4)    /* col4 row0（非全高纵贯线起点） */
#define C54 cell_index(5,4)    /* col4 row5（非全高纵贯线远端） */
#define C100 cell_index(10,0)  /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① col2 纵贯线内同段直走可达（row4-7 段 (4,2)→(7,2) 同段直走 can=1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = 5; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C42, C72);   /* col2 row4-7 段同段直走 */
        ok &= chk("① col2纵贯线内同段直走可达(非工兵(4,2)→(7,2)col2纵贯线row4-7段同段直走can=1,rail_path_straight同段放行)",
                  can == 1);
    }
    /* ② col2 纵贯线跨段隔断阻断（row4-7 段 (4,2)→(0,2) 跨 row3 行营隔断 can=0） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = 5; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C42, C02);   /* col2 row4-7 段跨 row3 隔断到 row0-2 段 */
        ok &= chk("② col2纵贯线跨段隔断阻断(非工兵(4,2)→(0,2)跨row3行营隔断can=0,col2纵贯全高10格被row3/8隔断成3段跨段阻断)",
                  can == 0);
    }
    /* ③ 对照 col2 纵贯线其他段同段直走（row0-2 段 (0,2)→(2,2) + row9-11 段
       (9,2)→(11,2) 同段直走 can=1） */
    {
        /* row0-2 段同段直走 */
        jq_state st1; st1.mode = JQ_MODE_LAY;
        jq_state_init(&st1); memset(&st1.cells, 0, sizeof(st1.cells));
        st1.stage = STG_PLAY; st1.winner = JQ_WIN_NONE; st1.turn = SIDE_BLUE;
        st1.cells[C02].rank = 5; st1.cells[C02].side = SIDE_BLUE; st1.cells[C02].revealed = 1;
        st1.cells[C100].rank = 1; st1.cells[C100].side = SIDE_RED; st1.cells[C100].revealed = 1;
        int r02 = jq_can_move(&st1, C02, C22);   /* row0-2 段 (0,2)→(2,2) 同段 */
        /* row9-11 段同段直走 */
        jq_state st2; st2.mode = JQ_MODE_LAY;
        jq_state_init(&st2); memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[C92].rank = 5; st2.cells[C92].side = SIDE_BLUE; st2.cells[C92].revealed = 1;
        st2.cells[C100].rank = 1; st2.cells[C100].side = SIDE_RED; st2.cells[C100].revealed = 1;
        int r911 = jq_can_move(&st2, C92, C112);  /* row9-11 段 (9,2)→(11,2) 同段 */
        ok &= chk("③ 对照col2纵贯线其他段同段直走(row0-2段(0,2)→(2,2)=1+row9-11段(9,2)→(11,2)=1同段直走can=1)",
                  r02 == 1 && r911 == 1);
    }
    /* ④ col2 纵贯线跨 row8 隔断阻断（row4-7 段 (7,2)→(9,2) 跨 row8 行营隔断
       can=0） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C72].rank = 5; st.cells[C72].side = SIDE_BLUE; st.cells[C72].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C72, C92);   /* col2 row4-7 段跨 row8 隔断到 row9-11 段 */
        ok &= chk("④ col2纵贯线跨row8隔断阻断(非工兵(7,2)→(9,2)跨row8行营隔断can=0,col2纵贯全高10格被row3/8隔断成3段跨row8阻断)",
                  can == 0);
    }
    /* ⑤ 对照 col0 非全高纵贯线跨非铁路阻断（col0 (0,0)→(5,0) 跨 row1/2/3/4
       非铁路 can=0，col0 仅 row0/5/6/11 = 4 格非纵贯全高） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C00].rank = 5; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C00, C50);   /* col0 跨 row1/2/3/4 非铁路 */
        ok &= chk("⑤ 对照col0非全高纵贯线跨非铁路阻断(非工兵(0,0)→(5,0)跨row1/2/3/4非铁路can=0,col0仅row0/5/6/11=4格非纵贯全高)",
                  can == 0);
    }
    /* ⑥ 对照 col4 非全高纵贯线跨非铁路阻断（col4 (0,4)→(5,4) 跨 row1/2/3/4
       非铁路 can=0，col4 仅 row0/5/6/11 = 4 格非纵贯全高） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C04].rank = 5; st.cells[C04].side = SIDE_BLUE; st.cells[C04].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C04, C54);   /* col4 跨 row1/2/3/4 非铁路 */
        ok &= chk("⑥ 对照col4非全高纵贯线跨非铁路阻断(非工兵(0,4)→(5,4)跨row1/2/3/4非铁路can=0,col4仅row0/5/6/11=4格非纵贯全高)",
                  can == 0);
    }
    /* ⑦ col2 纵贯线全高 10 格被 row3/8 隔断成 3 段完整拓扑实锤（row0-2/row4-7
       /row9-11 同段直走放行 + 跨段 row3/8 隔断阻断） */
    {
        int c2_cnt = 0;
        for (int r = 0; r < 12; r++)
            if (is_rail(r, 2)) c2_cnt++;
        /* col2 纵贯全高 10 格（row3/8 行营隔断非铁路） */
        int c2_0 = !is_rail(3, 2), c2_8 = !is_rail(8, 2);
        ok &= chk("⑦ col2纵贯线全高10格被row3/8隔断成3段完整拓扑(col2=row0/1/2/4/5/6/7/9/10/11共10格铁路row3/8行营隔断非铁路)",
                  c2_cnt == 10 && c2_0 && c2_8);
    }
    printf("dark_rail_c2_fullseg_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
