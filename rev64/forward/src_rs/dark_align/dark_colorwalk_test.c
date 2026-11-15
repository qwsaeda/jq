/* dark_colorwalk_test.c — 0xFF 全定色后走子比较（定色 side 还原 rank_of + rank 小=强）行为级实证
 * 补全现有缺口：§57 dark_flip_color_test 实证"先手翻子定色 + 翻棋交替 + 0xFF 全定
 * 色中间态"，但**全定色后走子比较（定色 side 还原 rank_of + rank 小=强）的完整
 * 走子场景**未独立用例（全定色后等同明棋口径，对照 §30/§35 既有 rank 小=强）。
 *
 * 对照 64 位实现（rules.c rank_of/rank_norm 按定色 side 还原 raw rank + §796-798
 * rank 小=强 L52 确证 + §817 同级互杀 else 分支）：
 *   · 0xFF 全定色后（各子 side 已归翻子方）走子比较 = 明棋同口径：
 *     大吃小（EV_EAT 攻方吃被吃方）/ 小撞大（EV_DEAD 攻方被吃）/ 同级互杀
 *     （EV_EAT 双消失）；
 *   · rank 小=强（rank1 司令最强 ... rank9 工兵最弱普通），蓝定色 rank1 攻
 *     红定色 rank9 = 大吃小 EV_EAT；
 *   · 全定色后暗棋走子比较与明棋完全一致（定色 side 还原后 rank_of 还原 raw
 *     rank，比较逻辑明暗棋同源）。
 * 64 位行为级实证（走已实现的 rank_of/rank_norm + jq_try_move 吃子比较路径，
 * 0 分叉，全定色后等同明棋）。
 *
 * 几何口径：col4 纵线 44(蓝)↔39(红) 相邻（§3 既有合法几何）；红 rank2@50 防
 *   困毙；cell 索引 row*5+col。
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
    /* ① 全定色后大吃小：蓝 rank1(强) 攻 红 rank9(弱) → EV_EAT（对照明棋口径） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 9; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        int ev = jq_try_move(&st, 44, 39);
        ok &= chk("① 全定色后大吃小(蓝rank1强攻红rank9弱)→EV_EAT",
                  ev == EV_EAT && st.cells[39].rank == 1
                  && st.cells[44].rank == RANK_EMPTY);
    }
    /* ② 全定色后小撞大（送死）：蓝 rank9(弱) 攻 红 rank1(强) → EV_DEAD */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 9; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 1; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        int ev = jq_try_move(&st, 44, 39);
        ok &= chk("② 全定色后小撞大送死(蓝rank9弱攻红rank1强)→EV_DEAD",
                  ev == EV_DEAD && st.cells[44].rank == RANK_EMPTY
                  && st.cells[39].rank == 1);
    }
    /* ③ 全定色后同级互杀：蓝 rank5 攻 红 rank5 → EV_EAT 双消失 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 5; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 5; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        int ev = jq_try_move(&st, 44, 39);
        ok &= chk("③ 全定色后同级互杀(蓝rank5攻红rank5)→EV_EAT双消失",
                  ev == EV_EAT && st.cells[44].rank == RANK_EMPTY
                  && st.cells[39].rank == RANK_EMPTY);
    }
    /* ④ 全定色后走子比较与明棋对照（同几何 LAY 口径一致，全定色后等同明棋） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;   /* 明棋对照 */
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 9; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        int ev = jq_try_move(&st, 44, 39);
        /* 明棋大吃小同口径（对照 ① 全定色后暗棋，全定色后等同明棋） */
        ok &= chk("④ 明棋对照大吃小(蓝rank1攻红rank9 LAY)→EV_EAT(全定色后等同)",
                  ev == EV_EAT && st.cells[39].rank == 1);
    }
    printf("dark_colorwalk_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
