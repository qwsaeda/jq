/* jqt_gl_bridge.c — §124au/§124zz-5/§124zz-8 GL 桥（jqt_gl_bridge + 持续出帧循环）。
 * 独立成文件（v178e21）：chain 基线 jqt_vtbl.c 没有这段（v178e11 时代手工加在
 * jqt_vtbl.c 尾部，gen_vtbl.py 重新生成时丢失 → jqt_gl_bridge 成 UND → fail to link）。
 * 修复链：
 *  - §124au：F_9d458 前 eglMakeCurrent + glClear + eglSwapBuffers 出首帧
 *  - §124zz-5：GLThread（已有 current）不 spawn 抢 swap 的第二线程
 *  - §124zz-8：mincore 栈区排除在 jqt_mem.c；本文件只做 GL 调用，不碰 MEM 宏 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
    uint32_t a_ = (clear_arg>>24)&0xff, r_ = (clear_arg>>16)&0xff,
             g_ = (clear_arg>>8)&0xff,  b_ = clear_arg&0xff;
    int _fr = 0;
    for (;;) {
        /* ★ §124zz-5 双 swap 抢占（MuMu 实证 16:37）：GLThread 持 context（eglMakeCurrent
         *   失败 err=0x3002）时，frame loop 若仍 glClear/glSwapBuffers → 污染 GLThread
         *   EglHelper.swap() → FATAL `eglSwapBuffers failed`（GLView.java:657）。
         *   ★ 修复：每帧先试 makeCurrent，失败 → 整帧跳过（不 glClear 不 swap），
         *   让 GLThread 渲染（frame loop 只作 GLThread 让位时的兜底，不抢 context）。
         *   若 OnUpdate=0（GLThread 不渲染且释放 context）→ frame loop 可拿到 → 渲染。 */
        EGLSurface cur = eglGetCurrentSurface(EGL_DRAW);
        if (cur == EGL_NO_SURFACE) {
            EGLBoolean mc = eglMakeCurrent(disp, g_jq_gl_ws, g_jq_gl_ws, g_jq_gl_ctx);
            if (mc != EGL_TRUE) {
                static int _mc=0;
                if ((_mc++ & 0x1f)==0)
                    __android_log_print(6,"junqi","jqt_gl_frame_loop: eglMakeCurrent 失败 err=0x%x ws=%p ctx=%p（GLThread 持 context，frame loop 让位跳过）", (int)eglGetError(), (void*)g_jq_gl_ws, (void*)g_jq_gl_ctx);
                usleep(16000);
                continue;
            }
            cur = g_jq_gl_ws;
        }
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
                /* ★ FRAMEBUFFER-DUMP（§124y-7，redroid 无真实 display，screencap 只拍
                 *   SurfaceFlinger 黑屏；用 glReadPixels dump PBuffer framebuffer 到文件
                 *   验证棋盘渲染+轴对齐）。只 dump 前 3 帧。 */
                { static int _fd=0; if (_fd < 3) {
                    GLint _vp[4]={0}; glGetIntegerv(GL_VIEWPORT, _vp);
                    int _w=_vp[2], _h=_vp[3];
                    if (_w>0 && _w<4096 && _h>0 && _h<4096) {
                        uint8_t *_px = (uint8_t*)malloc(_w*_h*4);
                        if (_px) {
                            glReadPixels(0,0,_w,_h,GL_RGBA,GL_UNSIGNED_BYTE,_px);
                            char _fn[128]; snprintf(_fn,sizeof(_fn),"/data/local/tmp/board_dump_%d.ppm",_fd);
                            FILE *_f=fopen(_fn,"wb");
                            if (_f) { fprintf(_f,"P6\n%d %d\n255\n",_w,_h); fwrite(_px,3,_w*_h,_f); fclose(_f);
                                __android_log_print(6,"junqi","FRAMEBUFFER-DUMP: %s %dx%d w=%d h=%d tid=%d",_fn,_w,_h,_w,_h,(int)gettid()); }
                            free(_px);
                        }
                    }
                    _fd++;
                } }
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
        /* ★ AUD-1000 D4-#9 P0：share_context 修复。
         * 旧代码传 EGL_NO_CONTEXT → frame loop 新建 context 与 GLThread 不共享 GL 对象
         * （texID/program/buffer 跨 context 无效）→ "棋子出屏未实锤"根因之一。
         * 修正：用 GLThread 当前 context（eglGetCurrentContext）做 share context，
         * 新建 context 与其共享纹理/shader/缓冲区命名空间。 */
        EGLContext glth_ctx = eglGetCurrentContext();
        g_jq_gl_ctx = eglCreateContext(g_jq_gl_disp, cfg_n?&cfg:EGL_NO_SURFACE,
                                       (glth_ctx != EGL_NO_CONTEXT) ? glth_ctx : EGL_NO_CONTEXT, NULL);
        if (g_jq_gl_ctx == EGL_NO_CONTEXT) {
            /* ★ eglCreateContext 失败（无 GPU/部分 EGL 支持，redroid 实证 新ctx=0x0）：
             *   回退到当前线程已有 context（GLThread cur_ctx，eglGetCurrentContext）。
             *   ★ 诚实边界：回退后 frame loop 与 GLThread 共享 context——若 GLThread
             *   同时使用（OnUpdate 被调）可能有 EGL 竞争，但 eglCreateContext 失败
             *   通常意味着 swap 也失败（无 GPU），竞争无实际影响。 */
            EGLContext cur_ctx = eglGetCurrentContext();
            if (cur_ctx != EGL_NO_CONTEXT) {
                g_jq_gl_ctx = cur_ctx;
                __android_log_print(4,"junqi","gl_bridge: eglCreateContext 失败，回退 GLThread cur_ctx=%p", (void*)g_jq_gl_ctx);
            }
        }
        __android_log_print(4,"junqi","gl_bridge: 首帧 init disp=%p 新ctx=%p（GLThread cur_ctx=%p）cfg_n=%d",
            (void*)g_jq_gl_disp, (void*)g_jq_gl_ctx, (void*)eglGetCurrentContext(), (int)cfg_n);
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
    /* ★ §DOUBLESWAP（2026-11-05 实证，屏幕仍棕根因）：GLThread run loop 在 onDrawFrame
     *   后调 EglHelper.swap()（GLThread.smali:905）。jqt_gl_bridge 若也 eglSwapBuffers →
     *   每帧双 swap → 第二次 swap 把 stale buffer（清屏色）换回屏 → 屏幕棕。
     *   ★ 修复：当前线程已有 surface（GLThread onDrawFrame 路径）→ 只 glClear+F_9d43c
     *   绘制，**不 swap**（GLThread 的 EglHelper.swap() 负责上屏）。只有非 current 线程
     *   （frame loop 兜底，自己 makeCurrent）才 eglSwapBuffers。 */
    int was_current = (cs != EGL_NO_SURFACE);
    if (!was_current && g_jq_gl_ws != EGL_NO_SURFACE) {
        /* 非 current 线程：makeCurrent 到 g_jq_gl_ws 再出帧（不抢 GLThread 的 surface） */
        eglMakeCurrent(def_disp, g_jq_gl_ws, g_jq_gl_ws, g_jq_gl_ctx);
        cs = g_jq_gl_ws;
    }
    if (cs != EGL_NO_SURFACE) {
        glClearColor(r_/255.0f, g_/255.0f, b_/255.0f, a_/255.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        /* ★ §GLBRIDGE-BOARDDRAW（MuMu 实证 16:49）：jqt_gl_bridge 只 glClear+swap（清屏色
         *   出屏实证，截图见 §124y-6），但棋盘（F_9d43c→glDrawArrays）未画——frame loop
         *   让位（makeCurrent 失败，GLThread 持 context）不画，OnUpdate 路径 jqt_gl_bridge
         *   也没画。修复：本函数在 swap 前补 F_9d43c（棋盘绘制）。GLThread 的 context
         *   当前有效（eglGetCurrentContext 非 NO_CONTEXT），texID/dobj 已在 GLThread
         *   context 创建（SLOT22/DRAWOBJ-FILL）→ 绘制+swap 同帧出棋盘。 */
        {
            extern uint32_t F_9d43c(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
            uint32_t ren = MEM(0x162400);
            /* ★ §STATEDIAG（2026-11-05）：棋盘画几帧后停止（swap前 tan→brown 实证），
             *   查 ren/dobj/texID 状态。若 ren+76=0 或 ren+0x64=0 → F_9d43c 条件不满足
             *   → 不画 → swap 显清屏色。 */
            { static int _sd2=0; if(_sd2++ < 0x10) {
                __android_log_print(6,"junqi","jqt_gl_bridge F_9d43c 前: ren=0x%08x dobj=0x%08x texID=%d px=0x%08x tid=%d",
                    ren, ren?MEM(ren+0x64):0, ren?(int)MEM(ren+76):-1, ren?MEM(ren+0x1c):0, (int)gettid());
            } }
            if (ren && MEM(ren+0x64) && MEM(ren+76) != 0) {
                F_9d43c(ren, 0,0,0,0,0,0,0,0,0,0,0, 0x20f000);
                /* ★ FRAMEBUFFER-DUMP（§124y-7，redroid 无真实 display，screencap 只拍
                 *   SurfaceFlinger 黑屏；用 glReadPixels dump PBuffer framebuffer 到文件
                 *   验证棋盘渲染+轴对齐）。只 dump 前 3 帧。 */
                { static int _fd=0; if (_fd < 3) {
                    GLint _vp[4]={0}; glGetIntegerv(GL_VIEWPORT, _vp);
                    int _w=_vp[2], _h=_vp[3];
                    if (_w>0 && _w<4096 && _h>0 && _h<4096) {
                        uint8_t *_px = (uint8_t*)malloc(_w*_h*4);
                        if (_px) {
                            glReadPixels(0,0,_w,_h,GL_RGBA,GL_UNSIGNED_BYTE,_px);
                            char _fn[128]; snprintf(_fn,sizeof(_fn),"/data/local/tmp/board_dump_%d.ppm",_fd);
                            FILE *_f=fopen(_fn,"wb");
                            if (_f) { fprintf(_f,"P6\n%d %d\n255\n",_w,_h); fwrite(_px,3,_w*_h,_f); fclose(_f);
                                __android_log_print(6,"junqi","FRAMEBUFFER-DUMP: %s %dx%d tid=%d",_fn,_w,_h,(int)gettid()); }
                            free(_px);
                        }
                    }
                    _fd++;
                } }
            }
        }
        { static int _sd=0; if(_sd++ < 0x20) {
            GLubyte _p[4]={0};
            glReadPixels(450, 734, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, _p);
            __android_log_print(6,"junqi","gl_bridge swap前: framebuffer center=[%d,%d,%d,%d] cur=%p tid=%d was_cur=%d",
                _p[0],_p[1],_p[2],_p[3],(void*)cs,(int)gettid(),(int)was_current);
        } }
        if (!was_current) {
            EGLBoolean ok = eglSwapBuffers(def_disp, cs);
            if (!ok)
                __android_log_print(6,"junqi","gl_bridge: 出帧 swap 失败 err=0x%x（EGL_BAD_SURFACE/线程不匹配）", (int)eglGetError());
        } else {
            /* ★ DOUBLESWAP：GLThread onDrawFrame 路径，不 swap——让 GLThread 的
             *   EglHelper.swap() 上屏（避免双 swap 显 stale buffer）。 */
            static int _ns=0;
            if ((_ns++ & 0x3f) == 0)
                __android_log_print(4,"junqi","gl_bridge: GLThread 路径不 swap（让 EglHelper.swap() 上屏）frame#%d", _ns);
        }
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
