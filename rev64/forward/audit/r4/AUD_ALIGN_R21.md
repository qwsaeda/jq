# AUD_ALIGN_R21: 交付物 (dist 4 阶段) 审计 + 包名口径漂移登记
# 生成: 2026-11-17
# 范围: 前 4 轮未查区 (dist 交付物 / 阶段 0-3 apk / BtSpp 接线 / 包名规范对照)

## 已核对无问题 (实锤)

1. **dist/ 4 阶段 apk 全量在位 + so 同源** (本轮重建):
   - phase0/1/2 此前**缺失** (11-16 只留 phase3), 本轮 `build_phases.sh` 重建 (幂等,
     构建前强制 `make so` 防旧 so 打交付物 —— §13 门禁在位)
   - 4 apk md5 与 rs_a64/*_signed.apk **逐一同源** (phase0 f47d204 / phase1 726264f /
     phase2 9ae5234 / phase3 e5687c8)
2. **双签验证 4/4 PASS**: apksigner verify 全 apk v1+v2+v3 = true (v4 false 为预期,
   无 v4 签名)
3. **阶段结构正确**:
   - phase0: 32 位 so (armeabi libjunqi.so 1435156B STORE) 无 arm64 (预期)
   - phase1: + arm64 4.8KB 桩 (预期)
   - phase2/3: arm64 = src_rs so (R18 渲染修复后重编, 0 告警)
4. **BtSpp.java 帧重组防悬挂在位**: 32KB 缓冲上限 + "半帧悬挂增长→清空" 判据
   (bt_semtest 152 条 + bt_semtest_dual 43 条覆盖, [2.5]/[2.6] 挂 12/12 回归)

## P1 (★ 包名口径漂移, 登记不擅改)

### P1-1 dist 4 apk 包名 = com.cnvcs.* (旧口径), 红线规范 = com.zxj / com.qwsaeda
- **现状实测**:
  - phase0/1: `com.cnvcs.junqi32` v1.78
  - phase2/3: `com.cnvcs.junqirs` v1.6.8
  - 构建树 dec_dual = `com.cnvcs.junqirs` (build_phases 4 阶段源树)
  - dec_rs = `com.qwsaeda.junqirs` (★ 正向树已演进, 但 4 阶段交付物走 dec_dual)
- **红线口径 (AGENTS 包名规范)**: 逆向 app = `com.zxj.junqirs` (逆军棋) /
  正向 app = `com.qwsaeda.junqirs` (新军棋), 演进链
  `com.cnvcs.junqirs → com.zxj.junqirs (逆向) → com.qwsaeda.junqirs (正向)`
- **根因判断**: 4 阶段交付物 (dist/phase0-3) 打包自 dec_dual 旧树, 包名停在演进前
  "com.cnvcs" 阶段; dec_rs 已演进 com.qwsaeda 但 4 阶段链未同步。属**包名演进
  半途状态**, 非构建缺陷
- **不改原因 (红线约束)**: 包名改动 = 全链 4 处 (smali 类包/manifest/native FindClass/
  JNI 导出符号 Java_com_<pkg>_App_*), 漏一处即 UnsatisfiedLinkError (历史实锤踩坑)。
  且本会话 = 逆向驱动 (com.zxj 域), 4 阶段交付物演进 (com.qwsaeda 域) 归正向会话,
  不抢活。**登记 R21, 正向会话承接时全链 4 处同改 + 重打 4 apk + 验签**
- **对交付的影响 (诚实口径)**: 当前 4 apk 可装可玩 (so 同源 + 双签 PASS), 包名与
  设备已装的 com.zxj/com.qwsaeda 并存不覆盖 (AGENTS 三包并存口径), 无功能阻塞

## P2 (登记)
- P2-1 dist/ 只有 apk 无哈希清单文件 (so 基准 2bf57a5b 仅存于 device_regress 判据),
  建议生成 `dist/MANIFEST.md` (4 apk md5 + 包名 + 双签态 + so 同源注) 供设备侧三方核对
  (与 REAL_DEVICE_ACCEPTANCE SOP §3.1 三方哈希口径对齐)

## 台账状态
- 总条目: 无问题 4 + P1×1 + P2×1 = 6 条
- 已修: 1 (dist 4 阶段 apk 补齐 + so 同源/双签实锤)
- 登记 (正向承接): 1 (包名演进全链 4 处, 不擅改)
- 登记 (非阻塞): 1 (dist MANIFEST.md)
- 撤回: 0

## 门禁基线 (R21)
- 沿用 R20: align_gate 8/8 + src_rs_regress 12/12 + [2.5] + [2.6] + 静态佐证 73/73
- 新增: dist 4 apk 双签 v1+v2+v3 = true ×4 + so 同源 ×4
