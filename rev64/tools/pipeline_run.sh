#!/bin/bash
# pipeline_run.sh — 军棋"逆向→正向→验收→补差异"流水线 orchestrator（PIPELINE.md 配套）
# 用法：
#   bash rev64/tools/pipeline_run.sh            # 跑能在本机跑通的段（段2回归+段3判据+回路C口径），输出流水线状态
#   bash rev64/tools/pipeline_run.sh --seg2      # 只跑段 2（正向开发回归 12/12 + 桌面 8/8）
#   bash rev64/tools/pipeline_run.sh --seg4check # 只跑段 4 前置检查（frida 工具在位 + 对象图缺口量化入口）
# 设计原则：每段"一条命令 + 一组判据 + 一个诚实边界"，失败项列出不伪装；真机段(段3屏幕/出声)只报"待真机"不误判 PASS。
set -u
WS=/disk/workspaces/jq
TOOLS=$WS/rev64/tools
RS=$WS/rev64/src_rs
PASS=0; FAIL=0; PENDING=0
ok()  { echo "  ✓ $1"; PASS=$((PASS+1)); }
bad() { echo "  ★ $1"; FAIL=$((FAIL+1)); }
pend(){ echo "  … $1（待真机 arm GPU，流水线不误判 PASS）"; PENDING=$((PENDING+1)); }

MODE="${1:---all}"

# ============ 段 1：逆向证据提取（静态资产在位性）============
seg1() {
  echo "=== 段 1 逆向证据提取（.rodata 明文 + PLT/dynsym/RELATIVE 固化）==="
  for f in frida-dump/rodata_dec.bin frida-dump/rodata_strings.txt; do
    [ -f "$WS/$f" ] && ok "$f 在位" || bad "$f 缺失"
  done
  [ -f "$WS/rev64/docs_rs/R-route_reversal.md" ] && ok "R-route_reversal.md（核心函数→规则/布局/常量证据链）" || bad "R-route_reversal.md 缺失"
  # C.5b 实测（不凭记忆）
  local so=$WS/rev64/dist/libjunqi_arm32_aligned.so
  if [ -f "$so" ]; then
    local plt=$(readelf -rW "$so" 2>/dev/null | grep -c JUMP_SLOT)
    local dsym=$(readelf -sW "$so" 2>/dev/null | awk 'NR>2 && /:/{n++} END{print n}')
    echo "  [C.5b] dist so $so：PLT=$plt dynsym=$dsym（随版本演进）"
  fi
}

# ============ 段 2：正向开发（src_rs 回归 12/12 + 桌面事件 8/8）============
seg2() {
  echo "=== 段 2 正向开发（src_rs 一条命令全回归）==="
  local out
  out=$(bash $WS/rev64/src_rs_regress.sh 2>&1)
  # 判据：末段"★ 全回归 PASS（12/12）"精确匹配（不模糊 grep FAIL，防误报）
  if echo "$out" | grep -q "全回归 PASS（12/12）"; then
    ok "src_rs_regress 12/12 全 PASS（编译/蓝牙/走局/规则回归/红线/布局/触摸/gametest/链接/走子预验证×2/桌面事件）"
  else
    bad "src_rs_regress 12/12 未全 PASS（跑 bash $WS/rev64/src_rs_regress.sh 看哪项挂）"
  fi
  local ev=$(cd $RS && make evverify MODE=--all 2>/dev/null | grep -oE '桌面事件验证: [0-9]+/[0-9]+ 全对')
  [ -n "$ev" ] && ok "$ev（桌面事件面）" || bad "evverify 桌面事件未跑通"
}

# ============ 段 3：真机验收（判据门，只报"待真机"不误判）============
seg3() {
  echo "=== 段 3 真机验收（能玩判据：装上→主界面→下一局→退出→重进不崩）==="
  # 本地能做的：真机全套脚本在位 + 判据口径
  [ -f "$WS/rev64/tools/mumu_fulltest.sh" ] && ok "mumu_fulltest.sh 在位（真机 9/9：走子 EAT+FLAG+MINE+重开+和棋+认输+困毙+UI 关闭/弹窗音效）" || bad "mumu_fulltest.sh 缺失"
  # 真机判定（需 MuMu/真机在线，离线只报 pending）
  if adb devices 2>/dev/null | grep -q "device$"; then
    echo "  [device online] 真机全套需 adb -s <dev> 触发，本 orchestrator 不自动跑（防 MuMu 触摸 flaky 误判）"
    pend "真机 9/9 全套（需 adb 在线 + mumu_fulltest.sh，触摸 flaky 由脚本多轮重试兜底）"
  else
    pend "真机 9/9 全套（当前离线，需 MuMu/真机 adb 在线）"
  fi
  pend "屏幕可见 + 音效出声 + 蓝牙（真机 arm GPU/音频验收，MuMu GLES1 受限替代不了）"
}

# ============ 段 4：差异补齐（frida/对象图前置检查，不自动跑重活）============
seg4() {
  echo "=== 段 4 差异补齐（unidbg/frida 喂对象图真值）前置检查 ==="
  [ -f "$TOOLS/frida_dump_objects.py" ] && ok "frida_dump_objects.py 在位（抓对象图真值）" || bad "frida_dump_objects.py 缺失"
  [ -f "$TOOLS/gen_shadow.py" ] && ok "gen_shadow.py 在位（指令翻译→C 影子运行时）" || bad "gen_shadow.py 缺失"
  [ -f "$TOOLS/tex_pipeline.py" ] && ok "tex_pipeline.py 在位（closure <addr> 量化对象图缺口）" || bad "tex_pipeline.py 缺失"
  # 诚实边界：对象图数据填充 + 真机 GPU 是段 4 退出条件，非"函数全翻译"
  pend "对象图数据填充（std::list node next / 场景对象 vtable / 字段）——补 stub 破不了，需真实 C++ 构造链 + 真机 arm GPU"
}

# ============ 回路 C：口径防漂移（双脚本）============
loopc() {
  echo "=== 回路 C 口径防漂移（check_consistency + course_sync_scan 双脚本）==="
  if bash $TOOLS/check_consistency.sh > /dev/null 2>&1; then ok "check_consistency.sh EXIT=0（AGENTS↔课程 N/M 口径全一致）"; else bad "check_consistency.sh 有漂移（跑 bash $TOOLS/check_consistency.sh 看差）"; fi
  if bash $TOOLS/course_sync_scan.sh > /dev/null 2>&1; then ok "course_sync_scan.sh EXIT=0（31 章正文 5 模式无裸写残留）"; else bad "course_sync_scan.sh 有旧口径残留（跑脚本看列差）"; fi
}

case "$MODE" in
  --seg1) seg1 ;;
  --seg2) seg2 ;;
  --seg3) seg3 ;;
  --seg4check|--) seg4 ;;
  --loopc) loopc ;;
  --all|*) seg1; seg2; seg3; seg4; loopc ;;
esac

echo ""
echo "=== 流水线状态汇总（PIPELINE.md 段 1-4 + 回路 C）==="
echo "  ✓ PASS=$PASS   ★ FAIL=$FAIL   … PENDING=$PENDING（待真机，不计入失败）"
[ $FAIL -eq 0 ] && echo "=== 本机可跑段全 PASS（段 3/4 真机部分 = PENDING 非失败）===" && exit 0
echo "=== 有 FAIL（见上方 ★ 标记）===" && exit 1
