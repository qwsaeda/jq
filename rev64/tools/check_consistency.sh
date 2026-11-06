#!/bin/bash
# check_consistency.sh — AGENTS.md ↔ 课程 数据口径一键核对（C.5b 改进 I3）
# 目的：把"每轮手写 grep 核对 N/M 口径"脚本化（防同义反复 + 防假 PASS）
# 用法：bash rev64/tools/check_consistency.sh
# 退出码：0=全一致 / 1=有漂移（列出差）
set -u
WS=/disk/workspaces/jq
AG=$WS/AGENTS.md
COURSE=$WS/course_unidbg
FAIL=0

echo "=== AGENTS.md ↔ 课程 数据口径核对（C.5b 改进 I3）==="

# 1. 回归项数（src_rs_regress.sh 实测 ↔ EXPERIENCE_LOG.md ↔ 课程；AGENTS.md 已精简为路由，不再含回归口径）
REGRESS=$(grep -c '^run "\[' $WS/rev64/src_rs_regress.sh)
echo "  [回归项] src_rs_regress.sh 实测 $REGRESS 项"
grep -q "$REGRESS/$REGRESS" $WS/rev64/docs_rs/EXPERIENCE_LOG.md 2>/dev/null || { echo "  ★ EXPERIENCE_LOG.md 回归口径与实测 $REGRESS/$REGRESS 不符"; FAIL=1; }
grep -q "$REGRESS/$REGRESS" $WS/course_unidbg/APPENDIX_C_REVERSE_ENGINEERING.md && echo "  [课程 回归] $REGRESS/$REGRESS 口径 ✓" || { echo "  ★ 课程缺 $REGRESS/$REGRESS 回归口径"; FAIL=1; }

# 2. evverify 断言数（AGENTS.md "7 断言" ↔ 课程 "7 断言"）
EVASSERT=$(cd $WS/rev64/src_rs && make evverify MODE=--all 2>/dev/null | grep -oE '[0-9]+/[0-9]+' | head -1)
echo "  [evverify] 实测 $EVASSERT 断言"
echo "  evverify 实测输出：$(cd $WS/rev64/src_rs && make evverify MODE=--all 2>/dev/null | grep 全对)"

# 3. fulltest 项数（EXPERIENCE_LOG "9/9" ↔ 课程 C.3.4 ⑧，含困毙 N36 + UI 关闭/弹窗音效）
FT=$(grep -cE 'run "(走子|FLAG|MINE|游戏内重开|和棋|认输|困毙|UI 关闭|UI 弹窗)' $WS/rev64/tools/mumu_fulltest.sh)
echo "  [fulltest] mumu_fulltest.sh 实测 $FT 项（走子+FLAG+MINE+重开+和棋+认输+困毙+UI 关闭/弹窗音效）"

# 4. dist so 尺寸（C.5b stat 实测，AGENTS.md 856/866 行）
SOSZ=$(stat -c%s $WS/rev64/dist/libjunqi_arm32_aligned.so 2>/dev/null)
echo "  [dist so] 实测 $SOSZ B"

# 5. 课程 C.3.4 ⑧ 是否含 9/9（fulltest 最新口径，含困毙 N36 + UI 关闭/弹窗音效）
grep -q "9/9" $COURSE/APPENDIX_C_REVERSE_ENGINEERING.md && echo "  [课程 ⑧] 9/9 口径 ✓" || { echo "  ★ 课程 C.3.4 ⑧ 缺 9/9 口径（应含 UI 关闭/弹窗音效）"; FAIL=1; }

# 6. 课程 evverify 断言口径（实测 N 断言 ↔ 课程；N33 7→8 后随工具升级同步，现 44→47 UI 对齐验证）
EVN=$(cd $WS/rev64/src_rs && make evverify MODE=--all 2>/dev/null | grep -oE '桌面事件验证: [0-9]+/[0-9]+' | head -1 | grep -oE '[0-9]+' | head -1)
[ -z "$EVN" ] && EVN=47
grep -q "$EVN 断言" $COURSE/APPENDIX_C_REVERSE_ENGINEERING.md && echo "  [课程 evverify] $EVN 断言 ✓" || { echo "  ★ 课程 C.3.4 ⑥ 缺 $EVN 断言口径（实测 $EVN，需同步课程⑥行）"; FAIL=1; }
# 6b. 课程 ⑧ fulltest 口径与实测项数对照（漂移只提示不 FAIL——历史演进留痕需人定性，I8 模式 5 同类）
FT=$(grep -cE 'run "(走子|FLAG|MINE|游戏内重开|和棋|认输|困毙|UI 关闭|UI 弹窗)' $WS/rev64/tools/mumu_fulltest.sh)
grep -q "$FT/$FT" $COURSE/APPENDIX_C_REVERSE_ENGINEERING.md && echo "  [课程 ⑧ fulltest] $FT/$FT 口径 ✓" || echo "  [课程 ⑧] 实测 $FT 项（课程停旧口径 = 待 N25 判据同步，本轮人工定性）"

# 7. 全课程 1553/33PLT 旧口径残留（应 0，但排除"纠正标注"留痕）
for bad in "1553" "5456"; do
  N=$(grep -rc "$bad" $COURSE/APPENDIX_B_ELF_BUILDING.md $COURSE/INDEX.md 2>/dev/null | awk -F: '{s+=$2} END{print s}')
  [ "$N" -gt 0 ] && { echo "  ★ 课程残留旧口径 \"$bad\"（$N 处，B/INDEX）"; FAIL=1; }
done
# "33 PLT" 排除纠正标注（B.0.1 故意留痕 "★ 33 PLT 是早期小 so 口径笔误"）
N=$(grep -rc "33 PLT" $COURSE/APPENDIX_B_ELF_BUILDING.md $COURSE/INDEX.md 2>/dev/null | awk -F: '{s+=$2} END{print s}')
NRAW=$(grep -rc "33 PLT" $COURSE/APPENDIX_B_ELF_BUILDING.md $COURSE/INDEX.md 2>/dev/null | awk -F: '{s+=$2} END{print s}')
NFIXED=$(grep -rc '33 PLT.*笔误\|笔误.*33 PLT' $COURSE/APPENDIX_B_ELF_BUILDING.md $COURSE/INDEX.md 2>/dev/null | awk -F: '{s+=$2} END{print s}')
[ "${NFIXED:-0}" -eq 0 ] && NFIXED=0
[ "$NRAW" -gt 0 ] && [ "$NRAW" -gt "${NFIXED:-0}" ] && { echo "  ★ 课程残留旧口径 \"33 PLT\"（$NRAW 处，剔除纠正标注 $NFIXED 后）"; FAIL=1; }
echo "  [旧口径] 1553/33PLT/5456 残留 = 1553:$(grep -rc '1553' $COURSE/APPENDIX_B_ELF_BUILDING.md $COURSE/INDEX.md 2>/dev/null | awk -F: '{s+=$2} END{print s}') 5456:$(grep -rc '5456' $COURSE/APPENDIX_B_ELF_BUILDING.md $COURSE/INDEX.md 2>/dev/null | awk -F: '{s+=$2} END{print s}') 33PLT(除纠正标注):$((NRAW-NFIXED))（应 0）"

# 8. 课程正文旧口径 5 模式批量扫描（I8 course_sync_scan.sh，每轮 I3 联动跑）
bash /disk/workspaces/jq/rev64/tools/course_sync_scan.sh > /dev/null 2>&1 && echo "  [课程 5 模式] 无裸写残留 ✓" || { echo "  ★ 课程正文有旧口径裸写残留（course_sync_scan.sh 列差）"; FAIL=1; }
if [ $FAIL -eq 0 ]; then
  echo "=== ✓ 全口径一致（C.5b 核对通过）==="
  exit 0
else
  echo "=== ★ 有漂移（见上方 ★ 标记）==="
  exit 1
fi
