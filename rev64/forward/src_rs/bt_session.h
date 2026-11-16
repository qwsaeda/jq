/*
 * bt_session.h — 蓝牙联机帧状态机（逆向重写，无 JNI）
 * 依据原版 0xbe9cc（OnBluetooth 主逻辑）+ 0x26f24（帧编码）+ §89/§92 确证。
 * 职责：维护会话状态（本方/对方/连接/回合），收帧→规则引擎→出帧。
 * 诚实边界：SPP socket IO 由 Java 宿主承担；7 字段语义为行为级约定
 *   （type, fc, fr, tc, tr, rank, side），非原版 C++ 类布局逐字段逆向。
 */
#ifndef RS_BT_SESSION_H
#define RS_BT_SESSION_H
#include "junqi.h"

#define BT_BUF 256

typedef enum {
    BT_ST_DISCONNECTED = 0,
    BT_ST_CONNECTED,       /* SPP 已连接，等对方数据 */
    BT_ST_OUR_TURN,        /* 轮到我方（等输入，AI 托管） */
    BT_ST_THEIR_TURN,      /* 轮到对方（等对方帧） */
} jq_bt_st;

typedef struct {
    jq_bt_st state;
    int      we_blue;       /* 1=我是蓝方（先手），0=我是红方 */
    int      cell_from[60]; /* 每格最后同步的 from 索引（对账用） */
    int      last_type;     /* 最后一帧的 type */
    jq_state peer_state;    /* 对方视角的状态镜像（SYNC 帧维护） */
    /* ★ R9-C（P0 UNDO 请求-应答事务态，v2 帧内扩展：0=legacy/idle 0 退化） */
    unsigned int tx_seq;          /* 发端单调 seq（从 1 起；0 保留给 legacy v2 UNDO 帧） */
    unsigned int last_req_seq;    /* 收端最近消费的 REQ seq（幂等去重键） */
    unsigned int pending_seq;     /* 发端等待 ACK/NACK 的 seq（0=空闲） */
    int      undo_pending;        /* 1=已发 UNDO_REQ 等待应答（走子闸口冻结） */
    unsigned int last_resp_seq;   /* 幂等重发缓存：上次应答的 REQ seq */
    unsigned char last_resp_kind;/* 0 none / 1 ACK / 2 NACK */
    unsigned char last_resp_reason; /* NACK reason（1 非法/3 回合/4 栈空/5 会话） */
    unsigned short last_resp_hash;  /* ACK hash_lo（回退后局面摘要） */
    unsigned char legacy_undo_done; /* legacy 帧已消费哨兵（幂等防二次 pop） */
    /* ★ R11-C（P1 全局帧窗口去重，R7-C-03/09 全局化）：v3 seq 独立槽归一化后统一判重。
       rx_last_seq：收端最近**接受**帧 seq（0=窗口未建/legacy/v2-only，判重跳过）；
       tx_last_seq：发端最近发送 seq 镜像（诊断）。两者均在 RESTART 成功后复位 0。
       单调窗口假设（非回绕，2^32 帧/会话不可达；RFC1982 serial compare 预留）。 */
    unsigned int rx_last_seq;     /* 全 type 最近接受帧 seq（0=窗口未建/legacy） */
    unsigned int tx_last_seq;     /* 发端最近发送 seq（诊断镜像） */
} jq_bt_session;

/* UNDO 请求-应答 rc 哨兵（jq_bt_session_on_frame 返回值扩展，R9-C）：
   0=消费（legacy 旧语义/ACK/NACK 帧）, 1=UNDO_REQ 接受（bridge 需回退+回 ACK），
   2=UNDO_REQ 幂等重放（重发缓存应答，不二次 pop）, -1/2/3=拒绝（NACK reason 映射）。 */
#define JQ_BT_RC_UNDO_REQ_OK   1
#define JQ_BT_RC_UNDO_REQ_DUP  2

/* 初始化会话（st=当前对局，we_blue=本方阵营） */
void  jq_bt_session_init(jq_bt_session *s, const jq_state *st, int we_blue);

/* 处理收到的 v2 帧（已 decode 好的 7 字段）。
   返回：0=已消费，-1=非法帧（type 越界/坐标非法），-2=状态机拒绝（回合不符）。 */
int   jq_bt_session_on_frame(jq_bt_session *s, jq_state *st,
                              int f1, int f2, int f3,
                              int f4, int f5, int f6, int f7);
/* ★ R11-C（P1 全局 seq 去重 + v3 接线，R10 回执 §2/§5 定案）：v3 归一化入口。
   decode_any 已产出 (type,seq,ref,f1..f6) 归一化视图；本封装：
   ① 前置全局判重（seq>0 且 rx_last_seq>0：重复/旧包丢弃，UNDO REQ/应答 豁免）
   ② 映射回旧 7 字段 f1..f7 调 jq_bt_session_on_frame（type→f1，f1..f6→f2..f7；
      UNDO 用归一化 seq/ref/f1/f2 填 f2..f5）
   ③ 成功消费后登记 s->rx_last_seq=seq（逐 case，见 bt_session.c）。
   返回：0=消费（含重复帧静默丢弃），-1=非法/旧包，-2/3=状态机，REQ_OK/DUP=UNDO。
   v2 兼容：decode_any 对 v2 非 UNDO 归一化 seq=0 → 前置判重跳过，0 退化。 */
int   jq_bt_session_on_frame_v3(jq_bt_session *s, jq_state *st,
                                 int type, unsigned int seq, unsigned int ref,
                                 int f1, int f2, int f3,
                                 int f4, int f5, int f6);

/* 构造要发给对方的帧（本方走子/翻棋/悔棋/同步），写入 buf，返回帧长 */
int   jq_bt_session_send_move(jq_bt_session *s, jq_state *st,
                               int from, int to, int flip, char *buf, int cap);
int   jq_bt_session_send_sync(jq_bt_session *s, jq_state *st,
                               char *buf, int cap_per_frame);

#endif /* RS_BT_SESSION_H */
