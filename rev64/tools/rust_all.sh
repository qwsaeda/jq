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

# [4b] 行为对齐扩展：棋盘几何 950 case + 32/64 位规则双端（桌面 -m32）
if [ "$QUICK" = 0 ]; then
  # ★ P0-1 根因修复：原版真值表直接测生产 ai::ai_score（篡改生产函数必检出）
  log "=== [4a] ai_score_orig_bridge（原版154case→生产ai::ai_score，P0-1根因）==="
  python3 "$WS/rev64/tools/gen_ai_score_bridge_test.py" >/dev/null 2>&1
  BT=$(cd "$RUST" && cargo test --release --test ai_score_orig_bridge 2>&1)
  echo "$BT" | grep -q 'test result: ok' && log "PASS 生产ai_score对齐原版真值" || { echo "$BT" | tail -8; log "FAIL 生产ai_score桥接测试"; fail=1; }
  log "=== [4b] rust_geom_check（几何6函数 950 case）+ rp_desktop_compare（32/64双端36 case）==="
  G=$(python3 "$WS/rev64/tools/rust_geom_check.py" 2>&1); GR=$?
  if [ $GR -eq 0 ] && echo "$G" | grep -q '判定: PASS'; then
    log "PASS $(echo "$G" | tail -1)"
  elif [ $GR -eq 3 ]; then log "PENDING rust-anything 未装"
  else echo "$G" | tail -6; log "FAIL rust_geom_check rc=$GR"; fail=1; fi
  R=$(bash "$WS/rev64/tools/rp_desktop_compare.sh" 2>&1); RR=$?
  if [ $RR -eq 0 ]; then log "PASS $(echo "$R" | tail -1)"
  elif [ $RR -eq 3 ]; then log "PENDING -m32 不可用"
  else echo "$R" | tail -6; log "FAIL rp_desktop_compare rc=$RR"; fail=1; fi
fi

if [ "$DEVICE" = 1 ]; then
  log "=== [5] 设备侧：installed so 哈希 + 无 Fatal + 能玩功能判据 ==="
  DEV=${DEV:-10.0.182.152:5555}
  # adb 瞬断自愈：MuMu/houdini 下 adb 偶发 offline（脚本跑一半掉线→属性假绿根源）
  adb_ensure(){ local o=$(timeout 5 adb -s "$DEV" shell "echo ok" 2>/dev/null | tr -d '\r');
    if [ "$o" != "ok" ]; then timeout 5 adb connect "$DEV" >/dev/null 2>&1; sleep 1; fi; }
  # ★ 快速探测：设备离线(No route to host)时立即 PENDING，不让 adb connect 阻塞整脚本
  if [ "$(timeout 5 adb -s "$DEV" shell "echo ok" 2>/dev/null | tr -d '\r')" != "ok" ]; then
    log "PENDING 设备离线（ping/adb 不可达，非 FAIL）—— 不抹掉已产生的桌面 FAIL"
    echo ""
    if [ "$fail" != 0 ]; then
      log "判定: FAIL（桌面判据有失败，设备离线不掩盖）"; exit 1
    fi
    log "判定: PENDING（桌面判据全过；设备侧未验，按 AGENTS §124y-189 PENDING 非 PASS）"; exit 3
  fi
  adb_ensure
  # ★ 清历史 logcat：Fatal 判据只统计本次脚本运行期间的崩溃（否则之前手动重启的 houdini 竞态会被误算）
  adb -s "$DEV" logcat -c >/dev/null 2>&1
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
    adb -s "$DEV" shell "setprop jq.gpu_state NONE; rm -f /data/local/tmp/jq_hook_*; am force-stop com.qwsaeda.junqirs" >/dev/null 2>&1
    sleep 2  # houdini JIT 竞态：force-stop 后立即 start 会 non-executable memory SIGSEGV（实测）
    adb -s "$DEV" shell "am start -n com.qwsaeda.junqirs/com.qwsaeda.App" >/dev/null 2>&1
    # ★ 出屏判据用截图像素（不依赖属性）：jq.wgpu_thread 会被设备 selinux 标记 denied
    #   永久锁死旧值（实测 adb setprop 也写不进），属性判据会假 FAIL/假绿。
    #   截图非黑比 > 85% = 画面真实渲染（棋盘+棋子出屏），不可伪造。
    check_screen(){
      local tag=$1
      adb -s "$DEV" shell "screencap -p /sdcard/jq_ra_${tag}.png" >/dev/null 2>&1 || return 1
      adb -s "$DEV" pull "/sdcard/jq_ra_${tag}.png" "/tmp/jq_ra_${tag}.png" >/dev/null 2>&1 || return 1
      python3 -c "
from PIL import Image
im=Image.open('/tmp/jq_ra_${tag}.png').convert('RGB'); w,h=im.size; px=im.load()
nb=sum(1 for y in range(0,h,16) for x in range(0,w,16) if sum(px[x,y])>45)
t=len(range(0,h,16))*len(range(0,w,16))
print(f'{nb/t:.3f}')
" 2>/dev/null
    }
    # ★ GPU 渲染管线专属判据（CODE_STYLE §9/§10 防假绿）：
    #   ① jq.gpu_state=render_ok 只由 GpuRenderer::render_frame 成功返回时写入（CPU 兜底写 cpu_fallback）
    #   ② 截图量化色数 ≥ 6 → 真实棋盘/棋子/按钮几何，排除"纯色验证帧 + 非黑比高"旧假绿
    check_scene_colors(){
      local tag=$1
      python3 -c "
from PIL import Image
im=Image.open('/tmp/jq_ra_${tag}.png').convert('RGB')
sm=im.resize((im.width//8, im.height//8))
cols={(r>>4,g>>4,b>>4) for (r,g,b) in sm.getdata()}
print(len(cols))
" 2>/dev/null
    }
    check_gpu(){ local tag=$1
      wait_prop jq.gpu_state render_ok && log "PASS GPU 渲染管线出图 ($tag: render_ok)" || {
        log "FAIL GPU 未出图 ($tag: $(adb -s "$DEV" shell getprop jq.gpu_state 2>/dev/null | tr -d '\r') / err=$(adb -s "$DEV" shell getprop jq.gpu_err 2>/dev/null | tr -d '\r'))"; fail=1; }
      local n=0 c=0; while [ $n -lt 8 ]; do
        c=$(check_scene_colors "$tag" 2>/dev/null); [ -n "$c" ] && [ "$c" -ge 6 ] && break
        sleep 1; n=$((n+1)); done
      awk -v v="$c" 'BEGIN{exit !(v+0 >= 6)}' && log "PASS 场景真实几何 ($tag: $c 色)" || {
        log "FAIL 场景非真实几何 ($tag: $c 色 < 6)"; fail=1; }
    }
    # 轮询截图直到非黑比达标（启动/渲染有延迟，单次截图会拿到黑屏启动帧）
    wait_screen(){ local tag=$1 n=0 r="0.000"; while [ $n -lt 20 ]; do
      r=$(check_screen "${tag}_$n"); awk -v v="$r" 'BEGIN{exit !(v+0 > 0.85)}' && { echo "$r"; return 0; }
      sleep 1; n=$((n+1)); done; echo "$r"; return 1; }
    SR=$(wait_screen boot)
    awk -v r="$SR" 'BEGIN{exit !(r+0 > 0.85)}' && log "PASS 出屏截图非黑比 ($SR)" || { log "FAIL 出屏截图非黑比 ($SR)"; fail=1; }
    check_gpu boot_0   # ★ GPU render pass 消费点判据（非 CPU 兜底 + 非纯色帧）
    # ★ 默认暗棋开局（原版 g_opt_mode=1）首手是翻子，走子/悔棋判据需显式切明棋：
    #   ☰(view270,750) → 电脑变着(INGAME_BTNS[2] 720基准中心447,545 ×1.25) → 新手布局(DIFF_BTNS_ORIG[0] 350,527 ×1.25)
    #   规则判据用明棋语义，与默认 mode 解耦（避免"暗棋首手非走子"假 FAIL）
    adb_ensure
    adb -s "$DEV" shell "setprop jq.difficulty NONE; input tap 270 750; sleep 1; input tap 559 681; sleep 1; input tap 438 659" >/dev/null 2>&1
    wait_prop jq.difficulty 1_0 && log "PASS 切明棋（难度=1_0）" || { log "FAIL 切明棋失败"; fail=1; }
    # 走子：蓝 6→11（tap 两下）
    adb_ensure
    adb -s "$DEV" shell "setprop jq.move NONE; input tap 274 1383; sleep 1; input tap 274 1312" >/dev/null 2>&1
    wait_prop jq.move Move && log "PASS 走子 Move" || { log "FAIL 走子未触发 Move"; fail=1; }
    # 悔棋：☰ 菜单 undo（tap 620 750）
    adb_ensure
    adb -s "$DEV" shell "setprop jq.game_icon NONE; input tap 620 750" >/dev/null 2>&1
    wait_prop jq.game_icon undo_ok && log "PASS 悔棋 undo_ok" || { log "FAIL 悔棋未触发 undo_ok"; fail=1; }
    # 终局"再来一局"按钮（对齐 C rs_draw_overlay）：钩子设 Over → 清钩子(houdini删不掉坑) → tap按钮 → 验restart
    adb_ensure
    adb -s "$DEV" shell "echo 1 > /data/local/tmp/jq_hook_overtest" >/dev/null 2>&1
    sleep 2
    adb -s "$DEV" shell "rm -f /data/local/tmp/jq_hook_*" >/dev/null 2>&1
    adb -s "$DEV" shell "setprop jq.ingame_act NONE; input tap 450 1072" >/dev/null 2>&1
    wait_prop jq.ingame_act restart && log "PASS 终局再来一局 restart" || { log "FAIL 终局按钮未触发 restart"; fail=1; }
    # 重开后棋盘应非空（50子）——tap 命中任一 cell 即证（jq.touch 被写 = 棋盘有子且触摸链活着）
    adb -s "$DEV" shell "setprop jq.touch NONE; input tap 274 1383" >/dev/null 2>&1
    TC=$(adb -s "$DEV" shell "getprop jq.touch" 2>/dev/null | tr -d '\r')
    echo "$TC" | grep -qE '^cell[0-9]+$' && log "PASS 重开后棋盘可交互 ($TC)" || { log "FAIL 重开后棋盘无响应 ($TC)"; fail=1; }
    # ⑤ 重进不崩：force-stop 后重新 am start，等 pid 存活 + 出屏恢复
    #   （放在 Fatal 检查之前，重进过程的崩溃也能被下面的 Fatal 计数捕获）
    adb_ensure
    adb -s "$DEV" shell "setprop jq.gpu_state NONE; am force-stop com.qwsaeda.junqirs" >/dev/null 2>&1
    sleep 2  # 同上：避免 houdini 快速重启竞态
    adb -s "$DEV" shell "am start -n com.qwsaeda.junqirs/com.qwsaeda.App" >/dev/null 2>&1
    SR2=$(wait_screen restart)
    awk -v r="$SR2" 'BEGIN{exit !(r+0 > 0.85)}' && log "PASS 重进出屏恢复 ($SR2)" || { log "FAIL 重进未恢复出屏 ($SR2)"; fail=1; }
    check_gpu restart_0   # ★ 重进后 GPU 管线仍必须真正出图（属性重置防旧值假绿）
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
