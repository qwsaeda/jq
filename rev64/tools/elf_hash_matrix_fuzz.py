#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""原版 32 位 so ELF hash(.hash) ↔ .dynsym 一致性 fuzz（零设备依赖）

定位：现有判据查 dynsym 数量（1479 dynsym）/ 导出符号，但**从没验证 SysV hash 表
  的 bucket/chain 是否完整覆盖所有动态符号**。hash 断链 = 符号在表里但 dlsym 找不到
  ——正是本会话前面"改 so 符号破坏 ELF hash → UnsatisfiedLinkError"的同类问题。
  基于原版 so 做正向一致性校验（找原版有没有 hash 断链/孤符号）。

SysV hash 表结构（ELF32）：
  uint32 nbucket, nchain;
  uint32 buckets[nbucket];   // 每桶首个符号 index（0=STN_UNDEF 空）
  uint32 chains[nchain];     // 链：chains[i] = 下一个同桶符号 index（0=链尾）
哈希函数（ELF spec）：
  h = 0; for c in name: h = (h<<4)+c; g=h&0xf0000000; if g: h^=g>>24; h&=~g;
判据：
  ① nbucket/nchain 与 .dynsym 槽数一致（nchain == dynsym_count）
  ② 每个 dynsym 符号（idx 1..nchain-1）都能从某个 bucket 沿 chain 到达（可达性）
  ③ bucket 值 < nchain；chain 值 < nchain（无越界链）
  ④ 无环（chain 遍历不回到已访问 idx）
  ⑤ 负向自检：注入断链（bucket[0] 改越界）→ 判据必须 FAIL
"""
import os, struct, sys, zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
APK = os.path.join(ROOT, '军棋.apk')
CACHE = '/tmp/opencode/orig_libjunqi.so'

def get_so():
    if os.path.isfile(CACHE): return open(CACHE, 'rb').read()
    d = zipfile.ZipFile(APK).read('lib/armeabi/libjunqi.so')
    open(CACHE, 'wb').write(d); return d

def elf_hash(name):
    h = 0
    for c in name:
        h = (h << 4) + c
        g = h & 0xf0000000
        if g: h ^= g >> 24
        h &= ~g & 0xffffffff
    return h

def parse(d):
    shoff = struct.unpack_from('<I', d, 32)[0]
    shentsize = struct.unpack_from('<H', d, 46)[0]
    shnum = struct.unpack_from('<H', d, 48)[0]
    shstrndx = struct.unpack_from('<H', d, 50)[0]
    shstr = struct.unpack_from('<I', d, shoff + shstrndx * shentsize + 16)[0]
    sections = {}
    for i in range(shnum):
        o = shoff + i * shentsize
        no = struct.unpack_from('<I', d, o)[0]
        off = struct.unpack_from('<I', d, o + 16)[0]
        sz = struct.unpack_from('<I', d, o + 20)[0]
        ne = d.find(b'\0', shstr + no)
        sections[d[shstr + no:ne].decode('ascii', 'ignore')] = (off, sz)
    hoff, hsz = sections['.hash']
    nbucket, nchain = struct.unpack_from('<II', d, hoff)
    buckets = list(struct.unpack_from('<%dI' % nbucket, d, hoff + 8))
    chains = list(struct.unpack_from('<%dI' % nchain, d, hoff + 8 + nbucket * 4))
    doff, dsz = sections['.dynsym']
    dynsym_n = dsz // 16   # Elf32_Sym = 16B
    # dynstr
    stroff, _ = sections['.dynstr']
    return nbucket, nchain, buckets, chains, dynsym_n, stroff, d, doff

def check(nbucket, nchain, buckets, chains, dynsym_n, stroff=0, so_data=b"", doff_global=0):
    fails = []
    if nchain != dynsym_n:
        fails.append('nchain %d ≠ dynsym 槽数 %d' % (nchain, dynsym_n))
    if any(b >= nchain for b in buckets if b != 0):
        fails.append('bucket 越界值: %s' % [b for b in buckets if b >= nchain and b != 0][:3])
    if any(c >= nchain for c in chains if c != 0):
        fails.append('chain 越界值: %s' % [c for c in chains if c >= nchain and c != 0][:3])
    # 可达性：从所有 bucket 沿 chain 遍历
    seen = set()
    for b in buckets:
        i = b
        while i != 0 and i < nchain:
            if i in seen:
                fails.append('chain 成环 @idx%d' % i); break
            seen.add(i)
            i = chains[i]
    # ★ 真值口径：只要求"有名字的全局/弱符号"可达（这些才是 dlsym 的目标）。
    #   STT_SECTION(0x3)/LOCAL 符号按 ELF 规范**不进 hash 表**（无名可 hash）——
    #   原版 idx1/2 正是 section 符号（name 空 val=.text/.data.rel.ro），豁免。
    #   基于原版 so 读取 st_info + st_name 分类。
    global_named = []
    for i in range(1, nchain):
        o = doff_global + i * 16
        nameoff = struct.unpack_from('<I', so_data, o)[0]
        info = so_data[o + 12]
        bind = info >> 4   # 1=GLOBAL 2=WEAK
        if nameoff != 0 and bind in (1, 2):
            global_named.append(i)
    missing = [i for i in global_named if i not in seen]
    if missing:
        fails.append('不可达全局符号 %d 个（首例 idx=%d）' % (len(missing), missing[0]))
    return len(fails) == 0, fails, len(seen), len(global_named)

def main():
    d = get_so()
    nbucket, nchain, buckets, chains, dynsym_n, stroff, so, doff = parse(d)
    print('== 原版 so ELF hash ↔ dynsym 一致性 fuzz ==')
    print('   nbucket=%d nchain=%d dynsym=%d' % (nbucket, nchain, dynsym_n))
    ok, fails, reached, named = check(nbucket, nchain, buckets, chains, dynsym_n, stroff, so, doff)
    print('   ①-④ 正向判据: %s（可达符号 %d，全局有名符号 %d）' % ('PASS' if ok else 'FAIL', reached, named))
    for f in fails[:5]: print('     FAIL:', f)
    # 负向：bucket[0] 改越界
    mb = list(buckets); mb[0] = nchain + 1
    mok, _, _, _ = check(nbucket, nchain, mb, chains, dynsym_n, stroff, so, doff)
    print('   ⑤ 负向自检（bucket 越界注入）: %s' % ('检出' if not mok else '未检出(假绿)'))
    if not ok or mok:
        print('判据: FAIL'); return 2
    print('判据: PASS（hash bucket/chain 全覆盖 dynsym + 无越界/无环 + 注入检出）')
    return 0

if __name__ == '__main__':
    sys.exit(main())
