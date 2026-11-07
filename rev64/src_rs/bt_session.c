/*
 * bt_session.c — 蓝牙联机帧状态机
 * 对齐原版 0xbe9cc 逻辑（收帧→校验→规则引擎→回合翻转→出帧）。
 * 7 字段行为级约定：f1=type, f2=fc, f3=fr, f4=tc, f5=tr, f6=rank, f7=side。
 * type 取值（jq_bt_type）：MOVE=0/FLIP=1/UNDO=2/SYNC=3/SYNC_CELL=4/RESTART=5/RESIGN=6。
 */
#include "bt_session.h"
#include "bt_protocol.h"
#include "ui_state.h"   /* ★ P1（datah_039/107）：ui_replay_record（蓝牙走子/翻棋记录复盘序列） */
#include <string.h>
static jq_state g_undo_snap;   /* ★ AUD-R-013：无写入点的遗留快照（BT_T_UNDO 已不再消费，保留占位） */

/* 坐标 → 索引（原版 0x6ef54：[0]=col [1]=row，cell_index(row,col)） */
static int cell_of(int col, int row, int *out_idx)
{
    if (col < 0 || col >= 5 || row < 0 || row >= 12) return -1;
    *out_idx = cell_index(row, col);
    return *out_idx;
}

/* 校验帧坐标（原版 0x39f20 用 0x6ef54 坐标比较，非法坐标 return） */
static int frame_coord_ok(int fc, int fr, int tc, int tr)
{
    return (fc >= 0 && fc < 5 && fr >= 0 && fr < 12 &&
            tc >= 0 && tc < 5 && tr >= 0 && tr < 12);
}

void jq_bt_session_init(jq_bt_session *s, const jq_state *st, int we_blue)
{
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->state    = BT_ST_CONNECTED;
    s->we_blue  = we_blue;
    s->peer_state = *st;
}

int jq_bt_session_on_frame(jq_bt_session *s, jq_state *st,
                           int f1, int f2, int f3,
                           int f4, int f5, int f6, int f7)
{
    if (!s || !st) return -1;
    s->last_type = f1;

    switch (f1) {
    case BT_T_MOVE: {
        if (!frame_coord_ok(f2, f3, f4, f5)) return -1;
        int from = cell_index(f3, f2), to = cell_index(f5, f4);
        /* 对方回合才接受 MOVE（状态机闸口，对齐原版 0xbe9cc 的回合判定） */
        if (s->state != BT_ST_THEIR_TURN) return -2;
        /* 用规则引擎验对方走子（原版 0x39f20 校验） */
        if (jq_try_move(st, from, to) < 0) return -1;  /* 非法走子拒绝 */
        ui_replay_record(from, to, 0);   /* ★ P1（datah_039/107）：蓝牙对端走子成功记录 1 步 */
        s->state = s->we_blue ? BT_ST_OUR_TURN : BT_ST_OUR_TURN; /* 回合翻转 */
        s->peer_state = *st;
        return 0;
    }
    case BT_T_FLIP: {
        if (!frame_coord_ok(f2, f3, 0, 0)) return -1;
        int idx = cell_index(f3, f2);
        if (s->state != BT_ST_THEIR_TURN) return -2;
        if (jq_flip(st, idx)) ui_replay_record(idx, idx, 1);   /* ★ P1（datah_039/107）：蓝牙对端翻棋记录 */
        s->state = BT_ST_OUR_TURN;
        return 0;
    }
    case BT_T_UNDO: {
        /* ★ AUD-R-013（P1）：UNDO 帧闸口改走 jni_bridge 暴露的 jq_bt_undo_available()
           （读实际悔棋栈 g_undo_top），替代恒 0 的 jq_snapshots_left（旧路径死代码，
           对端收到"悔棋已生效"但本地棋盘不变 → 状态发散）。
           g_undo_snap 在 bt_session 侧无写入点（全 0 空快照），恢复动作由
           jni_bridge 收帧回调侧执行（undo_pop + 广播），本处仅切回合。 */
        if (s->state != BT_ST_THEIR_TURN) return -2;
        if (jq_bt_undo_available()) {
            /* 栈深>0：本地悔棋栈可恢复 → 切回合后由收帧侧同步状态 */
        }
        s->state = BT_ST_OUR_TURN;
        return 0;
    }
    case BT_T_SYNC_CELL: {
        int idx = f2;
        if (idx < 0 || idx >= N_CELLS) return -1;
        s->peer_state.cells[idx].rank = f3;
        s->peer_state.cells[idx].side = f4;
        s->peer_state.cells[idx].revealed = f5 ? 1 : 0;
        return 0;
    }
    case BT_T_SYNC:
        /* 全盘同步：Java 侧按 60 帧拆开，每帧走 SYNC_CELL 路径 */
        s->peer_state = *st;
        return 0;
    case BT_T_RESTART:
        jq_state_init(st);
        s->state = BT_ST_OUR_TURN;
        return 0;
    case BT_T_RESIGN:
        st->winner = s->we_blue ? SIDE_RED : SIDE_BLUE;  /* 对方认输，我赢 */
        st->stage  = STG_OVER;
        s->state  = BT_ST_DISCONNECTED;
        return 0;
    default:
        return -1;
    }
}

int jq_bt_session_send_move(jq_bt_session *s, jq_state *st,
                             int from, int to, int flip, char *buf, int cap)
{
    if (!s || !buf || cap < BT_BUF) return -1;
    if (flip) {
        /* 翻棋帧：type=FLIP, f2=col, f3=row，其余占位 0 */
        int n = jq_bt_frame_encode(buf, cap, BT_T_FLIP,
                                    cell_col(from), cell_row(from),
                                    0, 0, 0, 0, 0);
        if (n < 0) return -1;
        s->state = BT_ST_THEIR_TURN;
        return n;
    }
    /* 走子帧：jq_bt_move_frame（bt_protocol.c 现成，语义对齐原版 0x26f24） */
    jq_event ev = 0;
    int n = jq_bt_move_frame(buf, cap, st, from, to, ev);
    if (n < 0) return -1;
    s->state = BT_ST_THEIR_TURN;
    return n;
}

int jq_bt_session_send_sync(jq_bt_session *s, jq_state *st,
                             char *buf, int cap_per_frame)
{
    if (!s || !buf) return -1;
    int n = jq_bt_frame_sync_encode(buf, cap_per_frame, st, 0, N_CELLS);
    s->state = BT_ST_THEIR_TURN;
    return n;
}
