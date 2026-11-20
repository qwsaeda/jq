/* sys/system_properties.h — 桌面构建 stub（src_rs/android_stub/sys/）
 * 最小 __system_property_get/set 声明（jni_bridge.c 的 setprop 测试钩子用） */
#ifndef ANDROID_STUB_SYSTEM_PROPERTIES_H
#define ANDROID_STUB_SYSTEM_PROPERTIES_H
#ifdef __cplusplus
extern "C" {
#endif
int __system_property_get(const char *name, char *value);
int __system_property_set(const char *name, const char *value);
#ifdef __cplusplus
}
#endif
#endif
