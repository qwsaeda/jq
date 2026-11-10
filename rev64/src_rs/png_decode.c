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
#include <stdint.h>   /* SIZE_MAX（B04-076 realloc 溢出防护） */
#include <zlib.h>
#include <dlfcn.h>   /* ★ AUD-1000 P2 批次（2026-11-10 dlopen 根治）：inflate 3 符号
   dlsym 动态解析（libz 免 NEEDED 直链）——MuMu 系统 libz 98KB 极简版 inflate 导出
   异常（NDK 对照 inflateInit_ 有、inflate 无，9/9 PASS 时 so 无 UND inflate 可 dlopen，
   本轮 UND inflate 后 dlopen failed: failed to link）。zlib.h 类型定义仍需（z_stream
   结构体 + Z_OK/Z_FINISH 常量 + inflate 函数指针 typedef），但 3 个函数符号改
   dlsym 运行时解析，链接期不写 UND 动态引用。*/

/* PNG 滤波器重建（每行前 1 字节 filter type） */
static void unfilter(unsigned char *out, const unsigned char *in,
                     int w, int h, int bpp) {
    int stride = w * bpp;
    const unsigned char *src = in;
    unsigned char *dst = out;
    for (int y = 0; y < h; y++) {
        int ft = *src++;
        /* ★ AUD-1000 B04-081（P2 已修）：filter type 0-4 白名单（PNG 规范 5 类），
           非 0-4（恶意/损坏）拒收——原 switch default 不拒按 None 处理（静默）。 */
        if (ft < 0 || ft > 4) return;
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

        if (memcmp(ctype, "IHDR", 4) == 0 && clen == 13) {
            /* ★ AUD-1000 B04-075（P1 已修）：IHDR 严格 == 13（PNG 规范 IHDR
               恒 13 字节）——旧版 >= 13 收非标准超长 IHDR（畸形/恶意文件
               14+ 字节日志字段错位解析，w/h 读到垃圾 → scanlen 巨值
               耗尽堆/越界）。超长 IHDR 拒收走整文件 -1。 */
            w = ((unsigned)cdata[0] << 24) | ((unsigned)cdata[1] << 16) |
                ((unsigned)cdata[2] << 8) | (unsigned)cdata[3];
            h = ((unsigned)cdata[4] << 24) | ((unsigned)cdata[5] << 16) |
                ((unsigned)cdata[6] << 8) | (unsigned)cdata[7];
            bitdepth = cdata[8];
            colortype = cdata[9];
            interlace = cdata[12];
        } else if (memcmp(ctype, "IDAT", 4) == 0) {
            if (idat_len + clen > idat_cap) {
                /* ★ AUD-1000 B04-076（P1 已修）：realloc 新容量 nc 溢出防护——
                   旧版 nc = cap*2 : clen，clen 超大（恶意 4GB 级 clen）时
                   cap*2 或 clen 本身超 SIZE_MAX → realloc 返回 NULL →
                   误判"内存不足"拒收合法文件；且 cap*2 回绕到小值时
                   nc<clen 直接 memcpy 越界写。现：nc 取 max(cap*2, clen)
                   且 nc 溢出（nc < clen）时拒收整个文件（return -1），
                   防超大 IDAT 耗尽堆/越界。 */
                size_t nc = idat_cap ? idat_cap * 2 : clen;
                if (nc < clen || nc > SIZE_MAX / 2) { free(idat); return -1; }
                unsigned char *tmp = (unsigned char *)realloc(idat, nc);
                if (!tmp) { free(idat); return -1; }
                idat = tmp; idat_cap = nc;
            }
            memcpy(idat + idat_len, cdata, clen);
            idat_len += clen;
        } else if (memcmp(ctype, "IEND", 4) == 0) {
            break;
        }
        /* ★ AUD-1000 B04-074/077（P0 已修）：clen 越界守护——旧实现
           `pos + 8 + clen > len` 中 clen 为恶意 4B 大端值（如 0xFFFFFFF0）时
           32 位加法回绕为小值 → 守护失效 → cdata/idat 越界读。
           改用 size_t 比较（len/pos 本身 size_t，clen 提升后无回绕）。 */
        if (clen > (unsigned)(len - pos - 8)) break;   /* chunk data+4B crc 超出输入 */
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

    /* ★ AUD-1000 B04-078（P1 已修）：w/h 上限校验——超大 PNG（恶意 w=h=65535
       RGBA）scanlen ≈ 16GB 耗尽堆；w/h*4 超阈值直接拒收。阈值 32MP
       （8192×4096 级，远超本仓最大资源 737×751/800×1205 的 50 倍）。 */
    if ((size_t)w * h > 32 * 1024 * 1024) { free(idat); return -1; }
    int stride = (int)w * channels;

    /* zlib 解压扫描线（PNG 每行带 1 字节 filter 头 → 解压产物 = (w*ch+1)*h 字节；
       旧 bug：raw 按 w*h*ch 分配却喂 (w*ch+1)*h 字节 → 越界写 → 真机 SIGSEGV 0x3a） */
    size_t scanlen = ((size_t)w * channels + 1) * h;
    unsigned char *raw = (unsigned char *)malloc(scanlen);
    if (!raw) { free(idat); return -1; }
    /* ★ dlsym 解析 inflate 3 符号（libz 运行时加载，免 NEEDED） */
    void *zdl = dlopen("libz.so", RTLD_NOW | RTLD_GLOBAL);
    if (!zdl) { free(raw); free(idat); return -1; }
    typedef int (*zinf_t)(z_stream *, int);
    typedef int (*ziinit_t)(z_stream *);
    typedef int (*ziend_t)(z_stream *);
    zinf_t p_inflate = (zinf_t)dlsym(zdl, "inflate");
    ziinit_t p_inflateInit = (ziinit_t)dlsym(zdl, "inflateInit_");
    ziend_t p_inflateEnd = (ziend_t)dlsym(zdl, "inflateEnd");
    if (!p_inflate || !p_inflateInit || !p_inflateEnd) {
        dlclose(zdl); free(raw); free(idat); return -1;
    }
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    /* ★ AUD-1000 B04-079（P1 已修）：idat_len > UINT_MAX 时 (uInt) 截断 →
       inflate 读半数据（avail_in 小值，Z_STREAM_END 永不达成 → 误判 -1
       或解出半图）。本仓 PNG 全 < 2MB，超过 4GB 的"IDAT"必为畸形文件，
       直接拒收（zlib 增量分段 inflate 对纹理用途无意义，不做）。
       同理 scanlen 超 uInt 上限时 avail_out 截断 → 输出缓冲不够
       inflate 写不进去（zlib 返 Z_OK 非 Z_STREAM_END 拒收），加防御。 */
    if (idat_len > (size_t)UINT_MAX) { free(idat); return -1; }
    if (scanlen > (size_t)UINT_MAX) { free(raw); free(idat); return -1; }
    if (p_inflateInit(&zs) != Z_OK) { dlclose(zdl); free(raw); free(idat); return -1; }
    zs.next_in = idat; zs.avail_in = (uInt)idat_len;
    zs.next_out = raw; zs.avail_out = (uInt)scanlen;
    int zr = p_inflate(&zs, Z_FINISH);
    p_inflateEnd(&zs);
    dlclose(zdl);
    free(idat);
    if (zr != Z_STREAM_END) { free(raw); return -1; }

    /* 滤波器重建（unfilter 输出 = w*h*channels，不带 filter 头） */
    size_t raw_len = (size_t)stride * h;
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
