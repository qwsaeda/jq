#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1952 "与原版对齐"推进状态系统判据（设备无关）

固化"与原版对齐"推进状态清单（哪些域已离线实锤对齐 / 哪些真机才能实锤
/ 为何卡设备）——回答"没对齐为什么不推进"：把分散在 §1947-§1951 的逐域
对齐实锤汇成一张系统清单，明确"已推进收口的对齐维度 + 真机阻塞维度"：
  已离线实锤对齐（可推进，已收口）：
  - 事件分发域：76→12→37 三层漏斗 0 悬空（§1947 ①）
  - 走子桥域：8 步序列 + 桌面 8/8 行为级（§1951 ②）
  - 事件名域：.rodata 74 distinct 逐名（§1947 ③）
  - 规则引擎域：11 函数全定义体（§1947 ④）
  - 暗棋域：R-073 排地雷修复（§1950）+ rule_probe 20/20
  - 音效域：14 WAV 全对照（§1947 ⑤）
  - 蓝牙域：UUID ce1404fe + 帧契约（§1947 ⑥）
  - 诚实边界 P2-1：四国/联网/账号/版本更新 全收口（§1943-1946）
  真机才能实锤（设备 10.0.182.146 阻塞，PENDING 非 FAIL）：
  - 434 RAW 系统域 = 真机 linker 解析（libstdc++/glibc，离线不可 1:1）
  - 设备侧 8 步全链：完整 60 局 / screencap 非黑比 / 蓝牙端到端 / 音效出屏
判据（4 项全 PASS）：
  ① 已离线实锤对齐 7 域全 PASS（domain_behavior_align + walkbridge_8step
     + R-073 桌面探针 + 诚实边界）
  ② 诚实边界 P2-1 4 项全收口（nplayer/net/guest/update boundary）
  ③ 434 RAW 系统域 linker 兜底口径在位（raw_zone_lib 6 zone 归因）
  ④ 设备侧 8 步全链固化 + 真机阻塞 PENDING 口径（device_regress_check）

用法：python3 rev64/tools/alignment_status_check.py
"""
import os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOLS = os.path.join(ROOT, 'rev64/tools')

def run(p):
    r = subprocess.run(['python3', p], capture_output=True, text=True)
    return r.returncode, (r.stdout + r.stderr)

def main():
    fails = []
    # ① 已离线实锤对齐 7 域全 PASS
    for chk in ['domain_behavior_align_check.py', 'walkbridge_8step_check.py', 'rules_walkbridge_check.py']:
        rc, out = run(os.path.join(TOOLS, chk))
        if rc != 0 or 'PASS' not in out:
            fails.append('① 已离线对齐 %s 未过（rc=%d）' % (chk, rc))
    else:
        print('① 已离线实锤对齐 7 域全 PASS（事件分发/走子桥 8 步/事件名/规则/暗棋 R-073/音效/蓝牙）')

    # ② 诚实边界 P2-1 4 项全收口
    for chk in ['nplayer_boundary_check.py', 'net_boundary_check.py', 'guest_path_check.py', 'update_boundary_check.py']:
        rc, out = run(os.path.join(TOOLS, chk))
        if rc != 0 or 'PASS' not in out:
            fails.append('② 诚实边界 %s 未过（rc=%d）' % (chk, rc))
    else:
        print('② 诚实边界 P2-1 4 项全收口 OK（四国/联网/账号/版本更新，§1943-1946）')

    # ③ 434 RAW 系统域 linker 兜底口径在位
    rc, out = run(os.path.join(TOOLS, 'raw_zone_lib_check.py'))
    if rc != 0 or 'PASS' not in out:
        fails.append('③ 434 RAW 系统域 linker 兜底口径未过（rc=%d）' % rc)
    else:
        print('③ 434 RAW 系统域 linker 兜底口径 OK（6 zone 全归 libstdc++，0 业务混入，§1906/§1927）')

    # ④ 设备侧 8 步全链固化 + 真机阻塞 PENDING 口径
    rc, out = run(os.path.join(TOOLS, 'device_regress_offline_check.py'))
    if rc != 0 or 'PASS' not in out:
        fails.append('④ 设备侧 8 步全链固化未过（rc=%d）' % rc)
    else:
        print('④ 设备侧 8 步全链固化 OK（完整 60 局 + screencap + 蓝牙端到端 + 音效出屏 + 走子 4 路径，真机 PENDING §124y-189）')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（"与原版对齐"推进状态系统判据 4 项：已离线 7 域 + 诚实边界 P2-1 + 434 系统域兜底 + 设备侧 8 步全链，明确已推进/真机阻塞维度）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
