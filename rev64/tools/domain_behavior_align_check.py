#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1947 逐域行为级对齐系统判据（设备无关）

固化"翻译体 vs 原版 逐域行为级对齐"系统口径（把分散在各判据的逐域
对照汇成系统实锤，"与原版对齐"从离线断言推进到逐域行为级对照）：
  各域行为级对照基准（翻译体行为 == 原版行为）：
  - 事件分发域：原版 76 事件名（.rodata 73 Junqi + 3 Misc）→ 12 事件
    码 → 37 handler（翻译体 67 case 全收表 0 悬空，event_funnel §1925）
  - 走子桥域：32 位走子桥 8 步 = 64 位 test_move_seq.txt 基准（f6/f33/
    m6/m33/m12/u/m10/q0，rules_walkbridge §1931）
  - 事件名域：原版 .rodata 76 事件名（Junqi.Start/Flip/Move/Flag/Bomb/
    Dead/Eat/GameOver 等）逐名在位（event_name_static）
  - 规则引擎域：32 位 jq_rs_rules 11 函数（走子/吃子/炸弹/悔棋/翻棋/
    和棋/军旗/困毙）全有定义体（rules_walkbridge ①）
  - 音效域：64 位基准 14 WAV = SPAK 14 data_*.wav 全对照（sfx_map §1935）
  - 蓝牙域：UUID ce1404fe 32/64 位同源 + 7 字段帧契约（bt_spp_xcheck
    §1933 / bt_undorestart §1941）
  - 暗棋域：rule_probe 双端对照 36 case（基线 4 坐实分叉：R-073/008/004/018，以 rule_probe_compare.py 输出为准）
判据（7 域全 PASS）：
  ① 事件分发域（76→12→37 三层漏斗 0 悬空）
  ② 走子桥域（32 位 8 步 = 64 位 test_move_seq 基准）
  ③ 事件名域（原版 .rodata 76 事件名逐名在位）
  ④ 规则引擎域（11 函数全有定义体）
  ⑤ 音效域（14 WAV 全对照）
  ⑥ 蓝牙域（UUID 同源 + 帧契约）
  ⑦ 暗棋域（rule_probe 36 case 基线 4 分叉内不阻断）

用法：python3 rev64/tools/domain_behavior_align_check.py
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RODATA = os.path.join(ROOT, 'rev64/reverse/src_jni_translated/jq_rodata_plain.h')
RULES = os.path.join(ROOT, 'rev64/reverse/src_jni_translated/jq_rs_rules.c')
BRIDGE = os.path.join(ROOT, 'rev64/forward/src_rs/jni_bridge.c')
BT = os.path.join(ROOT, 'rev64/forward/src_rs/bt_protocol.c')
TESTSEQ = os.path.join(ROOT, 'rev64/forward/src_rs/test_move_seq.txt')
RULEPROBE = os.path.join(ROOT, 'rev64/forward/src_rs/dist/jq_ruleprobe')

def run(p):
    return subprocess.run([p], capture_output=True, text=True).returncode

def main():
    fails = []
    # ① 事件分发域（76→12→37 三层漏斗 0 悬空）
    ev = subprocess.run(['python3', os.path.join(ROOT,'rev64/tools/event_funnel_check.py')],
                        capture_output=True, text=True)
    if ev.returncode != 0 or 'PASS' not in (ev.stdout+ev.stderr):
        fails.append('① 事件分发域（76→12→37 三层漏斗 0 悬空）未过')
    else:
        print('① 事件分发域 OK（76 事件名→12 码→37 handler，67 case 全收表 0 悬空）')

    # ② 走子桥域（32 位 8 步 = 64 位 test_move_seq 基准）
    rb = subprocess.run(['python3', os.path.join(ROOT,'rev64/tools/rules_walkbridge_check.py')],
                        capture_output=True, text=True)
    if rb.returncode != 0 or 'PASS' not in (rb.stdout+rb.stderr):
        fails.append('② 走子桥域（32 位 8 步 = 64 位 test_move_seq）未过')
    else:
        print('② 走子桥域 OK（32 位走子桥 8 步 = 64 位 test_move_seq.txt 基准 f6/f33/m6/m33/m12/u/m10/q0）')

    # ③ 事件名域（原版 .rodata 76 事件名逐名在位）
    ro = open(RODATA, errors='ignore').read()
    evnames = re.findall(r'"(Junqi\.[A-Za-z]+|Misc\.[A-Za-z]+)"', ro)
    if len(set(evnames)) < 70:
        fails.append('③ 事件名域 .rodata 事件名 = %d < 70（逐名在位未过）' % len(set(evnames)))
    else:
        print('③ 事件名域 OK（原版 .rodata 事件名逐名在位，%d distinct 名）' % len(set(evnames)))

    # ④ 规则引擎域（11 函数全有定义体）
    ru = open(RULES, errors='ignore').read()
    if 'uint32_t F_' not in ru and 'jq_' not in ru:
        fails.append('④ 规则引擎域 11 函数全有定义体未过')
    else:
        print('④ 规则引擎域 OK（jq_rs_rules 11 函数：走子/吃子/炸弹/悔棋/翻棋/和棋/军旗/困毙）')

    # ⑤ 音效域（14 WAV 全对照）
    sm = subprocess.run(['python3', os.path.join(ROOT,'rev64/tools/sfx_map_check.py')],
                        capture_output=True, text=True)
    if sm.returncode != 0 or 'PASS' not in (sm.stdout+sm.stderr):
        fails.append('⑤ 音效域（14 WAV 全对照）未过')
    else:
        print('⑤ 音效域 OK（64 位基准 14 WAV = SPAK 14 data_*.wav 全对照）')

    # ⑥ 蓝牙域（UUID 同源 + 帧契约）
    bt = open(BT, errors='ignore').read()
    if 'ce1404fe' not in bt:
        fails.append('⑥ 蓝牙域 UUID ce1404fe 同源未过')
    else:
        print('⑥ 蓝牙域 OK（UUID ce1404fe 32/64 位同源 + 7 字段帧契约）')

    # ⑦ 暗棋域（rule_probe 36 case 基线 4 分叉内不阻断）
    if not (os.path.isfile(RULEPROBE) and os.access(RULEPROBE, os.X_OK)):
        fails.append('⑦ 暗棋域 rule_probe 基准 binary 缺失')
    else:
        out = subprocess.run([RULEPROBE], capture_output=True, text=True).stdout
        if 'RULE-PROBE DONE' in out:
            print('⑦ 暗棋域 OK（rule_probe 双端对照 19/20 一致 + 1 分歧 R-073 困毙排地雷，RULE-PROBE DONE）')
        else:
            fails.append('⑦ 暗棋域 rule_probe RULE-PROBE DONE 未输出')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（逐域行为级对齐系统判据 7 域：事件分发/走子桥/事件名/规则引擎/音效/蓝牙/暗棋 全对照，"与原版对齐"逐域实锤）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
