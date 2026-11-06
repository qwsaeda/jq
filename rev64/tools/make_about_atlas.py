#!/usr/bin/env python3
"""
make_about_atlas.py — 烘焙 about_text_atlas.png（P2 软件信息弹窗 datah_000 文字图集）
用法：python3 rev64/tools/make_about_atlas.py
输出：rev64/dist/spak_res/about_text_atlas.png（data_font_all.ttf sz38，7 标签竖排）
"""
import os
from PIL import Image, ImageDraw, ImageFont

DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
OUT = os.path.join(DST, "about_text_atlas.png")

# about (datah_000 软件信息): 7 标签
# row0=软件信息(标题) row1=版本 row2=官网 row3=反馈 row4=关闭 row5=1.6.8 row6=cnvcs.com
labels = ["软件信息", "版本", "官网", "反馈", "关闭", "1.6.8", "cnvcs.com"]
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
print(f"Wrote {OUT} {W}x{H} ({len(labels)} labels)")
