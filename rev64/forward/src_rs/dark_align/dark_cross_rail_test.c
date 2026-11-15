/* dark_cross_rail_test.c — rail_path_straight"整条路径全铁路格"判定中间交叉格阻断
 * 场景行为级实证
 * 补全现有缺口：§77 dark_straight_rail_test 实锤非工兵直走 + 铁路横贯线完整拓扑
 * （row5 全宽贯通滑行 / row0 两端非铁路阻断 / 角格拓扑），但 **rail_path_
 * straight"整条路径全铁路格"判定的中间交叉格阻断场景**（row5 横贯线中间 col2
 * 纵贯线交叉格被己方/敌方子占据时阻断滑行）无独立用例——§77 诚实边界挂起"中
 * 间交叉格阻断非横贯线全宽场景"，本轮实锤中间交叉格（row5∩col2 = (5,2)）被己
 * 方/敌方子占据阻断滑行的完整判定。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 纵向 col2 纵贯全高 + 横向
 * row0/11 底线 + row5/6 中线贯通全宽）+ rules.c rail_path_straight 非工兵直走
 * 判定（"整条路径全铁路格 + 中间格全空"——路径上所有中间格须为铁路格且无子占
 * 据，己方/敌方子占中间格均阻断滑行）+ §77 既有横贯线全宽滑行口径）：
 *   · 中间交叉格空（row5∩col2 = (5,2) 空）：(5,0)→(5,4) 全铁路滑行放行（can=1，
 *     §77 既有 ①）；
 *   · 中间交叉格被己方子占：(5,0)→(5,4) 滑行阻断（can=0，中间格有子阻断"整条
 *     路径全空"）；
 *   · 中间交叉格被敌方子占：(5,0)→(5,4) 滑行阻断（can=0，敌方子占中间格同样
 *     阻断滑行——滑行非吃子，中间格敌方子也拒）；
 *   · col2 纵贯线中间 row5/6 交叉格空：(4,2)→(7,2) 纵向全铁路滑行放行（can=1，
 *     纵向贯通对称）。
 * 64 位行为级实证（走已实现的 rail_path_straight"整条路径全铁路格 + 中间格全空"
 * 判定 + §L68 十字铁路拓扑，0 分叉；几何走 §77 既有 row5/col2 横贯线 + 中线交
 * 叉格 (5,2)）。
 *
 * 几何口径：row5 横贯线 (5,0)↔(5,4)（中线全宽贯通，§77 既有）+ 中间 col2 纵贯
 *   线交叉格 (5,2)（row5∩col2）；col2 纵贯线 (4,2)↔(7,2)（纵向贯通）；红
 *   cell(10,0) 防困毙；mode=LAY 明棋；非工兵 rank5。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C50r cell_index(5,0)   /* 非工兵 row5 横贯线起点 */
#define C54r cell_index(5,4)   /* row5 横贯线远端 */
#define C52r cell_index(5,2)   /* 中间 col2 纵贯线交叉格（row5∩col2） */
#define C42r cell_index(4,2)   /* col2 纵贯线起点 */
#define C72r cell_index(7,2)   /* col2 纵贯线远端 */
#define C100 cell_index(10,0)  /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 中间交叉格空：row5 横贯线 (5,0)→(5,4) 全铁路滑行放行（§77 既有 ①） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50r].rank = 5; st.cells[C50r].side = SIDE_BLUE; st.cells[C50r].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C50r, C54r);
        ok &= chk("① 中间交叉格空row5横贯线全铁路滑行((5,0)→(5,4)can=1§77既有)",
                  can == 1);
    }
    /* ② 中间交叉格被己方子占：(5,0)→(5,4) 滑行阻断（中间格有子阻断"整条全空"） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50r].rank = 5; st.cells[C50r].side = SIDE_BLUE; st.cells[C50r].revealed = 1;
        st.cells[C52r].rank = 3; st.cells[C52r].side = SIDE_BLUE; st.cells[C52r].revealed = 1;  /* 己方占交叉格 */
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C50r, C54r);
        ok &= chk("② 中间col2交叉格被己方子占((5,0)→(5,4)滑行阻断can=0中间格有子)",
                  can == 0);
    }
    /* ③ 中间交叉格被敌方子占：(5,0)→(5,4) 滑行阻断（敌方子占中间格同样拒） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50r].rank = 5; st.cells[C50r].side = SIDE_BLUE; st.cells[C50r].revealed = 1;
        st.cells[C52r].rank = 3; st.cells[C52r].side = SIDE_RED;  st.cells[C52r].revealed = 1;  /* 敌方占交叉格 */
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C50r, C54r);
        ok &= chk("③ 中间col2交叉格被敌方子占((5,0)→(5,4)滑行阻断can=0敌方子同拒)",
                  can == 0);
    }
    /* ④ col2 纵贯线中间 row5/6 交叉格空：(4,2)→(7,2) 纵向全铁路滑行放行 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42r].rank = 5; st.cells[C42r].side = SIDE_BLUE; st.cells[C42r].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C42r, C72r);
        ok &= chk("④ col2纵贯线中间row5/6交叉格空((4,2)→(7,2)纵向全铁路滑行can=1)",
                  can == 1);
    }
    /* ⑤ 对照：col2 纵贯线中间交叉格被己方子占（(4,2)→(7,2) 路径 (5,2)/(6,2)
       己方子）→ 纵向滑行阻断 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42r].rank = 5; st.cells[C42r].side = SIDE_BLUE; st.cells[C42r].revealed = 1;
        st.cells[cell_index(5,2)].rank = 3; st.cells[cell_index(5,2)].side = SIDE_BLUE; st.cells[cell_index(5,2)].revealed = 1;  /* 己方占 (5,2) */
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C42r, C72r);
        ok &= chk("⑤ 对照col2纵贯线中间交叉格被己方子占((4,2)→(7,2)纵向滑行阻断can=0)",
                  can == 0);
    }
    printf("dark_cross_rail_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
