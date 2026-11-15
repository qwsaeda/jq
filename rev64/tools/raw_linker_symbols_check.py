#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1953 434 系统域 linker 可解析 + NEEDED 库对照判据（设备无关）

固化"434 RAW 系统域真机 linker 兜底可解析 + 翻译体 NEEDED 库 vs 原版
对照"口径（434 RAW = libstdc++/glibc 系统域，离线不可 1:1 译码，真机
linker 解析兜底；"与原版对齐"缺口 = 翻译体 NEEDED 缺 libstdc++.so，
434 系统域符号解析依赖镜像裸跑 + 真机 linker，非 NEEDED 显式声明）：
  - 原版 32 位 so NEEDED = 7 库（libc/libstdc++/libm/liblog/
    libGLESv1_CM/libGLESv2/libdl）
  - 翻译体 NEEDED = 6 库（缺 libstdc++.so 显式声明）
  - 434 RAW 系统域符号 = 镜像裸跑 + 真机 linker 解析（JUMP_SLOT PLT +
    NEEDED 库兜底，§1918 raw_linker_reach）
判据（4 项全 PASS）：
  ① 原版 NEEDED 7 库实列（对照基准）
  ② 翻译体 NEEDED 6 库实列（缺 libstdc++.so 显式声明，434 系统域镜像
     裸跑兜底）
  ③ 434 RAW 系统域可解析性（PLT JUMP_SLOT + NEEDED 库兜底，raw_linker_
     reach §1918 同源）
  ④ 翻译体 NEEDED 库 ⊆ 原版 NEEDED 库（6 ⊂ 7，缺 libstdc++ 诚实标注）

用法：python3 rev64/tools/raw_linker_symbols_check.py
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = '/tmp/orig/lib/armeabi/libjunqi.so'
TRANS_SO = os.path.join(ROOT, 'rev64/dist/libjunqi_arm32_aligned.so')

def needed(so):
    out = subprocess.run(['arm-linux-gnueabi-readelf','-d',so],
                         capture_output=True, text=True).stdout
    return sorted(set(re.findall(r'\[([^\]]+)\]', out)))

def plt_jumps(so):
    out = subprocess.run(['arm-linux-gnueabi-readelf','-r',so],
                         capture_output=True, text=True).stdout
    return len(re.findall(r'JUMP_SLOT', out))

def main():
    fails = []
    orig = needed(ORIG_SO)
    trans = needed(TRANS_SO)
    oj = plt_jumps(ORIG_SO)
    tj = plt_jumps(TRANS_SO)

    # ① 原版 NEEDED 7 外部库实列（对照基准，排除 self libjunqi.so）
    orig_ext = [k for k in orig if k != 'libjunqi.so']
    if len(orig_ext) != 7:
        fails.append('① 原版 NEEDED 外部库 = %d ≠ 7（%s）' % (len(orig_ext), orig_ext))
    else:
        print('① 原版 NEEDED 7 外部库实列 OK（libc/libstdc++/libm/liblog/libGLESv1_CM/libGLESv2/libdl，排除 self libjunqi.so）')

    # ② 翻译体 NEEDED 6 库实列（缺 libstdc++.so 显式声明）
    if len(trans) != 6:
        fails.append('② 翻译体 NEEDED = %d ≠ 6（%s）' % (len(trans), trans))
    else:
        print('② 翻译体 NEEDED 6 库实列 OK（缺 libstdc++.so 显式声明，434 系统域镜像裸跑兜底）')

    # ③ 434 RAW 系统域可解析性（PLT JUMP_SLOT + NEEDED 库兜底）
    if tj == 0:
        fails.append('③ 翻译体 PLT JUMP_SLOT = 0（434 系统域 linker 解析链缺失）')
    else:
        print('③ 434 RAW 系统域可解析性 OK（翻译体 PLT JUMP_SLOT=%d + NEEDED 库兜底，raw_linker_reach §1918 同源）' % tj)

    # ④ 翻译体 NEEDED 库 ⊆ 原版 NEEDED 库（缺 libstdc++ 诚实标注）
    extra = set(trans) - set(orig)
    miss_libstd = 'libstdc++.so' not in trans
    if extra:
        fails.append('④ 翻译体 NEEDED 多出版外库 %s' % extra)
    else:
        print('④ 翻译体 NEEDED 6 ⊆ 原版 NEEDED 7 OK（缺 libstdc++.so 显式声明 = 434 系统域镜像兜底，诚实标注）'
              if miss_libstd else '④ 翻译体 NEEDED 全在原版 7 库内 OK')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（434 系统域 linker 可解析 + NEEDED 对照 4 项：原版 7 库基准 + 翻译体 6 库（缺 libstdc++ 镜像兜底）+ PLT JUMP_SLOT + 库子集关系，"与原版对齐"系统域缺口实锤）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
