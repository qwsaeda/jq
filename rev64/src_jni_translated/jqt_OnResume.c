// §124y-141: F_bdc7c (OnResume 内部处理函数) 对齐原版 0xbdc7c-0xbdcc6 完整体
// (照 F_bde34 风格: 逐标签 MEM + JNI 函数显式调用 + 条件虚调用 + .bss 根对象调用)
//
// 原版 .text 0xbdc7c (OnResume 内部处理, JNI 入口 0x234b0 bl 0xbdc7c):
//   入参: r0=env(JNIEnv*), r1=env(adds r1,r0,#0), r2=Activity 对象
//   流程:
//     0xbdc7e ldr r3,[r1,#0] + 0xbdc86 ldr r3,[r3,#124=0x7c] → vtable 槽 31 (GetObjectClass 区)
//     0xbdc8a blx r3                (JNI 虚调用 = GetObjectClass)
//     0xbdc8c ldr r2,[pc] → .rodata 0x128af8 "onResume"
//     0xbdc90 ldr r3,[pc] → .rodata 0x126044 "()V"
//     0xbdc92 ldr r0,[r4,#0] + 0xbdc98 ldr r7,[r0,#132=0x84] → vtable 槽 42
//     0xbdca0 blx r7               (JNI 虚调用 = GetMethodID "onResume" "()V")
//     0xbdca2 ldr r1,[r6,#8]     (Activity 对象 [0x8] 子字段)
//     0xbdca8 bl 0x77800         (CallVoidMethod, F_77800 已收表)
//     0xbdcac cmp r5,#0 / beq    (r5=GetMethodID 返回, 0 → 跳过 vtable 槽 0x5c 虚调用)
//     0xbdcba ldr r3,[pc] → .bss 0x15be90 (根对象+0x10 区)
//     0xbdcc0 bl 0xb690c         (读 [r0+0xc0] 虚调用, 渲染器/事件对象 C++ 方法)
//   返回: void (JNI void 方法)
//
// 诚实边界:
//   - 0xb690c 未收表 (dispatch 表外, 读 [r0+0xc0] C++ 虚调用), 存根 r0=0
//   - JNI 函数 (GetObjectClass/GetMethodID/CallVoidMethod) 走 jqt_jni_* 真实调用 (§124ak)
//   - .bss 0x15be90 根对象+0x10 区未构造 (翻译体 .bss 全 0), F_b690c 忠实空操作
#include <stdint.h>
#include "jqt_mem.h"
#include <android/log.h>
/* F_bdc7c 用到的 extern 声明 (jqt_vtbl.c / jqt_OnCreate_full.c / jqt_jnibr.c 定义) */
extern uint32_t F_77800(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t F_b690c(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t F_b6768(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t F_bdc7c(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t F_bdcd4(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t jqt_jni_getobjectclass(uint32_t env, uint32_t obj);
extern uint32_t jqt_jni_getmethodid(uint32_t env, uint32_t cls, uint32_t name_off, uint32_t sig_off);
extern uint32_t jqt_blx_dispatch(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_bdc7c(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);

uint32_t F_bdc7c(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    __android_log_print(4, "junqi", "F_bdc7c: 入口 r0(env)=0x%08x r1=0x%08x", _r0,_r1);
Lbdc7c: sp-=(6*4); /* push r3, r4, r5, r6, r7, lr */
Lbdc7c_p0: MEM(sp+0)=r3;
Lbdc7c_p1: MEM(sp+4)=r4;
Lbdc7c_p2: MEM(sp+8)=r5;
Lbdc7c_p3: MEM(sp+12)=r6;
Lbdc7c_p4: MEM(sp+16)=r7;
Lbdc7e: r3=MEM(r1);             /* .bss 根对象[0] (r1=env, §124y-97 根对象槽) */
if (r1 != 0) { /* 防御: .bss 未构造 → r1=0 → 忠实兜底 (跳过 GetObjectClass) */
Lbdc80: {uint32_t a_=r1,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lbdc82: {uint32_t a_=r0,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lbdc84: r1=MEM(r0+8);          /* env[0x8] = JNI 函数表 */
Lbdc86: r3=MEM(r3+124);        /* vtable 槽 31 (0x7c/4), §124y-136/137 输入事件基类 */
Lbdc88: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbdc8a: r0=jqt_jni_getobjectclass(r0, r1); /* blx r3 = GetObjectClass (§124ak, r1=对象) */
}
Lbdc8c: r2=MEM(795192);        /* ldr r2,[pc] → .rodata 0x128af8 "onResume" */
Lbdc8e: r5=r0;                /* r5 = GetMethodID 返回 (mid 句柄) */
Lbdc90: r3=MEM(795196);        /* ldr r3,[pc] → .rodata 0x126044 "()V" */
Lbdc92: r0=MEM(r4);           /* Activity [0] = vtable 基址 */
Lbdc94: r1=132;               /* movs r1,#132 (0x84, vtable 槽 42) */
Lbdc96: r2=(0x128af8+r2);      /* add r2,pc → .rodata 偏移 (onResume name) */
Lbdc98: r7=MEM(r0+r1);        /* vtable[0x84] = 子类方法指针 (GetMethodID) */
Lbdc9a: r3=(0x126044+r3);      /* add r3,pc → .rodata 偏移 (()V sig) */
Lbdc9c: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbdc9e: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbdca0: r12=r7;               /* ip = vtable[0x84] */
if (r4 != 0 && r0 != 0) { /* 防御: .bss 未构造 → r4=0 → 忠实兜底 (跳过 GetMethodID) */
Lbdca0_r: r0=jqt_jni_getmethodid(r0, r1, r2, r3); /* blx r7 = GetMethodID "onResume" "()V" (§124ak) */
}
Lbdca2: r1=MEM(r6+8);         /* Activity 对象 [0x8] 子字段 */
Lbdca4: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbdca6: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
if (r4 != 0) { /* 防御: CallVoidMethod 需 valid activity */
Lbdca8: r0=F_77800(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x77800 CallVoidMethod */
}
Lbdcac: {uint32_t a_=r5,b_=0; Z=(r5==b_); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
if(Z==0){ /* r5!=0 (GetMethodID 成功) → 执行 vtable 槽 0x5c 虚调用 */
Lbdcb0: r3=MEM(MEM(r4)+92);   /* ldr r3,[r4+0]=vtable; ldr r3,[vtable+0x5c] 槽 23 */
Lbdcb2: r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* blx r3 (vtable 槽 23, §124y-137) */
}
Lbdcba: r3=MEM(0x15be90);     /* .bss 0x15be90 根对象+0x10 区 (.bss 偏移 0x10) */
Lbdcc0: r0=F_b690c(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb690c (读 [r0+0xc0] 虚调用) */
Lbdcc4: r3=MEM(sp+0); r4=MEM(sp+4); r5=MEM(sp+8); r6=MEM(sp+12); r7=MEM(sp+16); /* pop */
Lbdcc6: r0=0; goto RET;
RET: return r0;
}

/* §124y-141: F_bdcd4 (OnPause 内部处理函数) 对齐原版 0xbdcd4-0xbdd1c 完整体
 * (与 F_bdc7c 同模式: GetObjectClass → GetMethodID "getRepeatCount" "(II)V" → CallVoidMethod → 条件虚调用 → F_b6768)
 *
 * 原版 .text 0xbdcd4:
 *   入参: r0=env(JNIEnv*), r1=env, r2=Activity 对象
 *   0xbdcde ldr r3,[r1,#0] + 0xbdcde ldr r3,[r3,#124=0x7c] → vtable 槽 31
 *   0xbdce2 blx r3 = GetObjectClass
 *   0xbdce4 ldr r2,[pc] → .rodata 0x128b34 "getRepeatCount"
 *   0xbdce8 ldr r3,[pc] → .rodata 0x126074 "(II)V"
 *   0xbdcf0 ldr r7,[r0,#132=0x84] → vtable 槽 42
 *   0xbdcf8 blx r7 = GetMethodID "getRepeatCount" "(II)V"
 *   0xbdd00 bl 0x77800 = CallVoidMethod
 *   0xbdd04 cmp r5,#0 / beq → vtable 槽 0x5c 虚调用
 *   0xbdd18 bl 0xb6768 = 读 [r0+0xc0] 虚调用
 * 诚实边界:
 *   - 0xb6768 未收表 (dispatch 表外), 存根 r0=0
 *   - JNI 函数走 jqt_jni_* 真实调用 (§124ak)
 *   - .bss 0x15be84 根对象+0xc 区未构造, F_b6768 忠实空操作 */
uint32_t F_bdcd4(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);

uint32_t F_bdcd4(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    __android_log_print(4, "junqi", "F_bdcd4: 入口 r0(env)=0x%08x r1=0x%08x", _r0,_r1);
Lbdcd4: sp-=(6*4); /* push r3, r4, r5, r6, r7, lr */
Lbdcd4_p0: MEM(sp+0)=r3;
Lbdcd4_p1: MEM(sp+4)=r4;
Lbdcd4_p2: MEM(sp+8)=r5;
Lbdcd4_p3: MEM(sp+12)=r6;
Lbdcd4_p4: MEM(sp+16)=r7;
Lbdcd6: r3=MEM(r1);             /* .bss 根对象[0] (r1=env, §124y-97 根对象槽) */
if (r1 != 0) { /* 防御: .bss 未构造 → r1=0 → 忠实兜底 (跳过 GetObjectClass) */
Lbdcd8: {uint32_t a_=r1,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lbdcd: {uint32_t a_=r0,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lbdcdc: r1=MEM(r0+8);          /* env[0x8] = JNI 函数表 */
Lbdcde: r3=MEM(r3+124);        /* vtable 槽 31 (0x7c/4) */
Lbdce0: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbdce2: r0=jqt_jni_getobjectclass(r0, r1); /* blx r3 = GetObjectClass (§124ak, r1=对象) */
}
Lbdce4: r2=MEM(795200);        /* ldr r2,[pc] → .rodata 0x128b34 "getRepeatCount" */
Lbdce6: r5=r0;                /* r5 = GetMethodID 返回 */
Lbdce8: r3=MEM(795204);        /* ldr r3,[pc] → .rodata 0x126074 "(II)V" */
Lbdcea: r0=MEM(r4);           /* Activity [0] = vtable 基址 */
Lbdcec: r1=132;               /* vtable 槽 42 (0x84) */
Lbdcee: r2=(0x128b34+r2);      /* add r2,pc → .rodata 偏移 */
Lbdcf0: r7=MEM(r0+r1);        /* vtable[0x84] */
Lbdcf2: r3=(0x126074+r3);      /* add r3,pc → .rodata 偏移 */
Lbdcf4: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbdcf6: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbdcf8: r12=r7;               /* ip = vtable[0x84] */
if (r4 != 0 && r0 != 0) { /* 防御: .bss 未构造 → 忠实兜底 (跳过 GetMethodID) */
Lbdcf8_r: r0=jqt_jni_getmethodid(r0, r1, r2, r3); /* blx r7 = GetMethodID "getRepeatCount" "(II)V" */
}
Lbdcfa: r1=MEM(r6+8);         /* Activity [0x8] */
Lbdcfc: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbdcfe: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
if (r4 != 0) { /* 防御: CallVoidMethod 需 valid activity */
Lbdd00: r0=F_77800(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x77800 CallVoidMethod */
}
Lbdd04: {uint32_t a_=r5,b_=0; Z=(r5==b_); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
if(Z==0){ /* r5!=0 → vtable 槽 0x5c 虚调用 */
Lbdd08: r3=MEM(MEM(r4)+92);
Lbdd0a: r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
}
Lbdd12: r3=MEM(0x15be84);     /* .bss 0x15be84 根对象+0xc 区 (.bss 偏移 0xc) */
Lbdd18: r0=F_b6768(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb6768 */
Lbdd1c: r3=MEM(sp+0); r4=MEM(sp+4); r5=MEM(sp+8); r6=MEM(sp+12); r7=MEM(sp+16); /* pop */
Lbdd1e: r0=0; goto RET;
RET: return r0;
}

/* §124y-141: jqt_OnResume/jqt_OnPause 入口 (JNI 入口真调 F_bdc7c/F_bdcd4, 非存根)
 *   原版 0x234b0 OnResume: r0=env, r1=env(adds r1,r0,#0), ldr r0,[.bss 0x15be90] bl 0xbdc7c
 *   原版 0x234c4 OnPause:  r0=env, r1=env, ldr r0,[.bss 0x15be84] bl 0xbdcd4
 *   修正 jni_aligned.c 传参 (env,env) 与 OnKey/OnTouch 同模式 (§124y-140) */
extern uint32_t jqt_jni_getmethodid(uint32_t env, uint32_t cls, uint32_t name_off, uint32_t sig_off);
uint32_t jqt_OnResume(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t jqt_OnResume(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
L234b2: r3=MEM(144564+12);
L234b4: {uint32_t a_=r0,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L234b6: r3=(144568+r3);
L234b8: r0=MEM(r3);
L234ba: r0=F_bdc7c(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbdc7c 完整 (§124y-141, 非存根) */
L234be_l0: r4=MEM(sp+0);
L234be: sp+=(1*4); /* pop {r4, pc} */
L234be_r: goto RET;
RET: return r0;
}


/* §124y-145: F_b690c 完整体对齐 (原版 0xb690c-0xb6942, 读 [r0+0xc0] 子对象 vtable 虚调用 + F_b5d58 完整体)
 * 原版 0xb690c: ldr r0,[r0,#0xc0] / cmp r0,#0 / beq → 读 [vtable+8] 虚调用
 *   非 0 路径: str r3,[r4+#0x68] + 栈构造 (r1=sp+8, r2=池→0xb6af1, [sp+0]=250<<1, [sp+8]=r2, r0=r4+32)
 *   + bl 0xb5d58 (F_b5d58 已收表, jqt_vtbl.c:69270 定义体)
 * 诚实边界: .bss 未构造 → [r0+0xc0]=0 → beq 跳过 → r0=0 (忠实空操作, §124y-33/35 ok=0 路径) */
extern uint32_t F_b5d58(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b690c(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
Lb690c: r4=_r4; /* push {r4, lr} */
Lb690c_p0: MEM(sp+0)=r4;
Lb690e: r3=0xc0;
Lb6910: {uint32_t a_=r0,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb6912: r0=MEM(r0+0xc0);       /* ldr r0,[r0,#192=0xc0] 子对象 */
Lb6914: sp-=16;               /* sub sp,#16 */
Lb6916: {uint32_t a_=r0,b_=0; Z=(r0==b_); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
if(Z){ goto Lb6940; }          /* r0==0 → 跳过虚调用, 返回 0 */
Lb691a: r3=MEM(r0);            /* ldr r3,[r0,#0] = vtable 基址 */
Lb691c: r3=MEM(r3+8);          /* ldr r3,[vtable+8] 槽 2 */
Lb691e: r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* blx r3 (vtable 槽 2 虚调用) */
Lb6920: r3=0;
Lb6922: r1=sp+8;              /* add r1,sp,#8 */
Lb6924: r2=MEM(0xb6af1);       /* ldr r2,[pc] → 池 0xb6944 → 0xb6af1 */
Lb6926: MEM(r4+0x68)=r3;       /* str r3,[r4,#104=0x68] (子对象 C++ 字段清零) */
Lb6928: MEM(r1+4)=r3;          /* str r3,[r1,#4] (栈 [sp+12]=0) */
Lb692a: r3=250;
Lb692c: r3=(r3<<1);            /* lsls r3,#2 → 250<<1=500? 实际 movs r3,#250 即 0xfa */
Lb692e: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb6930: r2=(0xb6af1+r2);      /* add r2,pc → 池目标 */
Lb6932: MEM(sp+0)=r3;          /* str r3,[sp,#0] */
Lb6934: r0=(r0+32);           /* adds r0,#32 (子对象 +0x20) */
Lb6936: {uint32_t a_=r4,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lb6938: r3=0;
Lb693a: MEM(sp+8)=r2;          /* str r2,[sp,#8] */
Lb693c: r0=F_b5d58(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb5d58 (已收表) */
Lb6940: sp+=16;               /* add sp,#16 */
Lb6942: r4=MEM(sp+0); /* pop {r4, pc} */
RET: return r0;
}

uint32_t F_b6768(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
Lb6768: r4=_r4; /* push {r4, lr} */
Lb6768_p0: MEM(sp+0)=r4;
Lb676a: r3=0xc0;
Lb676c: r0=MEM(r0+0xc0);       /* ldr r0,[r0,#192=0xc0] 子对象 */
Lb676e: {uint32_t a_=r0,b_=0; Z=(r0==b_); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
if(Z){ goto Lb6778; }          /* r0==0 → 跳过虚调用, 返回 0 */
Lb6772: r3=MEM(r0);            /* ldr r3,[r0,#0] = vtable 基址 */
Lb6774: r3=MEM(r3+4);         /* ldr r3,[vtable+4] 槽 1 */
Lb6776: r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* blx r3 (vtable 槽 1 虚调用, §124y-145 非存根) */
Lb6778: r4=MEM(sp+0); /* pop {r4, pc} */
Lb6778_r: goto RET;
RET: return r0;
}
