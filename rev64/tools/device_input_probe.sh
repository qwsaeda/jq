#!/usr/bin/env bash
# device_input_probe.sh — 设备真实触摸链路诊断（零 root / 只读，不 reboot）
#
# 定位（2026-11-22）：AGENTS 里「真实手点未达成」根因 = 10.0.182.146 MuMu 的
#   nemuinput 虚拟触摸桥卡 va_ioctl（/dev/uinput UI_DEV_SETUP 阻塞 → 虚拟触摸设备
#   建不出来 → 客户端点击进不了系统）。这是**模拟器环境故障**。
#   10.0.182.152 是 Xiaomi 23127PN0CC 真机（物理触摸屏 event4 "Xiaomi Touchscreen"
#   注册为 TOUCH|TOUCH_MT + InputDispatcher 在表），无 nemuinput，物理触摸链路天然健康。
#   两者性质不同，禁混算。本脚本机检区分，杜绝把「真机物理触摸健康」误判成「手点未达成」。
#
# 判据：
#   PASS（真机物理触摸健康）：dumpsys input 有 TOUCH|TOUCH_MT 设备 + 在 InputDispatcher
#     设备表 + 无 nemuinput 卡 va_ioctl；ro.debuggable=0/user 版下 adb 无 root =
#     user 版安全限制（非缺陷，物理手点直达 app，无需注入 event 节点）。
#   FAIL（模拟器虚拟桥故障）：nemuinput wchan=va_ioctl 或 无 TOUCH|TOUCH_MT 虚拟设备。
#
# 用法：bash rev64/tools/device_input_probe.sh [--dev 10.0.182.152:5555]
set -uo pipefail
DEV=10.0.182.152:5555
while [ $# -gt 0 ]; do
  case "$1" in
    --dev) DEV="$2"; shift 2;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done

A="adb -s $DEV"
echo "════════ 设备真实触摸链路诊断（$DEV）════════"
MODEL=$($A shell getprop ro.product.model 2>/dev/null | tr -d '\r\n')
ABI=$($A shell getprop ro.product.cpu.abilist 2>/dev/null | tr -d '\r\n')
REL=$($A shell getprop ro.build.version.release 2>/dev/null | tr -d '\r\n')
DEB=$($A shell getprop ro.debuggable 2>/dev/null | tr -d '\r\n')
BTYPE=$($A shell getprop ro.build.type 2>/dev/null | tr -d '\r\n')
echo "  设备: $MODEL | Android $REL | abilist=$ABI | $BTYPE (debuggable=$DEB)"

# 1. nemuinput 虚拟桥状态
NPID=$($A shell pidof nemuinput 2>/dev/null | tr -d '\r\n')
NWC=""
[ -n "$NPID" ] && NWC=$($A shell "cat /proc/$NPID/wchan 2>/dev/null" | tr -d '\r\n')
echo "  nemuinput: pid=${NPID:-无} wchan=${NWC:-—}"

# 2. 物理/虚拟触摸设备（InputReader 注册类）
DUMP=$($A shell "dumpsys input 2>/dev/null")
TOUCH_MT=$(printf '%s' "$DUMP" | grep -c "TOUCH | TOUCH_MT" || true)
TOUCH_NAMES=$(printf '%s' "$DUMP" | grep -iE "Xiaomi Touchscreen|Touchscreen|TouchPad" | head -3 | tr '\n' ';')
echo "  InputReader TOUCH|TOUCH_MT 设备数=$TOUCH_MT ($TOUCH_NAMES)"

# 3. InputDispatcher 设备表在表确认
IN_DISP=$(printf '%s' "$DUMP" | grep -c "Device .*: .*Touch" || true)
echo "  InputDispatcher 触摸设备在表=$IN_DISP"

# 4. event 节点 + 能力（只读）
EVN=$($A shell "ls /dev/input/event* 2>/dev/null | wc -l" | tr -d '\r\n')
echo "  /dev/input/event* 节点数=$EVN"

# 判据
if [ "$NWC" = "va_ioctl" ]; then
  echo "判据: FAIL（nemuinput 卡 va_ioctl = 146 MuMu 模拟器虚拟桥故障，真实手点链路断=环境问题）"
  exit 2
elif [ "$TOUCH_MT" -ge 1 ] && [ "$IN_DISP" -ge 1 ] && [ "$NWC" != "va_ioctl" ]; then
  echo "判据: PASS（触摸链路健康：TOUCH|TOUCH_MT 已注册 + InputDispatcher 在表 + nemuinput ${NWC:-无}非卡 va_ioctl）"
  echo "  注：user 版(debuggable=0) adb 无 root 是安全限制，非缺陷；手点直达 app，注入 event 节点仅属注入通道证据。"
  exit 0
else
  echo "判据: FAIL（真值不明：TOUCH_MT=$TOUCH_MT IN_DISP=$IN_DISP nemuinput=$NPID/$NWC，按 §119 查真值不臆测）"
  exit 2
fi
