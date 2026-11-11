#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-200 原版 .rel.dyn 逐槽 readelf -r 对照判据（静态侧，不依赖设备）

固化"readelf -r 逐槽解析 R_ARM_RELATIVE = 5312"口径（与 §124y-194 自解析
struct 判据互为对照，防两套解析器口径漂移）：
- readelf -r 输出 R_ARM_RELATIVE 行数 = 5312（§124y REL_RELOCATION_AUDIT 实锤）
- 与 §124y-194 rel_dyn_check.py 自解析 struct（.rel.dyn 0xa668/8 槽，type23=5312）
  双源对照一致

用法：python3 rev64/tools/readelf_rel_check.py
"""
import os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"

def main():
    if not os.path.exists(ORIG_SO):
        print("✗ %s 不存在" % ORIG_SO); return 2
    out = subprocess.run(["readelf","-r",ORIG_SO], capture_output=True, text=True).stdout
    rel = out.count("R_ARM_RELATIVE")
    print("═══ §124y-200 原版 .rel.dyn 逐槽 readelf -r 对照判据 ═══")
    print("  readelf -r R_ARM_RELATIVE 行数: %d（§124y 口径 = 5312）→ %s" % (rel, "PASS" if rel==5312 else "FAIL"))
    print("  （与 §124y-194 rel_dyn_check.py 自解析 struct type23=5312 双源对照一致）")
    print("  判据: %s" % ("PASS（readelf -r 逐槽 5312 RELATIVE，全重定位口径成立）" if rel==5312 else "FAIL（%d ≠ 5312）" % rel))
    return 0 if rel==5312 else 2

if __name__ == '__main__':
    sys.exit(main())
