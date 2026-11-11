#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-206 翻译体 so 收表 handler 函数完整度判据（静态侧，不依赖设备）

固化"翻译体 so 含 24 handler 收表函数 + F_55d40 dispatch 区"口径（§124y-29/30/31
收表实锤）的可复跑判据（防翻译体 so 函数漂移）：
- 翻译体 so rev64/dist/libjunqi_arm32_aligned.so nm 内部符号
- 判据① 关键收表 handler 函数在（F_8571c/F_55ba4/F_3deb0/F_853c0/F_3f8f4 +
  扩展区 8 handler F_27a6c/F_953cc/F_91d18/F_91774/F_55d74/F_55fbc/F_9411c/F_33724）
- 判据② F_3ab64/F_3ab80/F_3a92c 事件分发主链函数在（native_event 判据锚点）

用法：python3 rev64/tools/translated_handlers_check.py
"""
import os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TRANS_SO = os.path.join(ROOT, "rev64/dist/libjunqi_arm32_aligned.so")

HANDLERS = ["F_8571c", "F_55ba4", "F_3deb0", "F_853c0", "F_3f8f4",
            "F_27a6c", "F_953cc", "F_91d18", "F_91774",
            "F_55d74", "F_55fbc", "F_9411c", "F_33724"]
MAIN_CHAIN = ["F_3ab64", "F_3ab80", "F_3a92c", "F_55d40"]

def main():
    if not os.path.exists(TRANS_SO):
        print("✗ %s 不存在" % TRANS_SO); return 2
    out = subprocess.run(["nm", TRANS_SO], capture_output=True, text=True).stdout
    have = set()
    for line in out.splitlines():
        parts = line.split()
        if len(parts) >= 3:
            have.add(parts[2])
    miss_h = [h for h in HANDLERS if h not in have]
    miss_m = [m for m in MAIN_CHAIN if m not in have]
    print("═══ §124y-206 翻译体 so 收表 handler 函数完整度判据 ═══")
    print("  收表 handler 13 个: 缺 %d %s" % (len(miss_h), miss_h if miss_h else "（全在）"))
    print("  事件分发主链 4 个（F_3ab64/F_3ab80/F_3a92c/F_55d40）: 缺 %d %s" % (len(miss_m), miss_m if miss_m else "（全在）"))
    ok = not miss_h and not miss_m
    print("  判据: %s" % ("PASS（17 关键函数全在，收表 + 主链完整）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
