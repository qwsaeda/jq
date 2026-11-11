/*
 * bt_spak.h — R6 蓝牙联机协议层公共定义（手写路线）
 *
 * 协议（对齐原版 0x26ec8 帧构造函数，REVERSE_NOTES §89/§92）：
 *   - 版本串 "v2\n"（0x26efc 构造），帧格式 "v2\n%d,%d,%d,%d,%d,%d,%d\n"（0x26f24 sprintf）
 *   - 7 字段 = 对局状态字段（原版结构体 offset 81/82/83/84/808/1780/1784，语义见 §94 任务B）
 *   - 60 格全盘同步（0x26f5e-0x26f84 循环），arm64 C 行格式（诚实边界重建）
 *
 * 语义帧类型（f1 字段）为 arm64 重建（原版 7 字段逐字段语义由并行子代理逆向），
 * 帧编解码与 jq_state 应用解耦：本头只定义线上帧 + 回调伪类型，应用在 junqi_jni.c。
 */
#ifndef BT_SPAK_H
#define BT_SPAK_H

#include "junqi_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 语义帧类型（f1 字段；线上帧 0-4,6,7，5 仅为回调伪类型）----
   原版帧字段含帧类型码（rodata 0x120ebc "code"）；arm64 用 f1 作类型码重建。 */
#define JQ_BT_T_MOVE       0   /* 走子（from,to 同步） */
#define JQ_BT_T_FLIP       1   /* 翻棋（idx 同步） */
#define JQ_BT_T_UNDO       2   /* 悔棋 */
#define JQ_BT_T_SYNC_REQ   3   /* 请求全盘同步（连接建立后发起方发） */
#define JQ_BT_T_SYNC       4   /* 全盘同步响应（c=stage,d=turn 后跟 60 格 C 行） */
#define JQ_BT_T_SYNC_CELL  5   /* C 行伪类型（reader 逐格回调，非线上帧） */
#define JQ_BT_T_RESTART    6   /* 重开 */
#define JQ_BT_T_RESIGN     7   /* 认输（a=胜方） */

/* ---- v2 帧编解码 ---- */
int jq_bt_frame_encode(char *buf, int cap, int f1, int f2, int f3, int f4,
                       int f5, int f6, int f7);
int jq_bt_frame_decode(const char *buf, int *f1, int *f2, int *f3, int *f4,
                       int *f5, int *f6, int *f7);

/* ---- 全盘同步 C 行： "C<idx>,<rank>,<side>,<revealed>\n"（arm64 重建）----
   decode 兼容 3 字段（无 side，由行推导）与 4 字段。 */
int jq_bt_frame_sync_encode(char *buf, int cap, const jq_state *st,
                            int start, int count);
int jq_bt_cell_decode(const char *line, int *idx, int *rank, int *side,
                      int *revealed);

#ifdef __cplusplus
}
#endif
#endif /* BT_SPAK_H */
