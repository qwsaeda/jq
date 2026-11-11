/*
 * render_stub.c — 桌面 headless 渲染桩（无 EGL/GL，jq_run 用）
 * 实现 render.h 的全部符号为 no-op，让桌面可执行不链接 Android GL。
 * 真机渲染走 render.c（EGL + glClear/glBegin/glTexImage2D）。
 */
#include "render.h"

int rs_render_init(rs_render *rr, const char *spak_dir, const jq_state *init_state,
                    AAssetManager *am, rs_egl_disp_t disp, rs_egl_surf_t surf, rs_egl_ctx_t ctx)
{
    (void)spak_dir; (void)am; (void)disp; (void)surf; (void)ctx;
    rr->game = init_state;
    rr->sel_cell = -1;
    rr->view_w = 720; rr->view_h = 1280;
    rr->dirty = 1;
    return 0;   /* 桌面：无 EGL，仅同步状态 */
}
void rs_render_frame(rs_render *rr)
{
    /* 桌面 headless：不真正出帧，仅同步游戏状态 */
    if (rr->game) (void)rr->game;
}
void rs_render_dirty(rs_render *rr) { rr->dirty = 1; }
void rs_render_set_selected(rs_render *rr, int cell) { rr->sel_cell = cell; rr->dirty = 1; }
void rs_render_resize(rs_render *rr, int w, int h)
{
    if (w > 0) rr->view_w = w;
    if (h > 0) rr->view_h = h;
}
void rs_render_free(rs_render *rr) { (void)rr; }
