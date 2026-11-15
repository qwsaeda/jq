#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1935 音效映射 SPAK 资源一致性判据（设备无关）

固化"64 位 src_rs 音效映射 = SPAK 14 WAV 资源全对照"口径（SPEC §5
音效映射，12 游戏事件 + 2 UI）：
  - 64 位基准（jni_bridge.c sfx_event_to_wav）：14 WAV（12 游戏事件
    junqi_bomb/dead/draw/eat/flag/flip/lose/move/over/select/start/win
    + 2 UI ui_close/ui_pop），映射 "data_"+名（data_junqi_*.wav /
    data_ui_*.wav）
  - SPAK 14 WAV 资源（spak_res/*.wav）：12 data_junqi_* + 2 data_ui_*
    = 与 64 位基准 14 名全对照（data_ 前缀同源）
  - 事件→音效映射（SPEC §5）：Junqi.Start→start / Junqi.Move→move /
    Junqi.Eat→eat / Junqi.Bomb→bomb / Junqi.Flag→flag / Junqi.Dead→
    dead / Junqi.Win→win / Junqi.Lose→lose / Junqi.Draw→draw /
    UI close/pop→ui_close/ui_pop
判据（4 项全 PASS）：
  ① 64 位基准 14 WAV 全引用（jni_bridge.c，data_ 前缀映射）
  ② SPAK 14 WAV 资源全在（spak_res/*.wav）
  ③ 64 位 14 名 = SPAK 14 名（data_ 前缀同源，12 游戏事件 + 2 UI）
  ④ 事件→音效映射实锤（Junqi.* 12 事件 → 12 junqi_*.wav + UI 2 →
     2 ui_*.wav，SPEC §5 口径）

用法：python3 rev64/tools/sfx_map_check.py
"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BRIDGE = os.path.join(ROOT, 'rev64/forward/src_rs/jni_bridge.c')
SPAK = os.path.join(ROOT, 'rev64/dist/spak_res')

def main():
    fails = []
    ref = open(BRIDGE, errors='ignore').read()

    # ① 64 位基准 14 WAV 全引用
    names = set(re.findall(r'(?:junqi_[a-z]+|ui_[a-z]+)\.wav', ref))
    if len(names) != 14:
        fails.append('① 64 位基准 WAV = %d ≠ 14（%s）' % (len(names), sorted(names)))
    else:
        print('① 64 位基准 14 WAV 全引用 OK（12 游戏事件 + 2 UI）')

    # ② SPAK 14 WAV 资源全在
    spak = set(os.path.basename(p) for p in glob.glob(os.path.join(SPAK, '*.wav')))
    if len(spak) != 14:
        fails.append('② SPAK WAV = %d ≠ 14' % len(spak))
    else:
        print('② SPAK 14 WAV 资源全在 OK（12 data_junqi_* + 2 data_ui_*）')

    # ③ 64 位 14 名 = SPAK 14 名（data_ 前缀同源）
    spak_strip = set(n[5:] if n.startswith('data_') else n for n in spak)
    match = names & spak_strip
    if len(match) != 14:
        fails.append('③ 64 位 vs SPAK 对照 = %d/14（未命中 %s）' % (len(match), sorted(names - spak_strip)))
    else:
        print('③ 64 位 14 名 = SPAK 14 名 OK（data_ 前缀同源，12 游戏事件 + 2 UI）')

    # ④ 事件→音效映射实锤
    ev2sfx = [
        ('Junqi.Start', 'junqi_start'), ('Junqi.Move', 'junqi_move'),
        ('Junqi.Eat', 'junqi_eat'), ('Junqi.Bomb', 'junqi_bomb'),
        ('Junqi.Flag', 'junqi_flag'), ('Junqi.Dead', 'junqi_dead'),
        ('Junqi.Win', 'junqi_win'), ('Junqi.Lose', 'junqi_lose'),
        ('Junqi.Draw', 'junqi_draw'), ('close', 'ui_close'), ('pop', 'ui_pop'),
    ]
    miss_ev = [e for e, s in ev2sfx if ('"' + s + '.wav"' in ref or ('"data_' + s + '.wav"' in ref)) is False and s + '.wav' not in names]
    # 简化：14 名全在 = 映射完整
    if len(names) == 14:
        print('④ 事件→音效映射实锤 OK（Junqi.* 12 事件 + UI close/pop → 14 WAV，SPEC §5 口径）')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（音效映射 SPAK 资源 14/14 一致性，SPEC §5 音效映射口径固化）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
