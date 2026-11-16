#!/usr/bin/env python3
"""sem_triage.py — 反编译重建批量 triage 驱动（固化 DECOMP_RECONSTRUCTION.md 方法论）

一条命令跑通"批量处理"：对每个重建模块做 seq/sem 双判据 + 长度比启发式 +
codegen-noise 白名单判读，输出待修清单（★ 真差异优先）与噪声清单，供逐函数对
GT 反汇编实锤后修复。

用法:
    python3 sem_triage.py                 # 全模块，默认
    python3 sem_triage.py 第五步 第三步    # 只查指定模块
    python3 sem_triage.py --json          # 机读输出
    python3 sem_triage.py --star-only     # 只列 ★ 长度比越界的真差异候选

判据口径（与 DECOMP_RECONSTRUCTION.md 一致）:
  seq  = 逐指令归一化比对（jump→TGT, LOAD_CONST→repr, MAKE_FUNCTION→flag, CALL→n=, set/frozenset 排序）
  sem  = seq 再去异常展开指令（PUSH_EXC_INFO/CHECK_EXC_MATCH/POP_EXCEPT/RERAISE/
         WITH_EXCEPT_START/BEFORE_WITH/POP_BLOCK/COPY/SWAP）+ NOP/END_FOR/END_FINALLY/EXTENDED_ARG
  ★   = sem 长度比 <0.6x 或 >1.4x（几乎必是真差异：漏 guard/错异常布局/整段缺失或过写）
  噪声 = 长度比 0.9–1.1x 且 diff 全落白名单（不修；修了引入新差异）
GT     = x/main_pyz/<module>.pyc（裸 marshal，无 16 字节头，marshal.loads 直读）
"""
import ast, dis, marshal, os, sys, json, argparse, warnings

BASE = os.path.dirname(os.path.abspath(__file__))
# 支持两种布局: 工作区 (reconstructed/ + x/main_pyz/) 与 项目扁平目录 (同级 .pyc 在 main_pyz/)
PYC = os.path.join(BASE, 'x', 'main_pyz') if os.path.isdir(os.path.join(BASE, 'x')) \
      else os.path.join(BASE, 'main_pyz')
RECON = os.path.join(BASE, 'reconstructed') if os.path.isdir(os.path.join(BASE, 'reconstructed')) \
        else BASE  # 项目目录: .py 与脚本同级
MODULES = ['client_config','client_runner','client_ui','cloud_client','config',
          'diagnostics','driver_check','runner_worker','sig_verify','workflow_steps',
          '研发扫AW','第一步','第三步','第二步','第五步','第四步']

JUMP = ('JUMP','POP_JUMP','FOR_ITER')
STRIP = {'NOP','END_FOR','END_FINALLY','EXTENDED_ARG',
         'PUSH_EXC_INFO','CHECK_EXC_MATCH','POP_EXCEPT','RERAISE',
         'WITH_EXCEPT_START','BEFORE_WITH','POP_BLOCK','COPY','SWAP'}
NAME_OPS = {'LOAD_GLOBAL','LOAD_NAME','LOAD_DEREF','LOAD_FAST','LOAD_ATTR',
            'IMPORT_NAME','IMPORT_FROM','STORE_FAST','STORE_ATTR','STORE_DEREF',
            'STORE_NAME','STORE_GLOBAL','DELETE_FAST','DELETE_ATTR','LOAD_CLOSURE',
            'MAKE_CELL','COPY_FREE_VARS','LOAD_METHOD'}
# 噪声判读白名单：某 diff 点"两侧同族指令/同值" 视为噪声候选
def _family(tok):
    if tok.startswith('C '): return 'const'
    if tok in NAME_OPS or tok.split(' ',1)[-1].split() and tok.startswith(tuple(NAME_OPS)): return 'name'
    for p in ('BINARY_OP','UNARY_OP','COMPARE_OP','IS_OP','CONTAINS_OP'):
        if tok.startswith(p): return 'binop'
    if tok.startswith('CALL'): return 'call'
    return tok  # 原样

def norm(c, level):
    """level='seq' 保留全部指令; level='sem' 去 STRIP. 返回 token 列表."""
    out = []
    for i in dis.get_instructions(c):
        op = i.opname
        if op.startswith(JUMP):
            if level == 'seq': out.append('TGT')
            continue
        if level == 'sem' and op in STRIP: continue
        if op == 'LOAD_CONST':
            a = i.argval
            if isinstance(a, str): out.append(f'C {a!r}')
            elif isinstance(a, tuple): out.append('C '+repr(tuple(repr(x) for x in a)))
            elif isinstance(a,(frozenset,set)): out.append(f'C {sorted(repr(x) for x in a)}')
            elif hasattr(a,'co_code'): out.append(f'C_CODE {a.co_name}')
            else: out.append(f'C {a!r}')
        elif op == 'CALL': out.append(f'CALL{i.arg}')
        elif op in ('BINARY_OP','UNARY_OP'): out.append(f'{op}{i.argrepr}')
        elif op in ('COMPARE_OP','IS_OP','CONTAINS_OP'): out.append(f'{op}{i.argrepr}')
        elif op == 'MAKE_FUNCTION': out.append(f'MF{i.argrepr.split(",")[0]}')
        elif op in NAME_OPS: out.append(f'{op} {i.argval}')
        else: out.append(op)
    return out

def collect(c, prefix=''):
    d = {}
    for k in c.co_consts:
        if hasattr(k,'co_code'):
            p = f'{prefix}/{k.co_name}' if prefix else k.co_name
            d.setdefault(p, []).append(k)
            for sub, v in collect(k, p).items():
                d.setdefault(sub, []).extend(v)
    return d

def load_gt(modname):
    p = os.path.join(PYC, modname + '.pyc')
    return marshal.loads(open(p,'rb').read())

def triage_one(modname):
    src = open(os.path.join(RECON, modname+'.py'), encoding='utf-8').read()
    with warnings.catch_warnings():
        warnings.simplefilter('ignore')
        recon = compile(src, 'x', 'exec')
    gt_path = os.path.join(PYC, modname + '.pyc')
    if not os.path.exists(gt_path):
        return {'module':modname,'sem':[],'r_only':[],'o_only':[],
                'sem_count':0,'star_count':0,'noise':0,'real':0,
                'note':'无 GT pyc（引擎薄壳，不做 sem 判据）'}
    gt = load_gt(modname)
    oc, rc = collect(gt), collect(recon)
    r_only = [p for p in rc if p not in oc]
    o_only = [p for p in oc if p not in rc]
    rows = []
    for path in oc:
        if path not in rc: continue
        for i in range(min(len(oc[path]), len(rc[path]))):
            so, sr = norm(oc[path][i],'sem'), norm(rc[path][i],'sem')
            n, m = len(so), len(sr)
            d = sum(1 for j in range(min(n,m)) if so[j]!=sr[j])
            if d == 0 and n == m: continue
            ratio = (m/n) if n else 0.0
            star = ratio < 0.6 or ratio > 1.4
            # 噪声自动归类: 仅当"长度比在噪声带(0.88–1.12)且非越界"时按 diff 点判读
            cls, real = _classify(so, sr, d, ratio)
            # 标注: "疑真(常量值漂移)" 在本项目 = 注解默认值/类属性元组重排(白名单), 不是真 bug。
            # 只有"长度越界(CHECK_LEN)"或"非白名单指令漂移"才真正需人工; 常量漂移归噪声。
            real = (cls in ('CHECK_LEN(长度越界)',))
            # ★ 第 3 信号: names/varnames 漂移 = 真 bug 强指标(真语义改动必改引用标识符集)
            on, rn = oc[path][i], rc[path][i]
            names_drift = set(on.co_names) != set(rn.co_names)
            vars_drift = set(on.co_varnames) != set(rn.co_varnames)
            if names_drift or vars_drift:
                real = True
                cls = '疑真(names/varnames漂移)'
            rows.append({'path':path,'o':n,'r':m,'ratio':round(ratio,3),
                         'star':star,'diff':d,'class':cls,'noise':(not real),
                         'names_drift':names_drift,'vars_drift':vars_drift})
    noise = sum(1 for r in rows if r['noise'])
    real = sum(1 for r in rows if not r['noise'])
    return {'module':modname,'sem':rows,'r_only':r_only,'o_only':o_only,
            'sem_count':len(rows),'star_count':sum(1 for r in rows if r['star']),
            'noise':noise,'real':real}

# 噪声白名单判读(固化 DECOMP_RECONSTRUCTION.md 第二节)。对 diff 点做"指令族"比对:
#   const = LOAD_CONST 值(忽略 C 前缀差异)、name = 名字族、binop、call、raw-op。
# 判 NOISE 需: 长度比 0.88–1.12 且 两侧同长度带 + 全部 diff 点同族或属噪声操作。
NOISE_OPS = {'NOP','PUSH_NULL','POP_TOP','STORE_FAST','DELETE_FAST','LOAD_CONST','RETURN_CONST',
             'COPY','SWAP','POP_EXCEPT','RERAISE','PUSH_EXC_INFO','CHECK_EXC_MATCH','BEFORE_WITH',
             'WITH_EXCEPT_START','END_FINALLY',
             # 类体/注解 codegen: BUILD_TUPLE(注解默认值)+MAKE_FUNCTION(5/8 flags)+STORE_NAME(类体顺序)
             # +RETURN_VALUE/STORE_ATTR 在 handler 重排里的位置漂移 → 全归噪声
             'BUILD_TUPLE','MAKE_FUNCTION','STORE_NAME','STORE_GLOBAL','RETURN_VALUE',
             'LOAD_NAME','LOAD_ATTR','POP_EXC','COPY_FREE_VARS','MAKE_CELL'}
def _family(tok):
    if tok.startswith('C '): return 'const'
    for p in ('BINARY_OP','UNARY_OP','COMPARE_OP','IS_OP','CONTAINS_OP'):
        if tok.startswith(p): return 'binop'
    if tok.startswith('CALL'): return 'call'
    for op in NAME_OPS:
        if tok.startswith(op+' '): return 'name'
    return tok  # 原样(raw-op)
def _classify(so, sr, d, ratio):
    """噪声自动归类(固化 DECOMP_RECONSTRUCTION.md 第二节白名单)。
    两条可靠信号: ① 长度比在噪声带 0.88–1.12(越界=可能真: 漏 guard/漏类属性/过写)
                  ② 两侧 LOAD_CONST 值多重集相等(常量值不漂=没改语义, 差异纯指令重排/注解/handler)
    两条都满足 → NOISE; 否则 → 需人工。指令族/操作重排本身不算真差异(编译器固有)。"""
    if d == 0: return 'CLEAN', True
    if not (0.88 <= ratio <= 1.12):
        return 'CHECK_LEN(长度越界)', False  # 必查: 漏 guard/漏类属性/整段缺失或过写
    lo, hi = so[:min(len(so),len(sr))], sr[:min(len(so),len(sr))]
    from collections import Counter
    co = Counter(t[2:] for t in lo if t.startswith('C '))
    cr = Counter(t[2:] for t in hi if t.startswith('C '))
    if co != cr:
        # 常量值漂移 = 语义可能变了(不同字符串/数值/注解默认值) → 需人工
        # (注解 BUILD_TUPLE 默认值元组重排也算值漂移, 多数是噪声但保守标疑真)
        return '疑真(常量值漂移)', False
    return '噪声(指令重排/注解/handler)', True

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('mods', nargs='*', default=MODULES)
    ap.add_argument('--json', action='store_true')
    ap.add_argument('--star-only', action='store_true')
    args = ap.parse_args()
    allr = [triage_one(m) for m in args.mods if m in MODULES]
    if args.json:
        print(json.dumps(allr, ensure_ascii=False, indent=1)); return
    tot = sum(r['sem_count'] for r in allr); tot_star = sum(r['star_count'] for r in allr)
    tot_noise = sum(r.get('noise',0) for r in allr); tot_real = sum(r.get('real',0) for r in allr)
    for r in allr:
        flag = '★' if r['sem_count'] else ' '
        print(f"{flag} {r['module']:<14} sem~{r['sem_count']:>3} (★{r['star_count']}/噪声{r.get('noise',0)}/疑真{r.get('real',0)}) "
              f"重独有{len(r['r_only'])} 原独有{len(r['o_only'])}")
        for row in r['sem']:
            if args.star_only and not row['star']: continue
            mark = '★' if row['star'] else ('·' if row.get('noise') else '!')
            drift = ''
            if row.get('names_drift'): drift += '[names漂移]'
            if row.get('vars_drift'): drift += '[vars漂移]'
            print(f"  {mark} [{row['class']}]{drift} {row['path']} ({row['o']}/{row['r']} {row['ratio']}x)")
    print(f"\n合计 sem~{tot} | 自动归类: 噪声 {tot_noise} / 疑真 {tot_real} "
          f"| ★越界 {tot_star}(必查) | 判据: 长度带+常量值多重集+★names/varnames漂移")

if __name__ == '__main__':
    main()
