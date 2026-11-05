#!/bin/bash
# probe_cycle.sh — "红果跑测试 + 对象图探查"一条龙（§124y-14 固化）
#
# 背景：本轮反复手敲"redroid_test.sh 抓 gs8 日志 → so_probe.py dispatch/slot 对照"，
# 两三步串起来。本脚本固化成一条命令：
#   1. redroid_test.sh --no-install（跑测试钩子 + GS8 诊断日志）
#   2. 抓日志里的 gs8_vt_off（gs[8] 真实 vtable 偏移）
#   3. so_probe.py dispatch 该 vtable + slot [0x18]（游戏更新槽）
#   4. so_probe.py slot 该 vtable 各真实方法槽（[0x10]/[0x28]/[0x48]）
#   5. 汇总判定：miss / 崩溃 / gs8 vtable 是否已收进 dispatch 表
#
# 用法：
#   bash rev64/tools/probe_cycle.sh                 # 全流程
#   bash rev64/tools/probe_cycle.sh --no-install    # 跳过装 APK
#   bash rev64/tools/probe_cycle.sh --slots 0x10,0x28,0x48   # 指定探查槽（默认 0x10,0x28,0x48）
#
# 诚实边界：只探查**静态 .so 槽值** + 运行时 gs8 vtable，不重建对象图；
# 游戏逻辑真正推进需 gs[8] 子对象完整字段 + 资源加载（需 MuMu）。
set -u
WS=/disk/workspaces/jq
SO_PROBE="python3 $WS/rev64/tools/so_probe.py"
SLOTS="0x10,0x28,0x48"
NO_INSTALL=0
while [ $# -gt 0 ]; do
  case "$1" in
    --no-install) NO_INSTALL=1 ;;
    --slots) shift; SLOTS=${1:-$SLOTS} ;;
    --slots=*) SLOTS=${1#--slots=} ;;
    *) echo "未知参数: $1"; exit 2 ;;
  esac
  shift
done

FLAGS=$([ $NO_INSTALL -eq 1 ] && echo "--no-install" || echo "")

echo "═══ [1/4] redroid 测试钩子（GS8 诊断）═══"
LOG=$(bash $WS/rev64/tools/redroid_test.sh $FLAGS --wait 4 2>&1)
echo "$LOG" | grep -E "GS8|PASS|FAIL" | head -4
GS8_LINE=$(echo "$LOG" | grep "TEST-GAME GS8" | head -1)

# 解析 gs8_vt_off（gs[8] 真实 vtable 偏移，如 0x159c80）
VT_OFF=$(echo "$GS8_LINE" | grep -oE "gs8_vt_off=0x[0-9a-f]+" | grep -oE "0x[0-9a-f]+" | head -1)
if [ -z "$VT_OFF" ]; then
  echo "⚠ 未抓到 gs8_vt_off（测试钩子未触发或日志轮转）——跳探查"
  exit 1
fi
echo "═══ gs[8] 真实 vtable 偏移: $VT_OFF ═══"

echo ""
echo "═══ [2/4] dispatch 查 vtable + 各槽是否收进表 ═══"
$SO_PROBE dispatch $VT_OFF 2>&1 | head -8

echo ""
echo "═══ [3/4] 探查各游戏方法槽 ═══"
IFS=',' read -ra SLOT_ARR <<< "$SLOTS"
for off in "${SLOT_ARR[@]}"; do
  echo "--- 槽 [$off] ---"
  $SO_PROBE slot $VT_OFF $off 2>&1 | head -10
done

echo ""
echo "═══ [4/4] 汇总判定 ═══"
CRASH=$(echo "$LOG" | grep -cE "Fatal signal|SIGSEGV|SIGABRT")
MISS=$(echo "$LOG" | grep "BLX miss" | grep -oE "off=0x[0-9a-f]+" | sort | uniq -c | sort -rn)
PID=$(echo "$LOG" | grep -oE "进程: [0-9a-z]+" | head -1 | awk '{print $2}')
echo "崩溃: $CRASH | 进程: ${PID:-?}"
echo "miss: ${MISS:-（无）}"
# 判定：vtable 各槽是否都在 dispatch 表
IN_TBL=0; NOT_IN=()
for off in "${SLOT_ARR[@]}"; do
  # so_probe slot 输出的目标函数地址
  FNADDR=$($SO_PROBE slot $VT_OFF $off 2>&1 | grep -oE "F_[0-9a-f]+" | head -1 | sed 's/F_//')
  if [ -n "$FNADDR" ]; then
    if $SO_PROBE dispatch 0x$FNADDR 2>&1 | grep -q "在 jqt_vtbl_offs\[\] 表: True\|特判里"; then
      IN_TBL=$((IN_TBL+1))
    else
      NOT_IN+=("0x$FNADDR ($off)")
    fi
  fi
done
echo "槽在表: $IN_TBL / ${#SLOT_ARR[@]} | 不在表: ${NOT_IN[*]:-无}"
echo "═══ 探查结束（诚实边界：静态槽对照，非对象图重建）═══"
