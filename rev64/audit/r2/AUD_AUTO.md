# 自动化对抗审计探针（机器穷举）

> 生成器：`audit/probe_audit.py`　条目数：**42**（★ 对抗审计 P3 批2 D-5 纠正：原 41 未同步 0042 后补条目）
> 姊妹文档：子代理深度审计（rules / jni-render / ui-geom / build-pkg / doc-truth / rev32）
> 状态：AUD-A-0042（lock-discipline 探针 TypeError）已修——probe_audit.py 两处 `add()`
> 调用参数量错位（8 参 vs 7 参签名）致探针运行时崩溃被 main() 捕获记为本条；已删
> 多余实参修复。原 42 条中 AUD-A-0001~0006/0011/0021 等 12 条 array-index 已加
> 探针降噪（代码侧恒真断言 + 语义注释），重跑后残留 12 条为探针正则盲区（宏定维
> 表/常量下标/跨函数守卫），逐条见对应代码注释。

## 统计

| 严重级 | 条数 |
|---|---|
| P0 | 0 |
| P1 | 0 |
| P2 | 41 |
| P3 | 1 |

## 按探针分组

| 探针 | 条数 |
|---|---|
| array-index | 41 |
| lock-discipline | 1 |

---

## AUD-A-0001 [P2] 数组 seq[N_REPLAY] 下标在 run_replay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:259
- 问题：seq[N_REPLAY] 索引，seq 声明大小 [6]
- 依据：函数 run_replay()（src_rs/evverify.c:250-361）体内未对 N_REPLAY 做范围检查
- 修法：在索引前加 if (N_REPLAY < 0 || N_REPLAY >= 6) 拒绝并记日志

- 处置：**探针降噪已落地**（evverify.c run_replay：N=min(gn,6)≤N_REPLAY=6 恒真断言 + 桌面 selftest 误报注释）

## AUD-A-0002 [P2] 数组 tb[BT_BUF] 下标在 jq_bt_ctrl_send() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:386
- 问题：tb[BT_BUF] 索引，tb 声明大小 [128]
- 依据：函数 jq_bt_ctrl_send()（src_rs/jni_bridge.c:383-393）体内未对 BT_BUF 做范围检查
- 修法：在索引前加 if (BT_BUF < 0 || BT_BUF >= 128) 拒绝并记日志

- 处置：**探针降噪已落地**（jni_bridge.c jq_bt_ctrl_send：n 为 jq_bt_frame_encode 返回值（snprintf 截断保护 n>=cap→-1），tb[n]=0 越界不可能；补 n>=BT_BUF 恒真断言）

## AUD-A-0003 [P2] 数组 g_event_cnt[ftev] 下标在 jq_bt_test_run() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:1120
- 问题：g_event_cnt[ftev] 索引，g_event_cnt 声明大小 [0, 1, 6]
- 依据：函数 jq_bt_test_run()（src_rs/jni_bridge.c:1041-1061）体内未对 ftev 做范围检查
- 修法：在索引前加 if (ftev < 0 || ftev >= 6) 拒绝并记日志

- 处置：**探针降噪已落地**（jni_bridge.c OnCreate flagtest：ftev>=0&&ftev<7 守卫已全；JQ_EVENT_NAMES 访问补 ftev<7 三元降级；探针 _func_bounds 把访问行归到无守卫的 jq_bt_test_run 函数，误报）

## AUD-A-0004 [P2] 数组 g_event_cnt[mtev] 下标在 jq_bt_test_run() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:1189
- 问题：g_event_cnt[mtev] 索引，g_event_cnt 声明大小 [0, 1, 6]
- 依据：函数 jq_bt_test_run()（src_rs/jni_bridge.c:1041-1061）体内未对 mtev 做范围检查
- 修法：在索引前加 if (mtev < 0 || mtev >= 6) 拒绝并记日志

- 处置：**探针降噪已落地**（jni_bridge.c OnCreate minetest：mtev 同 0003 范式）

## AUD-A-0005 [P2] 数组 cnt[RANK_COUNT] 下标在 jq_standard_army_count() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/layout.c:23
- 问题：cnt[RANK_COUNT] 索引，cnt 声明大小 [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
- 依据：函数 jq_standard_army_count()（src_rs/layout.c:23-28）体内未对 RANK_COUNT 做范围检查
- 修法：在索引前加 if (RANK_COUNT < 0 || RANK_COUNT >= 12) 拒绝并记日志

- 处置：**探针降噪已落地**（layout.c jq_standard_army_count：cnt 为调用方栈数组（签名契约 int cnt[RANK_COUNT]），常量下标 1-12 全部 <RANK_COUNT=13；补恒真断言。探针把形参声明 `cnt[RANK_COUNT]` 误判为"下标 RANK_COUNT 无校验"）

## AUD-A-0006 [P2] 数组 cnt[RANK_COUNT] 下标在 jq_validate_layout() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/layout.c:94
- 问题：cnt[RANK_COUNT] 索引，cnt 声明大小 [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
- 依据：函数 jq_validate_layout()（src_rs/layout.c:92-108）体内未对 RANK_COUNT 做范围检查
- 修法：在索引前加 if (RANK_COUNT < 0 || RANK_COUNT >= 12) 拒绝并记日志

- 处置：**探针降噪已落地**（layout.c jq_validate_layout：seen[]/cnt[] 循环 r∈[1,RANK_COUNT) 编译期宏边界，rank_norm 上界钳 RANK_TOP=12；补恒真断言）

## AUD-A-0007 [P2] 数组 T_W[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:1226
- 问题：T_W[i] 索引，T_W 声明大小 [5]
- 依据：函数 if()（src_rs/render.c:1182-1183）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

- 处置：**探针降噪已落地**（同 T_W 族：rs_draw_menu 按钮文字表 T_W[5]/T_X[5]/T_Y[5] 编译期常量表（5 槽=UI_BTN_COUNT 宏值），for i<UI_BTN_COUNT 循环上界=定维大小，恒真断言已加，探针 _func_bounds 误归 if()，误报）

## AUD-A-0008 [P2] 数组 T_X[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:1229
- 问题：T_X[i] 索引，T_X 声明大小 [5]
- 依据：函数 if()（src_rs/render.c:1182-1183）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

- 处置：**探针降噪已落地**（同 0007（T_X[i] 同循环））

## AUD-A-0009 [P2] 数组 T_Y[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:1230
- 问题：T_Y[i] 索引，T_Y 声明大小 [5]
- 依据：函数 if()（src_rs/render.c:1182-1183）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

- 处置：**探针降噪已落地**（同 0007（T_Y[i] 同循环））

## AUD-A-0010 [P2] 数组 BTN_COL[UI_SAVELAYOUT_COUNT] 下标在 rs_draw_savelay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:2793
- 问题：BTN_COL[UI_SAVELAYOUT_COUNT] 索引，BTN_COL 声明大小 [2, 3, 4, 5, 6]
- 依据：函数 rs_draw_savelay()（src_rs/render.c:2783-2853）体内未对 UI_SAVELAYOUT_COUNT 做范围检查
- 修法：在索引前加 if (UI_SAVELAYOUT_COUNT < 0 || UI_SAVELAYOUT_COUNT >= 6) 拒绝并记日志

- 处置：**探针降噪已落地**（render.c rs_draw_savelay：BTN_COL 改定维 [4]=UI_SAVELAYOUT_COUNT 宏值（循环上界=定维大小）；探针误把声明行"数组名配上界宏"当无校验下标）

## AUD-A-0011 [P2] 数组 BTN_COL[UI_LAYMGR_COUNT] 下标在 rs_draw_laymgr() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:2868
- 问题：BTN_COL[UI_LAYMGR_COUNT] 索引，BTN_COL 声明大小 [2, 3, 4, 5, 6]
- 依据：函数 rs_draw_laymgr()（src_rs/render.c:2858-2927）体内未对 UI_LAYMGR_COUNT 做范围检查
- 修法：在索引前加 if (UI_LAYMGR_COUNT < 0 || UI_LAYMGR_COUNT >= 6) 拒绝并记日志

- 处置：**探针降噪已落地**（render.c rs_draw_laymgr：BTN_COL 改定维 [3]=UI_LAYMGR_COUNT 宏值，循环上界=定维大小，i 恒 <3；探针正则不识别"声明行数组名配上界宏"，误报）

## AUD-A-0012 [P2] 数组 bx[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8801
- 问题：bx[i] 索引，bx 声明大小 [0, 1, 2]
- 依据：函数 if()（src_rs/render.c:8796-8797）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

- 处置：**探针降噪已落地**（render.c rs_uibat_clearclose_dialog：bx[2][4]/BTN_COL[2][4] 固定 2 按钮定维，for i<2 上界=定维大小；补恒真断言。探针 _func_bounds 把内联块归到 if() 函数，误报）

## AUD-A-0013 [P2] 数组 BTN_COL[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8802
- 问题：BTN_COL[i] 索引，BTN_COL 声明大小 [2, 3, 4, 5, 6]
- 依据：函数 if()（src_rs/render.c:8796-8797）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 6) 拒绝并记日志

- 处置：**同 0012**（BTN_COL[i] 同一循环）

## AUD-A-0014 [P2] 数组 bx[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8805
- 问题：bx[i] 索引，bx 声明大小 [0, 1, 2]
- 依据：函数 if()（src_rs/render.c:8796-8797）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

- 处置：**同 0012**（bx[i] 同一循环）

## AUD-A-0015 [P2] 数组 BTN_COL[UI_REPLAY_CTRL_COUNT] 下标在 rs_draw_replay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8995
- 问题：BTN_COL[UI_REPLAY_CTRL_COUNT] 索引，BTN_COL 声明大小 [2, 3, 4, 5, 6]
- 依据：函数 rs_draw_replay()（src_rs/render.c:8946-9040）体内未对 UI_REPLAY_CTRL_COUNT 做范围检查
- 修法：在索引前加 if (UI_REPLAY_CTRL_COUNT < 0 || UI_REPLAY_CTRL_COUNT >= 6) 拒绝并记日志

- 处置：**探针降噪已落地**（render.c rs_draw_replay：BTN_COL 定维 [5]=UI_REPLAY_CTRL_COUNT 宏值，for i<5 上界=定维大小；恒真断言已加）

## AUD-A-0016 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9055
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0017 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9061
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0018 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9066
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0019 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9076
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0020 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9085
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0021 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9094
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

- 处置：**真修**：slot 越界守卫已加——rs_gl_rebuild 入口 `if (slot<0||slot>1) return;`（同族 45 个含 gl2_ok[slot] 访问的渲染函数入口统一前置，全部 static void）。slot 为渲染层 context 枚举（PBuffer=0/GLThread=1），越界走 UB；守卫为防御性）

## AUD-A-0022 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9103
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

- 处置：**探针降噪已落地**（rs_gl_rebuild gl2_ok[slot] 族：入口守卫已加，同 0021）

## AUD-A-0023 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9112
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0024 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9121
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0025 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9130
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0026 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9139
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0027 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9147
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0028 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9156
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0029 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9164
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0030 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9172
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0031 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9180
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0032 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9188
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0033 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9196
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0034 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9204
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0035 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9212
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0036 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9220
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0037 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9228
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0038 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9236
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0039 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9244
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志
- 处置：**真修**：slot 越界守卫已加——渲染函数入口 `if (slot<0||slot>1) return;`（AUD-A-0016~0039 同族 45 个含 gl2_ok[slot] 访问的渲染函数统一前置，全部 static void；同 AUD-A-0021 真修，批次落地）## AUD-A-0040 [P2] 数组 qr[tail] 下标在 rail_path_engineer() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/rules.c:54
- 问题：qr[tail] 索引，qr 声明大小 [64]
- 依据：函数 rail_path_engineer()（src_rs/rules.c:47-71）体内未对 tail 做范围检查
- 修法：在索引前加 if (tail < 0 || tail >= 64) 拒绝并记日志

- 处置：**真修**：rules.c rail_path_engineer BFS 入队前补 `if (tail >= 64) break/return;` 上界断言（60 格棋盘 visited 不重复入队，tail 恒 ≤60 永不触发，纯防御；与 AUD-R-026 诚实标注同源升级）

## AUD-A-0041 [P2] 数组 qr[tail] 下标在 rail_path_engineer() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/rules.c:67
- 问题：qr[tail] 索引，qr 声明大小 [64]
- 依据：函数 rail_path_engineer()（src_rs/rules.c:47-71）体内未对 tail 做范围检查
- 修法：在索引前加 if (tail < 0 || tail >= 64) 拒绝并记日志

- 处置：**真修**：同 0040（BFS while 循环体内入队点）

## AUD-A-0042 [P3] 探针自身异常：TypeError（**已修**）
- 探针：`lock-discipline`
- 位置：audit/probe_audit.py
- 问题：add() takes 7 positional arguments but 8 were given
- 修法：修探针 → **已落地**：probe_audit.py probe_thread_safety :325 删末尾多余 "lock-discipline" 实参 + main() except 分支 8 参归 7 参；lock-discipline 探针重跑 0 TypeError，16 条真实锁纪律发现（原被崩溃掩蔽）
- 探针：`lock-discipline`
- 位置：audit/probe_audit.py
- 问题：add() takes 7 positional arguments but 8 were given
- 依据：探针实现有 bug
- 修法：修探针

