#!/usr/bin/env python3
"""dispatch_insert.py — jqt_vtbl.c dispatch 表批量插入工具（§124y-11 固化）

背景：本轮 F_78890 修复时手动 python 改 3 处（offs 表 / fns 表 / jqt_vtbl_n），
容易漏改或顺序错。本工具一次性完成：

1. 在 jqt_vtbl_offs[] 中指定位置插入 0xADDR
2. 在 jqt_vtbl_fns[] 中对应位置插入 F_ADDR
3. jqt_vtbl_n +1
4. 可选：生成简化翻译体 stub（push/pop + g_cur_fn + 空 return）

用法：
  # 只插表（翻译体已手写）
  python3 rev64/tools/dispatch_insert.py 0x78890
  python3 rev64/tools/dispatch_insert.py 0x78890 0xad248   # 批量插多个

  # 插表 + 生成 stub 翻译体
  python3 rev64/tools/dispatch_insert.py 0x78890 --gen-stub

  # 插表 + 生成 stub + 编译验证
  python3 rev64/tools/dispatch_insert.py 0x78890 --gen-stub --build

幂等：若 0xADDR 已在表则跳过；若 stub 已存在则不覆盖。
"""
import re, sys, os, subprocess

WS = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
V  = os.path.join(WS, 'rev64/reverse/src_jni_translated/jqt_vtbl.c')

def parse_args(argv):
    addrs = []
    gen_stub = '--gen-stub' in argv
    do_build = '--build' in argv
    for a in argv[1:]:
        if a.startswith('0x'):
            addrs.append(int(a, 16))
        elif a not in ('--gen-stub', '--build'):
            print(f"未知参数: {a}", file=sys.stderr); sys.exit(2)
    return addrs, gen_stub, do_build

def _offs_entries(body):
    """★ AUD-1000 B07-013（P1 计数口径统一）：只按**行首完整条目**解析 offs 值
    （与 align_vtbl_n 同口径，排除尾注释含 0x 地址 / 单行双值的多计）。
    旧版 re.findall(r'0x([0-9a-f]+)', body) 在注释/双值行多计 → 插入位置错位。"""
    vals = []
    for ln in body.split('\n'):
        m2 = re.match(r'^\s*0x([0-9a-f]{4,6})\s*,?\s*(?://.*)?$', ln)
        if m2:
            vals.append(int(m2.group(1), 16))
    return vals

def ensure_in_tables(s, off, fns_exists_check):
    """把 0xoff 插进 offs/fns 表（幂等）。fns_exists_check=0xoff 表示对应 F 函数名。"""
    changed = False
    m = re.search(r'const uint32_t jqt_vtbl_offs\[\] = \{(.*?)\};', s, re.S)
    if not m:
        print("✗ 找不到 jqt_vtbl_offs[]", file=sys.stderr); sys.exit(1)
    offs_body = m.group(1)
    addrs_in = _offs_entries(offs_body)
    if off in addrs_in:
        print(f"  0x{off:05x} 已在 offs 表（跳过）")
    else:
        # 按数值顺序找插入点（表是有序的）
        insert_before = None
        for i, v in enumerate(addrs_in):
            if v > off:
                insert_before = i; break
        # 找对应文本行
        lines = offs_body.split('\n')
        target_line_idx = None
        if insert_before is not None:
            # 找 addrs_in[insert_before] 在 lines 里的行号
            for li, ln in enumerate(lines):
                if f'0x{addrs_in[insert_before]:05x}' in ln:
                    target_line_idx = li; break
        else:
            target_line_idx = len(lines)  # 追加末尾
        new_entry = f'    0x{off:05x},'
        if target_line_idx is not None:
            lines.insert(target_line_idx, new_entry)
        else:
            lines.append(new_entry)
        s = s[:m.start(1)] + '\n'.join(lines) + s[m.end(1):]
        changed = True
        print(f"  ✓ 0x{off:05x} 插入 offs 表（在 0x{addrs_in[insert_before]:05x} 前）" if insert_before is not None else f"  ✓ 0x{off:05x} 追加 offs 表末尾")
    return s, changed

def ensure_fns_entry(s, off):
    """在 fns 表对应 0xoff 的位置插入 F_off（与 offs 表同序）。"""
    fn_name = f'F_{off:05x}'
    m = re.search(r'uint32_t \(\*const jqt_vtbl_fns\[\]\)\s*\(.*?\)\s*=\s*\{(.*?)\};', s, re.S)
    if not m:
        print("✗ 找不到 jqt_vtbl_fns[]", file=sys.stderr); sys.exit(1)
    fns_body = m.group(1)
    fns_lines = fns_body.split('\n')
    # 找 F_off 是否已存在
    if any(f'F_{off:05x}' in ln for ln in fns_lines):
        print(f"  F_{off:05x} 已在 fns 表（跳过）")
        return s, False
    # 找 0xoff 在 offs 表中的行号，fns 表对应同一行号
    m_offs = re.search(r'const uint32_t jqt_vtbl_offs\[\] = \{(.*?)\};', s, re.S)
    offs_lines = m_offs.group(1).split('\n')
    target_line_idx = None
    for li, ln in enumerate(offs_lines):
        if f'0x{off:05x}' in ln:
            target_line_idx = li  # fns 与 offs 逐行对齐：0xoff 在第 li 行 → F_off 也在第 li 行
            break
    if target_line_idx is None:
        print(f"  ✗ offs 表里没有 0x{off:05x}（先跑 ensure_in_tables）", file=sys.stderr)
        return s, False
    # 幂等：若该行已有 F_off 则跳过
    if fns_lines[target_line_idx].strip() == f'F_{off:05x},':
        print(f"  F_{off:05x} 已在 fns 表第 {target_line_idx} 行（跳过）")
        return s, False
    new_entry = f'    F_{off:05x},'
    fns_lines.insert(target_line_idx, new_entry)
    s = s[:m.start(1)] + '\n'.join(fns_lines) + s[m.end(1):]
    print(f"  ✓ F_{off:05x} 插入 fns 表（行 {target_line_idx}）")
    return s, True

def align_vtbl_n(s):
    """把 jqt_vtbl_n 对齐到 offs 表实际条目数（幂等，防多次插入时重复 +1 错位）"""
    m = re.search(r'const uint32_t jqt_vtbl_offs\[\] = \{(.*?)\};', s, re.S)
    if not m:
        return s, False
    # ★ AUD-1000 B07-013（P1 计数虚高）：旧版 len(re.findall(r'0x[0-9a-f]+', body))
    #   把尾注释里的 0x 地址 / 单行双值也计入 → jqt_vtbl_n 虚高 → jq_call 二分
    #   越界读 fns 表外。现只匹配**行首完整条目**（4-6 位 hex + 可选逗号 + 行尾），
    #   与 ensure_in_tables 插入格式（'    0x%05x,'）一致。
    actual = 0
    for ln in m.group(1).split('\n'):
        if re.match(r'^\s*0x[0-9a-f]{4,6}\s*,?\s*(?://.*)?$', ln):
            actual += 1
    m2 = re.search(r'const uint32_t jqt_vtbl_n = (\d+)u;', s)
    if not m2:
        print("  ⚠ 找不到 jqt_vtbl_n（需手动补）", file=sys.stderr)
        return s, False
    old = int(m2.group(1))
    if old == actual:
        print(f"  jqt_vtbl_n 已对齐 {actual}（跳过）")
        return s, False
    s = s.replace(m2.group(0), f'const uint32_t jqt_vtbl_n = {actual}u;')
    print(f"  ✓ jqt_vtbl_n: {old} → {actual}（对齐 offs 实际条目数）")
    return s, True

def gen_stub(s, off):
    """生成简化翻译体 stub（幂等：若 F_off 已定义则跳过）。
    插入位置 = jqt_vtbl.c 中"最后一个顶层函数定义结束"前（用 brace 深度=0 的 } 定位），
    避免插到函数体内部。
    ★ AUD-1000 B07-015（P1 锚点失配诊断）：fns 表锚点（'uint32_t (*const jqt_vtbl_fns[]'）
    失配（clang-format 重排/换行漂移致 find 返回 -1）时，旧版静默追加文件末尾 →
    fns[] 引用 F_xxx 先于 stub 定义 → C 隐式声明（C99 禁）→ 编译失败仅在
    do_build 才暴露。现失配时报具体告警（非笼统"可能编译失败"），且
    do_build 失败信息带"锚点失配"特征判别。"""
    fn_name = f'F_{off:05x}'
    # 检查是否已有完整定义（非 extern 声明）
    if re.search(rf'^uint32_t {fn_name}\(uint32_t _r0', s, re.M):
        print(f"  {fn_name} 已有定义（跳过 stub 生成）")
        return s, False
    stub = f"""
uint32_t {fn_name}(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){{
    g_cur_fn = 0x{off:05x};
    /* §124y-11 自动生成 stub——按需手工补完整翻译体 */
    return 0;
}}
"""
    # ★ 插入点：jqt_vtbl_fns[] 数组定义**之前**（fns 表引用 F_xxx，C 需先声明/定义）。
    # 找 "uint32_t (*const jqt_vtbl_fns[])" 行，在其前插入 stub。
    fns_idx = s.find('uint32_t (*const jqt_vtbl_fns[]')
    if fns_idx < 0:
        print(f"  ★ fns 表锚点失配：jqt_vtbl.c 找不到 'uint32_t (*const jqt_vtbl_fns[]'（行格式漂移？"
              f"clang-format 重排换行/const 位置？）。stub 追加文件末尾 → F_{off:05x} 无前置定义，"
              "C 隐式声明（C99 禁）→ 编译失败风险。请核对锚点后重跑 --build。", file=sys.stderr)
        s = s + '\n' + stub
        return s, True
    s = s[:fns_idx] + stub + s[fns_idx:]
    print(f"  ✓ {fn_name} stub 已生成（jqt_vtbl_fns[] 定义前）")
    return s, True

def do_build():
    print("═══ 编译验证 ═══")
    r = subprocess.run(['bash', os.path.join(WS, 'rev64/build_arm32_aligned.sh'),
                        os.path.join(WS, 'rev64/dist/libjunqi_arm32_aligned.so')],
                       capture_output=True, text=True)
    ok = 'OK:' in r.stdout
    print(r.stdout[-300:] if ok else r.stderr[-500:])
    if not ok:
        # ★ AUD-1000 B07-015（P1 诊断具体化）：fns 表锚点失配时 stub 被追加到
        #   文件末尾 → F_xxx 引用先于定义 → C 隐式声明（C99 禁）编译失败。
        #   报具体原因非笼统"编译失败"，便于定位是锚点漂移还是真语法错。
        if re.search(r'implicit declaration of .?F_[0-9a-f]+', r.stderr) or \
           re.search(r'conflicting types|incompatible', r.stderr):
            print("  ★ 疑似 fns 表锚点失配（stub 插入 fns[] 声明之后 → F_xxx 无定义/隐式声明）：",
                  "检查 jqt_vtbl.c 'uint32_t (*const jqt_vtbl_fns[]' 行是否还在（clang-format 重排漂移）", file=sys.stderr)
        else:
            print("  ★ 编译失败（非锚点特征，查上方 stderr）", file=sys.stderr)
    # 审计
    r2 = subprocess.run(['python3', os.path.join(WS, 'rev64/tools/link_audit.py'),
                         'audit', os.path.join(WS, 'rev64/dist/libjunqi_arm32_aligned.so')],
                        capture_output=True, text=True)
    print(r2.stdout[-200:])
    return ok and 'PASS' in r2.stdout

def main():
    addrs, do_gen_stub, do_build_flag = parse_args(sys.argv)
    if not addrs:
        print(__doc__); sys.exit(0)
    s = open(V).read()
    any_changed = False
    tbl_changed = []
    for off in addrs:
        print(f"═══ 处理 0x{off:05x} ═══")
        s, c1 = ensure_in_tables(s, off, off)
        s, c2 = ensure_fns_entry(s, off)
        c_tbl = c1 or c2
        if c_tbl:
            s, _ = align_vtbl_n(s)
            any_changed = True
            tbl_changed.append(off)
        if do_gen_stub:
            s, c4 = gen_stub(s, off)
            if c4: any_changed = True
    if any_changed:
        open(V, 'w').write(s)
        print(f"✓ jqt_vtbl.c 已更新（插表: {['0x%05x'%x for x in tbl_changed]}）")
    else:
        print("无改动（全部幂等跳过）")
    if do_build_flag:
        ok = do_build()
        if not ok:
            print("✗ 编译/审计失败——检查后重跑", file=sys.stderr); sys.exit(1)
        print("✓ 构建+审计 PASS")

if __name__ == '__main__':
    main()
