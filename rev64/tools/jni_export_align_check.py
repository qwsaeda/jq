#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1922 32 位翻译体 JNI 14 入口符号对齐判据（设备无关）

固化 B09-001 口径（AGENTS §124y-195 实锤）：原版 32 位 so = 14 个
Java_com_cnvcs_App_* JNI 导出符号，翻译体 32 位 so 必须 14 个全量
对齐（名称逐一匹配，VA 可不同——翻译体重编后地址漂移正常，符号名
不变 = JNI 绑定不脱钩）。

判据（4 项全 PASS）：
  ① 原版 so JNI 导出 = 14 个（B09-001 口径，非旧"15 个"误口径）
  ② 翻译体 so JNI 导出 = 14 个
  ③ 符号名集合逐一匹配（14/14，原版 ⊆ 翻译体）
  ④ 翻译体 so 无额外 Java_com 符号（14/14，无多出口）

用法：python3 rev64/tools/jni_export_align_check.py
"""
import os, re, subprocess, sys

ORIG = '/tmp/orig/lib/armeabi/libjunqi.so'
TRANS = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
                     'rev64/dist/libjunqi_arm32_aligned.so')

def jni_syms(so):
    out = subprocess.run(['arm-linux-gnueabi-nm', '-D', so], capture_output=True, text=True).stdout
    syms = set()
    for m in re.finditer(r'\b(Java_com_[A-Za-z0-9_]+)\b', out):
        syms.add(m.group(1))
    return syms

def main():
    orig = jni_syms(ORIG)
    trans = jni_syms(TRANS)
    fails = []

    # ① 原版 14
    if len(orig) != 14:
        fails.append('① 原版 JNI 导出 = %d ≠ 14（B09-001 口径）' % len(orig))
    else:
        print('① 原版 JNI 导出 = 14 OK（B09-001 口径）')

    # ② 翻译体 14
    if len(trans) != 14:
        fails.append('② 翻译体 JNI 导出 = %d ≠ 14' % len(trans))
    else:
        print('② 翻译体 JNI 导出 = 14 OK')

    # ③ 逐一匹配（原版 ⊆ 翻译体）
    missing = orig - trans
    if missing:
        fails.append('③ 翻译体缺 %d 个 JNI 符号：%s' % (len(missing), sorted(missing)))
    else:
        print('③ 符号名逐一匹配 14/14 OK（原版 ⊆ 翻译体）')

    # ④ 无多出口
    extra = trans - orig
    if extra:
        fails.append('④ 翻译体多 %d 个 JNI 符号：%s（JNI 绑定脱钩风险）' % (len(extra), sorted(extra)))
    else:
        print('④ 无多出口 OK（翻译体 14/14 = 原版 14/14）')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（JNI 14 入口全量对齐，B09-001 口径固化）' if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
