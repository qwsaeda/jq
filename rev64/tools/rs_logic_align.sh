#!/bin/bash
# rs_logic_align.sh — 正向 app「原版完整逻辑对齐」验收一条龙（脚本化/批量化）
#
# 目的：把"原版 vs 正向"的全部对齐判据收敛成一条命令，避免逐项手验（用户 2026-11-11
#       批评"光改光对比，不总结/不复用/不批量化/不脚本化"）。
#
# 用法:
#   bash rev64/tools/rs_logic_align.sh                # 全量（含 32/64 行为对比 + 真机）
#   bash rev64/tools/rs_logic_align.sh --desktop       # 只跑桌面（无设备）
#   bash rev64/tools/rs_logic_align.sh --dev IP:PORT   # 指定设备
#   bash rev64/tools/rs_logic_align.sh --skip-probe    # 跳过 32/64 设备行为对比
#
# 判据（缺一即 FAIL）:
#   [1] 编译 so（NDK arm64 + 14 JNI 导出）
#   [2] evverify --all（事件/业务 + 几何 + 弹窗按钮 Δ=0 命中）
#   [3] 一致性 selftest（规则引擎基准序列 9 行逐行一致）
#   [4] regress.py（走子/吃子/炸弹事件覆盖）
#   [5] 弹窗对齐状态（spak2ui status：从 ui_menu.h 实际枚举推导，非手工清单）
#   [6] 32/64 位规则行为实证对比（rule_probe_compare，分歧 ≤1 且 ≤ 已知 R-073）
#   [7] 真机 UI 全项对比（rs_ui_verify：房子+背景+按钮+标题 + 0 崩溃）
set -u
WS=/disk/workspaces/jq
RS=$WS/rev64/forward/src_rs
DEV=${DEV:-10.0.182.146:5555}
DESKTOP_ONLY=0
SKIP_PROBE=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --desktop) DESKTOP_ONLY=1; shift;;
    --skip-probe) SKIP_PROBE=1; shift;;
    --dev) DEV="$2"; shift 2;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done

PASS=0; FAIL=0
declare -a RESULTS
log()  { echo "[align] $*"; }
ok()   { PASS=$((PASS+1)); RESULTS+=("✓ $1 | $2"); echo "  ✓ $1 — $2"; }
bad()  { FAIL=$((FAIL+1)); RESULTS+=("✗ $1 | $2"); echo "  ✗ $1 — $2"; }

log "=== [1/7] 编译 so ==="
if (cd $RS && bash build_rs.sh so >/tmp/opencode/_align_so.log 2>&1); then
  NJNI=$(grep -oE "JNI 导出数: [0-9]+" /tmp/opencode/_align_so.log | grep -oE "[0-9]+" | head -1)
  if [ "${NJNI:-0}" -ge 14 ]; then ok "编译 so" "JNI 导出 ${NJNI} 个（≥14）"
  else bad "编译 so" "JNI 导出 ${NJNI:-0} < 14"; fi
else
  bad "编译 so" "编译失败（/tmp/opencode/_align_so.log）"; echo "--- 编译错误 ---"; grep -E "error:" /tmp/opencode/_align_so.log | head -5; exit 1
fi

log "=== [2/7] evverify --all（事件/业务 + 几何 + 弹窗）==="
if (cd $RS && make evverify >/tmp/opencode/_align_ev.log 2>&1 && ./dist/jq_evverify --all >>/tmp/opencode/_align_ev.log 2>&1); then
  LAST=$(grep -oE "桌面事件验证: [0-9]+/[0-9]+ 全对" /tmp/opencode/_align_ev.log | tail -1)
  ok "evverify" "${LAST:-PASS}"
else
  bad "evverify" "FAIL（/tmp/opencode/_align_ev.log）"; grep -iE "FAIL|不符" /tmp/opencode/_align_ev.log | head -5
fi

log "=== [3/7] 一致性 selftest（规则引擎基准 9 行）==="
if (cd $RS && make consistency >/dev/null 2>&1 && ./dist/jq_consistency selftest >/tmp/opencode/_align_cs.log 2>&1); then
  ok "一致性 selftest" "$(grep -oE 'PASS.*' /tmp/opencode/_align_cs.log | tail -1)"
else
  bad "一致性 selftest" "FAIL"
fi

log "=== [3b/7] 完整 60 局自动对弈（蓝 vs 红 AI，合法终局不崩）==="
if (cd $RS && make gametest >/dev/null 2>&1 && timeout 900 ./dist/jq_gametest >/tmp/opencode/_align_gt.log 2>&1); then
  n60=$(grep -c "ok: 对局合法终局" /tmp/opencode/_align_gt.log)
  if [ "$n60" -ge 60 ] && grep -q "全部通过" /tmp/opencode/_align_gt.log; then
    ok "60 局对弈" "60/60 合法终局 + 触摸/AI 回合交替全过"
  else
    bad "60 局对弈" "合法终局仅 $n60/60 或结论非全部通过"
  fi
else
  bad "60 局对弈" "FAIL（超时/崩溃）"
fi

log "=== [4/7] regress.py（走子/吃子/炸弹事件覆盖）==="
if (cd $RS && python3 regress.py >/tmp/opencode/_align_rg.log 2>&1); then
  ok "regress.py" "$(grep -oE '全部验收通过|覆盖: True' /tmp/opencode/_align_rg.log | tail -1)"
else
  bad "regress.py" "FAIL"
fi

log "=== [5/7] 弹窗对齐状态（代码真值推导）==="
ST=$(python3 $WS/rev64/tools/spak2ui.py status 2>/dev/null)
AL=$(echo "$ST" | grep -oE "已对齐 [0-9]+" | grep -oE "[0-9]+")
TOT=$(echo "$ST" | grep -oE "原版 [0-9]+" | grep -oE "[0-9]+")
BTN=$(echo "$ST" | grep -oE "可脚本化批量补的有按钮对话框 [0-9]+" | grep -oE "[0-9]+")
if [ "${BTN:-1}" -eq 0 ]; then ok "弹窗对齐" "已对齐 ${AL}/${TOT}，有按钮未对齐 0（全部补齐）"
else bad "弹窗对齐" "已对齐 ${AL}/${TOT}，仍有 ${BTN} 个有按钮弹窗未补"; fi

if [ $DESKTOP_ONLY -eq 1 ] || [ $SKIP_PROBE -eq 1 ]; then
  log "=== [6/7] 32/64 行为对比 — 跳过（--desktop/--skip-probe）==="
  RESULTS+=("- 32/64 行为对比 | 跳过")
else
  log "=== [6/7] 32/64 位规则行为实证对比（rule_probe_compare）==="
  if python3 $WS/rev64/tools/rule_probe_compare.py --dev $DEV >/tmp/opencode/_align_probe.log 2>&1; then
    D=$(grep -oE "分歧 [0-9]+ 条" /tmp/opencode/_align_probe.log | grep -oE "[0-9]+")
    ok "32/64 行为对比" "分歧 ${D:-?} 条（阈值 ≤1，R-073 地雷模型已知差异）"
  else
    bad "32/64 行为对比" "FAIL（/tmp/opencode/_align_probe.log）"
  fi
fi

log "=== [7/7] 真机 UI 全项对比 ==="
if [ $DESKTOP_ONLY -eq 1 ]; then
  RESULTS+=("- 真机 UI 全项 | 跳过（--desktop）")
else
  if bash $WS/rev64/tools/rs_ui_verify.sh --dev $DEV >/tmp/opencode/_align_ui.log 2>&1; then
    ok "真机 UI 全项" "$(grep -E '✓ 房子|✓ 背景|✓ 主界面按钮|✓ 军棋标题' /tmp/opencode/_align_ui.log | tr '\n' ' ')"
  else
    bad "真机 UI 全项" "FAIL（/tmp/opencode/_align_ui.log）"
    tail -8 /tmp/opencode/_align_ui.log
  fi
fi

echo
log "══════ 汇总 ══════"
for r in "${RESULTS[@]}"; do echo "  $r"; done
log "PASS=$PASS FAIL=$FAIL"
[ $FAIL -eq 0 ] && exit 0 || exit 1
