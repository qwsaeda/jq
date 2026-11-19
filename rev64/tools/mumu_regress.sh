#!/bin/bash
# mumu_regress.sh — 事件回归一键跑（批量，含 btconn）
# 收敛反复手工 for 循环（flag/mine/restart/draw/resign/stalemate/close/pop/btconn）。
# 用法:
#   bash rev64/tools/mumu_regress.sh --dev 10.0.182.146:5555 --only flag,btconn
#   （默认 DEV=10.0.182.146:5555 权威红果，★ R9-G-34 全工具链同源；旧 emulator-5554 口径废弃）
set -u
# ★ R9-G-34（P1 默认设备统一，权威红果，与 rule_probe_compare/mumu_align_smoke/
#   mumu_evtest 同源；旧 emulator-5554 = 未使用模拟器端口，一键回归误打假红——
#   显式 --dev 可覆盖）。
DEV=10.0.182.146:5555
ONLY=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --dev) DEV="$2"; shift 2;;
    --only) ONLY="$2"; shift 2;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done
KINDS="flag mine restart draw resign stalemate close pop btconn"
# ★ R48-Q78（P0 btconn 钩子防误判标注）：btconn = 模拟 App.btState("host_connected") 回调链路
#   （jni_bridge :1619-1645 setprop 钩子 + mumu_regress 回归），非真实 BtSpp RFCOMM accept/connect
#   数据面（BtSpp.java:44-197 真实 socket 需 BT 硬件/权限/配对，无硬件端到端 PENDING）。
#   btconn PASS = "协议态/UI 回调模拟 PASS"，**非** "蓝牙硬件端到端可联机"（SPEC §8 L202 红线）。
[ -n "$ONLY" ] && KINDS=$(echo "$ONLY" | tr ',' ' ')   # ★ 逗号分隔支持（--only flag,btconn）
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
    rc=$?
    # ★ R19-P1: rc=3 = 设备不可达 (PENDING, §124y-189 降级口径, 非判据 FAIL) →
    # 后续项必然同断, 提前汇总退 3 (调用方按 PENDING 处理, 不计 FAIL)。
    if [ $rc -eq 3 ]; then
      echo "  ⚠ $k: 设备 $DEV 不可达 (rc=3 PENDING)——后续项跳过, 恢复后重跑"
      echo "════════ 回归汇总：PASS=$PASS FLAKY=$FLAKY_N FAIL=$FAIL PENDING=设备离线 ════════"
      exit 3
    fi
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
