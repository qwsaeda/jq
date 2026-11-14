/* draw_exist_test.c — "双方无普通子→Draw / 一方无普通子→对方胜" 存在性验证
 * 走子后触发 stalemate_after，观察 winner。
 *  ① 双方都只剩旗/雷（无普通子）→ Draw
 *  ② 一方无普通子（被炸光）另一方有 → 对方胜（困毙）
 *  ③ 双方都有普通子但都困毙（flag-off 场景）→ 不 Draw → 单侧困毙 → BLUE 胜
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"

static void put(jq_state *st, int row, int col, int rank, int side, int rev)
{
    jq_cell *c = &st->cells[cell_index(row, col)];
    c->rank = (uint8_t)rank; c->side = (uint8_t)side;
    c->revealed = (uint8_t)(rev ? 1 : 0); c->flags = 0;
    if (rank == RANK_FLAG) c->flags = JQF_FLAG;
    if (rank == RANK_MINE) c->flags = JQF_MINE;
}

static int run_scenario(const char *name, void (*setup)(jq_state*), int from, int to)
{
    jq_state st;
    memset(&st, 0, sizeof(st));
    jq_state_init(&st);
    st.mode = JQ_MODE_LAY; st.stage = STG_PLAY; st.winner = JQ_WIN_NONE;
    st.turn = SIDE_BLUE;
    setup(&st);
    if (jq_can_move(&st, from, to)) {
        int ev = jq_try_move(&st, from, to);
        printf("%s: 走子 %d→%d ev=%d stage=%d winner=%d\n", name, from, to, ev,
               st.stage, st.winner);
        return st.winner;
    }
    printf("%s: %d→%d 不合法 stage=%d winner=%d\n", name, from, to, st.stage, st.winner);
    return st.winner;
}

/* ① 蓝炸弹+蓝地雷（无普通子），红 rank3 @54：蓝炸红 → 双方无普通子 → Draw */
static void s1(jq_state *st)
{
    put(st, 9, 4, RANK_BOMB, SIDE_BLUE, 1);     /* 蓝炸弹 @49 */
    put(st, 10, 0, RANK_MINE, SIDE_BLUE, 1);    /* 蓝地雷 @50 */
    put(st, 10, 4, 3 + 0xc, SIDE_RED, 1);       /* 红 rank3 @54（普通子） */
}

/* ② 蓝炸弹+蓝 rank2（有普通子），红 rank3 @54：蓝炸红 → 红无普通子 → 蓝胜 */
static void s2(jq_state *st)
{
    put(st, 9, 4, RANK_BOMB, SIDE_BLUE, 1);     /* 蓝炸弹 @49 */
    put(st, 10, 0, 2, SIDE_BLUE, 1);            /* 蓝 rank2 @50（普通子） */
    put(st, 10, 4, 3 + 0xc, SIDE_RED, 1);       /* 红 rank3 @54（普通子） */
}

/* ③ flag-off 场景：蓝 rank2 攻红军旗（开关关→不终局），红无子 → 蓝胜（非 Draw） */
static void s3(jq_state *st)
{
    put(st, 10, 3, 2, SIDE_BLUE, 1);            /* 蓝 rank2 @53 */
    put(st, 11, 3, RANK_FLAG + 0xc, SIDE_RED, 1); /* 红军旗 @58 */
}

int main(void)
{
    int w1 = run_scenario("① 双方仅旗/雷(无普通子)", s1, 49, 54);
    int ok = 1;
    printf("① 期望 DRAW 实际 %d → %s\n", w1, w1 == JQ_WIN_DRAW ? "PASS" : "FAIL");
    ok &= (w1 == JQ_WIN_DRAW);

    int w2 = run_scenario("② 蓝有普通子炸光红", s2, 49, 54);
    printf("② 期望 BLUE 实际 %d → %s\n", w2, w2 == JQ_WIN_BLUE ? "PASS" : "FAIL");
    ok &= (w2 == JQ_WIN_BLUE);

    ui_rule_flag_set(0);
    int w3 = run_scenario("③ flag-off 蓝攻旗(开关关)", s3, 53, 58);
    ui_rule_flag_set(1);
    printf("③ 期望 BLUE(红无子困毙) 实际 %d → %s\n", w3, w3 == JQ_WIN_BLUE ? "PASS" : "FAIL");
    ok &= (w3 == JQ_WIN_BLUE);

    printf("draw_exist_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
