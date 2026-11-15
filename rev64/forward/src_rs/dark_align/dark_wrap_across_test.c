/* dark_wrap_across_test.c — 工兵"拐弯绕行跨段可达"（row5/6 中线无隔断场景）+ col2
 * 隔断场景绕行不可达全场景对照行为级实证
 * 补全现有缺口：§82 dark_rail_fullseg_test 实锤 col2 隔断场景工兵"绕行不可达"
 * （col2 被行营隔断 + 隔断双重阻断，工兵拐相邻纵线也到不了目标段），§43
 * dark_rail_topo_test 既有"工兵拐弯绕行跨段可达"口径（无己方子时工兵拐弯到相邻
 * 纵线跨段），但**工兵"拐弯绕行跨段可达"（row5/6 中线无隔断场景）vs "绕行不可
 * 达"（col2 隔断场景）的全场景对照**无独立用例——工兵跨段绕行可达性依赖铁路
 * 拓扑隔断（row5/6 中线无隔断 → 拐 col2 进 row4-7 段可达；col2 被行营隔断 →
 * 拐相邻纵线不可达目标段），需独立断言实锤"可达/不可达"全场景对照。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + 本轮 is_rail 全格分布实
 * 锤：col2 纵贯线被行营 row3/8 隔断成三段 + row5/6 横贯全宽 + row0/11 横贯两端）
 * + is_camp 行营隔断（row3/8 §43 既有）+ rules.c rail_path_engineer 工兵 BFS
 * 拐弯路径引擎（无隔断场景拐弯跨段可达 / 隔断场景绕行不可达））：
 *   · is_rail 全格分布（本轮实锤）：col2 纵贯线 (0,2)(1,2)(2,2) | row3 空 |
 *     (4,2)(5,2)(6,2)(7,2) | row8 空 | (9,2)(10,2)(11,2)；row5/6 横贯全宽
 *     (5,0)-(5,4)/(6,0)-(6,4)；row0/11 横贯两端 (0,0)(0,4)(11,0)(11,4)；
 *   · 工兵拐弯绕行跨段可达（row5/6 中线无隔断）：工兵 @(5,0) 拐弯到 col2
 *     (5,2)/(6,2) 再下行到 row7 (7,2)（跨 row5/6 中线进入 col2 row4-7 段，can=1
 *     ——row5/6 中线无隔断，工兵拐 col2 进 row4-7 段可达，§43 既有口径实锤）；
 *   · 工兵拐弯绕行不可达（col2 隔断场景）：工兵 @(2,2)（col2 row0-2 段）拐相邻
 *     纵线到 row4-7 段（col2 被 row3 隔断 + 行营隔断双重阻断，can=0 ——§82 既有
 *     口径实锤）；
 *   · 全场景对照：工兵跨段绕行可达性 = 铁路拓扑隔断决定（row5/6 中线无隔断 →
 *     拐 col2 进 row4-7 段可达；col2 被行营隔断 → 拐相邻纵线不可达目标段）。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + is_camp 行营隔断 + rail_path_
 * engineer 工兵 BFS 拐弯路径引擎，0 分叉；几何走 §43/§82 既有 col2 纵贯线 +
 * row5/6 中线 + 行营隔断口径）。
 *
 * 几何口径：工兵 @(5,0)（row5 横贯线起点）拐 col2 (5,2)/(6,2) 下行到 row7
 *   (7,2)（row4-7 段，row5/6 中线无隔断拐弯可达）；工兵 @(2,2)（col2 row0-2
 *   段）拐相邻纵线到 row4-7 段（col2 被 row3 隔断 + 行营隔断绕行不可达）；红
 *   cell(10,0) 防困毙；mode=LAY 明棋；工兵 RANK_WORKER。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C50r cell_index(5,0)   /* row5 横贯线起点（工兵拐弯源） */
#define C72 cell_index(7,2)    /* col2 row7（row4-7 段，拐弯目标） */
#define C22 cell_index(2,2)    /* col2 row2（row0-2 段，隔断场景工兵源） */
#define C42 cell_index(4,2)    /* col2 row4（row4-7 段，隔断场景拐弯目标） */
#define C100 cell_index(10,0)  /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 工兵拐弯绕行跨段可达（row5/6 中线无隔断）：工兵 @(5,0) 拐 col2 (5,2)/
       (6,2) 下行到 row7 (7,2) can=1（row5/6 中线无隔断，§43 既有口径实锤） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50r].rank = RANK_WORKER; st.cells[C50r].side = SIDE_BLUE; st.cells[C50r].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C50r, C72);
        ok &= chk("① 工兵拐弯绕行跨段可达(row5/6中线无隔断 (5,0)拐col2到(7,2)can=1)",
                  can == 1);
    }
    /* ② 工兵拐弯绕行不可达（col2 隔断场景，§82 既有口径）：工兵 @(2,2) 拐相邻
       纵线到 row4-7 段 (4,2) can=0（col2 被 row3 隔断 + 行营隔断双重阻断） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C22].rank = RANK_WORKER; st.cells[C22].side = SIDE_BLUE; st.cells[C22].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C22, C42);
        ok &= chk("② 工兵拐弯绕行不可达(col2隔断场景 (2,2)拐相邻纵线到(4,2)can=0§82口径)",
                  can == 0);
    }
    /* ③ 全场景对照：row5/6 中线拐弯可达（①）vs col2 隔断拐弯不可达（②）——工
       兵跨段绕行可达性由铁路拓扑隔断决定 */
    {
        jq_state st1; jq_state_init(&st1); st1.mode = JQ_MODE_LAY;
        memset(&st1.cells, 0, sizeof(st1.cells));
        st1.stage = STG_PLAY; st1.winner = JQ_WIN_NONE; st1.turn = SIDE_BLUE;
        st1.cells[C50r].rank = RANK_WORKER; st1.cells[C50r].side = SIDE_BLUE; st1.cells[C50r].revealed = 1;
        st1.cells[C100].rank = 1; st1.cells[C100].side = SIDE_RED;  st1.cells[C100].revealed = 1;
        int reachable = jq_can_move(&st1, C50r, C72);   /* row5/6 中线拐弯可达 =1 */

        jq_state st2; jq_state_init(&st2); st2.mode = JQ_MODE_LAY;
        memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[C22].rank = RANK_WORKER; st2.cells[C22].side = SIDE_BLUE; st2.cells[C22].revealed = 1;
        st2.cells[C100].rank = 1; st2.cells[C100].side = SIDE_RED;  st2.cells[C100].revealed = 1;
        int blocked = jq_can_move(&st2, C22, C42);      /* col2 隔断拐弯不可达 =0 */

        ok &= chk("③ 全场景对照(row5/6中线拐弯可达=1 vs col2隔断拐弯不可达=0铁路拓扑隔断决定)",
                  reachable == 1 && blocked == 0);
    }
    /* ④ 工兵拐弯绕行同段（row5 横贯线内拐弯）：工兵 @(5,0) 拐到 row6 (6,4)
       can=1（row5/6 中线横贯，同中线拐弯放行） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50r].rank = RANK_WORKER; st.cells[C50r].side = SIDE_BLUE; st.cells[C50r].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C50r, cell_index(6, 4));
        ok &= chk("④ 工兵拐弯同中线横贯((5,0)拐到(6,4)row5/6中线can=1)", can == 1);
    }
    /* ⑤ is_rail 全格分布实锤（col2 隔断三段 + row5/6 横贯 + row0/11 两端，本轮
       诊断口径固化） */
    {
        int c2_seg = 0, c2_total = 0;
        for (int r = 0; r < 12; r++)
            if (is_rail(r, 2)) { c2_total++; if (r == 3 || r == 8) c2_seg++; }
        int row56 = 0;
        for (int c = 0; c < 5; c++) { if (is_rail(5, c)) row56++; if (is_rail(6, c)) row56++; }
        /* col2 纵贯线 10 格（row0/1/2/4/5/6/7/9/10/11，row3/8 行营隔断），row5/6 横贯 10 格 */
        ok &= chk("⑤ is_rail全格分布实锤(col2纵贯10格row3/8隔断+row5/6横贯10格)",
                  c2_total == 10 && c2_seg == 0 && row56 == 10);
    }
    printf("dark_wrap_across_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
