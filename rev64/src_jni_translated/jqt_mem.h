/* jqt_mem.h — 32 位翻译体统一内存访问
 *
 * 原版 libjunqi.so 的 VirtAddr = Offset（p_vaddr 从 0 起），翻译体 MEM(addr)
 * 用的是"虚拟地址=文件偏移"。运行时真实地址 = so 基址 + addr。
 *
 * 本头把 MEM/MEMH/MEMB 统一为可写影子内存 g_jqt_shadow（mmap MAP_HI）：
 *   - 初始化 jqt_mem_init()：mmap + memcpy 原版镜像（.text/.rodata 静态明文）
 *     + 注入 18 地址运行时值（.text 内嵌常量池，dlopen 才填）
 *   - MEM(addr) 直接读写 shadow[addr]（.data/.bss 区可写，翻译体写全局/对象）
 *
 * 这样翻译体 C 在 arm32/arm64 下语义一致，唯一差别是编译目标架构。
 */
#ifndef JQT_MEM_H
#define JQT_MEM_H

#include <stdint.h>

/* 原版 .so 镜像（汇编 jq_img.S .incbin），基址 0（虚拟地址=偏移） */
extern const unsigned char g_jqt_img[];
extern const unsigned char g_jqt_img_end[];
uint32_t jqt_img_size(void);

/* 可写影子内存（mmap MAP_HI，镜像+got 注入后即用） */
extern uint8_t *g_jqt_shadow;
#define JQT_MAP_HI 0x10000000u
/* ★ 越界保护（§113 同款）：垃圾指针（注入值不准/未重定位常量池）读落到零页，不崩。
 *   MEM(144496) 读注入值当指针 → MEM(0x1758xxxx) 超出 256MB 映射 → 原无保护 SIGSEGV。
 *   OnTimer 实测崩溃（fault addr 0x7dd136dc716a = g_jqt_shadow+0x36dc716a 越界）根因。 */
extern uint8_t *g_jqt_scratch;   /* 4KB 零页：越界读写落这里，不崩 */
/* ★ §124w：支持绝对指针。RELATIVE 重定位后 .data 槽值是"绝对指针"（g_jqt_shadow+offset）。
 *   旧宏 JQT_VALID(addr)=addr<MAP_HI 对绝对指针（>=g_jqt_shadow）返回 false → 落到零页读 0。
 *   新宏：addr>=g_jqt_shadow（绝对指针）→ 直接读 addr（影子内存已 mmap）；否则（偏移）→ g_jqt_shadow+addr。
 *   注意：g_jqt_shadow > MAP_HI（mmap 高位），偏移（<MAP_HI）与绝对指针（>=g_jqt_shadow）不重叠。 */
/* ★ §124x 修复：支持真实映射地址（malloc 堆/scudo/JNI 对象等绝对指针）。
 *   JQT_VALID 旧版只允许影子区 [g_jqt_shadow, g_jqt_shadow+MAP_HI)，真实堆指针
 *   （如 _Znwj 返回 0xea58fa50，在 [anon:scudo:primary]）被拒 → 落到零页读 0 →
 *   F_b635c 链表遍历 r5=0 永不终止（无限循环）。jqt_mem_valid 用 /proc/self/maps
 *   建真实映射区表（jqt_mem_init 时建），命中即允许直接解引用。
 *   影子偏移（<MAP_HI）与影子区 [g_jqt_shadow, g_jqt_shadow+MAP_HI) 永远有效。 */
#define JQT_IS_ABS(a)  ((uint32_t)(a) >= (uint32_t)g_jqt_shadow)
#define JQT_ADDR(a)    (JQT_IS_ABS(a) ? (uint32_t)(a) : g_jqt_shadow + (uint32_t)(a))
int jqt_mem_valid(uint32_t a);   /* jqt_mem.c 实现：偏移/影子区/真实映射区 */
/* ★ §124ac：_ZdlPv guard——指针可安全 free？NULL 安全，真实映射区（malloc 堆）可 free，
 *   影子区伪指针/无映射垃圾 → 0（跳过）。jqt_OnCreate.c _ZdlPv 用它挡 F_75ce8 垃圾 this。 */
int jqt_heap_ok(uint32_t p);
#define JQT_VALID(a)   (jqt_mem_valid((uint32_t)(a)))
/* ★ §124y 修复（sp 语义）：MEM(sp+N) 里 sp=0x20ef90 是真实栈地址（<MAP_HI），
 *   jqt_mem_valid 把它当"偏移"读 shadow+0x20efb4=0，原版读真实栈 → 崩溃。
 *   jqt_mem_addr() 真实映射优先（栈/堆/JNI 对象），命中直接返回 a（真实地址），
 *   未命中才当影子偏移。所有 MEM/MEMH/MEMB/MEMP 改用 jqt_mem_addr。 */
uint32_t jqt_mem_addr(uint32_t a); /* jqt_mem.c 实现：真实映射优先，返回最终访问地址 */
/* ★ §124w：外部函数指针参数转换。翻译体把影子"偏移"（<MAP_HI）传给外部函数（clock_gettime/
 *   pthread_mutex_lock 等），外部函数要绝对指针 → 加 g_jqt_shadow。若已是绝对指针（>=MAP_HI，
 *   RELATIVE 重定位输出 / malloc 返回）→ 保持原样。 */
#define JQT_ABS_PTR(a) ((uint32_t)(a) < JQT_MAP_HI ? g_jqt_shadow + (uint32_t)(a) : (uint32_t)(a))

/* 18 地址运行时注入表查询：命中返回 1 并写 *out；未命中返回 0 */
int jqt_got_lookup(uint32_t addr, uint32_t *out);

/* 初始化：mmap 影子 + memcpy 镜像 + 注入 18 常量 */
void jqt_mem_init(void);

/* ★ §TOUCH-INTERACT-TEST：重建真实映射区表（scudo 堆增长后调用，让新堆地址命中 MEM） */
void jqt_mem_rebuild(void);

/* 内存读/写：jqt_mem_addr 解析（真实映射优先），越界落 g_jqt_scratch 零页不崩 */
#define MEM(addr)  (*(uint32_t *)(jqt_mem_addr((uint32_t)(addr))))
#define MEMH(addr) (*(uint16_t *)(jqt_mem_addr((uint32_t)(addr))))
#define MEMB(addr) (*(uint8_t  *)(jqt_mem_addr((uint32_t)(addr))))
#define MEMP(addr) (jqt_mem_addr((uint32_t)(addr)))

#endif /* JQT_MEM_H */
