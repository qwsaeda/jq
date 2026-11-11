#!/bin/bash
# build_shadow_arm32.sh — 编译"影子运行时"**arm32(armeabi-v7a)** 库
#
# ★ 为什么需要它（§124zz-4）：
#   arm32"翻译体"（src_jni_translated/）是语义翻译层，调用闭包 3969 个函数里只覆盖 992（25%）
#   → 场景构造链缺失 → 对象图从未构造 → 只能出清屏色。
#   而"影子运行时"（gen_shadow.py 产物）是**机械翻译全量 .text**（6402 函数，含
#   F_242f4 场景初始化 + 全部 13 个惰性单例构造器）→ 覆盖 100%。
#   §116 已验证同一套 C 能编到 armv7a（14,499,632B，ELF32/ARM，14 个 JNI 导出）。
#
# 用法: ./build_shadow_arm32.sh [输出.so]
set -e
CLANG=/tmp/opencode/android-ndk-r25b/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi19-clang
GEN=/tmp/opencode/shadow
SRC=/disk/workspaces/jq/rev64/reverse/src_shadow
OUT=${1:-/disk/workspaces/jq/rev64/dist/libjunqi_arm_shadow.so}
TMP=/tmp/jq32_shadow_build
rm -rf $TMP; mkdir -p $TMP

echo "[1/5] 编译生成代码（机械翻译体全量 + dispatch + image + plt + init_array）"
GENFILES="jq_funcs_0 jq_funcs_1 jq_funcs_2 jq_funcs_3"
for f in $GENFILES jq_dispatch jq_image jq_plt_tbl jq_slot_imp jq_import_disp jq_init_array; do
  [ -f "$GEN/$f.c" ] || { echo "  ★ 缺 $GEN/$f.c（先跑 gen_shadow.py）"; exit 1; }
  $CLANG -c -O1 -fPIC -fno-strict-aliasing -I$SRC $GEN/$f.c -o $TMP/$f.o 2>&1 \
      | grep -E "error" | head -3 || true
done
# jq_funcs_thunks.c：旧版生成器把 ARM thunk 单独放这；新版（§123 起）已折入主文件 →
# 再编会 "duplicate symbol"。按"先试带 thunks，失败再不带"处理。
THUNK_OBJ=""
if [ -f "$GEN/jq_funcs_thunks.c" ]; then
  $CLANG -c -O1 -fPIC -fno-strict-aliasing -I$SRC $GEN/jq_funcs_thunks.c -o $TMP/jq_funcs_thunks.o 2>&1 \
      | grep -E "error" | head -3 || true
  [ -f "$TMP/jq_funcs_thunks.o" ] && THUNK_OBJ="$TMP/jq_funcs_thunks.o"
fi

echo "[2/5] 编译运行时（jq_rt/jq_jni/jq_imports）"
for f in jq_rt jq_jni jq_imports; do
  $CLANG -c -O1 -fPIC -fno-strict-aliasing -I$SRC $SRC/$f.c -o $TMP/$f.o 2>&1 \
      | grep -E "error" | head -3 || true
done

LINK_OBJS="$TMP/jq_funcs_0.o $TMP/jq_funcs_1.o $TMP/jq_funcs_2.o $TMP/jq_funcs_3.o \
  $TMP/jq_dispatch.o $TMP/jq_image.o $TMP/jq_plt_tbl.o $TMP/jq_slot_imp.o \
  $TMP/jq_import_disp.o $TMP/jq_init_array.o \
  $TMP/jq_rt.o $TMP/jq_jni.o $TMP/jq_imports.o"
LIBS="-llog -lGLESv1_CM -lGLESv2 -landroid -lz -lOpenSLES -lm"

echo "[3/5] 链接 arm32 .so"
if ! $CLANG -shared -o $OUT $LINK_OBJS $THUNK_OBJ $LIBS -Wl,--hash-style=sysv 2>/tmp/jq32_link_err.txt; then
  if grep -q "duplicate symbol" /tmp/jq32_link_err.txt && [ -n "$THUNK_OBJ" ]; then
    echo "  ★ thunk 与主文件重复符号 → 去掉 thunks 重链（新生成器已折入）"
    $CLANG -shared -o $OUT $LINK_OBJS $LIBS -Wl,--hash-style=sysv 2>&1 | head -5
  else
    head -8 /tmp/jq32_link_err.txt; exit 1
  fi
fi

echo "[4/5] 校验"
echo -n "  Java_com_cnvcs 导出: "; readelf --dyn-syms --wide $OUT | grep -c Java_com_cnvcs || true
echo -n "  架构: "; file -b $OUT | cut -c1-60
echo -n "  .hash 风格: "; readelf -S $OUT 2>/dev/null | grep -qE '\.gnu\.hash' && echo "gnu.hash ★MuMu 32 位 linker 不支持" || echo "sysv ✓"

echo "[5/5] OK: $OUT ($(stat -c%s $OUT) bytes)"
