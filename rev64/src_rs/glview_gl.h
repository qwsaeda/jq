#ifndef RS_GLVIEW_GL_H
#define RS_GLVIEW_GL_H
#include "junqi.h"
struct AAssetManager;
typedef struct AAssetManager AAssetManager;

typedef struct {
    void *disp;          /* EGLDisplay */
    void *surf;          /* EGLSurface */
    void *ctx;           /* EGLContext */
    void *rr;            /* rs_render*（①② 渲染器） */
} jq_glview;

int  jq_glview_init(jq_glview *gv, const jq_state *st, const char *spak_dir, void *surface,
                    AAssetManager *assets);
int  jq_glview_frame(jq_glview *gv);
void jq_glview_resize(jq_glview *gv, int w, int h);
void jq_glview_set_selected(jq_glview *gv, int cell);
void jq_glview_dirty(jq_glview *gv);
int  jq_glview_is_ready(void);
void jq_glview_destroy(jq_glview *gv);
void jq_glview_start_render_thread(void);  /* native 独立渲染线程（PBuffer GL2，绕开 Java EglHelper） */
void jq_glview_set_window(void *native_window, void *disp);  /* 渲染线程 window surface 目标（JNI 传 ANativeWindow*） */
void jq_glview_reset_win(void);  /* 重置渲染线程 window surface 失败标志（重试前调用） */
void jq_glview_set_surface_env(void *env, void *surface_jobj, void *disp);  /* 存 Surface jobject，渲染线程轮询 fromSurface */

#endif
