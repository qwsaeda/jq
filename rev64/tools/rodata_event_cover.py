#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-184 原版 .rodata 事件名 → 翻译体 g_jq_strings 覆盖度 0 缺口判据（静态，不依赖设备）

用户"光改光对比，经验不总结/不脚本化"指正的落实：把"事件名 76 覆盖度"从每轮
手敲 python 固化为可复跑判据。

判据：原版 so .rodata 明文 76 事件名（73 Junqi.* + 3 Misc.*）逐项在翻译体
jq_rodata_plain.h 的 g_jq_strings 数组里命中 → 0 缺口 PASS。

用法：python3 rev64/tools/rodata_event_cover.py
     （--fix 模式：把缺口项按原版 .rodata 文件偏移=VA 实锤值补进 g_jq_strings）
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"
TR_H = os.path.join(ROOT, "rev64/reverse/src_jni_translated/jq_rodata_plain.h")

def event_names(so):
    data = open(so, 'rb').read()
    # 完整 token：含数字/%d 后缀（Junqi.GameType%d / Misc.Graph%d 为模板串，%d 属 token）
    toks = set(m.group(0).decode() for m in re.finditer(rb'(?:Junqi|Misc)\.[A-Za-z0-9%]+', data))
    junqi = {x for x in toks if x.startswith('Junqi.')}
    misc = {x for x in toks if x.startswith('Misc.')}
    return sorted(toks), junqi, misc

def va_of(so, name):
    """原版 .rodata 文件偏移 = VA（delta=0，"Cancel" 实锤）。模板串带 %d 原样。"""
    data = open(so, 'rb').read()
    i = data.find(name.encode())
    return i if i >= 0 else None

def covered(tr_h, names):
    src = open(tr_h).read()
    m = re.search(r'g_jq_strings\[(\d+)\]', src)
    n = int(m.group(1)) if m else 0
    have = set(re.findall(r'"((?:Junqi|Misc)\.[^"]+)"', src))
    return n, names, [x for x in names if x not in have]

def main():
    fix = '--fix' in sys.argv
    alln, junqi, misc = event_names(ORIG_SO)
    n, names, missing = covered(TR_H, alln)
    print("═══ §124y-184 事件名覆盖度判据 ═══")
    print("  原版 .rodata 事件名: %d（Junqi %d + Misc %d）" % (len(alln), len(junqi), len(misc)))
    print("  翻译体 g_jq_strings 数组: %d 条" % n)
    print("  覆盖: %d / %d" % (len(alln)-len(missing), len(alln)))
    if not missing:
        print("  判据: 0 缺口 PASS")
        return 0
    print("  缺口 %d:" % len(missing))
    for x in missing:
        va = va_of(ORIG_SO, x)
        print("    - %s (VA≈0x%x)" % (x, va if va is not None else 0))
    if not fix:
        print("  判据: 存在缺口 FAIL（--fix 补全）")
        return 2
    # --fix：补全
    src = open(TR_H).read()
    anchor = re.search(r'^static const struct jq_str g_jq_strings\[\d+\] = \{$', src, re.M)
    # 数组大小更新
    newn = n + len(missing)
    src = re.sub(r'g_jq_strings\[\d+\]', 'g_jq_strings[%d]' % newn, src, 1)
    # 在 "};\"（数组尾）前插 3 条
    tail = src.index('\n};\n', src.index('g_jq_strings'))
    ins = ""
    for x in missing:
        va = va_of(ORIG_SO, x)
        if va is None:
            print("    ! %s 未找到文件偏移，跳过" % x); continue
        ins += '  {0x%x, "%s"},\n' % (va, x)
    src = src[:tail] + ins + src[tail:]
    open(TR_H, 'w').write(src)
    print("  已补 %d 条（g_jq_strings → %d），重跑本脚本应 0 缺口" % (len(missing), newn))
    return 0

if __name__ == '__main__':
    sys.exit(main())
