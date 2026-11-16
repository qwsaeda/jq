/* dark_rail_r5r6_worker_eat_test.c — 工兵 row5/row6 全宽横贯线"同段拐弯 + 吃
 * 子/扛旗终局交互"全场景对照（对称 §86/§87 横贯线中间 col2 交叉格吃子/吃雷/
 * 扛旗终局交互扩展到 row5/row6 全宽横贯线全端点 + 工兵拐弯场景对照）行为级实
 * 证
 * 补全现有缺口：§86/§87 dark_rail_cross_eat_test + dark_rail_cross_flagmine_
 * test（实锤横贯线中间 col2 交叉格"滑行到敌方子格吃子/吃雷/扛旗终局交互"全
 * 场景对照 + 中间格 vs 目标格语义分界 + 工兵/非工兵路径分派差异），§95/§96/
 * §97/§98 实锤工兵 row5/row6 全宽横贯线 col0-4 全 5 端点互为同段拐弯端点全对
 * 称闭环，但**工兵 row5/row6 全宽横贯线"同段拐弯 + 目标格有敌方子/军旗的吃子
 * /扛旗终局交互"全场景对照**无独立用例——§86/§87 实锤横贯线中间 col2 交叉格
 * "滑行到目标格有敌方子/军旗吃子/扛旗终局交互"（工兵 BFS 拒非空格雷/旗格 /
 * 非工兵 straight 放行扛旗 / 工兵吃雷仅一步相邻或暗棋），但**工兵 row5/row6
 * 全宽横贯线全端点（col0/col1/col2/col3/col4）同段拐弯到目标格有敌方子/军旗
 * 的吃子/扛旗终局交互全场景对照（工兵 vs 非工兵路径分派差异扩展到全宽横贯线
 * 全端点）**未独立断言。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83/§84/§85/§87/§88/§8
 * 9/§90/§91/§92/§93/§94/§95/§96/§97/§98 全格分布实锤：col2 纵贯 10 格被 row3/
 * 8 隔断成 3 段 + row5/6 横贯 10 格全宽 + row0/11 横贯非全宽仅 3 格铁路 col0
 * /2/4 + col0=4 格 / col1=2 格 / col3=2 格 / col4=4 格非纵贯全高 + row3/row8
 *  全非铁路行营隔断）+ rules.c rail_path_engineer 工兵 BFS 拐弯路径引擎（工
 * 兵 row5/row6 全宽横贯线全端点同段拐弯到目标格有敌方子/军旗 `walkable` 拒非
 * 空格 can=0，小撞大送死拒 / BFS 拒非空格雷/旗格）+ rail_path_straight 非工
 * 兵直走（非工兵 row5/row6 全宽横贯线同段滑行到旗格扛旗放行 EV_FLAG）+ 吃子
 * 比较 §796-798 rank 小=强 / §817 同级互杀 + 终局判定 :700 EV_FLAG / :754-79
 * 5 EV_MINE）：
 *   · 工兵 row5 全宽横贯线全端点同段拐弯到目标格有敌方强子（rank3）`can=0`（
 *     工兵 rank9 弱撞 rank3 强送死，jq_can_move 规则层拒——小撞大送死拒非放
 *     行）；
 *   · 工兵 row5/row6 全宽横贯线全端点同段拐弯到目标格有军旗 `can=0`（明棋工
 *     兵横贯滑行到旗格 BFS `walkable` 拒非空格，工兵扛旗终局仅一步相邻或暗棋
 *     场景——§87 实锤口径）；
 *   · 对照非工兵 row5 全宽横贯线同段滑行到旗格扛旗放行（非工兵 rank1 (5,0)→
 *     (5,2) `can=1 EV_FLAG winner=BLUE`——非工兵 `rail_path_straight` 直走放
 *     行扛旗终局）；
 *   · 工兵 vs 非工兵终局交互路径分派差异（工兵 BFS 拐弯路径引擎拒非空格雷/旗
 *     格 vs 非工兵 straight 直走放行扛旗，row5/row6 全宽横贯线全端点扩展到全
 *     场景对照）。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + rail_path_engineer 工兵 BFS
 *  + rail_path_straight 非工兵直走 + 吃子比较 §796-798 + 终局判定 :700/:754-
 * 795，0 分叉；几何走 §86/§87 既有横贯线中间 col2 交叉格吃子/扛旗终局交互口
 * 径 + §95/§96/§97/§98 既有工兵 row5/row6 全宽横贯线全端点同段拐弯口径 + 本
 * 轮补工兵同段拐弯 + 目标格敌方子/军旗吃子/扛旗终局交互全场景对照）。
 *
 * 几何口径：row5/row6 全宽横贯线全端点（col0/col1/col2/col3/col4 全 10 格
 *   is_rail=1）；工兵 row5/row6 全端点同段拐弯到目标格有敌方强子（rank3）/军
 *   旗（col2 交叉格 / col0 端 / col4 端）；对照非工兵 row5 同段滑行到旗格扛
 *   旗放行 EV_FLAG；蓝/红 cell(0,0)/(0,4) 防困毙；mode=LAY 明棋；工兵 RANK_WO
 *   RKER + 非工兵 rank1。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C50 cell_index(5,0)    /* row5 col0（全宽横贯线 col0 端） */
#define C52 cell_index(5,2)    /* row5 col2（全宽横贯线 col2 端，纵贯全高） */
#define C54 cell_index(5,4)    /* row5 col4（全宽横贯线 col4 端） */
#define C60 cell_index(6,0)    /* row6 col0（全宽横贯线 col0 端） */
#define C62 cell_index(6,2)    /* row6 col2（全宽横贯线 col2 端，纵贯全高） */
#define C64 cell_index(6,4)    /* row6 col4（全宽横贯线 col4 端） */
#define C00 cell_index(0,0)    /* 蓝防困毙 */
#define C04 cell_index(0,4)    /* 蓝防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 工兵 row5 全宽横贯线 col0 端同段拐弯到 col2 交叉格目标格有敌方强子
       （rank3）`can=0`（工兵 rank9 弱撞 rank3 强送死，jq_can_move 规则层拒） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50].rank = RANK_WORKER; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1;
        st.cells[C52].rank = 3; st.cells[C52].side = SIDE_RED; st.cells[C52].revealed = 1;
        st.cells[C00].rank = 1; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C50, C52);
        int ev = jq_try_move(&st, C50, C52);
        ok &= chk("① 工兵row5全宽横贯线col0端同段拐弯到col2交叉格目标格有敌方强子rank3(工兵(5,0)→(5,2)目标格红rank3小撞大送死can=0,工兵rank9弱撞rank3强jq_can_move规则层拒)",
                  can == 0 && ev == -1
                  && st.cells[C50].rank == RANK_WORKER  /* 工兵未动 */
                  && st.cells[C52].rank == 3);         /* 目标格红rank3留 */
    }
    /* ② 工兵 row5/row6 全宽横贯线 col0 端同段拐弯到 col2 交叉格目标格有军旗
       `can=0`（明棋工兵横贯滑行到旗格 BFS `walkable` 拒非空格，工兵扛旗终局仅
       一步相邻或暗棋场景——§87 实锤口径） */
    {
        /* row5 工兵 (5,0)→col2 (5,2) 目标格红军旗 */
        jq_state st1; st1.mode = JQ_MODE_LAY;
        jq_state_init(&st1); memset(&st1.cells, 0, sizeof(st1.cells));
        st1.stage = STG_PLAY; st1.winner = JQ_WIN_NONE; st1.turn = SIDE_BLUE;
        st1.cells[C50].rank = RANK_WORKER; st1.cells[C50].side = SIDE_BLUE; st1.cells[C50].revealed = 1;
        st1.cells[C52].rank = RANK_FLAG; st1.cells[C52].side = SIDE_RED; st1.cells[C52].revealed = 1;
        st1.cells[C00].rank = 1; st1.cells[C00].side = SIDE_BLUE; st1.cells[C00].revealed = 1;
        int r5_flag = (jq_can_move(&st1, C50, C52) == 0);
        /* row6 工兵 (6,0)→col2 (6,2) 目标格红军旗 */
        jq_state st2; st2.mode = JQ_MODE_LAY;
        jq_state_init(&st2); memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[C60].rank = RANK_WORKER; st2.cells[C60].side = SIDE_BLUE; st2.cells[C60].revealed = 1;
        st2.cells[C62].rank = RANK_FLAG; st2.cells[C62].side = SIDE_RED; st2.cells[C62].revealed = 1;
        st2.cells[C00].rank = 1; st2.cells[C00].side = SIDE_BLUE; st2.cells[C00].revealed = 1;
        int r6_flag = (jq_can_move(&st2, C60, C62) == 0);
        ok &= chk("② 工兵row5/row6全宽横贯线col0端同段拐弯到col2交叉格目标格有军旗can=0(row5工兵(5,0)→(5,2)红军旗/row6工兵(6,0)→(6,2)红军旗均can=0,明棋工兵横贯滑行到旗格BFS walkable拒非空格工兵扛旗终局仅一步相邻或暗棋§87实锤口径)",
                  r5_flag && r6_flag);
    }
    /* ③ 对照非工兵 row5 全宽横贯线同段滑行到旗格扛旗放行（非工兵 rank1 (5,0)
       →(5,2) `can=1 EV_FLAG winner=BLUE`——非工兵 `rail_path_straight` 直走放
       行扛旗终局，对照 ② 工兵拒） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1; /* 蓝rank1强 */
        st.cells[C52].rank = RANK_FLAG; st.cells[C52].side = SIDE_RED; st.cells[C52].revealed = 1;
        st.cells[C00].rank = 2; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C50, C52);
        int ev = jq_try_move(&st, C50, C52);
        ok &= chk("③ 对照非工兵row5全宽横贯线同段滑行到旗格扛旗放行(非工兵rank1(5,0)→(5,2)红军旗can=1 EV_FLAG winner=BLUE,rail_path_straight非工兵直走放行扛旗终局,对照②工兵拒)",
                  can == 1 && ev == EV_FLAG && st.winner == JQ_WIN_BLUE
                  && st.cells[C50].rank == RANK_EMPTY   /* 攻方源格清空 */
                  && st.cells[C52].rank == 1);          /* 攻子落位旗格 */
    }
    /* ④ 工兵 row5 全宽横贯线 col2 端同段拐弯到 col0/col4 端目标格有敌方强子
       （rank3）`can=0`（工兵 rank9 弱撞 rank3 强送死，jq_can_move 规则层拒） */
    {
        /* 工兵 (5,2)→col0 端 (5,0) 目标格红 rank3 */
        jq_state st1; st1.mode = JQ_MODE_LAY;
        jq_state_init(&st1); memset(&st1.cells, 0, sizeof(st1.cells));
        st1.stage = STG_PLAY; st1.winner = JQ_WIN_NONE; st1.turn = SIDE_BLUE;
        st1.cells[C52].rank = RANK_WORKER; st1.cells[C52].side = SIDE_BLUE; st1.cells[C52].revealed = 1;
        st1.cells[C50].rank = 3; st1.cells[C50].side = SIDE_RED; st1.cells[C50].revealed = 1;
        st1.cells[C04].rank = 1; st1.cells[C04].side = SIDE_BLUE; st1.cells[C04].revealed = 1;
        int r5_c0 = (jq_can_move(&st1, C52, C50) == 0);
        /* 工兵 (5,2)→col4 端 (5,4) 目标格红 rank3 */
        jq_state st2; st2.mode = JQ_MODE_LAY;
        jq_state_init(&st2); memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[C52].rank = RANK_WORKER; st2.cells[C52].side = SIDE_BLUE; st2.cells[C52].revealed = 1;
        st2.cells[C54].rank = 3; st2.cells[C54].side = SIDE_RED; st2.cells[C54].revealed = 1;
        st2.cells[C00].rank = 1; st2.cells[C00].side = SIDE_BLUE; st2.cells[C00].revealed = 1;
        int r5_c4 = (jq_can_move(&st2, C52, C54) == 0);
        ok &= chk("④ 工兵row5全宽横贯线col2端同段拐弯到col0/col4端目标格有敌方强子rank3(工兵(5,2)→col0(5,0)/col4(5,4)目标格红rank3小撞大送死can=0,工兵rank9弱撞rank3强jq_can_move规则层拒)",
                  r5_c0 && r5_c4);
    }
    /* ⑤ 工兵 row6 全宽横贯线全端点同段拐弯到目标格有敌方强子/军旗 `can=0`（
       工兵 rank9 弱撞 rank3 强送死拒 / BFS 拒非空格军旗，row6 全宽横贯线全端
       点扩展到全场景对照） */
    {
        /* 工兵 row6 col0 端 (6,0)→col2 (6,2) 目标格红 rank3 */
        jq_state st1; st1.mode = JQ_MODE_LAY;
        jq_state_init(&st1); memset(&st1.cells, 0, sizeof(st1.cells));
        st1.stage = STG_PLAY; st1.winner = JQ_WIN_NONE; st1.turn = SIDE_BLUE;
        st1.cells[C60].rank = RANK_WORKER; st1.cells[C60].side = SIDE_BLUE; st1.cells[C60].revealed = 1;
        st1.cells[C62].rank = 3; st1.cells[C62].side = SIDE_RED; st1.cells[C62].revealed = 1;
        st1.cells[C00].rank = 1; st1.cells[C00].side = SIDE_BLUE; st1.cells[C00].revealed = 1;
        int r6_eat = (jq_can_move(&st1, C60, C62) == 0);
        /* 工兵 row6 col4 端 (6,4)→col2 (6,2) 目标格红 rank3 */
        jq_state st2; st2.mode = JQ_MODE_LAY;
        jq_state_init(&st2); memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[C64].rank = RANK_WORKER; st2.cells[C64].side = SIDE_BLUE; st2.cells[C64].revealed = 1;
        st2.cells[C62].rank = 3; st2.cells[C62].side = SIDE_RED; st2.cells[C62].revealed = 1;
        st2.cells[C00].rank = 1; st2.cells[C00].side = SIDE_BLUE; st2.cells[C00].revealed = 1;
        int r6_eat4 = (jq_can_move(&st2, C64, C62) == 0);
        /* 工兵 row6 col2 端 (6,2)→col0 端 (6,0) 目标格红军旗 */
        jq_state st3; st3.mode = JQ_MODE_LAY;
        jq_state_init(&st3); memset(&st3.cells, 0, sizeof(st3.cells));
        st3.stage = STG_PLAY; st3.winner = JQ_WIN_NONE; st3.turn = SIDE_BLUE;
        st3.cells[C62].rank = RANK_WORKER; st3.cells[C62].side = SIDE_BLUE; st3.cells[C62].revealed = 1;
        st3.cells[C60].rank = RANK_FLAG; st3.cells[C60].side = SIDE_RED; st3.cells[C60].revealed = 1;
        st3.cells[C04].rank = 1; st3.cells[C04].side = SIDE_BLUE; st3.cells[C04].revealed = 1;
        int r6_flag = (jq_can_move(&st3, C62, C60) == 0);
        ok &= chk("⑤ 工兵row6全宽横贯线全端点同段拐弯到目标格有敌方强子/军旗can=0(row6工兵(6,0)/(6,4)→col2(6,2)目标格红rank3小撞大送死can=0+row6工兵(6,2)→col0(6,0)目标格红军旗BFS拒非空格can=0,row6全宽横贯线全端点扩展到全场景对照)",
                  r6_eat && r6_eat4 && r6_flag);
    }
    /* ⑥ 对照非工兵 row6 全宽横贯线同段滑行到旗格扛旗放行（非工兵 rank1 (6,0)
       →(6,2) `can=1 EV_FLAG winner=BLUE`——非工兵 `rail_path_straight` 直走放
       行扛旗终局，对照 ②⑤ 工兵拒） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C60].rank = 1; st.cells[C60].side = SIDE_BLUE; st.cells[C60].revealed = 1; /* 蓝rank1强 */
        st.cells[C62].rank = RANK_FLAG; st.cells[C62].side = SIDE_RED; st.cells[C62].revealed = 1;
        st.cells[C00].rank = 2; st.cells[C00].side = SIDE_BLUE; st.cells[C00].revealed = 1;
        int can = jq_can_move(&st, C60, C62);
        int ev = jq_try_move(&st, C60, C62);
        ok &= chk("⑥ 对照非工兵row6全宽横贯线同段滑行到旗格扛旗放行(非工兵rank1(6,0)→(6,2)红军旗can=1 EV_FLAG winner=BLUE,rail_path_straight非工兵直走放行扛旗终局,对照②⑤工兵拒)",
                  can == 1 && ev == EV_FLAG && st.winner == JQ_WIN_BLUE
                  && st.cells[C60].rank == RANK_EMPTY
                  && st.cells[C62].rank == 1);
    }
    /* ⑦ 工兵 vs 非工兵终局交互路径分派差异实锤（工兵 BFS 拐弯路径引擎拒非空格
       雷/旗格 vs 非工兵 straight 直走放行扛旗，row5/row6 全宽横贯线全端点扩展
       到全场景对照） */
    {
        /* 工兵 row5 col0 端同段拐弯到 col2 目标格红军旗拒（② 既有） */
        jq_state st1; st1.mode = JQ_MODE_LAY;
        jq_state_init(&st1); memset(&st1.cells, 0, sizeof(st1.cells));
        st1.stage = STG_PLAY; st1.winner = JQ_WIN_NONE; st1.turn = SIDE_BLUE;
        st1.cells[C50].rank = RANK_WORKER; st1.cells[C50].side = SIDE_BLUE; st1.cells[C50].revealed = 1;
        st1.cells[C52].rank = RANK_FLAG; st1.cells[C52].side = SIDE_RED; st1.cells[C52].revealed = 1;
        st1.cells[C00].rank = 1; st1.cells[C00].side = SIDE_BLUE; st1.cells[C00].revealed = 1;
        int wk_rej = (jq_can_move(&st1, C50, C52) == 0);
        /* 非工兵 row5 col0 端同段滑行到 col2 目标格红军旗放行（③ 既有） */
        jq_state st2; st2.mode = JQ_MODE_LAY;
        jq_state_init(&st2); memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[C50].rank = 1; st2.cells[C50].side = SIDE_BLUE; st2.cells[C50].revealed = 1;
        st2.cells[C52].rank = RANK_FLAG; st2.cells[C52].side = SIDE_RED; st2.cells[C52].revealed = 1;
        st2.cells[C00].rank = 2; st2.cells[C00].side = SIDE_BLUE; st2.cells[C00].revealed = 1;
        int nonwk_ok = (jq_can_move(&st2, C50, C52) == 1);
        ok &= chk("⑦ 工兵vs非工兵终局交互路径分派差异实锤(工兵BFS拐弯路径引擎拒非空格军旗[②⑤can=0]vs非工兵straight直走放行扛旗[③⑥can=1EV_FLAG],row5/row6全宽横贯线全端点扩展到全场景对照)",
                  wk_rej && nonwk_ok);
    }
    printf("dark_rail_r5r6_worker_eat_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
