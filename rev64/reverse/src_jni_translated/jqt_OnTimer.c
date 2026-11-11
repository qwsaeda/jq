// 机械化翻译自原版 32 位 libjunqi.so @ 0x23461 (OnTimer) — 6 标签
// bl/blx 存根 r0=0（与回归一致）；.rodata/.text 常量池需运行时明文注入
#include <stdint.h>
#include "jqt_mem.h"
uint32_t jqt_OnTimer(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t jqt_OnTimer(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
L23462: r3=MEM(144484+12);
L23464: {uint32_t a_=r0,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L23466: r3=(144488+r3);
L23468: r0=MEM(r3);
L2346a: r0=0; /* bl 0xbe930 存根 */
L2346e_l0: r4=MEM(sp+0);
L2346e: sp+=(1*4); /* pop {r4, pc} */
L2346e_r: goto RET;
RET: return r0;
}
