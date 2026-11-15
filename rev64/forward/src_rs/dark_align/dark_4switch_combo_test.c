/* dark_4switch_combo_test.c — 4 规则开关组合交互行为级实证（which=2 吃雷 × which=4 暗棋）
 * 补全现有缺口：§37 dark_rule_switch_test 覆盖各开关独立开/关，§50
 * dark_cross_switch_test 覆盖 which=1×which=4 交叉（源格/目标格分域），但
 * **which=2（吃雷）× which=4（暗棋）的事实交互**（which=2 关→工兵撞雷送死
 * →mines_cleared 不可累积到 3→暗棋扛旗门槛事实上不可达）无独立用例。
 *
 * 对照 64 位实现（rules.c §746 which=2 工兵撞雷开=挖雷/关=送死 + §318-328 暗棋
 * 扛旗门槛 mines_cleared>=3 + §301 which=4 FLIP 门控）：
 *   · which=2 开：工兵撞雷=EV_MINE（雷消失，mines_cleared[BLUE]++）；
 *   · which=2 关：工兵撞雷=EV_DEAD 送死（雷不消失，mines_cleared 不增）；
 *   · 暗棋扛旗门槛（§318-328）读 `mines_cleared[me]>=3`——**门槛判定只看
 *     mines 值，不读 which=2 开关**；但 which=2 关使 mines 无法累积到 3，
 *     暗棋扛旗路径**事实上不可达**（开关间事实交互，非门槛判定直接依赖 which=2）；
 *   · 门槛判定对照：mines_cleared=3 时（无论 which=2 开关）放行 / mines_cleared=0
 *     时拒——门槛读值非读开关。
 * 64 位行为级实证（走已实现的 §746 which=2 撞雷 + §318-328 扛旗门槛路径，0 分叉）。
 *
 * 几何口径：col1 纵线工兵 cell46(9,1) 攻 雷 cell51(10,1)（§32 既有挖雷几何）；
 *   扛旗目标=红军旗 cell56(11,1) 大本营（§32/§44/§49 既有几何）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_mine_set(int v);
extern void ui_rule_dark_set(int v);
#define C46 cell_index(9,1)    /* 工兵 cell46 */
#define C51 cell_index(10,1)   /* 雷 cell51 / 工兵扛旗源 */
#define C56 cell_index(11,1)   /* 红军旗 cell56 大本营 */
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
    /* ① which=2 开：工兵撞雷=EV_MINE（雷消失，mines_cleared[BLUE]++） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C46].rank = RANK_WORKER; st.cells[C46].side = SIDE_BLUE; st.cells[C46].revealed = 1;
        st.cells[C51].rank = RANK_MINE;  st.cells[C51].side = SIDE_RED;  st.cells[C51].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED; st.cells[C50].revealed = 1;
        ui_rule_mine_set(1);
        int ev = jq_try_move(&st, C46, C51);
        ok &= chk("① which=2开: 工兵46→51雷=EV_MINE+mines_cleared=1",
                  ev == EV_MINE && st.mines_cleared[SIDE_BLUE] == 1
                  && st.cells[C51].rank == RANK_WORKER, 1);
    }
    /* ② which=2 关：工兵撞雷=EV_DEAD 送死（雷不消失，mines_cleared 不增） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C46].rank = RANK_WORKER; st.cells[C46].side = SIDE_BLUE; st.cells[C46].revealed = 1;
        st.cells[C51].rank = RANK_MINE;  st.cells[C51].side = SIDE_RED;  st.cells[C51].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED; st.cells[C50].revealed = 1;
        ui_rule_mine_set(0);
        int ev = jq_try_move(&st, C46, C51);
        ok &= chk("② which=2关: 工兵46→51雷=EV_DEAD送死+雷留+mines_cleared=0",
                  ev == EV_DEAD && st.cells[C51].rank == RANK_MINE
                  && st.mines_cleared[SIDE_BLUE] == 0, 1);
        ui_rule_mine_set(1);   /* 复原 */
    }
    /* ③ 扛旗门槛读 mines 值非读 which=2：mines=3（which=2 关）→ 放行（门槛看值） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG;   st.cells[C56].side = SIDE_RED;  st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED; st.cells[C50].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 3;   /* 假设已挖满（门槛看值） */
        ui_rule_mine_set(0); ui_rule_dark_set(1);
        int can = jq_can_move(&st, C51, C56);
        ok &= chk("③ 门槛读mines值: mines=3(which=2关)扛红军旗=1(放行,门槛看值非开关)",
                  can, 1);
        ui_rule_mine_set(1);   /* 复原 */
    }
    /* ④ 门槛拒（mines=0）：which=2 关使 mines 不可达 3，扛旗事实上不可达 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C56].rank = RANK_FLAG;   st.cells[C56].side = SIDE_RED;  st.cells[C56].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED; st.cells[C50].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 0;   /* 未挖雷（which=2 关实际不可达 3） */
        ui_rule_mine_set(0); ui_rule_dark_set(1);
        int can = jq_can_move(&st, C51, C56);
        ok &= chk("④ 门槛拒: mines=0(which=2关不可达3)扛红军旗=0(事实上不可达)",
                  can, 0);
        ui_rule_mine_set(1); ui_rule_dark_set(1);
    }
    printf("dark_4switch_combo_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
