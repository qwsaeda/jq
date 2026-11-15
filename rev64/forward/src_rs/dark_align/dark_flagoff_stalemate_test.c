/* dark_flagoff_stalemate_test.c — which=3（扛旗开关）开/关 下困毙分界行为级实证
 * 补全现有缺口：§37 dark_rule_switch_test 覆盖 which=3 单开关（扛旗终局 vs
 * 走子不终局），§53 dark_stalemate_matrix_test 覆盖困毙扫描白名单，但
 * **which=3 关（flag-off）下"扛旗不终局 → 攻子冻大本营 → 靠困毙/吃光近似
 * 判胜负"的完整分界**未独立用例（§35 挂起"flag-off/mine-off 走单侧困毙
 * 路径产出相同 winner"）。
 *
 * 对照 64 位实现（rules.c §700 which=3 `if(cfg(3))jq_on_flag_capture` 终局 /
 * else 走子正常收口不置终局 + 诚实标注"开关关闭时旗被吃而局不终，后续胜负
 * 只能靠困毙/吃光可动子近似" + §748 通用收口 stage!=PLAY 跳过 turn）：
 *   · which=3 开：扛旗 → jq_on_flag_capture → 终局（winner 置，stage=OVER）；
 *   · which=3 关：扛旗走子正常收口（攻子入位 cell56）但**不置终局**
 *     （stage=PLAY, winner=NONE）；
 *   · which=3 关 + 对方无其他可动子：扛旗后对方困毙 → jq_on_stalemate 判
 *     攻方胜（靠困毙近似，非扛旗直接终局）。
 * 64 位行为级实证（走已实现的 §700 which=3 分支 + §555 jq_on_stalemate 困毙
 * 路径，0 分叉）。
 *
 * 几何口径：col1 纵线蓝攻子 cell51(10,1) 攻 红军旗 cell56(11,1) 大本营
 *   （§32/§44/§49 既有几何）；红防困毙用 cell50(10,0) 红子。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_flag_set(int v);
#define C51 cell_index(10,1)   /* 蓝攻子 */
#define C56 cell_index(11,1)   /* 红军旗大本营 */
#define C50 cell_index(10,0)   /* 红防困毙 */

static int chk(char *name, int got, int want)
{
    int ok = (got == want);
    printf("%s: got=%d want=%d → %s\n", name, got, want, ok ? "PASS" : "FAIL");
    return ok;
}

int main(void)
{
    int ok = 1;
    /* ① which=3 开：扛旗终局（winner=BLUE, stage=OVER） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED; st.cells[C50].revealed = 1;
        ui_rule_flag_set(1);
        int ev = jq_try_move(&st, C51, C56);
        ok &= chk("① which=3开: 扛旗终局(EV_FLAG+OVER+BLUE)",
                  ev == EV_FLAG && st.stage == STG_OVER && st.winner == JQ_WIN_BLUE, 1);
    }
    /* ② which=3 关：扛旗不终局（攻子入位 cell56, stage=PLAY winner=NONE） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED; st.cells[C50].revealed = 1;
        ui_rule_flag_set(0);
        int ev = jq_try_move(&st, C51, C56);
        int ok_ = (ev == EV_FLAG && st.stage == STG_PLAY && st.winner == JQ_WIN_NONE
                   && st.cells[C56].rank == 5);   /* 攻子 5 入位 */
        ui_rule_flag_set(1);   /* 复原 */
        ok &= chk("② which=3关: 扛旗不终局(EV_FLAG+PLAY+NONE,攻子入位)", ok_, 1);
    }
    /* ③ which=3 关 + 红无其他可动子：扛旗后红困毙 → 蓝胜（困毙近似，非直接终局） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        /* 红仅旗@56（被扛后红无其他子）—— 扛旗后红无子可走 → 困毙 */
        ui_rule_flag_set(0);
        int ev = jq_try_move(&st, C51, C56);
        int ok_ = (st.stage == STG_OVER && st.winner == JQ_WIN_BLUE);
        ui_rule_flag_set(1);   /* 复原 */
        ok &= chk("③ which=3关+红无子: 扛旗后红困毙→蓝胜(困毙近似)", ok_, 1);
        (void)ev;
    }
    /* ④ which=3 关 + 红有可动子：扛旗后红不困毙 → 对局继续（stage=PLAY） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED; st.cells[C50].revealed = 1;
        /* 红有 rank1@50 可动 → 扛旗后红不困毙，对局继续 */
        ui_rule_flag_set(0);
        int ev = jq_try_move(&st, C51, C56);
        int ok_ = (st.stage == STG_PLAY && st.winner == JQ_WIN_NONE);
        ui_rule_flag_set(1);   /* 复原 */
        ok &= chk("④ which=3关+红有可动子: 扛旗后红不困毙→对局继续(PLAY)", ok_, 1);
        (void)ev;
    }
    printf("dark_flagoff_stalemate_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
