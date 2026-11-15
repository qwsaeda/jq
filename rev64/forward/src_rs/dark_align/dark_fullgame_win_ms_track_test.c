/* dark_fullgame_win_ms_track_test.c — 暗棋完整对局"胜负收尾"路径全程 moves_since_eat
 * 追踪断言行为级实证
 * 补全现有缺口：§65 dark_fullgame_ai_ms_track_test 实锤完整 60 子对局以**判和
 * DRAW** 收尾（moves_since_eat 终值=70 触发 70 步阈值），但**胜负收尾路径**
 * （靠扛旗/吃光困毙判胜，winner=BLUE/RED，非 70 步判和）的 moves_since_eat 追踪
 * （终值<70 + 扛旗/吃子清零打断 70 步进度 + winner 判胜）无独立用例——完整对局
 * 两种收尾（判和 vs 胜负）的 moves_since_eat 语义差异未独立断言。
 *
 * 对照 64 位实现（rules.c jq_try_move 扛旗/吃子清零 §683/726/737/784/830 +
 * jq_on_flag_capture 扛旗终局（winner 置 + stage=OVER）+ jq_on_stalemate 困毙
 * 判胜 + 70 步阈值 stalemate_after §519（仅判和路径 moves_since_eat>=70 触发））：
 *   · 判和收尾（§65 既有）：纯空移累加到 70 → stalemate_after 判 DRAW，终值=70；
 *   · 胜负收尾（本轮）：扛旗/吃光困毙判胜（winner=BLUE/RED），**非 70 步判和**
 *     ——扛旗/吃子清零打断 moves_since_eat 进度（终值<70），对局以 winner 判胜收
 *     尾（stalemate_after 的 70 步判和路径未触发）；
 *   · 两种收尾 moves_since_eat 语义差异：判和=终值 70（累加到阈值），胜负=终值
 *     <70（扛旗/吃子清零打断 + winner 判胜先于 70 步阈值）。
 * 64 位行为级实证（走已实现的扛旗终局/吃光困毙/清零路径，0 分叉）。
 *
 * 几何口径：胜负A 扛旗（col1 纵线蓝 rank5@51 攻 红军旗@56 大本营 which=3 开，
 *   §54 既有几何，红无其他子→扛旗即胜）；胜负B 吃光（col4 纵线蓝 rank1@49 攻
 *   红 rank9@44 相邻，红仅 rank9@44 无其他子→吃光后红困毙蓝胜）；mode=LAY 明
 *   棋（绕暗棋工兵门槛，胜负收尾纯几何+开关路径）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_flag_set(int v);
extern void ui_rule_mine_set(int v);
#define C51 cell_index(10,1)   /* 蓝攻子 */
#define C56 cell_index(11,1)   /* 红军旗大本营 */
#define C49 cell_index(9,4)    /* 蓝 rank1（col4 纵线） */
#define C44 cell_index(8,4)    /* 红 rank9（col4 纵线 44 相邻 49） */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* 胜负A：扛旗判胜（which=3 开，winner=BLUE，非 70 步判和，ms 终值<70） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        ui_rule_flag_set(1); ui_rule_mine_set(1);
        st.moves_since_eat = 3;   /* 模拟胜负收尾前已空移 3 步 */
        int ev = jq_try_move(&st, C51, C56);
        ok &= chk("胜负A 扛旗判胜(which=3开winner=BLUE非70步判和ms终值<70)",
                  ev == EV_FLAG && st.stage == STG_OVER
                  && st.winner == JQ_WIN_BLUE && st.moves_since_eat < 70);
    }
    /* 胜负B：吃光困毙判胜（蓝吃光红最后子，winner=BLUE，非 70 步判和，ms 清零
       终值<70） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C49].rank = 1; st.cells[C49].side = SIDE_BLUE; st.cells[C49].revealed = 1;
        st.cells[C44].rank = 9; st.cells[C44].side = SIDE_RED;  st.cells[C44].revealed = 1;
        ui_rule_flag_set(1); ui_rule_mine_set(1);
        st.moves_since_eat = 5;   /* 模拟胜负收尾前已空移 5 步 */
        int ev = jq_try_move(&st, C49, C44);
        ok &= chk("胜负B 吃光困毙判胜(蓝吃红最后子winner=BLUE ms清零<70)",
                  ev == EV_EAT && st.stage == STG_OVER
                  && st.winner == JQ_WIN_BLUE && st.moves_since_eat == 0);
    }
    /* 胜负C（对照 §65 判和收尾）：纯空移累加到 70 判 DRAW（winner=JQ_WIN_DRAW，
       ms 终值=70，区别于胜负收尾 winner=BLUE/RED 终值<70） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        ui_rule_flag_set(1); ui_rule_mine_set(1);
        st.moves_since_eat = 69;   /* 模拟已空移 69 步 */
        int ev = jq_try_move(&st, 44, 39);   /* 第 70 步空移判和 */
        ok &= chk("胜负C 对照判和收尾(纯空移累加70判DRAW winner=DRAW终值=70)",
                  ev == EV_MOVE && st.stage == STG_OVER
                  && st.winner == JQ_WIN_DRAW && st.moves_since_eat == 70);
    }
    ui_rule_flag_set(1); ui_rule_mine_set(1);   /* 复原默认全开 */
    printf("dark_fullgame_win_ms_track_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
