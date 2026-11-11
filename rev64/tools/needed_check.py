#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-202 原版 so NEEDED 依赖库清单判据（静态侧，不依赖设备）

固化"原版 32 位 so NEEDED 依赖 7 个系统库"口径（§124y-197 got_plt 补强，
防依赖清单漂移）：
- readelf -d 实锤 NEEDED = libc/libstdc++/libm/liblog/libGLESv1_CM/libGLESv2/libdl
  （7 个系统库）+ SONAME = libjunqi.so
- 判据① NEEDED 7 库全在（缺一个 = 依赖漂移）
- 判据② SONAME = libjunqi.so（原版 so 自标识）

用法：python3 rev64/tools/needed_check.py
"""
import os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"
NEEDED = ["libc.so","libstdc++.so","libm.so","liblog.so","libGLESv1_CM.so","libGLESv2.so","libdl.so"]

def main():
    if not os.path.exists(ORIG_SO):
        print("✗ %s 不存在" % ORIG_SO); return 2
    out = subprocess.run(["readelf","-d",ORIG_SO], capture_output=True, text=True).stdout
    got = set()
    soname = ""
    for line in out.splitlines():
        if "NEEDED" in line:
            import re
            m = re.search(r'\[([^\]]+)\]', line)
            if m: got.add(m.group(1))
        if "SONAME" in line:
            m = re.search(r'\[([^\]]+)\]', line)
            if m: soname = m.group(1)
    miss = [n for n in NEEDED if n not in got]
    print("═══ §124y-202 原版 so NEEDED 依赖库清单判据 ═══")
    print("  NEEDED 依赖: %d 个（§124y 口径 = 7 个系统库）" % len(got))
    for n in NEEDED:
        print("    %-16s %s" % (n, "✓" if n in got else "✗ 缺失"))
    print("  SONAME: %s" % soname)
    print("  判据: %s" % ("PASS（7 NEEDED 全在 + SONAME=libjunqi.so）" if not miss and soname=="libjunqi.so" else "FAIL（缺 %s / SONAME=%s）" % (miss, soname)))
    return 0 if not miss and soname=="libjunqi.so" else 2

if __name__ == '__main__':
    sys.exit(main())
