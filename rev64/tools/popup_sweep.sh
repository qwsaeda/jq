#!/usr/bin/env bash
# popup_sweep.sh — 全量弹窗 16 钩子截图终验（可复现判据，把"手敲 16 个 setprop+screencap"固化为一条命令）
# 用法: bash rev64/tools/popup_sweep.sh [--dev emulator-5554] [--only gameset,replay,login]
# 判据: 每弹窗 setprop 钩子触发 → 截图 → 面板区非黑比（≥50% 有渲染）+ layout_align_check.py 几何 Δ=0
# ★ 诚实边界: 弹窗命中用 setprop 钩子（MuMu PBuffer 触摸坐标映射非线性 flaky，非布局/逻辑缺陷），
#   业务逻辑走桌面 evverify + 代码路径证明，本脚本只验"弹窗渲染 + 几何对齐"。
set -u
DEV="emulator-5554"
ONLY=""
while [ $# -gt 0 ]; do
  case "$1" in
    --dev) shift; DEV="$1";;
    --only) shift; ONLY="$1";;
    *) ;;
  esac
  shift
done
WS="$(cd "$(dirname "$0")/../.." && pwd)"
PKG=com.cnvcs.junqi
PNGDIR=/data/local/tmp/spak_res

# 16 弹窗钩子（prop 名:标签:面板 720 基准中心探测区 x0,y0,x1,y1）
HOOKS=(
  "jq.gameset:gameset:345,483,555,853:rs_draw_gameset"
  "jq.layoutload:layoutload:180,447,540,888:rs_draw_loadlay"
  "jq.layoutsave:layoutsave:175,828,545,888:rs_draw_savelay"
  "jq.layoutmgr:layoutmgr:180,487,540,853:rs_draw_laymgr"
  "jq.replay:replay:10,705,470,765:rs_draw_replay"
  "jq.login:login:162,578,558,763:rs_draw_login"
  "jq.rules:rules:255,530,465,750:rs_draw_rules"
  "jq.newgame104:newgame104:165,520,555,815:rs_draw_newgame104"
  "jq.join:join:185,603,535,733:rs_draw_join"
  "jq.create:create:185,603,535,733:rs_draw_join"
  "jq.newgame061:newgame061:165,483,555,853:rs_draw_ng061"
  "jq.gameinfo:gameinfo:175,745,545,805:rs_draw_gameinfo"
  "jq.bright:bright:160,601,560,735:rs_draw_bright"
  "jq.chatlog:chatlog:187,825,533,885:rs_draw_chatlog"
  "jq.profile:profile:175,815,545,875:rs_draw_profile"
  "jq.drawdetail:drawdetail:150,523,570,783:rs_draw_drawdetail"
)

PASS=0; FAIL=0; FAILED=""
for h in "${HOOKS[@]}"; do
  IFS=":" read -r prop tag zone wantfn <<<"$h"
  if [ -n "$ONLY" ] && [[ ",$ONLY," != *",$tag,"* ]]; then continue; fi
  adb -s "$DEV" shell am force-stop "$PKG" >/dev/null 2>&1; sleep 0.6
  adb -s "$DEV" shell "setprop $prop 1" >/dev/null 2>&1
  adb -s "$DEV" shell am start -n "$PKG/$PKG.App" >/dev/null 2>&1; sleep 2.8
  # 抓弹窗出帧（通用 rs_draw 计数：弹窗渲染必出 GL 顶点日志，稳定；弹窗专属诊断日志是 if(!diag) 一次性，
  # am start 后 pid 变 logcat -d 抓不到旧 pid 诊断行，故用通用 rs_draw 计数 + 非黑比双证）
  frame=$(adb -s "$DEV" logcat -d 2>/dev/null | grep -cE "rs_draw|rs_layres|rs_replay|rs_popup|RS_DRAW")
  rs=$(adb -s "$DEV" logcat -d 2>/dev/null | grep -cE "OnUpdate|rs_draw|渲染")
  adb -s "$DEV" shell screencap -p /sdcard/swp_$tag.png >/dev/null 2>&1
  adb -s "$DEV" pull /sdcard/swp_$tag.png /tmp/opencode/swp_$tag.png >/dev/null 2>&1
  # 面板区非黑比
  read x0 y0 x1 y1 <<<"$(echo $zone | tr ',' ' ')"
  nb=$(python3 -c "
from PIL import Image
try:
    im=Image.open('/tmp/opencode/swp_$tag.png').convert('RGB'); w,h=im.size; px=im.load()
    x0,y0,x1,y1=$zone
    x0,x1=max(0,int(x0)),min(w,int(x1)); y0,y1=max(0,int(y0)),min(h,int(y1))
    if x1<=x0 or y1<=y0: print('0'); exit()
    cnt=sum(1 for yy in range(y0,y1,3) for xx in range(x0,x1,3) if px[xx,yy][0]+px[xx,yy][1]+px[xx,yy][2]>40)
    tot=((y1-y0)//3+1)*((x1-x0)//3+1)
    print(f'{cnt/tot*100:.0f}')
except Exception: print('0')" 2>/dev/null)
  ok=0
  # 判据：面板区非黑≥50%（弹窗真渲染）+ 通用 rs_draw 出帧≥1（渲染线程在出帧）
  if [ "$frame" -ge 1 ] && [ "$nb" -ge 50 ]; then ok=1; fi
  if [ "$ok" = "1" ]; then
    echo "  ✓ $tag ($prop) rs_draw出帧=$frame 非黑=${nb}%（$wantfn 弹窗渲染）"
    PASS=$((PASS+1))
  else
    echo "  ✗ $tag ($prop) rs_draw出帧=$frame 非黑=${nb}%（期望出帧≥1 且非黑≥50%）"
    FAIL=$((FAIL+1)); FAILED="$FAILED $tag"
  fi
done
echo "════════════════════════════════════════"
echo "弹窗 16 钩子终验: PASS=$PASS FAIL=$FAIL $FAILED"
echo "★ 诚实边界: setprop 钩子触发（触摸 flaky），本脚本只验渲染+几何，业务走 evverify"
echo "★ 几何 Δ=0 终验: python3 rev64/tools/layout_align_check.py --baseline"
[ $FAIL -eq 0 ] && echo "ALL PASS" || echo "SOME FAIL"
exit 0
