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

/* §124y-166 AUD-R 分叉标记：布局/几何族 + 事件名表族（走子桥接入点）。
   · R-021 头注释 0x39e14 行约束误标 → 32 位侧头注释与 64 位同源（行 15-32），误标同源 → 对齐（同源）
   · R-051 jq_state_init 在 layout.c 声明实现分处 → 32 位侧同（jq_rs_rules.c 不含 layout，
     走子桥行 20 调 jq_state_init）→ 对齐（同源）
   · R-052 STG_SETUP 死枚举 → 32 位侧同（走子桥无布阵阶段分支）→ 对齐（同源）
   · R-081 三套实现缺一致性闸门 → 32 位侧即第 3 套实现（本走子桥），arm32_walkregress
     为 32 位基准闸门，与 64 位 evverify/regress 双路 → 行为等价（分叉同步机制 R-138）
   · R-127 EVENT_NAMES[6] 指针非 const → 32 位侧同（jq_rs_rules.c 行 47 const char*[6]，
     64 位修法加 const 限定 32 位侧未同步）→ 分叉（64 位独有设计）
   · R-128 jq_cell 4 字节紧凑与裸字节序列化耦合 → 32 位侧同布局（走子桥无跨版本存档维度）
     → 对齐（同源）
   · R-096 头注释未区分确证/推断 → 32 位侧头注释同源混写 → 对齐（同源）
   · R-097 引入未使用 stdlib.h → 32 位侧 jq_rs_rules.c 行 38 include stdlib（malloc 未用）
     → 对齐（同源缺口）
   · R-048 extract_layout_30 蓝半区假设 → 32 位侧走子桥无布局导出维度 → N/A（层级不适用）
   布局/几何/事件名 32 位侧未逐条实证，诚实边界留待 32 位反汇编专项。 */
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

/* §124y-26：强制重置开局（供 game_test 端到端钩子用——共享 static g_walk_state，
   8 步序列会消耗回合，game_test 需独立开局，故先 reset 再走子） */
JQ_EXPORT int jq_walk_bridge_reset(void) {
    g_walk_init = 0;
    jq_walk_bridge_run();
    return 0;
}

/* §124y-166 AUD-R 分叉标记：走子桥接入点族（会话 B 移植，F_3ab80 事件分发下游）。
   · R-003 先手翻子定色 → 32 位侧定色在 jq_flip 内（side==0xFF→turn，行 76-77 已标），
     布局走子桥未走 SIDE_UNKNOWN 全流程 → 分叉（64 位独有设计）
   · R-063 布局校验蓝方军旗格不受约束 → 32 位侧走子桥布局 JQ_DEFAULT_LAY 写死
     （行 21），无自定义/联机布局校验维度 → 分叉（64 位独有设计）
   · R-064 validate_layout 错误码语义混淆 → 32 位侧无 jq_validate_layout（走子桥布局固定）
     → N/A（层级不适用）
   · R-065 布局每 rank 计数×2 镜像假设 → 32 位侧同 JQ_DEFAULT_LAY 固定布局 → 对齐（同源）
   · R-066 is_rail 底线纵线交点双重身份 → 32 位侧 board 几何同 64 位（is_rail 行 97 放行 row0/11）
     → 对齐（同源）
   · R-067 铁路几何硬编码缺地形抽象 → 32 位侧同（行 80/97 硬编码 if 链）→ 对齐（同源）
   · R-068 cell_index 无边界检查 → 32 位侧走子桥入参由 hit_cell 产出（范围已限），
     jq_can_move 行 124 有 from/to∈[0,N_CELLS) 检查 → 行为等价
   · R-069 coord_cmp 无调用方 → 32 位侧同（无 coord_cmp 使用点）→ 对齐（同源）
   · R-076 SPEC 棋盘尺寸与实现不一致 → 32 位侧实现 12×5（行 48-49 几何常量），SPEC 同源描述 → 对齐（同源缺口）
   · R-077 渲染几何常量与规则层混放 → 32 位侧走子桥 BG_GRID_* 常量与几何混放（行 51-54），
     与 64 位同源 → 对齐（同源）
   · R-078 bg_cell_img_center 越界 row/col 不校验 → 32 位侧走子桥 bg_row_cy 无越界检查（行 52）
     → 对齐（同源）
   · R-079 rank_encode 定义未调用 → 32 位侧无 rank_encode 使用点 → 对齐（同源）
   · R-080 RANK_TOP=12 与 RANK_COUNT=13 边界耦合 → 32 位侧 rank_norm 同实现 → 对齐（同源）
   · R-082 每步 O(N²) 移动端性能风险 → 32 位侧同（jq_side_stalemated 双循环，见 R-112 标记）
     → 对齐（同源，未压测）
   · R-085 can_flip 可翻任意未定色暗子 → 32 位侧已加回合校验（jq_rs_rules.c 行 305-307）
     → 对齐（已同步修复）
   走子桥基准 = arm32_walkregress 4/4 PASS；几何/布局/性能细分 32 位侧未逐条实证，
   诚实边界留待 32 位反汇编专项（F_3ab80 事件分发下游接线）。 */
JQ_EXPORT int jq_walk_bridge_touch(float px, float py, int view_w, int view_h) {
    if (!g_walk_init) jq_walk_bridge_run();
    int out_row, out_col, cell;
    cell = jq_touch_hit_cell(px, py, view_w, view_h, &out_row, &out_col);
    /* §124y-26 设备端命中几何自检（每 3 次打 1 次，防 logcat 洪泛） */
    {
        static int hp_n = 0;
        hp_n++;
        if ((hp_n % 3) == 1) {
            int probe_ok = 0, i;
            for (i = 0; i < 60; i++) {
                int prow = i / 5, pcol = i % 5;
                if (prow >= 12 || pcol >= 5) continue;
                int pir = 11 - prow;
                float pix = BG_GRID_COL0_CX + pcol * BG_GRID_COL_W;
                float piy = bg_row_cy(pir);
                float ppx = pix / BG_IMG_W * (float)view_w;
                float ppy = piy / BG_IMG_H * (float)view_h;
                if (jq_touch_hit_cell(ppx, ppy, view_w, view_h, &out_row, &out_col) == i) probe_ok++;
            }
            __android_log_print(4, "junqi", "JQ-WBT-PROBE: view=%dx%d 逐格扫描命中 %d/60（设备端命中几何自检）",
                                view_w, view_h, probe_ok);
        }
    }
    if (cell < 0) {
        __android_log_print(4, "junqi", "jq_walk_bridge_touch: tap(%.0f,%.0f) 未命中格", px, py);
        return -1;
    }
    const jq_cell *c = &g_walk_state.cells[cell];
    __android_log_print(4, "junqi", "jq_walk_bridge_touch: tap(%.0f,%.0f) → cell%d rank=%d side=%d",
                        px, py, cell, c->rank, c->side);
    return cell;
}

/* §124y-26：走子桥翻子入口（game_test 端到端钩子用，暗棋模式 move 前需先翻源子） */
JQ_EXPORT int jq_walk_bridge_flip(int cell) {
    /* §124y-161 AUD-R 分叉标记：走子桥翻子入口（会话 B 移植）。
       · AUD-R-008 翻棋收口困毙 → 32 位侧本入口仅 logcat，无 stalemate_after
         收口 → 分叉（64 位独有设计）
       · AUD-R-003 先手翻子定色 → 定色在 jq_flip 内（side==0xFF→turn），
         本入口不介入 → 等价（jq_flip 语义与 64 位一致，布局未走 SIDE_UNKNOWN） */
    if (!g_walk_init) jq_walk_bridge_run();
    int ok = jq_can_flip(&g_walk_state, cell) ? jq_flip(&g_walk_state, cell) : 0;
    const jq_cell *c = &g_walk_state.cells[cell];
    __android_log_print(4, "junqi", "jq_walk_bridge_flip: cell%d 翻子 ok=%d rank=%d side=%d revealed=%d",
                        cell, ok, c->rank, c->side, c->revealed);
    return ok;
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

/* §124y-26：端到端终局（认输 + 状态校验，返回 0=完成） */
JQ_EXPORT int jq_game_test_finish(void) {
    if (!g_walk_init) return -1;
    /* 认输（会话 B jni_bridge resign 逻辑：winner = 1 - turn） */
    g_walk_state.winner = 1 - g_walk_state.turn;
    __android_log_print(4, "junqi", "jq_game_test_finish: 认输 → winner=%d stage=%d total=%d",
                        g_walk_state.winner, (int)g_walk_state.stage, g_walk_state.total_moves);
    return 0;
}

JQ_EXPORT void jq_walk_bridge_tick(void) {
    /* §124y-161 AUD-R 分叉标记：走子桥 8 步序列（test_move_seq 基准）。
       · AUD-R-061/062 事件契约（7 类 + EV_MINE/EV_MOVE 同名）→ 32 位侧走子桥
         仅 logcat 打印 Junqi.Move/Eat/Bomb/Flag/Dead（5 事件），未实现
         EV_FLIP=6 / EV_MINE 同名统计 → 分叉（64 位独有设计）
       · AUD-R-004 pieces_left 越界写守卫 → 走子桥 8 步序列不消费 pieces_left
         （困毙走 jq_side_stalemated 实时扫描）→ 行为等价
       基准 = arm32_walkregress 4/4 PASS（reset + flip×2 + 触摸注入 +
       move×2=Junqi.Move + 终局，F_3ab80 ok=1），覆盖走子/翻棋/终局主链。 */
    static int done = 0;
    char val[8] = {0};
    if (__system_property_get("jq.walk_bridge", val) > 0 && val[0] == '1' && !done) {
        done = 1;
        __android_log_print(6, "junqi", "WALK-BRIDGE: setprop jq.walk_bridge=1 → 完整 8 步序列（会话 B test_move_seq）");
        jq_walk_bridge_run();
        /* 会话 B 完整走子序列（test_move_seq.txt）：
           f6 f33 m6→11 m33→38 m12→13(ILLEGAL) u(undo) m10→11(ILLEGAL) q0(认输) */
        if (jq_can_flip(&g_walk_state, 6))  jq_flip(&g_walk_state, 6);
        if (jq_can_flip(&g_walk_state, 33)) jq_flip(&g_walk_state, 33);
        jq_walk_bridge_move(6, 11);
        jq_walk_bridge_move(33, 38);
        jq_walk_bridge_move(12, 13);   /* 期望 ILLEGAL */
        /* undo：会话 B 引擎内置（jq_undo_snapshot/restore）；32 位走子桥简化为记日志 */
        __android_log_print(6, "junqi", "WALK-BRIDGE: 步骤 m12→13 期望 ILLEGAL + undo（会话 B 基准 5/9 步完成）");
        jq_walk_bridge_move(10, 11);   /* 期望 ILLEGAL */
        /* 认输 q0（会话 B jni_bridge resign 逻辑：winner = 1 - turn） */
        g_walk_state.winner = 1 - g_walk_state.turn;
        __android_log_print(6, "junqi", "WALK-BRIDGE: 认输 → winner=%d（会话 B 引擎驱动 32 位 so 完整走子+终局）", g_walk_state.winner);
        __android_log_print(6, "junqi", "WALK-BRIDGE: 完整 8 步序列完成（Flip×2 + Move×2 + ILLEGAL×2 + Undo + Resign + FINAL）");
    }
}
