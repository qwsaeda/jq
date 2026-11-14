#!/bin/bash
# mumu_align_smoke.sh — 设备端对齐冒烟（走子/翻子 tap 序列 + logcat 判读）
# 固化：127.0.0.1:5556 明棋走子 + 暗棋翻子全链。flaky 容错：段失败自动重跑 1 次
# （对齐 mumu_regress.sh 既有口径；redroid 转译冷启动固有序时抖动，单跑偶发
# FAIL 非脚本逻辑错）。
# 诚实边界（§124y-279/189）：设备不可达 → PENDING 非 FAIL；
#   redroid12 冒烟口径，非红果 10.0.182.146 权威（不外推真机）。
# 用法：bash mumu_align_smoke.sh [--dev <addr>] [--only move|flip|all]
# 几何（view=720×1184，g_touch_s=0.925）：设备坐标 = 基准 720×1280 × 0.925。
set -u
DEV="127.0.0.1:5556"; PKG="com.qwsaeda.junqirs"; ACT="com.qwsaeda.junqirs/com.qwsaeda.App"
ONLY="all"
while [ $# -gt 0 ]; do
  case "$1" in
    --dev) DEV="$2"; shift 2;;
    --only) ONLY="$2"; shift 2;;
    *) shift;;
  esac
done
ADB="adb -s $DEV"
echo "=== 设备端对齐冒烟（$DEV，$PKG）==="
if ! timeout 15 $ADB shell "echo ok" >/dev/null 2>&1; then
  echo "判定: PENDING（设备 $DEV 不可达，§124y-189 降级非 FAIL）"; exit 0
fi
log() { timeout 20 $ADB shell "logcat -d -t $1" 2>/dev/null; }
tap() { $ADB shell "input tap $1 $2" >/dev/null 2>&1; sleep 2; }
wait_screen() {
  for i in $(seq 1 30); do log 200 | grep -q "rs_draw_menu 首帧" && return 0; sleep 1; done
  return 1
}
# 段函数（返回 0=命中）：开局确认 25s 窗口（转译冷启动）+ 重 tap 兜底
smoke_move() {
  $ADB shell "am force-stop $PKG; logcat -c" >/dev/null 2>&1
  $ADB shell "am start -n $ACT" >/dev/null 2>&1
  wait_screen || return 1
  tap 332 837; tap 324 558
  opened=0
  for i in $(seq 1 25); do sleep 1; log 60 | grep -q "布局明棋" && { opened=1; break; }; done
  [ $opened -eq 0 ] && { tap 324 558; sleep 3; }
  tap 78 774; tap 78 529; sleep 3
  log 200 | grep -qE "can_move=1"
}
smoke_flip() {
  $ADB shell "am force-stop $PKG; logcat -c" >/dev/null 2>&1
  $ADB shell "am start -n $ACT" >/dev/null 2>&1
  wait_screen || return 1
  tap 332 837; tap 458 488
  opened=0
  for i in $(seq 1 25); do sleep 1; log 60 | grep -q "翻棋暗棋" && { opened=1; break; }; done
  [ $opened -eq 0 ] && { tap 458 488; sleep 3; }
  tap 203 1023; tap 203 1023; sleep 4
  log 200 | grep -qE "can_flip=1|走子判定 翻棋"
}
# 段执行（flaky 容错：失败重跑 1 次）
run_seg() {
  local fn="$1" label="$2"
  if $fn; then echo "  $label: PASS"; return 0
  elif $fn; then echo "  $label: PASS（重跑 1 次命中，flaky 容错）"; return 0
  else echo "  $label: FAIL"; return 1; fi
}
cm=0; cf=0
if [ "$ONLY" = "all" ] || [ "$ONLY" = "move" ]; then
  echo "-- 明棋走子（cell25→cell30，默认布局唯一合法目标）--"
  run_seg smoke_move "明棋走子 can_move=1" && cm=1
fi
if [ "$ONLY" = "all" ] || [ "$ONLY" = "flip" ]; then
  echo "-- 暗棋翻子（cell6 两击 + AI 应着翻棋）--"
  run_seg smoke_flip "暗棋翻子 can_flip=1" && cf=1
fi
echo ""
echo "=== 冒烟判读 ==="
echo "  明棋走子 can_move=1: $([ $cm -eq 1 ] && echo PASS || echo FAIL)"
echo "  暗棋翻子 can_flip=1: $([ $cf -eq 1 ] && echo PASS || echo FAIL)"
crash=$(log 500 | grep -c "Fatal signal")
echo "  全链崩溃 Fatal signal: $crash（期望 0）"
[ "$crash" -gt 0 ] && { echo "判定: FAIL（崩溃）"; exit 1; }
if [ "$cm" -eq 1 ] && [ "$cf" -eq 1 ]; then
  echo "判定: PASS（$DEV 冒烟口径；红果 10.0.182.146 恢复后权威 5 判据复验）"
else
  echo "判定: FAIL（flaky 重跑后仍未命中，查设备状态/几何换算 §16/§19）"; exit 1
fi
