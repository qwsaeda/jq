/*
 * render.c — 出帧渲染器（GLES2 + GLSL + VBO）
 * 渲染链：EGL GL2 context → GLSL shader（顶点 xy+st → 纹理采样）→
 *   棋盘纹理（datah#188 737×751）+ 棋子图集（datah#183 212×280）VBO glDrawArrays。
 *
 * 为什么 GLES2 不是 GLES1：
 *   原版（API 19 时代）用 GLES1 固定管线，但本库是 arm64 现代重写，不必跟着用老栈。
 *   MuMu 64 位 GLES1 固定管线几何 no-op（已证），而 GLES2 context（EGL_CONTEXT_CLIENT_VERSION=2）
 *   下 GLSL + VBO + glVertexAttribPointer 真出像素（gl2_probe 实证：全屏红三角形读回 255,0,0,255）。
 *   GLES2 是 Android arm64 设备通用标准渲染栈。
 *
 * 纹理来源：SPAK 解包 PNG（spak_res/），经 png_decode 转 RGBA → glTexImage2D。
 * 棋盘几何对齐原版 L44：棋盘主体 5 列在 PNG x 0.366-0.631，y 全高。
 * 诚实边界：对象图为重写（非原版 C++ 场景对象图，③④ 见 render.h）；
 *   GL2 几何出帧在 MuMu 上由本库 GL2 context 自验（PBuffer 离屏），真机走窗口 surface。
 */
#include "render.h"
#include "png_decode.h"
#include "ui_menu.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define LOG_TAG "junqi_rs"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define N_ROWS 12
#define N_COLS 5
#define BOARD_PNG_W 737
#define BOARD_PNG_H 751
#define PIECE_PNG_W 212
#define PIECE_PNG_H 280
#define PIECE_GRID_W 4
#define PIECE_GRID_H 7
#define BOARD_UV_X0 0.366f
#define BOARD_UV_X1 0.631f

/* 顶点：xy（屏幕像素）+ st（纹理 UV），interleaved 四 float */
typedef struct { float x, y, u, v; } rs_v4f;

/* ---------------- GLSL shaders ---------------- */
static const char *RS_VS_SRC = "attribute vec2 aPos;attribute vec2 aUV;uniform vec2 uViewWH;varying vec2 vUV;void main(){vec2 ndc = aPos / uViewWH * 2.0 - 1.0;ndc.y = -ndc.y;gl_Position = vec4(ndc, 0.0, 1.0);vUV = aUV;}";
static const char *RS_FS_TEX_SRC =
    "precision mediump float;"
    "varying vec2 vUV;"
    "uniform sampler2D uTex;"
    "void main(){ gl_FragColor = texture2D(uTex, vUV); }";
static const char *RS_FS_SOLID_SRC =
    "precision mediump float;"
    "uniform vec4 uColor;"
    "void main(){ gl_FragColor = uColor; }";

static int rs_build_shader(rs_glid_t *out, GLenum type, const char *src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[256];
        glGetShaderInfoLog(sh, 256, NULL, log);
        LOGE("shader 编译失败: %s", log);
        glDeleteShader(sh);
        return 0;
    }
    *out = sh;
    return 1;
}

/* 建纹理 quad program（VBO xy+st → 采样纹理） */
static int rs_build_tex_prog(rs_glid_t *prog, rs_glid_t *aPos, rs_glid_t *aUV,
                             rs_glid_t *uTex)
{
    rs_glid_t vs, fs;
    if (!rs_build_shader(&vs, GL_VERTEX_SHADER, RS_VS_SRC)) return 0;
    if (!rs_build_shader(&fs, GL_FRAGMENT_SHADER, RS_FS_TEX_SRC)) {
        glDeleteShader(vs);
        return 0;
    }
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glBindAttribLocation(p, 0, "aPos");
    glBindAttribLocation(p, 1, "aUV");
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[256];
        glGetProgramInfoLog(p, 256, NULL, log);
        LOGE("tex program link 失败: %s", log);
        glDeleteProgram(p);
        return 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    *prog = p;
    if (aPos) *aPos = 0;
    if (aUV)  *aUV  = 1;
    if (uTex) *uTex = glGetUniformLocation(p, "uTex");
    return 1;
}

/* 纯色 program（选中高亮线，uColor 着色） */
static int rs_build_solid_prog(rs_glid_t *prog, rs_glid_t *aPos, rs_glid_t *uColor)
{
    rs_glid_t vs, fs;
    if (!rs_build_shader(&vs, GL_VERTEX_SHADER, RS_VS_SRC)) return 0;
    if (!rs_build_shader(&fs, GL_FRAGMENT_SHADER, RS_FS_SOLID_SRC)) {
        glDeleteShader(vs);
        return 0;
    }
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glBindAttribLocation(p, 0, "aPos");
    glBindAttribLocation(p, 1, "aUV");
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[256];
        glGetProgramInfoLog(p, 256, NULL, log);
        LOGE("solid program link 失败: %s", log);
        glDeleteProgram(p);
        return 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    *prog = p;
    if (aPos) *aPos = 0;
    if (uColor) *uColor = glGetUniformLocation(p, "uColor");
    return 1;
}

/* 顶点布局：xy+st interleaved，attribute0=xy(2f)@0，attribute1=st(2f)@8 */
static void rs_bind_vbo(rs_glid_t vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (const void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (const void *)8);
}

static void rs_unbind_vbo(void)
{
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

static int rs_make_vbo(rs_glid_t *vbo, const rs_v4f *verts, int n)
{
    glGenBuffers(1, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(n * sizeof(rs_v4f)), verts, GL_STATIC_DRAW);
    return *vbo != 0;
}

/* ---------------- 纹理加载 ---------------- */
static void rs_read_png(AAssetManager *am, const char *asset_path,
                        const char *fs_path, unsigned char **out, size_t *out_len)
{
    /* ★ 文件系统优先（/data/local/tmp/spak_res 是稳定路径，免 AAssetManager 内部
       pthread_mutex_lock 竞争——真机 libandroid 的 AAssetManager_open 在 render 线程
       与 AAsset 全局锁竞争 → SIGSEGV SI_KERNEL fault 0x0，实证崩溃栈 AAssetManager_open+54）。
       文件系统失败再试 assets。 */
    {
    FILE *f = fopen(fs_path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        unsigned char *buf = malloc(sz > 0 ? sz : 1);
        if (fread(buf, 1, sz > 0 ? sz : 1, f) == (size_t)(sz > 0 ? sz : 1)) {
            fclose(f);
            *out = buf; *out_len = sz;
            return;
        }
        free(buf);
        fclose(f);
    }
    }
    /* AAsset 直接链接（正常 so 已验证可 dlopen；免 dlopen/dlsym 引入 libdl VERNEED） */
    AAsset *a = NULL;
    if (am) a = AAssetManager_open(am, asset_path, AASSET_MODE_BUFFER);
    if (a) {
        int64_t sz = AAsset_getLength(a);
        unsigned char *buf = malloc(sz > 0 ? sz : 1);
        int64_t rd = AAsset_read(a, buf, sz > 0 ? (size_t)sz : 1);
        AAsset_close(a);
        if (rd == sz) { *out = buf; *out_len = sz; return; }
        free(buf);
    }
    FILE *f = fopen(fs_path, "rb");
    if (!f) { LOGI("纹理缺失（asset+fs 均无）%s / %s", asset_path, fs_path); return; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *buf = malloc(sz > 0 ? sz : 1);
    fread(buf, 1, sz > 0 ? sz : 1, f);
    fclose(f);
    *out = buf; *out_len = sz;
}

static void load_texture(rs_render *rr, AAssetManager *am,
                         const char *asset_path, const char *fs_path,
                         unsigned char **dst, size_t *dst_len, GLuint *tex,
                         int w, int h)
{
    unsigned char *buf = NULL;
    size_t sz = 0;
    rs_read_png(am, asset_path, fs_path, &buf, &sz);
    if (!buf) return;
    unsigned char *rgba = NULL;
    int dw = 0, dh = 0;
    if (png_decode(buf, sz, &rgba, &dw, &dh) == 0) {
        *dst = rgba; *dst_len = (size_t)w * h * 4;
        glGenTextures(1, tex);
        glBindTexture(GL_TEXTURE_2D, *tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        {
            GLenum e = glGetError();
            LOGI("纹理 %s %dx%d tex=%d GLERR=0x%x", asset_path, dw, dh, *tex, (unsigned)e);
        }
    }
    free(buf);
}

int rs_render_init(rs_render *rr, const char *spak_dir, const jq_state *init_state,
                   AAssetManager *am, EGLDisplay disp, EGLSurface surf, EGLContext ctx)
{
    memset(rr, 0, sizeof(*rr));
    rr->game = init_state;   /* 指针（非快照）：走子/翻棋后渲染自动更新 */
    rr->sel_cell = -1;
    rr->view_w = 720; rr->view_h = 1280;
    /* EGL 上下文复用：jq_glview_init 已建好 GL2 context（client version=2）。
       ★ 不在主线程 makeCurrent：PBuffer context 跨线程 makeCurrent 报 3002，
         且 GL 对象（shader/纹理/VBO）属于 context，主线程建的 GL 对象在
         GLThread 的 EglHelper context 里不存在 → 崩。
         故 GL 初始化（shader 编译 + 纹理上传 + VBO 创建）延迟到 GLThread
         首帧（rs_render_frame 里首次 eglMakeCurrent + 建对象，同 context 内可见）。 */
    rr->egl_disp = disp;
    rr->egl_surf = surf;
    rr->egl_ctx = ctx;
    rr->spak_dir = strdup(spak_dir ? spak_dir : "");
    rr->am = am;   /* 首帧建纹理用 */
    rr->gl2_ok[0] = 0; rr->gl2_ok[1] = 0;
    rr->gl_ok[0] = 0; rr->gl_ok[1] = 0;
    rr->gl_now_slot = 0; rr->gl_px_loaded = 0;
    rr->dirty = 1;
    /* 像素解码在主线程（Init）做（线程安全）：board/piece RGBA 跨 context 共享 */
    {
        char p1[512], p2[512], p3[512], p4[512], p5[512], p6[512], p7[512], p8[512];
        snprintf(p1, sizeof p1, "%s/datah_188_21d95e03e7692e98.bin", spak_dir ? spak_dir : "");
        snprintf(p2, sizeof p2, "%s/datah_183_9e1b7fbc4dfc09c3.bin", spak_dir ? spak_dir : "");
        snprintf(p3, sizeof p3, "%s/menu_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p4, sizeof p4, "%s/bg_menu_720.png", spak_dir ? spak_dir : "");
        snprintf(p5, sizeof p5, "%s/popup_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p6, sizeof p6, "%s/popup.png", spak_dir ? spak_dir : "");
        snprintf(p7, sizeof p7, "%s/menu_bg_294.png", spak_dir ? spak_dir : "");
        snprintf(p8, sizeof p8, "%s/imen_text_atlas.png", spak_dir ? spak_dir : "");
        char p9[512];
        snprintf(p9, sizeof p9, "%s/imen_icons_atlas.png", spak_dir ? spak_dir : "");
        char p10[512];
        snprintf(p10, sizeof p10, "%s/imen_hamburger_atlas.png", spak_dir ? spak_dir : "");
        char p11[512], p12[512];
        snprintf(p11, sizeof p11, "%s/menu_btn_big.png", spak_dir ? spak_dir : "");
        snprintf(p12, sizeof p12, "%s/menu_btn_small.png", spak_dir ? spak_dir : "");
        unsigned char *braw = NULL; size_t braw_len = 0;
        unsigned char *praw = NULL; size_t praw_len = 0;
        unsigned char *traw = NULL; size_t traw_len = 0;
        unsigned char *graw = NULL; size_t graw_len = 0;
        unsigned char *oraw = NULL; size_t oraw_len = 0;   /* 弹窗文字图集 */
        unsigned char *praw2 = NULL; size_t praw2_len = 0;  /* popup.png 9-patch */
        unsigned char *mraw = NULL; size_t mraw_len = 0;    /* ★ 菜单背景 datax_294 */
        unsigned char *iraw = NULL; size_t iraw_len = 0;    /* ★ ☰ 菜单文字图集 */
        unsigned char *icraw = NULL; size_t icraw_len = 0;  /* ★ ☰ 菜单图标图集 */
        unsigned char *hbraw = NULL; size_t hbraw_len = 0;  /* ★ ☰/↩ 图标图集 */
        unsigned char *bbraw = NULL; size_t bbraw_len = 0;  /* ★ 主菜单大按钮图集 */
        unsigned char *bsraw = NULL; size_t bsraw_len = 0;  /* ★ 主菜单小按钮图集 */
        unsigned char *bpx = NULL, *ppx = NULL, *tpx = NULL, *gpx = NULL;
        unsigned char *opx = NULL, *ppx2 = NULL, *mpx = NULL, *ipx = NULL, *icpx = NULL, *hbpx = NULL, *bbpx = NULL, *bspx = NULL;
        int bw = 0, bh = 0, pw = 0, ph = 0, tw = 0, th = 0, gw = 0, gh = 0;
        int ow = 0, oh = 0, pw2 = 0, ph2 = 0, mw = 0, mh = 0, iw = 0, ih = 0, icw = 0, ich = 0;
        int hbw = 0, hbh = 0, bbw = 0, bbh = 0, bsw = 0, bsh = 0;
        rs_read_png(am, "spak_res/datah_188_21d95e03e7692e98.bin", p1, &braw, &braw_len);
        rs_read_png(am, "spak_res/datah_183_9e1b7fbc4dfc09c3.bin", p2, &praw, &praw_len);
        rs_read_png(am, "spak_res/menu_text_atlas.png", p3, &traw, &traw_len);
        rs_read_png(am, "spak_res/bg_menu_720.png", p4, &graw, &graw_len);
        rs_read_png(am, "spak_res/popup_text_atlas.png", p5, &oraw, &oraw_len);
        rs_read_png(am, "spak_res/popup.png", p6, &praw2, &praw2_len);
        rs_read_png(am, "spak_res/menu_bg_294.png", p7, &mraw, &mraw_len);
        rs_read_png(am, "spak_res/imen_text_atlas.png", p8, &iraw, &iraw_len);
        rs_read_png(am, "spak_res/imen_icons_atlas.png", p9, &icraw, &icraw_len);
        rs_read_png(am, "spak_res/imen_hamburger_atlas.png", p10, &hbraw, &hbraw_len);
        rs_read_png(am, "spak_res/menu_btn_big.png", p11, &bbraw, &bbraw_len);
        rs_read_png(am, "spak_res/menu_btn_small.png", p12, &bsraw, &bsraw_len);
        if (braw && png_decode(braw, braw_len, &bpx, &bw, &bh) == 0) { rr->board_px = bpx; rr->board_px_len = (size_t)bw * bh * 4; }
        free(braw);
        if (praw && png_decode(praw, praw_len, &ppx, &pw, &ph) == 0) { rr->piece_px = ppx; rr->piece_px_len = (size_t)pw * ph * 4; }
        free(praw);
        /* ★ 层 2 视觉对齐：菜单按钮文字图集（原版字体 data_029 光栅化 5 label 竖排） */
        if (traw && png_decode(traw, traw_len, &tpx, &tw, &th) == 0) {
            rr->text_px = tpx; rr->text_px_len = (size_t)tw * th * 4;
            rr->text_w = tw; rr->text_h = th;
        }
        free(traw);
        /* ★ 层 2 视觉对齐：对局棋盘背景 datax_177（720×1280 原版完整棋盘背景） */
        if (graw && png_decode(graw, graw_len, &gpx, &gw, &gh) == 0) {
            rr->bg_px = gpx; rr->bg_px_len = (size_t)gw * gh * 4;
            rr->bg_w = gw; rr->bg_h = gh;
        }
        free(graw);
        /* ★ 原版 UI 对齐：主菜单背景 datax_294（800×1205 卡通士兵，与对局棋盘背景分离） */
        if (mraw && png_decode(mraw, mraw_len, &mpx, &mw, &mh) == 0) {
            rr->menu_bg_px = mpx; rr->menu_bg_px_len = (size_t)mw * mh * 4;
            rr->menu_bg_w = mw; rr->menu_bg_h = mh;
        }
        free(mraw);
        /* ★ 原版 UI 对齐：☰ 菜单文字图集（8 项竖排 160×320，data_029 字体光栅化） */
        if (iraw && png_decode(iraw, iraw_len, &ipx, &iw, &ih) == 0) {
            rr->imen_text_px = ipx; rr->imen_text_px_len = (size_t)iw * ih * 4;
            rr->imen_text_w = iw; rr->imen_text_h = ih;
        }
        free(iraw);
        /* ★ 原版 UI 对齐：☰ 菜单图标图集（8 图标 110×100 4×2，orig_ingame_menu 裁剪） */
        if (icraw && png_decode(icraw, icraw_len, &icpx, &icw, &ich) == 0) {
            rr->imen_icons_px = icpx; rr->imen_icons_px_len = (size_t)icw * ich * 4;
            rr->imen_icons_w = icw; rr->imen_icons_h = ich;
        }
        free(icraw);
        /* ★ 原版 UI 对齐：☰/↩ 图标图集（128×64，orig_human 裁剪，棋盘中部） */
        if (hbraw && png_decode(hbraw, hbraw_len, &hbpx, &hbw, &hbh) == 0) {
            rr->hamburger_px = hbpx; rr->hamburger_px_len = (size_t)hbw * hbh * 4;
            rr->hamburger_w = hbw; rr->hamburger_h = hbh;
        }
        free(hbraw);
        /* ★ 原版 UI 对齐：主菜单按钮图集（orig_menu 裁剪，原版蓝色圆角按钮含文字/图标） */
        if (bbraw && png_decode(bbraw, bbraw_len, &bbpx, &bbw, &bbh) == 0) {
            rr->btn_big_px = bbpx; rr->btn_big_px_len = (size_t)bbw * bbh * 4;
            rr->btn_big_w = bbw; rr->btn_big_h = bbh;
        }
        free(bbraw);
        if (bsraw && png_decode(bsraw, bsraw_len, &bspx, &bsw, &bsh) == 0) {
            rr->btn_small_px = bspx; rr->btn_small_px_len = (size_t)bsw * bsh * 4;
            rr->btn_small_w = bsw; rr->btn_small_h = bsh;
        }
        free(bsraw);
        /* ★ 选项 UI：弹窗文字图集（6 label 竖排 109×258） */
        if (oraw && png_decode(oraw, oraw_len, &opx, &ow, &oh) == 0) {
            rr->opt_text_px = opx; rr->opt_text_px_len = (size_t)ow * oh * 4;
            rr->opt_text_w = ow; rr->opt_text_h = oh;
        }
        free(oraw);
        /* ★ 选项 UI：popup.png 9-patch（24×24，9-patch 8,8,16,16） */
        if (praw2 && png_decode(praw2, praw2_len, &ppx2, &pw2, &ph2) == 0) {
            rr->popup_px = ppx2; rr->popup_px_len = (size_t)pw2 * ph2 * 4;
            rr->popup_w = pw2; rr->popup_h = ph2;
        }
        free(praw2);
        rr->gl_px_loaded = 1;
        LOGI("Init 像素解码：board=%s(%dx%d) piece=%s(%dx%d) text=%s(%dx%d) bg=%s(%dx%d) menu_bg=%s(%dx%d) popup=%s(%dx%d) opt_text=%s(%dx%d) imen_text=%s(%dx%d)",
             rr->board_px ? "OK" : "MISS", bw, bh, rr->piece_px ? "OK" : "MISS", pw, ph,
             rr->text_px ? "OK" : "MISS", tw, th, rr->bg_px ? "OK" : "MISS", gw, gh,
             rr->menu_bg_px ? "OK" : "MISS", mw, mh, rr->popup_px ? "OK" : "MISS", pw2, ph2,
             rr->opt_text_px ? "OK" : "MISS", ow, oh, rr->imen_text_px ? "OK" : "MISS", iw, ih,
             rr->imen_icons_px ? "OK" : "MISS", icw, ich, rr->hamburger_px ? "OK" : "MISS", hbw, hbh,
             rr->btn_big_px ? "OK" : "MISS", bbw, bbh, rr->btn_small_px ? "OK" : "MISS", bsw, bsh);
    }
    if (rr->egl_ctx)
        LOGI("render 初始化：GL2 context 就绪，GL 对象延迟到 GLThread 首帧建");
    else
        LOGI("render 初始化：EGL 上下文未传入（无 GPU 兜底），渲染退化为清屏验证");
    return 0;
}

/* GLThread 首帧 GL 初始化（PBuffer context 首次 makeCurrent 在本线程，
   之后 shader/纹理/VBO 都在同 context 内建，同 context 内对象互相可见）。 */
static void rs_gl_first_init(rs_render *rr)
{
    /* 纹理上传已归 rs_gl_rebuild（按 slot，每 context 一套）；本函数仅在
       像素未解码时兜底（正常路径 rs_render_init 主线程已解码，gl_px_loaded=1）。 */
}

/* 菜单/按钮 渲染（正向开发 UI 层；纯色 program 实心矩形，720×1280 基准按
   view_w/view_h 缩放——GLView 实际尺寸可能 <1280 高，按钮按屏幕比例映射） */
static void rs_draw_overlay(rs_render *rr, int slot, int winner)
{
    /* ★ 对局结束胜负横幅（stage==OVER 时画中心横幅，winner 色）+ "再来一局"按钮。
       复刻 rs_draw_menu 纯色 quad 通路（prog_solid + uColor + VBO glDrawArrays）。
       诚实边界：MuMu GLES1 固定管线几何出帧受限（§124zz 三条路径 fail），
       横幅/按钮像素出屏需真机 GPU 设备验收；音效 win/lose 已真机闭环。 */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w, sy = (float)rr->view_h;
    /* 胜负横幅（中心 20%-80% x，42%-58% y） */
    float x0 = 0.20f * sx, x1 = 0.80f * sx, y0 = 0.42f * sy, y1 = 0.58f * sy;
    rs_v4f quad[6] = {
        {x0, y0, 0, 0}, {x1, y0, 0, 0}, {x0, y1, 0, 0},
        {x0, y1, 0, 0}, {x1, y0, 0, 0}, {x1, y1, 0, 0}
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], sx, sy);
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* winner==BLUE → 蓝横幅；RED → 红横幅；2（和棋）→ 灰横幅 */
    if (winner == SIDE_BLUE)      glUniform4f(uc, 0.15f, 0.35f, 0.85f, 0.9f);
    else if (winner == SIDE_RED)  glUniform4f(uc, 0.85f, 0.20f, 0.20f, 0.9f);
    else                          glUniform4f(uc, 0.55f, 0.55f, 0.55f, 0.9f);   /* 和棋灰 */
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* "再来一局"按钮（横幅下方 35%-65% x，62%-72% y，绿色区分）。坐标与
       jni_bridge OnTouch 对局结束按钮一致（nx 0.35-0.65 / ny 0.62-0.72）。 */
    float bx0 = 0.35f * sx, bx1 = 0.65f * sx, by0 = 0.62f * sy, by1 = 0.72f * sy;
    rs_v4f bq[6] = {
        {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
        {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0}
    };
    glUniform4f(uc, 0.20f, 0.75f, 0.30f, 0.9f);   /* 绿色"再来一局"按钮 */
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    static int od = 0;
    if (!od) { od = 1; LOGI("rs_draw_overlay：胜负横幅（winner=%s）+ 再来一局按钮",
        winner == SIDE_BLUE ? "BLUE" : winner == SIDE_RED ? "RED" : "DRAW"); }
}

static void rs_draw_menu(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) {
        static int md = 0;
        if (!md) { LOGE("rs_draw_menu 跳过：prog_solid=%u gl2_ok=%d", (unsigned)rr->prog_solid[slot], rr->gl2_ok[slot]); md = 1; }
        return;
    }
    const ui_btn *btns = ui_menu_buttons();
    /* 按钮几何 720×1280 基准 → 按实际 view 缩放 */
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    static int menu_diag_done = 0;
    /* ① 全屏背景（★ 原版 UI 对齐：主菜单背景 datax_294 卡通士兵 800×1205；
       缺失退化深蓝底） */
    if (rr->tex_menu_bg[slot] && rr->prog_tex[slot]) {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f quad[6] = {
            {0, 0, 0, 0}, {w, 0, 1, 0}, {0, h, 0, 1},
            {0, h, 0, 1}, {w, 0, 1, 0}, {w, h, 1, 1},
        };
        glUseProgram(rr->prog_tex[slot]);
        glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rr->tex_menu_bg[slot]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    } else {
        rs_v4f quad[6] = {
            {0, 0, 0, 0}, {(float)rr->view_w, 0, 0, 0}, {0, (float)rr->view_h, 0, 0},
            {0, (float)rr->view_h, 0, 0}, {(float)rr->view_w, 0, 0, 0},
            {(float)rr->view_w, (float)rr->view_h, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
        glUniform4f(uc, 0.10f, 0.15f, 0.30f, 0.85f);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ★ 菜单按钮文字图集元数据（原版字体 data_029 光栅化，图集 136×214 竖排）：
       退化路径（按钮位图缺失时）用纯色矩形+文字；正常路径用 orig_menu 裁剪位图。 */
    static const int T_W[5] = {136, 136, 66, 68, 134};
    static const int T_X[5] = {0, 0, 35, 34, 1};
    static const int T_Y[5] = {0, 44, 88, 132, 176};
    const int T_H = 38;
    /* ★ 原版 UI 对齐：按钮位图（orig_menu 裁剪）——人机/两人用 big atlas
       （802×118 两槽），联网/联机/四国用 small atlas（666×116 三槽，四国窄条
       x 485-624 居中）。按钮矩形 g_btns（720 基准）不变，位图替换纯色矩形+文字。 */
    for (int i = 0; i < UI_BTN_COUNT; i++) {
        float x0 = btns[i].x0 * sx, x1 = btns[i].x1 * sx;
        float y0 = btns[i].y0 * sy, y1 = btns[i].y1 * sy;
        if (!menu_diag_done)
            LOGI("rs_draw_menu 首帧：btn%d 顶点 x0=%.1f y0=%.1f x1=%.1f y1=%.1f（view=%dx%d）",
                 i, x0, y0, x1, y1, rr->view_w, rr->view_h);
        /* 位图路径：选择 atlas + UV 子矩形 */
        rs_glid_t btex = 0; float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
        if (i < 2) {
            if (rr->tex_btn_big[slot]) {
                btex = rr->tex_btn_big[slot];
                u0 = i == 0 ? 0.0f : 0.5f; u1 = u0 + 0.5f; v0 = 0; v1 = 1;
            }
        } else {
            if (rr->tex_btn_small[slot]) {
                btex = rr->tex_btn_small[slot];
                if (i == 2) { u0 = 0; u1 = 1.0f/3.0f; }
                else if (i == 3) { u0 = 1.0f/3.0f; u1 = 2.0f/3.0f; }
                else { u0 = 485.0f/666.0f; u1 = 624.0f/666.0f; }  /* 四国窄条 */
                v0 = 0; v1 = 1;
            }
        }
        if (btex && rr->prog_tex[slot]) {
            /* 按钮位图渲染（原版蓝色圆角按钮 + 文字/图标） */
            rs_v4f quad[6] = {
                {x0, y0, u0, v0}, {x1, y0, u1, v0}, {x0, y1, u0, v1},
                {x0, y1, u0, v1}, {x1, y0, u1, v0}, {x1, y1, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, btex);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
            continue;   /* 位图已含文字，跳过纯色+文字退化 */
        }
        rs_v4f quad[6] = {
            {x0, y0, 0, 0}, {x1, y0, 0, 0}, {x0, y1, 0, 0},
            {x0, y1, 0, 0}, {x1, y0, 0, 0}, {x1, y1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
        glUniform4f(uc, 0.22f, 0.28f, 0.42f, 0.85f);   /* 按钮深蓝底（退化） */
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
        /* ★ 层 2 视觉对齐：按钮文字（原版字体图集，白字，按钮内居中） */
        if (rr->tex_text[slot] && rr->prog_tex[slot]) {
            float tw = T_W[i] * sx, th = T_H * sy;   /* 文字屏幕尺寸（720 基准缩放） */
            float tx0 = x0 + ((x1 - x0) - tw) * 0.5f;
            float ty0 = y0 + ((y1 - y0) - th) * 0.5f;
            float u0 = (float)T_X[i] / rr->text_w, u1 = (float)(T_X[i] + T_W[i]) / rr->text_w;
            float v0 = (float)T_Y[i] / rr->text_h, v1 = (float)(T_Y[i] + T_H) / rr->text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_text[slot]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
            if (!menu_diag_done) {
                menu_diag_done = 1;
                LOGI("rs_draw_menu 文字图集：tex_text=%u (%dx%d) btn%d 文字 (%.1f,%.1f)-(%.1f,%.1f)",
                     (unsigned)rr->tex_text[slot], rr->text_w, rr->text_h, i,
                     tx0, ty0, tx0 + tw, ty0 + th);
            }
        }
    }
}


/* 可走格高亮（选中格后 jq_can_move 合法目标格画绿色半透框 + 点；1 次顶点池
   glDrawArrays，与选中黄框同纯色 program）。 */
static void rs_draw_highlights(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    int nhl = 0;
    if (rr->sel_cell >= 0) {
        for (int t = 0; t < N_CELLS; t++)
            if (t != rr->sel_cell && rr->game && jq_can_move(rr->game, rr->sel_cell, t))
                rr->hl_cells[nhl++] = t;
    }
    rr->hl_n = nhl;
    if (nhl == 0) return;
    /* ★ 层 2：可走格高亮与棋子同几何（datax_177 BG_GRID，aPos.y=0=屏幕顶） */
    float vw = (float)rr->view_w, vh = (float)rr->view_h;
    float cell_w = BG_GRID_COL_W / BG_IMG_W * vw;
    float cell_h = BG_GRID_ROW_H / BG_IMG_H * vh;
    static rs_v4f hl_pool[64 * 8];
    int nverts = 0;
    for (int k = 0; k < nhl; k++) {
        int i = rr->hl_cells[k];
        int cr = cell_row(i), cc = cell_col(i);
        float ix, iy; bg_cell_img_center(cr, cc, &ix, &iy);
        float x0 = (ix - BG_GRID_COL_W * 0.5f) / BG_IMG_W * vw;
        float y0 = (iy - BG_GRID_ROW_H * 0.5f) / BG_IMG_H * vh;
        /* 目标格绿框 8 顶点（4 边线） */
        hl_pool[nverts++] = (rs_v4f){x0, y0, 0, 0};
        hl_pool[nverts++] = (rs_v4f){x0 + cell_w, y0, 0, 0};
        hl_pool[nverts++] = (rs_v4f){x0 + cell_w, y0, 0, 0};
        hl_pool[nverts++] = (rs_v4f){x0 + cell_w, y0 + cell_h, 0, 0};
        hl_pool[nverts++] = (rs_v4f){x0 + cell_w, y0 + cell_h, 0, 0};
        hl_pool[nverts++] = (rs_v4f){x0, y0 + cell_h, 0, 0};
        hl_pool[nverts++] = (rs_v4f){x0, y0 + cell_h, 0, 0};
        hl_pool[nverts++] = (rs_v4f){x0, y0, 0, 0};
    }
    if (nverts > 0) {
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
        glUniform4f(uc, 0.2f, 0.9f, 0.2f, 0.8f);   /* 绿色可走格 */
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(nverts * sizeof(rs_v4f)),
                     hl_pool, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_LINES, 0, nverts);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
}

/* ★ 原版 UI 对齐：☰（游戏内菜单）/↩（悔棋）图标画在棋盘中部。
   原版为浅蓝半透明圆角方块 + 白色标记（orig_human.png 棋盘中部
   ☰ 中心 (272,874)/↩ 中心 (625,873) 屏幕 900×1600，尺寸 ~52×47/55×57）。
   src_rs 用真实图标位图（imen_hamburger_atlas.png 128×64，orig_human 裁剪）
   替换旧纯色矩形近似；命中 ui_imen_icon_rect 同几何。 */
static void rs_draw_imen_icons(rs_render *rr, int slot)
{
    if (!rr->prog_tex[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    for (int which = 0; which < 2; which++) {
        float x0, y0, x1, y1;
        ui_imen_icon_rect(which, &x0, &y0, &x1, &y1);
        /* ★ 真实图标尺寸（720 基准）：☰ 42×38、↩ 44×46（原版实测 52×47/55×57 ×0.8）；
           以 icon_rect 中心对齐（命中区 70×70 含余量） */
        float icx = (x0 + x1) * 0.5f * sx, icy = (y0 + y1) * 0.5f * sy;
        float iw = (which == 0 ? 42.0f : 44.0f) * sx;
        float ih = (which == 0 ? 38.0f : 46.0f) * sy;
        float px0 = icx - iw * 0.5f, py0 = icy - ih * 0.5f, px1 = icx + iw * 0.5f, py1 = icy + ih * 0.5f;
        if (rr->tex_hamburger[slot] && rr->hamburger_w > 0 && rr->hamburger_h > 0) {
            /* 真实图标位图（atlas 128×64：☰ 左半、↩ 右半） */
            float u0 = which == 0 ? 0.0f : 0.5f, u1 = u0 + 0.5f;
            rs_v4f quad[6] = {
                {px0, py0, u0, 0}, {px1, py0, u1, 0}, {px0, py1, u0, 1},
                {px0, py1, u0, 1}, {px1, py0, u1, 0}, {px1, py1, u1, 1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_hamburger[slot]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
    }
    static int idiag = 0;
    if (!idiag) { idiag = 1; LOGI("rs_draw_imen_icons：☰/↩ 真实图标位图（orig_human 裁剪 128×64）"); }
}

/* ★ 原版 UI 对齐：☰ 游戏内菜单（datah_006 8 项）渲染。
   深色半透明面板横跨棋盘中部 + 8 项（图标近似纯色 + 文字图集）。
   popup 450×285 center（720×1280 基准），8 按钮 ui_imen_buttons()。 */
static void rs_draw_imen(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    /* ① 全屏模态遮罩（半透明黑，聚焦菜单） */
    rs_v4f mq[6] = {
        {0, 0, 0, 0}, {(float)rr->view_w, 0, 0, 0}, {0, (float)rr->view_h, 0, 0},
        {0, (float)rr->view_h, 0, 0}, {(float)rr->view_w, 0, 0, 0},
        {(float)rr->view_w, (float)rr->view_h, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    glUniform4f(uc, 0.0f, 0.0f, 0.0f, 0.35f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mq), mq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ② 面板：深色半透明矩形（popup 450×285 center 720 基准） */
    float px0 = UI_IMENU_POPUP_X0 * sx, py0 = UI_IMENU_POPUP_Y0 * sy;
    float pw = UI_IMENU_POPUP_W * sx, ph = UI_IMENU_POPUP_H * sy;
    rs_v4f pq[6] = {
        {px0, py0, 0, 0}, {px0 + pw, py0, 0, 0}, {px0, py0 + ph, 0, 0},
        {px0, py0 + ph, 0, 0}, {px0 + pw, py0, 0, 0}, {px0 + pw, py0 + ph, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.08f, 0.08f, 0.10f, 0.75f);   /* 深色半透明面板 */
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ③ 8 项：按钮图标（纯色近似：黄星/绿回/蓝箭/白纸/扳手/彩饼/蓝i/红X）+ 文字 */
    static const float ICON_COLOR[UI_IMENU_COUNT][4] = {
        {1.0f, 0.9f, 0.2f, 0.95f},   /* 重新开始 黄星 */
        {0.3f, 0.85f, 0.4f, 0.95f},  /* 悔棋 绿回 */
        {0.35f, 0.6f, 1.0f, 0.95f},  /* 电脑变着 蓝箭 */
        {0.95f, 0.95f, 0.95f, 0.95f},/* 游戏规则 白纸 */
        {0.8f, 0.8f, 0.85f, 0.95f},  /* 功能设置 扳手 */
        {0.95f, 0.5f, 0.3f, 0.95f},  /* 战况统计 彩饼 */
        {0.3f, 0.7f, 1.0f, 0.95f},   /* 软件信息 蓝i */
        {0.95f, 0.25f, 0.25f, 0.95f},/* 退出程序 红X */
    };
    const ui_imen_btn *im = ui_imen_buttons();
    /* ★ 原版 UI 对齐：③ 图标用真实位图（imen_icons_atlas.png 440×200，8 图标
       110×100 4×2，orig_ingame_menu 裁剪）替换纯色块近似。图标 88×80（720 基准）
       在按钮内 0.30 处，中心 = 按钮中心 x；UV = 第 i 图标子矩形。 */
    for (int i = 0; i < UI_IMENU_COUNT; i++) {
        float bx0 = im[i].x0 * sx, by0 = im[i].y0 * sy;
        float bx1 = im[i].x1 * sx, by1 = im[i].y1 * sy;
        float icx = (bx0 + bx1) * 0.5f;
        float icy = by0 + (by1 - by0) * 0.30f;
        float iw = 88.0f * sx, ih = 80.0f * sy;
        float icx0 = icx - iw * 0.5f, icx1 = icx + iw * 0.5f;
        float icy0 = icy - ih * 0.5f, icy1 = icy + ih * 0.5f;
        if (rr->tex_imen_icons[slot] && rr->prog_tex[slot] && rr->imen_icons_w > 0 && rr->imen_icons_h > 0) {
            /* 真实图标位图（UV 子矩形 = 第 i 图标，110×100 4×2） */
            float u0 = (float)(i % 4) / 4.0f, v0 = (float)(i / 4) / 2.0f;
            float u1 = u0 + 1.0f / 4.0f, v1 = v0 + 1.0f / 2.0f;
            rs_v4f iq[6] = {
                {icx0, icy0, u0, v0}, {icx1, icy0, u1, v0}, {icx0, icy1, u0, v1},
                {icx0, icy1, u0, v1}, {icx1, icy0, u1, v0}, {icx1, icy1, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_imen_icons[slot]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(iq), iq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        } else {
            /* 退化：纯色块近似（图标图集缺失时） */
            rs_v4f iq[6] = {
                {icx0, icy0, 0, 0}, {icx1, icy0, 0, 0}, {icx0, icy1, 0, 0},
                {icx0, icy1, 0, 0}, {icx1, icy0, 0, 0}, {icx1, icy1, 0, 0},
            };
            glUseProgram(rr->prog_solid[slot]);
            glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
            glUniform4f(uc, ICON_COLOR[i][0], ICON_COLOR[i][1], ICON_COLOR[i][2], ICON_COLOR[i][3]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(iq), iq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
    }
    /* ④ 8 项文字（imen_text_atlas 竖排，每项 160×40 源；★ 原版 UI 对齐（用户指正
       "重新开始/悔棋按钮小问题"）：文字过大超按钮宽——原版实测文字 bbox 157×37
       （900 屏）= 126×30（720 基准），非 160×40。中心在按钮内 0.90 处（实测行1
       文字中心 y=594、行2 y=780），中心 x = 按钮中心（与图标同列）。 */
    if (rr->tex_imen_text[slot] && rr->prog_tex[slot] && rr->imen_text_w > 0 && rr->imen_text_h > 0) {
        for (int i = 0; i < UI_IMENU_COUNT; i++) {
            float bx0 = im[i].x0 * sx, by0 = im[i].y0 * sy;
            float bx1 = im[i].x1 * sx, by1 = im[i].y1 * sy;
            float tcx = (bx0 + bx1) * 0.5f;
            float tcy = by0 + (by1 - by0) * 0.90f;
            float tw = 134.0f * sx, th = tw * (40.0f / 160.0f);   /* 原版实测：字形占图集 94% → tw=134 → 字形 126px（720 基准）=157px（900 屏），与原版 bbox 一致 */
            float tx0 = tcx - tw * 0.5f, ty0 = tcy - th * 0.5f;
            float u0 = 0.0f, u1 = 1.0f;   /* 整源宽（文字 160px 宽图集，全宽） */
            float v0 = (float)(i * 40) / rr->imen_text_h;
            float v1 = (float)(i * 40 + 40) / rr->imen_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_imen_text[slot]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
    }
    static int udiag = 0;
    if (!udiag) { udiag = 1; LOGI("rs_draw_imen：☰ 菜单 8 项已画（原版实测图标中心 4 列/2 行，面板 %.0f×%.0f）", UI_IMENU_POPUP_W, UI_IMENU_POPUP_H); }
}

/* 判当前是否在 GLThread/EglHelper 的 window context（≠ 我们 init 的 PBuffer context） */
static int now_is_glthread_ctx(rs_render *rr)
{
    EGLContext now = eglGetCurrentContext();
    return (rr->egl_ctx != EGL_NO_CONTEXT && now != EGL_NO_CONTEXT && now != rr->egl_ctx);
}

/* 每 context 重建 GL 对象（纹理/VBO/shader 属当前 context；像素 buffer 共享只上传一次） */
static void rs_gl_rebuild(rs_render *rr, int slot)
{
    /* GLSL program（当前 context） */
    int ok_t = rs_build_tex_prog(&rr->prog_tex[slot], NULL, NULL, NULL);
    int ok_s = rs_build_solid_prog(&rr->prog_solid[slot], NULL, NULL);
    rr->gl2_ok[slot] = (ok_t && ok_s);
    /* uViewWH：像素→NDC 换算（tex/solid 共用同一顶点 shader） */
    if (rr->prog_tex[slot])
        rr->uViewWH_tex[slot] = glGetUniformLocation(rr->prog_tex[slot], "uViewWH");
    if (rr->prog_solid[slot])
        rr->uViewWH_solid[slot] = glGetUniformLocation(rr->prog_solid[slot], "uViewWH");
    if (rr->gl2_ok[slot])
        LOGI("GL slot%d：GLSL program 编译成功（GLES2 几何出帧就绪）", slot);
    else
        LOGE("GL slot%d：GLSL program 编译失败 → 几何不出帧（退化为清屏色）", slot);
    /* 纹理（每 context 上传，共享像素） */
    if (rr->board_px && rr->gl2_ok[slot]) {
        int bw = (int)(rr->board_px_len > 0 ? rr->board_px_len / 4 / BOARD_PNG_H : BOARD_PNG_W);
        if (bw <= 0) bw = BOARD_PNG_W;
        glGenTextures(1, &rr->tex_board[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_board[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bw, BOARD_PNG_H, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->board_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    if (rr->piece_px && rr->gl2_ok[slot]) {
        glGenTextures(1, &rr->tex_piece[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_piece[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PIECE_PNG_W, PIECE_PNG_H, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->piece_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ 层 2 视觉对齐：菜单按钮文字图集（原版字体 data_029 光栅化） */
    if (rr->text_px && rr->gl2_ok[slot] && rr->text_w > 0 && rr->text_h > 0) {
        glGenTextures(1, &rr->tex_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->text_w, rr->text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ 层 2 视觉对齐：对局棋盘背景 datax_177（720×1280 完整棋盘背景） */
    if (rr->bg_px && rr->gl2_ok[slot] && rr->bg_w > 0 && rr->bg_h > 0) {
        glGenTextures(1, &rr->tex_bg[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_bg[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->bg_w, rr->bg_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->bg_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ 原版 UI 对齐：主菜单背景 datax_294（800×1205 卡通士兵） */
    if (rr->menu_bg_px && rr->gl2_ok[slot] && rr->menu_bg_w > 0 && rr->menu_bg_h > 0) {
        glGenTextures(1, &rr->tex_menu_bg[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_menu_bg[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->menu_bg_w, rr->menu_bg_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->menu_bg_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ 原版 UI 对齐：☰ 菜单文字图集（8 项竖排 160×320） */
    if (rr->imen_text_px && rr->gl2_ok[slot] && rr->imen_text_w > 0 && rr->imen_text_h > 0) {
        glGenTextures(1, &rr->tex_imen_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_imen_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->imen_text_w, rr->imen_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->imen_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ 原版 UI 对齐：☰ 菜单图标图集（8 图标 110×100 4×2，原版裁剪） */
    if (rr->imen_icons_px && rr->gl2_ok[slot] && rr->imen_icons_w > 0 && rr->imen_icons_h > 0) {
        glGenTextures(1, &rr->tex_imen_icons[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_imen_icons[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->imen_icons_w, rr->imen_icons_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->imen_icons_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ 原版 UI 对齐：☰/↩ 图标图集（128×64，orig_human 裁剪） */
    if (rr->hamburger_px && rr->gl2_ok[slot] && rr->hamburger_w > 0 && rr->hamburger_h > 0) {
        glGenTextures(1, &rr->tex_hamburger[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_hamburger[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->hamburger_w, rr->hamburger_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->hamburger_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ 原版 UI 对齐：主菜单按钮图集（orig_menu 裁剪） */
    if (rr->btn_big_px && rr->gl2_ok[slot] && rr->btn_big_w > 0 && rr->btn_big_h > 0) {
        glGenTextures(1, &rr->tex_btn_big[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_btn_big[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->btn_big_w, rr->btn_big_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->btn_big_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    if (rr->btn_small_px && rr->gl2_ok[slot] && rr->btn_small_w > 0 && rr->btn_small_h > 0) {
        glGenTextures(1, &rr->tex_btn_small[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_btn_small[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->btn_small_w, rr->btn_small_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->btn_small_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ 选项 UI：popup.png 9-patch 弹窗背景（24×24，9-patch 8,8,16,16） */
    if (rr->popup_px && rr->gl2_ok[slot] && rr->popup_w > 0 && rr->popup_h > 0) {
        glGenTextures(1, &rr->tex_popup[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_popup[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->popup_w, rr->popup_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->popup_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ 选项 UI：弹窗文字图集（6 label 竖排 109×258） */
    if (rr->opt_text_px && rr->gl2_ok[slot] && rr->opt_text_w > 0 && rr->opt_text_h > 0) {
        glGenTextures(1, &rr->tex_opt_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_opt_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->opt_text_w, rr->opt_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->opt_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* 棋盘 VBO */
    {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f board[6] = {
            {0, 0, BOARD_UV_X0, 0},  {w, 0, BOARD_UV_X1, 0},
            {0, h, BOARD_UV_X0, 1},  {0, h, BOARD_UV_X0, 1},
            {w, 0, BOARD_UV_X1, 0},  {w, h, BOARD_UV_X1, 1},
        };
        rs_make_vbo(&rr->vbo_board[slot], board, 6);
    }
    rr->gl_ok[slot] = rr->gl2_ok[slot] && rr->vbo_board[slot] && rr->tex_board[slot] && rr->tex_piece[slot];
}

void rs_render_frame(rs_render *rr)
{
    if (!rr->egl_ctx) return;
    /* 像素解码（跨 context 共享，只解一次）：纹理上传/几何对象在 rs_gl_rebuild 按 slot 建 */
    if (!rr->gl_px_loaded)
        rs_gl_first_init(rr);
    /* ★ 多 context 支持（正向出屏：OnUpdate 在 GLThread/EglHelper window context 调 rs_render_frame，
       而 GL 对象属于建它的 context）：context 切换时重建 GL 对象（纹理/VBO/shader 在当前 context 重传）。
       判定：eglGetCurrentContext() != rr->gl_now_ctx → 重建。
       ★ 真机 Adreno 实证：切回旧 context 删旧对象失败（eglMakeCurrent 返回 EGL_FALSE）→
       旧对象 ID 挂掉后在新 context 复用 → glDeleteProgram/glDrawArrays 崩（fault 0xf3）。
       故切 context 时【不删旧 context 的 GL 对象】——纹理/VBO/shader 泄漏随 context 回收，
       本场景 context 对数有限（PBuffer + GLThread window 两个），泄漏可接受。 */
    /* 多 context 支持（正向出屏：OnUpdate 在 GLThread/EglHelper window context 调 rs_render_frame，
       PBuffer 渲染线程在自己的 PBuffer context 调）：每 context 一套 GL 对象（纹理/VBO/shader
       属各自 context，EGL 规范 context 私有），避免跨 context 重建（真机 Adreno 崩因）。
       共享像素 buffer 跨 context 各自 glTexImage2D 上传（纹理是每 context 对象）。 */
    int slot = 0;   /* slot0 = PBuffer context（首 init），slot1 = 第二 context（GLThread window） */
    if (rr->egl_ctx && now_is_glthread_ctx(rr)) slot = 1;
    if (rr->gl_now_slot != slot) {
        rs_gl_rebuild(rr, slot);
        rr->gl_now_slot = slot;
        LOGI("rs_render_frame：切到 GL slot%d（context 对象已建）gl_ok=%d", slot, rr->gl_ok[slot]);
    }
    (void)slot;
    /* GL2 context 由 jq_glview_init 建（client version=2）；EglHelper 跨线程 makeCurrent
       在 mesa 下受限，本函数只在 GLThread 内调（jq_glview_frame → OnUpdate）。 */
    glViewport(0, 0, rr->view_w, rr->view_h);   /* 实际 view（720×1024）；PBuffer backing 720×1280，
        超出 view_h 的下 256 行未被画 → 清屏色（正常，view 比 backing 矮） */
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* 棋盘背景（★ 层 2 视觉对齐：原版 datax_177 完整棋盘背景 720×1280 全图铺满；
       旧版 datah_188 UV 0.366-0.631 裁剪铺满（拉伸变形+无标题）） */
    if (rr->tex_bg[slot] && rr->gl2_ok[slot]) {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f bq[6] = {
            {0, 0, 0, 0}, {w, 0, 1, 0}, {0, h, 0, 1},
            {0, h, 0, 1}, {w, 0, 1, 0}, {w, h, 1, 1},
        };
        glUseProgram(rr->prog_tex[slot]);
        glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rr->tex_bg[slot]);
        glGenBuffers(1, &rr->vbo_board[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_board[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_board[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_board[slot]);
        rr->vbo_board[slot] = 0;
    } else if (rr->tex_board[slot] && rr->vbo_board[slot] && rr->gl2_ok[slot]) {
        /* 退化：datah_188 裁剪铺满（bg 缺失） */
        glUseProgram(rr->prog_tex[slot]);
        glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rr->tex_board[slot]);
        rs_bind_vbo(rr->vbo_board[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
    }
    /* 棋子（60 格，revealed 子画到棋盘格上）：顶点池一次分配 + 单次 glDrawArrays
       （去每棋子 glGenBuffers/glDeleteBuffers——60fps × 60 格 = 3600 次/秒 buffer 开关，
       真机 GPU 会卡；改 1 次 glBufferData + 1 次 glDrawArrays） */
    if (rr->tex_piece[slot] && rr->gl2_ok[slot]) {
        /* ★ 层 2 视觉对齐：棋子画在 datax_177 棋盘格上（bg 全图铺满）。
           cell 中心/尺寸按图像棋盘格定位（image_row=11-设备row，row0=底）。
           ★ 顶点 shader ndc.y=-ndc.y → aPos.y=0=屏幕顶、view_h=屏幕底
           （旧注释"底=0"是错的，曾致棋子翻侧）。bg 正向 v=0=图顶 →
           图顶(iy=0)→aPos.y=0→屏幕顶，图底(iy=1280)→aPos.y=view_h→屏幕底。 */
        float vw = (float)rr->view_w, vh = (float)rr->view_h;
        float cell_w = BG_GRID_COL_W / BG_IMG_W * vw;
        float cell_h = BG_GRID_ROW_H / BG_IMG_H * vh;
        /* ★ 原版 UI 对齐（用户指正"棋子间隙"）：棋子不铺满格——原版实测
           （orig_human.png 900×1600）：棋子宽 103px/列距 175px=0.5886，
           棋子高 71px/行距 87px=0.816。用 0.59×0.82 比例，以格中心对齐
           → 横向间隙 = 格宽×(1-0.59)、纵向间隙 = 格高×(1-0.82)，对齐原版
           间隙比例（横向 41%、纵向 18%）。 */
        float piece_w = cell_w * 0.59f, piece_h = cell_h * 0.82f;
        static int piece_diag_done = 0;
        static rs_v4f piece_pool[N_CELLS * 6];
        static rs_v4f sel_line[8];
        int nverts = 0;
        for (int i = 0; i < N_CELLS; i++) {
            const jq_cell *c = &rr->game->cells[i];
            if (c->rank == RANK_EMPTY) continue;
            int rank = rank_norm(c->rank);
            if (rank < 1 || rank > 12) continue;
            int col = (rank - 1) % PIECE_GRID_W;
            int row = (rank - 1) / PIECE_GRID_W;
            if (c->side == SIDE_BLUE) row += 3;
            /* 暗棋未翻 → 画背面（row6），不暴露 rank；已翻 → 按 rank 映射正面 */
            if (!c->revealed) { row = PIECE_GRID_H - 1; col = 0; }
            int cr = cell_row(i), cc = cell_col(i);
            float ix, iy; bg_cell_img_center(cr, cc, &ix, &iy);
            /* bg_cell_img_center 返回格心 → 棋子 quad 以格左上为原点
               （aPos.y=0=屏幕顶，shader ndc.y=-ndc.y；bg 正向 v=0=图顶，
               图像 iy 直接映射 aPos.y=iy/BG_IMG_H*view_h，图底→屏幕底）
               ★ 棋子缩到 piece_w×piece_h（格中心对齐，留间隙对齐原版） */
            float x0 = (ix - BG_GRID_COL_W * 0.5f) / BG_IMG_W * vw + (cell_w - piece_w) * 0.5f;
            float y0 = (iy - BG_GRID_ROW_H * 0.5f) / BG_IMG_H * vh + (cell_h - piece_h) * 0.5f;
            float tw = 1.0f / PIECE_GRID_W, th = 1.0f / PIECE_GRID_H;
            float u0 = col * tw, v0 = row * th;
            piece_pool[nverts++] = (rs_v4f){x0, y0, u0, v0};
            piece_pool[nverts++] = (rs_v4f){x0 + piece_w, y0, u0 + tw, v0};
            piece_pool[nverts++] = (rs_v4f){x0, y0 + piece_h, u0, v0 + th};
            piece_pool[nverts++] = (rs_v4f){x0, y0 + piece_h, u0, v0 + th};
            piece_pool[nverts++] = (rs_v4f){x0 + piece_w, y0, u0 + tw, v0};
            piece_pool[nverts++] = (rs_v4f){x0 + piece_w, y0 + piece_h, u0 + tw, v0 + th};
            /* 选中格高亮线（纯色 program，单格 8 顶点；★ 包住棋子尺寸而非全格） */
            if (i == rr->sel_cell) {
                sel_line[0] = (rs_v4f){x0, y0, 0, 0};
                sel_line[1] = (rs_v4f){x0 + piece_w, y0, 0, 0};
                sel_line[2] = (rs_v4f){x0 + piece_w, y0, 0, 0};
                sel_line[3] = (rs_v4f){x0 + piece_w, y0 + piece_h, 0, 0};
                sel_line[4] = (rs_v4f){x0 + piece_w, y0 + piece_h, 0, 0};
                sel_line[5] = (rs_v4f){x0, y0 + piece_h, 0, 0};
                sel_line[6] = (rs_v4f){x0, y0 + piece_h, 0, 0};
                sel_line[7] = (rs_v4f){x0, y0, 0, 0};
            }
        }
        nverts &= ~2; nverts -= (nverts % 3);   /* ★ 取 3 的倍数（旧 &= ~2 只取偶数 → Mesa swrast 静默丢几何，实证修复） */
        if (nverts > 0) {
            if (!piece_diag_done) {
                LOGI("棋子首帧：nverts=%d vbo 前 glDrawArrays(GL_TRIANGLES,0,%d) 首个顶点 x0=%.1f y0=%.1f u0=%.3f v0=%.3f",
                     nverts, nverts, piece_pool[0].x, piece_pool[0].y, piece_pool[0].u, piece_pool[0].v);
                piece_diag_done = 1;
            }
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_piece[slot]);
            glGenBuffers(1, &rr->vbo_piece[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_piece[slot]);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(nverts * sizeof(rs_v4f)),
                         piece_pool, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_piece[slot]);
            glDrawArrays(GL_TRIANGLES, 0, nverts);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_piece[slot]);
            rr->vbo_piece[slot] = 0;
        }
        /* 选中高亮线（纯色） */
        if (rr->sel_cell >= 0) {
            glUseProgram(rr->prog_solid[slot]);
            glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
            glUniform4f(uc, 1.0f, 1.0f, 0.0f, 0.5f);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(sel_line), sel_line, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_LINES, 0, 8);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
        /* 可走格绿色高亮（选中格后 jq_can_move 合法目标格，正向 UI 层） */
        rs_draw_highlights(rr, slot);
        /* ★ 原版 UI 对齐：对局态画 ☰/↩ 图标（棋盘中部）；☰ 菜单态叠 datah_006 面板 */
        if (ui_screen_get() == UI_GAME || ui_screen_get() == UI_INGAME_MENU) {
            rs_draw_imen_icons(rr, slot);
        }
        if (ui_screen_get() == UI_INGAME_MENU) {
            rs_draw_imen(rr, slot);
        }
        /* 主菜单（UI_MENU 态：卡通士兵背景 + 5 按钮） */
        {
            static int menu_log = 0;
            if (!menu_log) {
                LOGI("rs_render_frame：ui_screen=%d → %s",
                     ui_screen_get(),
                     ui_screen_get() == UI_MENU ? "画菜单按钮" :
                     ui_screen_get() == UI_INGAME_MENU ? "画☰菜单" : "不画菜单");
                menu_log = 1;
            }
        }
        /* ★ 原版 UI 对齐：仅 UI_MENU 画主菜单（datax_294 背景 + 实测按钮）；
           datah_041 难度弹窗已移除（原版人机/两人直开局无弹窗） */
        if (ui_screen_get() == UI_MENU) {
            rs_draw_menu(rr, slot);
            /* 诊断：rs_draw_menu 后 glGetError + 按钮区 readPixels */
            static int mdiag2 = 0;
            if (!mdiag2) {
                mdiag2 = 1;
                GLenum e = glGetError();
                unsigned char px[4] = {0};
                glReadPixels((int)(205 * rr->view_w / 720.0f), (int)(rr->view_h - 560 * rr->view_h / 1280.0f),
                             1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
                LOGI("rs_draw_menu 后 GLERR=0x%x 按钮0区(205,560 基准)=RGBA(%d,%d,%d,%d)",
                     (unsigned)e, px[0], px[1], px[2], px[3]);
            }
        } else if (rr->game && rr->game->stage == STG_OVER) {
            /* ★ 对局结束（GAME 态且 stage=OVER）→ 胜负横幅（winner 色可视化） */
            rs_draw_overlay(rr, slot, rr->game->winner);
        }
    }
    glFlush();
    /* 诊断：UI_MENU 态下读回按钮 0 中心像素（实证按钮出像素，非清屏色） */
    if (ui_screen_get() == UI_MENU && rr->gl2_ok[slot]) {
        static int mdiag = 0;
        if (!mdiag || (mdiag % 120 == 0)) {
            const ui_btn *btns = ui_menu_buttons();
            float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
            float cx = (btns[0].x0 + btns[0].x1) * 0.5f * sx;
            float cy = (btns[0].y0 + btns[0].y1) * 0.5f * sy;
            /* glReadPixels 原点在左下 → 按钮 y（GL 向上）= cy 直接可用 */
            unsigned char px[4] = {0};
            glReadPixels((int)cx, (int)cy, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
            LOGI("菜单按钮0中心读回 (%.0f,%.0f) = RGBA(%d,%d,%d,%d)（蓝=(38,89,217) 则按钮出像素）",
                 cx, cy, px[0], px[1], px[2], px[3]);
            mdiag++;
        }
    }
}

void rs_render_diag(rs_render *rr)
{
    (void)rr;
}

void rs_render_dirty(rs_render *rr) { rr->dirty = 1; }
void rs_render_set_selected(rs_render *rr, int cell) { rr->sel_cell = cell; rr->dirty = 1; }
void rs_render_resize(rs_render *rr, int w, int h)
{
    if (w > 0) rr->view_w = w;
    if (h > 0) rr->view_h = h;
    rr->dirty = 1;
}

void rs_render_free(rs_render *rr)
{
    for (int i = 0; i < 2; i++) {
        if (rr->tex_board[i] && rr->gl_ok[i]) glDeleteTextures(1, &rr->tex_board[i]);
        if (rr->tex_piece[i] && rr->gl_ok[i]) glDeleteTextures(1, &rr->tex_piece[i]);
        if (rr->prog_tex[i] && rr->gl_ok[i]) glDeleteProgram(rr->prog_tex[i]);
        if (rr->prog_solid[i] && rr->gl_ok[i]) glDeleteProgram(rr->prog_solid[i]);
    }
    free(rr->board_px); rr->board_px = NULL;
    free(rr->piece_px); rr->piece_px = NULL;
    free(rr->spak_dir); rr->spak_dir = NULL;
    if (rr->egl_ctx) eglDestroyContext(rr->egl_disp, rr->egl_ctx);
    if (rr->egl_surf) eglDestroySurface(rr->egl_disp, rr->egl_surf);
    if (rr->egl_disp) eglTerminate(rr->egl_disp);
}
