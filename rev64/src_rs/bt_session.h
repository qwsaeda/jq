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
} jq_bt_session;

/* 初始化会话（st=当前对局，we_blue=本方阵营） */
void  jq_bt_session_init(jq_bt_session *s, const jq_state *st, int we_blue);

/* 处理收到的 v2 帧（已 decode 好的 7 字段）。
   返回：0=已消费，-1=非法帧（type 越界/坐标非法），-2=状态机拒绝（回合不符）。 */
int   jq_bt_session_on_frame(jq_bt_session *s, jq_state *st,
                              int f1, int f2, int f3,
                              int f4, int f5, int f6, int f7);

/* 构造要发给对方的帧（本方走子/翻棋/悔棋/同步），写入 buf，返回帧长 */
int   jq_bt_session_send_move(jq_bt_session *s, jq_state *st,
                               int from, int to, int flip, char *buf, int cap);
int   jq_bt_session_send_sync(jq_bt_session *s, jq_state *st,
                               char *buf, int cap_per_frame);

#endif /* RS_BT_SESSION_H */
