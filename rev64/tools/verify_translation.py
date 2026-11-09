#!/usr/bin/env python3
# verify_translation.py — 翻译体生成 vs 原版 自动校验流水线
# 拦截 off-by 类系统性问题（PC 相对寻址 / RELATIVE 重定位 / GOT 槽）
#
# 用法：python3 rev64/tools/verify_translation.py [--verbose]
#
# 校验类别：
#   A. add rX,pc 立即数（thumb2c_full.py pcv=a+4 未对齐）— 原版 19 个全部应有效
#   B. RELATIVE 重定位槽表（jq_rel_data.h）— 应覆盖原版全部 RELATIVE 槽（0 遗漏）
#   C. GOT 槽（JUMP_SLOT/GLOB_DAT）— g_jq_got_tbl 是 JNI 入口池非 GOT，GOT 由 PLT_TMPL 处理
#
# 每类输出：通过/失败 + 缺失清单（供追查）。

import re, subprocess, sys, struct

ORIG_SO = '/tmp/opencode/orig_libjunqi.so'
JQ_REL_H = 'rev64/src_jni_translated/jq_rel_data.h'
JNI_ALIGNED_C = 'rev64/src_jni_translated/jni_aligned.c'
TEXT_OFF, TEXT_SZ = 0x233e0, 0x0fd814

verbose = '--verbose' in sys.argv


def readelf_r():
    out = subprocess.run('arm-linux-gnueabi-readelf -r %s 2>/dev/null' % ORIG_SO,
                         shell=True, capture_output=True, text=True).stdout
    rel, got = [], []
    for line in out.splitlines():
        m = re.match(r'\s*([0-9a-f]+)', line)
        if not m:
            continue
        off = int(m.group(1), 16)
        if 'R_ARM_RELATIVE' in line:
            rel.append(off)
        elif 'R_ARM_JUMP_SLOT' in line or 'R_ARM_GLOB_DAT' in line:
            got.append(off)
    return rel, got


def check_add_pc():
    """A 类：原版 add rX,pc 的 pcv=a+4 应全部有效（thumb2c_full.py 未对齐 PC）。"""
    import capstone
    data = open(ORIG_SO, 'rb').read()
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    bad = []
    for ins in md.disasm(data[TEXT_OFF:TEXT_OFF + TEXT_SZ], TEXT_OFF):
        if ins.mnemonic == 'add' and 'pc' in ins.op_str and ins.op_str.split(',')[1].strip() == 'pc':
            pcv = ins.address + 4
            if not (TEXT_OFF <= pcv < TEXT_OFF + TEXT_SZ):
                bad.append((ins.address, pcv))
    return ('add rX,pc: %s' % ('PASS' if not bad else 'FAIL'), bad)


def check_rel():
    """B 类：jq_rel_data.h 槽表应覆盖原版全部 RELATIVE 槽。"""
    rel, _ = readelf_r()
    txt = open(JQ_REL_H).read()
    jq_slots = set(int(x, 16) for x in re.findall(r'0x([0-9a-f]+)u', txt))
    missing = sorted(set(rel) - jq_slots)
    return ('RELATIVE 槽表: %s (原版 %d / jq %d)' % (
        'PASS' if not missing else 'FAIL', len(set(rel)), len(jq_slots)), missing)


def check_got():
    """C 类：jqt_mem.c g_jqt_got_tbl（真表）——16 个 .text 常量池（<0x155000，静态位移）
    + 2 个 .data 占位（0x15be80/0x15be84，值 0，§124z-3 有意占位）。
    校验：16 个 .text 槽都在 JNI 入口区（0x233f0-0x23608），2 个占位值 0。
    ★ 注意：jni_aligned.c 的 g_jq_got_tbl（密文 18 槽）是**死代码**（旧表，§124x 前），
    不校验（避免误报）。真表在 jqt_mem.c。"""
    txt = open('rev64/src_jni_translated/jqt_mem.c').read()
    m = re.search(r'g_jqt_got_tbl\[\]\s*=\s*\{(.*?)\};', txt, re.S)
    if not m:
        return ('g_jqt_got_tbl: 未找到', [])
    pairs = re.findall(r'\{0x([0-9a-fA-F]+)u?,\s*0x([0-9a-fA-F]+)u?\}', m.group(1))
    offs = [(int(a, 16), int(b, 16)) for a, b in pairs]
    text_pool = [o for o, v in offs if o < 0x155000]
    data_placeholder = [o for o, v in offs if o >= 0x155000]
    # .text 常量池应 16 个，全部在 0x233f0-0x23608
    bad_text = [o for o in text_pool if not (0x233f0 <= o <= 0x23608)]
    # .data 占位应 2 个且值 0
    bad_data = [o for o, v in offs if o >= 0x155000 and v != 0]
    n_bad = len(bad_text) + len(bad_data)
    return ('g_jqt_got_tbl: %d 槽（%d .text 池 + %d .data 占位）%s' % (
        len(offs), len(text_pool), len(data_placeholder),
        'PASS' if n_bad == 0 else 'FAIL'), bad_text + bad_data)


def main():
    print('=== 翻译体 vs 原版 自动校验 ===')
    print('原版 so: %s' % ORIG_SO)
    any_fail = False
    for name, fn in [('A.add_pc', check_add_pc), ('B.rel', check_rel), ('C.got', check_got)]:
        status, bad = fn()
        print('[%s] %s' % (name, status))
        if bad and verbose:
            for b in bad[:20]:
                print('    ', b)
        if bad and not verbose:
            print('    缺失/异常 %d 项（--verbose 看明细）' % len(bad))
        if status != 'PASS':
            any_fail = True
    return any_fail


if __name__ == '__main__':
    import sys
    # ★ AUD-1000-ALL A5-F5-009（P1 已修）：verdict 恒 exit 0——旧版 main() 不返回退出码，
    #   B.rel FAIL（RELATIVE 槽表校验失败）进程仍 exit 0 → CI 假绿。修：main 返 any_fail，
    #   __main__ 按 any_fail sys.exit(1/0)。
    sys.exit(0 if not main() else 1)
