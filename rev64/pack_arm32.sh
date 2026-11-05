#!/bin/bash
# pack_arm32.sh — 打包 arm32 翻译体 APK（重打包三件套，工具自足版）
#
# 历史：旧版依赖 /tmp/opencode/apksigner（SignV2 工具）+ /tmp/opencode/jq_apk/store_compressed.py，
# /tmp 被清后工具丢失。本版自足：
#   - store_compressed.py 随脚本重建（STORE 逻辑不变）
#   - 签名用 system apksigner（v1+v2 双签）替代 SignV2
#   - zipalign 用 build-tools 36.0.0
#
# 步骤：lib/armeabi = 翻译体 so → apktool b → STORE → zipalign -f 4 → apksigner v1+v2
# 用法: ./pack_arm32.sh [输入.so] [输出.apk]
set -e
SO=${1:-/disk/workspaces/jq/rev64/dist/libjunqi_arm32_aligned.so}
OUT=${2:-/disk/workspaces/jq/rev64/dist/phase_chain_arm32.apk}
APKROOT=/tmp/opencode/jq_apk
KEY=/disk/workspaces/jq/junqi.keystore

# 原版 so 保护
if [ -f $APKROOT/dec/lib/armeabi/libjunqi.so ] && [ ! -f /tmp/opencode/orig_libjunqi.so ]; then
  cp $APKROOT/dec/lib/armeabi/libjunqi.so /tmp/opencode/orig_libjunqi.so
fi
rm -f $APKROOT/dec/lib/armeabi/libjunqi.so.orig

# 1. lib/armeabi = 翻译体
mkdir -p $APKROOT/dec/lib/armeabi
cp $SO $APKROOT/dec/lib/armeabi/libjunqi.so
# 2. 删 arm64/armeabi-v7a（强制 32 位 armeabi）
rm -rf $APKROOT/dec/lib/arm64-v8a $APKROOT/dec/lib/armeabi-v7a

# 2.5 链接安全审计门禁（N1 固化：C.3.1"打包前必跑"从手工步骤→每轮真跑）
#   不过则中止打包（防"补完函数却漏了 fns 表定义 → UND 悬空 → MuMu failed to link"，
#   §124zz 头号根因 v178e1~e3 实证）。逃生口：NO_AUDIT=1 跳过（应急用，非常态）。
#   缓存库缺失时 link_audit 自动降级为"基线/缓存 0 库"模式（只做 dispatch/悬空/.hash 检查，
#   不阻塞打包）——工具自足，不依赖 /tmp/opencode/mumu_arm_libs 在。
if [ -z "$NO_AUDIT" ]; then
  echo "=== 链接安全审计（link_audit.py，FAIL 则中止打包）==="
  if ! python3 /disk/workspaces/jq/rev64/tools/link_audit.py audit "$SO"; then
    echo "✗ 链接审计 FAIL → 中止打包（修 fns 表悬空 UND / .hash 风格 / dispatch 表后再试）"
    exit 1
  fi
fi

# 3. 重打包三件套
cd $APKROOT
apktool b dec -o /tmp/opencode/phase32_new.apk 2>&1 | tail -1
python3 $APKROOT/store_compressed.py /tmp/opencode/phase32_new.apk /tmp/opencode/phase32_stored.apk 2>&1 | tail -1
ZIPALIGN=/disk/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots/741/fs/opt/android/build-tools/36.0.0/zipalign
[ -x "$ZIPALIGN" ] || ZIPALIGN=zipalign
$ZIPALIGN -f 4 /tmp/opencode/phase32_stored.apk /tmp/opencode/phase32_aligned.apk 2>&1 | tail -1
bash /disk/workspaces/jq/rev64/sign_apk.sh /tmp/opencode/phase32_aligned.apk /tmp/opencode/phase32_signed.apk
cp /tmp/opencode/phase32_signed.apk $OUT

echo "=== 验证 ==="
apksigner verify --verbose $OUT 2>&1 | grep -E "Verified|Number" | head -3

python3 -c "
import zipfile, struct
z=zipfile.ZipFile('$OUT')
bad=[i.filename for i in z.infolist() if i.filename.endswith('.so') and i.compress_type!=0]
print('压缩.so:', bad if bad else '无')
mh=struct.unpack_from('<I',z.read('AndroidManifest.xml'),0)[0]
print('manifest 头: 0x%08x'%mh, 'OK' if mh==0x00080003 else '★ 错误')
import subprocess
v=subprocess.run(['aapt2','dump','badging','$OUT'],capture_output=True,text=True).stdout
for l in v.splitlines():
    if 'versionCode' in l: print(l.split('package:')[1].split('platformBuildVersionName')[0].strip()[:80])
"
echo "OK: $OUT ($(stat -c%s $OUT) bytes)"
