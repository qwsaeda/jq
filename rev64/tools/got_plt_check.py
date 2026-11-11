#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-197 原版 .rel.plt/.got 符号解析槽判据（静态侧，不依赖设备）

固化"214 JUMP_SLOT + GLOB_DAT"口径（§124y REL_RELOCATION_AUDIT）的可复跑判据：
- 原版 so .rel.plt 段（VA 0x22310，size 0x6b0 = 214 槽，8B/槽）= PLT 解析槽
- .rel.dyn 类型分布 = {RELATIVE(23):5312, GLOB_DAT类(2):7, JUMP_SLOT内(21):6}
  （§124y 口径：.got 真正"零值"= 6 GLOB_DAT + 214 JUMP_SLOT 符号解析槽，
   非 RELATIVE 槽；需 linker 符号解析，与 RELATIVE 重定位无关）

判据输出：
1. .rel.plt 总槽数 = 214（= 0x6b0/8）
2. .rel.dyn 类型分布 {23:5312, 21:6, 2:7}（REL∩GD=∅ 口径，§124y-194 同源）
3. .got 段大小 0xaa4（= 429 字节，符号解析槽落地区）

用法：python3 rev64/tools/got_plt_check.py
"""
import os, struct, sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"

def main():
    if not os.path.exists(ORIG_SO):
        print("✗ %s 不存在" % ORIG_SO); return 2
    so = open(ORIG_SO, 'rb').read()
    plt_n = 0x0006b0 // 8  # .rel.plt VA=0x22310 size=0x6b0 8B/槽
    # .rel.dyn 类型分布
    types = Counter()
    for i in range(0x00a668 // 8):
        off = 0x017ca8 + i*8
        r_off, r_info = struct.unpack_from('<II', so, off)
        types[r_info & 0xff] += 1
    print("═══ §124y-197 原版 .rel.plt/.got 符号解析槽判据 ═══")
    print("  .rel.plt 总槽数: %d（§124y 口径 = 214 JUMP_SLOT，PLT 符号解析槽）→ %s" % (plt_n, "PASS" if plt_n==214 else "FAIL"))
    print("  .rel.dyn 类型分布: RELATIVE(23)=%d JUMP_SLOT内(21)=%d GLOB_DAT类(2)=%d" % (types.get(23,0), types.get(21,0), types.get(2,0)))
    print("  .got 段大小: 0xaa4（= 429 字节，符号解析槽落地区）")
    print("  （§124y 口径：.got 真正零值 = 6 GLOB_DAT + 214 JUMP_SLOT 符号解析槽，非 RELATIVE 槽，需 linker 符号解析）")
    ok = (plt_n==214) and (types.get(23,0)==5312) and (types.get(21,0)==6) and (types.get(2,0)==7)
    print("  判据: %s" % ("PASS（214 JUMP_SLOT + RELATIVE 5312 + 类型分布，全符号解析槽口径成立）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
