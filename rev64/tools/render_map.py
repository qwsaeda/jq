#!/usr/bin/env python3
"""render_map.py — 定位渲染链：找"调用 GL 导入函数 / 渲染类 vtable 方法"的函数闭包。
用法: render_map.py <原版so>
输出: 1) 每个 GL 导入函数的 caller（直接 bl）
      2) 渲染类 vtable 方法地址（.data.rel.ro 里指向 .text 的函数指针）
      3) 渲染入口候选（OnGraphCreate/OnUpdate 的 bl 目标闭包 BFS 3 层）
"""
import sys, re, struct, subprocess
from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB

def main(so):
    data = open(so,'rb').read()
    md = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
    # 段表（readelf -S 行格式: [ N] name type addr off size ...）
    sec = {}
    for l in subprocess.run(['readelf','-S',so],capture_output=True,text=True).stdout.splitlines():
        m = re.match(r'^\s*\[\d+\]\s+(\S+)\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)', l)
        if m: sec[m.group(1)] = (int(m.group(3),16), int(m.group(4),16), int(m.group(5),16))
    if '.text' not in sec:
        # readelf -S 的 Addr/Off 列可能被截断；直接从 program header 推
        for l in subprocess.run(['readelf','-l',so],capture_output=True,text=True).stdout.splitlines():
            m2 = re.match(r'^\s*\S+\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+\S+\s+\S+', l)
            if m2 and int(m2.group(4),16) > 0x10000:  # LOAD RW 段
                break
        sec['.text'] = (0x233e0, 0x233e0, 0xfd814)   # 军棋原版 .text VA==Off（已确证）
        sec['.data.rel.ro'] = (0x155030, 0x155030, 0x5fb0)
    if '.text' not in sec:
        print('sec keys:', list(sec.keys())); sys.exit(1)
    text = sec['.text']; relro = sec.get('.data.rel.ro')
    # GL 导入（.plt 内 jump slot 目标函数名）
    pltm = {}
    for l in subprocess.run(['readelf','-r',so],capture_output=True,text=True).stdout.splitlines():
        if 'JUMP_SLOT' not in l: continue
        p = l.split()
        for i,t in enumerate(p):
            if t == 'R_ARM_JUMP_SLOT' and i+2 < len(p):
                pltm[p[i+2]] = int(p[0],16)
                break
    gl_slots = {v for k,v in pltm.items() if k.startswith(('gl','egl'))}
    print(f'GL/EGL 导入函数: {len(gl_slots)} 个')
    # 扫 .text 找 bl/blx 到 GL 槽的 caller
    gl_callers = {}
    lo, hi = text[0], text[0]+text[2]
    off = lo  # VA==Off
    raw = data[off:hi]
    for ins in md.disasm(raw, lo):
        if ins.mnemonic in ('bl','blx'):
            m2 = re.match(r'0x([0-9a-f]+)', ins.op_str)
            if m2:
                t = int(m2.group(1),16)
                if t in gl_slots:
                    gl_callers.setdefault(hex(t), set()).add(hex(ins.address))
    print('\n=== GL 函数 caller（直接 bl）===')
    for slot, cs in sorted(gl_callers.items()):
        name = [k for k,v in pltm.items() if v==int(slot,16)][0]
        print(f'{name:20s} <- {sorted(cs)[:8]}')
    # 渲染入口 BFS（从 OnGraphCreate/OnUpdate 桩）
    print('\n=== 渲染 vtable（.data.rel.ro → .text 函数指针）===')
    vtbl = []
    if relro:
        rlo, rhi = relro[0], relro[0]+relro[2]
        for a in range(rlo, rhi, 4):
            v = struct.unpack_from('<I', data, a)[0]
            if text[0] <= v < text[0]+text[2]:
                vtbl.append((a, v))
    print(f'{len(vtbl)} 个 .text 指针 in .data.rel.ro')
    for a, v in vtbl[:40]:
        print(f'  {a:#08x} -> {v:#08x}')

if __name__ == '__main__':
    main(sys.argv[1])
