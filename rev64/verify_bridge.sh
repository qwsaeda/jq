#!/bin/bash
# verify_bridge.sh — 一键 so→Java 桥通路验证（固化重复的 MuMu 验证循环）
#
# 之前手工重复 4 次的流程，现在一条命令跑完：
#   build_arm32_aligned.sh → pack_arm32.sh → 卸载 → push → 安装 → 清 logcat → 启动 → 等 → 抓标签
# 用法：
#   ./verify_bridge.sh                 # 默认抓 jqjni+jqjni2 标签
#   ./verify_bridge.sh "OnUpdate|FATAL" # 自定义 grep 模式
#   SKIP_BUILD=1 ./verify_bridge.sh     # 跳过编译/打包（已编译，只重跑 MuMu）
set -e
cd /disk/workspaces/jq/rev64
DEV=localhost:5555
PKG=com.cnvcs.junqi
ACT="$PKG/com.cnvcs.App"
GREP_PAT=${1:-"jqjni"}
WAIT=${2:-14}

if [ "$SKIP_BUILD" != "1" ]; then
  echo "=== [build] arm32 翻译体 + 打包 ==="
  bash build_arm32_aligned.sh >/dev/null 2>&1 && echo "  so OK"
  bash pack_arm32.sh >/dev/null 2>&1 && echo "  apk OK"
fi

APK=/disk/workspaces/jq/rev64/dist/phase_chain_arm32.apk
echo "=== [mumu] 安装 $APK ==="
adb -s $DEV shell "am force-stop $PKG" 2>/dev/null || true
adb -s $DEV shell "su 0 pm uninstall $PKG" >/dev/null 2>&1 || true
adb -s $DEV push "$APK" /sdcard/phase_chain_arm32.apk >/dev/null 2>&1
INST=$(adb -s $DEV shell "su 0 pm install /sdcard/phase_chain_arm32.apk" 2>&1 | tail -1)
echo "  install: $INST"

echo "=== [logcat] 清 + 启动 + 等 ${WAIT}s ==="
adb -s $DEV shell logcat -c
adb -s $DEV shell "am start -n $ACT" >/dev/null 2>&1
sleep "$WAIT"

echo "=== [result] grep '$GREP_PAT' ==="
adb -s $DEV shell logcat -d 2>/dev/null | grep -E "$GREP_PAT" | head -40
echo "=== [fatal] SIGSEGV/SIGABRT ==="
adb -s $DEV shell logcat -d 2>/dev/null | grep -E "FATAL|SIGSEGV|SIGABRT|invalid jobject" | head -8
echo "=== [proc] 进程状态 ==="
adb -s $DEV shell "ps -A | grep cnvcs || echo '  进程已退出'"
