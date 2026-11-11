#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-203 翻译体 so NEEDED 依赖面对照判据（静态侧，不依赖设备）

固化"翻译体 so 依赖面 ⊆ 原版 7 系统库"口径（§124y-202 needed 判据的翻译体侧对照，
防翻译体 so 引入新外部依赖）：
- 原版 so NEEDED = 7 系统库（libc/libstdc++/libm/liblog/libGLESv1_CM/libGLESv2/libdl）
- 翻译体 so rev64/dist/libjunqi_arm32_aligned.so NEEDED = 6（去 libstdc++ 32 位）
- 判据① 翻译体 NEEDED ⊆ 原版 NEEDED（依赖面收窄，不引入新库）
- 判据② 翻译体 NEEDED 不含 libstdc++（32 位 C++ 运行时剥离 = 翻译体纯 C 依赖面）

用法：python3 rev64/tools/translated_needed_check.py
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"
TRANS_SO = os.path.join(ROOT, "rev64/dist/libjunqi_arm32_aligned.so")

def needed(so):
    out = subprocess.run(["readelf","-d",so], capture_output=True, text=True).stdout
    s = set()
    for line in out.splitlines():
        if "NEEDED" in line:
            m = re.search(r'\[([^\]]+)\]', line)
            if m: s.add(m.group(1))
    return s

def main():
    for p in (ORIG_SO, TRANS_SO):
        if not os.path.exists(p):
            print("✗ %s 不存在" % p); return 2
    orig = needed(ORIG_SO)
    trans = needed(TRANS_SO)
    extra = sorted(trans - orig)
    stdcxx = "libstdc++.so" in trans
    print("═══ §124y-203 翻译体 so NEEDED 依赖面对照判据 ═══")
    print("  原版 so NEEDED: %d 个 %s" % (len(orig), sorted(orig)))
    print("  翻译体 so NEEDED: %d 个 %s" % (len(trans), sorted(trans)))
    print("  翻译体引入新库（⊄ 原版）: %d %s" % (len(extra), extra if extra else "（无）"))
    print("  翻译体含 libstdc++ 32 位: %s（纯 C 依赖面剥离）" % ("✗ 仍在" if stdcxx else "✓ 已剥离"))
    ok = (not extra) and (not stdcxx)
    print("  判据: %s" % ("PASS（翻译体依赖面 ⊆ 原版 7 系统库 + libstdc++ 已剥离）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
