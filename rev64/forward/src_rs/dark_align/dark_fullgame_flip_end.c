/* dark_fullgame_flip_end.c — 暗棋（ruleflip）翻雷/翻旗特殊终局分支链实证
 * 补全现有暗棋用例缺口（dark_fullgame 仅覆盖"翻棋回合交替"，缺翻雷/翻旗终局分支）：
 *   - 翻子定色：翻 cell51 归属翻子方（红翻→RED / 蓝翻→BLUE，§30/§31 0xFF 双方可翻口径）；
 *   - 翻出敌方雷 → 工兵挖雷 EV_MINE + mines_cleared++（§32 口径）；
 *   - 翻出己方雷 → 工兵不可挖（can_move=0，雷归己方）；
 *   - 翻出军旗 → 工兵(挖满 3 雷)扛旗 EV_FLAG 终局胜。
 *
 * 对照原版 ruleflip（SPAK data/junqi/ruleflip.txt）：
 *   「工兵挖 3 雷后才能扛军旗」「地雷仅工兵可吃」「翻出的子归属翻子方」——
 *   64 位 jq_flip（0xFF 翻归翻子方）+ move_local 暗棋段（rules.c:307-328）
 *   + jq_on_flag_capture（winner=captor_side）行为级 0 分叉。
 * 64 位行为级实证（走已实现的 jq_flip / EV_MINE / EV_FLAG 路径，0 分叉）。
 *
 * 几何口径：col1 纵线蓝工兵 cell46(row9col1) 攻红 cell51(row10col1)（相邻合法）；
 *   扛旗目标 = 红大本营军旗 cell56(row11col1)，工兵须先挖满 3 雷（mines_cleared>=3）。
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
    /* ① 翻子定色：红翻 cell51 → side 归 RED（翻归翻子方，§30/§31 口径） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[51].rank = RANK_MINE; st.cells[51].side = 0xFF; st.cells[51].revealed = 0;
        int can = jq_can_flip(&st, 51);
        int ev = jq_flip(&st, 51);
        ok &= chk("① 红翻 cell51→side归RED(翻归翻子方)",
                  can == 1 && ev >= 0 && st.cells[51].side == SIDE_RED
                  && st.cells[51].revealed == 1);
    }
    /* ② 翻出敌方雷→工兵挖雷 EV_MINE + mines_cleared[BLUE]++（§32 口径） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[46].rank = RANK_WORKER; st.cells[46].side = SIDE_BLUE; st.cells[46].revealed = 1;
        st.cells[51].rank = RANK_MINE;  st.cells[51].side = 0xFF;      st.cells[51].revealed = 0;
        jq_flip(&st, 51);            /* 红翻 51 → 归红（红雷） */
        st.turn = SIDE_BLUE;
        int ev = jq_try_move(&st, 46, 51);   /* 蓝工兵挖红雷 */
        ok &= chk("② 翻出敌方雷→工兵挖雷 EV_MINE+mines_cleared++",
                  ev == EV_MINE && st.mines_cleared[SIDE_BLUE] == 1
                  && st.cells[51].rank == RANK_WORKER);  /* 工兵进雷格 */
    }
    /* ③ 翻出己方雷→工兵不可挖（雷归翻子方=己方，can_move=0） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[46].rank = RANK_WORKER; st.cells[46].side = SIDE_BLUE; st.cells[46].revealed = 1;
        st.cells[51].rank = RANK_MINE;  st.cells[51].side = 0xFF;      st.cells[51].revealed = 0;
        jq_flip(&st, 51);            /* 蓝翻 51 → 归蓝（蓝雷=己方雷） */
        int can = jq_can_move(&st, 46, 51);   /* 蓝工兵挖自己雷 → 拒 */
        ok &= chk("③ 翻出己方雷→工兵不可挖(can_move=0)", can == 0);
    }
    /* ④ 红军旗(大本营,已翻)+挖满3雷→工兵扛旗 EV_FLAG 终局胜（§32 ④ 口径）
       注：大本营军旗格在暗棋下不可翻（安全岛例外，jq_can_flip(56)=0 实测），
       故直接置 cell56 为已翻红军旗（revealed=1 side=RED），不经翻子路径。 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[56].rank = RANK_FLAG; st.cells[56].side = SIDE_RED; st.cells[56].revealed = 1;
        st.cells[51].rank = RANK_WORKER; st.cells[51].side = SIDE_BLUE; st.cells[51].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 3;   /* 挖满 3 雷 */
        int can = jq_can_move(&st, 51, 56);   /* 工兵+挖满3雷→扛旗放行 */
        int ev = jq_try_move(&st, 51, 56);   /* 蓝工兵扛红军旗 */
        ok &= chk("④ 红军旗+挖满3雷→工兵扛旗 EV_FLAG 蓝胜",
                  can == 1 && ev == EV_FLAG && st.stage == STG_OVER
                  && st.winner == JQ_WIN_BLUE);
    }
    printf("dark_fullgame_flip_end: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
