#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-216 32 位翻译体 apk 内 so 哈希一致性判据（误打包坑兜底，静态侧）

固化"重编 so 后必须重打包 apk 才装机"口径（§124y-176 误打包坑）的可复跑判据：
- rev64/dist/phase_chain_arm32.apk 内 lib/armeabi/libjunqi.so 的 md5
- rev64/dist/libjunqi_arm32_aligned.so 的 md5
- 判据：两者 md5 一致（so 哈希 = apk 内 so 哈希，误打包未发生）

用法：python3 rev64/tools/apk_so_hash_check.py
"""
import hashlib, os, sys, zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
APK = os.path.join(ROOT, "rev64/dist/phase_chain_arm32.apk")
SO = os.path.join(ROOT, "rev64/dist/libjunqi_arm32_aligned.so")

def md5(p):
    return hashlib.md5(open(p,'rb').read()).hexdigest()

def main():
    for p in (APK, SO):
        if not os.path.exists(p):
            print("✗ %s 不存在" % p); return 2
    so_md5 = md5(SO)
    with zipfile.ZipFile(APK) as z:
        apk_so_name = None
        for n in z.namelist():
            if n.endswith('libjunqi.so') and 'armeabi' in n:
                apk_so_name = n; break
        if not apk_so_name:
            print("✗ apk 内无 lib/armeabi/libjunqi.so"); return 2
        apk_so_md5 = hashlib.md5(z.read(apk_so_name)).hexdigest()
    same = so_md5 == apk_so_md5
    print("═══ §124y-216 32 位翻译体 apk 内 so 哈希一致性判据 ═══")
    print("  apk 内 %s md5: %s" % (apk_so_name, apk_so_md5))
    print("  dist  so  md5: %s" % so_md5)
    print("  一致: %s" % ("PASS（未误打包）" if same else "FAIL（误打包：so 重编后未重打包 apk）"))
    print("  （坑固化 §124y-176：重编 so 后必须重打包 apk 才装机，so+apk 哈希一致性验证）")
    return 0 if same else 2

if __name__ == '__main__':
    sys.exit(main())
