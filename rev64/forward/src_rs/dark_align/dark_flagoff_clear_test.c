/* dark_flagoff_clear_test.c — 吃子清零 which=3 关路径（EV_FLAG :748 else 分支
 * moves_since_eat=0 同步）独立用例行为级实证
 * 补全现有缺口：§69 dark_4branch_clear_test 实锤 which=3 开路径（EV_FLAG :737/
 * :748 终局分支清零），但**which=3 关路径（EV_FLAG else 分支 :737-748 攻子入位
 * 不终局 + moves_since_eat=0 清零同步）**无独立用例——AUD-R-107 标注"EV_FLAG
 * 开关关闭路径也含计数=0 需保持同步"，which=3 关扛旗不终局路径（攻子入位 +
 * 困毙判定 + moves_since_eat 清零）的独立断言未覆盖。
 *
 * 对照 64 位实现（rules.c :720 `if (ui_rule_cfg_get(3))` 开分支（终局 + :737
 * 清零）/ else 分支（which=3 关 :737-748 攻子入位不终局 + stalemate_after +
 * moves_since_eat=0 清零同步，AUD-R-107 标注需与开分支保持同步））：
 *   · which=3 关 + 红有可动子：扛旗走子正常收口（攻子入位 cell56），**不置终局**
 *     （stage=PLAY，winner=NONE），但 moves_since_eat 清零（§748 同步，EV_FLAG
 *     事件仍清零 70 步进度——扛旗活动重置无吃子计数，与 which=3 开终局清零对称）；
 *   · which=3 关 + 红无其他子：扛旗后红困毙 → jq_on_stalemate 判攻方胜（winner
 *     = 攻方，stage=OVER），靠困毙近似（非扛旗直接终局，§54 既有口径）+
 *     moves_since_eat 清零；
 *   · 与 which=3 开对称：which=3 开扛旗直接终局（jq_on_flag_capture 置 winner
 *     + stage=OVER），which=3 关扛旗不直接终局（靠困毙近似 + stalemate_after
 *     判定），但**两者均清零 moves_since_eat**（EV_FLAG 事件无论终局与否都清零
 *     70 步进度，AUD-R-107"4 分支各含一份清零"含 which=3 关 else 分支）。
 * 64 位行为级实证（走已实现的 which=3 开/关 EV_FLAG 分支 + :748 清零同步路径，
 * 0 分叉；几何走 §54/§69 既有 LAY 明棋口径）。
 *
 * 几何口径：col1 纵线蓝 rank5@51 攻 红军旗 cell56(11,1) 大本营（§54/§69 既有
 *   扛旗几何）；红 cell50(10,0) 防困毙（which=3 关红有可动子→不终局）/ 红无
 *   cell50（which=3 关红无其他子→困毙近似）；mode=LAY 明棋（绕暗棋工兵门槛）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_mine_set(int v);
extern void ui_rule_flag_set(int v);
#define C51 cell_index(10,1)   /* 蓝攻子 */
#define C56 cell_index(11,1)   /* 红军旗大本营 */
#define C50 cell_index(10,0)   /* 红防困毙（有=可动子/无=困毙） */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① which=3 关 + 红有可动子：扛旗不终局（攻子入位 PLAY）+ moves_since_eat
       清零（§748 同步） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1; /* 红有可动子 */
        ui_rule_mine_set(1); ui_rule_flag_set(0);   /* which=3 关 */
        st.moves_since_eat = 3;
        int ev = jq_try_move(&st, C51, C56);
        ok &= chk("① which=3关+红有可动子: 扛旗不终局(PLAY)且ms清零(§748同步)",
                  ev == EV_FLAG && st.stage == STG_PLAY
                  && st.winner == JQ_WIN_NONE && st.moves_since_eat == 0
                  && st.cells[C56].rank == 5);   /* 攻子入位 */
    }
    /* ② which=3 关 + 红无其他子：扛旗后红困毙 → 蓝胜（困毙近似 §54）+ ms 清零 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        /* 红仅旗@56（无 cell50），扛旗后红无其他子 → 困毙 */
        ui_rule_mine_set(1); ui_rule_flag_set(0);   /* which=3 关 */
        st.moves_since_eat = 3;
        jq_try_move(&st, C51, C56);
        ok &= chk("② which=3关+红无其他子: 扛旗后红困毙→蓝胜(§54困毙近似)+ms清零",
                  st.stage == STG_OVER && st.winner == JQ_WIN_BLUE
                  && st.moves_since_eat == 0);
    }
    /* ③ 对照 which=3 开 + 红无其他子：扛旗直接终局（§69 既有，winner=BLUE
       + OVER）+ ms 清零（与 ② 对称，which=3 开/关均清零） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(1);   /* which=3 开 */
        st.moves_since_eat = 3;
        int ev = jq_try_move(&st, C51, C56);
        ok &= chk("③ 对照which=3开+红无其他子: 扛旗直接终局(OVER+BLUE)+ms清零(与②对称)",
                  ev == EV_FLAG && st.stage == STG_OVER
                  && st.winner == JQ_WIN_BLUE && st.moves_since_eat == 0);
    }
    ui_rule_mine_set(1); ui_rule_flag_set(1);   /* 复原默认全开 */
    printf("dark_flagoff_clear_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
