/*
 * native_window_jni.h — 桌面构建 stub（src_rs/android_stub/android/）
 * 最小 ANativeWindow API（jni_bridge.c 用到 ANativeWindow_fromSurface / release）
 */
#ifndef ANDROID_STUB_NATIVE_WINDOW_JNI_H
#define ANDROID_STUB_NATIVE_WINDOW_JNI_H
#include <stdint.h>
typedef struct ANativeWindow ANativeWindow;
#ifdef __cplusplus
extern "C" {
#endif
ANativeWindow *ANativeWindow_fromSurface(void *env, jobject surface);
void ANativeWindow_release(ANativeWindow *window);
int  ANativeWindow_getWidth(ANativeWindow *window);
int  ANativeWindow_getHeight(ANativeWindow *window);
#ifdef __cplusplus
}
#endif
#endif
