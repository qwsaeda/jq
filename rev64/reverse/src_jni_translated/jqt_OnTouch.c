// 机械化翻译自原版 32 位 libjunqi.so @ 0x23439 (OnTouch) — 6 标签
// §124ai：bl 0xbdd2c 改真调 F_bdd2c（触摸→对局路径，非存根）
#include <stdint.h>
#include <android/log.h>
#include "jqt_mem.h"
#define JQT_LOG_TAG "junqi"
uint32_t jqt_OnTouch(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t F_bdd2c(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t jqt_OnTouch(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    __android_log_print(4, JQT_LOG_TAG, "OnTouch: 入口 r0=0x%08x (MotionEvent)", _r0);
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
L2343a: r3=MEM(144444+12);
L2343c: {uint32_t a_=r0,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L2343e: r3=(144448+r3);
L23440: r0=MEM(r3);
L23442: r0=F_bdd2c(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbdd2c 完整 */
    { static int _td=0; if(_td++ < 0x40) __android_log_print(4, JQT_LOG_TAG, "OnTouch: F_bdd2c 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
L23446_l0: r4=MEM(sp+0);
L23446: sp+=(1*4); /* pop {r4, pc} */
L23446_r: goto RET;
RET: return r0;
}
