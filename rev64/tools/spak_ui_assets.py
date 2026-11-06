#!/usr/bin/env python3
"""spak_ui_assets.py — 原版 UI 位图真值索引（ch02 N45 门牌号→房间真值的落地工具）

背景: 原版 272 个 datah 对话框 XML 里 image="popup.cfg" / "/data/image/new.png" 等
      是「门牌号」；位图真值在 SPAK 的 datax 组（PNG）。datax 只有编号+hash，无文件名，
      故本工具用「视觉特征 + 语义」建立 可维护的 编号→语义 索引（JSON），供 UI 接入复用。

用法:
  python3 tools/spak_ui_assets.py --index      # 完整索引（编号/尺寸/可见度/边缘特征/已知语义）→ JSON + 打印
  python3 tools/spak_ui_assets.py --panel      # 面板/9patch 候选（边缘有色环 + 内部均匀）
  python3 tools/spak_ui_assets.py --icons      # 图标级（36-130px 方图）候选
  python3 tools/spak_ui_assets.py --sheet N   # 导出编号列表的对比 sheet（目视确认语义）
  python3 tools/spak_ui_assets.py --set 246 "圆形按钮（面板候选误判）"   # 写语义（累积）
"""
import os,re,json,glob,sys
from PIL import Image

BASE=os.path.join(os.path.dirname(os.path.abspath(__file__)),"..","dist","spak_res")
IDX=os.path.join(BASE,"ui_asset_index.json")

# 已人工确认的语义（目视识别；datax 编号 → 语义）
KNOWN={
 124:"蓝i(软件信息)",125:"蓝i(按下态)",128:"右箭头(电脑变着/前进)",129:"右箭头(按下态)",
 132:"气泡(聊天)",133:"气泡(按下态)",134:"绿回转(悔棋)",135:"绿回转(按下态)",
 136:"扳手(功能设置)",137:"扳手(按下态)",138:"蓝椭圆(操作说明)",139:"蓝椭圆(按下态)",
 140:"握手(提和)",141:"握手(按下态)",142:"红X(退出)",143:"红X(按下态)",
 144:"画笔纸(编辑)",145:"画笔纸(按下态)",146:"绿勾(确定)",147:"绿勾(按下态)",
 148:"文件夹(新建/载入)",149:"文件夹(按下态)",153:"黄星(重新开始)",154:"黄星(按下态)",
 159:"绿回转(悔棋变体)",160:"绿回转(按下态)",161:"背包(加载)",162:"背包(按下态)",
 163:"白纸(游戏规则)",164:"白纸(按下态)",165:"绿格纸(存档)",166:"绿格纸(按下态)",
 169:"左箭头(返回)",170:"左箭头(按下态)",171:"彩球(战况统计)",172:"彩球(按下态)",
 173:"放大镜(详细/搜索)",174:"放大镜(按下态)",179:"红旗(认输/投降,110×110)",
 194:"彩色按钮底图1(160×40,橙/紫/绿/蓝)",195:"彩色按钮底图2(244×61)",
 204:"竖条(61×976)",207:"竖条(24×95)",213:"方块(54×54)",
 218:"方块(77×77)",221:"方块(81×77)",227:"细线(30×2)",
 232:"竖条(24×44)",236:"竖条(24×64)",239:"竖条(24×60)",
 243:"蓝按钮(72×72)",244:"灰按钮(72×72)",246:"白圆按钮(72×72)",
 183:"棋子图集(344×427)",188:"棋盘(1100×1121)",177:"对局棋盘背景(720×1280)",
 185:"横幅(620×119)",294:"主菜单背景(800×1205)",
}

def datax_files():
    out=[]
    for f in sorted(glob.glob(os.path.join(BASE,"datax_*.bin"))):
        m=re.match(r'datax_(\d{3})_',os.path.basename(f))
        if not m: continue
        if open(f,'rb').read(4)!=b'\x89PNG': continue
        out.append((int(m.group(1)),f))
    return out

def features(f):
    try: im=Image.open(f).convert("RGBA")
    except Exception: return None
    w,h=im.size; px=im.load()
    n=w*h
    vis=sum(1 for y in range(h) for x in range(w) if px[x,y][3]>10 and sum(px[x,y][:3])>40)/n
    # 边缘有色环（9-patch 边框特征）：统计最外 2 圈中"不透明且与内部主色不同"的比例
    ring=0
    for x in range(w):
        for y in (0,1,h-2,h-1):
            if 0<=y<h:
                p=px[x,y]
                if p[3]>60: ring+=1
    for y in range(h):
        for x in (0,1,w-2,w-1):
            if 0<=x<w:
                p=px[x,y]
                if p[3]>60: ring+=1
    interior=[px[x,y] for y in range(2,max(3,h-2)) for x in range(2,max(3,w-2))]
    if interior:
        from collections import Counter
        c=Counter((p[0]//32,p[1]//32,p[2]//32) for p in interior if p[3]>40)
        main=c.most_common(1)[0] if c else None
    else: main=None
    # 内部均匀度（主色占比）
    unif=(main[1]/len(interior)) if (main and interior) else 0
    return {"w":w,"h":h,"vis":round(vis,3),"ring_px":ring,
            "main":[main[0] if main else None, round(unif,3) if main else 0]}

def load_idx():
    if os.path.exists(IDX):
        return json.load(open(IDX))
    return {}

def save_idx(idx):
    json.dump(idx,open(IDX,"w"),ensure_ascii=False,indent=1)
    print(f"索引已写 {IDX}（{len(idx)} 条）")

def cmd_index(args):
    idx=load_idx()
    rows=[]
    for n,f in datax_files():
        ft=features(f)
        if not ft: continue
        idx[str(n)]={"file":os.path.basename(f),**ft,"sem":idx.get(str(n),{}).get("sem") or KNOWN.get(n)}
        rows.append((n,ft,idx[str(n)]["sem"]))
    rows.sort()
    print(f"datax PNG 真值共 {len(rows)} 个：")
    for n,ft,sem in rows:
        print(f"  datax_{n:03d} {ft['w']}x{ft['h']} 可见{ft['vis']*100:3.0f}% 边缘环{ft['ring_px']:4d} "
              f"主色统一{ft['main'][1]*100:3.0f}%  {sem or ''}")
    save_idx(idx)

def cmd_panel(args):
    print("★ 弹窗面板 / 9-patch 候选（边缘有色环 + 内部均匀 + 近似方形）：")
    for n,f in datax_files():
        ft=features(f)
        if not ft: continue
        w,h=ft["w"],ft["h"]
        if not (40<=w<=600 and 40<=h<=600): continue
        ar=min(w,h)/max(w,h)
        if ar<0.55: continue
        if ft["ring_px"]<8: continue
        sem=KNOWN.get(n,"")
        print(f"  datax_{n:03d} {w}x{h} 可见{ft['vis']*100:3.0f}% 边缘环{ft['ring_px']:4d} "
              f"主色统一{ft['main'][1]*100:3.0f}%  {sem}")

def cmd_icons(args):
    print("★ 图标级位图候选（36-130px 方图）：")
    for n,f in datax_files():
        ft=features(f)
        if not ft: continue
        w,h=ft["w"],ft["h"]
        if 36<=w<=130 and 36<=h<=130 and min(w,h)/max(w,h)>0.6:
            print(f"  datax_{n:03d} {w}x{h} 可见{ft['vis']*100:3.0f}%  {KNOWN.get(n,'')}")

def cmd_sheet(args):
    nums=[int(x) for x in args]
    from PIL import ImageDraw
    tiles=[]
    for n in nums:
        g=[f for i,f in datax_files() if i==n]
        if not g: continue
        im=Image.open(g[0]).convert("RGBA")
        im=im.resize((64,64),Image.LANCZOS)
        tiles.append((n,im))
    if not tiles: print("无候选"); return
    cols=min(8,len(tiles)); rows=(len(tiles)+cols-1)//cols
    sheet=Image.new("RGBA",(cols*70,rows*78),(30,30,30,255))
    d=ImageDraw.Draw(sheet)
    for i,(n,im) in enumerate(tiles):
        r,c=divmod(i,cols)
        sheet.paste(im,(c*70+3,r*78+3),im)
        d.text((c*70+3,r*78+68),f"{n}",fill=(255,255,255,255))
    out="/tmp/opencode/jq_ui_assets/sheet2.png"
    os.makedirs(os.path.dirname(out),exist_ok=True)
    sheet.save(out); print("sheet:",out)

def cmd_set(args):
    n=int(args[0]); sem=args[1]
    idx=load_idx(); idx.setdefault(str(n),{})["sem"]=sem; save_idx(idx)

if __name__=="__main__":
    a=sys.argv[1:]
    if not a: print(__doc__); sys.exit(1)
    if a[0]=="--index": cmd_index(a[1:])
    elif a[0]=="--panel": cmd_panel(a[1:])
    elif a[0]=="--icons": cmd_icons(a[1:])
    elif a[0]=="--sheet": cmd_sheet(a[1:])
    elif a[0]=="--set": cmd_set(a[1:])
    else: print(__doc__)
