#ifndef RS_IMG_DECODE_H
#define RS_IMG_DECODE_H
#include <stddef.h>

/* 原版 SPAK 位图运行时解码（stb_image 内嵌，支持 baseline JPEG + PNG + BMP）。
   成功返回 0，*out_rgba = malloc 的 RGBA（4 字节/像素，row-major 顶到底），调用方
   img_decode_free() 释放。失败返回 -1（输出不动，调用方走纯色/烘焙 PNG 兜底）。 */
int  img_decode_jpeg(const unsigned char *data, size_t len,
                     unsigned char **out_rgba, int *out_w, int *out_h);
int  img_decode_any(const unsigned char *data, size_t len,
                    unsigned char **out_rgba, int *out_w, int *out_h);
void img_decode_free(unsigned char *px);

#endif
