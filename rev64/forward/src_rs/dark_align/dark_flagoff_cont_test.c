/* dark_flagoff_cont_test.c — which=3 关扛旗不终局后"对局继续"的后续走子路径行为级实证
 * 补全现有缺口：§70 dark_flagoff_clear_test 实锤 which=3 关扛旗路径清零（ms 3→0，
 * 红有可动子 PLAY / 红无子困毙近似），但**which=3 关扛旗不终局后"对局继续"的
 * 后续走子路径**（攻子入位 cell56 后回合交替 + 后续走子触发对方困毙判胜 / 对局
 * 持续演化）无独立用例——which=3 关扛旗不终局后红走子触发蓝困毙（攻子入位被
 * which=1 冻结 + 目标格阻断 → 蓝无合法着法 → 红胜，flag-off 对局持续演化的终局
 * 路径）未独立断言。
 *
 * 对照 64 位实现（rules.c which=3 关 else 分支 :737-748 攻子入位不终局 +
 * stalemate_after 困毙判定 + which=1 §237 大本营冻结 + jq_side_stalemated 困毙
 * 扫描 §555）：
 *   · which=3 关扛旗不终局（§70 既有）：攻子入位 cell56（蓝 5），turn 切红，
 *     stage=PLAY（不直接终局），ms 清零（§748 同步）；
 *   · 后续红走子（cell50→51）：红方走子后 stalemate_after 判定蓝方困毙——蓝方
 *     唯一子 cell56=5 被 which=1 大本营冻结（§237，which=1 开不能出大本营）+
 *     cell51 被红 1 占据（阻断蓝 5 出 col1 纵线路径）→ 蓝无合法着法 → 红胜
 *     （winner=RED，stage=OVER，困毙近似判胜——非 which=3 开扛旗直接终局，而是
 *     which=3 关路径靠后续走子触发对方困毙判胜）；
 *   · 关键口径：which=3 关"扛旗不直接终局"≠"对局无终局"——攻子入位后若对方（守
 *     方）因攻子入位 + which=1 冻结 + 目标格阻断而**无合法着法**，则后续走子
 *     触发对方困毙判攻方胜（flag-off 对局持续演化终局路径，区别于 which=3 开
 *     扛旗直接终局）。
 * 64 位行为级实证（走已实现的 which=3 关 else 分支 + which=1 冻结 + 困毙判定
 * 路径，0 分叉；几何走 §54/§70 既有 LAY 明棋口径 + which=1 开大本营冻结）。
 *
 * 几何口径：col1 纵线蓝 rank5@51 攻 红军旗 cell56(11,1) 大本营（§54/§70 既有
 *   扛旗几何）；红 rank1@50（守方唯一可动子，扛旗后走 50→51）；which=3 关 +
 *   which=1 开（大本营冻结）；mode=LAY 明棋。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_mine_set(int v);
extern void ui_rule_flag_set(int v);
extern void ui_rule_hq_set(int v);
#define C51 cell_index(10,1)   /* 蓝攻子 */
#define C56 cell_index(11,1)   /* 红军旗大本营 */
#define C50 cell_index(10,0)   /* 红守方唯一可动子 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① which=3 关扛旗不终局（攻子入位 cell56，§70 既有）：stage=PLAY + turn
       切红 + ms 清零 + cell56=5 攻子入位 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(0); ui_rule_hq_set(1);   /* which=3关+which=1开 */
        st.moves_since_eat = 3;
        int ev = jq_try_move(&st, C51, C56);
        ok &= chk("① which=3关扛旗不终局(攻子入位cell56 PLAY+turn红+ms清零§70)",
                  ev == EV_FLAG && st.stage == STG_PLAY
                  && st.winner == JQ_WIN_NONE && st.turn == SIDE_RED
                  && st.moves_since_eat == 0 && st.cells[C56].rank == 5);
    }
    /* ② 后续红走子（cell50→51）触发蓝困毙判红胜（flag-off 对局持续演化终局路径） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(0); ui_rule_hq_set(1);
        jq_try_move(&st, C51, C56);   /* ① 蓝扛旗入位, turn→RED */
        jq_try_move(&st, C50, C51);   /* ② 红走子 50→51 */
        ok &= chk("② 后续红走子触发蓝困毙判红胜(flag-off对局持续演化OVER+RED)",
                  st.stage == STG_OVER && st.winner == JQ_WIN_RED
                  && st.cells[C51].rank == 1);   /* 红 1 入位 cell51 */
    }
    /* ③ 对照 which=1 关（大本营不冻结）：扛旗后蓝 cell56 可出大本营（不困毙） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(0); ui_rule_hq_set(0);   /* which=1关 不冻结 */
        jq_try_move(&st, C51, C56);   /* 蓝扛旗入位, turn→RED */
        jq_try_move(&st, C50, C51);   /* 红走子 50→51 */
        /* which=1 关：蓝 cell56 不被冻结，红走子后蓝仍可出 cell56（不困毙） */
        int blue_movable = 0;
        for (int i = 0; i < N_CELLS; i++)
            if (st.cells[i].rank != RANK_EMPTY && st.cells[i].side == SIDE_BLUE
                && st.cells[i].revealed)
                for (int j = 0; j < N_CELLS; j++)
                    if (i != j && jq_can_move(&st, i, j)) { blue_movable = 1; break; }
        ok &= chk("③ 对照which=1关(不冻结): 扛旗后蓝cell56可出大本营(不困毙,对局持续PLAY)",
                  st.stage == STG_PLAY && blue_movable == 1);
    }
    /* ④ 关键口径：which=3 关"扛旗不直接终局"≠"对局无终局"（攻子入位后对方困毙
       仍判攻方胜，区别于 which=3 开扛旗直接终局） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        /* 红无其他子（无 cell50）→ 扛旗后红困毙（§70 ② 既有，which=3 关直接困毙判胜） */
        ui_rule_mine_set(1); ui_rule_flag_set(0); ui_rule_hq_set(1);
        jq_try_move(&st, C51, C56);
        ok &= chk("④ 关键口径which=3关扛旗不直接终局但对方困毙仍判胜(红无子OVER+BLUE§70)",
                  st.stage == STG_OVER && st.winner == JQ_WIN_BLUE);
    }
    ui_rule_mine_set(1); ui_rule_flag_set(1); ui_rule_hq_set(1);   /* 复原默认全开 */
    printf("dark_flagoff_cont_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
