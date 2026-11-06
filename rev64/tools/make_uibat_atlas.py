#!/usr/bin/env python3
"""
make_uibat_atlas.py — 烘焙 uibat_text_atlas.png（SPAK2UI 批量 12 弹窗共用文字图集）
用法：python3 rev64/tools/make_uibat_atlas.py
输出：rev64/dist/spak_res/uibat_text_atlas.png（data_font_all.ttf sz38，14 标签竖排 220×644）
"""
import os
from PIL import Image, ImageDraw, ImageFont

DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
OUT = os.path.join(DST, "uibat_text_atlas.png")

# SPAK2UI 批量 12 弹窗共用：14 标签
# row0=战况统计(044/045 标题) row1=详细(045/100 按钮+标题) row2=玩家资料(046 标题)
# row3=玩家列表(047 标题) row4=对局详情(049 标题) row5=我的对局(051 标题)
# row6=历史对局(054/110 标题) row7=创建游戏(073 标题) row8=人机对战(090 按钮)
# row9=对战方式(090 标题) row10=联网对战(090 按钮) row11=版本更新(118 标题/按钮)
# row12=清空(051/054/110/073 按钮) row13=关闭(046/047/049/118 按钮)
labels = ["战况统计", "详细", "玩家资料", "玩家列表", "对局详情",
          "我的对局", "历史对局", "创建游戏", "人机对战", "对战方式",
          "联网对战", "版本更新", "清空", "关闭"]
row_h = 46
W = 220
H = row_h * len(labels)

font = ImageFont.truetype(FONT, 38)
im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
dr = ImageDraw.Draw(im)
for i, lbl in enumerate(labels):
    bbox = dr.textbbox((0, 0), lbl, font=font)
    tw = bbox[2] - bbox[0]
    x = (W - tw) // 2
    y = i * row_h + (row_h - (bbox[3] - bbox[1])) // 2 - bbox[1]
    dr.text((x, y), lbl, font=font, fill=(255, 255, 255, 200))
im.save(OUT)
print(f"Wrote {OUT} {W}x{H} ({len(labels)} labels)")
