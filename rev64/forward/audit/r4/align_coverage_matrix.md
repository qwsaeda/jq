# 对齐覆盖矩阵（自动生成：rev64/tools/gen_align_coverage.py）

> 生成时间: 2026-11-20 04:22（每次 `python3 rev64/tools/gen_align_coverage.py` 重生；引用前确认与当前源码一致——evverify 真值以 `jq_evverify --all` SUMMARY 动态为准）

> 用途：规则条款 ↔ 桌面断言 ↔ 双端探针 三向覆盖关系可见化；并集中登记**硬编码期望值基准点**（改行为必须同步的清单）。

## ① 规则原文机判条款（33 项）

| # | 条款（节选） | 状态 |
|---|---|---|
| 1 | [明棋] 公路相邻一步合法 (4,1)->(4,2) | ✅ 有断言 |
| 2 | [明棋] 公路跨两格非法 (4,1)->(4,3) | ✅ 有断言 |
| 3 | [明棋] 非工兵铁路纵向直走远格 (4,2)->(7,2) | ✅ 有断言 |
| 4 | [明棋] 非工兵铁路不能拐弯 (5,4)->(6,2) | ✅ 有断言 |
| 5 | [明棋] 工兵铁路拐弯 (5,4)->(6,2) | ✅ 有断言 |
| 6 | [明棋] 铁路滑行被中间子阻断 (2,2)->(9,2) 非法 | ✅ 有断言 |
| 7 | [明棋] 行营安全岛：敌方不能吃行营中的棋子 | ✅ 有断言 |
| 8 | [明棋] 大本营内棋子不能再移动 | ✅ 有断言 |
| 9 | [明棋] 司令吃军长：攻方留 EV_EAT | ✅ 有断言 |
| 10 | [明棋] 军长撞司令：攻方死 EV_DEAD，司令留 | ✅ 有断言 |
| 11 | [明棋] 同级同归于尽 EV_EAT 两格空（原版 0x31de0 实证） | ✅ 有断言 |
| 12 | [明棋] 炸弹撞司令：双方消失 EV_BOMB | ✅ 有断言 |
| 13 | [明棋] 炸弹炸军旗：双方消失且攻方(蓝)胜 | ✅ 有断言 |
| 14 | [明棋] 工兵挖雷：工兵进目标格、雷消失 EV_MINE | ✅ 有断言 |
| 15 | [明棋] 非工兵撞雷：攻方死、雷留 EV_DEAD | ✅ 有断言 |
| 16 | [明棋] 任何棋子扛军旗即胜 EV_FLAG | ✅ 有断言 |
| 17 | [明棋] 普通子也能扛军旗 | ✅ 有断言 |
| 18 | [明棋] 连续 70 步无吃子判和 | ✅ 有断言 |
| 19 | [明棋] 总步数达 1000 判和 | ✅ 有断言 |
| 20 | [明棋] 轮红无棋可走 → 红判负（蓝胜） | ✅ 有断言 |
| 21 | [暗棋] 未翻开的己方子不可走 | ✅ 有断言 |
| 22 | [暗棋] 非工兵不能吃地雷 | ✅ 有断言 |
| 23 | [暗棋] 工兵能吃地雷 | ✅ 有断言 |
| 24 | [暗棋] 工兵未挖满 3 雷不能扛旗 | ✅ 有断言 |
| 25 | [暗棋] 工兵挖满 3 雷可扛旗 | ✅ 有断言 |
| 26 | [暗棋] 非工兵不能扛军旗 | ✅ 有断言 |
| 27 | [暗棋] 未翻地雷可翻（jq_can_flip：翻雷是合法翻子动作） | ✅ 有断言 |
| 28 | [暗棋] 仅剩未翻地雷 → 困毙（原版 0x30320 可翻扫描排地雷，翻雷不算可行动） | ✅ 有断言 |
| 29 | [暗棋] 大本营安全岛：敌方不能吃大本营中的普通子 | ✅ 有断言 |
| 30 | [暗棋] 大本营安全岛例外：工兵可吃大本营中的地雷 | ✅ 有断言 |
| 31 | [暗棋] 大本营安全岛例外：工兵挖满 3 雷可扛大本营军旗 | ✅ 有断言 |
| 32 | [暗棋] 大本营安全岛例外：挖满 3 雷后工兵可扛大本营军旗 | ✅ 有断言 |
| 33 | [明棋] 大本营非安全岛（rulelay 未禁，军旗须任何子可扛） | ✅ 有断言 |

## ② 暗棋对齐用例（断言强度：标记 + 退出码依赖 双条件）

| 用例 | 断言标记 | 退出码依赖 | 判定 |
|---|---|---|---|
| dark_02_switch_test | 10 | 是 | ✅ 真断言 |
| dark_32combo_matrix_test | 6 | 是 | ✅ 真断言 |
| dark_4branch_clear_test | 12 | 是 | ✅ 真断言 |
| dark_4switch_combo_test | 10 | 是 | ✅ 真断言 |
| dark_ai_score_test | 24 | 是 | ✅ 真断言 |
| dark_allclose_switch_test | 12 | 是 | ✅ 真断言 |
| dark_base_empty_test | 12 | 是 | ✅ 真断言 |
| dark_bfs_around_test | 12 | 是 | ✅ 真断言 |
| dark_bfs_thru_test | 12 | 是 | ✅ 真断言 |
| dark_bomb_test | 5 | 是 | ✅ 真断言 |
| dark_camp_safety_test | 22 | 是 | ✅ 真断言 |
| dark_camp_special_test | 12 | 是 | ✅ 真断言 |
| dark_colorwalk_test | 10 | 是 | ✅ 真断言 |
| dark_cross_rail_test | 12 | 是 | ✅ 真断言 |
| dark_cross_switch_test | 12 | 是 | ✅ 真断言 |
| dark_darkoff_matrix_test | 10 | 是 | ✅ 真断言 |
| dark_eat_reset_count_test | 12 | 是 | ✅ 真断言 |
| dark_flag_test | 7 | 是 | ✅ 真断言 |
| dark_flag_win_test | 8 | 是 | ✅ 真断言 |
| dark_flagoff_clear_test | 8 | 是 | ✅ 真断言 |
| dark_flagoff_cont_test | 10 | 是 | ✅ 真断言 |
| dark_flagoff_stalemate_test | 10 | 是 | ✅ 真断言 |
| dark_flip32_combo_test | 6 | 是 | ✅ 真断言 |
| dark_flip_color_test | 10 | 是 | ✅ 真断言 |
| dark_flip_draw_count_test | 6 | 是 | ✅ 真断言 |
| dark_flip_island_test | 12 | 是 | ✅ 真断言 |
| dark_flip_test | 1 | 是 | ✅ 真断言 |
| dark_fullgame | 5 | 是 | ✅ 真断言 |
| dark_fullgame_ai_ms_track_test | 10 | 是 | ✅ 真断言 |
| dark_fullgame_flip_end | 10 | 是 | ✅ 真断言 |
| dark_fullgame_fullon_ms_track_test | 10 | 是 | ✅ 真断言 |
| dark_fullgame_ms_track_test | 12 | 是 | ✅ 真断言 |
| dark_fullgame_redwin_ms_track_test | 8 | 是 | ✅ 真断言 |
| dark_fullgame_reset_test | 8 | 是 | ✅ 真断言 |
| dark_fullgame_statemachine_test | 10 | 是 | ✅ 真断言 |
| dark_fullgame_win_ms_track_test | 8 | 是 | ✅ 真断言 |
| dark_hq_freeze_test | 14 | 是 | ✅ 真断言 |
| dark_mine_flag_test | 10 | 是 | ✅ 真断言 |
| dark_mine_test | 5 | 是 | ✅ 真断言 |
| dark_mineoff_stalemate_test | 10 | 是 | ✅ 真断言 |
| dark_mixed_switch_test | 10 | 是 | ✅ 真断言 |
| dark_moves_seq_test | 10 | 是 | ✅ 真断言 |
| dark_multi_switch_matrix_test | 18 | 是 | ✅ 真断言 |
| dark_rail_c0c4_worker_seg_test | 18 | 是 | ✅ 真断言 |
| dark_rail_c1c3_worker_seg_test | 18 | 是 | ✅ 真断言 |
| dark_rail_c1c4cross_test | 14 | 是 | ✅ 真断言 |
| dark_rail_c2_fullseg_test | 16 | 是 | ✅ 真断言 |
| dark_rail_c2_worker_seg_test | 16 | 是 | ✅ 真断言 |
| dark_rail_c4corner_cross_eat_test | 14 | 是 | ✅ 真断言 |
| dark_rail_corner11_c4_test | 12 | 是 | ✅ 真断言 |
| dark_rail_corner11_test | 12 | 是 | ✅ 真断言 |
| dark_rail_corner_nonrail_test | 12 | 是 | ✅ 真断言 |
| dark_rail_cross_eat_test | 14 | 是 | ✅ 真断言 |
| dark_rail_cross_flagmine_test | 14 | 是 | ✅ 真断言 |
| dark_rail_cross_test | 20 | 是 | ✅ 真断言 |
| dark_rail_fullseg_test | 12 | 是 | ✅ 真断言 |
| dark_rail_r0r11_ends_test | 14 | 是 | ✅ 真断言 |
| dark_rail_r5r6_fullrow_test | 14 | 是 | ✅ 真断言 |
| dark_rail_r5r6_worker_eat_test | 16 | 是 | ✅ 真断言 |
| dark_rail_r6_fullrow_worker_test | 16 | 是 | ✅ 真断言 |
| dark_rail_r6cross_test | 14 | 是 | ✅ 真断言 |
| dark_rail_seg8_test | 10 | 是 | ✅ 真断言 |
| dark_rail_segment_test | 12 | 是 | ✅ 真断言 |
| dark_rail_topo_test | 22 | 是 | ✅ 真断言 |
| dark_rail_topology_matrix_test | 14 | 是 | ✅ 真断言 |
| dark_rail_walk_test | 12 | 是 | ✅ 真断言 |
| dark_redwin_ai_test | 8 | 是 | ✅ 真断言 |
| dark_rule_switch_test | 10 | 是 | ✅ 真断言 |
| dark_stalemate_matrix_test | 18 | 是 | ✅ 真断言 |
| dark_straight_rail_test | 12 | 是 | ✅ 真断言 |
| dark_tongji_flip_test | 8 | 是 | ✅ 真断言 |
| dark_touch_flip_flow_test | 32 | 是 | ✅ 真断言 |
| dark_undo_count_test | 8 | 是 | ✅ 真断言 |
| dark_w1off_cont_test | 10 | 是 | ✅ 真断言 |
| dark_w1off_multi_test | 10 | 是 | ✅ 真断言 |
| dark_win_test | 4 | 是 | ✅ 真断言 |
| dark_wrap_across_test | 12 | 是 | ✅ 真断言 |
| draw_exist_test | 4 | 是 | ✅ 真断言 |
| eat_dir_test | 5 | 是 | ✅ 真断言 |
| flip_draw_guard | 1 | 是 | ✅ 真断言 |
| flip_eat_test | 3 | 是 | ✅ 真断言 |
| flip_rail_test | 6 | 是 | ✅ 真断言 |
| layout_check_test | 7 | 是 | ✅ 真断言 |
| rail_block_test | 6 | 是 | ✅ 真断言 |
| rank_order_test | 5 | 是 | ✅ 真断言 |
| rule_text_conformance | 35 | 否 | ✅ 真断言 |

**合计 86 用例 / 981 条断言标记**（零断言或假绿候选 0 个：无）

## ③ 32↔64 双端探针（36 case，覆盖 AUD-R 24 项）

| 组 | case 数 | case 名 |
|---|---|---|
| A 组 | 6 | A1_cm_mid_to_bottom, A1_cm_result, A2_cm_bottom_row, A3_cm_into_camp, A4_cm_into_base, A5_cm_oob |
| B 组 | 4 | B1_cm_normal_vs_mine, B2_cm_worker_vs_mine, B3_bomb_vs_flag, B4_normal_vs_flag |
| C 组 | 7 | C1_cm_unrevealed, C2_flip_unknown, C3_flip_mine, C4_stalemate_only_mine, C4b_stalemate_only_flag, C5_stalemate_side_ne_turn … |
| D 组 | 3 | D1_flip_count, D2_stalemate_with_flag, DONE |
| E 组 | 9 | E1_worker_c2_same_seg, E2_worker_c2_cross_row3, E3_worker_c2_cross_row8, E4_worker_c0_to_c2, E5_worker_c4_to_c2, E6_worker_c1_to_c2 … |
| F 组 | 3 | F1_flip_first_side, F2_flip_then_stalemate, F3_flip_both_sides |
| G 组 | 4 | G1_cm_unrevealed_eat, G2_cm_enemy_dark_eat, G3_try_side0xff, G4_kill_all_stalemate |

探针覆盖的 AUD-R 项：R-001, R-002, R-003, R-004, R-005, R-006, R-007, R-008, R-009, R-011, R-015, R-016, R-018, R-020, R-025, R-028, R-029, R-030, R-033, R-045, R-071, R-073, R-086, R-089

## ④ 硬编码期望值基准点台账（改行为必须同步）

| 基准文件 | 期望条数 | 门禁入口 |
|---|---|---|
| src_rs/test_expected.txt | 9 | make benchcheck |
| src_rs/consistency_desktop.c BENCH_EXPECT | 9 | make benchcheck（双源需同步） |
| src_rs/evverify.c | 174 | make evverify |
| src_rs/dark_align/*.c（chk 断言） | 981 | dark_align_regress.sh |
| src_rs/rule_probe_shared.inc（探针期望值在 diff 中） | 36 | rule_probe_compare.py |
| forward/docs_rs/UI_COMPARE.md（Δ 表） | 12 项 | layout_align_check.py --baseline |
| rev64/dist/event_code_map.json / event_names_76.json | 73 | event_names_alignment_check.py |

## ⑤ 覆盖缺口提示（自动判定）

- ⚠️ 零断言/假绿候选用例：0 个（全部用例具备失败标记 + 退出码依赖）
- ⚠️ 分叉矩阵 102 条中仅 16 条已实证（其余为「已标记未实证」= 未对齐未验证）
- ⚠️ 双端探针 36 条 vs 规则条款 33 项 → 条款级探针化尚有缺口

