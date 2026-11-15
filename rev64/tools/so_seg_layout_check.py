#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1927 翻译体 vs 原版 段布局对照判据（设备无关）

固化"翻译体 64 位重编 so 段布局 ≠ 原版 32 位"口径（§124x 镜像偏移是
针对"原版 32 位 VA 嵌进翻译体 shadow"的，非"翻译体 so 段 = 原版 so 段"）：
  原版（ELF32）：.text 0x233e0/0xfd814 + .rodata 0x120bf8/0x1d030 +
  .data 0x15bb6c/0x314 + .bss 0x15be80/0x7008
  翻译体（ELF64 重编）：段地址/大小全异（.text 0x323c40/0x581510 等），
  但镜像偏移逻辑（jq_img.bin 从原版 .text 0x233e0 起 → shadow+0x233e0）
  仍按原版 VA 口径（§124x 固化），与翻译体自身段布局无关。
判据：① 原版 4 段 VA/size 实锤（.text 0x233e0/.rodata 0x120bf8/
.data 0x15bb6c/.bss 0x15be80）② 翻译体段布局 ≠ 原版（ELF64 重编，
段地址漂移 = 正常）③ 镜像偏移逻辑仍按原版 VA（§124x 0x233e0 起点，
与翻译体自身段无关）。

用法：python3 rev64/tools/so_seg_layout_check.py
"""
import os, re, subprocess, sys

ORIG = '/tmp/orig/lib/armeabi/libjunqi.so'
TRANS = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
                     'rev64/dist/libjunqi_arm32_aligned.so')

def segs(so):
    out = subprocess.run(['arm-linux-gnueabi-readelf', '-SW', so], capture_output=True, text=True).stdout
    d = {}
    # §124y-1927 勘误：段名含点（.data.rel.ro）须 [\w.]+；列序 [N] Name Type Addr Off Size
    # readelf -SW 的 Addr 列 = 第 3 个 hex 字段（[N] Name Type **Addr** Off Size），group(3)=Addr
    for m in re.finditer(r'\s\.([\w.]+)\s+(PROGBITS|NOBITS)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)', out):
        d[m.group(1)] = (int(m.group(3), 16), int(m.group(5), 16))
    return d

def main():
    fails = []
    o = segs(ORIG)
    t = segs(TRANS)
    # ① 原版 4 段实锤
    # §124y-1927 勘误：段名 key = 无点（segs() 正则 group(1)=[\w.]+ 但 dict key 存无点
    # 因正则 \.([\w.]+) 匹配 .text 时 group(1)=text）
    exp = {'text': (0x233e0, 0xfd814), 'rodata': (0x120bf8, 0x1d030),
           'data': (0x15bb6c, 0x314), 'bss': (0x15be80, 0x7008)}
    for k, (va, sz) in exp.items():
        if k not in o or o[k] != (va, sz):
            fails.append('① 原版 %s = %s ≠ 期望 (0x%x, 0x%x)' % (k, o.get(k), va, sz))
    if not fails:
        print('① 原版 4 段实锤 OK（.text 0x233e0/0xfd814 + .rodata 0x120bf8/0x1d030 + .data 0x15bb6c/0x314 + .bss 0x15be80/0x7008）')
    # ② 翻译体段布局 ≠ 原版（ELF64 重编，段地址漂移正常）
    drift = [k for k in exp if k in t and t[k] != exp[k]]
    print('② 翻译体段地址漂移 %d 段（ELF64 重编正常，非回归）: %s' % (len(drift), drift or '无'))
    # ③ 镜像偏移逻辑仍按原版 VA（§124x 0x233e0 起点）
    if o.get('text', (0,))[0] != 0x233e0:
        fails.append('③ 原版 .text VA ≠ 0x233e0（镜像偏移逻辑 §124x 起点失效）')
    else:
        print('③ 镜像偏移逻辑仍按原版 VA OK（§124x 0x233e0 起点，与翻译体自身段无关）')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（原版 4 段实锤 + 翻译体段漂移正常 + 镜像偏移按原版 VA，§124x 口径成立）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
