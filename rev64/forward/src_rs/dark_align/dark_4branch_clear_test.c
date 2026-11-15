/* dark_4branch_clear_test.c — 吃子清零 4 分支（EV_EAT/EV_BOMB/EV_MINE/EV_FLAG 各
 * 清零点 §686/737/748/795/841）全量矩阵行为级实证
 * 补全现有缺口：§64 dark_fullgame_ms_track_test / §66 / §67 实锤 EV_EAT 分支清零
 * （大吃小/吃光/扛旗清零），§46 dark_moves_seq_test 覆盖独立多步序列清零，但
 * **吃子清零 4 分支（EV_EAT/EV_BOMB/EV_MINE/EV_FLAG）全量矩阵**（各分支独立触发
 * + 清零断言，rules.c AUD-R-107 标注"4 分支各含一份清零"）无独立用例——EV_BOMB
 * （炸弹撞）/ EV_MINE（工兵吃雷 which=2 开）/ EV_FLAG（扛旗 which=3 开）各分支
 * 清零点独立断言未覆盖（§46 仅 EV_EAT 单分支，4 分支全量矩阵缺口）。
 *
 * 对照 64 位实现（rules.c AUD-R-107：st->moves_since_eat=0 在 4 分支（炸弹 :686 /
 * 旗 :737,:748 / 雷 :795 / 吃 :841）各含一份，统一 ate 标记留待下一轮；本轮实证
 * 4 分支各独立清零（行为级 0 分叉，各分支清零点均生效））：
 *   · EV_EAT（大吃小/同级互杀 :841）：蓝 rank1 强攻红 rank9 弱 → EV_EAT 清零；
 *   · EV_BOMB（炸弹撞 :686）：蓝 rank10（炸弹）攻红 rank1 → 双消失 EV_BOMB 清零；
 *   · EV_MINE（工兵吃雷 which=2 开 :795）：蓝工兵攻红雷 → EV_MINE 清零（mines+1）；
 *   · EV_FLAG（扛旗 which=3 开 :737/:748）：蓝 rank5 攻红军旗 → EV_FLAG 清零
 *     （扛旗终局 winner 置 + stage=OVER，清零在判和检查前 §519）。
 * 64 位行为级实证（走已实现的 4 分支清零路径 §686/737/748/795/841，0 分叉；几何
 * 走 §58/§61/§64 既有 LAY 明棋口径 + col4 纵线/col1 纵线既有几何）。
 *
 * 几何口径：EV_EAT/EV_BOMB 用 col4 纵线 44(蓝)攻 39(红)（§58/§64 既有走子几何）；
 *   EV_MINE 用 col1 纵线蓝工兵@51 攻雷@52（which=2 吃雷几何 §58/§61）；EV_FLAG
 *   用 col1 纵线蓝 rank5@51 攻 红军旗@56 大本营（§54/§61 扛旗几何）；红 cell50
 *   防困毙；mode=LAY 明棋（绕暗棋工兵门槛，EV_MINE/EV_FLAG 走 which=2/3 开路径）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_mine_set(int v);
extern void ui_rule_flag_set(int v);
extern void ui_rule_dark_set(int v);
#define C44 cell_index(8,4)    /* 蓝攻子（col4 纵线） */
#define C39 cell_index(7,4)    /* 红被攻（col4 纵线 39 相邻 44） */
#define C51 cell_index(10,1)   /* 蓝工兵/攻子（col1 纵线） */
#define C52 cell_index(10,2)   /* 红雷 */
#define C56 cell_index(11,1)   /* 红军旗大本营 */
#define C50 cell_index(10,0)   /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① EV_EAT 大吃小清零（蓝 rank1 攻红 rank9，§841 清零点） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C44].rank = 1; st.cells[C44].side = SIDE_BLUE; st.cells[C44].revealed = 1;
        st.cells[C39].rank = 9; st.cells[C39].side = SIDE_RED;  st.cells[C39].revealed = 1;
        st.cells[C50].rank = 2; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        st.moves_since_eat = 3;
        int ev = jq_try_move(&st, C44, C39);
        ok &= chk("① EV_EAT大吃小清零(蓝1攻红9 ev=EV_EAT ms3→0)",
                  ev == EV_EAT && st.moves_since_eat == 0);
    }
    /* ② EV_BOMB 炸弹撞清零（蓝 rank10 炸弹 攻红 rank1，双消失，§686 清零点） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C44].rank = RANK_BOMB; st.cells[C44].side = SIDE_BLUE; st.cells[C44].revealed = 1;
        st.cells[C39].rank = 1; st.cells[C39].side = SIDE_RED;  st.cells[C39].revealed = 1;
        st.cells[C50].rank = 2; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        st.moves_since_eat = 3;
        int ev = jq_try_move(&st, C44, C39);
        ok &= chk("② EV_BOMB炸弹撞清零(蓝炸弹攻红1双消失 ev=EV_BOMB ms3→0)",
                  ev == EV_BOMB && st.moves_since_eat == 0
                  && st.cells[C44].rank == RANK_EMPTY
                  && st.cells[C39].rank == RANK_EMPTY);
    }
    /* ③ EV_MINE 工兵吃雷清零（蓝工兵攻红雷 which=2 开，§795 清零点 + mines+1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C52].rank = RANK_MINE; st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(1); ui_rule_dark_set(1);
        st.moves_since_eat = 3;
        int ev = jq_try_move(&st, C51, C52);
        ok &= chk("③ EV_MINE工兵吃雷清零(蓝工兵攻雷 ev=EV_MINE ms3→0+mines1)",
                  ev == EV_MINE && st.moves_since_eat == 0
                  && st.mines_cleared[SIDE_BLUE] == 1);
    }
    /* ④ EV_FLAG 扛旗清零（蓝 rank5 攻红军旗 which=3 开，§737/:748 清零点 + 终局） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(1); ui_rule_dark_set(1);
        st.moves_since_eat = 3;
        int ev = jq_try_move(&st, C51, C56);
        ok &= chk("④ EV_FLAG扛旗清零(蓝5攻旗 ev=EV_FLAG ms3→0+OVER+BLUE)",
                  ev == EV_FLAG && st.moves_since_eat == 0
                  && st.stage == STG_OVER && st.winner == JQ_WIN_BLUE);
    }
    /* ⑤ 4 分支清零对照（各分支 ms 从 3 清零到 0，4/4 全清零 = AUD-R-107 4 分支
       各含一份清零全量实锤） */
    {
        /* 复用 ①-④ 实锤：4 分支各独立清零（本断言汇总确认 4 分支全清零） */
        int eat_clear = 1, bomb_clear = 1, mine_clear = 1, flag_clear = 1;
        jq_state s1; jq_state_init(&s1); s1.mode = JQ_MODE_LAY;
        memset(&s1.cells, 0, sizeof(s1.cells));
        s1.stage = STG_PLAY; s1.winner = JQ_WIN_NONE; s1.turn = SIDE_BLUE;
        s1.cells[C44].rank = 1; s1.cells[C44].side = SIDE_BLUE; s1.cells[C44].revealed = 1;
        s1.cells[C39].rank = 9; s1.cells[C39].side = SIDE_RED;  s1.cells[C39].revealed = 1;
        s1.cells[C50].rank = 2; s1.cells[C50].side = SIDE_RED;  s1.cells[C50].revealed = 1;
        s1.moves_since_eat = 3;
        eat_clear = (jq_try_move(&s1, C44, C39) == EV_EAT && s1.moves_since_eat == 0);

        jq_state s2; jq_state_init(&s2); s2.mode = JQ_MODE_LAY;
        memset(&s2.cells, 0, sizeof(s2.cells));
        s2.stage = STG_PLAY; s2.winner = JQ_WIN_NONE; s2.turn = SIDE_BLUE;
        s2.cells[C44].rank = RANK_BOMB; s2.cells[C44].side = SIDE_BLUE; s2.cells[C44].revealed = 1;
        s2.cells[C39].rank = 1; s2.cells[C39].side = SIDE_RED;  s2.cells[C39].revealed = 1;
        s2.cells[C50].rank = 2; s2.cells[C50].side = SIDE_RED;  s2.cells[C50].revealed = 1;
        s2.moves_since_eat = 3;
        bomb_clear = (jq_try_move(&s2, C44, C39) == EV_BOMB && s2.moves_since_eat == 0);

        jq_state s3; jq_state_init(&s3); s3.mode = JQ_MODE_LAY;
        memset(&s3.cells, 0, sizeof(s3.cells));
        s3.stage = STG_PLAY; s3.winner = JQ_WIN_NONE; s3.turn = SIDE_BLUE;
        s3.cells[C51].rank = RANK_WORKER; s3.cells[C51].side = SIDE_BLUE; s3.cells[C51].revealed = 1;
        s3.cells[C52].rank = RANK_MINE; s3.cells[C52].side = SIDE_RED;  s3.cells[C52].revealed = 1;
        s3.cells[C50].rank = 1; s3.cells[C50].side = SIDE_RED;  s3.cells[C50].revealed = 1;
        ui_rule_mine_set(1); ui_rule_flag_set(1); ui_rule_dark_set(1);
        s3.moves_since_eat = 3;
        mine_clear = (jq_try_move(&s3, C51, C52) == EV_MINE && s3.moves_since_eat == 0);

        jq_state s4; jq_state_init(&s4); s4.mode = JQ_MODE_LAY;
        memset(&s4.cells, 0, sizeof(s4.cells));
        s4.stage = STG_PLAY; s4.winner = JQ_WIN_NONE; s4.turn = SIDE_BLUE;
        s4.cells[C51].rank = 5; s4.cells[C51].side = SIDE_BLUE; s4.cells[C51].revealed = 1;
        s4.cells[C56].rank = RANK_FLAG; s4.cells[C56].side = SIDE_RED; s4.cells[C56].revealed = 1;
        s4.moves_since_eat = 3;
        flag_clear = (jq_try_move(&s4, C51, C56) == EV_FLAG && s4.moves_since_eat == 0);

        ok &= chk("⑤ 4分支清零对照(EAT/BOMB/MINE/FLAG 4/4全清零=AUD-R-107实锤)",
                  eat_clear && bomb_clear && mine_clear && flag_clear);
        ui_rule_mine_set(1); ui_rule_flag_set(1); ui_rule_dark_set(1);   /* 复原 */
    }
    ui_rule_mine_set(1); ui_rule_flag_set(1); ui_rule_dark_set(1);   /* 复原默认全开 */
    printf("dark_4branch_clear_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
