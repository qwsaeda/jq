#!/usr/bin/env python3.12
# -*- coding: utf-8 -*-
"""全集保真度验证 v2: 原版 pyc vs 重建源码, 递归结构对齐 + 全量 code object 差分。

对齐策略 (修复 v1 的 flat dict 碰撞丢失):
  - 顶层/每层 co_consts 中的 code objects 按 co_name 分组, 同名字按出现顺序配对。
  - 递归下钻 (类→方法, 函数→嵌套闭包/genexpr)。
  - 报告: 原版独有的 code object (重建缺失 — 潜在缺口) / 重建独有的 (多余)。

两层差分:
  - seq: 字节码级 (跳转目标归一 TGT / LOAD_CONST repr / MAKE_FUNCTION flag / CALL n,
        集合常量排序规范化)。
  - sem: 计算流级 (去跳转/NOP/END_FOR/END_FINALLY/EXTENDED_ARG, 保留常量值/名字/CALL数/
        运算, 集合常量排序规范化)。
"""
import dis
import marshal
import os
import sys
from collections import Counter

# 布局自适应: 工作区(x/main_pyz + reconstructed/) 或 项目扁平目录(同级 .py + 可选 gt_pyz/)
import os
BASE = os.path.dirname(os.path.abspath(__file__))
if os.path.isdir(os.path.join(BASE, 'x')):
    MAIN_PYZ = os.path.join(BASE, 'x', 'main_pyz')
    RECON = os.path.join(BASE, 'reconstructed')
elif os.path.isdir(os.path.join(BASE, 'gt_pyz')):
    MAIN_PYZ = os.path.join(BASE, 'gt_pyz')
    RECON = BASE
else:
    # 回退到工作区绝对路径(若存在)
    MAIN_PYZ = '/tmp/opencode/tqc_exe/x/main_pyz'
    RECON = '/tmp/opencode/tqc_exe/reconstructed'

MODULES = [
    'client_config', 'client_runner', 'client_ui', 'cloud_client',
    'driver_check', 'runner_worker', 'sig_verify', 'workflow_steps',
    '研发扫AW', '第一步', '第二步', '第三步', '第四步', '第五步',
]


def canon(v):
    if isinstance(v, frozenset):
        return ('frozenset', tuple(sorted(repr(x) for x in v)))
    if isinstance(v, set):
        return ('set', tuple(sorted(repr(x) for x in v)))
    return v


def seq(code):
    out = []
    for i in dis.get_instructions(code):
        op = i.opname
        if op.startswith(('JUMP', 'POP_JUMP', 'FOR_ITER')):
            out.append(f'{op} TGT')
        elif op == 'LOAD_CONST':
            a = i.argval
            if isinstance(a, str):
                out.append(f'CONST {a!r}')
            elif isinstance(a, tuple):
                out.append('CONST ' + repr(tuple(repr(x) for x in a)))
            elif isinstance(a, (frozenset, set)):
                out.append(f'CONST {canon(a)}')
            elif hasattr(a, 'co_code'):
                out.append(f'CONST_CODE {a.co_name}')
            else:
                out.append(f'CONST {a!r}')
        elif op in ('COMPARE_OP', 'IS_OP'):
            out.append(f'{op} {i.argrepr}')
        elif op == 'MAKE_FUNCTION':
            out.append(f'{op} {i.argrepr.split(",")[0]}')
        elif op == 'CALL':
            out.append(f'CALL n={i.arg}')
        elif op in ('LOAD_GLOBAL', 'LOAD_NAME', 'LOAD_DEREF', 'LOAD_FAST', 'STORE_FAST',
                    'STORE_DEREF', 'STORE_GLOBAL', 'STORE_NAME', 'LOAD_ATTR',
                    'LOAD_METHOD', 'IMPORT_NAME', 'IMPORT_FROM', 'DELETE_FAST',
                    'DELETE_DEREF', 'STORE_ATTR', 'LOAD_CLOSURE', 'MAKE_CELL'):
            out.append(f'{op} {i.argval}')
        else:
            out.append(op)
    return out


def sem(code):
    out = []
    for i in dis.get_instructions(code):
        op = i.opname
        if op.startswith(('JUMP', 'POP_JUMP', 'FOR_ITER')):
            continue
        if op in ('NOP', 'END_FOR', 'END_FINALLY', 'POP_BLOCK', 'EXTENDED_ARG'):
            continue
        if op == 'LOAD_CONST':
            a = i.argval
            if isinstance(a, str):
                out.append(f'C {a!r}')
            elif isinstance(a, tuple):
                out.append('C ' + repr(tuple(repr(x) for x in a)))
            elif isinstance(a, (frozenset, set)):
                out.append(f'C {canon(a)}')
            elif hasattr(a, 'co_code'):
                out.append(f'C_CODE {a.co_name}')
            else:
                out.append(f'C {a!r}')
        elif op == 'CALL':
            out.append(f'CALL{i.arg}')
        elif op in ('BINARY_OP', 'UNARY_OP'):
            out.append(f'{op}{i.argrepr}')
        elif op in ('COMPARE_OP', 'IS_OP', 'CONTAINS_OP'):
            out.append(f'{op}{i.argrepr}')
        elif op == 'MAKE_FUNCTION':
            out.append(f'MF{i.argrepr.split(",")[0]}')
        elif op in ('LOAD_GLOBAL', 'LOAD_NAME', 'LOAD_DEREF', 'LOAD_FAST', 'LOAD_ATTR',
                    'IMPORT_NAME', 'IMPORT_FROM', 'STORE_FAST', 'STORE_ATTR',
                    'STORE_DEREF', 'STORE_NAME', 'STORE_GLOBAL', 'DELETE_FAST',
                    'DELETE_ATTR', 'LOAD_CLOSURE', 'MAKE_CELL', 'COPY_FREE_VARS',
                    'LOAD_METHOD'):
            out.append(f'{op} {i.argval}')
        elif op in ('SWAP', 'COPY'):
            out.append(f'{op}')
        else:
            out.append(op)
    return out


def group_by_name(codes):
    """按 co_name 分组保序: {name: [code, ...]}"""
    d = {}
    for k in codes:
        d.setdefault(k.co_name, []).append(k)
    return d


def align(oc, rc, path=''):
    """递归结构对齐, 返回 (配对列表, 原版独有列表, 重建独有列表)。"""
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
    # 重建独有名字
    for name, rlst in rg.items():
        if name not in og:
            only_r.extend(f'{path}/{name}#{i}' for i in range(len(rlst)))
    return pairs, only_o, only_r


def verify_module(mod):
    pyc = os.path.join(MAIN_PYZ, f'{mod}.pyc')
    srcfile = os.path.join(RECON, f'{mod}.py')
    if not os.path.exists(pyc) or not os.path.exists(srcfile):
        return None
    orig = marshal.loads(open(pyc, 'rb').read())
    src = open(srcfile, encoding='utf-8').read()
    try:
        recon = compile(src, f'{mod}.py', 'exec')
    except SyntaxError as e:
        return {'mod': mod, 'syntax_err': f'L{e.lineno}: {e.msg}'}

    # 递归对齐
    pairs, only_o, only_r = align(orig, recon)
    # 递归收集所有配对
    all_pairs = list(pairs)
    # 下钻已配对
    stack = list(pairs)
    while stack:
        p, oc_, rc_ = stack.pop()
        sub, so, sr = align(oc_, rc_, p)
        all_pairs.extend(sub)
        stack.extend(sub)
        only_o.extend(so)
        only_r.extend(sr)

    # ★ 模块级(<module> 根 code object)纳入配对总数 —— 补"根对象不在 co_consts"盲区。
    # 根 code object 不在任何 co_consts 里, 历史 align() 从不覆盖它 → 模块级常量/导入/
    # 注解/默认值缺失(运行时 NameError 级真缺陷)一直查不出。现把 (<module>) 作为一个
    # 配对路径纳入 all_pairs, 让 seq/sem 总数统计覆盖模块级。
    all_pairs.append(('<module>', orig, recon))

    byte_exact = 0
    sem_diff = []
    byte_names = []
    for p, oc_, rc_ in all_pairs:
        if seq(oc_) == seq(rc_):
            byte_exact += 1
            byte_names.append(p)
        if sem(oc_) != sem(rc_):
            a, b = sem(oc_), sem(rc_)
            k = next((i for i in range(min(len(a), len(b))) if a[i] != b[i]),
                     min(len(a), len(b)))
            sem_diff.append((p, len(a), len(b), k))

    # 模块级 STORE_NAME 差集(GT 有 / recon 无 = 漏定义 → NameError 风险)
    on = {i.argval for i in dis.get_instructions(orig) if i.opname == 'STORE_NAME'}
    rn = {i.argval for i in dis.get_instructions(recon) if i.opname == 'STORE_NAME'}
    mod_missing = sorted(on - rn)
    mod_extra = sorted(rn - on)
    mod_byte = 1 if seq(orig) == seq(recon) else 0

    return {
        'mod': mod,
        'orig_co': len([k for k in orig.co_consts if hasattr(k, 'co_code')]),
        'pairs': len(all_pairs),
        'byte_exact': byte_exact,
        'sem_diff': sem_diff,
        'only_o': only_o,
        'only_r': only_r,
        'mod_byte': mod_byte,
        'mod_missing': mod_missing,
        'mod_extra': mod_extra,
    }


def main():
    print(f'{"模块":<16} {"配对":>5} {"字节码一致":>9} {"计算流差":>8} {"原独有":>7} {"重独有":>7} {"模块级":>8}')
    print('-' * 76)
    tot_pairs = 0
    tot_byte = 0
    tot_sem = 0
    tot_mod_ok = 0
    tot_mod = 0
    tot_mod_missing = 0
    for mod in MODULES:
        r = verify_module(mod)
        if r is None:
            print(f'{mod:<16} (无 pyc 或源文件)')
            continue
        if 'syntax_err' in r:
            print(f'{mod:<16} SYNTAX {r["syntax_err"]}')
            continue
        tot_pairs += r['pairs']
        tot_byte += r['byte_exact']
        tot_sem += len(r['sem_diff'])
        tot_mod += 1
        tot_mod_ok += r['mod_byte']
        tot_mod_missing += len(r['mod_missing'])
        flag = '  <<' if r['sem_diff'] or r['mod_missing'] else ''
        modflag = '模块级✓' if r['mod_byte'] else '模块级✗'
        print(f'{mod:<16} {r["pairs"]:>5} {r["byte_exact"]:>9} '
              f'{len(r["sem_diff"]):>8} {len(r["only_o"]):>7} {len(r["only_r"]):>7} {modflag:>8}{flag}')
        # 原版独有 (重建缺失) — 只列前 8
        if r['only_o']:
            print(f'    原版独有(重建缺): {r["only_o"][:8]}')
        # ★ 模块级缺失名(运行时 NameError 风险)
        if r['mod_missing']:
            print(f'    ★ 模块级名缺失(GT有recon无→NameError): {r["mod_missing"][:10]}')
        # 计算流差异 — 只列前 5
        for p, la, lb, k in r['sem_diff'][:5]:
            print(f'    ~ {p}: 原{la}/重{lb} idx={k}')
    print('-' * 76)
    print(f'总计: 配对 {tot_pairs}, 字节码一致 {tot_byte} '
          f'({tot_byte * 100.0 / tot_pairs:.1f}%), 计算流差异 {tot_sem}')
    print(f'模块级(<module>)比对: {tot_mod_ok}/{tot_mod} 一致, '
          f'模块级名缺失 {tot_mod_missing} 项(★=运行时 NameError 风险)')
    return 1 if (tot_sem or tot_mod_missing) else 0


if __name__ == '__main__':
    sys.exit(main())
