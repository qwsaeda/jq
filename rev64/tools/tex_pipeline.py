#!/usr/bin/env python3
# tex_pipeline.py — 军棋逆向纹理管线工具（§124zz 固化复用）
#
# 用途：
#   1. 反汇编原版 .so 搜 vtable dispatch（skipdata 处理 capstone 提前停止）
#   2. 找 vtable[N]（纹理更新/删除）dispatch 点
#   3. 检查翻译体 vs 原版 UND 符号差异（dlopen failed to link 排查）
#
# 用法：
#   python3 rev64/tools/tex_pipeline.py vtable22        # 搜 vtable[22]（0x58）dispatch 点
#   python3 rev64/tools/tex_pipeline.py vtable23        # 搜 vtable[23]（0x5c）dispatch 点
#   python3 rev64/tools/tex_pipeline.py undiff          # 翻译体 vs 原版 UND 符号差异
#   python3 rev64/tools/tex_pipeline.py funcs 0x9e0f0 0x9e204 0x9e2b4  # 反汇编函数+常量池
#
# 关键修复（§124zz）：
#   - capstone.disasm 在无效 Thumb 字节处提前停止（0x233e0 起只反汇编 343 条到 0x236de）
#     → 必须 md.skipdata=True 才能覆盖全 .text（485810 条到 0x120bf0）
#   - 此前搜 vtable[22] 漏报（0 个）→ skipdata 后找到 31 个 dispatch 点
#
# 依赖：capstone
import capstone, struct, subprocess, sys, re, json, os, bisect

ORIG = '/tmp/opencode/orig_libjunqi.so'
TEXT_OFF, TEXT_SZ = 0x233e0, 0x0fd814  # .text 起点/大小（原版 so）

def disasm_all(orig=ORIG):
    """反汇编全 .text（skipdata 处理提前停止）。"""
    data = open(orig, 'rb').read()
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True
    md.skipdata = True
    return data, list(md.disasm(data[TEXT_OFF:TEXT_OFF+TEXT_SZ], TEXT_OFF))

def find_vtable_dispatch(slot, disp=None, nxt=8):
    """找 ldr rX,[rY,#disp] + blx rX（vtable[slot] dispatch）。
    disp = slot*4；返回 [(ldr_addr, blx_addr, op_str), ...]"""
    disp = disp if disp is not None else slot*4
    data, insns = disasm_all()
    hits = []
    for i, ins in enumerate(insns):
        if ins.mnemonic == 'ldr' and len(ins.operands) == 2 and \
           ins.operands[1].type == capstone.arm.ARM_OP_MEM:
            mem = ins.operands[1].mem
            if mem.disp == disp and mem.base != capstone.arm.ARM_REG_PC:
                dst = ins.operands[0].reg
                for j in range(i+1, min(i+nxt, len(insns))):
                    b = insns[j]
                    if b.mnemonic == 'blx' and len(b.operands) == 1 and \
                       b.operands[0].type == capstone.arm.ARM_OP_REG and b.operands[0].reg == dst:
                        hits.append((ins.address, b.address, ins.op_str))
                        break
                    if b.mnemonic in ('bx', 'bl'):
                        break
    return hits

def show_func_consts(start, end):
    """反汇编函数 + 解析 ldr [pc,#imm] 常量池值。"""
    data = open(ORIG, 'rb').read()
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True
    print('=== 函数 0x%x-0x%x ===' % (start, end))
    for ins in md.disasm(data[start:end], start):
        line = '  0x%x: %s %s' % (ins.address, ins.mnemonic, ins.op_str)
        if ins.mnemonic == 'ldr' and len(ins.operands) == 2 and \
           ins.operands[1].type == capstone.arm.ARM_OP_MEM:
            mem = ins.operands[1].mem
            if mem.base == capstone.arm.ARM_REG_PC:
                t = (ins.address + 4) & ~3
                t += mem.disp
                if t + 4 <= len(data):
                    v = struct.unpack_from('<I', data, t)[0]
                    line += '  → 读 0x%x = 0x%x' % (t, v)
        print(line)

def _plt_map(orig=ORIG):
    """PLT 桩地址 → 符号名。解析 .rel.plt/.rel.dyn 的 JUMP_SLOT + PLT 桩反查 GOT 槽。"""
    data = open(orig, 'rb').read()
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True

    # 1) 可执行节（.plt / .text）——PLT 桩所在
    #    readelf -SW 行格式： [ 6] .plt  PROGBITS  <addr> <off> <size> <es> <flags> ...
    exec_ranges = []
    sec_re = re.compile(r'\[\s*\d+\]\s+(\S+)\s+\S+\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+\S*\s*([A-Z]+)')
    for line in subprocess.run(['arm-linux-gnueabi-readelf', '-SW', orig],
                               capture_output=True, text=True).stdout.splitlines():
        m = sec_re.search(line)
        if m and 'X' in m.group(5):
            exec_ranges.append((int(m.group(2), 16), int(m.group(3), 16), int(m.group(4), 16)))

    # 2) GOT 偏移 → 符号（.rel.dyn + .rel.plt 的 JUMP_SLOT）
    got_sym = {}
    for relsec in ('-rW',):
        for line in subprocess.run(['arm-linux-gnueabi-readelf', relsec, orig],
                                   capture_output=True, text=True).stdout.splitlines():
            p = line.split()
            # 格式: <offset> <info> R_ARM_JUMP_SLOT <symvalue> <symbol>
            if len(p) >= 5 and 'JUMP_SLOT' in line:
                try: got_sym[int(p[0], 16)] = p[4]
                except ValueError: pass

    # 3) PLT 桩 → 符号
    #    ★ ARM .plt 布局（实测）：PLT0 = 16 字节（0x229c0..0x229d0），
    #      条目 stride = 12，起始 0x229d4；.rel.plt 条目按**顺序**一一对应。
    #      条目形态：add ip,pc,#0x100000 / add ip,ip,#56,#20 / ldr pc,[ip,#off]!
    #      GOT 地址 = (桩+8+0x100000) + ror32(56,20) + off
    plt = {}
    plt_secs = [(a, o, s) for (a, o, s) in exec_ranges if s > 0]
    relplt = []
    for line in subprocess.run(['arm-linux-gnueabi-readelf', '-rW', orig],
                               capture_output=True, text=True).stdout.splitlines():
        p = line.split()
        if len(p) >= 5 and 'JUMP_SLOT' in line:
            relplt.append((int(p[0], 16), p[4]))
    # .rel.plt 与 .rel.dyn 的 JUMP_SLOT 合流（.got 槽 → 符号）
    got_all = dict(got_sym)
    for off, sym in relplt:
        got_all.setdefault(off, sym)

    def ror32(v, n):
        n &= 31
        return ((v >> n) | (v << (32 - n))) & 0xffffffff if n else v & 0xffffffff

    md_arm = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_ARM)
    md_arm.detail = True
    for addr, off, size in plt_secs:
        # ARM .plt：PLT0 = 20 字节（5×4，含 4 字节保留槽）+ 条目 stride 12
        if size < 40:
            continue
        base = addr + 20
        n = (size - 20) // 12
        for i in range(n):
            a = base + i * 12
            ins = list(md_arm.disasm(data[off + (a - addr):off + (a - addr) + 12], a))
            if len(ins) != 3:
                continue
            i0, i1, i2 = ins
            if not (i0.mnemonic == 'add' and i0.op_str.startswith('ip, pc')):
                continue
            if not (i2.mnemonic == 'ldr' and i2.op_str.startswith('pc, [ip')):
                continue
            def imm_of(i):
                for o in i.operands:
                    if o.type == capstone.arm.ARM_OP_IMM:
                        return o.imm
                return 0
            v = a + 8 + imm_of(i0)
            # add ip, ip, #imm8, #rot → capstone 打印 rotate 角度（非字段），需 ror
            m = re.search(r'#(\d+),\s*#(\d+)$', i1.op_str)
            if m:
                v += ror32(int(m.group(1)), int(m.group(2)))
            else:
                v += imm_of(i1)
            for o in i2.operands:
                if o.type == capstone.arm.ARM_OP_MEM:
                    v += o.mem.disp
            if v in got_all:
                plt[a] = got_all[v]
    return plt, got_all


GL_ENUMS = {0xde1: 'GL_TEXTURE_2D',
            0x2800: 'GL_TEXTURE_MAG_FILTER', 0x2801: 'GL_TEXTURE_MIN_FILTER',
            0x2802: 'GL_TEXTURE_WRAP_S', 0x2803: 'GL_TEXTURE_WRAP_T',
            0xcf5: 'GL_UNPACK_ALIGNMENT', 0x1908: 'GL_RGBA',
            0x1401: 'GL_UNSIGNED_BYTE', 0x2600: 'GL_NEAREST',
            0x2601: 'GL_LINEAR', 0x812f: 'GL_CLAMP_TO_EDGE', 0x2901: 'GL_REPEAT'}


def _fmt_val(v):
    """把寄存器值格式化成 '0x2802 GL_TEXTURE_WRAP_S' / '9729.0f(GL_LINEAR)' 形式。"""
    if v is None:
        return '?'
    u = v & 0xffffffff
    if u in GL_ENUMS:
        return '0x%x %s' % (u, GL_ENUMS[u])
    f = struct.unpack('<f', struct.pack('<I', u))[0]
    if abs(f) < 1e5 and f == int(f) and u not in (0,):
        extra = ''
        if int(f) == 0x2601: extra = ' (GL_LINEAR)'
        elif int(f) == 0x812f: extra = ' (GL_CLAMP_TO_EDGE)'
        elif int(f) == 0x2600: extra = ' (GL_NEAREST)'
        return '%.1ff%s' % (f, extra)
    return '0x%x' % u


def gl_trace(start, end, orig=ORIG):
    """追踪函数里的 GL 调用序列：blx/bl → PLT 符号 + **实参值**（轻量寄存器跟踪）。"""
    data = open(orig, 'rb').read()
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True
    plt, _ = _plt_map(orig)
    regs = {}          # 寄存器号 → 值
    stack = {}         # sp+偏移 → 值（仅跟踪 str 到栈的简单情形）
    print('=== GL 调用追踪 0x%x-0x%x ===' % (start, end))
    for ins in md.disasm(data[start:end], start):
        line = '  0x%x: %s %s' % (ins.address, ins.mnemonic, ins.op_str)
        ops = ins.operands
        rn = lambda o: o.reg if o.type == capstone.arm.ARM_OP_REG else None

        # --- 记录寄存器赋值 ---
        if ins.mnemonic == 'mov' and len(ops) >= 2 and rn(ops[0]) is not None:
            if ops[1].type == capstone.arm.ARM_OP_IMM:
                regs[rn(ops[0])] = ops[1].imm
            elif rn(ops[1]) is not None:
                regs[rn(ops[0])] = regs.get(rn(ops[1]))
        elif ins.mnemonic in ('movs', 'movw') and len(ops) >= 2 and rn(ops[0]) is not None:
            if ops[1].type == capstone.arm.ARM_OP_IMM:
                regs[rn(ops[0])] = ops[1].imm
        elif ins.mnemonic == 'ldr' and len(ops) == 2 and rn(ops[0]) is not None:
            mem = ops[1].mem
            if mem.base == capstone.arm.ARM_REG_PC:
                t = ((ins.address + 4) & ~3) + mem.disp
                if t + 4 <= len(data):
                    regs[rn(ops[0])] = struct.unpack_from('<I', data, t)[0]
            elif mem.base == capstone.arm.ARM_REG_SP and mem.disp in stack:
                regs[rn(ops[0])] = stack[mem.disp]
            elif mem.disp in (0x4c, 0x1c, 0x20, 0x24) and mem.base != capstone.arm.ARM_REG_SP:
                regs[rn(ops[0])] = None      # this[off] 运行时值
        elif ins.mnemonic in ('adds', 'add') and len(ops) >= 2 and rn(ops[0]) is not None:
            if ops[1].type == capstone.arm.ARM_OP_IMM:
                regs[rn(ops[0])] = (regs.get(rn(ops[0])) or 0) + ops[1].imm
            elif rn(ops[1]) is not None:
                regs[rn(ops[0])] = regs.get(rn(ops[1]))
        elif ins.mnemonic in ('lsls', 'lsl') and len(ops) >= 3 and rn(ops[0]) is not None:
            if rn(ops[1]) is not None and ops[2].type == capstone.arm.ARM_OP_IMM:
                regs[rn(ops[0])] = ((regs.get(rn(ops[1])) or 0) << ops[2].imm) & 0xffffffff
        elif ins.mnemonic == 'str' and len(ops) >= 2 and rn(ops[1]) is not None:
            mem = ops[0].mem
            if mem.base == capstone.arm.ARM_REG_SP:
                stack[mem.disp] = regs.get(rn(ops[1]))

        # --- GL 调用标注 ---
        if ins.mnemonic in ('blx', 'bl') and ops and \
           ops[0].type == capstone.arm.ARM_OP_IMM:
            tgt = ops[0].imm
            if tgt in plt:
                args = []
                for i in range(4):
                    v = regs.get(capstone.arm.ARM_REG_R0 + i)
                    args.append(_fmt_val(v))
                line += '\n         → %s(%s, %s, %s, %s)' % (plt[tgt], *args)
        print(line)


def _relocs(orig=ORIG):
    """→ {offset: (type, value)}。
    ★ R_ARM_RELATIVE 的 addend **不在 readelf 文本输出里**（那列是空的），
      addend 存在 .rel.dyn 条目对应的**目标地址处的 4 字节**里 → 必须解析节二进制。
    R_ARM_JUMP_SLOT(22) / R_ARM_GLOB_DAT(21) 等按 r_info>>8 取 .dynsym 符号名。"""
    data = open(orig, 'rb').read()
    # 找 .rel.dyn / .rel.plt 节
    secs = {}
    for line in subprocess.run(['arm-linux-gnueabi-readelf', '-SW', orig],
                               capture_output=True, text=True).stdout.splitlines():
        m = re.search(r'\[\s*\d+\]\s+(\.rel\.\S+)\s+\S+\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)',
                      line)
        if m:
            secs[m.group(1)] = (int(m.group(3), 16), int(m.group(4), 16))
    # .dynsym: 地址→符号名
    dynsym = {}
    syms = []
    for line in subprocess.run(['arm-linux-gnueabi-readelf', '-sW', orig],
                               capture_output=True, text=True).stdout.splitlines():
        m = re.match(r'\s*(\d+):\s+([0-9a-f]+)\s+\S+\s+\S+\s+\S+\s+\S+\s+\S+\s+(\S+)', line)
        if m:
            syms.append((int(m.group(1)), m.group(3)))

    rel = {}
    for name, (off, size) in secs.items():
        for i in range(size // 8):
            e = off + i * 8
            r_off, r_info = struct.unpack_from('<II', data, e)
            rtype = r_info & 0xff
            if rtype == 23:      # R_ARM_RELATIVE
                if r_off + 4 <= len(data):
                    rel[r_off] = ('RELATIVE', struct.unpack_from('<I', data, r_off)[0])
            elif rtype in (21, 22):   # GLOB_DAT / JUMP_SLOT
                si = r_info >> 8
                sym = next((s for n, s in syms if n == si), '?')
                rel[r_off] = ('SYM', sym)
    return rel


def vtable_dump(addr, count=64, orig=ORIG):
    """dump vtable（.data.rel.ro 里一串 RELATIVE 重定位 → 函数地址）。
    用法: vtable_dump(0x159720, 40)"""
    rel = _relocs(orig)
    print('=== vtable @ 0x%x（%d 槽）===' % (addr, count))
    for i in range(count):
        a = addr + i * 4
        if a in rel:
            kind, val = rel[a]
            if kind == 'RELATIVE':
                print('  [%3d] 0x%x → 0x%x' % (i, a, val))
            else:
                print('  [%3d] 0x%x → %s' % (i, a, val))
        else:
            print('  [%3d] 0x%x → (无重定位)' % (i, a))


def _str_mem(ins):
    """取 str 指令的 MEM 操作数（★ str 的操作数顺序是 [源, 目的]，MEM 是 operands[1]）。"""
    ops = ins.operands
    if len(ops) < 2:
        return None
    for o in ops:
        if o.type == capstone.arm.ARM_OP_MEM:
            return o.mem
    return None


def find_stores(offset, orig=ORIG):
    """扫 .text 找 `str rX, [rY, #offset]`（写该偏移的指令）→ 候选写入点。
    用法: find_stores(0x1c)  # 纹理像素指针字段"""
    data = open(orig, 'rb').read()
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True
    md.skipdata = True
    hits = []
    for ins in md.disasm(data[TEXT_OFF:TEXT_OFF + TEXT_SZ], TEXT_OFF):
        if ins.mnemonic != 'str':
            continue
        m = _str_mem(ins)
        if m is None:
            continue
        if m.disp == offset and m.base != capstone.arm.ARM_REG_SP:
            hits.append((ins.address, ins.op_str))
    print('=== str [*, #0x%x] 命中 %d 处 ===' % (offset, len(hits)))
    for a, s in hits[:60]:
        print('  0x%x: str %s' % (a, s))
    return hits


def findvtable(func, orig=ORIG):
    """找哪个 vtable 的哪个槽 == func（func 给 Thumb 地址，偶数=函数首地址）。
    用法: findvtable 0x9e2b4  →  打印 vtable 基址 + 槽号
    实现：扫全部 R_ARM_RELATIVE 重定位（值 == func），再回推候选 vtable 基址
    （槽号未知 → 把每个命中点都按 '基址=命中点-槽号*4' 列出，并标出 slot22/23）。"""
    rel = _relocs(orig)
    tgt = func & ~1        # Thumb 位去掉
    hits = [(off, val) for off, (k, val) in rel.items()
            if k == 'RELATIVE' and (val & ~1) == tgt]
    print('=== 找含 0x%x 的 vtable 槽：命中 %d 处 ===' % (tgt, len(hits)))
    for off, val in hits:
        print('  槽地址 0x%x → 0x%x' % (off, val))
        for slot in (22, 23, 31, 33, 35):
            base = off - slot * 4
            if base % 4 == 0 and base > 0x150000:
                print('      若为 slot[%d] → vtable 基址 = 0x%x' % (slot, base))
    # 反向：把每个命中点当 slot0 试解
    print('  --- 候选 vtable 基址（按命中点为 slot0 试解，看 slot22/23 是否合理）---')
    for off, val in hits:
        base = off
        ok = True
        for slot in (22, 23):
            a = base + slot * 4
            if a not in rel or rel[a][0] != 'RELATIVE':
                ok = False
                break
        if ok:
            print('      候选基址 0x%x：slot22=0x%x slot23=0x%x'
                  % (base, rel[base + 88][1], rel[base + 92][1]))


def texclass(vtable_base=0x1595c0, slots=72, orig=ORIG):
    """纹理类 vtable 分析：列出每个虚方法，并标出**写像素字段**（this[0x1c]/[0x20]/[0x24]）的。
    用法: texclass [vtable_base]"""
    data = open(orig, 'rb').read()
    rel = _relocs(orig)
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True
    md.skipdata = True
    print('=== 纹理类 vtable 0x%x：找写像素字段的虚方法 ===' % vtable_base)
    for i in range(slots):
        a = vtable_base + i * 4
        if a not in rel or rel[a][0] != 'RELATIVE':
            continue
        f = rel[a][1] & ~1
        # 在该方法后 0x400 字节内找写 0x1c/0x20/0x24 的 str
        writes = []
        for ins in md.disasm(data[f:f + 0x400], f):
            if ins.mnemonic != 'str':
                continue
            m = _str_mem(ins)
            if m is None:
                continue
            if m.disp in (0x1c, 0x20, 0x24) and m.base != capstone.arm.ARM_REG_SP:
                writes.append((ins.address, m.disp))
        mark = '  ★写像素字段: ' + ', '.join('0x%x(+0x%x)' % w for w in writes[:6]) if writes else ''
        print('  slot[%2d] 0x%06x%s' % (i, f, mark))


def funcat(addr, seed=None, orig=ORIG, show=True):
    seed = seed or ('/tmp/opencode/seed_v2.json' if os.path.exists('/tmp/opencode/seed_v2.json')
                    else '/disk/workspaces/jq/rev64/dist/seed_funcs.json')
    """找包含 addr 的函数（用 seed_v2.json 函数起始表二分），并可打印其反汇编。"""
    import bisect
    if not os.path.exists(seed):
        print('★ 缺 %s' % seed)
        return None
    starts = sorted(json.load(open(seed)))
    i = bisect.bisect_right(starts, addr) - 1
    if i < 0:
        print('★ 0x%x 之前无已知函数' % addr)
        return None
    st = starts[i]
    en = starts[i + 1] if i + 1 < len(starts) else st + 0x200
    print('=== 0x%x 属于函数 0x%x-0x%x（seed_v2 idx %d）===' % (addr, st, en, i))
    if show:
        data = open(orig, 'rb').read()
        md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
        md.detail = True
        for ins in md.disasm(data[st:en], st):
            line = '  0x%x: %s %s' % (ins.address, ins.mnemonic, ins.op_str)
            if ins.mnemonic == 'str':
                m = _str_mem(ins)
                if m is not None and m.disp in (0x1c, 0x20, 0x24, 0x28, 0x2c, 0x30, 0x34):
                    line += '   ★写纹理字段 +0x%x' % m.disp
            print(line)
    return st


def findctor(vtable, orig=ORIG):
    """找把 this[0] 设为给定 vtable 的构造器。
    模式（Thumb）: ldr rX,[pc,#N] ; add rX, pc[, #imm] ; adds rX, #8 ; str rX, [rY]
      → 值 = pool + Align(pc_ins+4,4) + imm + 8
    也顺带列出所有"设 vptr"的构造器（用于类图）。"""
    data = open(orig, 'rb').read()
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True
    md.skipdata = True
    insns = list(md.disasm(data[TEXT_OFF:TEXT_OFF + TEXT_SZ], TEXT_OFF))
    idx = {ins.address: i for i, ins in enumerate(insns)}
    found = []
    for i, ins in enumerate(insns):
        if ins.mnemonic != 'ldr' or len(ins.operands) != 2:
            continue
        m = ins.operands[1].mem
        if m.base != capstone.arm.ARM_REG_PC:
            continue
        pool_addr = ((ins.address + 4) & ~3) + m.disp
        if pool_addr + 4 > len(data):
            continue
        pool_val = struct.unpack_from('<I', data, pool_addr)[0]
        # 后一条必须是 add rX, pc
        if i + 1 >= len(insns):
            continue
        nxt = insns[i + 1]
        if not (nxt.mnemonic == 'add' and 'pc' in nxt.op_str and nxt.op_str.split(',')[0].strip() ==
                ins.op_str.split(',')[0].strip()):
            continue
        # add rX, pc 的立即数
        add_imm = 0
        mm = re.search(r'#(0x[0-9a-f]+|\d+)\s*$', nxt.op_str)
        if mm:
            add_imm = int(mm.group(1), 0)
        val = pool_val + ((nxt.address + 4) & ~3) + add_imm
        # 再看是否 +8 并 str
        if i + 2 < len(insns):
            n2 = insns[i + 2]
            if n2.mnemonic in ('adds', 'add') and n2.op_str.startswith(ins.op_str.split(',')[0].strip()):
                mm2 = re.search(r'#(0x[0-9a-f]+|\d+)', n2.op_str)
                if mm2:
                    val += int(mm2.group(1), 0)
        found.append((ins.address, val))
    print('=== 设 vptr 的构造器（%d 处），找 vtable 0x%x ===' % (len(found), vtable))
    hit = False
    for a, v in found:
        if v == vtable:
            print('  ★ 0x%x → vtable 0x%x' % (a, v))
            hit = True
    if not hit:
        print('  未直接命中（vtable 可能经基类/多级构造设置）')
    print('  --- 全部候选（取前 40）---')
    for a, v in found[:40]:
        print('    0x%x → 0x%x' % (a, v))
    return found


def vslots_in(start, end, orig=ORIG):
    """列出函数区间内所有 vtable 槽 dispatch（ldr rX,[rY,#slot*4] + blx rX）→ 槽号。
    用法: vslots_in 0xbf4d0 0xbf800"""
    data = open(orig, 'rb').read()
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True
    md.skipdata = True
    insns = list(md.disasm(data[start:end], start))
    out = []
    for i, ins in enumerate(insns):
        if ins.mnemonic != 'ldr' or len(ins.operands) != 2:
            continue
        m = ins.operands[1].mem
        if m.base == capstone.arm.ARM_REG_PC or m.disp % 4 or m.disp > 0x400:
            continue
        slot = m.disp // 4
        dst = ins.operands[0].reg
        for j in range(i + 1, min(i + 6, len(insns))):
            b = insns[j]
            if b.mnemonic == 'blx' and len(b.operands) == 1 and \
               b.operands[0].type == capstone.arm.ARM_OP_REG and b.operands[0].reg == dst:
                out.append((ins.address, b.address, slot, ins.op_str))
                break
            if b.mnemonic in ('bx', 'bl'):
                break
    print('=== 0x%x-0x%x 内 vtable 槽 dispatch: %d 处 ===' % (start, end, len(out)))
    agg = {}
    for a, b, s, s2 in out:
        agg.setdefault(s, []).append(a)
    for s in sorted(agg):
        print('  slot[%3d] (disp=0x%x) × %d  → %s' % (s, s * 4, len(agg[s]),
              ', '.join('0x%x' % x for x in agg[s][:8])))
    return out


def _vt_of_func(f, data, md, rel, span=0x60):
    """在函数前 span 字节内找 pc 相对寻址模式 → 返回 (kind, addr, ins_addr)。
    kind:
      'vtable'  — 计算出的是 vtable 基址（随后 str 到 this=[r0]/[r1]）
      'global'  — 计算出的是全局/静态对象槽地址（随后 ldr/str 到该槽）
      None      — 未识别
    """
    insns = list(md.disasm(data[f:f + span], f))
    for i, ins in enumerate(insns):
        if ins.mnemonic != 'ldr' or len(ins.operands) != 2:
            continue
        m = ins.operands[1].mem
        if m.base != capstone.arm.ARM_REG_PC:
            continue
        pool_addr = ((ins.address + 4) & ~3) + m.disp
        if pool_addr + 4 > len(data):
            continue
        pool_val = struct.unpack_from('<I', data, pool_addr)[0]
        dst = ins.op_str.split(',')[0].strip()
        for j in range(i + 1, min(i + 4, len(insns))):
            nxt = insns[j]
            if nxt.mnemonic == 'add' and nxt.op_str.startswith(dst) and 'pc' in nxt.op_str:
                add_imm = 0
                mm = re.search(r'#(0x[0-9a-f]+|\d+)\s*$', nxt.op_str)
                if mm:
                    add_imm = int(mm.group(1), 0)
                val = pool_val + ((nxt.address + 4) & ~3) + add_imm
                for k in range(j + 1, min(j + 4, len(insns))):
                    n2 = insns[k]
                    if n2.mnemonic in ('adds', 'add') and n2.op_str.startswith(dst):
                        mm2 = re.search(r'#(0x[0-9a-f]+|\d+)', n2.op_str)
                        if mm2:
                            val += int(mm2.group(1), 0)
                    if n2.mnemonic == 'str' and n2.op_str.startswith(dst):
                        tgt = n2.op_str.split('[')[1].split(',')[0].strip().rstrip(']')
                        kind = 'vtable' if tgt in ('r0', 'r1') else 'global'
                        return kind, val, ins.address
                return 'global', val, ins.address
    return None, None, None


def scenechain(root=0x242f4, orig=ORIG):
    """场景构造链分析：列出 F_242f4（场景初始化）直接调用的构造器，
    每个报告：函数地址 / 大小 / 它安装的 vtable / 该 vtable 是否存在于 .data.rel.ro。
    用法: scenechain [0x242f4]"""
    data = open(orig, 'rb').read()
    rel = _relocs(orig)
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True
    md.skipdata = True
    # 函数边界（seed_v2）
    starts = []
    _seed = '/tmp/opencode/seed_v2.json' if os.path.exists('/tmp/opencode/seed_v2.json') else '/disk/workspaces/jq/rev64/dist/seed_funcs.json'
    if os.path.exists(_seed):
        starts = sorted(json.load(open(_seed)))
    # 收集 root 的直接 bl 目标
    callees = []
    for ins in md.disasm(data[root:root + 0x200], root):
        if ins.mnemonic == 'bl' and ins.operands and \
           ins.operands[0].type == capstone.arm.ARM_OP_IMM:
            t = ins.operands[0].imm
            if t not in callees:
                callees.append(t)
    print('=== 场景构造链 root=0x%x：%d 个直接被调函数 ===' % (root, len(callees)))
    for c in callees:
        size = 0
        if starts:
            import bisect
            i = bisect.bisect_right(starts, c) - 1
            if i >= 0 and i + 1 < len(starts) and starts[i] == c:
                size = starts[i + 1] - c
        kind, val, at = _vt_of_func(c, data, md, rel)
        info = ''
        if kind == 'vtable':
            ok = any((val + k * 4) in rel and rel[val + k * 4][0] == 'RELATIVE'
                     for k in range(0, 60))
            tex = []
            for k in range(0, 60):
                a = val + k * 4
                if a in rel and rel[a][0] == 'RELATIVE':
                    v = rel[a][1] & ~1
                    if v in (0x9e2b4, 0x9e0d4, 0x9e204, 0x9e0f0):
                        tex.append('slot%d' % k)
            info = 'vtable=0x%x %s%s' % (val, '✓' if ok else '(非vtable?)',
                                          (' 含纹理方法:' + ','.join(tex)) if tex else '')
        elif kind == 'global':
            info = '全局静态槽=0x%x' % val
        print('  0x%06x  size~%-5d %s' % (c, size, info))
    return callees


def closure(root, orig=ORIG, src_dir='/disk/workspaces/jq/rev64/src_jni_translated',
            maxn=4000):
    """从 root 出发的**调用闭包**（只跟 bl/blx 直接分支），并对比翻译体里
    "已完整翻译 / 仅存根 / 完全没有" 的分布 → 明确"还差多少函数才能跑通"。
    翻译体状态判定：读 src 目录里 jqt_*.c 的函数定义形态。
    用法: closure 0x242f4"""
    data = open(orig, 'rb').read()
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True
    md.skipdata = True
    starts = []
    _seed = '/tmp/opencode/seed_v2.json' if os.path.exists('/tmp/opencode/seed_v2.json') else '/disk/workspaces/jq/rev64/dist/seed_funcs.json'
    if os.path.exists(_seed):
        starts = sorted(json.load(open(_seed)))
    sset = set(starts)

    # 翻译体函数状态表：name -> 'full' | 'stub'
    status = {}
    if os.path.isdir(src_dir):
        for f in os.listdir(src_dir):
            if not f.endswith('.c'):
                continue
            txt = open(os.path.join(src_dir, f), errors='ignore').read()
            for m in re.finditer(r'^uint32_t\s+(F_[0-9a-f]+)\(([^)]*)\)\s*\{', txt, re.M):
                nm, params = m.group(1), m.group(2)
                named = bool(re.search(r'_\w+\s*[,)]', params))
                status[nm] = 'full' if named else 'stub'

    def name_of(addr):
        return 'F_%x' % (addr & ~1)

    seen = set()
    stack = [root]
    edges = {}
    while stack and len(seen) < maxn:
        f = stack.pop()
        if f in seen or f not in sset:
            continue
        seen.add(f)
        kids = []
        try:
            for ins in md.disasm(data[f:f + 0x800], f):
                if ins.mnemonic in ('bl', 'blx') and ins.operands and \
                   ins.operands[0].type == capstone.arm.ARM_OP_IMM:
                    t = ins.operands[0].imm
                    if t in sset and t not in seen:
                        kids.append(t)
        except Exception:
            pass
        edges[f] = kids
        stack.extend(kids)

    full = stub = missing = 0
    miss_list = []
    for f in sorted(seen):
        st = status.get(name_of(f))
        if st == 'full':
            full += 1
        elif st == 'stub':
            stub += 1
        else:
            missing += 1
            miss_list.append(f)
    print('=== 调用闭包 root=0x%x：%d 个函数 ===' % (root, len(seen)))
    print('  完整翻译 : %d' % full)
    print('  仅存根   : %d' % stub)
    print('  ★ 翻译体完全没有: %d' % missing)
    if miss_list:
        print('  --- 缺失清单（前 30，按地址）---')
        print('  ' + ' '.join('0x%x' % x for x in miss_list[:30]))
        out = '/tmp/opencode/missing_funcs_0x%x.txt' % root
        with open(out, 'w') as f:
            f.write('# 翻译体缺失函数（root=0x%x 闭包，共 %d 个）\n' % (root, len(miss_list)))
            for x in miss_list:
                f.write('0x%x\n' % x)
        print('  完整清单已存: %s' % out)
    return seen, miss_list


def callers(target, orig=ORIG, limit=40):
    """反查：谁调用了 target（扫全 .text 的 bl/blx 立即数目标）。
    用法: callers 0x242f4"""
    data = open(orig, 'rb').read()
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True
    md.skipdata = True
    hits = []
    for ins in md.disasm(data[TEXT_OFF:TEXT_OFF + TEXT_SZ], TEXT_OFF):
        if ins.mnemonic in ('bl', 'blx') and ins.operands and \
           ins.operands[0].type == capstone.arm.ARM_OP_IMM:
            if ins.operands[0].imm == target:
                hits.append(ins.address)
    print('=== 谁调用 0x%x：%d 处 ===' % (target, len(hits)))
    for h in hits[:limit]:
        print('  0x%x' % h)
    return hits


def und_sym_diff():
    """对比翻译体 vs 原版 UND 符号（dlopen failed to link 排查）。"""
    def und(so):
        out = subprocess.run(['arm-linux-gnueabi-readelf', '-sW', so],
                             capture_output=True, text=True).stdout
        return set(re.findall(r'UND\s+(\S+)', out))
    t = und('/disk/workspaces/jq/rev64/dist/libjunqi_arm32_aligned.so')
    o = und(ORIG)
    only_t = t - o
    print('翻译体独有 UND（原版没有）:', sorted(only_t))

# ==================== §124zz-8 机械化修复工具（本轮手改模式固化） ====================


def rpi_scan(out_json='/tmp/opencode/rpi_vals.json'):
    """批量扫全 .text 的 RPI 三连（ldr rX,[pc,#disp] → add rX,pc → str rX,[rY,#d] / 解引用），
    用原版 .so 静态字节 + RELATIVE 重定位表算出每条真实槽值 → 生成 jqt_mem.c 注入表。
    对照 jni_aligned.c 手写 18 值表（g_jq_got_tbl）的机械化替代：全量 RPI 值，非手工逐个找。
    用法：python3 tex_pipeline.py rpi"""
    import struct, subprocess, re as _re
    data = open(ORIG, 'rb').read()
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True
    md.skipdata = True
    # RELATIVE 重定位目标（对照 槽值 = load_base + addend；翻译体 load_base = g_jqt_shadow）
    out = subprocess.run(['arm-linux-gnueabi-readelf', '-r', ORIG],
                          capture_output=True, text=True).stdout
    rel = set()
    for l in out.splitlines():
        m = _re.match(r'\s*([0-9a-f]+)\s+[0-9a-f]+\s+R_ARM_RELATIVE', l)
        if m:
            rel.add(int(m.group(1), 16))
    vals = {}
    # 扫全 .text（对照 ldr rX,[pc,#N] → add rX,pc → str/ldr 解引用的 RPI 模式）
    insns = list(md.disasm(data[TEXT_OFF:TEXT_OFF+TEXT_SZ], TEXT_OFF))
    for i in range(len(insns) - 2):
        a = insns[i]
        if a.mnemonic != 'ldr' or len(a.operands) != 2:
            continue
        op1 = a.operands[1]
        if op1.type != capstone.arm.ARM_OP_MEM:
            continue
        mem = op1.mem
        if mem.base != capstone.arm.ARM_REG_PC:
            continue
        rd = a.operands[0].reg
        pool = (a.address & ~2) + mem.disp
        if pool + 4 > len(data):
            continue
        pool_val = struct.unpack_from('<I', data, pool)[0]
        # add rX,pc
        b = insns[i+1]
        if not (b.mnemonic == 'add' and len(b.operands) == 2 and b.operands[0].reg == rd and
                b.operands[1].type == capstone.arm.ARM_OP_REG and
                b.operands[1].reg == capstone.arm.ARM_REG_PC):
            continue
        # RPI 目标：pool_val + pc（pc 运行时 = 基址 + b.address）
        # 翻译体语义：槽值（MEM 读）应 = pool_val + 目标偏移。对照 RELATIVE 重定位：
        #   若 pool 在 rel 列表 → 槽值 = load_base + pool_val（addend=pool_val）
        #   若 pool 不在 rel → 槽值 = pool_val + (b.address)（纯 RPI 偏移，编译期定值）
        if pool in rel:
            target = pool_val  # addend（翻译体 MEM 读偏移 = addend）
            kind = 'relative'
        else:
            target = (pool_val + b.address) & 0xFFFFFFFF
            kind = 'static'
        # 找写点（str rX,[rY,#d] 或 ldr rZ,[rX]）
        write_to = None
        for c in insns[i+2:i+6]:
            if c.mnemonic in ('str','strh','strb') and c.operands and \
               c.operands[0].type == capstone.arm.ARM_OP_REG and c.operands[0].reg == rd and \
               len(c.operands) >= 2 and c.operands[1].type == capstone.arm.ARM_OP_MEM:
                write_to = (c.operands[1].mem.base, c.operands[1].mem.disp, c.address)
                break
        if write_to is None:
            continue
        base, disp, waddr = write_to
        key = '0x%x' % waddr
        vals[key] = {
            'ldr_addr': '0x%x' % a.address,
            'pool': '0x%x' % pool,
            'pool_val': '0x%08x' % pool_val,
            'kind': kind,
            'target': '0x%08x' % target,
            'write_reg': str(base),
            'write_disp': '0x%x' % disp,
        }
    json.dump(vals, open(out_json, 'w'), indent=1)
    print('=== RPI 扫描：%d 条（落盘 %s）===' % (len(vals), out_json))
    n_rel = sum(1 for v in vals.values() if v['kind'] == 'relative')
    n_sta = sum(1 for v in vals.values() if v['kind'] == 'static')
    print('  RELATIVE 重定位槽: %d / 静态 RPI: %d' % (n_rel, n_sta))
    for k in list(vals)[:10]:
        v = vals[k]
        print('  写@%s [r%s+#%s] = 0x%s（%s，ldr@%s pool@%s val=0x%s）' % (
            k, str(v['write_reg']).replace('CPUARM_REG_',''), v['write_disp'],
            v['target'], v['kind'], v['ldr_addr'], v['pool'], v['pool_val']))


def i2f_sites(fix=False):
    """列出 .text 里所有 blx → __aeabi_i2f 的调用点（翻译体存根替换清单）。
    用法：python3 tex_pipeline.py i2f [--fix]
    --fix：in-place 把 src_jni_translated/*.c 里 "r0=0; /* blx 0x232c8 存根 */"
    批量替换为 i2f 真实实现（r0=int32 → float 位模式）。"""
    plt, _ = _plt_map_cached()
    i2f_addrs = [a for a, sp in plt.items() if sp == '__aeabi_i2f']
    print('=== __aeabi_i2f PLT 桩: %s ===' % [hex(a) for a in i2f_addrs])
    for target in i2f_addrs:
        found = callers(target, limit=300)
        print('--- blx 0x%x（__aeabi_i2f）调用点：%d 处' % (target, len(found)))
    if fix:
        import glob
        n = 0
        stub = 'r0=0; /* blx 0x232c8 存根 */'
        real = '{ float _f=(float)(int32_t)r0; r0=*(uint32_t*)&_f; } /* blx 0x232c8 __aeabi_i2f 真实 */'
        for f in sorted(glob.glob('/disk/workspaces/jq/rev64/src_jni_translated/*.c')):
            t = open(f).read()
            c = t.count(stub)
            if c:
                t = t.replace(stub, real)
                open(f, 'w').write(t)
                n += c
                print('  修 %s: %d 处' % (f.split('/')[-1], c))
        print('i2f --fix 共修 %d 处存根' % n)

# RPI 写点扫描（§〇b 公式修正 + §〇d RPI+adds 模式，机械化替代手工逐个找）
def _rpi_writes(data, insns, extra=0, max_span=10):
    """扫全 .text 的 RPI vtable 写点：ldr rX,[pc,#disp] → add[s] rX,pc [, #imm] → str rX,[reg].
    返回 [(ldr_addr, add_addr, add_imm, str_addr, rd, target)]。
    target = pool_val + (add_addr+4) + add_imm（§〇b 正确公式 pool=((addr+4)&~3)+disp）。
    extra：额外接受 add rX,#imm（RPI+N 模式，N 在 add 指令后追加，§〇d adds r3,#8）。
    max_span：add 与 str 之间最大指令间距。"""
    reg_imm = {capstone.arm.ARM_OP_IMM, capstone.arm.ARM_OP_REG}
    hits = []
    for i in range(len(insns) - 2):
        a = insns[i]
        if a.mnemonic != 'ldr' or len(a.operands) != 2:
            continue
        m = a.operands[1]
        if m.type != capstone.arm.ARM_OP_MEM or m.mem.base != capstone.arm.ARM_REG_PC:
            continue
        rd = a.operands[0].reg
        pool = ((a.address + 4) & ~3) + m.mem.disp   # §〇b 正确公式（旧 (addr&~2)+disp 差 4B）
        if pool + 4 > len(data):
            continue
        rpi = struct.unpack_from('<I', data, pool)[0]
        # 找 add[s] rX,pc 或 add[s] rX,pc,#imm（rX 与 ldr 目标同寄存器）
        for j in range(i + 1, min(i + max_span, len(insns))):
            b = insns[j]
            if b.mnemonic not in ('add', 'adds'):
                continue
            ops = b.operands
            if not ops or ops[0].reg != rd:
                continue
            # 形式1: add rX, pc  / add rX, pc, #imm
            if len(ops) >= 2 and ops[1].type == capstone.arm.ARM_OP_REG and \
               ops[1].reg == capstone.arm.ARM_REG_PC:
                imm = ops[2].imm if (len(ops) >= 3 and ops[2].type == capstone.arm.ARM_OP_IMM) else 0
                base = rpi + (b.address + 4)
                # 形式2（RPI+N）：同流后续 add[s] rX, #imm（§〇d adds r3,#8）
                for k in range(j + 1, min(j + 4, len(insns))):
                    c = insns[k]
                    if c.mnemonic in ('add', 'adds') and c.operands and \
                       c.operands[0].reg == rd and len(c.operands) == 2 and \
                       c.operands[1].type == capstone.arm.ARM_OP_IMM:
                        imm += c.operands[1].imm
                        j = k
                    else:
                        break
                target = base + imm
                # 找 str rX,[reg] / stm
                for k in range(j + 1, min(j + 6, len(insns))):
                    c = insns[k]
                    if c.mnemonic in ('str', 'stm') and c.operands and c.operands[0].reg == rd:
                        hits.append((a.address, b.address, imm, c.address, rd, target))
                        break
                break
    return hits


def vtwrite(target_arg):
    """找写 vtable=X（.data.rel.ro 地址）的构造器（§124zz-6 RPI 模式）。
    搜 .text 里 ldr rX,[pc] → add[s] rX,pc [, #imm] → str rX,[reg] 三连，RPI 解码 == X。
    §〇b 池地址公式修正（((addr+4)&~3)+disp）+ §〇d RPI+adds（adds r3,#8）模式接受。
    dlopen 后 RELATIVE 重定位 → 槽值 = 基址+X（绝对地址）。
    翻译体写 MEM(this+0) = JQT_ABS_PTR(偏移形式) 才是对的。
    用法：python3 tex_pipeline.py vtwrite 0x1577e0"""
    target = int(target_arg, 16)
    data = open(ORIG, 'rb').read()
    _md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    _md.detail = True
    _md.skipdata = True
    insns = list(_md.disasm(data[TEXT_OFF:TEXT_OFF + TEXT_SZ], TEXT_OFF))
    hits = [h for h in _rpi_writes(data, insns) if h[5] == target]
    print('=== 写 vtable=0x%x 的构造器（.text RPI 解码）：%d 处 ===' % (target, len(hits)))
    for a_addr, b_addr, imm, c_addr, rd, t in hits:
        print('  ldr@0x%x add@0x%x +%#x str@0x%x r%d → 0x%x' % (a_addr, b_addr, imm, c_addr, rd, t))
    out = subprocess.run(['arm-linux-gnueabi-readelf', '-r', ORIG],
                         capture_output=True, text=True).stdout
    rels = re.findall(r'([0-9a-f]+)\s+[0-9a-f]+\s+R_ARM_RELATIVE', out)
    has = ('0x%08x' % target) in rels or ('%08x' % target) in rels
    print('  0x%x 在 R_ARM_RELATIVE 列表? %s（True → 运行时=基址+0x%x 绝对地址，翻译体必须写 JQT_ABS_PTR）'
          % (target, has, target))


def vtwrites():
    """扫全 .text 的所有 RPI vtable 写点（无需预知目标），按 target 分组统计。
    机械化发现类 A/B vtable 构造器（§〇d RPI+8 模式）。
    用法：python3 tex_pipeline.py vtwrites [min] [max]   # 可选按 target 范围过滤"""
    lo = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0
    hi = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x180000
    data = open(ORIG, 'rb').read()
    _md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    _md.detail = True
    _md.skipdata = True
    insns = list(_md.disasm(data[TEXT_OFF:TEXT_OFF + TEXT_SZ], TEXT_OFF))
    hits = [h for h in _rpi_writes(data, insns) if lo <= h[5] < hi]
    by = {}
    for h in hits:
        by.setdefault(h[5], []).append(h)
    print('=== RPI vtable 写点：%d 处，%d 个目标（0x%x-0x%x）===' % (len(hits), len(by), lo, hi))
    for t in sorted(by):
        rows = by[t]
        print('\n-- vtable=0x%x（%d 个写点，RELATIVE=%s）' % (
            t, len(rows),
            '是' if ('%08x' % t) in (subprocess.run(
                ['arm-linux-gnueabi-readelf', '-r', ORIG],
                capture_output=True, text=True).stdout) else '否'))
        for a_addr, b_addr, imm, c_addr, rd, _t in rows[:6]:
            print('    ldr@0x%x add@0x%x +%#x str@0x%x r%d' % (a_addr, b_addr, imm, c_addr, rd))
        if len(rows) > 6:
            print('    ... %d more' % (len(rows) - 6))

# 已知 vtable 基址（§〇c/§〇d 实证，资产复用：来自 AGENTS.md 单例/类分析）
KNOWN_VTABLES = [0x1558d8, 0x1595c0, 0x159ed0, 0x1577e0, 0x157800]

# RPI 写点缓存（vtowners 复用，避免每次重扫 48 万指令）
_RPI_CACHE = None
def _rpi_targets():
    """所有 RPI 写点 target 集合（= vtable 基址候选，this[0]=vtable 显式写入处）。"""
    global _RPI_CACHE
    if _RPI_CACHE is None:
        data = open(ORIG, 'rb').read()
        _md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
        _md.detail = True
        _md.skipdata = True
        insns = list(_md.disasm(data[TEXT_OFF:TEXT_OFF + TEXT_SZ], TEXT_OFF))
        _RPI_CACHE = set(h[5] for h in _rpi_writes(data, insns))
    return _RPI_CACHE


def vtowners(func_addr):
    """找函数属于哪个 vtable（.data.rel.ro 里 RELATIVE 槽值 == func 的槽）。
    机械化查"函数→类归属"（§〇d 反思总结：复用 rpi_audit 公式）。
    vtable 基址候选 = RPI 写点 target（this[0]=vtable 显式写入处，§〇b 公式修正）。
    用法：python3 tex_pipeline.py vtowners 0x77b10"""
    func = func_addr & ~1
    rel = _relocs()
    slots = {a: v & ~1 for a, (k, v) in rel.items()
             if k == 'RELATIVE' and TEXT_OFF <= (v & ~1) < TEXT_OFF + TEXT_SZ}
    hits = sorted(a for a, v in slots.items() if v == func)
    print('=== 0x%x 的 vtable 槽（%d 个）===' % (func_addr, len(hits)))
    if not hits:
        return
    for s in hits:
        cand = None
        # ① 已知 vtable 基址（资产复用，最可靠）
        for b in sorted(KNOWN_VTABLES, reverse=True):
            off = s - b
            if off >= 0 and off % 4 == 0 and off < 0x200:
                cand = b
                break
        # ② 回退：RPI 写点 target（this[0]=vtable 显式写入处）
        if cand is None:
            bases = sorted(_rpi_targets())
            lo, hi = 0, len(bases)
            while lo < hi:
                mid = (lo + hi) // 2
                if bases[mid] <= s:
                    lo = mid + 1
                else:
                    hi = mid
            for b in reversed(bases[:lo]):
                off = s - b
                if off >= 0 and off % 4 == 0 and off < 0x200:
                    cand = b
                    break
        if cand is not None:
            print('  slot@0x%x  vtable=0x%x[%d]' % (s, cand, (s - cand) // 4))
        else:
            base = s
            while base - 4 in rel and rel[base - 4][0] == 'RELATIVE':
                base -= 4
            print('  slot@0x%x  vtable=0x%x[%d]（无候选，连续运行回退）' % (s, base, (s - base) // 4))


def upchain(addr, depth=6, limit=200):
    """上游 bl/blx 调用链 BFS（transitive callers）。机械化反推触发链（§〇d）。
    用法：python3 tex_pipeline.py upchain 0x9e2b4 [depth]"""
    data = open(ORIG, 'rb').read()
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True
    md.skipdata = True
    insns = list(md.disasm(data[TEXT_OFF:TEXT_OFF + TEXT_SZ], TEXT_OFF))
    direct = {}
    for ins in insns:
        if ins.mnemonic in ('bl', 'blx') and ins.operands and \
           ins.operands[0].type == capstone.arm.ARM_OP_IMM:
            t = ins.operands[0].imm
            direct.setdefault(t, []).append(ins.address)
    frontier = {addr}
    seen = set()
    allc = {}
    for d in range(depth):
        nxt = set()
        for f in frontier:
            for c in direct.get(f, []):
                if c in seen:
                    continue
                seen.add(c)
                allc.setdefault(c, d + 1)
                nxt.add(c)
        if not nxt:
            break
        frontier = nxt
        if len(seen) > limit:
            print('  （超 %d 上限，截断）' % limit)
            break
    print('=== 0x%x 上游调用链（BFS %d 层，%d 个调用点）===' % (addr, depth, len(seen)))
    for c in sorted(allc, key=lambda x: allc[x]):
        print('  层%d 0x%x' % (allc[c], c))
    return allc


def batchlog(*funcs):
    """批量给 jqt_vtbl.c 里的多个函数加入口插桩日志（§〇d 三连插桩模式机械化）。
    幂等：已有 ENTRY 日志/已有插桩注释的函数跳过。
    用法：python3 tex_pipeline.py batchlog 0x77840 0x77b10 0x23738"""
    f = '/disk/workspaces/jq/rev64/src_jni_translated/jqt_vtbl.c'
    src = open(f).read()
    lines = src.split('\n')
    done, skipped = [], []
    for fa in funcs:
        addr = int(fa, 16)
        name = 'F_%x' % addr
        pat = re.compile(r'uint32_t %s\([^)]*\)\{.*?g_cur_fn = 0x%x;' % (name, addr), re.S)
        m = pat.search(src)
        if not m:
            skipped.append((fa, '未找到定义体'))
            continue
        # 幂等：扫完整函数体（从定义到匹配的 }）看是否已有插桩
        # 找函数体结束（下一个 "\n}" 后跟空行或下一函数定义）
        body_end = src.find('\n}', m.end())
        if body_end == -1:
            body_end = m.end() + 300
        seg = src[m.end():body_end + 2]
        if 'BATCHLOG-ENTRY' in seg or '__android_log_print' in seg:
            skipped.append((fa, '已有插桩'))
            continue
        off = m.end()
        ln = src[:off].count('\n')
        indent = '    '
        ins = [
            '%s/* ★ BATCHLOG-ENTRY：%s 入口插桩（机械化批量，§〇d） */' % (indent, name),
            '%s{ __android_log_print(6,"junqi","ENTRY %s r0=0x%%08x r1=0x%%08x this_vt=0x%%08x", r0, r1, r0 ? MEM(r0) : 0); }' % (indent, name),
        ]
        lines.insert(ln + 1, '\n'.join(ins))
        done.append(fa)
    open(f, 'w').write('\n'.join(lines))
    print('=== batchlog：完成 %d，跳过 %d ===' % (len(done), len(skipped)))
    for d in done:
        print('  ✓ %s' % d)
    for fa, why in skipped:
        print('  ⤼ %s (%s)' % (fa, why))


def slot22chain():
    """slot22（纹理创建 F_9e2b4）触发链机械化推导（§〇d 反思总结"机械化链追查"）。
    一条命令出全链：vtable[22] dispatch 点 → 所属函数 → 函数 vtable 归属 → 上游调用链。
    用法：python3 tex_pipeline.py slot22chain"""
    import bisect
    seed = '/tmp/opencode/seed_v2.json' if os.path.exists('/tmp/opencode/seed_v2.json') else '/disk/workspaces/jq/rev64/dist/seed_funcs.json'
    starts = sorted(json.load(open(seed)))
    data, insns = disasm_all()
    # ① 一次性构建 bl/blx 直接调用映射（addr→callers）
    direct = {}
    for ins in insns:
        if ins.mnemonic in ('bl', 'blx') and ins.operands and \
           ins.operands[0].type == capstone.arm.ARM_OP_IMM:
            direct.setdefault(ins.operands[0].imm, []).append(ins.address)
    # ② vtable[22] dispatch 点 → 所属函数
    dps = find_vtable_dispatch(22)
    print('=== ① vtable[22]（纹理创建）dispatch 点：%d 处 ===' % len(dps))
    by_fn = {}
    for ldr_a, blx_a, op in dps:
        i = bisect.bisect_right(starts, ldr_a) - 1
        fn = starts[i] if i >= 0 else None
        by_fn.setdefault(fn, []).append((ldr_a, blx_a, op))
    for fn in sorted(by_fn, key=lambda x: (x is None, x)):
        rows = by_fn[fn]
        print('\n-- dispatch 所属函数 0x%x（%d 个 dispatch 点）--' % (fn if fn else 0, len(rows)))
        for ldr_a, blx_a, op in rows[:6]:
            print('    ldr@0x%x blx@0x%x  %s' % (ldr_a, blx_a, op))
        if len(rows) > 6:
            print('    ... %d more' % (len(rows) - 6))
        if fn:
            print('    函数 vtable 归属：')
            vtowners(fn)
    # ③ 上游调用链 BFS（复用 direct 映射，不再重反汇编）
    print('\n=== ③ 上游调用链（BFS 3 层，按函数）===')
    for fn in sorted(by_fn, key=lambda x: (x is None, x)):
        if not fn:
            continue
        seen, frontier, allc = set(), {fn}, {}
        for d in range(3):
            nxt = set()
            for f_ in frontier:
                for c in direct.get(f_, []):
                    if c in seen:
                        continue
                    seen.add(c)
                    allc.setdefault(c, d + 1)
                    nxt.add(c)
            if not nxt:
                break
            frontier = nxt
        print('\n-- 0x%x 上游（%d 个调用点）--' % (fn, len(seen)))
        for c in sorted(allc, key=lambda x: allc[x])[:20]:
            print('    层%d 0x%x' % (allc[c], c))


def _plt_map_cached():
    plt, got = _plt_map()
    return plt, got

def dispatchfix():
    """列出 .text 里所有 vtable dispatch 点（"ldr rX,[reg,#disp]; blx rX" 模式）。
    翻译体里每处 dispatch 槽值必须 JQT_ABS_PTR 透传（RELATIVE 后是绝对地址）。
    用法：python3 tex_pipeline.py dispatchfix [min_disp] [max_disp]（十六进制）"""
    data = open(ORIG, 'rb').read()
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True
    md.skipdata = True
    min_d = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0
    max_d = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x100
    sites = []
    insns = list(md.disasm(data[TEXT_OFF:TEXT_OFF + TEXT_SZ], TEXT_OFF))
    for i in range(1, len(insns)):
        a = insns[i]
        if a.mnemonic != 'blx' or not a.op_str:
            continue
        if not a.op_str.startswith('r'):
            continue
        rt = a.operands[0].reg
        for j in range(max(0, i - 3), i):
            p = insns[j]
            if p.mnemonic != 'ldr' or len(p.operands) != 2:
                continue
            if p.operands[0].reg != rt:
                continue
            if p.operands[1].type != capstone.arm.ARM_OP_MEM:
                continue
            pm = p.operands[1].mem
            disp = pm.disp
            if disp < min_d or disp > max_d:
                continue
            sites.append((a.address, p.address, rt, disp))
            break
    print('=== vtable dispatch 点（blx rX，前驱 ldr rX,[reg,#disp]）：%d 处（disp 范围 0x%x-0x%x）===' %
          (len(sites), min_d, max_d))
    # 对照翻译体存根状态
    import glob
    trans = {}
    for f in sorted(glob.glob('/disk/workspaces/jq/rev64/src_jni_translated/*.c')):
        for line in open(f):
            m2 = re.match(r'\s*L([0-9a-f]+): ', line)
            if m2 and 'blx dispatch' in line or (m2 and 'jqt_blx_dispatch' in line):
                addr = int(m2.group(1), 16)
                # 上一行是否是"强制 0x9e2e0"补丁
                trans.setdefault(addr, 'patched' if '_off == 0' in line else 'plain')
    n_plain = 0
    for a_addr, p_addr, rt, disp in sites:
        status = trans.get(a_addr, '?')
        tag = '' if status in ('plain', '?') else '  ← %s' % status
        print('  0x%x: ldr r%d,[?,#0x%x](slot%d) → blx r%d%s' %
              (a_addr, rt, disp, disp // 4, rt, tag))

def vtabswrite_check():
    """检查翻译体里 vtable 写入点是否包了 JQT_ABS_PTR（§124zz-6）。
    搜 src_jni_translated/*.c 里 "MEM(rX+0) = 0x15xxxx" 形式硬编码
    （.data.rel.ro 区 vtable 偏移，未包 ABS = 错，应为绝对地址）。
    用法：python3 tex_pipeline.py vtabswrite"""
    import glob
    bad = []
    for f in sorted(glob.glob('/disk/workspaces/jq/rev64/src_jni_translated/*.c')):
        s = open(f).read()
        for m in re.finditer(r'MEM\((\w+)\+0\)\s*=\s*(0x1[56][0-9a-f]{3})\b', s):
            reg, val = m.group(1), m.group(2)
            ctx = s[max(0, m.start() - 120): m.end()]
            if 'JQT_ABS_PTR' not in ctx:
                line_no = s.count('\n', 0, m.start()) + 1
                bad.append((f.split('/')[-1], line_no, reg, val))
    print('=== vtable 写入点未包 JQT_ABS_PTR（存偏移 = 错）：%d 处 ===' % len(bad))
    for f, ln, reg, val in bad:
        print('  %s:%d MEM(%s+0)=0x%s' % (f, ln, reg, val))
    if not bad:
        print('  （0 处——全部 vtable 写入已包 ABS，正确）')

if __name__ == '__main__':
    cmd = sys.argv[1] if len(sys.argv) > 1 else 'vtable22'
    if cmd == 'vtable22':
        h = find_vtable_dispatch(22)
        print('vtable[22]（纹理更新，disp=0x58）dispatch:', len(h))
        for x in h[:15]:
            print('  0x%x: %s → blx @0x%x' % (x[0], x[2], x[1]))
    elif cmd == 'vtable23':
        h = find_vtable_dispatch(23)
        print('vtable[23]（删除纹理，disp=0x5c）dispatch:', len(h))
        for x in h[:10]:
            print('  0x%x: %s → blx @0x%x' % (x[0], x[2], x[1]))
    elif cmd == 'callers':
        callers(int(sys.argv[2], 0))
    elif cmd == 'closure':
        closure(int(sys.argv[2], 0))
    elif cmd == 'scenechain':
        scenechain(int(sys.argv[2], 0) if len(sys.argv) > 2 else 0x242f4)
    elif cmd == 'vslots':
        a = int(sys.argv[2], 0)
        n = int(sys.argv[3], 0) if len(sys.argv) > 3 else 0x300
        vslots_in(a, a + n)
    elif cmd == 'vslot':
        n = int(sys.argv[2], 0)   # 十进制槽号（0x 前缀则十六进制）
        h = find_vtable_dispatch(n)
        print('vtable[%d]（disp=0x%x）dispatch: %d 处' % (n, n * 4, len(h)))
        for x in h[:25]:
            print('  0x%x: %s → blx @0x%x' % (x[0], x[2], x[1]))
    elif cmd == 'funcs':
        for a in sys.argv[2:]:
            show_func_consts(int(a, 16), int(a, 16) + 0x100)
    elif cmd == 'glcall':
        a = int(sys.argv[2], 16)
        n = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x100
        gl_trace(a, a + n)
    elif cmd == 'plt':
        plt, _ = _plt_map()
        print('PLT 映射 (%d 条):' % len(plt))
        for a in sorted(plt):
            print('  0x%05x → %s' % (a, plt[a]))
    elif cmd == 'texseq':
        # 纹理管线 4 函数的原版 GL 调用序列（核对翻译体用）
        for nm, a, ln in (('F_9e0d4 删除', 0x9e0d4, 0x1c),
                          ('F_9e204 创建', 0x9e204, 0x90),
                          ('F_9e0f0 上传', 0x9e0f0, 0x114),
                          ('F_9e2b4 更新', 0x9e2b4, 0x2e)):
            print('\n########## %s (0x%x) ##########' % (nm, a))
            gl_trace(a, a + ln)
    elif cmd == 'vtable':
        vtable_dump(int(sys.argv[2], 16),
                    int(sys.argv[3], 16) if len(sys.argv) > 3 else 64)
    elif cmd == 'stores':
        find_stores(int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x1c)
    elif cmd == 'texclass':
        texclass(int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x1595c0)
    elif cmd == 'funcat':
        funcat(int(sys.argv[2], 16))
    elif cmd == 'findctor':
        findctor(int(sys.argv[2], 16))
    elif cmd == 'findvtable':
        findvtable(int(sys.argv[2], 16))
    elif cmd == 'undiff':
        und_sym_diff()
    elif cmd == 'vtwrite':
        # 找写 vtable=X 的构造器（搜 .text 里 ldr+add[s] pc [, #imm] 后 str [reg,0]，RPI 解码 == X）
        vtwrite(sys.argv[2])
    elif cmd == 'slot22chain':
        slot22chain()
    elif cmd == 'vtwrites':
        # 扫全 .text 所有 RPI vtable 写点（无需预知目标）
        vtwrites()
    elif cmd == 'vtowners':
        vtowners(int(sys.argv[2], 16))
    elif cmd == 'upchain':
        upchain(int(sys.argv[2], 16),
                int(sys.argv[3], 10) if len(sys.argv) > 3 else 6)
    elif cmd == 'batchlog':
        batchlog(*sys.argv[2:])
    elif cmd == 'rpi':
        rpi_scan()
    elif cmd == 'i2f':
        i2f_sites(fix=('--fix' in sys.argv))
    elif cmd == 'dispatchfix':
        # 列出 .text 里所有 "MEM(vt+disp)" vtable dispatch 点（翻译体 JQT_ABS_PTR 修正清单）
        dispatchfix()
    elif cmd == 'vtabswrite':
        # 检查翻译体里 vtable 写入点是否存 RELATIVE 重定位后的绝对地址（JQT_ABS_PTR 包裹）
        vtabswrite_check()
    else:
        print('用法: tex_pipeline.py {vtable22|vtable23|vslots|scenechain|closure|callers|texclass|findvtable|findctor|funcat|stores|plt|glcall|texseq|undiff|vtwrite <addr>|slot22chain|vtwrites|vtowners <func>|upchain <addr> [depth]|batchlog <addr>...|i2f|dispatchfix|vtabswrite}')
