/*
 * walkverify.c — 桌面预验证走子序列（批量，免真机盲目试探）
 *
 * 用法: dist/jq_walkverify "25,30 27,32 28,33" [--reveal]
 *   每对 "from,to" 为蓝方走子；蓝走 → 红 AI 应着（贪心）→ 下一步蓝。
 *   打印每步 can_move + try_move 事件（EAT/BOMB/MOVE/FLAG），红 AI 走子。
 *   验证通过走子路径合法 + 事件类型正确，再上真机（mumu_walktest.sh）。
 *
 * 对齐 jni_bridge ai_pick 契约（红方贪心：吃子>空移>送死），
 * 仅桌面预验证用（红方 AI 行为与真机 jni_bridge 一致，非逐字段逆向）。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "junqi.h"

extern const uint8_t JQ_DEFAULT_LAY[30];

/* 红方贪心 AI（对齐 jni_bridge ai_pick：吃子 sc=1000 > 空移 sc=2 > 送死 sc=1） */
static int red_pick(jq_state *st, int *from, int *to)
{
    uint8_t me = SIDE_RED;
    int bf = -1, bt = -1, bs = -1;
    for (int i = 0; i < N_CELLS; i++) {
        jq_cell *c = &st->cells[i];
        if (c->rank == RANK_EMPTY || c->side != me || !c->revealed) continue;
        int r = rank_norm(c->rank);
        if (r == RANK_FLAG || r == RANK_MINE) continue;
        for (int t = 0; t < N_CELLS; t++) {
            if (t == i) continue;
            jq_cell *d = &st->cells[t];
            if (!jq_can_move(st, i, t)) continue;
            int sc = d->rank ? (r < rank_norm(d->rank) ? 1000 : 1) : 2;
            if (sc > bs) { bs = sc; bf = i; bt = t; }
        }
    }
    if (bf >= 0) { *from = bf; *to = bt; return 0; }
    return -1;
}

/* 蓝方贪心 AI（longrun 模式：自动找蓝第一合法走子，turn 交替持续验证） */
static int blue_pick(jq_state *st, int *from, int *to)
{
    uint8_t me = SIDE_BLUE;
    int bf = -1, bt = -1, bs = -1;
    for (int i = 0; i < N_CELLS; i++) {
        jq_cell *c = &st->cells[i];
        if (c->rank == RANK_EMPTY || c->side != me || !c->revealed) continue;
        int r = rank_norm(c->rank);
        if (r == RANK_FLAG || r == RANK_MINE) continue;
        for (int t = 0; t < N_CELLS; t++) {
            if (t == i) continue;
            jq_cell *d = &st->cells[t];
            if (!jq_can_move(st, i, t)) continue;
            int sc = d->rank ? (r < rank_norm(d->rank) ? 1000 : 1) : 2;
            if (sc > bs) { bs = sc; bf = i; bt = t; }
        }
    }
    if (bf >= 0) { *from = bf; *to = bt; return 0; }
    return -1;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("用法: jq_walkverify \"25,30 27,32\" | --longrun N\n");
        return 2;
    }
    jq_state st;
    jq_load_layout(&st, JQ_DEFAULT_LAY);
    for (int i = 0; i < N_CELLS; i++)
        if (st.cells[i].rank) st.cells[i].revealed = 1;

    /* --longrun N：自动蓝走→红应着→蓝走... N 个完整回合，验证 turn 交替持续。
       真实长路径走子（5+ 回合 turn 交替）桌面验证，免手写序列受红 AI 局面影响。 */
    if (strcmp(argv[1], "--longrun") == 0 && argc >= 3) {
        int target = atoi(argv[2]);
        int rounds = 0, turn_ok = 1;
        for (int r = 0; r < target; r++) {
            if (st.turn != SIDE_BLUE || st.stage != STG_PLAY) break;
            int bf, bt;
            if (blue_pick(&st, &bf, &bt) != 0) break;
            int bev = jq_try_move(&st, bf, bt);
            if (bev < 0) break;
            rounds++;
            int r_from = -1, r_to = -1;
            if (st.turn == SIDE_RED && red_pick(&st, &r_from, &r_to) == 0) {
                int rev = jq_try_move(&st, r_from, r_to);
                if (rev >= 0 && st.turn != SIDE_BLUE) turn_ok = 0;   /* 红应着后应切蓝 */
            }
            printf("回合%d: 蓝 %d→%d %s | 红应着 %d→%d %s turn=%d stage=%d\n",
                   r + 1, bf, bt, bev < 0 ? "-" : JQ_EVENT_NAMES[bev],
                   r_from, r_to, r_from < 0 ? "-" : "走",
                   st.turn, st.stage);
            if (st.turn != SIDE_BLUE && st.stage == STG_PLAY) turn_ok = 0;
        }
        int ok = (rounds >= target) && turn_ok;
        printf("=== 桌面长路径验证: %d/%d 回合 turn 交替 %s ===\n", rounds, target,
               ok ? "持续" : "中断  [FAIL]");
        return ok ? 0 : 1;
    }

    char buf[256];
    snprintf(buf, sizeof buf, "%s", argv[1]);
    int total = 0, pass = 0;
    for (char *tok = strtok(buf, " "); tok; tok = strtok(NULL, " ")) {
        int from, to;
        if (sscanf(tok, "%d,%d", &from, &to) != 2) continue;
        if (st.turn != SIDE_BLUE) {   /* 红方应着（若未走） */
            int rf, rt;
            if (st.turn == SIDE_RED && red_pick(&st, &rf, &rt) == 0) {
                int rv = jq_try_move(&st, rf, rt);
                printf("  红AI %d→%d: ev=%d %s\n", rf, rt, rv,
                       rv < 0 ? "-" : JQ_EVENT_NAMES[rv]);
            }
        }
        int can = jq_can_move(&st, from, to);
        int ev = can ? jq_try_move(&st, from, to) : -1;
        total++;
        int ok = (ev >= 0);
        if (ok) pass++;
        printf("走子 %d→%d: can_move=%d ev=%d %s turn=%d%s\n",
               from, to, can, ev, ev < 0 ? "-" : JQ_EVENT_NAMES[ev], st.turn,
               ok ? "" : "  [FAIL]");
    }
    printf("=== 桌面预验证: %d/%d 步合法 ===\n", pass, total);
    return pass == total && total > 0 ? 0 : 1;
}
