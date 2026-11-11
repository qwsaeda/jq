#!/usr/bin/env python3
"""patch_gate_chain.py — gate-set 触发链机械化补丁（幂等，防手改丢失）

本轮（§〇d 后续）实证沉淀：
  1. gate-set：OnCreate 置位 0x15be90 闸口 byte[0x51]=1 byte[0x41]=1 [0x30]=10
     → F_76b14 走 F_76a98 → F_782d4（r1==10 通过）→ F_7876c（类 B 构造）
  2. SLOT2：F_782d4 的 slot2 dispatch（F_78890 翻译缺口）失败 → 继续 L7830e 链
     （F_777f8/F_7809c/F_bd530...），L783ce 返回类 B this → [0x3c]=classB
     → F_76b14 [0x3c]!=0 路径 → F_77904/F_77a04 执行
  3. F_7880c 内部插桩 3 点（slot31/slot33/F_77840 前）——定位 F_7880c 崩点

用法: python3 rev64/tools/patch_gate_chain.py   # 幂等注入全部补丁
退出码: 0=全部已就位 / 2=某补丁缺失（注入失败）
"""
import re, sys

V = 'rev64/reverse/src_jni_translated/jqt_vtbl.c'
J = 'rev64/reverse/src_jni_translated/jni_aligned.c'

def patch_vtbl():
    src = open(V).read()
    n = 0
    # ---- 1. F_782d4 L7830c SLOT2：slot2 dispatch 失败 → 走 L783d2（slot1→F_7880c→F_77840→slot22）----
    old = 'L7830c: if(Z) goto L783d2;'
    new = ('L7830c: if(Z) { __android_log_print(6,"junqi",'
           '"F_782d4 L7830c SLOT2 缺口：走 L783d2（slot1 dispatch F_7887c→F_7880c→F_77840→slot22），类 B r5=0x%08x 已构造", r5); goto L783d2; } '
           '/* §SLOT2：slot2 dispatch(F_78890 缺失)失败，走 L783d2（F_7887c→F_7880c→F_77840→slot22 触发），L783e0 返回类 B this */')
    if src.count(old) == 1:
        src = src.replace(old, new); n += 1
    elif 'SLOT2 缺口' in src:
        pass  # 已注入
    else:
        print('FATAL: F_782d4 L7830c 未找到', file=sys.stderr); return None

    # ---- 1b. F_782d4 L783e0：slot1 dispatch 后返回类 B this（[0x3c]=classB）----
    old = 'L783e0: r0=0;'
    new = ('L783e0: r0=MEM(sp+20); /* §L783e0：slot1 dispatch（F_7887c→F_7880c→F_77840→slot22）'
           '后返回类 B this，→ [0x3c]=classB */')
    if src.count(old) == 1:
        src = src.replace(old, new); n += 1
    elif '§L783e0' in src:
        pass  # 已注入
    else:
        print('FATAL: F_782d4 L783e0 未找到', file=sys.stderr); return None

    # ---- 2. F_7880c 内部插桩：slot31 dispatch 前 ----
    old = ('L7882e: r3=MEM(r3+124);\n'
           'L78830: { r0=jqt_blx_dispatch(r3')
    new = ('L7882e: r3=MEM(r3+124);\n'
           'L78830: { __android_log_print(6,"junqi","F_7880c slot31 dispatch: this=0x%08x [8]=0x%08x slot31=0x%08x", r4, MEM(r4+8), r3); r0=jqt_blx_dispatch(r3')
    if src.count(old) == 1:
        src = src.replace(old, new); n += 1
    elif 'F_7880c slot31 dispatch' in src:
        pass
    else:
        print('FATAL: F_7880c slot31 插桩点未找到', file=sys.stderr); return None

    # ---- 3. F_7880c 内部插桩：slot33 dispatch 前 ----
    old = ('L7883e: r7=MEM(r0+r1);\n'
           'L78840: r3=(493636+r3);')
    new = ('L7883e: r7=MEM(r0+r1);\n'
           'L78840: { __android_log_print(6,"junqi","F_7880c slot33 dispatch: vtable=0x%08x slot33=0x%08x r6=0x%08x", r0, r7, r6); r3=(493636+r3);}')
    if src.count(old) == 1:
        src = src.replace(old, new); n += 1
    elif 'F_7880c slot33 dispatch' in src:
        pass
    else:
        print('FATAL: F_7880c slot33 插桩点未找到', file=sys.stderr); return None

    # ---- 4. F_7880c 内部插桩：F_77840 调用前 ----
    old = ('L78862: r0=F_77840(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x77840 完整 */')
    new = ('L78862: { __android_log_print(6,"junqi","F_7880c → bl F_77840（slot22 dispatch 点）：this=0x%08x", r4); r0=F_77840(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* bl 0x77840 完整 */')
    if src.count(old) == 1:
        src = src.replace(old, new); n += 1
    elif 'F_7880c → bl F_77840' in src:
        pass
    else:
        print('FATAL: F_7880c F_77840 调用点未找到', file=sys.stderr); return None

    open(V, 'w').write(src)
    return n


def patch_jni():
    src = open(J).read()
    n = 0
    # ---- 5. jni_aligned.c TRIGGER-GATE：置位 0x15be90 闸口 ----
    old = '    __android_log_print(4, "junqi", "OnCreate: 开始 F_23560 完整调用链");'
    new = '''    /* ★ §TRIGGER-GATE：直接置位 0x15be90 对象 F_76b14 早退闸口字节
     *   （byte[0x51]=1 + byte[0x41]=1，[0x3c]=0 已是），让 F_76b14 走
     *   F_76a98（类B构造→F_77b10→slot22→F_9e2b4 纹理创建）。
     *   F_76a98→F_782d4 的 r1 = [0x15be90+0x30]，F_782d4 要求 r1==10
     *   （cmp r1,#0xa 相等才 _Znwj(12)+F_7876c 类B构造），故 [0x30]=10。 */
    {
      uint32_t v90 = *(uint32_t*)(g_jqt_shadow + 0x15be90);
      if (v90) {
        MEMB(v90 + 0x51) = 1;   /* F_76b14 gate: [0x51]!=0 → 清 0 后走 [0x41] 分支 */
        MEMB(v90 + 0x41) = 1;   /* F_76b14 gate: byte[0x41]!=0 → F_76a98 */
        MEM(v90 + 0x30) = 10;   /* F_782d4 gate: r1=[0x30] 必须 ==10 才构造类B */
        __android_log_print(4, "junqi", "OnCreate: 置位 0x15be90 闸口 byte[0x51]=1 byte[0x41]=1 [0x30]=10 [0x3c]=0x%08x",
            v90 ? MEM(v90+0x3c) : 0);
      } else {
        __android_log_print(4, "junqi", "OnCreate: 0x15be90 未构造，跳过闸口置位");
      }
    }
'''
    if '§TRIGGER-GATE' in src:
        pass  # 已注入
    elif src.count(old) == 1:
        src = src.replace(old, new + old); n += 1
    else:
        print('FATAL: jni_aligned.c TRIGGER-GATE 锚点未找到', file=sys.stderr); return None

    # ---- 6. jni_aligned.c SLOT22-TRIGGER：OnUpdate 一次性 F_9e2b4 纹理创建触发 ----
    old = '    return (jboolean)F_23475(0,0,0,0,0,0,0,0,0,0,0,0,0x20f000);'
    new = '''    /* ★ §SLOT22-TRIGGER：一次性 slot22（F_9e2b4 纹理创建）触发（GLThread + 渲染器已构造
     *   + MEM 宏可用三条件）。F_9e2b4: this[76](texID)==0 → F_9e204（glGenTextures→texID
     *   + glTexImage2D 上传 jq_board_px 737×751）→ texID 离开 0。 */
    if (!g_slot22_triggered) {
        extern uint32_t F_9e2b4(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
        uint32_t ren = MEM(0x162400);
        if (ren) {
            g_slot22_triggered = 1;
            uint32_t before = MEM(ren+76);
            F_9e2b4(ren, 0,0,0,0,0,0,0,0,0,0,0, 0x20f000);
            __android_log_print(6,"junqi","SLOT22-TRIGGER: F_9e2b4(ren=0x%08x) texID before=0x%08x after=0x%08x this[76]=0x%08x px=0x%08x w=%d h=%d",
                ren, before, MEM(ren+76), MEM(ren+76), MEM(ren+28), MEM(ren+32), MEM(ren+36));
        } else {
            __android_log_print(6,"junqi","SLOT22-TRIGGER: 渲染器未构造（0x162400=0），等待...");
        }
    }
'''
    if '§SLOT22-TRIGGER' in src:
        pass  # 已注入
    elif src.count(old) == 1:
        src = src.replace(old, new + old); n += 1
    else:
        print('FATAL: jni_aligned.c SLOT22-TRIGGER 锚点未找到', file=sys.stderr); return None

    open(J, 'w').write(src)
    return n


def main():
    nv = patch_vtbl()
    if nv is None: sys.exit(2)
    nj = patch_jni()
    if nj is None: sys.exit(2)
    print('patch_gate_chain: vtbl=%d jni=%d（0=已全部就位，>0=本次注入数）' % (nv, nj))
    sys.exit(0)


if __name__ == '__main__':
    main()
