#!/bin/bash
# build_aligned.sh — 编译"对齐原版"arm64 库（14 JNI 入口 = 32 位指令机械翻译体）
# 与 build_arm64.sh 的区别：JNI 入口不用简化手写版 junqi_jni.c，而用 rev64/reverse/src_jni_translated
# 里从原版 32 位 .so 机械翻译出的 14 个 jqt_*.c + jni_aligned.c 适配层 + .rodata 明文注入。
# 用法: ./build_aligned.sh [输出.so]
set -e
CLANG=/tmp/opencode/android-ndk-r25b/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang
SYSROOT_INC=/tmp/opencode/android-ndk-r25b/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include
SRC=/disk/workspaces/jq/rev64/src
JTI=/disk/workspaces/jq/rev64/reverse/src_jni_translated
OUT=${1:-/disk/workspaces/jq/rev64/dist/libjunqi_arm64_aligned.so}
TMP=/tmp/jq64_aligned_build
mkdir -p $TMP

echo "[1/4] 编译 14 个 JNI 翻译体（aarch64）"
# ★ AUD-1000 B07-028（P1 已修，ABI 链接分叉消除）：jqt_mem/jqt_jnibr/
#   jqt_jni_calls_tbl/jqt_render_jni 单列编译（与 build_arm32_aligned.sh
#   排除逻辑对齐）——旧版 [1/4] 全编译 jqt_*.c 聚合链，与 arm32 侧
#   单列链接方式分叉（同一组源两 ABI 链接方式不同，跨 ABI 排查先
#   排除链接分叉）。单列 = jqt_mem 带自身 include 上下文、jqt_jnibr
#   带 SYSROOT jni.h、calls_tbl/render_jni 独立（与 arm32 脚本逐行同构）。
for f in $JTI/jqt_*.c; do
  b=$(basename $f .c)
  case "$b" in
    jqt_mem|jqt_jnibr|jqt_jni_calls_tbl|jqt_render_jni) continue;;  # 单列编译（下方）
  esac
  { echo '#include <stdint.h>'; echo '#include <stdio.h>'; echo '#include <stdlib.h>'; echo '#include <string.h>'; echo '#include <sys/mman.h>'; } > $TMP/$b.c
  cat $f >> $TMP/$b.c
  # ★ AUD-1000 批9 顺手修：jqt_gl_bridge.c include "jqt_mem.h"（§124zz-8 起），
  #   通用循环无 -I$JTI 找不到 → 补 JTI include 路径（其余文件无本地头不受影响）。
  $CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$JTI $TMP/$b.c -o $TMP/$b.o
done
# 单列（对齐 build_arm32_aligned [1/4] 排除项，jqt_mem.c 走 jqt_mem.h 上下文）
$CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$JTI $JTI/jqt_mem.c -o $TMP/jqt_mem.o
$CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$SYSROOT_INC -I$JTI $JTI/jqt_jnibr.c -o $TMP/jqt_jnibr.o
$CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$SYSROOT_INC -I$JTI $JTI/jqt_jni_calls_tbl.c -o $TMP/jqt_jni_calls_tbl.o
$CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$SYSROOT_INC -I$JTI $JTI/jqt_render_jni.c -o $TMP/jqt_render_jni.o

echo "[2/4] 编译适配层 jni_aligned.c（含 .rodata 明文注入 + JNI 入口签名）"
# jni_aligned.c 需 jni.h
$CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$SYSROOT_INC $JTI/jni_aligned.c -o $TMP/jni_aligned.o

echo "[3/4] 编译非 JNI 部分（渲染/蓝牙/SPAK/音频/保存/游戏逻辑，复用 rev64/src）"
for f in game_state game_logic render_gles2 bt_spak bt_net spak_load png_decode audio_sl save; do
  $CLANG -c -O2 -fPIC -fvisibility=hidden -I$SRC $SRC/$f.c -o $TMP/full_$f.o
done

echo "[4/4] 链接对齐版 .so"
$CLANG -shared -o $OUT \
  $TMP/jni_aligned.o $TMP/jqt_*.o $TMP/full_*.o \
  -llog -lGLESv2 -landroid -lz -lOpenSLES -lm
echo "OK: $OUT ($(stat -c%s $OUT) bytes)"
echo -n "JNI 符号: "; readelf --dyn-syms --wide $OUT | grep -c Java_com_cnvcs || true
