#!/usr/bin/env python3
"""extract_module_consts.py — 从 GT 原 pyc 模块级字节码提取常量/导入真值

用途: 补"模块级(<module> 根 code object)从未被比对"这一判据盲区 ——
从原版 pyc 的模块级指令流里, 还原每个 STORE_NAME 名对应的源级定义语句。

还原策略(按 LOAD/STORE 模式识别):
  * STORE_NAME X 之前是 LOAD_CONST v        →  X = v
  * STORE_NAME X 之前是 IMPORT_NAME m / IMPORT_FROM f →  import 语句
  * STORE_NAME X 之前是 BUILD_xxx (dict/tuple/list) → 复合字面量
  * STORE_NAME X 之前是 CALL/函数调用      →  调用表达式(需人工)
  * STORE_NAME X 之前是 LOAD_NAME/LOAD_ATTR →  引用/别名

用法:
    python3 extract_module_consts.py 第四步            # 打印该模块全部模块级定义
    python3 extract_module_consts.py 第四步 --missing  # 只打印 recon 缺的
"""
import ast, dis, json, marshal, os, sys

BASE = os.path.dirname(os.path.abspath(__file__))
PYC = (os.path.join(BASE, 'x', 'main_pyz') if os.path.isdir(os.path.join(BASE, 'x'))
       else '/tmp/opencode/tqc_exe/x/main_pyz')
RECON = (os.path.join(BASE, 'reconstructed') if os.path.isdir(os.path.join(BASE, 'reconstructed'))
         else '/tmp/opencode/tqc_exe/reconstructed')

def recon_names(mod):
    tree = ast.parse(open(os.path.join(RECON, mod + '.py'), encoding='utf-8').read())
    names = set()
    def scan(body):
        for n in body:
            if isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)):
                names.add(n.name)
            elif isinstance(n, ast.Assign):
                for t in n.targets:
                    for x in ast.walk(t):
                        if isinstance(x, ast.Name): names.add(x.id)
            elif isinstance(n, (ast.Import, ast.ImportFrom)):
                for a in n.names: names.add((a.asname or a.name).split('.')[0])
            elif isinstance(n, (ast.If, ast.Try, ast.With)):
                scan(n.body)
                for h in getattr(n, 'handlers', []): scan(h.body)
                scan(getattr(n, 'orelse', [])); scan(getattr(n, 'finalbody', []))
    scan(tree.body)
    return names

def extract(mod):
    """返回 {name: {'kind':..., 'value':...}}"""
    orig = marshal.loads(open(os.path.join(PYC, mod + '.pyc'), 'rb').read())
    ins = list(dis.get_instructions(orig))
    out = {}
    i = 0
    while i < len(ins):
        if ins[i].opname == 'STORE_NAME':
            name = ins[i].argval
            # 回溯找赋值源
            j = i - 1
            # 跳过 MAKE_FUNCTION / CALL 等
            while j >= 0 and ins[j].opname in ('POP_TOP', 'CALL', 'MAKE_FUNCTION',
                                                'BUILD_LIST', 'BUILD_TUPLE', 'BUILD_MAP',
                                                'BUILD_CONST_KEY_MAP', 'BUILD_SET',
                                                'BUILD_STRING', 'FORMAT_VALUE', 'LIST_EXTEND',
                                                'LIST_APPEND', 'SET_UPDATE', 'DICT_UPDATE',
                                                'DICT_MERGE', 'BUILD_SLICE'):
                j -= 1
            if j < 0: i += 1; continue
            src = ins[j]
            if src.opname == 'LOAD_CONST':
                out[name] = {'kind': 'const', 'value': src.argval}
            elif src.opname == 'IMPORT_NAME':
                out[name] = {'kind': 'import_module', 'value': src.argval}
            elif src.opname == 'IMPORT_FROM':
                # 找前一个 IMPORT_NAME
                m = None
                for k in range(j-1, max(-1, j-6), -1):
                    if ins[k].opname == 'IMPORT_NAME': m = ins[k].argval; break
                out[name] = {'kind': 'import_from', 'module': m, 'value': src.argval}
            elif src.opname in ('LOAD_NAME', 'LOAD_GLOBAL', 'LOAD_ATTR', 'LOAD_FAST'):
                out[name] = {'kind': 'ref', 'value': src.argval, 'op': src.opname}
            elif src.opname == 'BUILD_CONST_KEY_MAP':
                out[name] = {'kind': 'build_const_key_map', 'size': src.argval}
            else:
                out[name] = {'kind': 'op:' + src.opname, 'value': src.argval}
        i += 1
    return out

def main():
    mod = sys.argv[1]
    only_missing = '--missing' in sys.argv
    gt = extract(mod)
    rn = recon_names(mod)
    missing = {k: v for k, v in gt.items() if k not in rn}
    print(f"=== {mod}: GT 模块级名 {len(gt)}, recon 缺 {len(missing)} ===")
    for k, v in sorted(missing.items()):
        print(f"  {k:34} {v}")

if __name__ == '__main__':
    main()
