#!/usr/bin/env python3
"""
spak_unpack.py — 军棋 SPAK 资源包解包器（R5）

SPAK 算法（已逆向确认）：
  1. 每个 .pak 组是"合并流"：data0+data1、datah0+datah1、datam、datax0..3。
  2. 24 字节 SPAK 头：magic"SPAK"(4) + version(4) + f1(4) + f2(4) + f3(4) + f4(4)。
     - f1 = 文件表偏移（合并流内，f1+f2 = 合并流大小，即文件表在末尾）
     - f2 = 文件表加密+压缩长度
     - f3 = 文件表校验（c3238，未用）
     - f4 = 文件表解压后大小 = 条目数 × 28
  3. 文件表 = XOR(key[j%32]) 解密 + zlib 解压 → N×28 字节。
  4. 28 字节条目 = hash[16] + off[4] + size[4] + usize[4]。
     - hash：资源内容标识（16 字节，hex 编码为 32 字符）
     - off：数据区偏移（数据区 = 合并流[24:f1]，off 从 24 起，offset[i]=offset[i-1]+size[i-1]）
     - size：加密+压缩大小；usize：解压后大小
  5. 数据资源 = 数据区[off-24:off-24+size]，XOR(key[j%32]) 解密，zlib 解压（部分未压缩）。

SPAK key（32 字节，明文态，位于原版 .rodata VA 0x12a658，redroid dump 确认）：
  c2cbe5e7ce5c4ac411ca6c9906f476f86d4fcbb62621de0e6a67874c93f0d523

用法：python3 spak_unpack.py <assets_dir> <out_dir>
"""
import zlib, struct, os, sys, re

SPAK_KEY = bytes.fromhex('c2cbe5e7ce5c4ac411ca6c9906f476f86d4fcbb62621de0e6a67874c93f0d523')

# (组名, 成员列表, 文件表偏移 f1, 文件表长度 f2)
GROUPS = [
    ('data',  ['data0.pak', 'data1.pak'], 0x12d4f3, 2712),
    ('datah', ['datah0.pak', 'datah1.pak'], 0x155db2, 6571),
    ('datam', ['datam.pak'], 0x6e6f9, 4572),
    ('datax', ['datax0.pak', 'datax1.pak', 'datax2.pak', 'datax3.pak'], 0x2f3f86, 7278),
]


def xor_dec(data, key=SPAK_KEY):
    """data[j] ^= key[j%32]"""
    return bytes(b ^ key[i % 32] for i, b in enumerate(data))


def unpack_group(assets, group, members, f1, f2):
    blob = b''.join(open(os.path.join(assets, m), 'rb').read() for m in members)
    assert len(blob) == f1 + f2, f'{group}: 合并流大小 {len(blob)} != f1+f2={f1+f2}'
    tbl = blob[f1:f1 + f2]
    dec = xor_dec(tbl)
    out = zlib.decompress(dec)
    n = len(out) // 28
    assert len(out) == n * 28, f'{group}: 文件表不是 28 的倍数'
    dataregion = blob[24:f1]
    entries = []
    prev = 24
    for i in range(n):
        e = out[i * 28:(i + 1) * 28]
        h = e[0:16]
        off, size, usize = struct.unpack_from('<III', e, 16)
        assert off == prev, f'{group}#{i}: offset 链断裂 off={off} prev={prev}'
        prev = off + size
        raw = dataregion[off - 24: off - 24 + size]
        dec2 = xor_dec(raw)
        try:
            data = zlib.decompress(dec2)
        except zlib.error:
            data = dec2  # 未压缩
        entries.append((group, i, h, off, size, usize, len(data), data))
    return entries


def main():
    assets = sys.argv[1] if len(sys.argv) > 1 else '/tmp/opencode/jq_apk/dec/assets'
    outdir = sys.argv[2] if len(sys.argv) > 2 else '/disk/workspaces/jq/rev64/dist/spak_res'
    os.makedirs(outdir, exist_ok=True)
    all_entries = []
    for group, members, f1, f2 in GROUPS:
        entries = unpack_group(assets, group, members, f1, f2)
        print(f'[{group}] {len(entries)} 项解包成功')
        for g, i, h, off, size, usize, actual, data in entries:
            hexh = h.hex()
            fn = os.path.join(outdir, f'{g}_{i:03d}_{hexh[:16]}.bin')
            with open(fn, 'wb') as f:
                f.write(data)
            all_entries.append((g, i, hexh, off, size, usize, actual, fn))
    # manifest
    mf = os.path.join(outdir, 'manifest.tsv')
    with open(mf, 'w') as f:
        f.write('group\tidx\thash\toff\tsize\tusize\tactual\tfile\n')
        for e in all_entries:
            f.write('\t'.join(map(str, e)) + '\n')
    print(f'总计 {len(all_entries)} 资源 → {outdir}')
    print(f'清单: {mf}')


if __name__ == '__main__':
    main()
