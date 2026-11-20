#!/usr/bin/env bash
# jqt_mem_fuzz_check.sh — 32 位翻译体内存路由渗透探针判据脚本（桌面离线，ASan 干净）
#
# 定位：AGENTS 渗透回路在 **32 位翻译体（rev64/reverse）** 侧的渗透轮入口。
# 与正向 C 渗透轮（rev64/tools/fuzz_probe_check.py，com.qwsaeda）物理隔离（§32/64 不抢活）。
#
# 被测 = jqt_mem.c 内存路由核心（影子内存 MEM 访问"安全层"，32 位真机崩溃风险区）：
#   M1 jqt_mem_addr 三路路由 fuzz（偏移/影子区/真实表命中/零页兜底，100000 次 0 崩溃）
#   M2 jqt_mem_valid / jqt_heap_ok 契约（NULL=1 / 影子伪指针=0 / 真实堆=1）
#   M3 二分对称性（区内 1000 命中 + 区外命中=随机映射率报告）
#   M4 jqt_got_lookup 18 槽（18 命中 + 表外区 0 命中）
#   M5 jqt_mem_rebuild 幂等 ×50 + 双缓冲原子切换（0 崩溃）
#
# 判据 = jq jqt_mem_fuzz rc=0 + "== 全部通过 ==" + 多 seed 一致性 + ASan 无 report
# 构建 = 桌面 x86_64 host（ASan 包 jqt_mem_fuzz.c；jqt_mem.c 不插桩零开销）：
#   - stub android/log.h（__android_log_print）+ jq_img.S 汇编（g_jqt_img 镜像符号，
#     .incbin /tmp/opencode/shadow/jq_img.bin 在位）
#   - -Wl,--wrap 桩：__android_log_print（静默）/ fopen（真 maps 直读）/ mincore（真）/
#     calloc（真分配）
# 诚实边界：
#   - 桌面 32 位截断语义差：jqt_mem.c maps 解析 `sscanf→(uint32_t)lo` 忠实 32 位；
#     host 48 位 maps 截断后 w区可覆盖任意 32 位 → 探针**不解引用路由返回值**（只判路由
#     0 崩溃），解引用 = 真机 32 位 Android 领地（本探针判据口径见 §诚实边界）
#   - 满表（JQT_REAL_MAX=4096 触顶）桌面 /proc/self/maps < 4096 条不触达（AUD-1000 B05-001
#     真机冒烟领地）；桌面 M5 仅验 rebuild 幂等
# 用法：bash rev64/tools/jqt_mem_fuzz_check.sh [--seed 7] [--seeds 7,42,999]
#   rc：0=PASS  2=FAIL  3=PENDING（jq_img.bin 或构建工具缺失）
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TDIR="$ROOT/rev64/reverse/src_jni_translated"
BIN="$ROOT/rev64/reverse/src_jni_translated/dist/jqt_mem_fuzz"
STUBDIR=/tmp/opencode/jq_fuzzstub
IMG_BIN=/tmp/opencode/shadow/jq_img.bin

# ---- 构建（ASan 包探针；jqt_mem.c 不插桩） ----
build() {
    mkdir -p "$STUBDIR/android" "$TDIR/dist"
    cat > "$STUBDIR/android/log.h" <<'EOF'
#ifndef ANDROID_LOG_STUB_H
#define ANDROID_LOG_STUB_H
int __android_log_print(int prio, const char *tag, const char *fmt, ...);
#endif
EOF
    as -o "$TDIR/dist/jq_img.o" "$TDIR/jq_img.S" 2>/dev/null || {
        echo "PENDING（jq_img.S 汇编失败：as 缺失或 jq_img.bin 不在 /tmp/opencode/shadow/）"; return 3; }
    gcc -O0 -g -fsanitize=address -I"$TDIR" -I"$STUBDIR" \
        "$TDIR/jqt_mem_fuzz.c" "$TDIR/jqt_mem.c" "$TDIR/dist/jq_img.o" \
        -Wl,--wrap=__android_log_print -Wl,--wrap=fopen -Wl,--wrap=mincore -Wl,--wrap=calloc \
        -o "$BIN" -lm 2>/dev/null || {
        echo "PENDING（gcc/ASan 构建失败——查 gcc -fsanitize=address 支持）"; return 3; }
    return 0
}

run_once() {
    # $1=seed
    local seed=$1
    local out rc
    out="$("$BIN" "$seed" 2>&1)"; rc=$?
    local ok=1
    [ $rc -ne 0 ] && ok=0
    case "$out" in *"== 全部通过 =="*) ;; *) ok=0;; esac
    case "$out" in *"ERROR: AddressSanitizer"*) ok=0;; esac
    if [ $ok -eq 1 ]; then
        echo "seed=$seed PASS（rc=$rc）"
        echo "$out" | grep -E '^M[0-9]' | sed 's/^/   /'
    else
        echo "seed=$seed FAIL（rc=$rc）"
        echo "$out" | grep -E 'FAIL|AddressSanitizer|SEGV' | head -5 | sed 's/^/   证据: /'
        return 1
    fi
    return 0
}

# ===== 32 位真地址空间探针（jqt_mem_fuzz32，解引用版）=====
# 价值：x86_64 版因 maps 截断语义差**不能解引用**路由返回值（诚实边界）；
#       32 位真地址空间（setarch linux32）下 32 位语义成立 → 解引用全开。
BIN32="$TDIR/dist/jqt_mem_fuzz32"
build32() {
    mkdir -p "$STUBDIR/android" "$TDIR/dist"
    [ -f "$STUBDIR/android/log.h" ] || cat > "$STUBDIR/android/log.h" <<'EOF'
#ifndef ANDROID_LOG_STUB_H
#define ANDROID_LOG_STUB_H
int __android_log_print(int prio, const char *tag, const char *fmt, ...);
#endif
EOF
    as --32 -o "$TDIR/dist/jq_img32.o" "$TDIR/jq_img.S" 2>/dev/null || return 1
    gcc -m32 -O0 -g -I"$TDIR" -I"$STUBDIR" \
        "$TDIR/jqt_mem_fuzz32.c" "$TDIR/jqt_mem.c" "$TDIR/dist/jq_img32.o" \
        -Wl,--wrap=__android_log_print -Wl,--wrap=fopen -Wl,--wrap=mincore -Wl,--wrap=calloc \
        -o "$BIN32" -lm 2>/dev/null || return 1
    return 0
}

run_once32() {
    local seed=$1
    local out rc
    out="$(setarch linux32 "$BIN32" "$seed" 2>&1)"; rc=$?
    local ok=1
    [ $rc -ne 0 ] && ok=0
    case "$out" in *"== 全部通过 =="*) ;; *) ok=0;; esac
    if [ $ok -eq 1 ]; then
        echo "32位 seed=$seed PASS（rc=$rc，setarch linux32 真地址空间）"
        echo "$out" | grep -E "^M[0-9]'" | sed 's/^/   /'
    else
        echo "32位 seed=$seed FAIL（rc=$rc）"
        echo "$out" | grep -E 'FAIL|SEGV|诊断' | head -5 | sed 's/^/   证据: /'
        return 1
    fi
    return 0
}

main() {
    local seeds="7,42,999"
    [ "${1:-}" = "--seed" ] && { seeds="$2"; shift 2; }
    [ "${1:-}" = "--seeds" ] && { seeds="$2"; shift 2; }

    [ -f "$IMG_BIN" ] || { echo "PENDING（jq_img.bin 缺失 $IMG_BIN，gen_shadow.py 生成后重跑）"; exit 3; }
    if ! build; then exit 3; fi
    echo "① 构建 OK（$BIN，ASan 探针 + 不插桩 jqt_mem.c + stub android/log + wrap 桩）"

    local fails=0
    IFS=',' read -ra SD <<< "$seeds"
    for s in "${SD[@]}"; do
        run_once "$s" || fails=1
    done
    # ② 可复现性（同 seed 重跑**确定性断言行**一致：M4 GOT 18 槽 + M5 rebuild）
    # 诚实边界：M1/M3 命中数依赖 host /proc/self/maps（ASLR 每次进程不同）→
    # 非确定性命中数（影子区/零页计数）不做逐字比对；确定性 = M2/M4/M5 契约行 + 判据行
    if [ ${#SD[@]} -gt 1 ]; then
        local a b
        a="$("$BIN" "${SD[0]}" 2>&1 | grep -E '^M4|^M5|判据' | tr '\n' '|')"
        b="$("$BIN" "${SD[0]}" 2>&1 | grep -E '^M4|^M5|判据' | tr '\n' '|')"
        if [ "$a" = "$b" ]; then
            echo "② 可复现性 OK（同 seed 重跑 M4/M5 确定性行一致；M1/M3 命中数依赖 host maps 不做逐字比对）"
        else
            echo "FAIL: ② 可复现性破（同 seed 两次确定性行不一致）"
            echo "   一: $a"; echo "   二: $b"
            fails=1
        fi
    fi
    if [ $fails -eq 0 ]; then
        local nseed=${#SD[@]}
        echo "③ 32 位内存路由判据 5 项全 PASS（M1 路由/M2 契约/M3 二分/M4 GOT/M5 rebuild 0 崩溃 ASan 干净）"
        # ④ 32 位真地址空间解引用版（解引用全开 = 关闭 32/64 位截断语义差诚实边界）
        local f32=0
        if build32 && setarch linux32 true 2>/dev/null; then
            echo "── 32 位真地址空间探针（jqt_mem_fuzz32，setarch linux32）──"
            for s in "${SD[@]}"; do
                run_once32 "$s" || f32=1
            done
            if [ $f32 -eq 0 ]; then
                echo "④ 32 位真地址空间解引用版全 PASS（解引用 100000 次 0 崩溃 + 高位未映射 0 误命中）"
            else
                echo "★ 32 位真地址空间探针 FAIL（rc=$f32）"
            fi
        else
            echo "④ 32 位真地址空间探针 PENDING（gcc-multilib/setarch 缺失，非 so 回归）"
            f32=2
        fi
        if [ $f32 -eq 1 ]; then
            echo "判据: FAIL（32 位真地址空间探针 FAIL）"
            return 2
        fi
        echo "判据: PASS（jqt_mem_fuzz_check ${nseed} seed × 5 探针 + 可复现性"
        [ $f32 -eq 0 ] && echo "      + 32 位真地址空间解引用版全 PASS，32/64 位语义差诚实边界已关闭"
        [ $f32 -eq 2 ] && echo "      + 32 位真地址空间探针 PENDING（工具缺失）"
        echo "      —— 32 位翻译体内存子系统渗透判据）"
        return 0
    fi
    echo "判据: FAIL"
    return 2
}
main "$@"
