#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-218 出屏非黑比 ≥30% 判读基准三源对照判据（静态侧，不依赖设备）

固化"出屏判读基准 = 非黑比 ≥30%（§124y-179 判读口径）"的可复跑三源对照判据
（防判读基准漂移）：
- 翻译体源码 jqt_vtbl.c framebuffer dump 块（"帧非黑比=%d%%（<30，纹理未上传，跳过保留槽）"
  锚点 = 30% 阈值在源码）
- 判据脚本 arm32_hook_check.sh（framebuffer 项 `[ "$ratio" -ge 30 ]` = 30% 阈值在判据）
- 出屏判读口径（§124y-179：dump 非黑比 ≥30% 才写文件，防 0% 采样假象）

判据输出：
1. 源码 30% 阈值锚点在（jqt_vtbl.c "<30" 跳过槽判读）
2. 判据脚本 30% 阈值在（arm32_hook_check.sh -ge 30）
3. 双源一致（源码与判据脚本 30% 基准不漂移）

用法：python3 rev64/tools/fb_ratio_check.py
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
VTBL = os.path.join(ROOT, "rev64/reverse/src_jni_translated/jqt_vtbl.c")
HOOK = os.path.join(ROOT, "rev64/tools/arm32_hook_check.sh")

def main():
    for p in (VTBL, HOOK):
        if not os.path.exists(p):
            print("✗ %s 不存在" % p); return 2
    vsrc = open(VTBL).read()
    hsrc = open(HOOK).read()
    src_anchor = ("帧非黑比=" in vsrc) and ("<30" in vsrc)
    hook_anchor = ('-ge 30' in hsrc)
    print("═══ §124y-218 出屏非黑比 ≥30% 判读基准三源对照判据 ═══")
    print("  ① 源码 jqt_vtbl.c 30" + "%" + " 阈值锚点（帧非黑比日志 + <30 跳过槽）: " + ("PASS" if src_anchor else "FAIL"))
    print("  ② 判据脚本 arm32_hook_check.sh 30" + "%" + " 阈值（-ge 30）: " + ("PASS" if hook_anchor else "FAIL"))
    print("  ③ 双源一致（源码与判据脚本 30" + "%" + " 基准不漂移）: " + ("PASS" if (src_anchor and hook_anchor) else "FAIL"))
    print("  （§124y-179 判读口径：dump 非黑比 ≥30% 才写文件，防 0% 采样假象；none% = dump 未生成/轮询超时非出屏未达成）")
    ok = src_anchor and hook_anchor
    print("  判据: " + ("PASS（出屏 30" + "%" + " 判读基准双源一致，防漂移）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
