/*
 * touch_state.c — 两步走子交互态机（⑤b 逆源码：原版 OnTouch → 事件消费链）
 * 依据原版（capstone 确证）：
 *   0xbdd2c（OnTouch）：MotionEvent 坐标 → 0x7bf80 命中 → 结果码 sb →
 *     switch（0xbddcc）type 1=选中 / 2=取消 / 7=走子 → bl 0xbd580（事件入队，0x0c 字节）
 *   0xbd580：事件入队（场景事件队列，0xbd5a0 帧驱动消费）
 *   0x7bf80：命中测试（vtable 0xc8 槽 → 0x7cd60/0x7cd54 几何判定）
 *   0xbdd2c 两步交互：
 *     点格 A（type 1 选中）→ 选中态 vtable[0x5c] 记录 A + 高亮可走格
 *     点格 B（type 7 走子）→ 事件(7,A→B) 入队 → 帧驱动 jq_can_move(A,B) 判定走子
 *     点空白（type 2 取消）→ 清选中
 * 本态机（行为等价，堆自建）：
 *   JQ_TOUCH_IDLE → 点格 A → JQ_TOUCH_SELECTED(A)（高亮 A 的可走目标）
 *     → 点格 B → 走子事件（jq_can_move/jq_can_flip 判定）→ JQ_TOUCH_IDLE
 *     → 点 A 自身 → 翻棋（jq_can_flip）→ JQ_TOUCH_IDLE
 *     → 点空白 → JQ_TOUCH_IDLE（取消）
 * 诚实边界：原版 0x7bf80 命中特判（行营/越界/触摸半径）未逐指令逆，
 *   命中几何用 ② 棋盘 UV（0.366-0.631 × 全高）重建；事件码 1/2/7 为原版实测值。
 */
#include "touch_state.h"
#include "touch.h"
#include "junqi.h"
#include <string.h>

int jq_touch_state_init(jq_touch_state *ts)
{
    memset(ts, 0, sizeof(*ts));
    ts->mode = JQ_TS_IDLE;
    ts->sel = -1;
    return 0;
}

/* 重置选中态（翻棋失败 / 取消选中 / 走子后）。 */
void jq_touch_state_reset(jq_touch_state *ts)
{
    ts->mode = JQ_TS_IDLE;
    ts->sel = -1;
    ts->target = -1;
}

/* 处理一次触摸（px,py 屏幕坐标 → 事件）。
   返回：本触摸产生的场景事件码（1 选中 / 2 取消 / 7 走子 / 0 无）；
   out_from/out_to 走子事件填 from=选中格、to=目标格。 */
int jq_touch_state_process(jq_touch_state *ts, float px, float py,
                          int view_w, int view_h, int *out_from, int *out_to)
{
    if (out_from) *out_from = -1;
    if (out_to) *out_to = -1;
    int cell = jq_touch_hit_cell(px, py, view_w, view_h, NULL, NULL);
    if (cell < 0) {
        /* 点空白 → 取消选中（type 2） */
        if (ts->mode != JQ_TS_IDLE) {
            ts->mode = JQ_TS_IDLE;
            ts->sel = -1;
            return JQ_EV_UNSELECT;
        }
        return 0;
    }
    if (ts->mode == JQ_TS_IDLE) {
        /* 选中格 A（type 1） */
        ts->mode = JQ_TS_SELECTED;
        ts->sel = cell;
        ts->target = cell;
        if (out_from) *out_from = cell;
        return JQ_EV_SELECT;
    }
    /* 已选中态：
       - 点同一格 = 翻棋意图（走子到自身，帧驱动 jq_can_flip 判定）。
         ★ 翻棋后保持选中（标准军棋：翻棋成功仍选中，下一步点目标格走子）。
         翻棋失败由 OnTouch 手动回 IDLE（jq_touch_state_reset）。
       - 点新格 = 走子意图（type 7，A→B，帧驱动 jq_can_move 判定） */
    int from = ts->sel, to = cell;
    if (to == from) {
        if (out_from) *out_from = from;
        if (out_to) *out_to = to;
        return JQ_EV_MOVE;   /* 保持 SELECTED(A)，OnTouch 消费翻棋 */
    }
    ts->mode = JQ_TS_IDLE;
    ts->sel = -1;
    ts->target = -1;
    if (out_from) *out_from = from;
    if (out_to) *out_to = to;
    return JQ_EV_MOVE;   /* 帧驱动消费：to!=from 走子 */
}


/* 当前选中格（渲染高亮用）。-1=无。 */
int jq_touch_state_selected(const jq_touch_state *ts)
{
    return ts->sel;
}

/* 可走目标格（渲染高亮）：选中格 A 的合法走子/翻棋目标。
   返回目标格数；out_cells[] 填格索引。走子判定经 jq_can_move（需 state）。 */
int jq_touch_state_targets(const jq_state *st, const jq_touch_state *ts,
                          int *out_cells, int cap)
{
    if (ts->mode != JQ_TS_SELECTED || ts->sel < 0) return 0;
    int n = 0;
    int A = ts->sel;
    for (int t = 0; t < N_CELLS && n < cap; t++) {
        if (t == A) continue;
        if (jq_can_move(st, A, t)) out_cells[n++] = t;
    }
    /* 翻棋目标 = 自身 */
    if (jq_can_flip(st, A) && n < cap) out_cells[n++] = A;
    return n;
}
