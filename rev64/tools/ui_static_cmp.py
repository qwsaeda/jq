#!/usr/bin/env python3
"""静态 UI 对比：原版 datah_*.bin 布局 XML vs 正向 render.c/ui_menu.c 绘制几何。

方法：
  原版 datah_XXX：弹窗 rect="0,0,W,H" center=true → 弹窗原点 screen = ((720-W)/2,(1280-H)/2)
                  （或非 center 时用 rect 显式原点）；子控件 rect=(x0,y0,x1,y1) 为弹窗相对坐标。
  正向：UI_DXXX_POPUP_X0/Y0/W/H（屏坐标 720 基准）+ ui_dXXX_buttons() 按钮表（屏坐标）。
  对比：原版子控件换算到屏坐标 vs 正向按钮屏坐标，输出 Δ。
用法：python3 /tmp/opencode/ui_static_cmp.py [--json] [--show-diffs]
"""
import glob, re, os, sys, json

SPAK = "rev64/dist/spak_res"
UI_MENU_C = "rev64/forward/src_rs/ui_menu.c"
UI_MENU_H = "rev64/forward/src_rs/ui_menu.h"
RENDER_C = "rev64/forward/src_rs/render.c"

# ---------- 原版布局解析 ----------
def parse_orig(path):
    """解析 datah_*.bin，返回 {dialog:(W,H,cx,cy,center), children:[{name,style,rect,text,image}]}"""
    try:
        s = open(path, 'rb').read().decode('gbk', errors='replace')
    except Exception:
        return None
    # 根 Window（Dialog）
    m = re.search(r'<Window\b[^>]*style="(Dialog|BackDlg|Window)"[^>]*>', s)
    if not m:
        m = re.search(r'<Window\b[^>]*>', s)
        if not m:
            return None
    root = m.group(0)
    dr = re.search(r'rect="([^"]*)"', root)
    if not dr:
        return None
    rstr = dr.group(1)
    # 弹窗原点：c- 形式居中于屏中心（720/2=360, 1280/2=640）
    cm = re.match(r'c-([\d.]+),c-([\d.]+),(.*)', rstr)
    if cm:
        W_off, H_off = float(cm.group(1)), float(cm.group(2))
        rest = cm.group(3)
        toks = re.findall(r'([\d.]+)([wh]?)', rest)
        W = float(toks[0][0]) if toks else 400.0
        H = float(toks[1][0]) if len(toks) > 1 else 300.0
        cx, cy = 360.0 - W_off, 640.0 - H_off
        center = True
    else:
        vals = re.findall(r'[\d.]+', rstr)
        if len(vals) < 2:
            return None
        dx0, dy0 = float(vals[0]), float(vals[1])
        # 支持 "0,0,W,H" 和 "10,705,w460,h60" 两种形式
        if 'w' in rstr and 'h' in rstr:
            wm = re.search(r'w([\d.]+)', rstr); hm = re.search(r'h([\d.]+)', rstr)
            W = float(wm.group(1)) if wm else 400.0
            H = float(hm.group(1)) if hm else 300.0
        elif len(vals) >= 4:
            W, H = float(vals[2]) - dx0, float(vals[3]) - dy0
        else:
            W, H = 400.0, 300.0
        # 原版 app 弹窗一律居中（center=true 或 popup=true 或默认 Dialog 居中）；
        # 例外：BackDlg 带显式非零原点（如 datah_107 rect=10,705）固定位置
        center = 'center="true"' in root or 'popup="true"' in root or 'style="Dialog"' in root
        explicit = (dx0 > 0 or dy0 > 0) and ('BackDlg' in root)
        if center and not explicit:
            cx, cy = (720 - W) / 2.0, (1280 - H) / 2.0
        else:
            cx, cy = dx0, dy0
    # 子控件
    children = []
    # 用非贪婪提取 <Window ...>...</Window> 内层（跳过嵌套）
    inner = s[m.end():]
    # 直接找所有 <Window .../> 自闭合 或 内层 Window 开始标签
    for cm in re.finditer(r'<Window\b[^>]*>', inner):
        tag = cm.group(0)
        if 'style="Dialog"' in tag or 'style="BackDlg"' in tag:
            continue  # 跳过嵌套弹窗
        name = re.search(r'name="([^"]*)"', tag)
        style = re.search(r'style="([^"]*)"', tag)
        rect = re.search(r'rect="([^"]*)"', tag)
        text = re.search(r'text="([^"]*)"', tag)
        image = re.search(r'image="([^"]*)"', tag)
        rv = rect.group(1) if rect else ''
        vis_hidden = 'visible="false"' in tag
        children.append({
            'name': name.group(1) if name else '',
            'style': style.group(1) if style else '',
            'rect': rv,
            'text': text.group(1) if text else '',
            'image': image.group(1) if image else '',
            'hidden': vis_hidden,
        })
    return {'dialog': (W, H, cx, cy, center), 'children': children, 'raw': s}

def parse_rect(rstr):
    """解析原版 rect 字符串 → (x0,y0,x1,y1) 局部坐标。支持：
    - "x0,y0,x1,y1" 绝对
    - "x0,y0,w105,h135" 宽高（w/h 前缀）
    - "x0,y0,x1,h60" 混合
    - "c-225,c-210,w450,h150" 居中偏移
    返回 None 若解析失败。
    """
    if not rstr:
        return None
    r = rstr.strip()
    parts = [p.strip() for p in r.split(',')]
    if not parts or len(parts) < 2:
        return None
    ox = oy = 0.0
    # c- 前缀居中偏移（相对屏中心 360,640）
    if parts[0].startswith('c-'):
        try:
            ox = float(parts[0][2:]); oy = float(parts[1][2:])
        except Exception:
            return None
        parts = parts[2:]
    if len(parts) < 2:
        return None
    def num(t):
        try: return float(t.lstrip('whc'))
        except Exception: return None
    x0 = num(parts[0]); y0 = num(parts[1])
    if x0 is None or y0 is None:
        return None
    if len(parts) >= 4:
        p2, p3 = parts[2], parts[3]
        x1 = x0 + num(p2[1:]) if p2.startswith('w') else num(p2)
        y1 = y0 + num(p3[1:]) if p3.startswith('h') else num(p3)
    elif len(parts) == 3:
        p2 = parts[2]
        if p2.startswith('h'):
            x1 = x0 + 100; y1 = y0 + num(p2[1:])
        elif p2.startswith('w'):
            x1 = x0 + num(p2[1:]); y1 = y0 + 100
        else:
            x1 = num(p2); y1 = y0 + 100
    else:
        x1 = x0 + 100; y1 = y0 + 100
    if x1 is None or y1 is None:
        return None
    return (x0 + ox, y0 + oy, x1 + ox, y1 + oy)

def rect_to_screen(rect, cx, cy):
    """rect 弹窗相对 → 屏坐标。"""
    local = parse_rect(rect)
    if local is None:
        return None
    return (cx + local[0], cy + local[1], cx + local[2], cy + local[3])

# ---------- 正向几何提取 ----------
def parse_ui_menu_c():
    """从 ui_menu.c 提取所有 g_*_btns 表（按钮屏坐标 720 基准）+ datah 映射注释。"""
    s = open(UI_MENU_C, encoding='utf-8', errors='replace').read()
    tables = {}
    # 找 static const ui_*_btn g_*_btns[N] = { ... };
    for m in re.finditer(r'static const ui_(\w+)_btn\s+g_(\w+)_btns\[(\w+)\]\s*=\s*\{(.*?)\n\};', s, re.S):
        btn_type, gname, count, body = m.group(1), m.group(2), m.group(3), m.group(4)
        # 提取 { x0,y0,x1,y1, ... } 每行
        btns = []
        for bm in re.finditer(r'\{\s*([\d.]+)f?\s*,\s*([\d.]+)f?\s*,\s*([\d.]+)f?\s*,\s*([\d.]+)f?', body):
            btns.append((float(bm.group(1)), float(bm.group(2)), float(bm.group(3)), float(bm.group(4))))
        tables[gname] = {'type': btn_type, 'count': count, 'btns': btns}
    return tables, s

def parse_ui_menu_h():
    """从 ui_menu.h 提取所有 UI_*_POPUP_X0/Y0/W/H + UI_*_COUNT。"""
    s = open(UI_MENU_H, encoding='utf-8', errors='replace').read()
    consts = {}
    for m in re.finditer(r'#define\s+(UI_(\w+)_POPUP_([XYWH]))\s+([\d.]+)f?', s):
        consts[(m.group(2), m.group(3))] = float(m.group(4))
    counts = {}
    for m in re.finditer(r'#define\s+(UI_(\w+)_COUNT)\s+(\d+)', s):
        counts[m.group(2)] = int(m.group(3))
    return consts, counts

def main():
    show_diffs = '--show-diffs' in sys.argv
    # 原版全量
    orig = {}
    for p in sorted(glob.glob(f"{SPAK}/datah_*.bin")):
        base = os.path.basename(p)
        num = re.search(r'datah_(\d+)', base)
        if not num:
            continue
        n = num.group(1)
        info = parse_orig(p)
        if info:
            orig[n] = info
    # 正向
    tables, ui_menu_src = parse_ui_menu_c()
    consts, counts = parse_ui_menu_h()

    print(f"原版布局总数: {len(orig)}")
    print(f"正向按钮表总数: {len(tables)}")
    print(f"正向 POPUP 常量: {len(consts)//3 if len(consts)>=3 else len(consts)} 组")

    # 数据映射：datah_XXX → 正向表名（从 ui_menu.c 注释提取 datah_XXX）
    # 用注释里的 "datah_XXX 证据" 关联
    dmap = {}
    for gname, info in tables.items():
        # 找该表附近的 datah_ 引用
        idx = ui_menu_src.find(f"g_{gname}_btns")
        if idx < 0:
            continue
        near = ui_menu_src[max(0,idx-400):idx+400]
        m = re.search(r'datah_(\d+)', near)
        if m:
            dmap[m.group(1)] = gname

    print(f"正向→原版 datah 映射: {len(dmap)} 项")
    for n in sorted(dmap):
        print(f"  datah_{n} → {dmap[n]}")

    # 对比：原版子控件按钮 rect vs 正向按钮表
    diffs = []
    matched = 0
    for n, gname in dmap.items():
        if n not in orig:
            continue
        o = orig[n]
        W, H, cx, cy, center = o['dialog']
        btns = tables[gname]['btns']
        # 原版按钮（Button/IconBtn/SmallBtn2/CheckBox/Slider 交互控件，排除 Picture/TransLabel/TransTextBox/PiecePic/ListBox + visible=false 隐藏）
        orig_btns = [c for c in o['children'] if any(k in c['style'] for k in ('Button','IconBtn','CheckBox','Slider')) and not c.get('hidden',False)]
        # 去重重叠（原版冗余定义如 datah_016 Ok2/Cancel2、datah_050 Close1——用相近 rect 判定）
        dedup = []
        for c in sorted(orig_btns, key=lambda c: (rect_to_screen(c['rect'],cx,cy) or (0,0,0,0))[1]):
            cs = rect_to_screen(c['rect'],cx,cy)
            is_dup = False
            for d in dedup:
                ds = rect_to_screen(d['rect'],cx,cy)
                if cs and ds and abs(cs[0]-ds[0])<15 and abs(cs[1]-ds[1])<15 and abs(cs[2]-ds[2])<15 and abs(cs[3]-ds[3])<15:
                    is_dup = True; break
            if not is_dup:
                dedup.append(c)
        orig_btns = dedup
        if not orig_btns:
            continue
        matched += 1
        # 匹配：正向每个按钮 → 原版最近 y 控件（滑动条在表里是 Slider 区）
        print(f"\n### datah_{n}（{gname}，弹窗 {W:.0f}×{H:.0f} 居中({cx:.1f},{cy:.1f})）")
        used_orig = set()
        for fi, frct in enumerate(btns):
            # 找原版最近控件（按中心 y 距离）
            best = None; bestd = 1e9
            for oi, oc in enumerate(orig_btns):
                if oi in used_orig: continue
                orct = rect_to_screen(oc['rect'], cx, cy)
                if not orct: continue
                dy = abs((frct[1]+frct[3])/2 - (orct[1]+orct[3])/2)
                if dy < bestd: bestd, best = dy, (oi, oc, orct)
            if best is None:
                print(f"  [**EXTRA**] 正向({frct[0]:.0f},{frct[1]:.0f})-({frct[2]:.0f},{frct[3]:.0f}) 原版无")
                diffs.append((n, gname, fi, '', '', None, frct, None)); continue
            oi, oc, orct = best
            used_orig.add(oi)
            # 中心偏差
            dcx = ((frct[0]+frct[2])/2 - (orct[0]+orct[2])/2)
            dcy = ((frct[1]+frct[3])/2 - (orct[1]+orct[3])/2)
            maxd = max(abs(dcx), abs(dcy))
            flag = "OK" if maxd <= 3 else "**DIFF**"
            if maxd > 3:
                diffs.append((n, gname, fi, oc['name'], oc['text'], orct, frct, (dcx, dcy)))
            print(f"  [{flag}] {'['+oc['name']+']' if oc['name'] else '?':16s} "
                  f"原版({orct[0]:.0f},{orct[1]:.0f})-({orct[2]:.0f},{orct[3]:.0f}) "
                  f"正向({frct[0]:.0f},{frct[1]:.0f})-({frct[2]:.0f},{frct[3]:.0f}) "
                  f"Δ中心({dcx:+.0f},{dcy:+.0f}) text='{oc['text']}' style={oc['style']}")
        # 原版未被匹配的
        for oi, oc in enumerate(orig_btns):
            if oi in used_orig: continue
            orct = rect_to_screen(oc['rect'], cx, cy)
            if not orct: continue
            diffs.append((n, gname, oi, oc['name'], oc['text'], orct, None, None))
            print(f"  [**MISS**] {'['+oc['name']+']' if oc['name'] else '?':16s} "
                  f"原版({orct[0]:.0f},{orct[1]:.0f})-({orct[2]:.0f},{orct[3]:.0f}) 正向无 "
                  f"text='{oc['text']}' style={oc['style']}")

    print(f"\n{'='*100}")
    print(f"对比弹窗: {matched}，差异项: {len(diffs)}")
    if show_diffs:
        for d in diffs:
            print(f"  datah_{d[0]} {d[1]} [{d[2]}] name={d[3]} text={d[4]} Δ={d[7]}")

if __name__ == "__main__":
    main()
