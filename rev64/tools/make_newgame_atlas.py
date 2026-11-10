#!/usr/bin/env python3
"""
make_newgame_atlas.py — 烘焙 newgame_text_atlas.png（datah_062 两人新局 7 行）
★ 2026-11-10 用户"很多界面都没有对齐"指正：rs_draw_newgame（datah_062）缺
  Title'新局' + 标签玩法:/明暗棋: 未右对齐（原版 align=right rect 右缘 290）。
  加'新局'标题行（row0），玩法:/明暗棋: 右对齐（图集右缘），行号整体 +1。
用法：python3 tools/make_newgame_atlas.py
"""
import os
from PIL import Image, ImageDraw, ImageFont

DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
OUT = os.path.join(DST, "newgame_text_atlas.png")

labels = ["新局", "玩法:", "明暗棋:", "布局明棋", "翻棋暗棋", "确定", "取消"]
row_h = 46
W = 220
H = row_h * len(labels)

font = ImageFont.truetype(FONT, 38)
im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
dr = ImageDraw.Draw(im)
for i, lbl in enumerate(labels):
    bbox = dr.textbbox((0, 0), lbl, font=font)
    tw = bbox[2] - bbox[0]; th = bbox[3] - bbox[1]
    if 1 <= i <= 2:
        # 标签（玩法:/明暗棋:）右对齐：原版 align=right，文字右缘对齐 290（720 基准）
        x = W - tw - bbox[0]
    else:
        x = (W - tw) // 2 - bbox[0]
    y = i * row_h + (row_h - th) // 2 - bbox[1]
    dr.text((x, y), lbl, font=font, fill=(255, 255, 255, 200))
im.save(OUT)
print(f"Wrote {OUT} {W}x{H}")
