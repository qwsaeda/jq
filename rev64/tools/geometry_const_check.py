#!/usr/bin/env python3
"""geometry_const_check.py — ui.rs 几何常量 vs datah XML 真值自动核对（防常量漂移）

从原版 datah_*.bin 的 XML 提取面板 rect + 控件 rect，算 720 基准真值，
与 src_rs_rust/src/ui.rs 的 *_BTNS_ORIG / *_PANEL 常量逐项比对（容差 ≤1px 取整误差）。
让"UI 几何对齐"从人工核对变成可复跑判据。
用法：python3 rev64/tools/geometry_const_check.py
"""
import glob
import os
import re
import sys

ROOT = "/disk/workspaces/jq"
SPAK = f"{ROOT}/rev64/dist/spak_res"
UI_RS = f"{ROOT}/rev64/forward/src_rs_rust/src/ui.rs"
TOL = 1  # 居中偏移 .5 取整误差


def datah(name):
    fs = glob.glob(f"{SPAK}/{name}_*.bin")
    return open(fs[0], "rb").read().decode("gbk", errors="ignore") if fs else ""


def panel_offset(t):
    m = re.search(r'<Window[^>]*rect="0,0,(\d+),(\d+)"[^>]*center', t)
    if not m:
        return None
    W, H = int(m.group(1)), int(m.group(2))
    return (720 - W) // 2, (1280 - H) // 2, W, H


def controls(t):
    """返回 {name: (x0,y0,x1,y1)} 局部坐标"""
    out = {}
    for m in re.finditer(r'<Window([^>]*)>', t):
        a = m.group(1)
        nm = re.search(r'name="([^"]+)"', a)
        rc = re.search(r'rect="([\d,]+)"', a)
        if nm and rc:
            v = [int(x) for x in rc.group(1).split(",")]
            if len(v) == 4:
                out[nm.group(1)] = tuple(v)
    return out


def rs_const(name):
    """从 ui.rs 提取 pub const NAME = [...] 或 Rect{...}"""
    s = open(UI_RS).read()
    m = re.search(r'pub const ' + name + r'\s*:[^=]*=\s*\[(.*?)\];', s, re.S)
    if m:
        # 只匹配纯数字元组 (d,d,d,d)，跳过注释里的括号
        return [tuple(int(x) for x in g.split(',')) for g in re.findall(r'\(\s*([\d\s,]+)\s*\)', m.group(1))]
    m = re.search(r'pub const ' + name + r'\s*:[^=]*=\s*Rect\s*\{\s*x:\s*(\d+),\s*y:\s*(\d+),\s*w:\s*(\d+),\s*h:\s*(\d+)', s)
    if m:
        x, y, w, h = map(int, m.groups())
        return [(x, y, x + w, y + h)]
    return None


def check(label, got, want):
    if got is None:
        print(f"  FAIL {label}: ui.rs 常量缺失"); return 1
    if len(got) != len(want):
        print(f"  FAIL {label}: 数量 {len(got)} != 真值 {len(want)}"); return 1
    bad = 0
    for i, (g, w) in enumerate(zip(got, want)):
        if any(abs(a - b) > TOL for a, b in zip(g, w)):
            print(f"  FAIL {label}[{i}]: 常量{g} != 真值{w}"); bad += 1
    if not bad:
        print(f"  PASS {label}: {len(got)} 项几何对齐（容差 ≤{TOL}px）")
    return bad


def main():
    failed = 0
    # 难度 datah_041
    t = datah("datah_041"); ox, oy, _, _ = panel_offset(t); c = controls(t)
    want = [(ox+x0, oy+y0, ox+x1, oy+y1) for x0,y0,x1,y1 in
            [c[k] for k in ['LowLay','LowFlip','MidLay','MidFlip','HighLay','HighFlip','RankLay','RankFlip']]]
    failed += check("DIFF_BTNS_ORIG (datah_041)", rs_const("DIFF_BTNS_ORIG"), want)

    # 游戏设置 datah_012
    t = datah("datah_012"); ox, oy, _, _ = panel_offset(t); c = controls(t)
    want = [(ox+x0, oy+y0, ox+x1, oy+y1) for x0,y0,x1,y1 in
            [c[k] for k in ['Draw','Command','BingLei','BingQi','Ok','Cancel']]]
    failed += check("GAMESET_BTNS_ORIG (datah_012)", rs_const("GAMESET_BTNS_ORIG"), want)

    # 退出确认 datah_025（Confirm=退出 Ok, Cancel=继续）
    t = datah("datah_025"); ox, oy, _, _ = panel_offset(t); c = controls(t)
    want_quit = (ox+c['Confirm'][0], oy+c['Confirm'][1], ox+c['Confirm'][2], oy+c['Confirm'][3])
    want_stay = (ox+c['Cancel'][0], oy+c['Cancel'][1], ox+c['Cancel'][2], oy+c['Cancel'][3])
    failed += check("EXIT_BTN_QUIT (datah_025 Confirm)", rs_const("EXIT_BTN_QUIT"), [want_quit])
    failed += check("EXIT_BTN_STAY (datah_025 Cancel)", rs_const("EXIT_BTN_STAY"), [want_stay])

    # 声音设置 datah_001
    t = datah("datah_001"); ox, oy, _, _ = panel_offset(t); c = controls(t)
    want = [(ox+x0, oy+y0, ox+x1, oy+y1) for x0,y0,x1,y1 in
            [c[k] for k in ['Sound','BtnSound','Volume','Ok','Cancel']]]
    failed += check("SETTINGS_BTNS_ORIG (datah_001)", rs_const("SETTINGS_BTNS_ORIG"), want)

    # 新局 datah_062
    t = datah("datah_062"); ox, oy, _, _ = panel_offset(t); c = controls(t)
    want = [(ox+x0, oy+y0, ox+x1, oy+y1) for x0,y0,x1,y1 in
            [c[k] for k in ['Ok','Cancel','Lay','Show']]]
    failed += check("NEWGAME_BTNS_ORIG (datah_062)", rs_const("NEWGAME_BTNS_ORIG"), want)

    print(f"判定: {'FAIL' if failed else 'PASS'}（{failed} 项几何常量偏差）")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
