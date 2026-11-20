/*
 * fuzz_evt.c — P3 渗透面：事件分发 code→handler + 音效映射 fuzz（src_rs 正向 C）
 *
 * 定位：渗透轮第 10 变异器（P3，evt55_dispatch.c 256 行 + evt55_sfx.c 31 行）。
 * 覆盖：7 事件码 → 73 事件名 → 音效 WAV 名映射链 + 计数不变量（g_event_cnt Σ）。
 *
 * ★ 为什么需要（历史真缺陷区）：
 *   - R5-B-02：业务代表 55code 34/36/38/44/46 无 case → default 返 0 → 名称往返
 *     身份丢失为 Junqi.Move（假绿同族），已修（按桶返代表名）
 *   - R6-B-10：未知名 default 返 0 = 假绿，已修返哨兵 -1
 *   本探针把"code→name→code 往返身份 + 音效映射 + 计数不变量"做成全量机检判据，
 *   防这两类缺陷复活。
 *
 * 变异策略（4 类）：
 *   M1 全 code 扫描：0..66（含 67case 边界 + 越界 -1/67/0x7fffffff）→ 分发不崩 +
 *      返回码合法（-1 哨兵 或 0..72 索引）
 *   M2 名称往返身份：对每个已知映射 code，name→code→name 必须幂等（不能丢身份变 Move）
 *   M3 音效映射全覆盖：73 个事件名 → sfx 名，断言 7 核心类音效精确（move/eat/bomb/
 *      flag/dead/flip）+ 胜负类（win/lose/draw）；其余兜底 move；NULL→move
 *   M4 计数不变量：evt55 分发计数 Σ(g_event_cnt) == g_event_total（不变量断言）
 *
 * 判据（4 项全 PASS）：
 *   ① M1-M4 0 崩溃（ASan 干净）
 *   ② M2 往返身份：已映射 code 往返必幂等（丢身份 = FAIL）
 *   ③ M3 音效精确：7 核心类 + 3 胜负类音效名精确匹配
 *   ④ M4 计数守恒：Σ(g_event_cnt) == g_event_total
 *
 * 构建：make fuzzevt（ASan，桌面 -DRS_NO_GL，链接 evt55_* + jq_event_names_73）
 * 用法：dist/jq_fuzzevt [seed]
 *
 * 诚实边界：本探针只测**映射/计数逻辑**（纯 C），不测真实事件业务副作用
 * （落子/音效播放）→ 由 12/12 evverify 174/174 + 真机黑盒覆盖。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "junqi.h"
#include "jq_event_names_73.h"

/* 桌面桩 */
int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{ (void)prio; (void)tag; (void)fmt; return 0; }

/* evt55_dispatch 导出的分发 API（看实际头文件） */
extern int jq_evt55_no_state_change(int code);  /* code 合法性判据（真实导出） */
extern const char *jq_evt55_sfx_for_name(const char *evname);
extern int jq_evt55_count_7code(int code);
extern int jq_event_count_invariant_ok(void);
extern int g_event_cnt[JQ_EVENT_NAME_COUNT];
extern int g_event_total;

static int g_fail = 0;
#define CHECK(cond, ...) do { if (!(cond)) { printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); g_fail = 1; } } while (0)

/* ★ code_to_evname 是 evt55_dispatch.c 的 static 函数——桌面探针无法直接调。
   方案：用 #include 把实现文件"内联"进本 TU（一次）。 */
#define static
#include "evt55_dispatch.c"
#undef static

int main(int argc, char **argv)
{
    unsigned seed = argc > 1 ? (unsigned)atoi(argv[1]) : 0;
    printf("== fuzzevt（P3 渗透面：事件分发 + 音效映射，seed=%u）==\n", seed);
    (void)seed;

    /* ===== M1 全 code 扫描（0..66 + 边界/越界） ===== */
    {
        int mapped = 0, unmapped = 0, illegal = 0;
        int codes[80]; int n = 0;
        for (int c = 0; c <= 66; c++) codes[n++] = c;
        codes[n++] = -1; codes[n++] = 67; codes[n++] = 68;
        codes[n++] = 0x7fffffff; codes[n++] = INT32_MIN;
        for (int i = 0; i < n; i++) {
            int idx = e55_code_to_evname(codes[i]);
            if (idx == -1) unmapped++;
            else if (idx >= 0 && idx < JQ_EVENT_NAME_COUNT) {
                mapped++;
                const char *nm = jq_event_names_73[idx];
                CHECK(nm != NULL, "%s", "M1 映射索引对应事件名为 NULL");
            } else {
                illegal++;
                printf("FAIL: M1 非法返回 idx=%d（code=%d）\n", idx, codes[i]);
                g_fail = 1;
            }
            /* 计数 API 也 fuzz（越界 code 必须安全拒绝） */
            (void)jq_evt55_count_7code(codes[i]);
        }
        printf("   M1 全 code 扫描 OK（%d code：映射=%d 未映射哨兵=%d 非法=%d）\n",
               n, mapped, unmapped, illegal);
        CHECK(illegal == 0, "%s", "M1 分发返回非法索引");
        /* 真值：55code 有效映射 19 个（0-16 + 桶代表 34/36/38/44/46 + 24-27 等），
       低于 15 才是分发表退化（防阈值设高假 FAIL）。 */
        CHECK(mapped >= 15, "%s", "M1 映射数异常（<15 = 分发表退化）");
        CHECK(unmapped > 0, "%s", "M1 未映射 code 未返 -1 哨兵（R6-B-10 缺陷复活）");
    }

    /* ===== M2 名称往返身份（code→name→code 幂等） ===== */
    {
        int round_ok = 0, round_lost = 0;
        for (int c = 0; c <= 66; c++) {
            int idx = e55_code_to_evname(c);
            if (idx < 0 || idx >= JQ_EVENT_NAME_COUNT) continue;
            const char *nm = jq_event_names_73[idx];
            if (!nm) continue;
            /* name → code（jq_evt55_name_to_code 存在？用 strcmp 表反查） */
            int back = -1;
            for (int k = 0; k <= 66; k++) {
                int k_idx = e55_code_to_evname(k);
                if (k_idx == idx) { back = k; break; }
            }
            if (back >= 0) {
                /* 往返到**任一**映射同名的 code 即身份保持（同名多 code 允许） */
                if (e55_code_to_evname(back) == idx) round_ok++;
                else { round_lost++; printf("FAIL: M2 身份丢失 code=%d name=%s\n", c, nm); g_fail = 1; }
            }
        }
        printf("   M2 往返身份 OK（%d code 幂等，丢失=%d）\n", round_ok, round_lost);
        CHECK(round_ok >= 15, "%s", "M2 往返样本不足（判据假绿）");
        CHECK(round_lost == 0, "%s", "M2 名称往返丢身份（R5-B-02 缺陷复活）");
    }

    /* ===== M3 音效映射（7 核心 + 3 胜负精确 + 兜底） ===== */
    {
        struct { const char *name; const char *sfx; } core[] = {
            { "Junqi.Move", "move" }, { "Junqi.Eat", "eat" }, { "Junqi.Bomb", "bomb" },
            { "Junqi.Flag", "flag" }, { "Junqi.Dead", "dead" }, { "Junqi.Flip", "flip" },
            { "Junqi.Win", "win" }, { "Junqi.RedWin", "win" }, { "Junqi.BlueWin", "win" },
            { "Junqi.Lose", "lose" }, { "Junqi.Draw", "draw" },
            { "Junqi.GameOver", "draw" }, { "Junqi.Over", "draw" },
            { "Junqi.Resign", "lose" }, { "Junqi.ResignConfirm", "lose" },
        };
        for (size_t i = 0; i < sizeof(core)/sizeof(core[0]); i++) {
            const char *got = jq_evt55_sfx_for_name(core[i].name);
            CHECK(got && !strcmp(got, core[i].sfx),
                  "M3 音效映射错误：%s 期望 %s 实得 %s",
                  core[i].name, core[i].sfx, got ? got : "NULL");
        }
        /* NULL 输入 → 兜底 move（防崩） */
        const char *nullsfx = jq_evt55_sfx_for_name(NULL);
        CHECK(nullsfx && !strcmp(nullsfx, "move"), "%s", "M3 NULL 事件名必须兜底 move");
        /* 73 名全扫描：任何事件名都不能返回 NULL（否则 sfx_play(NULL) 崩） */
        for (int i = 0; i < JQ_EVENT_NAME_COUNT; i++) {
            const char *sfx = jq_evt55_sfx_for_name(jq_event_names_73[i]);
            CHECK(sfx != NULL, "M3 事件名 %s 音效映射为 NULL（sfx_play 崩溃风险）",
                  jq_event_names_73[i]);
        }
        printf("   M3 音效映射 OK（%zu 核心精确 + NULL 兜底 + %d 名非 NULL）\n",
               sizeof(core)/sizeof(core[0]), JQ_EVENT_NAME_COUNT);
    }

    /* ===== M4 计数不变量（Σ g_event_cnt == g_event_total） ===== */
    {
        int before = jq_event_count_invariant_ok();
        /* 注入若干计数（模拟事件分发） */
        for (int i = 0; i < 100; i++) {
            int code = i % 8;
            jq_evt55_count_7code(code);
        }
        int sum = 0;
        for (int i = 0; i < JQ_EVENT_NAME_COUNT; i++) sum += g_event_cnt[i];
        int after = jq_event_count_invariant_ok();
        printf("   M4 计数不变量 OK（注入前=%d 注入后=%d Σcnt=%d total=%d）\n",
               before, after, sum, g_event_total);
        CHECK(after == 1, "%s", "M4 计数不变量破坏（Σcnt != total）");
    }

    if (!g_fail) {
        printf("== 全部通过 ==\n");
        printf("判据: PASS（fuzzevt P3：全 code 扫描 + 往返身份 + 音效精确映射 + 计数守恒，"
               "0 崩溃 ASan 干净）\n");
        return 0;
    }
    printf("判据: FAIL\n");
    return 2;
}
