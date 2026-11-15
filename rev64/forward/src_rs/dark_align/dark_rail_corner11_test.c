/* dark_rail_corner11_test.c — row11 红方底角工兵拐弯（对称 row0 蓝方）+ col0 非铁
 * 路纵贯线拐弯受限行为级实证
 * 补全现有缺口：§84 dark_rail_corner_nonrail_test 实锤 row0 蓝方底角工兵拐弯受限
 * （工兵 @(0,0) 可达格仅 col0 下行 (1,0)，不能拐到 col2）+ col1/col4 非纵贯全高
 * 铁路线纵向跨段 can=0，但**row11 红方底角工兵拐弯（对称 row0 蓝方）**无独立用
 * 例——is_rail 全格分布实锤（§83 ⑤ / §84 ⑤）col0 非铁路纵贯线（col0 仅 row0 一
 * 格铁路），row11 红方底角 (11,0) 工兵拐弯受限（可达格仅 col0 上行 (10,0)，不
 * 能拐到 col2），与 row0 蓝方底角（§84 ①）对称未独立断言。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83/§84 全格分布实锤：
 * col2 纵贯 10 格 + row5/6 横贯 10 格 + row0/11 横贯两端 + col0/col1/col4 非纵
 * 贯全高）+ rules.c rail_path_engineer 工兵 BFS 拐弯路径引擎（底角非铁路纵贯线
 * 拐弯受限））：
 *   · row11 红方底角工兵拐弯受限（col0 非铁路纵贯线）：工兵 @(11,0) 可达格仅
 *     (10,0)（col0 上行），不能拐到 col2（(11,0)→(11,2) / (11,0)→(9,2) 均
 *     can=0，col0 非铁路纵贯线，对称 §84 row0 蓝方底角拐弯受限）；
 *   · row11 横贯线两端非铁路阻断（§80 既有）：非工兵 (11,0)→(11,4) can=0
 *     （row11 两端 col0/col4 非铁路，横贯线全宽滑行阻断，对称 §77 row0 两端非
 *     铁路）；
 *   · 工兵 row11 底角拐不到 col2 纵贯线（(11,0)→(9,2) can=0）：col0 非铁路纵贯
 *     线，工兵底角拐弯到 col2 需经 row11 横贯线（两端非铁路阻断）+ col2 纵贯线
 *     （col0/col1 非铁路），拐不到。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + rail_path_engineer 工兵 BFS，
 * 0 分叉；几何走 §80/§84 既有 row11 横贯线两端非铁路 + 底角 col0 非铁路纵贯线
 * 口径 + row11 红方对称）。
 *
 * 几何口径：row11 红方底角 (11,0)（col0 非铁路纵贯线，对称 §84 row0 蓝方底角
 *   (0,0)）；col0 上行 (10,0)（工兵底角唯一可达）；col2 (11,2)/(9,2)（纵贯线，
 *   底角拐弯目标）；row11 横贯线 (11,0)→(11,4)（两端非铁路）；蓝 cell(0,0) 防困
 *   毙；mode=LAY 明棋；工兵 RANK_WORKER + 非工兵 rank5。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C110 cell_index(11,0)  /* row11 红方底角（col0 非铁路纵贯线） */
#define C100 cell_index(10,0)  /* col0 上行（工兵底角唯一可达） */
#define C112 cell_index(11,2)  /* col2 row11（纵贯线，底角拐弯目标） */
#define C92 cell_index(9,2)    /* col2 row9（纵贯线 row9-11 段） */
#define C114 cell_index(11,4)  /* row11 横贯线远端（两端非铁路） */
#define C00 cell_index(0,0)    /* 蓝防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① row11 红方底角工兵拐弯受限（工兵 @(11,0) 可达格仅 (10,0)，不能拐到
       col2，对称 §84 row0 蓝方底角） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C110].rank = RANK_WORKER; st.cells[C110].side = SIDE_RED; st.cells[C110].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can_c2 = jq_can_move(&st, C110, C112);   /* 底角拐到 col2 (11,2) 拒 */
        int can_c0up = jq_can_move(&st, C110, C100); /* col0 上行 (10,0) 放行 */
        ok &= chk("① row11红方底角工兵拐弯受限(工兵(11,0)不能拐到col2(11,2)can=0,col0上行(10,0)can=1对称§84row0)",
                  can_c2 == 0 && can_c0up == 1);
    }
    /* ② 工兵 row11 底角拐不到 col2 纵贯线 row9-11 段（(11,0)→(9,2) can=0，
       col0 非铁路纵贯线拐不到 col2） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C110].rank = RANK_WORKER; st.cells[C110].side = SIDE_RED; st.cells[C110].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C110, C92);
        ok &= chk("② 工兵row11底角拐不到col2纵贯线row9-11段((11,0)→(9,2)can=0col0非铁路)",
                  can == 0);
    }
    /* ③ 非工兵 row11 横贯线两端非铁路阻断（(11,0)→(11,4) can=0，§80 既有 +
       对称 §77 row0 两端非铁路） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C110].rank = 5; st.cells[C110].side = SIDE_RED; st.cells[C110].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C110, C114);
        ok &= chk("③ 非工兵row11横贯线两端非铁路阻断((11,0)→(11,4)can=0§80既有对称§77row0)",
                  can == 0);
    }
    /* ④ 对照 row0 蓝方底角（§84 既有）：工兵 @(0,0) 可达格仅 (1,0)（col0 下行），
       不能拐到 col2——row11 红方底角（①）与 row0 蓝方底角（§84 ①）底角拐弯受限
       红蓝方对称 */
    {
        /* row0 蓝方底角（§84 ① 既有）对照：工兵 @(0,0) 拐不到 col2 + col0 下行
           (1,0) 放行 */
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[cell_index(0,0)].rank = RANK_WORKER; st.cells[cell_index(0,0)].side = SIDE_BLUE; st.cells[cell_index(0,0)].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int r0_c2 = jq_can_move(&st, cell_index(0,0), cell_index(2,2));   /* row0 底角拐 col2 拒 */
        int r0_down = jq_can_move(&st, cell_index(0,0), cell_index(1,0)); /* col0 下行放行 */
        /* row11 红方底角（① 实锤）：拐 col2 拒 + col0 上行放行 */
        jq_state st2; jq_state_init(&st2); st2.mode = JQ_MODE_LAY;
        memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_RED;
        st2.cells[C110].rank = RANK_WORKER; st2.cells[C110].side = SIDE_RED; st2.cells[C110].revealed = 1;
        st2.cells[C00].rank = 1; st2.cells[C00].side = SIDE_BLUE; st2.cells[C00].revealed = 1;
        int r11_c2 = jq_can_move(&st2, C110, C112);
        int r11_up = jq_can_move(&st2, C110, C100);
        ok &= chk("④ 底角拐弯受限红蓝方对称(row0蓝方拐col2拒+col0下行放行 vs row11红方拐col2拒+col0上行放行)",
                  r0_c2 == 0 && r0_down == 1 && r11_c2 == 0 && r11_up == 1);
    }
    /* ⑤ is_rail 底角 col0 纵贯全高性实锤（col0 仅 row0/5/6/11 四格铁路，非纵贯
       全高——底角 row0/row11 col0 有铁路但 row1/row2 等中段 col0 非铁路，底角
       工兵拐弯受限因 col0 非纵贯全高纵线） */
    {
        int c0 = 0;
        for (int r = 0; r < 12; r++)
            if (is_rail(r, 0)) c0++;
        /* col0 仅 row0/5/6/11 = 4 格铁路（非纵贯全高），底角工兵拐弯受限拓扑依据 */
        ok &= chk("⑤ is_rail底角col0纵贯全高性实锤(col0仅row0/5/6/11=4格铁路非纵贯全高)",
                  c0 == 4);
    }
    printf("dark_rail_corner11_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
