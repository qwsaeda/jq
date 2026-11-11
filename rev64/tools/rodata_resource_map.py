#!/usr/bin/env python3
"""rodata_resource_map.py — 原版 so .rodata 资源引用 ↔ SPAK 解包产物 静态映射

判据（用户"apk 资源当佐证"指令，静态路线）：
  ① 原版 32 位 so .rodata 静态提取 data/* 路径引用（strings 级）
  ② SPAK 解包产物（rev64/dist/spak_res/）按 PNG 魔数 + IHDR 尺寸 / JPG 魔数 特征建索引
  ③ 按尺寸特征匹配：737x751 棋盘 / 212x280 棋子 / 1100x1121 / 344x427 高分套等
输出：映射清单（.rodata 资源 → SPAK 解包文件 + 尺寸特征）

用法:
  python3 rev64/tools/rodata_resource_map.py [--so /tmp/opencode/orig_so/lib/armeabi/libjunqi.so]
"""
import os, re, struct, sys, argparse

def spak_index(res_dir):
    by_size = {}
    for f in sorted(os.listdir(res_dir)):
        m = re.match(r'^(data|datah|datax|datam)_\d+_([0-9a-f]+)\.bin$', f)
        if not m:
            continue
        d = open(os.path.join(res_dir, f), 'rb').read()
        if d[:8] == b'\x89PNG\r\n\x1a\n':
            w, h = struct.unpack('>II', d[16:24])
            by_size.setdefault((w, h), []).append(f)
        elif d[:2] == b'\xff\xd8':
            by_size.setdefault(('JPG',), []).append(f)
    return by_size

def rodata_paths(so_path):
    so = open(so_path, 'rb').read()
    return sorted(set(m.group(1).decode() for m in re.finditer(rb'(data/[a-zA-Z0-9/._%-]+)', so)))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--so', default='/tmp/opencode/orig_so/lib/armeabi/libjunqi.so')
    ap.add_argument('--res', default='/disk/workspaces/jq/rev64/dist/spak_res')
    a = ap.parse_args()

    idx = spak_index(a.res)
    paths = rodata_paths(a.so)

    # 尺寸特征匹配表（.rodata 资源 → SPAK 解包尺寸特征，静态佐证）
    FEATURE = {
        'data/sg/board.png': (737, 751),
        'data/junqi/piece.png': (212, 280),
        'data/sg/boardx.png': (1100, 1121),
        'data/sg/piecex.png': (344, 427),
    }
    print('=== 原版 .rodata 资源引用 ↔ SPAK 解包产物（尺寸特征静态映射）===')
    for p in paths:
        base = os.path.basename(p)
        if p in FEATURE:
            feat = FEATURE[p]
            hit = idx.get(feat, [])
            print(f'  {p} = PNG {feat[0]}x{feat[1]} → {hit}')
        elif base.endswith('.jpg') or p in ('data/720', 'data/800'):
            # jpg 套图/分辨率套：按 JPG 魔数命中
            n = len(idx.get(('JPG',), []))
            print(f'  {p} = JPG（解包 {n} 个 JPG bin，datax_294 等）')
        else:
            print(f'  {p} = （未做尺寸映射，按内容类型人工核对）')

if __name__ == '__main__':
    main()
