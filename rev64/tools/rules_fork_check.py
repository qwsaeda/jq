#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
rules_fork_check.py — R-138 分叉检测脚本（src_rs 重写 vs src_jni_translated 翻译体）

诚实边界前提：
  · src_rs/rules.c            = 64 位 C 正向重写（jni_bridge 调用，跑 64 位设备）
  · src_jni_translated/jq_rs_rules.c = 32 位翻译体（§124y-19 会话 B 规则引擎走子桥，
    汇编级 jqt_ 语境，跑 32 位设备）
  · 两者共同祖先 = commit 401a735（§124y-19），此后 src_rs 持续演进，
    翻译体侧只跟了 §124y-154/156/157（32 位路线事件表回填实证），未同步 src_rs 的
    AUD-R 修复批 → 产生行为分叉。

同步机制 = 行为一致性闸门（非代码同步）：
  · 32 位基准 = rev64/tools/arm32_walkregress.sh（4 路径回归，F_3ab80/F_3ab64 判据）
  · 64 位基准 = evverify 167/167 + regress.py 50/50 + consistency 9/9
  单条路线 PASS 不代表另一条路线对齐。

本脚本只做只读扫描 + 报告，不改任何 .c 文件：
  1. 从 git log 提取 401a735..HEAD 内改动过 src_rs/rules.c 的 commit 清单
  2. 从各 commit message + 当前 src_rs/rules.c 注释提取 AUD-R 项号（R-xxx）
  3. grep 翻译体侧（jq_rs_rules.c/h + jq_rs_walkbridge.c + 相关 32 位实证文件）
     是否出现对应 R-xxx 实证标记 / §124y 实证节
  4. 输出分叉矩阵：每条 src_rs 已修 AUD-R 项 → 翻译体侧实证状态
     （EVIDENCED / NO-EVIDENCE / N/A-层级不适用）
   5. ★ §124y-161：识别翻译体侧 "AUD-R 分叉标记" 注释（jq_rs_rules.c /
      jq_rs_walkbridge.c 内 "§124y-xxx AUD-R 分叉标记" 块注释中标出的 R-xxx）：
      已标记项 → 状态从 [分叉]（无实证）细化为 [已标记]（已标注为 64 位独有
      设计 / 32 位走子桥简化 / 行为等价）。
      "已标记" ≠ "已实证对齐"——只是降模糊度。
  6. ★ §124y-169（行为实证）：rule_probe_compare.py 用**逐字节同源**的规则行为
      探针（rev64/src_rs/rule_probe_shared.inc，32/64 双端编译）让两端引擎跑
      同一批棋盘位置 → diff 输出。**行为一致 = 该项已实证**；行为不一致 =
      **实证分歧**（分叉坐实，非判读推测）。
      实证覆盖项见 EVIDENCED_PROBE 映射表（探针用例 → AUD-R 项）。
"""
import re
import subprocess
import sys
import os

WS = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

RS_RULES = "rev64/src_rs/rules.c"
TR_RULES = "rev64/src_jni_translated/jq_rs_rules.c"
TR_RULES_H = "rev64/src_jni_translated/jq_rs_rules.h"
TR_WALK = "rev64/src_jni_translated/jq_rs_walkbridge.c"
TR_TEST = "rev64/src_jni_translated/jq_rs_walkbridge_test.c"
ANCESTOR = "401a735"

# 32 位侧的实证标记来源（§124y 事件表回填批次 + 走子桥回归脚本）
TR_EVIDENCE_GLOBS = [
    "rev64/src_jni_translated/jq_rs_rules.c",
    "rev64/src_jni_translated/jq_rs_rules.h",
    "rev64/src_jni_translated/jq_rs_walkbridge.c",
    "rev64/src_jni_translated/jq_rs_walkbridge_test.c",
    "rev64/tools/arm32_walkregress.sh",
]

# ★ §124y-169 行为实证映射：探针用例 → 覆盖的 AUD-R 项
#   依据 rev64/tools/rule_probe_compare.py 的双端 diff 结果（/tmp/opencode/probe*.txt）
#   · 一致 = 该项**已实证**（两端引擎在同一批棋盘位置上判定相同）
#   · 不一致 = **实证分歧**（分叉坐实）
# 说明：探针只覆盖可脚本化的规则判定点，不代表全量规则等价。

# ★ 实证分歧（双端行为不同 → 分叉坐实）：R-073 困毙白名单排地雷
PROBE_DIVERGENT = {
    "R-073": "C4：64 位困毙=1（排地雷）/ 32 位困毙=0（未排地雷）。"
              "★ 归因精确化（§124y-169 续，C4b 对照用例）：C4b=\u552f一暗子=军旗 → "
              "两端均 SM=1（军旗排除两端一致）→ 32 位侧未排的是**地雷**而非通用终局类，"
              "分歧精确到『32 位把暗地雷算作可行动』（可能源自 32 位布局地雷可翻出的假设）",
}


# ★ §124y-169 行为实证映射：探针用例 → 覆盖的 AUD-R 项（据双端 diff 结果，
#   一致 = 已实证；C4 = 实证分歧 R-073）。值内用例短代号（A1/A2/C4b...）被
#   probe_case_ids 解析用于 32/64 位落盘行匹配。
EVIDENCED_PROBE = {
    "R-001": "A1 A2（底线纵滑/横滑 can_move）",
    "R-002": "B1 B2 C6（地雷仅工兵吃 + 挖雷计数）",
    "R-005": "C5（困毙查 side != turn）",
    "R-006": "C1（未定色暗子不可动）",
    "R-009": "B3（炸弹 vs 军旗判定）",
    "R-016": "B4（吃旗分支事件/终局）",
    "R-020": "B3（炸弹撞旗终局信号）",
    "R-025": "A5（起点越界守卫）",
    "R-028": "A3（进入空行营）",
    "R-029": "A4（大本营目标判定）",
    "R-033": "D1（翻棋 total_moves 计数）",
    "R-045": "C3（暗地雷可翻判定）",
    "R-071": "A4（空大本营进入）",
    "R-073": "C4（困毙排地雷，实证分歧：32 位未排；C4b 军旗对照两端一致 → 32 位地雷模型=翻出即死可行动，语义解读见 jq_rs_rules.c §124y-169 标注）",
    "R-086": "D1（翻棋 moves_since_eat 计数）",
    "R-089": "D2（困毙 vs 可吃旗）/ C4b（唯一暗子=军旗 → 两端 SM=1 军旗排除一致）",
}


# ★ 探针用例短代号 → AUD-R 项（从 EVIDENCED_PROBE 映射表值解析）
def probe_case_ids(lines):
    out = set()
    pat = re.compile(r"[A-D]\d+(?:b)?")
    for l in lines:
        m = re.match(r"RULE-PROBE (\S+?)[_ ]", l + " ")
        if not m:
            m = re.search(r"RULE-PROBE ([A-D]\d+(?:b)?)", l)
        if not m:
            continue
        cid = m.group(1)
        for item, tag in EVIDENCED_PROBE.items():
            for u in pat.findall(tag):
                if u == cid:
                    out.add(item)
                    break
    return out


def read_probe_result():
    """读 rule_probe_compare.py 落盘的双端探针结果 → (一致项集合, 分歧项集合)。
    无落盘文件 → 返回空（探针未跑，不影响矩阵）。"""
    p64 = "/tmp/opencode/probe64.txt"
    p32 = "/tmp/opencode/probe32.txt"
    if not (os.path.exists(p64) and os.path.exists(p32)):
        return set(), set()
    a = set(l.strip() for l in open(p64) if l.strip())
    b = set(l.strip() for l in open(p32) if l.strip())
    same = a & b
    diff = a ^ b
    return probe_case_ids(same), probe_case_ids(diff)


# 审计项号 → 主题短标签（便于矩阵可读）
THEME = {
    "R-001": "底线铁路纵滑", "R-002": "暗棋 ruleflip 三规则", "R-003": "先手翻子定色",
    "R-004": "pieces_left 越界写", "R-005": "困毙 side 语义", "R-009": "AI 炸弹撞旗",
    "R-010": "AI 暗棋透视", "R-020": "炸弹撞旗判胜", "R-061": "EV_FLIP 事件契约",
    "R-081": "三实现一致性闸门", "R-087": "count_tbl 240B 表", "R-091": "API NULL 防护",
    "R-138": "分叉同步机制（本条）",
}


def git_log_r():
    """401a735..HEAD 内改动过 src_rs/rules.c 的 commit（新→旧）。"""
    out = subprocess.run(
        ["git", "log", "--oneline", f"{ANCESTOR}..HEAD", "--", RS_RULES],
        cwd=WS, capture_output=True, text=True,
    )
    return [ln for ln in out.stdout.splitlines() if ln.strip()]


def git_log_tr():
    """401a735..HEAD 内改动过 src_jni_translated/ 的 commit（翻译体侧历史）。"""
    out = subprocess.run(
        ["git", "log", "--oneline", f"{ANCESTOR}..HEAD", "--",
         "rev64/src_jni_translated/"],
        cwd=WS, capture_output=True, text=True,
    )
    return [ln for ln in out.stdout.splitlines() if ln.strip()]


def read(p):
    fp = os.path.join(WS, p)
    if not os.path.exists(fp):
        return ""
    with open(fp, encoding="utf-8", errors="replace") as f:
        return f.read()


def extract_items_from_rs():
    """从当前 src_rs/rules.c 注释里提取被标记的 R-xxx 项（审计修复痕迹）。"""
    txt = read(RS_RULES)
    found = set(re.findall(r"\bR-\d{3}\b", txt))
    found.discard("R-138")  # R-138 是机制条，非行为修复
    return found


def extract_items_from_commit_msgs(rs_lines):
    """从 commit message 里提取 R-xxx 项号。"""
    found = set()
    for ln in rs_lines:
        found |= set(re.findall(r"\bR-\d{3}\b", ln))
    return found


def check_tr_evidence(item):
    """
    检查某 R-xxx 项在翻译体侧是否有实证痕迹。
    判据（宽松，防误报为已同步）：
      ① 翻译体 .c/.h 里出现该 R-xxx 标记（注释/断言）
      ② arm32_walkregress.sh 等 32 位实证脚本引用该项
    都没有 → NO-EVIDENCE（诚实边界：未搬移代码，翻译体侧须走 §124y 反汇编路线）。
    """
    pat = re.compile(r"\b" + item + r"\b")
    hits = []
    for g in TR_EVIDENCE_GLOBS:
        for i, ln in enumerate(read(g).splitlines(), 1):
            if pat.search(ln):
                hits.append((g, i, ln.strip()[:80]))
    return hits


def collect_fork_marks():
    """★ §124y-161：收集翻译体侧 "AUD-R 分叉标记" 注释块中提及的 R-xxx 项。
    标记 = jq_rs_rules.c / jq_rs_walkbridge.c 内 "§124y-xxx AUD-R 分叉标记"
    开头的块注释，块内提及的 R-xxx 即"已标记分叉"（诚实标注：已注明该项为
    64 位独有设计 / 32 位走子桥简化 / 行为等价，非实证对齐）。
    返回 {item: [(file, block_line, snippet)]}。
    """
    marked_files = [g for g in TR_EVIDENCE_GLOBS if "jq_rs_rules.c" in g
                    or "jq_rs_walkbridge.c" in g]
    out = {}
    for g in marked_files:
        lines = read(g).splitlines()
        # 块注释边界：标记块从其 "/* §124y-xxx AUD-R 分叉标记" 行到首个 "*/" 行
        starts = [i for i, ln in enumerate(lines, 1) if "AUD-R 分叉标记" in ln]
        for s in starts:
            end = s
            for j in range(s, len(lines) + 1):
                if j > s and "*/" in lines[j - 1]:
                    end = j
                    break
            for j in range(s, min(end, len(lines) + 1)):
                if j < 1 or j > len(lines):
                    continue
                for m in re.findall(r"\bR-\d{3}\b", lines[j - 1]):
                    if m != "R-138":
                        out.setdefault(m, []).append(
                            (g, j, lines[j - 1].strip()[:80]))
    # 同一项同一行去重
    for k, v in out.items():
        seen, uniq = set(), []
        for t in v:
            if t not in seen:
                seen.add(t)
                uniq.append(t)
        out[k] = uniq
    return out


def main():
    rs_lines = git_log_r()
    tr_lines = git_log_tr()
    items = extract_items_from_rs() | extract_items_from_commit_msgs(rs_lines)
    items.discard("R-138")
    fork_marks = collect_fork_marks()

    print("═" * 78)
    print("R-138 分叉检测矩阵（src_rs/rules.c vs src_jni_translated/jq_rs_rules.c）")
    print("═" * 78)
    print(f"共同祖先        : {ANCESTOR}（§124y-19 会话 B 规则引擎走子桥移植）")
    print(f"src_rs 侧历史   : {len(rs_lines)} commits 改过 {RS_RULES}（401a735..HEAD）")
    print(f"翻译体侧历史    : {len(tr_lines)} commits 改过 src_jni_translated/")
    for ln in tr_lines:
        print(f"  · {ln[:76]}")
    print("-" * 78)
    print("行为一致性闸门（非代码同步）：")
    print("  · 32 位基准 = rev64/tools/arm32_walkregress.sh（4 路径回归，F_3ab80 判据）")
    print("  · 64 位基准 = evverify 167/167 + regress.py 50/50 + consistency 9/9")
    print("  · 单条路线 PASS 不代表另一条路线对齐；新 AUD-R 修复须双路验证。")
    print("-" * 78)

    no_ev, ev, marked = [], [], []
    # ★ §124y-169 行为实证：读双端探针 diff 结果（一致 → 已实证；分歧 → 实证分歧）
    probe_same, probe_diff = read_probe_result()
    if probe_same:
        print("-" * 78)
        print(f"★ §124y-169 行为实证（rule_probe_compare.py 双端同源探针 diff）：")
        print(f"  已实证一致 {len(probe_same)} 项：{' '.join(sorted(probe_same))}")
        if probe_diff:
            print(f"  ★ 实证分歧 {len(probe_diff)} 项（分叉坐实，非判读推测）：")
            for it in sorted(probe_diff):
                print(f"      {it}: {PROBE_DIVERGENT.get(it, '（见 probe32/probe64 diff）')}")
        print("-" * 78)
    for item in sorted(items):
        theme = THEME.get(item, "")
        if item in fork_marks:
            # ★ §124y-161 优先级：分叉标记注释内提及的项 → [已标记]（诚实标注，
            # 非实证对齐）；仅标记块外的命中才算真实证
            marks = fork_marks[item]
            hit_lines = {(g, i) for (g, i, l) in check_tr_evidence(item)}
            mark_lines = {(g, i) for (g, i, l) in marks}
            real_hits = [(g, i, l) for (g, i, l) in check_tr_evidence(item)
                         if (g, i) not in mark_lines]
            if real_hits:
                ev.append((item, theme, real_hits))
                print(f"  [已实证] {item} {theme:<20} 翻译体侧 {len(real_hits)} 处真实证（块外）+ {len(marks)} 处分叉标记")
            else:
                marked.append((item, theme, marks))
                print(f"  [已标记] {item} {theme:<20} 翻译体侧 AUD-R 分叉标记（诚实标注，非实证）")
            for g, i, l in marks[:3]:
                print(f"             · {g}:{i}: {l}")
        else:
            hits = check_tr_evidence(item)
            if hits:
                ev.append((item, theme, hits))
                print(f"  [已实证] {item} {theme:<20} 翻译体侧 {len(hits)} 处标记")
                for g, i, l in hits[:3]:
                    print(f"             · {g}:{i}: {l}")
            else:
                no_ev.append((item, theme))
                print(f"  [分叉]   {item} {theme:<20} 翻译体侧无实证标记（未同步，诚实边界）")

    print("-" * 78)
    # ★ 对抗审计 P3-F3（口径纠正）：原汇总"已实证 N 条"口径虚高——唯一"已实证"
    #   项 R-017 实为"32 位不适用"（const 限定丢弃告警，32 位翻译体 st 非 const 入参），
    #   非真实行为实证。现拆分为"已实证 / 已标记(含不适用) / 未标记"三档，
    #   并单列"不适用"项（32 位层级无该语义）。
    na_items = [it for it, _, _ in ev if it == "R-017"]
    real_ev = [(it, t, h) for (it, t, h) in ev if it != "R-017"]
    print(f"矩阵汇总：src_rs 已修 AUD-R 项 {len(items)} 条 | "
          f"行为实证 {len(probe_same)} 条 | 实证分歧 {len(probe_diff)} 条 | "
          f"已标记分叉 {len(marked)} 条（含不适用 {len(na_items)} 条）| "
          f"未标记分叉 {len(no_ev)} 条")
    print("已标记分叉清单（翻译体侧 AUD-R 分叉标记注释：已标注 64 位独有设计 / 走子桥简化 / 行为等价，非实证对齐）：")
    for item, theme, _marks in marked:
        print(f"  · {item} {theme}")
    print("未标记分叉清单（src_rs 已修、翻译体侧既无实证也未标记，须走 32 位反汇编路线对齐）：")
    for item, theme in no_ev:
        print(f"  · {item} {theme}")
    print("-" * 78)
    print("诚实边界：本脚本只读报告。翻译体侧规则修复须走 §124y 反汇编实证路线，")
    print("不搬移 src_rs 的 C 修复（层级不同：汇编级翻译 vs C 级重写，搬了也是语义近似）。")
    print("若需将分叉项降为 0，须：① 32 位 so 走子桥补实证（arm32_walkregress 4/4 PASS）")
    print("② 在 jq_rs_rules.c 内落对应行为改动 + 实证标记；或 ③ 明确冻结翻译体规则层。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
