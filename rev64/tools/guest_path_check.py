#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1943 账号系统游客路径实锤判据（设备无关）

固化"账号系统游客路径（本地单机唯一真生效路径）+ 登录/注册诚实边界"
口径（AGENTS 账号系统未逆向，游客 = 本地热座开局，BUSINESS_RISK_
LEDGER P2-1）：
  - 基准：正向 src_rs dist/jq_evverify --all（离线全量断言，含 login
    6 按钮几何 + 游客 hit=4 action=4 + 开局 stage=PLAY/turn=BLUE）
  - 游客路径断言：login guest hit=4 action=4 → 本地单机（g_vs_ai=0
    两人热座）→ 开局 stage=PLAY turn=BLUE（AUD-1000 B08-020 P1 已修
    开局断言）
  - 登录/注册诚实边界：action=2/3 → 边界对话框（服务器协议未逆向）
    → 回主菜单
判据（4 项全 PASS）：
  ① 基准 binary 在位（dist/jq_evverify 可执行）
  ② login 6 按钮几何对齐（src == 期望，6/6 命中）
  ③ 游客路径 hit=4 action=4 → 本地单机开局（stage=PLAY turn=BLUE）
  ④ 整体 jq_evverify --all rc=0（全量断言通过）

用法：python3 rev64/tools/guest_path_check.py
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EV = os.path.join(ROOT, 'rev64/forward/src_rs/dist/jq_evverify')

def main():
    fails = []
    # ① 基准 binary 在位
    if not (os.path.isfile(EV) and os.access(EV, os.X_OK)):
        print('① 基准 binary 缺失（%s）→ 先 make evverify' % EV)
        print('判据: PENDING（基准 binary 未构建，make evverify 后重跑）')
        return 3
    print('① 基准 binary 在位 OK（dist/jq_evverify --all 离线全量断言）')

    p = subprocess.run([EV, '--all'], capture_output=True, text=True)
    out = p.stdout + p.stderr
    rc = p.returncode

    # ② login 6 按钮几何对齐（src == 期望）
    geoms = re.findall(r'login btn\d: src\((\d+),(\d+)\)-\((\d+),(\d+)\) 期望\((\d+),(\d+)\)-\((\d+),(\d+)\)', out)
    align = [g for g in geoms if g[0]==g[4] and g[1]==g[5] and g[2]==g[6] and g[3]==g[7]]
    if len(geoms) != 6 or len(align) != 6:
        fails.append('② login 6 按钮几何 = %d/%d 对齐（非 6/6）' % (len(align), len(geoms)))
    else:
        print('② login 6 按钮几何对齐 OK（src == 期望，6/6 命中）')

    # ③ 游客路径 hit=4 action=4 → 本地单机开局
    m = re.search(r'login guest: hit=(\d+) action=(\d+).*stage=(\d+) turn=(\d+)', out)
    if m and m.group(1)=='4' and m.group(2)=='4' and m.group(3)=='1' and m.group(4)=='0':
        print('③ 游客路径实锤 OK（hit=4 action=4 → 本地单机 两人热座 开局 stage=PLAY turn=BLUE）')
    elif m:
        fails.append('③ 游客路径数值漂移（hit=%s action=%s stage=%s turn=%s ≠ 4/4/PLAY/BLUE）' % m.groups())
    else:
        fails.append('③ 游客路径断言缺失（login guest 未输出）')

    # ④ 整体 --all rc=0
    if rc == 0:
        print('④ jq_evverify --all 全量断言通过 OK（rc=0）')
    else:
        fails.append('④ jq_evverify --all rc=%d ≠ 0' % rc)

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（账号系统游客路径实锤 4 项：login 6 按钮几何 + 游客 hit=4→本地热座开局 + --all 全量通过，BUSINESS_RISK_LEDGER P2-1 诚实边界固化）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
