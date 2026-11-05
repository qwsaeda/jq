#!/bin/bash
# redroid_test.sh — redroid 测试钩子批量验证（§124y-11 固化）
#
# 背景：MuMu（10.0.182.146）宕机期间用 redroid（emulator-5554）做确定性验证。
#   setprop jq.test_game 1 触发 OnTimer 测试钩子 → 构造场景对象 + F_2af44。
# 本脚本把"装机→清 prop→启→触发→抓日志→判 miss→判崩溃"串成一条命令，
# 不再手敲 20 行 shell。
#
# 用法：
#   bash rev64/tools/redroid_test.sh                # 全流程（装+测+统计）
#   bash rev64/tools/redroid_test.sh --no-install   # 跳过装 APK（已装好）
#   bash rev64/tools/redroid_test.sh --wait 10      # 等待 10s（默认 5）
#   配 rev64/tools/so_probe.py 做对象图探查（vtable/dispatch/slot/reloc/strings）：
#     python3 rev64/tools/so_probe.py vtable 0x159c80
#     python3 rev64/tools/so_probe.py dispatch 0xad248
#     python3 rev64/tools/so_probe.py slot 0x159c80 0x28
#
# 判定：
#   - miss 计数（BLX miss off=0x...）
#   - 崩溃计数（Fatal signal / SIGSEGV / SIGABRT）
#   - 进程存活（pidof）
#   - F_a4234 / F_2afa4 关键日志
set -u
WS=/disk/workspaces/jq
DEV=emulator-5554
PKG=com.cnvcs.junqi32
APK=$WS/rev64/dist/phase_chain_arm32.apk
WAIT=5
INSTALL=1
while [ $# -gt 0 ]; do
  case "$1" in
    --no-install) INSTALL=0 ;;
    --wait) shift; WAIT=${1:-5} ;;
    --wait=*) WAIT=${1#--wait=} ;;
    *) echo "未知参数: $1"; exit 2 ;;
  esac
  shift
done

echo "═══ [1] redroid 连通性 ═══"
timeout 10 adb -s $DEV shell echo alive 2>&1 | head -1 || { echo "✗ redroid 不可达"; exit 1; }

if [ $INSTALL -eq 1 ]; then
  echo "═══ [2] 装 APK ═══"
  timeout 60 adb -s $DEV install -r "$APK" 2>&1 | tail -1
fi

echo "═══ [3] 清 prop + logcat + 启 ═══"
timeout 10 adb -s $DEV shell "setprop jq.test_game 0" 2>&1
timeout 10 adb -s $DEV logcat -c 2>&1
timeout 20 adb -s $DEV shell "am force-stop $PKG; am start -n $PKG/com.cnvcs.App" 2>&1 | tail -1
sleep $WAIT

echo "═══ [4] 触发测试钩子 ═══"
timeout 10 adb -s $DEV shell "setprop jq.test_game 1" 2>&1
sleep $WAIT

echo "═══ [5] 抓日志 ═══"
LOG=$(timeout 10 adb -s $DEV logcat -d 2>/dev/null)

echo "═══ [6] 判定 ═══"
MISS=$(echo "$LOG" | grep "BLX miss" | grep -oE "off=0x[0-9a-f]+" | sort | uniq -c | sort -rn)
CRASH=$(echo "$LOG" | grep -cE "Fatal signal|SIGSEGV|SIGABRT")
PID=$(timeout 10 adb -s $DEV shell pidof $PKG 2>/dev/null | tr -d '\r')

echo "─── miss 统计 ───"
echo "${MISS:-（无 miss）}"
echo "─── 崩溃计数: $CRASH ───"
echo "─── 进程: ${PID:-DEAD} ───"
echo "─── F_2afa4 关键日志 ───"
echo "$LOG" | grep -E "F_2afa4: this|gs8=" | head -3
echo "─── GS8 诊断（gs[8] 真实 vtable，§124y-13）───"
echo "$LOG" | grep "TEST-GAME GS8" | head -2
echo "─── F_a4234 日志 ───"
echo "$LOG" | grep "F_a4234:" | head -3

# 退出码：崩溃或进程死亡 = 失败
if [ "$CRASH" -gt 0 ] || [ -z "$PID" ]; then
  echo "✗ FAIL（崩溃或进程死亡）"
  exit 1
fi
echo "✓ PASS（进程存活，0 崩溃）"
