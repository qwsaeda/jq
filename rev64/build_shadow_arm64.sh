#!/bin/bash
# build_shadow_arm64.sh — 编译"影子运行时"arm64 库（2670+ 函数机械翻译体 + JNI 适配层）
# 输入: /tmp/opencode/shadow（gen_shadow.py 产物）+ rev64/src_shadow（运行时）
# 输出: rev64/dist/libjunqi_arm64_shadow.so
# 用法: ./build_shadow_arm64.sh [输出.so]
set -e
CLANG=/tmp/opencode/android-ndk-r25b/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang
GEN=/tmp/opencode/shadow
SRC=/disk/workspaces/jq/rev64/src_shadow
OUT=${1:-/disk/workspaces/jq/rev64/dist/libjunqi_arm64_shadow.so}
TMP=/tmp/jq64_shadow_build
mkdir -p $TMP

echo "[1/5] 编译生成代码（jq_funcs_*/dispatch/image/plt/slot/import_disp/init_array）"
for f in jq_funcs_0 jq_funcs_1 jq_funcs_2 jq_funcs_3 jq_dispatch jq_image jq_plt_tbl jq_slot_imp jq_import_disp jq_init_array; do
  $CLANG -c -O1 -fPIC -fno-strict-aliasing -I$SRC $GEN/$f.c -o $TMP/$f.o 2>&1 | grep -E "error" | head -3 || true
done

echo "[2/5] 编译运行时（jq_rt/jq_jni/jq_imports）"
for f in jq_rt jq_jni jq_imports; do
  $CLANG -c -O1 -fPIC -fno-strict-aliasing -I$SRC $SRC/$f.c -o $TMP/$f.o 2>&1 | grep -E "error" | head -3 || true
done

echo "[3/5] 链接 arm64 .so"
$CLANG -shared -o $OUT \
  $TMP/jq_funcs_0.o $TMP/jq_funcs_1.o $TMP/jq_funcs_2.o $TMP/jq_funcs_3.o \
  $TMP/jq_dispatch.o $TMP/jq_image.o $TMP/jq_plt_tbl.o $TMP/jq_slot_imp.o \
  $TMP/jq_import_disp.o $TMP/jq_init_array.o $TMP/jq_rt.o $TMP/jq_jni.o $TMP/jq_imports.o \
  -llog -lGLESv1_CM -lGLESv2 -landroid -lz -lOpenSLES -lm 2>&1 | head -10

echo "[4/5] 校验 JNI 导出符号"
echo -n "Java_com_cnvcs 导出: "; readelf --dyn-syms --wide $OUT | grep -c Java_com_cnvcs || true

echo "[5/5] OK: $OUT ($(stat -c%s $OUT) bytes)"
