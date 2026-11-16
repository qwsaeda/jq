#!/usr/bin/env python3.12
# -*- coding: utf-8 -*-
"""缺失函数清单: 原版 pyc 中所有 code object(含嵌套) 未在重建源码中出现的。

对齐策略: 递归结构对齐(同名字按顺序配对), 报告原版独有路径。
用法: python3.12 missing_inventory.py [模块...]
"""
import marshal
import os
import sys
from collections import Counter

# 布局自适应(同 verify_all_modules.py)
import os
_BASE = os.path.dirname(os.path.abspath(__file__))
if os.path.isdir(os.path.join(_BASE, 'x')):
    BASE = _BASE; MAIN_PYZ = os.path.join(_BASE, 'x/main_pyz'); RECON = os.path.join(_BASE, 'reconstructed')
elif os.path.isdir(os.path.join(_BASE, 'gt_pyz')):
    BASE = _BASE; MAIN_PYZ = os.path.join(_BASE, 'gt_pyz'); RECON = _BASE
else:
    BASE = '/tmp/opencode/tqc_exe'; MAIN_PYZ = os.path.join(BASE, 'x/main_pyz'); RECON = os.path.join(BASE, 'reconstructed')

MODULES = [
    'client_config', 'client_runner', 'client_ui', 'cloud_client',
    'driver_check', 'runner_worker', 'sig_verify', 'workflow_steps',
    '研发扫AW', '第一步', '第二步', '第三步', '第四步', '第五步',
]


def group_by_name(codes):
    d = {}
    for k in codes:
        d.setdefault(k.co_name, []).append(k)
    return d


def align(oc, rc, path=''):
    pairs = []
    only_o = []
    only_r = []
    ocs = [k for k in oc.co_consts if hasattr(k, 'co_code')]
    rcs = [k for k in rc.co_consts if hasattr(k, 'co_code')]
    og = group_by_name(ocs)
    rg = group_by_name(rcs)
    for name, olst in og.items():
        rlst = rg.get(name, [])
        if not rlst:
            only_o.extend(f'{path}/{name}#{i}' for i in range(len(olst)))
            continue
        for i in range(max(len(olst), len(rlst))):
            if i < len(olst) and i < len(rlst):
                p = f'{path}/{name}'
                if max(len(olst), len(rlst)) > 1:
                    p += f'#{i}'
                pairs.append((p, olst[i], rlst[i]))
            elif i < len(olst):
                only_o.append(f'{path}/{name}#{i}')
            else:
                only_r.append(f'{path}/{name}#{i}')
    for name, rlst in rg.items():
        if name not in og:
            only_r.extend(f'{path}/{name}#{i}' for i in range(len(rlst)))
    return pairs, only_o, only_r


def collect_missing(mod):
    pyc = os.path.join(MAIN_PYZ, f'{mod}.pyc')
    srcfile = os.path.join(RECON, f'{mod}.py')
    if not os.path.exists(pyc) or not os.path.exists(srcfile):
        return []
    orig = marshal.loads(open(pyc, 'rb').read())
    recon = compile(open(srcfile, encoding='utf-8').read(), f'{mod}.py', 'exec')
    pairs, only_o, only_r = align(orig, recon)
    all_missing = list(only_o)
    stack = list(pairs)
    while stack:
        p, oc_, rc_ = stack.pop()
        sub, so, sr = align(oc_, rc_, p)
        stack.extend(sub)
        all_missing.extend(so)
    return sorted(all_missing)


def main():
    mods = sys.argv[1:] or MODULES
    grand = 0
    for mod in mods:
        miss = collect_missing(mod)
        if miss:
            grand += len(miss)
            print(f'=== {mod}: 缺 {len(miss)} ===')
            for m in miss:
                print(f'  {m}')
    print(f'\n总计缺失 code object: {grand}')


if __name__ == '__main__':
    main()
