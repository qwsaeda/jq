# 自动化对抗审计探针（机器穷举）

> 生成器：`audit/probe_audit.py`　条目数：**820**
> 姊妹文档：子代理深度审计（rules / jni-render / ui-geom / build-pkg / doc-truth / rev32）

## 统计

| 严重级 | 条数 |
|---|---|
| P0 | 0 |
| P1 | 0 |
| P2 | 257 |
| P3 | 563 |

## 按探针分组

| 探针 | 条数 |
|---|---|
| todo | 498 |
| coverage | 154 |
| unsafe-func | 74 |
| geom-delta | 42 |
| array-index | 24 |
| ui-geom | 14 |
| int-overflow | 6 |
| artifact | 4 |
| test-gap | 3 |
| claim | 1 |

---

## AUD-A-0001 [P2] 数组 line2[l2] 下标在 main() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/bt_selftest.c:196
- 问题：line2[l2] 索引，line2 声明大小 [160]
- 依据：函数 main()（src_rs/bt_selftest.c:37-318）体内未对 l2 做范围检查
- 修法：在索引前加 if (l2 < 0 || l2 >= 160) 拒绝并记日志

## AUD-A-0002 [P2] 数组 cells[mto] 下标在 main() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/bt_semtest.c:92
- 问题：cells[mto] 索引，cells 声明大小 [5, 7, 33, 40]
- 依据：函数 main()（src_rs/bt_semtest.c:71-419）体内未对 mto 做范围检查
- 修法：在索引前加 if (mto < 0 || mto >= 40) 拒绝并记日志

## AUD-A-0003 [P2] 数组 cells[mto] 下标在 main() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/bt_semtest.c:94
- 问题：cells[mto] 索引，cells 声明大小 [5, 7, 33, 40]
- 依据：函数 main()（src_rs/bt_semtest.c:71-419）体内未对 mto 做范围检查
- 修法：在索引前加 if (mto < 0 || mto >= 40) 拒绝并记日志

## AUD-A-0004 [P2] 数组 cells[mfrom] 下标在 main() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/bt_semtest.c:95
- 问题：cells[mfrom] 索引，cells 声明大小 [5, 7, 33, 40]
- 依据：函数 main()（src_rs/bt_semtest.c:71-419）体内未对 mfrom 做范围检查
- 修法：在索引前加 if (mfrom < 0 || mfrom >= 40) 拒绝并记日志

## AUD-A-0005 [P2] 数组 b[hit] 下标在 run_popup_actions() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3485
- 问题：b[hit] 索引，b 声明大小 [0, 1, 2, 3]
- 依据：函数 run_popup_actions()（src_rs/evverify.c:3473-3512）体内未对 hit 做范围检查
- 修法：在索引前加 if (hit < 0 || hit >= 3) 拒绝并记日志

## AUD-A-0006 [P2] 数组 b[hit] 下标在 run_popup_actions() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3493
- 问题：b[hit] 索引，b 声明大小 [0, 1, 2, 3]
- 依据：函数 run_popup_actions()（src_rs/evverify.c:3473-3512）体内未对 hit 做范围检查
- 修法：在索引前加 if (hit < 0 || hit >= 3) 拒绝并记日志

## AUD-A-0007 [P2] 数组 b[hit] 下标在 run_popup_actions() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3496
- 问题：b[hit] 索引，b 声明大小 [0, 1, 2, 3]
- 依据：函数 run_popup_actions()（src_rs/evverify.c:3473-3512）体内未对 hit 做范围检查
- 修法：在索引前加 if (hit < 0 || hit >= 3) 拒绝并记日志

## AUD-A-0008 [P2] 数组 b[hit] 下标在 run_popup_actions() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3504
- 问题：b[hit] 索引，b 声明大小 [0, 1, 2, 3]
- 依据：函数 run_popup_actions()（src_rs/evverify.c:3473-3512）体内未对 hit 做范围检查
- 修法：在索引前加 if (hit < 0 || hit >= 3) 拒绝并记日志

## AUD-A-0009 [P2] 数组 b[hit] 下标在 run_popup_actions() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3507
- 问题：b[hit] 索引，b 声明大小 [0, 1, 2, 3]
- 依据：函数 run_popup_actions()（src_rs/evverify.c:3473-3512）体内未对 hit 做范围检查
- 修法：在索引前加 if (hit < 0 || hit >= 3) 拒绝并记日志

## AUD-A-0010 [P2] 数组 tb[BT_BUF] 下标在 jq_bt_ctrl_send() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:514
- 问题：tb[BT_BUF] 索引，tb 声明大小 [128]
- 依据：函数 jq_bt_ctrl_send()（src_rs/jni_bridge.c:511-524）体内未对 BT_BUF 做范围检查
- 修法：在索引前加 if (BT_BUF < 0 || BT_BUF >= 128) 拒绝并记日志

## AUD-A-0011 [P2] 数组 g_event_cnt[ftev] 下标在 jq_bt_test_run() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:1364
- 问题：g_event_cnt[ftev] 索引，g_event_cnt 声明大小 [0, 1, 7]
- 依据：函数 jq_bt_test_run()（src_rs/jni_bridge.c:1263-1283）体内未对 ftev 做范围检查
- 修法：在索引前加 if (ftev < 0 || ftev >= 7) 拒绝并记日志

## AUD-A-0012 [P2] 数组 g_event_cnt[mtev] 下标在 jq_bt_test_run() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:1437
- 问题：g_event_cnt[mtev] 索引，g_event_cnt 声明大小 [0, 1, 7]
- 依据：函数 jq_bt_test_run()（src_rs/jni_bridge.c:1263-1283）体内未对 mtev 做范围检查
- 修法：在索引前加 if (mtev < 0 || mtev >= 7) 拒绝并记日志

## AUD-A-0013 [P2] 数组 cnt[RANK_COUNT] 下标在 jq_standard_army_count() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/layout.c:33
- 问题：cnt[RANK_COUNT] 索引，cnt 声明大小 [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
- 依据：函数 jq_standard_army_count()（src_rs/layout.c:33-43）体内未对 RANK_COUNT 做范围检查
- 修法：在索引前加 if (RANK_COUNT < 0 || RANK_COUNT >= 12) 拒绝并记日志

## AUD-A-0014 [P2] 数组 cnt[RANK_COUNT] 下标在 jq_validate_layout() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/layout.c:144
- 问题：cnt[RANK_COUNT] 索引，cnt 声明大小 [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
- 依据：函数 jq_validate_layout()（src_rs/layout.c:142-168）体内未对 RANK_COUNT 做范围检查
- 修法：在索引前加 if (RANK_COUNT < 0 || RANK_COUNT >= 12) 拒绝并记日志

## AUD-A-0015 [P2] 数组 T_W[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:1222
- 问题：T_W[i] 索引，T_W 声明大小 [5]
- 依据：函数 if()（src_rs/render.c:1174-1175）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0016 [P2] 数组 T_X[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:1225
- 问题：T_X[i] 索引，T_X 声明大小 [5]
- 依据：函数 if()（src_rs/render.c:1174-1175）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0017 [P2] 数组 T_Y[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:1226
- 问题：T_Y[i] 索引，T_Y 声明大小 [5]
- 依据：函数 if()（src_rs/render.c:1174-1175）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0018 [P2] 数组 bx[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8974
- 问题：bx[i] 索引，bx 声明大小 [0, 1, 2]
- 依据：函数 if()（src_rs/render.c:8964-8965）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0019 [P2] 数组 BTN_COL[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8975
- 问题：BTN_COL[i] 索引，BTN_COL 声明大小 [2, 3, 4, 5, 6]
- 依据：函数 if()（src_rs/render.c:8964-8965）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 6) 拒绝并记日志

## AUD-A-0020 [P2] 数组 bx[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8978
- 问题：bx[i] 索引，bx 声明大小 [0, 1, 2]
- 依据：函数 if()（src_rs/render.c:8964-8965）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0021 [P2] 数组 gl2_ok[slot] 下标在 now_is_glthread_ctx() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9245
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1, 2]
- 依据：函数 now_is_glthread_ctx()（src_rs/render.c:9237-9241）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 2) 拒绝并记日志

## AUD-A-0022 [P2] 数组 pieces_left[SIDE_BLUE] 下标在 jq_try_move() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/rules.c:584
- 问题：pieces_left[SIDE_BLUE] 索引，pieces_left 声明大小 [2, 255]
- 依据：函数 jq_try_move()（src_rs/rules.c:501-744）体内未对 SIDE_BLUE 做范围检查
- 修法：在索引前加 if (SIDE_BLUE < 0 || SIDE_BLUE >= 255) 拒绝并记日志

## AUD-A-0023 [P2] 数组 pieces_left[SIDE_BLUE] 下标在 jq_try_move() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/rules.c:662
- 问题：pieces_left[SIDE_BLUE] 索引，pieces_left 声明大小 [2, 255]
- 依据：函数 jq_try_move()（src_rs/rules.c:501-744）体内未对 SIDE_BLUE 做范围检查
- 修法：在索引前加 if (SIDE_BLUE < 0 || SIDE_BLUE >= 255) 拒绝并记日志

## AUD-A-0024 [P2] 数组 nm[UI_LAY_NAME] 下标在 ui_layout_list() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/ui_state.c:478
- 问题：nm[UI_LAY_NAME] 索引，nm 声明大小 [0]
- 依据：函数 ui_layout_list()（src_rs/ui_state.c:435-487）体内未对 UI_LAY_NAME 做范围检查
- 修法：在索引前加 if (UI_LAY_NAME < 0 || UI_LAY_NAME >= N) 拒绝并记日志

## AUD-A-0025 [P2] 声明「蓝牙 v2 7 字段」的实现符号 bt_send_frame 不存在
- 探针：`claim`
- 位置：src_rs/jni_bridge.c
- 问题：找不到 bt_send_frame
- 依据：声明与实现不符
- 修法：补实现或修正文档声明

## AUD-A-0026 [P2] 原版 datah_009 未生成对应 UI_D009
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_009.bin 但无 UI_D009
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0027 [P2] 原版 datah_011 未生成对应 UI_D011
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_011.bin 但无 UI_D011
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0028 [P2] 原版 datah_014 未生成对应 UI_D014
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_014.bin 但无 UI_D014
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0029 [P2] 原版 datah_018 未生成对应 UI_D018
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_018.bin 但无 UI_D018
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0030 [P2] 原版 datah_040 未生成对应 UI_D040
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_040.bin 但无 UI_D040
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0031 [P2] 原版 datah_048 未生成对应 UI_D048
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_048.bin 但无 UI_D048
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0032 [P2] 原版 datah_056 未生成对应 UI_D056
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_056.bin 但无 UI_D056
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0033 [P2] 原版 datah_074 未生成对应 UI_D074
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_074.bin 但无 UI_D074
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0034 [P2] 原版 datah_076 未生成对应 UI_D076
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_076.bin 但无 UI_D076
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0035 [P2] 原版 datah_080 未生成对应 UI_D080
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_080.bin 但无 UI_D080
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0036 [P2] 原版 datah_081 未生成对应 UI_D081
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_081.bin 但无 UI_D081
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0037 [P2] 原版 datah_087 未生成对应 UI_D087
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_087.bin 但无 UI_D087
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0038 [P2] 原版 datah_088 未生成对应 UI_D088
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_088.bin 但无 UI_D088
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0039 [P2] 原版 datah_091 未生成对应 UI_D091
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_091.bin 但无 UI_D091
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0040 [P2] 原版 datah_092 未生成对应 UI_D092
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_092.bin 但无 UI_D092
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0041 [P2] 原版 datah_093 未生成对应 UI_D093
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_093.bin 但无 UI_D093
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0042 [P2] 原版 datah_103 未生成对应 UI_D103
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_103.bin 但无 UI_D103
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0043 [P2] 原版 datah_105 未生成对应 UI_D105
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_105.bin 但无 UI_D105
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0044 [P2] 原版 datah_106 未生成对应 UI_D106
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_106.bin 但无 UI_D106
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0045 [P2] 原版 datah_111 未生成对应 UI_D111
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_111.bin 但无 UI_D111
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0046 [P2] 原版 datah_115 未生成对应 UI_D115
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_115.bin 但无 UI_D115
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0047 [P2] 原版 datah_116 未生成对应 UI_D116
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_116.bin 但无 UI_D116
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0048 [P2] 原版 datah_117 未生成对应 UI_D117
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_117.bin 但无 UI_D117
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0049 [P2] 原版 datah_124 未生成对应 UI_D124
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_124.bin 但无 UI_D124
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0050 [P2] 原版 datah_125 未生成对应 UI_D125
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_125.bin 但无 UI_D125
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0051 [P2] 原版 datah_126 未生成对应 UI_D126
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_126.bin 但无 UI_D126
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0052 [P2] 原版 datah_127 未生成对应 UI_D127
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_127.bin 但无 UI_D127
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0053 [P2] 原版 datah_128 未生成对应 UI_D128
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_128.bin 但无 UI_D128
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0054 [P2] 原版 datah_129 未生成对应 UI_D129
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_129.bin 但无 UI_D129
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0055 [P2] 原版 datah_130 未生成对应 UI_D130
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_130.bin 但无 UI_D130
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0056 [P2] 原版 datah_131 未生成对应 UI_D131
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_131.bin 但无 UI_D131
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0057 [P2] 原版 datah_132 未生成对应 UI_D132
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_132.bin 但无 UI_D132
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0058 [P2] 原版 datah_133 未生成对应 UI_D133
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_133.bin 但无 UI_D133
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0059 [P2] 原版 datah_134 未生成对应 UI_D134
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_134.bin 但无 UI_D134
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0060 [P2] 原版 datah_135 未生成对应 UI_D135
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_135.bin 但无 UI_D135
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0061 [P2] 原版 datah_136 未生成对应 UI_D136
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_136.bin 但无 UI_D136
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0062 [P2] 原版 datah_137 未生成对应 UI_D137
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_137.bin 但无 UI_D137
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0063 [P2] 原版 datah_138 未生成对应 UI_D138
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_138.bin 但无 UI_D138
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0064 [P2] 原版 datah_139 未生成对应 UI_D139
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_139.bin 但无 UI_D139
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0065 [P2] 原版 datah_140 未生成对应 UI_D140
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_140.bin 但无 UI_D140
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0066 [P2] 原版 datah_141 未生成对应 UI_D141
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_141.bin 但无 UI_D141
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0067 [P2] 原版 datah_142 未生成对应 UI_D142
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_142.bin 但无 UI_D142
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0068 [P2] 原版 datah_143 未生成对应 UI_D143
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_143.bin 但无 UI_D143
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0069 [P2] 原版 datah_144 未生成对应 UI_D144
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_144.bin 但无 UI_D144
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0070 [P2] 原版 datah_145 未生成对应 UI_D145
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_145.bin 但无 UI_D145
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0071 [P2] 原版 datah_146 未生成对应 UI_D146
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_146.bin 但无 UI_D146
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0072 [P2] 原版 datah_147 未生成对应 UI_D147
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_147.bin 但无 UI_D147
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0073 [P2] 原版 datah_148 未生成对应 UI_D148
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_148.bin 但无 UI_D148
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0074 [P2] 原版 datah_149 未生成对应 UI_D149
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_149.bin 但无 UI_D149
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0075 [P2] 原版 datah_150 未生成对应 UI_D150
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_150.bin 但无 UI_D150
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0076 [P2] 原版 datah_151 未生成对应 UI_D151
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_151.bin 但无 UI_D151
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0077 [P2] 原版 datah_152 未生成对应 UI_D152
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_152.bin 但无 UI_D152
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0078 [P2] 原版 datah_153 未生成对应 UI_D153
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_153.bin 但无 UI_D153
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0079 [P2] 原版 datah_154 未生成对应 UI_D154
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_154.bin 但无 UI_D154
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0080 [P2] 原版 datah_155 未生成对应 UI_D155
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_155.bin 但无 UI_D155
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0081 [P2] 原版 datah_156 未生成对应 UI_D156
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_156.bin 但无 UI_D156
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0082 [P2] 原版 datah_157 未生成对应 UI_D157
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_157.bin 但无 UI_D157
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0083 [P2] 原版 datah_158 未生成对应 UI_D158
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_158.bin 但无 UI_D158
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0084 [P2] 原版 datah_159 未生成对应 UI_D159
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_159.bin 但无 UI_D159
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0085 [P2] 原版 datah_160 未生成对应 UI_D160
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_160.bin 但无 UI_D160
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0086 [P2] 原版 datah_161 未生成对应 UI_D161
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_161.bin 但无 UI_D161
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0087 [P2] 原版 datah_162 未生成对应 UI_D162
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_162.bin 但无 UI_D162
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0088 [P2] 原版 datah_163 未生成对应 UI_D163
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_163.bin 但无 UI_D163
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0089 [P2] 原版 datah_164 未生成对应 UI_D164
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_164.bin 但无 UI_D164
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0090 [P2] 原版 datah_165 未生成对应 UI_D165
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_165.bin 但无 UI_D165
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0091 [P2] 原版 datah_166 未生成对应 UI_D166
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_166.bin 但无 UI_D166
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0092 [P2] 原版 datah_167 未生成对应 UI_D167
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_167.bin 但无 UI_D167
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0093 [P2] 原版 datah_168 未生成对应 UI_D168
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_168.bin 但无 UI_D168
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0094 [P2] 原版 datah_169 未生成对应 UI_D169
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_169.bin 但无 UI_D169
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0095 [P2] 原版 datah_170 未生成对应 UI_D170
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_170.bin 但无 UI_D170
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0096 [P2] 原版 datah_171 未生成对应 UI_D171
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_171.bin 但无 UI_D171
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0097 [P2] 原版 datah_172 未生成对应 UI_D172
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_172.bin 但无 UI_D172
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0098 [P2] 原版 datah_173 未生成对应 UI_D173
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_173.bin 但无 UI_D173
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0099 [P2] 原版 datah_174 未生成对应 UI_D174
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_174.bin 但无 UI_D174
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0100 [P2] 原版 datah_175 未生成对应 UI_D175
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_175.bin 但无 UI_D175
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0101 [P2] 原版 datah_176 未生成对应 UI_D176
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_176.bin 但无 UI_D176
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0102 [P2] 原版 datah_177 未生成对应 UI_D177
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_177.bin 但无 UI_D177
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0103 [P2] 原版 datah_178 未生成对应 UI_D178
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_178.bin 但无 UI_D178
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0104 [P2] 原版 datah_179 未生成对应 UI_D179
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_179.bin 但无 UI_D179
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0105 [P2] 原版 datah_180 未生成对应 UI_D180
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_180.bin 但无 UI_D180
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0106 [P2] 原版 datah_181 未生成对应 UI_D181
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_181.bin 但无 UI_D181
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0107 [P2] 原版 datah_182 未生成对应 UI_D182
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_182.bin 但无 UI_D182
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0108 [P2] 原版 datah_183 未生成对应 UI_D183
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_183.bin 但无 UI_D183
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0109 [P2] 原版 datah_184 未生成对应 UI_D184
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_184.bin 但无 UI_D184
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0110 [P2] 原版 datah_185 未生成对应 UI_D185
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_185.bin 但无 UI_D185
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0111 [P2] 原版 datah_186 未生成对应 UI_D186
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_186.bin 但无 UI_D186
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0112 [P2] 原版 datah_187 未生成对应 UI_D187
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_187.bin 但无 UI_D187
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0113 [P2] 原版 datah_188 未生成对应 UI_D188
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_188.bin 但无 UI_D188
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0114 [P2] 原版 datah_189 未生成对应 UI_D189
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_189.bin 但无 UI_D189
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0115 [P2] 原版 datah_190 未生成对应 UI_D190
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_190.bin 但无 UI_D190
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0116 [P2] 原版 datah_191 未生成对应 UI_D191
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_191.bin 但无 UI_D191
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0117 [P2] 原版 datah_192 未生成对应 UI_D192
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_192.bin 但无 UI_D192
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0118 [P2] 原版 datah_193 未生成对应 UI_D193
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_193.bin 但无 UI_D193
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0119 [P2] 原版 datah_194 未生成对应 UI_D194
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_194.bin 但无 UI_D194
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0120 [P2] 原版 datah_195 未生成对应 UI_D195
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_195.bin 但无 UI_D195
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0121 [P2] 原版 datah_196 未生成对应 UI_D196
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_196.bin 但无 UI_D196
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0122 [P2] 原版 datah_197 未生成对应 UI_D197
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_197.bin 但无 UI_D197
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0123 [P2] 原版 datah_198 未生成对应 UI_D198
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_198.bin 但无 UI_D198
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0124 [P2] 原版 datah_199 未生成对应 UI_D199
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_199.bin 但无 UI_D199
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0125 [P2] 原版 datah_200 未生成对应 UI_D200
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_200.bin 但无 UI_D200
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0126 [P2] 原版 datah_201 未生成对应 UI_D201
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_201.bin 但无 UI_D201
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0127 [P2] 原版 datah_202 未生成对应 UI_D202
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_202.bin 但无 UI_D202
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0128 [P2] 原版 datah_203 未生成对应 UI_D203
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_203.bin 但无 UI_D203
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0129 [P2] 原版 datah_204 未生成对应 UI_D204
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_204.bin 但无 UI_D204
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0130 [P2] 原版 datah_205 未生成对应 UI_D205
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_205.bin 但无 UI_D205
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0131 [P2] 原版 datah_206 未生成对应 UI_D206
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_206.bin 但无 UI_D206
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0132 [P2] 原版 datah_207 未生成对应 UI_D207
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_207.bin 但无 UI_D207
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0133 [P2] 原版 datah_208 未生成对应 UI_D208
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_208.bin 但无 UI_D208
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0134 [P2] 原版 datah_209 未生成对应 UI_D209
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_209.bin 但无 UI_D209
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0135 [P2] 原版 datah_210 未生成对应 UI_D210
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_210.bin 但无 UI_D210
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0136 [P2] 原版 datah_211 未生成对应 UI_D211
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_211.bin 但无 UI_D211
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0137 [P2] 原版 datah_212 未生成对应 UI_D212
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_212.bin 但无 UI_D212
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0138 [P2] 原版 datah_213 未生成对应 UI_D213
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_213.bin 但无 UI_D213
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0139 [P2] 原版 datah_214 未生成对应 UI_D214
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_214.bin 但无 UI_D214
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0140 [P2] 原版 datah_215 未生成对应 UI_D215
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_215.bin 但无 UI_D215
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0141 [P2] 原版 datah_216 未生成对应 UI_D216
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_216.bin 但无 UI_D216
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0142 [P2] 原版 datah_217 未生成对应 UI_D217
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_217.bin 但无 UI_D217
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0143 [P2] 原版 datah_218 未生成对应 UI_D218
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_218.bin 但无 UI_D218
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0144 [P2] 原版 datah_219 未生成对应 UI_D219
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_219.bin 但无 UI_D219
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0145 [P2] 原版 datah_220 未生成对应 UI_D220
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_220.bin 但无 UI_D220
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0146 [P2] 原版 datah_221 未生成对应 UI_D221
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_221.bin 但无 UI_D221
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0147 [P2] 原版 datah_222 未生成对应 UI_D222
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_222.bin 但无 UI_D222
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0148 [P2] 原版 datah_223 未生成对应 UI_D223
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_223.bin 但无 UI_D223
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0149 [P2] 原版 datah_224 未生成对应 UI_D224
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_224.bin 但无 UI_D224
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0150 [P2] 原版 datah_225 未生成对应 UI_D225
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_225.bin 但无 UI_D225
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0151 [P2] 原版 datah_226 未生成对应 UI_D226
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_226.bin 但无 UI_D226
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0152 [P2] 原版 datah_227 未生成对应 UI_D227
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_227.bin 但无 UI_D227
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0153 [P2] 原版 datah_228 未生成对应 UI_D228
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_228.bin 但无 UI_D228
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0154 [P2] 原版 datah_229 未生成对应 UI_D229
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_229.bin 但无 UI_D229
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0155 [P2] 原版 datah_230 未生成对应 UI_D230
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_230.bin 但无 UI_D230
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0156 [P2] 原版 datah_231 未生成对应 UI_D231
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_231.bin 但无 UI_D231
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0157 [P2] 原版 datah_232 未生成对应 UI_D232
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_232.bin 但无 UI_D232
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0158 [P2] 原版 datah_233 未生成对应 UI_D233
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_233.bin 但无 UI_D233
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0159 [P2] 原版 datah_234 未生成对应 UI_D234
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_234.bin 但无 UI_D234
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0160 [P2] 原版 datah_235 未生成对应 UI_D235
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_235.bin 但无 UI_D235
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0161 [P2] 原版 datah_236 未生成对应 UI_D236
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_236.bin 但无 UI_D236
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0162 [P2] 原版 datah_237 未生成对应 UI_D237
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_237.bin 但无 UI_D237
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0163 [P2] 原版 datah_238 未生成对应 UI_D238
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_238.bin 但无 UI_D238
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0164 [P2] 原版 datah_239 未生成对应 UI_D239
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_239.bin 但无 UI_D239
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0165 [P2] 原版 datah_240 未生成对应 UI_D240
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_240.bin 但无 UI_D240
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0166 [P2] 原版 datah_241 未生成对应 UI_D241
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_241.bin 但无 UI_D241
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0167 [P2] 原版 datah_242 未生成对应 UI_D242
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_242.bin 但无 UI_D242
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0168 [P2] 原版 datah_243 未生成对应 UI_D243
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_243.bin 但无 UI_D243
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0169 [P2] 原版 datah_244 未生成对应 UI_D244
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_244.bin 但无 UI_D244
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0170 [P2] 原版 datah_245 未生成对应 UI_D245
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_245.bin 但无 UI_D245
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0171 [P2] 原版 datah_252 未生成对应 UI_D252
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_252.bin 但无 UI_D252
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0172 [P2] 原版 datah_254 未生成对应 UI_D254
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_254.bin 但无 UI_D254
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0173 [P2] 原版 datah_265 未生成对应 UI_D265
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_265.bin 但无 UI_D265
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0174 [P2] 原版 datah_266 未生成对应 UI_D266
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_266.bin 但无 UI_D266
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0175 [P2] 原版 datah_267 未生成对应 UI_D267
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_267.bin 但无 UI_D267
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0176 [P2] 原版 datah_268 未生成对应 UI_D268
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_268.bin 但无 UI_D268
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0177 [P2] 原版 datah_269 未生成对应 UI_D269
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_269.bin 但无 UI_D269
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0178 [P2] 原版 datah_270 未生成对应 UI_D270
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_270.bin 但无 UI_D270
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0179 [P2] 原版 datah_271 未生成对应 UI_D271
- 探针：`coverage`
- 位置：rev64/forward/src_rs/ui_menu.h
- 问题：spak_out 有 datah_271.bin 但无 UI_D271
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0180 [P2] UI_D019 按钮组终点超出面板
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.c
- 问题：按钮最大 (570,590) > 面板右下 (585,580)
- 依据：同上
- 修法：核对居中偏移或按钮坐标

## AUD-A-0181 [P2] UI_D079 按钮组终点超出面板
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.c
- 问题：按钮最大 (450,960) > 面板右下 (360,705)
- 依据：同上
- 修法：核对居中偏移或按钮坐标

## AUD-A-0182 [P2] UI_D083 按钮组终点超出面板
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.c
- 问题：按钮最大 (570,724) > 面板右下 (585,717)
- 依据：同上
- 修法：核对居中偏移或按钮坐标

## AUD-A-0183 [P2] UI_D099 按钮组终点超出面板
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.c
- 问题：按钮最大 (570,724) > 面板右下 (585,717)
- 依据：同上
- 修法：核对居中偏移或按钮坐标

## AUD-A-0184 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/audio.c:51
- 问题：调用 memset：memset(au, 0, sizeof(*au));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0185 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/audio.c:85
- 问题：调用 memset：memset(au, 0, sizeof(*au));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0186 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/audio.c:110
- 问题：调用 memcpy：memcpy(tag, p, 4); tag[4] = 0;
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0187 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/audio.c:152
- 问题：调用 memcpy：memcpy(pcm, buf + data_pos + 8, data_len);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0188 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/bt_protocol.c:75
- 问题：调用 memcpy：+ `memcpy(buf + n*cap, fb, len+1)` 在 cap_per_frame < 实际帧长时越界写
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0189 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/bt_protocol.c:88
- 问题：调用 memcpy：memcpy(buf + n * cap_per_frame, fb, len + 1);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0190 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/bt_selftest.c:157
- 问题：调用 memcpy：memcpy(wire + wl, wire, fn); wl += fn;
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0191 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/bt_selftest.c:172
- 问题：调用 memcpy：memcpy(frames[nfr], vs, flen);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0192 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/bt_selftest.c:214
- 问题：调用 memcpy：memcpy(multi + ml, fr, flen);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0193 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/bt_selftest.c:231
- 问题：调用 memcpy：memcpy(fr, vs, flen); fr[flen] = 0;
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0194 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/bt_selftest.c:265
- 问题：调用 memcpy：memcpy(wire1, fr, fn); wl1 = fn;
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0195 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/bt_selftest.c:267
- 问题：调用 memcpy：memcpy(wire1, fr, fn); wl1 = fn; wire1[wl1++] = '\n';
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0196 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/bt_selftest.c:279
- 问题：调用 memcpy：memcpy(rfr, vs, rl); rfr[rl] = 0;
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0197 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/bt_selftest.c:290
- 问题：调用 memcpy：memcpy(bare, fr, bl); bare[bl] = 0;
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0198 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/bt_selftest.c:294
- 问题：调用 memcpy：memcpy(wire2, bare, bl); wl2 = bl;
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0199 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/bt_selftest.c:296
- 问题：调用 memcpy：memcpy(wire2, bare, bl); wl2 = bl; wire2[wl2++] = '\n';
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0200 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/bt_selftest.c:307
- 问题：调用 memcpy：memcpy(rfr, vs, rl); rfr[rl] = 0;
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0201 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/bt_session.c:33
- 问题：调用 memset：memset(s, 0, sizeof(*s));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0202 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/containers.c:27
- 问题：调用 memset：memset(cc->count_tbl, 0, sizeof(cc->count_tbl));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0203 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/containers.c:53
- 问题：调用 memset：memset(cc, 0, sizeof(*cc));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0204 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/evverify.c:41
- 问题：调用 memset：memset(st, 0, sizeof(*st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0205 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/evverify.c:52
- 问题：调用 memset：memset(st, 0, sizeof(*st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0206 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/evverify.c:356
- 问题：调用 memset：memset(&st, 0, sizeof(st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0207 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/evverify.c:371
- 问题：调用 memset：memset(&st, 0, sizeof(st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0208 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/evverify.c:387
- 问题：调用 memset：memset(&st, 0, sizeof(st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0209 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/evverify.c:405
- 问题：调用 memset：memset(&st, 0, sizeof(st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0210 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/evverify.c:420
- 问题：调用 memset：memset(&st, 0, sizeof(st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0211 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/evverify.c:437
- 问题：调用 memset：memset(&st, 0, sizeof(st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0212 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/glview_gl.c:87
- 问题：调用 memset：memset(gv, 0, sizeof(*gv));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0213 [P2] strncpy 使用需复核（可能不补 \0（需核对））
- 探针：`unsafe-func`
- 位置：src_rs/jni_bridge.c:865
- 问题：调用 strncpy：if (chk) { fclose(chk); g_sfx_probe_ok = 1; strncpy(g_sfx_probe_path, pth, sizeof g_sfx_probe_path - 1); g_sfx_probe_path[sizeof g_sfx_probe_path - 1] = 0; sfx_
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0214 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/jni_bridge.c:1017
- 问题：调用 memset：memset(st, 0, sizeof(*st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0215 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/jni_bridge.c:1352
- 问题：调用 memset：memset(&g_state, 0, sizeof(g_state));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0216 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/jni_bridge.c:1420
- 问题：调用 memset：memset(&g_state, 0, sizeof(g_state));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0217 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/jni_bridge.c:2222
- 问题：调用 memset：memset(&resumed, 0, sizeof(resumed));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0218 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/jni_bridge.c:2297
- 问题：调用 memset：memset(&sa, 0, sizeof(sa));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0219 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/jni_bridge.c:8419
- 问题：调用 memcpy：memcpy(payload + plen, fr, flen);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0220 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/layout.c:40
- 问题：调用 memset：memset(cnt, 0, sizeof(int[RANK_COUNT]));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0221 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/layout.c:47
- 问题：调用 memset：memset(st, 0, sizeof(*st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0222 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/layout.c:147
- 问题：调用 memset：memset(seen, 0, sizeof(seen));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0223 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/main.c:152
- 问题：调用 memcpy：if (idx == 0) memcpy(lay, JQ_DEFAULT_LAY, 30);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0224 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/main.c:160
- 问题：调用 memcpy：memcpy(lay, JQ_DEFAULT_LAY, 30);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0225 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/main.c:164
- 问题：调用 memcpy：memcpy(lay, JQ_DEFAULT_LAY, 30);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0226 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/main.c:170
- 问题：调用 memcpy：memcpy(lay, JQ_DEFAULT_LAY, 30);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0227 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/main.c:173
- 问题：调用 memcpy：memcpy(lay, JQ_DEFAULT_LAY, 30);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0228 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/main.c:205
- 问题：调用 memcpy：memcpy(l2, JQ_DEFAULT_LAY, 30);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0229 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/png_decode.c:93
- 问题：调用 memcpy：memcpy(idat + idat_len, cdata, clen);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0230 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/png_decode.c:131
- 问题：调用 memset：memset(&zs, 0, sizeof(zs));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0231 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/render.c:423
- 问题：调用 memset：memset(rr, 0, sizeof(*rr));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0232 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/render_obj.c:11
- 问题：调用 memset：memset(l, 0, sizeof(*l));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0233 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/render_obj.c:43
- 问题：调用 memset：memset(v, 0, sizeof(*v));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0234 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/render_scene.c:82
- 问题：调用 memset：memset(sc, 0, sizeof(*sc));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0235 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/render_scene.c:107
- 问题：调用 memset：memset(sc, 0, sizeof(*sc));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0236 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/replay.c:18
- 问题：调用 memset：memset(rp, 0, sizeof(*rp));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0237 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:106
- 问题：调用 memset：memset(visited, 0, sizeof(visited));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0238 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:557
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0239 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:572
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0240 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:573
- 问题：调用 memset：memset(dst, 0, sizeof(*dst));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0241 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:612
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0242 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:659
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0243 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:693
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0244 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:701
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0245 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:711
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0246 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:721
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0247 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:722
- 问题：调用 memset：memset(dst, 0, sizeof(*dst));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0248 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rulestest.c:162
- 问题：调用 memset：memset(&st, 0, sizeof(st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0249 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rulestest.c:175
- 问题：调用 memset：memset(&st, 0, sizeof(st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0250 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rulestest.c:234
- 问题：调用 memset：memset(&st, 0, sizeof(st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0251 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/save.c:52
- 问题：调用 memset：memset(sd, 0, sizeof(*sd));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0252 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/touch_state.c:27
- 问题：调用 memset：memset(ts, 0, sizeof(*ts));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0253 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/ui_state.c:459
- 问题：调用 memcpy：memcpy(g_lay_names[g_lay_count++], names_local[i], UI_LAY_NAME);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0254 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/ui_state.c:465
- 问题：调用 memcpy：memcpy(tmp, g_lay_names[i], UI_LAY_NAME);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0255 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/ui_state.c:466
- 问题：调用 memcpy：memcpy(g_lay_names[i], g_lay_names[j], UI_LAY_NAME);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0256 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/ui_state.c:467
- 问题：调用 memcpy：memcpy(g_lay_names[j], tmp, UI_LAY_NAME);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0257 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/ui_state.c:481
- 问题：调用 memcpy：memcpy(nm, g_lay_names[i], UI_LAY_NAME);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-0258 [P3] 阶段 0 交付物存在
- 探针：`artifact`
- 位置：dist/phase0.apk
- 问题：大小 8205847 字节
- 依据：需校验 STORE/签名/ABI
- 修法：跑 build_phases.sh --verify-only

## AUD-A-0259 [P3] 阶段 1 交付物存在
- 探针：`artifact`
- 位置：dist/phase1.apk
- 问题：大小 8214110 字节
- 依据：需校验 STORE/签名/ABI
- 修法：跑 build_phases.sh --verify-only

## AUD-A-0260 [P3] 阶段 2 交付物存在
- 探针：`artifact`
- 位置：dist/phase2.apk
- 问题：大小 12245126 字节
- 依据：需校验 STORE/签名/ABI
- 修法：跑 build_phases.sh --verify-only

## AUD-A-0261 [P3] 阶段 3 交付物存在
- 探针：`artifact`
- 位置：dist/phase3.apk
- 问题：大小 10635325 字节
- 依据：需校验 STORE/签名/ABI
- 修法：跑 build_phases.sh --verify-only

## AUD-A-0262 [P3] UI_D006 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0263 [P3] UI_D007 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0264 [P3] UI_D010 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0265 [P3] UI_D012 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0266 [P3] UI_D016 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0267 [P3] UI_D017 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0268 [P3] UI_D020 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0269 [P3] UI_D033 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0270 [P3] UI_D034 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0271 [P3] UI_D035 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0272 [P3] UI_D038 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0273 [P3] UI_D041 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0274 [P3] UI_D042 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0275 [P3] UI_D043 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0276 [P3] UI_D044 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0277 [P3] UI_D047 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0278 [P3] UI_D055 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=-0.50, Y0 Δ=+0.00, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0279 [P3] UI_D061 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0280 [P3] UI_D062 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0281 [P3] UI_D063 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0282 [P3] UI_D065 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0283 [P3] UI_D067 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0284 [P3] UI_D068 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0285 [P3] UI_D069 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0286 [P3] UI_D070 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0287 [P3] UI_D071 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0288 [P3] UI_D072 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0289 [P3] UI_D077 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0290 [P3] UI_D083 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0291 [P3] UI_D084 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0292 [P3] UI_D086 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0293 [P3] UI_D096 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0294 [P3] UI_D099 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0295 [P3] UI_D102 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=-0.50, Y0 Δ=+0.00, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0296 [P3] UI_D112 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0297 [P3] UI_D119 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0298 [P3] UI_D121 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0299 [P3] UI_D123 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-0300 [P3] 窄类型乘法可能溢出
- 探针：`int-overflow`
- 位置：src_rs/bt_semtest.c:241
- 问题：窄类型赋值含乘法：int len = snprintf(frame, sizeof frame, "%.*s", 63, sb + i * 64);
- 依据：未见显式溢出检查
- 修法：中间用 int64_t 或加范围断言

## AUD-A-0301 [P3] 窄类型乘法可能溢出
- 探针：`int-overflow`
- 位置：src_rs/evverify.c:2513
- 问题：窄类型赋值含乘法：int i = r * 5 + c;
- 依据：未见显式溢出检查
- 修法：中间用 int64_t 或加范围断言

## AUD-A-0302 [P3] 窄类型乘法可能溢出
- 探针：`int-overflow`
- 位置：src_rs/png_decode.c:17
- 问题：窄类型赋值含乘法：int stride = w * bpp;
- 依据：未见显式溢出检查
- 修法：中间用 int64_t 或加范围断言

## AUD-A-0303 [P3] 窄类型乘法可能溢出
- 探针：`int-overflow`
- 位置：src_rs/png_decode.c:123
- 问题：窄类型赋值含乘法：int stride = (int)w * channels;
- 依据：未见显式溢出检查
- 修法：中间用 int64_t 或加范围断言

## AUD-A-0304 [P3] 窄类型乘法可能溢出
- 探针：`int-overflow`
- 位置：src_rs/render.c:2698
- 问题：窄类型赋值含乘法：int filled = have && lay[r * 5 + c] != 0;
- 依据：未见显式溢出检查
- 修法：中间用 int64_t 或加范围断言

## AUD-A-0305 [P3] 窄类型乘法可能溢出
- 探针：`int-overflow`
- 位置：src_rs/render.c:9894
- 问题：窄类型赋值含乘法：int sig = imgset_trans * 8 + imgset_piece * 4 + imgset_night * 2 + imgset_opp;
- 依据：未见显式溢出检查
- 修法：中间用 int64_t 或加范围断言

## AUD-A-0306 [P3] 一致性基准未覆盖 UI_MENU 路径
- 探针：`test-gap`
- 位置：src_rs/consistency_desktop.c
- 问题：未提及 UI_MENU
- 依据：基准序列覆盖不足
- 修法：补对应场景

## AUD-A-0307 [P3] 一致性基准未覆盖 UI_GAME 路径
- 探针：`test-gap`
- 位置：src_rs/consistency_desktop.c
- 问题：未提及 UI_GAME
- 依据：基准序列覆盖不足
- 修法：补对应场景

## AUD-A-0308 [P3] 一致性基准未覆盖 replay 路径
- 探针：`test-gap`
- 位置：src_rs/consistency_desktop.c
- 问题：未提及 replay
- 依据：基准序列覆盖不足
- 修法：补对应场景

## AUD-A-0309 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/bt_session.c:171
- 问题：/* 翻棋帧：type=FLIP, f2=col, f3=row，其余占位 0 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0310 [P3] 未完成标记（XXX）
- 探针：`todo`
- 位置：src_rs/consistency_desktop.c:54
- 问题：char tmpl[] = "/tmp/jq_bench_XXXXXX";
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0311 [P3] 未完成标记（XXX）
- 探针：`todo`
- 位置：src_rs/evverify.c:31
- 问题：#include <stdlib.h>   /* abs()（SPAK2UI 批量 run_uiXXX 几何 Δ 判定） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0312 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:347
- 问题：诚实边界：先手翻子定色（side=UNKNOWN）留待后续轮次；ai_pick 暗棋信息遮蔽未实现。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0313 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:581
- 问题：(void)is_flip;   /* AUD-A-0001 探针降噪：is_flip 全 0（桌面走子序列无翻棋步），仅占位容量守卫 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0314 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:899
- 问题：+ 几何 Δ=0 + 业务分支（桌号占位/确定边界对话框/取消，确定性不依赖触摸/GL）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0315 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:938
- 问题：/* 桌号（action=0）→ 占位标签（触摸 flaky 无输入框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0316 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:943
- 问题：printf("join table: hit=%d action=%d → 桌号占位标签（★ 诚实边界：不画真输入框）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0317 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:965
- 问题：+ 几何 Δ=0 + 业务分支（密码占位/确定边界对话框/取消，确定性不依赖触摸/GL）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0318 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1004
- 问题：/* 密码（action=0）→ 占位标签（触摸 flaky 无输入框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0319 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1009
- 问题：printf("create pwd: hit=%d action=%d → 密码占位标签（★ 诚实边界：不画真输入框）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0320 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1275
- 问题：★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0321 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1311
- 问题：/* 清除（action=0）→ 清本地占位记录（诚实边界：联机聊天文本未逆向） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0322 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1316
- 问题：printf("chatlog clear: hit=%d action=%d → 清本地占位记录（★ 诚实边界：联机聊天文本未逆向，仅蓝牙占位帧）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0323 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1333
- 问题：★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0324 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1370
- 问题：/* 修改（action=0）→ 诚实边界对话框（修改资料需账号系统，未实现） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0325 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1375
- 问题：printf("profile modify: hit=%d action=%d → 诚实边界对话框（修改资料需账号系统，未实现）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0326 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1391
- 问题：printf("profile: 3 按钮几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：账号系统未逆向，Info=占位）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0327 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1401
- 问题：★ 诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0328 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1443
- 问题：/* 提和（action=0）→ BT_T_DRAW 帧 + 诚实边界对话框（本地提和判定未实现） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0329 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1448
- 问题：printf("drawdetail draw: hit=%d action=%d → BT_T_DRAW 帧 + 诚实边界对话框（★ 本地提和判定未实现）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0330 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1457
- 问题：/* 聊天（action=2）→ BT_T_CHAT 占位帧 + 边界对话框（联机聊天文本未逆向） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0331 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1462
- 问题：printf("drawdetail chat: hit=%d action=%d → BT_T_CHAT 占位帧（★ 联机聊天文本未逆向）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0332 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1499
- 问题：printf("drawdetail: 7 图标几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：本地提和判定未实现，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0333 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1510
- 问题：★ 诚实边界：图像设置渲染参数未实现（src_rs 无对应渲染参数），仅 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0334 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1547
- 问题：/* 图像（action=0）→ UI_IMGSET7（图像设置简化版 datah_008，★ 诚实边界：渲染参数未实现） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0335 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1556
- 问题：printf("setentry img: hit=%d action=%d → UI_IMGSET7 图像设置弹窗（datah_008，★ 诚实边界：渲染参数未实现）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0336 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1573
- 问题：printf("setentry: 3 按钮几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：图像设置渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0337 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1584
- 问题：图像设置渲染参数未实现（src_rs 无对应渲染参数），仅 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0338 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1620
- 问题：/* 图像（action=0）→ UI_IMGSET8（图像设置简化版 datah_008，★ 诚实边界：渲染参数未实现） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0339 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1629
- 问题：printf("setentry4 img: hit=%d action=%d → UI_IMGSET8 图像设置（简化版 datah_008，★ 诚实边界：渲染参数未实现）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0340 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1639
- 问题：printf("setentry4: 2 按钮几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：图像设置渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0341 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1649
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0342 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1701
- 问题：printf("imgset7 trans: hit=%d action=%d → 开/关 循环 ui_imgset_trans_get()=%d（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0343 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1709
- 问题：printf("imgset7 piece: hit=%d action=%d → 开/关 循环（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0344 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1716
- 问题：printf("imgset7 night: hit=%d action=%d → 开/关 循环（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0345 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1723
- 问题：printf("imgset7 opp: hit=%d action=%d → 开/关 循环（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0346 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1745
- 问题：printf("imgset7: 6 控件几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：图像/夜间/对方棋子渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0347 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1755
- 问题：★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0348 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1802
- 问题：printf("imgset8 trans: hit=%d action=%d → 开/关 循环（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0349 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1810
- 问题：printf("imgset8 piece: hit=%d action=%d → 开/关 循环（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0350 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1817
- 问题：printf("imgset8 night: hit=%d action=%d → 开/关 循环（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0351 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1839
- 问题：printf("imgset8: 5 控件几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：图像/夜间渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0352 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1845
- 问题：命中判定 + 几何 Δ=0 + 业务分支（8 显示占位标签 + 开始→边界对话框/取消→回主菜单，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0353 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1850
- 问题：显示占位标签"蓝方/红方/布局/暗棋/开/开/开/开"，不画真数据）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0354 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1859
- 问题：{275.0f, 473.0f, 555.0f, 508.0f},   /* 0=先手 Player1（140,85,420,120）→ 占位标签"蓝方" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0355 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1860
- 问题：{275.0f, 513.0f, 555.0f, 548.0f},   /* 1=后手 Player2（140,125,420,160）→ 占位标签"红方" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0356 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1861
- 问题：{275.0f, 553.0f, 555.0f, 588.0f},   /* 2=玩法 Lay（140,165,420,200）→ 占位标签"布局" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0357 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1862
- 问题：{275.0f, 593.0f, 555.0f, 628.0f},   /* 3=明暗棋 Show（140,205,420,240）→ 占位标签"暗棋" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0358 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1863
- 问题：{275.0f, 633.0f, 555.0f, 668.0f},   /* 4=和棋 Draw（140,245,420,280）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0359 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1864
- 问题：{275.0f, 673.0f, 555.0f, 708.0f},   /* 5=大本营 CmdSafe（140,285,420,320）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0360 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1865
- 问题：{275.0f, 713.0f, 555.0f, 748.0f},   /* 6=吃地雷 BingLei（140,325,420,360）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0361 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1866
- 问题：{275.0f, 753.0f, 555.0f, 788.0f},   /* 7=吃军旗 BingQi（140,365,420,400）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0362 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1916
- 问题：printf("join065: 10 控件几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：联机开局配置=占位标签，开始→边界对话框）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0363 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2113
- 问题：开始/换桌→边界对话框；2 玩家显示区=占位标签（不画真数据） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0364 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2177
- 问题：printf("room060: 3 按钮几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：联机房间=边界对话框，2 玩家显示区=占位标签）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0365 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2183
- 问题：命中判定 + 几何 Δ=0 + 业务分支（发送→BT_T_CHAT 占位帧+边界对话框/记录→UI_CHATLOG/
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0366 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2187
- 问题：★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT，无真实文本渲染），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0367 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2188
- 问题：Msg 输入框简化为占位标签 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0368 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2195
- 问题：{175.0f, 723.0f, 285.0f, 783.0f},   /* 0=发送 Send（25,245,135,305）→ BT_T_CHAT 占位帧 + 边界对话框 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0369 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2225
- 问题：/* 发送（action=0）→ BT_T_CHAT 占位帧 + 边界对话框（联机聊天文本未逆向） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0370 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2230
- 问题：printf("chat063 send: hit=%d action=%d → BT_T_CHAT 占位帧 + 边界对话框（★ 诚实边界：联机聊天文本未逆向）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0371 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2256
- 问题：printf("chat063: 3 按钮几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：联机聊天文本未逆向，发送=占位帧+边界对话框）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0372 [P3] 未完成标记（XXX）
- 探针：`todo`
- 位置：src_rs/evverify.c:2533
- 问题：（阻断 src_rs_regress 12/12）；几何项（run_dXXX 120+ 弹窗几何）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0373 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2598
- 问题：（★ 诚实边界：账号系统未逆向，Info=占位，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0374 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:2601
- 问题：（★ 诚实边界：本地提和判定未实现，提和/认输走 BT 帧 + 边界对话框，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0375 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:2604
- 问题：（★ 诚实边界：图像设置渲染参数未实现，仅 UI 状态记录，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0376 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:2607
- 问题：（★ 诚实边界：图像设置渲染参数未实现，仅 UI 状态记录，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0377 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:2610
- 问题：（★ 诚实边界：图像/夜间/对方棋子渲染参数未实现，仅 UI 状态记录，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0378 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:2613
- 问题：（★ 诚实边界：图像/夜间渲染参数未实现，仅 UI 状态记录，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0379 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2616
- 问题：（★ 诚实边界：联机开局配置=占位标签，开始→边界对话框，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0380 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2625
- 问题：（★ 诚实边界：联机房间=边界对话框，2 玩家显示区=占位标签，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0381 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2628
- 问题：（★ 诚实边界：联机聊天文本未逆向，发送=占位帧+边界对话框，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0382 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2634
- 问题：几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：未逆向数据=占位标签+边界对话框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0383 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2873
- 问题：占位标签 + 边界对话框，桌面验证只证几何+命中+状态切换，不证数据真实现 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0384 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2984
- 问题：printf("ui46 close: hit=%d action=%d → 回对局（★ 诚实边界：玩家资料内容未逆向，占位标签）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0385 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:3027
- 问题：printf("ui47 close: hit=%d action=%d → 回对局（★ 诚实边界：玩家列表未逆向，占位）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0386 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:3070
- 问题：printf("ui49 close: hit=%d action=%d → 回对局（★ 诚实边界：对局详情内容未逆向，占位）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0387 [P3] 未完成标记（XXX）
- 探针：`todo`
- 位置：src_rs/evverify.c:3467
- 问题：run_dXXX 批量 120+ 弹窗纯几何断言（Δ=0 + 中心命中），无业务分支
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0388 [P3] 未完成标记（XXX）
- 探针：`todo`
- 位置：src_rs/evverify.c:3577
- 问题：/* ===== ★ AUD-1000 B08-025（P1 双源几何）：run_dXXX 期望坐标单源化 =====
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0389 [P3] 未完成标记（XXX）
- 探针：`todo`
- 位置：src_rs/evverify.c:3578
- 问题：旧版 run_dXXX 每个 exp[] 期望坐标**硬编码**（如 run_d001 exp 355,520-555,580），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0390 [P3] 未完成标记（XXX）
- 探针：`todo`
- 位置：src_rs/evverify.c:3579
- 问题：与 ui_menu.c g_dXXX_btns 真机表单源双写——ui_menu.c 改弹窗几何后 evverify
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0391 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1536
- 问题：OnCreate 弹 UI_JOIN 加入游戏弹窗：桌号占位标签 + 确定/取消）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0392 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1545
- 问题：LOGI("jq.join 钩子：→ UI_JOIN 加入游戏弹窗（datah_034 桌号占位标签 + 确定/取消，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0393 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1554
- 问题：OnCreate 弹 UI_CREATE 创建游戏弹窗：密码占位标签 + 确定/取消）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0394 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1563
- 问题：LOGI("jq.create 钩子：→ UI_CREATE 创建游戏弹窗（datah_035 密码占位标签 + 确定/取消，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0395 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1612
- 问题：OnCreate 弹 UI_CHATLOG 聊天记录弹窗：Msg 文本框（诚实边界占位） + 清除/关闭 2 按钮）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0396 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1614
- 问题：★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0397 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1620
- 问题：LOGI("jq.chatlog 钩子：→ UI_CHATLOG 聊天记录弹窗（datah_029 Msg 文本框（诚实边界占位） + "
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0398 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1626
- 问题：OnCreate 弹 UI_PROFILE 个人资料弹窗：Info 文本框（诚实边界占位） + 修改/帮助/返回 3 按钮）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0399 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1629
- 问题：★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0400 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1635
- 问题：LOGI("jq.profile 钩子：→ UI_PROFILE 个人资料弹窗（datah_031 Info 文本框（诚实边界占位） + "
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0401 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1643
- 问题：一次性（清 0）。★ 诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0402 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1651
- 问题："诚实边界：本地提和判定未实现，提和/认输走 BT 帧 + 边界对话框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0403 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1658
- 问题：一次性（清 0）。★ 诚实边界：本地提和判定未实现（原版需对端应答），联机提和/认输走
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0404 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1666
- 问题："提和/认输/聊天/系统设置/玩家资料/战况统计/软件信息，诚实边界：本地提和判定未实现，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0405 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1674
- 问题：占位标签 + 边界对话框，不包装成"已实现" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0406 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1701
- 问题："1 按钮，★ 诚实边界：玩家资料内容未逆向，占位标签）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0407 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1711
- 问题："1 按钮，★ 诚实边界：玩家列表未逆向，占位）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0408 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1721
- 问题："1 按钮，★ 诚实边界：对局详情内容未逆向，占位）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0409 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1798
- 问题：★ 诚实边界（已部分解除）：夜间模式/对方棋子接渲染层（暗色遮罩/红方隐藏），仅界面/棋子动画过渡帧未实现，入口指向 UI_IMGSET7；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0410 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1807
- 问题："★ 诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅动画过渡帧未实现）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0411 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1814
- 问题：★ 诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅动画过渡帧未实现，入口指向 UI_IMGSET8；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0412 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1822
- 问题："图像→UI_IMGSET8；声音→UI_SETTINGS（datah_001），★ 诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅动画过渡帧未实现）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0413 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1829
- 问题：★ 诚实边界（已部分解除）：夜间模式接渲染层暗色遮罩 / 对方棋子接渲染层隐藏红方 / 棋子动画关=不画，仅界面动画过渡帧未实现；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0414 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1837
- 问题："★ 诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅界面动画过渡帧未实现；"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0415 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1846
- 问题：★ 诚实边界：图像/夜间渲染参数未实现，开关仅 UI 状态记录；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0416 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1854
- 问题："★ 诚实边界：图像/夜间渲染参数未实现，仅 UI 状态记录；"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0417 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1863
- 问题：★ 诚实边界：联机开局配置=占位标签，开始→边界对话框（src_rs 仅蓝牙已实现）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0418 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1870
- 问题："4 规则显示 + 开始/取消，★ 诚实边界：联机开局配置=占位标签，开始→边界对话框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0419 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1905
- 问题：OnCreate 弹 UI_ROOM060 房间态弹窗：2 玩家房间列表（占位标签）+ 开始/换桌/离开。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0420 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1907
- 问题：★ 诚实边界：联机房间=边界对话框（src_rs 仅蓝牙已实现），2 玩家显示区=占位标签。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0421 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1913
- 问题：LOGI("jq.room060 钩子：→ UI_ROOM060 房间态弹窗（datah_060 2 玩家房间列表（占位标签）+ 开始/换桌/离开，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0422 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1914
- 问题："★ 诚实边界：联机房间=边界对话框，2 玩家显示区=占位标签）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0423 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1919
- 问题：OnCreate 弹 UI_CHAT063 聊天完整版弹窗：Msg 输入框（占位标签）+ 发送/记录/关闭。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0424 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1921
- 问题：★ 诚实边界：联机聊天文本未逆向，发送=BT_T_CHAT 占位帧+边界对话框；记录→UI_CHATLOG（datah_029）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0425 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1927
- 问题：LOGI("jq.chat063 钩子：→ UI_CHAT063 聊天完整版弹窗（datah_063 Msg 输入框（占位标签）+ 发送/记录/关闭，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0426 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1928
- 问题："★ 诚实边界：联机聊天文本未逆向，发送=占位帧+边界对话框；记录→UI_CHATLOG（datah_029））");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0427 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2277
- 问题：未实现而在 GLThread 上崩 → 信号 handler 设标志位，OnUpdate 检测到后跳过渲染
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0428 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2480
- 问题：LOGI("OnUpdate 加入游戏入口：join=1 → UI_JOIN 加入游戏弹窗（datah_034 桌号占位标签 + 确定/取消，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0429 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2497
- 问题：LOGI("OnUpdate 创建游戏入口：create=1 → UI_CREATE 创建游戏弹窗（datah_035 密码占位标签 + 确定/取消，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0430 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2559
- 问题：★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0431 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2565
- 问题：LOGI("OnUpdate 聊天记录入口：chatlog=1 → UI_CHATLOG 聊天记录弹窗（datah_029 Msg 文本框（诚实边界占位） + "
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0432 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2577
- 问题：★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0433 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2583
- 问题：LOGI("OnUpdate 个人资料入口：profile=1 → UI_PROFILE 个人资料弹窗（datah_031 Info 文本框（诚实边界占位） + "
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0434 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2594
- 问题：★ 诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0435 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2602
- 问题："提和/认输/聊天/系统设置/玩家资料/战况统计/软件信息，诚实边界：本地提和判定未实现，提和/认输走 BT 帧 + 边界对话框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0436 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2614
- 问题：★ 诚实边界：图像设置渲染参数未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0437 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2623
- 问题："图像→UI_IMGSET7（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）；声音→UI_SETTINGS（datah_001）；"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0438 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2635
- 问题：★ 诚实边界：图像设置渲染参数未实现，图像设置入口指向 UI_IMGSET8（datah_008 简化版，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0439 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2643
- 问题："无游戏设置项，联机态/蓝牙态；图像→UI_IMGSET8（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）；"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0440 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2655
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0441 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2663
- 问题："诚实边界：图像/夜间/对方棋子渲染参数未实现，仅 UI 状态记录；"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0442 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2677
- 问题：★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0443 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2685
- 问题："诚实边界：图像/夜间渲染参数未实现，仅 UI 状态记录；"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0444 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2700
- 问题：显示占位标签"蓝方/红方/布局/暗棋/开/开/开/开"，不画真数据）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0445 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2709
- 问题："4 规则显示 + 开始/取消，诚实边界：联机开局配置=占位标签，开始→边界对话框（src_rs 仅蓝牙已实现））");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0446 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2763
- 问题：OnCreate 弹 UI_ROOM060 房间态弹窗：2 玩家房间列表（占位标签）+ 开始/换桌/离开。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0447 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2766
- 问题：开始/换桌→边界对话框；2 玩家显示区=占位标签（不画真数据，触摸 flaky 下房间列表无意义）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0448 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2772
- 问题：LOGI("jq.room060 钩子：→ UI_ROOM060 房间态弹窗（datah_060 2 玩家房间列表（占位标签）+ 开始/换桌/离开，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0449 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2773
- 问题："诚实边界：联机房间=边界对话框（src_rs 仅蓝牙已实现），2 玩家显示区=占位标签）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0450 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2782
- 问题：OnCreate 弹 UI_CHAT063 聊天完整版弹窗：Msg 输入框（占位标签）+ 发送/记录/关闭。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0451 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2784
- 问题：★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT，无真实文本渲染），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0452 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2785
- 问题：Msg 输入框简化为占位标签"聊天输入（诚实边界：联机聊天文本未逆向）"；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0453 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2786
- 问题：发送→BT_T_CHAT 占位帧+边界对话框；记录→UI_CHATLOG（datah_029 已实现）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0454 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2793
- 问题：LOGI("jq.chat063 钩子：→ UI_CHAT063 聊天完整版弹窗（datah_063 Msg 输入框（占位标签）+ 发送/记录/关闭，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0455 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2794
- 问题："诚实边界：联机聊天文本未逆向，发送=BT_T_CHAT 占位帧+边界对话框；记录→UI_CHATLOG（datah_029））");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0456 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2863
- 问题：范式与 jq.login 同；不重启 app）。★ 诚实边界：未逆向数据=占位标签+边界对话框。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0457 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2884
- 问题：LOGI("OnUpdate SPAK2UI 批量入口：%s=1 → UI_UI%d 弹窗（诚实边界：未逆向数据=占位）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0458 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:3100
- 问题：/* UI 分流：菜单态点按钮 → 进对局（人机/重开），其它按钮占位日志。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0459 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:3144
- 问题：显示边界对话框（4 方规则未实现，勿静默开 2 人局误导为四国可玩）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0460 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:3459
- 问题：} else if (a == 0) {   /* Draw（和棋/提和）→ 边界对话框（诚实边界：本地提和判定未实现，提和/认输走 BT 帧） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0461 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:3855
- 问题：} else if (a == 1) {   /* Chat（聊天）→ 业务动作（聊天文本未逆向，发送=BT_T_CHAT 占位帧） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0462 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:4124
- 问题：jq_show_dialog(env, "提和（诚实边界）", "提和：原版需对端应答，src_rs 本地未实现提和判定。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0463 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:4567
- 问题：} else if (a == 1) {   /* Chat（聊天）→ 业务动作（聊天文本未逆向，发送=BT_T_CHAT 占位帧） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0464 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:4638
- 问题：jq_show_dialog(env, "提和（诚实边界）", "提和：原版需对端应答，src_rs 本地未实现提和判定。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0465 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:4725
- 问题：} else if (a == 0) {   /* Draw（和棋/提和）→ 边界对话框（诚实边界：本地提和判定未实现，提和/认输走 BT 帧） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0466 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5091
- 问题：} else if (a == 2) {   /* Chat（聊天）→ 业务动作（聊天文本未逆向，发送=BT_T_CHAT 占位帧） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0467 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5131
- 问题：jq_show_dialog(env, "提和（诚实边界）", "提和：原版需对端应答，src_rs 本地未实现提和判定。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0468 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5704
- 问题：} else if (a == 1) {   /* Chat（聊天）→ 业务动作（聊天文本未逆向，发送=BT_T_CHAT 占位帧） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0469 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5758
- 问题：} else if (a == 1) {   /* Chat（聊天）→ 业务动作（聊天文本未逆向，发送=BT_T_CHAT 占位帧） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0470 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5904
- 问题：jq_show_dialog(env, "提和（诚实边界）", "提和：原版需对端应答，src_rs 本地未实现提和判定。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0471 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6050
- 问题：LOGI("OnTouch 方式选择：Wi-Fi 联机 → UI_JOIN 加入游戏弹窗（datah_034 桌号占位标签 + 确定/取消，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0472 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6128
- 问题：点 2/3/4 人 → 诚实边界对话框（4 方规则未实现）→ 回主菜单；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0473 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6140
- 问题："未实现。原版四国棋盘/规则需另项逆向移植。\n"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0474 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6204
- 问题：点 用户名/密码（占位标签，无输入框）→ 无动作（LOGI）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0475 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6209
- 问题：Junqi.* 登录/注册 20+ 事件未实现）；游客=本地单机唯一真生效路径。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0476 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6218
- 问题：if (lact == 0 || lact == 1) {   /* 用户名/密码 占位标签（触摸 flaky 无输入框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0477 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6219
- 问题：LOGI("OnTouch 登录弹窗：用户名/密码 占位标签（★ 诚实边界：输入框简化，不画真输入框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0478 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6224
- 问题："未实现，当前为离线单机模式。请选择游客进入。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0479 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6231
- 问题："未实现，当前为离线单机模式。请选择游客进入。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0480 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6314
- 问题：点 取消 / 面板外 → 回对局（不载入）。★ 诚实边界：ListBox 滚动未实现（6 条截断）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0481 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6449
- 问题：点 取消 / 面板外 → 回对局。30 格预览区 = 小方块占位（★ 诚实边界）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0482 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6675
- 问题：点 桌号（占位标签）→ 无动作（LOGI）；点 确定 → 诚实边界对话框 + 回主菜单；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0483 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6686
- 问题：if (jact == 0) {   /* 桌号占位标签（触摸 flaky 无输入框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0484 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6687
- 问题：LOGI("OnTouch 加入游戏弹窗：桌号占位标签（★ 诚实边界：输入框简化，不画真输入框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0485 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6712
- 问题：点 密码（占位标签）→ 无动作（LOGI）；点 确定 → 诚实边界对话框 + 回主菜单；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0486 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6723
- 问题：if (cact == 0) {   /* 密码占位标签（触摸 flaky 无输入框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0487 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6724
- 问题：LOGI("OnTouch 创建游戏弹窗：密码占位标签（★ 诚实边界：输入框简化，不画真输入框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0488 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6814
- 问题：Info 文本=☰ 战况统计复用（渲染层占位标签 + LOGI 实际值）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0489 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6902
- 问题：点 清除 → 清本地占位记录（诚实边界：联机聊天文本未逆向，仅蓝牙占位帧）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0490 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6904
- 问题：Msg 文本框=诚实边界占位（渲染层占位标签 + LOGI）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0491 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6913
- 问题：if (cact == 0) {   /* 清除：清本地占位记录（诚实边界） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0492 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6914
- 问题：static char chat_log_clear = 1;   /* 占位记录标志（无真实聊天文本存储） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0493 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6917
- 问题：LOGI("OnTouch 聊天记录弹窗：清除 → 清本地占位记录（★ 诚实边界：联机聊天文本未逆向，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0494 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6918
- 问题："src_rs 蓝牙聊天是占位帧 BT_T_CHAT，无真实文本存储，清除=清占位标志）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0495 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6936
- 问题：点 修改 → 诚实边界对话框（修改资料需账号系统，未实现）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0496 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6939
- 问题：Info 文本框=诚实边界占位（渲染层占位标签 + LOGI）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0497 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6949
- 问题：if (pact == 0) {   /* 修改 → 诚实边界对话框（修改资料需账号系统，未实现） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0498 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6962
- 问题："这些字段均为诚实边界占位，未实现账号功能。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0499 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6983
- 问题：提和 → jq_bt_ctrl_send(BT_T_DRAW) + 诚实边界对话框（本地提和判定未实现，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0500 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6991
- 问题：★ 诚实边界：本地提和判定未实现（src_rs 离线单机无对端），联机提和/认输走
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0501 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7001
- 问题：if (dact == 0) {   /* 提和 → 发 BT_T_DRAW 帧 + 诚实边界对话框（本地提和判定未实现） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0502 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7005
- 问题："本地提和判定未实现（原版需对端应答，\n"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0503 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7008
- 问题：LOGI("OnTouch 提和详情：提和 → 发送 DRAW 帧 + 诚实边界对话框（本地提和判定未实现）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0504 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7020
- 问题：} else if (dact == 2) {   /* 聊天 → 发 BT_T_CHAT 占位帧 + 边界对话框 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0505 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7025
- 问题："当前发送聊天占位帧（对端无文本渲染）。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0506 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7027
- 问题：LOGI("OnTouch 提和详情：聊天 → 发送 CHAT 占位帧（联机聊天文本未逆向）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0507 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7079
- 问题：★ 诚实边界：图像设置渲染参数未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0508 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7093
- 问题："★ 诚实边界：渲染参数未实现，仅 UI 状态记录）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0509 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7119
- 问题：★ 诚实边界：图像设置渲染参数未实现，开关只是 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0510 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7132
- 问题："★ 诚实边界：渲染参数未实现，仅 UI 状态记录）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0511 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7154
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0512 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7167
- 问题：LOGI("OnTouch 图像设置弹窗（完整版）：界面动画切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0513 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7172
- 问题：LOGI("OnTouch 图像设置弹窗（完整版）：棋子动画切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0514 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7177
- 问题：LOGI("OnTouch 图像设置弹窗（完整版）：夜间模式切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0515 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7182
- 问题：LOGI("OnTouch 图像设置弹窗（完整版）：对方棋子切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0516 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7214
- 问题：★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0517 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7227
- 问题：LOGI("OnTouch 图像设置弹窗（简化版）：界面动画切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0518 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7232
- 问题：LOGI("OnTouch 图像设置弹窗（简化版）：棋子动画切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0519 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7237
- 问题：LOGI("OnTouch 图像设置弹窗（简化版）：夜间模式切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0520 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7264
- 问题：8 显示项 = 占位标签（先手/后手/玩法/明暗棋/4 规则，非按钮，显示当前值占位
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0521 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7278
- 问题：if (jact >= 0 && jact <= 7) {   /* 8 显示项：占位标签（不实际处理，仅日志） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0522 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7279
- 问题：LOGI("OnTouch 加入游戏完整版弹窗：显示项%d（占位标签，不实际处理，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0523 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7429
- 问题：（非按钮，简化为占位标签"蓝方/红方"，触摸 flaky 下房间列表无意义）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0524 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7436
- 问题：2 玩家显示区 = 占位标签（不画真数据） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0525 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7476
- 问题：Msg 输入框简化为占位标签"聊天输入（诚实边界：联机聊天文本未逆向）"；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0526 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7477
- 问题：点 发送 → jq_bt_ctrl_send(BT_T_CHAT) 占位帧 + 诚实边界对话框
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0527 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7478
- 问题：（"聊天（诚实边界）：联机聊天文本未逆向，src_rs 蓝牙聊天是占位帧 BT_T_CHAT，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0528 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7482
- 问题：★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0529 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7491
- 问题：if (c63act == 0) {   /* 发送 → BT_T_CHAT 占位帧 + 边界对话框 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0530 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7496
- 问题："src_rs 蓝牙聊天是占位帧 BT_T_CHAT，\n"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0531 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7499
- 问题：LOGI("OnTouch 聊天完整版弹窗：发送 → BT_T_CHAT 占位帧 + 边界对话框（联机聊天文本未逆向）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0532 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7643
- 问题：点面板外 → 回对局。★ 诚实边界：未逆向数据=占位标签+边界对话框，不包装成"已实现" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0533 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7654
- 问题："src_rs 无排行榜数据通道，占位。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0534 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7678
- 问题："src_rs 无详细战绩数据通道，占位。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0535 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7860
- 问题："src_rs 无详细战况数据通道，占位。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0536 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7886
- 问题："src_rs 无更新通道，占位。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0537 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7994
- 问题："诚实边界：本地提和判定未实现，提和/认输走 BT 帧 + 边界对话框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0538 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:8010
- 问题：datah_063 聊天完整版弹窗（Msg 输入框占位标签 + 发送/记录/关闭 3 按钮），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0539 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:8011
- 问题：发帧逻辑保留在弹窗内（发送→BT_T_CHAT 占位帧 + 边界对话框）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0540 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:8014
- 问题：LOGI("☰菜单 聊天（联机版）：→ UI_CHAT063 聊天完整版弹窗（datah_063 Msg 输入框占位 + "
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0541 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:8015
- 问题："发送/记录/关闭 3 按钮，诚实边界：联机聊天文本未逆向，发送=BT_T_CHAT 占位帧+边界对话框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0542 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:1514
- 问题：/* 图集 160 宽里字形按字数自然占位（4 字占 ~150、2 字占 ~75，居中）：
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0543 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:2020
- 问题：按钮几何 ui_nplayer_buttons()（720×1280 基准）。★ 诚实边界：四国=未实现 4 方规则。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0544 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2641
- 问题：简化：行底色 + 序号纯色块占位（★ 诚实边界：ListBox 文本渲染未接动态名，占位块）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0545 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2661
- 问题：else glUniform4f(uc, 0.15f, 0.15f, 0.18f, 0.60f);        /* 无条目灰暗占位 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0546 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2675
- 问题：★ 诚实边界：30 格 PiecePic 真棋子图未画，小方块占位（原版 Piece0-29 为棋子图）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0547 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2715
- 问题：if (!pdiag) { pdiag = 1; LOGI("rs_layres_preview：30 格棋子预览（小方块占位，选中=%s）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0548 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:2721
- 问题：取消/面板外=回对局。★ 诚实边界：ListBox 滚动未实现，6 条截断 + 选中金色高亮。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0549 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2873
- 问题：删除=删选中项；取消=回对局。★ 诚实边界：PiecePic 真棋子图未画（小方块占位）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0550 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2882
- 问题：/* 30 格棋子预览（小方块 6×5 占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0551 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2946
- 问题：if (!mdiag) { mdiag = 1; LOGI("rs_draw_laymgr：布局管理弹窗（datah_016 30 格预览（小方块占位）+ 载入/删除/取消，面板 400×475）"); }
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0552 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2949
- 问题：/* ★ P1（datah_107 证据）：复盘回放渲染——棋盘（当前回放位置，棋子占位方块范式）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0553 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2954
- 问题：复盘态不画真实棋子图（复用棋盘现有占位方块范式，棋子图集缺失则退化纯色格）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0554 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2989
- 问题：/* 复盘态棋子占位方块（60 格有子 → 蓝/红小方块，居中格区，不复用 rs_draw_board
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0555 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3027
- 问题：+ 用户名/密码占位标签（EditLabel 不画真输入框）+ 登录/注册/游客/返回 4 按钮 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0556 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3030
- 问题：游客=本地单机（无账号，唯一真生效路径）；输入框简化为占位标签（触摸 flaky）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0557 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3127
- 问题：/* ④ 用户名/密码 占位 EditLabel（矩形浅色底 + 占位文字，★ 诚实边界：不画真输入框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0558 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3147
- 问题：/* 占位标签文字（row1=用户名 row2=密码，输入框内左对齐） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0559 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3203
- 问题：登录按钮文字用 row6"账户"占位——★ 诚实边界：登录/注册 未逆向服务器协议） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0560 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3234
- 问题：LOGI("rs_draw_login：登录弹窗（datah_038 用户名/密码占位标签 + 登录/注册/游客/返回 4 按钮，面板 %.0f×%.0f，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0561 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3365
- 问题：/* 按钮文字（row1=翻棋 row2=布局 row3=工兵 占位，按钮内居中；★ 诚实边界：
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0562 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3366
- 问题：规则文本简化版，row3 工兵标签占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0563 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3612
- 问题：/* 确定/取消（row9=确定 row4=蓝方 占位，★ 诚实边界：取消文字用 row4 蓝方占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0564 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3614
- 问题：int row_idx = (i == 3) ? 9 : 4;   /* 确定=row9，取消=row4 蓝方占位 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0565 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3650
- 问题：+ 桌号/密码占位标签 + 确定/取消 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0566 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3751
- 问题：/* ④ 桌号/密码 占位 EditLabel（矩形浅色底 + 占位文字，★ 诚实边界：不画真输入框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0567 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3778
- 问题：/* 占位标签文字（row2=桌号 row3=密码，输入框内左对齐） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0568 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3860
- 问题：LOGI("rs_draw_join：加入/创建游戏弹窗（datah_%s 桌号/密码占位标签 + 确定/取消，面板 %.0f×%.0f，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0569 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4126
- 问题：/* 确定/取消 按钮文字（row10=确定 row6=蓝方 占位，★ 诚实边界：取消用 row6 占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0570 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4142
- 问题：+ Info 文本框（战况统计复用占位） + 复盘/删除/返回 3 按钮 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0571 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4155
- 问题：/* ③ Info 文本框（浅色矩形占位，datah_050 Info 30,75,390,285 420 系 → 180,525-540,735 720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0572 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4161
- 问题：/* ⑤ Info 文本框内 4 行占位标签（row1=蓝方 row2=红方 row3=步数 row4=回合，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0573 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4162
- 问题：战况统计语义复用；动态数字未烘焙 → 诚实边界占位标签 + LOGI 实际值） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0574 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4183
- 问题：LOGI("rs_draw_gameinfo：对局信息 Info 文本（蓝方=%d 红方=%d 步数=%d cur=%d/%d，占位标签未烘焙数字）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0575 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4212
- 问题：+ 滑条占位（不画真滑条，亮度 低/中/高 3 档循环切换区） + 确定/取消 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0576 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4224
- 问题：/* ③ 滑条占位条（Bright 区 152,601-568,649 720 基准，画 3 档占位色块 低/中/高，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0577 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4272
- 问题：+ Msg 文本框（诚实边界占位文本） + 清除/关闭 2 按钮 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0578 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4274
- 问题：★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0579 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4284
- 问题：/* ③ Msg 文本框（浅色矩形占位，datah_029 Msg 30,75,390,430 420 系 → 180,450-540,805 720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0580 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4290
- 问题：/* ⑤ Msg 文本框内占位标签（row1=聊天 row2=占位 row3=帧 row4=边界 row5=未逆向，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0581 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4291
- 问题：诚实边界占位文本；动态聊天文本未烘焙 → 占位标签） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0582 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4296
- 问题：CHATLOG_ROW, 2, 360.0f, 540.0f, 150.0f, uc);   /* 占位 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0583 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4322
- 问题："诚实边界：联机聊天文本未逆向，src_rs 蓝牙聊天是占位帧 BT_T_CHAT）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0584 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4328
- 问题：+ Info 文本框（诚实边界占位文本） + 修改/帮助/返回 3 按钮 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0585 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4330
- 问题：★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0586 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4340
- 问题：/* ③ Info 文本框（浅色矩形占位，datah_031 Info 30,75,390,420 420 系 → 180,455-540,800 720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0587 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4346
- 问题：/* ⑤ Info 文本框内占位标签（row1=账号 row2=昵称 row3=头像 row4=胜率 row5=边界 row6=未逆向，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0588 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4347
- 问题：诚实边界占位文本；动态账号信息未烘焙 → 占位标签） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0589 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4388
- 问题：图标图集 tex_imen_icons 8 槽占位映射） + 7 标签文字图集（drawdetail_text_atlas）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0590 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:4390
- 问题：★ 诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0591 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4392
- 问题：玩家资料/软件信息 图标=☰ 图标图集占位映射（原图集无专用图标）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0592 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4395
- 问题：玩家资料=0/软件信息=1 占位（原图集无专用图标，★ 诚实边界）） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0593 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:4451
- 问题："战况统计/软件信息，面板 %.0f×%.0f，图标=☰ 图标图集占位映射，诚实边界：本地提和判定未实现，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0594 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:4459
- 问题：★ 诚实边界：图像设置渲染参数未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0595 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:4505
- 问题："诚实边界：图像设置渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0596 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:4516
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0597 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4570
- 问题：/* 取消按钮：不画标签（原图集无"取消"标签，诚实边界占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0598 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:4609
- 问题："诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅界面动画过渡帧未实现，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0599 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4621
- 问题：+ 8 显示占位标签（先手/后手/玩法/明暗棋/4 规则） + 开始/取消 2 按钮 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0600 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4624
- 问题：显示占位标签"蓝方/红方/布局/暗棋/开/开/开/开"，不画真数据）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0601 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4638
- 问题：/* ④ 8 显示占位标签（row1=蓝方 row2=红方 row3=布局 row4=暗棋 row5=开 row6=开 row7=开 row8=开） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0602 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4641
- 问题：/* 浅色占位标签区（非按钮，仅显示） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0603 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4657
- 问题：int row_idx = (i == 8) ? 9 : 8;   /* row9=开始 row8=开（占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0604 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4665
- 问题：LOGI("rs_draw_join065：加入游戏完整版弹窗（datah_065 8 显示占位标签 + 开始/取消，面板 %.0f×%.0f，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0605 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4666
- 问题："诚实边界：联机开局配置=占位标签（蓝方/红方/布局/暗棋/开/开/开/开），"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0606 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4704
- 问题：else if (i == 2) vrow = ui_show_get() ? 8 : 7;   /* 明暗棋：明/暗（简化用 布局/翻棋 占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0607 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4705
- 问题：else vrow = 4;   /* 游戏规则：text="设置"（row4 占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0608 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4800
- 问题：+ 2 玩家占位标签（蓝方/红方） + 开始/换桌/离开 3 按钮 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0609 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4803
- 问题：开始/换桌→边界对话框；2 玩家显示区=占位标签（不画真数据，触摸 flaky 下房间列表无意义） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0610 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4816
- 问题：/* ④ 2 玩家占位标签（row1=蓝方 row2=红方，浅色矩形占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0611 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4842
- 问题：LOGI("rs_draw_room060：房间态弹窗（datah_060 2 玩家占位标签 + 开始/换桌/离开，面板 %.0f×%.0f，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0612 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4843
- 问题："诚实边界：联机房间=边界对话框（src_rs 仅蓝牙已实现），2 玩家显示区=占位标签）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0613 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4849
- 问题：+ Msg 输入框占位标签（"聊天输入（诚实边界：联机聊天文本未逆向）"）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0614 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4852
- 问题：★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT，无真实文本渲染），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0615 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4853
- 问题：Msg 输入框简化为占位标签；发送→BT_T_CHAT 占位帧+边界对话框；记录→UI_CHATLOG（datah_029 已实现） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0616 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4863
- 问题：/* ③ Msg 输入框占位（浅色矩形，datah_063 Msg 区简化为占位标签） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0617 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4870
- 问题：/* ⑤ Msg 输入框内占位标签（row1=输入 row2=占位 row3=边界 row4=未逆向） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0618 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4875
- 问题：CHAT063_ROW, 2, 360.0f, UI_CHAT063_POPUP_Y0 + 130.0f, 150.0f, uc);  /* 占位 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0619 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4899
- 问题：LOGI("rs_draw_chat063：聊天完整版弹窗（datah_063 Msg 输入框（占位标签）+ 发送/记录/关闭 3 按钮，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0620 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4900
- 问题："面板 %.0f×%.0f，诚实边界：联机聊天文本未逆向，发送=BT_T_CHAT 占位帧+边界对话框，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0621 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8746
- 问题：/* ③ Version 文本框（浅色矩形占位，datah_000 Version 140,90,390,120 400 系 → 300,590-550,620 720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0622 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8749
- 问题：/* ④ 官网文本框（浅色矩形占位，datah_000 官网 145,135,390,165 400 系 → 305,635-550,665 720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0623 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8764
- 问题：LOGI("rs_draw_about：软件信息 Version=1.6.8 + 官网 www.cnvcs.com（占位标签，未烘焙动态版本字段）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0624 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8808
- 问题：★ 诚实边界：排行榜/消息数据未逆向→边界对话框，按钮标签=图集 row12/row13 占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0625 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8844
- 问题：/* Info 文本框占位（22,15,367,120 局部 + 45,620 → 67,635-412,740，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0626 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8862
- 问题：/* ③ datah_046 玩家资料（420×420 居中 +150,+430；标题 + Info 占位 + 返回 1 按钮） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0627 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8870
- 问题：/* Info 文本框占位（30,75,390,325 局部 + 150,430 → 180,505-540,755，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0628 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8883
- 问题："1 按钮，★ 诚实边界：玩家资料内容未逆向，占位标签）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0629 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8888
- 问题：/* ④ datah_047 玩家列表（420×305 居中 +150,+487.5；标题 + ListBox 占位 + 关闭 1 按钮） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0630 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8896
- 问题：/* ListBox 占位（20,75,400,200 局部 + 150,487.5 → 170,563-550,688，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0631 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8909
- 问题："1 按钮，★ 诚实边界：玩家列表未逆向，ListBox 占位）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0632 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8914
- 问题：/* ⑤ datah_049 对局详情（420×610 居中 +150,+335；标题 + ListBox 占位 + 关闭 1 按钮） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0633 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8922
- 问题：/* ListBox 占位（20,75,400,505 局部 + 150,335 → 170,410-550,915，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0634 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8935
- 问题："1 按钮，★ 诚实边界：对局详情内容未逆向，ListBox 占位）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0635 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8952
- 问题：/* ListBox 占位（20,75,430,505 局部 + 135,335 → 155,410-585,915，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0636 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8983
- 问题：LOGI("%s：450×610 对局列表弹窗出帧（ListBox 占位 + 清空/关闭 2 按钮，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0637 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9007
- 问题：/* ⑧ datah_073 创建游戏（450×270 居中 +135,+505；标题 + 输入框占位 + 开始/取消 2 按钮） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0638 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9015
- 问题：/* Player1 输入框占位（30,85,420,120 局部 + 135,505 → 165,590-555,625，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0639 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9018
- 问题：/* Player2 输入框占位（30,130,420,165 局部 + 135,505 → 165,635-555,670，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0640 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9028
- 问题：/* 按钮文字：row8=人机对战/row10=联网对战 图集占位（★ 诚实边界：标签非精确"开始/取消"） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0641 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9094
- 问题：/* ⑫ datah_118 版本更新（450×360 居中 +135,+460；标题 + Info 占位 + 更新/取消 2 按钮） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0642 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9102
- 问题：/* Info 文本框占位（35,80,420,253 局部 + 135,460 → 170,540-555,713，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0643 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9112
- 问题：/* 按钮文字：row11/row13 占位（★ 诚实边界：标签非精确"更新/取消"） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0644 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9153
- 问题：/* ① 棋子占位方块（当前回放位置，诚实边界不画真棋子图） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0645 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9210
- 问题：/* ③ 步数标签（"复盘" 标题 row0 顶部 + 位置行 row6"第cur/n步"，文字图集占位标签；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0646 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9211
- 问题：动态 cur/n 数字未烘焙 → 诚实边界：只画"第步/结束"占位标签 + LOGI 实际步数） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0647 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9215
- 问题：rs_replay_text(rr, slot, 280.0f, 780.0f, 150.0f, 46.0f, 6, uc);     /* "第步" 位置行占位 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0648 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9221
- 问题：LOGI("rs_draw_replay：复盘回放渲染（datah_107 控制条 5 按钮 460×60@10,705 + 棋盘占位 + 步数标签 cur=%d/%d，jump 简化回开头/跳结尾，菜单简化退复盘）", cur, n);
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0649 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:9884
- 问题：关闭=强制棋子立即到位，诚实边界：动画帧未实现，开关仅门控"是否画"）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0650 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:9886
- 问题：- 棋子动画关（ui_imgset_piece=0）：棋子不画（诚实边界：过渡帧未实现，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0651 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9887
- 问题：关闭=棋子占位不画，对齐原版"关动画棋子静止"语义） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0652 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:9888
- 问题：int imgset_trans  = ui_imgset_cfg_get(0);   /* 界面动画（过渡帧未实现，门控用） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0653 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9890
- 问题：int imgset_piece  = ui_imgset_cfg_get(1);   /* 棋子动画：=0 时棋子不画（静止占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0654 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9920
- 问题：/* 棋子动画关 → 不画棋子（占位静止，诚实边界）；界面动画关 → 同上（无过渡帧可省） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0655 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9930
- 问题：- 棋子动画开关（ui_imgset_cfg_get(1)，默认 1=开）：=0 时棋子不画（静止占位）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0656 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:9931
- 问题：诚实边界：界面动画过渡帧未实现（开关仅门控）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0657 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:10078
- 问题：/* ★ P1（datah_038 证据）：登录弹窗（用户名/密码占位标签 + 登录/注册/游客/返回 4 按钮） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0658 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:10090
- 问题：/* ★ P2（datah_034 证据）：联机"加入游戏"弹窗（桌号占位标签 + 确定/取消，面板 420×235 居中） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0659 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:10093
- 问题：/* ★ P2（datah_035 证据）：联机"创建游戏"弹窗（密码占位标签 + 确定/取消，面板 420×235 居中） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0660 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:10127
- 问题：/* ★ P2（datah_065 证据）：加入游戏完整版弹窗（8 显示占位标签 + 开始/取消，面板 450×505 居中） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0661 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:10136
- 问题：/* ★ P2（datah_060 证据）：房间态弹窗（2 玩家占位标签 + 开始/换桌/离开，面板 450×352 居中） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0662 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:10139
- 问题：/* ★ P2（datah_063 证据）：聊天完整版弹窗（Msg 输入框占位 + 发送/记录/关闭，面板 420×325 居中） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0663 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.h:89
- 问题：记录/聊天/清除/关闭/占位/帧/边界/未逆向） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0664 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.h:113
- 问题：占位/边界/未逆向） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0665 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render_obj.h:23
- 问题：int _vtable;   /* 对齐原版节点 [0]（重写填 0，仅占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0666 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/replay.c:9
- 问题：*   只收蓝方半区，对局进行中（已被红子占位）提取会镜像出错误红方分布
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0667 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/replay.c:64
- 问题：对局进行中（已被红子占位）提取会镜像出错误红方分布（实证 init_lay 非零
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0668 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/replay.h:10
- 问题：*   - 复盘态不画真实棋子图（渲染层复用棋盘现有占位范式）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0669 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/rules.c:131
- 问题：空格非铁路占位 → 工兵 BFS 不可"滑行"进入（与 move_legal 条款一致）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0670 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/rules.c:132
- 问题：占位空格且非行营/大本营 → walkable 真 → 目标命中。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0671 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:5
- 问题：*   人机对战（开局 STG_PLAY 蓝先手）/ 两人对战（占位）/ 联网对战（占位）/
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0672 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:6
- 问题：*   四国军棋（占位）/ 悔棋重开（restart）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0673 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:8
- 问题：* 诚实边界：两人/联网/四国 = 占位（日志记录，不实现联机 UI）；人机 = 直接开局。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0674 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:111
- 问题：case 2: LOGI("UI菜单：联网对战（占位，SPP socket 由 Java 宿主承担）"); break;
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0675 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:112
- 问题：/* ★ 对抗审计 P1（批4 P1-R4）：原 case 3 日志误写"四国军棋（占位）"——
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0676 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:114
- 问题：case 3 应与 case 2 区分：联机=蓝牙 SPP 会话（已实装），联网=边界占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0677 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:259
- 问题：4 人 15,155,215,215 → 260,680-460,740）。★ 诚实边界：四国=未实现 4 方规则。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0678 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:350
- 问题：★ 诚实边界：ListBox 滚动未实现（触摸 flaky），文本列表 6 条截断 + 选中高亮。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0679 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:412
- 问题：简化小方块 6×5 占位（有子=蓝/红方块，真棋子图未画）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0680 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:413
- 问题：面板 400×475 居中（偏移+160,+402.5）。★ 诚实边界：原版 Ok2/Cancel2 重叠备用按钮未实现。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0681 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:470
- 问题：/* ★ P1（datah_038 证据）：登录弹窗 6 控件（用户名/密码占位标签 + 登录/注册/游客/返回）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0682 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:478
- 问题：★ 诚实边界：原版登录=账号系统（服务器协议未逆向）；输入框简化为占位标签
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0683 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:482
- 问题：{ 274.0f, 578.0f, 519.0f, 623.0f, 0 },   /* 用户名 EditLabel 占位标签（130,80,375,125） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0684 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:483
- 问题：{ 274.0f, 638.0f, 519.0f, 683.0f, 1 },   /* 密码 EditLabel 占位标签（130,140,375,185） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0685 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:577
- 问题：{ 270.0f, 602.5f, 510.0f, 647.5f, 0 },   /* 桌号 Table（120,80,360,125）→ 占位标签 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0686 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:610
- 问题：{ 250.0f, 602.5f, 470.0f, 647.5f, 0 },   /* 密码 Pwd（100,80,320,125）→ 占位标签 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0687 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:732
- 问题：★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0688 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:735
- 问题：{ 187.0f, 825.0f, 343.0f, 885.0f, 0 },   /* 清除 Clear（37,450,193,510）→ 清本地占位记录 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0689 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:761
- 问题：★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0690 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:793
- 问题：业务：提和/认输/聊天 = 发 BT 帧 + 边界对话框（本地提和判定未实现，诚实边界）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0691 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:795
- 问题：★ 诚实边界：原版提和需对端应答（判定未实现），src_rs 离线单机无对端，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0692 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:832
- 问题：★ 诚实边界：图像设置渲染参数未实现（src_rs 无对应渲染参数），开关仅 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0693 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:861
- 问题：★ 诚实边界：图像设置渲染参数未实现，开关仅 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0694 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:891
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0695 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:927
- 问题：★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0696 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:961
- 问题：业务：联机"加入游戏"完整版（8 项显示用 TransLabel 占位标签
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0697 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:964
- 问题：显示占位标签，不画真数据，触摸 flaky 下输入框无意义）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0698 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:969
- 问题：{ 275.0f, 473.0f, 555.0f, 508.0f, 0 },   /* 先手 Player1（140,85,420,120）→ 占位标签"蓝方" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0699 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:970
- 问题：{ 275.0f, 513.0f, 555.0f, 548.0f, 1 },   /* 后手 Player2（140,125,420,160）→ 占位标签"红方" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0700 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:971
- 问题：{ 275.0f, 553.0f, 555.0f, 588.0f, 2 },   /* 玩法 Lay（140,165,420,200）→ 占位标签"布局" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0701 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:972
- 问题：{ 275.0f, 593.0f, 555.0f, 628.0f, 3 },   /* 明暗棋 Show（140,205,420,240）→ 占位标签"暗棋" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0702 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:973
- 问题：{ 275.0f, 633.0f, 555.0f, 668.0f, 4 },   /* 和棋 Draw（140,245,420,280）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0703 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:974
- 问题：{ 275.0f, 673.0f, 555.0f, 708.0f, 5 },   /* 大本营 CmdSafe（140,285,420,320）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0704 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:975
- 问题：{ 275.0f, 713.0f, 555.0f, 748.0f, 6 },   /* 吃地雷 BingLei（140,325,420,360）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0705 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:976
- 问题：{ 275.0f, 753.0f, 555.0f, 788.0f, 7 },   /* 吃军旗 BingQi（140,365,420,400）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0706 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1076
- 问题：简化为占位标签"蓝方/红方"）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0707 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1079
- 问题：★ 诚实边界：2 玩家显示区 = 占位标签（不画真数据，触摸 flaky 下房间列表无意义） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0708 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1108
- 问题：Msg 输入框简化为占位标签"聊天输入（诚实边界：联机聊天文本未逆向）"。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0709 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1109
- 问题：业务：联机"聊天"完整版，发送→BT_T_CHAT 占位帧+边界对话框，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0710 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1111
- 问题：★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0711 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1115
- 问题：{ 175.0f, 723.0f, 285.0f, 783.0f, 0 },   /* 发送 Send（25,245,135,305）→ BT_T_CHAT 占位帧 + 边界对话框 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0712 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1170
- 问题：★ 诚实边界：排行榜/消息数据未逆向，按钮→边界对话框占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0713 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1214
- 问题：★ 诚实边界：玩家资料内容未逆向（账号系统未实现），Info 占位标签。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0714 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1235
- 问题：★ 诚实边界：玩家列表未逆向，ListBox 占位标签。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0715 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1256
- 问题：★ 诚实边界：对局详情内容未逆向，ListBox 占位标签。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0716 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:20
- 问题：诚实边界：选人数 → 边界对话框（4 方规则未实现）→ 回主菜单 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0717 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:36
- 问题：30 格棋子预览（简化小方块占位）+ 载入/删除/取消 3 按钮（原版 datah_016 400×475 面板居中）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0718 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:38
- 问题：诚实边界：30 格 PiecePic 真棋子图未画，小方块占位（有子=蓝/红） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0719 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:40
- 问题：用户名/密码占位标签（EditLabel，不画真输入框）+ 登录/注册/游客/返回 4 按钮
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0720 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:43
- 问题：Junqi.* 登录/注册 20+ 事件未实现）；游客=本地单机（无账号，唯一真生效路径）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0721 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:71
- 问题：桌号=占位标签（不画真输入框）。入口：datah_072 方式弹窗"Wi-Fi 联机"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0722 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:78
- 问题：密码=占位标签（不画真输入框）。入口：setprop jq.create 测试钩子。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0723 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:105
- 问题：占位帧（BT_T_CHAT，对端无文本渲染）；Msg 文本框显示诚实边界占位文本 +
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0724 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:106
- 问题：清除 → 清本地占位记录 + 关闭 → 回对局。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0725 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:110
- 问题：Info 文本=诚实边界占位（账号系统未逆向，离线单机无账号）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0726 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:111
- 问题：修改 → 诚实边界对话框（"修改资料需账号系统，未实现"）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0727 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:122
- 问题：值 1=进 / 0=退）。★ 诚实边界：图像设置=渲染参数未实现，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0728 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:140
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0729 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:150
- 问题：★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0730 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:155
- 问题：8 项显示用 TransLabel（非按钮，显示当前值占位标签"蓝方/红方/
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0731 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:163
- 问题：无联机开局逻辑，显示占位标签，不画真数据） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0732 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:188
- 问题：是 TransText 显示区（非按钮），简化为占位标签；开始/换桌/离开
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0733 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:195
- 问题：★ 诚实边界：2 玩家显示区 = 占位标签（不画真数据，触摸 flaky
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0734 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:200
- 问题：Msg 输入框简化为占位标签"聊天输入（诚实边界：联机聊天文本
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0735 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:201
- 问题：未逆向）"；发送 → BT_T_CHAT 占位帧 + 边界对话框；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0736 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:207
- 问题：★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0737 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:602
- 问题：★ 诚实边界：本地提和判定未实现（src_rs 离线单机无对端），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0738 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:612
- 问题：★ 诚实边界：排行榜/消息数据未逆向，按钮→边界对话框占位 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0739 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:621
- 问题：（标题"玩家资料" + Info 文本框占位 + 返回 1 按钮，原版 420×420
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0740 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:625
- 问题：★ 诚实边界：玩家资料内容未逆向（账号系统未实现），占位标签 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0741 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:627
- 问题：（标题"玩家列表" + ListBox 占位 + 关闭 1 按钮，原版 420×305
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0742 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:631
- 问题：★ 诚实边界：玩家列表未逆向，ListBox=占位标签 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0743 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:633
- 问题：（标题 + ListBox 占位 + 关闭 1 按钮，原版 420×610 面板居中
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0744 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:637
- 问题：★ 诚实边界：对局详情内容未逆向，ListBox=占位标签 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0745 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:639
- 问题：（标题"我的对局" + ListBox 占位 + 清空/关闭 2 按钮，原版 450×610
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0746 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:647
- 问题：（标题"历史对局" + ListBox 占位 + 清空/关闭 2 按钮，原版 450×610
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0747 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:654
- 问题：（标题"创建游戏" + Player1/Player2 输入框占位 + 开始/取消 2 按钮，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0748 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:678
- 问题：（标题"历史对局" + ListBox 占位 + 清空/关闭 2 按钮，原版 450×610
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0749 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:685
- 问题：（标题"版本更新" + Info 文本框占位 + 更新/取消 2 按钮，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0750 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:774
- 问题：void ui_menu_action_log(int action);  /* 按钮动作日志（占位语义） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0751 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:790
- 问题：action：0=2 人 1=3 人 2=4 人（默认 4）。★ 诚实边界：四国 4 方规则未实现（src_rs=10×9 两人军棋）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0752 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:853
- 问题：★ 诚实边界： ListBox 滚动未实现（触摸 flaky），文本列表 6 条截断 + 选中高亮。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0753 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:888
- 问题：20,75,380,360 区占位）+ 载入/删除/取消 3 按钮（原版 Ok 25,390,129,450 → 185,793-289,853；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0754 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:892
- 问题：★ 诚实边界： 30 格 PiecePic 真棋子图未画（小方块占位，有子=蓝/红方块）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0755 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:893
- 问题：原版 Ok2/Cancel2 重叠备用显示按钮未实现。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0756 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:939
- 问题：/* ★ P1（datah_038 证据）：登录弹窗——用户名/密码占位标签 + 登录/注册/游客/返回 4 按钮。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0757 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:950
- 问题：Junqi.* 登录/注册 20+ 事件未实现）；用户名/密码 输入框简化为占位标签（触摸 flaky
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0758 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1008
- 问题：/* ★ P2（datah_034 证据）：联机"加入游戏"弹窗——桌号输入（占位标签）+ 确定/取消。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0759 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1012
- 问题：action：0=桌号（占位标签）1=确定（诚实边界对话框）2=取消（回主菜单）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0760 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1029
- 问题：/* ★ P2（datah_035 证据）：联机"创建游戏"弹窗——密码输入（占位标签）+ 确定/取消。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0761 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1033
- 问题：action：0=密码（占位标签）1=确定（诚实边界对话框）2=取消（回主菜单）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0762 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1130
- 问题：action：0=清除（清本地占位记录）1=关闭（回对局）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0763 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1132
- 问题：src_rs 蓝牙聊天是占位帧（BT_T_CHAT，对端无文本渲染）；Msg 文本框显示诚实边界占位文本。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0764 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1152
- 问题：action：0=修改（诚实边界对话框：修改资料需账号系统未实现）1=帮助（显示帮助文本）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0765 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1157
- 问题：边界占位文本；修改=边界对话框。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0766 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1186
- 问题：★ 诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0767 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1210
- 问题：★ 诚实边界：图像设置入口指向 UI_IMGSET7（图像设置渲染参数未实现，仅 UI 状态记录）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0768 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1233
- 问题：★ 诚实边界：图像设置入口指向 UI_IMGSET8（图像设置渲染参数未实现，仅 UI 状态记录）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0769 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1262
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0770 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1293
- 问题：★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0771 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1322
- 问题：action：0-7=8 显示项（先手/后手/玩法/明暗棋/和棋/大本营/吃地雷/吃军旗，TransLabel 占位标签
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0772 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1323
- 问题：非按钮，显示当前值占位"蓝方/红方/布局/暗棋/开/开/开/开"）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0773 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1328
- 问题：显示占位标签，不画真数据；触摸 flaky 下输入框无意义） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0774 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1410
- 问题：简化为占位标签"蓝方/红方"）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0775 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1415
- 问题：★ 诚实边界：2 玩家显示区 = 占位标签（不画真数据，触摸 flaky 下房间列表无意义） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0776 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1436
- 问题：Msg 输入框简化为占位标签"聊天输入（诚实边界：联机聊天文本未逆向）"。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0777 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1437
- 问题：action：0=发送（→ BT_T_CHAT 占位帧 + 边界对话框）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0778 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1441
- 问题：★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0779 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:2593
- 问题：占位标签 + 边界对话框，不包装成"已实现" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0780 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:37
- 问题：/* ★ P1（datah_016/017/020）：布局存档 3 弹窗按钮桩（桌面 headless 无渲染，几何表返回空占位
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0781 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:82
- 问题：/* ★ P1（datah_039/107）：复盘回放控制条 5 按钮桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0782 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:95
- 问题：/* ★ P1（datah_038）：登录弹窗 6 控件桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0783 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:109
- 问题：/* ★ P2（datah_037 证据）：规则文本弹窗 3 按钮桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0784 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:122
- 问题：/* ★ P2（datah_104 证据）：新局完整版弹窗 5 控件桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0785 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:136
- 问题：/* ★ P2（datah_034 证据）：联机"加入游戏"弹窗 3 控件桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0786 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:149
- 问题：/* ★ P2（datah_035 证据）：联机"创建游戏"弹窗 3 控件桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0787 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:162
- 问题：/* ★ P2（datah_061 证据）：新局完整版（完整版补充）弹窗 6 控件桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0788 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:201
- 问题：诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0789 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:212
- 问题：诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0790 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:223
- 问题：/* ★ P2（datah_042 证据）：提和详情弹窗 7 图标桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0791 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:225
- 问题：诚实边界：本地提和判定未实现（原版需对端应答），提和/认输走 BT 帧 + 边界对话框。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0792 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:239
- 问题：/* ★ P2（datah_003/004 证据）：设置入口 3/2 按钮桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0793 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:241
- 问题：诚实边界：图像设置渲染参数未实现，开关仅 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0794 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:263
- 问题：诚实边界：图像/夜间模式渲染参数未实现，开关仅 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0795 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:288
- 问题：诚实边界：联机开局配置（src_rs 无联机开局逻辑），8 项显示=占位标签，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0796 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:348
- 问题：诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0797 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:349
- 问题：发送→占位帧+边界对话框。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0798 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:363
- 问题：反馈按钮→诚实边界对话框（几何表返回 720 基准占位供一致性/evverify 链接；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0799 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:376
- 问题：桌面桩与 ui_menu.c 真实几何表同步（诚实边界占位不包装成"已实现"）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0800 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:1596
- 问题：桩内命名 _stub2 避免与上方 720 基准占位表冲突。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0801 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_state.c:7
- 问题：* 两人/联网/四国 = 占位（日志记录，未实现联机 UI/四国规则，诚实边界）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0802 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_state.c:253
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0803 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_state.c:399
- 问题：文本列表 6 条截断 + 小方块占位预览，诚实边界见 AUDIT_LAYOUT_ALIGN §十六）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0804 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_state.h:7
- 问题：void ui_on_menu_action(int action);   /* 菜单按钮点击：0人机→选项弹窗 4重开 其它占位 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0805 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_state.h:73
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0806 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_state_stub.c:89
- 问题：诚实边界：图像/夜间模式渲染参数未实现，开关仅 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-0807 [P3] UI_D019 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (150,455)-(255,590) 面板 (135,430)-(585,580)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-0808 [P3] UI_D019 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (255,455)-(360,590) 面板 (135,430)-(585,580)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-0809 [P3] UI_D019 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (360,455)-(465,590) 面板 (135,430)-(585,580)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-0810 [P3] UI_D019 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (465,455)-(570,590) 面板 (135,430)-(585,580)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-0811 [P3] UI_D079 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (270,900)-(450,960) 面板 (140,365)-(360,705)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-0812 [P3] UI_D083 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (150,588)-(255,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-0813 [P3] UI_D083 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (255,588)-(360,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-0814 [P3] UI_D083 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (360,588)-(465,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-0815 [P3] UI_D083 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (465,588)-(570,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-0816 [P3] UI_D099 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (150,588)-(255,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-0817 [P3] UI_D099 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (255,588)-(360,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-0818 [P3] UI_D099 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (360,588)-(465,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-0819 [P3] UI_D099 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (465,588)-(570,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-0820 [P3] UI_D123 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (307,610)-(382,678) 面板 (300,602)-(420,677)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

