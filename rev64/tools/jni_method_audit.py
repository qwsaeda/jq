#!/usr/bin/env python3
"""jni_method_audit.py — JNI GetMethodID 异常清理审计（防坑 2 复发）。

背景（AGENTS.md src_rs 音效/AI 闭环 坑 2）：
  每个 (*env)->GetMethodID 失败（NoSuchMethodError）必须 ExceptionClear，否则
  pending exception 累积 → 后续 JNI 调用 ART abort（setDataSource(File) 不存在
  → NoSuchMethodError 未清 → 后续 GetMethodID abort，实证）。

用法：
  python3 jni_method_audit.py <file.c> [--window N]
    window = 检查 GetMethodID 后 N 行内是否有 ExceptionCheck/ExceptionClear（默认 6）

退出码：0 = 无缺口（全部有异常清理）；2 = 有缺口（列出行号）。

注：注释行误报（注释含 GetMethodID 字样）——审计跳过以 ' * ' / '//' / '/*' 开头的
行；三元表达式 `cls ? GetMethodID : NULL` 若后续有集中 ExceptionClear 算通过。
"""
import re
import sys


def audit(path: str, window: int = 8) -> list:
    lines = open(path).read().split('\n')
    bad = []
    in_block_comment = False
    for i, ln in enumerate(lines):
        # 块注释状态检测（/* ... */ 跨行）
        in_comment = in_block_comment
        stripped = ln.lstrip()
        if stripped.startswith('//'):
            in_comment = True
        if '/*' in ln:
            in_block_comment = True
            in_comment = True
        if '*/' in ln:
            in_block_comment = False
        if in_comment:
            continue
        if 'GetMethodID' not in ln:
            continue
        # 后续 window 行内是否有 ExceptionCheck/ExceptionClear
        window_lines = lines[i:i + window]
        if not any('ExceptionCheck' in w or 'ExceptionClear' in w for w in window_lines):
            bad.append((i + 1, ln.strip()[:80]))
    return bad


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    path = sys.argv[1]
    window = 8
    for arg in sys.argv[2:]:
        if arg.startswith('--window='):
            window = int(arg.split('=')[1])
    bad = audit(path, window)
    total = sum(1 for ln in open(path) if 'GetMethodID' in ln
                and not ln.lstrip().startswith(('*', '//', '/*')))
    print(f"[jni_method_audit] {path}: GetMethodID 非注释调用≈{total}，无异常清理 {len(bad)}")
    for b in bad:
        print(f"  line {b[0]}: {b[1]}")
    if bad:
        print("✗ 有缺口：GetMethodID 失败未 ExceptionClear → pending exception 累积 abort")
        return 2
    print("✓ 全部 GetMethodID 有异常清理（坑 2 防复发）")
    return 0


if __name__ == '__main__':
    sys.exit(main())
