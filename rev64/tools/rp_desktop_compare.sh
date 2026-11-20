#!/usr/bin/env bash
# rp_desktop_compare.sh — 规则引擎 32/64 位真双端行为对比（桌面 -m32，无需设备）
#
# 背景：rule_probe_compare.py --offline 只跑 64 位桌面探针、跳过 32 位对比（无 32 位设备），
#       导致"36 case 双端一致"的宣称在离线态没有真跑 = 静默降级假绿。
# 本脚本用 gcc -m32 把 32 位逆向翻译体（jq_rs_rules.c/jq_rs_board.c + 同源探针）
# 编成桌面 ELF32 可执行，与 64 位重写探针逐行 diff → 真正的双端行为实证。
#
# 用法：bash rev64/tools/rp_desktop_compare.sh
# rc：0=PASS（34/36一致 + 2已知分叉 R-008/R-004） / 1=FAIL（出现未知分歧） / 3=PENDING（-m32不可用）
set -uo pipefail
WS=/disk/workspaces/jq
TMP=/tmp/opencode/rp_desktop
rm -rf "$TMP"; mkdir -p "$TMP"

# 0. -m32 可用性
echo 'int main(){return 0;}' > "$TMP/t.c"
if ! gcc -m32 "$TMP/t.c" -o "$TMP/t" 2>/dev/null; then
  echo "PENDING: gcc -m32 不可用（缺 multilib）"; exit 3
fi

# 1. 构建 32 位探针（隔离目录，桌面适配：去 android/log.h、PROBE_LOG→printf、共享源本地化）
cp "$WS/rev64/reverse/src_jni_translated/jq_rs_rules.c" \
   "$WS/rev64/reverse/src_jni_translated/jq_rs_board.c" \
   "$WS/rev64/reverse/src_jni_translated/jq_rs_rules.h" \
   "$WS/rev64/forward/src_rs/rule_probe_shared.inc" "$TMP/"
cat > "$TMP/main.c" <<'EOF'
#include <stdio.h>
#include "jq_rs_rules.h"
void jq_rule_probe_run(void);
int main(void){ jq_rule_probe_run(); return 0; }
EOF
sed -e 's|#include <android/log.h>|#include <stdio.h>|' \
    -e 's|#define PROBE_LOG(...) __android_log_print(4, "junqi", __VA_ARGS__)|#define PROBE_LOG(...) printf(__VA_ARGS__); printf("\\n")|' \
    -e 's|#include "../../forward/src_rs/rule_probe_shared.inc"|#include "rule_probe_shared.inc"|' \
    "$WS/rev64/reverse/src_jni_translated/jq_rs_ruleprobe.c" > "$TMP/ruleprobe_desktop.c"
gcc -m32 -O1 -o "$TMP/rp32" "$TMP/ruleprobe_desktop.c" "$TMP/jq_rs_rules.c" "$TMP/jq_rs_board.c" "$TMP/main.c" -lm 2>/dev/null
[ -x "$TMP/rp32" ] || { echo "FAIL: 32位探针编译失败"; exit 1; }

# 2. 64 位桌面探针（用现成 dist/jq_ruleprobe，必要时重编）
RP64="$WS/rev64/forward/src_rs/dist/jq_ruleprobe"
if [ ! -x "$RP64" ]; then
  (cd "$WS/rev64/forward/src_rs" && make ruleprobe >/dev/null 2>&1)
fi
[ -x "$RP64" ] || { echo "FAIL: 64位探针缺失"; exit 1; }

# 3. 双端输出逐行 diff
"$TMP/rp32" 2>/dev/null | grep RULE-PROBE > "$TMP/p32.txt"
"$RP64" 2>/dev/null | grep RULE-PROBE > "$TMP/p64.txt"
N32=$(wc -l < "$TMP/p32.txt"); N64=$(wc -l < "$TMP/p64.txt")
echo "32位探针 $N32 行 / 64位探针 $N64 行"
if [ "$N32" != 36 ] || [ "$N64" != 36 ]; then echo "FAIL: 行数异常(应36)"; exit 1; fi

# 4. 已知分叉白名单（R-008 F2 / R-004 G3，EXPERIENCE_LOG 坐实）
diff -u "$TMP/p32.txt" "$TMP/p64.txt" > "$TMP/diff.txt" 2>&1
if [ $? -eq 0 ]; then
  echo "PASS: 36/36 双端逐字节一致"; exit 0
fi
# 提取分歧 case 名
FORKS=$(grep '^-[^-]' "$TMP/diff.txt" | grep -oE 'RULE-PROBE [A-Za-z0-9_]+' | awk '{print $2}' | sort -u | tr '\n' ' ')
echo "分歧 case: $FORKS"
KNOWN=" F2_flip_then_stalemate G3_try_side0xff "
UNKNOWN=0
for f in $FORKS; do
  case "$KNOWN" in *" $f "*) echo "  已知分叉 $f（R-008/R-004）";; *) echo "  ★ 未知分叉 $f"; UNKNOWN=1;; esac
done
if [ "$UNKNOWN" = 0 ]; then
  echo "PASS: 34/36 一致 + 2 已知分叉（R-008/R-004），无新增分歧"; exit 0
else
  echo "FAIL: 出现未知分叉（需白盒定位）"; cat "$TMP/diff.txt" | head -20; exit 1
fi
