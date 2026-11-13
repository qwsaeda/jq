#!/usr/bin/env python3
"""
make_layres_atlas.py — 烘焙 layres_text_atlas.png（datah_016/017/020 布局存档 9 标签竖排）
9 标签（row0-8）：载入布局 / 保存布局 / 布局管理 / 取消 / 保存 / 改名 / 删除 / 关闭 / 载入
用法：python3 rev64/tools/make_layres_atlas.py
输出：rev64/dist/spak_res/layres_text_atlas.png（220×414，data_font_all.ttf sz38）
"""
import os, sys
from PIL import Image, ImageDraw, ImageFont

DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
OUT = os.path.join(DST, "layres_text_atlas.png")

labels = ["载入布局", "保存布局", "布局管理", "取消", "保存", "改名", "删除", "关闭", "载入"]
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
