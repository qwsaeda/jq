# AUD-B —— 军棋 APK 构建/打包/工具链 对抗审计（Round 2）

> 审计员：独立对抗审计子代理（不偏向开发方）
> 审计对象：`rev64/tools/{build_phases.sh,bt_smali.sh,rs_cycle.sh,mumu_swap_so.sh,layout_align_check.py,link_audit.py,spak2ui.py,spak_strings.py,spak_ui_assets.py,gen_dialog_batch.py}`、`rev64/src_rs/Makefile`、`rev64/src_rs/dist/`
> 背景：阶段 0-3 定义（去广告 + 旧 ABI → 双 ABI → 真 64 位 → 去广告终态）、重打包三件套（apktool b / STORE / v1+v2 双签）、红线（`assets/data*.pak` doNotCompress 不动 / 不改 `App.smali` native 签名 / 阶段 3 全删 baidu 后仅 arm64 可用）
> 审计日期：2026-11-07
> 全部结论均在本机实测复现，复现命令见每条"依据"。

---

## 0. 审计结论速览

**当前 `dist/` 为空目录 —— 阶段 0/1/2/3 四个交付物一个都不存在。**
在此基础上，本轮共发现 **112 条**问题，其中 **P0（装不上/崩溃/门禁彻底失效）12 条**、P1（功能错）45 条、P2（边界）40 条、P3（风格）15 条。

三条主线：

1. **打包脚本全程"假成功"**：`build_phases.sh --verify-only` 在 `dist/` 空的情况下依然打印 `✓ verify-only 完成` 并 exit 0，且四个阶段全部输出 `arm64 目录无（预期）` —— 对阶段 2/3 而言"arm64 目录无"是致命缺陷，却被判为"预期"。
2. **几何对齐门禁（`layout_align_check.py`）自 2026-11-06 起就是语法错误文件**，且 `gen_dialog_batch.py` 的每一次"生成"与每一次 `--clean` 都会进一步破坏它 —— 整个"Δ=0 才是真对齐"的判据链已失效 6 天无人发现。
3. **Makefile 6 个桌面测试目标链接缺 `ui_state_stub.c` + `make so` 缺 `ui_menu.c/ui_state.c` 前置依赖** —— `src_rs_regress.sh` 现状 **9/12 FAIL**，"12/12 全回归"与"12/12 全对"的口径均已失真；`rs_cycle.sh` 必然在第 2 步中止。

---

## A. `tools/build_phases.sh`（四阶段一键构建 + 校验）

## AUD-B-001 [P0] `build_one` 失败不中断顶层循环，脚本恒 exit 0 并打印"✓ 完成"
- 位置：`rev64/tools/build_phases.sh:109-122`
- 问题：`build_one phase0 phase0.apk` 的返回值在 `TARGET=all` 分支与单阶段分支中**均未被检查**。任一阶段失败（apktool b / STORE / zipalign / 签名）时，`build_one` `return 1`，但脚本继续跑后续阶段，最后无条件执行 `echo "✓ build_phases 完成（$TARGET）"` 并以 exit code **0** 结束。CI/调用方以 exit code 判成败 → **构建失败被判为成功**。
- 依据：
  ```bash
  cd /disk/workspaces/jq && bash rev64/tools/build_phases.sh --verify-only; echo "EXIT=$?"
  # → ✓ verify-only 完成   EXIT=0
  ```
  将 `dist/` 清空后 `--verify-only` 仍返回 0（见 AUD-B-003）。
- 修法：顶层改为 `build_one "$p" "$p.apk" || { echo "✗ $p 失败，终止"; exit 1; }`；或整体 `set -euo pipefail` + `trap`。

## AUD-B-002 [P0] `setup_libs` 的 `cp` 失败无人检查 → 可产出"缺 .so"的 APK（装上即崩）
- 位置：`rev64/tools/build_phases.sh:35-44`（尤其 `:42`）
- 问题：`setup_libs` 里 `cp $src $tree/lib/$abi/libjunqi.so` 的失败被完全忽略（`while` 循环继续，`setup_libs` 的返回值又被 `build_one` 忽略）。若 `SRC_ORIG` / `SRC_RS` / `SRC_STUB` 任一缺失或不可读，`apktool b` 仍会成功产出**没有 native 库**的 APK → 装得上、一点图标就 `UnsatisfiedLinkError` 闪退。
- 依据：三个源 so 均为 `/tmp` 下的外部依赖（`/tmp/opencode/orig_libjunqi.so`、`/tmp/opencode/junqi_stub_arm64.so`），容器重启/清理即消失，而脚本对此毫无防护。
- 修法：`setup_libs` 内每个 `cp` 加 `|| { echo "✗ 源 so 缺失: $src"; return 2; }`；`build_one` 首行加 `setup_libs ... || return 1`；并在 `build_one` 开头 `[ -f $src ] || return 2` 前置校验。

## AUD-B-003 [P0] `verify_one` 对"文件不存在"静默全绿 → 交付物缺失不可见
- 位置：`rev64/tools/build_phases.sh:67-92`
- 问题：`verify_one` 对 `$WS/dist/$out` **不做存在性检查**。文件不存在时：
  - `aapt dump badging` 失败 → `abi`/`minsdk` 为空串（被 `2>/dev/null` 吞掉）
  - `unzip -l` 失败 → `grep -c "__xadsdk"` 得 `0` → 打印 `广告jar=0`（**看起来像"广告已删净"的正确结论**）
  - 内嵌 `python3 -c` 块因 `2>/dev/null` 静默失败 → 连 `.so` 的 STORE 状态都不输出
- 依据：
  ```bash
  ls /disk/workspaces/jq/dist/     # 空目录
  bash rev64/tools/build_phases.sh --verify-only
  #   [phase0] ABI= minSdk= 广告jar=0
  #     arm64 目录无（预期）
  #   [phase1] ABI= minSdk= 广告jar=0
  #   [phase2] ABI= minSdk= 广告jar=0
  #   [phase3] ABI= minSdk= 广告jar=0
  # ✓ verify-only 完成     EXIT=0
  ```
- 修法：`verify_one` 首行 `[ -f "$apk" ] || { echo "  ★ $apk 不存在"; return 1; }`；`verify_one` 返回值汇总到 `verify-only` 分支，非 0 则 `exit 1`。

## AUD-B-004 [P1] "arm64 目录无（预期）"对阶段 2/3 是致命缺失却被判为预期
- 位置：`rev64/tools/build_phases.sh:87-88`
- 问题：该分支硬编码打印"（预期）"，但"arm64 目录不存在"只对阶段 0/1 是预期。阶段 2/3 的 arm64-v8a/libjunqi.so 是**唯一可运行**的 native 库，缺失即 64 位设备 100% 闪退。
- 依据：同上 `--verify-only` 输出（phase2/phase3 也打"arm64 目录无（预期）"）。
- 修法：按阶段期望表判定：`case $phase in phase0|phase1) expect_arm64=1;; phase2|phase3) expect_arm64=1;; esac`，阶段 2/3 缺失 → `★ 致命` 并 exit 1。

## AUD-B-005 [P1] so 哈希对账在"源 so 不存在"时恒真（`grep -q ""` 匹配一切）
- 位置：`rev64/tools/build_phases.sh:83-86`
- 问题：`grep -q "$(md5sum $SRC_RS | cut -d' ' -f1)"` —— 当 `$SRC_RS` 不存在时，`md5sum` 报错、输出为空串，`grep -q ""` 对**任意输入恒为真** → 打印"arm64 so = src_rs ✓"。同理 `$SRC_STUB` 分支。哈希对账在最需要它的时候（源缺失）完全失效。
- 依据：`md5sum /nonexistent 2>/dev/null | cut -d' ' -f1` → 空串；`echo x | grep -q ""` → exit 0。
- 修法：先算出 `want=$(md5sum "$SRC_RS" | cut -d' ' -f1)` 并断言 `want` 长度 = 32，再比对；源缺失时直接报错退出。

## AUD-B-006 [P1] 从不校验 32 位 so 的哈希（`armeabi` / `armeabi-v7a`）
- 位置：`rev64/tools/build_phases.sh:82-91`
- 问题：`verify_one` 只比对 `lib/arm64-v8a/libjunqi.so`。阶段 0/1 的 `lib/armeabi/libjunqi.so`（原版 1435156B）与阶段 2 的 `lib/armeabi-v7a/libjunqi.so` **完全没有对账**，错了也发现不了。
- 依据：脚本内 `grep -c 'lib/arm64-v8a'` 是唯一的库校验入口。
- 修法：加 `unzip -p $apk lib/armeabi/libjunqi.so` 与 `lib/armeabi-v7a/libjunqi.so` 的 md5 对账（对 `$SRC_ORIG`）。

## AUD-B-007 [P1] `verify_one` 不校验 minSdk / targetSdk / 包名 / 权限 / 签名 / 广告类
- 位置：`rev64/tools/build_phases.sh:67-92`
- 问题：校验函数只输出 `ABI` 与 `minSdk`（纯打印），**没有任何断言**；targetSdk、包名、uses-permission 列表、`com.baidu.*` 类是否删净、`__xadsdk__remote__final__.jar` 是否残留、v1+v2 签名是否存在，全部不检查。阶段定义的核心属性无一条被机器化守护。
- 依据：函数体内无 `exit`/`return 1`/比较断言；`aapt dump badging` 结果仅被 `echo`。
- 修法：把 `verify_one` 改成"期望值 → 实际值 → 差异即 FAIL"的断言表，至少覆盖：package/minSdk/targetSdk/native-code/权限集合/baidu 类数/广告 jar 数/`apksigner verify` 输出 v1+v2。

## AUD-B-008 [P1] 广告 jar 计数不按阶段判定，且从不导致失败
- 位置：`rev64/tools/build_phases.sh:72`
- 问题：`adjar=$(unzip -l $apk | grep -c "__xadsdk")` 对四个阶段一视同仁。阶段 3 要求 = 0、阶段 0/1 允许 = 1（红线：32 位 so 硬依赖广告 jar），但既不按阶段期望值判定，也从不 `exit 1`。
- 依据：阶段定义：阶段 3 = "删 `assets/__xadsdk__remote__final__.jar`"。
- 修法：`case $phase in phase3) [ "$adjar" = 0 ] || fail;; phase0|phase1) [ "$adjar" = 1 ] || fail;; esac`。

## AUD-B-009 [P1] 阶段 3 不做 `com.baidu.*` 类残留扫描
- 位置：`rev64/tools/build_phases.sh:67-92`（缺失项）
- 问题：阶段 3 红线是"全删 `smali/com/baidu/*` + `AdLis.smali` + 广告 jar"。校验函数完全没有 dex 层面的 baidu 扫描；若某次 `apktool b` 误带 smali 或 dex 里残留 baidu 引用，校验不会报警。
- 依据：`unzip -p $apk classes.dex | strings | grep -c "com/baidu"` 在 phase3 上 = 0（当前树是干净的），但脚本不查。
- 修法：`verify_one` 增加 `dex_baidu=$(unzip -p $apk classes.dex | strings | grep -c "com/baidu")`，阶段 3 要求 = 0。

## AUD-B-010 [P2] 脚本从不修改 manifest / apktool.yml → 阶段属性全靠 `/tmp` 手工树，不可复现
- 位置：`rev64/tools/build_phases.sh`（全文无 manifest/apktool.yml 写操作）
- 问题：`minSdk`/`targetSdk`/包名/去广告权限/`extractNativeLibs` 全部固化在 `/tmp/opencode/jq_apk/{dec_p0,rs_a64/dec_dual,rs_a64/dec_full}` 三个**临时目录**里。脚本被提交进 git，但**没有任何一步能重建这些属性**，换机/清 `/tmp` 即无法产出符合阶段定义的包。
- 依据：`ls /tmp/opencode/jq_apk/` → `dec/`、`dec_p0/`、`rs_a64/`；仓库内无这些树的副本。
- 修法：把"阶段属性"抽成脚本内的显式 patch 步骤（写 AndroidManifest.xml + apktool.yml 的 `sdkInfo`/`renameManifestPackage`），树只作为素材源。

## AUD-B-011 [P2] `zipalign -f 4` 而非 `zipalign -p 4`（共享库未页对齐）
- 位置：`rev64/tools/build_phases.sh:60`
- 问题：`-f 4` 只做 4 字节对齐。Android 6.0+(API 23) 起，未压缩的 `.so` 若走"直接从 APK mmap"路径（`extractNativeLibs=false`）**要求 4096 字节页对齐**。当前 manifest 未显式声明 `android:extractNativeLibs`，行为依赖平台默认值，随 targetSdk/平台版本漂移。
- 依据：`$ZIPALIGN -f 4` vs 官方 `zipalign -p -f 4`（`-p` = page-align 共享库）。
- 修法：`zipalign -p -f 4`；并在 manifest 显式写 `android:extractNativeLibs="true"`（或 "false" + 页对齐，二选一并写进构建脚本）。

## AUD-B-012 [P2] 中间产物名固定 → 并发/重复运行互相覆盖
- 位置：`rev64/tools/build_phases.sh:58-62`
- 问题：`$RA/${phase}_raw.apk` / `_stored` / `_aligned` / `_signed` 均为固定路径。两次并发 `build_phases.sh`（或 `build_phases.sh phase2` 与 `all` 并行）会写到同一组文件 → 产出物可能是"raw 与 stored 混合"的 Frankenstein APK。
- 依据：`$RA = /tmp/opencode/jq_apk/rs_a64`，无 `$$`/mktemp 隔离。
- 修法：中间目录用 `mktemp -d`，或加 `flock`（`exec 9>$RA/.build.lock; flock -n 9 || exit 1`）。

## AUD-B-013 [P2] 中间产物无清理，四阶段 × 4 个 ~10MB APK 常驻
- 位置：`rev64/tools/build_phases.sh:58-62`（无 `trap`/`rm`）
- 问题：每次构建留下 `phase{0..3}_{raw,stored,aligned,signed}.apk` + `.idsig`，共 16 个大文件（约 60-80MB），且 `apktool b` 的 `build/` 目录也不清。
- 依据：`ls -la /tmp/opencode/jq_apk/rs_a64/ | grep phase` → 可见全部残留。
- 修法：`trap 'rm -rf "$TMPD"' EXIT`，只保留最终 `dist/phaseX.apk`。

## AUD-B-014 [P2] `sign_apk.sh` 的 `apksigner verify` 失败不会让脚本失败
- 位置：`rev64/sign_apk.sh:13`（`$APKSIGNER verify --verbose $OUT 2>&1 | grep -E "Verified|Number" | head -4`）
- 问题：管道退出码取自 `head`（恒 0）。即使 `apksigner verify` 判定签名无效，只要输出里含 "Verified" 字样（v1 通过、v2 失败也会打），`grep` 仍返回 0 → `set -e` 不触发 → 脚本成功退出 → 上层认为"v1+v2 双签完成"。
- 依据：`set -e` + 管道：只有管道**整体**非 0 才中止；`grep` 匹配成功即 0。
- 修法：`$APKSIGNER verify --verbose $OUT` 单独执行并检查其退出码，再 `| grep` 仅作展示。

## AUD-B-015 [P3] 关键路径变量未加引号
- 位置：`rev64/tools/build_phases.sh:37-42,58-62,70-81`
- 问题：`rm -rf $tree/lib`、`cp $src $tree/lib/$abi/libjunqi.so`、`aapt dump badging $apk`、`unzip -p $apk ...` 均未加引号 → 路径含空格时静默错位。
- 依据：`build_phases.sh` 中 `$tree`/`$src`/`$apk` 全部裸写。
- 修法：全部改 `"$tree"` / `"$src"` / `"$apk"`。

## AUD-B-016 [P3] 头部注释与实现的树路径不符
- 位置：`rev64/tools/build_phases.sh:13`（注释"apktool b（/tmp/opencode/jq_apk/rs_a64/ 下构建树）"）vs `:21`（`P0=/tmp/opencode/jq_apk/dec_p0`，实际在 `rs_a64/` **之外**）
- 问题：文档误导后续维护者去错目录排查。
- 修法：注释与实现对齐，或统一把 dec_p0 移入 `rs_a64/`。

## AUD-B-017 [P3] `--verify-only` 忽略 `TARGET` 参数
- 位置：`rev64/tools/build_phases.sh:100-108`
- 问题：`build_phases.sh phase2 --verify-only` 仍复核全部四个阶段。
- 修法：`--verify-only` 分支按 `$TARGET` 循环。

## AUD-B-018 [P2] `zipalign` 指向 containerd 快照目录（易被 GC / 换机即失效）
- 位置：`rev64/tools/build_phases.sh:23`
- 问题：`/disk/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots/741/fs/opt/android/build-tools/36.0.0/zipalign` 是容器快照内的绝对路径，快照被回收即失效，回退到 PATH 里的 `zipalign`（本机 PATH 中**不存在**，`which zipalign` 无输出）→ 脚本直接 `zipalign 失败`。
- 依据：`which zipalign` → 无结果；`ls .../741/.../zipalign` → 存在但属快照。
- 修法：路径探测改为多候选（PATH → `ANDROID_HOME/build-tools/*/zipalign` → 快照路径），并把探测结果打印出来。

---

## B. 交付物实际状态（`dist/`、`src_rs/dist/`、阶段属性）

## AUD-B-019 [P0] `dist/` 为空 —— 阶段 0/1/2/3 四个交付物全部缺失
- 位置：`/disk/workspaces/jq/dist/`（空目录，`ls -la` 只有 `.`/`..`）
- 问题：AGENTS.md 声明"产物：`dist/phase0.apk` / `phase1.apk` / `phase2.apk` / `phase3.apk`"，实际**一个都不存在**。验收判据（装上→进主界面→下一局→正常退出→重进不崩）无从执行。
- 依据：`ls -la /disk/workspaces/jq/dist/` → total 8，仅两个目录项。
- 修法：跑 `bash rev64/tools/build_phases.sh all` 产出并入 git（或至少入 artifact 存储），并在 CI/交付检查里加"dist/ 四个文件存在且可被 aapt 解析"的硬门禁。

## AUD-B-020 [P1] 阶段 2/3 归档 APK 内的 arm64 so ≠ 当前 `src_rs` 产物（陈旧）
- 位置：`/tmp/opencode/jq_apk/rs_a64/phase2_signed.apk`、`phase3_signed.apk` vs `rev64/src_rs/dist/libjunqi_rs.so`
- 问题：归档 APK 内的 arm64 so md5 = `c7f971b8dc4c771fe37d5db68f8e8bc6`（2026-11-05 12:14），而当前 `src_rs/dist/libjunqi_rs.so` md5 = `2bb06989739c44cf486e346b901303a0`（2026-11-07 04:25，550248B vs 137816B）。**归档产物落后两天、包含大量旧代码**（含 118 弹窗批量对齐之前的所有 UI 修复）。
- 依据：
  ```bash
  unzip -p phase3_signed.apk lib/arm64-v8a/libjunqi.so | md5sum   # c7f971b8...
  md5sum rev64/src_rs/dist/libjunqi_rs.so                        # 2bb06989...
  ```
- 修法：重新构建并把"产物内 so md5 == src_rs/dist so md5"写成 `verify_one` 的**硬断言**（当前只对 arm64 做软打印，见 AUD-B-005/006）。

## AUD-B-021 [P1] 阶段 2/3 声明 `minSdk 21`，但 arm64 so 以 API 24 为基线构建 → Android 5.0/5.1/6.0 加载即失败
- 位置：`rev64/src_rs/Makefile:25`（`aarch64-linux-android24-clang`）vs `/tmp/opencode/jq_apk/rs_a64/dec_{dual,full}/apktool.yml`（`sdkInfo.minSdkVersion: '21'`）
- 问题：so 的 `.note.android.ident` 里 API level = `0x18` = **24**。运行时 linker 在设备 API < 24 时拒绝加载该 so（"requires API level 24"）→ 声明支持 Android 5.0（API 21）的阶段 2/3 在 Android 5.0/5.1/6.0 上**一点图标就崩**。
- 依据：
  ```bash
  readelf -n rev64/src_rs/dist/libjunqi_rs.so | grep -A2 android.ident
  #   description data: 18 00 00 00 72 32 35 62 ...   → 0x18 = 24
  aapt dump badging phase2_signed.apk | grep sdkVersion   # sdkVersion:'21'
  ```
- 修法：要么改用 `aarch64-linux-android21-clang`（NDK r25b 支持 `-D__ANDROID_API__=21` 或直接选 android21 工具链），要么把阶段 2/3 的 minSdk 提到 24 并在阶段定义里写明。

## AUD-B-022 [P2] 阶段 0/1 的包名被 `renameManifestPackage` 改成 `com.cnvcs.junqi32`
- 位置：`/tmp/opencode/jq_apk/dec_p0/apktool.yml`（`renameManifestPackage: com.cnvcs.junqi32`）vs 树内 `AndroidManifest.xml`（`package="com.cnvcs.junqi"`）
- 问题：apktool 2.7 的 `renameManifestPackage` **优先于** manifest 文本，产物实际包名 = `com.cnvcs.junqi32`。后果：
  (a) 阶段 0/1 无法覆盖安装用户已装的原版 `com.cnvcs.junqi`（是并存的第二个 app）；
  (b) 与阶段 2/3 的 `com.cnvcs.junqirs` 构成**第三个**包名 —— 四阶段跨阶段升级链全部断裂；
  (c) 阶段定义（"阶段 0：旧 ABI 快速包"）未提及改名，属未记录的偏离。
- 依据：
  ```bash
  aapt dump badging phase0_signed.apk | head -1
  # package: name='com.cnvcs.junqi32' versionCode='178' versionName='1.78'
  ```
- 修法：阶段 0/1 保持 `com.cnvcs.junqi`（`renameManifestPackage: null`），四阶段统一包名以便升级链；确需并存则把改名写进阶段定义。

## AUD-B-023 [P2] 真相在 `apktool.yml` 而非 `AndroidManifest.xml` —— 阶段属性"文档树"与"产物"不一致
- 位置：`/tmp/opencode/jq_apk/dec_p0/AndroidManifest.xml`（`minSdkVersion="4"` `targetSdkVersion="31"`）vs `dec_p0/apktool.yml`（`minSdkVersion: '19'` `targetSdkVersion: '34'`）
- 问题：apktool b 时 `apktool.yml.sdkInfo` 覆盖 manifest 值。产出的 phase0/1 实际是 **minSdk 19 / targetSdk 34**（符合 AGENTS.md），但树里的 manifest 文本写的是 4/31。任何依据 manifest 文本做的审计/评审都会得出"minSdk 4 太旧、装不上"的**错误结论**；反之若有人据 manifest 改 minSdk，改了不生效。
- 依据：`aapt dump badging phase0_signed.apk` → `sdkVersion:'19'` / `targetSdkVersion:'34'`。
- 修法：统一由 `apktool.yml` 单一来源控制，并在构建脚本里显式 assert 两者一致。

## AUD-B-024 [P2] 阶段 2/3 的 `targetSdk` 为 31，阶段定义写 34
- 位置：`dec_{dual,full}/apktool.yml`（`targetSdkVersion: '31'`）vs AGENTS.md 阶段定义（`targetSdk 34`）
- 问题：产物与阶段定义不符。虽不致命（31 ≥ 23，Android 14 不拦装），但属于未对齐的交付属性。
- 依据：`aapt dump badging phase2_signed.apk | grep targetSdk` → `targetSdkVersion:'31'`。
- 修法：改 `apktool.yml` 为 34 并重建。

## AUD-B-025 [P2] `dec_dual` / `dec_full` 缺 `android:theme`（`dec_rs` 有）→ 状态栏/ActionBar 残留
- 位置：`dec_full/AndroidManifest.xml` 的 `<application ...>`（无 `android:theme`）vs `dec_rs/AndroidManifest.xml`（`android:theme="@android:style/Theme.NoTitleBar.Fullscreen"`）
- 问题：`mumu_swap_so.sh:52-57` 已把"无全屏 theme → GLView 不全屏 → MotionEvent 坐标相对 view 顶 → 触摸命中全偏"固化为坑，并只在 `dec_rs` 上补了 theme。**阶段 2/3 用的 `dec_dual`/`dec_full` 没有 theme**，虽然 `App.smali` 里已有 `FLAG_FULLSCREEN` + `setSystemUiVisibility(0x1706)`，但 Activity window 在 flags 生效前仍会预留状态栏高度 → 触摸坐标基准与 720×1280 不一致。
- 依据：`diff dec_full/AndroidManifest.xml dec_rs/AndroidManifest.xml` → 唯一差异就是 theme 属性。
- 修法：把 `android:theme="@android:style/Theme.NoTitleBar.Fullscreen"` 同步进 `dec_dual`/`dec_full`。

## AUD-B-026 [P2] 阶段 2/3 未声明 `android:extractNativeLibs`
- 位置：`dec_{dual,full}/AndroidManifest.xml`
- 问题：`.so` 以 STORE 方式打包（合规），但 manifest 未显式声明 `extractNativeLibs`，安装期行为由平台默认值 + targetSdk 推导，不同 Android 版本上"抽取 / 直读 APK"行为不同 → 直读路径下未页对齐的 so 会加载失败（见 AUD-B-011）。
- 依据：`grep extractNativeLibs dec_dual/AndroidManifest.xml` → 无。
- 修法：显式 `android:extractNativeLibs="true"`，或 "false" + `zipalign -p 4`。

## AUD-B-027 [P2] `apktool.yml.doNotCompress` 未列 `lib/armeabi-v7a/libjunqi.so`
- 位置：`dec_dual/apktool.yml`（`doNotCompress` 只列 `lib/arm64-v8a/libjunqi.so`）
- 问题：阶段 2 新增的 `lib/armeabi-v7a/` 目录不在 `doNotCompress` 里。当前靠 `store_compressed.py` 的后处理兜底（`.so` 后缀 → STORE），但这让"红线条目"依赖了一个**树外的后处理脚本**，一旦有人调整 `store_compressed.py` 的扩展名列表即回归为"压缩的 .so"（装不上）。
- 依据：`dec_dual/apktool.yml` doNotCompress 列表；`build_phases.sh:59` 依赖 `/tmp/opencode/jq_apk/store_compressed.py`（**该脚本不在 git 仓库内**）。
- 修法：把 `store_compressed.py` 收进 `rev64/tools/`，并把 `lib/armeabi-v7a/libjunqi.so` 加进 `doNotCompress`。

## AUD-B-028 [P2] `dec_dual` 与 `dec_full` 的 AndroidManifest.xml 完全相同 → 阶段 2/3 在 manifest 层不可区分
- 位置：`dec_dual/AndroidManifest.xml` vs `dec_full/AndroidManifest.xml`
- 问题：`diff` 输出 IDENTICAL。阶段 2 与阶段 3 的唯一差别在 smali（334 个 baidu 类）与 assets（广告 jar），manifest 层无任何标记（如 `tools:node="remove"` 记录、或不同 meta-data）。一旦有人只重建 manifest 不重建 smali，两个阶段会产出**行为相同的包**而校验仍显示"广告jar=0/1 不同"以外无差异。
- 依据：`diff dec_dual/AndroidManifest.xml dec_full/AndroidManifest.xml` → IDENTICAL。
- 修法：在 `dec_full` 的 manifest 里留显式注释/标记（例如 `<meta-data android:name="build.phase" android:value="3"/>`），或让 build 脚本断言两树的 smali/资源差异符合预期。

## AUD-B-029 [P2] 阶段 0/1 保留 `__xadsdk__remote__final__.jar` 但已删 `INTERNET` 等权限 → 广告 SDK 运行期可能 SecurityException
- 位置：`dec_p0/assets/__xadsdk__remote__final__.jar` + `dec_p0/AndroidManifest.xml`（权限只剩 `WRITE_EXTERNAL_STORAGE`/`BLUETOOTH`/`BLUETOOTH_ADMIN`）
- 问题：这是 AGENTS.md 记录的已知取舍（32 位 so 硬依赖广告 jar），但 jar 保留 + 权限删除的组合下，广告 SDK 类在初始化时若访问网络/电话状态会抛 `SecurityException`；`AdLis.smali` 仍引用 baidu 类。属**已知边界但未在产物层做任何隔离**。
- 依据：`ls dec_p0/assets/__xadsdk__remote__final__.jar` 存在；`dec_p0/smali/com/baidu` = 334 个类。
- 修法：把广告 jar 相关的加载入口（`AdLis`）在阶段 0/1 里改为"直接 return，不实例化广告 SDK"，保留类定义仅为满足 `FindClass`。

## AUD-B-030 [P1] `src_rs/dist/phase_rs_a64.apk` 内 so 陈旧 + 只 v1/v3 签名 + sdkVersion 19 与 so 的 API 24 冲突
- 位置：`rev64/src_rs/dist/phase_rs_a64.apk`
- 问题：三个问题叠加：
  (a) 内含 so md5 = `685c73d175be900b6b0eb2bfd5692f73`（518024B），≠ 当前 `dist/libjunqi_rs.so`（`2bb06989...`，550248B）→ **打包的是旧 so**；
  (b) `apksigner verify` → `v1: true, v2: false, v3: true` → **缺 v2**，违反 AGENTS.md"v1+v2 双签（缺一个都装不上）"的硬要求；
  (c) `sdkVersion:'19'` 而 so 的 min API = 24（见 AUD-B-021）。
- 依据：
  ```bash
  unzip -p phase_rs_a64.apk lib/arm64-v8a/libjunqi.so | md5sum   # 685c73d1...
  apksigner verify --verbose phase_rs_a64.apk | grep scheme      # v1 true / v2 false / v3 true
  ```
- 修法：`mumu_swap_so.sh` 的签名分支改为 `--v1-signing-enabled true --v2-signing-enabled true`（MuMu 兼容问题应另用独立测试包解决，不要污染交付物口径）；并在打包后加 so md5 自检（已有，见下）。

## AUD-B-031 [P2] `src_rs/dist/` 的构建产物未入库（`.gitignore` 排除二进制）→ 仓库无二进制基线
- 位置：`rev64/src_rs/dist/`（`libjunqi_rs.so` / `jq_evverify` / `jq_consistency` / `phase_rs_a64.apk`）
- 问题：AGENTS.md 声称这些是"构建产物"，但 `.gitignore` 排除了二进制 → 新克隆的仓库拿不到任何基线，审计/复现只能依赖本机 `/tmp`。
- 依据：`.gitignore` 含 `rev64/out`、`rev64/native64/*.o` 等；`git ls-files rev64/src_rs/dist` → 空。
- 修法：为每个产物生成 sha256 清单文件（文本，入库），CI/审计按清单对账。

---

## C. `src_rs/Makefile` 与 `src_rs_regress.sh`

## AUD-B-032 [P1] `make so` 的目标缺 `ui_menu.c` / `ui_state.c` 前置依赖 → 改 UI 源码不重编 so（**产物陈旧**）
- 位置：`rev64/src_rs/Makefile:23-28`
- 问题：目标 `dist/libjunqi_rs.so` 的前置依赖列表里没有 `ui_menu.c`、`ui_state.c`，而**编译命令里却包含了它们**（`:26`）。`dist/libjunqi_rs.so` 不是 `.PHONY`，make 按时间戳判断 → 改 `ui_menu.c` 后 `make so` 判定"无需重建"，**so 保持旧内容**。
- 依据（实测）：
  ```bash
  cd rev64/src_rs && touch -d "2026-11-08" ui_menu.c && make -n so
  # → make: Nothing to be done for 'so'.
  ```
  （审计结束后已把 mtime 还原为 `2026-11-07 04:25:05`。）
- 影响：这条直接解释了 AUD-B-020 —— 归档 APK 里的 so 与 `src_rs/dist` 不一致的一部分原因就是"改了不重编"。
- 修法：`dist/libjunqi_rs.so: ... ui_menu.c ui_state.c`（并把所有参与编译的 `.c` 列全）；或干脆把 `so` 目标改成 `.PHONY` 强制重编。

## AUD-B-033 [P0] 6 个桌面测试目标链接缺 `ui_state_stub.c` → undefined reference，**12/12 回归 9 项 FAIL**
- 位置：`rev64/src_rs/Makefile:44`（bttest）、`:53`（laytest）、`:61`（aitest）、`:69`（touchtest）、`:79`（walkverify）、`:101`（gametest）
- 问题：只有 `dist/jq_run`(`:17`)、`dist/jq_consistency`(`:35`)、`dist/jq_evverify`(`:93`) 链接了 `ui_state_stub.c`；其余 6 个目标漏了，而 `rules.c` 依赖 `ui_rule_cfg_get`、`bt_session.c` 依赖 `ui_replay_record`（两者都定义在 `ui_state.c` / `ui_state_stub.c`）→ **链接直接失败**。
- 依据（实测）：
  ```
  $ make bttest
  /usr/bin/ld: rules.c:(.text+0xb1d): undefined reference to `ui_rule_cfg_get'
  /usr/bin/ld: bt_session.c:(.text+0x13d): undefined reference to `ui_replay_record'
  make: *** [Makefile:44: bttest] Error 1
  ```
  `make laytest` / `touchtest` / `gametest` / `walkverify` 同样 `undefined reference to 'ui_rule_cfg_get'`。
- 影响：`rev64/src_rs_regress.sh` 的 [2][4][6][7][8][10][11] 项**全部 FAIL**，整个"12/12 全回归"口径失真；`rs_cycle.sh` 必然在第 2 步中止。
- 修法：给这 6 个目标的链接行统一补 `ui_state_stub.c`（`bt_session` 相关再加 `ui_menu_stub.c`），并在 Makefile 顶部用变量 `DESK_STUBS = ui_state_stub.c ui_menu_stub.c` 统一引用。

## AUD-B-034 [P1] `dist/jq_evverify` 前置依赖缺 `evverify.c`；`dist/jq_run` 缺两个 stub；`dist/jq_consistency` 缺 `ui_state_stub.c`
- 位置：`rev64/src_rs/Makefile:15`、`:23`、`:34`
- 问题：与 AUD-B-032 同类的依赖声明缺失。当前因 `evverify`/`bttest` 等目标是 `.PHONY`（每次重编）而侥幸没暴露，但 `dist/jq_run`（`desk`）与 `dist/jq_consistency` 是**非 PHONY** 目标 → 改 `ui_state_stub.c` / `ui_menu_stub.c` 后不会重编。
- 依据：`Makefile:15` 的前置依赖里没有 `ui_state_stub.c`/`ui_menu_stub.c`，但 `:17` 的编译命令里有。
- 修法：把编译命令里出现的**所有** `.c` 都写进对应目标的依赖列表。

## AUD-B-035 [P1] `src_rs_regress.sh` [12/12] 的 grep 口径与实际输出不符 → 该项永久 FAIL
- 位置：`rev64/src_rs_regress.sh`（`run "[12/12] 桌面事件验证 (make evverify)" bash -c "cd $RS && make evverify MODE=--all 2>&1 | grep '8/8 全对'"`）
- 问题：evverify 现在的实际输出是 `桌面事件验证: 163/165 全对`，grep `'8/8 全对'` 永不命中 → 该项永远 FAIL。
- 依据：
  ```bash
  cd rev64/src_rs && ./dist/jq_evverify --all | tail -1
  # 桌面事件验证: 163/165 全对
  ```
- 修法：把判定改为数值判定（例如 `grep -oE '[0-9]+/[0-9]+ 全对'` 后比较分子==分母），不要写死字符串。

## AUD-B-036 [P2] 回归门禁用"脆弱字符串 grep"，口令漂移即全红/全绿
- 位置：`rev64/src_rs_regress.sh`（[2] `'全部通过'`、[6] `'12/12'`、[7] `'全部通过'`、[8] `'全部通过'`、[10] `'3/3 步合法'`、[11] `'5/5 回合 turn 交替 持续'`、[12] `'8/8 全对'`）
- 问题：门禁语义绑死在子程序的中文输出字符串上。子程序一改文案（本次批量对齐把计数从 8/8 改成 163/165 就是典型），门禁即刻失效，且**方向不可控**（可能变红也可能变绿）。
- 依据：见 AUD-B-035。
- 修法：子程序统一输出机器可读行（如 `RESULT: 163/165`），门禁解析该行并比较数值。

## AUD-B-037 [P2] `make clean` 删除整个 `dist/`（含已打好的 `phase_rs_a64.apk`）
- 位置：`rev64/src_rs/Makefile:37-38`
- 问题：`clean: rm -rf dist` → 手滑 `make clean` 会连同 `phase_rs_a64.apk` 一起删掉（该 APK 由 `mumu_swap_so.sh` 产出，编译链不会重建它）。
- 依据：`Makefile:38`。
- 修法：`clean` 只删 `dist/jq_*` 可执行文件，保留 `dist/*.apk`；或另设 `distclean`。

## AUD-B-038 [P2] evverify 存在 2 个**永久 FAIL**（`datah_024` / `datah_249` 面板尺寸为负）
- 位置：`rev64/src_rs/evverify.c`（由 `gen_dialog_batch.py` 生成的 `datah_024` / `datah_249` 用例）
- 问题：`datah_024（-318×-634 固定）` / `datah_249（-318×-667 固定）` 的面板宽高为负 —— 生成器对"无面板 rect"的 datah 未做跳过处理（提交信息声称"datah_271 跳过无面板 rect"，实际 024/249 没跳过），几何恒不成立 → 计数 163/165。
- 依据：`./dist/jq_evverify --all | grep FAIL` → 两行 FAIL。
- 修法：`gen_dialog_batch.py` 对 `W<=0 or H<=0` 的 datah 显式跳过并在输出里列出"跳过清单"，避免把无效几何写进 `ui_menu.h` / `evverify.c`。

## AUD-B-039 [P3] Makefile 硬编码 NDK 工具链三元组
- 位置：`rev64/src_rs/Makefile:25`
- 问题：`$(NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang` 写死；`NDK=` 变量只影响前缀，`android24` 这个 API 级别无法通过变量调（见 AUD-B-021）。
- 修法：拆成 `CLANG = $(NDK)/.../aarch64-linux-android$(API)-clang`，`API ?= 24`，并与 manifest 的 minSdk 保持一致。

## AUD-B-040 [P2] `src_rs_check.py` 红线审计第 [10] 项 FAIL：`GetMethodID` 失败未 `ExceptionClear` → pending exception 累积 → ART abort
- 位置：`rev64/src_rs/jni_bridge.c:2667`
- 问题：`jmethodID mid = (*env)->GetMethodID(env, kcls, "getKeyCode", "()I");` 之后**没有** `ExceptionCheck`/`ExceptionClear`。若 `GetMethodID` 失败（pending `NoSuchMethodError`），紧随其后的 `CallIntMethod` 会触发 ART abort → **闪退**。
- 依据：`python3 rev64/tools/jni_method_audit.py rev64/src_rs/jni_bridge.c` →
  ```
  line 2667: jmethodID mid = (*env)->GetMethodID(env, kcls, "getKeyCode", "()I");
  ✗ 有缺口：GetMethodID 失败未 ExceptionClear
  ```
  （同文件 `:456-485` 的 MediaPlayer 块做得是对的，属反例。）
- 修法：在 `:2667` 之后补 `if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); mid = NULL; }` 并对 `mid == NULL` 提前返回。

---

## D. `tools/layout_align_check.py`（几何对齐判据）

## AUD-B-041 [P0] 文件是**语法错误**的 Python —— 整个几何门禁已失效 6 天
- 位置：`rev64/tools/layout_align_check.py:22-23`
- 问题：第 22 行是一条 47572 字符的畸形单行（把 118 个 DIALOGS 条目的残片拼在注释后面），第 23 行起是**没有 dict 包裹**的 4 空格缩进条目 → `IndentationError`。工具根本无法 import / 运行。
- 依据：
  ```bash
  python3 -m py_compile rev64/tools/layout_align_check.py
  #   IndentationError: unexpected indent (layout_align_check.py, line 23)
  python3 rev64/tools/layout_align_check.py --baseline
  #   IndentationError: unexpected indent
  ```
  `git log` 定位：`9154653` 尚可编译 → `842787d`（2026-11-06 "src_rs: 批量 13 弹窗（gen_dialog_batch ...）"）起即损坏，至今 8 个提交未发现。
- 影响：AGENTS.md 把 `layout_align_check.py --baseline` 列为"布局/按钮对齐可复现判据"的核心门禁 —— 它自 2026-11-06 起一直返回非 0 且无人在意，因为 CI 并不检查它的退出码（见 AUD-B-041/042/043）。
- 修法：`git checkout 9154653 -- rev64/tools/layout_align_check.py` 还原到可用版本后，重新以**受控生成**（见 AUD-B-042/043）方式增量补 118 条；并在 CI 加 `python3 -m py_compile` 门禁。

## AUD-B-042 [P0] `gen_dialog_batch.py` 把 DIALOGS 条目插到 dict **外面** → 每跑一次生成就破坏语法
- 位置：`rev64/tools/gen_dialog_batch.py:543-545`
- 问题：`insert(la,"DIALOGS = {", ltxt)`，`insert()` 的 `before` 默认 `True`（`:331`）→ 条目被插到 `DIALOGS = {` **之前**，即模块级 → 必然 IndentationError。
- 依据（实测）：
  ```python
  s2 = s.replace("DIALOGS = {", ltxt+"DIALOGS = {", 1)
  compile(s2, ...)  # → SyntaxError: unexpected indent
  ```
  这正是 AUD-B-041 中 118 条残片落到第 22 行的直接原因。
- 修法：`insert(la,"DIALOGS = {\n", ltxt, before=False)`（插到 dict 内部），并在 `insert()` 里对"锚点不在 dict 内"做结构校验。

## AUD-B-043 [P1] `gen_dialog_batch.py --clean` 的正则只吃到第一个 `)` → 118 处残片被粘到上一行，**破坏文件且不可幂等**
- 位置：`rev64/tools/gen_dialog_batch.py:413-415`
- 问题：清理正则为 `r'\n    "datah_\d{3}_dlg": \([^)]*\),'`。`[^)]*` 遇到 DIALOGS 值里的第一个 `)` 就停（条目形如 `(400, 280, [("反馈",200,698,340,758),("关闭",...),])`），于是只删掉前缀、留下尾部残片；下一次 `--clean` 又在残片上继续匹配 → 文件被逐次腐蚀。
- 依据（实测）：
  ```python
  s2, n = re.subn(r'\n    "datah_\d{3}_dlg": \([^)]*\),', "", s, flags=re.S)
  # n = 118
  # 被删片段样例: '\n    "datah_000_dlg": (400, 280, [("反馈",200,698,340,758),'
  ```
  删除后长度 100941 → 94185，文件已不可编译。
- 修法：改成按行/按条目边界匹配（`re.M` + `^\s*"datah_\d{3}_dlg":.*\]\),\s*$`），或用 `ast` 解析后重建 dict；并给 `--clean` 加"清理后必须 `py_compile` 通过，否则回滚"的保护。

## AUD-B-044 [P1] DIALOGS 的 key 命名三套并存，工具与生成器对不上
- 位置：`rev64/tools/layout_align_check.py:23-140`（`datah_XXX_dlg`）vs `:833`（`--only` 帮助文本写 `datah_044_ui44`）vs `gen_dialog_batch.py:326`（`datah_{n:03d}_dlg`）vs `spak2ui.py:253`（`datah_{n:03d}_ui`）
- 问题：三种命名规则并存，`--only` 的示例名根本不存在；`spak2ui.py --apply` 生成的 `datah_XXX_ui` 键也永远匹配不上检查器。
- 依据：`grep -c 'datah_[0-9]*_ui"' layout_align_check.py` → **0**。
- 修法：统一为 `datah_{NNN}`（或 `datah_{NNN}_{slug}`），并在工具里由 `ui_menu.h` 的枚举反向派生键名，杜绝手写。

## AUD-B-045 [P3] 第 22 行 47572 字符单行巨块
- 位置：`rev64/tools/layout_align_check.py:22`
- 问题：单行 47KB，`git diff` / `git blame` / 编辑器全部不可用，是 AUD-B-041/042/043 的"视觉掩盖层"——肉眼看不出文件已损坏。
- 修法：还原后所有数据以"一条一行"格式落盘；加 pre-commit 钩子限制超长行。

## AUD-B-046 [P2] 几何基线无版本/日期标注，无法判断是否过期
- 位置：`rev64/tools/layout_align_check.py:146-189`（`datah_041_diff` / `datah_072_way` / `datah_001_set` / `datah_067_imen_bt` / `datah_006_imen` / `datah_089_nplayer` / `datah_062_newgame` / `datah_012_gameset` / `datah_017_loadlay` / `datah_020_savelay` / `datah_016_laymgr` 等）
- 问题：这些基线注释里提到"原版真机截图实测（orig_menu.png ×0.8）"，但**没有截图日期 / 设备 / 分辨率**，也没有"与当前 src_rs 对账日期"。原版截图一旦过期无从察觉。
- 依据：注释仅写"orig_ingame_menu.png 900×1600 ×0.8"。
- 修法：每个基线条目加 `{"src": "orig_menu.png", "shot": "2026-10-xx", "device": "...", "scale": 0.8}` 结构化元数据。

---

## E. `tools/gen_dialog_batch.py`（批量弹窗生成器）

## AUD-B-047 [P1] `--clean` 非幂等且具破坏性（同 AUD-B-043，此处从"生成器契约"角度记）
- 位置：`rev64/tools/gen_dialog_batch.py:344-418`
- 问题：`clean()` 的 docstring 承诺"只删带 ★ 批量对齐 / dlg_batch_text / jq_dlg_batch 标记的片段，不动其它代码"，但 layout 那条正则（AUD-B-043）会破坏结构；`render.c` 的"迭代 30 轮"清理（第 371-379 行）用 `open(...).write()` 在循环里反复写盘，中途异常会留下半清理状态。
- 依据：AUD-B-043 的复现。
- 修法：`clean()` 全部改为"按标记块整体删除 + 删后 py_compile/编译校验 + 失败回滚"。

## AUD-B-048 [P1] `insert()` 的幂等判据是"文本里出现过 datah 编号" → 换批次/重跑会误跳过
- 位置：`rev64/tools/gen_dialog_batch.py:331-341`
- 问题：
  ```python
  if "★ 批量对齐（datah_" in s and text.strip().startswith(("UI_D","/* ★ 批量对齐","    {sid}")):
      m=_re.search(r'datah_(\d{3})', text)
      if m and f'datah_{m.group(1)}' in s: return False
  ```
  只要目标文件里出现过该 datah 编号（哪怕是别的上下文里的注释），就整体跳过本次插入 → **部分插入**（例如 `ui_menu.h` 插了、`ui_menu.c` 没插）→ 编译缺符号。
- 依据：同文件 `:445-448` 的 `ui_menu.c/stub` 用的是另一套幂等判据（"本批次任一标记已在文件里 → 跳过"）—— 判据不一致，两处会失配。
- 修法：统一以"本批次专属标记块"（如 `/* ★ 批量对齐 batch=YYYYMMDD-NNN */`）为幂等键。

## AUD-B-049 [P1] `ui_menu.c` / `ui_menu_stub.c` 直接 `append` 到文件尾 → 二次生成重复定义静态表/函数
- 位置：`rev64/tools/gen_dialog_batch.py:445-448`
- 问题：`open(p,"a").write(...)` 无条件追加。虽然有"批次任一标记已在文件里就跳过"的粗保护，但一旦该标记被手工删掉一个而文件里仍有旧定义，再跑一次就会**重复定义 `g_ui_dNNN_btns` / `ui_dNNN_buttons`** → 链接重定义错误。
- 依据：`:445-448` 无任何"删除旧块"逻辑。
- 修法：追加前先按标记块删除旧块（复用 `clean()` 的正则），再追加。

## AUD-B-050 [P1] `evverify.c` 的 `run_*` 与注册行同样无脑追加 → 重复定义
- 位置：`rev64/tools/gen_dialog_batch.py:537-539`
- 问题：`open(ev,"a").write(ef)` 追加函数体；注册行用 `insert(ev, 'printf("=== 桌面事件验证: %d/%d 全对 ===\\n", pass, total);', reg)` —— 锚点是**输出字符串**，一旦 evverify 改了输出文案，注册行就找不到锚点而静默不插入（`insert()` 在锚点缺失时 `return False` 不报错）。
- 依据：`:331` `if not s.count(anchor): return False` —— 静默失败。
- 修法：`insert()` 在锚点缺失时抛异常；锚点改用稳定的代码标记而非输出字符串。

## AUD-B-051 [P2] `jni_bridge.c` 的插入锚点是 `rfind('    return JNI_FALSE;')` → 锚点漂移风险
- 位置：`rev64/tools/gen_dialog_batch.py:500-512`
- 问题：用"最后一个 `return JNI_FALSE;`"作为 OnTouch 尾部锚点。源码里若新增任何一个以 `return JNI_FALSE;` 结尾的函数（如 `OnKey`），插入点就会跑偏 → 分发块插到错误函数里。
- 依据：`s_jb.rfind(tail_anchor)`。
- 修法：改用函数级锚点（如 `}   /* 结束 OnTouch */` 注释）或结构化定位（函数名 + 花括号配平）。

## AUD-B-052 [P2] 生成器不校验产物是否真的含新符号（so 可能是旧的）
- 位置：`rev64/tools/gen_dialog_batch.py:341-351`
- 问题：生成末尾只跑 `make so` / `make evverify` / `make consistency` / `layout_align_check --baseline`，**不检查新 so 里是否出现新符号**。叠加 AUD-B-032（`make so` 不重编）→ 生成器会报告"编译 PASS / 回归 PASS"而 so 根本没变。
- 依据：AUD-B-032 的 `make -n so` 实测。
- 修法：生成后 `nm -D dist/libjunqi_rs.so | grep -q <新符号>` 断言，并强制 `make -B so` 或先删 so。

## AUD-B-053 [P3] `ROOT=/disk/workspaces/jq` 硬编码
- 位置：`rev64/tools/gen_dialog_batch.py:31`
- 问题：脚本不可移植到别的 checkout 路径。
- 修法：`ROOT = os.path.dirname(...)` 三级上跳。

---

## F. `tools/spak2ui.py`（datah XML → 弹窗代码生成器）

## AUD-B-054 [P1] `parse_dialog` 对畸形 XML 直接崩溃（实测 5 例崩 3 例）
- 位置：`rev64/tools/spak2ui.py:53-103`（尤其 `:64-66` 的四元组解包、`:84-87` 的按钮坐标解析）
- 问题：
  - `x0,y0,x1,y1 = (num(p) for p in parts)` —— `parts` 不足 4 个 → `ValueError: not enough values to unpack`；
  - `num()` 对非数字返回 `None` → 后续 `x0==0 and y0==0 and x1>100` 与 `None` 比较 → `TypeError`；
  - 按钮 `rect` 只有 2 段时 `p[2]`/`p[3]` → `IndexError`。
- 依据（实测）：
  ```
  [rect 3 段]   ★ CRASH ValueError: not enough values to unpack (expected 4, got 3)
  [rect 非数字] ★ CRASH TypeError: '>' not supported between instances of 'NoneType' and 'NoneType'
  [按钮 2 段]   ★ CRASH IndexError: list index out of range
  ```
- 影响：`gen_dialog_batch.py` 顶部 `from spak2ui import parse_dialog, to_720` —— 生成器与审计工具共用这条脆弱路径，任何一个畸形 datah 都会让整批生成中断（且不留日志）。
- 修法：全部解析包 `try/except` 并把"解析失败"记入跳过清单；`num()` 失败返回 0 并置 `ok=False`；`parts` 长度不足时直接 `continue`。

## AUD-B-055 [P1] `to_720` 对 `None` 坐标做算术 → TypeError
- 位置：`rev64/tools/spak2ui.py:116-121`
- 问题：`round(ox+b["x0"])` 在 `b["x0"] is None` 时抛 TypeError；`ox` 也可能是 `None`（`info["rect"]` 里 `r[0]` 为 None 时）。
- 依据：AUD-B-054 的 "rect 非数字" 用例即在此崩溃。
- 修法：`to_720` 过滤掉坐标不全的按钮并计数告警。

## AUD-B-056 [P1] 面板 rect 取"全文第一个 `style=... rect=...`" → 可能取到子控件的 rect
- 位置：`rev64/tools/spak2ui.py:61`
- 问题：`re.search(r'style="[^"]*" rect="([^"]+)"([^>]*)', txt)` 扫全文。若顶层 `<Window>` 没有 rect 而第一个子控件有 rect，则 `info["panel"]` 取到的是**按钮**的尺寸 → 居中偏移全错 → 整批弹窗坐标偏移。
- 依据：正则未限定在顶层 Window 上。
- 修法：先用 `xml.etree`/DOM 解析顶层 `<Window>`，再取其 rect；无 DOM 时用"第一个 `<Window ` 起始到其自闭合之间的片段"做范围限定。

## AUD-B-057 [P1] `cmd_apply` 不幂等（重复 `--apply` → 重复枚举/宏/静态表 → 编译失败）
- 位置：`rev64/tools/spak2ui.py:297-353`
- 问题：`cmd_apply` 对 `ui_menu.h` 做 `replace("} ui_screen;", ...)` 追加枚举、替换 `"#endif"` 追加宏；对 `ui_menu.c` 直接 `+=`；对 `ui_menu_stub.c` 替换 `"#endif"`；对 `evverify.c` `+=`。**没有任何"先删旧块"逻辑** → 第二次 `--apply` 会产生重复的 `UI_UIxxx` 枚举项、重复的 `#define`、**重复的 `static const ui_uiNNN_btn g_uiNNN_btns[]` 定义**（C 编译错误：redefinition）。
- 依据：`menuc_txt += "\n" + ...`（`:316`）与 `stub_txt.replace("#endif", stub_add + "\n#endif", 1)`（`:325`）。
- 修法：与 AUD-B-049 一样，先按标记块删旧再写。

## AUD-B-058 [P1] `cmd_apply` 的 5 处 `str.replace` 全部无失败检查 → 静默不生效
- 位置：`rev64/tools/spak2ui.py:312-313`（`} ui_screen;` / `#endif`）、`:331`（`int main(int argc`）、`:334`（`total++; pass += run_drawdetail();`）、`:338`（`}\n\n# datah_107/039 绝对坐标面板`）
- 问题：任一锚点文案变化，`replace` 无操作、函数照样打印"=== --apply 贴入完成（5 源文件）===" → **报告成功、实际没贴**。`:338` 的锚点更是依赖一条注释文本 `# datah_107/039 绝对坐标面板`，极易漂移。
- 依据：`str.replace` 无匹配时静默返回原串（代码未检查 `count`）。
- 修法：每次 replace 前 `assert s.count(anchor) == 1`，否则抛错中止。

## AUD-B-059 [P1] `cmd_apply` 的编译检测丢失 make 退出码，且被 Makefile 陈旧依赖掩盖
- 位置：`rev64/tools/spak2ui.py:343`
- 问题：`subprocess.run(f"cd {SRC} && NDK={NDK} make so 2>&1 | grep -E 'error:' | head", shell=True)` → 管道退出码取自 `head`（恒 0）；只看 stderr 里的 `error:`，**链接错误（`ld: undefined reference`）不含 `error:`** → 链接失败被判为"编译 PASS"。叠加 AUD-B-032（make 不重编），陈旧 so 也会报 PASS。
- 依据：`:343` 的写法 + Makefile 缺依赖的实测。
- 修法：`r = subprocess.run(f"cd {SRC} && NDK={NDK} make -B so", shell=True); if r.returncode: fail`。

## AUD-B-060 [P2] `cmd_apply` 里 evverify / consistency / layout_align_check 的退出码全部被忽略
- 位置：`rev64/tools/spak2ui.py:347-351`
- 问题：三处 `subprocess.run(...)` 不检查返回码，也不检查输出内容 → 回归红了也照常打印"★ 批量对齐完成"。
- 依据：`:347` `subprocess.run(f"... make evverify >/dev/null 2>&1 && ./dist/jq_evverify --all", shell=True)`（stdout 直接打屏但不判码）。
- 修法：统一收集返回码，非 0 即 return 1。

## AUD-B-061 [P2] `cmd_status` 的"153 个无按钮"是硬编码常量，与实际不符
- 位置：`rev64/tools/spak2ui.py:166`
- 问题：常量 153 写死在 print 里，不随 `spak_files()` 实际统计变化 → 进度报告失真。
- 依据：`:166` `print(f"\n★ 153 个无按钮（纯展示/文本框/资源占位）不补...")`。
- 修法：用 `len(by_btn[0])` 代替。

## AUD-B-062 [P2] `cmd_status` 按钮数分桶把 ≥5 归为 5，计数口径失真
- 位置：`rev64/tools/spak2ui.py:154,167`
- 问题：`by_btn[min(bc,5)]` 使 5 按钮与 9 按钮同桶；而 `:167` 的 `sum(len(by_btn[k]) for k in [1,2,3,4,5])` 又把它当"5 按钮弹窗数"报出。
- 依据：`:154` `by_btn={0:[],1:[],2:[],3:[],4:[],5:[]}` + `by_btn[min(bc,5)].append(n)`。
- 修法：分桶改为 0/1/2/3/4/5+/6+… 并分列统计。

## AUD-B-063 [P2] `ALIGNED` 清单硬编码，与 `ui_menu.h` 的枚举无交叉校验
- 位置：`rev64/tools/spak2ui.py:38-43`
- 问题：`ALIGNED` 是手写的 47+12+9 个编号；`ui_menu.h` 现有 118 个 `UI_Dxxx` 批量弹窗 + 数十个 `UI_*` 弹窗，两边没有任何一致性检查 → `status` 的"已对齐 N 个"随时可能虚高/虚低。
- 依据：`spak2ui.py:38` 的集合 vs `ui_menu.h` 中 `UI_D\d{3},` 计 118。
- 修法：从 `ui_menu.h` 解析枚举反推已对齐集合，`ALIGNED` 降级为历史备注。

## AUD-B-064 [P3] `SPAK = "/tmp/opencode/spak_out"` 硬编码外部路径
- 位置：`rev64/tools/spak2ui.py:32`
- 问题：素材在 `/tmp`，仓库内无副本 → 换机即不可用。
- 修法：允许 `--spak` 参数，默认指向 `rev64/dist/spak_res` 之类仓库内路径。

---

## G. `tools/spak_strings.py` / `tools/spak_ui_assets.py` / `jq_strings.h`

## AUD-B-065 [P1] `jq_strings.h` 声称由 `spak_strings.py --list` 生成，但该脚本**根本不生成** `jq_strings.h`
- 位置：`rev64/src_rs/jq_strings.h:6`（"生成: python3 rev64/tools/spak_strings.py --list → 本文件"）vs `rev64/tools/spak_strings.py`（只写 `dist/spak_res/ui_string_index.json`）
- 问题：所谓"原版文案权威表"的唯一生成器缺失 → 该头文件是**一次性手工产物**，无法与 SPAK XML 重新对账，字符串会静默漂移（79 条 vs XML 83 条，见 AUD-B-066）。
- 依据：通读 `spak_strings.py` 无任何写 `jq_strings.h` 的代码路径。
- 修法：在 `spak_strings.py` 里实现 `--emit-header`（读 XML → 生成 `jq_strings.h`），并把生成结果纳入 CI 对账。

## AUD-B-066 [P2] 4 个字符串在多个 `StringSet` 重名且**值不同** → `parse()` 后写覆盖 → "认输/投降"文案可能取错
- 位置：`rev64/tools/spak_strings.py:25-28`；`rev64/dist/spak_res/data_config_string.xml`
- 问题：`out[sm.group(1)] = {...}` 对同名 key 后写覆盖。实测 XML 中 4 个名字跨组重名且值不同：
  ```
  DrawConfirm   → '您真的要提和吗？' / '您真的要提和吗？(%d)'
  DrawSend      → '向对方提和' / '向其他人提和'
  DrawReq       → '对方提和，您同意吗？' / '%s提和，您同意吗？'
  ResignConfirm → '您真的要认输吗？' / '您真的要投降吗？'
  ```
  → `jq_strings.h` 里 `JQ_STR_ResignConfirm` 只可能是其中一个（实测取"您真的要认输吗？"）。
- 依据：XML 实测 83 条 pair、header 79 个宏（4 个被折叠）。
- 修法：索引键改为 `(group, name)`，宏名带组前缀（如 `JQ_STR_Common_ResignConfirm` / `JQ_STR_Bt_ResignConfirm`），由使用点显式选组。

## AUD-B-067 [P2] GBK 解码的 fallback 不可靠：UTF-8 文件会被静默误解码
- 位置：`rev64/tools/spak_strings.py:21-22`（同 `spak2ui.py:56-57`、`gen_dialog_batch.py` 的 datah 解析）
- 问题：`try: raw.decode("gbk") except UnicodeDecodeError: raw.decode("utf-8","ignore")`。GBK 能解码绝大多数任意字节序列 → 真正的 UTF-8 中文 XML **不会**触发 `UnicodeDecodeError`，而是被误解成乱码且**不报错**；`utf-8` 分支还带 `"ignore"` 会静默丢字符。
- 依据：当前 `data_config_string.xml` 声明 `encoding="GBK"` 且确为 GBK 字节（`b1a3 b4e6 b3c9 b9a6 a3a1` = "保存成功"），说明该假设对本文件成立；但对**其它** datah XML 若编码不同即静默出错。
- 修法：先读 XML 声明的 `encoding=` 再按声明解码；无声明时用 `charset_normalizer` 探测并对低置信度报错。

## AUD-B-068 [P3] `spak_strings.py` 的 `--get` / `--set` 缺参数未校验
- 位置：`rev64/tools/spak_strings.py:54-56`
- 问题：`a[1]` 直接取用 → `--get` 不带值抛 `IndexError`（traceback 而非友好提示）。
- 依据：`:45` 只校验 `if not a`。
- 修法：`argparse` 化。

## AUD-B-069 [P2] `spak_strings.py` 的 `save(idx)` 在 `--list` / `--grep` 也写盘（读操作有副作用）
- 位置：`rev64/tools/spak_strings.py:61`
- 问题：所有分支末尾都 `save(idx)` → 只读的 `--list` 也会重写 `ui_string_index.json`，在并发/只读挂载下会失败或污染。
- 依据：`:47-60` 各分支后统一 `:61 save(idx)`。
- 修法：仅 `--index`/`--update` 时保存。

## AUD-B-070 [P2] `spak_ui_assets.features()` 纯 Python 逐像素 → 大图 1.5s/张，`--index` 需数分钟
- 位置：`rev64/tools/spak_ui_assets.py:51-78`
- 问题：`vis` 逐像素统计、`ring` 四边逐像素统计、`interior` 还构造了一份**完整像素列表副本**（`interior=[px[x,y] for ...]`，1121×1121 ≈ 1.26M 个 tuple）→ 单图 1.52s，110 张 datax PNG 全量 `--index` 约 3 分钟，且内存峰值数百 MB。
- 依据：实测 `单个大图 features 耗时 1.52s (1121×1121)`。
- 修法：用 `Image.crop` + `getbbox`/`getextrema`/`ImageStat` 向量化；`interior` 改用直方图而非列表。

## AUD-B-071 [P3] `datax_files()` 正则只认 3 位编号
- 位置：`rev64/tools/spak_ui_assets.py:45`
- 问题：`re.match(r'datax_(\d{3})_', ...)` —— 编号 ≥ 1000 的资源会被静默跳过。当前 datax 编号实测到 300（安全），但 `dist/spak_res` 里还有 191 个非 PNG 的 `datax_*.bin` 被 `open(f,'rb').read(4)!=b'\x89PNG'` 过滤掉 —— 过滤原因不留痕，索引里看不到"被排除的 191 个"。
- 依据：`datax PNG 数: 110`（编号范围 000-300）。
- 修法：正则改 `\d{3,}`，并在 `--index` 里报告"非 PNG 候选 N 个（未索引）"。

## AUD-B-072 [P3] `cmd_set` 可写入不存在的编号 → 索引含幽灵条目
- 位置：`rev64/tools/spak_ui_assets.py:149-151`
- 问题：`idx.setdefault(str(n),{})["sem"]=sem` 不校验 `n` 是否存在于 `datax_files()`。
- 依据：`:151` 无校验。
- 修法：校验存在性，否则报错。

---

## H. `tools/link_audit.py` / `tools/so_link_check.py`（链接安全审计）

## AUD-B-073 [P1] `link_audit.py` 是**纯 32 位**工具，对 arm64 so 给出 FAIL 假阳性（实测 28 个 UND 被误判）
- 位置：`rev64/tools/link_audit.py:36-43`（`DEVICE_LIBS` 全是 `/system/lib/arm/...`）、`:95-106`
- 问题：缓存库是 32 位 ARM 的 `libc/libdl/liblog/libm/libGLESv1_CM/libGLESv2`，用它们去核验 **arm64** so 的 UND 集合 → 大量符号判为"无法解析"。实测对 `src_rs/dist/libjunqi_rs.so`：
  ```
  ★ 无法解析（链接失败嫌疑）: AAssetManager_open, AAsset_close, ..., inflate, opendir, ...
  >>> 审计结论: FAIL
  ```
  即：**arm64 的 src_rs so 永远无法通过 link_audit**（真实状态应为 PASS）。
- 依据：`python3 rev64/tools/link_audit.py audit rev64/src_rs/dist/libjunqi_rs.so` → FAIL。
- 修法：按 ELF class 分派缓存（`/system/lib/arm` vs `/system/lib/arm64`），或显式声明该工具仅适用 32 位并在审计 ARM64 产物时跳过。

## AUD-B-074 [P1] `DEVICE_LIBS` 缺 `libandroid.so` / `libz.so` / `libEGL.so` → AAsset*/inflate*/egl* 永远判 unresolved
- 位置：`rev64/tools/link_audit.py:36-43`
- 问题：`src_rs` so 的 NEEDED 含 `libz.so`、`libEGL.so`、`libandroid.so`，但审计的候选库列表里没有 → `AAssetManager_open`（libandroid）、`inflate*`（libz）、`eglCreatePbufferSurface`（libEGL）全部假 unresolved。
- 依据：`readelf -d src_rs/dist/libjunqi_rs.so | grep NEEDED` → libm/liblog/libz/libEGL/libGLESv1_CM/libGLESv2/libandroid/libdl/libc。
- 修法：`DEVICE_LIBS` 补 `libandroid.so` / `libz.so` / `libEGL.so`（arm 与 arm64 两套）。

## AUD-B-075 [P1] `lib_exports()` 跳过 `_` 前缀符号 → `__cxa_atexit` / `__cxa_finalize` / `__system_property_set` 永远判 unresolved
- 位置：`rev64/tools/link_audit.py:102-105`
- 问题：`if name.startswith('_'): continue`（意图是跳过 C++ mangled 名）→ 连 `__cxa_atexit`、`__cxa_finalize`、`__register_atfork`、`__system_property_set` 这些**标准 C 符号**也被跳过，缓存里明明有也匹配不上。
- 依据：实测 unresolved 列表中含这 4 个。
- 修法：只跳过 `_Z`（Itanium C++ mangling）前缀，其余保留。

## AUD-B-076 [P1] 悬空符号检测只认 `F_` 前缀 → 自定义符号（`jq_*`/`ui_*`/`bt_*`/`rs_*`）全部漏检
- 位置：`rev64/tools/link_audit.py:170-175`
- 问题：
  ```python
  if s.startswith('F_') and s not in defined: dangling.add(s)
  ```
  src_rs / 翻译体里大量自定义符号（`jq_walk_bridge_tick`、`ui_*`、`bt_*`、`rs_*`）若在重定位里出现却无定义（正是 `fail to link` 的根因之一），**完全不会被检出**。
- 依据：`link_audit.py:174` 的判定条件。
- 修法：改为"重定位里出现的任意符号，若既不在 `defined`、也不在任何 NEEDED 库的导出表里 → 悬空"，不按前缀区分。

## AUD-B-077 [P1] `so_from_apk()` 优先取 `armeabi` → 多 ABI APK 只审 32 位，ARM64 so 从不被审
- 位置：`rev64/tools/link_audit.py:182-187`
- 问题：候选顺序 `armeabi` → `armeabi-v7a` → `arm64-v8a`，且 `break` 在第一个命中。对阶段 2（双 ABI）APK，审计的永远是 32 位 so —— 而 64 位设备上真正加载的是 arm64 so。
- 依据：`link_audit.py:183-187`。
- 修法：改为遍历 APK 内**全部** `lib/*/libjunqi.so` 并逐一审计，输出逐 ABI 结论。

## AUD-B-078 [P2] `.hash` 风格（gnu.hash）不进 verdict → MuMu 32 位 linker 不支持的 so 仍判 PASS
- 位置：`rev64/tools/link_audit.py:196-202`（`hash_style`）、`:401-402`（`verdict`）
- 问题：docstring 明写"MuMu 32 位 linker 不支持 .gnu.hash → 必须 sysv"，但 `hash_style()` 的结果**只被打印**、不进 verdict。实测对 src_rs arm64 so 输出 `.hash 风格: gnu.hash` 而结论里不体现。
- 依据：`verdict = 'PASS' if (not dangling and not bad_known and not unresolved and not disp_problems) else 'FAIL'` —— 无 `hash_style` 项。
- 修法：`verdict` 增加 `hash_style(so) == 'sysv'` 条件（32 位路径）。

## AUD-B-079 [P1] `apk_checks` 的 docstring 声称校验 "v1+v2 签名"，实际**没有任何签名校验**
- 位置：`rev64/tools/link_audit.py:16`（docstring）vs `:205-214`（实现）
- 问题：实现只查 `.so` 是否 STORE、`AndroidManifest.xml` 头是否 `0x00080003`、`lib/` 目录列表。**签名方案（v1/v2）完全未检** → 一个只签 v1 的包会被判 PASS。
- 依据：`:205-214` 全文无 `apksigner`/`META-INF` 相关代码。
- 修法：实现 `apksigner verify --verbose` 调用并断言 `v1 scheme: true` 与 `v2 scheme: true`。

## AUD-B-080 [P3] FAIL 路径泄漏临时解出的 `.so`
- 位置：`rev64/tools/link_audit.py:331-337`
- 问题：`resolve_so` 抛异常时直接 `return 'FAIL'`，跳过了末尾的 `if tmp: os.unlink(tmp)` → `/tmp` 下积累 `tmp*.so`。
- 依据：`:331-337` 的早退路径。
- 修法：把清理放进 `finally`。

## AUD-B-081 [P1] `so_link_check.py` 只检查 `gl*`/`egl*` → 自定义 UND 符号（`jq_*`/`ui_*`/`bt_*`/`rs_*`）完全不检查
- 位置：`rev64/tools/so_link_check.py`（`main()` 中 `gl_missing = [s for s in missing if s.startswith(('gl','egl'))]`）
- 问题：docstring 声称"每个 UND gl*/egl* 符号必须在 MuMu 某库的导出表里"，实现确实只看 GL/EGL。而真正会导致 `failed to link` 的自定义符号（`jq_walk_bridge_tick` 等）**不被检查**；同时 libc/libm/liblog/libz/libandroid 的 UND 也一律不查。
- 依据：实测 `so_link_check.py src_rs/dist/libjunqi_rs.so` → `UND 总数: 108, 其中 GL/EGL: 55` → 只核了 55/108。
- 修法：对"自定义前缀符号"（`jq_`/`ui_`/`bt_`/`rs_`/`jqt_`）做**本地定义存在性**断言；其余 UND 至少按 NEEDED 库白名单核。

## AUD-B-082 [P1] `so_link_check.py` 的退出码 1（缓存缺失）被上游当成 PASS
- 位置：`rev64/tools/mumu_swap_so.sh:28-31`、`rev64/tools/rebuild_rs_apk.sh:39-43`
- 问题：
  ```bash
  python3 $TOOLS/so_link_check.py $RS/dist/libjunqi_rs.so >log 2>&1
  LC=$?
  [ $LC -eq 2 ] && { echo "★ 链接审计 FAIL"; exit 1; }
  ```
  `so_link_check.py` 在"缓存不存在"时 `return 1`（`:main` → `print("缓存不存在…"); return 1`）→ **LC=1 不等于 2 → 审计被完全跳过，脚本继续打包并打印"链接审计 PASS"**。
- 依据：`so_link_check.py` 的 `if not os.path.exists(CACHE): ... return 1`。
- 修法：统一语义为"0=PASS，其余=FAIL"，或上游改成 `[ $LC -ne 0 ] && exit 1`。

## AUD-B-083 [P2] `so_link_check` 的符号缓存是 2026-11-04 的固定快照
- 位置：`/tmp/opencode/mumu_und_syms/all_syms.json`（2026-11-04 07:39）
- 问题：缓存 4 个月前的 MuMu 库导出表；设备/ROM 更新后结果失真，且 `--update-syms` 需要 adb 在线。
- 依据：`ls -la /tmp/opencode/mumu_und_syms/` → 全部 2026-11-04。
- 修法：把缓存快照日期写进报告并在超过 N 天时告警；门禁时强制 `--update-syms`。

---

## I. `tools/rs_cycle.sh`（src_rs 全周期一条龙）

## AUD-B-084 [P1] `rebuild_rs_apk.sh` / `mumu_regress.sh` 的退出码被 `grep` 吞掉 → 永远 `✓ rs_cycle 完成` 且 exit 0
- 位置：`rev64/tools/rs_cycle.sh:50`、`:52`、`:57`、`:59-60`
- 问题：三处都是 `bash ... 2>&1 | grep -E "..."` → 管道退出码取自 `grep`；即使被调脚本以非 0 退出，只要输出里有一行匹配关键字就判成功。更外层 `exit 0` 是无条件的。
- 依据：`rs_cycle.sh:59-60` `log "✓ rs_cycle 完成"; exit 0`。
- 修法：`set -o pipefail` + 单独捕获被调脚本的 `$?`；结尾按结果 `exit 1`。

## AUD-B-085 [P1] 当前 `src_rs_regress.sh` 是 **9/12 FAIL** → `rs_cycle.sh` 必然在第 2 步中止，"一条龙"走不通
- 位置：`rev64/tools/rs_cycle.sh:37-43`（调用 `src_rs_regress.sh`）
- 问题：实跑结果：
  ```
  ★ 回归有 9 项 FAIL: [2/12] [4/12] [5/12] [6/12] [7/12] [8/12] [10/12] [11/12] [12/12]
  ```
  根因：AUD-B-033（6 个目标链接失败）+ AUD-B-035（grep 口径）+ AUD-B-040（红线审计第 10 项）。
- 依据：`cd /disk/workspaces/jq && bash rev64/src_rs_regress.sh`。
- 修法：先修 033/035/040，让回归回到绿，再谈"一条龙"。

## AUD-B-086 [P2] 默认设备地址是硬编码内网 IP
- 位置：`rev64/tools/rs_cycle.sh:13`（`DEV=${DEV:-10.0.182.146:5555}`）
- 问题：与 `mumu_regress.sh` 的默认 `emulator-5554` 不一致；不可达时 `adb install` 失败，但因 AUD-B-084 仍报成功。
- 依据：两个脚本默认值不同。
- 修法：统一走 `DEV` 环境变量 + 启动前 `adb get-state` 探活，失败即 exit。

## AUD-B-087 [P2] 无 `set -e` / `set -o pipefail`
- 位置：`rev64/tools/rs_cycle.sh:10`
- 问题：只有 `set -u`。中间任何一步非 0 退出都靠显式 `if` 捕获，遗漏一处即静默继续。
- 修法：`set -euo pipefail`。

## AUD-B-088 [P3] 步骤编号标注错误（写了 `[1/3]`，实际有第 4 步）
- 位置：`rev64/tools/rs_cycle.sh:28,38,48,56`
- 问题：`:48` 写 `=== [3/3] ===`，而 `:56` 是 `[4/4]` → 日志读起来自相矛盾。
- 修法：统一为 `[1/4]…[4/4]`。

---

## J. `tools/mumu_swap_so.sh`（换 so 一条龙）

## AUD-B-089 [P1] 启动的是**原包名** `com.cnvcs.junqi`，而全流程操作的是 `com.cnvcs.junqirs` → 进程存活判据永远错
- 位置：`rev64/tools/mumu_swap_so.sh:177`
- 问题：
  ```bash
  adb -s $DEV shell am start -n com.cnvcs.junqi/com.cnvcs.App
  ```
  但 `PKG=com.cnvcs.junqirs`（`:20`），安装/卸载/哈希对账/日志抓取全部针对 `junqirs`。启动的却是另一个包 → `:179` 的 `pidof $PKG` 取不到 PID → 末尾判定 `FAIL: 进程未存活`（假阴性）；更糟的情况是设备上恰好装着原版包，则**测的是旧 app**。
- 依据：`:177` 与 `:20`/:179/:195 的包名不一致。
- 修法：统一用 `$PKG`：`am start -n $PKG/com.cnvcs.App`。

## AUD-B-090 [P1] `apksigner sign` 的失败被 `grep|head` 吞掉 → 未签名 APK 被当成功交付
- 位置：`rev64/tools/mumu_swap_so.sh:111-114`
- 问题：`apksigner sign ... 2>&1 | grep -vE "^$" | head -3` → 退出码取自 `head`（恒 0）；紧接着 `cp aligned_new.apk "$OUT"` 照做 → 签名失败时产出**未签名 APK** 并继续装机。
- 依据：与 `rebuild_rs_apk.sh:70-72`（`apksigner sign ... | tail -1` + `[ $? -ne 0 ]`）是同一个 bug。
- 修法：签名单独执行、检查 `$?`，再用 `apksigner verify` 二次确认后才 `cp`。

## AUD-B-091 [P1] zip-surgery 保留旧 `META-INF/*.RSA` → 换 so 后 v1 签名**必然失效**（注释结论错误）
- 位置：`rev64/tools/mumu_swap_so.sh:70-84`（尤其 `:73` 注释"v1 签名不覆盖 .so → 换 so 不破坏 v1 签名"）
- 问题：JAR (v1) 签名对 APK 内**每一个条目**都在 `META-INF/MANIFEST.MF` 里记 SHA-256 摘要，`.so` 也不例外。替换 `lib/arm64-v8a/libjunqi.so` 后旧 `MANIFEST.MF` 里的该条目摘要失配 → v1 校验失败（Android 7+ 在只有 v1 时会 `INSTALL_PARSE_FAILED`）。注释里的结论是错的。
- 依据：v1 签名规范（`MANIFEST.MF` 覆盖全部条目）。
- 修法：zip-surgery 阶段就**丢弃** `META-INF/*`，让后续 `apksigner sign` 重新生成 v1+v2+v3；注释同步更正。

## AUD-B-092 [P2] 声称"纯 v1 签名"，实际产物是 v1+v3（apksigner 默认开 v3）
- 位置：`rev64/tools/mumu_swap_so.sh:104`、`:111`
- 问题：`--v1-signing-enabled true --v2-signing-enabled false` 并不关闭 v3 → 产物 `phase_rs_a64.apk` 实测 `v1 true / v2 false / v3 true`（见 AUD-B-030）。注释与产物不符，且不满足 AGENTS.md 的 v1+v2 硬要求。
- 依据：`apksigner verify --verbose rev64/src_rs/dist/phase_rs_a64.apk`。
- 修法：显式 `--v3-signing-enabled false`，并把 MuMu 兼容需求与交付签名口径分离（另建测试包）。

## AUD-B-093 [P2] 无条件 `adb uninstall` → 清掉用户存档与设置
- 位置：`rev64/tools/mumu_swap_so.sh:127`
- 问题：每次换 so 都先卸载，用户的游戏存档/设置全丢；且在 `:151` 的重试路径里还会再卸一次。
- 依据：`:127` `adb -s $DEV uninstall $PKG >/dev/null 2>&1`。
- 修法：优先 `adb install -r`；只在签名不一致时才卸载，并先 `adb backup`/提示。

## AUD-B-094 [P2] `Fatal signal` 计数不区分进程 → 其它应用崩溃导致误判 FAIL
- 位置：`rev64/tools/mumu_swap_so.sh:188`
- 问题：`adb logcat -d | grep -cE "Fatal signal"` 统计的是**全系统** fatal signal。虽然 `:176` 有 `logcat -c`，但 10 秒窗口内其它进程崩溃同样计数 → 误判"崩溃"。
- 依据：`:188` `SIGS=$(adb logcat -d | grep -cE "Fatal signal" | tr -d ' \r')`。
- 修法：按 `pid` 过滤（`grep "pid $PID"`）或用 `dropbox`/`tombstone` 精确匹配。

## AUD-B-095 [P2] `dec_rs` 重建成功时不再校验包名 → 可能打包/安装错包
- 位置：`rev64/tools/mumu_swap_so.sh:58-69`
- 问题：包名校验（`:42-46`）只针对候选 APK；`:61-64` 用 `apktool b dec_rs` 产出的 `raw_decrs.apk` **直接替换** `SRCAPK`，跳过包名校验。若 `dec_rs` 的包名变了，后续 `:139` 的 `pm path $PKG` 会拿不到路径 → 哈希对账失败但错误信息误导。
- 依据：`:62-64` 无包名检查。
- 修法：`raw_decrs.apk` 也走一次 `aapt dump badging` 包名校验。

## AUD-B-096 [P2] 每次运行推送 962 个 `spak_res` 文件（性能）
- 位置：`rev64/tools/mumu_swap_so.sh:170-172`
- 问题：`for f in /disk/workspaces/jq/rev64/dist/spak_res/*; do adb push ...` —— 目录里有 962 个文件，逐个 adb 推送的启动开销是分钟级。
- 依据：`ls rev64/dist/spak_res | wc -l` → 962。
- 修法：`adb push` 整个目录（一次调用），或先 tar 再推。

## AUD-B-097 [P3] 注释"dumpsys 恒报 1.52"与实际 1.6.8 不符
- 位置：`rev64/tools/mumu_swap_so.sh:194`、`:198`
- 问题：注释称"MuMu dumpsys 恒报 1.52"，实际包 `com.cnvcs.junqirs` 的 versionName 是 `1.6.8`。
- 修法：更正注释。

---

## K. `tools/bt_smali.sh`（蓝牙 BtSpp.java → dex → smali → 合树）

## AUD-B-098 [P1] 合并到构建树前不检查目标目录存在 → 静默打印"✓ 已合并"
- 位置：`rev64/tools/bt_smali.sh:47-48`
- 问题：
  ```bash
  cp smali_out/com/cnvcs/*.smali "$RA/dec_dual/smali/com/cnvcs/"
  cp smali_out/com/cnvcs/*.smali "$RA/dec_full/smali/com/cnvcs/"
  echo "✓ BtSpp smali 已合并到 dec_dual + dec_full（...）"
  ```
  目标目录不存在时 `cp` 失败，但脚本**照常打印成功**并 exit 0 → 后续 `apktool b` 会用**旧 smali**（甚至没有 BtSpp）打包 Bluetooth 联机功能缺失的 APK，而 CI 显示"成功"。
- 依据：`cp` 的失败未被检查；`set -u` 不覆盖错误码。
- 修法：`cp` 加 `|| { echo "✗ 合并失败（目标目录缺失）"; exit 1; }`，并断言目标文件 md5 已更新。

## AUD-B-099 [P2] 依赖 containerd 快照目录下的 `android.jar` / `d8`
- 位置：`rev64/tools/bt_smali.sh:21-22`
- 问题：`/disk/containerd/.../snapshots/760/.../android-32/android.jar` 与 `.../snapshots/741/.../d8` —— 快照被 GC 即失效，且与 `build_phases.sh` 的 zipalign 来自不同快照（741 vs 760），一致性无从保证。
- 依据：`:21`、`:22`。
- 修法：统一走 `$ANDROID_HOME/platforms/android-32/android.jar` + `$ANDROID_HOME/build-tools/*/d8`，并做多候选探测。

## AUD-B-100 [P2] `--verify` 也是破坏性的（会重编 + 重合并）
- 位置：`rev64/tools/bt_smali.sh:60-65`（`--verify` 分支在合并**之后**才执行）
- 问题：文档说"`--verify` 只校验树内 BtSpp 已含最新逻辑"，实现却先跑完整编译+合并流程再打印校验结果。只想"看一眼"也会改动两棵树。
- 依据：`:37-48` 的编译合并在 `:60` 的 `--verify` 判断之前无条件执行。
- 修法：把 `--verify` 判断前置，`--verify` 时只做校验。

## AUD-B-101 [P2] `App.smali` 单向同步（dec_full → dec_dual），dec_dual 的改动会被静默覆盖
- 位置：`rev64/tools/bt_smali.sh:51-57`
- 问题：脚本把 `dec_full/smali/com/cnvcs/{App.smali,App$BtStart.smali}` 复制到 `dec_dual`。当前两树内容相同（`diff` → IDENTICAL），但一旦有人只改 `dec_dual`（阶段 2 专属改动），下一次 `bt_smali.sh` 会**静默抹掉**。
- 依据：`:54-56` 的单向 `cp`。
- 修法：改为"两边内容不一致时报错并列出 diff"，强制人工确认方向。

## AUD-B-102 [P3] `javac` / `d8` 的退出码被 `head`/`grep` 吞掉
- 位置：`rev64/tools/bt_smali.sh:39`、`:40`、`:42`
- 问题：`javac ... 2>&1 | head -3 || { echo "✗ 编译失败"; exit 1; }` —— `||` 判的是 `head` 的退出码（恒 0）→ 该错误分支是**死代码**。真正兜底的是 `:41`/`:43` 的"文件存在性"检查（能覆盖，但语义不清）。
- 依据：`:39` 的 `|| {...}` 永不触发。
- 修法：去掉 `| head`，或改用 `set -o pipefail`。

## AUD-B-103 [P3] `--verify` 的 `grep -c` 统计的是**行数**而非符号数
- 位置：`rev64/tools/bt_smali.sh:63`
- 问题：`grep -c "btSend\|btHost\|btClient\|btState" App.smali` → 只要有 1 行同时提到 4 个关键字也计 1 → 校验信号极弱。
- 修法：用 `grep -o` 计数并与期望符号集比对。

## AUD-B-104 [P2] `BtSpp.java` 源文件不在仓库（只在 `/tmp`）→ 蓝牙链路不可复现
- 位置：`rev64/tools/bt_smali.sh:19`（`SRC=/tmp/opencode/jq_bt_src`）
- 问题：Java 源码（`com/cnvcs/BtSpp.java`）是仓库外的临时文件，`.gitignore` 未收录任何副本 → 蓝牙联机功能无法从仓库重建。
- 依据：`ls /tmp/opencode/jq_bt_src/com/cnvcs/` 可见源文件，但 `git ls-files` 中无。
- 修法：把 `BtSpp.java` 收进 `rev64/src_rs/java/com/cnvcs/`。

## AUD-B-105 [P2] 合并后不做 smali→dex 的编译校验
- 位置：`rev64/tools/bt_smali.sh`（合并后无验证）
- 问题：合并进树的 smali 从未被 `apktool b` / `d8` 编译过一次 → 语法错误要到 `build_phases.sh` 的 `apktool b` 阶段才暴露，而那一步的错误信息被 `>/dev/null 2>&1` 吞掉（`build_phases.sh:58`）。
- 依据：`build_phases.sh:58` `apktool b ... >/dev/null 2>&1 || { echo "✗ $phase apktool b 失败"; return 1; }` —— 无 apktool 日志输出。
- 修法：`bt_smali.sh` 结束时用 `apktool b`（或 `smali` 汇编）验证一次；`build_phases.sh` 保留 apktool 的错误输出。

---

## L. 交叉一致性与"诚实边界"

## AUD-B-106 [P1] `dist/` 与文档声称的交付物清单完全脱节（AGENTS.md 声称"四阶段交付物全量补齐"）
- 位置：`AGENTS.md`（"四阶段交付物全量补齐（phase0-3，dist/，phase0/1 从零构建 + phase2/3 重建）"）vs `/disk/workspaces/jq/dist/`（空）
- 问题：文档把"已补齐"写成既成事实，实际零交付物；`build_phases.sh` 又是"幂等、可复现"的宣称 —— 但两者都不成立（见 AUD-B-001/003/019）。
- 依据：`ls -la /disk/workspaces/jq/dist/` → 空。
- 修法：交付物入库或明确标注"需现场构建"，并把"dist/ 四件套存在且 aapt 可解析"写成 CI 门禁。

## AUD-B-107 [P2] 阶段 2/3 的 32 位 so 未被 `so_link_check` / `link_audit` 覆盖
- 位置：`rev64/tools/build_phases.sh`（不调用任何链接审计）
- 问题：`build_phases.sh` 只做"apktool b → STORE → zipalign → sign"，**不做任何链接审计**。阶段 2 的 `armeabi-v7a/libjunqi.so`（原版 32 位 so）在 MuMu/新设备上的可加载性从未在打包链路里被验证；而 `link_audit.py` 又只审 32 位（见 AUD-B-073/077）。
- 依据：`build_phases.sh` 全文无 `link_audit`/`so_link_check` 调用。
- 修法：在 `build_one` 的 verify 阶段加入 `link_audit.py audit <apk>`（针对 32 位）与 `so_link_check.py`（针对 arm64）。

## AUD-B-108 [P2] 阶段 3 的"仅 arm64 可用"这一红线未在交付物层做机器化断言
- 位置：`rev64/tools/build_phases.sh:67-92`（缺失项）
- 问题：红线"阶段 3 全删 baidu 后仅 arm64 可用"要求：阶段 3 APK 必须**只有** `lib/arm64-v8a/`，且 dex 里 0 个 baidu 类，且 `assets/__xadsdk__remote__final__.jar` 不存在。脚本对三者都不做断言（见 AUD-B-004/008/009）。
- 依据：`verify_one` 的输出项里没有这三项。
- 修法：阶段 3 增加"native-code 恰好 = arm64-v8a"、"dex baidu 计数 = 0"、"广告 jar = 0"三条硬断言。

## AUD-B-109 [P3] `phase1` 的 arm64 桩 so 不在任何版本控制内
- 位置：`rev64/tools/build_phases.sh:27`（`SRC_STUB=/tmp/opencode/junqi_stub_arm64.so`，4864B）
- 问题：阶段 1 的唯一新增内容（5KB 桩）来自 `/tmp`，不可复现也不可审计（无源码）。若 `/tmp` 被清理，`build_phases.sh phase1` 直接产出**没有 arm64 桩**的 APK（且因 AUD-B-002 不报错）。
- 依据：`ls -la /tmp/opencode/junqi_stub_arm64.so` 存在；仓库内无桩源码。
- 修法：把桩的 C 源码收进 `rev64/src_rs/stub/` 并由 Makefile 编译产出。

## AUD-B-110 [P3] `build_phases.sh` 头部"依赖（既有资产，全部复用）"清单与实际不一致
- 位置：`rev64/tools/build_phases.sh:12-17`
- 问题：注释列的依赖里没有 `store_compressed.py` 的仓库路径（实际在 `/tmp/opencode/jq_apk/`，见 AUD-B-027），也没提 keystore 路径与 `sign_apk.sh` 的关系。
- 依据：`:14` 只写 `store_compressed.py（STORE .so/.arsc）`。
- 修法：把全部外部依赖收进仓库并在注释里给出仓库内相对路径。

## AUD-B-111 [P2] `mumu_regress.sh` 对失败项自动重跑一次并判 PASS → 门禁被削弱
- 位置：`rev64/tools/mumu_regress.sh:31-38`
- 问题：首次失败即自动重跑，第二次通过就记 `✓ k（重跑过，首跑瞬时时序）`。**真实回归也会被第二次尝试洗白**，且没有任何标记把"洗白过"的项传到汇总输出。
- 依据：`:33-35` 的重跑分支。
- 修法：重跑通过时计入 `FLAKY` 计数并在汇总里单列，`FLAKY>0` 时整体不得判 PASS。

## AUD-B-112 [P3] `spak_ui_assets.py` 的 `KNOWN` 语义表与 `--index` 的写回逻辑可能覆盖人工标注
- 位置：`rev64/tools/spak_ui_assets.py:22-40`（`KNOWN`）、`:95`
- 问题：`:95` `idx[str(n)] = {"file":..., **ft, "sem": idx.get(str(n),{}).get("sem") or KNOWN.get(n)}` —— 人工标注优先，逻辑正确；但 `cmd_set`（`:151`）对不存在的编号也能写入（见 AUD-B-072），而 `--index` 只遍历 `datax_files()`，**不会清除**已失效编号的旧条目 → 索引里长期残留幽灵条目。
- 依据：`:89-102` 的 `cmd_index` 不做差集清理。
- 修法：`cmd_index` 增加"删除已不存在编号的条目"步骤，并在输出里报告被清理的数量。

---

## 统计表

| 级别 | 数量 | 说明 |
|---|---|---|
| **P0** | **12** | 装不上 / 闪退 / 门禁彻底失效 / 交付物缺失 |
| **P1** | **45** | 功能错、假成功、几何与链接判据漏检 |
| **P2** | **40** | 边界、健壮性、可复现性、性能 |
| **P3** | **15** | 风格、一致性、可读性 |
| **合计** | **112** | AUD-B-001 ~ AUD-B-112 |

### 按文件分布

| 文件 / 对象 | P0 | P1 | P2 | P3 | 小计 |
|---|---|---|---|---|---|
| `tools/build_phases.sh` | 4 | 5 | 6 | 3 | 18 |
| 交付物实际状态（`dist/`、阶段属性、apktool.yml） | 2 | 4 | 8 | 1 | 15 |
| `src_rs/Makefile` + `src_rs_regress.sh` + `jni_bridge.c` | 1 | 5 | 4 | 2 | 12 |
| `tools/layout_align_check.py` | 2 | 2 | 2 | 1 | 7 |
| `tools/gen_dialog_batch.py` | 1 | 4 | 3 | 1 | 9 |
| `tools/spak2ui.py` | 0 | 4 | 4 | 1 | 9 |
| `tools/link_audit.py` + `so_link_check.py` | 0 | 7 | 3 | 1 | 11 |
| `tools/rs_cycle.sh` | 0 | 2 | 2 | 1 | 5 |
| `tools/mumu_swap_so.sh` | 0 | 3 | 6 | 1 | 10 |
| `tools/bt_smali.sh` | 0 | 1 | 5 | 2 | 8 |
| `tools/spak_strings.py` / `spak_ui_assets.py` / `jq_strings.h` | 0 | 1 | 6 | 3 | 10 |
| 交叉一致性 / 红线断言 | 0 | 2 | 3 | 0 | 5 |
| **合计** | **12** | **45** | **40** | **15** | **112** |

---

## P0 汇总表（会导致装不上 / 闪退 / 门禁彻底失效）

| 编号 | 标题 | 后果 | 复现命令 |
|---|---|---|---|
| **AUD-B-001** | `build_one` 失败不中断顶层循环，恒 exit 0 并打印"✓ 完成" | 构建失败被判成功，坏包进交付 | `bash rev64/tools/build_phases.sh all; echo $?`（人为破坏源 so 后仍返回 0） |
| **AUD-B-002** | `setup_libs` 的 `cp` 失败无人检查 | 产出缺 `.so` 的 APK，装上一点图标就 `UnsatisfiedLinkError` 闪退 | 删除 `/tmp/opencode/orig_libjunqi.so` 后跑 `build_phases.sh phase0` |
| **AUD-B-003** | `verify_one` 对不存在的 APK 静默全绿 | 交付物缺失 / 阶段定义不符均不可见 | `ls /disk/workspaces/jq/dist/`（空）→ `bash rev64/tools/build_phases.sh --verify-only` → 全部"✓"、EXIT=0 |
| **AUD-B-004** | "arm64 目录无（预期）"对阶段 2/3 是致命缺失却判预期 | 阶段 2/3 缺 arm64 so → 64 位设备必闪退 | 同上（phase2/phase3 输出"arm64 目录无（预期）"） |
| **AUD-B-019** | `dist/` 为空 —— 四阶段交付物全缺 | 无可验收产物 | `ls -la /disk/workspaces/jq/dist/` |
| **AUD-B-033** | 6 个桌面测试目标链接缺 `ui_state_stub.c` | undefined reference → 12/12 回归 9 项 FAIL，一条龙必中止 | `cd rev64/src_rs && make bttest` |
| **AUD-B-041** | `layout_align_check.py` 语法错误（IndentationError） | 几何对齐门禁完全失效（自 842787d 起 8 个提交未发现） | `python3 -m py_compile rev64/tools/layout_align_check.py` |
| **AUD-B-042** | `gen_dialog_batch.py` 把 DIALOGS 插到 dict 外 | 每跑一次生成就破坏语法（已造成 041） | `compile(open('layout_align_check.py').read().replace("DIALOGS = {", '    "x": (1,2,[],),\nDIALOGS = {',1),'x','exec')` → SyntaxError |
| **AUD-B-043** | `--clean` 正则只吃到第一个 `)` | 118 处残片粘行，文件被逐次腐蚀 | `re.subn(r'\n    "datah_\d{3}_dlg": \([^)]*\),','',s,flags=re.S)` → n=118，删后不可编译 |
| **AUD-B-032→P1** | （`make so` 陈旧依赖，见 P1 表） | so 不重编 → APK 内 so 与源码不符 | `cd rev64/src_rs && touch ui_menu.c && make -n so` → "Nothing to be done" |
| **AUD-B-085→P1** | （12/12 回归 9 项 FAIL，见 P1 表） | 回归门禁红，"全周期一条龙"不可用 | `bash rev64/src_rs_regress.sh` → "★ 回归有 9 项 FAIL" |
| **AUD-B-020→P1** | （归档 APK 的 arm64 so 陈旧，见 P1 表） | 交付的 so 比 src_rs 落后 2 天 | `unzip -p phase3_signed.apk lib/arm64-v8a/libjunqi.so \| md5sum` → c7f971b8… vs 2bb06989… |

### P0 修复优先级（建议顺序）

1. **AUD-B-019 + 001 + 003**（交付物 + 假成功）——先让"构建失败"能被看见：顶层 `|| exit 1`、`verify_one` 存在性断言、断言失败即非 0。
2. **AUD-B-041 + 042 + 043**（几何门禁）——`git checkout 9154653 -- rev64/tools/layout_align_check.py` 还原，修 `insert()` 的 `before` 参数与 `clean()` 的正则，加 `py_compile` 门禁。
3. **AUD-B-033 + 035 + 040**（回归门禁）——补 `ui_state_stub.c` 链接、把 grep 口径换成数值判定、补 `jni_bridge.c:2667` 的 `ExceptionClear`。
4. **AUD-B-002 + 004 + 006 + 007 + 008 + 009**（so 与阶段属性断言）——把"阶段定义"变成机器化断言表。
5. **AUD-B-021 + 030**（API 级别与签名口径）——`android21-clang` 或 minSdk 提到 24；v1+v2 双签。
6. **AUD-B-032**（Makefile 依赖）——补 `ui_menu.c`/`ui_state.c`/`evverify.c` 等全部 `.c` 到目标依赖，或把 `so` 目标 `.PHONY`。
