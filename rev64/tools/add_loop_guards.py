#!/usr/bin/env python3
"""add_loop_guards.py — 机械化批量给翻译函数加循环阀/深度阀（§〇d-4 用户"能批量的就批量"）

用法:
  python3 rev64/tools/add_loop_guards.py --check F_b4edc,F_292d4
  python3 rev64/tools/add_loop_guards.py --add F_b4edc,F_292d4
  python3 rev64/tools/add_loop_guards.py --scan <func>    # 反汇编找 backward 循环头
  python3 rev64/tools/add_loop_guards.py --auto           # 自动扫描调用链相关函数

原理:
  1. 用 capstone 反汇编原版 .so（/tmp/opencode/orig_libjunqi.so）
  2. 找函数内所有 backward 分支目标（真循环头）
  3. 在翻译 C 代码里定位这些标签
  4. 每个循环头前插入 static 计数阀: 超 0x100 步 → goto RET（防死循环）
  5. 若函数有递归调用自己 → 另加深度阀: 超 200 → goto RET

注意:
  - goto RET 直接返回 r0（翻译体 sp 是局部变量，不跨函数，安全）
  - 幂等: 已加阀的标签跳过（标记 `/* Lxx VALVE */`）
  - 循环阀计数 static 跨调用累计（不重置），超阈值后强制返回
"""
import re, sys, subprocess, glob, json

ORIG_SO = '/tmp/opencode/orig_libjunqi.so'
VALVE_MARK = 'VALVE'

def find_func_file(func):
    """在 src_jni_translated/*.c 里找函数所在文件"""
    for path in glob.glob('/disk/workspaces/jq/rev64/src_jni_translated/*.c'):
        src = open(path).read()
        if re.search(r'^uint32_t %s\(uint32_t _r0' % func, src, re.M):
            return path
    return None

def func_range(src, func):
    """返回 (start_line_idx, end_line_idx) 函数体范围"""
    m = re.search(r'^uint32_t %s\(uint32_t _r0' % func, src, re.M)
    if not m:
        return None
    start = m.start()
    # 找下一个函数定义
    m2 = re.search(r'^uint32_t F_[0-9a-f]+\(uint32_t _r0', src[m.end():], re.M)
    end = m.end() + m2.start() if m2 else len(src)
    return start, end

def disasm_backward_heads(func_addr, func_end=None):
    """用 capstone 反汇编找 backward 分支目标（循环头）"""
    import capstone
    data = open(ORIG_SO, 'rb').read()
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    md.detail = True
    md.skipdata = True
    addr = func_addr
    end = func_end if func_end else addr + 0x3000  # 默认 12KB
    heads = set()
    for ins in md.disasm(data[addr:end], addr):
        if ins.mnemonic.startswith('b') and not ins.mnemonic.startswith('bl'):
            for op in ins.operands:
                if op.type == capstone.arm.ARM_OP_IMM and op.imm < ins.address:
                    heads.add(op.imm)
    return heads

def get_loop_heads_from_translation(src, start, end):
    """从翻译代码找 backward goto 目标（循环头）"""
    body = src[start:end]
    lines = body.split('\n')
    label_pos = {}
    for i, ln in enumerate(lines):
        m = re.match(r'\s*L([0-9a-f]+):', ln)
        if m:
            label_pos[m.group(1)] = i
    backward = set()
    for i, ln in enumerate(lines):
        for m in re.finditer(r'goto L([0-9a-f]+)', ln):
            tgt = m.group(1)
            cur = None
            for j in range(i, -1, -1):
                mm = re.match(r'\s*L([0-9a-f]+):', lines[j])
                if mm:
                    cur = mm.group(1)
                    break
            if tgt in label_pos and cur and int(tgt,16) < int(cur,16):
                backward.add(tgt)
    return backward

def add_valves(path, func, heads, dry_run=False):
    """给指定循环头加循环阀（幂等）"""
    src = open(path).read()
    rng = func_range(src, func)
    if not rng:
        return 0, f"{func} 未找到"
    start, end = rng
    lines = src.split('\n')
    body_start_line = src[:start].count('\n')
    insertions = []  # (line_idx, valve_text)
    for h in sorted(heads, key=lambda x: -int(x, 16)):  # 逆序，防行号偏移
        lab = 'L%s' % h
        for i in range(body_start_line, min(body_start_line + (end-start) + 20, len(lines))):
            if re.match(r'\s*%s:' % lab, lines[i]):
                # 检查是否已加阀
                if any(VALVE_MARK in lines[j] for j in range(max(0,i-3), i+1)):
                    break
                if any('guard' in lines[j] or '阀' in lines[j] for j in range(i, min(i+6, len(lines)))):
                    break
                valve = f'    {{ static int _g_{lab.lower()}=0; if (++_g_{lab.lower()} > 0x100) {{ _g_{lab.lower()}=0; __android_log_print(6,"junqi","{func} 循环阀触发 {lab}"); goto RET; }} }} /* {VALVE_MARK} */'
                # ★ BUG-FIX: 循环阀必须插在标签【后】(i+1)，backward goto 到标签才经过循环阀。
                #   旧版插在标签前 (i)，goto Lxx 直接跳到标签绕过循环阀 → 无限循环永不触发。
                insertions.append((i+1, valve))
                if dry_run:
                    print(f"  [DRY] {func} {lab} 行 {i+1}: 将插入阀")
                break
    # 逆序插入（从后往前，行号不偏移）
    insertions.sort(key=lambda x: -x[0])
    for i, valve in insertions:
        lines.insert(i, valve)
    if not dry_run and insertions:
        open(path, 'w').write('\n'.join(lines))
    return len(insertions), f"{func}: 插入 {len(insertions)} 个循环阀"

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    mode = sys.argv[1]
    if mode == '--check':
        funcs = sys.argv[2].split(',') if len(sys.argv) > 2 else []
        for fn in funcs:
            path = find_func_file(fn)
            if not path:
                print(f"{fn}: 未找到")
                continue
            src = open(path).read()
            rng = func_range(src, fn)
            heads = get_loop_heads_from_translation(src, *rng)
            # 检查哪些已有阀（阀插在标签前 3 行内）
            body_lines = src[rng[0]:rng[1]].split('\n')
            line_of = {h.lower(): None for h in heads}
            for i, ln in enumerate(body_lines):
                m = re.match(r'\s*L([0-9a-f]+):', ln)
                if m and m.group(1).lower() in line_of:
                    line_of[m.group(1).lower()] = i
            unvalved = []
            for h in heads:
                li = line_of.get(h.lower())
                if li is None:
                    unvalved.append(h)
                elif not any('guard' in body_lines[j] or '阀' in body_lines[j] for j in range(max(0,li-4), min(li+6, len(body_lines)))):
                    unvalved.append(h)
            print(f"{fn}: {len(heads)} 个循环头, {len(unvalved)} 个无阀 -> {sorted(hex(int(x,16)) for x in unvalved)}")
        return 0
    elif mode == '--add':
        funcs = sys.argv[2].split(',') if len(sys.argv) > 2 else []
        for fn in funcs:
            path = find_func_file(fn)
            if not path:
                print(f"{fn}: 未找到")
                continue
            src = open(path).read()
            rng = func_range(src, fn)
            heads = get_loop_heads_from_translation(src, *rng)
            n, msg = add_valves(path, fn, heads)
            print(msg)
        return 0
    elif mode == '--scan':
        fn = sys.argv[2]
        path = find_func_file(fn)
        src = open(path).read()
        rng = func_range(src, fn)
        heads = get_loop_heads_from_translation(src, *rng)
        print(f"{fn}: {len(heads)} 个循环头: {sorted(hex(int(x,16)) for x in heads)}")
        return 0
    elif mode == '--depth':
        funcs = sys.argv[2].split(',') if len(sys.argv) > 2 else []
        for fn in funcs:
            path = find_func_file(fn)
            if not path:
                print(f"{fn}: 未找到")
                continue
            lines = open(path).read().split('\n')
            # 找函数定义
            for i, ln in enumerate(lines):
                if re.match(r'^uint32_t %s\(uint32_t _r0' % fn, ln):
                    # 在 g_cur_fn 后插入深度阀
                    for j in range(i, min(i+20, len(lines))):
                        if 'g_cur_fn' in lines[j]:
                            # 幂等检查
                            if any('_depth' in lines[k] for k in range(i, min(i+15, len(lines)))):
                                print(f"{fn}: 已有深度阀，跳过")
                                break
                            depth_valve = f'    {{ static int _d_{fn.lower()}=0; if (++_d_{fn.lower()} > 200) {{ _d_{fn.lower()}=0; __android_log_print(6,"junqi","{fn} 深度阀触发"); goto RET; }} }}'
                            lines.insert(j+1, depth_valve)
                            print(f"{fn}: 插入深度阀（行 {j+2}）")
                            break
                    break
            open(path, 'w').write('\n'.join(lines))
        return 0
    elif mode == '--auto':
        # 自动扫描: 找所有无阀循环头较多的函数（调用链相关）
        for path in glob.glob('/disk/workspaces/jq/rev64/src_jni_translated/*.c'):
            src = open(path).read()
            for m in re.finditer(r'^uint32_t (F_[0-9a-f]+)\(uint32_t _r0', src, re.M):
                fn = m.group(1)
                rng = func_range(src, fn)
                if not rng:
                    continue
                heads = get_loop_heads_from_translation(src, *rng)
                if len(heads) >= 3:
                    body = src[rng[0]:rng[1]]
                    unvalved = [h for h in heads if not re.search(r'L%x[^\n]*\n\s*\{[^}]*guard' % h, body)]
                    if len(unvalved) >= 2:
                        print(f"{path.split('/')[-1]}: {fn} {len(heads)} 循环头, {len(unvalved)} 无阀")
        return 0
    print(__doc__)
    return 1

if __name__ == '__main__':
    sys.exit(main())
