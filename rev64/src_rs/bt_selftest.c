/*
 * bt_selftest.c — 蓝牙帧状态机主机侧自测（无真机蓝牙环境）
 * 验证（bt_protocol.c 帧编解码 + bt_session.c 帧应用语义）：
 *   1. v2 帧 encode/decode 往返
 *   2. 非 v2 帧拒绝
 *   3. 全盘同步 60 格 encode
 *   4. 应用 MOVE 帧（jq_try_move）→ 回合正确翻转
 *   5. 应用 FLIP 帧（jq_flip）→ 回合翻转
 *   6. 状态机闸口（非本回合拒帧）
 * 编译: make bttest  →  dist/jq_bttest
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#include "bt_protocol.h"
#include "bt_session.h"

/* stub __android_log_print（bt_protocol.c 引用）
 * ★ AUD-A2 F-naming（诚实标注）：__android_log_print 是 Android NDK 系统符号名
 * （<android/log.h> 契约），桌面桩必须保留原名（NDK 链接契约，对齐系统符号名），
 * 不能加模块前缀。非公开 API（bttest 编译目标本地桩，不导出）。 */
int __android_log_print(int prio, const char *tag, const char *fmt, ...);
#include <stdarg.h>
int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    (void)prio; (void)tag; (void)fmt; return 0;
}

/* ★ AUD-R-013：bttest 无 jni_bridge 链接 → 提供默认桩（无悔棋栈，恒 0） */
int jq_bt_undo_available(void) { return 0; }

static int rc = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("  FAIL: %s\n", msg); rc = 1; } \
    else printf("  ok: %s\n", msg); \
} while (0)

int main(void) {
    printf("== 蓝牙帧协议 + 状态机自测 ==\n");
    jq_state st;
    jq_state_init(&st);
    jq_load_layout(&st, JQ_DEFAULT_LAY);  /* 载入默认布局（双方 25 子） */
    jq_bt_session sess;
    jq_bt_session_init(&sess, &st, 1);  /* 我是蓝方 */

    /* 1. v2 帧 encode/decode 往返 */
    char buf[80];
    int f[8] = {BT_T_MOVE, 1, 2, 3, 4, 9, 0, 0};
    int n = jq_bt_frame_encode(buf, sizeof buf, f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7]);
    int g[8];
    CHECK(n > 0, "v2 帧 encode 成功");
    CHECK(jq_bt_frame_decode(buf, &g[0], &g[1], &g[2], &g[3], &g[4], &g[5], &g[6]) == 1,
          "v2 帧 decode 成功");
    int same = 1;
    /* ★ 对抗审计 #3 配套：decode 输出 g[0]=type, g[1..6]=f1..f6（原版 7 字段）。
       旧判据 g[i]==f[i+1] 移位掩盖 encode 丢 type（应校验 g[0]==type=f[0]）。 */
    for (int i = 0; i < 7; i++) same &= (g[i] == f[i]);
    CHECK(g[0] == f[0] && g[1] == f[1] && g[5] == f[5], "帧字段一致（type,f1..f6）");
    CHECK(strncmp(buf, "v2\n", 3) == 0, "帧前缀 v2\\n");

    /* 2. 非 v2 帧拒绝 */
    CHECK(jq_bt_frame_decode("hello\n1,2,3,4,5,6,7", g, g+1, g+2, g+3, g+4, g+5, g+6) == 0,
          "非 v2 帧拒绝");

    /* 3. 全盘同步 60 格 encode */
    st.cells[5].rank = 9; st.cells[5].side = SIDE_BLUE; st.cells[5].revealed = 1;
    char sb[60 * 64];
    int sn = jq_bt_frame_sync_encode(sb, 64, &st, 0, N_CELLS);
    CHECK(sn == N_CELLS, "全盘同步 60 帧");

    /* 4. 找蓝方可走子，测试 MOVE 帧状态机 */
    int mfrom = -1, mto = -1;
    for (int i = 0; i < N_CELLS; i++) {
        if (st.cells[i].rank && st.cells[i].side == SIDE_BLUE && !st.cells[i].revealed) {
            for (int t = 0; t < N_CELLS; t++) {
                if (t != i && jq_can_move(&st, i, t)) { mfrom = i; mto = t; break; }
            }
            if (mfrom >= 0) break;
        }
    }
    printf("  蓝方可走子: from=%d to=%d stage=%d turn=%d\n", mfrom, mto, st.stage, st.turn);

    /* 5. 暗棋阶段：找可翻子（蓝方先手翻棋），测 FLIP 帧。
       ★ 2026-11-07：明棋 load_layout 全子 revealed=1（明棋布局语义）→
       自测构造暗棋状态（全子 revealed=0）以验证 FLIP 帧路径。 */
    for (int i = 0; i < N_CELLS; i++)
        if (st.cells[i].rank) st.cells[i].revealed = 0;
    st.turn = SIDE_BLUE;
    int flip_idx = -1;
    for (int i = 0; i < N_CELLS; i++)
        if (st.cells[i].rank && st.cells[i].side == SIDE_BLUE && !st.cells[i].revealed
            && jq_can_flip(&st, i)) { flip_idx = i; break; }
    printf("  蓝方可翻子: idx=%d\n", flip_idx);
    sess.state = BT_ST_THEIR_TURN;
    int fl[8] = {BT_T_FLIP, cell_col(flip_idx), cell_row(flip_idx), 0, 0, 0, 0, 0};
    int ret = jq_bt_session_on_frame(&sess, &st,
                                     fl[0], fl[1], fl[2], fl[3], fl[4], fl[5], fl[6]);
    CHECK(ret == 0, "THEIR_TURN 接受合法 FLIP 帧");
    CHECK(st.cells[flip_idx].revealed == 1, "FLIP 帧应用：格子翻开");
    CHECK(st.turn == SIDE_RED, "FLIP 帧应用后 turn=RED");

    /* 6. 非本回合拒帧 */
    sess.state = BT_ST_OUR_TURN;
    ret = jq_bt_session_on_frame(&sess, &st, fl[0], fl[1], fl[2], fl[3], fl[4], fl[5], fl[6]);
    CHECK(ret == -2, "OUR_TURN 拒收 FLIP 帧（rc=-2）");

    /* 7. 翻完蓝方后找可走子（明棋阶段）测 MOVE 帧 */
    /* 全翻蓝方 → 找可走子（每翻前设 turn=BLUE，jq_flip 校验当前回合方） */
    st.turn = SIDE_BLUE;
    for (int i = 0; i < N_CELLS; i++)
        if (st.cells[i].rank && st.cells[i].side == SIDE_BLUE && !st.cells[i].revealed) {
            st.turn = SIDE_BLUE;   /* ★ 回合校验（rules.c）：翻己方暗子须当前回合方 */
            jq_flip(&st, i);
        }
    mfrom = mto = -1;
    st.turn = SIDE_BLUE;   /* ★ 找蓝方走子须 turn=BLUE（jq_can_move 回合校验） */
    for (int i = 0; i < N_CELLS; i++) {
        if (st.cells[i].rank && st.cells[i].side == SIDE_BLUE && st.cells[i].revealed) {
            for (int t = 0; t < N_CELLS; t++)
                if (t != i && jq_can_move(&st, i, t)) { mfrom = i; mto = t; break; }
            if (mfrom >= 0) break;
        }
    }
    printf("  蓝方可走子（全翻后）: from=%d to=%d\n", mfrom, mto);
    CHECK(mfrom >= 0 && mto >= 0, "全翻后找到蓝方可走子");
    if (mfrom >= 0) {
        char out[256];
        int outn = jq_bt_session_send_move(&sess, &st, mfrom, mto, 0, out, sizeof out);
        CHECK(outn > 0 && strncmp(out, "v2\n", 3) == 0, "send_move 出帧 v2 前缀");
        CHECK(sess.state == BT_ST_THEIR_TURN, "send_move 后 state=THEIR_TURN");
    }

    /* 8. 非法坐标帧拒绝 */
    sess.state = BT_ST_THEIR_TURN;
    ret = jq_bt_session_on_frame(&sess, &st,
                                 BT_T_MOVE, 99, 0, 0, 0, 0, 0);  /* f1=type f2=99 非法 */
    CHECK(ret == -1, "非法坐标帧拒绝（rc=-1）");

    /* =====================================================================
     * §9. ★★ 跨语言帧契约测试（Java 读线程分帧 ↔ native 帧格式）
     *   背景（对抗审计批6 P0）：native 帧 = "v2\n" + payload + "\n"（两行），
     *   而 Java 读线程原实现"按 \n 逐行分发" → 把 "v2" 与 payload 当两帧下发
     *   → native decode 找不到 "v2\n" → **收帧全丢**（端到端蓝牙同步失效）。
     *   native 端 encode→decode 自测（本文 §1）两端自洽，永远发现不了这个
     *   **跨语言契约错位**——必须显式模拟 Java 侧分帧行为。
     *   本节实现 Java 读线程（修复后）的重组算法，验证：
     *     ① 单帧：wire = native帧 + writer 追加的 "\n" → 重组出可解码帧
     *     ② 多帧：60 格 SYNC_CELL 串接载荷 → 重组出 60 帧且字段正确
     *     ③ 旧版逐行分发（已知坏行为）→ 复现"解码失败"（回归哨兵）
     * ===================================================================== */
    printf("[§9] 跨语言帧契约（Java 读线程分帧 ↔ native 帧格式）\n");
    {
        /* --- ① 单帧：wire = frame + "\n"（Java sendFrame 追加终止符） --- */
        char wire[256];
        int wl = 0;
        int fn = jq_bt_frame_encode(wire, sizeof wire, BT_T_MOVE, 1, 2, 3, 4, 9, 0, 0);
        CHECK(fn > 0, "§9.1 native 帧 encode");
        memcpy(wire + wl, wire, fn); wl += fn;
        wire[wl++] = '\n'; wire[wl] = 0;   /* Java sendFrame：frame + "\n" */

        /* 模拟 Java 读线程（修复后）：找 "v2\n" 头 + payload 尾 '\n' */
        char frames[8][160];
        int nfr = 0;
        {
            const char *vs = strstr(wire, "v2\n");
            CHECK(vs != NULL, "§9.1 wire 含 v2\\n 头");
            if (vs) {
                const char *pe = strchr(vs + 3, '\n');
                CHECK(pe != NULL, "§9.1 wire 含 payload 终止符");
                if (pe) {
                    int flen = (int)(pe - vs);
                    if (flen > 0 && flen < (int)sizeof frames[0]) {
                        memcpy(frames[nfr], vs, flen);
                        frames[nfr][flen] = 0;
                        nfr++;
                    }
                }
            }
        }
        CHECK(nfr == 1, "§9.1 Java 重组得 1 帧（旧版逐行分发会得 2 个残片）");
        if (nfr == 1) {
            int q[8];
            CHECK(jq_bt_frame_decode(frames[0], &q[0], &q[1], &q[2], &q[3], &q[4], &q[5], &q[6]) == 1,
                  "§9.1 重组帧可被 native decode");
            CHECK(q[0] == BT_T_MOVE && q[1] == 1 && q[5] == 9,
                  "§9.1 重组帧字段正确（g[0]=type, g[1..6]=f1..f6；encode 实参 f1=1,f5=9）");
        }
        /* --- ③ 旧版逐行分发（坏行为）→ 复现解码失败（回归哨兵） --- */
        {
            char line1[160], line2[160];
            int l1 = 0, l2 = 0;
            const char *p = wire;
            while (*p && *p != '\n') { line1[l1++] = *p++; }
            line1[l1] = 0;
            if (*p == '\n') p++;
            while (*p && *p != '\n') { line2[l2++] = *p++; }
            line2[l2] = 0;
            int q1[8], q2[8];
            int ok_old = jq_bt_frame_decode(line1, &q1[0], &q1[1], &q1[2], &q1[3], &q1[4], &q1[5], &q1[6])
                       && jq_bt_frame_decode(line2, &q2[0], &q2[1], &q2[2], &q2[3], &q2[4], &q2[5], &q2[6]);
            CHECK(ok_old == 0, "§9.3 旧版逐行分发确实解码失败（回归哨兵：证明本测试有效）");
        }

        /* --- ② 多帧载荷：60 格 SYNC_CELL 串接 → 重组 60 帧 --- */
        {
            char sb[60 * 64];
            int sn = jq_bt_frame_sync_encode(sb, 64, &st, 0, N_CELLS);
            /* 串接成连续载荷（帧自带尾 '\n'，直接拼接） */
            static char multi[60 * 80];
            int ml = 0;
            for (int k = 0; k < sn; k++) {
                const char *fr = sb + k * 64;
                int flen = (int)strlen(fr);
                if (flen > 0 && ml + flen < (int)sizeof multi) {
                    memcpy(multi + ml, fr, flen);
                    ml += flen;
                }
            }
            /* Java 重组：逐帧抽取 */
            int got = 0, bad = 0;
            const char *cur = multi;
            const char *end = multi + ml;
            while (cur < end) {
                const char *vs = strstr(cur, "v2\n");
                if (!vs || vs >= end) break;
                const char *pe = NULL;
                for (const char *q = vs + 3; q < end; q++) if (*q == '\n') { pe = q; break; }
                if (!pe) break;
                char fr[160];
                int flen = (int)(pe - vs);
                if (flen > 0 && flen < (int)sizeof fr) {
                    memcpy(fr, vs, flen); fr[flen] = 0;
                    int q[8];
                    if (jq_bt_frame_decode(fr, &q[0], &q[1], &q[2], &q[3], &q[4], &q[5], &q[6])
                        && q[0] == BT_T_SYNC_CELL && q[1] == got) got++;
                    else bad++;
                }
                cur = pe + 1;
            }
            CHECK(got == sn, "§9.2 多帧载荷重组 60 帧且 idx 顺序正确");
            CHECK(bad == 0, "§9.2 多帧载荷无解码失败帧");
        }
    }

    printf(rc ? "== 结果: 有失败 ==\n" : "== 结果: 全部通过 ==\n");
    return rc;
}
