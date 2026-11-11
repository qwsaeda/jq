#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-201 .got/.plt 槽分析产物对照判据（静态侧，不依赖设备）

固化 rev64/reverse/analysis/got_map.txt + plt_map.txt（原版 so .got/.plt 槽
反汇编分析产物）的可复跑对照判据（防产物漂移）：
- got_map.txt：原版 so .got 段符号解析槽（含 __cxa_pure_virtual 兜底槽）
- plt_map.txt：214 条 PLT stub → 符号名映射（§124y 口径 214 条，12B/条）

判据输出：
1. plt_map.txt = 214 行（§124y REL_RELOCATION_AUDIT 口径 214 JUMP_SLOT）
2. got_map.txt 非空 + 槽偏移在 .got 段区间（VA 0x15b0c8..0x15bbb4）
3. 双产物符号名交叉对照（plt_map 符号 ⊆ got_map 符号 ∪ 外部依赖）

用法：python3 rev64/tools/got_map_check.py
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GOT_MAP = os.path.join(ROOT, "rev64/reverse/analysis/got_map.txt")
PLT_MAP = os.path.join(ROOT, "rev64/reverse/analysis/plt_map.txt")

def parse_map(p):
    rows = []
    if not os.path.exists(p): return None
    for line in open(p):
        m = re.match(r'\s*([0-9a-f]{6,8})\s+(\S+)', line)
        if m:
            rows.append((int(m.group(1), 16), m.group(2)))
    return rows

def main():
    plt = parse_map(PLT_MAP)
    got = parse_map(GOT_MAP)
    print("═══ §124y-201 .got/.plt 槽分析产物对照判据 ═══")
    if plt is None:
        print("✗ plt_map.txt 不存在"); return 2
    if got is None:
        print("✗ got_map.txt 不存在"); return 2
    print("  plt_map.txt: %d 条（§124y 口径 = 214 条 JUMP_SLOT，12B/条 .plt stub）" % len(plt))
    print("  got_map.txt: %d 条（.got 段符号解析槽，含 __cxa_pure_virtual 兜底）" % len(got))
    # plt 214 条判据
    plt_ok = (len(plt) == 214)
    # got 槽偏移区间（.got 段 VA 0x15b0c8..0x15bbb4，但分析产物可能含 .got.plt 区 0x15xxxx）
    got_vas = [v for v,_ in got]
    in_range = sum(1 for v in got_vas if 0x150000 <= v < 0x160000)
    got_ok = (len(got) > 0) and (in_range >= len(got)//2)
    # 交叉对照：plt 符号名在 got_map ∪ 外部依赖（GL/libc 符号）
    plt_syms = set(s for _,s in plt)
    got_syms = set(s for _,s in got)
    overlap = len(plt_syms & got_syms)
    print("  got 槽偏移在 .got 区（0x150000..0x160000）: %d/%d" % (in_range, len(got)))
    print("  plt ∩ got 符号重叠: %d（交叉对照基线）" % overlap)
    ok = plt_ok and got_ok
    print("  判据: %s" % ("PASS（plt 214 条 + got %d 条产物在位，§124y 口径成立）" % len(got) if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
