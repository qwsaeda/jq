// jqt_crt.c — MuMu 32 位 libc 兼容（bionic 32 位未导出 __register_atfork/__cxa_atexit/__cxa_finalize）
// 根因：-shared 链接引入 crtbegin_so/crtend_so，引用这三个符号；MuMu Android 12 的 32 位 libc
//       未导出 __register_atfork（bionic 32 位差异）→ dlopen "failed to link"。
// 修复：翻译体自实现（无 C++ 静态析构，__cxa_atexit 只返回成功不注册；__register_atfork 空操作）。
//       crtbegin_so 引用解析到本地定义（遮蔽 libc），消除 U 符号。
#include <stdint.h>
int __register_atfork(void (*a)(void), void (*b)(void), void (*c)(void), void *d) { (void)a;(void)b;(void)c;(void)d; return 0; }
int __cxa_atexit(void (*f)(void*), void *p, void *d) { (void)f;(void)p;(void)d; return 0; }
void __cxa_finalize(void *d) { (void)d; }
// ★ __cxa_guard_acquire：C++ 静态初始化保护。原版从 libstdc++.so 解析，翻译体无
//   libstdc++ DT_NEEDED → dlopen "failed to link"。自实现（翻译体是 C，无 C++ 静态对象）。
//   guard 语义：bit0=1 已初始化。返回 1=需初始化，0=已初始化跳过。
int __cxa_guard_acquire(int *guard) {
    if (*guard & 1) return 0;  // 已初始化
    return 1;  // 需初始化（翻译体无静态对象，实际不会执行初始化体）
}
void __cxa_guard_release(int *guard) { *guard |= 1; }
void __cxa_guard_abort(int *guard) { (void)guard; }
