#!/bin/bash
# build_arm64_mem.sh — 编译"翻译体"arm64 库（jqt_*.c → aarch64）
# 与 build_arm32_aligned.sh 同一份 C（jqt_mem.h 影子内存 + 18 常量注入），只换目标架构。
# 目标：arm32→C→arm32 验证 C 源码后 → C→arm64 纯编译升级（so 层）
set -e
CLANG=/tmp/opencode/android-ndk-r25b/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang
SYSROOT_INC=/tmp/opencode/android-ndk-r25b/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include
JTI=/disk/workspaces/jq/rev64/src_jni_translated
OUT=${1:-/disk/workspaces/jq/rev64/dist/libjunqi_arm64_mem.so}
TMP=/tmp/jq64_mem_build
rm -rf $TMP; mkdir -p $TMP

echo "[1/4] 编译 14 个 JNI 翻译体（aarch64）+ jqt_mem"
for f in $JTI/jqt_*.c; do
  b=$(basename $f .c)
  [ "$b" = "jqt_mem" ] && continue
  cp $f $TMP/$b.c
  $CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$JTI $TMP/$b.c -o $TMP/$b.o
done
$CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$JTI $JTI/jqt_mem.c -o $TMP/jqt_mem.o

echo "[2/4] 编译镜像嵌入 jq_img.S"
$CLANG -c -O0 -fPIC -w $JTI/jq_img.S -o $TMP/jq_img.o

echo "[3/4] 编译适配层 jni_aligned.c"
$CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$SYSROOT_INC -I$JTI $JTI/jni_aligned.c -o $TMP/jni_aligned.o

echo "[4/4] 链接 arm64 .so"
$CLANG -shared -o $OUT \
  $TMP/jni_aligned.o $TMP/jqt_*.o $TMP/jq_img.o \
  -llog -lm
echo "OK: $OUT ($(stat -c%s $OUT) bytes)"
echo -n "JNI 符号: "; aarch64-linux-gnu-nm -D $OUT 2>/dev/null | grep -c Java_com || true
