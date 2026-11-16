/* dark_rail_c1c3_worker_seg_test.c — 工兵 col1/col3 非全高纵贯线（row5/6 = 2
 * 格）"同段拐弯到 row5/6 全宽横贯线段"全场景对照 + 跨非铁路阻断对照行为级实
 * 证（对称 §96 col0/col4 非全高 4 格）
 * 补全现有缺口：§96 dark_rail_c0c4_worker_seg_test（实锤工兵 col0/col4 非全高
 * 纵贯线"同段拐弯到 row5/6 全宽横贯线段"全场景对照 + col0/col4 非全高纵贯线跨
 * 非铁路阻断对照 + col2 纵贯全高 row5 全宽横贯线同段拐弯对照，row5 全宽横贯
 * 线段内 col0/col2/col4 互为同段拐弯端点全对称闭环），但**工兵 col1/col3 非全
 * 高纵贯线（仅 row5/6 = 2 格）"同段拐弯到 row5/6 全宽横贯线段"全场景对照 +
 * col1/col3 非全高纵贯线跨非铁路阻断对照**无独立用例——§96 实锤 col0/col4 非
 * 全高 4 格纵贯线 row5 全宽横贯线段内同段拐弯（col0/col4→col2/col0/col4 同段
 * 拐弯）vs 跨非铁路阻断，但**col1/col3 非全高 2 格纵贯线（col1/col3 仅 row5/
 * 6 = 2 格铁路）在 row5/6 全宽横贯线段内同段拐弯到 col2/col0/col4 端（col1→c
 * ol2/col0 + col3→col2/col4 同段拐弯）vs 跨 row0-4/row7-11 非铁路阻断（col1/
 * col3 仅 2 格铁路跨非铁路拒）对照**未独立断言——col1/col3 非全高 2 格 vs col
 * 0/col4 非全高 4 格同段拐弯差异由纵贯全高性决定未独立实锤。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83/§84/§85/§87/§88/§8
 * 9/§90/§91/§92/§93/§94/§95/§96 全格分布实锤：col2 纵贯 10 格被 row3/8 隔断
 * 成 3 段 + row5/6 横贯 10 格全宽 + row0/11 横贯非全宽仅 3 格铁路 col0/2/4 +
 * col0=4 格 / col1=2 格 / col3=2 格 / col4=4 格非纵贯全高 + row3/row8 全非铁
 * 路行营隔断）+ rules.c rail_path_engineer 工兵 BFS 拐弯路径引擎（工兵 col1/
 * col3 非全高纵贯线在 row5/6 全宽横贯线段内同段拐弯到 col2/col0/col4 端放行
 * ，col1/col3 非全高纵贯线跨 row0-4/row7-11 非铁路阻断，col2 纵贯全高 row5
 * 全宽横贯线同段拐弯到 col1/col3 端放行））：
 *   · 工兵 col1 非全高纵贯线在 row5 全宽横贯线段内同段拐弯到 col2/col0（(5,1
 *     )→(5,2)/(5,0) 同段拐弯 can=1，row5 全宽横贯线 col1→col2/col0 拐弯放行
 *     ）；
 *   · 工兵 col3 非全高纵贯线在 row5 全宽横贯线段内同段拐弯到 col2/col4（(5,3
 *     )→(5,2)/(5,4) 同段拐弯 can=1，row5 全宽横贯线 col3→col2/col4 拐弯放行
 *     ）；
 *   · 工兵 col1/col3 非全高纵贯线跨 row0-4/row7-11 非铁路阻断（(5,1)→(0,1)/
 *     (9,1) / (5,3)→(0,3)/(9,3) 跨 row0-4/row7-11 非铁路 can=0，col1/col3 仅
 *     row5/6 = 2 格非纵贯全高跨非铁路拒）；
 *   · 对照 §96 col0/col4 非全高 4 格纵贯线 row5 全宽横贯线段内同段拐弯（col0
 *     /col4→col2/col0/col4 同段拐弯）——col1/col3 非全高 2 格 vs col0/col4 非
 *     全高 4 格同段拐弯差异由纵贯全高性决定（col1/col3 仅 2 格铁路 row5/6，
 *     col0/col4 4 格铁路 row0/5/6/11）。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + rail_path_engineer 工兵 BFS
 * ，0 分叉；几何走 §96 既有 col0/col4 非全高纵贯线 row5/6 全宽横贯线段内同段
 * 拐弯口径 + 本轮补 col1/col3 非全高 2 格纵贯线 row5/6 全宽横贯线段内同段拐
 * 弯 + col1/col3 跨非铁路阻断对照）。
 *
 * 几何口径：col1/col3 非全高纵贯线（col1/col3 仅 row5/6 = 2 格铁路，row0-4/r
 *   ow7-11 非铁路）；工兵 col1/col3 row5 全宽横贯线段 (5,1)/(5,3) 同段拐弯到
 *    col2/col0/col4 端（row5 全宽横贯线 col0-4 全 10 格 is_rail=1）；工兵 co
 *   l1/col3 跨 row0-4/row7-11 非铁路 (5,1)→(0,1)/(9,1) / (5,3)→(0,3)/(9,3)（
 *   can=0）；蓝 cell(0,0) 防困毙；mode=LAY 明棋；工兵 RANK_WORKER。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C51 cell_index(5,1)    /* col1 row5（非全高纵贯线 row5 全宽横贯线段） */
#define C53 cell_index(5,3)    /* col3 row5（非全高纵贯线 row5 全宽横贯线段） */
#define C50 cell_index(5,0)    /* col0 row5（row5 全宽横贯线 col0 端） */
#define C52 cell_index(5,2)    /* col2 row5（纵贯全高 line row5 全宽横贯线） */
#define C54 cell_index(5,4)    /* col4 row5（row5 全宽横贯线 col4 端） */
#define C01 cell_index(0,1)    /* col1 row0（col1 非全高，row0 col1 非铁路） */
#define C91 cell_index(9,1)    /* col1 row9（col1 非全高，row9 col1 非铁路） */
#define C03 cell_index(0,3)    /* col3 row0（col3 非全高，row0 col3 非铁路） */
#define C93 cell_index(9,3)    /* col3 row9（col3 非全高，row9 col3 非铁路） */
#define C00 cell_index(0,0)    /* 蓝防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 工兵 col1 非全高纵贯线 row5 全宽横贯线段同段拐弯到 col2（(5,1)→(5,2)
       同段拐弯 can=1，row5 全宽横贯线 col1→col2 拐弯） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C51, C52);   /* 工兵 col1→col2 row5 全宽横贯同段拐弯 */
        ok &= chk("① 工兵col1非全高纵贯线row5全宽横贯线段同段拐弯到col2(工兵(5,1)→col2(5,2)同段拐弯can=1,row5全宽横贯linecol1→col2拐弯放行)",
                  can == 1);
    }
    /* ② 工兵 col1 非全高纵贯线 row5 全宽横贯线段同段拐弯到 col0（(5,1)→(5,0)
       同段拐弯 can=1，row5 全宽横贯线 col1→col0 拐弯） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C51, C50);   /* 工兵 col1→col0 row5 全宽横贯同段拐弯 */
        ok &= chk("② 工兵col1非全高纵贯线row5全宽横贯线段同段拐弯到col0(工兵(5,1)→col0(5,0)同段拐弯can=1,row5全宽横贯linecol1→col0拐弯放行)",
                  can == 1);
    }
    /* ③ 工兵 col1 非全高纵贯线跨 row0-4/row7-11 非铁路阻断（(5,1)→(0,1) 跨 ro
       w0-4 / (5,1)→(9,1) 跨 row7-11 非铁路 can=0，col1 仅 row5/6 = 2 格非全
       高） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int cross04 = jq_can_move(&st, C51, C01);   /* 工兵 col1 跨 row0-4 非铁路 */
        int cross711 = jq_can_move(&st, C51, C91);  /* 工兵 col1 跨 row7-11 非铁路 */
        ok &= chk("③ 工兵col1非全高纵贯线跨row0-4/row7-11非铁路阻断(工兵(5,1)→(0,1)跨row0-4/ (5,1)→(9,1)跨row7-11非铁路can=0,col1仅row5/6=2格非纵贯全高)",
                  cross04 == 0 && cross711 == 0);
    }
    /* ④ 工兵 col3 非全高纵贯线 row5 全宽横贯线段同段拐弯到 col2（(5,3)→(5,2)
       同段拐弯 can=1，row5 全宽横贯线 col3→col2 拐弯） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C53].rank = RANK_WORKER; st.cells[C53].side = SIDE_BLUE; st.cells[C53].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C53, C52);   /* 工兵 col3→col2 row5 全宽横贯同段拐弯 */
        ok &= chk("④ 工兵col3非全高纵贯线row5全宽横贯线段同段拐弯到col2(工兵(5,3)→col2(5,2)同段拐弯can=1,row5全宽横贯linecol3→col2拐弯放行)",
                  can == 1);
    }
    /* ⑤ 工兵 col3 非全高纵贯线 row5 全宽横贯线段同段拐弯到 col4（(5,3)→(5,4)
       同段拐弯 can=1，row5 全宽横贯线 col3→col4 拐弯） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C53].rank = RANK_WORKER; st.cells[C53].side = SIDE_BLUE; st.cells[C53].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C53, C54);   /* 工兵 col3→col4 row5 全宽横贯同段拐弯 */
        ok &= chk("⑤ 工兵col3非全高纵贯线row5全宽横贯线段同段拐弯到col4(工兵(5,3)→col4(5,4)同段拐弯can=1,row5全宽横贯linecol3→col4拐弯放行)",
                  can == 1);
    }
    /* ⑥ 工兵 col3 非全高纵贯线跨 row0-4/row7-11 非铁路阻断（(5,3)→(0,3) 跨 ro
       w0-4 / (5,3)→(9,3) 跨 row7-11 非铁路 can=0，col3 仅 row5/6 = 2 格非全
       高） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C53].rank = RANK_WORKER; st.cells[C53].side = SIDE_BLUE; st.cells[C53].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int cross04 = jq_can_move(&st, C53, C03);   /* 工兵 col3 跨 row0-4 非铁路 */
        int cross711 = jq_can_move(&st, C53, C93);  /* 工兵 col3 跨 row7-11 非铁路 */
        ok &= chk("⑥ 工兵col3非全高纵贯线跨row0-4/row7-11非铁路阻断(工兵(5,3)→(0,3)跨row0-4/ (5,3)→(9,3)跨row7-11非铁路can=0,col3仅row5/6=2格非纵贯全高)",
                  cross04 == 0 && cross711 == 0);
    }
    /* ⑦ 对照 §96 col0/col4 非全高 4 格纵贯线 row5 全宽横贯线段内同段拐弯 vs
       本轮 col1/col3 非全高 2 格纵贯线同段拐弯差异（col0/col4 4 格 vs col1/co
       l3 2 格纵贯全高性差异） */
    {
        /* col0 非全高 4 格纵贯线 row5 全宽横贯线段同段拐弯到 col2（§96 ① 既有
           ） */
        jq_state st1; st1.mode = JQ_MODE_LAY;
        jq_state_init(&st1); memset(&st1.cells, 0, sizeof(st1.cells));
        st1.stage = STG_PLAY; st1.winner = JQ_WIN_NONE; st1.turn = SIDE_BLUE;
        st1.cells[C50].rank = RANK_WORKER; st1.cells[C50].side = SIDE_BLUE; st1.cells[C50].revealed = 1;
        st1.cells[C00].rank = 1; st1.cells[C00].side = SIDE_BLUE; st1.cells[C00].revealed = 1;
        int c0_c2 = jq_can_move(&st1, C50, C52) == 1;   /* col0→col2 同段拐弯放行 */
        /* col1 非全高 2 格纵贯线 row5 全宽横贯线段同段拐弯到 col2（① 既有） */
        jq_state st2; st2.mode = JQ_MODE_LAY;
        jq_state_init(&st2); memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[C51].rank = RANK_WORKER; st2.cells[C51].side = SIDE_BLUE; st2.cells[C51].revealed = 1;
        st2.cells[C00].rank = 1; st2.cells[C00].side = SIDE_BLUE; st2.cells[C00].revealed = 1;
        int c1_c2 = jq_can_move(&st2, C51, C52) == 1;   /* col1→col2 同段拐弯放行 */
        ok &= chk("⑦ 对照§96col0/col4非全高4格vs本轮col1/col3非全高2格纵贯线row5全宽横贯线段内同段拐弯(col0→col2放行§96①+col1→col2放行①,纵贯全高性差异col0/col4=4格col1/col3=2格)",
                  c0_c2 && c1_c2);
    }
    /* ⑧ is_rail col1/col3 非纵贯全高实锤（col1/col3 仅 row5/6 = 2 格铁路，工
       兵 col1/col3 非全高纵贯线 row5/6 全宽横贯线同段拐弯 vs 跨非铁路阻断拓扑
       依据 §84 ⑤ / §89 ⑥ 既有 + 本轮补工兵 col1/col3 非全高 2 格纵贯线拐弯场
       景） */
    {
        int c1 = 0, c3 = 0;
        for (int r = 0; r < 12; r++) {
            if (is_rail(r, 1)) c1++;
            if (is_rail(r, 3)) c3++;
        }
        ok &= chk("⑧ is_rail col1/col3非纵贯全高实锤(col1/col3仅row5/6=2格铁路,工兵col1/col3非全高纵贯线row5/6全宽横贯线同段拐弯vs跨非铁路阻断拓扑依据§84⑤§89⑥)",
                  c1 == 2 && c3 == 2);
    }
    printf("dark_rail_c1c3_worker_seg_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
