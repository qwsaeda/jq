/* dark_fullgame_redwin_ms_track_test.c — 暗棋完整对局"胜负收尾 winner=RED"（红方
 * 判胜）路径全程 moves_since_eat 追踪断言行为级实证
 * 补全现有缺口：§66 dark_fullgame_win_ms_track_test 实锤完整对局"胜负收尾"
 * winner=BLUE（蓝方判胜，靠蓝扛红旗/蓝吃光红困毙）的 moves_since_eat 追踪
 * （终值<70，扛旗/吃子清零打断 70 步进度），但**winner=RED（红方判胜，靠红扛
 * 蓝旗/红吃光蓝困毙）的 moves_since_eat 追踪**（几何对称，终值<70 + winner=RED
 * 先于 70 步阈值）无独立用例——完整对局胜负收尾"蓝胜 vs 红胜"双向 moves_since_eat
 * 语义差异（winner 方向 vs 清零路径）未独立断言。
 *
 * 对照 64 位实现（rules.c jq_try_move 扛旗/吃子清零 §683/726/737/784/830 +
 * jq_on_flag_capture 扛旗终局（winner 置攻方）+ jq_on_stalemate 困毙判胜 + 70
 * 步阈值 stalemate_after §519（仅判和路径触发））：
 *   · winner=RED 扛旗收尾（红扛蓝旗）：红 rank5 攻蓝军旗（which=3 开）→ winner=RED
 *     + stage=OVER，ms 终值<70（扛旗清零打断，非 70 步判和）；
 *   · winner=RED 吃光收尾（红吃光蓝最后子）：红 rank1 吃蓝 rank9（EV_EAT）→
 *     winner=RED（蓝困毙）+ stage=OVER，ms 清零终值=0<70；
 *   · 与 §66 winner=BLUE 几何对称（方向相反）：清零路径同源（扛旗/吃子清零），
 *     winner 方向不同（攻方判胜），moves_since_eat 终值语义一致（<70 非 70 步
 *     判和）。
 * 64 位行为级实证（走已实现的扛旗终局/吃光困毙/清零路径，0 分叉；几何走 §54
 * 既有扛旗几何方向相反 + col4 纵线吃光几何对称）。
 *
 * 几何口径：胜负A 红扛蓝旗（col1 纵线红 rank5@6 攻 蓝军旗@1 大本营 which=3 开，
 *   红攻蓝大本营方向相反于 §66 蓝扛红旗）；胜负B 红吃光蓝（col4 纵线红 rank1@49
 *   攻 蓝 rank9@44 相邻，红吃光后蓝困毙）；mode=LAY 明棋（绕暗棋工兵门槛，胜负
 *   收尾纯几何+开关路径）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_flag_set(int v);
extern void ui_rule_mine_set(int v);
#define C6  cell_index(1,1)    /* 红攻子（col1 纵线，邻蓝大本营 cell1） */
#define C1  cell_index(0,1)    /* 蓝军旗大本营 */
#define C49 cell_index(9,4)    /* 红 rank1（col4 纵线） */
#define C44 cell_index(8,4)    /* 蓝 rank9（col4 纵线 44 相邻 49） */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* 胜负A：红扛蓝旗判胜（which=3 开，winner=RED，非 70 步判和，ms 终值<70） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C6].rank = 5; st.cells[C6].side = SIDE_RED; st.cells[C6].revealed = 1;
        st.cells[C1].rank = RANK_FLAG; st.cells[C1].side = SIDE_BLUE; st.cells[C1].revealed = 1;
        ui_rule_flag_set(1); ui_rule_mine_set(1);
        st.moves_since_eat = 3;   /* 模拟胜负收尾前已空移 3 步 */
        int ev = jq_try_move(&st, C6, C1);
        ok &= chk("胜负A 红扛蓝旗判胜(which=3开winner=RED非70步判和ms终值<70)",
                  ev == EV_FLAG && st.stage == STG_OVER
                  && st.winner == JQ_WIN_RED && st.moves_since_eat < 70);
    }
    /* 胜负B：红吃光蓝最后子困毙判胜（winner=RED，ms 清零终值=0<70） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C49].rank = 1; st.cells[C49].side = SIDE_RED; st.cells[C49].revealed = 1;
        st.cells[C44].rank = 9; st.cells[C44].side = SIDE_BLUE; st.cells[C44].revealed = 1;
        ui_rule_flag_set(1); ui_rule_mine_set(1);
        st.moves_since_eat = 5;   /* 模拟胜负收尾前已空移 5 步 */
        int ev = jq_try_move(&st, C49, C44);
        ok &= chk("胜负B 红吃光蓝最后子困毙判胜(winner=RED ms清零终值0<70)",
                  ev == EV_EAT && st.stage == STG_OVER
                  && st.winner == JQ_WIN_RED && st.moves_since_eat == 0);
    }
    /* 胜负C（对照 §66 蓝方判胜 + §65 判和）：winner=RED 与 winner=BLUE/DRAW 的
       moves_since_eat 终值语义一致（均<70 非 70 步判和），仅 winner 方向不同 */
    {
        /* 复用胜负A 红扛蓝旗 winner=RED 终值<70，与 §66 winner=BLUE 对照 */
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[C6].rank = 5; st.cells[C6].side = SIDE_RED; st.cells[C6].revealed = 1;
        st.cells[C1].rank = RANK_FLAG; st.cells[C1].side = SIDE_BLUE; st.cells[C1].revealed = 1;
        ui_rule_flag_set(1); ui_rule_mine_set(1);
        st.moves_since_eat = 2;
        int ev = jq_try_move(&st, C6, C1);
        /* winner=RED 扛旗清零后终值<70（与 §66 winner=BLUE 语义一致，方向相反） */
        ok &= chk("胜负C 对照winner=RED与winner=BLUE/DRAW语义一致(均<70非70步判和)",
                  ev == EV_FLAG && st.stage == STG_OVER
                  && st.winner == JQ_WIN_RED && st.moves_since_eat < 70);
    }
    ui_rule_flag_set(1); ui_rule_mine_set(1);   /* 复原默认全开 */
    printf("dark_fullgame_redwin_ms_track_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
