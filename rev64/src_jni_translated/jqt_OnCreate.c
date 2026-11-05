// 机械化翻译自原版 32 位 libjunqi.so @ 0x23561 (OnCreate) — 86 标签
// bl/blx 存根 r0=0（与回归一致）；.rodata/.text 常量池需运行时明文注入
// ★ 对象图修复：_Znwj/_ZdlPv（operator new/delete）用 malloc/free 自实现，避免依赖 libc++_shared.so
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <android/log.h>
#include "jqt_mem.h"
/* 自实现 operator new/delete（避免 libc++_shared.so 依赖——MuMu 转译层对 libc++_shared 支持不完整，
 * 原版 so 用 libstdc++ 的 _Znwj/_ZdlPv，但翻译体用 malloc/free 等价实现，减少运行时依赖） */
/* ★ §124ac：_Znwj/_ZdlPv 配对 guard（比 jqt_heap_ok 更准——只 free 真正分配过的指针）。
 *   F_75ce8 收垃圾 this（对象图未构造）→ free(0x24000000/0x6ae4ea00) SIGSEGV/SIGABRT。
 *   维护"已分配指针集合"：_Znwj/_Znaj 记录，_ZdlPv/_ZdaPv 只 free 集合中的指针，
 *   垃圾（非 malloc 返回）直接 SKIP → 主线程崩溃消失。 */
#define JQ_ALLOC_MAX 65536
static uint32_t g_jq_alloc_tbl[JQ_ALLOC_MAX];
static uint32_t g_jq_alloc_sz[JQ_ALLOC_MAX];   /* ★ CANARY-DIAG：记录每分配尺寸 */
static int g_jq_alloc_n = 0;
static void jq_alloc_add(uint32_t p){ if (g_jq_alloc_n < JQ_ALLOC_MAX) g_jq_alloc_tbl[g_jq_alloc_n++] = p; }
static int jq_alloc_has(uint32_t p){ for (int i=0;i<g_jq_alloc_n;i++) if (g_jq_alloc_tbl[i]==p) return 1; return 0; }
static int jq_alloc_idx(uint32_t p){ for (int i=0;i<g_jq_alloc_n;i++) if (g_jq_alloc_tbl[i]==p) return i; return -1; }
static void jq_alloc_remove(uint32_t p){
    for (int i=0;i<g_jq_alloc_n;i++) if (g_jq_alloc_tbl[i]==p) { g_jq_alloc_tbl[i]=g_jq_alloc_tbl[--g_jq_alloc_n]; g_jq_alloc_sz[i]=g_jq_alloc_sz[g_jq_alloc_n]; return; }
}
/* ★ CANARY-DIAG（§124y-7，堆损坏 0xd3a40010 调查）：_Znwj 分配尾部写 canary，
 *   _ZdlPv 检查——若被破坏，说明该分配被越界写（定位溢出源）。
 *   canary = 4×0xDEADBEEF（16B）写在 [p+sz, p+sz+16)。malloc 实际给 ≥sz+16B，
 *   但 scudo 可能给 sz+16 附近——用 malloc_usable_size 保证 canary 在已分配区内。 */
#define JQ_CANARY_MAGIC 0xDEADBEEFu

/* 返回 1 若 p 对应分配 canary 完好（canary 在 p+sz，分配 sz+16 保证可写） */
static int jq_alloc_canary_ok(uint32_t p, uint32_t *out_sz){
    int idx = jq_alloc_idx(p);
    if (idx < 0) return 1;
    uint32_t sz = g_jq_alloc_sz[idx];
    if (out_sz) *out_sz = sz;
    uint32_t *can = (uint32_t*)(uintptr_t)(p + sz);
    for (int i=0;i<4;i++) if (can[i] != JQ_CANARY_MAGIC) return 0;
    return 1;
}

static void jq_canary_write(uint32_t p, uint32_t sz){
    uint32_t *can = (uint32_t*)(uintptr_t)(p + sz);
    for (int i=0;i<4;i++) can[i] = JQ_CANARY_MAGIC;
}

void* _Znwj(unsigned int sz){
    uint32_t alloc_sz = sz ? sz : 1;
    /* ★ HUGE-ALLOC-DIAG（§124y-7，redroid 无真实 surface 崩溃 malloc(4.29GB) 调查）：
     *   巨大分配（>16MB）记录来源（g_cur_fn+backtrace），防崩溃钳制到 16MB。
     *   正常分配（棋盘纹理 737×751×3=1.6MB，对象几十-几百 B）不受影响。 */
    if (alloc_sz > 0x1000000u) {
        extern volatile uint32_t g_cur_fn;
        void *ra = __builtin_return_address(0);
        Dl_info dli; const char *mod = "?"; uintptr_t off = 0;
        if (dladdr(ra, &dli) && dli.dli_fname) { mod = dli.dli_fname; off = (uintptr_t)ra - (uintptr_t)dli.dli_fbase; }
        __android_log_print(6,"junqi","★ HUGE-ALLOC: _Znwj(%u=0x%x) g_cur_fn=0x%08x off=0x%08x mod=%s tid=%d（钳制 16MB）",
            alloc_sz, alloc_sz, g_cur_fn, (uint32_t)off, mod, (int)gettid());
        alloc_sz = 0x1000000u;
    }
    void *p = malloc(alloc_sz + 16);
    uint32_t pp = (uint32_t)(uintptr_t)p;
    if (g_jq_alloc_n < JQ_ALLOC_MAX) {
        g_jq_alloc_tbl[g_jq_alloc_n] = pp;
        g_jq_alloc_sz[g_jq_alloc_n] = alloc_sz;
        g_jq_alloc_n++;
    }
    jq_canary_write(pp, alloc_sz);
    return p;
}
void* _Znaj(unsigned int sz){
    uint32_t alloc_sz = sz ? sz : 1;
    void *p = malloc(alloc_sz + 16);
    uint32_t pp = (uint32_t)(uintptr_t)p;
    if (g_jq_alloc_n < JQ_ALLOC_MAX) {
        g_jq_alloc_tbl[g_jq_alloc_n] = pp;
        g_jq_alloc_sz[g_jq_alloc_n] = alloc_sz;
        g_jq_alloc_n++;
    }
    jq_canary_write(pp, alloc_sz);
    return p;
}
void _ZdlPv(void* p){
    extern volatile uint32_t g_cur_fn;
    uint32_t pp = (uint32_t)(uintptr_t)p;
    void *ra = __builtin_return_address(0);
    Dl_info dli; const char *mod = "?"; uintptr_t off = 0;
    if (dladdr(ra, &dli) && dli.dli_fname) { mod = dli.dli_fname; off = (uintptr_t)ra - (uintptr_t)dli.dli_fbase; }
    if (!jq_alloc_has(pp)) {
        __android_log_print(6,"junqi","_ZdlPv SKIP 未分配指针 p=0x%08x g_cur_fn=0x%08x off=0x%08x mod=%s tid=%d", pp, g_cur_fn, (uint32_t)off, mod, (int)gettid());
        return;
    }
    /* ★ CANARY-DIAG：检查 canary（越界写检测） */
    uint32_t sz = 0;
    int ok = jq_alloc_canary_ok(pp, &sz);
    if (!ok) {
        __android_log_print(6,"junqi","★ CANARY-CORRUPT: p=0x%08x sz=%u g_cur_fn=0x%08x off=0x%08x mod=%s tid=%d（越界写检测，堆损坏候选）",
            pp, sz, g_cur_fn, (uint32_t)off, mod, (int)gettid());
    }
    jq_alloc_remove(pp);
    __android_log_print(6,"junqi","_ZdlPv(p=0x%08x sz=%u) g_cur_fn=0x%08x off=0x%08x mod=%s tid=%d", pp, sz, g_cur_fn, (uint32_t)off, mod, (int)gettid());
    free(p);
}
void _ZdaPv(void* p){
    uint32_t pp = (uint32_t)(uintptr_t)p;
    if (!jq_alloc_has(pp)) return;
    jq_alloc_remove(pp);
    free(p);
}
uint32_t jqt_OnCreate(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t jqt_OnCreate(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    uint32_t r0=_r0, r1=_r1, r2=_r2, r3=_r3, r4=_r4, r5=_r5, r6=_r6, r7=_r7, r8=_r8, r9=_r9, r10=_r10, r11=_r11, r12=0;
    uint32_t sp=_sp, lr=0;
    int Z=0, C=0, N=0, V=0;
L23562: r4=MEM(144740+96);
L23564: {uint32_t a_=r0,b_=0; r5=(a_+b_); Z=(r5==0); C=(r5<a_); N=((r5)>>31)&1; V=(((a_^r5)&(b_^r5))>>31)&1;}
L23566: {uint32_t a_=r2,b_=0; r7=(a_+b_); Z=(r7==0); C=(r7<a_); N=((r7)>>31)&1; V=(((a_^r7)&(b_^r7))>>31)&1;}
L23568: r4=(144748+r4);
L2356a: r3=MEM(r4);
L2356c: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
L2356e: if(Z) goto L23596;
L23570: r3=MEM(144756+84);
L23572: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L23574: r3=(144760+r3);
L23576: r0=MEM(r3);
L23578: r0=0; /* bl 0xbdacc 存根 */
L2357c: r6=MEM(r4);
L2357e: {int32_t A=(int32_t)r6,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r6>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
L23580: if(Z) goto L23592;
L23582: {uint32_t a_=r6,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
L23584: r0=0; /* bl 0x234f8 存根 */
L23588: {uint32_t a_=r6,b_=0; r0=(a_+b_); Z=(r0==0); C=(r0<a_); N=((r0)>>31)&1; V=(((a_^r0)&(b_^r0))>>31)&1;}
L2358a: _ZdlPv((void*)r0); r0=0; /* blx 0x22bfc 真实 operator delete */
L2358e: r3=0;
L23590: MEM(r4)=r3;
L23592: r0=0; /* bl 0xba12c 存根 */
L23596: r4=MEM(144792+52);
L23598: r0=0; /* bl 0xb9edc 存根 */
L2359c: r4=(144800+r4);
L2359e: r3=MEM(r4);
L235a0: {int32_t A=(int32_t)r3,B=(int32_t)(0);int64_t S=(int64_t)A-(int64_t)B;uint32_t t=(uint32_t)S;Z=(t==0);C=(r3>=0);N=(t>>31)&1;V=(((A^B)&(A^(int32_t)S))<0);}
L235a2: if(Z) goto L235b4;
L235a4: r3=MEM(144808+40);
L235a6: {uint32_t a_=r5,b_=0; r1=(a_+b_); Z=(r1==0); C=(r1<a_); N=((r1)>>31)&1; V=(((a_^r1)&(b_^r1))>>31)&1;}
L235a8: {uint32_t a_=r7,b_=0; r2=(a_+b_); Z=(r2==0); C=(r2<a_); N=((r2)>>31)&1; V=(((a_^r2)&(b_^r2))>>31)&1;}
L235aa: r3=(144812+r3);
L235ac: r0=MEM(r3);
L235ae: r0=0; /* bl 0xbe47c 存根 */
L235b2_l1: r4=MEM(sp+4);
L235b2_l2: r5=MEM(sp+8);
L235b2_l3: r6=MEM(sp+12);
L235b2_l4: r7=MEM(sp+16);
L235b2: sp+=(5*4); /* pop {r3, r4, r5, r6, r7, pc} */
L235b2_r: goto RET;
L235b4: r0=80;
L235b6: r0=(uint32_t)_Znwj(r0); /* blx 0x232f8 真实 operator new */
L235ba: {uint32_t a_=r0,b_=0; r6=(a_+b_); Z=(r6==0); C=(r6<a_); N=((r6)>>31)&1; V=(((a_^r6)&(b_^r6))>>31)&1;}
L235bc: r0=0; /* bl 0x24250 存根 */
L235c0: MEM(r4)=r6;
L235c2: goto L235a4;
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
L232f8_st0: MEM(r6+0)=r0;
L232f8: r6=r6+4;
L232fa: goto L2381c;
L237ec: ; /* 外部跳转目标 */
L2381c: ; /* 外部跳转目标 */
RET: return r0;
}
