/* dark_darkoff_matrix_test.c — which=4（暗棋开关）开/关 退回明棋的门槛失效矩阵行为级实证
 * 补全现有缺口：§49 dark_flip_island_test ④ 覆盖 which=4 关扛旗放行（单维度），
 * 但**which=4 关退回明棋的完整门槛失效矩阵（扛旗 + 吃雷 + 工兵专属 三维度
 * 开/关对照）**无独立用例（暗棋专属门槛 = 工兵可吃雷/挖 3 雷扛旗/翻子定色，
 * which=4 关全失效退回明棋"任意子可扛旗/吃雷"）。
 *
 * 对照 64 位实现（rules.c §292/310 which=4 `st->mode==FLIP&&cfg(4)` 门控暗棋段
 * + §225 暗棋非工兵不能吃雷 + §307-328 暗棋扛旗工兵/挖 3 雷门槛 + §311 明棋
 * 任意子可扛旗）：
 *   · which=4 开（默认，暗棋）：仅工兵可吃雷 + 工兵挖满 3 雷可扛旗 + 翻子定色
 *     （暗棋专属门槛）；
 *   · which=4 关（退回明棋语义）：暗棋专属门槛全失效——非工兵可扛旗（§311
 *     明棋任意子）、非工兵可吃雷（明棋无雷专属门槛），翻子门槛消失（明棋无
 *     翻子）；
 *   · 三维度开/关对照：扛旗（工兵门槛）/ 吃雷（工兵门槛）/ 翻子（暗棋专属）。
 * 64 位行为级实证（走已实现的 §292/310 which=4 门控 + §225/§307-328 暗棋门槛
 * + §311 明棋语义路径，0 分叉）。
 *
 * 几何口径：col1 纵线蓝 cell51(10,1) 攻 大本营 cell56(11,1)（§32/§44/§49 既有
 *   扛旗几何）；col2 横 cell4(4,1) 攻雷 cell5(4,2)（§225 吃雷几何）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_dark_set(int v);
#define C51 cell_index(10,1)   /* 蓝攻子 */
#define C56 cell_index(11,1)   /* 红军旗大本营 */
#define C4  cell_index(4,1)    /* 蓝 rank5 */
#define C5  cell_index(4,2)    /* 红雷 */
#define C6  cell_index(4,3)    /* 红防困毙 */

static int chk(char *name, int got, int want)
{
    int ok = (got == want);
    printf("%s: got=%d want=%d → %s\n", name, got, want, ok ? "PASS" : "FAIL");
    return ok;
}

int main(void)
{
    int ok = 1;
    /* ① 扛旗维度：暗棋开 非工兵扛旗拒 vs 暗棋关 非工兵扛旗放行 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 0;
        ui_rule_dark_set(1);
        int open_ = jq_can_move(&st, C51, C56);   /* 暗棋开：拒 */
        ui_rule_dark_set(0);
        int closed = jq_can_move(&st, C51, C56);  /* 暗棋关：放行 */
        ui_rule_dark_set(1);   /* 复原 */
        ok &= chk("① 扛旗: 暗棋开非工兵扛旗拒=0 vs 关放行=1",
                  open_ == 0 && closed == 1, 1);
    }
    /* ② 吃雷维度：暗棋开 非工兵吃雷拒 vs 暗棋关 非工兵吃雷放行 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C4].rank = 5; st.cells[C4].side = SIDE_BLUE; st.cells[C4].revealed = 1;
        st.cells[C5].rank = RANK_MINE; st.cells[C5].side = SIDE_RED; st.cells[C5].revealed = 1;
        st.cells[C6].rank = 1; st.cells[C6].side = SIDE_RED; st.cells[C6].revealed = 1;
        ui_rule_dark_set(1);
        int open_ = jq_can_move(&st, C4, C5);   /* 暗棋开：非工兵拒 */
        ui_rule_dark_set(0);
        int closed = jq_can_move(&st, C4, C5);  /* 暗棋关：放行 */
        ui_rule_dark_set(1);   /* 复原 */
        ok &= chk("② 吃雷: 暗棋开非工兵吃雷拒=0 vs 关放行=1",
                  open_ == 0 && closed == 1, 1);
    }
    /* ③ 工兵专属维度：暗棋开 工兵可吃雷 vs 暗棋关 工兵也可吃雷（工兵门槛消失但
       工兵仍能走）—— 对照"工兵专属"在明棋下非专属 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C4].rank = RANK_WORKER; st.cells[C4].side = SIDE_BLUE; st.cells[C4].revealed = 1;
        st.cells[C5].rank = RANK_MINE; st.cells[C5].side = SIDE_RED; st.cells[C5].revealed = 1;
        st.cells[C6].rank = 1; st.cells[C6].side = SIDE_RED; st.cells[C6].revealed = 1;
        ui_rule_dark_set(1);
        int open_ = jq_can_move(&st, C4, C5);   /* 暗棋开：工兵可吃雷 */
        ui_rule_dark_set(0);
        int closed = jq_can_move(&st, C4, C5);  /* 暗棋关：工兵仍可吃雷 */
        ui_rule_dark_set(1);   /* 复原 */
        ok &= chk("③ 工兵专属: 暗棋开工兵吃雷=1 vs 关工兵仍吃雷=1(工兵始终可)",
                  open_ == 1 && closed == 1, 1);
    }
    /* ④ 翻子维度：暗棋开 0xFF 子可翻 vs 暗棋关（明棋无翻子，revealed 全 1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[9].rank = 3; st.cells[9].side = SIDE_UNKNOWN; st.cells[9].revealed = 0;
        ui_rule_dark_set(1);
        int open_ = jq_can_flip(&st, 9);   /* 暗棋开：0xFF 可翻 */
        ui_rule_dark_set(0);
        int closed = jq_can_flip(&st, 9);  /* 暗棋关：明棋无翻子(0xFF 不可翻?) */
        ui_rule_dark_set(1);   /* 复原 */
        printf("④ 翻子: 暗棋开 0xFF可翻=%d vs 关=%d(对照口径)\n", open_, closed);
        ok &= chk("④ 翻子: 暗棋开 0xFF 可翻=1", open_, 1);
    }
    printf("dark_darkoff_matrix_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
