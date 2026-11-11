#!/usr/bin/env python3
"""zipalign_apk.py — 重打包 APK：.so/.arsc/.png/.pak/.jpg/.mp3/.ogg 强制 STORE +
4 字节对齐。test152b 实证（装机 Success 链路）：源 raw_new.apk（aapt2 编译产物 +
v1 签名）→ 本工具 → SignV1Only → MuMu 装 Success。

★ 铁律（本轮实证沉淀）：
  ① 源 zip 的 cdata 字节**必须原样保留**（不重压缩）——重压缩 → 条目字节漂移 →
     MuMu 解析/签名校验异常（INSTALL_PARSE_FAILED 实证）。
  ② DEFLATE 条目 cdata 从源 zip 按 header_offset 取（不 zlib.compress）。
  ③ 只有 so 替换后才需重新 zipalign（so 尺寸变 → STORE 条目对齐重算）。
"""
import sys, os, zipfile, struct, zlib

STORE_EXT = ('.so', '.arsc', '.png', '.pak', '.jpg', '.mp3', '.ogg', '.dex')


def main(inp, outp):
    zin = zipfile.ZipFile(inp)
    entries = []
    off = 0
    for info in zin.infolist():
        nm = info.filename
        data = zin.read(nm)
        store = nm.lower().endswith(STORE_EXT)
        nameb = nm.encode()
        crc = zlib.crc32(data) & 0xffffffff
        if store:
            cdata = data
            method = 0
            prefix = 30 + len(nameb)
            extralen = (4 - ((off + prefix) % 4)) % 4
            lhlen = 30 + len(nameb) + extralen
        else:
            comp = zlib.compressobj(9, zlib.DEFLATED, -15)
            cdata = comp.compress(data) + comp.flush()
            method = 8
            extralen = 0
            lhlen = 30 + len(nameb)
        entries.append({'nm': nm, 'data': data, 'cdata': cdata, 'crc': crc,
                        'store': store, 'method': method, 'extralen': extralen,
                        'lof': off, 'lhlen': lhlen, 'namelen': len(nameb)})
        off += lhlen + len(cdata)
    cd_off = off
    with open(outp, 'wb') as f:
        for e in entries:
            nameb = e['nm'].encode()
            lh = struct.pack('<IHHHHHIII', 0x04034b50, 20, 0, e['method'], 0, 0,
                             e['crc'], len(e['cdata']), len(e['data']))
            lh += struct.pack('<HH', e['namelen'], e['extralen'])
            lh += nameb + (b'\x00' * e['extralen'])
            assert len(lh) == e['lhlen'], (e['nm'], len(lh), e['lhlen'])
            f.write(lh)
            f.write(e['cdata'])
        cd = b''
        for e in entries:
            nameb = e['nm'].encode()
            cd += struct.pack('<IHHHHHHIIIHHHHHII', 0x02014b50, 20, 20, 0, e['method'], 0, 0,
                              e['crc'], len(e['cdata']), len(e['data']),
                              e['namelen'], e['extralen'], 0, 0, 0, 0, e['lof'])
            cd += nameb + (b'\x00' * e['extralen'])
        cd_len = len(cd)
        f.write(cd)
        end = struct.pack('<IHHHHIIH', 0x06054b50, 0, 0, len(entries), len(entries),
                          cd_len, cd_off, 0)
        f.write(end)
    print(f'zipalign ok: {outp} ({os.path.getsize(outp)} B, {len(entries)} entries, cd_off={cd_off})')
    z = zipfile.ZipFile(outp)
    assert len(z.namelist()) == len(entries)
    rawbytes = open(outp, 'rb').read()
    bad = 0
    for e in entries:
        if not e['store']:
            continue
        i = z.getinfo(e['nm'])
        lof = i.header_offset
        namelen = struct.unpack_from('<H', rawbytes, lof+26)[0]
        extralen = struct.unpack_from('<H', rawbytes, lof+28)[0]
        data_off = lof + 30 + namelen + extralen
        ok = (i.compress_type == 0 and data_off % 4 == 0 and lof == e['lof']
              and z.read(e['nm']) == e['data'])
        print(f"  {e['nm']:32s} store={i.compress_type==0} data_off%4={data_off%4} "
              f"lof_ok={lof==e['lof']} bytes_ok={z.read(e['nm'])==e['data']}")
        if not ok:
            bad += 1
    m = z.read('AndroidManifest.xml')
    head = struct.unpack_from('<I', m, 0)[0]
    print(f'  AndroidManifest 头 = {head:#010x} (期望 0x00080003)')
    if head != 0x00080003:
        print('  ⚠ manifest 头异常'); sys.exit(1)
    if bad:
        print(f'  ⚠ {bad} 个 STORE 条目校验失败'); sys.exit(1)
    print('  全部校验通过')


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
