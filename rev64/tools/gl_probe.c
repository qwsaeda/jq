/* gl_probe.c — MuMu 64 位 GL 出帧能力探测（app 进程内跑）
 * 编译 + 推送 + 运行：
 *   NDK=/tmp/opencode/android-ndk-r25b/toolchains/llvm/prebuilt/linux-x86_64
 *   $NDK/bin/aarch64-linux-android24-clang -O2 gl_probe.c -o /tmp/opencode/gl_probe \
 *       -lEGL -lGLESv1_CM -lGLESv2 -llog -lm
 *   adb push /tmp/opencode/gl_probe /data/local/tmp/gl_probe
 *   adb logcat -c && adb shell "/data/local/tmp/gl_probe"
 *   adb logcat -d | grep gl_probe
 * 判据（logcat gl_probe tag）：
 *   - PBUFFER 清屏色读回 = 设定色 → GL 离屏渲染通
 *   - 几何 no-op（纯色三角不变）→ MuMu 转译 GL 几何不可用（环境限制）
 */
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <android/log.h>
#include <stdlib.h>
#include <string.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "gl_probe", __VA_ARGS__)

#define CLEAR_R 0.0f
#define CLEAR_G 0.0f
#define CLEAR_B 1.0f          /* 纯蓝，区分桩全黑 */
#define READX 160
#define READY 240

int main(void) {
    EGLDisplay d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (d == EGL_NO_DISPLAY) { LOGI("display 无效"); return 1; }
    EGLint maj = 0, min = 0;
    if (!eglInitialize(d, &maj, &min)) { LOGI("eglInitialize 失败（需 app 进程 GL 环境）"); return 2; }
    const char *vendor = eglQueryString(d, EGL_VENDOR);
    const char *rend   = (const char *)eglQueryString(d, 0x30D1); /* EGL_RENDERER */
    LOGI("EGL %d.%d vendor=%s renderer=%s", maj, min,
         vendor ? vendor : "(null)", rend ? rend : "(null)");

    /* PBUFFER config（MuMu 无窗口，必须 PBUFFER_BIT；WINDOW_BIT 配 PBuffer → 3002） */
    EGLint pa[] = { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
                    EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
                    EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 16, EGL_NONE };
    EGLConfig cfg; EGLint n = 0;
    if (!eglChooseConfig(d, pa, &cfg, 1, &n) || n == 0) { LOGI("PBuffer config 不可用 n=%d", n); return 3; }
    EGLint psa[] = { EGL_WIDTH, 720, EGL_HEIGHT, 1280, EGL_NONE };
    EGLSurface s = eglCreatePbufferSurface(d, cfg, psa);
    if (s == EGL_NO_SURFACE) { LOGI("PBuffer surface 失败 err=0x%x", eglGetError()); return 4; }
    EGLContext c = eglCreateContext(d, cfg, EGL_NO_CONTEXT, NULL);
    if (c == EGL_NO_CONTEXT) { LOGI("Context 失败"); return 5; }
    eglMakeCurrent(d, s, s, c);

    glViewport(0, 0, 720, 1280);
    glClearColor(CLEAR_R, CLEAR_G, CLEAR_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glFinish();
    unsigned char px[4];
    glReadPixels(READX, READY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    unsigned r1 = (px[0]<<16) | (px[1]<<8) | px[2];
    int clear_ok = (px[0] > 128 && px[1] < 40 && px[2] > 128);  /* 纯蓝 255,0,255 */
    LOGI("[1] glClear 清屏色=0x%06x %s（期望 0x0000ff 纯蓝）", r1,
         clear_ok ? "OK（GL 离屏渲染通）" : "FAIL（全黑/错色，GL no-op 或桩）");

    /* 几何路径 1：client-side array（glVertexPointer 主机指针，无纹理纯色三角） */
    static float tri[] = { 0,0,0,0,  720,0,0,0,  0,1280,0,0 };
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(4, GL_FLOAT, 0, tri);
    glDisable(GL_TEXTURE_2D);
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);  /* 纯红 */
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisableClientState(GL_VERTEX_ARRAY);
    glFinish();
    glReadPixels(READX, READY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    unsigned r2 = (px[0]<<16) | (px[1]<<8) | px[2];
    int geo1_ok = (r2 != 0x0000ff);  /* 变了 = 几何出帧 */
    LOGI("[2] glVertexPointer 纯色三角后=0x%06x %s", r2,
         geo1_ok ? "OK（几何出帧）" : "no-op（MuMu 转译 GL 几何不可用）");

    /* 几何路径 2：glDrawTexfOES（OES_draw_texture，不依赖顶点指针） */
    typedef void (*drawtex_fn)(GLfloat, GLfloat, GLfloat);
    drawtex_fn dtx = (drawtex_fn)eglGetProcAddress("glDrawTexfOES");
    if (dtx) {
        GLuint t = 1;
        glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_2D, t);
        unsigned char solid[4] = { 255, 0, 0, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, solid);
        glEnable(GL_TEXTURE_2D);
        glColor4f(1,1,1,1);
        dtx(READX, READY, 0);
        glFinish();
        glReadPixels(READX, READY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
        unsigned r3 = (px[0]<<16) | (px[1]<<8) | px[2];
        int geo2_ok = (r3 != 0x0000ff);
        LOGI("[3] glDrawTexfOES 后=0x%06x %s", r3, geo2_ok ? "OK" : "no-op");
        clear_ok |= geo2_ok;  /* 任一路径出几何都算出帧 */
    } else {
        LOGI("[3] glDrawTexfOES 未导出（eglGetProcAddress=NULL）");
    }

    LOGI("=== 结论：清屏%s 几何%s ===",
         clear_ok ? "出帧" : "不出帧",
         (geo1_ok) ? "出帧" : "no-op（需真机 GPU 验收）");
    eglDestroyContext(d, c);
    eglDestroySurface(d, s);
    eglTerminate(d);
    return 0;
}
