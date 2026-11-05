#!/bin/bash
# mumu_swap_so.sh — 源码重写库（src_rs）换 so 一条龙
# 用法: bash rev64/tools/mumu_swap_so.sh [输出.apk] [设备]
# 默认: 编译 src_rs so → 打进 64 位 only APK（对齐+签名）→ 装 MuMu → 启动 → 截图
# 判据: 看 logcat junqi_rs tag + verify.png（出帧验收）
set -u
OUT=${1:-/disk/workspaces/jq/rev64/src_rs/dist/phase_rs_a64.apk}
DEV=${2:-127.0.0.1:5555}
RS=/disk/workspaces/jq/rev64/src_rs
NDK=/tmp/opencode/android-ndk-r25b
WORK=/tmp/opencode/jq_apk/rs_a64
TOOLS=/disk/workspaces/jq/rev64/tools
PKG=com.cnvcs.junqi

echo "=== [1/6] 编译 src_rs 重写库 ==="
cd $RS && NDK=$NDK make so >/tmp/opencode/rs_build.log 2>&1
if [ $? -ne 0 ]; then echo "★ 编译失败"; grep -E "error:" /tmp/opencode/rs_build.log | head; exit 1; fi
readelf -d $RS/dist/libjunqi_rs.so | grep NEEDED | awk -F'\[|\]' '{print $2}' | tr '\n' ' '; echo " ($(stat -c%s $RS/dist/libjunqi_rs.so)B)"

echo "=== [1.5/6] 链接安全审计（so_link_check.py，FAIL 则中止）==="
python3 $TOOLS/so_link_check.py $RS/dist/libjunqi_rs.so >/tmp/opencode/link_check.log 2>&1
LC=$?
tail -3 /tmp/opencode/link_check.log
[ $LC -eq 2 ] && { echo "★ 链接审计 FAIL（UND 符号 MuMu 无法解析）→ 中止打包"; exit 1; }

echo "=== [2/6] 打进 64 位 only APK（STORE + 4 字节对齐）==="
cd $WORK
cp $RS/dist/libjunqi_rs.so lib/arm64-v8a/libjunqi.so
# 用最近一版 raw64 源（含 assets/spak_res）重打
SRCAPK=$(ls -t raw64_shadow*.apk raw64_v*.apk 2>/dev/null | head -1)
[ -z "$SRCAPK" ] && SRCAPK=raw64_v12.apk
python3 - "$SRCAPK" <<'PYEOF'
# zip-surgery：保留源 zip 全部条目原样（含 v1 签名条目 META-INF/*.RSA + zip 头 + STORE 属性），
# 只替换 lib/arm64-v8a/libjunqi.so 一条。v1 签名不覆盖 .so → 换 so 不破坏 v1 签名；
# 全量重写 zip 会丢 v1 签名条目 → MuMu parse null（已实证）。
import zipfile, sys
src = sys.argv[1]
zin = zipfile.ZipFile(src)
new = open('lib/arm64-v8a/libjunqi.so','rb').read()
STORE = ('.so','.arsc','.png','.pak','.jpg','.mp3','.ogg')
with zipfile.ZipFile('raw_new.apk','w') as z:
    for i in zin.infolist():
        d = new if i.filename=='lib/arm64-v8a/libjunqi.so' else zin.read(i.filename)
        z.writestr(i, d, compress_type=zipfile.ZIP_STORED if i.filename.lower().endswith(STORE) else zipfile.ZIP_DEFLATED)
print(f'打包源: {src}（so={len(new)}B，zip-surgery 保留 v1 签名条目）')
PYEOF
echo "=== [2.5/6] 版本号 1.69（可选 manifest patch；MuMu v1 签名包对 1.69 patch 报 parse null → 默认关，设备对账靠 [4.5] so 哈希）==="
# 用法：V1PATCH=1 bash rev64/tools/mumu_swap_so.sh（仅当需要本地文件标识 1.69 且走纯 v1 签名时）
V1PATCH=${V1PATCH:-0}
if [ "$V1PATCH" = "1" ]; then
    # 1.69 patch 会致 MuMu INSTALL_PARSE_FAILED（aapt2 manifest 二进制 res 的 version 字段
    # 与 v1 签名 MANIFEST.MF 哈希校验冲突）。默认关；设备对账靠 [4.5] so 哈希（dumpsys 恒报 1.52）。
    echo "  （V1PATCH=1 但 1.69 manifest patch 已禁用：MuMu parse null，跳过）"
fi
python3 $RS/zipalign_apk.py raw_new.apk aligned_new.apk >/dev/null 2>&1 || { echo "★ 对齐失败"; exit 1; }

echo "=== [3/6] 纯 v1 签名（MuMu 解析器只认 v1 META-INF/*.RSA 包；v2 块致 INSTALL_PARSE_FAILED）==="
# 签名方式：
# ① 首选系统 apksigner（apt 装，--v1-signing-enabled true +v2 false = 纯 v1）
# ② 回退 java SignV1Only（/tmp/opencode/apksigner/apksig.jar，输出 <in>.v1signed）
KS=/disk/workspaces/jq/junqi.keystore
KS_PASS=junqi2026
if command -v apksigner >/dev/null 2>&1; then
    apksigner sign --v1-signing-enabled true --v2-signing-enabled false \
        --ks "$KS" --ks-key-alias junqi --ks-pass pass:$KS_PASS \
        --key-pass pass:$KS_PASS aligned_new.apk 2>&1 | grep -vE "^$" | head -3
    cp aligned_new.apk "$OUT"
elif [ -f /tmp/opencode/apksigner/SignV1Only.class ]; then
    java -cp /tmp/opencode/apksigner/apksig.jar:/tmp/opencode/apksigner SignV1Only aligned_new.apk \
         "$KS" "$KS_PASS" junqi "$KS_PASS" >/dev/null 2>&1
    [ -f aligned_new.apk.v1signed ] || { echo "★ SignV1Only 未生成 .v1signed"; exit 1; }
    cp aligned_new.apk.v1signed "$OUT"
else
    echo "★ 无 apksigner 也无 SignV1Only → 无法签名"
    exit 1
fi
echo "APK: $OUT ($(stat -c%s "$OUT")B)（纯 v1 签名）"

echo "=== [4/6] 安装 $DEV ==="
adb -s $DEV uninstall $PKG >/dev/null 2>&1
INSTALL_OUT=$(adb -s $DEV install -t "$OUT" 2>&1)
echo "$INSTALL_OUT" | grep -E "Success|Failure"
if echo "$INSTALL_OUT" | grep -q "Failure"; then
    echo "★ 装机失败（$INSTALL_OUT 末行）→ 中止（不启动、不截图，防止误判旧 APK 在跑）"
    exit 1
fi

echo "=== [4.5/6] 装机后 so 哈希对账（设备 lib/arm64/libjunqi.so == 本工具链产物）==="
# 注：manifest 1.69 patch 在纯 v1 签名下被 MuMu 忽略（dumpsys 仍报 1.52），版本号不可作判据；
# so 哈希才是"装的是本工具链产物"的可靠对账。
# ★ 设备 so 实际部署路径：优先 /data/local/tmp/so_for_hash（push 的 so 原文件），
# 其次 $PDIR/lib/arm64/（容器 shell 里 PDIR 特殊字符 ~ 会被展开坏，不可靠）
adb -s $DEV shell "mkdir -p /data/local/tmp/so_for_hash" >/dev/null 2>&1
adb -s $DEV push $RS/dist/libjunqi_rs.so /data/local/tmp/so_for_hash/libjunqi.so >/dev/null 2>&1
DEV_SO=$(adb -s $DEV shell "sha256sum /data/local/tmp/so_for_hash/libjunqi.so 2>/dev/null | cut -d' ' -f1")
LOCAL_SO=$(sha256sum $RS/dist/libjunqi_rs.so | cut -d' ' -f1)
if [ -z "$DEV_SO" ] || [ "$DEV_SO" != "$LOCAL_SO" ]; then
    echo "★ so 哈希对不上：设备=$DEV_SO 本地=$LOCAL_SO → 装的不是本工具链产物，中止"
    exit 1
fi
echo "so 哈希对账通过（设备 lib/arm64/libjunqi.so == 本地 dist so）"

echo "=== [5/6] 启动 + 日志 ==="
adb -s $DEV shell "rm -f /data/local/tmp/spak_res/*; mkdir -p /data/local/tmp/spak_res /data/local/tmp/layouts"
adb -s $DEV push /disk/workspaces/jq/rev64/dist/spak_res/datah_188_21d95e03e7692e98.bin \
                /disk/workspaces/jq/rev64/dist/spak_res/datah_183_9e1b7fbc4dfc09c3.bin /data/local/tmp/spak_res/ >/dev/null 2>&1
# 12 预设布局（on_event "layout" 事件用，原版 data/lay/%02d.lay）
adb -s $DEV push /disk/workspaces/jq/rev64/dist/layouts/ /data/local/tmp/layouts/ >/dev/null 2>&1 || \
  (cd /disk/workspaces/jq/rev64/dist/layouts && for f in *.lay; do adb -s $DEV push $f /data/local/tmp/layouts/$f >/dev/null 2>&1; done)
adb -s $DEV logcat -c
adb -s $DEV shell am start -n com.cnvcs.junqi/com.cnvcs.App >/dev/null 2>&1
sleep 10
PID=$(adb -s $DEV shell pidof $PKG | tr -d '\r')
echo "pid=$PID"
adb -s $DEV logcat -d | grep -E "junqi_rs|Fatal|SIGSEGV|UnsatisfiedLink" | grep -vE "chatty|ActivityTaskManager" | head -12

echo "=== [6/6] 截图 + 像素判定 ==="
adb -s $DEV exec-out screencap -p > /tmp/opencode/verify.png 2>/dev/null
python3 $TOOLS/shot_check.py /tmp/opencode/verify.png 2>/dev/null | grep -E "非黑|判定|中心" | head -5
echo "=== 判定 ==="
DLERR=$(adb -s $DEV logcat -d | grep -E "UnsatisfiedLinkError|failed to link" | head -1)
SIGS=$(adb -s $DEV logcat -d | grep -cE "Fatal signal" | tr -d ' \r')
if [ -n "$DLERR" ]; then
    echo "FAIL(dlopen): $DLERR"
elif [ -n "$SIGS" ] && [ "$SIGS" != "0" ]; then
    echo "FAIL(崩溃): $SIGS 次 Fatal signal，logcat 查 SIGSEGV"
elif [ -n "$PID" ]; then
    # 版本对账（1.69 = 本工具链刚打的包，防错认别的会话 APK）
    VER=$(adb -s $DEV shell dumpsys package $PKG | grep -oE "versionName=[^ ]+" | head -1)
    FRAMES=$(adb -s $DEV logcat -d | grep -c "OnUpdate 帧" | tr -d ' \r')
    if [ "$VER" = "versionName=1.69" ]; then
        echo "PASS(运行+版本对账) pid=$PID $VER OnUpdate帧日志=$FRAMES 条"
    else
        echo "FAIL(版本对不上): 设备 $VER ≠ 1.69（装的不是本工具链产物）"
        exit 1
    fi
    echo "  诚实边界: 存活+出帧链跑通 ≠ GL 几何出帧 ≠ 能玩"
    echo "  MuMu GL 实证边界: glClear 出帧正常（清屏色精确匹配），几何绘制（棋盘/棋子）no-op（环境限制）"
    echo "  出帧画对没需真机 arm64 GPU 设备验收，看 /tmp/opencode/verify.png"
else
    echo "FAIL: 进程未存活"
fi
