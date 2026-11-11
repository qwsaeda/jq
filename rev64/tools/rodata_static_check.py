#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-192 原版 .rodata 明文 dump 逐字节一致性判据（静态侧，不依赖设备）

固化"非加壳 so / .rodata 明文无自解密"口径的可复跑判据：
- frida-dump/rodata_dec.bin（118832B）= 运行时 dump 的 .rodata 明文
- 原版 so /tmp/opencode/orig_so/lib/armeabi/libjunqi.so .rodata 段
  （VA 0x120bf8，size 0x1d030 = 118832B）
- 判据：dump 与 so 静态文件 .rodata 段逐字节一致（MD5 相同）
  = 无运行时自解密（.rodata 明文直接可见，非加壳 so 静态指纹）

判据输出：
1. dump 大小 = 118832（= .rodata 段 size 0x1d030）
2. 逐字节一致（MD5 相同）
3. 已知字符串池可见（Junqi.Move/updateUrl/statHost 等在 dump 里可 grep）

用法：python3 rev64/tools/rodata_static_check.py
"""
import hashlib, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DUMP = os.path.join(ROOT, "frida-dump/rodata_dec.bin")
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"
RODATA_VA = 0x120bf8
RODATA_SIZE = 0x1d030  # 118832

def main():
    if not os.path.exists(DUMP):
        print("✗ %s 不存在" % DUMP); return 2
    if not os.path.exists(ORIG_SO):
        print("✗ %s 不存在" % ORIG_SO); return 2
    dec = open(DUMP, 'rb').read()
    so = open(ORIG_SO, 'rb').read()
    rodata = so[RODATA_VA:RODATA_VA+RODATA_SIZE]
    md5_dec = hashlib.md5(dec).hexdigest()
    md5_so = hashlib.md5(rodata).hexdigest()
    same = (dec == rodata)
    # 已知字符串池可见判据
    known = [b"Junqi.Move", b"updateUrl", b"statHost", b"data/config/"]
    vis = [s for s in known if s in dec]
    print("═══ §124y-192 原版 .rodata 明文 dump 逐字节一致性判据 ═══")
    print("  dump: %s（%d 字节）" % (os.path.relpath(DUMP, ROOT), len(dec)))
    print("  so .rodata 段: VA 0x%x size 0x%x（%d 字节）" % (RODATA_VA, RODATA_SIZE, RODATA_SIZE))
    print("  大小一致: %s（dump %d vs .rodata %d）" % (len(dec)==RODATA_SIZE, len(dec), RODATA_SIZE))
    print("  逐字节一致: %s" % ("PASS（MD5 相同）" if same else "FAIL（MD5 不同）"))
    print("  MD5 dump=%s  so=%s" % (md5_dec, md5_so))
    print("  已知字符串池可见: %d/%d %s（Junqi.Move/updateUrl/statHost/data/config/）" % (len(vis), len(known), vis))
    ok = same and len(dec)==RODATA_SIZE and len(vis)>=3
    print("  判据: %s" % ("PASS（非加壳 so / .rodata 明文无自解密口径成立）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
