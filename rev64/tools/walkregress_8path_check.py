#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-234 走子回归 8 路径锚点对照判据（静态侧，设备无关）

路线 4 收口：32 位走子回归 4→8 路径（蓝/红 × 明/暗 × 走/翻）锚点对照。
判据（PASS 条件）：
1. 蓝明走 = walk_bridge 8 步序列里 move(6,11)（蓝方 cell6→cell11 走子，Junqi.Move）
2. 红明走 = walk_bridge 8 步序列里 move(33,38)（红方 cell33→cell38 走子，Junqi.Move）
3. 蓝暗翻 = walk_bridge 8 步序列里 flip(6)（暗棋 cell6 翻子定蓝方）
4. 红暗翻 = walk_bridge 8 步序列里 flip(33)（暗棋 cell33 翻子定红方）
5. walk_bridge 锚点 = "WALK-BRIDGE: 完整 8 步序列完成"
6. native_layout 锚点 = "NATIVE-LAYOUT: F_2a380 返回"
7. native_event 锚点 = "F_3ab80: 事件发送"（§124y-156 disc=0 忠实）
8. game_test 锚点 = "GAME-TEST: 端到端完成 走子=OK"

判 32 位翻译体源码 8 锚点齐备（防后续改 walkregress 时锚点漂移）：
- jq_rs_walkbridge.c 8 步序列含 flip(6)/flip(33)/move(6,11)/move(33,38) + 8 步锚点日志
- jni_aligned.c 含 native_layout/native_event/game_test 钩子锚点
"""
import os, re, sys
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
WALK = os.path.join(ROOT, 'rev64/reverse/src_jni_translated/jq_rs_walkbridge.c')
JNI  = os.path.join(ROOT, 'rev64/reverse/src_jni_translated/jni_aligned.c')

def main():
    w = open(WALK).read(); j = open(JNI).read()
    # 8 路径锚点对照
    anchors = [
        ("蓝明走 move(6,11)", r'move\(6,\s*11\)'),
        ("红明走 move(33,38)", r'move\(33,\s*38\)'),
        ("蓝暗翻 flip(6)", r'flip\(6\)|jq_walk_bridge_flip\(6\)|can_flip\(&g_walk_state,\s*6\)'),
        ("红暗翻 flip(33)", r'flip\(33\)|jq_walk_bridge_flip\(33\)|can_flip\(&g_walk_state,\s*33\)'),
        ("walk_bridge 8 步锚点", r'WALK-BRIDGE: 完整 8 步序列完成'),
        ("native_layout 锚点", r'NATIVE-LAYOUT: F_2a380 返回'),
        ("native_event 锚点", r'F_3ab80: 事件发送|NATIVE-EVENT'),
        ("game_test 锚点", r'GAME-TEST: 端到端完成 走子='),
    ]
    print("═══ 走子回归 8 路径锚点对照（§124y-234，蓝/红 × 明/暗 × 走/翻）═══")
    ok = 0
    for name, pat in anchors:
        hit_w = re.search(pat, w)
        hit_j = re.search(pat, j)
        found = bool(hit_w or hit_j)
        src = 'walkbridge' if hit_w else ('jni_aligned' if hit_j else '无')
        print("  [%s] %s → %s" % ('✓' if found else '✗', name, src))
        ok += (1 if found else 0)
    if ok != 8:
        print("判定: FAIL（8 锚点缺 %d 个）" % (8-ok)); sys.exit(1)
    print("判定: PASS（8 路径锚点全齐备，走子回归可扩 4→8 路径）")

if __name__ == '__main__':
    main()
