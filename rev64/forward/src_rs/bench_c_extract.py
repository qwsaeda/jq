#!/usr/bin/env python3
"""bench_c_extract.py — 从 consistency_desktop.c 抽取 BENCH_EXPECT 内嵌表 → .bench_c_expect.txt
★ R7-D-18（P1 双源同步断言辅助）：Makefile benchcheck 第三步调用。"""
import re, io, os
here = os.path.dirname(os.path.abspath(__file__))
s = io.open(os.path.join(here, "consistency_desktop.c"), encoding="utf-8").read()
m = re.search(r"BENCH_EXPECT\[\]\s*=\s*\n?((?:\s*\"[^\n]*\n?){2,})", s)
assert m, "BENCH_EXPECT 数组体未找到"
body = m.group(1)
parts = re.findall(r'"((?:[^"\\]|\\.)*)"', body)
out = "".join(p.replace("\\n", "\n") for p in parts)
io.open(os.path.join(here, ".bench_c_expect.txt"), "w").write(out)
print("bench_c_extract: BENCH_EXPECT 抽取 %d 段" % len(parts))
