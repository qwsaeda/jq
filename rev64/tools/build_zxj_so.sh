#!/usr/bin/env bash
# build_zxj_so.sh — 用本会话最新 C 源码（含 movetest=60 等新钩子）重编 com.cnvcs 前缀 so，换进 zxj APK
#
# 定位：zxj_junqirs.apk（build_zxj_apk.sh 伪改名产物）内的 so 是旧版（6a1a1139，
#   只认 movetest=1 不认 60），而本会话 src_rs 最新 so（05ce20b0）有 movetest=60，
#   但导出 com.qwsaeda 前缀（正向演进）——zxj APK 的 smali 是 com.cnvcs 包，
#   native 符号必须 Java_com_cnvcs_* 才匹配。
#   → 本脚本：源码临时副本 sed com.qwsaeda→com.cnvcs → NDK 编译 → 换 so → 重打包签名。
#
# ★ 不动原源码树（AGENTS 红线：src_rs 包名随正向会话演进，本会话不改）
# 用法：bash rev64/tools/build_zxj_so.sh [--install] [--dev 10.0.182.152:5555]
set -uo pipefail
WS=/disk/workspaces/jq
SRC=$WS/rev64/forward/src_rs
NDK=/tmp/opencode/android-ndk-r25b
TMP=/tmp/opencode/zxj_so_build
ZO=/tmp/opencode/zxj_so_new.so
OUT=$WS/rev64/dist/zxj_junqirs_v2.apk
INSTALL=0
DEV=10.0.182.152:5555
while [ $# -gt 0 ]; do
  case "$1" in
    --install) INSTALL=1; shift;;
    --dev) DEV="$2"; shift 2;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done

echo "════════ 本会话 C so（com.cnvcs 前缀）重编 + 换进 zxj APK ════════"
rm -rf "$TMP" && mkdir -p "$TMP"
cp -a "$SRC"/. "$TMP"/ || exit 1
# sed 改包名（FindClass 字符串 + JNI 导出符号 + 任何 com/qwsaeda 引用）
n=0
for f in "$TMP"/*.c "$TMP"/*.h; do
  [ -f "$f" ] || continue
  if grep -q "com/qwsaeda\|com_qwsaeda\|com.qwsaeda" "$f" 2>/dev/null; then
    sed -i 's|com/qwsaeda|com/cnvcs|g; s|com_qwsaeda|com_cnvcs|g; s|com\.qwsaeda|com.cnvcs|g' "$f"
    n=$((n+1))
  fi
done
echo "  源码改前缀文件数: $n"

# 编译（复用 Makefile 的 so 目标规则，在临时树里 make）
cd "$TMP" || exit 1
make clean >/dev/null 2>&1 || true
NDK="$NDK" make so >/tmp/opencode/zxj_so_build.log 2>&1
if [ $? -ne 0 ]; then
  echo "✗ 编译失败:"; grep -E "error:" /tmp/opencode/zxj_so_build.log | head -5; exit 1
fi
cp "$TMP/dist/libjunqi_rs.so" "$ZO"
echo "  so 编译完成: $(stat -c%s "$ZO")B md5=$(md5sum "$ZO"|cut -d' ' -f1)"
echo "  JNI 导出: $(nm -D "$ZO" | grep -c 'Java_com_cnvcs_App_') 个 Java_com_cnvcs_App_*"
QRES=$(nm -D "$ZO" 2>/dev/null | grep -c "Java_com_qwsaeda" || true)
[ "$QRES" = "0" ] || { echo "✗ 仍有 qwsaeda 符号残留（$QRES）"; exit 1; }
HAS60=$(strings "$ZO" 2>/dev/null | grep -c "movetest=60" || true)
[ "${HAS60:-0}" -gt 0 ] || { echo "✗ movetest=60 钩子未编入（has=$HAS60）"; exit 1; }
echo "  ✓ movetest=60 钩子在位"

# 换 so 进 zxj APK 解包树（用 build_zxj_apk.sh 的干净解包）
WORK=/tmp/opencode/jq_apk/dec_zxj_v2
rm -rf "$WORK"
apktool d -f "$WS/rev64/dist/zxj_junqirs.apk" -o "$WORK" >/dev/null 2>&1 || exit 1
cp "$ZO" "$WORK/lib/arm64-v8a/libjunqi.so"
apktool b -f "$WORK" -o /tmp/opencode/zxj_v2_unsigned.apk >/dev/null 2>&1 || exit 1
python3 "$WS/rev64/tools/store_compressed.py" /tmp/opencode/zxj_v2_unsigned.apk /tmp/opencode/zxj_v2_stored.apk >/dev/null 2>&1 || exit 1
zipalign -f -p 4 /tmp/opencode/zxj_v2_stored.apk /tmp/opencode/zxj_v2_aligned.apk || exit 1
apksigner sign --ks "$WS/junqi.keystore" --ks-key-alias junqi \
  --ks-pass pass:junqi2026 --key-pass pass:junqi2026 \
  --v1-signing-enabled true --v2-signing-enabled true \
  --out "$OUT" /tmp/opencode/zxj_v2_aligned.apk || exit 1
echo "  ✓ APK: $OUT ($(stat -c%s "$OUT")B)"
aapt dump badging "$OUT" 2>/dev/null | head -1

if [ "$INSTALL" -eq 1 ]; then
  echo "── 装机 + 60 局钩子验证 ──"
  adb -s "$DEV" install -r "$OUT" 2>&1 | tail -1
  adb -s "$DEV" shell "am force-stop com.zxj.junqirs; sleep 1; setprop jq.movetest 60; am start -n com.zxj.junqirs/com.cnvcs.App" >/dev/null 2>&1
  sleep 18
  LOG=$(adb -s "$DEV" logcat -d 2>/dev/null | grep -E "movetest=60|完整 60 局" | tail -2)
  echo "  $LOG"
  echo "$LOG" | grep -q "完整 60 局" && echo "判据: PASS（本会话 C 版 60 局真机钩子）" || echo "判据: FAIL（查真值）"
fi
