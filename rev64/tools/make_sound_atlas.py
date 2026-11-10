#!/usr/bin/env python3
"""
make_sound_atlas.py — 烘焙 sound_text_atlas.png（datah_001 声音设置 标题+3 标签）
★ 2026-11-10 用户"游戏设置UI就没有对齐，很多界面都没有对齐"指正：
  rs_draw_d001（datah_001）只有 Sound/BtnSound/确定/取消 按钮文字，缺
  Title'声音设置' + 标签 静音模式:/按键声音:/音量:。烘焙本图集补画。
行高 40（匹配 RS_ATLAS_ROW_H，rs_atlas_label_wh 用）。
用法：python3 tools/make_sound_atlas.py
"""
import os
from PIL import Image, ImageDraw, ImageFont

DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
OUT = os.path.join(DST, "sound_text_atlas.png")

labels = ["声音设置", "静音模式:", "按键声音:", "音量:"]
row_h = 40
W = 220
H = row_h * len(labels)

font = ImageFont.truetype(FONT, 30)
im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
dr = ImageDraw.Draw(im)
for i, lbl in enumerate(labels):
    bbox = dr.textbbox((0, 0), lbl, font=font)
    tw = bbox[2] - bbox[0]; th = bbox[3] - bbox[1]
    if i == 0:
        # 标题居中
        x = (W - tw) // 2 - bbox[0]
    else:
        # 标签左对齐（原版 align=left，rect x0=40 局部 → 720 基准 175）
        x = 0
    y = i * row_h + (row_h - th) // 2 - bbox[1]
    dr.text((x, y), lbl, font=font, fill=(255, 255, 255, 220))
im.save(OUT)
print(f"Wrote {OUT} {W}x{H}")
