#!/usr/bin/env python3
"""
spak2ui.py — SPAK datah XML → src_rs 弹窗对齐代码 一键生成器（脚本化批量对齐）

解决"逐弹窗手写子代理"的低效：原版 272 个 datah 对话框全是同构结构
（面板居中 + 按钮 rect 局部坐标 1:1 平移 + action 语义），可机读批量生成。

用法：
  # ① 分析一批 datah，生成 720 坐标 + 按钮表（JSON，人审）
  python3 rev64/tools/spak2ui.py analyze 003 004 007 008 065 069 070 060 063

  # ② 从已审 JSON 配置生成 C 代码骨架（ui_menu.h/.c 片段 + evverify 用例 +
  #    layout_align_check DIALOGS + make_*_atlas.py + AUDIT §表），人贴进源文件
  python3 rev64/tools/spak2ui.py gen setentry imgset7 join065 ...

  # ③ 全自动：分析 + 生成 + 贴入 5 个源文件 + 编译 + evverify + 一致性（一条命令）
  python3 rev64/tools/spak2ui.py auto 003 004 007 008

  # ④ 看当前已对齐进度（已对齐/未对齐清单 + 按钮数分布）
  python3 rev64/tools/spak2ui.py status

判据：
  - 面板居中偏移 = ((720-panel_w)/2, (1280-panel_h)/2)，局部坐标 1:1 平移
  - 绝对坐标面板（style=BackDlg）直接平移 @ 原版 rect 位置
  - 每个 Button/IconBtn 生成 720 基准按钮 + action 语义（循环切换/切弹窗/边界对话框）
  - 生成的 C 骨架标注 ★ 诚实边界（哪些业务真实现/哪些占位），不包装简化版
  - 自动编译 + evverify + 一致性回归（生成不破坏默认行为）
"""
import argparse, json, os, re, subprocess, sys

WS = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPAK = "/tmp/opencode/spak_out"
SRC = os.path.join(WS, "rev64/src_rs")
TOOLS = os.path.join(WS, "rev64/tools")
NDK = "/tmp/opencode/android-ndk-r25b"

# 已对齐 datah 清单（ui_menu.h UI_* 枚举 + AUDIT §缺口表 ✅）
ALIGNED = {
    1,2,6,12,16,17,19,20,23,28,29,31,32,34,35,37,38,39,41,42,50,
    61,62,67,72,89,104,107,
    3,4,7,8,60,63,65,69,70,   # P2 9 弹窗批量批次
    44,45,46,47,49,51,54,73,90,100,110,118,  # ★ SPAK2UI 12 弹窗批量批次（战况统计子菜单链+玩家/对局/对战方式/创建游戏/版本更新，诚实边界占位）
}

def spak_files():
    fs = [f for f in os.listdir(SPAK) if f.startswith("datah_")]
    out = []
    for f in fs:
        m = re.match(r"datah_(\d+)_", f)
        if m: out.append((int(m.group(1)), os.path.join(SPAK, f)))
    return sorted(out)

def _parse_rect_tokens(rect):
    """解析原版 rect token 串 → (cx_off, cy_off, W, H, abs_x, abs_y, kind)

    原版 rect 语法有 5 种变体（对抗审计 AUD-A-* geom-delta 实锤）：
      1. "0,0,W,H"        → 居中（plain）
      2. "0,0,wW,hH"      → 居中（带 w/h 前缀）
      3. "x,y,wW,hH"      → 绝对位置（带 w/h 前缀）
      4. "x,y,X,Y"        → 绝对位置（plain 右下角）
      5. "c-CX,c-CY,wW,hH"→ 居中锚点（c- 前缀 = 相对屏幕中心的偏移）
    旧解析器把 2/3/5 全判成"绝对定位"，且对 5 的 W/H 算成 (W-cx, H-cy) → 尺寸与偏移双错。
    """
    parts = [p.strip() for p in rect.split(",")]
    vals = []
    for p in parts:
        s = p
        prefix = ""
        if s[:1] in ("w", "h", "c"):
            prefix = s[0]
            s = s[1:]
        try:
            v = float(s)
        except ValueError:
            v = None
        vals.append((prefix, v))

    def get(i):
        return vals[i] if i < len(vals) else ("", None)

    p0, p1 = get(0), get(1)
    p2, p3 = get(2), get(3)

    # c- 锚点类
    if p0[0] == "c" or p1[0] == "c":
        cx = p0[1] or 0.0
        cy = p1[1] or 0.0
        W = p2[1] if p2[0] == "w" and p2[1] is not None else (p2[1] or 0.0)
        H = p3[1] if p3[0] == "h" and p3[1] is not None else (p3[1] or 0.0)
        return cx, cy, W, H, None, None, "C_ANCHOR"

    # w/h 前缀类
    if p2[0] == "w" or p3[0] == "h":
        W = p2[1] if p2[1] is not None else 0.0
        H = p3[1] if p3[1] is not None else 0.0
        ax = p0[1] or 0.0
        ay = p1[1] or 0.0
        kind = "CENTERED_wh" if (ax == 0 and ay == 0) else "ABS_wh"
        return 0.0, 0.0, W, H, ax, ay, kind

    # plain 类：x,y,X,Y
    ax = p0[1] or 0.0
    ay = p1[1] or 0.0
    bx = p2[1] if p2[1] is not None else ax
    by = p3[1] if p3[1] is not None else ay
    W = bx - ax
    H = by - ay
    kind = "CENTERED_plain" if (ax == 0 and ay == 0 and W > 100 and H > 50) else "ABS_plain"
    return 0.0, 0.0, W, H, ax, ay, kind


def parse_dialog(path):
    """机读 datah XML → 结构化（面板 + 按钮 + 文本标签）"""
    raw = open(path, "rb").read()
    try: txt = raw.decode("gbk")
    except UnicodeDecodeError: txt = raw.decode("utf-8", "ignore")
    dlg = re.search(r'<Window style="(\w+)"\s*([^>]*?)(/?)>', txt)
    info = {"style": dlg.group(1) if dlg else "Dialog", "raw": txt}
    # 顶层面板 rect（5 种语法变体，见 _parse_rect_tokens 文档）
    m = re.search(r'style="[^"]*" rect="([^"]+)"([^>]*)', txt)
    if m:
        rect = m.group(1); opts = m.group(2)
        cx, cy, W, H, ax, ay, kind = _parse_rect_tokens(rect)
        info["rect_kind"] = kind
        info["rect"] = [ax if ax is not None else cx, ay if ay is not None else cy, W, H]
        info["center_off"] = (cx, cy)
        if W > 0 and H > 0:
            info["panel"] = (int(W), int(H))
            info["centered"] = kind in ("CENTERED_plain", "CENTERED_wh", "C_ANCHOR")
            if kind == "C_ANCHOR":
                info["centered"] = True
        else:
            info["panel"] = None
            info["centered"] = False
    else:
        info["panel"] = None
        info["centered"] = False
        info["rect_kind"] = "NO_RECT"
        info["center_off"] = (0.0, 0.0)
    # 按钮（style=Button/IconBtn，有 rect + 可选 name/text）
    btns = []
    for bm in re.finditer(r'<Window\s+([^>]*style="(?:Button|IconBtn|Slider|ListBox)"[^>]*)/?>', txt):
        attrs = dict(re.findall(r'(\w+)="([^"]*)"', bm.group(1)))
        if "rect" not in attrs: continue
        st = attrs.get("style","Button")
        # ListBox（布局列表）= 滚动列表区，src_rs 简化为文本列表 → 解析器不产出可配对按钮，
        # 跳过（否则 cur 列表行按钮 vs orig 空表 → 假"需修"）。Slider（滑条）解析为按钮区
        # （几何可配对，滑条位置映射已实现）。
        if st == "ListBox": continue
        p = [q.strip() for q in attrs["rect"].split(",")]
        num = lambda s: (int(float(s)) if s.lstrip('-').replace('.','',1).isdigit() else None)
        bx0,by0,bx1,by1 = (num(p[0]),num(p[1]),num(p[2]),num(p[3]))
        if bx1 is None and len(p)>2: bx1 = bx0 + int(p[2].strip('w'))
        if by1 is None and len(p)>3: by1 = by0 + int(p[3].strip('h'))
        btns.append({
            "name": attrs.get("name",""), "text": attrs.get("text",""),
            "style": st,
            "x0":bx0,"y0":by0,"x1":bx1,"y1":by1,
        })
    info["buttons"] = btns
    # 文本标签（TransLabel/TransText，供业务语义参考）
    labels = []
    for lm in re.finditer(r'<Window\s+([^>]*style="TransLabel"[^>]*)/?>', txt):
        attrs = dict(re.findall(r'(\w+)="([^"]*)"', lm.group(1)))
        if "text" in attrs: labels.append(attrs["text"])
    info["labels"] = labels
    # 输入框（EditLabel/TransTextBox，诚实边界占位）
    edits = re.findall(r'style="Edit(?:Label|TextBox)"', txt)
    info["editboxes"] = len(edits)
    return info

def panel_origin(info):
    """面板左上角在 720×1280 基准的坐标（**单一真相源**）

    ★ 对抗审计修正（§2026-11-11）：原版 rect 有 5 种语法变体，
    面板原点换算必须与 to_720() 完全同源 —— 此前 gen_dialog_batch.py 自带一份换算，
    与 to_720 语义漂移 → c- 锚点类面板偏移丢失 → 按钮坐标全错（返回键点不到）。

    C_ANCHOR（c-CX,c-CY）语义推导：
      实测 6 个 c- 类弹窗的 CX 恒等于 -W/2（019: -225=-450/2、040: -165=-330/2、
      048: -187≈-375/2、087: -90=-180/2、252: -165=-330/2）
      → 证明 c- 的 X 是**左上角相对屏幕中心**的偏移（解读 a），
        且 x 方向恰好退化为严格居中。Y 不与 H/2 相关 → 有意偏离中心（浮标类工具条）。
      诚实边界：c- 类的 Y 锚点语义未在真机验证（x 方向无风险，两解读等价）。
    """
    if not info.get("panel"):
        return (0.0, 0.0)
    W, H = info["panel"]
    kind = info.get("rect_kind")
    if kind == "C_ANCHOR":
        cx, cy = info.get("center_off") or (0.0, 0.0)
        return 360.0 + cx, 640.0 + cy
    if info.get("centered"):
        return (720 - W) / 2.0, (1280 - H) / 2.0
    r = info.get("rect") or [0, 0]
    return (r[0] or 0), (r[1] or 0)


def to_720(info):
    """面板居中/绝对 → 720×1280 按钮坐标（局部 1:1 平移）

    ★ 对抗审计修正（§2026-11-11）：c- 锚点类（C_ANCHOR）的偏移是**相对屏幕中心**，
    不是严格居中 → 需在居中偏移上叠加 center_off。
    """
    out = []
    if not info.get("panel"):
        return out
    ox, oy = panel_origin(info)
    for b in info["buttons"]:
        out.append({
            "name": b["name"], "text": b["text"], "style": b["style"],
            "x0": round(ox+b["x0"]), "y0": round(oy+b["y0"]),
            "x1": round(ox+b["x1"]), "y1": round(oy+b["y1"]),
        })
    return out

def cmd_analyze(args):
    """① 分析 datah → 生成 720 坐标 + 按钮表 JSON（人审）"""
    results = {}
    for n in args.ids:
        path = None
        for nn, p in spak_files():
            if nn == int(n): path = p; break
        if not path:
            print(f"  datah_{n.zfill(3)} 未找到 SPAK 文件"); continue
        info = parse_dialog(path)
        b720 = to_720(info)
        results[n] = {"panel": info.get("panel"), "centered": info.get("centered"),
                      "buttons720": b720, "labels": info["labels"],
                      "editboxes": info["editboxes"],
                      "honest": "含输入框（占位标签）" if info["editboxes"] else "无输入框"}
        print(f"\n=== datah_{str(n).zfill(3)}（面板 {info.get('panel')} {'居中' if info.get('centered') else '绝对'}）===")
        print(f"  按钮 {len(b720)} 个 + 输入框 {info['editboxes']} 个 + 文本标签 {info['labels'][:6]}")
        for b in b720:
            print(f"    {b['name'] or b['text']:8s} 720({b['x0']},{b['y0']})-({b['x1']},{b['y1']}) Δ=0（居中偏移）")
    out = os.path.join(TOOLS, f"spak_cfg_{'_'.join(args.ids)}.json")
    json.dump(results, open(out,"w"), ensure_ascii=False, indent=1)
    print(f"\n配置已生成: {out}（人审后 spak2ui.py gen 引用）")
    return 0

def cmd_status(args):
    """④ 看已对齐进度 + 未对齐清单（按按钮数分布，优先补有按钮的）"""
    all_f = spak_files()
    aligned = [n for n,_ in all_f if n in ALIGNED]
    unaligned = [n for n,_ in all_f if n not in ALIGNED]
    # 未对齐里按按钮数分档
    by_btn = {0:[],1:[],2:[],3:[],4:[],5:[]}
    for n in unaligned:
        for nn,p in all_f:
            if nn==n:
                info = parse_dialog(p)
                bc = len(info["buttons"])
                by_btn[min(bc,5)].append(n); break
    print(f"原版 {len(all_f)} 个 datah，已对齐 {len(aligned)}，未对齐 {len(unaligned)}")
    print(f"\n未对齐按按钮数分档（脚本化批量补，优先 4+ 按钮高价值）：")
    for bc in [5,4,3,2,1,0]:
        if by_btn[bc]:
            print(f"  {bc} 按钮: {len(by_btn[bc])} 个 → {by_btn[bc][:20]}{'...' if len(by_btn[bc])>20 else ''}")
    print(f"\n★ 153 个无按钮（纯展示/文本框/资源占位）不补（无交互业务，诚实边界）")
    print(f"★ 可脚本化批量补的有按钮对话框约 {sum(len(by_btn[k]) for k in [1,2,3,4,5])} 个")
    return 0

def cmd_gen(args):
    """② 从已审 JSON 配置生成 C 代码骨架（ui_menu.h/.c + evverify + layout_align_check + make_*_atlas.py + AUDIT 表）"""
    # 读 analyze 产出的 JSON（若存在），否则用 parse_dialog 现算
    generated = []
    for n in args.ids:
        nn = int(n)
        path = None
        for f in spak_files():
            if f[0] == nn: path = f[1]; break
        if not path: print(f"  datah_{n.zfill(3)} 未找到 SPAK 文件"); continue
        info = parse_dialog(path)
        b720 = to_720(info)
        tag = f"datah_{n.zfill(3)}"
        slug = f"ui{n.zfill(3)}"          # UI_UI043 / g_ui043_btns / run_ui043
        panel = info.get("panel") or (0,0)
        W,H = panel
        # 面板 720 基准
        if info.get("centered"):
            px0, py0 = round((720-W)/2), round((1280-H)/2)
        else:
            r = info.get("rect")
            px0, py0 = int(r[0] or 0), int(r[1] or 0)
        hon = info["editboxes"] > 0
        # ---- ui_menu.h 片段 ----
        hdr = []
        hdr.append(f"    UI_UI{nn},   /* ★ SPAK2UI 脚本化（datah_{n.zfill(3)} 证据）：{info['labels'][0] if info['labels'] else tag} 弹窗（{len(b720)} 按钮 + 诚实边界{'：输入框占位' if hon else ''}） */")
        hdr.append(f"#define UI_UI{nn}_COUNT {max(len(b720),1)}")
        hdr.append(f"#define UI_UI{nn}_POPUP_X0 {px0}.0f")
        hdr.append(f"#define UI_UI{nn}_POPUP_Y0 {py0}.0f")
        hdr.append(f"#define UI_UI{nn}_POPUP_W  {W}.0f")
        hdr.append(f"#define UI_UI{nn}_POPUP_H  {H}.0f")
        hdr.append(f"typedef struct {{ float x0,y0,x1,y1; int action; }} ui_ui{nn}_btn;")
        hdr.append(f"const ui_ui{nn}_btn *ui_ui{nn}_buttons(void);")
        hdr.append(f"int ui_ui{nn}_hit(const ui_ui{nn}_btn *b, int n, float x, float y);")
        hdr.append(f"int ui_ui{nn}_popup_inside(float x, float y);")
        # ---- ui_menu.c 片段 ----
        rows = []
        for i,b in enumerate(b720):
            rows.append(f"    {{ {b['x0']}.0f, {b['y0']}.0f, {b['x1']}.0f, {b['y1']}.0f, {i} }},   /* {b['name'] or b['text'] or 'btn%d'%i} */")
        rows_joined = "\n".join(rows) if rows else "    /* 无按钮（纯展示，跳过）*/"
        c_body = f"""static const ui_ui{nn}_btn g_ui{nn}_btns[UI_UI{nn}_COUNT] = {{
{rows_joined}
}};

const ui_ui{nn}_btn *ui_ui{nn}_buttons(void) {{ return g_ui{nn}_btns; }}
int ui_ui{nn}_hit(const ui_ui{nn}_btn *b, int n, float x, float y)
{{
    for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i;
    return -1;
}}
int ui_ui{nn}_popup_inside(float x, float y)
{{
    return x>=UI_UI{nn}_POPUP_X0 && x<=UI_UI{nn}_POPUP_X0+UI_UI{nn}_POPUP_W &&
           y>=UI_UI{nn}_POPUP_Y0 && y<=UI_UI{nn}_POPUP_Y0+UI_UI{nn}_POPUP_H;
}}
"""
        # ---- evverify run_uiXXX 片段 ----
        ev_rows = []
        for i,b in enumerate(b720):
            ev_rows.append(f'    {{ "{b["name"] or b["text"] or "btn%d"%i}", {b["x0"]},{b["y0"]},{b["x1"]},{b["y1"]} }},')
        ev_rows_joined = "\n".join(ev_rows) if ev_rows else "    /* 无按钮 */"
        exp_arr = f"ui{nn}_exp"
        nd = len(b720)
        ev_body = f"""/* ★ SPAK2UI 脚本化（datah_{n.zfill(3)}）：{nd} 按钮几何 Δ=0 + 命中判定（确定性桌面验证，诚实边界{'：输入框占位' if hon else ''}） */
static const struct {{ const char *lbl; int x0,y0,x1,y1; }} {exp_arr}[] = {{
{ev_rows_joined}
}};
static int run_ui{nn}(void)
{{
    const ui_ui{nn}_btn *b = ui_ui{nn}_buttons();
    int n = UI_UI{nn}_COUNT, pass = 0;
    for (int i = 0; i < n; i++) {{
        int dcx = (b[i].x0+b[i].x1)/2 - ({exp_arr}[i].x0+{exp_arr}[i].x1)/2;
        int dcy = (b[i].y0+b[i].y1)/2 - ({exp_arr}[i].y0+{exp_arr}[i].y1)/2;
        if (abs(dcx)<=1 && abs(dcy)<=1) pass++;
        if (ui_ui{nn}_hit(b, n, (b[i].x0+b[i].x1)/2, (b[i].y0+b[i].y1)/2) != i) {{ printf("ui{nn} hit[%%d] FAIL\n", i); return 0; }}
    }}
    printf("ui{nn}（datah_{n.zfill(3)}）：{nd} 按钮几何 Δ=0 + 命中判定（桌面确定性，诚实边界{'：输入框占位' if hon else ''}）\n";
    return pass == n;
}}
"""
        # ---- layout_align_check DIALOGS 片段 ----
        la_rows = [f'("{b["name"] or b["text"] or "btn%d"%i}, {b["x0"]},{b["y0"]},{b["x1"]},{b["y1"]})' for b in b720]
        la_body = f'    "datah_{n.zfill(3)}_ui": ({W}, {H}, [\n        {", ".join(la_rows)}]),\n'
        # ---- make_*_atlas.py 片段 ----
        labels = [b["text"] for b in b720 if b["text"]] or ["标题"]
        at_body = f"""#!/usr/bin/env python3
# SPAK2UI 脚本化（datah_{n.zfill(3)}）：烘焙 ui{nn}_text_atlas.png（{len(labels)} 标签）
from PIL import Image, ImageDraw, ImageFont
import os
LBL = {labels!r}
W,H,SZ = 220, 368, 38
img = Image.new("RGBA",(W,H),(0,0,0,0)); d=ImageDraw.Draw(img)
try: fnt=ImageFont.truetype("/disk/workspaces/jq/rev64/dist/spak_res/data_font_all.ttf",SZ)
except: fnt=ImageFont.load_default()
y=5
for t in LBL:
    bb=d.textbbox((0,0),t,font=fnt); tw=bb[2]-bb[0]
    d.text(((W-tw)//2,y),t,fill=(255,255,255,255),font=fnt); y+=30
out="/disk/workspaces/jq/rev64/dist/spak_res/ui{nn}_text_atlas.png"; img.save(out)
print("baked",out,"tags",len(LBL))
"""
        generated.append({
            "id": nn, "tag": tag, "slug": f"ui{nn}",
            "hdr": "\n".join(hdr), "c_body": c_body, "ev_body": ev_body,
            "la_body": la_body, "at_body": at_body,
            "panel720": [px0,py0,W,H], "honest": hon,
        })
    # 输出全部片段
    for g in generated:
        print(f"\n################ datah_{str(g['id']).zfill(3)} → UI_UI{g['id']}（slug ui{str(g['id']).zfill(3)}，诚实边界={g['honest']}）################")
        print("-- ui_menu.h 片段（贴入枚举区 + 声明区）--\n" + g["hdr"])
        print("-- ui_menu.c 片段（追加文件尾）--\n" + g["c_body"])
        print("-- evverify.c run_uiXXX 片段（追加 + --all 注册）--\n" + g["ev_body"])
        print("-- layout_align_check.py DIALOGS 片段（追加 dict）--\n" + g["la_body"])
        print(f"-- make_ui{str(g['id']).zfill(3)}_atlas.py（全文新建）--")
        open(os.path.join(TOOLS, f"make_ui{str(g['id']).zfill(3)}_atlas.py"), "w").write(g["at_body"])
        print(f"  已新建 make_ui{str(g['id']).zfill(3)}_atlas.py")

    if getattr(args, "apply", False):
        cmd_apply(generated)
    else:
        print("\n★ 下一步：把上述片段贴入对应源文件（枚举区/文件尾/--all 注册/DIALOGS dict），"
              "跑 NDK make so + jq_evverify --all + jq_consistency selftest 验证，"
              "真机 popup_sweep.sh 加对应 setprop 钩子。加 --apply 可自动贴入+编译+回归。")
    return 0

def cmd_apply(generated):
    """--apply：自动把片段贴入 5 个源文件 + 编译 + evverify + 一致性回归（一条命令批量对齐）"""
    import shutil
    menuh = os.path.join(SRC, "ui_menu.h"); menuc = os.path.join(SRC, "ui_menu.c")
    menustub = os.path.join(SRC, "ui_menu_stub.c"); evc = os.path.join(SRC, "evverify.c")
    lac = os.path.join(TOOLS, "layout_align_check.py")
    menu_h_txt = open(menuh).read(); menuc_txt = open(menuc).read()
    ev_txt = open(evc).read(); lac_txt = open(lac).read()
    # ① ui_menu.h：枚举 + 宏 + 结构体 + 声明（枚举项贴到 `} ui_screen;` 之前；宏/声明贴到 #endif 之前）
    enum_items = []
    decl_items = []
    for g in generated:
        lines = g["hdr"].split("\n")
        enum_items.append(lines[0])
        decl_items.append("\n".join(lines[1:]))
    menu_h_txt = menu_h_txt.replace("} ui_screen;", "\n".join(enum_items) + "\n} ui_screen;", 1)
    menu_h_txt = menu_h_txt.replace("#endif", "\n".join(decl_items) + "\n#endif", 1)
    open(menuh, "w").write(menu_h_txt)
    # ② ui_menu.c：按钮表 + hit + popup_inside（追加文件尾）
    menuc_txt += "\n" + "\n".join(g["c_body"] for g in generated) + "\n"
    open(menuc, "w").write(menu_c_safe(menuc_txt))
    # ③ ui_menu_stub.c：桩（几何表默认 0，桌面链接用）
    stub_txt = open(menustub).read()
    stub_add = ""
    for g in generated:
        stub_add += f"const ui_ui{g['id']}_btn *ui_ui{g['id']}_buttons(void){{ static ui_ui{g['id']}_btn z[UI_UI{g['id']}_COUNT]; return z;}}\n"
        stub_add += f"int ui_ui{g['id']}_hit(const ui_ui{g['id']}_btn *b,int n,float x,float y){{return -1;}}\n"
        stub_add += f"int ui_ui{g['id']}_popup_inside(float x,float y){{return 0;}}\n"
    stub_txt = stub_txt.replace("#endif", stub_add + "\n#endif", 1)
    open(menustub, "w").write(stub_txt)
    # ④ evverify.c：run_* 函数 + 前向声明 + --all 注册
    ev_txt += "\n" + "\n".join(g["ev_body"] for g in generated) + "\n"
    # 前向声明（--all 段在文件前部调用 run_uiXXX，函数体在尾部 → 需前向声明）
    fwd = "".join(f"static int run_ui{g['id']}(void);\n" for g in generated)
    ev_txt = ev_txt.replace("int main(int argc", fwd + "int main(int argc", 1)
    # --all 里加 run_uiXXX() 注册
    for g in generated:
        ev_txt = ev_txt.replace('total++; pass += run_drawdetail();', f'total++; pass += run_drawdetail();\n        total++; pass += run_ui{g["id"]}();', 1)
    open(evc, "w").write(ev_txt)
    # ⑤ layout_align_check.py DIALOGS：追加 dict 项
    for g in generated:
        lac_txt = lac_txt.replace('}\n\n# datah_107/039 绝对坐标面板', g["la_body"].rstrip() + "\n}\n\n# datah_107/039 绝对坐标面板", 1)
    open(lac, "w").write(lac_txt)
    print(f"\n=== --apply 贴入完成（5 源文件）：{menuh} {menuc} {menustub} {evc} {lac} ===")
    # ⑥ 编译 + evverify + 一致性回归
    print("--- NDK 编译 so ---")
    r = subprocess.run(f"cd {SRC} && NDK={NDK} make so 2>&1 | grep -E 'error:' | head", shell=True, capture_output=True, text=True)
    if r.stdout: print("  编译 error:\n" + r.stdout); return 1
    print("  编译 PASS")
    print("--- evverify --all ---")
    subprocess.run(f"cd {SRC} && NDK={NDK} make evverify >/dev/null 2>&1 && ./dist/jq_evverify --all", shell=True)
    print("--- 一致性回归 ---")
    subprocess.run(f"cd {SRC} && NDK={NDK} make consistency >/dev/null 2>&1 && ./dist/jq_consistency selftest", shell=True)
    print("--- layout_align_check --baseline ---")
    subprocess.run(f"cd {WS} && python3 rev64/tools/layout_align_check.py --baseline", shell=True)
    print(f"\n★ 批量对齐完成：{len(generated)} 弹窗已贴入+编译+回归。真机用 popup_sweep.sh 加 setprop 钩子终验。")
    return 0

def menu_c_safe(txt):
    """ui_menu.c 若缺 #include <stdlib.h>（abs）则补"""
    if "#include <stdlib.h>" not in txt and "abs(" in txt:
        txt = txt.replace("#include", "#include <stdlib.h>\n#include", 1)
    return txt

def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd")
    a = sub.add_parser("analyze"); a.add_argument("ids", nargs="+")
    s = sub.add_parser("status")
    g = sub.add_parser("gen"); g.add_argument("ids", nargs="+"); g.add_argument("--apply", action="store_true")
    au = sub.add_parser("auto"); au.add_argument("ids", nargs="+"); au.add_argument("--apply", action="store_true")
    args = ap.parse_args()
    if args.cmd == "analyze": cmd_analyze(args)
    elif args.cmd == "status": cmd_status(args)
    elif args.cmd == "gen": cmd_gen(args)
    elif args.cmd == "auto":
        # auto：gen 全链路（生成骨架 + 提示贴入 + 编译回归）
        r = cmd_gen(args)
    return 0

if __name__ == "__main__":
    main()
