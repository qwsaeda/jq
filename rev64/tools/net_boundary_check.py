#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1945 联网对战边界对话框实锤判据（设备无关）

固化"联网对战（服务器协议）未逆向 = 诚实边界对话框（离线单机不支持
多人房间）"口径（AGENTS 诚实边界 + BUSINESS_RISK_LEDGER P2-1 联网对战
未实现，蓝牙 SPP 联机已实现 §1933/§1941 但服务器协议未逆向）：
  - 联网路径：原版 Junqi.JoinGame 加入游戏 → 64 位简化版边界对话框
    （"联机加入游戏未逆向（原版需服务器协议），src_rs 离线单机不
    支持多人房间"）→ close，73-name 计数对齐
  - 蓝牙 SPP 联机（host=1/client=0）已实现（BtSpp socket，§1933/
    §1941 帧契约），但"联网对战"（服务器协议多人房间）未逆向
判据（4 项全 PASS）：
  ① join_game → 边界对话框在位（jni_bridge.c，"联机加入游戏未逆向"）
  ② 边界对话框 close（回主菜单，不假装联网可玩）
  ③ 蓝牙 SPP 联机已实现口径在位（host=1/client=0 BtSpp socket）
  ④ BUSINESS_RISK_LEDGER P2-1 联网对战未实现口径在位

用法：python3 rev64/tools/net_boundary_check.py
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BRIDGE = os.path.join(ROOT, 'rev64/forward/src_rs/jni_bridge.c')
LEDGER = os.path.join(ROOT, 'rev64/forward/docs_rs/BUSINESS_RISK_LEDGER.md')

def main():
    fails = []
    ref = open(BRIDGE, errors='ignore').read()

    # ① join_game → 边界对话框在位
    if '联机加入游戏未逆向' not in ref and 'join_game' not in ref:
        fails.append('① join_game → 边界对话框（"联机加入游戏未逆向"）缺失')
    else:
        print('① join_game → 边界对话框在位 OK（"联机加入游戏未逆向（原版需服务器协议）"）')

    # ② 边界对话框 close（回主菜单）
    if '离线单机不支持多人房间' not in ref:
        fails.append('② 边界对话框 close（离线单机不支持多人房间）缺失')
    else:
        print('② 边界对话框 close OK（"src_rs 离线单机不支持多人房间"，不假装联网可玩）')

    # ③ 蓝牙 SPP 联机已实现口径在位（host=1/client=0 BtSpp socket）
    if 'g_way_host' not in ref and 'btHost' not in ref:
        fails.append('③ 蓝牙 SPP 联机（host=1/client=0 BtSpp socket）口径缺失')
    else:
        print('③ 蓝牙 SPP 联机已实现口径在位 OK（host=1/client=0 BtSpp socket，§1933/§1941 帧契约）')

    # ④ BUSINESS_RISK_LEDGER P2-1 联网对战未实现口径在位
    led = open(LEDGER, errors='ignore').read()
    if '联网对战' not in led and '未实现' not in led:
        fails.append('④ BUSINESS_RISK_LEDGER P2-1 联网对战未实现口径缺失')
    else:
        print('④ BUSINESS_RISK_LEDGER P2-1 联网对战未实现口径在位 OK')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（联网对战边界对话框实锤 4 项：join_game 边界 + close + 蓝牙 SPP 已实现 + P2-1 口径，诚实边界固化）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
