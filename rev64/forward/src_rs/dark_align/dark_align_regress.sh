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
SRCS="$RS/board.c $RS/layout.c $RS/rules.c $RS/save.c $RS/bt_protocol.c $RS/replay.c $RS/ui_state_stub.c"
CC=gcc
CFLAGS="-O2 -Wall -I$RS -DRS_NO_GL"

ONLY=""
for a in "$@"; do
  case "$a" in
    --only) shift; ONLY="$1";;
  esac
done

pass=0; fail=0; pending=0
run_one() {
  local name="$1"
  if [ -n "$ONLY" ] && ! echo ",$ONLY," | grep -q ",$name,"; then return 0; fi
  if [ ! -f "$DARK/$name.c" ]; then echo "  [MISS] $name（缺源文件）"; pending=$((pending+1)); return 0; fi
  local out rc
  if $CC $CFLAGS -o /tmp/opencode/da_$name "$DARK/$name.c" $SRCS -lm 2>/tmp/opencode/da_$name.cc.log; then
    out=$(/tmp/opencode/da_$name 2>&1); rc=$?
    # 判据：① 显式"行首 FAIL "（用例逐项输出 FAIL xxx）→ FAIL；② 退出码 0 → PASS。
    # 注：rule_text_conformance 末行"合计 PASS=33 FAIL=0"含 FAIL 字样但非行首 → 不触发①（误判修正）。
    if echo "$out" | grep -qE "^FAIL | FAIL 共"; then
      echo "  [FAIL] $name"; echo "$out" | grep -E "^FAIL|合计" | head -3; fail=$((fail+1))
    elif [ $rc -eq 0 ]; then
      echo "  [PASS] $name"; pass=$((pass+1))
    else
      echo "  [?] $name（rc=$rc 且无 PASS 标记）"; echo "$out" | head -4; fail=$((fail+1))
    fi
  else
    echo "  [CC] $name 编译失败"; grep -E "error" /tmp/opencode/da_$name.cc.log | head -3; fail=$((fail+1))
  fi
}

echo "=== 暗棋对齐桌面回归（dark_align，0 设备依赖）==="
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

echo ""
echo "=== 合计 PASS=$pass FAIL=$fail MISS=$pending ==="
if [ "$fail" -gt 0 ]; then
  echo "判定: FAIL（定位根因后重跑）"; exit 1
fi
echo "判定: PASS（桌面层 0 退化；真机层走 mumu_align_smoke.sh）"
