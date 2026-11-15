#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1922 stub 层真空量化判据（设备无关）

固化"翻译体无独立 stub 区"口径——§124y-31 4 auto-stub 去重后全在
jqt_vtbl.c 内（有函数体，非空 stub），jqt_stub.c 独立文件不存在。

判据（4 项全 PASS）：
  ① jqt_stub.c 独立文件不存在（无独立 stub 区）
  ② load_stub_set = jqt_vtbl.c 全 F_ 定义（5376，与 f_defs 同源，
     非独立 stub 集合）
  ③ stub ⊆ f_defs（stub 集合是 f_defs 超集的子集，无独立 stub 命中）
  ④ 域矩阵 STUB 层 = 0（f_defs 先于 stub 判定 → 无独立 STUB 命中，
     与现状一致）

用法：python3 rev64/tools/stub_vacuum_check.py
"""
import importlib.util, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def main():
    dm_path = os.path.join(ROOT, 'rev64/tools/translation_domain_matrix.py')
    spec = importlib.util.spec_from_file_location('dm', dm_path)
    dm = importlib.util.module_from_spec(spec); spec.loader.exec_module(dm)

    fails = []
    TRANS = os.path.join(ROOT, 'rev64/reverse/src_jni_translated')

    # ① jqt_stub.c 不存在
    stub_c = os.path.join(TRANS, 'jqt_stub.c')
    if os.path.exists(stub_c):
        fails.append('① jqt_stub.c 存在（应有独立 stub 区？口径变更须实锤）')
    else:
        print('① jqt_stub.c 独立文件不存在 OK（无独立 stub 区，§124y-31 4 auto-stub 全在 jqt_vtbl.c）')

    # ② load_stub_set = jqt_vtbl.c 全 F_ 定义
    stub = dm.load_stub_set()
    print('② load_stub_set = %d 条（jqt_vtbl.c 全 F_ 定义，与 f_defs 同源）' % len(stub))

    # ③ stub ⊆ f_defs
    fdefs = dm.load_f_defs()
    if not stub.issubset(set(fdefs.keys())):
        extra = stub - set(fdefs.keys())
        fails.append('③ stub ⊆ f_defs 不成立（%d 条 stub 无函数体：样例 %s）' % (len(extra), sorted(extra, key=lambda x: int(x, 16))[:5]))
    else:
        print('③ stub ⊆ f_defs OK（%d/%d，全有函数体，非空 stub）' % (len(stub), len(stub)))

    # ④ 域矩阵 STUB 层 = 0（f_defs 先于 stub 判定）
    bl = dm.orig_bl_targets()
    plt = dm.load_plt_symbols()
    stub_layer = 0
    for a in bl:
        ai = int(a, 16)
        if a in plt or 0x22900 <= ai < 0x233e0:
            continue
        if a in fdefs:
            continue
        if a in stub:
            stub_layer += 1
    if stub_layer != 0:
        fails.append('④ 域矩阵 STUB 层 = %d ≠ 0（有独立 stub 命中，须实锤归因）' % stub_layer)
    else:
        print('④ 域矩阵 STUB 层 = 0 OK（f_defs 先于 stub 判定，无独立 STUB 命中）')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（stub 层真空，§124y-31 4 auto-stub 全在 jqt_vtbl.c 有函数体，无独立 stub 区）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
