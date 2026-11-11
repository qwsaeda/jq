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
