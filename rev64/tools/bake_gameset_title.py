#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""烘焙游戏设置标题黑字版（原版 Title color=black）。

gameset_text_atlas.png（220×414，9 行竖排白字）的 row0 是标题'游戏设置'。
原版 datah_012 XML：Title style="TransLabel" color="black"——标题黑字。
正向 gameset_text_atlas 是白字图集（按钮/标签 TransLabel color=white），
标题需单独黑字。本脚本把 row0 白字转黑字（保留 alpha 抗锯齿），
生成 gameset_title_black.png（220×46），render.c 标题绘制优先用黑版。

用法：
    python3 rev64/tools/bake_gameset_title.py
产物：rev64/dist/spak_res/gameset_title_black.png
幂等：重复运行覆盖。
"""
import os
from PIL import Image

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(BASE, "rev64/dist/spak_res/gameset_text_atlas.png")
DST = os.path.join(BASE, "rev64/dist/spak_res/gameset_title_black.png")


def main():
    if not os.path.exists(SRC):
        print("MISS gameset_text_atlas.png:", SRC)
        return 1
    im = Image.open(SRC).convert("RGBA")
    w, h = im.size
    row_h = h // 9
    row0 = im.crop((0, 0, w, row_h))
    px = row0.load()
    for y in range(row0.height):
        for x in range(row0.width):
            r, g, b, a = px[x, y]
            if a > 0:
                px[x, y] = (0, 0, 0, a)   # 白字转黑字，保留 alpha 抗锯齿
    row0.save(DST)
    print(f"OK gameset_title_black.png {row0.size}  (row0={row_h}px, 标题黑字)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
