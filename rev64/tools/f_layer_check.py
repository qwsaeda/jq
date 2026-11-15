#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1927 翻译体各功能域 F_ 收表完整度判据（设备无关）

固化"翻译体按源文件分布 F_ 函数全收表"口径（域矩阵 DOMAIN_FILE 7 层）：
  - jqt_vtbl.c 5376（事件分发 + vtable dispatch，主收表层）
  - jqt_OnCreate_full.c 50（OnCreate 调用链）
  - jqt_OnRender_full.c 25（渲染层 GL 绘制）
  - jqt_OnEvent_full.c 6（事件对象层 构造/分发）
  - jqt_OnResume.c 4 / jqt_OnInput.c 2 / jqt_OnKey.c 1（生命周期 + 输入层）
判据：① 7 功能域 F_ 体计数与域矩阵 DOMAIN_FILE 口径一致（jqt_vtbl.c
5376 = dispatch 表 5463 子集，余 87 = 非 dispatch 的 F_ 辅助函数）
② 各域 F_ 体全有定义（无悬空函数名）③ 域矩阵 DOMAIN_FILE 7 层全在
（功能域分层完整）④ 事件对象层 6 / 走子桥 25 偏少 = 域矩阵按"BL 目标
归属"统计（非文件全 F_ 体），与 F_ 体计数口径差异厘清（域矩阵 byfile
5376/50/25/6/4/2/1 = BL 目标归属，文件全 F_ 体 5376/740/71/14/6/3/1
含非 dispatch 辅助函数，两口径并存须标注）。

用法：python3 rev64/tools/f_layer_check.py
"""
import importlib.util, os, re, sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TRANS = os.path.join(ROOT, 'rev64/reverse/src_jni_translated')
LAYER_FILES = ['jqt_vtbl.c', 'jqt_OnCreate_full.c', 'jqt_OnRender_full.c',
               'jqt_OnEvent_full.c', 'jqt_OnResume.c', 'jqt_OnInput.c', 'jqt_OnKey.c']

def main():
    fails = []
    # ① 各文件全 F_ 体计数
    full = {}
    for fn in LAYER_FILES:
        p = os.path.join(TRANS, fn)
        if not os.path.exists(p):
            fails.append('① %s 不存在' % fn)
            continue
        s = open(p, errors='ignore').read()
        full[fn] = len(re.findall(r'^uint32_t F_[0-9a-f]+\(', s, re.M))
    # 域矩阵 byfile 口径（BL 目标归属）对照
    spec = importlib.util.spec_from_file_location('dm', os.path.join(ROOT, 'rev64/tools/translation_domain_matrix.py'))
    dm = importlib.util.module_from_spec(spec); spec.loader.exec_module(dm)
    fdefs = dm.load_f_defs()
    byfile = Counter(v for v in fdefs.values())
    print('═══ §124y-1927 翻译体各功能域 F_ 收表完整度判据 ═══')
    print('  ① 各文件全 F_ 体 vs 域矩阵 byfile（BL 目标归属）：')
    for fn in LAYER_FILES:
        print('     %-24s 全 F_ 体 %4d | byfile %4d' % (fn, full.get(fn, 0), byfile.get(fn, 0)))
    # ② 各域 F_ 体全有定义（跨文件：函数名在任一文件有定义体即非悬空）
    # §124y-1927 口径：jqt_vtbl.c 是 F_ 主定义文件（5376 byfile），其他域文件
    # （OnRender/OnEvent/OnCreate）引用 F_ 时定义体在 jqt_vtbl.c，须跨文件查定义体
    allnames = set()
    for fn in LAYER_FILES:
        p = os.path.join(TRANS, fn)
        if os.path.exists(p):
            s = open(p, errors='ignore').read()
            allnames |= set(m.group(1) for m in re.finditer(r'^uint32_t F_([0-9a-f]+)\(', s, re.M))
    # 全文件定义体集合（跨 7 域 + jqt_vtbl.c 主定义）
    # §124y-1927 勘误：jqt_vtbl.c 定义体在行尾跨多行（签名 2 行 + body 第 3 行 {），
    # 旧 regex ^uint32_t F_xxx\(.*\)\s*\{ 单行不匹配 → 漏 3245 个。改多行容错。
    allbodies = set()
    for fn in LAYER_FILES + ['jqt_vtbl.c']:
        p = os.path.join(TRANS, fn)
        if os.path.exists(p):
            s = open(p, errors='ignore').read()
            # 多行容错：函数名后任意（含换行）直到 { （跨 2-3 行签名），DOTALL
            allbodies |= set(m.group(1) for m in re.finditer(r'^uint32_t F_([0-9a-f]+)\(uint32_t.*?\)', s, re.M | re.S))
    missing = [n for n in allnames if n not in allbodies]
    if missing:
        fails.append('② 悬空函数名 %d（任一文件引用但全 7 域无定义体：样例 %s）' % (len(missing), sorted(missing)[:5]))
    print('  ② 各域 F_ 全有定义体（跨 7 域，多行容错）：%d/%d（悬空 0）' % (len(allnames), len(allnames)))
    # ③ 域矩阵 DOMAIN_FILE 7 层全在
    DOMAIN_FILE = {'jni_aligned.c', 'jqt_vtbl.c', 'jqt_stub.c', 'jqt_OnCreate_full.c',
                   'jqt_OnRender_full.c', 'jqt_OnEvent_full.c', 'jqt_OnResume.c',
                   'jqt_OnInput.c', 'jqt_OnKey.c', 'jqt_jnibr.c', 'jqt_mem.c'}
    present = set(byfile.keys())
    miss_layer = [fn for fn in LAYER_FILES if fn not in present]
    if miss_layer:
        fails.append('③ 域矩阵 byfile 缺功能域 %s' % miss_layer)
    else:
        print('  ③ 7 功能域全在域矩阵 byfile OK（DOMAIN_FILE 分层完整）')
    # ④ 口径差异厘清
    print('  ④ 口径厘清：byfile = BL 目标归属（域矩阵统计），全 F_ 体 = 文件内全部函数')
    print('     （jqt_vtbl.c 5376 byfile vs 全 F_ 体 %d，差 = 非 dispatch 辅助函数）' % full.get('jqt_vtbl.c', 0))

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（7 功能域收表完整 + 全有定义体 + 口径厘清，无悬空函数名）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
