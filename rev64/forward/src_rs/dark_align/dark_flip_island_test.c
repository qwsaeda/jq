/* dark_flip_island_test.c — 暗棋（FLIP）行营/大本营安全岛差异 + 暗棋开关门控行为级实证
 * 补全现有缺口：§48 dark_camp_special_test 覆盖行营/大本营安全岛差异但走
 * 明棋 LAY，**暗棋 FLIP 下"行营安全岛无例外 vs 大本营仅旗/雷例外"的 FLIP
 * 对照 + 暗棋开关（which=4）门控差异**未独立用例（§301 大本营例外受
 * `mode==FLIP&&cfg(4)` 门控 vs §288 行营判定无 mode 门控——暗棋下行营仍全拒，
 * 大本营例外需开关开）。
 *
 * 对照 64 位实现（rules.c §288 行营 `is_camp` 无 mode 门控 + §301 大本营
 * `is_base` 受 `mode==FLIP&&cfg(4)` 门控 + §37 which=4 暗棋开关）：
 *   · 暗棋行营安全岛无例外（§288 跨明/暗棋一致全拒雷/旗/普通子）；
 *   · 暗棋大本营例外（§301 仅旗/雷，受 FLIP 门控，开关关退回明棋任意子）；
 *   · 暗棋开关（which=4）门控差异：行营判定不受开关影响（恒全拒），大本营
 *     例外受开关控制（开=仅旗/雷 / 关=任意子可扛）。
 * 64 位行为级实证（走已实现的 §288 行营无例外 + §301 大本营例外 + which=4
 * 门控路径，0 分叉）。
 *
 * 几何口径：行营 (3,2) 蓝 cell42(4,2) 攻；大本营 cell56(11,1) 蓝 cell51(10,1)
 *   攻（§32/§44/§48 既有几何）；cell51 工兵挖满 3 雷扛旗。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_dark_set(int v);
#define C32 cell_index(3,2)    /* 行营(3,2) */
#define C42 cell_index(4,2)    /* (4,2) */
#define C56 cell_index(11,1)   /* 大本营 cell56(11,1) */
#define C51 cell_index(10,1)   /* (10,1) 大本营上邻 */

static int chk(char *name, int got, int want)
{
    int ok = (got == want);
    printf("%s: got=%d want=%d → %s\n", name, got, want, ok ? "PASS" : "FAIL");
    return ok;
}

int main(void)
{
    int ok = 1;
    /* ① 暗棋行营安全岛无例外（§288 跨明/暗棋一致全拒）：行营(3,2)红雷 + 蓝工兵攻=0 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = RANK_WORKER; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C32].rank = RANK_MINE;  st.cells[C32].side = SIDE_RED;  st.cells[C32].revealed = 1;
        ok &= chk("① 暗棋行营(3,2)红雷+蓝工兵攻=0(行营无例外,暗棋也拒)",
                  jq_can_move(&st, C42, C32), 0);
    }
    /* ② 暗棋大本营例外放行（§301 仅旗/雷，FLIP 门控）：大本营(11,1)红军旗 + 蓝工兵挖满扛旗=1 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG;   st.cells[C56].side = SIDE_RED;  st.cells[C56].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 3;
        ok &= chk("② 暗棋大本营(11,1)红军旗+蓝工兵挖满扛旗=1(大本营例外放行)",
                  jq_can_move(&st, C51, C56), 1);
    }
    /* ③ 暗棋大本营普通子拒（仅旗/雷例外）：大本营(11,1)红rank3 + 蓝工兵攻=0 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = 3;           st.cells[C56].side = SIDE_RED;  st.cells[C56].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 3;
        ok &= chk("③ 暗棋大本营(11,1)红rank3+蓝工兵攻=0(仅旗/雷例外)",
                  jq_can_move(&st, C51, C56), 0);
    }
    /* ④ 暗棋开关关（which=4=0）退回明棋：大本营红军旗 + 蓝 rank5 任意子可扛=1 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5;          st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG;  st.cells[C56].side = SIDE_RED;  st.cells[C56].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 0;
        ui_rule_dark_set(0);   /* 暗棋开关关 */
        int can = jq_can_move(&st, C51, C56);
        ui_rule_dark_set(1);   /* 复原 */
        ok &= chk("④ 暗棋开关关, 大本营(11,1)红军旗+蓝rank5攻=1(退回明棋任意子可扛)",
                  can, 1);
    }
    /* ⑤ 暗棋开关开对照：同几何（大本营红军旗 + 蓝 rank5 非工兵）扛旗拒（仅工兵） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5;          st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG;  st.cells[C56].side = SIDE_RED;  st.cells[C56].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 0;
        ui_rule_dark_set(1);   /* 暗棋开关开 */
        int can = jq_can_move(&st, C51, C56);
        ok &= chk("⑤ 暗棋开关开对照, 大本营(11,1)红军旗+蓝rank5非工兵扛旗=0(仅工兵)",
                  can, 0);
    }
    printf("dark_flip_island_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
