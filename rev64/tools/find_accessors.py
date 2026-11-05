#!/usr/bin/env python3
"""find_accessors.py — 机械化找 0x15be80 表的惰性单例访问器（getter）

访问器模式（对照 F_9d3d4 / 0x23596-0x235c0 实证）：
    ldr rX,[pc,#N]      # rX = 槽地址
    add rX,pc
    ldr rY,[rX]          # 读槽
    cmp rY,#0
    bne 返回路径          # 已构造 → 返回
    movs r0,#SIZE / ldr r0,=SIZE
    blx _Znwj            # 分配
    bl F_ctor            # 构造
    str rY,[rX]          # 写槽
    ... 返回 rY

用法：
  python3 find_accessors.py <so> > /tmp/accessors.json
"""
import struct, re, subprocess, json, argparse

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('so')
    ap.add_argument('--out', default='/tmp/accessors.json')
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
    # 找访问器：ldr rX,[pc]→add rX,pc→ldr rY,[rX]→cmp rY,#0→bne/beq
    # 对照 F_9d3d4 实证：push {r4,r5,r6,lr}; ldr r4,[pc,#28]; add r4,pc; ldr r5,[r4];
    #   cmp r5,#0; beq 构造; movs r0,#136; blx _Znwj; bl 9d988; str r5,[r4]
    accessors = {}
    for i,(a,body) in enumerate(insns):
        if not (0x233e0 <= a < 0x120bf4): continue
        if not body.startswith('push'): continue
        # 函数起始，向后 20 条找模式
        for j in range(i+1, min(i+25, len(insns))):
            aj,bj = insns[j]
            pj = bj.split(None,1)
            if len(pj)<2: continue
            if pj[0]=='ldr':
                m1 = re.match(r'(r\d+),\s*\[pc,\s*#(\d+)\]', pj[1])
                if not m1: continue
                base_r = m1.group(1)
                # 向后找 add base_r,pc
                for k in range(j+1, min(j+8, len(insns))):
                    ak,bk = insns[k]
                    pk = bk.split(None,1)
                    if len(pk)<2 or pk[0]!='add': continue
                    if not re.match(r'%s,\s*pc' % base_r, pk[1]): continue
                    # 向后找 ldr rY,[base_r] 或 [base_r,#0] + cmp
                    for q in range(k+1, min(k+8, len(insns))):
                        aq,bq = insns[q]
                        pq = bq.split(None,1)
                        if len(pq)<2 or pq[0]!='ldr': continue
                        m2 = re.match(r'(r\d+),\s*\[%s(?:,\s*#0)?\]' % base_r, pq[1])
                        if m2:
                            # 确认后 1-3 条有 cmp rY,#0
                            has_cmp = False
                            for w in range(q+1, min(q+4, len(insns))):
                                cw = insns[w][1].split(None,1)
                                if len(cw)>=2 and cw[0].startswith('cmp') and m2.group(1) in cw[1]:
                                    has_cmp = True; break
                            if not has_cmp: continue
                            pool = ((aj+4)&~3)+int(m1.group(2))
                            if pool+4 > SO_SIZE: break
                            C = struct.unpack_from('<I', d, pool)[0]
                            tgt = (C + ak) & 0xFFFFFFFF
                            if tgt in slots:
                                size = None; ctor = None; saw_znwj = False
                                for w in range(q+1, min(q+30, len(insns))):
                                    aw,bw = insns[w]
                                    pw = bw.split(None,1)
                                    if len(pw)<2: continue
                                    if pw[0].startswith('mov') and re.match(r'movs?\s+r0,\s*#(\d+)', pw[1]):
                                        ms = re.match(r'movs?\s+r0,\s*#(\d+)', pw[1])
                                        size = int(ms.group(1))
                                    if pw[0]=='blx' and 'Znwj' in pw[1]:
                                        saw_znwj = True
                                    if pw[0] in ('bl','bl.w') and saw_znwj and ctor is None:
                                        m5 = re.search(r'bl\s+([0-9a-f]+)', pw[1])
                                        if m5: ctor = int(m5.group(1),16)
                                    if pw[0].startswith('str') and base_r in pw[1]:
                                        break
                                key = f'0x{tgt:08x}'
                                accessors[key] = {
                                    'func': f'0x{a:08x}', 'base': base_r,
                                    'size': size, 'ctor': f'0x{ctor:08x}' if ctor else None,
                                }
                                break
                    break
            break
    # 输出
    for s in sorted(accessors):
        info = accessors[s]
        print(f'  槽 {s}: 访问器@0x{info["func"][2:]} ctor={info["ctor"]} size={info["size"]}')
    json.dump(accessors, open(args.out,'w'), indent=1)
    print(f'\n落盘 {args.out} ({len(accessors)} 槽）')

if __name__ == '__main__':
    main()
