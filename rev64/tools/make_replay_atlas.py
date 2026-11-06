#!/usr/bin/env python3
"""
make_replay_atlas.py — 烘焙 replay_text_atlas.png（datah_039/107 复盘 8 标签竖排）
8 标签（row0-7）：复盘/菜单/回退/前进/跳转/返回/第步/结束
用法：python3 /tmp/opencode/make_replay_atlas.py
输出：/disk/workspaces/jq/rev64/dist/spak_res/replay_text_atlas.png（220×368，data_font_all.ttf sz38）
"""
import os
from PIL import Image, ImageDraw, ImageFont

DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
OUT = os.path.join(DST, "replay_text_atlas.png")

labels = ["复盘", "菜单", "回退", "前进", "跳转", "返回", "第步", "结束"]
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
