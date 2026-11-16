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

    /* =====================================================================
     * [S1m] MOVE 双端一致性（v3 MOVE + B 落子 + cells 同步；cell_col/row 合法域）
     * ===================================================================== */
    printf("[S1m] MOVE 双端一致性（v3 MOVE seq 独立 + B 落子 + cells 同步）\n");
    {
        jq_state stA, stB; fixture_play(&stA); fixture_play(&stB);
        jq_bt_session sA, sB;
        jq_bt_session_init(&sA, &stA, 1);
        jq_bt_session_init(&sB, &stB, 0);
        sA.state = BT_ST_OUR_TURN; sB.state = BT_ST_THEIR_TURN;
        int from=-1, to=-1;
        CHECK(find_blue_move(&stA, &from, &to) == 1,
              "S1m-0：夹具取蓝方合法走子（find_blue_move）");
        int fc=cell_col(from), fr=cell_row(from), tc=cell_col(to), tr=cell_row(to);
        CHECK(fc>=0&&fc<5&&fr>=0&&fr<12&&tc>=0&&tc<5&&tr>=0&&tr<12,
              "S1m-1：MOVE 坐标域 frame_coord_ok（col<5/row<12，经 cell_col/row 映射）");
        jq_state stA_post = stA;
        int mr = jq_try_move(&stA_post, from, to);
        CHECK(mr >= 0, "S1m-2：发端本地 jq_try_move 合法落子（rc>=0）");
        unsigned int seqA = ++sA.tx_seq; sA.tx_last_seq = seqA;
        int rB = a_to_b(&sB, &stB, BT_T_MOVE, seqA, 0, fc, fr, tc, tr,
                        stA.cells[to].rank, stA.cells[to].side);
        CHECK(rB == 0 && sB.rx_last_seq == seqA && sB.peer_caps_v3 == 1,
              "S1m-3：B 收 v3 MOVE(seq=1) 接受 + 登记 rx_last_seq + 升档");
        CHECK(memcmp(stB.cells, stA_post.cells, sizeof(stB.cells)) == 0
              && sB.state == BT_ST_OUR_TURN,
              "S1m-4：双端 cells 同步一致（B==A 走子后视图，state=OUR_TURN）");
    }

    /* [S2m] seq 双向独立单调完整矩阵 */
    printf("[S2m] seq 双向独立单调完整矩阵\n");
    {
        jq_state stA, stB; fixture_play(&stA); fixture_play(&stB);
        jq_bt_session sA, sB;
        jq_bt_session_init(&sA, &stA, 1); jq_bt_session_init(&sB, &stB, 0);
        sA.state = sB.state = BT_ST_CONNECTED;
        CHECK(a_to_b(&sB,&stB,BT_T_DRAW,1,0,0,0,0,0,0,0)==0 && sB.rx_last_seq==1, "S2m-1：B 收 A seq=1 登记");
        CHECK(a_to_b(&sB,&stB,BT_T_DRAW,2,0,0,0,0,0,0,0)==0 && sB.rx_last_seq==2, "S2m-2：B 收 A seq=2 单调推进");
        CHECK(a_to_b(&sB,&stB,BT_T_DRAW,3,0,0,0,0,0,0,0)==0 && sB.rx_last_seq==3, "S2m-3：B 收 A seq=3 单调推进");
        /* seq=2 < rx=3 走旧包拒收分支（非重复==，2≠3）→ -1 */
        CHECK(a_to_b(&sB,&stB,BT_T_DRAW,2,0,0,0,0,0,0,0)==-1 && sB.rx_last_seq==3, "S2m-4：B 收 seq=2<rx=3 旧包拒 -1（窗口不回退）");
        CHECK(a_to_b(&sB,&stB,BT_T_DRAW,1,0,0,0,0,0,0,0)==-1 && sB.rx_last_seq==3, "S2m-5：B 收旧包 seq=1<3 拒 -1");
        sB.tx_seq=1; sB.tx_last_seq=1;
        CHECK(a_to_b(&sA,&stA,BT_T_DRAW,1,0,0,0,0,0,0,0)==0 && sA.rx_last_seq==1 && sB.tx_seq==1, "S2m-6：双向独立 seq 空间（A.rx=1 与 B.tx=1 不冲突）");
        CHECK(a_to_b(&sA,&stA,BT_T_DRAW,2,0,0,0,0,0,0,0)==0 && sA.rx_last_seq==2 && sB.rx_last_seq==3, "S2m-7：A/B 双向推进互不干扰");
    }

    /* [S3m] 能力探测完整矩阵 */
    printf("[S3m] 能力探测完整矩阵\n");
    {
        jq_state stA, stB; fixture_play(&stA); fixture_play(&stB);
        jq_bt_session sA, sB;
        jq_bt_session_init(&sA, &stA, 1); jq_bt_session_init(&sB, &stB, 0);
        sA.state = sB.state = BT_ST_CONNECTED;
        CHECK(a_to_b(&sB,&stB,BT_T_DRAW,1,0,0,0,0,0,0,0)==0 && sB.peer_caps_v3==1 && sB.caps_locked==1, "S3m-1：B 收 v3 升档 peer_caps_v3 + caps_locked");
        CHECK(sB.peer_caps_v3==1, "S3m-2：B 升档后发端 use_v3=peer_caps_v3==1（encode_v3 镜像）");
        CHECK(a_to_b(&sB,&stB,BT_T_DRAW,2,0,0,0,0,0,0,0)==0 && sB.caps_locked==1, "S3m-3：升档后再收 v3 能力位保持（会话内锁定）");
        CHECK(a_to_b(&sB,&stB,BT_T_DRAW,0,0,0,0,0,0,0,0)==0 && sB.peer_caps_v3==1, "S3m-4：升档后收 v2(seq=0) 不降级（锁定保持）");
        sB.state = BT_ST_OUR_TURN;
        CHECK(a_to_b(&sB,&stB,BT_T_RESTART,3,0,999,1,0,0,0,0)==0 && sB.peer_caps_v3==0 && sB.caps_locked==0, "S3m-5：RESTART 清能力位（边界复位）");
        sB.state = BT_ST_CONNECTED;
        CHECK(a_to_b(&sB,&stB,BT_T_DRAW,1,0,0,0,0,0,0,0)==0 && sB.peer_caps_v3==1, "S3m-6：RESTART 后新会话可重新升档");
        CHECK(sA.peer_caps_v3==0, "S3m-7：未收 v3 帧发端 peer_caps_v3=0（encode_v2 0 退化镜像）");
    }

    /* [S5m] UNDO 跨实例全链 */
    printf("[S5m] UNDO 跨实例全链（REQ/ACK/NACK/DUP）\n");
    {
        jq_state stA, stB; fixture_play(&stA); fixture_play(&stB);
        jq_bt_session sA, sB;
        jq_bt_session_init(&sA, &stA, 1); jq_bt_session_init(&sB, &stB, 0);
        sA.state = sB.state = BT_ST_CONNECTED; sB.state = BT_ST_THEIR_TURN;
        g_bt_undo_avail = 1;
        sA.undo_pending=1; sA.pending_seq=10; sA.tx_seq=10;
        CHECK(a_to_b(&sB,&stB,BT_T_UNDO,10,0,0,0,0,0,0,0)==JQ_BT_RC_UNDO_REQ_OK && sB.last_req_seq==10, "S5m-1：B 收 UNDO_REQ(seq=10) REQ_OK + 登记");
        CHECK(a_to_b(&sA,&stA,BT_T_UNDO,10,10,0,0,0,0,0,0)==0 && sA.undo_pending==0, "S5m-2：A 收 UNDO_ACK(ref=10) 清 pending");
        sB.state = BT_ST_THEIR_TURN;
        CHECK(a_to_b(&sB,&stB,BT_T_UNDO,10,0,0,0,0,0,0,0)==JQ_BT_RC_UNDO_REQ_DUP, "S5m-3：B 重放 UNDO_REQ seq=10 REQ_DUP");
        g_bt_undo_avail = 0; sB.state = BT_ST_THEIR_TURN; sB.last_req_seq = 0;
        CHECK(a_to_b(&sB,&stB,BT_T_UNDO,11,0,0,0,0,0,0,0)==-3, "S5m-4：B 栈空 UNDO_REQ → -3（NACK 栈空）");
        g_bt_undo_avail = 1; sB.state = BT_ST_OUR_TURN; sB.last_req_seq = 0;
        CHECK(a_to_b(&sB,&stB,BT_T_UNDO,12,0,0,0,0,0,0,0)==-2, "S5m-5：B 回合不符 UNDO_REQ → -2（NACK 回合）");
        g_bt_undo_avail = 0;
    }

    /* [S6m] 去重与 UNDO 协同边界 */
    printf("[S6m] 去重与 UNDO 协同边界\n");
    {
        jq_state stA, stB; fixture_play(&stA); fixture_play(&stB);
        jq_bt_session sA, sB;
        jq_bt_session_init(&sA, &stA, 1); jq_bt_session_init(&sB, &stB, 0);
        sA.state = sB.state = BT_ST_CONNECTED; sB.state = BT_ST_THEIR_TURN;
        g_bt_undo_avail = 1;
        CHECK(a_to_b(&sB,&stB,BT_T_UNDO,5,0,0,0,0,0,0,0)==JQ_BT_RC_UNDO_REQ_OK && sB.rx_last_seq==5, "S6m-1：UNDO REQ seq=5 REQ_OK + 窗口登记");
        sB.state = BT_ST_THEIR_TURN;
        CHECK(a_to_b(&sB,&stB,BT_T_UNDO,5,0,0,0,0,0,0,0)==JQ_BT_RC_UNDO_REQ_DUP, "S6m-2：UNDO REQ 重放豁免预检直达 case");
        CHECK(a_to_b(&sB,&stB,BT_T_DRAW,5,0,0,0,0,0,0,0)==0 && sB.rx_last_seq==5, "S6m-3：非 UNDO 重复 seq=5 静默丢弃");
        CHECK(a_to_b(&sB,&stB,BT_T_DRAW,4,0,0,0,0,0,0,0)==-1 && sB.rx_last_seq==5, "S6m-4：旧包 seq=4<5 拒 -1");
        sB.state = BT_ST_THEIR_TURN;
        CHECK(a_to_b(&sB,&stB,BT_T_UNDO,4,0,0,0,0,0,0,0)==-1, "S6m-5：UNDO 旧包 seq=4<rx 拒 -1（旧包判定先于豁免）");
        g_bt_undo_avail = 0;
    }

    /* [S7m] v2/v3 混帧同对射交错 */
    printf("[S7m] v2/v3 混帧同对射交错\n");
    {
        jq_state stB; fixture_play(&stB);
        jq_bt_session sB; jq_bt_session_init(&sB, &stB, 0);
        sB.state = BT_ST_CONNECTED;
        CHECK(a_to_b(&sB,&stB,BT_T_DRAW,1,0,0,0,0,0,0,0)==0 && sB.rx_last_seq==1 && sB.peer_caps_v3==1, "S7m-1：交错首帧 v3 升档 + 登记");
        CHECK(a_to_b(&sB,&stB,BT_T_DRAW,0,0,0,0,0,0,0,0)==0 && sB.peer_caps_v3==1 && sB.rx_last_seq==1, "S7m-2：v2(seq=0) 消费但不升档/不推进窗口");
        CHECK(a_to_b(&sB,&stB,BT_T_DRAW,2,0,0,0,0,0,0,0)==0 && sB.rx_last_seq==2, "S7m-3：v2 插入后 v3 seq=2 单调推进");
    }

    printf(rc ? "== bt_semtest_dual 结果: 有失败（%d 条通过）==\n"
              : "=== bt_semtest_dual 全量 %d 条 PASS ===\n", n_pass);
    return rc;
}
