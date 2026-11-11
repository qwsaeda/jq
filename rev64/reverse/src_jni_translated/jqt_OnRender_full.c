// 渲染路径完整调用链补充（gen_render_full.py 生成）— 4 渲染入口 BFS
// 仅含 OnCreate 链（jqt_OnCreate_full.c）没有的新增函数
#include <stdint.h>
extern uint32_t jqt_blx_dispatch(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);

#include "jqt_mem.h"
#include <GLES/gl.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <setjmp.h>
#include <wchar.h>
#include <math.h>
#include <semaphore.h>
#include <android/log.h>
extern void* _Znwj(unsigned int); extern void* _Znaj(unsigned int);
extern void _ZdlPv(void*); extern void _ZdaPv(void*);
extern float __aeabi_fsub(float,float); extern float __aeabi_fdiv(float,float);
extern int32_t __aeabi_idiv(int32_t,int32_t); extern uint32_t __aeabi_uidiv(uint32_t,uint32_t);
extern float __aeabi_l2f(int64_t);
extern uint32_t jq_jni_make_glview(void);
extern volatile uint32_t g_cur_fn;
uint32_t F_23461(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_23475(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_23489(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_2349d(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_7bf60(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_afbc0(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b3c20(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b3c28(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b3c40(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b3ecc(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b5a8c(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b5f58(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b6010(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b60b0(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b6168(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b66b0(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b6790(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_9d3d4(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b67c0(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b6890(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b69b4(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b6a64(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b7eb0(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b7f48(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_b8f20(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_bd548(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_bd5a0(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_bd74c(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_bd764(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_be930(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_becf4(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_bef78(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_bf140(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_bf4d0(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_c5d50(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_c5e38(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t F_23461(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0x23461;
L23462: r3=MEM(144484+12);
L23464: {uint32_t a_=r0,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L23466: r3=(144490+r3);
L23468: r0=MEM(r3);
L2346a: r0=F_be930(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbe930 完整 */
L2346e_l0: r4=MEM(sp+0);
L2346e: sp+=(1*4); /* pop {r4, pc} */
L2346e_r: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_23475(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0x23475;
    /* §124ag：诊断 glClear 已移除——F_9d458（真实 glClear）已在 F_23eec 强制 dispatch。
     * 若屏幕仍显色 → F_9d458 是真实清屏路径（验证）。 */
    /* §124au 攻 glClear 对象图：渲染循环每帧强制 glClear 出帧——对象图分歧下原版
     * glClear dispatch 不到（vtable=0x1595f4 未构造），这里每帧补清屏色帧。
     * F_23475 OnUpdate 在 GLThread 每帧调（GLView$Renderer.onDrawFrame）→ jqt_gl_bridge
     * 持续 eglSwapBuffers 出清屏色（非一次性首帧）。幂等：首帧 eglMakeCurrent，后续 glClear。 */
    extern void jqt_gl_bridge(uint32_t clear_arg);
    jqt_gl_bridge(0xff9c6630);   /* 蓝 R=48 G=102 B=156 A=255（清屏色） */
    { static int _fr=0; _fr++; if ((_fr & 0x3f)==0)
        __android_log_print(4,"junqi","F_23475 每帧 glClear #%d tid=%d", _fr, (int)gettid()); }
L23476: r3=MEM(144504+12);
L23478: {uint32_t a_=r0,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L2347a: r3=(144510+r3);
L2347c: r0=MEM(r3);
L2347e: r0=F_bd5a0(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd5a0 完整 */
L23482_l0: r4=MEM(sp+0);
L23482: sp+=(1*4); /* pop {r4, pc} */
L23482_r: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_23489(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0x23489;
L2348a: {uint32_t a_=r0,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L2348c: r0=MEM(144528+8);
L2348e: r0=(144530+r0);
L23490: r0=MEM(r0);
L23492: r0=F_bd74c(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd74c 完整 */
L23496_l0: r4=MEM(sp+0);
L23496: sp+=(1*4); /* pop {r4, pc} */
L23496_r: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_2349d(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0x2349d;
L2349e: r3=MEM(144544+12);
L234a0: {uint32_t a_=r0,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L234a2: r3=(144550+r3);
L234a4: r0=MEM(r3);
L234a6: r0=F_bd764(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd764 完整 */
L234aa_l0: r4=MEM(sp+0);
L234aa: sp+=(1*4); /* pop {r4, pc} */
L234aa_r: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_7bf60(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0x7bf60;
L7bf60: sp-=(2*4); /* push r2, r3 */
L7bf62: sp-=(3*4); /* push r4, r5, lr */
L7bf62_p0: MEM(sp+0)=r4;
L7bf62_p1: MEM(sp+4)=r5;
L7bf64: sp-=0xc;
L7bf66: r3=sp+0x18;
L7bf68_ld0: r2=MEM(r3+0);
L7bf68: r3=r3+4;
L7bf6a: r4=0xe6;
L7bf6c: r4=((r4<<1))&0xffffffffu; Z=(r4==0); N=(((r4)>>31))&1;
L7bf6e: MEM(sp+4)=r3;
L7bf70: r5=MEM(r0);
L7bf72: r4=MEM(r5+r4);
L7bf74: { r0=jqt_blx_dispatch(r4, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r4 vtable dispatch */
L7bf76: sp+=0xc;
L7bf78_l0: r4=MEM(sp+0);
L7bf78_l1: r5=MEM(sp+4);
L7bf78: sp+=(2*4); /* pop {r4, r5} */
L7bf7a: sp+=(1*4); /* pop {r3} */
L7bf7c: sp+=8;
L7bf7e: r0=0; goto RET; /* bx r3 存根 */
RET: return r0;
    return 0;
}

uint32_t F_afbc0(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xafbc0;
Lafbc0: MEM(r0)=r1;
Lafbc2: MEM(r0+4)=r2;
Lafbc4: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_b3c20(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb3c20;
Lb3c20: r0=MEM(r0);
Lb3c22: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_b3c28(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb3c28;
Lb3c28: r3=MEM(r0+4);
Lb3c2a: {int32_t A=(int32_t)r1,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r1>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb3c2c: if((Z||(N!=V))) goto Lb3c30;
Lb3c2e: {uint32_t a_=r3,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lb3c30: r3=MEM(r0+12);
Lb3c32: MEM(r0+8)=r1;
Lb3c34: {int32_t A=(int32_t)r3,B=(int32_t)r1;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=r1);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb3c36: if((Z||(N!=V))) goto Lb3c3a;
Lb3c38: MEM(r0+12)=r1;
Lb3c3a: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_b3c40(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb3c40;
Lb3c40: r3=MEM(r0+8);
Lb3c42: {int32_t A=(int32_t)r3,B=(int32_t)r1;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=r1);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb3c44: if((N!=V)) goto Lb3c4c;
Lb3c46: MEM(r0+12)=r1;
Lb3c48: r0=1;
Lb3c4a: goto RET;
Lb3c4c: MEM(r0+12)=r3;
Lb3c4e: r0=0;
Lb3c50: goto Lb3c4a;
RET: return r0;
    return 0;
}

uint32_t F_b3ecc(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb3ecc;
Lb3ecc: sp-=(4*4); /* push r4, r5, r6, lr */
Lb3ecc_p0: MEM(sp+0)=r4;
Lb3ecc_p1: MEM(sp+4)=r5;
Lb3ecc_p2: MEM(sp+8)=r6;
Lb3ece: r6=MEM(r0+12);
Lb3ed0: r5=MEM(r0+8);
Lb3ed2: {uint32_t a_=r0,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb3ed4: {int32_t A=(int32_t)r5,B=(int32_t)r6;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r6);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb3ed6: if((Z||(N!=V))) goto Lb3ef4;
Lb3ed8: r0=MEM(r0);
Lb3eda: {uint32_t a_=r0,b_=r6; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb3edc: r2=MEMB(r0);
Lb3ede: {int32_t A=(int32_t)r2,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r2>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb3ee0: if(Z) goto Lb3ef8;
Lb3ee2: {uint32_t a_=r5,b_=r6; r5=(a_-b_); Z=(r5==0); C=(a_>=b_); N=((r5)>>31)&1; V=(((a_^b_)&(a_^r5))>>31)&1;}
Lb3ee4: r2=0;
Lb3ee6: goto Lb3eee;
Lb3ee8: r4=MEMB(r0+r2);
Lb3eea: {int32_t A=(int32_t)r4,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r4>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb3eec: if(Z) goto Lb3efa;
Lb3eee: {uint32_t a_=r2,b_=1; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb3ef0: {int32_t A=(int32_t)r2,B=(int32_t)r5;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r2>=r5);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb3ef2: if(!Z) goto Lb3ee8;
Lb3ef4: {uint32_t a_=r1,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb3ef6_l0: r4=MEM(sp+0);
Lb3ef6_l1: r5=MEM(sp+4);
Lb3ef6_l2: r6=MEM(sp+8);
Lb3ef6: sp+=(3*4); /* pop {r4, r5, r6, pc} */
Lb3ef6_r: goto RET;
Lb3ef8: r2=0;
Lb3efa: {uint32_t a_=r6,b_=1; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb3efc: {uint32_t a_=r6,b_=r2; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb3efe: MEM(r3+12)=r2;
Lb3f00: goto Lb3ef6;
RET: return r0;
    return 0;
}

uint32_t F_b5a8c(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb5a8c;
Lb5a8c: sp-=(5*4); /* push r4, r5, r6, r7, lr */
Lb5a8c_p0: MEM(sp+0)=r4;
Lb5a8c_p1: MEM(sp+4)=r5;
Lb5a8c_p2: MEM(sp+8)=r6;
Lb5a8c_p3: MEM(sp+12)=r7;
Lb5a8e: r7=r10;
Lb5a90: r6=r9;
Lb5a92: r5=r8;
Lb5a94: sp-=(3*4); /* push r5, r6, r7 */
Lb5a94_p0: MEM(sp+0)=r5;
Lb5a94_p1: MEM(sp+4)=r6;
Lb5a94_p2: MEM(sp+8)=r7;
Lb5a96: {uint32_t a_=r0,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lb5a98: r4=MEM(r5+4);
Lb5a9a: r7=MEM(r5+8);
Lb5a9c: sp-=8;
Lb5a9e: r6=sp;
Lb5aa0: {uint32_t a_=r2,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb5aa2: MEM(sp)=r2;
Lb5aa4: MEM(sp+4)=r3;
Lb5aa6: r10=r3;
Lb5aa8: {int32_t A=(int32_t)r7,B=(int32_t)r4;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r7>=r4);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb5aaa: if(Z) goto Lb5b30;
Lb5aac: r2=1;
Lb5aae: {uint32_t a_=r2,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb5ab0: r12=r2;
Lb5ab2: r2=r10;
Lb5ab4: r3&=r2; Z=(r3==0); N=((r3)>>31)&1;
Lb5ab6: r8=r3;
Lb5ab8: goto Lb5ac0;
Lb5aba: {uint32_t a_=r4,b_=12; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb5abc: {int32_t A=(int32_t)r7,B=(int32_t)r4;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r7>=r4);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb5abe: if(Z) goto Lb5b30;
Lb5ac0: r3=MEM(r4);
Lb5ac2: {int32_t A=(int32_t)r3,B=(int32_t)r1;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=r1);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb5ac4: if(!Z) goto Lb5aba;
Lb5ac6: r3=MEM(r4+4);
Lb5ac8: {int32_t A=(int32_t)r0,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb5aca: if(!Z) goto Lb5aba;
Lb5acc: r3=MEM(r4+8);
Lb5ace: r9=r3;
Lb5ad0: {int32_t A=(int32_t)r9,B=(int32_t)r10;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r9>=r10);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb5ad2: if(Z) goto Lb5ae4;
Lb5ad4: {int32_t A=(int32_t)r0,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb5ad6: if(!Z) goto Lb5aba;
Lb5ad8: r2=r8;
Lb5ada: {int32_t A=(int32_t)r2,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r2>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb5adc: if(!Z) goto Lb5aba;
Lb5ade: r2=r12;
Lb5ae0: {uint32_t t=r3&r2; Z=(t==0); N=(t>>31)&1;}
Lb5ae2: if(!Z) goto Lb5aba;
Lb5ae4: r3=MEM(744168+84);
Lb5ae6: r2=MEM(sp);
Lb5ae8: r3=(744172+r3);
Lb5aea: r0=MEM(r3);
Lb5aec: r3=MEM(r6+4);
Lb5aee: r0=F_b6168(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb6168 完整 */
Lb5af2: r2=MEM(r5+8);
Lb5af4: {uint32_t a_=r4,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb5af6: {uint32_t a_=r6,b_=12; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb5af8: {uint32_t a_=r6,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb5afa: {int32_t A=(int32_t)r6,B=(int32_t)r2;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=r2);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb5afc: if(Z) goto Lb5b2c;
Lb5afe: r7=MEM(744192+64);
Lb5b00: {uint32_t a_=r2,b_=r6; r1=(a_-b_); Z=(r1==0); C=(a_>=b_); N=((r1)>>31)&1; V=(((a_^b_)&(a_^r1))>>31)&1;}
Lb5b02: r1=(((int32_t)r1>>(2)))&0xffffffffu; Z=(r1==0); N=(((r1)>>31))&1;
Lb5b04: {uint32_t a_=r7,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb5b06: r0*=r1;
Lb5b08: {int32_t A=(int32_t)r0,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb5b0a: if((!Z&&(N==V))) goto Lb5b10;
Lb5b0c: goto Lb5b2c;
Lb5b0e: {uint32_t a_=r6,b_=12; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb5b10: {uint32_t a_=r3,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lb5b12_ld0: r7=MEM(r1+0);
Lb5b12: r1=r1+4;
Lb5b14: {uint32_t a_=r4,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb5b16: {uint32_t a_=r0,b_=1; r0=(a_-b_); Z=(r0==0); C=(a_>=b_); N=((r0)>>31)&1; V=(((a_^b_)&(a_^r0))>>31)&1;}
Lb5b18_st0: MEM(r2+0)=r7;
Lb5b18: r2=r2+4;
Lb5b1a: r7=MEM(r3+4);
Lb5b1c: {uint32_t a_=r3,b_=12; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb5b1e: MEM(r4+4)=r7;
Lb5b20: r1=MEM(r1+4);
Lb5b22: {uint32_t a_=r6,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb5b24: MEM(r2+4)=r1;
Lb5b26: {int32_t A=(int32_t)r0,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb5b28: if(!Z) goto Lb5b0e;
Lb5b2a: r2=MEM(r5+8);
Lb5b2c: {uint32_t a_=r2,b_=12; r2=(a_-b_); Z=(r2==0); C=(a_>=b_); N=((r2)>>31)&1; V=(((a_^b_)&(a_^r2))>>31)&1;}
Lb5b2e: MEM(r5+8)=r2;
Lb5b30: sp+=8;
Lb5b32_l2: r4=MEM(sp+8);
Lb5b32: sp+=(3*4); /* pop {r2, r3, r4} */
Lb5b34: r8=r2;
Lb5b36: r9=r3;
Lb5b38: r10=r4;
Lb5b3a_l0: r4=MEM(sp+0);
Lb5b3a_l1: r5=MEM(sp+4);
Lb5b3a_l2: r6=MEM(sp+8);
Lb5b3a_l3: r7=MEM(sp+12);
Lb5b3a: sp+=(4*4); /* pop {r4, r5, r6, r7, pc} */
Lb5b3a_r: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_b5f58(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb5f58;
    { static int _sd=0; if(_sd++ < 0x400) __android_log_print(6,"junqi","F_b5f58 ENTRY r0=0x%08x r1=0x%08x tid=%d", r0, r1, (int)gettid()); }
Lb5f58: {uint32_t a_=r0,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb5f5a: r1=MEM(r3+76);
Lb5f5c: r0=MEM(r0+72);
Lb5f5e: goto RET;
RET: { static int _g=0; if (_g++ < 0x400) __android_log_print(6,"junqi","F_b5f58 RET r0=0x%08x tid=%d", r0, (int)gettid()); } return r0;
    return 0;
}

uint32_t F_b6010(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb6010;
Lb6010: sp-=(5*4); /* push r4, r5, r6, r7, lr */
Lb6010_p0: MEM(sp+0)=r4;
Lb6010_p1: MEM(sp+4)=r5;
Lb6010_p2: MEM(sp+8)=r6;
Lb6010_p3: MEM(sp+12)=r7;
Lb6012: sp-=12;
Lb6014: MEM(sp)=r2;
Lb6016: MEM(sp+4)=r3;
Lb6018: {uint32_t a_=r2,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb601a: {uint32_t a_=r3,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb601c: r3=MEM(r0+4);
Lb601e: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6020: if(Z) goto Lb60ac;
Lb6022: {uint32_t a_=r0,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb6024: goto Lb6030;
Lb6026: r5=MEM(r3+8);
Lb6028: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb602a: if(Z) goto Lb603e;
Lb602c: {uint32_t a_=r3,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb602e: {uint32_t a_=r5,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb6030: r5=MEM(r3+16);
Lb6032: {int32_t A=(int32_t)r5,B=(int32_t)r1;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r1);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6034: if((C)) goto Lb6026;
Lb6036: r5=MEM(r3+12);
Lb6038: {uint32_t a_=r6,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb603a: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb603c: if(!Z) goto Lb602c;
Lb603e: r7=1;
Lb6040: {int32_t A=(int32_t)r0,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6042: if(Z) goto Lb6062;
Lb6044: r5=MEM(r3+16);
Lb6046: {int32_t A=(int32_t)r5,B=(int32_t)r1;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r1);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6048: if(!Z) goto Lb6062;
Lb604a: r5=MEM(r3+24);
Lb604c: {int32_t A=(int32_t)r5,B=(int32_t)r4;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r4);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb604e: if(Z) goto Lb6066;
Lb6050: r5=MEM(r3+12);
Lb6052: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6054: if(Z) goto Lb608c;
Lb6056: {uint32_t a_=r5,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb6058: r5=MEM(r5+8);
Lb605a: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb605c: if(!Z) goto Lb6056;
Lb605e: {int32_t A=(int32_t)r0,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6060: if(!Z) goto Lb6044;
Lb6062: sp+=12;
Lb6064_l0: r4=MEM(sp+0);
Lb6064_l1: r5=MEM(sp+4);
Lb6064_l2: r6=MEM(sp+8);
Lb6064_l3: r7=MEM(sp+12);
Lb6064: sp+=(4*4); /* pop {r4, r5, r6, r7, pc} */
Lb6064_r: goto RET;
Lb6066: r5=MEM(r3+28);
Lb6068: {int32_t A=(int32_t)r5,B=(int32_t)r2;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r2);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb606a: if(Z) goto Lb6076;
Lb606c: {int32_t A=(int32_t)r4,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r4>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb606e: if(!Z) goto Lb6050;
Lb6070: r5|=r2; Z=(r5==0); N=((r5)>>31)&1;
Lb6072: {uint32_t t=r5&r7; Z=(t==0); N=(t>>31)&1;}
Lb6074: if(!Z) goto Lb6050;
Lb6076: r2=56;
Lb6078: r1=MEMB(r3+r2);
Lb607a: {int32_t A=(int32_t)r1,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r1>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb607c: if(!Z) goto Lb6062;
Lb607e: r1=MEM(r0+76);
Lb6080: r0=MEM(r0+72);
Lb6082: MEM(r3+48)=r0;
Lb6084: MEM(r3+52)=r1;
Lb6086: r1=1;
Lb6088: MEMB(r3+r2)=r1;
Lb608a: goto Lb6062;
Lb608c: r5=MEM(r3+4);
Lb608e: r6=MEM(r5+12);
Lb6090: r12=r6;
Lb6092: r6=0;
Lb6094: {int32_t A=(int32_t)r3,B=(int32_t)r12;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=r12);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6096: if(!Z) goto Lb60a4;
Lb6098: {uint32_t a_=r5,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb609a: r5=MEM(r5+4);
Lb609c: r6=MEM(r5+12);
Lb609e: {int32_t A=(int32_t)r6,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb60a0: if(Z) goto Lb6098;
Lb60a2: r6=MEM(r3+12);
Lb60a4: {int32_t A=(int32_t)r5,B=(int32_t)r6;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r6);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb60a6: if(Z) goto Lb6040;
Lb60a8: {uint32_t a_=r5,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb60aa: goto Lb6040;
Lb60ac: {uint32_t a_=r0,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb60ae: goto Lb603e;
RET: return r0;
    return 0;
}

uint32_t F_b60b0(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb60b0;
Lb60b0: sp-=(5*4); /* push r4, r5, r6, r7, lr */
Lb60b0_p0: MEM(sp+0)=r4;
Lb60b0_p1: MEM(sp+4)=r5;
Lb60b0_p2: MEM(sp+8)=r6;
Lb60b0_p3: MEM(sp+12)=r7;
Lb60b2: sp-=12;
Lb60b4: MEM(sp)=r2;
Lb60b6: MEM(sp+4)=r3;
Lb60b8: {uint32_t a_=r2,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb60ba: {uint32_t a_=r3,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb60bc: r3=MEM(r0+4);
Lb60be: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb60c0: if(Z) goto Lb6164;
Lb60c2: {uint32_t a_=r0,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb60c4: goto Lb60d0;
Lb60c6: r5=MEM(r3+8);
Lb60c8: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb60ca: if(Z) goto Lb60de;
Lb60cc: {uint32_t a_=r3,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb60ce: {uint32_t a_=r5,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb60d0: r5=MEM(r3+16);
Lb60d2: {int32_t A=(int32_t)r5,B=(int32_t)r1;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r1);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb60d4: if((C)) goto Lb60c6;
Lb60d6: r5=MEM(r3+12);
Lb60d8: {uint32_t a_=r6,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb60da: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb60dc: if(!Z) goto Lb60cc;
Lb60de: r7=1;
Lb60e0: {int32_t A=(int32_t)r0,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb60e2: if(Z) goto Lb6102;
Lb60e4: r5=MEM(r3+16);
Lb60e6: {int32_t A=(int32_t)r5,B=(int32_t)r1;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r1);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb60e8: if(!Z) goto Lb6102;
Lb60ea: r5=MEM(r3+24);
Lb60ec: {int32_t A=(int32_t)r5,B=(int32_t)r4;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r4);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb60ee: if(Z) goto Lb6106;
Lb60f0: r5=MEM(r3+12);
Lb60f2: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb60f4: if(Z) goto Lb6144;
Lb60f6: {uint32_t a_=r5,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb60f8: r5=MEM(r5+8);
Lb60fa: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb60fc: if(!Z) goto Lb60f6;
Lb60fe: {int32_t A=(int32_t)r0,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6100: if(!Z) goto Lb60e4;
Lb6102: sp+=12;
Lb6104_l0: r4=MEM(sp+0);
Lb6104_l1: r5=MEM(sp+4);
Lb6104_l2: r6=MEM(sp+8);
Lb6104_l3: r7=MEM(sp+12);
Lb6104: sp+=(4*4); /* pop {r4, r5, r6, r7, pc} */
Lb6104_r: goto RET;
Lb6106: r5=MEM(r3+28);
Lb6108: {int32_t A=(int32_t)r5,B=(int32_t)r2;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r2);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb610a: if(Z) goto Lb6116;
Lb610c: {int32_t A=(int32_t)r4,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r4>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb610e: if(!Z) goto Lb60f0;
Lb6110: r5|=r2; Z=(r5==0); N=((r5)>>31)&1;
Lb6112: {uint32_t t=r5&r7; Z=(t==0); N=(t>>31)&1;}
Lb6114: if(!Z) goto Lb60f0;
Lb6116: r2=56;
Lb6118: r1=MEMB(r3+r2);
Lb611a: {int32_t A=(int32_t)r1,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r1>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb611c: if(Z) goto Lb6102;
Lb611e: r4=MEM(r0+72);
Lb6120: r5=MEM(r0+76);
Lb6122: r0=MEM(r3+40);
Lb6124: r1=MEM(r3+44);
Lb6126: {uint32_t a_=r4,b_=r0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb6128: r5+=r1+C;
Lb612a: {uint32_t a_=r4,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb612c: {uint32_t a_=r5,b_=0; r7=(a_+b_); Z=(r7==0); C=(r7<a_); N=((r7)>>31)&1; V=(((a_^r7)&(b_^r7))>>31)&1;}
Lb612e: r0=MEM(r3+48);
Lb6130: r1=MEM(r3+52);
Lb6132: {uint32_t a_=r6,b_=r0; r6=(a_-b_); Z=(r6==0); C=(a_>=b_); N=((r6)>>31)&1; V=(((a_^b_)&(a_^r6))>>31)&1;}
Lb6134: r7-=r1+(1-C);
Lb6136: {uint32_t a_=r7,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lb6138: {uint32_t a_=r6,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb613a: MEM(r3+40)=r0;
Lb613c: MEM(r3+44)=r1;
Lb613e: r1=0;
Lb6140: MEMB(r3+r2)=r1;
Lb6142: goto Lb6102;
Lb6144: r5=MEM(r3+4);
Lb6146: r6=MEM(r5+12);
Lb6148: r12=r6;
Lb614a: r6=0;
Lb614c: {int32_t A=(int32_t)r3,B=(int32_t)r12;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=r12);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb614e: if(!Z) goto Lb615c;
Lb6150: {uint32_t a_=r5,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb6152: r5=MEM(r5+4);
Lb6154: r6=MEM(r5+12);
Lb6156: {int32_t A=(int32_t)r6,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6158: if(Z) goto Lb6150;
Lb615a: r6=MEM(r3+12);
Lb615c: {int32_t A=(int32_t)r5,B=(int32_t)r6;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r6);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb615e: if(Z) goto Lb60e0;
Lb6160: {uint32_t a_=r5,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb6162: goto Lb60e0;
Lb6164: {uint32_t a_=r0,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb6166: goto Lb60de;
RET: return r0;
    return 0;
}

uint32_t F_b6168(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb6168;
Lb6168: sp-=(5*4); /* push r4, r5, r6, r7, lr */
Lb6168_p0: MEM(sp+0)=r4;
Lb6168_p1: MEM(sp+4)=r5;
Lb6168_p2: MEM(sp+8)=r6;
Lb6168_p3: MEM(sp+12)=r7;
Lb616a: r7=r8;
Lb616c: sp-=(1*4); /* push r7 */
Lb616c_p0: MEM(sp+0)=r7;
Lb616e: sp-=8;
Lb6170: {uint32_t a_=r2,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb6172: MEM(sp)=r2;
Lb6174: MEM(sp+4)=r3;
Lb6176: {uint32_t a_=r3,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb6178: r3=MEM(r0+4);
Lb617a: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb617c: if(Z) goto Lb6208;
Lb617e: {uint32_t a_=r0,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb6180: goto Lb618c;
Lb6182: r5=MEM(r3+8);
Lb6184: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6186: if(Z) goto Lb619a;
Lb6188: {uint32_t a_=r3,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb618a: {uint32_t a_=r5,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb618c: r5=MEM(r3+16);
Lb618e: {int32_t A=(int32_t)r5,B=(int32_t)r1;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r1);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6190: if((C)) goto Lb6182;
Lb6192: r5=MEM(r3+12);
Lb6194: {uint32_t a_=r6,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb6196: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6198: if(!Z) goto Lb6188;
Lb619a: r7=1;
Lb619c: {uint32_t a_=r2,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lb619e: r5&=r7; Z=(r5==0); N=((r5)>>31)&1;
Lb61a0: r12=r5;
Lb61a2: {int32_t A=(int32_t)r0,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb61a4: if(Z) goto Lb61c6;
Lb61a6: r5=MEM(r3+16);
Lb61a8: {int32_t A=(int32_t)r5,B=(int32_t)r1;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r1);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb61aa: if(!Z) goto Lb61c6;
Lb61ac: r5=MEM(r3+24);
Lb61ae: r6=MEM(r3+28);
Lb61b0: {int32_t A=(int32_t)r4,B=(int32_t)r5;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r4>=r5);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb61b2: if(Z) goto Lb61ce;
Lb61b4: r5=MEM(r3+12);
Lb61b6: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb61b8: if(Z) goto Lb61e8;
Lb61ba: {uint32_t a_=r5,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb61bc: r5=MEM(r5+8);
Lb61be: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb61c0: if(!Z) goto Lb61ba;
Lb61c2: {int32_t A=(int32_t)r0,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb61c4: if(!Z) goto Lb61a6;
Lb61c6: sp+=8;
Lb61c8: sp+=(1*4); /* pop {r2} */
Lb61ca: r8=r2;
Lb61cc_l0: r4=MEM(sp+0);
Lb61cc_l1: r5=MEM(sp+4);
Lb61cc_l2: r6=MEM(sp+8);
Lb61cc_l3: r7=MEM(sp+12);
Lb61cc: sp+=(4*4); /* pop {r4, r5, r6, r7, pc} */
Lb61cc_r: goto RET;
Lb61ce: {int32_t A=(int32_t)r6,B=(int32_t)r2;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=r2);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb61d0: if(Z) goto Lb61e0;
Lb61d2: {int32_t A=(int32_t)r4,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r4>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb61d4: if(!Z) goto Lb61b4;
Lb61d6: r5=r12;
Lb61d8: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb61da: if(!Z) goto Lb61b4;
Lb61dc: {uint32_t t=r6&r7; Z=(t==0); N=(t>>31)&1;}
Lb61de: if(!Z) goto Lb61b4;
Lb61e0: r1=1;
Lb61e2: r2=57;
Lb61e4: MEMB(r3+r2)=r1;
Lb61e6: goto Lb61c6;
Lb61e8: r5=MEM(r3+4);
Lb61ea: r6=MEM(r5+12);
Lb61ec: r8=r6;
Lb61ee: r6=0;
Lb61f0: {int32_t A=(int32_t)r3,B=(int32_t)r8;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=r8);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb61f2: if(!Z) goto Lb6200;
Lb61f4: {uint32_t a_=r5,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb61f6: r5=MEM(r5+4);
Lb61f8: r6=MEM(r5+12);
Lb61fa: {int32_t A=(int32_t)r6,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb61fc: if(Z) goto Lb61f4;
Lb61fe: r6=MEM(r3+12);
Lb6200: {int32_t A=(int32_t)r6,B=(int32_t)r5;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=r5);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6202: if(Z) goto Lb61a2;
Lb6204: {uint32_t a_=r5,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb6206: goto Lb61a2;
Lb6208: {uint32_t a_=r0,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb620a: goto Lb619a;
RET: return r0;
    return 0;
}

uint32_t F_b66b0(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb66b0;
Lb66b0: sp-=(4*4); /* push r4, r5, r6, lr */
Lb66b0_p0: MEM(sp+0)=r4;
Lb66b0_p1: MEM(sp+4)=r5;
Lb66b0_p2: MEM(sp+8)=r6;
Lb66b2: r3=84;
Lb66b4: r3=MEMB(r0+r3);
Lb66b6: {uint32_t a_=r0,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb66b8: {uint32_t a_=r1,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lb66ba: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb66bc: if(!Z) goto Lb6700;
Lb66be: r3=MEM(r1);
Lb66c0: r6=MEM(r1+4);
Lb66c2: r2=MEM(r4+60);
Lb66c4: {uint32_t a_=r3,b_=r2; r3=(a_-b_); Z=(r3==0); C=(a_>=b_); N=((r3)>>31)&1; V=(((a_^b_)&(a_^r3))>>31)&1;}
Lb66c6: MEM(r5)=r3;
Lb66c8: r2=MEM(r4+64);
Lb66ca: {uint32_t a_=r6,b_=r2; r6=(a_-b_); Z=(r6==0); C=(a_>=b_); N=((r6)>>31)&1; V=(((a_^b_)&(a_^r6))>>31)&1;}
Lb66cc: MEM(r5+4)=r6;
Lb66ce: r1=MEM(r4+80);
Lb66d0: {int32_t A=(int32_t)r1,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r1>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb66d2: if((Z||(N!=V))) goto Lb66ee;
Lb66d4: r2=MEM(r4+76);
Lb66d6: {uint32_t a_=r2,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb66d8: r0*=r3;
Lb66da: { r0=(uint32_t)__aeabi_idiv((int32_t)r0,(int32_t)r1); } /* blx 0x23304 __aeabi_idiv 真实 */
Lb66de: MEM(r5)=r0;
Lb66e0: r3=MEM(r4+76);
Lb66e2: r1=MEM(r4+80);
Lb66e4: {uint32_t a_=r3,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb66e6: r0*=r6;
Lb66e8: { r0=(uint32_t)__aeabi_idiv((int32_t)r0,(int32_t)r1); } /* blx 0x23304 __aeabi_idiv 真实 */
Lb66ec: MEM(r5+4)=r0;
Lb66ee: r3=192;
Lb66f0: r0=MEM(r4+r3);
Lb66f2: {int32_t A=(int32_t)r0,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb66f4: if(Z) goto Lb66fe;
Lb66f6: r3=MEM(r0);
Lb66f8: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lb66fa: r3=MEM(r3+48);
Lb66fc: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lb66fe_l0: r4=MEM(sp+0);
Lb66fe_l1: r5=MEM(sp+4);
Lb66fe_l2: r6=MEM(sp+8);
Lb66fe: sp+=(3*4); /* pop {r4, r5, r6, pc} */
Lb66fe_r: goto RET;
Lb6700: r6=MEM(r1+4);
Lb6702: r2=MEM(r1);
Lb6704: MEM(r5)=r6;
Lb6706: {uint32_t a_=r6,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb6708: r6=MEM(r0+52);
Lb670a: {uint32_t a_=r6,b_=r2; r6=(a_-b_); Z=(r6==0); C=(a_>=b_); N=((r6)>>31)&1; V=(((a_^b_)&(a_^r6))>>31)&1;}
Lb670c: MEM(r1+4)=r6;
Lb670e: goto Lb66c2;
RET: return r0;
    return 0;
}

uint32_t F_b6790(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    __android_log_print(6,"junqi","F_b6790 入口 r0(this)=0x%08x MEM(0x15be88)=0x%08x", r0, MEM(0x15be88));    { uint32_t _vt=MEM(r0); __android_log_print(6,"junqi","F_b6790 vtable=0x%08x slots[0..5]=0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x", _vt, MEM(_vt+0),MEM(_vt+4),MEM(_vt+8),MEM(_vt+12),MEM(_vt+16),MEM(_vt+20)); }
    { /* §124af: 强制创建 glClear 单例（vtable=0x1595c0，slot13=0x9d459=F_9d458）——
       * F_9d3d4 是原版唯一创建点，但 F_242f4（其调用者）是存根 → 单例从未创建 → glClear 从不执行 → 黑屏。
       * 直接调 F_9d3d4() 创建单例（幂等，已存在则直接返回）。 */
      uint32_t _gl = F_9d3d4(0,0,0,0,0,0,0,0,0,0,0,0,_sp);
      __android_log_print(6,"junqi","F_b6790 glClear单例=0x%08x vtable=0x%08x slot13=0x%08x", _gl, MEM(_gl), MEM(MEM(_gl)+52));
    }
    { /* §124ae: 全对象图 dump——遍历 render obj 内所有指针，打印其 vtable，找 glClear 类 */
      for(int _i=0;_i<0x400;_i+=4){ uint32_t _p=MEM(r0+_i);
        if((_p>0xa0000000u && _p<0xf0000000u) || (_p>=0x155000u && _p<0x1600000u)){ uint32_t _vt=MEM(_p);
          __android_log_print(6,"junqi","F_b6790 obj[+0x%x]=0x%08x vtable=0x%08x", _i,_p,_vt);
        }
      }
    }
    { uint32_t _sub=MEM(r0+0xc0); if(_sub){ uint32_t _svt=MEM(_sub); __android_log_print(6,"junqi","F_b6790 subobj=0x%08x vtable=0x%08x slot16=0x%08x slot24=0x%08x slot36=0x%08x", _sub,_svt,MEM(_svt+16),MEM(_svt+24),MEM(_svt+36)); } else __android_log_print(6,"junqi","F_b6790 subobj=NULL"); }
    { /* 扫描渲染对象图找 glClear 类 vtable=0x1595f4 */
      uint32_t _base=r0; int _found=0;
      for(int _i=0;_i<0x200 && !_found;_i+=4){ uint32_t _p=MEM(_base+_i);
        if(_p>0xb0000000u && _p<0xc0000000u){ uint32_t _vt=MEM(_p);
          if(_vt==0x1595f4 || _vt==0x1595f4+1){ __android_log_print(6,"junqi","F_b6790 找到glClear类! obj=0x%08x at r0+0x%x vtable=0x%08x slot0=0x%08x", _p,_i,_vt,MEM(_vt)); _found=1; }
        }
      }
      if(!_found) __android_log_print(6,"junqi","F_b6790 渲染对象图0..0x200 无vtable=0x1595f4");
      /* ★ §124al：渲染器单例 0x15c0a0（.bss）诊断——对象图分歧核心 */
      { uint32_t _sing=MEM(0x15c0a0); uint32_t _svt=_sing?MEM(_sing):0;
        __android_log_print(6,"junqi","F_b6790 单例0x15c0a0=0x%08x vtable=0x%08x (期望0x1595c0)", _sing,_svt); }
      /* §124zz-6：渲染器单例 0x162400（vtable 0x1595c0）+ 纹理槽 this[76] 诊断 */
      { uint32_t _rs=MEM(0x162400);
        if (_rs) {
          uint32_t _rvt=MEM(_rs);
          uint32_t _rid=MEM(_rs+76);
          uint32_t _rpix=MEM(_rs+28);
          uint32_t _rw=MEM(_rs+32), _rh=MEM(_rs+36);
          __android_log_print(6,"junqi","F_b6790 渲染器单例0x162400=0x%08x vtable=0x%08x this[76](texID)=0x%08x this[28](px)=0x%08x w=%u h=%u",
            _rs, _rvt, _rid, _rpix, _rw, _rh);
        } else __android_log_print(6,"junqi","F_b6790 渲染器单例0x162400=0（未构造）"); }
    }
    { /* 扫描子对象字段 + 全局 .bss 找 vtable=0x1595f4 */
      uint32_t _sub2=MEM(r0+0xc0); int _f2=0;
      if(_sub2){ for(int _i=0;_i<0x300 && !_f2;_i+=4){ uint32_t _p=MEM(_sub2+_i);
        if(_p>0xb0000000u && _p<0xc0000000u){ uint32_t _vt=MEM(_p);
          if(_vt==0x1595f4||_vt==0x1595f5){ __android_log_print(6,"junqi","F_b6790 子对象图找到glClear! obj=0x%08x at+0x%x vt=0x%08x", _p,_i,_vt); _f2=1; } } } }
      for(int _i=0;_i<0x400 && !_f2;_i+=4){ uint32_t _p=MEM(0x15be80+_i);
        if(_p>0xb0000000u && _p<0xc0000000u){ uint32_t _vt=MEM(_p);
          if(_vt==0x1595f4||_vt==0x1595f5){ __android_log_print(6,"junqi","F_b6790 全局.bss找到glClear! slot=0x%x obj=0x%08x vt=0x%08x", 0x15be80+_i,_p,_vt); _f2=1; } } }
      if(!_f2) __android_log_print(6,"junqi","F_b6790 子对象+全局.bss 无vtable=0x1595f4");
    }
    g_cur_fn = 0xb6790;
Lb6790: sp-=(2*4); /* push r4, lr */
Lb6790_p0: MEM(sp+0)=r4;
Lb6792: r3=192;
Lb6794: {uint32_t a_=r0,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb6796: r0=MEM(r0+r3);
Lb6798: {int32_t A=(int32_t)r0,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb679a: if(Z) goto Lb67a2;
Lb679c: r3=MEM(r0);
Lb679e: r3=MEM(r3+16);
Lb67a0: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lb67a2: r2=1;
Lb67a4: r3=86;
Lb67a6: MEMB(r4+r3)=r2;
Lb67a8_l0: r4=MEM(sp+0);
Lb67a8: sp+=(1*4); /* pop {r4, pc} */
Lb67a8_r: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_b67c0(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb67c0;
Lb67c0: sp-=(2*4); /* push r4, lr */
Lb67c0_p0: MEM(sp+0)=r4;
Lb67c2: r3=192;
Lb67c4: r0=MEM(r0+r3);
Lb67c6: {int32_t A=(int32_t)r0,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb67c8: if(Z) goto Lb67d0;
Lb67ca: r3=MEM(r0);
Lb67cc: r3=MEM(r3+36);
Lb67ce: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lb67d0_l0: r4=MEM(sp+0);
Lb67d0: sp+=(1*4); /* pop {r4, pc} */
Lb67d0_r: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_b6890(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb6890;
Lb6890: sp-=(2*4); /* push r4, lr */
Lb6890_p0: MEM(sp+0)=r4;
Lb6892: {uint32_t a_=r1,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb6894: r1=MEM(r0+52);
Lb6896: sp-=8;
Lb6898: {uint32_t a_=r0,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb689a: {int32_t A=(int32_t)r1,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r1>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb689c: if(Z) goto Lb68cc;
Lb689e: r0=1;
Lb68a0: r1=86;
Lb68a2: MEMB(r4+r1)=r0;
Lb68a4: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb68a6: MEM(r4+52)=r3;
Lb68a8: MEM(r4+56)=r2;
Lb68aa: {uint32_t a_=r0,b_=60; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb68ac: MEM(sp)=r2;
Lb68ae: r1=0;
Lb68b0: r2=0;
Lb68b2: r0=F_afd18(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xafd18 完整 */
Lb68b6: r3=192;
Lb68b8: r0=MEM(r4+r3);
Lb68ba: {int32_t A=(int32_t)r0,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb68bc: if(Z) goto Lb68c8;
Lb68be: r3=MEM(r0);
Lb68c0: r1=MEM(r4+52);
Lb68c2: r2=MEM(r4+56);
Lb68c4: r3=MEM(r3+20);
Lb68c6: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lb68c8: sp+=8;
Lb68ca_l0: r4=MEM(sp+0);
Lb68ca: sp+=(1*4); /* pop {r4, pc} */
Lb68ca_r: goto RET;
Lb68cc: r1=MEM(r0+56);
Lb68ce: {int32_t A=(int32_t)r1,B=(int32_t)r2;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r1>=r2);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb68d0: if(!Z) goto Lb689e;
Lb68d2: goto Lb68c8;
RET: return r0;
    return 0;
}

uint32_t F_b69b4(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb69b4;
Lb69b4: sp-=(5*4); /* push r4, r5, r6, r7, lr */
Lb69b4_p0: MEM(sp+0)=r4;
Lb69b4_p1: MEM(sp+4)=r5;
Lb69b4_p2: MEM(sp+8)=r6;
Lb69b4_p3: MEM(sp+12)=r7;
Lb69b6: r7=r11;
Lb69b8: r6=r10;
Lb69ba: r5=r9;
Lb69bc: r4=r8;
Lb69be: sp-=(4*4); /* push r4, r5, r6, r7 */
Lb69be_p0: MEM(sp+0)=r4;
Lb69be_p1: MEM(sp+4)=r5;
Lb69be_p2: MEM(sp+8)=r6;
Lb69be_p3: MEM(sp+12)=r7;
Lb69c0: sp-=12;
Lb69c2: r3=sp+4;
Lb69c4: r11=r3;
Lb69c6: r3=MEM(747976+144);
Lb69c8: r6=MEM(747980+144);
Lb69ca: {uint32_t a_=r0,b_=0; r7=(a_+b_); Z=(r7==0); C=(r7<a_); N=((r7)>>31)&1; V=(((a_^r7)&(b_^r7))>>31)&1;}
Lb69cc: r8=r3;
Lb69ce: r3=MEM(747984+144);
Lb69d0: {uint32_t a_=r0,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb69d2: {uint32_t a_=r0,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lb69d4: r9=r3;
Lb69d6: r10=r0;
Lb69d8: {uint32_t a_=r7,b_=172; r7=(a_+b_); Z=(r7==0); C=(r7<a_); N=((r7)>>31)&1; V=(((a_^r7)&(b_^r7))>>31)&1;}
Lb69da: {uint32_t a_=r4,b_=156; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb69dc: {uint32_t a_=r5,b_=116; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lb69de: r6=(748002+r6);
Lb69e0: r8=(748004+r8);
Lb69e2: r9=(748006+r9);
Lb69e4: goto Lb6a3a;
Lb69e6: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb69e8: r0=F_b3c20(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb3c20 完整 */
Lb69ec: r2=MEM(sp+4);
Lb69ee: {uint32_t a_=r0,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lb69f0: {uint32_t a_=r7,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb69f2: r0=F_b7eb0(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb7eb0 完整 */
Lb69f6: {int32_t A=(int32_t)r0,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb69f8: if(Z) goto Lb6a48;
Lb69fa: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb69fc: r1=MEM(sp+4);
Lb69fe: r0=F_b3c28(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb3c28 完整 */
Lb6a02: r1=4;
Lb6a04: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb6a06: r0=F_b3c40(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb3c40 完整 */
Lb6a0a: {uint32_t a_=r6,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lb6a0c: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb6a0e: r0=F_b3ecc(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb3ecc 完整 */
Lb6a12: r1=r8;
Lb6a14: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb6a16: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb6a18: r0=F_b3694(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb3694 完整 */
Lb6a1c: {uint32_t a_=r6,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lb6a1e: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb6a20: r0=F_b3ecc(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb3ecc 完整 */
Lb6a24: r1=r9;
Lb6a26: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb6a28: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb6a2a: r0=F_b3694(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb3694 完整 */
Lb6a2e: r1=128;
Lb6a30: r0=r10;
Lb6a32: r1=((r1<<4))&0xffffffffu; Z=(r1==0); N=(((r1)>>31))&1;
Lb6a34: {uint32_t a_=r5,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb6a36: r0=F_b6808(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb6808 完整 */
Lb6a3a: {uint32_t a_=r7,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb6a3c: r1=r11;
Lb6a3e: r2=4;
Lb6a40: r0=F_b7f48(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb7f48 完整 */
Lb6a44: {int32_t A=(int32_t)r0,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6a46: if(!Z) goto Lb69e6;
Lb6a48: sp+=12;
Lb6a4a_l2: r4=MEM(sp+8);
Lb6a4a_l3: r5=MEM(sp+12);
Lb6a4a: sp+=(4*4); /* pop {r2, r3, r4, r5} */
Lb6a4c: r8=r2;
Lb6a4e: r9=r3;
Lb6a50: r10=r4;
Lb6a52: r11=r5;
Lb6a54_l0: r4=MEM(sp+0);
Lb6a54_l1: r5=MEM(sp+4);
Lb6a54_l2: r6=MEM(sp+8);
Lb6a54_l3: r7=MEM(sp+12);
Lb6a54: sp+=(4*4); /* pop {r4, r5, r6, r7, pc} */
Lb6a54_r: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_b6a64(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb6a64;
    { __android_log_print(6,"junqi","F_b6a64 ENTRY r0=0x%08x r1=0x%08x tid=%d", r0, r1, (int)gettid()); }
Lb6a64: sp-=(4*4); /* push r4, r5, r6, lr */
Lb6a64_p0: MEM(sp+0)=r4;
Lb6a64_p1: MEM(sp+4)=r5;
Lb6a64_p2: MEM(sp+8)=r6;
Lb6a66: r3=0x55;
Lb6a68: r3=MEMB(r0+r3);
Lb6a6a: {uint32_t a_=r0,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb6a6c: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6a6e: if(Z) goto Lb6aea;
Lb6a70: r5=0xc0;
Lb6a72: r3=MEM(r0+r5);
Lb6a74: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6a76: if(Z) goto Lb6aea;
Lb6a78: r0=F_b69b4(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 0x400) __android_log_print(6,"junqi","F_b6a64 Lb6a7c F_b69b4 返回 r0=0x%08x tid=%d", r0, (int)gettid()); } /* bl 0xb69b4 完整 */
Lb6a7c: r0=MEM(r4+r5);
Lb6a7e: {int32_t A=(int32_t)r0,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6a80: if(Z) goto Lb6a88;
Lb6a82: r3=MEM(r0);
Lb6a84: r3=MEM(r3+28);
Lb6a86: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
    { __android_log_print(6,"junqi","F_b6a64 Lb6a88 后 dispatch1 返回 r0=0x%08x r3=0x%08x tid=%d", r0, r3, (int)gettid()); }
Lb6a88: r1=MEM(r4+88);
Lb6a8a: r6=0;
Lb6a8c: {uint32_t a_=r1,b_=1; r1=(a_-b_); Z=(r1==0); C=(a_>=b_); N=((r1)>>31)&1; V=(((a_^b_)&(a_^r1))>>31)&1;}
Lb6a8e: {uint32_t a_=r1,b_=1; r3=(a_-b_); Z=(r3==0); C=(a_>=b_); N=((r3)>>31)&1; V=(((a_^b_)&(a_^r3))>>31)&1;}
Lb6a90: r1-=r3+(1-C);
Lb6a92: r2=0-r1;
Lb6a94: r0=30;
Lb6a96: r1=0;
Lb6a98: {uint32_t a_=r0,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lb6a9a: r0=0;
    { __android_log_print(6,"junqi","F_b6a64 Lb6a9a 前 F_b8f20 r4=0x%08x r5=0x%08x r6=0x%08x r2=0x%08x r1=0x%08x tid=%d", r4, r5, r6, r2, r1, (int)gettid()); }
Lb6a9c: r5&=r2; Z=(r5==0); N=((r5)>>31)&1;
Lb6a9e: r0=F_b8f20(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { __android_log_print(6,"junqi","F_b6a64 Lb6aa2 后 F_b8f20 返回 r0=0x%08x tid=%d", r0, (int)gettid()); } /* bl 0xb8f20 完整 */
Lb6aa2: r2=MEM(r4+96);
Lb6aa4: r3=MEM(r4+100);
Lb6aa6: {uint32_t a_=r5,b_=r2; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lb6aa8: r6+=r3+C;
Lb6aaa: {int32_t A=(int32_t)r6,B=(int32_t)r1;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=r1);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6aac: if((!Z&&(N==V))) goto Lb6aea;
Lb6aae: {int32_t A=(int32_t)r6,B=(int32_t)r1;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=r1);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6ab0: if(Z) goto Lb6ae6;
Lb6ab2: r3=87;
    { static int _g_loop=0; if (++_g_loop > 0x100) { _g_loop=0; __android_log_print(6,"junqi","F_b6a64 循环阀触发 Lb6ab2 r6=0x%08x r1=0x%08x 返回 0 tid=%d", r6, r1, (int)gettid()); return 0; } if (_g_loop < 8) __android_log_print(6,"junqi","F_b6a64 Lb6ab2 循环#%d r4=0x%08x r5=0x%08x r6=0x%08x r1=0x%08x tid=%d", _g_loop, r4, r5, r6, r1, (int)gettid()); }
Lb6ab4: r3=MEMB(r4+r3);
Lb6ab6: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6ab8: if(Z) goto Lb6ac2;
Lb6aba: r3=86;
Lb6abc: r3=MEMB(r4+r3);
Lb6abe: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6ac0: if(Z) goto Lb6aea;
Lb6ac2: r3=86;
Lb6ac4: r2=0;
Lb6ac6: MEM(r4+96)=r0;
Lb6ac8: MEM(r4+100)=r1;
Lb6aca: MEMB(r4+r3)=r2;
Lb6acc: r3=85;
Lb6ace: r3=MEMB(r4+r3);
Lb6ad0: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6ad2: if(Z) goto Lb6aee;
Lb6ad4: r3=192;
Lb6ad6: r0=MEM(r4+r3);
Lb6ad8: {int32_t A=(int32_t)r0,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6ada: if(Z) goto Lb6aee;
Lb6adc: r3=MEM(r0);
Lb6ade: r3=MEM(r3+32);
Lb6ae0: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_b6a64 Lb6ae2 后 dispatch2 返回 r0=0x%08x r3=0x%08x tid=%d", r0, r3, (int)gettid()); }
Lb6ae2: r0=1;
Lb6ae4: goto Lb6aec;
Lb6ae6: {int32_t A=(int32_t)r5,B=(int32_t)r0;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb6ae8: if((Z||!C)) goto Lb6ab2;
Lb6aea: r0=0;
Lb6aec_l0: r4=MEM(sp+0);
Lb6aec_l1: r5=MEM(sp+4);
Lb6aec_l2: r6=MEM(sp+8);
Lb6aec: sp+=(3*4); /* pop {r4, r5, r6, pc} */
Lb6aec_r: goto RET;
Lb6aee: r0=1;
Lb6af0: goto Lb6aec;
RET: { static int _g=0; if (++_g < 0x400) __android_log_print(6,"junqi","F_b6a64 RET r0=0x%08x tid=%d", r0, (int)gettid()); } return r0;
    return 0;
}

uint32_t F_b7eb0(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb7eb0;
Lb7eb0: sp-=(6*4); /* push r3, r4, r5, r6, r7, lr */
Lb7eb0_p1: MEM(sp+4)=r4;
Lb7eb0_p2: MEM(sp+8)=r5;
Lb7eb0_p3: MEM(sp+12)=r6;
Lb7eb0_p4: MEM(sp+16)=r7;
Lb7eb2: {uint32_t a_=r1,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb7eb4: r3=MEM(r0+12);
Lb7eb6: r1=MEM(r0+8);
Lb7eb8: {uint32_t a_=r2,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lb7eba: {uint32_t a_=r0,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lb7ebc: {uint32_t a_=r3,b_=r1; r2=(a_-b_); Z=(r2==0); C=(a_>=b_); N=((r2)>>31)&1; V=(((a_^b_)&(a_^r2))>>31)&1;}
Lb7ebe: {int32_t A=(int32_t)r3,B=(int32_t)r1;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=r1);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb7ec0: if((C)) goto Lb7ecc;
Lb7ec2: r2=MEM(r0+4);
Lb7ec4: r0=MEM(r0);
Lb7ec6: {uint32_t a_=r2,b_=r1; r2=(a_-b_); Z=(r2==0); C=(a_>=b_); N=((r2)>>31)&1; V=(((a_^b_)&(a_^r2))>>31)&1;}
Lb7ec8: {uint32_t a_=r3,b_=r2; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb7eca: {uint32_t a_=r2,b_=r0; r2=(a_-b_); Z=(r2==0); C=(a_>=b_); N=((r2)>>31)&1; V=(((a_^b_)&(a_^r2))>>31)&1;}
Lb7ecc: r0=0;
Lb7ece: {int32_t A=(int32_t)r5,B=(int32_t)r2;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r2);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb7ed0: if((!Z&&(N==V))) goto Lb7ef2;
Lb7ed2: {int32_t A=(int32_t)r6,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb7ed4: if(Z) goto Lb7ef4;
Lb7ed6: r7=0;
Lb7ed8: {int32_t A=(int32_t)r3,B=(int32_t)r1;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=r1);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb7eda: if((!C)) goto Lb7ef8;
Lb7edc: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb7ede: if((Z||(N!=V))) goto Lb7ef4;
Lb7ee0: {uint32_t a_=r5,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb7ee2: {uint32_t a_=r6,b_=r7; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb7ee4: r1=MEM(r4+8);
Lb7ee6: { memcpy((void*)(uintptr_t)JQT_ABS_PTR(r0),(const void*)(uintptr_t)JQT_ABS_PTR(r1),(size_t)r2); } /* blx 0x22adc memcpy 真实 */
Lb7eea: r3=MEM(r4+8);
Lb7eec: r0=1;
Lb7eee: {uint32_t a_=r3,b_=r5; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lb7ef0: MEM(r4+8)=r5;
Lb7ef2_l1: r4=MEM(sp+4);
Lb7ef2_l2: r5=MEM(sp+8);
Lb7ef2_l3: r6=MEM(sp+12);
Lb7ef2_l4: r7=MEM(sp+16);
Lb7ef2: sp+=(5*4); /* pop {r3, r4, r5, r6, r7, pc} */
Lb7ef2_r: goto RET;
Lb7ef4: r0=1;
Lb7ef6: goto Lb7ef2;
Lb7ef8: r3=MEM(r4+4);
Lb7efa: {uint32_t a_=r5,b_=0; r7=(a_+b_); Z=(r7==0); C=(r7<a_); N=((r7)>>31)&1; V=(((a_^r7)&(b_^r7))>>31)&1;}
Lb7efc: {uint32_t a_=r3,b_=r1; r3=(a_-b_); Z=(r3==0); C=(a_>=b_); N=((r3)>>31)&1; V=(((a_^b_)&(a_^r3))>>31)&1;}
Lb7efe: {int32_t A=(int32_t)r5,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb7f00: if((Z||(N!=V))) goto Lb7f04;
Lb7f02: {uint32_t a_=r3,b_=0; r7=(a_+b_); Z=(r7==0); C=(r7<a_); N=((r7)>>31)&1; V=(((a_^r7)&(b_^r7))>>31)&1;}
Lb7f04: {uint32_t a_=r7,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb7f06: {uint32_t a_=r6,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb7f08: { memcpy((void*)(uintptr_t)JQT_ABS_PTR(r0),(const void*)(uintptr_t)JQT_ABS_PTR(r1),(size_t)r2); } /* blx 0x22adc memcpy 真实 */
Lb7f0c: r3=MEM(r4+8);
Lb7f0e: r2=MEM(r4+4);
Lb7f10: {uint32_t a_=r5,b_=r7; r5=(a_-b_); Z=(r5==0); C=(a_>=b_); N=((r5)>>31)&1; V=(((a_^b_)&(a_^r5))>>31)&1;}
Lb7f12: {uint32_t a_=r3,b_=r7; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lb7f14: MEM(r4+8)=r3;
Lb7f16: {int32_t A=(int32_t)r3,B=(int32_t)r2;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=r2);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb7f18: if((!C)) goto Lb7edc;
Lb7f1a: r3=MEM(r4);
Lb7f1c: MEM(r4+8)=r3;
Lb7f1e: goto Lb7edc;
RET: return r0;
    return 0;
}

uint32_t F_b7f48(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb7f48;
Lb7f48: sp-=(5*4); /* push r4, r5, r6, r7, lr */
Lb7f48_p0: MEM(sp+0)=r4;
Lb7f48_p1: MEM(sp+4)=r5;
Lb7f48_p2: MEM(sp+8)=r6;
Lb7f48_p3: MEM(sp+12)=r7;
Lb7f4a: r7=r8;
Lb7f4c: sp-=(1*4); /* push r7 */
Lb7f4c_p0: MEM(sp+0)=r7;
Lb7f4e: r3=MEM(r0+12);
Lb7f50: r4=MEM(r0+8);
Lb7f52: {uint32_t a_=r2,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lb7f54: {uint32_t a_=r0,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lb7f56: {uint32_t a_=r1,b_=0; r7=(a_+b_); Z=(r7==0); C=(r7<a_); N=((r7)>>31)&1; V=(((a_^r7)&(b_^r7))>>31)&1;}
Lb7f58: {uint32_t a_=r3,b_=r4; r2=(a_-b_); Z=(r2==0); C=(a_>=b_); N=((r2)>>31)&1; V=(((a_^b_)&(a_^r2))>>31)&1;}
Lb7f5a: {int32_t A=(int32_t)r3,B=(int32_t)r4;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=r4);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb7f5c: if((C)) goto Lb7f68;
Lb7f5e: r2=MEM(r0+4);
Lb7f60: r1=MEM(r0);
Lb7f62: {uint32_t a_=r2,b_=r4; r2=(a_-b_); Z=(r2==0); C=(a_>=b_); N=((r2)>>31)&1; V=(((a_^b_)&(a_^r2))>>31)&1;}
Lb7f64: {uint32_t a_=r3,b_=r2; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb7f66: {uint32_t a_=r2,b_=r1; r2=(a_-b_); Z=(r2==0); C=(a_>=b_); N=((r2)>>31)&1; V=(((a_^b_)&(a_^r2))>>31)&1;}
Lb7f68: r0=0;
Lb7f6a: {int32_t A=(int32_t)r6,B=(int32_t)r2;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=r2);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb7f6c: if((!Z&&(N==V))) goto Lb7f8a;
Lb7f6e: r2=0;
Lb7f70: r8=r2;
Lb7f72: {int32_t A=(int32_t)r3,B=(int32_t)r4;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=r4);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb7f74: if((!C)) goto Lb7f90;
Lb7f76: r0=1;
Lb7f78: {int32_t A=(int32_t)r6,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb7f7a: if((Z||(N!=V))) goto Lb7f8a;
Lb7f7c: r2=r8;
Lb7f7e: {uint32_t a_=r7,b_=r2; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb7f80: {uint32_t a_=r4,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lb7f82: {uint32_t a_=r6,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lb7f84: { memcpy((void*)(uintptr_t)JQT_ABS_PTR(r0),(const void*)(uintptr_t)JQT_ABS_PTR(r1),(size_t)r2); } /* blx 0x22adc memcpy 真实 */
Lb7f88: r0=1;
Lb7f8a: sp+=(1*4); /* pop {r2} */
Lb7f8c: r8=r2;
Lb7f8e_l0: r4=MEM(sp+0);
Lb7f8e_l1: r5=MEM(sp+4);
Lb7f8e_l2: r6=MEM(sp+8);
Lb7f8e_l3: r7=MEM(sp+12);
Lb7f8e: sp+=(4*4); /* pop {r4, r5, r6, r7, pc} */
Lb7f8e_r: goto RET;
Lb7f90: r3=MEM(r5+4);
Lb7f92: r8=r6;
Lb7f94: {uint32_t a_=r3,b_=r4; r3=(a_-b_); Z=(r3==0); C=(a_>=b_); N=((r3)>>31)&1; V=(((a_^b_)&(a_^r3))>>31)&1;}
Lb7f96: {int32_t A=(int32_t)r6,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb7f98: if((Z||(N!=V))) goto Lb7f9c;
Lb7f9a: r8=r3;
Lb7f9c: {uint32_t a_=r4,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lb7f9e: {uint32_t a_=r7,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lb7fa0: r2=r8;
Lb7fa2: { memcpy((void*)(uintptr_t)JQT_ABS_PTR(r0),(const void*)(uintptr_t)JQT_ABS_PTR(r1),(size_t)r2); } /* blx 0x22adc memcpy 真实 */
Lb7fa6: r3=r8;
Lb7fa8: {uint32_t a_=r6,b_=r3; r6=(a_-b_); Z=(r6==0); C=(a_>=b_); N=((r6)>>31)&1; V=(((a_^b_)&(a_^r6))>>31)&1;}
Lb7faa: r3=MEM(r5+4);
Lb7fac: r4+=r8;
Lb7fae: {int32_t A=(int32_t)r4,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r4>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb7fb0: if((!C)) goto Lb7f76;
Lb7fb2: r4=MEM(r5);
Lb7fb4: goto Lb7f76;
RET: return r0;
    return 0;
}

uint32_t F_b8f20(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xb8f20;
    { static int _sd=0; if(_sd++ < 0x400) __android_log_print(6,"junqi","F_b8f20 ENTRY r0=0x%08x r1=0x%08x tid=%d", r0, r1, (int)gettid()); }
Lb8f20: sp-=(2*4); /* push r4, lr */
Lb8f20_p0: MEM(sp+0)=r4;
Lb8f22: {int32_t A=(int32_t)r0,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r0>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lb8f24: if(Z) goto Lb8f30;
Lb8f26: r3=MEM(757544+20);
Lb8f28: r3=(757548+r3);
Lb8f2a: r0=MEM(r3);
Lb8f2c: r0=F_b6218(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb6218 完整 */
Lb8f30: r3=MEM(757556+12);
Lb8f32: r3=(757558+r3);
Lb8f34: r0=MEM(r3);
Lb8f36: r0=F_b5f58(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb5f58 完整 */
Lb8f3a_l0: r4=MEM(sp+0);
Lb8f3a: sp+=(1*4); /* pop {r4, pc} */
Lb8f3a_r: goto RET;
RET: { static int _g=0; if (_g++ < 0x400) __android_log_print(6,"junqi","F_b8f20 RET r0=0x%08x tid=%d", r0, (int)gettid()); } return r0;
    return 0;
}

uint32_t F_bd548(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xbd548;
Lbd548: r0=MEM(r0+4);
Lbd54a: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_bd5a0(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xbd5a0;
Lbd5a0: sp-=(5*4); /* push r4, r5, r6, r7, lr */
Lbd5a0_p0: MEM(sp+0)=r4;
Lbd5a0_p1: MEM(sp+4)=r5;
Lbd5a0_p2: MEM(sp+8)=r6;
Lbd5a0_p3: MEM(sp+12)=r7;
Lbd5a2: r7=r11;
Lbd5a4: r6=r10;
Lbd5a6: r5=r9;
Lbd5a8: r4=r8;
Lbd5aa: sp-=(4*4); /* push r4, r5, r6, r7 */
Lbd5aa_p0: MEM(sp+0)=r4;
Lbd5aa_p1: MEM(sp+4)=r5;
Lbd5aa_p2: MEM(sp+8)=r6;
Lbd5aa_p3: MEM(sp+12)=r7;
Lbd5ac: sp-=36;
Lbd5ae: r7=sp+24;
Lbd5b0: {uint32_t a_=r0,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lbd5b2: {uint32_t a_=r7,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbd5b4: r0=F_afb68(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd5b8 F_afb68 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xafb68 完整 */
Lbd5b8: r3=12;
Lbd5ba: {uint32_t a_=r3,b_=r6; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lbd5bc: r8=r3;
Lbd5be: r3=MEM(775616+328);
Lbd5c0: r4=sp+12;
Lbd5c2: r9=r3;
Lbd5c4: r3=MEM(775624+324);
Lbd5c6: r9=(775626+r9);
Lbd5c8: r10=r3;
Lbd5ca: r3=MEM(775628+324);
Lbd5cc: r10=(775632+r10);
Lbd5ce: r3=(775634+r3);
Lbd5d0: MEM(sp+4)=r3;
Lbd5d2: r3=MEM(775636+320);
Lbd5d4: r11=r3;
Lbd5d6: r11=(775642+r11);
Lbd5d8: r0=MEM(r6+36);
Lbd5da: {uint32_t a_=r4,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbd5dc: r2=12;
Lbd5de: r0=F_b7eb0(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd5e2 F_b7eb0 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb7eb0 完整 */
Lbd5e2: {uint32_t a_=r0,b_=0; r5=(a_-b_); Z=(r5==0); C=(a_>=b_); N=((r5)>>31)&1; V=(((a_^b_)&(a_^r5))>>31)&1;}
Lbd5e4: if(Z) goto Lbd618;
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd5e6 路径 r6=0x%08x r3=0x%08x r5=0x%08x tid=%d", r6, r3, r5, (int)gettid()); }
Lbd5e6: r3=MEM(r4);
    { static int _g_bd5e6=0; if (++_g_bd5e6 > 0x100) { _g_bd5e6=0; __android_log_print(6,"junqi","F_bd5a0 循环阀触发 Lbd5e6 r5=0x%08x，强制返回 tid=%d", r5, (int)gettid()); return 0; } } /* VALVE */
Lbd5e8: {int32_t A=(int32_t)r3,B=(int32_t)(34);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=34);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbd5ea: if((!Z&&(N==V))) goto Lbd664;
Lbd5ec: {int32_t A=(int32_t)r3,B=(int32_t)(32);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=32);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbd5ee: if((N!=V)) goto Lbd68e;
Lbd5f0: r1=MEM(775668+292);
Lbd5f2: r2=MEM(r4+4);
Lbd5f4: r0=r8;
Lbd5f6: r1=(775674+r1);
Lbd5f8: r0=F_b38c0(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd5fc F_b38c0 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb38c0 完整 */
Lbd5fc: r3=MEM(775680+284);
Lbd5fe: r1=MEM(r4);
Lbd600: r2=r8;
Lbd602: r3=(775686+r3);
Lbd604: r0=MEM(r3);
Lbd606: r0=F_b6808(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd60a F_b6808 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb6808 完整 */
Lbd60a: r0=MEM(r6+36);
Lbd60c: {uint32_t a_=r4,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbd60e: r2=12;
Lbd610: r0=F_b7eb0(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd614 F_b7eb0#2 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb7eb0 完整 */
Lbd614: {uint32_t a_=r0,b_=0; r5=(a_-b_); Z=(r5==0); C=(a_>=b_); N=((r5)>>31)&1; V=(((a_^b_)&(a_^r5))>>31)&1;}
Lbd616: if(!Z) goto Lbd5e6;
Lbd618: r4=40;
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd618 路径 r6=0x%08x MEMB(r6+40)=0x%02x tid=%d", r6, (r6?MEMB(r6+40):0xff), (int)gettid()); }
Lbd61a: r3=MEMB(r6+r4);
Lbd61c: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbd61e: if(!Z) goto Lbd6e2;
Lbd620: r4=49;
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd620 路径 r6=0x%08x MEMB(r6+49)=0x%02x tid=%d", r6, (r6?MEMB(r6+49):0xff), (int)gettid()); }
Lbd622: r3=MEMB(r6+r4);
Lbd624: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbd626: if(Z) goto Lbd64c;
Lbd628: r1=MEM(775724+244);
Lbd62a: r2=MEM(r6+56);
Lbd62c: r0=r8;
Lbd62e: r1=(775730+r1);
Lbd630: r0=F_b38c0(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd634 F_b38c0#2 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb38c0 完整 */
Lbd634: r3=MEM(775736+236);
Lbd636: r1=MEM(775736+240);
Lbd638: r2=r8;
Lbd63a: r3=(775742+r3);
Lbd63c: r0=MEM(r3);
Lbd63e: r0=F_b6808(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd642 F_b6808#2 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb6808 完整 */
Lbd642: r0=MEM(r6+52);
Lbd644: r0=F_becf4(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd648 F_becf4 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xbecf4 完整 */
Lbd648: r3=0;
Lbd64a: MEMB(r6+r4)=r3;
Lbd64c: r3=MEM(775760+220);
Lbd64e: r3=(775762+r3);
Lbd650: r0=MEM(r3);
Lbd652: r0=F_b6a64(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd656 F_b6a64 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb6a64 完整 */
Lbd656: sp+=36;
Lbd658_l2: r4=MEM(sp+8);
Lbd658_l3: r5=MEM(sp+12);
Lbd658: sp+=(4*4); /* pop {r2, r3, r4, r5} */
Lbd65a: r8=r2;
Lbd65c: r9=r3;
Lbd65e: r10=r4;
Lbd660: r11=r5;
Lbd662_l0: r4=MEM(sp+0);
Lbd662_l1: r5=MEM(sp+4);
Lbd662_l2: r6=MEM(sp+8);
Lbd662_l3: r7=MEM(sp+12);
Lbd662: sp+=(4*4); /* pop {r4, r5, r6, r7, pc} */
Lbd662_r: goto RET;
Lbd664: {int32_t A=(int32_t)r3,B=(int32_t)(67);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=67);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbd666: if(Z) goto Lbd6d4;
Lbd668: r2=MEM(775788+196);
Lbd66a: {int32_t A=(int32_t)r3,B=(int32_t)r2;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=r2);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbd66c: if(!Z) goto Lbd5d8;
Lbd66e: r3=r10;
Lbd670: r0=MEM(r3);
Lbd672: r0=F_c5d50(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd676 F_c5d50 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xc5d50 完整 */
Lbd676: r1=MEM(sp+4);
Lbd678: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbd67a: r0=r8;
Lbd67c: r0=F_b3694(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd680 F_b3694 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb3694 完整 */
Lbd680: r3=r11;
Lbd682: r0=MEM(r3);
Lbd684: r1=MEM(r4);
Lbd686: r2=r8;
Lbd688: r0=F_b6808(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd68c F_b6808#3 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb6808 完整 */
Lbd68c: goto Lbd5d8;
Lbd68e: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbd690: if((Z||(N!=V))) goto Lbd5d8;
Lbd692: {int32_t A=(int32_t)r3,B=(int32_t)(2);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=2);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbd694: if((Z||(N!=V))) goto Lbd69a;
Lbd696: {int32_t A=(int32_t)r3,B=(int32_t)(7);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=7);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbd698: if(!Z) goto Lbd5d8;
Lbd69a: r5=MEM(775836+152);
Lbd69c: r2=MEM(r4+8);
Lbd69e: r1=MEM(r4+4);
Lbd6a0: {uint32_t a_=r7,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbd6a2: r5=(775846+r5);
Lbd6a4: r0=F_afbc0(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd6a8 F_afbc0 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xafbc0 完整 */
Lbd6a8: r0=MEM(r5);
Lbd6aa: {uint32_t a_=r7,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbd6ac: r0=F_b66b0(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd6b0 F_b66b0 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb66b0 完整 */
Lbd6b0: r1=MEM(775860+132);
Lbd6b2: r2=MEM(r7);
Lbd6b4: r0=r8;
Lbd6b6: r1=(775866+r1);
Lbd6b8: r0=F_b38c0(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd6bc F_b38c0#3 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb38c0 完整 */
Lbd6bc: r1=MEM(775872+124);
Lbd6be: r2=MEM(r7+4);
Lbd6c0: r0=r8;
Lbd6c2: r1=(775878+r1);
Lbd6c4: r0=F_b38c0(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd6c8 F_b38c0#4 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb38c0 完整 */
Lbd6c8: r0=MEM(r5);
Lbd6ca: r1=MEM(r4);
Lbd6cc: r2=r8;
Lbd6ce: r0=F_b6808(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd6d2 F_b6808#4 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb6808 完整 */
Lbd6d2: goto Lbd5d8;
Lbd6d4: r3=r9;
Lbd6d6: r0=MEM(r3);
Lbd6d8: r1=67;
Lbd6da: r2=0;
Lbd6dc: r0=F_b6808(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd6e0 F_b6808#5 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb6808 完整 */
Lbd6e0: goto Lbd5d8;
Lbd6e2: r1=MEM(775908+92);
Lbd6e4: r3=48;
Lbd6e6: r2=MEMB(r6+r3);
Lbd6e8: r1=(775916+r1);
Lbd6ea: r0=r8;
Lbd6ec: r0=F_b391c(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd6f0 F_b391c 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb391c 完整 */
Lbd6f0: r3=MEM(775924+80);
Lbd6f2: r1=MEM(775924+84);
Lbd6f4: r2=r8;
Lbd6f6: r3=(775930+r3);
Lbd6f8: r0=MEM(r3);
Lbd6fa: r0=F_b6808(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd6fe F_b6808#6 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xb6808 完整 */
Lbd6fe: r0=MEM(r6+44);
Lbd700: r0=F_becf4(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp);
    { static int _g=0; if (++_g < 6) __android_log_print(6,"junqi","F_bd5a0 Lbd704 F_becf4#2 返回 r0=0x%08x tid=%d", r0, (int)gettid()); }
 /* bl 0xbecf4 完整 */
Lbd704: MEMB(r6+r4)=r5;
Lbd706: goto Lbd620;
RET: return r0;
    return 0;
}

uint32_t F_bd74c(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xbd74c;
Lbd74c: sp-=(2*4); /* push r4, lr */
Lbd74c_p0: MEM(sp+0)=r4;
Lbd74e: r1=MEM(776016+16);
Lbd750: r1=(776020+r1);
Lbd752: r0=MEM(r1);
Lbd754: {uint32_t a_=r2,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbd756: {uint32_t a_=r3,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbd758: r0=F_b6890(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb6890 完整 */
Lbd75c_l0: r4=MEM(sp+0);
Lbd75c: sp+=(1*4); /* pop {r4, pc} */
Lbd75c_r: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_bd764(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xbd764;
Lbd764: sp-=(2*4); /* push r4, lr */
Lbd764_p0: MEM(sp+0)=r4;
Lbd766: r3=MEM(776040+12);
Lbd768: r3=(776044+r3);
Lbd76a: r0=MEM(r3);
Lbd76c: r0=F_b6790(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb6790 完整 */
Lbd770_l0: r4=MEM(sp+0);
Lbd770: sp+=(1*4); /* pop {r4, pc} */
Lbd770_r: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_be930(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xbe930;
Lbe930: sp-=(6*4); /* push r3, r4, r5, r6, r7, lr */
Lbe930_p1: MEM(sp+4)=r4;
Lbe930_p2: MEM(sp+8)=r5;
Lbe930_p3: MEM(sp+12)=r6;
Lbe930_p4: MEM(sp+16)=r7;
Lbe932: r3=MEM(780596+128);
Lbe934: {uint32_t a_=r0,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lbe936: {uint32_t a_=r1,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lbe938: r3=(780604+r3);
Lbe93a: r0=MEM(r3);
Lbe93c: r0=F_b67c0(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xb67c0 完整 */
Lbe940: r3=MEM(780612+116);
Lbe942: r3=(780614+r3);
Lbe944: r0=MEM(r3);
Lbe946: r0=F_c5e38(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xc5e38 完整 */
Lbe94a: r3=MEM(780620+112);
Lbe94c: r3=(780624+r3);
Lbe94e: r0=MEM(r3);
Lbe950: r0=F_bef78(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbef78 完整 */
Lbe954: r3=MEM(780632+104);
Lbe956: r3=(780634+r3);
Lbe958: r0=MEM(r3);
Lbe95a: r0=F_bf4d0(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbf4d0 完整 */
Lbe95e: r3=60;
Lbe960: r3=MEMB(r4+r3);
Lbe962: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbe964: if(Z) goto Lbe96e;
Lbe966: r3=61;
Lbe968: r3=MEMB(r4+r3);
Lbe96a: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbe96c: if(Z) goto Lbe970;
Lbe96e_l1: r4=MEM(sp+4);
Lbe96e_l2: r5=MEM(sp+8);
Lbe96e_l3: r6=MEM(sp+12);
Lbe96e_l4: r7=MEM(sp+16);
Lbe96e: sp+=(5*4); /* pop {r3, r4, r5, r6, r7, pc} */
Lbe96e_r: goto RET;
Lbe970: r3=MEM(r5);
Lbe972: r1=MEM(r4+4);
Lbe974: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbe976: r3=MEM(r3+124);
Lbe978: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbe97a: r2=MEM(780668+72);
Lbe97c: {uint32_t a_=r0,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lbe97e: r3=MEM(780672+72);
Lbe980: r0=MEM(r5);
Lbe982: r1=132;
Lbe984: r2=(780680+r2);
Lbe986: r7=MEM(r0+r1);
Lbe988: r3=(780684+r3);
Lbe98a: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbe98c: {uint32_t a_=r6,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbe98e: { r0=jqt_blx_dispatch(r7, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r7 vtable dispatch */
Lbe990: {uint32_t a_=r0,b_=0; r7=(a_+b_); Z=(r7==0); C=(r7<a_); N=((r7)>>31)&1; V=(((a_^r7)&(b_^r7))>>31)&1;}
Lbe992: {int32_t A=(int32_t)r6,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbe994: if(Z) goto Lbe9a0;
Lbe996: r3=MEM(r5);
Lbe998: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbe99a: {uint32_t a_=r6,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbe99c: r3=MEM(r3+92);
Lbe99e: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbe9a0: {uint32_t a_=r7,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbe9a2: r1=MEM(r4+4);
Lbe9a4: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbe9a6: r0=F_77800(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x77800 完整 */
Lbe9aa: r2=1;
Lbe9ac: r3=61;
Lbe9ae: MEMB(r4+r3)=r2;
Lbe9b0: goto Lbe96e;
RET: return r0;
    return 0;
}

uint32_t F_becf4(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xbecf4;
Lbecf4: sp-=(2*4); /* push r4, lr */
Lbecf4_p0: MEM(sp+0)=r4;
Lbecf6: r3=MEMB(r0);
Lbecf8: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbecfa: if(Z) goto Lbed02;
Lbecfc: {uint32_t a_=r0,b_=4; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbecfe: { sem_post((sem_t*)(uintptr_t)JQT_ABS_PTR(r0)); } /* blx 0x231f0 sem_post 真实 */
Lbed02_l0: r4=MEM(sp+0);
Lbed02: sp+=(1*4); /* pop {r4, pc} */
Lbed02_r: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_bef78(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xbef78;
Lbef78: sp-=(5*4); /* push r4, r5, r6, r7, lr */
Lbef78_p0: MEM(sp+0)=r4;
Lbef78_p1: MEM(sp+4)=r5;
Lbef78_p2: MEM(sp+8)=r6;
Lbef78_p3: MEM(sp+12)=r7;
Lbef7a: r7=r11;
Lbef7c: r6=r10;
Lbef7e: r5=r9;
Lbef80: r4=r8;
Lbef82: sp-=(4*4); /* push r4, r5, r6, r7 */
Lbef82_p0: MEM(sp+0)=r4;
Lbef82_p1: MEM(sp+4)=r5;
Lbef82_p2: MEM(sp+8)=r6;
Lbef82_p3: MEM(sp+12)=r7;
Lbef84: r5=MEM(782216+288);
Lbef86: r2=MEM(782216+292);
Lbef88: sp-=284;
Lbef8a: r5=(782222+r5);
Lbef8c: r3=MEM(r5+r2);
Lbef8e: {uint32_t a_=r0,b_=0; r7=(a_+b_); Z=(r7==0); C=(r7<a_); N=((r7)>>31)&1; V=(((a_^r7)&(b_^r7))>>31)&1;}
Lbef90: r8=r2;
Lbef92: r3=MEM(r3);
Lbef94: MEM(sp+276)=r3;
Lbef96: r3=MEMB(r0+24);
Lbef98: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbef9a: if(!Z) goto Lbefb8;
Lbef9c: r2=r8;
Lbef9e: r3=MEM(r5+r2);
Lbefa0: r2=MEM(sp+276);
Lbefa2: r3=MEM(r3);
Lbefa4: {int32_t A=(int32_t)r2,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r2>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbefa6: if(Z) goto Lbefaa;
Lbefa8: goto Lbf0a4;
Lbefaa: sp+=284;
Lbefac_l2: r4=MEM(sp+8);
Lbefac_l3: r5=MEM(sp+12);
Lbefac: sp+=(4*4); /* pop {r2, r3, r4, r5} */
Lbefae: r8=r2;
Lbefb0: r9=r3;
Lbefb2: r10=r4;
Lbefb4: r11=r5;
Lbefb6_l0: r4=MEM(sp+0);
Lbefb6_l1: r5=MEM(sp+4);
Lbefb6_l2: r6=MEM(sp+8);
Lbefb6_l3: r7=MEM(sp+12);
Lbefb6: sp+=(4*4); /* pop {r4, r5, r6, r7, pc} */
Lbefb6_r: goto RET;
Lbefb8: r6=MEM(782268+244);
Lbefba: r3=sp+20;
Lbefbc: r2=128;
Lbefbe: {uint32_t a_=r3,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbefc0: r2=((r2<<1))&0xffffffffu; Z=(r2==0); N=(((r2)>>31))&1;
Lbefc2: r6=(782278+r6);
Lbefc4: r0=MEM(r0+20);
Lbefc6: r9=r3;
Lbefc8: r0=F_c2da8(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xc2da8 完整 */
Lbefcc: r0=MEM(r6);
Lbefce: r0=F_bd530(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd530 完整 */
Lbefd2: r2=MEM(r0);
Lbefd4: r3=167;
Lbefd6: r3=((r3<<2))&0xffffffffu; Z=(r3==0); N=(((r3)>>31))&1;
Lbefd8: r3=MEM(r2+r3);
Lbefda: r1=r9;
Lbefdc: {uint32_t a_=r0,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lbefde: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbefe0: r3=MEM(r4);
Lbefe2: r1=MEM(782308+208);
Lbefe4: r11=r0;
Lbefe6: r3=MEM(r3+24);
Lbefe8: r1=(782316+r1);
Lbefea: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbefec: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbefee: r9=r0;
Lbeff0: r0=MEM(r4);
Lbeff2: r1=144;
Lbeff4: r1=((r1<<2))&0xffffffffu; Z=(r1==0); N=(((r1)>>31))&1;
Lbeff6: r1=MEM(r0+r1);
Lbeff8: r2=MEM(782332+188);
Lbeffa: r3=MEM(782332+192);
Lbeffc: r12=r1;
Lbeffe: r2=(782338+r2);
Lbf000: r3=(782340+r3);
Lbf002: r1=r9;
Lbf004: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf006: { r0=jqt_blx_dispatch(r12, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r12 vtable dispatch */
Lbf008: r1=MEM(r4);
Lbf00a: r3=150;
Lbf00c: r3=((r3<<2))&0xffffffffu; Z=(r3==0); N=(((r3)>>31))&1;
Lbf00e: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbf010: r3=MEM(r1+r3);
Lbf012: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf014: r1=r9;
Lbf016: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbf018: MEM(sp+12)=r0;
Lbf01a: r0=MEM(r4);
Lbf01c: r1=226;
Lbf01e: r1=((r1<<1))&0xffffffffu; Z=(r1==0); N=(((r1)>>31))&1;
Lbf020: r1=MEM(r0+r1);
Lbf022: r2=MEM(782372+156);
Lbf024: r3=MEM(782376+156);
Lbf026: r12=r1;
Lbf028: r2=(782380+r2);
Lbf02a: r3=(782382+r3);
Lbf02c: r1=r9;
Lbf02e: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf030: { r0=jqt_blx_dispatch(r12, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r12 vtable dispatch */
Lbf032: r10=r0;
Lbf034: r0=MEM(r6);
Lbf036: r0=F_bd548(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd548 完整 */
Lbf03a: r2=r11;
Lbf03c: MEM(sp)=r2;
Lbf03e: r2=MEM(sp+12);
Lbf040: {uint32_t a_=r0,b_=0; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lbf042: r1=r9;
Lbf044: MEM(sp+4)=r2;
Lbf046: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf048: r2=r10;
Lbf04a: r0=F_7bf60(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x7bf60 完整 */
Lbf04e: r3=r11;
Lbf050: r10=r0;
Lbf052: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbf054: if(Z) goto Lbf060;
Lbf056: r3=MEM(r4);
Lbf058: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf05a: r1=r11;
LLbf05c: r3=MEM(r3+92);
/* §124zz-7：同 Lbf5c0——槽值是绝对地址，JQT_ABS_PTR 透传。 */
{ uint32_t _mt = r3;
  r3 = JQT_ABS_PTR(_mt); }
Lbf05e: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbf060: r0=MEM(r4);
Lbf062: r2=MEM(782436+100);
Lbf064: r3=MEM(782440+100);
Lbf066: r1=132;
Lbf068: r6=MEM(r0+r1);
Lbf06a: r2=(782446+r2);
Lbf06c: r3=(782448+r3);
Lbf06e: r1=r9;
Lbf070: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf072: { r0=jqt_blx_dispatch(r6, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r6 vtable dispatch */
Lbf074: r1=r10;
Lbf076: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbf078: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf07a: r0=F_77800(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x77800 完整 */
Lbf07e: r2=r9;
Lbf080: {int32_t A=(int32_t)r2,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r2>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbf082: if(Z) goto Lbf08e;
Lbf084: r3=MEM(r4);
Lbf086: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf088: r1=r9;
LLbf08a: r3=MEM(r3+92);
/* §124zz-7：同 Lbf5c0——槽值是绝对地址，JQT_ABS_PTR 透传。 */
{ uint32_t _mt = r3;
  r3 = JQT_ABS_PTR(_mt); }
Lbf08c: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbf08e: r3=r10;
Lbf090: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbf092: if(Z) goto Lbf09e;
Lbf094: r3=MEM(r4);
Lbf096: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf098: r1=r10;
LLbf09a: r3=MEM(r3+92);
/* §124zz-7：同 Lbf5c0——槽值是绝对地址，JQT_ABS_PTR 透传。 */
{ uint32_t _mt = r3;
  r3 = JQT_ABS_PTR(_mt); }
Lbf09c: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbf09e: r3=0;
Lbf0a0: MEMB(r7+24)=r3;
Lbf0a2: goto Lbef9c;
Lbf0a4: r0=0; /* blx 0x22e60 存根 */
Lbf0a8_st0: MEM(r1+0)=r1;
Lbf0a8_st1: MEM(r1+4)=r3;
Lbf0a8_st2: MEM(r1+8)=r4;
Lbf0a8_st3: MEM(r1+12)=r5;
Lbf0a8: r1=r1+16;
Lbf0aa: r1=r1;
Lbf0ac: r0=((r0<<15))&0xffffffffu; Z=(r0==0); N=(((r0)>>31))&1;
Lbf0ae: r0=r0;
Lbf0b0_ld0: r1=MEM(r6+0);
Lbf0b0_ld1: r3=MEM(r6+4);
Lbf0b0_ld2: r4=MEM(r6+8);
Lbf0b0_ld3: r5=MEM(r6+12);
Lbf0b0_ld4: r7=MEM(r6+16);
Lbf0b0: r6=r6+20;
Lbf0b2: r1=r1;
Lbf0b4: r7=MEM(sp+128);
Lbf0b6: r6=r0;
Lbf0b8: r7=MEM(sp+136);
Lbf0ba: r6=r0;
Lbf0bc: MEM(sp+352)=r4;
Lbf0be: r6=r0;
Lbf0c0: r7=MEM(sp+32);
Lbf0c2: r6=r0;
Lbf0c4: r7=MEM(sp+72);
Lbf0c6: r6=r0;
Lbf0c8: r7=MEM(sp+120);
Lbf0ca: r6=r0;
Lbf0cc: r4=MEM(r2+124);
Lbf0ce: r6=r0;
Lbf0d0: sp-=(4*4); /* push r4, r5, r6, lr */
Lbf0d0_p0: MEM(sp+0)=r4;
Lbf0d0_p1: MEM(sp+4)=r5;
Lbf0d0_p2: MEM(sp+8)=r6;
Lbf0d2: r3=MEMB(r0+24);
Lbf0d4: {uint32_t a_=r0,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lbf0d6: {uint32_t a_=r1,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lbf0d8: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbf0da: if(!Z) goto Lbf0f0;
Lbf0dc: {uint32_t a_=r1,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf0de: { r0=(uint32_t)strlen((const char*)(uintptr_t)JQT_ABS_PTR(r0)); } /* blx 0x22c80 strlen 真实 */
Lbf0e2: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbf0e4: {uint32_t a_=r5,b_=r0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbf0e6: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf0e8: r0=F_23bec(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x23bec 完整 */
Lbf0ec: r3=1;
Lbf0ee: MEMB(r4+24)=r3;
Lbf0f0_l0: r4=MEM(sp+0);
Lbf0f0_l1: r5=MEM(sp+4);
Lbf0f0_l2: r6=MEM(sp+8);
Lbf0f0: sp+=(3*4); /* pop {r4, r5, r6, pc} */
Lbf0f0_r: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_bf140(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xbf140;
Lbf140: sp-=8;
Lbf142: r3=sp;
Lbf144: MEM(sp+4)=r0;
Lbf146: r2=sp+4;
Lbf148: {uint32_t a_=r3,b_=6; r3=(a_+b_); Z=(r3==0); C=(r3<a_); N=((r3)>>31)&1; V=(((a_^r3)&(b_^r3))>>31)&1;}
Lbf14a: r0=MEMB(r3);
Lbf14c: r1=MEMB(r2);
Lbf14e: MEMB(r2)=r0;
Lbf150: MEMB(r3)=r1;
Lbf152: r0=MEM(sp+4);
Lbf154: sp+=8;
Lbf156: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_bf4d0(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xbf4d0;
Lbf4d0: sp-=(5*4); /* push r4, r5, r6, r7, lr */
Lbf4d0_p0: MEM(sp+0)=r4;
Lbf4d0_p1: MEM(sp+4)=r5;
Lbf4d0_p2: MEM(sp+8)=r6;
Lbf4d0_p3: MEM(sp+12)=r7;
Lbf4d2: r7=r11;
Lbf4d4: r6=r10;
Lbf4d6: r5=r9;
/* ★ §124xxx 修复：翻译体 JNI 入口 r9=0（原版 ART 调用约定传 r9=渲染器单例）。
 *   F_bf4d0 渲染主循环所有 vtable dispatch（slot167/slot6/slot113/slot23 纹理上传）
 *   用 this=r5=r9；r9=0 → dispatch this=0 → MEM(0+92) 读零页 → 纹理上传/draw 失败 → 黑屏。
 *   回退：r9=0 时用渲染器单例（0x162400 槽，F_9d3d4 懒构造，vtable=0x1595c0 有效）。 */
if (r5 == 0) { r5 = MEM(0x162400); __android_log_print(6,"junqi","F_bf4d0 r9=0 回退渲染器单例=0x%08x", r5); }
/* §124au：主线程 EGL current=NO_SURFACE（线程本地）→ 主线程 glClear 无效。
 * 持续出帧由 GLThread（OnGraphCreate）内 spawn 的 jqt_gl_bridge_start_frame_loop 循环负责。
 * 主线程此处仅诊断渲染循环活跃。 */
{ static int _fr=0; _fr++; if ((_fr & 0x3f)==0)
    __android_log_print(4,"junqi","F_bf4d0 渲染循环活跃 #%d tid=%d（持续出帧走 GLThread 循环）", _fr, (int)gettid()); }
Lbf4d8: r4=r8;
Lbf4da: sp-=(4*4); /* push r4, r5, r6, r7 */
Lbf4da_p0: MEM(sp+0)=r4;
Lbf4da_p1: MEM(sp+4)=r5;
Lbf4da_p2: MEM(sp+8)=r6;
Lbf4da_p3: MEM(sp+12)=r7;
Lbf4dc: r4=MEM(783584+280);
Lbf4de: r2=MEM(783584+284);
Lbf4e0: sp-=308;
Lbf4e2: r4=(783590+r4);
Lbf4e4: r3=MEM(r4+r2);
Lbf4e6: {uint32_t a_=r0,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
/* ★ §124xxx 修复：r0=F_be930 传的渲染对象（原版=Java GL 对象 0xcd77e5e0）。
 *   翻译体 r0=MEM(0x162668)=0xce3a38d0（字符串 "android" 地址，对象图分歧）。
 *   检测 [r0] 是否有效 class 指针（art 区 0xd0000000-0xe0000000）：无效则回退渲染器单例。 */
{ uint32_t _r0cls = MEM(r0); if (!(_r0cls >= 0xd0000000u && _r0cls < 0xe0000000u)) { r0 = r5; r6 = r5; __android_log_print(6,"junqi","F_bf4d0 r0=0x%08x 无效class=0x%08x 回退渲染器单例=0x%08x", _r0, _r0cls, r5); } }
/* ★ §124yy 修复：bf4f0 `ldrb r3,[r0,#57]; cmp r3,#0; bne bf512` 是 dispatch 路径门。
 *   翻译体 r0=渲染器单例（回退后），byte[+57]=0（F_9f88c 只设 byte[+56]=1）→ 永不进 bf512
 *   → 渲染主循环提前退出（bf5f2）→ slot23 纹理上传不 dispatch → 黑屏。
 *   强制 byte[r0+57]=1（原版 in-game 该字节非零）→ 进 dispatch 路径。 */
MEMB(r0+57)=1;
Lbf4e8: r8=r2;
Lbf4ea: r3=MEM(r3);
Lbf4ec: MEM(sp+300)=r3;
Lbf4ee: r3=57;
Lbf4f0: r3=MEMB(r0+r3);
Lbf4f2: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbf4f4: if(!Z) goto Lbf512;
Lbf4f6: r2=r8;
Lbf4f8: r3=MEM(r4+r2);
Lbf4fa: r2=MEM(sp+300);
Lbf4fc: r3=MEM(r3);
Lbf4fe: {int32_t A=(int32_t)r2,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r2>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbf500: if(Z) goto Lbf504;
Lbf502: goto Lbf5f2;
Lbf504: sp+=308;
Lbf506_l2: r4=MEM(sp+8);
Lbf506_l3: r5=MEM(sp+12);
Lbf506: sp+=(4*4); /* pop {r2, r3, r4, r5} */
Lbf508: r8=r2;
Lbf50a: r9=r3;
Lbf50c: r10=r4;
Lbf50e: r11=r5;
Lbf510_l0: r4=MEM(sp+0);
Lbf510_l1: r5=MEM(sp+4);
Lbf510_l2: r6=MEM(sp+8);
Lbf510_l3: r7=MEM(sp+12);
Lbf510: sp+=(4*4); /* pop {r4, r5, r6, r7, pc} */
Lbf510_r: goto RET;
Lbf512: r7=MEM(783636+236);
Lbf514: r7=(783640+r7);
Lbf516: r0=MEM(r7);
/* ★ §124xxx 修复：r0=0x15be80 槽（翻译体=字符串 0xcd96ce80，原版=Java GL 对象）。
 *   F_bd530(r0) 读 [r0]=class 指针 → [class+0x10] slot4 dispatch。翻译体 r0 是字符串
 *   （[r0]=0x646e6100 ASCII）→ dispatch 垃圾 → F_bd530 返回垃圾 → r5=垃圾 → dispatch slot23 失败。
 *   回退：r0 无效（[r0] 非 class 指针区）时用渲染器单例（0x162400 槽）。 */
{ uint32_t _c = MEM(r0); if (!(_c >= 0xd0000000u && _c < 0xe0000000u)) { r0 = MEM(0x162400); __android_log_print(6,"junqi","F_bf4d0 Lbf516 r0=0x%08x 无效class=0x%08x 回退渲染器单例=0x%08x", r0, _c, MEM(0x162400)); } }
Lbf518: r0=F_bd530(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd530 完整 */
Lbf51c: {uint32_t a_=r0,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
/* ★ §124xxx 修复：r5=F_bd530 返回渲染对象。翻译体 F_bd530 输入错（原 0x15be80 槽字符串）→
 *   返回垃圾/0。r5 若无效（[r5] 非有效 vtable 0x1595c0 或 class 区）回退渲染器单例。
 *   §124yy 加强：r5 在范围但 [r5]（vtable）≠ 渲染器 vtable（0x1595c0 偏移或 shadow+0x1595c0）
 *   → 也是垃圾（F_bd530 dispatch slot4 返回的非渲染器对象）→ 回退渲染器单例。 */
{ uint32_t _rs = MEM(0x162400); if (!(r5 >= 0xa0000000u && r5 < 0xf0000000u) || (MEM(r5) != 0x1595c0u && MEM(r5) != (uint32_t)(g_jqt_shadow + 0x1595c0) && r5 != _rs)) { r5 = _rs; __android_log_print(6,"junqi","F_bf4d0 Lbf51c r5=0x%08x vt=0x%08x 无效 回退渲染器单例=0x%08x", r5, MEM(r5), _rs); } }
Lbf51e: r0=MEM(r7);
Lbf520: r0=F_bd548(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd548 完整 */
Lbf524: r3=sp+44;
Lbf526: r2=128;
Lbf528: {uint32_t a_=r3,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbf52a: r2=((r2<<1))&0xffffffffu; Z=(r2==0); N=(((r2)>>31))&1;
Lbf52c: MEM(sp+36)=r0;
Lbf52e: r0=MEM(r6+20);
Lbf530: r9=r3;
Lbf532: r0=F_c2da8(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xc2da8 完整 */
Lbf536: r3=MEM(r5);
Lbf538: r7=167;
Lbf53a: r7=((r7<<2))&0xffffffffu; Z=(r7==0); N=(((r7)>>31))&1;
Lbf53c: r3=MEM(r3+r7);
Lbf53e: r1=r9;
Lbf540: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf542: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbf544: r2=128;
Lbf546: r2=((r2<<1))&0xffffffffu; Z=(r2==0); N=(((r2)>>31))&1;
Lbf548: r11=r0;
/* ★ §124yy 修复：bf5b6 `cmp r11,#0; beq bf5c4` 是纹理上传门（fp=r11）。
 *   fp=vtable[167] dispatch 结果；vtable[167]=0（原版也 0，.data.rel.ro 只读）→ fp=0
 *   → bf5c2（vtable[23] 纹理上传）被跳过。强制 fp=1 → 纹理上传 dispatch 执行。 */
if (r11 == 0) { r11 = 1; __android_log_print(6,"junqi","F_bf4d0 fp(r11)=0 强制=1 → 纹理上传 dispatch"); }
Lbf54a: r1=r9;
Lbf54c: r0=MEM(r6+44);
Lbf54e: r0=F_c2da8(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xc2da8 完整 */
Lbf552: r3=MEM(r5);
Lbf554: r1=r9;
Lbf556: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf558: r3=MEM(r3+r7);
Lbf55a: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbf55c: r10=r0;
/* ★ §124yy 修复：bf5c6 `cmp sl,#0; beq bf5d4` 是第二纹理上传门（sl=r10）。
 *   sl=vtable[167] dispatch 结果=0 → bf5d2 跳过。强制 sl=1 → bf5d2 纹理上传 dispatch 执行。 */
if (r10 == 0) { r10 = 1; __android_log_print(6,"junqi","F_bf4d0 sl(r10)=0 强制=1 → 第二纹理上传 dispatch"); }
Lbf55e: r0=MEM(r6+48);
Lbf560: r0=F_bf140(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbf140 完整 */
Lbf564: MEM(sp+32)=r0;
Lbf566: r0=MEM(r6+52);
Lbf568: r0=F_bf140(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbf140 完整 */
Lbf56c: MEM(sp+28)=r0;
Lbf56e: r3=MEM(r5);
Lbf570: r1=MEM(783732+144);
Lbf572: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf574: r3=MEM(r3+24);
Lbf576: r1=(783738+r1);
Lbf578: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbf57a: r2=MEM(783740+140);
Lbf57c: r9=r0;
Lbf57e: r3=MEM(783744+140);
Lbf580: r0=MEM(r5);
Lbf582: r1=226;
Lbf584: r1=((r1<<1))&0xffffffffu; Z=(r1==0); N=(((r1)>>31))&1;
Lbf586: r7=MEM(r0+r1);
Lbf588: r2=(783756+r2);
Lbf58a: r3=(783758+r3);
Lbf58c: r1=r9;
Lbf58e: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf590: { r0=jqt_blx_dispatch(r7, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r7 vtable dispatch */
Lbf592: r3=r11;
Lbf594: MEM(sp)=r3;
Lbf596: r3=r10;
Lbf598: MEM(sp+4)=r3;
Lbf59a: r3=MEM(sp+32);
Lbf59c: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbf59e: r1=r9;
Lbf5a0: MEM(sp+8)=r3;
Lbf5a2: r3=MEM(sp+28);
Lbf5a4: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf5a6: MEM(sp+12)=r3;
Lbf5a8: r3=56;
Lbf5aa: r3=MEMB(r6+r3);
Lbf5ac: MEM(sp+16)=r3;
Lbf5ae: r3=MEM(sp+36);
Lbf5b0: r0=F_7874c(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x7874c 完整 */
Lbf5b4: r2=r11;
/* §124yy 诊断：fp(r11) 门 + r5 + vtable[23] 目标 */
{ __android_log_print(6,"junqi","F_bf4d0 Lbf5b4 fp(r11)=0x%08x r5=0x%08x r10(sl)=0x%08x r9=0x%08x", r11, r5, r10, r9); }
Lbf5b6: {int32_t A=(int32_t)r2,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r2>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbf5b8: if(Z) goto Lbf5c4;
Lbf5ba: r3=MEM(r5);
{ __android_log_print(6,"junqi","F_bf4d0 Lbf5ba vtable=0x%08x [vt+92]=0x%08x", r3, MEM(r3+92)); }
Lbf5bc: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf5be: r1=r11;
LLbf5c0: r3=MEM(r3+92);
/* §124zz-7：MEM(r3+92) 读出的是 RELATIVE 重定位后的绝对地址（.text 区函数）。
 * 原版 vtable 槽值 = so基址+0x9e0d4 等 → 32 位实指针（>=0xd0000000 区或 shadow 区）。
 * 旧"强制 0x9e2e0"补丁假设槽值是 shadow 偏移 → 把正确绝对地址误改 → 调错函数。
 * 修正：只把 >=MAP_HI 的实指针透传（JQT_ABS_PTR 保持），<MAP_HI 的当 shadow 偏移加基址。 */
{ uint32_t _mt = r3;
  r3 = JQT_ABS_PTR(_mt); }
/* §124yy 诊断：dispatch 目标 r3 + g_jqt_shadow */
{ __android_log_print(6,"junqi","F_bf4d0 Lbf5c2 dispatch r3=0x%08x shadow=0x%08x off=0x%08x", r3, (uint32_t)g_jqt_shadow, (r3>=(uint32_t)g_jqt_shadow? r3-(uint32_t)g_jqt_shadow : r3)); }
Lbf5c2: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbf5c4: r3=r10;
Lbf5c6: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbf5c8: if(Z) goto Lbf5d4;
Lbf5ca: r3=MEM(r5);
/* §124zz-7：vtable 槽值 RELATIVE 重定位后是绝对地址（.text 基址+0x9e0d4 等）。
 * MEM(vt+92) 读出的是绝对地址，JQT_ABS_PTR 对 >=MAP_HI 的实指针保持透传。
 * 旧代码对 slot 值做了偏移假设 → 实指针被当 shadow 偏移加基址 → 越界 miss。 */
Lbf5cc: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf5ce: r1=r10;
LLbf5d0: r3=MEM(r3+92);
/* §124zz-7：同 Lbf5c0——槽值是绝对地址，JQT_ABS_PTR 透传，不强制改。 */
{ uint32_t _mt = r3;
  r3 = JQT_ABS_PTR(_mt); }
Lbf5d2: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbf5d4: r2=r9;
Lbf5d6: {int32_t A=(int32_t)r2,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r2>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbf5d8: if(Z) goto Lbf5e4;
Lbf5da: r3=MEM(r5);
Lbf5dc: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf5de: r1=r9;
LLbf5e0: r3=MEM(r3+92);
/* §124zz-7：同 Lbf5c0——槽值是绝对地址，JQT_ABS_PTR 透传。 */
{ uint32_t _mt = r3;
  r3 = JQT_ABS_PTR(_mt); }
Lbf5e2: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbf5e4: r2=1;
Lbf5e6: r3=58;
Lbf5e8: MEMB(r6+r3)=r2;
Lbf5ea: r2=0;
Lbf5ec: r3=57;
Lbf5ee: MEMB(r6+r3)=r2;
Lbf5f0: goto Lbf4f6;
Lbf5f2: r0=0; /* blx 0x22e60 存根 */
Lbf5f6: r8=r8;
Lbf5f8: /* UNHANDLED cbnz r2, #0xbf674 */
Lbf5fa: r1=r1;
Lbf5fc: r0=((r0<<0xf))&0xffffffffu; Z=(r0==0); N=(((r0)>>31))&1;
Lbf5fe: r0=r0;
Lbf600_ld0: r3=MEM(r1+0);
Lbf600_ld1: r5=MEM(r1+4);
Lbf600_ld2: r6=MEM(r1+8);
Lbf600: r1=r1+12;
Lbf602: r1=r1;
Lbf604: r2=MEM(sp+440);
Lbf606: r6=r0;
Lbf608: r2=MEM(sp+448);
Lbf60a: r6=r0;
Lbf60c: r2=MEM(sp+472);
Lbf60e: r6=r0;
Lbf610: sp-=(4*4); /* push r4, r5, r6, lr */
Lbf610_p0: MEM(sp+0)=r4;
Lbf610_p1: MEM(sp+4)=r5;
Lbf610_p2: MEM(sp+8)=r6;
Lbf612: r3=57;
Lbf614: r2=0;
Lbf616: MEMB(r0+r3)=r2;
Lbf618: r3=58;
Lbf61a: r3=MEMB(r0+r3);
Lbf61c: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbf61e: if(!Z) goto Lbf622;
Lbf620_l0: r4=MEM(sp+0);
Lbf620_l1: r5=MEM(sp+4);
Lbf620_l2: r6=MEM(sp+8);
Lbf620: sp+=(3*4); /* pop {r4, r5, r6, pc} */
Lbf620_r: goto RET;
Lbf622: r3=MEM(783908+72);
Lbf624: r3=(783912+r3);
Lbf626: r0=MEM(r3);
Lbf628: r0=F_bd530(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd530 完整 */
Lbf62c: r1=MEM(783920+64);
Lbf62e: r3=MEM(r0);
Lbf630: {uint32_t a_=r0,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lbf632: r1=(783926+r1);
Lbf634: r3=MEM(r3+24);
Lbf636: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbf638: r2=MEM(783932+56);
Lbf63a: {uint32_t a_=r0,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lbf63c: r3=MEM(783936+56);
Lbf63e: r0=MEM(r4);
Lbf640: r1=226;
Lbf642: r1=((r1<<1))&0xffffffffu; Z=(r1==0); N=(((r1)>>31))&1;
Lbf644: r6=MEM(r0+r1);
Lbf646: r2=(783946+r2);
Lbf648: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbf64a: r3=(783950+r3);
Lbf64c: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf64e: { r0=jqt_blx_dispatch(r6, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r6 vtable dispatch */
Lbf650: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lbf652: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lbf654: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf656: r0=F_7874c(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x7874c 完整 */
Lbf65a: {int32_t A=(int32_t)r5,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r5>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lbf65c: if(Z) goto Lbf620;
Lbf65e: r3=MEM(r4);
Lbf660: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf662: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
LLbf664: r3=MEM(r3+92);
/* §124zz-7：同 Lbf5c0——槽值是绝对地址，JQT_ABS_PTR 透传。 */
{ uint32_t _mt = r3;
  r3 = JQT_ABS_PTR(_mt); }
Lbf666: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lbf668: goto Lbf620;
Lbf674: r6=MEM(r5+44);
Lbf676: r6=r0;
Lbf678: r6=MEM(r6+28);
Lbf67a: r6=r0;
Lbf67c: sp-=(2*4); /* push r4, lr */
Lbf67c_p0: MEM(sp+0)=r4;
Lbf67e: {uint32_t a_=r0,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lbf680: r0=F_bf610(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbf610 完整 */
Lbf684: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf686: {uint32_t a_=r0,b_=24; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf688: r0=F_25988(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x25988 完整 */
Lbf68c: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf68e: r0=F_25988(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x25988 完整 */
Lbf692: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lbf694_l0: r4=MEM(sp+0);
Lbf694: sp+=(1*4); /* pop {r4, pc} */
Lbf694_r: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_c5d50(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xc5d50;
Lc5d50: r0=MEM(r0+100);
Lc5d52: goto RET;
RET: return r0;
    return 0;
}

uint32_t F_c5e38(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
    g_cur_fn = 0xc5e38;
Lc5e38: sp-=(5*4); /* push r4, r5, r6, r7, lr */
Lc5e38_p0: MEM(sp+0)=r4;
Lc5e38_p1: MEM(sp+4)=r5;
Lc5e38_p2: MEM(sp+8)=r6;
Lc5e38_p3: MEM(sp+12)=r7;
Lc5e3a: r7=r11;
Lc5e3c: r6=r10;
Lc5e3e: r5=r9;
Lc5e40: r4=r8;
Lc5e42: sp-=(4*4); /* push r4, r5, r6, r7 */
Lc5e42_p0: MEM(sp+0)=r4;
Lc5e42_p1: MEM(sp+4)=r5;
Lc5e42_p2: MEM(sp+8)=r6;
Lc5e42_p3: MEM(sp+12)=r7;
Lc5e44: r5=MEM(810568+496);
Lc5e46: r1=MEM(810568+500);
Lc5e48: r4=MEM(810572+500);
Lc5e4a: r5=(810574+r5);
Lc5e4c: r3=MEM(r5+r1);
Lc5e4e: r2=MEM(810576+500);
Lc5e50: sp+=r4;
Lc5e52: r3=MEM(r3);
Lc5e54: r2=sp+r2;
Lc5e56: {uint32_t a_=r0,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
Lc5e58: MEM(r2)=r3;
Lc5e5a: r3=MEMB(r0);
Lc5e5c: r8=r1;
Lc5e5e: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc5e60: if(!Z) goto Lc5e84;
Lc5e62: r2=r8;
Lc5e64: r1=MEM(810600+476);
Lc5e66: r3=MEM(r5+r2);
Lc5e68: r1=sp+r1;
Lc5e6a: r2=MEM(r1);
Lc5e6c: r3=MEM(r3);
Lc5e6e: {int32_t A=(int32_t)r2,B=(int32_t)r3;int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r2>=r3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc5e70: if(Z) goto Lc5e74;
Lc5e72: goto Lc6034;
Lc5e74: r3=MEM(810616+464);
Lc5e76: sp+=r3;
Lc5e78_l2: r4=MEM(sp+8);
Lc5e78_l3: r5=MEM(sp+12);
Lc5e78: sp+=(4*4); /* pop {r2, r3, r4, r5} */
Lc5e7a: r8=r2;
Lc5e7c: r9=r3;
Lc5e7e: r10=r4;
Lc5e80: r11=r5;
Lc5e82_l0: r4=MEM(sp+0);
Lc5e82_l1: r5=MEM(sp+4);
Lc5e82_l2: r6=MEM(sp+8);
Lc5e82_l3: r7=MEM(sp+12);
Lc5e82: sp+=(4*4); /* pop {r4, r5, r6, r7, pc} */
Lc5e82_r: goto RET;
Lc5e84: r7=MEM(810632+452);
Lc5e86: r7=(810634+r7);
Lc5e88: r0=MEM(r7);
Lc5e8a: r0=F_bd530(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd530 完整 */
Lc5e8e: {uint32_t a_=r0,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lc5e90: r0=MEM(r7);
Lc5e92: r0=F_bd548(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xbd548 完整 */
Lc5e96: r3=MEM(810648+440);
Lc5e98: r2=128;
Lc5e9a: MEM(sp+28)=r0;
Lc5e9c: r3=sp+r3;
Lc5e9e: {uint32_t a_=r3,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lc5ea0: r2=((r2<<1))&0xffffffffu; Z=(r2==0); N=(((r2)>>31))&1;
Lc5ea2: r0=MEM(r6+24);
Lc5ea4: r9=r3;
Lc5ea6: r0=F_c2da8(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xc2da8 完整 */
Lc5eaa: r2=128;
Lc5eac: r1=sp+36;
Lc5eae: r2=((r2<<7))&0xffffffffu; Z=(r2==0); N=(((r2)>>31))&1;
Lc5eb0: r0=MEM(r6+48);
Lc5eb2: r10=r1;
Lc5eb4: r0=F_c2da8(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xc2da8 完整 */
Lc5eb8: r2=MEM(810684+408);
Lc5eba: r0=MEM(r6+72);
Lc5ebc: r7=167;
Lc5ebe: r2=sp+r2;
Lc5ec0: r11=r2;
Lc5ec2: {uint32_t a_=r2,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lc5ec4: r2=64;
Lc5ec6: r0=F_c2da8(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0xc2da8 完整 */
Lc5eca: r3=MEM(r4);
Lc5ecc: r7=((r7<<2))&0xffffffffu; Z=(r7==0); N=(((r7)>>31))&1;
Lc5ece: r1=r9;
Lc5ed0: r3=MEM(r3+r7);
Lc5ed2: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5ed4: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lc5ed6: MEM(sp+20)=r0;
Lc5ed8: r3=MEM(r4);
Lc5eda: r1=r10;
Lc5edc: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5ede: r3=MEM(r3+r7);
Lc5ee0: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lc5ee2: MEM(sp+16)=r0;
Lc5ee4: r3=MEM(r4);
Lc5ee6: r1=r11;
Lc5ee8: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5eea: r3=MEM(r3+r7);
Lc5eec: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lc5eee: r3=MEM(r6+76);
Lc5ef0: r11=r0;
Lc5ef2: {int32_t A=(int32_t)r3,B=(int32_t)(3);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc5ef4: if(!Z) goto Lc5ef8;
Lc5ef6: goto Lc602c;
Lc5ef8: {int32_t A=(int32_t)r3,B=(int32_t)(3);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=3);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc5efa: if((!Z&&(N==V))) goto Lc5fbc;
Lc5efc: {int32_t A=(int32_t)r3,B=(int32_t)(2);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=2);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc5efe: if(!Z) goto Lc5f02;
Lc5f00: goto Lc6024;
Lc5f02: r1=0;
Lc5f04: MEM(sp+24)=r1;
Lc5f06: {int32_t A=(int32_t)r3,B=(int32_t)(10);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=10);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc5f08: if(!Z) goto Lc5f0c;
Lc5f0a: goto Lc6016;
Lc5f0c: r1=MEM(810768+328);
Lc5f0e: r1=(810770+r1);
Lc5f10: r3=MEM(r4);
Lc5f12: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5f14: r3=MEM(r3+24);
Lc5f16: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lc5f18: r2=MEM(810780+320);
Lc5f1a: r9=r0;
Lc5f1c: r3=MEM(810784+320);
Lc5f1e: r0=MEM(r4);
Lc5f20: r1=132;
Lc5f22: r2=(810790+r2);
Lc5f24: r7=MEM(r0+r1);
Lc5f26: r3=(810794+r3);
Lc5f28: r1=r9;
Lc5f2a: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5f2c: { r0=jqt_blx_dispatch(r7, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r7 vtable dispatch */
Lc5f2e: r3=MEM(sp+16);
Lc5f30: r1=MEM(sp+20);
Lc5f32: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lc5f34: MEM(sp+4)=r3;
Lc5f36: r3=MEM(sp+24);
Lc5f38: MEM(sp)=r1;
Lc5f3a: r1=r11;
Lc5f3c: MEM(sp+8)=r1;
Lc5f3e: MEM(sp+12)=r3;
Lc5f40: r1=r9;
Lc5f42: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5f44: r3=MEM(sp+28);
Lc5f46: r0=F_77fa4(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x77fa4 完整 */
Lc5f4a: r1=MEM(sp+20);
Lc5f4c: r10=r0;
Lc5f4e: {int32_t A=(int32_t)r1,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r1>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc5f50: if(Z) goto Lc5f5a;
Lc5f52: r3=MEM(r4);
Lc5f54: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5f56: r3=MEM(r3+92);
Lc5f58: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lc5f5a: r2=MEM(sp+16);
Lc5f5c: {int32_t A=(int32_t)r2,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r2>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc5f5e: if(Z) goto Lc5f6a;
Lc5f60: r3=MEM(r4);
Lc5f62: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5f64: {uint32_t a_=r2,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lc5f66: r3=MEM(r3+92);
Lc5f68: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lc5f6a: r3=r11;
Lc5f6c: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc5f6e: if(Z) goto Lc5f7a;
Lc5f70: r3=MEM(r4);
Lc5f72: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5f74: r1=r11;
Lc5f76: r3=MEM(r3+92);
Lc5f78: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lc5f7a: r0=MEM(r4);
Lc5f7c: r2=MEM(810880+228);
Lc5f7e: r3=MEM(810880+232);
Lc5f80: r1=132;
Lc5f82: r7=MEM(r0+r1);
Lc5f84: r2=(810888+r2);
Lc5f86: r3=(810890+r3);
Lc5f88: r1=r9;
Lc5f8a: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5f8c: { r0=jqt_blx_dispatch(r7, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r7 vtable dispatch */
Lc5f8e: r1=r10;
Lc5f90: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lc5f92: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5f94: r0=F_77800(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x77800 完整 */
Lc5f98: r1=r9;
Lc5f9a: {int32_t A=(int32_t)r1,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r1>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc5f9c: if(Z) goto Lc5fa6;
Lc5f9e: r3=MEM(r4);
Lc5fa0: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5fa2: r3=MEM(r3+92);
Lc5fa4: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lc5fa6: r2=r10;
Lc5fa8: {int32_t A=(int32_t)r2,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r2>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc5faa: if(Z) goto Lc5fb6;
Lc5fac: r3=MEM(r4);
Lc5fae: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5fb0: r1=r10;
Lc5fb2: r3=MEM(r3+92);
Lc5fb4: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lc5fb6: r3=0;
Lc5fb8: MEMB(r6)=r3;
Lc5fba: goto Lc5e62;
Lc5fbc: {int32_t A=(int32_t)r3,B=(int32_t)(4);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=4);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc5fbe: if(Z) goto Lc601c;
Lc5fc0: {int32_t A=(int32_t)r3,B=(int32_t)(5);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=5);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc5fc2: if(!Z) goto Lc5f02;
Lc5fc4: r3=MEM(810952+164);
Lc5fc6: r9=r3;
Lc5fc8: r9=(810956+r9);
Lc5fca: r3=MEM(r4);
Lc5fcc: r1=MEM(810960+160);
Lc5fce: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5fd0: r3=MEM(r3+24);
Lc5fd2: r1=(810966+r1);
Lc5fd4: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lc5fd6: r1=MEM(r4);
Lc5fd8: r3=MEM(810972+152);
Lc5fda: r2=144;
Lc5fdc: r10=r0;
Lc5fde: r2=((r2<<2))&0xffffffffu; Z=(r2==0); N=(((r2)>>31))&1;
Lc5fe0: r7=MEM(r1+r2);
Lc5fe2: r3=(810982+r3);
Lc5fe4: r1=r10;
Lc5fe6: r2=r9;
Lc5fe8: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5fea: { r0=jqt_blx_dispatch(r7, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r7 vtable dispatch */
Lc5fec: r1=MEM(r4);
Lc5fee: r3=150;
Lc5ff0: r3=((r3<<2))&0xffffffffu; Z=(r3==0); N=(((r3)>>31))&1;
Lc5ff2: {uint32_t a_=r0,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lc5ff4: r3=MEM(r1+r3);
Lc5ff6: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc5ff8: r1=r10;
Lc5ffa: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lc5ffc: r1=r10;
Lc5ffe: MEM(sp+24)=r0;
Lc6000: {int32_t A=(int32_t)r1,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r1>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc6002: if(Z) goto Lc600e;
Lc6004: r3=MEM(r4);
Lc6006: {uint32_t a_=r4,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc6008: r1=r10;
Lc600a: r3=MEM(r3+92);
Lc600c: { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); } /* blx r3 vtable dispatch */
Lc600e: r3=MEM(r6+76);
Lc6010: {int32_t A=(int32_t)r3,B=(int32_t)(10);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=10);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
Lc6012: if(Z) goto Lc6016;
Lc6014: goto Lc5f0c;
Lc6016: r1=MEM(811032+96);
Lc6018: r1=(811036+r1);
Lc601a: goto Lc5f10;
Lc601c: r3=MEM(811040+92);
Lc601e: r9=r3;
Lc6020: r9=(811044+r9);
Lc6022: goto Lc5fca;
Lc6024: r3=MEM(811048+88);
Lc6026: r9=r3;
Lc6028: r9=(811052+r9);
Lc602a: goto Lc5fca;
Lc602c: r2=MEM(811056+84);
Lc602e: r9=r2;
Lc6030: r9=(811060+r9);
Lc6032: goto Lc5fca;
Lc6034: r0=0; /* blx 0x22e60 存根 */
Lc6038: MEMH(r7+r1)=r2;
Lc603a: r1=r1;
Lc603c: r0=((r0<<15))&0xffffffffu; Z=(r0==0); N=(((r0)>>31))&1;
Lc603e: r0=r0;
Lc6040: ; /* bkpt #148 */
Lc6042: /* UNHANDLED .byte 0xff, 0xff */
Lc6044: r4+=r4+C;
Lc6046: r0=r0;
Lc6048: r4+=r5+C;
Lc604a: r0=r0;
Lc604c: r6=(int32_t)(int16_t)MEMH(r6+r7);
Lc604e: r1=r1;
Lc6050: r4&=r4; Z=(r4==0); N=((r4)>>31)&1;
Lc6052: r0=r0;
Lc6054: r4=((r4)>=32?0:((int32_t)r4>>((r4)&31)));
Lc6056: r0=r0;
Lc6058: r6=((r6)>=32?0:((int32_t)r6>>((r6)&31)));
Lc605a: r6=r0;
Lc605c: r6=((r0<<5))&0xffffffffu; Z=(r6==0); N=(((r6)>>31))&1;
Lc605e: r6=r0;
Lc6060: r6+=r0+C;
Lc6062: r6=r0;
Lc6064: {uint32_t a_=r0,b_=4; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc6066: r6=r0;
Lc6068: r2=((r7<<2))&0xffffffffu; Z=(r2==0); N=(((r2)>>31))&1;
Lc606a: r6=r0;
Lc606c: r0^=r3; Z=(r0==0); N=((r0)>>31)&1;
Lc606e: r6=r0;
Lc6070: r2=((r0)>=32?0:((int32_t)r2>>((r0)&31)));
Lc6072: r6=r0;
Lc6074: r4=118;
Lc6076: r6=r0;
Lc6078: r0^=r0; Z=(r0==0); N=((r0)>>31)&1;
Lc607a: r6=r0;
Lc607c: {uint32_t a_=r7,b_=212; r7=(a_-b_); Z=(r7==0); C=(a_>=b_); N=((r7)>>31)&1; V=(((a_^b_)&(a_^r7))>>31)&1;}
Lc607e: r6=r0;
Lc6080: r0=((r3)>=32?0:((r0<<((r3)&31)))&0xffffffffu);
Lc6082: r6=r0;
Lc6084: {uint32_t a_=r7,b_=220; r7=(a_-b_); Z=(r7==0); C=(a_>=b_); N=((r7)>>31)&1; V=(((a_^b_)&(a_^r7))>>31)&1;}
Lc6086: r6=r0;
Lc6088: sp-=(4*4); /* push r4, r5, r6, lr */
Lc6088_p0: MEM(sp+0)=r4;
Lc6088_p1: MEM(sp+4)=r5;
Lc6088_p2: MEM(sp+8)=r6;
Lc608a: {uint32_t a_=r0,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lc608c: {uint32_t a_=r1,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc608e: {uint32_t a_=r1,b_=0; r4=(a_+b_); Z=(r4==0); C=(r4<a_); N=((r4)>>31)&1; V=(((a_^r4)&(b_^r4))>>31)&1;}
Lc6090: { r0=(uint32_t)strlen((const char*)(uintptr_t)JQT_ABS_PTR(r0)); } /* blx 0x22c80 strlen 真实 */
Lc6094: {uint32_t a_=r5,b_=80; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
Lc6096: {uint32_t a_=r4,b_=r0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
Lc6098: {uint32_t a_=r4,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
Lc609a: {uint32_t a_=r5,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
Lc609c: r0=F_23bec(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x23bec 完整 */
Lc60a0_l0: r4=MEM(sp+0);
Lc60a0_l1: r5=MEM(sp+4);
Lc60a0_l2: r6=MEM(sp+8);
Lc60a0: sp+=(3*4); /* pop {r4, r5, r6, pc} */
Lc60a0_r: goto RET;
RET: return r0;
    return 0;
}

