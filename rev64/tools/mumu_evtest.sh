#!/bin/bash
# mumu_evtest.sh — 事件测试一条龙（FLAG/MINE 验证，setprop 触发）
# 固化：flagtest/minetest 验证不再手写 setprop+重启+抓日志。
# 用法:
#   bash rev64/tools/mumu_evtest.sh flag   # FLAG（蓝 rank2@53 吃红军旗@58）
#   bash rev64/tools/mumu_evtest.sh mine   # MINE（蓝工兵@49 挖红雷@54，红有子不结束对局）
#   bash rev64/tools/mumu_evtest.sh draw   # 和棋（OnUpdate jq_on_draw winner=2）
#   bash rev64/tools/mumu_evtest.sh resign # 认输（OnUpdate on_event resign 对方胜）
#   bash rev64/tools/mumu_evtest.sh stalemate # 困毙（OnUpdate jq_on_stalemate 无子可动方负）
#   bash rev64/tools/mumu_evtest.sh --dev IP:PORT flag
# 原理: setprop jq.flagtest/jq.minetest 1 → 重启 → OnCreate 构造近旗/挖雷状态 + 自动走一步
#       （触摸 MuMu flaky 不达 native，走 OnCreate 自动触发路径；事件链路/规则引擎/音效真实）
# 判据: 日志出现 "flagtest：... ev=3 Junqi.Flag" / "minetest：... ev=5 Junqi.Move" + 进程存活
set -u
DEV=${DEV:-10.0.182.146:5555}
KIND=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --dev) DEV="$2"; shift 2;;
    --*) echo "未知参数: $1"; exit 2;;
    *) KIND="$1"; shift;;
  esac
done
[ -z "$KIND" ] && { echo "用法: $0 [--dev IP:PORT] <flag|mine|restart|draw|resign>"; exit 2; }

log() { echo "[evtest] $*"; }

# ★ 对抗审计 P2-B13（时序稳健性）：原各测试用固定 sleep 8/10/12 等 app 启动/GLThread
#   attach 等待——MuMu/redroid 启动慢时 property 未被消费 → 假 FAIL（实测 draw/restart
#   首跑失败重跑即过的 flaky 根因）。改为"轮询到判据出现或超时"：
#   wait_log <pattern> <max_wait_s>：每 1s 扫一次 logcat，命中即返回 0；超时返回 1。
# ★ AUD-1000 B07-003（P1 假绿三型-判据兜底稀释）：默认窗口 20s→40s（MuMu 冷启
#   GLThread attach 慢，20-25s 才出判据日志的"真慢"被 20s 超时误判 flaky 放过）。
wait_log() {
  local pat=$1 maxw=${2:-40} i=0
  while [ "$i" -lt "$maxw" ]; do
    if adb -s $DEV logcat -d 2>/dev/null | grep -qiE "$pat"; then return 0; fi
    sleep 1; i=$((i+1))
  done
  return 1
}

# ★ AUD-1000 B07-003：判据行新鲜度校验（末条判据行 PID 须属当前进程，防 logcat -c
#   无权限静默失败后历史残留日志误判 PASS，对齐 verify_asset_chain P1-A6 模式）。
#   logcat 行 "MM-DD HH:MM:SS.mmm PID TID L tag: msg" 第 4 字段 PID；PID 列缺失
#   （旧设备）或 pidof 无输出 → 退化为"行存在即通过"（不引入假红）。
fresh_log() {
  local pat=$1
  local pid line lp
  pid=$(adb -s $DEV shell pidof com.cnvcs.junqirs 2>/dev/null | tr -d '\r' | tr ',' ' ' | head -1 | tr -d ' ')
  line=$(adb -s $DEV logcat -d 2>/dev/null | grep -iE "$pat" | tail -1)
  [ -z "$line" ] && return 1
  lp=$(echo "$line" | awk '{print $4}')
  if [[ "$lp" =~ ^[0-9]+$ ]]; then
    case " $pid " in *" $lp "*) return 0;; esac
    return 1
  fi
  return 0
}

# 等待 so 进程起来（app 启动判据）
wait_proc() {
  local maxw=${1:-20} i=0
  while [ "$i" -lt "$maxw" ]; do
    if [ -n "$(adb -s $DEV shell pidof com.cnvcs.junqirs 2>/dev/null | tr -d '\r')" ]; then return 0; fi
    sleep 1; i=$((i+1))
  done
  return 1
}

case "$KIND" in
  flag)
    PROP=jq.flagtest; PAT="flagtest：.*ev=3 Junqi.Flag"; SFX="sfx：播放 flag";;
  mine)
    PROP=jq.minetest; PAT="minetest：.*ev=5 Junqi.Move"; SFX="sfx：播放 move";;
  restart)
    # 游戏内重开：先 flagtest 走 FLAG（stage=OVER）→ setprop restartgame → OnUpdate 重开（stage=PLAY）
    log "=== restart 事件测试（flagtest→OVER→restartgame→重开）==="
    adb -s $DEV shell am force-stop com.cnvcs.junqirs >/dev/null 2>&1; sleep 3
    adb -s $DEV logcat -c
    adb -s $DEV shell setprop jq.flagtest 1
    adb -s $DEV shell am start -n com.cnvcs.junqirs/com.cnvcs.App >/dev/null 2>&1
    wait_proc 20 || { log "★ 进程未启动（20s 超时）"; exit 1; }
    sleep 3   # 让首帧/OnUpdate 链起稳
    adb -s $DEV shell setprop jq.restartgame 1
    wait_log "游戏内重开：.*stage=PLAY" 20 || true   /* 轮询到判据（超时由下方 if 判 FAIL） */
    echo "=== restart 事件日志 ==="
    adb -s $DEV logcat -d 2>/dev/null | grep "junqi_rs" | grep -iE "flagtest|游戏内重开" | tail -3
    if adb -s $DEV logcat -d 2>/dev/null | grep -qiE "游戏内重开：.*stage=PLAY"; then
      log "✓ 游戏内重开出现（stage=PLAY 蓝先手）"
    else
      log "★ 游戏内重开未出现"; exit 1
    fi
    if adb -s $DEV shell pidof com.cnvcs.junqirs >/dev/null 2>&1; then
      log "✓ 进程存活 $(adb -s $DEV shell pidof com.cnvcs.junqirs)"
    else
      log "★ 进程死了"; exit 1
    fi
    log "✓ restart 事件测试完成"
    exit 0;;
  btconn)
    # 蓝牙连接自动开局：setprop jq.btconn 1 → OnCreate spawn 后台线程模拟 BtSpp 回调
    # btState("host_connected") → runOnUiThread → App$BtStart → OnEvent menu 1 → 两人对局
    log "=== btconn 连接自动开局测试（setprop jq.btconn 1 → 模拟 host_connected）==="
    adb -s $DEV shell am force-stop com.cnvcs.junqirs >/dev/null 2>&1; sleep 3
    adb -s $DEV logcat -c
    adb -s $DEV shell setprop jq.btconn 1
    adb -s $DEV shell am start -n com.cnvcs.junqirs/com.cnvcs.App >/dev/null 2>&1
    wait_proc 20 || { log "★ 进程未启动（20s 超时）"; exit 1; }
    wait_log "OnEvent menu 开局 mode=1" 25 || true   /* 轮询到判据 */
    sleep 3   # 让首帧/OnUpdate 链起稳
    echo "=== btconn 事件日志 ==="
    adb -s $DEV logcat -d 2>/dev/null | grep -iE "junqi_rs|junqi_bt" | grep -iE "host_connected|OnEvent menu 开局|ui_screen=1" | tail -4
    # ★ AUD-1000-ALL A12-D7-F12-009（P1 已修）：btconn/restart 判据 grep 加 PID 新鲜度
    #   （fresh_log，B07-003 7 项范式）——旧版裸 grep logcat -d 全量（logcat -c 非 root
    #   时历史残留 → 旧进程判据行误判 PASS）。修：判据行须含当前 PID。
    BT_PID=$(adb -s $DEV shell pidof com.cnvcs.junqirs | tr -d '\r')
    if adb -s $DEV logcat -d 2>/dev/null | grep -E "OnEvent menu 开局 mode=1" | grep -q "${BT_PID:-__pid__}"; then
      log "✓ 连接后两人对局自动开局（mode=1 蓝先手）"
    else
      log "★ 对局自动开局未出现"; exit 1
    fi
    # ★ B06-085（P2 已修）：FATAL 只收 AndroidRuntime:E 行（旧 grep -qiE 'FATAL|ClassNotFound'
    #   对全量 logcat 误判——历史/其他 app 的 FATAL 行 + JNI 陷阱固有 ClassNotFound 噪声
    #   （AGENTS.md 记录 native 线程 FindClass ClassNotFound 已知陷阱）误判 btconn 崩溃。
    #   限定 tag=AndroidRuntime（崩溃必出 AndroidRuntime FATAL 行）+ ClassNotFound 限定 junqi 进程。 */
    if adb -s $DEV logcat -d -s AndroidRuntime:E 2>/dev/null | grep -q "FATAL EXCEPTION" \
       || adb -s $DEV logcat -d -s junqi_rs:E 2>/dev/null | grep -q "ClassNotFound"; then
      log "★ 崩溃"; exit 1
    fi
    if adb -s $DEV shell pidof com.cnvcs.junqirs >/dev/null 2>&1; then
      log "✓ 进程存活 $(adb -s $DEV shell pidof com.cnvcs.junqirs)"
    else
      log "★ 进程死了"; exit 1
    fi
    log "✓ btconn 事件测试完成"
    exit 0;;
  draw)
    # 和棋：重启确保 OnUpdate 活跃（MuMu GLThread attach flaky 停帧 → property 不消费）
    # → setprop jq.drawtest 1 → jq_on_draw（winner=2）
    log "=== draw 事件测试（重启 → setprop jq.drawtest 1 → jq_on_draw winner=2）==="
    adb -s $DEV shell am force-stop com.cnvcs.junqirs >/dev/null 2>&1
    sleep 2
    adb -s $DEV shell am start -n com.cnvcs.junqirs/com.cnvcs.App >/dev/null 2>&1
    wait_proc 20 || { log "★ 进程未启动（20s 超时）"; exit 1; }
    sleep 3   # 让首帧/OnUpdate 链起稳
    adb -s $DEV logcat -c
    adb -s $DEV shell setprop jq.drawtest 1
    wait_log "OnUpdate 和棋：drawtest" 20 || true   /* 轮询到判据（超时由下方 if 判 FAIL） */
    echo "=== draw 事件日志 ==="
    adb -s $DEV logcat -d 2>/dev/null | grep "junqi_rs" | grep -iE "和棋|sfx：播放 draw" | tail -3
    if adb -s $DEV logcat -d 2>/dev/null | grep -qiE "OnUpdate 和棋：drawtest"; then
      log "✓ draw 事件出现（winner=2 和棋）"
    else
      log "★ draw 事件未出现"; exit 1
    fi
    if adb -s $DEV logcat -d 2>/dev/null | grep -qiE "sfx：播放 draw"; then
      log "✓ draw 音效出现"
    else
      log "★ draw 音效未出现"; exit 1
    fi
    if adb -s $DEV shell pidof com.cnvcs.junqirs >/dev/null 2>&1; then
      log "✓ 进程存活 $(adb -s $DEV shell pidof com.cnvcs.junqirs)"
    else
      log "★ 进程死了"; exit 1
    fi
    log "✓ draw 事件测试完成"
    exit 0;;
  resign)
    # 认输：重启确保 OnUpdate 活跃（同 draw 理由）→ setprop jq.resigntest 1 → on_event resign（对方胜）
    log "=== resign 事件测试（重启 → setprop jq.resigntest 1 → on_event resign 对方胜）==="
    adb -s $DEV shell am force-stop com.cnvcs.junqirs >/dev/null 2>&1
    sleep 2
    adb -s $DEV shell am start -n com.cnvcs.junqirs/com.cnvcs.App >/dev/null 2>&1
    wait_proc 20 || { log "★ 进程未启动（20s 超时）"; exit 1; }
    sleep 3   # 让首帧/OnUpdate 链起稳
    adb -s $DEV logcat -c
    adb -s $DEV shell setprop jq.resigntest 1
    wait_log "OnUpdate 认输：resigntest" 20 || true   /* 轮询到判据（超时由下方 if 判 FAIL） */
    echo "=== resign 事件日志 ==="
    adb -s $DEV logcat -d 2>/dev/null | grep "junqi_rs" | grep -iE "OnEvent resign|sfx：播放 lose" | tail -3
    if adb -s $DEV logcat -d 2>/dev/null | grep -qiE "OnUpdate 认输：resigntest"; then
      log "✓ resign 事件出现（对方胜）"
    else
      log "★ resign 事件未出现"; exit 1
    fi
    if adb -s $DEV logcat -d 2>/dev/null | grep -qiE "sfx：播放 lose"; then
      log "✓ resign 音效出现"
    else
      log "★ resign 音效未出现"; exit 1
    fi
    if adb -s $DEV shell pidof com.cnvcs.junqirs >/dev/null 2>&1; then
      log "✓ 进程存活 $(adb -s $DEV shell pidof com.cnvcs.junqirs)"
    else
      log "★ 进程死了"; exit 1
    fi
    log "✓ resign 事件测试完成"
    exit 0;;
  stalemate)
    # 困毙：重启确保 OnUpdate 活跃（GLThread surface attach flaky 停帧 → property 不消费）
    # → setprop jq.stalematetest 1 → jq_on_stalemate（无子可动方负）
    log "=== stalemate 事件测试（重启 → setprop jq.stalematetest 1 → jq_on_stalemate winner=1-turn）==="
    adb -s $DEV shell am force-stop com.cnvcs.junqirs >/dev/null 2>&1
    sleep 2
    adb -s $DEV shell am start -n com.cnvcs.junqirs/com.cnvcs.App >/dev/null 2>&1
    wait_proc 20 || { log "★ 进程未启动（20s 超时）"; exit 1; }
    sleep 3   # 让首帧/OnUpdate 链起稳
    adb -s $DEV logcat -c
    adb -s $DEV shell setprop jq.stalematetest 1
    wait_log "OnUpdate 困毙：stalematetest" 20 || true   /* 轮询到判据 */
    echo "=== stalemate 事件日志 ==="
    adb -s $DEV logcat -d 2>/dev/null | grep "junqi_rs" | grep -iE "困毙|sfx：播放 lose" | tail -3
    if adb -s $DEV logcat -d 2>/dev/null | grep -qiE "OnUpdate 困毙：stalematetest"; then
      log "✓ stalemate 事件出现（无子可动方负）"
    else
      log "★ stalemate 事件未出现"; exit 1
    fi
    if adb -s $DEV logcat -d 2>/dev/null | grep -qiE "sfx：播放 lose"; then
      log "✓ stalemate 音效出现"
    else
      log "★ stalemate 音效未出现"; exit 1
    fi
    if adb -s $DEV shell pidof com.cnvcs.junqirs >/dev/null 2>&1; then
      log "✓ 进程存活 $(adb -s $DEV shell pidof com.cnvcs.junqirs)"
    else
      log "★ 进程死了"; exit 1
    fi
    log "✓ stalemate 事件测试完成"
    exit 0;;
  close)
    # UI 关闭音效：重启确保 OnUpdate 活跃（GLThread surface attach flaky 会停帧 → OnUpdate 不跑
    # property 不消费）→ setprop jq.closetest 1 → sfx close
    log "=== close 音效测试（重启 → setprop jq.closetest 1 → sfx close）==="
    adb -s $DEV shell am force-stop com.cnvcs.junqirs >/dev/null 2>&1
    sleep 2
    adb -s $DEV shell am start -n com.cnvcs.junqirs/com.cnvcs.App >/dev/null 2>&1
    wait_proc 20 || { log "★ 进程未启动（20s 超时）"; exit 1; }
    sleep 3   # 让首帧/OnUpdate 链起稳
    adb -s $DEV logcat -c
    adb -s $DEV shell setprop jq.closetest 1
    wait_log "sfx：播放 close" 20 || true   /* 轮询到判据 */
    if adb -s $DEV logcat -d 2>/dev/null | grep -qiE "sfx：播放 close"; then
      log "✓ close 音效出现"
    else
      log "★ close 音效未出现"; exit 1
    fi
    if adb -s $DEV shell pidof com.cnvcs.junqirs >/dev/null 2>&1; then
      log "✓ 进程存活 $(adb -s $DEV shell pidof com.cnvcs.junqirs)"
    else
      log "★ 进程死了"; exit 1
    fi
    log "✓ close 音效测试完成"
    exit 0;;
  pop)
    # UI 弹窗音效：重启确保 OnUpdate 活跃（同 close 理由）→ setprop jq.poptest 1 → sfx pop
    log "=== pop 音效测试（重启 → setprop jq.poptest 1 → sfx pop）==="
    adb -s $DEV shell am force-stop com.cnvcs.junqirs >/dev/null 2>&1
    sleep 2
    adb -s $DEV shell am start -n com.cnvcs.junqirs/com.cnvcs.App >/dev/null 2>&1
    wait_proc 20 || { log "★ 进程未启动（20s 超时）"; exit 1; }
    sleep 3   # 让首帧/OnUpdate 链起稳
    adb -s $DEV logcat -c
    adb -s $DEV shell setprop jq.poptest 1
    wait_log "sfx：播放 pop" 20 || true   /* 轮询到判据 */
    if adb -s $DEV logcat -d 2>/dev/null | grep -qiE "sfx：播放 pop"; then
      log "✓ pop 音效出现"
    else
      log "★ pop 音效未出现"; exit 1
    fi
    if adb -s $DEV shell pidof com.cnvcs.junqirs >/dev/null 2>&1; then
      log "✓ 进程存活 $(adb -s $DEV shell pidof com.cnvcs.junqirs)"
    else
      log "★ 进程死了"; exit 1
    fi
    log "✓ pop 音效测试完成"
    exit 0;;
  *) log "未知事件: $KIND（flag|mine|restart|draw|resign|stalemate|close|pop）"; exit 2;;
esac

# ★ AUD-1000 B07-004 时序修正 + 本轮回归修复：flag/mine/restart 钩子在 **OnCreate**
#   读 property（jni_bridge OnCreate flagtest/minetest 段），时序须 **setprop 先行
#   （force-stop 清进程后）→ logcat -c → am start**——app 起来时 OnCreate 读到 1。
#   旧 B07-004 误改成 "am start→setprop"（与 draw/resign 的 OnUpdate 轮询钩子一致），
#   但 OnCreate 钩子在 app 起来时已读完 prop，setprop 在后 → 钩子 0 命中（本轮实测
#   flag/mine FAIL 根因）。修回 setprop 先行；draw/resign/stalemate/close/pop 系
#   OnUpdate 每帧轮询 prop，保持 am start→setprop 现状（本分支不触达）。
case "$PROP" in
  jq.flagtest|jq.minetest|jq.native_layout|jq.walk_bridge|jq.game_test)
    log "=== $KIND 事件测试（setprop $PROP 1 → logcat -c → am start，OnCreate 钩子）==="
    adb -s $DEV shell am force-stop com.cnvcs.junqirs >/dev/null 2>&1; sleep 3
    adb -s $DEV shell setprop $PROP 0; sleep 1
    adb -s $DEV shell setprop $PROP 1
    adb -s $DEV logcat -c
    adb -s $DEV shell am start -n com.cnvcs.junqirs/com.cnvcs.App >/dev/null 2>&1
    wait_proc 20 || { log "★ 进程未启动（20s 超时）"; exit 1; }
    ;;
  *)
    log "=== $KIND 事件测试（am start → logcat -c → setprop $PROP 1，OnUpdate 轮询钩子）==="
    adb -s $DEV shell am force-stop com.cnvcs.junqirs >/dev/null 2>&1; sleep 3
    adb -s $DEV shell am start -n com.cnvcs.junqirs/com.cnvcs.App >/dev/null 2>&1
    wait_proc 20 || { log "★ 进程未启动（20s 超时）"; exit 1; }
    sleep 2   # 让首帧/OnUpdate 链起稳
    adb -s $DEV logcat -c
    adb -s $DEV shell setprop $PROP 1
    ;;
esac
wait_log "$PAT" 40 || true   /* ★ P2-B13/B07-003：轮询到判据（B07-003 窗口 40s；超时由下方 if 判 FAIL） */
wait_log "$SFX" 40 || true   /* 音效判据单独轮询 */

echo "=== $KIND 事件日志 ==="
adb -s $DEV logcat -d 2>/dev/null | grep "junqi_rs" | grep -iE "$PAT|$SFX" | tail -4

# ★ AUD-1000 B07-003：判据二次 grep 改 fresh_log（PID 新鲜度校验）——logcat -c
#   非 root EACCES 静默失败时历史进程残留日志不再误判 PASS；PID 列缺失设备退化
#   为"行存在即通过"（不引入假红）。
if fresh_log "$PAT"; then
  log "✓ $KIND 事件出现（本次进程日志）"
else
  log "★ $KIND 事件未出现（或仅历史残留日志）"; exit 1
fi
if fresh_log "$SFX"; then
  log "✓ $KIND 音效出现（本次进程日志）"
else
  log "★ $KIND 音效未出现"; exit 1
fi
if adb -s $DEV shell pidof com.cnvcs.junqirs >/dev/null 2>&1; then
  log "✓ 进程存活 $(adb -s $DEV shell pidof com.cnvcs.junqirs)"
else
  log "★ 进程死了"; exit 1
fi
log "✓ $KIND 事件测试完成"
exit 0
