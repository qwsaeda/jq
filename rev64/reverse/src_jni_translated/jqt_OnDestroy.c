// 机械化翻译自原版 32 位 libjunqi.so @ 0x235d5 (OnDestroy) — 60 标签
// bl/blx 存根 r0=0（与回归一致）；.rodata/.text 常量池需运行时明文注入
#include <stdint.h>
#include "jqt_mem.h"
uint32_t jqt_OnDestroy(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t jqt_OnDestroy(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
L235d6: r3=MEM(144856+44);
L235d8: r4=MEM(144860+44);
L235da: {uint32_t a_=r0,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L235dc: r3=(144864+r3);
L235de: r0=MEM(r3);
L235e0: r4=(144868+r4);
L235e2: r0=0; /* bl 0xbdacc 存根 */
L235e6: r5=MEM(r4);
L235e8: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
L235ea: if(Z) goto L235fc;
L235ec: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
L235ee: r0=0; /* bl 0x234f8 存根 */
L235f2: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
L235f4: r0=0; /* blx 0x22bfc 存根 */
L235f8: r3=0;
L235fa: MEM(r4)=r3;
L235fc: r0=0; /* bl 0xba12c 存根 */
L23600_l0: r4=MEM(sp+0);
L23600_l1: r5=MEM(sp+4);
L23600_l2: r6=MEM(sp+8);
L23600: sp+=(3*4); /* pop {r4, r5, r6, pc} */
L23600_r: goto RET;
L23648: {int32_t A=(int32_t)r4,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r4>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
L2364a: if(Z) goto L236d0;
L2364c: r3=200;
L2364e: r3=((r3<<3))&0xffffffffu; Z=(r3==0); N=(((r3)>>31))&1;
L23650: {int32_t A=(int32_t)r4,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r4>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
L23652: if(Z) goto L236d0;
L23654: r1=MEM(144984+140);
L23656: r0=0; /* blx 0x22da0 存根 */
L2365a: {int32_t A=(int32_t)r0,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
L2365c: if(Z) goto L236c0;
L2365e: r1=180;
L23660: r2=148;
L23662: {uint32_t a_=r7,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
L23664: r1=((r1<<2))&0xffffffffu; Z=(r1==0); N=(((r1)>>31))&1;
L23666: r2=((r2<<3))&0xffffffffu; Z=(r2==0); N=(((r2)>>31))&1;
L23668: r0=0; /* bl 0xafc44 存根 */
L2366c: goto L23684;
L23684_l1: r4=MEM(sp+4);
L23684_l2: r5=MEM(sp+8);
L23684_l3: r6=MEM(sp+12);
L23684_l4: r7=MEM(sp+16);
L23684: sp+=(5*4); /* pop {r3, r4, r5, r6, r7, pc} */
L23684_r: goto RET;
L236c0: r1=180;
L236c2: r2=160;
L236c4: {uint32_t a_=r7,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
L236c6: r1=((r1<<2))&0xffffffffu; Z=(r1==0); N=(((r1)>>31))&1;
L236c8: r2=((r2<<3))&0xffffffffu; Z=(r2==0); N=(((r2)>>31))&1;
L236ca: r0=0; /* bl 0xafc44 存根 */
L236ce: goto L23684;
L236d0: r1=200;
L236d2: {uint32_t a_=r7,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
L236d4: r1=((r1<<2))&0xffffffffu; Z=(r1==0); N=(((r1)>>31))&1;
L236d6: r2=MEM(145112+20);
L236d8: r0=0; /* bl 0xafc44 存根 */
L236dc: goto L23684;
L22bfc_st0: MEM(r6+0)=r0;
L22bfc: r6=r6+4;
L22bfe: goto L23120;
L22da0_st0: MEM(r6+0)=r0;
L22da0: r6=r6+4;
L22da2: goto L232c4;
L23120: r0=0; /* blx 0x643c9c 存根 */
L23124_st0: MEM(r6+0)=r0;
L23124: r6=r6+4;
L23126: goto L23648;
L232c4: r0=0; /* blx 0x52be40 存根 */
L232c8_st0: MEM(r6+0)=r0;
L232c8: r6=r6+4;
L232ca: goto L237ec;
L237ec: ; /* 外部跳转目标 */
RET: return r0;
}
