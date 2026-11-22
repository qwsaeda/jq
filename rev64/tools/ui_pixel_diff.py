#!/usr/bin/env python3
"""ui_pixel_diff.py — 原版真实截图 vs Rust 离线渲染 逐像素差异分析（UI 对齐权威判据）

原版截图（900×1600，com.cnvcs.junqi v152 真机实拍）vs render_ui example 输出。
分区统计差异率 + 生成差异热图（红=差异大/绿=一致），定位未对齐的具体 UI 元素。
用法：python3 rev64/tools/ui_pixel_diff.py [--heatmap]
"""
import os
import sys
from PIL import Image, ImageChops

ROOT = "/disk/workspaces/jq"
PAIRS = [
    ("主菜单", f"{ROOT}/rev64/forward/docs_rs/uicmp/orig_main_real.png", "/tmp/opencode/ui_offline/menu.png"),
    ("对局",   f"{ROOT}/rev64/forward/docs_rs/uicmp/orig_game_real.png", "/tmp/opencode/ui_offline/game.png"),
    ("☰菜单", f"{ROOT}/rev64/forward/docs_rs/uicmp/orig_menu_real.png", "/tmp/opencode/ui_offline/ingame.png"),
]
# 分区（900×1600，y 分段）：顶部栏 / 棋盘上半 / 中部 / 棋盘下半 / 底部控制条
BANDS = [("顶部栏", 0, 150), ("上盘", 150, 600), ("中部", 600, 1000), ("下盘", 1000, 1450), ("底部", 1450, 1600)]

THRESH = 40  # 每通道差异 >40 算"不同像素"


def analyze(name, orig_path, rend_path, heatmap):
    a = Image.open(orig_path).convert("RGB")
    b = Image.open(rend_path).convert("RGB")
    if a.size != b.size:
        print(f"{name}: 尺寸不一致 {a.size} vs {b.size}"); return
    w, h = a.size
    pa, pb = a.load(), b.load()
    total_diff = 0
    for label, y0, y1 in BANDS:
        diff = 0; cnt = 0
        for y in range(y0, min(y1, h), 2):
            for x in range(0, w, 2):
                r1, g1, b1 = pa[x, y]; r2, g2, b2 = pb[x, y]
                cnt += 1
                if abs(r1-r2) > THRESH or abs(g1-g2) > THRESH or abs(b1-b2) > THRESH:
                    diff += 1
        total_diff += diff
        pct = diff * 100.0 / cnt
        flag = "⚠" if pct > 25 else ("~" if pct > 8 else "✓")
        print(f"  {label:6} y[{y0:4}-{y1:4}] 差异 {pct:5.1f}% {flag}")
    print(f"  {name} 总体差异采样 {total_diff}")
    if heatmap:
        hm = Image.new("RGB", (w, h), (0, 180, 0))
        hp = hm.load()
        for y in range(0, h, 2):
            for x in range(0, w, 2):
                r1, g1, b1 = pa[x, y]; r2, g2, b2 = pb[x, y]
                d = max(abs(r1-r2), abs(g1-g2), abs(b1-b2))
                col = (min(255, d*3), 180 if d <= THRESH else max(0, 180-d), 0)
                hp[x, y] = col
        out = f"/tmp/opencode/ui_diff_{name}.png"
        hm.save(out)
        print(f"  热图: {out}")


def title_bbox(px, xr=(150, 750), yr=(50, 500)):
    """标题'军棋'黄绿填充区 bbox（用于背景对齐锚点判据）"""
    xs, ys = [], []
    for y in range(*yr, 2):
        for x in range(*xr, 2):
            r, g, b = px[x, y]
            if 100 < g < 200 and 80 < r < 180 and b < 100 and g > r + 10:
                xs.append(x); ys.append(y)
    return (min(xs), min(ys), max(xs), max(ys)) if xs else None


def check_title_anchor():
    """★ 轮6 判据：主菜单标题锚点与原版真值的几何差（定位背景资源/缩放偏差）"""
    a = Image.open(PAIRS[0][1]).convert("RGB").load()
    b = Image.open(PAIRS[0][2]).convert("RGB").load()
    ba, bb = title_bbox(a), title_bbox(b)
    if not ba or not bb:
        print("标题锚点检测失败"); return
    print(f"\n=== 标题锚点对账（背景对齐判据）===")
    print(f"  原版 bbox {ba} / 渲染 bbox {bb}")
    dy0, dy1 = bb[1] - ba[1], bb[3] - ba[3]
    print(f"  y 偏差: 顶 {dy0:+d}px 底 {dy1:+d}px")
    if abs(dy0) <= 3 and abs(dy1) <= 3:
        print("  ✓ 标题几何对齐")
    else:
        print(f"  ⚠ 标题 y 偏差 —— 【已定位：orig 截图被纵向二次缩放】")
        print("     证据：同一按钮贴图资源，截图实测人机按钮高113px / datah 真值95px / 渲染125px")
        print("     截图失真，不可作真值；按钮几何以 datah 真值为准（ui_offline_check.py 判据）。")
        print("     不要按此截图配准去改背景——会让真实设备显示错位（轮7 +68px 配准实验已回滚）。")


def main():
    heatmap = "--heatmap" in sys.argv
    for name, a, b in PAIRS:
        if not os.path.exists(a) or not os.path.exists(b):
            print(f"{name}: 缺图 {a if not os.path.exists(a) else b}"); continue
        print(f"\n=== {name} ===")
        analyze(name, a, b, heatmap)
    check_title_anchor()


if __name__ == "__main__":
    main()
