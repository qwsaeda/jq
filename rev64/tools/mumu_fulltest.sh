#!/bin/bash
# mumu_fulltest.sh — 真机全套验证一条龙（走子 + FLAG + MINE + 重开）
# 固化：每轮手写"走子→事件→重开"组合验证 → 一条命令。
# 用法:
#   bash rev64/tools/mumu_fulltest.sh             # 走子 EAT + FLAG + MINE + 重开 + 和棋 + 认输 + 困毙 + close/pop（全 9 项）
#   bash rev64/tools/mumu_fulltest.sh --skip-walk # 跳过走子（只事件+重开）
#   bash rev64/tools/mumu_fulltest.sh --dev IP:PORT
# 判据: 全项 PASS 才 exit 0（走子 EAT 事件 + FLAG ev=3 + MINE ev=5 + 重开 stage=PLAY + 和棋 + 认输 + 困毙 + close/pop 音效）
set -u
DEV=${DEV:-10.0.182.146:5555}
TOOLS=/disk/workspaces/jq/rev64/tools
SKIP_WALK=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --skip-walk) SKIP_WALK=1; shift;;
    --dev) DEV="$2"; shift 2;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done
log() { echo "[fulltest] $*"; }
FAILED=()
run() {
  local name=$1; shift
  log "=== $name ==="
  if "$@"; then log "✓ $name PASS"; else log "★ $name FAIL"; FAILED+=("$name"); fi
}

if [[ $SKIP_WALK -eq 0 ]]; then
  run "走子 EAT（mumu_walktest --restart --check-turn）" \
    bash $TOOLS/mumu_walktest.sh --restart --seq "25,30" --expect "Eat" --check-turn --dev $DEV
else
  log "跳过走子（--skip-walk）"
fi
run "FLAG（mumu_evtest flag）" bash $TOOLS/mumu_evtest.sh flag --dev $DEV
run "MINE（mumu_evtest mine）" bash $TOOLS/mumu_evtest.sh mine --dev $DEV
run "游戏内重开（mumu_evtest restart）" bash $TOOLS/mumu_evtest.sh restart --dev $DEV
run "和棋（mumu_evtest draw）" bash $TOOLS/mumu_evtest.sh draw --dev $DEV
run "认输（mumu_evtest resign）" bash $TOOLS/mumu_evtest.sh resign --dev $DEV
run "困毙（mumu_evtest stalemate）" bash $TOOLS/mumu_evtest.sh stalemate --dev $DEV
run "UI 关闭音效（mumu_evtest close）" bash $TOOLS/mumu_evtest.sh close --dev $DEV
run "UI 弹窗音效（mumu_evtest pop）" bash $TOOLS/mumu_evtest.sh pop --dev $DEV

echo
if [ ${#FAILED[@]} -eq 0 ]; then
  log "★ 真机全套验证 PASS（$([ $SKIP_WALK -eq 1 ] && echo 8 || echo 9)/$([ $SKIP_WALK -eq 1 ] && echo 8 || echo 9)）"
  exit 0
else
  log "★ 真机全套验证 FAIL: ${FAILED[*]}"
  exit 1
fi
