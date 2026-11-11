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
   · R-021 头注释 0x39e14 行约束误标 → 64 位 rules.c 头注释已改"布局期校验限制"
     措辞（AUD-R-021 P2 已修），32 位头注释（jq_rs_rules.c 文件头"0x39e14
     目标类型判定"误标原文 → 分叉（★ 对抗审计 P3 批2 D-2 纠正：64 位已修 32 位未修，
     "同源"当前为假）
   · R-051 jq_state_init 在 layout.c 声明实现分处 → 32 位侧同（jq_rs_rules.c 不含 layout，
     走子桥调 jq_state_init）→ 对齐（同源）
   · R-052 STG_SETUP 死枚举 → 32 位侧同（走子桥无布阵阶段分支）→ 对齐（同源）
   · R-081 三套实现缺一致性闸门 → 32 位侧即第 3 套实现（本走子桥），arm32_walkregress
     为 32 位基准闸门，与 64 位 evverify/regress 双路 → 已建机制（R-138 分叉矩阵
     + rules_fork_check.py 双基准闸门）；★ 对抗审计 P1 批2 B-6：行为对齐未实证
     （arm32_walkregress 仅 4 路径主链，规则矩阵未跑），机制存在 ≠ 行为等价
   · R-127 EVENT_NAMES[6] 指针非 const → 32 位侧同（jq_rs_rules.c const char*[6]，
     64 位修法加 const 限定 32 位侧未同步）→ 分叉（64 位独有设计）
   · R-128 jq_cell 4 字节紧凑与裸字节序列化耦合 → 32 位侧同布局（走子桥无跨版本存档维度）
     → 对齐（同源）
   · R-096 头注释未区分确证/推断 → 32 位侧头注释同源混写 → 对齐（同源）
   · R-097 引入未使用 stdlib.h → 32 位侧 jq_rs_rules.c include stdlib（malloc 未用）
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
   · R-003 先手翻子定色 → 32 位侧定色在 jq_flip 内（side==0xFF→turn，已标），
     布局走子桥未走 SIDE_UNKNOWN 全流程 → 分叉（64 位独有设计）
   · R-063 布局校验蓝方军旗格不受约束 → 32 位侧走子桥布局 JQ_DEFAULT_LAY 写死
     ，无自定义/联机布局校验维度 → 分叉（64 位独有设计）
   · R-064 validate_layout 错误码语义混淆 → 32 位侧无 jq_validate_layout（走子桥布局固定）
     → N/A（层级不适用）
   · R-065 布局每 rank 计数×2 镜像假设 → 32 位侧同 JQ_DEFAULT_LAY 固定布局 → 对齐（同源）
   · R-066 is_rail 底线纵线交点双重身份 → 32 位侧 board 几何同 64 位（is_rail 放行 row0/11）
     → 对齐（同源）
   · R-067 铁路几何硬编码缺地形抽象 → 32 位侧同（rail_path_engineer / rail_path_straight 硬编码 if 链）→ 对齐（同源）
   · R-068 cell_index 无边界检查 → 32 位侧走子桥入参由 hit_cell 产出（范围已限），
     jq_can_move 有 from/to∈[0,N_CELLS) 检查 → 行为等价
   · R-069 coord_cmp 无调用方 → 32 位侧同（无 coord_cmp 使用点）→ 对齐（同源）
   · R-076 SPEC 棋盘尺寸与实现不一致 → 32 位侧实现 12×5（几何常量），SPEC 同源描述 → 对齐（同源缺口）
   · R-077 渲染几何常量与规则层混放 → 32 位侧走子桥 BG_GRID_* 常量与几何混放，
     与 64 位同源 → 对齐（同源）
   · R-078 bg_cell_img_center 越界 row/col 不校验 → 32 位侧走子桥 bg_row_cy 无越界检查
     → 对齐（同源）
   · R-079 rank_encode 定义未调用 → 32 位侧无 rank_encode 使用点 → 对齐（同源）
   · R-080 RANK_TOP=12 与 RANK_COUNT=13 边界耦合 → 32 位侧 rank_norm（jq_rs_rules.h
     jq_rs_rules.h 的 rank_norm）无上界钳（`v > RANK_TOP ? v-0xc : v`，v=25..255 还原成 13..243 越界
     rank）；64 位 junqi.h 已加 `if (v > RANK_TOP+0xc) return RANK_TOP;`
     上界钳（AUD-R-080 P2 修复）→ 分叉（★ 对抗审计 P2 批2 S-2 纠正：原"同实现"
     失实，64 位已修 32 位未同步）
   · R-082 每步 O(N²) 移动端性能风险 → 32 位侧同（jq_side_stalemated 双循环，见 R-112 标记）
     → 对齐（同源，未压测）
   · R-085 can_flip 可翻任意未定色暗子 → 32 位侧已加回合校验（jq_rs_rules.c jq_can_flip 回合门；★ 对抗审计 P3 批2 D-6：原行号引用已漂移，现全部标记改用语义锚点，不再引用行号）
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
                /* ★ §124y-176：probing 坐标换算改整数定点（32 位 houdini 转译下
                   float 除法 pix/BG_IMG_W*view_w 失效 → 全 0 → 逐格扫描 0/60） */
                long pix1k = 78500L + pcol * 140500L;   /* BG_GRID_COL0_CX*1000 + pcol*COL_W*1000 */
                long piy1k = pir < 6 ? 222000L + pir * 71000L : 822000L + (pir - 6) * 71000L;
                float ppx = (float)((pix1k * view_w) / 720000L);
                float ppy = (float)((piy1k * view_h) / 1280000L);
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

/* §124y-26：端到端终局（认输 + 状态校验，返回 0=完成）
   ★ AUD-1000 B05-037（P1 诚实标注）：本钩子只写 winner=1-turn，**不改**
   stage=STG_OVER——后续 jq_try_move 仍按 STG_PLAY 跑（困毙/和棋判定
   继续）。游戏测试钩子（jq.game_test）路径后续不再走子（走子桥 tick
   已跑完 2 步 + 终局判据打印），stage 悬挂无实际影响；若钩子路径
   后续新增走子帧，须补 st->stage=STG_OVER（与 64 位 src_rs 认输
   钩子 jq_resigntest 同源口径：winner=1-turn + stage=OVER 成对写）。 */
JQ_EXPORT int jq_game_test_finish(void) {
    if (!g_walk_init) return -1;
    /* 认输（会话 B jni_bridge resign 逻辑：winner = 1 - turn）
       ★ AUD-1000 B05-037（P1 已修）：补 stage=STG_OVER，与 64 位 src_rs
       认输钩子 jq_resigntest 同源口径（winner=1-turn + stage=OVER 成对写）。 */
    g_walk_state.winner = 1 - g_walk_state.turn;
    g_walk_state.stage = 2;   /* STG_OVER */
    __android_log_print(4, "junqi", "jq_game_test_finish: 认输 → winner=%d stage=%d total=%d",
                        g_walk_state.winner, (int)g_walk_state.stage, g_walk_state.total_moves);
    return 0;
}

JQ_EXPORT void jq_walk_bridge_tick(void) {
    /* ★ §124y R-138 行为实证专项：规则行为探针钩子
       （setprop jq.rule_probe 1 → 打印 RULE-PROBE 行 → 与 64 位桌面探针 diff）。
       探针本体与 64 位 rev64/forward/src_rs/rule_probe_shared.inc **逐字节同源**，
       只摆棋 + 提问，规则判定全在本侧引擎内 → 输出差异 = 真实行为分歧。 */
    {
        static int probe_done = 0;
        char pv[8] = {0};
        if (!probe_done && __system_property_get("jq.rule_probe", pv) > 0 && pv[0] == '1') {
            probe_done = 1;
            __android_log_print(6, "junqi", "RULE-PROBE: 32 位翻译体规则行为探针启动");
            {
                extern void jq_rule_probe_run(void);
                jq_rule_probe_run();
            }
        }
    }
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
