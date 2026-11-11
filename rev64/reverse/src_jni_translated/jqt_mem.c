/* jqt_mem.c — 32 位翻译体内存访问实现
 *
 * g_jqt_shadow: 可写影子内存（mmap JQT_MAP_HI），初始 = 原版镜像 + 18 常量注入。
 * 军棋非壳：.rodata 静态=运行时（0/118832 不同），无需解密路由。
 */
#include "jqt_mem.h"
#include "jq_rel_data.h"
#include "jq_bss_runtime.h"
#include "jq_scudo_slots.h"   /* §124y-150: .bss 316 scudo 指针槽映射表 */
#include "jq_event_tbl_fill.h"  /* §124y-154: 事件表 17 行 @ .bss+0x4704 (纯掩码, ref dump 真值) */
/* §124y-151: SCUDO-FILL 默认禁用 (F_c0ed0 析构器死循环根因, 对照实验实锤), 置 1 启用 */
#ifndef JQT_SCUDO_FILL
#define JQT_SCUDO_FILL 0
#endif
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <android/log.h>
#include <stdio.h>

/* 原版 .so 完整镜像（汇编 jq_img.S 定义 g_jqt_img / g_jqt_img_end） */
uint32_t jqt_img_size(void) { return (uint32_t)(g_jqt_img_end - g_jqt_img); }

uint8_t *g_jqt_shadow = 0;
uint8_t *g_jqt_scratch = 0;   /* 4KB 零页：越界（垃圾指针）读写落这里，不崩 */

/* ★ §124x：真实映射区表（/proc/self/maps 建，jqt_mem_init 时快照）。
 *   MEM 对"绝对指针"（>=g_jqt_shadow）先查影子区，再查真实映射区（malloc 堆/scudo/
 *   JNI 对象），命中即直接解引用；未命中落零页不崩。设备上 /proc/self/maps 有 ~2000 条，
 *   256 上限不够（实测 n=256 满表漏掉 scudo 堆 → MISS）→ 扩到 4096 + 排序 + 二分。 */
#define JQT_REAL_MAX 4096
/* AUD-1000 D1-#2 P0 修：双缓冲原子化。
 * 旧代码直接写 g_jqt_real_ranges/g_jqt_real_n，frame-loop 线程 jqt_real_find
 * 二分读并发时，重建瞬间表处于半填充态 → 指针解析错区。
 * 改为双缓冲：写备用表，完成后原子切换指针（32 位平台指针/int 赋值原子）。 */
static struct { uint32_t lo, hi; } g_jqt_real_buf_a[JQT_REAL_MAX];
static struct { uint32_t lo, hi; } g_jqt_real_buf_b[JQT_REAL_MAX];
static struct { uint32_t lo, hi; } *g_jqt_real_active = g_jqt_real_buf_a;
static struct { uint32_t lo, hi; } *g_jqt_real_swp    = g_jqt_real_buf_b;
static int g_jqt_real_n_active = 0;
static int g_jqt_real_n_swp    = 0;
/* 兼容旧引用：g_jqt_real_ranges/g_jqt_real_n 仍指向 active */
#define g_jqt_real_ranges  g_jqt_real_active
#define g_jqt_real_n       g_jqt_real_n_active

static int jqt_range_cmp(const void *a, const void *b) {
    uint32_t la = ((const struct { uint32_t lo, hi; } *)a)->lo;
    uint32_t lb = ((const struct { uint32_t lo, hi; } *)b)->lo;
    return (la > lb) - (la < lb);
}

static void jqt_build_real_ranges(void) {
    /* AUD-1000 D1-#2：写 swp 缓冲，完成后原子切换（不动 active 缓冲） */
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) return;
    char line[512];
    g_jqt_real_n_swp = 0;
    while (fgets(line, sizeof(line), f) && g_jqt_real_n_swp < JQT_REAL_MAX) {
        unsigned long lo, hi; char perm[8];
        if (sscanf(line, "%lx-%lx %4s", &lo, &hi, perm) == 3) {
            /* ★ AUD-1000 B05-001（P1 R-138 诚实标注）：JQT_REAL_MAX=4096 满表时
               静默截断（循环条件直接结束，无 miss 日志/无扩容提示）——
               设备 /proc/self/maps 有 ~1943 条（实测 real_n=1799），常态
               未满；但 scudo 堆增长 + 多线程 mprotect 分片极端场景可能
               逼近上限，漏收映射 → MEM 落零页读 0（与"真实堆指针未进
               快照"同症状）。加"满表打一条 logcat"属行为改动（log 频率），
               R-138 32 位翻译体只标注；扩容选项（2 路表/65536）留待专项。 */
            /* ★ §124ac：只收**可写**区（perm[1]=='w'）。旧逻辑只排除不可读（perm[0]=='-'），
             *   只读区（r--p，如 MuMu 0x39c0000 保留区）也进表 → jqt_mem_addr 返回只读地址 →
             *   str r0,[r1] SEGV_ACCERR（fault 0x39c039c，F_6e994+0x5bc）。写 MEM 必须可写区。
             *   只读区（真实 .rodata 等）翻译体通过影子偏移访问，不需真实映射表命中。
             *   跳过影子区与零页。
             * ★ §124zz-8：排除**栈区**（bionic 栈含 guard page，只读/PROT_NONE 混排，
             *   整段收表后 MEM 读到 guard 页 → SEGV_ACCERR fault 恰在 4KB 边界
             *   （v178e16 GLThread 0xb5ad000）。栈上合法 sp 访问走 jqt_mem_addr 的
             *   host sp 内联路径（§124y-2），不需要真实映射表命中。
             *   识别：/proc/self/maps 里栈区名称含 "[stack"（主线程）或
             *   "0x..." 匿名区紧跟同名 .so 映射之后——简化：名称含 "stack" 排除。 */
            { char name[256]=""; char *ns=line; while (*ns && *ns!='/') ns++;
              if (*ns) strncpy(name, ns, sizeof(name)-1);
              int is_stack = (strstr(name, "stack") != NULL);
              if (perm[1] == 'w' && hi > lo && !is_stack &&
                  !(lo >= (unsigned long)g_jqt_shadow && hi <= (unsigned long)g_jqt_shadow + JQT_MAP_HI) &&
                  !(lo >= (unsigned long)g_jqt_scratch && hi <= (unsigned long)g_jqt_scratch + 4096)) {
                g_jqt_real_swp[g_jqt_real_n_swp].lo = (uint32_t)lo;
                g_jqt_real_swp[g_jqt_real_n_swp].hi = (uint32_t)hi;
                g_jqt_real_n_swp++;
            } }
        }
    }
    fclose(f);
    /* ★ AUD-1000 B05-001（P1 已修）：满表告警——JQT_REAL_MAX=4096 满时打一条 logcat，
       排障者可区分「映射区数正常 <4096」与「满表漏收（scudo 堆增长逼近上限）」。
       常态 1799 条不触达；告警每 600 次重建打一条（防高频 log 刷屏）。 */
    static int jqt_real_warn_cnt = 0;
    if (g_jqt_real_n_swp >= JQT_REAL_MAX && jqt_real_warn_cnt++ % 600 == 0)
        __android_log_print(6, "junqi", "jqt_mem: JQT_REAL_MAX=%d 满表（漏收 /proc/self/maps 尾部映射，scudo 堆 MEM 可能落零页）", JQT_REAL_MAX);
    qsort(g_jqt_real_swp, g_jqt_real_n_swp, sizeof(g_jqt_real_swp[0]), jqt_range_cmp);
    /* 原子切换：读者最多看到一个完整旧快照，不会看到半填充态 */
    { struct { uint32_t lo, hi; } *t = g_jqt_real_active;
      int tn = g_jqt_real_n_active;
      g_jqt_real_active  = g_jqt_real_swp;
      g_jqt_real_n_active = g_jqt_real_n_swp;
      g_jqt_real_swp     = t;
      g_jqt_real_n_swp   = tn;
    }
}

/* 二分查真实映射区（排序后），命中返回 1 */
static int jqt_real_find(uint32_t a) {
    if (g_jqt_real_n <= 0) return 0;
    int lo = 0, hi = g_jqt_real_n - 1;
    while (lo <= hi) {
        int mid = (lo + hi) >> 1;
        if (a < g_jqt_real_ranges[mid].lo) hi = mid - 1;
        else if (a >= g_jqt_real_ranges[mid].hi) lo = mid + 1;
        else return 1;
    }
    return 0;
}

int jqt_mem_valid(uint32_t a) {
    if (a < JQT_MAP_HI) return 1;   /* 影子偏移 */
    if (a >= (uint32_t)g_jqt_shadow && a < (uint32_t)g_jqt_shadow + JQT_MAP_HI) return 1;  /* 影子区 */
    return jqt_real_find(a);        /* 二分查真实映射区 */
}

/* ★ §124ac（本轮）：_ZdlPv guard。F_75ce8 收垃圾 this（对象图未构造）→ free(0x6ae4ea00)
 *   SIGSEGV。判断指针是否可安全 free：NULL 安全（free(NULL) no-op），真实映射区（malloc 堆）
 *   可 free，影子区伪指针（翻译体把偏移转绝对 g_jqt_shadow+off）与无映射垃圾 → 不可 free。
 *   只 free 真实堆指针，跳过影子区/垃圾 → 主线程 _ZdlPv 崩溃消失。 */
int jqt_heap_ok(uint32_t p) {
    if (p == 0) return 1;           /* free(NULL) 是 no-op，安全 */
    if (p >= (uint32_t)g_jqt_shadow && p < (uint32_t)g_jqt_shadow + JQT_MAP_HI) return 0; /* 影子伪指针 */
    return jqt_real_find(p);        /* 真实映射区（malloc/scudo 堆）→ 可 free */
}

/* ★ §124y 修复（sp 语义，崩溃根因）：MEM(sp+N) 里 sp 是真实栈地址（如 0x20ef90 < MAP_HI），
 *   旧 jqt_mem_valid 对 a<MAP_HI 直接返回 1（当偏移）→ MEM(sp+36) 读 shadow+0x20efb4=0
 *   （mmap 零页），原版读真实栈 → r2/r3 垃圾 → L23f8e 写 0x15c0cc=0xffffff10 →
 *   F_76664 入口 r3=0xffffff10 → SIGSEGV fault 0xffff0010。
 *   ★ §124y-2 实证（mincore=-1 + fault 0x20efec host 无映射）：ARM 虚拟栈地址在 host
 *   /proc/self/maps 无映射，且 houdini 只映射 sp 寄存器（不映射任意 ARM 虚拟地址）。
 *   → jqt_mem_addr 无法靠"返回 a"访问 ARM 虚拟栈（调用者解引用 → host SIGSEGV）。
 *   ★ 结论：必须用 ARM sp 寄存器访问 [sp+N]。翻译体 C 函数入口时 ARM sp = sp 参数值，
 *   用内联汇编临时改 sp（mov sp,sp_val; ldr [sp,#N]; mov sp,old）→ houdini 映射到 x86 栈。
 *   本函数保持旧行为（shadow+a for <MAP_HI），sp 访问由生成器改为内联汇编。 */
/* ★ §124ac：mincore 页级验证。真实映射区表是 JNI_OnLoad 快照，scudo 堆释放/增长后
 *   地址可能已不可访问（表命中但实际 fault 0xe85af805 等，SEGV_MAPERR）。
 *   mincore(p,1) 实时检查页是否已映射；未映射 → 落零页（读 0 写丢弃，不崩）。
 *   代价：每 MEM 一次 mincore 系统调用。真实堆读很频繁 → 只对"命中真实映射区表"的
 *   地址做 mincore（影子偏移/影子区不查，快）。 */
static int jqt_mincore_ok(uint32_t a) {
    /* §124zz-8 修：mincore 第 2 参是 **page 数**（size_t），非字节数。
     * 旧代码 vec 是 1 字节 unsigned char + 第 2 参传 4096 → 32 位 libc 每页
     * sizeof(unsigned long)=4 字节 → vec 需 4096*4=16384 字节（1 页驻留标志
     * 位数组），栈上只有 1 字节 → 越界写（v178e14 GLThread SIGSEGV，
     * fault 0xb42d000 恰 4KB 边界 = vec 页首）。
     * 第 2 参改传 1（只查 1 页）+ vec 类型改 unsigned long（4 字节，1 页足量）。
     * AUD-1000 B05-004 诚实边界：vec&1==0（已映射但未驻留）时落零页，与
     * "可访问"语义有差距——大页面 swap 场景原版会缺页调入读到真实值，
     * 翻译体直接读 0（不崩，忠实兜底）。mincore 开销大，仅对命中真实映射
     * 区表的地址调用（快路径影子偏移/影子区不查）。 */
    unsigned long vec = 0;
    void *pg = (void *)(uintptr_t)(a & ~0xffful);
    if (mincore(pg, 1, &vec) != 0) return 0;   /* 未映射/无效 → 落零页 */
    return (vec & 1) != 0;                      /* 页已驻留 */
}
uint32_t jqt_mem_addr(uint32_t a) {
    /* ★ AUD-1000 B05-005（P1 R-138 诚实标注）：查序 = 先真实映射区表，
       再影子偏移区（a < JQT_MAP_HI）。.text/.rodata 偏移（如 0x233e0+
       区内）若数值恰好落在某真实匿名映射区（如 0x2xxxxx 区低 32 位
       撞车）会被真实表抢占 → 影子语义丢失（读真实映射区而非影子镜像）。
       现状：翻译体 so 加载基址高（0x7x/0x8x 段），偏移区 [0x233e0,
       0x200000) 与设备 maps 低段无实测碰撞（real_n=1799 区表抽样
       无 0x2xxxxx 可写区），行为零影响。根治选项（偏移区优先解 +
       真实表只兜底 >=MAP_HI；或建表排除偏移区数值）属行为改动，
       R-138 只标注。 */
    if (jqt_real_find(a)) {
        /* ★ §124ac：表命中但页可能已释放（scudo 快照过期）→ mincore 验证。
         *   未映射 → 落零页（F_75ce8 遍历垃圾 this 读 0 不崩）。 */
        if (!jqt_mincore_ok(a)) return (uint32_t)g_jqt_scratch;
        return a;
    }
    if (a < JQT_MAP_HI) {
        return g_jqt_shadow + a;
    }
    if (a >= (uint32_t)g_jqt_shadow && a < (uint32_t)g_jqt_shadow + JQT_MAP_HI) return a;
    return (uint32_t)g_jqt_scratch;
}

/* ★ §TOUCH-INTERACT-TEST：重建真实映射区表（/proc/self/maps 重扫）。
 *   JNI_OnLoad 快照后 scudo 堆可能增长/新区 mmap → 新分配的 _Znwj 地址不在旧表
 *   → MEM(addr) 落零页（读 0 写丢弃）。F_2afa4 构造游戏状态前调用，让新堆地址命中。 */
void jqt_mem_rebuild(void) {
    jqt_build_real_ranges();
}

/* 18 地址运行时注入表：key=文件偏移（虚拟地址），value=注入值
 * ★ §124z-3 修复：16 个 .text 常量池是 PC 相对位移（原版 `ldr rX,[pc,#N]; add rX,pc`，
 *   翻译体把 add rX,pc 实现为 rX=file_off+rX，故注入值应为**静态位移**（如 0x00138918），
 *   非 redroid 运行时绝对地址（0x96a779e4 等——不同进程基址不同，注入即错）。
 *   旧表用 frida-dump 推算的 redroid 绝对值，导致 r4=MEM(0x235c4)+0x2356C 落到 host
 *   随机映射读垃圾 0x6ae4bdf0 → 主线程 _ZdlPv(垃圾) SIGSEGV。静态值 = jq_img.bin 原始值
 *   （memcpy 后已在影子内存，注入是 no-op，保留表仅为文档/一致性）。2 .data 占位 0。 */
static const struct { uint32_t off; uint32_t val; } g_jqt_got_tbl[] = {
  {0x0233f0, 0x00138a96}, {0x023404, 0x00138a82}, {0x023420, 0x00138a6a},
  {0x023434, 0x00138a52}, {0x023448, 0x00138a3e}, {0x02345c, 0x00138a2a},
  {0x023470, 0x00138a16}, {0x023484, 0x00138a02}, {0x023498, 0x001389ee},
  {0x0234ac, 0x001389da}, {0x0234c0, 0x001389c6}, {0x0234d4, 0x001389b2},
  {0x0235c4, 0x00138918}, {0x0235cc, 0x001388e4}, {0x023604, 0x001388a0},
  {0x023608, 0x001388a0}, {0x15be80, 0x00000000}, {0x15be84, 0x00000000}
};
#define JQT_GOT_N (sizeof(g_jqt_got_tbl)/sizeof(g_jqt_got_tbl[0]))

int jqt_got_lookup(uint32_t addr, uint32_t *out) {
    for (unsigned i = 0; i < JQT_GOT_N; i++) {
        if (g_jqt_got_tbl[i].off == addr) { *out = g_jqt_got_tbl[i].val; return 1; }
    }
    return 0;
}

/* ★ OBJ-FILL（本轮）：.bss 全局对象表 0x15bf00-0x15bf24 自建空对象。
 * 根因：F_23c58 读 0x15BF10（翻译体 .bss=0）→ r0=0 → F_a88e4/F_a7210 收 0 垃圾
 * → 渲染树遍历垃圾 → GLThread F_834c0 循环阀触发。
 * 原版运行时 0x15BF10=0xcf8d7f80（scudo 堆对象指针）；0x15bf00-0x15bf24 共 10 槽
 * 全是 scudo 堆指针（全局对象表，MuMu dump 实证 0xd0829150/0xd0828e90/.../0xf33e3f80）。
 * 修复：calloc 分配 0x1000B 零缓冲（对象图全 0 → STL 空容器语义 begin=end=0,size=0
 * → 遍历立即终止，不崩），写指针进表。分配在 jqt_build_real_ranges **之前**，
 * 确保堆区在真映射区表内（jqt_mem_valid 可命中，MEM 解引用有效）。
 * 诚实边界：空对象只保证"遍历终止"，非真实对象语义（字段值全 0 可能与原版不同）。 */
#define JQT_EMPTY_TBL_N 10
static uint32_t g_jqt_empty_tbl[JQT_EMPTY_TBL_N];
static void jqt_build_empty_objs(void) {
    for (int i = 0; i < JQT_EMPTY_TBL_N; i++)
        g_jqt_empty_tbl[i] = (uint32_t)(uintptr_t)calloc(1, 0x1000);
}

/* ★ §124y-150: 316 scudo 指针槽自建堆对象池 (替代 MuMu 原版 scudo 堆, 跨进程指针失效修复)
 * 背景：.bss dump 的 316 个 scudo 指针槽指向 MuMu 原版进程堆 (0xcf/0xce/0xd0/0xf3 区),
 *   翻译体进程 jqt_mem_valid 真映射区表无此区 → 解引用 0x3a 类小地址 SIGSEGV。
 * 修复：每个 scudo 槽分配 4KB calloc 自建对象 (calloc 堆区在 jqt_build_real_ranges 快照内,
 *   jqt_mem_valid 命中), dump 原值存对象 [0], 槽写自建堆基址。MEM 读槽 = 自建堆指针,
 *   解引用 [0] 得 dump 原值 (如 0xcf8d78f0), 下游 C++ 逻辑读到"看起来正确"的 scudo 块首字段。
 * 诚实边界：自建对象 [1+] 全 0 (scudo 块后续字段如 size/next 未还原), 深度遍历会读到 0
 *   (= 空容器语义, 遍历立即终止), 非真实对象图语义。 */
#include "jq_scudo_slots.h"
static uint32_t g_jqt_scudo_objs[JQT_SCUDO_SLOT_N];
static void jqt_build_scudo_objs(void) {
    for (unsigned i = 0; i < JQT_SCUDO_SLOT_N; i++) {
        void *o = calloc(1, 0x1000);
        if (o) {
            /* §124y-151: 对象 [0] 存 0 (非 dump 值) — scudo 槽非 0 但 C++ 析构器 F_c0ed0 读 [0]=0
             * 走空容器语义 (链表 [r4+40]=0 判 0 终止遍历), 不触发深度遍历死循环。
             * 诚实边界: scudo 块首字段 (size/prev 等) 未还原, 槽非 0 = 对象存在标志,
             * 字段全 0 = STL 空容器 (begin=end=0 语义), 与 OBJ-FILL 10 空对象同模式。 */
            g_jqt_scudo_objs[i] = (uint32_t)(uintptr_t)o;
        }
    }
}

/* ★ §124w-ALIGN-STATIC：GL 顶点缓冲区（真实 calloc 堆指针，GL 驱动可直接解引用）。
 *   原版 dobj+8/12/16 存 _Znwj(malloc) 绝对堆指针；翻译体在 GLThread 转译层
 *   malloc 返 0x3f800000 哨兵（scudo 堆 ABI 差异），calloc 则安全（jqt_build_empty_objs
 *   实证 calloc 堆区在 jqt_build_real_ranges 快照内）。三块：
 *   pos_buf = 18×2 float = 144B / col_buf = 18×4 float = 288B / uv_buf = 18×2 float = 144B */
static uint8_t *g_jqt_pos_buf = 0, *g_jqt_col_buf = 0, *g_jqt_uv_buf = 0;

uint32_t jqt_gl_buf_pos(void) { return (uint32_t)(uintptr_t)g_jqt_pos_buf; }
uint32_t jqt_gl_buf_col(void) { return (uint32_t)(uintptr_t)g_jqt_col_buf; }
uint32_t jqt_gl_buf_uv(void)  { return (uint32_t)(uintptr_t)g_jqt_uv_buf; }

void jqt_mem_init(void) {
    if (g_jqt_shadow) return;
    uint32_t sz = jqt_img_size();
    /* ★ §SINGLETON-FIX-ROOT（本轮根因）：jq_img.bin 实际 0x13b234B，比预期 0x138aa0B 大 0x2794B！
     *   img 尾部 0x138aa0 起 = .bss 区域（0x15be80 起）的**垃圾文件数据**（.GCC: (GNU) 4.4.3
     *   字符串，GCC 版本注释 + .comment/.rodata 尾），memcpy(shadow+0x233e0, img, sz) 用全 sz
     *   会覆盖影子 .bss 0x15be80-0x15ef34（10132B）→ 全局单例槽（0x15be80 表）读到 GCC 字符串
     *   垃圾 → F_bd764/F_b6790 帧驱动收到垃圾指针 → 场景不构造。
     *   修复：截断 sz 到 0x15be80-0x233e0 = 0x138aa0（.text+.rodata+.data 文件内容），
     *   .bss 从 0x15be80 起由下方 memset 正确清零。 */
    uint32_t IMG_EXPECT_SZ = 0x15be80u - 0x233e0u;   /* 0x138aa0 */
    if (sz > IMG_EXPECT_SZ) sz = IMG_EXPECT_SZ;
    g_jqt_shadow = (uint8_t *)mmap(0, JQT_MAP_HI, PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (g_jqt_shadow == MAP_FAILED) { g_jqt_shadow = (uint8_t *)calloc(1, JQT_MAP_HI); }
    /* ★ OBJ-FILL：先分配空对象（扩展堆区），再建真映射区表 → 表含堆区 */
    /* §124w-ALIGN-STATIC：GL 顶点缓冲（真实 calloc 堆指针，GL 驱动可直接解引用） */
    g_jqt_pos_buf = (uint8_t *)calloc(1, 18*2*4);   /* 144B */
    g_jqt_col_buf = (uint8_t *)calloc(1, 18*4*4);   /* 288B */
    g_jqt_uv_buf  = (uint8_t *)calloc(1, 18*2*4);   /* 144B */
    jqt_build_empty_objs();
    /* ★ §124y-150：先分配 316 scudo 槽自建堆对象（calloc 堆区需进真映射区表），再建表 */
    jqt_build_scudo_objs();
    /* ★ §124x：真实映射区表（/proc/self/maps 快照）——MEM 支持 malloc 堆/JNI 对象绝对指针 */
    jqt_build_real_ranges();
    /* 4KB 零页：越界（垃圾指针）读写落这里（mmap 保证零初始化） */
    g_jqt_scratch = (uint8_t *)mmap(0, 4096, PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (g_jqt_scratch == MAP_FAILED) { g_jqt_scratch = (uint8_t *)calloc(1, 4096); }
    /* ★ §124x 修复（根因）：jq_img.bin 从 .text 起点 (0x233e0) 开始
     *   （gen_shadow.py IMG_LO=0x233e0，见 jq_image.c JQ_IMG_VA），必须拷到
     *   g_jqt_shadow+0x233e0，使 g_jqt_shadow[VA] = 镜像[VA-0x233e0] = 原版[VA]。
     *   旧代码拷到 g_jqt_shadow+0 → 所有 .text/.rodata MEM 读偏移 +0x233e0 →
     *   字面量池/free-list 读错 → pthread_mutex_lock(g_jqt_shadow+0) 卡死（splash 永不清除）。
     *   验证：镜像[0]=原版[0x233e0]=10b5011c，镜像[0xFD818]=原版[0x120bf8]="text"。 */
    #define JQT_IMG_VA 0x233e0u
    uint32_t img_end = JQT_IMG_VA + sz;   /* = 0x15be80（.data 结束，首 LOAD+次 LOAD 文件内容） */
    memcpy(g_jqt_shadow + JQT_IMG_VA, g_jqt_img, sz);
    /* ★ 全局槽清零（.bss 0x15be80-0x200000）：模拟运行时未初始化全局=0。
     *   0xb9edc（全局单例初始化）检查 [r5]==0 决定"空→分配"路径；镜像里 .data 是垃圾指针
     *   （未重定位），不清零则 0xb9edc 走"已初始化"分支 → 调用链错位。 */
    if (img_end < 0x200000) memset(g_jqt_shadow + img_end, 0, 0x200000 - img_end);
    /* ★ §124w：拷贝原版 .data 段（0x155000-0x15be80，含 .init_array/.data.rel.ro/.dynamic/.got/.data）
     *   到影子内存。之前 .data 全零 → _Pthread_alloc 读 MEM(0x15B8FC)=0 → pthread_mutex_lock(0) 崩溃。
     *   原版 .data 段在文件里是"未重定位"原始值（如 0x0015f910），需下面 RELATIVE 重定位加基址。 */
    memcpy(g_jqt_shadow + JQ_DATA_VA, jq_data_blob, JQ_DATA_SZ);
    /* ★ §124w：应用 RELATIVE 重定位。原版 dlopen 时 linker 给 5312 个 R_ARM_RELATIVE 槽加 load_base；
     *   影子内存的"load_base"= g_jqt_shadow（影子地址 0 → g_jqt_shadow）。加基址后槽值=绝对指针，
     *   指向影子内存内部（.bss 零初始化 → 池空/mutex=INITIALIZER），外部函数（pthread_mutex_lock）
     *   拿到绝对指针可正常工作。旧代码 .data 全零 → pthread_mutex_lock(0) 崩溃（fault addr 0x1）。 */
    for (unsigned i = 0; i < JQ_REL_N; i++) {
        uint32_t A = jq_rel_tbl[i];
        *(uint32_t *)(g_jqt_shadow + A) += (uint32_t)g_jqt_shadow;
    }
    /* ★ §RPI-FIX：12 个 .bss 标量/sentinel 槽运行时值（MuMu 原版 dump，非指针，安全回填）。
     * 背景：RPI 公式修正（pool=((addr+4)&~3)+disp）后，真超边界 311 条集中在 57 个 .bss 槽。
     * 其中 10 scalar（位标志/小整数/1.0f）+ 2 sentinel（0xffffffff/0xfffffffe 纹理空哨兵）
     * 是静态标量，直接回填安全（跨进程指针问题只影响 scudo 指针槽，本表不含指针）。
     * 15 个 scudo 指针槽需自建对象（F_9d3d4 渲染器单例已构造 0x162400，其余待对象图提取）。
     * 工具：rev64/tools/bss_backfill.py（槽分类+表生成）。 */
    {
        static const struct { uint32_t off; uint32_t val; } jq_bss_scalar_tbl[] = {
            {0x0015f69cu, 0x00000001u},
            {0x0015f89cu, 0x0000003bu},
            {0x00160510u, 0xffffffffu},
            {0x00161e64u, 0x0000ffffu},
            {0x001622c0u, 0x00000001u},
            {0x001623f8u, 0x00000003u},
            {0x00162580u, 0x3f800000u},
            {0x001625bcu, 0x3f800000u},
            {0x0016263cu, 0x3f800000u},
            {0x0016265cu, 0x3f800000u},
            /* ★ AUD-1000 B05-011（P1 R-138 诚实标注）：以上 4 槽 = MuMu 原版
               进程 scudo 堆哨兵值（0x3f800000 类，GLThread 转译层 _Znwj
               返回 0x3f8000xx 实证）——该值在 MuMu 原版进程指向真实 scudo
               堆对象，**跨进程到翻译体无效**：翻译体 _Znwj 走 malloc
               （非 scudo），0x3f800000 无对应堆对象 → MEM 解引用这 4 槽
               读到的地址落零页（jqt_mincore_ok 判未映射 → scratch 读 0），
               忠实兜底（读 0 不崩），非可工作指针。映射为翻译体自建
               calloc 对象基址属行为改动（需补对象图构造），R-138 只标注。 */
            {0x001626c0u, 0xfffffffeu},
            {0x00162924u, 0x0000002au},
        };
        for (unsigned i = 0; i < sizeof(jq_bss_scalar_tbl)/sizeof(jq_bss_scalar_tbl[0]); i++)
            *(uint32_t *)(g_jqt_shadow + jq_bss_scalar_tbl[i].off) = jq_bss_scalar_tbl[i].val;
    }
    /* ★ OBJ-FILL：.bss 全局对象表 0x15bf00-0x15bf24 写自建空对象指针（根因修复）。
     * 原版运行时这些槽是 scudo 堆对象指针（MuMu dump 实证），翻译体 .bss 清零后 = 0
     * → F_23c58 读 0x15BF10=0 → r0=0 垃圾传播 → 渲染树遍历不终止。
     * 写空对象指针（全 0 对象图 → 遍历立即终止）。 */
    for (unsigned i = 0; i < JQT_EMPTY_TBL_N; i++) {
        *(uint32_t *)(g_jqt_shadow + 0x15bf00 + i*4) = g_jqt_empty_tbl[i];
        __android_log_print(4, "junqi", "OBJ-FILL: 0x%08x = empty_obj 0x%08x",
                            0x15bf00 + i*4, g_jqt_empty_tbl[i]);
    }
    /* ★ §124y-150: 316 scudo 指针槽回填自建堆对象 (跨进程指针失效修复, §sp28-SCALE-9c 回退方案改良)
     * 每个 scudo 槽写自建 calloc 对象基址 (jqt_build_scudo_objs 分配, 堆区在真映射区表内),
     * 对象 [0] = dump 原值 (scudo 块首字段), MEM 读槽 → 解引用得 dump 原值。
     * 诚实边界: 自建对象 [1+] 全 0 (scudo 块后续字段未还原), 深度遍历读 0 = 空容器语义。 */
    /* §124y-151: SCUDO-FILL 回填段 (316 scudo 槽 → 自建堆对象基址)
     * 默认禁用 (JQT_SCUDO_FILL=0): 实锤 SCUDO-FILL 让 316 scudo 槽非 0 后,
     * C++ 析构器 F_c0ed0 (C0ED0 区, 已收表) 遍历 scudo 链表读到非 0 子对象 → 循环不终止。
     * 即使对象 [0]=0, 槽本身非 0 = C++ 容器"已构造"标志 → 析构器走"非空"路径 → 读 [子对象+40] 非 0 深度遍历。
     * 对照实验实锤: 无 SCUDO-FILL 4 路径 PASS, 有 SCUDO-FILL F_c0ed0 死循环 (WATCHDOG 停留)。
     * 诚实边界: .bss scudo 槽回填在"非 0 = 已构造"语义下不可行, 保持全 0 (未构造) 忠实兜底。
     * 待 JQT_SCUDO_FILL=1 启用 (需 F_c0ed0 遍历到 [0]=0 即终止的修复, 或 unidbg 补 C++ 对象图)。 */
#if JQT_SCUDO_FILL
    for (unsigned i = 0; i < JQT_SCUDO_SLOT_N; i++) {
        if (g_jqt_scudo_objs[i] == 0) continue;
        /* §124y-151: 跳过 OBJ-FILL 区 0x15bf00-0x15bf24 (10 空对象表, 渲染树遍历修复专用, 勿覆盖) */
        uint32_t va = jq_scudo_slot_tbl[i].va;
        if (va >= 0x15bf00u && va < 0x15bf28u) continue;
        *(uint32_t *)(g_jqt_shadow + va) = g_jqt_scudo_objs[i];
    }
    { static int _sc=0; if (++_sc <= 4) __android_log_print(4, "junqi",
        "SCUDO-FILL: 回填 %u 槽 (首 0x%08x=0x%08x, dump 原值 0x%08x)",
        JQT_SCUDO_SLOT_N, jq_scudo_slot_tbl[0].va, g_jqt_scudo_objs[0], jq_scudo_slot_tbl[0].dump_val); }
#else
    /* AUD-1000 B05-014：JQT_SCUDO_FILL=0 时旧代码无条件打"回填 N 槽"日志（误导排障），
       现改为明确的"跳过"日志 */
    { static int _sc=0; if (++_sc <= 4) __android_log_print(4, "junqi",
        "SCUDO-FILL: 跳过（JQT_SCUDO_FILL=0，§124y-151 对照实验禁用）"); }
#endif
    /* §124y-166 修正: EVENT-TBL-FILL 事件掩码表 17 行 × 17 字段 = **289 字节** @ .bss+0x4704
     * （F_3a92c 真实访问序 = 17B stride, 旧版 4B stride/68B 写错位置, 已纠正）
     * 掩码字节语义 = (事件码 0..16) << 4 | 标志位（高 nibble = 事件码）
     * 与 scudo 槽 0 交集（纯掩码值非指针）→ F_c0ed0 析构器不深度遍历（无子对象指针）。
     * 对照 §124y-151 scudo 槽回填不可行（非 0 指针 = 已构造容器）, 纯掩码值安全。
     * ★ §124y-166: 改为**字节级**回填（逐字节写, 避免 4B 写覆盖相邻字节）。 */
#if JQ_EVENT_TBL_FILL
    for (unsigned i = 0; i < JQ_EVENT_TBL_BYTES; i++) {
        *(uint8_t *)(g_jqt_shadow + JQ_EVENT_TBL_BASE + i) = jq_event_tbl_fill_bytes[i];
    }
    { static int _et=0; if (++_et <= 4) __android_log_print(4, "junqi",
        "EVENT-TBL-FILL: 回填 %u 字节 @ .bss+0x4704 (17行×17字段 stride=17B, §124y-166 字节级, ref dump 真值)",
        JQ_EVENT_TBL_BYTES); }
#endif
    /* ★ §sp28-SCALE-9c .bss 运行时构造态回填 = 实证失败（回退）：
     * MuMu 原版 32 位 so 进程 dd /proc/PID/mem dump .bss 28680B（79% 非零）memcpy 进
     * 影子 .bss → GLThread SIGSEGV SEGV_MAPERR 0x3a（下游读 .bss 槽里的 scudo 指针
     * 0xcxxxxxxx，该堆在翻译体进程未映射 → 解引用 0x3a 类小地址崩）。
     * 根因 = .bss 运行时值里的指针指向 MuMu 原版进程堆（scudo 0xcfxxxxxx），
     * 翻译体进程 jqt_mem_valid 真映射区表无此区 → 越界 0x3a。
     * 结论：.bss 整段回填不可行（指针跨进程失效），只能填"翻译体自身构造的对象"
     * （F_9d3d4 渲染器单例 0x162400，sp28-ROOT 已证可行）。回退全 0 清零稳定态。 */
    /* if (img_end < 0x200000) { 回退后保留原清零逻辑（下面 memset 已覆盖） */
    /* 注入 18 地址运行时值（.text 内嵌常量池，dlopen 才填） */
    for (unsigned i = 0; i < JQT_GOT_N; i++) {
        *(uint32_t *)(g_jqt_shadow + g_jqt_got_tbl[i].off) = g_jqt_got_tbl[i].val;
    }
    /* ★ §124x 诊断：验证镜像偏移修复生效。
     *   MEM(0x6e9c0) 应为 0xec728（_M_deallocate 字面量池），MEM(0x15b8fc) 应为 g_jqt_shadow+0x15f910。 */
    __android_log_print(4, "junqi", "mem_init: shadow=%p sz=0x%x img_end=0x%x real_n=%d", g_jqt_shadow, sz, img_end, g_jqt_real_n);
    /* §124zz-8：dump maps 前 8 条（确认栈区在表外的依据） */
    { char ln[512]; FILE *mf=fopen("/proc/self/maps","r"); int _d=0;
      if (mf) { while (fgets(ln,sizeof(ln),mf) && _d<8) { _d++;
                __android_log_print(4,"junqi","mem_init: maps[%d]=%s", _d-1, ln); } fclose(mf); } }
    /* §124zz-8 诊断：dump 表前 16 条 + 表里所有含 [stack 的条目（验证栈区排除生效） */
    for (int i = 0; i < g_jqt_real_n && i < 16; i++)
        __android_log_print(4, "junqi", "mem_init: rtbl[%d]=0x%08x-0x%08x", i,
                            g_jqt_real_ranges[i].lo, g_jqt_real_ranges[i].hi);
    /* 表已排掉 [stack 区（jqt_build_real_ranges 过滤）——确认：遍历 /proc/self/maps 里
     * [stack] 行，检查其范围是否**不在**表内 */
    { char ln[512]; FILE *mf=fopen("/proc/self/maps","r");
      if (mf) { while (fgets(ln,sizeof(ln),mf)) {
          char *st=strstr(ln,"[stack");
          if (st) { unsigned long lo,hi;
            if (sscanf(ln,"%lx-%lx",&lo,&hi)==2) {
              int in=0;
              for (int i=0;i<g_jqt_real_n;i++)
                if (lo>=g_jqt_real_ranges[i].lo && lo<g_jqt_real_ranges[i].hi) { in=1; break; }
              __android_log_print(6,"junqi","mem_init: [stack] 0x%08x-0x%08x 在表内=%d（应 0）",
                                  lo,hi,in);
            } } } fclose(mf); } }
    __android_log_print(4, "junqi", "mem_init: MEM(0x6e9c0)=0x%08x MEM(0x120bf8)=0x%08x MEM(0x15b8fc)=0x%08x",
                        *(uint32_t *)(g_jqt_shadow + 0x6e9c0),
                        *(uint32_t *)(g_jqt_shadow + 0x120bf8),
                        *(uint32_t *)(g_jqt_shadow + 0x15b8fc));
}
