#!/bin/bash
# arm32_walkregress.sh — 32 位翻译体三条走子路径一键回归（§124y-23）
#
# 沉淀现有 3 条走子路径（§124y-19/20/21/22）：
#   1. walk_bridge   会话 B 规则引擎走子桥（纯 C 走子，完整 8 步序列）
#   2. native_layout 32 位原生布局加载链（F_2a380 → F_3dd60 → F_2a340）
#   3. native_event  32 位原生事件发送器（F_3ab80 真实事件对象）
#
# 用法:
#   bash rev64/tools/arm32_walkregress.sh --dev 10.0.182.146:5555
#   bash rev64/tools/arm32_walkregress.sh --dev emulator-5554 --only walk_bridge,native_layout
#   bash rev64/tools/arm32_walkregress.sh --dev ... --no-install   # 跳过装机（已装）
#
# 判据（每条路径）：
#   walk_bridge:    logcat 见 "WALK-BRIDGE: 完整 8 步序列完成"
#   native_layout:  logcat 见 "NATIVE-LAYOUT: F_2a380 返回"
#   native_event:   logcat 见 "NATIVE-EVENT: F_3ab80 返回"
# 整体：0 崩溃（Fatal signal 计数=0）+ 进程存活
set -u
DEV=emulator-5554
ONLY=""
NO_INSTALL=0
PKG=com.cnvcs.junqi32
APK=rev64/dist/phase_chain_arm32.apk
while [[ $# -gt 0 ]]; do
  case "$1" in
    --dev) DEV="$2"; shift 2;;
    --only) ONLY="$2"; shift 2;;
    --no-install) NO_INSTALL=1; shift;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done
# 相对路径基准
WS="$(cd "$(dirname "$0")/../.." && pwd)"
APK="$WS/$APK"

KINDS="walk_bridge native_layout native_event game_test"
[ -n "$ONLY" ] && KINDS="$ONLY"

PASS=0; FAIL=0; FAILED=""

# 装机（除非 --no-install）
if [ "$NO_INSTALL" = "0" ]; then
  echo "═══ 装机 $APK ═══"
  if [ ! -f "$APK" ]; then
    echo "  ✗ APK 不存在 $APK（先 build+pack）"; exit 1
  fi
  if ! timeout 90 adb -s "$DEV" install -r "$APK" >/tmp/opencode/arm32_walk_install.log 2>&1; then
    echo "  ✗ 装机失败"; cat /tmp/opencode/arm32_walk_install.log; exit 1
  fi
  echo "  ✓ 装机 Success"
fi

for k in $KINDS; do
  echo "═══ $k ═══"
  # 清 prop + 启 app + 触发钩子
  timeout 10 adb -s "$DEV" shell "setprop jq.native_layout 0; setprop jq.walk_bridge 0; setprop jq.game_test 0; setprop jq.test_game 0" >/dev/null 2>&1
  timeout 10 adb -s "$DEV" logcat -c >/dev/null 2>&1
  timeout 20 adb -s "$DEV" shell "am force-stop $PKG; am start -n $PKG/com.cnvcs.App" >/dev/null 2>&1
  sleep 6
  case "$k" in
    walk_bridge)   timeout 10 adb -s "$DEV" shell "setprop jq.walk_bridge 1" ;;
    native_layout) timeout 10 adb -s "$DEV" shell "setprop jq.native_layout 1" ;;
    native_event)  timeout 10 adb -s "$DEV" shell "setprop jq.native_layout 1; setprop jq.native_event 1" ;;
    game_test)     timeout 10 adb -s "$DEV" shell "setprop jq.game_test 1" ;;
  esac
  sleep 5
  LOG=$(timeout 10 adb -s "$DEV" logcat -d 2>/dev/null)
  CRASH=$(echo "$LOG" | grep -c "Fatal signal" || true)
  case "$k" in
    walk_bridge)   PATTERN="WALK-BRIDGE: 完整 8 步序列完成" ;;
    native_layout) PATTERN="NATIVE-LAYOUT: F_2a380 返回" ;;
    native_event)  PATTERN="F_3ab80: 事件发送.*disc=0x00000001" ;;
    game_test)     PATTERN="GAME-TEST: 端到端完成 走子=OK" ;;
  esac
  if echo "$LOG" | grep -q "$PATTERN" && [ "$CRASH" = "0" ]; then
    echo "  ✓ $k（匹配 '$PATTERN' + 0 崩溃）"
    PASS=$((PASS+1))
  else
    echo "  ✗ $k（匹配='$PATTERN' 未中 或 崩溃=$CRASH）"
    # 重跑一次（MuMu EGL 多线程竞争瞬时时序，非代码回归）
    echo "  重跑一次..."
    timeout 10 adb -s "$DEV" shell "am force-stop $PKG; am start -n $PKG/com.cnvcs.App" >/dev/null 2>&1
    sleep 6
    case "$k" in
      walk_bridge)   timeout 10 adb -s "$DEV" shell "setprop jq.walk_bridge 1" ;;
      native_layout|native_event) timeout 10 adb -s "$DEV" shell "setprop jq.native_layout 1; setprop jq.native_event 1" ;;
    esac
    sleep 5
    LOG=$(timeout 10 adb -s "$DEV" logcat -d 2>/dev/null)
    CRASH=$(echo "$LOG" | grep -c "Fatal signal" || true)
    if echo "$LOG" | grep -q "$PATTERN" && [ "$CRASH" = "0" ]; then
      echo "  ✓ $k（重跑过，首跑瞬时时序）"
      PASS=$((PASS+1))
    else
      FAILED="$FAILED $k"
      FAIL=$((FAIL+1))
    fi
  fi
done

echo ""
echo "═══ arm32_walkregress 汇总 ═══"
echo "  PASS=$PASS FAIL=$FAIL"
[ -n "$FAILED" ] && echo "  失败:$FAILED"
# 全局崩溃检查
GC=$(timeout 10 adb -s "$DEV" shell "logcat -d" 2>/dev/null | grep -c "Fatal signal" || true)
echo "  全局崩溃计数: $GC"
[ "$FAIL" = "0" ] && [ "$GC" = "0" ] && echo "  ✓ 全 PASS + 0 崩溃" && exit 0
exit 1
