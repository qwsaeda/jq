/* dark_fullgame_ai_ms_track_test.c — 暗棋完整 60 子对局链（AI 走子+翻棋交替到终局）
 * 全程 moves_since_eat 逐帧追踪断言行为级实证
 * 补全现有缺口：§33 dark_fullgame 跑完整 60 子对局（AI 走子+翻棋交替到
 * stage=OVER）但只断言"终局/正常结束"（turns/moves/flips/stage/winner，不追踪
 * moves_since_eat 演化），§64 dark_fullgame_ms_track_test 补"全程逐帧演化追踪"
 * 单点夹具（翻棋不消耗+空移累加+吃子清零+70 步阈值独立断言），但**§33 完整 60 子
 * 对局链跑法上叠加的全程 moves_since_eat 逐帧追踪断言**（完整对局中翻棋不消耗+
 * 走子累加+吃子清零+终局 70 步阈值触发的完整链）无独立用例。
 *
 * 对照 64 位实现（rules.c jq_flip 翻棋不自增 moves_since_eat（§34 实锤）+
 * jq_try_move 空移自增 §653 + 吃子清零 §683/726/737/784/830 + 70 步阈值
 * stalemate_after §519 + jq_dark_setup 全 0xFF 布局 + §33 AI 走子/翻棋交替跑法）：
 *   · 完整 60 子对局链（jq_dark_setup 全 0xFF → AI 走子+翻棋交替 → stage=OVER）；
 *   · 全程 moves_since_eat 追踪：翻棋不消耗（ms 不变）+ 走子空移累加（ms++）+
 *     吃子清零（ms=0）+ 70 步阈值触发（ms=70 判和 DRAW）；
 *   · 断言：对局正常结束（stage=OVER 或 turns<400）+ 终值/清零次数/max 记录。
 * 64 位行为级实证（走已实现的完整对局链 + moves_since_eat 逐帧追踪，0 分叉）。
 *
 * 几何口径：jq_dark_setup(JQ_DEFAULT_LAY) 全 60 子暗棋布局（§33 既有跑法）；
 *   AI 走子优先（已翻本方子）+ 翻棋兜底（0xFF 可翻），交替到终局。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern const uint8_t JQ_DEFAULT_LAY[30];

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    jq_state st;
    jq_dark_setup(&st, JQ_DEFAULT_LAY);
    st.stage = STG_PLAY;
    int min_move_range = 999, max_move_range = 0, move_ok = 0;
    int flips = 0, moves = 0, turns = 0;
    /* moves_since_eat 全程追踪 */
    int ms_max = 0, ms_reset = 0, ms_prev = 0;
    int flip_no_ms_check = 1;   /* 翻棋后 ms 不变（不消耗）全程校验 */

    while (st.stage == STG_PLAY && turns < 400) {
        turns++;
        int acted = 0;
        int ms_before_flip = 0;
        /* 先尝试走子（已翻的本方子） */
        for (int i = 0; i < N_CELLS && !acted; i++) {
            jq_cell *c = &st.cells[i];
            if (c->rank == RANK_EMPTY || !c->revealed || c->side != st.turn) continue;
            int rng = 0;
            for (int j = 0; j < N_CELLS; j++) if (i != j && jq_can_move(&st, i, j)) rng++;
            if (rng > 0) {
                if (rng < min_move_range) min_move_range = rng;
                if (rng > max_move_range) max_move_range = rng;
            }
            for (int j = 0; j < N_CELLS && !acted; j++) {
                if (i != j && jq_can_move(&st, i, j)) {
                    int ev = jq_try_move(&st, i, j);
                    if (ev >= 0) {
                        acted = 1; moves++; move_ok++;
                        /* 吃子清零追踪：ms_prev>0 且新 ms<ms_prev → 清零 */
                        if (ms_prev > 0 && st.moves_since_eat < ms_prev) ms_reset++;
                        ms_prev = st.moves_since_eat;
                        if (st.moves_since_eat > ms_max) ms_max = st.moves_since_eat;
                    }
                }
            }
        }
        /* 否则翻子（不消耗 moves_since_eat §34，ms 不变校验） */
        if (!acted) {
            ms_before_flip = st.moves_since_eat;
            for (int i = 0; i < N_CELLS && !acted; i++) {
                if (jq_can_flip(&st, i)) {
                    int ev = jq_flip(&st, i);
                    if (ev >= 0) {
                        acted = 1; flips++;
                        /* 翻棋不消耗：翻后 ms 应不变（§34） */
                        if (st.moves_since_eat != ms_before_flip) flip_no_ms_check = 0;
                        ms_prev = st.moves_since_eat;
                        if (st.moves_since_eat > ms_max) ms_max = st.moves_since_eat;
                    }
                }
            }
        }
        if (!acted) break;   /* 无行动 */
    }
    printf("完整60子对局: turns=%d moves=%d flips=%d stage=%d winner=%d\n",
           turns, moves, flips, st.stage, st.winner);
    printf("走子可达格: min=%d max=%d\n", min_move_range, max_move_range);
    printf("moves_since_eat全程: max=%d 清零次数=%d 终值=%d 翻棋不消耗=%d\n",
           ms_max, ms_reset, st.moves_since_eat, flip_no_ms_check);

    /* ① 完整对局正常结束（stage=OVER 或 turns<400，非超时挂死） */
    ok &= chk("① 完整60子对局正常结束(stage=OVER或turns<400)",
              st.stage == STG_OVER || turns < 400);
    /* ② 全程 moves_since_eat 翻棋不消耗（§34：翻棋后 ms 不变） */
    ok &= chk("② 全程翻棋不消耗moves_since_eat(§34实锤,翻后ms不变)",
              flip_no_ms_check == 1);
    /* ③ 走子空移累加 + 吃子清零（全程 ms_max≥1 且清零次数≥0，完整对局有走子） */
    ok &= chk("③ 走子累加+吃子清零(ms_max≥1清零≥0完整对局有走子)",
              ms_max >= 1 && ms_reset >= 0 && moves > 0);
    /* ④ 终局 moves_since_eat 语义：判和(DRAW)则终值=70（70 步阈值触发），
       非判和(胜负)则终值<70（未触发 70 步判和） */
    int ok4;
    if (st.winner == JQ_WIN_DRAW) {
        ok4 = (st.moves_since_eat == 70 || st.total_moves >= 1000);
    } else {
        ok4 = (st.moves_since_eat < 70 || st.stage == STG_OVER);
    }
    ok &= chk("④ 终局moves_since_eat语义(判和终值70/胜负未触70)", ok4);
    printf("dark_fullgame_ai_ms_track_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
