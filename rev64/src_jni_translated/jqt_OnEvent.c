// 机械化翻译自原版 32 位 libjunqi.so @ 0x233e1 (OnEvent) — 6 标签
// §124ai：bl 0xbd914 改真调 F_bd914（触摸→对局路径，非存根）
#include <stdint.h>
#include <android/log.h>
#include "jqt_mem.h"
#define JQT_LOG_TAG "junqi"
uint32_t jqt_OnEvent(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t F_bd914(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t jqt_OnEvent(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    __android_log_print(4, JQT_LOG_TAG, "OnEvent: 入口 r0=0x%08x r1=0x%08x", _r0, _r1);
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
L233e2: {uint32_t a_=r0,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L233e4: r0=MEM(144360+8);
L233e6: r0=(144360+r0);
L233e8: r0=MEM(r0);
L233ea: r0=F_bd914(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd914 完整 */
L233ee_l0: r4=MEM(sp+0);
L233ee: sp+=(1*4); /* pop {r4, pc} */
L233ee_r: goto RET;
RET: return r0;
}
