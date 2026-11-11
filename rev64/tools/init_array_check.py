#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-211 原版 so INIT_ARRAY 12 构造器清单判据（静态侧，不依赖设备）

固化"原版 32 位 so INIT_ARRAY = 12 个构造器（INIT_ARRAYSZ=48 字节）"口径
（§124y unidbg 课程 / init_array 11/12 实锤基准）的可复跑判据：
- 原版 so .dynamic INIT_ARRAY VA=0x155000 / INIT_ARRAYSZ=48（= 12 个 4B 指针）
- 12 构造器指针（Thumb 指令偏移，低 bit=1 奇数化）：
  init[0]=0x03dae5 ... init[11]=0x11c10d（§124y 实锤 init[11]=0x11c10d
  moneypunct<wchar_t>::do_negative_sign 函数中间偏移起跳 = unicorn2 精度 bug 点）

判据输出：
1. INIT_ARRAYSZ = 48（= 12 构造器）
2. 12 指针全非 0 + 全在 .text 段区间（0x233e0..0x13d1f4）
3. init[11] = 0x11c10d（§124y 实锤锚点，防清单漂移）

用法：python3 rev64/tools/init_array_check.py
"""
import os, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"
INIT_VA = 0x155000
N = 12

def main():
    if not os.path.exists(ORIG_SO):
        print("✗ %s 不存在" % ORIG_SO); return 2
    so = open(ORIG_SO, 'rb').read()
    # INIT_ARRAYSZ 实锤
    d = subprocess.run(["readelf","-d",ORIG_SO], capture_output=True, text=True).stdout
    sz = 0
    import re
    for line in d.splitlines():
        if "INIT_ARRAYSZ" in line:
            m = re.search(r'\(INIT_ARRAYSZ\)\s+(\d+)', line)
            sz = int(m.group(1)) if m else 0
            break
    if sz != 48:
        print("✗ INIT_ARRAYSZ=%d ≠ 48（12 构造器口径）" % sz); return 2
    init = so[INIT_VA:INIT_VA+48]
    ptrs = [struct.unpack_from('<I', init, i*4)[0] for i in range(N)]
    non_zero = all(p != 0 for p in ptrs)
    in_text = all(0x233e0 <= (p & ~1) < 0x13d1f4 for p in ptrs)
    anchor = (ptrs[11] == 0x11c10d)
    print("═══ §124y-211 原版 so INIT_ARRAY 12 构造器清单判据 ═══")
    print("  INIT_ARRAYSZ: %d（= 12 构造器 × 4B）" % sz)
    print("  12 指针全非0: %s / 全在 .text 段区间: %s" % (non_zero, in_text))
    for i,p in enumerate(ptrs):
        print("    init[%02d] = 0x%06x" % (i, p))
    print("  init[11]=0x11c10d 锚点（§124y 实锤点，unicorn2 精度 bug 起跳处）: %s" % ("PASS" if anchor else "FAIL"))
    ok = non_zero and in_text and anchor
    print("  判据: %s" % ("PASS（12 构造器清单 + init[11] 锚点，§124y 口径成立）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
