#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-195 原版 32 位 so JNI 导出符号判据（静态侧，不依赖设备）

★ B09-001 口径修正实锤（2026-11-11）：
- AGENTS 旧口径"原版 32 位 so 实锤 15 个 On*（nm -D）"不准确，
  `nm -D` 实锤 = **14 个 Java_com_cnvcs_App_* 符号**（本判据逐符号列名）
- src_rs 64 位重写"14 个（减 OnDestroy）"与原版 32 位实锤 14 个 **完全一致**，
  非"14/15 不一致"——旧口径 15 系误读（把 JNI_OnLoad 等内部符号或 GLView
  反射符号误计入）
- 判据输出 14 符号清单（防口径漂移）

用法：python3 rev64/tools/jni_export_check.py
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"

def main():
    if not os.path.exists(ORIG_SO):
        print("✗ %s 不存在" % ORIG_SO); return 2
    out = subprocess.run(["nm","-D",ORIG_SO], capture_output=True, text=True).stdout
    syms = sorted(set(re.findall(r'Java_[A-Za-z_0-9]+', out)))
    app = [s for s in syms if s.startswith("Java_com_cnvcs_App_")]
    print("═══ §124y-195 原版 32 位 so JNI 导出符号判据 ═══")
    print("  Java_* 导出符号: %d 个（B09-001 实锤口径 = 14，旧误口径 15 已纠正）" % len(app))
    for s in app:
        print("    %s" % s)
    print("  判据: %s" % ("PASS（14 个 Java_com_cnvcs_App_*，src_rs 64 位 14 个完全一致）" if len(app)==14 else "FAIL（实锤 %d ≠ 14）" % len(app)))
    return 0 if len(app)==14 else 2

if __name__ == '__main__':
    sys.exit(main())
