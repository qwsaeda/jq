/* jqt_jnibr.c — so→Java JNI 桥（JQJNI）实现
 *
 * 43 个 so→Java 调用统一走 jqt_jni_call(id, n_args, args)：
 *   - 从 jq_jni_calls[id] 取 cls/name/sig
 *   - FindClass + GetMethodID（缓存）
 *   - 按签名分发 CallXxxMethod（void/object/int/bool/long）
 *
 * 调用约定：
 *   - <init>：args[0]=this 不存在，参数全传，NewObject 返回
 *   - 静态（Toast.makeText/SiteView.create）：args 全传，CallStaticXxxMethod
 *   - 实例：args[0]=this 句柄，args[1..]=实际参，CallXxxMethod(this, mid, args[1..])
 */
#include "jqt_jnibr.h"
#include "jqt_jni_calls.h"
#include <jni.h>
#include <string.h>
#include <android/log.h>

#define LOGI(...) __android_log_print(4, "jqjni", __VA_ARGS__)
#define LOGE(...) __android_log_print(6, "jqjni", __VA_ARGS__)

static JNIEnv *g_env = NULL;
static JavaVM *g_vm = NULL;  /* ★ §124am：跨线程 env 修复——存 JavaVM，jqt_jni_call 用 GetEnv 拿当前线程 env */

/* ★ 调用计数 + 首个调用 logcat（验证端到端通路）：OnCreate/OnRender 渲染链
 *   在 JNI 边界调 jqt_jni_call 时计数 + 首条打 jqjni 标签。MuMu 重启 app 后
 *   logcat | grep jqjni 看是否真被调。0 次 = 桥未接线（需把 P0 接线提前到渲染链）。 */
static uint32_t g_call_count = 0;
static uint32_t g_last_id = 0xFFFFFFFF;

typedef struct {
    jmethodID mid;
    jclass cls;
    int ok;        /* GetMethodID 成功标志（selftest 逐条验证用） */
    int cls_fail;  /* FindClass 失败标志 */
} JqJniCache;
static JqJniCache g_cache[43];  /* = JQJNINUM */

int jqt_jni_call_ok(uint32_t jni_id){
    return (jni_id < 43) ? g_cache[jni_id].ok : 0;
}
int jqt_jni_call_cls_fail(uint32_t jni_id){
    return (jni_id < 43) ? g_cache[jni_id].cls_fail : 0;
}

void jqt_jni_setenv(void *env, void *activity){
    g_env = (JNIEnv *)env;
    if (g_env) (*g_env)->GetJavaVM(g_env, &g_vm);  /* ★ §124am：存 JavaVM 供 GetEnv */
    memset(g_cache, 0, sizeof(g_cache));
    /* ★ AUD-1000 B05-047（P1 R-138 诚实标注）：memset 清 ok/cls_fail 标志
       但 g_call_count/g_last_id **不清**——selftest 后 call_count 持续
       累加（无功能影响），但"首条打日志"条件 g_call_count<=60 跨
       selftest 偏移（selftest 消费掉前 60 条额度，真实帧日志窗口后移）。
       诊断口径：看 jqt_jni_call #N 序号时注意 N 含 selftest 段。
       清零属行为改动（改变日志窗口），R-138 只标注。 */
}

/* 从签名取返回值类型：sig 形如 "(II)V" 或 "()Landroid/view/ViewParent;"
 * 返回类型 = sig 最后 ')' 之后到 NUL 的字符串首字符 */
static char ret_type_of(const char *sig){
    const char *p = strrchr(sig, ')');
    return p ? p[1] : 0;
}

/* 静态方法判定（手工）：Toast.makeText / SiteView.create */
static int is_static_id(int id){
    JqJniCall *c = &jq_jni_calls[id];
    if (strcmp(c->name, "makeText") == 0 && strcmp(c->cls, "android/widget/Toast") == 0) return 1;
    if (strcmp(c->name, "create") == 0 && strcmp(c->cls, "com/cnvcs/SiteView") == 0) return 1;
    if (strcmp(c->name, "getDefaultAdapter") == 0) return 1;  /* BluetoothAdapter.getDefaultAdapter 静态 */
    return 0;
}

static jmethodID resolve(int id){
    if (g_cache[id].mid) return g_cache[id].mid;
    if (!g_env) return 0;
    JqJniCall *c = &jq_jni_calls[id];
    jclass cls = (*g_env)->FindClass(g_env, c->cls);
    if (!cls) {
        LOGE("FindClass %s 失败", c->cls);
        g_cache[id].cls_fail = 1;
        (*g_env)->ExceptionClear(g_env);
        return 0;
    }
    jmethodID mid = (*g_env)->GetMethodID(g_env, cls, c->name, c->sig);
    if (!mid) {
        LOGE("GetMethodID %s.%s %s 失败", c->cls, c->name, c->sig);
        (*g_env)->ExceptionClear(g_env);
        (*g_env)->DeleteLocalRef(g_env, cls);
        return 0;
    }
    g_cache[id].mid = mid;
    /* ★ AUD-1000 B05-048（P1 R-138 诚实标注）：cache.cls 提 GlobalRef
       旧版没做（存的是局部引用 cls）——§124am 修复后 jqt_jni_call 每次
       重取 FindClass 已**弃用** cache.cls（行 161 起 FindClass 直取），
       cache.cls 现为死字段（跨线程/跨帧后局部引用失效的隐患已不消费）。
       resolve_only 的 selftest 循环判 miss_cls 用 cls_fail 标志而非
       cache.cls，不受影响。删死字段/改 GlobalRef 属行为改动，R-138 只标注。 */
    g_cache[id].cls = cls;
    g_cache[id].ok = 1;
    return mid;
}

/* 只解析（FindClass+GetMethodID），不调 CallXxx——selftest 逐条验证专用。
 * 成功返回 1，失败返回 0。安全：不传 receiver，无局部引用失效风险。 */
int jqt_jni_resolve_only(uint32_t jni_id){
    if (jni_id >= 43) return 0;
    g_cache[jni_id].mid = 0;
    g_cache[jni_id].ok = 0;
    g_cache[jni_id].cls_fail = 0;
    jmethodID mid = resolve((int)jni_id);
    return mid ? 1 : 0;
}

/* 解析参数：按签名 arg_types（去掉首 ( 和尾 ) 及返回类型）逐参填 jvalue[]。
 *   基本类型 I/Z/J/F/D/S/B/C：args[k] 当 32/64 位值
 *   对象/数组 L...; / [L...;：args[k] 当 jobject 句柄（低 32 位）
 * 返回实参个数（不含 this）。 */
static int parse_args(const char *sig, uint32_t n_args, const uint32_t *args, jvalue out[12]){
    int k = 0, depth = 0, ni = 0;
    for (int i = 0; sig[i]; i++) {
        char ch = sig[i];
        if (ch == '(') { depth++; continue; }
        if (ch == ')') { depth--; if (depth == 0) break; continue; }
        if (depth == 0) {
            if (ch == 'I' || ch == 'Z' || ch == 'J' || ch == 'F' || ch == 'D' ||
                ch == 'S' || ch == 'B' || ch == 'C') {
                switch (ch) {
                    case 'I': case 'Z': case 'B': case 'C': case 'S':
                        out[ni].j = (jint)(int32_t)args[k]; break;
                    case 'J':
                        out[ni].j = (jlong)(int64_t)args[k]; break;
                    case 'F': { float f; memcpy(&f, &args[k], 4); out[ni].f = f; break; }
                    case 'D': { double d; memcpy(&d, &args[k], 4); out[ni].d = d; break; }
                }
                k++;
            } else if (ch == 'L' || ch == '[') {
                out[ni].l = (jobject)(uintptr_t)args[k];
                k++;
                while (sig[i] && sig[i] != ';') i++;  /* 跳到 ';' */
            }
            ni++;
        }
    }
    return ni;
}

uint32_t jqt_jni_call(uint32_t jni_id, uint32_t n_args, const uint32_t *args){
    if (!g_env) { LOGE("jni env NULL（未 setenv）"); return 0; }
    if (jni_id >= JQJNINUM) { LOGE("jni_id %u 越界（max %d）", jni_id, JQJNINUM); return 0; }
    /* ★ §124am 跨线程 env 修复：GLThread 调 jqt_jni_call 用主线程 g_env → 跨线程 JNIEnv 非法
     *   （'accessed stale Local'，View.<init> MterpInvokeVirtual 崩）。用 JavaVM->GetEnv 拿
     *   当前线程 env（GLThread 是 Java 线程，已 attach 到 JVM）。 */
    JNIEnv *env = NULL;
    if (g_vm && (*g_vm)->GetEnv(g_vm, (void**)&env, JNI_VERSION_1_6) == JNI_OK && env) {
        /* 当前线程 env 已获取 */
    } else {
        /* ★ 2026-11-11（继续对齐原版）：GetEnv 失败说明本线程未 attach 到 JVM。
         *   旧版回退主线程 g_env（跨线程 stale JNIEnv → strlen SIGSEGV，
         *   MuMu x86 houdini 转译层实测 HeapTaskDaemon/渲染线程崩）。
         *   未 attach 线程改用 AttachCurrentThread 取本线程合法 env。 */
        if (g_vm && (*g_vm)->AttachCurrentThread(g_vm, (void**)&env, 0) == JNI_OK && env) {
            LOGI("jqt_jni_call: 本线程未 attach，AttachCurrentThread 成功");
        } else {
            env = g_env;  /* 兜底仍回退主线程 env（无 JVM 时保持旧行为） */
        }
    }
    /* 调用计数 + 首条/每 256 条打 logcat（jqjni 标签，MuMu 验证通路用） */
    g_call_count++;
    /* ★ §124aj 诊断：前 60 条每条打（崩溃定位用，确认后恢复聚合） */
    if (g_call_count <= 60 || (g_call_count & 0xFFu) == 0) {
        LOGI("jqt_jni_call #%u id=%u %s.%s n=%u arg0=0x%08x", g_call_count, jni_id,
            jq_jni_calls[jni_id].cls, jq_jni_calls[jni_id].name, n_args,
            n_args>0?args[0]:0);
        g_last_id = jni_id;
    }
    JqJniCall *c = &jq_jni_calls[jni_id];
    /* ★ §124am 跨线程修复：jclass/jmethodID 都是 FindClass/GetMethodID 的局部引用，
     *   跨线程（GLThread 用主线程 OnTimer setenv 的 g_env）复用 → 'accessed stale Local'。
     *   每次调用前重新 FindClass + GetMethodID（同线程 FindClass+CallXxx 安全），弃 resolve 缓存。 */
    jclass cls = (*env)->FindClass(env, c->cls);
    if (!cls) {
        LOGE("FindClass %s 失败（跨线程）", c->cls);
        (*env)->ExceptionClear(env);
        return 0;
    }
    jmethodID mid = (*env)->GetMethodID(env, cls, c->name, c->sig);
    if (!mid) {
        LOGE("GetMethodID %s.%s %s 失败（跨线程）", c->cls, c->name, c->sig);
        (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, cls);
        return 0;
    }

    jvalue jv[12];
    int nreal = parse_args(c->sig, n_args, args, jv);
    char rt = ret_type_of(c->sig);
    jobject res = NULL;
    jint rint = 0;
    jboolean rbool = 0;
    jlong rlong = 0;

    if (strcmp(c->name, "<init>") == 0) {
        res = (*env)->NewObject(env, cls, mid,
            nreal>0?jv[0].j:0, nreal>1?jv[1].j:0, nreal>2?jv[2].j:0,
            nreal>3?jv[3].j:0, nreal>4?jv[4].j:0, nreal>5?jv[5].j:0,
            nreal>6?jv[6].j:0, nreal>7?jv[7].j:0);
    } else if (is_static_id((int)jni_id)) {
        switch (rt) {
            case 'V': (*env)->CallStaticVoidMethodA(env, cls, mid, jv); break;
            case 'Z': rbool = (*env)->CallStaticBooleanMethodA(env, cls, mid, jv); break;
            case 'I': rint  = (*env)->CallStaticIntMethodA(env, cls, mid, jv); break;
            case 'J': rlong = (*env)->CallStaticLongMethodA(env, cls, mid, jv); break;
            default:  res   = (*env)->CallStaticObjectMethodA(env, cls, mid, jv); break;
        }
    } else {
        /* 实例：args[0]=this 句柄，实际参 = jv（parse_args 不含 this，因 sig 不含 this）
           ★ AUD-1000 B05-050（P1 已修，实例调用 args 非空校验）：n_args=0
           时旧版读 args[0] 越界（args 指针可能 NULL/越界读）——非静态
           非 <init> 实例调用必须有 this。加守卫：n_args==0 → 返回 0
           （忠实兜底，同 env NULL 路径）。 */
        if (n_args == 0) {
            LOGE("%s.%s 实例调用缺 this（n_args=0）→ 忠实兜底返回 0", c->cls, c->name);
            (*env)->DeleteLocalRef(env, cls);
            return 0;
        }
        jobject thisobj = (jobject)(uintptr_t)args[0];
        switch (rt) {
            case 'V': (*env)->CallVoidMethodA(env, thisobj, mid, jv); break;
            case 'Z': rbool = (*env)->CallBooleanMethodA(env, thisobj, mid, jv); break;
            case 'I': rint  = (*env)->CallIntMethodA(env, thisobj, mid, jv); break;
            case 'J': rlong = (*env)->CallLongMethodA(env, thisobj, mid, jv); break;
            default:  res   = (*env)->CallObjectMethodA(env, thisobj, mid, jv); break;
        }
    }

    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        LOGE("%s.%s 异常（已清）", c->cls, c->name);
        return 0;
    }
    if (strcmp(c->name, "<init>") == 0) return (uint32_t)(uintptr_t)res;
    switch (rt) {
        case 'Z': return (uint32_t)rbool;
        case 'I': return (uint32_t)rint;
        case 'J': return (uint32_t)(long)rlong;
        case 'V': return 0;
        default:  return (uint32_t)(uintptr_t)res;
    }
}

/* ★ §124ak：F_bdd2c bdd48 `blx r3` = GetObjectClass（JNIEnv vtable[31]，offset 124/4）。
 *   翻译体把 blx 替换成 jqt_blx_dispatch（查原版 dispatch 表）→ JNI 函数指针
 *   （真实高地址 >=MAP_HI）不在表内 → return 0（无日志）→ GetObjectClass 未调用
 *   → r5=0 → 触摸不处理（屏幕不变化）。
 *   特判：真实调用 GetObjectClass(env, obj)。jclass 返回（uint32_t 句柄）。
 *   签名与 JNIEnv vtable[31] 一致：jclass GetObjectClass(JNIEnv*, jobject)。 */
uint32_t jqt_jni_getobjectclass(uint32_t env, uint32_t obj){
    JNIEnv *e = (JNIEnv *)(uintptr_t)env;
    if (!e) { LOGE("GetObjectClass: env NULL"); return 0; }
    if (obj == 0) { /* §124y-143 防御: java_object==null → JNI_CHECK 崩 SIGABRT */
        LOGE("GetObjectClass: obj NULL (忠实兜底, 返回 0)");
        return 0;
    }
    /* ★ 2026-11-11（继续对齐原版）：GetEnv 失败回退主线程 env 是未 attach 线程的
     *   stale JNIEnv → strlen(env->XXX) SIGSEGV（MuMu x86 houdini 转译层实测）。
     *   未 attach 线程改为 AttachCurrentThread 取本线程合法 env，不再回退主线程 env。 */
    if (!e && g_vm) {
        JNIEnv *attached = NULL;
        if ((*g_vm)->GetEnv(g_vm, (void **)&attached, JNI_VERSION_1_6) != JNI_OK) {
            if ((*g_vm)->AttachCurrentThread(g_vm, (void **)&attached, 0) != JNI_OK || !attached) {
                LOGE("GetEnv/Attach 失败（无合法 env，返回 0）");
                return 0;
            }
        }
        e = attached;
    }
    if (!e) { LOGE("GetObjectClass: env NULL"); return 0; }
    jclass cls = (*e)->GetObjectClass(e, (jobject)(uintptr_t)obj);
    if (!cls && (*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    return (uint32_t)(uintptr_t)cls;
}

/* ★ §124ak：F_bdd2c bdd60/bdd84 `blx r6/ip` = GetMethodID（JNIEnv functions[33]，offset 132/4）。
 *   特判真实调用。name/sig 是影子偏移（<MAP_HI），转真实地址用 extern g_jqt_shadow
 *   （不 include jqt_mem.h——§124ak 实测 include 引入 MuMu dlopen 偶发失败）。
 *   ★ AUD-1000 B05-054（P1 已修，JQT_ABS_PTR 语义）：off 可能已是绝对地址
 *   （>=MAP_HI，如某调用方传 RELATIVE 重定位值/真实堆指针），旧版无条件
 *   g_jqt_shadow+off 越界（off 0x7xxxxxxx + shadow 基址 → 远超映射区读
 *   SIGSEGV）。现 JQT_ABS_PTR 语义（jqt_mem.h JQT_MAP_HI=0x10000000）：
 *   off<MAP_HI → shadow+off（偏移）；off>=MAP_HI → 原样（已是绝对指针），
 *   与 jqt_mem.h JQT_ABS_PTR 同口径。常量直用（不 include jqt_mem.h，
 *   §124ak dlopen 偶发失败教训），与 JQT_MAP_HI 值同步（改 MAP_HI 需
 *   本处同步，注释锚点防漂移）。 */
extern uint8_t *g_jqt_shadow;
uint32_t jqt_jni_getmethodid(uint32_t env, uint32_t cls, uint32_t name_off, uint32_t sig_off){
    JNIEnv *e = (JNIEnv *)(uintptr_t)env;
    if (!e || !cls) { LOGE("GetMethodID: env/cls NULL"); return 0; }
    if (name_off == 0 || sig_off == 0) { /* §124y-143 防御: name/sig 偏移无效 → 返回 0 */
        LOGE("GetMethodID: name/sig NULL (忠实兜底, 返回 0)");
        return 0;
    }
    /* ★ B05-054：JQT_ABS_PTR 语义（常量 0x10000000u = jqt_mem.h JQT_MAP_HI，
       同步锚点见上方注释）——off 已是绝对地址（>=MAP_HI，RELATIVE 重定位
       值/真实指针）时原样用，不 +shadow 越界。 */
    #define JQ_BR_ABS(off) (((uint32_t)(off) < 0x10000000u) ? (uint32_t)(g_jqt_shadow + (off)) : (off))
    const char *name = (const char *)(uintptr_t)JQ_BR_ABS(name_off);
    const char *sig  = (const char *)(uintptr_t)JQ_BR_ABS(sig_off);
    jmethodID mid = (*e)->GetMethodID(e, (jclass)(uintptr_t)cls, name, sig);
    if (!mid && (*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    return (uint32_t)(uintptr_t)mid;
}

/* ★ §124ak：F_bdd2c bddc6 `blx r3` = DeleteLocalRef（JNIEnv functions[23]，offset 92/4）。 */
uint32_t jqt_jni_deletelocalref(uint32_t env, uint32_t obj){
    JNIEnv *e = (JNIEnv *)(uintptr_t)env;
    if (!e) return 0;
    (*e)->DeleteLocalRef(e, (jobject)(uintptr_t)obj);
    return 0;
}

/* ★ §124y-142：F_bd870 bda0 `blx r3` = GetStringUTFChars（jstring → const char*）。
 *   特判真实调用。返回 char* 指针（UTF-8 字符串），需 ReleaseStringUTFChars 释放
 *   （翻译体诚实边界：未调 Release，依赖进程生命周期）。 */
uint32_t jqt_jni_getstringutfchars(uint32_t env, uint32_t str){
    JNIEnv *e = (JNIEnv *)(uintptr_t)env;
    if (!e || !str) { LOGE("GetStringUTFChars: env/str NULL"); return 0; }
    jboolean _iscopy = JNI_TRUE;
    const char *s = (*e)->GetStringUTFChars(e, (jstring)(uintptr_t)str, &_iscopy);
    if (!s && (*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    return (uint32_t)(uintptr_t)s;
}
