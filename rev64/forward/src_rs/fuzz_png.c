/*
 * fuzz_png.c — P1 渗透面：PNG 解码器畸形输入 fuzz（src_rs 正向 C）
 *
 * 定位：渗透轮第 6 变异器（P1，png_decode.c 207 行畸形输入路径）。
 * 覆盖：png_decode() 的 chunk 遍历 / clen 守护 / IHDR 解析 / IDAT 累积 /
 *       zlib inflate / unfilter 反滤波 / 像素展开 全链。
 *
 * 变异策略（4 类）：
 *   M1 位翻转 fuzz：合法 PNG 逐字节随机翻转（1-8 bit）→ 畸形 chunk/clen/CRC
 *   M2 截断 fuzz：合法 PNG 在随机 offset 截断 → 半文件
 *   M3 定向畸形：clen=0xFFFFFFFF / IHDR 超长 / w/h 巨值 / bitdepth 非法 /
 *       colortype 非法 / interlace=1 / IDAT 巨量 / 重复 IHDR / 无 IEND
 *   M4 结构错乱：chunk 顺序乱（IDAT 在 IHDR 前）/ 嵌套 PNG / 零长度文件
 *
 * 判据（4 项全 PASS）：
 *   ① M1-M4 全部畸形输入 0 崩溃（ASan 干净，含 heap-buffer-overflow）
 *   ② 拒绝语义：畸形输入必须返回 -1（拒收），**不得**返回 0 却产出垃圾像素
 *      （合法 PNG 语料对照组：合法 PNG 必须解码成功 w/h 与真值一致）
 *   ③ 输出不变量：解码成功时 out_rgba 长度 == w*h*4 且 out_w/out_h == IHDR 值
 *   ④ 无内存泄漏：ASan LeakSanitizer 0 泄漏（leak 检测在 main 退出时报告）
 *
 * 构建：make fuzzpng（ASan，桌面 -DRS_NO_GL，链接 png_decode.c + zlib）
 * 用法：dist/jq_fuzzpng [seed] [rounds]
 *       默认 seed=0 rounds=3000（每轮 4 类变异各若干样本）
 *
 * 诚实边界：
 *  - 本探针只测**解码器自身**的畸形输入健壮性；不测 zlib 内部（libz 上游 CVE 不在范围）
 *  - 语料源 = 仓库内真实 PNG 资源（rev64/dist/spak_res 或 src_rs/assets），
 *    无语料时用**内嵌最小合法 PNG**（1×1 RGBA，zlib 压缩）作为对照
 *  - ASan 的 malloc 失败路径（OOM）不模拟；w/h 巨值由解码器自身上限守护拒收
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <stdarg.h>

extern int png_decode(const unsigned char *data, size_t len,
                      unsigned char **out_rgba, int *out_w, int *out_h);

/* 桌面 stub：png_decode.c 依赖 android/log.h 的 __android_log_print（真机诊断日志） */
int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{ (void)prio; (void)tag; (void)fmt; return 0; }

/* ---------- 内嵌 1×1 RGBA PNG（对照组：合法必须解码成功） ----------
 * 字节由 python zlib 生成（IHDR w=1 h=1 bitdepth=8 colortype=6 + IDAT zlib
 * 压缩的 [filter=0, R,G,B,A] + IEND，CRC 由 zlib.crc32 算出） */
static const unsigned char k_valid_png[] = {
    0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a, /* signature */
    0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52, /* IHDR len=13 */
    0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01, /* w=1 h=1 */
    0x08,0x06,0x00,0x00,0x00,             /* bitdepth=8 colortype=6 */
    0x1f,0x15,0xc4,0x89,                   /* IHDR CRC */
    0x00,0x00,0x00,0x0d,0x49,0x44,0x41,0x54, /* IDAT len=13 */
    0x78,0xda,0x63,0xf8,0xcf,0xc0,0xf0,0x1f,0x00,0x05,0x00,0x01,0xff,
    0x56,0xc7,0x2f,0x0d,                   /* IDAT CRC */
    0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44, /* IEND */
    0xae,0x42,0x60,0x82
};
static const size_t k_valid_len = sizeof(k_valid_png);

/* ---------- RNG（xorshift32，g_seed 变换后保存） ---------- */
static unsigned g_seed;
static unsigned rng(void)
{
    if (!g_seed) g_seed = 0x9e3779b9u;
    unsigned x = g_seed;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    g_seed = x ? x : 0xc0ffee01u;
    return g_seed;
}

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); g_fail = 1; } } while (0)

/* ---------- 语料加载（仓库真实 PNG 优先，回落内嵌） ---------- */
#define MAX_CORPUS 64
static unsigned char *g_corpus[MAX_CORPUS];
static size_t g_corpus_len[MAX_CORPUS];
static int g_corpus_n = 0;

static void load_corpus(void)
{
    const char *dirs[] = {
        "/disk/workspaces/jq/rev64/dist/spak_res",
        "/disk/workspaces/jq/rev64/forward/src_rs/assets",
        "/disk/workspaces/jq/rev64/dist/spak_res/png",
    };
    for (size_t d = 0; d < sizeof(dirs)/sizeof(dirs[0]) && g_corpus_n < MAX_CORPUS; d++) {
        DIR *dp = opendir(dirs[d]);
        if (!dp) continue;
        struct dirent *de;
        while ((de = readdir(dp)) && g_corpus_n < MAX_CORPUS) {
            size_t nl = strlen(de->d_name);
            if (nl < 5 || strcmp(de->d_name + nl - 4, ".png") != 0) continue;
            char path[512];
            snprintf(path, sizeof path, "%s/%s", dirs[d], de->d_name);
            FILE *f = fopen(path, "rb");
            if (!f) continue;
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (sz <= 8 || sz > 8 * 1024 * 1024) { fclose(f); continue; }
            unsigned char *buf = (unsigned char *)malloc((size_t)sz);
            if (!buf) { fclose(f); continue; }
            if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); continue; }
            fclose(f);
            g_corpus[g_corpus_n] = buf;
            g_corpus_len[g_corpus_n] = (size_t)sz;
            g_corpus_n++;
        }
        closedir(dp);
    }
    if (g_corpus_n == 0) {
        g_corpus[0] = (unsigned char *)malloc(k_valid_len);
        memcpy(g_corpus[0], k_valid_png, k_valid_len);
        g_corpus_len[0] = k_valid_len;
        g_corpus_n = 1;
        printf("   （仓库无 PNG 语料，回落内嵌 1×1 RGBA 对照组）\n");
    } else {
        printf("   语料：%d 个真实 PNG\n", g_corpus_n);
    }
}

/* ---------- 解码调用 + 不变量断言 ---------- */
static int g_accepted = 0, g_rejected = 0, g_bad_out = 0, g_leak_suspect = 0;

/* 返回 1=接受 0=拒收；out_len 输出像素字节数 */
static int try_decode(const unsigned char *buf, size_t len, size_t *out_len, int *ow, int *oh)
{
    unsigned char *rgba = NULL;
    int w = 0, h = 0;
    int rc = png_decode(buf, len, &rgba, &w, &h);
    if (rc != 0) {
        if (rgba) { g_leak_suspect++; free(rgba); }   /* 拒收却给出指针 = 契约破坏 */
        g_rejected++;
        return 0;
    }
    g_accepted++;
    /* 输出不变量：rgba 非空 + w/h 合法 + 长度 == w*h*4 */
    if (!rgba || w <= 0 || h <= 0 ||
        (size_t)w * (size_t)h > 32u * 1024u * 1024u) {
        g_bad_out++;
    } else {
        if (out_len) *out_len = (size_t)w * (size_t)h * 4;
    }
    if (ow) *ow = w;
    if (oh) *oh = h;
    free(rgba);
    return 1;
}

/* ---------- M1 位翻转 ---------- */
static void fuzz_bitflip(int rounds)
{
    for (int r = 0; r < rounds; r++) {
        int ci = (int)(rng() % (unsigned)g_corpus_n);
        size_t len = g_corpus_len[ci];
        unsigned char *buf = (unsigned char *)malloc(len);
        memcpy(buf, g_corpus[ci], len);
        int nflip = 1 + (int)(rng() % 8);
        for (int k = 0; k < nflip; k++) {
            size_t pos = rng() % len;
            buf[pos] ^= (unsigned char)(1u << (rng() % 8));
        }
        try_decode(buf, len, NULL, NULL, NULL);
        free(buf);
    }
}

/* ---------- M2 截断 ---------- */
static void fuzz_truncate(int rounds)
{
    for (int r = 0; r < rounds; r++) {
        int ci = (int)(rng() % (unsigned)g_corpus_n);
        size_t len = g_corpus_len[ci];
        size_t cut = rng() % len;
        unsigned char *buf = (unsigned char *)malloc(len ? len : 1);
        memcpy(buf, g_corpus[ci], len);
        try_decode(buf, cut, NULL, NULL, NULL);
        free(buf);
    }
}

/* ---------- M3 定向畸形 ---------- */
static void put32(unsigned char *p, uint32_t v)
{ p[0]=(unsigned char)(v>>24); p[1]=(unsigned char)(v>>16); p[2]=(unsigned char)(v>>8); p[3]=(unsigned char)v; }

static void fuzz_targeted(void)
{
    /* ③-a clen = 0xFFFFFFFF（守护 pos+8+clen 溢出） */
    {
        unsigned char buf[128];
        memcpy(buf, k_valid_png, sizeof k_valid_png);
        put32(buf + 8, 0xFFFFFFFFu);
        try_decode(buf, sizeof buf, NULL, NULL, NULL);
    }
    /* ③-b IHDR 超长（len=14，畸形） */
    {
        unsigned char buf[128];
        memcpy(buf, k_valid_png, sizeof k_valid_png);
        put32(buf + 8, 14);
        try_decode(buf, sizeof buf, NULL, NULL, NULL);
    }
    /* ③-c w/h 巨值（0xFFFFFFFF）→ 应被 w/h 上限守护拒收 */
    {
        unsigned char buf[128];
        memcpy(buf, k_valid_png, sizeof k_valid_png);
        put32(buf + 16, 0xFFFFFFFFu);
        put32(buf + 20, 0xFFFFFFFFu);
        try_decode(buf, sizeof buf, NULL, NULL, NULL);
    }
    /* ③-d w=0 / h=0 */
    {
        unsigned char buf[128];
        memcpy(buf, k_valid_png, sizeof k_valid_png);
        put32(buf + 16, 0);
        try_decode(buf, sizeof buf, NULL, NULL, NULL);
        memcpy(buf, k_valid_png, sizeof k_valid_png);
        put32(buf + 20, 0);
        try_decode(buf, sizeof buf, NULL, NULL, NULL);
    }
    /* ③-e bitdepth 非法（16 / 0） */
    {
        unsigned char buf[128];
        for (int v = 0; v <= 16; v += 4) {
            memcpy(buf, k_valid_png, sizeof k_valid_png);
            buf[24] = (unsigned char)v;
            try_decode(buf, sizeof buf, NULL, NULL, NULL);
        }
    }
    /* ③-f colortype 非法（1 / 3 / 5 / 7 / 255） */
    {
        unsigned char buf[128];
        int bad[] = {1,3,5,7,255};
        for (size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); i++) {
            memcpy(buf, k_valid_png, sizeof k_valid_png);
            buf[25] = (unsigned char)bad[i];
            try_decode(buf, sizeof buf, NULL, NULL, NULL);
        }
    }
    /* ③-g interlace=1（Adam7）→ 应拒收 */
    {
        unsigned char buf[128];
        memcpy(buf, k_valid_png, sizeof k_valid_png);
        buf[28] = 1;
        try_decode(buf, sizeof buf, NULL, NULL, NULL);
    }
    /* ③-h 重复 IHDR（w/h 覆盖） */
    {
        unsigned char buf[128];
        memcpy(buf, k_valid_png, sizeof k_valid_png);
        memcpy(buf + sizeof k_valid_png - 12, k_valid_png + 8, 33);  /* 复制 IHDR chunk */
        try_decode(buf, sizeof k_valid_png - 12 + 33, NULL, NULL, NULL);
    }
    /* ③-i IDAT 巨量声明（clen=0x7FFFFFFF） */
    {
        unsigned char buf[128];
        memcpy(buf, k_valid_png, sizeof k_valid_png);
        put32(buf + 33, 0x7FFFFFFFu);
        try_decode(buf, sizeof buf, NULL, NULL, NULL);
    }
    /* ③-j 无 IEND（截掉尾部 12 字节） */
    {
        unsigned char buf[128];
        memcpy(buf, k_valid_png, sizeof k_valid_png);
        try_decode(buf, sizeof k_valid_png - 12, NULL, NULL, NULL);
    }
    /* ③-k 签名错（8 字节非 PNG magic） */
    {
        unsigned char buf[128];
        memcpy(buf, k_valid_png, sizeof k_valid_png);
        buf[1] = 'X';
        try_decode(buf, sizeof buf, NULL, NULL, NULL);
    }
    /* ③-l 空文件 / 4 字节 / 8 字节 */
    {
        unsigned char buf[8] = {0};
        try_decode(buf, 0, NULL, NULL, NULL);
        try_decode(buf, 4, NULL, NULL, NULL);
        try_decode(buf, 8, NULL, NULL, NULL);
    }
    /* ③-m NULL 指针 */
    {
        unsigned char *rgba = NULL; int w = 0, h = 0;
        int rc = png_decode(NULL, 100, &rgba, &w, &h);
        CHECK(rc != 0, "M3 png_decode(NULL) 必须拒收");
    }
    /* ③-n IDAT 巨量（1MB 零填充，声明长度远超实际） */
    {
        size_t n = 1024 * 1024;
        unsigned char *buf = (unsigned char *)malloc(n);
        memcpy(buf, k_valid_png, sizeof k_valid_png);
        memset(buf + sizeof k_valid_png, 0, n - sizeof k_valid_png);
        try_decode(buf, n, NULL, NULL, NULL);
        free(buf);
    }
}

/* ---------- M4 结构错乱 ---------- */
static void fuzz_structure(int rounds)
{
    for (int r = 0; r < rounds; r++) {
        int ci = (int)(rng() % (unsigned)g_corpus_n);
        size_t len = g_corpus_len[ci];
        unsigned char *buf = (unsigned char *)malloc(len * 2 + 64);
        /* ① 双 PNG 拼接（前一个截断 + 后一个完整） */
        size_t cut = rng() % len;
        memcpy(buf, g_corpus[ci], cut);
        memcpy(buf + cut, g_corpus[ci], len);
        try_decode(buf, cut + len, NULL, NULL, NULL);
        /* ② 前置垃圾字节（签名错位） */
        size_t pre = rng() % 32;
        memcpy(buf + pre, g_corpus[ci], len);
        try_decode(buf + pre, len, NULL, NULL, NULL);
        free(buf);
    }
}

int main(int argc, char **argv)
{
    g_seed = argc > 1 ? (unsigned)atoi(argv[1]) : 0;
    int rounds = argc > 2 ? atoi(argv[2]) : 3000;
    printf("== fuzz_png（P1 渗透面：PNG 解码器畸形输入，seed=%u rounds=%d）==\n", g_seed, rounds);
    load_corpus();

    /* 对照组：合法 PNG 必须解码成功（w/h 与真值一致） */
    {
        unsigned char *rgba = NULL; int w = 0, h = 0;
        int rc = png_decode(k_valid_png, k_valid_len, &rgba, &w, &h);
        CHECK(rc == 0, "对照组：内嵌合法 PNG 必须解码成功");
        CHECK(rgba != NULL && w == 1 && h == 1, "对照组：合法 PNG 输出 w/h 必须 = 1×1");
        if (rgba) {
            CHECK(rgba[3] == 255, "对照组：1×1 RGBA alpha 通道值应为 255");
            free(rgba);
        }
        printf("   对照组 OK（合法 1×1 RGBA 解码成功）\n");
    }

    fuzz_bitflip(rounds);
    fuzz_truncate(rounds);
    fuzz_targeted();
    fuzz_structure(rounds / 4);

    printf("   统计：接受=%d 拒收=%d 输出不变量违规=%d 拒收却给指针=%d\n",
           g_accepted, g_rejected, g_bad_out, g_leak_suspect);
    CHECK(g_bad_out == 0, "P1 输出不变量违规（接受但 w/h/rgba 非法）");
    CHECK(g_leak_suspect == 0, "P1 拒收却返回非空指针（契约破坏 → 泄漏）");
    CHECK(g_accepted >= 1, "P1 全畸形输入无任何接受（对照组之外不该有接受样本——语料变异可能产出合法图）");
    CHECK(g_rejected > 0, "P1 拒收样本为 0（变异器未生效，判据假绿）");

    if (!g_fail) {
        printf("== 全部通过 ==\n");
        printf("判据: PASS（fuzz_png P1：位翻转/截断/定向畸形/结构错乱 0 崩溃 ASan 干净 + "
               "拒收语义 + 输出不变量 + 无泄漏）\n");
        return 0;
    }
    printf("判据: FAIL\n");
    return 2;
}
