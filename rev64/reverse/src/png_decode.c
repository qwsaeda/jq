/*
 * png_decode.c — 极简 PNG 解码器（仅 8bit RGB/RGBA，非隔行）
 * 依赖 zlib（已链接 -lz），无外部 libpng 依赖。
 * 返回 malloc 的 RGBA 缓冲（4 字节/像素），w/h 输出。
 * 供 render_gles2.c 加载真实棋盘/棋子 PNG 纹理。
 */
#include "png_decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

/* PNG 滤波器重建（每行前 1 字节 filter type） */
static void unfilter(unsigned char *out, const unsigned char *in,
                     int w, int h, int bpp) {
    int stride = w * bpp;
    const unsigned char *src = in;
    unsigned char *dst = out;
    for (int y = 0; y < h; y++) {
        int ft = *src++;
        const unsigned char *prev = (y == 0) ? NULL : dst - stride;
        for (int x = 0; x < stride; x++) {
            int a = (x >= bpp) ? dst[x - bpp] : 0;      /* 左 */
            int b = prev ? prev[x] : 0;                 /* 上 */
            int c = (prev && x >= bpp) ? prev[x - bpp] : 0; /* 左上 */
            int v = src[x];
            switch (ft) {
                case 0: break;                                   /* None */
                case 1: v += a; break;                           /* Sub */
                case 2: v += b; break;                           /* Up */
                case 3: v += (a + b) / 2; break;                 /* Average */
                case 4: {                                        /* Paeth */
                    int p = a + b - c;
                    int pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
                    v += (pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c);
                    break;
                }
            }
            dst[x] = (unsigned char)(v & 0xFF);
        }
        src += stride;
        dst += stride;
    }
}

/* 解码 PNG：rgba 输出（4 字节/像素），成功返回 0，失败返回 -1 */
int png_decode(const unsigned char *data, size_t len,
               unsigned char **out_rgba, int *out_w, int *out_h) {
    if (!data || len < 8 || memcmp(data, "\x89PNG\r\n\x1a\n", 8) != 0)
        return -1;

    size_t pos = 8;
    unsigned w = 0, h = 0, bitdepth = 0, colortype = 0, interlace = 0;
    unsigned char *idat = NULL;
    size_t idat_len = 0, idat_cap = 0;

    /* 遍历 chunk */
    while (pos + 8 <= len) {
        unsigned clen = ((unsigned)data[pos] << 24) | ((unsigned)data[pos+1] << 16) |
                        ((unsigned)data[pos+2] << 8) | (unsigned)data[pos+3];
        const unsigned char *ctype = data + pos + 4;
        const unsigned char *cdata = data + pos + 8;
        if (pos + 8 + clen > len) break;

        if (memcmp(ctype, "IHDR", 4) == 0 && clen >= 13) {
            w = ((unsigned)cdata[0] << 24) | ((unsigned)cdata[1] << 16) |
                ((unsigned)cdata[2] << 8) | (unsigned)cdata[3];
            h = ((unsigned)cdata[4] << 24) | ((unsigned)cdata[5] << 16) |
                ((unsigned)cdata[6] << 8) | (unsigned)cdata[7];
            bitdepth = cdata[8];
            colortype = cdata[9];
            interlace = cdata[12];
        } else if (memcmp(ctype, "IDAT", 4) == 0) {
            if (idat_len + clen > idat_cap) {
                size_t nc = idat_cap ? idat_cap * 2 : clen;
                unsigned char *tmp = (unsigned char *)realloc(idat, nc);
                if (!tmp) { free(idat); return -1; }
                idat = tmp; idat_cap = nc;
            }
            memcpy(idat + idat_len, cdata, clen);
            idat_len += clen;
        } else if (memcmp(ctype, "IEND", 4) == 0) {
            break;
        }
        pos += 12 + clen;
    }

    if (!idat || w == 0 || h == 0 || interlace != 0) { free(idat); return -1; }

    /* 仅支持 8bit：灰=0, RGB=2, 灰度+α=4, RGBA=6 */
    int channels;
    switch (colortype) {
        case 0: channels = 1; break;   /* 灰度 */
        case 2: channels = 3; break;   /* RGB */
        case 4: channels = 2; break;   /* 灰度+α */
        case 6: channels = 4; break;   /* RGBA */
        default: free(idat); return -1;
    }
    if (bitdepth != 8) { free(idat); return -1; }

    int stride = (int)w * channels;
    size_t raw_len = (size_t)stride * h;

    /* zlib 解压扫描线 */
    unsigned char *raw = (unsigned char *)malloc(raw_len);
    if (!raw) { free(idat); return -1; }
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    if (inflateInit(&zs) != Z_OK) { free(raw); free(idat); return -1; }
    zs.next_in = idat; zs.avail_in = (uInt)idat_len;
    zs.next_out = raw; zs.avail_out = (uInt)raw_len;
    int zr = inflate(&zs, Z_FINISH);
    size_t got = zs.total_out;
    inflateEnd(&zs);
    free(idat);
    /* PNG IDAT 流末尾可含填充/校验字节，只要输出达到 raw_len 即算成功
       （Z_FINISH 在 avail_out 恰好耗尽时返回 Z_BUF_ERROR 属正常）。 */
    if (zr != Z_STREAM_END && got != raw_len) { free(raw); return -1; }
    if (got != raw_len) { free(raw); return -1; }

    /* 滤波器重建（in-place：raw 是输入，需临时缓冲） */
    unsigned char *unf = (unsigned char *)malloc(raw_len);
    if (!unf) { free(raw); return -1; }
    unfilter(unf, raw, (int)w, (int)h, channels);
    free(raw);

    /* 转为 RGBA（4 字节/像素） */
    unsigned char *rgba = (unsigned char *)malloc((size_t)w * h * 4);
    if (!rgba) { free(unf); return -1; }
    for (size_t i = 0, n = (size_t)w * h; i < n; i++) {
        const unsigned char *p = unf + i * channels;
        unsigned char *q = rgba + i * 4;
        switch (colortype) {
            case 0: q[0]=q[1]=q[2]=p[0]; q[3]=255; break;
            case 2: q[0]=p[0]; q[1]=p[1]; q[2]=p[2]; q[3]=255; break;
            case 4: q[0]=q[1]=q[2]=p[0]; q[3]=p[1]; break;
            case 6: q[0]=p[0]; q[1]=p[1]; q[2]=p[2]; q[3]=p[3]; break;
        }
    }
    free(unf);
    *out_rgba = rgba;
    *out_w = (int)w;
    *out_h = (int)h;
    return 0;
}
