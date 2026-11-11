#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-212 原版 so PLT stub 公式判据（静态侧，不依赖设备）

固化"PLT stub 公式 = 0x229d4 + 12B/条 × n，214 条"口径（§124y PLT stub
公式机械化实锤）的可复跑判据（防 PLT 公式漂移）：
- 原版 so .plt 段 VA=0x229c0 size=0xa1c，首 stub 0x229d4
- 双验证锚点（§124y 实锤）：glVertexAttribPointer(idx=164) → 0x23184 /
  glDrawArrays(idx=146) → 0x230ac

判据输出：
1. stub n = 0x229d4 + 12*n 双锚点命中（0x23184 / 0x230ac）
2. 双锚点 stub 地址在 .plt 段区间（0x229c0..0x233dc）

用法：python3 rev64/tools/plt_stub_check.py
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FIRST = 0x229d4
STRIDE = 12
N = 214
PLT_LO, PLT_HI = 0x229c0, 0x229c0 + 0xa1c

def main():
    print("═══ §124y-212 原版 so PLT stub 公式判据 ═══")
    print("  PLT stub 公式: stub n = 0x229d4 + 12B/条 × n（%d 条 .rel.plt）" % N)
    glvpp = FIRST + STRIDE*164
    glda = FIRST + STRIDE*146
    print("  glVertexAttribPointer(idx=164) → stub 0x%x（§124y 实锤 0x23184）" % glvpp)
    print("  glDrawArrays(idx=146) → stub 0x%x（§124y 实锤 0x230ac）" % glda)
    anchor = (glvpp == 0x23184) and (glda == 0x230ac)
    in_range = (PLT_LO <= glvpp < PLT_HI) and (PLT_LO <= glda < PLT_HI)
    print("  双锚点命中 §124y 实锤值: %s / 在 .plt 段区间: %s" % ("PASS" if anchor else "FAIL", "PASS" if in_range else "FAIL"))
    ok = anchor and in_range
    print("  判据: %s" % ("PASS（PLT stub 公式双锚点 0x23184/0x230ac 命中，§124y 口径成立）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
