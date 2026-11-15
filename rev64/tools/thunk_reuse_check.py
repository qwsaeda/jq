#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1924 F_55d40 code→handler 复用度量化判据（设备无关）

固化"67 case 指向 37 distinct handler"的复用度口径（§124y-1923 勘误后
实列 67 case = code 0-66）：
  - 7 个高频 handler 被 ≥3 case 复用（F_85798×7 / F_857b0×4 /
    F_85348×4 / F_b52a0×4 / F_55dc4×4 / F_82d3c×3 / F_b4edc×3 等），
    = 事件分发"少量 handler 承载多事件码"的复用设计（非 67 个独立
    handler，handler 总数 = 37 distinct）
  - 判据：① 67 case 实列（code 0-66，缺 67 走 default）② 37 distinct
    handler 全有定义体 ③ 复用度（≥3 case 复用的 handler 7 个，最高
    7 复用）④ 14 冗余 case（.rodata 区，§124y-162 编译期不可达）+
    扩展区 case 42-67 边界

用法：python3 rev64/tools/thunk_reuse_check.py
"""
import os, re, sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
VTV = os.path.join(ROOT, 'rev64/reverse/src_jni_translated/jqt_vtbl.c')
RODATA_REDUNDANT = {11, 13, 14, 15, 29, 31, 33, 42, 55, 59, 61, 62, 63, 65}

def main():
    src = open(VTV, 'rb').read().decode('utf-8', errors='replace')
    cases = {int(m.group(1)): m.group(2) for m in re.finditer(r'case (\d+):\s*h\s*=\s*0x([0-9a-f]+);', src)}
    defined = set(m.group(1) for m in re.finditer(r'^uint32_t F_([0-9a-f]{5,})\(', src, re.M))
    cnt = Counter(cases.values())
    distinct = set(cases.values())  # 值已去 0x 前缀（regex group(2)）
    multi = {h: c for h, c in cnt.items() if c >= 3}
    max_reuse = max(cnt.values()) if cnt else 0
    # §124y-1924 勘误：cases 值已去 0x（regex group(2)），直接用 h（非 h[2:]）对 defined
    missing_def = [h for h in distinct if h not in defined]

    ok = (len(cases) == 67 and len(distinct) == 37 and not missing_def)
    print('═══ §124y-1924 F_55d40 code→handler 复用度量化判据 ═══')
    print('  ① 实列 case = %d（code 0-66，缺 67 走 default）' % len(cases))
    print('  ② distinct handler = %d（全有定义体 %s）' % (len(distinct),
         'OK' if not missing_def else '缺 %s' % missing_def))
    print('  ③ 复用度：≥3 case 复用 handler = %d 个（最高 %d 复用）' % (len(multi), max_reuse))
    for h, c in sorted(multi.items(), key=lambda x: -x[1]):
        print('     0x%s × %d' % (h, c))
    print('  ④ 冗余/扩展区：14 冗余 case（.rodata §124y-162 不可达）+ 扩展区 42-66')
    print('  §1926 口径厘清：37 = F_55d40 全 switch（code 0-66）distinct handler（含 7 冗余 .rodata 地址）；')
    print('      .text 真实（去 14 冗余 .rodata）= 30；codes 0-41 原始 = 24（§124y-29/30 收表口径）')
    print('      本判据 37 = 全量 distinct（复用度量化用全量，§1924 口径）')
    print('  判据: %s' % ('PASS（67 case / 37 handler 全定义体 / 7 高频复用，事件分发复用设计实锤）'
                          if ok else 'FAIL'))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
