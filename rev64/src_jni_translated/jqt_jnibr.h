/* jqt_jnibr.h — so→Java JNI 桥（JQJNI）
 *
 * 原版 32 位 so 里 so→Java 调用是 FindClass/GetMethodID/CallXxxMethod，
 * 机械翻译体里这些是 bl 存根（r0=0）。JQJNI 桥在适配层补 JNI 语义：
 * 翻译体侧调 jqt_jni_call(id, ret, args...)，桥层用 JNIEnv 真正调 Java。
 *
 * 82 个 JNI 调用（gen_jni_calls.py 提取）每条一个 jni_id。
 * id 对应 jni_aligned.c 里 jq_jni_calls[] 表（由 jq_jni_calls.json 生成）。
 */
#ifndef JQT_JNIBR_H
#define JQT_JNIBR_H
#include <stdint.h>

/* jni_id 由 jq_jni_calls.json 分配（0..N） */
#define JQJNI_MAXCALLS 96

/* 桥入口：ret=调用返回的 jobject/jint 句柄（低 32 位），0 表示失败
 *   args[] 最多 8 个参数（jobject 句柄 / jint 值混合，按签名序）
 *   n_args 实际参数个数。jni_id 对应表项。
 *   返回值：jobject 的 (uint32_t)(uintptr_t) 或 jint 值或 0（void 方法） */
uint32_t jqt_jni_call(uint32_t jni_id, uint32_t n_args, const uint32_t *args);

/* ★ selftest 逐条验证：GetMethodID 是否成功（按 jni_id 缓存）
 *   call_ok=1 表示该 id 的 FindClass+GetMethodID 都成功（不管 CallXxx 结果）
 *   call_cls_fail=1 表示 FindClass 就失败（类不存在，如渲染链未初始化） */
int jqt_jni_call_ok(uint32_t jni_id);
int jqt_jni_call_cls_fail(uint32_t jni_id);

/* selftest 逐条验证：只触发 FindClass+GetMethodID 解析（不调 CallXxx，
 *   避免 receiver=null 跨线程局部引用失效 → invalid jobject）。
 *   返回 1=解析成功，0=失败（类或方法名/签名不匹配）。 */
int jqt_jni_resolve_only(uint32_t jni_id);

/* 注册全局 JNIEnv（JNI_OnLoad 调） */
void jqt_jni_setenv(void *env, void *activity);

#endif
