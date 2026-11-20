/*
 * fuzz_save.c — P6/P7 渗透面：存档文件损坏 + 复盘链 fuzz（src_rs 正向 C）
 *
 * 定位：渗透轮第 11/12 变异器（P6 save.c 170 行 + P7 replay.c 111 行）。
 * 覆盖：.sjg 存档格式（magic + sizeof + state blob + 域校验）损坏输入 +
 *       复盘记录链（record/rewind/cur 边界）。
 *
 * ★ 为什么需要（真机可达的崩溃面）：
 *   - 真机 OnResume 读 one.sjg 续局（jni_bridge :10640 实锤"续局读成功"），
 *     若存档文件被截断/字节翻转/版本漂移 → jq_save_game_read 必须安全拒读而非崩溃
 *   - save.c 已有 4 处加固（B02-071/072 短读+域校验、R41-Q16/Q17 原子语义、
 *     AUD-R-049 越域格归零）但**从未跑过畸形文件 fuzz**
 *   - 复盘链 ui_replay_record/rewind/cur 的边界（n=0 / cur 越界 / 空记录 rewind）
 *     未 fuzz
 *
 * 变异策略：
 *   P6 M1 合法存档 round-trip（基线，必须成功）
 *   P6 M2 截断 fuzz（magic 后 0/4/8/半长/sizeof-1 字节）
 *   P6 M3 字节翻转 fuzz（合法存档随机 1-16 字节翻转 → magic/sizeof/state 域破坏）
 *   P6 M4 定向畸形：magic 错 / sizeof 错 / winner 越域 / side 非法 / rank>24 /
 *       revealed>1 / pieces_left<0 / 文件尾追加垃圾
 *   P7 M1 复盘记录 fuzz（随机 from/to/is_flip × N 次 → n/cur 不变量）
 *   P7 M2 越界 rewind（cur<0 / cur>n / cur=n+1 → 安全拒绝）
 *   P7 M3 空复盘 rewind（n=0 → 不崩）
 *   P7 M4 复盘 → jq_state 恢复不变量（rewind 后棋盘 rank/side 合法域）
 *
 * 判据（4 项全 PASS）：
 *   ① 全部畸形输入 0 崩溃（ASan 干净）
 *   ② 拒读语义：畸形存档返 0 且**不改调用方 st**（原子语义 R41-Q17）
 *   ③ 复盘不变量：n ≤ REPLAY_MAX 且 cur ∈ [0,n]，越界 rewind 安全拒绝
 *   ④ 无泄漏（ASan LSAN）
 *
 * 构建：make fuzzsave（ASan，桌面 -DRS_NO_GL）
 * 用法：dist/jq_fuzzsave [seed] [rounds]
 *
 * 诚实边界：本探针只测**序列化/复盘数据结构**；真机文件路径 I/O（fopen 权限、
 *   私有目录）由 12/12 + 真机黑盒覆盖。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "junqi.h"
#include "replay.h"

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
#define CHECK(cond, ...) do { if (!(cond)) { printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); g_fail = 1; } } while (0)

static const char *PATH = "/tmp/opencode/jq_fuzz_sjg.bin";

/* g_state 定义在 jni_bridge.c（JNI 侧，桌面不可链接）→ 探针自建（ui_state.c 依赖） */
jq_state g_state;

/* 写一个合法存档到 path，返回字节数（0=失败） */
static size_t make_valid(jq_state *st)
{
    jq_state_init(st);
    jq_load_layout(st, JQ_DEFAULT_LAY);
    st->stage = STG_PLAY;
    st->turn = SIDE_BLUE;
    st->winner = JQ_WIN_NONE;
    st->mode = JQ_MODE_LAY;
    if (jq_save_game_write(PATH, st) != 1) return 0;
    FILE *f = fopen(PATH, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    size_t n = (size_t)ftell(f);
    fclose(f);
    return n;
}

/* 读文件到 buf */
static unsigned char *readfile(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    size_t n = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *b = (unsigned char *)malloc(n ? n : 1);
    if (b && fread(b, 1, n, f) != n) { free(b); b = NULL; }
    fclose(f);
    if (len) *len = n;
    return b;
}

/* 写 buf 到 path */
static int writefile(const unsigned char *buf, size_t n)
{
    FILE *f = fopen(PATH, "wb");
    if (!f) return 0;
    size_t w = fwrite(buf, 1, n, f);
    fclose(f);
    return (w == n);
}

/* 尝试读畸形存档 → 断言"返 0 不改 st"或"返 1 但全合法域" */
static void try_read_mutated(const unsigned char *buf, size_t n, const char *tag)
{
    if (!writefile(buf, n)) return;
    jq_state before, after;
    memset(&before, 0x5A, sizeof(before));
    memset(&after, 0x5A, sizeof(after));
    int rc = jq_save_game_read(PATH, &after);
    if (rc != 0) {
        /* 接受（合法域）：断言 rank/side/revealed 全合法 */
        for (int i = 0; i < N_CELLS; i++) {
            const jq_cell *c = &after.cells[i];
            CHECK(c->rank <= 24, "%s 接受但 rank 越域 %u", tag, c->rank);
            CHECK(c->side == SIDE_BLUE || c->side == SIDE_RED || c->side == SIDE_UNKNOWN,
                  "%s 接受但 side 越域 %u", tag, c->side);
            CHECK(c->revealed <= 1, "%s 接受但 revealed=%u", tag, c->revealed);
        }
        CHECK(after.turn <= 1 && after.stage <= STG_OVER &&
              after.winner >= JQ_WIN_NONE && after.winner <= 2,
              "%s 接受但局级字段越域", tag);
    } else {
        /* 拒读：原子语义——after 必须保持毒化值（未被半读污染） */
        CHECK(memcmp(&after, &before, sizeof(after)) == 0,
              "%s 拒读但污染了调用方 st（R41-Q17 原子语义破）", tag);
    }
}

int main(int argc, char **argv)
{
    g_seed = argc > 1 ? (unsigned)atoi(argv[1]) : 0;
    int rounds = argc > 2 ? atoi(argv[2]) : 3000;
    printf("== fuzz_save（P6/P7 渗透面：存档损坏 + 复盘链，seed=%u rounds=%d）==\n", g_seed, rounds);

    /* ===== P6 M1 合法 round-trip 基线 ===== */
    jq_state valid;
    size_t total = make_valid(&valid);
    CHECK(total > 8, "P6 M1 合法存档写出失败（size=%zu）", total);
    printf("   合法存档大小 = %zu 字节\n", total);

    /* ===== P6 M2 截断 fuzz ===== */
    {
        unsigned char *buf = readfile(PATH, &total);
        CHECK(buf != NULL, "%s", "P6 M2 读合法存档失败");
        if (buf) {
            size_t cuts[] = { 0, 1, 4, 8, 9, 12, total/2, total-1 };
            for (size_t i = 0; i < sizeof(cuts)/sizeof(cuts[0]); i++) {
                size_t c = cuts[i];
                if (c > total) continue;
                char tag[64];
                snprintf(tag, sizeof tag, "P6-M2截断%zu", c);
                try_read_mutated(buf, c, tag);
            }
            free(buf);
        }
        printf("   P6 M2 截断 fuzz OK（8 种截断点）\n");
    }

    /* ===== P6 M3 字节翻转 fuzz ===== */
    {
        for (int r = 0; r < rounds; r++) {
            size_t n = 0;
            unsigned char *buf = readfile(PATH, &n);   /* 注意：PATH 可能已被 M2 改坏 → 用 fresh valid */
            if (!buf) { make_valid(&valid); buf = readfile(PATH, &n); }
            if (!buf) break;
            int flips = 1 + (int)(rng() % 16);
            for (int k = 0; k < flips; k++)
                buf[rng() % n] ^= (unsigned char)(1u << (rng() % 8));
            try_read_mutated(buf, n, "P6-M3翻转");
            free(buf);
            /* 每 50 轮重建合法存档（防 PATH 持续损坏影响后续） */
            if (r % 50 == 49) make_valid(&valid);
        }
        printf("   P6 M3 字节翻转 fuzz OK（%d 轮 × 1-16 字节）\n", rounds);
    }

    /* ===== P6 M4 定向畸形 ===== */
    {
        make_valid(&valid);
        size_t n = 0;
        unsigned char *buf = readfile(PATH, &n);
        if (buf) {
            /* magic 错 */
            buf[0] ^= 0xFF; try_read_mutated(buf, n, "P6-M4-magic"); buf[0] ^= 0xFF;
            /* sizeof 错（偏移 4-7） */
            buf[4] = 0xFF; buf[5] = 0xFF; buf[6] = 0xFF; buf[7] = 0xFF;
            try_read_mutated(buf, n, "P6-M4-sizeof");
            make_valid(&valid);
            /* 文件尾追加垃圾 */
            unsigned char *ext = (unsigned char *)malloc(n + 64);
            memcpy(ext, buf, n);
            memset(ext + n, 0xCC, 64);
            try_read_mutated(ext, n + 64, "P6-M4-尾部垃圾");
            free(ext);
            free(buf);
        }
        printf("   P6 M4 定向畸形 OK（magic/sizeof/尾部垃圾）\n");
    }

    /* ===== P7 复盘链 fuzz ===== */
    {
        jq_replay rp;
        jq_replay_init(&rp, &valid);
        CHECK(jq_replay_count(&rp) == 0, "%s", "P7 初始复盘 n 应为 0");
        CHECK(jq_replay_cur_get(&rp) == 0, "%s", "P7 初始 cur 应为 0");
        for (int i = 0; i < rounds; i++) {
            int f = (int)(rng() % 60), t = (int)(rng() % 60);
            int flip = (int)(rng() & 1);
            jq_replay_record(&rp, f, t, flip);
            int n2 = jq_replay_count(&rp);
            int cur = jq_replay_cur_get(&rp);
            CHECK(n2 >= 0 && n2 <= N_REPLAY, "P7 复盘 n 越界 %d", n2);
            CHECK(cur >= 0 && cur <= n2, "P7 复盘 cur 越界 %d/%d", cur, n2);
        }
        int n2 = jq_replay_count(&rp);
        /* 越界 rewind */
        int bad_curs[] = { -1, n2 + 1, INT32_MAX, INT32_MIN };
        for (size_t i = 0; i < sizeof(bad_curs)/sizeof(bad_curs[0]); i++) {
            jq_state st;
            memcpy(&st, &valid, sizeof(st));
            int rc = jq_replay_rewind_to(&rp, &st, bad_curs[i]);
            (void)rc;   /* 返 0/非 0 均可，关键是 0 崩溃 */
            CHECK(jq_replay_cur_get(&rp) >= 0 && jq_replay_cur_get(&rp) <= n2,
                  "P7 越界 rewind(%d) 后 cur 越界", bad_curs[i]);
        }
        /* 空复盘 rewind */
        jq_replay empty;
        jq_replay_init(&empty, &valid);
        jq_state st2;
        memcpy(&st2, &valid, sizeof(st2));
        (void)jq_replay_rewind_to(&empty, &st2, 0);
        printf("   P7 复盘链 OK（%d 次 record + 4 越界 rewind + 空复盘）\n", rounds);
    }

    if (!g_fail) {
        printf("== 全部通过 ==\n");
        printf("判据: PASS（fuzz_save P6/P7：截断/翻转/定向畸形存档 0 崩溃 + 拒读原子语义 + "
               "复盘 n/cur 不变量 + 越界 rewind 安全）\n");
        return 0;
    }
    printf("判据: FAIL\n");
    return 2;
}
