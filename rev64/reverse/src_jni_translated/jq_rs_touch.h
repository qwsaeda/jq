#ifndef RS_TOUCH_H
#define RS_TOUCH_H
#include "jq_rs_rules.h"

#define JQ_EV_SELECT   1   /* 原版 0xbddcc switch：选中 */
#define JQ_EV_UNSELECT 2   /* 取消选中 */
#define JQ_EV_MOVE     7   /* 走子 */

/* 触摸坐标 → 棋盘格（★ 层 2：datax_177 棋盘格 BG_GRID，几何在 junqi.h）。
   返回格索引（-1 越界/空白）。 */
int jq_touch_hit_cell(float px, float py, int view_w, int view_h,
                      int *out_row, int *out_col);
/* 触摸处理（0xbdd2c 语义）：命中格 → 事件类型（1/2/7）。 */
int jq_touch_process(float px, float py, int view_w, int view_h,
                    int sel, int *out_event, int *out_cell);

#endif
