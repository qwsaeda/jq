/*
 * bt_protocol.c — 蓝牙联机协议（逆源码重写，无 JNI 依赖）
 * 依据原版确证（REVERSE_NOTES §89/§92 + spak_analysis）：
 *   - RFCOMM SPP，UUID `ce1404fe-0863-4278-86c2-c9c560f0e215`（rodata 0x121334）
 *   - 服务名 `junqi`（0x120d18）
 *   - 协议版本 v2（0x26efc 构造 "v2\n"）
 *   - 单步帧：`v2\n%d,%d,%d,%d,%d,%d,%d\n`（sprintf 0x26f24，格式串 0x120e94），
 *     7 字段 = 对局状态（帧类型/坐标/事件 等，0x26f5e 引用）。
 *   - 全盘同步：循环 60 格（`cmp r6,#60`，0x26f5e-0x26f84）逐格发帧。
 * 诚实边界：7 字段逐字段语义（offset 81/82/83/84/808/1780/1784）需原版
 *   C++ 类布局逆向（大工作量，AGENTS 红线：不伪装已实现）。本文件实现
 *   帧编解码 + 60 格全盘同步帧构造（字段顺序对齐原版 sprintf 调用序，
 *   语义标注为"类型,from_col,from_row,to_col,to_row,rank,side"行为级约定），
 *   SPP socket IO 由宿主（Java BluetoothSocket）承担。
 */
#include "junqi.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define JQ_BT_UUID "ce1404fe-0863-4278-86c2-c9c560f0e215"
#define JQ_BT_SERVICE "junqi"
#define JQ_BT_VERSION "v2"

typedef enum {
    BT_T_MOVE = 0,
    BT_T_FLIP,
    BT_T_UNDO,
    BT_T_SYNC,
    BT_T_SYNC_CELL,
    BT_T_RESTART,
    BT_T_RESIGN,
} jq_bt_type;

/* 构造单步帧 "v2\n%d,%d,%d,%d,%d,%d,%d\n"（原版 0x26f24）。
   字段：type, fc, fr, tc, tr, rank, side。返回写入字节数（不含 NUL），<0 失败。 */
int jq_bt_frame_encode(char *buf, int cap, int type, int f1, int f2,
                      int f3, int f4, int f5, int f6, int f7)
{
    if (!buf || cap < 8) return -1;
    /* ★ 对抗审计 #3：原版帧 = v2\ntype,fc,fr,tc,tr,rank,side\n（7 字段，type 首字段）。
       旧版 snprintf 只用 f1..f7，丢 type → 帧类型丢失，decode 首字段当 fc 错位。
       修正 = type 作为首字段，f1..f6 为数据（f7 兼容旧调用方，忽略）。 */
    int n = snprintf(buf, cap, JQ_BT_VERSION "\n%d,%d,%d,%d,%d,%d,%d\n",
                     type, f1, f2, f3, f4, f5, f6);
    if (n < 0 || n >= cap) return -1;
    return n;
}

/* 解析单步帧：跳过 "v2\n" 前缀，解析 7 字段。返回 1 成功，0 失败/非 v2 帧。 */
int jq_bt_frame_decode(const char *buf, int *f1, int *f2, int *f3,
                       int *f4, int *f5, int *f6, int *f7)
{
    if (!buf) return 0;
    const char *p = strstr(buf, JQ_BT_VERSION "\n");
    if (!p) return 0;
    p += strlen(JQ_BT_VERSION) + 1;
    return sscanf(p, "%d,%d,%d,%d,%d,%d,%d",
                  f1, f2, f3, f4, f5, f6, f7) == 7;
}

/* 全盘同步：60 格逐格构造帧（原版 0x26f5e-0x26f84 `cmp r6,#60` 循环）。
   每格一帧：BT_T_SYNC_CELL, idx, rank, side, revealed。返回帧数。 */
int jq_bt_frame_sync_encode(char *buf, int cap_per_frame,
                           const jq_state *st, int start, int count)
{
    int n = 0;
    for (int i = start; i < start + count && i < N_CELLS; i++) {
        const jq_cell *c = &st->cells[i];
        char fb[64];
        int len = jq_bt_frame_encode(fb, sizeof fb, BT_T_SYNC_CELL,
                                     i, c->rank, c->side, c->revealed, 0, 0, 0);
        if (len > 0 && cap_per_frame >= len)
            memcpy(buf + n * cap_per_frame, fb, len + 1);
        n++;
    }
    return n;
}

/* 构造走子帧（type=BT_T_MOVE，坐标对 + 事件 rank/side） */
int jq_bt_move_frame(char *buf, int cap, const jq_state *st, int from, int to,
                     jq_event ev)
{
    int fc = cell_col(from), fr = cell_row(from);
    int tc = cell_col(to), tr = cell_row(to);
    return jq_bt_frame_encode(buf, cap, BT_T_MOVE,
                              fc, fr, tc, tr,
                              st->cells[to].rank, st->cells[to].side, ev);
}
