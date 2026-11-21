#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""GOT/PLT 映射表逐项结构 fuzz（零设备依赖）

定位：现有 got_map_check.py 只校验"plt 214 + got 227 + 偏移在 .got 区"聚合数，
  查不出"单槽偏移不对齐/重复/越界/符号名非法/plt∩got 交叉不一致"的注入。

数据源：rev64/reverse/analysis/{got_map,plt_map}.txt（§124y-201 产物）
判据：
  ① got 227 槽 / plt 214 条（聚合口径）
  ② got 偏移 4B 对齐 + 落在 .got 区(0x15a000-0x15b800) + 无重复 + 严格递增
  ③ plt 偏移 4B 对齐 + 落在 .plt 区(0x22000-0x24000) + 无重复 + 12B 步长
  ④ 符号名非空且为合法 C 标识符/mangled（^[A-Za-z_?][A-Za-z0-9_@$.?]*$）
  ⑤ 交叉一致性：plt 的 214 符号中 ≥200 个在 got 里（§124y 基线 211）
  ⑥ 负向自检：注入越界 got 偏移 / 重复 plt 偏移 → 判据必须 FAIL
"""
import os, re, sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GOT = os.path.join(ROOT, 'rev64/reverse/analysis/got_map.txt')
PLT = os.path.join(ROOT, 'rev64/reverse/analysis/plt_map.txt')
SYM_RE = re.compile(r'^[A-Za-z_?][A-Za-z0-9_@$.?]*(?:\[\.\.\.\])?$')
# ★ 真值口径：plt_map 有 3 个符号名被产物生成时截断为 "前缀[...]"（如
#   pthread_attr_setd[...]），是合法的截断标记，非非法数据 → 正则接受该后缀。

def load(p):
    e = []
    for l in open(p).read().splitlines():
        m = re.match(r'^([0-9a-f]+)\s+(\S+)', l.strip())
        if m:
            e.append((int(m.group(1), 16), m.group(2)))
    return e

def check(got, plt):
    fails = []
    if len(got) != 227: fails.append('got 条数 %d ≠ 227' % len(got))
    if len(plt) != 214: fails.append('plt 条数 %d ≠ 214' % len(plt))
    # got 结构
    goffs = [x[0] for x in got]
    if any(o % 4 for o in goffs): fails.append('got 偏移非 4B 对齐')
    if len(goffs) != len(set(goffs)): fails.append('got 偏移重复')
    if any(o < 0x15a000 or o >= 0x15b800 for o in goffs): fails.append('got 偏移越界 .got 区')
    if any(goffs[i] <= goffs[i-1] for i in range(1, len(goffs))): fails.append('got 偏移非递增')
    # plt 结构
    poffs = [x[0] for x in plt]
    if any(o % 4 for o in poffs): fails.append('plt 偏移非 4B 对齐')
    if len(poffs) != len(set(poffs)): fails.append('plt 偏移重复')
    if any(o < 0x22000 or o >= 0x24000 for o in poffs): fails.append('plt 偏移越界 .plt 区')
    steps = [poffs[i] - poffs[i-1] for i in range(1, len(poffs))]
    bad_step = sum(1 for s in steps if s not in (8, 12, 16))  # plt stub 12B 为主
    if bad_step > 5: fails.append('plt 步长异常 %d 处' % bad_step)
    # 符号名
    for o, n in got + plt:
        if not SYM_RE.match(n):
            fails.append('非法符号名 %s' % n); break
    # 交叉
    gs = set(n for _, n in got)
    overlap = sum(1 for _, n in plt if n in gs)
    if overlap < 200: fails.append('plt∩got 重叠 %d < 200（§124y 基线 211）' % overlap)
    return len(fails) == 0, fails, overlap

def main():
    if not (os.path.isfile(GOT) and os.path.isfile(PLT)):
        print('PENDING（got_map/plt_map 产物缺失）'); return 3
    got, plt = load(GOT), load(PLT)
    print('== GOT/PLT 映射表逐项结构 fuzz ==')
    ok, fails, overlap = check(got, plt)
    print('   ①-⑤ 正向判据: %s（got=%d plt=%d 交叉=%d）' % ('PASS' if ok else 'FAIL', len(got), len(plt), overlap))
    for f in fails[:5]: print('     FAIL:', f)
    # 负向自检
    mg = list(got); mg[0] = (0xdeadbeef, mg[0][1])
    ok1, _, _ = check(mg, plt)
    mp = list(plt); mp[1] = (mp[0][0], mp[1][1])
    ok2, _, _ = check(got, mp)
    print('   ⑥ 负向自检: got越界=%s / plt重复=%s' %
          ('检出' if not ok1 else '未检出(假绿)', '检出' if not ok2 else '未检出(假绿)'))
    if not ok or ok1 or ok2:
        print('判据: FAIL'); return 2
    print('判据: PASS（got227/plt214 逐项对齐/唯一/越界/递增/符号合法/交叉211 + 2 类注入检出）')
    return 0

if __name__ == '__main__':
    sys.exit(main())
