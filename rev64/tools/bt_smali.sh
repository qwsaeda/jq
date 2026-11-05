#!/bin/bash
# bt_smali.sh — 蓝牙 BtSpp.java → dex → smali → 合并进构建树（批量化固化）
#
# 收敛本轮 BT 接线反复手工做的：javac → d8 → baksmali → cp 到 dec_dual/dec_full。
# BtSpp.java 改动后一条命令重编重合并，幂等（不依赖残留）。
#
# 用法:
#   bash rev64/tools/bt_smali.sh            # 编译 + 合并（幂等）
#   bash rev64/tools/bt_smali.sh --verify   # 只校验树内 BtSpp 已含最新逻辑
#
# 依赖:
#   - javac（/usr/bin/javac）
#   - android-32.jar（android.platforms）
#   - d8（build-tools 36）
#   - baksmali
#   - 源 /tmp/opencode/jq_bt_src/com/cnvcs/BtSpp.java
#   - 构建树 dec_dual/dec_full（/tmp/opencode/jq_apk/rs_a64/）
set -u
SRC=/tmp/opencode/jq_bt_src
RA=/tmp/opencode/jq_apk/rs_a64
ANDROID_JAR=/disk/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots/760/fs/opt/android/platforms/android-32/android.jar
D8=/disk/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots/741/fs/opt/android/build-tools/36.0.0/d8
[ -x "$D8" ] || D8=$(command -v d8)
API=21

# 编译期 stub（真实 App 由 dex 合并提供；仅需签名匹配，不入 dex）
cat > "$SRC/stubs/App.java" <<'EOF'
package com.cnvcs;
public class App {
    public static native void OnBluetooth(android.content.Intent i);
    public static void btState(String s) {}
    public static void btSend(String s) {}
}
EOF

cd "$SRC"
rm -rf stubs_classes classes dex smali_out
mkdir -p stubs_classes classes dex
javac -classpath "$ANDROID_JAR" -d stubs_classes stubs/App.java 2>&1 | head -3 || { echo "✗ stub 编译失败"; exit 1; }
javac -classpath "$ANDROID_JAR:stubs_classes" -d classes com/cnvcs/BtSpp.java 2>&1 | grep -v "deprecat" | head -3
[ -f classes/com/cnvcs/BtSpp.class ] || { echo "✗ BtSpp 编译失败"; exit 1; }
$D8 --output dex --min-api $API classes/com/cnvcs/*.class 2>&1 | grep -v "not found" | head -3
[ -f dex/classes.dex ] || { echo "✗ d8 失败"; exit 1; }
baksmali d --api $API dex/classes.dex -o smali_out 2>&1 | head -2
[ -f smali_out/com/cnvcs/BtSpp.smali ] || { echo "✗ baksmali 失败"; exit 1; }

cp smali_out/com/cnvcs/*.smali "$RA/dec_dual/smali/com/cnvcs/"
cp smali_out/com/cnvcs/*.smali "$RA/dec_full/smali/com/cnvcs/"
echo "✓ BtSpp smali 已合并到 dec_dual + dec_full（$(find smali_out -name '*.smali' | wc -l) 文件）"

# ★ App.smali + App$BtStart 同步：dec_full 为手工编辑源（showInfo/bt 方法/全面屏），
#   dec_dual 派生——一条命令保两树一致（防 App.smali 手工改后忘同步 dec_dual）。
for f in App.smali 'App$BtStart.smali'; do
    if [ -f "$RA/dec_full/smali/com/cnvcs/$f" ]; then
        cp "$RA/dec_full/smali/com/cnvcs/$f" "$RA/dec_dual/smali/com/cnvcs/$f"
    fi
done
echo "✓ App.smali/App\$BtStart 同步到 dec_dual（源=dec_full）"

if [ "${1:-}" = "--verify" ]; then
    for t in dec_dual dec_full; do
        n=$(ls "$RA/$t/smali/com/cnvcs/BtSpp"*.smali 2>/dev/null | wc -l)
        m=$(grep -c "btSend\|btHost\|btClient\|btState" "$RA/$t/smali/com/cnvcs/App.smali" 2>/dev/null)
        echo "  [$t] BtSpp.smali=$n App bt 方法=$m"
    done
fi
