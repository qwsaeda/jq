#!/bin/bash
# pack_phase.sh — 重打包三件套（apktool b → store_compressed → SignV2）
# ★ AUD-1000 B07-030（P1 历史脚本标注）：本脚本产 'phase_hand.apk'（SignV2 = v1 单签），
#   非 phase0-3 交付物；交付用 build_phases.sh + sign_apk.sh（v1+v2 双签，可装机）。
# 用法: ./pack_phase.sh [输入.so] [输出.apk]
# 依赖: /tmp/opencode/jq_apk（dec 目录）、store_compressed.py、apksigner
# ★ AUD-1000 B06-051（P1 假绿三型-管道吞码）：加 set -o pipefail——旧版仅 set -e，
#   下方 apktool b / store / SignV2 各段 `2>&1 | tail -1` 吞上游失败码（tail 退 0）
#   → 重打包失败静默继续到签名/验证段，产出旧 APK 假绿。
#   注：本脚本为**历史脚本**（B07-030 标注），交付物构建用 build_phases.sh + sign_apk.sh
#   （v1+v2 双签）；本处保留 SignV2（v1 时代 CLI 旧名），勿误当可装交付 APK。
set -e
set -o pipefail
SO=${1:-/disk/workspaces/jq/rev64/dist/libjunqi_arm64_hand.so}
OUT=${2:-/disk/workspaces/jq/rev64/dist/phase_hand.apk}
APKROOT=/tmp/opencode/jq_apk
KEY=/disk/workspaces/jq/junqi.keystore
cp $SO $APKROOT/dec/lib/arm64-v8a/libjunqi.so
cd $APKROOT
apktool b dec -o /tmp/opencode/phase_hand_new.apk 2>&1 | tail -1
python3 store_compressed.py /tmp/opencode/phase_hand_new.apk /tmp/opencode/phase_hand_stored.apk 2>&1 | tail -1
cd /tmp/opencode/apksigner
java -cp apksig.jar:apksigner-cli.jar:. SignV2 /tmp/opencode/phase_hand_stored.apk $KEY junqi2026 junqi junqi2026 2>&1 | tail -1
cp /tmp/opencode/phase_hand_stored.apk.signed $OUT
echo "=== 验证 ==="
java -cp apksig.jar:apksigner-cli.jar:. CheckSig $OUT 2>&1 | tail -1
python3 -c "
import zipfile, struct
z=zipfile.ZipFile('$OUT')
bad=[i.filename for i in z.infolist() if i.filename.endswith('.so') and i.compress_type!=0]
print('压缩.so:', bad if bad else '无')
print('manifest 头: 0x%08x'%struct.unpack_from('<I',z.read('AndroidManifest.xml'),0)[0])
print('APK内 .so:', len(z.read('lib/arm64-v8a/libjunqi.so')), 'bytes')
"
echo "OK: $OUT ($(stat -c%s $OUT) bytes)"
