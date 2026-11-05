#!/usr/bin/env python3
# gen_arm_thunks.py — 扫描原版 32 位 .so 的 .text，提取 ARM this-adjusting thunk
#   vtable 槽位偶数地址 = ARM 模式 thunk：ldr ip,[pc,#8]; add ip,pc,ip; sub r0,r0,#imm; bx ip
#   语义: r0 -= imm; tail-call F_<target>(Thumb 方法)
# 输出: /tmp/arm_thunks.json（[{'a':thunk地址,'imm':调整量,'t':Thumb目标,'bit':bit0}...]）
# 用法: python3 rev64/tools/gen_arm_thunks.py [原版.so] [输出.json]
import sys, struct, json
from capstone import *

SO = sys.argv[1] if len(sys.argv) > 1 else '/tmp/opencode/jq_apk/dec/lib/armeabi/libjunqi.so'
OUT = sys.argv[2] if len(sys.argv) > 2 else '/tmp/arm_thunks.json'
TEXT_LO, TEXT_HI = 0x233e0, 0x120bf8  # .text 段

d = open(SO, 'rb').read()
mdA = Cs(CS_ARCH_ARM, CS_MODE_ARM)
mdT = Cs(CS_ARCH_ARM, CS_MODE_THUMB); mdT.skipdata = True
thunks = []
for off in range(TEXT_LO, TEXT_HI - 16, 4):
    insns = list(mdA.disasm(d[off:off + 16], off))
    if len(insns) < 4:
        continue
    a, b, c, e = insns[0], insns[1], insns[2], insns[3]
    if (a.mnemonic == 'ldr' and a.op_str.startswith('ip, [pc') and
        b.mnemonic == 'add' and b.op_str == 'ip, pc, ip' and
        c.mnemonic == 'sub' and c.op_str.startswith('r0, r0') and
        e.mnemonic == 'bx' and e.op_str == 'ip'):
        lit = struct.unpack_from('<I', d, off + 16)[0]
        tgt = (off + 12 + lit) & 0xffffffff
        imm = int(c.op_str.split('#')[1], 16)
        # 校验目标是真 Thumb 方法（push 前导）
        t_even = tgt & ~1
        first = list(mdT.disasm(d[t_even:t_even + 6], t_even))
        ok = bool(first) and first[0].mnemonic in ('push','mov','movs','sub','add','ldr','str','bx','pop','stm','ldm','vpush')
        thunks.append({'a': off, 'imm': imm, 't': tgt & ~1, 'bit': tgt & 1, 'ok': bool(ok)})

json.dump(thunks, open(OUT, 'w'))  # 全部（含 ok=False 的 7 个，gen_shadow 仍按 ARM 模式翻译）
print(f'ARM this-adjusting thunk: {len(thunks)} 扫描, {sum(1 for t in thunks if t["ok"])} 目标为真方法')
print(f'写出 {OUT}')
