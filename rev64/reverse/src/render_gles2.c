/*
 * render_gles2.c — R4 渲染（arm64 手写路线）
 * 原版 32 位 libjunqi.so 用 GLES1（libGLESv1_CM.so，立即模式 glBegin/glEnd）。
 * 但新 NDK 的 GLES1 头与 libGLESv1_CM.so stub **已移除立即模式符号**（glBegin/glEnd），
 * 无法在 arm64 下编译链接。故本骨架改用 **GLES2**（shader + 顶点缓冲）。
 *
 * 视觉：真实棋盘 PNG（若已通过 junqi_render_set_board_texture 注入）作背景，
 *       否则回退到色块网格（可编译骨架）。棋子用色块（纹理加载在 R5 资源层）。
 */
#include <GLES2/gl2.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>
#include "junqi_core.h"

#define LOG_TAG "junqi64"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static int g_w = 0, g_h = 0;
static GLuint g_prog = 0;
static GLint g_a_pos = -1, g_a_color = -1;
static GLint g_u_tex = -1;
static GLint g_a_uv = -1;
static GLint g_u_has_tex = -1;
static GLuint g_board_tex = 0;      /* 棋盘纹理（真实 PNG，可选） */
static int g_board_w = 0, g_board_h = 0;
static GLuint g_piece_tex = 0;      /* 棋子图集（原版 datah[183]，212×280，4列×7行，53×40/格） */
static int g_piece_w = 0, g_piece_h = 0;  /* 图集宽高（212×280） */
static int g_selected = -1;         /* 选中格（高亮），-1=无 */
static int g_setup = 0;             /* 布阵模式：缩小棋盘留确认按钮 */



static const char *VS =
    "attribute vec2 a_pos; attribute vec3 a_color; attribute vec2 a_uv;"
    "varying vec3 v_color; varying vec2 v_uv;"
    "void main(){ gl_Position=vec4(a_pos,0.0,1.0); v_color=a_color; v_uv=a_uv; }";
static const char *FS =
    "precision mediump float; varying vec3 v_color; varying vec2 v_uv;"
    "uniform sampler2D u_tex; uniform int u_has_tex;"
    "void main(){ if(u_has_tex==1){ gl_FragColor=texture2D(u_tex,v_uv); }"
    "else { gl_FragColor=vec4(v_color,1.0); } }";

static GLuint compile_shader(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) return 0;
    return s;
}

void junqi_render_init(int w, int h) {
    g_w = w; g_h = h;
    glViewport(0, 0, w, h);
    GLuint vs = compile_shader(GL_VERTEX_SHADER, VS);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, FS);
    g_prog = glCreateProgram();
    glAttachShader(g_prog, vs);
    glAttachShader(g_prog, fs);
    glLinkProgram(g_prog);
    glUseProgram(g_prog);
    g_a_pos = glGetAttribLocation(g_prog, "a_pos");
    g_a_color = glGetAttribLocation(g_prog, "a_color");
    g_u_tex = glGetUniformLocation(g_prog, "u_tex");
    g_a_uv = glGetAttribLocation(g_prog, "a_uv");
    g_u_has_tex = glGetUniformLocation(g_prog, "u_has_tex");
}

/* 注入真实棋盘纹理（RGBA 解码后的 PNG），替换色块背景 */
void junqi_render_set_board_texture(const unsigned char *rgba, int w, int h) {
    if (!rgba || w <= 0 || h <= 0) return;
    if (!g_board_tex) glGenTextures(1, &g_board_tex);
    glBindTexture(GL_TEXTURE_2D, g_board_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    g_board_w = w; g_board_h = h;
}

/* 设置选中格（-1=无），渲染时高亮 */
void junqi_render_set_selected(int idx) { g_selected = idx; }

/* 注入棋子图集（原版 datah[183]，212×280，4列×7行，53×40/格；rank→列 (rank-1)%4、子行 (rank-1)/4，
   红(side=1)=行0-2、蓝(side=0)=行3-5；row6=背面(绿)/选中(橙/蓝)/空(黑)——见 REVERSE_NOTES §85） */
void junqi_render_set_piece_atlas(const unsigned char *rgba, int w, int h) {
    if (!rgba || w <= 0 || h <= 0) return;
    if (!g_piece_tex) glGenTextures(1, &g_piece_tex);
    glBindTexture(GL_TEXTURE_2D, g_piece_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    g_piece_w = w; g_piece_h = h;
    LOGI("棋子图集: 已注入 %dx%d", w, h);
}

/* 布阵模式开关：缩小棋盘，底部显示确认按钮（与 touch 区域一致） */
void junqi_render_set_setup(int on) { g_setup = on; }

/* draw_quad 定义在本文件后部，这里前向声明（draw_hud/draw_digit 用到） */
static void draw_quad(float x0, float y0, float x1, float y1,
                      float R, float G, float B, float *verts, int *n);

/* ---- 像素字模（5×7 数字 0-9 + 冒号/空格），不依赖 TTF ---- */
static const uint8_t FONT_DIGIT[11][7] = {
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, /* 0 */
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, /* 1 */
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, /* 2 */
    {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}, /* 3 */
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, /* 4 */
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, /* 5 */
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, /* 6 */
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, /* 7 */
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, /* 8 */
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, /* 9 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 空格 */
};

/* 特殊子符号（5×7 像素字模）：军旗/炸弹/地雷。L46。 */
static const uint8_t FONT_FLAG[7]  = {0x10,0x1E,0x1E,0x1C,0x10,0x10,0x10}; /* 军旗 */
static const uint8_t FONT_BOMB[7]  = {0x04,0x04,0x0E,0x1F,0x1F,0x0E,0x04}; /* 炸弹 */
static const uint8_t FONT_MINE[7]  = {0x04,0x15,0x0E,0x1F,0x0E,0x15,0x04}; /* 地雷 */
static const uint8_t FONT_CHECK[7] = {0x00,0x00,0x03,0x04,0x18,0x10,0x00}; /* 确认勾 */

/* 画一个 5×7 像素字模（glyph=7 字节位图），在归一化 (x,y) 左下角。写进 verts。 */
static void draw_glyph(const uint8_t *glyph, float x, float y, float pw,
                       float R, float G, float B, float *verts, int *n) {
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (!(glyph[row] & (0x10 >> col))) continue;
            float x0 = x + col * pw, y0 = y + (6 - row) * pw;
            draw_quad(x0, y0, x0 + pw, y0 + pw, R, G, B, verts, n);
        }
    }
}

/* 画一个像素字模数字（5×7），在归一化 (x,y) 左下角，pw=像素宽。写进 verts。 */
static void draw_digit(int d, float x, float y, float pw,
                       float R, float G, float B, float *verts, int *n) {
    if (d < 0 || d > 9) d = 10;
    draw_glyph(FONT_DIGIT[d], x, y, pw, R, G, B, verts, n);
}

/* 画特殊子符号（军旗/炸弹/地雷），返回 1 已画。L46。 */
static int draw_special_symbol(int rank, float x, float y, float pw,
                               float R, float G, float B, float *verts, int *n) {
    const uint8_t *g = NULL;
    if (rank == JQ_RANK_FLAG)       g = FONT_FLAG;
    else if (rank == JQ_RANK_BOMB)  g = FONT_BOMB;
    else if (rank == JQ_RANK_MINE)  g = FONT_MINE;
    if (!g) return 0;
    draw_glyph(g, x, y, pw, R, G, B, verts, n);
    return 1;
}

/* 画整数（十进制），从 (x,y) 起，每数字宽 6*pw。返回末尾 x。 */
static float draw_int(int val, float x, float y, float pw,
                      float R, float G, float B, float *verts, int *n) {
    char buf[16];
    int len = snprintf(buf, sizeof(buf), "%d", val);
    for (int i = 0; i < len; i++)
        draw_digit(buf[i] - '0', x + i * 6 * pw, y, pw, R, G, B, verts, n);
    return x + len * 6 * pw;
}

/* HUD：顶部条显示当前方 + 双方剩余子数 */
static void draw_hud(const jq_state *st) {
    if (!st) return;
    float verts[512 * 5];
    int n = 0;
    /* 顶部半透明背景条（画在棋盘上方空隙） */
    float bar_y0 = 0.93f, bar_y1 = 1.0f;
    draw_quad(-1.0f, bar_y0, 1.0f, bar_y1, 0.1f, 0.1f, 0.15f, verts, &n);
    /* 当前方指示（彩块 + 蓝/红） */
    float cur_col = (st->turn == JQ_SIDE_BLUE) ? 0.2f : 0.8f;
    float cur_colg = (st->turn == JQ_SIDE_BLUE) ? 0.3f : 0.2f;
    draw_quad(-0.97f, 0.945f, -0.90f, 0.985f, cur_col, cur_colg, 0.2f, verts, &n);
    draw_int(st->pieces_left[JQ_SIDE_BLUE], -0.86f, 0.945f, 0.006f,
             0.3f, 0.5f, 1.0f, verts, &n);
    draw_int(st->pieces_left[JQ_SIDE_RED], 0.75f, 0.945f, 0.006f,
             1.0f, 0.4f, 0.4f, verts, &n);
    glVertexAttribPointer(g_a_pos, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), verts);
    glEnableVertexAttribArray(g_a_pos);
    glVertexAttribPointer(g_a_color, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), verts + 2);
    glEnableVertexAttribArray(g_a_color);
    glDrawArrays(GL_TRIANGLES, 0, n / 5);
}

/* 注册棋子贴图（side 0=蓝/1=红，rank=JQ_RANK_* 或 32-40）。
   已回退为 rank 数字渲染（L38），此接口保留为空操作（兼容头文件）。 */
void junqi_render_set_piece_texture(int side, int rank,
                                    const unsigned char *rgba, int w, int h) {
    (void)side; (void)rank; (void)rgba; (void)w; (void)h;
}

/* 画一个单位四边形（归一化坐标），带颜色 */
static void draw_quad(float x0, float y0, float x1, float y1,
                      float R, float G, float B, float *verts, int *n) {
    float quad[6][2] = {{x0,y0},{x1,y0},{x1,y1}, {x0,y0},{x1,y1},{x0,y1}};
    for (int k = 0; k < 6; k++) {
        verts[(*n)++] = quad[k][0];
        verts[(*n)++] = quad[k][1];
        verts[(*n)++] = R; verts[(*n)++] = G; verts[(*n)++] = B;
    }
}

/* 棋盘区域（格方形，5:12 比例居中）— frame 与 touch 共用。L35。
   布阵模式缩小棋盘（bh=0.62），底部留出确认按钮条。 */
static void board_region(int w, int h, float *bx0, float *by0, float *bx1, float *by1) {
    float aspect_s = (h > 0) ? (float)w / (float)h : 0.5625f;
    float bh = g_setup ? 0.62f : 0.92f;
    float bw = bh * (5.0f / 12.0f) / aspect_s;
    if (bw > 0.93f) { bw = 0.93f; bh = bw * (12.0f / 5.0f) * aspect_s; }
    float cy = g_setup ? -0.02f : -0.03f;
    *bx0 = -bw; *bx1 = bw;
    *by1 = cy + bh; *by0 = cy - bh;
}

/* 布阵确认按钮（归一化矩形，底部横条）— frame 与 hit-test 共用 */
static void setup_button_rect(float *x0, float *y0, float *x1, float *y1) {
    *x0 = -0.35f; *x1 = 0.35f;
    *y0 = -0.95f; *y1 = -0.80f;
}

/* 布阵确认按钮命中测试（像素坐标） */
int junqi_render_setup_confirm_hit(float px, float py, int w, int h) {
    if (!g_setup || w <= 0 || h <= 0) return 0;
    float nx = px / w * 2.0f - 1.0f;
    float ny = 1.0f - py / h * 2.0f;
    float x0, y0, x1, y1;
    setup_button_rect(&x0, &y0, &x1, &y1);
    return (nx >= x0 && nx <= x1 && ny >= y0 && ny <= y1);
}

/* 屏幕像素 → 棋盘格索引（与渲染棋盘区域一致） */
int junqi_render_touch_cell(float px, float py, int w, int h) {
    if (w <= 0 || h <= 0) return -1;
    float nx = px / w * 2.0f - 1.0f;
    float ny = 1.0f - py / h * 2.0f;
    float bx0, by0, bx1, by1;
    board_region(w, h, &bx0, &by0, &bx1, &by1);
    if (nx < bx0 || nx > bx1 || ny < by0 || ny > by1) return -1;  /* 棋盘外 */
    float cw = (bx1 - bx0) / JQ_BOARD_COLS;
    float ch = (by1 - by0) / JQ_BOARD_ROWS;
    int col = (int)((nx - bx0) / cw);
    int row = (int)((by1 - ny) / ch);
    if (col < 0 || col >= JQ_BOARD_COLS || row < 0 || row >= JQ_BOARD_ROWS) return -1;
    return jq_cell_index(row, col);
}

void junqi_render_frame(const jq_state *st) {
    glClearColor(0.1f, 0.35f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(g_prog);

    /* 棋盘区域：居中 + 保持格方形（5:12 比例）。L35。 */
    float bx0, by0, bx1, by1;
    board_region(g_w, g_h, &bx0, &by0, &bx1, &by1);
    float cw = (bx1 - bx0) / JQ_BOARD_COLS;
    float ch = (by1 - by0) / JQ_BOARD_ROWS;

    /* 真实棋盘纹理背景（若已注入）— 映射棋盘主体（PNG 中央 340px/737），全高 */
    if (g_board_tex) {
        glUniform1i(g_u_has_tex, 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_board_tex);
        glUniform1i(g_u_tex, 0);
        float tverts[12][6];
        int tn = 0;
        float pos[4][2]={{bx0,by0},{bx1,by0},{bx1,by1},{bx0,by1}};
        float uv[4][2]={{0.366f,0},{0.631f,0},{0.631f,1},{0.366f,1}}; /* 棋盘主体（5列中央） */
        int idx[6]={0,1,2, 0,2,3};
        for (int k=0;k<6;k++){
            tverts[tn][0]=pos[idx[k]][0]; tverts[tn][1]=pos[idx[k]][1];
            tverts[tn][2]=uv[idx[k]][0]; tverts[tn][3]=uv[idx[k]][1];
            tverts[tn][4]=0; tverts[tn][5]=0;
            tn++;
        }
        glVertexAttribPointer(g_a_pos, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), tverts);
        glEnableVertexAttribArray(g_a_pos);
        glVertexAttribPointer(g_a_uv, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), tverts+2);
        glEnableVertexAttribArray(g_a_uv);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        /* 恢复色块模式画格子（覆盖在纹理上） */
        glUniform1i(g_u_has_tex, 0);
    } else {
        glUniform1i(g_u_has_tex, 0);
    }

    /* 色块：只画有子格（空格让棋盘纹理露出，格线/行营/大本营标记在纹理上）。L42。
       棋子图集已注入时跳过（图集通道渲染所有有子格，避免色块闪烁/浪费）。 */
    if (!(g_piece_tex && g_piece_w > 0 && g_piece_h > 0)) {
    float verts[60 * 2 * 3 * 5];
    int n = 0;
    for (int r = 0; r < JQ_BOARD_ROWS; r++) {
        for (int c = 0; c < JQ_BOARD_COLS; c++) {
            int idx = jq_cell_index(r, c);
            const jq_cell *cell = &st->board[idx];
            if (cell->rank == JQ_EMPTY) continue;   /* 空格不画（纹理露出） */
            float x0 = bx0 + c * cw, x1 = x0 + cw;
            float y1 = by1 - r * ch, y0 = y1 - ch;
            float R, G, B;
            if (!cell->revealed)        { R=0.35f;G=0.55f;B=0.35f; }  /* 背面绿 */
            else if (cell->side == JQ_SIDE_BLUE) { R=0.2f;G=0.3f;B=0.8f; }
            else                        { R=0.8f;G=0.2f;B=0.2f; }
            /* 选中格高亮（玩家反馈） */
            if (idx == g_selected) {
                R = (R + 1.0f) * 0.5f; G = (G + 1.0f) * 0.5f; B = (B + 1.0f) * 0.5f;
            }
            draw_quad(x0, y0, x1, y1, R, G, B, verts, &n);
        }
    }
    glVertexAttribPointer(g_a_pos, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), verts);
    glEnableVertexAttribArray(g_a_pos);
    glVertexAttribPointer(g_a_color, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), verts + 2);
    glEnableVertexAttribArray(g_a_color);
    glDrawArrays(GL_TRIANGLES, 0, n / 5);
    }

    /* 棋子图集通道：已注入 data/sg/piece.png（212×280，4列×7行，53×40/格）
       时，用图集子纹理渲染全部棋子。已翻开子：红方(side=1)行0-2、蓝方(side=0)行3-5
       （rank→列 (rank-1)%4，子行 (rank-1)/4）；未翻开子（背面）：row6 col0 绿块；
       选中格：row6 col1(红)/col2(蓝) 高亮。
       棋子保持图集比例（53:40）渲染，按棋盘格宽缩放居中（不拉伸到棋盘格 39×59）。 */
    if (g_piece_tex && g_piece_w > 0 && g_piece_h > 0) {
        float cell_w = (float)g_piece_w / 4.0f;   /* 53 */
        float cell_h = (float)g_piece_h / 7.0f;   /* 40 */
        float pt[60 * 6 * 4];
        int pn = 0;
        for (int r = 0; r < JQ_BOARD_ROWS; r++) {
            for (int c = 0; c < JQ_BOARD_COLS; c++) {
                int idx = jq_cell_index(r, c);
                const jq_cell *cell = &st->board[idx];
                if (cell->rank == JQ_EMPTY) continue;
                int col, row;
                if (!cell->revealed) {
                    col = 0; row = 6;
                } else if (idx == g_selected) {
                    col = (cell->side == JQ_SIDE_RED) ? 1 : 2;
                    row = 6;
                } else {
                    int rank = cell->rank;
                    if (rank < 1 || rank > 12) rank = 1;
                    col = (rank - 1) % 4;
                    int sub = (rank - 1) / 4;
                    row = (cell->side == JQ_SIDE_RED) ? sub : (3 + sub);
                }
                float u0 = col * cell_w / g_piece_w;
                float v0 = row * cell_h / g_piece_h;
                float u1 = u0 + cell_w / g_piece_w;
                float v1 = v0 + cell_h / g_piece_h;
                /* 棋盘格区域（x0..x1, y0..y1），棋子保持 53:40 比例按格宽缩放居中 */
                float gx0 = bx0 + c * cw, gx1 = gx0 + cw;
                float gy1 = by1 - r * ch, gy0 = gy1 - ch;
                float aspect = cell_w / cell_h;   /* 1.325 */
                float pw = cw;                    /* 按棋盘格宽 */
                float ph = pw / aspect;           /* 39/1.325=29.4 */
                if (ph > ch) { ph = ch; pw = ph * aspect; }
                float cx = (gx0 + gx1) / 2.0f, cy = (gy0 + gy1) / 2.0f;
                float x0 = cx - pw / 2.0f, x1 = cx + pw / 2.0f;
                float y0 = cy - ph / 2.0f, y1 = cy + ph / 2.0f;
                float pos[4][2] = {{x0,y0},{x1,y0},{x1,y1},{x0,y1}};
                float uv[4][2]  = {{u0,v0},{u1,v0},{u1,v1},{u0,v1}};
                int q[6] = {0,1,2, 0,2,3};
                for (int k = 0; k < 6; k++) {
                    pt[pn++]=pos[q[k]][0]; pt[pn++]=pos[q[k]][1];
                    pt[pn++]=uv[q[k]][0]; pt[pn++]=uv[q[k]][1];
                }
            }
        }
        if (pn > 0) {
            glUniform1i(g_u_has_tex, 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, g_piece_tex);
            glUniform1i(g_u_tex, 0);
            glVertexAttribPointer(g_a_pos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), pt);
            glEnableVertexAttribArray(g_a_pos);
            glVertexAttribPointer(g_a_uv, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), pt + 2);
            glEnableVertexAttribArray(g_a_uv);
            glDrawArrays(GL_TRIANGLES, 0, pn / 4);
            glUniform1i(g_u_has_tex, 0);
        }
    }

    /* 符号/rank 通道：已revealed子——特殊子画符号（军旗/炸弹/地雷），普通子画 rank 数字。L46。
       棋子图集已含 rank 文字（红方"司令/军长..."）时跳过（避免重复文字）。 */
    if (!(g_piece_tex && g_piece_w > 0 && g_piece_h > 0)) {
    for (int r = 0; r < JQ_BOARD_ROWS; r++) {
        for (int c = 0; c < JQ_BOARD_COLS; c++) {
            int idx = jq_cell_index(r, c);
            const jq_cell *cell = &st->board[idx];
            if (cell->rank == JQ_EMPTY || !cell->revealed) continue;
            float x0 = bx0 + c * cw, x1 = x0 + cw;
            float y1 = by1 - r * ch, y0 = y1 - ch;
            float pw = ch * 0.14f;              /* 像素宽 */
            /* 字色：蓝=浅蓝，红=浅红（对背面/色块清晰） */
            float TR = 1.0f, TG = 1.0f, TB = 1.0f;
            if (cell->side == JQ_SIDE_BLUE)      { TR=0.55f; TG=0.75f; TB=1.0f; }
            else                                 { TR=1.0f;  TG=0.5f;  TB=0.4f; }
            /* 特殊子：符号（军旗/炸弹/地雷），宽 5*pw 居中；普通子：数字，宽 6*pw/位 */
            int is_special = (cell->is_flag || cell->is_bomb || cell->is_mine);
            int glyph_w = is_special ? 5 : 12;   /* 像素列数（地雷符号5，数字2位12） */
            float tw = glyph_w * pw;
            float dx = (x0 + x1) / 2.0f - tw / 2.0f;
            float dy = (y0 + y1) / 2.0f - 3.5f * pw;
            float rv[64 * 5];
            int rn = 0;
            int drew = 0;
            if (is_special) {
                drew = draw_special_symbol(cell->rank, dx, dy, pw, TR, TG, TB, rv, &rn);
            }
            if (!drew) draw_int(cell->rank, dx, dy, pw, TR, TG, TB, rv, &rn);
            glVertexAttribPointer(g_a_pos, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), rv);
            glEnableVertexAttribArray(g_a_pos);
            glVertexAttribPointer(g_a_color, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), rv + 2);
            glEnableVertexAttribArray(g_a_color);
            glDrawArrays(GL_TRIANGLES, 0, rn / 5);
        }
    }
    }

    /* 可走格提示：选中己方子 → 高亮其可走/可吃的目标格（黄框）。L46。 */
    if (g_selected >= 0 && g_selected < JQ_CELLS) {
        const jq_cell *sel = &st->board[g_selected];
        if (sel->rank != JQ_EMPTY && sel->side == st->turn && sel->revealed &&
            !sel->is_flag && !sel->is_mine) {
            float hint[60 * 4 * 2 * 5];   /* 每格最多 4 条边 */
            int hn = 0;
            for (int r = 0; r < JQ_BOARD_ROWS; r++) {
                for (int c = 0; c < JQ_BOARD_COLS; c++) {
                    int tidx = jq_cell_index(r, c);
                    if (tidx == g_selected) continue;
                    if (!jq_can_move(st, g_selected, tidx)) continue;
                    /* 目标格画黄色边框（4 条细边），半透明 */
                    float x0 = bx0 + c * cw, x1 = x0 + cw;
                    float y1 = by1 - r * ch, y0 = y1 - ch;
                    float t = ch * 0.05f;
                    float R=1.0f, G=0.85f, B=0.2f;
                    draw_quad(x0, y1 - t, x1, y1, R, G, B, hint, &hn);       /* 上 */
                    draw_quad(x0, y0, x1, y0 + t, R, G, B, hint, &hn);       /* 下 */
                    draw_quad(x0, y0, x0 + t, y1, R, G, B, hint, &hn);       /* 左 */
                    draw_quad(x1 - t, y0, x1, y1, R, G, B, hint, &hn);       /* 右 */
                }
            }
            if (hn > 0) {
                glVertexAttribPointer(g_a_pos, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), hint);
                glEnableVertexAttribArray(g_a_pos);
                glVertexAttribPointer(g_a_color, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), hint + 2);
                glEnableVertexAttribArray(g_a_color);
                glDrawArrays(GL_TRIANGLES, 0, hn / 5);
            }
        }
    }

    /* HUD：当前方指示 + 各方剩余子数（像素字模，不依赖 TTF，L32） */
    draw_hud(st);

    /* 布阵阶段：底部确认按钮（绿底 + 白勾），命中区域与 hit-test 一致 */
    if (g_setup) {
        float x0, y0, x1, y1;
        setup_button_rect(&x0, &y0, &x1, &y1);
        float bv[32 * 5];
        int bn = 0;
        draw_quad(x0, y0, x1, y1, 0.15f, 0.62f, 0.22f, bv, &bn);
        /* 白边（细） */
        float t = 0.008f;
        draw_quad(x0, y1 - t, x1, y1, 1.0f, 1.0f, 1.0f, bv, &bn);
        draw_quad(x0, y0, x1, y0 + t, 1.0f, 1.0f, 1.0f, bv, &bn);
        draw_quad(x0, y0, x0 + t, y1, 1.0f, 1.0f, 1.0f, bv, &bn);
        draw_quad(x1 - t, y0, x1, y1, 1.0f, 1.0f, 1.0f, bv, &bn);
        /* 中央白勾 */
        float pw = (y1 - y0) * 0.05f;
        draw_glyph(FONT_CHECK, (x0 + x1) / 2.0f - 2.5f * pw,
                   (y0 + y1) / 2.0f - 3.5f * pw, pw, 1.0f, 1.0f, 1.0f, bv, &bn);
        glVertexAttribPointer(g_a_pos, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), bv);
        glEnableVertexAttribArray(g_a_pos);
        glVertexAttribPointer(g_a_color, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), bv + 2);
        glEnableVertexAttribArray(g_a_color);
        glDrawArrays(GL_TRIANGLES, 0, bn / 5);
    }
}
