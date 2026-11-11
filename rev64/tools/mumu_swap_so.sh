#!/bin/bash
# mumu_swap_so.sh — 源码重写库（src_rs）换 so 一条龙
# 用法: bash rev64/tools/mumu_swap_so.sh [输出.apk] [设备]
# 默认: 编译 src_rs so → 打进 64 位 only APK（对齐+签名）→ 装 MuMu → 启动 → 截图
# 判据: 看 logcat junqi_rs tag + verify.png（出帧验收）
set -u
OUT=${1:-/disk/workspaces/jq/rev64/forward/src_rs/dist/phase_rs_a64.apk}
# ★ 坑固化（2026-11-06）：相对路径 OUT 在脚本 cd $WORK 后 cp 会静默失败（目录不存在）
#   → OUT 永不更新 → 设备一直跑老 so（"改了没效果/钩子不触发"假象）。
#   统一转绝对路径（相对路径按调用时 CWD 解析）。
case "$OUT" in
  /*) ;;
  *) OUT="$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/$(basename "$OUT")" ;;
esac
DEV=${2:-127.0.0.1:5555}
RS=/disk/workspaces/jq/rev64/forward/src_rs
NDK=/tmp/opencode/android-ndk-r25b
WORK=/tmp/opencode/jq_apk/rs_a64
TOOLS=/disk/workspaces/jq/rev64/tools
PKG=com.cnvcs.junqirs

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
# ★★ 坑固化（2026-11-06）：源 APK 必须**包名 = $PKG**（com.cnvcs.junqirs）。
#   旧实现按"最近修改"选 raw64_shadow*.apk → 那是 com.cnvcs.junqi 包名（另一个包），
#   装机后 junqirs 根本没被更新，而旧"对账"查的是任意安装包 → 永远"通过"
#   → 设备长期跑老 so（"改了没效果/钩子不触发"假象的真凶）。
#   现在：优先用 rs168_wav.apk / raw168_168.apk（已知包名 junqirs），并用 aapt 校验包名。
SRCAPK=""
for cand in rs168_wav.apk raw168_168.apk rs168_new.apk; do
    [ -f "$WORK/$cand" ] || continue
    cpkg=$(aapt dump badging "$WORK/$cand" 2>/dev/null | grep -oE "package: name='[^']+'" | head -1 | sed "s/.*name='//;s/'//")
    if [ "$cpkg" = "$PKG" ]; then SRCAPK="$cand"; break; fi
done
if [ -z "$SRCAPK" ]; then
    echo "★ 找不到包名=$PKG 的源 APK（rs168_wav.apk / raw168_168.apk）→ 中止（防止装错包名导致设备跑老 so）"
    exit 1
fi
echo "  [源 APK] $SRCAPK（包名已校验 = $PKG）"
# ★★ 坑固化（2026-11-06）：rs168_wav.apk 的 manifest 无全屏 theme（默认 theme 带 ActionBar
#   + 状态栏）→ GLView 不全屏 → MotionEvent 坐标相对 view 顶，与 720×1280 基准差一截
#   → 全部触摸命中失效（误判为"按钮点了没反应"）。dec_rs 树 manifest 已补
#   android:theme="@android:style/Theme.NoTitleBar.Fullscreen" + App.smali FLAG_FULLSCREEN
#   + setSystemUiVisibility(0x1706) 沉浸式 sticky。
#   故优先从 dec_rs 树 apktool 重建（含全屏 theme + 最新 smali），失败才回退 $SRCAPK。
DECTREE=$WORK/dec_rs
if [ -d "$DECTREE" ]; then
    cp $RS/dist/libjunqi_rs.so $DECTREE/lib/arm64-v8a/libjunqi.so
    # ★ 2026-11-11 现代全面屏适配：App.smali 源在 src_rs（git 记录，含 displayCutout
    #   SHORT_EDGES + 沉浸式），构建时同步到 dec_rs（防 dec_rs 重建丢 displayCutout）。
    if [ -f "$RS/App.smali" ]; then
        cp $RS/App.smali $DECTREE/smali/com/qwsaeda/App.smali
    fi
    (cd $WORK && apktool b dec_rs -o raw_decrs.apk) >/tmp/opencode/mumu_apktool.log 2>&1
    if [ -f "$WORK/raw_decrs.apk" ]; then
        SRCAPK="raw_decrs.apk"
        echo "  [源 APK] 改用 dec_rs 树重建（含全屏 theme + 沉浸式，apktool b 成功）"
    else
        echo "  [警告] dec_rs 树重建失败（回退 $SRCAPK，可能无全屏 theme → 触摸坐标会偏）"
        tail -3 /tmp/opencode/mumu_apktool.log
    fi
fi
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

# ★ 打包后自检（2026-11-06 坑固化）：OUT 内 so md5 必须 == dist so md5，
#   否则说明打包/复制失败 → 后续装机跑的还是老 so（静默失败，必须显式报错中止）。
APK_SO_MD5=$(unzip -p aligned_new.apk lib/arm64-v8a/libjunqi.so 2>/dev/null | md5sum | awk '{print $1}')
DIST_SO_MD5=$(md5sum $RS/dist/libjunqi_rs.so | awk '{print $1}')
if [ "$APK_SO_MD5" != "$DIST_SO_MD5" ]; then
  echo "★ 自检失败：APK 内 so md5=$APK_SO_MD5 != dist so md5=$DIST_SO_MD5（打包/复制失败，中止——否则设备跑老 so）"
  exit 1
fi
echo "  [自检] APK 内 so md5 == dist so md5（$DIST_SO_MD5）"
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
INSTALL_RC=$?
echo "$INSTALL_OUT" | grep -E "Success|Failure"
# ★ AUD-1000-ALL A5-F5-003（P1 已修）：install rc 断言——旧版只 grep "Failure" 文本，
#   设备离线（adb 连接断开）时 install 输出 "error: device offline"（非 Failure 字样）
#   → grep 漏判静默继续（假绿"装机成功"实为设备不可达）。rc 非 0 或含 Failure/error 均中止。
if [ $INSTALL_RC -ne 0 ] || echo "$INSTALL_OUT" | grep -qE "Failure|error: device|error: " ; then
    echo "★ 装机失败（rc=$INSTALL_RC，离线/失败文本命中）→ 中止（不启动、不截图，防止误判旧 APK 在跑）"
    exit 1
fi

echo "=== [4.5/6] 装机后 so 哈希对账（★ 查设备实际安装的 so，非自 push 副本）==="
# ★★ 坑固化（2026-11-06）：旧实现 push 一份 so 到 /data/local/tmp 再与本地比 → 永远相等
#   = 假对账，设备实际跑老 so 也不报错（"改了没效果/钩子不触发"假象的真凶）。
#   现改为：查 pm path 定位已安装 APK → 读其 lib/<abi>/libjunqi.so 的真实 sha256。
APK_PATH=$(adb -s $DEV shell "pm path $PKG" 2>/dev/null | head -1 | sed 's/package://' | tr -d '\r')
DEV_SO=""
if [ -n "$APK_PATH" ]; then
    # 已安装 APK 的 so：从设备 pull 出来算 sha256（最可靠，不依赖容器 shell 路径展开）
    adb -s $DEV pull "$APK_PATH" /tmp/opencode/_installed.apk >/dev/null 2>&1
    DEV_SO=$(unzip -p /tmp/opencode/_installed.apk lib/arm64-v8a/libjunqi.so 2>/dev/null | sha256sum | cut -d' ' -f1)
    [ -z "$DEV_SO" ] && DEV_SO=$(unzip -p /tmp/opencode/_installed.apk lib/arm64/libjunqi.so 2>/dev/null | sha256sum | cut -d' ' -f1)
fi
LOCAL_SO=$(sha256sum $RS/dist/libjunqi_rs.so | cut -d' ' -f1)
if [ -z "$DEV_SO" ] || [ "$DEV_SO" != "$LOCAL_SO" ]; then
    echo "★ 设备实际 so 哈希对不上：设备=${DEV_SO:0:12} 本地=${LOCAL_SO:0:12}"
    echo "  → 装机未生效（老 so 还在跑）→ 强制重装（uninstall + install）后重试"
    adb -s $DEV uninstall $PKG >/dev/null 2>&1
    adb -s $DEV install -t "$OUT" 2>&1 | grep -E "Success|Failure"
    sleep 1
    APK_PATH=$(adb -s $DEV shell "pm path $PKG" 2>/dev/null | head -1 | sed 's/package://' | tr -d '\r')
    if [ -n "$APK_PATH" ]; then
        adb -s $DEV pull "$APK_PATH" /tmp/opencode/_installed.apk >/dev/null 2>&1
        DEV_SO=$(unzip -p /tmp/opencode/_installed.apk lib/arm64-v8a/libjunqi.so 2>/dev/null | sha256sum | cut -d' ' -f1)
    fi
    if [ -z "$DEV_SO" ] || [ "$DEV_SO" != "$LOCAL_SO" ]; then
        echo "★ 强制重装后仍对不上（设备=${DEV_SO:0:12} 本地=${LOCAL_SO:0:12}）→ 中止（设备跑的不是本工具链产物）"
        exit 1
    fi
fi
echo "so 哈希对账通过（设备已安装 APK 内 so == 本地 dist so，sha256=${LOCAL_SO:0:12}）"

echo "=== [5/6] 启动 + 日志 ==="
adb -s $DEV shell "mkdir -p /data/local/tmp/spak_res /data/local/tmp/layouts"
# ★ 全量 spak_res（bin + png 纹理），勿只推 bin（漏推 PNG → 菜单/☰/棋盘 UI 全退化）
#   ★ 勿 rm 再推（会删掉 PNG）；单文件循环幂等。
#   ★ 诚实边界（对抗审计 P3-A3/P1-A4）：全量=965 张（962 SPAK 解包 + 3 裁剪覆盖），
#   arm64 src_rs 实际只读 44 张（spak_render_refs.txt 清单）。推 965 全量无害但慢
#   （965 次 adb push）；若只需 44 张可改 `for f in $(cat $WS/rev64/tools/spak_render_refs.txt)`。
for f in /disk/workspaces/jq/rev64/dist/spak_res/*; do
  adb -s $DEV push "$f" /data/local/tmp/spak_res/ >/dev/null 2>&1
done
# 12 预设布局（on_event "layout" 事件用，原版 data/lay/%02d.lay）
adb -s $DEV push /disk/workspaces/jq/rev64/dist/layouts/ /data/local/tmp/layouts/ >/dev/null 2>&1 || \
  (cd /disk/workspaces/jq/rev64/dist/layouts && for f in *.lay; do adb -s $DEV push $f /data/local/tmp/layouts/$f >/dev/null 2>&1; done)
adb -s $DEV logcat -c
# ★ 对抗审计 P2-A8（P2-6）：包名修正——原 `com.cnvcs.junqi`（32 位原版包）与
#   PKG=com.cnvcs.junqirs（64 位包）矛盾：装 64 位 so 后启动 32 位 app → so 永不加载，
#   :179 pidof $PKG（junqirs）查不到进程 → 假红 FAIL。改 $PKG（junqirs）。
#   主 activity = com.cnvcs.App（aapt dump badging launchable-activity 实证）。
# ★ AUD-1000-ALL A5-F5-028/029（P1 已修）：启动前清 logcat（防 stale Fatal signal 误判）
#   + sleep 10 改轮询 wait（MuMu 冷启首帧窗口 20-25s，固定 10s 截图全黑假红）。
adb -s $DEV logcat -c 2>/dev/null || true
adb -s $DEV shell am start -n "$PKG/com.cnvcs.App" >/dev/null 2>&1
# 轮询进程存活（最长 30s，每 2s 一次，替代固定 sleep 10）
PID=""
for _w in $(seq 1 15); do
  sleep 2
  PID=$(adb -s $DEV shell pidof $PKG | tr -d '\r')
  [ -n "$PID" ] && break
done
echo "pid=$PID"
adb -s $DEV logcat -d | grep -E "junqi_rs|Fatal|SIGSEGV|UnsatisfiedLink" | grep -vE "chatty|ActivityTaskManager" | head -12

echo "=== [6/6] 截图 + 像素判定 ==="
adb -s $DEV exec-out screencap -p > /tmp/opencode/verify.png 2>/dev/null
python3 $TOOLS/shot_check.py /tmp/opencode/verify.png 2>/dev/null | grep -E "非黑|判定|中心" | head -5
echo "=== 判定 ==="
# ★ A5-F5-029：logcat -c 已清（启动前），-d 内容=本次启动段，stale Fatal 误判消除
DLERR=$(adb -s $DEV logcat -d | grep -E "UnsatisfiedLinkError|failed to link" | head -1)
SIGS=$(adb -s $DEV logcat -d | grep -cE "Fatal signal" | tr -d ' \r')
if [ -n "$DLERR" ]; then
    echo "FAIL(dlopen): $DLERR"
elif [ -n "$SIGS" ] && [ "$SIGS" != "0" ]; then
    echo "FAIL(崩溃): $SIGS 次 Fatal signal，logcat 查 SIGSEGV"
elif [ -n "$PID" ]; then
    # 版本对账（so 哈希已在 [4.5] 通过 = 本工具链产物；dumpsys 恒报 1.52 是 MuMu v1 签名包限制，非判据）
    VER=$(adb -s $DEV shell dumpsys package $PKG | grep -oE "versionName=[^ ]+" | head -1)
    FRAMES=$(adb -s $DEV logcat -d | grep -c "OnUpdate 帧" | tr -d ' \r')
    # 以 so 哈希对账为准（[4.5] 已通过），版本号仅作信息记录
    echo "PASS(运行+so哈希对账) pid=$PID $VER（MuMu dumpsys 恒报 1.52，so 哈希已通过 [4.5] 对账）OnUpdate帧日志=$FRAMES 条"
    echo "  诚实边界: 存活+出帧链跑通 ≠ GL 几何出帧 ≠ 能玩"
    echo "  MuMu GL 实证边界: glClear 出帧正常（清屏色精确匹配），几何绘制（棋盘/棋子）no-op（环境限制）"
    echo "  出帧画对没需真机 arm64 GPU 设备验收，看 /tmp/opencode/verify.png"
else
    echo "FAIL: 进程未存活"
fi
