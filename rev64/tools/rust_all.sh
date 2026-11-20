#!/usr/bin/env bash
# rust_all.sh — Rust 正向（src_rs_rust）一键全量回归（防判据散落漏挂 = CODE_STYLE §9 静默降级假绿）
#
# 背景：AGENTS.md 新增 fuzz_all.sh 串起 C/逆向渗透判据，但 Rust 侧判据
# （cargo test / clippy / android build / rust_anything 行为对齐）散落多处，
# 改了 Rust 代码后没有一键全量入口。本脚本补齐，与 fuzz_all.sh 并列。
#
# 用法：bash rev64/tools/rust_all.sh [--quick] [--device]
#   --quick   跳过耗时的 154 case 行为对齐（只跑编译门禁）
#   --device  加跑设备侧检查（需 adb，设备可达时查 installed so 哈希 + 无 Fatal）
# rc：0=全 PASS / 1=有 FAIL / 3=PENDING（工具缺失）
set -uo pipefail

WS=/disk/workspaces/jq
RUST=$WS/rev64/forward/src_rs_rust
export PATH="$HOME/.cargo/bin:$PATH"
QUICK=0; DEVICE=0
for a in "$@"; do
  case "$a" in
    --quick) QUICK=1;;
    --device) DEVICE=1;;
    *) echo "未知参数: $a"; exit 2;;
  esac
done

fail=0
log(){ echo "[rust_all] $*"; }

log "=== [1/4] cargo test --release（95 测试）==="
T=$(cd "$RUST" && cargo test --release 2>&1)
if echo "$T" | grep -qE 'test result: ok\. [0-9]+ passed'; then
  NP=$(echo "$T" | grep -oE '[0-9]+ passed' | head -1)
  log "PASS cargo test ($NP)"
else
  echo "$T" | tail -10; log "FAIL cargo test"; fail=1
fi

log "=== [2/4] cargo clippy -D warnings（零警告门禁）==="
(cd "$RUST" && cargo clippy --all-targets --release -- -D warnings >/dev/null 2>&1)
if [ $? -eq 0 ]; then log "PASS clippy -D warnings"; else log "FAIL clippy"; fail=1; fi

log "=== [3/4] android target 编译（0 错 0 警 / 14 符号）==="
B=$(cd "$RUST" && cargo build --target aarch64-linux-android --release 2>&1)
E=$(echo "$B" | grep -c 'error:'); W=$(echo "$B" | grep -c '^warning:')
SO=$RUST/target/aarch64-linux-android/release/libjunqi_rs.so
S=$(nm -D "$SO" 2>/dev/null | grep -cE 'Java_com_qwsaeda_App_')
if [ "$E" = 0 ] && [ "$W" = 0 ] && [ "$S" = 14 ]; then
  log "PASS android 0错/0警/14符号"
else
  log "FAIL android errors=$E warnings=$W symbols=$S"; fail=1
fi

if [ "$QUICK" = 0 ]; then
  log "=== [4/4] rust_anything_check（原版32位行为 154 case 对齐 + 负向分叉可检出）==="
  A=$(python3 "$WS/rev64/tools/rust_anything_check.py" 2>&1); RC=$?
  if [ $RC -eq 0 ] && echo "$A" | grep -q '判定: PASS'; then
    log "PASS $(echo "$A" | tail -1)"
  elif [ $RC -eq 3 ]; then
    log "PENDING rust-anything 未装（非 FAIL）"
  else
    echo "$A" | tail -8; log "FAIL rust_anything_check rc=$RC"; fail=1
  fi
else
  log "=== [4/4] 跳过（--quick）==="
fi

if [ "$DEVICE" = 1 ]; then
  log "=== [5] 设备侧：installed so 哈希 + 无 Fatal + 能玩功能判据 ==="
  DEV=${DEV:-10.0.182.152:5555}
  # adb 瞬断自愈：MuMu/houdini 下 adb 偶发 offline（脚本跑一半掉线→属性假绿根源）
  adb_ensure(){ local o=$(adb -s "$DEV" shell "echo ok" 2>/dev/null | tr -d '\r');
    if [ "$o" != "ok" ]; then adb connect "$DEV" >/dev/null 2>&1; sleep 1; fi; }
  adb_ensure
  P=$(adb -s "$DEV" shell "pm path com.qwsaeda.junqirs" 2>/dev/null | sed 's/package://' | tr -d '\r' | head -1)
  if [ -n "$P" ]; then
    DH=$(adb -s "$DEV" exec-out "unzip -p $P lib/arm64-v8a/libjunqi.so" 2>/dev/null | md5sum | cut -d' ' -f1)
    LH=$(md5sum "$SO" | cut -d' ' -f1)
    [ "$DH" = "$LH" ] && log "PASS 设备 so 对账 ($DH)" || { log "FAIL 设备 so 不一致 设备=$DH 本地=$LH"; fail=1; }
    # 轮询等属性（CODE_STYLE §11：固定 sleep→wait_log，抗 houdini 时序抖动）
    # ★ 关键：offline 时 getprop 返回 "adb: device offline"，绝不能当属性值匹配（假绿根源）
    wait_prop(){ local key=$1 want=$2 n=0; while [ $n -lt 15 ]; do
      adb_ensure
      local v=$(adb -s "$DEV" shell "getprop $key" 2>/dev/null | tr -d '\r')
      case "$v" in *offline*|*error*|"") ;; *) [ "$v" = "$want" ] && return 0;; esac
      sleep 1; n=$((n+1)); done; return 1; }
    # 每个 adb 操作前自愈
    adb_ensure
    # 干净重启（清残留钩子 + 文件探针）
    adb_ensure
    adb -s "$DEV" shell "rm -f /data/local/tmp/jq_hook_*; am force-stop com.qwsaeda.junqirs" >/dev/null 2>&1
    sleep 2  # houdini JIT 竞态：force-stop 后立即 start 会 non-executable memory SIGSEGV（实测）
    adb -s "$DEV" shell "am start -n com.qwsaeda.junqirs/com.qwsaeda.App" >/dev/null 2>&1
    wait_prop jq.wgpu_thread rendering && log "PASS 出屏 rendering" || { log "FAIL 出屏未达 rendering"; fail=1; }
    # 走子：蓝 6→11（tap 两下）
    adb_ensure
    adb -s "$DEV" shell "setprop jq.move NONE; input tap 274 1383; sleep 1; input tap 274 1312" >/dev/null 2>&1
    wait_prop jq.move Move && log "PASS 走子 Move" || { log "FAIL 走子未触发 Move"; fail=1; }
    # 悔棋：☰ 菜单 undo（tap 620 750）
    adb_ensure
    adb -s "$DEV" shell "setprop jq.game_icon NONE; input tap 620 750" >/dev/null 2>&1
    wait_prop jq.game_icon undo_ok && log "PASS 悔棋 undo_ok" || { log "FAIL 悔棋未触发 undo_ok"; fail=1; }
    # ⑤ 重进不崩：force-stop 后重新 am start，等 pid 存活 + 出屏恢复
    #   （放在 Fatal 检查之前，重进过程的崩溃也能被下面的 Fatal 计数捕获）
    adb_ensure
    adb -s "$DEV" shell "am force-stop com.qwsaeda.junqirs" >/dev/null 2>&1
    sleep 2  # 同上：避免 houdini 快速重启竞态
    adb -s "$DEV" shell "am start -n com.qwsaeda.junqirs/com.qwsaeda.App" >/dev/null 2>&1
    wait_prop jq.wgpu_thread rendering && log "PASS 重进出屏恢复" || { log "FAIL 重进未恢复出屏"; fail=1; }
    adb_ensure
    RP=""; for pn in 1 2 3 4 5; do adb_ensure; RP=$(adb -s "$DEV" shell "pidof com.qwsaeda.junqirs" 2>/dev/null | tr -d '\r' | head -1); [ -n "$RP" ] && break; sleep 1; done
    [ -n "$RP" ] && log "PASS 重进 pid 存活 ($RP)" || { log "FAIL 重进 pid 不存在"; fail=1; }
    F=$(adb -s "$DEV" logcat -d 2>&1 | grep -c 'Fatal signal\|Surface has already been released\|No pending exception')
    [ "$F" = 0 ] && log "PASS 无 Fatal" || { log "FAIL Fatal=$F"; fail=1; }
  else
    log "PENDING 设备不可达"
  fi
fi

echo ""
if [ "$fail" = 0 ]; then
  log "判定: PASS（Rust 全量判据无遗漏）"
  exit 0
else
  log "判定: FAIL（有判据未过）"
  exit 1
fi
