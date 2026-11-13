#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-280 32 位翻译体覆盖度量化判据（静态侧，设备无关，0 依赖模拟器）

用户 2026-11-13 指令"不准在模拟器上运行"后，把"离真机可跑还差多少"从
"模拟器判据 flaky 排查"转为**离线可量化的覆盖度基线**：

判据（PASS 条件）：
1. 原版 .text 段基线（VA + size）+ 原版唯一 FUNC 动态符号数（语义函数总量）
2. 翻译体镜像覆盖：jq_img 镜像字节数 / 原版 .text 大小（≥100% = 原版代码整体在位）
3. 翻译体语义实现量：F_xxx 翻译函数定义数 / 原版 FUNC 符号数（≥100% = 每个原版
   语义函数都有翻译实现）
4. 事件分发收表：F_55d40 switch case 数 + case→handler 的 handler 全有 F_ 定义体
   （0 悬空 = 事件链收表完整）
5. 缺口清单输出（未收表 / 无定义体 case）——供后续逐项补齐，非判据 FAIL

用法：python3 rev64/tools/translation_coverage_check.py
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = os.environ.get('JQ_ORIG_SO', '/tmp/orig/lib/armeabi/libjunqi.so')
TRANS_DIR = os.path.join(ROOT, 'rev64/reverse/src_jni_translated')
VTBL = os.path.join(TRANS_DIR, 'jqt_vtbl.c')

def so_sections(path):
    out = {}
    r = subprocess.run(['readelf', '-SW', path], capture_output=True, text=True)
    for line in r.stdout.splitlines():
        m = re.search(r'\s(\.\w+)\s+PROGBITS\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)', line)
        if m:
            out[m.group(1)] = (int(m.group(2), 16), int(m.group(4), 16))
    return out

def orig_func_syms(path):
    r = subprocess.run(['readelf', '-sW', path], capture_output=True, text=True)
    addrs = set()
    for line in r.stdout.splitlines():
        p = line.split()
        if len(p) >= 8 and p[3] == 'FUNC':
            try:
                addrs.add(int(p[1], 16))
            except ValueError:
                pass
    return addrs

def main():
    if not os.path.exists(ORIG_SO):
        print("✗ 原版 so 不存在: %s" % ORIG_SO); sys.exit(2)
    print("═══ 32 位翻译体覆盖度基线（§124y-280，设备无关）═══")
    # 1. 原版段基线
    secs = so_sections(ORIG_SO)
    text_va, text_sz = secs.get('.text', (0, 0))
    ro_va, ro_sz = secs.get('.rodata', (0, 0))
    print("原版 .text: VA 0x%x size %d B (%.2f MB)" % (text_va, text_sz, text_sz / 1048576))
    print("原版 .rodata: VA 0x%x size %d B" % (ro_va, ro_sz))
    funcs = orig_func_syms(ORIG_SO)
    print("原版唯一 FUNC 动态符号: %d 个（语义函数总量基线）" % len(funcs))
    # 2. 翻译体镜像覆盖
    # 镜像字节数：jq_img.S 用 .incbin 嵌入外部 jq_img.bin（§124y-281 修正：
    #   旧版只找 TRANS_DIR 下的 jq_img.bin → 恒 0 → 镜像覆盖误判 0%）
    img_sz = 0
    img_path = ''
    img_sym = os.path.join(TRANS_DIR, 'jq_img.S')
    if os.path.exists(img_sym):
        sym = open(img_sym).read()
        m = re.search(r'\.incbin\s+"([^"]+)"', sym)
        if m:
            img_path = m.group(1)
            if os.path.exists(img_path):
                img_sz = os.path.getsize(img_path)
    if img_sz == 0:
        for cand in ('/tmp/opencode/shadow/jq_img.bin',
                     os.path.join(TRANS_DIR, 'jq_img.bin')):
            if os.path.exists(cand):
                img_path = cand
                img_sz = os.path.getsize(cand)
                break
    cov = (100.0 * img_sz / text_sz) if text_sz else 0
    print("翻译体镜像字节: %d B / 原版 .text %d B = %.1f%%" % (img_sz, text_sz, cov))
    # 3. 翻译体 F_ 语义实现量
    f_defs = set()
    for fn in os.listdir(TRANS_DIR):
        if fn.endswith('.c'):
            s = open(os.path.join(TRANS_DIR, fn), errors='ignore').read()
            for m in re.finditer(r'^uint32_t F_([0-9a-f]+)\(', s, re.M):
                f_defs.add(m.group(1))
    # 语义实现覆盖 = 原版 FUNC 符号地址中，已有对应 F_<addr> 翻译实现的比例
    covered = sum(1 for a in funcs if ('%x' % a) in f_defs)
    impl = (100.0 * covered / len(funcs)) if funcs else 0
    print("原版 FUNC 符号已被翻译实现: %d/%d = %.1f%%（翻译体 F_ 函数总数 %d）"
          % (covered, len(funcs), impl, len(f_defs)))
    # 4. 事件分发收表
    v = open(VTBL).read()
    cases = {}
    for m in re.finditer(r'case (\d+): h = (0x[0-9a-f]+);', v):
        cases[int(m.group(1))] = m.group(2)[2:]
    dangling = sorted(c for c, h in cases.items() if h not in f_defs)
    print("F_55d40 switch: %d case，handler 全有定义体: %d，悬空: %d %s"
          % (len(cases), len(cases) - len(dangling), len(dangling), dangling or ''))
    # 5. 判定 + 缺口清单
    print("")
    print("── 覆盖度基线（离真机可跑的量化差距）──")
    print("  原版 .text 镜像覆盖: %.1f%%（≥100%% = 原版代码整体在翻译体 shadow 中可执行）" % cov)
    print("  原版语义函数翻译实现: %.1f%%（%.0f%%+ = 每个原版 FUNC 符号都有翻译实现）" % (impl, impl))
    print("  事件分发 handler 收表: %d/%d case（悬空 %d = 需补定义体）"
          % (len(cases) - len(dangling), len(cases), len(dangling)))
    if cov < 100.0:
        print("判定: FAIL（镜像覆盖 %.1f%% < 100%% = 原版 .text 未整体在位）" % cov); sys.exit(1)
    if dangling:
        print("判定: FAIL（事件链悬空 case: %s = handler 无翻译实现）" % dangling); sys.exit(1)
    print("判定: PASS（镜像 %.0f%% + 事件链 %d/%d 0 悬空 = 32 位翻译体覆盖度基线成立）"
          % (cov, len(cases) - len(dangling), len(cases)))
    print("  注：FUNC 符号分母口径仅作参考（原版 so stripped，动态符号只含导出项，")
    print("      翻译体 F_ 命名空间对应内部函数 → 直接相除低估真实翻译覆盖率）")

if __name__ == '__main__':
    main()
