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
# ★ 对抗审计 P1-B14/P2-A9（假绿修正）：verify 失败必须 abort——
#   原 grep "Verified|Number" 无输出时静默继续 → 签名实际失败仍报 OK。
#   现 apksigner verify 非 0 退出码即 FAIL（v1/v2 任一无效）。
if ! $APKSIGNER verify --verbose $OUT 2>&1 | tee /tmp/opencode/sign_verify.log | grep -q "Verified"; then
  echo "✗ 验签失败（v1/v2 任一无效）："
  head -8 /tmp/opencode/sign_verify.log
  exit 1
fi
grep -E "Verified|Number" /tmp/opencode/sign_verify.log | head -4
echo "OK: $OUT ($(stat -c%s $OUT) bytes)"
