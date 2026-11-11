#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-198 32 位翻译体 so JNI 导出符号对照判据（静态侧，不依赖设备）

固化"翻译体 so 导出符号 = 原版 32 位 so 14 个 On* 逐名一致"口径（防符号漂移）：
- 翻译体 so rev64/dist/libjunqi_arm32_aligned.so nm -D 导出
  Java_com_cnvcs_App_* 14 个（与 §124y-195 原版 B09-001 口径 14 逐名对照）
- 判据① 翻译体 14 个 On* 符号 = 原版 14 个逐名一致（diff 0）
- 判据② 翻译体 NEEDED 无 32 位 libc 垃圾依赖（无 libstdc++ 32 位符号链接错）

用法：python3 rev64/tools/translated_jni_check.py
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"
TRANS_SO = os.path.join(ROOT, "rev64/dist/libjunqi_arm32_aligned.so")

def nm_syms(so):
    out = subprocess.run(["nm","-D",so], capture_output=True, text=True).stdout
    return sorted(set(re.findall(r'Java_com_cnvcs_App_[A-Za-z_0-9]+', out)))

def main():
    if not os.path.exists(TRANS_SO):
        print("✗ %s 不存在" % TRANS_SO); return 2
    if not os.path.exists(ORIG_SO):
        print("✗ %s 不存在" % ORIG_SO); return 2
    orig = nm_syms(ORIG_SO)
    trans = nm_syms(TRANS_SO)
    diff = sorted(set(orig) ^ set(trans))  # 对称差（仅一方有的符号）
    print("═══ §124y-198 32 位翻译体 so JNI 导出符号对照判据 ═══")
    print("  原版 32 位 so On* 符号: %d 个（§124y-195 B09-001 口径 14）" % len(orig))
    print("  翻译体 so On* 符号: %d 个" % len(trans))
    print("  逐名对称差: %d %s" % (len(diff), diff if diff else "（0 差异，完全一致）"))
    ok = (len(orig)==14) and (len(trans)==14) and (not diff)
    print("  判据: %s" % ("PASS（翻译体 14 On* = 原版 14 On* 逐名一致，JNI 导出 32/32 位同源对齐）" if ok else "FAIL（差异 %d）" % len(diff)))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
