#!/usr/bin/env python3
"""
make_p2_batch_atlas.py — 烘焙 P2 批量 9 弹窗文字图集（7 个 PNG）
输出：rev64/dist/spak_res/{setentry,imgset,join065,create069,rules070,room060,chat063}_text_atlas.png
data_font_all.ttf sz38，各弹窗标签竖排。
"""
import os
from PIL import Image, ImageDraw, ImageFont

DST = "/disk/workspaces/jq/rev64/dist/spak_res"
FONT = os.path.join(DST, "data_font_all.ttf")
row_h = 46
W = 220

atlas_configs = {
    # setentry (datah_003/004 设置入口共用): 7 标签
    # row0=设置(标题，原版 datah_003/004 无标题——渲染已删) row1=图像设置 row2=声音设置 row3=游戏设置
    # row4=简化 row5=取消 row6=返回
    # ★ 2026-11-11 用户"设置入口弹窗不是原版"指正：原版按钮文字"图像设置/声音设置/游戏设置"（一行），
    #   旧图集"图像/声音/游戏"（缺"设置"后缀）——改全名。
    "setentry_text_atlas": ["设置", "图像设置", "声音设置", "游戏设置", "简化", "取消", "返回"],
    # imgset (datah_007/008 图像设置共用): 10 标签
    # row0=图像(标题) row1=界面 row2=棋子 row3=夜间 row4=对方
    # row5=开(7=开 占位) row6=关 row7=开 row8=关 row9=确定 row10=取消
    # 实际用 10 行：图像/界面/棋子/夜间/对方/开/关/开/关/确定
    "imgset_text_atlas": ["图像", "界面", "棋子", "夜间", "对方", "开", "关", "开", "关", "确定"],
    # join065 (datah_065 加入游戏完整版): 10 标签
    # row0=加入(标题) row1=蓝方 row2=红方 row3=布局 row4=暗棋
    # row5=开 row6=开 row7=开 row8=开 row9=开始
    "join065_text_atlas": ["加入", "先手方:", "后手方:", "玩法:", "明暗棋:", "和棋:", "大本营:", "吃地雷:", "吃军旗:", "开", "蓝", "红", "布局", "暗棋", "开", "开", "开", "开", "开始"],
    # create069 (datah_069 创建游戏完整版): 10 标签
    # row0=创建(标题) row1=先后手 row2=玩法 row3=明暗棋 row4=规则
    # row5=蓝方 row6=红方 row7=布局 row8=翻棋 row9=确定
    "create069_text_atlas": ["创建", "先后手:", "玩法:", "明暗棋:", "游戏规则:", "蓝", "红", "布局", "暗棋", "确定"],
    # rules070 (datah_070 游戏规则完整版): 10 标签
    # row0=规则(标题) row1=和棋 row2=大本营 row3=吃雷 row4=吃旗
    # row5=开 row6=关 row7=开 row8=关 row9=确定
    "rules070_text_atlas": ["规则", "和棋:", "大本营:", "吃地雷:", "吃军旗:", "开", "关", "开", "关", "确定"],
    # room060 (datah_060 房间态): 8 标签
    # row0=房间(标题) row1=蓝方 row2=红方 row3=开始 row4=换桌 row5=离开 row6=边界 row7=未实现
    "room060_text_atlas": ["房间", "玩家1", "准备中", "局分", "胜率", "玩家2", "准备中", "局分", "胜率", "开始", "换桌", "离开"],
    # chat063 (datah_063 聊天完整版): 8 标签
    # row0=聊天(标题) row1=输入 row2=占位 row3=边界 row4=未逆向
    # row5=发送 row6=记录 row7=关闭
    "chat063_text_atlas": ["聊天", "聊天记录（联机聊天文本未逆向，蓝牙聊天=占位帧）", "发送", "记录", "关闭"],
}

font = ImageFont.truetype(FONT, 38)
for name, labels in atlas_configs.items():
    H = row_h * len(labels)
    OUT = os.path.join(DST, name + ".png")
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
print("All P2 batch atlases done")
