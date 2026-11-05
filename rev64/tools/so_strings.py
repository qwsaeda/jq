#!/usr/bin/env python3
"""so_strings.py — 原版 32 位 so 字符串提取（含地址，替代手写 bytes.find 循环）
用法:
  python3 so_strings.py <so路径> [子串过滤]     # 列 .rodata 所有 ≥4 字节字符串（带文件偏移）
  python3 so_strings.py <so路径> address         # 按偏移找（十六进制）
  python3 so_strings.py <so路径> bluetooth       # 按关键词过滤
输出: 偏移 0x%x + 字符串内容（UTF-8 解码，非 UTF-8 显示 \\x 转义）
"""
import sys, re

def all_strings(so, min_len=4):
    """提取 so 全部 NUL 结尾字符串（文件偏移 + 内容）"""
    data = open(so, 'rb').read()
    out = []
    start = None
    for i, b in enumerate(data):
        if b >= 0x20:  # 可打印
            if start is None: start = i
        else:
            if start is not None and i - start >= min_len:
                raw = data[start:i]
                try:
                    s = raw.decode('utf-8')
                    out.append((start, s))
                except UnicodeDecodeError:
                    out.append((start, None))  # 非 UTF-8
            start = None
    return out

def main():
    if not sys.argv[1:]:
        print(__doc__); return 1
    so = sys.argv[1]
    rodata_only = '--rodata' in sys.argv
    args = [a for a in sys.argv[2:] if a != '--rodata']
    if rodata_only:
        # .rodata 明文区（原版 VA==Off：0x120bf8-0x13dc28）
        data = open(so, 'rb').read()
        lo, hi = 0x120bf8, 0x13dc28
        region = data[lo:hi]
        key = args[0].lower() if args else None
        if key and re.fullmatch(r'0x[0-9a-fA-F]+', key):
            off = int(key, 16) - lo
            e = region.find(b'\x00', off)
            s = region[off:e]
            print(f"  {lo+off:#x}: {s!r}")
        else:
            i = 0
            while i < len(region):
                if region[i] >= 0x20:
                    j = i
                    while j < len(region) and region[j] >= 0x20: j += 1
                    if j - i >= 4:
                        raw = region[i:j]
                        try: s = raw.decode('utf-8')
                        except UnicodeDecodeError: s = None
                        if s and (key is None or key in s.lower()):
                            print(f"  {lo+i:#x}: {s!r}")
                    i = j
                else: i += 1
        return 0
    if len(args) >= 1:
        key = args[0]
        if re.fullmatch(r'0x[0-9a-fA-F]+', key):
            off = int(key, 16)
            data = open(so, 'rb').read()
            e = data.find(b'\x00', off)
            s = data[off:e]
            try: print(f"  {off:#x}: {s.decode('utf-8')!r}")
            except UnicodeDecodeError: print(f"  {off:#x}: {s!r}")
            return 0
        # 关键词过滤
        for off, s in all_strings(so):
            if s and key.lower() in s.lower():
                print(f"  {off:#x}: {s!r}")
    else:
        for off, s in all_strings(so):
            if s is None: continue  # 只打 UTF-8
            print(f"  {off:#010x}: {s}")
    return 0

if __name__ == '__main__':
    sys.exit(main())
