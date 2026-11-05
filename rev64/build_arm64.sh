#!/bin/bash
# build_arm64.sh — 编译 arm64 libjunqi（R6 逆向对齐路线）
# 用法: ./build_arm64.sh [输出.so路径]
# 依赖: NDK r25b 于 /tmp/opencode/android-ndk-r25b
set -e
SRC=/disk/workspaces/jq/rev64/src
OUT=${1:-/disk/workspaces/jq/rev64/dist/libjunqi_arm64_hand.so}
CLANG=/tmp/opencode/android-ndk-r25b/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang
TMP=/tmp/jq64_build
mkdir -p $TMP
cd $SRC
for f in game_state game_logic junqi_jni render_gles2 bt_spak bt_net spak_load png_decode audio_sl save; do
  $CLANG -c -O2 -fPIC -fvisibility=hidden -I. $f.c -o $TMP/jq64_$f.o
done
$CLANG -shared -o $OUT \
  $TMP/jq64_game_state.o $TMP/jq64_game_logic.o $TMP/jq64_junqi_jni.o $TMP/jq64_render_gles2.o \
  $TMP/jq64_bt_spak.o $TMP/jq64_bt_net.o $TMP/jq64_spak_load.o $TMP/jq64_png_decode.o $TMP/jq64_audio_sl.o $TMP/jq64_save.o \
  -llog -lGLESv2 -landroid -lz -lOpenSLES -lm
echo "OK: $OUT ($(stat -c%s $OUT) bytes)"
# 校验 JNI 符号数
echo -n "JNI 符号: "
readelf --dyn-syms --wide $OUT | grep -c Java_com || true
