#!/bin/bash
# mumu_bt_test.sh — 蓝牙帧状态机 MuMu 实机验证（编译进 so 的部分）
# 用法: bash rev64/tools/mumu_bt_test.sh [设备]
# 做: ① make bttest（桌面 14 断言全过才继续）② 编译 so  ③ 装 MuMu
#     ④ 起 app  ⑤ 验证进程存活 + 无崩 + bt_session 已链入 so（nm 符号检查）
# 诚实边界: 无真机蓝牙 SPP 通路验证（Java BluetoothSocket 宿主承担）；
#   帧逻辑正确性由 bttest 桌面断言覆盖，MuMu 只验"链入+不崩"。
set -u
DEV=${1:-127.0.0.1:5555}
RS=/disk/workspaces/jq/rev64/src_rs
NDK=/tmp/opencode/android-ndk-r25b

echo "=== [1/5] 桌面蓝牙自测（bttest）==="
cd $RS
if ! make bttest 2>&1 | grep -q "全部通过"; then
    echo "★ bttest 有 FAIL → 中止（帧逻辑未过）"; exit 1
fi
echo "  PASS: 14 项断言全过"

echo "=== [2/5] 编译 so + 确认 bt_session 符号链入 ==="
NDK=$NDK make so >/dev/null 2>&1 || { echo "★ 编译失败"; exit 1; }
BT_SYMS=$(nm dist/libjunqi_rs.so 2>/dev/null | grep -c "jq_bt_session")
echo "  jq_bt_session* 符号数: $BT_SYMS"
[ "$BT_SYMS" -ge 3 ] || { echo "★ bt_session 未链入 so（符号<3）"; exit 1; }
echo "  PASS: bt_session 链入 so"

echo "=== [3/5] 打包 + 装 MuMu $DEV ==="
bash $RS/../tools/mumu_swap_so.sh 2>&1 | grep -E "APK:|Success|FAIL" | head -3

echo "=== [4/5] 进程存活 + 崩溃检查 ==="
sleep 8
PID=$(adb -s $DEV shell pidof com.cnvcs.junqi | tr -d '\r')
CRASH=$(adb -s $DEV logcat -d | grep -cE "Fatal signal|SIGSEGV" | tr -d ' ')
echo "  pid=$PID crash_count=$CRASH"
[ -n "$PID" ] && [ "$CRASH" = "0" ] && echo "  PASS: 进程存活无崩溃" || { echo "★ FAIL"; exit 1; }

echo "=== [5/5] OnBluetooth 入口存在（JNI 符号导出）==="
JNI_BT=$(readelf -sW $RS/dist/libjunqi_rs.so | grep -c "Java_com_cnvcs_App_OnBluetooth")
[ "$JNI_BT" -ge 1 ] && echo "  PASS: OnBluetooth JNI 导出存在" || { echo "★ OnBluetooth 未导出"; exit 1; }

echo
echo "★ bttest PASS + so 链入 + MuMu 不崩 + JNI 导出"
echo "  诚实边界: SPP socket 宿主承担，真机蓝牙联机需真机两设备验收"
