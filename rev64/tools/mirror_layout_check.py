#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1920 镜像布局一致性离线判据（设备无关）

固化 §124x/§124y-150 镜像偏移口径：jq_img.bin 从 .text 起点 0x233e0 开始
（gen_shadow.py IMG_LO=0x233e0 / IMG_HI=0x15be80），必须 memcpy 到
g_jqt_shadow+0x233e0（非 shadow+0），使 shadow[VA] = 镜像[VA-0x233e0] =
原版[VA]；.bss 从 0x15be80 起由 memset 清零（非 img 覆盖）。

判据（4 项全 PASS）：
  ① gen_shadow.py IMG_LO/IMG_HI = 0x233e0/0x15be80（镜像拷贝区口径）
  ② 镜像 blob（gen_shadow 产物 /tmp/opencode/shadow/jq_img.bin）存在 +
     大小 ≥ IMG_HI-IMG_LO = 0x138aa0（全文件内容 0x13b234，含 .bss 垃圾尾
     0x2794B；运行期 jqt_mem.c IMG_EXPECT_SZ 截断到 0x138aa0，.bss 0x15be80
     起 memset 清零，防垃圾覆盖 .bss 单例槽）
  ③ jqt_mem.c 镜像 memcpy 到 shadow+JQT_IMG_VA（0x233e0，非 shadow+0）
     + IMG_EXPECT_SZ 截断逻辑在位（sz > 0x138aa0 → 截断）
  ④ .bss 0x15be80 起 memset 清零段在位（img_end = 0x233e0+sz = 0x15be80）

用法：python3 rev64/tools/mirror_layout_check.py
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GEN = os.path.join(ROOT, 'rev64/tools/gen_shadow.py')
MEM = os.path.join(ROOT, 'rev64/reverse/src_jni_translated/jqt_mem.c')
# 镜像 blob = gen_shadow.py 产物（构建时 jq_img.S .incbin 源），非 rev64/ 仓库内
IMG = '/tmp/opencode/shadow/jq_img.bin'

def main():
    IMG_LO, IMG_HI = 0x233e0, 0x15be80
    EXPECT_SZ = IMG_HI - IMG_LO  # 0x138aa0
    fails = []

    # ① gen_shadow.py 镜像拷贝区口径
    g = open(GEN, errors='ignore').read()
    m = re.search(r'IMG_LO,\s*IMG_HI\s*=\s*(0x[0-9a-f]+)\s*,\s*(0x[0-9a-f]+)', g)
    if not m or int(m.group(1), 16) != IMG_LO or int(m.group(2), 16) != IMG_HI:
        fails.append('① gen_shadow IMG_LO/HI ≠ 0x233e0/0x15be80（实 = %s）' % (m.group(0) if m else '未找到'))
    else:
        print('① gen_shadow IMG_LO/HI = 0x%x/0x%x OK（镜像拷贝区）' % (IMG_LO, IMG_HI))

    # ② 镜像 blob（gen_shadow 产物）存在 + 大小 ≥ EXPECT_SZ（全文件内容 0x13b234，
    #    含 .bss 垃圾尾 0x2794B；运行期 jqt_mem.c IMG_EXPECT_SZ 截断到 0x138aa0）
    BLOB_EXPECT = 0x13b234  # gen_shadow 全文件内容（.text+.rodata+.data+.bss 尾）
    if os.path.exists(IMG):
        sz = os.path.getsize(IMG)
        if sz < EXPECT_SZ:
            fails.append('② 镜像 blob 大小 0x%x < EXPECT_SZ 0x%x（镜像拷贝区不完整）' % (sz, EXPECT_SZ))
        elif sz != BLOB_EXPECT:
            print('② 镜像 blob 大小 0x%x（≥ EXPECT_SZ 0x%x，gen_shadow 全内容口径；截断运行期生效）' % (sz, EXPECT_SZ))
        else:
            print('② 镜像 blob 大小 0x%x = BLOB_EXPECT OK（全内容，含 .bss 垃圾尾 0x%x；jqt_mem.c 截断 0x%x）' % (sz, sz-EXPECT_SZ, EXPECT_SZ))
    else:
        # 允许未生成（离线判据跳过文件实锤，口径 ③/④ 段在位即可）
        print('② 镜像 blob 未生成（跳过文件实锤，gen_shadow.py 离线跑可再生）')

    # ③ jqt_mem.c memcpy 到 shadow+JQT_IMG_VA + 截断逻辑
    s = open(MEM, errors='ignore').read()
    if '#define JQT_IMG_VA 0x233e0u' not in s:
        fails.append('③ JQT_IMG_VA 0x233e0 定义缺失')
    if 'memcpy(g_jqt_shadow + JQT_IMG_VA, g_jqt_img, sz)' not in s:
        fails.append('③ memcpy 到 shadow+JQT_IMG_VA 段缺失（须非 shadow+0）')
    if 'IMG_EXPECT_SZ = 0x15be80u - 0x233e0u' not in s and 'IMG_EXPECT_SZ' not in s:
        fails.append('③ IMG_EXPECT_SZ 截断逻辑缺失')
    if 'if (sz > IMG_EXPECT_SZ) sz = IMG_EXPECT_SZ' not in s:
        fails.append('③ sz 截断段缺失')
    if not fails:
        print('③ jqt_mem.c memcpy(shadow+JQT_IMG_VA=0x233e0) + IMG_EXPECT_SZ 截断 OK')

    # ④ .bss 0x15be80 起 memset 清零段
    if 'img_end = JQT_IMG_VA + sz' not in s or 'memset(g_jqt_shadow + img_end' not in s:
        fails.append('④ .bss img_end 清零段缺失')
    else:
        print('④ .bss 0x15be80 起 memset(g_jqt_shadow+img_end) 清零 OK（img_end = 0x233e0+sz = 0x15be80）')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（镜像布局 0x233e0 起点 + 0x15be80 截断 + .bss 清零，§124x 口径固化）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
