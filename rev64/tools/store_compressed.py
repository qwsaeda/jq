#!/usr/bin/env python3
"""强制 .so/.arsc/.png/.pak/.wav 为 STORE（不压缩）—— apktool 默认会压缩，装必失败。
用法: python3 store_compressed.py in.apk out.apk

★ 2026-11-22 审计 RJNI-001：WAV 必须 STORE——AssetManager.openFd() 对压缩资源
  抛 IOException（openFd 要求数据连续/已对齐，DEFLATE 不满足）。正向 Rust 版
  sfx.rs:40 用 openFd 播放 28 个 WAV，此前全 DEFLATE → 每次走子带音效崩溃。"""
import sys, zipfile
STORE_EXTS = ('.so', '.arsc', '.png', '.pak', '.dex', '.wav')
def main():
    src, dst = sys.argv[1], sys.argv[2]
    zin = zipfile.ZipFile(src, 'r')
    zout = zipfile.ZipFile(dst, 'w')
    for info in zin.infolist():
        data = zin.read(info.filename)
        newinfo = zipfile.ZipInfo(info.filename, date_time=info.date_time)
        if any(info.filename.endswith(e) for e in STORE_EXTS):
            newinfo.compress_type = zipfile.ZIP_STORED
        else:
            newinfo.compress_type = zipfile.ZIP_DEFLATED
        newinfo.external_attr = info.external_attr
        zout.writestr(newinfo, data)
    zout.close()
    zin.close()
    print('stored: %s' % dst)
if __name__ == '__main__':
    main()
