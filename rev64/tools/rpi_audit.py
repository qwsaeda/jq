#!/usr/bin/env python3
"""rpi_audit.py — RPI（加载PC相对）三连机械化审计工具（公式修正版）

RPI 三连：`ldr rX,[pc,#N]` → `add rX,pc` → 后续用 rX 作为基址。
池地址公式（ARM Thumb，★正确）：pool = ((addr+4) & ~3) + disp

历史教训（§RPI-FIX，本轮修正）：
- 旧分析用 `(addr&~2)+disp` → 池地址**差 4 字节**（Thumb PC 语义 = addr+4 再 4 对齐）
  → 4078 条"超边界 RPI"里 3767 条是**假阳性**（读错池值 → target 错乱）
- 正确公式 `((addr+4)&~3)+disp`（objdump 注解实证：0xbe9d8→0xbec60、0x23426→0x23434、
  0x45b6a→0x45b84 全对）→ 真超边界仅 **311 条**（305 .bss + 6 超尾假阳性）
- **翻译体生成代码本身用正确公式**（MEM(addr+4+disp)），无 RPI 翻译 bug

用法：
  python3 rpi_audit.py <so> [--out /tmp/rpi_invalid.json] [--dump]
  --dump   打印超边界明细

输出：
  /tmp/rpi_invalid.json  超边界 RPI 清单（ldr/add/pool/C/tgt）
  统计：真 RPI 总数 / target∈.so / 超边界 / ∈.bss / 超尾
"""
import struct, json, re, subprocess, sys, argparse

def load_insns(so_path):
    """objdump 反汇编 → (addr, mnemonic, op_str) 列表"""
    out = subprocess.run(['arm-linux-gnueabi-objdump','-d',so_path],
                         capture_output=True,text=True).stdout
    insns = []
    for l in out.splitlines():
        m = re.match(r'^\s*([0-9a-f]+):\s+([0-9a-f]{4})\s+(.*)', l)
        if m:
            insns.append((int(m.group(1),16), m.group(3).strip()))
    return insns

def audit(so_path):
    d = open(so_path,'rb').read()
    SO_SIZE = len(d)
    insns = load_insns(so_path)
    # 段范围（找 .text）——readelf -S 列：Idx Name Type Addr Off Size ...
    out = subprocess.run(['arm-linux-gnueabi-readelf','-S',so_path],
                         capture_output=True,text=True).stdout
    text_lo, text_hi = 0x233e0, 0x120bf4  # 默认（原版军棋）
    for l in out.splitlines():
        m = re.match(r'\s*\[\s*\d+\]\s+\.text\s+\S+\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)', l)
        if m:
            text_lo = int(m.group(1),16)
            text_hi = text_lo + int(m.group(2),16)
            break
    rpi_total = rpi_valid = 0
    rpi_invalid = []
    for i,(a,body) in enumerate(insns):
        if not (text_lo <= a < text_hi): continue
        parts = body.split(None,1)
        if len(parts)<2: continue
        mn = parts[0]
        if mn not in ('ldr','ldrh'): continue
        mm = re.match(r'(r\d+),\s*\[pc,\s*#(\d+)\]', parts[1])
        if not mm: continue
        rd = mm.group(1); disp = int(mm.group(2))
        pool = ((a+4) & ~3) + disp   # ★ 正确公式
        if pool+4 > SO_SIZE: continue
        C = struct.unpack_from('<I', d, pool)[0]
        addpc_addr = None
        for j in range(i+1, min(i+5, len(insns))):
            a2, body2 = insns[j]
            p2 = body2.split(None,1)
            if len(p2)>=2 and p2[0]=='add':
                m2 = re.match(r'%s,\s*pc' % rd, p2[1])
                if m2:
                    addpc_addr = a2; break
        if addpc_addr is None: continue
        rpi_total += 1
        target = (C + addpc_addr) & 0xFFFFFFFF
        if target < SO_SIZE and target+4 <= SO_SIZE:
            rpi_valid += 1
        else:
            rpi_invalid.append((a, addpc_addr, pool, C, addpc_addr, target))
    return d, insns, SO_SIZE, rpi_total, rpi_valid, rpi_invalid

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('so')
    ap.add_argument('--out', default='/tmp/rpi_invalid.json')
    ap.add_argument('--dump', action='store_true')
    args = ap.parse_args()
    d, insns, SO_SIZE, rpi_total, rpi_valid, rpi_invalid = audit(args.so)
    bss_lo, bss_hi = 0x15be80, 0x162e88
    n_bss = sum(1 for x in rpi_invalid if bss_lo <= x[5] < bss_hi)
    print(f'.so 大小 = 0x{SO_SIZE:x}')
    print(f'真 RPI 总数: {rpi_total}')
    print(f'  target ∈ .so 文件范围: {rpi_valid}')
    print(f'  target 超 .so 边界: {len(rpi_invalid)}')
    print(f'  超边界中 ∈ .bss: {n_bss}')
    print(f'  超边界中 > .bss 尾: {len(rpi_invalid)-n_bss}')
    if args.dump:
        for x in rpi_invalid[:40]:
            print(f'  ldr@0x{x[0]:08x} add@0x{x[1]:08x} pool=0x{x[2]:08x} C=0x{x[3]:08x} → target=0x{x[5]:08x}')
    json.dump([{'ldr':'0x%x'%x[0],'add':'0x%x'%x[1],'pool':'0x%x'%x[2],
                'C':'0x%x'%x[3],'pc':'0x%x'%x[4],'tgt':'0x%x'%x[5]}
               for x in rpi_invalid], open(args.out,'w'), indent=1)
    print(f'清单落盘 {args.out} ({len(rpi_invalid)} 条)')

if __name__ == '__main__':
    main()
