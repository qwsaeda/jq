#!/bin/bash
# dark_align_regress.sh — 暗棋对齐验证一键方案（桌面层，0 设备依赖）
# 固化：本轮散落在 /tmp/opencode 的 9 个暗棋对齐用例 + 规则原文 33 项机判。
# 改一次规则层（rules.c/jni_bridge.c）→ 跑一次本脚本，0 退化才允许推进。
# 用法：
#   bash rev64/forward/src_rs/dark_align/dark_align_regress.sh            # 全跑
#   bash ... dark_align_regress.sh --only flag,mine,flip                 # 子集
# 判据：
#   - 每个用例自判 PASS/FAIL（退出码 0=PASS）；汇总 PASS/FAIL。
#   - rule_text_conformance = 规则原文逐条机判（33 项，原版 SPAK 规则文本权威）。
#   - 全 PASS 才允许推进；任一 FAIL 定位根因（日志+几何对照），修完重跑。
# 诚实边界：桌面层 0 设备依赖；真机层（走子交互/音效/完整对弈）走
#   mumu_align_smoke.sh（设备在则实锤，offline 降级 PENDING 非 FAIL，§124y-189 口径）。
set -u
WS=/disk/workspaces/jq
RS=$WS/rev64/forward/src_rs
DARK=$RS/dark_align
SRCS="$RS/board.c $RS/layout.c $RS/rules.c $RS/save.c $RS/bt_protocol.c $RS/replay.c $RS/ui_state_stub.c $RS/touch_state.c $RS/touch.c"
CC=gcc
CFLAGS="-O2 -Wall -I$RS -DRS_NO_GL"

ONLY=""
for a in "$@"; do
  case "$a" in
    --only) shift; ONLY="$1";;
  esac
done

pass=0; fail=0; pending=0

# ★ Round 1 对抗审计 P0（§104 假绿三型之"判据恒真/盲区"）——判据自检 meta 门禁：
#   旧判据 = `grep -qE "^FAIL | FAIL 共"` + 退出码 0。Round 1 实锤两条假绿路径：
#   ① FAIL 打在**行尾**（`printf("... → %s\n", ok?"PASS":"FAIL")`）→ 行首匹配漏判；
#   ② main 尾部**无条件 return 0**（9 个零断言观测脚本同款）→ 退出码恒 0 兜底失效。
#   实锤判据（合成失败样例 → 旧判据判 PASS = 假绿）。
#   现判据：① 退出码 != 0 → FAIL；② 输出含 "FAIL" 且非汇总行（"FAIL=0" 剥离后仍有
#   FAIL）→ FAIL；两条任一即失败（行首/行尾 FAIL 全覆盖 + 退出码兜底）。
judge_case() {   # $1=用例输出 $2=退出码 → 打印 PASS/FAIL
  local out="$1" rc="$2"
  if [ "$rc" -ne 0 ]; then echo "FAIL"; return; fi
  if echo "$out" | sed 's/FAIL=0//g' | grep -q "FAIL"; then echo "FAIL"; return; fi
  echo "PASS"
}

# 判据自检：合成「行尾 FAIL + return 0」失败样例必须被判 FAIL，
# 「全 PASS + return 0」样例必须被判 PASS——判据自身回归测试（防复发）。
self_test_judge() {
  local t_src=/tmp/opencode/.judge_selftest.c t_bin=/tmp/opencode/.judge_selftest
  cat > "$t_src" <<'EOF'
#include <stdio.h>
int main(void){
#ifdef FAILCASE
    printf("① 行尾失败标记: got=0 want=1 → %s\n", "FAIL");
#else
    printf("① 全通过: got=1 want=1 → %s\n", "PASS");
#endif
    return 0;
}
EOF
  local rc1 rc2 out1 out2 v1 v2
  $CC -O2 -o "$t_bin" "$t_src" 2>/dev/null || { echo "  [JUDGE] 自检编译失败"; return 1; }
  out1=$($t_bin 2>&1); rc1=$?; v1=$(judge_case "$out1" "$rc1")
  $CC -O2 -DFAILCASE -o "$t_bin" "$t_src" 2>/dev/null || { echo "  [JUDGE] 自检编译失败"; return 1; }
  out2=$($t_bin 2>&1); rc2=$?; v2=$(judge_case "$out2" "$rc2")
  if [ "$v1" = "PASS" ] && [ "$v2" = "FAIL" ]; then
    echo "  [JUDGE] 判据自检 PASS（行尾 FAIL 必被抓 + 全 PASS 不误判）"; return 0
  fi
  echo "  [JUDGE] ★判据自检 FAIL★（通过样例判=$v1，失败样例判=$v2）——门禁本身不可信"; return 1
}

run_one() {
  local name="$1"
  if [ -n "$ONLY" ] && ! echo ",$ONLY," | grep -q ",$name,"; then return 0; fi
  if [ ! -f "$DARK/$name.c" ]; then echo "  [MISS] $name（缺源文件）"; pending=$((pending+1)); return 0; fi
  local out rc verdict
  if $CC $CFLAGS -o /tmp/opencode/da_$name "$DARK/$name.c" $SRCS -lm 2>/tmp/opencode/da_$name.cc.log; then
    out=$(/tmp/opencode/da_$name 2>&1); rc=$?
    verdict=$(judge_case "$out" "$rc")
    if [ "$verdict" = "FAIL" ]; then
      echo "  [FAIL] $name"; echo "$out" | grep -E "FAIL" | grep -v "FAIL=0" | head -3; fail=$((fail+1))
    else
      echo "  [PASS] $name"; pass=$((pass+1))
    fi
  else
    echo "  [CC] $name 编译失败"; grep -E "error" /tmp/opencode/da_$name.cc.log | head -3; fail=$((fail+1))
  fi
}

echo "=== 暗棋对齐桌面回归（dark_align，0 设备依赖）==="
# ★ 判据自检 meta 门禁（先跑：判据自身不可信则后续 PASS/FAIL 无意义）
if ! self_test_judge; then
  echo "判定: FAIL（判据自检未过——门禁本身失效，先修判据）"; exit 1
fi
# 规则原文机判（权威，33 项）
run_one rule_text_conformance
# 暗棋专项（翻子/终局/挖雷/扛旗/全对局/可达格）
run_one dark_flip_test
run_one dark_win_test
run_one dark_flag_test
run_one dark_mine_test
run_one dark_fullgame
run_one flip_eat_test
run_one flip_rail_test
run_one flip_draw_guard
run_one draw_exist_test
run_one eat_dir_test
run_one rail_block_test
run_one layout_check_test
run_one rank_order_test
run_one dark_bomb_test
run_one dark_flag_win_test
run_one dark_mine_flag_test
run_one dark_fullgame_flip_end
run_one dark_flip_draw_count_test
run_one dark_eat_reset_count_test
run_one dark_undo_count_test
run_one dark_rule_switch_test
run_one dark_tongji_flip_test
run_one dark_ai_score_test
run_one dark_rail_topo_test
run_one dark_camp_safety_test
run_one dark_hq_freeze_test
run_one dark_bfs_thru_test
run_one dark_base_empty_test
run_one dark_rail_walk_test
run_one dark_moves_seq_test
run_one dark_rail_cross_test
run_one dark_camp_special_test
run_one dark_flip_island_test
run_one dark_cross_switch_test
run_one dark_4switch_combo_test
run_one dark_02_switch_test
run_one dark_stalemate_matrix_test
run_one dark_flagoff_stalemate_test
run_one dark_mineoff_stalemate_test
run_one dark_darkoff_matrix_test
run_one dark_flip_color_test
run_one dark_fullgame_reset_test
run_one dark_colorwalk_test
run_one dark_fullgame_statemachine_test
run_one dark_multi_switch_matrix_test
run_one dark_allclose_switch_test
run_one dark_mixed_switch_test
run_one dark_fullgame_ms_track_test
run_one dark_fullgame_ai_ms_track_test
run_one dark_fullgame_win_ms_track_test
run_one dark_fullgame_redwin_ms_track_test
run_one dark_32combo_matrix_test
run_one dark_4branch_clear_test
run_one dark_flagoff_clear_test
run_one dark_flagoff_cont_test
run_one dark_w1off_cont_test
run_one dark_w1off_multi_test
run_one dark_fullgame_fullon_ms_track_test
run_one dark_flip32_combo_test
run_one dark_bfs_around_test
run_one dark_straight_rail_test
run_one dark_redwin_ai_test
run_one dark_cross_rail_test
run_one dark_rail_segment_test
run_one dark_rail_seg8_test
run_one dark_rail_fullseg_test
run_one dark_wrap_across_test
run_one dark_rail_corner_nonrail_test

run_one dark_rail_corner11_test
run_one dark_rail_cross_eat_test
run_one dark_rail_cross_flagmine_test
run_one dark_rail_c4corner_cross_eat_test
run_one dark_rail_corner11_c4_test
run_one dark_rail_c1c4cross_test
run_one dark_rail_r6cross_test
run_one dark_rail_r0r11_ends_test
run_one dark_rail_topology_matrix_test
run_one dark_rail_r5r6_fullrow_test
run_one dark_rail_c2_fullseg_test
run_one dark_rail_c2_worker_seg_test
run_one dark_rail_c0c4_worker_seg_test
run_one dark_rail_c1c3_worker_seg_test
run_one dark_rail_r6_fullrow_worker_test
run_one dark_rail_r5r6_worker_eat_test
run_one dark_touch_flip_flow_test


echo ""
echo "=== 合计 PASS=$pass FAIL=$fail MISS=$pending ==="
if [ "$fail" -gt 0 ]; then
  echo "判定: FAIL（定位根因后重跑）"; exit 1
fi
echo "判定: PASS（桌面层 0 退化；真机层走 mumu_align_smoke.sh）"
