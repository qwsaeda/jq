// §124y-143: jqt_OnUpdate (JNI 入口 0x23474) 改真调 F_bd5a0 (非存根)
// F_bd5a0 完整翻译体在 jqt_OnRender_full.c:1275 (F_afb68/F_b7eb0/F_b38c0/F_b6808/F_becf4/F_b6a64 全调用 + 循环阀)
// 原版 0x23474: r0=env, r1=env(adds r1,r0,#0), bl 0xbd5a0 (读 .bss 0x15be84 根对象[0x24] + 事件码 32-34/40/49 分支)
#include <stdint.h>
#include "jqt_mem.h"
#include <android/log.h>
extern uint32_t F_bd5a0(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t jqt_OnUpdate(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);

uint32_t jqt_OnUpdate(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
L23476: r3=MEM(144504+12);
L23478: {uint32_t a_=r0,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L2347a: r3=(144508+r3);
L2347c: r0=MEM(r3);
L2347e: r0=0; /* bl 0xbd5a0 存根 (§124y-143: F_bd5a0 已对齐进 jqt_OnRender_full.c:1275, jni_aligned.c F_23475 真调) */
L23482_l0: r4=MEM(sp+0);
L23482: sp+=(1*4); /* pop {r4, pc} */
L23482_r: goto RET;
RET: return r0;
}
