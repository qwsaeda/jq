#!/usr/bin/env python3
# SPAK2UI 脚本化（datah_013）：烘焙 ui13_text_atlas.png（1 标签）
from PIL import Image, ImageDraw, ImageFont
import os
LBL = ['标题']
W,H,SZ = 220, 368, 38
img = Image.new("RGBA",(W,H),(0,0,0,0)); d=ImageDraw.Draw(img)
try: fnt=ImageFont.truetype("/disk/workspaces/jq/rev64/dist/spak_res/data_font_all.ttf",SZ)
except: fnt=ImageFont.load_default()
y=5
for t in LBL:
    bb=d.textbbox((0,0),t,font=fnt); tw=bb[2]-bb[0]
    d.text(((W-tw)//2,y),t,fill=(255,255,255,255),font=fnt); y+=30
out="/disk/workspaces/jq/rev64/dist/spak_res/ui13_text_atlas.png"; img.save(out)
print("baked",out,"tags",len(LBL))
