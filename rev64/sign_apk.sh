#!/bin/bash
# sign_apk.sh — v1+v2 双签（替代已丢失的 /tmp/opencode/apksigner 的 SignV2 工具）
# 用法: bash rev64/sign_apk.sh <in.apk> <out.apk>
# 依赖: system apksigner (/usr/bin/apksigner) + junqi.keystore (junqi2026/junqi/junqi2026)
set -e
IN=$1; OUT=$2
KEY=/disk/workspaces/jq/junqi.keystore
APKSIGNER=/usr/bin/apksigner
# apksigner sign --v1-signing-enabled true --v2-signing-enabled true
$APKSIGNER sign --ks $KEY --ks-key-alias junqi --ks-pass pass:junqi2026 --key-pass pass:junqi2026 \
  --v1-signing-enabled true --v2-signing-enabled true \
  --out $OUT $IN
$APKSIGNER verify --verbose $OUT 2>&1 | grep -E "Verified|Number" | head -4
echo "OK: $OUT ($(stat -c%s $OUT) bytes)"
