#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""APK 包名改 com.cnvcs.junqirs → com.zxj.junqirs（逆向 app 规范名，本会话职责）

4 处同步（AGENTS 红线，缺一即 UnsatisfiedLinkError/ClassNotFound）：
  ① smali 类包路径 com/cnvcs → com/zxj + 文件内容引用
  ② AndroidManifest.xml package + 组件名
  ③ native so 内 FindClass 字符串 com/cnvcs/App → com/zxj/App（二进制 patch）
  ④ JNI 导出符号 Java_com_cnvcs_App_* → Java_com_zxj_App_*（ELF dynsym 字符串 patch）

★ 等长 patch 技巧：com/cnvcs/App(15B) → com/zxj/App(12B)，短 3 字节 → 用 \0 填充
  剩余字节（dynsym 字符串表条目是变长 \0 结尾，缩短合法）；Java_com_cnvcs_App_OnCreate
  → Java_com_zxj_App_OnCreate 短 3 → 同样 \0 填充。ELF 符号表偏移/哈希不变（dynsym
  的 st_name 指向字符串表偏移，字符串内容缩短不影响其它符号）。
  ⚠ 但 DT_HASH/GNU_HASH 基于符号名计算——改字符串会破坏 hash → dlsym/dlsym 查找
  可能失败！Android linker 解析 JNI 用 dlsym("Java_com_zxj_App_OnCreate") → 依赖
  hash。需验证：改后 nm -D 能看到新符号 + 真机 loadLibrary 成功才算过。

用法：python3 rev64/tools/rename_cnvcs_zxj.py <解包目录>
"""
import os, sys, shutil, re

def patch_smali(root):
    """① smali 目录迁移 + 文本替换"""
    smali = os.path.join(root, 'smali')
    src = os.path.join(smali, 'com', 'cnvcs')
    dst = os.path.join(smali, 'com', 'zxj')
    if os.path.isdir(src):
        if os.path.isdir(dst):
            shutil.rmtree(dst)
        shutil.move(src, dst)
    n = 0
    for dp, _, fs in os.walk(root):
        for f in fs:
            if not f.endswith(('.smali', '.xml')):
                continue
            p = os.path.join(dp, f)
            try:
                s = open(p, encoding='utf-8').read()
            except Exception:
                continue
            new = s.replace('com/cnvcs', 'com/zxj').replace('com.cnvcs', 'com.zxj')
            if new != s:
                open(p, 'w', encoding='utf-8').write(new)
                n += 1
    return n

def patch_manifest(root):
    """② AndroidManifest package + 组件名（① 已含 .xml 替换，核对即可）"""
    p = os.path.join(root, 'AndroidManifest.xml')
    s = open(p, encoding='utf-8').read()
    return 'com.zxj' if 'package="com.zxj.junqirs"' in s else 'FAIL'

def patch_so(root):
    """③④ so 内 FindClass 字符串 + JNI 导出符号（等长 \0 填充）"""
    hits = []
    for dp, _, fs in os.walk(os.path.join(root, 'lib')):
        for f in fs:
            if not f.endswith('.so'):
                continue
            p = os.path.join(dp, f)
            data = bytearray(open(p, 'rb').read())
            # ★ 等长 patch：整串替换后**尾部补 \0**（不在中间插 \0！中间插会把
            #   OnCreate 等后缀截断 → 14 符号全变 Java_com_zxj_App_ → JNI 绑定失败）
            #   正确：Java_com_cnvcs_App_OnCreate → Java_com_zxj_App_OnCreate + \0\0\0
            for old, new in (
                (b'com/cnvcs/App', b'com/zxj/App'),
                (b'com/cnvcs/GLView', b'com/zxj/GLView'),
            ):
                padded = new + b'\x00' * (len(old) - len(new))
                pos = 0
                while True:
                    i = data.find(old, pos)
                    if i < 0:
                        break
                    data[i:i+len(padded)] = padded
                    hits.append('%s: %s→%s @0x%x' % (f, old.decode(), new.decode(), i))
                    pos = i + len(padded)
            # JNI 符号：整符号替换（含 OnXxx 后缀），尾部补 \0
            # 用正则匹配 Java_com_cnvcs_App_ + 合法标识符，整体迁移
            pat = re.compile(rb'Java_com_cnvcs_App_[A-Za-z0-9_]+')
            for m in list(pat.finditer(bytes(data))):
                old = m.group()
                new = old.replace(b'com_cnvcs', b'com_zxj')
                padded = new + b'\x00' * (len(old) - len(new))   # 短 3 → 尾补 3
                data[m.start():m.end()] = padded
                hits.append('%s: %s→%s @0x%x' % (f, old.decode()[:30], new.decode()[:30], m.start()))
            open(p, 'wb').write(data)
    return hits

def main():
    if len(sys.argv) < 2:
        print('用法: python3 rename_cnvcs_zxj.py <解包目录>'); return 2
    root = sys.argv[1]
    print('== APK 改名 com.cnvcs.junqirs → com.zxj.junqirs ==')
    n = patch_smali(root)
    print('① smali 改文件数:', n)
    m = patch_manifest(root)
    print('② Manifest:', m)
    hits = patch_so(root)
    print('③④ so patch 次数:', len(hits))
    for h in hits[:8]:
        print('   ', h)
    print('下一步：apktool b → STORE → zipalign → 双签 → 装机验证（nm -D 查 Java_com_zxj_App_*）')
    return 0

if __name__ == '__main__':
    sys.exit(main())
