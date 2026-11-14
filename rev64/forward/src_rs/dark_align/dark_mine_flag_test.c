/* dark_mine_flag_test.c — 暗棋（ruleflip）挖雷→扛旗胜负完整链实证
 * 补全现有用例缺口：
 *   - rule_text_conformance 已覆盖暗棋扛棋门槛的 can_move 层（未挖满 3 雷拒 / 放行）；
 *   - 缺"走子执行器层 + 胜负定案"完整链：工兵挖雷 EV_MINE 实际递增 mines_cleared、
 *     工兵挖满 3 雷后扛旗 EV_FLAG→终局胜、非工兵暗棋扛旗被拒（即使挖满）。
 *
 * 对照原版 ruleflip 原文（SPAK data/junqi/ruleflip.txt）：
 *   「工兵挖 3 雷后才能扛军旗」「地雷仅工兵可吃」——
 *   64 位 move_local 暗棋段（rules.c:307-328）行为级对齐：
 *     · 目标雷：仅工兵(rank9)可吃（EV_MINE，mines_cleared[me]++）；
 *     · 目标旗：仅工兵 + mines_cleared[me]>=3 可扛（EV_FLAG，扛旗方胜）。
 * 64 位行为级实证（走已实现的 EV_MINE 挖雷路径 + EV_FLAG 扛旗路径，0 分叉）。
 *
 * 几何口径：col1 纵线蓝工兵 cell46(row9col1) 攻 红雷 cell51(row10col1)（相邻合法）；
 *   扛旗目标 = 红大本营军旗 cell56(row11col1)，工兵须先挖满 3 雷（mines_cleared>=3）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"

static int chk(char *name, int cond, int detail_ev, int want_ev)
{
    printf("%s: ev=%d(应%s=%d) → %s\n", name, detail_ev,
           want_ev >= 0 ? "EV_x" : "n/a", want_ev, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 暗棋工兵挖雷：蓝工兵 46→51(雷) → EV_MINE + mines_cleared[BLUE]=1 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[46].rank = RANK_WORKER; st.cells[46].side = SIDE_BLUE; st.cells[46].revealed = 1;
        st.cells[51].rank = RANK_MINE;  st.cells[51].side = SIDE_RED;  st.cells[51].revealed = 1;
        int ev = jq_try_move(&st, 46, 51);
        /* 挖雷：工兵落雷格 51（*dst=*src 交换，0x3a15c），源格 46 空，雷消失，
           mines_cleared[BLUE]++（§3 测试桩判据陷阱"工兵挖雷后工兵进目标格"） */
        ok &= chk("① 工兵挖雷 EV_MINE", ev == EV_MINE && st.mines_cleared[SIDE_BLUE] == 1
                  && st.cells[46].rank == RANK_EMPTY /* 源格空 */ && st.cells[51].rank == RANK_WORKER /* 工兵进雷格 */,
                  ev, EV_MINE);
    }
    /* ② 非工兵暗棋扛旗被拒（即使 mines_cleared 已 3）：rank2 攻旗 can_move=0 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[51].rank = 2; st.cells[51].side = SIDE_BLUE; st.cells[51].revealed = 1; /* rank2@cell51 */
        st.cells[56].rank = RANK_FLAG; st.cells[56].side = SIDE_RED; st.cells[56].revealed = 1; /* 红军旗@56 */
        st.mines_cleared[SIDE_BLUE] = 3;   /* 即使挖满，非工兵也不能扛 */
        int can = jq_can_move(&st, 51, 56);
        ok &= chk("② 非工兵暗棋扛旗被拒(挖满也拒)", can == 0, -1, -1);
    }
    /* ③ 工兵未挖满 3 雷扛旗被拒（mines_cleared=2，can_move=0） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[51].rank = RANK_WORKER; st.cells[51].side = SIDE_BLUE; st.cells[51].revealed = 1;
        st.cells[56].rank = RANK_FLAG; st.cells[56].side = SIDE_RED; st.cells[56].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 2;   /* 未挖满 3 雷 */
        int can = jq_can_move(&st, 51, 56);
        ok &= chk("③ 工兵未挖满 3 雷扛旗被拒", can == 0, -1, -1);
    }
    /* ④ 工兵挖满 3 雷扛旗→扛旗方(蓝)胜：EV_FLAG + stage=OVER + winner=BLUE */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[51].rank = RANK_WORKER; st.cells[51].side = SIDE_BLUE; st.cells[51].revealed = 1;
        st.cells[56].rank = RANK_FLAG; st.cells[56].side = SIDE_RED; st.cells[56].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 3;   /* 挖满 3 雷 */
        int ev = jq_try_move(&st, 51, 56);
        ok &= chk("④ 工兵挖满扛旗→蓝胜", ev == EV_FLAG && st.stage == STG_OVER
                  && st.winner == JQ_WIN_BLUE, ev, EV_FLAG);
    }
    printf("dark_mine_flag_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
