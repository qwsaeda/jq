#!/usr/bin/env python3
"""gen_dialog_batch.py — 原版 datah 对话框批量对齐生成器（脚本化）

用法:
  python3 tools/gen_dialog_batch.py --list 22 77 82          # 只打印解析结果
  python3 tools/gen_dialog_batch.py 22 77 82                 # 生成代码 + 烘焙文字图集 + 插入 src_rs

产物:
  rev64/dist/spak_res/dlg_batch_text.png   共用文字图集（所有弹窗按钮文字竖排）
  rev64/src_rs/ui_menu.h / ui_menu.c / ui_menu_stub.c   枚举 + 按钮表 + hit
  rev64/src_rs/render.h / render.c                        纹理字段 + rs_draw_* + 帧调度
  rev64/src_rs/jni_bridge.c                               OnTouch 分发 + setprop 钩子
  rev64/src_rs/evverify.c                                 run_* 几何 Δ=0 + 命中
  rev64/tools/layout_align_check.py                       DIALOGS 几何

几何规则（与既有已对齐弹窗一致）:
  面板 rect "0,0,W,H"（center=true）→ 720 基准偏移 ((720-W)/2,(1280-H)/2)，局部 1:1 平移
  面板 rect "x,y,wW,hH"（固定位置）  → 720 基准用原 x,y（不居中偏移），局部 1:1 平移
文字: 原版 XML 的按钮 text，用原版字体 data_font_all.ttf 烘焙进共用图集（行高 40，字形宽按实测记录）
诚实边界: 业务无法逆向者 → 边界对话框 + 明确标注，不包装成"已实现"
"""
import os,re,sys,glob,shutil
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
from spak2ui import parse_dialog, to_720

ROOT="/disk/workspaces/jq"
RS=f"{ROOT}/rev64/src_rs"
SPAK=f"{ROOT}/rev64/dist/spak_res"
SPAK_OUT="/tmp/opencode/spak_out"
ATLAS=f"{SPAK}/dlg_batch_text.png"
FONT=f"{SPAK}/data_font_all.ttf"

# 业务映射（按钮文字 → 行为）；未命中 → 边界对话框（诚实边界）
REUSE_UI = {  # 按钮文字 → 已有弹窗（点击直接切过去，复用已对齐业务）
 "对局信息":"UI_GAMEINFO","功能设置":"UI_GAMESET","软件信息":"UI_ABOUT",
 "游戏规则":"UI_RULES","规则":"UI_RULES","复盘":"UI_REPLAY",
 "菜单":"UI_INGAME_MENU","新局":"UI_NEWGAME","聊天":"UI_CHAT063",
 "载入":"UI_LOAD_LAY","保存":"UI_SAVE_LAY","Rename":"UI_LAY_MGR",
 "回退":"UI_REPLAY","前进":"UI_REPLAY","跳转":"UI_REPLAY",
}
SPECIAL = {  # 按钮文字 → 特殊业务（直接动作）
 "悔棋":"undo_pop();ui_screen_set(UI_GAME); LOGI(\"批量对齐：悔棋 → undo_pop（已悔步）\");",
 "清空":"ui_replay_init(); ui_screen_set(UI_GAME); LOGI(\"批量对齐：清空 → 清复盘记录（★ 诚实边界：原版清的是本弹窗列表存档，未逆向）\");",
 "删除":"ui_replay_init(); ui_screen_set(UI_GAME); LOGI(\"批量对齐：删除 → 清复盘记录（★ 诚实边界：原版删的是存档项，未逆向）\");",
 "完成":"ui_screen_set(UI_GAME); LOGI(\"批量对齐：完成 → 回对局\");",
 "确定":"ui_screen_set(UI_GAME); LOGI(\"批量对齐：确定 → 回对局\");",
 "取消":"ui_screen_set(UI_GAME); LOGI(\"批量对齐：取消 → 回对局\");",
 "返回":"ui_screen_set(UI_GAME); LOGI(\"批量对齐：返回 → 回对局\");",
 "离开":"ui_screen_set(UI_MENU); LOGI(\"批量对齐：离开 → 回主菜单\");",
 "投降":"g_state.winner = (g_state.turn == SIDE_BLUE) ? SIDE_RED : SIDE_BLUE; g_state.stage = STG_OVER; ui_screen_set(UI_GAME); LOGI(\"批量对齐：投降 → 本机败（winner=%d）\", g_state.winner);",
 "提和":"jq_show_dialog(env, \"提和（诚实边界）\", \"提和：原版需对端应答，src_rs 本地未实现提和判定。\"); ui_screen_set(UI_GAME);",
 "取消托管":"jq_show_dialog(env, \"取消托管（诚实边界）\", \"取消托管：原版 AI 托管通道未逆向。\"); ui_screen_set(UI_GAME);",
 "开局":"jq_show_dialog(env, \"开局（诚实边界）\", \"开局：原版开局操作通道未逆向。\"); ui_screen_set(UI_GAME);",
 "终局":"jq_show_dialog(env, \"终局（诚实边界）\", \"终局：原版终局操作通道未逆向。\"); ui_screen_set(UI_GAME);",
 "切换显示":"jq_show_dialog(env, \"切换显示（诚实边界）\", \"切换显示：原版棋盘显示切换通道未逆向。\"); ui_screen_set(UI_GAME);",
 "Expand":"jq_show_dialog(env, \"Expand（诚实边界）\", \"Expand：原版设置扩展项未逆向。\"); ui_screen_set(UI_GAME);",
 "Clockwise":"jq_show_dialog(env, \"Clockwise（诚实边界）\", \"Clockwise：原版旋转设置未逆向。\"); ui_screen_set(UI_GAME);",
 "Chat":"jq_show_dialog(env, \"Chat（诚实边界）\", \"Chat：原版聊天设置项未逆向。\"); ui_screen_set(UI_GAME);",
 "四暗":"ui_screen_set(UI_DIFFICULTY); LOGI(\"批量对齐：四暗 → 难度选择（datah_098 布局规则）\");",
 "双明":"ui_screen_set(UI_DIFFICULTY); LOGI(\"批量对齐：双明 → 难度选择（datah_098 布局规则）\");",
}
BOUNDARY_TEXT = {  # 明确业务未逆向 → 边界对话框（诚实标注）
 "网络":"联网通道未逆向","排行":"排行榜未逆向","详细":"详细数据未逆向",
 "注册":"注册服务未逆向","登录":"登录服务未逆向","消息":"消息服务未逆向",
 "更新":"版本更新通道未逆向","好友":"好友对战未逆向","四国":"四国规则未逆向",
}

def datah_path(n):
    g=glob.glob(f"{SPAK_OUT}/datah_{n:03d}_*.bin")
    return g[0] if g else None

def load_batches(nums):
    """解析 → 每弹窗：panel/buttons(720)/文本行"""
    from PIL import ImageFont
    out=[]
    font_cache={}
    def font(sz):
        if sz not in font_cache: font_cache[sz]=ImageFont.truetype(FONT,sz)
        return font_cache[sz]
    rows=[]      # (text, width_px, height_px)
    for n in nums:
        p=datah_path(n)
        if not p: print(f"  ⚠ datah_{n:03d} 无文件，跳过"); continue
        info=parse_dialog(p)
        if not info.get("panel"):
            print(f"  ⚠ datah_{n:03d} 无面板 rect，跳过"); continue
        # ★ 规则 1（横条型）：panel 高 ≤ 120 → 先按原版 rect 去重互斥按钮组（同规则 2），
        #   再用原版真实 x 坐标。均分整条宽仅作兜底（去重后坐标无效/全挤一处时）。
        #   ★ 坑（本轮 datah_095）：旧版对 strip 直接按 9 按钮均分 → 原版实为 5 个独立槽
        #     （复盘/聊天同位、提和/取消托管/离开/返回同位），9 等分既错位又重复。
        # ★ 规则 2：其余情况去掉与前面按钮 rect 相交 >30% 者（原版互斥按钮组，只留首个）
        strip = bool(info.get("panel")) and info["panel"][1] <= 120
        raw_items=list(info.get("buttons", []))
        # ── 互斥去重（strip / 非 strip 共用）
        ded=[]
        for b in raw_items:
            ov=False
            for k in ded:
                ix0,iy0=max(b["x0"],k["x0"]),max(b["y0"],k["y0"])
                ix1,iy1=min(b["x1"],k["x1"]),min(b["y1"],k["y1"])
                if ix1>ix0 and iy1>iy0:
                    inter=(ix1-ix0)*(iy1-iy0)
                    a1=(b["x1"]-b["x0"])*(b["y1"]-b["y0"]) or 1
                    a2=(k["x1"]-k["x0"])*(k["y1"]-k["y0"]) or 1
                    if inter/min(a1,a2) > 0.3:
                        ov=True; break
            if not ov: ded.append(b)
        if strip and ded:
            # 坐标有效性：x 需铺开（最大跨度 ≥ 面板宽 60%）且各槽不全等宽
            xs=[b["x1"] for b in ded]; x0s=[b["x0"] for b in ded]
            span=(max(xs)-min(x0s)) if xs else 0
            Wp,Hp=info["panel"]
            if span < Wp*0.6 or len(set((b["x0"],b["x1"]) for b in ded)) < 2:
                # 兜底：按去重后槽位均分整条宽
                n_btn=len(ded); bw=Wp/float(n_btn)
                ded=[{"name":b["name"],"text":b["text"],"style":b["style"],
                      "x0":int(round(i*bw)),"y0":0,
                      "x1":int(round((i+1)*bw)),"y1":Hp} for i,b in enumerate(ded)]
            else:
                # 原版坐标有效 → y 归一到面板高（x 保留原版真值）
                ded=[{**b,"y0":0,"y1":info["panel"][1]} for b in ded]
        raw_items=ded
        if not raw_items:
            print(f"  ⚠ datah_{n:03d} 去重后无按钮，跳过"); continue
        items=[]
        for b in raw_items:
            txt=b["text"] or b["name"] or ""
            # 估算字形宽（720 基准，原版按钮文字约 30.5/字；用原版字体量真实宽）
            if txt:
                f=font(38)
                try: bb=f.getbbox(txt); wpx=bb[2]-bb[0]; hpx=bb[3]-bb[1]
                except Exception: wpx=len(txt)*30; hpx=38
            else:
                wpx=0; hpx=0
            if info.get("centered"):
                ox,oy=(720-info["panel"][0])/2.0,(1280-info["panel"][1])/2.0
            else:
                rr=info.get("rect") or [0,0]
                ox,oy=(rr[0] or 0),(rr[1] or 0)
            items.append({"text":txt,"name":b["name"],"x0":round(ox+b["x0"]),"y0":round(oy+b["y0"]),
                          "x1":round(ox+b["x1"]),"y1":round(oy+b["y1"]),"w":0})
            items[-1]["w"]=round(wpx*0.62)  # 38px 字高 ≈ 720 基准 30px 高，字宽 ×0.62
            items[-1]["h"]=round(hpx*0.62) if hpx else 0
            rows.append(txt)
        out.append({"n":n,"panel":info["panel"],"centered":bool(info.get("centered")),
                    "items":items,"labels":info.get("labels",[])})
    return out

def bake_atlas(rows):
    """烘焙共用文字图集（竖排，行高 40）；返回 (图集宽, 每行字形宽列表)"""
    from PIL import Image,ImageDraw,ImageFont
    W=1
    for t in rows:
        if t: W=max(W,len(t)*46)
    W=((W+3)//4)*4
    H=len(rows)*40
    im=Image.new("RGBA",(W,H),(0,0,0,0))
    d=ImageDraw.Draw(im)
    f=ImageFont.truetype(FONT,38)
    widths=[]; heights=[]
    for i,t in enumerate(rows):
        y=i*40
        if not t: widths.append(0); heights.append(0); continue
        bb=d.textbbox((0,0),t,font=f)
        w=bb[2]-bb[0]; h=bb[3]-bb[1]
        widths.append(bb[2]-bb[0])       # ★ 图集内原始像素宽（运行时按目标字高等比缩）
        heights.append(bb[3]-bb[1])      # ★ 图集内原始像素高
        d.text((2, y+(40-(bb[3]-bb[1]))//2-bb[1]), t, font=f, fill=(255,255,255,255))
    os.makedirs(os.path.dirname(ATLAS),exist_ok=True)
    im.save(ATLAS)
    return W,H,widths,heights

def gen_code(batches,widths,heights):
    """生成各文件代码片段（返回 dict: 文件 → 插入文本 + 锚点）"""
    H={}; HDEF={}; C={}; STUB={}; RH={}; R={}; J={}; E={}; L={}
    row=0
    for b in batches:
        n=b["n"]; tag=f"D{n:03d}"; sid=f"UI_{tag}"
        cnt=len(b["items"])
        W,Hh=b["panel"]
        if b["centered"]: ox,oy=(720-W)/2,(1280-Hh)/2
        else: ox,oy=0,0
        # 面板偏移（固定位置类从 rect 取）——parse_dialog 的 to_720 已用同样规则算按钮，
        # 面板宏需与之一致：这里重算
        # （to_720 对 centered 用居中偏移；非 centered 用 rect 原点 → 需回读 rect）
        # 简化：面板 X0/Y0 直接由按钮最小包围盒反推不合适 → 用 spak2ui 解析结果
        info=parse_dialog(datah_path(n))
        r=info.get("rect")
        if b["centered"]: px0,py0=round((720-W)/2),round((1280-Hh)/2)
        elif r and r[0] and r[1]: px0,py0=round(r[0]),round(r[1])
        else: px0,py0=round((720-W)/2),round((1280-Hh)/2)
        # ── ui_menu.h
        H[tag]=f"""
    {sid},   /* ★ 批量对齐（datah_{n:03d} 证据）：{b['labels'][0] if b['labels'] else '原版对话框'}
                       面板 {W}×{Hh}（{'居中' if b['centered'] else '固定位置'}），{cnt} 按钮
                       文字/业务按原版 XML text + .rodata 事件名，边界项已标注 */"""
        HDEF[tag]=f"""
#define {sid}_COUNT {cnt}
#define {sid}_POPUP_X0 {px0}.0f
#define {sid}_POPUP_Y0 {py0}.0f
#define {sid}_POPUP_W  {W}.0f
#define {sid}_POPUP_H  {Hh}.0f
typedef struct {{ float x0,y0,x1,y1; int action; }} ui_{tag.lower()}_btn;
const ui_{tag.lower()}_btn *ui_{tag.lower()}_buttons(void);
int ui_{tag.lower()}_hit(const ui_{tag.lower()}_btn *b, int n, float x, float y);
"""
        # ── ui_menu.c 按钮表
        tbl=[]
        for i,it in enumerate(b["items"]):
            lbl=(it["text"] or it["name"] or f"B{i}")[:20]
            tbl.append(f'    {{ {it["x0"]}.0f, {it["y0"]}.0f, {it["x1"]}.0f, {it["y1"]}.0f, {i} }},  /* {lbl} */')
        C[tag]=f"""
/* ★ 批量对齐（datah_{n:03d}）：{cnt} 按钮（720 基准，局部 1:1 平移） */
static const ui_{tag.lower()}_btn g_{tag.lower()}_btns[{sid}_COUNT] = {{
{chr(10).join(tbl)}
}};
const ui_{tag.lower()}_btn *ui_{tag.lower()}_buttons(void) {{ return g_{tag.lower()}_btns; }}
int ui_{tag.lower()}_hit(const ui_{tag.lower()}_btn *b, int n, float x, float y)
{{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }}"""
        STUB[tag]=C[tag]
        # ── render.h 字段（共用文字图集 + 每弹窗文字行偏移表在 C 里）
        RH[tag]=f"""
    /* ★ 批量对齐 datah_{n:03d}：文字行在共用图集中的行号 {row}..{row+cnt-1} */"""
        row+=cnt
        # ── render.c 绘制
        # ★ per-button 字形像素宽高数组（坑：f-string 里 {widths[...+i]} 的 i 泄漏自上一个
        #   Python 循环 → 全按钮共用同一宽度 → 文字被压成窄竖条。必须显式用 j 展开成字面量。）
        _r0 = row - cnt
        pw = ",".join(str(widths[_r0 + j] if _r0 + j < len(widths) else 0) for j in range(cnt))
        ph = ",".join(str(heights[_r0 + j] if _r0 + j < len(heights) else 0) for j in range(cnt))
        R[tag]=f"""
/* ★ 批量对齐（datah_{n:03d}）：{cnt} 按钮 + 原版按钮文字（共用图集 dlg_batch_text.png）
   ★ 文字不变形三要素（本轮 datah_095 文字被压成窄竖条实证后修正）：
     1) 每按钮独立字形像素宽高（{tag}_pw/ph 数组，非块级常量——旧版 i 泄漏成常量）
     2) 图集行高固定 40（bake_atlas 行距）→ v = (行号*40)/图集高；n_rows 不再传 cnt
     3) u1 = 字形像素宽/图集宽（旧版传 1.0 = 整行 416px 压进字宽）
   目标字高 30px（720 基准），宽按字形 bbox 比例等比缩放。 */
static const int {tag}_pw[{sid}_COUNT] = {{ {pw} }};
static const int {tag}_ph[{sid}_COUNT] = {{ {ph} }};
static void rs_draw_{tag.lower()}(rs_render *rr, int slot)
{{
    GLint uc = glGetUniformLocation(rr->prog_solid[slot], "uColor");
    rs_popup_shell(rr, slot, {sid}_POPUP_X0, {sid}_POPUP_Y0, {sid}_POPUP_W, {sid}_POPUP_H, 0, uc);
    const ui_{tag.lower()}_btn *bb = ui_{tag.lower()}_buttons();
    for (int i = 0; i < {sid}_COUNT; i++) {{
        rs_solid_btn(rr, slot, bb[i].x0, bb[i].y0, bb[i].x1, bb[i].y1,
                     0.22f, 0.28f, 0.42f, 0.85f, 0, uc);
        int gr = {_r0} + i;   /* 共用图集行号（bake_atlas 顺序） */
        int wpx = {tag}_pw[i], hpx = {tag}_ph[i];
        if (wpx > 0 && hpx > 0 && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {{
            float scale = 30.0f / (float)hpx;   /* 目标字高 30px（720 基准），宽等比 */
            float tw = (float)wpx * scale, th = 30.0f;
            float u1 = ((float)wpx + 1.0f) / (float)rr->dlg_text_w;
            rs_atlas_label_wh(rr, slot, rr->tex_dlg_text[slot], rr->dlg_text_w, rr->dlg_text_h,
                              gr, (bb[i].x0 + bb[i].x1) * 0.5f, (bb[i].y0 + bb[i].y1) * 0.5f,
                              tw, th, 0.0f, u1, uc);
        }}
    }}
    if (!{tag.lower()}_diag) {{ {tag.lower()}_diag = 1;
        LOGI("rs_draw_{tag.lower()}：datah_{n:03d} 弹窗（{W}×{Hh} 面板 + {cnt} 按钮，原版按钮文字）");
    }}
}}"""
        # ── jni_bridge 分发
        acts=[]
        for i,it in enumerate(b["items"]):
            txt=it["text"] or it["name"] or f"按钮{i}"
            reuse=REUSE_UI.get(txt)
            spec=SPECIAL.get(txt)
            if reuse:
                acts.append(f'            }} else if (a == {i}) {{   /* {txt} → {reuse} */\n'
                            f'                ui_screen_set({reuse}); sfx_play(env, "pop");')
            elif spec:
                acts.append(f'            }} else if (a == {i}) {{   /* {txt} → 业务动作 */\n'
                            f'                {spec} sfx_play(env, "pop");')
            else:
                acts.append(f'            }} else if (a == {i}) {{   /* {txt} → 边界对话框（诚实边界） */\n'
                            f'                jq_show_dialog(env, "{txt}（诚实边界）",\n'
                            f'                    "{txt}：原版业务未逆向，src_rs 无此通道。");\n'
                            f'                ui_screen_set(UI_GAME); sfx_play(env, "pop");')
        J[tag]=f"""
    /* ★ 批量对齐（datah_{n:03d}）：{cnt} 按钮业务分发 */
    if (ui_screen_get() == {sid}) {{
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_{tag.lower()}_btn *bb = ui_{tag.lower()}_buttons();
        int bh = ui_{tag.lower()}_hit(bb, {sid}_COUNT, bx, by);
        if (bh >= 0) {{
            int a = bb[bh].action;
            LOGI("OnTouch datah_{n:03d} 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {{
{chr(10).join(acts)}
            }}   /* 收尾：末分支闭合 */
        }}
        return JNI_TRUE;
    }}"""
        # ── evverify
        exp=[f'    {{ "{it["text"] or it["name"]}", {it["x0"]}, {it["y0"]}, {it["x1"]}, {it["y1"]} }},'
             for it in b["items"]]
        E[tag]=f"""
/* ★ 批量对齐（datah_{n:03d}）：{cnt} 按钮几何 Δ=0 + 命中判定 */
int run_{tag.lower()}(void)
{{
    const ui_{tag.lower()}_btn *b = ui_{tag.lower()}_buttons();
    static const struct {{ const char *lbl; int x0,y0,x1,y1; }} exp[] = {{
{chr(10).join(exp)}
    }};
    int n = {sid}_COUNT, ok = 1;
    for (int i = 0; i < n; i++) {{
        if (b[i].x0 != exp[i].x0 || b[i].y0 != exp[i].y0 ||
            b[i].x1 != exp[i].x1 || b[i].y1 != exp[i].y1) {{
            ok = 0;
            printf("  %s btn%d 几何不符 src(%.0f,%.0f)-(%.0f,%.0f) 期望(%d,%d)-(%d,%d)\\n",
                   exp[i].lbl, i, b[i].x0, b[i].y0, b[i].x1, b[i].y1,
                   exp[i].x0, exp[i].y0, exp[i].x1, exp[i].y1);
        }}
        float cx = (b[i].x0 + b[i].x1) * 0.5f, cy = (b[i].y0 + b[i].y1) * 0.5f;
        int hit = ui_{tag.lower()}_hit(b, n, cx, cy);
        ok &= (hit == i);
    }}
    printf("datah_{n:03d}（{W}×{Hh} {'居中' if b['centered'] else '固定'}）：{cnt} 按钮几何 Δ=0 + 命中判定%s\\n",
           ok ? "PASS" : "FAIL");
    return ok;
}}"""
        # ── layout_align_check DIALOGS
        L[tag]=f"""    "datah_{n:03d}_dlg": ({W}, {Hh}, ["""
        for it in b["items"]:
            L[tag]+=f'("{it["text"] or it["name"]}",{it["x0"]},{it["y0"]},{it["x1"]},{it["y1"]}),'
        L[tag]+="]),\n"
    return H,HDEF,C,STUB,RH,R,J,E,L,row

def insert(path, anchor, text, before=True):
    s=open(path).read()
    if "★ 批量对齐（datah_" in s and text.strip().startswith(("UI_D","/* ★ 批量对齐","    {sid}")):
        # 幂等：同批次标记已存在则跳过
        import re as _re
        m=_re.search(r'datah_(\d{3})', text)
        if m and f'datah_{m.group(1)}' in s:
            return False
    if not s.count(anchor): return False
    s=s.replace(anchor, (text+anchor) if before else (anchor+text), 1)
    open(path,"w").write(s)
    return True

def clean():
    """删除本生成器插入的所有片段（模板迭代后重新生成用）。
    只删带 ★ 批量对齐 / dlg_batch_text / jq_dlg_batch 标记的片段，不动其它代码。"""
    import re as _re
    RS_ = RS
    def sub(path, pat, repl="", flags=_re.S, label=""):
        try: s = open(path).read()
        except FileNotFoundError: return 0
        s2, n = _re.subn(pat, repl, s, flags=flags)
        if n: open(path, "w").write(s2)
        return n
    tot = 0
    # ── ui_menu.h：枚举块 + 宏块
    tot += sub(f"{RS_}/ui_menu.h",
               r'\n    UI_D\d{3},   /\* ★ 批量对齐（datah_\d{3} 证据）.*?\*/', "", _re.S)
    tot += sub(f"{RS_}/ui_menu.h",
               r'\n(?:#define UI_D\d{3}_COUNT \d+\n)?(?:#define UI_D\d{3}_POPUP_\w+\s+[-\d.]+f\n|typedef struct \{ float x0,y0,x1,y1; int action; \} ui_d\d{3}_btn;\n|const ui_d\d{3}_btn \*ui_d\d{3}_buttons\(void\);\n|int ui_d\d{3}_hit\(const ui_d\d{3}_btn \*b, int n, float x, float y\);[ \t]*\n)+')
    # ── ui_menu.c / stub：按钮表 + 访问器
    for f in ("ui_menu.c", "ui_menu_stub.c"):
        tot += sub(f"{RS_}/{f}",
                   r'\n/\* ★ 批量对齐（datah_\d{3}）：\d+ 按钮（720 基准，局部 1:1 平移） \*/.*?return -1; \}',
                   "", _re.S)
    # ── render.h：图集字段（兼容正常换行 + 同行 append 残留两种形态）
    tot += sub(f"{RS_}/render.h",
               '\n    rs_glid_t tex_dlg_text\[2\];[^\n]*?void \*dlg_text_px; size_t dlg_text_px_len; int dlg_text_w, dlg_text_h;\n')
    tot += sub(f"{RS_}/render.h", '    rs_glid_t tex_dlg_text\[2\];[^\n]*\n')
    tot += sub(f"{RS_}/render.h", '    void \*dlg_text_px; size_t dlg_text_px_len; int dlg_text_w, dlg_text_h;\n')
    tot += sub(f"{RS_}/render.h",
               'rs_glid_t tex_dlg_text\[2\];[^\n]*?void \*dlg_text_px; size_t dlg_text_px_len; int dlg_text_w, dlg_text_h;')
    # 兜底：clean 前 append 到同一行的两个孤立片段
    tot += sub(f"{RS_}/render.h", r'void \*dlg_text_px; size_t dlg_text_px_len; int dlg_text_w, dlg_text_h;')
    tot += sub(f"{RS_}/render.h", r'rs_glid_t tex_dlg_text\[2\];   /\* ★ 批量对齐：共用按钮文字图集（dlg_batch_text\.png） \*/')
    tot += sub(f"{RS_}/render.h", '\n    /\* ★ 批量对齐 datah_\d{3}：[^\n]*\*/')
    # ── render.c：diag 变量 / 加载 / 上传 / 绘制函数 / 帧调度
    tot += sub(f"{RS_}/render.c", r'(?:static int d\d{3}_diag = 0;\n)+')
    # ★ 迭代清同行/跨行残留（旧版 p104/dlgraw 等 append 到同一行 → 单条正则删不完）
    import re as _re2
    for _round in range(30):
        _rc=open(f"{RS_}/render.c").read(); _prev=_rc
        _rc=_re2.sub(r'char p104\[512\];', '', _rc)
        _rc=_re2.sub(r'snprintf\(p104, sizeof p104, "%s/dlg_batch_text\.png"[^;]*;', '', _rc)
        _rc=_re2.sub(r'rs_read_png\(am, "spak_res/dlg_batch_text\.png", p104, &dlgraw, &dlgraw_len\);', '', _rc)
        _rc=_re2.sub(r'unsigned char \*dlgraw = NULL; size_t dlgraw_len = 0;', '', _rc)
        _rc=_re2.sub(r'unsigned char \*dlgpx=NULL; int dlgw=0, dlgh=0;', '', _rc)
        _rc=_re2.sub(r'if \(dlgraw && png_decode\(dlgraw, dlgraw_len, &dlgpx, &dlgw, &dlgh\) == 0\) \{[\s\S]*?free\(dlgraw\);', '', _rc)
        _rc=_re2.sub(r'if \(rr->dlg_text_px && rr->gl2_ok\[slot\][\s\S]*?\n    \}\n(?=\n)', '', _rc)
        if _rc==_prev: break
        open(f"{RS_}/render.c","w").write(_rc); tot+=1
    tot += sub(f"{RS_}/render.c",
               r'(?:\n/\* ★ 批量对齐（datah_\d{3}）：\d+ 按钮 \+ 原版按钮文字.*?\n\}(?=static void rs_draw_about|\n/\* ★ 批量对齐))+',
               "", _re.S)
    tot += sub(f"{RS_}/render.c",
               r'\n        \} else if \(ui_screen_get\(\) == UI_D\d{3}\) \{\n            rs_draw_d\d{3}\(rr, slot\);')
    # ── jni_bridge.c：钩子表+函数 / 钩子调用 / 业务分发
    tot += sub(f"{RS_}/jni_bridge.c",
               r'\n/\* ★ 批量对齐 setprop 钩子表.*?\n\}\n(?=\n#include "ui_menu\.h")', "", _re.S)
    tot += sub(f"{RS_}/jni_bridge.c", r'\n    jq_dlg_batch_hook\("(?:OnCreate|OnUpdate)"\);')
    tot += sub(f"{RS_}/jni_bridge.c",
               r'(?:\n    /\* ★ 批量对齐（datah_\d{3}）：\d+ 按钮业务分发 \*/.*?\n        return JNI_TRUE;\n    \})+',
               "", _re.S)
    # 钩子表 + 钩子函数（表驱动，重复 append 必炸 → clean 必须删干净）
    tot += sub(f"{RS_}/jni_bridge.c",
               '\n/\* ★ 批量对齐 setprop 钩子表.*?\n\}\n(?=#include)',
               "", _re.S)
    # ── evverify.c：run_* + 注册行
    tot += sub(f"{RS_}/evverify.c", r'\n        total\+\+; pass \+= run_d\d{3}\(\);')
    tot += sub(f"{RS_}/evverify.c",
               r'\n/\* ★ 批量对齐（datah_\d{3}）：\d+ 按钮几何 Δ=0 \+ 命中判定 \*/.*?\n    return ok;\n\}',
               "", _re.S)
    # ── layout_align_check.py：DIALOGS 条目
    tot += sub(f"{ROOT}/rev64/tools/layout_align_check.py",
               r'\n    "datah_\d{3}_dlg": \([^)]*\),', "", _re.S)
    print(f"--clean：删除 {tot} 处生成片段")
    return 0

def main():
    if "--clean" in sys.argv:
        return clean()
    args=[a for a in sys.argv[1:] if not a.startswith("--")]
    if not args: print("用法: gen_dialog_batch.py <datah 编号...>"); return 1
    nums=[int(a) for a in args]
    batches=load_batches(nums)
    if not batches: return 1
    rows=[]; 
    for b in batches:
        for it in b["items"]: rows.append(it["text"] or it["name"])
    W,HIMG,widths,heights=bake_atlas(rows)
    print(f"文字图集 {ATLAS} {W}×{HIMG}（{len(rows)} 行，原版字体 data_font_all.ttf）")
    Hh,HDEF,C,STUB,RH,R,J,E,L,total=batches and gen_code(batches,widths,heights)
    # 插入各文件
    uh=f"{RS}/ui_menu.h"; uc=f"{RS}/ui_menu.c"; us=f"{RS}/ui_menu_stub.c"
    rh=f"{RS}/render.h"; rc=f"{RS}/render.c"; jb=f"{RS}/jni_bridge.c"; ev=f"{RS}/evverify.c"
    la=f"{ROOT}/rev64/tools/layout_align_check.py"
    # ui_menu.h：枚举插入（UI_ABOUT 后）+ 宏+声明插入（UI_ABOUT_COUNT 后）
    enums="".join(Hh[k] for k in Hh)
    defs="".join(HDEF[k] for k in HDEF)
    insert(uh,"    UI_DRAWDETAIL,",enums)
    insert(uh,"#define UI_ABOUT_COUNT 2",defs+"\n")
    for f,tbl in (("ui_menu.c",C),("ui_menu_stub.c",STUB)):
        p=f"{RS}/{f}"
        # ★ 幂等：本批次任一 datah 标记已在文件里 → 跳过（否则重复 append = 重复符号）
        if any(f"★ 批量对齐（datah_{k[1:]}）" in open(p).read() for k in tbl):
            continue
        open(p,"a").write("".join(tbl[k] for k in tbl))
    # render.h：共用文字图集字段（一次）+ 诊断变量
    insert(rh,"    rs_glid_t tex_imen_text[2];",
        "    rs_glid_t tex_dlg_text[2];   /* ★ 批量对齐：共用按钮文字图集（dlg_batch_text.png） */\n"
        "    void *dlg_text_px; size_t dlg_text_px_len; int dlg_text_w, dlg_text_h;\n")
    # render.c：加载 + 上传 + 绘制函数 + 帧调度
    diag="".join(f"static int {k.lower()}_diag = 0;\n" for k in R)
    insert(rc,"static int panraw_seen = 0;", diag)
    insert(rc,'        snprintf(p9, sizeof p9, "%s/imen_icons_atlas.png", spak_dir ? spak_dir : "");',
        '        char p104[512];\n        snprintf(p104, sizeof p104, "%s/dlg_batch_text.png", spak_dir ? spak_dir : "");\n')
    insert(rc,'        rs_read_png(am, "spak_res/imen_icons_atlas.png", p9, &icraw, &icraw_len);',
        '        rs_read_png(am, "spak_res/dlg_batch_text.png", p104, &dlgraw, &dlgraw_len);\n')
    _dlgload = ("        if (dlgraw && png_decode(dlgraw, dlgraw_len, &dlgpx, &dlgw, &dlgh) == 0) {\n"
        "            rr->dlg_text_px = dlgpx; rr->dlg_text_px_len = (size_t)dlgw * dlgh * 4;\n"
        "            rr->dlg_text_w = dlgw; rr->dlg_text_h = dlgh;\n"
        "            LOGI(\"批量对齐文字图集加载 OK：dlg_batch_text.png %dx%d（" + str(total) + " 行）\", dlgw, dlgh);\n"
        "        }\n        free(dlgraw);\n")
    insert(rc,"        free(icraw); free(ioraw);", _dlgload)
    insert(rc,"        unsigned char *ioraw = NULL; size_t ioraw_len = 0;",
        "        unsigned char *dlgraw = NULL; size_t dlgraw_len = 0;\n"
        "        unsigned char *dlgpx=NULL; int dlgw=0, dlgh=0;\n")
    # GL 上传
    insert(rc,"    /* ★ 原版 datax 图标真值图集（16 图标 4×4）纹理上传 */",
        "    if (rr->dlg_text_px && rr->gl2_ok[slot] && rr->dlg_text_w > 0 && rr->dlg_text_h > 0) {\n"
        "        glGenTextures(1, &rr->tex_dlg_text[slot]);\n"
        "        glBindTexture(GL_TEXTURE_2D, rr->tex_dlg_text[slot]);\n"
        "        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rr->dlg_text_w, rr->dlg_text_h, 0,\n"
        "                     GL_RGBA, GL_UNSIGNED_BYTE, rr->dlg_text_px);\n"
        "        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);\n"
        "        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);\n"
        "    }\n")
    # 绘制函数（放在 rs_draw_about 前）+ 帧调度
    rfn="".join(R[k] for k in R)
    insert(rc,"static void rs_draw_about(rs_render *rr, int slot)", rfn)
    cases="".join(
        f"        }} else if (ui_screen_get() == UI_{k}) {{\n"
        f"            rs_draw_{k.lower()}(rr, slot);\n" for k in R)
    insert(rc,"        } else if (ui_screen_get() == UI_ABOUT) {", cases)
    # jni_bridge 分发（★ 追加到 OnTouch 末尾：OnTouch switch 内 UI_ABOUT 后面跟的是
    # 其它 if（如 UI_CHAT063），没有 "if (ui_screen_get() == UI_ABOUT)" 可插 → 锚点失配
    # → 旧版生成后 jni_bridge 漏分发块，编译必炸，但当时被"其它弹窗正常"掩盖）。
    # 用锚点 "jq_dlg_batch_hook(\"OnUpdate\")" 之后的 return/收尾处不可靠 → 直接找
    # OnTouch 函数最后一段的 "return JNI_FALSE;" 前插入（追加式，幂等标记 ★ 批量对齐）。
    jf="".join(J[k] for k in J)
    # 在 "return JNI_FALSE;\n}   /* 结束 OnTouch" 前插；找不到则回退到 UI_ABOUT 锚点
    s_jb=open(jb).read()
    tail_anchor='    return JNI_FALSE;'
    if "★ 批量对齐（datah_" in s_jb:
        print(f"    (jni_bridge 已有批量对齐块，跳过分发追加)")
    else:
        pos=s_jb.rfind(tail_anchor)   # ★ rfind（最后一处=OnTouch 末尾的 return）
        if pos>=0:
            s_jb=s_jb[:pos] + jf + "\n" + s_jb[pos:]
            open(jb,"w").write(s_jb)
            print(f"    jni_bridge 分发追加于末处 return JNI_FALSE 前（{len(jf)} 字符）")
        else:
            insert(jb,"    if (ui_screen_get() == UI_ABOUT) {", jf)
    # ★ setprop 钩子（表驱动，prop = jq.dlgNNN，1=进弹窗）
    hooks="\n".join(f'    {{ "jq.dlg{k[1:]}", UI_{k} }},' for k in J)
    hooktbl=f'''
/* ★ 批量对齐 setprop 钩子表（prop=jq.dlgNNN → UI_DNNN，1=进弹窗/0=退）。
   真机渲染验证入口（触摸命中 flaky 时用 setprop 触发）。 */
static const struct {{ const char *prop; int scr; }} g_dlg_batch_hooks[] = {{
{hooks}
}};
#define DLG_BATCH_HOOK_N ((int)(sizeof(g_dlg_batch_hooks)/sizeof(g_dlg_batch_hooks[0])))
void jq_dlg_batch_hook(const char *tag)
{{
    char dbp[16] = {{0}};
    __system_property_get("jq.dlgbatch", dbp);
    if (!dbp[0]) return;
    for (int hi = 0; hi < DLG_BATCH_HOOK_N; hi++) {{
        char cur[16] = {{0}};
        __system_property_get(g_dlg_batch_hooks[hi].prop, cur);
        if (strcmp(cur, "1") == 0) {{
            ui_screen_set(g_dlg_batch_hooks[hi].scr);
            __android_log_print(ANDROID_LOG_INFO, "junqi_rs",
                "%s 批量对齐钩子：%s=1 → UI_%d（datah_%s 弹窗渲染验证）",
                 tag, g_dlg_batch_hooks[hi].prop, g_dlg_batch_hooks[hi].scr, g_dlg_batch_hooks[hi].prop + 7);
            __system_property_set(g_dlg_batch_hooks[hi].prop, "0");
            break;
        }}
    }}
}}
'''
    insert(jb,'#include "ui_menu.h"', hooktbl)   # ★ 钩子表+函数定义放在 ui_menu.h 之后（枚举可见 + 调用点之前）
    # OnCreate / OnUpdate 各插一次调用
    insert(jb,"    /* ★ ☰ 菜单渲染验证钩子（adb setprop jq.imen 1 → OnCreate/OnUpdate 直接进",
            '    jq_dlg_batch_hook("OnCreate");\n')
    insert(jb,"    /* ★ ☰ 菜单渲染验证钩子（同会话 OnUpdate 轮询 → UI_INGAME_MENU） */",
            '    jq_dlg_batch_hook("OnUpdate");\n')
    # evverify：run_* + 注册
    ef="".join(E[k] for k in E)
    open(ev,"a").write(ef)
    reg="".join(f"        total++; pass += run_{k.lower()}();\n" for k in E)
    insert(ev,'        printf("=== 桌面事件验证: %d/%d 全对 ===\\n", pass, total);', reg)
    # layout_align_check DIALOGS
    ltxt="".join(L[k] for k in L)
    insert(la,"DIALOGS = {", ltxt)
    print(f"已插入：ui_menu.h/c/stub + render.h/c + jni_bridge.c + evverify.c + layout_align_check.py")
    print(f"弹窗 {len(batches)} 个：{[b['n'] for b in batches]}")
    return 0

if __name__=="__main__":
    sys.exit(main())
