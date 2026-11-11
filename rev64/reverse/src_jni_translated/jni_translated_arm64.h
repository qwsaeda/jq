// jni_translated_arm64.h — 32 位翻译体 → arm64 地址模型适配层
// 原版 JNI 入口是 32 位 Thumb（指针 uint32_t，.so 基址 0x40000000 + GOT 重定位）。
// 翻译体 C 用 uint32_t MEM(addr)。arm64 运行需：
//   1. 把 32 位指针语义映射到 arm64 的 64 位真实地址（.so 重定位基址）
//   2. .rodata 加密常量池需注入 frida-dump 明文（运行时自解密结果）
//   3. bl/blx 存根 r0=0（与回归一致）
//
// 此头提供 32→64 指针换算宏 + 全局表注入点。编译 arm64 时包含。
#ifndef JNI_TRANSLATED_ARM64_H
#define JNI_TRANSLATED_ARM64_H
#include <stdint.h>

/* 32 位 .so 虚拟基址（EABI dlopen 默认 0x40000000，ELF32） */
#define JQ32_SO_BASE   0x40000000u
#define JQ32_RODATA_OFF 0x120bf8u   /* .rodata 段偏移（readelf） */
#define JQ32_TEXT_OFF   0x23000u

/* 运行时注入（由 JNI_OnLoad / 主 App 初始化）：
   - g_jq64_text_base: arm64 下 .text 映射基址（mmap 的 libjunqi.so 段）
   - g_jq64_rodata_plain: frida-dump 明文字节（118KB），覆盖加密 .rodata 常量池 */
extern uint8_t *g_jq64_text_base;     /* arm64 .text 运行基址 */
extern uint8_t *g_jq64_rodata_plain;  /* 解密后 .rodata 明文（frida-dump/rodata_dec.bin） */
extern uint32_t g_jq64_got_init[0x1000]; /* 模拟 .data.rel.ro 已初始化全局表 */

/* 32 位指针 → arm64 真实地址：
   若 32 位值落在 .text/.rodata 段偏移（加 SO_BASE 后），映射到 arm64 运行基址 */
static inline uint64_t jq32_to_64(uint32_t v) {
    /* v 是 32 位虚拟地址（含 SO_BASE）。减去 SO_BASE 得段偏移，加 arm64 基址 */
    uint32_t off = v - JQ32_SO_BASE;
    return (uint64_t)g_jq64_text_base + off;
}

/* 32 位翻译体的 MEM 宏适配：读 32 位地址 v 处的 4/2/1 字节
   .rodata 区（加密常量池）→ 读 frida-dump 明文；.text 区 → 读 arm64 运行基址 */
#define JQ64_MEMB32(v)  (* (uint8_t *)jq32_to_64((uint32_t)(v)) )
#define JQ64_MEMH32(v)  (* (uint16_t*)jq32_to_64((uint32_t)(v)) )
#define JQ64_MEM32(v)   (* (uint32_t*)jq32_to_64((uint32_t)(v)) )

#endif
