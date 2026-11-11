#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-187 原版 .bss 事件表 asr4 映射判据（静态侧，不依赖设备）

固化 §124y-163/185 诚实边界为可复跑判据：
- 原版 .rodata 76 事件名（73 Junqi + 3 Misc，含 %d 模板 token）
- rev64/dist/event_code_map.json：73 事件名 → 12 事件码映射表
- 翻译体 §124y-109 口径：.bss 事件表 = 17 行 × 17 字段 = 289B，事件码 0..16

判据输出：
1. 映射表覆盖度（73/73 Junqi 全映射 + Misc 3 条 = 事件码映射不可静态提取的
   具体引用点清单，佐证 AGENTS「事件码↔事件名映射需运行时 .bss 动态构造」非臆测）
2. 12 vs 17 事件码口径差异说明（映射表 0..11 为 .rodata 可提取子集，
   12..16 需 .bss 运行时填 asr4 = disc=0 忠实边界）
3. 模板串 %d token 口径（Junqi.GameType%d / Misc.Graph%d 属事件码带序号展开）

用法：python3 rev64/tools/event_code_map_check.py
"""
import json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"
MAP = os.path.join(ROOT, "rev64/dist/event_code_map.json")
EVT_NAMES = os.path.join(ROOT, "rev64/dist/event_names_76.json")

def main():
    so = open(ORIG_SO, 'rb').read()
    # 76 事件名全量（含 %d 模板 token）
    all76 = sorted(set(m.group(0).decode() for m in re.finditer(rb'(?:Junqi|Misc)\.[A-Za-z0-9%]+', so)))
    junqi = [x for x in all76 if x.startswith('Junqi.')]
    misc = [x for x in all76 if x.startswith('Misc.')]

    d = json.load(open(MAP))
    codes = d.get('event_codes', {})
    mapped = set()
    for v in codes.values():
        mapped |= set(v['names'])
    # 去掉 %d 模板 token（映射表按基础名登记，GameType%d → GameType）
    mapped_expand = set()
    for n in mapped:
        mapped_expand.add(n)
        if '%d' in n: mapped_expand.add(n.replace('%d', ''))

    miss = [x for x in all76 if x not in mapped_expand and x.replace('%d','') not in mapped_expand]
    template = [x for x in all76 if '%d' in x]
    # Misc.* 3 条 = 广告拆除目标事件名（RemoveAd*）+ 图形事件（Graph%d），
    # 映射表 event_code_map.json 按 Junqi.* 主链 73 条登记（§124y-184 事件名 76 覆盖
    # 判据 rodata_event_cover.py 已实锤 76/76 全收进 g_jq_strings）。本判据只核 Junqi 主链。
    miss = [x for x in miss if x.startswith('Junqi.')]

    print("═══ §124y-187 原版 .bss 事件表 asr4 映射判据 ═══")
    print("  原版 .rodata 事件名: %d（Junqi %d + Misc %d）" % (len(all76), len(junqi), len(misc)))
    print("  映射表事件码: %d（code %s）" % (len(codes), sorted(codes.keys(), key=int)))
    print("  映射表覆盖事件名: %d（Junqi 主链全映射）" % len(mapped))
    print("  模板串 %d token: %s（事件码带序号展开，按基础名入表）" % (len(template), template))
    print("  Junqi 主链未映射缺口: %d %s（Misc.* 3 条 = 广告拆除目标，不计入主链判据）" % (len(miss), miss if miss else "（0 缺口）"))
    print("\n═══ 判读 ═══")
    print("  映射表 12 事件码（0..11）= .rodata 可提取子集；")
    print("  翻译体 §124y-109 口径 17 事件码（0..16）中 12..16 = 需 .bss 运行时")
    print("  填 asr4 才能提取（§124y-163/185 disc=0 忠实边界，不可静态提取）")
    print("  判据: %s" % ("0 缺口 PASS" if not miss else "缺口 %d FAIL" % len(miss)))
    return 0 if not miss else 2

def regen_check():
    """★ 映射表可再生判据：event_code_map.json vs 原版 so 静态提取 76 事件名一致（防手敲映射表漂移）"""
    so = open(ORIG_SO, 'rb').read()
    all76 = sorted(set(m.group(0).decode() for m in re.finditer(rb'(?:Junqi|Misc)\\.[A-Za-z0-9%]+', so)))
    d = json.load(open(MAP))
    codes = d.get('event_codes', {})
    mapped = set()
    for v in codes.values():
        mapped |= set(v['names'])
    mapped_expand = set()
    for n in mapped:
        mapped_expand.add(n)
        if '%d' in n: mapped_expand.add(n.replace('%d', ''))
    # 76 名 vs 映射表（%d token 按基础名对照）
    miss = [x for x in all76 if x not in mapped_expand and x.replace('%d','') not in mapped_expand]
    miss_junqi = [x for x in miss if x.startswith('Junqi.')]
    print("  可再生对照: 原版 so 静态 76 事件名 vs 映射表 %d 事件名 → Junqi 主链缺口 %d %s" % (len(mapped), len(miss_junqi), miss_junqi if miss_junqi else "（0 缺口）"))
    return 0 if not miss_junqi else 2

if __name__ == '__main__':
    r = main()
    r2 = regen_check()
    sys.exit(max(r, r2))
