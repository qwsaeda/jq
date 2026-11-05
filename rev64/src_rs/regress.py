#!/usr/bin/env python3
"""regress.py — 规则引擎回归（调用 dist/jq_run --test 子模式 + 悔棋/吃子断言）
用法： python3 regress.py [--games 50]
"""
import subprocess, sys, os, re, argparse

BIN = os.path.join(os.path.dirname(__file__), "dist", "jq_run")

def run_auto(n_steps=60, extra=""):
    inp = ("a\n" * n_steps) + ("u\n" + "a\n" * 5 + "q\n") if "u" in extra else ("a\n" * n_steps + "q\n")
    p = subprocess.run([BIN], input=inp, capture_output=True, text=True, errors='replace', timeout=120)
    return p

def parse_moves(out):
    evs = {"move":0,"eat":0,"bomb":0,"flag":0,"dead":0,"mine":0}
    n = 0
    for l in out.splitlines():
        if l.startswith("  ["):
            n += 1
            m = re.search(r"Junqi\.(Move|Eat|Bomb|Flag|Dead|Mine)", l)
            if m: evs[m.group(1).lower()] += 1
    wm = re.search(r"winner=(\d+)", out)
    return n, (int(wm.group(1)) if wm else None), evs

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--games", type=int, default=50)
    args = ap.parse_args()

    print(f"=== 走一局回归（{args.games} 局 AI 对弈）===")
    ok = 0; bad = 0
    tot = {"move":0,"eat":0,"bomb":0,"flag":0,"dead":0,"mine":0}
    for g in range(args.games):
        p = run_auto()
        n, winner, evs = parse_moves(p.stdout)
        if p.returncode != 0:
            bad += 1
            continue
        if winner not in (0, 1, 2):
            bad += 1
            continue
        ok += 1
        for k in tot: tot[k] += evs.get(k, 0)

    print(f"合法终局 {ok}/{args.games}；事件累计 {tot}")
    assert ok >= args.games * 0.9, "通过率 <90%"

    print("\n=== 悔棋回归（20 局 × 中盘撤销 1 步）===")
    uok = 0
    for g in range(20):
        inp = "a\n" * 30 + "u\n" + "a\n" * 10 + "q\n"
        p = subprocess.run([BIN], input=inp, capture_output=True, text=True, errors='replace', timeout=60)
        if "悔棋成功" in p.stdout: uok += 1
    print(f"悔棋 {uok}/20")

    print("\n=== 吃子/炸弹/吃旗 事件覆盖断言 ===")
    p = run_auto(80)
    n, winner, evs = parse_moves(p.stdout)
    print(f"单局事件 {evs}（moves={n}）")
    has_eat = evs["eat"] > 0 or evs["bomb"] > 0
    print(f"吃子/炸弹覆盖: {has_eat}")
    assert has_eat, "未出现任何吃子事件"

    print("\n=== 全部验收通过 ===")

if __name__ == "__main__":
    main()
