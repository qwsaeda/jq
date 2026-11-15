/* dark_rail_r0r11_ends_test.c — row0/row11 横贯线两端（col0/col4）工兵拐弯场
 * 景行为级实证 + "横贯线两端铁路属性"口径纠正
 * 补全现有缺口：§85 dark_rail_corner11_test（实锤 row11 col0 底角工兵拐弯受
 * 限，可达格仅 col0 上行 (10,0)）+ §88 dark_rail_corner11_c4_test（实锤 row11
 * col4 底角工兵拐弯受限，可达格仅 col4 上行 (10,4)）+ §84/§87（实锤 row0 col0
 * /col4 底角工兵拐弯受限），但**row0/row11 横贯线两端（col0/col4）工兵拐弯场
 * 景全对称（4 底角 row0 col0/col4 + row11 col0/col4 工兵拐弯受限对照）+ 横贯
 * 线两端铁路属性口径纠正**无独立用例——§80/§85/§88 既有口径"row0/11 横贯线两
 * 端非铁路"系误判：实锤 is_rail(row0/11, col0/4)=1（横贯线全宽铁路，两端含
 * col0/col4），工兵底角拐弯受限真因 = col0/col4 纵贯线非纵贯全高（col0 仅
 * row0/5/6/11 = 4 格 / col4 仅 row0/5/6/11 = 4 格），拐不到 col2 纵贯全高线
 * row4-7/row9-11 段。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83/§84/§85/§87/§88/§89
 * 全格分布实锤：col2 纵贯 10 格 + row5/6 横贯 10 格 + row0/11 横贯两端 + col0
 * =4 格 / col1=2 格 / col4=4 格非纵贯全高）+ rules.c rail_path_engineer 工兵 B
 * FS 拐弯路径引擎（底角 col0/col4 非纵贯全高拐弯受限，横贯线两端 is_rail=1 但
 * 纵贯线拐不到 col2 纵贯全高线））：
 *   · row0/row11 横贯线两端 col0/col4 是铁路（is_rail=1，横贯线全宽）——纠正
 *     §80/§85/§88 "两端非铁路"误判口径；
 *   · 工兵底角拐弯受限真因 = col0/col4 纵贯线非纵贯全高（col0/col4 仅 4 格铁
 *     路），拐不到 col2 纵贯全高线 row4-7/row9-11 段（col2 唯一纵贯全高 10
 *     格）；
 *   · 4 底角（row0 col0/col4 + row11 col0/col4）工兵拐弯受限全对称（row0 蓝方
 *     下行 col0/col4 / row11 红方上行 col0/col4，拐不到 col2 纵贯全高线），对
 *     称 §84/§85/§87/§88 既有底角工兵拐弯受限实锤。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + rail_path_engineer 工兵 BFS，
 * 0 分叉；几何走 §80/§84/§85/§87/§88 既有 row0/11 横贯线两端 + 底角 col0/col4
 * 非纵贯全高口径 + 横贯线两端铁路属性纠正）。
 *
 * 几何口径：row0 横贯线两端 (0,0) col0 蓝底角 / (0,4) col4 蓝底角（is_rail=1
 *   横贯线全宽）；row11 横贯线两端 (11,0) col0 红底角 / (11,4) col4 红底角
 *   （is_rail=1 横贯线全宽）；col0 下行 (1,0) / col4 下行 (1,4)（row0 蓝方）
 *   / col0 上行 (10,0) / col4 上行 (10,4)（row11 红方）工兵底角唯一可达；col2
 *   (0,2)/(11,2)（纵贯全高线，底角拐弯目标拐不到）；col2 纵贯 row4-7 段 (4,2)
 *   / row9-11 段 (9,2)（col0/col4 非纵贯全高拐不到）；红/蓝 cell(10,0)/(0,0)
 *   防困毙；mode=LAY 明棋；工兵 RANK_WORKER。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C00 cell_index(0,0)    /* row0 col0 蓝底角（横贯线两端 col0，is_rail=1） */
#define C04 cell_index(0,4)    /* row0 col4 蓝底角（横贯线两端 col4，is_rail=1） */
#define C110 cell_index(11,0)  /* row11 col0 红底角（横贯线两端 col0，is_rail=1） */
#define C114 cell_index(11,4)  /* row11 col4 红底角（横贯线两端 col4，is_rail=1） */
#define C10 cell_index(1,0)    /* col0 下行（row0 蓝方工兵底角唯一可达） */
#define C14 cell_index(1,4)    /* col4 下行（row0 蓝方工兵底角唯一可达） */
#define C100 cell_index(10,0)  /* col0 上行（row11 红方工兵底角唯一可达） */
#define C104 cell_index(10,4)  /* col4 上行（row11 红方工兵底角唯一可达） */
#define C02 cell_index(0,2)    /* col2 row0（纵贯全高线，底角拐弯目标） */
#define C112 cell_index(11,2)  /* col2 row11（纵贯全高线，底角拐弯目标） */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① row0/row11 横贯线两端 col0/col4 是铁路（is_rail=1，横贯线全宽）——纠
       正 §80/§85/§88 "两端非铁路"误判口径 */
    {
        int r0_c0 = is_rail(0, 0), r0_c4 = is_rail(0, 4);
        int r11_c0 = is_rail(11, 0), r11_c4 = is_rail(11, 4);
        ok &= chk("① row0/row11横贯线两端col0/col4是铁路(is_rail(0,0)/(0,4)/(11,0)/(11,4)=1横贯线全宽,纠正§80/§85/§88两端非铁路误判)",
                  r0_c0 == 1 && r0_c4 == 1 && r11_c0 == 1 && r11_c4 == 1);
    }
    /* ② row0 蓝方 col0 底角工兵拐弯受限（工兵 @(0,0) 可达格仅 col0 下行 (1,0)
       ，不能拐到 col2 纵贯全高线，对称 §84 既有） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C00].rank = RANK_WORKER; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can_c2 = jq_can_move(&st, C00, C02);   /* 底角拐到 col2 (0,2) 拒 */
        int can_c0down = jq_can_move(&st, C00, C10); /* col0 下行 (1,0) 放行 */
        ok &= chk("② row0col0底角工兵拐弯受限(工兵(0,0)不能拐到col2(0,2)can=0,col0下行(1,0)can=1对称§84,col0非纵贯全高拐不到col2纵贯全高线)",
                  can_c2 == 0 && can_c0down == 1);
    }
    /* ③ row0 蓝方 col4 底角工兵拐弯受限（工兵 @(0,4) 可达格仅 col4 下行 (1,4)
       ，不能拐到 col2 纵贯全高线，对称 §87 既有） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C04].rank = RANK_WORKER; st.cells[C04].side = SIDE_BLUE; st.cells[C04].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can_c2 = jq_can_move(&st, C04, C02);   /* 底角拐到 col2 (0,2) 拒 */
        int can_c4down = jq_can_move(&st, C04, C14); /* col4 下行 (1,4) 放行 */
        ok &= chk("③ row0col4底角工兵拐弯受限(工兵(0,4)不能拐到col2(0,2)can=0,col4下行(1,4)can=1对称§87,col4非纵贯全高拐不到col2纵贯全高线)",
                  can_c2 == 0 && can_c4down == 1);
    }
    /* ④ row11 红方 col0 底角工兵拐弯受限（工兵 @(11,0) 可达格仅 col0 上行
       (10,0)，不能拐到 col2 纵贯全高线，对称 §85 既有） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C110].rank = RANK_WORKER; st.cells[C110].side = SIDE_RED; st.cells[C110].revealed = 1;
        st.cells[cell_index(0,0)].rank = 1; st.cells[cell_index(0,0)].side = SIDE_BLUE; st.cells[cell_index(0,0)].revealed = 1;
        int can_c2 = jq_can_move(&st, C110, C112);   /* 底角拐到 col2 (11,2) 拒 */
        int can_c0up = jq_can_move(&st, C110, C100); /* col0 上行 (10,0) 放行 */
        ok &= chk("④ row11col0底角工兵拐弯受限(工兵(11,0)不能拐到col2(11,2)can=0,col0上行(10,0)can=1对称§85,col0非纵贯全高拐不到col2纵贯全高线)",
                  can_c2 == 0 && can_c0up == 1);
    }
    /* ⑤ row11 红方 col4 底角工兵拐弯受限（工兵 @(11,4) 可达格仅 col4 上行
       (10,4)，不能拐到 col2 纵贯全高线，对称 §88 既有） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C114].rank = RANK_WORKER; st.cells[C114].side = SIDE_RED; st.cells[C114].revealed = 1;
        st.cells[cell_index(0,0)].rank = 1; st.cells[cell_index(0,0)].side = SIDE_BLUE; st.cells[cell_index(0,0)].revealed = 1;
        int can_c2 = jq_can_move(&st, C114, C112);   /* 底角拐到 col2 (11,2) 拒 */
        int can_c4up = jq_can_move(&st, C114, C104); /* col4 上行 (10,4) 放行 */
        ok &= chk("⑤ row11col4底角工兵拐弯受限(工兵(11,4)不能拐到col2(11,2)can=0,col4上行(10,4)can=1对称§88,col4非纵贯全高拐不到col2纵贯全高线)",
                  can_c2 == 0 && can_c4up == 1);
    }
    /* ⑥ 4 底角工兵拐弯受限全对称（row0 蓝方 col0/col4 下行 + row11 红方
       col0/col4 上行，拐不到 col2 纵贯全高线）对照实锤 + is_rail 横贯线两端铁
       路属性 + col0/col4 纵贯线非纵贯全高拓扑依据 */
    {
        int r0_c0_down = 0, r0_c4_down = 0, r11_c0_up = 0, r11_c4_up = 0;
        int r0_c0_c2 = 0, r0_c4_c2 = 0, r11_c0_c2 = 0, r11_c4_c2 = 0;
        /* row0 蓝方 col0 底角（② 既有） */
        {
            jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
            memset(&st.cells, 0, sizeof(st.cells));
            st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
            st.cells[C00].rank = RANK_WORKER; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
            st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
            r0_c0_down = (jq_can_move(&st, C00, C10) == 1);
            r0_c0_c2 = (jq_can_move(&st, C00, C02) == 0);
        }
        /* row0 蓝方 col4 底角（③ 既有） */
        {
            jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
            memset(&st.cells, 0, sizeof(st.cells));
            st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
            st.cells[C04].rank = RANK_WORKER; st.cells[C04].side = SIDE_BLUE; st.cells[C04].revealed = 1;
            st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
            r0_c4_down = (jq_can_move(&st, C04, C14) == 1);
            r0_c4_c2 = (jq_can_move(&st, C04, C02) == 0);
        }
        /* row11 红方 col0 底角（④ 既有） */
        {
            jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
            memset(&st.cells, 0, sizeof(st.cells));
            st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
            st.cells[C110].rank = RANK_WORKER; st.cells[C110].side = SIDE_RED; st.cells[C110].revealed = 1;
            st.cells[cell_index(0,0)].rank = 1; st.cells[cell_index(0,0)].side = SIDE_BLUE; st.cells[cell_index(0,0)].revealed = 1;
            r11_c0_up = (jq_can_move(&st, C110, C100) == 1);
            r11_c0_c2 = (jq_can_move(&st, C110, C112) == 0);
        }
        /* row11 红方 col4 底角（⑤ 既有） */
        {
            jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
            memset(&st.cells, 0, sizeof(st.cells));
            st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
            st.cells[C114].rank = RANK_WORKER; st.cells[C114].side = SIDE_RED; st.cells[C114].revealed = 1;
            st.cells[cell_index(0,0)].rank = 1; st.cells[cell_index(0,0)].side = SIDE_BLUE; st.cells[cell_index(0,0)].revealed = 1;
            r11_c4_up = (jq_can_move(&st, C114, C104) == 1);
            r11_c4_c2 = (jq_can_move(&st, C114, C112) == 0);
        }
        int c0 = 0, c4 = 0;
        for (int r = 0; r < 12; r++) {
            if (is_rail(r, 0)) c0++;
            if (is_rail(r, 4)) c4++;
        }
        ok &= chk("⑥ 4底角工兵拐弯受限全对称(row0蓝方col0/col4下行+row11红方col0/col4上行拐不到col2纵贯全高线)+横贯线两端铁路属性+col0/col4纵贯线非纵贯全高(col0=4格/col4=4格)拓扑依据",
                  r0_c0_down && r0_c4_down && r11_c0_up && r11_c4_up
                  && r0_c0_c2 && r0_c4_c2 && r11_c0_c2 && r11_c4_c2
                  && c0 == 4 && c4 == 4);
    }
    printf("dark_rail_r0r11_ends_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
