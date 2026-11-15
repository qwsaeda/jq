/* dark_mineoff_stalemate_test.c — which=2（吃雷/地雷无敌开关）开/关 下困毙分界行为级实证
 * 补全现有缺口：§51 dark_4switch_combo_test 覆盖 which=2 开/关基础行为
 * （挖雷 vs 送死），§54 dark_flagoff_stalemate_test 覆盖 which=3 flag-off
 * 困毙分界，但**which=2 关（mine-off 地雷无敌）下"工兵撞雷送死 → 该方无
 * 其他子 → 困毙判负"的完整分界**（与 §54 flag-off 同源的"开关关→胜负靠
 * 困毙近似"机制，地雷无敌场景）无独立用例（§35 挂起"mine-off 走单侧困毙
 * 路径"）。
 *
 * 对照 64 位实现（rules.c §746 which=2 `if(cfg(2)&&工兵)挖雷 else 送死`
 * + §555 jq_on_stalemate 困毙近似 + §37 which=2 地雷无敌开关）：
 *   · which=2 开（默认）：工兵撞雷=EV_MINE 挖雷（雷消失，工兵进雷格，
 *     mines_cleared++）；
 *   · which=2 关（mine-off 地雷无敌）：工兵撞雷=EV_DEAD 送死（工兵消失，
 *     雷不消失，mines_cleared 不增）——地雷无敌；
 *   · which=2 关 + 双方无其他可动子：工兵送死后双方困毙 → 和棋（winner=DRAW，
 *     §30 draw_exist_test "双方无普通子→Draw" 口径）；
 *   · which=2 关 + 该方有可动子（对方仅雷不可动）：工兵送死后对方困毙 → 该方
 *     胜（单侧困毙近似，与 §54 flag-off 同源的"开关关→胜负靠困毙近似"机制）。
 * 64 位行为级实证（走已实现的 §746 which=2 分支 + §555 困毙近似路径，0 分叉）。
 *
 * 几何口径：col1 纵线蓝工兵 cell46(9,1) 撞 红雷 cell51(10,1)（§32 既有挖雷
 *   几何）；蓝防困毙/红防困毙用 cell50(10,0)。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_mine_set(int v);
#define C46 cell_index(9,1)    /* 蓝工兵 */
#define C51 cell_index(10,1)   /* 红雷 */
#define C50 cell_index(10,0)   /* 防困毙格 */

static int chk(char *name, int got, int want)
{
    int ok = (got == want);
    printf("%s: got=%d want=%d → %s\n", name, got, want, ok ? "PASS" : "FAIL");
    return ok;
}

int main(void)
{
    int ok = 1;
    /* ① which=2 开：工兵挖雷（EV_MINE，雷消失工兵进雷格，mines_cleared++） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C46].rank = RANK_WORKER; st.cells[C46].side = SIDE_BLUE; st.cells[C46].revealed = 1;
        st.cells[C51].rank = RANK_MINE;  st.cells[C51].side = SIDE_RED;  st.cells[C51].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED; st.cells[C50].revealed = 1;
        ui_rule_mine_set(1);
        int ev = jq_try_move(&st, C46, C51);
        ok &= chk("① which=2开: 工兵挖雷(EV_MINE+工兵进雷格+mines_cleared++)",
                  ev == EV_MINE && st.cells[C51].rank == RANK_WORKER
                  && st.mines_cleared[SIDE_BLUE] == 1, 1);
    }
    /* ② which=2 关：工兵撞雷送死（EV_DEAD，工兵消失雷留，mines_cleared 不增） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C46].rank = RANK_WORKER; st.cells[C46].side = SIDE_BLUE; st.cells[C46].revealed = 1;
        st.cells[C51].rank = RANK_MINE;  st.cells[C51].side = SIDE_RED;  st.cells[C51].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED; st.cells[C50].revealed = 1;
        ui_rule_mine_set(0);
        int ev = jq_try_move(&st, C46, C51);
        int ok_ = (ev == EV_DEAD && st.cells[C46].rank == RANK_EMPTY
                   && st.cells[C51].rank == RANK_MINE
                   && st.mines_cleared[SIDE_BLUE] == 0);
        ui_rule_mine_set(1);   /* 复原 */
        ok &= chk("② which=2关: 工兵撞雷送死(EV_DEAD+工兵消失雷留+mines不增)", ok_, 1);
    }
    /* ③ which=2 关 + 双方无其他可动子：工兵送死后蓝无子(工兵死)+红仅雷不可动
       → 双困毙 → 和棋（winner=DRAW，§30 draw_exist_test "双方无普通子→Draw" 口径） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C46].rank = RANK_WORKER; st.cells[C46].side = SIDE_BLUE; st.cells[C46].revealed = 1;
        st.cells[C51].rank = RANK_MINE;  st.cells[C51].side = SIDE_RED;  st.cells[C51].revealed = 1;
        /* 蓝仅工兵@46（送死后蓝无子），红仅雷@51（不可动）→ 双困毙 */
        ui_rule_mine_set(0);
        int ev = jq_try_move(&st, C46, C51);
        int ok_ = (st.stage == STG_OVER && st.winner == JQ_WIN_DRAW);
        ui_rule_mine_set(1);   /* 复原 */
        ok &= chk("③ which=2关+双方无子: 工兵送死后双困毙→和棋(DRAW,§30口径)", ok_, 1);
        (void)ev;
    }
    /* ④ which=2 关 + 蓝有可动子(rank2@8,1)：工兵送死后红仅雷不可动→红困毙→蓝胜 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C46].rank = RANK_WORKER; st.cells[C46].side = SIDE_BLUE; st.cells[C46].revealed = 1;
        st.cells[C51].rank = RANK_MINE;  st.cells[C51].side = SIDE_RED;  st.cells[C51].revealed = 1;
        st.cells[cell_index(8,1)].rank = 2; st.cells[cell_index(8,1)].side = SIDE_BLUE;
        st.cells[cell_index(8,1)].revealed = 1;   /* 蓝 rank2@8,1 可动 */
        st.cells[cell_index(8,0)].rank = 0;        /* (8,0) 空, rank2 可走 */
        ui_rule_mine_set(0);
        int ev = jq_try_move(&st, C46, C51);
        int ok_ = (st.stage == STG_OVER && st.winner == JQ_WIN_BLUE);
        ui_rule_mine_set(1);   /* 复原 */
        ok &= chk("④ which=2关+蓝有可动子: 工兵送死后红仅雷不可动→红困毙→蓝胜", ok_, 1);
        (void)ev;
    }
    printf("dark_mineoff_stalemate_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
