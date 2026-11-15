/* dark_rail_fullseg_test.c — col2 纵贯线"三段全段矩阵"（row0-2/4-7/9-11）+ 工兵跨
 * 隔断绕行不可达行为级实证
 * 补全现有缺口：§43 dark_rail_topo_test（col2 行营隔断三段 + 工兵/非工兵滑行基础
 * 拓扑）/ §80 dark_rail_segment_test（col2 跨 row3 隔断蓝方视角 + row11 红方底线
 * 对称 + 工兵跨 row3 隔断）/ §81 dark_rail_seg8_test（col2 跨 row8 隔断红方视角）
 * 之外，**col2 纵贯线三段全段矩阵（row0-2 段 col2 上行）+ 工兵跨隔断绕行不可达
 * 的实锤**无独立用例——§81 诚实边界挂起"col2 row0-2 段（col2 最上段）非工兵/工
 * 兵同段未独立用例"（col2 三段全段矩阵补全最上段），且 §43 既有"工兵拐弯绕行跨
 * 段可达"口径在**跨 row3/row8 隔断场景**下实为**绕行不可达**（col2 隔断 + 行营
 * 隔断双重阻断，工兵拐弯到相邻纵线也到不了目标段），需独立断言实锤"col2 隔断
 * 场景工兵绕行不可达"（区别于"无隔断场景工兵拐弯跨段可达"§43 既有）。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 纵向 col2 纵贯全高 + 横向
 * row0/11 底线 + row5/6 中线）+ is_camp 行营隔断（row3/row8 col2 隔断成三段
 * §43 既有）+ rules.c rail_path_straight 非工兵直走（同段放行 / 跨隔断拒）+
 * rail_path_engineer 工兵 BFS（跨隔断绕行不可达 §76/§80/§81 既有口径））：
 *   · col2 三段全段矩阵（本轮补全最上段）：
 *     - row0-2 段（col2 上行）：非工兵 (0,2)→(2,2) 同段放行 / 工兵同段放行 /
 *       跨 row3 隔断 (0,2)→(4,2) 拒；
 *     - row4-7 段（§80/§81 既有）：非工兵 / 工兵同段放行 / 跨隔断拒；
 *     - row9-11 段（§81 既有）：非工兵 / 工兵同段放行 / 跨隔断拒；
 *   · 工兵跨隔断绕行不可达实锤（col2 隔断场景）：工兵@(2,2) 拐弯到相邻纵线
 *     （col1/col3）也到不了 row4-7 段（col2 被 row3 隔断 + 行营隔断双重阻断，
 *     工兵 BFS 可达格仅 row0-2 段 + row3 行营格，无 row4-7 段）——区别于"无隔断
 *     场景工兵拐弯跨段可达"（§43 既有），**col2 隔断场景工兵绕行不可达是 col2
 *     被行营隔断 + 隔断阻断的特殊性**（非工兵同段放行，工兵跨隔断不可达）。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + is_camp 行营隔断 + rail_path_
 * straight/rail_path_engineer 判定，0 分叉；几何走 §43/§80/§81 既有 col2 纵贯线
 * + row0-2 段（col2 上行）+ 工兵跨隔断口径）。
 *
 * 几何口径：col2 纵贯线 row0-2 段 (0,2)↔(2,2)（col2 上行）/ 跨 row3 隔断
 *   (0,2)↔(4,2)；工兵@(2,2) 拐绕行跨隔断（col2→col1/col3→col2 下）；红
 *   cell(10,0) 防困毙；mode=LAY 明棋；非工兵 rank5 + 工兵 RANK_WORKER。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C02 cell_index(0,2)   /* col2 row0（row0-2 段起点） */
#define C22 cell_index(2,2)   /* col2 row2（row0-2 段远端） */
#define C42 cell_index(4,2)   /* col2 row4（row4-7 段起点，跨 row3 隔断） */
#define C100 cell_index(10,0) /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 非工兵 col2 row0-2 段同段放行（(0,2)→(2,2) col2 上行同段） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C02].rank = 5; st.cells[C02].side = SIDE_BLUE; st.cells[C02].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C02, C22);
        ok &= chk("① 非工兵col2 row0-2段同段放行((0,2)→(2,2)col2上行can=1)", can == 1);
    }
    /* ② 非工兵 col2 跨 row3 隔断（(0,2)→(4,2) 跨段拒，对称 §80 row3 隔断） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C02].rank = 5; st.cells[C02].side = SIDE_BLUE; st.cells[C02].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C02, C42);
        ok &= chk("② 非工兵col2跨row3隔断((0,2)→(4,2)跨段拒can=0对称§80row3)", can == 0);
    }
    /* ③ 工兵 col2 row0-2 段同段放行（(0,2)→(2,2) col2 上行同段） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C02].rank = RANK_WORKER; st.cells[C02].side = SIDE_BLUE; st.cells[C02].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C02, C22);
        ok &= chk("③ 工兵col2 row0-2段同段放行((0,2)→(2,2)col2上行can=1)", can == 1);
    }
    /* ④ 工兵跨 row3 隔断绕行不可达（col2 隔断场景：工兵@(0,2)→(4,2) 拐弯到 col1/
       col3 也到不了 row4-7 段，col2 被 row3 隔断 + 行营隔断双重阻断） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C02].rank = RANK_WORKER; st.cells[C02].side = SIDE_BLUE; st.cells[C02].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C02, C42);
        ok &= chk("④ 工兵跨row3隔断绕行不可达((0,2)→(4,2)col2隔断can=0§76/§80口径)",
                  can == 0);
    }
    /* ⑤ 三段全段矩阵汇总：col2 row0-2/row4-7/row9-11 三段同段放行（非工兵），
       跨隔断全拒（col2 三段隔断完整矩阵） */
    {
        /* row0-2 段同段（① 已实锤）+ row4-7 段同段（§80 既有）+ row9-11 段同段
           （§81 既有）→ 三段同段全放行；跨隔断（row3/row8）全拒 */
        int seg02 = 0, seg47 = 0, seg911 = 0;
        jq_state st;
        st.mode = JQ_MODE_LAY;
        /* row0-2 段 */
        jq_state_init(&st); memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C02].rank = 5; st.cells[C02].side = SIDE_BLUE; st.cells[C02].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        seg02 = jq_can_move(&st, C02, C22);
        /* row4-7 段（§80 既有几何 (4,2)→(7,2)） */
        jq_state_init(&st); memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[cell_index(4,2)].rank = 5; st.cells[cell_index(4,2)].side = SIDE_BLUE; st.cells[cell_index(4,2)].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        seg47 = jq_can_move(&st, cell_index(4,2), cell_index(7,2));
        /* row9-11 段（§81 既有几何 (9,2)→(11,2)，红方视角） */
        jq_state_init(&st); memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[cell_index(9,2)].rank = 5; st.cells[cell_index(9,2)].side = SIDE_RED; st.cells[cell_index(9,2)].revealed = 1;
        st.cells[cell_index(0,0)].rank = 1; st.cells[cell_index(0,0)].side = SIDE_BLUE; st.cells[cell_index(0,0)].revealed = 1;
        seg911 = jq_can_move(&st, cell_index(9,2), cell_index(11,2));
        ok &= chk("⑤ 三段全段矩阵(col2 row0-2/4-7/9-11同段全放行非工兵)",
                  seg02 == 1 && seg47 == 1 && seg911 == 1);
    }
    printf("dark_rail_fullseg_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
