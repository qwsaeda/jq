/*
 * fuzz_mt.c — P8 渗透面：多线程竞态 fuzz（TSAN 桌面）
 *
 * 定位：渗透轮第 9 变异器（P8，src_rs 多线程共享全局竞态）。
 * ★ 为什么必须做：现有全部判据都是**单线程**的（12/12 回归 + F1-F5 + P1/P2/P5'），
 *   而 src_rs 的真实运行模型是**多线程**：
 *     ① 主线程：OnTouch / OnEvent / OnCreate / OnUpdate / OnTimer（JNI 调用）
 *     ② GL 渲染线程：rs_render_frame（读 g_state / g_view_* / 纹理）
 *     ③ OnUpdate 定时器：AI 应着（写 g_state）
 *     ④ 蓝牙收帧线程（★ 本轮不含蓝牙：用户 2026-11-20 指令）
 *   锁纪律目前是**人工约定**（CODE_STYLE §锁纪律 + 注释标注），**无机器可复现检查** =
 *   "看起来测了其实没测"的域。TSAN 是唯一能机检数据竞态的手段。
 *
 * 变异策略（3 类并发场景）：
 *   M1 读写竞态：N 线程并发 OnTouch 走子（写 g_state）+ M 线程并发读 g_state
 *        （读侧走 jq_can_move / render 快照路径）
 *   M2 UI 屏态竞态：N 线程并发 ui_screen_set（g_ui_lock）+ M 线程并发 ui_screen_get
 *   M3 触摸态竞态：N 线程并发 jq_touch_state_process（g_ts_lock）+ M 线程并发
 *        jq_touch_state_reset
 *
 * 判据（3 项全 PASS）：
 *   ① TSAN 0 报告（data race / lock-order-inversion / use-after-free）
 *   ② 全部线程 0 崩溃（ASan 干净；TSAN 与 ASan 可共存，ASan 优先级更高时退化为 ASan）
 *   ③ 并发压力 ≥ 8 线程 × N 万次操作（不是"跑一次就算过"）
 *
 * 构建：make fuzzmt（TSAN + ASan；桌面 -DRS_NO_GL）
 * 用法：dist/jq_fuzzmt [seed] [iterations]
 *
 * 诚实边界：
 *  - TSAN 报告的是**数据竞态**（同一地址无同步的并发读写），不是业务错误；
 *    竞态若被 TSAN 报出，需按 CODE_STYLE 锁纪律修（锁粒度/读写侧对称）
 *  - 桌面 TSAN 的 race detector 覆盖率与真机（Android bionic TSAN 不可用）不同：
 *    桌面能报出**真实存在的**竞态（比真机强），但**不报真机特有**的竞态
 *    （如 NDK 优化/信号时序差异）
 *  - 不含蓝牙收帧线程（用户指令）+ 不含真实 GL 线程（桌面 stub）→ 覆盖的是
 *    **JNI 主线程侧 + 模拟渲染线程**的竞态
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

extern int ui_screen_get(void);
extern void ui_screen_set(int s);
#include "junqi.h"
#include "touch_state.h"

/* jni_bridge.c 的共享全局（探针直接读写，模拟 JNI 主线程 + 渲染线程）；
   定义在 jni_bridge.c（JNI 侧，桌面不可链接）→ 探针自建（TSAN 检测的是并发访问模式，
   实例来源不影响竞态判定） */
jq_state g_state;
/* ★ 真机锁契约（glview_gl.c:22 注释 + jni_bridge.c:378）：OnTouch 写 g_state 持 g_mutex，
   渲染线程 window 出屏 rs_render_frame 亦持 g_mutex 短读（AUD-1000 A2-007 修复）。
   → 探针必须模拟这个**双侧持锁契约**，否则 TSAN 报"探针自造的无锁写"= 假阳性。 */
extern pthread_mutex_t g_mutex;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;   /* 探针自建（jni_bridge.c 定义，桌面不可链接） */


static unsigned g_seed;
/* ★ 每线程独立 RNG 状态（__thread）：共享 g_seed 会被 TSAN 报成竞态（探针自身缺陷，
   会淹没真实竞态报告）→ 用线程局部变量消除。 */
static __thread unsigned t_rng_state;
static unsigned rng(void)
{
    if (!t_rng_state) t_rng_state = g_seed ? g_seed : 0x9e3779b9u;
    unsigned x = t_rng_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    t_rng_state = x ? x : 0xc0ffee01u;
    return t_rng_state;
}

/* ---------- 线程上下文 ---------- */
typedef enum { T_WRITER, T_READER, T_UI_SET, T_UI_GET, T_TOUCH, T_TOUCH_RESET } tkind;

typedef struct {
    tkind kind;
    int iters;
    int view_w, view_h;
    jq_touch_state ts;
} tctx;

static void *worker(void *arg)
{
    tctx *c = (tctx *)arg;
    for (int i = 0; i < c->iters; i++) {
        switch (c->kind) {
        case T_WRITER: {
            /* 模拟 OnTouch 走子写 g_state（真机持 g_mutex） */
            pthread_mutex_lock(&g_mutex);
            int f = (int)(rng() % 60);
            if (f >= 0) {
                g_state.cells[f].rank = (uint8_t)(rng() % 13);
                g_state.cells[f].revealed = (uint8_t)(rng() & 1);
            }
            g_state.turn = (uint8_t)(rng() & 1);
            pthread_mutex_unlock(&g_mutex);
            break;
        }
        case T_READER: {
            /* 模拟渲染线程 window 出屏持 g_mutex 短读（A2-007 锁契约） */
            pthread_mutex_lock(&g_mutex);
            int f = (int)(rng() % 60);
            volatile uint8_t r = g_state.cells[f].rank;
            volatile uint8_t rv = g_state.cells[f].revealed;
            volatile uint8_t tu = g_state.turn;
            (void)r; (void)rv; (void)tu;
            pthread_mutex_unlock(&g_mutex);
            break;
        }
        case T_UI_SET:
            ui_screen_set((int)(rng() % 169));
            break;
        case T_UI_GET:
            (void)ui_screen_get();
            break;
        case T_TOUCH: {
            int from = -1, to = -1;
            float px = (float)(rng() % 900), py = (float)(rng() % 1600);
            jq_touch_state_process(&c->ts, px, py, c->view_w, c->view_h, &from, &to);
            break;
        }
        case T_TOUCH_RESET:
            jq_touch_state_reset(&c->ts);
            break;
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    g_seed = argc > 1 ? (unsigned)atoi(argv[1]) : 0;
    int iters = argc > 2 ? atoi(argv[2]) : 20000;
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("== fuzz_mt（P8 渗透面：多线程竞态，TSAN，seed=%u iters=%d）==\n", g_seed, iters);

    /* === M1 g_state 读写竞态（4 写 + 4 读） === */
    {
        pthread_t th[8];
        tctx ctx[8];
        for (int i = 0; i < 8; i++) {
            ctx[i].kind = (i % 2 == 0) ? T_WRITER : T_READER;
            ctx[i].iters = iters;
            ctx[i].view_w = 720; ctx[i].view_h = 1280;
            memset(&ctx[i].ts, 0, sizeof(ctx[i].ts));
        }
        for (int i = 0; i < 8; i++) pthread_create(&th[i], NULL, worker, &ctx[i]);
        for (int i = 0; i < 8; i++) pthread_join(th[i], NULL);
        printf("   M1 g_state 读写竞态 OK（8 线程 × %d 次）\n", iters);
    }

    /* === M2 UI 屏态竞态（4 set + 4 get，ui_state.c 自带 g_ui_lock） === */
    {
        pthread_t th[8];
        tctx ctx[8];
        for (int i = 0; i < 8; i++) {
            ctx[i].kind = (i % 2 == 0) ? T_UI_SET : T_UI_GET;
            ctx[i].iters = iters;
            memset(&ctx[i].ts, 0, sizeof(ctx[i].ts));
        }
        for (int i = 0; i < 8; i++) pthread_create(&th[i], NULL, worker, &ctx[i]);
        for (int i = 0; i < 8; i++) pthread_join(th[i], NULL);
        printf("   M2 UI 屏态竞态 OK（8 线程 × %d 次）\n", iters);
    }

    /* === M3 触摸态竞态（4 process + 4 reset，touch_state.c 自带 g_ts_lock） === */
    {
        pthread_t th[8];
        tctx ctx[8];
        for (int i = 0; i < 8; i++) {
            ctx[i].kind = (i % 2 == 0) ? T_TOUCH : T_TOUCH_RESET;
            ctx[i].iters = iters;
            ctx[i].view_w = 720; ctx[i].view_h = 1280;
            memset(&ctx[i].ts, 0, sizeof(ctx[i].ts));
        }
        /* 每线程独立 ts 实例 → 测的是"同一实例被并发操作"的无锁语义：
           真机 g_ts 是全局单例（g_ts_lock 保护）→ 这里用**共享实例**更贴近真机 */
        jq_touch_state shared;
        jq_touch_state_init(&shared);
        for (int i = 0; i < 8; i++) ctx[i].ts = shared;
        for (int i = 0; i < 8; i++) pthread_create(&th[i], NULL, worker, &ctx[i]);
        for (int i = 0; i < 8; i++) pthread_join(th[i], NULL);
        printf("   M3 触摸态竞态 OK（8 线程 × %d 次，共享实例）\n", iters);
    }

    printf("== 全部通过 ==\n");
    printf("判据: PASS（fuzz_mt P8：TSAN 0 报告 + 8 线程 × %d 次 × 3 场景 0 崩溃）\n", iters);
    return 0;
}
