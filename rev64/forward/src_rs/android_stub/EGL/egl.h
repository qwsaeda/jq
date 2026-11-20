/* EGL/egl.h — 桌面构建 stub（src_rs/android_stub/EGL/）
 * 最小 EGL 类型与常量（jni_bridge.c / glview_gl.c 用到的子集）。
 * ★ 桌面探针（fuzz_jni）不建 GL context——这些常量仅供编译期类型/常量解析；
 *   运行时相关函数由 glview_gl.c 的桌面分支或探针桩处理。
 */
#ifndef ANDROID_STUB_EGL_H
#define ANDROID_STUB_EGL_H
#include <stdint.h>
typedef void *EGLDisplay;
typedef void *EGLConfig;
typedef void *EGLContext;
typedef void *EGLSurface;
typedef void *EGLClientBuffer;
typedef int32_t EGLint;
typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef int EGLint32;

#define EGL_DEFAULT_DISPLAY     ((EGLDisplay)0)
#define EGL_NO_DISPLAY          ((EGLDisplay)0)
#define EGL_NO_CONTEXT          ((EGLContext)0)
#define EGL_NO_SURFACE          ((EGLSurface)0)
#define EGL_FALSE               0
#define EGL_TRUE                1
#define EGL_SUCCESS             0x3000
#define EGL_NOT_INITIALIZED     0x3001
#define EGL_BAD_ACCESS          0x3002
#define EGL_BAD_ALLOC           0x3003
#define EGL_BAD_ATTRIBUTE       0x3004
#define EGL_BAD_CONFIG          0x3005
#define EGL_BAD_CONTEXT         0x3006
#define EGL_BAD_CURRENT_SURFACE 0x3007
#define EGL_BAD_DISPLAY         0x3008
#define EGL_BAD_MATCH           0x3009
#define EGL_BAD_NATIVE_WINDOW   0x300B
#define EGL_BAD_PARAMETER       0x300C
#define EGL_BAD_SURFACE         0x300D
#define EGL_BUFFER_SIZE         0x3020
#define EGL_ALPHA_SIZE          0x3021
#define EGL_BLUE_SIZE           0x3022
#define EGL_GREEN_SIZE          0x3023
#define EGL_RED_SIZE            0x3024
#define EGL_DEPTH_SIZE          0x3025
#define EGL_NONE                0x3038
#define EGL_SURFACE_TYPE        0x3033
#define EGL_WINDOW_BIT          0x0004
#define EGL_PBUFFER_BIT         0x0001
#define EGL_RENDERABLE_TYPE     0x3040
#define EGL_OPENGL_ES2_BIT      0x0004
#define EGL_CONTEXT_CLIENT_VERSION 0x3098
#define EGL_WIDTH               0x3057
#define EGL_HEIGHT              0x3056
#define EGL_VENDOR              0x3053
#define EGL_VERSION             0x3054
#define EGL_EXTENSIONS          0x3055

#ifdef __cplusplus
extern "C" {
#endif
EGLDisplay eglGetDisplay(void *display_id);
EGLBoolean eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor);
EGLBoolean eglTerminate(EGLDisplay dpy);
EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);
EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, void *window, const EGLint *attrib_list);
EGLSurface eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx);
EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface);
EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface);
EGLBoolean eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);
const char *eglQueryString(EGLDisplay dpy, EGLint name);
#ifdef __cplusplus
}
#endif
#endif /* ANDROID_STUB_EGL_H */
