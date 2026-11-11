#!/bin/bash
# rs_ui_verify.sh — 正向 app UI 验证一条龙（脚本化/批量化）
# 用法:
#   bash rev64/tools/rs_ui_verify.sh                     # build so → rebuild → start → 截图 → 房子/背景对比
#   bash rev64/tools/rs_ui_verify.sh --no-rebuild        # 只 start + 截图（用当前已装 APK）
#   bash rev64/tools/rs_ui_verify.sh --dev IP:PORT
# 判据:
#   [房子] 原版房子区域红顶+白墙像素 > 阈值（房子图标显示）
#   [背景] 全屏非黑比 > 90%（背景/按钮/标题渲染）
#   [崩溃] pid 存活 + 0 FATAL
set -u
WS=/disk/workspaces/jq
RS=$WS/rev64/forward/src_rs
DEV=${DEV:-10.0.182.146:5555}
REBUILD=1
while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-rebuild) REBUILD=0; shift;;
    --dev) DEV="$2"; shift 2;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done
OUT=/tmp/opencode/ui_verify.png
log() { echo "[ui_verify] $*"; }

# 0. 设备可达
adb -s $DEV connect $DEV >/dev/null 2>&1
if ! adb -s $DEV shell true >/dev/null 2>&1; then
  log "★ 设备不可达: $DEV（ping/网络断，检查 MuMu/设备）"; exit 1
fi

# 1. 构建 so（可选）
if [[ $REBUILD -eq 1 ]]; then
  log "=== [1/5] 构建 so ==="
  (cd $RS && bash build_rs.sh so 2>&1 | tail -3)
  log "=== [2/5] rebuild apk + 装机 + spak_res ==="
  bash $WS/rev64/tools/rebuild_rs_apk.sh --dev $DEV 2>&1 | grep -E "✓ so|Success" | tail -2
fi

# 2. 启动 app
log "=== [3/5] 启动正向 app ==="
adb -s $DEV shell am force-stop com.qwsaeda.junqirs >/dev/null 2>&1; sleep 2
adb -s $DEV logcat -c 2>&1; sleep 1
adb -s $DEV shell am start -n com.qwsaeda.junqirs/com.qwsaeda.App >/dev/null 2>&1
sleep 14

# 3. 判据：pid 存活
PID=$(adb -s $DEV shell pidof com.qwsaeda.junqirs 2>/dev/null | tr -d '\r')
if [ -z "$PID" ]; then
  log "★ 崩溃：pid 空"; adb -s $DEV logcat -d 2>/dev/null | grep -iE "FATAL|UnsatisfiedLink" | tail -3; exit 1
fi
log "✓ pid=$PID 存活"

# 4. 截图 + 房子/背景对比
log "=== [4/5] 截图 + 房子/背景对比 ==="
adb -s $DEV exec-out screencap -p > $OUT 2>/dev/null
python3 - <<'PYEOF'
from PIL import Image
f = Image.open('/tmp/opencode/ui_verify.png').convert('RGB')
px = f.load()
# 房子区域（720 基准 44,1056-140,1196 → 900×1600 ×1.25 = 55,1320-175,1495）
red = white = 0
for y in range(1330, 1500):
    for x in range(60, 176):
        r, g, b = px[x, y]
        if r > 220 and g > 220 and b > 200: white += 1
        elif r > 180 and g < 90 and b < 90: red += 1
# 背景非黑比
nb = sum(1 for y in range(0, 1600, 10) for x in range(0, 900, 10) if sum(px[x, y]) > 30)
ratio = nb / 14400 * 100
print(f'[ui_verify] 房子区域: 红顶={red} 白墙={white}（阈值 红顶>200 且 白墙>100）')
print(f'[ui_verify] 背景非黑比: {ratio:.1f}%（阈值 >90%）')
ok = (red > 200 and white > 100 and ratio > 90.0)
print(f'[ui_verify] 判定: {"PASS" if ok else "FAIL"}（房子红顶+白墙+背景全达标）')
exit(0 if ok else 1)
PYEOF
rc=$?
[ $rc -ne 0 ] && { log "★ UI 验证 FAIL（房子未显示或背景退化）"; exit 1; }

# 5. 崩溃检查
log "=== [5/5] 崩溃检查 ==="
CRASH=$(adb -s $DEV logcat -d 2>/dev/null | grep -cE "FATAL EXCEPTION|Fatal signal|UnsatisfiedLink")
if [ "$CRASH" -gt 0 ]; then
  log "★ 崩溃日志 $CRASH 条"; adb -s $DEV logcat -d 2>/dev/null | grep -E "FATAL|Fatal signal|UnsatisfiedLink" | tail -3; exit 1
fi
log "✓ UI 验证全 PASS（房子显示 + 背景正常 + 0 崩溃）"
