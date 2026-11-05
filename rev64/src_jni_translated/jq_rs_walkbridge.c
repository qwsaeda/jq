/* jq_rs_walkbridge.c — 会话 B 规则引擎的 32 位 so 接入点（§124y-19 借鉴会话 B）
 * 把会话 B 纯 C 走子规则引擎（jq_rs_rules.c 等）接到 32 位 so 触摸下游：
 * OnTouch 坐标 → jq_touch_hit_cell 命中格 → jq_try_move 走子 → 打 Junqi.* 事件。
 * 触发：setprop jq.walk_bridge 1 → OnTimer 调 jq_walk_bridge_tick() */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <android/log.h>
#include <sys/system_properties.h>
#include "jq_rs_rules.h"
#include "jq_rs_walkbridge.h"
#define JQ_EXPORT __attribute__((visibility("default")))


static jq_state g_walk_state;
static int g_walk_init = 0;

JQ_EXPORT int jq_walk_bridge_run(void) {
    if (!g_walk_init) {
        jq_state_init(&g_walk_state);
        jq_load_layout(&g_walk_state, JQ_DEFAULT_LAY);
        jq_refresh_movable_counts(&g_walk_state);
        g_walk_init = 1;
        __android_log_print(4, "junqi", "jq_walk_bridge: 开局完成（会话 B 38 子布局 JQ_DEFAULT_LAY）");
    }
    return 0;
}

JQ_EXPORT int jq_walk_bridge_touch(float px, float py, int view_w, int view_h) {
    if (!g_walk_init) jq_walk_bridge_run();
    int out_row, out_col, cell;
    cell = jq_touch_hit_cell(px, py, view_w, view_h, &out_row, &out_col);
    if (cell < 0) {
        __android_log_print(4, "junqi", "jq_walk_bridge_touch: tap(%.0f,%.0f) 未命中格", px, py);
        return -1;
    }
    const jq_cell *c = &g_walk_state.cells[cell];
    __android_log_print(4, "junqi", "jq_walk_bridge_touch: tap(%.0f,%.0f) → cell%d rank=%d side=%d",
                        px, py, cell, c->rank, c->side);
    return cell;
}

JQ_EXPORT int jq_walk_bridge_move(int from, int to) {
    if (!g_walk_init) jq_walk_bridge_run();
    int ev = jq_try_move(&g_walk_state, from, to);
    static const char *evname[] = {"Move","Eat","Bomb","Flag","Dead","Move"};
    if (ev >= 0 && ev < 6)
        __android_log_print(4, "junqi", "jq_walk_bridge_move: cell%d→%d 事件=Junqi.%s（会话 B 引擎驱动）",
                            from, to, evname[ev]);
    else
        __android_log_print(4, "junqi", "jq_walk_bridge_move: cell%d→%d 非法（ev=%d）", from, to, ev);
    return ev;
}

JQ_EXPORT void jq_walk_bridge_tick(void) {
    static int done = 0;
    char val[8] = {0};
    if (__system_property_get("jq.walk_bridge", val) > 0 && val[0] == '1' && !done) {
        done = 1;
        __android_log_print(6, "junqi", "WALK-BRIDGE: setprop jq.walk_bridge=1 → 开局 + 走子");
        jq_walk_bridge_run();
        if (jq_can_flip(&g_walk_state, 6))  jq_flip(&g_walk_state, 6);
        if (jq_can_flip(&g_walk_state, 33)) jq_flip(&g_walk_state, 33);
        jq_walk_bridge_move(6, 11);
        jq_walk_bridge_move(33, 38);
        __android_log_print(6, "junqi", "WALK-BRIDGE: 走子序列完成（会话 B 引擎驱动 32 位 so 走子事件）");
    }
}
