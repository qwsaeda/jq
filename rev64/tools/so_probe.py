#!/usr/bin/env python3
"""so_probe.py — 32 位 so 逆向探查工具（§124y-14 固化）

背景：本轮反复手敲"反汇编 vtable 槽 + 读 .so 静态数据 + 查 dispatch 表 + 定位函数"，
每次 5-10 条命令。本工具把高频探查动作固化成子命令，批量/可复用。

子命令：
  vtable <addr> [count]   读静态 vtable 槽值（.rodata），标注 F_xxx 函数边界
                          例: so_probe.py vtable 0x159c80 20
  func <addr>            反汇编某地址所在函数（自动往前找 push{ 入口 + 往后 32 指令）
                          例: so_probe.py func 0xad248
  slot <vt_addr> <off>   读 vtable 某槽值 + 反汇编目标函数首 4 指令
                          例: so_probe.py slot 0x159c80 0x28
  dispatch <addr>        查 0xADDR 是否在 jqt_vtbl_offs[] dispatch 表 + 对应 F_xxx 行号
                          例: so_probe.py dispatch 0xad248
  reloc <addr> [range]   读某 VA 的 RELATIVE 重定位目标（加 load_base 后的值）
                          例: so_probe.py reloc 0x1599e4
  strings <substr>       在 .rodata 明文里搜字符串 + 偏移

依赖：arm-linux-gnueabi-objdump / arm-linux-gnueabi-readelf（本机已装）。
默认 so 路径 /tmp/orig/lib/armeabi/libjunqi.so（原版 32 位 libjunqi.so，VA=文件偏移）。
"""
import struct, subprocess, sys, os, re, shutil

SO = os.environ.get('JQ_SO', '/tmp/orig/lib/armeabi/libjunqi.so')
OBJDUMP = 'arm-linux-gnueabi-objdump'
READELF = 'arm-linux-gnueabi-readelf'
JQT_VTBL = os.path.join(os.path.dirname(__file__), '..', 'src_jni_translated', 'jqt_vtbl.c')

def hex2i(s):
    return int(s, 16) if s.startswith('0x') else int(s)

def rd32(va):
    with open(SO, 'rb') as f:
        f.seek(va)
        return struct.unpack('<I', f.read(4))[0]

def rd_bytes(va, n):
    with open(SO, 'rb') as f:
        f.seek(va)
        return f.read(n)

def is_text_ptr(v):
    return 0x233e0 <= (v & ~1) <= 0x138aa0

def cmd_vtable(addr, count=12):
    vt = hex2i(addr)
    print(f"=== vtable 0x{vt:06x}（{count} 槽）===")
    for i in range(count):
        off = i * 4
        v = rd32(vt + off)
        tag = f"(F_{(v & ~1):05x})" if is_text_ptr(v) else ("NULL" if v == 0 else f"(0x{v:08x})")
        print(f"  [0x{off:02x}] = 0x{v:08x} {tag}")
    return 0

def cmd_func(addr, instrs=24):
    start = (hex2i(addr) & ~1) if isinstance(addr, str) else (addr & ~1)
    # 自动往前找函数入口（push { 模式 0xb5xx / 0xbdxx 等），最多回退 0x200
    entry = start
    data = rd_bytes(max(0, start - 0x200), 0x200 + 8)
    base = start - 0x200
    # 简化：从 start 直接反汇编（objdump 会按地址对齐）
    r = subprocess.run([OBJDUMP, '-D', f'--start-address=0x{start:x}',
                        f'--stop-address=0x{start + instrs*2:x}', SO],
                       capture_output=True, text=True)
    lines = [l for l in r.stdout.splitlines() if l.strip() and 'file format' not in l and 'Disassembly' not in l]
    print(f"=== 反汇编 0x{start:06x}（{instrs} 条指令）===")
    print('\n'.join(lines[:instrs + 4]))
    return 0

def cmd_slot(vt_addr, off):
    vt = hex2i(vt_addr); off_i = hex2i(off)
    v = rd32(vt + off_i)
    target = v & ~1
    print(f"=== vtable 0x{vt:06x} [0x{off_i:02x}] = 0x{v:08x} → F_{target:05x} ===")
    if is_text_ptr(v):
        cmd_func(target, 6)
    return 0

def load_dispatch_tbl():
    """读 jqt_vtbl.c 的 offs 表 → {addr: fns_idx}"""
    with open(JQT_VTBL) as f:
        s = f.read()
    m = re.search(r'const uint32_t jqt_vtbl_offs\[\] = \{(.*?)\};', s, re.S)
    offs = [int(x,16) for x in re.findall(r'0x([0-9a-f]+)', m.group(1))]
    m2 = re.search(r'uint32_t \(\*const jqt_vtbl_fns\[\]\)\s*\(.*?\)\s*=\s*\{(.*?)\};', s, re.S)
    fns = re.findall(r'F_[0-9a-f]+', m2.group(1))
    return offs, fns

def cmd_dispatch(addr):
    off = hex2i(addr)
    offs, fns = load_dispatch_tbl()
    in_tbl = off in offs
    print(f"=== 0x{off:05x} 在 jqt_vtbl_offs[] 表: {in_tbl} ===")
    if in_tbl:
        i = offs.index(off)
        print(f"  offs[{i}] = 0x{off:05x} ↔ fns[{i}] = {fns[i]}")
        # 找 F_xxx 的**函数体定义**（排除 extern 声明行）
        fn = fns[i]
        # 用 objdump 风格匹配：行以 "uint32_t F_xxx(uint32_t _r0" 开头（非 "extern "）
        with open(JQT_VTBL) as f:
            for li, ln in enumerate(f, 1):
                if ln.startswith(f'uint32_t {fn}(uint32_t _r0'):
                    print(f"  定义: {JQT_VTBL}:{li}")
                    break
            else:
                print(f"  ⚠ {fn} 无函数体定义（仅 extern/特判）→ 可能 link 悬空")
    else:
        # 找最近函数边界 + 提示是否在 jqt_blx_dispatch 特判里
        import bisect
        i = bisect.bisect_right(offs, off) - 1
        if i >= 0:
            print(f"  下方 0x{offs[i]:05x}（F_{offs[i]:05x}），上方 0x{offs[i+1]:05x}")
            print(f"  → 0x{off:05x} 在 0x{offs[i]:05x}-0x{offs[i+1]:05x} 函数体内（未单独收进表）")
        # 查是否在 jqt_blx_dispatch 的 if (off == 0x...) 特判里
        with open(JQT_VTBL) as f:
            src = f.read()
        spec_hits = re.findall(rf'if \(off == 0x{off:05x}\)', src)
        if spec_hits:
            print(f"  ★ 在 jqt_blx_dispatch 特判里（{len(spec_hits)} 处 if off==0x{off:05x}）→ 走特判直调，不在表也正常")
        else:
            print(f"  ⚠ 不在表也不在特判 → 若被 dispatch 会 miss（需 dispatch_insert.py 收表或加特判）")
    return 0

def cmd_reloc(va, rng=8):
    """查某 VA 的重定位（不只 R_ARM_RELATIVE——.bss/.data 槽也可能是 GLOB_DAT/PLT 等）。
    列出该 VA 的**所有**重定位记录（readelf -r），标注类型。"""
    va_i = hex2i(va)
    out = subprocess.run([READELF, '-r', SO], capture_output=True, text=True).stdout
    found = []
    for line in out.splitlines():
        # 重定位行格式: "  0001599e4 R_ARM_RELATIVE 0000000000000000" 或带 symbol
        m = re.match(r'\s*([0-9a-f]+)\s+(R_ARM_\w+)\s+(.*)', line)
        if m and int(m.group(1), 16) == va_i:
            found.append((m.group(2), m.group(3)))
    val = rd32(va_i)
    if found:
        print(f"=== 0x{va_i:06x} 重定位命中 {len(found)} 处（静态值=0x{val:08x}）===")
        for rtype, extra in found:
            # R_ARM_RELATIVE：运行时 = load_base + addend(=VA)
            # 其他：标注符号
            if rtype == 'R_ARM_RELATIVE':
                print(f"  {rtype}（运行时 = load_base + 0x{va_i:06x}）")
            else:
                print(f"  {rtype} {extra}")
    else:
        print(f"0x{va_i:06x} 无重定位记录（.bss 槽多为 BSS 零初始化，dlopen 不填，运行时值靠构造器写）")
    return 0

def cmd_strings(sub):
    """.rodata 明文搜字符串（限定 0x120000-0x138aa0 区标注 ✓，避免误报 .text/.data 巧合字节）"""
    data = rd_bytes(0, os.path.getsize(SO))
    sub_b = sub.encode()
    RD_LO, RD_HI = 0x120000, 0x138aa0
    idx = 0
    hits = []
    while True:
        i = data.find(sub_b, idx)
        if i < 0: break
        j = data.find(b'\0', i)
        hits.append((i, data[i:j].decode('utf-8','replace'), RD_LO <= i < RD_HI))
        idx = i + 1
    print(f"=== 字符串 '{sub}' 命中 {len(hits)} 处（.rodata 区 0x{RD_LO:x}-0x{RD_HI:x} 标 ✓）===")
    for off, s, in_rd in hits[:30]:
        mark = "✓" if in_rd else "·"
        print(f"  {mark} 0x{off:06x}: {s!r}")
    return 0

def main():
    if len(sys.argv) < 2:
        print(__doc__); return 0
    cmd = sys.argv[1]
    args = sys.argv[2:]
    if cmd == 'vtable':
        return cmd_vtable(args[0], int(args[1]) if len(args)>1 else 12)
    if cmd == 'func':
        return cmd_func(args[0], int(args[1]) if len(args)>1 else 24)
    if cmd == 'slot':
        return cmd_slot(args[0], args[1])
    if cmd == 'dispatch':
        return cmd_dispatch(args[0])
    if cmd == 'reloc':
        return cmd_reloc(args[0], int(args[1]) if len(args)>1 else 8)
    if cmd == 'strings':
        return cmd_strings(args[0])
    print(f"未知子命令: {cmd}\n{__doc__}"); return 2

if __name__ == '__main__':
    sys.exit(main())
