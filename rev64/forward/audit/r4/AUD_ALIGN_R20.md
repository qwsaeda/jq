# AUD_ALIGN_R20: P2 边角修复 + 全门禁基线复跑
# 生成: 2026-11-17
# 承接: R19 台账 P2 登记项 (SPEC L106 前缀误导 / arm32_hook_check exit 钳位)

## P2 (★ 已修)

### P2-1 arm32_hook_check.sh:434 `exit "$FAIL"` >255 低 8 位归零 (R19 登记)
- **根因**: 直接 exit FAIL 计数, 若未来 KINDS 扩 >255 项全 FAIL → exit 255 →
  调用方 `[ $? -eq 0 ]` 仍误判 (bash 退出码仅 8 位)
- **修**: 末行改 `[ "$FAIL" -eq 0 ] && exit 0 || exit 1` 钳位 (PASS=0/FAIL>0 二值)
- **实锤**: bash -n 语法 OK; 现行 4 项无行为变化 (FAIL=0 → 0, FAIL>0 → 1)

### P2-2 SPEC_BUSINESS.md:106 "§AGENTS.md 红线" 前缀误导 (R19 登记)
- **根因**: AGENTS.md 无 § 章节体系, "§AGENTS.md" 写法暗示章节号, 读者照 § 找不到锚点
- **修**: 改 "AGENTS.md 顶层红线" + R20 注 (明确无 § 章节)

## P2 维持 (非阻塞, 登记)

### P2-3 设备 8 步全链 (R17 P2-1 同源)
- **复测 (2026-11-17)**: `adb connect 10.0.182.146:5555` → "No route to host", 仍离线
- **口径维持**: PENDING 非 FAIL (§124y-189); 桌面门禁全绿兜底 (见下方基线)
- **恢复后收口入口**: `bash rev64/tools/device_regress_check.sh` (基准 2bf57a5b 已同步, R19 P1-8)
  + `bash rev64/tools/mumu_regress.sh --dev 10.0.182.146:5555` + `mumu_align_smoke.sh` 权威 5 判据
- **R20 加固 (R19 P1-1 后)**: 设备离线跑 mumu_regress 现自动 rc=3 PENDING 提前终止 (非 flaky 误标),
  离线态误报风险已消除, PENDING 收口只待设备恢复

## 台账状态
- 总条目: P2×2 已修 + P2×1 维持登记 = 3 条
- 已修: 2, 维持登记: 1 (设备离线)
- 撤回: 0

## 全门禁基线 (R20 复跑, 2026-11-17)
- align_gate **8/8 PASS** (evverify 172/172 / 事件表 0 缺口 / 布局 Δ=0 / dark_align 86/86 /
  走子桥 8 步 / 64 位探针离线 / 事件名 73 逐名 / sfx 14 WAV)
- src_rs_regress **12/12 + [2.5] + [2.6] 全 PASS** (红线审计 / GL-UND / evverify "全对" 三 ★)
- arm32_static_evidence **73/73 全 PASS** (R19 判据漂移同步后维持)
- make so 0 新增告警 (R18 P0-1/P0-2 渲染修复后)
- 设备冒烟: PENDING (10.0.182.146 离线, P2-3)
