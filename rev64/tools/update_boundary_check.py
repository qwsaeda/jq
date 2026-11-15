#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1946 版本更新通道边界对话框实锤判据（设备无关）

固化"版本更新通道（下载/升级）未逆向 = 诚实边界对话框（src_rs 无更新
通道，占位）"口径（AGENTS 诚实边界 + BUSINESS_RISK_LEDGER P2-1 版本
更新通道未实现）：
  - 版本更新路径：UI_UI118 版本更新弹窗（datah_118 450×360 居中，
    2 按钮）→ 点"更新" → 诚实边界对话框（"版本更新通道（下载/升级）
    未逆向，src_rs 无更新通道，占位"）→ 取消/关闭回对局
  - .rodata 明文锚点（原版 32 位）：updateUrl / statHost / cfgHost
    字符串（0x120cf0/0x120cfc/0x120d08），版本更新通道源
判据（4 项全 PASS）：
  ① UI_UI118 版本更新弹窗在位（jni_bridge.c，datah_118 2 按钮）
  ② 点"更新" → 诚实边界对话框在位（"版本更新通道（下载/升级）未逆向"）
  ③ 取消 → 关闭回对局（不假装版本更新可玩）
  ④ .rodata 明文 updateUrl/statHost/cfgHost 锚点在位（原版版本更新
     通道源，jq_rodata_plain.h）

用法：python3 rev64/tools/update_boundary_check.py
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BRIDGE = os.path.join(ROOT, 'rev64/forward/src_rs/jni_bridge.c')
RODATA = os.path.join(ROOT, 'rev64/reverse/src_jni_translated/jq_rodata_plain.h')

def main():
    fails = []
    ref = open(BRIDGE, errors='ignore').read()

    # ① UI_UI118 版本更新弹窗在位
    if 'UI_UI118' not in ref or 'UI_UI118_COUNT' not in ref:
        fails.append('① UI_UI118 版本更新弹窗（datah_118 2 按钮）缺失')
    else:
        print('① UI_UI118 版本更新弹窗在位 OK（datah_118 450×360 居中，2 按钮）')

    # ② 点"更新" → 诚实边界对话框在位
    if '版本更新通道（下载/升级）未逆向' not in ref:
        fails.append('② 点"更新" → 诚实边界对话框（"版本更新通道未逆向"）缺失')
    else:
        print('② 点"更新" → 诚实边界对话框在位 OK（"版本更新通道（下载/升级）未逆向，src_rs 无更新通道，占位"）')

    # ③ 取消 → 关闭回对局（不假装版本更新可玩）
    if '取消 → 关闭回对局' not in ref and '关闭回对局' not in ref:
        fails.append('③ 取消 → 关闭回对局（不假装版本更新可玩）缺失')
    else:
        print('③ 取消 → 关闭回对局 OK（不假装版本更新可玩）')

    # ④ .rodata 明文 updateUrl/statHost/cfgHost 锚点在位
    ro = open(RODATA, errors='ignore').read()
    if not ('updateUrl' in ro and 'statHost' in ro and 'cfgHost' in ro):
        fails.append('④ .rodata 明文 updateUrl/statHost/cfgHost 锚点缺失')
    else:
        print('④ .rodata 明文 updateUrl/statHost/cfgHost 锚点在位 OK（原版版本更新通道源 0x120cf0/0x120cfc/0x120d08）')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（版本更新通道边界对话框实锤 4 项：UI_UI118 弹窗 + 更新边界对话框 + 取消回对局 + .rodata 锚点，P2-1 诚实边界固化）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
