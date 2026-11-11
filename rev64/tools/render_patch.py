#!/usr/bin/env python3
"""render_patch.py — jqt_OnRender_full.c 渲染链补丁机械化（幂等，防手工重复）
用法: python3 rev64/tools/render_patch.py
作用:
  1. Lbf51c vtable 检查豁免（§sp27）：r5==MEM(0x162400) 时不判"无效"——
     旧检查 `MEM(r5)!=0x1595c0 字面量` 误杀已构造的渲染器单例自身
     （RELATIVE 重定位后 vtable 槽值 = shadow+0x1595c0 绝对地址，
       MEM(r5) 读出的不是 0x1595c0 字面量 → 误判"无效"→ 回退单例（其实回退值就是自己））
     修法：引入 _rs=MEM(0x162400)，r5==_rs 时豁免
  2. brace 完整性检查
  全部幂等：已注入则跳过。
"""
import re, sys

PATH = 'rev64/reverse/src_jni_translated/jqt_OnRender_full.c'

# ============ 补丁 1: Lbf51c vtable 检查豁免 ============
LB51C_OLD = ("if (!(r5 >= 0xa0000000u && r5 < 0xf0000000u) || "
             "(MEM(r5) != 0x1595c0u && MEM(r5) != (uint32_t)(g_jqt_shadow + 0x1595c0))) { "
             "r5 = MEM(0x162400); __android_log_print(6,\"junqi\","
             "\"F_bf4d0 Lbf51c r5=0x%08x vt=0x%08x 无效 回退渲染器单例=0x%08x\", r5, MEM(r5), MEM(0x162400)); }")
LB51C_NEW = ("{ uint32_t _rs = MEM(0x162400); if (!(r5 >= 0xa0000000u && r5 < 0xf0000000u) || "
             "(MEM(r5) != 0x1595c0u && MEM(r5) != (uint32_t)(g_jqt_shadow + 0x1595c0) && r5 != _rs)) { "
             "r5 = _rs; __android_log_print(6,\"junqi\","
             "\"F_bf4d0 Lbf51c r5=0x%08x vt=0x%08x 无效 回退渲染器单例=0x%08x\", r5, MEM(r5), _rs); } }")

def main():
    src = open(PATH).read()
    print("jqt_OnRender_full.c: %d bytes" % len(src))

    # 补丁 1: Lbf51c 豁免
    if '_rs' in src and 'r5 != _rs' in src:
        print("补丁1 Lbf51c 豁免: 已注入（跳过）")
    elif LB51C_OLD in src:
        src = src.replace(LB51C_OLD, LB51C_NEW, 1)
        print("补丁1 Lbf51c vtable 检查豁免: 注入成功")
    else:
        print("补丁1 警告: Lbf51c 旧行未找到（可能已改过或行漂移）")

    # brace 完整性检查
    depth = 0; neg = 0
    for l in src.split('\n'):
        depth += l.count('{') - l.count('}')
        if depth < 0:
            neg += 1; depth = 0
    if neg:
        print("brace 检查: 负深度 %d 处（需人工检查）" % neg)
        sys.exit(1)
    print("brace 检查: 全部通过")

    open(PATH, 'w').write(src)
    print("已写 %s (%d bytes)" % (PATH, len(src)))

if __name__ == '__main__':
    main()
