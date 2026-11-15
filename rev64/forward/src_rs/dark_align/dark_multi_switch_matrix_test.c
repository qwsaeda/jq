/* dark_multi_switch_matrix_test.c — 暗棋多规则开关组合（which=2 吃雷 × which=3 扛旗
 * 开/关矩阵）的终局行为级实证
 * 补全现有缺口：§51 dark_4switch_combo_test 覆盖 which=2×which=4 两开关键交互
 * （吃雷清零阻断扛旗门槛），§54 dark_flagoff_stalemate_test 覆盖 which=3 单开关
 * （扛旗终局 vs 困毙近似），§55 dark_mineoff_stalemate_test 覆盖 which=2 单开关
 * （吃雷清零 vs 送死），但**多开关组合（which=2 关 + which=3 关 同时）的终局交互
 * 矩阵**无独立用例（双关时"吃雷不清零 + 扛旗不置终局"叠加，靠困毙近似判胜，
 * 非任一端开关单控）。
 *
 * 对照 64 位实现（rules.c §746 which=2 吃雷清零门控 + §700 which=3 扛旗终局门控，
 * 两开关**分域独立**——which=2 控"工兵吃雷是否清零 moves_since_eat/mines"，
 * which=3 控"扛旗是否置终局"，组合下各自行为叠加）：
 *   · which=2 开 + which=3 开（默认全开）：工兵吃雷清零（EV_MINE）+ 扛旗终局
 *     （EV_FLAG+OVER）；
 *   · which=2 关 + which=3 开：工兵撞雷送死（EV_DEAD，不清零）+ 扛旗终局；
 *   · which=2 开 + which=3 关：工兵吃雷清零 + 扛旗不置终局（攻子入位，靠困毙
 *     近似）；
 *   · which=2 关 + which=3 关（双关）：工兵撞雷送死（不清零）+ 扛旗不置终局
 *     （攻子入位，靠困毙近似）——双关下**两行为叠加**（雷不清零 + 旗不终局），
 *     终局判定统一退回困毙/吃光近似（非任一端开关单控）。
 * 64 位行为级实证（走已实现的 §746/§700 which=2/3 门控 + EV_MINE/EV_DEAD/
 * EV_FLAG 路径，0 分叉；几何走 §54 既有 LAY 明棋口径，绕开暗棋工兵门槛）。
 *
 * 几何口径：col1 纵线蓝攻子 cell51(10,1) 攻 红军旗 cell56(11,1) 大本营（§32/§44/
 *   §49/§54 既有扛旗几何）+ cell51 攻雷 cell52(10,2)（which=2 吃雷几何）；红
 *   cell50(10,0) 防困毙；mode=LAY（明棋，§54 既有口径，绕暗棋工兵扛旗门槛）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_mine_set(int v);
extern void ui_rule_flag_set(int v);
#define C51 cell_index(10,1)   /* 蓝攻子 */
#define C52 cell_index(10,2)   /* 红雷 */
#define C56 cell_index(11,1)   /* 红军旗大本营 */
#define C50 cell_index(10,0)   /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

/* 工兵吃雷夹具（C51 工兵 攻 C52 雷），返 ev + mines 增量 */
static int setup_worker_mine(int mine_on, int flag_on, jq_state *out)
{
    jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
    memset(&st.cells, 0, sizeof(st.cells));
    st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
    st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
    st.cells[C52].rank = RANK_MINE; st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;
    st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
    ui_rule_mine_set(mine_on); ui_rule_flag_set(flag_on);
    int ev = jq_try_move(&st, C51, C52);
    *out = st;   /* 走子后状态（mines/stage 已更新） */
    return ev;
}

/* 扛旗夹具（C51 rank5 攻 C56 旗），返终局状态 */
static int setup_flag(int mine_on, int flag_on, jq_state *out, int red_guard)
{
    jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
    memset(&st.cells, 0, sizeof(st.cells));
    st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
    st.cells[C51].rank = 5; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
    st.cells[C56].rank = RANK_FLAG; st.cells[C56].side = SIDE_RED; st.cells[C56].revealed = 1;
    if (red_guard) {
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED; st.cells[C50].revealed = 1;
    }
    ui_rule_mine_set(mine_on); ui_rule_flag_set(flag_on);
    int ev = jq_try_move(&st, C51, C56);
    *out = st;   /* 走子后状态（stage/winner/cells 已更新） */
    return ev;
}

int main(void)
{
    int ok = 1;
    /* ① 全开（which=2 开 + which=3 开）：工兵吃雷清零（EV_MINE）+ 扛旗终局（OVER） */
    {
        jq_state st;
        int ev = setup_worker_mine(1, 1, &st);
        ok &= chk("① 全开 which=2开: 工兵吃雷 EV_MINE(清零)",
                  ev == EV_MINE && st.mines_cleared[SIDE_BLUE] == 1);
        jq_state stf;
        setup_flag(1, 1, &stf, 1);
        ok &= chk("① 全开 which=3开: 扛旗终局(OVER)",
                  stf.stage == STG_OVER && stf.winner != JQ_WIN_NONE);
    }
    /* ② which=2 关 + which=3 开：工兵撞雷送死（EV_DEAD 不清零）+ 扛旗终局 */
    {
        jq_state st;
        int ev = setup_worker_mine(0, 1, &st);
        ok &= chk("② which=2关: 工兵撞雷送死 EV_DEAD(不清零)",
                  ev == EV_DEAD && st.mines_cleared[SIDE_BLUE] == 0);
        jq_state stf;
        setup_flag(0, 1, &stf, 1);
        ok &= chk("② which=3开: 扛旗仍终局(OVER, 不受 which=2 关影响)",
                  stf.stage == STG_OVER);
    }
    /* ③ which=2 开 + which=3 关：工兵吃雷清零 + 扛旗不置终局（困毙近似） */
    {
        jq_state st;
        int ev = setup_worker_mine(1, 0, &st);
        ok &= chk("③ which=2开: 工兵吃雷仍 EV_MINE(清零, 不受 which=3 关影响)",
                  ev == EV_MINE && st.mines_cleared[SIDE_BLUE] == 1);
        jq_state stf;
        setup_flag(1, 0, &stf, 1);   /* 红有 guard@50 → 扛旗后不困毙 */
        ok &= chk("③ which=3关: 扛旗不置终局(PLAY+攻子入位, 困毙近似)",
                  stf.stage == STG_PLAY && stf.cells[C56].rank == 5);
    }
    /* ④ 双关（which=2 关 + which=3 关）：工兵撞雷送死 + 扛旗不置终局（双行为叠加） */
    {
        jq_state st;
        int ev = setup_worker_mine(0, 0, &st);
        ok &= chk("④ 双关 which=2关: 工兵撞雷送死 EV_DEAD(不清零)",
                  ev == EV_DEAD && st.mines_cleared[SIDE_BLUE] == 0);
        jq_state stf;
        setup_flag(0, 0, &stf, 1);   /* 红有 guard@50 → 双关下扛旗后不困毙 */
        ok &= chk("④ 双关 which=3关: 扛旗不置终局(PLAY+攻子入位, 双行为叠加)",
                  stf.stage == STG_PLAY && stf.cells[C56].rank == 5);
    }
    ui_rule_mine_set(1); ui_rule_flag_set(1);   /* 复原默认全开 */
    printf("dark_multi_switch_matrix_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
