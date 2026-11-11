#ifndef RS_TOUCH_STATE_H
#define RS_TOUCH_STATE_H
#include "junqi.h"
#include "touch.h"

typedef enum {
    JQ_TS_IDLE = 0,    /* 无选中 */
    JQ_TS_SELECTED     /* 已选中格 A，等目标 B（或点 A 翻棋） */
} jq_touch_mode;

typedef struct {
    jq_touch_mode mode;
    int sel;           /* 选中格 A（-1=无） */
    int target;        /* 待走目标（帧驱动消费） */
} jq_touch_state;

int  jq_touch_state_init(jq_touch_state *ts);
void jq_touch_state_reset(jq_touch_state *ts);
/* 处理触摸（屏幕坐标）。返回事件码 1/2/7/0；out_from/out_to 填走子端点。 */
int  jq_touch_state_process(jq_touch_state *ts, float px, float py,
                            int view_w, int view_h, int *out_from, int *out_to);
int  jq_touch_state_selected(const jq_touch_state *ts);
/* 选中格 A 的合法走子/翻棋目标（渲染高亮）。返回格数。 */
int  jq_touch_state_targets(const jq_state *st, const jq_touch_state *ts,
                            int *out_cells, int cap);

#endif
