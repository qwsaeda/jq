/* jq_rt.c — 影子地址空间运行时：内存映射、分配器、间接调用分发、init_array 启动 */
#include "jq_shadow.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifdef _WIN32
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

uint8_t *g_jq_base = 0;
__thread uint32_t g_jq_ret1, g_jq_ret2, g_jq_ret3;

#define IMAGE_END   0x162e80u
#define STACK_TOP   0x1f0000u
#define STACK_GUARD 0x00010000u

#define HEAP_LO     0x02000000u
#define HEAP_HI     0x08000000u

#define IMP_BASE    0x0b000000u
#define ENV_TOKEN   0x0c000000u
#define ENV_TABLE   0x0c001000u
#define THUNK_BASE  0x0d000000u
#define OBJ_BASE    0x0e000000u
#define MID_BASE    0x0e800000u
#define FID_BASE    0x0ea00000u
#define HOST_BASE   0x0f000000u

#define MAP_LO      0x00000000u
/* MAP_HI 已定义于 jq_shadow.h（256MB 影子地址空间） */

/* ---------------- 日志 ---------------- */
static int g_verbose = 1;

void jq_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
#ifdef __ANDROID__
    __android_log_vprint(ANDROID_LOG_ERROR, "jq", fmt, ap);
#else
    vfprintf(stderr, fmt, ap);
#endif
    va_end(ap);
}

/* ---------------- 内存布局 ---------------- */
uint8_t *g_jq_scratch = 0;

void jq_mem_init(void) {
    if (g_jq_base) return;
#ifdef _WIN32
    g_jq_base = (uint8_t *)calloc(1, MAP_HI);
    g_jq_scratch = (uint8_t *)calloc(1, 0x1000u);
#else
    void *p = mmap(0, MAP_HI, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) { perror("mmap shadow"); abort(); }
    g_jq_base = (uint8_t *)p;
    g_jq_scratch = (uint8_t *)mmap(0, 0x1000u, PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (g_jq_scratch == MAP_FAILED) g_jq_scratch = (uint8_t *)calloc(1, 0x1000u);
    memset(g_jq_scratch, 0, 0x1000u);
#endif
    /* 镜像区清零（bss 段自然为零） */
    memset(g_jq_base, 0, IMAGE_END);
    /* 嵌入的镜像切片 */
    memcpy(g_jq_base + JQ_IMG_BLOB_VA, jq_img_blob,
           (size_t)(jq_img_blob_end - jq_img_blob));
    /* .got 的 214 个 JUMP_SLOT 填 import 令牌（静态文件里是 PLT0 地址） */
    for (int i = 0; i < jq_slot_imp_n; i++) {
        MEM(jq_slot_imp[i].slot) = IMP_BASE + jq_slot_imp[i].imp * 4u;
    }
}

/* ---------------- 影子堆 ---------------- */
/* 块头 8 字节：size(含头,8 对齐) + next_free。自由块串成链表。 */
#define BLK_SIZE(a)  MEM(a)
#define BLK_NEXT(a)  MEM((a) + 4)
static uint32_t g_heap_bump = HEAP_LO;
static uint32_t g_free_head = 0;
static uint32_t g_heap_used = 0;

static uint32_t blk_alloc(uint32_t n) {
    n = (n + 7u) & ~7u;
    uint32_t need = n + 8u;
    /* 自由链表 first-fit */
    uint32_t prev = 0, cur = g_free_head;
    while (cur) {
        uint32_t sz = BLK_SIZE(cur);
        if (sz >= need) {
            uint32_t nxt = BLK_NEXT(cur);
            if (sz - need >= 24u) {
                uint32_t rest = cur + need;
                BLK_SIZE(rest) = sz - need;
                BLK_NEXT(rest) = nxt;
                if (prev) BLK_NEXT(prev) = rest; else g_free_head = rest;
                BLK_SIZE(cur) = need;
            } else {
                if (prev) BLK_NEXT(prev) = nxt; else g_free_head = nxt;
            }
            BLK_NEXT(cur) = 0;
            g_heap_used += BLK_SIZE(cur);
            return cur + 8u;
        }
        prev = cur;
        cur = BLK_NEXT(cur);
    }
    if (g_heap_bump + need > HEAP_HI) {
        jq_log("[jq] 影子堆耗尽 (%u bytes used)\n", g_heap_used);
        return 0;
    }
    uint32_t b = g_heap_bump;
    g_heap_bump += need;
    BLK_SIZE(b) = need;
    BLK_NEXT(b) = 0;
    g_heap_used += need;
    return b + 8u;
}

uint32_t jq_malloc(uint32_t n) {
    if (n == 0) n = 1;
    return blk_alloc(n);
}

void jq_free(uint32_t p) {
    if (!p) return;
    if (p < HEAP_LO || p >= HEAP_HI) {
        jq_log("[jq] free 非影子堆指针 0x%x\n", p);
        return;
    }
    uint32_t b = p - 8u;
    g_heap_used -= BLK_SIZE(b);
    BLK_NEXT(b) = g_free_head;
    g_free_head = b;
}

uint32_t jq_realloc(uint32_t p, uint32_t n) {
    if (!p) return jq_malloc(n);
    if (!n) { jq_free(p); return 0; }
    uint32_t old = BLK_SIZE(p - 8u) - 8u;
    if (old >= n) return p;
    uint32_t q = jq_malloc(n);
    if (!q) return 0;
    memcpy(g_jq_base + q, g_jq_base + p, old);
    jq_free(p);
    return q;
}

uint32_t jq_calloc(uint32_t nmemb, uint32_t size) {
    uint32_t n = nmemb * size;
    uint32_t p = jq_malloc(n);
    if (p) memset(g_jq_base + p, 0, n);
    return p;
}

/* ---------------- 宿主指针句柄 ---------------- */
static uint32_t g_host_n = 0;
static void *g_host_ptr[4096];

uint32_t jq_handle(void *host) {
    if (!host) return 0;
    for (uint32_t i = 0; i < g_host_n; i++)
        if (g_host_ptr[i] == host) return HOST_BASE + i * 4u;
    if (g_host_n >= 4096) { jq_log("[jq] 宿主句柄表满\n"); return 0; }
    g_host_ptr[g_host_n] = host;
    return HOST_BASE + (g_host_n++) * 4u;
}

void *jq_host(uint32_t h) {
    if (!h) return 0;
    if (h < HOST_BASE) { jq_log("[jq] 非法宿主句柄 0x%x\n", h); return 0; }
    uint32_t i = (h - HOST_BASE) / 4u;
    if (i >= g_host_n) return 0;
    return g_host_ptr[i];
}

/* 把宿主内存复制进影子堆 */
uint32_t jq_push_bytes(const void *src, uint32_t n) {
    uint32_t p = jq_malloc(n + 1);
    if (!p) return 0;
    memcpy(g_jq_base + p, src, n);
    MEMB(p + n) = 0;
    return p;
}

uint32_t jq_push_string(const char *s) {
    return s ? jq_push_bytes(s, (uint32_t)strlen(s) + 1) : 0;
}

/* ---------------- 分发 ---------------- */
uint32_t jq_plt(uint32_t stub, JQ_ARGS) {
    /* 二分找 stub → slot，再读 .got 令牌 */
    int lo = 0, hi = jq_plt_tbl_n - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (jq_plt_tbl[mid].stub == stub) {
            uint32_t tok = MEM(jq_plt_tbl[mid].slot);
            return jq_call(tok, JQ_PASS);
        }
        if (jq_plt_tbl[mid].stub < stub) lo = mid + 1; else hi = mid - 1;
    }
    jq_log("[jq] 未登记 PLT stub 0x%x\n", stub);
    return 0;
}

uint32_t jq_jni_thunk(uint32_t idx, JQ_ARGS);   /* jq_jni.c */

uint32_t jq_vm_thunk(uint32_t idx, JQ_ARGS);

uint32_t jq_call_inner(uint32_t target, JQ_ARGS);

/* 递归深度安全阀：状态分歧（上游 vtable 调用返回 0/null 留错字段）会使
 * 合法 .text 目标进入无限递归（如 F_6fff8 的 r9 索引永不达哨兵 0x71360）。
 * 单纯返回 0 不破环（被调方拿 0 后仍自循环），故用 setjmp/longjmp 从最外层
 * jq_call 处整体放弃当前 JNI 操作并返回 0（类比"未知调用返回 0"），让生命
 * 周期可继续走完而不崩栈。预算：单个 JNI 入口内的 jq_call 计数上限，兜住
 * 浅层无限循环（如 OnGraphChange 在对象图分歧后反复调 0x0 的空转）。 */
#define JQ_MAX_DEPTH  0x1000u
#define JQ_MAX_BUDGET 0x200000u
static __thread uint32_t g_jq_depth = 0;
static __thread uint32_t g_jq_budget = 0;
static __thread sigjmp_buf g_jq_buf;
static __thread int g_jq_buf_valid = 0;

/* 影子库内崩溃（ART CheckJNI abort / 越界访问）→ 标记分歧 + 放弃当前 JNI 链，
 * 而非让整个进程崩。无有效 setjmp（非影子库上下文）→ 恢复默认处理。 */
extern void jq_jni_mark_diverged(void);
static void jq_sig_handler(int sig) {
    if (g_jq_buf_valid) {
        jq_jni_mark_diverged();
        jq_log("[jq] 捕获信号 %d，放弃当前 JNI 调用链\n", sig);
        siglongjmp(g_jq_buf, 1);
    }
    signal(sig, SIG_DFL);
    raise(sig);
}

uint32_t jq_call(uint32_t target, JQ_ARGS) {
    if (g_jq_depth == 0) {
        g_jq_buf_valid = 1;
        g_jq_budget = JQ_MAX_BUDGET;
        if (sigsetjmp(g_jq_buf, 0) != 0) {
            /* 深度超限/预算耗尽/信号捕获 siglongjmp 回此处：放弃整个调用链 */
            g_jq_depth = 0;
            g_jq_buf_valid = 0;
            jq_log("[jq] 深度超限/预算耗尽，放弃当前 JNI 调用链 (0x%x)\n", target);
            return 0;
        }
    }
    if (++g_jq_depth > JQ_MAX_DEPTH || --g_jq_budget == 0) {
        siglongjmp(g_jq_buf, 1);
    }
    uint32_t rc = jq_call_inner(target, JQ_PASS);
    g_jq_depth--;
    if (g_jq_depth == 0) g_jq_buf_valid = 0;
    return rc;
}

uint32_t jq_call_inner(uint32_t target, JQ_ARGS) {
    target &= ~1u;
    if (target >= IMP_BASE && target < IMP_BASE + 4u * 0x1000u)
        return jq_import((target - IMP_BASE) / 4u, JQ_PASS);
    if (target >= JQ_JAVAVM_TAB && target < JQ_JAVAVM_TAB + 0x40u) {
        /* JavaVM 接口函数槽（GetEnv=0x0b801018 等，经 thunk 令牌分发） */
        return jq_vm_thunk((target - JQ_JAVAVM_TAB) / 8u + 2u, JQ_PASS);
    }
    if (target >= THUNK_BASE && target < THUNK_BASE + 4u * 0x1000u)
        return jq_jni_thunk((target - THUNK_BASE) / 4u, JQ_PASS);
    if (target >= 0x229c0u && target < 0x233e0u)
        return jq_plt(target, JQ_PASS);
    int lo = 0, hi = jq_dispatch_n - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (jq_dispatch[mid].addr == target)
            return jq_dispatch[mid].fn(JQ_PASS);
        if (jq_dispatch[mid].addr < target) lo = mid + 1; else hi = mid - 1;
    }
    /* 回溯：调用者 PC（翻译函数内）+ 节流（每 512 次打一次，防刷屏）。
       64 位宿主 PC → 减 g_jq_base 得 32 位影子地址才有效二分（§124zz：
       直接拿 64 位 PC 低 32 位去二分分发表是误命中——分发表存 32 位影子值）。 */
    static int g_unknown_cnt = 0;
    if ((++g_unknown_cnt & 0x3fff) == 1) {
        void *caller = __builtin_return_address(0);
        uint64_t cpc = (uint64_t)(uintptr_t)caller;
        uint32_t ca32 = 0;
        int in_shadow = 0;
        if (g_jq_base && cpc >= (uint64_t)(uintptr_t)g_jq_base &&
            cpc < (uint64_t)(uintptr_t)g_jq_base + 0x10000000ull) {
            ca32 = (uint32_t)(cpc - (uint64_t)(uintptr_t)g_jq_base);
            in_shadow = 1;
        }
        jq_log("[jq] 未知调用目标 0x%x (r0=0x%x r1=0x%x sp=0x%x) cnt=%d\n",
               target, r0, r1, sp, g_unknown_cnt);
        if (!in_shadow) {
            jq_log("[jq] 调用者不在影子区（host pc=0x%llx，运行时/PLT/imports）\n",
                   (unsigned long long)cpc);
        } else {
            int lo2 = 0, hi2 = jq_dispatch_n - 1, found = -1, hit_addr = 0;
            while (lo2 <= hi2) {
                int mid = (lo2 + hi2) / 2;
                if (jq_dispatch[mid].addr == ca32) { found = 1; hit_addr = ca32; break; }
                if (jq_dispatch[mid].addr < ca32) lo2 = mid + 1; else hi2 = mid - 1;
            }
            if (found)
                jq_log("[jq] 调用者=分发表项 0x%x（命中）\n", hit_addr);
            else
                jq_log("[jq] 调用者=影子区 0x%x 未命中分发表（函数内偏移/PLT/imports）\n", ca32);
        }
    }
    return 0;
}

uint32_t jq_shadow_sp(void) { return STACK_TOP - STACK_GUARD; }

/* ---------------- 启动 ---------------- */
void jq_init(void) {
    jq_mem_init();
    jq_jni_reset();
    /* 安装影子库崩溃兜底：SIGABRT（ART CheckJNI）/SIGSEGV（越界）→ 放弃链 */
    signal(SIGABRT, jq_sig_handler);
    signal(SIGSEGV, jq_sig_handler);
    signal(SIGBUS, jq_sig_handler);
    /* 运行 .init_array 构造函数（全局对象/静态表初始化） */
    for (int i = 0; i < jq_init_array_n; i++) {
        uint32_t sp = STACK_TOP - 0x1000u;
        jq_call(jq_init_array[i], 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sp);
    }
    if (g_verbose) jq_log("[jq] init_array %d 个构造完成，heap=%u\n", jq_init_array_n, g_heap_used);
}
