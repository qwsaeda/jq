/* dark_tongji_flip_test.c — 暗棋（FLIP）同级互杀 + 双炸弹相撞对照实证
 * 补全现有缺口：eat_dir_test / rule_text_conformance 同级互杀用例全走明棋 LAY，
 * 暗棋 FLIP 下"同级互杀（rank 相等非 BOMB → EV_EAT 双消失）"与"双炸弹相撞
 * （BOMB 撞 BOMB → EV_BOMB 同归，非同级互杀分支）"未独立覆盖。
 *
 * 对照 64 位实现（rules.c jq_try_move）：
 *   · 普通子同级互杀（§817 else 分支，sr==dr_ 且非 BOMB）→ EV_EAT 双格清空 +
 *     pieces_left 双方--（§827 B01-086 攻守跨方 1-me），§798 实证对齐原版
 *     0x31de0（同级发 Junqi.Eat 非 Bomb）；
 *   · 双炸弹相撞（§含炸弹同归分支，先于同级判定）→ EV_BOMB 双格清空（炸弹同归
 *     优先于 rank 相等同级——BOMB rank 相等但命中炸弹分支非同级互杀）。
 * 64 位行为级实证（走已实现的同级互杀 + 炸弹同归分支，暗棋/明棋 0 分叉）。
 *
 * 几何口径：col4 纵线 44(蓝)→39(红) 相邻（§3 既有合法几何）；红 rank1@50
 * 防困毙路径掩盖终局判定。
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
    /* ① 暗棋普通子同级互杀（蓝 rank5 攻红 rank5 已翻）→ EV_EAT + 双格空 + PLAY
       （对照 eat_dir_test ③ 明棋口径，暗棋 FLIP 0 分叉） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 5; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 5; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 1; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        int can = jq_can_move(&st, 44, 39);
        int ev = jq_try_move(&st, 44, 39);
        ok &= chk("① 暗棋普通子同级互杀(蓝5攻红5)→EV_EAT双格空+PLAY",
                  can == 1 && ev == EV_EAT && st.cells[44].rank == RANK_EMPTY
                  && st.cells[39].rank == RANK_EMPTY
                  && st.stage == STG_PLAY && st.winner == JQ_WIN_NONE);
    }
    /* ② 暗棋双炸弹相撞（蓝 BOMB 攻红 BOMB）→ EV_BOMB 双格空（炸弹同归优先于同级互杀） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = RANK_BOMB; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = RANK_BOMB; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 1; st.cells[50].side = SIDE_RED; st.cells[50].revealed = 1;
        int ev = jq_try_move(&st, 44, 39);
        ok &= chk("② 暗棋双炸弹相撞(蓝BOMB攻红BOMB)→EV_BOMB双格空(炸弹同归优先)",
                  ev == EV_BOMB && st.cells[44].rank == RANK_EMPTY
                  && st.cells[39].rank == RANK_EMPTY
                  && st.stage == STG_PLAY && st.winner == JQ_WIN_NONE);
    }
    /* ③ 明棋同级互杀对照（蓝 rank5 攻红 rank5 已翻，LAY）→ EV_EAT 双格空（与①同口径） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 5; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 5; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 1; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        int ev = jq_try_move(&st, 44, 39);
        ok &= chk("③ 明棋同级互杀(蓝5攻红5 LAY)→EV_EAT双格空(与①暗棋同口径)",
                  ev == EV_EAT && st.cells[44].rank == RANK_EMPTY
                  && st.cells[39].rank == RANK_EMPTY
                  && st.stage == STG_PLAY && st.winner == JQ_WIN_NONE);
    }
    printf("dark_tongji_flip_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
