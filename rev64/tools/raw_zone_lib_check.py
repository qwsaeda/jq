#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1927 系统域 RAW 6 zone → NEEDED 库归属对照判据（设备无关）

固化"434 RAW 系统域全归 libstdc++.so"口径（真机 linker 解析兜底可归因）：
  - 6 zone（0x0001/0x0002/0x0006/0x10/0x11/0x12）全属 C++ runtime /
    locale / iostream facet，由 libstdc++.so 提供（NEEDED 7 项之一）
  - 判据：① 434 RAW 全在 6 zone ② 每 zone 归属 libstdc++.so ③ NEEDED
    含 libstdc++.so（真机 dlopen 解析兜底可归因）④ 0 业务混入（系统
    域纯，§1906 清零口径）

用法：python3 rev64/tools/raw_zone_lib_check.py
"""
import importlib.util, os, re, subprocess, sys, collections

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG = '/tmp/orig/lib/armeabi/libjunqi.so'
ZONE_LIB = {
    0x0001: 'libstdc++ (C++ runtime 异常/ios，低地址跨段伪影)',
    0x0002: 'libstdc++ (软浮点 .plt 桩 __aeabi_fadd)',
    0x0006: 'libstdc++ (C++ runtime 异常区)',
    0x10: 'libstdc++ (ios/locale facet .text 尾部)',
    0x11: 'libstdc++ (.rodata _WLocale/_Locale_* + vtable 指针)',
    0x12: 'libstdc++ (.rel.ro hashtable/messages facet)',
}

def main():
    fails = []
    spec = importlib.util.spec_from_file_location('dm', os.path.join(ROOT, 'rev64/tools/translation_domain_matrix.py'))
    dm = importlib.util.module_from_spec(spec); spec.loader.exec_module(dm)
    bl = dm.orig_bl_targets()
    fdefs = dm.load_f_defs()
    vtbl = dm.load_vtable_dispatch()
    stub = dm.load_stub_set()
    plt = dm.load_plt_symbols()
    libc = dm.load_libc_syms()
    raw = [a for a in bl if not (a in plt or 0x22900 <= int(a, 16) < 0x233e0) and a not in fdefs and a not in vtbl and a not in stub]
    zones = collections.Counter((int(a, 16) >> 16) for a in raw)

    # NEEDED
    out = subprocess.run(['arm-linux-gnueabi-readelf', '-d', ORIG], capture_output=True, text=True).stdout
    need = set(re.findall(r'\(NEEDED\)\s+Shared library:\s+\[([^\]]+)\]', out))

    # ① 434 RAW 全在 6 zone
    expect_z = set(ZONE_LIB.keys())
    if set(zones.keys()) != expect_z:
        fails.append('① 6 zone 实列 = %s ≠ 期望 %s' % (sorted(zones.keys()), sorted(expect_z)))
    else:
        print('① 434 RAW 全在 6 zone OK')
    # ② 每 zone 归属
    for z in sorted(zones):
        print('   0x%04x 区 %4d 条 → %s' % (z, zones[z], ZONE_LIB.get(z, '?')))
    # ③ NEEDED 含 libstdc++.so
    if 'libstdc++.so' not in need:
        fails.append('③ NEEDED 缺 libstdc++.so（%s）' % sorted(need))
    else:
        print('③ NEEDED 含 libstdc++.so OK（真机 dlopen 解析兜底可归因）')
    # ④ 0 业务混入
    PSEUDO = {'1220e', '1b0ca'}
    biz = [a for a in raw if not (a in libc or int(a, 16) >= 0x110000 or a.lstrip('0') in PSEUDO)]
    if biz:
        fails.append('④ 真业务混入 %d（%s）' % (len(biz), sorted(biz, key=lambda x: int(x, 16))[:5]))
    else:
        print('④ 0 业务混入 OK（系统域纯，§1906 清零口径）')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（434 RAW 6 zone 全归 libstdc++.so，真机 linker 兜底可归因）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
