/* jq_shadow.h — 影子 32 位地址空间运行时公共定义
 *
 * 原版 libjunqi.so 是 ARM32，所有指针都是 32 位。我们把整个进程的
 * "app 虚拟地址空间"映射成一段真实内存 g_jq_base，原版的每个 32 位
 * 地址 A 对应真实地址 g_jq_base + A。这样翻译出来的 C 里指针宽度、
 * 指针算术、结构体布局全部与原版一致（唯一差别是真实地址加了基址）。
 *
 * 地址空间布局（影子地址，非真实地址）：
 *   0x00000000 - 0x00162e80  原版 .so 镜像 + .bss（映射到文件副本）
 *   0x00162e80 - 0x001f0000  影子栈（sp 初始 0x1f0000，向下增长）
 *   0x02000000 - 0x0a000000  影子堆（128MB，operator new/malloc 用）
 *   0x0b000000 + i*4         import 令牌（jq_import 分发）
 *   0x0c000000               JNIEnv 令牌（*env 指向 0x0c001000 函数表）
 *   0x0c001000 + i*4         JNI 函数表槽位（每格是 0x0d000000+i*4 令牌）
 *   0x0d000000 + i*4         JNI thunk 令牌
 *   0x0e000000 + n*4         Java 对象伪句柄（映射真实 jobject）
 *   0x0f000000 + n*4         宿主指针句柄（FILE*、DIR*、pthread_mutex_t* 等）
 */
#ifndef JQ_SHADOW_H
#define JQ_SHADOW_H

#include <stdint.h>
#include <stddef.h>

extern uint8_t *g_jq_base;
extern uint8_t *g_jq_scratch;   /* 4KB 零页：越界（垃圾指针）读写落到这里，不崩 */

/* 影子地址空间上限（mmap 256MB，见 jq_mem_init）。任何 >= 此值的"指针"都是
 * 对象图分歧产生的垃圾值（如 F_7de74 用 0x43f00042 解引用）。把越界访问导到
 * 零页，读得 0、写丢弃，避免 SIGSEGV —— 等价于"未知调用返回 0"的安全阀。 */
#define MAP_HI 0x10000000u
#define JQ_VALID(a) ((uint32_t)(a) < MAP_HI)
#define MEM(a)   (*(uint32_t *)(JQ_VALID(a) ? g_jq_base + (uint32_t)(a) : g_jq_scratch))
#define MEMH(a)  (*(uint16_t *)(JQ_VALID(a) ? g_jq_base + (uint32_t)(a) : g_jq_scratch))
#define MEMB(a)  (*(uint8_t  *)(JQ_VALID(a) ? g_jq_base + (uint32_t)(a) : g_jq_scratch))
#define MEMP(a)  (g_jq_base + ((uint32_t)(a) < MAP_HI ? (uint32_t)(a) : 0))
#define MEMSH(a) ((int16_t)MEMH(a))

#define JQ_ARGS uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, \
                uint32_t r4, uint32_t r5, uint32_t r6, uint32_t r7, \
                uint32_t r8, uint32_t r9, uint32_t r10, uint32_t r11, uint32_t sp
#define JQ_PASS r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, sp

typedef uint32_t (*jq_fn_t)(JQ_ARGS);
typedef struct { uint32_t addr; jq_fn_t fn; } jq_disp_t;
typedef struct { uint32_t stub; uint32_t slot; } jq_plt_t;
typedef struct { uint32_t slot; uint32_t imp; } jq_slot_imp_t;

extern const jq_disp_t jq_dispatch[];
extern const int jq_dispatch_n;
extern const jq_plt_t jq_plt_tbl[];
extern const int jq_plt_tbl_n;
extern const jq_slot_imp_t jq_slot_imp[];
extern const int jq_slot_imp_n;
extern const uint32_t jq_init_array[];
extern const int jq_init_array_n;

/* 整段镜像（.text+...+.bss，.rodata 已替换为解密态），VA 见 jq_image.c */
extern const unsigned char jq_img_blob[];
extern const unsigned char jq_img_blob_end[];
#define JQ_IMG_BLOB_VA  0x233e0u
#define JQ_IMG_RODATA_VA 0x120bf8u
#define JQ_IMG_DRL_VA    0x155030u
#define JQ_IMG_GOT_VA    0x15b0c8u
#define JQ_IMG_DATA_VA   0x15bb6cu

/* 多寄存器返回值通道：ARM AAPCS 下 r1..r3 是调用者可破坏寄存器，
 * 但 64 位返回值/__aeabi_ldivmod 会用 r0:r1 甚至 r2:r3。翻译函数与
 * import wrapper 都在返回前把最终 r1..r3 存到这里，调用点再取回。 */
extern __thread uint32_t g_jq_ret1, g_jq_ret2, g_jq_ret3;
#define JQ_RET(v) do { g_jq_ret1 = r1; g_jq_ret2 = r2; g_jq_ret3 = r3; return (v); } while (0)
#define JQ_CALL(t) (r0 = jq_call((t), JQ_PASS), r1 = g_jq_ret1, r2 = g_jq_ret2, r3 = g_jq_ret3, r0)

uint32_t jq_call(uint32_t target, JQ_ARGS);
uint32_t jq_plt(uint32_t stub, JQ_ARGS);
uint32_t jq_import(uint32_t idx, JQ_ARGS);
uint32_t jq_vm_thunk(uint32_t idx, JQ_ARGS);

/* JavaVM 接口影子（token 0x0b800000 起，接口函数槽在 0x0b801000 起） */
#define JQ_JAVAVM_BASE 0x0b800000u
#define JQ_JAVAVM_TAB  0x0b801000u

void jq_init(void);
uint32_t jq_shadow_sp(void);
void jq_log(const char *fmt, ...);

/* 影子堆 */
uint32_t jq_malloc(uint32_t n);
void     jq_free(uint32_t p);
uint32_t jq_realloc(uint32_t p, uint32_t n);
uint32_t jq_calloc(uint32_t nmemb, uint32_t size);

/* 宿主指针 ↔ 影子句柄 */
uint32_t jq_handle(void *host);
void    *jq_host(uint32_t h);
/* 把宿主内存块复制进影子堆，返回影子指针 */
uint32_t jq_push_bytes(const void *src, uint32_t n);
uint32_t jq_push_string(const char *s);

/* JNI 桥 */
void     jq_jni_reset(void);
uint32_t jq_jni_env(void);
uint32_t jq_jni_object(void *jobj);
void    *jq_jni_real(uint32_t h);
uint32_t jq_jni_new_object(void *jobj);

#endif /* JQ_SHADOW_H */
