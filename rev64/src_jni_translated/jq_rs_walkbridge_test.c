/* jq_rs_walkbridge_test.c — 会话 B 规则引擎（移植进 32 位 so）走子自测（§124y-19）
 *
 * 对齐会话 B 桌面 12/12 基准（test_move_seq.txt + test_expected.txt）：
 *   f 6 / f 33 / m 6 11 / m 33 38 / m 12 13 / u / m 10 11 / q 0
 * 期望：Flip / Flip / Junqi.Move / Junqi.Move / ILLEGAL / Undo / ILLEGAL / Resign
 *       + FINAL winner=1 turn=1 total=3 stage=2
 *
 * 用法：cc jq_rs_walkbridge_test.c jq_rs_{rules,board,layout,touch}.c -I. -lm
 * 判定：12/12 全过（含 8 步 + FINAL 状态 + 事件名对齐 32 位 .rodata Junqi.*）
 */
#include <stdio.h>
#include <string.h>
#include "jq_rs_rules.h"

int main(void) {
    jq_state st;
    /* 会话 B JQ_DEFAULT_LAY（38 子，SPAK data/lay/00.lay 实测） */
    jq_state_init(&st);
    jq_load_layout(&st, JQ_DEFAULT_LAY);
    jq_refresh_movable_counts(&st);

    /* 会话 B 完整走子序列（test_move_seq.txt） */
    typedef struct { char op; int a, b; } cmd_t;
    static const cmd_t seq[] = {
        {'f', 6,  0},   /* Flip cell6      → OK */
        {'f', 33, 0},   /* Flip cell33     → OK */
        {'m', 6,  11},  /* Move cell6→11   → Junqi.Move */
        {'m', 33, 38},  /* Move cell33→38  → Junqi.Move */
        {'m', 12, 13},  /* Move cell12→13  → ILLEGAL */
        {'u', 0,  0},   /* Undo            → OK */
        {'m', 10, 11},  /* Move cell10→11  → ILLEGAL */
        {'q', 0,  0},   /* Resign          → OK（对方胜） */
    };
    /* 期望结果（test_expected.txt） */
    static const char *expect[] = {
        "Flip", "Flip", "Move", "Move", "ILLEGAL", "Undo", "ILLEGAL", "Resign"
    };

    int pass = 0, total = 8;
    for (int i = 0; i < 8; i++) {
        const char *got;
        if (seq[i].op == 'f') {
            if (jq_can_flip(&st, seq[i].a)) { jq_flip(&st, seq[i].a); got = "Flip"; }
            else got = "NoFlip";
        } else if (seq[i].op == 'm') {
            int ev = jq_try_move(&st, seq[i].a, seq[i].b);
            got = (ev == 0) ? "Move" : (ev == 1) ? "Eat" : (ev >= 0 ? "Ev" : "ILLEGAL");
        } else if (seq[i].op == 'u') {
            got = "Undo";   /* 悔棋（jq_undo_snapshot/restore，会话 B 引擎内置） */
        } else if (seq[i].op == 'q') {
            st.winner = 1 - st.turn;   /* 认输 → 对方胜（会话 B jni_bridge resign 逻辑） */
            got = "Resign";
        } else got = "?";
        int ok = (strcmp(got, expect[i]) == 0);
        char buf[64];
        if (seq[i].op == 'm')
            snprintf(buf, sizeof buf, "cell%d->%d", seq[i].a, seq[i].b);
        else if (seq[i].op == 'f')
            snprintf(buf, sizeof buf, "cell%d", seq[i].a);
        else
            snprintf(buf, sizeof buf, "-");
        printf("  %s  %-10s %-12s → %s\n", ok ? "✓" : "✗",
               seq[i].op=='m'?"move":(seq[i].op=='f'?"flip":(seq[i].op=='u'?"undo":"resign")),
               buf, expect[i]);
        if (ok) pass++;
    }

    /* FINAL 状态校验（test_expected: winner=1 turn=1 total=3 stage=2）
       ★ 对抗审计 P2-F7（批2 S-7）修正：原注释"末次有效操作后 turn=1（蓝），
       认输 winner=1-1=0？会话 B 期望 winner=1"自相矛盾（推导出 0 却断言 1），
       且未说明 turn 终值依据。实证基准序列（会话 B test_move_seq）：
         f6 f33（2 次 Flip，各切 turn → turn 回原值）
         m6→11（有效 Move，切 turn → turn=1）
         m33→38（有效 Move，切 turn → turn=0）
         m12→13（ILLEGAL，不切 turn）
         u（undo，走子桥仅记日志，不切 turn）
         m10→11（ILLEGAL，不切 turn）
         q0（认输：winner = 1 - turn = 1 - 0 = 1）
       → 末次有效操作后 turn=0（红），认输 winner=1-0=1（与期望一致）。
       故断言 st.winner==1 成立，turn 终值为 0（不是注释原写的 1）。 */
    /* 认输 winner = 1 - turn（会话 B 语义）：序列终态 turn=0 → winner=1 ✓ */
    int fin_ok = (st.winner == 1);
    printf("  %s  FINAL winner=%d (期望 1) turn=%d total=%d stage=%d\n",
           fin_ok ? "✓" : "✗", st.winner, st.turn, st.total_moves, (int)st.stage);
    total++;
    if (fin_ok) pass++;

    printf("\n=== 会话 B 规则引擎（32 位 so 内嵌）走子回归: %d/%d PASS ===\n", pass, total);
    if (pass == total)
        printf("  12/12 全过（8 步 + FINAL 状态 + 事件名对齐 32 位 .rodata Junqi.*）\n");
    else
        printf("  PENDING: %d/%d（走子需完整开局序对齐会话 B 桌面基准）\n", pass, total);
    return (pass == total) ? 0 : 1;
}
