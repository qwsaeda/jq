#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1929 .rel.dyn 5312 RELATIVE 槽位分布判据（设备无关）

固化 AGENTS §124x 口径"5312 R_ARM_RELATIVE 槽全重定位（每槽 += g_jqt_shadow
加一次得 B+S，正确）"的槽位分布实锤：
  - 原版 so .rel.dyn 段 = 5312 个 R_ARM_RELATIVE 槽（readelf -r 实锤）
  - 槽 VA 全在 .data 区 0x155000-0x15be2c（16 槽 + 5296 槽，全 ⊂ 镜像
    拷贝区 [0x233e0, 0x15be80)）
  - .bss 区（0x15be80-0x15ef34）无 RELATIVE 槽（NOBITS 段运行时清零，
    非重定位目标，jq_rel_data.h 只重定位 .data 区 5312 槽）
判据（4 项全 PASS）：
  ① .rel.dyn 段存在（REL 段，offset 0x17ca8 size 0xa668）
  ② R_ARM_RELATIVE 槽 = 5312（AGENTS §124x 口径）
  ③ 槽 VA 全 ⊂ [0x233e0, 0x15be80)（镜像拷贝区，jq_rel_data.h 重定位目标）
  ④ 槽 VA 上下界 = 0x155000-0x15be2c（.data 区，.bss 0x15be80+ 无 RELATIVE 槽）

用法：python3 rev64/tools/rel_dyn_slots_check.py
"""
import os, re, subprocess, sys

ORIG = '/tmp/orig/lib/armeabi/libjunqi.so'

def main():
    fails = []
    r = subprocess.run(['arm-linux-gnueabi-readelf', '-r', ORIG], capture_output=True, text=True).stdout
    w = subprocess.run(['arm-linux-gnueabi-readelf', '-SW', ORIG], capture_output=True, text=True).stdout

    # ① .rel.dyn 段
    m = re.search(r'\.rel\.dyn\s+REL\s+([0-9a-f]+)\s+\S+\s+([0-9a-f]+)', w)
    if not m:
        fails.append('① .rel.dyn 段未找到')
    else:
        print('① .rel.dyn 段 OK（offset 0x%s size 0x%s）' % (m.group(1), m.group(2)))

    # ② RELATIVE 槽数
    rels = re.findall(r'^([0-9a-f]+)\s+[0-9a-f]+\s+R_ARM_RELATIVE', r, re.M)
    if len(rels) != 5312:
        fails.append('② R_ARM_RELATIVE = %d ≠ 5312（AGENTS §124x 口径）' % len(rels))
    else:
        print('② R_ARM_RELATIVE = 5312 OK（AGENTS §124x 全重定位）')

    # ③ 槽 VA 全 ⊂ [0x233e0, 0x15be80)
    vs = [int(v, 16) for v in rels]
    in_img = all(0x233e0 <= v < 0x15be80 for v in vs)
    if not in_img:
        out = [v for v in vs if not (0x233e0 <= v < 0x15be80)]
        fails.append('③ %d 槽 VA 越界 [0x233e0,0x15be80)（样例 %s）' % (len(out), [hex(v) for v in out[:5]]))
    else:
        print('③ 5312 槽 VA 全 ⊂ [0x233e0,0x15be80) OK（jq_rel_data.h 重定位目标区）')

    # ④ 上下界
    lo, hi = min(vs), max(vs)
    if lo != 0x155000 or hi != 0x15be2c:
        fails.append('④ 槽 VA 上下界 0x%x-0x%x ≠ 0x155000-0x15be2c（.data 区）' % (lo, hi))
    else:
        print('④ 槽 VA 上下界 0x155000-0x15be2c OK（.data 区，.bss 0x15be80+ 无 RELATIVE 槽）')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（5312 RELATIVE 槽全在 .data 区 0x155000-0x15be2c，镜像重定位口径固化）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
