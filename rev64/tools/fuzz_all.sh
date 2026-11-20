#!/usr/bin/env bash
# fuzz_all.sh — 渗透轮总入口（2026-11-20，所有渗透判据一键跑，无蓝牙）
#
# 定位：解决"判据散落在 fuzz_probe_check.py / jqt_mem_fuzz_check.sh / vtable_dispatch_fuzz.py
#   / src_rs_regress.sh 多处，漏挂 = 静默降级假绿（CODE_STYLE §9）"的问题。
#   本脚本 = 渗透判据的**单一入口**，CI/人工只需跑这一个。
#
# 三段：
#   A 正向 C 渗透面（fuzz_probe_check.py → F1-F5 + P1/P2/P3/P5'/P6/P7/P8 的 binary 判据）
#   B 32 位翻译体渗透面（jqt_mem_fuzz_check.sh → M1-M5 + 32 位真地址空间解引用版）
#   C 32 位 dispatch 表（vtable_dispatch_fuzz.py → 5463 槽）
#   D Frida 真机设备侧（可选 --device；环境依赖 frida-server，默认跳过）
#
# 判据：A/B/C 全 PASS = 渗透轮达成（D 为设备侧加分项，PENDING 不阻断）
# 用法：bash rev64/tools/fuzz_all.sh [--quick] [--device]
#   --quick  : 正向用短轮次（CI 快速门禁）
#   --device : 跑 Frida 设备侧（需 frida-server + app 运行）
set -uo pipefail
WS=/disk/workspaces/jq
QUICK=0
DEVICE=0
while [ $# -gt 0 ]; do
  case "$1" in
    --quick) QUICK=1; shift;;
    --device) DEVICE=1; shift;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done

echo "════════ 渗透轮总入口（fuzz_all.sh，无蓝牙）════════"
FAILS=0

# ── A 正向 C 渗透面 ──
echo "── A 正向 C（F1-F5 + P1/P2/P3/P5'/P6/P7/P8）──"
if [ $QUICK -eq 1 ]; then
  A_OUT=$(python3 "$WS/rev64/tools/fuzz_probe_check.py" --offline \
          --games 100 --touch 1000 --events 200 --saves 50 \
          --png-rounds 500 --ui-steps 300 2>&1)
else
  A_OUT=$(python3 "$WS/rev64/tools/fuzz_probe_check.py" --offline 2>&1)
fi
A_RC=$?
echo "$A_OUT" | grep -E "判据:|^④" | tail -12
[ $A_RC -ne 0 ] && { echo "  ✗ A 段 FAIL（rc=$A_RC）"; FAILS=$((FAILS+1)); } || echo "  ✓ A 段 PASS"

# ── B 32 位翻译体渗透面 ──
echo "── B 32 位翻译体（M1-M5 + 真地址空间解引用版）──"
B_OUT=$(bash "$WS/rev64/tools/jqt_mem_fuzz_check.sh" 2>&1); B_RC=$?
echo "$B_OUT" | grep -E "判据:|^④|真地址空间" | tail -6
[ $B_RC -ne 0 ] && { echo "  ✗ B 段 FAIL（rc=$B_RC）"; FAILS=$((FAILS+1)); } || echo "  ✓ B 段 PASS"

# ── C 32 位 dispatch 表 ──
echo "── C 32 位 dispatch 表（5463 槽）──"
C_OUT=$(python3 "$WS/rev64/tools/vtable_dispatch_fuzz.py" 2>&1); C_RC=$?
echo "$C_OUT" | grep -E "判据:|①|②|诚实边界" | tail -5
[ $C_RC -ne 0 ] && { echo "  ✗ C 段 FAIL（rc=$C_RC）"; FAILS=$((FAILS+1)); } || echo "  ✓ C 段 PASS"

# ── D Frida 设备侧（可选）──
if [ $DEVICE -eq 1 ]; then
  echo "── D Frida 设备侧（JNI 桥梁 hook，需 frida-server）──"
  DEV_ADDR="10.0.182.152:5555"
  python3 -c "import frida; frida.get_device('$DEV_ADDR', timeout=5)" >/dev/null 2>&1
  if [ $? -eq 0 ]; then
    D_OUT=$(python3 "$WS/rev64/tools/frida_jni_bridge_hook.py" --dev "$DEV_ADDR" --wait 8 2>&1)
    echo "$D_OUT" | grep -E "判据:|hook 安装|FindClass" | tail -4
    echo "$D_OUT" | grep -q "判据: PASS" && echo "  ✓ D 段 PASS" || { echo "  ✗ D 段 FAIL（设备侧非 so 回归）"; }
  else
    echo "  ⚠ D 段 PENDING（frida-server 不可达，环境依赖）"
  fi
fi

echo "════════ 汇总 ════════"
if [ $FAILS -eq 0 ]; then
  echo "渗透轮判据: PASS（A 正向 + B 32位 + C dispatch 全通）"
  exit 0
else
  echo "渗透轮判据: FAIL（$FAILS 段失败）"
  exit 1
fi
