/* dark_rail_r6cross_test.c — row6 横贯交叉格（(6,1)/(6,4)）工兵拐弯场景行为
 * 级实证（对称 row5 横贯 §89）
 * 补全现有缺口：§89 dark_rail_c1c4cross_test 实锤 col1/col4 交叉格（row5/6
 * 横贯 ∩ col1/col4）工兵拐弯场景"横贯线内拐弯可达 vs 跨段拐弯受限"差异（col1
 * 仅 row5/6 = 2 格 / col4 仅 row0/5/6/11 = 4 格非纵贯全高），但**row6 横贯交
 * 叉格（(6,1)/(6,4)）工兵拐弯场景（对称 row5 横贯）**无独立用例——col1/col4 仅
 * row5/6 铁路（col4 另含 row0/11），row6 横贯交叉格 (6,1)/(6,4) 工兵 row6 横
 * 贯线内拐弯到 col2 纵贯 row4-7 段可达 vs 跨段拐弯到 col2 纵贯 row0-2/row9-11
 * 段受限（col1/col4 非纵贯全高拐不到），与 row5 横贯交叉格（§89 既有）对称未独
 * 立断言。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83/§84/§85/§87/§88/§89
 * 全格分布实锤：col2 纵贯 10 格 + row5/6 横贯 10 格 + row0/11 横贯两端 + col0=
 * 4 格 / col1=2 格 / col4=4 格非纵贯全高）+ rules.c rail_path_engineer 工兵 B
 * FS 拐弯路径引擎（col1/col4 交叉格 row5/6 横贯线内拐弯到 col2 纵贯 row4-7 段
 * 可达，跨段拐弯到 col2 纵贯 row0-2/row9-11 段受限）+ rail_path_straight 非工
 * 兵直走（col1/col4 非纵贯全高纵向跨段拒））：
 *   · row6 col1 交叉格 (6,1) 工兵横贯线内拐弯可达（row6 横贯 → col2 纵贯 row4-
 *     7 段拐弯，(6,1)→(6,2) 同 row6 横贯 can=1）；
 *   · row6 col1 交叉格 (6,1) 跨段拐弯受限（col1 非纵贯全高，(6,1)→(9,2) 拐不
 *     到 col2 纵贯 row9-11 段 can=0，(6,1)→(3,2) 拐不到 col2 纵贯 row0-2 段
 *     can=0）；
 *   · row6 col4 交叉格 (6,4) 工兵横贯线内拐弯可达（row6 横贯 → col2 纵贯 row4-
 *     7 段拐弯，(6,4)→(6,2) 同 row6 横贯 can=1）；
 *   · row6 col4 交叉格 (6,4) 跨段拐弯受限（col4 非纵贯全高，(6,4)→(9,2) 拐不
 *     到 col2 纵贯 row9-11 段 can=0）；
 *   · 对照 row5 横贯交叉格（§89 既有）：row5 (5,1)→(5,2) 同 row5 横贯 can=1
 *     vs row6 (6,1)→(6,2) 同 row6 横贯 can=1——row5/row6 横贯交叉格工兵拐弯
 *     "横贯线内拐弯可达 vs 跨段拐弯受限"对称实锤。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + rail_path_engineer 工兵 BFS，
 * 0 分叉；几何走 §89 既有 col1/col4 非纵贯全高 + col2 纵贯全高 + row5/6 横贯
 * 交叉格对称口径）。
 *
 * 几何口径：row6 col1 交叉格 (6,1)（row6 横贯 ∩ col1，col1 仅 row5/6 铁路非纵
 *   贯全高）；row6 col4 交叉格 (6,4)（row6 横贯 ∩ col4，col4 仅 row0/5/6/11
 *   铁路非纵贯全高）；col2 纵贯 (6,2)（row6 横贯 ∩ col2 纵贯全高）；col2 纵贯
 *   row0-2 段 (3,2) / row9-11 段 (9,2)（跨段拐弯目标，col1/col4 非纵贯全高拐
 *   不到）；对照 row5 col1 交叉格 (5,1)（§89 既有）；蓝 cell(0,0) 防困毙；mode
 *   =LAY 明棋；工兵 RANK_WORKER。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C61 cell_index(6,1)    /* row6 col1 交叉格 (row6 横贯 ∩ col1) */
#define C64 cell_index(6,4)    /* row6 col4 交叉格 (row6 横贯 ∩ col4) */
#define C62 cell_index(6,2)    /* col2 纵贯 (row6 横贯 ∩ col2 纵贯全高) */
#define C32 cell_index(3,2)    /* col2 纵贯 row0-2 段（跨段拐弯目标） */
#define C92 cell_index(9,2)    /* col2 纵贯 row9-11 段（跨段拐弯目标） */
#define C51 cell_index(5,1)    /* 对照 row5 col1 交叉格（§89 既有） */
#define C00 cell_index(0,0)    /* 蓝防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① row6 col1 交叉格 (6,1) 工兵横贯线内拐弯可达（row6 横贯 → col2 纵贯
       row4-7 段拐弯，(6,1)→(6,2) 同 row6 横贯 can=1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C61].rank = RANK_WORKER; st.cells[C61].side = SIDE_BLUE; st.cells[C61].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can_same = jq_can_move(&st, C61, C62);   /* 同 row6 横贯 (6,1)→(6,2) 放行 */
        ok &= chk("① row6col1交叉格工兵横贯线内拐弯可达(工兵(6,1)→col2(6,2)同row6横贯can=1,row6横贯→col2纵贯row4-7段拐弯)",
                  can_same == 1);
    }
    /* ② row6 col1 交叉格 (6,1) 跨段拐弯受限（col1 非纵贯全高，(6,1)→(9,2) 拐
       不到 col2 纵贯 row9-11 段 can=0，(6,1)→(3,2) 拐不到 col2 纵贯 row0-2
       段 can=0） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C61].rank = RANK_WORKER; st.cells[C61].side = SIDE_BLUE; st.cells[C61].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can_down = jq_can_move(&st, C61, C92);  /* 跨段 (6,1)→(9,2) 拐不到 */
        int can_up = jq_can_move(&st, C61, C32);    /* 跨段 (6,1)→(3,2) 拐不到 */
        ok &= chk("② row6col1交叉格工兵跨段拐弯受限(工兵(6,1)→col2(9,2)拐不到col2纵贯row9-11段can=0,工兵(6,1)→col2(3,2)拐不到col2纵贯row0-2段can=0,col1仅row5/6铁路非纵贯全高)",
                  can_down == 0 && can_up == 0);
    }
    /* ③ row6 col4 交叉格 (6,4) 工兵横贯线内拐弯可达（row6 横贯 → col2 纵贯
       row4-7 段拐弯，(6,4)→(6,2) 同 row6 横贯 can=1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C64].rank = RANK_WORKER; st.cells[C64].side = SIDE_BLUE; st.cells[C64].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can_same = jq_can_move(&st, C64, C62);   /* 同 row6 横贯 (6,4)→(6,2) 放行 */
        ok &= chk("③ row6col4交叉格工兵横贯线内拐弯可达(工兵(6,4)→col2(6,2)同row6横贯can=1,row6横贯→col2纵贯row4-7段拐弯)",
                  can_same == 1);
    }
    /* ④ row6 col4 交叉格 (6,4) 跨段拐弯受限（col4 非纵贯全高，(6,4)→(9,2) 拐
       不到 col2 纵贯 row9-11 段 can=0） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C64].rank = RANK_WORKER; st.cells[C64].side = SIDE_BLUE; st.cells[C64].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can_down = jq_can_move(&st, C64, C92);  /* 跨段 (6,4)→(9,2) 拐不到 */
        ok &= chk("④ row6col4交叉格工兵跨段拐弯受限(工兵(6,4)→col2(9,2)拐不到col2纵贯row9-11段can=0,col4仅row0/5/6/11铁路非纵贯全高)",
                  can_down == 0);
    }
    /* ⑤ 对照 row5 横贯交叉格（§89 既有）：row5 (5,1)→(5,2) 同 row5 横贯 can=1
       vs row6 (6,1)→(6,2) 同 row6 横贯 can=1——row5/row6 横贯交叉格工兵拐弯对
       称实锤 */
    {
        /* row5 横贯交叉格（§89 既有）：(5,1)→(5,2) 同 row5 横贯放行 */
        jq_state st2; jq_state_init(&st2); st2.mode = JQ_MODE_LAY;
        memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[C51].rank = RANK_WORKER; st2.cells[C51].side = SIDE_BLUE; st2.cells[C51].revealed = 1;
        st2.cells[C00].rank = 1; st2.cells[C00].side = SIDE_BLUE; st2.cells[C00].revealed = 1;
        int r5_same = jq_can_move(&st2, C51, cell_index(5, 2));  /* row5 同横贯 */
        /* row6 横贯交叉格（① 实锤）：(6,1)→(6,2) 同 row6 横贯放行 */
        jq_state st3; jq_state_init(&st3); st3.mode = JQ_MODE_LAY;
        memset(&st3.cells, 0, sizeof(st3.cells));
        st3.stage = STG_PLAY; st3.winner = JQ_WIN_NONE; st3.turn = SIDE_BLUE;
        st3.cells[C61].rank = RANK_WORKER; st3.cells[C61].side = SIDE_BLUE; st3.cells[C61].revealed = 1;
        st3.cells[C00].rank = 1; st3.cells[C00].side = SIDE_BLUE; st3.cells[C00].revealed = 1;
        int r6_same = jq_can_move(&st3, C61, C62);  /* row6 同横贯 */
        ok &= chk("⑤ 对照row5横贯交叉格(§89既有(5,1)→(5,2)=1)vs row6横贯交叉格((6,1)→(6,2)=1),row5/row6对称",
                  r5_same == 1 && r6_same == 1);
    }
    /* ⑥ is_rail col1/col4 非纵贯全高实锤（col1 仅 row5/6 = 2 格 / col4 仅 row0/
       5/6/11 = 4 格，row6 横贯交叉格工兵拐弯"横贯线内拐弯可达 vs 跨段拐弯受限"
       对称 row5 拓扑依据，§84 ⑤ / §89 ⑥ 既有 + 本轮补 row6 对称） */
    {
        int c1 = 0, c4 = 0;
        for (int r = 0; r < 12; r++) {
            if (is_rail(r, 1)) c1++;
            if (is_rail(r, 4)) c4++;
        }
        ok &= chk("⑥ is_rail col1/col4非纵贯全高实锤(col1仅row5/6=2格/col4仅row0/5/6/11=4格,row6横贯交叉格工兵拐弯对称row5拓扑依据§84⑤§89⑥)",
                  c1 == 2 && c4 == 4);
    }
    printf("dark_rail_r6cross_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
