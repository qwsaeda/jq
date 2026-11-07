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
FLAKY=""
FLAKY_N=0
for k in $KINDS; do
  echo "═══ $k ═══"
  if timeout 90 bash "$(dirname "$0")/mumu_evtest.sh" --dev "$DEV" "$k" >/tmp/opencode/regress_$k.log 2>&1; then
    echo "  ✓ $k"
    PASS=$((PASS+1))
  else
    # 重跑一次（瞬时时序——redroid/MuMu 触摸 flaky、WAV 加载时序，非代码回归）
    # ★ 对抗审计 P1-J5（flaky 假绿修正）：重跑 PASS 单独计 FLAKY（非 PASS）——
    #   原"1 FAIL + 1 PASS 即记 PASS"会把偶发竞态/真回归掩盖。flaky 项供人工
    #   复核（连续 2 次 PASS 才可信），不计入"全事件回归 PASS"总数。
    if timeout 90 bash "$(dirname "$0")/mumu_evtest.sh" --dev "$DEV" "$k" >/tmp/opencode/regress_${k}_r.log 2>&1; then
      echo "  ~ $k（FLAKY：首跑瞬时时序，重跑过——不计 PASS，供人工复核）"
      FLAKY="$FLAKY $k"; FLAKY_N=$((FLAKY_N+1))
    else
      echo "  ✗ $k"
      FAIL=$((FAIL+1)); FAILED="$FAILED $k"
    fi
  fi
done
echo "════════ 回归汇总：PASS=$PASS FLAKY=$FLAKY_N FAIL=$FAIL ════════"
[ -n "$FAILED" ] && { echo "失败: $FAILED"; exit 1; }
[ -n "$FLAKY" ] && { echo "flaky（重跑过，非确定性 PASS，建议人工复核）: $FLAKY"; exit 2; }
echo "✓ 全事件回归 PASS"
