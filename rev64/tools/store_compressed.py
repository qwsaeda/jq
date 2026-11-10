#!/usr/bin/env python3
"""强制 .so/.arsc/.png/.pak 为 STORE（不压缩）—— apktool 默认会压缩，装必失败。
用法: python3 store_compressed.py in.apk out.apk"""
import sys, zipfile
STORE_EXTS = ('.so', '.arsc', '.png', '.pak', '.dex')
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
