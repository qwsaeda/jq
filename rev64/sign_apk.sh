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
# ★ 对抗审计修复（§2026-11-11）：旧写法 `apksigner verify | tee | grep -q` 里
#   grep -q 一旦匹配首行就退出 → tee/apksigner 收到 SIGPIPE → pipefail 把整条
#   管道判为失败 → 明明签名成功也报"验签失败"（假红，构建被误中止）。
#   改为：先把完整输出落日志（不接早退的 grep），再对日志做判定。
if ! $APKSIGNER verify --verbose $OUT > /tmp/opencode/sign_verify.log 2>&1; then
  echo "✗ 验签失败（apksigner verify 非 0）："
  head -8 /tmp/opencode/sign_verify.log
  exit 1
fi
if ! grep -qE "Verified using v[0-9] scheme .*: true" /tmp/opencode/sign_verify.log; then
  echo "✗ 验签失败（v1/v2 无 true）："
  head -8 /tmp/opencode/sign_verify.log
  exit 1
fi
# v2 必须真启用（新版 Android 安装硬要求）
grep -qE "Verified using v2 scheme .*: true" /tmp/opencode/sign_verify.log || {
  echo "✗ 验签失败（v2 未启用/未通过）："
  head -8 /tmp/opencode/sign_verify.log
  exit 1
}
grep -E "Verified|Number" /tmp/opencode/sign_verify.log | head -4
echo "OK: $OUT ($(stat -c%s $OUT) bytes)"
