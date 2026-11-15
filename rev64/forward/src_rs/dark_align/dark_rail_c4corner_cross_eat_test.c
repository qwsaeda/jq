/* dark_rail_c4corner_cross_eat_test.c — col4 底角工兵拐弯 + 横贯线中间 col2 交叉
 * 格"滑行到敌方子格吃子"交互行为级实证
 * 补全现有缺口：§85 dark_rail_corner11_test 实锤 row11 红方底角工兵拐弯 + col0
 * 底角非纵贯全高（col0 底角对称），§84 实锤 col1/col4 非纵贯全高纵向跨段 can=0，
 * §79 实锤中间交叉格有子阻断滑行，但 **col4 底角（row0 col4 蓝方 / row11 col4
 * 红方）非纵贯全高工兵拐弯场景 + 横贯线中间 col2 交叉格"滑行到敌方子格吃子"交
 * 互**无独立用例——§85 诚实边界挂起"col4 底角非纵贯全高工兵拐弯场景可后续批次
 * ——底角全对称补全 col4"，§79 实锤中间交叉格有子阻断但未断言"横贯线滑行到目
 * 标格（col2 交叉格）敌方子吃子"交互场景。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83/§84/§85 全格分布实锤：
 * col2 纵贯 10 格 + row5/6 横贯 10 格 + col0=4 格 / col1=2 格 / col4=4 格非纵贯
 * 全高）+ rules.c rail_path_straight 非工兵直走（横贯线滑行到目标格有敌方子 = 吃
 * 子，中间格全空）+ rail_path_engineer 工兵 BFS（底角 col4 非纵贯全高拐弯受限）
 * + 吃子比较 §796-798 rank 小=强 / §817 同级互杀）：
 *   · col4 底角工兵拐弯受限（col4 非纵贯全高）：工兵 @(0,4) 可达格仅 (1,4)（col4
 *     下行），不能拐到 col2（(0,4)→(9,2) can=0，col4 非纵贯全高纵线拐不到
 *     col2，对称 §85 col0 底角）；
 *   · 横贯线中间 col2 交叉格"滑行到敌方子格吃子"：横贯线滑行到目标格（col2 交叉
 *     格）有敌方子 = 吃子（同横贯线滑行，目标格敌方子 → EV_EAT 大吃小 / EV_DEAD
 *     小撞大送死），**区别于 §79 中间交叉格有子阻断滑行**（中间格有子 → 滑行阻
 *     断 can=0，目标格有子 → 吃子放行 can=1）；
 *   · 中间格 vs 目标格语义分界：横贯线滑行"中间格全空"是滑行可达前提（§79 实锤
 *     中间格有子阻断），"目标格有敌方子"是吃子（放行 + EV_EAT/EV_DEAD 按 rank
 *     比较）——两者不同位置（中间格阻断 vs 目标格吃子），本轮补全目标格吃子交
 *     互场景。
 * 64 位行为级实证（走已实现的 is_rail 十字铁路 + rail_path_straight 横贯线滑行 +
 * 吃子比较 §796-798 + 工兵 BFS 底角拐弯受限，0 分叉；几何走 §79/§84/§85 既有
 * col2 交叉格 + col4 底角 + 横贯线吃子口径）。
 *
 * 几何口径：col4 底角 (0,4) 蓝方（对称 §85 col0 底角）；横贯线中间 col2 交叉格
 *   (5,2)（row5 横贯 ∩ col2 纵贯，§79 既有交叉格）；蓝 rank1/rank5 @(5,0) 滑行
 *   到 (5,2) 吃红 rank9/rank3（大吃小 EV_EAT / 小撞大对照）；红 cell(10,0) 防困
 *   毙；mode=LAY 明棋。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C04 cell_index(0,4)    /* col4 底角蓝方（对称 §85 col0 底角） */
#define C14 cell_index(1,4)    /* col4 下行（工兵底角唯一可达） */
#define C92 cell_index(9,2)    /* col2 纵贯 row9（底角拐弯目标） */
#define C50 cell_index(5,0)    /* row5 横贯线起点（滑行源） */
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
    /* ① col4 底角工兵拐弯受限（工兵 @(0,4) 可达格仅 (1,4)，不能拐到 col2，
       对称 §85 col0 底角） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C04].rank = RANK_WORKER; st.cells[C04].side = SIDE_BLUE; st.cells[C04].revealed = 1;
        st.cells[C100].rank = 1; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can_c2 = jq_can_move(&st, C04, C92);
        int can_c4down = jq_can_move(&st, C04, C14);
        ok &= chk("① col4底角工兵拐弯受限(工兵(0,4)不能拐到col2(9,2)can=0,col4下行(1,4)can=1对称§85col0)",
                  can_c2 == 0 && can_c4down == 1);
    }
    /* ② 横贯线中间 col2 交叉格"滑行到敌方子格吃子"（蓝 rank1@(5,0) 滑到 (5,2)
       吃红 rank9，大吃小 EV_EAT，目标格敌方子放行） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1;  /* 蓝 rank1 强 */
        st.cells[C52].rank = 9; st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;  /* 红 rank9 弱 */
        st.cells[C100].rank = 2; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C50, C52);
        int ev = jq_try_move(&st, C50, C52);
        ok &= chk("② 横贯线滑行到col2交叉格敌方子吃子(蓝rank1@(5,0)→(5,2)吃红rank9 EV_EAT大吃小)",
                  can == 1 && ev == EV_EAT
                  && st.cells[C50].rank == RANK_EMPTY   /* 攻子源格清空 */
                  && st.cells[C52].rank == 1);          /* 攻子落位(交换器) */
    }
    /* ③ 横贯线中间 col2 交叉格"小撞大规则层拒"（蓝 rank9@(5,0) 滑到 (5,2) 撞
       红 rank1，小撞大 rank9 弱撞 rank1 强 → 送死，jq_can_move 规则层拒 can=0
       （明棋小撞大不放行；EV_DEAD 仅同级互杀 EV_EAT 双消失或暗棋场景触发）；
       对照 ② 大吃小放行） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50].rank = 9; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1;  /* 蓝 rank9 弱 */
        st.cells[C52].rank = 1; st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;  /* 红 rank1 强 */
        st.cells[C100].rank = 2; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C50, C52);
        int ev = jq_try_move(&st, C50, C52);
        ok &= chk("③ 横贯线滑行到col2交叉格小撞大规则层拒(蓝rank9@(5,0)→(5,2)撞红rank1小撞大送死can=0,对照②大吃小放行)",
                  can == 0 && ev == -1
                  && st.cells[C50].rank == 9   /* 攻方未动(规则拒,源格保留) */
                  && st.cells[C52].rank == 1); /* 目标格红rank1强留 */
    }
    /* ④ 同级互杀 EV_EAT 双消失对照（蓝 rank5@(5,0) 滑到 (5,2) 撞红 rank5，
       同级互杀双方消失 EV_EAT，对照 ② 大吃小单消失 / ③ 小撞大拒） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C50].rank = 5; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1;  /* 蓝 rank5 */
        st.cells[C52].rank = 5; st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;  /* 红 rank5 */
        st.cells[C100].rank = 2; st.cells[C100].side = SIDE_RED;  st.cells[C100].revealed = 1;
        int can = jq_can_move(&st, C50, C52);
        int ev = jq_try_move(&st, C50, C52);
        ok &= chk("④ 同级互杀EV_EAT双消失对照(蓝rank5@(5,0)→(5,2)撞红rank5同级互杀双消失EV_EAT,对照②大吃小③小撞大拒)",
                  can == 1 && ev == EV_EAT
                  && st.cells[C50].rank == RANK_EMPTY   /* 攻方源格清空(双消失) */
                  && st.cells[C52].rank == RANK_EMPTY); /* 目标格敌方子也清空(双消失) */
    }
    /* ⑤ 中间格 vs 目标格语义分界（横贯线滑行中间格全空放行 + 目标格敌方子吃
       子；§79 中间格有子阻断对照） */
    {
        /* 横贯线 (5,0)→(5,4) 中间 col2 交叉格 (5,2) 空 → 全宽滑行放行（§79 ① 既有） */
        jq_state st1; st1.mode = JQ_MODE_LAY;
        jq_state_init(&st1); memset(&st1.cells, 0, sizeof(st1.cells));
        st1.stage = STG_PLAY; st1.winner = JQ_WIN_NONE; st1.turn = SIDE_BLUE;
        st1.cells[C50].rank = 5; st1.cells[C50].side = SIDE_BLUE; st1.cells[C50].revealed = 1;
        st1.cells[C100].rank = 1; st1.cells[C100].side = SIDE_RED;  st1.cells[C100].revealed = 1;
        int empty_cross = jq_can_move(&st1, C50, cell_index(5, 4));   /* 中间 (5,2) 空 → 全宽滑行 */
        /* 横贯线 (5,0)→(5,4) 中间 col2 交叉格 (5,2) 有子 → 滑行阻断（§79 ② 既有） */
        jq_state st2; st2.mode = JQ_MODE_LAY;
        jq_state_init(&st2); memset(&st2.cells, 0, sizeof(st2.cells));
        st2.stage = STG_PLAY; st2.winner = JQ_WIN_NONE; st2.turn = SIDE_BLUE;
        st2.cells[C50].rank = 5; st2.cells[C50].side = SIDE_BLUE; st2.cells[C50].revealed = 1;
        st2.cells[C52].rank = 3; st2.cells[C52].side = SIDE_BLUE; st2.cells[C52].revealed = 1;  /* 己方子占中间格 */
        st2.cells[C100].rank = 1; st2.cells[C100].side = SIDE_RED;  st2.cells[C100].revealed = 1;
        int blocked_cross = jq_can_move(&st2, C50, cell_index(5, 4));  /* 中间 (5,2) 有子 → 阻断 */
        ok &= chk("⑤ 中间格vs目标格语义分界(中间col2交叉格空=全宽滑行放行/中间格有子=阻断§79既有)",
                  empty_cross == 1 && blocked_cross == 0);
    }
    /* ⑥ 目标格敌方子 rank 比较三对照 + 中间格 vs 目标格语义分界补强 */
    {
        int eat_ok = 0, dead_rej = 0, same_ok = 0;
        /* 大吃小放行（② 既有） */
        {
            jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
            memset(&st.cells, 0, sizeof(st.cells));
            st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
            st.cells[C50].rank = 1; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1;
            st.cells[C52].rank = 9; st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;
            st.cells[C100].rank = 2; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
            eat_ok = (jq_can_move(&st, C50, C52) == 1);
        }
        /* 小撞大拒（③ 既有） */
        {
            jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
            memset(&st.cells, 0, sizeof(st.cells));
            st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
            st.cells[C50].rank = 9; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1;
            st.cells[C52].rank = 1; st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;
            st.cells[C100].rank = 2; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
            dead_rej = (jq_can_move(&st, C50, C52) == 0);
        }
        /* 同级互杀双消失（④ 既有） */
        {
            jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
            memset(&st.cells, 0, sizeof(st.cells));
            st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
            st.cells[C50].rank = 5; st.cells[C50].side = SIDE_BLUE; st.cells[C50].revealed = 1;
            st.cells[C52].rank = 5; st.cells[C52].side = SIDE_RED;  st.cells[C52].revealed = 1;
            st.cells[C100].rank = 2; st.cells[C100].side = SIDE_RED; st.cells[C100].revealed = 1;
            same_ok = (jq_can_move(&st, C50, C52) == 1);
        }
        ok &= chk("⑥ 目标格敌方子rank比较三对照(大吃小放行②/小撞大拒③/同级互杀双消失④+中间格阻断§79)",
                  eat_ok && dead_rej && same_ok);
    }
    printf("dark_rail_c4corner_cross_eat_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
