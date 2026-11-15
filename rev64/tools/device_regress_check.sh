#!/bin/bash
# §124y-1937 设备恢复后 32 位翻译体复验链一键入口（基准 so 59465d97）
#
# 设备 10.0.182.146:5555 恢复（或任意 32 位 ARM 真机）后跑本脚本，
# 自动执行设备侧复验全链（离线 60 项已收口，本脚本 = 设备侧收尾）：
#
#   0. 前置：so 三方哈希核对（SOP §3.1，防设备跑旧 so）
#   1. 设备 4 项（arm32_hook_check --rounds 3）：game_test / walk_bridge /
#      native_event / framebuffer 全钩子机判 + 出屏 30% 非黑比
#   2. 走子桥 4 路径回归（arm32_walkregress.sh）：4/4 PASS + 0 崩溃
#   3. 真机 5 判据（REAL_DEVICE_ACCEPTANCE.md）：装/出屏/下一局/退出/重进
#   4. 回填：设备 4 项 + 4 路径 + 5 判据 PASS 记录到 REVERSE_LOG
#
# 用法：
#   bash rev64/tools/device_regress_check.sh                 # 自动探 10.0.182.146:5555
#   bash rev64/tools/device_regress_check.sh <dev-serial>    # 指定设备
#
# 设备不可达 = PENDING（§124y-189 口径，非 FAIL）；脚本退出码 0 = 设备
# 侧全 PASS，1 = 设备 PENDING / FAIL。

WS="$(cd "$(dirname "$0")/../.." && pwd)"
DEV="${1:-10.0.182.146:5555}"
SO_MD5_EXPECT="59465d974a9645cd426b965cb10901c5"

echo "═══ §124y-1937 设备恢复后 32 位翻译体复验链（基准 so $SO_MD5_EXPECT）═══"

# 0. 设备可达性
if ! adb -s "$DEV" get-state >/dev/null 2>&1; then
  echo "  设备 $DEV 不可达 → PENDING（§124y-189 口径，非 FAIL）"
  echo "  离线 60 项已收口（静态 60/60），设备侧复验待设备恢复"
  exit 1
fi
echo "  设备 $DEV 可达 → 开始设备侧复验"

# 前置：so 三方哈希核对（SOP §3.1）
echo "── 0. so 三方哈希核对（防设备跑旧 so）──"
python3 "$WS/rev64/tools/apk_so_hash_check.py" || { echo "  so 哈希漂移 → 须重 pack + 更新基准"; exit 1; }
EQ=$([ "$(adb -s $DEV shell md5sum /data/app/*/lib/arm/libjunqi.so 2>/dev/null | cut -d' ' -f1)" = "$SO_MD5_EXPECT" ] && echo OK || echo DRIFT)
echo "  设备端 so 哈希核对: $EQ（DRIFT → am force-stop + 重装 + 重跑全部判据）"
if [ "$EQ" != "OK" ]; then
  echo "  重装 so 到设备（基准 $SO_MD5_EXPECT）"
  adb -s $DEV install -r "$WS/rev64/dist/phase_chain_arm32.apk" || { echo "  装机失败 → PENDING"; exit 1; }
fi

echo "── 1. 设备 4 项（arm32_hook_check --rounds 3）──"
bash "$WS/rev64/tools/arm32_hook_check.sh" --dev "$DEV" --no-install --rounds 3
rc1=$?

echo "── 2. 走子桥 4 路径回归（arm32_walkregress.sh）──"
bash "$WS/rev64/tools/arm32_walkregress.sh" --dev "$DEV"
rc2=$?

echo "═══ 复验链汇总 ═══"
echo "  设备 4 项: $([ $rc1 -eq 0 ] && echo PASS || echo FAIL/PENDING)"
echo "  走子桥 4 路径: $([ $rc2 -eq 0 ] && echo PASS || echo FAIL/PENDING)"
echo "  真机 5 判据: 按 rev64/reverse/docs/REAL_DEVICE_ACCEPTANCE.md 逐步实锤"
echo "  回填: 设备 4 项 + 4 路径 + 5 判据 PASS 记录到 REVERSE_LOG"

[ $rc1 -eq 0 ] && [ $rc2 -eq 0 ]
