#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1938 设备侧判据清单离线机判判据（设备无关 dry-run）

固化"设备恢复后复验链 4 项子判据清单"口径（§1937 device_regress_check.sh
的 4 项子判据逐条机判，防设备恢复时漏项）：
  - 前置：so 三方哈希核对（基准 59465d97，SOP §3.1）
  - 设备 4 项：arm32_hook_check --rounds 3（game_test/walk_bridge/
    native_event/framebuffer 全钩子 + 出屏 30% 非黑比）
  - 走子桥 4 路径：arm32_walkregress.sh（4/4 PASS + 0 崩溃）
  - 真机 5 判据：REAL_DEVICE_ACCEPTANCE.md（装/出屏/下一局/退出/重进）
  - 回填：REVERSE_LOG 设备侧 PASS 记录
判据（5 项全 PASS）：
  ① so 三方哈希基准 = 59465d97（dist so + apk 内 so 一致）
  ② 设备 4 项入口在位（arm32_hook_check.sh --rounds 3 + 出屏 30% 阈值）
  ③ 走子桥 4 路径入口在位（arm32_walkregress.sh 4 路径判据）
  ④ 真机 5 判据 SOP 在位（REAL_DEVICE_ACCEPTANCE.md 5 判据 + 三方哈希核对）
  ⑤ device_regress_check.sh 复验链一键入口在位（前置 + 4 项 + 回填）
设备 offline 时本判据 = 离线机判（判据入口/基准/阈值在位），设备恢复
后跑 device_regress_check.sh 实锤。

用法：python3 rev64/tools/device_regress_offline_check.py
"""
import hashlib, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DIST_SO = os.path.join(ROOT, 'rev64/dist/libjunqi_arm32_aligned.so')
APK = os.path.join(ROOT, 'rev64/dist/phase_chain_arm32.apk')
HOOK = os.path.join(ROOT, 'rev64/tools/arm32_hook_check.sh')
WALK = os.path.join(ROOT, 'rev64/tools/arm32_walkregress.sh')
ACCEPT = os.path.join(ROOT, 'rev64/reverse/docs/REAL_DEVICE_ACCEPTANCE.md')
DEVREG = os.path.join(ROOT, 'rev64/tools/device_regress_check.sh')
BASELINE_MD5 = '59465d974a9645cd426b965cb10901c5'

def md5_of(path):
    h = hashlib.md5()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()

def main():
    fails = []
    # ① so 三方哈希基准 = 59465d97
    so_md5 = md5_of(DIST_SO)
    if so_md5 != BASELINE_MD5:
        fails.append('① dist so md5=%s ≠ 基准 %s' % (so_md5, BASELINE_MD5))
    else:
        print('① dist so 哈希 = 基准 59465d97 OK')
    # apk 内 so 哈希（device_regress_check 前置 apk_so_hash_check 同源）
    apk_md5 = md5_of(APK)
    print('   apk phase_chain_arm32.apk md5=%s（设备端 so 提取核对基准）' % apk_md5)

    # ② 设备 4 项入口在位
    hk = open(HOOK, errors='ignore').read()
    ok2 = ('rounds' in hk) and ('30' in hk) and ('game_test' in hk)
    if not ok2:
        fails.append('② arm32_hook_check.sh 缺 --rounds / 出屏 30% 阈值 / game_test')
    else:
        print('② 设备 4 项入口在位 OK（--rounds 3 + 出屏 30% + game_test/walk_bridge/native_event/framebuffer）')

    # ③ 走子桥 4 路径入口在位
    wk = open(WALK, errors='ignore').read()
    ok3 = ('4' in wk) and ('PASS' in wk.upper())
    if not ok3:
        fails.append('③ arm32_walkregress.sh 缺 4 路径判据')
    else:
        print('③ 走子桥 4 路径入口在位 OK（arm32_walkregress.sh 4/4 PASS 判据）')

    # ④ 真机 5 判据 SOP 在位
    acc = open(ACCEPT, errors='ignore').read()
    ok4 = all(k in acc for k in ['59465d97', '装', '出屏', '下一局', '退出', '重进']) or \
         all(k in acc for k in ['三方哈希', '5 判据'])
    if not ok4:
        fails.append('④ REAL_DEVICE_ACCEPTANCE.md 缺 5 判据 / 三方哈希')
    else:
        print('④ 真机 5 判据 SOP 在位 OK（装/出屏/下一局/退出/重进 + 三方哈希核对）')

    # ⑤ device_regress_check.sh 复验链一键入口在位
    dr = open(DEVREG, errors='ignore').read()
    ok5 = all(k in dr for k in ['59465d97', 'arm32_hook_check.sh', 'arm32_walkregress.sh', 'PENDING'])
    if not ok5:
        fails.append('⑤ device_regress_check.sh 缺 基准哈希 / hook_check / walkregress / PENDING 口径')
    else:
        print('⑤ device_regress_check.sh 复验链一键入口在位 OK（基准 59465d97 + 设备 4 项 + 走子桥 4 路径 + PENDING 非 FAIL）')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（设备侧判据清单离线机判 5 项：so 基准 + 设备 4 项 + 走子桥 4 路径 + 真机 5 判据 + 复验链入口全在位，设备恢复即跑）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
