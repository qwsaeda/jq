/* dark_flip_color_test.c — 暗棋先手翻子定色 + 翻棋回合交替 + 0xFF 全定色中间态行为级实证
 * 补全现有缺口：§33 dark_fullgame / dark_fullgame_flip_end 跑完整对局（翻棋+走子
 * 交替到终局），§30 dark_flip_test 覆盖翻子基本语义，但**"先手翻子定色（蓝先手
 * 翻→定蓝）+ 翻棋回合交替（每次翻子定归翻子方 + turn 交替）+ 0xFF 全定色中间态"
 * 未独立断言**（§B01-059 定色归翻子方 + §B01-011 0xFF 归属修复的完整对局链中间态）。
 *
 * 对照 64 位实现（rules.c jq_flip §1010 先手翻子定色 `if(side==UNKNOWN)side=turn`
 * + §1012 turn=1-turn 交替 + jq_dark_setup 全 0xFF 布局）：
 *   · 先手翻子定色：蓝先手翻 0xFF 子 → 该子定色归蓝（翻子方，§B01-059）；
 *   · 翻棋回合交替：蓝翻→定蓝+turn切红，红翻→定红+turn切蓝（每次翻子定归翻子
 *     方 + turn 交替，§1012 `turn=1-turn`）；
 *   · 0xFF 全定色：翻完所有 0xFF 子后无未翻子（全定色，各子 side 归翻子方）。
 * 64 位行为级实证（走已实现的 jq_flip 定色/交替 + jq_can_flip 路径，0 分叉）。
 *
 * 几何口径：cell 9(1,4)/10(2,0)/11(2,1) 等 0xFF 未翻子；暗棋 FLIP 模式。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 先手翻子定色：蓝先手翻 0xFF 子 cell9 → 定色归蓝（side=BLUE）+ turn 切红 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[9].rank = 3; st.cells[9].side = SIDE_UNKNOWN; st.cells[9].revealed = 0;
        int ev = jq_flip(&st, 9);
        ok &= chk("① 蓝先手翻0xFF子定色归蓝(side=BLUE)+turn切红",
                  ev >= 0 && st.cells[9].side == SIDE_BLUE
                  && st.cells[9].revealed == 1 && st.turn == SIDE_RED);
    }
    /* ② 翻棋回合交替：蓝翻9→定蓝/红翻10→定红/蓝翻11→定蓝，turn 交替 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[9].rank = 3;  st.cells[9].side = SIDE_UNKNOWN;  st.cells[9].revealed = 0;
        st.cells[10].rank = 4; st.cells[10].side = SIDE_UNKNOWN; st.cells[10].revealed = 0;
        st.cells[11].rank = 5; st.cells[11].side = SIDE_UNKNOWN; st.cells[11].revealed = 0;
        jq_flip(&st, 9);    /* 蓝翻9 */
        int t1 = st.turn;                 /* 应切红 */
        jq_flip(&st, 10);   /* 红翻10 */
        int t2 = st.turn;                 /* 应切蓝 */
        jq_flip(&st, 11);   /* 蓝翻11 */
        int t3 = st.turn;                 /* 应切红 */
        ok &= chk("② 翻棋回合交替(蓝9定蓝/红10定红/蓝11定蓝,turn交替)",
                  st.cells[9].side == SIDE_BLUE && st.cells[10].side == SIDE_RED
                  && st.cells[11].side == SIDE_BLUE
                  && t1 == SIDE_RED && t2 == SIDE_BLUE && t3 == SIDE_RED);
    }
    /* ③ 0xFF 全定色中间态：翻完所有 0xFF 子后无未翻子（全定色） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[9].rank = 3;  st.cells[9].side = SIDE_UNKNOWN;  st.cells[9].revealed = 0;
        st.cells[10].rank = 4; st.cells[10].side = SIDE_UNKNOWN; st.cells[10].revealed = 0;
        int flips = 0;
        for (int i = 0; i < N_CELLS && st.stage == STG_PLAY; i++)
            if (jq_can_flip(&st, i)) { jq_flip(&st, i); flips++; }
        int unrevealed = 0;
        for (int i = 0; i < N_CELLS; i++)
            if (st.cells[i].rank != RANK_EMPTY && !st.cells[i].revealed) unrevealed++;
        ok &= chk("③ 0xFF全定色中间态(翻完所有0xFF子后未翻子=0)",
                  flips > 0 && unrevealed == 0 && st.cells[9].revealed == 1
                  && st.cells[10].revealed == 1);
    }
    /* ④ 先手翻子定色对称：红先手翻 0xFF 子 → 定色归红（对照 ① 蓝先手定蓝） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[50].rank = 6; st.cells[50].side = SIDE_UNKNOWN; st.cells[50].revealed = 0;
        int ev = jq_flip(&st, 50);
        ok &= chk("④ 红先手翻0xFF子定色归红(side=RED)+turn切蓝",
                  ev >= 0 && st.cells[50].side == SIDE_RED
                  && st.cells[50].revealed == 1 && st.turn == SIDE_BLUE);
    }
    printf("dark_flip_color_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
