#!/bin/bash
# mumu_verify.sh — MuMu 实机验证脚本（§124zz 固化复用）
#
# 一条命令完成：连接 → 装 APK → 启动 → 采集 logcat → 截图 → 关键日志摘要 → 判定
# 用法：
#   bash rev64/tools/mumu_verify.sh [apk路径] [设备地址]
#   默认 apk = /tmp/opencode/p32_v178e5.apk，设备 = 10.0.182.146:5555
#
# 判定逻辑（诚实边界）：
#   - dlopen 失败 → FAIL（UnsatisfiedLinkError / failed to link）
#   - 进程存活 + 无异常 → PASS(运行)   ← 注意：≠"能玩"，≠"棋盘已画出"
#   - 额外看纹理管线日志（F_9e204/F_9e0f0）→ 判定纹理是否真上传
set -u
APK=${1:-/tmp/opencode/p32_v178e5.apk}
DEV=${2:-10.0.182.146:5555}
# ★ PKG 必须从 APK 自动推导（§〇d 修正：旧版硬编码 com.cnvcs.junqi → 验错包！
#   翻译体 APK 是 com.cnvcs.junqi32/1.78，原版 com.cnvcs.junqi/1.52 同设备共存）
AAPT=$(ls /disk/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots/*/fs/opt/android/build-tools/*/aapt 2>/dev/null | head -1)
PKG=$("$AAPT" dump badging "$APK" 2>/dev/null | grep -oE "package: name='[^']+'" | head -1 | sed "s/package: name='//;s/'//")
[ -z "$PKG" ] && PKG=com.cnvcs.junqi32
OUT=/tmp/opencode/mumu_verify
mkdir -p $OUT
echo "  ★ 目标包: $PKG（从 APK 自动推导）"

echo "=== [1/6] 连接设备 $DEV ==="
adb disconnect $DEV >/dev/null 2>&1
adb connect $DEV 2>&1 | tail -1
if ! adb -s $DEV get-state >/dev/null 2>&1; then
  echo "★ 设备不可达（$DEV）——无法验证"
  exit 3
fi
adb -s $DEV shell getprop ro.product.model 2>/dev/null | tr -d '\r'

echo "=== [2/6] 安装 $APK ==="
adb -s $DEV push "$APK" /data/local/tmp/verify.apk 2>&1 | tail -1
adb -s $DEV shell "su 0 pm install -r /data/local/tmp/verify.apk" 2>&1 | tr -d '\r' | tail -1

echo "=== [3/6] 启动 ==="
adb -s $DEV logcat -c >/dev/null 2>&1
adb -s $DEV shell "am force-stop $PKG; am start -n $PKG/com.cnvcs.App" 2>&1 | tail -1
# ★ AUD-1000-ALL A12-F12-004（P2 已修）：固定 sleep 12 改轮询（MuMu 冷启首帧窗口 20-25s，
#   固定 12s 截图全黑假红；轮询最长 30s，进程起来即走后续段）。
for _w in $(seq 1 15); do
  sleep 2
  PID=$(adb -s $DEV shell "pidof $PKG" 2>/dev/null | tr -d '\r\n ')
  [ -n "$PID" ] && break
done

echo "=== [4/6] 进程状态 ==="
PID=$(adb -s $DEV shell "pidof $PKG" 2>/dev/null | tr -d '\r\n ')   /* 轮询段后重读（进程可能已起） */
echo "  pid = ${PID:-<无，进程已死>}"

echo "=== [5/6] 日志摘要 ==="
adb -s $DEV logcat -d 2>/dev/null > $OUT/logcat.txt
echo "  --- 链接/崩溃 ---"
grep -iE "UnsatisfiedLink|failed to link|FATAL|AndroidRuntime" $OUT/logcat.txt | head -8
echo "  --- 纹理管线 ---"
grep -E "F_9e204|F_9e0f0|F_9e0d4|F_9e2b4" $OUT/logcat.txt | head -10
echo "  --- GL 桥/出帧 ---"
grep -E "gl_bridge|循环出帧" $OUT/logcat.txt | tail -5
echo "  --- 错误汇总（junqi tag）---"
grep -iE "junqi" $OUT/logcat.txt | grep -iE "失败|错误|error|fail|未解析|无数据" | head -10

echo "=== [6/6] 截图 ==="
adb -s $DEV shell "screencap -p /data/local/tmp/verify.png" 2>/dev/null
adb -s $DEV pull /data/local/tmp/verify.png $OUT/verify.png 2>&1 | tail -1

echo
echo "=== 判定 ==="
if grep -q "UnsatisfiedLink\|failed to link" $OUT/logcat.txt; then
  echo "FAIL —— dlopen 失败（链接问题；跑 link_audit.py 查）"
  exit 2
elif [ -z "${PID:-}" ]; then
  echo "FAIL —— 进程已死（查上面的崩溃日志）"
  exit 2
else
  echo "PASS(运行) —— 进程存活 pid=$PID"
  echo "  ★ 诚实边界：进程存活 ≠ 棋盘已画出 ≠ 能玩。"
  echo "    看 $OUT/verify.png 是否有棋盘纹理；看上面『纹理管线』日志 F_9e204/F_9e0f0 是否命中。"
  exit 0
fi
