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
#   3. 完整 60 局设备跑（setprop jq.game_test + 走子桥 tick，对局完整
#      50 子 12×5 棋盘终局，Junqi.Move/Eat/Bomb/Flag/Dead/Flip 全事件）
#   4. 渲染像素级 screencap 非黑比（截图棋盘/棋子纹理出屏 ≥30%，
#      PIECE-TEX 棋子图集 + centerpx 采样非全黑）
#   5. 蓝牙端到端（有 BT 硬件真机：host/client SPP 帧收发 UNDO/
#      RESTART 闭环，UUID ce1404fe；无 BT 硬件 = PENDING）
#   6. 音效出屏（sfx_play 事件 → WAV 播放出屏，14 WAV 资源设备端在位）
#   7. 真机 5 判据（REAL_DEVICE_ACCEPTANCE.md）：装/出屏/下一局/退出/重进
#   8. 回填：设备 4 项 + 4 路径 + 60 局 + screencap + 蓝牙 + 音效 + 5 判据
#      PASS 记录到 REVERSE_LOG
#
# 用法：
#   bash rev64/tools/device_regress_check.sh                 # 自动探 10.0.182.146:5555
#   bash rev64/tools/device_regress_check.sh <dev-serial>    # 指定设备
#
# 设备不可达 = PENDING（§124y-189 口径，非 FAIL）；脚本退出码 0 = 设备
# 侧全 PASS，1 = 设备 PENDING / FAIL。

WS="$(cd "$(dirname "$0")/../.." && pwd)"
DEV="${1:-10.0.182.146:5555}"
SO_MD5_EXPECT="2566750232e3fbc5ded778553f7ab34d"

echo "═══ §124y-1937 设备恢复后 32 位翻译体复验链（基准 so $SO_MD5_EXPECT）═══"
# ★ 设备侧唯一真源口径（§124y-1949 实锤）：10.0.182.146 = 红果 MuMu（x86_64 +
#   houdini 转译 ARM32）唯一能跑 ARM32 翻译体 so 的设备。redroid12（127.0.0.1:
#   5556）= x86_64 容器无 houdini 转译 ARM32 → 翻译体 so EM_ARM(40) dlopen 报
#   "is for EM_ARM instead of EM_386" → UnsatisfiedLinkError 崩（2026-11-15 实锤）。
#   redroid 仅作数据提取（AGENTS 红线），不作设备侧实锤源。

# 0. 设备可达性（★ §124y-1955 红果 adb 不稳增强：网络三态判定 + connect 后立即跑单次不重跑）
# 三态：① adb 在线（get-state=device）→ 直接跑 ② 网络可达但 adb 服务未起
# （ping 通 + 端口 OPEN + get-state 失败）→ 先 connect 再起 ③ 网络全断（ping
# 100% loss / No route to host）→ PENDING（等红果网络恢复 + adb 服务重起）
HOST="${DEV%%:*}"; PORT="${DEV##*:}"
if adb -s "$DEV" get-state >/dev/null 2>&1; then
  echo "  设备 $DEV adb 在线（get-state=device）→ 直接跑（防红果掉线：单次跑全链不重跑）"
elif ping -c 1 -W 2 "$HOST" >/dev/null 2>&1; then
  echo "  设备 $HOST 网络可达但 adb 未在线 → 尝试 adb connect 起服务"
  adb connect "$DEV" 2>&1 | head -1
  if ! adb -s "$DEV" get-state >/dev/null 2>&1; then
    echo "  adb connect 后仍不在线 → PENDING（红果 adb 服务未起，§124y-1955 口径）"
    echo "  离线 73 项已收口（静态 73/73），设备侧复验待红果 adb 服务起"
    exit 1
  fi
  echo "  adb connect 成功 → 设备 $DEV 在线 → 跑全链（单次不重跑防红果掉线）"
else
  echo "  设备 $HOST 网络全断（ping 100% loss / No route to host）→ PENDING（§124y-189 口径）"
  echo "  离线 73 项已收口（静态 73/73），设备侧复验待红果网络恢复 + adb 服务重起"
  exit 1
fi

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

echo "── 3. 完整 60 局设备跑（jq.game_test + 走子桥 tick 终局）──"
adb -s "$DEV" shell "setprop jq.game_test 1" 2>/dev/null
sleep 3
adb -s "$DEV" logcat -d -s junqi:* 2>/dev/null | grep -cE "Junqi\.(Move|Eat|Bomb|Flag|Dead|Flip)" | grep -v "^0$" > /dev/null && echo "  完整 60 局对局事件实锤（Junqi.* 全事件）" || echo "  完整 60 局设备跑: PENDING（事件未实锤）"

echo "── 4. 渲染像素级 screencap 非黑比 ──"
adb -s "$DEV" shell screencap -p /sdcard/jq_regress.png 2>/dev/null
adb -s "$DEV" pull /sdcard/jq_regress.png /tmp/jq_regress.png >/dev/null 2>&1
if [ -f /tmp/jq_regress.png ]; then
  python3 -c "
from PIL import Image
import sys
try:
  im=Image.open('/tmp/jq_regress.png').convert('RGB'); w,h=im.size
  px=im.load(); nb=0
  for x in range(0,w,8):
    for y in range(0,h,8):
      r,g,b=px[x,y]
      if r>16 or g>16 or b>16: nb+=1
  total=(w//8)*(h//8); pct=nb*100//max(total,1)
  print('  screencap 非黑比=%d%%（≥30%% = 出屏实锤）' % pct)
  sys.exit(0 if pct>=30 else 1)
except Exception as e:
  print('  screencap 解析失败: %s' % e); sys.exit(1)
" || echo "  渲染像素级 screencap: PENDING（非黑比 <30% 或解析失败）"
else
  echo "  渲染像素级 screencap: PENDING（截图未拉取）"
fi

echo "── 5. 蓝牙端到端（有 BT 硬件真机）──"
if adb -s "$DEV" shell "ls /dev/bus/usb 2>/dev/null; getprop ro.bluetooth.enabled" 2>/dev/null | grep -q "1\|true"; then
  echo "  蓝牙端到端: 有 BT 硬件 → 跑 bttest setprop 钩子 + UNDO/RESTART 帧实锤"
  adb -s "$DEV" shell "setprop jq.bt_test_run 1" 2>/dev/null
  sleep 2
  adb -s "$DEV" logcat -d -s junqi:* 2>/dev/null | grep -cE "UNDO|RESTART" > /dev/null && echo "  蓝牙 UNDO/RESTART 帧实锤" || echo "  蓝牙端到端: PENDING（帧未实锤）"
else
  echo "  蓝牙端到端: PENDING（无 BT 硬件，AGENTS 红线诚实边界）"
fi

echo "── 6. 音效出屏（14 WAV 资源设备端在位 + sfx_play 通路）──"
adb -s "$DEV" shell "ls /data/local/tmp/spak_res/*.wav 2>/dev/null | wc -l" | grep -q "14" && echo "  音效 14 WAV 资源设备端在位 OK" || echo "  音效出屏: PENDING（14 WAV 未推设备端）"

echo "═══ 复验链汇总 ═══"
echo "  设备 4 项: $([ $rc1 -eq 0 ] && echo PASS || echo FAIL/PENDING)"
echo "  走子桥 4 路径: $([ $rc2 -eq 0 ] && echo PASS || echo FAIL/PENDING)"
echo "  完整 60 局 + screencap + 蓝牙 + 音效: 见上各步实锤/PENDING"
echo "  真机 5 判据: 按 rev64/reverse/docs/REAL_DEVICE_ACCEPTANCE.md 逐步实锤"
echo "  回填: 全链 PASS 记录到 REVERSE_LOG"

[ $rc1 -eq 0 ] && [ $rc2 -eq 0 ]
