#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-199 32 位翻译体 so 事件名字符串池对照判据（静态侧，不依赖设备）

固化"翻译体 so 内嵌事件名字符串池 ⊇ 原版 .rodata 76 事件名"口径（防字符串漂移）：
- 翻译体 so rev64/dist/libjunqi_arm32_aligned.so 经 jq_rodata_plain.h g_jq_strings
  内嵌 76 事件名（73 Junqi + 3 Misc）
- 原版 so .rodata 76 事件名（§124y-183 族谱，含 %d 模板 token）
- 判据① 翻译体 so strings 提取 Junqi.* 事件名 ⊇ 原版 73 Junqi（0 缺口）
- 判据② 翻译体 so 内嵌事件名总数 ≥ 73（g_jq_strings 76 条口径）

用法：python3 rev64/tools/translated_strpool_check.py
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"
TRANS_SO = os.path.join(ROOT, "rev64/dist/libjunqi_arm32_aligned.so")

def extract_event_names(blob):
    return set(m.group(0).decode() for m in re.finditer(rb'(?:Junqi|Misc)\.[A-Za-z0-9%]+', blob))

def main():
    for p in (ORIG_SO, TRANS_SO):
        if not os.path.exists(p):
            print("✗ %s 不存在" % p); return 2
    orig = extract_event_names(open(ORIG_SO, 'rb').read())
    # 翻译体 so 用 strings 提取（二进制内嵌字符串池）
    trans_raw = open(TRANS_SO, 'rb').read()
    trans = extract_event_names(trans_raw)
    orig_junqi = {n for n in orig if n.startswith('Junqi.')}
    miss = sorted(orig_junqi - trans)
    print("═══ §124y-199 32 位翻译体 so 事件名字符串池对照判据 ═══")
    print("  原版 .rodata 事件名: %d（Junqi %d + Misc %d，含 %d 模板 token）" % (
        len(orig), len(orig_junqi), len(orig)-len(orig_junqi),
        sum(1 for n in orig if '%d' in n)))
    print("  翻译体 so 内嵌事件名: %d（g_jq_strings 76 条口径）" % len(trans))
    print("  Junqi 主链缺口: %d %s" % (len(miss), miss[:6] if miss else "（0 缺口）"))
    ok = (not miss) and (len(trans) >= 73)
    print("  判据: %s" % ("PASS（翻译体 so 内嵌事件名 ⊇ 原版 73 Junqi 主链 0 缺口）" if ok else "FAIL（缺口 %d / 总数 %d）" % (len(miss), len(trans))))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
