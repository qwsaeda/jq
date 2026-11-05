#ifndef RS_RENDER_H
#define RS_RENDER_H
/*
 * render.h — 最小出帧渲染器（①+②：GL 状态机 + 纹理管线）
 * 依据原版：
 *   - GL 导入 35 函数（.dynsym：glClear/glBegin/glVertex/glDrawArrays/glTexImage2D...）
 *   - 渲染入口 0xbd764（OnGraphCreate 桩 bl）/ 0xbd5a0（OnUpdate）/ 0xbdd2c（OnTouch）
 *   - 棋盘纹理 SPAK datah#188（737×751，十字棋盘 PNG，L44/L75）
 *   - 棋子图集 SPAK datah#183（212×280，4列×7行 53×40/格，rank→列/行，L75 已确证）
 * 诚实边界：EGL/渲染对象图为重写（非原版 C++ 场景对象图，③④ 未逆）；
 *   纹理/几何对齐原版 SPAK 资源，出帧后视觉验收需真机 GPU。
 */
#include "junqi.h"
#include "ui_state.h"
#include <stdint.h>
#include <stddef.h>
struct AAssetManager;
typedef struct AAssetManager AAssetManager;

#ifndef RS_NO_GL
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
typedef GLuint rs_glid_t;
typedef EGLDisplay rs_egl_disp_t;
typedef EGLSurface rs_egl_surf_t;
typedef EGLContext rs_egl_ctx_t;
#else
typedef uint32_t rs_glid_t;
typedef int rs_egl_disp_t;
typedef int rs_egl_surf_t;
typedef int rs_egl_ctx_t;
#endif

#define N_ROWS 12
#define N_COLS 5

typedef struct {
    const jq_state *game;   /* 指向 JNI 侧全局对局状态（非快照）：走子/翻棋后渲染自动更新 */
    rs_egl_disp_t egl_disp;
    rs_egl_surf_t egl_surf;
    rs_egl_ctx_t  egl_ctx;
    int view_w, view_h;            /* 渲染尺寸（OnGraphChange 更新） */
    /* ★ 双 slot GL 对象（slot0=PBuffer context，slot1=GLThread/EglHelper window context；
       GL 对象 context 私有，各自建一套，避免跨 context 重建崩溃） */
    rs_glid_t tex_board[2];     /* 棋盘 737×751（GL 纹理，每 context 一套） */
    rs_glid_t tex_piece[2];     /* 棋子图集 212×280（GL 纹理，每 context 一套） */
    rs_glid_t tex_text[2];      /* 菜单按钮文字图集 136×214（原版字体 data_029 光栅化） */
    rs_glid_t tex_bg[2];        /* 对局棋盘背景 datax_177 720×1280（原版完整棋盘背景） */
    rs_glid_t tex_menu_bg[2];   /* ★ 原版 UI 对齐：主菜单背景 datax_294 800×1205（卡通士兵） */
    rs_glid_t tex_imen_text[2]; /* ★ ☰ 菜单文字图集 160×320（8 项竖排，data_029 字体） */
    rs_glid_t tex_imen_icons[2];/* ★ ☰ 菜单图标图集 440×200（8 图标 110×100 4×2，原版裁剪） */
    rs_glid_t tex_hamburger[2]; /* ★ ☰/↩ 图标图集 128×64（orig_human 裁剪） */
    rs_glid_t tex_btn_big[2];   /* ★ 主菜单大按钮图集 802×118（人机+两人，orig_menu 裁剪） */
    rs_glid_t tex_btn_small[2]; /* ★ 主菜单小按钮图集 666×116（联网+联机+四国，orig_menu 裁剪） */
    rs_glid_t tex_popup[2];     /* ★ 选项 UI：popup.png 24×24 9-patch(8,8,16,16) 弹窗背景 */
    rs_glid_t tex_opt_text[2];  /* ★ 选项 UI：弹窗文字图集 109×258（6 标签竖排） */
    rs_glid_t tex_waysel_text[2]; /* ★ 对抗审计（datah_072）：方式选择文字图集 220×184（4 标签竖排） */
    rs_glid_t tex_btimen_text[2]; /* ★ 对抗审计（datah_067）：联机 ☰ 菜单文字图集 160×320（8 项竖排） */
    rs_glid_t tex_settings_text[2]; /* ★ 对抗审计（datah_001）：声音设置文字图集 200×460（10 标签竖排） */
    void *board_px; size_t board_px_len;   /* 解码后 RGBA（跨 context 共享，只解码一次） */
    void *piece_px;  size_t piece_px_len;
    void *text_px;   size_t text_px_len;   /* 菜单文字图集 RGBA（5 label 竖排） */
    int text_w, text_h;                    /* 文字图集尺寸 */
    void *bg_px;     size_t bg_px_len;     /* 对局棋盘背景 datax_177 RGBA（720×1280） */
    int bg_w, bg_h;                        /* 背景尺寸 */
    void *menu_bg_px; size_t menu_bg_px_len;   /* ★ 主菜单背景 datax_294 RGBA（800×1205） */
    int menu_bg_w, menu_bg_h;                  /* 主菜单背景尺寸 */
    void *popup_px;  size_t popup_px_len;  /* ★ 弹窗背景 popup.png RGBA（24×24 9-patch） */
    int popup_w, popup_h;                  /* 弹窗背景尺寸 */
    void *opt_text_px; size_t opt_text_px_len;  /* ★ 弹窗文字图集 RGBA（6 label 竖排） */
    int opt_text_w, opt_text_h;                 /* 弹窗文字图集尺寸 */
    void *waysel_text_px; size_t waysel_text_px_len; /* ★ 方式选择文字图集 RGBA（4 标签竖排 220×184） */
    int waysel_text_w, waysel_text_h;           /* 方式选择文字图集尺寸 */
    void *btimen_text_px; size_t btimen_text_px_len; /* ★ 联机 ☰ 菜单文字图集 RGBA（8 项竖排 160×320） */
    int btimen_text_w, btimen_text_h;           /* 联机 ☰ 菜单文字图集尺寸 */
    void *settings_text_px; size_t settings_text_px_len; /* ★ 声音设置文字图集 RGBA（10 标签竖排 200×460） */
    int settings_text_w, settings_text_h;       /* 声音设置文字图集尺寸 */
    void *imen_text_px; size_t imen_text_px_len; /* ★ ☰ 菜单文字图集 RGBA（8 项竖排 160×320） */
    int imen_text_w, imen_text_h;               /* ☰ 菜单文字图集尺寸 */
    void *imen_icons_px; size_t imen_icons_px_len; /* ★ ☰ 菜单图标图集 RGBA（8 图标 110×100 4×2，原版裁剪） */
    int imen_icons_w, imen_icons_h;             /* ☰ 菜单图标图集尺寸 */
    void *hamburger_px; size_t hamburger_px_len; /* ★ ☰/↩ 图标图集 RGBA（128×64，orig_human 裁剪） */
    int hamburger_w, hamburger_h;               /* ☰/↩ 图标图集尺寸 */
    void *btn_big_px; size_t btn_big_px_len;    /* ★ 主菜单大按钮图集 RGBA（802×118，人机+两人） */
    int btn_big_w, btn_big_h;
    void *btn_small_px; size_t btn_small_px_len;/* ★ 主菜单小按钮图集 RGBA（666×116，联网+联机+四国） */
    int btn_small_w, btn_small_h;
    int sel_cell;                /* 选中格（-1=无） */
    int hl_cells[61]; int hl_n;  /* 可走格高亮（jq_touch_state_targets 结果） */
    int dirty;                   /* 脏标记（走子后重绘） */
    /* VBO 路线（GLES2 + GLSL shader，绕开 GLES1 固定管线几何 no-op） */
    rs_glid_t vbo_board[2];      /* 棋盘 6 顶点 xy+st（每 context 一套） */
    rs_glid_t vbo_piece[2];      /* 棋子动态顶点（每帧重建，每 context 一套） */
    rs_glid_t vbo_sel[2];        /* 选中高亮线（每 context 一套） */
    int gl_ok[2];                /* GL 上下文 + VBO 可用标志（每 slot） */
    /* GLES2 GLSL shader（顶点 xy+st 四元，纹理采样；MuMu 64 位 GL2 context 下几何出帧） */
    rs_glid_t prog_tex[2];       /* 纹理 quad program（棋盘/棋子，每 context 一套） */
    rs_glid_t prog_solid[2];     /* 纯色 program（选中高亮线，每 context 一套） */
    rs_glid_t uViewWH_tex[2];    /* tex program 的 uViewWH uniform 位置（像素→NDC） */
    rs_glid_t uViewWH_solid[2];  /* solid program 的 uViewWH uniform 位置 */
    int gl2_ok[2];               /* GL2 shader 编译成功标志（每 slot） */
    /* GL 首帧延迟初始化（PBuffer context 首次 makeCurrent 在渲染线程，
       GLThread window context 在 OnUpdate 内 rs_gl_rebuild slot1） */
    char *spak_dir;             /* SPAK 纹理目录（首帧建纹理用） */
    AAssetManager *am;          /* AAssetManager（首帧建纹理用） */
    int gl_now_slot;            /* 当前 GL 对象 slot（rs_render_frame 检测切换） */
    int gl_px_loaded;          /* 像素解码完成标志（跨 slot 共享） */
    int board_gl_loaded;       /* 兼容字段（纹理上传已归 rs_gl_rebuild，未用） */
} rs_render;

/* 初始化：复用 jq_glview_init 已建好的 EGL 上下文（不再自建，避免双套 EGL 冲突）。
   加载 SPAK 纹理（从 g_assets 或文件系统）。返回 0 成功。 */
int rs_render_init(rs_render *rr, const char *spak_dir, const jq_state *init_state,
                   AAssetManager *assets,
                   rs_egl_disp_t disp, rs_egl_surf_t surf, rs_egl_ctx_t ctx);
/* 每帧渲染（OnUpdate/OnTimer 调）：清屏 + 棋盘 + 棋子 + 选中高亮。 */
void rs_render_frame(rs_render *rr);
void rs_render_diag(rs_render *rr); /* 出帧诊断（swap 前读 back buffer） */
/* 状态变更（走子/吃子后调用，标记重绘 + 更新选中）。 */
void rs_render_dirty(rs_render *rr);
void rs_render_set_selected(rs_render *rr, int cell);
/* 尺寸更新（OnGraphChange）。 */
void rs_render_resize(rs_render *rr, int w, int h);
/* 释放 EGL + 纹理。 */
void rs_render_free(rs_render *rr);

#endif /* RS_RENDER_H */
