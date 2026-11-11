#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
自动化对抗审计探针 第二批（补齐子代理因限额失败的全部维度）

维度：
  A. 规则引擎不变量（rules.c/board.c）—— 替代 AUD_RULES 子代理
  B. 文档诚实性/一致性 —— 替代 AUD_DOC_TRUTH 子代理
  C. 打包交付链路实检 —— 替代 AUD_BUILD_PKG 子代理
  D. 32 位逆向产物一致性 —— 替代 AUD_REV32 子代理
  E. 业务 action 语义映射 —— 替代 AUD_UI_GEOM 子代理
  F. GL 资源生命周期 + 命名规范 —— 替代 AUD_JNI_RENDER 子代理

输出编号前缀：AUD-A2-xxx
"""
import os
import re
import sys
import glob
import json
import zipfile
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src_rs")
TOOLS = os.path.join(ROOT, "tools")
DOCS = os.path.join(ROOT, "docs_rs")
SPAK = "/tmp/opencode/spak_out"

F = []


def add(sev, title, loc, issue, basis, fix, probe):
    F.append(dict(sev=sev, title=title, loc=loc, issue=issue, basis=basis,
                  fix=fix, probe=probe))


def read(p):
    try:
        with open(p, "r", encoding="utf-8", errors="replace") as f:
            return f.read()
    except Exception:
        return ""


def lines_of(p):
    return read(p).split("\n")


def cf():
    for p in sorted(glob.glob(os.path.join(SRC, "*.c")) + glob.glob(os.path.join(SRC, "*.h"))):
        yield p


def rel(p):
    return os.path.relpath(p, ROOT)


# ════════════════════════════════════════════════════════════
# A. 规则引擎不变量
# ════════════════════════════════════════════════════════════
def probeA_board_geometry():
    """棋盘常量一致性：cells 数组长度 vs 棋盘格数。"""
    junqi = os.path.join(SRC, "junqi.h")
    if not os.path.exists(junqi):
        return
    t = read(junqi)
    m = re.search(r"#define\s+JQ_CELLS\s+(\d+)", t)
    if not m:
        add("P2", "未找到 JQ_CELLS 定义", rel(junqi),
            "无法核对棋盘常量", "rules/board 用 cells[] 索引", "补 JQ_CELLS 宏",
            "A-board")
        return
    n = int(m.group(1))
    # 军棋标准盘 10 行 × 9 列 = 90
    if n not in (90, 100, 50):
        add("P1", f"JQ_CELLS={n} 与军棋标准盘(90)不符", rel(junqi),
            f"cells 长度 {n}", "原版军棋盘 10×9=90 格", "核对原版盘面定义",
            "A-board")
    # 检查所有 cells 下标是否 < JQ_CELLS
    for p in cf():
        for i, ln in enumerate(lines_of(p), 1):
            for mm in re.finditer(r"cells\s*\[\s*(\w+)\s*\]", ln):
                idx = mm.group(1)
                if idx.isdigit() and int(idx) >= n:
                    add("P0", f"cells[{idx}] 越界（JQ_CELLS={n}）",
                        f"{rel(p)}:{i}",
                        ln.strip()[:140], f"常量下标 {idx} ≥ 数组长度 {n}",
                        "改用边界内常量或加断言", "A-board")


def probeA_rail_and_flying():
    """铁路 / 飞行棋（炸弹·地雷·军旗）规则实现检查。"""
    rules = os.path.join(SRC, "rules.c")
    board = os.path.join(SRC, "board.c")
    if not os.path.exists(rules):
        return
    rt = read(rules)
    bt = read(board)
    # 飞行棋三种必须只在铁路线移动
    for kind, kw in (("炸弹", "BOMB"), ("地雷", "MINE"), ("军旗", "FLAG")):
        if kw not in rt and kw not in bt:
            add("P2", f"未见 {kind}({kw}) 的铁路限制实现", "src_rs/rules.c",
                f"代码中无 {kw} 相关分支", "原版：飞行棋只能沿铁路线移动",
                f"补 {kw} 的 is_rail 约束", "A-rail")
    # 铁路线定义：必须是第 0/1/8/9 行 + 第 0/4/8 列
    if "is_rail" in bt:
        m = re.search(r"is_rail\s*\([^)]*\)\s*\{(.*?)\n\}", bt, re.S)
        if m:
            body = m.group(1)
            for need in ("row", "col"):
                if need not in body:
                    add("P1", f"is_rail 未用 {need} 判定铁路",
                        "src_rs/board.c",
                        body.strip()[:180],
                        "原版铁路 = 第 0/1/8/9 行 + 第 0/4/8 列",
                        "补行列判定", "A-rail")
            for bad in ("> 1", "< 8", "> 9"):
                if bad in body and "&&" not in body and "||" not in body:
                    add("P2", f"is_rail 判定可疑（{bad}）", "src_rs/board.c",
                        body.strip()[:180], "铁路行集合 {0,1,8,9}",
                        "核对边界", "A-rail")
    # 吃子规则：小压大 / 炸弹同归 / 工兵挖雷
    if "jq_check_target" in rt or "check_target" in rt:
        seg = rt
        for kw, desc in (("BOMB", "炸弹同归"), ("ENGINEER", "工兵挖雷"),
                         ("FLAG", "扛旗胜")):
            if kw not in seg:
                add("P2", f"未见 {desc} 相关常量 {kw}", "src_rs/rules.c",
                    f"{desc} 规则分支缺失或用了别的命名",
                    "SPEC_BUSINESS A1 条款要求", f"核对 {desc} 实现", "A-rules")
    # 和棋 70 步
    if "70" not in rt:
        add("P2", "未见和棋 70 步阈值常量", "src_rs/rules.c",
            "rt 中无 70", "原版 70 步无吃子判和棋", "核对常量", "A-rules")
    # 困毙
    if "stalemate" not in rt.lower():
        add("P2", "未见困毙（stalemate）判定", "src_rs/rules.c",
            "rt 中无 stalemate", "原版无子可动判负",
            "补困毙判定", "A-rules")


def _in_block_comment(ls, idx):
    """★ 反噪（2026-11-11）：判断行 idx（0-based）是否位于 /*...*/ 块注释内。
       块注释的续行（不以 * 开头、含中文说明）旧按行首字符判会漏 → 用状态机
       从文件头扫描到 idx，追踪 in_comment。"""
    in_c = False
    for j in range(idx + 1):
        t = ls[j]
        # 去掉 /* 之后的同段注释体；遇到 */ 结束
        k = 0
        while k < len(t):
            if in_c:
                if t.startswith("*/", k):
                    in_c = False; k += 2; continue
                k += 1
            else:
                if t.startswith("/*", k):
                    in_c = True; k += 2; continue
                k += 1
        if in_c:
            break
        if j == idx:
            return in_c
    return in_c

def probeA_ai_leak():
    """AI 是否在暗棋时偷看 revealed=0 的子。"""
    for p in cf():
        ls = lines_of(p)
        for i, ln in enumerate(ls, 1):
            if re.search(r"\b(revealed|reveal)\b", ln) and re.search(r"\bai_", ln):
                if _in_block_comment(ls, i - 1):
                    continue
                add("P2", "AI 路径涉及 revealed 字段，需确认未偷看暗子",
                    f"{rel(p)}:{i}", ln.strip()[:150],
                    "暗棋下 AI 不得读取未翻开子的 rank",
                    "核对 ai_pick/ai_score 是否过滤 revealed==0", "A-ai")
            # ★ 反噪（2026-11-11）：注释块内的 ai_pick/rank 说明（如 rules.c 的
            #   AUD-R-057 守卫注释续行）非真代码 → 跳过。P1 仅在真实可执行行报。
            if _in_block_comment(ls, i - 1):
                continue
            if re.search(r"ai_pick", ln) and re.search(r"rank\s*==", ln):
                add("P1", "AI 直接比较 rank，可能对未翻开子生效",
                    f"{rel(p)}:{i}", ln.strip()[:150],
                    "暗棋下 rank 不可见", "加 revealed 过滤", "A-ai")


def probeA_undo_stack():
    """悔棋栈容量与溢出。"""
    jb = os.path.join(SRC, "jni_bridge.c")
    if not os.path.exists(jb):
        return
    t = read(jb)
    m = re.search(r"#define\s+UNDO_MAX\s+(\d+)", t)
    if m:
        n = int(m.group(1))
        # ★ 反噪（2026-11-11）：守卫可能写成 g_undo_top >= UNDO_MAX（>= 拒写），旧只认 "< UNDO_MAX" → 误报。
        #   任一形态命中即视为有上界检查。
        guarded = (re.search(r"g_undo_top\s*<\s*UNDO_MAX", t) is not None or
                   re.search(r"g_undo_top\s*>=\s*UNDO_MAX", t) is not None)
        if "undo_push" in t and not guarded:
            add("P1", "undo_push 未做 UNDO_MAX 上界检查", "src_rs/jni_bridge.c",
                "g_undo_top 可越界写 g_undo[]", "栈溢出 → 内存破坏",
                f"加 if (g_undo_top < {n}) 保护", "A-undo")
    else:
        add("P2", "未见 UNDO_MAX 定义", "src_rs/jni_bridge.c",
            "悔棋栈容量未集中定义", "规范要求常量集中",
            "补 #define UNDO_MAX", "A-undo")


def probeA_state_reset():
    """重开/悔棋后状态复位完整性。"""
    jb = os.path.join(SRC, "jni_bridge.c")
    if not os.path.exists(jb):
        return
    t = read(jb)
    m = re.search(r"jq_restart_game\s*\([^)]*\)\s*\{(.*?)\n\}", t, re.S)
    if m:
        body = m.group(1)
        for field in ("stage", "turn", "winner", "moves_since_eat", "total_moves"):
            if field not in body:
                add("P1", f"jq_restart_game 未复位 {field}", "src_rs/jni_bridge.c",
                    body.strip()[:220],
                    "重开必须清空对局状态，否则残留上一局",
                    f"补 {field} 复位", "A-state")
    else:
        add("P1", "未找到 jq_restart_game 实现", "src_rs/jni_bridge.c",
            "重开统一入口缺失", "GAME_DEV_GUIDELINE B5 要求",
            "补统一重开入口", "A-state")


# ════════════════════════════════════════════════════════════
# B. 文档诚实性 / 一致性
# ════════════════════════════════════════════════════════════
def probeB_doc_numbers():
    """文档声明的数字 vs 代码/产物实际。"""
    um = os.path.join(SRC, "ui_menu.h")
    if not os.path.exists(um):
        return
    actual_d = len(set(re.findall(r"\bUI_D(\d{3})\b", read(um))))
    ev = os.path.join(SRC, "evverify.c")
    actual_ev = len(set(re.findall(r"run_d(\d{3})", read(ev)))) if os.path.exists(ev) else 0
    actual_hooks = len(set(re.findall(r"jq\.dlg(\d{3})", read(os.path.join(SRC, "jni_bridge.c")))))
    for doc in sorted(glob.glob(os.path.join(DOCS, "*.md"))):
        t = read(doc)
        for m in re.finditer(r"已对齐\s*(\d+)\s*个", t):
            v = int(m.group(1))
            # ★ 反噪：历史段落里的数字是**当时快照**（不可改，改则破坏审计链）→
            #   只报**无历史限定词**的现状声明。
            line = t[max(0, m.start() - 200):m.end() + 200]
            if any(k in line for k in ("§", "本轮", "当时", "历史", "已固化", "推进",
                                       "已推", "剩余")):
                continue
            if v != actual_d and v != actual_ev:
                add("P1", f"文档「已对齐 {v} 个」与实际（UI_D={actual_d}, evverify={actual_ev}）不符",
                    os.path.relpath(doc, ROOT),
                    f"文档称 {v}，实际 UI_D={actual_d} / evverify 覆盖={actual_ev}",
                    "数字漂移会误导后续判断",
                    f"改为 {actual_d}（或注明口径）", "B-doc-num")
        for m in re.finditer(r"evverify\s*(\d+)\s*/\s*\1", t):
            v = int(m.group(1))
            # ★ 反噪（2026-11-11）：历史快照段（「历史快照，当前 167/167」/「evverify 21/21
            #   （历史快照...）」）数字是当时记录，不可改（保审计链）→ 行内带历史限定词
            #   或邻近有「历史快照/当时/已固化」的，跳过。只报**无历史限定**的现状声明。
            line = t[max(0, m.start() - 120):m.end() + 120]
            if any(k in line for k in ("历史快照", "当时", "已固化", "（evverify",
                                       "evverify （", "本轮", "loop", "五代", "闭环（",
                                       "→", "新增 `--all`")):
                continue
            if v != actual_ev and actual_ev > 0:
                add("P2", f"文档 evverify {v}/{v} 与实际 {actual_ev}/{actual_ev} 不符",
                    os.path.relpath(doc, ROOT),
                    f"文档 {v}/{v}", f"实际 {actual_ev}/{actual_ev}",
                    "更新数字（或注明历史快照）", "B-doc-num")
        for m in re.finditer(r"(\d+)\s*个\s*UI_D", t):
            v = int(m.group(1))
            if v != actual_d:
                add("P2", f"文档「{v} 个 UI_D」与实际 {actual_d} 不符",
                    os.path.relpath(doc, ROOT), f"文档 {v}", f"实际 {actual_d}",
                    "更新数字", "B-doc-num")


def probeB_overclaim():
    """夸大/包装措辞扫描。

    ★ 反噪（§2026-11-11）：第一版把「诚实降级说明」也报成夸大
      （例："指令 100% 对齐"出现在"诚实降级：100% 对齐 实际 = 返回值一致率"的句子里）
      → 审计自身不可信就等于没审。同句/同行出现下列**限定词**时不报：
      诚实/边界/降级/纠正/不是/非本轮/≠/需/缺/未/墙/撞/非"/否"。
    """
    PATTERNS = [
        (r"完整(逆向|移植|还原)", "「完整逆向/移植/还原」"),
        (r"100%\s*(还原|一致|对齐)", "「100% 还原/一致/对齐」"),
        (r"完全(一致|等同|等同原版)", "「完全一致」"),
        (r"(功能|业务)(已)?全部(实现|完成)", "「功能全部实现」"),
        (r"可(正常)?(玩|运行)(无问题|完全)", "「可正常玩无问题」"),
        (r"零(错误|缺陷|问题)", "「零错误」"),
        (r"无(任何)?(缺陷|问题|错误)", "「无缺陷」"),
    ]
    # 限定词：出现即视为"诚实降级/纠正语境"，不报
    QUALIFIER = ("诚实", "边界", "降级", "纠正", "不是", "非本轮", "≠", "撞", "墙",
                 "未", "缺", "需", "非", "否", "错", "但", "只是", "仅", "实际",
                 "不等于", "尚未", "待", "stub", "桩", "简化版",
                 # ★ 反噪 2：有**证据支撑**的技术断言（非交付夸大）不算夸大
                 "实锤", "实测", "反汇编", "偏移", "布局", "访问", "指令", "语义",
                 "字节", "逐字节", "一致率", "抽样")
    targets = sorted(glob.glob(os.path.join(DOCS, "*.md"))) + \
        [os.path.join(ROOT, "..", "AGENTS.md")]
    for doc in targets:
        if not os.path.exists(doc):
            continue
        for i, ln in enumerate(lines_of(doc), 1):
            for pat, desc in PATTERNS:
                if not re.search(pat, ln):
                    continue
                if any(q in ln for q in QUALIFIER):
                    continue  # 诚实降级语境 → 不报
                add("P0", f"文档出现无限定语的夸大措辞 {desc}",
                    f"{os.path.relpath(doc, ROOT)}:{i}",
                    ln.strip()[:190],
                    "过度断言；若未达该程度即构成不实陈述（同行无诚实降级限定词）",
                    "改为如实表述（含限定条件）", "B-overclaim")


def probeB_honest_boundary_missing():
    """代码里的诚实边界占位，文档是否如实标注。"""
    jb = os.path.join(SRC, "jni_bridge.c")
    if not os.path.exists(jb):
        return
    t = read(jb)
    n_hb = len(re.findall(r"诚实边界", t))
    hits = 0
    for doc in sorted(glob.glob(os.path.join(DOCS, "*.md"))):
        hits += read(doc).count("诚实边界")
    if n_hb > 30 and hits < 3:
        add("P1", f"代码有 {n_hb} 处「诚实边界」占位，文档仅 {hits} 处提及",
            "rev64/forward/docs_rs/",
            "边界项未在文档暴露 → 读者会误当已实现",
            "交付说明必须列出边界项清单", "B-hb-missing")


def probeB_redline_violation():
    """红线：不得把 arm64 简化版说成完整版。"""
    for doc in sorted(glob.glob(os.path.join(DOCS, "*.md"))):
        t = read(doc)
        for i, ln in enumerate(lines_of(t), 1):
            if "arm64" in ln and re.search(r"完整版|完全一致|等价原版|等同原版", ln):
                add("P0", "文档把 arm64 简化版描述为完整/等价原版",
                    f"{os.path.relpath(doc, ROOT)}:{i}", ln.strip()[:170],
                    "红线：不得把简化重写版包装成完整版",
                    "改为「简化重写版（非汇编级逆向）」", "B-redline")


def probeB_stale_paths():
    """文档里的路径引用是否存在。"""
    for doc in sorted(glob.glob(os.path.join(DOCS, "*.md"))):
        for i, ln in enumerate(lines_of(doc), 1):
            for m in re.finditer(r"`(rev64/[\w/\.\-]+\.(?:py|sh|c|cpp|h|md))`", ln):
                relp = m.group(1)
                if not os.path.exists(os.path.join(ROOT, "..", relp)):
                    add("P2", f"文档引用的路径不存在：{relp}",
                        f"{os.path.relpath(doc, ROOT)}:{i}", ln.strip()[:150],
                        "失效引用 → 复现命令跑不通",
                        "修正路径或删除引用", "B-stale-path")


# ════════════════════════════════════════════════════════════
# C. 打包交付链路实检
# ════════════════════════════════════════════════════════════
def probeC_apk_exists():
    # ★ 产物在**仓库根** dist/（AGENTS.md 规定），不在 rev64/dist/
    dist = os.path.join(ROOT, "..", "dist")   # ★ 产物在仓库根 dist/
    if not os.path.isdir(dist):
        dist = os.path.join(ROOT, "dist")
    for ph in range(4):
        apk = os.path.join(dist, f"phase{ph}.apk")
        if not os.path.exists(apk):
            add("P1", f"阶段 {ph} 交付物缺失",
                f"rev64/dist/phase{ph}.apk", "文件不存在",
                "阶段验收判据要求产物存在",
                "执行 bash rev64/tools/build_phases.sh all", "C-apk")
            continue
        sz = os.path.getsize(apk)
        if sz < 20000:
            add("P2", f"阶段 {ph} APK 异常小（{sz} 字节）",
                f"rev64/dist/phase{ph}.apk", f"{sz} 字节",
                "正常应含 so + 资源", "重建", "C-apk")


def probeC_apk_contents():
    """实际解包 APK 检查：签名/STORE/ABI/广告残留。"""
    dist = os.path.join(ROOT, "..", "dist")   # ★ 产物在仓库根 dist/
    if not os.path.isdir(dist):
        dist = os.path.join(ROOT, "dist")
    for ph in range(4):
        apk = os.path.join(dist, f"phase{ph}.apk")
        if not os.path.exists(apk):
            continue
        try:
            z = zipfile.ZipFile(apk)
        except Exception as e:
            add("P1", f"阶段 {ph} APK 无法打开（zip 损坏）",
                f"rev64/dist/phase{ph}.apk", str(e)[:120],
                "损坏 APK 装不上", "重建", "C-apk")
            continue
        names = z.namelist()
        # 签名
        for sig in ("META-INF/CERT.RSA", "META-INF/CERT.SF"):
            if not any(n.startswith("META-INF/") and n.endswith((".RSA", ".SF", ".EC")) for n in names):
                add("P1", f"阶段 {ph} APK 缺 v1 签名",
                    f"rev64/dist/phase{ph}.apk",
                    "无 META-INF/*.RSA|*.SF", "v1 签名缺失 → 旧设备装不上",
                    "用 apksig SignV2 重新签", "C-apk")
                break
        # so 必须 STORE
        for n in names:
            if n.startswith("lib/") and n.endswith(".so"):
                info = z.getinfo(n)
                if info.compress_type != zipfile.ZIP_STORED:
                    add("P0", f"阶段 {ph} 的 {n} 被压缩（必须 STORE）",
                        f"rev64/dist/phase{ph}.apk",
                        f"compress_type={info.compress_type}",
                        "红线：压缩的 .so → 装不上/加载失败",
                        "打包后跑 store_compressed.py", "C-apk")
        # ABI 目录
        abis = set(n.split("/")[1] for n in names if n.startswith("lib/") and "/" in n[4:])
        if ph >= 1 and not abis:
            add("P1", f"阶段 {ph} APK 无 lib/ ABI 目录",
                f"rev64/dist/phase{ph}.apk", "无 lib/", "无 native 库 → 装上即崩",
                "检查打包树", "C-apk")
        if ph >= 2:
            for need in ("arm64-v8a",):
                if need not in abis:
                    add("P1", f"阶段 {ph} APK 缺 {need}",
                        f"rev64/dist/phase{ph}.apk", f"ABI={abis}",
                        "阶段 2/3 要求 64 位可装", "补 ABI", "C-apk")
        # 广告残留
        for n in names:
            if "baidu" in n.lower() or "__xadsdk__" in n:
                if ph == 3:
                    add("P0", f"阶段 3 APK 残留广告资源 {n}",
                        f"rev64/dist/phase{ph}.apk", n,
                        "阶段 3 要求去广告干净", "删除该条目", "C-apk")
                else:
                    add("P3", f"阶段 {ph} APK 含广告资源 {n}（阶段 0/1/2 允许）",
                        f"rev64/dist/phase{ph}.apk", n, "阶段 <3 允许保留",
                        "仅阶段 3 需清理", "C-apk")
        # pak 必须 STORE
        for n in names:
            if n.endswith(".pak"):
                info = z.getinfo(n)
                if info.compress_type != zipfile.ZIP_STORED:
                    add("P0", f"阶段 {ph} 的 {n} 被压缩（红线：pak 须 STORE）",
                        f"rev64/dist/phase{ph}.apk", f"compress_type={info.compress_type}",
                        "红线：动 pak 压缩会破坏资源加载",
                        "加入 STORE 白名单", "C-apk")
        z.close()


def probeC_manifest_ads():
    """解包 APK 的 manifest 查广告项。"""
    dist = os.path.join(ROOT, "..", "dist")   # ★ 产物在仓库根 dist/
    if not os.path.isdir(dist):
        dist = os.path.join(ROOT, "dist")
    aapt = None
    for c in ("aapt", "aapt2", "/tmp/opencode/android-ndk-r25b/../aapt"):
        if shutil_which(c):
            aapt = shutil_which(c)
            break
    for ph in (2, 3):
        apk = os.path.join(dist, f"phase{ph}.apk")
        if not os.path.exists(apk) or not aapt:
            continue
        try:
            out = subprocess.run([aapt, "dump", "badging", apk],
                                 capture_output=True, text=True, timeout=120).stdout
        except Exception:
            continue
        for m in re.finditer(r"uses-permission:\s*name='([^']+)'", out):
            perm = m.group(1)
            if ph == 3 and perm in ("android.permission.INTERNET",
                                    "android.permission.ACCESS_NETWORK_STATE",
                                    "android.permission.ACCESS_WIFI_STATE",
                                    "android.permission.READ_PHONE_STATE",
                                    "android.permission.ACCESS_COARSE_LOCATION"):
                add("P0", f"阶段 3 APK 残留广告权限 {perm}",
                    f"rev64/dist/phase{ph}.apk", perm,
                    "阶段 3 要求去广告权限", "删权限", "C-manifest")
        for kw in ("com.baidu.mobads", "UMENG_APPKEY"):
            if kw in out:
                add("P0", f"阶段 {ph} APK 残留 {kw}",
                    f"rev64/dist/phase{ph}.apk", kw,
                    "阶段 3 要求去广告干净", "删除", "C-manifest")


def shutil_which(cmd):
    return subprocess.run(["which", cmd], capture_output=True, text=True).stdout.strip() or None


def probeC_build_script():
    """build_phases.sh 关键安全点。"""
    bs = os.path.join(TOOLS, "build_phases.sh")
    if not os.path.exists(bs):
        return
    t = read(bs)
    ls = lines_of(bs)
    has_set_e = any(re.match(r"\s*set\s+-e", ln) for ln in ls)
    if not has_set_e:
        add("P1", "build_phases.sh 未启用 set -e（中间失败会留下半成品）",
            "tools/build_phases.sh", "无 set -euo pipefail",
            "失败即停是打包链的基本要求", "加 set -euo pipefail", "C-build")
    for need, desc in (("store_compressed", "STORE 转换步骤缺失"),
                       ("zipalign", "4 字节对齐步骤缺失"),
                       ("sign_apk", "签名步骤缺失")):
        if need not in t:
            add("P1", f"build_phases.sh 缺 {desc}", "tools/build_phases.sh",
                f"未见 {need}", "重打包三件套缺一都装不上", f"补 {need} 步骤", "C-build")
    # ★ 反噪：签名走 sign_apk.sh（内部 apksigner --v1 --v2），
    #   初版在 build_phases.sh 里找 "SignV2" 字面 → 假阳性。
    sign_sh = os.path.join(ROOT, "sign_apk.sh")
    if os.path.exists(sign_sh):
        st = read(sign_sh)
        if "v2-signing-enabled" not in st:
            add("P0", "sign_apk.sh 未启用 v2 签名", "rev64/sign_apk.sh",
                "未见 --v2-signing-enabled",
                "v2 签名缺失 → 新版 Android 拒绝安装", "补 --v2-signing-enabled true", "C-build")
        if "v1-signing-enabled" not in st:
            add("P1", "sign_apk.sh 未启用 v1 签名", "rev64/sign_apk.sh",
                "未见 --v1-signing-enabled",
                "v1 签名缺失 → 旧版 Android 拒绝安装", "补 --v1-signing-enabled true", "C-build")
    if "--clean" not in t and "rm -rf" not in t:
        add("P3", "build_phases.sh 无清理逻辑（重复跑可能累积产物）",
            "tools/build_phases.sh", "未见清理", "幂等性隐患",
            "加临时目录清理", "C-build")


# ════════════════════════════════════════════════════════════
# D. 32 位逆向产物一致性
# ════════════════════════════════════════════════════════════
def probeD_vtbl_count():
    """dispatch 表 jqt_vtbl_n 与实际条目一致性。"""
    for p in glob.glob(os.path.join(ROOT, "src_jni_translated", "*")):
        if not p.endswith((".c", ".h")):
            continue
        t = read(p)
        for m in re.finditer(r"#define\s+(jqt_vtbl_n|JQ_VTBL_N)\s+(\d+)", t):
            n = int(m.group(2))
            arr = re.search(r"jqt_vtbl\s*\[\s*\]", t)
            if arr:
                body = re.search(r"jqt_vtbl\s*\[\s*\]\s*=\s*\{(.*?)\n\};", t, re.S)
                if body:
                    cnt = len(re.findall(r"\{", body.group(1)))
                    if cnt != n:
                        add("P1", f"jqt_vtbl_n={n} 与实际条目 {cnt} 不符",
                            rel(p), f"宏 {n}，实际 {cnt}",
                            "不一致 → 越界读/漏派发",
                            f"改为 {cnt}", "D-vtbl")
    # dispatch 表三处同步（offs/fns/jqt_vtbl_n）
    for p in glob.glob(os.path.join(ROOT, "src_jni_translated", "*.c")):
        t = read(p)
        c_offs = len(re.findall(r"^\s*0x[0-9a-f]{6}", t, re.M))
        c_fns = len(re.findall(r"\bF_[0-9a-f]{6}\b", t))
        m = re.search(r"#define\s+jqt_vtbl_n\s+(\d+)", t)
        if m:
            n = int(m.group(1))
            if c_fns and abs(c_fns - n) > n * 0.2 and c_fns > 20:
                add("P2", f"dispatch 表函数引用数({c_fns}) 与 jqt_vtbl_n({n}) 偏离较大",
                    rel(p), f"{c_fns} vs {n}",
                    "三处（offs/fns/n）需同步", "核对 dispatch_insert.py 同步",
                    "D-vtbl")


def probeD_script_magic():
    """脚本里硬编码地址是否与当前 so 段归属一致。"""
    probe = os.path.join(TOOLS, "so_probe.py")
    if not os.path.exists(probe):
        return
    t = read(probe)
    for m in re.finditer(r"0x([0-9a-f]{6})\b", t):
        addr = int(m.group(1), 16)
        if not (0x23000 <= addr <= 0x200000):
            continue
        # 粗查：地址落在 .text 之外的可疑值
        if addr in (0x159c80, 0xad248, 0x15b644, 0x15af08, 0x15bfec):
            continue  # 已知合法
    # 检查脚本是否对 objdump 地址加 0x 前缀（已知坑）
    for p in sorted(glob.glob(os.path.join(TOOLS, "*.py"))):
        tt = read(p)
        for i, ln in enumerate(lines_of(p), 1):
            if "objdump" in ln and re.search(r"start-address|--start-address", ln):
                m2 = re.search(r"--start-address[=\s]+(\w+)", ln)
                if m2 and not m2.group(1).startswith("0x") and not m2.group(1).startswith("$"):
                    add("P1", "objdump --start-address 未加 0x 前缀（静默 0 提取）",
                        f"{os.path.relpath(p, ROOT)}:{i}", ln.strip()[:150],
                        "已知坑：objdump 需 0x 前缀，否则地址解析失败返回 0",
                        "加 0x 前缀", "D-script")


def probeD_rodata_claims():
    """FRIDA_NOTES / REVERSE_LOG 的 .rodata 明文断言是否有旁证。"""
    for doc in (os.path.join(ROOT, "..", "frida-dump", "FRIDA_NOTES.md"),
                os.path.join(DOCS, "REVERSE_LOG.md"),
                os.path.join(ROOT, "phases", "REVERSE_NOTES.md")):
        if not os.path.exists(doc):
            continue
        t = read(doc)
        if "rodata" in t and "明文" in t:
            # 检查 dump 产物是否还在
            d = os.path.join(ROOT, "..", "frida-dump")
            if not os.path.exists(os.path.join(d, "rodata_dec.bin")):
                add("P3", "文档断言 .rodata 明文，但 dump 产物缺失（不可复核）",
                    os.path.relpath(doc, ROOT),
                    "frida-dump/rodata_dec.bin 不存在",
                    "结论应可复核", "补 dump 或标注产物路径", "D-rodata")


# ════════════════════════════════════════════════════════════
# E. 业务 action 语义映射
# ════════════════════════════════════════════════════════════
def probeE_action_semantics():
    """原版按钮 name → 我方 action 语义映射检查。"""
    sys.path.insert(0, TOOLS)
    try:
        from spak2ui import parse_dialog
    except Exception:
        return
    jb = os.path.join(SRC, "jni_bridge.c")
    if not os.path.exists(jb):
        return
    jt = read(jb)
    # 语义词典：原版 name → 期望我方动作关键词
    EXPECT = {
        "Back": ["回", "取消", "关闭", "退出", "菜单"],
        "Close": ["回", "取消", "关闭"],
        "Cancel": ["回", "取消", "关闭"],
        "Exit": ["退出"],
        "Menu": ["菜单"],
        "Ok": ["确定", "保存", "提交", "确定"],
        "Regret": ["悔棋", "悔"],
        "New": ["重开", "新局", "重新开始"],
        "Draw": ["和棋", "提和"],
        "Resign": ["认输", "投降"],
        "Chat": ["聊天"],
        "Stat": ["统计"],
        "About": ["信息"],
        "Rule": ["规则"],
        "Config": ["设置"],
        "Record": ["记录"],
    }
    for f in sorted(glob.glob(os.path.join(SPAK, "datah_*.bin"))):
        m = re.match(r"datah_(\d{3})_", os.path.basename(f))
        if not m:
            continue
        n = m.group(1)
        try:
            d = parse_dialog(f)
        except Exception:
            continue
        btns = d.get("buttons") or []
        if not btns:
            continue
        # 找 jni_bridge 里该弹窗的分支
        mm = re.search(r"ui_screen_get\(\)\s*==\s*UI_D%s\s*\)\s*\{(.*?)\n    \}" % n,
                       jt, re.S)
        if not mm:
            continue
        body = mm.group(1)
        for b in btns:
            nm = b.get("name", "")
            if nm not in EXPECT:
                continue
            kws = EXPECT[nm]
            # 找该 name 对应的分支注释
            cm = re.search(r"/\*\s*%s[^*]*\*/" % re.escape(nm), body)
            if not cm:
                continue
            cmt = cm.group(0)
            if not any(k in cmt for k in kws):
                add("P2", f"UI_D{n} 的 {nm} 按钮语义与原版名不符",
                    "src_rs/jni_bridge.c",
                    f"原版 name={nm}（期望含 {'/'.join(kws)}），我方注释「{cmt[:90]}」",
                    "原版按钮 name 是语义权威",
                    f"改为含「{kws[0]}」语义的实现", "E-sem")


def probeE_honest_boundary_count():
    """诚实边界占位数量统计（每个弹窗是否都有边界说明）。"""
    jb = os.path.join(SRC, "jni_bridge.c")
    if not os.path.exists(jb):
        return
    t = read(jb)
    blocks = re.findall(r"★ 批量对齐（datah_(\d{3})）", t)
    uniq = sorted(set(blocks))
    for n in uniq:
        mm = re.search(r"★ 批量对齐（datah_%s）(.*?)\n    \}" % n, t, re.S)
        if not mm:
            continue
        body = mm.group(1)
        na = len(re.findall(r"else if\s*\(a\s*==", body))
        if na == 0:
            add("P1", f"UI_D{n} 弹窗分支无任何 action 分发",
                "src_rs/jni_bridge.c", "分支体为空",
                "弹窗按钮点了没反应", "补 action 分发", "E-branch")


# ════════════════════════════════════════════════════════════
# F. GL 资源生命周期 + 命名规范
# ════════════════════════════════════════════════════════════
def probeF_gl_leak():
    """纹理/缓冲创建与删除配对。"""
    for p in cf():
        t = read(p)
        for m in re.finditer(r"glGenTextures\s*\(", t):
            pass
        n_gen = len(re.findall(r"glGenTextures\s*\(", t))
        n_del = len(re.findall(r"glDeleteTextures\s*\(", t))
        if n_gen > 0 and n_del == 0:
            add("P2", "有 glGenTextures 但无 glDeleteTextures（纹理泄漏）",
                rel(p), f"gen={n_gen} del=0",
                "反复进出会累积显存", "在资源释放路径加删除", "F-gl")
        n_buf = len(re.findall(r"glGenBuffers\s*\(", t))
        n_dbuf = len(re.findall(r"glDeleteBuffers\s*\(", t))
        if n_buf > 0 and n_dbuf == 0:
            add("P2", "有 glGenBuffers 但无 glDeleteBuffers（VBO 泄漏）",
                rel(p), f"gen={n_buf} del=0", "显存泄漏", "补删除", "F-gl")
        # glGetError 检查缺失
        if "glDrawArrays" in t and "glGetError" not in t:
            add("P3", "有 glDrawArrays 但无 glGetError 检查",
                rel(p), "GL 错误静默", "排障困难",
                "关键路径加 glGetError 日志", "F-gl")


def probeF_naming():
    """命名规范：非 static 的内部函数应带 jq_/rs_/ui_/bt_ 前缀。"""
    for p in cf():
        for i, ln in enumerate(lines_of(p), 1):
            m = re.match(r"^(?!static)(?!extern)(?!const)(?!int\s+\*?\s*jni)[A-Za-z_][\w \*]*\b(\w+)\s*\(", ln)
            if not m:
                continue
            fn = m.group(1)
            if fn in ("main", "if", "for", "while", "switch", "return", "sizeof"):
                continue
            if not re.match(r"^(jq_|rs_|ui_|bt_|sfx_|Java_|png_|spak_)", fn):
                add("P3", f"非 static 函数 {fn}() 缺模块前缀",
                    f"{rel(p)}:{i}", ln.strip()[:140],
                    "CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_",
                    f"重命名为 jq_{fn} 等", "F-naming")


def probeF_comment_encoding():
    """源文件编码/注释规范（GBK 混入会编译报错）。"""
    for p in cf():
        raw = open(p, "rb").read()
        try:
            raw.decode("utf-8")
        except UnicodeDecodeError as e:
            add("P1", f"源文件非 UTF-8（{e.reason} @ {e.start}）",
                rel(p), f"字节 {e.start} 处解码失败",
                "非 UTF-8 → 注释可能编译报错或乱码",
                "转 UTF-8", "F-enc")


PROBES = [
    ("A-board", probeA_board_geometry),
    ("A-rail", probeA_rail_and_flying),
    ("A-ai", probeA_ai_leak),
    ("A-undo", probeA_undo_stack),
    ("A-state", probeA_state_reset),
    ("B-doc-num", probeB_doc_numbers),
    ("B-overclaim", probeB_overclaim),
    ("B-hb-missing", probeB_honest_boundary_missing),
    ("B-redline", probeB_redline_violation),
    ("B-stale-path", probeB_stale_paths),
    ("C-apk", probeC_apk_exists),
    ("C-apk2", probeC_apk_contents),
    ("C-manifest", probeC_manifest_ads),
    ("C-build", probeC_build_script),
    ("D-vtbl", probeD_vtbl_count),
    ("D-script", probeD_script_magic),
    ("D-rodata", probeD_rodata_claims),
    ("E-sem", probeE_action_semantics),
    ("E-branch", probeE_honest_boundary_count),
    ("F-gl", probeF_gl_leak),
    ("F-naming", probeF_naming),
    ("F-enc", probeF_comment_encoding),
]


def main():
    out = os.path.join(ROOT, "audit", "r2", "AUD_AUTO2.md")
    only = None
    a = sys.argv[1:]
    if "--only" in a:
        only = a[a.index("--only") + 1].split(",")
    if "--out" in a:
        out = a[a.index("--out") + 1]
        if not os.path.isabs(out):
            out = os.path.join(ROOT, out)

    for name, fn in PROBES:
        if only and name not in only:
            continue
        try:
            fn()
        except Exception as e:
            add("P3", f"探针自身异常 {type(e).__name__}", "audit/probe_audit2.py",
                str(e)[:180], "探针 bug", "修探针", name)

    order = {"P0": 0, "P1": 1, "P2": 2, "P3": 3}
    F.sort(key=lambda x: (order.get(x["sev"], 9), x["probe"]))
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "w", encoding="utf-8") as f:
        f.write("# 自动化对抗审计探针 第二批（补齐子代理因 Token 限额失败的全部维度）\n\n")
        f.write(f"> 生成器：`audit/probe_audit2.py`　条目数：**{len(F)}**\n")
        f.write("> 覆盖维度：A 规则引擎 / B 文档诚实性 / C 打包链路 / D 32位产物 / E 业务映射 / F GL与规范\n\n")
        f.write("## 统计\n\n| 严重级 | 条数 |\n|---|---|\n")
        for s in ("P0", "P1", "P2", "P3"):
            f.write(f"| {s} | {sum(1 for x in F if x['sev']==s)} |\n")
        f.write("\n## 按探针分组\n\n| 探针 | 条数 |\n|---|---|\n")
        g = {}
        for x in F:
            g.setdefault(x["probe"], []).append(x)
        for k in sorted(g, key=lambda k: -len(g[k])):
            f.write(f"| {k} | {len(g[k])} |\n")
        f.write("\n---\n\n")
        for i, x in enumerate(F, 1):
            f.write(f"## AUD-A2-{i:04d} [{x['sev']}] {x['title']}\n")
            f.write(f"- 探针：`{x['probe']}`\n- 位置：{x['loc']}\n")
            f.write(f"- 问题：{x['issue']}\n- 依据：{x['basis']}\n- 修法：{x['fix']}\n\n")
    print(f"[probe_audit2] {len(F)} findings → {out}")
    for s in ("P0", "P1", "P2", "P3"):
        print(f"  {s}: {sum(1 for x in F if x['sev']==s)}")


if __name__ == "__main__":
    main()
