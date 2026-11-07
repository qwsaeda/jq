// 机械化翻译自原版 32 位 libjunqi.so @ 0x2344d (OnKey) — 6 标签
// bl/blx 存根 r0=0（与回归一致）；.rodata/.text 常量池需运行时明文注入
#include <stdint.h>
#include "jqt_mem.h"
#include <android/log.h>
/* §124y-140: F_bde34/F_bd778 用到的 extern 声明 (jqt_vtbl.c 定义, jqt_jnibr.c 定义) */
extern uint32_t F_7bf80(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t F_b7e00(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t jqt_jni_getobjectclass(uint32_t env, uint32_t obj);
extern uint32_t jqt_jni_getmethodid(uint32_t env, uint32_t cls, uint32_t name_off, uint32_t sig_off);
extern volatile uint32_t g_cur_fn;
extern uint32_t jqt_blx_dispatch(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t F_bde34(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
/* F_bd778 定义在 jqt_OnEvent_full.c (§124ai), 此处 extern 引用 */
extern uint32_t F_bd778(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t jqt_OnKey(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t jqt_OnKey(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
L2344e: r3=MEM(144464+12);
L23450: {uint32_t a_=r0,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L23452: r3=(144468+r3);
L23454: r0=MEM(r3);
L23456: r0=F_bde34(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbde34 完整 (§124y-140, 非存根) */
L2345a_l0: r4=MEM(sp+0);
L2345a: sp+=(1*4); /* pop {r4, pc} */
L2345a_r: goto RET;
RET: return r0;
}

/* §124y-140: F_bde34 (OnKey 内部处理函数) 对齐原版 0xbde34-0xbdf50 完整体
 * (照 F_bdd2c 风格: 逐标签 MEM + JNI 函数显式调用 + 键码匹配分支)
 *
 * 原版 .text 0xbde34 (OnKey 内部处理, JNI 入口 0x2344c bl 0xbde34):
 *   入参: r0=JNIEnv* (env), r1=KeyEvent* (JNI 对象), r2=mid? (第 3 参)
 *   流程:
 *     0xbde40 ldr r3,[r1,#0]        (KeyEvent 对象 [0] = 首字段, C++ 封装对象)
 *     0xbde46 ldr r3,[r3,#124=0x7c] (vtable 槽 31 = GetObjectClass 区, §124y-136/137)
 *     0xbde50 blx r3                (JNI 虚调用 = GetObjectClass, jqt_jni_getobjectclass)
 *     0xbde52 ldr r3,[r4,#0]        (r4=KeyEvent 对象, [0]=vtable 基址)
 *     0xbde54 movs r1,#132(0x84)    (vtable 槽 42 = 0x84/4, GetMethodID 区)
 *     0xbde58 ldr r3,[r3,r1]       (vtable[0x84] = 子类方法指针)
 *     0xbde5a ldr r2,[pc]          (GetMethodID name/sig .rodata 偏移)
 *     0xbde60 mov ip,r3
 *     0xbde6c blx ip               (JNI 虚调用 = GetMethodID, jqt_jni_getmethodid)
 *     0xbde74 bl 0x7bf80           (CallIntMethod, F_7bf80 已收表 §124y-97)
 *     0xbde8e bl 0x7bf80           (CallIntMethod, 第 2 次)
 *     0xbdeb4 bl 0x7bf80           (CallIntMethod, 第 3 次)
 *     0xbdec6-0xbdedc: 键码匹配分支 (.bss 0x16266c 单例槽区, ldr [r3+16/20/24/28/32/36/40/44] 比较)
 *       - r9 = 键码值 (KeyEvent.getCode() 返回, 0x7bf80 CallIntMethod)
 *       - 比较 .bss 0x16266c 区 [r3+16/20/24/28/32/36/40/44] (8 个键码常量槽, §124y-139 vtable 区)
 *       - 匹配 18/27/37/40/39/38 (str [r1,#4] 写, §124y-134 的 str 区) → bl 0xbd778 (按键事件写入)
 *       - 全不匹配 → 返回 r0=0
 *     0xbdf28 bl 0xbd778           (按键事件写入, F_bd778)
 *     返回: 匹配 r0=1 / 不匹配 r0=0
 *
 * 诚实边界:
 *   - .bss 0x16266c 键码常量槽 (.bss 偏移 0x67EC, §124y-139 区) MuMu 实锤非 0
 *     (dump[0x6804]=2/0x6808=0/0x680C=4/0x6810=1/0x6814=0x52/0x6820=0x13), 键码匹配分支照原版走
 *   - 0xbd778 按键事件写入函数 (F_bd778) 定义在 jqt_OnEvent_full.c (§124ai 完整体, 读 [r0+0x3c] 字节 + 条件调 F_b7e00),
 *     F_bde34 调用 F_bd778 真调 (非存根), 与原版 0xbdf28 bl 0xbd778 对齐
 *   - JNI 函数 (GetObjectClass/GetMethodID/CallIntMethod) 走 jqt_jni_* 真实调用 (§124ak 实锤) */

uint32_t F_bde34(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    __android_log_print(4, "junqi", "F_bde34: 入口 r0(env)=0x%08x r1(KeyEvent)=0x%08x r2=0x%08x", _r0,_r1,_r2);
Lbde34: sp-=(5*4); /* push r4, r5, r6, r7, lr */
Lbde34_p0: MEM(sp+0)=r4;
Lbde34_p1: MEM(sp+4)=r5;
Lbde34_p2: MEM(sp+8)=r6;
Lbde34_p3: MEM(sp+12)=r7;
Lbde36: r7=r11;
Lbde38: r6=r10;
Lbde3a: r5=r9;
Lbde3c: r4=r8;
Lbde3e: sp-=(4*4); /* push r4, r5, r6, r7 */
Lbde3e_p0: MEM(sp+0)=r4;
Lbde3e_p1: MEM(sp+4)=r5;
Lbde3e_p2: MEM(sp+8)=r6;
Lbde3e_p3: MEM(sp+12)=r7;
Lbde40: r3=MEM(r1);            /* KeyEvent 对象 [0] */
Lbde42: {uint32_t a_=r1,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lbde44: sp-=20;
Lbde46: r3=MEM(r3+124);        /* vtable 槽 31 (0x7c/4), §124y-136/137 输入事件基类 */
Lbde48: r11=r0;                /* fp=env (r0=JNI 参数 1 = env) */
Lbde4a: {uint32_t a_=r1,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbde4c: {uint32_t a_=r2,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbde4e: {uint32_t a_=r2,b_=0; r7=(a_+b_); Z=(r7==0); C=(r7<a_); N=((r7)>>31)&1; V=(((a_^r7)&(b_^r7))>>31)&1;}
Lbde50: r0=jqt_jni_getobjectclass(r11, r0); /* blx r3 = GetObjectClass (§124ak, 输入事件基类槽 31) */
Lbde52: r3=MEM(r4);            /* KeyEvent 对象 [0] = vtable 基址 */
Lbde54: r1=132;                /* movs r1,#132 (0x84, vtable 槽 42) */
Lbde56: r5=MEM(795212);        /* ldr r5,[pc,#244] → .rodata 0x1265c8 (GetMethodID name 偏移) */
Lbde58: r3=MEM(r3+r1);        /* vtable[0x84] = 子类方法指针 */
Lbde5a: r2=MEM(795216);        /* ldr r2,[pc,#244] → .rodata 0x128b08 (GetMethodID sig 偏移) */
Lbde5c: {uint32_t a_=r0,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lbde5e: r5=(0x1265c8+r5);      /* add r5,pc → .rodata 偏移 (GetName) */
Lbde60: r12=r3;               /* ip = vtable[0x84] 方法指针 */
Lbde62: r8=r1;               /* r8 = 132 (0x84 槽索引) */
Lbde64: {uint32_t a_=r5,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lbde66: r2=(0x128b08+r2);      /* add r2,pc → .rodata 偏移 (sig) */
Lbde68: {uint32_t a_=r6,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbde6a: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbde6c: r0=jqt_jni_getmethodid(r0, r1, r2, r5); /* blx ip = GetMethodID (§124ak, 输入事件基类槽 42) */
Lbde6e: {uint32_t a_=r7,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbde70: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbde72: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbde74: r0=F_7bf80(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x7bf80 CallIntMethod (getKeyCode → r9) */
Lbde78: r3=MEM(r4);            /* 重新读 [r4] = vtable 基址 */
Lbde7a: r1=r8;               /* r1 = 132 (0x84 槽 42 索引) */
Lbde7c: r2=MEM(795220);        /* .rodata 0x128b24 (name 第 2 次) */
Lbde7e: r3=MEM(r3+r1);        /* vtable[0x84] */
Lbde80: r9=r0;               /* r9 = 第 1 次 GetMethodID 返回 (mid, 非键码) */
Lbde82: r2=(0x128b24+r2);
Lbde84: r12=r3;
Lbde86: {uint32_t a_=r6,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbde88: {uint32_t a_=r5,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lbde8a: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbde8c: r0=jqt_jni_getmethodid(r0, r1, r2, r5); /* blx ip = GetMethodID (第 2 次, 槽 42, getAction) */
Lbde8e: {uint32_t a_=r7,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbde90: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbde92: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbde94: r0=F_7bf80(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x7bf80 CallIntMethod (getAction → sl) */
Lbdea0: r10=r0;              /* sl=r0 = 第 2 次 CallIntMethod 返回 (action 值, 键码匹配用) */
Lbdea4: r1=r8;              /* re-load 槽索引 */
Lbdea6: r3=MEM(795224);      /* .rodata 0x128b30 (name 第 3 次) */
Lbdea8: r0=MEM(r4);          /* re-read [r4] = vtable 基址 */
Lbdeaa: r3=MEM(r0+r1);       /* vtable[0x84] */
Lbdeac: r0=jqt_jni_getmethodid(r0, r1, r3, r5); /* blx ip = GetMethodID (第 3 次, 槽 42) */
Lbdeb0: {uint32_t a_=r7,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbdeb2: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbdeb4: r0=F_7bf80(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x7bf80 CallIntMethod (第 3 次) */
Lbdeb8: {uint32_t a_=r6,b_=0; Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
if(Z==0){ /* r6 != 0 → 执行 vtable 槽 0x5c 虚调用 */
Lbdec0: r3=MEM(MEM(r4)+92);  /* ldr r3,[r4+0]=vtable; ldr r3,[vtable+0x5c] 槽 23 */
Lbdec2: r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* blx r3 (vtable 槽 23, §124y-137) */
}
Lbdec6: r3=MEM(0x16266c);     /* .bss 0x16266c 键码单例槽 (.bss 偏移 0x67EC, §124y-139 区) */
Lbdeca: r2=MEM(r3+16);        /* ldr r2,[r3+16] = 键码常量槽 0 (MuMu 实锤=2) */
Lbdecc: {uint32_t a_=r9,b_=r2; Z=(r9==b_); C=(r9<b_); N=((a_^b_)>>31)&1; V=(((a_^r9)&(b_^r9))>>31)&1;}
if(Z==1){ goto Lbdf18; }       /* 键码匹配 → 进一步判定 */
Lbded0: r3=MEM(0x16266c);     /* 重读 .bss 基址 */
Lbded2: r0=0;                /* movs r0,#0 */
Lbded6: r2=MEM(r3+24);        /* ldr r2,[r3+24] = 键码常量槽 1 (MuMu 实锤=4) */
Lbded8: {uint32_t a_=r9,b_=r2; Z=(r9==b_); C=(r9<b_); N=((a_^b_)>>31)&1; V=(((a_^r9)&(b_^r9))>>31)&1;}
if(Z==1){ goto Lbdeea; }       /* 键码匹配 0x22 分支 */
Lbdeea: r2=0x22;               /* movs r2,#34 (0x22, 事件码层 2, §124y-132) */
Lbdeec: MEM(sp+4)=r2;          /* str r2,[sp+4] = 事件码 0x22 写栈 */
Lbdeee: r1=sp+4;              /* add r1,sp,#4 (str [r1+4] = [sp+8]) */
Lbdef0: r2=MEM(r3+20);        /* ldr r2,[r3+20] = 键码常量槽 2 (MuMu 实锤=0) */
Lbdef2: {uint32_t a_=r10,b_=r2; Z=(r10==b_); C=(r10<b_); N=((a_^b_)>>31)&1; V=(((a_^r10)&(b_^r10))>>31)&1;}
if(Z==1){ goto Lbdf2e; }       /* action 匹配 → 事件码 0x27 */
Lbdef6: r2=MEM(r3+28);        /* 键码常量槽 3 (MuMu 实锤=1) */
Lbdefa: {uint32_t a_=r10,b_=r2; Z=(r10==b_); C=(r10<b_); N=((a_^b_)>>31)&1; V=(((a_^r10)&(b_^r10))>>31)&1;}
if(Z==1){ goto Lbdf22; }       /* action 匹配 → 事件码 18 */
Lbdefc: r2=MEM(r3+32);        /* 键码常量槽 4 (MuMu 实锤=0x52/82) */
Lbdf00: {uint32_t a_=r10,b_=r2; Z=(r10==b_); C=(r10<b_); N=((a_^b_)>>31)&1; V=(((a_^r10)&(b_^r10))>>31)&1;}
if(Z==1){ goto Lbdf34; }       /* action 匹配 → 事件码 0x25 */
Lbdf02: r2=MEM(r3+36);        /* 键码常量槽 5 (MuMu 实锤=0) */
Lbdf06: {uint32_t a_=r10,b_=r2; Z=(r10==b_); C=(r10<b_); N=((a_^b_)>>31)&1; V=(((a_^r10)&(b_^r10))>>31)&1;}
if(Z==1){ goto Lbdf40; }       /* action 匹配 → 事件码 0x27 */
Lbdf08: r2=MEM(r3+40);        /* 键码常量槽 6 (MuMu 实锤=0) */
Lbdf0c: {uint32_t a_=r10,b_=r2; Z=(r10==b_); C=(r10<b_); N=((a_^b_)>>31)&1; V=(((a_^r10)&(b_^r10))>>31)&1;}
if(Z==1){ goto Lbdf46; }       /* action 匹配 → 事件码 0x26 */
Lbdf0e: r3=MEM(r3+44);        /* 键码常量槽 7 (MuMu 实锤=0x13/19) */
Lbdf10: {uint32_t a_=r10,b_=r3; Z=(r10==b_); C=(r10<b_); N=((a_^b_)>>31)&1; V=(((a_^r10)&(b_^r3))>>31)&1;}
if(Z==1){ goto Lbdf3a; }       /* action 匹配 → 事件码 0x28 */
Lbdf14: r0=1;                  /* 全不匹配 → 返回 1 */
goto Lbdedc;
Lbdf18: r3=MEM(r3+20);        /* 键码常量槽 2 (Lbdeea 跳转目标, 重新读 .bss) */
Lbdf1a: {uint32_t a_=r10,b_=r3; Z=(r10==b_); C=(r10<b_); N=((a_^b_)>>31)&1; V=(((a_^r10)&(b_^r3))>>31)&1;}
if(Z==0){ goto Lbded0; }       /* 不匹配 → 继续比较 (回 Lbded0) */
Lbdf1e: r0=1;                  /* 匹配成功 → 返回 1 */
goto Lbdedc;
Lbdf22: r3=18;                 /* movs r3,#18 (事件码 18, 键码匹配写入) */
Lbdf24: MEM(r1+4)=r3;          /* str r3,[r1+4] = 事件码 18 写事件对象 */
Lbdf26: r0=r11;               /* mov r0,fp = env */
Lbdf28: r0=F_bd778(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd778 按键事件写入 */
goto Lbdedc;
Lbdf2e: r3=27;                 /* movs r3,#27 (事件码 27) */
Lbdf30: MEM(r1+4)=r3;          /* 事件码 27 写入 */
goto Lbdf26;
Lbdf34: r3=0x25;               /* movs r3,#37 (事件码 0x25) */
Lbdf36: MEM(r1+4)=r3;          /* 事件码 0x25 写入 */
goto Lbdf26;
Lbdf3a: r3=0x28;               /* movs r3,#40 (事件码 0x28) */
Lbdf3c: MEM(r1+4)=r3;          /* 事件码 0x28 写入 */
goto Lbdf26;
Lbdf40: r3=0x27;               /* movs r3,#39 (事件码 0x27) */
Lbdf42: MEM(r1+4)=r3;          /* 事件码 0x27 写入 */
goto Lbdf26;
Lbdf46: r3=0x26;               /* movs r3,#38 (事件码 0x26) */
Lbdf48: MEM(r1+4)=r3;          /* 事件码 0x26 写入 */
goto Lbdf26;
Lbdedc: sp+=20;
Lbdede: r2=MEM(sp+0); r3=MEM(sp+4); r4=MEM(sp+8); r5=MEM(sp+12); /* pop {r2,r3,r4,r5} */
Lbdee0: r8=r2; r9=r3; r10=r4; r11=r5; /* mov r8/r9/sl/fp */
Lbdee8: r4=MEM(sp+0); /* pop {r4, pc} */
Lbdee8_r: goto RET;
RET: return r0;
}

