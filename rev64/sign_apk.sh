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
# ★ AUD-1000 D6-#20 P0 修：旧判据 grep -q "Verified" 命中 "Verified using v2 ...: false"
#   也通过（判据恒真）。改法：apksigner verify 非 0 退出码直接 FAIL（pipefail 下
#   管道任一环节失败即失败），再确认 grep 到至少一个 ": true"。
set -o pipefail
if ! $APKSIGNER verify --verbose $OUT 2>&1 | tee /tmp/opencode/sign_verify.log | grep -qE "Verified using v[0-9] scheme .*: true"; then
  echo "✗ 验签失败（v1/v2 无 true 或 apksigner 非 0）："
  head -8 /tmp/opencode/sign_verify.log
  exit 1
fi
grep -E "Verified|Number" /tmp/opencode/sign_verify.log | head -4
echo "OK: $OUT ($(stat -c%s $OUT) bytes)"
