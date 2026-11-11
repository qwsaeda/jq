#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-194 原版 .rel.dyn 重定位槽判据（静态侧，不依赖设备）

固化"RELATIVE 5312 槽全重定位"口径（§124y REL_RELOCATION_AUDIT）的可复跑判据：
- 原版 so .rel.dyn 段（VA 0x17ca8，size 0xa668 = 5325 槽，8B/槽 Elf32_Rela）
- R_ARM_RELATIVE = 类型 23（★ 坑：32 位 ARM RELATIVE type = 23 非 2；type 2 = R_ARM_GLOB_DAT）
- 判据① .rel.dyn 总槽数 = 5325（= 0xa668/8）
- 判据② R_ARM_RELATIVE(23) 槽数 = 5312（§124y 口径，逐一匹配 readelf）
- 判据③ 类型分布 = {23:5312, 21:6, 2:7}（RELATIVE 5312 + PLT 21:6 + GLOB_DAT 2:7）

用法：python3 rev64/tools/rel_dyn_check.py
"""
import os, struct, sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"
REL_VA = 0x017ca8
REL_SIZE = 0x00a668

def main():
    if not os.path.exists(ORIG_SO):
        print("✗ %s 不存在" % ORIG_SO); return 2
    so = open(ORIG_SO, 'rb').read()
    n = REL_SIZE // 8
    types = Counter()
    for i in range(n):
        off = REL_VA + i*8
        r_off, r_info = struct.unpack_from('<II', so, off)
        types[r_info & 0xff] += 1
    rel = types.get(23, 0)  # R_ARM_RELATIVE = 23
    plt = types.get(21, 0)  # R_ARM_JUMP_SLOT = 21
    gdat = types.get(2, 0)  # R_ARM_GLOB_DAT?（type 2，槽数 7）
    print("═══ §124y-194 原版 .rel.dyn 重定位槽判据 ═══")
    print("  .rel.dyn 段: VA 0x%x size 0x%x（%d 槽，8B/槽）" % (REL_VA, REL_SIZE, n))
    print("  总槽数: %d（= 0xa668/8 = 5325）→ %s" % (n, "PASS" if n==5325 else "FAIL"))
    print("  R_ARM_RELATIVE(23) 槽数: %d（§124y 口径 = 5312，REL∩GD=∅ 已实锤）→ %s" % (rel, "PASS" if rel==5312 else "FAIL"))
    print("  类型分布: RELATIVE(23)=%d JUMP_SLOT(21)=%d type2=%d" % (rel, plt, gdat))
    print("  （坑固化：32 位 ARM R_ARM_RELATIVE type=23 非 2；type 2=7 槽 = GLOB_DAT 类，与 RELATIVE 5312 槽独立）")
    ok = (n==5325) and (rel==5312) and (plt==6) and (gdat==7)
    print("  判据: %s" % ("PASS（RELATIVE 5312 + 槽数 5325 + 类型分布，全重定位口径成立）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
