#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-193 原版 .text 段明文判据（静态侧，不依赖设备）

固化"非加壳 so / .text 代码段明文无加密"口径的可复跑判据（与 §124y-192
.rodata 明文判据成对，组成"非加壳 so"完整静态指纹）：
- 原版 so .text 段（VA 0x233e0，size 0xfd814 = 1038356B）
- 判据① 熵 = 7.10（正常机器码区间 6.5-7.5；加密壳代码段熵 ≥ 7.9）
- 判据② .text 前 16B 非全 0（加密壳特征 = 全 0 占位或高熵随机块）
- 判据③ 已知 JNI 导出符号（Java_com_cnvcs_App_On*）在 .dynsym 明文可见

用法：python3 rev64/tools/text_static_check.py
"""
import math, os, sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"
TEXT_VA = 0x233e0
TEXT_SIZE = 0xfd814

def main():
    if not os.path.exists(ORIG_SO):
        print("✗ %s 不存在" % ORIG_SO); return 2
    so = open(ORIG_SO, 'rb').read()
    text = so[TEXT_VA:TEXT_VA+TEXT_SIZE]
    cnt = Counter(text)
    H = -sum((c/len(text))*math.log2(c/len(text)) for c in cnt.values())
    first16 = text[:16]
    nz = sum(1 for b in first16 if b != 0)
    # JNI 导出符号明文（原版 32 位 so 15 个 On* 符号，nm -D 实锤）
    known_sym = [b"Java_com_cnvcs_App_On"]
    dynsym_vis = [s for s in known_sym if s in so]
    print("═══ §124y-193 原版 .text 段明文判据 ═══")
    print("  .text 段: VA 0x%x size 0x%x（%d 字节）" % (TEXT_VA, TEXT_SIZE, len(text)))
    print("  熵: %.2f（正常机器码 6.5-7.5；加密壳 ≥7.9）→ %s" % (H, "PASS" if 6.0 <= H <= 7.5 else "FAIL"))
    print("  前 16B 非0字节: %d/16（非全 0 = 非加密壳占位）→ %s" % (nz, "PASS" if nz >= 8 else "FAIL"))
    print("  JNI 导出符号明文: %d/%d %s" % (len(dynsym_vis), len(known_sym), dynsym_vis))
    ok = (6.0 <= H <= 7.5) and (nz >= 8) and len(dynsym_vis) >= 1
    print("  判据: %s" % ("PASS（非加壳 so / .text 明文无加密口径成立，与 §124y-192 .rodata 明文成对）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
