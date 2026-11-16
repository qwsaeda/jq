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
    BT_T_DRAW,   /* ★ 对抗审计（datah_067）：提和 */
    BT_T_CHAT,   /* ★ 对抗审计（datah_067）：聊天 */
    /* ★ R13：能力协商握手帧（显式 CAP，v3-only 双端互不先发 v3 死锁根治；连接确认后
       发 1 次；旧对端 default 拒收 -1 = v2-only 降级维持，双端同版才互通 v3/CAP） */
    BT_T_CAP,
} jq_bt_type;

/* ★ R13：CAP 能力位（v3 承载 f2=cap_ver / f3=cap_mask；bit0 = v3 帧 + 全局 seq 去重能力） */
#define JQ_BT_CAP_VER  1
#define JQ_BT_CAP_V3   0x01u

/* ★ R11-C（P0 v3 完整帧双栈 API，R10 回执 §1/§2 定案）：
   v3 帧形 "v3\n" + "<type>,<seq>,<ref>,<f1>..<f6>\n"（10 字段，seq/ref 独立槽，
   不再借 f2/f3 数据域；v2 = 7 字段 type+数据域，R9 UNDO 复用 f1..f4）。
   双栈互通：decode_any 自动识别 v3/v2，统一输出 v3 归一化视图
   （v2 → seq=ref=0；v2 UNDO R9 语义映射 seq=f1_v2, ref=f2_v2, f1=f3_v2, f2=f4_v2）。
   v2 既有 API 行为 0 改动（兼容存量调用 + bt_semtest 100 基线）。 */
#define JQ_BT_V3_VERSION "v3"

int jq_bt_frame_encode_v3(char *buf, int cap, int type, unsigned int seq,
                          unsigned int ref, int f1, int f2, int f3,
                          int f4, int f5, int f6);
int jq_bt_frame_decode_v3(const char *buf, int *type, unsigned int *seq,
                          unsigned int *ref, int *f1, int *f2, int *f3,
                          int *f4, int *f5, int *f6);
/* 双栈入口：v3 优先，失败回退 v2（v2 命中输出归一化 seq=ref=0，type 取 v2 csv0，
   f1..f6 取 v2 csv1..6；v2 UNDO 型做 R9 语义迁移，见 bt_protocol.c 注释）。
   返回 1 成功 / 0 失败（非 v2/v3 帧）。 */
int jq_bt_frame_decode_any(const char *buf, int *type, unsigned int *seq,
                           unsigned int *ref, int *f1, int *f2, int *f3,
                           int *f4, int *f5, int *f6);

int jq_bt_frame_encode(char *buf, int cap, int type, int f1, int f2,
                      int f3, int f4, int f5, int f6, int f7);
int jq_bt_frame_decode(const char *buf, int *f1, int *f2, int *f3,
                       int *f4, int *f5, int *f6, int *f7);
int jq_bt_frame_sync_encode(char *buf, int cap_per_frame,
                           const jq_state *st, int start, int count);
int jq_bt_move_frame(char *buf, int cap, const jq_state *st, int from, int to,
                     jq_event ev);
/* ★ R12-C：局面摘要 hash_lo（16 位 FNV-1a，双端同实现同输入→同值；覆盖 60 格
   rank/side/revealed + stage + turn 共 182 字节固定序喂入；不含 moves/winner
   计数缓存，同构局面=可继续对弈等价态）。UNDO ACK 携带，mismatch 才全量 SYNC。 */
unsigned short jq_state_hash_lo(const jq_state *st);

#endif /* RS_BT_PROTOCOL_H */
