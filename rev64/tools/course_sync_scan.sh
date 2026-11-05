#!/bin/bash
# course_sync_scan.sh — 课程正文"旧口径残留"5 模式批量扫描（C.5b 红线 4 批量化 / 轮次 21）
# 目的：附录先演进、章节正文漂移（ch01/02/03 各 1 处漏同步的同型问题）——
#       31 章正文 + 附录全量扫 5 类旧口径，输出"需修/留痕 OK"清单，不自动改（改需人定性）
# 用法：bash rev64/tools/course_sync_scan.sh [额外模式...]
# 输出：每模式命中行 + 定性建议；命中行含"§124x-z/早期/演进/误记/笔误"= 留痕 OK，否则 = 需修
set -u
COURSE=/disk/workspaces/jq/course_unidbg
BODY="$COURSE/0[1-4]*"
FAIL=0

echo "=== 课程正文旧口径批量扫描（C.5b 红线 4 / 轮次 21 I8）==="

# 模式 1：18 常量旧路线（redroid dump 注入，无 §124x-z 静态固化标注 = 需修）
M1=$(grep -rn "redroid dump 72B 注入" $BODY 2>/dev/null | grep -v "§124x-z\|早期\|留痕" | grep -v "已做")
[ -n "$M1" ] && { echo "[模式1 18常量 redroid dump 裸写] 需修："; echo "$M1"; FAIL=1; }
echo "  模式1 总命中：$(grep -rc 'redroid dump 72B 注入' $BODY 2>/dev/null | awk -F: '{s+=$2} END{print s}') 行（带演进标注 = 留痕 OK）"

# 模式 2：音效 16 旧口径（N23，实测 14）
M2=$(grep -rn "音效 16\|16 音效\|共 16 个" $BODY 2>/dev/null | grep -v "误记\|早期\|N23\|改 14")
[ -n "$M2" ] && { echo "[模式2 音效 16] 需修："; echo "$M2"; FAIL=1; }

# 模式 3：旧数字（1553/5456/33 PLT，无"笔误/旧口径"标注 = 需修）
M3=$(grep -rn "1553\|5456\|33 PLT" $BODY 2>/dev/null | grep -v "笔误\|旧口径\|早期\|N8")
[ -n "$M3" ] && { echo "[模式3 旧数字] 需修："; echo "$M3"; FAIL=1; }

# 模式 4：旧回归项数（11/11、10/10 全 PASS，无"→12/12"演进标注 = 需修）
M4=$(grep -rn "11/11 全 PASS\|10/10 全 PASS" $BODY 2>/dev/null | grep -v "→\|演进\|升级")
[ -n "$M4" ] && { echo "[模式4 旧回归项] 需修："; echo "$M4"; FAIL=1; }

# 模式 5：旧断言/全套项数（evverify 5/5、6/6 全对；fulltest 4/4、6/6 PASS，无演进标注 = 需修）
M5=$(grep -rn "5/5 全对\|6/6 全对\|4/4 PASS\|6/6 PASS" $BODY 2>/dev/null | grep -v "演进\|早期\|→\|升级\|第 [0-9]* 轮\|轮次")
[ -n "$M5" ] && { echo "[模式5 旧断言数] 需修（逐行定性，历史轮次记录里的 OK）："; echo "$M5"; }

# 用户自定义额外模式
for pat in "$@"; do
  M=$(grep -rn "$pat" $BODY 2>/dev/null)
  [ -n "$M" ] && { echo "[自定义 $pat] 命中："; echo "$M"; }
done

if [ $FAIL -eq 0 ]; then
  echo "=== ✓ 5 模式无裸写残留（全部带演进标注或 0 命中）==="
  exit 0
else
  echo "=== ★ 有裸写残留（见上方'需修'标记，逐处定性后修）==="
  exit 1
fi
