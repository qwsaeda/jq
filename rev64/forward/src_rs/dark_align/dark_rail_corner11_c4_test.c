/* dark_rail_corner11_c4_test.c — row11 col4 红方底角工兵拐弯（对称 row0 蓝方
 * col4 §87）+ col4 底角红蓝方全对称补全行为级实证
 * 补全现有缺口：§87 dark_rail_c4corner_cross_eat_test 实锤 row0 蓝方 col4 底角
 * （(0,4)）工兵拐弯受限（可达格仅 col4 下行 (1,4)，不能拐到 col2 (9,2)），§85
 * 实锤 row11 红方 col0 底角（(11,0)）工兵拐弯受限（可达格仅 col0 上行 (10,0)），
 * 但**row11 红方 col4 底角（(11,4)）工兵拐弯场景（对称 row0 蓝方 col4 底角）**
 * 无独立用例——col4 仅 row0/5/6/11 四格铁路（非纵贯全高，§84 ⑤ 实锤），row11
 * col4 红方底角 (11,4) 工兵拐弯受限（可达格仅 col4 上行 (10,4)，不能拐到 col2
 * 纵贯全高线），与 row0 蓝方 col4 底角（§87 ①）红蓝方对称未独立断言。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83/§84/§85 全格分布实
 * 锤：col2 纵贯 10 格 + row5/6 横贯 10 格 + row0/11 横贯两端 + col0=4 格 / col
 * 1=2 格 / col4=4 格非纵贯全高）+ rules.c rail_path_engineer 工兵 BFS 拐弯路径
 * 引擎（底角 col4 非纵贯全高拐弯受限）+ rail_path_straight 非工兵直走（row11
 * 横贯线两端非铁路阻断））：
 *   · row11 col4 红方底角工兵拐弯受限（col4 非纵贯全高）：工兵 @(11,4) 可达格
 *     仅 (10,4)（col4 上行），不能拐到 col2（(11,4)→(11,2) / (11,4)→(9,2) 均
 *     can=0，col4 非纵贯全高拐不到 col2 纵贯线，对称 §87 row0 蓝方 col4 底角
 *     拐弯受限）；
 *   · 对照 row0 蓝方 col4 底角（§87 既有）：工兵 @(0,4) 可达格仅 (1,4)（col4
 *     下行），不能拐到 col2——row11 col4 红方（本轮）与 row0 col4 蓝方（§87）
 *     底角拐弯受限红蓝方对称（仅 col4 上行/下行方向不同）；
 *   · row11 横贯线两端非铁路阻断（§80 既有）：非工兵 (11,4)→(11,0) can=0
 *     （row11 两端 col0/col4 非铁路，横贯线全宽滑行阻断，对称 §77 row0 两端非
 *     铁路）。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + rail_path_engineer 工兵 BFS，
 * 0 分叉；几何走 §80/§85/§87 既有 row11 横贯线两端非铁路 + col4 底角非纵贯全
 * 高 + row11 col4 红方对称口径）。
 *
 * 几何口径：row11 col4 红方底角 (11,4)（col4 非纵贯全高，对称 §87 row0 col4
 *   蓝方底角 (0,4)）；col4 上行 (10,4)（工兵底角唯一可达）；col2 (11,2)/(9,2)
 *   （纵贯线，底角拐弯目标）；row11 横贯线 (11,4)→(11,0)（两端非铁路）；蓝
 *   cell(0,0) 防困毙；mode=LAY 明棋；工兵 RANK_WORKER + 非工兵 rank5。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C114 cell_index(11,4)  /* row11 col4 红方底角（col4 非纵贯全高） */
#define C104 cell_index(10,4)  /* col4 上行（工兵底角唯一可达） */
#define C112 cell_index(11,2)  /* col2 row11（纵贯线，底角拐弯目标） */
#define C92 cell_index(9,2)    /* col2 row9（纵贯线 row9-11 段） */
#define C110 cell_index(11,0)  /* row11 横贯线远端（两端非铁路） */
#define C00 cell_index(0,0)    /* 蓝防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① row11 col4 红方底角工兵拐弯受限（工兵 @(11,4) 可达格仅 (10,4)，不能
       拐到 col2，对称 §87 row0 蓝方 col4 底角） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C114].rank = RANK_WORKER; st.cells[C114].side = SIDE_RED; st.cells[C114].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can_c2 = jq_can_move(&st, C114, C112);   /* 底角拐到 col2 (11,2) 拒 */
        int can_c4up = jq_can_move(&st, C114, C104); /* col4 上行 (10,4) 放行 */
        ok &= chk("① row11col4红方底角工兵拐弯受限(工兵(11,4)不能拐到col2(11,2)can=0,col4上行(10,4)can=1对称§87row0)",
                  can_c2 == 0 && can_c4up == 1);
    }
    /* ② 工兵 row11 col4 底角拐不到 col2 纵贯线 row9-11 段（(11,4)→(9,2) can=0，
       col4 非纵贯全高拐不到 col2） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C114].rank = RANK_WORKER; st.cells[C114].side = SIDE_RED; st.cells[C114].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C114, C92);
        ok &= chk("② 工兵row11col4底角拐不到col2纵贯线row9-11段((11,4)→(9,2)can=0col4非纵贯全高)",
                  can == 0);
    }
    /* ③ 非工兵 row11 col4 横贯线两端非铁路阻断（(11,4)→(11,0) can=0，§80 既有
       + 对称 §77 row0 两端非铁路） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C114].rank = 5; st.cells[C114].side = SIDE_RED; st.cells[C114].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C114, C110);
        ok &= chk("③ 非工兵row11col4横贯线两端非铁路阻断((11,4)→(11,0)can=0§80既有对称§77row0)",
                  can == 0);
    }
    /* ④ 对照 row0 蓝方 col4 底角（§87 既有）：工兵 @(0,4) 可达格仅 (1,4)（col4
       下行），不能拐到 col2——row11 col4 红方（①）与 row0 col4 蓝方（§87 ①）
       底角拐弯受限红蓝方对称 */
    {
        /* row0 蓝方 col4 底角（§87 ① 既有）对照：工兵 @(0,4) 拐不到 col2 + col4
           下行 (1,4) 放行 */
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[cell_index(0,4)].rank = RANK_WORKER; st.cells[cell_index(0,4)].side = SIDE_BLUE; st.cells[cell_index(0,4)].revealed = 1;
        st.cells[cell_index(10,0)].rank = 1; st.cells[cell_index(10,0)].side = SIDE_RED; st.cells[cell_index(10,0)].revealed = 1;
        int r0_c2 = jq_can_move(&st, cell_index(0,4), cell_index(9,2));
        int r0_down = jq_can_move(&st, cell_index(0,4), cell_index(1,4));
        /* row11 col4 红方底角（① 实锤）：拐 col2 拒 + col4 上行放行 */
        jq_state st2; jq_state_init(&st2); st2.mode = JQ_MODE_LAY;
        memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_RED;
        st2.cells[C114].rank = RANK_WORKER; st2.cells[C114].side = SIDE_RED; st2.cells[C114].revealed = 1;
        st2.cells[C00].rank = 1; st2.cells[C00].side = SIDE_BLUE; st2.cells[C00].revealed = 1;
        int r11_c2 = jq_can_move(&st2, C114, C112);
        int r11_up = jq_can_move(&st2, C114, C104);
        ok &= chk("④ col4底角拐弯受限红蓝方对称(row0蓝方拐col2拒+col4下行放行vsrow11红方拐col2拒+col4上行放行)",
                  r0_c2 == 0 && r0_down == 1 && r11_c2 == 0 && r11_up == 1);
    }
    /* ⑤ is_rail col4 纵贯全高性实锤（col4 仅 row0/5/6/11 = 4 格铁路，非纵贯
       全高——底角 col4 工兵拐弯受限拓扑依据，§84 ⑤ 既有 + 本轮补 row11 col4
       红方对称） */
    {
        int c4 = 0;
        for (int r = 0; r < 12; r++)
            if (is_rail(r, 4)) c4++;
        ok &= chk("⑤ is_rail col4非纵贯全高实锤(col4仅row0/5/6/11=4格铁路,底角col4工兵拐弯受限拓扑依据§84⑤)",
                  c4 == 4);
    }
    printf("dark_rail_corner11_c4_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
