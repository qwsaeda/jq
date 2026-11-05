#!/usr/bin/env python3
"""gen_shadow.py — 生成"影子 32 位地址空间"运行时的机械翻译 C 代码。

核心思想：把原版 .so 的 32 位指针语义原样保留。
- 所有原版指针仍然是 uint32_t（影子地址），真实内存 = g_jq_base + addr
- 每个原版函数翻译成 uint32_t F_xxx(r0..r11, sp)，push/pop 落在影子栈上
- 任何间接调用（blx rX / bx rX / mov pc,rX）统一走 jq_call(target, regs, sp)，
  target 取值区间决定去向：翻译函数 / PLT stub / import 令牌 / JNI thunk 令牌
- 修正项（相对 thumb2c_full.translate_func）：
  1. Thumb `add rd, pc` 的 PC = 指令地址+4（**不**做 4 字节对齐；只有 ldr [pc,#imm] 对齐）
  2. push/pop 保存/恢复列表内全部寄存器（含 r0-r3/r12/lr/pc 槽位）
  3. bl/blx/bx/mov pc 不再存根 r0=0，改为真调用 jq_call
用法: python3 gen_shadow.py <outdir> [函数集 json]
"""
import os, re, sys, json, struct, bisect, importlib

sys.path.insert(0, '/tmp/opencode')
import thumb2c_full as T

SO = '/tmp/opencode/jq_apk/dec/lib/armeabi/libjunqi.so'
RODATA_DEC = '/disk/workspaces/jq/frida-dump/rodata_dec.bin'
RODATA_VA = 0x120bf8
DRL_LO, DRL_HI = 0x155030, 0x15afe0
GOT_LO, GOT_HI = 0x15b0c8, 0x15bb6c
DATA_LO, DATA_HI = 0x15bb6c, 0x15be80
PLT_LO, PLT_HI = 0x229c0, 0x233e0
TEXT_LO, TEXT_HI = 0x233e0, 0x120bf8

REGRE = r'(?:r(?:1[0-5]|[0-9])|sp|lr|pc)'
REGORDER = {f'r{i}': i for i in range(13)}
REGORDER['sp'] = 13
REGORDER['lr'] = 14
REGORDER['pc'] = 15


def load_insns():
    insns = T.load_insns()
    if T._ADDRS_CACHE is None or len(T._ADDRS_CACHE) != len(insns):
        T._ADDRS_CACHE = [a for a, mn, r in insns]
        T._IDX_CACHE = {a: i for i, a in enumerate(T._ADDRS_CACHE)}
    return insns


def reglist(r):
    m = re.match(r'\{([^}]*)\}', r)
    if not m:
        return []
    regs = re.findall(REGRE, m.group(1))
    return sorted(regs, key=lambda x: REGORDER.get(x, 99))


CALLARGS = "r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp"
JQ_PASS_STR = CALLARGS


def emit_insn(a, mn, r):
    """返回该指令对应的 C 行（替换原翻译器输出）。返回 None 表示沿用原输出。"""
    lbl = f"L{a:x}"
    if mn == 'push':
        regs = reglist(r)
        if not regs:
            return [f"{lbl}: ;"]
        out = [f"{lbl}: /* push {r} */"]
        out.append(f"{lbl}_sp: sp-=({len(regs)*4});")
        for k, reg in enumerate(regs):
            if reg == 'sp':
                continue
            out.append(f"{lbl}_p{k}: MEM(sp+{k*4})={reg};")
        return out
    if mn == 'pop':
        regs = reglist(r)
        has_pc = 'pc' in regs
        out = []
        first = True
        for k, reg in enumerate(regs):
            if reg == 'sp':
                continue
            if reg == 'pc':
                continue
            pre = f"{lbl}:" if first else f"{lbl}_l{k}:"
            first = False
            out.append(f"{pre} {reg}=MEM(sp+{k*4});")
        if first:
            out.append(f"{lbl}: ;")
        out.append(f"{lbl}_sp: sp+=({len(regs)*4});")
        if has_pc:
            out.append(f"{lbl}_r: goto RET;")
        return out
    # 直接调用 / 间接调用
    if mn in ('bl', 'blx'):
        m = re.search(r'0x([0-9a-f]+)', r)
        if m:
            t = int(m.group(1), 16)
            if PLT_LO <= t < PLT_HI:
                return [f"{lbl}: r0=jq_plt(0x{t:x},{CALLARGS});"]
            return [f"{lbl}: r0=jq_call(0x{t:x},{CALLARGS});"]
        m = re.match(REGRE + r'$', r.strip())
        if m:
            return [f"{lbl}: r0=jq_call({m.group(0)},{CALLARGS});"]
        return [f"{lbl}: /* !!! 未识别 {mn} {r} */"]
    if mn == 'bx':
        m = re.match(REGRE + r'$', r.strip())
        if m and m.group(0) == 'lr':
            return [f"{lbl}: goto RET;"]
        if m:
            # bx reg：可能是"pop {reg}; bx reg"形式（reg=LR 槽 → 值恒 0，真返回），
            # 也可能是真尾调用（reg=函数指针）。翻译函数里 lr 局部恒 0，因此
            # reg==0 即代表 LR 槽被 pop 出来 → 返回；否则按真尾调用走 jq_call。
            return [f"{lbl}: if({m.group(0)}) r0=jq_call({m.group(0)},{CALLARGS}); goto RET;"]
        return [f"{lbl}: goto RET;"]
    # cbz/cbnz：寄存器为零/非零分支（capstone 形式，llvm 无此助记符）
    if mn in ('cbz', 'cbnz'):
        m = re.match(r'(' + REGRE + r'),\s*#?0x?([0-9a-f]+)$', r.strip())
        if m:
            reg = m.group(1)
            t = int(m.group(2), 16)
            cond = '==0' if mn == 'cbz' else '!=0'
            return [f"{lbl}: if({reg}{cond}) goto L{t:x};"]
        return [f"{lbl}: /* !!! 未识别 {mn} {r} */"]
    if mn in ('mov', 'movs') and re.match(r'pc\s*,\s*' + REGRE + r'$', r.strip()):
        rm = re.match(r'pc\s*,\s*(' + REGRE + r')$', r.strip()).group(1)
        return [f"{lbl}: r0=jq_call({rm},{CALLARGS}); goto RET;"]
    if mn in ('mov', 'movs') and re.match(r'(' + REGRE + r')\s*,\s*pc$', r.strip()):
        rd = re.match(r'(' + REGRE + r')\s*,\s*pc$', r.strip()).group(1)
        return [f"{lbl}: {rd}={a+4};"]
    if mn == 'add' and re.match(r'pc\s*,\s*lr$', r.strip()):
        return [f"{lbl}: goto RET;"]
    if mn == 'cmp' and re.match(r'pc\s*,\s*pc$', r.strip()):
        return [f"{lbl}: Z=1; C=1; N=0; V=0;"]
    # PC 相对 add：Thumb 语义 PC=指令地址+4（不对齐）
    if mn in ('add', 'adds'):
        m = re.match(r'(' + REGRE + r')\s*,\s*pc$', r.strip())
        if m:
            return [f"{lbl}: {m.group(1)}={a+4}+{m.group(1)};"]
        m = re.match(r'(' + REGRE + r')\s*,\s*pc\s*,\s*#(.*)$', r.strip())
        if m:
            return [f"{lbl}: {m.group(1)}=(({a+4})&~3u)+({m.group(2)});"]
    return None


_IMAP = None


def dedupe_labels(txt):
    out = []
    seen = set()
    skipping = False
    for ln in txt.split('\n'):
        m = re.match(r'L([0-9a-f]+):', ln)
        if m:
            key = m.group(1)
            if key in seen:
                skipping = True
                continue
            seen.add(key)
            skipping = False
            out.append(ln)
            continue
        if skipping:
            if re.match(r'L[0-9a-f]+_', ln):
                continue
            skipping = False
        out.append(ln)
    return '\n'.join(out)


def _norm(mn):
    # capstone 的 .w 宽指令与普通形式助记符一致（add.w==add, ldr.w==ldr ...），
    # 剥掉 .w 后缀让 thumb2c_full 的基础处理器接管；movw/movt 保留（需单独处理）。
    if mn.endswith('.w') and mn not in ('movw', 'movt'):
        return mn[:-2]
    return mn


def translate_shadow(insns, f):
    global _IMAP
    insns2 = [(a, _norm(mn), r) for a, mn, r in insns]
    lines = T.translate_func(insns2, f, f + 1, callee=set(), full=False)
    if _IMAP is None or len(_IMAP) != len(insns):
        _IMAP = {a: (_norm(mn), r) for a, mn, r in insns}
    imap = _IMAP
    out = []
    i = 0
    n = len(lines)
    while i < n:
        ln = lines[i]
        if ln.strip().startswith('#define'):
            i += 1
            continue
        m = re.match(r'L([0-9a-f]+)[:_]', ln)
        if not m:
            out.append(ln)
            i += 1
            continue
        a = int(m.group(1), 16)
        j = i
        pat = re.compile(r'L%x[:_]' % a)
        while j < n and pat.match(lines[j]):
            j += 1
        mn, rr = imap.get(a, ('?', ''))
        rep = emit_insn(a, mn, rr)
        out.extend(rep if rep is not None else lines[i:j])
        i = j
    # 后处理：去掉指向"未定义标签"的 goto（cbz/cbnz 目标落在数据池/误对齐区）。
    # 例如 disasm 数据池导致 cbnz r0,#0x40868 的 0x40868 不是合法指令地址，
    # _reachable 不会把 0x40868 加入 body → 无 L40868: 标签 → 编译失败。
    # 处理：把 goto L<未定义> 替换为注释（等价 fall-through），保住其余翻译。
    defined = set(re.findall(r'^L([0-9a-f]+):', '\n'.join(out), re.M))
    fixed = []
    for ln in out:
        m = re.search(r'goto L([0-9a-f]+);', ln)
        if m and m.group(1) not in defined:
            ln = re.sub(r'goto L([0-9a-f]+);', '/* 未定义跳转目标 L\\1，fall-through */;', ln)
        fixed.append(ln)
    return fixed


SIG = ','.join(['uint32_t'] * 13)


PARAMS = ','.join(f'uint32_t _{n}' for n in ['r0','r1','r2','r3','r4','r5','r6','r7','r8','r9','r10','r11','sp'])


def gen_funcs(insns, funcs, texts):
    decls = ''.join(f'uint32_t F_{a:x}({SIG});\n' for a in funcs)
    body = []
    for a in funcs:
        body.append(f'uint32_t F_{a:x}({PARAMS}){{')
        body.append(texts[a])
        body.append('    JQ_RET(r0);\n}\n')
    return decls, '\n'.join(body)


def c_bytes(name, data, per=24):
    out = [f'const unsigned char {name}[] = {{']
    for i in range(0, len(data), per):
        out.append(','.join(str(b) for b in data[i:i + per]) + ',')
    out.append('};')
    out.append(f'const unsigned int {name}_len = {len(data)};')
    return '\n'.join(out)


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else '/tmp/opencode/shadow'
    seedf = sys.argv[2] if len(sys.argv) > 2 else '/tmp/opencode/seed_all.json'
    os.makedirs(outdir, exist_ok=True)
    d = open(SO, 'rb').read()
    rob = open(RODATA_DEC, 'rb').read()
    insns = load_insns()
    funcs = sorted(set(json.load(open(seedf))))
    print(f'函数数 {len(funcs)}')

    # ARM this-adjusting thunk 表（vtable 槽位是偶数地址 = ARM 模式）：
    #   0x236f0(ARM): ldr ip,[pc,#8]; add ip,pc,ip; sub r0,r0,#0x20; bx ip
    #   → 真实语义: r0 -= 0x20; tail-call F_<target>（Thumb 方法，见 /tmp/arm_thunks.json）
    # Thumb 反汇编把它误解为跳表 case（stm r0!,{r3}; b ...），导致对象图被破坏。
    # 这里是 264 个 ARM thunk 的正确翻译（257 个目标确认为 push 前导的真实方法）。
    ARM_THUNKS = {}
    try:
        for e in json.load(open('/tmp/arm_thunks.json')):
            ARM_THUNKS[e['a']] = (e['imm'], e['t'])
        print(f'ARM thunk 覆盖 {len(ARM_THUNKS)}')
    except FileNotFoundError:
        print('警告: /tmp/arm_thunks.json 不存在，跳过 ARM thunk 覆盖')

    # thunk 目标必须进 seed（否则 F_<target> 未声明）
    extra_seed = set()
    for a, (imm, t) in ARM_THUNKS.items():
        extra_seed.add(a)
        extra_seed.add(t)
    new_funcs = sorted(set(funcs) | extra_seed)
    if len(new_funcs) != len(funcs):
        print(f'seed 增加 thunk 目标: {len(funcs)} -> {len(new_funcs)}')
    funcs = new_funcs

    # 每个函数只翻译一次（正则很慢）
    texts = {}
    for k, a in enumerate(funcs):
        if a in ARM_THUNKS:
            imm, t = ARM_THUNKS[a]
            texts[a] = (
                f'    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, '
                f'r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;\n'
                f'    uint32_t sp=_sp, lr=0;\n'
                f'    int Z=0, C=0, N=0, V=0;\n'
                f'    r0 = r0 - {imm}u;\n'
                f'    JQ_RET(F_{t:x}(r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, sp));\n'
            )
            if k % 200 == 0:
                print(f'  翻译 {k}/{len(funcs)} (ARM thunk 0x{a:x})', flush=True)
            continue
        # bx lr 返回桩（raw bytes 0x70 0x47）：即使反汇编在数据池处失步（如 0x7f4ec），
        # 也强制翻译为"原样返回 r0"，避免被当成完整函数导致无限递归。
        if d[a:a + 2] == b'\x70\x47':
            texts[a] = (
                f'    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, '
                f'r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;\n'
                f'    uint32_t sp=_sp, lr=0;\n'
                f'    int Z=0, C=0, N=0, V=0;\n'
                f'    JQ_RET(r0);\n'
            )
            if k % 200 == 0:
                print(f'  翻译 {k}/{len(funcs)} (bx lr 桩 0x{a:x})', flush=True)
            continue
        txt = '\n'.join(translate_shadow(insns, a))
        txt = re.sub(r'\(\((r\d+|r1[0-2])>>32\)\)&0xffffffffu', '0u', txt)
        txt = dedupe_labels(txt)
        texts[a] = txt.replace('RET: return r0;', 'RET: JQ_RET(r0);')
        if k % 200 == 0:
            print(f'  翻译 {k}/{len(funcs)}', flush=True)

    # 按函数切块，4 个文件
    chunks = 4
    per = (len(funcs) + chunks - 1) // chunks
    parts = []
    for ci in range(chunks):
        sl = funcs[ci * per:(ci + 1) * per]
        if not sl:
            continue
        dl, bd = gen_funcs(insns, sl, texts)
        fn = os.path.join(outdir, f'jq_funcs_{ci}.c')
        with open(fn, 'w') as f:
            f.write('#include "jq_shadow.h"\n')
            f.write(dl)
            f.write(bd)
        parts.append((fn, len(sl)))
        print(f'  {fn}: {len(sl)} 函数')

    # 分发表
    with open(os.path.join(outdir, 'jq_dispatch.c'), 'w') as f:
        f.write('#include "jq_shadow.h"\n')
        f.write(''.join(f'uint32_t F_{a:x}({SIG});\n' for a in funcs))
        f.write('\nconst jq_disp_t jq_dispatch[] = {\n')
        for a in funcs:
            f.write(f'  {{0x{a:x}u, F_{a:x}}},\n')
        f.write('};\n')
        f.write(f'const int jq_dispatch_n = {len(funcs)};\n')

    # 镜像数据：整段 [0x233e0, 0x162e80)（.text/.rodata/.ARM.*/.init_array/
    # .data.rel.ro/.dynamic/.got/.data/.bss），其中 .rodata 用解密态替换。
    # .text 里的字面量池是静态明文，翻译后的代码用 MEM() 读取，必须真实存在。
    IMG_LO, IMG_HI = 0x233e0, 0x15be80  # 到 .data 结束；.bss 保持清零
    blob = bytearray(d[IMG_LO:IMG_HI])
    r_off = RODATA_VA - IMG_LO
    blob[r_off:r_off + len(rob)] = rob
    blobpath = os.path.join(outdir, 'jq_img.bin')
    with open(blobpath, 'wb') as f:
        f.write(blob)
    with open(os.path.join(outdir, 'jq_image.c'), 'w') as f:
        f.write('#include "jq_shadow.h"\n')
        f.write(f'#define JQ_IMG_VA 0x{IMG_LO:x}u\n')
        f.write(f'#define JQ_IMG_END_VA 0x{IMG_HI:x}u\n')
        f.write('__asm__(".section .rodata\\n"\n')
        f.write('        ".global jq_img_blob\\n"\n')
        f.write('        "jq_img_blob:\\n"\n')
        f.write(f'        ".incbin \\"{blobpath}\\"\\n"\n')
        f.write('        ".global jq_img_blob_end\\n"\n')
        f.write('        "jq_img_blob_end:\\n"\n')
        f.write('        ".previous\\n");\n')

    # PLT 表和导入清单（全部 214 个 .rel.plt 槽位：令牌 -> import 索引）
    plt = json.load(open('/tmp/opencode/plt_all.json'))
    def _slot(v):
        x = v['slot']
        return int(x, 16) if isinstance(x, str) else int(x)
    slots = sorted(((_slot(v), v['sym']) for v in plt.values() if v.get('sym')))
    names = [s for _, s in slots]
    impidx = {}
    for i, (_, s) in enumerate(slots):
        impidx.setdefault(s, i)
    supported = set(names)
    stubs = json.load(open('/tmp/opencode/seed_imports.json'))['stubs']

    allstubs = sorted((int(k, 16), v) for k, v in plt.items() if v.get('sym'))
    with open(os.path.join(outdir, 'jq_plt_tbl.c'), 'w') as f:
        f.write('#include "jq_shadow.h"\n')
        f.write('const jq_plt_t jq_plt_tbl[] = {\n')
        for sa, v in allstubs:
            sv = v['slot']
            slot = int(sv, 16) if isinstance(sv, str) else int(sv)
            f.write(f'  {{0x{sa:x}u, 0x{slot:x}u}},\n')
        f.write('};\n')
        f.write(f'const int jq_plt_tbl_n = {len(allstubs)};\n')

    with open(os.path.join(outdir, 'jq_slot_imp.c'), 'w') as f:
        f.write('#include "jq_shadow.h"\n')
        f.write('const jq_slot_imp_t jq_slot_imp[] = {\n')
        for slot, sym in slots:
            f.write(f'  {{0x{slot:x}u, {impidx[sym]}}},\n')
        f.write('};\n')
        f.write(f'const int jq_slot_imp_n = {len(slots)};\n')

    with open(os.path.join(outdir, 'jq_imports_gen.h'), 'w') as f:
        f.write('/* 自动生成：.rel.plt 槽位 -> import 索引 */\n')
        f.write('#ifndef JQ_IMPORTS_GEN_H\n#define JQ_IMPORTS_GEN_H\n')
        for sym, i in sorted(impidx.items()):
            f.write(f'#define JQ_IMP_{sym} {i}\n')
        f.write(f'#define JQ_IMP_N {len(slots)}\n#endif\n')

    with open(os.path.join(outdir, 'jq_import_disp.c'), 'w') as f:
        f.write('#include "jq_shadow.h"\n')
        f.write('#include "jq_imports_gen.h"\n')
        f.write(''.join(f'uint32_t jq_imp_{s}({SIG});\n' for s in sorted(supported)))
        f.write('\nconst char *const jq_imp_names[] = {\n')
        for s in names:
            f.write(f'  "{s}",\n')
        f.write('};\n')
        f.write(f'const int jq_imp_names_n = {len(names)};\n\n')
        f.write('uint32_t jq_import(uint32_t idx, %s)\n{\n' % PARAMS)
        f.write('  uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, sp=_sp;\n')
        f.write('  (void)r4; (void)r5; (void)r6; (void)r7; (void)r8; (void)r9; (void)r10; (void)r11; (void)sp;\n')
        f.write('  switch (idx) {\n')
        for sym, i in sorted(impidx.items()):
            if sym in supported:
                f.write(f'    case {i}: return jq_imp_{sym}({JQ_PASS_STR});\n')
        f.write('    default: break;\n  }\n')
        f.write('  jq_log("[jq] 未实现 import #%u %s\\n", idx, '
                'idx < (uint32_t)jq_imp_names_n ? jq_imp_names[idx] : "?");\n')
        f.write('  return 0;\n}\n')
    print(f'  import 符号 {len(impidx)} 个（实现 {len(supported)}），PLT 表 {len(stubs)} 条')

    # 初始化函数（init_array 12 个）
    ia = [struct.unpack_from('<I', d, x)[0] & ~1 for x in range(0x155000, 0x155030, 4)]
    with open(os.path.join(outdir, 'jq_init_array.c'), 'w') as f:
        f.write('#include "jq_shadow.h"\n')
        f.write('const uint32_t jq_init_array[] = {')
        f.write(','.join(f'0x{x:x}u' for x in ia))
        f.write('};\n')
        f.write(f'const int jq_init_array_n = {len(ia)};\n')

    print('生成完成:', outdir)


if __name__ == '__main__':
    main()
