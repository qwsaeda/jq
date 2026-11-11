#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-207 全钩子 4 项判据锚点对照脚本（静态侧，不依赖设备）

固化"全钩子 4 项判据锚点"（§124y-178 全钩子基准）的可复跑对照判据（防判据漂移）：
- game_test     判据锚点 = "GAME-TEST: 端到端完成 走子=OK" + "touch_int: tap1k(274000,1268000) → cell6"
                 + "EVT-TBL-DUMP.*非0字节=129"（事件掩码表 289B 忠实语义）
- walk_bridge   判据锚点 = "WALK-BRIDGE: 完整 8 步序列完成"（§124y-204 三源对照）
- native_event  判据锚点 = "F_3ab80: 事件发送"（.bss 事件表 asr4 恒 0 → disc=0 忠实兜底）
- framebuffer   判据锚点 = 出屏非黑比 ≥30%（§124y-179 防 0% 假象，dump 文件权威）

判据输出：
1. 翻译体源码（jni_aligned.c / jq_rs_walkbridge.c / jqt_vtbl.c）含全部判据锚点字符串
2. 判据锚点与全钩子脚本（arm32_hook_check.sh / arm32_walkregress.sh）一致（防串改漂移）

用法：python3 rev64/tools/hook_anchors_check.py
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC_JNI = os.path.join(ROOT, "rev64/reverse/src_jni_translated/jni_aligned.c")
SRC_WALK = os.path.join(ROOT, "rev64/reverse/src_jni_translated/jq_rs_walkbridge.c")
SRC_VTBL = os.path.join(ROOT, "rev64/reverse/src_jni_translated/jqt_vtbl.c")
HOOK = os.path.join(ROOT, "rev64/tools/arm32_hook_check.sh")
REGRESS = os.path.join(ROOT, "rev64/tools/arm32_walkregress.sh")

# 判据锚点 → 所在源码文件
ANCHORS = {
    # ★ 坑固化：源码 GAME-TEST 日志 = "走子=%s"（成功填 "OK(2步Move)"），脚本判据
    #   "走子=OK" = 前缀子串匹配。锚点核源码实际格式 "GAME-TEST: 端到端完成" +
    #   "走子="（变量段），防判据串改漂移。
    "GAME-TEST: 端到端完成": [SRC_JNI],
    "走子=": [SRC_JNI],
    "touch_int: tap1k": [SRC_WALK],
    "EVT-TBL-DUMP": [SRC_JNI],
    "WALK-BRIDGE: 完整 8 步序列完成": [SRC_WALK],
    "F_3ab80": [SRC_VTBL, SRC_JNI],
}

def main():
    files = {}
    for p in (SRC_JNI, SRC_WALK, SRC_VTBL, HOOK, REGRESS):
        if not os.path.exists(p):
            print("✗ %s 不存在" % p); return 2
        files[p] = open(p).read()
    print("═══ §124y-207 全钩子 4 项判据锚点对照脚本 ═══")
    ok = True
    for anchor, srcs in ANCHORS.items():
        # 锚点在指定源码文件至少 1 处命中
        hit_src = any(anchor in files[s] for s in srcs if s in files)
        # 锚点在判据脚本（hook_check/walkregress）出现
        hit_hook = anchor in files.get(HOOK, "") or anchor in files.get(REGRESS, "")
        mark = "✓" if (hit_src and hit_hook) else ("~" if (hit_src or hit_hook) else "✗")
        if not (hit_src and hit_hook): ok = False
        print("  %s %-44s 源码=%s 脚本=%s" % (mark, anchor, hit_src, hit_hook))
    print("  framebuffer 出屏非黑比 ≥30% = dump 文件权威判读（§124y-179，无字符串锚点，运行时判据）")
    print("  判据: %s" % ("PASS（4 项判据锚点 源码+脚本双源一致，防判据漂移）" if ok else "FAIL（锚点缺失/漂移）"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
