#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
rule_probe_compare.py — 32/64 位规则引擎**行为实证对比**（§124y R-138 分叉实证专项）

目的：
  分叉矩阵（R-138）此前 102 条 = 已标记 101 + 已实证 0 —— "已标记"只是**读代码
  判读**，不是行为实证。本脚本用**逐字节同源**的规则行为探针
  （rev64/src_rs/rule_probe_shared.inc）让两端引擎跑同一批棋盘位置，
  diff 输出 → 一致 = **行为实证**；不一致 = **实证分歧**（分叉坐实）。

用法：
  python3 rev64/tools/rule_probe_compare.py                # 全量（64 位桌面 + 32 位设备）
  python3 rev64/tools/rule_probe_compare.py --dev emulator-5554
  python3 rev64/tools/rule_probe_compare.py --offline       # 只跑 64 位（无设备）

流程：
  ① 64 位：make ruleprobe → dist/jq_ruleprobe（桌面，stdout 出 RULE-PROBE 行）
  ② 32 位：装 rev64/dist/phase_chain_arm32.apk（包名 com.cnvcs.junqi32）
            setprop jq.rule_probe 1 → am start → logcat 取 RULE-PROBE 行
  ③ 规范化（去 logcat 前缀 / 去重 / 排序）→ diff → 输出一致/分歧清单

诚实边界：
  · 探针只覆盖**可脚本化的规则判定点**（走子合法性 / 吃子 / 炸弹撞旗 / 翻棋 /
    困毙 / 计数），不覆盖 UI 几何 / 音效 / 蓝牙协议 / 渲染。
  · "一致"仅代表**探针覆盖到的用例**行为一致，非全量规则等价证明。
  · 32 位侧需真机/模拟器（ARM 转译环境），无设备时本脚本只出 64 位基线。
"""
import os
import re
import subprocess
import sys
import argparse

WS = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(WS, "rev64", "src_rs")
DEV = "10.0.182.146:5555"
PKG32 = "com.cnvcs.junqi32"

fail = 0


def check(cond, msg):
    global fail
    print(f"  {'✓' if cond else '✗ FAIL'}: {msg}")
    if not cond:
        fail += 1


def sh(cmd, timeout=120):
    p = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)
    return p.stdout


def norm(lines):
    """规范化：★ logcat 行带 "MM-DD HH:MM:SS.mmm PID TID L tag: " 前缀
       （64 位桌面行则无前缀）→ 先用正则抽出 "RULE-PROBE ..." 段再比对，
       否则两端行格式不同 → 全部误判为分歧。"""
    out = set()
    for ln in lines:
        m = re.search(r"(RULE-PROBE.*)$", ln)
        if not m:
            continue
        seg = m.group(1).strip()
        if "启动" in seg:
            continue
        out.add(seg)
    return sorted(out)


def run_64():
    print("=== ① 64 位（桌面探针）===")
    r = subprocess.run(["make", "ruleprobe"], cwd=SRC, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout[-800:], r.stderr[-800:])
        return None
    exe = os.path.join(SRC, "dist", "jq_ruleprobe")
    if not os.path.exists(exe):
        return None
    out = sh(f"{exe}")
    lines = norm(out.split("\n"))
    print(f"  64 位探针输出 {len(lines)} 条")
    return lines


def run_32(dev):
    print(f"=== ② 32 位（设备 {dev} 探针）===")
    apk = os.path.join(WS, "rev64", "dist", "phase_chain_arm32.apk")
    if not os.path.exists(apk):
        print(f"  ✗ APK 不存在 {apk}（先 build_arm32_aligned.sh + pack_arm32.sh）")
        return None
    r = subprocess.run(["adb", "-s", dev, "install", "-r", apk],
                       capture_output=True, text=True, timeout=180)
    if "Success" not in (r.stdout + r.stderr):
        print(f"  ✗ 装机失败: {r.stdout.strip()} {r.stderr.strip()}")
        return None
    print("  ✓ 装机 Success")
    sh(f'adb -s {dev} shell "am force-stop {PKG32}"')
    sh(f'adb -s {dev} shell "setprop jq.rule_probe 1"')
    sh(f'adb -s {dev} logcat -c')
    sh(f'adb -s {dev} shell "am start -n {PKG32}/com.cnvcs.App"')
    # ★ 轮询等待（重装后 32 位 app 走 houdini 转译冷启动慢，固定 sleep 会假红）
    import time
    out = ""
    for _ in range(30):
        time.sleep(2)
        out = sh(f'adb -s {dev} logcat -d 2>/dev/null | grep "RULE-PROBE"')
        if "RULE-PROBE DONE" in out:
            break
    lines = norm(out.split("\n"))
    print(f"  32 位探针输出 {len(lines)} 条")
    return lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dev", default=DEV)
    ap.add_argument("--offline", action="store_true")
    a = ap.parse_args()

    print("32/64 位规则引擎行为实证对比（§124y R-138 分叉实证专项）")
    p64 = run_64()
    if p64 is None:
        print("✗ 64 位探针未跑通")
        return 1
    check(len(p64) > 0, f"64 位探针输出非空（{len(p64)} 条）")

    if a.offline:
        print("\n=== ③ 对比：离线模式（无 32 位设备）===")
        print("  跳过 32 位对比（--offline）")
        return 0

    p32 = run_32(a.dev)
    if p32 is None:
        print("\n=== ③ 对比：跳过（32 位探针未跑通）===")
        return 1

    print("\n=== ③ 对比 ===")
    only64 = [x for x in p64 if x not in p32]
    only32 = [x for x in p32 if x not in p64]
    same = [x for x in p64 if x in p32]
    print(f"  一致 {len(same)} 条 / 分歧 {len(only64) + len(only32)} 条")
    if only64:
        print("  ── 仅 64 位（32 位不同 = 实证分歧）──")
        for x in only64:
            print(f"     64: {x}")
    if only32:
        print("  ── 仅 32 位（32 位不同 = 实证分歧）──")
        for x in only32:
            print(f"     32: {x}")
    check(len(same) >= len(p64) - 1,
          f"行为一致用例 ≥ {len(p64)-1}/{len(p64)}（分歧数 ≤1）")

    # 落盘（供文档引用）
    os.makedirs("/tmp/opencode", exist_ok=True)
    with open("/tmp/opencode/probe64.txt", "w") as f:
        f.write("\n".join(p64) + "\n")
    with open("/tmp/opencode/probe32.txt", "w") as f:
        f.write("\n".join(p32) + "\n")
    print(f"  落盘: /tmp/opencode/probe64.txt + probe32.txt")

    print(f"\n{'=== 行为实证完成 ===' if fail == 0 else f'=== {fail} 项 FAIL ==='}")
    return 0 if fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
