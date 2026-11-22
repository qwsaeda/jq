# dist/ 交付物清单 (MANIFEST)
# 生成: 2026-11-17 (AUD_ALIGN_R21 P2-1)
# 用途: 设备侧三方哈希核对 (REAL_DEVICE_ACCEPTANCE SOP §3.1 口径)

## 4 阶段 apk

| 阶段 | 包名 (v 版本) | md5 | so 构成 | 双签 |
|------|--------------|-----|---------|------|
| phase0 | com.cnvcs.junqi32 (v1.78) | f47d204dbb424aeaf7b7eb31e5590fe0 | armeabi 原版 32 位 so 1435156B STORE, 无 arm64 | v1+v2+v3 ✓ |
| phase1 | com.cnvcs.junqi32 (v1.78) | 726264f13dde6b1f964869b89c0030fb | + arm64 4.8KB 桩 | v1+v2+v3 ✓ |
| phase2 | com.cnvcs.junqirs (v1.6.8) | 5105335d95378701976c0dd775d02470 | armeabi-v7a 32 位 + arm64 src_rs so（2026-11-22 重建：arm64 so 691808B = 当前源码 05ce20b0，旧 so cf4b2513 已淘汰） | v1+v2+v3 ✓ |
| phase3 | com.cnvcs.junqirs (v1.6.8) | a25ffa4a33d1c0f015491956784173a0 | 同 phase2 + 去广告终态（2026-11-22 同步重建） | v1+v2+v3 ✓ |

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
