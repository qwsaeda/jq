#!/usr/bin/env python3
"""ui_color_check.py — 弹窗配色与原版真值逐色校验（防自创色值假对齐）

原版色值来源：uicmp/{difficulty,gameset}_orig_vs_fw.png 左半（原版 datah 渲染）像素统计真值。
校验 render_ui 离线渲染的弹窗按钮/面板像素 == 原版色值（容差 ≤6/通道）。
用法：python3 rev64/tools/ui_color_check.py
"""
import sys
from PIL import Image

OUT = "/tmp/opencode/ui_offline"
# (屏, 采样点, 期望RGB原版真值, 说明)
CHECKS = [
    ("difficulty", (390, 658), (58, 103, 189), "难度布局按钮=原版蓝"),
    ("difficulty", (570, 658), (128, 80, 190), "难度翻棋按钮=原版紫"),
    ("difficulty", (250, 700), (40, 40, 40),    "难度面板=原版深灰"),
    ("gameset", (650, 620), (80, 190, 140),     "游戏设置开关=原版绿"),
    ("gameset", (255, 1028), (58, 103, 189),    "游戏设置确定=原版蓝"),
    ("gameset", (581, 1028), (160, 160, 160),   "游戏设置取消=原版灰"),
    ("gameset", (250, 700), (45, 45, 45),       "游戏设置面板=原版深灰"),
]
TOL = 6


def main():
    failed = 0
    for screen, (x, y), want, label in CHECKS:
        im = Image.open(f"{OUT}/{screen}.png").convert("RGB")
        got = im.load()[x, y]
        ok = all(abs(g - w) <= TOL for g, w in zip(got, want))
        print(f"{'PASS' if ok else 'FAIL'} {label}: got={got} want={want}")
        if not ok:
            failed += 1
    print(f"判定: {'FAIL' if failed else 'PASS'}（{len(CHECKS)-failed}/{len(CHECKS)} 配色对齐原版）")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
