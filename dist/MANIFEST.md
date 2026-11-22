# dist/ 交付物清单 (MANIFEST)
# 生成: 2026-11-17 (AUD_ALIGN_R21 P2-1)
# 用途: 设备侧三方哈希核对 (REAL_DEVICE_ACCEPTANCE SOP §3.1 口径)

## 4 阶段 apk

| 阶段 | 包名 (v 版本) | md5 | so 构成 | 双签 |
|------|--------------|-----|---------|------|
| phase0 | com.cnvcs.junqi32 (v1.78) | f47d204dbb424aeaf7b7eb31e5590fe0 | armeabi 原版 32 位 so 1435156B STORE, 无 arm64 | v1+v2+v3 ✓ |
| phase1 | com.cnvcs.junqi32 (v1.78) | 726264f13dde6b1f964869b89c0030fb | + arm64 4.8KB 桩 | v1+v2+v3 ✓ |
| phase2 | com.cnvcs.junqirs (v1.6.8) | e399b49502f14ab912ed244905261409 | armeabi-v7a 32 位 + arm64 so（2026-11-22 四次重建：+ **28 WAV 改 STORE**（RJNI-001 openFd 崩溃根治）；JNI cnvcs 前缀由 build_phases 内建产出（根治反复覆盖回归）；targetSdk 34） | v1+v2+v3 ✓ |
| phase3 | com.cnvcs.junqirs (v1.6.8) | 98f56e7fcc4434289da2f063fa57ee24 | 同 phase2 + 去广告终态（WAV STORE + JNI 前缀 + targetSdk 34 同步对齐） | v1+v2+v3 ✓ |

## so 基准
- 32 位翻译体 so: `rev64/dist/libjunqi_arm32_aligned.so` md5 = **d8c98fd9baf61f5a6efff530d17b77ee**（★ R24: ExceptionClear 修复重编同步）
  (★ R23-P1: P1 批次修复 (OOME 守卫/flip 界检/n 语义/注释漂移) 重编 2026-11-17, 旧基准 790fb19a/2bf57a5b/25667502/59465d97 全过期)
- 64 位 so: `rev64/forward/src_rs/dist/libjunqi_rs.so` (R18 渲染修复后, make so 0 告警)

## 包名口径注 (★ R21 P1-1 登记)
- 4 阶段 apk 包名 = **com.cnvcs 旧口径** (dec_dual 树, 演进链 `com.cnvcs → com.zxj → com.qwsaeda` 的起点)
- 红线规范: 逆向 app = com.zxj.junqirs / 正向 app = com.qwsaeda.junqirs
- dec_rs 树已演进 com.qwsaeda, 但 4 阶段交付物走 dec_dual 未同步 → **正向会话承接全链 4 处
  包名改动 (smali/manifest/native FindClass/JNI 导出符号) + 重打 + 验签**, 本清单随之刷新

## 验签口径
- 重签工具: `rev64/sign_apk.sh` (v1+v2 双签, junqi.keystore) / apksigner (NDK build-tools)
- 验证: `apksigner verify --verbose dist/phaseX.apk` 须 v1+v2 = true
- STORE 三件套: .so/.arsc/.png 不压缩 + resources.arsc 4 字节对齐 (zipalign -p 4)

## 真机五判据实证 (2026-11-22, 设备 10.0.182.152:5555 恢复后)
- 设备身份: Xiaomi 23127PN0CC / Android 15 / abilist=x86_64,arm64-v8a,x86 (无 32 位 ARM)
- phase3.apk (md5 ba5322b3, targetSdk34, cnvcs so) 装机 Success + 五判据全 PASS:
  ① 装得上 ✓ (streamed install Success) ② 进主界面 ✓ (JNI_OnLoad 成功 + board=OK 全纹理 OK + 出屏主路 eglSwapBuffers 激活 + 截图 900×1600 非黑 99.0% 铺满无黑边)
  ③ 下一局 ✓ (jq.movetest 3/3: Junqi.Move×2 + Junqi.Dead×1, 复盘 n=3) ④ 正常退出 ✓ (force-stop 0 崩溃) ⑤ 重进不崩 ✓ (pid 19457 + board=OK + 全链 0 Fatal/0 UnsatisfiedLink)
- 意义: 本轮四阶段交付物整条修复链 (arm64 so 陈旧 → JNI 前缀断裂 → targetSdk 漂移) 真机闭合；cnvcs so 前缀 + dex 一致 = JNI_OnLoad 真机实锤，targetSdk34 在 Android 15 装机/运行无回归
- 旧设备 10.0.182.146 → 152 IP 漂移 (MuMu 实例重启), 历史 146 引用 = 环境史存档

### 四阶段真机装机口径全验（2026-11-22, 152）
- **phase3** ✓ arm64 真 so，五判据全 PASS（见上，当前设备已恢复装回 phase3）
- **phase2** ✓ arm64 路径五判据全 PASS（pid 20118→20250，board=OK + movetest 3/3 + 退出/重进 0 崩溃；armeabi-v7a 32 位在 152 不被选用，走 arm64）
- **phase0** 预期内装不上：纯 armeabi 32 位 so，152 abilist=x86_64,arm64-v8a,x86 无 32 位 ARM → INSTALL_FAILED_NO_MATCHING_ABIS（AGENTS 已记"64 位 only 设备装不上"）
- **phase1** 预期内装得上但跑不起来：arm64 4.8KB 桩 → 装机 Success 但启动即退（pid 空 + board=OK×0），AGENTS"装得上但桩跑不起来（阶段 2 入口）"实锤
- 结论：四阶段在 152（无 32 位 ARM 的 64 位 only 设备）上的表现与 AGENTS 阶段定义**逐条吻合**，无偏差；phase2/3 真 so 均可玩，phase0 装不上/phase1 桩跑不起来均为预期内诚实边界

### 全链防抖复验（2026-11-22，设备 152 在线）
- 真机五判据各连跑 3 轮：**com.zxj.junqirs 3/3 轮 PASS**（pid 30157→30273→30384→30496→30606→30720，board=OK + movetest 3/3 + 0 崩溃）+ **phase3/com.cnvcs.junqirs 3/3 轮 PASS**（pid 30841→31408），共 6 轮 0 flaky
- 零设备段全 PASS：渗透轮 `fuzz_all --quick` A/B/C/C2/C3/C4/C5 全通 + 四阶段完整性门禁 7 维度（JNI/minSdk-targetSdk/STORE-4B/sign/cert/ad）+ 32 位双端总入口（静态 74 + 渗透轮 + P9 dispatch，设备正确 PENDING）
- 判据：全链防抖无抖动，6 轮真机 + 三套零设备判据全绿；设备侧无 32 位 ABI 属环境约束（两入口已加 ABI 预检正确降级，非 so 回归）
