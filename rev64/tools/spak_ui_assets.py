#!/usr/bin/env python3
"""spak_ui_assets.py：原版 UI 位图真值清单（ch02 N45 门牌号→房间真值）
用法: python3 tools/spak_ui_assets.py [--list] [--popup] [--icons]
   --list   全量 datax PNG 位图清单（编号/尺寸/可见度/大小）
   --popup  弹窗面板级位图（panel 候选，popup.cfg 真值来源）
   --icons  图标级位图（icon_info/file/config 类，40-80px 方图）
原版资源真值在 dist/spak_res/，so 里只有门牌号（XML image= 引用 / 常量池），真值在此。
"""
import glob,os,re,sys
from PIL import Image
BASE=os.environ.get("SPAK_RES","rev64/dist/spak_res")
def pngmeta(f):
    if open(f,'rb').read(4)!=b'\x89PNG': return None
    try:
        im=Image.open(f).convert("RGBA")
    except: return None
    w,h=im.size
    px=list(im.getdata())
    vis=sum(1 for p in px if p[3]>10 and sum(p[:3])>40)/len(px)
    return w,h,vis,os.path.getsize(f)
def datax_all():
    out=[]
    for f in sorted(glob.glob(os.path.join(BASE,"datax_*.bin"))):
        m=re.match(r'datax_(\d{3})_',os.path.basename(f))
        if not m: continue
        meta=pngmeta(f)
        if meta: out.append((int(m.group(1)),os.path.basename(f),meta))
    return out
if __name__=="__main__":
    args=sys.argv[1:] or ["--list"]
    rows=datax_all()
    if "--list" in args:
        print(f"原版 datax PNG 位图真值共 {len(rows)} 个（dist/spak_res/）：")
        for n,f,(w,h,vis,sz) in rows:
            print(f"  datax_{n:03d} {w}x{h} 可见{vis*100:3.0f}% {sz//1024:>4}KB {f}")
    if "--popup" in args:
        pp=[r for r in rows if r[2][2]>0.05 and (r[2][0]<520 and r[2][1]<320)]
        pp.sort()
        print(f"\n★ 弹窗面板/按钮级位图（宽<520 高<320 可见>5%）共 {len(pp)} 个：")
        for n,f,(w,h,vis,sz) in pp:
            tag=""
            if w>300 and h<60: tag="← 横幅/长条按钮"
            elif 150<w<300 and 40<h<120: tag="← 按钮底图"
            elif 60<w<130 and 60<h<130: tag="← 图标/面板块"
            print(f"  datax_{n:03d} {w}x{h} 可见{vis*100:3.0f}% {sz//1024:>4}KB {tag}")
    if "--icons" in args:
        ic=[r for r in rows if 36<=r[2][0]<=82 and 36<=r[2][1]<=82]
        ic.sort()
        print(f"\n★ 图标级位图（icon_*.png 真值，36-82px 方图）共 {len(ic)} 个：")
        for n,f,(w,h,vis,sz) in ic:
            print(f"  datax_{n:03d} {w}x{h} 可见{vis*100:3.0f}% {sz//1024:>3}KB")
