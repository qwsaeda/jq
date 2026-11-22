#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""四阶段交付物 dex↔so JNI 前缀一致性门禁（零设备依赖）

定位：2026-11-22 发现 phase2/3 因包名 cnvcs + so 导出 qwsaeda 前缀导致
  UnsatisfiedLinkError（AGENTS"全链改名4处漏1处即崩"）。本判据机检 dist/phase0-3.apk
  每个 so 目录的 JNI 导出前缀与 dex 类名前缀一致，防重建复发。

判据（4 阶段全 PASS）：
  phase0/1: dex=com/cnvcs/* + armeabi so JNI=Java_com_cnvcs_App_*（原版32位）
  phase1 arm64 桩: 无 JNI 符号（预期）
  phase2/3: dex=com/cnvcs/* + arm64 so JNI=Java_com_cnvcs_App_*（重编对齐后）
  所有 APK: dex 无 qwsaeda 残留 + so 无 Java_com_qwsaeda 残留
"""
import os, zipfile, subprocess, sys, hashlib

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DIST = os.path.join(ROOT, 'dist')

EXPECT = {
    'phase0': {'dex': b'com/cnvcs/', 'abis': {'armeabi': 'cnvcs'}},
    'phase1': {'dex': b'com/cnvcs/', 'abis': {'armeabi': 'cnvcs', 'arm64-v8a': 'stub'}},
    'phase2': {'dex': b'com/cnvcs/', 'abis': {'armeabi-v7a': 'orig', 'arm64-v8a': 'cnvcs'}},
    'phase3': {'dex': b'com/cnvcs/', 'abis': {'arm64-v8a': 'cnvcs'}},
}

def jni_prefixes(so_bytes):
    t = '/tmp/opencode/_jni_prefix.so'
    open(t, 'wb').write(so_bytes)
    nm = subprocess.run(['nm', '-D', t], capture_output=True, text=True).stdout
    return nm.count('Java_com_cnvcs_'), nm.count('Java_com_qwsaeda')

def main():
    fails = []
    print('== 四阶段 dex↔so JNI 前缀一致性门禁 ==')
    for phase, exp in EXPECT.items():
        apk = os.path.join(DIST, phase + '.apk')
        if not os.path.isfile(apk):
            fails.append('%s APK 缺失' % phase); continue
        z = zipfile.ZipFile(apk)
        dex = z.read('classes.dex')
        cn = dex.count(exp['dex'])
        qw = dex.count(b'com/qwsaeda/')
        ok = cn >= 10 and qw == 0
        print('%s dex: cnvcs=%d qwsaeda=%d %s' % (phase, cn, qw, '✓' if ok else '✗'))
        if not ok: fails.append('%s dex 前缀异常（cnvcs=%d qwsaeda=%d）' % (phase, cn, qw))
        for abi, kind in exp['abis'].items():
            so_paths = [n for n in z.namelist() if n.startswith('lib/%s/' % abi) and n.endswith('.so')]
            if not so_paths:
                fails.append('%s/%s so 缺失' % (phase, abi)); continue
            c, q = jni_prefixes(z.read(so_paths[0]))
            if kind == 'stub':
                sok = (c == 0 and q == 0)
            elif kind == 'orig':
                sok = (c >= 14 and q == 0)
            else:  # cnvcs 重编
                sok = (c == 14 and q == 0)
            print('   %-14s JNI cnvcs=%d qwsaeda=%d %s' % (abi, c, q, '✓' if sok else '✗'))
            if not sok: fails.append('%s/%s JNI 前缀异常（cnvcs=%d qwsaeda=%d）' % (phase, abi, c, q))
    if fails:
        for f in fails: print('FAIL:', f)
        print('判据: FAIL'); return 2
    print('判据: PASS（四阶段 dex↔so JNI 前缀全一致，0 qwsaeda 残留）')
    return 0

if __name__ == '__main__':
    sys.exit(main())
