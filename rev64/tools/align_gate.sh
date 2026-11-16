#!/bin/bash
# align_gate.sh — 正向 64 位「对齐」一键门禁（Round 1 D-18 落地）
#
# 目的：把分散的 5+ 个门禁串成**一条命令**，防"漏跑某个门禁"（CODE_STYLE §9
#       假绿三型之"静默降级"——门禁存在但没人跑 = 等于没有）。
# 门禁清单（每项独立判据，汇总 PASS/FAIL，任一 FAIL → 退出码 1）：
#   ① make evverify                  —— 以 SUMMARY 动态真值为准（当前 172/172 = 事件/业务 40/40 + 几何 132/132；★ R7-D-08 去硬编码防再次漂移）
#   ② event_code_map_check.py        —— 原版 .bss 事件表 asr4 映射 0 缺口
#   ③ layout_align_check.py --baseline —— 布局几何 Δ=0
#   ④ dark_align_regress.sh          —— 暗棋对齐桌面回归（0 设备依赖）+ **判据自检**
#   ⑤ make benchcheck                —— 走子桥 8 步文件基准新鲜（§103 基准过期防线）
#   ⑥ rule_probe_compare.py --offline —— 64 位探针基线可跑（32 位设备项需 --dev）
# 用法：
#   bash rev64/tools/align_gate.sh                # 全跑（桌面层，0 设备依赖）
#   bash rev64/tools/align_gate.sh --dev 10.0.182.146:5555   # 附加 32↔64 双端对照
# 诚实边界：⑥ 的双端对照需 32 位设备；未给 --dev 时只跑 64 位基线（不冒充双端结论）。
set -uo pipefail
WS=/disk/workspaces/jq
RS=$WS/rev64/forward/src_rs
DEV=""
while [ $# -gt 0 ]; do
  case "$1" in
    --dev) DEV="$2"; shift 2;;
    *) shift;;
  esac
done

pass=0; fail=0
run_gate() {   # $1=门禁名 $2..=命令
  local name="$1"; shift
  echo ""
  echo "──────── [$name] ────────"
  local out rc
  out=$(cd "$RS" && timeout 900 "$@" 2>&1); rc=$?
  echo "$out" | tail -6
  if [ $rc -eq 0 ]; then
    echo "  ✓ [$name] PASS"; pass=$((pass+1))
  else
    echo "  ✗ [$name] FAIL（rc=$rc）"; fail=$((fail+1))
  fi
}

echo "=== 正向对齐一键门禁（align_gate）==="
run_gate "① evverify（SUMMARY 动态真值）" make evverify
run_gate "② 事件表映射 0 缺口" python3 "$WS/rev64/tools/event_code_map_check.py"
run_gate "③ 布局几何 Δ=0" python3 "$WS/rev64/tools/layout_align_check.py" --baseline
run_gate "④ 暗棋对齐回归 + 判据自检" bash "$RS/dark_align/dark_align_regress.sh"
run_gate "⑤ 走子桥 8 步基准新鲜" make benchcheck
run_gate "⑦ 事件名逐名对齐（原版 .rodata 73 名）" python3 "$WS/rev64/tools/event_names_alignment_check.py"
if [ -n "$DEV" ]; then
  run_gate "⑥ 32↔64 双端探针对照" python3 "$WS/rev64/tools/rule_probe_compare.py" --dev "$DEV"
else
  run_gate "⑥ 64 位探针基线（--offline）" python3 "$WS/rev64/tools/rule_probe_compare.py" --offline
fi

echo ""
echo "=== 合计 PASS=$pass FAIL=$fail ==="
if [ "$fail" -gt 0 ]; then
  echo "判定: FAIL（定位根因后重跑）"; exit 1
fi
echo "判定: PASS（对齐门禁全过；设备冒烟另走 mumu_align_smoke.sh）"
