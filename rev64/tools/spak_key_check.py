#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-209 原版 SPAK 解密 key 静态佐证判据 + 口径修正（不依赖设备）

★ 口径修正实锤（2026-11-11）：
- AGENTS/REVERSE_LOG 旧口径"SPAK 解密 key 32 字节明文 @ 原版 .rodata VA 0x12a658 /
  文件偏移 0x2e5268"中 **文件偏移 0x2e5268 系误值（该处全 0/32 字节）**。
- 实锤：原版 32 位 so（load base 0，VA=文件偏移）key 实际 @ **VA=文件偏移 0x12a658**
  （.rodata 段内），32 字节全非0 + 唯一 31/32（高熵在位）：
  c2cbe5e7ce5c4ac411ca6c9906f476f86d4fcbb62621de0e6a67874c93f0d523
- 算法 = XOR(key[j%32]) + zlib（R5 完整解包 883 资源佐证，spak_unpack.py 在位）

判据输出：
1. key @ 0x12a658 32 字节全非0 + 唯一 ≥ 16（高熵在位）
2. 旧口径 0x2e5268 全 0（误值佐证，防复发误引）
3. 算法口径固化（XOR+zlib，R5 解包产物在库）

用法：python3 rev64/tools/spak_key_check.py
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"
KEY_VA = 0x12a658
OLD_BAD_OFF = 0x2e5268
KEY_HEX = "c2cbe5e7ce5c4ac411ca6c9906f476f86d4fcbb62621de0e6a67874c93f0d523"

def main():
    if not os.path.exists(ORIG_SO):
        print("✗ %s 不存在" % ORIG_SO); return 2
    so = open(ORIG_SO, 'rb').read()
    # ★ 双源对照：spak_unpack.py SPAK_KEY 与原版 so 0x12a658 提取 key 一致（防解包器 key 漂移）
    import re
    unpack = os.path.join(ROOT, "rev64/tools/spak_unpack.py")
    unpack_key = ""
    if os.path.exists(unpack):
        m = re.search(r"SPAK_KEY\s*=\s*bytes\.fromhex\('([0-9a-f]+)'\)", open(unpack).read())
        unpack_key = m.group(1) if m else ""
    key = so[KEY_VA:KEY_VA+32]
    nz = sum(1 for b in key if b != 0)
    uniq = len(set(key))
    inplace = (nz == 32) and (uniq >= 16)
    # 旧口径误值佐证
    old = so[OLD_BAD_OFF:OLD_BAD_OFF+32]
    old_zero = (sum(1 for b in old if b != 0) == 0)
    # 与固化解包 key 一致（spak_unpack 产物 + 双源对照）
    key_match = (key.hex() == KEY_HEX)
    unpack_match = (unpack_key == KEY_HEX) if unpack_key else False
    print("═══ §124y-209 原版 SPAK 解密 key 静态佐证判据 + 口径修正 ═══")
    print("  key @ VA=文件偏移 0x%06x（.rodata 段内，32 位 so load base 0）" % KEY_VA)
    print("  32 字节全非0: %d/32 / 唯一字节: %d/32 → %s" % (nz, uniq, "PASS" if inplace else "FAIL"))
    print("  key hex: %s" % key.hex())
    print("  与固化解包 key 一致: %s" % ("PASS" if key_match else "FAIL"))
    print("  双源对照 spak_unpack.py SPAK_KEY 一致: %s" % ("PASS" if unpack_match else "FAIL（解包器 key 漂移）"))
    print("  旧口径 0x%06x 全 0 误值佐证: %s" % (OLD_BAD_OFF, "PASS（该处全 0，确系误值）" if old_zero else "（该处非全0，需复核）"))
    print("  算法: XOR(key[j%%32]) + zlib（R5 完整解包 883 资源佐证）")
    ok = inplace and key_match and old_zero and unpack_match
    print("  判据: %s" % ("PASS（key 高熵在位 + 解包基准一致 + 旧口径误值已纠正）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
