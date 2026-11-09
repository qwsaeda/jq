#!/bin/bash
# build_arm32_aligned.sh — 编译"翻译体"arm32 库（jqt_*.c → armv7a）
# 目标：arm32→C→arm32 验证——编成 32 位 so，unidbg 加载跑 OnCreate 比对原版 32 位 so
# MEM 用 jqt_mem.h（镜像数组 + 18 常量注入），非裸指针
set -e
CLANG=/tmp/opencode/android-ndk-r25b/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi24-clang
SYSROOT_INC=/tmp/opencode/android-ndk-r25b/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include
JTI=/disk/workspaces/jq/rev64/src_jni_translated
OUT=${1:-/disk/workspaces/jq/rev64/dist/libjunqi_arm32_aligned.so}
TMP=/tmp/jq32_aligned_build
rm -rf $TMP; mkdir -p $TMP

echo "[1/4] 编译 JNI 翻译体 + 会话 B 规则引擎（jq_rs_*.c）+ jqt_mem"
# 会话 B 规则引擎（rules.c/board.c/layout.c/touch.c，纯 C 无 GL/JNI 依赖，
# 作为 32 位 so 内嵌走子验证基准——§124y-19 借鉴会话 B 正向开发）
for f in $JTI/jq_rs_rules.c $JTI/jq_rs_board.c $JTI/jq_rs_layout.c $JTI/jq_rs_touch.c $JTI/jq_rs_walkbridge.c $JTI/jq_rs_ruleprobe.c; do
  [ -f "$f" ] || { echo "⚠ 缺会话 B 模块 $f"; continue; }
  b=$(basename $f .c)
  cp $f $TMP/$b.c
  # A4-13：+src_rs（jq_default_lay.h 双端共享头）
  $CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$SYSROOT_INC -I$JTI -I/disk/workspaces/jq/rev64/src_rs $TMP/$b.c -o $TMP/$b.o
done
for f in $JTI/jqt_*.c; do
  b=$(basename $f .c)
  # ★ AUD-1000 B07-028（P1 已修，ABI 链接分叉消除）：arm64 build_aligned.sh
  #   旧版 [1/4] 全编译 jqt_*.c（含 jqt_mem/jqt_jnibr/jqt_jni_calls_tbl/
  #   jqt_render_jni）+ [4/4] 全链 $TMP/jqt_*.o——与 arm32 本脚本的
  #   单列编译（jqt_mem 带 jqt_mem.h include 上下文 / jqt_jnibr 带
  #   SYSROOT jni.h / calls_tbl+render_jni 独立）分叉，同一组源在
  #   两 ABI 产物里链接方式不同（arm64 聚合链 vs arm32 单列链），
  #   排查"arm32 过 arm64 挂"（或反之）时先排除链接分叉。现 arm64
  #   侧对齐本脚本排除逻辑（build_aligned.sh 行 24 同步 case 排除，
  #   此处 jqt_mem 单独补 SYSROOT_INC 一致性）。
  [ "$b" = "jqt_mem" ] && continue
  [ "$b" = "jqt_jnibr" ] && continue
  [ "$b" = "jqt_jni_calls_tbl" ] && continue
  cp $f $TMP/$b.c
  $CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$JTI $TMP/$b.c -o $TMP/$b.o
done
$CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$JTI $JTI/jqt_mem.c -o $TMP/jqt_mem.o
# JQJNI 桥（so→Java JNI 语义）：jqt_jnibr.c + jqt_jni_calls_tbl.c
$CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$SYSROOT_INC -I$JTI $JTI/jqt_jnibr.c -o $TMP/jqt_jnibr.o
$CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$SYSROOT_INC -I$JTI $JTI/jqt_jni_calls_tbl.c -o $TMP/jqt_jni_calls_tbl.o
$CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$SYSROOT_INC -I$JTI $JTI/jqt_render_jni.c -o $TMP/jqt_render_jni.o

echo "[2/4] 编译镜像嵌入 jq_img.S"
$CLANG -c -O0 -fPIC -w $JTI/jq_img.S -o $TMP/jq_img.o

echo "[3/4] 编译适配层 jni_aligned.c"
$CLANG -c -O0 -fPIC -fvisibility=hidden -w -I$SYSROOT_INC -I$JTI $JTI/jni_aligned.c -o $TMP/jni_aligned.o

echo "[4/4] 链接 arm32 .so"
JQT_OBJ=""
for o in $TMP/jqt_*.o; do
  bn=$(basename $o .o)
  case "$bn" in jqt_jnibr|jqt_jni_calls_tbl|jqt_render_jni) ;; *) JQT_OBJ="$JQT_OBJ $o";; esac
done
# ★ §124y-19：会话 B 规则引擎（jq_rs_*.o）也链进 so（纯 C 走子逻辑，无 GL 依赖）
JQ_RS_OBJ=""
for o in $TMP/jq_rs_*.o; do JQ_RS_OBJ="$JQ_RS_OBJ $o"; done
$CLANG -shared -o $OUT \
  $TMP/jni_aligned.o $JQT_OBJ $JQ_RS_OBJ $TMP/jqt_jnibr.o $TMP/jqt_jni_calls_tbl.o $TMP/jqt_render_jni.o $TMP/jq_img.o \
  -Wl,-z,lazy -Wl,--hash-style=sysv \
  -llog -lm -lGLESv1_CM -lGLESv2
echo "OK: $OUT ($(stat -c%s $OUT) bytes)"
echo -n "JNI 符号: "; arm-linux-gnueabi-readelf --dyn-syms --wide $OUT 2>/dev/null | grep -c Java_com_cnvcs || true
