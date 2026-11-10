#!/usr/bin/env python3
"""
make_exit_atlas.py — 烘焙 exit_text_atlas.png（datah_025 ExitConfirm 3 标签竖排）
★ 2026-11-10 用户"返回键点击无效/这不是悔棋"指正：↩ 返回 → ExitConfirm 弹窗
  （原版 Junqi.ExitConfirm='真的要退出吗？' + 确定/取消）。
3 标签：真的要退出吗？ / 确定 / 取消
用法：python3 rev64/tools/make_exit_atlas.py
"""
import os
from PIL import Image, ImageDraw, ImageFont

DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
OUT = os.path.join(DST, "exit_text_atlas.png")

labels = ["真的要退出吗？", "您真的要悔棋吗？", "您真的要认输吗？", "确定", "取消"]
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
