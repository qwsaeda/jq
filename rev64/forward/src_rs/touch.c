/*
 * touch.c — 触摸 → 棋盘格命中（⑤ 逆源码：原版 OnTouch 0xbdd2c）
 * 依据原版（capstone 确证）：
 *   0xbdd2c：JNI 调 MotionEvent getX(→r0)/getY(→r2)（vtable 0x84/0x7c 槽），
 *     bl 0x7bf80（坐标归一化/命中测试）→ sb=命中结果；
 *     ldr r3,[r4,#0x5c] blx r3（选中态 vtable）；
 *     switch（0xbddcc）：sb == [r3+4]→type 1（选中）/ == [r3+8]→type 2（取消）/
 *       == [r3+0xc]→type 7（走子）；bl 0xbd778（场景事件入队，0x0c 字节）。
 * 棋盘格映射（② 渲染几何对齐）：
 *   原版棋盘 12×5，蓝 row0-5 / 红 row6-11（L79）。
 *   触摸坐标 (px,py) → 格：col = (px - UV_X0*W)/cellW，row = (1-py/H)*ROWS 取整。
 *   行营/大本营在铁路外但仍是格（命中合法）。
 * 事件：0x0c 字节（type,x,y），接 ③b 场景帧驱动（jq_scene 事件队列，原版 0xbd580 入队）。
 * 诚实边界：MotionEvent JNI 调用为桩（宿主 Java 给 x/y），命中几何按 ② 的
 *   棋盘 UV（0.366-0.631 × 全高）重建；原版 0x7bf80 命中算法（是否含行营/越界
 *   特判）未逐指令逆（标注"行为级"），事件类型码 1/2/7 为原版 switch 实测值。
 */
#include "touch.h"
#include "junqi.h"
#include <math.h>
#include <stdlib.h>

/* ★ 层 2：棋盘格命中几何 = datax_177 BG_GRID（junqi.h），TCH_UV 0.366-0.631 已退役 */
/* 事件类型（原版 0xbddcc switch 实测值） */
#define JQ_EV_SELECT   1   /* 选中一格 */
#define JQ_EV_UNSELECT 2   /* 取消选中 */
#define JQ_EV_MOVE     7   /* 走子 */

int jq_touch_hit_cell(float px, float py, int view_w, int view_h,
                      int *out_row, int *out_col)
{
    /* ★ 层 2 视觉对齐：datax_177 棋盘格命中（与 render.c 棋子 BG_GRID 同几何）。
       视图坐标（顶左原点 y 下）→ 图像像素 → 最近格中心（半格距离判界）→
       设备行（row0=底，image_row=11-设备row）。
       ★ 2026-11-11 现代全面屏等比（与渲染棋盘 bg_s=vw/720 + bg_by0 同几何）：
       旧 img_x=px/view_w*BG_IMG_W 独立缩放在非 16:9 屏（20:9）拉伸错位。
       16:9 s=1.25,by0=0 → 与原版一致（无回归）。 */
    float bs = (float)view_w / 720.0f;   /* 棋盘等比（按宽，保比例） */
    float bby0 = ((float)view_h - 1280.0f * bs) * 0.5f;
    float img_x = px / bs;
    float img_y = (py - bby0) / bs;
    int col = (int)lroundf((img_x - BG_GRID_COL0_CX) / BG_GRID_COL_W);
    if (col < 0 || col >= BOARD_COLS) return -1;
    float cx = BG_GRID_COL0_CX + col * BG_GRID_COL_W;
    if (fabsf(img_x - cx) > BG_GRID_COL_W * 0.5f) return -1;
    /* 最近图像行（row5/6 铁路中线间隙：中点 699.5=(577+822)/2，间隙内命中空白） */
    float mid = (bg_row_cy(5) + bg_row_cy(6)) * 0.5f;
    int image_row;
    if (img_y < mid)
        image_row = (int)lroundf((img_y - BG_GRID_ROW0_CY) / BG_GRID_ROW_H);
    else
        image_row = 6 + (int)lroundf((img_y - BG_GRID_ROW6_CY) / BG_GRID_ROW_H);
    if (image_row < 0 || image_row >= BOARD_ROWS) return -1;
    float cy = bg_row_cy(image_row);
    if (fabsf(img_y - cy) > BG_GRID_ROW_H * 0.5f) return -1;
    int row = BOARD_ROWS - 1 - image_row;   /* 设备行（row0=底，蓝/人） */
    if (out_row) *out_row = row;
    if (out_col) *out_col = col;
    return cell_index(row, col);
}

/* 触摸处理（0xbdd2c 语义）：
   px,py（屏幕坐标）→ 命中格 → 命中结果码（sb）→ 事件类型（1/2/7）。
   sel：当前选中格（-1=无）。
   返回：事件类型码；调用方经 jq_event_push 入场景队列（0xbd580 语义）。 */
int jq_touch_process(float px, float py, int view_w, int view_h,
                    int sel, int *out_event, int *out_cell)
{
    int r = -1, c = -1;
    int idx = jq_touch_hit_cell(px, py, view_w, view_h, &r, &c);
    if (out_cell) *out_cell = idx;
    if (out_event) *out_event = 0;
    if (idx < 0) {
        /* 点空白 → 取消选中（type 2） */
        if (out_event) *out_event = JQ_EV_UNSELECT;
        return JQ_EV_UNSELECT;
    }
    if (sel < 0 || sel != idx) {
        /* 点新格 → 选中（type 1） */
        if (out_event) *out_event = JQ_EV_SELECT;
        return JQ_EV_SELECT;
    }
    /* 点已选中的格 → 走子（type 7），目标同格（翻棋由 jq_can_flip 在 0xbd5a0 帧驱动消费） */
    if (out_event) *out_event = JQ_EV_MOVE;
    return JQ_EV_MOVE;
}
