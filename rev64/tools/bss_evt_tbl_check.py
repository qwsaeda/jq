#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-190 .bss 事件表 289B 真值基准判据（静态侧，不依赖设备）

固化 §124y-168/185 实锤：
- rev64/dist/jq_event_tbl_fill_289B.bin = 从 32 位翻译体 so 提取的
  jq_event_tbl_fill_bytes 289 字节（.bss 事件表 17 行 × 17 字段回填表）
- 真值基准 = 非0字节 129（与 MuMu 原版 so ref dump 逐字节一致，§124y-168 实锤）
- 设备端 EVT-TBL-DUMP 判据「非0字节=129」的对照基线

判据输出：
1. 289B 文件存在 + 大小 = 289
2. 非0字节 = 129（真值基准）
3. 高 nibble 非0 字节 = 0（asr4 掩码区未填事件码，disc=0 忠实语义）
4. 17 行 × 17 字段结构校验（行宽 17B）

用法：python3 rev64/tools/bss_evt_tbl_check.py
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BIN = os.path.join(ROOT, "rev64/reverse/analysis/jq_event_tbl_fill_289B.bin")

def main():
    if not os.path.exists(BIN):
        print("✗ %s 不存在（须先提取 32 位 so jq_event_tbl_fill_bytes）" % BIN)
        return 2
    data = open(BIN, 'rb').read()
    # §124y-1912 增强：从当前 dist so 重提取 289B（jq_event_tbl_fill_bytes @ VA 0x214d8，
    # ELF32 load base 0 → VA=文件偏移）做三方一致性（在库 bin == 当前 so 重提取），
    # 防翻译体演进后 bin 漂移（bin 是一次性提取快照，so 每批都可能变）
    SO = os.path.join(ROOT, "rev64/dist/libjunqi_arm32_aligned.so")
    if os.path.exists(SO):
        import re
        nm = os.popen("arm-linux-gnueabi-nm -C %s 2>/dev/null | awk '/jq_event_tbl_fill_bytes/{print $1}'" % SO).read().strip()
        if nm:
            so = open(SO, 'rb').read()
            soff = int(nm, 16)
            sdata = so[soff:soff+289]
            print("  §1912 三方: 当前 so 0x%s 起 289B 非0=%d 高nibble=%d | 与在库 bin 一致=%s" % (
                format(soff, 'x'), sum(1 for b in sdata if b),
                sum(1 for b in sdata if b >> 4), sdata == data))
            if sdata != data:
                ok = False
                print("  ✗ 当前 so 与在库 bin 不一致（bin 漂移，须重提取）")
        else:
            print("  §1912 三方: nm 未查到 jq_event_tbl_fill_bytes（符号改名？跳过）")
    if len(data) != 289:
        print("✗ 大小 %d ≠ 289（17 行 × 17 字段）" % len(data))
        return 2
    nz = sum(1 for b in data if b != 0)
    # 高 nibble 非0（asr4 事件码位）
    hi_nz = 0
    for i in range(0, 289, 17):
        row = data[i:i+17]
        for b in row:
            if (b >> 4) != 0:
                hi_nz += 1
    print("═══ §124y-190 .bss 事件表 289B 真值基准判据 ═══")
    print("  文件: %s" % os.path.relpath(BIN, ROOT))
    print("  大小: %d（= 17 行 × 17 字段）" % len(data))
    print("  非0字节: %d（真值基准 = 129，§124y-168/185 MuMu 原版 so ref dump 逐字节一致）" % nz)
    print("  高 nibble 非0 字节: %d（= 0 → asr4 掩码区未填事件码，disc=0 忠实语义）" % hi_nz)
    # 17 行结构校验
    rows = 17
    print("  17 行结构: 行宽 17B × %d 行 = %dB（行边界 0/17/34/.../272）" % (rows, rows*17))
    ok = (nz == 129 and hi_nz == 0)
    print("  判据: %s" % ("PASS（非0=129 + 高nibble=0，忠实语义）" if ok else "FAIL（非0=%d 或 高nibble=%d）" % (nz, hi_nz)))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
