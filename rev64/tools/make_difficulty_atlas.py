#!/usr/bin/env python3
"""make_difficulty_atlas.py — 烘焙 difficulty_text_atlas.png（datah_041 人机难度 4 区标签 + 布局/翻棋）"""
import os
from PIL import Image, ImageDraw, ImageFont
DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
OUT = os.path.join(DST, "difficulty_text_atlas.png")
labels = ["新手区:", "中级区:", "高级区:", "好友区:", "布局", "翻棋"]
row_h = 46; W = 220; H = row_h * len(labels)
font = ImageFont.truetype(FONT, 38)
im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
dr = ImageDraw.Draw(im)
for i, lbl in enumerate(labels):
    bbox = dr.textbbox((0, 0), lbl, font=font)
    tw = bbox[2] - bbox[0]; th = bbox[3] - bbox[1]
    x = (W - tw) // 2 - bbox[0]  # 居中（渲染时再按左缘/按钮中心定位）
    y = i * row_h + (row_h - th) // 2 - bbox[1]
    dr.text((x, y), lbl, font=font, fill=(255, 255, 255, 200))
im.save(OUT)
print(f"Wrote {OUT} {W}x{H}")
