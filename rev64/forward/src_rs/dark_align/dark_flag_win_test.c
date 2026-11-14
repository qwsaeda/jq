/* dark_flag_win_test.c — 扛旗胜（EV_FLAG）全方向 + 困毙对称
 * 补全现有胜负用例缺口：
 *   - dark_win_test 仅测"红困毙→蓝胜"（单方向，暗棋）；
 *   - dark_bomb_test ②③ 测"炸弹撞旗→撞旗方胜"（EV_BOMB 路径，含旗判胜）；
 *   - 本用例补"非炸弹扛旗胜（EV_FLAG）全方向" + "困毙对称（蓝困毙→红胜）"。
 *
 * 对照原版（rulelay 原文 + 0x30200 胜负分发）：
 *   - "任何棋子都能扛军旗"→ 扛旗方胜（jq_on_flag_capture winner=captor_side）；
 *   - 困毙（0x302b4）无棋可走方负 → jq_on_stalemate winner=1-loser_side。
 * 64 位行为级实证（走 64 位已实现的 jq_on_flag_capture / jq_side_stalemated 路径，
 * 规则层胜负 0 分叉）。
 *
 * 几何口径（复用 rule_text_conformance 扛旗夹具，明棋 LAY）：
 *   攻子从 (10,1)=cell51 攻红大本营 (11,1)=cell56（军旗格）→ 蓝扛红旗=蓝胜；
 *   对称：红普通子从 (5,4)? 不——红扛蓝旗须红子攻蓝大本营 (0,1)=cell1/(0,3)=cell3，
 *   用红普通子从 (1,1)=cell6 攻 (0,1)=cell1（蓝大本营军旗格）→ 红扛蓝旗=红胜。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"

static int expect(char *name, jq_state *st, int from, int to, int want_winner)
{
    int ev = jq_try_move(st, from, to);
    int ok = (st->stage == STG_OVER && st->winner == want_winner);
    printf("%s: 走子 %d→%d ev=%d stage=%d winner=%d（期望 %s=%d）→ %s\n",
           name, from, to, ev, st->stage, st->winner,
           want_winner == JQ_WIN_BLUE ? "BLUE" : want_winner == JQ_WIN_RED ? "RED" : "DRAW",
           want_winner, ok ? "PASS" : "FAIL");
    return ok;
}

int main(void)
{
    int ok = 1;
    /* ① 蓝扛红旗（明棋，普通子攻红大本营军旗）→ 蓝胜（EV_FLAG）
       蓝 rank5 @cell51(10,1) 攻 红军旗 @cell56(11,1) */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[51].rank = 5; st.cells[51].side = SIDE_BLUE; st.cells[51].revealed = 1;
        st.cells[56].rank = RANK_FLAG; st.cells[56].side = SIDE_RED; st.cells[56].revealed = 1;
        ok &= expect("① 蓝扛红旗→蓝胜(EV_FLAG)", &st, 51, 56, JQ_WIN_BLUE);
    }
    /* ② 红扛蓝旗（对称，明棋）→ 红胜（EV_FLAG）
       红 rank2 @cell6(1,1) 攻 蓝军旗 @cell1(0,1) */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[6].rank = 2; st.cells[6].side = SIDE_RED; st.cells[6].revealed = 1;
        st.cells[1].rank = RANK_FLAG; st.cells[1].side = SIDE_BLUE; st.cells[1].revealed = 1;
        ok &= expect("② 红扛蓝旗→红胜(EV_FLAG)", &st, 6, 1, JQ_WIN_RED);
    }
    /* ③ 困毙对称（暗棋，蓝无普通可动子→蓝困毙→红胜），对称 dark_win_test 反向
       构造：蓝仅 flag@56+mine@50（无普通子），红 rank3@49。
       红先走（49→44）触发收口 → 轮到蓝 → 蓝无棋可走（困毙）→ 红胜 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[56].rank = RANK_FLAG; st.cells[56].side = SIDE_BLUE; st.cells[56].revealed = 1;
        st.cells[50].rank = RANK_MINE;  st.cells[50].side = SIDE_BLUE; st.cells[50].revealed = 1;
        st.cells[49].rank = 3; st.cells[49].side = SIDE_RED; st.cells[49].revealed = 1;
        ok &= expect("③ 蓝困毙→红胜", &st, 49, 44, JQ_WIN_RED);
    }
    printf("dark_flag_win_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
