/*
 * consistency_desktop.c — 一致测试（重写版规则引擎侧）
 * 用法：./consistency_desktop <move_seq.txt>
 *   move_seq.txt：每行一个命令（与原版 OnEvent 协议对齐）：
 *     m <from> <to>     走子/吃子（格子索引 0-59）→ 原版 OnEvent("move","f,t")
 *     f <idx>           翻棋 → OnEvent("flip","idx")
 *     u                 悔棋 → OnEvent("undo","")
 *     q <loser>         认输（loser=认输方 0/1）→ OnEvent("resign","")
 *     L                 重开 → OnEvent("restart","")
 * 输出（可观测面，与原版对拍）：
 *   每行 → "OK <事件名>" / "ILLEGAL"
 *   末行 → "FINAL winner=<w> turn=<t> total=<n> since_eat=<m> stage=<s>"
 * 口径：构造输入下行为一致，非功能等价证明（红线照旧）。
 */
#include "junqi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNDO_MAX 16
static jq_state g_undo[UNDO_MAX];
static int g_undo_top = 0;
static void undo_push(const jq_state *st) { if (g_undo_top < UNDO_MAX) g_undo[g_undo_top++] = *st; }
static int undo_pop(jq_state *st) { if (g_undo_top <= 0) return 0; *st = g_undo[--g_undo_top]; return 1; }

/* 内置基准序列（selftest 用）：触发 翻棋/走子/非法/悔棋/认输 全事件面。
   与 test_move_seq.txt + test_expected.txt 同步。改动需两边一起改。
   ★ 回合校验（rules.c jq_can_move/jq_can_flip）：走子/翻棋限当前回合方（翻棋=走 1 步切回合）。
     故序列为合法交替（蓝 f6 → 红 f33 → 蓝 m6,11 → 红 m33,38），非旧版连翻蓝方。 */
static const char BENCH_SEQ[] =
    "f 6\nf 33\nm 6 11\nm 33 38\nm 12 13\nu\nm 10 11\nq 0\n";
static const char BENCH_EXPECT[] =
    "OK Flip\nOK Flip\nOK Junqi.Move\nOK Junqi.Move\nILLEGAL\nOK Undo\nILLEGAL\nOK Resign\n"
    "FINAL winner=1 turn=1 total=3 since_eat=3 stage=2\n";

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s <move_seq.txt|selftest>\n", argv[0]); return 2; }
    jq_state st;
    jq_load_layout(&st, JQ_DEFAULT_LAY);
    char line[128];
    FILE *f = NULL;
    int selftest = (strcmp(argv[1], "selftest") == 0);
    if (selftest) {
        /* 内置序列：直接喂，输出到 stdout；再与 BENCH_EXPECT 比对 */
        const char *src = BENCH_SEQ; size_t slen = strlen(src); size_t pos = 0;
        /* 把内置序列灌进 line 缓冲逐行处理（复用下方 while） */
        /* 简单实现：写临时文件再读，避免大改控制流 */
        char tmpl[] = "/tmp/jq_bench_XXXXXX";
        int fd = mkstemp(tmpl);
        if (fd >= 0) { write(fd, src, slen); close(fd); f = fopen(tmpl, "r"); remove(tmpl); }
    } else {
        f = fopen(argv[1], "r");
        if (!f) { fprintf(stderr, "open %s fail\n", argv[1]); return 2; }
    }
    if (!f) return 2;
    /* 自测需把输出重定向到内存再比对 */
    char outbuf[4096]; size_t outpos = 0;
    int do_expect_check = selftest;
    while (fgets(line, sizeof line, f) && st.stage != STG_OVER) {
        char cmd[8];
        int a = 0, b = 0;
        if (sscanf(line, "%7s %d %d", cmd, &a, &b) < 1) continue;
        const char *o;
        if (cmd[0] == 'm') {
            undo_push(&st);
            int ev = jq_try_move(&st, a, b);
            if (ev < 0 || ev >= 6) {
                g_undo_top--;
                printf("ILLEGAL\n");
                if (do_expect_check) outpos += snprintf(outbuf+outpos, sizeof outbuf-outpos, "ILLEGAL\n");
            } else {
                o = JQ_EVENT_NAMES[ev];
                printf("OK %s\n", o);
                if (do_expect_check) outpos += snprintf(outbuf+outpos, sizeof outbuf-outpos, "OK %s\n", o);
            }
            continue;
        } else if (cmd[0] == 'f') {
            undo_push(&st);
            int ok = jq_flip(&st, a);
            if (!ok) g_undo_top--;
            o = ok ? "OK Flip" : "ILLEGAL";
            printf("%s\n", o);
            if (do_expect_check) outpos += snprintf(outbuf+outpos, sizeof outbuf-outpos, "%s\n", o);
            continue;
        } else if (cmd[0] == 'u') {
            int ok = undo_pop(&st);
            o = ok ? "OK Undo" : "NOUNDO";
            printf("%s\n", o);
            if (do_expect_check) outpos += snprintf(outbuf+outpos, sizeof outbuf-outpos, "%s\n", o);
            continue;
        } else if (cmd[0] == 'q') {
            st.winner = 1 - a; st.stage = STG_OVER;
            printf("OK Resign\n");
            if (do_expect_check) outpos += snprintf(outbuf+outpos, sizeof outbuf-outpos, "OK Resign\n");
            continue;
        } else if (cmd[0] == 'L') {
            jq_load_layout(&st, JQ_DEFAULT_LAY); g_undo_top = 0;
            printf("OK Restart\n");
            if (do_expect_check) outpos += snprintf(outbuf+outpos, sizeof outbuf-outpos, "OK Restart\n");
            continue;
        }
    }
    fclose(f);
    char finline[128];
    snprintf(finline, sizeof finline, "FINAL winner=%d turn=%d total=%d since_eat=%d stage=%d\n",
             st.winner, st.turn, st.total_moves, st.moves_since_eat, st.stage);
    printf("%s", finline);
    if (do_expect_check) {
        outpos += snprintf(outbuf+outpos, sizeof outbuf-outpos, "%s", finline);
        outbuf[outpos] = 0;
        int same = (strcmp(outbuf, BENCH_EXPECT) == 0);
        printf("\n[selftest] %s 基准序列 9 行逐行一致\n", same ? "PASS" : "FAIL");
        return same ? 0 : 1;
    }
    return 0;
}
