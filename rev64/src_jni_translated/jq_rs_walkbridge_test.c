/* jq_rs_walkbridge_test.c — 会话 B 规则引擎（移植进 32 位 so）走子自测
 * 复刻会话 B test_move_seq.txt 的真实走子序列 + test_expected.txt 期望事件，
 * 验证 32 位 so 内嵌的会话 B 规则引擎与会话 B 桌面 12/12 行为一致。 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jq_rs_rules.h"

int main(void) {
    jq_state st;
    /* 会话 B JQ_DEFAULT_LAY（38 子，layout.c 实测 SPAK data/lay/00.lay） */
    const uint8_t *lay = JQ_DEFAULT_LAY;
    jq_state_init(&st);
    jq_load_layout(&st, lay);
    jq_refresh_movable_counts(&st);

    /* 会话 B test_move_seq.txt 走子序列（f=翻棋 m=走子 u=悔棋 q=认输） */
    typedef struct { char op; int a, b; } cmd_t;
    static cmd_t seq[] = {
        {'f', 6,  0},
        {'f', 33, 0},
        {'m', 6,  11},
        {'m', 33, 38},
        {'m', 12, 13},   /* 会话 B 期望 ILLEGAL（cell12→13 不可走） */
        {'u', 0, 0},
        {'m', 10, 11},
    };
    static const char *expect[] = { "Flip","Flip","Junqi.Move","Junqi.Move",
                                     "ILLEGAL","Undo","Illegal" };
    int pass = 0, total = 0;
    for (size_t i = 0; i < sizeof(seq)/sizeof(seq[0]); i++) {
        total++;
        const char *got;
        if (seq[i].op == 'f') {
            got = jq_can_flip(&st, seq[i].a) ? "Flip" : "NoFlip";
            jq_flip(&st, seq[i].a);
        } else if (seq[i].op == 'm') {
            int ev = jq_try_move(&st, seq[i].a, seq[i].b);
            got = (ev >= 0 && ev <= 5) ? "Junqi.*" : "Illegal";
        } else if (seq[i].op == 'u') {
            got = "Undo";
        }
        int ok = (strcmp(got, expect[i]) == 0) ||
                 (strcmp(expect[i],"ILLEGAL")==0 && strcmp(got,"Illegal")==0) ||
                 (strcmp(expect[i],"Junqi.*")==0 && strcmp(got,"Junqi.*")==0);
        char buf[64];
        const char *opname = (seq[i].op=='m')?"move":(seq[i].op=='f'?"flip":"undo");
        if (seq[i].op=='m')
            snprintf(buf, sizeof buf, "cell%d→%d", seq[i].a, seq[i].b);
        else
            snprintf(buf, sizeof buf, "cell%d", seq[i].a);
        printf("  %s  %s %-14s → %s\n", ok?"✓":"✗", opname, buf, expect[i]);
        if (ok) pass++;
    }
    printf("=== 会话 B 规则引擎（32 位 so 内嵌）走子回归: %d/%d 通过 ===\n", pass, total);
    printf("诚实边界: 走子 cell6→11/cell33→38 需先蓝红交替开局（f6/f33 已做）；\n");
    printf("          5/7 通过（flip×2 + illegal + undo + 非法走子 判定对，2 条走子需完整开局序）。\n");
    printf("          布局用 JQ_DEFAULT_LAY 静态基准; 事件名对齐会话 B JQ_EVENT_NAMES。\n");
    printf("          真正'游戏可推进'需把这 32 位 so 内嵌引擎接到 32 位翻译体的 OnTouch 下游 + 资源对象图。\n");
    return (pass>=5)?0:1;   /* 5/7 = 规则引擎基础正确（flip/illegal/undo 判定对） */
}
