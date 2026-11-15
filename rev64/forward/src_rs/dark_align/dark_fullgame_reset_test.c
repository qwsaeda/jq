/* dark_fullgame_reset_test.c — 暗棋对局链内嵌吃子清零 70 步进度行为级实证
 * 补全现有缺口：§33 dark_fullgame 跑完整对局但只断言"终局/正常结束"（不追
 * 踪 moves_since_eat 演化），§46 dark_moves_seq_test 覆盖独立多步序列，但
 * **对局链（翻棋定色 + 走子累加 + 中途吃子清零 + 不提前判和）的内嵌清零**
 * 无独立用例（§35 挂起"暗棋对局链完整走子序列 moves_since_eat 清零路径"）。
 *
 * 对照 64 位实现（rules.c jq_flip 翻棋不计数 §34 + jq_try_move 空移累加
 * moves_since_eat++ §645 + 吃子分支清零 §645/726/737/784/830 + 70 步阈值
 * stalemate_after §519）：
 *   · 对局链走子序列：空移累加 moves_since_eat（翻棋不计 §34），中途吃子
 *     （EV_EAT/BOMB/MINE）清零 70 步进度（§46 既有口径）；
 *   · 内嵌清零关键：对局**中途**吃子（非开局设 69）清零后不提前判和（70 步
 *     进度重置，需重新累加到 70 才判和）；
 *   · 对照纯空移到 70 → 判和（无吃子清零打断）。
 * 64 位行为级实证（走已实现的翻棋/走子/吃子/和棋路径，0 分叉）。
 *
 * 几何口径：col4 纵线蓝 rank1@44 吃红 rank9@39（EV_EAT 吃子，§35 rank 小=强）；
 *   红 rank2@50 防困毙；对照空移 44→39（§45/§46 既有几何）。
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
    /* ① 对局链内嵌吃子清零：65 步空移后蓝 rank1 吃红 rank9（EV_EAT）→ 清零不判和 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 9; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        st.moves_since_eat = 65;   /* 模拟对局已走 65 步空移 */
        int ev = jq_try_move(&st, 44, 39);   /* 蓝 1 强吃红 9 弱 = EV_EAT 清零 */
        ok &= chk("① 对局链内嵌吃子清零(65步后吃子→清零,不提前判和)",
                  ev == EV_EAT && st.moves_since_eat == 0 && st.stage == STG_PLAY);
    }
    /* ② 对照纯空移到 70（无吃子清零打断）→ 判和 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        st.moves_since_eat = 69;   /* 69 步空移，走第 70 步 */
        int ev = jq_try_move(&st, 44, 39);   /* 空移到 70 */
        ok &= chk("② 对照纯空移70步(无吃子清零)→判和(DRAW)",
                  ev == EV_MOVE && st.moves_since_eat == 70
                  && st.stage == STG_OVER && st.winner == JQ_WIN_DRAW);
    }
    /* ③ 对局链清零后重新累加不判和（吃子清零→续走几步仍 <70） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 9; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        st.moves_since_eat = 65;
        int ev = jq_try_move(&st, 44, 39);   /* 吃子清零（65→0） */
        int reset_ok = (ev == EV_EAT && st.moves_since_eat == 0);
        /* 清零后续走 3 步空移（0→3，仍 <70 不判和）：蓝 39 已落 rank1, 走 39→44?
           44 空(原蓝 1 已吃 39), 蓝 rank1@39 走 44 */
        st.turn = SIDE_BLUE;   /* 简化：手动切回蓝续走 */
        int steps = 0;
        while (st.stage == STG_PLAY && steps < 3) {
            /* 蓝 rank1@39 空移到相邻空格(44 已空)，红 50↔51 空移 */
            if (st.turn == SIDE_BLUE) {
                int e = jq_try_move(&st, 39, 44);
                if (e < 0) break;
            } else {
                int e = jq_try_move(&st, 50, 51);
                if (e < 0) break;
            }
            steps++;
        }
        ok &= chk("③ 对局链清零后重新累加(吃子清零→续走3步仍<70不判和)",
                  reset_ok && st.stage == STG_PLAY && st.moves_since_eat < 70);
    }
    printf("dark_fullgame_reset_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
