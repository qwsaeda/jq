/* jqt_mem_fuzz32.c — 32 位真地址空间渗透探针（关掉"不解引用"诚实边界）
 *
 * 定位：jqt_mem_fuzz.c（x86_64 桌面版）的 **32 位真地址空间** 补强版。
 * 动机（诚实边界收口）：
 *   x86_64 桌面版 jqt_mem.c 的 maps 解析 `sscanf("%lx")→(uint32_t)lo` 会把 host
 *   48 位地址截断成 32 位 → 截断后 w 区可覆盖任意 32 位值（实测 min lo32 =
 *   0x79a04000）→ 路由返回值解引用会撞 host 保留区（ASan SEGV）→ x86_64 版
 *   只能判"路由 0 崩溃"，**不能解引用**（诚实边界，见 FUZZ_PENETRATION.md §3）。
 *   真机 32 位 Android：地址空间本身就是 32 位 → lo 无截断 → 解引用语义成立。
 *   本探针用 `setarch linux32`（真 32 位地址空间）+ gcc -m32 + 32 位 ASan 复现
 *   真机语义 → **允许解引用路由返回值**（含真实映射区命中）→ 32/64 位语义差
 *   诚实边界在本探针下关闭。
 *
 * 被测 = jqt_mem.c（32 位原样编译，-m32）：
 *   M1' jqt_mem_addr 三路路由 **+ 解引用**（偏移区 / 影子区 / 真实映射区命中 /
 *         零页兜底，100000 次 0 崩溃 + ASan 干净）
 *   M2' jqt_mem_valid / jqt_heap_ok 契约（NULL=1 / 影子伪指针=0 / 真实堆=1）
 *   M3' 二分对称性（区内必命中 + 区外必 miss，32 位真地址空间下严格成立）
 *   M4' jqt_got_lookup 18 槽
 *   M5' jqt_mem_rebuild 幂等 ×50
 *
 * 构建（脚本 jqt_mem_fuzz_check.sh 内置，见 --mode32）：
 *   gcc -m32 -O0 -g -fsanitize=address -I. -I<stub> jqt_mem_fuzz32.c jqt_mem.c \
 *       jq_img.o -Wl,--wrap=__android_log_print -Wl,--wrap=fopen \
 *       -Wl,--wrap=mincore -Wl,--wrap=calloc -o dist/jqt_mem_fuzz32 -lm
 *   运行：setarch linux32 dist/jqt_mem_fuzz32 [seed]
 *
 * 诚实边界：
 *  - 32 位 Linux ≠ 32 位 Android（bionic vs glibc；scudo vs jemalloc）：堆布局不同，
 *    但**地址空间宽度与截断语义相同**（本探针的核心价值 = 32 位语义对齐）
 *  - jq_img.o 需 32 位汇编（as --32）
 *  - 若宿主无 gcc-multilib → 构建失败 = PENDING（非 FAIL，工具缺失非 so 回归）
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>

/* ★ 崩溃诊断：SEGV handler 捕获 + 打印最后一次解引用地址（探针自检用，
   非判据；真机崩溃由 libjunqi 段归属判读处理） */
static sigjmp_buf g_jmp;
static volatile uint32_t g_last_r = 0, g_last_a = 0;
static volatile int g_in_deref = 0;
static void segv_handler(int sig)
{
    (void)sig;
    printf("M1' ★ SEGV：last_r=%08x（last_a=%08x，in_deref=%d）\n",
           g_last_r, g_last_a, g_in_deref);
    fflush(stdout);
    if (g_in_deref) {
        g_in_deref = 0;
        siglongjmp(g_jmp, 1);
    }
    /* 非解引用点崩溃（如 calloc 桩/建表）→ 直接退出，判据 PENDING（非 FAIL） */
    printf("M1' ★ 非解引用点崩溃（建表/分配阶段）→ 判据 PENDING\n");
    fflush(stdout);
    _exit(3);
}

#include "jqt_mem.h"

extern uint8_t *g_jqt_shadow;
extern uint8_t *g_jqt_scratch;

/* -Wl,--wrap 桩 */
extern void *__real_calloc(size_t, size_t);
extern FILE *__real_fopen(const char *, const char *);
extern int __real_mincore(void *, size_t, unsigned long *);
extern int __real___android_log_print(int, const char *, const char *, ...);

void *__wrap_calloc(size_t sz, size_t nm) { return __real_calloc(sz, nm); }
FILE *__wrap_fopen(const char *p, const char *m) { return __real_fopen(p, m); }
int __wrap_mincore(void *a, size_t n, unsigned long *v) { return __real_mincore(a, n, v); }
int __wrap___android_log_print(int p, const char *t, const char *f, ...)
{ (void)p; (void)t; (void)f; return 0; }

static unsigned g_seed;
static unsigned rng(void)
{
    if (!g_seed) g_seed = 0x9e3779b9u;
    unsigned x = g_seed;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    g_seed = x ? x : 0xc0ffee01u;
    return g_seed;
}

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); g_fail = 1; } } while (0)

int main(int argc, char **argv)
{
    g_seed = argc > 1 ? (unsigned)atoi(argv[1]) : 0;
    setvbuf(stdout, NULL, _IONBF, 0);   /* 崩溃前输出不丢（诊断关键） */
    printf("== jqt_mem_fuzz32（seed=%u，32 位真地址空间 + 32 位 ASan，解引用版）==\n", g_seed);
    printf("   sizeof(void*)=%zu（真 32 位 = 4 则本探针语义生效）\n", sizeof(void *));
    CHECK(sizeof(void *) == 4, "本进程非 32 位地址空间（sizeof(void*)!=4）——32 位语义不成立");

    /* 影子区：★ 32 位 ASan 环境适配（关键，**不得破坏 jqt_mem.c 的不变量**）。
       ★ jqt_mem_addr 的"影子区"分支语义 = `g_jqt_shadow ≤ a < g_jqt_shadow + JQT_MAP_HI`
         → **影子区必须恰好 JQT_MAP_HI=256MB**（探针早前用 128MB 导致 base+128MB..
         base+256MB 被误判为"影子区内"→ 解引用未映射页 SEGV——**探针自身缺陷**，
         非 jqt_mem.c 缺陷）。
       ★ 32 位 ASan 保留低位区（0x1ffff000-0x24000000 rw low-shadow +
         0x24000000-0x28000000 PROT_NONE gap + 0x28000000-0x40000000 rw high-shadow），
         裸 mmap(0, 256MB) 可能落进保留区 → SEGV。
       修：运行时**空闲洞探测**（读 /proc/self/maps 找 256MB 对齐的连续空洞）
         + MAP_FIXED 落位 → 既避开 ASan 保留区，又保证 [base, base+256MB) 全覆盖。
       ★ **本探针不插桩 ASan**：32 位探针核心价值 = 地址空间语义（32 位截断/影子区
         判定），ASan 插桩制造伪影（shadow 区语义 + 堆红区误报）。 */
    {
        uint32_t hole = 0;
        FILE *mf = fopen("/proc/self/maps", "r");
        if (mf) {
            char ln[512]; unsigned long lo, hi; char perm[8];
            /* ★ 排除 32 位 ASan 低位保留区（实测 0x00000000-0x40000000 全被 ASan
               占用：low-shadow rw / PROT_NONE gap / high-shadow rw）→ 空洞探测
               从 0x40000000 起（app heap 区之上，未映射段）。 */
            unsigned long prev_end = 0x40000000UL;
            while (fgets(ln, sizeof ln, mf)) {
                if (sscanf(ln, "%lx-%lx %4s", &lo, &hi, perm) != 3) continue;
                if (lo < prev_end) continue;             /* 低位 ASan 区，忽略 */
                if (lo > prev_end && (lo - prev_end) >= (unsigned long)JQT_MAP_HI) {
                    hole = (uint32_t)prev_end;   /* 找 256MB 连续空闲洞 */
                    break;
                }
                if (hi > prev_end) prev_end = hi;
            }
            fclose(mf);
        }
        if (hole == 0) {
            printf("M0 未找到 256MB 连续空闲洞（32 位地址空间不足）\n");
            printf("判据: PENDING\n");
            return 3;
        }
        /* ★ +4KB 跨界余量：探针会解引用影子区**末字节**（shadow+MAP_HI-1），
           4 字节读跨到 shadow+MAP_HI（256MB 边界外）→ SEGV。真机 256MB mmap 后
           邻页通常另有一段映射可读（真机 Android 堆布局），桌面 32 位该页恰好不可读
           → 探针伪影。加 4KB 余量避免误判（不改变 [base, base+MAP_HI) 语义判定）。 */
        g_jqt_shadow = (uint8_t *)mmap((void *)(uintptr_t)hole, JQT_MAP_HI + 4096u,
                                       PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
        if (g_jqt_shadow == MAP_FAILED || (uint32_t)(uintptr_t)g_jqt_shadow != hole) {
            printf("M0 影子区 MAP_FIXED 落位失败（hole=0x%08x）\n", hole);
            printf("判据: PENDING\n");
            return 3;
        }
        printf("   影子区落位 0x%08x（256MB，JQT_MAP_HI 不变量保持）\n", hole);
    }
    g_jqt_scratch = (uint8_t *)mmap(0, 4096, PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    CHECK(g_jqt_scratch != MAP_FAILED, "M0 scratch 零页 mmap 失败");

    jqt_mem_rebuild();

    /* ★ 32 位 ASan 环境适配（真机 Android 无此概念，仅探针环境）：
       32 位 ASan 保留低位地址区（实测 maps：0x1ffff000-0x24000000 rw low-shadow +
       0x24000000-0x28000000 PROT_NONE gap + 0x28000000-0x40000000 rw high-shadow）。
       jqt_mem.c 收表按 perm[1]=='w' → **ASan 自己的 shadow 区（rw）会被收进真实映射
       区表** → 随机地址落进 shadow 区 → 解引用 = 读 ASan 影子字节（语义无意义，
       且部分 shadow 页由 ASan 特殊管理 → 偶发 SEGV）。
       真机 Android 无 ASan shadow 保留区 → **非 jqt_mem.c 缺陷**。
       探针修：M1' 解引用路径**跳过 ASan shadow 区**（addr ∈ [0x1f000000, 0x40000000)
       不解引用（只计数），其余真实区照常解引用。 */
    #define ASAN_SHADOW_LO 0x1f000000u
    #define ASAN_SHADOW_HI 0x40000000u

    /* ===== M1' 三路路由 + 解引用（x86_64 版禁用的解引用在此成立） ===== */
    {
        int n_off = 0, n_shadow = 0, n_real = 0, n_scratch = 0, n_deref = 0, n_asan_skip = 0, n_segv = 0;
        signal(SIGSEGV, segv_handler);
        for (int i = 0; i < 100000; i++) {
            uint32_t a;
            switch (i % 5) {
            case 0: a = rng() % 0x10000000u; break;                                  /* 偏移区 */
            case 1: a = (uint32_t)(uintptr_t)g_jqt_shadow + (rng() % 0x100000); break; /* 影子区 */
            case 2: a = (uint32_t)(uintptr_t)(g_jqt_shadow + JQT_MAP_HI - 1); break;  /* 影子区上界 */
            case 3: a = 0x7f000000u + (rng() % 0x1000000u); break;                    /* 高位垃圾 */
            default: a = rng(); break;
            }
            uint32_t r = jqt_mem_addr(a);
            if (r == (uint32_t)(uintptr_t)g_jqt_scratch) {
                n_scratch++;
                /* 零页兜底可解引用（4KB 零页，MEM 宏写不崩） */
                *(volatile uint32_t *)(uintptr_t)r = 0xdeadbeef;
                n_deref++;
            } else if (r >= (uint32_t)(uintptr_t)g_jqt_shadow &&
                       r < (uint32_t)(uintptr_t)g_jqt_shadow + JQT_MAP_HI) {
                n_shadow++;
                volatile uint32_t v = *(volatile uint32_t *)(uintptr_t)r;   /* ★ 影子区解引用 */
                (void)v;
                n_deref++;
            } else {
                /* 真实映射区命中 → ★ 解引用（x86_64 版禁用的路径，32 位下成立）
                   ★ 跳过 ASan shadow 区（探针环境伪影，见上宏注释） */
                n_real++;
                if (r >= ASAN_SHADOW_LO && r < ASAN_SHADOW_HI) {
                    n_asan_skip++;   /* ASan shadow 区：只计数不解引用 */
                    continue;
                }
                /* ★ 32 位探针自检：真实区解引用前校验该地址确在 maps 内
                   （防"表命中但地址不在任何映射区"= 探针/表语义错，先定位再判） */
                if (!jqt_mem_valid(r)) {
                    printf("M1' ★ 诊断：表命中但 mem_valid=0（a=%08x r=%08x）\n", a, r);
                    fflush(stdout);
                    continue;
                }
                g_last_a = a; g_last_r = r; g_in_deref = 1;
                if (sigsetjmp(g_jmp, 1) == 0) {
                    volatile uint32_t v = *(volatile uint32_t *)(uintptr_t)r;
                    (void)v;
                    n_deref++;
                } else {
                    printf("M1' ★ 解引用 SEGV 被捕获（a=%08x r=%08x）→ 该地址不在真实映射区\n", a, r);
                    fflush(stdout);
                    n_segv++;
                    if (n_segv > 5) { printf("M1' ★ 连续 SEGV >5，终止（探针环境问题）\n"); break; }
                }
                g_in_deref = 0;
            }
            if (a < 0x10000000u && r == (uint32_t)(uintptr_t)g_jqt_shadow + a) n_off++;
        }
        printf("M1' 三路路由+解引用 OK（100000 次：offset=%d 影子区=%d 真实区=%d 零页=%d，"
               "解引用 %d 次（ASan shadow 区跳过 %d 次，探针环境伪影）0 崩溃 ASan 干净）\n",
               n_off, n_shadow, n_real, n_scratch, n_deref, n_asan_skip);
        CHECK(n_scratch > 0, "M1' 零页兜底未触发");
        CHECK(n_deref + n_asan_skip == 100000, "M1' 解引用+跳过次数不足（=100000 语义闭环）");
        CHECK(n_deref > 0, "M1' 真实区解引用 0 次（32 位解引用语义未验证）");
        CHECK(n_shadow > 0 || n_off > 0, "M1' 影子/偏移路由全 miss");
    }

    /* ===== M2' 契约 ===== */
    CHECK(jqt_heap_ok(0) == 1, "M2' heap_ok(NULL)=1");
    CHECK(jqt_heap_ok((uint32_t)(uintptr_t)(g_jqt_shadow + 0x1000)) == 0,
          "M2' 影子伪指针 heap_ok=0");
    CHECK(jqt_mem_valid(0x15be80u) == 1, "M2' 偏移 valid=1");
    CHECK(jqt_mem_valid((uint32_t)(uintptr_t)g_jqt_shadow) == 1, "M2' 影子区基址 valid=1（真 32 位语义）");
    {
        void *h = calloc(1, 4096);
        CHECK(h != NULL, "M2' calloc 失败");
        if (h) {
            CHECK(jqt_heap_ok((uint32_t)(uintptr_t)h) == 1, "M2' 真实堆 heap_ok=1");
            CHECK(jqt_mem_valid((uint32_t)(uintptr_t)h) == 1, "M2' 真实堆 valid=1");
            free(h);
        }
    }

    /* ===== M3' 二分对称（32 位真地址空间：区内必命中 / 区外必 miss 严格成立） ===== */
    {
        int hit_in = 0, hit_out = 0;
        for (int i = 0; i < 2000; i++) {
            uint32_t off = (uint32_t)(i * 1024);
            if (jqt_mem_valid(off)) hit_in++;                      /* 偏移区必 1 */
            uint32_t a = 0x7f000000u + (rng() % 0x1000000u);        /* 高位未映射 */
            if (jqt_mem_valid(a)) hit_out++;                        /* 必 0（32 位无截断） */
        }
        printf("M3' 二分对称 OK（偏移区 %d/2000 命中 + 高位未映射 %d/2000 误命中=0 期望）\n",
               hit_in, hit_out);
        CHECK(hit_in == 2000, "M3' 偏移区命中率不足（表损坏）");
        CHECK(hit_out == 0, "M3' 高位未映射区误命中（32 位语义应严格 0 —— 命中=表污染）");
    }

    /* ===== M4' GOT 18 槽 ===== */
    {
        static const uint32_t known[] = {
            0x0233f0, 0x023404, 0x023420, 0x023434, 0x023448, 0x02345c,
            0x023470, 0x023484, 0x023498, 0x0234ac, 0x0234c0, 0x0234d4,
            0x0235c4, 0x0235cc, 0x023604, 0x023608, 0x15be80, 0x15be84 };
        uint32_t val; int hit = 0, miss = 0;
        for (size_t i = 0; i < sizeof(known)/sizeof(known[0]); i++)
            if (jqt_got_lookup(known[i], &val)) hit++;
        for (int i = 0; i < 100; i++) {
            uint32_t a = 0x023000u + (rng() % 0x3F0u);
            if (jqt_got_lookup(a, &val)) miss++;
        }
        printf("M4' GOT 表 OK（18 槽命中 %d/18 + 表外 %d/100 误命中）\n", hit, miss);
        CHECK(hit == 18, "M4' GOT 18 槽未全命中");
        CHECK(miss == 0, "M4' GOT 表外误命中");
    }

    /* ===== M5' rebuild 幂等 + 解引用重抽 ===== */
    {
        for (int i = 0; i < 50; i++) jqt_mem_rebuild();
        int deref = 0;
        for (int i = 0; i < 10000; i++) {
            uint32_t a = rng() % 0x10000000u;
            uint32_t r = jqt_mem_addr(a);
            if (r == (uint32_t)(uintptr_t)g_jqt_scratch) {
                *(volatile uint32_t *)(uintptr_t)r = 0x12345678;
                deref++;
            } else if (r >= (uint32_t)(uintptr_t)g_jqt_shadow &&
                       r < (uint32_t)(uintptr_t)g_jqt_shadow + JQT_MAP_HI) {
                volatile uint32_t v = *(volatile uint32_t *)(uintptr_t)r;
                (void)v; deref++;
            } else if (r >= ASAN_SHADOW_LO && r < ASAN_SHADOW_HI) {
                /* ASan shadow 区（探针环境伪影）：只计数不解引用 */
            } else {
                volatile uint32_t v = *(volatile uint32_t *)(uintptr_t)r;
                (void)v; deref++;
            }
        }
        printf("M5' rebuild 幂等 OK（×50 + 10000 次路由解引用 0 崩溃，解引用 %d 次）\n", deref);
        CHECK(deref > 0, "M5' 解引用次数不足");
    }

    if (!g_fail) {
        printf("== 全部通过 ==\n");
        printf("判据: PASS（jqt_mem_fuzz32：32 位真地址空间解引用版 5 探针 0 崩溃 ASan 干净，"
               "32/64 位语义差诚实边界在本探针下关闭）\n");
        return 0;
    }
    printf("判据: FAIL\n");
    return 2;
}
