#!/usr/bin/env bash
# 串行单线程补视频:每次只跑一段,429 则 sleep 90 重试,彻底避免并发限流。
# 只补缺失的段。
set -u
export $(grep -v '^#' /disk/workspaces/treasureIsl/.env | xargs) 2>/dev/null
P2="/disk/workspaces/treasureIsl/scripts/agnes_video.py"
V="/disk/workspaces/jq/course_unidbg/videos"

declare -A PROMPTS=(
[21]="Technical explainer animation: a 32-bit ARM shared object calling fork(). A before-and-after panel: parent PID / child PID. Three sub-boxes: mock fork syscall, mock child process, handle fork return value. A note 'unidbg single-threaded simulation, no real fork, use stub to skip ad SDK background process'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, sans-serif. No photorealism. No hands."
[23]="Technical explainer animation: identifier recording in a 32-bit ARM shared object simulation. Two cases: Case 1 JNI_OnLoad environment (register 14 On* methods + FindClass AdView, green check), Case 2 file identifier strategy (one.sjg hash + timestamp, 7 config XML, 6 resolution dirs, green check). A badge 'military chess 32-bit so identifier recording (2 cases)'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, sans-serif. No photorealism. No hands."
[25]="Technical explainer animation: a seven-step environment enhancement framework for a 32-bit ARM shared object. Seven boxes: AndroidEmulator (unicorn2), DalvikVM, loadLibrary, stub libGLESv1_CM, inject 18 .text constants, hook cxa_atexit, call 14 JNI entries. Green checkmarks on all 7 steps. A red warning note 'unidbg cannot read statically, need redroid dump'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, sans-serif. No photorealism. No hands."
[26]="Technical explainer animation: environment fallback summary for a 32-bit ARM shared object. Three panels: 6-step initialization (arch selection, process name, backend, root dir, DalvikVM, loadLibrary), 3 crash types (SIGSEGV zero-GOT, SIGFPE divmod, SIGBUS C++ object graph), 4 fallback rules (syscall priority, fill .got with real ARM libc, stub GL, redroid dump 18 addresses). A badge 'military chess 32-bit so environment fallback summary'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, sans-serif. No photorealism. No hands."
[28]="Technical explainer animation: xHook detection for a 32-bit ARM shared object. Seven boxes in a circle: ELF DT_FLAGS, Linker layout, redirect tracking, memory scan, jump verification, inline hook, module detection. A side panel: unidbg countermeasure (fill .got with real ARM libc, LD_PRELOAD empty, stub GL). A badge 'military chess 32-bit so xHook detection'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, sans-serif. No photorealism. No hands."
[29]="Technical explainer animation: JNI layer reversal for a 32-bit ARM shared object. FindClass panel: com/baidu/mobads/AdView (red, return null), android/os/Environment (green), java/lang/System (green). GetMethodID panel: AdView.onCreate (red, skip), App.OnCreate (green), GLView.render (green). A badge 'military chess 32-bit so JNI reversal'. Dark navy engineering blueprint style, calm motion, cyan accents, monospace names, Chinese+English labels, sans-serif. No photorealism. No hands."
[30]="Technical explainer animation: an eight-dimension unidbg detection matrix for a 32-bit ARM shared object. A 2x4 grid: base address, JNI call, class detection, file descriptor, uname, runtime, detect Unicorn (switch to unicorn2), detect Frida (LD_PRELOAD empty). Each with a green checkmark. A badge 'military chess 32-bit so unidbg detection matrix'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, sans-serif. No photorealism. No hands."
[31]="Technical explainer animation: production deployment of a 32-bit ARM shared object via Spring Boot + unidbg. A Spring Boot controller with 2 REST endpoints. An arrow to an UnidbgRunner service wrapping AndroidEmulator, DalvikVM, loadLibrary, inject 18 .text constants, hook cxa_atexit, call 14 JNI entries. A badge 'military chess 32-bit so production deployment'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, monospace URLs, sans-serif. No photorealism. No hands."
)

# 先杀掉旧的第二轮脚本(避免并发触发 429)
pkill -f retry_videos_round2.sh 2>/dev/null
sleep 3

for i in 21 23 25 26 28 29 30 31; do
  [ -f "$V/ch${i}_intro.mp4" ] && { echo "[skip] ch${i} exists"; continue; }
  echo "[gen] ch${i} (serial, 429 backoff 90s)..."
  # 429 退避:最多试 3 次,每次失败 sleep 90s
  ok=0
  for attempt in 1 2 3; do
    out=$(python3 "$P2" --prompt "${PROMPTS[$i]}" \
      --out "$V/ch${i}_intro.mp4" --seconds 10 --ratio 16:9 2>&1)
    if [ -f "$V/ch${i}_intro.mp4" ]; then
      echo "  OK ch${i} (attempt $attempt)"
      ok=1
      break
    fi
    echo "  FAIL ch${i} attempt $attempt: $(echo "$out" | tail -1)"
    sleep 90
  done
  [ $ok -eq 1 ] || echo "  GIVING UP ch${i} after 3 attempts"
done
echo "===ROUND3-SERIAL DONE==="
