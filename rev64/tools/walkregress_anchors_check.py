#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-219 32 位 4 路径走子回归判据锚点对照脚本（静态侧，不依赖设备）

固化"4 路径走子回归（arm32_walkregress.sh）判据锚点 = 翻译体源码实锤日志
串"口径（§124y-23 三条走子路径 + §124y-26 game_test）的可复跑对照判据
（防判据漂移）：
- walk_bridge   判据锚点 = "WALK-BRIDGE: 完整 8 步序列完成"（jq_rs_walkbridge.c）
- native_layout 判据锚点 = "NATIVE-LAYOUT: F_2a380 返回"（原生布局加载链）
- native_event  判据锚点 = "F_3ab80: 事件发送.*disc="（.bss 事件表 disc=0 忠实兜底）
- game_test     判据锚点 = "GAME-TEST: 端到端完成 走子=OK"（§124y-26 端到端）

判据输出：
1. 4 判据锚点在翻译体源码（jq_rs_walkbridge.c / jqt_vtbl.c / jni_aligned.c）实锤
2. 4 判据锚点与 arm32_walkregress.sh PATTERN 一致（防脚本判据串改漂移）

用法：python3 rev64/tools/walkregress_anchors_check.py
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC_FILES = [
    os.path.join(ROOT, "rev64/reverse/src_jni_translated/jq_rs_walkbridge.c"),
    os.path.join(ROOT, "rev64/reverse/src_jni_translated/jqt_vtbl.c"),
    os.path.join(ROOT, "rev64/reverse/src_jni_translated/jni_aligned.c"),
]
REGRESS = os.path.join(ROOT, "rev64/tools/arm32_walkregress.sh")

ANCHORS = {
    "WALK-BRIDGE: 完整 8 步序列完成": "jq_rs_walkbridge.c",
    "NATIVE-LAYOUT: F_2a380 返回": None,  # 在翻译体任一源码
    "F_3ab80: 事件发送": None,
    "GAME-TEST: 端到端完成": None,
}

def main():
    for p in SRC_FILES + [REGRESS]:
        if not os.path.exists(p):
            print("✗ %s 不存在" % p); return 2
    srcblob = "".join(open(p).read() for p in SRC_FILES)
    reg = open(REGRESS).read()
    print("═══ §124y-219 32 位 4 路径走子回归判据锚点对照脚本 ═══")
    ok = True
    for anchor, expect_file in ANCHORS.items():
        # 锚点在翻译体源码实锤
        in_src = anchor in srcblob
        # 锚点在回归脚本 PATTERN 出现（判据一致）
        in_reg = anchor in reg
        mark = "✓" if (in_src and in_reg) else "✗"
        if not (in_src and in_reg):
            ok = False
        print("  %s %-40s 源码=%s 脚本=%s" % (mark, anchor, in_src, in_reg))
    print("  （native_event 判据 'F_3ab80: 事件发送' 带 .*disc= 正则，本判据核前缀锚点即可）")
    print("  判据: %s" % ("PASS（4 路径判据锚点 源码+脚本双源一致，防判据漂移）" if ok else "FAIL（锚点缺失/漂移）"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
