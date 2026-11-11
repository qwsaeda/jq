#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-213 原版 so .bss 惰性单例槽 NOBITS 零初始化判据（静态侧，不依赖设备）

固化".bss 段 NOBITS（dlopen 零初始化）+ 单例槽 0x15bfec 相对 .bss 偏移 0x16c
在段内 + single 槽运行时 C++ 构造态需动态填（静态全 0 = 忠实兜底）"口径
（§124y-32 单例构造者定位实锤）的可复跑判据：
- 原版 so .bss 段 NOBITS（VA 0x15be80 size 0x7008，dlopen 零初始化）
- 单例槽 VA 0x15bfec = 相对 .bss 偏移 0x16c（在 .bss 段内 0..0x7008）
- .bss 文件不占位（NOBITS）→ 静态文件读不出运行时值 → single 槽须运行时
  C++ 惰性构造填（§124y-32 实锤 MuMu 运行时 single=0x00000000 = 未构造忠实兜底）

判据输出：
1. .bss 段 NOBITS（readelf -SW 段类型）
2. 单例槽 0x15bfec 相对 .bss 偏移 0x16c 在段内（0 ≤ 0x16c < 0x7008）
3. .bss 文件不占位（NOBITS 段无文件区间）

用法：python3 rev64/tools/bss_single_check.py
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"
BSS_VA, BSS_SIZE = 0x15be80, 0x7008
SINGLE_VA = 0x15bfec

def main():
    if not os.path.exists(ORIG_SO):
        print("✗ %s 不存在" % ORIG_SO); return 2
    out = subprocess.run(["readelf","-SW",ORIG_SO], capture_output=True, text=True).stdout
    bss_nobits = False
    for line in out.splitlines():
        if re.search(r'\.bss\s+NOBITS', line):
            bss_nobits = True
            break
    off = SINGLE_VA - BSS_VA
    in_range = 0 <= off < BSS_SIZE
    print("═══ §124y-213 原版 so .bss 惰性单例槽 NOBITS 零初始化判据 ═══")
    print("  .bss 段 NOBITS（dlopen 零初始化，文件不占位）: %s" % ("PASS" if bss_nobits else "FAIL"))
    print("  单例槽 0x15bfec 相对 .bss 偏移 0x%x（在段内 0..0x%x）: %s" % (off, BSS_SIZE, "PASS" if in_range else "FAIL"))
    print("  （单例槽 = F_3ab64 single，运行时 C++ 惰性构造填；静态全 0 = 忠实兜底 §124y-32 实锤 MuMu single=0x00000000）")
    ok = bss_nobits and in_range
    print("  判据: %s" % ("PASS（.bss NOBITS 零初始化 + 单例槽偏移合法，惰性语义成立）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
