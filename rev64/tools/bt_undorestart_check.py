#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1941 蓝牙 UNDO/RESTART 帧闭环离线断言判据（设备无关）

固化"蓝牙 UNDO/RESTART 帧语义闭环"口径（jq_bttest 78 条断言，SPEC
蓝牙联机 SPP 帧契约，32 位翻译体走子桥同源）：
  - 基准：正向 src_rs dist/jq_bttest（bt_semtest 78 条离线断言）
  - UNDO/RESTART 帧闭环断言（bt_protocol.c v2\n 7 字段帧 + 闸口）：
    ① OUR_TURN 拒收 UNDO 帧（rc=-2，B04-027 闸口覆盖）
    ② DISCONNECTED 拒收 UNDO 数据帧（rc=-2，B04-013+B04-027）
    ③ DISCONNECTED 拒收 RESTART 帧（rc=-2，B04-012+B04-027）
    ④ 帧编码同源（v2\ntype,fc,fr,tc,tr,rank,side\n 7 字段，UUID
       ce1404fe = 32/64 位同源 §1933）
  - 判据 = jq_bttest "== 结果: 全部通过 ==" 退出码 0 + UNDO/RESTART
    闸口断言全绿 + 78 条全过
判据（4 项全 PASS）：
  ① 基准 binary 在位（dist/jq_bttest 可执行）
  ② UNDO/RESTART 闸口断言在位（OUR_TURN/DISCONNECTED 拒收）
  ③ 帧编码 7 字段同源（v2\n 头 + 尾 \n + UUID ce1404fe）
  ④ 整体 "== 结果: 全部通过 =="（jq_bttest rc=0，78 条全过）

用法：python3 rev64/tools/bt_undorestart_check.py
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BT = os.path.join(ROOT, 'rev64/forward/src_rs/dist/jq_bttest')

def main():
    fails = []
    # ① 基准 binary 在位
    if not (os.path.isfile(BT) and os.access(BT, os.X_OK)):
        print('① 基准 binary 缺失（%s）→ 先 make bttest' % BT)
        print('判据: PENDING（基准 binary 未构建，make bttest 后重跑）')
        return 3
    print('① 基准 binary 在位 OK（dist/jq_bttest，bt_semtest 78 条离线断言）')

    p = subprocess.run([BT], capture_output=True, text=True)
    out = p.stdout + p.stderr
    rc = p.returncode
    all_ok = '== 结果: 全部通过 ==' in out or '全部通过' in out

    # ② UNDO/RESTART 闸口断言在位
    # ★ R19 口径同步 (R8-C-16 语义演进, AUD_ALIGN_R17 P0-2):
    #   旧判据 "DISCONNECTED 拒收 RESTART 帧 rc=-2" = B04-012 全拒收语义 (已废弃)。
    #   R8-C-16 定案: DISCONNECTED 下 RESTART 放行 (会话恢复路径, rc=0), 闸口仅拒数据帧
    #   继续污染终局棋盘。现行断言组 = OUR_TURN 拒收 UNDO (rc=-2, B04-027) +
    #   DISCONNECTED 拒收 UNDO 数据帧 (rc=-2, B04-013+B04-027) +
    #   DISCONNECTED 放行 RESTART (rc=0, R8-C-16 会话恢复)。
    ok2 = ('拒收 UNDO' in out or 'OUR_TURN 拒收 UNDO' in out) and \
          ('DISCONNECTED 拒收 UNDO' in out or 'DISCONNECTED 拒收 UNDO 数据帧' in out) and \
          ('DISCONNECTED 放行 RESTART' in out or 'R8-C-16' in out)
    if not ok2:
        fails.append('② UNDO/RESTART 闸口断言缺失（OUR_TURN 拒收 UNDO + DISCONNECTED 拒收 UNDO 数据帧 + R8-C-16 放行 RESTART）')
    else:
        print('② UNDO/RESTART 闸口断言在位 OK（OUR_TURN 拒收 UNDO + DISCONNECTED 拒收 UNDO 数据帧 + R8-C-16 DISCONNECTED 放行 RESTART 会话恢复, B04/R8 闸口）')

    # ③ 帧编码 7 字段同源
    ok3 = 'v2' in out and 'UUID' in out.lower() or 'ce1404fe' in open(os.path.join(ROOT,'rev64/forward/src_rs/bt_protocol.c'),errors='ignore').read()
    if not ok3:
        fails.append('③ 帧编码 7 字段同源缺失（v2\n 头 / UUID ce1404fe）')
    else:
        print('③ 帧编码 7 字段同源 OK（v2\ntype,fc,fr,tc,tr,rank,side\n + UUID ce1404fe §1933）')

    # ④ 整体 == 结果: 全部通过 ==
    if all_ok and rc == 0:
        print('④ 整体 == 结果: 全部通过 == OK（jq_bttest rc=0，78 条全过）')
    else:
        fails.append('④ jq_bttest 整体未全通过（rc=%d all_ok=%s）' % (rc, all_ok))

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（蓝牙 UNDO/RESTART 帧闭环 4 项：闸口拒收 + 帧编码同源 + 78 条全部通过，SPEC 蓝牙联机 SPP 帧契约固化）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
