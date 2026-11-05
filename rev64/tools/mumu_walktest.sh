#!/bin/bash
# mumu_walktest.sh — 真机走子一条龙（触摸重试 + 事件验证 + 日志抓取 + 进程检查）
# 固化：每轮手写 adb 走子脚本（force-stop→菜单→选 cell→走 cell→抓日志）→ 一条命令。
# 用法:
#   bash rev64/tools/mumu_walktest.sh --restart --seq "25,30 28,33"
#   bash rev64/tools/mumu_walktest.sh --seq "27,32" --expect Bomb
#   bash rev64/tools/mumu_walktest.sh 25,30           # 裸 from,to 追加到 seq
# 参数:
#   --restart   先 force-stop + 重启 + 菜单进对局（否则复用当前对局）
#   --seq "f,t f,t ..."  批量走子序列（每对一步：选 f → 走 t）
#   --expect EVENT   验证最后一步事件（EAT/BOMB/MOVE/FLAG，大小写不敏感）
#   --check-turn   验证 turn 交替持续（"turn 切回蓝方" ≥ SEQ 步数，红 AI 应着闭环）
#   --dev IP:PORT   默认 10.0.182.146:5555
# 坐标: 调 walk_coords.py（cell→tap 公式固化）
# 判据: 每步"选中 from + 走 to"日志出现（多轮重试，触摸 flaky 环境限制）
set -u
DEV=${DEV:-10.0.182.146:5555}
TOOLS=/disk/workspaces/jq/rev64/tools
RESTART=0
EXPECT=""
CHECK_TURN=0
SEQ=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --restart) RESTART=1; shift;;
    --expect)  EXPECT="$2"; shift 2;;
    --check-turn) CHECK_TURN=1; shift;;
    --seq)     SEQ+=($2); shift 2;;
    --dev)     DEV="$2"; shift 2;;
    *)         SEQ+=($1); shift;;
  esac
done
if [[ ${#SEQ[@]} -eq 0 ]]; then echo "用法: $0 [--restart] [--seq \"f,t ...\"] [--expect EVENT] [--check-turn]"; exit 2; fi

log() { echo "[walktest] $*"; }

# 触摸重试直到日志出现 pattern（多轮，触摸 flaky）
tap_until() { # tap_until x y pattern [max]
  local x=$1 y=$2 pat=$3 max=${4:-6}
  for i in $(seq 1 $max); do
    adb -s $DEV shell input tap $x $y >/dev/null 2>&1
    sleep 2
    if adb -s $DEV logcat -d 2>/dev/null | grep -qi "$pat"; then return 0; fi
  done
  return 1
}

# 重启 + 菜单进对局
if [[ $RESTART -eq 1 ]]; then
  log "重启 app（force-stop + am start）"
  adb -s $DEV shell am force-stop com.cnvcs.junqirs >/dev/null 2>&1
  sleep 3
  adb -s $DEV logcat -c
  adb -s $DEV shell am start -n com.cnvcs.junqirs/com.cnvcs.App >/dev/null 2>&1
  # ★ 全面屏：启动 GL 初始化可 >10s，轮询菜单渲染就绪（rs_draw_menu 首帧）再 tap
  for w in $(seq 1 20); do
    sleep 1
    if adb -s $DEV logcat -d 2>/dev/null | grep -q "rs_draw_menu 首帧"; then break; fi
  done
  sleep 2
  log "菜单进对局（tap 448,1089 人机按钮；★ 原版 UI 对齐：人机 → 直开局，无难度弹窗）"
  tap_until 448 1089 "OnTouch 菜单进对局" 6 || { log "★ 菜单进对局失败（触摸 flaky）"; exit 1; }
fi

# 批量走子序列
STEP=0
for pair in "${SEQ[@]}"; do
  STEP=$((STEP+1))
  from=${pair%,*}; to=${pair#*,}
  # 坐标（walk_coords.py --seq 输出）
  coords=$(python3 $TOOLS/walk_coords.py --seq "$from,$to" | head -1)
  fx=$(echo "$coords" | grep -oE 'tap\([0-9]+' | head -1 | grep -oE '[0-9]+')
  fy=$(echo "$coords" | grep -oE ',[0-9]+\)' | head -1 | grep -oE '[0-9]+')
  tx=$(echo "$coords" | grep -oE 'tap\([0-9]+' | tail -1 | grep -oE '[0-9]+')
  ty=$(echo "$coords" | grep -oE ',[0-9]+\)' | tail -1 | grep -oE '[0-9]+')
  log "步骤$STEP: cell$from→cell$to（tap($fx,$fy)→tap($tx,$ty)）"
  tap_until $fx $fy "from=$from" 6 || { log "★ 选中 cell$from 失败"; exit 1; }
  tap_until $tx $ty "from=$from to=$to" 6 || { log "★ 走子 cell$from→$to 失败"; exit 1; }
  log "步骤$STEP: cell$from→$to 成功"
done

# 期望事件验证 + 日志 + 进程
if [[ -n "$EXPECT" ]]; then
  if adb -s $DEV logcat -d 2>/dev/null | grep -qi "$EXPECT"; then
    log "✓ 期望事件 $EXPECT 出现"
  else
    log "★ 期望事件 $EXPECT 未出现"; exit 1
  fi
fi
# turn 交替持续验证（红 AI 应着 → "turn 切回蓝方" ≥ SEQ 步数）
if [[ $CHECK_TURN -eq 1 ]]; then
  TURNS=$(adb -s $DEV logcat -d 2>/dev/null | grep "junqi_rs" | grep -c "turn 切回蓝方")
  if [[ "$TURNS" -ge "${#SEQ[@]}" ]]; then
    log "✓ turn 交替持续（turn 切回蓝方 $TURNS 次 ≥ ${#SEQ[@]} 步）"
  else
    log "★ turn 交替中断（turn 切回蓝方 $TURNS 次 < ${#SEQ[@]} 步）"; exit 1
  fi
fi
log "=== 走子/事件日志 ==="
adb -s $DEV logcat -d 2>/dev/null | grep "junqi_rs" | grep -v chatty | grep -iE "走子判定|红方 AI|sfx：播放" | tail -20
log "=== 进程 ==="
adb -s $DEV shell pidof com.cnvcs.junqirs >/dev/null 2>&1 && log "✓ 进程存活 $(adb -s $DEV shell pidof com.cnvcs.junqirs)" || { log "★ 进程死了"; exit 1; }
log "✓ 走子一条龙完成（${#SEQ[@]} 步）"
exit 0
