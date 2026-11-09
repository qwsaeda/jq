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
/* ★ 对抗审计 P3（批3 P3-B19）：原 static jq_state g_undo_snap 死代码
   （无写入/读取点，jq_state 体量不小白占 BSS）。删除。
   UNDO 悔棋栈实际在 jni_bridge.c g_undo[]（jq_bt_undo_available 读 g_undo_top）。 */

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
    /* ★ AUD-1000 B04-014（P1 已修）：last_type 仅合法 type（switch case 命中）后
       才记录——旧实现 switch 前先写 s->last_type=f1，type 越界（default 返 -1）
       时 last_type 已被越界值污染，诊断日志按 last_type 判"最后有效帧"误判。 */

    /* ★ AUD-1000 B04-013（P1 已修）：DISCONNECTED 真终态闸口——认输/断连后
       数据帧（MOVE/FLIP/UNDO/SYNC_CELL/SYNC）拒收；RESTART/RESIGN 放行。 */
    if (s->state == BT_ST_DISCONNECTED &&
        f1 != BT_T_RESTART && f1 != BT_T_RESIGN)
        return -2;
    switch (f1) {
    case BT_T_MOVE: {
        s->last_type = f1;   /* ★ B04-014（P1 已修）：仅合法 case 命中才记 last_type */
        if (!frame_coord_ok(f2, f3, f4, f5)) return -1;
        int from = cell_index(f3, f2), to = cell_index(f5, f4);
        /* 对方回合才接受 MOVE（状态机闸口，对齐原版 0xbe9cc 的回合判定） */
        if (s->state != BT_ST_THEIR_TURN) return -2;
        /* 用规则引擎验对方走子（原版 0x39f20 校验） */
        if (jq_try_move(st, from, to) < 0) return -1;  /* 非法走子拒绝 */
        ui_replay_record(from, to, 0);   /* ★ P1（datah_039/107）：蓝牙对端走子成功记录 1 步 */
        /* ★ 对抗审计 P1（批3 P1-B6）：原 `we_blue ? OUR : OUR` 死三元（两分支
           同值，we_blue 判定无效笔误残留）。语义：收对端 MOVE/FLIP 后恒轮本方
           走 → 直接置 OUR_TURN，去无意义三元。 */
        s->state = BT_ST_OUR_TURN;
        s->peer_state = *st;
        return 0;
    }
    case BT_T_FLIP: {
        s->last_type = f1;   /* ★ B04-014（P1 已修）：仅合法 case 命中才记 last_type */
        if (!frame_coord_ok(f2, f3, 0, 0)) return -1;
        int idx = cell_index(f3, f2);
        if (s->state != BT_ST_THEIR_TURN) return -2;
        if (jq_flip(st, idx)) ui_replay_record(idx, idx, 1);   /* ★ P1（datah_039/107）：蓝牙对端翻棋记录 */
        s->state = BT_ST_OUR_TURN;
        return 0;
    }
    case BT_T_UNDO: {
        s->last_type = f1;   /* ★ B04-014（P1 已修）：仅合法 case 命中才记 last_type */
        /* ★ AUD-R-013（P1）：UNDO 帧闸口改走 jni_bridge 暴露的 jq_bt_undo_available()
           （读实际悔棋栈 g_undo_top），替代恒 0 的 jq_snapshots_left（旧路径死代码）。
           诚实边界（★ 对抗审计 P1 批3 P1-B9/P1-J12）：本地悔棋回退（undo_pop +
           广播）由 jni_bridge 收帧回调侧执行，本 case 仅切回合。若 jni_bridge
           收帧侧未接 undo_pop → 双向悔棋状态发散（本地棋盘与对端已回退的棋盘
           不一致）——当前为 PENDING 缺口，logcat 可诊断。
            ★ AUD-1000 B04-023（P1 已注释，诚实边界）：UNDO 消费后 s->state 切
            OUR_TURN 但本地 undo_pop 不联动——收帧侧（jni_bridge 约 :8047）
            UNDO rc==0 才 undo_pop + 广播 send_sync 全盘对账；on_frame 切
            回合与收帧侧恢复两处分离，漏接 undo_pop 则本地棋盘与对端已回
            退态发散。顺序确认：rc==-3 拒收（栈空）时 state 不切（正确），
            rc==0 消费后 state 已切，恢复靠收帧侧 send_sync 全量还原兜底。 */
        if (s->state != BT_ST_THEIR_TURN) return -2;
        if (!jq_bt_undo_available()) {
            /* 栈空：拒绝（对端误发 UNDO 或本端无可悔步） */
            return -3;
        }
        s->state = BT_ST_OUR_TURN;
        return 0;
    }
    case BT_T_SYNC_CELL: {
        s->last_type = f1;   /* ★ B04-014（P1 已修）：仅合法 case 命中才记 last_type */
        /* ★ AUD-R-084（P2）：同步帧 side/rank 合法性白名单过滤——防止越界/非法值
           污染规则引擎 side 语义（AUD-R-004 pieces_left[255] 越界写 / AUD-R-047
           rank_norm 错误还原同源）。
           帧字段（bt_protocol.h 约定）：f2=idx, f3=rank, f4=side, f5=revealed
           （sync 编码 jq_bt_frame_sync_encode 每格一帧），故白名单过滤 f3/f4/f5。
           rank∈{0..24}（蓝 0-12 / 红编码 13-24）、side∈{0,1,0xFF}，超域拒收。
           ★ AUD-1000 B04-010（P0 已修）：旧白名单 f3∈[0,24] 与 f4（side）独立判定，
             蓝方侧（f4=0/0xFF）rank>12 是脏域（蓝合法域 [0,12]，+0xc 编码仅红方），
             rank_norm 钳位后与真军旗同值（B01-010/033/061 同源）。修：side 感知域
             校验——蓝/未定侧 f3∈[0,12]，红侧 f3∈[0,24]，超域拒收。 */
        int idx = f2;
        if (idx < 0 || idx >= N_CELLS) return -1;
        if (f4 != 0 && f4 != 1 && f4 != SIDE_UNKNOWN) return -1;
        /* ★ AUD-1000 B04-011（P1 诚实标注）：side/rank 联合域——现行 rmax 逻辑
           即"联合校验"：f4==1（红侧）rank∈[0,24]（含 +0xc 红编码 13-24）；
           f4==0（蓝）/0xFF（未定色）rank∈[0,12]（未知侧不能存红编码，
           "红编码 3 误标未定色" rank=15 已被 rmax=RANK_TOP 拦）。
           B04-011 所指的"独立判定放行 0xFF+15"缺陷已被 B04-010 P0 修复覆盖，
           本条 = 行为已闭合，注释锚点防复发。 */
        int rmax = (f4 == 1) ? RANK_TOP + 0xc : RANK_TOP;
        if (f3 < 0 || f3 > rmax) return -1;
        s->peer_state.cells[idx].rank = (uint8_t)f3;
        s->peer_state.cells[idx].side = (uint8_t)f4;
        s->peer_state.cells[idx].revealed = f5 ? 1 : 0;
        return 0;
    }
    case BT_T_SYNC:
        s->last_type = f1;   /* ★ B04-014（P1 已修）：仅合法 case 命中才记 last_type */
        /* ★ 对抗审计 P1（批3 P1-B5）：BT_T_SYNC 原 `s->peer_state = *st` 语义错
           （把本地 state 拷进 peer_state = "对端就是本地镜像"，并非收到对端同步帧；
           且对端误发 type=3 时本端忽略对端真实数据还谎报消费成功 → 状态发散）。
           实际全盘同步走 BT_T_SYNC_CELL 批量帧（Java 侧 jq_bt_session_send_sync 按 60 帧
           拆开，收端走 SYNC_CELL 逐格更新），BT_T_SYNC 无发送点 = 死代码。
           修正：收到 BT_T_SYNC 只重置 peer_state 为初始态（等待后续 SYNC_CELL 填充），
           不拷本地 state。 */
        jq_state_init(&s->peer_state);
        return 0;
    case BT_T_RESTART:
        /* ★ AUD-1000 B04-012（P0 已修）：RESTART 加 DISCONNECTED 闸口——旧实现
           无 s->state 校验，对端在已认输/断连（DISCONNECTED）态发 RESTART 仍重置
           本地棋盘（jq_state_init+强置 OUR_TURN），且 bt_semtest §5 用已 RESIGN 后
           的 sessr 发 RESTART 也通过（掩盖无闸口）。修正：仅对局进行中
           （CONNECTED 态：OUR_TURN/THEIR_TURN）接受 RESTART，DISCONNECTED 终态拒收。 */
        if (s->state == BT_ST_DISCONNECTED) return -2;
        s->last_type = f1;   /* ★ B04-014（P1 已修）：仅合法 case 命中才记 last_type */
        jq_state_init(st);
        s->state = BT_ST_OUR_TURN;
        return 0;
    case BT_T_RESIGN:
        s->last_type = f1;   /* ★ B04-014（P1 已修）：仅合法 case 命中才记 last_type */
        /* ★ AUD-BT-RESIGN（P0 真修）：方向纠正——RESIGN 帧是**对端**发给
           本端的"对端认输"通知，winner 应为**本端阵营**（we_blue=1 我蓝 →
           蓝胜 SIDE_BLUE；we_blue=0 我红 → 红胜 SIDE_RED）。旧式
           `we_blue ? SIDE_RED : SIDE_BLUE` 恰好相反（注释"对方认输，我赢"
           与代码矛盾），致蓝牙对局认输后 winner 误判为负方。
           对照 jni_bridge 本地认输（g_state.winner = 1 - g_state.turn，
           本端方负）：BT RESIGN 帧语义 = 对端认输 = 本端胜，方向不同。 */
        st->winner = s->we_blue ? SIDE_BLUE : SIDE_RED;  /* 对端认输，本端胜 */
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
    /* ★ AUD-1000 B04-017（P1 误报撤回注释）：from/to 越界不可达——
       frame_coord_ok 校验 0<=col<5/0<=row<12，cell_index∈[0,60) 恒成立；
       from==to 自指由 jq_try_move 返负拦截（bt_semtest §1 rc=-1 已证）。 */
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
    /* ★ AUD-1000 B04-021（P1 已修）：cap_per_frame 下限守护（旧实现越界依赖
       sync_encode 内部 len+1<=cap 契约，cap 过小时 B04-001 越界放大；
       帧最小长 "v2\n"+7 字段+换行 ≥ 40 字节（含 NUL 裕量））。 */
    if (cap_per_frame < 40) return -1;
    int n = jq_bt_frame_sync_encode(buf, cap_per_frame, st, 0, N_CELLS);
    s->state = BT_ST_THEIR_TURN;
    return n;
}
