/*
 * bt_spak.c — R6 联机 / 蓝牙协议层（手写路线）
 *
 * 状态更新（R6）：**SPAK 解密 key 已提取**（32B，内嵌 spak_load.c），
 * 全部 4 组（data/datah/datam/datax，883 条目）已可完整解包。SPAK 就绪。
 *
 * 蓝牙联机协议已确证（REVERSE_NOTES §89/§92）：
 *   - UUID `ce1404fe-0863-4278-86c2-c9c560f0e215`（rodata 0x121334）
 *   - 服务名 `junqi`（rodata 0x120d18）
 *   - 协议版本 **v2**（0x26efc 构造 "v2\n"）+ 单步帧 `v2\n%d,%d,%d,%d,%d,%d,%d\n`
 *     （7字段对局状态，0x120e94）+ 60格全盘同步（0x26f5e-0x26f84）
 *   - OnBluetooth 主逻辑 0xbe9cc（getAction/getIntExtra/getAddress）
 *
 * 本文件实现 native 协议层（帧编解码），供蓝牙联机（bt_net.c Java
 * BluetoothSocket IO + junqi_jni.c 帧状态机）使用。帧格式对齐原版 0x26f24
 * （sprintf "v2\n%d,...\n"）。
 */
#include "junqi_core.h"
#include "bt_spak.h"
#include <android/log.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define LOG_TAG "junqi64"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

/* 协议版本（原版 0x26efc 构造 "v2\n"） */
#define JQ_BT_VER "v2"

/* ---- 帧编码：构造 "v2\n%d,%d,%d,%d,%d,%d,%d\n"（原版 0x26f24 sprintf）----
   7 字段 = 对局状态字段（rank/side/action + 对局计数，偏移 81/82/83/84/808/1780/1784）。
   返回写入字节数（不含 NUL），<0 失败。 */
int jq_bt_frame_encode(char *buf, int cap, int f1, int f2, int f3, int f4,
                       int f5, int f6, int f7) {
    if (!buf || cap < 8) return -1;
    int n = snprintf(buf, cap, JQ_BT_VER "\n%d,%d,%d,%d,%d,%d,%d\n",
                     f1, f2, f3, f4, f5, f6, f7);
    if (n < 0 || n >= cap) return -1;
    return n;
}

/* ---- 帧解码：解析 "v2\n%d,%d,%d,%d,%d,%d,%d\n"（跳过版本前缀）----
   返回 1 成功（7 字段解析完整），0 失败/非 v2 帧。 */
int jq_bt_frame_decode(const char *buf, int *f1, int *f2, int *f3, int *f4,
                       int *f5, int *f6, int *f7) {
    if (!buf) return 0;
    /* 跳过版本前缀 "v2\n" */
    const char *p = strstr(buf, JQ_BT_VER "\n");
    if (!p) return 0;
    p += strlen(JQ_BT_VER) + 1;
    if (sscanf(p, "%d,%d,%d,%d,%d,%d,%d", f1, f2, f3, f4, f5, f6, f7) != 7) return 0;
    return 1;
}

/* ---- 全盘同步帧编码（原版 0x26f5e-0x26f84：60 格循环，每格一帧）----
   arm64 简化版：60 格逐个编码（每格 rank,side,revealed 三字段），调用方负责发送。
   C 行格式（arm64 重建，诚实边界）："C<idx>,<rank>,<side>,<revealed>\n"。
   返回写入字节数（不含 NUL），<0 失败。 */
int jq_bt_frame_sync_encode(char *buf, int cap, const jq_state *st, int start, int count) {
    if (!buf || cap < 4 || !st) return -1;
    int n = 0;
    for (int i = start; i < start + count && i < JQ_CELLS && n < cap - 8; i++) {
        int w = snprintf(buf + n, cap - n, "C%d,%d,%d,%d\n",
                         i, st->board[i].rank, st->board[i].side,
                         st->board[i].revealed);
        if (w < 0 || w >= cap - n) return -1;
        n += w;
    }
    return n;
}

/* ---- C 行解码： "C<idx>,<rank>,<side>,<revealed>" ----
   兼容 4 字段（含 side）与 3 字段（无 side，由行推导）。
   返回 1 成功，0 失败/非 C 行。 */
int jq_bt_cell_decode(const char *line, int *idx, int *rank, int *side,
                      int *revealed) {
    if (!line || line[0] != 'C') return 0;
    int i, r, s, v;
    if (sscanf(line + 1, "%d,%d,%d,%d", &i, &r, &s, &v) == 4) {
        *idx = i; *rank = r; *side = s; *revealed = v;
        return 1;
    }
    if (sscanf(line + 1, "%d,%d,%d", &i, &r, &v) == 3) {
        int row = i / JQ_BOARD_COLS;
        *idx = i; *rank = r;
        *side = (row <= 4) ? JQ_SIDE_BLUE : (row >= 7 ? JQ_SIDE_RED : JQ_SIDE_UNKNOWN);
        *revealed = v;
        return 1;
    }
    return 0;
}

/* 占位：在 JNI 层被调用，记录"未实现"（完整联机需 Java BluetoothSocket IO） */
void junqi_bt_not_implemented(void) {
    LOGI("R6 蓝牙联机占位：协议层就绪（v2 帧编解码），Java Socket IO 待接");
}
