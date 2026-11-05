#!/usr/bin/env bash
# 第三轮兜底:只生成当前仍缺失的视频(429 间隔 120s,最稳)。
set -u
export $(grep -v '^#' /disk/workspaces/treasureIsl/.env | xargs) 2>/dev/null
P2="/disk/workspaces/treasureIsl/scripts/agnes_video.py"
V="/disk/workspaces/jq/course_unidbg/videos"

declare -A PROMPTS=(
[12]="Technical explainer animation: locating and invoking a shared object function inside a simulator. A symbol table box 'libjunqi.so .dynsym' with 14 named exports (OnCreate through OnRestore). An arrow to a badge 'JNI entries at Thumb odd addresses, align to even instruction'. Then a 3-step panel: getJNIEnv, callStaticJniMethodObject, object graph push to 0x43ed8. A red note 'remaining block: C++ object graph semantics missing'. Dark navy engineering blueprint style, calm motion, cyan accents, monospace symbol names, Chinese+English labels, sans-serif. No photorealism. No hands."
[13]="Technical explainer animation: JNI layer trace in a 32-bit ARM shared object. A pipeline: OnCreate entry at 0x233e1, GLView native processing, FindClass AdView (yellow warning), 14 On* static method calls. A 4-step DiagJni panel: getJNIEnv, getJavaVM, callStaticJniMethodObject, object graph push to 0x43ed8. A note '18 .text constant pool addresses need redroid dump, unidbg cannot read statically'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, sans-serif. No photorealism. No hands."
[14]="Technical explainer animation: a memory manager for a 32-bit ARM shared object. Four boxes: SvcMemory (virtual memory + stack + heap), AndroidElfLoader (ELF segment mapper), symbol resolver (findSymbol), Hook/Patch framework. A side panel 'military chess: base 0x40000000, .got zero slots, 12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT all verified passing'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, monospace addresses, sans-serif. No photorealism. No hands."
[21]="Technical explainer animation: a 32-bit ARM shared object calling fork(). A before-and-after panel: parent PID / child PID. Three sub-boxes: mock fork syscall, mock child process, handle fork return value. A note 'unidbg single-threaded simulation, no real fork, use stub to skip ad SDK background process'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, sans-serif. No photorealism. No hands."
[23]="Technical explainer animation: identifier recording in a 32-bit ARM shared object simulation. Two cases: Case 1 JNI_OnLoad environment (register 14 On* methods + FindClass AdView, green check), Case 2 file identifier strategy (one.sjg hash + timestamp, 7 config XML, 6 resolution dirs, green check). A badge 'military chess 32-bit so identifier recording (2 cases)'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, sans-serif. No photorealism. No hands."
[25]="Technical explainer animation: a seven-step environment enhancement framework for a 32-bit ARM shared object. Seven boxes: AndroidEmulator (unicorn2), DalvikVM, loadLibrary, stub libGLESv1_CM, inject 18 .text constants, hook cxa_atexit, call 14 JNI entries. Green checkmarks on all 7 steps. A red warning note 'unidbg cannot read statically, need redroid dump'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, sans-serif. No photorealism. No hands."
[26]="Technical explainer animation: environment fallback summary for a 32-bit ARM shared object. Three panels: 6-step initialization (arch selection, process name, backend, root dir, DalvikVM, loadLibrary), 3 crash types (SIGSEGV zero-GOT, SIGFPE divmod, SIGBUS C++ object graph), 4 fallback rules (syscall priority, fill .got with real ARM libc, stub GL, redroid dump 18 addresses). A badge 'military chess 32-bit so environment fallback summary'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, sans-serif. No photorealism. No hands."
[28]="Technical explainer animation: xHook detection for a 32-bit ARM shared object. Seven boxes in a circle: ELF DT_FLAGS, Linker layout, redirect tracking, memory scan, jump verification, inline hook, module detection. A side panel: unidbg countermeasure (fill .got with real ARM libc, LD_PRELOAD empty, stub GL). A badge 'military chess 32-bit so xHook detection'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, sans-serif. No photorealism. No hands."
[29]="Technical explainer animation: JNI layer reversal for a 32-bit ARM shared object. FindClass panel: com/baidu/mobads/AdView (red, return null), android/os/Environment (green), java/lang/System (green). GetMethodID panel: AdView.onCreate (red, skip), App.OnCreate (green), GLView.render (green). A badge 'military chess 32-bit so JNI reversal'. Dark navy engineering blueprint style, calm motion, cyan accents, monospace names, Chinese+English labels, sans-serif. No photorealism. No hands."
[30]="Technical explainer animation: an eight-dimension unidbg detection matrix for a 32-bit ARM shared object. A 2x4 grid: base address, JNI call, class detection, file descriptor, uname, runtime, detect Unicorn (switch to unicorn2), detect Frida (LD_PRELOAD empty). Each with a green checkmark. A badge 'military chess 32-bit so unidbg detection matrix'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, sans-serif. No photorealism. No hands."
[31]="Technical explainer animation: production deployment of a 32-bit ARM shared object via Spring Boot + unidbg. A Spring Boot controller with 2 REST endpoints. An arrow to an UnidbgRunner service wrapping AndroidEmulator, DalvikVM, loadLibrary, inject 18 .text constants, hook cxa_atexit, call 14 JNI entries. A badge 'military chess 32-bit so production deployment'. Dark navy engineering blueprint style, calm motion, cyan accents, Chinese+English labels, monospace URLs, sans-serif. No photorealism. No hands."
)

# 只处理仍缺失的段(第二轮可能已补一部分)
missing=()
for i in 12 13 14 21 23 25 26 28 29 30 31; do
  [ -f "$V/ch${i}_intro.mp4" ] || missing+=($i)
done

if [ ${#missing[@]} -eq 0 ]; then
  echo "=== 全部已生成,无需第三轮 ==="
  exit 0
fi

echo "=== 第三轮处理 ${#missing[@]} 段: ${missing[*]} ==="
for i in "${missing[@]}"; do
  echo "[gen] ch${i}..."
  python3 "$P2" --prompt "${PROMPTS[$i]}" \
    --out "$V/ch${i}_intro.mp4" --seconds 10 --ratio 16:9 2>&1 | tail -3
  sleep 120
done
echo "===ROUND3 DONE==="
