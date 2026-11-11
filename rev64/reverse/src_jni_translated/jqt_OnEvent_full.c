/* jqt_OnEvent_full.c — 触摸→对局路径（F_bd914/F_bdd2c + 缺失 callee）
 * 生成自 gen_onevent_full.py（bd914_full.c + bdd2c_full.c）
 * 修复：blx rX vtable dispatch → jqt_blx_dispatch；blx 0x233ac → __aeabi_f2iz 真实；
 *       blx 0x22e60 __stack_chk_fail noreturn → 保持存根。
 * jqt_blx_dispatch 定义在 jqt_vtbl.c（vtable 槽值→目标函数）。 */
#include <stdint.h>
#include <android/log.h>
#include <stddef.h>
#include <jni.h>
#include "jqt_mem.h"
uint32_t F_b26ac(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b4238(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b6948(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b7e00(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_bd580(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_bd778(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_bd914(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_bdd2c(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
/* ★ §TOUCH-JNI（2026-11-05）：F_bdd2c JNI 调用（GetObjectClass/GetMethodID/DeleteLocalRef）
 *   定义在 jqt_jnibr.c。原版 blx 到 JNIEnv functions[]（真实指针非 shadow 偏移）→ dispatch miss
 *   → 触摸坐标解析失败。修复：显式调用这些 JNI 桥。 */
uint32_t jqt_jni_getobjectclass(uint32_t env, uint32_t obj);
uint32_t jqt_jni_getmethodid(uint32_t env, uint32_t cls, uint32_t name_off, uint32_t sig_off);
uint32_t jqt_jni_deletelocalref(uint32_t env, uint32_t obj);

uint32_t F_b26ac(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    /* §124ai：函数体 #define MEM 移除，用 jqt_mem.h MEM（jqt_mem_addr 加偏移） */
    /* ★ AUD-1000 B05-067（P1 已修，32 位 so 反汇编实锤）：0xb26ac 函数体
       ARM 实锤 3 指令：PUSH {r2,r5}; MOV r0,#0; POP {r0,r2,r4,r6,pc}。
       这是一个恒返 0 的 trivial 存根（保存 r2/r5，清零 r0，弹栈返回）。
       旧翻译体 "RET: return r0" 不准确（原版恒返 0，非透传 r0），现修正。
       后续 +0x0c..+0x2c 的 6 条指令（LDR r0,[sp,#0x60] / LDR r1,[sp,#0x5c]
       / AND r0,r0,#1 / STR/... / CMP / MOV）在本函数 POP pc 后为冷尾
       （不可达死区，原版编译器填充），翻译体不展开。 */
Lb26ac: r0=0; /* MOV r0, #0（原版恒返 0，非透传入参 r0） */
    return r0;
}
uint32_t F_b6948(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    /* §124ai：函数体 #define MEM 移除，用 jqt_mem.h MEM（jqt_mem_addr 加偏移） */
Lb6948: sp-=(5*4); /* push r4, r5, r6, r7, lr */
Lb6948_p0: MEM(sp+0)=r4;
Lb6948_p1: MEM(sp+4)=r5;
Lb6948_p2: MEM(sp+8)=r6;
Lb6948_p3: MEM(sp+12)=r7;
Lb694a: r7=r8;
Lb694c: sp-=(1*4); /* push r7 */
Lb694c_p0: MEM(sp+0)=r7;
Lb694e: r8=r0;
Lb6950: {uint32_t a_=r1,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb6952: {uint32_t a_=r2,b_=0; r7=(a_+b_); Z=(r7==0); C=(r7<a_); N=((r7)>>31)&1; V=(((a_^r7)&(b_^r7))>>31)&1;}
Lb6954: {uint32_t a_=r1,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lb6956: r0=0; /* blx 0x22c80 存根 */
Lb695a: {uint32_t a_=r0,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb695c: {uint32_t a_=r7,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb695e: r0=0; /* blx 0x22c80 存根 */
Lb6962: r3=128;
Lb6964: {uint32_t a_=r4,b_=r0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb6966: {uint32_t a_=r6,b_=6; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb6968: r3=((r3<<6))&0xffffffffu; Z=(r3==0); N=(((r3)>>31))&1;
Lb696a: {int32_t A=(int32_t)r6,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb696c: if((!Z&&(N==V))) goto Lb69ac;
Lb696e: r4=r8;
Lb6970: {uint32_t a_=r4,b_=140; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb6972: r1=0;
Lb6974: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb6976: r0=F_b3c40(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb3c40 完整 */
Lb697a: {uint32_t a_=r6,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lb697c: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb697e: r0=F_b3dcc(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb3dcc 完整 */
Lb6982: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lb6984: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb6986: r0=F_b4238(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb4238 完整 */
Lb698a: {uint32_t a_=r7,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lb698c: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb698e: r0=F_b4238(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb4238 完整 */
Lb6992: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb6994: r0=F_b3c20(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb3c20 完整 */
Lb6998: {uint32_t a_=r0,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lb699a: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb699c: r0=F_b3c24(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb3c24 完整 */
Lb69a0: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb69a2: r0=r8;
Lb69a4: {uint32_t a_=r0,b_=172; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb69a6: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lb69a8: r0=F_b7e00(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb7e00 完整 */
Lb69ac: sp+=(1*4); /* pop {r2} */
Lb69ae: r8=r2;
Lb69b0_l0: r4=MEM(sp+0);
Lb69b0_l1: r5=MEM(sp+4);
Lb69b0_l2: r6=MEM(sp+8);
Lb69b0_l3: r7=MEM(sp+12);
Lb69b0: sp+=(4*4); /* pop {r4, r5, r6, r7, pc} */
Lb69b0_r: goto RET;
RET: return r0;
    return 0;
}
uint32_t F_bd580(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    /* §124ai：函数体 #define MEM 移除，用 jqt_mem.h MEM（jqt_mem_addr 加偏移） */
    /* ★ §TOUCH-JNI（2026-11-05）：原版 F_bd580 = CallFloatMethod 包装（functions[0xe0/4=56]）。
     *   旧翻译体是空函数（直接返回 r0=env 当 float）→ getX/getY 返回垃圾 → 触摸坐标解析失败。
     *   修复：直接 CallFloatMethod(env, obj, mid) 返回 float 位模式。 */
    {
        uint32_t _env = r0;
        /* ★ AUD-1000 B05-069（P1 R-138 诚实标注）：`_env < 0x70000000u` 判
           env 无效基于 32 位设备 host 地址布局假设（JNIEnv* 高地址区）——
           x86_64 译层（MuMu houdini）env 可能 < 0x70000000 但低位合法，
           或 >= 0x70000000 但为垃圾值，误判双向。改用 jqt_mem_valid
           或存 g_jni_env 非 0 判更稳，属行为改动（判读语义变化），
           R-138 只标注；现状阈值在 MuMu 真机 env=0x8xxxxxxx 实锤可用。 */
        if (_env < 0x70000000u || r1 == 0 || r2 == 0) { r0 = 0; }
        else {
            JNIEnv *e = (JNIEnv*)(uintptr_t)_env;
            jfloat _ret = (*e)->CallFloatMethod(e, (jobject)(uintptr_t)r1, (jmethodID)(uintptr_t)r2);
            if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
            uint32_t _bits; __builtin_memcpy(&_bits, &_ret, 4);
            r0 = _bits;
            __android_log_print(6,"junqi","F_bd580 CallFloatMethod: env=0x%08x obj=0x%08x mid=0x%08x ret=%.2f bits=0x%08x", _env, (uint32_t)r1, (uint32_t)r2, (double)_ret, _bits);
        }
    }
RET: return r0;
    return 0;
}
uint32_t F_bd778(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    /* §124ai：函数体 #define MEM 移除，用 jqt_mem.h MEM（jqt_mem_addr 加偏移） */
Lbd778: sp-=(2*4); /* push r4, lr */
Lbd778_p0: MEM(sp+0)=r4;
Lbd77a: r3=60;
Lbd77c: r3=MEMB(r0+r3);
Lbd77e: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbd780: if(!Z) goto Lbd78a;
Lbd782: r0=MEM(r0+36);
Lbd784: r2=12;
Lbd786: r0=F_b7e00(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb7e00 完整 */
Lbd78a: r0=1;
Lbd78c_l0: r4=MEM(sp+0);
Lbd78c: sp+=(1*4); /* pop {r4, pc} */
Lbd78c_r: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_bd914(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    __android_log_print(4, "junqi", "F_bd914: 入口 r0=0x%08x r1=0x%08x r2=0x%08x r3=0x%08x", _r0,_r1,_r2,_r3);
    /* §124ai：函数体 #define MEM 移除，用 jqt_mem.h MEM（jqt_mem_addr 加偏移） */
Lbd914: sp-=(5*4); /* push r4, r5, r6, r7, lr */
Lbd914_p0: MEM(sp+0)=r4;
Lbd914_p1: MEM(sp+4)=r5;
Lbd914_p2: MEM(sp+8)=r6;
Lbd914_p3: MEM(sp+12)=r7;
Lbd916: r7=r11;
Lbd918: r6=r10;
Lbd91a: r5=r9;
Lbd91c: r4=r8;
Lbd91e: sp-=(4*4); /* push r4, r5, r6, r7 */
Lbd91e_p0: MEM(sp+0)=r4;
Lbd91e_p1: MEM(sp+4)=r5;
Lbd91e_p2: MEM(sp+8)=r6;
Lbd91e_p3: MEM(sp+12)=r7;
Lbd920: r4=MEM(776484+212);
Lbd922: r5=MEM(776484+216);
Lbd924: r11=r3;
Lbd926: sp+=r4; /* ★ AUD-1000 B05-071（P1 R-138 诚实标注）：r4=MEM(776484+212)
           常量池值，原版 ARM 语义 sp+=r4 是"sp 相对调整"；C 函数栈
           非 ARM 栈，本地 sp 变量 +=r4 后后续 MEM(sp+4) 读的是
           "伪栈偏移"而非真实 C 栈。r4 为 RELATIVE 槽值时须 -shadow
           还原偏移才语义一致（见 §124ac sp 语义修复）。现状 32 位
           翻译体此函数走忠实兜底（sp 本地变量语义），行为由 4 路径
           回归实证——偏移正确性依赖 r4 值域（小常量），R-138 只标注。 */
Lbd928: {uint32_t a_=r1,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lbd92a: r1=MEM(776492+212);
Lbd92c: r5=(776496+r5);
Lbd92e: r8=r2;
Lbd930: r3=MEM(r5+r1);
Lbd932: r2=MEM(776500+208);
Lbd934: MEM(sp+4)=r0;
Lbd936: r3=MEM(r3);
Lbd938: r2=sp+r2;
Lbd93a: MEM(sp)=r1;
Lbd93c: MEM(r2)=r3;
Lbd93e: r3=MEM(r4);
Lbd940: r6=169;
Lbd942: r6=((r6<<2))&0xffffffffu; Z=(r6==0); N=(((r6)>>31))&1;
Lbd944: r3=MEM(r3+r6);
Lbd946: r1=r8;
Lbd948: r2=0;
Lbd94a: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbd94c: r0=jqt_blx_dispatch(r3, r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, sp); /* vtable dispatch */
Lbd94e: r3=MEM(776528+184);
Lbd950: r9=r0;
Lbd952: r1=128;
Lbd954: r3=sp+r3;
Lbd956: {uint32_t a_=r3,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbd958: r1=((r1<<1))&0xffffffffu; Z=(r1==0); N=(((r1)>>31))&1;
Lbd95a: r2=r9;
Lbd95c: r10=r3;
Lbd95e: r0=F_b4504(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb4504 完整 */
Lbd962: r3=MEM(r4);
Lbd964: r7=170;
Lbd966: r7=((r7<<2))&0xffffffffu; Z=(r7==0); N=(((r7)>>31))&1;
Lbd968: r1=r8;
Lbd96a: r3=MEM(r3+r7);
Lbd96c: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbd96e: r2=r9;
Lbd970: r0=jqt_blx_dispatch(r3, r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, sp); /* vtable dispatch */
Lbd972: r3=MEM(r4);
Lbd974: r1=r11;
Lbd976: r2=0;
Lbd978: r3=MEM(r3+r6);
Lbd97a: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbd97c: r0=jqt_blx_dispatch(r3, r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, sp); /* vtable dispatch */
Lbd97e: r2=128;
Lbd980: r1=sp+12;
Lbd982: r2=((r2<<7))&0xffffffffu; Z=(r2==0); N=(((r2)>>31))&1;
Lbd984: {uint32_t a_=r0,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lbd986: r8=r1;
Lbd988: r0=F_c2d24(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xc2d24 完整 */
Lbd98c: r3=MEM(r4);
Lbd98e: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbd990: r1=r11;
Lbd992: r3=MEM(r3+r7);
Lbd994: {uint32_t a_=r6,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbd996: r0=jqt_blx_dispatch(r3, r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, sp); /* vtable dispatch */
Lbd998: r1=MEM(776604+112);
Lbd99a: r0=r10;
Lbd99c: r1=(776608+r1);
Lbd99e: r0=F_b4db4(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb4db4 完整 */
Lbd9a2: {int32_t A=(int32_t)r0,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbd9a4: if(!Z) goto Lbd9e0;
Lbd9a6: r2=MEM(sp+4);
Lbd9a8: r3=60;
Lbd9aa: r3=MEMB(r2+r3);
Lbd9ac: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbd9ae: if(Z) goto Lbd9d0;
Lbd9b0: r1=MEM(sp);
Lbd9b2: r3=MEM(r5+r1);
Lbd9b4: r1=MEM(776632+76);
Lbd9b6: r1=sp+r1;
Lbd9b8: r2=MEM(r1);
Lbd9ba: r3=MEM(r3);
Lbd9bc: {int32_t A=(int32_t)r2,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r2>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbd9be: if(!Z) goto Lbd9f4;
Lbd9c0: r3=MEM(776644+76);
Lbd9c2: sp+=r3;
Lbd9c4_l2: r4=MEM(sp+8);
Lbd9c4_l3: r5=MEM(sp+12);
Lbd9c4: sp+=(4*4); /* pop {r2, r3, r4, r5} */
Lbd9c6: r8=r2;
Lbd9c8: r9=r3;
Lbd9ca: r10=r4;
Lbd9cc: r11=r5;
Lbd9ce_l0: r4=MEM(sp+0);
Lbd9ce_l1: r5=MEM(sp+4);
Lbd9ce_l2: r6=MEM(sp+8);
Lbd9ce_l3: r7=MEM(sp+12);
Lbd9ce: sp+=(4*4); /* pop {r4, r5, r6, r7, pc} */
Lbd9ce_r: goto RET;
Lbd9d0: r3=MEM(776660+64);
Lbd9d2: r1=r10;
Lbd9d4: r2=r8;
Lbd9d6: r3=(776666+r3);
Lbd9d8: r0=MEM(r3);
Lbd9da: r0=F_b6948(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb6948 完整 */
Lbd9de: goto Lbd9b0;
Lbd9e0: r3=MEM(776676+52);
Lbd9e2: r2=MEM(776676+56);
Lbd9e4: r1=2;
Lbd9e6: r3=(776682+r3);
Lbd9e8: r0=MEM(r3);
Lbd9ea: r2=(776686+r2);
Lbd9ec: r3=r8;
Lbd9ee: r0=F_b26ac(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb26ac 完整 */
Lbd9f2: goto Lbd9b0;
Lbd9f4: r0=0; /* blx 0x22e60 存根 */
RET: return r0;
    return 0;
}
uint32_t F_bdd2c(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    __android_log_print(4, "junqi", "F_bdd2c: 入口 r0=0x%08x r1=0x%08x r2=0x%08x r3=0x%08x", _r0,_r1,_r2,_r3);
    /* §124ai：函数体 #define MEM 移除，用 jqt_mem.h MEM（jqt_mem_addr 加偏移） */
Lbdd2c: sp-=(5*4); /* push r4, r5, r6, r7, lr */
Lbdd2c_p0: MEM(sp+0)=r4;
Lbdd2c_p1: MEM(sp+4)=r5;
Lbdd2c_p2: MEM(sp+8)=r6;
Lbdd2c_p3: MEM(sp+12)=r7;
Lbdd2e: r7=r11;
Lbdd30: r6=r10;
Lbdd32: r5=r9;
Lbdd34: r4=r8;
Lbdd36: sp-=(4*4); /* push r4, r5, r6, r7 */
Lbdd36_p0: MEM(sp+0)=r4;
Lbdd36_p1: MEM(sp+4)=r5;
Lbdd36_p2: MEM(sp+8)=r6;
Lbdd36_p3: MEM(sp+12)=r7;
Lbdd38: r3=MEM(r1);
Lbdd3a: {uint32_t a_=r1,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lbdd3c: sp-=20;
Lbdd3e: r3=MEM(r3+124);
Lbdd40: r11=r0;
Lbdd42: {uint32_t a_=r1,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbdd44: {uint32_t a_=r2,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbdd46: {uint32_t a_=r2,b_=0; r7=(a_+b_); Z=(r7==0); C=(r7<a_); N=((r7)>>31)&1; V=(((a_^r7)&(b_^r7))>>31)&1;}
Lbdd48: r0=jqt_jni_getobjectclass(r0, r1); /* ★ §TOUCH-JNI：原版 blx r3=GetObjectClass（vtable[31]），显式调用 */
Lbdd4a: r1=MEM(r4);
Lbdd4c: r2=MEM(777552+204);
Lbdd4e: r3=MEM(777552+208);
Lbdd50: {uint32_t a_=r0,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lbdd52: r0=132;
Lbdd54: r6=MEM(r1+r0);
Lbdd56: r8=r0;
Lbdd58: r3=(777564+r3);
Lbdd5a: r2=(777566+r2);
Lbdd5c: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbdd5e: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbdd60: r0=jqt_jni_getmethodid(r0, r1, r2, r3); /* ★ §TOUCH-JNI：原版 blx r6=GetMethodID（"getAction","()I"），显式调用 */
Lbdd62: {uint32_t a_=r7,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbdd64: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbdd66: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbdd68: r0=F_7bf80(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x7bf80 完整 */
Lbdd6c: r3=MEM(r4);
Lbdd6e: r1=r8;
Lbdd70: r6=MEM(777588+176);
Lbdd72: r3=MEM(r3+r1);
Lbdd74: r2=MEM(777592+176);
Lbdd76: r6=(777594+r6);
Lbdd78: r12=r3;
Lbdd7a: r2=(777598+r2);
Lbdd7c: {uint32_t a_=r6,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lbdd7e: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbdd80: r9=r0;
Lbdd82: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbdd84: r0=jqt_jni_getmethodid(r0, r1, r2, r3); /* ★ §TOUCH-JNI：原版 blx ip=GetMethodID（"getX","()F"），显式调用 */
Lbdd86: {uint32_t a_=r7,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbdd88: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbdd8a: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbdd8c: r0=F_bd580(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd580 完整 */
Lbdd90: r0=(uint32_t)(int)(*(float*)&r0); /* __aeabi_f2iz 真实
      ★ AUD-1000 B05-073（P1 R-138 诚实标注）：F_bd580 返回的 float 位模式
      强转 int（截断）——NaN/Inf 时 C 侧 UB（浮点陷阱，ARM __aeabi_f2iz
      对 NaN 行为定义依赖 FPU 状态）。F_bd580 走 CallFloatMethod（getX/
      getY），正常坐标有限值；NaN/Inf 仅 touch 事件异常时可达（§TOUCH-JNI
      实锤坐标解析失败走 r0=0 兜底路径，不到此强转）。显式 isfinite 判 +
      截断分支属行为改动（FPU 异常处理），R-138 只标注。 */
Lbdd94: r3=MEM(r4);
Lbdd96: r10=r0;
Lbdd98: r0=r8;
Lbdd9a: r3=MEM(r3+r0);
Lbdd9c: r2=MEM(777632+140);
Lbdd9e: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbdda0: r12=r3;
Lbdda2: r2=(777638+r2);
Lbdda4: {uint32_t a_=r6,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lbdda6: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbdda8: r0=jqt_jni_getmethodid(r0, r1, r2, r3); /* ★ §TOUCH-JNI：原版 blx ip=GetMethodID（"getY","()F"），显式调用 */
Lbddaa: {uint32_t a_=r7,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbddac: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbddae: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbddb0: r0=F_bd580(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd580 完整 */
Lbddb4: r0=(uint32_t)(int)(*(float*)&r0); /* __aeabi_f2iz 真实 */
Lbddb8: {uint32_t a_=r0,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lbddba: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbddbc: if(Z) goto Lbddc8;
Lbddbe: r3=MEM(r4);
Lbddc0: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbddc2: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbddc4: r3=MEM(r3+92);
Lbddc6: r0=jqt_jni_deletelocalref(r0, r1); /* ★ §TOUCH-JNI：原版 blx r3=DeleteLocalRef（vtable[23]），显式调用 */
Lbddc8: r3=MEM(777676+100);
Lbddca: r3=(777678+r3);
Lbddcc: r2=MEM(r3+4);
Lbddce: {int32_t A=(int32_t)r9,B=(int32_t)r2;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r9>=r2);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbddd0: if(Z) goto Lbde02;
Lbddd2: r2=MEM(r3+8);
Lbddd4: {int32_t A=(int32_t)r9,B=(int32_t)r2;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r9>=r2);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbddd6: if(Z) goto Lbde08;
Lbddd8: r3=MEM(r3+12);
Lbddda: r0=0;
Lbdddc: {int32_t A=(int32_t)r9,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r9>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbddde: if(Z) goto Lbddee;
Lbdde0: sp+=20;
Lbdde2_l2: r4=MEM(sp+8);
Lbdde2_l3: r5=MEM(sp+12);
Lbdde2: sp+=(4*4); /* pop {r2, r3, r4, r5} */
Lbdde4: r8=r2;
Lbdde6: r9=r3;
Lbdde8: r10=r4;
Lbddea: r11=r5;
Lbddec_l0: r4=MEM(sp+0);
Lbddec_l1: r5=MEM(sp+4);
Lbddec_l2: r6=MEM(sp+8);
Lbddec_l3: r7=MEM(sp+12);
Lbddec: sp+=(4*4); /* pop {r4, r5, r6, r7, pc} */
Lbddec_r: goto RET;
Lbddee: r1=sp+4;
Lbddf0: r3=7;
Lbddf2: MEM(sp+4)=r3;
Lbddf4: r3=r10;
Lbddf6: MEM(r1+4)=r3;
Lbddf8: MEM(r1+8)=r6;
Lbddfa: r0=r11;
Lbddfc: r0=F_bd778(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd778 完整 */
Lbde00: goto Lbdde0;
Lbde02: r1=sp+4;
Lbde04: r3=1;
Lbde06: goto Lbddf2;
Lbde08: r1=sp+4;
Lbde0a: r0=r10;
Lbde0c: r3=2;
Lbde0e: MEM(r1+4)=r0;
Lbde10: MEM(r1+8)=r6;
Lbde12: r0=r11;
Lbde14: MEM(sp+4)=r3;
Lbde16: r0=F_bd778(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd778 完整 */
Lbde1a: goto Lbdde0;
RET: return r0;
    return 0;
}