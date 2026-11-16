#!/usr/bin/env python3
"""reorder_module_level.py — 按 GT 原版模块级 STORE_NAME 顺序重排重建源码的顶层块

背景: 模块级(<module> 根 code object)此前从未纳入判据比对 → 顶层定义顺序
与原版不一致(语义等价但字节码不一致)。本工具按 GT 顺序重排顶层块:
  · 简单赋值 (NAME = ...)
  · 函数/类定义
  · import 语句
  · 其他块(try/if 等有副作用的)保持原相对位置, 锚在最近的同名块之后

用法:
    python3 reorder_module_level.py 第五步 第一步        # 预览(打印计划)
    python3 reorder_module_level.py 第五步 --apply      # 执行重排
"""
import ast, dis, marshal, os, sys

BASE = os.path.dirname(os.path.abspath(__file__))
PYC = (os.path.join(BASE, 'x', 'main_pyz') if os.path.isdir(os.path.join(BASE, 'x'))
       else '/tmp/opencode/tqc_exe/x/main_pyz')
RECON = (os.path.join(BASE, 'reconstructed') if os.path.isdir(os.path.join(BASE, 'reconstructed'))
         else '/tmp/opencode/tqc_exe/reconstructed')

def gt_order(mod):
    orig = marshal.loads(open(os.path.join(PYC, mod + '.pyc'), 'rb').read())
    out = []
    for i in dis.get_instructions(orig):
        if i.opname == 'STORE_NAME' and i.argval not in out:
            out.append(i.argval)
    return out

def blocks(tree, src_lines):
    """返回 [(name_or_None, start_line0, end_line0, kind)]

    ★ 注意: 带装饰器的函数, ast 的 n.lineno 指向 def 行, 装饰器行在其之前 ——
      必须用 min(decorator.lineno) 作为块起点, 否则重排会丢装饰器。
    """
    b = []
    for n in tree.body:
        name = None
        kind = type(n).__name__
        start = n.lineno - 1
        if isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)):
            name = n.name
            if getattr(n, 'decorator_list', None):
                start = min(start, min(d.lineno for d in n.decorator_list) - 1)
        elif isinstance(n, ast.Assign):
            ts = [t.id for t in n.targets if isinstance(t, ast.Name)]
            if len(ts) == 1: name = ts[0]
        elif isinstance(n, (ast.Import, ast.ImportFrom)):
            names = []
            for a in n.names:
                nm = (a.asname or a.name).split('.')[0]
                names.append(nm)
            name = names[0] if len(names) == 1 else None
        b.append((name, start, n.end_lineno, kind))
    return b

def reorder(mod, apply=False):
    path = os.path.join(RECON, mod + '.py')
    src = open(path, encoding='utf-8').read()
    lines = src.split('\n')
    tree = ast.parse(src)
    blks = blocks(tree, lines)
    order = gt_order(mod)
    # 每个块的目标位置: GT 顺序里它的名
    idx = {}
    pos = 0
    for i, (name, s, e, kind) in enumerate(blks):
        if name and name in order:
            idx[i] = order.index(name)
        else:
            idx[i] = None
    # 稳定重排: 有序的按 GT 序; 无名的锚在前一个有名块之后
    plan = []
    last = -1
    for i, (name, s, e, kind) in enumerate(blks):
        if idx[i] is None:
            plan.append((last, i))
        else:
            plan.append((idx[i], i))
            last = idx[i]
    plan_sorted = sorted(plan, key=lambda x: (x[0], x[1]))
    new_order = [i for _, i in plan_sorted]
    changed = new_order != list(range(len(blks)))
    print(f"=== {mod}: {len(blks)} 顶层块, 需重排={changed} ===")
    if not changed:
        return False
    for i in new_order:
        name, s, e, kind = blks[i]
        print(f"   {name or '<无名块>':<28} lines {s+1}-{e}  ({kind})")
    if apply:
        out = []
        for i in new_order:
            name, s, e, kind = blks[i]
            out.extend(lines[s:e])
            out.append('')
        open(path, 'w', encoding='utf-8').write('\n'.join(out))
        print(f"   → 已重排并写回 {path}")
    return True

def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    apply_ = '--apply' in sys.argv
    for m in args:
        reorder(m, apply_)

if __name__ == '__main__':
    main()
