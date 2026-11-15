/* dark_rail_corner_nonrail_test.c — row0/row11 底角工兵拐弯 + col1/col4 非纵贯全高
 * 铁路线纵向跨段 can=0 行为级实证
 * 补全现有缺口：§83 dark_wrap_across_test 实锤工兵拐弯绕行"row5/6 中线无隔断可
 * 达 vs col2 隔断不可达"全场景对照 + is_rail 全格分布（col2 纵贯 10 格 + row5/6
 * 横贯 10 格），但**row0/row11 底角工兵拐弯场景 + col1/col4 非纵贯全高铁路线纵
 * 向跨段 can=0**无独立用例——is_rail 全格分布实锤（§83 ⑤）揭示 col1/col4 非纵贯
 * 全高（col1 仅 row5 铁路 / col4 仅 row0/5 铁路），底角 col0/col4 非铁路纵贯线，
 * 工兵底角拐弯受限 + 非工兵 col1/col4 纵向跨段不可滑行未独立断言。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83 全格分布实锤：col2 纵
 * 贯 10 格 + row5/6 横贯 10 格 + row0/11 横贯两端）+ rules.c rail_path_straight
 * 非工兵直走（非纵贯全高线跨段拒）+ rail_path_engineer 工兵 BFS（底角非铁路拐
 * 弯受限））：
 *   · row0 底角工兵拐弯受限（col0 非铁路纵贯线）：工兵 @(0,0) 可达格仅 (1,0)
 *     （col0 下行），**不能拐弯到 col2**（col0 非铁路，(0,0)→(0,2) 横向 + 下行
 *     拐弯受限）——底角 col0 非纵贯全高铁路线，工兵拐弯受限；
 *   · col1 非纵贯全高（is_rail col1 仅 row5）：非工兵 (0,1)→(9,1) 纵向跨段
 *     can=0（col1 非纵贯全高铁路线，跨段不可滑行）；
 *   · col4 非纵贯全高（is_rail col4 仅 row0/5）：非工兵 (0,4)→(9,4) 纵向跨段
 *     can=0（col4 非纵贯全高铁路线，跨段不可滑行）；
 *   · 对照 col2 纵贯全高（§83 既有）：col2 (0,2)→(9,2) 跨隔断/同段（col2 唯一
 *     纵贯全高铁路线）——col1/col4 与 col2 的纵贯全高性差异实锤（col2 纵贯 10
 *     格 vs col1/col4 仅部分铁路格）。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + rail_path_straight 非工兵直走
 * + rail_path_engineer 工兵 BFS，0 分叉；几何走 §83 is_rail 全格分布口径 + 底
 * 角/col1/col4 非纵贯全高线）。
 *
 * 几何口径：row0 底角 (0,0)（col0 非铁路纵贯线）/ col1 (0,1)→(9,1) / col4
 *   (0,4)→(9,4)（非纵贯全高铁路线）；对照 col2 (0,2) 纵贯全高；红 cell(10,0)
 *   防困毙；mode=LAY 明棋；非工兵 rank5 + 工兵 RANK_WORKER。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C00 cell_index(0,0)   /* row0 底角（col0 非铁路纵贯线） */
#define C10 cell_index(0,1)   /* col1 起点（非纵贯全高） */
#define C91 cell_index(9,1)   /* col1 远端 */
#define C04 cell_index(0,4)   /* col4 起点（非纵贯全高） */
#define C94 cell_index(9,4)   /* col4 远端 */
#define C02 cell_index(0,2)   /* col2 起点（纵贯全高，对照） */
#define C92 cell_index(9,2)   /* col2 远端 */
#define C100 cell_index(10,0) /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① row0 底角工兵拐弯受限：工兵 @(0,0) 不能拐弯到 col2（col0 非铁路纵贯
       线，可达格仅 col0 下行 (1,0)） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C00].rank = RANK_WORKER; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        /* 底角工兵到 col2 (9,2) 拐弯受限（col0 非铁路纵贯线，col0 下行 (1,0) 非
           铁路，不能拐到 col2） */
        int can_c2 = jq_can_move(&st, C00, C92);
        int can_c0down = jq_can_move(&st, C00, cell_index(1, 0));   /* col0 下行应可 */
        ok &= chk("① row0底角工兵拐弯受限(工兵(0,0)不能拐到col2(9,2)can=0,col0下行(1,0)can=1)",
                  can_c2 == 0 && can_c0down == 1);
    }
    /* ② col1 非纵贯全高：非工兵 (0,1)→(9,1) 纵向跨段 can=0（col1 仅 row5 铁路） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C10].rank = 5; st.cells[C10].side = SIDE_BLUE; st.cells[C10].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C10, C91);
        ok &= chk("② col1非纵贯全高(非工兵(0,1)→(9,1)纵向跨段can=0col1仅row5铁路)", can == 0);
    }
    /* ③ col4 非纵贯全高：非工兵 (0,4)→(9,4) 纵向跨段 can=0（col4 仅 row0/5 铁路） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C04].rank = 5; st.cells[C04].side = SIDE_BLUE; st.cells[C04].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C04, C94);
        ok &= chk("③ col4非纵贯全高(非工兵(0,4)→(9,4)纵向跨段can=0col4仅row0/5铁路)",
                  can == 0);
    }
    /* ④ 对照 col2 纵贯全高（§83 既有）：col2 同段放行 vs col1/col4 跨段拒 */
    {
        /* col2 同段 row4-7（§80 既有几何 (4,2)→(7,2) 放行） */
        jq_state st2; jq_state_init(&st2); st2.mode = JQ_MODE_LAY;
        memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[cell_index(4,2)].rank = 5; st2.cells[cell_index(4,2)].side = SIDE_BLUE; st2.cells[cell_index(4,2)].revealed = 1;
        st2.cells[C100].rank = 1; st2.cells[C100].side = SIDE_RED;  st2.cells[C100].revealed = 1;
        int c2_ok = jq_can_move(&st2, cell_index(4,2), cell_index(7,2)) == 1;
        /* col1 同段？col1 非纵贯全高，col1 无"同段"（仅 row5 一格铁路） */
        jq_state st3; jq_state_init(&st3); st3.mode = JQ_MODE_LAY;
        memset(&st3.cells, 0, sizeof(st3.cells));
        st3.stage = STG_PLAY; st3.winner = JQ_WIN_NONE; st3.turn = SIDE_BLUE;
        st3.cells[cell_index(4,1)].rank = 5; st3.cells[cell_index(4,1)].side = SIDE_BLUE; st3.cells[cell_index(4,1)].revealed = 1;
        st3.cells[C100].rank = 1; st3.cells[C100].side = SIDE_RED;  st3.cells[C100].revealed = 1;
        int c1_cross = jq_can_move(&st3, cell_index(4,1), cell_index(6,1));  /* col1 row4→row6 跨 row5 隔断 */
        ok &= chk("④ 对照col2纵贯全高同段放行(col2(4,2)→(7,2)=1)vs col1跨段拒(col1(4,1)→(6,1)=0)",
                  c2_ok && c1_cross == 0);
    }
    /* ⑤ is_rail 纵贯全高性实锤（col2 纵贯 10 格 vs col1/col4 仅部分，§83 口径） */
    {
        int c2 = 0, c1 = 0, c4 = 0;
        for (int r = 0; r < 12; r++) {
            if (is_rail(r, 2)) c2++;
            if (is_rail(r, 1)) c1++;
            if (is_rail(r, 4)) c4++;
        }
        /* col2 纵贯 10 格（§83 实锤），col1 仅 row5/6 = 2 格，col4 仅 row0/5/6/11 = 4 格 */
        ok &= chk("⑤ is_rail纵贯全高性实锤(col2=10格纵贯全高vs col1=2格/col4=4格非纵贯全高)",
                  c2 == 10 && c1 == 2 && c4 == 4);
    }
    printf("dark_rail_corner_nonrail_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
