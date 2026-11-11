#!/bin/bash
# build_rs.sh — 逆源码重写（src_rs）构建脚本
# 用法：
#   ./build_rs.sh desk          # 桌面 headless 可执行（验收：能编译 + 走一局）
#   ./build_rs.sh so            # NDK arm64 共享库（14 On* JNI 入口）
#   ./build_rs.sh apk [so路径]  # 打包可安装 APK（复用 dec/ apktool 产物 + 三件套签名）
set -e
cd "$(dirname "$0")"
NDK=${NDK:-/tmp/opencode/android-ndk-r25b}

case "${1:-desk}" in
  desk)
    make desk
    echo "桌面可执行: dist/jq_run"
    ;;
  so)
    make so NDK="$NDK"
    echo "NDK arm64 库: dist/libjunqi_rs.so"
    readelf -h dist/libjunqi_rs.so | grep -E 'Class|Machine'
    readelf --dyn-syms -W dist/libjunqi_rs.so | grep -c 'Java_com' | xargs echo "JNI 导出数:"
    ;;
  apk)
    SO="${2:-dist/libjunqi_rs.so}"
    [ -f "$SO" ] || { echo "先 make so"; exit 1; }
    DEC=/tmp/opencode/jq_apk/dec
    WORK=/tmp/opencode/jq_apk/rs_apk
    rm -rf "$WORK"; cp -r "$DEC" "$WORK"
    # 双 ABI：保留原版 32 位 armeabi libjunqi.so（完整逻辑），
    # 新增 arm64-v8a = 本次逆源码重写库（规则引擎 + 14 On* 桩）。
    mkdir -p "$WORK/lib/arm64-v8a"
    cp "$SO" "$WORK/lib/arm64-v8a/libjunqi.so"
    cd "$WORK"
    apktool b . -o raw.apk 2>/dev/null
    python3 /tmp/opencode/jq_apk/store_compressed.py raw.apk stored.apk 2>/dev/null || \
      python3 /tmp/opencode/jq_apk/store_compressed.py raw.apk stored.apk
    java -cp /tmp/opencode/apksigner/apksig.jar:/tmp/opencode/apksigner SignV2 \
         stored.apk /disk/workspaces/jq/junqi.keystore junqi2026 junqi junqi2026
    cp stored.apk.signed /disk/workspaces/jq/rev64/forward/src_rs/dist/phase_rs_apk.apk
    echo "APK: dist/phase_rs_apk.apk"
    # 校验：v1+v2 签名 + .so 未压缩 + 双 ABI
    cd /tmp/opencode/jq_apk/rs_apk
    unzip -l stored.apk.signed | grep -E '\.so$|\.arsc$|AndroidManifest' 
    ;;
  *)
    echo "用法: $0 [desk|so|apk [so路径]]"; exit 1
    ;;
esac
