/*
 * spak_load.c — R5 军棋 SPAK 资源包加载器（arm64 原生实现）
 *
 * 逆向结论（已通过 redroid 明文 .rodata + 原始 .so 验证）：
 *   - SPAK 加密 = XOR(key[j%32]) + zlib 压缩
 *   - key 32 字节 @ 原版 .rodata VA 0x12a658（明文态已确认）
 *   - 文件表条目 = hash[16] + off[4] + size[4] + usize[4]，28 字节
 *   - hash = MD5(资源路径)（已验证：data/font/all.ttf、data/junqi/move.wav 等）
 *   - 文件表在"合并流"末尾：f1+f2 = 合并流大小
 *
 * 本模块提供：
 *   spak_load(assets_dir)  — 读取并解析全部 SPAK 组（data/datah/datam/datax）
 *   spak_get(path)          — 按 MD5(路径) 查找资源，返回解压数据
 *   spak_free()             — 释放
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <zlib.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include "spak_load.h"

#define LOG_TAG "junqi64"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* SPAK key（32 字节，明文态） */
static const uint8_t SPAK_KEY[32] = {
    0xc2,0xcb,0xe5,0xe7,0xce,0x5c,0x4a,0xc4,
    0x11,0xca,0x6c,0x99,0x06,0xf4,0x76,0xf8,
    0x6d,0x4f,0xcb,0xb6,0x26,0x21,0xde,0x0e,
    0x6a,0x67,0x87,0x4c,0x93,0xf0,0xd5,0x23
};

/* 一个 SPAK 组 */
typedef struct {
    const char *name;          /* "data"/"datah"/"datam"/"datax" */
    const char *members[4];    /* .pak 文件名（合并流） */
    int n_members;
    uint32_t f1;               /* 文件表偏移（合并流内） */
    uint32_t f2;               /* 文件表加密长度 */
} spak_group;

static const spak_group GROUPS[] = {
    {"data",  {"data0.pak","data1.pak",0,0}, 2, 0x12d4f3, 2712},
    {"datah", {"datah0.pak","datah1.pak",0,0}, 2, 0x155db2, 6571},
    {"datam", {"datam.pak",0,0,0}, 1, 0x6e6f9, 4572},
    {"datax", {"datax0.pak","datax1.pak","datax2.pak","datax3.pak"}, 4, 0x2f3f86, 7278},
};
#define N_GROUPS 4

/* 一个资源条目 */
typedef struct {
    char path[256];            /* MD5(路径) → 路径（反查，可选） */
    uint8_t hash[16];
    uint32_t off, size, usize;
    uint8_t *data;             /* 解压后数据 */
    uint32_t data_len;
} spak_entry;

static spak_entry *g_entries = NULL;
static int g_n_entries = 0;

/* data[j] ^= key[j%32] */
static void xor_dec(uint8_t *buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i++)
        buf[i] ^= SPAK_KEY[i % 32];
}

/* 读整个文件 */
static uint8_t *read_file(const char *dir, const char *name, uint32_t *out_len) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc(sz > 0 ? sz : 1);
    if (sz > 0) fread(buf, 1, sz, f);
    fclose(f);
    *out_len = (uint32_t)sz;
    return buf;
}

/* zlib 解压（返回 malloc 缓冲，长度存 out_len；失败返回 NULL） */
static uint8_t *zlib_inflate(const uint8_t *src, uint32_t src_len, uint32_t *out_len) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    if (inflateInit(&zs) != Z_OK) return NULL;
    uint32_t cap = src_len * 4 + 4096;
    uint8_t *out = (uint8_t *)malloc(cap);
    zs.next_in = (Bytef *)src;
    zs.avail_in = src_len;
    zs.next_out = out;
    zs.avail_out = cap;
    int r = inflate(&zs, Z_FINISH);
    if (r != Z_STREAM_END) { inflateEnd(&zs); free(out); return NULL; }
    *out_len = (uint32_t)zs.total_out;
    inflateEnd(&zs);
    return out;
}

/* MD5（RFC1321 完整实现）— 文件表 hash = MD5(资源路径) */
#define MD5_ROTL(x,c) (((x)<<(c))|((x)>>(32-(c))))
static uint32_t md5_k[64] = {
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
    0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
    0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
    0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
    0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
    0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
    0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
};
static uint32_t md5_r[64] = {
    7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
    5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
    4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
    6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
};
static void md5_block(uint32_t state[4], const uint8_t block[64]) {
    uint32_t m[16];
    for (int i = 0; i < 16; i++)
        m[i] = (uint32_t)block[i*4] | ((uint32_t)block[i*4+1]<<8) |
               ((uint32_t)block[i*4+2]<<16) | ((uint32_t)block[i*4+3]<<24);
    uint32_t a=state[0], b=state[1], c=state[2], d=state[3];
    for (int i = 0; i < 64; i++) {
        uint32_t f; int g;
        if (i < 16)      { f=(b&c)|(~b&d); g=i; }
        else if (i < 32) { f=(d&b)|(~d&c); g=(5*i+1)&15; }
        else if (i < 48) { f=b^c^d;        g=(3*i+5)&15; }
        else             { f=c^(b|~d);     g=(7*i)&15; }
        f += a + md5_k[i] + m[g];
        a = d; d = c; c = b;
        b += MD5_ROTL(f, md5_r[i]);
    }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
}
static void md5(const uint8_t *msg, uint32_t len, uint8_t out[16]) {
    uint32_t state[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
    uint32_t nblocks = len / 64;
    for (uint32_t i = 0; i < nblocks; i++)
        md5_block(state, msg + i*64);
    /* 尾部 */
    uint8_t tail[128];
    uint32_t rem = len % 64;
    memcpy(tail, msg + nblocks*64, rem);
    tail[rem] = 0x80;
    uint32_t tail_len = (rem < 56) ? 64 : 128;
    memset(tail + rem + 1, 0, tail_len - rem - 1);
    uint64_t bitlen = (uint64_t)len * 8;
    memcpy(tail + tail_len - 8, &bitlen, 8);
    md5_block(state, tail);
    if (tail_len == 128) md5_block(state, tail + 64);
    for (int i = 0; i < 4; i++) {
        out[i*4]   = (uint8_t)(state[i]);
        out[i*4+1] = (uint8_t)(state[i]>>8);
        out[i*4+2] = (uint8_t)(state[i]>>16);
        out[i*4+3] = (uint8_t)(state[i]>>24);
    }
}

/* 解析并加载一个 SPAK 组的合并流（blob 由调用者提供，这里负责 free） */
static int load_group(uint8_t *blob, uint32_t total, const spak_group *g) {
    if (total != g->f1 + g->f2) {
        LOGE("SPAK %s: 合并流 %u != f1+f2 %u", g->name, total, g->f1+g->f2);
        free(blob); return -1;
    }
    /* 文件表：XOR + zlib */
    uint8_t *tbl = (uint8_t *)malloc(g->f2);
    memcpy(tbl, blob + g->f1, g->f2);
    xor_dec(tbl, g->f2);
    uint32_t tbl_len;
    uint8_t *ft = zlib_inflate(tbl, g->f2, &tbl_len);
    free(tbl);
    if (!ft) { LOGE("SPAK %s: 文件表 zlib 失败", g->name); free(blob); return -1; }
    int n = tbl_len / 28;
    LOGI("SPAK %s: %d 条目", g->name, n);
    /* 数据区 = blob[24:f1] */
    uint32_t data_start = 24;
    uint32_t data_region_len = g->f1 - data_start;
    uint8_t *data_region = blob + data_start;
    for (int i = 0; i < n; i++) {
        uint8_t *e = ft + i * 28;
        spak_entry *ent = &g_entries[g_n_entries];
        memcpy(ent->hash, e, 16);
        memcpy(&ent->off, e + 16, 4);
        memcpy(&ent->size, e + 20, 4);
        memcpy(&ent->usize, e + 24, 4);
        ent->data = NULL;
        ent->data_len = 0;
        /* 数据区资源：XOR + zlib；未压缩条目（usize==size）直接返回 XOR 数据 */
        if (ent->off >= data_start && ent->off - data_start + ent->size <= data_region_len) {
            uint8_t *raw = (uint8_t *)malloc(ent->size);
            memcpy(raw, data_region + (ent->off - data_start), ent->size);
            xor_dec(raw, ent->size);
            if (ent->usize == ent->size) {
                /* 未压缩（棋盘/棋子图集等大图）：XOR 后即资源数据 */
                ent->data = raw;
                ent->data_len = ent->size;
            } else {
                ent->data = zlib_inflate(raw, ent->size, &ent->data_len);
                if (!ent->data) { ent->data = raw; ent->data_len = ent->size; }
                else free(raw);
            }
        }
        g_n_entries++;
    }
    free(ft);
    free(blob);
    return 0;
}

/* 重置（释放旧资源） */
static void spak_reset(void) {
    if (g_entries) {
        for (int i = 0; i < g_n_entries; i++) free(g_entries[i].data);
        free(g_entries);
        g_entries = NULL; g_n_entries = 0;
    }
    g_entries = (spak_entry *)calloc(2048, sizeof(spak_entry));
    g_n_entries = 0;
}

int spak_load(const char *assets_dir) {
    spak_reset();
    for (int gi = 0; gi < N_GROUPS; gi++) {
        const spak_group *g = &GROUPS[gi];
        uint32_t total = 0;
        uint8_t *blob = NULL;
        for (int mi = 0; mi < g->n_members; mi++) {
            uint32_t len;
            uint8_t *part = read_file(assets_dir, g->members[mi], &len);
            if (!part) { LOGE("SPAK: 缺文件 %s", g->members[mi]); continue; }
            uint8_t *nb = (uint8_t *)realloc(blob, total + len);
            if (!nb) { free(part); free(blob); return -1; }
            blob = nb;
            memcpy(blob + total, part, len);
            total += len;
            free(part);
        }
        if (!blob) continue;
        load_group(blob, total, g);
    }
    LOGI("SPAK: 共 %d 资源加载", g_n_entries);
    return 0;
}

/* 从 Android AAssetManager 加载（APK assets 无文件路径）。
   mgr 来自 Activity.getAssets()。 */
int spak_load_assetmanager(void *mgr) {
    if (!mgr) return spak_load(NULL);
    AAssetManager *am = (AAssetManager *)mgr;
    spak_reset();
    for (int gi = 0; gi < N_GROUPS; gi++) {
        const spak_group *g = &GROUPS[gi];
        uint32_t total = 0;
        uint8_t *blob = NULL;
        for (int mi = 0; mi < g->n_members; mi++) {
            AAsset *a = AAssetManager_open(am, g->members[mi], AASSET_MODE_BUFFER);
            if (!a) { LOGE("SPAK: 缺 assets/%s", g->members[mi]); continue; }
            uint32_t len = (uint32_t)AAsset_getLength(a);
            uint8_t *part = (uint8_t *)malloc(len ? len : 1);
            if (len) AAsset_read(a, part, len);
            AAsset_close(a);
            uint8_t *nb = (uint8_t *)realloc(blob, total + len);
            if (!nb) { free(part); free(blob); return -1; }
            blob = nb;
            memcpy(blob + total, part, len);
            total += len;
            free(part);
        }
        if (!blob) continue;
        load_group(blob, total, g);
    }
    LOGI("SPAK: 共 %d 资源加载 (assetmanager)", g_n_entries);
    return 0;
}

/* 按路径查找（MD5(路径) 匹配条目 hash） */
const uint8_t *spak_get(const char *path, uint32_t *out_len) {
    if (!g_entries) return NULL;
    uint8_t h[16];
    md5((const uint8_t *)path, (uint32_t)strlen(path), h);
    return spak_get_by_hash(h, out_len);
}

/* 按 hash 查找 */
const uint8_t *spak_get_by_hash(const uint8_t hash[16], uint32_t *out_len) {
    if (!g_entries) return NULL;
    for (int i = 0; i < g_n_entries; i++) {
        if (memcmp(g_entries[i].hash, hash, 16) == 0) {
            if (out_len) *out_len = g_entries[i].data_len;
            return g_entries[i].data;
        }
    }
    return NULL;
}

void spak_free(void) {
    if (!g_entries) return;
    for (int i = 0; i < g_n_entries; i++) free(g_entries[i].data);
    free(g_entries);
    g_entries = NULL;
    g_n_entries = 0;
}
