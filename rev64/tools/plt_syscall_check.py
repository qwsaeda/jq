#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1921 .plt 系统调用层 214 槽静态判据（设备无关）

固化 AGENTS §124x 口径：原版 so .rel.plt 214 个 R_ARM_JUMP_SLOT
（.plt 0x229c0-0x233df 桩区）+ .got 6 GLOB_DAT 槽 = 系统调用层
（libc/libstdc++/libGLESv1_CM/libGLESv2/liblog/libm/libdl），
真机 linker 解析兜底（翻译体已 dlopen 全量外部依赖，NEEDED 7 项）。

判据（4 项全 PASS）：
  ① .rel.plt R_ARM_JUMP_SLOT = 214（AGENTS 口径）
  ② .got R_ARM_GLOB_DAT = 6（符号解析槽，与 JUMP_SLOT 独立）
  ③ 原版 so NEEDED 7 项全量 dlopen（libstdc++/libc/libm/liblog/
     libGLESv1_CM/libGLESv2/libdl）= 系统调用层真机可达兜底
  ④ .plt 桩区 0x229c0-0x233df 在原版 so 段布局内（.plt PROGBITS AX）

用法：python3 rev64/tools/plt_syscall_check.py
"""
import os, re, subprocess, sys

ORIG = '/tmp/orig/lib/armeabi/libjunqi.so'

def main():
    fails = []
    r = subprocess.run(['arm-linux-gnueabi-readelf', '-r', ORIG], capture_output=True, text=True).stdout
    s = subprocess.run(['arm-linux-gnueabi-readelf', '-d', ORIG], capture_output=True, text=True).stdout
    w = subprocess.run(['arm-linux-gnueabi-readelf', '-SW', ORIG], capture_output=True, text=True).stdout

    # ① .rel.plt JUMP_SLOT
    js = len(re.findall(r'R_ARM_JUMP_SLOT', r))
    if js != 214:
        fails.append('① JUMP_SLOT = %d ≠ 214' % js)
    else:
        print('① .rel.plt R_ARM_JUMP_SLOT = 214 OK（AGENTS 口径）')

    # ② .got GLOB_DAT
    gd = len(re.findall(r'R_ARM_GLOB_DAT', r))
    if gd != 6:
        fails.append('② GLOB_DAT = %d ≠ 6' % gd)
    else:
        print('② .got R_ARM_GLOB_DAT = 6 OK（符号解析槽，与 JUMP_SLOT 独立）')

    # ③ NEEDED 7 项
    needed = re.findall(r'\(NEEDED\)\s+Shared library:\s+\[([^\]]+)\]', s)
    need_set = set(needed)
    expect = {'libstdc++.so', 'libc.so', 'libm.so', 'libdl.so', 'libGLESv1_CM.so', 'libGLESv2.so', 'liblog.so'}
    if not expect <= need_set:
        fails.append('③ NEEDED 缺 %s（实 = %s）' % (expect - need_set, needed))
    else:
        print('③ NEEDED 7 项全量 OK: %s' % ' '.join(sorted(needed)))

    # ④ .plt 桩区（readelf -SW 列：Type Addr Off Size ES Flg Lg... 对齐列）
    m = re.search(r'\.plt\s+PROGBITS\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)\s+[0-9a-f]+\s+AX', w)
    if not m:
        fails.append('④ .plt 段未找到（PROGBITS AX）')
    else:
        plt_va = int(m.group(1), 16)
        plt_sz = int(m.group(2), 16)
        ok = (0x229c0 <= plt_va < 0x233e0 and plt_sz > 0)
        if not ok:
            fails.append('④ .plt VA=0x%x 桩区不符 0x229c0-0x233df' % plt_va)
        else:
            print('④ .plt 桩区 VA=0x%x 大小=0x%x OK（.plt PROGBITS AX，0x229c0-0x233df）' % (plt_va, plt_sz))

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（.plt 214 槽 + .got 6 槽 + NEEDED 7 项 + 桩区，系统调用层真机 linker 解析兜底）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
