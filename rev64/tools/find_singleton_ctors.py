#!/usr/bin/env python3
"""find_singleton_ctors.py — 机械化找 0x15be80 全局单例表的构造器

模式（对照 0x23596-0x235c0 实证）：
    ldr rX,[pc,#N]          # rX = 槽地址
    bl 访问器               # F_访问器（惰性单例，幂等）
    add rX,pc
    ldr rY,[rX]             # 读槽
    cmp rY,#0
    beq 构造路径
    构造路径:
    movs r0,#SIZE           # 或 ldr r0,=SIZE
    blx _Znwj               # 分配
    bl F_构造               # 构造对象
    str r6,[rX]             # 写槽 = 单例

用法：
  python3 find_singleton_ctors.py <so> --slots 0x15be80,0x15be84,... > /tmp/singleton_ctors.json

输出：
  {槽: {'ctor': F_构造, 'size': SIZE, 'accessor': F_访问器, 'instr': [...]}}
"""
import struct, re, subprocess, json, argparse, sys

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('so')
    ap.add_argument('--slots', default='0x15be80,0x15be84,0x15be88,0x15be8c,0x15be90,0x15be94,0x15be98,0x15be9c,0x15bea0,0x15bea4,0x15bea8,0x15beac,0x15beb0,0x15beb4,0x15beb8,0x15bebc')
    ap.add_argument('--out', default='/tmp/singleton_ctors.json')
    args = ap.parse_args()
    slots = [int(s,16) for s in args.slots.split(',')]
    d = open(args.so,'rb').read()
    SO_SIZE = len(d)
    out = subprocess.run(['arm-linux-gnueabi-objdump','-d',args.so],capture_output=True,text=True).stdout
    lines = out.splitlines()
    insns = []
    for l in lines:
        m = re.match(r'^\s*([0-9a-f]+):\s+([0-9a-f]{4})\s+(.*)', l)
        if m:
            insns.append((int(m.group(1),16), m.group(3).strip()))
    # 找 str rX,[槽]：str 目标 = RPI 槽地址
    ctors = {}
    for i,(a,body) in enumerate(insns):
        if not (0x233e0 <= a < 0x120bf4): continue
        parts = body.split(None,1)
        if len(parts)<2 or not parts[0].startswith('str'): continue
        mm = re.match(r'(r\d+),\s*\[(r\d+),\s*#?(\d+)?\]', parts[1])
        if not mm: continue
        # 回溯找 add rY,pc（RPI 基址）
        base_r = mm.group(2)
        disp = int(mm.group(3)) if mm.group(3) else 0
        for j in range(max(0,i-5), i):
            a2,b2 = insns[j]
            p2 = b2.split(None,1)
            if len(p2)>=2 and p2[0]=='add':
                m3 = re.match(r'%s,\s*pc' % base_r, p2[1])
                if m3:
                    # 回溯 ldr rY,[pc] 得池值
                    for k in range(max(0,j-5), j):
                        a3,b3 = insns[k]
                        p3 = b3.split(None,1)
                        if len(p3)>=2 and p3[0]=='ldr':
                            m4 = re.match(r'%s,\s*\[pc,\s*#(\d+)\]' % base_r, p3[1])
                            if m4:
                                pool = ((a3+4)&~3)+int(m4.group(1))
                                if pool+4 > SO_SIZE: break
                                C = struct.unpack_from('<I', d, pool)[0]
                                tgt = (C + a2) & 0xFFFFFFFF
                                if tgt in slots:
                                    # 找 _Znwj + 构造 bl + 槽
                                    # 向后 30 条找 blx _Znwj 和 bl F_xxx 和 str 槽
                                    znwj_size = None; ctor = None; accessor = None
                                    for m2 in range(j, min(i+40, len(insns))):
                                        am,bm = insns[m2]
                                        pm = bm.split(None,1)
                                        if len(pm)<2: continue
                                        if 'blx' in pm[0] and 'Znwj' in bm:
                                            # 找前面的 movs r0,#SIZE 或 ldr r0,=SIZE
                                            for q in range(max(j,m2-4), m2):
                                                aq,bq = insns[q]
                                                mq = re.match(r'movs?\s+r0,\s*#(\d+)', bq)
                                                if mq:
                                                    znwj_size = int(mq.group(1)); break
                                                mq2 = re.match(r'ldr\s+r0,\s*\[pc', bq)
                                                if mq2:
                                                    znwj_size = -1  # ldr 装载，大小未知
                                        if pm[0] in ('bl','bl.w') and not bm.startswith('blx'):
                                            m5 = re.search(r'bl\s+([0-9a-f]+)', bm)
                                            if m5:
                                                ctor = int(m5.group(1),16)
                                        if bm.startswith('str') and re.match(r'%s' % mm.group(1), bm):
                                            break  # 已写槽
                                    # 访问器 = 槽读前最近的 bl（0x23598 模式）
                                    for m2 in range(max(0,j-8), j):
                                        am,bm = insns[m2]
                                        m5 = re.search(r'bl\s+([0-9a-f]+)', bm)
                                        if m5:
                                            accessor = int(m5.group(1),16); break
                                    key = f'0x{tgt:08x}'
                                    ctors[key] = {
                                        'ctor': f'0x{ctor:08x}' if ctor else None,
                                        'size': znwj_size,
                                        'accessor': f'0x{accessor:08x}' if accessor else None,
                                        'str_at': f'0x{a:08x}',
                                        'pattern': 'ldr+add+str',
                                    }
                                    print(f'  槽 {key}: ctor={ctors[key]["ctor"]} size={znwj_size} accessor={ctors[key]["accessor"]}')
                                break
                            break
                    break
            break
    json.dump(ctors, open(args.out,'w'), indent=1)
    print(f'\n落盘 {args.out} ({len(ctors)} 槽）')

if __name__ == '__main__':
    main()
