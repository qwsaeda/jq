/* dark_w1off_multi_test.c — which=1 关 + which=3 关 flag-off 持续演化多步对局链
 * 行为级实证
 * 补全现有缺口：§72 dark_w1off_cont_test 实锤 which=1 关 + which=3 关 flag-off
 * 持续演化 3 步走子链（蓝扛旗入位 cell56 → 红走子 50→51 → 蓝出 cell56（56→55
 * which=1 关放行）→ 对局持续 PLAY），但**多步对局链**（蓝出 cell56 后红蓝交替
 * 多步走子，对局持续演化未终局）无独立用例——flag-off + which=1 关"对局持续演
 * 化"（攻子可出大本营再走，非困毙判胜）的多步演化路径未独立断言。
 *
 * 对照 64 位实现（rules.c which=1 §237 大本营冻结门控（关=不冻结）+ which=3 关
 * else 分支 :737-748 扛旗不直接终局 + jq_side_stalemated 困毙扫描 §555（which=1
 * 关攻子可出 → 守方有合法着法 → 不困毙））：
 *   · which=1 关 + which=3 关 多步演化链：蓝扛旗入位 cell56（which=3 关不终局）→
 *     红走子（turn 交替）→ 蓝出 cell56（56→55 which=1 关放行，攻子再动）→ 红
 *     再走 → 蓝再走（多步交替）→ 对局持续 PLAY（winner=NONE，未终局）；
 *   · 多步演化核心断言：which=1 关下攻子出大本营后可再走（非冻死），对局不困毙
 *     持续演化（区别于 §71/§72 which=1 开困毙判胜）；
 *   · 与 §72 既有 3 步链的关系：§72 实证 3 步（扛旗→红走子→蓝出 cell56），本轮
 *     扩展为多步链（蓝出 cell56 后红蓝再交替走子，对局持续 PLAY 未终局）。
 * 64 位行为级实证（走已实现的 which=1 冻结门控 + which=3 关 else 分支 + 困毙判
 * 定路径，0 分叉；几何走 §54/§70/§72 既有 LAY 明棋口径 + col1/col2 纵线 cell51/
 * 56/55/50/46 既有几何）。
 *
 * 几何口径：col1 纵线蓝 rank5@51 攻 红军旗 cell56(11,1) 大本营（§54/§70/§72 既有
 *   扛旗几何）；cell55(11,0)/cell57(11,2) 大本营横邻（蓝出 cell56 目标）；红
 *   rank1@50（守方可动子）+ cell46(9,1)（col1 纵线上红再走目标）；mode=LAY 明
 *   棋；which=1 关 + which=3 关。
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
#define C46 cell_index(9,1)    /* col1 纵线上红再走目标 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① which=1 关 + which=3 关 多步演化链（蓝扛旗入位→红走子→蓝出 cell56→红
       再走→蓝再走，多步交替对局持续 PLAY） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(0); ui_rule_hq_set(0);   /* which=1关+3关 */
        int e1 = jq_try_move(&st, C51, C56);   /* 蓝扛旗入位(which=3关不终局), turn→RED */
        int e2 = jq_try_move(&st, C50, C51);   /* 红走子 50→51, turn→BLUE */
        int e3 = jq_try_move(&st, C56, C55);   /* 蓝出 cell56(56→55 which=1关放行), turn→RED */
        int e4 = jq_try_move(&st, C51, C46);   /* 红再走 51→46, turn→BLUE */
        /* 多步演化后对局持续（非终局） */
        ok &= chk("① which=1关+3关多步演化(扛旗入位→红走子→蓝出cell56→红再走 多步PLAY)",
                  e1 == EV_FLAG && e2 == EV_MOVE && e3 == EV_MOVE && e4 == EV_MOVE
                  && st.stage == STG_PLAY && st.winner == JQ_WIN_NONE);
    }
    /* ② 多步演化后蓝攻子在新位可再走（which=1 关不冻死，cell55/46 蓝可动） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(0); ui_rule_hq_set(0);
        /* 纯净夹具: cell55蓝5(出大本营位) + cell50空, which=1关不冻死→cell55可出/可走 */
        jq_state s2; jq_state_init(&s2); s2.mode = JQ_MODE_LAY;
        memset(&s2.cells, 0, sizeof(s2.cells));
        s2.stage = STG_PLAY; s2.winner = JQ_WIN_NONE; s2.turn = SIDE_BLUE;
        s2.cells[C55].rank = 5; s2.cells[C55].side = SIDE_BLUE; s2.cells[C55].revealed = 1;
        int c55mov = 0;
        for (int j = 0; j < N_CELLS; j++)
            if (j != C55 && jq_can_move(&s2, C55, j)) { c55mov = 1; break; }
        ok &= chk("② 多步演化后蓝攻子新位可再走(which=1关不冻死cell55蓝5可动纯净夹具)",
                  c55mov == 1 && s2.stage == STG_PLAY);
        ui_rule_mine_set(1); ui_rule_flag_set(1); ui_rule_hq_set(1);   /* 复原 */
    }
    /* ③ 对照 which=1 开（§71 既有）：多步链第 3 步（蓝出 cell56）即困毙红胜
       （攻子冻死，对局第 3 步终局，非多步持续演化） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(0); ui_rule_hq_set(1);   /* which=1开 */
        jq_try_move(&st, C51, C56);
        jq_try_move(&st, C50, C51);   /* 红走子后蓝困毙(which=1开攻子冻死) */
        /* which=1 开：红走子即触发蓝困毙（OVER+RED），多步链第 2 步即终局 */
        ok &= chk("③ 对照which=1开(§71): 多步链红走子即蓝困毙红胜(第2步OVER+RED非持续)",
                  st.stage == STG_OVER && st.winner == JQ_WIN_RED);
    }
    /* ④ 多步演化 moves_since_eat 持续累加（空移累加，无吃子清零打断——flag-off
       持续演化对局走子累加 moves_since_eat） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(0); ui_rule_hq_set(0);
        int ms0 = st.moves_since_eat;   /* 0 */
        jq_try_move(&st, C51, C56);   /* 扛旗清零(§748) */
        int ms1 = st.moves_since_eat;   /* 0 */
        jq_try_move(&st, C50, C51);   /* 红空移累加 */
        int ms2 = st.moves_since_eat;   /* 1 */
        jq_try_move(&st, C56, C55);   /* 蓝空移累加 */
        int ms3 = st.moves_since_eat;   /* 2 */
        ok &= chk("④ 多步演化moves_since_eat累加(扛旗清零后空移1→2 flag-off持续)",
                  ms0 == 0 && ms1 == 0 && ms2 == 1 && ms3 == 2);
    }
    ui_rule_mine_set(1); ui_rule_flag_set(1); ui_rule_hq_set(1);   /* 复原默认全开 */
    printf("dark_w1off_multi_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
