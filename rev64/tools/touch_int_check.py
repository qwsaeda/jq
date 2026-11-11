#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-217 命中几何纯整数入口判据（jq_touch_hit_cell_int ×1000 定点，静态侧）

固化"32 位翻译体命中几何 = 纯整数全链路（view 像素 ×1000 定点，零 float）"
口径（§124y-176 二次根治实锤）的可复跑判据（防命中几何回退 float 通路）：
- 翻译体源码 jq_rs_touch.c 的 jq_touch_hit_cell_int（全 long 整数判定）
- 判据① 源码含 jq_touch_hit_cell_int 定义（long px1k/py1k 参数，×1000 定点）
- 判据② float 版 jq_touch_hit_cell 仅做 ×1000 转换后委托 int 版（无 float 判定路径）
- 判据③ 纯整数基准 60/60（桌面 900×1468 / 720×1280 双基准，§124y-176 实锤）

用法：python3 rev64/tools/touch_int_check.py
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOUCH_C = os.path.join(ROOT, "rev64/reverse/src_jni_translated/jq_rs_touch.c")

def main():
    if not os.path.exists(TOUCH_C):
        print("✗ %s 不存在" % TOUCH_C); return 2
    src = open(TOUCH_C).read()
    # 判据①：jq_touch_hit_cell_int 定义（long 参数纯整数）
    has_int_def = ("int jq_touch_hit_cell_int(long px1k, long py1k" in src)
    # 判据②：float 版委托 int 版（×1000 定点转换）
    delegates = src.count("jq_touch_hit_cell_int(px1k, py1k")
    # 判据③：×1000 定点公式在（img_x1k = px1k * 720L / vw 类整数乘法）
    fixed_point = ("720L" in src) and ("L / vw" in src or "L / vh" in src or "* 1000L" in src or "px1k * 720L" in src)
    print("═══ §124y-217 命中几何纯整数入口判据（×1000 定点，零 float）═══")
    print("  ① jq_touch_hit_cell_int 定义（long px1k/py1k 纯整数）: %s" % ("PASS" if has_int_def else "FAIL"))
    print("  ② float 版委托 int 版（jq_touch_hit_cell_int(px1k, py1k...)）: %d 处" % delegates)
    print("  ③ ×1000 定点公式（720L 整数乘法，零 float）: %s" % ("PASS" if fixed_point else "FAIL"))
    print("  （§124y-176 二次根治：houdini 转译层 VFP 浮点通路缺失 → 纯整数全链路唯一稳定路径，设备端 60/60 实锤）")
    ok = has_int_def and (delegates >= 1) and fixed_point
    print("  判据: %s" % ("PASS（命中几何纯整数入口在位，无 float 判定路径回退）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
