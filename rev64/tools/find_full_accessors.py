#!/usr/bin/env python3
"""find_full_accessors.py — 找完整惰性单例访问器（含 _Znwj 分配 + bl 构造 + str 写槽）

完整访问器模式（对照 0x23596-0x235c0 / F_9d3d4 实证）：
    push {...}
    ldr rX,[pc,#N]      # rX = 槽地址
    add rX,pc
    ldr rY,[rX]          # 读槽
    cmp rY,#0
    bne 返回              # 已构造
    movs r0,#SIZE / ldr r0,=SIZE
    blx _Znwj            # 分配
    bl F_ctor            # 构造
    str rY,[rX]          # 写槽
    ... 返回 rY

与 find_accessors.py 区别：要求函数体含 blx _Znwj（真分配器），且记录 ctor/size。

用法：
  python3 find_full_accessors.py <so> --out /tmp/full_accessors.json
"""
import struct, re, subprocess, json, argparse

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('so')
    ap.add_argument('--out', default='/tmp/full_accessors.json')
    args = ap.parse_args()
    d = open(args.so,'rb').read()
    SO_SIZE = len(d)
    out = subprocess.run(['arm-linux-gnueabi-objdump','-d',args.so],capture_output=True,text=True).stdout
    lines = out.splitlines()
    insns = []
    for l in lines:
        m = re.match(r'^\s*([0-9a-f]+):\s+([0-9a-f]{4})\s+(.*)', l)
        if m: insns.append((int(m.group(1),16), m.group(3).strip()))
    slots = [0x15be80+4*i for i in range(16)]
    accessors = {}
    for i,(a,body) in enumerate(insns):
        if not (0x233e0 <= a < 0x120bf4): continue
        if not body.startswith('push'): continue
        # 函数体找 blx _Znwj（候选访问器）
        func_insns = insns[i:min(i+120, len(insns))]
        if not any('Znwj' in b for _,b in func_insns):
            continue
        # 找 ldr rX,[pc]→add rX,pc→ldr rY,[rX]→cmp→beq→_Znwj→bl ctor→str 槽
        for j in range(i+1, min(i+40, len(insns))):
            aj,bj = insns[j]
            pj = bj.split(None,1)
            if len(pj)<2 or pj[0]!='ldr': continue
            m1 = re.match(r'(r\d+),\s*\[pc,\s*#(\d+)\]', pj[1])
            if not m1: continue
            base_r = m1.group(1)
            for k in range(j+1, min(j+8, len(insns))):
                ak,bk = insns[k]
                pk = bk.split(None,1)
                if len(pk)<2 or pk[0]!='add': continue
                if not re.match(r'%s,\s*pc' % base_r, pk[1]): continue
                for q in range(k+1, min(k+8, len(insns))):
                    aq,bq = insns[q]
                    pq = bq.split(None,1)
                    if len(pq)<2 or pq[0]!='ldr': continue
                    m2 = re.match(r'(r\d+),\s*\[%s(?:,\s*#0)?\]' % base_r, pq[1])
                    if not m2: continue
                    # 向后找 _Znwj + bl ctor + str 槽
                    size = None; ctor = None; saw_znwj = False
                    for w in range(q+1, min(q+35, len(insns))):
                        aw,bw = insns[w]
                        pw = bw.split(None,1)
                        if len(pw)<2: continue
                        if pw[0].startswith('mov') and re.match(r'movs?\s+r0,\s*#(\d+)', pw[1]):
                            ms = re.match(r'movs?\s+r0,\s*#(\d+)', pw[1])
                            size = int(ms.group(1))
                        if 'Znwj' in bw:
                            saw_znwj = True
                        if ('bl\t' in bw or 'bl.w' in bw) and saw_znwj and ctor is None:
                            m5 = re.search(r'bl\s+([0-9a-f]+)', bw)
                            if m5: ctor = int(m5.group(1),16)
                        if pw[0].startswith('str') and base_r in pw[1]:
                            break
                    if not saw_znwj: continue  # 非分配器（纯读）
                    pool = ((aj+4)&~3)+int(m1.group(2))
                    if pool+4 > SO_SIZE: break
                    C = struct.unpack_from('<I', d, pool)[0]
                    tgt = (C + ak) & 0xFFFFFFFF
                    if tgt in slots:
                        key = f'0x{tgt:08x}'
                        accessors[key] = {
                            'func': f'0x{a:08x}', 'base': base_r,
                            'size': size, 'ctor': f'0x{ctor:08x}' if ctor else None,
                        }
                        print(f'  槽 {key}: 访问器@0x{a:08x} ctor={accessors[key]["ctor"]} size={size}')
                    break
                break
            break
    json.dump(accessors, open(args.out,'w'), indent=1)
    print(f'\n落盘 {args.out} ({len(accessors)} 槽）')

if __name__ == '__main__':
    main()
