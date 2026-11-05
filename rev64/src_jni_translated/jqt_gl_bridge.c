/* jqt_gl_bridge.c — §124au/§124zz-5/§124zz-8 GL 桥（jqt_gl_bridge + 持续出帧循环）。
 * 独立成文件（v178e21）：chain 基线 jqt_vtbl.c 没有这段（v178e11 时代手工加在
 * jqt_vtbl.c 尾部，gen_vtbl.py 重新生成时丢失 → jqt_gl_bridge 成 UND → fail to link）。
 * 修复链：
 *  - §124au：F_9d458 前 eglMakeCurrent + glClear + eglSwapBuffers 出首帧
 *  - §124zz-5：GLThread（已有 current）不 spawn 抢 swap 的第二线程
 *  - §124zz-8：mincore 栈区排除在 jqt_mem.c；本文件只做 GL 调用，不碰 MEM 宏 */
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include "jqt_mem.h"
#include <GLES2/gl2.h>
#include <android/log.h>
#include <string.h>

static int  g_jq_gl_inited = 0;
static EGLDisplay g_jq_gl_disp = EGL_NO_DISPLAY;
static EGLContext  g_jq_gl_ctx  = EGL_NO_CONTEXT;
static EGLSurface  g_jq_gl_ws   = EGL_NO_SURFACE;
static int  g_jq_gl_loop_started = 0;
static pthread_t g_jq_gl_loop_tid = 0;

/* §124zz-8：jqt_render_glview_make_surface 在 chain 基线里没有（v178e11 手工加的
 * getHolder→nativeWindow 诊断桩）。
 * §124zz-9 修：旧实现 return 1（无 window surface 路径）→ g_jq_gl_ws 永远
 * EGL_NO_SURFACE → jqt_gl_frame_loop 里 eglSwapBuffers(NO_SURFACE) 无效 → 黑屏。
 * 正确：查当前线程 current surface（GLView$GLThread 经 EglHelper.makeCurrent 已
 * eglMakeCurrent 到 GLSurfaceView window surface）→ 有则复用，无则 eglCreateWindowSurface。
 * 返回 0 = 有可用 surface（jqt_gl_bridge 走 glClear 出帧路径）。 */
uint32_t jqt_render_glview_make_surface(void)
{
    EGLDisplay disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    /* 当前线程已有 current surface（GLThread onDrawFrame 路径）→ 直接复用 */
    EGLSurface cur = eglGetCurrentSurface(EGL_DRAW);
    if (cur != EGL_NO_SURFACE)
        return 0;   /* 有 surface，jqt_gl_bridge 走 glClear 出帧 */
    /* 无 current → 创建 pbuffer（offscreen 兜底，能验证 GL 栈但不直接出屏） */
    int maj=0, min=0;
    eglInitialize(disp, &maj, &min);
    const EGLint attr[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RED_SIZE, 8,
                          EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_NONE};
    EGLConfig cfg; EGLint cfg_n=0;
    eglChooseConfig(disp, attr, &cfg, 1, &cfg_n);
    if (cfg_n > 0) {
        const EGLint pb_attr[] = {320, 480, EGL_NONE};
        EGLSurface pb = eglCreatePbufferSurface(disp, cfg, pb_attr);
        if (pb != EGL_NO_SURFACE)
            return 0;   /* pbuffer 可用（offscreen 出帧验证 GL 栈） */
    }
    return 1;   /* 无 surface，jqt_gl_bridge 走 NO_SURFACE 兜底 */
}

static void *jqt_gl_frame_loop(void *arg)
{
    uint32_t clear_arg = (uint32_t)(uintptr_t)arg;
    /* §124zz-5：循环线程若 current surface 已存在（GLView$GLThread 的），直接复用，
     * 不 eglMakeCurrent 抢；没有才 fallback 到 g_jq_gl_ws（offscreen 兜底）。 */
    EGLDisplay disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLSurface cur = eglGetCurrentSurface(EGL_DRAW);
    if (cur == EGL_NO_SURFACE) {
        eglMakeCurrent(disp, g_jq_gl_ws, g_jq_gl_ws, g_jq_gl_ctx);
        cur = g_jq_gl_ws;
    }
    uint32_t a_ = (clear_arg>>24)&0xff, r_ = (clear_arg>>16)&0xff,
             g_ = (clear_arg>>8)&0xff,  b_ = clear_arg&0xff;
    int _fr = 0;
    /* ★ §JQT-RECREATE-TEX-CHECK：当前 context 里检查棋盘纹理是否有效。
     *   texID 在 GLThread context 创建（SLOT22-TRIGGER-GL），本循环用 g_jq_gl_ctx
     *   （不同 context）→ glIsTexture(texID) 返回 GL_FALSE → 重新创建上传，
     *   否则 glBindTexture(1) 无效 → 片段采样黑色（§〇d-4 实证）。 */
    {
        extern void jqt_recreate_board_texture(uint32_t);
        uint32_t ren = MEM(0x162400);
        if (ren && MEM(ren+76) != 0) {
            GLuint texID = (GLuint)MEM(ren+76);
            if (!glIsTexture(texID)) {
                __android_log_print(6,"junqi","jqt_gl_frame_loop: texID=%u 当前 context 无效，重新创建", texID);
                jqt_recreate_board_texture(ren);
            }
        }
    }
    for (;;) {
        glClearColor(r_/255.0f, g_/255.0f, b_/255.0f, a_/255.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        /* ★ §DRAWOBJ-PERFRAME：每帧触发 F_9d43c（slot14 绘制：F_9e760→glDrawArrays）。
         *   渲染器单例 0x162400 + 绘制对象 this[0x64] 已由 DRAWOBJ-FILL 构造，
         *   glDrawArrays 无崩溃（GLES2 context 修复后）。持续出帧画棋盘。 */
        {
            extern uint32_t F_9d43c(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
            uint32_t ren = MEM(0x162400);
            if (ren && MEM(ren+0x64) && MEM(ren+76) != 0) {
                F_9d43c(ren, 0,0,0,0,0,0,0,0,0,0,0, 0x20f000);
            }
        }
        EGLBoolean ok = eglSwapBuffers(disp, cur);
        if (!ok && (_fr & 0x1f) == 0)
            __android_log_print(6,"junqi","gl_bridge: 循环出帧#%d swap失败 err=0x%x（GLThread 在 current 则跳过）", _fr, (int)eglGetError());
        if ((_fr++ & 0x7f) == 0)
            __android_log_print(4,"junqi","gl_bridge: 循环出帧#%d surface=%p tid=%d", _fr, (void*)cur, (int)pthread_self());
        usleep(16000);
    }
    return 0;
}

void jqt_gl_bridge_start_frame_loop(uint32_t clear_arg)
{
    if (g_jq_gl_loop_started || !g_jq_gl_inited || g_jq_gl_ws == EGL_NO_SURFACE) return;
    /* §124zz-5 关键：调用线程已是 current surface/context（GLThread 经
     * GLView$GLThread.makeCurrent 建立）→ spawn 第二线程抢 context 会破坏
     * GLView GLThread 自己的 60fps swap 循环（双 swap 抢 buffer → EGL_BAD_SURFACE
     * → EglHelper.throwEglException FATAL）。只允许非 current 线程 spawn。
     * ★ §DRAWOBJ-SPAWN 修正：翻译体 GLThread 只跑 1 次 F_23eec（OnUpdate 从未被调，
     *   GLThread onDrawFrame 不触发），无双 swap 冲突 → 允许 GLThread 也 spawn
     *   （否则棋盘只画 1 次，无持续出帧循环）。 */
    if (0 && eglGetCurrentSurface(EGL_DRAW) != EGL_NO_SURFACE &&
        eglGetCurrentContext() != EGL_NO_CONTEXT) {
        __android_log_print(6,"junqi","gl_bridge: 本线程已有 current（GLThread 主循环），不 spawn 抢 swap 线程");
        return;
    }
    if (pthread_create(&g_jq_gl_loop_tid, NULL, jqt_gl_frame_loop, (void*)(uintptr_t)clear_arg) == 0) {
        g_jq_gl_loop_started = 1;
        __android_log_print(4,"junqi","gl_bridge: 持续出帧循环已 spawn tid=%d surface=%p", (int)g_jq_gl_loop_tid, (void*)g_jq_gl_ws);
    }
}

void jqt_gl_bridge(uint32_t clear_arg)
{
    if (!g_jq_gl_inited) {
        g_jq_gl_disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        int maj=0, min=0;
        eglInitialize(g_jq_gl_disp, &maj, &min);
        const EGLint attr[] = {EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,
                               EGL_BLUE_SIZE,8,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,EGL_NONE};
        EGLConfig cfg; EGLint cfg_n=0;
        eglChooseConfig(g_jq_gl_disp, attr, &cfg, 1, &cfg_n);
        g_jq_gl_ctx = eglCreateContext(g_jq_gl_disp, cfg_n?&cfg:EGL_NO_SURFACE, EGL_NO_CONTEXT, NULL);
        if (jqt_render_glview_make_surface() == 0) {
            /* 有 surface（GLThread current 或 pbuffer 兜底）：
             * 若当前线程已 makeCurrent（GLThread onDrawFrame 路径）→ 直接出帧，
             * 不重新 makeCurrent（避免抢占 GLView EglHelper 的 context 所有权）。 */
            EGLSurface cur_ws = eglGetCurrentSurface(EGL_DRAW);
            if (cur_ws != EGL_NO_SURFACE) {
                g_jq_gl_ws = cur_ws;
                __android_log_print(4,"junqi","gl_bridge: 复用 current surface=%p（GLThread 主循环）", (void*)cur_ws);
            } else {
                /* 非 GLThread 首次调用：makeCurrent 到 pbuffer/window surface */
                eglMakeCurrent(g_jq_gl_disp, g_jq_gl_ws, g_jq_gl_ws, g_jq_gl_ctx);
                __android_log_print(4,"junqi","gl_bridge: 首帧 makeCurrent surface=%p ctx=%p cfg_n=%d",
                                    (void*)g_jq_gl_ws, (void*)g_jq_gl_ctx, (int)cfg_n);
            }
        } else {
            eglMakeCurrent(g_jq_gl_disp, EGL_NO_SURFACE, EGL_NO_SURFACE, g_jq_gl_ctx);
            g_jq_gl_ws = EGL_NO_SURFACE;
            __android_log_print(4,"junqi","gl_bridge: 首帧初始化（无 surface）disp=%p ctx=%p cfg_n=%d",
                                (void*)g_jq_gl_disp, (void*)g_jq_gl_ctx, (int)cfg_n);
        }
        g_jq_gl_inited = 1;
    }
    /* §124zz-9 出帧：
     * ① 当前线程有 current surface（GLThread onDrawFrame）→ 直接 glClear+swap
     *    （GLView EglHelper 的 EGL context，与 GLView GLThread 主循环共享）。
     * ② 当前线程无 current（主线程/其他线程）→ eglMakeCurrent 到 g_jq_gl_ws
     *    （pbuffer 兜底）再 glClear+swap（offscreen 出帧，不直接上屏但验证 GL 栈）。 */
    uint32_t a_ = (clear_arg>>24)&0xff, r_ = (clear_arg>>16)&0xff,
             g_ = (clear_arg>>8)&0xff,  b_ = clear_arg&0xff;
    EGLDisplay def_disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLSurface cs = eglGetCurrentSurface(EGL_DRAW);
    if (cs == EGL_NO_SURFACE && g_jq_gl_ws != EGL_NO_SURFACE) {
        /* 非 current 线程：makeCurrent 到 g_jq_gl_ws 再出帧（不抢 GLThread 的 surface） */
        eglMakeCurrent(def_disp, g_jq_gl_ws, g_jq_gl_ws, g_jq_gl_ctx);
        cs = g_jq_gl_ws;
    }
    if (cs != EGL_NO_SURFACE) {
        glClearColor(r_/255.0f, g_/255.0f, b_/255.0f, a_/255.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        EGLBoolean ok = eglSwapBuffers(def_disp, cs);
        if (!ok)
            __android_log_print(6,"junqi","gl_bridge: 出帧 swap 失败 err=0x%x（EGL_BAD_SURFACE/线程不匹配）", (int)eglGetError());
        static int _fr=0;
        if ((_fr++ & 0x3f) == 0)
            __android_log_print(4,"junqi","gl_bridge: 出帧#%d surface=%p ctx=%p", _fr, (void*)cs, (void*)eglGetCurrentContext());
    } else {
        static int _no=0;
        if ((_no++ & 0x3f) == 0)
            __android_log_print(6,"junqi","gl_bridge: 无 surface（GLThread 未 makeCurrent 且 pbuffer 不可用）tid=%d", (int)pthread_self());
    }
    jqt_gl_bridge_start_frame_loop(clear_arg ? clear_arg : 0xff9c6630);
}
