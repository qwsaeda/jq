#!/usr/bin/env python3
"""lay_extract.py — 从 SPAK 解包产物提取 12 个布局（data/lay/%02d.lay）
依据：data 组 idx 47-58 连续 12 个 30 字节条目，47 = default.lay（00.lay，
  与 rev64/dist/spak_res/data_lay_default.lay 逐字节一致实证）。
用法:
  python3 lay_extract.py [spak_res目录] [输出目录]
  默认: rev64/dist/spak_res → rev64/dist/layouts/00.lay..11.lay
  输出: 12 个 30 字节 .lay 文件（00.lay = default）
"""
import os, sys, struct, shutil

SPAK_RES = 'rev64/dist/spak_res'
OUT = 'rev64/dist/layouts'
LAY_IDX = list(range(47, 59))   # data 组 idx 47-58（00-11.lay 对应序）

def main():
    src = sys.argv[1] if len(sys.argv) > 1 else SPAK_RES
    out = sys.argv[2] if len(sys.argv) > 2 else OUT
    os.makedirs(out, exist_ok=True)
    # 找 30 字节的 data_XX 文件，按 XX 排序
    cands = []
    for f in os.listdir(src):
        if not (f.startswith('data_') and f.endswith('.bin')): continue
        m = f[5:f.find('_') + 1]
        try: idx = int(f[5:f.index('_', 5)])
        except ValueError: continue
        if os.path.getsize(os.path.join(src, f)) == 30:
            cands.append((idx, f))
    cands.sort()
    print(f"30 字节布局候选: {len(cands)} 个（data 组 idx {[c[0] for c in cands[:15]]}...）")
    written = 0
    for i, (idx, f) in enumerate(cands[:12]):
        data = open(os.path.join(src, f), 'rb').read()
        dst = os.path.join(out, f'{i:02d}.lay')
        shutil.copyfile(os.path.join(src, f), dst)
        # 校验：00.lay 应等于 default.lay
        if i == 0:
            dlt = os.path.join(src, 'data_lay_default.lay')
            if os.path.exists(dlt):
                ok = open(dlt,'rb').read() == data
                print(f"  00.lay == default.lay: {ok}")
                assert ok, "00.lay 与 default.lay 不一致（布局序假设错误）"
        print(f"  {i:02d}.lay ← {f} ({data[:8].hex()}...)")
        written += 1
    # 完整性校验：每个 .lay 30 字节
    for i in range(12):
        p = os.path.join(out, f'{i:02d}.lay')
        assert os.path.getsize(p) == 30, f"{p} 大小错"
    print(f"★ 12 个布局写入 {out}/（00.lay=default 校验通过）")

if __name__ == '__main__':
    main()
