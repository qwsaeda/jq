/*
 * bt_session.c — 蓝牙联机帧状态机
 * 对齐原版 0xbe9cc 逻辑（收帧→校验→规则引擎→回合翻转→出帧）。
 * 7 字段行为级约定：f1=type, f2=fc, f3=fr, f4=tc, f5=tr, f6=rank, f7=side。
 * type 取值（jq_bt_type，权威定义在 bt_protocol.h）：MOVE=0/FLIP=1/UNDO=2/SYNC=3/
 * SYNC_CELL=4/RESTART=5/RESIGN=6/DRAW=7/CHAT=8。★ AUD-1000-ALL A4-01（P1 已修）：
 * 旧注释仅列 7 类（缺 DRAW/CHAT），与 .h 权威枚举（9 类）双源漂移；switch 未处理
 * DRAW/CHAT（default 拒收，bt_semtest B04-040 验 type=8/9 拒收），注释补全对齐 .h。
 */
#include "bt_session.h"
#include "bt_protocol.h"
#include "ui_state.h"   /* ★ P1（datah_039/107）：ui_replay_record（蓝牙走子/翻棋记录复盘序列） */
/* ★ Round 1 对抗审计 C-13（重开收端重载默认阵）：jq_default_lay.h 是**定义**头
   （const 数组在 C 里具外部链接 → 多 TU 包含会 multiple definition），故此处只
   extern 声明，不 include 头（layout.c 侧是唯一定义处）。 */
extern const uint8_t JQ_DEFAULT_LAY[30];
#include <string.h>
/* ★ 对抗审计 P3（批3 P3-B19）：原 static jq_state g_undo_snap 死代码
   （无写入/读取点，jq_state 体量不小白占 BSS）。删除。
   UNDO 悔棋栈实际在 jni_bridge.c g_undo[]（jq_bt_undo_available 读 g_undo_top）。 */

/* ★ AUD-1000-ALL A1-D4-001（P3 已修）：cell_of 死代码删除（全仓 0 调用点，frame_coord_ok/cell_index 为实际使用路径） */

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
        f1 != BT_T_RESTART && f1 != BT_T_RESIGN &&
        f1 != BT_T_DRAW && f1 != BT_T_CHAT)   /* ★ R6-C-03/09：DRAW/CHAT 合法消费（会话态可提和/聊天，数据帧仍拒） */
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
        /* ★ R9-C（P0 UNDO 请求-应答语义，v2 帧内扩展，R7-C-01/02/03/04 定案落地）：
           帧字段（f1=type，f2..=数据域）：f2=seq（>=1 新语义；0=legacy v2 旧帧）、
           f3=ref（REQ=0 / 应答=回显 req seq）、f4=hash_lo（REQ=发起前摘要可选 /
           ACK=回退后摘要 / NACK=reason 1非法/3回合/4栈空/5会话）、f5=1=NACK 子型
           标记（ACK f5=0）。
           三型分流：
           · legacy（f2==0 && f3==0）：旧 v2 帧——仅闸口+切回合，不 pop/不回帧
             （收帧侧 jni_bridge 走旧 undo_pop+SYNC 全量广播路径，0 退化兼容）；
           · REQ（f3==0 && f2>0）：请求方（对端）发起悔棋——本端闸口校验：
             回合不符 rc=-2（NACK reason=3）；栈空 rc=-3（reason=4）；
             幂等重放（f2==last_req_seq 且已有缓存应答）rc=JQ_BT_RC_UNDO_REQ_DUP
             （bridge 重发缓存 ACK/NACK，不二次 pop）；接受 rc=JQ_BT_RC_UNDO_REQ_OK
             （bridge 执行对称回退 + 回 UNDO_ACK(ref, hash)）；
           · 应答（f3>0）：ACK（f5==0）/NACK（f5==1）——仅记录缓存（幂等重发用），
             rc=0；实际本地回退/解锁由 bridge 发端 pending 路径执行（职责分离：
             session 层=闸口/幂等/应答元数据，bridge 层=回退实操+回帧）。
           诚实边界：桌面 bt_semtest 桩 jq_bt_undo_available 控栈深（B04-031）；
           无 BT 硬件端到端 = 真机 PENDING。 */
        if (f2 == 0 && f3 == 0) {
            /* legacy v2 UNDO（R8 前发端 999/999 缺省帧同族）：旧语义 0 退化。
               ★ 幂等定案（R9-C-06）：legacy 帧 seq=0 亦登记 last_req_seq=0 哨兵
               ——二次收 legacy 帧 → REQ_DUP（bridge 重发缓存/不再二次 pop，
               防重传/回环二次消费；旧 99 条基线 0 退化——首次 legacy 路径 rc=0 不变）。 */
            if (s->state != BT_ST_THEIR_TURN) return -2;
            if (!jq_bt_undo_available()) return -3;
            if (s->last_req_seq == 0 && s->legacy_undo_done)
                return JQ_BT_RC_UNDO_REQ_DUP;
            s->legacy_undo_done = 1;
            s->state = BT_ST_OUR_TURN;
            return 0;
        }
        if (f3 != 0) {
            /* 应答帧（ACK/NACK）：缓存供幂等重发；本地动作由 bridge 发端 pending 路径做 */
            s->last_resp_seq = (unsigned int)f2;
            s->last_resp_kind = (f5 == 1) ? 2 : 1;
            s->last_resp_reason = (f5 == 1) ? (unsigned char)f4 : 0;
            s->last_resp_hash = (f5 == 1) ? 0 : (unsigned short)(f4 & 0xffff);
            if (s->undo_pending && s->pending_seq == (unsigned int)f2) {
                s->undo_pending = 0;
                s->pending_seq = 0;
                s->state = (s->last_resp_kind == 1) ? BT_ST_OUR_TURN : s->state;
            }
            return 0;
        }
        /* REQ（f2>0, f3==0）——幂等判重必须在闸口校验**之前**：
           首次 REQ 接受后 state 已切 OUR_TURN，重放帧若先走回合闸口必 -2
           （§5c U4 实锤 rc=-2）；重放语义 = 重发缓存应答（不再校验栈深，
           栈态由首次消费决定）。 */
        if (f2 == (int)s->last_req_seq && s->last_req_seq != 0)
            return JQ_BT_RC_UNDO_REQ_DUP;                  /* 幂等：重发缓存应答 */
        if (s->state != BT_ST_THEIR_TURN) return -2;      /* NACK reason=3（回合不符） */
        if (!jq_bt_undo_available()) return -3;            /* NACK reason=4（栈空） */
        s->last_req_seq = (unsigned int)f2;
        s->state = BT_ST_OUR_TURN;
        return JQ_BT_RC_UNDO_REQ_OK;                       /* bridge：对称回退 + 回 ACK */
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
        /* ★ Round 1 对抗审计 C-16（P1 协议级修复）：SYNC_CELL 原只写 `peer_state`
           **死影子**（全仓读取点仅 bt_semtest 单测，jni_bridge/rules/render 均不读）
           → "对端权威态覆盖本端"的收敛通道**完全失效**（双端发散无修复途径）。
           修：除保留 peer_state 镜像外，**同时回写权威态 st->cells**（SYNC_CELL 的
           协议语义就是"以对端权威态覆盖"，逐格应用；BT SPP 为可靠有序流，60 帧
           到齐即全盘一致）。诚实边界：**发送侧 jq_bt_session_send_sync 目前无运行
           时调用点**（仅 bt_semtest 使用）→ 本修复为"接收侧就绪"，发送侧接线见
           R4-C-16 后续（UNDO 广播 SYNC_CELL 链）。 */
        st->cells[idx].rank = (uint8_t)f3;
        st->cells[idx].side = (uint8_t)f4;
        st->cells[idx].revealed = f5 ? 1 : 0;
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
        /* ★ R8-C-16（定案落地）：DISCONNECTED 下 RESTART 放行语义——会话已终局
           （RESIGN/断连）后对端发起重开 = 重建对局（B04-012 闸口仅拒**数据帧**
           继续污染终局棋盘；重开本身是合法恢复路径，bt_semtest §7 夹具依赖：
           DISCONNECTED→RESTART→CONNECTED 分流恢复会话态）。
           诚实边界：重连后帧窗口去重（v3 seq）未落地前，旧局在途帧可能被
           新会话消费——R8-C-17 复位集（bt_restart_reset_all）落地后清帧窗口。 */
        /* ★ R8-C-14/15 根因修正：case 入口必须将 DISCONNECTED 迁到 CONNECTED
           （会话恢复路径——RESIGN/断连终局后对端重开 = 重建会话）。旧 B04-012
           全拒收语义 + 顶层 :49-53 放行 RESTART 但 case 内 state 恒 0 → 后续
           MOVE/FLIP/UNDO 闸口（要求 state==THEIR_TURN）恒拒收 = "放行后死会话"。
           修：case 首行 s->state = BT_ST_CONNECTED；:182 首手分流覆写为
           OUR/THEIR_TURN（v2 999 缺省角色分流 0 退化，R8-C-15 帧权威覆写）。 */
        if (s->state == BT_ST_DISCONNECTED) s->state = BT_ST_CONNECTED;
        s->last_type = f1;   /* ★ B04-014（P1 已修）：仅合法 case 命中才记 last_type */
        /* ★ R8-C-14/15（P1，R7-C-14/15 定案落地：RESTART 携带 mode+host_is_blue，
           方案 A 帧内扩字段，R4-C-25 "帧不携带 mode"诚实边界闭合）：
           RESTART 帧字段约定：f1=mode（0=明棋/1=暗棋/999=v2 兼容帧缺省）、
           f2=host_is_blue（1=主机蓝/0=主机红，首手归属权威；999=v2 兼容缺省）。
           收端语义：① f1∈{0,1} 时**帧 mode 权威**（覆盖本机 ui_opt 选项，杜绝
           "一明一暗"双端分叉，R7-C-14 定案：host 发起方选项为权威）；f1==999
           （v2 旧发端无字段）回退本机 ui_opt_get_mode（R5 阶段 2 行为，0 退化）。
           ② f2∈{0,1} 时首手按 f2（host 蓝先手）vs 本端角色（we_blue）定
           s->state（取代 we_blue 单角色推定，双端先后手一致，R7-C-15 host_is_blue 校验）；
           f2==999 回退 we_blue 角色分流（v2 兼容）。
           拒绝语义：f1∉{0,1,999} 或 f2∉{0,1,999} 拒收 rc=-1（非法 mode 不重开，
           对齐 R7-C-12 NACK_MODE_MISMATCH 桌面等价——v3 完整 NACK 帧落地后
           本拒收点升级为 NACK 回帧，双端均不改局）。 */
        /* ★ R8-C-14 字段语义定案（★ 探针实锤纠正：on_frame 形参 f1=type、f2..f7=数据
           字段 1..6——jq_bt_session_on_frame(s,st,f1,f2,f3,f4,f5,f6,f7)，f1 恒为帧类型，
           mode/host_is_blue 走**数据字段** f2/mode + f3/host_is_blue（与 v2 7 字段编码
           "type,fc,fr,tc,tr,rank,side" 首字段=type 约定一致，bt_protocol.c:43-46）：
           f2=mode（0=明棋/1=暗棋/999=v2 兼容缺省）、f3=host_is_blue（1/0；999=v2 缺省）。
           旧误设"字段在 f1/f2"已删（f1=5 type 恒拒收 = 探针实锤 rst_mode=5 REJECT）。 */
        int rst_mode = f2, rst_blue = f3;
        if (rst_mode != 999 && rst_mode != 0 && rst_mode != 1) return -1;
        if (rst_blue != 999 && rst_blue != 0 && rst_blue != 1) return -1;

        /* ★ R8-C-16 落地修正：旧 B04-012 case 内 DISCONNECTED 二次拒收（rc=-2）
           已删——DISCONNECTED 下 RESTART 放行 = 会话恢复路径（顶层闸口 :49-53 仅
           拒数据帧，重开本身合法；§7 夹具 DISCONNECTED→RESTART→CONNECTED 分流
           依赖此放行）。B04-012 历史注释（:154-158）保留为演变注，语义以 R8-C-16
           定案为准：闸口仅拒数据帧继续污染终局棋盘，RESTART = 合法恢复。
           诚实边界：重连后帧窗口去重（v3 seq）未落地前，旧局在途帧可能被新会话
           消费——R8-C-17 复位集（bt_restart_reset_all）落地后清帧窗口。 */
        /* ★ Round 1 对抗审计 C-13/C-15/C-25（P1 协议级修复）：旧实现收端 RESTART
           走 `jq_state_init(st)` —— 该函数只 memset + 设 turn/stage/winner/mode，
           **60 格 cells 全清零且不载入默认布局**（layout.c 实证）→ 收端变成"空棋盘
           + stage=PLAY" → 无子可动 → 永久软锁；且无条件 `s->state = BT_ST_OUR_TURN`
           → **主机重开后收端也变 OUR_TURN**，双端互发首手被回合闸口丢弃 → 开局卡死。
           修：① 按模式重载默认布局（R8：mode = 帧权威或本机 ui_opt 回退）；
           ② 重置 stage/turn/winner/计数；③ 回合闸口按**首手归属**分流
           （R8：host_is_blue 帧权威，999 回退 we_blue 角色），与 start_bt_session 首帧分流一致。 */
        int eff_mode = (rst_mode == 999) ? ui_opt_get_mode() : rst_mode;
        if (eff_mode == 1)
            jq_dark_setup(st, JQ_DEFAULT_LAY);
        else
            jq_load_layout(st, JQ_DEFAULT_LAY);
        st->stage = STG_PLAY;
        st->turn = SIDE_BLUE;
        st->winner = JQ_WIN_NONE;
        st->moves_since_eat = 0;
        st->total_moves = 0;
        for (int ri = 0; ri < 2; ri++) { st->mines_cleared[ri] = 0; st->pieces_left[ri] = 0; }
        /* R8-C-15：首手归属 = 帧 host_is_blue 权威（999 回退 we_blue 角色分流） */
        if (rst_blue == 999)
            s->state = s->we_blue ? BT_ST_OUR_TURN : BT_ST_THEIR_TURN;
        else
            s->state = (rst_blue == s->we_blue) ? BT_ST_OUR_TURN : BT_ST_THEIR_TURN;
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
    case BT_T_DRAW:
        /* ★ R6-C-03/C-09（P1 最小闭合）：提和帧协议已定义（bt_protocol.h
           BT_T_DRAW，datah_067 ☰ 8 项）但旧实现无收帧 case → 落 default
           拒收（rc=-1）= 帧协议与实现永久分叉（收帧必拒 + 无回执不可诊断）。
           修：合法消费（rc=0 记 last_type），**不改棋局**（本地提和判定未
           逆向 = 诚实边界，jni_bridge 侧 OnBluetooth 分发 UI/evt55 提示，
           走子回合语义不变）；f1 携带 0=请求/1=同意/2=拒绝（发端 on_event
           draw_req/draw_agree/draw_disagree 同源）。 */
        s->last_type = f1;
        return 0;
    case BT_T_CHAT:
        /* ★ R6-C-03/C-09：聊天帧透传消费（rc=0），不改棋局——聊天内容
           f1-f6 由 jni_bridge 侧路由 UI（诚实边界：聊天 UI 未移植，
           帧合法消费保证双端会话态不因此拒收发散）。 */
        s->last_type = f1;
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
