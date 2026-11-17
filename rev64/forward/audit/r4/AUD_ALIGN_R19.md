# AUD_ALIGN_R19: 工具链假绿防护 + 判据漂移同步
# 生成: 2026-11-17
# 承接: R18 台账 "R19 输入 = 工具链 5 项" + 静态佐证 4 项 FAIL 根因治理

## P1 (★ 已修, 实锤)

### P1-1 mumu_evtest.sh 设备离线误判 (管道吞码 + 无前置门禁)
- **根因**: 设备离线时 `adb logcat -d` 全失败 → wait_log 轮询 40s 全 miss → 误判 "app 无响应",
  调用方 mumu_regress 重跑仍 FAIL 误标 flaky/回归 (实为环境断连)
- **修**: 头部加 `timeout 10 adb shell true` 可达性前置门禁, 离线 → **rc=3 PENDING**
  (区分判据 FAIL rc=1); mumu_regress.sh 对 rc=3 提前汇总退 3 (§124y-189 降级口径, 后续项跳过)
- **实锤**: bash -n 语法 OK; 离线态 rc=3 路径

### P1-2 arm32_verify_all.sh `--static-only` SKIPPED 无退出码区分 (静默降级)
- **根因**: SKIPPED/PENDING 全走 `exit 0` (脚本原本无末行 exit), 调用方把 "--static-only 产物" 当 "双端全 PASS" 误报
- **修**: 末行 case 加退出码分级: 静态 FAIL → rc=1 / PENDING → rc=0 / **SKIPPED → rc=2** / 设备 FAIL → rc=1
- **实锤**: `--static-only rc=2` / `full (设备离线自动 PENDING) rc=0` 双路径实跑

### P1-3 pack_arm32.sh:58 验签判据恒真 (同型 sign_apk.sh D6-#20 已修未同步)
- **根因**: 裸 `grep -E "Verified|Number"` 命中 "Verified using v1 ...: false" 也 PASS
- **修**: 先落日志再 `grep -E "Verified using v[0-9] scheme .*: true"` 锚定 + apksigner rc 显式取,
  非 0 → exit 1; 无 true 行 → exit 1 (双判据)

### P1-4 so_probe.py / thunk_code_map.py 默认 so 路径无存在性守卫 (判据恒真)
- **根因**: `/tmp/orig/` 被清空后 so_probe 抛 FileNotFoundError Traceback (上游误判 "工具崩溃");
  thunk_code_map objdump 报 'bad file' → mapping={} → --check 误报 "0 个 handler" 假绿
- **修**: 两脚本 main() 前置 `os.path.exists(SO)` 守卫, 缺 so → **return 1** 判据码 + 提示设 JQ_SO

### P1-5 arm32_static_evidence.sh 68 处 "判据脚本缺失→PASS+1" 静默降级 (模式治理)
- **根因**: 权威基准 "静态 73 项全 PASS" 下, 任一判据脚本误删/改名 → 假 PASS 且 FAILED 空 → 污染基准
- **修**: 68 处 else 分支全改 "缺失 → FAIL+1 + FAILED 登记" (CODE_STYLE §9 假绿三型之 "缓存缺失禁映射 PASS" 落地)
- **实锤**: 改后实跑 `PASS=73 FAIL=0` (判据脚本全在位, 无回归)

## P1 判据漂移同步 (★ 已修, 静态佐证 4 项 FAIL 根因闭合)

### P1-6 bt_undorestart_check.py ② 判据与 R8-C-16 新语义矛盾 (真回归, 非误报)
- **根因**: ② 仍锚 "DISCONNECTED **拒收** RESTART rc=-2" (B04-012 旧全拒收语义),
  R17 P0-2 已把 bt_selftest.c 断言改 "DISCONNECTED **放行** RESTART rc=0" (R8-C-16 会话恢复)
  → 判据 grep 恒 miss → 假红
- **修**: ② 判据组改 "OUR_TURN 拒收 UNDO + DISCONNECTED 拒收 UNDO 数据帧 + R8-C-16 放行 RESTART" 三联
- **实锤**: `python3 bt_undorestart_check.py` → ②③④ 全 PASS

### P1-7 event_name_static_check.py ③ 假 FAIL (norm 单向归一)
- **根因**: §124y-187 模板串口径只归一 so 侧 (Junqi.GameType%d → Junqi.GameType),
  map 侧存模板形态 "Junqi.GameType%d" 未归一 → 同一串双向误报 (缺=GameType 多=GameType%d)
- **修**: norm 双向对称 (so_norm + map_norm 同去 %d)
- **实锤**: `python3 event_name_static_check.py` → 73/73 全对照 PASS

### P1-8 device_regress_offline_check.py ① so md5 基准过期 (假红)
- **根因**: 基准 `25667502` = 旧翻译体 so; 2026-11-16 重编后 dist so = `2bf57a5b`,
  判据恒 FAIL (非代码缺陷, 基准未随重编同步)
- **修**: ① BASELINE_MD5 更新 `2bf57a5b` + 注释 "随 so 重编同步, 否则恒 FAIL 假红";
  ①b 新增 **apk 内 so 提取三方一致性核对** (原只打 apk 整包 md5 无基准可对照, 改 zipfile 提取
  lib/armeabi/libjunqi.so 算 md5 与 dist so 直接对); ⑤ 同步 device_regress_check.sh SO_MD5_EXPECT
- **实锤**: `① dist so 哈希 = 基准 OK + apk 内 so 同源 OK` → 5 项全 PASS

### P1-9 alignment_status_check.py ④ "设备侧 8 步全链未过 (rc=2)" 级联假红
- **根因**: ④ 实跑 device_regress_offline_check.py, R18 改 jni_bridge.c 后二进制过期 →
  该判据 ⑤ 段 grep 旧基准 md5 miss → rc≠0 → ④ 级联 FAIL (PENDING 口径误判未过)
- **修**: 随 P1-8 基准同步自然解除; ④ 现 PASS ("真机 PENDING §124y-189" 口径正确, 非 FAIL)
- **实锤**: 静态佐证全跑 **PASS=73 FAIL=0** (4 项 FAIL 全闭合)

## P2 (登记, 非阻塞)
- SPEC L106 "§AGENTS.md 红线" 前缀误导 (登记, 低危)
- arm32_hook_check.sh:434 `exit "$FAIL"` >255 低 8 位归零 (当前 4 项无影响, 钳位登记)
- 设备恢复后跑 `device_regress_check.sh` 实锤 8 步全链 (PENDING, 同 R17 P2-1)

## 台账状态
- 总条目: P1×9 (5 防护 + 4 漂移同步) + P2×3 = 12 条
- 已修: **9** (P1 全修, 实锤全 PASS)
- PENDING: 1 (设备 8 步全链, 离线)
- 登记: 2
- 撤回: 0

## 门禁基线 (R19 闭环, 2026-11-17 桌面实测)
- arm32_static_evidence.sh **73/73 全 PASS** (R18 后 4 项 FAIL 全闭合)
- arm32_verify_all.sh --static-only **rc=2** (SKIPPED 分级实锤) / 设备离线自动 PENDING rc=0
- bt_undorestart_check / event_name_static_check / device_regress_offline_check 3 判据 PASS
- 正向侧沿用 R18 基线: 全回归 12/12 + [2.5] + [2.6] / align_gate 8/8 / evverify 172/172
