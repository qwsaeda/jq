// §124y-142: F_bd870 (OnInput 内部处理函数) 对齐原版 0xbd870-0xbd8f0 完整体
// (照 F_bde34 风格: 逐标签 MEM + JNI 函数显式调用 + .got 表基址 + blx 虚调用 + F_bd778 按键事件写入)
//
// 原版 .text 0xbd870 (OnInput 内部处理, JNI 入口 0x233f4 bl 0xbd870):
//   入参: r0=env(JNIEnv*), r1=env(adds r1,r0,#0), r2=jstring (输入文本)
//   流程:
//     0xbd878 ldr r3,[pc,#124]→.got[0x15b488]=.bss 0x15be80 根对象 (OnInput 表基址 scudo)
//     0xbd87c ldr r2,[pc] = 0x3c0 (表偏移索引 0x3c0/4=240 槽)
//     0xbd882 ldr r5,[r3,#0x3c0] (.got 表 240 槽 = .bss 单例指针)
//     0xbd888 ldr r3,[r5,#0] (scudo 块 [0] = C++ 对象 vtable/首字段)
//     0xbd890 ldr r2,[r1,#0] (env[0] = JNIEnv 函数表)
//     0xbd892 movs r3,#169; 0xbd894 lsls r3,#2 (169<<2=676, 字节偏移索引)
//     0xbd896 ldr r3,[r2,#676] (JNIEnv functions[676/4=169]=GetStringUTFChars 区)
//     0xbd8a0 blx r3 (JNI 虚调用 = GetStringUTFChars(jstring) → char*)
//     0xbd8a4 movs r2,#128; 0xbd8a8 lsls r2,#7 (128<<7=16384, C++ 对象字段索引)
//     0xbd8ae bl 0xc2d24 (F_c2d24 已收表, C++ 缓冲区函数)
//     0xbd8b2 ldr r2,[r4,#0] (重新读 [r4] = vtable 基址)
//     0xbd8b4 movs r3,#170; 0xbd8b6 lsls r3,#2 (170<<2=680)
//     0xbd8b8 ldr r3,[r2,#680] (vtable[680/4=170] = 子类方法指针)
//     0xbd8c0 blx r3 (JNI 虚调用 = 第 2 次 GetMethodID 区)
//     0xbd8c2 ldr r3,[pc]→.bss 0x162660 (OnInput C++ 对象区, .bss 偏移 0x67E0)
//     0xbd8c8 bl 0xc6088 (字符串处理函数, 未收表, 存根)
//     0xbd8ce ldr r3,[pc] (栈金丝雀)
//     0xbd8d6 bl 0xbd778 (F_bd778 按键事件写入, jqt_OnEvent_full.c 定义体)
//     0xbd8da ldr r3,[pc] (栈金丝雀恢复)
//     0xbd8de ldr r2,[sp,#0] (栈金丝雀比对)
//     0xbd8e0 ldr r3,[r5,#0] (原 scudo 块 [0])
//     0xbd8e2 cmp r2,r3 (金丝雀校验)
//     0xbd8e4 bne 0xbd8f2 (__stack_chk_fail, 栈溢出保护)
//   返回: 0 (void)
//
// 诚实边界:
//   - .got[0x15b488]=.bss 0x15be80 根对象 scudo 指针 (翻译体 .bss 全 0 → 表 [0x3c0]=0 → 忠实兜底)
//   - JNI 函数 GetStringUTFChars (functions[169]) 走 jqt_jni_* 真实调用 (§124ak)
//   - 0xc6088 字符串处理函数 (bl strlen + 缓冲区构造) 未收表, 存根 r0=0
//   - 0xbd778 = F_bd778 按键事件写入 (jqt_OnEvent_full.c:120 完整体, §124y-140 已对齐)
#include <stdint.h>
#include "jqt_mem.h"
#include <android/log.h>
/* F_bd870 用到的 extern 声明 */
extern uint32_t F_c2d24(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t F_c6088(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t F_bd778(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t jqt_jni_getstringutfchars(uint32_t env, uint32_t str);
extern uint32_t jqt_blx_dispatch(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_bd870(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);

uint32_t F_bd870(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    __android_log_print(4, "junqi", "F_bd870: 入口 r0(env)=0x%08x r1=0x%08x r2(jstring)=0x%08x", _r0,_r1,_r2);
Lbd870: r7=_r9; /* mov r7,r9 */
Lbd874: r6=_r8; /* mov r6,r8 */
Lbd876: sp-=(2*4); /* push {r6, r7} */
Lbd876_p0: MEM(sp+0)=r6;
Lbd876_p1: MEM(sp+4)=r7;
Lbd878: r3=MEM(0x15b0c8);       /* ldr r3,[pc] → .got[0x15b488] = .bss 0x15be80 根对象 scudo 指针 */
Lbd87a: {uint32_t a_=r2,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lbd87c: r2=0x3c0;              /* ldr r2,[pc] = 表偏移索引 0x3c0/4=240 槽 */
Lbd87e: r3=(0x15b0c8+r3);      /* add r3,pc → .got 基址 */
Lbd880: r4=MEM(sp);            /* sp 偏移 (栈金丝雀区) */
Lbd882: r5=MEM(r3+0x3c0);      /* ldr r5,[r3,#0x3c0] = .got 表 240 槽 (.bss 单例指针) */
Lbd884: r2=0x400c;             /* ldr r2,[pc] = 第 2 偏移 (栈金丝雀 0x400c) */
Lbd886: sp=sp+r4;              /* add sp,r4 (栈金丝雀对齐) */
Lbd888: r3=MEM(r5);            /* ldr r3,[r5,#0] = scudo 块 [0] */
Lbd88a: r2=(0x400c+r2);        /* add r2,pc */
Lbd88c: {uint32_t a_=r1,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lbd88e: MEM(sp)=r3;            /* str r3,[sp,#0] = 栈金丝雀写入 */
Lbd890: r2=MEM(r1);            /* ldr r2,[r1,#0] = env[0] = JNIEnv 函数表 */
Lbd892: r3=169;
Lbd894: r3=(r3<<2);            /* lsls r3,#2 → 169<<2=676 (字节偏移索引) */
Lbd896: r3=MEM(r2+r3);         /* ldr r3,[r2,#676] = JNIEnv functions[169] (GetStringUTFChars 区) */
Lbd898: r9=r0;                 /* mov r9,r0 (env 保存) */
Lbd89a: r2=0;                  /* movs r2,#0 */
Lbd89c: {uint32_t a_=r1,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbd89e: {uint32_t a_=r6,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbd8a0: r0=jqt_jni_getstringutfchars(r9, r0); /* blx r3 = GetStringUTFChars (§124y-142, jstring → char*) */
Lbd8a2: r3=sp+12;              /* add r3,sp,#12 */
Lbd8a4: r2=128;
Lbd8a6: {uint32_t a_=r3,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbd8a8: r2=(r2<<7);            /* lsls r2,#7 → 128<<7=16384 (C++ 对象字段索引) */
Lbd8aa: r8=r3;                 /* mov r8,r3 */
Lbd8ac: {uint32_t a_=r0,b_=0; r7=(a_+b_); Z=(r7==0); C=(r7<a_); N=((r7)>>31)&1; V=(((a_^r7)&(b_^r7))>>31)&1;}
Lbd8ae: r0=F_c2d24(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xc2d24 (F_c2d24 已收表) */
Lbd8b2: r2=MEM(r4);            /* ldr r2,[r4,#0] = 重新读 [r4] = vtable 基址 */
Lbd8b4: r3=170;
Lbd8b6: r3=(r3<<2);            /* lsls r3,#2 → 170<<2=680 */
Lbd8b8: r3=MEM(r2+r3);         /* ldr r3,[r2,#680] = vtable[170] 子类方法指针 */
Lbd8ba: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbd8bc: {uint32_t a_=r7,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbd8be: {uint32_t a_=r6,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbd8c0: r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* blx r3 = 第 2 次 JNI 虚调用 (GetMethodID 区) */
Lbd8c2: r3=MEM(0x162660);     /* ldr r3,[pc] → .bss 0x162660 (OnInput C++ 对象区, 偏移 0x67E0) */
Lbd8c4: r1=r8;               /* mov r1,r8 */
Lbd8c6: r3=(0x162660+r3);     /* add r3,pc → .bss 偏移 */
Lbd8c8: r0=MEM(r3);           /* ldr r0,[r3,#0] = .bss 0x162660 区 [0] */
Lbd8ca: r0=F_c6088(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xc6088 (字符串处理, §124y-144 完整体: strlen+F_23bec) */
Lbd8ce: r3=MEM(sp);            /* ldr r3,[pc] = 栈金丝雀 */
Lbd8d0: r0=r9;               /* mov r0,r9 = env */
Lbd8d2: r1=sp;               /* mov r1,sp */
Lbd8d4: MEM(sp)=r3;            /* str r3,[sp,#0] = 栈金丝雀比对 */
Lbd8d6: r0=F_bd778(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd778 (按键事件写入, §124y-140) */
Lbd8da: r3=MEM(sp);            /* ldr r3,[pc] = 栈金丝雀恢复 */
Lbd8dc: r3=(0x400c+r3);        /* add r3,pc */
Lbd8de: r2=MEM(r3);            /* ldr r2,[r3,#0] */
Lbd8e0: r3=MEM(r5);            /* ldr r3,[r5,#0] = 原 scudo 块 [0] */
Lbd8e2: {uint32_t a_=r2,b_=r3; Z=(r2==b_); C=(r2<b_); N=((a_^b_)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
if(Z==0){ /* 栈金丝雀校验失败 → __stack_chk_fail */
Lbd8f2: r0=0; goto RET;       /* blx __stack_chk_fail (存根, .bss 未构造忠实兜底) */
}
Lbd8ea: r4=MEM(sp+0); r5=MEM(sp+4); /* pop {r2, r3} + 恢复 */
Lbd8ec: r8=r4; r9=r5;
Lbd8f0: sp+=(2*4);
Lbd8f0_r: r0=0; goto RET;
RET: return r0;
}

/* §124y-144: F_c6088 (字符串处理函数) 完整体对齐 (原版 0xc6088-0xc60a0)
 * 入参: r0=目标 buffer 指针, r1=源字符串 (char*)
 * 流程: blx strlen(r1) → len; r5=r0+0x50; r2=r1+len; r1=str; r0=r5; bl F_23bec(已收表, jqt_OnCreate_full.c:13973)
 * 调用 F_23bec(buf+0x50, str, str+len, ...) 构造字符串缓冲区 (事件对象 [0x50] 区)。 */
extern uint32_t F_23bec(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
#include <string.h>
uint32_t F_c6088(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
Lc6088: r4=_r4; /* push {r4, r5, r6, lr} */
Lc6088_p0: MEM(sp+0)=r4;
Lc608a: {uint32_t a_=r0,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lc608c: {uint32_t a_=r1,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc608e: {uint32_t a_=r1,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lc6090: { /* blx strlen@plt: 算 strlen(r1=str 指针) */
    uint32_t _len = 0;
    if (r1 != 0) {
        volatile const char *_ps = (volatile const char *)(uintptr_t)JQT_ABS_PTR(r1);
        if (_ps) { while (_ps[_len]) _len++; }
    }
    r0 = _len;
}
Lc6094: r5=(r5+0x50); /* adds r5,#80 */
Lc6096: r2=(r4+r0);   /* adds r2,r4,r0 (str+len) */
Lc6098: {uint32_t a_=r4,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lc609a: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc609c: r0=F_23bec(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x23bec (F_23bec 已收表) */
Lc60a0: r4=MEM(sp+0); /* pop {r4, lr} */
Lc60a0_r: goto RET;
RET: return r0;
}

/* jqt_OnInput 入口 (JNI 入口真调 F_bd870, 非存根) */
extern uint32_t F_bd870(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t jqt_OnInput(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);

uint32_t jqt_OnInput(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
L233f6: r3=MEM(144376+12);
L233f8: {uint32_t a_=r0,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L233fa: r3=(144380+r3);
L233fc: r0=MEM(r3);
L233fe: r0=F_bd870(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd870 完整 (§124y-142, 非存根) */
L23402_l0: r4=MEM(sp+0);
L23402: sp+=(1*4); /* pop {r4, pc} */
L23402_r: goto RET;
RET: return r0;
}
