#!/usr/bin/env python3
"""
make_rules_atlas.py — 烘焙 rules_text_atlas.png（datah_037 规则文本弹窗 8 标签竖排）
8 标签（row0-7）：规则 / 翻棋 / 布局 / 工兵 / 地雷 / 炸弹 / 夺旗 / 边界
用法：python3 rev64/tools/make_rules_atlas.py
输出：rev64/dist/spak_res/rules_text_atlas.png（220×368，data_font_all.ttf sz38）
"""
import os
from PIL import Image, ImageDraw, ImageFont

DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
OUT = os.path.join(DST, "rules_text_atlas.png")

labels = ["规则", "规则-翻棋", "规则-布局", "返回", "地雷", "炸弹", "夺旗", "边界"]
row_h = 46
W = 220
H = row_h * len(labels)

font = ImageFont.truetype(FONT, 38)
im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
dr = ImageDraw.Draw(im)
for i, lbl in enumerate(labels):
    bbox = dr.textbbox((0, 0), lbl, font=font)
    tw = bbox[2] - bbox[0]; th = bbox[3] - bbox[1]
    x = (W - tw) // 2
    y = i * row_h + (row_h - th) // 2 - bbox[1]
    dr.text((x, y), lbl, font=font, fill=(255, 255, 255, 200))
im.save(OUT)
print(f"Wrote {OUT} {W}x{H}")
