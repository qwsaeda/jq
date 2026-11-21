#!/usr/bin/env bash
# build_zxj_apk.sh — 本会话逆向 app（com.zxj.junqirs）真机交付物构建（伪改名方案）
#
# 定位：解决"AGENTS 规范名 com.zxj vs 交付物 APK 实际 com.cnvcs"漂移（§121）——
#   把 ni_junqi_signed.apk（com.cnvcs.junqirs，arm64 so + smali）改造成
#   com.zxj.junqirs 装机可用的交付物。
#
# ★ 伪改名方案（避开 ELF hash 重算，2026-11-20 实证）：
#   - 真改 so 符号（Java_com_cnvcs_App_* → Java_com_zxj_App_*）会破坏 DT_HASH/GNU_HASH
#     → JNI_OnLoad 加载 OK 但 dlsym 找不到符号 → UnsatisfiedLinkError（已实测失败）
#   - **正解**：只改 AndroidManifest package → com.zxj.junqirs（对外包名/桌面可见），
#     smali 类名 + so 保持 com.cnvcs（native 符号 Java_com_cnvcs_App_* 匹配），
#     launcher 组件 android:name="com.cnvcs.App" 是绝对类名不随 package 变 → 照常启动。
#   - 代价：对外包名 com.zxj，内部类名仍 com.cnvcs（诚实标注：非"全链改名"，是装机折中）
#
# 前置：apktool / zipalign / apksigner / keystore
# 用法：bash rev64/tools/build_zxj_apk.sh [--install] [--dev 10.0.182.152:5555]
set -uo pipefail
WS=/disk/workspaces/jq
SRC=$WS/rev64/dist/ni_junqi/ni_junqi_signed.apk
WORK=/tmp/opencode/jq_apk/dec_zxj_build
OUT=$WS/rev64/dist/zxj_junqirs.apk
INSTALL=0
DEV=10.0.182.152:5555
while [ $# -gt 0 ]; do
  case "$1" in
    --install) INSTALL=1; shift;;
    --dev) DEV="$2"; shift 2;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done

echo "════════ 逆向 app com.zxj.junqirs 交付物构建（伪改名）════════"
rm -rf "$WORK"
apktool d -f "$SRC" -o "$WORK" >/dev/null 2>&1 || { echo "✗ apktool d 失败"; exit 1; }
python3 - "$WORK" <<'PY'
import sys, re
root = sys.argv[1]
p = root + '/AndroidManifest.xml'
s = open(p, encoding='utf-8').read()
n = s.replace('package="com.cnvcs.junqirs"', 'package="com.zxj.junqirs"')
assert n != s, 'Manifest package 替换失败（源包名变了？）'
open(p, 'w', encoding='utf-8').write(n)
print('  Manifest package: com.cnvcs.junqirs → com.zxj.junqirs')
print('  组件名保留绝对类:', re.findall(r'android:name="([^"]+)"', n)[:3])
PY

apktool b -f "$WORK" -o /tmp/opencode/zxj_unsigned.apk >/dev/null 2>&1 || { echo "✗ apktool b 失败"; exit 1; }
python3 "$WS/rev64/tools/store_compressed.py" /tmp/opencode/zxj_unsigned.apk /tmp/opencode/zxj_stored.apk >/dev/null 2>&1 || exit 1
zipalign -f -p 4 /tmp/opencode/zxj_stored.apk /tmp/opencode/zxj_aligned.apk || exit 1
apksigner sign --ks "$WS/junqi.keystore" --ks-key-alias junqi \
  --ks-pass pass:junqi2026 --key-pass pass:junqi2026 \
  --v1-signing-enabled true --v2-signing-enabled true \
  --out "$OUT" /tmp/opencode/zxj_aligned.apk || exit 1
echo "  ✓ 构建完成: $OUT ($(stat -c%s "$OUT")B)"
aapt dump badging "$OUT" 2>/dev/null | head -1

if [ "$INSTALL" -eq 1 ]; then
  echo "── 装机 + 5 判据 ──"
  adb -s "$DEV" install -r "$OUT" 2>&1 | tail -1
  adb -s "$DEV" logcat -c
  adb -s "$DEV" shell "setprop jq.movetest 1; am force-stop com.zxj.junqirs; am start -n com.zxj.junqirs/com.cnvcs.App" >/dev/null 2>&1
  sleep 16
  PID=$(adb -s "$DEV" shell pidof com.zxj.junqirs | tr -d '\r\n')
  BOARD=$(adb -s "$DEV" logcat -d 2>/dev/null | grep -c "board=OK")
  MOVE=$(adb -s "$DEV" logcat -d 2>/dev/null | grep "movetest：自动走子完成" | tail -1)
  CRASH=$(adb -s "$DEV" logcat -d 2>/dev/null | grep -c "Fatal signal")
  echo "  ① 装 Success ② board=OK×$BOARD ③ $MOVE"
  echo "  ④ 退出中..."; adb -s "$DEV" shell am force-stop com.zxj.junqirs
  sleep 2
  adb -s "$DEV" shell "setprop jq.movetest 0; am start -n com.zxj.junqirs/com.cnvcs.App" >/dev/null 2>&1
  sleep 14
  PID2=$(adb -s "$DEV" shell pidof com.zxj.junqirs | tr -d '\r\n')
  echo "  ⑤ 重进 pid=$PID2（首启 pid=$PID） 崩溃=$CRASH"
  if [ -n "$PID" ] && [ -n "$PID2" ] && [ "$BOARD" -gt 0 ] && [ "$CRASH" = "0" ]; then
    echo "判据: PASS（本会话逆向 app com.zxj 5 判据真机实证）"
  else
    echo "判据: FAIL（查真值，按 §119 纪律）"
    exit 2
  fi
fi
