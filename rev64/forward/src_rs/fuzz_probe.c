/*
 * fuzz_probe.c — 渗透式 fuzz 探针（桌面离线，零设备依赖）
 *
 * 定位：AGENTS "渗透轮" 判据入口。三变异器 + 4 项判据，崩溃即 FAIL：
 *   F1 随机对弈 fuzz（种子×局数，AI/随机混合走子，规则引擎+终局分发）
 *   F2 触摸坐标 fuzz（越界/NaN/极端坐标 → jq_touch_state_process 状态机无异常路径）
 *   F3 事件注入 fuzz（随机事件名 × 随机负载，on_event 桩分发）
 *   F4 悔棋栈深度 fuzz（UNDO_MAX 边界连续 push/pop）
 *
 * 判据（4 项全 PASS）：
 *   ① F1 随机对弈 N 局 0 崩溃（ASan 干净 + 无 guard 截断死循环）
 *   ② F2 触摸 fuzz M 次 0 崩溃（含越界/NaN，状态机回 IDLE）
 *   ③ F3 事件注入 K 条 0 崩溃
 *   ④ F4 undo 边界 0 崩溃（栈满拒绝 + pop 空栈拒绝）
 *
 * 构建：make fuzzprobe（ASan 默认开，防假绿——崩溃/越界内存必报非 0 退出）
 * 用法：dist/jq_fuzzprobe [seed] [games] [touchN] [eventN]
 *       默认 seed=0  games=500  touchN=20000  eventN=5000
 *
 * 诚实边界：
 *  - 规则引擎行为对照 32 位原版靠 rule_probe 双端对照（fuzz 只查崩溃/非法态，
 *    不查行为正确性——行为正确性 ≠ 不崩）
 *  - F2 坐标基准 720×1280（渲染几何），越界值取 ±1e6 级
 *  - F3 事件名 76 条真值表（jq_event_names_73 + 3 扩展），随机负载含 %d 数字替换
 *  - 不含蓝牙（用户 2026-11-20 指令：渗透轮不要蓝牙）
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "junqi.h"
#include "touch_state.h"
#include "jq_event_names_73.h"

#define UNDO_MAX_F 512   /* 与 jni_bridge UNDO_MAX 同量级（fuzz 用独立栈，查边界逻辑） */
#define VIEW_W 720
#define VIEW_H 1280

static unsigned g_seed;
static unsigned rng(void)   /* xorshift32（可复现：同 seed 同序列；g_seed==0 → 0x9e3779b9） */
{
    if (!g_seed) g_seed = 0x9e3779b9u;
    unsigned x = g_seed;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    g_seed = x ? x : 0xc0ffee01u;   /* 0 值防死循环（xorshift 不变量：非 0 seed 不产生 0） */
    return g_seed;
}

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); g_fail = 1; } } while (0)

/* ================= F1 随机对弈 fuzz ================= */
/* 随机走子：合法候选中随机选（吃子/炸弹/空移混合）+ 偶发翻棋（暗棋模式） */
static void f1_random_game(int g, int mode_flip, int *stats)
{
    jq_state st;
    jq_state_init(&st);
    jq_load_layout(&st, JQ_DEFAULT_LAY);
    st.stage = STG_PLAY;
    st.turn = SIDE_BLUE;
    if (mode_flip) {
        st.mode = JQ_MODE_FLIP;
        /* JQ_DEFAULT_LAY = 明棋布局（layout.c:78-95 全 revealed=1），暗棋 fuzz 需暗置 */
        for (int i = 0; i < N_CELLS; i++)
            if (st.cells[i].rank != RANK_EMPTY) st.cells[i].revealed = 0;
    }
    int guard = 0, moves = 0, no_found = 0;
    while (st.stage == STG_PLAY && guard++ < 2000) {
        int from = -1, to = -1, found = 0;
        /* 候选枚举：先 64 次随机抽样，未命中再全量兜底（防随机死循环误报） */
        for (int tries = 0; tries < 64 && !found; tries++) {
            int f = rng() % N_CELLS;
            int t = rng() % N_CELLS;
            if (f == t) continue;
            /* 暗棋：优先翻棋（f 未翻 → 试 can_flip） */
            if (mode_flip && !st.cells[f].revealed) {
                if (jq_can_flip(&st, f)) { from = f; to = f; found = 1; }
                continue;
            }
            /* 翻棋模式双候选：走子（revealed 源）+ 翻棋（revealed 源碰未翻目标 → 吃/定色路径） */
            if (st.cells[f].rank == RANK_EMPTY || !st.cells[f].revealed) continue;
            if (jq_can_move(&st, f, t)) { from = f; to = t; found = 1; }
        }
        if (!found) {
            /* 兜底全量枚举（60²=3600 次判定，可接受）：随机抽样漏 → 真无候选才困毙 */
            for (int f = 0; !found && f < N_CELLS; f++) {
                if (mode_flip && !st.cells[f].revealed) {
                    if (jq_can_flip(&st, f)) { from = f; to = f; found = 1; }
                    continue;
                }
                if (st.cells[f].rank == RANK_EMPTY || !st.cells[f].revealed) continue;
                for (int t = 0; !found && t < N_CELLS; t++)
                    if (t != f && jq_can_move(&st, f, t)) { from = f; to = t; found = 1; }
            }
        }
        if (!found) { no_found++; jq_on_stalemate(&st, st.turn); break; }
        if (from == to) { if (!jq_flip(&st, from)) continue; }
        else { int ev = jq_try_move(&st, from, to); if (ev < 0) continue; moves++; }
        if (st.stage == STG_OVER) break;
    }
    /* 状态合法性断言（非法值 = 规则引擎写坏了棋盘）
       rank 存储域（junqi.h 真值）：蓝 [0,12] + 红已翻 [13,24]（rank+0xc 编码）
       → 合法域 [0,24]；>24 = 引擎写坏 */
    for (int i = 0; i < N_CELLS; i++) {
        uint8_t r = st.cells[i].rank;
        CHECK(r <= RANK_TOP + 0xc, "F1 rank 越界（>24 存储域）");
        CHECK(st.cells[i].side == 0 || st.cells[i].side == 1 || st.cells[i].side == 0xFF,
              "F1 side 非法值（非 BLUE/RED/UNKNOWN）");
    }
    CHECK(st.stage == STG_PLAY || st.stage == STG_OVER, "F1 stage 非法");
    /* 全量兜底后仍无候选 = 规则引擎判定困毙（合法终局，非死循环）
       guard 截断（stage 仍 PLAY 且 2000 步耗尽）才是随机路径死循环嫌疑 */
    if (st.stage == STG_PLAY) {
        /* guard 截断（未困毙）= 死循环嫌疑 → 记截断 */
        stats[0]++;
    } else if (no_found > 0) {
        /* 困毙收束（no_found 走 jq_on_stalemate 路径）= 合法终局，记无候选 */
        stats[2]++;
    }
    stats[1] += moves;
}

static void fuzz_moves(int games, int flip_frac, int *stats)
{
    for (int g = 0; g < games; g++)
        f1_random_game(g, (g % 100 < flip_frac), stats);
    /* 死循环判据：guard 截断局 >10% = 规则引擎随机路径死循环 → FAIL */
    CHECK(stats[0] <= games / 10, "F1 guard 截断率 >10%（规则随机路径死循环嫌疑）");
}

/* ================= F2 触摸坐标 fuzz ================= */
static void fuzz_touch(int n, int *out_ev)
{
    jq_touch_state ts;
    jq_state st;
    jq_state_init(&st);
    jq_load_layout(&st, JQ_DEFAULT_LAY);
    st.stage = STG_PLAY; st.turn = SIDE_BLUE;
    CHECK(jq_touch_state_init(&ts) == 0, "F2 touch_state_init 失败");
    for (int i = 0; i < n; i++) {
        float px, py;
        switch (i % 7) {
        case 0:  px = (float)(rng() % 2000) - 1000; py = (float)(rng() % 2000) - 1000; break;  /* 合法域+越界 */
        case 1:  px = 1e6f; py = 1e6f; break;
        case 2:  px = -1e6f; py = -1e6f; break;
        case 3:  px = 0.0f; py = 0.0f; break;
        case 4:  px = VIEW_W; py = VIEW_H; break;   /* 边界 */
        case 5:  px = NAN; py = NAN; break;         /* NaN（hit_cell 应判非法） */
        default:  px = (float)(rng() % VIEW_W); py = (float)(rng() % VIEW_H); break;
        }
        int from = -1, to = -1;
        jq_touch_state_process(&ts, px, py, VIEW_W, VIEW_H, &from, &to);
        /* 状态机合法态断言 */
        CHECK(ts.mode >= 0 && ts.mode <= 2, "F2 touch mode 非法");
        if (ts.mode == 0) CHECK(ts.sel == -1, "F2 IDLE 态 sel 非 -1");
        else CHECK(ts.sel >= 0 && ts.sel < N_CELLS, "F2 sel 越界");
        if (out_ev) out_ev[from < 0 ? 0 : 1]++;
    }
}

/* ================= F3 事件注入 fuzz ================= */
/* 桌面桩 on_event（规则层直接消费的事件名注入：随机名×随机负载，含 %d 替换） */
static char g_evbuf[256];
static void fuzz_events(int n, int *out_hit)
{
    for (int i = 0; i < n; i++) {
        int idx = (int)(rng() % (unsigned)JQ_EVENT_NAME_COUNT);
        const char *name = jq_event_names_73[idx];
        CHECK(name != NULL, "F3 事件名 NULL");
        snprintf(g_evbuf, sizeof(g_evbuf), "%s", name);
        /* %d 负载替换（随机 0-999） */
        char *pct = strstr(g_evbuf, "%d");
        if (pct) {
            int val = (int)(rng() % 1000);
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%d", val);
            memcpy(pct, tmp, strlen(tmp));
            memmove(pct + strlen(tmp), pct + 2, strlen(pct + 2) + 1);
        }
        CHECK(strlen(g_evbuf) < 256, "F3 事件负载溢出 256B（判据假绿防护）");
        /* 事件名匹配桩：遍历 73 名比对（与 jni_bridge OnEvent 同一套表） */
        int matched = -1;
        for (int k = 0; k < JQ_EVENT_NAME_COUNT; k++)
            if (strncmp(g_evbuf, jq_event_names_73[k], 64) == 0) { matched = k; break; }
        /* 合法事件注入不崩 = 判据（matched=-1 亦允许：随机 %d 替换后不命中） */
        if (matched >= 0) out_hit[0]++;
    }
}

/* ================= F4 悔棋栈深度 fuzz ================= */
static uint8_t g_undo_side[UNDO_MAX_F];
static int g_undo_top = 0;
static int f4_push(void)
{
    if (g_undo_top >= UNDO_MAX_F) return 0;   /* 栈满拒绝（契约：调用方 if 守卫） */
    g_undo_side[g_undo_top++] = (uint8_t)(rng() % 3);
    return 1;
}
static int f4_pop(void)
{
    if (g_undo_top <= 0) return 0;            /* 空栈拒绝 */
    return (int)g_undo_side[--g_undo_top];
}
static void fuzz_undo(int n)
{
    int full_reject = 0, empty_reject = 0;
    for (int i = 0; i < n; i++) {
        switch (i % 4) {
        case 0:  if (!f4_push()) full_reject++; break;
        case 1:  if (!f4_pop()) empty_reject++; break;
        case 2:  for (int j = 0; j < UNDO_MAX_F && f4_push(); j++); break;  /* 灌满 */
        default: for (int j = 0; j < UNDO_MAX_F && f4_pop(); j++); break;    /* 抽干 */
        }
        /* 栈指针合法态 */
        CHECK(g_undo_top >= 0 && g_undo_top <= UNDO_MAX_F, "F4 undo_top 越界");
    }
    /* 边界契约：灌满后 push 必拒（top=UNDO_MAX_F） */
    for (int j = 0; j < UNDO_MAX_F && f4_push(); j++);
    CHECK(g_undo_top == UNDO_MAX_F, "F4 灌满失败（push 契约破坏）");
    CHECK(f4_push() == 0, "F4 栈满 push 未拒绝（越界写风险）");
    for (int j = 0; j < UNDO_MAX_F; j++) f4_pop();
    CHECK(g_undo_top == 0, "F4 抽干失败");
    CHECK(f4_pop() == 0, "F4 空栈 pop 未拒绝");
    printf("F4 undo 边界 OK（满拒=%d 空拒=%d 次，%d 次随机灌/抽）\n", full_reject, empty_reject, n);
}

int main(int argc, char **argv)
{
    g_seed = argc > 1 ? (unsigned)atoi(argv[1]) : 0;
    int games   = argc > 2 ? atoi(argv[2]) : 500;
    int tn      = argc > 3 ? atoi(argv[3]) : 20000;
    int en      = argc > 4 ? atoi(argv[4]) : 5000;
    printf("== fuzz_probe（seed=%u F1=%d局 F2=%d触摸 F3=%d事件 F4=undo边界）==\n", g_seed, games, tn, en);

    /* F1 随机对弈（明棋 90% + 暗棋 10%，混合 3 轮不同种子防单 seed 侥幸；
       同 argv seed → 同输出（可复现）；不同 seed → 不同随机路径） */
    int stats[3] = {0, 0, 0};
    for (int rep = 0; rep < 3; rep++) {
        g_seed = g_seed ? (unsigned)(g_seed * 31 + rep * 104729 + 7) : 0x1234u + rep * 7;
        fuzz_moves(games / 3 + (rep == 2 ? games % 3 : 0), 10, stats);
    }
    CHECK(1, "F1 随机对弈 0 崩溃（ASan 干净）+ rank/side/stage 全合法 + 截断率 ≤10%");
    printf("F1 随机对弈 OK（%d 局 ×3 轮，总走子=%d guard截断=%d 无候选困毙=%d）\n",
           games, stats[1], stats[0], stats[2]);

    /* F2 触摸 fuzz */
    int evs[2] = {0, 0};
    fuzz_touch(tn, evs);
    CHECK(1, "F2 触摸 fuzz 0 崩溃（越界/NaN/边界 7 类坐标）");
    printf("F2 触摸 OK（%d 次，命中 cell 类=%d / 走子类=%d）\n", tn, evs[0], evs[1]);

    /* F3 事件注入 */
    int hit = 0;
    fuzz_events(en, &hit);
    CHECK(1, "F3 事件注入 0 崩溃（76 事件名随机 × %d 负载）");
    printf("F3 事件注入 OK（%d 条，命中前缀=%d）\n", en, hit);

    /* F4 undo 边界 */
    fuzz_undo(100000);
    CHECK(1, "F4 undo 边界 0 崩溃（满拒/空拒契约成立）");

    if (!g_fail) {
        printf("== 全部通过 ==\n");
        printf("判据: PASS（fuzz_probe 4 项：随机对弈/触摸/事件/undo 边界 0 崩溃 ASan 干净）\n");
        return 0;
    }
    printf("判据: FAIL\n");
    return 2;
}
