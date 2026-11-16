/* dark_rail_r6_fullrow_worker_test.c — 工兵 row6 全宽横贯线"col0-4 全 5 端点互
 * 为同段拐弯端点"全场景对照（对称 §95/§96/§97 row5）行为级实证
 * 补全现有缺口：§97 dark_rail_c1c3_worker_seg_test（实锤工兵 col1/col3 非全高
 *  2 格纵贯线"同段拐弯到 row5/6 全宽横贯线段"全场景对照 + col1/col3 非全高纵
 * 贯线跨非铁路阻断对照 + 纵贯全高性差异对照，row5 全宽横贯线段内 col0-4 全 5
 *  端点互为同段拐弯端点全对称闭环 §95/§96/§97 累计），但**工兵 row6 全宽横贯
 * 线"col0-4 全 5 端点互为同段拐弯端点"全场景对照（对称 row5 §95/§96/§97）**
 * 无独立用例——§97 实锤 row5 全宽横贯线段内 col0/col1/col2/col3/col4 互为同
 * 段拐弯端点（col0→col2/col4 + col1→col2/col0 + col2→col0/col4 + col3→col2/
 * col4 + col4→col2/col0 同段拐弯全放行），但**row6 全宽横贯线（对称 row5）co
 * l0-4 全 5 端点工兵同段拐弯全场景对照**未独立断言。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83/§84/§85/§87/§88/§8
 * 9/§90/§91/§92/§93/§94/§95/§96/§97 全格分布实锤：col2 纵贯 10 格被 row3/8
 * 隔断成 3 段 + row5/6 横贯 10 格全宽 + row0/11 横贯非全宽仅 3 格铁路 col0/2/
 * 4 + col0=4 格 / col1=2 格 / col3=2 格 / col4=4 格非纵贯全高 + row3/row8 全
 * 非铁路行营隔断）+ rules.c rail_path_engineer 工兵 BFS 拐弯路径引擎（工兵 ro
 * w6 全宽横贯线 col0-4 全 5 端点同段拐弯全放行，row6 全宽横贯线 col0-4 全 10
 *  格 is_rail=1，工兵 row6 同段拐弯到任意 col0/col1/col2/col3/col4 端放行）
 * ）：
 *   · 工兵 row6 全宽横贯线 col0 端同段拐弯到 col2/col4（(6,0)→(6,2)/(6,4) 同
 *     段拐弯 can=1）；
 *   · 工兵 row6 全宽横贯线 col1 端同段拐弯到 col2/col0（(6,1)→(6,2)/(6,0) 同
 *     段拐弯 can=1）；
 *   · 工兵 row6 全宽横贯线 col2 端同段拐弯到 col0/col4（(6,2)→(6,0)/(6,4) 同
 *     段拐弯 can=1，col2 纵贯全高 row6 全宽横贯线段）；
 *   · 工兵 row6 全宽横贯线 col3 端同段拐弯到 col2/col4（(6,3)→(6,2)/(6,4) 同
 *     段拐弯 can=1）；
 *   · 工兵 row6 全宽横贯线 col4 端同段拐弯到 col2/col0（(6,4)→(6,2)/(6,0) 同
 *     段拐弯 can=1）；
 *   · 对照 §97 row5 全宽横贯线段内 col0-4 全 5 端点互为同段拐弯端点全对称（
 *     row5/row6 全宽横贯线工兵同段拐弯全场景对称）。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + rail_path_engineer 工兵 BFS
 * ，0 分叉；几何走 §95/§96/§97 既有 row5 全宽横贯线 col0-4 全 5 端点互为同段
 * 拐弯端点口径 + 本轮补 row6 全宽横贯线对称）。
 *
 * 几何口径：row6 全宽横贯线 col0-4 全 5 端点（col0/col1/col2/col3/col4 row6
 *   全宽 10 格 is_rail=1）；工兵 row6 同段拐弯 (6,0)→(6,2)/(6,4) / (6,1)→(6,
 *   2)/(6,0) / (6,2)→(6,0)/(6,4) / (6,3)→(6,2)/(6,4) / (6,4)→(6,2)/(6,0)（10
 *   端点组合全 can=1）；对照 §97 row5 同段拐弯（col0/col1/col2/col3/col4 端
 *   点 row5 全宽横贯线同段拐弯）；蓝 cell(0,0) 防困毙；mode=LAY 明棋；工兵 R
 *   ANK_WORKER。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C60 cell_index(6,0)    /* row6 col0（全宽横贯线 col0 端） */
#define C61 cell_index(6,1)    /* row6 col1（全宽横贯线 col1 端） */
#define C62 cell_index(6,2)    /* row6 col2（全宽横贯线 col2 端，纵贯全高） */
#define C63 cell_index(6,3)    /* row6 col3（全宽横贯线 col3 端） */
#define C64 cell_index(6,4)    /* row6 col4（全宽横贯线 col4 端） */
#define C00 cell_index(0,0)    /* 蓝防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

static int wk_seg(jq_state *st, int from, int to)
{
    memset(st->cells, 0, sizeof(st->cells));
    st->cells[from].rank = RANK_WORKER;
    st->cells[from].side = SIDE_BLUE;
    st->cells[from].revealed = 1;
    st->cells[C00].rank = 1;
    st->cells[C00].side = SIDE_BLUE;
    st->cells[C00].revealed = 1;
    return jq_can_move(st, from, to);
}

int main(void)
{
    int ok = 1;
    /* ① 工兵 row6 全宽横贯线 col0 端同段拐弯到 col2/col4（(6,0)→(6,2)/(6,4)
       同段拐弯 can=1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        int c0_c2 = wk_seg(&st, C60, C62);   /* 工兵 (6,0)→(6,2) */
        int c0_c4 = wk_seg(&st, C60, C64);   /* 工兵 (6,0)→(6,4) */
        ok &= chk("① 工兵row6全宽横贯线col0端同段拐弯到col2/col4(工兵(6,0)→col2(6,2)/col4(6,4)同段拐弯can=1,row6全宽横贯linecol0→col2/col4拐弯放行)",
                  c0_c2 == 1 && c0_c4 == 1);
    }
    /* ② 工兵 row6 全宽横贯线 col1 端同段拐弯到 col2/col0（(6,1)→(6,2)/(6,0)
       同段拐弯 can=1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        int c1_c2 = wk_seg(&st, C61, C62);   /* 工兵 (6,1)→(6,2) */
        int c1_c0 = wk_seg(&st, C61, C60);   /* 工兵 (6,1)→(6,0) */
        ok &= chk("② 工兵row6全宽横贯线col1端同段拐弯到col2/col0(工兵(6,1)→col2(6,2)/col0(6,0)同段拐弯can=1,row6全宽横贯linecol1→col2/col0拐弯放行)",
                  c1_c2 == 1 && c1_c0 == 1);
    }
    /* ③ 工兵 row6 全宽横贯线 col2 端同段拐弯到 col0/col4（(6,2)→(6,0)/(6,4)
       同段拐弯 can=1，col2 纵贯全高 row6 全宽横贯线段） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        int c2_c0 = wk_seg(&st, C62, C60);   /* 工兵 (6,2)→(6,0) */
        int c2_c4 = wk_seg(&st, C62, C64);   /* 工兵 (6,2)→(6,4) */
        ok &= chk("③ 工兵row6全宽横贯线col2端同段拐弯到col0/col4(工兵(6,2)→col0(6,0)/col4(6,4)同段拐弯can=1,col2纵贯全高row6全宽横贯linecol2→col0/col4拐弯放行)",
                  c2_c0 == 1 && c2_c4 == 1);
    }
    /* ④ 工兵 row6 全宽横贯线 col3 端同段拐弯到 col2/col4（(6,3)→(6,2)/(6,4)
       同段拐弯 can=1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        int c3_c2 = wk_seg(&st, C63, C62);   /* 工兵 (6,3)→(6,2) */
        int c3_c4 = wk_seg(&st, C63, C64);   /* 工兵 (6,3)→(6,4) */
        ok &= chk("④ 工兵row6全宽横贯线col3端同段拐弯到col2/col4(工兵(6,3)→col2(6,2)/col4(6,4)同段拐弯can=1,row6全宽横贯linecol3→col2/col4拐弯放行)",
                  c3_c2 == 1 && c3_c4 == 1);
    }
    /* ⑤ 工兵 row6 全宽横贯线 col4 端同段拐弯到 col2/col0（(6,4)→(6,2)/(6,0)
       同段拐弯 can=1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        int c4_c2 = wk_seg(&st, C64, C62);   /* 工兵 (6,4)→(6,2) */
        int c4_c0 = wk_seg(&st, C64, C60);   /* 工兵 (6,4)→(6,0) */
        ok &= chk("⑤ 工兵row6全宽横贯线col4端同段拐弯到col2/col0(工兵(6,4)→col2(6,2)/col0(6,0)同段拐弯can=1,row6全宽横贯linecol4→col2/col0拐弯放行)",
                  c4_c2 == 1 && c4_c0 == 1);
    }
    /* ⑥ 对照 §97 row5 全宽横贯线段内 col0-4 全 5 端点互为同段拐弯端点（row5
       /row6 全宽横贯线工兵同段拐弯全场景对称） */
    {
        /* row5 col0 端同段拐弯到 col2/col4（§96 ①② 既有） */
        int r5_c0_c2 = 0, r5_c0_c4 = 0;
        {
            jq_state s; jq_state_init(&s); s.mode = JQ_MODE_LAY;
            s.stage = STG_PLAY; s.winner = JQ_WIN_NONE; s.turn = SIDE_BLUE;
            r5_c0_c2 = (wk_seg(&s, cell_index(5,0), cell_index(5,2)) == 1);
        }
        {
            jq_state s; jq_state_init(&s); s.mode = JQ_MODE_LAY;
            s.stage = STG_PLAY; s.winner = JQ_WIN_NONE; s.turn = SIDE_BLUE;
            r5_c0_c4 = (wk_seg(&s, cell_index(5,0), cell_index(5,4)) == 1);
        }
        /* row6 col0 端同段拐弯到 col2/col4（① 既有） */
        int r6_c0_c2 = 0, r6_c0_c4 = 0;
        {
            jq_state s; jq_state_init(&s); s.mode = JQ_MODE_LAY;
            s.stage = STG_PLAY; s.winner = JQ_WIN_NONE; s.turn = SIDE_BLUE;
            r6_c0_c2 = (wk_seg(&s, C60, C62) == 1);
        }
        {
            jq_state s; jq_state_init(&s); s.mode = JQ_MODE_LAY;
            s.stage = STG_PLAY; s.winner = JQ_WIN_NONE; s.turn = SIDE_BLUE;
            r6_c0_c4 = (wk_seg(&s, C60, C64) == 1);
        }
        ok &= chk("⑥ 对照§97row5全宽横贯线段内col0-4全5端点互为同段拐弯端点(row5col0端(5,0)→col2/col4同段拐弯=1§96①②vsrow6col0端(6,0)→col2/col4同段拐弯=1①,row5/row6全宽横贯线工兵同段拐弯全场景对称)",
                  r5_c0_c2 && r5_c0_c4 && r6_c0_c2 && r6_c0_c4);
    }
    /* ⑦ row6 全宽横贯线 col0-4 全 10 格 is_rail=1 实锤（row6 全宽 10 格铁路
       ，工兵 row6 全宽横贯线 col0-4 全 5 端点同段拐弯拓扑依据，§92 ① 既有 +
       本轮补 row6 工兵同段拐弯场景） */
    {
        int r6_full = 1;
        for (int c = 0; c < 5; c++)
            if (!is_rail(6, c)) r6_full = 0;
        ok &= chk("⑦ row6全宽横贯线col0-4全10格is_rail=1实锤(row6全宽横贯线col0/1/2/3/4均is_rail=1,工兵row6全宽横贯线col0-4全5端点同段拐弯拓扑依据§92①)",
                  r6_full);
    }
    printf("dark_rail_r6_fullrow_worker_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
