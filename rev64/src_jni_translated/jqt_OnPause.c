// §124y-141: jqt_OnPause (JNI 入口 0x234c5) 改真调 F_bdcd4 (非存根)
// F_bdcd4 定义在 jqt_OnResume.c (与 F_bdc7c 同文件, 照 jqt_OnKey.c 模式)
#include <stdint.h>
#include "jqt_mem.h"
#include <android/log.h>
extern uint32_t F_bdcd4(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t jqt_OnPause(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);

uint32_t jqt_OnPause(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
L234c6: r3=MEM(144584+12);
L234c8: {uint32_t a_=r0,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L234ca: r3=(144588+r3);
L234cc: r0=MEM(r3);
L234ce: r0=F_bdcd4(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbdcd4 完整 (§124y-141, 非存根) */
L234d2_l0: r4=MEM(sp+0);
L234d2: sp+=(1*4); /* pop {r4, pc} */
L234d2_r: goto RET;
RET: return r0;
}
