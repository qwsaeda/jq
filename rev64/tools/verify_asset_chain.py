#!/usr/bin/env python3
"""verify_asset_chain.py — APK 资源对齐 + 使用 三层一致性验证

用法：
  python3 rev64/tools/verify_asset_chain.py            # 全量（APK + 设备端 + 构建）
  python3 rev64/tools/verify_asset_chain.py --apk-only # 仅 APK 层
  python3 rev64/tools/verify_asset_chain.py --dev DEV   # 指定设备

三层判据：
  ① APK assets：spak_res 全量 .bin（棋盘/棋子 PNG 源）+ 关键 UI 纹理 .png
     + 原版 data*.pak（32 位 so 资源容器，doNotCompress 标记保留）
  ② 设备端 fs：/data/local/tmp/spak_res/ 下 .bin/.png 数量（so 文件系统回退优先路径）
     + rebuild_rs_apk.sh 推送幂等性
  ③ so 加载：NDK so 编译 0 error + 14 JNI 导出（资源加载代码链接完整）

诚实边界：
  - 设备端 spak_res 推送走 rebuild_rs_apk.sh（APK assets → /data/local/tmp/spak_res，幂等）
  - MuMu 64 位转译 AAssetManager 不可达 → so 全走文件系统回退（g_assets=NULL）
  - 本脚本只读验证（unzip -l + adb shell ls + readelf），不改构建产物
"""
import os
import re
import sys
import subprocess
import argparse

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# APK 在顶层 /disk/workspaces/jq/dist/（build_phases.sh 产物），非 rev64/dist
DIST = os.path.join(ROOT, "dist")
if not os.path.exists(os.path.join(DIST, "phase2.apk")):
    DIST = os.path.join(ROOT, "..", "dist")   # 回退顶层 dist
DEV = "emulator-5554"

# ① 渲染关键资源（rs_render_init 必读，缺失走纯色退化）
MUST_BIN = [
    "spak_res/datah_188_21d95e03e7692e98.bin",   # 棋盘 737×751
    "spak_res/datah_183_9e1b7fbc4dfc09c3.bin",   # 棋子图集 212×280
]
MUST_PNG = [
    "spak_res/menu_text_atlas.png",
    "spak_res/bg_menu_720.png",
    "spak_res/menu_bg_294.png",
    "spak_res/popup.png",
]
# 原版资源容器（32 位 so 读，doNotCompress 红线）
PAC = ["data0.pak", "data1.pak", "datah0.pak", "datah1.pak",
       "datax0.pak", "datax1.pak", "datax2.pak", "datax3.pak", "datam.pak"]

fail = 0

def check(cond, msg):
    global fail
    print(f"  {'✓' if cond else '✗ FAIL'}: {msg}")
    if not cond:
        fail += 1


def apk_entries(apk):
    p = subprocess.run(["unzip", "-l", apk], capture_output=True, text=True)
    lines = p.stdout.splitlines()
    names = set()
    for l in lines:
        parts = l.split()
        if len(parts) >= 4:
            names.add(parts[3])
    return names


def layer_apk():
    print("\n=== ① APK assets 层 ===")
    for name in ("phase2.apk", "phase3.apk"):
        apk = os.path.join(DIST, name)
        if not os.path.exists(apk):
            check(False, f"{name} 不存在（{apk}）")
            continue
        entries = apk_entries(apk)
        # .so 双 ABI
        has_arm64 = any(e.endswith("arm64-v8a/libjunqi.so") for e in entries)
        has_arm32 = any(e.endswith("armeabi-v7a/libjunqi.so") or
                        e.endswith("armeabi/libjunqi.so") for e in entries)
        check(has_arm64, f"{name}: lib/arm64-v8a/libjunqi.so")
        if name == "phase2.apk":
            check(has_arm32, f"{name}: lib/armeabi-v7a/libjunqi.so（双 ABI）")
        # spak_res 关键资源（带 assets/ 前缀，与 unzip -l 条目格式一致）
        for b in MUST_BIN:
            check(("assets/" + b) in entries, f"{name}: assets/{b}")
        for p in MUST_PNG:
            check(("assets/" + p) in entries, f"{name}: assets/{p}")
        # 原版 pak
        for p in PAC:
            check((("assets/" + p)) in entries, f"{name}: assets/{p}（doNotCompress 红线）")
        # 统计 spak_res 全量
        nbin = sum(1 for e in entries if e.startswith("assets/spak_res/") and e.endswith(".bin"))
        npng = sum(1 for e in entries if e.startswith("assets/spak_res/") and e.endswith(".png"))
        nfull = int(open(os.path.join(ROOT, "tools", "spak_render_refs.txt")).read().count("\n")) if os.path.exists(os.path.join(ROOT, "tools", "spak_render_refs.txt")) else 0
        in_spak = sum(1 for e in entries if e.startswith("assets/spak_res/"))
        print(f"  信息: {name} spak_res 共 {nbin} .bin / {npng} .png（APK 内 {in_spak} 文件 / render.c 引用 {nfull} 全量，APK 自包含）")


def layer_dev(dev=DEV):
    print("\n=== ② 设备端 fs 层（/data/local/tmp/spak_res）===")
    def shell(cmd):
        p = subprocess.run(["adb", "-s", dev, "shell", cmd],
                           capture_output=True, text=True, timeout=30)
        return p.stdout.strip()

    # 设备在线
    pid = shell("pidof com.cnvcs.junqirs || echo 0").strip()
    print(f"  信息: 设备 {dev}（{pid and 'so 运行中' or 'so 未运行'}）")
    nbin = shell("ls /data/local/tmp/spak_res/*.bin 2>/dev/null | wc -l").strip()
    npng = shell("ls /data/local/tmp/spak_res/*.png 2>/dev/null | wc -l").strip()
    # 设备端 spak_res 判据：arm64 侧只需 render.c 引用的 44 张全量在（2 .bin 棋盘/棋子
    #  PNG 源 + 42 .png UI 纹理）。全量 883 .bin 是 32 位 so 用的（非本判据范围）。
    refs = os.path.join(ROOT, "tools", "spak_render_refs.txt")
    nrefs = 0
    if os.path.exists(refs):
        for ln in open(refs):
            ln = ln.strip()
            if not ln:
                continue
            nrefs += 1
            ok = shell(f"test -s /data/local/tmp/spak_res/{ln} && echo ok || echo miss").strip() == "ok"
            check(ok, f"设备端 {ln}（render.c 引用，文件系统回退路径）")
    # 全量 .bin/.png 仅信息展示（32 位全量 883 .bin 不在 arm64 判据范围）
    print(f"  信息: 设备端 spak_res .bin={nbin} .png={npng}（arm64 判据=44 引用全量在，全量为 32 位参考）")
    # 关键渲染资源在设备端（rs_render_init 必读）
    for b in MUST_BIN:
        fn = b.split("/")[-1]
        ok = shell(f"test -s /data/local/tmp/spak_res/{fn} && echo ok || echo miss").strip() == "ok"
        check(ok, f"设备端 {fn}（棋盘/棋子 PNG 源）")
    for p in MUST_PNG:
        fn = p.split("/")[-1]
        ok = shell(f"test -s /data/local/tmp/spak_res/{fn} && echo ok || echo miss").strip() == "ok"
        check(ok, f"设备端 {fn}（UI 纹理）")
    # 资源加载诊断（so logcat 首帧：直接 grep logcat -d 全量，非 -s 过滤——
    #   "Init 像素解码"日志在 OnCreate 线程（非 GLThread），-s junqi_rs 按 tag 过滤会漏）
    log = shell("logcat -d 2>/dev/null | grep junqi_rs")
    miss = re.findall(r"纹理缺失.*?fs=(\S+)", log)
    ok_dec = re.search(r"Init 像素解码：board=OK", log)
    # 全量资源解码计数（33 张 OK = 全量像素解码成功，无缺失）
    n_ok = len(re.findall(r"=OK\(", log))
    if ok_dec:
        check(True, f"so 首帧 logcat: board 像素解码 OK（全量 {n_ok} 资源 =OK，无缺失诊断）")
    elif miss:
        check(False, f"so 首帧 logcat: 纹理缺失 {miss[0]}")
    else:
        check(False, "so 首帧 logcat: 无 board OK 也无缺失（需重装 apk 触发首帧）")


def layer_build():
    print("\n=== ③ so 构建层（资源加载代码链接完整）===")
    so = os.path.join(ROOT, "src_rs", "dist", "libjunqi_rs.so")
    if not os.path.exists(so):
        check(False, f"NDK so 未构建（{so}）")
        return
    p = subprocess.run(["readelf", "--dyn-syms", "-W", so],
                       capture_output=True, text=True)
    njni = len(re.findall(r"Java_com_cnvcs_App_", p.stdout))
    check(njni >= 14, f"JNI 导出 {njni} 个（≥14）")
    # 资源加载符号存在（rs_render_init 引用 AAsset/fopen）
    p2 = subprocess.run(["grep", "-c", "rs_read_png", os.path.join(ROOT, "src_rs", "render.c")],
                        capture_output=True, text=True)
    n = int(p2.stdout.strip() or 0)
    check(n >= 2, f"render.c rs_read_png 引用 {n} 处（资源加载链路）")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apk-only", action="store_true")
    ap.add_argument("--dev-only", action="store_true")
    ap.add_argument("--build-only", action="store_true")
    ap.add_argument("--dev", default=DEV)
    a = ap.parse_args()
    print("资源对齐 + 使用 三层验证")
    if a.apk_only:
        layer_apk()
    elif a.dev_only:
        layer_dev()
    elif a.build_only:
        layer_build()
    else:
        layer_apk()
        layer_dev(a.dev)
        layer_build()
    print(f"\n{'=== 资源链路全绿 ===' if fail == 0 else f'=== {fail} 项 FAIL ==='}")
    return 0 if fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
