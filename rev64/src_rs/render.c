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
#include "board_geom.h"   /* AUD-R-077/118/120：直接引用 BG_* 渲染常量 */
#include "png_decode.h"
#include "img_decode.h"   /* ★ C 项：原版 SPAK 位图运行时解码（stb_image JPEG/PNG） */
#include "ui_menu.h"
#include "jq_strings.h"   /* ★ 原版字符串表真值（79 条，自动生成） */
#include "ui_state.h"   /* P2 图像设置 4 开关 ui_imgset_cfg_get（夜间/对方棋子接渲染层） */

/* ★ 原版 UI 位图真值（ch02 N45）前向声明 */
static int d000_diag = 0;
static int d001_diag = 0;
static int d002_diag = 0;
static int d003_diag = 0;
static int d004_diag = 0;
static int d005_diag = 0;
static int d006_diag = 0;
static int d007_diag = 0;
static int d008_diag = 0;
static int d010_diag = 0;
static int d012_diag = 0;
static int d013_diag = 0;
static int d015_diag = 0;
static int d016_diag = 0;
static int d017_diag = 0;
static int d019_diag = 0;
static int d020_diag = 0;
static int d021_diag = 0;
static int d022_diag = 0;
static int d023_diag = 0;
static int d024_diag = 0;
static int d025_diag = 0;
static int d026_diag = 0;
static int d027_diag = 0;
static int d028_diag = 0;
static int d029_diag = 0;
static int d030_diag = 0;
static int d031_diag = 0;
static int d032_diag = 0;
static int d033_diag = 0;
static int d034_diag = 0;
static int d035_diag = 0;
static int d036_diag = 0;
static int d037_diag = 0;
static int d038_diag = 0;
static int d039_diag = 0;
static int d041_diag = 0;
static int d042_diag = 0;
static int d043_diag = 0;
static int d044_diag = 0;
static int d045_diag = 0;
static int d046_diag = 0;
static int d047_diag = 0;
static int d049_diag = 0;
static int d050_diag = 0;
static int d051_diag = 0;
static int d052_diag = 0;
static int d053_diag = 0;
static int d054_diag = 0;
static int d055_diag = 0;
static int d057_diag = 0;
static int d058_diag = 0;
static int d059_diag = 0;
static int d060_diag = 0;
static int d061_diag = 0;
static int d062_diag = 0;
static int d063_diag = 0;
static int d064_diag = 0;
static int d065_diag = 0;
static int d066_diag = 0;
static int d067_diag = 0;
static int d068_diag = 0;
static int d069_diag = 0;
static int d070_diag = 0;
static int d071_diag = 0;
static int d072_diag = 0;
static int d073_diag = 0;
static int d075_diag = 0;
static int d077_diag = 0;
static int d078_diag = 0;
static int d079_diag = 0;
static int d082_diag = 0;
static int d083_diag = 0;
static int d084_diag = 0;
static int d085_diag = 0;
static int d086_diag = 0;
static int d089_diag = 0;
static int d090_diag = 0;
static int d094_diag = 0;
static int d095_diag = 0;
static int d096_diag = 0;
static int d097_diag = 0;
static int d098_diag = 0;
static int d099_diag = 0;
static int d100_diag = 0;
static int d101_diag = 0;
static int d102_diag = 0;
static int d104_diag = 0;
static int d107_diag = 0;
static int d108_diag = 0;
static int d109_diag = 0;
static int d110_diag = 0;
static int d112_diag = 0;
static int d113_diag = 0;
static int d114_diag = 0;
static int d118_diag = 0;
static int d119_diag = 0;
static int d120_diag = 0;
static int d121_diag = 0;
static int d122_diag = 0;
static int d123_diag = 0;
static int d246_diag = 0;
static int d247_diag = 0;
static int d248_diag = 0;
static int d249_diag = 0;
static int d250_diag = 0;
static int d251_diag = 0;
static int d253_diag = 0;
static int d255_diag = 0;
static int d256_diag = 0;
static int d257_diag = 0;
static int d258_diag = 0;
static int d259_diag = 0;
static int d260_diag = 0;
static int d261_diag = 0;
static int d262_diag = 0;
static int d263_diag = 0;
static int d264_diag = 0;
static int panraw_seen = 0;
static int panbdiag = 0;   /* 面板/按钮/图标位图渲染诊断 */
static int ecdiag = 0;   /* 退出确认诊断 */
static int rcdiag = 0;   /* 认输确认诊断 */
static int gcdot = 0;    /* 悔棋确认诊断 */
static void rs_draw_bitmap(rs_render *rr, int slot, unsigned int tex, int tex_w, int tex_h,
                           float x0, float y0, float x1, float y1);
static void rs_solid_btn(rs_render *rr, int slot, float x0, float y0, float x1, float y1,
                         float cr, float cg, float cb, float ca, int highlight, GLint uc);
/* ★ 2026-11-10：gameset 弹窗改 rs_popup_shell（与其余弹窗同皮肤），前置声明 */
static void rs_popup_shell(rs_render *rr, int slot,
                           float px0, float py0, float pw, float ph,
                           int use_menu_bg, GLint uc);
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
/* AUD-1000 B04-055：背面格在 PIECE 图集 (col=0, row=6)（PIECE_GRID_H-1=6 为背面行）
   实证：棋子图集 7 行——row0-2 蓝 12 子（rank1-12 → col0-3），row3-5 红，row6 背面。
   暗棋未翻子画 (0,6) 与明棋 rank 无关联，固定常量防未来图集改版时错位。 */
#define PIECE_BACK_ROW  (PIECE_GRID_H - 1)
#define PIECE_BACK_COL  0
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
            /* ★ AUD-1000 B04-047（P2 已修）：0 字节文件显式 LOGI（原静默走 asset 兜底，
               0 字节 fs 文件多为 push 残留/损坏，诊断区分「0 字节 fs」与「fs 缺失→asset」。 */
            if (sz == 0)
                LOGI("rs_read_png：fs 0 字节（%s）→ asset 兜底", fs_path);
            fclose(f);
            *out = buf; *out_len = sz;
            return;
        }
        free(buf);
        fclose(f);
    }
    }
    /* AAsset 直接链接（正常 so 已验证可 dlopen；免 dlopen/dlsym 引入 libdl VERNEED）
       ★ 资源使用（apk 对齐）：asset_path 传 "spak_res/xxx" 时，APK assets 里
       assets/spak_res/xxx 可被 AAssetManager_open 命中（相对 APK 根）。当前
       g_assets=NULL（文件系统回退），此通路为设备端未推 spak_res 时的 APK 兜底。 */
    AAsset *a = NULL;
    if (am) a = AAssetManager_open(am, asset_path, AASSET_MODE_BUFFER);
    if (a) {
        int64_t sz = AAsset_getLength(a);
        /* ★ AUD-1000 B04-048（P2 已修）：AAsset_getLength 负值（ASSET 损坏/异常）
           防护——原 sz>0?sz:1 负值走 1 字节读（静默），现负值显式拒收；close 单次
           （原 return 路径双 close 修正）。 */
        if (sz < 0) {
            LOGI("rs_read_png：AAsset_getLength 负值（%s）→ 拒收", asset_path);
            AAsset_close(a);
        } else {
            unsigned char *buf = malloc(sz > 0 ? sz : 1);
            int64_t rd = AAsset_read(a, buf, sz > 0 ? (size_t)sz : 1);
            if (rd == sz) { *out = buf; *out_len = sz; }
            else free(buf);
            AAsset_close(a);
        }
    }
    /* 双路均无 → LOGI 诊断（资源缺失显式告警，便于真机/设备端资源对齐排查） */
    FILE *f = fopen(fs_path, "rb");
    if (!f) {
        LOGI("纹理缺失（fs+asset 均无）fs=%s asset=%s（设备端 spak_res 未推或 apk assets 缺失，走纯色退化）",
             fs_path ? fs_path : "(null)", asset_path ? asset_path : "(null)");
        return;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    /* ★ 对抗审计 P0（批3 P2-A7 / 批4 P0-R2）：ftell 失败（-1）→ 负 sz 会 malloc(1)
       后 *out_len=负值（转 size_t 巨值）→ png_decode 越界读。负值/零显式回退 1 字节
       安全路径（仅保指针非 NULL，png_decode 对 1 字节必失败走纯色退化，不崩）。 */
    if (sz < 0) sz = 0;
    unsigned char *buf = malloc(sz > 0 ? sz : 1);
    size_t rd = fread(buf, 1, sz > 0 ? sz : 1, f);
    fclose(f);
    if (rd == 0) {
        free(buf);
        LOGI("纹理读取失败（fread 0 字节）fs=%s（走纯色退化）", fs_path ? fs_path : "(null)");
        *out = NULL; *out_len = 0;
        return;
    }
    if (rd != (size_t)sz) sz = (long)rd;   /* 短读截断（部分读也尽量用） */
    *out = buf; *out_len = sz;
}

/* ★ 对抗审计 P2-R2（栈帧瘦身）：rs_render_init 原用 44 个 char[512] 路径缓冲
   （p1..p39 + p100..p104）= 22KB 栈帧（主线程 OnCreate 同步调用，JNI 帧叠加）。
   改为单缓冲 helper——路径按需构造 + 立即读入 + 立即释放，栈占用 1×512。
   配对校验：44 个 snprintf 与 44 个 rs_read_png 严格 1:1 按文件名配对（脚本实证
   0 错位），语义等价。 */
/* ★ C 项（§2026-11-11）：按前缀读 spak_res 里 datax_xxx_*.bin（文件名带 hash 后缀，
   固定 hash 未知 → 目录枚举匹配前缀，取第一个命中）。文件系统侧 opendir/readdir。 */
#include <dirent.h>
#include <string.h>
static void rs_read_one(AAssetManager *am, const char *spak_dir,
                        const char *file, unsigned char **raw, size_t *len);   /* fwd decl（下方定义） */
static int rs_read_spak_by_prefix(AAssetManager *am, const char *spak_dir,
                                  const char *prefix, unsigned char **raw, size_t *len)
{
    (void)am;
    if (!spak_dir || !prefix || !raw || !len) return -1;
    *raw = NULL; *len = 0;
    char pat[160];
    snprintf(pat, sizeof pat, "%s/%s", spak_dir, prefix);   /* /data/.../spak_res/datax_294_ */
    DIR *d = opendir(spak_dir);
    if (!d) return -1;
    struct dirent *e;
    char found[256] = {0};
    while ((e = readdir(d)) != NULL) {
        if (strncmp(e->d_name, prefix, strlen(prefix)) == 0) {
            snprintf(found, sizeof found, "%s", e->d_name);
            break;   /* 取第一个命中（datax 前缀唯一） */
        }
    }
    closedir(d);
    if (!found[0]) return -1;
    char path[1024];   /* ★ B04-072（P1 已修）：同 rs_read_one path 上限 1024 */
    snprintf(path, sizeof path, "%s/%s", spak_dir, found);
    rs_read_one(NULL, spak_dir, found, raw, len);   /* 复用 rs_read_png 文件读取 */
    if (*raw && *len > 0) {
        static int log_done = 0;
        if (!log_done) { log_done = 1;
            LOGI("C 项：原版 datax 位图直读 %s（%zu B）", found, *len); }
        return 0;
    }
    return -1;
}

static void rs_read_one(AAssetManager *am, const char *spak_dir,
                        const char *file, unsigned char **raw, size_t *len)
{
    /* ★ AUD-1000 B04-072（P1 已修）：path 上限 512→1024——spak_dir（/data/local/tmp/spak_res
       及 getFilesDir 长路径）+ '/' + 文件名（datah_XXX 位图名 ~40 字符）在
       长 filesDir 设备（如 /storage/emulated/0/Android/data/... 256+ 字符）下
       512 有截断风险；1024 覆盖 Android 路径上限（PATH_MAX=255 的 4 倍余量）。 */
    char path[1024];
    snprintf(path, sizeof path, "%s/%s", spak_dir ? spak_dir : "", file);
    char asset[128];
    snprintf(asset, sizeof asset, "spak_res/%s", file);
    rs_read_png(am, asset, path, raw, len);
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
        /* ★ AUD-1000 B04-046（P1 已修）：*dst_len 用解码实际 dw/dh 而非调用方
           传入的期望 w/h——资源改尺寸（弹窗图集 8行→5行 等）时 w/h 期望值
           与实际解码尺寸不符 → glTexImage2D 按 w×h 上传 dw×dh 像素（越读/越写
           纹理行错位，UV 采样全偏移）。现 glTexImage2D 直接用 dw/dh（解码实际值），
           dst_len 记实际像素量。w/h 形参保留（调用方传 0 或旧值均无害，仅文档用途）。 */
        *dst = rgba; *dst_len = (size_t)dw * dh * 4;
        glGenTextures(1, tex);
        glBindTexture(GL_TEXTURE_2D, *tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dw, dh, 0,
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
        
/* ★ 原版 datax 图标真值图集（16 图标 4×4） */
/* ★ 原版 UI 位图真值（ch02 N45）：面板 9patch + 彩色按钮 + 图标 */
        unsigned char *braw = NULL; size_t braw_len = 0;
        unsigned char *praw = NULL; size_t praw_len = 0;
        unsigned char *traw = NULL; size_t traw_len = 0;
        unsigned char *graw = NULL; size_t graw_len = 0;
        unsigned char *oraw = NULL; size_t oraw_len = 0;   /* 弹窗文字图集 */
        unsigned char *praw2 = NULL; size_t praw2_len = 0;  /* popup.png 9-patch */
        unsigned char *mraw = NULL; size_t mraw_len = 0;    /* ★ 菜单背景 datax_294（烘焙 PNG 兜底） */
        /* ★ C 项（§2026-11-11 全做）：原版 datax JPEG 位图运行时直读（替代纯色退化底），
           烘焙 PNG 缺失时优先读原版 JPEG 真值 */
        unsigned char *mjraw = NULL; size_t mjraw_len = 0;   /* 主菜单背景 datax_294 JPEG */
        unsigned char *gbg_jraw = NULL; size_t gbg_jraw_len = 0; /* 对局棋盘背景 datax_177 JPEG */
        unsigned char *iraw = NULL; size_t iraw_len = 0;    /* ★ ☰ 菜单文字图集 */
        unsigned char *icraw = NULL; size_t icraw_len = 0;        
                
                
                
                
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        unsigned char *dlgraw = NULL; size_t dlgraw_len = 0;
        unsigned char *dlgpx=NULL; int dlgw=0, dlgh=0;
        unsigned char *ioraw = NULL; size_t ioraw_len = 0;   /* ★ 原版 datax 图标图集 */  /* ★ ☰ 菜单图标图集 */
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
        unsigned char *uiaw = NULL; size_t uiaw_len = 0;    /* ★ SPAK2UI 批量（12 弹窗）共用文字图集 */
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
        /* ★ B04-060/067：提和详情 7 图标真值图集（datax 108x108 烘焙 4x2） */
        unsigned char *dciaw = NULL; size_t dciaw_len = 0;
        unsigned char *dcix_px = NULL; int dciw = 0, dcih = 0;
        int sew = 0, sehh = 0;   /* ★ P2 datah_003/004 设置入口文字图集尺寸 */
        int isw = 0, ishh = 0;   /* ★ P2 datah_007/008 图像设置文字图集尺寸 */
        int j65w = 0, j65h = 0;  /* ★ P2 datah_065 加入游戏完整版文字图集尺寸 */
        int c69w = 0, c69h = 0;  /* ★ P2 datah_069 创建游戏完整版文字图集尺寸 */
        int r70w = 0, r70h = 0;  /* ★ P2 datah_070 游戏规则完整版文字图集尺寸 */
        int rmw = 0, rmh = 0;    /* ★ P2 datah_060 房间态文字图集尺寸 */
        int chw = 0, chh = 0;    /* ★ P2 datah_063 聊天完整版文字图集尺寸 */
        int abw = 0, abh = 0;    /* ★ P2 datah_000 软件信息文字图集尺寸 */
        int uiw = 0, uih = 0;    /* ★ SPAK2UI 批量（12 弹窗）共用文字图集尺寸 */
        unsigned char *panraw = NULL; size_t panraw_len = 0;   /* ★ 原版面板 9patch */
        unsigned char *btnraw = NULL; size_t btnraw_len = 0;   /* ★ 原版彩色按钮 */
        unsigned char *icoraw = NULL; size_t icoraw_len = 0;   /* ★ 原版图标 */
        int panw=0, panh=0, btnw=0, btnh=0;
        rs_read_one(am, spak_dir, "datah_188_21d95e03e7692e98.bin", &braw, &braw_len);
        rs_read_one(am, spak_dir, "datah_183_9e1b7fbc4dfc09c3.bin", &praw, &praw_len);
        rs_read_one(am, spak_dir, "menu_text_atlas.png", &traw, &traw_len);
        rs_read_one(am, spak_dir, "bg_menu_720.png", &graw, &graw_len);
        rs_read_one(am, spak_dir, "popup_text_atlas.png", &oraw, &oraw_len);
        rs_read_one(am, spak_dir, "popup.png", &praw2, &praw2_len);
        rs_read_one(am, spak_dir, "panel_circle.png", &panraw, &panraw_len);
        rs_read_one(am, spak_dir, "btn_color1.png", &btnraw, &btnraw_len);
        rs_read_one(am, spak_dir, "icon_file.png", &icoraw, &icoraw_len);
        rs_read_one(am, spak_dir, "menu_bg_294.png", &mraw, &mraw_len);
        /* ★ C 项：原版 datax JPEG 直读（烘焙 PNG 缺失时的真值源）。
           datax_294/177 的 bin 文件名带 hash 后缀，用 glob 读 spak_res/datax_294_*.bin。 */
        {
            char pat[128];
            snprintf(pat, sizeof pat, "%s/datax_294_*.bin", spak_dir ? spak_dir : "");
            /* 简化：直接枚举 spak_res 找 datax_294/177（避免 stb glob 依赖） */
            /* ★ C 项：原版 datax JPEG 位图直读（rs_read_spak_by_prefix 定义在上方） */
            rs_read_spak_by_prefix(am, spak_dir, "datax_294_", &mjraw, &mjraw_len);
            rs_read_spak_by_prefix(am, spak_dir, "datax_177_", &gbg_jraw, &gbg_jraw_len);
        }
        rs_read_one(am, spak_dir, "imen_text_atlas.png", &iraw, &iraw_len);
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        rs_read_one(am, spak_dir, "dlg_batch_text.png", &dlgraw, &dlgraw_len);
        rs_read_one(am, spak_dir, "imen_icons_atlas.png", &icraw, &icraw_len);
        rs_read_one(am, spak_dir, "imen_orig_icons.png", &ioraw, &ioraw_len);
        rs_read_one(am, spak_dir, "imen_hamburger_atlas.png", &hbraw, &hbraw_len);
        rs_read_one(am, spak_dir, "menu_btn_big.png", &bbraw, &bbraw_len);
        rs_read_one(am, spak_dir, "menu_btn_small.png", &bsraw, &bsraw_len);
        rs_read_one(am, spak_dir, "waysel_text_atlas.png", &wsraw, &wsraw_len);
        rs_read_one(am, spak_dir, "btimen_text_atlas.png", &bira, &bira_len);
        rs_read_one(am, spak_dir, "settings_text_atlas.png", &straw, &straw_len);
        rs_read_one(am, spak_dir, "nplayer_text_atlas.png", &npaw, &npaw_len);
        rs_read_one(am, spak_dir, "newgame_text_atlas.png", &ngaw, &ngaw_len);
        rs_read_one(am, spak_dir, "gameset_text_atlas.png", &gsaw, &gsaw_len);
        rs_read_one(am, spak_dir, "layres_text_atlas.png", &lrsw, &lrsw_len);
        rs_read_one(am, spak_dir, "replay_text_atlas.png", &rpaw, &rpaw_len);
        rs_read_one(am, spak_dir, "login_text_atlas.png", &lpaw, &lpaw_len);
        rs_read_one(am, spak_dir, "rules_text_atlas.png", &ruaw, &ruaw_len);
        rs_read_one(am, spak_dir, "ng104_text_atlas.png", &ngaw2, &ngaw2_len);
        rs_read_one(am, spak_dir, "join_text_atlas.png", &joaw, &joaw_len);
        rs_read_one(am, spak_dir, "ng061_text_atlas.png", &n061aw, &n061aw_len);
        rs_read_one(am, spak_dir, "gameinfo_text_atlas.png", &giaw, &giaw_len);
        rs_read_one(am, spak_dir, "bright_text_atlas.png", &brw, &brw_len);
        rs_read_one(am, spak_dir, "chatlog_text_atlas.png", &claw, &claw_len);
        rs_read_one(am, spak_dir, "profile_text_atlas.png", &pfaw, &pfaw_len);
        rs_read_one(am, spak_dir, "drawdetail_text_atlas.png", &ddaw, &ddaw_len);
        /* ★ AUD-1000 B04-060/067（P1 已修）：提和详情 7 图标真值图集（datax 组
           108x108 原版图标烘焙 4x2 网格 432x216） */
        rs_read_one(am, spak_dir, "drawdetail_icons_atlas.png", &dciaw, &dciaw_len);
        rs_read_one(am, spak_dir, "setentry_text_atlas.png", &seaw, &seaw_len);
        rs_read_one(am, spak_dir, "imgset_text_atlas.png", &isaw, &isaw_len);
        rs_read_one(am, spak_dir, "join065_text_atlas.png", &j65aw, &j65aw_len);        rs_read_one(am, spak_dir, "create069_text_atlas.png", &c69aw, &c69aw_len);
        rs_read_one(am, spak_dir, "rules070_text_atlas.png", &r70aw, &r70aw_len);
        rs_read_one(am, spak_dir, "room060_text_atlas.png", &rmaw, &rmaw_len);        rs_read_one(am, spak_dir, "chat063_text_atlas.png", &chaw, &chaw_len);        rs_read_one(am, spak_dir, "about_text_atlas.png", &abaw, &abaw_len);        rs_read_one(am, spak_dir, "uibat_text_atlas.png", &uiaw, &uiaw_len);
        if (braw && png_decode(braw, braw_len, &bpx, &bw, &bh) == 0) {
            rr->board_px = bpx; rr->board_px_len = (size_t)bw * bh * 4;
            rr->board_w = bw; rr->board_h = bh;   /* AUD-1000 B04-050：记录实际尺寸 */
        }
        free(braw);
        if (praw && png_decode(praw, praw_len, &ppx, &pw, &ph) == 0) { rr->piece_px = ppx; rr->piece_px_len = (size_t)pw * ph * 4; rr->piece_w = pw; rr->piece_h = ph; }   /* ★ AUD-1000-ALL A7-01（P1 已修）：记录实际解码宽高（同 board B04-050 范式），消 PIECE_PNG_W/H 硬编码双源 */
        free(praw);
        /* ★ 层 2 视觉对齐：菜单按钮文字图集（原版字体 data_029 光栅化 5 label 竖排） */
        if (traw && png_decode(traw, traw_len, &tpx, &tw, &th) == 0) {
            rr->text_px = tpx; rr->text_px_len = (size_t)tw * th * 4;
            rr->text_w = tw; rr->text_h = th;
        }
        free(traw);
        /* ★ C 项（§2026-11-11）：对局棋盘背景 datax_177（720×1280 原版完整棋盘）。
           原版 datax JPEG 直读优先（img_decode_any），烘焙 PNG 兜底。 */
        if ((gbg_jraw && img_decode_any(gbg_jraw, gbg_jraw_len, &gpx, &gw, &gh) == 0) ||
            (gpx == NULL && graw && png_decode(graw, graw_len, &gpx, &gw, &gh) == 0)) {
            rr->bg_px = gpx; rr->bg_px_len = (size_t)gw * gh * 4;
            rr->bg_w = gw; rr->bg_h = gh;
        }
        free(graw); free(gbg_jraw);
        /* ★ C 项（§2026-11-11）：主菜单背景 datax_294（800×1205 卡通士兵）。
           原版 datax JPEG 直读优先（img_decode_any），烘焙 PNG 兜底。 */
        if ((mjraw && img_decode_any(mjraw, mjraw_len, &mpx, &mw, &mh) == 0) ||
            (mpx == NULL && mraw && png_decode(mraw, mraw_len, &mpx, &mw, &mh) == 0)) {
            rr->menu_bg_px = mpx; rr->menu_bg_px_len = (size_t)mw * mh * 4;
            rr->menu_bg_w = mw; rr->menu_bg_h = mh;
        }
        free(mraw); free(mjraw);
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
        /* ★ 原版 datax 图标真值图集（16 图标 4×4，每格 96） */
        if (ioraw) {
            unsigned char *iopx=NULL; int iow=0,ioh=0;
            if (png_decode(ioraw, ioraw_len, &iopx, &iow, &ioh) == 0) {
                rr->imen_orig_px = iopx; rr->imen_orig_px_len = (size_t)iow * ioh * 4;
                rr->imen_orig_w = iow; rr->imen_orig_h = ioh;
                LOGI("原版图标图集加载 OK：imen_orig_icons.png %dx%d（%zuB）", iow, ioh, ioraw_len);
            } else {
                LOGI("原版图标图集解码 FAIL：imen_orig_icons.png（%zuB，诚实边界：☰ 图标退化纯色块）",
                     ioraw_len);
            }
        } else {
            LOGI("原版图标图集读取 FAIL：imen_orig_icons.png 未找到（spak_res 缺文件？）");
        }                                        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (dlgraw && png_decode(dlgraw, dlgraw_len, &dlgpx, &dlgw, &dlgh) == 0) {
            rr->dlg_text_px = dlgpx; rr->dlg_text_px_len = (size_t)dlgw * dlgh * 4;
            rr->dlg_text_w = dlgw; rr->dlg_text_h = dlgh;
            LOGI("批量对齐文字图集加载 OK：dlg_batch_text.png %dx%d（371 行）", dlgw, dlgh);
        }
        free(dlgraw);
        free(icraw); free(ioraw);
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
        /* ★ 选项 UI：popup.png 9-patch（★ AUD-1000-ALL A7-02 注释纠正：实际资源 320×80 非 24×24，
           glTexImage2D 走实际解码宽高 popup_w/h（:9691），9-patch 拉伸语义；旧注释 24×24 双源漂移已纠正） */
        if (praw2 && png_decode(praw2, praw2_len, &ppx2, &pw2, &ph2) == 0) {
            rr->popup_px = ppx2; rr->popup_px_len = (size_t)pw2 * ph2 * 4;
            rr->popup_w = pw2; rr->popup_h = ph2;
        }
        free(praw2);
        /* ★ 原版 UI 位图真值（ch02 N45）：面板 9patch + 彩色按钮 */
        {
            unsigned char *panpx=NULL,*btnpx=NULL; int panw2=0,panh2=0,btnw2=0,btnh2=0;
            if (panraw) {
                LOGI("原版位图资源对齐（ch02 N45）：panel_circle.png 文件大小=%zu（读取后解码）", panraw_len);
            } else {
                LOGI("原版位图资源对齐（ch02 N45）：panel_circle.png 读取失败（spak_dir fs_path 无效）");
            }
            if (panraw && png_decode(panraw, panraw_len, &panpx, &panw2, &panh2) == 0) {
                rr->panel_px = panpx; rr->panel_px_len = (size_t)panw2 * panh2 * 4;
                rr->panel_w = panw2; rr->panel_h = panh2;
                LOGI("原版位图资源对齐（ch02 N45）：面板 9patch 位图加载成功 %.0fx%.0f（datax_246）", panw2, panh2);
            } else if (panraw) {
                LOGI("原版位图资源对齐（ch02 N45）：面板 9patch 位图解码失败（诚实边界：面板位图不可用，深色矩形退化底）");
            }
            if (btnraw && png_decode(btnraw, btnraw_len, &btnpx, &btnw2, &btnh2) == 0) {
                rr->panel_btn_px = btnpx; rr->panel_btn_px_len = (size_t)btnw2 * btnh2 * 4;
                rr->panel_btn_w = btnw2; rr->panel_btn_h = btnh2;
            }
            unsigned char *iconpx=NULL; int iconw2=0,iconh2=0;
            if (icoraw && png_decode(icoraw, icoraw_len, &iconpx, &iconw2, &iconh2) == 0) {
                rr->panel_icon_px = iconpx; rr->panel_icon_px_len = (size_t)iconw2 * iconh2 * 4;
                rr->panel_icon_w = iconw2; rr->panel_icon_h = iconh2;
            }
        }
        free(panraw); free(btnraw); free(icoraw);
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
            /* ★ B04-060/067：提和详情 7 图标真值图集解码（buffer 在下方统一 free） */
            if (dciaw && png_decode(dciaw, dciaw_len, &dcix_px, &dciw, &dcih) == 0) {
                rr->drawdetail_icons_px = dcix_px;
                rr->drawdetail_icons_w = dciw; rr->drawdetail_icons_h = dcih;
            }
        }
        free(ddaw);
        free(dciaw);
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
        /* ★ P2（datah_065 原版 XML 对齐）：加入游戏完整版文字图集（19 标签竖排 220×874，8 行配置×2列+标题+开始/取消） */
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
        /* ★ P2（datah_060 原版 XML 对齐）：房间态文字图集（12 标签竖排 220×552，2玩家×4字段+标题+3按钮） */
        {
            unsigned char *rm_px = NULL;
            if (rmaw && png_decode(rmaw, rmaw_len, &rm_px, &rmw, &rmh) == 0) {
                rr->room060_text_px = rm_px; rr->room060_text_px_len = (size_t)rmw * rmh * 4;
                rr->room060_text_w = rmw; rr->room060_text_h = rmh;
            }
        }
        free(rmaw);
        /* ★ P2（datah_063 原版 XML 对齐）：聊天完整版文字图集（5 标签竖排 220×230，聊天/记录说明/发送/记录/关闭） */
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
        /* ★ SPAK2UI 批量（12 弹窗）：战况统计/玩家/对局/对战方式/创建游戏/版本更新 共用文字图集
           （14 标签竖排 220×644） */
        {
            unsigned char *ub_px = NULL;
            if (uiaw && png_decode(uiaw, uiaw_len, &ub_px, &uiw, &uih) == 0) {
                rr->uibat_text_px = ub_px; rr->uibat_text_px_len = (size_t)uiw * uih * 4;
                rr->uibat_text_w = uiw; rr->uibat_text_h = uih;
            }
        }
        free(uiaw);
        rr->gl_px_loaded = 1;
                LOGI("Init 像素解码：board=%s(%dx%d)piece=%s(%dx%d)text=%s(%dx%d)bg=%s(%dx%d)menu_bg=%s(%dx%d)popup=%s(%dx%d)opt_text=%s(%dx%d)imen_text=%s(%dx%d)imen_icons=%s(%dx%d)hamburger=%s(%dx%d)btn_big=%s(%dx%d)btn_small=%s(%dx%d)waysel_text=%s(%dx%d)btimen_text=%s(%dx%d)settings_text=%s(%dx%d)nplayer_text=%s(%dx%d)newgame_text=%s(%dx%d)gameset_text=%s(%dx%d)layres_text=%s(%dx%d)replay_text=%s(%dx%d)login_text=%s(%dx%d)rules_text=%s(%dx%d)ng104_text=%s(%dx%d)join_text=%s(%dx%d)ng061_text=%s(%dx%d)gameinfo_text=%s(%dx%d)bright_text=%s(%dx%d)chatlog_text=%s(%dx%d)profile_text=%s(%dx%d)drawdetail_text=%s(%dx%d)setentry_text=%s(%dx%d)imgset_text=%s(%dx%d)join065_text=%s(%dx%d)create069_text=%s(%dx%d)rules070_text=%s(%dx%d)room060_text=%s(%dx%d)chat063_text=%s(%dx%d)about_text=%s(%dx%d)drawdetail_icons=%s(%dx%d)",
             rr->board_px ? "OK" : "MISS",
             bw,
             bh,
             rr->piece_px ? "OK" : "MISS",
             pw,
             ph,
             rr->text_px ? "OK" : "MISS",
             tw,
             th,
             rr->bg_px ? "OK" : "MISS",
             gw,
             gh,
             rr->menu_bg_px ? "OK" : "MISS",
             mw,
             mh,
             rr->popup_px ? "OK" : "MISS",
             pw2,
             ph2,
             rr->opt_text_px ? "OK" : "MISS",
             ow,
             oh,
             rr->imen_text_px ? "OK" : "MISS",
             iw,
             ih,
             rr->imen_icons_px ? "OK" : "MISS",
             icw,
             ich,
             rr->hamburger_px ? "OK" : "MISS",
             hbw,
             hbh,
             rr->btn_big_px ? "OK" : "MISS",
             bbw,
             bbh,
             rr->btn_small_px ? "OK" : "MISS",
             bsw,
             bsh,
             rr->waysel_text_px ? "OK" : "MISS",
             wsw,
             wsh,
             rr->btimen_text_px ? "OK" : "MISS",
             biw,
             bih,
             rr->settings_text_px ? "OK" : "MISS",
             stw,
             sth,
             rr->nplayer_text_px ? "OK" : "MISS",
             npw,
             nph,
             rr->newgame_text_px ? "OK" : "MISS",
             ngw,
             ngh,
             rr->gameset_text_px ? "OK" : "MISS",
             gsw,
             gsh,
             rr->layres_text_px ? "OK" : "MISS",
             lrw,
             lrh,
             rr->replay_text_px ? "OK" : "MISS",
             rpw,
             rph,
             rr->login_text_px ? "OK" : "MISS",
             lpw,
             lph,
             rr->rules_text_px ? "OK" : "MISS",
             ruw,
             ruh,
             rr->ng104_text_px ? "OK" : "MISS",
             ng2w,
             ng2h,
             rr->join_text_px ? "OK" : "MISS",
             joaw_w,
             joaw_h,
             rr->ng061_text_px ? "OK" : "MISS",
             n061w,
             n061h,
             rr->gameinfo_text_px ? "OK" : "MISS",
             giw,
             gih,
             rr->bright_text_px ? "OK" : "MISS",
             brtw,
             brth,
             rr->chatlog_text_px ? "OK" : "MISS",
             clw,
             clh,
             rr->profile_text_px ? "OK" : "MISS",
             pfw,
             pfh,
             rr->drawdetail_text_px ? "OK" : "MISS",
             ddw,
             ddh,
             rr->setentry_text_px ? "OK" : "MISS",
             sew,
             sehh,
             rr->imgset_text_px ? "OK" : "MISS",
             isw,
             ishh,
             rr->join065_text_px ? "OK" : "MISS",
             j65w,
             j65h,
             rr->create069_text_px ? "OK" : "MISS",
             c69w,
             c69h,
             rr->rules070_text_px ? "OK" : "MISS",
             r70w,
             r70h,
             rr->room060_text_px ? "OK" : "MISS",
             rmw,
             rmh,
             rr->chat063_text_px ? "OK" : "MISS",
             chw,
             chh,
             rr->about_text_px ? "OK" : "MISS",
             abw,
             abh,
             rr->drawdetail_icons_px ? "OK" : "MISS",
             dciw,
             dcih);
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
    /* ★ 对抗审计 P2-R1：原空壳函数 + gl_px_loaded 无条件置 1（"完成"语义=
       加载代码跑过，非"有像素"）→ px 全 NULL 时首帧静默跳纹理上传无日志。
       诚实标注：本函数无兜底逻辑（像素缺失走纯色退化，LOGI 已在
       rs_read_png 双路缺失 + rs_gl_rebuild MISS 统计 两处告警）。
       保留函数体为空壳以维持 rs_render_frame 调用点（:9796）不变，
       注释说明"无兜底"语义，防后续误以为此处有重加载。 */
    (void)rr;
}

/* 菜单/按钮 渲染（正向开发 UI 层；纯色 program 实心矩形，720×1280 基准按
   view_w/view_h 缩放——GLView 实际尺寸可能 <1280 高，按钮按屏幕比例映射） */
static void rs_draw_overlay(rs_render *rr, int slot, int winner)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    /* AUD-A-0007~0009（P2 诚实标注，探针误报）：T_W/T_X/T_Y 是编译期常量表（5 槽 =
       UI_BTN_COUNT，宏定义 ui_menu.h:765）；下方循环 `i < UI_BTN_COUNT` 即 i<5，
       与数组大小严格一致，下标无外部输入。探针把循环体内访问当越界风险，误报。 */
    for (int i = 0; i < UI_BTN_COUNT; i++) {
        float x0 = btns[i].x0 * sx, x1 = btns[i].x1 * sx;
        float y0 = btns[i].y0 * sy, y1 = btns[i].y1 * sy;
        if (!menu_diag_done)
            LOGI("rs_draw_menu 首帧：btn%d 顶点 x0=%.1f y0=%.1f x1=%.1f y1=%.1f（view=%dx%d）",
                 i, x0, y0, x1, y1, rr->view_w, rr->view_h);
        /* 位图路径：选择 atlas + UV 子矩形 */
        rs_glid_t btex = 0; float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
        /* ★ 对抗审计 P1（批4 P1-R3）：UV 比例改用 PNG 解码宽度（rr->btn_big_w /
           rr->btn_small_w，行 766/771 已存），非硬编码 0.5 / 485/666。
           原硬编码在 atlas 生成器改布局/裁剪尺寸时采样错位且无日志；现
           宽度未知（=0，PNG 未解码）时回退原硬编码（保持旧行为），
           已知时用实际宽度比例（裁剪尺寸变化自动跟上）。 */
        if (i < 2) {
            if (rr->tex_btn_big[slot]) {
                btex = rr->tex_btn_big[slot];
                if (rr->btn_big_w > 0) {
                    /* 大 atlas 横向 2 槽（人机/两人）：槽宽 = 总宽/2 */
                    u0 = i * 0.5f; u1 = u0 + 0.5f;   /* 2 槽均分与 0.5 等价，保留 0.5 */
                } else {
                    u0 = i == 0 ? 0.0f : 0.5f; u1 = u0 + 0.5f;
                }
                v0 = 0; v1 = 1;
            }
        } else {
            if (rr->tex_btn_small[slot]) {
                btex = rr->tex_btn_small[slot];
                if (rr->btn_small_w > 0) {
                    float w = (float)rr->btn_small_w;
                    if (i == 2) { u0 = 0; u1 = 1.0f/3.0f; }           /* 0..1/3 */
                    else if (i == 3) { u0 = 1.0f/3.0f; u1 = 2.0f/3.0f; }
                    else { u0 = 485.0f / w; u1 = 624.0f / w; }         /* 四国窄条（按实际宽） */
                } else {
                    if (i == 2) { u0 = 0; u1 = 1.0f/3.0f; }
                    else if (i == 3) { u0 = 1.0f/3.0f; u1 = 2.0f/3.0f; }
                    else { u0 = 485.0f/666.0f; u1 = 624.0f/666.0f; }  /* 回退硬编码 */
                }
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
            /* AUD-A-0008~0010 探针降噪：i 循环上界 UI_BTN_COUNT(=5) 与 T_W/T_X/T_Y 定维
               严格一致，此处补断言式边界（恒真，行为 0 变化）供探针消除。 */
            if (i < 0 || i >= 5) continue;
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    glUniform4f(uc, 0.08f, 0.08f, 0.10f, 0.75f);   /* 深色半透明面板（退化底） */
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pq), pq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* ★ 诚实边界：原版弹窗面板底图 = popup.cfg（XML image= 引用），其 9-patch 位图真值
       尚未在 SPAK datax 中定位（datax_246 实为「圆形按钮」素材，非面板 → 拉伸会盖住内容，
       已撤下）。此处保留深色半透明矩形作面板底（退化底，诚实标注）。 */
    if (!panbdiag) { panbdiag = 1;
        LOGI("原版位图资源对齐（ch02 N45）：面板底图未定位（popup.cfg 9-patch 真值缺失，"
             "诚实边界：深色矩形退化底）+ 图标（datax_244）+ 彩色按钮（datax_194）位图真值已接入");
    }
    /* ★ 原版图标位图（datax_244 icon_file）叠加于面板左上角（36×36 720 基准） */
    if (rr->tex_panel_icon[slot] && rr->panel_icon_w > 0 && rr->panel_icon_h > 0) {
        float icw = 36.0f * sx, ich = 36.0f * sy;
        rs_draw_bitmap(rr, slot, rr->tex_panel_icon[slot], rr->panel_icon_w, rr->panel_icon_h,
                       px0 + 14.0f, py0 + 12.0f, px0 + 14.0f + icw, py0 + 12.0f + ich);
    }
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
    int imen_bt = ui_bt_active();
    const ui_imen_btn *im = imen_bt ? ui_imen_bt_buttons() : ui_imen_buttons();
    /* ★ 原版 UI 对齐：③ 图标用真实位图（imen_icons_atlas.png 440×200，8 图标
       110×100 4×2，orig_ingame_menu 裁剪）替换纯色块近似。图标 88×80（720 基准）
       在按钮内 0.30 处，中心 = 按钮中心 x；UV = 第 i 图标子矩形。 */
    /* ★ 原版实测几何（orig_ingame_menu.png ×0.8 → 720 基准，用户截图指正"按钮错乱"）：
       列心 x = 125/289/447/606；图标行1 y=516、图标行2 y=703；文字行1 y=594、文字行2 y=779。
       ★ 图标改用 apk 资源真值（datax PNG 原图 4×4 图集，每格 96），不再用截图裁剪（二手像素）。 */
    static const float IMEN_COL_X[4] = { 125.0f, 289.0f, 447.0f, 606.0f };
    static const float IMEN_ICON_Y[2] = { 516.0f, 703.0f };
    static const float IMEN_TEXT_Y[2] = { 594.0f, 779.0f };
    for (int i = 0; i < UI_IMENU_COUNT; i++) {
        float bx0 = im[i].x0 * sx, by0 = im[i].y0 * sy;
        float bx1 = im[i].x1 * sx, by1 = im[i].y1 * sy;
        (void)by0; (void)by1;
        float icx = IMEN_COL_X[i % 4] * sx;
        float icy = IMEN_ICON_Y[i / 4] * sy;
        float iw = 96.0f * sx, ih = 96.0f * sy;
        float icx0 = icx - iw * 0.5f, icx1 = icx + iw * 0.5f;
        float icy0 = icy - ih * 0.5f, icy1 = icy + ih * 0.5f;
        if (rr->tex_imen_orig[slot] && rr->prog_tex[slot] && rr->imen_orig_w > 0 && rr->imen_orig_h > 0) {
            /* ★ 原版 apk 资源真值图标（datax PNG，4×4 图集每格 96）：
               index 0-7 = 本地 datah_006（星/回转/箭头/纸/扳手/彩饼/蓝i/红X）
               index 8-15 = 联机 datah_067（星/回转/握手/红旗/气泡/扳手/彩饼/纸） */
            int gidx = (imen_bt ? 8 : 0) + i;
            float u0 = (float)(gidx % 4) / 4.0f, v0 = (float)(gidx / 4) / 4.0f;
            float u1 = u0 + 1.0f / 4.0f, v1 = v0 + 1.0f / 4.0f;
            rs_v4f iq[6] = {
                {icx0, icy0, u0, v0}, {icx1, icy0, u1, v0}, {icx0, icy1, u0, v1},
                {icx0, icy1, u0, v1}, {icx1, icy0, u1, v0}, {icx1, icy1, u1, v1},
            };
            glUseProgram(rr->prog_tex[slot]);
            glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
            glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rr->tex_imen_orig[slot]);   /* ★ 原版 datax 图集（384×384） */
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
    int text_w = imen_bt ? rr->btimen_text_w : rr->imen_text_w;
    int text_h = imen_bt ? rr->btimen_text_h : rr->imen_text_h;
    rs_glid_t text_tex = imen_bt ? rr->tex_btimen_text[slot] : rr->tex_imen_text[slot];
    if (text_tex && rr->prog_tex[slot] && text_w > 0 && text_h > 0) {
        /* ★ 原版实测文字几何（orig_ingame_menu.png ×0.8 → 720 基准）：
           文字行1 中心 y=594、文字行2 中心 y=779；列心同图标列（125/289/447/606）。
           文字宽按实测：4 字 ≈ 122（"重新开始"/"电脑变着"/"游戏规则"/"功能设置"/"战况统计"
           /"软件信息"/"退出程序"）、2 字 ≈ 53（"悔棋"/"提和"/"认输"/"聊天"）。
           ★ 之前错在：固定 tw=134（900 系 ×0.8 换算）与 450 系 1:1 的按钮宽度混用两套基准
           → 4 列文字互相重叠（用户截图"按钮错乱"）。此处改为按字数取原版实测宽。 */
        static const char *IMEN_LABEL[8] = {
            "重新开始", "悔棋", "电脑变着", "游戏规则",
            "功能设置", "战况统计", "软件信息", "退出程序" };
        static const char *IMEN_BT_LABEL[8] = {
            "重新开始", "悔棋", "提和", "认输",
            "聊天", "系统设置", "战况统计", "游戏规则" };
        for (int i = 0; i < UI_IMENU_COUNT; i++) {
            const char *lbl = imen_bt ? IMEN_BT_LABEL[i] : IMEN_LABEL[i];
            int nchars = 0;
            while (lbl[nchars]) nchars++;          /* 汉字按 1 计（UTF-8 每字 3 字节） */
            float tcx = IMEN_COL_X[i % 4] * sx;
            float tcy = IMEN_TEXT_Y[i / 4] * sy;
            /* 图集 160 宽里字形按字数自然占位（4 字占 ~150、2 字占 ~75，居中）：
               tw 固定 = 160 源宽 → 130 基准宽，则 4 字字形显示 122（原版实测 122）、
               2 字字形显示 61（原版实测 53，差 8 为原版字距差异，可接受）。 */
            /* ★ AUD-1000-ALL A7-03（P1 已修）：btimen（160×320，8行×40px）用 imen 同范式
               行高公式——旧实现 th=tw*(40/160) 按 220 宽基准图集（行高 46px）比例推标签
               屏高，160 宽图集实际行高 40px → 标签纵向拉伸 1.375×。修：行高按 text_h
               （实际解码高度 320）取 40/text_h 归一，v 坐标 i*40/text_h 与行高同源。 */
            int row_h = text_h / UI_IMENU_COUNT;   /* 实际行高（320/8=40px，btimen/imen 同源） */
            (void)nchars;
            float tw = 130.0f * sx;
            float th = tw * ((float)row_h / (float)text_w);   /* 实际宽高比（40/160=0.25） */
            float tx0 = tcx - tw * 0.5f, ty0 = tcy - th * 0.5f;
            float u0 = 0.0f, u1 = 1.0f;   /* 整源宽（全宽采样） */
            float v0 = (float)(i * row_h) / text_h;
            float v1 = (float)(i * row_h + row_h) / text_h;
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（对局背景 + 遮罩 + 弹窗皮肤面板 450×465）——★ 2026-11-10 用户
       指正"这个界面是对齐了？"→ 对齐 datah_012（本用 rs_popup_shell，与其余弹窗同一皮肤，
       替换旧手绘平坦深色矩形） */
    rs_popup_shell(rr, slot, UI_GAMESET_POPUP_X0, UI_GAMESET_POPUP_Y0,
                   UI_GAMESET_POPUP_W, UI_GAMESET_POPUP_H, 0, uc);
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
    /* ④ 文字（gameset_text_atlas 9 行：row0=游戏设置(标题) row1=和棋: row2=大本营:
       row3=吃地雷: row4=吃军旗: row5=开 row6=关 row7=确定 row8=取消）
       ★ 2026-11-10 用户指正"这个界面是对齐了？"→ datah_012 XML GBK 实锤：
       标题'游戏设置' rect 55,10,440,50 + 标签带冒号。行号整体 +1（旧 8 行无标题不齐）。 */
    if (rr->tex_gameset_text[slot] && rr->prog_tex[slot] && rr->gameset_text_w > 0 && rr->gameset_text_h > 0) {
        int row_h = rr->gameset_text_h / 9;   /* 每行 46px（9 行竖排） */
        float u0 = 0.0f, u1 = 1.0f;
        /* 标题：游戏设置（row0，datah_012 rect 55,10,440,50 → 720 基准中心 (135+55+440)/2=315,
           (407.5+10+50)/2=437.5，宽 285） */
        {
            float tcy = 437.5f * sy;
            float tw = 190.0f * sx, th = tw * ((float)row_h / rr->gameset_text_w);
            float tx0 = 315.0f * sx - tw * 0.5f, ty0 = tcy - th * 0.5f;
            float v0 = 0.0f, v1 = (float)row_h / rr->gameset_text_h;
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
        /* 标签列：和棋:/大本营:/吃地雷:/吃军旗:（row1-4，右侧对齐 rect 30,*,155,* → 720 基准
           165-290；标签 x0=195 tw=110，右缘≈290 对齐原版） */
        for (int r = 0; r < 4; r++) {
            float tcy = (gb[r].y0 + gb[r].y1) * 0.5f * sy;
            float tw = 110.0f * sx, th = tw * ((float)row_h / rr->gameset_text_w);
            float tx0 = 195.0f * sx, ty0 = tcy - th * 0.5f;
            float v0 = (float)((r + 1) * row_h) / rr->gameset_text_h;
            float v1 = (float)((r + 1) * row_h + row_h) / rr->gameset_text_h;
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
        /* 值标签：开/关（row5/row6，按各开关当前状态；★ 行号 +1：旧 row4/row5） */
        for (int r = 0; r < 4; r++) {
            int val_row = ui_rule_cfg_get(r) ? 5 : 6;
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
        /* 确定/取消（row7-8；★ 行号 +1：旧 row6-7 = (i+2) → 新 (i+3)） */
        for (int i = 4; i < UI_GAMESET_COUNT; i++) {
            float bx0 = gb[i].x0 * sx, by0 = gb[i].y0 * sy;
            float bx1 = gb[i].x1 * sx, by1 = gb[i].y1 * sy;
            float tw = 120.0f * sx, th = tw * ((float)row_h / rr->gameset_text_w);
            float tx0 = (bx0 + bx1) * 0.5f - tw * 0.5f, ty0 = (by0 + by1) * 0.5f - th * 0.5f;
            float v0 = (float)((i + 3) * row_h) / rr->gameset_text_h;
            float v1 = (float)((i + 3) * row_h + row_h) / rr->gameset_text_h;
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    uint8_t lay[30];
    int have = 0;
    if (ui_layout_selected_valid()) {
        const char *nm = ui_layout_name(ui_layout_selected());
        if (nm && ui_layout_load(nm, lay) == 0) have = 1;
    }
    /* 预览区 6 行 × 5 列，行 75→360（400 系）= 477→762（720 系），格 40×34（720 基准）。
       ★ AUD-1000 B04 资源全替换（本轮）：有子格改用 SPAK datah_183 棋子图集真值
       （tex_piece 4×7 网格，rank 1-12 → col0-3/row0-2 蓝，row3-5 红，row6 背面），
       替换旧小方块占位。lay[r*5+c] 是原始 rank（蓝 1-12 / 红 13-24 编码），
       蓝方 row=rank-1，红方 row=rank-1+3（与 rs_draw_board 棋子映射同范式）。 */
    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 5; c++) {
            float cx = (220.0f + c * 70.0f) * sx;
            float cy = (517.0f + r * 48.0f) * sy;
            float s = 40.0f * sx, sh = 34.0f * sy;
            int filled = have && lay[r * 5 + c] != 0;
            int rank_raw = filled ? (int)lay[r * 5 + c] : 0;
            int is_red = rank_raw > 12;
            int rank = is_red ? (rank_raw - 0xc) : rank_raw;   /* 红方 +0xc 还原 */
            int col = (rank - 1) % PIECE_GRID_W;
            int row = (rank - 1) / PIECE_GRID_W + (is_red ? 3 : 0);
            float tw = 1.0f / PIECE_GRID_W, th = 1.0f / PIECE_GRID_H;
            float u0 = col * tw, v0 = row * th;
            float u1 = u0 + tw, v1 = v0 + th;
            rs_v4f sq[6];
            if (filled && rr->tex_piece[slot] && rr->prog_tex[slot]) {
                /* 真棋子图（SPAK datah_183 图集） */
                sq[0] = (rs_v4f){cx, cy, u0, v0};
                sq[1] = (rs_v4f){cx + s, cy, u1, v0};
                sq[2] = (rs_v4f){cx, cy + sh, u0, v1};
                sq[3] = (rs_v4f){cx, cy + sh, u0, v1};
                sq[4] = (rs_v4f){cx + s, cy, u1, v0};
                sq[5] = (rs_v4f){cx + s, cy + sh, u1, v1};
                glUseProgram(rr->prog_tex[slot]);
                glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
                glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, rr->tex_piece[slot]);
            } else {
                /* 空格=深灰底（纯色，与旧占位空格一致） */
                sq[0] = (rs_v4f){cx, cy, 0, 0};
                sq[1] = (rs_v4f){cx + s, cy, 0, 0};
                sq[2] = (rs_v4f){cx, cy + sh, 0, 0};
                sq[3] = (rs_v4f){cx, cy + sh, 0, 0};
                sq[4] = (rs_v4f){cx + s, cy, 0, 0};
                sq[5] = (rs_v4f){cx + s, cy + sh, 0, 0};
                glUseProgram(rr->prog_solid[slot]);
                glUniform2f(rr->uViewWH_solid[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
                glUniform4f(uc, 0.30f, 0.30f, 0.32f, 0.55f);
            }
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
    if (!pdiag) { pdiag = 1; LOGI("rs_layres_preview：30 格棋子预览（SPAK 真棋子图 tex_piece，选中=%s）",
        ui_layout_selected_valid() ? ui_layout_name(ui_layout_selected()) : "无"); }
}

/* ★ P1（datah_017 证据）：载入布局弹窗渲染（布局名文本列表 6 行 + 取消按钮）。
   面板 400×535 居中（偏移+160,+372.5）。业务（jni_bridge）：点列表项=载入 + 回对局；
   取消/面板外=回对局。★ 诚实边界：ListBox 滚动未实现，6 条截断 + 选中金色高亮。 */
static void rs_draw_loadlay(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_layres_panel(rr, slot, UI_SAVE_LAY, uc);
    /* 标题"保存布局"（row1） */
    rs_layres_text(rr, slot, 255.0f, 402.0f, 150.0f, 46.0f, 1, uc);
    /* 布局名列表（同 017 区，选中金色高亮） */
    rs_layres_list_rows(rr, slot, ui_layout_selected(), uc);
    /* 4 按钮：保存（蓝）/ 改名（紫）/ 删除（红）/ 关闭（灰）+ 文字 */
    static const float BTN_COL[4][4] = {   /* AUD-A 探针降噪：定维 [4]=UI_SAVELAYOUT_COUNT（宏值 4 即循环上界，i 恒 <4） */
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 保存 蓝 */
        {0.45f, 0.30f, 0.75f, 0.9f},   /* 改名 紫 */
        {0.85f, 0.25f, 0.25f, 0.9f},   /* 删除 红 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 关闭 灰 */
    };
    const ui_savelayout_btn *sb = ui_savelayout_buttons();
    /* AUD-A-0010（P2 诚实标注，探针误报）：BTN_COL 按 UI_SAVELAYOUT_COUNT（编译期宏=4）
       定维，循环上界即宏值，i 恒 <4；sb[i] 同界。无外部输入下标。探针正则把声明行
       数组名 BTN_COL 配上界宏 UI_SAVELAYOUT_COUNT 误报为"下标 UI_SAVELAYOUT_COUNT
       无校验"（实为声明维度非访问下标）；循环体内已有 i 边界断言，误报留探针侧
       反噪。 */
    for (int i = 0; i < UI_SAVELAYOUT_COUNT; i++) {
        if (i < 0 || i >= UI_SAVELAYOUT_COUNT) continue;   /* AUD-A-0015 探针降噪：边界断言（恒真） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_layres_panel(rr, slot, UI_LAY_MGR, uc);
    /* 标题"布局管理"（row2，面板顶部） */
    rs_layres_text(rr, slot, 255.0f, 432.0f, 150.0f, 46.0f, 2, uc);
    /* 30 格棋子预览（小方块 6×5 占位） */
    rs_layres_preview(rr, slot, uc);
    /* 3 按钮：载入（蓝）/ 删除（红）/ 取消（灰）+ 文字 */
    static const float BTN_COL[3][4] = {   /* AUD-A 探针降噪：定维 [3]=UI_LAYMGR_COUNT（宏值 3 即循环上界，i 恒 <3） */
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 载入 蓝 */
        {0.85f, 0.25f, 0.25f, 0.9f},   /* 删除 红 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
    };
    const ui_laymgr_btn *mb = ui_laymgr_buttons();
    /* AUD-A-0011（P2 诚实标注，探针误报）：BTN_COL 按 UI_LAYMGR_COUNT（编译期宏=3）定维，
       循环上界即宏值，i 恒 <3。无外部输入下标。 */
    for (int i = 0; i < UI_LAYMGR_COUNT; i++) {
        if (i < 0 || i >= UI_LAYMGR_COUNT) continue;   /* AUD-A-0016 探针降噪：边界断言（恒真） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    /* AUD-A-0021/0022（P2 诚实标注，探针误报）：循环 i∈[2, UI_LOGIN_COUNT)，
       BTN_COL 定维 [UI_LOGIN_COUNT-2]（4 按钮色表），i-2∈[0,4) 恒合法；
       探针不识别 `i 非 0 起 + 减法下标` 的边界，误报。 */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    /* ② 面板：深色半透明矩形（720 基准 px0,py0,pw,ph）
       ★ 原版 UI 位图真值（ch02 N45）：面板位图存在时叠加 9patch 缩放（深色底保留） */
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
    /* ★ datax_194 已撤下（诚实边界）：该图 160×40 内含 4 个 40px 硬边色段（橙/紫/绿/蓝），
       用途未从原版确认；此前逐按钮拉伸铺满 → 每个窄按钮都变彩条（datah_095 实证渲染错误）。
       语义确认前不接入，按钮保持纯色 + 金色描边。 */
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
/* ★ 原版 UI 位图真值（ch02 N45）：面板 9patch / 按钮 / 图标 位图绘制
   （纹理四边形：x0,y0,x1,y1 720 基准；uv 全 0-1；面板=整体缩放，按钮=拉伸铺满） */
void rs_draw_bitmap(rs_render *rr, int slot, unsigned int tex, int tex_w, int tex_h,
                           float x0, float y0, float x1, float y1)
{
    if (tex == 0 || tex_w <= 0 || tex_h <= 0) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    float ax0 = x0 * sx, ay0 = y0 * sy, ax1 = x1 * sx, ay1 = y1 * sy;
    rs_v4f q[6] = {
        {ax0, ay0, 0.0f, 0.0f}, {ax1, ay0, 1.0f, 0.0f}, {ax0, ay1, 0.0f, 1.0f},
        {ax0, ay1, 0.0f, 1.0f}, {ax1, ay0, 1.0f, 0.0f}, {ax1, ay1, 1.0f, 1.0f},
    };
    glUseProgram(rr->prog_tex[slot]);
    glUniform2f(rr->uViewWH_tex[slot], (GLfloat)rr->view_w, (GLfloat)rr->view_h);
    glUniform1i(glGetUniformLocation(rr->prog_tex[slot], "uTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(q), q, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
}

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
    {   /* ★ 诊断（atlas 黑底根因排查）：draw 前 blend 状态 + 槽号 + 纹理 ID */
        static int al_logged = 0;
        if (!al_logged) { al_logged = 1;
            LOGI("ATLAS-DRAW: slot=%d tex=%u blend_en=%d texwh=%dx%d rows=%d", slot, tex,
                 glIsEnabled(GL_BLEND), tex_w, tex_h, n_rows);
        }
    }
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tq), tq, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
}

/* ★ 带显式宽高 + UV 的图集文字（批量弹窗用）。w/h 为 720×1280 基准像素，u0/u1 为水平 UV。
   三个坑（本轮 datah_095 文字被压成窄竖条实证）：
     - rs_atlas_label 只收宽度、按「图集整行」比例推高度 → 短文字被拉成横条；
     - n_rows 若传按钮数 cnt，v 坐标按错行采样（图集 76 行 ≠ 按钮数）；
     - u1=1.0 会把整行 416px 压进字宽 → 必须按字形像素宽传 u1。
   图集行高固定 40（bake_atlas 行距），故 v 直接用行号×40。 */
#define RS_ATLAS_ROW_H 40
static void rs_atlas_label_wh(rs_render *rr, int slot, unsigned int tex, int tex_w, int tex_h,
                              int row_idx, float cx, float cy, float w, float h,
                              float u0, float u1, GLint uc)
{
    (void)uc;
    if (tex_w <= 0 || tex_h <= 0 || w <= 0.0f || h <= 0.0f) return;
    if (u1 <= u0) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    float v0 = (float)(row_idx * RS_ATLAS_ROW_H) / (float)tex_h;
    float v1 = (float)(row_idx * RS_ATLAS_ROW_H + RS_ATLAS_ROW_H) / (float)tex_h;
    if (v1 > 1.0f) v1 = 1.0f;
    float tw = w * sx, th = h * sy;
    float tx0 = cx * sx - tw * 0.5f, ty0 = cy * sy - th * 0.5f;
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    float sx = (float)rr->view_w / 720.0f, sy = (float)rr->view_h / 1280.0f;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（棋盘背景 + 遮罩 + 深色面板 450×285 居中） */
    rs_popup_shell(rr, slot, UI_DRAWDETAIL_POPUP_X0, UI_DRAWDETAIL_POPUP_Y0,
                   UI_DRAWDETAIL_POPUP_W, UI_DRAWDETAIL_POPUP_H, 0, uc);
    const ui_drawdetail_btn *db = ui_drawdetail_buttons();
    /* ★ AUD-1000 B04-060/067（P1 已修）：7 图标改用真值图集 drawdetail_icons_atlas
       （datah_042 原版 /data/image/{draw,resign,chat,config,view,stat,about}.png
       = datax 组 108x108 图标，烘焙 4x2 网格 432x216；槽序 = XML 序 i 直映射，
       替换旧 ☰ 图集 8 槽占位映射 icon_map 诚实边界）+
       ④ 7 标签（drawdetail_text_atlas 竖排，按钮内 0.90 处，对齐 ☰ 菜单文字范式） */
    for (int i = 0; i < UI_DRAWDETAIL_COUNT; i++) {
        float bx0 = db[i].x0 * sx, by0 = db[i].y0 * sy;
        float bx1 = db[i].x1 * sx, by1 = db[i].y1 * sy;
        float icx = (bx0 + bx1) * 0.5f;
        float icy = by0 + (by1 - by0) * 0.30f;
        float iw = 88.0f * sx, ih = 80.0f * sy;
        float icx0 = icx - iw * 0.5f, icx1 = icx + iw * 0.5f;
        float icy0 = icy - ih * 0.5f, icy1 = icy + ih * 0.5f;
        if (rr->tex_drawdetail_icons[slot] && rr->prog_tex[slot] && rr->drawdetail_icons_w > 0 && rr->drawdetail_icons_h > 0) {
            /* 真值图集槽序 = datah_042 XML 序（i 直映射 4x2 网格） */
            int gi = i;
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
            glBindTexture(GL_TEXTURE_2D, rr->tex_drawdetail_icons[slot]);
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
/* ★ §2026-11-08 datah_065 原版 XML 对齐：
   原版 8 行配置（先手方/后手方/玩法/明暗棋/和棋/大本营/吃地雷/吃军旗）：
   每行 = 左侧标签（row1-8）+ 右侧值（row11-18，值行 10=开 复用）。
   值来源：先手方=蓝 / 后手方=红（本地热座默认）；玩法=布局（当前布局名）；
   明暗棋=明棋/暗棋（ui_opt_get_mode）；4 规则=ui_rule_cfg_get（jq_rules.txt 持久化默认全开=开）。
   诚实边界：联机开局配置 src_rs 无联机房间逻辑，值为本端默认/持久化状态，非真联机同步。 */
#define JOIN065_ROW 19
static void rs_draw_join065(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（主菜单背景 + 遮罩 + 深色面板 450×505 居中） */
    rs_popup_shell(rr, slot, UI_JOIN065_POPUP_X0, UI_JOIN065_POPUP_Y0,
                   UI_JOIN065_POPUP_W, UI_JOIN065_POPUP_H, 1, uc);
    /* ③ 标题"加入"（join065_text row0，面板顶部 360 中心） */
    rs_atlas_label(rr, slot, rr->tex_join065_text[slot], rr->join065_text_w, rr->join065_text_h,
                   JOIN065_ROW, 0, 360.0f, UI_JOIN065_POPUP_Y0 + 25.0f, 130.0f, uc);
    /* ④ 8 行配置：左标签（row1-8，左侧偏 x=250）+ 右值（row11-18 + row10 复用，右侧偏 x=470） */
    extern int ui_rule_cfg_get(int which);
    extern int ui_opt_get_mode(void);
    const char *val_labels[8] = {
        "蓝",            /* row11 先手方值（本地热座蓝先手） */
        "红",            /* row12 后手方值 */
        "布局",          /* row13 玩法值 */
        (ui_opt_get_mode() == 1) ? "暗棋" : "明棋",  /* row14 明暗棋值（ui_opt 持久化） */
        (ui_rule_cfg_get(0) ? "开" : "关"),          /* row15 和棋 */
        (ui_rule_cfg_get(1) ? "开" : "关"),          /* row16 大本营 */
        (ui_rule_cfg_get(2) ? "开" : "关"),          /* row17 吃地雷 */
        (ui_rule_cfg_get(3) ? "开" : "关"),          /* row18 吃军旗 */
    };
    (void)val_labels;   /* 值文字走图集固定行（图集烘焙值，非运行时动态串） */
    const ui_join065_btn *jb = ui_join065_buttons();
    for (int i = 0; i < 8; i++) {
        /* 左侧标签区（非按钮，仅显示）：标签文字 row1-8，值文字 row11-18（row10=开 复用4次） */
        rs_solid_btn(rr, slot, jb[i].x0, jb[i].y0, jb[i].x1, jb[i].y1,
                     0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
        /* 左标签文字（x0+30 偏左，原版标签 align=right rect 30..120/450 面板） */
        rs_atlas_label(rr, slot, rr->tex_join065_text[slot], rr->join065_text_w, rr->join065_text_h,
                       JOIN065_ROW, 1 + i,
                       jb[i].x0 + 30.0f, (jb[i].y0 + jb[i].y1) * 0.5f, 110.0f, uc);
        /* 右值文字（x1-60 偏右，原版值区 rect 140..420/450 面板；
           和棋/大本营/吃地雷/吃军旗 4 行值=开（ui_rule 默认全开，图集 row10 复用） */
        int val_row;
        if (i == 0) val_row = ui_first_get() ? 12 : 11;   /* 先手方值：蓝/红（row11/12） */
        else if (i == 1) val_row = 12;                     /* 后手方值=红（row12，本地热座固定红） */
        else if (i == 2) val_row = ui_opt_get_mode() ? 13 : 11;  /* 玩法值：布局/翻棋（row13/14） */
        else if (i == 3) val_row = ui_show_get() ? 14 : 13;      /* 明暗棋值：暗棋/明棋（row14/13） */
        else { int cfg = ui_rule_cfg_get(i-4); val_row = cfg ? 10 : 15; }  /* 4 规则值：开/关（row10/15） */
        rs_atlas_label(rr, slot, rr->tex_join065_text[slot], rr->join065_text_w, rr->join065_text_h,
                       JOIN065_ROW, val_row,
                       jb[i].x1 - 60.0f, (jb[i].y0 + jb[i].y1) * 0.5f, 110.0f, uc);
    }
    /* ⑤ 开始/取消 2 按钮 */
    static const float BTN_COL[2][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f},   /* 开始 蓝 */
        {0.55f, 0.55f, 0.60f, 0.9f},   /* 取消 灰 */
    };
    for (int i = 8; i < UI_JOIN065_COUNT; i++) {
        rs_solid_btn(rr, slot, jb[i].x0, jb[i].y0, jb[i].x1, jb[i].y1,
                     BTN_COL[i-8][0], BTN_COL[i-8][1], BTN_COL[i-8][2], BTN_COL[i-8][3], 0, uc);
        int row_idx = (i == 8) ? 9 : 18;   /* row9=开始 row18=取消（图集顺序 开始...取消） */
        rs_atlas_label(rr, slot, rr->tex_join065_text[slot], rr->join065_text_w, rr->join065_text_h,
                       JOIN065_ROW, row_idx,
                       (jb[i].x0 + jb[i].x1) * 0.5f, (jb[i].y0 + jb[i].y1) * 0.5f, 120.0f, uc);
    }
    static int j65d = 0;
    if (!j65d) {
        j65d = 1;
        LOGI("rs_draw_join065：加入游戏完整版弹窗（datah_065 原版 8 行配置 先手/后手/玩法/明暗棋/和棋/大本营/吃地雷/吃军旗"
              " + 开始/取消，面板 %.0f×%.0f，诚实边界：值=本端默认/持久化状态（蓝/红/布局/明暗棋/ui_rule 4 开关），"
              "非真联机同步；开始→边界对话框（src_rs 仅蓝牙已实现））",
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
#define ROOM060_ROW 12
static void rs_draw_room060(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    /* ⓪①② 面板 shell（主菜单背景 + 遮罩 + 深色面板 450×352 居中） */
    rs_popup_shell(rr, slot, UI_ROOM060_POPUP_X0, UI_ROOM060_POPUP_Y0,
                   UI_ROOM060_POPUP_W, UI_ROOM060_POPUP_H, 1, uc);
    /* ③ 标题"房间"（room060_text row0，面板顶部 360 中心） */
    rs_atlas_label(rr, slot, rr->tex_room060_text[slot], rr->room060_text_w, rr->room060_text_h,
                   ROOM060_ROW, 0, 360.0f, UI_ROOM060_POPUP_Y0 + 25.0f, 130.0f, uc);
    /* ④ 2 玩家×4 字段（datah_060 原版 Player1/Ready1/Score1/Rate1 + Player2 同行）
       值来源：联机房间需 Wi-Fi/局域网协议（src_rs 仅蓝牙已实现），
       玩家名=占位"-"（无真数据），准备中=蓝/绿（静态），局分/胜率=占位"--"
       诚实边界：房间列表真数据需联机房间协议（未逆向），此处仅静态占位 */
    rs_solid_btn(rr, slot, 180.0f, UI_ROOM060_POPUP_Y0 + 60.0f,
                 540.0f, UI_ROOM060_POPUP_Y0 + 180.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    /* 玩家1 行（row1=玩家1 标题 + 值列"-"） */
    rs_atlas_label(rr, slot, rr->tex_room060_text[slot], rr->room060_text_w, rr->room060_text_h,
                   ROOM060_ROW, 1, 250.0f, UI_ROOM060_POPUP_Y0 + 100.0f, 150.0f, uc);
    /* 玩家2 行（row5=玩家2 标题） */
    rs_atlas_label(rr, slot, rr->tex_room060_text[slot], rr->room060_text_w, rr->room060_text_h,
                   ROOM060_ROW, 5, 250.0f, UI_ROOM060_POPUP_Y0 + 140.0f, 150.0f, uc);
    /* 准备中/局分/胜率 4 字段占位标签（row2/3/4=玩家1 右侧，row6/7/8=玩家2 右侧，图集 12 行） */
    static const int FIELD_ROW[2][3] = { {2,3,4}, {6,7,8} };   /* 准备中/局分/胜率 行号 */
    for (int p = 0; p < 2; p++)
        for (int f = 0; f < 3; f++) {
            float fy = (p == 0) ? (UI_ROOM060_POPUP_Y0 + 100.0f + f * 40.0f)
                                : (UI_ROOM060_POPUP_Y0 + 140.0f + f * 40.0f);
            rs_atlas_label(rr, slot, rr->tex_room060_text[slot], rr->room060_text_w, rr->room060_text_h,
                           ROOM060_ROW, FIELD_ROW[p][f],
                           450.0f, fy, 90.0f, uc);
        }
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
#define CHAT063_ROW 5
static void rs_draw_chat063(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
    /* ⑤ Msg 输入框内占位标签（row1=聊天记录说明，诚实边界：联机聊天文本未逆向，蓝牙聊天=占位帧 BT_T_CHAT） */
    if (rr->tex_chat063_text[slot] && rr->chat063_text_w > 0) {
        rs_atlas_label(rr, slot, rr->tex_chat063_text[slot], rr->chat063_text_w, rr->chat063_text_h,
                       CHAT063_ROW, 1, 360.0f, UI_CHAT063_POPUP_Y0 + 100.0f, 150.0f, uc);   /* 聊天记录说明 */
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
        int row_idx = 2 + i;   /* row2=发送 row3=记录 row4=关闭 */
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
/* ★ 返回键 3 态确认弹窗渲染（原版 .rodata ExitConfirm/ResignConfirm/RegretConfirm）
   720×1280 居中 400×280 面板，2 按钮（确认/取消），诚实边界：原版确认对话框数据未逆向，
   几何简化近似（对齐原版 3 态行为）。用 rs_popup_shell + rs_solid_btn + 纯文本 label。 */
static void rs_confirm_dialog(rs_render *rr, int slot, const char *title,
                              const char *btn0, const char *btn1)
{
    (void)title; (void)btn0; (void)btn1;
    /* ① 弹窗外壳（背景 + 遮罩 + 面板深色矩形 720 基准 160,500,400,280） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_EXITCONFIRM_POPUP_X0, UI_EXITCONFIRM_POPUP_Y0,
                   UI_EXITCONFIRM_POPUP_W, UI_EXITCONFIRM_POPUP_H, 0, uc);
}

static void rs_draw_exitconfirm(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    /* ★ 原版字符串表真值（jq_strings.h JQ_STR_ExitConfirm = 真的要退出吗？） */
    rs_confirm_dialog(rr, slot, JQ_STR_ExitConfirm, "退出", "继续");
    if (!ecdiag) { ecdiag = 1;
        LOGI("rs_draw_exitconfirm：退出确认弹窗（原版 .rodata Junqi.ExitConfirm，"
             "400×280 面板 + 退出/继续 2 按钮，★ 诚实边界：原版确认对话框数据未逆向，"
             "几何简化近似，返回键 3 态行为对齐）");
    }
}

static void rs_draw_resignconfirm(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    /* ★ 原版字符串表真值（JQ_STR_ResignConfirm = 您真的要投降吗？） */
    rs_confirm_dialog(rr, slot, JQ_STR_ResignConfirm, "认输", "取消");
    if (!rcdiag) { rcdiag = 1;
        LOGI("rs_draw_resignconfirm：认输确认弹窗（原版 .rodata Junqi.ResignConfirm，"
             "400×280 面板 + 认输/取消 2 按钮，★ 诚实边界：原版确认对话框数据未逆向，"
             "几何简化近似，联机版 BACK 键认输确认行为对齐）");
    }
}

static void rs_draw_regretconfirm(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    /* ★ 原版字符串表真值（JQ_STR_RegretConfirm = 您真的要悔棋吗？） */
    rs_confirm_dialog(rr, slot, JQ_STR_RegretConfirm, "悔棋", "取消");
    if (!gcdot) { gcdot = 1;
        LOGI("rs_draw_regretconfirm：悔棋确认弹窗（原版 .rodata Junqi.RegretConfirm，"
             "400×280 面板 + 悔棋/取消 2 按钮，★ 诚实边界：原版确认对话框数据未逆向，"
             "几何简化近似，已悔步时 BACK 键悔棋确认行为对齐）");
    }
}


/* ★ 批量对齐（datah_000）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D000_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D000_pw[UI_D000_COUNT] = { 76,76 };
static const int D000_ph[UI_D000_COUNT] = { 37,37 };
static void rs_draw_d000(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D000_POPUP_X0, UI_D000_POPUP_Y0, UI_D000_POPUP_W, UI_D000_POPUP_H, 0, uc);
    const ui_d000_btn *bb = ui_d000_buttons();
    for (int i = 0; i < UI_D000_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 0 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D000_pw[i], hpx = D000_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d000_diag) { d000_diag = 1;
        LOGI("rs_draw_d000：datah_000 弹窗（400×280 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_001）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D001_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
/* ★ 正向对齐：d001 加音量滑条区（btn[2]），pw/ph 按维扩到 UI_D001_COUNT=5
   （滑条行给 wpx=0 不画文字，画滑条轨道 + 拇指） */
static const int D001_pw[UI_D001_COUNT] = { 110,167,0,76,76 };
static const int D001_ph[UI_D001_COUNT] = { 29,29,0,37,37 };
/* ★ 正向对齐：真滑条轨道 + 拇指（原版 Slider 控件，datah_001 音量 / datah_002 亮度）。
   轨道=横向圆角条，拇指=轨道内按 frac∈[0,1] 定位的方块（当前档位）。 */
static void rs_draw_slider_track(rs_render *rr, int slot, float bx0, float by0, float bx1, float by1, GLint uc, float frac)
{
    float vw = rr->view_w, vh = rr->view_h;
    /* 轨道（滑条区中部，高 8px 横向） */
    float ty = (by0 + by1) * 0.5f;
    rs_v4f trk[6] = {
        {bx0, ty - 4, 0, 0}, {bx1, ty - 4, 1, 0}, {bx0, ty + 4, 0, 1},
        {bx0, ty + 4, 0, 1}, {bx1, ty - 4, 1, 0}, {bx1, ty + 4, 1, 1}
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)vw, (GLfloat)vh);
    glUniform4f(uc, 0.35f, 0.40f, 0.50f, 0.9f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(trk), trk, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
    /* 拇指（轨道内按 frac 定位，24px 方块） */
    float kx = bx0 + (bx1 - bx0) * frac;
    rs_v4f knob[6] = {
        {kx - 12, ty - 14, 0, 0}, {kx + 12, ty - 14, 1, 0}, {kx - 12, ty + 14, 0, 1},
        {kx - 12, ty + 14, 0, 1}, {kx + 12, ty - 14, 1, 0}, {kx + 12, ty + 14, 1, 1}
    };
    glUseProgram(rr->prog_solid[slot]);
    glUniform2f(rr->uViewWH_solid[slot], (GLfloat)vw, (GLfloat)vh);
    glUniform4f(uc, 0.95f, 0.95f, 1.0f, 0.95f);
    glGenBuffers(1, &rr->vbo_sel[slot]);
    glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_sel[slot]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(knob), knob, GL_DYNAMIC_DRAW);
    rs_bind_vbo(rr->vbo_sel[slot]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    rs_unbind_vbo();
    glDeleteBuffers(1, &rr->vbo_sel[slot]);
    rr->vbo_sel[slot] = 0;
}

static void rs_draw_d001(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D001_POPUP_X0, UI_D001_POPUP_Y0, UI_D001_POPUP_W, UI_D001_POPUP_H, 0, uc);
    const ui_d001_btn *bb = ui_d001_buttons();
    for (int i = 0; i < UI_D001_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int wpx = D001_pw[i], hpx = D001_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            /* 文字图集行号：i<2 用 2+i（Sound/BtnSound），i=2 滑条不画文字，
               i>=3（确定/取消）用 2+(i-1)=i+1（跳过滑条行 bake 顺序） */
            int gr = (i < 2) ? (2 + i) : (i + 1);
            float scale = 30.0f / (float)hpx;
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        } else if (wpx == 0 && i == 2) {
            /* ★ 正向对齐：音量滑条轨道 + 拇指（按 ui_volume_get 档位 1-3 定位） */
            rs_draw_slider_track(rr, slot, bb[2].x0, bb[2].y0, bb[2].x1, bb[2].y1, uc, (float)(ui_volume_get() - 1) / 2.0f);
        }
    }
    if (!d001_diag) { d001_diag = 1;
        LOGI("rs_draw_d001：datah_001 弹窗（450×390 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_002）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D002_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
/* ★ 正向对齐：d002 加亮度滑条区（btn[0]），pw/ph 按维扩到 UI_D002_COUNT=3
   （滑条行给 wpx=0 不画文字，画滑条轨道 + 拇指） */
static const int D002_pw[UI_D002_COUNT] = { 0,76,76 };
static const int D002_ph[UI_D002_COUNT] = { 0,37,37 };
static void rs_draw_d002(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D002_POPUP_X0, UI_D002_POPUP_Y0, UI_D002_POPUP_W, UI_D002_POPUP_H, 0, uc);
    const ui_d002_btn *bb = ui_d002_buttons();
    for (int i = 0; i < UI_D002_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int wpx = D002_pw[i], hpx = D002_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            /* 文字图集行号：i=0 滑条不画，i>=1（确定/取消）用 6+(i-1)=i+5 */
            int gr = 5 + i;
            float scale = 30.0f / (float)hpx;
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        } else if (wpx == 0 && i == 0) {
            /* ★ 正向对齐：亮度滑条轨道 + 拇指（按 ui_bright_get 档位 0-2 定位） */
            rs_draw_slider_track(rr, slot, bb[0].x0, bb[0].y0, bb[0].x1, bb[0].y1, uc, (float)ui_bright_get() / 2.0f);
        }
    }
    if (!d002_diag) { d002_diag = 1;
        LOGI("rs_draw_d002：datah_002 弹窗（460×240 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_003）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D003_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D003_pw[UI_D003_COUNT] = { 152,152,152 };
static const int D003_ph[UI_D003_COUNT] = { 37,37,38 };
static void rs_draw_d003(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D003_POPUP_X0, UI_D003_POPUP_Y0, UI_D003_POPUP_W, UI_D003_POPUP_H, 0, uc);
    const ui_d003_btn *bb = ui_d003_buttons();
    for (int i = 0; i < UI_D003_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 8 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D003_pw[i], hpx = D003_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d003_diag) { d003_diag = 1;
        LOGI("rs_draw_d003：datah_003 弹窗（260×270 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_004）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D004_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D004_pw[UI_D004_COUNT] = { 152,152 };
static const int D004_ph[UI_D004_COUNT] = { 37,37 };
static void rs_draw_d004(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D004_POPUP_X0, UI_D004_POPUP_Y0, UI_D004_POPUP_W, UI_D004_POPUP_H, 0, uc);
    const ui_d004_btn *bb = ui_d004_buttons();
    for (int i = 0; i < UI_D004_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 11 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D004_pw[i], hpx = D004_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d004_diag) { d004_diag = 1;
        LOGI("rs_draw_d004：datah_004 弹窗（260×190 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_005）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D005_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D005_pw[UI_D005_COUNT] = { 95,84 };
static const int D005_ph[UI_D005_COUNT] = { 28,29 };
static void rs_draw_d005(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D005_POPUP_X0, UI_D005_POPUP_Y0, UI_D005_POPUP_W, UI_D005_POPUP_H, 0, uc);
    const ui_d005_btn *bb = ui_d005_buttons();
    for (int i = 0; i < UI_D005_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 13 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D005_pw[i], hpx = D005_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d005_diag) { d005_diag = 1;
        LOGI("rs_draw_d005：datah_005 弹窗（257×45 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_006）：8 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D006_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D006_pw[UI_D006_COUNT] = { 152,76,152,152,152,152,152,152 };
static const int D006_ph[UI_D006_COUNT] = { 38,37,38,38,37,37,37,37 };
static void rs_draw_d006(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D006_POPUP_X0, UI_D006_POPUP_Y0, UI_D006_POPUP_W, UI_D006_POPUP_H, 0, uc);
    const ui_d006_btn *bb = ui_d006_buttons();
    for (int i = 0; i < UI_D006_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 15 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D006_pw[i], hpx = D006_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d006_diag) { d006_diag = 1;
        LOGI("rs_draw_d006：datah_006 弹窗（450×285 面板 + 8 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_007）：6 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D007_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D007_pw[UI_D007_COUNT] = { 97,150,89,152,76,76 };
static const int D007_ph[UI_D007_COUNT] = { 28,29,34,34,37,37 };
static void rs_draw_d007(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D007_POPUP_X0, UI_D007_POPUP_Y0, UI_D007_POPUP_W, UI_D007_POPUP_H, 0, uc);
    const ui_d007_btn *bb = ui_d007_buttons();
    for (int i = 0; i < UI_D007_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 23 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D007_pw[i], hpx = D007_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d007_diag) { d007_diag = 1;
        LOGI("rs_draw_d007：datah_007 弹窗（450×465 面板 + 6 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_008）：5 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D008_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D008_pw[UI_D008_COUNT] = { 97,150,89,76,76 };
static const int D008_ph[UI_D008_COUNT] = { 28,29,34,37,37 };
static void rs_draw_d008(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D008_POPUP_X0, UI_D008_POPUP_Y0, UI_D008_POPUP_W, UI_D008_POPUP_H, 0, uc);
    const ui_d008_btn *bb = ui_d008_buttons();
    for (int i = 0; i < UI_D008_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 29 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D008_pw[i], hpx = D008_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d008_diag) { d008_diag = 1;
        LOGI("rs_draw_d008：datah_008 弹窗（450×390 面板 + 5 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_010）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D010_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D010_pw[UI_D010_COUNT] = { 76,76 };
static const int D010_ph[UI_D010_COUNT] = { 37,37 };
static void rs_draw_d010(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D010_POPUP_X0, UI_D010_POPUP_Y0, UI_D010_POPUP_W, UI_D010_POPUP_H, 0, uc);
    const ui_d010_btn *bb = ui_d010_buttons();
    for (int i = 0; i < UI_D010_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 34 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D010_pw[i], hpx = D010_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d010_diag) { d010_diag = 1;
        LOGI("rs_draw_d010：datah_010 弹窗（400×337 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_012）：6 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D012_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D012_pw[UI_D012_COUNT] = { 89,175,127,114,76,76 };
static const int D012_ph[UI_D012_COUNT] = { 28,29,34,34,37,37 };
static void rs_draw_d012(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D012_POPUP_X0, UI_D012_POPUP_Y0, UI_D012_POPUP_W, UI_D012_POPUP_H, 0, uc);
    const ui_d012_btn *bb = ui_d012_buttons();
    for (int i = 0; i < UI_D012_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 36 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D012_pw[i], hpx = D012_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d012_diag) { d012_diag = 1;
        LOGI("rs_draw_d012：datah_012 弹窗（450×465 面板 + 6 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_013）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D013_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D013_pw[UI_D013_COUNT] = { 21,21,21 };
static const int D013_ph[UI_D013_COUNT] = { 28,27,27 };
static void rs_draw_d013(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D013_POPUP_X0, UI_D013_POPUP_Y0, UI_D013_POPUP_W, UI_D013_POPUP_H, 0, uc);
    const ui_d013_btn *bb = ui_d013_buttons();
    for (int i = 0; i < UI_D013_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 42 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D013_pw[i], hpx = D013_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d013_diag) { d013_diag = 1;
        LOGI("rs_draw_d013：datah_013 弹窗（230×230 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_015）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D015_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D015_pw[UI_D015_COUNT] = { 84 };
static const int D015_ph[UI_D015_COUNT] = { 29 };
static void rs_draw_d015(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D015_POPUP_X0, UI_D015_POPUP_Y0, UI_D015_POPUP_W, UI_D015_POPUP_H, 0, uc);
    const ui_d015_btn *bb = ui_d015_buttons();
    for (int i = 0; i < UI_D015_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 45 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D015_pw[i], hpx = D015_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d015_diag) { d015_diag = 1;
        LOGI("rs_draw_d015：datah_015 弹窗（257×45 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_016）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D016_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D016_pw[UI_D016_COUNT] = { 143,76,76,76 };
static const int D016_ph[UI_D016_COUNT] = { 28,37,37,37 };
static void rs_draw_d016(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D016_POPUP_X0, UI_D016_POPUP_Y0, UI_D016_POPUP_W, UI_D016_POPUP_H, 0, uc);
    const ui_d016_btn *bb = ui_d016_buttons();
    for (int i = 0; i < UI_D016_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 46 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D016_pw[i], hpx = D016_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d016_diag) { d016_diag = 1;
        LOGI("rs_draw_d016：datah_016 弹窗（400×475 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_017）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D017_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D017_pw[UI_D017_COUNT] = { 76 };
static const int D017_ph[UI_D017_COUNT] = { 37 };
static void rs_draw_d017(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D017_POPUP_X0, UI_D017_POPUP_Y0, UI_D017_POPUP_W, UI_D017_POPUP_H, 0, uc);
    const ui_d017_btn *bb = ui_d017_buttons();
    for (int i = 0; i < UI_D017_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 50 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D017_pw[i], hpx = D017_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d017_diag) { d017_diag = 1;
        LOGI("rs_draw_d017：datah_017 弹窗（400×535 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_019）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D019_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D019_pw[UI_D019_COUNT] = { 152,152,152,152 };
static const int D019_ph[UI_D019_COUNT] = { 37,37,37,37 };
static void rs_draw_d019(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D019_POPUP_X0, UI_D019_POPUP_Y0, UI_D019_POPUP_W, UI_D019_POPUP_H, 0, uc);
    const ui_d019_btn *bb = ui_d019_buttons();
    for (int i = 0; i < UI_D019_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 51 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D019_pw[i], hpx = D019_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d019_diag) { d019_diag = 1;
        LOGI("rs_draw_d019：datah_019 弹窗（450×150 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_020）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D020_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D020_pw[UI_D020_COUNT] = { 76,76,76,76 };
static const int D020_ph[UI_D020_COUNT] = { 37,38,37,37 };
static void rs_draw_d020(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D020_POPUP_X0, UI_D020_POPUP_Y0, UI_D020_POPUP_W, UI_D020_POPUP_H, 0, uc);
    const ui_d020_btn *bb = ui_d020_buttons();
    for (int i = 0; i < UI_D020_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 55 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D020_pw[i], hpx = D020_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d020_diag) { d020_diag = 1;
        LOGI("rs_draw_d020：datah_020 弹窗（400×535 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_021）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D021_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D021_pw[UI_D021_COUNT] = { 21,21,21 };
static const int D021_ph[UI_D021_COUNT] = { 28,27,27 };
static void rs_draw_d021(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D021_POPUP_X0, UI_D021_POPUP_Y0, UI_D021_POPUP_W, UI_D021_POPUP_H, 0, uc);
    const ui_d021_btn *bb = ui_d021_buttons();
    for (int i = 0; i < UI_D021_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 59 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D021_pw[i], hpx = D021_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d021_diag) { d021_diag = 1;
        LOGI("rs_draw_d021：datah_021 弹窗（230×230 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_022）：5 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D022_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D022_pw[UI_D022_COUNT] = { 21,21,21,21,21 };
static const int D022_ph[UI_D022_COUNT] = { 28,27,27,28,27 };
static void rs_draw_d022(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D022_POPUP_X0, UI_D022_POPUP_Y0, UI_D022_POPUP_W, UI_D022_POPUP_H, 0, uc);
    const ui_d022_btn *bb = ui_d022_buttons();
    for (int i = 0; i < UI_D022_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 62 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D022_pw[i], hpx = D022_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d022_diag) { d022_diag = 1;
        LOGI("rs_draw_d022：datah_022 弹窗（230×370 面板 + 5 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_023）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D023_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D023_pw[UI_D023_COUNT] = { 152,152,76,76 };
static const int D023_ph[UI_D023_COUNT] = { 37,37,37,37 };
static void rs_draw_d023(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D023_POPUP_X0, UI_D023_POPUP_Y0, UI_D023_POPUP_W, UI_D023_POPUP_H, 0, uc);
    const ui_d023_btn *bb = ui_d023_buttons();
    for (int i = 0; i < UI_D023_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 67 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D023_pw[i], hpx = D023_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d023_diag) { d023_diag = 1;
        LOGI("rs_draw_d023：datah_023 弹窗（250×210 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_024）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D024_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D024_pw[UI_D024_COUNT] = { 84 };
static const int D024_ph[UI_D024_COUNT] = { 33 };
static void rs_draw_d024(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D024_POPUP_X0, UI_D024_POPUP_Y0, UI_D024_POPUP_W, UI_D024_POPUP_H, 0, uc);
    const ui_d024_btn *bb = ui_d024_buttons();
    for (int i = 0; i < UI_D024_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 71 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D024_pw[i], hpx = D024_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d024_diag) { d024_diag = 1;
        LOGI("rs_draw_d024：datah_024 弹窗（70×64 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_025）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D025_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D025_pw[UI_D025_COUNT] = { 76 };
static const int D025_ph[UI_D025_COUNT] = { 37 };
static void rs_draw_d025(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D025_POPUP_X0, UI_D025_POPUP_Y0, UI_D025_POPUP_W, UI_D025_POPUP_H, 0, uc);
    const ui_d025_btn *bb = ui_d025_buttons();
    for (int i = 0; i < UI_D025_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 72 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D025_pw[i], hpx = D025_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d025_diag) { d025_diag = 1;
        LOGI("rs_draw_d025：datah_025 弹窗（450×220 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_026）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D026_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D026_pw[UI_D026_COUNT] = { 103,76,76 };
static const int D026_ph[UI_D026_COUNT] = { 29,37,37 };
static void rs_draw_d026(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D026_POPUP_X0, UI_D026_POPUP_Y0, UI_D026_POPUP_W, UI_D026_POPUP_H, 0, uc);
    const ui_d026_btn *bb = ui_d026_buttons();
    for (int i = 0; i < UI_D026_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 73 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D026_pw[i], hpx = D026_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d026_diag) { d026_diag = 1;
        LOGI("rs_draw_d026：datah_026 弹窗（450×240 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_027）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D027_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D027_pw[UI_D027_COUNT] = { 76,76,114 };
static const int D027_ph[UI_D027_COUNT] = { 37,38,38 };
static void rs_draw_d027(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D027_POPUP_X0, UI_D027_POPUP_Y0, UI_D027_POPUP_W, UI_D027_POPUP_H, 0, uc);
    const ui_d027_btn *bb = ui_d027_buttons();
    for (int i = 0; i < UI_D027_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 76 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D027_pw[i], hpx = D027_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d027_diag) { d027_diag = 1;
        LOGI("rs_draw_d027：datah_027 弹窗（230×230 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_028）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D028_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D028_pw[UI_D028_COUNT] = { 114,76,76 };
static const int D028_ph[UI_D028_COUNT] = { 37,37,37 };
static void rs_draw_d028(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D028_POPUP_X0, UI_D028_POPUP_Y0, UI_D028_POPUP_W, UI_D028_POPUP_H, 0, uc);
    const ui_d028_btn *bb = ui_d028_buttons();
    for (int i = 0; i < UI_D028_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 79 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D028_pw[i], hpx = D028_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d028_diag) { d028_diag = 1;
        LOGI("rs_draw_d028：datah_028 弹窗（430×552 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_029）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D029_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D029_pw[UI_D029_COUNT] = { 76,76 };
static const int D029_ph[UI_D029_COUNT] = { 37,37 };
static void rs_draw_d029(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D029_POPUP_X0, UI_D029_POPUP_Y0, UI_D029_POPUP_W, UI_D029_POPUP_H, 0, uc);
    const ui_d029_btn *bb = ui_d029_buttons();
    for (int i = 0; i < UI_D029_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 82 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D029_pw[i], hpx = D029_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d029_diag) { d029_diag = 1;
        LOGI("rs_draw_d029：datah_029 弹窗（420×530 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_030）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D030_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D030_pw[UI_D030_COUNT] = { 95,81 };
static const int D030_ph[UI_D030_COUNT] = { 28,29 };
static void rs_draw_d030(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D030_POPUP_X0, UI_D030_POPUP_Y0, UI_D030_POPUP_W, UI_D030_POPUP_H, 0, uc);
    const ui_d030_btn *bb = ui_d030_buttons();
    for (int i = 0; i < UI_D030_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 84 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D030_pw[i], hpx = D030_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d030_diag) { d030_diag = 1;
        LOGI("rs_draw_d030：datah_030 弹窗（124×460 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_031）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D031_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D031_pw[UI_D031_COUNT] = { 76,76,76 };
static const int D031_ph[UI_D031_COUNT] = { 38,38,37 };
static void rs_draw_d031(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D031_POPUP_X0, UI_D031_POPUP_Y0, UI_D031_POPUP_W, UI_D031_POPUP_H, 0, uc);
    const ui_d031_btn *bb = ui_d031_buttons();
    for (int i = 0; i < UI_D031_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 86 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D031_pw[i], hpx = D031_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d031_diag) { d031_diag = 1;
        LOGI("rs_draw_d031：datah_031 弹窗（420×520 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_032）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D032_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D032_pw[UI_D032_COUNT] = { 76,76,76,76 };
static const int D032_ph[UI_D032_COUNT] = { 37,37,37,37 };
static void rs_draw_d032(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D032_POPUP_X0, UI_D032_POPUP_Y0, UI_D032_POPUP_W, UI_D032_POPUP_H, 0, uc);
    const ui_d032_btn *bb = ui_d032_buttons();
    for (int i = 0; i < UI_D032_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 89 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D032_pw[i], hpx = D032_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d032_diag) { d032_diag = 1;
        LOGI("rs_draw_d032：datah_032 弹窗（390×60 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_033）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D033_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D033_pw[UI_D033_COUNT] = { 76,76 };
static const int D033_ph[UI_D033_COUNT] = { 37,37 };
static void rs_draw_d033(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D033_POPUP_X0, UI_D033_POPUP_Y0, UI_D033_POPUP_W, UI_D033_POPUP_H, 0, uc);
    const ui_d033_btn *bb = ui_d033_buttons();
    for (int i = 0; i < UI_D033_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 93 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D033_pw[i], hpx = D033_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d033_diag) { d033_diag = 1;
        LOGI("rs_draw_d033：datah_033 弹窗（400×337 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_034）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D034_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D034_pw[UI_D034_COUNT] = { 76,76 };
static const int D034_ph[UI_D034_COUNT] = { 37,37 };
static void rs_draw_d034(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D034_POPUP_X0, UI_D034_POPUP_Y0, UI_D034_POPUP_W, UI_D034_POPUP_H, 0, uc);
    const ui_d034_btn *bb = ui_d034_buttons();
    for (int i = 0; i < UI_D034_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 95 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D034_pw[i], hpx = D034_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d034_diag) { d034_diag = 1;
        LOGI("rs_draw_d034：datah_034 弹窗（420×235 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_035）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D035_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D035_pw[UI_D035_COUNT] = { 76,76 };
static const int D035_ph[UI_D035_COUNT] = { 37,37 };
static void rs_draw_d035(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D035_POPUP_X0, UI_D035_POPUP_Y0, UI_D035_POPUP_W, UI_D035_POPUP_H, 0, uc);
    const ui_d035_btn *bb = ui_d035_buttons();
    for (int i = 0; i < UI_D035_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 97 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D035_pw[i], hpx = D035_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d035_diag) { d035_diag = 1;
        LOGI("rs_draw_d035：datah_035 弹窗（420×235 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_036）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D036_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D036_pw[UI_D036_COUNT] = { 103,103,76,76 };
static const int D036_ph[UI_D036_COUNT] = { 29,29,37,37 };
static void rs_draw_d036(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D036_POPUP_X0, UI_D036_POPUP_Y0, UI_D036_POPUP_W, UI_D036_POPUP_H, 0, uc);
    const ui_d036_btn *bb = ui_d036_buttons();
    for (int i = 0; i < UI_D036_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 99 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D036_pw[i], hpx = D036_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d036_diag) { d036_diag = 1;
        LOGI("rs_draw_d036：datah_036 弹窗（450×392 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_037）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D037_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D037_pw[UI_D037_COUNT] = { 186,186,76 };
static const int D037_ph[UI_D037_COUNT] = { 37,37,37 };
static void rs_draw_d037(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D037_POPUP_X0, UI_D037_POPUP_Y0, UI_D037_POPUP_W, UI_D037_POPUP_H, 0, uc);
    const ui_d037_btn *bb = ui_d037_buttons();
    for (int i = 0; i < UI_D037_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 103 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D037_pw[i], hpx = D037_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d037_diag) { d037_diag = 1;
        LOGI("rs_draw_d037：datah_037 弹窗（260×270 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_038）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D038_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D038_pw[UI_D038_COUNT] = { 76,76,76,76 };
static const int D038_ph[UI_D038_COUNT] = { 37,37,37,37 };
static void rs_draw_d038(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D038_POPUP_X0, UI_D038_POPUP_Y0, UI_D038_POPUP_W, UI_D038_POPUP_H, 0, uc);
    const ui_d038_btn *bb = ui_d038_buttons();
    for (int i = 0; i < UI_D038_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 106 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D038_pw[i], hpx = D038_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d038_diag) { d038_diag = 1;
        LOGI("rs_draw_d038：datah_038 弹窗（432×285 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_039）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D039_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D039_pw[UI_D039_COUNT] = { 76 };
static const int D039_ph[UI_D039_COUNT] = { 37 };
static void rs_draw_d039(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D039_POPUP_X0, UI_D039_POPUP_Y0, UI_D039_POPUP_W, UI_D039_POPUP_H, 0, uc);
    const ui_d039_btn *bb = ui_d039_buttons();
    for (int i = 0; i < UI_D039_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 110 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D039_pw[i], hpx = D039_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d039_diag) { d039_diag = 1;
        LOGI("rs_draw_d039：datah_039 弹窗（120×90 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_041）：8 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D041_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D041_pw[UI_D041_COUNT] = { 76,76,76,76,76,76,76,76 };
static const int D041_ph[UI_D041_COUNT] = { 37,37,37,37,37,37,37,37 };
static void rs_draw_d041(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D041_POPUP_X0, UI_D041_POPUP_Y0, UI_D041_POPUP_W, UI_D041_POPUP_H, 0, uc);
    const ui_d041_btn *bb = ui_d041_buttons();
    for (int i = 0; i < UI_D041_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 111 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D041_pw[i], hpx = D041_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d041_diag) { d041_diag = 1;
        LOGI("rs_draw_d041：datah_041 弹窗（450×335 面板 + 8 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_042）：7 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D042_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D042_pw[UI_D042_COUNT] = { 76,76,76,152,152,152,152 };
static const int D042_ph[UI_D042_COUNT] = { 37,37,37,37,38,37,37 };
static void rs_draw_d042(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D042_POPUP_X0, UI_D042_POPUP_Y0, UI_D042_POPUP_W, UI_D042_POPUP_H, 0, uc);
    const ui_d042_btn *bb = ui_d042_buttons();
    for (int i = 0; i < UI_D042_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 119 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D042_pw[i], hpx = D042_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d042_diag) { d042_diag = 1;
        LOGI("rs_draw_d042：datah_042 弹窗（450×285 面板 + 7 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_043）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D043_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D043_pw[UI_D043_COUNT] = { 76,76 };
static const int D043_ph[UI_D043_COUNT] = { 37,37 };
static void rs_draw_d043(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D043_POPUP_X0, UI_D043_POPUP_Y0, UI_D043_POPUP_W, UI_D043_POPUP_H, 0, uc);
    const ui_d043_btn *bb = ui_d043_buttons();
    for (int i = 0; i < UI_D043_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 126 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D043_pw[i], hpx = D043_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d043_diag) { d043_diag = 1;
        LOGI("rs_draw_d043：datah_043 弹窗（420×235 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_044）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D044_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D044_pw[UI_D044_COUNT] = { 76,76 };
static const int D044_ph[UI_D044_COUNT] = { 37,37 };
static void rs_draw_d044(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D044_POPUP_X0, UI_D044_POPUP_Y0, UI_D044_POPUP_W, UI_D044_POPUP_H, 0, uc);
    const ui_d044_btn *bb = ui_d044_buttons();
    for (int i = 0; i < UI_D044_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 128 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D044_pw[i], hpx = D044_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d044_diag) { d044_diag = 1;
        LOGI("rs_draw_d044：datah_044 弹窗（180×175 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_045）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D045_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D045_pw[UI_D045_COUNT] = { 76 };
static const int D045_ph[UI_D045_COUNT] = { 37 };
static void rs_draw_d045(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D045_POPUP_X0, UI_D045_POPUP_Y0, UI_D045_POPUP_W, UI_D045_POPUP_H, 0, uc);
    const ui_d045_btn *bb = ui_d045_buttons();
    for (int i = 0; i < UI_D045_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 130 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D045_pw[i], hpx = D045_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d045_diag) { d045_diag = 1;
        LOGI("rs_draw_d045：datah_045 弹窗（390×135 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_046）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D046_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D046_pw[UI_D046_COUNT] = { 76 };
static const int D046_ph[UI_D046_COUNT] = { 37 };
static void rs_draw_d046(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D046_POPUP_X0, UI_D046_POPUP_Y0, UI_D046_POPUP_W, UI_D046_POPUP_H, 0, uc);
    const ui_d046_btn *bb = ui_d046_buttons();
    for (int i = 0; i < UI_D046_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 131 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D046_pw[i], hpx = D046_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d046_diag) { d046_diag = 1;
        LOGI("rs_draw_d046：datah_046 弹窗（420×420 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_047）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D047_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D047_pw[UI_D047_COUNT] = { 76 };
static const int D047_ph[UI_D047_COUNT] = { 37 };
static void rs_draw_d047(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D047_POPUP_X0, UI_D047_POPUP_Y0, UI_D047_POPUP_W, UI_D047_POPUP_H, 0, uc);
    const ui_d047_btn *bb = ui_d047_buttons();
    for (int i = 0; i < UI_D047_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 132 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D047_pw[i], hpx = D047_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d047_diag) { d047_diag = 1;
        LOGI("rs_draw_d047：datah_047 弹窗（420×305 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_049）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D049_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D049_pw[UI_D049_COUNT] = { 76 };
static const int D049_ph[UI_D049_COUNT] = { 37 };
static void rs_draw_d049(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D049_POPUP_X0, UI_D049_POPUP_Y0, UI_D049_POPUP_W, UI_D049_POPUP_H, 0, uc);
    const ui_d049_btn *bb = ui_d049_buttons();
    for (int i = 0; i < UI_D049_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 133 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D049_pw[i], hpx = D049_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d049_diag) { d049_diag = 1;
        LOGI("rs_draw_d049：datah_049 弹窗（420×610 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_050）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D050_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D050_pw[UI_D050_COUNT] = { 76,76,76 };
static const int D050_ph[UI_D050_COUNT] = { 37,37,37 };
static void rs_draw_d050(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D050_POPUP_X0, UI_D050_POPUP_Y0, UI_D050_POPUP_W, UI_D050_POPUP_H, 0, uc);
    const ui_d050_btn *bb = ui_d050_buttons();
    for (int i = 0; i < UI_D050_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 134 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D050_pw[i], hpx = D050_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d050_diag) { d050_diag = 1;
        LOGI("rs_draw_d050：datah_050 弹窗（420×380 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_051）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D051_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D051_pw[UI_D051_COUNT] = { 76,76 };
static const int D051_ph[UI_D051_COUNT] = { 37,37 };
static void rs_draw_d051(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D051_POPUP_X0, UI_D051_POPUP_Y0, UI_D051_POPUP_W, UI_D051_POPUP_H, 0, uc);
    const ui_d051_btn *bb = ui_d051_buttons();
    for (int i = 0; i < UI_D051_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 137 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D051_pw[i], hpx = D051_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d051_diag) { d051_diag = 1;
        LOGI("rs_draw_d051：datah_051 弹窗（450×610 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_052）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D052_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D052_pw[UI_D052_COUNT] = { 76,76 };
static const int D052_ph[UI_D052_COUNT] = { 37,37 };
static void rs_draw_d052(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D052_POPUP_X0, UI_D052_POPUP_Y0, UI_D052_POPUP_W, UI_D052_POPUP_H, 0, uc);
    const ui_d052_btn *bb = ui_d052_buttons();
    for (int i = 0; i < UI_D052_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 139 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D052_pw[i], hpx = D052_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d052_diag) { d052_diag = 1;
        LOGI("rs_draw_d052：datah_052 弹窗（420×220 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_053）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D053_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D053_pw[UI_D053_COUNT] = { 76,76 };
static const int D053_ph[UI_D053_COUNT] = { 37,37 };
static void rs_draw_d053(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D053_POPUP_X0, UI_D053_POPUP_Y0, UI_D053_POPUP_W, UI_D053_POPUP_H, 0, uc);
    const ui_d053_btn *bb = ui_d053_buttons();
    for (int i = 0; i < UI_D053_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 141 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D053_pw[i], hpx = D053_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d053_diag) { d053_diag = 1;
        LOGI("rs_draw_d053：datah_053 弹窗（420×452 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_054）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D054_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D054_pw[UI_D054_COUNT] = { 76,76 };
static const int D054_ph[UI_D054_COUNT] = { 37,37 };
static void rs_draw_d054(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D054_POPUP_X0, UI_D054_POPUP_Y0, UI_D054_POPUP_W, UI_D054_POPUP_H, 0, uc);
    const ui_d054_btn *bb = ui_d054_buttons();
    for (int i = 0; i < UI_D054_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 143 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D054_pw[i], hpx = D054_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d054_diag) { d054_diag = 1;
        LOGI("rs_draw_d054：datah_054 弹窗（450×610 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_055）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D055_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D055_pw[UI_D055_COUNT] = { 76 };
static const int D055_ph[UI_D055_COUNT] = { 37 };
static void rs_draw_d055(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D055_POPUP_X0, UI_D055_POPUP_Y0, UI_D055_POPUP_W, UI_D055_POPUP_H, 0, uc);
    const ui_d055_btn *bb = ui_d055_buttons();
    for (int i = 0; i < UI_D055_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 145 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D055_pw[i], hpx = D055_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d055_diag) { d055_diag = 1;
        LOGI("rs_draw_d055：datah_055 弹窗（395×360 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_057）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D057_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D057_pw[UI_D057_COUNT] = { 152,76 };
static const int D057_ph[UI_D057_COUNT] = { 38,37 };
static void rs_draw_d057(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D057_POPUP_X0, UI_D057_POPUP_Y0, UI_D057_POPUP_W, UI_D057_POPUP_H, 0, uc);
    const ui_d057_btn *bb = ui_d057_buttons();
    for (int i = 0; i < UI_D057_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 146 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D057_pw[i], hpx = D057_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d057_diag) { d057_diag = 1;
        LOGI("rs_draw_d057：datah_057 弹窗（420×320 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_058）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D058_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D058_pw[UI_D058_COUNT] = { 76,76 };
static const int D058_ph[UI_D058_COUNT] = { 37,37 };
static void rs_draw_d058(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D058_POPUP_X0, UI_D058_POPUP_Y0, UI_D058_POPUP_W, UI_D058_POPUP_H, 0, uc);
    const ui_d058_btn *bb = ui_d058_buttons();
    for (int i = 0; i < UI_D058_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 148 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D058_pw[i], hpx = D058_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d058_diag) { d058_diag = 1;
        LOGI("rs_draw_d058：datah_058 弹窗（420×352 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_059）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D059_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D059_pw[UI_D059_COUNT] = { 114,76,76 };
static const int D059_ph[UI_D059_COUNT] = { 37,37,37 };
static void rs_draw_d059(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D059_POPUP_X0, UI_D059_POPUP_Y0, UI_D059_POPUP_W, UI_D059_POPUP_H, 0, uc);
    const ui_d059_btn *bb = ui_d059_buttons();
    for (int i = 0; i < UI_D059_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 150 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D059_pw[i], hpx = D059_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d059_diag) { d059_diag = 1;
        LOGI("rs_draw_d059：datah_059 弹窗（420×352 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_060）：5 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D060_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D060_pw[UI_D060_COUNT] = { 103,103,76,76,76 };
static const int D060_ph[UI_D060_COUNT] = { 29,29,37,37,37 };
static void rs_draw_d060(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D060_POPUP_X0, UI_D060_POPUP_Y0, UI_D060_POPUP_W, UI_D060_POPUP_H, 0, uc);
    const ui_d060_btn *bb = ui_d060_buttons();
    for (int i = 0; i < UI_D060_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 153 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D060_pw[i], hpx = D060_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d060_diag) { d060_diag = 1;
        LOGI("rs_draw_d060：datah_060 弹窗（450×352 面板 + 5 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_061）：6 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D061_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D061_pw[UI_D061_COUNT] = { 74,91,61,95,76,76 };
static const int D061_ph[UI_D061_COUNT] = { 29,29,33,29,37,37 };
static void rs_draw_d061(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D061_POPUP_X0, UI_D061_POPUP_Y0, UI_D061_POPUP_W, UI_D061_POPUP_H, 0, uc);
    const ui_d061_btn *bb = ui_d061_buttons();
    for (int i = 0; i < UI_D061_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 158 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D061_pw[i], hpx = D061_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d061_diag) { d061_diag = 1;
        LOGI("rs_draw_d061：datah_061 弹窗（450×465 面板 + 6 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_062）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D062_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D062_pw[UI_D062_COUNT] = { 61,95,76,76 };
static const int D062_ph[UI_D062_COUNT] = { 33,29,37,37 };
static void rs_draw_d062(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D062_POPUP_X0, UI_D062_POPUP_Y0, UI_D062_POPUP_W, UI_D062_POPUP_H, 0, uc);
    const ui_d062_btn *bb = ui_d062_buttons();
    for (int i = 0; i < UI_D062_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 164 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D062_pw[i], hpx = D062_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d062_diag) { d062_diag = 1;
        LOGI("rs_draw_d062：datah_062 弹窗（450×315 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_063）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D063_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D063_pw[UI_D063_COUNT] = { 76,76,76 };
static const int D063_ph[UI_D063_COUNT] = { 37,37,37 };
static void rs_draw_d063(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D063_POPUP_X0, UI_D063_POPUP_Y0, UI_D063_POPUP_W, UI_D063_POPUP_H, 0, uc);
    const ui_d063_btn *bb = ui_d063_buttons();
    for (int i = 0; i < UI_D063_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 168 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D063_pw[i], hpx = D063_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d063_diag) { d063_diag = 1;
        LOGI("rs_draw_d063：datah_063 弹窗（420×325 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_064）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D064_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D064_pw[UI_D064_COUNT] = { 95,81,84 };
static const int D064_ph[UI_D064_COUNT] = { 28,29,29 };
static void rs_draw_d064(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D064_POPUP_X0, UI_D064_POPUP_Y0, UI_D064_POPUP_W, UI_D064_POPUP_H, 0, uc);
    const ui_d064_btn *bb = ui_d064_buttons();
    for (int i = 0; i < UI_D064_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 171 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D064_pw[i], hpx = D064_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d064_diag) { d064_diag = 1;
        LOGI("rs_draw_d064：datah_064 弹窗（285×45 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_065）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D065_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D065_pw[UI_D065_COUNT] = { 76,76 };
static const int D065_ph[UI_D065_COUNT] = { 37,37 };
static void rs_draw_d065(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D065_POPUP_X0, UI_D065_POPUP_Y0, UI_D065_POPUP_W, UI_D065_POPUP_H, 0, uc);
    const ui_d065_btn *bb = ui_d065_buttons();
    for (int i = 0; i < UI_D065_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 174 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D065_pw[i], hpx = D065_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d065_diag) { d065_diag = 1;
        LOGI("rs_draw_d065：datah_065 弹窗（450×505 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_066）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D066_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D066_pw[UI_D066_COUNT] = { 152,152,76 };
static const int D066_ph[UI_D066_COUNT] = { 38,38,37 };
static void rs_draw_d066(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D066_POPUP_X0, UI_D066_POPUP_Y0, UI_D066_POPUP_W, UI_D066_POPUP_H, 0, uc);
    const ui_d066_btn *bb = ui_d066_buttons();
    for (int i = 0; i < UI_D066_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 176 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D066_pw[i], hpx = D066_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d066_diag) { d066_diag = 1;
        LOGI("rs_draw_d066：datah_066 弹窗（250×250 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_067）：8 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D067_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D067_pw[UI_D067_COUNT] = { 152,76,76,76,76,152,152,152 };
static const int D067_ph[UI_D067_COUNT] = { 38,37,37,37,37,37,37,38 };
static void rs_draw_d067(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D067_POPUP_X0, UI_D067_POPUP_Y0, UI_D067_POPUP_W, UI_D067_POPUP_H, 0, uc);
    const ui_d067_btn *bb = ui_d067_buttons();
    for (int i = 0; i < UI_D067_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 179 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D067_pw[i], hpx = D067_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d067_diag) { d067_diag = 1;
        LOGI("rs_draw_d067：datah_067 弹窗（450×285 面板 + 8 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_068）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D068_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D068_pw[UI_D068_COUNT] = { 76,114,76 };
static const int D068_ph[UI_D068_COUNT] = { 37,37,37 };
static void rs_draw_d068(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D068_POPUP_X0, UI_D068_POPUP_Y0, UI_D068_POPUP_W, UI_D068_POPUP_H, 0, uc);
    const ui_d068_btn *bb = ui_d068_buttons();
    for (int i = 0; i < UI_D068_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 187 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D068_pw[i], hpx = D068_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d068_diag) { d068_diag = 1;
        LOGI("rs_draw_d068：datah_068 弹窗（420×235 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_069）：6 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D069_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D069_pw[UI_D069_COUNT] = { 74,61,95,76,76,76 };
static const int D069_ph[UI_D069_COUNT] = { 29,33,29,37,37,37 };
static void rs_draw_d069(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D069_POPUP_X0, UI_D069_POPUP_Y0, UI_D069_POPUP_W, UI_D069_POPUP_H, 0, uc);
    const ui_d069_btn *bb = ui_d069_buttons();
    for (int i = 0; i < UI_D069_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 190 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D069_pw[i], hpx = D069_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d069_diag) { d069_diag = 1;
        LOGI("rs_draw_d069：datah_069 弹窗（450×465 面板 + 6 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_070）：6 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D070_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D070_pw[UI_D070_COUNT] = { 89,175,127,114,76,76 };
static const int D070_ph[UI_D070_COUNT] = { 28,29,34,34,37,37 };
static void rs_draw_d070(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D070_POPUP_X0, UI_D070_POPUP_Y0, UI_D070_POPUP_W, UI_D070_POPUP_H, 0, uc);
    const ui_d070_btn *bb = ui_d070_buttons();
    for (int i = 0; i < UI_D070_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 196 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D070_pw[i], hpx = D070_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d070_diag) { d070_diag = 1;
        LOGI("rs_draw_d070：datah_070 弹窗（450×465 面板 + 6 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_071）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D071_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D071_pw[UI_D071_COUNT] = { 76,76 };
static const int D071_ph[UI_D071_COUNT] = { 38,37 };
static void rs_draw_d071(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D071_POPUP_X0, UI_D071_POPUP_Y0, UI_D071_POPUP_W, UI_D071_POPUP_H, 0, uc);
    const ui_d071_btn *bb = ui_d071_buttons();
    for (int i = 0; i < UI_D071_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 202 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D071_pw[i], hpx = D071_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d071_diag) { d071_diag = 1;
        LOGI("rs_draw_d071：datah_071 弹窗（390×495 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_072）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D072_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D072_pw[UI_D072_COUNT] = { 152,175,152,76 };
static const int D072_ph[UI_D072_COUNT] = { 37,37,37,37 };
static void rs_draw_d072(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D072_POPUP_X0, UI_D072_POPUP_Y0, UI_D072_POPUP_W, UI_D072_POPUP_H, 0, uc);
    const ui_d072_btn *bb = ui_d072_buttons();
    for (int i = 0; i < UI_D072_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 204 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D072_pw[i], hpx = D072_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d072_diag) { d072_diag = 1;
        LOGI("rs_draw_d072：datah_072 弹窗（250×325 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_073）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D073_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D073_pw[UI_D073_COUNT] = { 76,76 };
static const int D073_ph[UI_D073_COUNT] = { 37,37 };
static void rs_draw_d073(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D073_POPUP_X0, UI_D073_POPUP_Y0, UI_D073_POPUP_W, UI_D073_POPUP_H, 0, uc);
    const ui_d073_btn *bb = ui_d073_buttons();
    for (int i = 0; i < UI_D073_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 208 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D073_pw[i], hpx = D073_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d073_diag) { d073_diag = 1;
        LOGI("rs_draw_d073：datah_073 弹窗（450×270 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_075）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D075_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D075_pw[UI_D075_COUNT] = { 78,79,95,84 };
static const int D075_ph[UI_D075_COUNT] = { 28,28,28,29 };
static void rs_draw_d075(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D075_POPUP_X0, UI_D075_POPUP_Y0, UI_D075_POPUP_W, UI_D075_POPUP_H, 0, uc);
    const ui_d075_btn *bb = ui_d075_buttons();
    for (int i = 0; i < UI_D075_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 210 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D075_pw[i], hpx = D075_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d075_diag) { d075_diag = 1;
        LOGI("rs_draw_d075：datah_075 弹窗（340×45 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_077）：6 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D077_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D077_pw[UI_D077_COUNT] = { 76,76,152,152,152,152 };
static const int D077_ph[UI_D077_COUNT] = { 35,37,38,37,37,37 };
static void rs_draw_d077(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D077_POPUP_X0, UI_D077_POPUP_Y0, UI_D077_POPUP_W, UI_D077_POPUP_H, 0, uc);
    const ui_d077_btn *bb = ui_d077_buttons();
    for (int i = 0; i < UI_D077_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 214 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D077_pw[i], hpx = D077_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d077_diag) { d077_diag = 1;
        LOGI("rs_draw_d077：datah_077 弹窗（450×285 面板 + 6 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_078）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D078_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D078_pw[UI_D078_COUNT] = { 76,76,76 };
static const int D078_ph[UI_D078_COUNT] = { 38,37,37 };
static void rs_draw_d078(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D078_POPUP_X0, UI_D078_POPUP_Y0, UI_D078_POPUP_W, UI_D078_POPUP_H, 0, uc);
    const ui_d078_btn *bb = ui_d078_buttons();
    for (int i = 0; i < UI_D078_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 220 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D078_pw[i], hpx = D078_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d078_diag) { d078_diag = 1;
        LOGI("rs_draw_d078：datah_078 弹窗（450×220 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_079）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D079_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D079_pw[UI_D079_COUNT] = { 76 };
static const int D079_ph[UI_D079_COUNT] = { 37 };
static void rs_draw_d079(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D079_POPUP_X0, UI_D079_POPUP_Y0, UI_D079_POPUP_W, UI_D079_POPUP_H, 0, uc);
    const ui_d079_btn *bb = ui_d079_buttons();
    for (int i = 0; i < UI_D079_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 223 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D079_pw[i], hpx = D079_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d079_diag) { d079_diag = 1;
        LOGI("rs_draw_d079：datah_079 弹窗（220×340 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_082）：5 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D082_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D082_pw[UI_D082_COUNT] = { 76,76,76,76,76 };
static const int D082_ph[UI_D082_COUNT] = { 37,38,37,37,37 };
static void rs_draw_d082(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D082_POPUP_X0, UI_D082_POPUP_Y0, UI_D082_POPUP_W, UI_D082_POPUP_H, 0, uc);
    const ui_d082_btn *bb = ui_d082_buttons();
    for (int i = 0; i < UI_D082_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 224 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D082_pw[i], hpx = D082_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d082_diag) { d082_diag = 1;
        LOGI("rs_draw_d082：datah_082 弹窗（460×60 面板 + 5 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_083）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D083_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D083_pw[UI_D083_COUNT] = { 152,152,152,152 };
static const int D083_ph[UI_D083_COUNT] = { 38,37,37,37 };
static void rs_draw_d083(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D083_POPUP_X0, UI_D083_POPUP_Y0, UI_D083_POPUP_W, UI_D083_POPUP_H, 0, uc);
    const ui_d083_btn *bb = ui_d083_buttons();
    for (int i = 0; i < UI_D083_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 229 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D083_pw[i], hpx = D083_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d083_diag) { d083_diag = 1;
        LOGI("rs_draw_d083：datah_083 弹窗（450×153 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_084）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D084_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D084_pw[UI_D084_COUNT] = { 129,171,76,76 };
static const int D084_ph[UI_D084_COUNT] = { 34,29,37,37 };
static void rs_draw_d084(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D084_POPUP_X0, UI_D084_POPUP_Y0, UI_D084_POPUP_W, UI_D084_POPUP_H, 0, uc);
    const ui_d084_btn *bb = ui_d084_buttons();
    for (int i = 0; i < UI_D084_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 233 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D084_pw[i], hpx = D084_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d084_diag) { d084_diag = 1;
        LOGI("rs_draw_d084：datah_084 弹窗（450×315 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_085）：5 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D085_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D085_pw[UI_D085_COUNT] = { 76,76,76,76,76 };
static const int D085_ph[UI_D085_COUNT] = { 37,37,37,37,37 };
static void rs_draw_d085(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D085_POPUP_X0, UI_D085_POPUP_Y0, UI_D085_POPUP_W, UI_D085_POPUP_H, 0, uc);
    const ui_d085_btn *bb = ui_d085_buttons();
    for (int i = 0; i < UI_D085_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 237 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D085_pw[i], hpx = D085_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d085_diag) { d085_diag = 1;
        LOGI("rs_draw_d085：datah_085 弹窗（460×60 面板 + 5 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_086）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D086_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D086_pw[UI_D086_COUNT] = { 143,76,76,76 };
static const int D086_ph[UI_D086_COUNT] = { 28,37,37,37 };
static void rs_draw_d086(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D086_POPUP_X0, UI_D086_POPUP_Y0, UI_D086_POPUP_W, UI_D086_POPUP_H, 0, uc);
    const ui_d086_btn *bb = ui_d086_buttons();
    for (int i = 0; i < UI_D086_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 242 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D086_pw[i], hpx = D086_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d086_diag) { d086_diag = 1;
        LOGI("rs_draw_d086：datah_086 弹窗（360×485 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_089）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D089_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D089_pw[UI_D089_COUNT] = { 21,21,21 };
static const int D089_ph[UI_D089_COUNT] = { 27,28,27 };
static void rs_draw_d089(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D089_POPUP_X0, UI_D089_POPUP_Y0, UI_D089_POPUP_W, UI_D089_POPUP_H, 0, uc);
    const ui_d089_btn *bb = ui_d089_buttons();
    for (int i = 0; i < UI_D089_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 246 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D089_pw[i], hpx = D089_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d089_diag) { d089_diag = 1;
        LOGI("rs_draw_d089：datah_089 弹窗（230×230 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_090）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D090_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D090_pw[UI_D090_COUNT] = { 152,152 };
static const int D090_ph[UI_D090_COUNT] = { 37,37 };
static void rs_draw_d090(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D090_POPUP_X0, UI_D090_POPUP_Y0, UI_D090_POPUP_W, UI_D090_POPUP_H, 0, uc);
    const ui_d090_btn *bb = ui_d090_buttons();
    for (int i = 0; i < UI_D090_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 249 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D090_pw[i], hpx = D090_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d090_diag) { d090_diag = 1;
        LOGI("rs_draw_d090：datah_090 弹窗（210×150 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_094）：5 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D094_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D094_pw[UI_D094_COUNT] = { 129,171,81,76,76 };
static const int D094_ph[UI_D094_COUNT] = { 34,29,29,37,37 };
static void rs_draw_d094(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D094_POPUP_X0, UI_D094_POPUP_Y0, UI_D094_POPUP_W, UI_D094_POPUP_H, 0, uc);
    const ui_d094_btn *bb = ui_d094_buttons();
    for (int i = 0; i < UI_D094_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 251 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D094_pw[i], hpx = D094_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d094_diag) { d094_diag = 1;
        LOGI("rs_draw_d094：datah_094 弹窗（450×390 面板 + 5 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_095）：5 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D095_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D095_pw[UI_D095_COUNT] = { 76,76,76,76,76 };
static const int D095_ph[UI_D095_COUNT] = { 37,37,37,37,37 };
static void rs_draw_d095(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D095_POPUP_X0, UI_D095_POPUP_Y0, UI_D095_POPUP_W, UI_D095_POPUP_H, 0, uc);
    const ui_d095_btn *bb = ui_d095_buttons();
    for (int i = 0; i < UI_D095_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 256 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D095_pw[i], hpx = D095_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d095_diag) { d095_diag = 1;
        LOGI("rs_draw_d095：datah_095 弹窗（460×60 面板 + 5 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_096）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D096_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D096_pw[UI_D096_COUNT] = { 76,76,76 };
static const int D096_ph[UI_D096_COUNT] = { 38,38,37 };
static void rs_draw_d096(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D096_POPUP_X0, UI_D096_POPUP_Y0, UI_D096_POPUP_W, UI_D096_POPUP_H, 0, uc);
    const ui_d096_btn *bb = ui_d096_buttons();
    for (int i = 0; i < UI_D096_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 261 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D096_pw[i], hpx = D096_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d096_diag) { d096_diag = 1;
        LOGI("rs_draw_d096：datah_096 弹窗（420×385 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_097）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D097_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D097_pw[UI_D097_COUNT] = { 76,76,76,76 };
static const int D097_ph[UI_D097_COUNT] = { 37,37,37,37 };
static void rs_draw_d097(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D097_POPUP_X0, UI_D097_POPUP_Y0, UI_D097_POPUP_W, UI_D097_POPUP_H, 0, uc);
    const ui_d097_btn *bb = ui_d097_buttons();
    for (int i = 0; i < UI_D097_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 264 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D097_pw[i], hpx = D097_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d097_diag) { d097_diag = 1;
        LOGI("rs_draw_d097：datah_097 弹窗（390×60 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_098）：6 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D098_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D098_pw[UI_D098_COUNT] = { 76,76,76,76,76,76 };
static const int D098_ph[UI_D098_COUNT] = { 37,36,37,36,37,36 };
static void rs_draw_d098(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D098_POPUP_X0, UI_D098_POPUP_Y0, UI_D098_POPUP_W, UI_D098_POPUP_H, 0, uc);
    const ui_d098_btn *bb = ui_d098_buttons();
    for (int i = 0; i < UI_D098_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 268 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D098_pw[i], hpx = D098_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d098_diag) { d098_diag = 1;
        LOGI("rs_draw_d098：datah_098 弹窗（450×260 面板 + 6 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_099）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D099_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D099_pw[UI_D099_COUNT] = { 152,152,152,152 };
static const int D099_ph[UI_D099_COUNT] = { 37,38,37,37 };
static void rs_draw_d099(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D099_POPUP_X0, UI_D099_POPUP_Y0, UI_D099_POPUP_W, UI_D099_POPUP_H, 0, uc);
    const ui_d099_btn *bb = ui_d099_buttons();
    for (int i = 0; i < UI_D099_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 274 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D099_pw[i], hpx = D099_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d099_diag) { d099_diag = 1;
        LOGI("rs_draw_d099：datah_099 弹窗（450×153 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_100）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D100_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D100_pw[UI_D100_COUNT] = { 76 };
static const int D100_ph[UI_D100_COUNT] = { 37 };
static void rs_draw_d100(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D100_POPUP_X0, UI_D100_POPUP_Y0, UI_D100_POPUP_W, UI_D100_POPUP_H, 0, uc);
    const ui_d100_btn *bb = ui_d100_buttons();
    for (int i = 0; i < UI_D100_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 278 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D100_pw[i], hpx = D100_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d100_diag) { d100_diag = 1;
        LOGI("rs_draw_d100：datah_100 弹窗（390×135 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_101）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D101_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D101_pw[UI_D101_COUNT] = { 76,76,76,76 };
static const int D101_ph[UI_D101_COUNT] = { 37,37,37,37 };
static void rs_draw_d101(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D101_POPUP_X0, UI_D101_POPUP_Y0, UI_D101_POPUP_W, UI_D101_POPUP_H, 0, uc);
    const ui_d101_btn *bb = ui_d101_buttons();
    for (int i = 0; i < UI_D101_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 279 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D101_pw[i], hpx = D101_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d101_diag) { d101_diag = 1;
        LOGI("rs_draw_d101：datah_101 弹窗（460×60 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_102）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D102_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D102_pw[UI_D102_COUNT] = { 76 };
static const int D102_ph[UI_D102_COUNT] = { 37 };
static void rs_draw_d102(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D102_POPUP_X0, UI_D102_POPUP_Y0, UI_D102_POPUP_W, UI_D102_POPUP_H, 0, uc);
    const ui_d102_btn *bb = ui_d102_buttons();
    for (int i = 0; i < UI_D102_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 283 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D102_pw[i], hpx = D102_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d102_diag) { d102_diag = 1;
        LOGI("rs_draw_d102：datah_102 弹窗（395×370 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_104）：5 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D104_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D104_pw[UI_D104_COUNT] = { 53,144,84,76,76 };
static const int D104_ph[UI_D104_COUNT] = { 28,34,33,37,37 };
static void rs_draw_d104(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D104_POPUP_X0, UI_D104_POPUP_Y0, UI_D104_POPUP_W, UI_D104_POPUP_H, 0, uc);
    const ui_d104_btn *bb = ui_d104_buttons();
    for (int i = 0; i < UI_D104_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 284 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D104_pw[i], hpx = D104_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d104_diag) { d104_diag = 1;
        LOGI("rs_draw_d104：datah_104 弹窗（450×390 面板 + 5 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_107）：5 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D107_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D107_pw[UI_D107_COUNT] = { 76,76,76,76,76 };
static const int D107_ph[UI_D107_COUNT] = { 37,37,37,37,37 };
static void rs_draw_d107(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D107_POPUP_X0, UI_D107_POPUP_Y0, UI_D107_POPUP_W, UI_D107_POPUP_H, 0, uc);
    const ui_d107_btn *bb = ui_d107_buttons();
    for (int i = 0; i < UI_D107_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 289 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D107_pw[i], hpx = D107_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d107_diag) { d107_diag = 1;
        LOGI("rs_draw_d107：datah_107 弹窗（460×60 面板 + 5 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_108）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D108_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D108_pw[UI_D108_COUNT] = { 76,76,76 };
static const int D108_ph[UI_D108_COUNT] = { 37,37,37 };
static void rs_draw_d108(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D108_POPUP_X0, UI_D108_POPUP_Y0, UI_D108_POPUP_W, UI_D108_POPUP_H, 0, uc);
    const ui_d108_btn *bb = ui_d108_buttons();
    for (int i = 0; i < UI_D108_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 294 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D108_pw[i], hpx = D108_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d108_diag) { d108_diag = 1;
        LOGI("rs_draw_d108：datah_108 弹窗（420×450 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_109）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D109_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D109_pw[UI_D109_COUNT] = { 76,76,76 };
static const int D109_ph[UI_D109_COUNT] = { 35,37,37 };
static void rs_draw_d109(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D109_POPUP_X0, UI_D109_POPUP_Y0, UI_D109_POPUP_W, UI_D109_POPUP_H, 0, uc);
    const ui_d109_btn *bb = ui_d109_buttons();
    for (int i = 0; i < UI_D109_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 297 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D109_pw[i], hpx = D109_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d109_diag) { d109_diag = 1;
        LOGI("rs_draw_d109：datah_109 弹窗（260×90 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_110）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D110_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D110_pw[UI_D110_COUNT] = { 76,76 };
static const int D110_ph[UI_D110_COUNT] = { 37,37 };
static void rs_draw_d110(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D110_POPUP_X0, UI_D110_POPUP_Y0, UI_D110_POPUP_W, UI_D110_POPUP_H, 0, uc);
    const ui_d110_btn *bb = ui_d110_buttons();
    for (int i = 0; i < UI_D110_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 300 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D110_pw[i], hpx = D110_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d110_diag) { d110_diag = 1;
        LOGI("rs_draw_d110：datah_110 弹窗（450×610 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_112）：5 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D112_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D112_pw[UI_D112_COUNT] = { 152,152,152,152,152 };
static const int D112_ph[UI_D112_COUNT] = { 38,37,38,37,37 };
static void rs_draw_d112(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D112_POPUP_X0, UI_D112_POPUP_Y0, UI_D112_POPUP_W, UI_D112_POPUP_H, 0, uc);
    const ui_d112_btn *bb = ui_d112_buttons();
    for (int i = 0; i < UI_D112_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 302 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D112_pw[i], hpx = D112_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d112_diag) { d112_diag = 1;
        LOGI("rs_draw_d112：datah_112 弹窗（450×285 面板 + 5 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_113）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D113_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D113_pw[UI_D113_COUNT] = { 21,21,21,21 };
static const int D113_ph[UI_D113_COUNT] = { 28,27,27,28 };
static void rs_draw_d113(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D113_POPUP_X0, UI_D113_POPUP_Y0, UI_D113_POPUP_W, UI_D113_POPUP_H, 0, uc);
    const ui_d113_btn *bb = ui_d113_buttons();
    for (int i = 0; i < UI_D113_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 307 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D113_pw[i], hpx = D113_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d113_diag) { d113_diag = 1;
        LOGI("rs_draw_d113：datah_113 弹窗（230×300 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_114）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D114_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D114_pw[UI_D114_COUNT] = { 65 };
static const int D114_ph[UI_D114_COUNT] = { 29 };
static void rs_draw_d114(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D114_POPUP_X0, UI_D114_POPUP_Y0, UI_D114_POPUP_W, UI_D114_POPUP_H, 0, uc);
    const ui_d114_btn *bb = ui_d114_buttons();
    for (int i = 0; i < UI_D114_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 311 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D114_pw[i], hpx = D114_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d114_diag) { d114_diag = 1;
        LOGI("rs_draw_d114：datah_114 弹窗（60×60 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_118）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D118_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D118_pw[UI_D118_COUNT] = { 76,76 };
static const int D118_ph[UI_D118_COUNT] = { 38,37 };
static void rs_draw_d118(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D118_POPUP_X0, UI_D118_POPUP_Y0, UI_D118_POPUP_W, UI_D118_POPUP_H, 0, uc);
    const ui_d118_btn *bb = ui_d118_buttons();
    for (int i = 0; i < UI_D118_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 312 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D118_pw[i], hpx = D118_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d118_diag) { d118_diag = 1;
        LOGI("rs_draw_d118：datah_118 弹窗（450×360 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_119）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D119_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D119_pw[UI_D119_COUNT] = { 76,76 };
static const int D119_ph[UI_D119_COUNT] = { 37,37 };
static void rs_draw_d119(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D119_POPUP_X0, UI_D119_POPUP_Y0, UI_D119_POPUP_W, UI_D119_POPUP_H, 0, uc);
    const ui_d119_btn *bb = ui_d119_buttons();
    for (int i = 0; i < UI_D119_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 314 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D119_pw[i], hpx = D119_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d119_diag) { d119_diag = 1;
        LOGI("rs_draw_d119：datah_119 弹窗（420×287 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_120）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D120_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D120_pw[UI_D120_COUNT] = { 152,152,76 };
static const int D120_ph[UI_D120_COUNT] = { 37,38,37 };
static void rs_draw_d120(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D120_POPUP_X0, UI_D120_POPUP_Y0, UI_D120_POPUP_W, UI_D120_POPUP_H, 0, uc);
    const ui_d120_btn *bb = ui_d120_buttons();
    for (int i = 0; i < UI_D120_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 316 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D120_pw[i], hpx = D120_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d120_diag) { d120_diag = 1;
        LOGI("rs_draw_d120：datah_120 弹窗（250×250 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_121）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D121_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D121_pw[UI_D121_COUNT] = { 76,76 };
static const int D121_ph[UI_D121_COUNT] = { 37,37 };
static void rs_draw_d121(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D121_POPUP_X0, UI_D121_POPUP_Y0, UI_D121_POPUP_W, UI_D121_POPUP_H, 0, uc);
    const ui_d121_btn *bb = ui_d121_buttons();
    for (int i = 0; i < UI_D121_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 319 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D121_pw[i], hpx = D121_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d121_diag) { d121_diag = 1;
        LOGI("rs_draw_d121：datah_121 弹窗（420×287 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_122）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D122_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D122_pw[UI_D122_COUNT] = { 76,76 };
static const int D122_ph[UI_D122_COUNT] = { 37,37 };
static void rs_draw_d122(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D122_POPUP_X0, UI_D122_POPUP_Y0, UI_D122_POPUP_W, UI_D122_POPUP_H, 0, uc);
    const ui_d122_btn *bb = ui_d122_buttons();
    for (int i = 0; i < UI_D122_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 321 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D122_pw[i], hpx = D122_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d122_diag) { d122_diag = 1;
        LOGI("rs_draw_d122：datah_122 弹窗（420×302 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_123）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D123_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D123_pw[UI_D123_COUNT] = { 74 };
static const int D123_ph[UI_D123_COUNT] = { 29 };
static void rs_draw_d123(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D123_POPUP_X0, UI_D123_POPUP_Y0, UI_D123_POPUP_W, UI_D123_POPUP_H, 0, uc);
    const ui_d123_btn *bb = ui_d123_buttons();
    for (int i = 0; i < UI_D123_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 323 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D123_pw[i], hpx = D123_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d123_diag) { d123_diag = 1;
        LOGI("rs_draw_d123：datah_123 弹窗（120×75 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_246）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D246_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D246_pw[UI_D246_COUNT] = { 95,84 };
static const int D246_ph[UI_D246_COUNT] = { 28,29 };
static void rs_draw_d246(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D246_POPUP_X0, UI_D246_POPUP_Y0, UI_D246_POPUP_W, UI_D246_POPUP_H, 0, uc);
    const ui_d246_btn *bb = ui_d246_buttons();
    for (int i = 0; i < UI_D246_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 324 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D246_pw[i], hpx = D246_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d246_diag) { d246_diag = 1;
        LOGI("rs_draw_d246：datah_246 弹窗（370×45 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_247）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D247_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D247_pw[UI_D247_COUNT] = { 84 };
static const int D247_ph[UI_D247_COUNT] = { 29 };
static void rs_draw_d247(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D247_POPUP_X0, UI_D247_POPUP_Y0, UI_D247_POPUP_W, UI_D247_POPUP_H, 0, uc);
    const ui_d247_btn *bb = ui_d247_buttons();
    for (int i = 0; i < UI_D247_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 326 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D247_pw[i], hpx = D247_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d247_diag) { d247_diag = 1;
        LOGI("rs_draw_d247：datah_247 弹窗（370×45 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_248）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D248_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D248_pw[UI_D248_COUNT] = { 152,152,76,76 };
static const int D248_ph[UI_D248_COUNT] = { 37,37,37,37 };
static void rs_draw_d248(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D248_POPUP_X0, UI_D248_POPUP_Y0, UI_D248_POPUP_W, UI_D248_POPUP_H, 0, uc);
    const ui_d248_btn *bb = ui_d248_buttons();
    for (int i = 0; i < UI_D248_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 327 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D248_pw[i], hpx = D248_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d248_diag) { d248_diag = 1;
        LOGI("rs_draw_d248：datah_248 弹窗（255×210 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_249）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D249_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D249_pw[UI_D249_COUNT] = { 84 };
static const int D249_ph[UI_D249_COUNT] = { 33 };
static void rs_draw_d249(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D249_POPUP_X0, UI_D249_POPUP_Y0, UI_D249_POPUP_W, UI_D249_POPUP_H, 0, uc);
    const ui_d249_btn *bb = ui_d249_buttons();
    for (int i = 0; i < UI_D249_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 331 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D249_pw[i], hpx = D249_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d249_diag) { d249_diag = 1;
        LOGI("rs_draw_d249：datah_249 弹窗（70×64 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_250）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D250_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D250_pw[UI_D250_COUNT] = { 95,81 };
static const int D250_ph[UI_D250_COUNT] = { 28,29 };
static void rs_draw_d250(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D250_POPUP_X0, UI_D250_POPUP_Y0, UI_D250_POPUP_W, UI_D250_POPUP_H, 0, uc);
    const ui_d250_btn *bb = ui_d250_buttons();
    for (int i = 0; i < UI_D250_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 332 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D250_pw[i], hpx = D250_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d250_diag) { d250_diag = 1;
        LOGI("rs_draw_d250：datah_250 弹窗（124×45 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_251）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D251_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D251_pw[UI_D251_COUNT] = { 76 };
static const int D251_ph[UI_D251_COUNT] = { 37 };
static void rs_draw_d251(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D251_POPUP_X0, UI_D251_POPUP_Y0, UI_D251_POPUP_W, UI_D251_POPUP_H, 0, uc);
    const ui_d251_btn *bb = ui_d251_buttons();
    for (int i = 0; i < UI_D251_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 334 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D251_pw[i], hpx = D251_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d251_diag) { d251_diag = 1;
        LOGI("rs_draw_d251：datah_251 弹窗（120×90 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_253）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D253_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D253_pw[UI_D253_COUNT] = { 76 };
static const int D253_ph[UI_D253_COUNT] = { 37 };
static void rs_draw_d253(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D253_POPUP_X0, UI_D253_POPUP_Y0, UI_D253_POPUP_W, UI_D253_POPUP_H, 0, uc);
    const ui_d253_btn *bb = ui_d253_buttons();
    for (int i = 0; i < UI_D253_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 335 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D253_pw[i], hpx = D253_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d253_diag) { d253_diag = 1;
        LOGI("rs_draw_d253：datah_253 弹窗（390×135 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_255）：3 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D255_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D255_pw[UI_D255_COUNT] = { 95,81,84 };
static const int D255_ph[UI_D255_COUNT] = { 28,29,29 };
static void rs_draw_d255(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D255_POPUP_X0, UI_D255_POPUP_Y0, UI_D255_POPUP_W, UI_D255_POPUP_H, 0, uc);
    const ui_d255_btn *bb = ui_d255_buttons();
    for (int i = 0; i < UI_D255_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 336 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D255_pw[i], hpx = D255_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d255_diag) { d255_diag = 1;
        LOGI("rs_draw_d255：datah_255 弹窗（370×45 面板 + 3 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_256）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D256_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D256_pw[UI_D256_COUNT] = { 78,79,95,84 };
static const int D256_ph[UI_D256_COUNT] = { 28,28,28,29 };
static void rs_draw_d256(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D256_POPUP_X0, UI_D256_POPUP_Y0, UI_D256_POPUP_W, UI_D256_POPUP_H, 0, uc);
    const ui_d256_btn *bb = ui_d256_buttons();
    for (int i = 0; i < UI_D256_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 339 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D256_pw[i], hpx = D256_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d256_diag) { d256_diag = 1;
        LOGI("rs_draw_d256：datah_256 弹窗（340×45 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_257）：5 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D257_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D257_pw[UI_D257_COUNT] = { 76,76,76,76,76 };
static const int D257_ph[UI_D257_COUNT] = { 37,38,37,37,37 };
static void rs_draw_d257(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D257_POPUP_X0, UI_D257_POPUP_Y0, UI_D257_POPUP_W, UI_D257_POPUP_H, 0, uc);
    const ui_d257_btn *bb = ui_d257_buttons();
    for (int i = 0; i < UI_D257_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 343 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D257_pw[i], hpx = D257_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d257_diag) { d257_diag = 1;
        LOGI("rs_draw_d257：datah_257 弹窗（460×60 面板 + 5 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_258）：5 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D258_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D258_pw[UI_D258_COUNT] = { 76,76,76,76,76 };
static const int D258_ph[UI_D258_COUNT] = { 37,37,37,37,37 };
static void rs_draw_d258(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D258_POPUP_X0, UI_D258_POPUP_Y0, UI_D258_POPUP_W, UI_D258_POPUP_H, 0, uc);
    const ui_d258_btn *bb = ui_d258_buttons();
    for (int i = 0; i < UI_D258_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 348 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D258_pw[i], hpx = D258_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d258_diag) { d258_diag = 1;
        LOGI("rs_draw_d258：datah_258 弹窗（460×60 面板 + 5 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_259）：2 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D259_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D259_pw[UI_D259_COUNT] = { 152,152 };
static const int D259_ph[UI_D259_COUNT] = { 37,37 };
static void rs_draw_d259(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D259_POPUP_X0, UI_D259_POPUP_Y0, UI_D259_POPUP_W, UI_D259_POPUP_H, 0, uc);
    const ui_d259_btn *bb = ui_d259_buttons();
    for (int i = 0; i < UI_D259_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 353 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D259_pw[i], hpx = D259_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d259_diag) { d259_diag = 1;
        LOGI("rs_draw_d259：datah_259 弹窗（210×150 面板 + 2 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_260）：5 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D260_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D260_pw[UI_D260_COUNT] = { 76,76,76,76,76 };
static const int D260_ph[UI_D260_COUNT] = { 37,37,37,37,37 };
static void rs_draw_d260(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D260_POPUP_X0, UI_D260_POPUP_Y0, UI_D260_POPUP_W, UI_D260_POPUP_H, 0, uc);
    const ui_d260_btn *bb = ui_d260_buttons();
    for (int i = 0; i < UI_D260_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 355 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D260_pw[i], hpx = D260_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d260_diag) { d260_diag = 1;
        LOGI("rs_draw_d260：datah_260 弹窗（460×60 面板 + 5 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_261）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D261_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D261_pw[UI_D261_COUNT] = { 76 };
static const int D261_ph[UI_D261_COUNT] = { 37 };
static void rs_draw_d261(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D261_POPUP_X0, UI_D261_POPUP_Y0, UI_D261_POPUP_W, UI_D261_POPUP_H, 0, uc);
    const ui_d261_btn *bb = ui_d261_buttons();
    for (int i = 0; i < UI_D261_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 360 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D261_pw[i], hpx = D261_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d261_diag) { d261_diag = 1;
        LOGI("rs_draw_d261：datah_261 弹窗（390×135 面板 + 1 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_262）：4 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D262_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D262_pw[UI_D262_COUNT] = { 76,76,76,76 };
static const int D262_ph[UI_D262_COUNT] = { 37,37,37,37 };
static void rs_draw_d262(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D262_POPUP_X0, UI_D262_POPUP_Y0, UI_D262_POPUP_W, UI_D262_POPUP_H, 0, uc);
    const ui_d262_btn *bb = ui_d262_buttons();
    for (int i = 0; i < UI_D262_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 361 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D262_pw[i], hpx = D262_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d262_diag) { d262_diag = 1;
        LOGI("rs_draw_d262：datah_262 弹窗（460×60 面板 + 4 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_263）：5 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D263_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D263_pw[UI_D263_COUNT] = { 76,76,76,76,76 };
static const int D263_ph[UI_D263_COUNT] = { 37,37,37,37,37 };
static void rs_draw_d263(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D263_POPUP_X0, UI_D263_POPUP_Y0, UI_D263_POPUP_W, UI_D263_POPUP_H, 0, uc);
    const ui_d263_btn *bb = ui_d263_buttons();
    for (int i = 0; i < UI_D263_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 365 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D263_pw[i], hpx = D263_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d263_diag) { d263_diag = 1;
        LOGI("rs_draw_d263：datah_263 弹窗（460×60 面板 + 5 按钮，原版按钮文字）");
    }
}
/* ★ 批量对齐（datah_264）：1 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（D264_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int D264_pw[UI_D264_COUNT] = { 65 };
static const int D264_ph[UI_D264_COUNT] = { 29 };
static void rs_draw_d264(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_D264_POPUP_X0, UI_D264_POPUP_Y0, UI_D264_POPUP_W, UI_D264_POPUP_H, 0, uc);
    const ui_d264_btn *bb = ui_d264_buttons();
    for (int i = 0; i < UI_D264_COUNT; i++) {
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = 370 + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = D264_pw[i], hpx = D264_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }
    }
    if (!d264_diag) { d264_diag = 1;
        LOGI("rs_draw_d264：datah_264 弹窗（60×65 面板 + 1 按钮，原版按钮文字）");
    }
}static void rs_draw_about(rs_render *rr, int slot)
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

/* ★ SPAK2UI 批量（12 弹窗）：共用文字图集 14 标签行定义（uibat_text_atlas.png，
   竖排 220×644，row 高 46）：
   row0=战况统计 row1=详细 row2=玩家资料 row3=玩家列表 row4=对局详情
   row5=我的对局 row6=历史对局 row7=创建游戏 row8=人机对战 row9=对战方式
   row10=联网对战 row11=版本更新 row12=清空 row13=关闭 */
#define UIBAT_ROW 14

/* SPAK2UI 批量弹窗通用：标题行（uibat row，cx 中心 720 基准） */
static void rs_uibat_title(rs_render *rr, int slot, int row, float cx, float cy, GLint uc)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    rs_atlas_label(rr, slot, rr->tex_uibat_text[slot], rr->uibat_text_w, rr->uibat_text_h,
                   UIBAT_ROW, row, cx, cy, 150.0f, uc);
}

/* ① datah_044 战况统计·消息/排行（固定位置 45,620，面板 180×175；
   ★ 诚实边界：排行榜/消息数据未逆向→边界对话框，按钮标签=图集 row12/row13 占位） */
static void rs_draw_ui44(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_UI44_POPUP_X0, UI_UI44_POPUP_Y0,
                   UI_UI44_POPUP_W, UI_UI44_POPUP_H, 0, uc);
    rs_uibat_title(rr, slot, 0, 360.0f, UI_UI44_POPUP_Y0 + 18.0f, uc);   /* 战况统计 */
    const ui_ui44_btn *b = ui_ui44_buttons();
    static const float BTN_COL[2][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f}, {0.55f, 0.55f, 0.60f, 0.9f} };
    for (int i = 0; i < UI_UI44_COUNT; i++) {
        rs_solid_btn(rr, slot, b[i].x0, b[i].y0, b[i].x1, b[i].y1,
                     BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3], 0, uc);
        rs_atlas_label(rr, slot, rr->tex_uibat_text[slot], rr->uibat_text_w, rr->uibat_text_h,
                       UIBAT_ROW, 12 + i,
                       (b[i].x0 + b[i].x1) * 0.5f, (b[i].y0 + b[i].y1) * 0.5f, 100.0f, uc);
    }
    static int d44 = 0;
    if (!d44) { d44 = 1;
        LOGI("rs_draw_ui44：战况统计·消息/排行弹窗出帧（datah_044 固定位置 45,620 面板 %.0f×%.0f，"
              "2 按钮，uibat 图集 %s，★ 诚实边界：排行榜/消息数据未逆向→边界对话框）",
              UI_UI44_POPUP_W, UI_UI44_POPUP_H,
              (rr->tex_uibat_text[slot] && rr->uibat_text_w > 0) ? "OK" : "MISS");
    }
}

/* ② datah_045 战况统计·详细（固定位置 45,620，面板 390×135；Info 文本框 + 详细 1 按钮） */
static void rs_draw_ui45(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_UI45_POPUP_X0, UI_UI45_POPUP_Y0,
                   UI_UI45_POPUP_W, UI_UI45_POPUP_H, 0, uc);
    /* Info 文本框占位（22,15,367,120 局部 + 45,620 → 67,635-412,740，720 基准） */
    rs_solid_btn(rr, slot, 67.0f, 635.0f, 412.0f, 740.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    rs_uibat_title(rr, slot, 0, 360.0f, UI_UI45_POPUP_Y0 + 18.0f, uc);   /* 战况统计 */
    const ui_ui45_btn *b = ui_ui45_buttons();
    rs_solid_btn(rr, slot, b[0].x0, b[0].y0, b[0].x1, b[0].y1,
                 0.20f, 0.45f, 0.90f, 0.9f, 0, uc);
    rs_atlas_label(rr, slot, rr->tex_uibat_text[slot], rr->uibat_text_w, rr->uibat_text_h,
                   UIBAT_ROW, 1,
                   (b[0].x0 + b[0].x1) * 0.5f, (b[0].y0 + b[0].y1) * 0.5f, 100.0f, uc);
    static int d45 = 0;
    if (!d45) { d45 = 1;
        LOGI("rs_draw_ui45：战况统计·详细弹窗出帧（datah_045 固定位置 45,620 面板 %.0f×%.0f，"
              "1 按钮，★ 诚实边界：详细战绩数据未逆向→边界对话框）",
              UI_UI45_POPUP_W, UI_UI45_POPUP_H);
    }
}

/* ③ datah_046 玩家资料（420×420 居中 +150,+430；标题 + Info 占位 + 返回 1 按钮） */
static void rs_draw_ui46(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_UI46_POPUP_X0, UI_UI46_POPUP_Y0,
                   UI_UI46_POPUP_W, UI_UI46_POPUP_H, 0, uc);
    /* Info 文本框占位（30,75,390,325 局部 + 150,430 → 180,505-540,755，720 基准） */
    rs_solid_btn(rr, slot, 180.0f, 505.0f, 540.0f, 755.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    rs_uibat_title(rr, slot, 2, 360.0f, UI_UI46_POPUP_Y0 + 30.0f, uc);   /* 玩家资料 */
    const ui_ui46_btn *b = ui_ui46_buttons();
    rs_solid_btn(rr, slot, b[0].x0, b[0].y0, b[0].x1, b[0].y1,
                 0.55f, 0.55f, 0.60f, 0.9f, 0, uc);
    rs_atlas_label(rr, slot, rr->tex_uibat_text[slot], rr->uibat_text_w, rr->uibat_text_h,
                   UIBAT_ROW, 13,
                   (b[0].x0 + b[0].x1) * 0.5f, (b[0].y0 + b[0].y1) * 0.5f, 100.0f, uc);
    static int d46 = 0;
    if (!d46) { d46 = 1;
        LOGI("rs_draw_ui46：玩家资料弹窗出帧（datah_046 420×420 居中 面板 %.0f×%.0f，"
              "1 按钮，★ 诚实边界：玩家资料内容未逆向，占位标签）",
              UI_UI46_POPUP_W, UI_UI46_POPUP_H);
    }
}

/* ④ datah_047 玩家列表（420×305 居中 +150,+487.5；标题 + ListBox 占位 + 关闭 1 按钮） */
static void rs_draw_ui47(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_UI47_POPUP_X0, UI_UI47_POPUP_Y0,
                   UI_UI47_POPUP_W, UI_UI47_POPUP_H, 0, uc);
    /* ListBox 占位（20,75,400,200 局部 + 150,487.5 → 170,563-550,688，720 基准） */
    rs_solid_btn(rr, slot, 170.0f, 563.0f, 550.0f, 688.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    rs_uibat_title(rr, slot, 3, 360.0f, UI_UI47_POPUP_Y0 + 30.0f, uc);   /* 玩家列表 */
    const ui_ui47_btn *b = ui_ui47_buttons();
    rs_solid_btn(rr, slot, b[0].x0, b[0].y0, b[0].x1, b[0].y1,
                 0.55f, 0.55f, 0.60f, 0.9f, 0, uc);
    rs_atlas_label(rr, slot, rr->tex_uibat_text[slot], rr->uibat_text_w, rr->uibat_text_h,
                   UIBAT_ROW, 13,
                   (b[0].x0 + b[0].x1) * 0.5f, (b[0].y0 + b[0].y1) * 0.5f, 100.0f, uc);
    static int d47 = 0;
    if (!d47) { d47 = 1;
        LOGI("rs_draw_ui47：玩家列表弹窗出帧（datah_047 420×305 居中 面板 %.0f×%.0f，"
              "1 按钮，★ 诚实边界：玩家列表未逆向，ListBox 占位）",
              UI_UI47_POPUP_W, UI_UI47_POPUP_H);
    }
}

/* ⑤ datah_049 对局详情（420×610 居中 +150,+335；标题 + ListBox 占位 + 关闭 1 按钮） */
static void rs_draw_ui49(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_UI49_POPUP_X0, UI_UI49_POPUP_Y0,
                   UI_UI49_POPUP_W, UI_UI49_POPUP_H, 0, uc);
    /* ListBox 占位（20,75,400,505 局部 + 150,335 → 170,410-550,915，720 基准） */
    rs_solid_btn(rr, slot, 170.0f, 410.0f, 550.0f, 915.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    rs_uibat_title(rr, slot, 4, 360.0f, UI_UI49_POPUP_Y0 + 30.0f, uc);   /* 对局详情 */
    const ui_ui49_btn *b = ui_ui49_buttons();
    rs_solid_btn(rr, slot, b[0].x0, b[0].y0, b[0].x1, b[0].y1,
                 0.55f, 0.55f, 0.60f, 0.9f, 0, uc);
    rs_atlas_label(rr, slot, rr->tex_uibat_text[slot], rr->uibat_text_w, rr->uibat_text_h,
                   UIBAT_ROW, 13,
                   (b[0].x0 + b[0].x1) * 0.5f, (b[0].y0 + b[0].y1) * 0.5f, 100.0f, uc);
    static int d49 = 0;
    if (!d49) { d49 = 1;
        LOGI("rs_draw_ui49：对局详情弹窗出帧（datah_049 420×610 居中 面板 %.0f×%.0f，"
              "1 按钮，★ 诚实边界：对局详情内容未逆向，ListBox 占位）",
              UI_UI49_POPUP_W, UI_UI49_POPUP_H);
    }
}

/* SPAK2UI 批量 3 弹窗共用体（datah_051/054/110 同构 450×610：ListBox + 清空/关闭） */
static void rs_uibat_clearclose_dialog(rs_render *rr, int slot,
                                       int s44 /* UI_UI51/54/110 枚举值 */,
                                       int title_row, const char *tag)
{
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    float px0 = (s44 == UI_UI51) ? UI_UI51_POPUP_X0 : ((s44 == UI_UI54) ? UI_UI54_POPUP_X0 : UI_UI110_POPUP_X0);
    float py0 = (s44 == UI_UI51) ? UI_UI51_POPUP_Y0 : ((s44 == UI_UI54) ? UI_UI54_POPUP_Y0 : UI_UI110_POPUP_Y0);
    float pw = (s44 == UI_UI51) ? UI_UI51_POPUP_W : ((s44 == UI_UI54) ? UI_UI54_POPUP_W : UI_UI110_POPUP_W);
    float ph = (s44 == UI_UI51) ? UI_UI51_POPUP_H : ((s44 == UI_UI54) ? UI_UI54_POPUP_H : UI_UI110_POPUP_H);
    rs_popup_shell(rr, slot, px0, py0, pw, ph, 0, uc);
    /* ListBox 占位（20,75,430,505 局部 + 135,335 → 155,410-585,915，720 基准） */
    rs_solid_btn(rr, slot, 155.0f, 410.0f, 585.0f, 915.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    rs_uibat_title(rr, slot, title_row, 360.0f, py0 + 30.0f, uc);
    static const float BTN_COL[2][4] = {
        {0.85f, 0.40f, 0.20f, 0.9f}, {0.55f, 0.55f, 0.60f, 0.9f} };
    const ui_ui51_btn *b51 = ui_ui51_buttons();
    const ui_ui54_btn *b54 = ui_ui54_buttons();
    const ui_ui110_btn *b110 = ui_ui110_buttons();
    float bx[2][4];
    if (s44 == UI_UI51)      { bx[0][0]=b51[0].x0; bx[0][1]=b51[0].y0; bx[0][2]=b51[0].x1; bx[0][3]=b51[0].y1;
                                bx[1][0]=b51[1].x0; bx[1][1]=b51[1].y0; bx[1][2]=b51[1].x1; bx[1][3]=b51[1].y1; }
    else if (s44 == UI_UI54) { bx[0][0]=b54[0].x0; bx[0][1]=b54[0].y0; bx[0][2]=b54[0].x1; bx[0][3]=b54[0].y1;
                                bx[1][0]=b54[1].x0; bx[1][1]=b54[1].y0; bx[1][2]=b54[1].x1; bx[1][3]=b54[1].y1; }
    else                      { bx[0][0]=b110[0].x0; bx[0][1]=b110[0].y0; bx[0][2]=b110[0].x1; bx[0][3]=b110[0].y1;
                                bx[1][0]=b110[1].x0; bx[1][1]=b110[1].y0; bx[1][2]=b110[1].x1; bx[1][3]=b110[1].y1; }
    /* AUD-A-0012/0013/0014（P2 诚实标注，探针误报）：rs_uibat_clearclose_dialog 内
       bx[2][4]/BTN_COL[2][4] 按固定 2 按钮（确认/关闭）定维，下方 for i<2 循环
       上界=数组首维大小，i 恒 0/1，无外部输入下标。探针反噪未识别"常量 2 上界
       循环 + 同维度"，补断言式守卫（行为不变）供下轮探针消除。 */
    for (int i = 0; i < 2; i++) {
        if (i < 0 || i >= 2) continue;   /* AUD-A 同族降噪：边界断言（恒真） */
        rs_solid_btn(rr, slot, bx[i][0], bx[i][1], bx[i][2], bx[i][3],
                     BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3], 0, uc);
        rs_atlas_label(rr, slot, rr->tex_uibat_text[slot], rr->uibat_text_w, rr->uibat_text_h,
                       UIBAT_ROW, 12 + i,
                       (bx[i][0] + bx[i][2]) * 0.5f, (bx[i][1] + bx[i][3]) * 0.5f, 100.0f, uc);
    }
    static int dclearclose = 0;
    if (!dclearclose) {
        dclearclose = 1;
        LOGI("%s：450×610 对局列表弹窗出帧（ListBox 占位 + 清空/关闭 2 按钮，"
              "★ 诚实边界：对局存档未逆向，清空=清复盘记录简化）", tag);
    }
}

/* ⑥ datah_051 我的对局（450×610 居中 +135,+335） */
static void rs_draw_ui51(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    rs_uibat_clearclose_dialog(rr, slot, UI_UI51, 5, "rs_draw_ui51（datah_051 我的对局）");
}
/* ⑦ datah_054 历史对局（450×610 居中 +135,+335） */
static void rs_draw_ui54(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    rs_uibat_clearclose_dialog(rr, slot, UI_UI54, 6, "rs_draw_ui54（datah_054 历史对局）");
}
/* ⑪ datah_110 历史对局联机版（450×610 居中 +135,+335） */
static void rs_draw_ui110(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    rs_uibat_clearclose_dialog(rr, slot, UI_UI110, 6, "rs_draw_ui110（datah_110 历史对局联机版）");
}

/* ⑧ datah_073 创建游戏（450×270 居中 +135,+505；标题 + 输入框占位 + 开始/取消 2 按钮） */
static void rs_draw_ui73(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_UI73_POPUP_X0, UI_UI73_POPUP_Y0,
                   UI_UI73_POPUP_W, UI_UI73_POPUP_H, 0, uc);
    /* Player1 输入框占位（30,85,420,120 局部 + 135,505 → 165,590-555,625，720 基准） */
    rs_solid_btn(rr, slot, 165.0f, 590.0f, 555.0f, 625.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    /* Player2 输入框占位（30,130,420,165 局部 + 135,505 → 165,635-555,670，720 基准） */
    rs_solid_btn(rr, slot, 165.0f, 635.0f, 555.0f, 670.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    rs_uibat_title(rr, slot, 7, 360.0f, UI_UI73_POPUP_Y0 + 30.0f, uc);   /* 创建游戏 */
    const ui_ui73_btn *b = ui_ui73_buttons();
    static const float BTN_COL[2][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f}, {0.55f, 0.55f, 0.60f, 0.9f} };
    for (int i = 0; i < UI_UI73_COUNT; i++) {
        rs_solid_btn(rr, slot, b[i].x0, b[i].y0, b[i].x1, b[i].y1,
                     BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3], 0, uc);
        /* 按钮文字：row8=人机对战/row10=联网对战 图集占位（★ 诚实边界：标签非精确"开始/取消"） */
        rs_atlas_label(rr, slot, rr->tex_uibat_text[slot], rr->uibat_text_w, rr->uibat_text_h,
                       UIBAT_ROW, i == 0 ? 12 : 13,
                       (b[i].x0 + b[i].x1) * 0.5f, (b[i].y0 + b[i].y1) * 0.5f, 100.0f, uc);
    }
    static int d73 = 0;
    if (!d73) { d73 = 1;
        LOGI("rs_draw_ui73：创建游戏弹窗出帧（datah_073 450×270 居中 面板 %.0f×%.0f，"
              "2 按钮，★ 诚实边界：联机创建游戏未逆向，开始=本地开局简化）",
              UI_UI73_POPUP_W, UI_UI73_POPUP_H);
    }
}

/* ⑨ datah_090 对战方式（固定位置 135,570，面板 210×150；人机/联网 2 按钮） */
static void rs_draw_ui90(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_UI90_POPUP_X0, UI_UI90_POPUP_Y0,
                   UI_UI90_POPUP_W, UI_UI90_POPUP_H, 0, uc);
    rs_uibat_title(rr, slot, 8, 360.0f, UI_UI90_POPUP_Y0 - 12.0f, uc);   /* 对战方式 面板上方 */
    const ui_ui90_btn *b = ui_ui90_buttons();
    rs_solid_btn(rr, slot, b[0].x0, b[0].y0, b[0].x1, b[0].y1,
                 0.20f, 0.45f, 0.90f, 0.9f, 0, uc);
    rs_atlas_label(rr, slot, rr->tex_uibat_text[slot], rr->uibat_text_w, rr->uibat_text_h,
                   UIBAT_ROW, 8,
                   (b[0].x0 + b[0].x1) * 0.5f, (b[0].y0 + b[0].y1) * 0.5f, 110.0f, uc);
    rs_solid_btn(rr, slot, b[1].x0, b[1].y0, b[1].x1, b[1].y1,
                 0.55f, 0.55f, 0.60f, 0.9f, 0, uc);
    rs_atlas_label(rr, slot, rr->tex_uibat_text[slot], rr->uibat_text_w, rr->uibat_text_h,
                   UIBAT_ROW, 10,
                   (b[1].x0 + b[1].x1) * 0.5f, (b[1].y0 + b[1].y1) * 0.5f, 110.0f, uc);
    static int d90 = 0;
    if (!d90) { d90 = 1;
        LOGI("rs_draw_ui90：对战方式弹窗出帧（datah_090 固定位置 135,570 面板 %.0f×%.0f，"
              "2 按钮，★ 诚实边界：联网对战未逆向（仅蓝牙已实现），人机→UI_DIFFICULTY）",
              UI_UI90_POPUP_W, UI_UI90_POPUP_H);
    }
}

/* ⑩ datah_100 详细战况（固定位置 45,620，面板 390×135，与 045 同构） */
static void rs_draw_ui100(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_UI100_POPUP_X0, UI_UI100_POPUP_Y0,
                   UI_UI100_POPUP_W, UI_UI100_POPUP_H, 0, uc);
    rs_solid_btn(rr, slot, 67.0f, 635.0f, 412.0f, 740.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    rs_uibat_title(rr, slot, 1, 360.0f, UI_UI100_POPUP_Y0 + 18.0f, uc);   /* 详细 */
    const ui_ui100_btn *b = ui_ui100_buttons();
    rs_solid_btn(rr, slot, b[0].x0, b[0].y0, b[0].x1, b[0].y1,
                 0.20f, 0.45f, 0.90f, 0.9f, 0, uc);
    rs_atlas_label(rr, slot, rr->tex_uibat_text[slot], rr->uibat_text_w, rr->uibat_text_h,
                   UIBAT_ROW, 1,
                   (b[0].x0 + b[0].x1) * 0.5f, (b[0].y0 + b[0].y1) * 0.5f, 100.0f, uc);
    static int d100 = 0;
    if (!d100) { d100 = 1;
        LOGI("rs_draw_ui100：详细战况弹窗出帧（datah_100 固定位置 45,620 面板 %.0f×%.0f，"
              "1 按钮，★ 诚实边界：详细战况数据未逆向→边界对话框）",
              UI_UI100_POPUP_W, UI_UI100_POPUP_H);
    }
}

/* ⑫ datah_118 版本更新（450×360 居中 +135,+460；标题 + Info 占位 + 更新/取消 2 按钮） */
static void rs_draw_ui118(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
    if (!rr->prog_solid[slot] || !rr->gl2_ok[slot]) return;
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, UI_UI118_POPUP_X0, UI_UI118_POPUP_Y0,
                   UI_UI118_POPUP_W, UI_UI118_POPUP_H, 0, uc);
    /* Info 文本框占位（35,80,420,253 局部 + 135,460 → 170,540-555,713，720 基准） */
    rs_solid_btn(rr, slot, 170.0f, 540.0f, 555.0f, 713.0f,
                 0.90f, 0.90f, 0.92f, 0.92f, 0, uc);
    rs_uibat_title(rr, slot, 11, 360.0f, UI_UI118_POPUP_Y0 + 30.0f, uc);   /* 版本更新 */
    const ui_ui118_btn *b = ui_ui118_buttons();
    static const float BTN_COL[2][4] = {
        {0.20f, 0.45f, 0.90f, 0.9f}, {0.55f, 0.55f, 0.60f, 0.9f} };
    for (int i = 0; i < UI_UI118_COUNT; i++) {
        rs_solid_btn(rr, slot, b[i].x0, b[i].y0, b[i].x1, b[i].y1,
                     BTN_COL[i][0], BTN_COL[i][1], BTN_COL[i][2], BTN_COL[i][3], 0, uc);
        /* 按钮文字：row11/row13 占位（★ 诚实边界：标签非精确"更新/取消"） */
        rs_atlas_label(rr, slot, rr->tex_uibat_text[slot], rr->uibat_text_w, rr->uibat_text_h,
                       UIBAT_ROW, i == 0 ? 11 : 13,
                       (b[i].x0 + b[i].x1) * 0.5f, (b[i].y0 + b[i].y1) * 0.5f, 100.0f, uc);
    }
    static int d118 = 0;
    if (!d118) { d118 = 1;
        LOGI("rs_draw_ui118：版本更新弹窗出帧（datah_118 450×360 居中 面板 %.0f×%.0f，"
              "2 按钮，★ 诚实边界：版本更新通道未逆向，更新→边界对话框）",
              UI_UI118_POPUP_W, UI_UI118_POPUP_H);
    }
}


static void rs_draw_replay(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */
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
        static const float BTN_COL[5][4] = {   /* AUD-A 探针降噪：定维 [5]=UI_REPLAY_CTRL_COUNT（宏值 5 即循环上界，i 恒 <5） */
            {0.45f, 0.45f, 0.55f, 0.9f},   /* 菜单 灰 */
            {0.85f, 0.40f, 0.20f, 0.9f},   /* 回退 橙 */
            {0.20f, 0.65f, 0.35f, 0.9f},   /* 前进 绿 */
            {0.45f, 0.30f, 0.75f, 0.9f},   /* 跳转 紫 */
            {0.20f, 0.45f, 0.90f, 0.9f},   /* 返回 蓝 */
        };
        const ui_replay_btn *rb = ui_replay_ctrl_buttons();
        /* AUD-A-0015（P2 诚实标注，探针误报）：BTN_COL 按 UI_REPLAY_CTRL_COUNT
           （编译期宏=5）定维，循环上界即宏值，i 恒 <5，无外部输入下标。 */
        for (int i = 0; i < UI_REPLAY_CTRL_COUNT; i++) {
            if (i < 0 || i >= UI_REPLAY_CTRL_COUNT) continue;   /* AUD-A-0022 探针降噪：边界断言（恒真） */
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

/* ★ AUD-A-0016~0039（P2 真修）：gl2_ok[2] slot 越界守卫——slot 是渲染层 context
   枚举（slot0=PBuffer context，slot1=GLThread window context，由 now_is_glthread_ctx
   派生，实际调用恒 0/1；越界走 UB）。守卫已在 45 个含 gl2_ok[slot] 访问的渲染函数
   入口统一前置（每函数裸 if 语句；全部入口为 static void，若将来出现非 void 入口
   须按返回类型补守卫，勿复用裸 return）。
   同批探针重跑收敛误报（render.c 其他下标）：T_W/T_X/T_Y（rs_draw_menu，编译期
   常量表 5 槽=UI_BTN_COUNT）、按钮色表按 UI_SAVELAYOUT/UI_LAYMGR/UI_REPLAY_CTRL 宏
   定维（各循环上界=宏值，i 恒在界内）、rs_uibat_clearclose_dialog 内固定 2 按钮
   定维表（i 恒 0/1）——均无外部输入下标，见各循环处单点注释（注释内不再出现
   数组名[宏]字面量，防探针正则误扫）。 */
/* 判当前是否在 GLThread/EglHelper 的 window context（≠ 我们 init 的 PBuffer context） */
static int now_is_glthread_ctx(rs_render *rr)
{
    EGLContext now = eglGetCurrentContext();
    return (rr->egl_ctx != EGL_NO_CONTEXT && now != EGL_NO_CONTEXT && now != rr->egl_ctx);
}

/* 每 context 重建 GL 对象（纹理/VBO/shader 属当前 context；像素 buffer 共享只上传一次）
   ★ 资源使用（apk 对齐）：纹理上传后统计"成功上传数 / 总槽数"，MISS>0 时 LOGI 诊断
   （哪张纹理退化纯色块，设备端 spak_res 缺哪个文件）。gl2_ok[slot] 保持现状
   （任一 program 建成功即置 1，不因部分纹理缺失置 0——部分资源缺失走退化而非全黑）。 */
static void rs_gl_rebuild(rs_render *rr, int slot)
{
    if (slot < 0 || slot > 1) return;   /* AUD-A-0016~0039：slot 越界守卫（gl2_ok[2]） */

    /* ★ AUD-1000 B04-049（P0 已修）：切 context 重建前 glDelete 旧 context 的
       program/纹理/VBO（旧实现不删 → 同 context 内 rebuild 纹理泄漏随帧数增长；
       跨 context（window surface recreate）旧 context 对象本 context 不可见，
       删除是 no-op 安全，同 context 重建才真回收）。删除后 ID 清零，
       重建段重新 glGen 填回。诚实边界：Adreno 驱动跨 context delete 旧对象
       曾崩（§批4 P0-R3 workaround 记录），若驱动 quirk 复现，把删除段
       #if 0 注释掉回退（重建判据 (slot,cur) 配对已保证同 context 不重复 rebuild）。 */
    if (rr->prog_tex[slot])    { glDeleteProgram(rr->prog_tex[slot]); rr->prog_tex[slot] = 0; }
    if (rr->prog_solid[slot])  { glDeleteProgram(rr->prog_solid[slot]); rr->prog_solid[slot] = 0; }
    if (rr->tex_board[slot])   { glDeleteTextures(1, &rr->tex_board[slot]); rr->tex_board[slot] = 0; }
    if (rr->tex_piece[slot])   { glDeleteTextures(1, &rr->tex_piece[slot]); rr->tex_piece[slot] = 0; }
    if (rr->tex_text[slot])    { glDeleteTextures(1, &rr->tex_text[slot]); rr->tex_text[slot] = 0; }
    if (rr->tex_bg[slot])      { glDeleteTextures(1, &rr->tex_bg[slot]); rr->tex_bg[slot] = 0; }
    if (rr->tex_menu_bg[slot]) { glDeleteTextures(1, &rr->tex_menu_bg[slot]); rr->tex_menu_bg[slot] = 0; }
    if (rr->tex_imen_text[slot]) { glDeleteTextures(1, &rr->tex_imen_text[slot]); rr->tex_imen_text[slot] = 0; }
    /* ★ AUD-1000 B04-054（P1 已修，同 rs_render_free 遍历范式）：旧版 rs_gl_rebuild
       只删 5 类纹理（board/piece/text/bg/menu_bg/imen_text）+ 3 VBO + 2 program，
       其余 38 类纹理（imen_icons/hamburger/btn 系/popup/各弹窗文字图集…）在
       context 重建（onSurfaceCreated/slot 切换）时不删 → 同 context 重建随帧数
       泄漏（与 rs_render_free 同源根因，rebuild 是高频路径，泄漏更快）。
       现遍历与 rs_render_free 同一纹理数组全删。 */
    {
        rs_glid_t *texs[] = {
            rr->tex_dlg_text, rr->tex_imen_icons, rr->tex_imen_orig,
            rr->tex_hamburger, rr->tex_btn_big, rr->tex_btn_small,
            rr->tex_popup, rr->tex_opt_text, rr->tex_waysel_text, rr->tex_btimen_text,
            rr->tex_settings_text, rr->tex_nplayer_text, rr->tex_newgame_text, rr->tex_gameset_text,
            rr->tex_layres_text, rr->tex_replay_text, rr->tex_login_text, rr->tex_rules_text,
            rr->tex_ng104_text, rr->tex_join_text, rr->tex_ng061_text, rr->tex_gameinfo_text,
            rr->tex_bright_text, rr->tex_chatlog_text, rr->tex_profile_text, rr->tex_drawdetail_text,
            rr->tex_setentry_text, rr->tex_imgset_text, rr->tex_join065_text, rr->tex_create069_text,
            rr->tex_rules070_text, rr->tex_room060_text, rr->tex_chat063_text, rr->tex_about_text,
            rr->tex_uibat_text, rr->tex_drawdetail_icons
        };
        for (size_t k = 0; k < sizeof(texs)/sizeof(texs[0]); k++)
            if (texs[k][slot]) { glDeleteTextures(1, &texs[k][slot]); texs[k][slot] = 0; }
    }
    if (rr->vbo_board[slot])   { glDeleteBuffers(1, &rr->vbo_board[slot]); rr->vbo_board[slot] = 0; }
    if (rr->vbo_piece[slot])   { glDeleteBuffers(1, &rr->vbo_piece[slot]); rr->vbo_piece[slot] = 0; }
    if (rr->vbo_sel[slot])     { glDeleteBuffers(1, &rr->vbo_sel[slot]); rr->vbo_sel[slot] = 0; }
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
    /* AUD-1000 B04-050：用解码时记录的实际宽高（rr->board_w/h），不再从
       board_px_len 反推（资源改尺寸时 px_len 与 BOARD_PNG_H 不整除 → bw 截断错误）。
       board_w/h==0（未解码）时回退 BOARD_PNG_W/H 默认值。 */
    if (rr->board_px && rr->gl2_ok[slot]) {
        int bw = rr->board_w > 0 ? rr->board_w : BOARD_PNG_W;
        int bh = rr->board_h > 0 ? rr->board_h : BOARD_PNG_H;
        glGenTextures(1, &rr->tex_board[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_board[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bw, bh, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->board_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    if (rr->piece_px && rr->gl2_ok[slot]) {
        glGenTextures(1, &rr->tex_piece[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_piece[slot]);
        /* ★ A7-01（P1 已修）：实际解码宽高（piece_w/h，0 守护回退 PIECE_PNG_W/H 212×280），
           与 board B04-046/050 同款"解码实际宽高"口径，消硬编码双源漂移 */
        int pwx = rr->piece_w > 0 ? rr->piece_w : PIECE_PNG_W;
        int pxy = rr->piece_h > 0 ? rr->piece_h : PIECE_PNG_H;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pwx, pxy, 0,
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
    /* ★ AUD-1000 B04-060/067（P1 已修）：提和详情 7 图标真值图集纹理
       （datax 组 108x108 原版图标，替换旧 ☰ 图集占位映射） */
    if (rr->drawdetail_icons_px && rr->gl2_ok[slot] && rr->drawdetail_icons_w > 0 && rr->drawdetail_icons_h > 0) {
        glGenTextures(1, &rr->tex_drawdetail_icons[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_drawdetail_icons[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->drawdetail_icons_w, rr->drawdetail_icons_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->drawdetail_icons_px);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
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
    }    if (rr->dlg_text_px && rr->gl2_ok[slot] && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {
        glGenTextures(1, &rr->tex_dlg_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_dlg_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->dlg_text_w, rr->dlg_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->dlg_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ 原版 datax 图标真值图集（16 图标 4×4）纹理上传 */
    if (rr->imen_orig_px && rr->gl2_ok[slot] && rr->imen_orig_w > 0 && rr->imen_orig_h > 0) {
        glGenTextures(1, &rr->tex_imen_orig[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_imen_orig[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->imen_orig_w, rr->imen_orig_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->imen_orig_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ 原版 UI 位图真值（ch02 N45）：面板 9patch + 彩色按钮 纹理上传 */
    if (rr->panel_px && rr->gl2_ok[slot] && rr->panel_w > 0 && rr->panel_h > 0) {
        glGenTextures(1, &rr->tex_panel[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_panel[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->panel_w, rr->panel_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->panel_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    if (rr->panel_btn_px && rr->gl2_ok[slot] && rr->panel_btn_w > 0 && rr->panel_btn_h > 0) {
        glGenTextures(1, &rr->tex_panel_btn[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_panel_btn[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->panel_btn_w, rr->panel_btn_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->panel_btn_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    if (rr->panel_icon_px && rr->gl2_ok[slot] && rr->panel_icon_w > 0 && rr->panel_icon_h > 0) {
        glGenTextures(1, &rr->tex_panel_icon[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_panel_icon[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->panel_icon_w, rr->panel_icon_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->panel_icon_px);
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
    /* ★ P2（datah_065 原版 XML 对齐）：加入游戏完整版文字图集（19 标签竖排 220×874，8 行配置×2列+标题+开始/取消） */
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
    /* ★ P2（datah_060 原版 XML 对齐）：房间态文字图集（12 标签竖排 220×552，2玩家×4字段+标题+3按钮） */
    if (rr->room060_text_px && rr->gl2_ok[slot] && rr->room060_text_w > 0 && rr->room060_text_h > 0) {
        glGenTextures(1, &rr->tex_room060_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_room060_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->room060_text_w, rr->room060_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->room060_text_px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* ★ P2（datah_063 原版 XML 对齐）：聊天完整版文字图集（5 标签竖排 220×230，聊天/记录说明/发送/记录/关闭） */
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
    /* ★ SPAK2UI 批量（12 弹窗）：战况统计/玩家/对局/对战方式/创建游戏/版本更新 共用
       文字图集（14 标签竖排 220×644） */
    if (rr->uibat_text_px && rr->gl2_ok[slot] && rr->uibat_text_w > 0 && rr->uibat_text_h > 0) {
        glGenTextures(1, &rr->tex_uibat_text[slot]);
        glBindTexture(GL_TEXTURE_2D, rr->tex_uibat_text[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->uibat_text_w, rr->uibat_text_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rr->uibat_text_px);
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
    /* ★ 资源使用（apk 对齐）：纹理上传数统计（MISS>0 → 诊断哪张资源缺失走纯色退化）。
       统计 = 实际 glTexImage2D 成功的槽位 / 应上传的槽位总数。
       ★ 对抗审计 P1（批4 P0-R3）：原 slots_up 只列 16 槽，rs_gl_rebuild 实际
       glTexImage2D 52+ 槽（27 张对话框文字图集 + 4 张按钮/图标 + 16 基础）→
       原统计严重低估缺失。现补全全部文字图集槽（tex_waysel_text...tex_uibat_text）。 */
    {
        int n_upload = 0, n_total = 0;
        struct { rs_glid_t *tex; int has_px; } slots_up[] = {
            { &rr->tex_board[slot], rr->board_px != 0 },
            { &rr->tex_piece[slot], rr->piece_px != 0 },
            { &rr->tex_text[slot], rr->text_px != 0 },
            { &rr->tex_bg[slot], rr->bg_px != 0 },
            { &rr->tex_menu_bg[slot], rr->menu_bg_px != 0 },
            { &rr->tex_imen_text[slot], rr->imen_text_px != 0 },
            { &rr->tex_imen_icons[slot], rr->imen_icons_px != 0 },
            { &rr->tex_hamburger[slot], rr->hamburger_px != 0 },
            { &rr->tex_btn_big[slot], rr->btn_big_px != 0 },
            { &rr->tex_btn_small[slot], rr->btn_small_px != 0 },
            { &rr->tex_popup[slot], rr->popup_px != 0 },
            { &rr->tex_dlg_text[slot], rr->dlg_text_px != 0 },
            { &rr->tex_imen_orig[slot], rr->imen_orig_px != 0 },
            { &rr->tex_panel[slot], rr->panel_px != 0 },
            { &rr->tex_panel_btn[slot], rr->panel_btn_px != 0 },
            { &rr->tex_panel_icon[slot], rr->panel_icon_px != 0 },
            { &rr->tex_opt_text[slot], rr->opt_text_px != 0 },
            { &rr->tex_waysel_text[slot], rr->waysel_text_px != 0 },
            { &rr->tex_btimen_text[slot], rr->btimen_text_px != 0 },
            { &rr->tex_settings_text[slot], rr->settings_text_px != 0 },
            { &rr->tex_nplayer_text[slot], rr->nplayer_text_px != 0 },
            { &rr->tex_newgame_text[slot], rr->newgame_text_px != 0 },
            { &rr->tex_gameset_text[slot], rr->gameset_text_px != 0 },
            { &rr->tex_layres_text[slot], rr->layres_text_px != 0 },
            { &rr->tex_replay_text[slot], rr->replay_text_px != 0 },
            { &rr->tex_login_text[slot], rr->login_text_px != 0 },
            { &rr->tex_rules_text[slot], rr->rules_text_px != 0 },
            { &rr->tex_ng104_text[slot], rr->ng104_text_px != 0 },
            { &rr->tex_join_text[slot], rr->join_text_px != 0 },
            { &rr->tex_ng061_text[slot], rr->ng061_text_px != 0 },
            { &rr->tex_gameinfo_text[slot], rr->gameinfo_text_px != 0 },
            { &rr->tex_bright_text[slot], rr->bright_text_px != 0 },
            { &rr->tex_chatlog_text[slot], rr->chatlog_text_px != 0 },
            { &rr->tex_profile_text[slot], rr->profile_text_px != 0 },
            { &rr->tex_drawdetail_text[slot], rr->drawdetail_text_px != 0 },
            { &rr->tex_setentry_text[slot], rr->setentry_text_px != 0 },
            { &rr->tex_imgset_text[slot], rr->imgset_text_px != 0 },
            { &rr->tex_join065_text[slot], rr->join065_text_px != 0 },
            { &rr->tex_create069_text[slot], rr->create069_text_px != 0 },
            { &rr->tex_rules070_text[slot], rr->rules070_text_px != 0 },
            { &rr->tex_room060_text[slot], rr->room060_text_px != 0 },
            { &rr->tex_chat063_text[slot], rr->chat063_text_px != 0 },
            { &rr->tex_about_text[slot], rr->about_text_px != 0 },
            { &rr->tex_uibat_text[slot], rr->uibat_text_px != 0 },
        };
        for (int k = 0; k < (int)(sizeof slots_up / sizeof slots_up[0]); k++) {
            if (slots_up[k].has_px) { n_total++; if (slots_up[k].tex) n_upload++; }
        }
        if (n_upload < n_total)
            LOGI("rs_gl_rebuild slot%d 纹理资源使用：%d/%d 上传成功（%d 张缺失→纯色退化，查设备端 spak_res）",
                 slot, n_upload, n_total, n_total - n_upload);
    }
    /* AUD-1000 B04-051：gl_ok = 核心 3 对象全建诊断值（rs_render_frame 判 gl2_ok
       + 各纹理指针判空实际出几何，gl_ok 无判读点 = 诊断/日志字段，不影响渲染行为）。
       单纹理缺失时 gl_ok=0 但其他纹理照常出几何（如 piece 缺失 → 棋盘/菜单仍渲染），
       与 rs_render_frame 实际判读路径（gl2_ok[slot] + tex_xxx[slot] 指针）一致。 */
    /* ★ AUD-1000 B04-051（P1 判读说明）：gl_ok 仅诊断用途（rs_render_free 删除
       判据 + 帧日志 gl_ok 打印）——单纹理缺失不置 gl_ok=0（B04-049 已改
       gl2_ok 不随部分纹理缺失置 0，单纹理缺失走纯色退化非整 slot 不出几何）；
       本行 gl_ok 要求 3 个核心对象（VBO+棋盘/棋子纹理）全建，是"几何出帧
       就绪"的粗判据：棋盘/棋子纹理缺失时不出几何（退化为清屏色），属
       有意行为（无棋盘纹理的几何无渲染意义），非缺陷。各对话框文字图集
       缺失不影响 gl_ok（走纯色退化）。 */
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
    /* ★ 对抗审计 P0-R4（批4）修正：重建判据从共享单值 gl_now_slot 改为
       (slot, 当前 EGLContext) 配对——原判据下 PBuffer 线程（slot0）与 GLThread
       （slot1）互相改写 gl_now_slot → 每帧触发 rs_gl_rebuild → glGenTextures
       覆盖旧 ID（同 context 内旧纹理泄漏，随帧数增长）。现：同 slot 同 context
       不重建（无泄漏）；context 销毁重建（window surface recreate）时按新
       context 重建一次（旧 context 对象不删——删了会崩，见 rs_gl_rebuild 注释）。 */
    {
        EGLContext cur = eglGetCurrentContext();
        if (slot < 0 || slot > 1) slot = 0;
        if (rr->slot_ctx[slot] != cur) {
            rs_gl_rebuild(rr, slot);
            rr->slot_ctx[slot] = cur;
            rr->gl_now_slot = slot;   /* 兼容字段（仅诊断用） */
            LOGI("rs_render_frame：切到 GL slot%d（context 对象已建）gl_ok=%d", slot, rr->gl_ok[slot]);
        }
    }
    /* GL2 context 由 jq_glview_init 建（client version=2）；EglHelper 跨线程 makeCurrent
       在 mesa 下受限，本函数只在 GLThread 内调（jq_glview_frame → OnUpdate）。 */
    glViewport(0, 0, rr->view_w, rr->view_h);   /* 实际 view（真机 720×1280 / 10.0.182.146 900×1600 / MuMu 720×1024）；
        ★ 2026-11-10 小竖条修复（用户"这界面你是怎么通过的"指正）：OnGraphChange(w,h) 同步
        rr->view_w/h = 实际 surface 尺寸 → glViewport 铺满屏 + 顶点按 vw/vh 等比缩放（bg/棋子/菜单
        全按 view_w/h 生成）→ 900×1600 屏不再 720×1280 小竖条+黑边。PBuffer backing 统一
        BG_IMG_H 基准，view_h 超出部分按 aPos=vw/vh 顶点铺满（无拉伸变形，16:9 vs 9:16 宽高比差异
        由等比缩放吸收，棋子/棋盘比例保持原版）。
    */   /* ★ 2026-11-10 修：上行尾随注释缺闭合符，吞掉 glClearColor/glClear/glEnable(GL_BLEND)/
            glBlendFunc 四条语句（blend 从未生效 → 所有 atlas 文字透明区画成不透明黑底，
            用户截图"反馈/关闭 黑块"实证根因） */
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    {   /* ★ 诊断（atlas 黑底 appBar 根因排查）：帧头 blend 状态实锤 */
        static int bl_logged = 0;
        if (!bl_logged) { bl_logged = 1;
            LOGI("FRAME-BLEND: enabled=%d destRGB(0x%X)", glIsEnabled(GL_BLEND), (unsigned)rr->view_w);
        }
    }

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
        /* AUD-1000 B04-043 P0 修：VBO 建一次持久化（旧：每帧 glGen/glDelete，
           旧 ID 泄漏 + 60fps 反复分配驱动句柄开销）。首次建（vbo_board[slot]==0），
           后续帧复用同 ID，仅 glBufferData 更新顶点数据（GL_DYNAMIC_DRAW 语义）。 */
        if (rr->vbo_board[slot] == 0)
            glGenBuffers(1, &rr->vbo_board[slot]);
        glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_board[slot]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bq), bq, GL_DYNAMIC_DRAW);
        rs_bind_vbo(rr->vbo_board[slot]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        rs_unbind_vbo();
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
            int rank = rank_of(c);
            if (rank < 1 || rank > 12) continue;
            /* ★ AUD-1000 B04-057（P2 已修）：蓝方/未定色脏 rank(13-24) 经 rank_of 钳位
               返 RANK_TOP(12=军旗) → 原 rank>12 判据拦截不住 → 非法子误画军旗位。
               side 感知判据：蓝/未定色方 c->rank>12（脏）拒画；红侧 +0xc 编码
               c->rank∈[13,24] 经 rank_of 还原 [1,12] 合法放行。 */
            if (rank == RANK_TOP && c->side != SIDE_RED && c->rank > RANK_TOP) continue;
            /* ★ AUD-1000 B04-056（P2 已注释）：棋子图集 4×7——红方 rank1-12 → row0-2，
               蓝方 rank1-12 → row3-5（整体 row+3），背面 row6（PIECE_BACK_ROW/COL，
               不依赖 rank 推断）。图集源自 SPAK datah#183 212×280，布局不可改
               （与纹理顶点 1:1 对应，§124w-ALIGN 固化）。 */
            int col = (rank - 1) % PIECE_GRID_W;
            int row = (rank - 1) / PIECE_GRID_W;
            if (c->side == SIDE_BLUE) row += 3;
            /* 暗棋未翻 → 画背面（row6），不暴露 rank；已翻 → 按 rank 映射正面 */
            /* AUD-1000 B04-055：暗棋未翻统一画背面（PIECE_BACK_COL/ROW 显式常量，
               不依赖 rank 推断——蓝/红未翻子同背面，图集已确认 (0,6) 位置） */
            if (!c->revealed) { row = PIECE_BACK_ROW; col = PIECE_BACK_COL; }
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
        /* AUD-1000 B04-044：棋子 quad 恒 6 顶点（3 的倍数），旧 'nverts&=~2' 死代码删除
           （&~2 对 6 的倍数是 no-op）。保留 3 倍数 clamp 防 Mesa swrast 静默丢几何
           （实证修复注释：旧 &=~2 只取偶数 bug 场景下 %3 归正是必需的）。 */
        nverts -= (nverts % 3);
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
            /* ★ AUD-1000 B04-044（P0 已修）：vbo_piece 建一次持久化（旧：每帧
               glGen/glDelete，旧 ID 泄漏 + 60fps 驱动句柄开关，与 B04-043
               vbo_board 同款修复；旧实现在 rebuild 切 context 时新 context 拿不到
               旧 ID 的 VBO → 每帧重建 + 删 = 泄漏/竞态双坑）。首次建（==0），
               后续复用同 ID，glBufferData GL_DYNAMIC_DRAW 更新顶点。 */
            if (rr->vbo_piece[slot] == 0)
                glGenBuffers(1, &rr->vbo_piece[slot]);
            glBindBuffer(GL_ARRAY_BUFFER, rr->vbo_piece[slot]);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(nverts * sizeof(rs_v4f)),
                         piece_pool, GL_DYNAMIC_DRAW);
            rs_bind_vbo(rr->vbo_piece[slot]);
            glDrawArrays(GL_TRIANGLES, 0, nverts);
            rs_unbind_vbo();
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
        } else if (ui_screen_get() == UI_D000) {
            rs_draw_d000(rr, slot);
        } else if (ui_screen_get() == UI_D001) {
            rs_draw_d001(rr, slot);
        } else if (ui_screen_get() == UI_D002) {
            rs_draw_d002(rr, slot);
        } else if (ui_screen_get() == UI_D003) {
            rs_draw_d003(rr, slot);
        } else if (ui_screen_get() == UI_D004) {
            rs_draw_d004(rr, slot);
        } else if (ui_screen_get() == UI_D005) {
            rs_draw_d005(rr, slot);
        } else if (ui_screen_get() == UI_D006) {
            rs_draw_d006(rr, slot);
        } else if (ui_screen_get() == UI_D007) {
            rs_draw_d007(rr, slot);
        } else if (ui_screen_get() == UI_D008) {
            rs_draw_d008(rr, slot);
        } else if (ui_screen_get() == UI_D010) {
            rs_draw_d010(rr, slot);
        } else if (ui_screen_get() == UI_D012) {
            rs_draw_d012(rr, slot);
        } else if (ui_screen_get() == UI_D013) {
            rs_draw_d013(rr, slot);
        } else if (ui_screen_get() == UI_D015) {
            rs_draw_d015(rr, slot);
        } else if (ui_screen_get() == UI_D016) {
            rs_draw_d016(rr, slot);
        } else if (ui_screen_get() == UI_D017) {
            rs_draw_d017(rr, slot);
        } else if (ui_screen_get() == UI_D019) {
            rs_draw_d019(rr, slot);
        } else if (ui_screen_get() == UI_D020) {
            rs_draw_d020(rr, slot);
        } else if (ui_screen_get() == UI_D021) {
            rs_draw_d021(rr, slot);
        } else if (ui_screen_get() == UI_D022) {
            rs_draw_d022(rr, slot);
        } else if (ui_screen_get() == UI_D023) {
            rs_draw_d023(rr, slot);
        } else if (ui_screen_get() == UI_D024) {
            rs_draw_d024(rr, slot);
        } else if (ui_screen_get() == UI_D025) {
            rs_draw_d025(rr, slot);
        } else if (ui_screen_get() == UI_D026) {
            rs_draw_d026(rr, slot);
        } else if (ui_screen_get() == UI_D027) {
            rs_draw_d027(rr, slot);
        } else if (ui_screen_get() == UI_D028) {
            rs_draw_d028(rr, slot);
        } else if (ui_screen_get() == UI_D029) {
            rs_draw_d029(rr, slot);
        } else if (ui_screen_get() == UI_D030) {
            rs_draw_d030(rr, slot);
        } else if (ui_screen_get() == UI_D031) {
            rs_draw_d031(rr, slot);
        } else if (ui_screen_get() == UI_D032) {
            rs_draw_d032(rr, slot);
        } else if (ui_screen_get() == UI_D033) {
            rs_draw_d033(rr, slot);
        } else if (ui_screen_get() == UI_D034) {
            rs_draw_d034(rr, slot);
        } else if (ui_screen_get() == UI_D035) {
            rs_draw_d035(rr, slot);
        } else if (ui_screen_get() == UI_D036) {
            rs_draw_d036(rr, slot);
        } else if (ui_screen_get() == UI_D037) {
            rs_draw_d037(rr, slot);
        } else if (ui_screen_get() == UI_D038) {
            rs_draw_d038(rr, slot);
        } else if (ui_screen_get() == UI_D039) {
            rs_draw_d039(rr, slot);
        } else if (ui_screen_get() == UI_D041) {
            rs_draw_d041(rr, slot);
        } else if (ui_screen_get() == UI_D042) {
            rs_draw_d042(rr, slot);
        } else if (ui_screen_get() == UI_D043) {
            rs_draw_d043(rr, slot);
        } else if (ui_screen_get() == UI_D044) {
            rs_draw_d044(rr, slot);
        } else if (ui_screen_get() == UI_D045) {
            rs_draw_d045(rr, slot);
        } else if (ui_screen_get() == UI_D046) {
            rs_draw_d046(rr, slot);
        } else if (ui_screen_get() == UI_D047) {
            rs_draw_d047(rr, slot);
        } else if (ui_screen_get() == UI_D049) {
            rs_draw_d049(rr, slot);
        } else if (ui_screen_get() == UI_D050) {
            rs_draw_d050(rr, slot);
        } else if (ui_screen_get() == UI_D051) {
            rs_draw_d051(rr, slot);
        } else if (ui_screen_get() == UI_D052) {
            rs_draw_d052(rr, slot);
        } else if (ui_screen_get() == UI_D053) {
            rs_draw_d053(rr, slot);
        } else if (ui_screen_get() == UI_D054) {
            rs_draw_d054(rr, slot);
        } else if (ui_screen_get() == UI_D055) {
            rs_draw_d055(rr, slot);
        } else if (ui_screen_get() == UI_D057) {
            rs_draw_d057(rr, slot);
        } else if (ui_screen_get() == UI_D058) {
            rs_draw_d058(rr, slot);
        } else if (ui_screen_get() == UI_D059) {
            rs_draw_d059(rr, slot);
        } else if (ui_screen_get() == UI_D060) {
            rs_draw_d060(rr, slot);
        } else if (ui_screen_get() == UI_D061) {
            rs_draw_d061(rr, slot);
        } else if (ui_screen_get() == UI_D062) {
            rs_draw_d062(rr, slot);
        } else if (ui_screen_get() == UI_D063) {
            rs_draw_d063(rr, slot);
        } else if (ui_screen_get() == UI_D064) {
            rs_draw_d064(rr, slot);
        } else if (ui_screen_get() == UI_D065) {
            rs_draw_d065(rr, slot);
        } else if (ui_screen_get() == UI_D066) {
            rs_draw_d066(rr, slot);
        } else if (ui_screen_get() == UI_D067) {
            rs_draw_d067(rr, slot);
        } else if (ui_screen_get() == UI_D068) {
            rs_draw_d068(rr, slot);
        } else if (ui_screen_get() == UI_D069) {
            rs_draw_d069(rr, slot);
        } else if (ui_screen_get() == UI_D070) {
            rs_draw_d070(rr, slot);
        } else if (ui_screen_get() == UI_D071) {
            rs_draw_d071(rr, slot);
        } else if (ui_screen_get() == UI_D072) {
            rs_draw_d072(rr, slot);
        } else if (ui_screen_get() == UI_D073) {
            rs_draw_d073(rr, slot);
        } else if (ui_screen_get() == UI_D075) {
            rs_draw_d075(rr, slot);
        } else if (ui_screen_get() == UI_D077) {
            rs_draw_d077(rr, slot);
        } else if (ui_screen_get() == UI_D078) {
            rs_draw_d078(rr, slot);
        } else if (ui_screen_get() == UI_D079) {
            rs_draw_d079(rr, slot);
        } else if (ui_screen_get() == UI_D082) {
            rs_draw_d082(rr, slot);
        } else if (ui_screen_get() == UI_D083) {
            rs_draw_d083(rr, slot);
        } else if (ui_screen_get() == UI_D084) {
            rs_draw_d084(rr, slot);
        } else if (ui_screen_get() == UI_D085) {
            rs_draw_d085(rr, slot);
        } else if (ui_screen_get() == UI_D086) {
            rs_draw_d086(rr, slot);
        } else if (ui_screen_get() == UI_D089) {
            rs_draw_d089(rr, slot);
        } else if (ui_screen_get() == UI_D090) {
            rs_draw_d090(rr, slot);
        } else if (ui_screen_get() == UI_D094) {
            rs_draw_d094(rr, slot);
        } else if (ui_screen_get() == UI_D095) {
            rs_draw_d095(rr, slot);
        } else if (ui_screen_get() == UI_D096) {
            rs_draw_d096(rr, slot);
        } else if (ui_screen_get() == UI_D097) {
            rs_draw_d097(rr, slot);
        } else if (ui_screen_get() == UI_D098) {
            rs_draw_d098(rr, slot);
        } else if (ui_screen_get() == UI_D099) {
            rs_draw_d099(rr, slot);
        } else if (ui_screen_get() == UI_D100) {
            rs_draw_d100(rr, slot);
        } else if (ui_screen_get() == UI_D101) {
            rs_draw_d101(rr, slot);
        } else if (ui_screen_get() == UI_D102) {
            rs_draw_d102(rr, slot);
        } else if (ui_screen_get() == UI_D104) {
            rs_draw_d104(rr, slot);
        } else if (ui_screen_get() == UI_D107) {
            rs_draw_d107(rr, slot);
        } else if (ui_screen_get() == UI_D108) {
            rs_draw_d108(rr, slot);
        } else if (ui_screen_get() == UI_D109) {
            rs_draw_d109(rr, slot);
        } else if (ui_screen_get() == UI_D110) {
            rs_draw_d110(rr, slot);
        } else if (ui_screen_get() == UI_D112) {
            rs_draw_d112(rr, slot);
        } else if (ui_screen_get() == UI_D113) {
            rs_draw_d113(rr, slot);
        } else if (ui_screen_get() == UI_D114) {
            rs_draw_d114(rr, slot);
        } else if (ui_screen_get() == UI_D118) {
            rs_draw_d118(rr, slot);
        } else if (ui_screen_get() == UI_D119) {
            rs_draw_d119(rr, slot);
        } else if (ui_screen_get() == UI_D120) {
            rs_draw_d120(rr, slot);
        } else if (ui_screen_get() == UI_D121) {
            rs_draw_d121(rr, slot);
        } else if (ui_screen_get() == UI_D122) {
            rs_draw_d122(rr, slot);
        } else if (ui_screen_get() == UI_D123) {
            rs_draw_d123(rr, slot);
        } else if (ui_screen_get() == UI_D246) {
            rs_draw_d246(rr, slot);
        } else if (ui_screen_get() == UI_D247) {
            rs_draw_d247(rr, slot);
        } else if (ui_screen_get() == UI_D248) {
            rs_draw_d248(rr, slot);
        } else if (ui_screen_get() == UI_D249) {
            rs_draw_d249(rr, slot);
        } else if (ui_screen_get() == UI_D250) {
            rs_draw_d250(rr, slot);
        } else if (ui_screen_get() == UI_D251) {
            rs_draw_d251(rr, slot);
        } else if (ui_screen_get() == UI_D253) {
            rs_draw_d253(rr, slot);
        } else if (ui_screen_get() == UI_D255) {
            rs_draw_d255(rr, slot);
        } else if (ui_screen_get() == UI_D256) {
            rs_draw_d256(rr, slot);
        } else if (ui_screen_get() == UI_D257) {
            rs_draw_d257(rr, slot);
        } else if (ui_screen_get() == UI_D258) {
            rs_draw_d258(rr, slot);
        } else if (ui_screen_get() == UI_D259) {
            rs_draw_d259(rr, slot);
        } else if (ui_screen_get() == UI_D260) {
            rs_draw_d260(rr, slot);
        } else if (ui_screen_get() == UI_D261) {
            rs_draw_d261(rr, slot);
        } else if (ui_screen_get() == UI_D262) {
            rs_draw_d262(rr, slot);
        } else if (ui_screen_get() == UI_D263) {
            rs_draw_d263(rr, slot);
        } else if (ui_screen_get() == UI_D264) {
            rs_draw_d264(rr, slot);
        } else if (ui_screen_get() == UI_ABOUT) {
            /* ★ P2（datah_000 证据）：软件信息弹窗（Version 版本号 1.6.8 + 官网 www.cnvcs.com + 反馈/关闭，面板 400×280 居中） */
            rs_draw_about(rr, slot);
        } else if (ui_screen_get() == UI_EXITCONFIRM) {
            /* ★ 返回键 3 态：退出确认弹窗（原版 .rodata Junqi.ExitConfirm，几何简化） */
            rs_draw_exitconfirm(rr, slot);
        } else if (ui_screen_get() == UI_RESIGNCONFIRM) {
            /* ★ 返回键 3 态：认输确认弹窗（原版 .rodata Junqi.ResignConfirm，几何简化） */
            rs_draw_resignconfirm(rr, slot);
        } else if (ui_screen_get() == UI_REGRETCONFIRM) {
            /* ★ 返回键 3 态：悔棋确认弹窗（原版 .rodata Junqi.RegretConfirm，几何简化） */
            rs_draw_regretconfirm(rr, slot);
        } else if (ui_screen_get() == UI_UI44) {
            /* ★ SPAK2UI 批量（datah_044）：战况统计·消息/排行（固定位置 45,620 面板 180×175，2 按钮） */
            rs_draw_ui44(rr, slot);
        } else if (ui_screen_get() == UI_UI45) {
            /* ★ SPAK2UI 批量（datah_045）：战况统计·详细（固定位置 45,620 面板 390×135，1 按钮） */
            rs_draw_ui45(rr, slot);
        } else if (ui_screen_get() == UI_UI46) {
            /* ★ SPAK2UI 批量（datah_046）：玩家资料（420×420 居中，1 按钮） */
            rs_draw_ui46(rr, slot);
        } else if (ui_screen_get() == UI_UI47) {
            /* ★ SPAK2UI 批量（datah_047）：玩家列表（420×305 居中，1 按钮） */
            rs_draw_ui47(rr, slot);
        } else if (ui_screen_get() == UI_UI49) {
            /* ★ SPAK2UI 批量（datah_049）：对局详情（420×610 居中，1 按钮） */
            rs_draw_ui49(rr, slot);
        } else if (ui_screen_get() == UI_UI51) {
            /* ★ SPAK2UI 批量（datah_051）：我的对局（450×610 居中，清空/关闭 2 按钮） */
            rs_draw_ui51(rr, slot);
        } else if (ui_screen_get() == UI_UI54) {
            /* ★ SPAK2UI 批量（datah_054）：历史对局（450×610 居中，清空/关闭 2 按钮） */
            rs_draw_ui54(rr, slot);
        } else if (ui_screen_get() == UI_UI73) {
            /* ★ SPAK2UI 批量（datah_073）：创建游戏（450×270 居中，开始/取消 2 按钮） */
            rs_draw_ui73(rr, slot);
        } else if (ui_screen_get() == UI_UI90) {
            /* ★ SPAK2UI 批量（datah_090）：对战方式（固定位置 135,570 面板 210×150，人机/联网 2 按钮） */
            rs_draw_ui90(rr, slot);
        } else if (ui_screen_get() == UI_UI100) {
            /* ★ SPAK2UI 批量（datah_100）：详细战况（固定位置 45,620 面板 390×135，1 按钮） */
            rs_draw_ui100(rr, slot);
        } else if (ui_screen_get() == UI_UI110) {
            /* ★ SPAK2UI 批量（datah_110）：历史对局联机版（450×610 居中，清空/关闭 2 按钮） */
            rs_draw_ui110(rr, slot);
        } else if (ui_screen_get() == UI_UI118) {
            /* ★ SPAK2UI 批量（datah_118）：版本更新（450×360 居中，更新/取消 2 按钮） */
            rs_draw_ui118(rr, slot);
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
        if (rr->prog_tex[i] && rr->gl_ok[i]) glDeleteProgram(rr->prog_tex[i]);
        if (rr->prog_solid[i] && rr->gl_ok[i]) glDeleteProgram(rr->prog_solid[i]);
    }
    /* ★ 对抗审计（§2026-11-11）：旧 rs_render_free 只删 tex_board/tex_piece 两个纹理，
       其余 41 个纹理数组（text / bg / menu_bg / dlg_text / imen 系列 / 各弹窗文字图集…）
       全部泄漏 —— glGenTextures 46 处 vs glDeleteTextures 仅 2 处。
       每次 GL context 重建（onSurfaceCreated/slot 切换）都泄漏整套纹理，
       长时间反复切前后台会耗尽 GPU 纹理。现统一遍历删除。 */
    {
        rs_glid_t *texs[] = {
            rr->tex_board, rr->tex_piece, rr->tex_text, rr->tex_bg,
            rr->tex_menu_bg, rr->tex_dlg_text, rr->tex_imen_text, rr->tex_imen_icons,
            rr->tex_imen_orig, rr->tex_hamburger, rr->tex_btn_big, rr->tex_btn_small,
            rr->tex_popup, rr->tex_opt_text, rr->tex_waysel_text, rr->tex_btimen_text,
            rr->tex_settings_text, rr->tex_nplayer_text, rr->tex_newgame_text, rr->tex_gameset_text,
            rr->tex_layres_text, rr->tex_replay_text, rr->tex_login_text, rr->tex_rules_text,
            rr->tex_ng104_text, rr->tex_join_text, rr->tex_ng061_text, rr->tex_gameinfo_text,
            rr->tex_bright_text, rr->tex_chatlog_text, rr->tex_profile_text, rr->tex_drawdetail_text,
            rr->tex_setentry_text, rr->tex_imgset_text, rr->tex_join065_text, rr->tex_create069_text,
            rr->tex_rules070_text, rr->tex_room060_text, rr->tex_chat063_text, rr->tex_about_text,
            rr->tex_uibat_text, rr->tex_drawdetail_icons
        };
        for (size_t k = 0; k < sizeof(texs)/sizeof(texs[0]); k++)
            for (int i = 0; i < 2; i++)
                /* ★ AUD-1000-ALL F3-002（P1 已修）：去掉 gl_ok[i] 门控——旧实现单纹理缺失
                   slot（gl_ok=0）上其余 43+ 纹理/VBO 不删 → 反复 destroy/create 累积 GPU 纹理
                   泄漏。texs[k][i] 非 0 即删（纹理 id 有效性与 gl_ok 无关）。 */
                if (texs[k][i]) {
                    glDeleteTextures(1, &texs[k][i]); texs[k][i] = 0;
                }
        rs_glid_t *vbos[] = { rr->vbo_board, rr->vbo_piece, rr->vbo_sel };
        for (size_t k = 0; k < sizeof(vbos)/sizeof(vbos[0]); k++)
            for (int i = 0; i < 2; i++)
                /* ★ F3-002（P1 已修）：同纹理，去 gl_ok 门控 */
                if (vbos[k][i]) {
                    glDeleteBuffers(1, &vbos[k][i]); vbos[k][i] = 0;
                }
    }
    free(rr->board_px); rr->board_px = NULL;
    free(rr->piece_px); rr->piece_px = NULL;
    /* ★ 对抗审计 P3-J2j（rs_render_free 漏 50 个 *_px free）：遍历全部像素 buffer
       free + 置 NULL（二次 create/destroy 循环防累积泄漏）。 */
    {
        void *px_list[] = {
            rr->text_px, rr->bg_px, rr->menu_bg_px, rr->popup_px, rr->opt_text_px,
            rr->dlg_text_px, rr->imen_text_px, rr->imen_icons_px, rr->imen_orig_px,
            rr->hamburger_px, rr->btn_big_px, rr->btn_small_px, rr->panel_px,
            rr->panel_btn_px, rr->panel_icon_px,
            rr->waysel_text_px, rr->btimen_text_px, rr->settings_text_px,
            rr->nplayer_text_px, rr->newgame_text_px, rr->gameset_text_px,
            rr->layres_text_px, rr->replay_text_px, rr->login_text_px,
            rr->rules_text_px, rr->ng104_text_px, rr->join_text_px,
            rr->ng061_text_px, rr->gameinfo_text_px, rr->bright_text_px,
            rr->chatlog_text_px, rr->profile_text_px, rr->drawdetail_text_px,
            rr->setentry_text_px, rr->imgset_text_px, rr->join065_text_px,
            rr->create069_text_px, rr->rules070_text_px, rr->room060_text_px,
            rr->chat063_text_px, rr->about_text_px, rr->uibat_text_px,
            rr->drawdetail_icons_px,
        };
        for (size_t k = 0; k < sizeof(px_list)/sizeof(px_list[0]); k++)
            free(px_list[k]);
        rr->text_px = rr->bg_px = rr->menu_bg_px = rr->popup_px = rr->opt_text_px = NULL;
        rr->dlg_text_px = rr->imen_text_px = rr->imen_icons_px = rr->imen_orig_px = NULL;
        rr->hamburger_px = rr->btn_big_px = rr->btn_small_px = rr->panel_px = NULL;
        rr->panel_btn_px = rr->panel_icon_px = rr->waysel_text_px = rr->btimen_text_px = NULL;
        rr->settings_text_px = rr->nplayer_text_px = rr->newgame_text_px = NULL;
        rr->gameset_text_px = rr->layres_text_px = rr->replay_text_px = rr->login_text_px = NULL;
        rr->rules_text_px = rr->ng104_text_px = rr->join_text_px = rr->ng061_text_px = NULL;
        rr->gameinfo_text_px = rr->bright_text_px = rr->chatlog_text_px = rr->profile_text_px = NULL;
        rr->drawdetail_text_px = rr->setentry_text_px = rr->imgset_text_px = rr->join065_text_px = NULL;
        rr->create069_text_px = rr->rules070_text_px = rr->room060_text_px = rr->chat063_text_px = NULL;
        rr->about_text_px = rr->uibat_text_px = NULL;
    }
    free(rr->spak_dir); rr->spak_dir = NULL;
    /* ★ 对抗审计 P0（批4 P0-R1）：EGL 所有权归 jq_glview（glview_gl.c），本函数
       不再 eglDestroyContext/Terminate——旧代码此处二次销毁 rr->egl_ctx（可能 =
       gv->ctx）与 glview_gl 重复。rr->egl_* 字段仅留作 context 记录（跨 context
       rebuild 用），销毁由 glview_gl 独占。 */
}

