#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
event_names_alignment_check.py — 事件名逐名对齐门禁（Round 1 §103 手工 diff → 脚本化）

目的：把 §103「正向 73 事件名 vs 原版 .rodata 73 名逐名 diff 0 缺 0 多」从一次性
手工核对，固化为**可重复门禁**（防正向事件名漂移 = 事件契约失配）。

基准（只读，不改逆向产物）：
  · 原版 32 位 .rodata 明文字符串池：rev64/reverse/src_jni_translated/jq_rodata_plain.h
  · 正向 64 位事件名表：rev64/forward/src_rs/jq_event_names_73.c（+ .h 计数常量）

判据：
  ① 原版 Junqi.* 唯一名集合 == 正向事件名表集合（差集双向为空）；
  ② 正向名表条数 == JQ_EVENT_NAME_COUNT 常量（防表与头文件脱节）；
  ③ 原版 Junqi.* 唯一名数 ≥ 70（防基准文件被截断/清空导致"空集也通过"——判据恒真防护）。

退出码：0 = PASS；1 = FAIL（任一判据不满足）。
用法：python3 rev64/tools/event_names_alignment_check.py [--json]
"""
import argparse
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_H = os.path.join(ROOT, "rev64/reverse/src_jni_translated/jq_rodata_plain.h")
FWD_C = os.path.join(ROOT, "rev64/forward/src_rs/jq_event_names_73.c")
FWD_H = os.path.join(ROOT, "rev64/forward/src_rs/jq_event_names_73.h")

fails = 0


def check(cond, msg):
    global fails
    print(f"  {'✓' if cond else '✗ FAIL'}: {msg}")
    if not cond:
        fails += 1
    return cond


def orig_names():
    txt = open(ORIG_H, encoding="utf-8", errors="ignore").read()
    # ★ 口径陷阱（Round 3 实锤）：原版含**格式化事件名模板** "Junqi.GameType%d"
    #   ——简单正则 Junqi\.[A-Za-z0-9_]+ 会漏掉 % 形态 → 计数 72 vs 73 误判"漏名"。
    #   本门禁正则必须允许 % 形态（与 jq_event_names_73.c 表体解析口径一致）。
    return set(re.findall(r"Junqi\.[A-Za-z0-9_]+%?d?", txt))


def fwd_names():
    txt = open(FWD_C, encoding="utf-8", errors="ignore").read()
    # 取字符串常量（表体内），排除注释里的示例
    body = txt.split("JQ_EVENT_NAME_COUNT] = {", 1)[-1]
    body = body.split("};", 1)[0]
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    return set(re.findall(r'"(Junqi\.[A-Za-z0-9_]+%?d?)"', body))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--json", action="store_true")
    a = ap.parse_args()

    o = orig_names()
    f = fwd_names()
    print("=== 事件名逐名对齐门禁（正向 73 名 vs 原版 .rodata 名）===")
    print(f"  原版唯一名 {len(o)} / 正向唯一名 {len(f)}")

    missing = sorted(o - f)      # 原版有、正向无（正向漏名 = 契约缺）
    extra = sorted(f - o)        # 正向有、原版无（正向多名 = 漂移）
    check(not missing, f"正向无漏名（原版有正向无：{missing or '无'}）")
    check(not extra, f"正向无多名（正向有原版无：{extra or '无'}）")

    hdr = open(FWD_H, encoding="utf-8", errors="ignore").read()
    m = re.search(r"#define\s+JQ_EVENT_NAME_COUNT\s+(\d+)", hdr)
    cnt = int(m.group(1)) if m else -1
    check(cnt > 0 and cnt == len(f), f"名表条数 == JQ_EVENT_NAME_COUNT（{cnt} vs {len(f)}）")
    # 判据恒真防护：基准集合被清空/截断时不得判 PASS
    check(len(o) >= 70, f"原版基准名数 ≥ 70（实测 {len(o)}）——防基准文件清空导致空集通过")

    if a.json:
        print(json.dumps({"orig": len(o), "fwd": len(f), "missing": missing,
                          "extra": extra, "fails": fails}, ensure_ascii=False))
    print(f"\n{'=== PASS（事件名逐名对齐）===' if fails == 0 else f'=== {fails} 项 FAIL ==='}")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
