/* dark_rule_switch_test.c — 游戏设置 4 规则开关开/关分支行为级实证
 * 补全现有缺口：rule_text_conformance / draw_exist_test 仅覆盖开关开（默认 1）
 * 路径，4 开关关闭分支（§P1 datah_012 证据）无独立用例。
 *
 * 对照 64 位实现（rules.c 开关读点 + 桌面桩 ui_state_stub.c ui_rule_*_set）：
 *   · 和棋开关（which=0，g_rule_draw）：§519 `if (ui_rule_cfg_get(0) &&
 *     moves_since_eat>=70||total_moves>=1000) jq_on_draw`——关→不判和；
 *   · 吃地雷开关（which=2，g_rule_mine）：§746 工兵撞雷——关=工兵撞雷必死
 *     （EV_DEAD 攻方消失，雷不消失，送死）；
 *   · 吃军旗开关（which=3，g_rule_flag）：§709 `if (ui_rule_cfg_get(3))
 *     jq_on_flag_capture`——关→扛旗走子正常收口不置终局（事件码仍 EV_FLAG，
 *     stage 不 OVER，旗被攻子覆盖但不判胜）；
 *   · 暗棋开关（which=4，g_rule_dark）：§292/310 `st->mode==FLIP &&
 *     ui_rule_cfg_get(4)`——关→暗棋 FLIP 退回明棋语义（非工兵/未挖雷也可扛旗）。
 * 64 位行为级实证（走已实现的 4 开关分支路径，0 分叉）。
 *
 * 夹具口径：和棋/扛旗场景放"双方有子"（红 rank1@50）防困毙路径掩盖开关效果
 * （单侧无子会触发 jq_on_stalemate 困毙判胜，掩盖和棋/扛旗开关判定）。
 * 几何：col1 纵线 46(蓝工兵)→51(红雷) 吃雷 / 44(蓝rank1)→39 空移 /
 *   51(蓝rank5)→56(红军旗大本营) 扛旗。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern void ui_rule_draw_set(int v);
extern void ui_rule_mine_set(int v);
extern void ui_rule_flag_set(int v);
extern void ui_rule_dark_set(int v);

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 和棋开关开 vs 关：69→70 步无吃子，开=DRAW / 关=PLAY */
    {
        jq_state a, b;
        memset(&a, 0, sizeof(a)); jq_state_init(&a); a.mode = JQ_MODE_LAY;
        memset(&a.cells, 0, sizeof(a.cells));
        a.stage = STG_PLAY; a.winner = JQ_WIN_NONE; a.turn = SIDE_BLUE;
        a.cells[44].rank = 1; a.cells[44].side = SIDE_BLUE; a.cells[44].revealed = 1;
        a.cells[50].rank = 1; a.cells[50].side = SIDE_RED;  a.cells[50].revealed = 1;
        a.moves_since_eat = 69;
        ui_rule_draw_set(1);
        int eva = jq_try_move(&a, 44, 39);
        int open_draw = (a.stage == STG_OVER && a.winner == JQ_WIN_DRAW);
        /* 同夹具开关关 */
        memset(&b, 0, sizeof(b)); jq_state_init(&b); b.mode = JQ_MODE_LAY;
        memset(&b.cells, 0, sizeof(b.cells));
        b.stage = STG_PLAY; b.winner = JQ_WIN_NONE; b.turn = SIDE_BLUE;
        b.cells[44].rank = 1; b.cells[44].side = SIDE_BLUE; b.cells[44].revealed = 1;
        b.cells[50].rank = 1; b.cells[50].side = SIDE_RED;  b.cells[50].revealed = 1;
        b.moves_since_eat = 69;
        ui_rule_draw_set(0);
        int evb = jq_try_move(&b, 44, 39);
        int closed_nodraw = (b.stage == STG_PLAY && b.winner == JQ_WIN_NONE);
        ui_rule_draw_set(1);   /* 复原 */
        ok &= chk("① 和棋开关开=70步判和 / 关=不判和(PLAY)",
                  open_draw && closed_nodraw);
        (void)eva; (void)evb;
    }
    /* ② 吃地雷开关开 vs 关：工兵撞雷，开=EV_MINE工兵进雷格 / 关=EV_DEAD工兵送死雷留 */
    {
        /* 开：工兵挖雷 */
        jq_state a; memset(&a, 0, sizeof(a)); jq_state_init(&a); a.mode = JQ_MODE_FLIP;
        memset(&a.cells, 0, sizeof(a.cells));
        a.stage = STG_PLAY; a.winner = JQ_WIN_NONE; a.turn = SIDE_BLUE;
        a.cells[46].rank = RANK_WORKER; a.cells[46].side = SIDE_BLUE; a.cells[46].revealed = 1;
        a.cells[51].rank = RANK_MINE;  a.cells[51].side = SIDE_RED;  a.cells[51].revealed = 1;
        ui_rule_mine_set(1);
        int eva = jq_try_move(&a, 46, 51);
        int open_mine = (eva == EV_MINE && a.cells[46].rank == RANK_EMPTY
                         && a.cells[51].rank == RANK_WORKER && a.mines_cleared[SIDE_BLUE] == 1);
        /* 关：工兵撞雷送死（雷不消失，工兵消失） */
        jq_state b; memset(&b, 0, sizeof(b)); jq_state_init(&b); b.mode = JQ_MODE_FLIP;
        memset(&b.cells, 0, sizeof(b.cells));
        b.stage = STG_PLAY; b.winner = JQ_WIN_NONE; b.turn = SIDE_BLUE;
        b.cells[46].rank = RANK_WORKER; b.cells[46].side = SIDE_BLUE; b.cells[46].revealed = 1;
        b.cells[51].rank = RANK_MINE;  b.cells[51].side = SIDE_RED;  b.cells[51].revealed = 1;
        b.cells[50].rank = 1; b.cells[50].side = SIDE_RED; b.cells[50].revealed = 1; /* 红有子防困毙 */
        ui_rule_mine_set(0);
        int evb = jq_try_move(&b, 46, 51);
        int closed_dead = (evb == EV_DEAD && b.cells[46].rank == RANK_EMPTY
                           && b.cells[51].rank == RANK_MINE);   /* 雷留 */
        ui_rule_mine_set(1);   /* 复原 */
        ok &= chk("② 吃雷开关开=工兵挖雷EV_MINE / 关=工兵送死EV_DEAD雷留",
                  open_mine && closed_dead);
    }
    /* ③ 吃军旗开关开 vs 关：扛旗，开=EV_FLAG终局胜 / 关=EV_FLAG走子不终局 */
    {
        jq_state a; memset(&a, 0, sizeof(a)); jq_state_init(&a); a.mode = JQ_MODE_LAY;
        memset(&a.cells, 0, sizeof(a.cells));
        a.stage = STG_PLAY; a.winner = JQ_WIN_NONE; a.turn = SIDE_BLUE;
        a.cells[51].rank = 5; a.cells[51].side = SIDE_BLUE; a.cells[51].revealed = 1;
        a.cells[56].rank = RANK_FLAG; a.cells[56].side = SIDE_RED; a.cells[56].revealed = 1;
        a.cells[50].rank = 1; a.cells[50].side = SIDE_RED; a.cells[50].revealed = 1;
        ui_rule_flag_set(1);
        int eva = jq_try_move(&a, 51, 56);
        int open_flag = (eva == EV_FLAG && a.stage == STG_OVER && a.winner == JQ_WIN_BLUE);
        jq_state b; memset(&b, 0, sizeof(b)); jq_state_init(&b); b.mode = JQ_MODE_LAY;
        memset(&b.cells, 0, sizeof(b.cells));
        b.stage = STG_PLAY; b.winner = JQ_WIN_NONE; b.turn = SIDE_BLUE;
        b.cells[51].rank = 5; b.cells[51].side = SIDE_BLUE; b.cells[51].revealed = 1;
        b.cells[56].rank = RANK_FLAG; b.cells[56].side = SIDE_RED; b.cells[56].revealed = 1;
        b.cells[50].rank = 1; b.cells[50].side = SIDE_RED; b.cells[50].revealed = 1;
        ui_rule_flag_set(0);
        int evb = jq_try_move(&b, 51, 56);
        int closed_noflag = (evb == EV_FLAG && b.stage == STG_PLAY
                             && b.winner == JQ_WIN_NONE);   /* 不终局 */
        ui_rule_flag_set(1);   /* 复原 */
        ok &= chk("③ 扛旗开关开=EV_FLAG终局胜 / 关=EV_FLAG走子不终局",
                  open_flag && closed_noflag);
    }
    /* ④ 暗棋开关开 vs 关：非工兵未挖雷扛旗，开=拒(暗棋门槛) / 关=放行(明棋语义) */
    {
        jq_state a; memset(&a, 0, sizeof(a)); jq_state_init(&a); a.mode = JQ_MODE_FLIP;
        memset(&a.cells, 0, sizeof(a.cells));
        a.stage = STG_PLAY; a.winner = JQ_WIN_NONE; a.turn = SIDE_BLUE;
        a.cells[51].rank = 5; a.cells[51].side = SIDE_BLUE; a.cells[51].revealed = 1;
        a.cells[56].rank = RANK_FLAG; a.cells[56].side = SIDE_RED; a.cells[56].revealed = 1;
        a.mines_cleared[SIDE_BLUE] = 0;   /* 未挖雷 */
        ui_rule_dark_set(1);
        int can_open = jq_can_move(&a, 51, 56);   /* 暗棋开+未挖雷+非工兵=拒 */
        jq_state b; memset(&b, 0, sizeof(b)); jq_state_init(&b); b.mode = JQ_MODE_FLIP;
        memset(&b.cells, 0, sizeof(b.cells));
        b.stage = STG_PLAY; b.winner = JQ_WIN_NONE; b.turn = SIDE_BLUE;
        b.cells[51].rank = 5; b.cells[51].side = SIDE_BLUE; b.cells[51].revealed = 1;
        b.cells[56].rank = RANK_FLAG; b.cells[56].side = SIDE_RED; b.cells[56].revealed = 1;
        b.mines_cleared[SIDE_BLUE] = 0;
        ui_rule_dark_set(0);
        int can_closed = jq_can_move(&b, 51, 56);   /* 暗棋关=明棋语义=放行 */
        ui_rule_dark_set(1);   /* 复原 */
        ok &= chk("④ 暗棋开关开=非工兵未挖雷扛旗拒 / 关=明棋语义放行",
                  can_open == 0 && can_closed == 1);
    }
    printf("dark_rule_switch_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
