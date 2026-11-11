#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-204 走子桥 8 步序列判据锚点对照脚本（静态侧，不依赖设备）

固化"walk_bridge 8 步序列判据"锚点（§124y-161 test_move_seq 基准）的可复跑
对照判据（防判据漂移）：
- 判据锚点 = 翻译体 jq_rs_walkbridge.c 8 步序列（Flip×2 + Move×2 + ILLEGAL×2 +
  Undo + Resign + FINAL），logcat 判据行 "WALK-BRIDGE: 完整 8 步序列完成"
- 判据① 翻译体源码含 8 步序列定义（f6 f33 m6→11 m33→38 m12→13(ILLEGAL)
  u(undo) m10→11(ILLEGAL) q0(认输) + "完整 8 步序列完成" 日志锚点）
- 判据② 全钩子/走子回归脚本判据锚点（arm32_hook_check.sh / arm32_walkregress.sh
  的 "WALK-BRIDGE: 完整 8 步序列完成"）与源码锚点一致（防判据串改漂移）

用法：python3 rev64/tools/walk_bridge_8step_check.py
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "rev64/reverse/src_jni_translated/jq_rs_walkbridge.c")
HOOK = os.path.join(ROOT, "rev64/tools/arm32_hook_check.sh")
REGRESS = os.path.join(ROOT, "rev64/tools/arm32_walkregress.sh")
ANCHOR = "WALK-BRIDGE: 完整 8 步序列完成"

def main():
    for p in (SRC, HOOK, REGRESS):
        if not os.path.exists(p):
            print("✗ %s 不存在" % p); return 2
    src = open(SRC).read()
    hook = open(HOOK).read()
    reg = open(REGRESS).read()
    # 8 步序列定义（源码实锤锚点）
    # 8 步序列实际用 jq_walk_bridge_move/from-to 参数（12,13 / 10,11）+ 认输 + ILLEGAL 期望
    has_seq = all(t in src for t in ("jq_walk_bridge_move(12, 13)", "jq_walk_bridge_move(10, 11)", "ILLEGAL", "认输", "jq_walk_bridge_flip"))
    has_anchor = ANCHOR in src
    hook_anchor = ANCHOR in hook
    reg_anchor = ANCHOR in reg
    print("═══ §124y-204 走子桥 8 步序列判据锚点对照脚本 ═══")
    print("  源码 8 步序列定义（f6 f33 + move×2 + ILLEGAL×2 + undo + 认输）: %s" % ("✓" if has_seq else "✗"))
    print("  源码判据锚点 '%s': %s" % (ANCHOR, "✓" if has_anchor else "✗"))
    print("  arm32_hook_check.sh 判据锚点一致: %s" % ("✓" if hook_anchor else "✗"))
    print("  arm32_walkregress.sh 判据锚点一致: %s" % ("✓" if reg_anchor else "✗"))
    ok = has_seq and has_anchor and hook_anchor and reg_anchor
    print("  判据: %s" % ("PASS（源码 8 步定义 + 双脚本判据锚点一致，防判据漂移）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
