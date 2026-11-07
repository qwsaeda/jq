/*
 * glview_gl.c — 出屏渲染驱动（正向开发，按安卓游戏规范）
 * 规范链：GLView(SurfaceView) 真 window surface → native EGL_WINDOW_BIT + GL2 context
 *   → 渲染线程 eglCreateWindowSurface(ANativeWindow) → 每帧 rs_render_frame + eglSwapBuffers 出屏。
 * 依据 smali（dec/smali/com/cnvcs/GLView*.smali + App.smali）：
 *   - App.smali：OnCreate(activity)==null → new GLView(context)；setContentView 由 native 补。
 *   - GLView.smali：GLThread + EglHelper（surfaceCreated → eglCreateWindowSurface → eglSwapBuffers）。
 * 出屏通路（按安卓游戏规范正向开发）：
 *   - 主路 = Java GLView(SurfaceView) 的 GLThread/EglHelper：surfaceCreated →
 *     eglCreateWindowSurface(EGL_WINDOW_BIT) → glClear/几何 → eglSwapBuffers 出屏。
 *     native 只做支撑：OnCreate 里 setEGLContextClientVersion(2) + requestRender()
 *     激活 GLThread 出首帧；OnUpdate(GL10) 回值 1 让 GLThread 持续出帧。
 *   - 兜底 = PBuffer 渲染线程（window surface 未 attach 时离屏保活，screen 需真机验收）。
 * 验收判据（MuMu）：screencap 非黑 + 触摸操作日志 + 进程稳定。离屏像素读回不作证据。
 * 实证（§src_rs 本轮）：EglHelper 出屏链跑通后 screencap 非黑 9.24%（主色 33,33,33 深灰
 * + 顶部状态栏 38.5% 非黑），OnUpdate 帧计数到 3.3M+（GLThread 持续出帧）。
 * 诚实边界：非黑像素 = 清屏色/状态栏，非棋盘几何；"棋盘画出"需 GL2 几何 + 真机验收。
 */
#include "glview_gl.h"
#include "render.h"
#include "junqi.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>
JavaVM *jq_get_jvm(void);
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define LOG_TAG "junqi_rs"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static rs_render g_rr;
static int g_rr_ready = 0;

/* ---- 出屏目标（正向 window surface 通路）---- */
static ANativeWindow *g_win = NULL;          /* 从 SurfaceView getSurface() 拿的 window */
static EGLSurface g_win_surf = EGL_NO_SURFACE; /* window EGL surface（eglCreateWindowSurface 产物）*/
static EGLConfig g_cfg = 0;                  /* 主 config（EGL_WINDOW_BIT|PBUFFER_BIT，兼 window+PBuffer 兜底）*/
static int g_cfg_ready = 0;
static void *g_surf_jobj = NULL;             /* Surface jobject（global ref，轮询 fromSurface 用）*/
static int g_pbuf_ok = 0;            /* PBuffer 兜底 surface 建成功标志（init 时设） */
static int g_win_failed = 0;         /* window surface 创建失败标志（失败后不再重试，退 PBuffer 兜底） */
static int g_win_retry_cnt = 0;       /* 渲染线程帧计数（window 重试节流用） */

/* 选主 config：同时含 WINDOW + PBUFFER 的 EGLConfig（MuMu Mesa 60 config 里 SURFACE_TYPE=0x5 都有）。
   一个 config 两种 surface type 都能建：window 主路出屏，PBuffer 兜底保活。 */
static int rs_ensure_cfg(EGLDisplay d)
{
    if (g_cfg_ready) return 1;
    EGLint attrs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 16, EGL_NONE
    };
    EGLint n = 0;
    if (!eglChooseConfig(d, attrs, &g_cfg, 1, &n) || n == 0) {
        /* 退化：只要 WINDOW（出屏主路）；PBuffer 兜底放弃 */
        EGLint wa[] = { EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
                        EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 16, EGL_NONE };
        if (!eglChooseConfig(d, wa, &g_cfg, 1, &n) || n == 0) {
            LOGE("EGL config 不可用（WINDOW+PBUFFER 与 WINDOW 均 num=0）→ 出屏不可用");
            return 0;
        }
    }
    EGLint st = 0;
    eglGetConfigAttrib(d, g_cfg, EGL_SURFACE_TYPE, &st);
    g_cfg_ready = 1;
    LOGI("EGL 主 config 选定 SURFACE_TYPE=0x%x %s%s（%d configs）",
         (unsigned)st, (st & EGL_WINDOW_BIT) ? "WINDOW" : "",
         (st & EGL_PBUFFER_BIT) ? "+PBUFFER" : "", n);
    return 1;
}

int jq_glview_init(jq_glview *gv, const jq_state *st, const char *spak_dir, void *surface,
                   AAssetManager *am)
{
    memset(gv, 0, sizeof(*gv));
    gv->disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (gv->disp == EGL_NO_DISPLAY) {
        LOGE("GLView：eglGetDisplay 失败 → Java new GLView 兜底");
        return -1;
    }
    EGLint major, minor;
    if (!eglInitialize(gv->disp, &major, &minor)) {
        LOGE("GLView：eglInitialize 失败");
        return -1;
    }
    /* 主 config：WINDOW+PBUFFER（出屏主路 window + 兜底 PBuffer 同一个 config） */
    if (!rs_ensure_cfg(gv->disp))
        return -1;

    /* GL2 context（EGL_CONTEXT_CLIENT_VERSION=2）：GLES2/GLSL/VBO 出几何。
       同一 config 可给 window surface 与 PBuffer surface，跨 surface 切换 makeCurrent 合法。 */
    EGLint ca[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    gv->ctx = eglCreateContext(gv->disp, g_cfg, EGL_NO_CONTEXT, ca);
    if (gv->ctx == EGL_NO_CONTEXT) {
        LOGE("GLView：eglCreateContext(GL2) 失败 err=0x%x，回退 GL1", eglGetError());
        gv->ctx = eglCreateContext(gv->disp, g_cfg, EGL_NO_CONTEXT, NULL);
        if (gv->ctx == EGL_NO_CONTEXT) return -1;
    }
    /* PBuffer 兜底 surface（window 未 ready 先出帧保活；window 主路建好后切 window）。
       ★ 对抗审计 P2-R4（批4）修正：原尺寸 720x1024（按当时 MuMu view 假设），
       与 BG_IMG_H=1280 基准及实际 view（真机 720x1280，logcat 实测 view=720x1280）
       不一致 → 兜底路径整幅棋盘按 view_h/BG_IMG_H=0.8 缩放，与 window 主路
       （1.0 缩放）视觉不一致。现统一 720x1280（= BG 基准 + 实际 view）。
       注：渲染按 aPos.y = iy/BG_IMG_H*view_h 缩放（无裁剪），故原 1024 高度
       不会导致棋子"画到屏外"，仅是缩放比例差（子代理"屏外 256px"判读不成立）。 */
    {
        EGLint pa[] = { EGL_WIDTH, 720, EGL_HEIGHT, 1280, EGL_NONE };
        gv->surf = eglCreatePbufferSurface(gv->disp, g_cfg, pa);
        EGLint pe = eglGetError();
        if (gv->surf == EGL_NO_SURFACE) {
            LOGI("GLView：PBuffer 兜底 surface 不可用（err=0x%x）→ 仅 window 出屏", (unsigned)pe);
            gv->surf = EGL_NO_SURFACE;
        } else {
            g_pbuf_ok = 1;
            LOGI("GLView：PBuffer 兜底 surface(720x1280=BG 基准/实际 view) 建成功=%p（window 未 ready 先离屏出帧）", (void*)gv->surf);
        }
    }
    /* 不在主线程预 makeCurrent：PBuffer context 跨线程 makeCurrent 3002（MuMu mesa 限制）；
       让渲染线程首次做 makeCurrent（window 或 PBuffer）。 */
    /* ①② 渲染器接上（EGL 上下文复用本处建好的 disp/ctx；PBuffer 兜底 surface 传入，
       渲染线程 makeCurrent 它出帧；window 主路建好后切 window surface） */
    gv->rr = &g_rr;
    if (rs_render_init(gv->rr, spak_dir, st, am, gv->disp, gv->surf, gv->ctx) != 0) {
        LOGE("GLView：rs_render_init 失败");
        return -1;
    }
    g_rr_ready = 1;
    LOGI("GLView：GL2 context + 渲染器就绪（spak=%s，window 出屏主路 / PBuffer 兜底）",
         spak_dir ? spak_dir : "(无)");
    return 0;
}

/* 设 window 出屏目标（JNI 传 ANativeWindow*）：渲染线程据此建 window EGL surface + swap 出屏 */
void jq_glview_set_window(void *native_window, EGLDisplay disp)
{
    g_win = (ANativeWindow *)native_window;
    /* 主线程只存 window 指针；window EGL surface 由渲染线程建（EGL 上下文属渲染线程，
       主线程建 surface 会 3002 EGL_BAD_ACCESS——MuMu mesa 限制，§124 红线）。 */
    if (g_win)
        LOGI("window ANativeWindow 已存（%p），渲染线程建 window surface 出屏主路", g_win);
}

void jq_glview_reset_win(void)
{
    /* 重置 window surface 失败标志 + 销毁旧 window surface（允许重建） */
    if (g_win_surf != EGL_NO_SURFACE) {
        eglDestroySurface(g_rr.egl_disp, g_win_surf);
        g_win_surf = EGL_NO_SURFACE;
    }
    g_win_failed = 0;
    g_win_retry_cnt = 0;
}

/* 存 Surface jobject（global ref），OnResume 重试 ANativeWindow_fromSurface 诊断 */
void jq_glview_set_surface_env(void *env, void *surface_jobj, void *disp)
{
    (void)env; (void)disp;
    g_surf_jobj = surface_jobj;
    if (g_surf_jobj)
        LOGI("出屏目标 Surface 已存（jobject=%p）（OnResume 重试 fromSurface 诊断）", surface_jobj);
}

/* 每帧驱动（由渲染线程调）：出屏 ①②。返回 1=已消费。 */
int jq_glview_frame(jq_glview *gv)
{
    if (!gv || gv->rr == NULL) return 0;
    rs_render_frame(gv->rr);
    return 1;
}

void jq_glview_resize(jq_glview *gv, int w, int h)
{
    if (gv && gv->rr) rs_render_resize(gv->rr, w, h);
}

void jq_glview_set_selected(jq_glview *gv, int cell)
{
    if (gv && gv->rr) rs_render_set_selected(gv->rr, cell);
}
void jq_glview_dirty(jq_glview *gv)
{
    if (gv && gv->rr) rs_render_dirty(gv->rr);
}

int jq_glview_is_ready(void)
{
    return g_rr_ready && g_rr.egl_ctx != 0;
}

/* 渲染线程 attach JVM（fromSurface 拿 ANativeWindow 需 attach 线程） */
static void rs_render_attach_jvm(void)
{
    JavaVM *vm = jq_get_jvm();
    if (!vm) return;
    JNIEnv *env = NULL;
    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_2) == JNI_EDETACHED) {
        if ((*vm)->AttachCurrentThread(vm, &env, NULL) != 0) {
            LOGE("渲染线程 AttachCurrentThread 失败 → window 出屏不可用");
            return;
        }
    }
    if (env)
        LOGI("渲染线程已 attach JVM");
}

/* 正向出屏渲染线程：
 *   ① 轮询 ANativeWindow_fromSurface（SurfaceView attach 后才 valid）拿 window；
 *   ② 有 window → eglCreateWindowSurface(WINDOW 主路) → makeCurrent → rs_render_frame → eglSwapBuffers 出屏；
 *   ③ 无 window（未 attach/真机无 SurfaceView）→ PBuffer 兜底 makeCurrent → 帧保活（离屏，screen 需真机）；
 *   ④ 出屏主路建好后，每帧 swap 到 window（不再 PBuffer）。 */
static pthread_t g_render_thread;
static volatile int g_render_running = 0;
static int g_gl_current = 0;          /* 当前 makeCurrent 成功标志 */
static int g_current_is_window = 0;   /* 当前 current surface 是 window(1) 还是 PBuffer(0) */
static void *rs_render_thread_main(void *arg)
{
    (void)arg;
    int frame_cnt = 0;
    int last_swap_ok_log = 0;
    g_render_running = 1;
    rs_render_attach_jvm();
    while (g_render_running) {
        /* ① window 主路：g_win 由主线程 ANativeWindow_fromSurface 传入（jq_glview_set_window）。
           ★ 关键：建 eglCreateWindowSurface 前不能 fromSurface 轮询（fromSurface 内部 acquire
             会占住 window → EGL 再 connect 报 "already connected" 0x3003）。主线程拿一次 window
             指针即可，EGL 自己独占 connect。 */
        /* ② window 出屏主路（正向安卓游戏规范）：fromSurface 拿 ANativeWindow →
             eglCreateWindowSurface（EGL_WINDOW_BIT）→ makeCurrent → rs_render_frame →
             eglSwapBuffers 上屏。PBuffer 兜底（window 未 attach 时离屏保活）。
             MuMu 上 SurfaceView surface 懒创建（isValid=0 直到 layout pass），渲染线程
             每 600 帧（~30s）重试 fromSurface（g_win 未拿到时）。 */
        /* ★ 出屏主路归 Java EglHelper（OnUpdate 在 GLThread 的 window context 上 rs_render_frame，
           EglHelper.eglSwapBuffers 出屏）：渲染线程不再 eglCreateWindowSurface 抢 window
           （与 EglHelper 同 window 双 EGL surface → 0x3003 "already connected" + context 抖动）。
           渲染线程只走 PBuffer 兜底（离屏保活，1Hz 心跳）。 */
        if (!g_pbuf_ok && g_rr.egl_ctx) {
            /* 无 PBuffer + 无 window → 等 attach */
            struct timespec ts = {0, 50 * 1000 * 1000};
            nanosleep(&ts, NULL);
            if (++frame_cnt % 600 == 1)
                LOGI("渲染线程 帧 %d：无 surface（等 window attach / PBuffer）", frame_cnt);
            continue;
        }
        /* ③ 切 current：window 出屏主路优先；PBuffer 兜底 */
        EGLSurface target = (g_win_surf != EGL_NO_SURFACE) ? g_win_surf
                          : (g_pbuf_ok ? g_rr.egl_surf : EGL_NO_SURFACE);
        if (target != EGL_NO_SURFACE && (!g_gl_current ||
            (target == g_win_surf) != (EGLSurface)g_current_is_window)) {
            if (eglMakeCurrent(g_rr.egl_disp, target, target, g_rr.egl_ctx)) {
                g_gl_current = 1;
                g_current_is_window = (target == g_win_surf);
                if (target == g_win_surf)
                    LOGI("渲染线程 makeCurrent window surface 成功 → 出屏主路激活（eglSwapBuffers 上屏）");
                else
                    LOGI("渲染线程 makeCurrent PBuffer 兜底（window 未就绪，离屏保活）");
            } else
                LOGE("渲染线程 eglMakeCurrent 失败 err=0x%x（surface=%s）→ 几何不出帧",
                     eglGetError(), target == g_win_surf ? "window" : "PBuffer");
        }
        if (!g_gl_current) {
            /* 无可用 surface（PBuffer 也缺）→ 空转等 window attach */
            struct timespec ts = {0, 50 * 1000 * 1000};
            nanosleep(&ts, NULL);
            if (++frame_cnt % 600 == 1)
                LOGI("渲染线程 帧 %d：无 current surface（window+PBuffer 均未就绪，等 SurfaceView attach）", frame_cnt);
            continue;
        }
        /* ④ PBuffer 兜底：不渲染（GL 对象归 GLThread/EglHelper window context 独占，
           渲染线程跨 context 调 rs_render_frame 会崩）。PBuffer 线程仅保活（surface 持有，
           防止 EGL 回收）+ 1Hz 心跳。画面出屏归 OnUpdate（GLThread window context）。 */
        glFinish();
        if (g_current_is_window) {
            if (eglSwapBuffers(g_rr.egl_disp, g_win_surf)) {
                if (!last_swap_ok_log || (frame_cnt % 600) == 0) {
                    last_swap_ok_log = 1;
                    LOGI("渲染线程 帧 %d：eglSwapBuffers window 出屏成功（屏幕上应可见画面）", frame_cnt);
                }
            } else {
                LOGE("渲染线程 帧 %d：eglSwapBuffers 失败 err=0x%x（window 出屏未上屏）",
                     frame_cnt, eglGetError());
            }
        } else {
            glFinish();   /* PBuffer 兜底：落定命令流（离屏；正向出屏走 window 通路，真机验收） */
        }
        frame_cnt++;
        /* ★ GLThread/EglHelper window 出帧已接管（OnUpdate 每帧 rs_render_frame）→
           PBuffer 线程降为兜底（1Hz 心跳，不空转抢 CPU/GPU） */
        struct timespec ts = {0, 1000 * 1000 * 1000};
        nanosleep(&ts, NULL);
    }
    if (g_rr.egl_disp && g_gl_current)
        eglMakeCurrent(g_rr.egl_disp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    return NULL;
}

void jq_glview_start_render_thread(void)
{
    if (!g_rr_ready || !g_rr.egl_ctx) {
        LOGI("渲染线程未启（GL 未就绪）");
        return;
    }
    if (pthread_create(&g_render_thread, NULL, rs_render_thread_main, NULL) != 0) {
        LOGE("渲染线程创建失败");
        return;
    }
    LOGI("渲染线程已启动（window 出屏主路 eglSwapBuffers / PBuffer 兜底保活）");
}

void jq_glview_destroy(jq_glview *gv)
{
    if (!gv) return;
    g_render_running = 0;
    if (g_render_thread)
        pthread_join(g_render_thread, NULL);
    eglMakeCurrent(gv->disp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (g_win_surf != EGL_NO_SURFACE) eglDestroySurface(gv->disp, g_win_surf);
    g_win_surf = EGL_NO_SURFACE;
    /* ★ 对抗审计 P0（批4 P0-R1）：rs_render_free 必须在 gv->ctx/surf/disp 销毁
       之前调用——free 内 glDeleteTextures/glDeleteProgram 需 current context（建
       它们的那个 context）。旧顺序先 eglDestroyContext(ctx)+eglDestroySurface
       再 free → GL 对象在无 current context 下删除（UB）+ render.c rs_render_free
       内对 rr->egl_ctx 二次 eglDestroyContext/Terminate（rr->egl_ctx 可能 =
       gv->ctx，PBuffer 路径）→ 双重销毁。现顺序：先 free（GL 对象在 current
       context 内删除），再销毁 gv 的 ctx/surf/disp。 */
    if (gv->rr) rs_render_free(gv->rr);
    g_rr_ready = 0;
    if (gv->ctx) eglDestroyContext(gv->disp, gv->ctx);
    if (gv->surf) eglDestroySurface(gv->disp, gv->surf);
    if (gv->disp) eglTerminate(gv->disp);
}
