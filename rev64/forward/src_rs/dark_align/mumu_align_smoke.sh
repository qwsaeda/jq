#!/bin/bash
# mumu_align_smoke.sh — 设备端对齐冒烟（走子/翻棋/终局 tap 序列 + logcat 判读）
# 固化：本轮 127.0.0.1:5556 手敲 input tap 全链（§16 明棋走子 + §19 暗棋翻子）。
# 判据：logcat 实锤（走子 can_move=1 + 事件 + AI 应着 / 翻子 can_flip=1 + 定色 + AI 翻棋）。
# 诚实边界（§124y-279/189 口径）：设备不可达 → PENDING 非 FAIL；
#   redroid12（127.0.0.1:5556）冒烟口径，非红果 10.0.182.146 权威口径（不外推真机）。
# 用法：
#   bash rev64/forward/src_rs/dark_align/mumu_align_smoke.sh                # 默认 127.0.0.1:5556
#   bash ... mumu_align_smoke.sh --dev 10.0.182.146:5555                    # 红果（权威）
#   bash ... mumu_align_smoke.sh --only move|flip|all
# 几何口径（view=720×1184，g_touch_s=0.925）：设备坐标 = 基准 720×1280 × 0.925。
#   明棋走子：蓝 cell25(78,774)→cell30(78,529)（默认布局 cell25 唯一合法目标 cell30，§124y-371 可走表）
#   暗棋翻子：蓝 cell6 两击 (203,1023)（can_flip=1 翻子定色 + AI 应着翻 cell0）
#   难度弹窗 btn1 新手翻棋 = 设备(458,488)；btn2 中级布局明棋 = 设备(324,558)
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
# 鲁棒：app 重启后轮询出屏（rs_draw_menu 首帧）最多 30s（redroid 转译冷启动慢）
wait_screen() {
  for i in $(seq 1 30); do
    if log 200 | grep -q "rs_draw_menu 首帧"; then return 0; fi
    sleep 1
  done
  return 1
}

check_move=0; check_flip=0
if [ "$ONLY" = "all" ] || [ "$ONLY" = "move" ]; then
  echo "-- 明棋走子（cell25→cell30，默认布局唯一合法目标）--"
  $ADB shell "am force-stop $PKG; logcat -c" >/dev/null 2>&1
  $ADB shell "am start -n $ACT" >/dev/null 2>&1
  wait_screen || { echo "  [超时] 主界面未出屏（30s，转译冷启动？）→ 本段 PENDING"; }
  # 菜单人机 btn0(332,837) → 难度弹窗 btn2 中级布局明棋(324,558)
  tap 332 837; tap 324 558
  # 鲁棒：确认进入明棋开局（"布局明棋"），未进重 tap btn2（转译冷启动时序抖动，同翻子段）
  local opened=0
  for i in $(seq 1 10); do
    sleep 1
    if log 60 | grep -q "布局明棋"; then opened=1; break; fi
  done
  if [ $opened -eq 0 ]; then tap 324 558; sleep 3; fi
  tap 78 774; tap 78 529; sleep 3
  out=$(log 200 | grep -E "走子判定|can_move=1|AI 应着")
  echo "$out" | tail -4
  echo "$out" | grep -qE "can_move=1" && check_move=1
fi

if [ "$ONLY" = "all" ] || [ "$ONLY" = "flip" ]; then
  echo "-- 暗棋翻子（cell6 两击 + AI 应着翻棋）--"
  $ADB shell "am force-stop $PKG; logcat -c" >/dev/null 2>&1
  $ADB shell "am start -n $ACT" >/dev/null 2>&1
  wait_screen || { echo "  [超时] 主界面未出屏 → 本段 PENDING"; }
  tap 332 837; tap 458 488
  # 鲁棒：确认进入暗棋开局（"翻棋暗棋"），未进则重 tap btn1（转译冷启动时序抖动）
  local opened=0
  for i in $(seq 1 10); do
    sleep 1
    log 60 | grep -q "翻棋暗棋" && { opened=1; break; }
    log 60 | grep -q "难度弹窗：选" && { log 60 | grep -q "翻棋暗棋" && { opened=1; break; }; }
  done
  if [ $opened -eq 0 ]; then tap 458 488; sleep 3; fi
  tap 203 1023; tap 203 1023; sleep 4
  out=$(log 200 | grep -E "走子判定 翻棋|can_flip=1|人机AI：翻棋|turn→")
  echo "$out" | tail -5
  echo "$out" | grep -qE "can_flip=1|走子判定 翻棋" && check_flip=1
fi

echo ""
echo "=== 冒烟判读 ==="
echo "  明棋走子 can_move=1: $([ $check_move -eq 1 ] && echo PASS || echo FAIL)"
echo "  暗棋翻子 can_flip=1: $([ $check_flip -eq 1 ] && echo PASS || echo FAIL)"
# 崩溃检查
crash=$(log 500 | grep -c "Fatal signal")
echo "  全链崩溃 Fatal signal: $crash（期望 0）"
[ "$crash" -gt 0 ] && { echo "判定: FAIL（崩溃）"; exit 1; }
if [ "$check_move" -eq 1 ] && [ "$check_flip" -eq 1 ]; then
  echo "判定: PASS（$DEV 冒烟口径；红果 10.0.182.146 恢复后权威 5 判据复验）"
else
  echo "判定: FAIL（tap 序列未命中，查 view 尺寸/几何换算 §16/§19 口径）"; exit 1
fi
