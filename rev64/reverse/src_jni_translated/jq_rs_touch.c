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
#include "jq_rs_touch.h"
#include "jq_rs_rules.h"
#include <math.h>
#include <android/log.h>
#include <stdlib.h>

/* ★ 层 2：棋盘格命中几何 = datax_177 BG_GRID（junqi.h），TCH_UV 0.366-0.631 已退役 */
/* 事件类型（原版 0xbddcc switch 实测值） */
#define JQ_EV_SELECT   1   /* 选中一格 */
#define JQ_EV_UNSELECT 2   /* 取消选中 */
#define JQ_EV_MOVE     7   /* 走子 */

int jq_touch_hit_cell(float px, float py, int view_w, int view_h,
                      int *out_row, int *out_col)
{
    /* ★ §124y-176 命中几何 0/60 根治（2026-11-11，用户"继续"指令）：
       旧 float 路径 img_x = px/(float)view_w*BG_IMG_W 在 32 位 so 经 MuMu
       houdini（x86_64 宿主转译 ARM32）的 VFP vdiv.f32/vmul.f32 行为异常 → 设备端
       60 格 probing 全 img_x=0/0.00 → 命中 0/60（HITDBG 实锤，桌面 x86 60/60）。
       修 = 整数定点（×1000 缩放，BG 常量 78.5→78500 / 140.5→140500 /
       222→222000 / 71→71000 / 822→822000 / 720→720000 / 1280→1280000，全 ×1000 定点），
       绕开 32 位浮点乘法/除法（long 整数运算在转译层稳定）。
       ★ 2026-11-11 二次实锤：float→double (px*1000.0) 在设备端同样失效（0.0），
       改 long double 乘法：(long double)px * 1000.0L（80-bit extended 不依赖 VFP）。
       判据：JQ-WBT-PROBE 逐格扫描 60/60（设备端）。 */
    long ix1k = (long)((long double)px * 1000.0L);   /* px 已 ≤view_w，×1000 不溢出 */
    long iy1k = (long)((long double)py * 1000.0L);
    long vw = view_w ? view_w : 1, vh = view_h ? view_h : 1;
    /* img_x ×1000 = px*1000 * 720000 / (view_w*1000) /1000 → 定点：
       img_x_1k = ix1k * 720 / vw （因 BG_IMG_W=720，×1000 约去一个 1000）*/
    long img_x1k = ix1k * 720L / vw;             /* = img_x * 1000（px/view*720 *1000）*/
    long img_y1k = iy1k * 1280L / vh;            /* = img_y * 1000 */
    /* BG_GRID_COL0_CX=78.5 → 78500 (×1000)；COL_W=140.5 → 140500 */
    long col0cx_1k = 78500L, colw_1k = 140500L;
    long row0cy_1k = 222000L, rowh_1k = 71000L, row6cy_1k = 822000L;
    /* 整数版"最近列"：col = round((img_x - col0cx)/colw) = (img_x1k-col0cx_1k + colw_1k/2) / colw_1k */
    int col = (int)((img_x1k - col0cx_1k + colw_1k / 2) / colw_1k);
    if (col < 0 || col >= BOARD_COLS) { __android_log_print(4,"junqi","HITDBG: col越界 img_x1k=%ld col=%d → -1", img_x1k, col); return -1; }
    long cx1k = col0cx_1k + (long)col * colw_1k;
    long diff = img_x1k - cx1k; if (diff < 0) diff = -diff;
    long col_gap1k = diff;
    if (col_gap1k > colw_1k / 2) { __android_log_print(4,"junqi","HITDBG: col_gap越界 img_x1k=%ld cx1k=%ld gap1k=%ld → -1", img_x1k, cx1k, col_gap1k); return -1; }
    /* row5/6 铁路中线 699.5 → 699500 (×1000) */
    long mid1k = 699500L;
    int image_row;
    if (img_y1k < mid1k)
        image_row = (int)((img_y1k - row0cy_1k + rowh_1k / 2) / rowh_1k);
    else
        image_row = 6 + (int)((img_y1k - row6cy_1k + rowh_1k / 2) / rowh_1k);
    if (image_row < 0 || image_row >= BOARD_ROWS) { __android_log_print(4,"junqi","HITDBG: image_row越界 img_y1k=%ld ir=%d → -1", img_y1k, image_row); return -1; }
    long cy1k = (image_row < 6) ? row0cy_1k + (long)image_row * rowh_1k
                                 : row6cy_1k + (long)(image_row - 6) * rowh_1k;
    long rdiff = img_y1k - cy1k; if (rdiff < 0) rdiff = -rdiff;
    if (rdiff > rowh_1k / 2) { __android_log_print(4,"junqi","HITDBG: row_gap越界 img_y1k=%ld cy1k=%ld gap1k=%ld → -1", img_y1k, cy1k, rdiff); return -1; }
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
