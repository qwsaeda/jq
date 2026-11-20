#ifndef ANDROID_LOG_STUB_H
#define ANDROID_LOG_STUB_H
/* 桌面构建 stub（Android NDK 侧用真 <android/log.h>）：
 * 用途 = 让 src_rs 的 native 源文件（png_decode.c / ui_menu.c / ui_state.c …）
 * 能在桌面 x86_64 编译（渗透轮 fuzz 探针 / 桌面回归 binary）。
 * 只提供 __android_log_print 声明 + 优先级常量；实现在各 fuzz 探针内（静默桩）。
 * ★ 不影响真机构建：Android NDK 的 <android/log.h> 优先（include 顺序由 -I 决定），
 *   桌面构建显式 -I 本目录且不改 NDK 路径。
 */
#define ANDROID_LOG_VERBOSE 2
#define ANDROID_LOG_DEBUG   3
#define ANDROID_LOG_INFO    4
#define ANDROID_LOG_WARN    5
#define ANDROID_LOG_ERROR   6
#define ANDROID_LOG_FATAL   7

int __android_log_print(int prio, const char *tag, const char *fmt, ...);

#endif /* ANDROID_LOG_STUB_H */
