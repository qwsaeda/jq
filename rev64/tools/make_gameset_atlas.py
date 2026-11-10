#!/usr/bin/env python3
"""
make_gameset_atlas.py — 烘焙 gameset_text_atlas.png（datah_012 游戏设置 9 标签竖排）
★ 2026-11-10 用户指正"这个界面是对齐了？"→ 对齐 datah_012 XML（GBK 原文实锤）：
  Title '游戏设置'（rect 55,10,440,50）+ 标签带冒号（和棋:/大本营:/吃地雷:/吃军旗:）
  + 开/关 + 确定/取消 = 9 竖排行。旧版 8 行无标题无冒号 = 未对齐。
用法：python3 tools/make_gameset_atlas.py
"""
import os, sys
from PIL import Image, ImageDraw, ImageFont

DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
OUT = os.path.join(DST, "gameset_text_atlas.png")

labels = ["游戏设置", "和棋:", "大本营:", "吃地雷:", "吃军旗:", "开", "关", "确定", "取消"]
row_h = 46
W = 220
H = row_h * len(labels)

font = ImageFont.truetype(FONT, 38)
im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
dr = ImageDraw.Draw(im)
for i, lbl in enumerate(labels):
    bbox = dr.textbbox((0, 0), lbl, font=font)
    tw = bbox[2] - bbox[0]; th = bbox[3] - bbox[1]
    if 1 <= i <= 4:
        # 标签（和棋:/大本营:/吃地雷:/吃军旗:）右对齐：原版 datah_012 align=right，
        # 文字右缘对齐到 290（720 基准）。图集内右缘=W，渲染时 box 右缘对齐 290。
        x = W - tw - bbox[0]
    else:
        x = (W - tw) // 2 - bbox[0]
    y = i * row_h + (row_h - th) // 2 - bbox[1]
    dr.text((x, y), lbl, font=font, fill=(255, 255, 255, 200))
im.save(OUT)
print(f"Wrote {OUT} {W}x{H}")
