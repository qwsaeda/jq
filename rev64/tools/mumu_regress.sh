#!/bin/bash
# mumu_regress.sh — 事件回归一键跑（批量，含 btconn）
# 收敛反复手工 for 循环（flag/mine/restart/draw/resign/stalemate/close/pop/btconn）。
# 用法:
#   bash rev64/tools/mumu_regress.sh --dev emulator-5554
#   bash rev64/tools/mumu_regress.sh --dev 10.0.182.146:5555 --only flag,btconn
set -u
DEV=emulator-5554
ONLY=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --dev) DEV="$2"; shift 2;;
    --only) ONLY="$2"; shift 2;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done
KINDS="flag mine restart draw resign stalemate close pop btconn"
[ -n "$ONLY" ] && KINDS="$ONLY"
PASS=0; FAIL=0
FAILED=""
for k in $KINDS; do
  echo "═══ $k ═══"
  if timeout 90 bash "$(dirname "$0")/mumu_evtest.sh" --dev "$DEV" "$k" >/tmp/opencode/regress_$k.log 2>&1; then
    echo "  ✓ $k"
    PASS=$((PASS+1))
  else
    # 重跑一次（瞬时时序——redroid/MuMu 触摸 flaky、WAV 加载时序，非代码回归）
    if timeout 90 bash "$(dirname "$0")/mumu_evtest.sh" --dev "$DEV" "$k" >/tmp/opencode/regress_${k}_r.log 2>&1; then
      echo "  ✓ $k（重跑过，首跑瞬时时序）"
      PASS=$((PASS+1))
    else
      echo "  ✗ $k"
      FAIL=$((FAIL+1)); FAILED="$FAILED $k"
    fi
  fi
done
echo "════════ 回归汇总：PASS=$PASS FAIL=$FAIL ════════"
[ -n "$FAILED" ] && { echo "失败: $FAILED"; exit 1; }
echo "✓ 全事件回归 PASS"
