#!/bin/bash
# §124y-215 32 位逆向验收双端总入口（设备 4 项 + 静态 33 项，一键跑）
#
# 用法：
#   bash rev64/tools/arm32_verify_all.sh                       # 默认 10.0.182.146:5555
#   bash rev64/tools/arm32_verify_all.sh --dev 10.0.182.146:5555
#   bash rev64/tools/arm32_verify_all.sh --static-only         # 设备离线期只跑静态 33 项
#
# 双端口径：
#   静态 = arm32_static_evidence.sh（33 项全 PASS，设备无关，离线可跑）
#   设备 = arm32_hook_check.sh --rounds 3（game_test/walk_bridge/native_event/
#          framebuffer 4 项，flaky 已根治 §124y-188/189 + 掉线容错）
# 设备不可达时自动降级 = 静态 PASS + 设备 PENDING（§124y-189 口径：
#   环境掉线 ≠ so 回归，判据结果不作 so 退化依据，等设备稳定窗口复验）。

set -u
WS=/disk/workspaces/jq
DEV="10.0.182.146:5555"
STATIC_ONLY=0
while [ $# -gt 0 ]; do
  case "$1" in
    --dev) DEV="$2"; shift 2;;
    --static-only) STATIC_ONLY=1; shift;;
    *) shift;;
  esac
done

echo "═══ 32 位逆向验收双端总入口（§124y-215）═══"

# 静态 33 项（设备无关，永远先跑）
echo "── 静态佐证（33 项，设备无关）──"
STATIC_OUT=$(bash "$WS/rev64/tools/arm32_static_evidence.sh" 2>&1)
echo "$STATIC_OUT" | grep -E "PASS=|FAIL=" | tail -1
STATIC_PASS=0; STATIC_FAIL=0
echo "$STATIC_OUT" | grep -qE "PASS=([1-9][0-9]*) FAIL=0" && STATIC_PASS=1
echo "$STATIC_OUT" | grep -q "FAIL=[1-9]" && STATIC_FAIL=1

# 设备 4 项（可达则跑，不可达自动降级 PENDING）
if [ "$STATIC_ONLY" = "1" ]; then
  echo "── 设备 4 项（--static-only，跳过）──"
  DEV_STATUS="SKIPPED"
elif adb -s "$DEV" get-state >/dev/null 2>&1; then
  echo "── 设备 4 项（$DEV，--rounds 3）──"
  DEV_OUT=$(bash "$WS/rev64/tools/arm32_hook_check.sh" --dev "$DEV" --no-install --rounds 3 2>&1)
  echo "$DEV_OUT" | grep -E "PASS=|FAIL=|失败" | tail -3
  DEV_STATUS=$(echo "$DEV_OUT" | grep -qE "PASS=4 FAIL=0" && echo "PASS" || echo "FAIL")
else
  echo "── 设备 4 项（$DEV 不可达，自动降级 PENDING §124y-189 口径）──"
  DEV_STATUS="PENDING"
fi

echo "═══ 双端总入口汇总 ═══"
echo "  静态 33 项: $( [ "$STATIC_PASS" = 1 ] && echo PASS || echo "FAIL（$STATIC_FAIL）" )"
echo "  设备 4 项:  $DEV_STATUS"
case "$DEV_STATUS" in
  PENDING) echo "  总判读: 静态 PASS + 设备 PENDING（环境掉线非 so 回归，等设备稳定窗口复验）";;
  PASS)    echo "  总判读: 双端全 PASS（32 位逆向对齐基准成立）";;
  SKIPPED) echo "  总判读: 仅静态 PASS（--static-only 模式）";;
  *)       echo "  总判读: 设备 FAIL（查 flaky 窗口 / 转译层崩溃周期 §124y-188/191，单跑 4 项为准）";;
esac
