/* jq_rs_walkbridge_jni.c — 会话 B 规则引擎的 32 位 so 接入点（§124y-19 借鉴会话 B）
 *
 * 作用：把会话 B 的纯 C 走子规则引擎（jq_rs_rules.c 等）接到 32 位 so 的
 * 触摸下游，让 32 位版 OnTouch 坐标 → jq_touch_hit_cell 命中格 → jq_try_move 走子
 * → 打 Junqi.* 事件日志。这样 32 位翻译体的"游戏逻辑"可被会话 B 规则引擎驱动
 * （不依赖未填充的 .pak 资源对象图，纯逻辑走子可机判）。
 *
 * 触发：
 *   setprop jq.walk_bridge 1   → OnTimer 触发 jq_walk_bridge_tick()
 *   （注入固定坐标 353,927 = 会话 B cell25 基准 → 命中格 → 走子 → 打事件）
 *
 * 诚实边界：
 *   - 走子结果（Junqi.Move/Eat/Bomb/Flag/Dead）来自会话 B 规则引擎（纯逻辑）
 *   - 真机渲染的棋子位置/棋盘出屏 仍需 32 位翻译体的 GL 路径（资源对象图）
 *   - 这是"逻辑走子桥"，非"视觉走子"——验证规则引擎可驱动 32 位 so 的走子事件
 */
#include <stdint.h>
#include <stdio.h>
#include <android/log.h>
#include "jq_rs_rules.h"
#include "jq_rs_walkbridge.h"

/* 全局走子状态（会话 B jq_state，32 位 so 内嵌） */
static jq_state g_walk_state;
static int g_walk_init = 0;

/* 开局 + 加载会话 B 38 子布局（JQ_DEFAULT_LAY） */
int jq_walk_bridge_run(void) {
    if (!g_walk_init) {
        jq_state_init(&g_walk_state);
        jq_load_layout(&g_walk_state, JQ_DEFAULT_LAY);
        jq_refresh_movable_counts(&g_walk_state);
        g_walk_init = 1;
        __android_log_print(4, "junqi", "jq_walk_bridge: 开局完成（会话 B 38 子布局 JQ_DEFAULT_LAY）");
    }
    return 0;
}

/* 坐标→格子→走子 全链（会话 B touch.c 命中几何 + rules.c 走子规则） */
int jq_walk_bridge_touch(float px, float py, int view_w, int view_h) {
    if (!g_walk_init) jq_walk_bridge_run();
    int out_row, out_col, cell;
    cell = jq_touch_hit_cell(px, py, view_w, view_h, &out_row, &out_col);
    if (cell < 0) {
        __android_log_print(4, "junqi", "jq_walk_bridge_touch: tap(%.0f,%.0f) 未命中格", px, py);
        return -1;
    }
    /* 找该格的 rank（蓝方 = 原值） */
    const jq_cell *c = &g_walk_state.cells[cell];
    __android_log_print(4, "junqi", "jq_walk_bridge_touch: tap(%.0f,%.0f) → cell%d (row%d col%d) rank=%d side=%d",
                        px, py, cell, out_row, out_col, c->rank, c->side);
    return cell;
}

/* 走子 + 打事件（对照会话 B 桌面单测基准） */
int jq_walk_bridge_move(int from, int to) {
    if (!g_walk_init) jq_walk_bridge_run();
    int ev = jq_try_move(&g_walk_state, from, to);
    static const char *evname[] = {"Move","Eat","Bomb","Flag","Dead","Move"};
    if (ev >= 0 && ev < 6)
        __android_log_print(4, "junqi", "jq_walk_bridge_move: cell%d→%d 事件=Junqi.%s（会话 B 规则引擎驱动）",
                            from, to, evname[ev]);
    else
        __android_log_print(4, "junqi", "jq_walk_bridge_move: cell%d→%d 非法（ev=%d）", from, to, ev);
    return ev;
}

/* OnTimer 触发钩子：setprop jq.walk_bridge 1 → 开局 + 走 cell25→30 + 打事件 */
void jq_walk_bridge_tick(void) {
    static int done = 0;
    char val[8] = {0};
    extern int __system_property_get(const char *, char *);
    if (__system_property_get("jq.walk_bridge", val) > 0 && val[0] == '1' && !done) {
        done = 1;
        __android_log_print(6, "junqi", "WALK-BRIDGE: setprop jq.walk_bridge=1 → 开局 + 走子");
        jq_walk_bridge_run();
        /* 会话 B 基准走子序列（test_move_seq.txt）：f6 f33 m6→11 m33→38 */
        if (jq_can_flip(&g_walk_state, 6))  jq_flip(&g_walk_state, 6);
        if (jq_can_flip(&g_walk_state, 33)) jq_flip(&g_walk_state, 33);
        jq_walk_bridge_move(6, 11);
        jq_walk_bridge_move(33, 38);
        __android_log_print(6, "junqi", "WALK-BRIDGE: 走子序列完成（会话 B 规则引擎驱动 32 位 so 走子事件）");
    }
}
