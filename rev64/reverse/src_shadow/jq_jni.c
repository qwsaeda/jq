/* jq_jni.c — JNIEnv 影子函数表 + 14 个 Java_com_cnvcs_App_On* 导出入口
 *
 * 原版 native 拿到的 JNIEnv* 是 32 位指针，按 (*env)[slot](env, ...) 调用。
 * 影子地址空间里：
 *   env 令牌 0x0c000000 → [0x0c000000] = 0x0c001000 函数表
 *   函数表每一格 = 0x0d000000 + slot*4（thunk 令牌）
 * thunk 收到 idx 与原始寄存器/影子栈，转成真实 JNI 调用；Java 对象用
 * 0x0e000000 起的伪句柄映射。
 */
#include "jq_shadow.h"
#include "jq_jni_slots.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>

#define ENV_TOKEN   0x0c000000u
#define ENV_TABLE   0x0c001000u
/* JNI_OnLoad 捕获的真 env 函数表（32 位下标 0..7 = 标准 JNI 函数表前 8 项，
   8..11 = JavaVM 方法槽 thunk 令牌）。MEM(ENV_TABLE+i*4) 直接读这里。 */

#define THUNK_BASE  0x0d000000u

/* ART JNIEnv 函数表（标准 jni.h 布局：前 3 保留槽，第 4 起为 API 函数指针）。
   仅用于 JNI_OnLoad 读取真实 env->functions；桩 VM 下保持 THUNK 占位。 */
struct JNIFunTab {
    void *r0, *r1, *r2;               /* 保留 */
    void *fns[200];                  /* GetVersion .. */
};
#define OBJ_BASE    0x0e000000u
#define MID_BASE    0x0e800000u
#define FID_BASE    0x0ea00000u

typedef union { int64_t j; double d; float f; int32_t i; void *l; } jval;

static void *g_real_env = NULL;
/* 真值表：JNI_OnLoad 从真 env 捕获，按 JNI 标准函数表下标存放。
   32 位翻译体读 ENV_TABLE[i] 时：i<8 → 函数表（下标 0=GetVersion），
   i=8/9/10/11 → JavaVM 对象表（Destroy/Attach/Detach/GetEnv 的 thunk 令牌）。 */
static uint64_t g_real_env_tab[12];
static int g_real_env_tab_n = 0;
/* 分歧闩锁：对象图分歧（垃圾句柄/NULL 对象 Call*）一旦检测到，后续所有
 * 真实 JNI 调用直接返回 0 —— 不把坏引用喂 ART（SIGABRT），也不做无效调用。 */
static int g_jni_diverged = 0;
/* 供信号处理（SIGABRT/SIGSEGV 兜底）置位：外部崩溃也标记分歧，后续 JNI 返回 0 */
void jq_jni_mark_diverged(void) { g_jni_diverged = 1; }

/* ---------------- 句柄表 ---------------- */
#define MAXH 65536
static void *g_obj[MAXH]; static int g_obj_n;
static void *g_mid[MAXH]; static char g_mid_sig[MAXH][96]; static int g_mid_n;
static void *g_fid[MAXH]; static char g_fid_sig[MAXH][96]; static int g_fid_n;

void jq_set_env(void *env) { g_real_env = env; }

static int obj_find(void *p) {
    for (int i = g_obj_n - 1; i >= 0; i--) if (g_obj[i] == p) return i;
    return -1;
}
uint32_t jq_jni_object(void *p) {
    if (!p) return 0;
    int i = obj_find(p);
    if (i < 0) {
        if (g_obj_n >= MAXH) { jq_log("[jq] JNI 对象句柄表满\n"); return 0; }
        i = g_obj_n; g_obj[i] = p; g_obj_n++;
    }
    return OBJ_BASE + (uint32_t)i * 4u;
}
uint32_t jq_jni_new_object(void *p) { return jq_jni_object(p); }
void *jq_jni_real(uint32_t h) {
    if (!h) return NULL;
    if (h < OBJ_BASE || h >= OBJ_BASE + (uint32_t)MAXH * 4u) {
        jq_log("[jq] 非法对象句柄 0x%x（分歧闩锁）\n", h);
        g_jni_diverged = 1;
        return NULL;
    }
    uint32_t i = (h - OBJ_BASE) / 4u;
    if (i < (uint32_t)g_obj_n) return g_obj[i];
    jq_log("[jq] 越界对象句柄 0x%x（分歧闩锁）\n", h);
    g_jni_diverged = 1;
    return NULL;
}

static uint32_t mid_new(void *id, const char *sig) {
    for (int i = g_mid_n - 1; i >= 0; i--)
        if (g_mid[i] == id && sig && strcmp(g_mid_sig[i], sig) == 0)
            return MID_BASE + (uint32_t)i * 4u;
    if (g_mid_n >= MAXH) return 0;
    int i = g_mid_n++;
    g_mid[i] = id;
    if (sig) { strncpy(g_mid_sig[i], sig, 95); g_mid_sig[i][95] = 0; }
    else g_mid_sig[i][0] = 0;
    return MID_BASE + (uint32_t)i * 4u;
}
static void *mid_real(uint32_t h) {
    if (h < MID_BASE || h >= MID_BASE + (uint32_t)MAXH * 4u) return NULL;
    uint32_t i = (h - MID_BASE) / 4u;
    return i < (uint32_t)g_mid_n ? g_mid[i] : NULL;
}
static const char *mid_sig(uint32_t h) {
    if (h < MID_BASE || h >= MID_BASE + (uint32_t)MAXH * 4u) return "";
    uint32_t i = (h - MID_BASE) / 4u;
    return i < (uint32_t)g_mid_n ? g_mid_sig[i] : "";
}
static uint32_t fid_new(void *id, const char *sig) {
    for (int i = g_fid_n - 1; i >= 0; i--)
        if (g_fid[i] == id && sig && strcmp(g_fid_sig[i], sig) == 0)
            return FID_BASE + (uint32_t)i * 4u;
    if (g_fid_n >= MAXH) return 0;
    int i = g_fid_n++;
    g_fid[i] = id;
    if (sig) { strncpy(g_fid_sig[i], sig, 95); g_fid_sig[i][95] = 0; }
    else g_fid_sig[i][0] = 0;
    return FID_BASE + (uint32_t)i * 4u;
}
static void *fid_real(uint32_t h) {
    if (h < FID_BASE || h >= FID_BASE + (uint32_t)MAXH * 4u) return NULL;
    uint32_t i = (h - FID_BASE) / 4u;
    return i < (uint32_t)g_fid_n ? g_fid[i] : NULL;
}
static const char *fid_sig(uint32_t h) {
    if (h < FID_BASE || h >= FID_BASE + (uint32_t)MAXH * 4u) return "";
    uint32_t i = (h - FID_BASE) / 4u;
    return i < (uint32_t)g_fid_n ? g_fid_sig[i] : "";
}

/* ---------------- 初始化 ---------------- */
void jq_jni_reset(void) {
    MEM(ENV_TOKEN) = ENV_TABLE;
    for (int i = 0; i < JQS_N; i++) MEM(ENV_TABLE + (uint32_t)i * 4u) = THUNK_BASE + (uint32_t)i * 4u;
    g_obj_n = g_mid_n = g_fid_n = 0;
}
uint32_t jq_jni_env(void) { return ENV_TOKEN; }

/* ---------------- 真实 JNI 调用 ---------------- */
#define JTAB ((void **)((void **)g_real_env)[0])
#define JF(slot) (JTAB[slot])

static float b2f(uint32_t x) { float f; memcpy(&f, &x, 4); return f; }
static uint32_t f2b(float f) { uint32_t x; memcpy(&x, &f, 4); return x; }
static double bits2d(uint32_t lo, uint32_t hi) {
    uint64_t u = ((uint64_t)hi << 32) | lo; double d; memcpy(&d, &u, 8); return d;
}
static uint64_t bits2u(uint32_t lo, uint32_t hi) { return ((uint64_t)hi << 32) | lo; }
static uint32_t lo32(uint64_t v) { return (uint32_t)v; }
static uint32_t hi32(uint64_t v) { return (uint32_t)(v >> 32); }

/* 按签名把影子参数转成宿主 jvalue[]。is_array=1 表示 src 是 jvalue 数组。 */
static int build_args(const char *sig, int is_array, uint32_t src, jval *out) {
    int n = 0;
    const char *p = strchr(sig, '(');
    if (!p) return 0;
    p++;
    while (*p && *p != ')') {
        char t = *p++;
        if (t == 'L' || t == '[') {
            while (*p && *p != ';') p++;
            if (*p == ';') p++;
            uint32_t h = is_array ? MEM(src + n * 8) : MEM(src + n * 4);
            out[n].l = jq_jni_real(h);
        } else if (t == 'J' || t == 'D') {
            uint32_t a, b;
            if (is_array) { a = MEM(src + n * 8); b = MEM(src + n * 8 + 4); }
            else { a = MEM(src); b = MEM(src + 4); }
            if (t == 'J') out[n].j = (int64_t)bits2u(a, b);
            else out[n].d = bits2d(a, b);
        } else {
            uint32_t v = is_array ? MEM(src + n * 8) : MEM(src + n * 4);
            if (t == 'F') out[n].f = b2f(v);
            else out[n].i = (int32_t)v;
        }
        n++;
    }
    return n;
}

/* 变参（V 系列）：r3 是 VA_LIST（ap 指针，指向可变参首址），按 MEM 顺序读 */
static int build_args_v(const char *sig, uint32_t r3, uint32_t sp, jval *out) {
    int n = 0;
    uint32_t regs[1] = {r3};
    int ri = 0;
    uint32_t cur = sp;
    const char *p = strchr(sig, '(');
    if (!p) return 0;
    p++;
    while (*p && *p != ')') {
        char t = *p++;
        if (t == 'L' || t == '[') {
            while (*p && *p != ';') p++;
            if (*p == ';') p++;
            uint32_t h = (ri < 1) ? regs[ri++] : (cur += 4, MEM(cur - 4));
            out[n].l = jq_jni_real(h);
        } else if (t == 'J' || t == 'D') {
            /* AAPCS：栈上 64 位实参 8 字节对齐 */
            if (ri >= 1 && (cur & 7)) cur += 4;
            uint32_t a, b;
            if (ri < 1) { a = regs[ri++]; b = (ri < 1) ? (cur += 4, MEM(cur - 4)) : (cur += 4, MEM(cur - 4)); }
            else { a = MEM(cur); b = MEM(cur + 4); cur += 8; }
            (void)b;
            if (t == 'J') out[n].j = (int64_t)bits2u(a, b);
            else out[n].d = bits2d(a, b);
        } else {
            uint32_t v = (ri < 1) ? regs[ri++] : (cur += 4, MEM(cur - 4));
            if (t == 'F') out[n].f = b2f(v);
            else out[n].i = (int32_t)v;
        }
        n++;
    }
    return n;
}

/* V 系列变参：r3 是 VA_LIST 指针（ap 首址），按 ap 后继地址顺序读 */
static int build_args_mem(const char *sig, uint32_t ap, jval *out) {
    int n = 0;
    const char *p = strchr(sig, '(');
    if (!p) return 0;
    p++;
    uint32_t cur = ap;
    while (*p && *p != ')') {
        char t = *p++;
        if (cur & 7) cur += 4;
        if (t == 'L' || t == '[') {
            while (*p && *p != ';') p++;
            if (*p == ';') p++;
            out[n].l = jq_jni_real(MEM(cur));
            cur += 4;
        } else if (t == 'J' || t == 'D') {
            uint32_t a = MEM(cur), b = MEM(cur + 4);
            if (t == 'J') out[n].j = (int64_t)bits2u(a, b);
            else out[n].d = bits2d(a, b);
            cur += 8;
        } else {
            uint32_t v = MEM(cur);
            if (t == 'F') out[n].f = b2f(v);
            else out[n].i = (int32_t)v;
            cur += 4;
        }
        n++;
    }
    return n;
}

/* 真实 JNI 只能接收宿主指针（jobject/jmethodID 高地址）。影子地址空间
 * [0,0x10000000) 内不可能是真实 JNI 对象 —— 对象图分歧会把影子句柄/垃圾值
 * 当对象传进来（如 0x149），屏蔽为 NULL 防真实 JNI 解引用崩；同时置分歧闩锁。 */
static void *san(void *p) {
    if ((uintptr_t)p < 0x10000000u) {
        if (p) g_jni_diverged = 1;
        return NULL;
    }
    return p;
}

/* 解析真实对象句柄；垃圾（置闩锁）→ 放弃本次 JNI 调用返回 0。
 * 用于 switch 里 `JF(idx)(..., jq_jni_real(rX), ...)` 同表达式调用：
 * jq_jni_real 置闩锁后必须立即返回，不能在同一表达式继续调 JF（否则
 * GetObjectClass(NULL) 等喂 ART → SIGABRT）。 */
#define JQ_RR(h) ({ void *_p = jq_jni_real(h); if (g_jni_diverged) return 0; _p; })

/* rt: 0 void, 1 object, 2 bool, 3 int, 4 long, 5 float, 6 double */
static uint32_t real_call(int slot, int rt, void *a1, void *a2, void *a3, jval *args) {
    typedef void *V;
    if (g_jni_diverged) {
        /* 对象图已分歧：不再调任何真实 JNI，按 0 返回（防 ART abort） */
        return 0;
    }
    if (!JF(slot)) {
        /* 真实 JNIEnv 表槽位为 null（槽号越界/该函数本 ART 无/索引错）→ 不崩 */
        jq_log("[jq] JNI 槽位 %d 为 null（rt=%d），按 0 返回\n", slot, rt);
        return 0;
    }
    a1 = san(a1); a2 = san(a2); a3 = san(a3);
    if (!a1 && rt && slot >= 34) {
        /* Call*Method 对象为 NULL（分歧）：不调真实 JNI，按 0 返回 */
        jq_log("[jq] JNI Call* slot=%d 对象 NULL（分歧闩锁），按 0 返回\n", slot);
        g_jni_diverged = 1;
        return 0;
    }
    switch (rt) {
    case 0: ((void (*)(void *, void *, void *, void *, jval *))JF(slot))(g_real_env, a1, a2, a3, args); return 0;
    case 1: return jq_jni_object(((V (*)(void *, void *, void *, void *, jval *))JF(slot))(g_real_env, a1, a2, a3, args));
    case 2: return (uint32_t)(((unsigned char (*)(void *, void *, void *, void *, jval *))JF(slot))(g_real_env, a1, a2, a3, args));
    case 3: return (uint32_t)((int32_t (*)(void *, void *, void *, void *, jval *))JF(slot))(g_real_env, a1, a2, a3, args);
    case 4: {
        int64_t v = ((int64_t (*)(void *, void *, void *, void *, jval *))JF(slot))(g_real_env, a1, a2, a3, args);
        g_jq_ret1 = hi32((uint64_t)v); g_jq_ret2 = 0; g_jq_ret3 = 0;
        return lo32((uint64_t)v);
    }
    case 5: return f2b(((float (*)(void *, void *, void *, void *, jval *))JF(slot))(g_real_env, a1, a2, a3, args));
    case 6: {
        double d = ((double (*)(void *, void *, void *, void *, jval *))JF(slot))(g_real_env, a1, a2, a3, args);
        uint64_t u; memcpy(&u, &d, 8);
        g_jq_ret1 = hi32(u); g_jq_ret2 = 0; g_jq_ret3 = 0;
        return lo32(u);
    }
    }
    return 0;
}

/* 调用类型表：(返回类型, A 槽位偏移基准) */
static const int g_call_rt[10] = {1, 2, 3, 3, 3, 3, 3, 4, 5, 6};
/* Object, Boolean, Byte, Char, Short, Int, Long, Float, Double */

/* 数组元素暂存表 */
typedef struct { uint32_t sh; void *host; int n, esz; void *arr; int mode; } jarr;
#define MAXARR 64
static jarr g_arr[MAXARR];
static int g_arr_n;

/* ---------------- JavaVM thunk ---------------- */
uint32_t jq_vm_thunk(uint32_t idx, JQ_ARGS) {
    (void)idx;
    /* GetEnv(JavaVM*, void **penv, jint)：*penv = 0x0c000000 即当前 JNIEnv */
    if (idx == 3) { MEM(MEM(r2)) = ENV_TOKEN; return 0x00000000; }
    /* AttachCurrentThread / AttachCurrentThreadAsDaemon */
    if (idx == 1 || idx == 4) { MEM(r2) = ENV_TOKEN; return 0x00000000; }
    /* DetachCurrentThread / DestroyJavaVM：无操作 */
    return 0x00000000;
}

/* ---------------- thunk ---------------- */
uint32_t jq_jni_thunk(uint32_t idx, JQ_ARGS) {
    if (!g_real_env) { jq_log("[jq] JNI 调用 #%u 但无真实 env\n", idx); return 0; }
    if (g_jni_diverged) {
        /* 对象图已分歧（前次调用置位闩锁）：本次 JNI 调用直接返回 0，
         * 不走到 switch 的直接 JF(idx) 调用（否则垃圾引用喂 ART → SIGABRT）。 */
        return 0;
    }
    /* Call*Method（实例）34..63 */
    if (idx >= JQS_CallObjectMethod && idx <= JQS_CallVoidMethodA) {
        int t = (idx - JQS_CallObjectMethod) / 3;
        int variant = (idx - JQS_CallObjectMethod) % 3;
        jval args[24]; int n = 0;
        const char *sig = mid_sig(r2);
        void *mid = mid_real(r2);
        void *obj = JQ_RR(r1);
        if (variant == 2) n = build_args(sig, 1, r3, args);
        else if (variant == 1) n = build_args_mem(sig, r3, args);
        else if (g_call_rt[t] != 0) { args[0].i = (int32_t)r3; n = 1; }
        (void)n;
        int slotA = JQS_CallObjectMethodA + t * 3;
        if (!obj) jq_log("[jq] Call* slot=%d 对象 NULL r1=0x%x sig=\"%s\"\n", slotA, r1, sig);
        uint32_t r = real_call(slotA, g_call_rt[t], obj, mid, NULL, args);
        jq_log("[jq] Call* slot=%d -> 0x%x sig=\"%s\"\n", slotA, r, sig);
        return r;
    }
    /* CallNonvirtual* 64..93 */
    if (idx >= JQS_CallNonvirtualObjectMethod && idx <= JQS_CallNonvirtualVoidMethodA) {
        int t = (idx - JQS_CallNonvirtualObjectMethod) / 3;
        int variant = (idx - JQS_CallNonvirtualObjectMethod) % 3;
        jval args[24];
        const char *sig = mid_sig(r3);
        void *mid = mid_real(r3);
        void *obj = JQ_RR(r1);
        void *cls = JQ_RR(r2);
        if (variant == 2) build_args(sig, 1, MEM(sp), args);
        else if (variant == 0) build_args_v(sig, MEM(sp), sp + 4, args);
        int slotA = JQS_CallNonvirtualObjectMethodA + t * 3;
        return real_call(slotA, g_call_rt[t], obj, cls, mid, args);
    }
    /* CallStatic* 114..141 */
    if (idx >= JQS_CallStaticObjectMethod && idx <= JQS_CallStaticVoidMethodA) {
        int t = (idx - JQS_CallStaticObjectMethod) / 3;
        int variant = (idx - JQS_CallStaticObjectMethod) % 3;
        jval args[24];
        const char *sig = mid_sig(r2);
        void *mid = mid_real(r2);
        void *cls = JQ_RR(r1);
        if (variant == 2) build_args(sig, 1, r3, args);
        else if (variant == 1) build_args_mem(sig, r3, args);
        int slotA = JQS_CallStaticObjectMethodA + t * 3;
        return real_call(slotA, g_call_rt[t], cls, mid, NULL, args);
    }
    /* JavaVM 接口：GetJavaVM(219) 后 vm 级调用走 VM thunk 令牌 */
    if (idx == JQS_GetJavaVM) {
        MEM(r1) = JQ_JAVAVM_BASE;   /* *out = JavaVM* */
        MEM(JQ_JAVAVM_BASE) = JQ_JAVAVM_TAB;
        MEM(JQ_JAVAVM_TAB + 8u * 2) = 0x0b801000u; /* DestroyJavaVM */
        MEM(JQ_JAVAVM_TAB + 8u * 3) = 0x0b801008u; /* AttachCurrentThread */
        MEM(JQ_JAVAVM_TAB + 8u * 4) = 0x0b801010u; /* DetachCurrentThread */
        MEM(JQ_JAVAVM_TAB + 8u * 5) = 0x0b801018u; /* GetEnv */
        MEM(JQ_JAVAVM_TAB + 8u * 6) = 0x0b801020u; /* AttachCurrentThreadAsDaemon */
        return 0;
    }
    switch (idx) {
    case JQS_GetVersion: return 0x00010006u;
    case JQS_FindClass: {
        const char *cn = (const char *)MEMP(r1);
        void *c = ((void *(*)(void *, const char *))JF(idx))(g_real_env, cn);
        jq_log("[jq] FindClass \"%s\" -> %p\n", cn ? cn : "(null)", c);
        return jq_jni_object(c);
    }
    case JQS_GetSuperclass: return jq_jni_object(((void *(*)(void *, void *))JF(idx))(g_real_env, JQ_RR(r1)));
    case JQS_IsAssignableFrom: return (uint32_t)((unsigned char (*)(void *, void *, void *))JF(idx))(g_real_env, JQ_RR(r1), JQ_RR(r2));
    case JQS_Throw: return (uint32_t)((int32_t (*)(void *, void *))JF(idx))(g_real_env, JQ_RR(r1));
    case JQS_ThrowNew: return (uint32_t)((int32_t (*)(void *, void *, const char *))JF(idx))(g_real_env, JQ_RR(r1), (const char *)MEMP(r2));
    case JQS_ExceptionOccurred: return jq_jni_object(((void *(*)(void *))JF(idx))(g_real_env));
    case JQS_ExceptionDescribe: ((void (*)(void *))JF(idx))(g_real_env); return 0;
    case JQS_ExceptionClear: ((void (*)(void *))JF(idx))(g_real_env); return 0;
    case JQS_ExceptionCheck: return (uint32_t)((unsigned char (*)(void *))JF(idx))(g_real_env);
    case JQS_PushLocalFrame: return (uint32_t)((int32_t (*)(void *, int))JF(idx))(g_real_env, (int)r1);
    case JQS_PopLocalFrame: return jq_jni_object(((void *(*)(void *, void *))JF(idx))(g_real_env, JQ_RR(r1)));
    case JQS_NewGlobalRef: return jq_jni_object(((void *(*)(void *, void *))JF(idx))(g_real_env, JQ_RR(r1)));
    case JQS_DeleteGlobalRef: ((void (*)(void *, void *))JF(idx))(g_real_env, JQ_RR(r1)); return 0;
    case JQS_DeleteLocalRef: return 0;
    case JQS_NewLocalRef: return jq_jni_object(((void *(*)(void *, void *))JF(idx))(g_real_env, JQ_RR(r1)));
    case JQS_IsSameObject: return (uint32_t)((unsigned char (*)(void *, void *, void *))JF(idx))(g_real_env, JQ_RR(r1), JQ_RR(r2));
    case JQS_AllocObject: return jq_jni_object(((void *(*)(void *, void *))JF(idx))(g_real_env, JQ_RR(r1)));
    case JQS_NewObjectA: {
        jval args[24];
        const char *sig = mid_sig(r2);
        build_args(sig, 1, r3, args);
        return jq_jni_object(((void *(*)(void *, void *, void *, jval *))JF(idx))(g_real_env, JQ_RR(r1), mid_real(r2), args));
    }
    case JQS_NewObjectV: {
        jval args[24];
        const char *sig = mid_sig(r2);
        build_args_mem(sig, r3, args);
        return jq_jni_object(((void *(*)(void *, void *, void *, jval *))JF(JQS_NewObjectA))(g_real_env, JQ_RR(r1), mid_real(r2), args));
    }
    case JQS_NewObject: {
        jval *args = (jval *)MEMP(r3);
        return jq_jni_object(((void *(*)(void *, void *, void *, jval *))JF(JQS_NewObjectA))(g_real_env, JQ_RR(r1), mid_real(r2), args));
    }
    case JQS_GetObjectClass: return jq_jni_object(((void *(*)(void *, void *))JF(idx))(g_real_env, JQ_RR(r1)));
    case JQS_IsInstanceOf: return (uint32_t)((unsigned char (*)(void *, void *, void *))JF(idx))(g_real_env, JQ_RR(r1), JQ_RR(r2));
    case JQS_GetMethodID: {
        void *id = ((void *(*)(void *, void *, const char *, const char *))JF(idx))(g_real_env, JQ_RR(r1), (const char *)MEMP(r2), (const char *)MEMP(r3));
        jq_log("[jq] GetMethodID %s%s -> %p\n", (const char *)MEMP(r2), (const char *)MEMP(r3), id);
        return mid_new(id, (const char *)MEMP(r3));
    }
    case JQS_GetStaticMethodID: {
        void *id = ((void *(*)(void *, void *, const char *, const char *))JF(idx))(g_real_env, JQ_RR(r1), (const char *)MEMP(r2), (const char *)MEMP(r3));
        return mid_new(id, (const char *)MEMP(r3));
    }
    case JQS_GetFieldID: {
        void *id = ((void *(*)(void *, void *, const char *, const char *))JF(idx))(g_real_env, JQ_RR(r1), (const char *)MEMP(r2), (const char *)MEMP(r3));
        return fid_new(id, (const char *)MEMP(r3));
    }
    case JQS_GetStaticFieldID: {
        void *id = ((void *(*)(void *, void *, const char *, const char *))JF(idx))(g_real_env, JQ_RR(r1), (const char *)MEMP(r2), (const char *)MEMP(r3));
        return fid_new(id, (const char *)MEMP(r3));
    }
    /* 字段访问 95..112 / 143..160 */
    #define GETFIELD(slot, ctype, conv) \
        case slot: return conv(((ctype (*)(void *, void *, void *))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2)))
    GETFIELD(JQS_GetObjectField, void *, jq_jni_object);
    GETFIELD(JQS_GetBooleanField, unsigned char, (uint32_t));
    GETFIELD(JQS_GetByteField, unsigned char, (uint32_t));
    GETFIELD(JQS_GetCharField, uint16_t, (uint32_t));
    GETFIELD(JQS_GetShortField, int16_t, (uint32_t)(int32_t));
    GETFIELD(JQS_GetIntField, int32_t, (uint32_t));
    GETFIELD(JQS_GetFloatField, float, f2b);
    GETFIELD(JQS_GetStaticObjectField, void *, jq_jni_object);
    GETFIELD(JQS_GetStaticBooleanField, unsigned char, (uint32_t));
    GETFIELD(JQS_GetStaticByteField, unsigned char, (uint32_t));
    GETFIELD(JQS_GetStaticCharField, uint16_t, (uint32_t));
    GETFIELD(JQS_GetStaticShortField, int16_t, (uint32_t)(int32_t));
    GETFIELD(JQS_GetStaticIntField, int32_t, (uint32_t));
    GETFIELD(JQS_GetStaticFloatField, float, f2b);
    #undef GETFIELD
    case JQS_GetLongField: {
        int64_t v = ((int64_t (*)(void *, void *, void *))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2));
        r1 = hi32((uint64_t)v); JQ_RET(lo32((uint64_t)v));
    }
    case JQS_GetDoubleField: {
        double d = ((double (*)(void *, void *, void *))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2));
        uint64_t u; memcpy(&u, &d, 8);
        r1 = hi32(u); JQ_RET(lo32(u));
    }
    case JQS_GetStaticLongField: {
        int64_t v = ((int64_t (*)(void *, void *, void *))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2));
        r1 = hi32((uint64_t)v); JQ_RET(lo32((uint64_t)v));
    }
    case JQS_GetStaticDoubleField: {
        double d = ((double (*)(void *, void *, void *))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2));
        uint64_t u; memcpy(&u, &d, 8);
        r1 = hi32(u); JQ_RET(lo32(u));
    }
    case JQS_SetObjectField:
        ((void (*)(void *, void *, void *, void *))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2), JQ_RR(r3)); return 0;
    case JQS_SetBooleanField: case JQS_SetByteField: case JQS_SetCharField:
    case JQS_SetShortField: case JQS_SetIntField:
        ((void (*)(void *, void *, void *, int32_t))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2), (int32_t)r3); return 0;
    case JQS_SetFloatField:
        ((void (*)(void *, void *, void *, float))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2), b2f(r3)); return 0;
    case JQS_SetLongField:
        ((void (*)(void *, void *, void *, int64_t))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2), (int64_t)bits2u(r3, MEM(sp))); return 0;
    case JQS_SetDoubleField:
        ((void (*)(void *, void *, void *, double))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2), bits2d(r3, MEM(sp))); return 0;
    case JQS_SetStaticObjectField:
        ((void (*)(void *, void *, void *, void *))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2), JQ_RR(r3)); return 0;
    case JQS_SetStaticBooleanField: case JQS_SetStaticByteField: case JQS_SetStaticCharField:
    case JQS_SetStaticShortField: case JQS_SetStaticIntField:
        ((void (*)(void *, void *, void *, int32_t))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2), (int32_t)r3); return 0;
    case JQS_SetStaticFloatField:
        ((void (*)(void *, void *, void *, float))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2), b2f(r3)); return 0;
    case JQS_SetStaticLongField:
        ((void (*)(void *, void *, void *, int64_t))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2), (int64_t)bits2u(r3, MEM(sp))); return 0;
    case JQS_SetStaticDoubleField:
        ((void (*)(void *, void *, void *, double))JF(idx))(g_real_env, JQ_RR(r1), fid_real(r2), bits2d(r3, MEM(sp))); return 0;

    case JQS_NewStringUTF: return jq_jni_object(((void *(*)(void *, const char *))JF(idx))(g_real_env, (const char *)MEMP(r1)));
    case JQS_GetStringUTFLength: return (uint32_t)((int32_t (*)(void *, void *))JF(idx))(g_real_env, JQ_RR(r1));
    case JQS_GetStringUTFChars: {
        unsigned char *s = (unsigned char *)((void *(*)(void *, void *, unsigned char *))JF(idx))(g_real_env, JQ_RR(r1), (unsigned char *)MEMP(r2));
        return s ? jq_push_string((const char *)s) : 0;
    }
    case JQS_ReleaseStringUTFChars: return 0;
    case JQS_GetStringLength: return (uint32_t)((int32_t (*)(void *, void *))JF(idx))(g_real_env, JQ_RR(r1));

    case JQS_GetArrayLength: return (uint32_t)((int32_t (*)(void *, void *))JF(idx))(g_real_env, JQ_RR(r1));
    case JQS_NewObjectArray: {
        void *a = ((void *(*)(void *, int, void *, void *))JF(idx))(g_real_env, (int)r1, JQ_RR(r2), JQ_RR(r3));
        return jq_jni_object(a);
    }
    case JQS_GetObjectArrayElement:
        return jq_jni_object(((void *(*)(void *, void *, int))JF(idx))(g_real_env, JQ_RR(r1), (int)r2));
    case JQS_SetObjectArrayElement:
        ((void (*)(void *, void *, int, void *))JF(idx))(g_real_env, JQ_RR(r1), (int)r2, JQ_RR(r3)); return 0;

    /* New<Type>Array 173..180 */
    case JQS_NewBooleanArray: case JQS_NewByteArray: case JQS_NewCharArray:
    case JQS_NewShortArray: case JQS_NewIntArray: case JQS_NewLongArray:
    case JQS_NewFloatArray: case JQS_NewDoubleArray:
        return jq_jni_object(((void *(*)(void *, int))JF(idx))(g_real_env, (int)r1));

    default: break;
    }
    /* Get<Type>ArrayElements 181..188 / Release 189..196 */
    if (idx >= JQS_GetBooleanArrayElements && idx <= JQS_GetDoubleArrayElements) {
        int t = idx - JQS_GetBooleanArrayElements;
        static const int esz[8] = {1, 1, 2, 2, 4, 8, 4, 8};
        void *arr = JQ_RR(r1);
        unsigned char isCopy = 0;
        void *p = ((void *(*)(void *, void *, unsigned char *))JF(idx))(g_real_env, arr, &isCopy);
        if (!p) return 0;
        int n = ((int32_t (*)(void *, void *))JF(JQS_GetArrayLength))(g_real_env, arr);
        uint32_t sh = jq_malloc((uint32_t)(n * esz[t]));
        memcpy(MEMP(sh), p, (size_t)(n * esz[t]));
        if (g_arr_n < MAXARR) {
            g_arr[g_arr_n].sh = sh; g_arr[g_arr_n].host = p; g_arr[g_arr_n].n = n;
            g_arr[g_arr_n].esz = esz[t]; g_arr[g_arr_n].arr = arr; g_arr[g_arr_n].mode = isCopy;
            g_arr[g_arr_n].mode = idx; g_arr_n++;
        }
        return sh;
    }
    if (idx >= JQS_ReleaseBooleanArrayElements && idx <= JQS_ReleaseDoubleArrayElements) {
        for (int i = 0; i < g_arr_n; i++) {
            if (g_arr[i].sh == r1) {
                int n = g_arr[i].n * g_arr[i].esz;
                void *p = ((void *(*)(void *, void *, void *, int))JF(idx))(g_real_env, g_arr[i].arr, g_arr[i].host, (int)r2);
                if (r2 == 0 && p) memcpy(jq_host(0) /*unused*/, p, 0); /* no-op */
                if (r2 != 2 /* JNI_COMMIT 只提交，不释放 */) jq_free(g_arr[i].sh);
                g_arr[i] = g_arr[--g_arr_n];
                break;
            }
        }
        return 0;
    }
    jq_log("[jq] 未实现 JNI 槽位 #%u\n", idx);
    return 0;
}

/* ---------------- 14 个 JNI 入口 ---------------- */
#define ENTRY_ADDR_OnEvent       0x233e0u
#define ENTRY_ADDR_OnInput       0x233f4u
#define ENTRY_ADDR_OnResult      0x23408u
#define ENTRY_ADDR_OnBluetooth   0x23424u
#define ENTRY_ADDR_OnTouch       0x23438u
#define ENTRY_ADDR_OnKey         0x2344cu
#define ENTRY_ADDR_OnTimer       0x23460u
#define ENTRY_ADDR_OnUpdate      0x23474u
#define ENTRY_ADDR_OnGraphChange 0x23488u
#define ENTRY_ADDR_OnGraphCreate 0x2349cu
#define ENTRY_ADDR_OnResume      0x234b0u
#define ENTRY_ADDR_OnPause       0x234c4u
#define ENTRY_ADDR_OnCreate      0x23560u
#define ENTRY_ADDR_OnDestroy     0x235d4u

typedef struct JNINativeInterface_ JNINativeInterface;
typedef const struct JNINativeInterface_ *JNIEnvP;
typedef void *jobject_p;
typedef void *jclass_p;

/* OnCreate 分段跟踪（定位 SIGSEGV 0xdead0000 发生在哪段） */
static int g_oc_seg = 0;
#define OC_SEG(n) do { if (g_oc_seg < (n)) { g_oc_seg = (n); jq_log("[jq] OnCreate seg=%d env=%p\n", (n), g_real_env); } } while(0)

static uint32_t call_entry(uint32_t addr, void *env, void *a1, void *a2, void *a3) {
    jq_set_env(env);
    uint32_t c = jq_jni_object(a1);
    uint32_t b = jq_jni_object(a2);
    uint32_t a = jq_jni_object(a3);
    return jq_call(addr, ENV_TOKEN, c, b, a,
                   0, 0, 0, 0, 0, 0, 0, 0, jq_shadow_sp());
}

/* fallback view：OnCreate 对象图分歧返回 nil 时，用真实 JNI 造一个最小
 * GLView（Java 不再 NPE）+ setContentView（模拟 native OnCreate 契约）。
 * 诚实边界：这是优雅降级（空 GL 面），非游戏板渲染，不包装成"能玩"。 */
static jobject_p make_fallback_view(JNIEnvP env, jobject_p act) {
    if (g_jni_diverged) g_jni_diverged = 0;  /* 恢复，允许真实 JNI 调用 */
    void *vc = ((void *(*)(void *, const char *))JF(JQS_FindClass))(g_real_env, "com/cnvcs/GLView");
    if (!vc) { jq_log("[jq] fallback: FindClass GLView 失败\n"); return NULL; }
    void *ctor = ((void *(*)(void *, void *, const char *, const char *))JF(JQS_GetMethodID))(g_real_env, vc, "<init>", "(Landroid/content/Context;)V");
    if (!ctor) { jq_log("[jq] fallback: GetMethodID <init> 失败\n"); return NULL; }
    jobject_p view;
    {
        jval cargs[1]; cargs[0].l = act;
        view = ((jobject_p (*)(void *, void *, void *, jval *))JF(JQS_NewObjectA))(g_real_env, vc, ctor, cargs);
    }
    jq_log("[jq] fallback: NewObject GLView -> %p\n", view);
    /* setContentView(view)：模拟 native OnCreate 的 decor view 安装 */
    void *avc = ((void *(*)(void *, const char *))JF(JQS_FindClass))(g_real_env, "android/app/Activity");
    if (avc) {
        void *scv = ((void *(*)(void *, void *, const char *, const char *))JF(JQS_GetMethodID))(g_real_env, avc, "setContentView", "(Landroid/view/View;)V");
        if (scv) {
            jval cargs[1]; cargs[0].l = view;
            ((void (*)(void *, void *, void *, jval *))JF(JQS_CallVoidMethodA))(g_real_env, act, scv, cargs);
        }
    }
    return view;
}

jobject_p Java_com_cnvcs_App_OnCreate(JNIEnvP env, jclass_p cls, jobject_p act) {
    OC_SEG(1);
    uint32_t r = call_entry(ENTRY_ADDR_OnCreate, (void *)env, cls, act, NULL);
    OC_SEG(2);
    /* §124zz 红线：深度超限/预算耗尽（longjmp 放弃 OnCreate 链）后 g_jni_diverged 已置位，
       此时对象图已分歧——再走 make_fallback_view 喂真 JNI 会触发 CheckJNI abort
       （22:24/22:26 实证）。已分歧 → 直接返回 NULL，Java 侧 App.smali 自带
       "OnCreate==null → new GLView(context)" 兜底（无需 native 造 view）。 */
    jobject_p v = (jobject_p)jq_jni_real(r);
    if (!v && !g_jni_diverged) {
        jq_log("[jq] OnCreate 返回 nil（未分歧），fallback GLView\n");
        v = make_fallback_view(env, act);
    } else if (!v) {
        jq_log("[jq] OnCreate 分歧（longjmp 放弃），返回 NULL → Java new GLView 兜底\n");
        g_jni_diverged = 0;   /* 清闩锁让后续 JNI 入口（OnUpdate/OnTimer）可继续 */
    }
    return v;
}
void Java_com_cnvcs_App_OnDestroy(JNIEnvP env, jclass_p cls) {
    call_entry(ENTRY_ADDR_OnDestroy, (void *)env, cls, NULL, NULL);
}
void Java_com_cnvcs_App_OnEvent(JNIEnvP env, jclass_p cls, jobject_p a, jobject_p b) {
    call_entry(ENTRY_ADDR_OnEvent, (void *)env, cls, a, b);
}
void Java_com_cnvcs_App_OnInput(JNIEnvP env, jclass_p cls, jobject_p a) {
    call_entry(ENTRY_ADDR_OnInput, (void *)env, cls, a, NULL);
}
void Java_com_cnvcs_App_OnResult(JNIEnvP env, jclass_p cls, int a, int b, jobject_p c) {
    jq_set_env((void *)env);
    jq_call(ENTRY_ADDR_OnResult, ENV_TOKEN, jq_jni_object(cls), (uint32_t)a, (uint32_t)b, 0, 0, 0, 0, 0, 0, 0, 0, jq_shadow_sp());
}
void Java_com_cnvcs_App_OnBluetooth(JNIEnvP env, jclass_p cls, jobject_p a) {
    call_entry(ENTRY_ADDR_OnBluetooth, (void *)env, cls, a, NULL);
}
long Java_com_cnvcs_App_OnTouch(JNIEnvP env, jclass_p cls, jobject_p a) {
    call_entry(ENTRY_ADDR_OnTouch, (void *)env, cls, a, NULL);
    return 0;
}
long Java_com_cnvcs_App_OnKey(JNIEnvP env, jclass_p cls, jobject_p a) {
    call_entry(ENTRY_ADDR_OnKey, (void *)env, cls, a, NULL);
    return 0;
}
void Java_com_cnvcs_App_OnTimer(JNIEnvP env, jclass_p cls) {
    call_entry(ENTRY_ADDR_OnTimer, (void *)env, cls, NULL, NULL);
}
long Java_com_cnvcs_App_OnUpdate(JNIEnvP env, jclass_p cls) {
    return (long)call_entry(ENTRY_ADDR_OnUpdate, (void *)env, cls, NULL, NULL);
}
void Java_com_cnvcs_App_OnGraphChange(JNIEnvP env, jclass_p cls, int w, int h) {
    jq_set_env((void *)env);
    jq_call(ENTRY_ADDR_OnGraphChange, ENV_TOKEN, jq_jni_object(cls), (uint32_t)w, (uint32_t)h, 0, 0, 0, 0, 0, 0, 0, 0, jq_shadow_sp());
}
void Java_com_cnvcs_App_OnGraphCreate(JNIEnvP env, jclass_p cls) {
    call_entry(ENTRY_ADDR_OnGraphCreate, (void *)env, cls, NULL, NULL);
}
void Java_com_cnvcs_App_OnResume(JNIEnvP env, jclass_p cls) {
    call_entry(ENTRY_ADDR_OnResume, (void *)env, cls, NULL, NULL);
}
void Java_com_cnvcs_App_OnPause(JNIEnvP env, jclass_p cls) {
    call_entry(ENTRY_ADDR_OnPause, (void *)env, cls, NULL, NULL);
}

/* JNI_OnLoad — Android 加载 .so 时由 ART 调用。必须初始化影子运行时
 * （g_jq_base + init_array 构造），否则 OnCreate 时 g_jq_base=0 → SIGSEGV
 * （x86_64 harness jq_test 显式调 jq_init()，Android 上只能靠这个入口）。 */
int JNI_OnLoad(void *vm, void *reserved) {
    (void)vm; (void)reserved;
    jq_init();
    /* §124zz 红线（实证）：MuMu 64 位转译层下读 vm->functions 表 → 0xdead 段 SIGSEGV。
       安全路径：g_real_env 保持 NULL；jq_jni_thunk 走 "无真实 env" 守卫返回 0，
       翻译体对象图分歧由安全阀（§113 深度/预算 + 信号兜底）兜住，不喂垃圾给 ART。
       代价：JNI 交互（GLView/setContentView/Log）全失效 → 主界面黑屏（同 §124z 边界）。
       出帧链闭环需 64 位宿主下 env 函数表安全捕获，属下一步架构改动。 */
    return 0x00010006;
}
