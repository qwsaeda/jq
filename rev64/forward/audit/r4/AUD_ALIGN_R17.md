# AUD_ALIGN_R17: P0 门禁击穿修复 + 台账漂移治理
# 生成: 2026-11-17 (正向审计承接逆向 R16 输入)
# 范围: src_rs P0 三项 + P1 批次 + R16 rules.c 9 处注释纠错

## P0 (★ 已修, 全回归 12/12 + 2.5/2.6 PASS 实锤)

### P0-1 bt_semtest 未挂 12/12 回归 (静默降级)
- **根因**: Makefile `bttest`/`bt_semtest` 两 target 源文件列表同源 (仅 test 文件不同),
  但 src_rs_regress.sh [2/12] 只跑 `make bttest` → bt_semtest.c 152 条断言池不进门禁,
  断言池扩容 (145→152) 后 12/12 不感知
- **修**: src_rs_regress.sh 新增 [2.5/12] `make bt_semtest | grep '152 条 PASS'` +
  [2.6/12] `make bt_semtest_dual | grep '43 条 PASS'`
- **实锤**: 全回归 12/12 + 2.5/2.6 全 PASS (2026-11-17 跑)

### P0-2 bt_selftest.c:129 断言与 R8-C-16 新语义矛盾 (make bttest 必 FAIL)
- **根因**: bt_session.c R8-C-16 定案 "DISCONNECTED 下 RESTART 放行 (会话恢复路径)",
  但 bt_selftest.c 仍断言 `== -2` (旧 B04-012 全拒收语义) → [2/12] 实测 FAIL
- **修**: bt_selftest.c :128-130 断言改 `== 0` + 注释对齐 R8-C-16
- **实锤**: `make bttest` 全 PASS, 全回归 [2/12] 转绿
- **台账同步**: AUD_1000 r3 B04-012/027/039 "FIXED (rc=-2 断言)" 状态标注与 R8-C-16 矛盾
  (该断言本身已失效) → r3 台账 B04-012 条目加 "R8-C-16 后语义演进, 断言基准改 rc=0" 注

### P0-3 jni_bridge.c GetMethodID 无异常清理 (src_rs_check [5/12] 长期红)
- **根因**: :2802 `getAbsolutePath` 后直接 CallObjectMethod / :3786 `getAction` 后未查
  pending exception → jni_method_audit.py 报 2 缺口, src_rs_check.py [5/12] FAIL,
  且 r3/r4 台账无 OPEN 项跟踪 (漏登记)
- **修**: 两处补 `ExceptionCheck → ExceptionClear → amid/abs = NULL` 守卫
  (:2802 区 abs 判空 + js/s 判空 + f_cls DeleteLocalRef; :3786 区 amid 判空)
- **实锤**: jni_method_audit "无异常清理 0" + `make so` 编译通过 + src_rs_check.py 全 PASS
  + 全回归 [5/12] 转绿

## P1 (★ 已修)

### P1-1 rules.c 9 处注释纠错 (R16 待修项, 正向承接落地)
- :14 "0x39f20 移动执行器" → "0x39f20 区 (走子执行器语义, 非完整移动执行; 原版 32 位 so 地址)"
- :440/:514 加 "(原版 32 位 so 地址)" 前缀
- :1213 保留口径 (语义对齐正确) + 加 "原版 32 位 so 地址" 标注
- :1216/1219/1224/1232/1263 "0x55d40 handler 实证单步评分" →
  "0x55d40 区无集中评分表, 单步贪心分散在执行器+handler (75 thunk 实证, ORIG_THUNK_55d40.json)"
- ai_score 评分表加 "正向设计, 非 32 位地址对齐" 标注

### P1-2 r3 台账 "待修队列" 段 14 条与 JSON _status 矛盾
- "待修队列 (P1 最高杠杆前 20 条)" 段头加 "★ 2026-11-17 刷新: 本队列 14 条已全部 FIXED/撤回,
  段保留为历史批次记录, 现行待修以 JSON _status 字段为准"

### P1-3 SPEC_BUSINESS.md:205 死函数描述漂移
- L205 `sfx_for_event_name()` 段落加 "★ 已删 (L242 3 死函数清理), 现行 = jq_evt55_sfx_for_name,
  本段保留为设计演变记录"

### P1-4 AUD_1000_ALL.md 矩阵 "运行中" 假停滞信号
- 12 子代理矩阵表头加 "★ 收口: 全部 12 域完结, 统计/批次表 (L25-45) 为最终态;
  矩阵行 '运行中' 为生成时点快照未刷新, 以合并统计为准"

### P1-5 src_rs_regress.sh 注释硬编码 "172/172"
- L58 注释改 "动态真值, 以 SUMMARY 行为准" (历史 8/8→167/172 三代漂移教训)

## P2 (★ 设备不可达, 降级登记 — 2026-11-17)

### P2-1 权威 5 判据 + 双端真机部署冒烟 (设备依赖, 待设备恢复)
- **现状**: `adb connect 10.0.182.146:5555` → "No route to host" (2026-11-17 实测), 红果离线
- **降级口径**: 桌面门禁全绿 (src_rs_regress 12/12 + [2.5] + [2.6] / align_gate 8/8 /
  jni_method_audit 缺口 0), 设备侧判据维持 PENDING (R15 §4 "权威 5 判据 + 双端真机" 同源,
  非 R17 新增, 设备恢复后跑 `bash rev64/tools/mumu_regress.sh --dev 10.0.182.146:5555`
  + `mumu_align_smoke.sh` 权威 5 判据 (装/board=OK/movetest 3/3/退出/重进) 收口)
- **入口固化**: mumu_regress.sh DEV 默认 = 10.0.182.146:5555 (R9-G-34 全工具链同源, 勿改)

### P2-2 epoch 域限承载位改造 (非阻塞观察项)
- UNDO ref_lo 16 位域限: 典型对局 <10K 帧安全, 超长会话才需扩 ref 高位 (R15 定案非阻塞)
- **登记口径**: 若出现超长会话需求 (单局 >65535 UNDO 帧) 再启动; 当前 60 帧对账 (R15 续弈)
  已覆盖常规断线场景, 不主动改

## 台账状态

- 总条目: P0×3 + P1×5 + P2×2 = 10 条
- 已修: **8** (P0×3 + P1×5, 全回归 12/12 + 2.5/2.6 PASS 实锤)
- 设备降级 PENDING: **1** (P2-1 权威 5 判据 + 双端真机, 10.0.182.146 离线, 桌面门禁已全绿兜底)
- 定案非阻塞: **1** (P2-2 epoch 域限, 超长会话才启动)
- 撤回: 0
- P0: 3 (全修), P1: 5 (全修), P2: 2 (1 设备 PENDING + 1 非阻塞观察)

## 门禁基线 (R17 闭环, 2026-11-17 桌面实测)

- src_rs_regress.sh **12/12 + [2.5] + [2.6] 全 PASS** (P0 修复后实跑)
- jni_method_audit: 无异常清理 0
- align_gate 8/8 + evverify **172/172** (事件 40/40 + 几何 132/132, SUMMARY 动态真值) + dark_align **86/86**
- bt_semtest **152/152** + bt_semtest_dual **43/43** (★ 本轮起挂 [2.5]/[2.6], 不再是门禁外断言池)
- 设备冒烟 (mumu_regress 9 事件 + 权威 5 判据): **PENDING** (10.0.182.146 离线, P2-1)
