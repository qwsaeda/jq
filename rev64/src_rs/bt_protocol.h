/*
 * bt_protocol.h — 蓝牙联机协议接口（无 JNI）
 * 参见 bt_protocol.c 头部注释（UUID/服务名/帧格式 原版确证）。
 */
#ifndef RS_BT_PROTOCOL_H
#define RS_BT_PROTOCOL_H

#include "junqi.h"

#define JQ_BT_UUID "ce1404fe-0863-4278-86c2-c9c560f0e215"
#define JQ_BT_SERVICE "junqi"

typedef enum {
    BT_T_MOVE = 0,
    BT_T_FLIP,
    BT_T_UNDO,
    BT_T_SYNC,
    BT_T_SYNC_CELL,
    BT_T_RESTART,
    BT_T_RESIGN,
} jq_bt_type;

int jq_bt_frame_encode(char *buf, int cap, int type, int f1, int f2,
                      int f3, int f4, int f5, int f6, int f7);
int jq_bt_frame_decode(const char *buf, int *f1, int *f2, int *f3,
                       int *f4, int *f5, int *f6, int *f7);
int jq_bt_frame_sync_encode(char *buf, int cap_per_frame,
                           const jq_state *st, int start, int count);
int jq_bt_move_frame(char *buf, int cap, const jq_state *st, int from, int to,
                     jq_event ev);

#endif /* RS_BT_PROTOCOL_H */
