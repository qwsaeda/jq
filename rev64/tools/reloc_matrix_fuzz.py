#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""原版 32 位 so .rel.dyn 重定位表**逐项结构** fuzz（零设备依赖）

定位：现有 rel_dyn_check.py / readelf_rel_check.py 只校验聚合数（5312 RELATIVE +
  类型分布），查不出"某槽 r_offset 落在非法段 / 重复 / 顺序错乱但总数不变"的注入。
  本判据逐项解析 5325 槽，验证结构不变量 + 负向自检。

数据源：军棋.apk → lib/armeabi/libjunqi.so（原版 32 位 ARM，非壳）
判据：
  ① 槽数 5325（0xa668/8）+ 类型分布 {23:5312, 21:6, 2:7}
  ② 每个 RELATIVE 槽 r_offset 落在合法可写段内（.data/.bss 区，见 ELF LOAD）
  ③ r_offset 无重复（5325 槽应互异——重复 = 同地址双重重定位 = 逻辑错）
  ④ r_offset 严格递增（原版链接器按地址序输出重定位表）
  ⑤ 负向自检：注入一个越界 r_offset / 一个重复 → 判据必须 FAIL（非恒真）
"""
import os, re, struct, subprocess, sys, zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
APK = os.path.join(ROOT, '军棋.apk')
CACHE = '/tmp/opencode/orig_libjunqi.so'

def get_so():
    if os.path.isfile(CACHE):
        return open(CACHE, 'rb').read()
    z = zipfile.ZipFile(APK)
    d = z.read('lib/armeabi/libjunqi.so')
    open(CACHE, 'wb').write(d)
    return d

def parse_elf(data):
    """返回 (rel_offset, rel_size, load_ranges)"""
    assert data[:4] == b'\x7fELF' and data[4] == 1, '非 ELF32'
    # ELF32 header: e_phoff@28 (4B), e_phentsize@42, e_phnum@44
    phoff = struct.unpack_from('<I', data, 28)[0]
    phentsize = struct.unpack_from('<H', data, 42)[0]
    phnum = struct.unpack_from('<H', data, 44)[0]
    loads = []
    for i in range(phnum):
        o = phoff + i * phentsize
        p_type = struct.unpack_from('<I', data, o)[0]
        if p_type != 1:  # PT_LOAD
            continue
        p_offset, p_vaddr, _, p_filesz, p_memsz, p_flags = struct.unpack_from('<IIIIII', data, o + 4)
        loads.append((p_vaddr, p_vaddr + p_memsz, p_flags))
    # .rel.dyn section: 从 section header 找 SHT_REL(9)
    shoff = struct.unpack_from('<I', data, 32)[0]
    shentsize = struct.unpack_from('<H', data, 46)[0]
    shnum = struct.unpack_from('<H', data, 48)[0]
    shstrndx = struct.unpack_from('<H', data, 50)[0]
    shstr_off = struct.unpack_from('<I', data, shoff + shstrndx * shentsize + 16)[0]
    rel_off = rel_size = None
    for i in range(shnum):
        o = shoff + i * shentsize
        sh_type = struct.unpack_from('<I', data, o + 4)[0]
        name_off = struct.unpack_from('<I', data, o)[0]
        name_end = data.find(b'\x00', shstr_off + name_off)
        name = data[shstr_off + name_off:name_end].decode('ascii', 'ignore')
        if sh_type == 9 and name == '.rel.dyn':  # SHT_REL
            rel_off = struct.unpack_from('<I', data, o + 16)[0]
            rel_size = struct.unpack_from('<I', data, o + 20)[0]
    return rel_off, rel_size, loads

def parse_relocs(data, off, size):
    """每个 REL 槽 8B: r_offset(4) + r_info(4)；type = r_info & 0xff"""
    out = []
    for i in range(size // 8):
        r_off, r_info = struct.unpack_from('<II', data, off + i * 8)
        out.append((r_off, r_info & 0xff, r_info >> 8))
    return out

def check(relocs, loads):
    fails = []
    # ① 类型分布
    if len(relocs) != 5325:
        fails.append('槽数 %d ≠ 5325' % len(relocs))
    kinds = {}
    for _, t, _ in relocs:
        kinds[t] = kinds.get(t, 0) + 1
    expected_kinds = {23: 5312, 21: 6, 2: 7}
    if kinds != expected_kinds:
        fails.append('类型分布 %s ≠ %s' % (kinds, expected_kinds))
    # ② r_offset 落在合法可写段（PT_LOAD 且 flags&2）
    writable = [(a, b) for a, b, f in loads if f & 2]
    for idx, (r_off, t, sym) in enumerate(relocs):
        if not any(a <= r_off < b for a, b in writable):
            fails.append('槽%d r_offset=0x%x 不在可写 LOAD 段' % (idx, r_off))
            break
    # ③ 无重复
    offs = [x[0] for x in relocs]
    dup = len(offs) - len(set(offs))
    if dup:
        fails.append('重复 r_offset %d 个' % dup)
    # ④ 分段递增（同类型内 r_offset 递增；类型切换处允许回退——链接器分段输出）
    #    实测：5312 RELATIVE 递增至 0x15be2c → 13 个 type2/21 回到 0x15a0b0 段，
    #    段内仍递增。全局严格递增假设错误，按类型分段校验。
    inv = 0
    for i in range(1, len(relocs)):
        if relocs[i][1] == relocs[i-1][1] and relocs[i][0] <= relocs[i-1][0]:
            inv += 1
    if inv:
        fails.append('同类型内非递增 r_offset %d 处' % inv)
    # ④b 类型分段数（应恰好 3 段：RELATIVE / type2 / type21 各一段）
    seg = 1
    for i in range(1, len(relocs)):
        if relocs[i][1] != relocs[i-1][1]:
            seg += 1
    if seg != 3:
        fails.append('类型分段数 %d ≠ 3（链接器分段布局异常）' % seg)
    return len(fails) == 0, fails

def main():
    data = get_so()
    print('== 原版 32 位 so .rel.dyn 逐项结构 fuzz ==')
    rel_off, rel_size, loads = parse_elf(data)
    print('   .rel.dyn @0x%x size=0x%x（%d 槽）' % (rel_off, rel_size, rel_size // 8))
    print('   PT_LOAD: %s' % ', '.join('0x%x-0x%x(f=%d)' % l for l in loads))
    relocs = parse_relocs(data, rel_off, rel_size)
    ok, fails = check(relocs, loads)
    print('   ①-④ 正向判据: %s' % ('PASS' if ok else 'FAIL'))
    for f in fails[:5]: print('     FAIL:', f)

    # ⑤ 负向自检：越界 r_offset + 重复 r_offset
    mutant = list(relocs)
    mutant[0] = (0xdeadbeef, mutant[0][1], mutant[0][2])  # 越界
    mok1, _ = check(mutant, loads)
    mutant2 = list(relocs)
    mutant2[1] = (mutant2[0][0], mutant2[1][1], mutant2[1][2])  # 重复 slot0
    mok2, _ = check(mutant2, loads)
    print('   ⑤ 负向自检: 越界注入=%s / 重复注入=%s' %
          ('检出' if not mok1 else '未检出(假绿)', '检出' if not mok2 else '未检出(假绿)'))

    if not ok or mok1 or mok2:
        print('判据: FAIL'); return 2
    print('判据: PASS（5325 槽逐项 r_offset 合法/互异/递增 + 类型分布 + 2 类注入可检出）')
    return 0

if __name__ == '__main__':
    sys.exit(main())
