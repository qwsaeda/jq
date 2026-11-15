/* dark_rail_c1c4cross_test.c — col1/col4 交叉格（row5/6 横贯 ∩ col1/col4）工兵
 * 拐弯场景行为级实证
 * 补全现有缺口：§83 dark_wrap_across_test 实锤工兵拐弯绕行"row5/6 中线无隔断
 * 可达 vs col2 隔断不可达"全场景对照 + is_rail 全格分布（col2 纵贯 10 格 +
 * row5/6 横贯 10 格 + col0=4 格 / col1=2 格 / col4=4 格非纵贯全高），§87/§88
 * 实锤 col4/col0 底角红蓝方 4 格全对称，但**col1/col4 交叉格（row5/6 横贯 ∩
 * col1/col4）工兵拐弯场景**无独立用例——col1 仅 row5/6 铁路 / col4 仅 row0/5/
 * 6/11 铁路（非纵贯全高），col1/col4 交叉格工兵横贯线内拐弯（row5 横贯 → col2
 * 纵贯 row4-7 段）可达 vs 跨段拐弯（col1/col4 → col2 纵贯 row9-11 段）受限
 * （col1/col4 非纵贯全高拐不到 col2 纵贯 row9-11 段）未独立断言。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83/§84/§85/§87/§88 全
 * 格分布实锤：col2 纵贯 10 格 + row5/6 横贯 10 格 + row0/11 横贯两端 + col0=4
 * 格 / col1=2 格 / col4=4 格非纵贯全高）+ rules.c rail_path_engineer 工兵 BFS
 * 拐弯路径引擎（col1/col4 交叉格横贯线内拐弯到 col2 纵贯 row4-7 段可达，跨段
 * 拐弯到 col2 纵贯 row9-11 段受限）+ rail_path_straight 非工兵直走（col1/col4
 * 非纵贯全高纵向跨段拒））：
 *   · col1 交叉格 (5,1) 工兵横贯线内拐弯可达（row5 横贯 → col2 纵贯 row4-7 段
 *     拐弯，(5,1)→(5,2) 同 row5 横贯 can=1）；
 *   · col1 交叉格 (5,1) 跨段拐弯受限（col1 非纵贯全高，(5,1)→(9,2) 拐不到 col2
 *     纵贯 row9-11 段 can=0）；
 *   · col4 交叉格 (5,4) 工兵横贯线内拐弯可达（row5 横贯 → col2 纵贯 row4-7 段
 *     拐弯，(5,4)→(5,2) 同 row5 横贯 can=1）；
 *   · col4 交叉格 (5,4) 跨段拐弯受限（col4 非纵贯全高，(5,4)→(9,2) 拐不到 col2
 *     纵贯 row9-11 段 can=0）；
 *   · 对照 col2 纵贯全高（§83 既有）：col2 (5,2) 同段 row4-7 放行 vs col1/col4
 *     交叉格跨段 row9-11 拒——col1/col4 非纵贯全高交叉格工兵拐弯"横贯线内拐弯
 *     可达 vs 跨段拐弯受限"差异实锤。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + rail_path_engineer 工兵 BFS，
 * 0 分叉；几何走 §83/§84/§87/§88 既有 col1/col4 非纵贯全高 + col2 纵贯全高 +
 * row5 横贯交叉格口径）。
 *
 * 几何口径：col1 交叉格 (5,1)（row5 横贯 ∩ col1，col1 仅 row5/6 铁路非纵贯全
 *   高）；col4 交叉格 (5,4)（row5 横贯 ∩ col4，col4 仅 row0/5/6/11 铁路非纵贯
 *   全高）；col2 纵贯 (5,2)（row5 横贯 ∩ col2 纵贯全高，§79 既有交叉格）；col2
 *   纵贯 row9 段 (9,2)（跨段拐弯目标，col1/col4 非纵贯全高拐不到）；红 cell
 *   (10,0) 防困毙；mode=LAY 明棋；工兵 RANK_WORKER。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C51 cell_index(5,1)    /* col1 交叉格 (row5 横贯 ∩ col1) */
#define C54 cell_index(5,4)    /* col4 交叉格 (row5 横贯 ∩ col4) */
#define C52 cell_index(5,2)    /* col2 交叉格 (row5 横贯 ∩ col2 纵贯全高) */
#define C92 cell_index(9,2)    /* col2 纵贯 row9 段（跨段拐弯目标） */
#define C41 cell_index(4,1)    /* col1 row4（col1 非纵贯全高，(4,1) 非铁路） */
#define C44 cell_index(4,4)    /* col4 row4（col4 非纵贯全高，(4,4) 非铁路） */
#define C100 cell_index(10,0)  /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① col1 交叉格 (5,1) 工兵横贯线内拐弯可达（row5 横贯 → col2 纵贯 row4-7
       段拐弯，(5,1)→(5,2) 同 row5 横贯 can=1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can_same = jq_can_move(&st, C51, C52);   /* 同 row5 横贯 (5,1)→(5,2) 放行 */
        ok &= chk("① col1交叉格工兵横贯线内拐弯可达(工兵(5,1)→col2(5,2)同row5横贯can=1,row5横贯→col2纵贯row4-7段拐弯)",
                  can_same == 1);
    }
    /* ② col1 交叉格 (5,1) 跨段拐弯受限（col1 非纵贯全高，(5,1)→(9,2) 拐不到
       col2 纵贯 row9-11 段 can=0） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can_cross = jq_can_move(&st, C51, C92);  /* 跨段 (5,1)→(9,2) 拐不到 */
        ok &= chk("② col1交叉格工兵跨段拐弯受限(工兵(5,1)→col2(9,2)拐不到col2纵贯row9-11段can=0,col1仅row5/6铁路非纵贯全高)",
                  can_cross == 0);
    }
    /* ③ col4 交叉格 (5,4) 工兵横贯线内拐弯可达（row5 横贯 → col2 纵贯 row4-7
       段拐弯，(5,4)→(5,2) 同 row5 横贯 can=1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C54].rank = RANK_WORKER; st.cells[C54].side = SIDE_BLUE; st.cells[C54].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can_same = jq_can_move(&st, C54, C52);   /* 同 row5 横贯 (5,4)→(5,2) 放行 */
        ok &= chk("③ col4交叉格工兵横贯线内拐弯可达(工兵(5,4)→col2(5,2)同row5横贯can=1,row5横贯→col2纵贯row4-7段拐弯)",
                  can_same == 1);
    }
    /* ④ col4 交叉格 (5,4) 跨段拐弯受限（col4 非纵贯全高，(5,4)→(9,2) 拐不到
       col2 纵贯 row9-11 段 can=0） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C54].rank = RANK_WORKER; st.cells[C54].side = SIDE_BLUE; st.cells[C54].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can_cross = jq_can_move(&st, C54, C92);  /* 跨段 (5,4)→(9,2) 拐不到 */
        ok &= chk("④ col4交叉格工兵跨段拐弯受限(工兵(5,4)→col2(9,2)拐不到col2纵贯row9-11段can=0,col4仅row0/5/6/11铁路非纵贯全高)",
                  can_cross == 0);
    }
    /* ⑤ 对照 col2 纵贯全高同段放行（§80 既有 (4,2)→(7,2) row4-7 段放行）vs
       col1/col4 交叉格跨段 row9-11 拒（②④） */
    {
        /* col2 同段 row4-7（§80 既有几何 (4,2)→(7,2) 放行） */
        jq_state st2; jq_state_init(&st2); st2.mode = JQ_MODE_LAY;
        memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[cell_index(4,2)].rank = 5; st2.cells[cell_index(4,2)].side = SIDE_BLUE; st2.cells[cell_index(4,2)].revealed = 1;
        st2.cells[C100].rank = 1; st2.cells[C100].side = SIDE_RED; st2.cells[C100].revealed = 1;
        int c2_ok = jq_can_move(&st2, cell_index(4,2), cell_index(7,2)) == 1;
        /* col1/col4 交叉格跨段 row9-11 拒（②④ 既有） */
        jq_state st3; jq_state_init(&st3); st3.mode = JQ_MODE_LAY;
        memset(&st3.cells, 0, sizeof(st3.cells));
        st3.stage = STG_PLAY; st3.winner = JQ_WIN_NONE; st3.turn = SIDE_BLUE;
        st3.cells[C51].rank = RANK_WORKER; st3.cells[C51].side = SIDE_BLUE; st3.cells[C51].revealed = 1;
        st3.cells[C100].rank = 1; st3.cells[C100].side = SIDE_RED; st3.cells[C100].revealed = 1;
        int c1_cross = jq_can_move(&st3, C51, C92) == 0;
        jq_state st4; jq_state_init(&st4); st4.mode = JQ_MODE_LAY;
        memset(&st4.cells, 0, sizeof(st4.cells));
        st4.stage = STG_PLAY; st4.winner = JQ_WIN_NONE; st4.turn = SIDE_BLUE;
        st4.cells[C54].rank = RANK_WORKER; st4.cells[C54].side = SIDE_BLUE; st4.cells[C54].revealed = 1;
        st4.cells[C100].rank = 1; st4.cells[C100].side = SIDE_RED; st4.cells[C100].revealed = 1;
        int c4_cross = jq_can_move(&st4, C54, C92) == 0;
        ok &= chk("⑤ 对照col2纵贯全高同段放行(col2(4,2)→(7,2)=1)vs col1/col4交叉格跨段row9-11拒(col1(5,1)→(9,2)=0,col4(5,4)→(9,2)=0)",
                  c2_ok && c1_cross && c4_cross);
    }
    /* ⑥ is_rail col1/col4 非纵贯全高实锤（col1 仅 row5/6 = 2 格 / col4 仅 row0/
       5/6/11 = 4 格，交叉格 (5,1)/(5,4) 工兵横贯线内拐弯可达 vs 跨段拐弯受限
       拓扑依据，§84 ⑤ 既有 + 本轮补交叉格场景） */
    {
        int c1 = 0, c4 = 0;
        for (int r = 0; r < 12; r++) {
            if (is_rail(r, 1)) c1++;
            if (is_rail(r, 4)) c4++;
        }
        ok &= chk("⑥ is_rail col1/col4非纵贯全高实锤(col1仅row5/6=2格/col4仅row0/5/6/11=4格,交叉格工兵拐弯拓扑依据§84⑤)",
                  c1 == 2 && c4 == 4);
    }
    printf("dark_rail_c1c4cross_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
