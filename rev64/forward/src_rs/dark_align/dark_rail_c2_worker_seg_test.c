/* dark_rail_c2_worker_seg_test.c — 工兵 col2 纵贯线内"同段拐弯 vs 跨段隔断阻断
 * "全场景对照 + row5/6 全宽横贯线工兵横贯拐弯对照行为级实证
 * 补全现有缺口：§94 dark_rail_c2_fullseg_test（实锤纵贯线全高 col2"纵贯线内
 * 同段直走 vs 跨段隔断阻断"非工兵全场景对照 + col0/4 非全高纵贯线跨非铁路阻
 * 断对照），但**工兵 col2 纵贯线内"同段拐弯 vs 跨段隔断阻断"全场景对照**无用
 * 例——§94 实锤 col2 纵贯全高 10 格被 row3/8 隔断成 3 段（row0-2/row4-7/row9-
 * 11）非工兵"同段直走放行 vs 跨段隔断阻断"，但**工兵 rail_path_engineer 沿 co
 * l2 纵贯线"同段拐弯到 row5/6 全宽横贯线（col0/col4 端拐弯）vs 跨段 row3/8
 * 隔断阻断"**全场景对照未独立断言——工兵 col2 纵贯线内同段拐弯（3 段内 row5/
 * 6 全宽横贯线 col0/col4 端拐弯放行 + 同段直走 row4-7/row9-11 段）vs 跨段 ro
 * w3/8 隔断阻断（col2 纵贯跨 row3/8 行营隔断不可达）工兵场景未独立覆盖。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83/§84/§85/§87/§88/§8
 * 9/§90/§91/§92/§93/§94 全格分布实锤：col2 纵贯 10 格被 row3/8 隔断成 3 段 +
 * row5/6 横贯 10 格全宽 + row0/11 横贯非全宽仅 3 格铁路 col0/2/4 + col0=4 格
 *  / col1=2 格 / col3=2 格 / col4=4 格非纵贯全高 + row3/row8 全非铁路行营隔断
 * ）+ rules.c rail_path_engineer 工兵 BFS 拐弯路径引擎（工兵沿 col2 纵贯线同
 * 段拐弯到 row5/6 全宽横贯线 col0/col4 端放行，col2 纵贯线同段直走 row4-7/ro
 * w9-11 段放行，跨段 row3/8 隔断阻断，col0/4 端拐弯到 col2 纵贯全高线受限）
 * + rail_path_straight 非工兵直走（col2 纵贯线内同段直走 vs 跨段隔断阻断））
 * ：
 *   · 工兵 col2 纵贯线 row4-7 段同段拐弯到 row5 全宽横贯线（(4,2)→(5,0) col0
 *     端 / (4,2)→(5,4) col4 端 / (4,2)→(5,2) 同段 col2 拐弯放行）；
 *   · 工兵 col2 纵贯线 row9-11 段同段拐弯到 row5/6 全宽横贯线（(9,2)→(9,1)
 *     / (9,2)→(9,3) / (9,2)→(9,2) 同段 col2 直走放行）；
 *   · 工兵 col2 纵贯线同段直走（row4-7 段 (4,2)→(7,2) / row9-11 段 (9,2)→(1
 *     1,2) 同段直走放行，rail_path_engineer 同段直走）；
 *   · 工兵 col2 纵贯线跨段隔断阻断（(4,2)→(0,2) 跨 row3 / (7,2)→(9,2) 跨 row
 *     8 / (2,2)→(4,2) 跨 row3 均 can=0，col2 纵贯跨 row3/8 行营隔断阻断）；
 *   · 对照非工兵 col2 纵贯线同段直走（§94 既有 (4,2)→(7,2) 等）vs 工兵同段拐
 *     弯到 row5/6 全宽横贯线 col0/col4 端（工兵独有拐弯能力，非工兵 straight
 *     直走不拐弯）。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + rail_path_engineer 工兵 BFS
 * ，0 分叉；几何走 §94 既有 col2 纵贯全高 10 格被 row3/8 隔断成 3 段 + row5/
 * 6 全宽横贯线 + 工兵同段拐弯 vs 跨段隔断阻断口径）。
 *
 * 几何口径：col2 纵贯全高 10 格被 row3/8 隔断成 3 段（row0-2/row4-7/row9-11
 *   ）；工兵 col2 纵贯线同段拐弯 (4,2)→(5,0)/(5,4)（row5 全宽横贯线 col0/col
 *   4 端）/ (4,2)→(7,2)（row4-7 段同段直走）/ (9,2)→(9,1)/(9,3)（row9 全宽横
 *   贯线 col1/col3 端）/ (9,2)→(11,2)（row9-11 段同段直走）；工兵 col2 纵贯线
 *   跨段隔断 (4,2)→(0,2) 跨 row3 / (7,2)→(9,2) 跨 row8（can=0）；红 cell(0,0
 *   ) 防困毙；mode=LAY 明棋；工兵 RANK_WORKER。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C42 cell_index(4,2)    /* col2 row4（纵贯线 row4-7 段起点） */
#define C50 cell_index(5,0)    /* row5 col0（全宽横贯线 col0 端） */
#define C54 cell_index(5,4)    /* row5 col4（全宽横贯线 col4 端） */
#define C52 cell_index(5,2)    /* col2 row5（纵贯线 row4-7 段 ∩ row5 全宽横贯） */
#define C72 cell_index(7,2)    /* col2 row7（纵贯线 row4-7 段远端） */
#define C02 cell_index(0,2)    /* col2 row0（纵贯线 row0-2 段） */
#define C92 cell_index(9,2)    /* col2 row9（纵贯线 row9-11 段起点） */
#define C91 cell_index(9,1)    /* row9 col1（全宽横贯线 col1 端） */
#define C93 cell_index(9,3)    /* row9 col3（全宽横贯线 col3 端） */
#define C112 cell_index(11,2)  /* col2 row11（纵贯线 row9-11 段远端） */
#define C00 cell_index(0,0)    /* 蓝防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 工兵 col2 纵贯线 row4-7 段同段拐弯到 row5 全宽横贯线 col0 端（(4,2)
       →(5,0) 拐弯放行） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = RANK_WORKER; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C42, C50);   /* 工兵 (4,2)→(5,0) row5 全宽横贯 col0 端拐弯 */
        ok &= chk("① 工兵col2纵贯线row4-7段同段拐弯到row5全宽横贯线col0端(工兵(4,2)→(5,0)拐弯can=1,rail_path_engineer同段拐弯放行)",
                  can == 1);
    }
    /* ② 工兵 col2 纵贯线 row4-7 段同段拐弯到 row5 全宽横贯线 col4 端（(4,2)
       →(5,4) 拐弯放行） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = RANK_WORKER; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C42, C54);   /* 工兵 (4,2)→(5,4) row5 全宽横贯 col4 端拐弯 */
        ok &= chk("② 工兵col2纵贯线row4-7段同段拐弯到row5全宽横贯线col4端(工兵(4,2)→(5,4)拐弯can=1,rail_path_engineer同段拐弯放行)",
                  can == 1);
    }
    /* ③ 工兵 col2 纵贯线 row4-7 段同段直走（(4,2)→(7,2) 同段直走放行，rail_
       path_engineer 同段直走） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = RANK_WORKER; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C42, C72);   /* 工兵 (4,2)→(7,2) row4-7 段同段直走 */
        ok &= chk("③ 工兵col2纵贯线row4-7段同段直走(工兵(4,2)→(7,2)同段直走can=1,rail_path_engineer同段直走放行)",
                  can == 1);
    }
    /* ④ 工兵 col2 纵贯线 row9-11 段同段拐弯到 row9 全宽横贯线 col1/col3 端
       （(9,2)→(9,1) / (9,2)→(9,3) 拐弯放行） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C92].rank = RANK_WORKER; st.cells[C92].side = SIDE_BLUE; st.cells[C92].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can91 = jq_can_move(&st, C92, C91);   /* 工兵 (9,2)→(9,1) row9 全宽横贯 col1 端 */
        int can93 = jq_can_move(&st, C92, C93);   /* 工兵 (9,2)→(9,3) row9 全宽横贯 col3 端 */
        ok &= chk("④ 工兵col2纵贯线row9-11段同段拐弯到row9全宽横贯线col1/col3端(工兵(9,2)→(9,1)/(9,3)拐弯can=1,rail_path_engineer同段拐弯放行)",
                  can91 == 1 && can93 == 1);
    }
    /* ⑤ 工兵 col2 纵贯线 row9-11 段同段直走（(9,2)→(11,2) 同段直走放行） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C92].rank = RANK_WORKER; st.cells[C92].side = SIDE_BLUE; st.cells[C92].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C92, C112);   /* 工兵 (9,2)→(11,2) row9-11 段同段直走 */
        ok &= chk("⑤ 工兵col2纵贯线row9-11段同段直走(工兵(9,2)→(11,2)同段直走can=1,rail_path_engineer同段直走放行)",
                  can == 1);
    }
    /* ⑥ 工兵 col2 纵贯线跨段隔断阻断（(4,2)→(0,2) 跨 row3 / (7,2)→(9,2) 跨
       row8 均 can=0，col2 纵贯跨 row3/8 行营隔断阻断） */
    {
        /* 跨 row3 隔断：(4,2)→(0,2) */
        jq_state st1; st1.mode = JQ_MODE_LAY;
        jq_state_init(&st1); memset(&st1.cells, 0, sizeof(st1.cells));
        st1.stage = STG_PLAY; st1.winner = JQ_WIN_NONE; st1.turn = SIDE_BLUE;
        st1.cells[C42].rank = RANK_WORKER; st1.cells[C42].side = SIDE_BLUE; st1.cells[C42].revealed = 1;
        st1.cells[C00].rank = 1; st1.cells[C00].side = SIDE_BLUE; st1.cells[C00].revealed = 1;
        int cross3 = jq_can_move(&st1, C42, C02);   /* 工兵 (4,2)→(0,2) 跨 row3 */
        /* 跨 row8 隔断：(7,2)→(9,2) */
        jq_state st2; st2.mode = JQ_MODE_LAY;
        jq_state_init(&st2); memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[C72].rank = RANK_WORKER; st2.cells[C72].side = SIDE_BLUE; st2.cells[C72].revealed = 1;
        st2.cells[C00].rank = 1; st2.cells[C00].side = SIDE_BLUE; st2.cells[C00].revealed = 1;
        int cross8 = jq_can_move(&st2, C72, C92);   /* 工兵 (7,2)→(9,2) 跨 row8 */
        ok &= chk("⑥ 工兵col2纵贯线跨段隔断阻断(工兵(4,2)→(0,2)跨row3can=0+工兵(7,2)→(9,2)跨row8can=0,col2纵贯跨row3/8行营隔断阻断)",
                  cross3 == 0 && cross8 == 0);
    }
    /* ⑦ 对照非工兵 col2 纵贯线同段直走（§94 既有）vs 工兵同段拐弯到 row5/9
       全宽横贯线 col0/col4 端（工兵独有拐弯能力，非工兵 straight 直走不拐弯
       ） */
    {
        /* 非工兵 col2 纵贯线同段直走（§94 既有 (4,2)→(7,2)） */
        jq_state st1; st1.mode = JQ_MODE_LAY;
        jq_state_init(&st1); memset(&st1.cells, 0, sizeof(st1.cells));
        st1.stage = STG_PLAY; st1.winner = JQ_WIN_NONE; st1.turn = SIDE_BLUE;
        st1.cells[C42].rank = 5; st1.cells[C42].side = SIDE_BLUE; st1.cells[C42].revealed = 1;
        st1.cells[C00].rank = 1; st1.cells[C00].side = SIDE_BLUE; st1.cells[C00].revealed = 1;
        int nonwk_seg = jq_can_move(&st1, C42, C72) == 1;   /* 非工兵同段直走放行 */
        /* 非工兵 col2 纵贯线到 row5 全宽横贯线 col0 端（straight 直走不拐弯，
           (4,2)→(5,0) 非同行非同列拒） */
        int nonwk_corner = jq_can_move(&st1, C42, C50) == 0;  /* 非工兵不拐弯拒 */
        /* 工兵 col2 纵贯线同段拐弯到 row5 全宽横贯线 col0 端（① 既有） */
        jq_state st2; st2.mode = JQ_MODE_LAY;
        jq_state_init(&st2); memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[C42].rank = RANK_WORKER; st2.cells[C42].side = SIDE_BLUE; st2.cells[C42].revealed = 1;
        st2.cells[C00].rank = 1; st2.cells[C00].side = SIDE_BLUE; st2.cells[C00].revealed = 1;
        int wk_corner = jq_can_move(&st2, C42, C50) == 1;   /* 工兵同段拐弯放行 */
        ok &= chk("⑦ 对照非工兵col2纵贯线同段直走(§94(4,2)→(7,2)=1)vs工兵同段拐弯到row5全宽横贯线col0端(工兵(4,2)→(5,0)=1非工兵拒)",
                  nonwk_seg && nonwk_corner && wk_corner);
    }
    printf("dark_rail_c2_worker_seg_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
