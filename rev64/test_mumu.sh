#!/bin/bash
# test_mumu.sh — MuMu 测试固化脚本（§124ai）
# 用法: ./test_mumu.sh <apk路径> [等待秒]
# 流程: 推→装→清logcat→启→等→判存活→判崩溃→截图→取样色→抓关键日志
set -e
APK=${1:?用法: test_mumu.sh <apk> [等待秒]}
WAIT=${2:-8}
ADB="adb -s 10.0.182.146:5555"

echo "=== [1/7] 推 APK ==="
$ADB push "$APK" /sdcard/$(basename "$APK") 2>&1 | tail -1

echo "=== [2/7] 安装 ==="
$ADB shell "su 0 pm install -r /sdcard/$(basename "$APK")" 2>&1 | tail -1

echo "=== [3/7] 清 logcat + 启动 ==="
$ADB logcat -c 2>/dev/null
$ADB shell "am force-stop com.cnvcs.junqi; sleep 1; am start -n com.cnvcs.junqi/com.cnvcs.App" 2>&1 | tail -1

echo "=== [4/7] 等待 ${WAIT}s ==="
sleep $WAIT

echo "=== [5/7] 存活/崩溃判定 ==="
PID=$($ADB shell "pidof com.cnvcs.junqi" 2>/dev/null | tr -d '\r')
echo "PID: ${PID:-DEAD}"
CRASH=$($ADB logcat -d 2>/dev/null | grep -cE "Fatal signal|SIGSEGV|SIGABRT")
echo "crash count: $CRASH"

echo "=== [6/7] 截图取样色 ==="
$ADB exec-out screencap -p > /tmp/opencode/mumu_test.png 2>/dev/null
python3 -c "
from PIL import Image
try:
    im=Image.open('/tmp/opencode/mumu_test.png').convert('RGB')
    w,h=im.size
    pts=[(w//2,h//2),(w//4,h//2),(w//2,h//4),(w//4,h//4),(w//2,3*h//4)]
    for p in pts: print(p, im.getpixel(p))
except Exception as e:
    print('截图失败', e)
"

echo "=== [7/7] 关键日志（junqi 非噪声）==="
$ADB logcat -d 2>/dev/null | grep -E "junqi" | grep -vE "_ZdlPv SKIP|MEMLOG|jqjni2" | tail -20
