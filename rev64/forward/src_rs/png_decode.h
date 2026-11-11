/* png_decode.h — 极简 PNG 解码（8bit RGB/RGBA，依赖 zlib） */
#ifndef PNG_DECODE_H
#define PNG_DECODE_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 解码 PNG data[len] 为 RGBA（4 字节/像素，malloc 分配）。
   成功返回 0（*out_rgba 需 free），失败返回 -1。 */
int png_decode(const unsigned char *data, size_t len,
               unsigned char **out_rgba, int *out_w, int *out_h);

#ifdef __cplusplus
}
#endif
#endif /* PNG_DECODE_H */
