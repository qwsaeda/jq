#!/usr/bin/env python3
"""gen_onevent_full.py — 生成 jqt_OnEvent_full.c（自包含版，§124aj 固化）

用途：从原版 32 位 libjunqi.so 机械翻译 F_bd914（OnEvent 路径）+
      F_bdd2c（OnTouch 路径）+ 缺失 callee，生成编译版 C 文件。

流程（全自动）：
  1. 用 thumb2c_full.gen_c_file_full 生成 bd914/bdd2c 完整调用链
  2. 提取缺失函数（编译版未定义的）
  3. 修复 blx 存根：vtable dispatch → jqt_blx_dispatch；
     0x233ac(__aeabi_f2iz) → (int)(float) 真实；0x22e60(__stack_chk_fail noreturn) → 存根
  4. 移除函数体 #define MEM（改用 jqt_mem.h 的 MEM，jqt_mem_addr 加 g_jqt_shadow 偏移）
  5. 写 rev64/src_jni_translated/jqt_OnEvent_full.c

用法: python3 gen_onevent_full.py [--workdir /disk/workspaces/jq]
环境依赖: /tmp/opencode/thumb2c_full.py（§124ai 三修复版）,
          rev64/analysis/func_boundaries.txt（函数边界）
"""
import re, sys, os, glob

# 路径（默认工作区）
ROOT = '/disk/workspaces/jq'
if '--workdir' in sys.argv:
    ROOT = sys.argv[sys.argv.index('--workdir')+1]
THUMB2C = '/tmp/opencode/thumb2c_full.py'
BOUNDARIES = os.path.join(ROOT, 'rev64/analysis/func_boundaries.txt')
JTI = os.path.join(ROOT, 'rev64/src_jni_translated')

def gen_chain(funcs, max_funcs=30):
    """用 thumb2c_full 生成完整调用链。返回 {addr: body}"""
    sys.path.insert(0, os.path.dirname(THUMB2C))
    from thumb2c_full import load_insns, gen_c_file_full
    insns = load_insns()
    entries = []
    for line in open(BOUNDARIES):
        p = line.split()
        if p:
            try: entries.append(int(p[0], 16))
            except: pass
    entries = sorted(set(entries))
    funcs_map = {}
    for a in funcs:
        c = gen_c_file_full(insns, entries, [a], max_funcs=max_funcs)
        for m in re.finditer(r'uint32_t F_([0-9a-f]+)\(uint32_t _r0', c):
            fa = int(m.group(1), 16)
            i = m.start()
            m2 = re.search(r'\nuint32_t F_[0-9a-f]+\(uint32_t', c[i+10:])
            j = i+10+m2.start() if m2 else len(c)
            funcs_map[fa] = c[i:j]
    return funcs_map

def fix_stubs(body, fn_addr=None):
    """修复 blx 存根"""
    lines = body.split('\n')
    out = []
    for line in lines:
        # ★ §124ak：F_bdd2c Lbdd48 = GetObjectClass（JNIEnv vtable[31]，真实 JNI 函数）。
        #   特判替换（jqt_blx_dispatch 只查原版 dispatch 表，真实函数指针不在表内 → 0）。
        if fn_addr == 0xbdd2c:
            m = re.match(r'^(Lbdd48): r0=jqt_blx_dispatch\(r3,.*$', line.strip())
            if m:
                out.append(f"{m.group(1)}: r0=jqt_jni_getobjectclass(r0, r1); /* §124ak GetObjectClass(env,obj) 真实调用 */")
                continue
            # ★ §124ak：Lbdd60/Lbdd84 = GetMethodID（JNIEnv functions[33]）。
            m = re.match(r'^(Lbdd(?:60|84)): r0=jqt_blx_dispatch\(r[0-9]+,.*$', line.strip())
            if m:
                out.append(f"{m.group(1)}: r0=jqt_jni_getmethodid(r0, r1, r2, r3); /* §124ak GetMethodID(env,jclass,name,sig) 真实调用 */")
                continue
            # ★ §124ak：Lbddc6 = DeleteLocalRef（JNIEnv functions[23]）。
            m = re.match(r'^(Lbddc6): r0=jqt_blx_dispatch\(r[0-9]+,.*$', line.strip())
            if m:
                out.append(f"{m.group(1)}: r0=jqt_jni_deletelocalref(r0, r1); /* §124ak DeleteLocalRef(env,obj) 真实调用 */")
                continue
        m = re.match(r'^(L[0-9a-f]+): r0=0; /\* blx (r[0-9]+) 存根 \*/$', line.strip())
        if m:
            lbl, reg = m.group(1), m.group(2)
            args = ', '.join(['r0','r1','r2','r3','r4','r5','r6','r7','r8','r9','r10','r11','sp'])
            out.append(f"{lbl}: r0=jqt_blx_dispatch({reg}, {args}); /* vtable dispatch */")
            continue
        m = re.match(r'^(L[0-9a-f]+): r0=0; /\* blx 0x233ac 存根 \*/$', line.strip())
        if m:
            lbl = m.group(1)
            out.append(f"{lbl}: r0=(uint32_t)(int)(*(float*)&r0); /* __aeabi_f2iz 真实 */")
            continue
        m = re.match(r'^(L[0-9a-f]+): r0=0; /\* blx 0x22e60 存根 \*/$', line.strip())
        if m:
            out.append(line)  # noreturn，保持存根
            continue
        out.append(line)
    return '\n'.join(out)

def remove_mem_define(body):
    """移除函数体 #define MEM（改用 jqt_mem.h 的 MEM）"""
    old = '''    #define MEM(addr) (*(uint32_t*)((uint8_t*)(addr)))
    #define MEMH(addr) (*(uint16_t*)((uint8_t*)(addr)))
    #define MEMB(addr) (*(uint8_t*)((uint8_t*)(addr)))'''
    new = '''    /* §124ai：函数体 #define MEM 移除，用 jqt_mem.h MEM（jqt_mem_addr 加偏移） */'''
    return body.replace(old, new)

def add_entry_log(body, fn_addr):
    """给 F_bd914/F_bdd2c 加入口诊断日志（§124aj，触摸路径定位用）"""
    if fn_addr not in (0xbd914, 0xbdd2c):
        return body
    marker = 'int Z=0, C=0, N=0, V=0;'
    j = body.find(marker)
    if j < 0:
        return body
    insert_pos = j + len(marker)
    log_line = ('\n    __android_log_print(4, "junqi", "F_%x: 入口 r0=0x%%08x r1=0x%%08x '
                 'r2=0x%%08x r3=0x%%08x", _r0,_r1,_r2,_r3);' % fn_addr)
    return body[:insert_pos] + log_line + body[insert_pos:]

def main():
    # 1. 生成两个链
    print('[1/5] 生成 F_bd914/F_bdd2c 完整链...')
    funcs = gen_chain([0xbd914, 0xbdd2c])
    print(f'      共 {len(funcs)} 函数')
    # 2. 编译版已有函数
    print('[2/5] 检查编译版已有函数...')
    compiled_existing = set()
    for f in glob.glob(os.path.join(JTI, '*.c')):
        if os.path.basename(f) == 'jqt_OnEvent_full.c':
            continue  # §124aj：排除自身（本脚本生成的目标文件）
        txt = open(f).read()
        for m in re.finditer(r'uint32_t F_([0-9a-f]+)\(uint32_t _r0', txt):
            compiled_existing.add(int(m.group(1), 16))
    # 3. 缺失函数
    need = sorted(a for a in funcs if a not in compiled_existing)
    print(f'[3/5] 缺失函数（需生成）: {[hex(a) for a in need]}')
    # 4. 修复存根 + 移除 MEM 定义 + 入口日志
    print('[4/5] 修复存根 + 移除 MEM 定义 + 入口日志...')
    sig = 'uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t'
    decls = [f'uint32_t F_{a:x}({sig});' for a in need]
    bodies = []
    for a in need:
        body = fix_stubs(funcs[a], a)
        body = remove_mem_define(body)
        body = add_entry_log(body, a)
        bodies.append(body)
    # 5. 写文件
    print('[5/5] 写 jqt_OnEvent_full.c...')
    hdr = '''/* jqt_OnEvent_full.c — 触摸→对局路径（F_bd914/F_bdd2c + 缺失 callee）
 * 生成自 gen_onevent_full.py（§124aj 自包含版，thumb2c_full §124ai 三修复）
 * 修复：blx rX vtable dispatch → jqt_blx_dispatch；blx 0x233ac → __aeabi_f2iz 真实；
 *       blx 0x22e60 __stack_chk_fail noreturn → 保持存根。
 * jqt_blx_dispatch 定义在 jqt_vtbl.c（vtable 槽值→目标函数）。 */
#include <stdint.h>
#include <android/log.h>
#include <stddef.h>
#include "jqt_mem.h"
extern uint32_t jqt_jni_getobjectclass(uint32_t, uint32_t);  /* §124ak F_bdd2c GetObjectClass */
extern uint32_t jqt_jni_getmethodid(uint32_t, uint32_t, uint32_t, uint32_t);  /* §124ak F_bdd2c GetMethodID */
extern uint32_t jqt_jni_deletelocalref(uint32_t, uint32_t);  /* §124ak F_bdd2c DeleteLocalRef */
'''
    out = hdr + '\n'.join(decls) + '\n\n' + '\n'.join(bodies)
    dst = os.path.join(JTI, 'jqt_OnEvent_full.c')
    open(dst, 'w').write(out)
    print(f'      写入 {dst} ({len(out)}B)')

if __name__ == '__main__':
    main()
