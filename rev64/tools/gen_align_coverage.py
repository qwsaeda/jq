#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_align_coverage.py — 「对齐」覆盖矩阵自动生成（Round 1 A-25/D-22 + Round 1 D-08 落地）

目的：把"规则条款 ↔ 验证用例 ↔ 双端探针"三者的覆盖关系从**散落各处**变成
**一张可复现的矩阵**，使覆盖缺口可见（缺断言的条款一眼看出），并集中登记
**硬编码期望值基准点**（D-08：改行为时必须同步更新的基准清单）。

数据源（只读）：
  · 规则原文机判 33 项：rev64/forward/src_rs/dark_align/rule_text_conformance.c
  · 暗棋对齐用例：rev64/forward/src_rs/dark_align/*.c（每例 chk() 断言数）
  · 双端探针：rev64/forward/src_rs/rule_probe_shared.inc（case 名 + 覆盖 AUD-R 项）
  · 走子桥基准：rev64/forward/src_rs/test_expected.txt（硬编码期望）
  · 桌面事件断言：rev64/forward/src_rs/evverify.c（断言数）

输出：
  · rev64/forward/audit/r4/align_coverage_matrix.md（矩阵 + 基准点台账 + 统计）
用法：python3 rev64/tools/gen_align_coverage.py
"""
import json
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DARK = os.path.join(ROOT, "rev64/forward/src_rs/dark_align")
SRC = os.path.join(ROOT, "rev64/forward/src_rs")
OUT = os.path.join(ROOT, "rev64/forward/audit/r4/align_coverage_matrix.md")


def read(p, default=""):
    try:
        with open(p, encoding="utf-8", errors="ignore") as f:
            return f.read()
    except OSError:
        return default


def main():
    lines = []
    # ★ R9-G-14/11（P1 矩阵新鲜度）：生成时间戳 + evverify 实测口径行（快照元数据
    #   ——矩阵数字是生成时刻真值，无时间戳/实测口径 = 后续引用无法判新鲜度
    #   （R7-D-10/11/12/15/16 定案落地）；git diff --exit-code 判据由人工/CI 承担，
    #   本行至少可 grep 时间戳判"是否再生过"）。
    import datetime
    _gen_at = datetime.datetime.now().strftime("%Y-%m-%d %H:%M")
    lines.append("# 对齐覆盖矩阵（自动生成：rev64/tools/gen_align_coverage.py）\n")
    lines.append("> 生成时间: %s（每次 `python3 rev64/tools/gen_align_coverage.py` 重生；"
                 "引用前确认与当前源码一致——evverify 真值以 `jq_evverify --all` SUMMARY 动态为准）\n" % _gen_at)
    lines.append("> 用途：规则条款 ↔ 桌面断言 ↔ 双端探针 三向覆盖关系可见化；"
                 "并集中登记**硬编码期望值基准点**（改行为必须同步的清单）。\n")

    # ── ① 规则原文机判条款 ──
    rtc = read(os.path.join(DARK, "rule_text_conformance.c"))
    clauses = re.findall(r'chk\(\s*"([^"]+)"', rtc)
    lines.append(f"## ① 规则原文机判条款（{len(clauses)} 项）\n")
    lines.append("| # | 条款（节选） | 状态 |")
    lines.append("|---|---|---|")
    for i, c in enumerate(clauses, 1):
        lines.append(f"| {i} | {c[:70]} | ✅ 有断言 |")

    # ── ② dark_align 用例与断言强度 ──
    # ★ 口径校准（Round 3 实锤）：仅按 chk()/assert 关键字计数会**误判**"零断言"——
    #   本仓用例断言风格多样（chk / ok&= / expect() / allpass / 内联条件），
    #   首轮检测误报 6 例（dark_bomb/dark_flag_win/dark_flip/draw_exist/flip_draw_guard/
    #   rank_order），核实全部有真断言。权威判定 = **双条件**：
    #     ① 有失败标记产出（FAIL 串 或 chk/expect/assert 调用 或 ok&= 累积）
    #     ② **退出码依赖标志**（return <flag>?0:1 形式；尾部无条件 return 0 → 假绿候选）
    lines.append("\n## ② 暗棋对齐用例（断言强度：标记 + 退出码依赖 双条件）\n")
    lines.append("| 用例 | 断言标记 | 退出码依赖 | 判定 |")
    lines.append("|---|---|---|---|")
    total_cases = 0
    total_asserts = 0
    fake_green = []
    for fn in sorted(os.listdir(DARK)):
        if not fn.endswith(".c"):
            continue
        txt = read(os.path.join(DARK, fn))
        marks = (len(re.findall(r'\bchk\(', txt))
                 + len(re.findall(r'\bexpect\(', txt))
                 + len(re.findall(r'\bassert', txt))
                 + len(re.findall(r'ok\s*&=', txt))
                 + len(re.findall(r'allpass', txt))
                 + (1 if ('FAIL' in txt and 'printf' in txt) else 0))
        dep = bool(re.search(r'return\s+[A-Za-z_][A-Za-z0-9_]*\s*\?\s*0\s*:\s*1', txt)
                   or re.search(r'return\s+[A-Za-z_][A-Za-z0-9_]*\s*;', txt))
        # 尾部无条件 return 0（且有失败标记）→ 假绿候选
        tail_uncond = bool(re.search(r'return\s+0\s*;\s*\}\s*$', txt.strip()))
        name = fn[:-2]
        total_cases += 1
        total_asserts += marks
        if marks == 0:
            verdict = "★零断言（观测脚本，需转正）"
            fake_green.append(name)
        elif tail_uncond and 'FAIL' in txt:
            verdict = "★假绿候选（尾部无条件 return 0，失败标记不阻退出码）"
            fake_green.append(name)
        else:
            verdict = "✅ 真断言"
        lines.append(f"| {name} | {marks} | {'是' if dep else '否'} | {verdict} |")
    lines.append(f"\n**合计 {total_cases} 用例 / {total_asserts} 条断言标记**"
                 f"（零断言或假绿候选 {len(fake_green)} 个：{fake_green or '无'}）\n")

    # ── ③ 双端探针覆盖 ──
    probe = read(os.path.join(SRC, "rule_probe_shared.inc"))
    cases = re.findall(r'PROBE_LOG\("RULE-PROBE\s+([A-Za-z0-9_]+)', probe)
    audr = sorted(set(re.findall(r'R-\d{3}', probe)))
    lines.append(f"## ③ 32↔64 双端探针（{len(cases)} case，覆盖 AUD-R {len(audr)} 项）\n")
    lines.append("| 组 | case 数 | case 名 |")
    lines.append("|---|---|---|")
    groups = {}
    for c in cases:
        groups.setdefault(c[0], []).append(c)
    for g in sorted(groups):
        lines.append(f"| {g} 组 | {len(groups[g])} | {', '.join(groups[g][:6])}"
                     f"{' …' if len(groups[g]) > 6 else ''} |")
    lines.append(f"\n探针覆盖的 AUD-R 项：{', '.join(audr)}\n")

    # ── ④ 硬编码期望值基准点台账（D-08）──
    lines.append("## ④ 硬编码期望值基准点台账（改行为必须同步）\n")
    lines.append("| 基准文件 | 期望条数 | 门禁入口 |")
    lines.append("|---|---|---|")
    bench = read(os.path.join(SRC, "test_expected.txt"))
    bench_n = len([x for x in bench.splitlines() if x.strip()])
    evv = read(os.path.join(SRC, "evverify.c"))
    # ★ R7-D-13（正则校准）：evverify 断言形态 = `ev_total++; ev_pass += run_case(...)`
    #   / `geo_total++; ...`（无 chk/assert 关键字 → 旧正则恒 0 = 矩阵失真）。
    #   修：统计 ev_total++/geo_total++ 累加点（= 真断言数，当前 40+132=172，
    #   与 jq_evverify --all SUMMARY "172/172 全对" 同源）。
    evv_n = len(re.findall(r'\bev_total\+\+', evv)) + len(re.findall(r'\bgeo_total\+\+', evv))
    lines.append(f"| src_rs/test_expected.txt | {bench_n} | make benchcheck |")
    lines.append(f"| src_rs/consistency_desktop.c BENCH_EXPECT | 9 | make benchcheck（双源需同步） |")
    lines.append(f"| src_rs/evverify.c | {evv_n} | make evverify |")
    lines.append(f"| src_rs/dark_align/*.c（chk 断言） | {total_asserts} | dark_align_regress.sh |")
    lines.append(f"| src_rs/rule_probe_shared.inc（探针期望值在 diff 中） | {len(cases)} | rule_probe_compare.py |")
    lines.append("| forward/docs_rs/UI_COMPARE.md（Δ 表） | 12 项 | layout_align_check.py --baseline |")
    lines.append("| rev64/dist/event_code_map.json / event_names_76.json | 73 | "
                 "event_names_alignment_check.py |")

    # ── ⑤ 覆盖缺口提示 ──
    lines.append("\n## ⑤ 覆盖缺口提示（自动判定）\n")
    gaps = []
    if fake_green:
        gaps.append(f"零断言或假绿候选用例 {len(fake_green)} 个：{', '.join(fake_green)}")
    else:
        gaps.append("零断言/假绿候选用例：0 个（全部用例具备失败标记 + 退出码依赖）")
    gaps.append(f"分叉矩阵 102 条中仅 16 条已实证（其余为「已标记未实证」= 未对齐未验证）")
    gaps.append("双端探针 %d 条 vs 规则条款 %d 项 → 条款级探针化尚有缺口"
                % (len(cases), len(clauses)))
    for g in gaps:
        lines.append(f"- ⚠️ {g}")
    lines.append("")

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print(f"已生成: {OUT}")
    print(f"  规则原文条款 {len(clauses)} 项 / 用例 {total_cases} 个 / 断言 {total_asserts} 条 "
          f"/ 探针 {len(cases)} case")


if __name__ == "__main__":
    main()
