/* dark_rail_c0c4_worker_seg_test.c — 工兵 col0/col4 非全高纵贯线"同段拐弯到
 * row5/6 全宽横贯线"全场景对照 + col2 纵贯全高 row5 全宽横贯线同段拐弯对照行
 * 为级实证
 * 补全现有缺口：§95 dark_rail_c2_worker_seg_test（实锤工兵 col2 纵贯线内"同段
 * 拐弯到 row5/9 全宽横贯线 col0/col1/col3/col4 端 vs 跨段 row3/8 隔断阻断"全
 * 场景对照 + 工兵独有拐弯能力 vs 非工兵 straight 直走差异），但**工兵 col0/
 * col4 非全高纵贯线"同段拐弯到 row5/6 全宽横贯线段"全场景对照 + col0/col4 非
 * 全高纵贯线跨非铁路阻断对照**无独立用例——§95 实锤工兵 col2 纵贯全高 row5/
 * 9 全宽横贯线 col0/col4 端同段拐弯（col2→col0/col4 拐弯），但**工兵 col0/co
 * l4 非全高纵贯线（仅 row0/5/6/11 = 4 格）在 row5/6 全宽横贯线段内同段拐弯到
 * col2 纵贯全高线（col0→col2 / col4→col2 拐弯）vs 跨 row1/2/3/4 非铁路阻断（
 * col0/col4 非全高跨非铁路拒）对照 + col2 纵贯全高 row5 全宽横贯线同段拐弯对
 * 照（col2→col0 同段拐弯，对称 col0→col2）**未独立断言。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83/§84/§85/§87/§88/§8
 * 9/§90/§91/§92/§93/§94/§95 全格分布实锤：col2 纵贯 10 格被 row3/8 隔断成 3
 * 段 + row5/6 横贯 10 格全宽 + row0/11 横贯非全宽仅 3 格铁路 col0/2/4 + col0=
 * 4 格 / col1=2 格 / col3=2 格 / col4=4 格非纵贯全高 + row3/row8 全非铁路行营
 * 隔断）+ rules.c rail_path_engineer 工兵 BFS 拐弯路径引擎（工兵 col0/col4 非
 * 全高纵贯线在 row5/6 全宽横贯线段内同段拐弯到 col2 纵贯全高线放行，col0/col
 * 4 非全高纵贯线跨 row1/2/3/4 非铁路阻断，col2 纵贯全高 row5 全宽横贯线同段
 * 拐弯到 col0/col4 端放行））：
 *   · 工兵 col0 非全高纵贯线在 row5 全宽横贯线段内同段拐弯到 col2/col4（(5,0
 *     )→(5,2) / (5,0)→(5,4) 同段拐弯 can=1，row5 全宽横贯线 col0→col2/col4
 *     拐弯放行）；
 *   · 工兵 col4 非全高纵贯线在 row5 全宽横贯线段内同段拐弯到 col2/col0（(5,4
 *     )→(5,2) / (5,4)→(5,0) 同段拐弯 can=1，row5 全宽横贯线 col4→col2/col0
 *     拐弯放行）；
 *   · 工兵 col0/col4 非全高纵贯线跨 row1/2/3/4 非铁路阻断（(0,0)→(5,0) / (0,
 *     4)→(5,4) 跨 row1/2/3/4 非铁路 can=0，col0/col4 仅 row0/5/6/11 = 4 格非
 *     纵贯全高跨非铁路拒）；
 *   · 对照 col2 纵贯全高 row5 全宽横贯线同段拐弯到 col0/col4（(5,2)→(5,0) /
 *     (5,2)→(5,4) 同段拐弯 can=1，col2 纵贯全高 row5 全宽横贯线 col2→col0/co
 *     l4 拐弯放行，对称 ①②）。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + rail_path_engineer 工兵 BFS
 * ，0 分叉；几何走 §95 既有 col2 纵贯全高 row5/9 全宽横贯线 col0/col4 端同段
 * 拐弯口径 + 本轮补 col0/col4 非全高纵贯线 row5/6 全宽横贯线段内同段拐弯 +
 * col0/col4 跨非铁路阻断对照）。
 *
 * 几何口径：col0/col4 非全高纵贯线（col0/col4 仅 row0/5/6/11 = 4 格铁路，ro
 *   w1/2/3/4/7/8/9/10 非铁路）；工兵 col0/col4 row5 全宽横贯线段 (5,0)/(5,4)
 *    同段拐弯到 col2/col4/col0 端（row5 全宽横贯线 col0-4 全 10 格 is_rail=1
 *   ）；工兵 col0/col4 跨 row1/2/3/4 非铁路 (0,0)→(5,0) / (0,4)→(5,4)（can=
 *   0）；col2 纵贯全高 row5 全宽横贯线段 (5,2) 同段拐弯到 col0/col4 端（对称
 *    §95 ①②）；蓝 cell(0,4)/(0,0) 防困毙；mode=LAY 明棋；工兵 RANK_WORKER。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C50 cell_index(5,0)    /* col0 row5（非全高纵贯线 row5 全宽横贯线段） */
#define C54 cell_index(5,4)    /* col4 row5（非全高纵贯线 row5 全宽横贯线段） */
#define C52 cell_index(5,2)    /* col2 row5（纵贯全高 line row5 全宽横贯线段） */
#define C00 cell_index(0,0)    /* col0 row0（非全高纵贯线底角） */
#define C04 cell_index(0,4)    /* col4 row0（非全高纵贯线底角） */
#define C40 cell_index(4,0)    /* col0 row4（col0 非全高，row4 col0 非铁路） */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 工兵 col0 非全高纵贯线 row5 全宽横贯线段同段拐弯到 col2（(5,0)→(5,2)
       同段拐弯 can=1，row5 全宽横贯线 col0→col2 拐弯） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50].rank = RANK_WORKER; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1;
        st.cells[C04].rank = 1; st.cells[C04].side = SIDE_BLUE; st.cells[C04].revealed = 1;
        int can = jq_can_move(&st, C50, C52);   /* 工兵 col0→col2 row5 全宽横贯同段拐弯 */
        ok &= chk("① 工兵col0非全高纵贯线row5全宽横贯线段同段拐弯到col2(工兵(5,0)→col2(5,2)同段拐弯can=1,row5全宽横贯linecol0→col2拐弯放行)",
                  can == 1);
    }
    /* ② 工兵 col0 非全高纵贯线 row5 全宽横贯线段同段拐弯到 col4（(5,0)→(5,4)
       同段拐弯 can=1，row5 全宽横贯线 col0→col4 拐弯） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50].rank = RANK_WORKER; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1;
        st.cells[C04].rank = 1; st.cells[C04].side = SIDE_BLUE; st.cells[C04].revealed = 1;
        int can = jq_can_move(&st, C50, C54);   /* 工兵 col0→col4 row5 全宽横贯同段拐弯 */
        ok &= chk("② 工兵col0非全高纵贯线row5全宽横贯线段同段拐弯到col4(工兵(5,0)→col4(5,4)同段拐弯can=1,row5全宽横贯linecol0→col4拐弯放行)",
                  can == 1);
    }
    /* ③ 工兵 col0 非全高纵贯线跨 row1/2/3/4 非铁路阻断（(0,0)→(5,0) 跨 row1/
       2/3/4 非铁路 can=0，col0 仅 row0/5/6/11 = 4 格非全高） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C00].rank = RANK_WORKER; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        st.cells[C04].rank = 1; st.cells[C04].side = SIDE_BLUE; st.cells[C04].revealed = 1;
        int can = jq_can_move(&st, C00, C50);   /* 工兵 col0 跨 row1/2/3/4 非铁路 */
        ok &= chk("③ 工兵col0非全高纵贯线跨row1/2/3/4非铁路阻断(工兵(0,0)→(5,0)跨row1/2/3/4非铁路can=0,col0仅row0/5/6/11=4格非纵贯全高)",
                  can == 0);
    }
    /* ④ 工兵 col4 非全高纵贯线 row5 全宽横贯线段同段拐弯到 col2（(5,4)→(5,2)
       同段拐弯 can=1，row5 全宽横贯线 col4→col2 拐弯） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C54].rank = RANK_WORKER; st.cells[C54].side = SIDE_BLUE; st.cells[C54].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C54, C52);   /* 工兵 col4→col2 row5 全宽横贯同段拐弯 */
        ok &= chk("④ 工兵col4非全高纵贯线row5全宽横贯线段同段拐弯到col2(工兵(5,4)→col2(5,2)同段拐弯can=1,row5全宽横贯linecol4→col2拐弯放行)",
                  can == 1);
    }
    /* ⑤ 工兵 col4 非全高纵贯线 row5 全宽横贯线段同段拐弯到 col0（(5,4)→(5,0)
       同段拐弯 can=1，row5 全宽横贯线 col4→col0 拐弯） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C54].rank = RANK_WORKER; st.cells[C54].side = SIDE_BLUE; st.cells[C54].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C54, C50);   /* 工兵 col4→col0 row5 全宽横贯同段拐弯 */
        ok &= chk("⑤ 工兵col4非全高纵贯线row5全宽横贯线段同段拐弯到col0(工兵(5,4)→col0(5,0)同段拐弯can=1,row5全宽横贯linecol4→col0拐弯放行)",
                  can == 1);
    }
    /* ⑥ 工兵 col4 非全高纵贯线跨 row1/2/3/4 非铁路阻断（(0,4)→(5,4) 跨 row1/
       2/3/4 非铁路 can=0，col4 仅 row0/5/6/11 = 4 格非全高） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C04].rank = RANK_WORKER; st.cells[C04].side = SIDE_BLUE; st.cells[C04].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C04, C54);   /* 工兵 col4 跨 row1/2/3/4 非铁路 */
        ok &= chk("⑥ 工兵col4非全高纵贯线跨row1/2/3/4非铁路阻断(工兵(0,4)→(5,4)跨row1/2/3/4非铁路can=0,col4仅row0/5/6/11=4格非纵贯全高)",
                  can == 0);
    }
    /* ⑦ 对照 col2 纵贯全高 row5 全宽横贯线段同段拐弯到 col0/col4（(5,2)→(5,0
       ) / (5,2)→(5,4) 同段拐弯 can=1，col2 纵贯全高 row5 全宽横贯线 col2→col
       0/col4 拐弯，对称 ①④） */
    {
        /* col2→col0 同段拐弯（对称 ① col0→col2） */
        jq_state st1; st1.mode = JQ_MODE_LAY;
        jq_state_init(&st1); memset(&st1.cells, 0, sizeof(st1.cells));
        st1.stage = STG_PLAY; st1.winner = JQ_WIN_NONE; st1.turn = SIDE_BLUE;
        st1.cells[C52].rank = RANK_WORKER; st1.cells[C52].side = SIDE_BLUE; st1.cells[C52].revealed = 1;
        st1.cells[C04].rank = 1; st1.cells[C04].side = SIDE_BLUE; st1.cells[C04].revealed = 1;
        int c2_c0 = jq_can_move(&st1, C52, C50);   /* 工兵 col2→col0 row5 全宽横贯同段拐弯 */
        /* col2→col4 同段拐弯（对称 ④ col4→col2） */
        jq_state st2; st2.mode = JQ_MODE_LAY;
        jq_state_init(&st2); memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[C52].rank = RANK_WORKER; st2.cells[C52].side = SIDE_BLUE; st2.cells[C52].revealed = 1;
        st2.cells[C00].rank = 1; st2.cells[C00].side = SIDE_BLUE; st2.cells[C00].revealed = 1;
        int c2_c4 = jq_can_move(&st2, C52, C54);   /* 工兵 col2→col4 row5 全宽横贯同段拐弯 */
        ok &= chk("⑦ 对照col2纵贯全高row5全宽横贯线段同段拐弯到col0/col4(工兵(5,2)→col0(5,0)/col4(5,4)同段拐弯can=1,col2纵贯全高row5全宽横贯linecol2→col0/col4拐弯对称①④)",
                  c2_c0 == 1 && c2_c4 == 1);
    }
    /* ⑧ is_rail col0/col4 非纵贯全高实锤（col0/col4 仅 row0/5/6/11 = 4 格铁路
       ，工兵 col0/col4 非全高纵贯线 row5/6 全宽横贯线段内同段拐弯 vs 跨 row1/
       2/3/4 非铁路阻断拓扑依据，§84 ⑤ / §91 ⑥ 既有 + 本轮补工兵 col0/col4 非
       全高纵贯线拐弯场景） */
    {
        int c0 = 0, c4 = 0;
        for (int r = 0; r < 12; r++) {
            if (is_rail(r, 0)) c0++;
            if (is_rail(r, 4)) c4++;
        }
        ok &= chk("⑧ is_rail col0/col4非纵贯全高实锤(col0/col4仅row0/5/6/11=4格铁路,工兵col0/col4非全高纵贯线row5/6全宽横贯线同段拐弯vs跨非铁路阻断拓扑依据§84⑤§91⑥)",
                  c0 == 4 && c4 == 4);
    }
    printf("dark_rail_c0c4_worker_seg_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
