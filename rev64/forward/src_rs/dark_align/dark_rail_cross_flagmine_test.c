/* dark_rail_cross_flagmine_test.c — 横贯线中间 col2 交叉格"滑行+吃雷/扛旗终局
 * 交互"行为级实证（明棋 vs 暗棋 + 一步相邻 vs 铁路滑行路径分派）
 * 补全现有缺口：§86 dark_rail_cross_eat_test 实锤横贯线滑行到 col2 交叉格"吃
 * 子放行 / 小撞大拒 / 同级互杀双消失"对照 + 中间格 vs 目标格语义分界（普通吃
 * 子场景），但**横贯线滑行到 col2 交叉格"扛旗/吃雷终局交互"**（EV_FLAG/EV_MINE
 * 终局场景，rules.c:318-336 暗棋专属分支 + :403-404 一步相邻放行 + :406-407 工
 * 兵 BFS walkable 拒非空格雷格）无独立用例——§86 诚实边界挂起"横贯线'滑行到
 * col2 交叉格扛旗/吃雷终局交互'（EV_FLAG/EV_MINE 场景）本轮未独立用例"。
 *
 * 对照 64 位实现（rules.c jq_can_move 路径分派（:403-404 一步相邻 return true
 * 直接放行 / :406-407 工兵 rail_path_engineer BFS 拐弯 / 非工兵
 * rail_path_straight 直走）+ walkable（:82-90 目标格必须 RANK_EMPTY 才通行，
 * 雷/旗非空格拒）+ 暗棋专属分支（:318-336 暗棋+which=4 开+目标格雷/旗→工兵放
 * 行）+ 终局判定（:754-795 EV_MINE 工兵吃雷 / :700 EV_FLAG 扛旗终局）+ board.c
 * is_rail 十字铁路（col2 纵贯 10 格 + row5/6 横贯 10 格））：
 *   · 明棋横贯线滑行到 col2 交叉格吃雷终局拒（工兵 BFS walkable 拒非空格雷格，
 *     工兵铁路滑行不可进入雷格 can=0）——工兵吃雷仅在一步相邻（公路线）或暗棋
 *     专属分支场景；
 *   · 明棋横贯线滑行到 col2 交叉格扛旗终局拒（同上 BFS 拒非空格旗格 can=0）
 *     ——扛旗终局仅在一步相邻或暗棋专属分支场景；
 *   · 明棋一步相邻（公路线）进雷格吃雷终局放行（:403-404 相邻 return true）
 *     ——工兵相邻进雷格 EV_MINE 工兵留雷消失（:793）；
 *   · 明棋一步相邻（公路线）进旗格扛旗终局放行（:403-404 相邻 return true）
 *     ——扛旗 EV_FLAG 攻方落位红旗消失终局 winner=攻方（:700）；
 *   · 暗棋横贯线滑行到 col2 交叉格吃雷终局放行（:318-323 暗棋+which=4 开+工
 *     兵放行）——暗棋专属分支放行工兵铁路滑行吃雷 EV_MINE；
 *   · 暗棋横贯线滑行到 col2 交叉格扛旗终局放行（:318-336 暗棋+which=4 开+工
 *     兵+挖满 3 雷放行）——暗棋专属分支放行工兵铁路滑行扛旗 EV_FLAG（mines_
 *     cleared[me]>=3 门槛）。
 * 64 位行为级实证（走已实现的 jq_can_move 路径分派 + walkable + 暗棋专属分支
 * + 终局判定，0 分叉；几何走 §86 既有 col2 交叉格 + 横贯线/一步相邻路径分派
 * + 暗棋 which=4 开关口径）。
 *
 * 几何口径：col2 交叉格 (5,2)（row5 横贯 ∩ col2 纵贯，§79/§86 既有交叉格）；
 *   横贯线滑行 (5,0)→(5,2)（同 row5 横贯线，col0→col2 跨 col1）；一步相邻
 *   (5,1)→(5,2)（col1→col2 一步相邻公路线）；红雷/旗 cell(5,2) 目标格；蓝工
 *   兵/非工兵 cell(5,0)/(5,1) 攻方；红防困毙 cell(10,0)；mode=LAY/FLIP；明棋
 *   which=4 默认开 / 暗棋 which=4 默认开（ui_state_stub 默认全 1）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C50 cell_index(5,0)    /* row5 横贯线起点（滑行源） */
#define C51 cell_index(5,1)    /* row5 col1（一步相邻源） */
#define C52 cell_index(5,2)    /* 横贯线中间 col2 交叉格（目标格） */
#define C100 cell_index(10,0)  /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 明棋横贯线滑行到 col2 交叉格吃雷终局拒（工兵 BFS walkable 拒非空格
       雷格，工兵铁路滑行不可进入雷格 can=0——工兵吃雷仅一步相邻或暗棋场景） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50].rank = RANK_WORKER; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1;
        st.cells[C52].rank = RANK_MINE;  st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C50, C52);
        int ev = jq_try_move(&st, C50, C52);
        ok &= chk("① 明棋横贯线滑行到col2交叉格吃雷终局拒(工兵(5,0)→(5,2)滑到雷格BFS walkable拒非空格can=0工兵吃雷仅一步相邻或暗棋)",
                  can == 0 && ev == -1
                  && st.cells[C50].rank == RANK_WORKER  /* 工兵未动 */
                  && st.cells[C52].rank == RANK_MINE);  /* 雷留 */
    }
    /* ② 明棋非工兵横贯线滑行到 col2 交叉格扛旗终局放行（rail_path_straight 非工
       兵直走放行，非工兵横贯线同 row5 滑行到旗格 EV_FLAG 攻方落位红旗消失终局
       winner=攻方 :700——非工兵扛旗不依赖 BFS walkable，走 straight 直走路径） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1; /* 蓝rank1强 */
        st.cells[C52].rank = RANK_FLAG; st.cells[C52].side = SIDE_RED; st.cells[C52].revealed = 1;
        st.cells[C100].rank = 2; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C50, C52);
        int ev = jq_try_move(&st, C50, C52);
        ok &= chk("② 明棋非工兵横贯线滑行到col2交叉格扛旗终局放行(蓝rank1(5,0)→(5,2)非工兵straight直走EV_FLAG攻方落位红旗消失winner=BLUE:700)",
                  can == 1 && ev == EV_FLAG
                  && st.cells[C50].rank == RANK_EMPTY  /* 攻方源格清空 */
                  && st.cells[C52].rank == 1           /* 攻子落位(红旗消失) */
                  && st.winner == JQ_WIN_BLUE);        /* 终局蓝胜 */
    }
    /* ③ 明棋一步相邻（公路线）进雷格吃雷终局放行（:403-404 相邻 return true，
       工兵相邻进雷格 EV_MINE 工兵留雷消失 :793） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = RANK_WORKER; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1;
        st.cells[C52].rank = RANK_MINE;  st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C51, C52);
        int ev = jq_try_move(&st, C51, C52);
        ok &= chk("③ 明棋一步相邻进雷格吃雷终局放行(工兵(5,1)→(5,2)相邻进雷格EV_MINE工兵留雷消失:403-404相邻return true)",
                  can == 1 && ev == EV_MINE
                  && st.cells[C51].rank == RANK_EMPTY  /* 工兵源格清空(工兵留雷格) */
                  && st.cells[C52].rank == RANK_WORKER); /* 工兵落位雷格(雷消失) */
    }
    /* ④ 明棋一步相邻（公路线）进旗格扛旗终局放行（:403-404 相邻 return true，
       扛旗 EV_FLAG 攻方落位红旗消失终局 winner=攻方 :700） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C51].rank = 1; st.cells[C51].side = SIDE_BLUE; st.cells[C51].revealed = 1; /* 蓝rank1 */
        st.cells[C52].rank = RANK_FLAG; st.cells[C52].side = SIDE_RED; st.cells[C52].revealed = 1;
        st.cells[C100].rank = 2; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C51, C52);
        int ev = jq_try_move(&st, C51, C52);
        ok &= chk("④ 明棋一步相邻进旗格扛旗终局放行(蓝rank1(5,1)→(5,2)相邻进旗格EV_FLAG攻方落位红旗消失终局winner=BLUE:700)",
                  can == 1 && ev == EV_FLAG
                  && st.cells[C51].rank == RANK_EMPTY  /* 攻方源格清空(落位旗格) */
                  && st.cells[C52].rank == 1           /* 攻子落位(红旗消失) */
                  && st.winner == JQ_WIN_BLUE);        /* 终局蓝胜 */
    }
    /* ⑤ 暗棋工兵横贯线滑行到 col2 交叉格吃雷终局拒（工兵 rail_path_engineer BFS
       walkable 拒非空格雷格，暗棋专属分支 :318-323 放行"工兵可吃雷"判定但路径
       可达性仍走 BFS，BFS 拒非空格雷格 can=0——工兵吃雷仅一步相邻场景；对照
       ⑥ 暗棋工兵一步相邻进雷格放行） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50].rank = RANK_WORKER; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1;
        st.cells[C52].rank = RANK_MINE;  st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C50, C52);
        int ev = jq_try_move(&st, C50, C52);
        /* 暗棋工兵横贯线滑行吃雷：BFS walkable 拒非空格雷格 can=0 */
        int can5_rej = (can == 0 && ev == -1);
        /* 暗棋工兵一步相邻进雷格（对照放行）：(5,1)→(5,2) 相邻 return true */
        jq_state st5; jq_state_init(&st5); st5.mode = JQ_MODE_FLIP;
        memset(&st5.cells, 0, sizeof(st5.cells));
        st5.stage = STG_PLAY; st5.winner = JQ_WIN_NONE; st5.turn = SIDE_BLUE;
        st5.cells[C51].rank = RANK_WORKER; st5.cells[C51].side = SIDE_BLUE; st5.cells[C51].revealed = 1;
        st5.cells[C52].rank = RANK_MINE;  st5.cells[C52].side = SIDE_RED;  st5.cells[C52].revealed = 1;
        st5.cells[C100].rank = 1; st5.cells[C100].side = SIDE_RED; st5.cells[C100].revealed = 1;
        int can5_adj = jq_can_move(&st5, C51, C52);
        int ev5_adj = jq_try_move(&st5, C51, C52);
        int can5_adj_ok = (can5_adj == 1 && ev5_adj == EV_MINE);
        ok &= chk("⑤ 暗棋工兵横贯线滑行到col2交叉格吃雷终局拒(BFS walkable拒非空格雷格can=0,对照一步相邻放行:318-323专属分支+相邻return true)",
                  can5_rej && can5_adj_ok);
    }
    /* ⑥ 暗棋工兵横贯线滑行到 col2 交叉格扛旗终局拒（工兵 rail_path_engineer BFS
       walkable 拒非空格旗格，暗棋专属分支 :318-336 放行"工兵可扛旗+挖满 3 雷"
       判定但路径可达性仍走 BFS，BFS 拒非空格旗格 can=0——工兵扛旗仅一步相邻
       场景；对照暗棋工兵一步相邻进旗格放行） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50].rank = RANK_WORKER; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1;
        st.cells[C52].rank = RANK_FLAG;  st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;
        st.cells[C100].rank = 2; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
        st.mines_cleared[SIDE_BLUE] = 3;  /* 攻方工兵已挖满 3 雷（扛旗门槛） */
        int can = jq_can_move(&st, C50, C52);
        int ev = jq_try_move(&st, C50, C52);
        /* 暗棋工兵横贯线滑行扛旗：BFS walkable 拒非空格旗格 can=0 */
        int can6_rej = (can == 0 && ev == -1);
        /* 暗棋工兵一步相邻进旗格（对照放行）：(5,1)→(5,2) 相邻 return true +
           暗棋专属分支 :318-336 工兵+挖满 3 雷放行 EV_FLAG */
        jq_state st6; jq_state_init(&st6); st6.mode = JQ_MODE_FLIP;
        memset(&st6.cells, 0, sizeof(st6.cells));
        st6.stage = STG_PLAY; st6.winner = JQ_WIN_NONE; st6.turn = SIDE_BLUE;
        st6.cells[C51].rank = RANK_WORKER; st6.cells[C51].side = SIDE_BLUE; st6.cells[C51].revealed = 1;
        st6.cells[C52].rank = RANK_FLAG;  st6.cells[C52].side = SIDE_RED;  st6.cells[C52].revealed = 1;
        st6.cells[C100].rank = 2; st6.cells[C100].side = SIDE_RED; st6.cells[C100].revealed = 1;
        st6.mines_cleared[SIDE_BLUE] = 3;
        int can6_adj = jq_can_move(&st6, C51, C52);
        int ev6_adj = jq_try_move(&st6, C51, C52);
        int can6_adj_ok = (can6_adj == 1 && ev6_adj == EV_FLAG && st6.winner == JQ_WIN_BLUE);
        ok &= chk("⑥ 暗棋工兵横贯线滑行到col2交叉格扛旗终局拒(BFS walkable拒非空格旗格can=0,对照一步相邻+挖满3雷放行:318-336专属分支+相邻return true)",
                  can6_rej && can6_adj_ok);
    }
    printf("dark_rail_cross_flagmine_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
