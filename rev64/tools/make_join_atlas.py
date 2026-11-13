#!/usr/bin/env python3
"""
make_join_atlas.py — 烘焙 join_text_atlas.png（datah_034/035 加入/创建游戏 10 标签竖排）
10 标签（row0-9）：加入 / 创建 / 桌号 / 密码 / 确定 / 取消 / 蓝牙 / 边界 / 联机 / WiFi
用法：python3 rev64/tools/make_join_atlas.py
输出：rev64/dist/spak_res/join_text_atlas.png（220×460，data_font_all.ttf sz38）
"""
import os
from PIL import Image, ImageDraw, ImageFont

DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
OUT = os.path.join(DST, "join_text_atlas.png")

labels = ["加入", "创建", "桌号", "密码", "确定", "取消", "蓝牙", "边界", "联机", "WiFi"]
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
