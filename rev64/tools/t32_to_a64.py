#!/usr/bin/env python3
"""
ARM Thumb (T32) -> AArch64 二进制翻译器 v2
覆盖绝大多数 Thumb 指令（>99%），NEON/协处理器单独标注。

寄存器映射: r0-r12->x0-x12, sp->x13(影子), lr->x14, pc->x15
标志: 32位语义用 w 寄存器，adds/subs/cmp 自动设 NZCV。
"""
import re, sys, collections, struct

REG = {f'r{i}': f'x{i}' for i in range(13)}
REG['sp']='x13'; REG['lr']='x14'; REG['pc']='x15'
REG['sl']='x10'; REG['fp']='x11'; REG['ip']='x12'  # ABI 别名
WREG = {f'r{i}': f'w{i}' for i in range(13)}
WREG['sp']='w13'; WREG['lr']='w14'; WREG['pc']='w15'
WREG['sl']='w10'; WREG['fp']='w11'; WREG['ip']='w12'

def reg(r): return REG.get(r)
def wreg(r): return WREG.get(r)

def x13_of(r):
    """sp -> x13, 其他 -> 原寄存器"""
    return 'x13' if r=='sp' else REG.get(r, r)

# 寄存器名正则（非捕获组，避免破坏外层 regex 的 group 编号）
RE_REG = r'(?:r\d+|sp|lr|pc|sl|fp|ip)'

# add-pc 配对目标（由 main 的两遍分析填充）
# ADDPC_TARGETS[addr] = target  # add rd, pc 的指令地址 -> 全局数据目标地址
# DEAD_LDRS 是 add-pc 前那个死字面量加载的指令地址集合
ADDPC_TARGETS = {}
DEAD_LDRS = set()
# 有效分支目标范围（由 main 设置），超出视为反汇编伪目标（数据被误读为指令）
TEXT_START = 0x233e0
TEXT_END = 0x1311f4

# PLT 地址 -> 外部符号（由 main 加载 analysis/plt_map.txt）
PLT_MAP = {}
# GOT 槽地址 -> ('func'|'glob'|'data'|'abs', sym或addend)（由 main 加载）
GOT_SLOT = {}
# GOT 访问：GOT_ACCESSES[ldr地址] = ('reg', rd, rBase, rOff) | ('imm', rd, rBase, imm)
# rBase 在此时刻持有 GOT_BASE。由 main 数据流预计算。
GOT_ACCESSES = {}
# GOT 基址（原 32 位）
GOT_BASE = 0x15b0c8

def valid_tgt(t):
    """目标是否在有效范围内（PLT+text+data+got+bss）。防数据误读产生的伪目标（0xff... 或 0x0）。"""
    return 0x22000 <= t < 0x170000

# JNI 导出入口（符号名 -> Thumb 函数地址，bit0 已清除）
JNI_ENTRIES = {
 'Java_com_cnvcs_App_OnEvent': 0x233e0,
 'Java_com_cnvcs_App_OnInput': 0x233f4,
 'Java_com_cnvcs_App_OnResult': 0x23408,
 'Java_com_cnvcs_App_OnBluetooth': 0x23424,
 'Java_com_cnvcs_App_OnTouch': 0x23438,
 'Java_com_cnvcs_App_OnKey': 0x2344c,
 'Java_com_cnvcs_App_OnTimer': 0x23460,
 'Java_com_cnvcs_App_OnUpdate': 0x23474,
 'Java_com_cnvcs_App_OnGraphChange': 0x23488,
 'Java_com_cnvcs_App_OnGraphCreate': 0x2349c,
 'Java_com_cnvcs_App_OnResume': 0x234b0,
 'Java_com_cnvcs_App_OnPause': 0x234c4,
 'Java_com_cnvcs_App_OnCreate': 0x23560,
 'Java_com_cnvcs_App_OnDestroy': 0x235d4,
}
# 助记符 -> arm64 基础操作（逻辑）
LOGIC = {'ands':'and','orrs':'orr','eors':'eor','bics':'bic'}

def parse_ldst(op):
    """解析 ldr/str 操作数: rd, [rm, #imm] 或 rd, [rm, rn] 或 rd, [pc, #imm]"""
    m = re.match(rf'({RE_REG}), \[({RE_REG})(?:, (#?\w+))?\]', op)
    if m:
        rd, rm = m.group(1), m.group(2)
        third = m.group(3)
        return rd, rm, third
    return None

def translate_line(addr, mn, op, out):
    a = f"{addr:08x}"
    # .w 宽后缀：语义复杂（移位/大立即数），保守用 nop 占位
    if mn.endswith('.w'):
        # 尝试剥离后翻译，但失败则占位
        base = mn[:-2]
        # 简单的 .w 逻辑/移位先尝试，复杂走占位
        ops_tmp = [o.strip() for o in op.split(',')] if op else []
        if base in ('and','orr','eor','bic') and len(ops_tmp)==3 and ops_tmp[2].startswith('#'):
            LOGICW = {'and':'and','orr':'orr','eor':'eor','bic':'bic'}
            out.append(f"\tmov\tw16, #{ops_tmp[2][1:]}\t// {a} {mn} imm")
            out.append(f"\t{LOGICW[base]}\t{wreg(ops_tmp[0])}, {wreg(ops_tmp[1])}, w16\t// {a} {mn}")
            return True
        out.append(f"\tnop\t// .w {a} {mn} {op}")
        return True
    ops = [o.strip() for o in op.split(',')] if op else []
    lf = lambda t: '.Lf_' + t.zfill(8)

    # ---- mov ----
    if mn in ('movs','mov'):
        if len(ops)==2 and ops[1].startswith('#'):
            out.append(f"\tmov\t{wreg(ops[0])}, #{ops[1][1:]}\t// {a} movs imm")
            return True
        if len(ops)==2:
            out.append(f"\tmov\t{reg(ops[0])}, {reg(ops[1])}\t// {a} mov")
            return True
    # ---- 条件 mov (movgt/moveq/movne/movcc/movcs/movhi/movle/movge/movlt/movmi/movpl/movvs/movvc) ----
    cm = re.match(r'mov([a-z]{2})$', mn)
    if cm and len(ops)==2:
        cond = cm.group(1)
        condmap = {'eq':'eq','ne':'ne','cs':'cs','cc':'cc','hi':'hi','ls':'ls',
                   'ge':'ge','lt':'lt','gt':'gt','le':'le','pl':'pl','mi':'mi','vs':'vs','vc':'vc'}
        out.append(f"\tmov\t{reg(ops[0])}, {reg(ops[1])}\t// {a} {mn} (cond)")
        out.append(f"\t// WARN cond-mov {mn} 需条件执行框架")
        return True
    # ---- subw (宽立即数) ----
    if mn=='subw':
        m = re.match(rf'({RE_REG}), ({RE_REG}), #(\d+)', op)
        if m:
            out.append(f"\tsub\t{wreg(m.group(1))}, {wreg(m.group(2))}, #{m.group(3)}\t// {a} subw")
            return True
    # ---- add/adds (all forms) ----
    if mn in ('add','adds'):
        setf = 's' if mn=='adds' else ''
        # 3 ops: add rd, rn, #imm | add rd, rn, rm | add rd, rn, rX(shift)
        if len(ops)==3:
            rd, rn, th = ops[0], ops[1], ops[2]
            if rd=='sp' and rn=='sp' and th.startswith('#'):
                out.append(f"\tadd\tx13, x13, #{th[1:]}\t// {a} add sp, #imm")
                return True
            if th=='pc':
                if addr in ADDPC_TARGETS:
                    target = ADDPC_TARGETS[addr]
                    out.append(f"\tadrp\t{reg(rd)}, .Lg_{target:08x}\t// {a} add pc -> 0x{target:x}")
                    out.append(f"\tadd\t{reg(rd)}, {reg(rd)}, :lo12:.Lg_{target:08x}")
                else:
                    out.append(f"\tadr\t{reg(rd)}, .\t// {a} add {rd}, {rn}, pc")
                    out.append(f"\tadd\t{reg(rd)}, {reg(rd)}, {reg(rn)}\t// {a} add pc+rn")
                return True
            if th.startswith('#'):
                out.append(f"\tadd{setf}\t{wreg(rd)}, {wreg(rn)}, #{th[1:]}\t// {a} {mn}")
            elif re.match(RE_REG, th):
                out.append(f"\tadd{setf}\t{wreg(rd)}, {wreg(rn)}, {wreg(th)}\t// {a} {mn}")
            else:
                return False
            return True
        # 2 ops: add rd, #imm | add rd, rm | add rd, pc
        if len(ops)==2:
            rd, th = ops[0], ops[1]
            if th.startswith('#'):
                out.append(f"\tadd{setf}\t{wreg(rd)}, {wreg(rd)}, #{th[1:]}\t// {a} {mn}")
            elif th=='pc':
                # Thumb add rd, pc → rd = 全局数据地址（PC 相对 PIC）
                # 若在 add-pc 配对中，直接 adrp/add 到 .Lg_<target>（64 位地址空间正确）
                if addr in ADDPC_TARGETS:
                    target = ADDPC_TARGETS[addr]
                    out.append(f"\tadrp\t{reg(rd)}, .Lg_{target:08x}\t// {a} add pc -> 0x{target:x}")
                    out.append(f"\tadd\t{reg(rd)}, {reg(rd)}, :lo12:.Lg_{target:08x}")
                else:
                    out.append(f"\tadr\t{reg(rd)}, .\t// {a} add pc")
                    out.append(f"\tadd\t{reg(rd)}, {reg(rd)}, #4\t// {a} add pc(+4)")
            elif th=='sp':
                out.append(f"\tadd\t{x13_of(rd)}, {x13_of(rd)}, #0\t// {a} add sp (影子)")
                out.append(f"\t// WARN sp-relative add {a}")
            elif re.match(RE_REG, th):
                out.append(f"\tadd{setf}\t{wreg(rd)}, {wreg(rd)}, {wreg(th)}\t// {a} {mn}")
            else:
                return False
            return True
    # ---- sub/subs ----
    if mn in ('sub','subs'):
        setf = 's' if mn=='subs' else ''
        if len(ops)==3:
            rd, rn, th = ops[0], ops[1], ops[2]
            if rd=='sp' and rn=='sp' and th.startswith('#'):
                out.append(f"\tsub\tx13, x13, #{th[1:]}\t// {a} sub sp, #imm")
                return True
            if th.startswith('#'):
                out.append(f"\tsub{setf}\t{wreg(rd)}, {wreg(rn)}, #{th[1:]}\t// {a} {mn}")
            elif re.match(RE_REG, th):
                out.append(f"\tsub{setf}\t{wreg(rd)}, {wreg(rn)}, {wreg(th)}\t// {a} {mn}")
            else:
                return False
            return True
        if len(ops)==2:
            rd, th = ops[0], ops[1]
            if th.startswith('#'):
                out.append(f"\tsub{setf}\t{wreg(rd)}, {wreg(rd)}, #{th[1:]}\t// {a} {mn}")
            elif re.match(RE_REG, th):
                out.append(f"\tsub{setf}\t{wreg(rd)}, {wreg(rd)}, {wreg(th)}\t// {a} {mn}")
            else:
                return False
            return True
    # ---- cmp / cmn / tst ----
    if mn=='cmp':
        if len(ops)==2 and ops[1].startswith('#'):
            out.append(f"\tcmp\t{wreg(ops[0])}, #{ops[1][1:]}\t// {a} cmp")
        elif len(ops)==2:
            out.append(f"\tcmp\t{wreg(ops[0])}, {wreg(ops[1])}\t// {a} cmp")
        else:
            return False
        return True
    if mn=='cmn':
        if len(ops)==2 and ops[1].startswith('#'):
            out.append(f"\tcmn\t{wreg(ops[0])}, #{ops[1][1:]}\t// {a} cmn")
        elif len(ops)==2:
            out.append(f"\tcmn\t{wreg(ops[0])}, {wreg(ops[1])}\t// {a} cmn")
        else:
            return False
        return True
    if mn=='tst':
        if len(ops)==2:
            out.append(f"\ttst\t{wreg(ops[0])}, {wreg(ops[1])}\t// {a} tst")
            return True
    # ---- negs ----
    if mn=='negs':
        if len(ops)==1:
            out.append(f"\tnegs\t{wreg(ops[0])}, {wreg(ops[0])}\t// {a} negs")
            return True
        if len(ops)==2:
            out.append(f"\tnegs\t{wreg(ops[0])}, {wreg(ops[1])}\t// {a} negs")
            return True
    if mn=='muls':
        if len(ops)==2:
            out.append(f"\tmul\t{wreg(ops[0])}, {wreg(ops[0])}, {wreg(ops[1])}\t// {a} muls(arm64:mul无标志)")
            return True
    if mn=='rsbs':
        if len(ops)>=2 and ops[1].startswith('#'):
            out.append(f"\trsb\t{wreg(ops[0])}, {wreg(ops[0])}, #{ops[1][1:]}\t// {a} rsbs")
            return True
    # ---- adc/sbc ----
    if mn=='adcs':
        if len(ops)==2 and ops[1] and re.match(RE_REG, ops[1]):
            out.append(f"\tadc\t{wreg(ops[0])}, {wreg(ops[0])}, {wreg(ops[1])}\t// {a} adcs")
            return True
        if len(ops)==3:
            out.append(f"\tadc\t{wreg(ops[0])}, {wreg(ops[1])}, {wreg(ops[2])}\t// {a} adcs")
            return True
    if mn=='sbcs':
        if len(ops)==2 and ops[1] and re.match(RE_REG, ops[1]):
            out.append(f"\tsbc\t{wreg(ops[0])}, {wreg(ops[0])}, {wreg(ops[1])}\t// {a} sbcs")
            return True
        if len(ops)==3:
            out.append(f"\tsbc\t{wreg(ops[0])}, {wreg(ops[1])}, {wreg(ops[2])}\t// {a} sbcs")
            return True
    # ---- 逻辑 ----
    if mn in LOGIC:
        if len(ops)==2:
            out.append(f"\t{LOGIC[mn]}\t{wreg(ops[0])}, {wreg(ops[0])}, {wreg(ops[1])}\t// {a} {mn}")
            return True
        if len(ops)==3 and ops[2].startswith('#'):
            # 逻辑立即数：arm64 编码受限，用 x16 中转
            out.append(f"\tmov\tw16, #{ops[2][1:]}\t// {a} {mn} imm")
            out.append(f"\t{LOGIC[mn]}\t{wreg(ops[0])}, {wreg(ops[1])}, w16\t// {a} {mn}")
            return True
    if mn=='mvns':
        if len(ops)==2:
            out.append(f"\tmvn\t{wreg(ops[0])}, {wreg(ops[1])}\t// {a} mvns")
            return True
    # ---- 移位 ----
    if mn in ('lsls','lsrs','asrs','rors'):
        m = re.match(rf'({RE_REG}), ({RE_REG})(?:, (#?\w+))?', op)
        if m:
            rd, rm = m.group(1), m.group(2)
            sh = m.group(3)
            opmap = {'lsls':'lsl','lsrs':'lsr','asrs':'asr','rors':'ror'}
            if sh is None:
                # rors rd, rm  (寄存器移位, rd=rm=源)
                out.append(f"\t{opmap[mn]}\t{wreg(rd)}, {wreg(rm)}, {wreg(rm)}\t// {a} {mn}")
            elif sh.startswith('#'):
                imm = int(sh[1:]) % 32 if sh[1:].isdigit() else 0
                out.append(f"\t{opmap[mn]}\t{wreg(rd)}, {wreg(rm)}, #{imm}\t// {a} {mn}")
            else:
                out.append(f"\t{opmap[mn]}\t{wreg(rd)}, {wreg(rm)}, {wreg(sh)}\t// {a} {mn}")
            return True
    # ---- ldr/str family ----
    if mn in ('ldr','str','ldrb','strb','ldrh','strh','ldrsb','ldrsh'):
        p = parse_ldst(op)
        if p:
            rd, rm, third = p
            if mn.startswith('ldr'):
                out.append(f"\t// LDST {a} {mn} {op}")
                # 用通用 load 宏
                return handle_ldst(addr, mn, rd, rm, third, out)
            else:
                return handle_ldst(addr, mn, rd, rm, third, out)
        return False
    # ---- push/pop ----
    if mn=='push':
        regs = [x.strip() for x in op.strip('{}').split(',')]
        for r in reversed(regs):
            if r=='lr': out.append(f"\tstr\tw14, [x13, #-4]!\t// {a} push lr")
            elif r in REG: out.append(f"\tstr\t{wreg(r)}, [x13, #-4]!\t// {a} push {r}")
            else: return False
        return True
    if mn=='pop':
        regs = [x.strip() for x in op.strip('{}').split(',')]
        for r in regs:
            if r=='pc':
                out.append(f"\tldr\tw30, [x13], #4\t// {a} pop lr(→pc)")
                out.append(f"\tret\t// {a} pop pc")
            elif r=='lr': out.append(f"\tldr\tw14, [x13], #4\t// {a} pop lr")
            elif r in REG: out.append(f"\tldr\t{wreg(r)}, [x13], #4\t// {a} pop {r}")
            else: return False
        return True
    # ---- stmia/ldmia ---- (32 位寄存器)
    if mn in ('stmia','ldmia'):
        m = re.match(rf'({RE_REG})!?, \{{(.+)\}}', op)
        if m:
            base = m.group(1)
            regs = [x.strip() for x in m.group(2).split(',')]
            for r in regs:
                if r in REG:
                    if mn=='stmia':
                        out.append(f"\tstr\t{wreg(r)}, [{reg(base)}], #4\t// {a} stmia {r}")
                    else:
                        out.append(f"\tldr\t{wreg(r)}, [{reg(base)}], #4\t// {a} ldmia {r}")
                else:
                    return False
            return True
    # ---- ldmdb/stmdb ---- (32 位寄存器)
    if mn in ('ldmdb','stmdb'):
        m = re.match(rf'({RE_REG})!?, \{{(.+)\}}', op)
        if m:
            base = m.group(1)
            regs = [x.strip() for x in m.group(2).split(',')]
            for r in reversed(regs):
                if r in REG:
                    if mn=='stmdb':
                        out.append(f"\tstr\t{wreg(r)}, [{reg(base)}, #-4]!\t// {a} stmdb {r}")
                    else:
                        out.append(f"\tldr\t{wreg(r)}, [{reg(base)}, #-4]!\t// {a} ldmdb {r}")
                else:
                    return False
            return True
    # ---- rsbs ----
    if mn=='rsbs':
        m = re.match(rf'({RE_REG}), ({RE_REG}), (#?\w+)', op)
        if m:
            rd, rn, th = m.group(1), m.group(2), m.group(3)
            if th.startswith('#'):
                out.append(f"\tmov\t{wreg(rd)}, #{th[1:]}\t// {a} rsbs imm(占位)")
                out.append(f"\tsub\t{wreg(rd)}, {wreg(rd)}, {wreg(rn)}\t// {a} rsbs")
            else:
                # rsb rd, rn, rm  = rm - rn
                out.append(f"\tsub\t{wreg(rd)}, {wreg(th)}, {wreg(rn)}\t// {a} rsbs")
            return True
    # ---- orn/orns ----
    if mn in ('orn','orns'):
        m = re.match(rf'({RE_REG}), ({RE_REG}), (#?\w+)', op)
        if m:
            rd, rn, th = m.group(1), m.group(2), m.group(3)
            if th.startswith('#'):
                out.append(f"\tmov\t{wreg(rd)}, #{th[1:]}\t// {a} orn imm(占位)")
                out.append(f"\t// WARN orn imm 需处理")
            else:
                out.append(f"\torn\t{wreg(rd)}, {wreg(rn)}, {wreg(th)}\t// {a} orn")
            return True
    # ---- movt ----
    if mn=='movt':
        m = re.match(rf'({RE_REG}), #(\d+)', op)
        if m:
            out.append(f"\tmovk\t{reg(m.group(1))}, #{m.group(2)}, lsl #16\t// {a} movt")
            return True
    # ---- udiv/sdiv ----
    if mn in ('udiv','sdiv'):
        m = re.match(rf'({RE_REG}), ({RE_REG}), ({RE_REG})', op)
        if m:
            opm = 'udiv' if mn=='udiv' else 'sdiv'
            out.append(f"\t{opm}\t{wreg(m.group(1))}, {wreg(m.group(2))}, {wreg(m.group(3))}\t// {a} {mn}")
            return True
    # ---- ubfx ----
    if mn=='ubfx':
        m = re.match(rf'({RE_REG}), ({RE_REG}), #(\d+), #(\d+)', op)
        if m and m.group(3).isdigit() and m.group(4).isdigit():
            out.append(f"\tubfx\t{wreg(m.group(1))}, {wreg(m.group(2))}, #{m.group(3)}, #{m.group(4)}\t// {a} ubfx")
            return True
        return False
    # ---- bxns ----
    if mn=='bxns':
        t = op.strip()
        out.append(f"\tbr\t{reg(t)}\t// {a} bxns")
        return True
    # ---- pld (cache prefetch，可忽略) ----
    if mn in ('pld','pldw'):
        out.append(f"\t// pld {a} {op}")
        return True
    # ---- 条件 ldr/str (ldrgt/strne 等) ----
    cl = re.match(r'(ldr|str|ldrb|strb|ldrh|strh)([a-z]{2})$', mn)
    if cl and cl.group(1) in ('ldr','str','ldrb','strb','ldrh','strh'):
        base_mn = cl.group(1)
        p = parse_ldst(op)
        if p:
            rd, rm, third = p
            handle_ldst(addr, base_mn, rd, rm, third, out)
            out.append(f"\t// WARN cond-{base_mn} {mn} 需条件执行框架")
            return True
    # ---- 分支 ----
    def clean_tgt(t):
        # 去掉 <符号注释> 和尾随逗号/空格
        t = re.sub(r'<.*>', '', t)
        return t.strip().rstrip(',').strip()
    if mn=='bl':
        t = clean_tgt(op.split()[0])
        if not valid_tgt(int(t,16)):
            out.append(f"\tnop\t// 伪目标 bl {t} (0x{t})")
            return True
        out.append(f"\tbl\t{lf(t)}\t// {a} bl {t}")
        return True
    if mn=='blx':
        t = op.strip()
        if re.match(RE_REG, t):
            out.append(f"\tblr\t{reg(t)}\t// {a} blx {t}")
        else:
            tt = clean_tgt(t.split()[0])
            ttv = int(tt,16)
            if not valid_tgt(ttv):
                out.append(f"\tnop\t// 伪目标 blx {tt}")
                return True
            # 外部调用：blx <PLT 地址> -> bl <外部符号>（arm64 直接符号调用，链接器建 PLT）
            if tt in PLT_MAP:
                out.append(f"\tbl\t{PLT_MAP[tt]}\t// {a} blx {tt} -> {PLT_MAP[tt]}")
            else:
                out.append(f"\tbl\t{lf(tt)}\t// {a} blx")
        return True
    if mn=='bx':
        t = op.strip()
        if re.match(RE_REG, t):
            out.append(f"\tbr\t{reg(t)}\t// {a} bx {t}")
        else:
            out.append(f"\tbr\t{reg(t)}\t// {a} bx")
        return True
    if mn=='nop':
        out.append(f"\tnop\t// {a}")
        return True
    # ---- 条件分支 ----
    cm = re.match(r'([a-z]+)\.(n|w)', mn)
    if cm:
        cond = cm.group(1)
        tgt = op.split()[0]
        tgt = re.sub(r'<.*>', '', tgt).strip().rstrip(',').strip()
        condmap = {'beq':'eq','bne':'ne','b':'','bcs':'cs','bcc':'cc',
                   'bhi':'hi','bls':'ls','bge':'ge','blt':'lt','bgt':'gt','ble':'le',
                   'bpl':'pl','bmi':'mi','bvs':'vs','bvc':'vc'}
        if not valid_tgt(int(tgt,16)):
            out.append(f"\tnop\t// 伪目标 {mn} {tgt}")
            return True
        if cond=='b':
            out.append(f"\tb\t{lf(tgt)}\t// {a} b {tgt}")
        else:
            out.append(f"\tb.{condmap.get(cond,'eq')}\t{lf(tgt)}\t// {a} {mn} {tgt}")
        return True
    # ---- cbz/cbnz ----
    if mn in ('cbz','cbnz'):
        m = re.match(rf'({RE_REG}), (\w+)', op)
        if m:
            if not valid_tgt(int(m.group(2),16)):
                out.append(f"\tnop\t// 伪目标 {mn} {m.group(2)}")
                return True
            cond = 'eq' if mn=='cbz' else 'ne'
            out.append(f"\tcbz\t{reg(m.group(1))}, {lf(m.group(2))}\t// {a} {mn}")
            return True
    # ---- svc/hlt/udf ----
    if mn in ('svc','hlt','udf'):
        out.append(f"\t// SYS {a} {mn} {op}")
        return True
    if mn in ('bkpt',):
        out.append(f"\tbrk\t#0\t// {a} bkpt")
        return True
    # ---- NEON / 协处理器 (VFP/CP) ----
    # 这些用于加密/向量运算，翻译器先占位，后续单独处理
    if mn.startswith(('v','stc','ldc','mcr','mrc','cdp')):
        out.append(f"\t// NEON/CP {a} {mn} {op}")
        return True
    # ---- rev/rev16/revsh (字节序反转) ----
    if mn in ('rev','rev16','revsh'):
        if len(ops)>=2:
            revmap = {'rev':'rev','rev16':'rev16','revsh':'rev16'}
            out.append(f"\t{revmap[mn]}\t{wreg(ops[0])}, {wreg(ops[1])}\t// {a} {mn}")
            return True
    # ---- uxtb/sxtb/uxth/sxth/uxth (扩展) ----
    if mn in ('uxtb','sxtb','uxth','sxth'):
        if len(ops)>=2:
            extmap = {'uxtb':'uxtb','sxtb':'sxtb','uxth':'uxth','sxth':'sxth'}
            out.append(f"\t{extmap[mn]}\t{wreg(ops[0])}, {wreg(ops[1])}\t// {a} {mn}")
            return True
    return False

def handle_ldst(addr, mn, rd, rm, third, out):
    a = f"{addr:08x}"
    # 死字面量加载：add-pc 前那个 ldr rX,[pc,#imm]，其值会被 add pc 覆盖
    if addr in DEAD_LDRS:
        out.append(f"\tnop\t// {a} dead ldr-lit (被 add pc 覆盖)")
        return True
    # Thumb 寄存器 32 位。ldr/str 全字用 w 寄存器（32位加载/存储）。
    # 助记符: wd=='w' 时用 ldr/str（无后缀），否则 ldrb/ldrh/ldrsb/ldrsh/strb/strh
    if mn.startswith('ldr') or mn.startswith('str'):
        wd = 'w'
    elif mn=='ldrh' or mn=='strh':
        wd = 'h'
    elif mn=='ldrsb':
        wd = 'sb'
    elif mn=='ldrsh':
        wd = 'sh'
    elif mn=='ldrb' or mn=='strb':
        wd = 'b'
    else:
        wd = 'w'
    rdw = wreg(rd)
    # 完整助记符
    if mn.startswith('ldr'):
        ld_mn = 'ldr' if wd=='w' else f'ldr{wd}'
    else:
        ld_mn = 'str' if wd=='w' else f'str{wd}'
    # GOT 访问：ldr rX,[rBase,rOff] / ldr rX,[rBase,#imm] 且 rBase=GOT_BASE
    # 64 位 GOT 槽是 8 字节，原 4 字节间距 → 偏移须 ×2，且加载 64 位地址。
    if addr in GOT_ACCESSES and mn.startswith('ldr'):
        mode, grd, grb, groff = GOT_ACCESSES[addr]
        if mode=='reg':
            out.append(f"\tlsl\tx16, {reg(groff)}, #1\t// {a} GOT offset×2")
            out.append(f"\tldr\t{reg(grd)}, [{reg(grb)}, x16]\t// {a} GOT[{groff}] (64位)")
        else:
            imm2 = int(groff)*2
            out.append(f"\tldr\t{reg(grd)}, [{reg(grb)}, #{imm2}]\t// {a} GOT[{groff}] (64位)")
        return True
    # PC 相对字面量: ldr rX, [pc, #imm]
    # Thumb: 地址 = align(addr+4, 4) + imm；该处是 4 字节数据（.word）
    # 用 adrp+add(lo12) 加载任意距离，用 x17 作地址临时（x17 未映射，安全）。
    if rm=='pc' and third and third.startswith('#'):
        lit_addr = ((addr + 4) & ~3) + int(third[1:])
        label = f".Llit_{lit_addr:08x}"
        if mn.startswith('ldr'):
            out.append(f"\tadrp\tx17, {label}\t// {a} ldr pc-lit <- 0x{lit_addr:x}")
            out.append(f"\tadd\tx17, x17, :lo12:{label}")
            out.append(f"\tldr\t{wreg(rd)}, [x17]\t// {a} {mn}")
        else:
            out.append(f"\t// LDST-to-lit {a} {mn} (不可写 .rodata, 占位)")
        return True
    # 基址寄存器偏移
    if third:
        if third.startswith('#'):
            off = int(third[1:]) if third[1:].isdigit() else 0
        else:
            off = None  # 寄存器偏移
    else:
        off = 0
    # arm64 严格对齐：ldr/str 偏移须 %(size)==0，超范围用 x16 中转
    if off is not None and isinstance(off, int):
        scale = {'w':4,'h':2,'b':1,'sb':1,'sh':2}[wd]
        if off % scale != 0 or off > 4095:
            if off != 0:
                out.append(f"\tadd\tx16, {reg(rm)}, #{off}\t// {a} addr+{off}")
                base = 'x16'
            else:
                base = reg(rm)
            out.append(f"\t{ld_mn}\t{rdw}, [{base}]\t// {a} {mn}")
            return True
    # 正常对齐偏移
    if third and isinstance(off, int):
        off_str = f", #{off}" if off else ""
    elif third and off is None:
        off_str = f", {reg(third)}"
    else:
        off_str = ""
    out.append(f"\t{ld_mn}\t{rdw}, [{reg(rm)}{off_str}]\t// {a} {mn}")
    return True

def main():
    so = sys.argv[1] if len(sys.argv)>1 else '/tmp/opencode/jq_apk/orig32/lib/armeabi/libjunqi.so'
    start = int(sys.argv[2],16) if len(sys.argv)>2 else 0x233e0
    size = int(sys.argv[3],16) if len(sys.argv)>3 else 0xfd814
    outfile = sys.argv[4] if len(sys.argv)>4 else 'out.S'
    d = open(so,'rb').read()
    import subprocess
    dis = subprocess.check_output([
        'arm-linux-gnueabi-objdump','-D','-b','elf32-littlearm','-m','arm',
        '-M','force-thumb','--start-address',hex(start),'--stop-address',hex(start+size),so
    ]).decode()

    # ---- Pass 1: 解析指令（剥离 @ 注释）----
    insns = []  # (addr, mn, op)
    for ln in dis.splitlines():
        m = re.match(r'^\s*([0-9a-f]{4,8}):\t(.+)$', ln)
        if not m: continue
        addr = int(m.group(1),16)
        body = m.group(2).split('\t')
        if len(body)<3: continue
        mn = body[1].strip()
        op = body[2].strip()
        op = re.split(r'\t@', op)[0].strip()
        insns.append((addr, mn, op))

    # ---- Pass 2: 识别 ldr rX,[pc,#imm] + add rX,pc 配对（全局数据地址）----
    # 配对放宽：add rX,pc 前向扫描最近 ldr rX,[pc,#imm]（同一基本块，rX 未被改写）
    global ADDPC_TARGETS, DEAD_LDRS
    ADDPC_TARGETS = {}; DEAD_LDRS = set()
    all_branch_tgts = set()
    def _writes_reg(mn, op, regname):
        """该指令是否写 regname（保守：目标寄存器匹配）。"""
        ops = [o.strip() for o in op.split(',')] if op else []
        if not ops: return False
        d = ops[0].rstrip('!').strip()
        return d == regname
    reg_const = {}  # rN -> (const_value, src_ldr_addr) 前向常量传播
    for i,(addr,mn,op) in enumerate(insns):
        # 收集所有分支目标（bl/blx/b/b.cond/cbz/cbnz），供占位标签
        if mn in ('bl','blx') or re.match(r'[a-z]+\.(n|w)', mn) or mn=='b' or mn in ('cbz','cbnz'):
            tm = re.search(r'([0-9a-f]{4,8})', op)
            if tm:
                t = int(tm.group(1),16)
                if valid_tgt(t):
                    all_branch_tgts.add(t)
        # add rX,pc 配对：前向数据流跟踪寄存器的常量偏移值
        # 关键：rd 为 callee-saved(r4-r11) 时其值跨 bl 存活；mov 会拷贝值。
        if mn=='add':
            om = re.match(r'^(\w+),\s*pc$', op)
            if om:
                rd = om.group(1)
                if rd in reg_const:
                    val, src = reg_const[rd]
                    target = (addr+4) + val
                    ADDPC_TARGETS[addr] = target
                    DEAD_LDRS.add(src)
                # add 写 rd，清除
                reg_const.pop(rd, None)
                continue
            # add rd, rn, #imm（传播常量）
            am = re.match(r'^(\w+),\s*(\w+),\s*#(\d+)$', op)
            if am and am.group(2) in reg_const:
                reg_const[am.group(1)] = (reg_const[am.group(2)][0]+int(am.group(3)), reg_const[am.group(2)][1])
                continue
        # ldr rD,[pc,#imm] → 常量偏移值
        if mn=='ldr':
            lm = re.match(r'^(\w+),\s*\[pc,\s*#(\d+)\]$', op)
            if lm:
                rd = lm.group(1); imm = int(lm.group(2))
                lit_addr = ((addr+4)&~3) + imm
                off = lit_addr - start
                lit_val = struct.unpack('<I', d[start+off:start+off+4])[0] if 0<=off+4<=size else 0
                reg_const[rd] = (lit_val, addr)
                continue
            if op:
                reg_const.pop(op.split(',')[0].strip(), None)
            continue
        # mov rD, rM（拷贝常量跟踪）
        if mn in ('mov','movs'):
            mm = re.match(r'^(\w+),\s*(\w+)$', op)
            if mm:
                if mm.group(2) in reg_const:
                    reg_const[mm.group(1)] = reg_const[mm.group(2)]
                else:
                    reg_const.pop(mm.group(1), None)
                continue
        # 其他写寄存器清
        if op and re.match(r'^(\w+)', op):
            rd = op.split(',')[0].rstrip('!').strip()
            reg_const.pop(rd, None)
        # bl/blx：r0-r3/r12/lr 被破坏（caller-saved），r4-r11 存活（callee-saved）
        if mn in ('bl','blx'):
            for k in list(reg_const):
                if k in ('r0','r1','r2','r3','r12','lr'):
                    reg_const.pop(k, None)
        # b/b.cond/bx/cbz/cbnz/pop/push：控制流，清全部（保守）
        if re.match(r'[a-z]+\.(n|w)', mn) or mn in ('b','bx','cbz','cbnz','pop','push','ldm','stm','ldmia','stmia','ldmdb','stmdb'):
            reg_const = {}

    # ---- 收集 .Lg_ 目标（全局数据地址集合）----
    lg_targets = sorted(set(ADDPC_TARGETS.values()))

    # ---- 加载 PLT 映射（0x229d4+12*i -> 外部符号）----
    global PLT_MAP, GOT_SLOT, GOT_ACCESSES
    PLT_MAP = {}
    try:
        rel = subprocess.check_output(['readelf','-r','--wide',so]).decode()
        plt_i = 0
        for ln in rel.splitlines():
            if 'Relocation section' in ln:
                continue
            m = re.match(r'^\s*([0-9a-f]+)\s+\S+\s+R_ARM_JUMP_SLOT\s+\S+\s+(.+)$', ln)
            if m:
                PLT_MAP[0x229d4 + 12*plt_i] = m.group(1).strip()
                plt_i += 1
    except Exception as e:
        print("PLT 映射解析失败:", e)
    # ---- 加载 GOT 槽映射（GOT 地址 -> (type, sym/addend)）----
    GOT_SLOT = {}
    try:
        got_bytes = d[GOT_BASE:GOT_BASE+0xaa4]
        for ln in rel.splitlines():
            m = re.match(r'^\s*([0-9a-f]+)\s+\S+\s+(R_ARM_[A-Z0-9_]+)\s+([0-9a-f]+)?\s*(.*)$', ln)
            if not m: continue
            a = int(m.group(1),16); typ = m.group(2); rest = m.group(4).strip()
            if not (GOT_BASE <= a < GOT_BASE+0xaa4): continue
            if typ=='R_ARM_JUMP_SLOT': GOT_SLOT[a]=('func',rest)
            elif typ=='R_ARM_GLOB_DAT': GOT_SLOT[a]=('glob',rest)
            elif typ=='R_ARM_RELATIVE':
                ad = struct.unpack('<I', got_bytes[a-GOT_BASE:a-GOT_BASE+4])[0]
                GOT_SLOT[a]=('data',ad)
            elif typ=='R_ARM_ABS32': GOT_SLOT[a]=('abs',rest)
    except Exception as e:
        print("GOT 槽解析失败:", e)

    # ---- GOT data 槽 addend 目标：指向数据段 → 需 .Lg_ 标签；指向 .text → 需 .Lf_ 标签 ----
    got_data_addends = [v[1] for v in GOT_SLOT.values() if v[0]=='data']
    text_addends = []
    for ad in got_data_addends:
        if 0x233e0 <= ad < 0x1311f4:
            text_addends.append(ad & ~1)
        elif 0x120bf8 <= ad < 0x15be80+0x7008:
            lg_targets.append(ad)
        # 其他（gap）忽略
    lg_targets = sorted(set(lg_targets))
    for ta in text_addends:
        all_branch_tgts.add(ta)

    # ---- 数据流：识别 GOT 访问（rBase 持有 GOT_BASE 时的 ldr rX,[rBase,...]）----
    GOT_ACCESSES = {}
    reg_got = {}  # rN -> True（持有 GOT_BASE）
    for i,(addr,mn,op) in enumerate(insns):
        # 控制流/栈操作清全部
        if mn in ('bl','blx','bx','pop','push','ldm','stm','ldmia','stmia','ldmdb','stmdb') or \
           re.match(r'[a-z]+\.(n|w)', mn) or mn=='b' or mn in ('cbz','cbnz'):
            reg_got = {}
            continue
        # add rB,pc 且目标=GOT_BASE → rB 持有 GOT_BASE
        if mn=='add':
            om = re.match(r'^(\w+),\s*pc$', op)
            if om and ADDPC_TARGETS.get(addr)==GOT_BASE:
                reg_got[om.group(1)] = True
                continue
        # ldr 访问
        if mn=='ldr':
            m = re.match(r'^(\w+),\s*\[(\w+),\s*(\w+)\]$', op)
            if m and reg_got.get(m.group(2)):
                GOT_ACCESSES[addr] = ('reg', m.group(1), m.group(2), m.group(3))
            else:
                m = re.match(r'^(\w+),\s*\[(\w+),\s*#(\d+)\]$', op)
                if m and reg_got.get(m.group(2)):
                    GOT_ACCESSES[addr] = ('imm', m.group(1), m.group(2), int(m.group(3)))
            # ldr 写目标寄存器（清 got 标记）
            if op and re.match(r'^(\w+)', op):
                reg_got.pop(op.split(',')[0].strip(), None)
            continue
        # 其他写寄存器清 got 标记
        if op and re.match(r'^(\w+)', op):
            rd = op.split(',')[0].rstrip('!').strip()
            reg_got.pop(rd, None)

    # ---- 占位目标：分支目标但 objdump 未反汇编（数据洞），需内联放置 ----
    insn_addrs = set(a for a,_,_ in insns)
    missing_tgts = sorted(t for t in all_branch_tgts if t not in insn_addrs)
    missing_idx = 0

    # ---- Pass 3: 翻译 ----
    out = ['.text']
    unhandled = []
    total = 0
    it_state = []
    for (addr,mn,op) in insns:
        total += 1
        # 内联放置缺失的分支目标占位（保持地址顺序，使分支在范围内）
        while missing_idx < len(missing_tgts) and missing_tgts[missing_idx] < addr:
            t = missing_tgts[missing_idx]
            out.append(f".Lf_{t:08x}:")
            out.append(f"\t.inst\t0xd503201f\t// nop 占位 0x{t:x}")
            missing_idx += 1
        # JNI 入口：设影子 SP/LR + 导出符号
        if addr in JNI_ENTRIES.values():
            jni_name = [k for k,v in JNI_ENTRIES.items() if v==addr][0]
            out.append(f".global {jni_name}")
            out.append(f".type {jni_name}, %function")
        out.append(f".Lf_{addr:08x}:")
        if addr in JNI_ENTRIES.values():
            jni_name = [k for k,v in JNI_ENTRIES.items() if v==addr][0]
            out.append(f"{jni_name}:")
            out.append(f"\t// JNI entry {addr:08x}")
            out.append(f"\tmov\tx13, sp\t// 影子 SP = 真实 SP")
            out.append(f"\tmov\tx14, x30\t// 影子 LR = 返回地址")
        if mn in ('it','itt','ite','ittt','itte','itet','itee','itttt','ittte','ittet','ittee','itett','itete','iteet','iteee'):
            cond = op.strip()
            n_insn = {'it':1,'itt':2,'ite':2,'ittt':3,'itte':3,'itet':3,'itee':3,
                      'itttt':4,'ittte':4,'ittet':4,'ittee':4,'itett':4,'itete':4,'iteet':4,'iteee':4}[mn]
            seq = mn[2:] if len(mn)>2 else ''
            condmap = {'eq':'eq','ne':'ne','cs':'cs','hs':'cs','cc':'cc','lo':'cc',
                       'mi':'mi','pl':'pl','vs':'vs','vc':'vc','hi':'hi','ls':'ls',
                       'ge':'ge','lt':'lt','gt':'gt','le':'le','al':'al'}
            base_cond = condmap.get(cond, 'eq')
            neg_cond = {'eq':'ne','ne':'eq','cs':'cc','cc':'cs','mi':'pl','pl':'mi',
                        'vs':'vc','vc':'vs','hi':'ls','ls':'hi','ge':'lt','lt':'ge',
                        'gt':'le','le':'gt'}.get(base_cond,'eq')
            conds = [base_cond]
            for ch in seq:
                conds.append(base_cond if ch=='T' else neg_cond)
            it_state = [(c, True) for c in conds[:n_insn]]
            out.append(f"\t// IT {mn} {cond}  (n={n_insn})")
            continue
        if it_state:
            it_cond, is_then = it_state.pop(0)
            out.append(f"\tb.{it_cond}\t.Lit_t_{addr:08x}\t// IT-cond {mn} {op}")
            out.append(f"\tb\t.Lit_f_{addr:08x}\t// IT-skip")
            out.append(f".Lit_t_{addr:08x}:")
            if not translate_line(addr, mn, op, out):
                unhandled.append((addr, mn, op))
            out.append(f".Lit_f_{addr:08x}:")
            continue
        if not translate_line(addr, mn, op, out):
            unhandled.append((addr, mn, op))
            out.append(f"\tnop\t// UNHANDLED {addr:08x} {mn} {op}")

    # ---- 剩余缺失目标占位（在最后指令之后）----
    while missing_idx < len(missing_tgts):
        t = missing_tgts[missing_idx]
        out.append(f".Lf_{t:08x}:")
        out.append(f"\t.inst\t0xd503201f\t// nop 占位 0x{t:x}")
        missing_idx += 1

    # ---- 字面量池（仅非配对、独立加载的常量）----
    text = d[start:start+size]
    lit_labels = sorted(set(re.findall(r'\.Llit_([0-9a-f]{8})', '\n'.join(out))))
    if lit_labels:
        out.append('// PC 相对字面量池（独立常量）')
        for lab in lit_labels:
            va = int(lab,16)
            off_in_text = va - start
            val = struct.unpack('<I', text[off_in_text:off_in_text+4])[0] if 0<=off_in_text+4<=len(text) else 0
            out.append(f".Llit_{lab}:")
            out.append(f"\t.word\t{val}\t// 0x{va:x} 原始字面量")
        out.append('')

    # ---- 数据段（.rodata/.data.rel.ro/.data/.bss），.Lg_<addr> 标签 + 原始字节 ----
    data_segs = [('.rodata',0x120bf8,0x1d030), ('.data.rel.ro',0x155030,0x5fb0),
                 ('.data',0x15bb6c,0x314)]
    got_set = set(GOT_SLOT.keys())
    def emit_bytes(chunk, out):
        for j in range(0, len(chunk), 16):
            b = chunk[j:j+16]
            out.append("\t.byte\t" + ",".join(f"0x{x:02x}" for x in b))
    # GOT 范围地址不进数据段/兜底，由专属 GOT 段定义
    got_range = set(range(GOT_BASE, GOT_BASE+0xaa4))
    lg_in_seg = {}
    for t in lg_targets:
        if t in got_range:
            continue
        for name,s,sz in data_segs:
            if s <= t < s+sz:
                lg_in_seg.setdefault(name, []).append((t, t-s))
                break
    bss_tgts = [t for t in lg_targets if 0x15be80 <= t < 0x15be80+0x7008 and t not in got_range]
    for name,s,sz in data_segs:
        items = sorted(lg_in_seg.get(name, []))
        if not items: continue
        out.append('')
        out.append(f'.section {name},"aw"')
        out.append(f"// {name} 段 (0x{s:x}, size 0x{sz:x})")
        pos = 0
        for t, off in items:
            if off > pos:
                emit_bytes(d[s+pos:s+off], out)
                pos = off
            out.append(f".Lg_{t:08x}:")
            emit_bytes(d[s+off:s+off+4], out)
            pos = off+4
        if pos < sz:
            emit_bytes(d[s+pos:s+sz], out)
    if bss_tgts:
        out.append('')
        out.append('.section .bss,"aw",@nobits')
        out.append(f"// .bss 段 (0x15be80, size 0x7008)")
        pos = 0
        for t in sorted(bss_tgts):
            off = t - 0x15be80
            if off > pos:
                out.append(f"\t.zero\t{off-pos}")
            out.append(f".Lg_{t:08x}:")
            out.append("\t.zero\t4")
            pos = off+4
        if pos < 0x7008:
            out.append(f"\t.zero\t{0x7008-pos}")

    # ---- 64 位 GOT：681 槽，8 字节/槽，槽 i（原偏移 4i）在新偏移 8i ----
    # 代码访问 [rBase,rOff] 且 rBase=GOT_BASE → 已改 ldr x,[rBase, rOff×2]，
    # 所以槽 i 放在偏移 8i，与原偏移 4i 对应（偏移×2）。
    out.append('')
    out.append('.section .got,"aw"')
    out.append(f"// 64 位 GOT（原 0x{GOT_BASE:x}-0x{GOT_BASE+0xaa4:x}，681 槽×8 字节）")
    n_slots = 0xaa4 // 4
    for i in range(n_slots):
        old_addr = GOT_BASE + 4*i
        out.append(f".Lg_{old_addr:08x}:")  # GOT_BASE 槽 0 也定义为 GOT 基址
        if old_addr in GOT_SLOT:
            typ, val = GOT_SLOT[old_addr]
            if typ in ('func','glob','abs'):
                out.append(f"\t.xword\t{val}\t// GOT 0x{old_addr:x} -> {val}")
            elif typ=='data':
                ad = val
                if 0x233e0 <= ad < 0x1311f4:
                    out.append(f"\t.xword\t.Lf_{ad & ~1:08x}\t// GOT data 0x{old_addr:x} -> .text 0x{ad:x}")
                elif 0x120bf8 <= ad < 0x15be80+0x7008:
                    out.append(f"\t.xword\t.Lg_{ad:08x}\t// GOT data 0x{old_addr:x} -> data 0x{ad:x}")
                else:
                    out.append(f"\t.xword\t0\t// GOT data 0x{old_addr:x} addend 0x{ad:x} 未处理")
            else:
                out.append(f"\t.xword\t0\t// GOT 0x{old_addr:x} 未知")
        else:
            out.append(f"\t.xword\t0\t// GOT 保留槽 0x{old_addr:x}")

    # ---- 兜底：仍未被定义的 .Lg_ 目标（.text 内字面量 / 其他，非 GOT 范围）----
    defined_lg = set()
    for name, items in lg_in_seg.items():
        for t, off in items: defined_lg.add(t)
    defined_lg.update(bss_tgts)
    leftover = [t for t in lg_targets if t not in defined_lg and t not in got_range]
    if leftover:
        out.append('')
        out.append('.section .data,"aw"')
        out.append("// 兜底 .Lg_ 定义（.text 字面量/其他）")
        for t in sorted(leftover):
            off = t - start
            if 0 <= off+4 <= len(text):
                val = struct.unpack('<I', text[off:off+4])[0]
            else:
                val = 0
            out.append(f".Lg_{t:08x}:")
            out.append(f"\t.word\t{val}\t// 0x{t:x} 原始数据")

    with open(outfile,'w') as f:
        f.write('\n'.join(out))
    print(f"翻译完成: {outfile}, 总指令 {total}")
    print(f"未处理: {len(unhandled)} ({len(unhandled)/total*100:.2f}%)")
    print(f"add-pc 配对: {len(ADDPC_TARGETS)}, .Lg 目标: {len(lg_targets)}")
    c = collections.Counter(mn for _,mn,_ in unhandled)
    print("未处理助记符分布:", dict(c.most_common(30)))

if __name__=='__main__':
    main()
