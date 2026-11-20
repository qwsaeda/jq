#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""渗透轮判据脚本：fuzz_probe 4 变异器（无蓝牙）设备无关离线断言

固化"渗透轮"判据（AGENTS 渗透回路：黑盒→白盒→渗透→白盒→综合，本脚本 = 渗透轮入口）：
  基准：正向 src_rs dist/jq_fuzzprobe（make fuzzprobe，ASan 默认开防假绿）
  4 变异器（不含蓝牙，用户 2026-11-20 指令）：
    F1 随机对弈 fuzz（种子×局数，明棋 90% + 暗棋 10%，随机走子/翻棋）
    F2 触摸坐标 fuzz（越界/NaN/边界/极端坐标 → jq_touch_state_process 状态机无异常路径）
    F3 事件注入 fuzz（76 事件名随机 × 随机 %d 负载）
    F4 悔棋栈深度 fuzz（UNDO_MAX 边界连续灌/抽/满拒/空拒）
  判据 = jq_fuzzprobe rc=0 + "== 全部通过 ==" + ASan 无 report + 多 seed 一致性
  （同 seed 同输出可复现；3 seed 全 PASS = 单 seed 侥幸排除）

用法：python3 rev64/tools/fuzz_probe_check.py [--seed 7] [--games 500] [--seeds 7,42,999]
  默认：3 seed（7/42/999）× 默认规模（F1=500 局 F2=20000 F3=5000 F4=undo 边界）
  rc：0=PASS  2=FAIL（fuzz_probe 非 0 退出）  3=PENDING（binary 未构建）
"""
import argparse, os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GT = os.path.join(ROOT, 'rev64/forward/src_rs/dist/jq_fuzzprobe')

def run_once(seed, games, tn, en, timeout=300):
    cmd = [GT, str(seed), str(games), str(tn), str(en)]
    p = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    out = p.stdout + p.stderr
    ok = p.returncode == 0 and '== 全部通过 ==' in out
    # ASan 假绿防护：即便 rc=0，ASan report（ERROR: AddressSanitizer）也判 FAIL
    asan = 'ERROR: AddressSanitizer' in out or 'runtime error' in out
    return ok and not asan, out, p.returncode

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--seed', type=int, default=None, help='单 seed 快跑')
    ap.add_argument('--seeds', type=str, default='7,42,999', help='逗号分隔多 seed（默认 3 个防侥幸）')
    ap.add_argument('--games', type=int, default=500)
    ap.add_argument('--touch', type=int, default=20000)
    ap.add_argument('--events', type=int, default=5000)
    ap.add_argument('--offline', action='store_true', help='binary 缺失时 PENDING 非 FAIL')
    a = ap.parse_args()

    fails = []
    if not (os.path.isfile(GT) and os.access(GT, os.X_OK)):
        if a.offline:
            print('PENDING（jq_fuzzprobe 未构建，make fuzzprobe 后重跑）')
            return 3
        print('FAIL: 基准 binary 缺失（dist/jq_fuzzprobe）→ make fuzzprobe')
        return 2
    print('① 基准 binary 在位 OK（dist/jq_fuzzprobe ASan 版）')

    seeds = [a.seed] if a.seed is not None else [int(x) for x in a.seeds.split(',')]
    for s in seeds:
        ok, out, rc = run_once(s, a.games, a.touch, a.events)
        f1 = [l for l in out.splitlines() if l.startswith('F1 ')]
        f2 = [l for l in out.splitlines() if l.startswith('F2 ')]
        f3 = [l for l in out.splitlines() if l.startswith('F3 ')]
        f4 = [l for l in out.splitlines() if l.startswith('F4 ')]
        verdict = 'PASS' if ok else 'FAIL'
        print('seed=%d %s（rc=%d）' % (s, verdict, rc))
        for l in f1 + f2 + f3 + f4:
            print('   ' + l)
        if not ok:
            fails.append('seed=%d fuzz_probe 非全 PASS（rc=%d）' % (s, rc))
            for l in out.splitlines():
                if 'FAIL' in l or 'AddressSanitizer' in l or 'ERROR' in l:
                    print('   证据: ' + l)
    # ② 多 seed 一致性（可复现）：同 seed 重跑必同输出（F1 行数值一致）
    if len(seeds) > 1:
        ok1, out1, _ = run_once(seeds[0], a.games, a.touch, a.events)
        ok2, out2, _ = run_once(seeds[0], a.games, a.touch, a.events)
        f1a = [l for l in out1.splitlines() if l.startswith('F1 ')][0]
        f1b = [l for l in out2.splitlines() if l.startswith('F1 ')][0]
        if f1a == f1b:
            print('② 可复现性 OK（同 seed 重跑 F1 输出一致）')
        else:
            fails.append('② 可复现性破（同 seed 两次 F1 输出不一致）')
            print('   两次: %s | %s' % (f1a, f1b))
    # ③ 判据汇总（4 变异器全绿）
    if not fails:
        print('③ 渗透轮判据 4 项全 PASS（F1 随机对弈/F2 触摸/F3 事件/F4 undo 边界 0 崩溃 ASan 干净）')
        print('判据: PASS（fuzz_probe_check %d seed × 4 变异器 + 可复现性，无蓝牙渗透判据）' % len(seeds))
        return 0
    for f in fails:
        print('FAIL:', f)
    print('判据: FAIL')
    return 2

if __name__ == '__main__':
    sys.exit(main())
