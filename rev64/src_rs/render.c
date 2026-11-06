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
#include "ui_state.h"   /* P2 图像设置 4 开关 ui_imgset_cfg_get（夜间/对方棋子接渲染层） */
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
        char p11[512], p12[512], p13[512], p14[512], p15[512], p16[512], p17[512];
        snprintf(p11, sizeof p11, "%s/menu_btn_big.png", spak_dir ? spak_dir : "");
        snprintf(p12, sizeof p12, "%s/menu_btn_small.png", spak_dir ? spak_dir : "");
        snprintf(p13, sizeof p13, "%s/waysel_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p14, sizeof p14, "%s/btimen_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p15, sizeof p15, "%s/settings_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p16, sizeof p16, "%s/nplayer_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p17, sizeof p17, "%s/newgame_text_atlas.png", spak_dir ? spak_dir : "");
        char p18[512];
        snprintf(p18, sizeof p18, "%s/gameset_text_atlas.png", spak_dir ? spak_dir : "");
        char p19[512];
        snprintf(p19, sizeof p19, "%s/layres_text_atlas.png", spak_dir ? spak_dir : "");
        char p20[512];
        snprintf(p20, sizeof p20, "%s/replay_text_atlas.png", spak_dir ? spak_dir : "");
        char p21[512];
        snprintf(p21, sizeof p21, "%s/login_text_atlas.png", spak_dir ? spak_dir : "");
        char p22[512];
        snprintf(p22, sizeof p22, "%s/rules_text_atlas.png", spak_dir ? spak_dir : "");
        char p23[512];
        snprintf(p23, sizeof p23, "%s/ng104_text_atlas.png", spak_dir ? spak_dir : "");
        char p24[512];
        snprintf(p24, sizeof p24, "%s/join_text_atlas.png", spak_dir ? spak_dir : "");
        char p25[512], p26[512], p27[512], p28[512], p29[512], p30[512];
        snprintf(p25, sizeof p25, "%s/ng061_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p26, sizeof p26, "%s/gameinfo_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p27, sizeof p27, "%s/bright_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p28, sizeof p28, "%s/chatlog_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p29, sizeof p29, "%s/profile_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p30, sizeof p30, "%s/drawdetail_text_atlas.png", spak_dir ? spak_dir : "");
        char p31[512], p32[512], p33[512], p34[512], p35[512], p36[512], p37[512];
        snprintf(p31, sizeof p31, "%s/setentry_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p32, sizeof p32, "%s/imgset_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p33, sizeof p33, "%s/join065_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p34, sizeof p34, "%s/create069_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p35, sizeof p35, "%s/rules070_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p36, sizeof p36, "%s/room060_text_atlas.png", spak_dir ? spak_dir : "");
        snprintf(p37, sizeof p37, "%s/chat063_text_atlas.png", spak_dir ? spak_dir : "");
        char p38[512];
        snprintf(p38, sizeof p38, "%s/about_text_atlas.png", spak_dir ? spak_dir : "");
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
        unsigned char *wsraw = NULL; size_t wsraw_len = 0;  /* ★ 方式选择文字图集 */
        unsigned char *bira = NULL; size_t bira_len = 0;    /* ★ 联机 ☰ 菜单文字图集 */
        unsigned char *straw = NULL; size_t straw_len = 0;  /* ★ 声音设置文字图集 */
        unsigned char *npaw = NULL; size_t npaw_len = 0;    /* ★ P0 datah_089 四国人数文字图集 */
        unsigned char *ngaw = NULL; size_t ngaw_len = 0;    /* ★ P0 datah_062 两人新局文字图集 */
        unsigned char *gsaw = NULL; size_t gsaw_len = 0;    /* ★ P1 datah_012 游戏设置文字图集 */
        unsigned char *lrsw = NULL; size_t lrsw_len = 0;    /* ★ P1 datah_016/017/020 布局存档文字图集 */
        unsigned char *rpaw = NULL; size_t rpaw_len = 0;    /* ★ P1 datah_039/107 复盘文字图集 */
        unsigned char *lpaw = NULL; size_t lpaw_len = 0;    /* ★ P1 datah_038 登录文字图集 */
        unsigned char *ruaw = NULL; size_t ruaw_len = 0;    /* ★ P2 datah_037 规则文本文字图集 */
        unsigned char *ngaw2 = NULL; size_t ngaw2_len = 0;  /* ★ P2 datah_104 新局完整版文字图集 */
        unsigned char *joaw = NULL; size_t joaw_len = 0;    /* ★ P2 datah_034/035 加入/创建游戏文字图集 */
        unsigned char *n061aw = NULL; size_t n061aw_len = 0; /* ★ P2 datah_061 新局完整版（完整版补充）文字图集 */
        unsigned char *giaw = NULL; size_t giaw_len = 0;    /* ★ P2 datah_050 对局信息文字图集 */
        unsigned char *brw = NULL; size_t brw_len = 0;      /* ★ P2 datah_002 亮度调节文字图集 */
        unsigned char *claw = NULL; size_t claw_len = 0;    /* ★ P2 datah_029 聊天记录文字图集 */
        unsigned char *pfaw = NULL; size_t pfaw_len = 0;    /* ★ P2 datah_031 个人资料文字图集 */
        unsigned char *ddaw = NULL; size_t ddaw_len = 0;    /* ★ P2 datah_042 提和详情文字图集 */
        unsigned char *seaw = NULL; size_t seaw_len = 0;    /* ★ P2 datah_003/004 设置入口文字图集 */
        unsigned char *isaw = NULL; size_t isaw_len = 0;    /* ★ P2 datah_007/008 图像设置文字图集 */
        unsigned char *j65aw = NULL; size_t j65aw_len = 0;  /* ★ P2 datah_065 加入游戏完整版文字图集 */
        unsigned char *c69aw = NULL; size_t c69aw_len = 0;  /* ★ P2 datah_069 创建游戏完整版文字图集 */
        unsigned char *r70aw = NULL; size_t r70aw_len = 0;  /* ★ P2 datah_070 游戏规则完整版文字图集 */
        unsigned char *rmaw = NULL; size_t rmaw_len = 0;    /* ★ P2 datah_060 房间态文字图集 */
        unsigned char *chaw = NULL; size_t chaw_len = 0;    /* ★ P2 datah_063 聊天完整版文字图集 */
        unsigned char *abaw = NULL; size_t abaw_len = 0;    /* ★ P2 datah_000 软件信息文字图集 */
        unsigned char *bpx = NULL, *ppx = NULL, *tpx = NULL, *gpx = NULL;
        unsigned char *opx = NULL, *ppx2 = NULL, *mpx = NULL, *ipx = NULL, *icpx = NULL, *hbpx = NULL, *bbpx = NULL, *bspx = NULL;
        unsigned char *wspx = NULL, *bipx = NULL, *stpx = NULL;
        unsigned char *nppx = NULL, *ngpx = NULL, *gsp_x = NULL, *lrpx = NULL, *rp_px = NULL, *lp_px = NULL;
        int bw = 0, bh = 0, pw = 0, ph = 0, tw = 0, th = 0, gw = 0, gh = 0;
        int ow = 0, oh = 0, pw2 = 0, ph2 = 0, mw = 0, mh = 0, iw = 0, ih = 0, icw = 0, ich = 0;
        int hbw = 0, hbh = 0, bbw = 0, bbh = 0, bsw = 0, bsh = 0, wsw = 0, wsh = 0, biw = 0, bih = 0, stw = 0, sth = 0;
        int npw = 0, nph = 0, ngw = 0, ngh = 0, gsw = 0, gsh = 0, lrw = 0, lrh = 0, rpw = 0, rph = 0;
        int lpw = 0, lph = 0;   /* ★ P1 datah_038 登录文字图集尺寸 */
        int ruw = 0, ruh = 0;   /* ★ P2 datah_037 规则文本文字图集尺寸 */
        int ng2w = 0, ng2h = 0; /* ★ P2 datah_104 新局完整版文字图集尺寸 */
        int joaw_w = 0, joaw_h = 0; /* ★ P2 datah_034/035 加入/创建游戏文字图集尺寸 */
        int n061w = 0, n061h = 0; /* ★ P2 datah_061 新局完整版（完整版补充）文字图集尺寸 */
        int giw = 0, gih = 0;    /* ★ P2 datah_050 对局信息文字图集尺寸 */
        int brtw = 0, brth = 0;  /* ★ P2 datah_002 亮度调节文字图集尺寸 */
        int clw = 0, clh = 0;    /* ★ P2 datah_029 聊天记录文字图集尺寸 */
        int pfw = 0, pfh = 0;    /* ★ P2 datah_031 个人资料文字图集尺寸 */
        int ddw = 0, ddh = 0;    /* ★ P2 datah_042 提和详情文字图集尺寸 */
        int sew = 0, sehh = 0;   /* ★ P2 datah_003/004 设置入口文字图集尺寸 */
        int isw = 0, ishh = 0;   /* ★ P2 datah_007/008 图像设置文字图集尺寸 */
        int j65w = 0, j65h = 0;  /* ★ P2 datah_065 加入游戏完整版文字图集尺寸 */
        int c69w = 0, c69h = 0;  /* ★ P2 datah_069 创建游戏完整版文字图集尺寸 */
        int r70w = 0, r70h = 0;  /* ★ P2 datah_070 游戏规则完整版文字图集尺寸 */
        int rmw = 0, rmh = 0;    /* ★ P2 datah_060 房间态文字图集尺寸 */
        int chw = 0, chh = 0;    /* ★ P2 datah_063 聊天完整版文字图集尺寸 */
        int abw = 0, abh = 0;    /* ★ P2 datah_000 软件信息文字图集尺寸 */
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
        rs_read_png(am, "spak_res/waysel_text_atlas.png", p13, &wsraw, &wsraw_len);
        rs_read_png(am, "spak_res/btimen_text_atlas.png", p14, &bira, &bira_len);
        rs_read_png(am, "spak_res/settings_text_atlas.png", p15, &straw, &straw_len);
        rs_read_png(am, "spak_res/nplayer_text_atlas.png", p16, &npaw, &npaw_len);
        rs_read_png(am, "spak_res/newgame_text_atlas.png", p17, &ngaw, &ngaw_len);
        rs_read_png(am, "spak_res/gameset_text_atlas.png", p18, &gsaw, &gsaw_len);
        rs_read_png(am, "spak_res/layres_text_atlas.png", p19, &lrsw, &lrsw_len);
        rs_read_png(am, "spak_res/replay_text_atlas.png", p20, &rpaw, &rpaw_len);
        rs_read_png(am, "spak_res/login_text_atlas.png", p21, &lpaw, &lpaw_len);
        rs_read_png(am, "spak_res/rules_text_atlas.png", p22, &ruaw, &ruaw_len);
        rs_read_png(am, "spak_res/ng104_text_atlas.png", p23, &ngaw2, &ngaw2_len);
        rs_read_png(am, "spak_res/join_text_atlas.png", p24, &joaw, &joaw_len);
        rs_read_png(am, "spak_res/ng061_text_atlas.png", p25, &n061aw, &n061aw_len);
        rs_read_png(am, "spak_res/gameinfo_text_atlas.png", p26, &giaw, &giaw_len);
        rs_read_png(am, "spak_res/bright_text_atlas.png", p27, &brw, &brw_len);
        rs_read_png(am, "spak_res/chatlog_text_atlas.png", p28, &claw, &claw_len);
        rs_read_png(am, "spak_res/profile_text_atlas.png", p29, &pfaw, &pfaw_len);
        rs_read_png(am, "spak_res/drawdetail_text_atlas.png", p30, &ddaw, &ddaw_len);
        rs_read_png(am, "spak_res/setentry_text_atlas.png", p31, &seaw, &seaw_len);
        rs_read_png(am, "spak_res/imgset_text_atlas.png", p32, &isaw, &isaw_len);
        rs_read_png(am, "spak_res/join065_text_atlas.png", p33, &j65aw, &j65aw_len);
        rs_read_png(am, "spak_res/create069_text_atlas.png", p34, &c69aw, &c69aw_len);
        rs_read_png(am, "spak_res/rules070_text_atlas.png", p35, &r70aw, &r70aw_len);
        rs_read_png(am, "spak_res/room060_text_atlas.png", p36, &rmaw, &rmaw_len);
        rs_read_png(am, "spak_res/chat063_text_atlas.png", p37, &chaw, &chaw_len);
        rs_read_png(am, "spak_res/about_text_atlas.png", p38, &abaw, &abaw_len);
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
        /* ★ 对抗审计（datah_072）：方式选择文字图集（4 标签竖排 220×184） */
        if (wsraw && png_decode(wsraw, wsraw_len, &wspx, &wsw, &wsh) == 0) {
            rr->waysel_text_px = wspx; rr->waysel_text_px_len = (size_t)wsw * wsh * 4;
            rr->waysel_text_w = wsw; rr->waysel_text_h = wsh;
        }
        free(wsraw);
        /* ★ 对抗审计（datah_067）：联机 ☰ 菜单文字图集（8 项竖排 160×320） */
        if (bira && png_decode(bira, bira_len, &bipx, &biw, &bih) == 0) {
            rr->btimen_text_px = bipx; rr->btimen_text_px_len = (size_t)biw * bih * 4;
            rr->btimen_text_w = biw; rr->btimen_text_h = bih;
        }
        free(bira);
        /* ★ 对抗审计（datah_001）：声音设置文字图集（10 标签竖排 200×460） */
        if (straw && png_decode(straw, straw_len, &stpx, &stw, &sth) == 0) {
            rr->settings_text_px = stpx; rr->settings_text_px_len = (size_t)stw * sth * 4;
            rr->settings_text_w = stw; rr->settings_text_h = sth;
        }
        free(straw);
        /* ★ P0（datah_089）：四国人数选择文字图集（3 标签竖排 220×138：2人/3人/4人） */
        if (npaw && png_decode(npaw, npaw_len, &nppx, &npw, &nph) == 0) {
            rr->nplayer_text_px = nppx; rr->nplayer_text_px_len = (size_t)npw * nph * 4;
            rr->nplayer_text_w = npw; rr->nplayer_text_h = nph;
        }
        free(npaw);
        /* ★ P0（datah_062）：两人新局文字图集（6 标签竖排 220×276：玩法/布局明棋/翻棋暗棋/明暗棋/确定/取消） */
        if (ngaw && png_decode(ngaw, ngaw_len, &ngpx, &ngw, &ngh) == 0) {
            rr->newgame_text_px = ngpx; rr->newgame_text_px_len = (size_t)ngw * ngh * 4;
            rr->newgame_text_w = ngw; rr->newgame_text_h = ngh;
        }
        free(ngaw);
        /* ★ P1（datah_012）：游戏设置文字图集（8 标签竖排 220×368：和棋/大本营/吃地雷/吃军旗/开/关/确定/取消） */
        if (gsaw && png_decode(gsaw, gsaw_len, &gsp_x, &gsw, &gsh) == 0) {
            rr->gameset_text_px = gsp_x; rr->gameset_text_px_len = (size_t)gsw * gsh * 4;
            rr->gameset_text_w = gsw; rr->gameset_text_h = gsh;
        }
        free(gsaw);
        /* ★ P1（datah_016/017/020）：布局存档文字图集（9 标签竖排 220×414：
           载入布局/保存布局/布局管理/取消/保存/改名/删除/关闭/载入） */
        if (lrsw && png_decode(lrsw, lrsw_len, &lrpx, &lrw, &lrh) == 0) {
            rr->layres_text_px = lrpx; rr->layres_text_px_len = (size_t)lrw * lrh * 4;
            rr->layres_text_w = lrw; rr->layres_text_h = lrh;
        }
        free(lrsw);
        /* ★ P1（datah_039/107）：复盘文字图集（8 标签竖排 220×368：
           复盘/菜单/回退/前进/跳转/返回/第N步/结束） */
        if (rpaw && png_decode(rpaw, rpaw_len, &rp_px, &rpw, &rph) == 0) {
            rr->replay_text_px = rp_px; rr->replay_text_px_len = (size_t)rpw * rph * 4;
            rr->replay_text_w = rpw; rr->replay_text_h = rph;
        }
        free(rpaw);
        /* ★ P1（datah_038）：登录文字图集（8 标签竖排 220×368：
           登录/用户名/密码/注册/游客/返回/账户/边界） */
        if (lpaw && png_decode(lpaw, lpaw_len, &lp_px, &lpw, &lph) == 0) {
            rr->login_text_px = lp_px; rr->login_text_px_len = (size_t)lpw * lph * 4;
            rr->login_text_w = lpw; rr->login_text_h = lph;
        }
        free(lpaw);
        /* ★ P2（datah_037）：规则文本文字图集（8 标签竖排 220×368：
           规则/翻棋/布局/工兵/地雷/炸弹/夺旗/边界） */
        {
            unsigned char *ru_px = NULL;
            if (ruaw && png_decode(ruaw, ruaw_len, &ru_px, &ruw, &ruh) == 0) {
                rr->rules_text_px = ru_px; rr->rules_text_px_len = (size_t)ruw * ruh * 4;
                rr->rules_text_w = ruw; rr->rules_text_h = ruh;
            }
        }
        free(ruaw);
        /* ★ P2（datah_104）：新局完整版文字图集（10 标签竖排 220×460：
           新局/我方/电脑/玩法/蓝方/红方/新手/中级/高级/确定） */
        {
            unsigned char *ng2_px = NULL;
            if (ngaw2 && png_decode(ngaw2, ngaw2_len, &ng2_px, &ng2w, &ng2h) == 0) {
                rr->ng104_text_px = ng2_px; rr->ng104_text_px_len = (size_t)ng2w * ng2h * 4;
                rr->ng104_text_w = ng2w; rr->ng104_text_h = ng2h;
            }
        }
        free(ngaw2);
        /* ★ P2（datah_034/035）：加入/创建游戏文字图集（10 标签竖排 220×460：
           加入/创建/桌号/密码/确定/取消/蓝牙/边界/联机/Wi-Fi） */
        {
            unsigned char *jo_px = NULL;
            if (joaw && png_decode(joaw, joaw_len, &jo_px, &joaw_w, &joaw_h) == 0) {
                rr->join_text_px = jo_px; rr->join_text_px_len = (size_t)joaw_w * joaw_h * 4;
                rr->join_text_w = joaw_w; rr->join_text_h = joaw_h;
            }
        }
        free(joaw);
        /* ★ P2（datah_061）：新局完整版（完整版补充）文字图集（10 标签竖排 220×460） */
        {
            unsigned char *n061_px = NULL;
            if (n061aw && png_decode(n061aw, n061aw_len, &n061_px, &n061w, &n061h) == 0) {
                rr->ng061_text_px = n061_px; rr->ng061_text_px_len = (size_t)n061w * n061h * 4;
                rr->ng061_text_w = n061w; rr->ng061_text_h = n061h;
            }
        }
        free(n061aw);
        /* ★ P2（datah_050）：对局信息文字图集（8 标签竖排 220×368） */
        {
            unsigned char *gi_px = NULL;
            if (giaw && png_decode(giaw, giaw_len, &gi_px, &giw, &gih) == 0) {
                rr->gameinfo_text_px = gi_px; rr->gameinfo_text_px_len = (size_t)giw * gih * 4;
                rr->gameinfo_text_w = giw; rr->gameinfo_text_h = gih;
            }
        }
        free(giaw);
        /* ★ P2（datah_002）：亮度调节文字图集（10 标签竖排 220×460） */
        {
            unsigned char *br_px = NULL;
            if (brw && png_decode(brw, brw_len, &br_px, &brtw, &brth) == 0) {
                rr->bright_text_px = br_px; rr->bright_text_px_len = (size_t)brtw * brth * 4;
                rr->bright_text_w = brtw; rr->bright_text_h = brth;
            }
        }
        free(brw);
        /* ★ P2（datah_029）：聊天记录文字图集（8 标签竖排 220×368） */
        {
            unsigned char *cl_px = NULL;
            if (claw && png_decode(claw, claw_len, &cl_px, &clw, &clh) == 0) {
                rr->chatlog_text_px = cl_px; rr->chatlog_text_px_len = (size_t)clw * clh * 4;
                rr->chatlog_text_w = clw; rr->chatlog_text_h = clh;
            }
        }
        free(claw);
        /* ★ P2（datah_031）：个人资料文字图集（10 标签竖排 220×460） */
        {
            unsigned char *pf_px = NULL;
            if (pfaw && png_decode(pfaw, pfaw_len, &pf_px, &pfw, &pfh) == 0) {
                rr->profile_text_px = pf_px; rr->profile_text_px_len = (size_t)pfw * pfh * 4;
                rr->profile_text_w = pfw; rr->profile_text_h = pfh;
            }
        }
        free(pfaw);
        /* ★ P2（datah_042）：提和详情文字图集（7 标签竖排 220×322） */
        {
            unsigned char *dd_px = NULL;
            if (ddaw && png_decode(ddaw, ddaw_len, &dd_px, &ddw, &ddh) == 0) {
                rr->drawdetail_text_px = dd_px; rr->drawdetail_text_px_len = (size_t)ddw * ddh * 4;
                rr->drawdetail_text_w = ddw; rr->drawdetail_text_h = ddh;
            }
        }
        free(ddaw);
        /* ★ P2（datah_003/004）：设置入口文字图集（7 标签竖排 220×322） */
        {
            unsigned char *se_px = NULL;
            if (seaw && png_decode(seaw, seaw_len, &se_px, &sew, &sehh) == 0) {
                rr->setentry_text_px = se_px; rr->setentry_text_px_len = (size_t)sew * sehh * 4;
                rr->setentry_text_w = sew; rr->setentry_text_h = sehh;
            }
        }
        free(seaw);
        /* ★ P2（datah_007/008）：图像设置文字图集（10 标签竖排 220×460） */
        {
            unsigned char *is_px = NULL;
            if (isaw && png_decode(isaw, isaw_len, &is_px, &isw, &ishh) == 0) {
                rr->imgset_text_px = is_px; rr->imgset_text_px_len = (size_t)isw * ishh * 4;
                rr->imgset_text_w = isw; rr->imgset_text_h = ishh;
            }
        }
        free(isaw);
        /* ★ P2（datah_065）：加入游戏完整版文字图集（10 标签竖排 220×460） */
        {
            unsigned char *j65_px = NULL;
            if (j65aw && png_decode(j65aw, j65aw_len, &j65_px, &j65w, &j65h) == 0) {
                rr->join065_text_px = j65_px; rr->join065_text_px_len = (size_t)j65w * j65h * 4;
                rr->join065_text_w = j65w; rr->join065_text_h = j65h;
            }
        }
        free(j65aw);
        /* ★ P2（datah_069）：创建游戏完整版文字图集（10 标签竖排 220×460） */
        {
            unsigned char *c69_px = NULL;
            if (c69aw && png_decode(c69aw, c69aw_len, &c69_px, &c69w, &c69h) == 0) {
                rr->create069_text_px = c69_px; rr->create069_text_px_len = (size_t)c69w * c69h * 4;
                rr->create069_text_w = c69w; rr->create069_text_h = c69h;
            }
        }
        free(c69aw);
        /* ★ P2（datah_070）：游戏规则完整版文字图集（10 标签竖排 220×460） */
        {
            unsigned char *r70_px = NULL;
            if (r70aw && png_decode(r70aw, r70aw_len, &r70_px, &r70w, &r70h) == 0) {
                rr->rules070_text_px = r70_px; rr->rules070_text_px_len = (size_t)r70w * r70h * 4;
                rr->rules070_text_w = r70w; rr->rules070_text_h = r70h;
            }
        }
        free(r70aw);
        /* ★ P2（datah_060）：房间态文字图集（8 标签竖排 220×368） */
        {
            unsigned char *rm_px = NULL;
            if (rmaw && png_decode(rmaw, rmaw_len, &rm_px, &rmw, &rmh) == 0) {
                rr->room060_text_px = rm_px; rr->room060_text_px_len = (size_t)rmw * rmh * 4;
                rr->room060_text_w = rmw; rr->room060_text_h = rmh;
            }
        }
        free(rmaw);
        /* ★ P2（datah_063）：聊天完整版文字图集（8 标签竖排 220×368） */
        {
            unsigned char *ch_px = NULL;
            if (chaw && png_decode(chaw, chaw_len, &ch_px, &chw, &chh) == 0) {
                rr->chat063_text_px = ch_px; rr->chat063_text_px_len = (size_t)chw * chh * 4;
                rr->chat063_text_w = chw; rr->chat063_text_h = chh;
            }
        }
        free(chaw);
        /* ★ P2（datah_000）：软件信息文字图集（7 标签竖排 220×322：软件信息/版本/
           官网/反馈/关闭/1.6.8/cnvcs.com） */
        {
            unsigned char *ab_px = NULL;
            if (abaw && png_decode(abaw, abaw_len, &ab_px, &abw, &abh) == 0) {
                rr->about_text_px = ab_px; rr->about_text_px_len = (size_t)abw * abh * 4;
                rr->about_text_w = abw; rr->about_text_h = abh;
            }
        }
        free(abaw);
        rr->gl_px_loaded = 1;
        LOGI("Init 像素解码：board=%s(%dx%d) piece=%s(%dx%d) text=%s(%dx%d) bg=%s(%dx%d) menu_bg=%s(%dx%d) popup=%s(%dx%d) opt_text=%s(%dx%d) imen_text=%s(%dx%d) waysel=%s(%dx%d) btimen=%s(%dx%d) settings=%s(%dx%d) nplayer=%s(%dx%d) newgame=%s(%dx%d) gameset=%s(%dx%d) layres=%s(%dx%d) replay=%s(%dx%d) login=%s(%dx%d) rules=%s(%dx%d) ng104=%s(%dx%d) join=%s(%dx%d) ng061=%s(%dx%d) gameinfo=%s(%dx%d) bright=%s(%dx%d) chatlog=%s(%dx%d) profile=%s(%dx%d) drawdetail=%s(%dx%d) setentry=%s(%dx%d) imgset=%s(%dx%d) join065=%s(%dx%d) create069=%s(%dx%d) rules070=%s(%dx%d) room060=%s(%dx%d) chat063=%s(%dx%d) about=%s(%dx%d)",
             rr->board_px ? "OK" : "MISS", bw, bh, rr->piece_px ? "OK" : "MISS", pw, ph,
             rr->text_px ? "OK" : "MISS", tw, th, rr->bg_px ? "OK" : "MISS", gw, gh,
             rr->menu_bg_px ? "OK" : "MISS", mw, mh, rr->popup_px ? "OK" : "MISS", pw2, ph2,
             rr->opt_text_px ? "OK" : "MISS", ow, oh, rr->imen_text_px ? "OK" : "MISS", iw, ih,
             rr->imen_icons_px ? "OK" : "MISS", icw, ich, rr->hamburger_px ? "OK" : "MISS", hbw, hbh,
             rr->btn_big_px ? "OK" : "MISS", bbw, bbh, rr->btn_small_px ? "OK" : "MISS", bsw, bsh,
             rr->waysel_text_px ? "OK" : "MISS", wsw, wsh, rr->btimen_text_px ? "OK" : "MISS", biw, bih,
             rr->settings_text_px ? "OK" : "MISS", stw, sth,
             rr->nplayer_text_px ? "OK" : "MISS", npw, nph, rr->newgame_text_px ? "OK" : "MISS", ngw, ngh,
              rr->gameset_text_px ? "OK" : "MISS", gsw, gsh,
              rr->layres_text_px ? "OK" : "MISS", lrw, lrh,
              rr->replay_text_px ? "OK" : "MISS", rpw, rph,
               rr->login_text_px ? "OK" : "MISS", lpw, lph,
                rr->rules_text_px ? "OK" : "MISS", ruw, ruh,
                rr->ng104_text_px ? "OK" : "MISS", ng2w, ng2h,
                rr->join_text_px ? "OK" : "MISS", joaw_w, joaw_h,
                 rr->join_text_px ? "OK" : "MISS", joaw_w, joaw_h,
                 rr->ng061_text_px ? "OK" : "MISS", n061w, n061h,
                 rr->gameinfo_text_px ? "OK" : "MISS", giw, gih,
                 rr->bright_text_px ? "OK" : "MISS", brtw, brth,
                 rr->chatlog_text_px ? "OK" : "MISS", clw, clh,
                 rr->profile_text_px ? "OK" : "MISS", pfw, pfh,
                 rr->drawdetail_text_px ? "OK" : "MISS", ddw, ddh,
                 rr->setentry_text_px ? "OK" : "MISS", sew, sehh,
                 rr->imgset_text_px ? "OK" : "MISS", isw, ishh,
                 rr->join065_text_px ? "OK" : "MISS", j65w, j65h,
                 rr->create069_text_px ? "OK" : "MISS", c69w, c69h,
                 rr->rules070_text_px ? "OK" : "MISS", r70w, r70h,
                 rr->room060_text_px ? "OK" : "MISS", rmw, rmh,
                 rr->chat063_text_px ? "OK" : "MISS", chw, chh,
                 rr->about_text_px ? "OK" : "MISS", abw, abh);
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
    /* ★ 对抗审计（datah_067）：蓝牙联机 ☰ 菜单用联机版按钮表（提和/认输/聊天），
       本地模式用 datah_006 版（电脑变着/软件信息/退出） */
    const ui_imen_btn *im = ui_bt_active() ? ui_imen_bt_buttons() : ui_imen_buttons();
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
       文字中心 y=594、行2 y=780），中心 x = 按钮中心（与图标同列）。
       ★ 对抗审计（datah_067）：蓝牙联机 ☰ 菜单用 btimen_text_atlas（提和/认输/聊天）。 */
    int imen_bt = ui_bt_active();
    int text_w = imen_bt ? rr->btimen_text_w : rr->imen_text_w;
    int text_h = imen_bt ? rr->btimen_text_h : rr->imen_text_h;
    rs_glid_t text_tex = imen_bt ? rr->tex_btimen_text[slot] : rr->tex_imen_text[slot];
    if (text_tex && rr->prog_tex[slot] && text_w > 0 && text_h > 0) {
        for (int i = 0; i < UI_IMENU_COUNT; i++) {
            float bx0 = im[i].x0 * sx, by0 = im[i].y0 * sy;
            float bx1 = im[i].x1 * sx, by1 = im[i].y1 * sy;
            float tcx = (bx0 + bx1) * 0.5f;
            float tcy = by0 + (by1 - by0) * 0.90f;
            float tw = 134.0f * sx, th = tw * (40.0f / 160.0f);   /* 原版实测：字形占图集 94% → tw=134 → 字形 126px（720 基准）=157px（900 屏），与原版 bbox 一致 */
            float tx0 = tcx - tw * 0.5f, ty0 = tcy - th * 0.5f;
            float u0 = 0.0f, u1 = 1.0f;   /* 整源宽（文字 160px 宽图集，全宽） */
            float v0 = (float)(i * 40) / text_h;
            float v1 = (float)(i * 40 + 40) / text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, text_tex);
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

/* ★ 对抗审计（datah_041 证据）：人机难度选择弹窗渲染。
   面板深色半透明（复用 imen 面板）+ 8 按钮（布局/翻棋 2 列 × 新手/中级/高级/好友 4 行）
   + 左列 region 标签（新手区/中级区/高级区/好友区，opt_text row0-3）+ 按钮内 mode 标签
   （布局/翻棋，opt_text row4-5）。按钮几何 ui_diff_buttons()（720×1280 基准）。 */
static void rs_draw_difficulty(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪ 背景：主菜单背景 datax_294（对抗审计：难度弹窗是主菜单上的模态框，
       非棋盘——原版 datah_041 从主菜单弹，背景应为主菜单） */
    if (rr->tex_menu_bg[slot] && rr->prog_tex[slot]) {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f bg[6] = {
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
        glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ① 全屏模态遮罩（半透明黑，聚焦弹窗） */
    rs_v4f mq[6] = {
        {0, 0, 0, 0}, {(float)rr->view_w, 0, 0, 0}, {0, (float)rr->view_h, 0, 0},
        {0, (float)rr->view_h, 0, 0}, {(float)rr->view_w, 0, 0, 0},
        {(float)rr->view_w, (float)rr->view_h, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.0f, 0.0f, 0.0f, 0.35f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mq), mq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ② 面板：深色半透明矩形（datah_041 450×335 缩放 720 基准） */
    float px0 = UI_DIFF_POPUP_X0 * sx, py0 = UI_DIFF_POPUP_Y0 * sy;
    float pw = UI_DIFF_POPUP_W * sx, ph = UI_DIFF_POPUP_H * sy;
    rs_v4f pq[6] = {
        {px0, py0, 0, 0}, {px0 + pw, py0, 0, 0}, {px0, py0 + ph, 0, 0},
        {px0, py0 + ph, 0, 0}, {px0 + pw, py0, 0, 0}, {px0 + pw, py0 + ph, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.08f, 0.08f, 0.10f, 0.78f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ③ 8 按钮：布局/翻棋 2 列 × 4 行，纯色圆角近似（选中高亮当前难度）+ mode 标签 */
    static const float BTN_COL[2][4] = {
        {0.25f, 0.45f, 0.85f, 0.9f},   /* 布局 蓝 */
        {0.45f, 0.30f, 0.75f, 0.9f},   /* 翻棋 紫 */
    };
    const ui_diff_btn *db = ui_diff_buttons();
    int cur_level = ui_opt_get_level(), cur_mode = ui_opt_get_mode();
    for (int i = 0; i < UI_DIFF_COUNT; i++) {
        int level = db[i].action / 2 + 1, mode = db[i].action % 2;
        float bx0 = db[i].x0 * sx, by0 = db[i].y0 * sy;
        float bx1 = db[i].x1 * sx, by1 = db[i].y1 * sy;
        /* 当前选中项亮边（金色），未选中普通 */
        int sel = (level == cur_level && mode == cur_mode);
        float cr = sel ? 1.0f : BTN_COL[mode][0], cg = sel ? 0.85f : BTN_COL[mode][1],
              cb = sel ? 0.2f : BTN_COL[mode][2];
        rs_v4f bq[6] = {
            {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
            {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, cr, cg, cb, sel ? 0.95f : 0.85f);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ④ region 标签（opt_text row0-3：新手区/中级区/高级区/好友区）+ mode 标签（row4-5：布局/翻棋） */
    if (rr->tex_opt_text[slot] && rr->prog_tex[slot] && rr->opt_text_w > 0 && rr->opt_text_h > 0) {
        float u0 = 0.0f, u1 = 1.0f;
        int row_h = rr->opt_text_h / 6;   /* 每行 43px（6 标签竖排） */
        /* region 标签：左列 x 200-300，y 对齐各行中心 */
        for (int r = 0; r < 4; r++) {
            int row_idx = r;   /* row0-3 = 新手/中级/高级/好友区 */
            /* ★ 原版 datah_041 区域标签列 x=30-150（450 系）→ 720 屏幕 x=165-285（+135 偏移） */
            float tcy = (db[r * 2].y0 + db[r * 2].y1) * 0.5f * sy;
            float tw = 120.0f * sx, th = tw * ((float)row_h / rr->opt_text_w);
            float tx0 = 165.0f * sx, ty0 = tcy - th * 0.5f;
            float v0 = (float)(row_idx * row_h) / rr->opt_text_h;
            float v1 = (float)(row_idx * row_h + row_h) / rr->opt_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_opt_text[slot]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
        /* mode 标签：每按钮内居中（布局=row4，翻棋=row5） */
        for (int i = 0; i < UI_DIFF_COUNT; i++) {
            int mode = db[i].action % 2;
            int row_idx = 4 + mode;   /* row4=布局 row5=翻棋 */
            float bx0 = db[i].x0 * sx, by0 = db[i].y0 * sy;
            float bx1 = db[i].x1 * sx, by1 = db[i].y1 * sy;
            float tw = 80.0f * sx, th = tw * ((float)row_h / rr->opt_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float v0 = (float)(row_idx * row_h) / rr->opt_text_h;
            float v1 = (float)(row_idx * row_h + row_h) / rr->opt_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_opt_text[slot]);
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
    static int ddiag = 0;
    if (!ddiag) { ddiag = 1; LOGI("rs_draw_difficulty：人机难度弹窗（datah_041 新手/中级/高级/好友区 × 布局/翻棋 8 按钮，面板 %.0f×%.0f）", UI_DIFF_POPUP_W, UI_DIFF_POPUP_H); }
}

/* ★ 对抗审计（datah_072 证据）：联网/联机方式选择弹窗渲染。
   面板深色半透明 + 4 按钮（蓝牙联机/Wi-Fi 联机/个人资料/返回）+ 文字图集。
   按钮几何 ui_way_buttons()（720×1280 基准）。 */
static void rs_draw_waysel(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪ 背景：主菜单背景（方式选择弹窗是主菜单上的模态框） */
    if (rr->tex_menu_bg[slot] && rr->prog_tex[slot]) {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f bg[6] = {
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
        glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ① 全屏模态遮罩 */
    rs_v4f mq[6] = {
        {0, 0, 0, 0}, {(float)rr->view_w, 0, 0, 0}, {0, (float)rr->view_h, 0, 0},
        {0, (float)rr->view_h, 0, 0}, {(float)rr->view_w, 0, 0, 0},
        {(float)rr->view_w, (float)rr->view_h, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.0f, 0.0f, 0.0f, 0.35f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mq), mq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ② 面板：深色半透明矩形 */
    float px0 = UI_WAYSEL_POPUP_X0 * sx, py0 = UI_WAYSEL_POPUP_Y0 * sy;
    float pw = UI_WAYSEL_POPUP_W * sx, ph = UI_WAYSEL_POPUP_H * sy;
    rs_v4f pq[6] = {
        {px0, py0, 0, 0}, {px0 + pw, py0, 0, 0}, {px0, py0 + ph, 0, 0},
        {px0, py0 + ph, 0, 0}, {px0 + pw, py0, 0, 0}, {px0 + pw, py0 + ph, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.08f, 0.08f, 0.10f, 0.78f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ③ 4 按钮：纯色圆角近似 + 文字 */
    static const float BTN_COL[4][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 蓝牙联机 蓝 */
        {0.30f, 0.70f, 0.50f, 0.9f},   /* Wi-Fi 联机 绿 */
        {0.80f, 0.60f, 0.25f, 0.9f},   /* 个人资料 橙 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 返回 灰 */
    };
    const ui_way_btn *wb = ui_way_buttons();
    for (int i = 0; i < UI_WAYSEL_COUNT; i++) {
        float bx0 = wb[i].x0 * sx, by0 = wb[i].y0 * sy;
        float bx1 = wb[i].x1 * sx, by1 = wb[i].y1 * sy;
        rs_v4f bq[6] = {
            {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
            {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ④ 4 按钮文字（waysel_text_atlas，每按钮一 label，居中） */
    if (rr->tex_waysel_text[slot] && rr->prog_tex[slot] && rr->waysel_text_w > 0 && rr->waysel_text_h > 0) {
        int row_h = rr->waysel_text_h / UI_WAYSEL_COUNT;   /* 每行 46px */
        for (int i = 0; i < UI_WAYSEL_COUNT; i++) {
            float bx0 = wb[i].x0 * sx, by0 = wb[i].y0 * sy;
            float bx1 = wb[i].x1 * sx, by1 = wb[i].y1 * sy;
            float tw = 150.0f * sx, th = tw * ((float)row_h / rr->waysel_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float u0 = 0.0f, u1 = 1.0f;
            float v0 = (float)(i * row_h) / rr->waysel_text_h;
            float v1 = (float)(i * row_h + row_h) / rr->waysel_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_waysel_text[slot]);
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
    static int wdiag = 0;
    if (!wdiag) { wdiag = 1; LOGI("rs_draw_waysel：联网/联机方式选择弹窗（datah_072 蓝牙联机/Wi-Fi 联机/个人资料/返回 4 按钮，面板 %.0f×%.0f）", UI_WAYSEL_POPUP_W, UI_WAYSEL_POPUP_H); }
}

/* ★ 对抗审计（datah_001 证据）：声音设置弹窗渲染。
   面板深色半透明 + 5 按钮（静音模式/按键声音/音量/确定/取消）+ 文字图集。
   按钮几何 ui_set_buttons()（720×1280 基准）。 */
static void rs_draw_settings(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪ 背景：对局棋盘背景（设置弹窗从 ☰ 菜单弹出，背景为对局） */
    if (rr->tex_bg[slot] && rr->prog_tex[slot]) {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f bg[6] = {
            {0, 0, 0, 0}, {w, 0, 1, 0}, {0, h, 0, 1},
            {0, h, 0, 1}, {w, 0, 1, 0}, {w, h, 1, 1},
        };
        glUseProgram(rr->prog_tex[slot]);
        glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rr->tex_bg[slot]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ① 全屏模态遮罩 */
    rs_v4f mq[6] = {
        {0, 0, 0, 0}, {(float)rr->view_w, 0, 0, 0}, {0, (float)rr->view_h, 0, 0},
        {0, (float)rr->view_h, 0, 0}, {(float)rr->view_w, 0, 0, 0},
        {(float)rr->view_w, (float)rr->view_h, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.0f, 0.0f, 0.0f, 0.35f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mq), mq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ② 面板：深色半透明矩形 */
    float px0 = UI_SETTINGS_POPUP_X0 * sx, py0 = UI_SETTINGS_POPUP_Y0 * sy;
    float pw = UI_SETTINGS_POPUP_W * sx, ph = UI_SETTINGS_POPUP_H * sy;
    rs_v4f pq[6] = {
        {px0, py0, 0, 0}, {px0 + pw, py0, 0, 0}, {px0, py0 + ph, 0, 0},
        {px0, py0 + ph, 0, 0}, {px0 + pw, py0, 0, 0}, {px0 + pw, py0 + ph, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.08f, 0.08f, 0.10f, 0.78f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ③ 5 按钮：2 开关 + 音量 + 确定/取消 */
    static const float BTN_COL[5][4] = {
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 静音模式 灰 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 按键声音 灰 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 音量 灰 */
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 确定 蓝 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
    };
    const ui_set_btn *sb = ui_set_buttons();
    for (int i = 0; i < UI_SETTINGS_COUNT; i++) {
        float bx0 = sb[i].x0 * sx, by0 = sb[i].y0 * sy;
        float bx1 = sb[i].x1 * sx, by1 = sb[i].y1 * sy;
        rs_v4f bq[6] = {
            {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
            {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ④ 文字：左列 label（静音模式/按键声音/音量，settings_text row0-2）+ 按钮内值
       （开/关 row5-6，低/中/高 row7-9）+ 确定/取消（row3-4） */
    if (rr->tex_settings_text[slot] && rr->prog_tex[slot] && rr->settings_text_w > 0 && rr->settings_text_h > 0) {
        int row_h = rr->settings_text_h / 10;   /* 每行 46px */
        float u0 = 0.0f, u1 = 1.0f;
        /* label 左列：x 200-290 */
        for (int r = 0; r < 3; r++) {
            float tcy = (sb[r].y0 + sb[r].y1) * 0.5f * sy;
            float tw = 90.0f * sx, th = tw * ((float)row_h / rr->settings_text_w);
            float tx0 = 200.0f * sx, ty0 = tcy - th * 0.5f;
            float v0 = (float)(r * row_h) / rr->settings_text_h;
            float v1 = (float)(r * row_h + row_h) / rr->settings_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_settings_text[slot]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
        /* 值标签（开关开/关，音量低/中/高） */
        int val_rows[3] = {5 + 0, 5 + 1, 7 + (ui_volume_get() - 1)};  /* 静音row5开/6关, 按键row5/6, 音量row7-9 */
        if (ui_snd_mute_get()) val_rows[0] = 6;   /* 静音=关显示 */
        if (!ui_btn_sound_get()) val_rows[1] = 6; /* 按键声音=关 */
        for (int r = 0; r < 3; r++) {
            float bx0 = sb[r].x0 * sx, by0 = sb[r].y0 * sy;
            float bx1 = sb[r].x1 * sx, by1 = sb[r].y1 * sy;
            float tw = 60.0f * sx, th = tw * ((float)row_h / rr->settings_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float v0 = (float)(val_rows[r] * row_h) / rr->settings_text_h;
            float v1 = (float)(val_rows[r] * row_h + row_h) / rr->settings_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_settings_text[slot]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
        /* 确定/取消（row3-4） */
        for (int i = 3; i < 5; i++) {
            float bx0 = sb[i].x0 * sx, by0 = sb[i].y0 * sy;
            float bx1 = sb[i].x1 * sx, by1 = sb[i].y1 * sy;
            float tw = 120.0f * sx, th = tw * ((float)row_h / rr->settings_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float v0 = (float)(i * row_h) / rr->settings_text_h;
            float v1 = (float)(i * row_h + row_h) / rr->settings_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_settings_text[slot]);
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
    static int stdiag = 0;
    if (!stdiag) { stdiag = 1; LOGI("rs_draw_settings：声音设置弹窗（datah_001 静音模式/按键声音/音量/确定/取消，面板 %.0f×%.0f）", UI_SETTINGS_POPUP_W, UI_SETTINGS_POPUP_H); }
}

/* ★ P0（datah_089 证据）：四国人数选择弹窗渲染。
   主菜单背景 + 面板深色半透明 + 3 按钮（2 人/3 人/4 人，纯色）+ 文字图集。
   按钮几何 ui_nplayer_buttons()（720×1280 基准）。★ 诚实边界：四国=未实现 4 方规则。 */
static void rs_draw_nplayer(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪ 背景：主菜单背景（四国人数选择弹窗是主菜单上的模态框） */
    if (rr->tex_menu_bg[slot] && rr->prog_tex[slot]) {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f bg[6] = {
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
        glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ① 全屏模态遮罩 */
    rs_v4f mq[6] = {
        {0, 0, 0, 0}, {(float)rr->view_w, 0, 0, 0}, {0, (float)rr->view_h, 0, 0},
        {0, (float)rr->view_h, 0, 0}, {(float)rr->view_w, 0, 0, 0},
        {(float)rr->view_w, (float)rr->view_h, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.0f, 0.0f, 0.0f, 0.35f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mq), mq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ② 面板：深色半透明矩形（datah_089 230×230 居中，720 基准 245,525-475,755） */
    float px0 = UI_NPLAYER_POPUP_X0 * sx, py0 = UI_NPLAYER_POPUP_Y0 * sy;
    float pw = UI_NPLAYER_POPUP_W * sx, ph = UI_NPLAYER_POPUP_H * sy;
    rs_v4f pq[6] = {
        {px0, py0, 0, 0}, {px0 + pw, py0, 0, 0}, {px0, py0 + ph, 0, 0},
        {px0, py0 + ph, 0, 0}, {px0 + pw, py0, 0, 0}, {px0 + pw, py0 + ph, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.08f, 0.08f, 0.10f, 0.78f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ③ 3 按钮：2 人/3 人/4 人（纯色） */
    static const float BTN_COL[3][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 2 人 蓝 */
        {0.30f, 0.70f, 0.50f, 0.9f},   /* 3 人 绿 */
        {0.95f, 0.80f, 0.20f, 0.9f},   /* 4 人 金（默认四国=4 人，金色=推荐） */
    };
    const ui_nplayer_btn *nb = ui_nplayer_buttons();
    for (int i = 0; i < UI_NPLAYER_COUNT; i++) {
        float bx0 = nb[i].x0 * sx, by0 = nb[i].y0 * sy;
        float bx1 = nb[i].x1 * sx, by1 = nb[i].y1 * sy;
        rs_v4f bq[6] = {
            {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
            {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ④ 3 按钮文字（nplayer_text_atlas，每按钮一 label，居中） */
    if (rr->tex_nplayer_text[slot] && rr->prog_tex[slot] && rr->nplayer_text_w > 0 && rr->nplayer_text_h > 0) {
        int row_h = rr->nplayer_text_h / UI_NPLAYER_COUNT;   /* 每行 46px */
        for (int i = 0; i < UI_NPLAYER_COUNT; i++) {
            float bx0 = nb[i].x0 * sx, by0 = nb[i].y0 * sy;
            float bx1 = nb[i].x1 * sx, by1 = nb[i].y1 * sy;
            float tw = 150.0f * sx, th = tw * ((float)row_h / rr->nplayer_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float u0 = 0.0f, u1 = 1.0f;
            float v0 = (float)(i * row_h) / rr->nplayer_text_h;
            float v1 = (float)(i * row_h + row_h) / rr->nplayer_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_nplayer_text[slot]);
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
    static int npdiag = 0;
    if (!npdiag) { npdiag = 1; LOGI("rs_draw_nplayer：四国人数选择弹窗（datah_089 2/3/4 人 3 按钮，面板 %.0f×%.0f，诚实边界）", UI_NPLAYER_POPUP_W, UI_NPLAYER_POPUP_H); }
}

/* ★ P0（datah_062 证据）：两人新局简化弹窗渲染。
   主菜单背景 + 面板深色半透明 + 4 控件（玩法/明暗棋 + 确定/取消）+ 文字图集。
   按钮几何 ui_newgame_buttons()（720×1280 基准）。★ 诚实边界：两人热座（AI 不介入）。 */
static void rs_draw_newgame(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪ 背景：主菜单背景（新局弹窗是主菜单上的模态框） */
    if (rr->tex_menu_bg[slot] && rr->prog_tex[slot]) {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f bg[6] = {
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
        glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ① 全屏模态遮罩 */
    rs_v4f mq[6] = {
        {0, 0, 0, 0}, {(float)rr->view_w, 0, 0, 0}, {0, (float)rr->view_h, 0, 0},
        {0, (float)rr->view_h, 0, 0}, {(float)rr->view_w, 0, 0, 0},
        {(float)rr->view_w, (float)rr->view_h, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.0f, 0.0f, 0.0f, 0.35f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mq), mq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ② 面板：深色半透明矩形（datah_062 450×315 居中，720 基准 135,482.5-585,797.5） */
    float px0 = UI_NEWGAME_POPUP_X0 * sx, py0 = UI_NEWGAME_POPUP_Y0 * sy;
    float pw = UI_NEWGAME_POPUP_W * sx, ph = UI_NEWGAME_POPUP_H * sy;
    rs_v4f pq[6] = {
        {px0, py0, 0, 0}, {px0 + pw, py0, 0, 0}, {px0, py0 + ph, 0, 0},
        {px0, py0 + ph, 0, 0}, {px0 + pw, py0, 0, 0}, {px0 + pw, py0 + ph, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.08f, 0.08f, 0.10f, 0.78f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ③ 4 控件：玩法/明暗棋（当前模式高亮金色）+ 确定/取消（纯色） */
    static const float BTN_COL[4][4] = {
        {0.45f, 0.30f, 0.75f, 0.9f},   /* 玩法 紫 */
        {0.45f, 0.30f, 0.75f, 0.9f},   /* 明暗棋 紫 */
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 确定 蓝 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
    };
    /* 玩法/明暗棋 2 个切换按钮当前模式标签行：row2=布局明棋 row3=翻棋暗棋 */
    const ui_newgame_btn *nb = ui_newgame_buttons();
    int cur_mode = ui_opt_get_mode();   /* 0=布局明棋 1=翻棋暗棋 */
    for (int i = 0; i < UI_NEWGAME_COUNT; i++) {
        float bx0 = nb[i].x0 * sx, by0 = nb[i].y0 * sy;
        float bx1 = nb[i].x1 * sx, by1 = nb[i].y1 * sy;
        rs_v4f bq[6] = {
            {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
            {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
        /* ★ 玩法/明暗棋 2 个切换按钮：当前选中模式金色高亮（非选中的画普通色） */
        if (i == 0 || i == 1) {
            float cr = 1.0f, cg = 0.85f, cb = 0.2f;
            glUniform4f(uc, cr, cg, cb, 0.15f);
            rs_v4f oq[6] = {
                {bx0 - 4*sx, by0 - 4*sy, 0, 0}, {bx1 + 4*sx, by0 - 4*sy, 0, 0}, {bx0 - 4*sx, by1 + 4*sy, 0, 0},
                {bx0 - 4*sx, by1 + 4*sy, 0, 0}, {bx1 + 4*sx, by0 - 4*sy, 0, 0}, {bx1 + 4*sx, by1 + 4*sy, 0, 0},
            };
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(oq), oq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
    }
    /* ④ 文字：标签列（玩法:/明暗棋: 2 项）+ 当前模式标签 + 确定/取消
       （newgame_text_atlas 6 行：row0=玩法 row1=明暗棋 row2=布局明棋 row3=翻棋暗棋 row4=确定 row5=取消） */
    if (rr->tex_newgame_text[slot] && rr->prog_tex[slot] && rr->newgame_text_w > 0 && rr->newgame_text_h > 0) {
        int row_h = rr->newgame_text_h / 6;   /* 每行 46px（6 标签竖排） */
        float u0 = 0.0f, u1 = 1.0f;
        /* 标签列：玩法/明暗棋 左侧 label（x 165-290，对齐原版 datah_062 标签位置） */
        for (int r = 0; r < 2; r++) {
            float tcy = (nb[r].y0 + nb[r].y1) * 0.5f * sy;
            float tw = 90.0f * sx, th = tw * ((float)row_h / rr->newgame_text_w);
            float tx0 = 165.0f * sx, ty0 = tcy - th * 0.5f;
            float v0 = (float)(r * row_h) / rr->newgame_text_h;
            float v1 = (float)(r * row_h + row_h) / rr->newgame_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_newgame_text[slot]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
        /* 当前模式标签（玩法/明暗棋 按钮内，row2=布局明棋 row3=翻棋暗棋，按当前 ui_opt_get_mode() 高亮对应行） */
        for (int r = 0; r < 2; r++) {
            int mode = ui_opt_get_mode();
            int row_idx = 2 + (mode ? 1 : 0);   /* 行 2=布局明棋 行 3=翻棋暗棋 */
            float bx0 = nb[r].x0 * sx, by0 = nb[r].y0 * sy;
            float bx1 = nb[r].x1 * sx, by1 = nb[r].y1 * sy;
            float tw = 150.0f * sx, th = tw * ((float)row_h / rr->newgame_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float v0 = (float)(row_idx * row_h) / rr->newgame_text_h;
            float v1 = (float)(row_idx * row_h + row_h) / rr->newgame_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_newgame_text[slot]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
        /* 确定/取消（row4-5） */
        for (int i = 2; i < UI_NEWGAME_COUNT; i++) {
            float bx0 = nb[i].x0 * sx, by0 = nb[i].y0 * sy;
            float bx1 = nb[i].x1 * sx, by1 = nb[i].y1 * sy;
            float tw = 120.0f * sx, th = tw * ((float)row_h / rr->newgame_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float v0 = (float)(i * row_h) / rr->newgame_text_h;
            float v1 = (float)(i * row_h + row_h) / rr->newgame_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_newgame_text[slot]);
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
    static int ngdiag = 0;
    if (!ngdiag) { ngdiag = 1; LOGI("rs_draw_newgame：两人新局弹窗（datah_062 玩法/明暗棋+确定/取消，面板 %.0f×%.0f，两人热座 AI 不介入）", UI_NEWGAME_POPUP_W, UI_NEWGAME_POPUP_H); }
}

/* ★ P1（datah_012 证据）：游戏设置弹窗渲染。
   对局棋盘背景 + 面板深色半透明 + 6 控件（4 规则开关 + 确定/取消）+ 文字图集。
   按钮几何 ui_gameset_buttons()（720×1280 基准）。4 开关各显示当前"开/关"状态
   （ui_rule_cfg_get 0=和棋 1=大本营 2=吃地雷 3=吃军旗，默认全开对齐原版默认）。
   ★ 诚实边界：开关"开"状态的原版精确可选值 .rodata 未读全，默认全开。 */
static void rs_draw_gameset(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪ 背景：对局棋盘背景（游戏设置弹窗从 ☰ 菜单弹出，背景为对局） */
    if (rr->tex_bg[slot] && rr->prog_tex[slot]) {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f bg[6] = {
            {0, 0, 0, 0}, {w, 0, 1, 0}, {0, h, 0, 1},
            {0, h, 0, 1}, {w, 0, 1, 0}, {w, h, 1, 1},
        };
        glUseProgram(rr->prog_tex[slot]);
        glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rr->tex_bg[slot]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ① 全屏模态遮罩 */
    rs_v4f mq[6] = {
        {0, 0, 0, 0}, {(float)rr->view_w, 0, 0, 0}, {0, (float)rr->view_h, 0, 0},
        {0, (float)rr->view_h, 0, 0}, {(float)rr->view_w, 0, 0, 0},
        {(float)rr->view_w, (float)rr->view_h, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.0f, 0.0f, 0.0f, 0.35f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mq), mq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ② 面板：深色半透明矩形（datah_012 450×465 居中，720 基准 135,407.5-585,872.5） */
    float px0 = UI_GAMESET_POPUP_X0 * sx, py0 = UI_GAMESET_POPUP_Y0 * sy;
    float pw = UI_GAMESET_POPUP_W * sx, ph = UI_GAMESET_POPUP_H * sy;
    rs_v4f pq[6] = {
        {px0, py0, 0, 0}, {px0 + pw, py0, 0, 0}, {px0, py0 + ph, 0, 0},
        {px0, py0 + ph, 0, 0}, {px0 + pw, py0, 0, 0}, {px0 + pw, py0 + ph, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.08f, 0.08f, 0.10f, 0.78f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ③ 6 控件：4 开关（开=绿 / 关=灰红）+ 确定/取消（纯色） */
    static const float BTN_COL[6][4] = {
        {0.30f, 0.70f, 0.50f, 0.9f},   /* 和棋 开关 */
        {0.30f, 0.70f, 0.50f, 0.9f},   /* 大本营 开关 */
        {0.30f, 0.70f, 0.50f, 0.9f},   /* 吃地雷 开关 */
        {0.30f, 0.70f, 0.50f, 0.9f},   /* 吃军旗 开关 */
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 确定 蓝 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
    };
    const ui_gameset_btn *gb = ui_gameset_buttons();
    for (int i = 0; i < UI_GAMESET_COUNT; i++) {
        float bx0 = gb[i].x0 * sx, by0 = gb[i].y0 * sy;
        float bx1 = gb[i].x1 * sx, by1 = gb[i].y1 * sy;
        rs_v4f bq[6] = {
            {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
            {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        /* 4 开关：开=绿色 / 关=红灰（状态可视化）；确定/取消纯色 */
        if (i < 4)
            glUniform4f(uc, ui_rule_cfg_get(i) ? BTN_COL[i][0] : 0.85f,
                        ui_rule_cfg_get(i) ? BTN_COL[i][1] : 0.35f,
                        ui_rule_cfg_get(i) ? BTN_COL[i][2] : 0.35f, 0.9f);
        else
            glUniform4f(uc, BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ④ 文字（gameset_text_atlas 8 行：row0=和棋 row1=大本营 row2=吃地雷 row3=吃军旗
       row4=开 row5=关 row6=确定 row7=取消） */
    if (rr->tex_gameset_text[slot] && rr->prog_tex[slot] && rr->gameset_text_w > 0 && rr->gameset_text_h > 0) {
        int row_h = rr->gameset_text_h / 8;   /* 每行 46px */
        float u0 = 0.0f, u1 = 1.0f;
        /* 标签列：和棋:/大本营:/吃地雷:/吃军旗: 左侧 label（x 200-320，对齐原版 datah_012 标签位置） */
        for (int r = 0; r < 4; r++) {
            float tcy = (gb[r].y0 + gb[r].y1) * 0.5f * sy;
            float tw = 100.0f * sx, th = tw * ((float)row_h / rr->gameset_text_w);
            float tx0 = 195.0f * sx, ty0 = tcy - th * 0.5f;
            float v0 = (float)(r * row_h) / rr->gameset_text_h;
            float v1 = (float)(r * row_h + row_h) / rr->gameset_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_gameset_text[slot]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
        /* 值标签：开/关（row4/row5，按各开关当前状态） */
        for (int r = 0; r < 4; r++) {
            int val_row = ui_rule_cfg_get(r) ? 4 : 5;
            float bx0 = gb[r].x0 * sx, by0 = gb[r].y0 * sy;
            float bx1 = gb[r].x1 * sx, by1 = gb[r].y1 * sy;
            float tw = 60.0f * sx, th = tw * ((float)row_h / rr->gameset_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float v0 = (float)(val_row * row_h) / rr->gameset_text_h;
            float v1 = (float)(val_row * row_h + row_h) / rr->gameset_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_gameset_text[slot]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
        /* 确定/取消（row6-7） */
        for (int i = 4; i < UI_GAMESET_COUNT; i++) {
            float bx0 = gb[i].x0 * sx, by0 = gb[i].y0 * sy;
            float bx1 = gb[i].x1 * sx, by1 = gb[i].y1 * sy;
            float tw = 120.0f * sx, th = tw * ((float)row_h / rr->gameset_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float v0 = (float)((i + 2) * row_h) / rr->gameset_text_h;
            float v1 = (float)((i + 2) * row_h + row_h) / rr->gameset_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_gameset_text[slot]);
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
    static int gsdiag = 0;
    if (!gsdiag) { gsdiag = 1; LOGI("rs_draw_gameset：游戏设置弹窗（datah_012 4 规则开关 和棋=%d 大本营=%d 地雷=%d 扛旗=%d + 确定/取消，面板 %.0f×%.0f）",
        ui_rule_cfg_get(0), ui_rule_cfg_get(1), ui_rule_cfg_get(2), ui_rule_cfg_get(3), UI_GAMESET_POPUP_W, UI_GAMESET_POPUP_H); }
}

/* 布局存档弹窗共用：全屏模态遮罩 + 深色半透明面板（ui_menu.h 面板宏，720 基准 ×sx/sy） */
static void rs_layres_panel(rs_render *rr, int slot, int ui, GLint uc)
{
    float px0, py0, pw, ph;
    if (ui == UI_LOAD_LAY)
        px0 = UI_LOADLAYOUT_POPUP_X0, py0 = UI_LOADLAYOUT_POPUP_Y0, pw = UI_LOADLAYOUT_POPUP_W, ph = UI_LOADLAYOUT_POPUP_H;
    else if (ui == UI_SAVE_LAY)
        px0 = UI_SAVELAYOUT_POPUP_X0, py0 = UI_SAVELAYOUT_POPUP_Y0, pw = UI_SAVELAYOUT_POPUP_W, ph = UI_SAVELAYOUT_POPUP_H;
    else
        px0 = UI_LAYMGR_POPUP_X0, py0 = UI_LAYMGR_POPUP_Y0, pw = UI_LAYMGR_POPUP_W, ph = UI_LAYMGR_POPUP_H;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    /* ⓪ 背景：对局棋盘背景（布局存档弹窗从 ☰/对局弹出，背景为对局） */
    if (rr->tex_bg[slot] && rr->prog_tex[slot]) {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f bg[6] = {
            {0, 0, 0, 0}, {w, 0, 1, 0}, {0, h, 0, 1},
            {0, h, 0, 1}, {w, 0, 1, 0}, {w, h, 1, 1},
        };
        glUseProgram(rr->prog_tex[slot]);
        glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rr->tex_bg[slot]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ① 全屏模态遮罩 */
    rs_v4f mq[6] = {
        {0, 0, 0, 0}, {(float)rr->view_w, 0, 0, 0}, {0, (float)rr->view_h, 0, 0},
        {0, (float)rr->view_h, 0, 0}, {(float)rr->view_w, 0, 0, 0},
        {(float)rr->view_w, (float)rr->view_h, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.0f, 0.0f, 0.0f, 0.35f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mq), mq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ② 面板：深色半透明矩形 */
    float X0 = px0 * sx, Y0 = py0 * sy;
    float W = pw * sx, H = ph * sy;
    rs_v4f pq[6] = {
        {X0, Y0, 0, 0}, {X0 + W, Y0, 0, 0}, {X0, Y0 + H, 0, 0},
        {X0, Y0 + H, 0, 0}, {X0 + W, Y0, 0, 0}, {X0 + W, Y0 + H, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.08f, 0.08f, 0.10f, 0.78f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
}

/* 布局存档文字图集 9 行 row 索引（layres_text_atlas 220×414，每行 46px，row_h=h/9）：
   row0=载入布局 row1=保存布局 row2=布局管理 row3=取消 row4=保存 row5=改名
   row6=删除 row7=关闭 row8=载入 */
#define LAYRES_ROW 9
static void rs_layres_text(rs_render *rr, int slot, float x, float y, float w,
                           float h, int row, GLint uc)
{
    (void)uc;
    if (!rr->tex_layres_text[slot] || !rr->prog_tex[slot] ||
        rr->layres_text_w <= 0 || rr->layres_text_h <= 0) return;
    int row_h = rr->layres_text_h / LAYRES_ROW;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    float tw = w * sx, th = h * sy;
    float u0 = 0.0f, u1 = 1.0f;
    float v0 = (float)(row * row_h) / rr->layres_text_h;
    float v1 = (float)(row * row_h + row_h) / rr->layres_text_h;
    rs_v4f tq[6] = {
        {x * sx, y * sy, u0, v0}, {x * sx + tw, y * sy, u1, v0}, {x * sx, y * sy + th, u0, v1},
        {x * sx, y * sy + th, u0, v1}, {x * sx + tw, y * sy, u1, v0}, {x * sx + tw, y * sy + th, u1, v1},
    };
    glUseProgram(rr->prog_tex[slot]);
    glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rr->tex_layres_text[slot]);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
}

/* 布局列表行（017/020 共用）：6 行文本列表区（180,447-540,807 720 基准，行高 60px），
   行底色（选中=金色高亮 / 未选中=深蓝）+ 布局名位图无法烘焙动态文本 →
   简化：行底色 + 序号纯色块占位（★ 诚实边界：ListBox 文本渲染未接动态名，占位块）。 */
static void rs_layres_list_rows(rs_render *rr, int slot, int sel, GLint uc)
{
    const ui_loadlayout_btn *lb = ui_loadlayout_buttons();   /* 前 6 项 = 列表行 */
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    int n = ui_layout_count() > 6 ? 6 : ui_layout_count();
    for (int i = 0; i < 6; i++) {
        float bx0 = lb[i].x0 * sx, by0 = lb[i].y0 * sy;
        float bx1 = lb[i].x1 * sx, by1 = lb[i].y1 * sy;
        int active = (i < n);
        int is_sel = active && (i == ui_layout_selected());
        rs_v4f bq[6] = {
            {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
            {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        if (is_sel) glUniform4f(uc, 1.0f, 0.85f, 0.2f, 0.95f);   /* 选中金色高亮 */
        else if (active) glUniform4f(uc, 0.30f, 0.40f, 0.60f, 0.90f);  /* 未选中深蓝 */
        else glUniform4f(uc, 0.15f, 0.15f, 0.18f, 0.60f);        /* 无条目灰暗占位 */
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
}

/* 布局管理 30 格棋子预览（016，简化小方块 6×5，180,477-540,762 720 基准区）：
   选中布局载入临时 state → jq_extract_layout_30 → 每格画蓝/红小方块（有子）/深灰（空）。
   ★ 诚实边界：30 格 PiecePic 真棋子图未画，小方块占位（原版 Piece0-29 为棋子图）。 */
static void rs_layres_preview(rs_render *rr, int slot, GLint uc)
{
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    uint8_t lay[30];
    int have = 0;
    if (ui_layout_selected_valid()) {
        const char *nm = ui_layout_name(ui_layout_selected());
        if (nm && ui_layout_load(nm, lay) == 0) have = 1;
    }
    /* 预览区 6 行 × 5 列，行 75→360（400 系）= 477→762（720 系），格 40×34（720 基准） */
    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 5; c++) {
            float cx = (220.0f + c * 70.0f) * sx;
            float cy = (517.0f + r * 48.0f) * sy;
            float s = 40.0f * sx, sh = 34.0f * sy;
            rs_v4f sq[6] = {
                {cx, cy, 0, 0}, {cx + s, cy, 0, 0}, {cx, cy + sh, 0, 0},
                {cx, cy + sh, 0, 0}, {cx + s, cy, 0, 0}, {cx + s, cy + sh, 0, 0},
            };
            glUseProgram(rr->prog_solid[slot]);
            glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            int filled = have && lay[r * 5 + c] != 0;
            glUniform4f(uc,
                        filled ? 0.35f : 0.30f,
                        filled ? 0.55f : 0.30f,
                        filled ? 0.95f : 0.32f,   /* 有子=蓝底；★ 诚实边界：真棋子图未画 */
                        filled ? 0.9f : 0.55f);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(sq), sq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
    }
    static int pdiag = 0;
    if (!pdiag) { pdiag = 1; LOGI("rs_layres_preview：30 格棋子预览（小方块占位，选中=%s）",
        ui_layout_selected_valid() ? ui_layout_name(ui_layout_selected()) : "无"); }
}

/* ★ P1（datah_017 证据）：载入布局弹窗渲染（布局名文本列表 6 行 + 取消按钮）。
   面板 400×535 居中（偏移+160,+372.5）。业务（jni_bridge）：点列表项=载入 + 回对局；
   取消/面板外=回对局。★ 诚实边界：ListBox 滚动未实现，6 条截断 + 选中金色高亮。 */
static void rs_draw_loadlay(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_layres_panel(rr, slot, UI_LOAD_LAY, uc);
    /* 标题"载入布局"（layres row0，面板顶部 160,392-560,442 720 基准区） */
    rs_layres_text(rr, slot, 255.0f, 402.0f, 150.0f, 46.0f, 0, uc);
    /* 布局名文本列表（前 6 条，选中金色高亮） */
    rs_layres_list_rows(rr, slot, ui_layout_selected(), uc);
    /* 取消按钮（280,828-440,888，row3 取消 文字居中） */
    {
        const ui_loadlayout_btn *lb = ui_loadlayout_buttons();
        float bx0 = lb[6].x0 * (float)rr->view_w / 720.0f;
        float by0 = lb[6].y0 * (float)rr->view_h / 1280.0f;
        float bx1 = lb[6].x1 * (float)rr->view_w / 720.0f;
        float by1 = lb[6].y1 * (float)rr->view_h / 1280.0f;
        rs_v4f bq[6] = {
            {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
            {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, 0.55f, 0.55f, 0.60f, 0.9f);   /* 取消 灰 */
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
        /* 取消 文字（row3，按钮内居中） */
        if (rr->tex_layres_text[slot] && rr->prog_tex[slot] &&
            rr->layres_text_w > 0 && rr->layres_text_h > 0) {
            int row_h = rr->layres_text_h / LAYRES_ROW;
            float tw = 60.0f * (float)rr->view_w / 720.0f;
            float th = tw * ((float)row_h / rr->layres_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f;
            float ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float u0 = 0.0f, u1 = 1.0f;
            float v0 = (float)(3 * row_h) / rr->layres_text_h;
            float v1 = (float)(3 * row_h + row_h) / rr->layres_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_layres_text[slot]);
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
    static int ldiag = 0;
    if (!ldiag) { ldiag = 1; LOGI("rs_draw_loadlay：载入布局弹窗（datah_017 布局列表 %d 条（前 6 显示）+ 取消，面板 400×535，ListBox 滚动简化）",
        ui_layout_count()); }
}

/* ★ P1（datah_020 证据）：保存布局弹窗渲染（布局名列表 + 保存/改名/删除/关闭 4 按钮）。
   面板 400×535 居中。业务（jni_bridge）：保存=当前棋盘 30 字节 .lay；改名=旧名+数字后缀；
   删除=删选中项；关闭=回对局。★ 诚实边界：改名自动编号（无文本输入框）。 */
static void rs_draw_savelay(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_layres_panel(rr, slot, UI_SAVE_LAY, uc);
    /* 标题"保存布局"（row1） */
    rs_layres_text(rr, slot, 255.0f, 402.0f, 150.0f, 46.0f, 1, uc);
    /* 布局名列表（同 017 区，选中金色高亮） */
    rs_layres_list_rows(rr, slot, ui_layout_selected(), uc);
    /* 4 按钮：保存（蓝）/ 改名（紫）/ 删除（红）/ 关闭（灰）+ 文字 */
    static const float BTN_COL[UI_SAVELAYOUT_COUNT][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 保存 蓝 */
        {0.45f, 0.30f, 0.75f, 0.9f},   /* 改名 紫 */
        {0.85f, 0.25f, 0.25f, 0.9f},   /* 删除 红 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 关闭 灰 */
    };
    const ui_savelayout_btn *sb = ui_savelayout_buttons();
    for (int i = 0; i < UI_SAVELAYOUT_COUNT; i++) {
        float bx0 = sb[i].x0 * (float)rr->view_w / 720.0f;
        float by0 = sb[i].y0 * (float)rr->view_h / 1280.0f;
        float bx1 = sb[i].x1 * (float)rr->view_w / 720.0f;
        float by1 = sb[i].y1 * (float)rr->view_h / 1280.0f;
        rs_v4f bq[6] = {
            {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
            {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
        /* 按钮文字（row4=保存 row5=改名 row6=删除 row7=关闭，按钮内居中） */
        if (rr->tex_layres_text[slot] && rr->prog_tex[slot] &&
            rr->layres_text_w > 0 && rr->layres_text_h > 0) {
            int row_h = rr->layres_text_h / LAYRES_ROW;
            int row_idx = 4 + i;
            float tw = 55.0f * (float)rr->view_w / 720.0f;
            float th = tw * ((float)row_h / rr->layres_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f;
            float ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float u0 = 0.0f, u1 = 1.0f;
            float v0 = (float)(row_idx * row_h) / rr->layres_text_h;
            float v1 = (float)(row_idx * row_h + row_h) / rr->layres_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_layres_text[slot]);
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
    static int sdiag = 0;
    if (!sdiag) { sdiag = 1; LOGI("rs_draw_savelay：保存布局弹窗（datah_020 保存/改名/删除/关闭 4 按钮，面板 400×535，改名自动编号简化）"); }
}

/* ★ P1（datah_016 证据）：布局管理弹窗渲染（30 格棋子预览小方块 + 载入/删除/取消 3 按钮）。
   面板 400×475 居中（偏移+160,+4"); }2.5）。业务（jni_bridge）：载入=应用到棋盘；
   删除=删选中项；取消=回对局。★ 诚实边界：PiecePic 真棋子图未画（小方块占位）。 */
static void rs_draw_laymgr(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_layres_panel(rr, slot, UI_LAY_MGR, uc);
    /* 标题"布局管理"（row2，面板顶部） */
    rs_layres_text(rr, slot, 255.0f, 432.0f, 150.0f, 46.0f, 2, uc);
    /* 30 格棋子预览（小方块 6×5 占位） */
    rs_layres_preview(rr, slot, uc);
    /* 3 按钮：载入（蓝）/ 删除（红）/ 取消（灰）+ 文字 */
    static const float BTN_COL[UI_LAYMGR_COUNT][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 载入 蓝 */
        {0.85f, 0.25f, 0.25f, 0.9f},   /* 删除 红 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
    };
    const ui_laymgr_btn *mb = ui_laymgr_buttons();
    for (int i = 0; i < UI_LAYMGR_COUNT; i++) {
        float bx0 = mb[i].x0 * (float)rr->view_w / 720.0f;
        float by0 = mb[i].y0 * (float)rr->view_h / 1280.0f;
        float bx1 = mb[i].x1 * (float)rr->view_w / 720.0f;
        float by1 = mb[i].y1 * (float)rr->view_h / 1280.0f;
        rs_v4f bq[6] = {
            {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
            {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
        /* 按钮文字（row8=载入 row6=删除 row3=取消，按钮内居中） */
        if (rr->tex_layres_text[slot] && rr->prog_tex[slot] &&
            rr->layres_text_w > 0 && rr->layres_text_h > 0) {
            int row_h = rr->layres_text_h / LAYRES_ROW;
            int row_idx = (i == 0) ? 8 : (i == 1 ? 6 : 3);
            float tw = 55.0f * (float)rr->view_w / 720.0f;
            float th = tw * ((float)row_h / rr->layres_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f;
            float ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float u0 = 0.0f, u1 = 1.0f;
            float v0 = (float)(row_idx * row_h) / rr->layres_text_h;
            float v1 = (float)(row_idx * row_h + row_h) / rr->layres_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_layres_text[slot]);
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
    static int mdiag = 0;
    if (!mdiag) { mdiag = 1; LOGI("rs_draw_laymgr：布局管理弹窗（datah_016 30 格预览（小方块占位）+ 载入/删除/取消，面板 400×475）"); }
}

/* ★ P1（datah_107 证据）：复盘回放渲染——棋盘（当前回放位置，棋子占位方块范式）
   + datah_107 底部 5 按钮控制条（菜单/回退/前进/跳转/返回，460×60 @10,705 不居中）
   + 步数标签（"复盘" 标题 + "第cur/n步" 位置行）。
   业务（jni_bridge OnTouch）：回退 cur-- 重建 / 前进 cur++ 走 1 步 / 跳转 回开头/跳结尾
   循环 / 返回/菜单 退复盘回对局。★ 诚实边界：跳转无步进输入框；菜单无子菜单；
   复盘态不画真实棋子图（复用棋盘现有占位方块范式，棋子图集缺失则退化纯色格）。 */
/* 复盘文字图集 8 行 row 索引（replay_text_atlas 220×368，每行 46px，row_h=h/8）：
   row0=复盘 row1=菜单 row2=回退 row3=前进 row4=跳转 row5=返回 row6=第步 row7=结束 */
#define REPLAY_ROW 8
static void rs_replay_text(rs_render *rr, int slot, float x, float y, float w,
                           float h, int row, GLint uc)
{
    (void)uc;
    if (!rr->tex_replay_text[slot] || !rr->prog_tex[slot] ||
        rr->replay_text_w <= 0 || rr->replay_text_h <= 0) return;
    int row_h = rr->replay_text_h / REPLAY_ROW;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    float tw = w * sx, th = h * sy;
    float u0 = 0.0f, u1 = 1.0f;
    float v0 = (float)(row * row_h) / rr->replay_text_h;
    float v1 = (float)(row * row_h + row_h) / rr->replay_text_h;
    rs_v4f tq[6] = {
        {x * sx, y * sy, u0, v0}, {x * sx + tw, y * sy, u1, v0}, {x * sx, y * sy + th, u0, v1},
        {x * sx, y * sy + th, u0, v1}, {x * sx + tw, y * sy, u1, v0}, {x * sx + tw, y * sy + th, u1, v1},
    };
    glUseProgram(rr->prog_tex[slot]);
    glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rr->tex_replay_text[slot]);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
}

/* 复盘态棋子占位方块（60 格有子 → 蓝/红小方块，居中格区，不复用 rs_draw_board
   棋子图集——复盘态诚实边界不画真棋子图） */
static void rs_replay_pieces(rs_render *rr, int slot, GLint uc)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot] || !rr->game) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    /* 棋盘主体格区（720 基准）：中心 60 格 6 列 × 10 行，每格 70×88 */
    float x0 = 190.0f * sx, y0 = 340.0f * sy;
    float cw = 70.0f * sx, ch = 88.0f * sy;
    for (int i = 0; i < N_CELLS; i++) {
        const jq_cell *c = &rr->game->cells[i];
        if (c->rank == RANK_EMPTY) continue;
        int cr = cell_row(i), cc = cell_col(i);
        /* cell_row 0=底（渲染 y 向下）：第 row 格 → 屏幕 y = y0 + (11-row)*ch */
        float bx = x0 + cc * cw + cw * 0.20f;
        float by = y0 + (11 - cr) * ch + ch * 0.10f;
        rs_v4f pq[6] = {
            {bx, by, 0, 0}, {bx + cw * 0.6f, by, 0, 0}, {bx, by + ch * 0.8f, 0, 0},
            {bx, by + ch * 0.8f, 0, 0}, {bx + cw * 0.6f, by, 0, 0}, {bx + cw * 0.6f, by + ch * 0.8f, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, c->side == SIDE_BLUE ? 0.30f : 0.85f,
                    c->side == SIDE_BLUE ? 0.50f : 0.20f,
                    c->side == SIDE_BLUE ? 0.90f : 0.20f, 0.85f);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
}

/* ★ P1（datah_038 证据）：登录弹窗渲染——主菜单背景 + 面板深色半透明
   + 用户名/密码占位标签（EditLabel 不画真输入框）+ 登录/注册/游客/返回 4 按钮 + 文字图集。
   按钮几何 ui_login_buttons()（720×1280 基准）。
   ★ 诚实边界：原版登录=账号系统（登录/注册 需服务器协议，src_rs 未逆向）；
   游客=本地单机（无账号，唯一真生效路径）；输入框简化为占位标签（触摸 flaky）。 */
#define LOGIN_ROW 8
static void rs_login_text(rs_render *rr, int slot, float x, float y, float w,
                          float h, int row, GLint uc)
{
    (void)uc;
    if (!rr->tex_login_text[slot] || !rr->prog_tex[slot] ||
        rr->login_text_w <= 0 || rr->login_text_h <= 0) return;
    int row_h = rr->login_text_h / LOGIN_ROW;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    float tw = w * sx, th = h * sy;
    float u0 = 0.0f, u1 = 1.0f;
    float v0 = (float)(row * row_h) / rr->login_text_h;
    float v1 = (float)(row * row_h + row_h) / rr->login_text_h;
    rs_v4f tq[6] = {
        {x * sx, y * sy, u0, v0}, {x * sx + tw, y * sy, u1, v0}, {x * sx, y * sy + th, u0, v1},
        {x * sx, y * sy + th, u0, v1}, {x * sx + tw, y * sy, u1, v0}, {x * sx + tw, y * sy + th, u1, v1},
    };
    glUseProgram(rr->prog_tex[slot]);
    glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rr->tex_login_text[slot]);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
}

static void rs_draw_login(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪ 背景：主菜单背景（登录弹窗是主菜单上的模态框，datah_072 方式弹窗"个人资料"弹出） */
    if (rr->tex_menu_bg[slot] && rr->prog_tex[slot]) {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f bg[6] = {
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
        glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ① 全屏模态遮罩（半透明黑，聚焦弹窗） */
    rs_v4f mq[6] = {
        {0, 0, 0, 0}, {(float)rr->view_w, 0, 0, 0}, {0, (float)rr->view_h, 0, 0},
        {0, (float)rr->view_h, 0, 0}, {(float)rr->view_w, 0, 0, 0},
        {(float)rr->view_w, (float)rr->view_h, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.0f, 0.0f, 0.0f, 0.35f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mq), mq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ② 面板：深色半透明矩形（datah_038 432×285 居中，720 基准 144,497.5-576,782.5） */
    float px0 = UI_LOGIN_POPUP_X0 * sx, py0 = UI_LOGIN_POPUP_Y0 * sy;
    float pw = UI_LOGIN_POPUP_W * sx, ph = UI_LOGIN_POPUP_H * sy;
    rs_v4f pq[6] = {
        {px0, py0, 0, 0}, {px0 + pw, py0, 0, 0}, {px0, py0 + ph, 0, 0},
        {px0, py0 + ph, 0, 0}, {px0 + pw, py0, 0, 0}, {px0 + pw, py0 + ph, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.08f, 0.08f, 0.10f, 0.78f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ③ 标题"登录"（login_text row0，面板顶部 285,512-435,558 720 基准区） */
    rs_login_text(rr, slot, 285.0f, 512.0f, 150.0f, 46.0f, 0, uc);
    /* ④ 用户名/密码 占位 EditLabel（矩形浅色底 + 占位文字，★ 诚实边界：不画真输入框） */
    const ui_login_btn *lb = ui_login_buttons();
    for (int i = 0; i < 2; i++) {
        float bx0 = lb[i].x0 * sx, by0 = lb[i].y0 * sy;
        float bx1 = lb[i].x1 * sx, by1 = lb[i].y1 * sy;
        rs_v4f eq[6] = {
            {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
            {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, 0.90f, 0.90f, 0.92f, 0.92f);   /* 浅色输入框底 */
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(eq), eq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
        /* 占位标签文字（row1=用户名 row2=密码，输入框内左对齐） */
        float tw = 90.0f * sx, th = tw * ((float)(rr->login_text_h / LOGIN_ROW) / rr->login_text_w);
        float tx0 = bx0 + 14.0f * sx, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
        float u0 = 0.0f, u1 = 1.0f;
        int row_h = rr->login_text_h / LOGIN_ROW;
        float v0 = (float)((1 + i) * row_h) / rr->login_text_h;
        float v1 = (float)((1 + i) * row_h + row_h) / rr->login_text_h;
        rs_v4f tq[6] = {
            {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
            {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
        };
        if (rr->tex_login_text[slot] && rr->prog_tex[slot]) {
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_login_text[slot]);
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
    /* ⑤ 4 按钮：登录（蓝）/ 注册（绿）/ 游客（金）/ 返回（灰）纯色 */
    static const float BTN_COL[4][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 登录 蓝 */
        {0.30f, 0.70f, 0.50f, 0.9f},   /* 注册 绿 */
        {0.95f, 0.80f, 0.20f, 0.9f},   /* 游客 金（本地单机唯一真生效路径） */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 返回 灰 */
    };
    for (int i = 2; i < UI_LOGIN_COUNT; i++) {
        float bx0 = lb[i].x0 * sx, by0 = lb[i].y0 * sy;
        float bx1 = lb[i].x1 * sx, by1 = lb[i].y1 * sy;
        rs_v4f bq[6] = {
            {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
            {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, BTN_COL[i - 2][0], BTN_COL[i - 2][1], BTN_COL[i - 2][2], BTN_COL[i - 2][3]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
        /* 按钮文字（row3=注册 row4=游客 row5=返回；登录文字 row1 已画在用户名框，
           登录按钮文字用 row6"账户"占位——★ 诚实边界：登录/注册 未逆向服务器协议） */
        int row_idx = (i == 2) ? 6 : (i == 3 ? 3 : (i == 4 ? 4 : 5));
        float tw = 60.0f * sx, th = tw * ((float)(rr->login_text_h / LOGIN_ROW) / rr->login_text_w);
        float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
        float u0 = 0.0f, u1 = 1.0f;
        int row_h = rr->login_text_h / LOGIN_ROW;
        float v0 = (float)(row_idx * row_h) / rr->login_text_h;
        float v1 = (float)(row_idx * row_h + row_h) / rr->login_text_h;
        rs_v4f tq[6] = {
            {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
            {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
        };
        if (rr->tex_login_text[slot] && rr->prog_tex[slot] && rr->login_text_w > 0 && rr->login_text_h > 0) {
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_login_text[slot]);
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
    static int ldiag = 0;
    if (!ldiag) {
        ldiag = 1;
        LOGI("rs_draw_login：登录弹窗（datah_038 用户名/密码占位标签 + 登录/注册/游客/返回 4 按钮，面板 %.0f×%.0f，"
             "诚实边界：账号系统/登录注册服务器协议未逆向，游客=本地单机唯一真生效路径）",
             UI_LOGIN_POPUP_W, UI_LOGIN_POPUP_H);
    }
}

/* ★ P2（datah_037 证据）：规则文本弹窗渲染——主菜单背景 + 面板深色半透明
   + 规则-翻棋/规则-布局 2 模式切换按钮 + 返回 + 文字图集。
   按钮几何 ui_rules_buttons()（720×1280 基准）。
   ★ 诚实边界：规则文本=src_rs 现有简化版固定文本（2 套：翻棋/布局）。 */
#define RULES_ROW 8
static void rs_rules_text(rs_render *rr, int slot, float x, float y, float w,
                           float h, int row, GLint uc)
{
    (void)uc;
    if (!rr->tex_rules_text[slot] || !rr->prog_tex[slot] ||
        rr->rules_text_w <= 0 || rr->rules_text_h <= 0) return;
    int row_h = rr->rules_text_h / RULES_ROW;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    float tw = w * sx, th = h * sy;
    float u0 = 0.0f, u1 = 1.0f;
    float v0 = (float)(row * row_h) / rr->rules_text_h;
    float v1 = (float)(row * row_h + row_h) / rr->rules_text_h;
    rs_v4f tq[6] = {
        {x * sx, y * sy, u0, v0}, {x * sx + tw, y * sy, u1, v0}, {x * sx, y * sy + th, u0, v1},
        {x * sx, y * sy + th, u0, v1}, {x * sx + tw, y * sy, u1, v0}, {x * sx + tw, y * sy + th, u1, v1},
    };
    glUseProgram(rr->prog_tex[slot]);
    glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rr->tex_rules_text[slot]);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
}

static void rs_draw_rules(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪ 背景：主菜单背景（规则文本弹窗是主菜单上的模态框） */
    if (rr->tex_menu_bg[slot] && rr->prog_tex[slot]) {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f bg[6] = {
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
        glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ① 全屏模态遮罩 */
    rs_v4f mq[6] = {
        {0, 0, 0, 0}, {(float)rr->view_w, 0, 0, 0}, {0, (float)rr->view_h, 0, 0},
        {0, (float)rr->view_h, 0, 0}, {(float)rr->view_w, 0, 0, 0},
        {(float)rr->view_w, (float)rr->view_h, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.0f, 0.0f, 0.0f, 0.35f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mq), mq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ② 面板：深色半透明矩形（datah_037 260×270 居中，720 基准 230,505-490,775） */
    float px0 = UI_RULES_POPUP_X0 * sx, py0 = UI_RULES_POPUP_Y0 * sy;
    float pw = UI_RULES_POPUP_W * sx, ph = UI_RULES_POPUP_H * sy;
    rs_v4f pq[6] = {
        {px0, py0, 0, 0}, {px0 + pw, py0, 0, 0}, {px0, py0 + ph, 0, 0},
        {px0, py0 + ph, 0, 0}, {px0 + pw, py0, 0, 0}, {px0 + pw, py0 + ph, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.08f, 0.08f, 0.10f, 0.78f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ③ 标题"规则"（rules_text row0，面板顶部 310,520-440,566 720 基准区） */
    rs_rules_text(rr, slot, 310.0f, 520.0f, 130.0f, 46.0f, 0, uc);
    /* ④ 3 按钮：规则-翻棋（紫）/ 规则-布局（蓝）/ 返回（灰）纯色 */
    static const float BTN_COL[3][4] = {
        {0.45f, 0.30f, 0.75f, 0.9f},   /* 规则-翻棋 紫 */
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 规则-布局 蓝 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 返回 灰 */
    };
    const ui_rules_btn *rb = ui_rules_buttons();
    for (int i = 0; i < UI_RULES_COUNT; i++) {
        float bx0 = rb[i].x0 * sx, by0 = rb[i].y0 * sy;
        float bx1 = rb[i].x1 * sx, by1 = rb[i].y1 * sy;
        rs_v4f bq[6] = {
            {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
            {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
        /* 按钮文字（row1=翻棋 row2=布局 row3=工兵 占位，按钮内居中；★ 诚实边界：
           规则文本简化版，row3 工兵标签占位） */
        int row_idx = (i == 0) ? 1 : (i == 1 ? 2 : 3);
        float tw = 120.0f * sx, th = tw * ((float)(rr->rules_text_h / RULES_ROW) / rr->rules_text_w);
        float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
        if (rr->tex_rules_text[slot] && rr->prog_tex[slot] && rr->rules_text_w > 0 && rr->rules_text_h > 0) {
            int row_h = rr->rules_text_h / RULES_ROW;
            float u0 = 0.0f, u1 = 1.0f;
            float v0 = (float)(row_idx * row_h) / rr->rules_text_h;
            float v1 = (float)(row_idx * row_h + row_h) / rr->rules_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_rules_text[slot]);
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
    static int rdr = 0;
    if (!rdr) {
        rdr = 1;
        LOGI("rs_draw_rules：规则文本弹窗（datah_037 规则-翻棋/规则-布局 2 模式 + 返回，面板 %.0f×%.0f，"
             "诚实边界：规则文本=src_rs 简化版固定文本 2 套）",
             UI_RULES_POPUP_W, UI_RULES_POPUP_H);
    }
}

/* ★ P2（datah_104 证据）：新局完整版弹窗渲染——主菜单背景 + 面板深色半透明
   + 我方/电脑水平/玩法 3 选择 + 确定/取消 + 文字图集。
   按钮几何 ui_newgame104_buttons()（720×1280 基准）。
   ★ 诚实边界：3 项各循环切换当前值（对齐 datah_001 音量循环 + datah_012 开关循环范式）。 */
#define NG104_ROW 10
static void rs_ng104_text(rs_render *rr, int slot, float x, float y, float w,
                           float h, int row, GLint uc)
{
    (void)uc;
    if (!rr->tex_ng104_text[slot] || !rr->prog_tex[slot] ||
        rr->ng104_text_w <= 0 || rr->ng104_text_h <= 0) return;
    int row_h = rr->ng104_text_h / NG104_ROW;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    float tw = w * sx, th = h * sy;
    float u0 = 0.0f, u1 = 1.0f;
    float v0 = (float)(row * row_h) / rr->ng104_text_h;
    float v1 = (float)(row * row_h + row_h) / rr->ng104_text_h;
    rs_v4f tq[6] = {
        {x * sx, y * sy, u0, v0}, {x * sx + tw, y * sy, u1, v0}, {x * sx, y * sy + th, u0, v1},
        {x * sx, y * sy + th, u0, v1}, {x * sx + tw, y * sy, u1, v0}, {x * sx + tw, y * sy + th, u1, v1},
    };
    glUseProgram(rr->prog_tex[slot]);
    glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rr->tex_ng104_text[slot]);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
}

static void rs_draw_newgame104(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪ 背景：主菜单背景（新局完整版弹窗是主菜单上的模态框） */
    if (rr->tex_menu_bg[slot] && rr->prog_tex[slot]) {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f bg[6] = {
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
        glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ① 全屏模态遮罩 */
    rs_v4f mq[6] = {
        {0, 0, 0, 0}, {(float)rr->view_w, 0, 0, 0}, {0, (float)rr->view_h, 0, 0},
        {0, (float)rr->view_h, 0, 0}, {(float)rr->view_w, 0, 0, 0},
        {(float)rr->view_w, (float)rr->view_h, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.0f, 0.0f, 0.0f, 0.35f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mq), mq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ② 面板：深色半透明矩形（datah_104 450×390 居中，720 基准 135,445-585,835） */
    float px0 = UI_NEWGAME104_POPUP_X0 * sx, py0 = UI_NEWGAME104_POPUP_Y0 * sy;
    float pw = UI_NEWGAME104_POPUP_W * sx, ph = UI_NEWGAME104_POPUP_H * sy;
    rs_v4f pq[6] = {
        {px0, py0, 0, 0}, {px0 + pw, py0, 0, 0}, {px0, py0 + ph, 0, 0},
        {px0, py0 + ph, 0, 0}, {px0 + pw, py0, 0, 0}, {px0 + pw, py0 + ph, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.08f, 0.08f, 0.10f, 0.78f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ③ 标题"新局"（ng104_text row0，面板顶部 285,460-435,506 720 基准区） */
    rs_ng104_text(rr, slot, 285.0f, 460.0f, 150.0f, 46.0f, 0, uc);
    /* ④ 5 控件：我方/电脑水平/玩法（当前值高亮金色）+ 确定/取消（纯色） */
    static const float BTN_COL[5][4] = {
        {0.45f, 0.30f, 0.75f, 0.9f},   /* 我方 紫 */
        {0.45f, 0.30f, 0.75f, 0.9f},   /* 电脑水平 紫 */
        {0.45f, 0.30f, 0.75f, 0.9f},   /* 玩法 紫 */
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 确定 蓝 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
    };
    const ui_newgame104_btn *nb = ui_newgame104_buttons();
    for (int i = 0; i < UI_NEWGAME104_COUNT; i++) {
        float bx0 = nb[i].x0 * sx, by0 = nb[i].y0 * sy;
        float bx1 = nb[i].x1 * sx, by1 = nb[i].y1 * sy;
        rs_v4f bq[6] = {
            {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
            {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
        /* 3 选择按钮：当前值金色高亮（非选中的画普通色） */
        if (i < 3) {
            float cr = 1.0f, cg = 0.85f, cb = 0.2f;
            glUniform4f(uc, cr, cg, cb, 0.15f);
            rs_v4f oq[6] = {
                {bx0 - 4*sx, by0 - 4*sy, 0, 0}, {bx1 + 4*sx, by0 - 4*sy, 0, 0}, {bx0 - 4*sx, by1 + 4*sy, 0, 0},
                {bx0 - 4*sx, by1 + 4*sy, 0, 0}, {bx1 + 4*sx, by0 - 4*sy, 0, 0}, {bx1 + 4*sx, by1 + 4*sy, 0, 0},
            };
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(oq), oq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
    }
    /* ⑤ 文字：标签列（我方:/电脑水平:/玩法: 3 项）+ 当前值标签 + 确定/取消
       （ng104_text_atlas 10 行：row0=新局 row1=我方 row2=电脑 row3=玩法
       row4=蓝方 row5=红方 row6=新手 row7=中级 row8=高级 row9=确定） */
    if (rr->tex_ng104_text[slot] && rr->prog_tex[slot] && rr->ng104_text_w > 0 && rr->ng104_text_h > 0) {
        int row_h = rr->ng104_text_h / NG104_ROW;   /* 每行 46px（10 标签竖排） */
        float u0 = 0.0f, u1 = 1.0f;
        /* 标签列：我方/电脑水平/玩法 左侧 label（x 165-290，对齐原版 datah_104 标签位置） */
        for (int r = 0; r < 3; r++) {
            int label_row = 1 + r;   /* row1=我方 row2=电脑 row3=玩法 */
            float tcy = (nb[r].y0 + nb[r].y1) * 0.5f * sy;
            float tw = 90.0f * sx, th = tw * ((float)row_h / rr->ng104_text_w);
            float tx0 = 165.0f * sx, ty0 = tcy - th * 0.5f;
            float v0 = (float)(label_row * row_h) / rr->ng104_text_h;
            float v1 = (float)(label_row * row_h + row_h) / rr->ng104_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_ng104_text[slot]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
        /* 当前值标签（我方/电脑水平/玩法 按钮内，按当前 ui_opt_get_mode/level 高亮对应行） */
        int cur_level = ui_opt_get_level();   /* 1=新手 2=中级 3=高级 4=好友 */
        int cur_mode = ui_opt_get_mode();     /* 0=布局明棋 1=翻棋暗棋 */
        int val_rows[3] = { 4, 6, cur_mode ? 5 : 6 };  /* 我方=蓝方(row4)，电脑=新手(row6)，玩法=蓝方(row4)/红方(row5) 简化 */
        if (cur_level == 2) val_rows[1] = 7;   /* 中级 */
        if (cur_level == 3) val_rows[1] = 8;   /* 高级 */
        for (int r = 0; r < 3; r++) {
            int row_idx = val_rows[r];
            float bx0 = nb[r].x0 * sx, by0 = nb[r].y0 * sy;
            float bx1 = nb[r].x1 * sx, by1 = nb[r].y1 * sy;
            float tw = 150.0f * sx, th = tw * ((float)row_h / rr->ng104_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float v0 = (float)(row_idx * row_h) / rr->ng104_text_h;
            float v1 = (float)(row_idx * row_h + row_h) / rr->ng104_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_ng104_text[slot]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
        /* 确定/取消（row9=确定 row4=蓝方 占位，★ 诚实边界：取消文字用 row4 蓝方占位） */
        for (int i = 3; i < UI_NEWGAME104_COUNT; i++) {
            int row_idx = (i == 3) ? 9 : 4;   /* 确定=row9，取消=row4 蓝方占位 */
            float bx0 = nb[i].x0 * sx, by0 = nb[i].y0 * sy;
            float bx1 = nb[i].x1 * sx, by1 = nb[i].y1 * sy;
            float tw = 120.0f * sx, th = tw * ((float)row_h / rr->ng104_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float v0 = (float)(row_idx * row_h) / rr->ng104_text_h;
            float v1 = (float)(row_idx * row_h + row_h) / rr->ng104_text_h;
            rs_v4f tq[6] = {
                {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
                {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_ng104_text[slot]);
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
    static int ngdr = 0;
    if (!ngdr) {
        ngdr = 1;
        LOGI("rs_draw_newgame104：新局完整版弹窗（datah_104 我方/电脑水平/玩法 3 选择 + 确定/取消，面板 %.0f×%.0f，"
             "诚实边界：3 项各循环切换当前值）",
             UI_NEWGAME104_POPUP_W, UI_NEWGAME104_POPUP_H);
    }
}

/* ★ P2（datah_034/035 证据）：联机"加入游戏"/"创建游戏"弹窗渲染——主菜单背景 + 面板深色半透明
   + 桌号/密码占位标签 + 确定/取消 + 文字图集。
   按钮几何 ui_join_buttons()/ui_create_buttons()（720×1280 基准）。
   ★ 诚实边界：原版加入/创建=局域网/Wi-Fi 联机协议（未逆向），src_rs 仅蓝牙 SPP；
   确定 → 诚实边界对话框；取消 → 回主菜单。 */
#define JOIN_ROW 10
static void rs_join_text(rs_render *rr, int slot, float x, float y, float w,
                           float h, int row, GLint uc)
{
    (void)uc;
    if (!rr->tex_join_text[slot] || !rr->prog_tex[slot] ||
        rr->join_text_w <= 0 || rr->join_text_h <= 0) return;
    int row_h = rr->join_text_h / JOIN_ROW;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    float tw = w * sx, th = h * sy;
    float u0 = 0.0f, u1 = 1.0f;
    float v0 = (float)(row * row_h) / rr->join_text_h;
    float v1 = (float)(row * row_h + row_h) / rr->join_text_h;
    rs_v4f tq[6] = {
        {x * sx, y * sy, u0, v0}, {x * sx + tw, y * sy, u1, v0}, {x * sx, y * sy + th, u0, v1},
        {x * sx, y * sy + th, u0, v1}, {x * sx + tw, y * sy, u1, v0}, {x * sx + tw, y * sy + th, u1, v1},
    };
    glUseProgram(rr->prog_tex[slot]);
    glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rr->tex_join_text[slot]);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
}

static void rs_draw_join(rs_render *rr, int slot, int is_create)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪ 背景：主菜单背景（加入/创建弹窗是主菜单上的模态框） */
    if (rr->tex_menu_bg[slot] && rr->prog_tex[slot]) {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f bg[6] = {
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
        glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ① 全屏模态遮罩 */
    rs_v4f mq[6] = {
        {0, 0, 0, 0}, {(float)rr->view_w, 0, 0, 0}, {0, (float)rr->view_h, 0, 0},
        {0, (float)rr->view_h, 0, 0}, {(float)rr->view_w, 0, 0, 0},
        {(float)rr->view_w, (float)rr->view_h, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.0f, 0.0f, 0.0f, 0.35f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mq), mq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ② 面板：深色半透明矩形（datah_034/035 420×235 居中，720 基准 150,522.5-570,757.5） */
    float px0 = UI_JOIN_POPUP_X0 * sx, py0 = UI_JOIN_POPUP_Y0 * sy;
    float pw = UI_JOIN_POPUP_W * sx, ph = UI_JOIN_POPUP_H * sy;
    rs_v4f pq[6] = {
        {px0, py0, 0, 0}, {px0 + pw, py0, 0, 0}, {px0, py0 + ph, 0, 0},
        {px0, py0 + ph, 0, 0}, {px0 + pw, py0, 0, 0}, {px0 + pw, py0 + ph, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.08f, 0.08f, 0.10f, 0.78f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ③ 标题"加入"/"创建"（join_text row0=加入 row1=创建，面板顶部 310,537-440,583 720 基准区） */
    int title_row = is_create ? 1 : 0;
    rs_join_text(rr, slot, 310.0f, 537.0f, 130.0f, 46.0f, title_row, uc);
    /* ④ 桌号/密码 占位 EditLabel（矩形浅色底 + 占位文字，★ 诚实边界：不画真输入框） */
    const ui_join_btn *jb = is_create ? NULL : ui_join_buttons();
    const ui_create_btn *cb = is_create ? ui_create_buttons() : NULL;
    float ex0, ey0, ex1, ey1;
    int eact;
    if (is_create) {
        ex0 = cb[0].x0; ey0 = cb[0].y0; ex1 = cb[0].x1; ey1 = cb[0].y1; eact = cb[0].action;
    } else {
        ex0 = jb[0].x0; ey0 = jb[0].y0; ex1 = jb[0].x1; ey1 = jb[0].y1; eact = jb[0].action;
    }
    float bx0 = ex0 * sx, by0 = ey0 * sy;
    float bx1 = ex1 * sx, by1 = ey1 * sy;
    rs_v4f eq[6] = {
        {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
        {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.90f, 0.90f, 0.92f, 0.92f);   /* 浅色输入框底 */
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(eq), eq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* 占位标签文字（row2=桌号 row3=密码，输入框内左对齐） */
    int ph_row = is_create ? 3 : 2;
    float tw = 90.0f * sx, th = tw * ((float)(rr->join_text_h / JOIN_ROW) / rr->join_text_w);
    float tx0 = bx0 + 14.0f * sx, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
    float u0 = 0.0f, u1 = 1.0f;
    int row_h = rr->join_text_h / JOIN_ROW;
    float v0 = (float)(ph_row * row_h) / rr->join_text_h;
    float v1 = (float)(ph_row * row_h + row_h) / rr->join_text_h;
    rs_v4f tq[6] = {
        {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
        {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
    };
    if (rr->tex_join_text[slot] && rr->prog_tex[slot]) {
        glUseProgram(rr->prog_tex[slot]);
        glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rr->tex_join_text[slot]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ⑤ 确定/取消 2 按钮：确定（蓝）/ 取消（灰）纯色 + 文字 */
    static const float BTN_COL[2][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 确定 蓝 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
    };
    for (int i = 1; i < UI_JOIN_COUNT; i++) {
        float ox0 = (is_create ? cb[i].x0 : jb[i].x0) * sx;
        float oy0 = (is_create ? cb[i].y0 : jb[i].y0) * sy;
        float ox1 = (is_create ? cb[i].x1 : jb[i].x1) * sx;
        float oy1 = (is_create ? cb[i].y1 : jb[i].y1) * sy;
        int bidx = i - 1;   /* 0=确定 1=取消 */
        rs_v4f bq[6] = {
            {ox0, oy0, 0, 0}, {ox1, oy0, 0, 0}, {ox0, oy1, 0, 0},
            {ox0, oy1, 0, 0}, {ox1, oy0, 0, 0}, {ox1, oy1, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, BTN_COL[bidx][0], BTN_COL[bidx][1], BTN_COL[bidx][2], BTN_COL[bidx][3]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
        /* 按钮文字（row4=确定 row5=取消，按钮内居中；★ 诚实边界：确定=边界对话框） */
        int row_idx = (bidx == 0) ? 4 : 5;
        float tw2 = 60.0f * sx, th2 = tw2 * ((float)(rr->join_text_h / JOIN_ROW) / rr->join_text_w);
        float tx2 = (ox0 + ox1) * 0.5f - tw2 * 0.5f, ty2 = (oy0 + oy1) * 0.5f - th2 * 0.5f;
        float v2 = (float)(row_idx * row_h) / rr->join_text_h;
        float v3 = (float)(row_idx * row_h + row_h) / rr->join_text_h;
        rs_v4f tq2[6] = {
            {tx2, ty2, u0, v2}, {tx2 + tw2, ty2, u1, v2}, {tx2, ty2 + th2, u0, v3},
            {tx2, ty2 + th2, u0, v3}, {tx2 + tw2, ty2, u1, v2}, {tx2 + tw2, ty2 + th2, u1, v3},
        };
        if (rr->tex_join_text[slot] && rr->prog_tex[slot] && rr->join_text_w > 0 && rr->join_text_h > 0) {
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_join_text[slot]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(tq2), tq2, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
    }
    static int jd = 0;
    if (!jd) {
        jd = 1;
        LOGI("rs_draw_join：加入/创建游戏弹窗（datah_%s 桌号/密码占位标签 + 确定/取消，面板 %.0f×%.0f，"
             "诚实边界：原版加入/创建=Wi-Fi/局域网协议未逆向，src_rs 仅蓝牙联机已实现，确定→边界对话框）",
             is_create ? "035" : "034", UI_JOIN_POPUP_W, UI_JOIN_POPUP_H);
    }
}

/* ★ P2 最后 5 弹窗（datah_061/050/002/029/031）共享面板渲染辅助：
   主菜单/棋盘背景 + 全屏模态遮罩 + 深色半透明面板矩形（按 720 基准 POPUP 宏）。
   各 rs_draw_* 调用此辅助画 ⓪①② 层，再各自画按钮/文字。 */
static void rs_popup_shell(rs_render *rr, int slot,
                           float px0, float py0, float pw, float ph,
                           int use_menu_bg, GLint uc)
{
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    /* ⓪ 背景：主菜单背景（use_menu_bg=1）或对局棋盘背景（use_menu_bg=0，复用 tex_bg） */
    if (rr->prog_tex[slot] &&
        (use_menu_bg ? rr->tex_menu_bg[slot] : rr->tex_bg[slot])) {
        float w = (float)rr->view_w, h = (float)rr->view_h;
        rs_v4f bg[6] = {
            {0, 0, 0, 0}, {w, 0, 1, 0}, {0, h, 0, 1},
            {0, h, 0, 1}, {w, 0, 1, 0}, {w, h, 1, 1},
        };
        glUseProgram(rr->prog_tex[slot]);
        glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, use_menu_bg ? rr->tex_menu_bg[slot] : rr->tex_bg[slot]);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ① 全屏模态遮罩 */
    rs_v4f mq[6] = {
        {0, 0, 0, 0}, {(float)rr->view_w, 0, 0, 0}, {0, (float)rr->view_h, 0, 0},
        {0, (float)rr->view_h, 0, 0}, {(float)rr->view_w, 0, 0, 0},
        {(float)rr->view_w, (float)rr->view_h, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, 0.0f, 0.0f, 0.0f, 0.35f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mq), mq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ② 面板：深色半透明矩形（720 基准 px0,py0,pw,ph） */
    float p0x = px0 * sx, p0y = py0 * sy;
    float pw2 = pw * sx, ph2 = ph * sy;
    rs_v4f pq[6] = {
        {p0x, p0y, 0, 0}, {p0x + pw2, p0y, 0, 0}, {p0x, p0y + ph2, 0, 0},
        {p0x, p0y + ph2, 0, 0}, {p0x + pw2, p0y, 0, 0}, {p0x + pw2, p0y + ph2, 0, 0},
    };
    glUniform4f(uc, 0.08f, 0.08f, 0.10f, 0.78f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
}

/* 纯色按钮 quad（720 基准坐标，含可选金色高亮描边 highlight=1） */
static void rs_solid_btn(rs_render *rr, int slot, float x0, float y0, float x1, float y1,
                         float cr, float cg, float cb, float ca, int highlight, GLint uc)
{
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    rs_v4f bq[6] = {
        {x0 * sx, y0 * sy, 0, 0}, {x1 * sx, y0 * sy, 0, 0}, {x0 * sx, y1 * sy, 0, 0},
        {x0 * sx, y1 * sy, 0, 0}, {x1 * sx, y0 * sy, 0, 0}, {x1 * sx, y1 * sy, 0, 0},
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform4f(uc, cr, cg, cb, ca);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    if (highlight) {
        /* 金色高亮描边（按钮外围 4px 720 基准） */
        rs_v4f oq[6] = {
            {x0 * sx - 4*sx, y0 * sy - 4*sy, 0, 0}, {x1 * sx + 4*sx, y0 * sy - 4*sy, 0, 0},
            {x0 * sx - 4*sx, y1 * sy + 4*sy, 0, 0}, {x0 * sx - 4*sx, y1 * sy + 4*sy, 0, 0},
            {x1 * sx + 4*sx, y0 * sy - 4*sy, 0, 0}, {x1 * sx + 4*sx, y1 * sy + 4*sy, 0, 0},
        };
        glUniform4f(uc, 1.0f, 0.85f, 0.2f, 0.15f);
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(oq), oq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
}

/* 文字图集标签绘制（单标签居中于 cx,cy 720 基准，宽 w；atlas 行索引 row_idx，总行数 n_rows） */
static void rs_atlas_label(rs_render *rr, int slot, unsigned int tex, int tex_w, int tex_h,
                           int n_rows, int row_idx, float cx, float cy, float w, GLint uc)
{
    (void)uc;
    if (tex_w <= 0 || tex_h <= 0) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    int row_h = tex_h / n_rows;
    float tw = w * sx, th = tw * ((float)row_h / (float)tex_w);
    float tx0 = cx * sx - tw * 0.5f, ty0 = cy * sy - th * 0.5f;
    float u0 = 0.0f, u1 = 1.0f;
    float v0 = (float)(row_idx * row_h) / (float)tex_h;
    float v1 = (float)(row_idx * row_h + row_h) / (float)tex_h;
    rs_v4f tq[6] = {
        {tx0, ty0, u0, v0}, {tx0 + tw, ty0, u1, v0}, {tx0, ty0 + th, u0, v1},
        {tx0, ty0 + th, u0, v1}, {tx0 + tw, ty0, u1, v0}, {tx0 + tw, ty0 + th, u1, v1},
    };
    glUseProgram(rr->prog_tex[slot]);
    glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
}

/* ★ P2（datah_061 证据）：新局完整版（完整版补充）弹窗渲染——主菜单背景 + 面板
   + 先后手/电脑水平/玩法/明暗棋 4 选择 + 确定/取消 + 文字图集。
   按钮几何 ui_ng061_buttons()（720×1280 基准）。
   ★ 诚实边界：4 项各循环切换当前值（对齐 datah_104 循环切换范式）。 */
#define NG061_ROW 10
static void rs_draw_ng061(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（主菜单背景 + 遮罩 + 深色面板 450×465 居中） */
    rs_popup_shell(rr, slot, UI_NG061_POPUP_X0, UI_NG061_POPUP_Y0,
                   UI_NG061_POPUP_W, UI_NG061_POPUP_H, 1, uc);
    /* ③ 标题"新局"（ng061_text row0，面板顶部 360,445 720 基准中心） */
    rs_atlas_label(rr, slot, rr->tex_ng061_text[slot], rr->ng061_text_w, rr->ng061_text_h,
                   NG061_ROW, 0, 360.0f, 445.0f, 150.0f, uc);
    /* ④ 6 控件：4 选择（紫）+ 确定（蓝）/ 取消（灰）；4 选择按钮金色高亮描边 */
    static const float BTN_COL[6][4] = {
        {0.45f, 0.30f, 0.75f, 0.9f},   /* 先后手 紫 */
        {0.45f, 0.30f, 0.75f, 0.9f},   /* 电脑水平 紫 */
        {0.45f, 0.30f, 0.75f, 0.9f},   /* 玩法 紫 */
        {0.45f, 0.30f, 0.75f, 0.9f},   /* 明暗棋 紫 */
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 确定 蓝 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
    };
    const ui_ng061_btn *nb = ui_ng061_buttons();
    for (int i = 0; i < UI_NG061_COUNT; i++)
        rs_solid_btn(rr, slot, nb[i].x0, nb[i].y0, nb[i].x1, nb[i].y1,
                     BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3],
                     i < 4, uc);
    /* ⑤ 文字：4 标签列（先后手/电脑水平/玩法/明暗棋 row1-4 左侧 x=227）+
        当前值标签（按钮内，按 ui_opt_get_level/mode + 先后手/明暗棋 当前循环位） */
    if (rr->tex_ng061_text[slot] && rr->ng061_text_w > 0) {
        int cur_level = ui_opt_get_level();   /* 1-4 */
        int cur_mode = ui_opt_get_mode();      /* 0=布局 1=翻棋 */
        /* 4 标签列（左侧 x=227 720 基准中心，对齐原版 datah_061 标签 30,90,165,120 系） */
        for (int r = 0; r < 4; r++)
            rs_atlas_label(rr, slot, rr->tex_ng061_text[slot], rr->ng061_text_w, rr->ng061_text_h,
                           NG061_ROW, 1 + r, 227.0f,
                           (nb[r].y0 + nb[r].y1) * 0.5f, 90.0f, uc);
        /* 当前值标签（按钮内）：
           先后手(row5=蓝/row6=红，ui_first 0=蓝先手 1=红先手)、
           电脑水平(row7-10=新手/中级/高级/好友，level 1-4→row 6+level)、
           玩法(row6=布局/row7=翻棋，cur_mode 0/1)、
           明暗棋(row8=明/row9=暗，cur_show 0=明 1=暗) */
        int vrow0 = (ui_first_get() ? 6 : 5);   /* 先后手当前值 */
        int vrow1 = 6 + cur_level;             /* 电脑水平当前值（row7-10） */
        int vrow2 = (cur_mode ? 7 : 6);        /* 玩法当前值（布局/翻棋） */
        int vrow3 = (ui_show_get() ? 9 : 8);   /* 明暗棋当前值（明/暗） */
        int vrows[4] = { vrow0, vrow1, vrow2, vrow3 };
        for (int r = 0; r < 4; r++)
            rs_atlas_label(rr, slot, rr->tex_ng061_text[slot], rr->ng061_text_w, rr->ng061_text_h,
                           NG061_ROW, vrows[r],
                           (nb[r].x0 + nb[r].x1) * 0.5f, (nb[r].y0 + nb[r].y1) * 0.5f, 150.0f, uc);
        /* 确定/取消 按钮文字（row10=确定 row6=蓝方 占位，★ 诚实边界：取消用 row6 占位） */
        rs_atlas_label(rr, slot, rr->tex_ng061_text[slot], rr->ng061_text_w, rr->ng061_text_h,
                       NG061_ROW, 10, (nb[4].x0 + nb[4].x1) * 0.5f, (nb[4].y0 + nb[4].y1) * 0.5f, 120.0f, uc);
        rs_atlas_label(rr, slot, rr->tex_ng061_text[slot], rr->ng061_text_w, rr->ng061_text_h,
                       NG061_ROW, 6, (nb[5].x0 + nb[5].x1) * 0.5f, (nb[5].y0 + nb[5].y1) * 0.5f, 120.0f, uc);
    }
    static int n61d = 0;
    if (!n61d) {
        n61d = 1;
        LOGI("rs_draw_ng061：新局完整版（完整版补充）弹窗（datah_061 先后手/电脑水平/玩法/明暗棋 4 选择 + 确定/取消，"
              "面板 %.0f×%.0f，诚实边界：4 项各循环切换当前值简化版）",
              UI_NG061_POPUP_W, UI_NG061_POPUP_H);
    }
}

/* ★ P2（datah_050 证据）：对局信息弹窗渲染——棋盘背景 + 面板
   + Info 文本框（战况统计复用占位） + 复盘/删除/返回 3 按钮 + 文字图集。
   按钮几何 ui_gameinfo_buttons()（720×1280 基准）。
   ★ 诚实边界：Info 文本=☰ 战况统计复用，删除=清复盘记录简化。 */
#define GAMEINFO_ROW 8
static void rs_draw_gameinfo(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（棋盘背景 + 遮罩 + 深色面板 420×380 居中） */
    rs_popup_shell(rr, slot, UI_GAMEINFO_POPUP_X0, UI_GAMEINFO_POPUP_Y0,
                   UI_GAMEINFO_POPUP_W, UI_GAMEINFO_POPUP_H, 0, uc);
    /* ③ Info 文本框（浅色矩形占位，datah_050 Info 30,75,390,285 420 系 → 180,525-540,735 720 基准） */
    rs_solid_btn(rr, slot, 180.0f, 525.0f, 540.0f, 735.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    /* ④ 标题"信息"（gameinfo_text row0，面板顶部 360,473 720 基准中心） */
    rs_atlas_label(rr, slot, rr->tex_gameinfo_text[slot], rr->gameinfo_text_w, rr->gameinfo_text_h,
                   GAMEINFO_ROW, 0, 360.0f, 473.0f, 130.0f, uc);
    /* ⑤ Info 文本框内 4 行占位标签（row1=蓝方 row2=红方 row3=步数 row4=回合，
        战况统计语义复用；动态数字未烘焙 → 诚实边界占位标签 + LOGI 实际值） */
    if (rr->tex_gameinfo_text[slot] && rr->gameinfo_text_w > 0) {
        int cur = ui_replay_cur(), n = ui_replay_count();
        int b = 0, r = 0;
        if (rr->game)
            for (int i = 0; i < N_CELLS; i++) {
                if (rr->game->cells[i].rank == RANK_EMPTY) continue;
                if (rr->game->cells[i].side == SIDE_BLUE) b++;
                else if (rr->game->cells[i].side == SIDE_RED) r++;
            }
        rs_atlas_label(rr, slot, rr->tex_gameinfo_text[slot], rr->gameinfo_text_w, rr->gameinfo_text_h,
                       GAMEINFO_ROW, 1, 360.0f, 560.0f, 150.0f, uc);   /* 蓝方 */
        rs_atlas_label(rr, slot, rr->tex_gameinfo_text[slot], rr->gameinfo_text_w, rr->gameinfo_text_h,
                       GAMEINFO_ROW, 2, 360.0f, 620.0f, 150.0f, uc);   /* 红方 */
        rs_atlas_label(rr, slot, rr->tex_gameinfo_text[slot], rr->gameinfo_text_w, rr->gameinfo_text_h,
                       GAMEINFO_ROW, 3, 360.0f, 680.0f, 150.0f, uc);   /* 步数 */
        rs_atlas_label(rr, slot, rr->tex_gameinfo_text[slot], rr->gameinfo_text_w, rr->gameinfo_text_h,
                       GAMEINFO_ROW, 4, 360.0f, 735.0f, 150.0f, uc);   /* 回合 */
        static int gid = 0;
        if (!gid) {
            gid = 1;
            LOGI("rs_draw_gameinfo：对局信息 Info 文本（蓝方=%d 红方=%d 步数=%d cur=%d/%d，占位标签未烘焙数字）",
                 b, r, n, cur, n);
        }
    }
    /* ⑥ 3 按钮：复盘（蓝）/ 删除（橙）/ 返回（灰）+ 文字 */
    static const float BTN_COL[3][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 复盘 蓝 */
        {0.85f, 0.40f, 0.20f, 0.9f},   /* 删除 橙 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 返回 灰 */
    };
    const ui_gameinfo_btn *gb = ui_gameinfo_buttons();
    for (int i = 0; i < UI_GAMEINFO_COUNT; i++) {
        rs_solid_btn(rr, slot, gb[i].x0, gb[i].y0, gb[i].x1, gb[i].y1,
                     BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3], 0, uc);
        rs_atlas_label(rr, slot, rr->tex_gameinfo_text[slot], rr->gameinfo_text_w, rr->gameinfo_text_h,
                       GAMEINFO_ROW, 5 + i,
                       (gb[i].x0 + gb[i].x1) * 0.5f, (gb[i].y0 + gb[i].y1) * 0.5f, 120.0f, uc);
    }
    (void)sx; (void)sy;
    static int gid2 = 0;
    if (!gid2) {
        gid2 = 1;
        LOGI("rs_draw_gameinfo：对局信息弹窗（datah_050 Info 文本框 + 复盘/删除/返回 3 按钮，"
              "面板 %.0f×%.0f，诚实边界：Info=战况统计复用，删除=清复盘记录简化）",
              UI_GAMEINFO_POPUP_W, UI_GAMEINFO_POPUP_H);
    }
}

/* ★ P2（datah_002 证据）：亮度调节弹窗渲染——主菜单背景 + 面板
   + 滑条占位（不画真滑条，亮度 低/中/高 3 档循环切换区） + 确定/取消 + 文字图集。
   按钮几何 ui_bright_buttons()（720×1280 基准）。
   ★ 诚实边界：原版滑条拖拽=触摸 flaky，简化为 3 档循环切换（对齐 datah_001 音量循环范式）。 */
#define BRIGHT_ROW 10
static void rs_draw_bright(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（主菜单背景 + 遮罩 + 深色面板 460×240 居中） */
    rs_popup_shell(rr, slot, UI_BRIGHT_POPUP_X0, UI_BRIGHT_POPUP_Y0,
                   UI_BRIGHT_POPUP_W, UI_BRIGHT_POPUP_H, 1, uc);
    /* ③ 滑条占位条（Bright 区 152,601-568,649 720 基准，画 3 档占位色块 低/中/高，
       ★ 诚实边界：不画真滑条，3 档循环切换区简化） */
    const ui_bright_btn *bb = ui_bright_buttons();
    float bx0 = bb[0].x0, bx1 = bb[0].x1, by0 = bb[0].y0, by1 = bb[0].y1;
    int cur = ui_bright_get();   /* 0=低 1=中 2=高（ui_state 亮度档位） */
    static const float SEG_COL[3][4] = {
        {0.30f, 0.30f, 0.35f, 0.85f},   /* 低 暗灰 */
        {0.55f, 0.60f, 0.65f, 0.9f},    /* 中 中灰 */
        {0.90f, 0.90f, 0.95f, 0.95f},   /* 高 亮白 */
    };
    float seg_w = (bx1 - bx0) / 3.0f;
    for (int s = 0; s < 3; s++) {
        float sx0 = bx0 + seg_w * s, sx1 = bx0 + seg_w * (s + 1);
        rs_solid_btn(rr, slot, sx0, by0, sx1, by1,
                     SEG_COL[s][0], SEG_COL[s][1], SEG_COL[s][2], SEG_COL[s][3], s == cur, uc);
        rs_atlas_label(rr, slot, rr->tex_bright_text[slot], rr->bright_text_w, rr->bright_text_h,
                       BRIGHT_ROW, 2 + s,   /* row2=低 row3=中 row4=高 */
                       (sx0 + sx1) * 0.5f, (by0 + by1) * 0.5f, 60.0f, uc);
    }
    /* ④ 标题"亮度"（bright_text row0，面板顶部 360,543 720 基准中心） */
    rs_atlas_label(rr, slot, rr->tex_bright_text[slot], rr->bright_text_w, rr->bright_text_h,
                   BRIGHT_ROW, 0, 360.0f, 543.0f, 130.0f, uc);
    /* ⑤ 滑条标签"滑条"（row1，滑条区上方 360,585 720 基准中心） */
    rs_atlas_label(rr, slot, rr->tex_bright_text[slot], rr->bright_text_w, rr->bright_text_h,
                   BRIGHT_ROW, 1, 360.0f, 585.0f, 90.0f, uc);
    /* ⑥ 确定/取消 2 按钮：确定（蓝）/ 取消（灰）+ 文字 */
    static const float BTN_COL[2][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 确定 蓝 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
    };
    for (int i = 1; i < UI_BRIGHT_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     BTN_COL[i - 1][0], BTN_COL[i - 1][1], BTN_COL[i - 1][2], BTN_COL[i - 1][3], 0, uc);
        int row_idx = (i == 1) ? 5 : 6;   /* row5=确定 row6=取消 */
        rs_atlas_label(rr, slot, rr->tex_bright_text[slot], rr->bright_text_w, rr->bright_text_h,
                       BRIGHT_ROW, row_idx,
                       (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f, 120.0f, uc);
    }
    static int bd = 0;
    if (!bd) {
        bd = 1;
        LOGI("rs_draw_bright：亮度调节弹窗（datah_002 滑条简化=亮度 低/中/高 3 档循环切换 + 确定/取消，"
              "面板 %.0f×%.0f，诚实边界：原版滑条拖拽=触摸 flaky，简化为 3 档循环切换，不画真滑条，cur=%d）",
              UI_BRIGHT_POPUP_W, UI_BRIGHT_POPUP_H, cur);
    }
}

/* ★ P2（datah_029 证据）：聊天记录弹窗渲染——棋盘背景 + 面板
   + Msg 文本框（诚实边界占位文本） + 清除/关闭 2 按钮 + 文字图集。
   按钮几何 ui_chatlog_buttons()（720×1280 基准）。
   ★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
#define CHATLOG_ROW 8
static void rs_draw_chatlog(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（棋盘背景 + 遮罩 + 深色面板 420×530 居中） */
    rs_popup_shell(rr, slot, UI_CHATLOG_POPUP_X0, UI_CHATLOG_POPUP_Y0,
                   UI_CHATLOG_POPUP_W, UI_CHATLOG_POPUP_H, 0, uc);
    /* ③ Msg 文本框（浅色矩形占位，datah_029 Msg 30,75,390,430 420 系 → 180,450-540,805 720 基准） */
    rs_solid_btn(rr, slot, 180.0f, 450.0f, 540.0f, 805.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    /* ④ 标题"记录"（chatlog_text row0，面板顶部 360,413 720 基准中心） */
    rs_atlas_label(rr, slot, rr->tex_chatlog_text[slot], rr->chatlog_text_w, rr->chatlog_text_h,
                   CHATLOG_ROW, 0, 360.0f, 413.0f, 130.0f, uc);
    /* ⑤ Msg 文本框内占位标签（row1=聊天 row2=占位 row3=帧 row4=边界 row5=未逆向，
        诚实边界占位文本；动态聊天文本未烘焙 → 占位标签） */
    if (rr->tex_chatlog_text[slot] && rr->chatlog_text_w > 0) {
        rs_atlas_label(rr, slot, rr->tex_chatlog_text[slot], rr->chatlog_text_w, rr->chatlog_text_h,
                       CHATLOG_ROW, 1, 360.0f, 480.0f, 150.0f, uc);   /* 聊天 */
        rs_atlas_label(rr, slot, rr->tex_chatlog_text[slot], rr->chatlog_text_w, rr->chatlog_text_h,
                       CHATLOG_ROW, 2, 360.0f, 540.0f, 150.0f, uc);   /* 占位 */
        rs_atlas_label(rr, slot, rr->tex_chatlog_text[slot], rr->chatlog_text_w, rr->chatlog_text_h,
                       CHATLOG_ROW, 3, 360.0f, 600.0f, 150.0f, uc);   /* 帧 */
        rs_atlas_label(rr, slot, rr->tex_chatlog_text[slot], rr->chatlog_text_w, rr->chatlog_text_h,
                       CHATLOG_ROW, 4, 360.0f, 660.0f, 150.0f, uc);   /* 边界 */
        rs_atlas_label(rr, slot, rr->tex_chatlog_text[slot], rr->chatlog_text_w, rr->chatlog_text_h,
                       CHATLOG_ROW, 5, 360.0f, 720.0f, 150.0f, uc);   /* 未逆向 */
    }
    /* ⑥ 2 按钮：清除（橙）/ 关闭（灰） */
    static const float BTN_COL[2][4] = {
        {0.85f, 0.40f, 0.20f, 0.9f},   /* 清除 橙 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 关闭 灰 */
    };
    const ui_chatlog_btn *cb = ui_chatlog_buttons();
    for (int i = 0; i < UI_CHATLOG_COUNT; i++) {
        rs_solid_btn(rr, slot, cb[i].x0, cb[i].y0, cb[i].x1, cb[i].y1,
                     BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3], 0, uc);
        int row_idx = (i == 0) ? 6 : 7;   /* row6=清除 row7=关闭 */
        rs_atlas_label(rr, slot, rr->tex_chatlog_text[slot], rr->chatlog_text_w, rr->chatlog_text_h,
                       CHATLOG_ROW, row_idx,
                       (cb[i].x0 + cb[i].x1) * 0.5f, (cb[i].y0 + cb[i].y1) * 0.5f, 120.0f, uc);
    }
    static int chd = 0;
    if (!chd) {
        chd = 1;
        LOGI("rs_draw_chatlog：聊天记录弹窗（datah_029 Msg 文本框 + 清除/关闭 2 按钮，面板 %.0f×%.0f，"
              "诚实边界：联机聊天文本未逆向，src_rs 蓝牙聊天是占位帧 BT_T_CHAT）",
              UI_CHATLOG_POPUP_W, UI_CHATLOG_POPUP_H);
    }
}

/* ★ P2（datah_031 证据）：个人资料弹窗渲染——主菜单背景 + 面板
   + Info 文本框（诚实边界占位文本） + 修改/帮助/返回 3 按钮 + 文字图集。
   按钮几何 ui_profile_buttons()（720×1280 基准）。
   ★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
#define PROFILE_ROW 10
static void rs_draw_profile(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（主菜单背景 + 遮罩 + 深色面板 420×520 居中） */
    rs_popup_shell(rr, slot, UI_PROFILE_POPUP_X0, UI_PROFILE_POPUP_Y0,
                   UI_PROFILE_POPUP_W, UI_PROFILE_POPUP_H, 1, uc);
    /* ③ Info 文本框（浅色矩形占位，datah_031 Info 30,75,390,420 420 系 → 180,455-540,800 720 基准） */
    rs_solid_btn(rr, slot, 180.0f, 455.0f, 540.0f, 800.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    /* ④ 标题"资料"（profile_text row0，面板顶部 360,418 720 基准中心） */
    rs_atlas_label(rr, slot, rr->tex_profile_text[slot], rr->profile_text_w, rr->profile_text_h,
                   PROFILE_ROW, 0, 360.0f, 418.0f, 130.0f, uc);
    /* ⑤ Info 文本框内占位标签（row1=账号 row2=昵称 row3=头像 row4=胜率 row5=边界 row6=未逆向，
        诚实边界占位文本；动态账号信息未烘焙 → 占位标签） */
    if (rr->tex_profile_text[slot] && rr->profile_text_w > 0) {
        rs_atlas_label(rr, slot, rr->tex_profile_text[slot], rr->profile_text_w, rr->profile_text_h,
                       PROFILE_ROW, 1, 360.0f, 490.0f, 150.0f, uc);   /* 账号 */
        rs_atlas_label(rr, slot, rr->tex_profile_text[slot], rr->profile_text_w, rr->profile_text_h,
                       PROFILE_ROW, 2, 360.0f, 540.0f, 150.0f, uc);   /* 昵称 */
        rs_atlas_label(rr, slot, rr->tex_profile_text[slot], rr->profile_text_w, rr->profile_text_h,
                       PROFILE_ROW, 3, 360.0f, 590.0f, 150.0f, uc);   /* 头像 */
        rs_atlas_label(rr, slot, rr->tex_profile_text[slot], rr->profile_text_w, rr->profile_text_h,
                       PROFILE_ROW, 4, 360.0f, 640.0f, 150.0f, uc);   /* 胜率 */
        rs_atlas_label(rr, slot, rr->tex_profile_text[slot], rr->profile_text_w, rr->profile_text_h,
                       PROFILE_ROW, 5, 360.0f, 690.0f, 150.0f, uc);   /* 边界 */
        rs_atlas_label(rr, slot, rr->tex_profile_text[slot], rr->profile_text_w, rr->profile_text_h,
                       PROFILE_ROW, 6, 360.0f, 740.0f, 150.0f, uc);   /* 未逆向 */
    }
    /* ⑥ 3 按钮：修改（橙）/ 帮助（蓝）/ 返回（灰） */
    static const float BTN_COL[3][4] = {
        {0.85f, 0.40f, 0.20f, 0.9f},   /* 修改 橙 */
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 帮助 蓝 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 返回 灰 */
    };
    const ui_profile_btn *pb = ui_profile_buttons();
    for (int i = 0; i < UI_PROFILE_COUNT; i++) {
        rs_solid_btn(rr, slot, pb[i].x0, pb[i].y0, pb[i].x1, pb[i].y1,
                     BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3], 0, uc);
        int row_idx = 7 + i;   /* row7=修改 row8=帮助 row9=返回 */
        rs_atlas_label(rr, slot, rr->tex_profile_text[slot], rr->profile_text_w, rr->profile_text_h,
                       PROFILE_ROW, row_idx,
                       (pb[i].x0 + pb[i].x1) * 0.5f, (pb[i].y0 + pb[i].y1) * 0.5f, 120.0f, uc);
    }
    static int pfd = 0;
    if (!pfd) {
        pfd = 1;
        LOGI("rs_draw_profile：个人资料弹窗（datah_031 Info 文本框 + 修改/帮助/返回 3 按钮，面板 %.0f×%.0f，"
              "诚实边界：账号系统未逆向，离线单机无账号，修改→边界对话框）",
              UI_PROFILE_POPUP_W, UI_PROFILE_POPUP_H);
    }
}

/* ★ P2（datah_042 证据）：提和详情弹窗渲染——棋盘背景 + 面板（450×285 居中）
   + 7 图标（提和/认输/聊天/系统设置/玩家资料/战况统计/软件信息，复用 ☰ 菜单
   图标图集 tex_imen_icons 8 槽占位映射） + 7 标签文字图集（drawdetail_text_atlas）。
   按钮几何 ui_drawdetail_buttons()（720×1280 基准）。
   ★ 诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
   提和/认输走 BT_T_DRAW/BT_T_RESIGN 帧 + 边界对话框，不包装成"已提和判定"；
   玩家资料/软件信息 图标=☰ 图标图集占位映射（原图集无专用图标）。 */
#define DRAWDETAIL_ROW 7
/* 7 图标 → ☰ 图标图集 8 槽映射（bt 版原位：提和=2 认输=3 聊天=4 系统设置=5 战况统计=6；
   玩家资料=0/软件信息=1 占位（原图集无专用图标，★ 诚实边界）） */
static void rs_draw_drawdetail(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（棋盘背景 + 遮罩 + 深色面板 450×285 居中） */
    rs_popup_shell(rr, slot, UI_DRAWDETAIL_POPUP_X0, UI_DRAWDETAIL_POPUP_Y0,
                   UI_DRAWDETAIL_POPUP_W, UI_DRAWDETAIL_POPUP_H, 0, uc);
    const ui_drawdetail_btn *db = ui_drawdetail_buttons();
    static const int icon_map[DRAWDETAIL_ROW] = {2, 3, 4, 5, 0, 6, 1};
    /* ③ 7 图标：☰ 菜单真实图标位图（imen_icons_atlas 8 槽 110×100 4×2）+
       ④ 7 标签（drawdetail_text_atlas 竖排，按钮内 0.90 处，对齐 ☰ 菜单文字范式） */
    for (int i = 0; i < UI_DRAWDETAIL_COUNT; i++) {
        float bx0 = db[i].x0 * sx, by0 = db[i].y0 * sy;
        float bx1 = db[i].x1 * sx, by1 = db[i].y1 * sy;
        float icx = (bx0 + bx1) * 0.5f;
        float icy = by0 + (by1 - by0) * 0.30f;
        float iw = 88.0f * sx, ih = 80.0f * sy;
        float icx0 = icx - iw * 0.5f, icx1 = icx + iw * 0.5f;
        float icy0 = icy - ih * 0.5f, icy1 = icy + ih * 0.5f;
        if (rr->tex_imen_icons[slot] && rr->prog_tex[slot] && rr->imen_icons_w > 0 && rr->imen_icons_h > 0) {
            int gi = icon_map[i];
            float u0 = (float)(gi % 4) / 4.0f, v0 = (float)(gi / 4) / 2.0f;
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
        }
        /* 7 标签文字（按钮内 0.90 处，对齐 ☰ 菜单文字范式） */
        if (rr->tex_drawdetail_text[slot] && rr->prog_tex[slot] &&
            rr->drawdetail_text_w > 0 && rr->drawdetail_text_h > 0) {
            rs_atlas_label(rr, slot, rr->tex_drawdetail_text[slot],
                           rr->drawdetail_text_w, rr->drawdetail_text_h,
                           DRAWDETAIL_ROW, i, icx, by0 + (by1 - by0) * 0.90f, 95.0f, uc);
        }
    }
    static int ddd = 0;
    if (!ddd) {
        ddd = 1;
        LOGI("rs_draw_drawdetail：提和详情弹窗（datah_042 联机版 ☰ 菜单 7 图标：提和/认输/聊天/系统设置/玩家资料/"
              "战况统计/软件信息，面板 %.0f×%.0f，图标=☰ 图标图集占位映射，诚实边界：本地提和判定未实现，"
              "提和/认输走 BT 帧 + 边界对话框）", UI_DRAWDETAIL_POPUP_W, UI_DRAWDETAIL_POPUP_H);
    }
}

/* ★ P2（datah_003/004 证据）：设置入口弹窗渲染——主菜单背景 + 面板
   + 3/2 按钮 + 文字图集（setentry_text_atlas）。
   按钮几何 ui_setentry_buttons()/ui_setentry4_buttons()（720×1280 基准）。
   ★ 诚实边界：图像设置渲染参数未实现（src_rs 无对应渲染参数），
   开关只是 UI 状态记录（不实际改渲染）。 */
#define SETENTRY_ROW 7
static void rs_draw_setentry(rs_render *rr, int slot, int is_simplified)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（主菜单背景 + 遮罩 + 深色面板） */
    if (is_simplified)
        rs_popup_shell(rr, slot, UI_SETEXTTRY4_POPUP_X0, UI_SETEXTTRY4_POPUP_Y0,
                       UI_SETEXTTRY4_POPUP_W, UI_SETEXTTRY4_POPUP_H, 1, uc);
    else
        rs_popup_shell(rr, slot, UI_SETEXTTRY_POPUP_X0, UI_SETEXTTRY_POPUP_Y0,
                       UI_SETEXTTRY_POPUP_W, UI_SETEXTTRY_POPUP_H, 1, uc);
    /* ③ 按钮 + 文字 */
    if (is_simplified) {
        const ui_setentry4_btn *sb = ui_setentry4_buttons();
        for (int i = 0; i < UI_SETEXTTRY4_COUNT; i++) {
            rs_solid_btn(rr, slot, sb[i].x0, sb[i].y0, sb[i].x1, sb[i].y1,
                         0.45f, 0.30f, 0.75f, 0.9f, 0, uc);
            rs_atlas_label(rr, slot, rr->tex_setentry_text[slot], rr->setentry_text_w, rr->setentry_text_h,
                           SETENTRY_ROW, 1 + i,
                           (sb[i].x0 + sb[i].x1) * 0.5f, (sb[i].y0 + sb[i].y1) * 0.5f, 120.0f, uc);
        }
    } else {
        const ui_setentry_btn *sb = ui_setentry_buttons();
        for (int i = 0; i < UI_SETEXTTRY_COUNT; i++) {
            rs_solid_btn(rr, slot, sb[i].x0, sb[i].y0, sb[i].x1, sb[i].y1,
                         0.45f, 0.30f, 0.75f, 0.9f, 0, uc);
            rs_atlas_label(rr, slot, rr->tex_setentry_text[slot], rr->setentry_text_w, rr->setentry_text_h,
                           SETENTRY_ROW, 1 + i,
                           (sb[i].x0 + sb[i].x1) * 0.5f, (sb[i].y0 + sb[i].y1) * 0.5f, 120.0f, uc);
        }
    }
    /* ④ 标题"设置"（setentry_text row0，面板顶部 360 中心） */
    if (is_simplified)
        rs_atlas_label(rr, slot, rr->tex_setentry_text[slot], rr->setentry_text_w, rr->setentry_text_h,
                       SETENTRY_ROW, 0, 360.0f, UI_SETEXTTRY4_POPUP_Y0 + 25.0f, 130.0f, uc);
    else
        rs_atlas_label(rr, slot, rr->tex_setentry_text[slot], rr->setentry_text_w, rr->setentry_text_h,
                       SETENTRY_ROW, 0, 360.0f, UI_SETEXTTRY_POPUP_Y0 + 25.0f, 130.0f, uc);
    static int sed = 0;
    if (!sed) {
        sed = 1;
        LOGI("rs_draw_setentry：设置入口弹窗（datah_%s 图像/声音%s 入口，面板 %.0f×%.0f，"
              "诚实边界：图像设置渲染参数未实现，仅 UI 状态记录）",
              is_simplified ? "004" : "003",
              is_simplified ? "" : "/游戏设置",
              is_simplified ? UI_SETEXTTRY4_POPUP_W : UI_SETEXTTRY_POPUP_W,
              is_simplified ? UI_SETEXTTRY4_POPUP_H : UI_SETEXTTRY_POPUP_H);
    }
}

/* ★ P2（datah_007/008 证据）：图像设置弹窗渲染——主菜单背景 + 面板
   + 4/3 开关（各循环切换 开/关，金色高亮描边） + 确定/取消 + 文字图集。
   按钮几何 ui_imgset7_buttons()/ui_imgset8_buttons()（720×1280 基准）。
   ★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
   开关只是 UI 状态记录（不实际改渲染）。 */
#define IMGSET_ROW 10
static void rs_draw_imgset(rs_render *rr, int slot, int is_simplified)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（主菜单背景 + 遮罩 + 深色面板） */
    if (is_simplified)
        rs_popup_shell(rr, slot, UI_IMGSET8_POPUP_X0, UI_IMGSET8_POPUP_Y0,
                       UI_IMGSET8_POPUP_W, UI_IMGSET8_POPUP_H, 1, uc);
    else
        rs_popup_shell(rr, slot, UI_IMGSET7_POPUP_X0, UI_IMGSET7_POPUP_Y0,
                       UI_IMGSET7_POPUP_W, UI_IMGSET7_POPUP_H, 1, uc);
    /* ③ 标题"图像"（imgset_text row0，面板顶部 360 中心） */
    if (is_simplified)
        rs_atlas_label(rr, slot, rr->tex_imgset_text[slot], rr->imgset_text_w, rr->imgset_text_h,
                       IMGSET_ROW, 0, 360.0f, UI_IMGSET8_POPUP_Y0 + 25.0f, 130.0f, uc);
    else
        rs_atlas_label(rr, slot, rr->tex_imgset_text[slot], rr->imgset_text_w, rr->imgset_text_h,
                       IMGSET_ROW, 0, 360.0f, UI_IMGSET7_POPUP_Y0 + 25.0f, 130.0f, uc);
    /* ④ 开关按钮（各循环切换 开/关，金色高亮描边）+ 标签 */
    if (is_simplified) {
        const ui_imgset8_btn *ib = ui_imgset8_buttons();
        for (int i = 0; i < 3; i++) {
            rs_solid_btn(rr, slot, ib[i].x0, ib[i].y0, ib[i].x1, ib[i].y1,
                         0.45f, 0.30f, 0.75f, 0.9f, 1, uc);
            /* 标签列（row1=界面 row2=棋子 row3=夜间） */
            rs_atlas_label(rr, slot, rr->tex_imgset_text[slot], rr->imgset_text_w, rr->imgset_text_h,
                           IMGSET_ROW, 1 + i, 227.0f,
                           (ib[i].y0 + ib[i].y1) * 0.5f, 90.0f, uc);
            /* 当前值标签（row7=开 row8=关，按 ui_imgset_*_get） */
            int vrow;
            if (i == 0) vrow = ui_imgset_trans_get() ? 7 : 8;
            else if (i == 1) vrow = ui_imgset_piece_get() ? 7 : 8;
            else vrow = ui_imgset_night_get() ? 7 : 8;
            rs_atlas_label(rr, slot, rr->tex_imgset_text[slot], rr->imgset_text_w, rr->imgset_text_h,
                           IMGSET_ROW, vrow,
                           (ib[i].x0 + ib[i].x1) * 0.5f, (ib[i].y0 + ib[i].y1) * 0.5f, 150.0f, uc);
        }
        /* ⑤ 确定/取消 2 按钮 */
        static const float BTN_COL[2][4] = {
            {0.20f, 0.45f, 0.90f, 0.9f},   /* 确定 蓝 */
            {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
        };
        for (int i = 3; i < UI_IMGSET8_COUNT; i++) {
            rs_solid_btn(rr, slot, ib[i].x0, ib[i].y0, ib[i].x1, ib[i].y1,
                         BTN_COL[i-3][0], BTN_COL[i-3][1], BTN_COL[i-3][2], BTN_COL[i-3][3], 0, uc);
            if (i == 3) {   /* 确定按钮：row9=确定 */
                rs_atlas_label(rr, slot, rr->tex_imgset_text[slot], rr->imgset_text_w, rr->imgset_text_h,
                               IMGSET_ROW, 9,
                               (ib[i].x0 + ib[i].x1) * 0.5f, (ib[i].y0 + ib[i].y1) * 0.5f, 120.0f, uc);
            }
            /* 取消按钮：不画标签（原图集无"取消"标签，诚实边界占位） */
        }
    } else {
        const ui_imgset7_btn *ib = ui_imgset7_buttons();
        for (int i = 0; i < 4; i++) {
            rs_solid_btn(rr, slot, ib[i].x0, ib[i].y0, ib[i].x1, ib[i].y1,
                         0.45f, 0.30f, 0.75f, 0.9f, 1, uc);
            /* 标签列（row1=界面 row2=棋子 row3=夜间 row4=对方） */
            rs_atlas_label(rr, slot, rr->tex_imgset_text[slot], rr->imgset_text_w, rr->imgset_text_h,
                           IMGSET_ROW, 1 + i, 227.0f,
                           (ib[i].y0 + ib[i].y1) * 0.5f, 90.0f, uc);
            /* 当前值标签（row7=开 row8=关，按 ui_imgset_*_get） */
            int vrow;
            if (i == 0) vrow = ui_imgset_trans_get() ? 7 : 8;
            else if (i == 1) vrow = ui_imgset_piece_get() ? 7 : 8;
            else if (i == 2) vrow = ui_imgset_night_get() ? 7 : 8;
            else vrow = ui_imgset_opp_get() ? 7 : 8;
            rs_atlas_label(rr, slot, rr->tex_imgset_text[slot], rr->imgset_text_w, rr->imgset_text_h,
                           IMGSET_ROW, vrow,
                           (ib[i].x0 + ib[i].x1) * 0.5f, (ib[i].y0 + ib[i].y1) * 0.5f, 150.0f, uc);
        }
        /* ⑤ 确定/取消 2 按钮 */
        static const float BTN_COL[2][4] = {
            {0.20f, 0.45f, 0.90f, 0.9f},   /* 确定 蓝 */
            {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
        };
        for (int i = 4; i < UI_IMGSET7_COUNT; i++) {
            rs_solid_btn(rr, slot, ib[i].x0, ib[i].y0, ib[i].x1, ib[i].y1,
                         BTN_COL[i-4][0], BTN_COL[i-4][1], BTN_COL[i-4][2], BTN_COL[i-4][3], 0, uc);
            int row_idx = (i == 4) ? 9 : 10;   /* row9=确定 row10=取消 */
            rs_atlas_label(rr, slot, rr->tex_imgset_text[slot], rr->imgset_text_w, rr->imgset_text_h,
                           IMGSET_ROW, row_idx,
                           (ib[i].x0 + ib[i].x1) * 0.5f, (ib[i].y0 + ib[i].y1) * 0.5f, 120.0f, uc);
        }
    }
    static int imgd = 0;
    if (!imgd) {
        imgd = 1;
        LOGI("rs_draw_imgset：图像设置弹窗（datah_%s 开关 + 确定/取消，面板 %.0f×%.0f，"
              "诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅界面动画过渡帧未实现，"
              "界面=%d 棋子=%d 夜间=%d%s）",
              is_simplified ? "008" : "007",
              is_simplified ? UI_IMGSET8_POPUP_W : UI_IMGSET7_POPUP_W,
              is_simplified ? UI_IMGSET8_POPUP_H : UI_IMGSET7_POPUP_H,
              ui_imgset_trans_get(), ui_imgset_piece_get(), ui_imgset_night_get(),
              is_simplified ? "" : " 对方=开",
              0);
    }
}

/* ★ P2（datah_065 证据）：加入游戏完整版弹窗渲染——主菜单背景 + 面板
   + 8 显示占位标签（先手/后手/玩法/明暗棋/4 规则） + 开始/取消 2 按钮 + 文字图集。
   按钮几何 ui_join065_buttons()（720×1280 基准）。
   ★ 诚实边界：先手/后手/玩法/明暗棋/4 规则 = 联机开局配置（src_rs 无联机开局逻辑，
   显示占位标签"蓝方/红方/布局/暗棋/开/开/开/开"，不画真数据）；
   开始→边界对话框（"加入游戏完整版：联机开局需 Wi-Fi/局域网房间协议，src_rs 仅蓝牙已实现"） */
#define JOIN065_ROW 10
static void rs_draw_join065(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（主菜单背景 + 遮罩 + 深色面板 450×505 居中） */
    rs_popup_shell(rr, slot, UI_JOIN065_POPUP_X0, UI_JOIN065_POPUP_Y0,
                   UI_JOIN065_POPUP_W, UI_JOIN065_POPUP_H, 1, uc);
    /* ③ 标题"加入"（join065_text row0，面板顶部 360 中心） */
    rs_atlas_label(rr, slot, rr->tex_join065_text[slot], rr->join065_text_w, rr->join065_text_h,
                   JOIN065_ROW, 0, 360.0f, UI_JOIN065_POPUP_Y0 + 25.0f, 130.0f, uc);
    /* ④ 8 显示占位标签（row1=蓝方 row2=红方 row3=布局 row4=暗棋 row5=开 row6=开 row7=开 row8=开） */
    const ui_join065_btn *jb = ui_join065_buttons();
    for (int i = 0; i < 8; i++) {
        /* 浅色占位标签区（非按钮，仅显示） */
        rs_solid_btn(rr, slot, jb[i].x0, jb[i].y0, jb[i].x1, jb[i].y1,
                     0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
        /* 标签文字（row1-8 = 蓝方/红方/布局/暗棋/开/开/开/开） */
        rs_atlas_label(rr, slot, rr->tex_join065_text[slot], rr->join065_text_w, rr->join065_text_h,
                       JOIN065_ROW, 1 + i,
                       (jb[i].x0 + jb[i].x1) * 0.5f, (jb[i].y0 + jb[i].y1) * 0.5f, 150.0f, uc);
    }
    /* ⑤ 开始/取消 2 按钮 */
    static const float BTN_COL[2][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 开始 蓝 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
    };
    for (int i = 8; i < UI_JOIN065_COUNT; i++) {
        rs_solid_btn(rr, slot, jb[i].x0, jb[i].y0, jb[i].x1, jb[i].y1,
                     BTN_COL[i-8][0], BTN_COL[i-8][1], BTN_COL[i-8][2], BTN_COL[i-8][3], 0, uc);
        int row_idx = (i == 8) ? 9 : 8;   /* row9=开始 row8=开（占位） */
        rs_atlas_label(rr, slot, rr->tex_join065_text[slot], rr->join065_text_w, rr->join065_text_h,
                       JOIN065_ROW, row_idx,
                       (jb[i].x0 + jb[i].x1) * 0.5f, (jb[i].y0 + jb[i].y1) * 0.5f, 120.0f, uc);
    }
    static int j65d = 0;
    if (!j65d) {
        j65d = 1;
        LOGI("rs_draw_join065：加入游戏完整版弹窗（datah_065 8 显示占位标签 + 开始/取消，面板 %.0f×%.0f，"
              "诚实边界：联机开局配置=占位标签（蓝方/红方/布局/暗棋/开/开/开/开），"
              "开始→边界对话框（src_rs 仅蓝牙已实现））",
              UI_JOIN065_POPUP_W, UI_JOIN065_POPUP_H);
    }
}

/* ★ P2（datah_069 证据）：创建游戏完整版弹窗渲染——主菜单背景 + 面板
   + 4 选择（先后手/玩法/明暗棋/游戏规则，各循环切换 开/关或 蓝/红，金色高亮描边）
   + 确定/取消 + 文字图集。
   按钮几何 ui_create069_buttons()（720×1280 基准）。
   ★ 诚实边界：先后手/玩法/明暗棋 = 联机开局配置（src_rs 无联机房间创建逻辑，
   循环切换仅 UI 状态记录）；确定→边界对话框（"创建游戏完整版：联机房间创建需
   Wi-Fi/局域网协议，src_rs 仅蓝牙已实现"） */
#define CREATE069_ROW 10
static void rs_draw_create069(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（主菜单背景 + 遮罩 + 深色面板 450×465 居中） */
    rs_popup_shell(rr, slot, UI_CREATE069_POPUP_X0, UI_CREATE069_POPUP_Y0,
                   UI_CREATE069_POPUP_W, UI_CREATE069_POPUP_H, 1, uc);
    /* ③ 标题"创建"（create069_text row0，面板顶部 360 中心） */
    rs_atlas_label(rr, slot, rr->tex_create069_text[slot], rr->create069_text_w, rr->create069_text_h,
                   CREATE069_ROW, 0, 360.0f, UI_CREATE069_POPUP_Y0 + 25.0f, 130.0f, uc);
    /* ④ 4 选择按钮（先后手/玩法/明暗棋/游戏规则，金色高亮描边）+ 标签 */
    const ui_create069_btn *cb = ui_create069_buttons();
    for (int i = 0; i < 4; i++) {
        rs_solid_btn(rr, slot, cb[i].x0, cb[i].y0, cb[i].x1, cb[i].y1,
                     0.45f, 0.30f, 0.75f, 0.9f, 1, uc);
        /* 标签列（row1=先后手 row2=玩法 row3=明暗棋 row4=规则） */
        rs_atlas_label(rr, slot, rr->tex_create069_text[slot], rr->create069_text_w, rr->create069_text_h,
                       CREATE069_ROW, 1 + i, 227.0f,
                       (cb[i].y0 + cb[i].y1) * 0.5f, 90.0f, uc);
        /* 当前值标签（row5=蓝方 row6=红方 row7=布局 row8=翻棋，按 ui_first/ui_opt/ui_show） */
        int vrow;
        if (i == 0) vrow = ui_first_get() ? 6 : 5;   /* 先后手：蓝方/红方 */
        else if (i == 1) vrow = ui_opt_get_mode() ? 8 : 7;   /* 玩法：布局/翻棋 */
        else if (i == 2) vrow = ui_show_get() ? 8 : 7;   /* 明暗棋：明/暗（简化用 布局/翻棋 占位） */
        else vrow = 4;   /* 游戏规则：text="设置"（row4 占位） */
        rs_atlas_label(rr, slot, rr->tex_create069_text[slot], rr->create069_text_w, rr->create069_text_h,
                       CREATE069_ROW, vrow,
                       (cb[i].x0 + cb[i].x1) * 0.5f, (cb[i].y0 + cb[i].y1) * 0.5f, 150.0f, uc);
    }
    /* ⑤ 确定/取消 2 按钮 */
    static const float BTN_COL[2][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 确定 蓝 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
    };
    for (int i = 4; i < UI_CREATE069_COUNT; i++) {
        rs_solid_btn(rr, slot, cb[i].x0, cb[i].y0, cb[i].x1, cb[i].y1,
                     BTN_COL[i-4][0], BTN_COL[i-4][1], BTN_COL[i-4][2], BTN_COL[i-4][3], 0, uc);
        int row_idx = (i == 4) ? 9 : 10;   /* row9=确定 row10=取消 */
        rs_atlas_label(rr, slot, rr->tex_create069_text[slot], rr->create069_text_w, rr->create069_text_h,
                       CREATE069_ROW, row_idx,
                       (cb[i].x0 + cb[i].x1) * 0.5f, (cb[i].y0 + cb[i].y1) * 0.5f, 120.0f, uc);
    }
    static int c69d = 0;
    if (!c69d) {
        c69d = 1;
        LOGI("rs_draw_create069：创建游戏完整版弹窗（datah_069 4 选择 + 确定/取消，面板 %.0f×%.0f，"
              "诚实边界：联机房间创建=边界对话框（src_rs 仅蓝牙已实现），"
              "先后手=%s 玩法=%s 明暗棋=%s）",
              UI_CREATE069_POPUP_W, UI_CREATE069_POPUP_H,
              ui_first_get() ? "红先手" : "蓝先手",
              ui_opt_get_mode() ? "翻棋" : "布局",
              ui_show_get() ? "暗棋" : "明棋");
    }
}

/* ★ P2（datah_070 证据）：游戏规则完整版弹窗渲染——主菜单背景 + 面板
   + 4 规则开关（和棋/大本营/吃地雷/吃军旗，各循环切换 开/关，金色高亮描边）
   + 确定/取消 + 文字图集。
   按钮几何 ui_rules070_buttons()（720×1280 基准）。
   与 datah_012 游戏设置同构，复用 ui_rule_*_get/set。
   ★ 与 datah_012 区分：012 是本地 ☰ 菜单"功能设置"→UI_GAMESET，
   070 是联机"游戏规则"弹窗（datah_069 创建游戏"游戏规则"按钮切此弹窗）。 */
#define RULES070_ROW 10
static void rs_draw_rules070(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（主菜单背景 + 遮罩 + 深色面板 450×465 居中） */
    rs_popup_shell(rr, slot, UI_RULES070_POPUP_X0, UI_RULES070_POPUP_Y0,
                   UI_RULES070_POPUP_W, UI_RULES070_POPUP_H, 1, uc);
    /* ③ 标题"规则"（rules070_text row0，面板顶部 360 中心） */
    rs_atlas_label(rr, slot, rr->tex_rules070_text[slot], rr->rules070_text_w, rr->rules070_text_h,
                   RULES070_ROW, 0, 360.0f, UI_RULES070_POPUP_Y0 + 25.0f, 130.0f, uc);
    /* ④ 4 规则开关（各循环切换 开/关，金色高亮描边）+ 标签 */
    const ui_rules070_btn *rb = ui_rules070_buttons();
    for (int i = 0; i < 4; i++) {
        rs_solid_btn(rr, slot, rb[i].x0, rb[i].y0, rb[i].x1, rb[i].y1,
                     0.45f, 0.30f, 0.75f, 0.9f, 1, uc);
        /* 标签列（row1=和棋 row2=大本营 row3=吃雷 row4=吃旗） */
        rs_atlas_label(rr, slot, rr->tex_rules070_text[slot], rr->rules070_text_w, rr->rules070_text_h,
                       RULES070_ROW, 1 + i, 227.0f,
                       (rb[i].y0 + rb[i].y1) * 0.5f, 90.0f, uc);
        /* 当前值标签（row5=开 row6=关，按 ui_rule_*_get） */
        int cur;
        if (i == 0) cur = ui_rule_draw_get();
        else if (i == 1) cur = ui_rule_hq_get();
        else if (i == 2) cur = ui_rule_mine_get();
        else cur = ui_rule_flag_get();
        int vrow = cur ? 5 : 6;
        rs_atlas_label(rr, slot, rr->tex_rules070_text[slot], rr->rules070_text_w, rr->rules070_text_h,
                       RULES070_ROW, vrow,
                       (rb[i].x0 + rb[i].x1) * 0.5f, (rb[i].y0 + rb[i].y1) * 0.5f, 150.0f, uc);
    }
    /* ⑤ 确定/取消 2 按钮 */
    static const float BTN_COL[2][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 确定 蓝 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
    };
    for (int i = 4; i < UI_RULES070_COUNT; i++) {
        rs_solid_btn(rr, slot, rb[i].x0, rb[i].y0, rb[i].x1, rb[i].y1,
                     BTN_COL[i-4][0], BTN_COL[i-4][1], BTN_COL[i-4][2], BTN_COL[i-4][3], 0, uc);
        int row_idx = (i == 4) ? 9 : 10;   /* row9=确定 row10=取消 */
        rs_atlas_label(rr, slot, rr->tex_rules070_text[slot], rr->rules070_text_w, rr->rules070_text_h,
                       RULES070_ROW, row_idx,
                       (rb[i].x0 + rb[i].x1) * 0.5f, (rb[i].y0 + rb[i].y1) * 0.5f, 120.0f, uc);
    }
    static int r70d = 0;
    if (!r70d) {
        r70d = 1;
        LOGI("rs_draw_rules070：游戏规则完整版弹窗（datah_070 4 规则开关 + 确定/取消，面板 %.0f×%.0f，"
              "与 datah_012 同构，复用 ui_rule_*；"
              "和棋=%d 大本营=%d 吃雷=%d 吃旗=%d）",
              UI_RULES070_POPUP_W, UI_RULES070_POPUP_H,
              ui_rule_draw_get(), ui_rule_hq_get(), ui_rule_mine_get(), ui_rule_flag_get());
    }
}

/* ★ P2（datah_060 证据）：房间态弹窗渲染——主菜单背景 + 面板
   + 2 玩家占位标签（蓝方/红方） + 开始/换桌/离开 3 按钮 + 文字图集。
   按钮几何 ui_room060_buttons()（720×1280 基准）。
   ★ 诚实边界：联机房间需 Wi-Fi/局域网协议（src_rs 仅蓝牙已实现），
   开始/换桌→边界对话框；2 玩家显示区=占位标签（不画真数据，触摸 flaky 下房间列表无意义） */
#define ROOM060_ROW 8
static void rs_draw_room060(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（主菜单背景 + 遮罩 + 深色面板 450×352 居中） */
    rs_popup_shell(rr, slot, UI_ROOM060_POPUP_X0, UI_ROOM060_POPUP_Y0,
                   UI_ROOM060_POPUP_W, UI_ROOM060_POPUP_H, 1, uc);
    /* ③ 标题"房间"（room060_text row0，面板顶部 360 中心） */
    rs_atlas_label(rr, slot, rr->tex_room060_text[slot], rr->room060_text_w, rr->room060_text_h,
                   ROOM060_ROW, 0, 360.0f, UI_ROOM060_POPUP_Y0 + 25.0f, 130.0f, uc);
    /* ④ 2 玩家占位标签（row1=蓝方 row2=红方，浅色矩形占位） */
    rs_solid_btn(rr, slot, 180.0f, UI_ROOM060_POPUP_Y0 + 60.0f,
                 540.0f, UI_ROOM060_POPUP_Y0 + 180.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    rs_atlas_label(rr, slot, rr->tex_room060_text[slot], rr->room060_text_w, rr->room060_text_h,
                   ROOM060_ROW, 1, 250.0f, UI_ROOM060_POPUP_Y0 + 100.0f, 150.0f, uc);
    rs_atlas_label(rr, slot, rr->tex_room060_text[slot], rr->room060_text_w, rr->room060_text_h,
                   ROOM060_ROW, 2, 450.0f, UI_ROOM060_POPUP_Y0 + 140.0f, 150.0f, uc);
    /* ⑤ 3 按钮：开始（蓝）/ 换桌（橙）/ 离开（灰） */
    static const float BTN_COL[3][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 开始 蓝 */
        {0.85f, 0.40f, 0.20f, 0.9f},   /* 换桌 橙 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 离开 灰 */
    };
    const ui_room060_btn *rm = ui_room060_buttons();
    for (int i = 0; i < UI_ROOM060_COUNT; i++) {
        rs_solid_btn(rr, slot, rm[i].x0, rm[i].y0, rm[i].x1, rm[i].y1,
                     BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3], 0, uc);
        int row_idx = 3 + i;   /* row3=开始 row4=换桌 row5=离开 */
        rs_atlas_label(rr, slot, rr->tex_room060_text[slot], rr->room060_text_w, rr->room060_text_h,
                       ROOM060_ROW, row_idx,
                       (rm[i].x0 + rm[i].x1) * 0.5f, (rm[i].y0 + rm[i].y1) * 0.5f, 120.0f, uc);
    }
    static int rmd = 0;
    if (!rmd) {
        rmd = 1;
        LOGI("rs_draw_room060：房间态弹窗（datah_060 2 玩家占位标签 + 开始/换桌/离开，面板 %.0f×%.0f，"
              "诚实边界：联机房间=边界对话框（src_rs 仅蓝牙已实现），2 玩家显示区=占位标签）",
              UI_ROOM060_POPUP_W, UI_ROOM060_POPUP_H);
    }
}

/* ★ P2（datah_063 证据）：聊天完整版弹窗渲染——棋盘背景 + 面板
   + Msg 输入框占位标签（"聊天输入（诚实边界：联机聊天文本未逆向）"）
   + 发送/记录/关闭 3 按钮 + 文字图集。
   按钮几何 ui_chat063_buttons()（720×1280 基准）。
   ★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT，无真实文本渲染），
   Msg 输入框简化为占位标签；发送→BT_T_CHAT 占位帧+边界对话框；记录→UI_CHATLOG（datah_029 已实现） */
#define CHAT063_ROW 8
static void rs_draw_chat063(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（棋盘背景 + 遮罩 + 深色面板 420×325 居中） */
    rs_popup_shell(rr, slot, UI_CHAT063_POPUP_X0, UI_CHAT063_POPUP_Y0,
                   UI_CHAT063_POPUP_W, UI_CHAT063_POPUP_H, 0, uc);
    /* ③ Msg 输入框占位（浅色矩形，datah_063 Msg 区简化为占位标签） */
    rs_solid_btn(rr, slot, 180.0f, UI_CHAT063_POPUP_Y0 + 40.0f,
                 540.0f, UI_CHAT063_POPUP_Y0 + 240.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    /* ④ 标题"聊天"（chat063_text row0，面板顶部 360 中心） */
    rs_atlas_label(rr, slot, rr->tex_chat063_text[slot], rr->chat063_text_w, rr->chat063_text_h,
                   CHAT063_ROW, 0, 360.0f, UI_CHAT063_POPUP_Y0 + 20.0f, 130.0f, uc);
    /* ⑤ Msg 输入框内占位标签（row1=输入 row2=占位 row3=边界 row4=未逆向） */
    if (rr->tex_chat063_text[slot] && rr->chat063_text_w > 0) {
        rs_atlas_label(rr, slot, rr->tex_chat063_text[slot], rr->chat063_text_w, rr->chat063_text_h,
                       CHAT063_ROW, 1, 360.0f, UI_CHAT063_POPUP_Y0 + 80.0f, 150.0f, uc);   /* 输入 */
        rs_atlas_label(rr, slot, rr->tex_chat063_text[slot], rr->chat063_text_w, rr->chat063_text_h,
                       CHAT063_ROW, 2, 360.0f, UI_CHAT063_POPUP_Y0 + 130.0f, 150.0f, uc);  /* 占位 */
        rs_atlas_label(rr, slot, rr->tex_chat063_text[slot], rr->chat063_text_w, rr->chat063_text_h,
                       CHAT063_ROW, 3, 360.0f, UI_CHAT063_POPUP_Y0 + 180.0f, 150.0f, uc);  /* 边界 */
        rs_atlas_label(rr, slot, rr->tex_chat063_text[slot], rr->chat063_text_w, rr->chat063_text_h,
                       CHAT063_ROW, 4, 360.0f, UI_CHAT063_POPUP_Y0 + 230.0f, 150.0f, uc);  /* 未逆向 */
    }
    /* ⑥ 3 按钮：发送（蓝）/ 记录（橙）/ 关闭（灰） */
    static const float BTN_COL[3][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 发送 蓝 */
        {0.85f, 0.40f, 0.20f, 0.9f},   /* 记录 橙 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 关闭 灰 */
    };
    const ui_chat063_btn *c63 = ui_chat063_buttons();
    for (int i = 0; i < UI_CHAT063_COUNT; i++) {
        rs_solid_btn(rr, slot, c63[i].x0, c63[i].y0, c63[i].x1, c63[i].y1,
                     BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3], 0, uc);
        int row_idx = 5 + i;   /* row5=发送 row6=记录 row7=关闭 */
        rs_atlas_label(rr, slot, rr->tex_chat063_text[slot], rr->chat063_text_w, rr->chat063_text_h,
                       CHAT063_ROW, row_idx,
                       (c63[i].x0 + c63[i].x1) * 0.5f, (c63[i].y0 + c63[i].y1) * 0.5f, 120.0f, uc);
    }
    static int chd2 = 0;
    if (!chd2) {
        chd2 = 1;
        LOGI("rs_draw_chat063：聊天完整版弹窗（datah_063 Msg 输入框（占位标签）+ 发送/记录/关闭 3 按钮，"
              "面板 %.0f×%.0f，诚实边界：联机聊天文本未逆向，发送=BT_T_CHAT 占位帧+边界对话框，"
              "记录→UI_CHATLOG（datah_029 已实现））",
              UI_CHAT063_POPUP_W, UI_CHAT063_POPUP_H);
    }
}


/* ★ P2（datah_000 证据）：软件信息弹窗渲染——棋盘背景 + 面板
   + Version 版本号文本 + 官网文本 + 反馈/关闭 2 按钮 + 文字图集。
   按钮几何 ui_about_buttons()（720×1280 基准）。
   ★ 诚实边界：原版反馈=邮件/客服通道（未逆向），src_rs 无反馈通道，
   反馈按钮→诚实边界对话框，不包装成"已反馈"。 */
#define ABOUT_ROW 7
static void rs_draw_about(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（棋盘背景 + 遮罩 + 深色面板 400×280 居中） */
    rs_popup_shell(rr, slot, UI_ABOUT_POPUP_X0, UI_ABOUT_POPUP_Y0,
                   UI_ABOUT_POPUP_W, UI_ABOUT_POPUP_H, 0, uc);
    /* ③ Version 文本框（浅色矩形占位，datah_000 Version 140,90,390,120 400 系 → 300,590-550,620 720 基准） */
    rs_solid_btn(rr, slot, 300.0f, 590.0f, 550.0f, 620.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    /* ④ 官网文本框（浅色矩形占位，datah_000 官网 145,135,390,165 400 系 → 305,635-550,665 720 基准） */
    rs_solid_btn(rr, slot, 305.0f, 635.0f, 550.0f, 665.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    /* ⑤ 标题"软件信息"（about_text row0，面板顶部 360,520 720 基准中心） */
    rs_atlas_label(rr, slot, rr->tex_about_text[slot], rr->about_text_w, rr->about_text_h,
                   ABOUT_ROW, 0, 360.0f, 520.0f, 130.0f, uc);
    /* ⑥ Version 文本框内"1.6.8"（row5=1.6.8）+ 官网文本框内"cnvcs.com"（row6=cnvcs.com） */
    if (rr->tex_about_text[slot] && rr->about_text_w > 0) {
        rs_atlas_label(rr, slot, rr->tex_about_text[slot], rr->about_text_w, rr->about_text_h,
                       ABOUT_ROW, 5, 425.0f, 605.0f, 200.0f, uc);   /* 版本号 1.6.8 */
        rs_atlas_label(rr, slot, rr->tex_about_text[slot], rr->about_text_w, rr->about_text_h,
                       ABOUT_ROW, 6, 427.0f, 650.0f, 200.0f, uc);   /* 官网 cnvcs.com */
        static int avid = 0;
        if (!avid) {
            avid = 1;
            LOGI("rs_draw_about：软件信息 Version=1.6.8 + 官网 www.cnvcs.com（占位标签，未烘焙动态版本字段）");
        }
    }
    /* ⑦ 2 按钮：反馈（蓝）/ 关闭（灰）+ 文字 */
    static const float BTN_COL[2][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 反馈 蓝 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 关闭 灰 */
    };
    const ui_about_btn *ab = ui_about_buttons();
    for (int i = 0; i < UI_ABOUT_COUNT; i++) {
        rs_solid_btn(rr, slot, ab[i].x0, ab[i].y0, ab[i].x1, ab[i].y1,
                     BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3], 0, uc);
        int row_idx = 3 + i;   /* row3=反馈 row4=关闭 */
        rs_atlas_label(rr, slot, rr->tex_about_text[slot], rr->about_text_w, rr->about_text_h,
                       ABOUT_ROW, row_idx,
                       (ab[i].x0 + ab[i].x1) * 0.5f, (ab[i].y0 + ab[i].y1) * 0.5f, 120.0f, uc);
    }
    (void)sx; (void)sy;
    static int ab2 = 0;
    if (!ab2) {
        ab2 = 1;
        LOGI("rs_draw_about：软件信息弹窗（datah_000 Version 版本号 1.6.8 + 官网 www.cnvcs.com + "
              "反馈/关闭 2 按钮，面板 %.0f×%.0f，诚实边界：原版反馈通道未逆向，src_rs 无反馈通道，"
              "反馈按钮→边界对话框，不包装成已反馈）",
              UI_ABOUT_POPUP_W, UI_ABOUT_POPUP_H);
    }
}


static void rs_draw_replay(rs_render *rr, int slot)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪ 背景：对局棋盘背景（复盘态画当前回放位置棋盘，复用 tex_bg 全图铺满） */
    if (rr->tex_bg[slot] && rr->prog_tex[slot]) {
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
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
    }
    /* ① 棋子占位方块（当前回放位置，诚实边界不画真棋子图） */
    rs_replay_pieces(rr, slot, uc);
    /* ② 控制条 5 按钮（datah_107，460×60 @10,705 不居中；先画条底再 5 按钮+文字） */
    {
        rs_v4f pq[6] = {
            {UI_REPLAY_CTRL_POPUP_X0 * sx, UI_REPLAY_CTRL_POPUP_Y0 * sy, 0, 0},
            {(UI_REPLAY_CTRL_POPUP_X0 + UI_REPLAY_CTRL_POPUP_W) * sx, UI_REPLAY_CTRL_POPUP_Y0 * sy, 0, 0},
            {UI_REPLAY_CTRL_POPUP_X0 * sx, (UI_REPLAY_CTRL_POPUP_Y0 + UI_REPLAY_CTRL_POPUP_H) * sy, 0, 0},
            {UI_REPLAY_CTRL_POPUP_X0 * sx, (UI_REPLAY_CTRL_POPUP_Y0 + UI_REPLAY_CTRL_POPUP_H) * sy, 0, 0},
            {(UI_REPLAY_CTRL_POPUP_X0 + UI_REPLAY_CTRL_POPUP_W) * sx, UI_REPLAY_CTRL_POPUP_Y0 * sy, 0, 0},
            {(UI_REPLAY_CTRL_POPUP_X0 + UI_REPLAY_CTRL_POPUP_W) * sx, (UI_REPLAY_CTRL_POPUP_Y0 + UI_REPLAY_CTRL_POPUP_H) * sy, 0, 0},
        };
        glUseProgram(rr->prog_solid[slot]);
        glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
        glUniform4f(uc, 0.10f, 0.10f, 0.14f, 0.92f);   /* 条底 深色 */
        glGenBuffers(1, &rr->vbo_sel[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_sel[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
        glDeleteBuffers(1, &rr->vbo_sel[slot]);
        rr->vbo_sel[slot] = 0;
        static const float BTN_COL[UI_REPLAY_CTRL_COUNT][4] = {
            {0.45f, 0.45f, 0.55f, 0.9f},   /* 菜单 灰 */
            {0.85f, 0.40f, 0.20f, 0.9f},   /* 回退 橙 */
            {0.20f, 0.65f, 0.35f, 0.9f},   /* 前进 绿 */
            {0.45f, 0.30f, 0.75f, 0.9f},   /* 跳转 紫 */
            {0.20f, 0.45f, 0.90f, 0.9f},   /* 返回 蓝 */
        };
        const ui_replay_btn *rb = ui_replay_ctrl_buttons();
        for (int i = 0; i < UI_REPLAY_CTRL_COUNT; i++) {
            float bx0 = rb[i].x0 * sx, by0 = rb[i].y0 * sy;
            float bx1 = rb[i].x1 * sx, by1 = rb[i].y1 * sy;
            rs_v4f bq[6] = {
                {bx0, by0, 0, 0}, {bx1, by0, 0, 0}, {bx0, by1, 0, 0},
                {bx0, by1, 0, 0}, {bx1, by0, 0, 0}, {bx1, by1, 0, 0},
            };
            glUseProgram(rr->prog_solid[slot]);
            glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform4f(uc, BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3]);
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
            /* 按钮文字（row1-5=菜单/回退/前进/跳转/返回，按钮内居中） */
            rs_replay_text(rr, slot, (rb[i].x0 + rb[i].x1) * 0.5f - 45.0f,
                           (rb[i].y0 + rb[i].y1) * 0.5f - 23.0f, 90.0f, 46.0f, 1 + i, uc);
        }
    }
    /* ③ 步数标签（"复盘" 标题 row0 顶部 + 位置行 row6"第cur/n步"，文字图集占位标签；
       动态 cur/n 数字未烘焙 → 诚实边界：只画"第步/结束"占位标签 + LOGI 实际步数） */
    {
        int cur = ui_replay_cur(), n = ui_replay_count();
        rs_replay_text(rr, slot, 280.0f, 300.0f, 150.0f, 46.0f, 0, uc);      /* "复盘" 标题 */
        rs_replay_text(rr, slot, 280.0f, 780.0f, 150.0f, 46.0f, 6, uc);     /* "第步" 位置行占位 */
        if (n == 0)
            rs_replay_text(rr, slot, 280.0f, 830.0f, 150.0f, 46.0f, 7, uc); /* "结束"（无记录） */
        static int rdiag = 0;
        if (!rdiag) {
            rdiag = 1;
            LOGI("rs_draw_replay：复盘回放渲染（datah_107 控制条 5 按钮 460×60@10,705 + 棋盘占位 + 步数标签 cur=%d/%d，jump 简化回开头/跳结尾，菜单简化退复盘）", cur, n);
        }
    }
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
    /* ★ 对抗审计（datah_072）：方式选择文字图集（4 标签竖排 220×184） */
    if (rr->waysel_text_px && rr->gl2_ok[slot] && rr->waysel_text_w > 0 && rr->waysel_text_h > 0) {
        glGenTextures(1, &rr->tex_waysel_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_waysel_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->waysel_text_w, rr->waysel_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->waysel_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ 对抗审计（datah_067）：联机 ☰ 菜单文字图集（8 项竖排 160×320） */
    if (rr->btimen_text_px && rr->gl2_ok[slot] && rr->btimen_text_w > 0 && rr->btimen_text_h > 0) {
        glGenTextures(1, &rr->tex_btimen_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_btimen_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->btimen_text_w, rr->btimen_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->btimen_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ 对抗审计（datah_001）：声音设置文字图集（10 标签竖排 200×460） */
    if (rr->settings_text_px && rr->gl2_ok[slot] && rr->settings_text_w > 0 && rr->settings_text_h > 0) {
        glGenTextures(1, &rr->tex_settings_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_settings_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->settings_text_w, rr->settings_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->settings_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P0（datah_089）：四国人数选择文字图集（3 标签竖排 220×138） */
    if (rr->nplayer_text_px && rr->gl2_ok[slot] && rr->nplayer_text_w > 0 && rr->nplayer_text_h > 0) {
        glGenTextures(1, &rr->tex_nplayer_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_nplayer_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->nplayer_text_w, rr->nplayer_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->nplayer_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P0（datah_062）：两人新局文字图集（6 标签竖排 220×276） */
    if (rr->newgame_text_px && rr->gl2_ok[slot] && rr->newgame_text_w > 0 && rr->newgame_text_h > 0) {
        glGenTextures(1, &rr->tex_newgame_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_newgame_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->newgame_text_w, rr->newgame_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->newgame_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P1（datah_012）：游戏设置文字图集（8 标签竖排 220×368） */
    if (rr->gameset_text_px && rr->gl2_ok[slot] && rr->gameset_text_w > 0 && rr->gameset_text_h > 0) {
        glGenTextures(1, &rr->tex_gameset_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_gameset_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->gameset_text_w, rr->gameset_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->gameset_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P1（datah_016/017/020）：布局存档文字图集（9 标签竖排 220×414） */
    if (rr->layres_text_px && rr->gl2_ok[slot] && rr->layres_text_w > 0 && rr->layres_text_h > 0) {
        glGenTextures(1, &rr->tex_layres_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_layres_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->layres_text_w, rr->layres_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->layres_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P1（datah_039/107）：复盘文字图集（8 标签竖排 220×368） */
    if (rr->replay_text_px && rr->gl2_ok[slot] && rr->replay_text_w > 0 && rr->replay_text_h > 0) {
        glGenTextures(1, &rr->tex_replay_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_replay_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->replay_text_w, rr->replay_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->replay_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P1（datah_038）：登录文字图集（8 标签竖排 220×368） */
    if (rr->login_text_px && rr->gl2_ok[slot] && rr->login_text_w > 0 && rr->login_text_h > 0) {
        glGenTextures(1, &rr->tex_login_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_login_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->login_text_w, rr->login_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->login_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_037）：规则文本文字图集（8 标签竖排 220×368） */
    if (rr->rules_text_px && rr->gl2_ok[slot] && rr->rules_text_w > 0 && rr->rules_text_h > 0) {
        glGenTextures(1, &rr->tex_rules_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_rules_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->rules_text_w, rr->rules_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->rules_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_104）：新局完整版文字图集（10 标签竖排 220×460） */
    if (rr->ng104_text_px && rr->gl2_ok[slot] && rr->ng104_text_w > 0 && rr->ng104_text_h > 0) {
        glGenTextures(1, &rr->tex_ng104_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_ng104_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->ng104_text_w, rr->ng104_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->ng104_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_034/035）：加入/创建游戏文字图集（10 标签竖排 220×460） */
    if (rr->join_text_px && rr->gl2_ok[slot] && rr->join_text_w > 0 && rr->join_text_h > 0) {
        glGenTextures(1, &rr->tex_join_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_join_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->join_text_w, rr->join_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->join_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_061）：新局完整版（完整版补充）文字图集（10 标签竖排 220×460） */
    if (rr->ng061_text_px && rr->gl2_ok[slot] && rr->ng061_text_w > 0 && rr->ng061_text_h > 0) {
        glGenTextures(1, &rr->tex_ng061_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_ng061_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->ng061_text_w, rr->ng061_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->ng061_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_050）：对局信息文字图集（8 标签竖排 220×368） */
    if (rr->gameinfo_text_px && rr->gl2_ok[slot] && rr->gameinfo_text_w > 0 && rr->gameinfo_text_h > 0) {
        glGenTextures(1, &rr->tex_gameinfo_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_gameinfo_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->gameinfo_text_w, rr->gameinfo_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->gameinfo_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_002）：亮度调节文字图集（10 标签竖排 220×460） */
    if (rr->bright_text_px && rr->gl2_ok[slot] && rr->bright_text_w > 0 && rr->bright_text_h > 0) {
        glGenTextures(1, &rr->tex_bright_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_bright_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->bright_text_w, rr->bright_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->bright_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_029）：聊天记录文字图集（8 标签竖排 220×368） */
    if (rr->chatlog_text_px && rr->gl2_ok[slot] && rr->chatlog_text_w > 0 && rr->chatlog_text_h > 0) {
        glGenTextures(1, &rr->tex_chatlog_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_chatlog_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->chatlog_text_w, rr->chatlog_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->chatlog_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_031）：个人资料文字图集（10 标签竖排 220×460） */
    if (rr->profile_text_px && rr->gl2_ok[slot] && rr->profile_text_w > 0 && rr->profile_text_h > 0) {
        glGenTextures(1, &rr->tex_profile_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_profile_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->profile_text_w, rr->profile_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->profile_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_042）：提和详情文字图集（7 标签竖排 220×322） */
    if (rr->drawdetail_text_px && rr->gl2_ok[slot] && rr->drawdetail_text_w > 0 && rr->drawdetail_text_h > 0) {
        glGenTextures(1, &rr->tex_drawdetail_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_drawdetail_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->drawdetail_text_w, rr->drawdetail_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->drawdetail_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_003/004）：设置入口文字图集（7 标签竖排 220×322） */
    if (rr->setentry_text_px && rr->gl2_ok[slot] && rr->setentry_text_w > 0 && rr->setentry_text_h > 0) {
        glGenTextures(1, &rr->tex_setentry_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_setentry_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->setentry_text_w, rr->setentry_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->setentry_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_007/008）：图像设置文字图集（10 标签竖排 220×460） */
    if (rr->imgset_text_px && rr->gl2_ok[slot] && rr->imgset_text_w > 0 && rr->imgset_text_h > 0) {
        glGenTextures(1, &rr->tex_imgset_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_imgset_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->imgset_text_w, rr->imgset_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->imgset_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_065）：加入游戏完整版文字图集（10 标签竖排 220×460） */
    if (rr->join065_text_px && rr->gl2_ok[slot] && rr->join065_text_w > 0 && rr->join065_text_h > 0) {
        glGenTextures(1, &rr->tex_join065_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_join065_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->join065_text_w, rr->join065_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->join065_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_069）：创建游戏完整版文字图集（10 标签竖排 220×460） */
    if (rr->create069_text_px && rr->gl2_ok[slot] && rr->create069_text_w > 0 && rr->create069_text_h > 0) {
        glGenTextures(1, &rr->tex_create069_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_create069_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->create069_text_w, rr->create069_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->create069_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_070）：游戏规则完整版文字图集（10 标签竖排 220×460） */
    if (rr->rules070_text_px && rr->gl2_ok[slot] && rr->rules070_text_w > 0 && rr->rules070_text_h > 0) {
        glGenTextures(1, &rr->tex_rules070_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_rules070_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->rules070_text_w, rr->rules070_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->rules070_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_060）：房间态文字图集（8 标签竖排 220×368） */
    if (rr->room060_text_px && rr->gl2_ok[slot] && rr->room060_text_w > 0 && rr->room060_text_h > 0) {
        glGenTextures(1, &rr->tex_room060_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_room060_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->room060_text_w, rr->room060_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->room060_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_063）：聊天完整版文字图集（8 标签竖排 220×368） */
    if (rr->chat063_text_px && rr->gl2_ok[slot] && rr->chat063_text_w > 0 && rr->chat063_text_h > 0) {
        glGenTextures(1, &rr->tex_chat063_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_chat063_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->chat063_text_w, rr->chat063_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->chat063_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_000）：软件信息文字图集（7 标签竖排 220×322） */
    if (rr->about_text_px && rr->gl2_ok[slot] && rr->about_text_w > 0 && rr->about_text_h > 0) {
        glGenTextures(1, &rr->tex_about_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_about_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->about_text_w, rr->about_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->about_text_px);
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
        /* ★ P2 图像设置开关接渲染层（datah_007/008，选项真实生效非仅 UI 记录）：
           - 界面动画关（ui_imgset_trans=0）：棋子/棋盘不画入场过渡（src_rs 无过渡帧，
             关闭=强制棋子立即到位，诚实边界：动画帧未实现，开关仅门控"是否画"）
           - 夜间模式开（ui_imgset_night=1）：棋盘背景加暗色遮罩（全屏半透黑）
           - 棋子动画关（ui_imgset_piece=0）：棋子不画（诚实边界：过渡帧未实现，
             关闭=棋子占位不画，对齐原版"关动画棋子静止"语义） */
        int imgset_trans  = ui_imgset_cfg_get(0);   /* 界面动画（过渡帧未实现，门控用） */
        (void)imgset_trans;
        int imgset_piece  = ui_imgset_cfg_get(1);   /* 棋子动画：=0 时棋子不画（静止占位） */
        int imgset_night  = ui_imgset_cfg_get(2);   /* 夜间模式：=1 时棋盘加暗色遮罩 */
        int imgset_opp    = ui_imgset_cfg_get(3);   /* 对方棋子：=0 时红方棋子不画 */
        {
            int sig = imgset_trans * 8 + imgset_piece * 4 + imgset_night * 2 + imgset_opp;
            static int imgset_diag = -1;
            if (imgset_diag != sig) {
                imgset_diag = sig;
                LOGI("rs_render_frame：图像设置 4 开关接渲染层（界面=%d 棋子动画=%d 夜间=%d 对方棋子=%d；"
                     "夜间=1→暗色遮罩/对方=0→红方隐藏/棋子动画=0→棋子不画，真实生效非仅 UI 记录）",
                     imgset_trans, imgset_piece, imgset_night, imgset_opp);
            }
        }
        if (imgset_night) {
            /* 夜间模式：全屏暗色遮罩（棋子渲染前画，半透黑） */
            rs_v4f nq[6] = {{0,0,0,0},{vw,0,1,0},{0,vh,0,1},
                             {0,vh,0,1},{vw,0,1,0},{vw,vh,1,1}};
            glUseProgram(rr->prog_solid[slot]);
            glUniform2f(rr->uViewWH_solid[slot], (GLfloat)vw, (GLfloat)vh);
            GLint ncu = glGetUniformLocation(rr->prog_solid[slot], "uColor");
            glUniform4f(ncu, 0.0f, 0.0f, 0.05f, 0.45f);   /* 暗蓝黑遮罩 */
            glGenBuffers(1, &rr->vbo_sel[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(nq), nq, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_sel[slot]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rs_unbind_vbo();
            glDeleteBuffers(1, &rr->vbo_sel[slot]);
            rr->vbo_sel[slot] = 0;
        }
        /* 棋子动画关 → 不画棋子（占位静止，诚实边界）；界面动画关 → 同上（无过渡帧可省） */
        static int piece_diag_done = 0;
        static rs_v4f piece_pool[N_CELLS * 6];
        static rs_v4f sel_line[8];
        int nverts = 0;
        for (int i = 0; i < N_CELLS; i++) {
            const jq_cell *c = &rr->game->cells[i];
            if (c->rank == RANK_EMPTY) continue;
            /* ★ P2 图像设置开关接渲染层（真实生效，非仅 UI 记录）：
               - 对方棋子开关（ui_imgset_cfg_get(3)，默认 1=显示）：=0 时红方棋子不画
               - 棋子动画开关（ui_imgset_cfg_get(1)，默认 1=开）：=0 时棋子不画（静止占位）
               诚实边界：界面动画过渡帧未实现（开关仅门控）。 */
            if (!imgset_opp && c->side == SIDE_RED) continue;
            if (!imgset_piece) continue;
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
           对抗审计（datah_041）：UI_DIFFICULTY 画人机难度弹窗（新手/中级/高级/好友 × 布局/翻棋） */
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
        } else if (ui_screen_get() == UI_DIFFICULTY) {
            /* ★ 对抗审计（datah_041 证据）：人机难度选择弹窗 */
            rs_draw_difficulty(rr, slot);
        } else if (ui_screen_get() == UI_WAYSEL) {
            /* ★ 对抗审计（datah_072 证据）：联网/联机方式选择弹窗 */
            rs_draw_waysel(rr, slot);
        } else if (ui_screen_get() == UI_SETTINGS) {
            /* ★ 对抗审计（datah_001 证据）：声音设置弹窗 */
            rs_draw_settings(rr, slot);
        } else if (ui_screen_get() == UI_NPLAYER) {
            /* ★ P0（datah_089 证据）：四国人数选择弹窗（2/3/4 人） */
            rs_draw_nplayer(rr, slot);
        } else if (ui_screen_get() == UI_NEWGAME) {
            /* ★ P0（datah_062 证据）：两人新局简化弹窗（玩法/明暗棋+确定/取消） */
            rs_draw_newgame(rr, slot);
        } else if (ui_screen_get() == UI_GAMESET) {
            /* ★ P1（datah_012 证据）：游戏设置弹窗（4 规则开关+确定/取消） */
            rs_draw_gameset(rr, slot);
        } else if (ui_screen_get() == UI_LOAD_LAY) {
            /* ★ P1（datah_017 证据）：载入布局弹窗（布局名列表 6 条+取消） */
            rs_draw_loadlay(rr, slot);
        } else if (ui_screen_get() == UI_SAVE_LAY) {
            /* ★ P1（datah_020 证据）：保存布局弹窗（保存/改名/删除/关闭 4 按钮） */
            rs_draw_savelay(rr, slot);
        } else if (ui_screen_get() == UI_LAY_MGR) {
            /* ★ P1（datah_016 证据）：布局管理弹窗（30 格预览小方块+载入/删除/取消） */
            rs_draw_laymgr(rr, slot);
        } else if (ui_screen_get() == UI_LOGIN) {
            /* ★ P1（datah_038 证据）：登录弹窗（用户名/密码占位标签 + 登录/注册/游客/返回 4 按钮） */
            rs_draw_login(rr, slot);
        } else if (ui_screen_get() == UI_REPLAY) {
            /* ★ P1（datah_039/107 证据）：复盘回放态（棋盘当前回放位置 + datah_107 控制条 5 按钮 + 步数标签） */
            rs_draw_replay(rr, slot);
        } else if (ui_screen_get() == UI_RULES) {
            /* ★ P2（datah_037 证据）：规则文本弹窗（规则-翻棋/规则-布局 2 模式 + 返回，面板 260×270 居中） */
            rs_draw_rules(rr, slot);
        } else if (ui_screen_get() == UI_NEWGAME104) {
            /* ★ P2（datah_104 证据）：新局完整版弹窗（我方/电脑水平/玩法 3 选择 + 确定/取消，面板 450×390 居中） */
            rs_draw_newgame104(rr, slot);
        } else if (ui_screen_get() == UI_JOIN) {
            /* ★ P2（datah_034 证据）：联机"加入游戏"弹窗（桌号占位标签 + 确定/取消，面板 420×235 居中） */
            rs_draw_join(rr, slot, 0);
        } else if (ui_screen_get() == UI_CREATE) {
            /* ★ P2（datah_035 证据）：联机"创建游戏"弹窗（密码占位标签 + 确定/取消，面板 420×235 居中） */
            rs_draw_join(rr, slot, 1);
        } else if (ui_screen_get() == UI_NG061) {
            /* ★ P2（datah_061 证据）：新局完整版（完整版补充）弹窗（先后手/电脑水平/玩法/明暗棋 4 选择 + 确定/取消，面板 450×465 居中） */
            rs_draw_ng061(rr, slot);
        } else if (ui_screen_get() == UI_GAMEINFO) {
            /* ★ P2（datah_050 证据）：对局信息弹窗（Info 文本框 + 复盘/删除/返回 3 按钮，面板 420×380 居中） */
            rs_draw_gameinfo(rr, slot);
        } else if (ui_screen_get() == UI_BRIGHT) {
            /* ★ P2（datah_002 证据）：亮度调节弹窗（滑条简化=亮度 低/中/高 3 档循环 + 确定/取消，面板 460×240 居中） */
            rs_draw_bright(rr, slot);
        } else if (ui_screen_get() == UI_CHATLOG) {
            /* ★ P2（datah_029 证据）：聊天记录弹窗（Msg 文本框 + 清除/关闭 2 按钮，面板 420×530 居中） */
            rs_draw_chatlog(rr, slot);
        } else if (ui_screen_get() == UI_PROFILE) {
            /* ★ P2（datah_031 证据）：个人资料弹窗（Info 文本框 + 修改/帮助/返回 3 按钮，面板 420×520 居中） */
            rs_draw_profile(rr, slot);
        } else if (ui_screen_get() == UI_DRAWDETAIL) {
            /* ★ P2（datah_042 证据）：提和详情弹窗（联机版 ☰ 菜单 7 图标：提和/认输/聊天/系统设置/玩家资料/
               战况统计/软件信息，面板 450×285 居中） */
            rs_draw_drawdetail(rr, slot);
        } else if (ui_screen_get() == UI_SETEXTTRY) {
            /* ★ P2（datah_003 证据）：设置入口弹窗（完整版，图像/声音/游戏设置 3 按钮，面板 260×270 居中） */
            rs_draw_setentry(rr, slot, 0);
        } else if (ui_screen_get() == UI_SETEXTTRY4) {
            /* ★ P2（datah_004 证据）：设置入口（简化版）弹窗（图像/声音 2 按钮，面板 260×190 居中） */
            rs_draw_setentry(rr, slot, 1);
        } else if (ui_screen_get() == UI_IMGSET7) {
            /* ★ P2（datah_007 证据）：图像设置弹窗（完整版，4 开关 + 确定/取消，面板 450×465 居中） */
            rs_draw_imgset(rr, slot, 0);
        } else if (ui_screen_get() == UI_IMGSET8) {
            /* ★ P2（datah_008 证据）：图像设置弹窗（简化版，3 开关 + 确定/取消，面板 450×390 居中） */
            rs_draw_imgset(rr, slot, 1);
        } else if (ui_screen_get() == UI_JOIN065) {
            /* ★ P2（datah_065 证据）：加入游戏完整版弹窗（8 显示占位标签 + 开始/取消，面板 450×505 居中） */
            rs_draw_join065(rr, slot);
        } else if (ui_screen_get() == UI_CREATE069) {
            /* ★ P2（datah_069 证据）：创建游戏完整版弹窗（4 选择 + 确定/取消，面板 450×465 居中） */
            rs_draw_create069(rr, slot);
        } else if (ui_screen_get() == UI_RULES070) {
            /* ★ P2（datah_070 证据）：游戏规则完整版弹窗（4 规则开关 + 确定/取消，面板 450×465 居中） */
            rs_draw_rules070(rr, slot);
        } else if (ui_screen_get() == UI_ROOM060) {
            /* ★ P2（datah_060 证据）：房间态弹窗（2 玩家占位标签 + 开始/换桌/离开，面板 450×352 居中） */
            rs_draw_room060(rr, slot);
        } else if (ui_screen_get() == UI_CHAT063) {
            /* ★ P2（datah_063 证据）：聊天完整版弹窗（Msg 输入框占位 + 发送/记录/关闭，面板 420×325 居中） */
            rs_draw_chat063(rr, slot);
        } else if (ui_screen_get() == UI_ABOUT) {
            /* ★ P2（datah_000 证据）：软件信息弹窗（Version 版本号 1.6.8 + 官网 www.cnvcs.com + 反馈/关闭，面板 400×280 居中） */
            rs_draw_about(rr, slot);
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
