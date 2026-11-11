#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
自动化对抗审计探针（机器穷举）
目标：产出千级条可复核的审计发现（与子代理深度审计互补）。

用法：
  python3 audit/probe_audit.py --out audit/r2/AUD_AUTO.md [--only <probe 组>]

设计原则：
  - 每条发现必须**可复核**：给出文件:行号 / 具体数值 / 具体命令
  - 宁可标 P2/P3 也不漏项（明确标注"需人工确认"）
  - 编号连续：AUD-A-xxx
"""
import os
import re
import sys
import glob
import json
import struct
import hashlib
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src_rs")
TOOLS = os.path.join(ROOT, "tools")
SPAK = "/tmp/opencode/spak_out"

FINDINGS = []


def add(probe, sev, title, loc, issue, basis, fix):
    FINDINGS.append({
        "probe": probe,
        "sev": sev,
        "title": title,
        "loc": loc,
        "issue": issue,
        "basis": basis,
        "fix": fix,
    })


def read(path):
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            return f.read()
    except Exception:
        return ""


def lines_of(path):
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            return f.read().split("\n")
    except Exception:
        return []


def iter_c_files():
    for p in sorted(glob.glob(os.path.join(SRC, "*.c")) + glob.glob(os.path.join(SRC, "*.h"))):
        yield p


# ─────────────────────────────────────────────────────────────
# P1: 危险 C 函数 / 无界操作
# ─────────────────────────────────────────────────────────────
UNSAFE_FUNCS = [
    (r"\bstrcpy\s*\(", "strcpy", "无界拷贝，目标可能溢出"),
    (r"\bstrcat\s*\(", "strcat", "无界拼接，目标可能溢出"),
    (r"\bsprintf\s*\(", "sprintf", "无界格式化输出，写入栈/全局固定缓冲"),
    (r"\bgets\s*\(", "gets", "经典栈溢出函数"),
    (r"\bvsprintf\s*\(", "vsprintf", "无界格式化输出"),
    (r"\bstrncpy\s*\(", "strncpy", "可能不补 \\0（需核对）"),
    (r"\bmemcpy\s*\(", "memcpy", "需核对长度来源"),
    (r"\bmemmove\s*\(", "memmove", "需核对长度与重叠"),
    (r"\bmemset\s*\(", "memset", "需核对长度"),
    (r"\balloca\s*\(", "alloca", "栈动态分配，长度不可控"),
    (r"\bstrtok\s*\(", "strtok", "非线程安全 + 不可重入"),
]
NO_BOUND_LIST = [
    "snprintf", "strncpy", "memcpy", "memmove", "memset", "strncat", "fread", "fwrite",
]


def probe_unsafe_funcs():
    for p in iter_c_files():
        rel = os.path.relpath(p, ROOT)
        for i, ln in enumerate(lines_of(p), 1):
            st = ln.strip()
            if (st.startswith("//") or st.startswith("*") or st.startswith("/*")
                    or st.endswith("*/") or "/*" in ln):
                continue          # ★ 反噪：注释里提到 strtok/sprintf 的说明文字不算调用
            if "strtok" in ln and "strtok(" not in ln:
                continue          # ★ 反噪：已改 strtok_r 的行不算
            for pat, name, desc in UNSAFE_FUNCS:
                if re.search(pat, ln):
                    has_bound = any(b + "(" in ln for b in NO_BOUND_LIST)
                    sev = "P1" if not has_bound else "P2"
                    add("unsafe-func", sev,
                        f"{name} 使用需复核（{desc}）",
                        f"{rel}:{i}",
                        f"调用 {name}：{ln.strip()[:160]}",
                        f"该行未见边界参数约束（同行无 {'/'.join(NO_BOUND_LIST[:4])} 等受控替代）",
                        "改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界")


def probe_fixed_buf_overflow():
    """固定栈/全局缓冲区 + 无界写入的组合扫描。"""
    for p in iter_c_files():
        rel = os.path.relpath(p, ROOT)
        ls = lines_of(p)
        bufs = []
        for i, ln in enumerate(ls, 1):
            m = re.search(r"\bchar\s+(\w+)\s*\[\s*(\d+)\s*\]", ln)
            if m:
                bufs.append((i, m.group(1), int(m.group(2))))
        for i, ln in enumerate(ls, 1):
            for (bi, bn, bs) in bufs:
                if re.search(r"\b%s\b" % re.escape(bn), ln) and i != bi:
                    if re.search(r"\b(sprintf|strcpy|strcat)\s*\(\s*%s\b" % re.escape(bn), ln):
                        add("buf-overflow", "P1",
                            f"无界写入固定缓冲 {bn}[{bs}]",
                            f"{rel}:{i}",
                            f"对固定大小缓冲 {bn}[{bs}] 使用无界写入：{ln.strip()[:140]}",
                            f"声明于 {rel}:{bi}，大小 {bs} 字节",
                            f"改 snprintf({bn}, sizeof {bn}, ...) 并检查返回值 < sizeof {bn}")
                    elif re.search(r"\b%s\b\s*\[", ln) and re.search(r"\b(strcpy|strcat)\b", ln):
                        add("buf-overflow", "P2",
                            f"疑似越界写 {bn}[{bs}]",
                            f"{rel}:{i}",
                            f"语句涉及 {bn} 与无界拷贝：{ln.strip()[:140]}",
                            f"声明于 {rel}:{bi}，大小 {bs}",
                            "改为受控拷贝")


def _func_bounds(ls, idx_line):
    """找到该行所属函数的起始行与函数体范围。"""
    for j in range(idx_line - 1, max(-1, idx_line - 200), -1):
        m = re.search(r"^[A-Za-z_].*\b(\w+)\s*\([^;]*\)\s*\{?\s*$", ls[j]) or \
            re.search(r"^\s*(?:static\s+)?\w[\w\s\*]*\b(\w+)\s*\([^;{]*\)\s*\{", ls[j])
        if m:
            # 找函数体结束
            depth = 0
            for k in range(j, min(len(ls), j + 400)):
                depth += ls[k].count("{") - ls[k].count("}")
                if depth <= 0 and k > j:
                    return j + 1, k + 1, m.group(1)
            return j + 1, min(len(ls), j + 400), m.group(1)
    return None, None, None


def probe_array_index():
    """数组下标来自函数参数/外部输入且整个函数体内无范围校验。"""
    for p in iter_c_files():
        rel = os.path.relpath(p, ROOT)
        ls = lines_of(p)
        # 收集所有数组声明及其长度
        arrs = {}
        for i, ln in enumerate(ls, 1):
            for m in re.finditer(r"\b(\w+)\s*\[\s*(\d+)\s*\]", ln):
                arrs.setdefault(m.group(1), set()).add(int(m.group(2)))
            for m in re.finditer(r"\b(\w+)\s*\[\s*\]\s*;", ln):
                arrs.setdefault(m.group(1), set()).add(-1)
        for i, ln in enumerate(ls, 1):
            m = re.search(r"\b(\w+)\s*\[\s*([A-Za-z_]\w*)\s*\]", ln)
            if not m:
                continue
            arr, idx = m.group(1), m.group(2)
            if arr not in arrs:
                continue
            if arr.lower() in ("argv", "argc"):
                continue
            if re.search(r"^\s*(/\*|\*|//)", ln):
                continue
            # 找所属函数
            fs, fe, fname = _func_bounds(ls, i)
            if fs is None:
                continue
            body = "\n".join(ls[fs - 1:fe])
            # 整个函数体内是否有对该下标变量的范围校验
            if re.search(r"(if|while|assert)\s*\(?[^{;\n]*\b%s\b\s*(>=|>|<=|<)\s*" % re.escape(idx), body):
                continue
            # ★ 反噪（§2026-11-11）：**计数循环变量**天然有界
            #   （for (size_t i = 0; i < N; i++) → i < N 已保证），
            #   初版未识别 → 776 条误报（EV2WAV[i] / au->pcm[i] 等全是有界循环）。
            if re.search(r"for\s*\([^;]*\b%s\s*=\s*0\s*;[^;]*\b%s\b\s*<" % (re.escape(idx), re.escape(idx)), body):
                continue
            if re.search(r"for\s*\([^;]*\b%s\s*=\s*0\s*;[^;]*\b%s\b\s*<\s*\w+" % (re.escape(idx), re.escape(idx)), body):
                continue
            if re.search(r"\b%s\s*=\s*[^;]*%%\s*\w+" % re.escape(idx), body):
                continue
            if re.search(r"\b%s\b\s*=\s*0\s*;" % re.escape(idx), body) and \
               re.search(r"\bfor\s*\(\s*\w+\s*=\s*0\s*;\s*\w+\s*<", body):
                continue
            # ★ 反噪（2026-11-11）：下标在函数体内**任意位置**有范围守卫（if/assert/continue/return
            #   前置检查）→ 不算越界。旧只查紧邻，守卫写在循环外/函数头会漏。
            if re.search(r"\b%s\s*(>=|>)\s*\d" % re.escape(idx), body) and \
               re.search(r"\b%s\s*(<|<=)\s*" % re.escape(idx), body):
                continue
            # ★ 反噪：下标是**编译期常量/宏边界**循环变量（for i < N_CELLS / RANK_COUNT /
            #   sizeof 等宏）→ 天然有界。
            if re.search(r"for\s*\([^;]*\b%s\s*=[^;]*;[^;]*\b%s\s*<\s*(\w+|\d)" % (re.escape(idx), re.escape(idx)), body) \
               and re.search(r"\b%s\s*(<|<=)\s*(\d|\w+(?:_COUNT|_MAX|_N)?|N_\w+|sizeof)" % re.escape(idx), body):
                continue
            # 下标来源必须是参数或全局外部量
            if not re.search(r"\b%s\b" % re.escape(idx), body.split("\n")[0]) and \
               not re.search(r"\(\s*[^)]*\b%s\b[^)]*\)" % re.escape(idx), body[:600]):
                # 不是参数也不是局部声明来源 → 可能是局部循环变量（低风险）
                if re.search(r"\b(int|size_t|uint\d+_t|char)\s+%s\b" % re.escape(idx), body):
                    continue
            sizes = sorted(arrs[arr])
            add("array-index", "P2",
                f"数组 {arr}[{idx}] 下标在 {fname}() 内无范围校验",
                f"{rel}:{i}",
                f"{arr}[{idx}] 索引，{arr} 声明大小 {sizes}",
                f"函数 {fname}()（{rel}:{fs}-{fe}）体内未对 {idx} 做范围检查",
                f"在索引前加 if ({idx} < 0 || {idx} >= {max(sizes) if max(sizes)>0 else 'N'}) 拒绝并记日志")


def probe_int_overflow():
    for p in iter_c_files():
        rel = os.path.relpath(p, ROOT)
        for i, ln in enumerate(lines_of(p), 1):
            if re.search(r"\b(int|short|uint8_t|int16_t)\b.*=.*\b\w+\s*\*\s*\w+", ln):
                add("int-overflow", "P3",
                    "窄类型乘法可能溢出",
                    f"{rel}:{i}",
                    f"窄类型赋值含乘法：{ln.strip()[:140]}",
                    "未见显式溢出检查",
                    "中间用 int64_t 或加范围断言")
            if re.search(r"\(\s*int\s*\)\s*\w+\s*[-+]\s*\w+", ln):
                add("int-overflow", "P3",
                    "指针差值强转 int 可能溢出/截断",
                    f"{rel}:{i}",
                    f"强转表达式：{ln.strip()[:140]}",
                    "大数组下 int 可能为负",
                    "改用 ptrdiff_t/size_t 并校验非负")


def probe_missing_null_check():
    """JNI 返回值未做 NULL 检查即使用（jni_bridge.c 重点）。"""
    jb = os.path.join(SRC, "jni_bridge.c")
    if not os.path.exists(jb):
        return
    ls = lines_of(jb)
    jni_calls = re.compile(r"\(\s*env\s*\)\s*->\s*(Get|Call|Find|New|GetObjectClass|GetStatic)\w*\s*\(")
    for i, ln in enumerate(ls, 1):
        m = re.search(r"(\w+)\s*=\s*\(\s*env\s*\)\s*->\s*(Get\w*|FindClass|NewStringUTF|GetStringUTFChars|NewObject\w*|Call\w*Method\w*)\s*\(", ln)
        if not m:
            continue
        var, call = m.group(1), m.group(2)
        if call in ("GetStringUTFChars",):
            continue
        # 往后看 6 行是否有 null 检查
        window = "\n".join(ls[i-1:i+6])
        if re.search(r"(if\s*\(\s*%s\s*==\s*NULL|if\s*\(\s*!%s\s*\)|%s\s*==\s*NULL|!\s*%s\s*\)" % (re.escape(var),) * 4, window):
            continue
        add("jni-null", "P1",
            f"JNI {call} 返回值 {var} 未见 NULL 防御",
            f"src_rs/jni_bridge.c:{i}",
            f"{var} = env->{call}(...) 后未见 NULL 检查：{ln.strip()[:140]}",
            f"往后 6 行内未匹配到 {var} 的 NULL 判空",
            f"在 {var} 使用前加 if (!{var}) return <错误路径>;")


def probe_localref_leak():
    """局部引用未释放扫描（粗粒度：成对性）。"""
    jb = os.path.join(SRC, "jni_bridge.c")
    if not os.path.exists(jb):
        return
    ls = lines_of(jb)
    created = []
    for i, ln in enumerate(ls, 1):
        for m in re.finditer(r"(\w+)\s*=\s*\(\s*env\s*\)\s*->\s*(NewStringUTF|NewObject\w*|GetObjectClass|GetStaticObject\w*|GetStringUTFChars|CallObjectMethod\w*)\s*\(", ln):
            created.append((i, m.group(1), m.group(2)))
    for (ci, var, call) in created:
        # 查后续是否有 DeleteLocalRef(var)
        tail = "\n".join(ls[ci-1:ci+120])
        if re.search(r"DeleteLocalRef\s*\(\s*%s\s*\)" % re.escape(var), tail):
            continue
        if call == "GetStringUTFChars":
            continue
        add("jni-leak", "P2",
            f"局部引用 {var}（{call}）未见 DeleteLocalRef",
            f"src_rs/jni_bridge.c:{ci}",
            f"{var} 由 {call} 创建，后续未见对应 DeleteLocalRef",
            "120 行窗口内未匹配到释放",
            f"在使用完毕后加 (*env)->DeleteLocalRef(env, {var});")


def probe_thread_safety():
    """锁纪律：g_state 读写是否在 g_mutex 内。"""
    jb = os.path.join(SRC, "jni_bridge.c")
    if not os.path.exists(jb):
        return
    ls = lines_of(jb)
    # ★ 反噪（§2026-11-11）：初版按**文件顺序**猜锁区间 → 误报 5 条
    #   （jq_restart_game 的注释明确写「调用方需已持有 g_mutex（OnUpdate/OnTouch 锁内；
    #   OnCreate 单线程无需锁）」→ 函数体本身不加锁是正确的）。
    #   现改为**数据流感知**：先解析每个函数的 doc comment，若声明「调用方需持锁」
    #   则跳过其函数体内的 g_state 写入；同时只报**无任何锁上下文**的写入。
    CALLER_LOCKS = ("需已持有 g_mutex", "调用方需", "持锁", "调用者需")
    # ★ 反噪（§2026-11-11 续）：旧 doc_buf 逐行判「startswith /* 或 *」→ 块注释续行
    #   不以 * 开头（如「   调用方需已持有 g_mutex（...）」带缩进）即被清空，
    #   读不到持锁声明 → jq_restart_game 的 g_state 写入误报。现改**真块注释状态机**
    #   （与 probeA_ai_leak 的 _in_block_comment 同范式，文件级 in_comment 追踪）。
    func_doc = {}          # 行号 -> (函数名, 是否声明「调用方持锁」)
    # ① 单次前扫（O(n)）建立每行「是否处于 /*...*/ 块注释内」布尔表，
    #   再收集「块注释内出现持锁关键词」的行集合。旧 O(n²) 逐行重放 → 改单次遍历。
    in_block = [False] * (len(ls) + 1)   # 1-based：in_block[i] = 第 i 行是否全/部分在块注释内
    in_c = False
    for i, ln in enumerate(ls, 1):
        m = 0
        line_any = False
        while m < len(ln):
            if in_c:
                if ln.startswith("*/", m):
                    in_c = False; m += 2; continue
                line_any = True; m += 1
            else:
                if ln.startswith("/*", m):
                    in_c = True; m += 2; line_any = True; continue
                m += 1
        in_block[i] = line_any or in_c
    locked_doc_lines = set()
    for i in range(1, len(ls) + 1):
        if in_block[i] and any(kw in ls[i - 1] for kw in CALLER_LOCKS):
            locked_doc_lines.add(i)
    # ② 函数定义行 → 标记「调用方持锁」= 该函数 doc 区（定义行前 1 起向上 12 行内）
    #   任一行是块注释内的持锁声明。
    #   ★ 反噪（2026-11-11）：fm 旧正则把**函数调用行**（jq_load_layout(&g_state..) /
    #     if (dark) / else 后的调用）误判为函数定义 → g_state 写入向上找到"最近的伪函数"
    #     （lock_decl=False）遮蔽真正的持锁定义（jq_restart_game:252）→ 误报。
    #   现收紧为**真正的定义**：行尾带 { 或紧跟下一行 {，且以返回类型开头（不含 '(' 开头
    #   的调用形态），并排除控制流/宏/调用行。
    func_lock = {}
    for i, ln in enumerate(ls, 1):
        st = ln.lstrip()
        # 排除控制流语句、宏调用、前缀运算符
        if st.startswith(("if", "while", "for", "switch", "return", "else", "do",
                          "}", ";", "(", ",", "//", "/*", "*", "case")):
            continue
        # 定义形态：static <type> name(...) {  或  <type> name(...)  尾带 {
        mdef = re.match(r"^(?:static\s+)?(?:[A-Za-z_][\w]*(?:\s+|\*)+)\b([A-Za-z_]\w*)\s*\([^)]*\)\s*(\{|$)", ln)
        if not mdef:
            continue
        if "(" in st and st.split("(")[0].lstrip().startswith(("jq_", "ui_", "sfx_", "LOGI", "LOGE")):
            # 形如 "jq_xxx(a); " 的调用行（行内有分号结尾或单语句）排除
            pass
        fdef = i
        lock_decl = any(j in locked_doc_lines for j in range(max(1, fdef - 12), fdef))
        func_lock[fdef] = lock_decl
    for i, ln in enumerate(ls, 1):
        st = ln.strip()
        if "g_state" in ln and "=" in ln and not st.startswith("/*"):
            # ★ 反噪（2026-11-11）：真写判据收紧——排除只读表达式（ternary 右值 / 条件 /
            #   比较里的 g_state.<field> 读，如 "g_state.turn == SIDE_BLUE ? ..." / "if (g_state.x==y||...)")。
            #   只保留真正的 g_state.<field> = 赋值（行内有 g_state.X 后紧跟 [^=] = ，或行内以 g_state 开头的写语句）。
            if not re.search(r"g_state\.\w+\s*=(?!=)", ln):
                continue
            # 找到本行所属函数（向上最近的函数定义）
            owner = None
            caller_locks = False
            for j in range(i - 1, max(0, i - 120), -1):
                if j in func_lock:
                    owner = "fn"
                    caller_locks = func_lock[j]
                    break
            if owner and caller_locks:
                continue    # 声明了「调用方持锁」→ 函数体内不加锁是正确设计
            # ★ 反噪：锁检测同时往前看 80 行（lock 常写在 g_state 写语句的**前一行**，
            #   旧版只往后看 80 行漏掉「先 lock 后写」范式 → 误报）。
            body = "\n".join(ls[max(0, i - 81):i + 80])
            if "pthread_mutex_lock(&g_mutex)" in body:
                continue
                # AUD-A-0042（P3 真修）：旧版此处多传 1 个 "lock-discipline" 实参（8 参 vs
                # add() 7 参）→ 探针运行时 TypeError 被 main() try/except 捕获记为
                # "探针自身异常" 条目。删末尾多余实参修复。
                add("lock-discipline", "P2",
                    f"{owner or '?'}() 内 g_state 写入未见加锁",
                    f"src_rs/jni_bridge.c:{i}",
                    f"锁外写 g_state：{ln.strip()[:140]}",
                    "该函数既未自行加锁，也未声明「调用方持锁」",
                    "加锁或补「调用方需已持有 g_mutex」注释声明")


def probe_recursive_lock():
    """非递归 mutex 下嵌套加锁死锁风险。"""
    jb = os.path.join(SRC, "jni_bridge.c")
    if not os.path.exists(jb):
        return
    ls = lines_of(jb)
    depth = 0
    for i, ln in enumerate(ls, 1):
        # ★ 反噪（2026-11-11）：跳过纯注释行（行内 lock/unlock 字样出现在 // 或 /* 注释里
        #   不算真实加锁）——取行尾/行内注释前的代码部分再判。
        code = ln
        ci = code.find("//")
        if ci >= 0:
            code = code[:ci]
        if "pthread_mutex_lock(&g_mutex)" in code:
            depth += 1
            if depth > 1:
                add("deadlock", "P0",
                    "g_mutex 非递归却嵌套加锁 → 死锁",
                    f"src_rs/jni_bridge.c:{i}",
                    "同一调用路径第二次 pthread_mutex_lock(&g_mutex)",
                    "g_mutex 由 pthread_mutex_init(&g_mutex, NULL) 创建（默认非递归）",
                    "把内层加锁改为不加锁（外层已持锁），或改用 PTHREAD_MUTEX_RECURSIVE")
        if "pthread_mutex_unlock(&g_mutex)" in code:
            depth = max(0, depth - 1)


def probe_signal_unsafe():
    """信号处理器里的非异步安全调用。"""
    for p in iter_c_files():
        rel = os.path.relpath(p, ROOT)
        ls = lines_of(p)
        in_handler = False
        for i, ln in enumerate(ls, 1):
            if re.search(r"\w*sig_?handler\s*\(", ln) and "{" in ln:
                in_handler = True
                start = i
                continue
            if in_handler:
                for fn in ("printf", "malloc", "free", "LOGI", "LOGE", "snprintf", "pthread_", "fopen", "fwrite"):
                    if re.search(r"\b%s\b" % re.escape(fn), ln):
                        add("signal-safety", "P1",
                            f"信号处理器内调用非异步安全函数 {fn}",
                            f"{rel}:{i}",
                            f"{ln.strip()[:140]}",
                            "信号处理器只能调用 async-signal-safe 函数",
                            "改用 write(2) 到固定 fd 或设置 volatile 标志位由主流程处理")
                if ln.strip().startswith("}"):
                    in_handler = False


def probe_log_in_early_file():
    """LOGI 宏在文件顶部使用但宏定义在下方（已发生过的坑）。"""
    for p in iter_c_files():
        rel = os.path.relpath(p, ROOT)
        ls = lines_of(p)
        macro_line = None
        for i, ln in enumerate(ls, 1):
            if re.match(r"\s*#\s*define\s+(LOGI|LOGE|LOGD)\b", ln):
                macro_line = i
                break
        if macro_line is None:
            continue
        for i, ln in enumerate(ls, 1):
            if i < macro_line and re.search(r"\b(LOGI|LOGE|LOGD)\s*\(", ln):
                add("macro-order", "P1",
                    f"{rel}:{i} 使用 LOG 宏但宏定义在第 {macro_line} 行（之后）",
                    f"{rel}:{i}",
                    f"{ln.strip()[:140]}",
                    f"宏定义位于 {rel}:{macro_line}，此处更早 → 展开为未定义标识符 → 链接失败（U LOGI）",
                    "把宏定义移到文件顶部，或改用 __android_log_print")


def probe_todo_fixme():
    for p in iter_c_files():
        rel = os.path.relpath(p, ROOT)
        for i, ln in enumerate(lines_of(p), 1):
            for kw in ("TODO", "FIXME", "XXX", "HACK", "暂未", "未实现", "占位"):
                if kw in ln:
                    add("todo", "P3",
                        f"未完成标记（{kw}）",
                        f"{rel}:{i}",
                        ln.strip()[:160],
                        "源码内显式未完成标记，可能被误当完成",
                        "补实现或在交付说明中列为已知边界")
                    break


def probe_magic_numbers():
    """弹窗/坐标魔法数：原版 XML 有权威值，代码里手写数字需核对。"""
    um = os.path.join(SRC, "ui_menu.h")
    if not os.path.exists(um):
        return
    txt = read(um)
    for m in re.finditer(r"#define\s+UI_D(\d{3})_POPUP_(X0|Y0|W|H)\s+([\d.]+)f", txt):
        n, kind, val = m.group(1), m.group(2), float(m.group(3))
        if kind == "W" and val <= 0:
            add("magic", "P1", f"UI_D{n} 面板宽度非正", f"src_rs/ui_menu.h", f"W={val}", "面板宽必须 > 0", "核对原版 XML rect")
        if kind == "H" and val <= 0:
            add("magic", "P1", f"UI_D{n} 面板高度非正", f"src_rs/ui_menu.h", f"H={val}", "面板高必须 > 0", "核对原版 XML rect")
        if kind == "X0" and (val < 0 or val > 720):
            add("magic", "P2", f"UI_D{n} 面板 X0 越界", f"src_rs/ui_menu.h", f"X0={val}", "720 基准下 X0 应在 [0,720)", "核对原版 XML rect + 居中偏移")
        if kind == "Y0" and (val < 0 or val > 1280):
            add("magic", "P2", f"UI_D{n} 面板 Y0 越界", f"src_rs/ui_menu.h", f"Y0={val}", "1280 基准下 Y0 应在 [0,1280)", "核对原版 XML rect + 居中偏移")
    # 面板超出屏幕
    panels = {}
    for m in re.finditer(r"#define\s+UI_D(\d{3})_POPUP_(X0|Y0|W|H)\s+([\d.]+)f", txt):
        panels.setdefault(m.group(1), {})[m.group(2)] = float(m.group(3))
    for n, d in panels.items():
        if all(k in d for k in ("X0", "Y0", "W", "H")):
            if d["X0"] + d["W"] > 720.5:
                add("magic", "P2", f"UI_D{n} 面板右边界超出 720", "src_rs/ui_menu.h",
                    f"X0={d['X0']} + W={d['W']} = {d['X0']+d['W']}", "超出屏幕右界", "核对居中偏移计算")
            if d["Y0"] + d["H"] > 1280.5:
                add("magic", "P2", f"UI_D{n} 面板下边界超出 1280", "src_rs/ui_menu.h",
                    f"Y0={d['Y0']} + H={d['H']} = {d['Y0']+d['H']}", "超出屏幕下界", "核对居中偏移计算")


def probe_button_out_of_panel():
    """按钮 rect 超出所属面板范围。

    ★ 反噪（§2026-11-11）：原版本身存在「按钮比面板大」的排版
      （datah_019 面板 450×150 但按钮局部 y=25..160 → 绝对 455..590 超面板底 580），
      我方如实还原原版坐标 → 探针误报为 bug。
      改为：**同时**核对原版 XML 绝对坐标 —— 只有「我方绝对坐标 ≠ 原版绝对坐标」
      才是真问题（原版如此 → 记 P3 说明，不算缺陷）。
    """
    um = os.path.join(SRC, "ui_menu.h")
    umc = os.path.join(SRC, "ui_menu.c")
    if not (os.path.exists(um) and os.path.exists(umc)):
        return
    ht = read(um)
    ct = read(umc)
    panels = {}
    for m in re.finditer(r"#define\s+UI_D(\d{3})_POPUP_(X0|Y0|W|H)\s+([\d.]+)f", ht):
        panels.setdefault(m.group(1), {})[m.group(2)] = float(m.group(3))
    dlgs = _load_spak_dialogs()
    for n, p in sorted(panels.items()):
        if not all(k in p for k in ("X0", "Y0", "W", "H")):
            continue
        m = re.search(r"g_d%s_btns\[UI_D%s_COUNT\]\s*=\s*\{(.*?)\n\};" % (n, n), ct, re.S)
        if not m:
            continue
        ours = re.findall(r"\{\s*([\d.]+)f\s*,\s*([\d.]+)f\s*,\s*([\d.]+)f\s*,\s*([\d.]+)f\s*,",
                          m.group(1))
        px0, py0 = p["X0"], p["Y0"]
        px1, py1 = p["X0"] + p["W"], p["Y0"] + p["H"]
        d = dlgs.get(n)
        d_btns = (d.get("buttons") if d else None) or []
        exp = _expected_geom(d) if d else None
        for i, o in enumerate(ours):
            x0, y0, x1, y1 = (float(v) for v in o)
            if x1 <= x0 or y1 <= y0:
                add("ui-geom", "P1", f"UI_D{n} 按钮 rect 退化（宽或高非正）",
                    "src_rs/ui_menu.c", f"rect=({x0},{y0})-({x1},{y1})",
                    "宽高必须为正", "核对原版 XML rect")
                continue
            if x0 < px0 - 0.5 or x1 > px1 + 0.5 or y0 < py0 - 0.5 or y1 > py1 + 0.5:
                orig_ok = False
                if i < len(d_btns) and exp:
                    b = d_btns[i]
                    ox, oy = exp[0], exp[1]
                    if abs((ox + b["x0"]) - x0) <= 1 and abs((oy + b["y0"]) - y0) <= 1 \
                       and abs((ox + b["x1"]) - x1) <= 1 and abs((oy + b["y1"]) - y1) <= 1:
                        orig_ok = True
                if orig_ok:
                    add("ui-geom", "P3",
                        f"UI_D{n} 按钮超出面板（原版排版如此，已如实还原）",
                        "src_rs/ui_menu.c",
                        f"按钮 ({x0:.0f},{y0:.0f})-({x1:.0f},{y1:.0f}) 面板 ({px0:.0f},{py0:.0f})-({px1:.0f},{py1:.0f})",
                        "与原版 XML 绝对坐标一致 → 非我方缺陷",
                        "无需修（原版 UI 本身如此）")
                else:
                    add("ui-geom", "P1",
                        f"UI_D{n} 按钮超出面板且与原版不符",
                        "src_rs/ui_menu.c",
                        f"按钮 ({x0:.0f},{y0:.0f})-({x1:.0f},{y1:.0f}) 面板 ({px0:.0f},{py0:.0f})-({px1:.0f},{py1:.0f})",
                        "与原版绝对坐标不一致 → 我方换算有误",
                        "核对 panel_origin 与原版 rect")
            if x0 < 0 or y0 < 0 or x1 > 720 or y1 > 1280:
                add("ui-geom", "P2",
                    f"UI_D{n} 按钮超出 720×1280 屏幕",
                    "src_rs/ui_menu.c",
                    f"rect=({x0},{y0})-({x1},{y1})",
                    "坐标应在屏幕范围内",
                    "核对换算")


def probe_count_mismatch():
    """COUNT 宏与实际按钮数不一致。"""
    um = os.path.join(SRC, "ui_menu.h")
    umc = os.path.join(SRC, "ui_menu.c")
    if not (os.path.exists(um) and os.path.exists(umc)):
        return
    ht = read(um)
    ct = read(umc)
    counts = dict(re.findall(r"#define\s+UI_D(\d{3})_COUNT\s+(\d+)", ht))
    for n, c in counts.items():
        m = re.search(r"g_d%s_btns\[UI_D%s_COUNT\]\s*=\s*\{(.*?)\n\};" % (n, n), ct, re.S)
        if not m:
            add("count-mismatch", "P2",
                f"UI_D{n} 按钮表未找到（COUNT={c}）",
                "src_rs/ui_menu.c",
                f"声明了 UI_D{n}_COUNT={c} 但找不到 g_d{n}_btns 定义",
                "定义缺失 → 编译失败或链接失败",
                "重新生成或补定义")
            continue
        body = m.group(1)
        actual = len(re.findall(r"\{\s*[\d.]+f\s*,", body))
        if actual != int(c):
            add("count-mismatch", "P1",
                f"UI_D{n} COUNT 宏({c}) 与实际按钮数({actual}) 不一致",
                "src_rs/ui_menu.c",
                f"#define UI_D{n}_COUNT {c}，但表内 {actual} 项",
                "不一致 → 命中检测越界读/漏检",
                f"把 COUNT 改为 {actual}（或补齐按钮）")


def probe_render_button_mismatch():
    """render.c 的绘制按钮数 vs ui_menu.c 的按钮表。"""
    rc = os.path.join(SRC, "render.c")
    umc = os.path.join(SRC, "ui_menu.c")
    if not (os.path.exists(rc) and os.path.exists(umc)):
        return
    rl = lines_of(rc)
    ul = lines_of(umc)
    for i, ln in enumerate(rl, 1):
        m = re.search(r"rs_draw_d(\d{3})\b", ln)
        if not m:
            continue
        n = m.group(1)
        # 找 ui_menu.c 的表
        mm = re.search(r"g_d%s_btns\[UI_D%s_COUNT\]\s*=\s*\{(.*?)\n\};" % (n, n), "\n".join(ul), re.S)
        if not mm:
            continue
        cnt = len(re.findall(r"\{\s*[\d.]+f\s*,", mm.group(1)))
        # 绘制函数内循环上界
        fn_start = None
        for j in range(i - 1, max(0, i - 40), -1):
            if re.search(r"static void rs_draw_d%s\s*\(" % n, rl[j]):
                fn_start = j
                break
        if fn_start is None:
            continue
        body = "\n".join(rl[fn_start:fn_start + 120])
        loops = re.findall(r"for\s*\(\s*int\s+\w+\s*=\s*0\s*;\s*\w+\s*<\s*UI_D%s_COUNT" % n, body)
        hard = re.findall(r"for\s*\(\s*int\s+\w+\s*=\s*0\s*;\s*\w+\s*<\s*(\d+)\s*;", body)
        if hard:
            for h in hard:
                if int(h) != cnt:
                    add("render-mismatch", "P2",
                        f"rs_draw_d{n} 硬编码循环上界 {h} ≠ 实际按钮数 {cnt}",
                        f"src_rs/render.c:{fn_start+1}",
                        f"绘制循环用常量 {h}，按钮表有 {cnt} 项",
                        "常量与表不一致 → 多画或少画按钮",
                        "改用 UI_D%s_COUNT" % n)
        if not loops and not hard:
            add("render-mismatch", "P2",
                f"rs_draw_d{n} 未见按钮遍历循环",
                f"src_rs/render.c:{fn_start+1}",
                "绘制函数内未见 for 遍历按钮表",
                "可能按钮未绘制",
                "补 UI_D%s_COUNT 遍历" % n)


def probe_evidence_hook_table():
    """setprop 钩子表与 UI_D 枚举的一致性。"""
    jb = os.path.join(SRC, "jni_bridge.c")
    um = os.path.join(SRC, "ui_menu.h")
    if not (os.path.exists(jb) and os.path.exists(um)):
        return
    ht = read(um)
    jt = read(jb)
    enums = set(re.findall(r"\bUI_D(\d{3})\b", ht))
    hooks = set(re.findall(r"\{\s*\"jq\.dlg(\d{3})\"", jt))
    missing = sorted(enums - hooks)
    for n in missing:
        add("hook-gap", "P2",
            f"UI_D{n} 无 setprop 测试钩子",
            "src_rs/jni_bridge.c",
            f"枚举存在 UI_D{n} 但钩子表无 jq.dlg{n}",
            "缺钩子 → 无法在真机触发验证该弹窗",
            f"在钩子表加 {{\"jq.dlg{n}\", UI_D{n}}}")
    extra = sorted(hooks - enums)
    for n in extra:
        add("hook-gap", "P1",
            f"钩子表引用不存在的 UI_D{n}",
            "src_rs/jni_bridge.c",
            f"钩子表含 jq.dlg{n} → UI_D{n}，但 ui_menu.h 无该枚举",
            "未定义标识符 → 编译失败",
            "补枚举或删钩子")


def probe_evverify_coverage():
    """evverify 是否覆盖所有 UI_D 弹窗。"""
    ev = os.path.join(SRC, "evverify.c")
    um = os.path.join(SRC, "ui_menu.h")
    if not (os.path.exists(ev) and os.path.exists(um)):
        return
    et = read(ev)
    ht = read(um)
    enums = sorted(set(re.findall(r"\bUI_D(\d{3})\b", ht)))
    runs = set(re.findall(r"run_d(\d{3})", et))
    miss = [n for n in enums if n not in runs]
    for n in miss:
        add("test-gap", "P2",
            f"UI_D{n} 无 evverify 桌面断言",
            "src_rs/evverify.c",
            f"ui_menu.h 有 UI_D{n}，evverify.c 无 run_d{n}",
            "缺回归覆盖 → 改动无保护",
            f"补 run_d{n} 断言")


def probe_consistency_baseline():
    """一致性基准是否覆盖最新弹窗与 UI 状态。"""
    cs = os.path.join(SRC, "consistency_desktop.c")
    if not os.path.exists(cs):
        return
    txt = read(cs)
    for kw in ("UI_MENU", "UI_GAME", "replay", "undo", "restart"):
        if kw not in txt:
            add("test-gap", "P3",
                f"一致性基准未覆盖 {kw} 路径",
                "src_rs/consistency_desktop.c",
                f"未提及 {kw}",
                "基准序列覆盖不足",
                "补对应场景")


def probe_layout_check_sync():
    """layout_align_check.py 的 DIALOGS 条目 vs 实际弹窗数。"""
    lac = os.path.join(TOOLS, "layout_align_check.py")
    if not os.path.exists(lac):
        return
    txt = read(lac)
    entries = set(re.findall(r"datah_(\d{3})", txt))
    um = os.path.join(SRC, "ui_menu.h")
    if not os.path.exists(um):
        return
    enums = set(re.findall(r"\bUI_D(\d{3})\b", read(um)))
    miss = sorted(enums - entries)
    for n in miss:
        add("test-gap", "P2",
            f"UI_D{n} 缺 layout_align_check 基线条目",
            "tools/layout_align_check.py",
            f"ui_menu.h 有 UI_D{n}，对齐检查无 datah_{n} 条目",
            "几何 Δ=0 判据未覆盖该弹窗",
            f"补 datah_{n} 基准 rect")


def probe_dist_artifacts():
    """构建产物与源码同步性（so 是否比源码旧）。"""
    so = os.path.join(SRC, "dist", "libjunqi_rs.so")
    if not os.path.exists(so):
        add("artifact", "P1", "libjunqi_rs.so 缺失", "src_rs/dist", "产物不存在", "无法验证/装机", "执行 make so")
        return
    so_mt = os.path.getmtime(so)
    newest_src = 0
    newest_file = None
    # ★ 反噪：只有**参与 so 链接**的源文件才影响产物时效；
    #   桌面工具（main.c/walkverify.c/evverify.c 等）不进 so → 不比较。
    # ★ 反噪（2026-11-11）：Makefile so 目标链接的是 ui_menu.c/ui_state.c（真实表），
    #   ui_menu_stub.c/ui_state_stub.c/render_stub.c 只进桌面目标（jq_evverify 等）
    #   不进 so → 改 stub 不动 so mtime，不应报"so 比源码旧"假红。
    SO_SRC_EXCLUDE = ("main.c", "walkverify.c", "evverify.c", "consistency_desktop.c",
                      "ai_selftest.c", "bt_selftest.c", "lay_selftest.c",
                      "touch_selftest.c", "game_selftest.c", "gl2_probe_gl.c",
                      "ui_menu_stub.c", "ui_state_stub.c", "render_stub.c",
                      "rule_probe.c")
    for p in iter_c_files():
        if os.path.basename(p) in SO_SRC_EXCLUDE:
            continue
        m = os.path.getmtime(p)
        if m > newest_src:
            newest_src = m
            newest_file = p
    if newest_src > so_mt + 1:
        add("artifact", "P1",
            "so 产物比源码旧（未重新编译）",
            "src_rs/dist/libjunqi_rs.so",
            f"源码最新修改 {os.path.relpath(newest_file, ROOT)}，so 早于它",
            "so mtime < 源码 mtime → 验证结果不可信",
            "重新 make so 后再跑回归")


def probe_link_audit():
    """UND 符号审计：是否存在未解析的自定义符号。"""
    nm = "/tmp/opencode/android-ndk-r25b/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-nm"
    so = os.path.join(SRC, "dist", "libjunqi_rs.so")
    if not (os.path.exists(nm) and os.path.exists(so)):
        return
    try:
        out = subprocess.run([nm, "-u", so], capture_output=True, text=True, timeout=60).stdout
    except Exception:
        return
    allowed_prefix = ("U __", "U _", "U printf", "U malloc", "U free", "U memset",
                      "U memcpy", "U str", "U snprintf", "U sprintf", "U log", "U pthread",
                      "U abort", "U exit", "U raise", "U signal", "U sig", "U open", "U close",
                      "U read", "U write", "U fopen", "U fclose", "U fread", "U fwrite",
                      "U atoi", "U atol", "U qsort", "U rand", "U srand", "U time",
                      "U clock", "U getpid", "U system", "U dl", "U __system", "U getenv")
    for ln in out.split("\n"):
        ln = ln.strip()
        if not ln:
            continue
        sym = ln.split()[-1]
        if sym.startswith("LOGI") or sym.startswith("LOGE") or sym.startswith("LOGD"):
            add("link", "P0",
                f"so 存在未定义 LOG 宏符号 {sym}",
                "src_rs/dist/libjunqi_rs.so",
                f"llvm-nm -u 输出 {sym}：宏定义顺序错误 → dlopen failed: failed to link",
                "宏未展开为函数调用",
                "宏定义移至文件顶部，或改用 __android_log_print")
        if sym.startswith(("jq_", "ui_", "rs_", "bt_")) and not sym.startswith("__"):
            add("link", "P0",
                f"so 存在未定义内部符号 {sym}",
                "src_rs/dist/libjunqi_rs.so",
                f"llvm-nm -u 输出 {sym}",
                "内部符号未定义 → 链接/加载失败",
                "检查定义是否被条件编译排除")


def probe_apk_artifacts():
    """阶段交付物 APK 存在性（★ 注意：产物在**仓库根** dist/，不在 rev64/dist/）。"""
    # ★ 反噪（§2026-11-11）：初版只查 rev64/dist/ → 4 个阶段全报「缺失」（假阳性）
    dists = [os.path.join(ROOT, "..", "dist"), os.path.join(ROOT, "dist")]
    for ph in range(4):
        apk = next((os.path.join(d, f"phase{ph}.apk") for d in dists
                    if os.path.exists(os.path.join(d, f"phase{ph}.apk"))), None)
        if not apk:
            add("artifact", "P1",
                f"阶段 {ph} 交付物缺失",
                "dist/phase{0}.apk".format(ph),
                "在 rev64/dist/ 与 仓库根 dist/ 均未找到",
                "阶段验收判据要求产物存在",
                "执行 bash rev64/tools/build_phases.sh all")
            continue
        add("artifact", "P3",
            f"阶段 {ph} 交付物存在",
            os.path.relpath(apk, os.path.join(ROOT, "..")),
            f"大小 {os.path.getsize(apk)} 字节",
            "需校验 STORE/签名/ABI",
            "跑 build_phases.sh --verify-only")


def probe_spak_coverage():
    """原版 datah 覆盖率 + 缺失清单。"""
    if not os.path.isdir(SPAK):
        return
    files = sorted(glob.glob(os.path.join(SPAK, "datah_*.bin")))
    um = os.path.join(SRC, "ui_menu.h")
    if not os.path.exists(um):
        return
    enums = set(re.findall(r"\bUI_D(\d{3})\b", read(um)))
    have = set()
    for f in files:
        m = re.match(r"datah_(\d{3})_", os.path.basename(f))
        if m:
            have.add(m.group(1))
    missing = sorted(have - enums)
    for n in missing:
        add("coverage", "P2",
            f"原版 datah_{n} 未生成对应 UI_D{n}",
            "rev64/forward/src_rs/ui_menu.h",
            f"spak_out 有 datah_{n}.bin 但无 UI_D{n}",
            "原版该对话框未对齐",
            f"用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）")
    orphan = sorted(enums - have)
    for n in orphan:
        add("coverage", "P1",
            f"UI_D{n} 无对应原版 datah_{n}",
            "rev64/forward/src_rs/ui_menu.h",
            f"枚举存在但 spak_out 无 datah_{n}.bin",
            "凭空生成 → 无原版依据",
            "核对来源或删除")


# ─────────────────────────────────────────────────────────────
# 高价值探针：原版几何 Δ 自动核对（权威来源 = 原版 XML）
# ─────────────────────────────────────────────────────────────
def _load_spak_dialogs():
    sys.path.insert(0, TOOLS)
    try:
        from spak2ui import parse_dialog
    except Exception:
        return {}
    out = {}
    for f in sorted(glob.glob(os.path.join(SPAK, "datah_*.bin"))):
        m = re.match(r"datah_(\d{3})_", os.path.basename(f))
        if not m:
            continue
        try:
            out[m.group(1)] = parse_dialog(f)
        except Exception:
            continue
    return out


def _expected_geom(d):
    """按 spak2ui.panel_origin 语义重算原版 720 基准几何（单一真相源）。"""
    if not d.get("panel"):
        return None
    sys.path.insert(0, TOOLS)
    try:
        from spak2ui import panel_origin
    except Exception:
        return None
    ox, oy = panel_origin(d)
    w, h = d["panel"]
    return ox, oy, w, h


def probe_geom_delta():
    """面板几何 Δ：ui_menu.h 的 POPUP_* vs 原版 XML 重算值。"""
    um = os.path.join(SRC, "ui_menu.h")
    if not os.path.exists(um):
        return
    dlgs = _load_spak_dialogs()
    if not dlgs:
        return
    ht = read(um)
    panels = {}
    for m in re.finditer(r"#define\s+UI_D(\d{3})_POPUP_(X0|Y0|W|H)\s+([\d.]+)f", ht):
        panels.setdefault(m.group(1), {})[m.group(2)] = float(m.group(3))
    for n, p in sorted(panels.items()):
        d = dlgs.get(n)
        if not d:
            continue
        exp = _expected_geom(d)
        if not exp:
            continue
        ox, oy, w, h = exp
        got = (p.get("X0"), p.get("Y0"), p.get("W"), p.get("H"))
        if None in got:
            continue
        deltas = {
            "X0": got[0] - ox, "Y0": got[1] - oy,
            "W": got[2] - w, "H": got[3] - h,
        }
        bad = {k: v for k, v in deltas.items() if abs(v) > 0.51}
        if bad:
            desc = ", ".join(f"{k} Δ={v:+.1f}" for k, v in bad.items())
            add("geom-delta", "P1",
                f"UI_D{n} 面板几何与原版 Δ≠0",
                "src_rs/ui_menu.h",
                f"我方 X0/Y0/W/H = {got}，原版重算 = ({ox:.1f},{oy:.1f},{w:.1f},{h:.1f})；{desc}",
                f"原版 datah_{n} rect={d.get('rect')} centered={d.get('centered')}；"
                f"判定阈 |Δ|>0.5",
                "按原版 rect + 居中偏移重算并修正 POPUP_* 宏")
        elif any(abs(v) > 0 for v in deltas.values()):
            desc = ", ".join(f"{k} Δ={v:+.2f}" for k, v in deltas.items())
            add("geom-delta", "P3",
                f"UI_D{n} 面板几何存在亚像素偏差",
                "src_rs/ui_menu.h",
                f"{desc}",
                "浮点取整差，视觉可接受",
                "可保留（记录为亚像素偏差）")


def probe_button_geom_delta():
    """按钮几何 Δ：ui_menu.c 的按钮表 vs 原版 XML 重算值。"""
    umc = os.path.join(SRC, "ui_menu.c")
    if not os.path.exists(umc):
        return
    dlgs = _load_spak_dialogs()
    if not dlgs:
        return
    ct = read(umc)
    for n, d in sorted(dlgs.items()):
        exp = _expected_geom(d)
        if not exp:
            continue
        ox, oy, w, h = exp
        btns = d.get("buttons") or []
        if not btns:
            continue
        m = re.search(r"g_d%s_btns\[UI_D%s_COUNT\]\s*=\s*\{(.*?)\n\};" % (n, n), ct, re.S)
        if not m:
            continue
        body = m.group(1)
        ours = re.findall(r"\{\s*([\d.]+)f\s*,\s*([\d.]+)f\s*,\s*([\d.]+)f\s*,\s*([\d.]+)f\s*,", body)
        if len(ours) != len(btns):
            continue
        # strip 横条型：原版互斥重叠 >30% 只留首个
        keep = []
        for b in btns:
            rect = (b["x0"], b["y0"], b["x1"], b["y1"])
            dup = False
            for kb in keep:
                krect = (kb["x0"], kb["y0"], kb["x1"], kb["y1"])
                ix = max(0, min(rect[2], krect[2]) - max(rect[0], krect[0]))
                iy = max(0, min(rect[3], krect[3]) - max(rect[1], krect[1]))
                inter = ix * iy
                a1 = (rect[2] - rect[0]) * (rect[3] - rect[1])
                a2 = (krect[2] - krect[0]) * (krect[3] - krect[1])
                if a1 > 0 and a2 > 0 and inter / min(a1, a2) > 0.30:
                    dup = True
                    break
            if not dup:
                keep.append(b)
        if len(keep) != len(ours):
            add("geom-delta", "P2",
                f"UI_D{n} 按钮数与原版去重后不一致",
                "src_rs/ui_menu.c",
                f"我方 {len(ours)} 项，原版去重后 {len(keep)} 项（原版 {len(btns)} 项）",
                "strip 横条型按互斥重叠>30% 只留首个",
                "核对去重逻辑或补/减按钮")
            continue
        for idx, (o, kb) in enumerate(zip(ours, keep)):
            ox0, oy0, ox1, oy1 = (float(v) for v in o)
            ex0 = ox + kb["x0"]
            ey0 = oy + kb["y0"]
            ex1 = ox + kb["x1"]
            ey1 = oy + kb["y1"]
            ds = (ox0 - ex0, oy0 - ey0, ox1 - ex1, oy1 - ey1)
            if any(abs(v) > 0.51 for v in ds):
                desc = ", ".join(f"{k} Δ={v:+.1f}" for k, v in
                                 zip(("x0", "y0", "x1", "y1"), ds) if abs(v) > 0.51)
                add("geom-delta", "P1",
                    f"UI_D{n} 按钮#{idx}（{kb.get('name')}）几何 Δ≠0",
                    "src_rs/ui_menu.c",
                    f"我方 ({ox0:.0f},{oy0:.0f})-({ox1:.0f},{oy1:.0f})，"
                    f"原版 ({ex0:.0f},{ey0:.0f})-({ex1:.0f},{ey1:.0f})；{desc}",
                    f"原版 datah_{n} 局部 rect=({kb['x0']},{kb['y0']})-({kb['x1']},{kb['y1']})，"
                    f"面板偏移=({ox:.1f},{oy:.1f})",
                    "按原版局部 rect + 面板偏移重算按钮坐标")


def probe_button_text_delta():
    """按钮文案对比：原版 XML text vs 我方按钮表注释（注释 = 渲染文字来源）。

    ★ 反噪（§2026-11-11）：
      初版（a）`cur` 状态永不重置 → 跨弹窗串行误匹配（把别处的 /* Rename */ 当本弹窗）
      （b）取整文件首个注释而非当前行
      → 77 条全是误报（实际按钮表注释就是原版中文）。
      现改为：只在 `g_dNNN_btns[...]{ ... };` **块内**逐行取注释比对。
    """
    umc = os.path.join(SRC, "ui_menu.c")
    if not os.path.exists(umc):
        return
    dlgs = _load_spak_dialogs()
    if not dlgs:
        return
    ls = lines_of(umc)
    i = 0
    while i < len(ls):
        m = re.search(r"g_d(\d{3})_btns\[UI_D\d{3}_COUNT\]\s*=\s*\{", ls[i])
        if not m:
            i += 1
            continue
        n = m.group(1)
        d = dlgs.get(n)
        if not d:
            i += 1
            continue
        btns = d.get("buttons") or []
        # 收集块内按钮行（含行尾注释）
        rows = []
        j = i
        while j < len(ls) and "};" not in ls[j]:
            ln = ls[j]
            mm = re.search(r"\{\s*([\d.]+)f\s*,.*?\}\s*,\s*(?:/\*\s*(.*?)\s*\*/)?", ln)
            if mm:
                rows.append((j + 1, (mm.group(2) or "").strip()))
            j += 1
        if len(rows) == len(btns):
            for (ln_no, cmt), b in zip(rows, btns):
                t = (b.get("text") or "").strip()
                if not t:
                    continue
                # 注释里可能带补充说明（"改名 Rename（旧名+数字后缀）"）→ 只要含原版文案即通过
                if t not in cmt:
                    add("text-delta", "P1",
                        f"UI_D{n} 按钮（{b.get('name')}）渲染文案与原版不一致",
                        f"src_rs/ui_menu.c:{ln_no}",
                        f"我方注释「{cmt}」未含原版 text「{t}」",
                        "按钮表注释即渲染文字来源（原版 text 为权威）",
                        f"改用原版文案「{t}」", "text-delta")
        i = j + 1


def probe_jni_signature():
    """JNI 入口签名 vs App.smali 的 native 声明。"""
    jb = os.path.join(SRC, "jni_bridge.c")
    smali_roots = glob.glob(os.path.join(ROOT, "..", "**", "smali", "com", "cnvcs", "App.smali"),
                            recursive=True)
    if not os.path.exists(jb) or not smali_roots:
        return
    sm = read(smali_roots[0])
    decl = {}
    for m in re.finditer(r"native\s+(\w+)\s+(\w+)\s*\(([^)]*)\)", sm):
        decl[m.group(2)] = (m.group(1), m.group(3).strip())
    jt = read(jb)
    for m in re.finditer(r"Java_com_cnvcs_App_(On\w+)\s*\(\s*JNIEnv\s*\*\s*\w+\s*,\s*jclass\s+\w+\s*\)", jt):
        name = "Java_com_cnvcs_App_" + m.group(1)
        if m.group(1) not in decl:
            add("jni-sig", "P1",
                f"JNI 入口 {m.group(1)} 在 App.smali 无 native 声明",
                "src_rs/jni_bridge.c",
                f"native {name} 未在 smali 中声明",
                "原版 smali 是签名权威 → 多余入口不会被调用",
                "核对是否多余，或补 smali 声明（注意红线：不得改 native 签名）")
    for nm, (rt, args) in decl.items():
        if nm.startswith("On") and f"Java_com_cnvcs_App_{nm}" not in jt:
            add("jni-sig", "P0",
                f"App.smali 声明的 native {nm} 在 native so 中缺失",
                "smali/com/cnvcs/App.smali",
                f"native {nm}({args}) 无对应 JNIEXPORT 实现",
                "缺失 → 调用即 UnsatisfiedLinkError → app 崩溃",
                f"补 Java_com_cnvcs_App_{nm} 实现")


def probe_compile_warnings():
    """全量编译警告扫描（-Wall）。"""
    mk = os.path.join(SRC, "Makefile")
    so = os.path.join(SRC, "dist", "libjunqi_rs.so")
    if not os.path.exists(mk):
        return
    ndk = "/tmp/opencode/android-ndk-r25b"
    if not os.path.isdir(ndk):
        return
    try:
        out = subprocess.run(["make", "-C", SRC, "so", f"NDK={ndk}"],
                             capture_output=True, text=True, timeout=900,
                             env=dict(os.environ, EXTRA_CFLAGS="-Wall -Wextra -Wno-unused-parameter"))
        txt = out.stdout + out.stderr
    except Exception as e:
        add("compile-warn", "P3", "编译警告扫描失败", "src_rs", str(e)[:150], "扫描未完成", "手动编译")
        return
    for m in re.finditer(r"^(\S+\.c):(\d+):(\d+):\s*warning:\s*(.*)$", txt, re.M):
        add("compile-warn", "P2",
            f"编译警告：{m.group(4)[:80]}",
            f"src_rs/{m.group(1)}:{m.group(2)}",
            m.group(4)[:200],
            "编译器诊断（-Wall -Wextra）",
            "按警告修（多为隐式转换/未使用/格式串）")


def probe_spell_magic_coord():
    """坐标硬编码与原版 rect 的一致性抽查（POPUP 宏 vs 按钮表推算面板范围）。"""
    umc = os.path.join(SRC, "ui_menu.c")
    um = os.path.join(SRC, "ui_menu.h")
    if not (os.path.exists(umc) and os.path.exists(um)):
        return
    ct = read(umc)
    ht = read(um)
    panels = {}
    for m in re.finditer(r"#define\s+UI_D(\d{3})_POPUP_(X0|Y0|W|H)\s+([\d.]+)f", ht):
        panels.setdefault(m.group(1), {})[m.group(2)] = float(m.group(3))
    for n, p in sorted(panels.items()):
        if not all(k in p for k in ("X0", "Y0", "W", "H")):
            continue
        m = re.search(r"g_d%s_btns\[UI_D%s_COUNT\]\s*=\s*\{(.*?)\n\};" % (n, n), ct, re.S)
        if not m:
            continue
        ours = re.findall(r"\{\s*([\d.]+)f\s*,\s*([\d.]+)f\s*,\s*([\d.]+)f\s*,\s*([\d.]+)f\s*,",
                          m.group(1))
        if not ours:
            continue
        bx0 = min(float(o[0]) for o in ours)
        by0 = min(float(o[1]) for o in ours)
        bx1 = max(float(o[2]) for o in ours)
        by1 = max(float(o[3]) for o in ours)
        # 按钮组应落在面板内（或面板是背景，按钮可溢出一点点）
        if bx0 < p["X0"] - 1.0 or by0 < p["Y0"] - 1.0:
            add("geom-delta", "P2",
                f"UI_D{n} 按钮组起点超出面板",
                "src_rs/ui_menu.c",
                f"按钮最小 ({bx0:.0f},{by0:.0f}) < 面板 ({p['X0']:.0f},{p['Y0']:.0f})",
                "局部 1:1 平移语义 → 按钮应 ≥ 面板原点",
                "核对居中偏移或按钮坐标")
        if bx1 > p["X0"] + p["W"] + 1.0 or by1 > p["Y0"] + p["H"] + 1.0:
            add("geom-delta", "P2",
                f"UI_D{n} 按钮组终点超出面板",
                "src_rs/ui_menu.c",
                f"按钮最大 ({bx1:.0f},{by1:.0f}) > 面板右下 ({p['X0']+p['W']:.0f},{p['Y0']+p['H']:.0f})",
                "同上",
                "核对居中偏移或按钮坐标")


PROBES_EXTRA = [
    ("geom-delta", probe_geom_delta),
    ("geom-delta2", probe_button_geom_delta),
    ("geom-delta3", probe_spell_magic_coord),
    ("text-delta", probe_button_text_delta),
    ("jni-sig", probe_jni_signature),
    ("compile-warn", probe_compile_warnings),
]


def probe_manifest_ads():
    """阶段 3 声明与实际资源一致性。"""
    for p in glob.glob(os.path.join(ROOT, "..", "**", "AndroidManifest.xml"), recursive=True):
        try:
            if os.path.getsize(p) > 2_000_000:
                continue
            txt = read(p)
        except Exception:
            continue
        for kw in ("com.baidu.mobads", "UMENG_APPKEY", "__xadsdk__"):
            if kw in txt:
                add("ads", "P1",
                    f"manifest 残留广告项 {kw}",
                    os.path.relpath(p, ROOT),
                    f"manifest 中出现 {kw}",
                    "阶段 3 要求去广告干净",
                    "删除该条目")


def probe_pak_do_not_compress():
    for p in glob.glob(os.path.join(ROOT, "..", "**", "*.pak"), recursive=True):
        add("redline", "P2",
            "pak 资源需保持 doNotCompress",
            os.path.relpath(p, ROOT),
            "打包时必须 STORE",
            "红线：动 doNotCompress 会破坏资源加载",
            "打包脚本白名单包含 *.pak")


def probe_claim_vs_code():
    """Jenkins 声明 vs 代码实现。"""
    jb = os.path.join(SRC, "jni_bridge.c")
    if not os.path.exists(jb):
        return
    txt = read(jb)
    for claim, code in [
        ("14 个音效", "sfx_for_move"),
        ("蓝牙 v2 7 字段", "bt_send_frame"),
        ("悔棋连续 pop", "undo_pop"),
        ("明棋开局", "jq_restart_game"),
    ]:
        if claim and code not in txt:
            add("claim", "P2",
                f"声明「{claim}」的实现符号 {code} 不存在",
                "src_rs/jni_bridge.c",
                f"找不到 {code}",
                "声明与实现不符",
                "补实现或修正文档声明")


def probe_git_state():
    """git 未提交改动（潜在半成品）。"""
    try:
        out = subprocess.run(["git", "status", "--porcelain", "rev64/"],
                             cwd=ROOT, capture_output=True, text=True, timeout=60).stdout
    except Exception:
        return
    n = len([l for l in out.split("\n") if l.strip()])
    if n:
        add("git", "P3",
            f"rev64/ 有 {n} 个未提交改动",
            "rev64/",
            "工作区非干净",
            "未提交内容可能半成品",
            "提交或回滚")


def probe_pull_request_push_state():
    try:
        out = subprocess.run(["git", "status", "-sb"], cwd=ROOT,
                             capture_output=True, text=True, timeout=60).stdout
    except Exception:
        return
    if "ahead" in out:
        add("git", "P3",
            "本地提交未推送到 origin/main",
            "仓库",
            out.strip().split("\n")[0],
            "远端缺最新提交",
            "git push origin main")


PROBES = [
    ("unsafe-func", probe_unsafe_funcs),
    ("buf-overflow", probe_fixed_buf_overflow),
    ("array-index", probe_array_index),
    ("int-overflow", probe_int_overflow),
    ("jni-null", probe_missing_null_check),
    ("jni-leak", probe_localref_leak),
    ("lock-discipline", probe_thread_safety),
    ("deadlock", probe_recursive_lock),
    ("signal-safety", probe_signal_unsafe),
    ("macro-order", probe_log_in_early_file),
    ("todo", probe_todo_fixme),
    ("magic", probe_magic_numbers),
    ("ui-geom", probe_button_out_of_panel),
    ("count-mismatch", probe_count_mismatch),
    ("render-mismatch", probe_render_button_mismatch),
    ("hook-gap", probe_evidence_hook_table),
    ("test-gap", probe_evverify_coverage),
    ("test-gap2", probe_layout_check_sync),
    ("test-gap3", probe_consistency_baseline),
    ("artifact", probe_dist_artifacts),
    ("artifact2", probe_apk_artifacts),
    ("link", probe_link_audit),
    ("coverage", probe_spak_coverage),
    ("ads", probe_manifest_ads),
    ("redline", probe_pak_do_not_compress),
    ("claim", probe_claim_vs_code),
    ("git", probe_git_state),
    ("git2", probe_pull_request_push_state),
] + PROBES_EXTRA


def main():
    out = os.path.join(ROOT, "audit", "r2", "AUD_AUTO.md")
    only = None
    args = sys.argv[1:]
    if "--only" in args:
        only = args[args.index("--only") + 1].split(",")
    if "--out" in args:
        out = args[args.index("--out") + 1]
        if not os.path.isabs(out):
            out = os.path.join(ROOT, out)

    for name, fn in PROBES:
        if only and name not in only:
            continue
        try:
            fn()
        except Exception as e:
            # AUD-A-0042（P3 探针自身异常）：add() 收 7 位（probe/sev/title/loc/issue/basis/fix），
            # 旧版此处误传 8 个实参（str(e)[:200] 与 "探针实现有 bug" 多拆 1 个）
            # → TypeError 又被 try/except 再捕获递归进 add 死循环。归并 issue/basis 修复。
            add(name, "P3", f"探针自身异常：{type(e).__name__}",
                "audit/probe_audit.py", f"{str(e)[:200]}（探针实现有 bug）", "修探针", "修探针")

    sev_order = {"P0": 0, "P1": 1, "P2": 2, "P3": 3}
    FINDINGS.sort(key=lambda f: (sev_order.get(f["sev"], 9), f["probe"]))

    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "w", encoding="utf-8") as f:
        f.write("# 自动化对抗审计探针（机器穷举）\n\n")
        f.write(f"> 生成器：`audit/probe_audit.py`　条目数：**{len(FINDINGS)}**\n")
        f.write("> 姊妹文档：子代理深度审计（rules / jni-render / ui-geom / build-pkg / doc-truth / rev32）\n\n")
        f.write("## 统计\n\n")
        f.write("| 严重级 | 条数 |\n|---|---|\n")
        for s in ("P0", "P1", "P2", "P3"):
            f.write(f"| {s} | {sum(1 for x in FINDINGS if x['sev']==s)} |\n")
        f.write("\n## 按探针分组\n\n| 探针 | 条数 |\n|---|---|\n")
        probes = {}
        for x in FINDINGS:
            probes.setdefault(x["probe"], []).append(x)
        for k in sorted(probes, key=lambda k: -len(probes[k])):
            f.write(f"| {k} | {len(probes[k])} |\n")
        f.write("\n---\n\n")
        for idx, x in enumerate(FINDINGS, 1):
            f.write(f"## AUD-A-{idx:04d} [{x['sev']}] {x['title']}\n")
            f.write(f"- 探针：`{x['probe']}`\n")
            f.write(f"- 位置：{x['loc']}\n")
            f.write(f"- 问题：{x['issue']}\n")
            f.write(f"- 依据：{x['basis']}\n")
            f.write(f"- 修法：{x['fix']}\n\n")
    print(f"[probe_audit] {len(FINDINGS)} findings → {out}")
    for s in ("P0", "P1", "P2", "P3"):
        print(f"  {s}: {sum(1 for x in FINDINGS if x['sev']==s)}")


if __name__ == "__main__":
    main()
