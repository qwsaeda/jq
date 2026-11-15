/* dark_w1off_cont_test.c — which=1 关（大本营不冻结）+ which=3 关（扛旗不终局）
 * flag-off 对局持续演化走子链行为级实证
 * 补全现有缺口：§71 dark_flagoff_cont_test 实锤 which=1 开（大本营冻结）+
 * which=3 关 flag-off 后续走子触发蓝困毙判红胜（② 红走子后蓝困毙 OVER+RED）+
 * which=1 关对照（③ 蓝 cell56 可出大本营不困毙对局持续 PLAY 单点），但
 * **which=1 关 + which=3 关 flag-off 对局持续演化的完整走子链**（蓝扛旗入位
 * cell56 → 红走子 → 蓝出 cell56（which=1 关不冻结）→ 对局持续 PLAY 未终局）
 * 无独立用例——flag-off + which=1 关"对局持续演化"（非困毙判胜，区别于 §71
 * which=1 开困毙判胜）的完整走子链未独立断言。
 *
 * 对照 64 位实现（rules.c which=1 §237 大本营冻结门控（关=不冻结）+ which=3 关
 * else 分支 :737-748 扛旗不直接终局 + jq_side_stalemated 困毙扫描 §555（which=1
 * 关攻子可出大本营 → 守方有合法着法 → 不困毙））：
 *   · which=1 关 + which=3 关：蓝扛旗入位 cell56（which=3 关不终局）→ 红走子 →
 *     蓝出 cell56（which=1 关不冻结，56→55 放行）→ 对局持续 PLAY（winner=NONE，
 *     未终局）——flag-off 对局**持续演化**（守方攻子可出大本营，不困毙）；
 *   · 对照 which=1 开（§71 既有）：蓝扛旗入位 cell56 → 红走子 → 蓝 cell56 被
 *     which=1 冻结（56→55 拒）→ 蓝无合法着法 → 困毙红胜（OVER+RED）；
 *   · 关键差异：which=1 控"攻子入大本营后能否出"——关=可出（对局持续）/ 开=冻结
 *     （困毙判胜），which=3 关"扛旗不直接终局"下 which=1 开/关决定 flag-off
 *     对局"持续演化 vs 困毙判胜"。
 * 64 位行为级实证（走已实现的 which=1 冻结门控 + which=3 关 else 分支 + 困毙判定
 * 路径，0 分叉；几何走 §54/§70/§71 既有 LAY 明棋口径 + col1 纵线 cell51/56/55/50
 * 既有几何）。
 *
 * 几何口径：col1 纵线蓝 rank5@51 攻 红军旗 cell56(11,1) 大本营（§54/§70/§71 既有
 *   扛旗几何）；cell55(11,0) 大本营横邻（蓝出 cell56 目标）；红 rank1@50（守方
 *   可动子）；mode=LAY 明棋。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_mine_set(int v);
extern void ui_rule_flag_set(int v);
extern void ui_rule_hq_set(int v);
#define C51 cell_index(10,1)   /* 蓝攻子 */
#define C56 cell_index(11,1)   /* 红军旗大本营 */
#define C55 cell_index(11,0)   /* 大本营横邻（蓝出 cell56 目标） */
#define C50 cell_index(10,0)   /* 红守方可动子 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① which=1 关 + which=3 关 flag-off 持续演化走子链（蓝扛旗入位→红走子→
       蓝出 cell56→对局持续 PLAY） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(0); ui_rule_hq_set(0);   /* which=1关+3关 */
        jq_try_move(&st, C51, C56);   /* 蓝扛旗入位(which=3关不终局), turn→RED */
        int after_flag = st.stage;     /* PLAY */
        jq_try_move(&st, C50, C51);   /* 红走子 50→51, turn→BLUE */
        int ev_blue_out = jq_try_move(&st, C56, C55);   /* 蓝出 cell56(56→55,which=1关放行) */
        ok &= chk("① which=1关+3关 flag-off持续演化(蓝扛旗入位→红走子→蓝出cell56→PLAY)",
                  after_flag == STG_PLAY && ev_blue_out == EV_MOVE
                  && st.stage == STG_PLAY && st.winner == JQ_WIN_NONE);
    }
    /* ② 蓝出 cell56 后对局持续（cell56 变空 + cell55=蓝 5，蓝子在新位可再走） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(0); ui_rule_hq_set(0);
        jq_try_move(&st, C51, C56);
        jq_try_move(&st, C50, C51);
        jq_try_move(&st, C56, C55);
        /* 蓝 5 现 cell55（出大本营），对局持续演化（非终局） */
        ok &= chk("② 蓝出cell56后对局持续(cell55=蓝5+cell56空+PLAY未终局)",
                  st.cells[C55].rank == 5 && st.cells[C56].rank == RANK_EMPTY
                  && st.stage == STG_PLAY);
    }
    /* ③ 对照 which=1 开（§71 既有）：蓝扛旗入位→红走子→蓝 cell56 冻结(56→55 拒)
       → 蓝无合法着法 → 困毙红胜（OVER+RED） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(0); ui_rule_hq_set(1);   /* which=1开 */
        jq_try_move(&st, C51, C56);   /* 蓝扛旗入位, turn→RED */
        jq_try_move(&st, C50, C51);   /* 红走子, 蓝 cell56 被 which=1 冻结 → 困毙 */
        ok &= chk("③ 对照which=1开(§71): 蓝cell56冻结→困毙红胜(OVER+RED)",
                  st.stage == STG_OVER && st.winner == JQ_WIN_RED);
    }
    /* ④ 关键口径：which=1 控 flag-off 对局"持续演化 vs 困毙判胜"（which=3 关下
       攻子入大本营后能否出） */
    {
        jq_state st1; jq_state_init(&st1); st1.mode = JQ_MODE_LAY;
        memset(&st1.cells, 0, sizeof(st1.cells));
        st1.stage = STG_PLAY; st1.winner = JQ_WIN_NONE; st1.turn = SIDE_BLUE;
        st1.cells[C51].rank = 5; st1.cells[C51].side = SIDE_BLUE; st1.cells[C51].revealed = 1;
        st1.cells[C56].rank = RANK_FLAG; st1.cells[C56].side = SIDE_RED; st1.cells[C56].revealed = 1;
        st1.cells[C50].rank = 1; st1.cells[C50].side = SIDE_RED;  st1.cells[C50].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(0); ui_rule_hq_set(0);   /* which=1关 */
        jq_try_move(&st1, C51, C56); jq_try_move(&st1, C50, C51);
        int w1off_play = (st1.stage == STG_PLAY);

        jq_state st2; jq_state_init(&st2); st2.mode = JQ_MODE_LAY;
        memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[C51].rank = 5; st2.cells[C51].side = SIDE_BLUE; st2.cells[C51].revealed = 1;
        st2.cells[C56].rank = RANK_FLAG; st2.cells[C56].side = SIDE_RED; st2.cells[C56].revealed = 1;
        st2.cells[C50].rank = 1; st2.cells[C50].side = SIDE_RED;  st2.cells[C50].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(0); ui_rule_hq_set(1);   /* which=1开 */
        jq_try_move(&st2, C51, C56); jq_try_move(&st2, C50, C51);
        int w1on_over = (st2.stage == STG_OVER && st2.winner == JQ_WIN_RED);

        ok &= chk("④ 关键口径which=1控flag-off'持续演化vs困毙判胜'(关=PLAY/开=OVER+RED)",
                  w1off_play && w1on_over);
        ui_rule_mine_set(1); ui_rule_flag_set(1); ui_rule_hq_set(1);   /* 复原 */
    }
    ui_rule_mine_set(1); ui_rule_flag_set(1); ui_rule_hq_set(1);   /* 复原默认全开 */
    printf("dark_w1off_cont_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
