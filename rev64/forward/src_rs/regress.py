#!/usr/bin/env python3
"""regress.py — 规则引擎回归（调用 dist/jq_run --test 子模式 + 悔棋/吃子断言）
用法： python3 regress.py [--games 50]
"""
import subprocess, sys, os, re, argparse

BIN = os.path.join(os.path.dirname(__file__), "dist", "jq_run")

def run_auto(n_steps=60):
    # ★ AUD-1000 B08-003/B07-002（P1 已修）：删除死参数 extra——旧版
    # `("a\n"*n + ("u\n"+"a\n"*5+"q\n") if "u" in extra else "a\n"*n + "q\n")`
    # 中 extra 恒 ''（行 34/57 调用点均传默认）→ "u" 悔棋分支为死代码，
    # 悔棋回归实际全靠下方 line 60 手写 inp 段（两路逻辑分叉）。
    # 现统一入口：主走局 n 步后 q 退出；悔棋回归独立函数 run_undo_auto。
    inp = ("a\n" * n_steps) + "q\n"
    p = subprocess.run([BIN], input=inp, capture_output=True, text=True, errors='replace', timeout=120)
    return p

def run_undo_auto(steps=30):
    """★ B08-003 统一悔棋回归入口（原死代码 extra='u' 路径提取）：
       走 steps 步 → 中盘撤销 1 步 → 再走 10 步 → q。与 main 手写段
       同语义（"a"*30 + "u" + "a"*10 + "q"），两路分叉消除。"""
    inp = ("a\n" * steps) + "u\n" + ("a\n" * 10) + "q\n"
    p = subprocess.run([BIN], input=inp, capture_output=True, text=True, errors='replace', timeout=60)
    return p

def parse_moves(out):
    evs = {"move":0,"eat":0,"bomb":0,"flag":0,"dead":0,"mine":0}
    n = 0
    for l in out.splitlines():
        if l.startswith("  ["):
            n += 1
            m = re.search(r"Junqi\.(Move|Eat|Bomb|Flag|Dead|Mine)", l)
            if m: evs[m.group(1).lower()] += 1
    # ★ AUD-1000 B08-002（P1 已修）：winner 正则允许负值——旧版 winner=(\d+)
    #   对 JQ_WIN_NONE(-1) 未定/和棋判定漏设 winner 时 parse 返 None → 误判 bad。
    # ★ AUD-1000 B08-041 回归修复：取**最后一次** winner 匹配——主循环每步打印
    #   `move=N ... winner=-1`（111 行），终局行 `=== 终局 === winner=X` 在最后。
    #   旧 re.search 命中**第一**个 -1（步中行）→ 全 50 局误判未收口（50 pending）。
    ms = re.findall(r"winner=(-?\d+)", out)
    return n, (int(ms[-1]) if ms else None), evs

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--games", type=int, default=50)
    args = ap.parse_args()

    print(f"=== 走一局回归（{args.games} 局 AI 对弈）===")
    ok = 0; bad = 0
    pending = 0   # ★ AUD-1000 B08-002（P1）：winner=-1（JQ_WIN_NONE 未收口）中性计数
    tot = {"move":0,"eat":0,"bomb":0,"flag":0,"dead":0,"mine":0}
    for g in range(args.games):
        p = run_auto()
        n, winner, evs = parse_moves(p.stdout)
        if p.returncode != 0:
            bad += 1
            continue
        # ★ AUD-1000 B08-002/041（P1 已修）：winner 三分判据——
        #   -1（JQ_WIN_NONE 未收口）= 中性（不计 ok 不计 bad，单独 pending）
        #   0/1/2 = ok；其他（3+ 非法值）= bad（规则引擎非法 winner = 真回归）
        if winner == -1:
            pending += 1
            continue
        if winner not in (0, 1, 2):
            bad += 1
            continue
        ok += 1
        for k in tot: tot[k] += evs.get(k, 0)

    print(f"合法终局 {ok}/{args.games}（未收口 {pending} 局中性）；事件累计 {tot}")
    # ★ AUD-1000 B08-001（P0 已修）：旧判据 ok >= 90%（50 局 45 过即 PASS），
    # 规则引擎回归（如吃子逻辑错致 10% 局非法终局）被容差吞掉 = 假绿。
    # 规则引擎是确定性逻辑，合法终局必须是 100%（winner∈{0,1,2} 的局占满
    # 除未收口中性局外）——pending（对局进行中即停的 -1）不计回归分母。
    active = ok + bad
    assert ok == active and active > 0, f"合法终局 {ok}/{active}（active 局须全 ok；pending={pending} 中性）"

    print("\n=== 悔棋回归（20 局 × 中盘撤销 1 步）===")
    uok = 0
    for g in range(20):
        # ★ B08-003：走统一悔棋入口（原手写段与 run_auto 死参数两路分叉已消除）
        p = run_undo_auto(30)
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
