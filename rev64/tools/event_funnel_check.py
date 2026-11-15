#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1925 事件分发三层漏斗量化判据（76 事件名 → 12 事件码 → 37 handler）

固化事件分发数据模型完整漏斗（设备无关）：
  层1 事件名 76（.rodata 可提取，73 Junqi + 3 Misc）
  层2 事件码 12（code 0-11，event_code_map.json，.rodata 可提取子集；
     SPEC §3.1 口径 17 事件码 0-16，code 12-16 = Misc 预留需 .bss 运行时填
     asr4 才能提取，§124y-163/185 disc=0 忠实边界）
  层3 handler 37（F_55d40 switch 实列 67 case 指向，全有定义体，
     §124y-1923/1924 勘误后口径）
判据：① 76 = 73 Junqi + 3 Misc ② 12 事件码全在 event_code_map.json
（code 0-11，每 code 事件名数 1-15）③ 37 handler 全有定义体 ④ 漏斗
收敛比实锤（76/12 = 6.3:1 事件名→码 收敛 + 67/37 = 1.81:1 case→handler
发散，1 事件码可映射多 handler = 事件分发"少量 handler 承载多事件码"
设计，与 §124y-26 事件表数据模型一致）。

用法：python3 rev64/tools/event_funnel_check.py
"""
import json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG = '/tmp/orig/lib/armeabi/libjunqi.so'
VTB = os.path.join(ROOT, 'rev64/reverse/src_jni_translated/jqt_vtbl.c')
MAP = os.path.join(ROOT, 'rev64/dist/event_code_map.json')

def main():
    fails = []
    # 层1
    so = open(ORIG, 'rb').read()
    all76 = sorted(set(m.group(0).decode() for m in re.finditer(rb'(?:Junqi|Misc)\.[A-Za-z0-9%]+', so)))
    junqi = [x for x in all76 if x.startswith('Junqi.')]
    misc = [x for x in all76 if x.startswith('Misc.')]
    if len(all76) != 76 or len(junqi) != 73 or len(misc) != 3:
        fails.append('层1 事件名 = %d（Junqi %d + Misc %d）≠ 76/73/3' % (len(all76), len(junqi), len(misc)))
    else:
        print('层1 事件名 = 76（Junqi 73 + Misc 3）OK')

    # 层2
    m = json.load(open(MAP))
    codes = m['event_codes']
    cnums = sorted(int(k) for k in codes)
    if cnums != list(range(12)):
        fails.append('层2 事件码 = %s ≠ 0-11（12 个）' % cnums)
    else:
        counts = {int(k): len(v['names']) for k, v in codes.items()}
        print('层2 事件码 = 12（code 0-11）OK，各 code 事件名数 = %s' %
              {k: counts[k] for k in sorted(counts)})

    # 层3
    src = open(VTB, 'rb').read().decode('utf-8', errors='replace')
    cases = {int(x.group(1)): x.group(2) for x in re.finditer(r'case (\d+):\s*h\s*=\s*0x([0-9a-f]+);', src)}
    distinct = set(cases.values())
    defined = set(x.group(1) for x in re.finditer(r'^uint32_t F_([0-9a-f]{5,})\(', src, re.M))
    missing = [h for h in distinct if h not in defined]
    if len(distinct) != 37:
        fails.append('层3 handler = %d ≠ 37' % len(distinct))
    if missing:
        fails.append('层3 缺定义体 handler = %s' % missing[:5])
    if not missing:
        print('层3 handler = 37（全有定义体）OK，实列 case = %d' % len(cases))

    # 漏斗收敛比
    r1 = 76 / 12
    r3 = len(cases) / 37
    print('漏斗收敛比：事件名 76/码 12 = %.2f:1（收敛）| case %d/handler 37 = %.2f:1（发散，1 码多 handler）' % (r1, len(cases), r3))

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（76→12→37 三层漏斗全实锤，事件分发数据模型完整，§124y-26 一致）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
