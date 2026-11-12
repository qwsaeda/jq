#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-236 路线 2 暗棋翻子规则 32 位实证判据（静态侧，桌面级，设备无关）

路线 2 收口：暗棋翻子（先手定色 / 己方可翻 / 敌方不可翻 / 军旗不可翻 /
已翻不可再翻 / 终局不可翻）6 类规则 32 位翻译体 jq_can_flip/jq_flip 行为
实证。判据 = 编译 32 位 jq_rs_rules.c + 构造暗棋 state 跑 8 场景 → 逐场景
机判（与 64 位 src_rs 行为对照，AUD-R-003/008 分叉矩阵已知项）。

8 场景（暗棋布局 side=0xFF 未定 + revealed=0）：
  S1 先手蓝 turn=0 翻 cell6 → ok=1 + 定色 side=BLUE(0) + revealed=1
  S2 翻后 turn 切红(1) + cell6 不可再翻（已翻 revealed=1）
  S3 红方 cell33 未翻 → 红回合(turn=1) 可翻 → ok=1 + 定色 RED(1)
  S4 红方翻敌方暗子（turn=1 翻 cell12 蓝已定色暗子）→ ok=0（敌方不可翻）
  S5 军旗（RANK_FLAG）不可翻 → ok=0
  S6 空格（RANK_EMPTY）不可翻 → ok=0
  S7 终局（stage=OVER）不可翻 → ok=0
  S8 和棋累加（moves_since_eat 达 70 → jq_on_draw 触发）

用法：python3 rev64/tools/dark_flip_probe_check.py
（编译 32 位 jq_rs_rules.c + 临时 main，桌面跑 8 场景机判）
"""
import os, re, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC_DIR = os.path.join(ROOT, "rev64/reverse/src_jni_translated")
CC = os.environ.get("CC", "gcc")

# 临时 main：构造暗棋 state + 8 场景，机判每场景返回码
PROBE_SRC = r'''
#include <stdio.h>
#include <stdlib.h>
#include "jq_rs_rules.h"

int main(void) {
    int pass = 0, fail = 0;
    /* 暗棋布局：全 cell side=0xFF revealed=0 rank 混布 */
    jq_state st;
    jq_state_init(&st);
    st.stage = STG_PLAY; st.turn = 0;
    st.moves_since_eat = 0; st.total_moves = 0;
    /* cell6 师长 暗（side=0xFF revealed=0 rank=3 师长） */
    st.cells[6].rank = 3; st.cells[6].side = 0xFF; st.cells[6].revealed = 0;
    /* cell33 连长 暗 */
    st.cells[33].rank = 2; st.cells[33].side = 0xFF; st.cells[33].revealed = 0;
    /* cell12 蓝已定色暗子（revealed=0 side=BLUE） */
    st.cells[12].rank = 1; st.cells[12].side = 0; st.cells[12].revealed = 0;
    /* cell20 军旗 */
    st.cells[20].rank = RANK_FLAG; st.cells[20].side = 0; st.cells[20].revealed = 0;

    /* S1 先手蓝 flip cell6 */
    int r = jq_can_flip(&st, 6);
    if (r) { jq_flip(&st, 6); }
    int ok = jq_can_flip(&st, 6); /* flip 后 revealed=1，can_flip 应 false */
    /* S1 判：flip 前 can=1，flip 后 side 定 0 revealed=1 turn 切 1 */
    if (st.cells[6].side == 0 && st.cells[6].revealed == 1 && st.turn == 1) { printf("S1 PASS\n"); pass++; }
    else { printf("S1 FAIL(side=%d rev=%d turn=%d)\n", st.cells[6].side, st.cells[6].revealed, st.turn); fail++; }

    /* S2 已翻不可再翻 */
    r = jq_can_flip(&st, 6);
    if (r == 0) { printf("S2 PASS\n"); pass++; } else { printf("S2 FAIL(can=%d)\n", r); fail++; }

    /* S3 红 turn=1 flip cell33（红回合 可翻红己方暗子? cell33 side=0xFF 未定色）
       jq_can_flip 对 side=0xFF 恒 true（暗棋未定子可翻，先手定色） */
    r = jq_can_flip(&st, 33);
    int c33 = r;
    if (c33) jq_flip(&st, 33);
    /* S3 判：turn=1 时 cell33 0xFF 可翻（未定子）→ flip 后定色 RED(1) */
    if (st.cells[33].side == 1 && st.cells[33].revealed == 1 && st.turn == 0) { printf("S3 PASS\n"); pass++; }
    else { printf("S3 FAIL(side=%d rev=%d turn=%d)\n", st.cells[33].side, st.cells[33].revealed, st.turn); fail++; }

    /* S4 红翻敌方已定色暗子 cell12（side=0 BLUE，turn=0 蓝回合）
       jq_can_flip：side=0 != turn=0? turn 现为 0（S3 flip 后切回蓝），
       side==turn(0==0) → 可翻（蓝己方暗子）。构造敌方场景：
       再 flip 使 turn=1（红），则 cell12 side=0 != turn=1 → 不可翻 */
    if (st.turn == 0) { /* 翻一个红子让 turn 切红 */
        st.cells[30].rank = 1; st.cells[30].side = 0xFF; st.cells[30].revealed = 0;
        jq_flip(&st, 30); /* 定 RED, turn->1 */
    }
    r = jq_can_flip(&st, 12);
    /* turn=1 RED, cell12 side=0 BLUE ≠ turn → 敌方不可翻 */
    if (r == 0) { printf("S4 PASS\n"); pass++; } else { printf("S4 FAIL(can=%d, 敌方暗子应不可翻)\n", r); fail++; }

    /* S5 军旗不可翻 */
    r = jq_can_flip(&st, 20);
    if (r == 0) { printf("S5 PASS\n"); pass++; } else { printf("S5 FAIL(军旗 can=%d)\n", r); fail++; }

    /* S6 空格不可翻 */
    r = jq_can_flip(&st, 40); /* 未布子 cell */
    if (r == 0) { printf("S6 PASS\n"); pass++; } else { printf("S6 FAIL(空格 can=%d)\n", r); fail++; }

    /* S7 终局不可翻 */
    st.stage = STG_OVER;
    r = jq_can_flip(&st, 33);
    if (r == 0) { printf("S7 PASS\n"); pass++; } else { printf("S7 FAIL(终局 can=%d)\n", r); fail++; }

    /* S8 和棋累加（翻到 moves_since_eat=70 触发 jq_on_draw） */
    st.stage = STG_PLAY; st.moves_since_eat = 69; st.turn = 0;
    st.cells[5].rank = 1; st.cells[5].side = 0xFF; st.cells[5].revealed = 0;
    if (jq_can_flip(&st, 5)) jq_flip(&st, 5);
    /* flip 后 moves_since_eat=70 → jq_on_draw → stage=OVER */
    if (st.stage == STG_OVER) { printf("S8 PASS\n"); pass++; } else { printf("S8 FAIL(stage=%d 未触发和棋)\n", st.stage); fail++; }

    printf("暗棋翻子 8 场景: %d PASS / %d FAIL\n", pass, fail);
    return fail ? 1 : 0;
}
'''

def main():
    for f in ("jq_rs_rules.c", "jq_rs_rules.h", "jq_rs_board.c", "jq_rs_layout.c"):
        if not os.path.exists(os.path.join(SRC_DIR, f)):
            print("✗ 缺 %s" % f); sys.exit(2)
    with tempfile.TemporaryDirectory() as td:
        probe = os.path.join(td, "dark_probe.c")
        open(probe, "w").write(PROBE_SRC)
        out = os.path.join(td, "dark_probe")
        cmd = [CC, "-I", SRC_DIR, "-I", os.path.join(SRC_DIR, "..", "src"),
               probe, os.path.join(SRC_DIR, "jq_rs_rules.c"),
               os.path.join(SRC_DIR, "jq_rs_board.c"),
               os.path.join(SRC_DIR, "jq_rs_layout.c"),
               "-o", out]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode:
            print("✗ 编译失败：\n%s" % r.stderr[-800:]); sys.exit(2)
        r = subprocess.run([out], capture_output=True, text=True)
        print(r.stdout)
        if r.returncode:
            print("判定: FAIL（暗棋 8 场景未全 PASS）"); sys.exit(1)
        print("判定: PASS（32 位暗棋翻子规则 8 场景实证，路线 2 静态收口）")

if __name__ == "__main__":
    main()
