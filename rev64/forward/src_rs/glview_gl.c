/*
 * glview_gl.c — 出屏渲染驱动（正向开发，按安卓游戏规范）
 * 规范链：GLView(SurfaceView) 真 window surface → native EGL_WINDOW_BIT + GL2 context
 *   → 渲染线程 eglCreateWindowSurface(ANativeWindow) → 每帧 rs_render_frame + eglSwapBuffers 出屏。
 * 依据 smali（dec/smali/com/qwsaeda/GLView*.smali + App.smali）：
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
#include <pthread.h>
extern pthread_mutex_t g_mutex;   /* ★ AUD-1000-ALL A2-007（P1 已修）：渲染线程 window 出屏持 g_mutex 短读（与 OnUpdate GLThread 路径统一锁契约，消棋子闪错帧竞态） */
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
/* ★ 2026-11-10 flag/mine/restart 回归：渲染线程每帧消费 property 钩子
   （原内联在 OnUpdate，GLThread 停帧时全死 → 6 项回归 FAIL）。g_ehl_env =
   渲染线程 attach JVM 的 env（rs_render_attach_jvm 建，fromSurface 轮询段已在用）；
   env 未建时传 NULL（jq_glview_props_update 内 sfx/on_event 需 env——NULL 时
   仅规则钩子生效，音效/认输跳过，测试钩子可接受）。 */
void jq_glview_props_update(JNIEnv *env);
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
/* ★ MuMu 出屏修复：渲染线程 window fromSurface 重试状态 */
static int  win_retry_cnt = 0;        /* fromSurface 重试次数 */
static int  win_last_try = 0;         /* 上次重试的 frame_cnt（节流每 60 帧） */
static JNIEnv *g_ehl_env = NULL;       /* 渲染线程 attach JVM 拿的 env（fromSurface 需） */
static int  egl_current_thread_attached = 0; /* 渲染线程已 attach JVM（fromSurface 前置） */
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
    /* ★ AUD-1000 B04-097（P1 已标注，720×1280 固定 backing 缩放有意为之）：
       PBuffer 尺寸固定 720×1280（= BG 基准），view 实际尺寸 resize 时**不**
       eglDestroySurface+recreate PBuffer——① 720×1280 固定 backing 经
       rs_render_frame 的 aPos/uViewWH 缩放管线按实际 view 等比缩放出帧
       （aPos.y = iy/BG_IMG_H*view_h，无裁剪，B04 批4 已实证 1024 高度
       不会画到屏外）；② resize 时重建 PBuffer surface 会打断渲染线程
       current（makeCurrent 3002 + 首帧重建闪烁），收益（缩放比 1.0 精确）
       小于代价。诚实边界：view≠720×1280 时 PBuffer 离屏 backing 与 view
       非 1:1，出屏画面经 window 通路（EglHelper 按 view 尺寸 swap）不受
       PBuffer 尺寸影响；PBuffer 仅作离屏保活，非出屏路径。 */
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
    int ri = rs_render_init(gv->rr, spak_dir, st, am, gv->disp, gv->surf, gv->ctx);
    if (ri != 0) {
        LOGE("GLView：rs_render_init 失败");
        return -1;
    }
    g_rr_ready = 1;
    LOGI("GLView：GL2 context + 渲染器就绪（spak=%s，window 出屏主路 / PBuffer 兜底）",
         spak_dir ? spak_dir : "(无)");
    return 0;
}

/* ★ 2026-11-11 ANativeWindow 引用生命周期修复（崩溃根因）：
   ANativeWindow_fromSurface() 每调一次 acquire 一份引用（内部 incStrong），调用者必须
   ANativeWindow_release() 配对（decStrong）。旧代码从 Surface getSurface()/渲染线程
   fromSurface 拿的引用**从不 release** → 底层 ANativeWindow refcount 被抬高，当 Surface
   被系统销毁（窗口切换/20:9 wm size 频繁变更）时 over-release → 崩溃堆栈
   "ANativeWindow_fromSurface+69 → RefBase::decStrong → abort"（decStrong too many times）。
   ★ 修复契约：fromSurface 引用只作 eglCreateWindowSurface 的输入（EGL 内部自己 connect
   acquire 一份持有），创建成功/失败后**立即 release**——代码不长期持有 ANativeWindow，
   杜绝 refcount 过放 + 过期指针操作。 */
static void rs_release_win(void)
{
    if (g_win) {
        ANativeWindow_release(g_win);
        g_win = NULL;
    }
}

/* 设 window 出屏目标（JNI 传 ANativeWindow*）：渲染线程据此建 window EGL surface + swap 出屏 */
void jq_glview_set_window(void *native_window, EGLDisplay disp)
{
    (void)disp;
    /* ★ 覆盖前先释放旧引用（主线程 OnCreate/OnResume 可能多次 set_window，旧 g_win 引用
       不 release 会泄漏 + refcount 紊乱）。新引用仅由渲染线程消费（eglCreateWindowSurface
       输入，建后立即 rs_release_win 释放）。 */
    rs_release_win();
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
/* ★ 2026-11-10 小竖条修复：取渲染层 eglQuerySurface 同步的实际 view 尺寸
   （JNI OnTouch 入口同步 g_view_w/h，触摸命中几何 = 渲染几何基准一致）。 */
int jq_glview_view_size(int *w, int *h)
{
    if (w) *w = g_rr.view_w;
    if (h) *h = g_rr.view_h;
    return 0;
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
        g_ehl_env = env;
        egl_current_thread_attached = 1;
        LOGI("渲染线程已 attach JVM（env=%p，fromSurface 轮询 window 出屏就绪）", (void*)env);
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
        /* ★ MuMu 出屏修复（2026-11-11 全做）：surface 未 attach（OnCreate fromSurface 返 NULL）
           时，渲染线程 attach JVM 后用 global ref 轮询 ANativeWindow_fromSurface（每 60 帧重试），
           拿到 window 后 eglCreateWindowSurface + makeCurrent → 切 window 出屏（画面真正上屏）。
           ★ 0x3003 "already connected" 防护：eglCreateWindowSurface 前先检查 g_win_surf 是否已建，
             且不与 EglHelper 同 window 双 surface（EglHelper 出帧后本线程 eglDestroyContext 让出）。
           PBuffer 仅作 window 未就绪时的离屏保活（1Hz 心跳）。 */
        if (g_win_surf == EGL_NO_SURFACE && egl_current_thread_attached) {
            /* ★ 2026-11-11 g_win 优先消费（主线程 OnCreate fromSurface 成功传入）；
               g_win 为 NULL（主线程未拿到 / surface 懒创建）才 fromSurface 轮询
               （g_surf_jobj 兜底，MuMu surface 懒创建场景，每 60 帧重试）。
               统一所有权：g_win 只作 eglCreateWindowSurface 输入，建后立即 rs_release_win
               释放（不长期持有 ANativeWindow，杜绝 decStrong too many times 崩溃）。 */
            ANativeWindow *nw = g_win;
            int from_retry = 0;
            if (!nw && g_surf_jobj && (!win_retry_cnt || (frame_cnt - win_last_try) >= 60)) {
                win_last_try = frame_cnt;
                win_retry_cnt++;
                nw = ANativeWindow_fromSurface(g_ehl_env, g_surf_jobj);
                from_retry = 1;
            }
            if (nw) {
                if (from_retry)
                    g_win = nw;   /* 轮询拿的归 g_win 统一释放 */
                /* 建 window EGL surface（EGL_WINDOW_BIT）→ 切 window 出屏主路 */
                g_win_surf = eglCreateWindowSurface(g_rr.egl_disp, g_cfg, (EGLNativeWindowType)nw, NULL);
                if (g_win_surf != EGL_NO_SURFACE) {
                    /* ★ 2026-11-10 小竖条修复（根因二）：GLView onSurfaceChanged 未回调
                       native OnGraphChange（调用链断裂）→ view 停留默认 720×1280，
                       而真机 900×1600 → 小竖条+黑边。此处用 eglQuerySurface 取实际
                       window surface 尺寸（EGL_WIDTH/EGL_HEIGHT）→ 同步 g_rr.view_w/h，
                       免 smali 改动，铺满屏。 */
                    EGLint sw = 0, sh = 0;
                    if (eglQuerySurface(g_rr.egl_disp, g_win_surf, EGL_WIDTH, &sw) &&
                        eglQuerySurface(g_rr.egl_disp, g_win_surf, EGL_HEIGHT, &sh)) {
                        if (sw > 0 && sh > 0 && (sw != g_rr.view_w || sh != g_rr.view_h)) {
                            LOGI("渲染线程 eglQuerySurface 同步 view 尺寸：%dx%d → %dx%d（铺满屏修小竖条黑边）",
                                 g_rr.view_w, g_rr.view_h, sw, sh);
                            g_rr.view_w = sw;
                            g_rr.view_h = sh;
                            g_rr.dirty = 1;
                        }
                    }
                    LOGI("渲染线程 window surface 创建成功（%s）→ 出屏主路激活 view=%dx%d",
                         from_retry ? "fromSurface 轮询" : "主线程 set_window", g_rr.view_w, g_rr.view_h);
                } else {
                    LOGE("渲染线程 eglCreateWindowSurface 失败 err=0x%x（%s 引用随即释放，下轮重试）",
                         eglGetError(), from_retry ? "fromSurface" : "set_window");
                }
                /* ★ ANativeWindow 引用生命周期修复：fromSurface 引用只作 EGL 输入
                   （EGL 内部 connect 自己 acquire 持有），创建成功/失败后立即释放——
                   不长期持有 ANativeWindow，杜绝 decStrong too many times 崩溃。 */
                rs_release_win();
            } else if (win_retry_cnt % 12 == 1)
                LOGI("渲染线程 fromSurface 未 ready（第 %d 次，等 surface attach）", win_retry_cnt);
        }
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
            g_current_is_window != ((g_win_surf != EGL_NO_SURFACE && target == g_win_surf) ? 1 : 0))) {
            if (eglMakeCurrent(g_rr.egl_disp, target, target, g_rr.egl_ctx)) {
                g_gl_current = 1;
                g_current_is_window = (target == g_win_surf);
                if (target == g_win_surf)
                    LOGI("渲染线程 makeCurrent window surface 成功 → 出屏主路激活（eglSwapBuffers 上屏）");
                else
                    LOGI("渲染线程 makeCurrent PBuffer 兜底（window 未就绪，离屏保活）");
            } else {
                /* ★ AUD-1000 B04-093（P1 已修）：makeCurrent 失败置
                   g_gl_current=0（旧版保持旧值 1 → 下方 ④ 段误判有 current
                   保活，实际 EGL 内部 current 已无/已变，PBuffer 保活判读错乱
                   + glFinish 在无 current context 调 = GL error 风暴）。
                   置 0 走下方 "无 current surface → 空转等 window attach" 路径。 */
                g_gl_current = 0;
                g_current_is_window = 0;
                LOGE("渲染线程 eglMakeCurrent 失败 err=0x%x（surface=%s）→ 几何不出帧",
                     eglGetError(), target == g_win_surf ? "window" : "PBuffer");
            }
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
        /* ★ AUD-1000 B04-094（P1 已修）：主循环顶 glFinish 已落定命令流，
           PBuffer 分支旧版再 glFinish（同一帧同 context 两次，GPU 同步冗余
           拖 1Hz 心跳节拍）→ 删兜底 else 的冗余 glFinish。 */
        if (g_current_is_window) {
            /* ★ 出图根因修复（2026-11-12）：渲染线程 window 出屏分支此前**只 eglSwapBuffers
               不画几何**——rs_render_frame 只在 GLThread 调 OnUpdate 时画（GLThread 未出帧时
               slot1 的 GL 对象/纹理未建，tex_bg[slot1]=0 → swap 上屏的是黑/清屏色 framebuffer）。
               现 window 分支 swap 前调 rs_render_frame（在 window context 上建 slot1 GL 对象
               + 传纹理 + 画几何），再 eglSwapBuffers 上屏 → 画面真出图。
               诚实边界：渲染线程读 g_state（rr->game->cells）不持 g_mutex（PBuffer 离屏分支
               本就如此，tear 不可见；window 出屏读 g_state 与 OnTouch/OnEvent 写竞态 →
               棋子闪错帧可能性，低频可接受——与 PBuffer 兜底线程同款诚实边界）。 */
            {
                static int ffirst = 0;
                if (!ffirst) { ffirst = 1;
                    LOGI("渲染线程 window 首帧：rs_render_frame 前 ui_screen=%d（UI_MENU=1188/UI_GAME=? 实际值，rs_render_frame 按其分发画面）",
                         (int)(*(int*)&g_rr.game->stage));
                }
            }
            {
                /* ★ swap 前清 GL error queue（AUD-1000 B04 假绿根因 2）：
                   rs_render_frame 内 glDrawElements/glDrawArrays 若触发软驱动 0x502
                   （GL_INVALID_OPERATION 自维持），旧 error 残留会让下次 glGetError
                   永远返 0x502 → 判读"几何未画"假象。swap 前清一次确保 glGetError
                   反映的是 swap 后新状态。 */
                while (glGetError() != GL_NO_ERROR) {}
            }
            /* ★ AUD-1000-ALL A2-007（P1 已修）：window 出屏读 g_state（rr->game->cells）
               持 g_mutex 短读锁——旧实现渲染线程无锁读，与 OnTouch/OnEvent 写 g_state
               竞态 → 棋子闪错帧（窗口出屏可观测路径）。与 OnUpdate GLThread 路径
               （:2609 持锁 rs_render_frame）统一锁契约。PBuffer 兜底线程不持锁
               （不上屏，撕裂不可见，诚实边界不变）。 */
            /* ★ A2-007（P1 已修）+ 出图实证（2026-11-10）：渲染线程 window 出屏读
               g_state 持 g_mutex 短锁（与 OnUpdate GLThread 路径 :2740 持锁契约统一，
               防棋子闪错帧）。rs_render_frame 纯 GL 不回调 Java/不嵌套取 g_mutex
               → 短锁无死锁（OnUpdate 持锁段最长 ms 级，本线程等待 <1 帧）。
               诚实边界：持锁等待期间帧滞后（同 PBuffer 兜底线程不持锁口径一致，
               不上屏撕裂不可见）。 */
            pthread_mutex_lock(&g_mutex);
            rs_render_frame(&g_rr);
            pthread_mutex_unlock(&g_mutex);
            if (eglSwapBuffers(g_rr.egl_disp, g_win_surf)) {
                if (frame_cnt < 200 || (frame_cnt % 300) == 0) {
                    /* ★ 出图实证：swap 成功记 GLERR + 几何就绪 + 纹理状态
                       （frame_cnt<200 前 200 帧都记，防 1Hz 节拍下 600 帧节流漏证据） */
                    GLenum glerr = glGetError();
                    int geom_ready = (g_rr.tex_bg[1] != 0) || (g_rr.board_px != 0);
                    LOGI("渲染线程 帧 %d：swap成功 GLERR=0x%x 几何就绪=%d tex_bg1=%u board_px=%p",
                         frame_cnt, (unsigned)glerr, geom_ready,
                         (unsigned)g_rr.tex_bg[1], (void*)g_rr.board_px);
                }
            } else {
                if (frame_cnt < 50 || frame_cnt % 1000 == 0)
                    LOGE("渲染线程 帧 %d：eglSwapBuffers 失败 err=0x%x（window 出屏未上屏；0x3000=EGL_BAD_SURFACE 周期诊断）",
                         frame_cnt, eglGetError());
                /* ★ 0x3000 周期自愈：swap 连续失败（ANativeWindow buffer 被系统回收后失效）
                   每 3000 帧尝试重建 window surface（fromSurface + eglCreateWindowSurface），
                   免 app 重启。重建成功则后续帧恢复出屏。 */
                if (frame_cnt % 3000 == 2999 && g_surf_jobj && egl_current_thread_attached) {
                    static int g_swap_fail_streak = 0;
                    g_swap_fail_streak++;
                    if (g_swap_fail_streak >= 10) {   /* 连续 10 次失败才重建（防抖动） */
                        LOGI("渲染线程 帧 %d：swap 0x3000 自愈——重建 window surface（连续 %d 次失败）",
                             frame_cnt, g_swap_fail_streak);
                        if (g_win_surf != EGL_NO_SURFACE)
                            eglDestroySurface(g_rr.egl_disp, g_win_surf);
                        g_win_surf = EGL_NO_SURFACE;
                        g_gl_current = 0;
                        g_swap_fail_streak = 0;
                    }
                } else {
                    static int g_swap_fail_streak = 0;
                    g_swap_fail_streak = 0;
                }
            }
        }
        frame_cnt++;
        /* ★ 2026-11-10 flag/mine/restart 回归：每帧消费 property 钩子（restartgame/
           undotest/resigntest/drawtest/stalematetest/closetest/poptest/replay/rules...）。
           渲染线程 env = g_ehl_env（attach JVM 后有效；未 attach 传 NULL——jq_glview_props_update
           内规则段（g_mutex+jq_*）无 env 依赖可执行，sfx/on_event 段 NULL env 下
           (*env)->xxx 会崩→本线程 attach 失败时 g_ehl_env 保持 NULL，on_event/sfx 路径
           需 env 非 NULL——见下方 NULL 守卫）。 */
        if (g_ehl_env)
            jq_glview_props_update(g_ehl_env);
        /* ★ 出图帧率（2026-11-09 修复）：window 出屏主路走 20fps（200ms 节拍，
           原版 60fps 由 Java GLThread/EglHelper 接管）；PBuffer 兜底降 1Hz 保活
           （不空转抢 CPU/GPU） */
        struct timespec ts = g_current_is_window
            ? (struct timespec){0, 200 * 1000 * 1000}
            : (struct timespec){0, 1000 * 1000 * 1000};
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
