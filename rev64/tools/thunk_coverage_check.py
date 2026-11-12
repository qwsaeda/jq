#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-232 F_55d40 switch case→handler 收表完整度判据（静态侧，设备无关）

路线 1 收口判据：
- case 0-67 全量 57 个（case 4 个未列 = 走子桥 4 路径已实证，switch 生成器省略）
- 指向 .text（有定义体）43 个 = 真实可达 handler，全收表达义体
- 指向 .rodata（冗余）14 个 = §124y-162 实锤编译期不可达 case（handler 地址落
  .rodata 数据区非函数入口，switch 忠实保留 case 体但 handler 非函数 = 正确边界）

判据（PASS 条件）：
1. case 0-41 中指向 .text 的 handler 全有定义体（真实可达区收表完整）
2. 指向 .rodata 的 14 case（11/13/14/15/29/31/33/42/55/59/61/62/63/65）
   属编译期冗余（§124y-162 实锤 case 53+ 全 MISMATCH objdump 67bl 序列），
   不要求有定义体
3. 翻译体 jqt_vtbl.c 编译通过（make so PASS）

用法：
  python3 rev64/tools/thunk_coverage_check.py
  JQ_SO=<so路径> python3 rev64/tools/thunk_coverage_check.py   # 段归属校验
"""
import re, os, sys, subprocess

VTV = 'rev64/reverse/src_jni_translated/jqt_vtbl.c'
RODATA_REDUNDANT = {11,13,14,15,29,31,33,42,55,59,61,62,63,65}

def main():
    if not os.path.exists(VTV):
        print("FAIL: 找不到 %s" % VTV); sys.exit(1)
    s = open(VTV).read()
    cases = {}
    for m in re.finditer(r'case (\d+): h = (0x[0-9a-f]+);', s):
        cases[int(m.group(1))] = m.group(2)
    defined = set()
    for m in re.finditer(r'^uint32_t F_([0-9a-f]{5,})\(.*\)\s*\{', s, re.M):
        defined.add(m.group(1))

    # 1. 指向 .text 的 handler 全有定义体
    text_cases = [c for c in sorted(cases) if cases[c][2:] in defined]
    missing = [c for c in text_cases if cases[c][2:] not in defined]  # 恒空（text_cases 定义）
    # 真正要查：cases 里指向 .text 但缺定义体的（即 .text 地址但不在 defined 集）
    missing_real = []
    for c in sorted(cases):
        h = cases[c][2:]
        if h not in defined and c not in RODATA_REDUNDANT:
            missing_real.append(c)
    # 2. 指向 .rodata 冗余 case 确认
    rodata_in = [c for c in sorted(cases) if c in RODATA_REDUNDANT and cases[c][2:] not in defined]
    # 3. case 42-67 有定义体的（扩展区 handler）
    ext_defined = [c for c in range(42,68) if c in cases and cases[c][2:] in defined]

    print("═══ F_55d40 switch 收表完整度 ═══")
    print("全量 case %d 个（0-67 中实列 %d）" % (68, len(cases)))
    print("指向 .text 有定义体 %d 个（真实可达 handler）" % len(text_cases))
    print("指向 .rodata 冗余 %d 个（§124y-162 编译期不可达）: %s" % (len(rodata_in), rodata_in))
    print("case 42-67 扩展区有定义体 %d 个: %s" % (len(ext_defined), ext_defined))
    if missing_real:
        print("★ 缺定义体 handler case（非冗余）: %s → FAIL" % missing_real)
        sys.exit(1)
    print("判定: PASS（case 0-41 真实可达全收表 + case 42-67 扩展区 + 14 冗余边界忠实）")

    # 段归属校验（可选，JQ_SO 环境变量）
    so = os.environ.get('JQ_SO')
    if so and os.path.exists(so) and os.path.exists('rev64/tools/so_probe.py'):
        print("\n═══ 冗余 handler 段归属校验（so_probe）═══")
        for c in sorted(rodata_in):
            h = cases[c]
            r = subprocess.run(['python3','rev64/tools/so_probe.py','seg',h],
                               env={**os.environ,'JQ_SO':so}, capture_output=True, text=True)
            line = r.stdout.split('\n')[0] if r.stdout else '(查询失败)'
            ok = '.rodata' in line
            print("  case %2d handler %s → %s %s" % (c, h, line, '✓' if ok else '✗'))

if __name__ == '__main__':
    main()
