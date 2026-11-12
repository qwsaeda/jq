#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-222 翻译体 so ABI 一致性 + 重定位槽数判据（静态侧，不依赖设备）

固化"32 位翻译体 so = ELF32/ARM（与原版同 ABI）+ 重定位槽数基准"口径
（防翻译体 so ABI 漂移/误链 64 位）：
- 翻译体 so rev64/dist/libjunqi_arm32_aligned.so readelf -h = ELF32 + ARM
- readelf -r 重定位类型分布 = RELATIVE 2270 + JUMP_SLOT 95 + GLOB_DAT 6
  （翻译体代码量 < 原版 5312 RELATIVE，槽数不同属预期，判核类型分布非逐值对齐）

判据输出：
1. ELF32 + Machine=ARM（与原版同 ABI，非误链 64 位）
2. 重定位含 R_ARM_RELATIVE + R_ARM_JUMP_SLOT + R_ARM_GLOB_DAT（三类全在）

用法：python3 rev64/tools/translated_abi_check.py
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TRANS_SO = os.path.join(ROOT, "rev64/dist/libjunqi_arm32_aligned.so")

def main():
    if not os.path.exists(TRANS_SO):
        print("✗ %s 不存在" % TRANS_SO); return 2
    h = subprocess.run(["readelf","-h",TRANS_SO], capture_output=True, text=True).stdout
    is32 = "ELF32" in h
    is_arm = "ARM" in h.split("Machine:")[-1] if "Machine:" in h else False
    r = subprocess.run(["readelf","-r",TRANS_SO], capture_output=True, text=True).stdout
    rel = r.count("R_ARM_RELATIVE")
    js = r.count("R_ARM_JUMP_SLOT")
    gd = r.count("R_ARM_GLOB_DAT")
    print("═══ §124y-222 翻译体 so ABI 一致性 + 重定位槽数判据 ═══")
    print("  ELF32 + Machine=ARM（与原版同 ABI，非误链 64 位）: %s" % ("PASS" if (is32 and is_arm) else "FAIL"))
    print("  重定位类型分布: RELATIVE=%d JUMP_SLOT=%d GLOB_DAT=%d" % (rel, js, gd))
    print("  （翻译体代码量 < 原版，RELATIVE 2270 vs 原版 5312 槽数不同属预期，判核类型全在）")
    ok = is32 and is_arm and (rel > 0 and js > 0 and gd > 0)
    print("  判据: %s" % ("PASS（翻译体 ELF32/ARM + 三类重定位全在，ABI 一致非漂移）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
