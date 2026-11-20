/*
 * platform_stub.c — 桌面构建平台桩（src_rs/）
 * 用途：渗透轮 JNI 入口 fuzz（fuzz_jni）把 jni_bridge.c 拉到桌面链接时，
 *   补齐平台/GL/音频侧符号（真机构建走真实现，不含本文件）。
 *
 * 桩语义原则（防假绿）：
 *  - **不模拟业务**（不返回假成功数据）→ so 侧若依赖桩返回值，会走"空实现"分支，
 *    探针只关心"不崩 + 参数防御"，不关心业务正确性（业务由真机覆盖）
 *  - ANativeWindow_fromSurface 返 NULL（= surface 未就绪，so 侧走 fallback 分支）
 *  - __system_property_get 返 0（无属性 → 测试钩子不触发）
 *  - jq_glview_is_ready 返 0（GL 未初始化 → so 侧走"未就绪"分支，缩短执行路径）
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "jni.h"
#include "glview_gl.h"
#include "render_obj.h"
#include "junqi.h"

/* android log（src_rs 多处依赖；桌面静默桩） */
int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{ (void)prio; (void)tag; (void)fmt; return 0; }

/* ---- android native window ---- */
struct ANativeWindow { int dummy; };
struct ANativeWindow *ANativeWindow_fromSurface(void *env, jobject surface)
{ (void)env; (void)surface; return NULL; }
void ANativeWindow_release(struct ANativeWindow *w) { (void)w; }
int  ANativeWindow_getWidth(struct ANativeWindow *w) { (void)w; return 0; }
int  ANativeWindow_getHeight(struct ANativeWindow *w) { (void)w; return 0; }

/* ---- system properties（测试钩子开关） ---- */
int __system_property_get(const char *name, char *value)
{ (void)name; if (value) value[0] = 0; return 0; }
int __system_property_set(const char *name, const char *value)
{ (void)name; (void)value; return 0; }

/* ---- glview_gl.c 导出（桌面桩：GL 未初始化 → ready=0） ---- */
int jq_glview_init(jq_glview *gv, const jq_state *st, const char *spak_dir, void *surface,
                   AAssetManager *assets)
{ (void)gv; (void)st; (void)spak_dir; (void)surface; (void)assets; return -1; }
int jq_glview_frame(jq_glview *gv) { (void)gv; return 0; }
void jq_glview_resize(jq_glview *gv, int w, int h) { (void)gv; (void)w; (void)h; }
void jq_glview_set_selected(jq_glview *gv, int cell) { (void)gv; (void)cell; }
void jq_glview_dirty(jq_glview *gv) { (void)gv; }
int  jq_glview_is_ready(void) { return 0; }
void jq_glview_start_render_thread(void) { }
void jq_glview_set_window(void *native_window, void *disp) { (void)native_window; (void)disp; }
void jq_glview_set_surface_env(void *env, void *surface_jobj, void *disp)
{ (void)env; (void)surface_jobj; (void)disp; }
int jq_glview_view_size(int *w, int *h) { if (w) *w = 720; if (h) *h = 1280; return 0; }

/* ---- 渲染/容器/音频侧（桌面空实现） ---- */
#include "render.h"
void rs_render_frame(rs_render *rr) { (void)rr; }
void jq_subsys_list_init(jq_subsys_list *l) { (void)l; }
void jq_subsys_list_push(jq_subsys_list *l, jq_subsys *s) { (void)l; (void)s; }
void jq_egl_obj_init(jq_egl_obj *e) { (void)e; }
void jq_audio_vec_init(jq_audio_vec *v) { (void)v; }
