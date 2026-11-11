#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
clean_guard.py — 批量生成片段的**精确删除**工具（括号配对，不用非贪婪大块正则）

★ 2026-11-11 事故复盘（gen_dialog_batch.py clean 的第三型误删）：
  旧 clean 正则 `(?:\n/\* ★ 批量对齐（datah_\d{3}）：\d+ 按钮 \+ 原版按钮文字.*?\n\}
  (?=static void rs_draw_about|\n/\* ★ 批量对齐))+` 用**非贪婪**匹配"批量块 + 直到下一个锚点"，
  当批量块后面**不是** rs_draw_about（而是 rs_gl_rebuild 尾部 + rs_render_frame）时，
  匹配会一路吃到下一个 `}` → **误删 rs_render_frame**（帧渲染主函数）→
  so 留下 UND rs_render_frame → Android linker "dlopen failed: failed to link" →
  启动即 UnsatisfiedLinkError（loadLibrary 失败被 smali catch 吞掉）。

  修法：删除必须**按函数名 + 括号配对**精确到函数边界，绝不跨越下一个函数。

用法:
  python3 rev64/tools/clean_guard.py remove_func <file> <func_name> [...]
  python3 rev64/tools/clean_guard.py remove_block <file> <start_marker>   # 到下一个同族 marker 前
"""
import re
import sys


def _match_brace(s, open_idx):
    """返回与 s[open_idx] 处 '{' 配对的 '}' 的下标（跳过字符串/字符/注释）。"""
    assert s[open_idx] == '{'
    depth = 0
    i = open_idx
    n = len(s)
    while i < n:
        c = s[i]
        if c == '"':                      # 字符串
            i += 1
            while i < n:
                if s[i] == '\\':
                    i += 2
                    continue
                if s[i] == '"':
                    break
                i += 1
        elif c == "'":                    # 字符
            i += 1
            while i < n:
                if s[i] == '\\':
                    i += 2
                    continue
                if s[i] == "'":
                    break
                i += 1
        elif c == '/' and i + 1 < n and s[i+1] == '/':   # 行注释
            while i < n and s[i] != '\n':
                i += 1
            continue
        elif c == '/' and i + 1 < n and s[i+1] == '*':   # 块注释
            j = s.find('*/', i + 2)
            i = (j + 2) if j >= 0 else n
            continue
        elif c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def remove_comment_block(path, marker_re):
    """删除包含 marker 的**整块注释**（从 '/*' 到配对 '*/'）。
    ★ 用于替代"只删注释首行"的行级正则——后者会留下半截注释裸露成代码
      （实测：render.c:5557 'unexpected character <U+2605>'）。
    """
    s = open(path, encoding='utf-8').read()
    m = re.search(marker_re, s)
    if not m:
        return 0
    ob = s.rfind('/*', 0, m.end())
    if ob < 0:
        return 0
    cb = s.find('*/', ob + 2)
    if cb < 0:
        return 0
    end = cb + 2
    while end < len(s) and s[end] in '\r\n':
        end += 1
    open(path, 'w', encoding='utf-8').write(s[:ob] + s[end:])
    return 1


def remove_block_by_text(path, anchor_re):
    """删除包含 anchor 文本的**整个语句块**（从 anchor 前的 '{' 起，按括号配对到配对 '}'）。
    ★ 用于替代危险的非贪婪正则（如 `if (...) \\{[\\s\\S]*?\\n    \\}\\n`）——
      后者在配对 '}' 已被其他清理步骤删掉时，会一路吃到后面的任意函数（如 rs_render_frame）。
    """
    s = open(path, encoding='utf-8').read()
    m = re.search(anchor_re, s)
    if not m:
        return 0
    ob = s.find('{', m.end())   # ★ 往**后**找（if 条件内无 '{' → 第一个 '{' 即 body 起点）；
    if ob < 0:                    #   旧版用 rfind（往前）会找到外层 for 的 '{' → 删范围过大 → 代码撕裂
        return 0
    head = s[max(0, ob - 200):ob]
    if ')' not in head[-80:]:
        return 0
    cb = _match_brace(s, ob)
    if cb < 0:
        return 0
    end = cb + 1
    while end < len(s) and s[end] in '\r\n':
        end += 1
    open(path, 'w', encoding='utf-8').write(s[:ob] + s[end:])
    return 1


def remove_func(path, func_name, also_leading_comment=True):
    """删除 `static void <func_name>(...) { ... }` 整函数（含其上方紧邻的注释块）。"""
    s = open(path, encoding='utf-8').read()
    pat = re.compile(rf'(?:^|\n)(?:/\*(?:[^*]|\*(?!/))*\*/\s*\n)*static\s+[\w \*]*\b{re.escape(func_name)}\s*\(')
    m = pat.search(s)
    if not m:
        return 0
    start = m.start()
    if start < s.index(func_name):
        start = m.start() + 1 if s[m.start()] == '\n' else m.start()
    # 定位函数体的 '{'
    ob = s.find('{', m.end())
    if ob < 0:
        return 0
    cb = _match_brace(s, ob)
    if cb < 0:
        return 0
    end = cb + 1
    while end < len(s) and s[end] in '\r\n':
        end += 1
    s2 = s[:start] + s[end:]
    open(path, 'w', encoding='utf-8').write(s2)
    return 1


def remove_block(path, marker):
    """删除从 marker 行开始、到下一个 '/* ★ 批量对齐' 同族 marker 之前的块。"""
    s = open(path, encoding='utf-8').read()
    idx = s.find(marker)
    if idx < 0:
        return 0
    nxt = s.find('/* ★ 批量对齐', idx + len(marker))
    end = nxt if nxt >= 0 else len(s)
    s2 = s[:idx] + s[end:]
    open(path, 'w', encoding='utf-8').write(s2)
    return 1


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        return 2
    cmd = sys.argv[1]
    path = sys.argv[2]
    n = 0
    for name in sys.argv[3:]:
        n += remove_func(path, name)
    print(f"removed {n} func(s) from {path}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
