import ast, marshal, dis, sys, os
_BASE=os.path.dirname(os.path.abspath(__file__))
if os.path.isdir(os.path.join(_BASE,'x')):
    PYC=f'x/main_pyz'; SRC='reconstructed'
elif os.path.isdir(os.path.join(_BASE,'gt_pyz')):
    PYC=f'gt_pyz'; SRC='.'
else:
    PYC='/tmp/opencode/tqc_exe/x/main_pyz'; SRC='/tmp/opencode/tqc_exe/reconstructed'
def find_path(c, names):
    if not names: return c
    name=names[0]
    for k in c.co_consts:
        if hasattr(k,'co_code') and k.co_name==name:
            r=find_path(k,names[1:])
            if r: return r
    return None
mod=sys.argv[1]
path=sys.argv[2].split('/')
orig=marshal.loads(open(f'{PYC}/{mod}.pyc','rb').read())
o=find_path(orig,path)
try:
    rc=compile(open(f'{SRC}/{mod}.py',encoding='utf-8').read(),'x','exec')
    r=find_path(rc,path)
except Exception as e:
    r=None
print(f"=== {mod}.{'.'.join(path)} ===")
if o is None:
    print("原版缺"); sys.exit(0)
print(f"原版: arg={o.co_argcount} nlocals={o.co_nlocals} names={o.co_names} varnames={o.co_varnames}")
print(f"原版 doc: {o.co_consts[0] if o.co_consts and isinstance(o.co_consts[0],str) else '(无)'}")
print("--- 原版 dis ---")
dis.dis(o)
if r is not None:
    print(f"--- 重建 dis (arg={r.co_argcount} nlocals={r.co_nlocals}) ---")
    dis.dis(r)
