#!/bin/bash
# build_phases.sh — 四阶段交付物"一键构建 + 校验"（批量化固化，防手工重复 5+ 次）
#
# 收敛本会话反复手工做的：apktool b → store_compressed → zipalign → sign → dist/phaseX.apk → 验证。
# 每阶段从构建树（dec_p0/dec_dual/dec_full）用源 so 重置 lib/（幂等，不依赖树残留状态）。
#
# 用法:
#   bash rev64/tools/build_phases.sh              # 构建全部 4 个（phase0-3）
#   bash rev64/tools/build_phases.sh phase2       # 只构建 phase2
#   bash rev64/tools/build_phases.sh all --no-verify  # 跳过验证
#
# 依赖（既有资产，全部复用）:
#   - apktool b（/tmp/opencode/jq_apk/rs_a64/ 下构建树）
#   - store_compressed.py（STORE .so/.arsc）
#   - zipalign（build-tools 36 或 PATH）
#   - sign_apk.sh（v1+v2 双签，junqi.keystore）
#   - 源 so：orig_libjunqi.so（原版 32 位）/ libjunqi_rs.so（src_rs）/ junqi_stub_arm64.so（4.8KB 桩）
set -euo pipefail
WS=/disk/workspaces/jq
RA=/tmp/opencode/jq_apk/rs_a64
P0=/tmp/opencode/jq_apk/dec_p0   # phase0/1 树（原版 dec 副本：baidu 334 类 + 广告 jar 保留 + manifest 广告项删）
KEY_STORE=$WS/junqi.keystore
ZIPALIGN=/disk/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots/741/fs/opt/android/build-tools/36.0.0/zipalign
[ -x "$ZIPALIGN" ] || ZIPALIGN=zipalign
SRC_ORIG=/tmp/opencode/orig_libjunqi.so
SRC_RS=$WS/rev64/src_rs/dist/libjunqi_rs.so
SRC_STUB=/tmp/opencode/junqi_stub_arm64.so
DO_VERIFY=1

# ★ 对抗审计（2026-11-11）假绿根治：build_phases 从不重编 so——若 src_rs 源码已改
#   但 make so 没跑，交付物会打进**旧的** libjunqi_rs.so（APK 与设备跑的行为不一致，
#   真机验收"钩子未命中/文案旧"全是假象）。现构建前强制重编 src_rs so。
#   （幂等：源码无改动时 make 秒过；有改动则必刷新。）
make_so_rs() {
  echo "══════ 重编 src_rs arm64 so（防旧 so 打进交付物）══════"
  ( cd $WS/rev64/src_rs && NDK=/tmp/opencode/android-ndk-r25b make so ) || {
    echo "✗ src_rs so 编译失败（源码有改动，先修再构建）"; return 1; }
}

# 构建树 lib 布局（每阶段：树名 + lib/子目录→源 so）
# phase0: dec_p0 仅 armeabi=orig
# phase1: dec_p0 armeabi=orig + arm64-v8a=stub
# ★ AUD-A2 C-apk: 阶段 0/1 保留广告 jar（32 位 so 硬依赖，AGENTS.md 红线），仅阶段 3 清理
# phase2: dec_dual armeabi-v7a=orig + arm64-v8a=src_rs
# phase3: dec_full arm64-v8a=src_rs
setup_libs() {
  local tree=$1; shift
  rm -rf $tree/lib
  mkdir -p $tree/lib
  while [ $# -gt 0 ]; do
    local abi=$1 src=$2; shift 2
    mkdir -p $tree/lib/$abi
    cp $src $tree/lib/$abi/libjunqi.so
  done
}

# ★ spak_res 注入（render.c 引用的 44 个文件，APK 资产保全）：
#   从 rev64/dist/spak_res/（965 全量，其中 3 张被裁剪版同名覆盖）取
#   spak_render_refs.txt 清单，注入构建树 assets/spak_res/（幂等，rm -rf 旧残留后重拷）。
#   ★ 诚实边界（对抗审计 P0-A2/P3-A1）：当前 so 走文件系统回退（g_assets=NULL，
#   MuMu 转译 AAssetManager 不可达），APK assets 是资产保全非运行时路径；
#   若未来修 g_assets 可达则 APK 自包含生效。
inject_spak_res() {
  local tree=$1
  local refs=$WS/rev64/tools/spak_render_refs.txt
  local src=$WS/rev64/dist/spak_res
  # ★ 对抗审计 P1-A1/P2-A1：refs 文件缺失走 return 1（原跳过 → 构建成功但资源缺失假绿）
  [ -f "$refs" ] || { echo "  ✗ spak_render_refs.txt 缺失"; return 1; }
  local refs_n=$(grep -c . "$refs")   # ★ 局部化（原全局耦合）；grep -c 文件不存在已先守
  local n=0; local miss=0
  rm -rf "$tree/assets/spak_res"      # ★ 对抗审计 P2-A2：幂等清理（防 refs.txt 缩减后残留旧文件）
  mkdir -p "$tree/assets/spak_res"
  while read -r f; do
    [ -z "$f" ] && continue
    if [ -f "$src/$f" ]; then
      cp "$src/$f" "$tree/assets/spak_res/$f" 2>/dev/null && n=$((n+1))
    else
      miss=$((miss+1)); echo "  ⚠ spak_res 源缺 $f（$src/$f 不存在）"
    fi
  done < "$refs"
  [ "$n" -eq "$refs_n" ] || { echo "  ✗ spak_res 注入不全 $n/$refs_n（缺 $miss）"; return 1; }
  echo "  spak_res 注入 $n/$refs_n 文件 → $tree/assets/spak_res/"
}

build_one() {
  local phase=$1 out=$2
  case $phase in
    phase0) setup_libs $P0 armeabi $SRC_ORIG ;;
    phase1) setup_libs $P0 armeabi $SRC_ORIG arm64-v8a $SRC_STUB ;;
    phase2) setup_libs $RA/dec_dual armeabi-v7a $SRC_ORIG arm64-v8a $SRC_RS ;;
    phase3) setup_libs $RA/dec_full arm64-v8a $SRC_RS ;;
    *) echo "未知阶段 $phase"; return 2 ;;
  esac
  local tree
  case $phase in phase0|phase1) tree=$P0;; phase2) tree=$RA/dec_dual;; phase3) tree=$RA/dec_full;; esac
  # ★ 仅 arm64 侧（src_rs so）需要 spak_res 全量资源（32 位 so 走 data*.pak，不用 spak_res）
  case $phase in
    phase2|phase3) inject_spak_res $tree || return 1 ;;
    *) : ;;
  esac
  echo "════════ [$phase] apktool b $tree → STORE → zipalign → sign ════════"
  apktool b $tree -o $RA/${phase}_raw.apk >/tmp/opencode/phase_${phase}_apktool.log 2>&1 || { echo "✗ $phase apktool b 失败"; tail -8 /tmp/opencode/phase_${phase}_apktool.log; return 1; }
  python3 /tmp/opencode/jq_apk/store_compressed.py $RA/${phase}_raw.apk $RA/${phase}_stored.apk >/tmp/opencode/phase_${phase}_store.log 2>&1 || { echo "✗ $phase STORE 失败"; tail -8 /tmp/opencode/phase_${phase}_store.log; return 1; }
  $ZIPALIGN -f 4 $RA/${phase}_stored.apk $RA/${phase}_aligned.apk >/tmp/opencode/phase_${phase}_zipalign.log 2>&1 || { echo "✗ $phase zipalign 失败"; tail -8 /tmp/opencode/phase_${phase}_zipalign.log; return 1; }
  bash $WS/rev64/sign_apk.sh $RA/${phase}_aligned.apk $RA/${phase}_signed.apk >/tmp/opencode/phase_${phase}_sign.log 2>&1 || { echo "✗ $phase 签名失败"; tail -8 /tmp/opencode/phase_${phase}_sign.log; return 1; }
  cp $RA/${phase}_signed.apk $WS/dist/$out
  echo "✓ $out 构建完成"
  if [ $DO_VERIFY -eq 1 ]; then verify_one $phase $out; fi
  return 0
}

verify_one() {
  local phase=$1 out=$2
  local apk=$WS/dist/$out
  # ★ 对抗审计 P1-J4：SRC_RS 缺失时 md5 空 → grep -q "" 恒真（假绿"arm64 so = src_rs ✓"）
  [ -f "$SRC_RS" ] || { echo "  ✗ $out SRC_RS 缺失（$SRC_RS），so 哈希对账无法执行"; return 1; }
  [ -f "$apk" ] || { echo "  ✗ $out APK 不存在"; return 1; }
  local abi=$(aapt dump badging $apk 2>/dev/null | grep "native-code" | sed 's/native-code: //')
  local minsdk=$(aapt dump badging $apk 2>/dev/null | grep "sdkVersion" | sed 's/sdkVersion: //')
  local adjar=$(unzip -l $apk 2>/dev/null | grep -c "__xadsdk")
  echo "  [$phase] ABI=$abi minSdk=$minsdk 广告jar=$adjar"
  # ★ 对抗审计 P2-A3：ABI 判据按 phase 精确匹配期望集合（原只打印不判）——
  #   phase0={armeabi} / phase1={armeabi,arm64-v8a} / phase2={armeabi-v7a,arm64-v8a}
  #   / phase3={arm64-v8a}。phase2 误带 armeabi(v5) 目录（原判据漏检）。
  local expect_abi exp_ok=1
  case $phase in
    phase0) expect_abi="armeabi" ;;
    phase1) expect_abi="armeabi arm64-v8a" ;;
    phase2) expect_abi="armeabi-v7a arm64-v8a" ;;
    phase3) expect_abi="arm64-v8a" ;;
  esac
  for a in $expect_abi; do
    echo "$abi" | grep -q "'$a'" || { echo "  ✗ $phase 缺 ABI $a（实际=$abi）"; exp_ok=0; }
  done
  # 实际 ABI 目录数（APK 内 lib/ 子目录）
  local n_abi_dir=$(unzip -l $apk 2>/dev/null | grep -oE "lib/[^/]+/" | sort -u | wc -l)
  if [ "$n_abi_dir" -ne "$(echo $expect_abi | wc -w)" ]; then
    echo "  ✗ $phase ABI 目录数=$n_abi_dir（期望 $(echo $expect_abi | wc -w)：$expect_abi）"; exp_ok=0
  fi
  [ "$exp_ok" -eq 1 ] || return 1
  # ★ 对抗审计 P1-J3：phase3 去广告终态必须 adjar=0（原只打印不判 → 假绿）
  if [ "$phase" = "phase3" ] && [ "$adjar" != "0" ]; then
    echo "  ✗ phase3 广告jar=$adjar（应 0，去广告终态）"; return 1
  fi
  # ★ 对抗审计 P1-A2：phase2/3 增 spak_res 条目数抽查（对齐 verify_asset_chain.py ①层）
  case $phase in
    phase2|phase3)
      local nspak=$(unzip -l $apk 2>/dev/null | grep -c "assets/spak_res/")
      local refs_n60=$(grep -vE "^\s*#|^\s*$" $WS/rev64/tools/spak_render_refs.txt | grep -c . || true)
      echo "  [$phase] spak_res 条目=$nspak（预期 $refs_n60=refs 清单全量，61=44 PNG/位图+2 棋子 bin+14 WAV+2 原版 datax JPEG（C 项）+1 提和图标图集）"
      ;;
  esac
  python3 -c "
import zipfile,hashlib
z=zipfile.ZipFile('$apk')
sos=[n for n in z.namelist() if n.endswith('.so')]
for n in sos:
    c=z.getinfo(n).compress_type
    print(f'    {n}: {\"STORE\" if c==0 else \"★压缩\"} {z.getinfo(n).file_size}B')
" 2>/dev/null
  # so 哈希（arm64=src_rs / armeabi=orig / 桩 对账）
  # ★ 假红修复（§2026-11-11）：phase0/1 无 arm64 so 时 `unzip -p` 退出码 11
  #   （no matching files）→ pipefail + set -e 直接中止构建（阶段 0 永远构建不完）。
  #   同时空输入的 md5 是 d41d8cd9…，`-n` 判据恒真 → 会被当成"有 so"。
  local so_md5 stub_md5 rs_md5
  local EMPTY_MD5=d41d8cd98f00b204e9800998ecf8427e
  set +e
  so_md5=$(unzip -p $apk lib/arm64-v8a/libjunqi.so 2>/dev/null | md5sum | cut -d' ' -f1)
  set -e
  [ "$so_md5" = "$EMPTY_MD5" ] && so_md5=""
  stub_md5=$( [ -f "$SRC_STUB" ] && md5sum $SRC_STUB | cut -d' ' -f1 || echo "" )
  rs_md5=$( [ -f "$SRC_RS" ] && md5sum $SRC_RS | cut -d' ' -f1 || echo "" )
  if [ -n "$so_md5" ] && [ -n "$rs_md5" ] && [ "$so_md5" = "$rs_md5" ]; then
    echo "    arm64 so = src_rs ✓"
  elif [ -n "$so_md5" ] && [ "$so_md5" = "$stub_md5" ]; then
    echo "    arm64 so = 4.8KB 桩（阶段 1 预期）"
  elif [ "$(unzip -l $apk 2>/dev/null | grep -c 'lib/arm64-v8a')" = "0" ]; then
    echo "    arm64 目录无（预期）"
  else
    echo "    arm64 so ≠ src_rs（异常：so_md5=$so_md5 rs_md5=$rs_md5）"
    # AUD-1000 D6-#17 P0 修：异常分支必须 return 1（旧 RC=0 假绿"verify-only 完成"）
    return 1
  fi
}

# 参数解析
TARGET=all
VERIFY_ONLY=0
for a in "$@"; do case $a in phase0|phase1|phase2|phase3|all) TARGET=$a;; --no-verify) DO_VERIFY=0;; --verify-only) VERIFY_ONLY=1;; *) echo "未知参数 $a"; exit 2;; esac; done

mkdir -p $WS/dist
if [ $VERIFY_ONLY -eq 1 ]; then
  echo "════════ 复核已有交付物（不重建）════════"
  verify_one phase0 phase0.apk
  verify_one phase1 phase1.apk
  verify_one phase2 phase2.apk
  verify_one phase3 phase3.apk
  echo "✓ verify-only 完成"
  exit 0
fi
# 阶段 2/3 用 src_rs so → 构建前先重编（防旧 so 假绿）
case $TARGET in all|phase2|phase3) make_so_rs || exit 1 ;; esac
if [ "$TARGET" = "all" ]; then
  build_one phase0 phase0.apk
  build_one phase1 phase1.apk
  build_one phase2 phase2.apk
  build_one phase3 phase3.apk
else
  case $TARGET in
    phase0) build_one phase0 phase0.apk;;
    phase1) build_one phase1 phase1.apk;;
    phase2) build_one phase2 phase2.apk;;
    phase3) build_one phase3 phase3.apk;;
  esac
fi
# ★ 对抗审计修正（§2026-11-11）：build_one 失败时原脚本**继续往下跑**并最终打印
#   "✓ build_phases 完成" → 失败也报成功（假绿）。现改为任一阶段失败即中止。
echo "✓ build_phases 完成（$TARGET）"
