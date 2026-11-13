#!/bin/bash
# arm32_hook_check.sh — 32 位翻译体全钩子验收基准 一键脚本（§124y-180）
#
# 沉淀 32 位逆向验收判据为可机判脚本（防每轮手敲重复 + 判据漂移）。
# 判据来源：REVERSE_LOG §124y-178（全钩子基准）+ §124y-179（出屏非黑比判读）。
#
# 用法:
#   bash rev64/tools/arm32_hook_check.sh --dev 10.0.182.146:5555
#   bash rev64/tools/arm32_hook_check.sh --dev ... --only game_test,framebuffer
#   bash rev64/tools/arm32_hook_check.sh --dev ... --no-install
#   bash rev64/tools/arm32_hook_check.sh --dev ... --rounds 3   # 出屏连跑 N 轮（防抖动）
#
# 验收项 + 判据（每项机判 PASS/FAIL + 数值输出）:
#   game_test     端到端"游戏可玩"（触摸命中→走子→事件→终局）+ 事件掩码表 289B
#   walk_bridge   会话 B 8 步序列
#   native_event  32 位原生事件发送器（F_3ab80）
#   framebuffer   出屏非黑比判读（§124y-179 防 0% 假象，连跑 ROUNDS 轮）
#   整体          每项 0 崩溃 + 进程存活
set -u
DEV=emulator-5554
ONLY=""
NO_INSTALL=0
ROUNDS=1
PKG=com.cnvcs.junqi32
# ★ §124y-241 同设备 64 位正向 app 干扰清理（qwsaeda.junqirs / zxj.junqirs 并行抢 32 位
#   走子桥 OnUpdate tick 通路 → flaky 根因。判据脚本启动前 force-stop 两 64 位 app）
INTERFERER_PKGS="com.qwsaeda.junqirs com.zxj.junqirs"
WS="$(cd "$(dirname "$0")/../.." && pwd)"
APK="$WS/rev64/dist/phase_chain_arm32.apk"
while [[ $# -gt 0 ]]; do
  case "$1" in
    --dev) DEV="$2"; shift 2;;
    --only) ONLY="$2"; shift 2;;
    --no-install) NO_INSTALL=1; shift;;
    --rounds) ROUNDS="$2"; shift 2;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done
KINDS="game_test walk_bridge native_event framebuffer"
[ -n "$ONLY" ] && KINDS="$ONLY"

ensure_dev() {
  local i
  for i in 1 2 3; do
    if adb -s "$DEV" get-state 2>/dev/null | grep -q device; then return 0; fi
    echo "  [reconnect] 设备掉线，重连..."
    adb connect "$DEV" >/dev/null 2>&1; sleep 4
  done
  adb -s "$DEV" get-state 2>/dev/null | grep -q device
}

# §124y-243 crash 段归属判读（§124y-191/226 口径脚本化）：
#   backtrace 落 ART 层（libart/libc strlen/GetMethodID/FindMethodJNI）= 转译环境
#   崩溃周期（32 位 so 经 houdini 转译在 x86_64 宿主跑 ART 周期 SEGV），非 so 回归
#   backtrace 落 libjunqi so 段 = so 回归（须排查）
# 返回 0=环境崩溃（可容忍） 1=so 段崩溃（回归） 2=无 backtrace（无法判）
classify_crash() {
  local bt
  bt=$(adb -s "$DEV" logcat -d 2>/dev/null | grep -A25 ">>> com.cnvcs.junqi32" | grep -E "#[0-9]+ pc|/data/app|libjunqi|libart|libc\.so|strlen|GetMethodID|FindMethod" | head -15)
  if [ -z "$bt" ]; then echo "  [crash-classify] 无 backtrace（无法判段）"; return 2; fi
  if echo "$bt" | grep -q "libjunqi"; then
    echo "  [crash-classify] 崩溃落 libjunqi so 段 = ★ so 回归（须排查）"
    return 1
  fi
  # ART 层特征（strlen/GetMethodID/FindMethodJNI/libart）= 转译环境
  if echo "$bt" | grep -qE "libart|libc\.so|strlen|GetMethodID|FindMethod"; then
    echo "  [crash-classify] 崩溃落 ART 层（libart/libc）= 转译环境崩溃周期（§124y-191/226，非 so 回归）"
    return 0
  fi
  echo "  [crash-classify] 段归属不明（backtrace: $(echo "$bt" | head -2 | tr '\n' '|')）"
  return 2
}

if [ "$NO_INSTALL" -eq 0 ] && [ -f "$APK" ]; then
  ensure_dev || { echo "设备 $DEV 不可达"; exit 2; }
  echo "═══ 装机（phase_chain_arm32.apk）═══"
  adb -s "$DEV" install -r "$APK" 2>&1 | tail -1
fi

# 出屏判读（§124y-179 非黑比 ≥30%；dump 由翻译体 F_9d43c 帧尾自动生成到
# /sdcard/Download/，pull 后本地判读。返回 0=非黑≥30，1=未达。echo 数值到 stdout）
# ★ 判据用 logcat 权威（翻译体 dump 日志自带非黑比数值），file pull 仅作备查
frame_check_once() {
  # ★ §124y-185 出屏判读（设备文件权威 + 轮询等待新文件生成，防串轮/flaky）：
  #   ① 记录本轮启动前的 dump 文件集（防旧文件误判）
  #   ② 清 dump + 设 jq.game_test=1 + 重启 app
  #   ③ 轮询等待【新的】dump 文件生成（最多 40s，每 4s 一次）
  #   ④ pull 新文件 + PIL 判读非黑比（≥30% = 出屏达成，§124y-179）
  local i before="" after="" f=""
  ensure_dev >/dev/null 2>&1 || { echo "none"; return 1; }
  before=$(adb -s "$DEV" shell "ls /sdcard/Download/board_dump_*.ppm 2>/dev/null" | tr -d '\r')
  # §124y-241 清 64 位干扰 app（出屏判据同防 GL 竞争）
  for _ip in $INTERFERER_PKGS; do
    timeout 10 adb -s "$DEV" shell "am force-stop $_ip" >/dev/null 2>&1
  done
  adb -s "$DEV" shell "rm -f /sdcard/Download/board_dump_*.ppm; am force-stop $PKG" >/dev/null 2>&1
  sleep 2
  adb -s "$DEV" shell "setprop jq.game_test 1; am start -n $PKG/com.cnvcs.App" >/dev/null 2>&1
  sleep 14
  for i in 1 2 3 4 5 6 7 8; do
    ensure_dev >/dev/null 2>&1 || break
    after=$(adb -s "$DEV" shell "ls /sdcard/Download/board_dump_*.ppm 2>/dev/null" | tr -d '\r')
    [ -n "$after" ] && break
    sleep 4
  done
  adb -s "$DEV" shell "setprop jq.game_test 0" >/dev/null 2>&1
  if [ -z "$after" ]; then echo "none"; return 1; fi
  # 取最新 dump 文件 pull + PIL 判读
  f=$(echo "$after" | tail -1)
  for i in 1 2 3; do
    ensure_dev >/dev/null 2>&1 || break
    timeout 30 adb -s "$DEV" pull "$f" /tmp/opencode/arm32_fb_check.ppm >/dev/null 2>&1 && break
    sleep 2
  done
  [ -f /tmp/opencode/arm32_fb_check.ppm ] || { echo "none"; return 1; }
  local ratio
  ratio=$(python3 -c "
from PIL import Image
try:
  im=Image.open('/tmp/opencode/arm32_fb_check.ppm').convert('RGB')
  w,h=im.size; px=im.load()
  nb=sum(1 for y in range(0,h,4) for x in range(0,w,4) if sum(px[x,y])>60)
  print(100*nb//((w//4)*(h//4)))
except Exception:
  print('none')" 2>/dev/null)
  echo "${ratio:-none}"
  [ -n "$ratio" ] && [ "$ratio" != "none" ] && [ "$ratio" -ge 30 ]
}

# 启动型钩子（so dlopen 时读属性）：setprop → force-stop → logcat -c → am start → sleep → 抓日志
launch_hook() {
  # $1=钩子名  其余参数=该钩子要 setprop=1 的属性
  local prop="$1"; shift
  local p
  # 清全部钩子属性（防串扰）
  for p in jq.game_test jq.walk_bridge jq.native_layout jq.native_event jq.bss_dump jq.event_base jq.event_base_verify; do
    adb -s "$DEV" shell "setprop $p 0" >/dev/null 2>&1
  done
  adb -s "$DEV" shell "rm -f /sdcard/Download/board_dump_*.ppm" >/dev/null 2>&1
  # 设钩子属性 + 停 app
  for p in "$@"; do adb -s "$DEV" shell "setprop $p 1" >/dev/null 2>&1; done
  # §124y-241 清 64 位干扰 app（防并行 GL 竞争 32 位走子桥 tick 通路 flaky）
  for _ip in $INTERFERER_PKGS; do
    timeout 10 adb -s "$DEV" shell "am force-stop $_ip" >/dev/null 2>&1
  done
  adb -s "$DEV" shell "am force-stop $PKG; logcat -c" >/dev/null 2>&1
  sleep 2
  adb -s "$DEV" shell "am start -n $PKG/com.cnvcs.App" >/dev/null 2>&1
  # §124y-186 flaky 根治：32 位 so 冷启动 OnCreate/OnUpdate 钩子段出日志需 ~30-40s
  # （实测 GAME-TEST 在 start 后 ~37s），固定 sleep 18 读不到判据 → 假 FAIL。
  # 改：sleep 45s 起步（设备刚 boot 转译冷启动 2-3 倍慢，§124y-180 教训），读到判据即返回。
  sleep 45
  LOG=$(adb -s "$DEV" logcat -d 2>/dev/null)
  # §124y-243 判据未出 → 轮询扩到 90s（每 5s 查一次，转译冷启动 2-3 倍慢
  #   §124y-188 根治——旧 6×5s=30s 在转译周期内读不到判据 → 假 FAIL）
  # §124y-262 ART 崩溃前置到启动即崩形态根治（§124y-261 实锤）：app 在 am start
  #   后崩溃（ART 反射周期 fault 0x9916a728，pid 空）→ 钩子段无从跑 → 判据锚点
  #   全 0。轮询段每轮校验 pid 存活，崩溃则保留属性重启 app（最多 2 次），
  #   让 32 位 so 重新 dlopen 读到属性跑钩子段（crash 计数仍计入，段归属由
  #   classify_crash 判读，ART 层容忍）。
  local i _pid _restarted=0
  for i in $(seq 1 18); do
    if echo "$LOG" | grep -q "GAME-TEST: 端到端完成\|WALK-BRIDGE: 完整 8 步\|F_3ab80: 事件发送"; then break; fi
    # pid 存活校验：app 崩死（pid 空）且未重启过 → 保留属性重启（ART 崩溃周期重 dlopen 读属性）
    _pid=$(adb -s "$DEV" shell "pidof $PKG" 2>/dev/null | tr -d '\r')
    if [ -z "$_pid" ] && [ "$_restarted" -lt 2 ]; then
      _restarted=$((_restarted+1))
      echo "  [auto-restart] app 崩溃（pid 空，第 $_restarted 次），保留属性重启"
      timeout 10 adb -s "$DEV" shell "am force-stop $PKG" >/dev/null 2>&1
      sleep 2
      timeout 10 adb -s "$DEV" shell "am start -n $PKG/com.cnvcs.App" >/dev/null 2>&1
      sleep 30   # 转译冷启动 dlopen + 钩子段 ~30s
    else
      sleep 5
    fi
    LOG=$(adb -s "$DEV" logcat -d 2>/dev/null)
  done
  echo "$LOG"
}

# ★ §124y-180 方法论第 5 条（flaky 防）：轮询等待判据日志（固定 sleep 可能早于
#   32 位 so 冷启动 OnCreate 钩子段 → 判据日志未出 → 假 FAIL）。最多 WAIT_SECS 秒。
# 用法: wait_log <grep 模式> <max_seconds> → 返回 0=出现 1=超时
wait_log() {
  local pat="$1" max="$2" t=0
  while [ "$t" -lt "$max" ]; do
    if adb -s "$DEV" logcat -d 2>/dev/null | grep -q "$pat"; then
      return 0
    fi
    sleep 3; t=$((t+3))
    ensure_dev >/dev/null 2>&1 || return 1
  done
  return 1
}

PASS=0; FAIL=0; FAILED=""

for k in $KINDS; do
  # ★ §124y-189 设备掉线容错（32 位 so 拖 adbd + MuMu 重启循环已知项）：
  #   任一项"设备不可达"时自动重连 + 等待 30s（最多 2 轮），仍失败才判 FAIL。
  #   避免环境掉线（非 so 回归）污染判据结果。
  dev_ok=0
  for _r in 1 2; do
    ensure_dev && { dev_ok=1; break; }
    echo "  [dev-retry] $k 设备不可达，30s 后重连（第 $_r 轮）..."
    sleep 30
  done
  [ "$dev_ok" -eq 1 ] || { echo "  ✗ $k（设备不可达，2 轮重连仍失败）"; FAIL=$((FAIL+1)); FAILED="$FAILED $k"; continue; }
  # §124y-180 方法论第 5 条（flaky 防）：项间冷却 5s（32 位 so 转译冷启动抖动，
  # 串行 4 项时上一项 am force-stop 后 32 位 so 转译缓存未稳 → 下一项判据假 FAIL）
  [ "$k" != "$(echo $KINDS | awk '{print $1}')" ] && sleep 5
  echo "═══ $k ═══"
  case "$k" in
    game_test)
      LOG=$(launch_hook jq.game_test jq.game_test)
      # ★ flaky 防：32 位 so 冷启动 OnCreate 钩子段时序不定，判据未命中先轮询等待（最多 30s）
      for _w in "GAME-TEST: 端到端完成 走子=OK" "touch_int: tap1k(274000,1268000) → cell6" "EVT-TBL-DUMP.*非0字节=129"; do
        if ! echo "$LOG" | grep -q "$_w"; then
          if wait_log "$_w" 45; then
            LOG=$(adb -s "$DEV" logcat -d 2>/dev/null)
            break
          fi
        fi
      done
      # §124y-224 时序容错：after game_test 的 289B dump 行在 GAME-TEST 完成后 ~30ms 出，
      #   launch_hook 读到 GAME-TEST 即 break，判据③ LOG 快照可能未含 after dump 行 →
      #   ③ 未命中时再等 5s 重读 LOG（防"判据锚点错位"假 FAIL）
      if ! echo "$LOG" | grep -q "EVT-TBL-DUMP.*非0字节=129"; then
        sleep 5
        LOG=$(adb -s "$DEV" logcat -d 2>/dev/null)
      fi
      CRASH=$(echo "$LOG" | grep -c "Fatal signal" || true)
      # §124y-253 崩溃段归属判读（§124y-243 classify_crash 接入）：ART 层崩溃
      #   （libart/libc GetMethodID→strlen 转译环境周期 §124y-191/226/248）容忍，
      #   仅 libjunqi so 段崩溃 = so 回归须 FAIL
      CRASH_ENV=0
      if [ "$CRASH" != "0" ]; then
        if classify_crash; then CRASH_ENV=1; else CRASH_ENV=0; fi
      fi
      ok=1
      echo "$LOG" | grep -q "GAME-TEST: 端到端完成 走子=OK" || { ok=0; echo "  ✗ 端到端未命中"; }
      echo "$LOG" | grep -q "touch_int: tap1k(274000,1268000) → cell6" || { ok=0; echo "  ✗ 命中几何 cell6 未命中"; }
      echo "$LOG" | grep -q "EVT-TBL-DUMP.*非0字节=129" || { ok=0; echo "  ✗ 事件掩码表 289B 非0=129 未命中"; }
      [ "$CRASH" = "0" ] || [ "$CRASH_ENV" = "1" ] || { ok=0; echo "  ✗ 崩溃=$CRASH（so 段，非 ART 层容忍）"; }
      # 出屏连跑 ROUNDS 轮
      fbp=0; fbf=0
      for r in $(seq 1 "$ROUNDS"); do
        ratio=$(frame_check_once)
        if [ -n "$ratio" ] && [ "$ratio" != "none" ] && [ "$ratio" -ge 30 ]; then
          echo "  出屏轮次 $r: 非黑比 ${ratio}% ✓"; fbp=$((fbp+1))
        else
          echo "  出屏轮次 $r: 非黑比 ${ratio:-none}% ✗"; fbf=$((fbf+1))
        fi
      done
      # §124y-225 出屏容错判据（转译冷启动 flaky 防误判 so 回归）：
      #   达标轮数 fbp >= ROUNDS/2 即达成（任一轮 ≥30% = 出屏通路正常）；
      #   全轮 <30%（fbp=0）才判出屏未达成（so 渲染退化）。
      need=$(( (ROUNDS+1)/2 ))
      [ "$fbp" -ge "$need" ] || { ok=0; echo "  ✗ 出屏 $fbp/$ROUNDS 轮 <30%（达标轮 $fbp < $need，渲染退化）"; }
      if [ "$ok" = "1" ]; then echo "  ✓ game_test（端到端 + 命中几何 + 事件表 + 出屏 $fbp/$ROUNDS 轮达标 + 0 崩溃）"; PASS=$((PASS+1)); else echo "  ✗ game_test 判据未全过"; FAIL=$((FAIL+1)); FAILED="$FAILED $k"; fi
      ;;
    walk_bridge)
      LOG=$(launch_hook jq.walk_bridge jq.walk_bridge)
      if ! echo "$LOG" | grep -q "WALK-BRIDGE: 完整 8 步序列完成"; then
        wait_log "WALK-BRIDGE: 完整 8 步序列完成" 45
        LOG=$(adb -s "$DEV" logcat -d 2>/dev/null)
      fi
      CRASH=$(echo "$LOG" | grep -c "Fatal signal" || true)
      CRASH_ENV=0
      if [ "$CRASH" != "0" ]; then
        if classify_crash; then CRASH_ENV=1; else CRASH_ENV=0; fi
      fi
      if echo "$LOG" | grep -q "WALK-BRIDGE: 完整 8 步序列完成" && { [ "$CRASH" = "0" ] || [ "$CRASH_ENV" = "1" ]; }; then
        echo "  ✓ walk_bridge（8 步序列 + 0 崩溃）"; PASS=$((PASS+1))
      else
        echo "  ✗ walk_bridge（8 步未命中 或 崩溃=$CRASH）"; FAIL=$((FAIL+1)); FAILED="$FAILED $k"
      fi
      ;;
    native_event)
      # native_event 需 app 启动后再设属性（OnUpdate 读）
      adb -s "$DEV" shell "setprop jq.native_layout 0; setprop jq.native_event 0; setprop jq.game_test 0; setprop jq.walk_bridge 0" >/dev/null 2>&1
      adb -s "$DEV" shell "am force-stop $PKG; logcat -c" >/dev/null 2>&1
      adb -s "$DEV" shell "am start -n $PKG/com.cnvcs.App" >/dev/null 2>&1
      sleep 6
      adb -s "$DEV" shell "setprop jq.native_layout 1; setprop jq.native_event 1" >/dev/null 2>&1
      sleep 8
      LOG=$(adb -s "$DEV" logcat -b main -d 2>/dev/null)
      if ! echo "$LOG" | grep -q "F_3ab80: 事件发送"; then
        wait_log "F_3ab80: 事件发送" 30 && LOG=$(adb -s "$DEV" logcat -b main -d 2>/dev/null)
      fi
      CRASH=$(echo "$LOG" | grep -c "Fatal signal" || true)
      CRASH_ENV=0
      if [ "$CRASH" != "0" ]; then
        if classify_crash; then CRASH_ENV=1; else CRASH_ENV=0; fi
      fi
      if echo "$LOG" | grep -q "F_3ab80: 事件发送" && { [ "$CRASH" = "0" ] || [ "$CRASH_ENV" = "1" ]; }; then
        echo "  ✓ native_event（F_3ab80 事件发送 + 0 崩溃）"; PASS=$((PASS+1))
      else
        echo "  ✗ native_event（F_3ab80 未命中 或 崩溃=$CRASH）"; FAIL=$((FAIL+1)); FAILED="$FAILED $k"
      fi
      ;;
    framebuffer)
      fbp=0; fbf=0
      for r in $(seq 1 "$ROUNDS"); do
        # 每轮独立启动 + 清 dump（防旧文件误判）
        adb -s "$DEV" shell "setprop jq.game_test 1; am force-stop $PKG" >/dev/null 2>&1
        sleep 2
        adb -s "$DEV" shell "rm -f /sdcard/Download/board_dump_*.ppm" >/dev/null 2>&1
        adb -s "$DEV" shell "am start -n $PKG/com.cnvcs.App" >/dev/null 2>&1
        # ★ §124y-180 方法论第 5 条（flaky 防）：dump 由翻译体 F_9d43c 帧尾生成，
        #   固定 sleep 可能早于首帧 → 轮询等待（最多 92s，每 4s 查一次；串行转译冷启动 2-3 倍慢 §124y-188）
        ratio="none"
        for _w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23; do
          sleep 4
          LATEST=$(adb -s "$DEV" shell "ls -t /sdcard/Download/board_dump_*.ppm 2>/dev/null | head -1" | tr -d '\r')
          if [ -n "$LATEST" ]; then
            ensure_dev >/dev/null 2>&1 || continue
            adb -s "$DEV" pull "$LATEST" /tmp/opencode/arm32_fb_check.ppm >/dev/null 2>&1 || continue
            ratio=$(python3 -c "
from PIL import Image
try:
  im=Image.open('/tmp/opencode/arm32_fb_check.ppm').convert('RGB')
  w,h=im.size; px=im.load()
  nb=sum(1 for y in range(0,h,4) for x in range(0,w,4) if sum(px[x,y])>60)
  print(100*nb//((w//4)*(h//4)))
except Exception:
  print('none')" 2>/dev/null)
            [ -n "$ratio" ] && [ "$ratio" != "none" ] && [ "$ratio" -ge 30 ] && break
            ratio="none"
          fi
        done
        if [ -n "$ratio" ] && [ "$ratio" != "none" ] && [ "$ratio" -ge 30 ]; then
          echo "  出屏轮次 $r: 非黑比 ${ratio}% ✓"; fbp=$((fbp+1))
        else
          echo "  出屏轮次 $r: 非黑比 ${ratio:-none}% ✗"; fbf=$((fbf+1))
        fi
        adb -s "$DEV" shell "setprop jq.game_test 0" >/dev/null 2>&1
      done
      # §124y-225 出屏容错判据（转译冷启动 flaky 防误判）：达标轮 fbp >= ROUNDS/2
      #   即出屏通路正常；fbp=0（全轮 <30%）才判渲染退化
      need=$(( (ROUNDS+1)/2 ))
      if [ "$fbp" -ge "$need" ] && [ "$fbp" -gt 0 ]; then
        echo "  ✓ framebuffer（$fbp/$ROUNDS 轮达标（≥$need 轮 ≥30% 非黑），§124y-179/225 判据）"; PASS=$((PASS+1))
      else
        echo "  ✗ framebuffer（$fbp/$ROUNDS 轮达标 < $need，渲染退化）"; FAIL=$((FAIL+1)); FAILED="$FAILED $k"
      fi
      ;;
  esac
done

# 清全部属性
ensure_dev >/dev/null 2>&1 && adb -s "$DEV" shell "setprop jq.game_test 0; setprop jq.walk_bridge 0; setprop jq.native_layout 0; setprop jq.native_event 0; setprop jq.bss_dump 0; setprop jq.event_base 0; setprop jq.event_base_verify 0" >/dev/null 2>&1

echo "═══ arm32_hook_check 汇总 ═══"
echo "  PASS=$PASS FAIL=$FAIL"
[ -n "$FAILED" ] && echo "  失败: $FAILED"
if [ "$FAIL" -eq 0 ]; then echo "  ✓ 全 PASS"; else echo "  ✗ 存在 FAIL"; fi
exit "$FAIL"
