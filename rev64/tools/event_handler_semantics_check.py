#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1919 事件 handler 忠实语义离线核查判据（设备无关）

固化 §124y-29/32/33 实锤：24 事件 handler 的"返回 2 兜底"口径——
  - F_8571c = getter（vtable 0x94d80：0x38!=0 → 底层+0x70，0x38=0 → ret=0，
    **非 2 兜底**，§124y-32 忠实修正：原版 code4 在 F_3ab64 层 ok=0）
  - 其余 22 handler（F_85704/F_33878/F_55dc4/F_55ba4/F_3deb0/F_853c0/
    F_3f8f4 等）= "return 0 忠实"（.bss 未构造时 0x38=0 → 早返回 0，
    非 2 兜底）
  - 24 handler 中 **0 条** "return 2 兜底"（"让主链 ok=1 走通"的设计
    被 §124y-32/33 忠实修正推翻，改"return 0 忠实"）
判据：扫 24 handler 函数体，统计 return 2 / return 0 / getter（非 0/2），
输出"return 2 兜底 = 0（§124y-32/33 忠实修正后口径）+ 24 handler 全
有定义体（0 悬空）"。

用法：python3 rev64/tools/event_handler_semantics_check.py
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
VTBL = os.path.join(ROOT, 'rev64/reverse/src_jni_translated/jqt_vtbl.c')

# §124y-29/30 实锤 24 事件 handler（codes 0-41 → 24 handler，0x55d40 区 switch）
HANDLERS = [
    'F_8571c', 'F_85704', 'F_33878', 'F_55dc4', 'F_55ba4',
    'F_3deb0', 'F_853c0', 'F_3f8f4', 'F_27a6c', 'F_953cc',
    'F_91d18', 'F_91774', 'F_55d74', 'F_55fbc', 'F_9411c',
    'F_33724', 'F_85730', 'F_85744', 'F_85758', 'F_85798',
    'F_857b0', 'F_85854', 'F_85858', 'F_859a4',
]

def main():
    src = open(VTBL, 'rb').read().decode('utf-8', errors='replace')
    ret2 = ret0 = getter = missing = 0
    detail = []
    for h in HANDLERS:
        # 体提取：从 '{' 起到下一个 '\nuint32_t F_' 或 '\n}'（贪婪，含嵌套）
        i = src.find('uint32_t ' + h + '(uint32_t _r0')
        if i < 0:
            missing += 1
            detail.append((h, 'MISSING'))
            continue
        brace = src.find('{', i)
        nxt = src.find('\nuint32_t F_', brace)
        nxt2 = src.find('\n}', brace)
        end = min(x for x in (nxt, nxt2) if x > 0)
        body = src[brace:end]
        rets = re.findall(r'return\s+(.+?);', body)
        last = rets[-1] if rets else None
        if re.match(r'^2\b', last or ''):
            ret2 += 1
            detail.append((h, 'return 2 兜底'))
        elif last is None or (last or '').strip() in ('0', '_r0', '_r1', '_r2', '_r4', 'r', 'ret', 'r0', 'r4', 'thisp'):
            # 隐式 0 / 显式 return 0 / return 入参·局部变量（.bss 未构造时全 0）均归
            # "return 0 忠实"（§124y-32/33 忠实修正：handler 真语义 = .bss 未构造早返回 0）
            ret0 += 1
            detail.append((h, 'return 0（%s 忠实）' % ((last or '隐式').strip() if last else '隐式')))
        else:
            getter += 1
            detail.append((h, 'getter/复杂（return %s）' % (last or '?')))
    ok = (missing == 0 and ret2 == 0)  # 24 全有定义体 + 0 "return 2 兜底"
    print('═══ §124y-1919 事件 handler 忠实语义离线核查判据 ═══')
    print('  24 事件 handler（§124y-29/30 实锤 codes 0-41 → 24 handler）')
    print('  定义体齐全: %d/24（缺 %d）' % (24 - missing, missing))
    print('  return 2 兜底 = %d（§124y-32/33 忠实修正后应 = 0，原版 code4 在 F_3ab64 层 ok=0）' % ret2)
    print('  return 0 忠实 = %d（显式 return 0 / 隐式 void 体 0，.bss 未构造早返回 0）' % ret0)
    print('  getter/复杂 = %d（F_8571c 型：0x38!=0 → 底层+0x70 getter / jqt_recreate_board_texture 等）' % getter)
    for h, k in detail:
        if k not in ('return 0（显式/隐式）',):
            print('    %s → %s' % (h, k))
    print('  判据: %s' % ('PASS（24 handler 定义体齐全 + 0 return 2 兜底，忠实语义收口）'
                          if ok else 'FAIL'))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
