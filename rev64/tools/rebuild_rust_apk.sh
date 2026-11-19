#!/usr/bin/env bash
# rebuild_rust_apk.sh — src_rs_rust（Rust 64 位正向）换 so 一条龙
# 对齐 rebuild_rs_apk.sh（C 版）：Rust 交叉编译 so → 注入 dec_rs → STORE → zipalign → 双签 → 装机 → 对账
# 用法：
#   bash rev64/tools/rebuild_rust_apk.sh            # 编译 Rust so → 打包 → 签名 → 装 → 对账
#   bash rev64/tools/rebuild_rust_apk.sh --no-install  # 不装机（设备不可达时仅出 APK）
set -uo pipefail

ROOT=/disk/workspaces/jq
RUST=$ROOT/rev64/forward/src_rs_rust
WORK=/tmp/opencode/jq_apk
DECRS=$WORK/dec_rs        # com.qwsaeda.junqirs 构建树（smali/manifest 对齐 qwsaeda）
OUTAPK=$ROOT/dist/phase_rs_rust.apk
NDK_CLANG=/tmp/opencode/android-ndk-r25b/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang
export PATH="$HOME/.cargo/bin:$PATH"

log(){ echo "[rebuild_rust_apk] $*"; }

[ -d "$RUST" ] || { log "★ src_rs_rust 缺失"; exit 1; }
[ -d "$DECRS" ] || { log "★ dec_rs 构建树缺失: $DECRS（需 apktool d 解包，包名 com.qwsaeda.junqirs）"; exit 1; }

# [0/6] 交叉编译 Rust so（arm64 cdylib，14 JNI 符号）
log "=== [0/6] 交叉编译 Rust so（aarch64-linux-android）==="
cd "$RUST"
cargo build --target aarch64-linux-android --release 2>&1 | grep -E "^error|Finished" | head -5
RSO=$RUST/target/aarch64-linux-android/release/libjunqi_rs.so
[ -f "$RSO" ] || { log "★ Rust so 编译失败"; exit 1; }
# 14 符号门禁（对齐 C so_link_check）
NSYM=$(nm -D "$RSO" 2>/dev/null | grep -cE "Java_com_qwsaeda_App_")
[ "$NSYM" -ge 14 ] || { log "★ 14 JNI 符号缺失（实 $NSYM）"; exit 1; }
log "Rust so OK：$(du -h "$RSO" | cut -f1)，JNI 导出 $NSYM 符号"

# [1/6] 注入 so（dec_rs lib/arm64-v8a/）
log "=== [1/6] 注入 Rust so → dec_rs ==="
LIBDIR=$DECRS/lib/arm64-v8a
mkdir -p "$LIBDIR"
cp "$RSO" "$LIBDIR/libjunqi_rs.so"
log "so 注入：$LIBDIR/libjunqi_rs.so（$(md5sum "$LIBDIR/libjunqi_rs.so" | cut -d' ' -f1)）"

# [2/6] apktool 重编
log "=== [2/6] apktool b dec_rs → 源 APK ==="
cd "$DECRS"
SRCAPK=$WORK/rs_rust_src.apk
apktool b . -o "$SRCAPK" 2>&1 | tail -2
[ -f "$SRCAPK" ] || { log "★ apktool b 失败"; exit 1; }

# [3/6] STORE + zipalign（.so/.arsc 未压缩 + 4 字节对齐，targetSdk≥31 必须）
log "=== [3/6] STORE + zipalign ==="
STORED=$WORK/rs_rust_stored.apk
python3 "$WORK/store_compressed.py" "$SRCAPK" "$STORED" 2>&1 | tail -2
ALIGNED=$WORK/rs_rust_aligned.apk
# zipalign（.so 4 字节，-p 4 png 16k 对齐）
zipalign -f -p 4 4194304 "$STORED" "$ALIGNED" 2>&1 | tail -1 || zipalign -f -p 4 "$STORED" "$ALIGNED"
[ -f "$ALIGNED" ] || { log "★ zipalign 失败"; exit 1; }

# [4/6] 双签（v1+v2，apksigner 工具，对齐 C rebuild_rs_apk.sh + 防管道吞码 PIPESTATUS）
log "=== [4/6] v1+v2 双签（apksigner）==="
set -o pipefail
apksigner sign --v1-signing-enabled true --v2-signing-enabled true \
  --ks "$ROOT/junqi.keystore" --ks-pass pass:junqi2026 \
  --ks-key-alias junqi --key-pass pass:junqi2026 "$ALIGNED" 2>&1 | tail -2
sign_rc=${PIPESTATUS[0]}
[ $sign_rc -ne 0 ] && { log "★ 签名失败（apksigner rc=$sign_rc）"; exit 1; }
mv -f "$ALIGNED" "$OUTAPK"
if ! apksigner verify --verbose "$OUTAPK" 2>&1 | grep -qE "Verified using v[0-9] scheme .*: true"; then
    log "★ 验签失败"; apksigner verify --verbose "$OUTAPK" 2>&1 | head -6; exit 1;
fi
log "APK：$OUTAPK（$(du -h "$OUTAPK" | cut -f1)）+ 双签验证 PASS"

# [5/6] 装机（设备可达时；--no-install 或设备不可达跳过）
if [ "${1:-}" = "--no-install" ]; then
    log "=== [5/6] --no-install 跳过装机 ==="
else
    log "=== [5/6] 装机 10.0.182.152:5555 ==="
    timeout 5 adb connect 10.0.182.152:5555 2>&1 | head -1
    if adb -s 10.0.182.152:5555 install -r "$OUTAPK" 2>&1 | grep -q "Success"; then
        log "装机 Success"
    else
        log "★ 装机失败（设备不可达/安装错误）——APK 已出，设备可达后手动 adb install"
    fi
fi

# [6/6] 对账（设备 so 哈希 = 本地）
if [ "${1:-}" != "--no-install" ]; then
    log "=== [6/6] so 哈希对账 ==="
    LOCAL_MD5=$(md5sum "$RSO" | cut -d' ' -f1)
    DEV_SO=$(adb -s 10.0.182.152:5555 shell "pm path com.qwsaeda.junqirs" 2>/dev/null | head -1 | sed 's/package://')
    if [ -n "$DEV_SO" ]; then
        DEV_MD5=$(adb -s 10.0.182.152:5555 shell "unzip -p $DEV_SO lib/arm64-v8a/libjunqi_rs.so 2>/dev/null | md5sum" | cut -d' ' -f1)
        [ "$LOCAL_MD5" = "$DEV_MD5" ] && log "对账 PASS（$LOCAL_MD5）" || log "★ 对账 FAIL（本地 $LOCAL_MD5 / 设备 $DEV_MD5）"
    else
        log "（设备不可达，对账 PENDING）"
    fi
fi

log "完成：$OUTAPK（Rust so $(md5sum "$RSO" | cut -d' ' -f1)）"
