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
set -u
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

# 构建树 lib 布局（每阶段：树名 + lib/子目录→源 so）
# phase0: dec_p0 仅 armeabi=orig
# phase1: dec_p0 armeabi=orig + arm64-v8a=stub
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
  echo "════════ [$phase] apktool b $tree → STORE → zipalign → sign ════════"
  apktool b $tree -o $RA/${phase}_raw.apk >/dev/null 2>&1 || { echo "✗ $phase apktool b 失败"; return 1; }
  python3 /tmp/opencode/jq_apk/store_compressed.py $RA/${phase}_raw.apk $RA/${phase}_stored.apk >/dev/null 2>&1 || { echo "✗ $phase STORE 失败"; return 1; }
  $ZIPALIGN -f 4 $RA/${phase}_stored.apk $RA/${phase}_aligned.apk >/dev/null 2>&1 || { echo "✗ $phase zipalign 失败"; return 1; }
  bash $WS/rev64/sign_apk.sh $RA/${phase}_aligned.apk $RA/${phase}_signed.apk >/dev/null 2>&1 || { echo "✗ $phase 签名失败"; return 1; }
  cp $RA/${phase}_signed.apk $WS/dist/$out
  echo "✓ $out 构建完成"
  if [ $DO_VERIFY -eq 1 ]; then verify_one $phase $out; fi
}

verify_one() {
  local phase=$1 out=$2
  local apk=$WS/dist/$out
  local abi=$(aapt dump badging $apk 2>/dev/null | grep "native-code" | sed 's/native-code: //')
  local minsdk=$(aapt dump badging $apk 2>/dev/null | grep "sdkVersion" | sed 's/sdkVersion: //')
  local adjar=$(unzip -l $apk 2>/dev/null | grep -c "__xadsdk")
  echo "  [$phase] ABI=$abi minSdk=$minsdk 广告jar=$adjar"
  python3 -c "
import zipfile,hashlib
z=zipfile.ZipFile('$apk')
sos=[n for n in z.namelist() if n.endswith('.so')]
for n in sos:
    c=z.getinfo(n).compress_type
    print(f'    {n}: {\"STORE\" if c==0 else \"★压缩\"} {z.getinfo(n).file_size}B')
" 2>/dev/null
  # so 哈希（arm64=src_rs / armeabi=orig / 桩 对账）
  if unzip -p $apk lib/arm64-v8a/libjunqi.so 2>/dev/null | md5sum | grep -q "$(md5sum $SRC_RS | cut -d' ' -f1)"; then
    echo "    arm64 so = src_rs ✓"
  elif unzip -p $apk lib/arm64-v8a/libjunqi.so 2>/dev/null | md5sum | grep -q "$(md5sum $SRC_STUB | cut -d' ' -f1)"; then
    echo "    arm64 so = 4.8KB 桩（阶段 1 预期）"
  elif [ "$(unzip -l $apk 2>/dev/null | grep -c 'lib/arm64-v8a')" = "0" ]; then
    echo "    arm64 目录无（预期）"
  else
    echo "    arm64 so ≠ src_rs（异常）"
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
echo "✓ build_phases 完成（$TARGET）"
