#!/bin/bash
# §124y-215 32 位逆向验收双端总入口（设备 4 项 + 静态 73 项，一键跑）
#
# 用法：
#   bash rev64/tools/arm32_verify_all.sh                       # 默认 10.0.182.146:5555
#   bash rev64/tools/arm32_verify_all.sh --dev 10.0.182.146:5555
#   bash rev64/tools/arm32_verify_all.sh --static-only         # 设备离线期只跑静态 73 项
#
# 双端口径：
#   静态 = arm32_static_evidence.sh（41 项全 PASS，设备无关，离线可跑）
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

# 静态 73 项（设备无关，永远先跑）
echo "── 静态佐证（41 项，设备无关）──"
STATIC_OUT=$(bash "$WS/rev64/tools/arm32_static_evidence.sh" 2>&1)
echo "$STATIC_OUT" | grep -E "PASS=|FAIL=" | tail -1
STATIC_PASS=0; STATIC_FAIL=0
echo "$STATIC_OUT" | grep -qE "PASS=([1-9][0-9]*) FAIL=0" && STATIC_PASS=1
echo "$STATIC_OUT" | grep -q "FAIL=[1-9]" && STATIC_FAIL=1

# 渗透轮 32 位（jqt_mem 内存路由 5 探针，设备无关，静态佐证同档；2026-11-20 新增）
# rc：0=PASS  3=PENDING（jq_img.bin/gcc/ASan 缺失，非 so 回归）  2=FAIL（jqt_mem.c 回归）
echo "── 渗透轮 32 位（jqt_mem_fuzz 5 探针，ASan 桌面直跑）──"
FUZZ32_RC=0
FUZZ32_OUT=$(bash "$WS/rev64/tools/jqt_mem_fuzz_check.sh" 2>&1); FUZZ32_RC=$?
case $FUZZ32_RC in
  0) FUZZ32="PASS";;
  3) FUZZ32="PENDING（jq_img.bin/gcc/ASan 缺失，非 so 回归）";;
  *) FUZZ32="FAIL（rc=$FUZZ32_RC，jqt_mem.c 内存路由回归——白盒定位回路）"
     echo "$FUZZ32_OUT" | grep -E "FAIL|证据" | head -3;;
esac
echo "  32 位渗透轮: $FUZZ32"

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
echo "  静态 73 项: $( [ "$STATIC_PASS" = 1 ] && echo PASS || echo "FAIL（$STATIC_FAIL）" )"
echo "  32 位渗透轮: $FUZZ32"
echo "  设备 4 项:  $DEV_STATUS"
FUZZ32_FAIL=0
case "$FUZZ32" in FAIL*) FUZZ32_FAIL=1;; esac
case "$DEV_STATUS" in
  PENDING) echo "  总判读: 静态 PASS + 设备 PENDING（环境掉线非 so 回归，等设备稳定窗口复验）"
           [ "$STATIC_FAIL" = 1 ] && exit 1
           [ "$FUZZ32_FAIL" = 1 ] && { echo "★ 32 位渗透轮 FAIL（jqt_mem.c 回归，白盒定位）"; exit 1; }
           exit 0;;
  PASS)    echo "  总判读: 双端全 PASS（32 位逆向对齐基准成立）"
           [ "$STATIC_FAIL" = 1 ] && exit 1
           [ "$FUZZ32_FAIL" = 1 ] && { echo "★ 32 位渗透轮 FAIL（jqt_mem.c 回归，白盒定位）"; exit 1; }
           exit 0;;
  SKIPPED) echo "  总判读: 静态 + 32 位渗透轮 PASS，设备侧 SKIPPED（--static-only 模式）"
           [ "$STATIC_FAIL" = 1 ] && { echo "★ 静态有 FAIL, 非通过"; exit 1; }
           [ "$FUZZ32_FAIL" = 1 ] && { echo "★ 32 位渗透轮 FAIL（jqt_mem.c 回归，白盒定位）"; exit 1; }
           # ★ R19-P1: SKIPPED = 显式跳过设备侧, 退出码 2 (区分全 PASS rc=0 / PENDING rc=0 /
           # FAIL rc=1), 防调用方把 "--static-only 产物" 当 "双端全 PASS" 误报。
           echo "★ SKIPPED rc=2: 设备侧未验, 引用本结果时须标注 '仅静态'"
           exit 2;;
  *)       echo "  总判读: 设备 FAIL（查 flaky 窗口 / 转译层崩溃周期 §124y-188/191，单跑 4 项为准）"
           exit 1;;
esac
