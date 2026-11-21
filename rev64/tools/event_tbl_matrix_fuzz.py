#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""事件表 289B 矩阵结构 fuzz（零设备依赖）

定位：现有 bss_evt_tbl_check.py 只校验"非0=129 + 高nibble=0"两个聚合数，
  无法检测"某行模式被改但总数不变"的注入（如行6的 0x1 从字段0挪到字段1，
  非0总数仍是129）。本判据把 289B 当 17×17 矩阵，机检**每行模式签名**，
  并做变异检出自检（注入单行错位 → 必须 FAIL，防判据恒真）。

真值（rev64/reverse/analysis/jq_event_tbl_fill_289B.bin，§124y-168 MuMu 原版 ref dump）：
  17 行 × 17 字段，非0=129，全部 0x01（asr4 掩码位图）
  模式签名：
    行 0-5, 11-16（12 行）：字段 6-10 非0（5 个）→ 签名 "6-10"
    行 6, 8, 10（3 行）  ：15 个非0
    行 7, 9（2 行）      ：12 个非0
  12×5 + 3×15 + 2×12 = 60+45+24 = 129 ✓

判据：
  ① 289B = 17×17，非0=129，全 0x01（与既有基准一致）
  ② 逐行模式签名与真值一致（12 行 "6-10" + 3 行 15 + 2 行 12）
  ③ 负向自检：注入单行错位（行0 字段6→字段0），判据必须 FAIL（非假绿）
"""
import os, sys, hashlib

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REF = os.path.join(ROOT, 'rev64/reverse/analysis/jq_event_tbl_fill_289B.bin')

# 真值签名（从 ref dump 实测推导）
EXPECTED_SIG = {}
for r in list(range(0, 6)) + list(range(11, 17)):
    EXPECTED_SIG[r] = ('fields', tuple(range(6, 11)))   # 字段 6-10
for r in (6, 8, 10):
    EXPECTED_SIG[r] = ('count15', None)
for r in (7, 9):
    EXPECTED_SIG[r] = ('count12', None)

def row_signature(row):
    nz = tuple(i for i, b in enumerate(row) if b)
    return nz

def check_table(data):
    """返回 (ok, 失败原因列表)"""
    fails = []
    if len(data) != 289:
        return False, ['大小 %d ≠ 289' % len(data)]
    if any(b not in (0, 1) for b in data):
        fails.append('存在非 0/1 字节')
    nz = sum(1 for b in data if b)
    if nz != 129:
        fails.append('非0总数 %d ≠ 129' % nz)
    for r in range(17):
        row = data[r*17:(r+1)*17]
        sig = row_signature(row)
        kind, fields = EXPECTED_SIG[r]
        if kind == 'fields':
            if sig != fields:
                fails.append('行%d 字段模式 %s ≠ 期望 %s' % (r, sig, fields))
        elif kind == 'count15' and len(sig) != 15:
            fails.append('行%d 非0数 %d ≠ 15' % (r, len(sig)))
        elif kind == 'count12' and len(sig) != 12:
            fails.append('行%d 非0数 %d ≠ 12' % (r, len(sig)))
    return len(fails) == 0, fails

def main():
    if not os.path.isfile(REF):
        print('PENDING（ref dump 缺失: %s）' % REF); return 3
    data = open(REF, 'rb').read()
    print('== 事件表 289B 矩阵结构 fuzz ==')
    print('   ref: %s' % REF)
    print('   md5: %s' % hashlib.md5(data).hexdigest())

    ok, fails = check_table(data)
    print('   ①+② 正向判据: %s（非0=%d）' % ('PASS' if ok else 'FAIL', sum(1 for b in data if b)))
    for f in fails[:5]: print('     FAIL:', f)

    # ③ 负向自检：注入单行错位（行0 字段6→字段0），必须 FAIL
    mutant = bytearray(data)
    mutant[6] = 0; mutant[0] = 1   # 行0 字段6 移到 字段0，非0总数不变
    mok, mfails = check_table(bytes(mutant))
    print('   ③ 负向自检（行0 字段6→0）: %s' % ('PASS（成功检出注入，非假绿）' if not mok else 'FAIL（注入未检出=判据恒真）'))

    if not ok:
        print('判据: FAIL（正向判据失败）'); return 2
    if mok:
        print('判据: FAIL（负向自检未检出注入=恒真判据）'); return 2
    print('判据: PASS（289B 矩阵逐行模式签名一致 + 单行错位注入可检出）')
    return 0

if __name__ == '__main__':
    sys.exit(main())
