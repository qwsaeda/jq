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

# 4. 截图 + 全项 UI 对比（主界面按钮/标题/房子/背景 + ☰菜单星形）
log "=== [4/5] 截图 + 全项 UI 对比 ==="
adb -s $DEV exec-out screencap -p > $OUT 2>/dev/null
python3 - <<'PYEOF'
from PIL import Image
f = Image.open('/tmp/opencode/ui_verify.png').convert('RGB')
px = f.load()
results = []

# ① 房子区域（720 基准 44,1056-140,1196 → 900×1600 ×1.25 = 55,1320-175,1495）
# ★ icon_house.png 实际配色 = 灰墙(224,224,224)+暗红顶(192,0,0)（非纯白纯红，
#   blend+清屏后色偏，判据配色放宽到实际渲染色）
red = white = 0
for y in range(1330, 1500):
    for x in range(60, 176):
        r, g, b = px[x, y]
        if r > 150 and g > 150 and b > 150: white += 1        # 灰白墙
        elif r > 120 and g < 70 and b < 70: red += 1           # 暗红顶
results.append(('房子红顶+白墙', red > 100 and white > 100, f'红顶={red} 白墙={white}'))

# ② 背景非黑比 + 绿色像素（草地，区分纯色退化：白色/棕色非黑比高但绿=0）
nb = sum(1 for y in range(0, 1600, 10) for x in range(0, 900, 10) if sum(px[x, y]) > 30)
ratio = nb / 14400 * 100
green = sum(1 for y in range(0, 1600, 10) for x in range(0, 900, 10)
            if px[x, y][1] > 100 and px[x, y][1] > px[x, y][0] + 30 and px[x, y][1] > px[x, y][2] + 60)
results.append(('背景', ratio > 90.0 and green > 500, f'非黑={ratio:.1f}% 绿={green}'))

# ③ 主界面按钮（蓝底，5 按钮 y=1084-1479 区）：检测按钮区非纯色（有蓝色按钮）
btn_blue = sum(1 for y in range(1084, 1479, 4) for x in range(215, 645, 4)
               if px[x, y][2] > 120 and px[x, y][2] > px[x, y][0] + 30 and px[x, y][1] < 120)
results.append(('主界面按钮', btn_blue > 500, f'蓝色像素={btn_blue}'))

# ④ ☰菜单星形（黄色，需先截☰菜单图——此处用主界面黄色元素粗检，非严格）
# 注：☰菜单需 tap ☰ 按钮后截图，主界面无星形；此项在独立菜单截图验证
# ⑤ 标题区（军棋绿字在背景图内，y=240-460）：绿色像素存在（标题渲染）
title_g = sum(1 for y in range(240, 460, 4) for x in range(180, 720, 4)
              if px[x, y][0] < 200 and px[x, y][1] > 120 and px[x, y][2] < 130
              and px[x, y][1] > px[x, y][0] + 20 and px[x, y][1] > px[x, y][2] + 40)
results.append(('军棋标题', title_g > 500, f'绿色像素={title_g}'))

# 汇总
print(f'[ui_verify] 全项 UI 对比:')
allok = True
for name, ok, detail in results:
    print(f'  {"✓" if ok else "✗"} {name}: {detail}')
    allok = allok and ok
print(f'[ui_verify] 判定: {"PASS" if allok else "FAIL"}（房子+背景+按钮+标题 全达标）')
exit(0 if allok else 1)
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
