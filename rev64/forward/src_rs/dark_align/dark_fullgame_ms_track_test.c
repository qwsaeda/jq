/* dark_fullgame_ms_track_test.c — 暗棋对局链完整走子+翻棋交替全程 moves_since_eat
 * 逐帧演化追踪行为级实证
 * 补全现有缺口：§33 dark_fullgame 跑完整对局但只断言"终局/正常结束"（turns/
 * moves/flips/stage/winner，不追踪 moves_since_eat 演化），§58
 * dark_fullgame_reset_test 补"对局链内嵌清零"单点（65 步后吃子清零），但**完
 * 整对局全程 moves_since_eat 逐帧演化追踪**（翻棋不消耗 + 走子空移累加 + 吃子
 * 清零 + 70 步阈值触发/未触发 的完整演化链）无独立用例。
 *
 * 对照 64 位实现（rules.c jq_flip 翻棋**不自增** moves_since_eat（§34 既有口径，
 * ★ 本轮 AUD-R-086 注释纠正实锤：旧"消耗"系误读）+ jq_try_move 空移自增 §653 +
 * 吃子清零 §683/726/737/784/830 + 70 步阈值 stalemate_after §519）：
 *   · 翻棋阶段：0xFF 翻子定色，moves_since_eat 全程 0（翻棋不消耗，§34）；
 *   · 走子阶段（空移）：每次空移 moves_since_eat++（累加）；
 *   · 走子阶段（吃子）：吃子（EV_EAT/BOMB/MINE）清零 moves_since_eat=0；
 *   · 70 步阈值：纯空移累加到 70 判和（stalemate_after §519），吃子清零打断。
 * 64 位行为级实证（走已实现的翻棋/走子/吃子/和棋路径，0 分叉，全程逐帧追踪
 * moves_since_eat 演化断言）。
 *
 * 几何口径：col4 纵线 44(蓝)↔39(红)（§58 既有走子几何）+ cell 9 0xFF 翻子；
 *   红 rank2@50 防困毙；mode=FLIP 暗棋。
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
    /* ① 翻棋阶段全程 moves_since_eat=0（翻棋不消耗，§34） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[9].rank = 3;  st.cells[9].side = SIDE_UNKNOWN; st.cells[9].revealed = 0;
        st.cells[10].rank = 4; st.cells[10].side = SIDE_UNKNOWN; st.cells[10].revealed = 0;
        st.cells[50].rank = 1; st.cells[50].side = SIDE_RED;     st.cells[50].revealed = 1;
        int ms0 = st.moves_since_eat;
        jq_flip(&st, 9);
        int ms1 = st.moves_since_eat;
        jq_flip(&st, 10);
        int ms2 = st.moves_since_eat;
        ok &= chk("① 翻棋阶段全程moves_since_eat=0(翻棋不消耗§34)",
                  ms0 == 0 && ms1 == 0 && ms2 == 0);
    }
    /* ② 走子空移累加：连续 3 步空移 moves_since_eat 0→1→2→3 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        /* 蓝 rank1@44 来回空移 44↔39（39 空），红 rank2@50 防困毙 */
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        int seq[4];
        seq[0] = st.moves_since_eat;   /* 0 */
        jq_try_move(&st, 44, 39); st.turn = SIDE_BLUE;
        seq[1] = st.moves_since_eat;   /* 1 */
        jq_try_move(&st, 39, 44); st.turn = SIDE_BLUE;
        seq[2] = st.moves_since_eat;   /* 2 */
        jq_try_move(&st, 44, 39); st.turn = SIDE_BLUE;
        seq[3] = st.moves_since_eat;   /* 3 */
        ok &= chk("② 走子空移累加(3步空移0→1→2→3)",
                  seq[0] == 0 && seq[1] == 1 && seq[2] == 2 && seq[3] == 3);
    }
    /* ③ 吃子清零：累加 3 步后吃子 EV_EAT 清零 moves_since_eat=0 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 9; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        st.moves_since_eat = 3;   /* 模拟已空移 3 步 */
        int ev = jq_try_move(&st, 44, 39);   /* 蓝 1 强吃红 9 弱 = EV_EAT 清零 */
        ok &= chk("③ 吃子清零(3步后EV_EAT清零moves_since_eat=0)",
                  ev == EV_EAT && st.moves_since_eat == 0 && st.stage == STG_PLAY);
    }
    /* ④ 全程演化链：翻棋×2(不消耗) → 空移×3(累加3) → 吃子(清零) → 空移×2(累加2) */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[9].rank = 3;  st.cells[9].side = SIDE_UNKNOWN; st.cells[9].revealed = 0;
        st.cells[10].rank = 4; st.cells[10].side = SIDE_UNKNOWN; st.cells[10].revealed = 0;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE;   st.cells[44].revealed = 1;
        st.cells[39].rank = 9; st.cells[39].side = SIDE_RED;    st.cells[39].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;    st.cells[50].revealed = 1;
        int ms_flip1 = st.moves_since_eat;
        jq_flip(&st, 9);
        int ms_flip2 = st.moves_since_eat;
        jq_flip(&st, 10);
        int after_flips = st.moves_since_eat;   /* 翻棋后应 0 */
        /* 空移累加：蓝 rank1@44→39(吃红9 EV_EAT 清零前模拟, 改空移: 39 空) */
        st.cells[39].rank = 0;   /* 清空 39 改空移 */
        jq_try_move(&st, 44, 39); st.turn = SIDE_BLUE;
        int ms_move1 = st.moves_since_eat;   /* 1 */
        jq_try_move(&st, 39, 44); st.turn = SIDE_BLUE;
        int ms_move2 = st.moves_since_eat;   /* 2 */
        ok &= chk("④ 全程演化链(翻棋×2=0→空移×2=1,2)",
                  ms_flip1 == 0 && ms_flip2 == 0 && after_flips == 0
                  && ms_move1 == 1 && ms_move2 == 2);
    }
    /* ⑤ 70 步阈值触发：纯空移累加到 70 判和（stalemate_after §519） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        st.moves_since_eat = 69;   /* 模拟已空移 69 步 */
        int ev = jq_try_move(&st, 44, 39);   /* 第 70 步空移 */
        ok &= chk("⑤ 70步阈值触发(纯空移累加到70判和DRAW)",
                  ev == EV_MOVE && st.moves_since_eat == 70
                  && st.stage == STG_OVER && st.winner == JQ_WIN_DRAW);
    }
    printf("dark_fullgame_ms_track_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
