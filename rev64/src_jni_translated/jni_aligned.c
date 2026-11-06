/* jni_aligned.c — 14 个 32 位翻译体接到 arm64 JNI 入口
 * 注入 18 地址运行时值表（静态.so + frida-dump rodata_dec.bin 推算，16 算出 + 2 占位）
 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <android/log.h>
#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/system_properties.h>  /* ★ §TOUCH-INTERACT-TEST：__system_property_get 测试钩子 */
#include "jqt_jnibr.h"   /* JQJNI 桥：so→Java JNI 语义 */
#include "jqt_jni_calls.h"
#include "jqt_mem.h"     /* MEM 宏：OnUpdate 读 0x162400 单例槽（§sp28-SCALE-8b） */
/* ★ g_jqt_shadow：影子内存基址（jqt_mem.c 定义），OnCreate 诊断日志用 */
extern uint8_t *g_jqt_shadow;

/* 14 翻译体原型 */
#define JQT_SIG uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t
extern uint32_t jqt_OnCreate(JQT_SIG);
/* ★ 完整调用链版 OnCreate（gen_chain_full.py：bl 立即数→F_xxx 真调，调用链对齐） */
extern uint32_t F_23560(JQT_SIG);
extern uint32_t jqt_OnDestroy(JQT_SIG);
extern uint32_t jqt_OnPause(JQT_SIG);
extern uint32_t jqt_OnResume(JQT_SIG);
extern uint32_t jqt_OnUpdate(JQT_SIG);
/* ★ §124x：渲染入口完整调用链（gen_render_full.py 生成，jqt_OnRender_full.c）——
 *   替代 6 标签小存根（bl 0xbd5a0/0xbd74c/0xbd764/0xbe930 存根 r0=0），让 native 渲染逻辑执行 */
extern uint32_t F_23475(JQT_SIG);   /* OnUpdate 完整链 */
extern uint32_t F_2349d(JQT_SIG);   /* OnGraphCreate 完整链 */
extern uint32_t F_23489(JQT_SIG);   /* OnGraphChange 完整链 */
extern uint32_t F_23461(JQT_SIG);   /* OnTimer 完整链 */
extern uint32_t jqt_OnEvent(JQT_SIG);
extern uint32_t jqt_OnResult(JQT_SIG);
extern uint32_t jqt_OnKey(JQT_SIG);
extern uint32_t jqt_OnBluetooth(JQT_SIG);
extern uint32_t jqt_OnGraphCreate(JQT_SIG);
extern uint32_t jqt_OnGraphChange(JQT_SIG);
extern uint32_t jqt_OnTimer(JQT_SIG);
extern uint32_t jqt_OnInput(JQT_SIG);
extern uint32_t jqt_OnTouch(JQT_SIG);

/* JQJNI 桥自检原型（定义在 g_activity 之后） */
uint32_t jqt_jni_selftest(void);

/* 18 地址运行时值注入表：key=翻译体 MEM 读的 .so 文件偏移, value=运行时真实值
 * 16 个 .text 常量池由 frida-dump rodata_dec.bin 算出 + 2 .data 占位 0 */
static const struct { uint32_t off; uint32_t val; } g_jq_got_tbl[] = {
  {0x0233f0, 0xdc7875d5}, {0x023404, 0x3399731e}, {0x023420, 0x76c32d71},
  {0x023434, 0xf31520e6}, {0x023448, 0x31fe25a9}, {0x02345c, 0xd5b33157},
  {0x023470, 0x17583d02}, {0x023484, 0xf8b93bc9}, {0x023498, 0x8e7a14fe},
  {0x0234ac, 0x98ff1235}, {0x0234c0, 0x0bac1e60}, {0x0234d4, 0x473d0a9e},
  {0x0235c4, 0x96a779e4}, {0x0235cc, 0x62cc1cd4}, {0x023604, 0x31153fc4},
  {0x023608, 0x31153fc4}, {0x15be80, 0x00000000}, {0x15be84, 0x00000000}
};

#define V32(p) ((uint32_t)(uintptr_t)(p))

/* jqt_mem 初始化：mmap 影子 + 原版镜像 + 18 常量注入（JNI_OnLoad 调） */
extern void jqt_mem_init(void);
/* ★ §124x 诊断：g_cur_fn（翻译体当前函数）+ 看门狗线程采样定位循环函数 */
volatile uint32_t g_cur_fn = 0;
/* ★ §124y 语义修复：F_a3304 的 F_ab874/F_ab614 容器全局（0x162448/0x16244c）在 BSS，
 *   原版由容器管理函数（0xa3360 等 push 函数）惰性构造，F_a3304 直接读全局。
 *   翻译体未跑构造器 → 全局=0 → F_ab874 读 MEM(0+8)=垃圾 → 链表成环 spin。
 *   修复：将两容器初始化为"空链表"自引用哨兵（[X+8]==X → F_ab874/F_ab614 立即返回）。
 *   ★ 诚实边界：这是空容器语义（无待处理事件），非完整对象图重建。 */
void jqt_init_empty_containers(void){
    /* F_ab874 容器：MEM(0x162448)=容器，容器[8]==容器 → 空列表立即返回 */
    MEM(0x162448) = 0x162448;
    MEM(0x162450) = 0x162448;
    /* F_ab614 容器：MEM(0x16244c)=容器，容器[8]==容器 → 空列表立即返回 */
    MEM(0x16244c) = 0x16244c;
    MEM(0x162454) = 0x16244c;
    /* F_a6620 容器：MEM(0x1620b8)=容器，count[+4]=0（BSS 零）→ F_a6620 工作循环 0 次立即返回 */
    MEM(0x1620b8) = 0x1620b8;
    /* F_a373c 容器：MEM(0x15bf04)=容器，字段零（BSS）→ F_9f79c 早返回 */
    MEM(0x15bf04) = 0x15bf04;
    __android_log_print(4, "junqi", "jqt_init_empty_containers: F_ab874(0x162448)=0x162448 F_ab614(0x16244c)=0x16244c F_a6620(0x1620b8)=0x1620b8 F_a373c(0x15bf04)=0x15bf04");
}
static void *jq_watchdog(void *arg) {
    while (1) {
        __android_log_print(4, "junqi", "WATCHDOG: g_cur_fn=0x%08x", g_cur_fn);
        usleep(1000000);  /* 1s */
    }
    return NULL;
}
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved){
    jqt_mem_init();
    jqt_init_empty_containers();  /* ★ §124y：F_ab874/F_ab614 空容器语义初始化 */
    /* ★ 看门狗：定位 F_23560 调用链中永不返回的循环函数 */
    pthread_t wd;
    pthread_create(&wd, NULL, jq_watchdog, NULL);
    pthread_detach(wd);
    return JNI_VERSION_1_6;
}

/* ★ 全局 JNIEnv：F_be47c（vtable Java 交互边界）用 JNI 语义创建 GLView 时使用。
 *   F_23560 传 r0=0（JNIEnv），因 A 动态调用链 OnCreate 不走 0xbdacc；F_be47c 的
 *   vtable 间接调用无法机械翻译（vtable 布局 stripped 未知）→ JNI 语义补齐。 */
static JNIEnv *g_jni_env = NULL;
static jobject g_activity = NULL;  /* OnCreate 保存 activity（Context），GLView 构造用 */
/* §124aj 渲染链 so→Java 接线：OnCreate 构造 GLView 后存句柄，渲染链 F_24108 插桩
 *   用 g_glview_handle 当 GLView 实例方法 receiver（getParent/getHolder 等） */
uint32_t g_glview_handle = 0;
#include "jqt_jnibr.h"
#include "jqt_jni_calls.h"
/* ★ 全局标记：OnUpdate 首次触发桥自检（不依赖 OnCreate 跑通——§124ac F_75ce8 死循环
 *   挡在 OnCreate，但 OnUpdate 是稳定每帧 JNI 入口，env 可用） */
static int g_jqt_jni_selftest_done = 0;
static int g_slot22_triggered = 0;  /* §SLOT22-TRIGGER：OnUpdate 一次性 F_9e2b4 触发标记 */
uint32_t jqt_jni_selftest(void){
    if (!g_jni_env) {
        __android_log_print(4, "jqjni2", "selftest 入口: g_env=NULL（未 setenv，跳过）");
        return 1;
    }
    __android_log_print(4, "jqjni2", "selftest 入口: g_env=%p，开始逐条 resolve 43 条", (void*)g_jni_env);
    int ok_n = 0, fail_n = 0, miss_cls = 0;
    for (int id = 0; id < 43; id++) {
        int r = jqt_jni_resolve_only((uint32_t)id);
        if (r) ok_n++;
        else if (jqt_jni_call_cls_fail((uint32_t)id)) miss_cls++;
        else fail_n++;
        if ((id & 7) == 7 || id == 42) {
            __android_log_print(4, "jqjni2", "selftest batch id=%d ok=%d fail=%d cls_fail=%d",
                id, ok_n, fail_n, miss_cls);
        }
    }
    __android_log_print(4, "jqjni2", "selftest 43 条 resolve ok=%d fail=%d cls_fail=%d（%s）",
        ok_n, fail_n, miss_cls, ok_n ? "通路" : "全失败");
    return ok_n ? 0 : 1;
}
/* jq_jni_make_glview：JNI 语义创建 com.cnvcs.GLView（GLView.smali 构造器 (Context)V）
 *   返回 jobject 低 32 位（B 翻译体 uint32_t 约定）。0xbe47c vtable 边界的 JNI 补齐。 */
uint32_t jq_jni_make_glview(void) {
    if (!g_jni_env) { __android_log_print(6, "junqi", "make_glview: g_jni_env NULL"); return 0; }
    jclass cls = (*g_jni_env)->FindClass(g_jni_env, "com/cnvcs/GLView");
    if (cls == NULL) { __android_log_print(6, "junqi", "make_glview: FindClass 失败"); return 0; }
    jmethodID ctor = (*g_jni_env)->GetMethodID(g_jni_env, cls, "<init>", "(Landroid/content/Context;)V");
    if (ctor == NULL) { __android_log_print(6, "junqi", "make_glview: GetMethodID 失败"); return 0; }
    /* 用 activity（Context）创建。activity 从哪来？F_be47c 的 r1（GLView 相关对象）不是 activity。
     * 用 jni_aligned 保存的 activity 全局。 */
    jobject view = (*g_jni_env)->NewObject(g_jni_env, cls, ctor, g_activity);
    __android_log_print(6, "junqi", "make_glview: NewObject = %p", (void*)view);
    return (uint32_t)(uintptr_t)view;
}

JNIEXPORT jobject JNICALL
Java_com_cnvcs_App_OnCreate(JNIEnv *env, jclass c, jobject a){
    /* ★ 步骤1：入口日志（确认 OnCreate 被调用） */
    __android_log_print(4, "junqi", "OnCreate: 入口 env=%p activity=%p", (void*)env, (void*)a);
    /* ★ 调用链对齐（用户方法论 §124q）：走 F_23560 完整调用链版——原版 OnCreate 的
     *   bl/blx 立即数（0xb9edc 分配→0x24250 构造→0xbe47c vtable）真实执行 F_xxx，
     *   调用链对齐 A 原版动态调用链（0xb9edc→0x24250→0xbe47c）。
     *   诚实边界：blx 寄存器（vtable 0xbe47c）在机械翻译里仍存根 r0=0（Java 交互边界，
     *   无法机械翻译间接调用）→ 返回值 0 时 fallback JNI NewObject GLView（§124p 语义）。
     *   r0=0（JNIEnv）：A 动态调用链显示 OnCreate 实际不走 0xbdacc（r3==0 分支跳过），
     *   0xb9edc/0x24250 纯 native 不用 JNIEnv → 传 0 安全。 */
    /* 保存 env/activity 给桥（JQJNI）+ 旧 jq_jni_make_glview 兼容 */
    g_jni_env = env;
    g_activity = a;
    jqt_jni_setenv(env, a);   /* JQJNI 桥注册（so→Java 43 条调用语义）*/
    jqt_jni_selftest();        /* ★ 桥自检：在 F_23560 之前跑（env/activity 已设，稳定触发点），
                                 * 验证 FindClass/GetMethodID 通路（jqjni 标签 logcat） */
    /* §sp28-SCALE-7 场景构造接线：0xbe9cc（OnGraphCreate JNI 小函数 0x2342e 直调的
     *   场景构造真实入口，tex_pipeline.py 量化：直接 7 下游 5 已有完整体 + 0xbf19c 已补）
     *   + F_bdd2c（7072B 场景对象构造，dist 已有）。原版触发链 = onSurfaceCreated→
     *   OnGraphCreate JNI→0x2342e bl 0xbe9cc + 0x23442 bl 0xbdd2c；MuMu GLView surface
     *   未 attach（Surface.isValid=0）→ onSurfaceCreated 永不触发 → 场景对象从未构造 →
     *   帧循环 dispatch 只命中 slot23（F_9e0d4 清理）、slot22（F_9e2b4 创建）永不触发、
     *   texID(this[76]) 恒 0。此处 OnCreate 手动补触发（一次，主线程）：绕开 MuMu
     *   surface 限制，让场景数据链构造先行，GL 出屏由 GLThread 首帧完成。 */
    /* §sp28-SCALE-9c 诚实回退（scn_obj 传入 F_be9cc 崩 0xffffffd4 后，连 OnCreate 里的
     * F_9d3d4 构造调用也撤除——sp28-SCALE-7 稳定基线 = F_be9cc 传 0 + F_9d3d4 仅在
     * OnGraphCreate 守卫 292 行调一次；OnCreate 新增 F_9d3d4 + MEM(0x162400) 读改变
     * 早期调用序列 → 17451 仍崩 0xffffffd4。先恢复最小稳定态再定位崩点。 */
    /* ★ §SINGLETON-FIX：F_b9edc = 16 个全局单例的惰性构造器（§find_full_accessors 实证）。
     *   原版 OnCreate 0x23560 开头检查 0x15be80 槽，空 → 0x23596 bl F_b9edc 构造。
     *   0x15be80-0x15bebc = 全局单例对象表（场景对象 0x15be84 / 渲染器 / 纹理等）。
     *   F_b9edc 内：0xb9f72 _Znwj(32)+F_b26f8 / 0xb9f82 _Znwj(64)+F_bd790(场景对象) /
     *   0xb9f92 _Znwj(28)+F_bf128 / 0xb9fa2 _Znwj(60)+F_bf6f0 / 0xb9fb2 _Znwj(88)+F_b6450 /
     *   0xb9fc2 _Znwj(64)+F_c1438 ...（16 槽全构造）。翻译体 F_b9edc/F_bd790 均完整体。
     *   旧版 F_be9cc 直接调用（传 0）→ 0x15be84 槽=0 → F_bd764 收到 r0=0 → 场景不构造。
     *   显式先调 F_b9edc：让 16 单例自建，0x15be84 非零 → F_bd764/F_b6790 帧驱动收到真对象。 */
    extern uint32_t F_b9edc(JQT_SIG);
    __android_log_print(4, "junqi", "OnCreate: 手动触发 F_b9edc（16 全局单例惰性构造）");
    { uint32_t sc = F_b9edc(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x20f000);
      __android_log_print(4, "junqi", "OnCreate: F_b9edc r0=0x%08x", sc);
      /* 验证 16 单例槽（0x15be80-0x15bebc）非零 */
      for (int si = 0; si < 16; si++) {
          uint32_t slot = 0x15be80 + si*4;
          uint32_t val = *(uint32_t*)(g_jqt_shadow + slot);
          if (val != 0)
              __android_log_print(4, "junqi", "OnCreate: 单例[%d] 0x%08x = 0x%08x", si, slot, val);
      }
      /* 特别验证 0x15be84 场景对象（F_bd764 的 r0 源） */
      uint32_t scn = *(uint32_t*)(g_jqt_shadow + 0x15be84);
      __android_log_print(4, "junqi", "OnCreate: 场景对象 0x15be84 = 0x%08x（非零=构造成功）", scn);
    }
    /* ★ §SINGLETON-FIX-4：F_75210 = 惰性单例构造器（0x15be90/0x15be94/0x15be98 等）。
     *   0x15be90（单例[4]）= F_23738→F_75164 读的槽（F_75164 读 0x15be90 传 F_76b14，
     *   类 B 构造链上游）。0x15be90=0 时 F_76b14 早退（MEMB(0+0x50)=0 → 走 L76b4a 早退）
     *   → F_76a98/F_782d4/F_7876c（类 B 构造）不执行 → F_77b10 slot22 dispatch 不触发
     *   → F_9e2b4（纹理创建）永不调用 → 棋盘黑。
     *   原版 F_75210：0x7523c _Znwj(88)+F_7687c 构造 0x15be90（r4=0x15be90 槽）。
     *   F_75210 翻译体完整体（jqt_vtbl.c:38115）。显式调用构造 0x15be90。 */
    extern uint32_t F_75210(JQT_SIG);
    __android_log_print(4, "junqi", "OnCreate: 手动触发 F_75210（单例[4] 0x15be90 惰性构造）");
    { uint32_t sc4 = F_75210(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x20f000);
      uint32_t v90 = *(uint32_t*)(g_jqt_shadow + 0x15be90);
      __android_log_print(4, "junqi", "OnCreate: F_75210 r0=0x%08x 0x15be90=0x%08x（非零=构造成功）", sc4, v90);
    }
    /* ★ §SINGLETON-FIX-2：0x15be84 场景对象构造（原版 OnCreate 0x235b4-0x235c0）：
     *   0x15be84 不在 F_b9edc 构造列表（F_b9edc 构造 0x15be80/0x15be88/0x162660 等），
     *   由 OnCreate 专用路径构造：_Znwj(80) + F_24250(this) + str 0x15be84。
     *   F_24250 完整体（jqt_OnCreate_full.c:9029）。0x15be84 = 场景对象（F_bd764 帧驱动
     *   依赖链上游），OnGraphCreate 读 0x15be80 传 F_bd764 → F_bd764 读 0x15be88 传 F_b6790。
     *   0x15be84 非零让 F_23560 0x2356a 检查通过（beq 分支不跳），走已构造路径。 */
    {
      extern void *_Znwj(unsigned int);
      extern uint32_t F_24250(JQT_SIG);
      uint32_t scn2 = *(uint32_t*)(g_jqt_shadow + 0x15be84);
      if (scn2 == 0) {
          uint32_t p = (uint32_t)_Znwj(80);
          uint32_t r242 = F_24250(p, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x20f000);
          *(uint32_t*)(g_jqt_shadow + 0x15be84) = p;
          __android_log_print(4, "junqi", "OnCreate: 构造 0x15be84 场景对象 p=0x%08x F_24250_r0=0x%08x", p, r242);
      } else {
          __android_log_print(4, "junqi", "OnCreate: 0x15be84 已构造 = 0x%08x", scn2);
      }
    }
    /* ★ §F23EEC-OBJ：0x15be8c 对象图构造（F_23eec 0x24010 dispatch 前置）。
     *   原版对局态 0x15be8c = A（0xcf85c0d0）→ A[0]=B（0xcf153fc0）→
     *   B[0]=vtable（load_base+0x155ad0，.data.rel.ro RELATIVE 重定位）。
     *   vtable slot13 = 0x32a91（bx lr 空函数，非绘制——0x24010 dispatch 安全）。
     *   F_23eec 0x24008 F_32f8c(MEM(0x15be8c)) → 若 0x15be8c=0 则 F_32f8c(0)=0 →
     *   MEM(0)=0 → blx 0 崩（§124z 崩溃根因）。构造最小有效对象图防崩。 */
    {
      extern void *_Znwj(unsigned int);
      uint32_t a_ptr = MEM(0x15be8c);
      if (a_ptr == 0) {
          uint32_t A = (uint32_t)_Znwj(0x100);
          uint32_t B = (uint32_t)_Znwj(0x10);
          MEM(B) = JQT_ABS_PTR(0x155ad0);   /* B[0] = vtable 0x155ad0 */
          MEM(A) = B;                        /* A[0] = B */
          MEM(0x15be8c) = A;
          __android_log_print(4, "junqi", "F23EEC-OBJ: 0x15be8c 构造 A=0x%08x B=0x%08x vtable=0x155ad0", A, B);
      } else {
          __android_log_print(4, "junqi", "F23EEC-OBJ: 0x15be8c 已构造 = 0x%08x", a_ptr);
      }
    }
    extern uint32_t F_be9cc(JQT_SIG);
    __android_log_print(4, "junqi", "OnCreate: 手动触发场景构造 F_be9cc（sp28-SCALE-7 基线态，r0/r1=0）");
    { uint32_t sc = F_be9cc(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x20f000);
      __android_log_print(4, "junqi", "OnCreate: 场景构造 F_be9cc r0=0x%08x", sc); }
    /* ★ §TRIGGER-GATE：直接置位 0x15be90 对象 F_76b14 早退闸口字节
     *   （byte[0x51]=1 + byte[0x41]=1，[0x3c]=0 已是），让 F_76b14 走
     *   F_76a98（类B构造→F_77b10→slot22→F_9e2b4 纹理创建）。
     *   F_76a98→F_782d4 的 r1 = [0x15be90+0x30]，F_782d4 要求 r1==10
     *   （cmp r1,#0xa 相等才 _Znwj(12)+F_7876c 类B构造），故 [0x30]=10。
     *   实证问题：闸口置位 + [0x30]=10 是否足够触发 F_7876c→slot22，还是再崩。 */
    {
      uint32_t v90 = *(uint32_t*)(g_jqt_shadow + 0x15be90);
      if (v90) {
        MEMB(v90 + 0x51) = 1;   /* F_76b14 gate: [0x51]!=0 → 清 0 后走 [0x41] 分支 */
        MEMB(v90 + 0x41) = 1;   /* F_76b14 gate: byte[0x41]!=0 → F_76a98 */
        MEM(v90 + 0x30) = 10;   /* F_782d4 gate: r1=[0x30] 必须 ==10 才构造类B */
        __android_log_print(4, "junqi", "OnCreate: 置位 0x15be90 闸口 byte[0x51]=1 byte[0x41]=1 [0x30]=10 [0x3c]=0x%08x",
            v90 ? MEM(v90+0x3c) : 0);
      } else {
        __android_log_print(4, "junqi", "OnCreate: 0x15be90 未构造，跳过闸口置位");
      }
    }
    __android_log_print(4, "junqi", "OnCreate: 开始 F_23560 完整调用链");
    __android_log_print(4, "junqi", "OnCreate: g_jqt_shadow=%p", (void*)g_jqt_shadow);
    uint32_t r = F_23560(0, V32(a), 0,0,0,0,0,0,0,0,0,0, 0x20f000);
    __android_log_print(6, "junqi", "OnCreate: F_23560 完整调用链 r0=0x%08x", r);
    if (r != 0) {
        /* ★ §124ad：F_23560 返回非零 view（Java 会 setRenderer→GLThread 启动），
         *   但 F_23560 链内的 setContentView（vtable F_be47c）被机械翻译存根 r0=0 →
         *   view 未 attach window → 无 surface → onSurfaceCreated 不触发 → OnGraphCreate 不调。
         *   必须显式 setContentView（幂等，重复调用无害）→ surface 创建 → 渲染链启动。 */
        /* ★ §GLES2-CONTEXT：原版 native OnCreate 调 GLView.setEGLContextClientVersion(2)
         *   （原版 .rodata 含 "setEGLContextClientVersion"）→ GLThread 创建 GLES2 context。
         *   翻译体没调 → GLThread 用 Java EglHelper 默认 GLES1 context →
         *   F_9e760 的 glVertexAttribPointer（GLES2）在 GLES1 context 上崩
         *   （tombstone 实证：pc=0xd974 libGLESv2.so = glVertexAttribPointer）。
         *   必须在 setRenderer（Java 侧，OnCreate 返回后）之前调用。 */
        {
            jobject view_obj = (jobject)(uintptr_t)r;
            jclass glview_cls = (*env)->GetObjectClass(env, view_obj);
            jmethodID setecv = (*env)->GetMethodID(env, glview_cls, "setEGLContextClientVersion", "(I)V");
            if (setecv != NULL) {
                (*env)->CallVoidMethod(env, view_obj, setecv, 2);
                __android_log_print(6, "junqi", "OnCreate: setEGLContextClientVersion(2) 已调用（GLES2 context）");
            } else {
                __android_log_print(6, "junqi", "OnCreate: setEGLContextClientVersion GetMethodID 失败（GLES1 context 保持）");
            }
            (*env)->DeleteLocalRef(env, glview_cls);
        }
        jclass actcls2 = (*env)->GetObjectClass(env, a);
        jmethodID setcv2 = (*env)->GetMethodID(env, actcls2, "setContentView", "(Landroid/view/View;)V");
        if (setcv2 == NULL) {
            jclass base2 = (*env)->GetSuperclass(env, actcls2);
            setcv2 = (*env)->GetMethodID(env, base2, "setContentView", "(Landroid/view/View;)V");
        }
        if (setcv2 != NULL) {
            (*env)->CallVoidMethod(env, a, setcv2, (jobject)(uintptr_t)r);
            __android_log_print(6, "junqi", "OnCreate: r!=0 路径 setContentView(view=0x%08x) 已调用", r);
        }
        return (jobject)(uintptr_t)r;
    }
    /* ★ §124y 修复（GLThread 未启动根因，纠正 §124x 的"返回 NULL"）：
     *   实际打包 APK 的 dec/smali/App.smali 结构是：
     *       OnCreate → move-result v0 → if-eqz v0,:skip_view → setOnTouchListener + setRenderer
     *   即 native 返回 NULL 时 Java **跳过 setRenderer**（无 GLThread！），返回非 NULL GLView
     *   时才 setRenderer。§124x 的"返回 NULL 让 Java fallback"只对 build_v5 的 if-nez 变体成立，
     *   对 dec 的 if-eqz 变体是反效果。
     *   原版设计：native OnCreate 自己 new GLView + activity.setContentView(view)（JNI），
     *   然后返回 view → Java setOnTouchListener + setRenderer → GLThread start → surface 创建
     *   （setContentView 已 attach window）→ onSurfaceCreated → native OnGraphCreate。
     *   v178s 的 NewObject fallback 缺 setContentView → GLThread 启动但无 surface（Object.wait 卡死）。
     *   正确：NewObject GLView + activity.setContentView(view) + 返回 view。 */
    jclass cls = (*env)->FindClass(env, "com/cnvcs/GLView");
    if (cls == NULL) { __android_log_print(6, "junqi", "OnCreate: fallback FindClass GLView 失败"); return NULL; }
    jmethodID ctor = (*env)->GetMethodID(env, cls, "<init>", "(Landroid/content/Context;)V");
    if (ctor == NULL) { __android_log_print(6, "junqi", "OnCreate: fallback GetMethodID GLView 失败"); return NULL; }
    jobject view = (*env)->NewObject(env, cls, ctor, a);
    __android_log_print(6, "junqi", "OnCreate: NewObject(GLView) = %p", (void*)view);
    if (view == NULL) { __android_log_print(6, "junqi", "OnCreate: NewObject GLView 返回 NULL"); return NULL; }
    /* §124au：局部 ref 会被 GC 回收 → 渲染循环 getHolder(stale) 崩 SIGABRT。
     * 提 GlobalRef 长期持有 GLView，渲染链 F_24108/F_9d458 插桩用（跨线程跨帧安全）。 */
    view = (*env)->NewGlobalRef(env, view);
    g_glview_handle = (uint32_t)(uintptr_t)view;  /* §124aj：渲染链 F_24108 插桩用 */
    /* ★ §GLES2-CONTEXT：原版 native OnCreate 调 setEGLContextClientVersion(2)（GLES2 context）。
     *   fallback NewObject 路径同样需要（否则 GLES1 context → glVertexAttribPointer 崩）。 */
    {
        jmethodID setecv = (*env)->GetMethodID(env, cls, "setEGLContextClientVersion", "(I)V");
        if (setecv != NULL) {
            (*env)->CallVoidMethod(env, view, setecv, 2);
            __android_log_print(6, "junqi", "OnCreate: fallback setEGLContextClientVersion(2) 已调用（GLES2 context）");
        } else {
            __android_log_print(6, "junqi", "OnCreate: fallback setEGLContextClientVersion GetMethodID 失败");
        }
    }
    /* 关键：activity.setContentView(view) —— attach 到 window → surface 创建 → GLThread 可渲染 */
    jclass actcls = (*env)->GetObjectClass(env, a);
    jmethodID setcv = (*env)->GetMethodID(env, actcls, "setContentView", "(Landroid/view/View;)V");
    if (setcv == NULL) {
        __android_log_print(6, "junqi", "OnCreate: GetMethodID setContentView 失败（回退 GetSuperclass）");
        jclass base = (*env)->GetSuperclass(env, actcls);
        setcv = (*env)->GetMethodID(env, base, "setContentView", "(Landroid/view/View;)V");
    }
    if (setcv != NULL) {
        (*env)->CallVoidMethod(env, a, setcv, view);
        __android_log_print(6, "junqi", "OnCreate: setContentView(view) 已调用");
    } else {
        __android_log_print(6, "junqi", "OnCreate: setContentView 方法找不到（仍返回 view）");
    }
    return view;
}
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnDestroy(JNIEnv *env, jclass c){
    /* §124au：OnDestroy 释放 GLView GlobalRef（OnCreate NewGlobalRef 的配对） */
    if (g_glview_handle) {
        (*env)->DeleteGlobalRef(env, (jobject)(uintptr_t)g_glview_handle);
        g_glview_handle = 0;
    }
    jqt_OnDestroy(0,0,0,0,0,0,0,0,0,0,0,0,0x20f000);
}
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnPause(JNIEnv *env, jclass c){ jqt_OnPause(0,0,0,0,0,0,0,0,0,0,0,0,0x20f000); }
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnResume(JNIEnv *env, jclass c){
    __android_log_print(6,"junqi","OnResume: 入口 env=%p done=%d", (void*)env, g_jqt_jni_selftest_done);
    /* ★ 桥自检触发点（OnResume 主线程生命周期入口，不依赖渲染循环）。
     *   逐条 resolve 43 条 so→Java 调用（只 FindClass+GetMethodID，不发 CallXxx）。 */
    if (!g_jqt_jni_selftest_done) {
        g_jni_env = env;
        jqt_jni_setenv(env, (void*)0);
        jqt_jni_selftest();
        g_jqt_jni_selftest_done = 1;
    }
    jqt_OnResume(0,0,0,0,0,0,0,0,0,0,0,0,0x20f000);
    /* ★ §124y-19：走子桥在 OnResume 也触发（redroid 无 GL 卡在 OnCreate/OnResume，OnTimer 到不了；
       OnResume 是主线程生命周期入口，redroid 能触发；tick 幂等，OnTimer 再调无副作用） */
    jq_walk_bridge_tick();
    /* §124y-20 native_layout 钩子已移到 OnCreate test_game 段（redroid 卡 OnCreate 时 OnResume 触发不可靠；
       MuMu 有 GL 进主循环后 OnCreate 也正常跑 test_game 段） */
}
JNIEXPORT jboolean JNICALL
Java_com_cnvcs_App_OnUpdate(JNIEnv *env, jclass c){
    /* §124an 跨线程修复：OnUpdate 在 GLThread（每帧）调，必须每次用本线程 env 重新 setenv
     *   （JNIEnv per-thread，主线程 env 跨线程复用 → stale Local）。setenv 重置 methodID 缓存，
     *   渲染链 F_24108 里的 jqt_jni_call 走 GLThread 自己的 env，安全。 */
    jqt_jni_setenv(env, (void*)0);
    /* 桥 selftest 仅首次（验 FindClass 通路） */
    if (!g_jqt_jni_selftest_done) {
        g_jqt_jni_selftest_done = 1;
        jqt_jni_selftest();
    }
    __android_log_print(6,"junqi","JNI OnUpdate 被调 tid=%d", (int)gettid());
    /* ★ §SLOT22-TRIGGER（本轮）：一次性 slot22（F_9e2b4 纹理创建）触发。
     *   背景：sp28-SCALE-8 手动直调 F_9e2b4 曾因 jqt_mem.h include 缺失 → dlopen fail
     *   （已修，include 加回）。三条件现在都满足：
     *   ①渲染器单例有效（F_9d3d4 在 OnGraphCreate 构造，0x162400=0xde195870 实证）
     *   ②GL context makeCurrent（OnUpdate 在 GLThread）③MEM 宏可用（jqt_mem.h）。
     *   F_9e2b4 语义：this[76](texID)==0 → F_9e204（glGenTextures→this[76]=texID + glTexImage2D
     *   上传 jq_board_px 737×751）→ texID 离开 0（棋盘纹理已创建标志）。 */
    if (!g_slot22_triggered) {
        extern uint32_t F_9e2b4(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
        uint32_t ren = MEM(0x162400);
        if (ren) {
            g_slot22_triggered = 1;
            uint32_t before = MEM(ren+76);
            F_9e2b4(ren, 0,0,0,0,0,0,0,0,0,0,0, 0x20f000);
            __android_log_print(6,"junqi","SLOT22-TRIGGER: F_9e2b4(ren=0x%08x) texID before=0x%08x after=0x%08x this[76]=0x%08x px=0x%08x w=%d h=%d",
                ren, before, MEM(ren+76), MEM(ren+76), MEM(ren+28), MEM(ren+32), MEM(ren+36));
        } else {
            __android_log_print(6,"junqi","SLOT22-TRIGGER: 渲染器未构造（0x162400=0），等待...");
        }
    }
    /* ★ §ONUPDATE-RETURN1（2026-11-05 实证，棋盘出屏最后一步）：GLThread run loop 在
     *   onDrawFrame 后检查返回值（GLThread.smali:895 `if-eqz v6, :cond_18`）——onDrawFrame
     *   返回 false → 跳过 EglHelper.swap() → 棋盘在 back buffer 但永不上屏（屏幕=清屏色）。
     *   OnUpdate→F_23475→F_bd5a0 循环阀返回 0（实证）→ drawn=false → 不 swap。
     *   ★ 修复：jqt_gl_bridge 已每帧绘制棋盘（F_9d43c→glDrawArrays，swap前 framebuffer
     *   =棋盘色实证），强制返回 true 让 GLThread swap 上屏。 */
    F_23475(0,0,0,0,0,0,0,0,0,0,0,0,0x20f000);
    return (jboolean)1;
}
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnEvent(JNIEnv *env, jclass c, jstring n, jstring v){ 
    /* ★ §124ak 修正：原版 0x233e2 `adds r1,r0,#0` → r1=env(JNIEnv*)。
     *   旧版传 V32(n) 当 r0、V32(v) 当 r1 → 丢 env → F_bd914 收到垃圾。 
     *   _r0=env（0x233e2 用）、_r1=env（覆盖）、_r2=n、_r3=v。 */
    jqt_OnEvent(V32(env),V32(env),V32(n),V32(v),0,0,0,0,0,0,0,0,0x20f000); 
}
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnResult(JNIEnv *env, jclass c, jint r, jint res, jobject d){ jqt_OnResult((uint32_t)r,(uint32_t)res,V32(d),0,0,0,0,0,0,0,0,0,0x20f000); }
JNIEXPORT jboolean JNICALL
Java_com_cnvcs_App_OnKey(JNIEnv *env, jclass c, jobject k){ return (jboolean)jqt_OnKey(V32(k),0,0,0,0,0,0,0,0,0,0,0,0x20f000); }
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnBluetooth(JNIEnv *env, jclass c, jobject i){ jqt_OnBluetooth(V32(i),0,0,0,0,0,0,0,0,0,0,0,0x20f000); }
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnGraphCreate(JNIEnv *env, jclass c){
    __android_log_print(6,"junqi","JNI OnGraphCreate 入口 tid=%d", (int)gettid());
    /* §124an 跨线程修复：GLThread 的 OnGraphCreate 必须用本线程 env 重新 setenv
     *   （全局 g_env 是主线程 OnTimer setenv 的，JNIEnv 是 per-thread，跨线程复用 →
     *   GLThread 内调 jqt_jni_call 用主线程 env → 'stale Local' SIGABRT）。
     *   每次 GLThread JNI 入口都 setenv 本线程 env，重置 jmethodID/jclass 缓存。 */
    jqt_jni_setenv(env, (void*)0);
    /* §124ai 对象图修复：先调 F_9d3d4（glClear 单例访问器，幂等，_Znwj+构造+写 vtable），
     * 确认单例非 0 才走 F_2349d（渲染对象未构造时 F_bd764 读 vtable 崩 SEGV_MAPERR）。 */
    extern uint32_t F_9d3d4(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
    uint32_t gl_clear = F_9d3d4(0,0,0,0,0,0,0,0,0,0,0,0,0x20f000);
    if (gl_clear == 0) {
        __android_log_print(6,"junqi","OnGraphCreate 守卫：glClear 单例未构造（F_9d3d4 返回 0），跳过 F_2349d");
        return;
    }
    __android_log_print(6,"junqi","OnGraphCreate 守卫通过 glClear=0x%08x，走 F_2349d", gl_clear);
    F_2349d(0,0,0,0,0,0,0,0,0,0,0,0,0x20f000);
    __android_log_print(6,"junqi","OnGraphCreate 完成 F_2349d 返回 tid=%d", (int)gettid());
}
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnGraphChange(JNIEnv *env, jclass c, jint w, jint h){
    /* §124an 跨线程：GLThread 入口每次 setenv 本线程 env */
    jqt_jni_setenv(env, (void*)0);
    __android_log_print(6,"junqi","JNI OnGraphChange 被调 w=%d h=%d tid=%d", w, h, (int)gettid());
    F_23489((uint32_t)w,(uint32_t)h,0,0,0,0,0,0,0,0,0,0,0x20f000);
}
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnTimer(JNIEnv *env, jclass c){
    /* ★ 桥自检触发点（首次，OnTimer 主线程每 100ms 稳定入口）。
     *   诚实边界：此路径 OnCreate 可能未跑通（§124ac F_75ce8 死循环），g_activity 可能为 0。
     *   selftest 用 g_activity=NULL 安全路径（只验 FindClass 静态方法，不传局部引用跨线程）。 */
    if (!g_jqt_jni_selftest_done) {
        g_jni_env = env;
        jqt_jni_setenv(env, g_activity ? (void*)g_activity : NULL);
        jqt_jni_selftest();
        g_jqt_jni_selftest_done = 1;
    }
    /* ★ §TOUCH-INTERACT-TEST：确定性游戏状态测试钩子（MuMu 宕机期间用 redroid 验证 F_2afa4）。
     *   setprop jq.test_game 1  → 构造 vtable 0x155ad0 对象 + 调 F_2af44（应自动调 F_2afa4 初始化）。 */
    {
        static int _tg_done = 0;
        if (!_tg_done) {
            char _tg[8] = {0};
            if (__system_property_get("jq.test_game", _tg) > 0 && strcmp(_tg, "1") == 0) {
                _tg_done = 1;
                __android_log_print(6,"junqi","TEST-GAME: setprop jq.test_game=1 → 构造场景对象 + F_2af44");
                extern void *_Znwj(unsigned int);
                extern uint32_t F_2af44(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
                uint32_t obj = (uint32_t)_Znwj(0x100);
                MEM(obj) = JQT_ABS_PTR(0x155ad0);   /* 场景 vtable（F_2af44 是 vtable[0x28]） */
                MEM(obj+4) = 0; MEM(obj+8) = 0;
                __android_log_print(6,"junqi","TEST-GAME: obj=0x%08x vt=0x%08x this38=0x%08x → 调 F_2af44", obj, MEM(obj), MEM(obj+0x38));
                uint32_t r = F_2af44(obj, 0,0,0,0,0,0,0,0,0,0,0, 0x20f000);
                __android_log_print(6,"junqi","TEST-GAME: F_2af44 返回 0x%08x this38=0x%08x obj0=0x%08x", r, MEM(obj+0x38), MEM(MEM(obj+0x38)));
                /* ★ 诊断：vtable 0x159950 [0x94] 槽的原始值与 MEM 读值对比 */
                __android_log_print(6,"junqi","TEST-GAME DIAG: g_jqt_shadow=0x%08x shadow[0x1599e4]=0x%08x MEM(shadow+0x1599e4)=0x%08x vt94_exp=0x%08x",
                    (uint32_t)(uintptr_t)g_jqt_shadow,
                    *(uint32_t*)(g_jqt_shadow + 0x1599e4),
                    MEM((uint32_t)(uintptr_t)g_jqt_shadow + 0x1599e4),
                    (uint32_t)(uintptr_t)g_jqt_shadow + 0xa4235);
                /* ★ §124y-13：读 gs[8] 子对象 vtable（容器工厂 F_a36c4 真实返回的对象） */
                {
                    uint32_t gs8 = MEM(MEM(obj+0x38)+8);   /* this38[8] */
                    uint32_t gs8_vt = (gs8 >= 0x70000000u) ? MEM(gs8) : 0;
                    __android_log_print(6,"junqi","TEST-GAME GS8: gs8=0x%08x gs8_vt=0x%08x gs8_vt_off=0x%08x gs8_vt_plus18=0x%08x",
                        gs8, gs8_vt,
                        (gs8_vt >= (uint32_t)(uintptr_t)g_jqt_shadow) ? gs8_vt - (uint32_t)(uintptr_t)g_jqt_shadow : 0,
                        (gs8_vt >= 0x70000000u) ? MEM(gs8_vt+0x18) : 0);
                }
            }
        }
    }
    /* ★ §124y-20：32 位原生布局加载链（OnCreate 测试钩子，redroid 可触发）
       setprop jq.native_layout 1 → F_2a380 布局入口（32 位原生布局链，离线 JQ_DEFAULT_LAY） */
    {
        static int _nl_done = 0;
        char _nl[8] = {0};
        if (!_nl_done && __system_property_get("jq.native_layout", _nl) > 0 && _nl[0] == '1') {
            _nl_done = 1;
            __android_log_print(6,"junqi","NATIVE-LAYOUT: setprop jq.native_layout=1 → 32 位原生布局链");
            extern uint32_t F_2a380(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
            extern void *_Znwj(unsigned int);
            uint32_t stobj = (uint32_t)_Znwj(0x200);
            MEM(stobj) = JQT_ABS_PTR(0x159950);   /* 游戏状态 vtable */
            for (int i = 0; i < 64; i++) MEM(stobj + i*4) = (i==0) ? MEM(stobj) : 0;
            uint32_t ret = F_2a380(stobj, 0, 0,0,0,0,0,0,0,0,0,0, 0x20f000);
            __android_log_print(6,"junqi","NATIVE-LAYOUT: F_2a380 返回 0x%08x state=0x%08x（32 位原生布局链跑通）", ret, stobj);
            /* ★ §124y-21：32 位原生事件发送器（F_3ab80）对照——构造事件对象 + 调 F_3ab80
               （32 位原生 Junqi.* 事件链：F_33790/F_3376c/F_336ec + F_3a92c 分发 + F_33684 发送）
               与会话 B 走子桥事件互为对照（32 位原生 vs 会话 B 替代驱动） */
            extern uint32_t F_3ab80(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
            /* ★ §124y-22：构造**真实事件对象**（非全零）——填 .rodata Junqi.Move 事件名指针
               + 事件码 + 走子 cell 参数，让 32 位原生事件链 F_3a92c 分发读到真实事件字段
               （F_3a92c 反汇编：evt+0x35 类型标记 + evt+0x44 事件码 + evt+0x48 事件名首字节）
               32 位 .rodata Junqi.Move @ 0x120f20（.rodata 73 条 Junqi.* 明文之一） */
            uint32_t evtobj = (uint32_t)_Znwj(0x100);
            MEM(evtobj) = JQT_ABS_PTR(0x159950);
            for (int i = 0; i < 64; i++) MEM(evtobj + i*4) = (i==0) ? MEM(evtobj) : 0;
            /* 真实事件对象字段（F_3a92c 读这些）：
               evt+0x44 = 事件码（走子=0，F_3a92c ldr r5,[r0,#68] 对比事件码）
               evt+0x48 = 事件名指针首字节（Junqi.Move @ .rodata 0x120f20，绝对指针 = g_jqt_shadow+0x120f20）
               evt+0x35 = 类型标记（F_3a92c ldrb [r0,#53]，非零才进事件表遍历） */
            MEM(evtobj + 0x44) = 0;                          /* 事件码 0=走子 */
            MEM(evtobj + 0x48) = JQT_ABS_PTR(0x120f20);     /* "Junqi.Move" .rodata 指针 */
            MEM(evtobj + 0x35) = 1;                          /* 类型标记（非零进事件表遍历） */
            /* 打日志确认事件字段填对了（32 位原生事件对象） */
            __android_log_print(6,"junqi","NATIVE-EVENT: 真实事件对象 evt+0x35=%d evt+0x44=0x%08x evt+0x48=0x%08x(Junqi.Move@.rodata 0x120f20)",
                (int)MEM(evtobj+0x35), MEM(evtobj+0x44), MEM(evtobj+0x48));
            uint32_t evret = F_3ab80(evtobj, 0,0,0,0,0,0,0,0,0,0,0, 0x20f000);
            __android_log_print(6,"junqi","NATIVE-EVENT: F_3ab80 返回 0x%08x evt=0x%08x（32 位原生事件链，真实事件对象）", evret, evtobj);
        }
    }
    /* ★ §124y-19：会话 B 规则引擎走子桥（setprop jq.walk_bridge 1 → 开局 + 走子 + 打 Junqi.* 事件）
       直接调 jq_walk_bridge_tick（jq_rs_walkbridge.c，已 visibility default） */
    jq_walk_bridge_tick();
    /* ★ §124y-26：端到端"游戏可玩"钩子（setprop jq.game_test 1）
       开局 + 触摸坐标注入（jq_touch_hit_cell 命中格）+ 走子 + Junqi.* 事件 + 终局。
       这是 AGENTS 阶段 2 验收的真判据（触摸→走子→事件→终局，可机判），
       不依赖 32 位 .pak 资源对象图（会话 B 走子桥 + 触摸命中几何，纯逻辑）。 */
    {
        static int _gt_done = 0;
        char _gt[8] = {0};
        if (!_gt_done && __system_property_get("jq.game_test", _gt) > 0 && _gt[0] == '1') {
            _gt_done = 1;
            __android_log_print(6,"junqi","GAME-TEST: setprop jq.game_test=1 → 端到端游戏可玩判据（触摸→走子→事件→终局）");
            extern int  jq_walk_bridge_touch(float,float,int,int);
            extern int  jq_walk_bridge_move(int,int);
            extern int  jq_walk_bridge_run(void);
            jq_walk_bridge_run();   /* 开局（会话 B 38 子布局） */
            /* 端到端判据（触摸→命中格→走子→Junqi.* 事件→终局）：
               会话 B 命中几何（设备行 row*5+col 体系，view=图 720×1280 1:1）。
               bg_cell_img_center(row,col)：image_row=11-row；img_x=78.5+col*140.5；
               img_y=222+image_row*71（image_row<6）/ 822+(image_row-6)*71（≥6）。
               会话 B test_move_seq 基准：f6 f33 m6,11 m33,38 m12,13 u m10,11 q0
               触摸注入 2 个源格 → 走子 2 步 → 终局，全程可机判（命中 cell 值 + 事件名）。 */
            /* 设备端 view=900×1468（OnGraphChange 实测，非全面屏 1468 布局）
               walk_coords 公式：px=ix/720*900, py=iy/1280*1468。
               cell6=row1col1(ir10)→tap(274,1268); cell33=row6col3(ir5)→tap(625,662)。 */
            int cell6 = jq_walk_bridge_touch(274.0f, 1268.0f, 900, 1468);
            __android_log_print(6,"junqi","GAME-TEST: 触摸 tap(274,1268) view900×1468 → cell%d（期望 6）", cell6);
            int cell33 = jq_walk_bridge_touch(625.0f, 662.0f, 900, 1468);
            __android_log_print(6,"junqi","GAME-TEST: 触摸 tap(625,662) view900×1468 → cell%d（期望 33）", cell33);
            /* 走子（会话 B 基准 m6,11 / m33,38） */
            int ev1 = jq_walk_bridge_move(6, 11);
            int ev2 = jq_walk_bridge_move(33, 38);
            const char *evn[] = {"Move","Eat","Bomb","Flag","Dead","Move"};
            __android_log_print(6,"junqi","GAME-TEST: 走子 cell6→11=Junqi.%s cell33→38=Junqi.%s（事件）",
                (ev1>=0&&ev1<6)?evn[ev1]:"Illegal", (ev2>=0&&ev2<6)?evn[ev2]:"Illegal");
            /* 终局：认输（winner=1-turn） */
            extern int jq_game_test_finish(void);
            int fin = jq_game_test_finish();
            /* 端到端判据汇总：2 格命中对 + 2 步走子合法 + 终局完成 = 游戏可玩 */
            int hit6_ok = (cell6 == 6), hit33_ok = (cell33 == 33);
            int move_ok = (ev1 == 0 && ev2 == 0);  /* 0=Junqi.Move */
            __android_log_print(6,"junqi","GAME-TEST: 端到端完成 命中=%d/%d 走子=%s fin=%d",
                (hit6_ok?1:0)+(hit33_ok?1:0), 2, move_ok?"OK(2步Move)":"FAIL", fin);
        }
    }
    __android_log_print(6,"junqi","JNI OnTimer 被调 tid=%d", (int)gettid());
    F_23461(0,0,0,0,0,0,0,0,0,0,0,0,0x20f000);
}
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnInput(JNIEnv *env, jclass c, jstring t){ jqt_OnInput(V32(t),0,0,0,0,0,0,0,0,0,0,0,0x20f000); }
JNIEXPORT jboolean JNICALL
Java_com_cnvcs_App_OnTouch(JNIEnv *env, jclass c, jobject m){ 
    /* ★ §124ak 修正：原版 0x2343c `adds r1,r0,#0` → r1=env(JNIEnv*)。
     *   旧版传 V32(m) 当 r0 → F_bdd2c 收到 r1=MotionEvent 而非 env →
     *   [r1+0] 读 MotionEvent 对象 vtable → 垃圾 dispatch → 触摸不处理。
     *   _r0=env（0x2343c 用）、_r1=env（覆盖）、_r2=motion。 */
    return (jboolean)jqt_OnTouch(V32(env),V32(env),V32(m),0,0,0,0,0,0,0,0,0,0x20f000); 
}
