#!/usr/bin/env python3
"""ui_truth_verify.py — 弹窗真值核对（离线权威判据）
从原版 datah bin 反解面板 + 按钮真坐标（720×1280 基准），与
ui_menu.h 面板宏 + ui_menu.c 按钮表逐个核 Δ。
面板 3 类语法：
  ① center="true"（或无 rect）：面板 450×N 居中 720×1280，ox=(720-pw)/2, oy=(1280-ph)/2
  ② 绝对坐标 rect（115,540 / 388,698,w70,h64）：面板 @x,y，按钮局部平移
  ③ c±N 相对 center：面板 @360±N, 640±N，按钮局部平移
按钮 rect 均为面板内局部坐标 → 720 基准 = 面板偏移 + 局部
用法: python3 rev64/tools/ui_truth_verify.py [--threshold 8]
"""
import re,json,sys,os,glob
WS=os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RES=WS+'/rev64/dist/spak_res'
def parse_rect(tok):
    """rect token → (x0,y0,x1,y1) 面板局部，或 None"""
    t=tok.strip()
    m=re.match(r'(-?\d+),(-?\d+),(-?\d+),(-?\d+)',t)
    if m: return list(map(int,m.groups()))
    m=re.match(r'(\d+),(\d+),w(\d+),h(\d+)',t)
    if m:
        x,y,w,h=map(int,m.groups()); return [x,y,x+w,y+h]
    m=re.match(r'c(-?\d+),c(-?\d+),c(\d+),c(\d+)',t)
    if m:
        x0,y0,x1,y1=map(int,m.groups())
        return [360+x0,640+y0,360+x1,640+y1]
    return None
data={}
for f in sorted(glob.glob(RES+'/datah_*.bin')):
    n=int(os.path.basename(f).split('_')[1].split('.')[0])
    d=open(f,'rb').read().decode('gbk',errors='ignore')
    nb=re.findall(r'style="Button"\s+rect="([^"]+)"', d)
    if not nb: continue
    pm=re.search(r'style="(?:Dialog|BackDlg)"\s+rect="([^"]+)"', d)
    # center 属性在面板 tag 内（Dialog/BackDlg 行）：用 PanelStyle 行捕获
    # 原版弹窗定位惯例：center="true" 显式居中 / 无 rect=默认 450×335 居中 /
    # rect 以 0,0 起（0,0,W,H）= 居中弹窗（原版 450/230/420 系 Dialog 均 0,0 起）/
    # rect 绝对坐标（115,540 / 388,698）= 绝对定位（非居中）。
    pm_raw=pm.group(1) if pm else ''
    # 居中判定：center="true" 显式 / 无 rect（默认 450×335 居中）/
    # 0,0 起且 W≥180 且 H≥150（弹窗惯例；排除绝对定位小面板 120×75/60×60）
    is_00=re.match(r'0,0,', pm_raw) is not None
    big_ok=False
    if is_00 and pm_raw:
        rr2=__import__('re').match(r'0,0,(\d+),(\d+)', pm_raw)
        if rr2:
            wv,hv=int(rr2.group(1)),int(rr2.group(2))
            if wv>=450 or hv>=230: big_ok=True  # 弹窗尺寸（450/460/420/430 宽 或 230+ 高）
    center=re.search(r'style="(?:Dialog|BackDlg)"[^/]*center="true"', d) or (pm is None) or (is_00 and big_ok)
    # 面板 720 基准偏移
    if center or not pm:
        # center=true（或无 rect）：面板居中 720×1280。
        # rect 显式 0,0,W,H → W/H 取真值；无 rect → 默认 450×335
        pw,ph=450,335
        if pm:
            r=parse_rect(pm.group(1))
            if r and r[0]==0 and r[1]==0:
                pw,ph=r[2],r[3]
        ox,oy=(720-pw)/2.0,(1280-ph)/2.0
        # center 面板按钮 = 面板偏移 + 局部（下方 btns 段用 ox,oy 平移，正确）
    else:
        r=parse_rect(pm.group(1))
        if not r: continue
        ox,oy=r[0],r[1]; pw,ph=r[2]-r[0],r[3]-r[1]
    btns=[]
    for b in nb:
        br=parse_rect(b)
        if br:
            btns.append([br[0]+ox,br[1]+oy,br[2]+ox,br[3]+oy])
    data[str(n)]={'pw':pw,'ph':ph,'ox':ox,'oy':oy,'btns':btns,'center':bool(center)}
json.dump(data,open('/tmp/opencode/ui_truth.json','w'))
# 核面板宏
h=open(WS+'/rev64/forward/src_rs/ui_menu.h',encoding='utf-8').read()
src=open(WS+'/rev64/forward/src_rs/ui_menu.c',encoding='utf-8').read()
named={'diff':41,'way':72,'set':1,'nplayer':89,'newgame':62,'gameset':12,'loadlayout':17,
 'savelayout':20,'laymgr':16,'login':38,'rules':37,'newgame104':104,'join':34,'create':35,
 'ng061':61,'gameinfo':50,'bright':2,'chatlog':29,'profile':31,'setentry':3,'setentry4':4,
 'imgset7':7,'imgset8':8,'join065':65,'create069':69,'rules070':70,'room060':60,'chat063':63,
 'about':0,'exit':25,'replay':107}
thr=float(sys.argv[sys.argv.index('--threshold')+1]) if '--threshold' in sys.argv else 8.0
pb=pc=0
for m in re.finditer(r'#define (UI_([A-Z0-9]+)_POPUP_X0)\s+([\d.]+f?)', h):
    tag=m.group(2); x0=float(m.group(3).rstrip('f'))
    def gv(nm):
        mm=re.search(rf'#define UI_{tag}_POPUP_{nm}\s+([\d.]+f?)',h)
        return float(mm.group(1).rstrip('f')) if mm else None
    y0=gv('Y0');w=gv('W');hh=gv('H')
    if None in (y0,w,hh): continue
    nn=named.get(tag)
    if re.fullmatch(r'D(\d{3})',tag): nn=int(tag[1:])
    if nn is None or str(nn) not in data: continue
    v=data[str(nn)]
    pc+=1
    if abs(x0-v['ox'])>5 or abs(y0-v['oy'])>5 or abs(w-v['pw'])>8 or abs(hh-v['ph'])>8:
        pb+=1; print(f"  面板宏偏 {tag}(datah_{nn:03d}): 实值({x0:.0f},{y0:.0f},{w:.0f}x{hh:.0f}) 原版({v['ox']:.0f},{v['oy']:.0f},{v['pw']}x{v['ph']})")
print(f"面板宏核 {pc} 个，偏 {pb}（阈值 5px/8px）")
# 核按钮
bb=bc=ph_cnt=0
def nf(x): return float(x.rstrip('f'))
allcase=dict()
for tag,nn in {k:v for k,v in named.items() if k!='replay'}.items():
    allcase[tag]=(nn,rf'static const ui_{tag}_btn g_{tag}_btns')
for m in re.finditer(r'static const ui_d(\d{3})_btn g_d\1_btns', src):
    allcase['d'+m.group(1)]=(int(m.group(1)), None)
for tag,(nn,patt) in allcase.items():
    v=data.get(str(nn))
    if not v: continue
    real=re.findall(r'\{\s*([\d.]+f?),\s*([\d.]+f?),\s*([\d.]+f?),\s*([\d.]+f?),\s*(\d+)\s*\}',
                    src[src.find(patt):src.find(patt)+3000]) if patt else []
    if not real: continue
    if not v['btns']:
        if len(real)>0: ph_cnt+=len(real)
        continue
    bc+=1
    for i,o in enumerate(v['btns']):
        ocx,ocy=(o[0]+o[2])/2,(o[1]+o[3])/2
        best,bd=-1,1e9
        for j,r in enumerate(real):
            dc=abs((nf(r[0])+nf(r[2]))/2-ocx)+abs((nf(r[1])+nf(r[3]))/2-ocy)
            if dc<bd: bd=dc;best=j
        if best>=0 and bd>thr:
            bb+=1; print(f"  按钮偏 {tag}(datah_{nn:03d}) btn{i}: 原版中心({ocx:.0f},{ocy:.0f}) 最近实值btn{best} Δ={bd:.0f}")
    if len(real)>len(v['btns']): ph_cnt+=len(real)-len(v['btns'])
print(f"按钮核 {bc} 个弹窗，偏 {bb}（阈值 {thr}px）/ 简化占位控件 {ph_cnt} 个（原版无，不核 Δ）")
print("PASS" if pb==0 and bb==0 else "FAIL")
