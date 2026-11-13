#!/usr/bin/env python3
"""
make_ng104_atlas.py — 烘焙 ng104_text_atlas.png（datah_104 新局完整版 10 标签竖排）
10 标签（row0-9）：新局 / 我方 / 电脑 / 玩法 / 蓝方 / 红方 / 新手 / 中级 / 高级 / 确定
用法：python3 rev64/tools/make_ng104_atlas.py
输出：rev64/dist/spak_res/ng104_text_atlas.png（220×460，data_font_all.ttf sz38）
"""
import os
from PIL import Image, ImageDraw, ImageFont

DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
OUT = os.path.join(DST, "ng104_text_atlas.png")

labels = ["新局", "我方", "电脑", "玩法", "蓝方", "红方", "新手", "中级", "高级", "确定"]
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
    fill = (0, 0, 0, 255) if i == 0 else (255, 255, 255, 200)  # row0 title black, others white
    dr.text((x, y), lbl, font=font, fill=fill)
im.save(OUT)
print(f"Wrote {OUT} {W}x{H}")
