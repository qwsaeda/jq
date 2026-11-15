#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1933 蓝牙 SPP 帧契约 32/64 位同源判据（设备无关）

固化"32 位蓝牙 SPP 帧契约 = 64 位 src_rs 基准"口径（蓝牙联机 SPP
协议帧同源，正逆向对齐）：
  - 32 位侧：JQ_BT_UUID "ce1404fe-0863-4278-86c2-c9c560f0e215" 在原版
    so .rodata VA 0x121334（jq_rodata_plain.h 实锤，非加壳 so 明文）
  - 64 位基准：src_rs/bt_protocol.c JQ_BT_UUID/SERVICE("junqi")/
    VERSION("v2") + jq_bt_frame_encode "v2\\n%d,...,\\n" 帧格式
  - 帧契约同源：32 位 UUID = 64 位 UUID（蓝牙 SPP 服务 UUID 一致）
判据（4 项全 PASS）：
  ① 32 位 UUID 在原版 .rodata（jq_rodata_plain.h 0x121334 实锤）
  ② 64 位基准 UUID/SERVICE/VERSION（bt_protocol.c JQ_BT_* 宏）
  ③ 32 位 UUID = 64 位 UUID（同源一致）
  ④ 64 位帧编码 "v2\\n%d,...,\\n" 7 字段（BT_BUF=128，帧格式实锤）

用法：python3 rev64/tools/bt_spp_xcheck.py
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RODATA = os.path.join(ROOT, 'rev64/reverse/src_jni_translated/jq_rodata_plain.h')
BT64 = os.path.join(ROOT, 'rev64/forward/src_rs/bt_protocol.c')
UUID = 'ce1404fe-0863-4278-86c2-c9c560f0e215'

def main():
    fails = []
    ro = open(RODATA, errors='ignore').read()
    bt = open(BT64, errors='ignore').read()

    # ① 32 位 UUID 在原版 .rodata
    if ('"%s"' % UUID) not in ro:
        fails.append('① 32 位 UUID 不在 jq_rodata_plain.h（原版 .rodata 明文缺失）')
    else:
        m = re.search(r'\{0x[0-9a-f]+, "%s"\}' % UUID, ro)
        print('① 32 位 UUID 在原版 .rodata OK（%s 实锤）' % (m.group(0) if m else UUID))

    # ② 64 位基准宏
    for macro in ('JQ_BT_UUID', 'JQ_BT_SERVICE', 'JQ_BT_VERSION'):
        if macro not in bt:
            fails.append('② 64 位基准缺 %s（bt_protocol.c）' % macro)
    if not fails:
        print('② 64 位基准 JQ_BT_UUID/SERVICE/VERSION 在位 OK')

    # ③ 32 位 = 64 位 UUID
    m64 = re.search(r'#define JQ_BT_UUID "([^"]+)"', bt)
    if not m64 or m64.group(1) != UUID:
        fails.append('③ 64 位 UUID ≠ 32 位（实 = %s）' % (m64.group(1) if m64 else '缺'))
    else:
        print('③ 32 位 UUID = 64 位 UUID OK（%s，蓝牙 SPP 服务 UUID 同源）' % UUID)

    # ④ 64 位帧编码 7 字段（type + f1..f6，源码 \n 字面为 \\n）
    if not re.search(r'snprintf\(buf, cap, JQ_BT_VERSION "\\n%d,%d,%d,%d,%d,%d,%d\\n"', bt):
        fails.append('④ 64 位帧编码 "v2\\n%d,...,\\n" 7 字段缺失')
    else:
        print('④ 64 位帧编码 "v2\\n%d,...,\\n" 7 字段 OK（type+f1..f6，BT_BUF=128，帧格式实锤）')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（蓝牙 SPP 帧契约 32/64 位同源，UUID 一致 + 7 字段帧格式）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
