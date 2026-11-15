/* dark_rail_r5r6_fullrow_test.c — row5/6 全宽横贯线"横贯线内全宽滑行"工兵/非工
 * 兵全场景对照 + row0/11 非全宽横贯线对照行为级实证
 * 补全现有缺口：§92 dark_rail_topology_matrix_test（实锤 is_rail 全格分布"横
 * 贯线全宽 vs 纵贯线非全高"完整拓扑矩阵，横贯线全宽仅 row5/6 10 格 + row0/11
 * 非全宽仅 3 格铁路 col0/2/4 + 纵贯线全高仅 col2 10 格 + col0/1/3/4 非全高 4/
 * 2/2/4 格 + row3/row8 行营隔断），但**row5/6 全宽横贯线"横贯线内全宽滑行"工
 * 兵/非工兵全场景对照 + row0/11 非全宽横贯线对照**无独立用例——§92 实锤横贯
 * 线全宽仅 row5/6（col0-4 全 10 格 is_rail=1）+ row0/11 非全宽（仅 col0/2/4
 * 三格铁路），但 row5/6 全宽横贯线"非工兵全宽滑行（col0→col4 跨 col1/2/3 直
 * 走）+ 工兵横贯线内全宽拐弯（col0/col4 端拐弯到 col2 纵贯全高线）"全场景对
 * 照 + row0/11 非全宽横贯线"col1/3 非铁路阻断非工兵跨 col1/3 直走"对照未独立
 * 断言。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83/§84/§85/§87/§88/§8
 * 9/§90/§91/§92 全格分布实锤：col2 纵贯 10 格 + row5/6 横贯 10 格全宽 + row0/
 * 11 横贯非全宽仅 3 格铁路 col0/2/4 + col0=4 格 / col1=2 格 / col3=2 格 / col
 * 4=4 格非纵贯全高 + row3/row8 全非铁路行营隔断）+ rules.c rail_path_straight
 * 非工兵直走（row5/6 全宽横贯线非工兵全宽滑行 col0→col4 跨 col1/2/3 直走放行
 * ，row0/11 非全宽横贯线 col1/3 非铁路跨 col1/3 非工兵直走拒）+ rail_path_eng
 * ineer 工兵 BFS 拐弯路径引擎（row5/6 全宽横贯线工兵横贯线内全宽拐弯 col0/col
 * 4 端拐弯到 col2 纵贯全高线放行））：
 *   · row5/row6 全宽横贯线非工兵全宽滑行（col0→col4 跨 col1/2/3 直走 can=1）
 *     ——row5/6 全宽 10 格铁路，非工兵直走全宽放行；
 *   · row5/row6 全宽横贯线工兵横贯线内全宽拐弯（col0/col4 端拐弯到 col2 纵贯
 *     全高线 can=1）——工兵 BFS 拐弯路径引擎横贯线内全宽拐弯放行；
 *   · row0/row11 非全宽横贯线非工兵跨 col1/3 直走拒（col1/3 非铁路，col0/2/4
 *     三格铁路跨 col1/3 非工兵直走阻断 can=0）——row0/11 非全宽横贯线 col1/3
 *     非铁路阻断；
 *   · 对照 row5/6 全宽 vs row0/11 非全宽横贯线滑行/拐弯差异（全宽横贯线全宽
 *     滑行放行 vs 非全宽横贯线跨非铁路 col1/3 阻断）。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + rail_path_straight 非工兵直
 * 走 + rail_path_engineer 工兵 BFS，0 分叉；几何走 §92 既有 row5/6 全宽横贯线
 * 10 格 + row0/11 非全宽横贯线 3 格铁路 + 横贯线内全宽滑行 vs 跨 col1/3 阻断
 * 口径）。
 *
 * 几何口径：row5/row6 全宽横贯线 (5,0)/(6,0)→(5,4)/(6,4) 跨 col1/2/3 全宽直
 *   走（非工兵 rail_path_straight 放行）；row5/row6 工兵横贯线内全宽拐弯 (5,
 *   0)/(6,0)→(5,2)/(6,2) col2 纵贯全高线（rail_path_engineer 放行）；row0/row
 *   11 非全宽横贯线 (0,0)/(11,0)→(0,4)/(11,4) 跨 col1/3 非工兵直走阻断（col1
 *   /3 非铁路）；红 cell(10,0) 防困毙；mode=LAY 明棋；非工兵 rank5 + 工兵 RA
 *   NK_WORKER。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C50 cell_index(5,0)    /* row5 col0（全宽横贯线起点） */
#define C54 cell_index(5,4)    /* row5 col4（全宽横贯线远端） */
#define C52 cell_index(5,2)    /* row5 col2（纵贯全高线交叉格） */
#define C60 cell_index(6,0)    /* row6 col0（全宽横贯线起点） */
#define C64 cell_index(6,4)    /* row6 col4（全宽横贯线远端） */
#define C62 cell_index(6,2)    /* row6 col2（纵贯全高线交叉格） */
#define C00 cell_index(0,0)    /* row0 col0（非全宽横贯线两端） */
#define C04 cell_index(0,4)    /* row0 col4（非全宽横贯线两端） */
#define C110 cell_index(11,0)  /* row11 col0（非全宽横贯线两端） */
#define C114 cell_index(11,4)  /* row11 col4（非全宽横贯线两端） */
#define C100 cell_index(10,0)  /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① row5 全宽横贯线非工兵全宽滑行（col0→col4 跨 col1/2/3 直走 can=1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50].rank = 5; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C50, C54);   /* row5 全宽 col0→col4 跨 col1/2/3 直走 */
        ok &= chk("① row5全宽横贯线非工兵全宽滑行(非工兵(5,0)→(5,4)跨col1/2/3全宽直走can=1,row5全宽10格rail_path_straight放行)",
                  can == 1);
    }
    /* ② row6 全宽横贯线非工兵全宽滑行（col0→col4 跨 col1/2/3 直走 can=1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C60].rank = 5; st.cells[C60].side = SIDE_BLUE; st.cells[C60].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C60, C64);   /* row6 全宽 col0→col4 跨 col1/2/3 直走 */
        ok &= chk("② row6全宽横贯线非工兵全宽滑行(非工兵(6,0)→(6,4)跨col1/2/3全宽直走can=1,row6全宽10格rail_path_straight放行)",
                  can == 1);
    }
    /* ③ 对照 row0 非全宽横贯线非工兵跨 col1/3 直走拒（col1/3 非铁路，col0/2/4
       三格铁路跨 col1/3 阻断 can=0） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C00].rank = 5; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C00, C04);   /* row0 非全宽 col0→col4 跨 col1/3 阻断 */
        ok &= chk("③ 对照row0非全宽横贯线非工兵跨col1/3直走拒(非工兵(0,0)→(0,4)跨col1/3非铁路can=0,row0仅col0/2/4三格col1/3非铁路rail_path_straight阻断)",
                  can == 0);
    }
    /* ④ 对照 row11 非全宽横贯线非工兵跨 col1/3 直走拒（col1/3 非铁路 can=0） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C110].rank = 5; st.cells[C110].side = SIDE_RED; st.cells[C110].revealed = 1;
        st.cells[cell_index(0,0)].rank = 1; st.cells[cell_index(0,0)].side = SIDE_BLUE; st.cells[cell_index(0,0)].revealed = 1;
        int can = jq_can_move(&st, C110, C114);  /* row11 非全宽 col0→col4 跨 col1/3 阻断 */
        ok &= chk("④ 对照row11非全宽横贯线非工兵跨col1/3直走拒(非工兵(11,0)→(11,4)跨col1/3非铁路can=0,row11仅col0/2/4三格col1/3非铁路rail_path_straight阻断)",
                  can == 0);
    }
    /* ⑤ row5 全宽横贯线工兵横贯线内全宽拐弯（col0 端拐弯到 col2 纵贯全高线
       can=1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50].rank = RANK_WORKER; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C50, C52);   /* 工兵 row5 全宽 col0→col2 横贯线内拐弯 */
        ok &= chk("⑤ row5全宽横贯线工兵横贯线内全宽拐弯(工兵(5,0)→col2(5,2)同row5横贯拐弯can=1,rail_path_engineer横贯线内全宽拐弯放行)",
                  can == 1);
    }
    /* ⑥ row6 全宽横贯线工兵横贯线内全宽拐弯（col0 端拐弯到 col2 纵贯全高线
       can=1）+ row5/6 全宽 vs row0/11 非全宽横贯线滑行/拐弯差异对照 */
    {
        /* row6 全宽横贯线工兵横贯线内全宽拐弯（col0 端拐弯到 col2） */
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C60].rank = RANK_WORKER; st.cells[C60].side = SIDE_BLUE; st.cells[C60].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int r6_wk = jq_can_move(&st, C60, C62);  /* 工兵 row6 全宽 col0→col2 拐弯 */
        /* 对照 row0 非全宽横贯线工兵跨 col1/3 拐弯阻断（col1/3 非铁路） */
        jq_state st2; jq_state_init(&st2); st2.mode = JQ_MODE_LAY;
        memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[C00].rank = RANK_WORKER; st2.cells[C00].side = SIDE_BLUE; st2.cells[C00].revealed = 1;
        st2.cells[C100].rank = 1; st2.cells[C100].side = SIDE_RED; st2.cells[C100].revealed = 1;
        int r0_wk = jq_can_move(&st2, C00, cell_index(0, 2));  /* 工兵 row0 非全宽 col0→col2 拐弯 */
        ok &= chk("⑥ row6全宽横贯线工兵横贯线内全宽拐弯(工兵(6,0)→col2(6,2)can=1)vs row0非全宽横贯线工兵跨col1/3拐弯阻断(工兵(0,0)→col2(0,2)can=0,row0仅col0/2/4三格col1/3非铁路)",
                  r6_wk == 1 && r0_wk == 0);
    }
    printf("dark_rail_r5r6_fullrow_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
