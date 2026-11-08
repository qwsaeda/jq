/*
 * img_decode.c — 原版 SPAK 位图运行时解码器（C 项：让 64 位 so 真读原版 datax JPEG）
 *
 * 背景（§2026-11-11 "全做"）：64 位 src_rs 原走"烘焙 PNG 占位"路线，不运行时读原版
 *   datax 位图（965 个 .bin 资产保全但不消费）。stb_image（public domain 单文件解码器，
 *   支持 baseline JPEG + PNG）落地后，so 可运行时直读 spak_res/ 里的原版 datax JPEG：
 *   - 主菜单背景 datax_294（800×1205 卡通士兵）
 *   - 对局棋盘背景 datax_177（720×1280 完整棋盘）
 *   替代"纯色退化底"占位 → 视觉真正对齐原版。
 *
 * 设计：
 *   - 单文件 stb_image.h 实现（STB_IMAGE_IMPLEMENTATION）收在本编译单元内，不外泄符号。
 *   - img_decode_jpeg()/img_decode_any() 仅返回 RGBA 缓冲（4 字节/像素，row-major 顶到底），
 *     调用方（render.c）负责 free + 上传 GL。
 *   - 失败回 NULL（调用方走纯色/烘焙 PNG 兜底，行为不变、不崩）。
 *   - 不引入 libjpeg/png 系统库依赖，NDK 纯静态内嵌。
 */
#include "img_decode.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/* stb_image 内部 malloc，跨文件 free 不保证 → 统一用 stbi_free_image */

int img_decode_jpeg(const unsigned char *data, size_t len,
                   unsigned char **out_rgba, int *out_w, int *out_h)
{
    if (!data || len < 2 || !out_rgba || !out_w || !out_h) return -1;
    stbi_set_flip_vertically_on_load(1);   /* 与 png_decode 一致：顶到底 row-major */
    unsigned char *px = stbi_load_from_memory(data, (int)len, out_w, out_h, 0, 4);
    if (!px) return -1;
    *out_rgba = px;
    return 0;
}

int img_decode_any(const unsigned char *data, size_t len,
                  unsigned char **out_rgba, int *out_w, int *out_h)
{
    /* stb_image 自动识别 PNG/JPEG/BMP/GIF（含 datax 解密后明文各格式） */
    if (!data || len < 4 || !out_rgba || !out_w || !out_h) return -1;
    stbi_set_flip_vertically_on_load(1);
    int comp = 0;
    unsigned char *px = stbi_load_from_memory(data, (int)len, out_w, out_h, &comp, 4);
    if (!px || comp != 4) return -1;
    *out_rgba = px;
    return 0;
}

void img_decode_free(unsigned char *px)
{
    if (px) free(px);
}
