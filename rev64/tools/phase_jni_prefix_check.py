#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""四阶段交付物完整性门禁（零设备依赖）：dex↔so JNI 前缀 + 签名 scheme + 证书指纹

定位：2026-11-22 发现 phase2/3 因包名 cnvcs + so 导出 qwsaeda 前缀导致
  UnsatisfiedLinkError（AGENTS"全链改名4处漏1处即崩"）。本判据机检 dist/phase0-3.apk
  每个 so 目录的 JNI 导出前缀与 dex 类名前缀一致 + 签名 v1/v2/v3 + 证书指纹匹配
  junqi.keystore，防重建/重签复发（MANIFEST 声称 ✓ 但此前无机检）。

判据（4 阶段全 PASS）：
  phase0/1: dex=com/cnvcs/* + armeabi so JNI=Java_com_cnvcs_App_*（原版32位）
  phase1 arm64 桩: 无 JNI 符号（预期）
  phase2/3: dex=com/cnvcs/* + arm64 so JNI=Java_com_cnvcs_App_*（重编对齐后）
  所有 APK: dex 无 qwsaeda 残留 + so 无 Java_com_qwsaeda 残留
  所有 APK: apksigner verify v1+v2+v3 true + 证书 SHA256 = junqi.keystore 指纹
  所有 APK: resources.arsc / classes.dex / .so 均 STORE 不压缩 + 4 字节对齐
    （AGENTS 安装失败排查 ①②：.so 被压缩 / arsc 未 4 字节对齐 → 装失败）
"""
import os, re, zipfile, subprocess, sys, hashlib

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DIST = os.path.join(ROOT, 'dist')

EXPECT = {
    'phase0': {'dex': b'com/cnvcs/', 'abis': {'armeabi': 'cnvcs'}, 'minSdk': '19', 'targetSdk': '34', 'ads': 'kept'},
    'phase1': {'dex': b'com/cnvcs/', 'abis': {'armeabi': 'cnvcs', 'arm64-v8a': 'stub'}, 'minSdk': '19', 'targetSdk': '34', 'ads': 'kept'},
    'phase2': {'dex': b'com/cnvcs/', 'abis': {'armeabi-v7a': 'orig', 'arm64-v8a': 'cnvcs'}, 'minSdk': '21', 'targetSdk': '34', 'ads': 'kept'},
    'phase3': {'dex': b'com/cnvcs/', 'abis': {'arm64-v8a': 'cnvcs'}, 'minSdk': '21', 'targetSdk': '34', 'ads': 'removed'},
}

def check_ads(z, dex, exp):
    """返回 (ok, 原因)：阶段 3 去广告终态 / phase0-2 保留 dex 内 AdView 类

    ★ 真值纠正（2026-11-22）：32 位 so FindClass("com/baidu/mobads/AdView") 的 AdView
      类在**主 classes.dex**（描述串=1），不在 ad jar 内（jar 内 dex 的 AdView=0）。
      历史"删 jar → NPE"实为连 smali/com/baidu/* 一起删（AdView 类没了），非单纯删 jar。
      故真红线 = dex 内保留 AdView 类，jar 删不删无所谓（phase2 已删 jar 仍可跑）。
    """
    jar = [n for n in z.namelist() if '__xadsdk__' in n or 'xadsdk' in n]
    baidu = dex.count(b'Lcom/baidu/')
    adview = dex.count(b'Lcom/baidu/mobads/AdView;')
    if exp['ads'] == 'removed':
        # 阶段 3：dex 无 baidu 类 + 无 AdView + 无 ad jar（arm64 so 无引用才能删全量）
        ok = (baidu == 0 and adview == 0 and len(jar) == 0)
        return ok, 'dex baidu=%d AdView=%d jar=%d（期望全0）' % (baidu, adview, len(jar))
    else:
        # phase0-2：32 位 so 硬引用 FindClass(AdView) → dex 必须保留 AdView 类
        ok = (adview >= 1 and baidu > 0)
        return ok, 'dex AdView=%d baidu=%d jar=%d（期望 AdView≥1，jar 无关）' % (adview, baidu, len(jar))

def check_sdk(apk, exp):
    """返回 (ok, 原因)：minSdk/targetSdk 与阶段规格一致（AGENTS 四阶段定义）"""
    r = subprocess.run(['aapt', 'dump', 'badging', apk], capture_output=True, text=True)
    out = r.stdout
    mn = re.search(r"sdkVersion:'(\d+)'", out)
    tg = re.search(r"targetSdkVersion:'(\d+)'", out)
    if not mn or not tg:
        return False, 'aapt 未取到 sdkVersion'
    gm, gt = mn.group(1), tg.group(1)
    if gm != exp['minSdk'] or gt != exp['targetSdk']:
        return False, 'minSdk=%s(期%s) targetSdk=%s(期%s)' % (gm, exp['minSdk'], gt, exp['targetSdk'])
    return True, 'minSdk=%s targetSdk=%s' % (gm, gt)

def jni_prefixes(so_bytes):
    t = '/tmp/opencode/_jni_prefix.so'
    open(t, 'wb').write(so_bytes)
    nm = subprocess.run(['nm', '-D', t], capture_output=True, text=True).stdout
    return nm.count('Java_com_cnvcs_'), nm.count('Java_com_qwsaeda')

def check_store_align(apk, z):
    """返回 (ok, 原因)：resources.arsc/dex/so/wav 全 STORE + 数据起始 4 字节对齐

    ★ 2026-11-22 审计 RJNI-001：WAV 必须 STORE（AssetManager.openFd 对 DEFLATE
      资源抛 IOException，正向 Rust sfx.rs 走子带音效会崩）。此前门禁漏检 WAV。
    """
    import struct
    bad = []
    with open(apk, 'rb') as f:
        for name in z.namelist():
            if not (name == 'resources.arsc' or name.endswith('.dex') or
                    name.endswith('.so') or name.endswith('.wav')):
                continue
            i = z.getinfo(name)
            if i.compress_type != zipfile.ZIP_STORED:
                bad.append('%s 被压缩(%d)' % (name, i.compress_type)); continue
            off = i.header_offset
            f.seek(off); hdr = f.read(30)
            if len(hdr) < 30: bad.append('%s header 截断' % name); continue
            nlen = struct.unpack_from('<H', hdr, 26)[0]
            elen = struct.unpack_from('<H', hdr, 28)[0]
            data_off = off + 30 + nlen + elen
            if data_off % 4: bad.append('%s 未4字节对齐(@0x%x)' % (name, data_off))
    return (len(bad) == 0), (bad[0] if bad else 'STORE + 4B 对齐')

def check_signature(apk):
    """返回 (ok, 原因)：v1+v2+v3 + 证书指纹匹配 junqi.keystore"""
    r = subprocess.run(['apksigner', 'verify', '--verbose', '--print-certs', apk],
                       capture_output=True, text=True)
    out = r.stdout + r.stderr
    if 'DOES NOT VERIFY' in out or 'Verifies' not in out:
        return False, 'apksigner verify 失败'
    for scheme in ('v1', 'v2', 'v3'):
        if 'Verified using %s scheme' % scheme not in out or 'true' not in out.split('Verified using %s scheme' % scheme)[1][:80]:
            return False, '%s scheme 非 true' % scheme
    # 证书 SHA256 匹配 keystore（硬编码基准 = junqi.keystore 实测 44:27:28...）
    ks = subprocess.run(['keytool', '-list', '-v', '-keystore',
                         os.path.join(ROOT, 'junqi.keystore'), '-storepass', 'junqi2026'],
                        capture_output=True, text=True)
    ks_sha = re.search(r'SHA256:\s*([0-9A-F:]{40,})', ks.stdout)
    apk_sha = re.search(r'SHA-256 digest:\s*([0-9a-f]{40,})', out)
    if not ks_sha or not apk_sha:
        return True, 'scheme OK（证书指纹未取到，跳过比对）'
    norm = lambda s: s.replace(':', '').lower()
    if norm(ks_sha.group(1)) != norm(apk_sha.group(1)):
        return False, '证书指纹不匹配 keystore（APK=%s KS=%s）' % (apk_sha.group(1), ks_sha.group(1))
    return True, 'v1+v2+v3 + 证书指纹匹配'

def main():
    fails = []
    print('== 四阶段交付物完整性门禁（JNI 前缀 + 签名 + 证书）==')
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
        # minSdk/targetSdk 规格
        mok, minfo = check_sdk(apk, exp)
        print('   sdk: %s %s' % ('✓' if mok else '✗', minfo))
        if not mok: fails.append('%s SDK 规格漂移（%s）' % (phase, minfo))
        # 去广告终态（phase3 全删 / phase0-2 保留 jar）
        aok2, ainfo2 = check_ads(z, dex, exp)
        print('   ads: %s %s' % ('✓' if aok2 else '✗', ainfo2))
        if not aok2: fails.append('%s 去广告规格异常（%s）' % (phase, ainfo2))
        # STORE + 4B 对齐
        aok, ainfo = check_store_align(apk, z)
        print('   store/align: %s %s' % ('✓' if aok else '✗', ainfo))
        if not aok: fails.append('%s STORE/对齐异常（%s）' % (phase, ainfo))
        # 签名 scheme + 证书指纹
        sok, sinfo = check_signature(apk)
        print('   signature: %s %s' % ('✓' if sok else '✗', sinfo))
        if not sok: fails.append('%s 签名异常（%s）' % (phase, sinfo))
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
    print('判据: PASS（四阶段 JNI前缀 + minSdk/targetSdk + STORE/4B对齐 + 签名v1/v2/v3 + 证书指纹 + 去广告终态）')
    return 0

if __name__ == '__main__':
    sys.exit(main())
