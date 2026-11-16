#!/usr/bin/env python3
"""global_ref_check.py — 模块级全局引用完整性判据（补 missing_inventory 的盲区）

为什么需要: missing_inventory 只查"code object 缺没缺", 查不出
"模块级常量/导入缺失" —— 这类缺陷在字节码里表现为 LOAD_GLOBAL 指向一个
模块级从未 STORE_NAME 过的名字 → 运行时 NameError。

判据:
  1) 编译模块, 递归取所有 code object 的 LOAD_GLOBAL argval(= 真实全局引用)
  2) 收集模块级定义名: 顶层 def/class/assign/import + 顶层 if/try/with/循环体内的
     同样赋值(模块级作用域) + import
  3) 差集减去内建名 = 疑似 NameError
  4) 与 GT(原 pyc) 模块级 STORE_NAME 集合交叉确认:
     - GT 模块级有该名, recon 无 → ★ 真缺陷(recon 漏定义)
     - GT 也无 → 该名在 GT 里也是运行期由 global 语句赋值(非缺陷), 需人工确认

用法:
    python3 global_ref_check.py                # 全模块
    python3 global_ref_check.py 第五步          # 单模块
    python3 global_ref_check.py --json
"""
import ast, builtins, dis, glob, json, marshal, os, sys

BASE = os.path.dirname(os.path.abspath(__file__))
if os.path.isdir(os.path.join(BASE, 'x')):
    PYC = os.path.join(BASE, 'x', 'main_pyz'); RECON = os.path.join(BASE, 'reconstructed')
elif os.path.isdir(os.path.join(BASE, 'gt_pyz')):
    PYC = os.path.join(BASE, 'gt_pyz'); RECON = BASE
else:
    PYC = '/tmp/opencode/tqc_exe/x/main_pyz'; RECON = '/tmp/opencode/tqc_exe/reconstructed'

BUILTINS = set(dir(builtins))
# 模块级"赋值目标"收集: 递归进入 if/try/with/for/while 的 body(但不进 def/class)
def collect_defined(tree):
    defined = set()
    def scan(body):
        for n in body:
            if isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)):
                defined.add(n.name)
            elif isinstance(n, ast.Assign):
                for t in n.targets:
                    for x in ast.walk(t):
                        if isinstance(x, ast.Name): defined.add(x.id)
            elif isinstance(n, (ast.AnnAssign, ast.AugAssign)):
                t = n.target
                if isinstance(t, ast.Name): defined.add(t.id)
            elif isinstance(n, (ast.Import, ast.ImportFrom)):
                for a in n.names: defined.add((a.asname or a.name).split('.')[0])
            elif isinstance(n, (ast.If, ast.Try, ast.With, ast.AsyncWith)):
                scan(n.body)
                for h in getattr(n, 'handlers', []): scan(h.body)
                scan(getattr(n, 'orelse', []))
                scan(getattr(n, 'finalbody', []))
            elif isinstance(n, (ast.For, ast.AsyncFor, ast.While)):
                scan(n.body); scan(n.orelse)
    scan(tree.body)
    return defined

def load_globals(code):
    used = set()
    def walk(c):
        for i in dis.get_instructions(c):
            if i.opname == 'LOAD_GLOBAL': used.add(i.argval)
            if isinstance(i.argval, type(code)): walk(i.argval)
    walk(code)
    return used

def gt_module_names(mod):
    p = os.path.join(PYC, mod + '.pyc')
    if not os.path.exists(p): return None
    orig = marshal.loads(open(p, 'rb').read())
    return {i.argval for i in dis.get_instructions(orig) if i.opname == 'STORE_NAME'}

def check(mod):
    src = open(os.path.join(RECON, mod + '.py'), encoding='utf-8').read()
    tree = ast.parse(src)
    defined = collect_defined(tree)
    code = compile(src, mod, 'exec')
    used = load_globals(code)
    missing = sorted(x for x in used - defined - BUILTINS if not x.startswith('__'))
    gt = gt_module_names(mod)
    out = []
    for n in missing:
        in_gt = (gt is not None and n in gt)
        out.append({'name': n, 'gt_module_level': in_gt,
                    'verdict': '★真缺陷(recon漏定义)' if in_gt else '运行期赋值(需人工确认)'})
    return {'module': mod, 'missing': out,
            'real': [o for o in out if o['verdict'].startswith('★')],
            'runtime': [o for o in out if not o['verdict'].startswith('★')]}

def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    mods = args or sorted(os.path.basename(p)[:-3] for p in glob.glob(os.path.join(RECON, '*.py')))
    allr = []
    for m in mods:
        r = check(m); allr.append(r)
        if r['missing']:
            print(f"★ {m}:")
            for o in r['missing']:
                print(f"    {o['name']:32} {o['verdict']}")
    tot_real = sum(len(r['real']) for r in allr)
    tot_rt = sum(len(r['runtime']) for r in allr)
    print(f"\n合计: ★真缺陷 {tot_real} / 运行期赋值 {tot_rt}")
    if '--json' in sys.argv:
        print(json.dumps(allr, ensure_ascii=False, indent=1))

if __name__ == '__main__':
    main()
