#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""全量弹窗对齐审计（对抗审计批 2026-11-11）
用 spak2ui 单一真相源（panel_origin）重算全部已对齐 datah 弹窗的 720 基准几何，
与 ui_menu.h/.c 现实现逐按钮比对。Δ > 1px 即报"未对齐"。
"""
import sys, os, re, json
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
from spak2ui import parse_dialog, spak_files, panel_origin

H = open(os.path.join(ROOT, "src_rs/ui_menu.h"), encoding="utf-8").read()
C = open(os.path.join(ROOT, "src_rs/ui_menu.c"), encoding="utf-8").read()

ROW = re.compile(r"\{\s*([\d.]+)f?\s*,\s*([\d.]+)f?\s*,\s*([\d.]+)f?\s*,\s*([\d.]+)f?\s*[,}]")
TBL = re.compile(r'static const \S+\s+(g_\w+_btns)\s*\[[^\]]*\]\s*=\s*\{')

def all_btn_tables():
    tables = {}
    pos = 0
    for m in TBL.finditer(C):
        tbl = m.group(1)
        end = C.find('};', m.end())
        seg = C[m.end():end]
        # 跳过注释行干扰：逐行匹配
        rows = []
        for ln in seg.splitlines():
            rm = ROW.search(ln)
            if rm:
                rows.append(tuple(float(x) for x in rm.groups()))
        tables[tbl] = rows
    return tables

TABLES = all_btn_tables()
print(f"== ui_menu.c 抓到 {len(TABLES)} 个按钮表（总行数 {sum(len(v) for v in TABLES.values())}）==")
for k in ("g_way_btns", "g_gameset_btns", "g_set_btns"):
    print(f"  {k}: {len(TABLES.get(k, []))} 行")

def macros(name):
    out = {}
    for k in ("X0", "Y0", "W", "H"):
        m = re.search(r"#define\s+" + re.escape(name) + r"_POPUP_" + k + r"\s+([\d.]+)", H)
        out[k] = float(m.group(1)) if m else None
    return out if all(v is not None for v in out.values()) else None

ALIASES = {
    "UI_SETTINGS": "g_set_btns", "UI_WAYSEL": "g_way_btns",
    "UI_IMENU": "g_imen_btns", "UI_NPLAYER": "g_nplayer_btns",
    "UI_NEWGAME": "g_newgame_btns", "UI_GAMESET": "g_gameset_btns",
    "UI_NEWGAME104": "g_newgame104_btns", "UI_JOIN065": "g_join065_btns",
    "UI_CREATE069": "g_create069_btns", "UI_RULES070": "g_rules070_btns",
    "UI_ROOM060": "g_room060_btns", "UI_CHAT063": "g_chat063_btns",
    "UI_NG061": "g_ng061_btns", "UI_SETEXTTRY": "g_setentry_btns",
    "UI_SETEXTTRY4": "g_setentry4_btns", "UI_IMGSET7": "g_imgset7_btns",
    "UI_IMGSET8": "g_imgset8_btns", "UI_JOIN": "g_join_btns",
    "UI_CREATE": "g_create_btns", "UI_ABOUT": "g_about_btns",
    "UI_BRIGHT": "g_bright_btns", "UI_CHATLOG": "g_chatlog_btns",
    "UI_PROFILE": "g_profile_btns", "UI_DRAWDETAIL": "g_drawdetail_btns",
    "UI_GAMEINFO": "g_gameinfo_btns", "UI_LOADLAYOUT": "g_loadlayout_btns",
    "UI_SAVELAYOUT": "g_savelayout_btns", "UI_LAYMGR": "g_laymgr_btns",
    "UI_REPLAY_CTRL": "g_replay_ctrl_btns", "UI_LOGIN": "g_login_btns",
    "UI_RULES": "g_rules_btns", "UI_DIFF": "g_diff_btns",
    "UI_EXITCONFIRM": "g_exitconfirm_btns",
}

def find_table_for(name):
    if name in ALIASES and ALIASES[name] in TABLES:
        return ALIASES[name]
    stem = name[3:]
    low = stem.lower()
    cand = f"g_{low}_btns"
    if cand in TABLES: return cand
    m = re.match(r"UI_D(\d+)", name)
    if m:
        cand = f"g_d{m.group(1)}_btns"
        if cand in TABLES: return cand
    m = re.match(r"UI_UI(\d+)", name)
    if m:
        cand = f"g_ui{m.group(1)}_btns"
        if cand in TABLES: return cand
    return None

# UI 宏名 → datah 号 权威映射（以 ui_menu.h 枚举/宏区注释为唯一来源，
# 人工核过：每个枚举项注释里写死 datah_NNN；宏名与枚举名可能不同
# （如 UI_SETTINGS 宏 vs UI_D001 别名共享 datah_001 面板）。
DATAH_MAP = {
    "UI_SETTINGS": 1, "UI_D001": 1, "UI_BRIGHT": 2, "UI_D002": 2,
    "UI_WAYSEL": 72, "UI_D072": 72, "UI_DIFF": 41, "UI_D041": 41,
    "UI_NPLAYER": 89, "UI_D089": 89, "UI_NEWGAME": 62, "UI_D062": 62,
    "UI_GAMESET": 12, "UI_D012": 12, "UI_D061": 61, "UI_NG061": 61,
    "UI_LOADLAYOUT": 17, "UI_D017": 17, "UI_SAVELAYOUT": 20, "UI_D020": 20,
    "UI_LAYMGR": 16, "UI_D016": 16, "UI_REPLAY_CTRL": 107, "UI_D107": 107,
    "UI_LOGIN": 38, "UI_D038": 38, "UI_RULES": 37, "UI_D037": 37,
    "UI_NEWGAME104": 104, "UI_D104": 104, "UI_JOIN": 34, "UI_D034": 34,
    "UI_CREATE": 35, "UI_D035": 35, "UI_JOIN065": 65, "UI_D065": 65,
    "UI_CREATE069": 69, "UI_D069": 69, "UI_ROOM060": 60, "UI_D060": 60,
    "UI_RULES070": 70, "UI_D070": 70, "UI_IMGSET7": 8, "UI_D008": 8,
    "UI_IMGSET8": 8, "UI_ABOUT": 0, "UI_D000": 0,
    "UI_BRIGHT": 2, "UI_CHATLOG": 29, "UI_D029": 29,
    "UI_PROFILE": 31, "UI_D031": 31, "UI_DRAWDETAIL": 42, "UI_D042": 42,
    "UI_SETEXTTRY": 3, "UI_D003": 3, "UI_SETEXTTRY4": 4, "UI_D004": 4,
    "UI_CHAT063": 63, "UI_D063": 63, "UI_GAMEINFO": 50, "UI_D050": 50,
    "UI_D005": 5, "UI_D006": 6, "UI_D007": 7, "UI_D010": 10,
    "UI_D013": 13, "UI_D015": 15, "UI_D019": 19, "UI_D021": 21,
    "UI_D022": 22, "UI_D023": 23, "UI_D024": 24, "UI_D025": 25,
    "UI_D026": 26, "UI_D027": 27, "UI_D028": 28, "UI_D030": 30,
    "UI_D032": 32, "UI_D033": 33, "UI_D036": 36, "UI_D039": 39,
    "UI_D043": 43, "UI_D044": 44, "UI_D045": 45, "UI_D046": 46,
    "UI_D047": 47, "UI_D049": 49, "UI_D051": 51, "UI_D052": 52,
    "UI_D053": 53, "UI_D054": 54, "UI_D055": 55, "UI_D057": 57,
    "UI_D058": 58, "UI_D059": 59, "UI_D064": 64, "UI_D066": 66,
    "UI_D067": 67, "UI_D068": 68, "UI_D071": 71, "UI_D073": 73,
    "UI_D075": 75, "UI_D077": 77, "UI_D078": 78, "UI_D079": 79,
    "UI_D082": 82, "UI_D083": 83, "UI_D084": 84, "UI_D085": 85,
    "UI_D086": 86, "UI_D090": 90, "UI_D094": 94, "UI_D095": 95,
    "UI_D096": 96, "UI_D097": 97, "UI_D098": 98, "UI_D099": 99,
    "UI_D100": 100, "UI_D101": 101, "UI_D102": 102, "UI_D108": 108,
    "UI_D109": 109, "UI_D110": 110, "UI_D112": 112, "UI_D113": 113,
    "UI_D114": 114, "UI_D118": 118, "UI_D119": 119, "UI_D120": 120,
    "UI_D121": 121, "UI_D122": 122, "UI_D123": 123, "UI_D246": 246,
    "UI_D247": 247, "UI_D248": 248, "UI_D249": 249, "UI_D250": 250,
    "UI_D251": 251, "UI_D253": 253, "UI_D255": 255, "UI_D256": 256,
    "UI_D257": 257, "UI_D258": 258, "UI_D259": 259, "UI_D260": 260,
    "UI_D261": 261, "UI_D262": 262, "UI_D263": 263, "UI_D264": 264,
    "UI_UI44": 44, "UI_UI45": 45, "UI_UI46": 46, "UI_UI47": 47,
    "UI_UI49": 49, "UI_UI51": 51, "UI_UI54": 54, "UI_UI73": 73,
    "UI_UI90": 90, "UI_UI100": 100, "UI_UI110": 110, "UI_UI118": 118,
    "UI_IMENU": 6, "UI_D006": 6,
    # UI_EXITCONFIRM/UI_RESIGNCONFIRM/UI_REGRETCONFIRM：非 datah 弹窗
    #（原版 .rodata 事件名 ExitConfirm 等对话框未逆向，几何"对照 datah_032"
    # 只是参考系不是同一弹窗；脚本里 datah_032 = 390×60 工具条，不应对比 400×280 确认弹窗）
    "UI_IMENU": 6,
}

def enum_to_datah(name):
    return DATAH_MAP.get(name)

# 非 datah 真弹窗 / 已人工核实的诚实边界占位（不进"需修"清单，单独统计）
KNOWN_NONDATAH = {
    "UI_IMENU": "☰ 菜单=原版实测 orig_ingame_menu.png 900×1600 ×0.8（用户截图指正），非 datah_006 XML 定义；datah_006 450×285 为另一套菜单弹窗，不冲突",
    "UI_EXITCONFIRM": "原版 .rodata ExitConfirm 事件名对话框，XML 未逆向（几何对照 datah_032 参考系，非同一弹窗）",
}
HONEST_BOUNDARY = {
    "UI_IMGSET7": "第 4 开关'对方棋子'（action=3）非 datah_008 原版控件（原版只有 3 开关 Trans/PieceAni/Night），为旧实现扩展；几何已按 450×390 面板纠正，按钮 0-2/Ok/Cancel 全部 Δ=0，仅 action=3 保留原位区间（不挡 Ok/Cancel），不删以保 ui_imgset_opp 业务链（渲染参数=UI 状态记录，诚实边界）",
    "UI_JOIN065": "datah_065 8 项=联机开局配置（src_rs 无联机开局逻辑）→ 占位标签+边界对话框（不画真数据，不包装已实现）",
    "UI_LOADLAYOUT": "ListBox 滚动未实现（触摸 flaky）→ 6 条文本截断+选中高亮，无取消按钮对齐（原版 017 无 Cancel 主按钮，仅列表区）",
    "UI_LOGIN": "账号系统未逆向（服务器协议）→ 用户名/密码=占位标签，游客=本地单机唯一真生效路径",
    "UI_SETTINGS": "音量滑条未画真滑条（触摸 flaky）→ 低/中/高 3 档循环切换（诚实边界已标注）",
    "UI_BRIGHT": "滑条未画真滑条 → 低/中/高 3 档循环切换（诚实边界）",
    "UI_CREATE": "创建游戏=联机房间协议未逆向 → 边界对话框（诚实边界）",
    "UI_JOIN": "加入游戏=联机房间协议未逆向 → 边界对话框（诚实边界）",
    "UI_ROOM060": "房间列表=联机协议未逆向 → 边界对话框（诚实边界）",
    "UI_GAMEINFO": "对局信息 Info 文本=☰ 战况统计复用，删除=清复盘记录简化（原版删除对局存档未逆向）",
}
names = sorted(set(re.findall(r"#define (UI_\w+)_POPUP_X0", H)))
print(f"== 共 {len(names)} 个弹窗宏组 ==")
problems, skipped = [], 0
for name in names:
    dh = enum_to_datah(name)
    if dh is None:
        skipped += 1
        continue
    sp = None
    for n, p in spak_files():
        if n == dh: sp = p; break
    if not sp: continue
    info = parse_dialog(sp)
    m = macros(name)
    if not m: continue
    ox, oy = panel_origin(info)
    pW, pH = (info.get("panel") or (0, 0))
    if not pW:
        skipped += 1
        continue
    dx = abs(m["X0"] - round(ox)); dy = abs(m["Y0"] - round(oy))
    dW = abs(m["W"] - pW); dH = abs(m["H"] - pH)
    tbl = find_table_for(name)
    cur = TABLES.get(tbl) if tbl else None
    orig = [[round(ox + b["x0"]), round(oy + b["y0"]), round(ox + b["x1"]), round(oy + b["y1"])]
            for b in info["buttons"]]
    issues = []
    if dx > 1 or dy > 1 or dW > 1 or dH > 1:
        issues.append(f"panel cur({m['X0']},{m['Y0']},{m['W']},{m['H']}) "
                      f"orig({round(ox)},{round(oy)},{pW},{pH}) "
                      f"Δx{dx:.1f} Δy{dy:.1f} ΔW{dW:.1f} ΔH{dH:.1f}")
    if cur is not None:
        # ★ 人工主按钮映射（跳过原版 XML 里位置重合的备用隐藏按钮 Ok2/Cancel2/Close1 等，
        #   它们与主按钮重叠，cur 只实现主按钮 → 贪心配对会被干扰误报）。
        #   键=datah 号，值=cur 按钮应配对的 orig index（按 cur 顺序）。未列出的走贪心兜底。
        # 原版 XML 主按钮 index（cur 应配对的主按钮，跳过位置重合的备用 Ok2/Cancel2/Close1）
        MAIN_BTN = {
            16:  [0,1,2,3],   # d016/d016(laymgr)：Rename/Ok/Delete/Cancel（跳 Ok2/Cancel2）
            25:  [1],          # d025：Confirm（跳 Ok/Cancel 备用）
            50:  [0,1,2],      # d050：View/Remove/Close（跳 Close1）
            71:  [0,1],        # d071：Search/Cancel（跳 Cancel1）
            86:  [0,1,2,3],   # d086：Rename/Ok/Delete/Cancel（跳 Ok2/Cancel2）
            95:  [0,1,2,3,4], # d095：5 个实现
            108: [0,1,2],     # d108：View/Remove/Close（跳 Close1）
            260: [0,1,2,3,4], # d260：5 个实现
        }
        # 人工核对确认为"业务扩展按钮"的 cur index（不参与几何对齐判据，
        # 滑条/输入框占位/暗子预览/联机配置项等，诚实边界）：
        EXT_BTN = {
            2:  [0],          # d002 亮度滑条（触摸 flaky 简化为循环切换）
            35: [0],          # d035 创建游戏密码占位标签（无输入框）
            34: [0],          # d034 加入游戏名称占位标签（无输入框）
            65: [0,1,2,3,4,5,6,7],  # d065 联机开局配置 8 项占位（src_rs 无联机开局逻辑）
            17: [0,1,2,3,4,5],      # d017 布局列表 6 项（ListBox 简化为文本列表）
            38: [0,1],        # d038 登录用户名/密码占位标签（无输入框）
            1:  [2],          # d001 音量滑条（触摸 flaky 简化为循环切换）
        }
        main = MAIN_BTN.get(dh)
        ext = EXT_BTN.get(dh, [])
        if main is not None:
            for k in range(min(len(cur), len(main))):
                if k in ext:
                    continue
                o_ = orig[main[k]]
                dd = max(abs(cur[k][0]-o_[0]), abs(cur[k][1]-o_[1]),
                         abs(cur[k][2]-o_[2]), abs(cur[k][3]-o_[3]))
                if dd > 1:
                    issues.append(f"btn{cur[k][0]:.0f} 配 orig{main[k]}({o_[0]},{o_[1]},{o_[2]},{o_[3]}) Δ={dd:.0f}")
            # 扩展按钮（ext）单独标注为诚实边界，不算几何缺陷
            for k in ext:
                if k < len(cur):
                    issues.append(f"ext按钮{cur[k][0]:.0f}@{cur[k][1]:.0f}（业务扩展/占位，非几何缺陷，诚实边界）")
        else:
            # 贪心最优配对（每个 cur 配一个 orig，曼哈顿升序；备用按钮"未配"不报）
            dists = []
            for ci, c_ in enumerate(cur):
                for oi, o_ in enumerate(orig):
                    d = abs(c_[0]-o_[0]) + abs(c_[1]-o_[1]) + abs(c_[2]-o_[2]) + abs(c_[3]-o_[3])
                    dists.append((d, ci, oi))
            dists.sort()
            used_o = set(); pair_dd = {}
            for d, ci, oi in dists:
                if oi in used_o: continue
                used_o.add(oi)
                pair_dd[ci] = (max(abs(cur[ci][0]-orig[oi][0]), abs(cur[ci][1]-orig[oi][1]),
                                   abs(cur[ci][2]-orig[oi][2]), abs(cur[ci][3]-orig[oi][3])), oi)
            for ci in range(len(cur)):
                dd, oi = pair_dd.get(ci, (9999, None))
                if dd > 2:
                    o_ = orig[oi] if oi is not None else (0,0,0,0)
                    issues.append(f"btn{ci} cur({cur[ci][0]},{cur[ci][1]},{cur[ci][2]},{cur[ci][3]}) "
                                  f"≠ 配到 orig{oi}({o_[0]},{o_[1]},{o_[2]},{o_[3]}) Δ={dd:.0f}")
        # 去重
        seen = set(); uniq = []
        for x in issues:
            if x not in seen:
                seen.add(x); uniq.append(x)
        issues = uniq
    elif tbl is None:
        issues.append(f"按钮表未找到（UI 宏 {name}）")
    if issues:
        problems.append({"ui": name, "datah": dh, "table": tbl, "issues": issues})

print(f"\n== 有问题 {len(problems)} 组，跳过 {skipped}（无 datah 注释/无面板解析）==")
for p in problems:
    print(f"  ✗ {p['ui']} (datah_{p['datah']:03d}, 表={p['table']})")
    for s in p["issues"][:8]:
        print("      ", s)

# 分类：需修（真误配/硬错位） vs 诚实边界（占位不修） vs 非 datah（实测基准）
need_fix, honest, nondatah = [], [], []
for p in problems:
    if p["ui"] in KNOWN_NONDATAH:
        nondatah.append(p); continue
    if p["ui"] in HONEST_BOUNDARY:
        honest.append(p); continue
    need_fix.append(p)
print(f"\n══ 需修（真误配/硬错位）{len(need_fix)} ══")
for p in need_fix:
    print(f"  ✗ {p['ui']} (dh_{p['datah']:03d})")
    for i in p["issues"]: print("     ", i)
print(f"\n══ 诚实边界占位（不修，已标注）{len(honest)} ══")
for p in honest:
    print(f"  · {p['ui']} (dh_{p['datah']:03d}): {HONEST_BOUNDARY[p['ui']][:40]}...")
print(f"\n══ 非 datah 实测基准（不修）{len(nondatah)} ══")
for p in nondatah:
    print(f"  · {p['ui']}: {KNOWN_NONDATAH[p['ui']][:40]}...")
json.dump({"need_fix": need_fix, "honest": honest, "nondatah": nondatah},
          open(os.path.join(ROOT, "audit/r2/UI_ALIGN_PROB.json"), "w"),
          ensure_ascii=False, indent=1)
print(f"\n→ audit/r2/UI_ALIGN_PROB.json（需修 {len(need_fix)} / 诚实边界 {len(honest)} / 非datah {len(nondatah)}）")
