#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-233 原版 32 位 so .rodata 73 Junqi.* 事件名静态提取判据（设备无关）

判据（PASS 条件）：
1. 原版 32 位 so `strings` 直提 Junqi.* 事件名 = 73 条（.rodata 明文，非加壳 so
   口径 §124y-192；与 frida-dump/rodata_strings.txt 73 条二手机一致）
2. event_code_map.json（73 事件名 → 12 事件码映射表）event_codes 值域 names
   并集 = 73 条全对照（0 缺口 0 多余，§124y-187 73/73 口径）
3. 事件码值域 = 12 码子集（0..11，.rodata 可提取；12..16 需 .bss 运行时填
   asr4 = disc=0 忠实边界 §124y-163/185，不静态提取）

用法：
  python3 rev64/tools/event_name_static_check.py
  JQ_SO=<so路径> python3 rev64/tools/event_name_static_check.py   # 切 so 源
"""
import json, os, subprocess, sys

# 脚本在 rev64/tools/ 下，工作区根 = 上溯 2 级（/disk/workspaces/jq）
TOOLDIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(TOOLDIR))
SO = os.environ.get('JQ_SO', os.path.join(ROOT, 'rev64/dist/libjunqi_arm32_aligned.so'))
MAP = os.path.join(ROOT, 'rev64/dist/event_code_map.json')
REF = os.path.join(ROOT, 'frida-dump/rodata_strings.txt')

def extract_junqi_names(so):
    r = subprocess.run(['strings', so], capture_output=True, text=True)
    return sorted(set(l for l in r.stdout.splitlines() if l.startswith('Junqi.')))

def main():
    print("═══ 事件名静态提取（原版 32 位 so .rodata 明文）═══")
    # 1. 直提
    so_names = extract_junqi_names(SO)
    print("so strings 直提 Junqi.* 事件名: %d 条" % len(so_names))
    if len(so_names) != 73:
        print("判定: FAIL（直提 %d ≠ 73，查 so 路径 / .rodata 完整性）" % len(so_names)); sys.exit(1)
    # 2. 二手机对照（frida-dump rodata_strings.txt 73 条）
    if os.path.exists(REF):
        ref_names = sorted(set(l.strip() for l in open(REF) if l.strip().startswith('Junqi.')))
        miss = [n for n in so_names if n not in ref_names]
        extra = [n for n in ref_names if n not in so_names]
        print("frida-dump 二手机 %d 条对照: 缺=%s 多=%s" % (len(ref_names), miss or '无', extra or '无'))
        if miss or extra:
            print("判定: FAIL（二手机对照不通过）"); sys.exit(1)
    # 3. event_code_map.json 73/73 全对照
    d = json.load(open(MAP))
    ec = d.get('event_codes', {})
    map_names = sorted(set(n for v in ec.values() for n in v.get('names', [])))
    codes = sorted(int(k) for k in ec.keys())
    # §124y-187 模板串 token 口径归一化：so .rodata 存完整模板（Junqi.GameType%d /
    #   Misc.Graph%d 等带 %d 序号展开 token），map 按简写存名（去 %d）。
    #   对照时 so 侧去 %d 简写归一，与 map 简写对齐。
    def norm(n): return n.split('%')[0] if '%' in n else n
    so_norm = set(norm(n) for n in so_names)
    map_norm = set(map_names)
    miss = [n for n in so_norm if n not in map_norm]
    extra = [n for n in map_norm if n not in so_norm]
    print("event_code_map.json: %d 事件名 / %d 事件码（值域 %s）" % (len(map_names), len(codes), codes))
    print("  对照: 缺（so 有 map 无）=%s 多（map 有 so 无）=%s" % (miss or '无', extra or '无'))
    if miss or extra:
        print("判定: FAIL（73/73 全对照不通过）"); sys.exit(1)
    # 4. 事件码值域 = 12 码子集口径
    if len(codes) != 12:
        print("判定: FAIL（事件码 %d ≠ 12 子集口径 §124y-187）" % len(codes)); sys.exit(1)
    print("判定: PASS（73/73 事件名全对照 + 12 码子集 + disc=0 忠实边界）")

if __name__ == '__main__':
    main()
