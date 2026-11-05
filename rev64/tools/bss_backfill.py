#!/usr/bin/env python3
"""bss_backfill.py — .bss 槽分类 + 选择性回填表生成（MuMu 原版 dump → 翻译体）

背景（§RPI-FIX 修正后）：
- 真 RPI 超边界 311 条 → 305 条 target ∈ .bss（57 个不同槽）+ 6 条超尾假阳性
- .bss 段 = 0x15be80-0x162e88（28680B），MuMu 原版 dump 在 bss_mumu_runtime.bin
- 之前整段回填崩 0x3a 根因 = scudo 堆指针（≥0xc0000000）跨进程失效
- 修正策略：**选择性回填**——标量/浮点/位标志/哨兵直接回填；scudo 指针槽置 0
  （待自建对象）；指向原版 .so 段的指针转偏移+加翻译体基址

用法：
  python3 bss_backfill.py <dump.bin> <rpi_invalid.json> [--bss-lo 0x15be80] \
      [--out-scalar /tmp/jq_bss_scalar.h] [--out-ptr /tmp/jq_bss_ptr.json]
  输出：
    --out-scalar  标量槽回填 C 头（直接 memcpy 进 jqt_mem.c）
    --out-ptr     指针槽清单（待自建对象，含 dump 值）
  分类：
    zero      值=0（翻译体镜像已一致）
    scalar    值<0x10000 或 0x3f800000（浮点 1.0）/位标志（可安全回填）
    sentinel  0xffffffff/0xfffffffe（纹理空哨兵，可回填）
    so_ptr    指向原版 .so 段（load_base 已知，转偏移回填）
    scudo     堆指针（≥0xc0000000，需自建，置 0）
    other     其他（默认置 0 保守）
"""
import struct, json, argparse, sys

def classify_slots(dump, rpi_invalid, bss_lo, bss_hi, load_base=None):
    """对 rpi_invalid 里所有 .bss target 槽分类，返回 {slot: (val, cls)}"""
    slots = {}
    for x in rpi_invalid:
        t = int(x['tgt'],16)
        if bss_lo <= t < bss_hi:
            slots[t] = None
    result = {}
    for t in sorted(slots):
        off = t - bss_lo
        if off+4 > len(dump):
            result[t] = (0, 'oob')
            continue
        v = struct.unpack_from('<I', dump, off)[0]
        if v == 0:
            cls = 'zero'
        elif v < 0x10000:
            cls = 'scalar'
        elif v == 0x3f800000:
            cls = 'scalar'  # 1.0f
        elif v in (0xffffffff, 0xfffffffe):
            cls = 'sentinel'
        elif load_base and load_base <= v < load_base + 0x162e88:
            cls = 'so_ptr'
        elif v >= 0xc0000000:
            cls = 'scudo'
        else:
            cls = 'other'
        result[t] = (v, cls)
    return result

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('dump')
    ap.add_argument('rpi_invalid')
    ap.add_argument('--bss-lo', default='0x15be80')
    ap.add_argument('--bss-hi', default='0x162e88')
    ap.add_argument('--load-base', default='0x0c024000', help='MuMu 原版 load_base（so_ptr 转偏移用）')
    ap.add_argument('--out-scalar', default='/tmp/jq_bss_scalar.h')
    ap.add_argument('--out-ptr', default='/tmp/jq_bss_ptr.json')
    args = ap.parse_args()
    dump = open(args.dump,'rb').read()
    rpi = json.load(open(args.rpi_invalid))
    bss_lo = int(args.bss_lo,16); bss_hi = int(args.bss_hi,16)
    lb = int(args.load_base,16) if args.load_base else None
    slots = classify_slots(dump, rpi, bss_lo, bss_hi, lb)
    from collections import Counter
    cnt = Counter(c for _,c in slots.values())
    print(f'.bss 槽总数: {len(slots)}')
    for k,n in cnt.most_common():
        print(f'  {k}: {n}')
    # 标量 + sentinel + so_ptr 可回填
    backfillable = [(t,v) for t,(v,c) in slots.items() if c in ('scalar','sentinel','so_ptr')]
    print(f'\n可回填槽（scalar/sentinel/so_ptr）: {len(backfillable)}')
    # 生成标量回填 C
    c = ['/* bss_backfill.py 生成：.bss 可回填标量槽（MuMu 原版 dump，非指针，安全） */',
         'static const struct { uint32_t off; uint32_t val; } jq_bss_scalar_tbl[] = {']
    for t,v in backfillable:
        c.append(f'    {{0x{t:08x}u, 0x{v:08x}u}},')
    c.append('};')
    c.append('#define JQ_BSS_SCALAR_N (sizeof(jq_bss_scalar_tbl)/sizeof(jq_bss_scalar_tbl[0]))')
    open(args.out_scalar,'w').write('\n'.join(c)+'\n')
    # 指针槽清单
    ptr_slots = {f'0x{t:08x}': {'val': v, 'cls': cls} for t,(v,cls) in slots.items() if cls in ('scudo','other')}
    json.dump(ptr_slots, open(args.out_ptr,'w'), indent=1)
    print(f'标量回填表 → {args.out_scalar}')
    print(f'指针槽清单 → {args.out_ptr}')
    # 明细
    print('\n=== 各槽分类明细 ===')
    for t,(v,cls) in slots.items():
        print(f'  0x{t:08x} = 0x{v:08x} [{cls}]')

if __name__ == '__main__':
    main()
