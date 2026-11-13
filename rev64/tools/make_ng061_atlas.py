#!/usr/bin/env python3
"""
make_ng061_atlas.py — 烘焙 ng061_text_atlas.png（P2 最后 5 弹窗文字图集）
用法：python3 rev64/tools/make_ng061_atlas.py
输出：rev64/dist/spak_res/ng061_text_atlas.png（data_font_all.ttf sz38）
"""
import os
from PIL import Image, ImageDraw, ImageFont

DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
OUT = os.path.join(DST, "ng061_text_atlas.png")

# labels per atlas (each row = 1 label, vertical stack)
# ng061: 新局/先后手/电脑/玩法/蓝/红/新手/中级/高级/确定
# gameinfo: 信息/蓝方/红方/步数/回合/复盘/删除/返回
# bright: 亮度/滑条/低/中/高/确定/取消/调节/档位/保存
# chatlog: 记录/聊天/占位/帧/边界/未逆向/清除/关闭
# profile: 资料/账号/昵称/头像/胜率/边界/未逆向/修改/帮助/返回
labels = {
    "ng061": ["新局", "先后手", "电脑", "玩法", "蓝", "红", "新手", "中级", "高级", "确定"],
    "gameinfo": ["信息", "蓝方", "红方", "步数", "回合", "复盘", "删除", "返回"],
    "bright": ["亮度", "滑条", "低", "中", "高", "确定", "取消", "调节", "档位", "保存"],
    "chatlog": ["记录", "聊天", "占位", "帧", "边界", "未逆向", "清除", "关闭"],
    "profile": ["资料", "账号", "昵称", "头像", "胜率", "边界", "未逆向", "修改", "帮助", "返回"],
}["ng061"]
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
