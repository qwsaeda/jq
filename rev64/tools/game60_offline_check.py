#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1939 完整 60 局对弈离线断言判据（设备无关）

固化"完整 60 局对弈行为级离线断言"口径（jq_gametest 桌面 60 局对弈
闭环，SPEC 完整对弈行为基准，32 位翻译体规则引擎同源）：
  - 基准：正向 src_rs dist/jq_gametest（-DRS_NO_GL 桌面 binary，零设备）
  - 完整 60 局断言（12×5=60 格棋盘 50 子）：
    ① 每次 jq_try_move 返回已知合法事件（无未分类返回值）
    ② 对局含吃子/炸弹事件（规则引擎非全空移，真实军棋走子）
    ③ 人机对战回合交替闭环（蓝走子=10 红AI应着=10 翻转=20 困毙=0）
    ④ 红方 AI 有实际应着（非全卡死）+ 蓝方有实际走子 + 回合翻转
       （蓝→红 交替，非单方死循环）
  - 判据 = jq_gametest "== 全部通过 ==" 退出码 0 + 4 项断言全绿
判据（4 项全 PASS）：
  ① 基准 binary 在位（dist/jq_gametest 可执行）
  ② 完整 60 局对局含吃子/炸弹事件（非全空移）
  ③ 回合交替闭环（蓝走子 10 / 红 AI 应着 10 / 翻转 20 / 困毙 0）
  ④ 整体 "== 全部通过 =="（jq_gametest rc=0）

用法：python3 rev64/tools/game60_offline_check.py
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GT = os.path.join(ROOT, 'rev64/forward/src_rs/dist/jq_gametest')

def main():
    fails = []
    # ① 基准 binary 在位
    if not (os.path.isfile(GT) and os.access(GT, os.X_OK)):
        print('① 基准 binary 缺失（%s）→ 先 make gametest' % GT)
        print('判据: PENDING（基准 binary 未构建，make gametest 后重跑）')
        return 3
    print('① 基准 binary 在位 OK（dist/jq_gametest -DRS_NO_GL 桌面）')

    out = subprocess.run([GT], capture_output=True, text=True).stdout
    rc = subprocess.run([GT], capture_output=True, text=True).returncode
    all_ok = '== 全部通过 ==' in out

    # ② 完整 60 局对局含吃子/炸弹事件（非全空移）
    if '对局含吃子/炸弹事件' in out:
        print('② 完整 60 局对局含吃子/炸弹事件 OK（非全空移，真实军棋走子）')
    else:
        fails.append('② 完整 60 局对局缺吃子/炸弹事件（非全空移断言未过）')

    # ③ 回合交替闭环（蓝走子 10 / 红 AI 应着 10 / 翻转 20 / 困毙 0）
    m = re.search(r'蓝方走子=(\d+) 红方AI走子=(\d+) 翻转=(\d+) 困毙=(\d+)', out)
    if m and m.group(1) == '10' and m.group(2) == '10' and m.group(3) == '20' and m.group(4) == '0':
        print('③ 回合交替闭环 OK（蓝走子=10 红AI应着=10 翻转=20 困毙=0，单局 20 回合上限）')
    elif m:
        fails.append('③ 回合交替数值漂移（蓝=%s 红=%s 翻转=%s 困毙=%s ≠ 10/10/20/0）' % m.groups())
    else:
        fails.append('③ 回合交替闭环断言缺失（蓝走子/红AI应着/翻转/困毙 未输出）')

    # ④ 整体 == 全部通过 ==
    if all_ok and rc == 0:
        print('④ 整体 == 全部通过 == OK（jq_gametest rc=0）')
    else:
        fails.append('④ jq_gametest 整体未全通过（rc=%d all_ok=%s）' % (rc, all_ok))

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（完整 60 局对弈离线断言 4 项：吃子/炸弹 + 回合交替闭环 10/10/20/0 + 全部通过，SPEC 完整对弈行为基准固化）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
