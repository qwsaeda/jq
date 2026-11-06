#!/usr/bin/env python3
"""
make_drawdetail_atlas.py — 烘焙 drawdetail_text_atlas.png（P2 datah_042 提和详情文字图集）
用法：python3 rev64/tools/make_drawdetail_atlas.py
输出：rev64/dist/spak_res/drawdetail_text_atlas.png（data_font_all.ttf sz38，7 标签竖排）
★ P2（datah_042 证据）：提和详情弹窗（联机版 ☰ 菜单 7 图标）文字图集——
提和/认输/聊天/系统设置/玩家资料/战况统计/软件信息 7 标签竖排（每行 1 标签）。
诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
联机提和/认输走 BT_T_DRAW/BT_T_RESIGN 帧 + 边界对话框，不包装成"已提和判定"。
"""
import os
from PIL import Image, ImageDraw, ImageFont

DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
OUT = os.path.join(DST, "drawdetail_text_atlas.png")

# labels per atlas (each row = 1 label, vertical stack)
# drawdetail: 提和/认输/聊天/系统设置/玩家资料/战况统计/软件信息
labels = {
    "drawdetail": ["提和", "认输", "聊天", "系统设置", "玩家资料", "战况统计", "软件信息"],
}["drawdetail"]
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
