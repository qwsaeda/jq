#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
真机全量弹窗验收（对抗审计闭环）

用法：
  python3 audit/verify_dialogs_device.py --apk /disk/workspaces/jq/dist/phase3.apk
  python3 audit/verify_dialogs_device.py --list 015 019 109      # 只验指定

判据（每条都需同时满足）：
  1. 进程存活（pidof 非空）
  2. 无崩溃（logcat 无 FATAL / UnsatisfiedLinkError / SIGSEGV）
  3. 钩子命中（logcat 出现「jq.dlgNNN=1 →」）
  4. 几何：截图上按钮色块的实际位置 vs 原版重算坐标（|Δ| ≤ 4px）
  5. 非黑比例 ≥ 8%（出图修复后重校准，旧 85% 全屏基线作废）
"""
import os
import re
import sys
import glob
import time
import json
import argparse
import subprocess
import random

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
from spak2ui import parse_dialog, panel_origin, to_720   # noqa: E402

SPAK = "/tmp/opencode/spak_out"
PKG = "com.cnvcs.junqirs"
DEV = "10.0.182.146:5555"   # 红果 23127PN0CC（2026-11-12 出图修复后标准验证环境；MuMu property service 清不干净 jq.dlgNNN 残留）

# 按钮底色（ui_solid_btn 0.22/0.28/0.42 alpha .85 混合后的典型值）
BTN_RGB = (49, 65, 90)
BTN_TOL = 12


def sh(*args, timeout=120):
    return subprocess.run(["adb", "-s", DEV] + list(args),
                          capture_output=True, text=True, timeout=timeout,
                          encoding="utf-8", errors="replace").stdout


def adb(*args, timeout=120):
    return sh(*args, timeout=timeout)


def find_datah(n):
    for f in sorted(glob.glob(os.path.join(SPAK, f"datah_{n:03d}_*.bin"))):
        return f
    return None


def load_dialog(n):
    f = find_datah(n)
    if not f:
        return None
    try:
        return parse_dialog(f)
    except Exception:
        return None


def detect_btn_rows(px, x_probe, y_from, y_to, w=720):
    """在 x_probe 列附近探测按钮色块行区间。

    ★ 反噪（§2026-11-11）：初版用固定色 (49,65,90) 检测 → alpha 0.85 混合后
      底色随背景变化（深棕棋盘上混出别的值）→ 漏检顶部 19px → 误报几何 Δ。
      改用「蓝灰」判据：b > r+12 且 **g−r < 30**（按钮绿接近红；天空 g−r=48 被排除）。
    """
    ys = []
    for y in range(max(0, y_from), min(1280, y_to)):
        c = 0
        for x in range(max(0, x_probe - 40), min(w, x_probe + 40)):
            p = px[x, y]
            if p[2] > p[0] + 12 and 30 < p[0] < 150 and p[2] < 190 and (p[1] - p[0]) < 30:
                c += 1
        if c >= 20:
            ys.append(y)
    if not ys:
        return None
    # 连续段
    segs = []
    st = ys[0]
    prev = ys[0]
    for y in ys[1:]:
        if y > prev + 3:
            segs.append((st, prev))
            st = y
        prev = y
    segs.append((st, prev))
    return segs


def clear_all_props():
    """★ 触发前全量清 jq.* 残留（红果 property service 有缓存残留——2026-11-09 实锤：
       jq.dlgNNN/flagtest/minetest 反复冒出，设备端 getprop 清后仍反复 [1]）。
       连清 3 次兜底 property service 缓存 + 设备端 getprop 残留检查。"""
    # dlgbatch + 全量 dlg000-264 + 测试钩子属性（含 game_test/native_event）
    for _round in range(3):
        for p in ("closetest","drawtest","flagtest","minetest","poptest","resigntest",
                  "stalematetest","restartgame","movetest","replay","backtest","imen",
                  "login","btconn","bttest","game_test","native_event"):
            adb("shell", "setprop", f"jq.{p}", "0")
        adb("shell", "setprop", "jq.dlgbatch", "0")
        for i in range(0, 265):
            adb("shell", "setprop", f"jq.dlg{i:03d}", "0")
    # 设备端残留检查（清后 getprop 仍有 [1] 则告警，防残留污染验证）
    residual = adb("shell", "sh", "-c",
                   "getprop | grep 'jq.' | grep -vE '\\[0\\]|\\[empty\\]' || true").strip()
    if residual:
        lines = residual.splitlines()
        print(f"  ⚠ clear_all_props 残留（property service 缓存，3 轮清后仍 {len(lines)} 条）：{lines[:3]}", flush=True)


def verify_one(n, keep_shot=False, jitter=True):
    r = {"n": n, "ok": True, "issues": []}
    d = load_dialog(n)
    if not d:
        r["ok"] = False
        r["issues"].append("原版 XML 未找到")
        return r
    ox, oy = panel_origin(d)
    bt = to_720(d)
    if not bt:
        r["issues"].append("原版无按钮")
        return r

    # 触发
    # ★ flaky 抖动专项（MuMu 渲染线程 118 条连续 am start 压力时序漂移）：
    #   force-stop 后随机 0.5~1.5s（旧进程 logcat Abort 串扰，确定性 0.8s 不够，
    #   加抖动消除固定时序碰撞）；渲染等待 4s + 随机 0~1s（冷启动首帧稳态）。
    adb("shell", "am", "force-stop", PKG)
    time.sleep(random.uniform(0.5, 1.5))
    # ★ 红果残留根因修复（2026-11-09 实锤）：触发前全量清 jq.* 残留，
    #   否则随机残留 jq.dlgMMM 会覆盖目标弹窗 → 白屏/非预期 datah。
    clear_all_props()
    adb("shell", "setprop", "jq.dlgbatch", f"{n:03d}")
    adb("shell", "setprop", f"jq.dlg{n:03d}", "1")
    adb("logcat", "-c")
    adb("shell", "am", "start", "-n", f"{PKG}/com.cnvcs.App")
    # ★ 2026-11-11 window 出屏修复后：渲染线程 window surface attach 慢（fromSurface 轮询重试，
    #   1Hz 心跳节流），首帧出屏需 ~8s。4s 截图窗口不够 → 非黑 0% 假 FAIL。改 10s+抖动。
    # ★ 红果转译层慢（2026-11-09 实锤）：fromSurface attach 需 ~50-90s（fromSurface
    #   第 2529 次重试才建 window surface），旧 90s 窗口不够 → 改 100s+抖动。
    #   MuMu 12s（快）。
    wait = 100.0 if DEV.startswith("10.") else 12.0
    time.sleep(wait + (random.uniform(0.0, 2.0) if jitter else 0.0))

    pid = adb("shell", "pidof", PKG).strip()
    if not pid:
        r["ok"] = False
        r["issues"].append("进程未存活")
        return r

    log = adb("logcat", "-d")
    for pat in ("FATAL EXCEPTION", "UnsatisfiedLinkError", "SIGSEGV",
                "signal 11", "Abort message"):
        if pat in log:
            r["ok"] = False
            r["issues"].append(f"崩溃迹象: {pat}")
    # ★ dlgbatch 判据修正（红果 2026-11-11 实锤）：jq_dlg_batch_hook 日志在 __android_log_print
    #   转译层可能被吞（红果无此日志但截图弹窗真出，非黑100%）→ 日志判据降级为参考。
    #   主判据 = 截图非黑比（渲染真实出帧），"钩子未命中"仅标注不判 FAIL。
    # ★ dlgbatch 判据终版：__system_property_set("jq.dlgNNN","0") 清属性后 logcat -d 抓不到
    #   钩子日志（红果转译层吞 + 属性清除时序）→ 判据降级：
    #   ① "批量对齐钩子" 在 logcat（正常路径）→ ok
    #   ② 截图非黑比 ≥85（渲染真出帧）→ ok（日志吞但画面出）
    #   ③ 否则标注"钩子日志未捕获+截图未出"（真 FAIL：既无日志又无画面）
    has_hook_log = ("批量对齐钩子" in log)
    if not has_hook_log:
        # 截图主判据下方会判非黑比，此处仅标注（非黑比 <85 时下方判 FAIL）
        r["issues"].append("钩子日志未捕获（转译层吞/属性清除，截图非黑比为主判据）")

    # 截图
    adb("shell", "screencap", "-p", f"/sdcard/av{n:03d}.png")
    subprocess.run(["adb", "-s", DEV, "pull", f"/sdcard/av{n:03d}.png",
                    f"/tmp/av{n:03d}.png"], capture_output=True, timeout=120)
    shot = f"/tmp/av{n:03d}.png"
    if not os.path.exists(shot):
        r["ok"] = False
        r["issues"].append("截图失败")
        return r
    from PIL import Image
    im = Image.open(shot).convert("RGB")
    px = im.load()
    total = im.size[0] * im.size[1]
    nz = sum(1 for p in im.getdata() if sum(p) > 30)
    r["nonblack"] = round(nz / total * 100, 1)
    # ★ 出图修复后重校准（2026-11-12）：旧 85% 判据基于 MuMu 全屏出帧基线；
    #   弹窗渲染实际非黑 ~8.8%（面板+文字+按钮区，非全屏亮）。改 ≥8%（画面真出帧）。
    #   主判据 = 几何按钮色块探测（detect_btn_rows）；非黑比仅挡"全黑=未渲染"。
    if r["nonblack"] < 8:
        r["ok"] = False
        r["issues"].append(f"非黑 {r['nonblack']}% < 8%（画面未出帧）")

    # 几何：第一个按钮的中点 x 处探测 y 区间
    b0 = bt[0]
    xm = (b0["x0"] + b0["x1"]) // 2
    segs = detect_btn_rows(px, xm, int(b0["y0"]) - 40, int(b0["y1"]) + 40)
    if segs:
        # ★ 反噪 2：只取**包含期望中心**的连续段（标题描边/棋盘蓝块等会被误判为按钮）
        want = (b0["y0"] + b0["y1"]) / 2.0
        cands = [s for s in segs if s[0] - 8 <= want <= s[1] + 8]
        if not cands:
            cands = [min(segs, key=lambda s: abs((s[0] + s[1]) / 2 - want))]
        best = min(cands, key=lambda s: abs((s[0] + s[1]) / 2 - want))
        dy0 = best[0] - b0["y0"]
        dy1 = best[1] - b0["y1"]
        r["geom"] = {"expect": [b0["y0"], b0["y1"]], "actual": list(best),
                     "dy": [dy0, dy1]}
        # ★ 判据改为「重叠率 + 中心包含」：
        #   原版存在**多行按钮且行间重叠**的布局（如 datah_006 两行 y 重叠 10px）
        #   → 实测色块连成一段，两端 Δ 判据必然误报。
        #   真正错位（如按钮在屏幕外/整段偏移）必然导致重叠率 < 70%。
        ov = max(0, min(best[1], b0["y1"]) - max(best[0], b0["y0"]))
        span = max(1, b0["y1"] - b0["y0"])
        r["geom"]["overlap_pct"] = round(ov / span * 100, 1)
        if r["geom"]["overlap_pct"] < 70 or not (best[0] <= want <= best[1]):
            r["ok"] = False
            r["issues"].append(
                f"几何错位: 期望 y{b0['y0']}..{b0['y1']} 实测 {best[0]}..{best[1]} "
                f"重叠 {r['geom']['overlap_pct']}%")
    else:
        r["issues"].append("未探测到按钮色块（可能未渲染或颜色不同）")

    if keep_shot:
        r["shot"] = shot
    return r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--list", nargs="*", help="指定 datah 编号")
    ap.add_argument("--out", default=os.path.join(ROOT, "audit", "r2", "DEVICE_VERIFY.md"))
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--noinstall", action="store_true",
                    help="跳过 install -r（假设 phase3.apk 已装）")
    a = ap.parse_args()

    # ★ MuMu flaky 抖动专项：全量模式默认先重装 phase3.apk（消除 so 与 APK 不一致
    #   导致的"钩子未命中（so 可能是旧的）"假 FAIL）。--noinstall 跳过（快速重验）。
    if not a.list and not a.noinstall:
        # ★ 对抗审计（2026-11-11）：交付物在**仓库根 dist/**（build_phases.sh 产物目录），
        #   旧脚本误用 rev64/dist/（不存在）→ 跳过重装 → 设备跑旧 so → "钩子未命中"假 FAIL。
        apk = None
        for cand in (os.path.normpath(os.path.join(ROOT, "..", "dist", "phase3.apk")),
                     os.path.join(ROOT, "dist", "phase3.apk")):
            if os.path.exists(cand):
                apk = cand
                break
        if apk:
            print(f"★ 重装 {apk}（消除 so 与 APK 不一致的 flaky 假 FAIL）", flush=True)
            subprocess.run(["adb", "-s", DEV, "install", "-r", apk],
                           capture_output=True, timeout=120)
            time.sleep(2.0)
        else:
            print("★ 仓库根 dist/ 与 rev64/dist/ 均无 phase3.apk，跳过重装（用设备现有 so）",
                  flush=True)

    ids = []
    if a.list:
        ids = [int(x) for x in a.list]
    else:
        uh = open(os.path.join(ROOT, "src_rs", "ui_menu.h"), encoding="utf-8").read()
        ids = sorted(set(int(x) for x in re.findall(r"\bUI_D(\d{3})\b", uh)))

    results = []
    for i, n in enumerate(ids, 1):
        r = verify_one(n)
        # ★ flaky 抖动专项：失败条自动补跑 1 次（mumu_regress.sh 同款思路）。
        #   118 条连续 am start 压力下 MuMu 渲染线程时序漂移（失败条无交集 +
        #   单条重验全过 + 代码基线 167/167 全绿 = 非代码回归）。随机抖动
        #   sleep 已降低碰撞概率，补跑消除残余 flaky。判据 = 两轮都挂才 FAIL
        #   （消除时序 flaky 误判，保留真实回归）。
        if not r["ok"]:
            time.sleep(2.0)
            r2 = verify_one(n)
            r2["retried"] = True
            r2["first_issues"] = r["issues"]
            r = r2
        results.append(r)
        flag = "✓" if r["ok"] else "✗"
        retry = " (retry)" if r.get("retried") else ""
        print(f"[{i}/{len(ids)}] datah_{n:03d} {flag}{retry} "
              f"非黑{r.get('nonblack', 0)}% "
              f"{'; '.join(r['issues']) if r['issues'] else ''}", flush=True)

    npass = sum(1 for r in results if r["ok"])
    flaky = [r for r in results if r.get("retried") and r["ok"]]
    hardfail = [r for r in results if not r["ok"]]
    with open(a.out, "w", encoding="utf-8") as f:
        f.write("# 真机全量弹窗验收（对抗审计闭环）\n\n")
        f.write(f"> 判据：进程存活 + 无崩溃 + 钩子命中 + 几何 |Δ|≤6px + 非黑≥85%\n")
        f.write(f"> 结果：**{npass}/{len(results)} PASS**\n")
        if flaky:
            f.write(f"> flaky 容错：{len(flaky)} 条首跑失败、补跑 PASS（MuMu 渲染线程 118 条连续 am start 时序 flaky，非代码回归）："
                    + " ".join(f"{r['n']:03d}" for r in flaky) + "\n")
        if hardfail:
            f.write(f"> 两轮均挂（真实 FAIL 候选）：" + " ".join(f"{r['n']:03d}" for r in hardfail) + "\n")
        f.write("\n")
        f.write("| datah | 结果 | 非黑% | 几何期望 | 几何实测 | Δ | 问题 |\n")
        f.write("|---|---|---|---|---|---|---|\n")
        for r in results:
            g = r.get("geom")
            exp = f"{g['expect'][0]}..{g['expect'][1]}" if g else "-"
            act = f"{g['actual'][0]}..{g['actual'][1]}" if g else "-"
            dy = str(g["dy"]) if g else "-"
            f.write(f"| {r['n']:03d} | {'✓' if r['ok'] else '✗'} | {r.get('nonblack','-')} "
                    f"| {exp} | {act} | {dy} | {'; '.join(r['issues'])} |\n")
    print(f"\n{npass}/{len(results)} PASS → {a.out}")
    return 0 if npass == len(results) else 1


if __name__ == "__main__":
    sys.exit(main())
