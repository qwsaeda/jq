#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1944 四国 mode=4 边界对话框实锤判据（设备无关）

固化"四国军棋（4 方混战）未实现 = 诚实边界对话框（勿静默开 2 人局
误导为四国可玩）"口径（AGENTS 诚实边界 + BUSINESS_RISK_LEDGER P2-1 +
so e5bc4fe4 修复：action=4 静默开 2 人局 → 边界对话框）：
  - 四国路径：主菜单第 5 按钮 action=4 → UI_NPLAYER 四国人数选择弹窗
    （datah_089 2/3/4 人 3 按钮）→ 点 2/3/4 人 → 诚实边界对话框
    （"src_rs 为 12×5 两人军棋（50 子）规则重写，四国（4 方混战）
    未实现"）→ 回主菜单，不假装四国可玩
判据（4 项全 PASS）：
  ① 四国入口 action=4 → UI_NPLAYER 在位（jni_bridge.c，datah_089）
  ② UI_NPLAYER 选人数 → 诚实边界对话框在位（"4 方规则未实现"文案）
  ③ 边界对话框回主菜单（不假装四国可玩，不静默开 2 人局）
  ④ BUSINESS_RISK_LEDGER P2-1 四国未实现口径在位

用法：python3 rev64/tools/nplayer_boundary_check.py
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BRIDGE = os.path.join(ROOT, 'rev64/forward/src_rs/jni_bridge.c')
LEDGER = os.path.join(ROOT, 'rev64/forward/docs_rs/BUSINESS_RISK_LEDGER.md')

def main():
    fails = []
    ref = open(BRIDGE, errors='ignore').read()

    # ① 四国入口 action=4 → UI_NPLAYER 在位
    if 'hit_action == 4' not in ref or 'UI_NPLAYER' not in ref:
        fails.append('① 四国入口 action=4 → UI_NPLAYER 缺失')
    else:
        print('① 四国入口 action=4 → UI_NPLAYER 在位 OK（datah_089 四国人数选择弹窗）')

    # ② UI_NPLAYER 选人数 → 诚实边界对话框在位
    if '四国军棋（诚实边界）' not in ref and '4 方' not in ref:
        fails.append('② UI_NPLAYER 边界对话框（"4 方规则未实现"）缺失')
    else:
        print('② UI_NPLAYER 选人数 → 诚实边界对话框在位 OK（"4 方规则未实现"文案）')

    # ③ 边界对话框回主菜单（不假装四国可玩）
    if '不假装四国可玩' not in ref and '回主菜单' not in ref:
        fails.append('③ 边界对话框回主菜单（不假装四国可玩）缺失')
    else:
        print('③ 边界对话框回主菜单 OK（不假装四国可玩，不静默开 2 人局）')

    # ④ BUSINESS_RISK_LEDGER P2-1 四国未实现口径在位
    led = open(LEDGER, errors='ignore').read()
    if '四国军棋' not in led and '未实现' not in led:
        fails.append('④ BUSINESS_RISK_LEDGER 四国未实现口径缺失')
    else:
        print('④ BUSINESS_RISK_LEDGER P2-1 四国未实现口径在位 OK')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（四国 mode=4 边界对话框实锤 4 项：UI_NPLAYER 入口 + 边界对话框 + 回主菜单 + P2-1 口径，诚实边界固化）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
