/* dark_eat_reset_count_test.c — 暗棋/明棋吃子分支 moves_since_eat 清零行为级实证
 * 补全现有缺口：dark_flip_draw_count_test 覆盖"翻棋不计数 + 走子累加判和"，
 * rule_text_conformance 覆盖明棋 70/1000 步，但**吃子分支的 moves_since_eat=0
 * 清零 + EV_DEAD 送死不清零例外**（§B01-022 P1 修复）无独立验证用例。
 *
 * 对照 64 位实现（rules.c jq_try_move 4 分支 + 1 例外，§675/726/737/784/830）：
 *   · EV_EAT（明棋大吃小）/EV_BOMB（炸弹同归）/EV_FLAG（扛旗）/EV_MINE（真挖雷）
 *     各含 moves_since_eat=0（吃子活动重置 70 步无吃子进度）；
 *   · EV_DEAD 送死（撞雷雷未消失，无子被吃）**不**清零 moves_since_eat——
 *     §B01-022（P1）已修：旧实现 if/else 公共尾部清零把"送死"当"吃子"误重置，
 *     修后送死分支不清零（70 步进度保持）。
 *   · 暗棋吃子（蓝 rank1 强子吃已翻红 rank9 弱子）走 EV_EAT（大压小 src 吃
 *     dst），蓝 rank9 弱子撞红 rank1 强子走 EV_DEAD（攻方被吃），但**暗棋送死
 *     仍有子被吃（红 1 存蓝 9 消），清零正确**——与明棋 EV_DEAD 送死（撞雷雷不
 *     消失，无子被吃）**不清零**的 §B01-022 例外不同。
 * 64 位行为级实证（走已实现的 4 分支清零 + 明棋 EV_DEAD 送死不清零路径，0 分叉）。
 *
 * ★ rank 编码口径（§796-798 实证 L52 确证）：rank 小=强（rank1 最强，rank10 最弱），
 *   sr < dr_ = 大压小（EV_EAT 攻方吃被吃方）；sr > dr_ = 攻方被吃（EV_DEAD）。
 * 几何口径：col4 纵线 44(蓝 rank1)→39(红 rank9) 吃子 / 44(BOMB)→39(rank3) 炸弹 /
 *   col1 纵线 46(蓝工兵)→51(红雷) 挖雷；明棋送死 44(rank2)→39(红雷)。
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
    /* ① 暗棋吃子（蓝 rank1 强子吃已翻红 rank9 弱子）→ EV_EAT + moves_since_eat 清零
       ★ rank 编码 rank 小=强（L52 确证，§796-798）：rank1 最强，rank9 最弱。
       蓝 1(sr=1) < 红 9(dr=9) → 大压小 src 吃 dst（EV_EAT），红 9 消失。 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 9; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.moves_since_eat = 69;
        int ev = jq_try_move(&st, 44, 39);
        /* 交换器语义（§3 测试桩判据陷阱既有口径）：*dst=*src（蓝 1 整格落到 39，
           39 的 rank=1）+ memset(src,0)（44 清空）。红 9 被吃消失（被蓝 1 覆盖）。 */
        ok &= chk("① 暗棋吃子(蓝rank1强吃红rank9弱)→EV_EAT+moves_since_eat清零",
                  ev == EV_EAT && st.moves_since_eat == 0
                  && st.cells[39].rank == 1 && st.cells[44].rank == RANK_EMPTY);
    }
    /* ①b 暗棋送死（蓝 rank9 弱子撞红 rank1 强子）→ EV_DEAD + 攻方消失 + moves_since_eat 清零
       蓝 9(sr=9) > 红 1(dr=1) → src 被吃（EV_DEAD），蓝 9 消失、红 1 留，仍清零
       （暗棋吃子/送死都算"吃子活动"，重置 70 步进度——与明棋 EV_DEAD 送死不清零
       §B01-022 的"雷未消失"例外不同：暗棋送死有子被吃（红 1 存，蓝 9 消），清零正确）。 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 9; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 1; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.moves_since_eat = 69;
        int ev = jq_try_move(&st, 44, 39);
        ok &= chk("①b 暗棋送死(蓝rank9弱撞红rank1强)→EV_DEAD+攻方消失+清零",
                  ev == EV_DEAD && st.moves_since_eat == 0
                  && st.cells[44].rank == RANK_EMPTY && st.cells[39].rank == 1);
    }
    /* ② 暗棋炸弹（蓝 BOMB 撞红 rank3）→ EV_BOMB + 同归 + moves_since_eat 清零 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = RANK_BOMB; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 3; st.cells[39].side = SIDE_RED; st.cells[39].revealed = 1;
        st.moves_since_eat = 69;
        int ev = jq_try_move(&st, 44, 39);
        ok &= chk("② 暗棋炸弹(蓝BOMB撞红3)→EV_BOMB+同归+moves_since_eat清零",
                  ev == EV_BOMB && st.moves_since_eat == 0
                  && st.cells[44].rank == RANK_EMPTY && st.cells[39].rank == RANK_EMPTY);
    }
    /* ③ 暗棋挖雷（蓝工兵挖红雷）→ EV_MINE + mines_cleared++ + moves_since_eat 清零 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[46].rank = RANK_WORKER; st.cells[46].side = SIDE_BLUE; st.cells[46].revealed = 1;
        st.cells[51].rank = RANK_MINE;  st.cells[51].side = SIDE_RED;  st.cells[51].revealed = 1;
        st.moves_since_eat = 69;
        int ev = jq_try_move(&st, 46, 51);
        ok &= chk("③ 暗棋挖雷(蓝工兵挖红雷)→EV_MINE+mines_cleared+++清零",
                  ev == EV_MINE && st.mines_cleared[SIDE_BLUE] == 1
                  && st.moves_since_eat == 0 && st.cells[51].rank == RANK_WORKER);
    }
    /* ④ 明棋 EV_DEAD 送死（蓝 rank2 撞红雷，雷不消失）→ 不清零 moves_since_eat（§B01-022） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 2; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = RANK_MINE; st.cells[39].side = SIDE_RED; st.cells[39].revealed = 1;
        st.moves_since_eat = 69;
        int ev = jq_try_move(&st, 44, 39);
        ok &= chk("④ 明棋EV_DEAD送死(蓝2撞红雷)→不清零 moves_since_eat(§B01-022)",
                  ev == EV_DEAD && st.moves_since_eat == 69
                  && st.cells[39].rank == RANK_MINE);  /* 雷 11 不消失 */
    }
    printf("dark_eat_reset_count_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
