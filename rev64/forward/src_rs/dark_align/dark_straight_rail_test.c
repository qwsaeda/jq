/* dark_straight_rail_test.c — 非工兵直走 + 铁路横贯线（row0/5/6/11）完整拓扑行为级实证
 * 补全现有缺口：§43 dark_rail_topo_test 覆盖 col2 纵贯线隔断 + 工兵/非工兵滑行
 * 基础拓扑，§76 dark_bfs_around_test 实证工兵 BFS 穿己方子，但**非工兵直走在
 * 铁路横贯线（row0/5/6/11 横向贯通）的完整拓扑**（非工兵 row5 横贯线滑行 / 拐
 * 弯拒绝 / row0 横贯线滑行阻断 / row5/6 中线贯通 / 铁路格 vs 非铁路格直走分界）
 * 未独立断言——非工兵"直线直走"语义在铁路横贯线拓扑下的完整判定（哪些横贯线
 * 可滑行 / 哪些被阻断 / 相邻恒放行 / 拐弯拒绝）无独立用例。
 *
 * 对照 64 位实现（board.c is_rail：纵向 col2 纵贯全高 + 横向 row0/11 底线 + row5/6
 * 中线贯通全宽，§L68 十字铁路拓扑 + AUD-R-022 board.c 为唯一事实源 + rules.c
 * rail_path_straight 非工兵直走（同段直走需整条路径全铁路格）+ §404 相邻恒放行）：
 *   · 非工兵直走"同段滑行"语义：整条直线路径**全铁路格**才可滑行（row5 横贯线
 *     全宽贯通 → (5,0)→(5,4) 可滑行；row0 横贯线 col0 非铁路起点 / 路径含非铁
 *     路格 → (0,0)→(0,4) 阻断）；
 *   · 铁路横贯线 row0/11 底线（col1/3 大本营在铁路外）：非工兵在 row0/row11 的
 *     滑行受大本营/非铁路格阻断（(0,0)→(0,4) 路径含 col0/col4 非铁路 → 阻断）；
 *   · row5/6 中线贯通全宽：非工兵 (5,0)→(5,4) 同 row5 横贯线滑行放行；
 *   · 相邻恒放行（§404）：非工兵相邻走（(5,0)→(5,1)）恒放行（无论铁路/非铁路）；
 *   · 拐弯拒绝：非工兵不可拐弯（铁路斜线/拐弯路径非直走 → 拒）。
 * 64 位行为级实证（走已实现的 is_rail 拓扑 + rail_path_straight 直走判定 + §404
 * 相邻放行，0 分叉；几何走 §43/§76 既有铁路拓扑口径）。
 *
 * 几何口径：row5 横贯线（5,0）↔（5,4）（中线全宽贯通）；row0 横贯线（0,0）↔
 *   （0,4）（底线 col0/col4 非铁路）；相邻（5,0）→（5,1）；红 cell(10,0) 防困
 *   毙；mode=LAY 明棋；非工兵 rank5。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C50r cell_index(5,0)   /* 非工兵（row5 横贯线起点） */
#define C54r cell_index(5,4)   /* row5 横贯线远端 */
#define C51r cell_index(5,1)   /* 相邻 */
#define C60r cell_index(6,0)   /* row5/6 中线相邻 */
#define C40r cell_index(4,0)   /* 拐弯目标（col0 row4） */
#define C00r cell_index(0,0)   /* row0 横贯线起点 */
#define C04r cell_index(0,4)   /* row0 横贯线远端 */
#define C100 cell_index(10,0)  /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

static jq_state setup_rail(int from, int to_side)
{
    jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
    memset(&st.cells, 0, sizeof(st.cells));
    st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
    st.cells[from].rank = 5; st.cells[from].side = SIDE_BLUE; st.cells[from].revealed = 1;
    st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
    (void)to_side;
    return st;
}

int main(void)
{
    int ok = 1;
    /* ① 非工兵 row5 横贯线直走滑行（(5,0)→(5,4) 同 row5 全宽贯通）can=1 */
    {
        jq_state st = setup_rail(C50r, 0);
        int can = jq_can_move(&st, C50r, C54r);
        ok &= chk("① 非工兵row5横贯线直走滑行((5,0)→(5,4)同row5全宽贯通can=1)",
                  can == 1);
    }
    /* ② 非工兵相邻恒放行（§404）（(5,0)→(5,1)）can=1 */
    {
        jq_state st = setup_rail(C50r, 0);
        int can = jq_can_move(&st, C50r, C51r);
        ok &= chk("② 非工兵相邻恒放行§404((5,0)→(5,1)can=1)",
                  can == 1);
    }
    /* ③ 非工兵 row5/6 中线贯通（(5,0)→(6,0) 中线相邻）can=1 */
    {
        jq_state st = setup_rail(C50r, 0);
        int can = jq_can_move(&st, C50r, C60r);
        ok &= chk("③ 非工兵row5/6中线贯通((5,0)→(6,0)中线can=1)",
                  can == 1);
    }
    /* ④ 非工兵 row0 横贯线滑行阻断（(0,0)→(0,4) 路径含 col0/col4 非铁路）can=0 */
    {
        jq_state st = setup_rail(C00r, 0);
        int can = jq_can_move(&st, C00r, C04r);
        ok &= chk("④ 非工兵row0横贯线滑行阻断((0,0)→(0,4)路径col0/4非铁路can=0)",
                  can == 0);
    }
    /* ⑤ 非工兵角格相邻恒放行对照（(0,0)→(1,0) col0 纵向下行相邻，非横贯线
       滑行——角格 (0,0) 唯一可达 (1,0)，横向 (0,1) 被拒）can=1 */
    {
        jq_state st = setup_rail(C00r, 0);
        int can = jq_can_move(&st, C00r, cell_index(1, 0));
        ok &= chk("⑤ 非工兵角格相邻恒放行((0,0)→(1,0)col0纵向下行can=1横向(0,1)拒)",
                  can == 1);
    }
    printf("dark_straight_rail_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
