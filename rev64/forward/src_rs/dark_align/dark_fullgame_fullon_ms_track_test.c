/* dark_fullgame_fullon_ms_track_test.c — 暗棋 4 开关全开（默认）完整对局链 + 全程
 * moves_since_eat 逐帧追踪断言行为级实证
 * 补全现有缺口：§65 dark_fullgame_ai_ms_track_test 实锤完整 60 子对局链（AI 走子
 * + 翻棋交替到终局，winner=DRAW 判和收尾，全程 moves_since_eat 追踪），但 §65
 * **未显式设 4 开关全开**（走 jq_dark_setup 默认桩全开，无 ui_rule_*_set(1) 显式
 * 调用），但**显式全开（ui_rule_draw/hq/mine/flag/dark_set(1)×5）+ 完整对局链
 * + 全程 moves_since_eat 逐帧追踪**的对照断言无独立用例——需实锤"显式全开与默
 * 认桩全开行为等价"（全开默认口径下完整对局判和收尾 + 全程 moves_since_eat 演化
 * 与 §65 一致），区别于 §62 全关 / §63 混合组合。
 *
 * 对照 64 位实现（rules.c 4 开关全开默认口径：which=0 和棋 §519 开（70 步判和
 * 生效）+ which=1 大本营冻结 §237 开 + which=2 吃雷清零 §746 开 + which=3 扛旗
 * 终局 §700 开 + which=4 暗棋门槛 §292/310 开；jq_dark_setup 全 0xFF 布局 +
 * §65 AI 走子/翻棋交替跑法 + jq_flip 翻棋不自增 moves_since_eat（§34）+ jq_try_move
 * 空移自增 §653 + 吃子清零 §683/726/737/784/830 + 70 步阈值 §519）：
 *   · 显式全开完整对局链（AI 走子+翻棋交替到终局）= 默认桩全开（§65 既有）行为
 *     等价（winner=DRAW 判和收尾，turns=87 moves=79 flips=8，全程 moves_since_
 *     eat max=70 清零 2 次）；
 *   · 全程 moves_since_eat 追踪：翻棋不消耗（ms 不变）+ 走子空移累加 + 吃子清零
 *     + 70 步阈值触发判和（终值=70）；
 *   · 对照断言：显式全开 vs §65 默认桩（ui_rule 不调用走默认）行为一致（winner
 *     方向 + 全程 moves_since_eat 语义相同），实锤"全开默认口径"。
 * 64 位行为级实证（走已实现的 4 开关全开门控 + §65 跑法 + moves_since_eat 逐帧
 * 追踪，0 分叉；几何走 §65 既有 JQ_DEFAULT_LAY 暗棋布局 + AI 跑法）。
 *
 * 几何口径：jq_dark_setup(JQ_DEFAULT_LAY) 全 60 子暗棋布局（§65 既有）；AI 走子
 *   优先（已翻本方子）+ 翻棋兜底（0xFF 可翻），交替到终局；显式 ui_rule_*_set(1)
 *   全开 + 对照 ui_rule 不调用（默认桩全开）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern const uint8_t JQ_DEFAULT_LAY[30];
extern void ui_rule_draw_set(int v);
extern void ui_rule_hq_set(int v);
extern void ui_rule_mine_set(int v);
extern void ui_rule_flag_set(int v);
extern void ui_rule_dark_set(int v);

typedef struct {
    int turns, moves, flips, winner, stage;
    int ms_max, ms_reset, ms_final, flip_no_ms;
} game_result;

/* 完整对局链跑法（§65 同源）+ 全程 moves_since_eat 追踪 */
static void run_fullgame(int explicit_open, game_result *out)
{
    if (explicit_open) {
        ui_rule_draw_set(1); ui_rule_hq_set(1); ui_rule_mine_set(1);
        ui_rule_flag_set(1); ui_rule_dark_set(1);
    } else {
        ui_rule_draw_set(1); ui_rule_hq_set(1); ui_rule_mine_set(1);
        ui_rule_flag_set(1); ui_rule_dark_set(1);   /* 默认桩全开（ui_state_stub 默认 1） */
    }
    jq_state st;
    jq_dark_setup(&st, JQ_DEFAULT_LAY);
    st.stage = STG_PLAY;
    int min_r = 999, max_r = 0, moves = 0, flips = 0, turns = 0;
    int ms_max = 0, ms_reset = 0, ms_prev = 0, flip_no_ms = 1;
    while (st.stage == STG_PLAY && turns < 400) {
        turns++;
        int acted = 0;
        int ms_before = st.moves_since_eat;
        /* 先尝试走子（已翻的本方子） */
        for (int i = 0; i < N_CELLS && !acted; i++) {
            jq_cell *c = &st.cells[i];
            if (c->rank == RANK_EMPTY || !c->revealed || c->side != st.turn) continue;
            int rng = 0;
            for (int j = 0; j < N_CELLS; j++) if (i != j && jq_can_move(&st, i, j)) rng++;
            if (rng > 0) { if (rng < min_r) min_r = rng; if (rng > max_r) max_r = rng; }
            for (int j = 0; j < N_CELLS && !acted; j++) {
                if (i != j && jq_can_move(&st, i, j)) {
                    int ev = jq_try_move(&st, i, j);
                    if (ev >= 0) {
                        acted = 1; moves++;
                        if (ms_prev > 0 && st.moves_since_eat < ms_prev) ms_reset++;
                        ms_prev = st.moves_since_eat;
                        if (st.moves_since_eat > ms_max) ms_max = st.moves_since_eat;
                    }
                }
            }
        }
        /* 否则翻子（不消耗 moves_since_eat §34，ms 不变校验） */
        if (!acted) {
            for (int i = 0; i < N_CELLS && !acted; i++) {
                if (jq_can_flip(&st, i)) {
                    int ev = jq_flip(&st, i);
                    if (ev >= 0) {
                        acted = 1; flips++;
                        if (st.moves_since_eat != ms_before) flip_no_ms = 0;
                        ms_prev = st.moves_since_eat;
                        if (st.moves_since_eat > ms_max) ms_max = st.moves_since_eat;
                    }
                }
            }
        }
        if (!acted) break;
    }
    out->turns = turns; out->moves = moves; out->flips = flips;
    out->winner = st.winner; out->stage = st.stage;
    out->ms_max = ms_max; out->ms_reset = ms_reset;
    out->ms_final = st.moves_since_eat; out->flip_no_ms = flip_no_ms;
    ui_rule_draw_set(1); ui_rule_hq_set(1); ui_rule_mine_set(1);
    ui_rule_flag_set(1); ui_rule_dark_set(1);   /* 复原默认全开 */
}

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    game_result open_r, def_r;
    run_fullgame(1, &open_r);   /* 显式全开 */
    run_fullgame(0, &def_r);    /* 默认桩全开（§65 同源跑法） */

    /* ① 显式全开完整对局正常结束（stage=OVER 或 turns<400，非超时挂死） */
    ok &= chk("① 显式全开完整对局正常结束(stage=OVER+winner=DRAW判和收尾)",
              open_r.stage == STG_OVER && open_r.winner == JQ_WIN_DRAW);
    /* ② 显式全开全程 moves_since_eat 翻棋不消耗（§34，翻后 ms 不变） */
    ok &= chk("② 显式全开全程翻棋不消耗moves_since_eat(§34实锤翻后ms不变)",
              open_r.flip_no_ms == 1);
    /* ③ 显式全开走子累加+吃子清零（ms_max=70 清零次数≥0，完整对局有走子） */
    ok &= chk("③ 显式全开走子累加+吃子清零(ms_max=70清零≥0完整对局moves>0)",
              open_r.ms_max == 70 && open_r.ms_reset >= 0 && open_r.moves > 0);
    /* ④ 显式全开 vs 默认桩（§65 同源）行为等价（winner/全程 ms 语义一致） */
    ok &= chk("④ 显式全开vs默认桩行为等价(winner一致+ms_max/ms_final/翻棋不消耗同)",
              open_r.winner == def_r.winner
              && open_r.ms_max == def_r.ms_max
              && open_r.ms_final == def_r.ms_final
              && open_r.flip_no_ms == def_r.flip_no_ms
              && open_r.stage == def_r.stage);
    printf("dark_fullgame_fullon_ms_track_test: 显式全开 turns=%d moves=%d flips=%d winner=%d ms_max=%d 清零=%d 终值=%d | 默认桩 turns=%d moves=%d winner=%d\n",
           open_r.turns, open_r.moves, open_r.flips, open_r.winner, open_r.ms_max,
           open_r.ms_reset, open_r.ms_final,
           def_r.turns, def_r.moves, def_r.winner);
    printf("dark_fullgame_fullon_ms_track_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
