/*
 * bt_semtest_dual.c — R12 混跑矩阵双实例对射回归（v3/v2 能力混跑 + seq 去重 + UNDO 全链）
 *
 * 定位：桌面双实例桩（A/B 会话 + 帧通道），验证 R11 全链 v3 协议在混跑场景的
 *       能力探测/seq 去重/UNDO 请求-应答语义。**不链接 jni_bridge**（纯 bt_session
 *       + bt_protocol），桩风格对齐 bt_semtest.c。
 *
 * 场景：
 *   S1 v3↔v3：双端升档互发 v3 帧，seq 双向独立单调 + 去重
 *   S2 v3发↔v2收：A 发 v3，B 未升档（对 v2-only 发端 partial，收端对称升档）
 *   S3 v2发↔v3收：A 仅 v2（seq=0 归一化），B 收 v2 0 退化不升档
 *   S4 RESTART 跨实例窗口边界：A RESTART 复位 12+2 字段，新会话 seq 从 1 重新单调
 *   S5 UNDO 跨实例请求-应答：A REQ → B REQ_OK + 对称回退 + ACK；A 收 ACK 清 pending
 *
 * 诚实边界：桌面双实例对射 = 协议/状态机语义（非真实 SPP MTU 分片/断连时序）；
 *           无 BT 硬件端到端 = 真机 PENDING；跨重启强一致另 R12 主攻③。
 */
#include <stdio.h>
#include "junqi.h"
#include "bt_protocol.h"
#include "bt_session.h"

/* stub（对齐 bt_semtest.c）：__android_log_print 保留 NDK 符号名，桌面桩不导出 */
int __android_log_print(int prio, const char *tag, const char *fmt, ...);
#include <stdarg.h>
int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    (void)prio; (void)tag; (void)fmt; return 0;
}

/* 悔棋栈桩（B04-031 同 bt_semtest）：g_bt_undo_avail 控栈深 */
static int g_bt_undo_avail = 0;
int jq_bt_undo_available(void) { return g_bt_undo_avail; }

/* ---- CHECK 风格（对齐 bt_semtest.c）---- */
static int rc = 0, n_pass = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("  FAIL: %s\n", msg); rc = 1; } \
    else { printf("  ok: %s\n", msg); n_pass++; } \
} while (0)

/* ---- 夹具工厂（对齐 bt_semtest.c）---- */
static void fixture_play(jq_state *st)
{
    jq_state_init(st);
    jq_load_layout(st, JQ_DEFAULT_LAY);
}
static int find_blue_move(const jq_state *st, int *from, int *to)
{
    for (int i = 0; i < N_CELLS; i++)
        if (st->cells[i].rank && st->cells[i].side == SIDE_BLUE && st->cells[i].revealed)
            for (int t = 0; t < N_CELLS; t++)
                if (t != i && jq_can_move(st, i, t)) { *from = i; *to = t; return 1; }
    return 0;
}

/* ---- 帧通道：A 发 v3/v2 帧 → B 收 decode_any + on_frame_v3（双向对射）----
   seq/ref/f1..f6 走归一化视图；v3 帧携带 seq，v2 帧归一化 seq=0。 */
static int a_to_b(jq_bt_session *sB, jq_state *stB, int type, unsigned seq, unsigned ref,
                  int f1, int f2, int f3, int f4, int f5, int f6)
{
    /* 收端直接按归一化视图喂 on_frame_v3（等价 decode_any 后视图） */
    return jq_bt_session_on_frame_v3(sB, stB, type, seq, ref, f1, f2, f3, f4, f5, f6);
}

int main(void)
{
    printf("[S1] v3↔v3 双端升档 + seq 独立单调 + 去重\n");
    {
        jq_state stA, stB; fixture_play(&stA); fixture_play(&stB);
        jq_bt_session sA, sB;
        jq_bt_session_init(&sA, &stA, 1);   /* A 蓝先手 */
        jq_bt_session_init(&sB, &stB, 0);   /* B 红后手 */
        sA.state = sB.state = BT_ST_CONNECTED;
        sA.peer_caps_v3 = sB.peer_caps_v3 = 0;

        /* S1 用 DRAW（无坐标/回合闸）做纯 seq 登记隔离，避免 FLIP/MOVE 坐标域干扰 */
        int rB_draw = a_to_b(&sA, &stA, BT_T_DRAW, 1, 0, 0, 0, 0, 0, 0, 0);
        CHECK(rB_draw == 0 && sA.rx_last_seq == 1 && sA.peer_caps_v3 == 1,
              "S1-1：A 收 B v3 DRAW(seq=1) → 接受 + 登记 rx_last_seq=1 + 升 peer_caps_v3");
        /* A 重放同 seq=1 → 前置判重丢弃（非 UNDO，非旧包） */
        int rB_dup = a_to_b(&sA, &stA, BT_T_DRAW, 1, 0, 0, 0, 0, 0, 0, 0);
        CHECK(rB_dup == 0 && sA.rx_last_seq == 1,
              "S1-2：A 重放 seq=1 → 前置判重静默丢弃（rx_last_seq 不变）");
        /* B 独立 seq 空间：B 收 A 的 v3 DRAW(seq=1)，B 自身 rx_last_seq 独立从 1 起 */
        int rA_draw = a_to_b(&sB, &stB, BT_T_DRAW, 1, 0, 0, 0, 0, 0, 0, 0);
        CHECK(rA_draw == 0 && sB.rx_last_seq == 1,
              "S1-3：B 收 A v3 DRAW(seq=1) 独立 seq 空间（rx_last_seq=1，双向独立单调）");
        /* 推进 B 到 seq=2（新帧放行），再发 seq=1 旧包 → -1 */
        int r_adv = a_to_b(&sB, &stB, BT_T_DRAW, 2, 0, 0, 0, 0, 0, 0, 0);
        int r_old = a_to_b(&sB, &stB, BT_T_DRAW, 1, 0, 0, 0, 0, 0, 0, 0);
        CHECK(r_adv == 0 && sB.rx_last_seq == 2 && r_old == -1,
              "S1-4：B 推进 seq=2 → 旧包 seq=1 < 2 拒 -1（窗口不回退）");
    }

    printf("[S2] v3发↔v2收（对端未升档 partial，收端对称升档）\n");
    {
        jq_state stA, stB; fixture_play(&stA); fixture_play(&stB);
        jq_bt_session sA, sB;
        jq_bt_session_init(&sA, &stA, 1);
        jq_bt_session_init(&sB, &stB, 0);
        sA.state = sB.state = BT_ST_CONNECTED;
        sB.peer_caps_v3 = 0;   /* B 未升档（对 v2-only 发端 partial） */
        /* A 发 v3 帧（seq=1）→ B 收：B 首次收 v3 帧 → 对称升档 peer_caps_v3=1 */
        int r1 = a_to_b(&sB, &stB, BT_T_DRAW, 1, 0, 0, 0, 0, 0, 0, 0);
        CHECK(r1 == 0 && sB.peer_caps_v3 == 1 && sB.caps_locked == 1,
              "S2-1：B（未升档）收 v3 帧 → 对称升档 peer_caps_v3=1 + 锁定");
        /* B 升档后发端将 encode_v3；v2-only 对端 = B 从未发 v3 前 seq_dedup partial */
    }

    printf("[S3] v2发↔v3收（v2 归一化 seq=0 0 退化不升档）\n");
    {
        jq_state stA, stB; fixture_play(&stA); fixture_play(&stB);
        jq_bt_session sA, sB;
        jq_bt_session_init(&sA, &stA, 1);
        jq_bt_session_init(&sB, &stB, 0);
        sA.state = sB.state = BT_ST_CONNECTED;
        sB.peer_caps_v3 = 0;
        /* A 仅 v2（seq=0 归一化）→ B 收 v2 帧 0 退化 + 不升档 */
        int r1 = a_to_b(&sB, &stB, BT_T_DRAW, 0, 0, 0, 0, 0, 0, 0, 0);
        CHECK(r1 == 0 && sB.peer_caps_v3 == 0,
              "S3-1：B 收 v2 帧（seq=0 归一化）→ 0 退化正常消费 + 不升档");
    }

    printf("[S4] RESTART 跨实例窗口边界（复位 12+2 字段，新会话 seq 从 1 重新单调）\n");
    {
        jq_state stA; fixture_play(&stA);
        jq_bt_session sA; jq_bt_session_init(&sA, &stA, 1);
        sA.state = BT_ST_OUR_TURN;
        sA.rx_last_seq = 50; sA.tx_seq = 50; sA.last_req_seq = 7;
        sA.undo_pending = 1; sA.pending_seq = 9; sA.legacy_undo_done = 1;
        sA.peer_caps_v3 = 1; sA.caps_locked = 1;
        int r1 = a_to_b(&sA, &stA, BT_T_RESTART, 51, 0, 999, 1, 0, 0, 0, 0);
        CHECK(r1 == 0 && sA.rx_last_seq == 0 && sA.tx_seq == 0
              && sA.last_req_seq == 0 && sA.undo_pending == 0
              && sA.legacy_undo_done == 0 && sA.peer_caps_v3 == 0 && sA.caps_locked == 0,
              "S4-1：RESTART 成功复位窗口 + UNDO 事务 + 能力位（新会话边界）");
        /* 新会话 seq 从 1 重新单调 */
        sA.state = BT_ST_CONNECTED;
        int r2 = a_to_b(&sA, &stA, BT_T_DRAW, 1, 0, 0, 0, 0, 0, 0, 0);
        CHECK(r2 == 0 && sA.rx_last_seq == 1,
              "S4-2：RESTART 后新会话 seq=1 重新单调（窗口从 0 起）");
    }

    printf("[S5] UNDO 跨实例请求-应答（A REQ → B REQ_OK + 对称回退 + ACK）\n");
    {
        jq_state stA, stB; fixture_play(&stA); fixture_play(&stB);
        jq_bt_session sA, sB;
        jq_bt_session_init(&sA, &stA, 1);
        jq_bt_session_init(&sB, &stB, 0);
        sA.state = sB.state = BT_ST_CONNECTED;
        g_bt_undo_avail = 1;   /* B 桩：栈可悔 */
        /* A 发 UNDO_REQ(seq=5) → B：THEIR_TURN + 可悔 → REQ_OK + 登记 */
        sB.state = BT_ST_THEIR_TURN;
        int r1 = a_to_b(&sB, &stB, BT_T_UNDO, 5, 0, 0, 0, 0, 0, 0, 0);
        CHECK(r1 == JQ_BT_RC_UNDO_REQ_OK && sB.last_req_seq == 5,
              "S5-1：B 收 UNDO_REQ(seq=5) → REQ_OK + 登记 last_req_seq=5");
        /* B 重放同 seq=5 → REQ_DUP（幂等豁免预检直达 case） */
        int r1d = a_to_b(&sB, &stB, BT_T_UNDO, 5, 0, 0, 0, 0, 0, 0, 0);
        CHECK(r1d == JQ_BT_RC_UNDO_REQ_DUP,
              "S5-2：B 重放 seq=5 → REQ_DUP（幂等重发缓存，不二次回退）");
        /* A 收 UNDO_ACK(seq=5, ref=5) → 清 pending */
        sA.undo_pending = 1; sA.pending_seq = 5;
        int r2 = a_to_b(&sA, &stA, BT_T_UNDO, 5, 5, 0, 0, 0, 0, 0, 0);
        CHECK(r2 == 0 && sA.undo_pending == 0,
              "S5-3：A 收 UNDO_ACK(ref=5) → 清 pending（发端事务闭环）");
        g_bt_undo_avail = 0;
    }

    printf(rc ? "== bt_semtest_dual 结果: 有失败（%d 条通过）==\n"
              : "=== bt_semtest_dual 全量 %d 条 PASS ===\n", n_pass);
    return rc;
}
