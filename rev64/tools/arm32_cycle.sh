#!/bin/bash
# arm32_cycle.sh — arm32 翻译体"编译→打包→装机→验证"一条龙（§〇d 工具固化）
#
# 目的：每轮"改 jqt_vtbl.c / 翻译体 → 重编 → 重打包 → 装 MuMu → 启动 → 看日志"，
# 不再手写多段 adb/apktool/zipalign 命令。这是 arm32 逆向最重复的手工循环。
#
# 用法：
#   bash rev64/tools/arm32_cycle.sh                # 编译+打包+验证（设备可达）
#   bash rev64/tools/arm32_cycle.sh --no-build     # 只打包+验证（用现有 so）
#   bash rev64/tools/arm32_cycle.sh --no-verify    # 编译+打包，不上设备
#   bash rev64/tools/arm32_cycle.sh --wait 20      # 验证等待 20s（默认 12）
#
# 步骤（复用既有脚本，不重复实现）：
#   build  = build_arm32_aligned.sh（jqt_*.c → libjunqi_arm32_aligned.so）
#   pack   = pack_arm32.sh（→ phase_chain_arm32.apk，STORE+zipalign+v1+v2）
#   verify = mumu_verify.sh（连接→装→启→logcat→截图→判定）
#
# 诚实边界：verify 的"PASS(运行)"≠"能玩"/"棋盘画出"（mumu_verify.sh 注释）。
set -u
WS=/disk/workspaces/jq
BUILD=1 PACK=1 VERIFY=1 WAIT=12
while [ $# -gt 0 ]; do
  case "$1" in
    --no-build) BUILD=0 ;;
    --no-pack) PACK=0 ;;
    --no-verify) VERIFY=0 ;;
    --wait) shift; WAIT=${1:-12} ;;
    --wait=*) WAIT=${1#--wait=} ;;
    *) echo "未知参数: $1"; exit 2 ;;
  esac
  shift
done
SO=$WS/rev64/dist/libjunqi_arm32_aligned.so
APK=$WS/rev64/dist/phase_chain_arm32.apk
DEV=10.0.182.146:5555

if [ $BUILD -eq 1 ]; then
  echo "══════════ [1/3] 编译翻译体 so ══════════"
  bash $WS/rev64/build_arm32_aligned.sh "$SO" || { echo "✗ 编译失败"; exit 1; }
fi
if [ $PACK -eq 1 ]; then
  echo "══════════ [2/3] 重打包 APK ══════════"
  bash $WS/rev64/pack_arm32.sh "$SO" "$APK" || { echo "✗ 打包失败"; exit 1; }
fi
if [ $VERIFY -eq 1 ]; then
  echo "══════════ [3/3] MuMu 装机验证（等待 ${WAIT}s）══════════"
  # 连接检查
  adb connect $DEV >/dev/null 2>&1
  if ! adb -s $DEV get-state >/dev/null 2>&1; then
    # ★ AUD-1000-ALL A5-F5-005（P1 已修）：设备不可达改 exit 1（旧版 exit 0 静默降级，
    #   断连被映射成 PASS，CI 假绿）。产物已就绪但验证未执行——门禁须区分
    #   "未验（exit 1）"与"验过 PASS（exit 0）"，调用方按 rc 判。
    echo "★ 设备不可达（$DEV）——验证未执行（exit 1，非 PASS；产物已就绪，设备恢复后重跑）"
    exit 1
  fi
  # 用 mumu_verify.sh 但覆盖等待时间
  # ★ AUD-1000-ALL A5-F5-006（P2 已修）：`bash mumu_verify.sh 2>&1 | sed` 管道——上游
  #   mumu_verify.sh 失败（装机/判据 FAIL exit 1）被 sed 吞码（管道 rc=sed 的 0），
  #   外层 `bash arm32_cycle.sh` 误判 PASS。修：捕获 PIPESTATUS[0]（bash），显式传播 rc。
  bash $WS/rev64/tools/mumu_verify.sh "$APK" "$DEV" 2>&1 | sed 's/sleep 12/sleep '"$WAIT"'/'
  VERIFY_RC=${PIPESTATUS[0]}
  if [ $VERIFY_RC -ne 0 ]; then
    echo "★ mumu_verify.sh 失败（rc=$VERIFY_RC）——验证 FAIL（非设备不可达，判据/装机段失败）"
    exit 1
  fi
else
  echo "══════════ 产物就绪（未验证）══════════"
fi
echo "✓ 完成：$SO"
echo "✓ 完成：$APK"
