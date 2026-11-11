/* jq_test.c — x86_64 宿主上的假 JNI + GL 桩，用于验证影子地址空间翻译运行时
 * 用法: ./jq_test [--create] [--timer N]
 */
#include "jq_shadow.h"
#include "jq_jni_slots.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>

/* 14 个导出入口 */
extern void *Java_com_cnvcs_App_OnCreate(void *, void *, void *);
extern void Java_com_cnvcs_App_OnDestroy(void *, void *);
extern void Java_com_cnvcs_App_OnEvent(void *, void *, void *, void *);
extern void Java_com_cnvcs_App_OnInput(void *, void *, void *);
extern void Java_com_cnvcs_App_OnResult(void *, void *, int, int, void *);
extern void Java_com_cnvcs_App_OnBluetooth(void *, void *, void *);
extern long Java_com_cnvcs_App_OnTouch(void *, void *, void *);
extern long Java_com_cnvcs_App_OnKey(void *, void *, void *);
extern void Java_com_cnvcs_App_OnTimer(void *, void *);
extern long Java_com_cnvcs_App_OnUpdate(void *, void *);
extern void Java_com_cnvcs_App_OnGraphChange(void *, void *, int, int);
extern void Java_com_cnvcs_App_OnGraphCreate(void *, void *);
extern void Java_com_cnvcs_App_OnResume(void *, void *);
extern void Java_com_cnvcs_App_OnPause(void *, void *);

/* ---------------- GL 桩 ---------------- */
static const char *GL_VERSION  = "OpenGL ES-CM 1.1";
static const char *GL_VENDOR   = "jq-test";
static const char *GL_RENDERER = "jq-test";
static const char *GL_EXT      = "GL_OES_texture_npot";
void glBindTexture(unsigned int a, unsigned int b) { (void)a; (void)b; }
void glBlendFuncSeparate(unsigned int a, unsigned int b, unsigned int c, unsigned int d) { (void)a; (void)b; (void)c; (void)d; }
void glClear(unsigned int a) { (void)a; }
void glClearColor(float a, float b, float c, float d) { (void)a; (void)b; (void)c; (void)d; }
void glDeleteProgram(unsigned int a) { (void)a; }
void glDeleteShader(unsigned int a) { (void)a; }
void glDeleteTextures(int a, const unsigned int *b) { (void)a; (void)b; }
void glDisable(unsigned int a) { (void)a; }
void glDrawArrays(unsigned int a, int b, int c) { (void)a; (void)b; (void)c; }
void glEnable(unsigned int a) { (void)a; }
void glEnableVertexAttribArray(unsigned int a) { (void)a; }
void glFlush(void) {}
int glGetAttribLocation(unsigned int a, const char *b) { (void)a; (void)b; return -1; }
void glGetIntegerv(unsigned int p, int *v) {
    switch (p) {
    case 0x0D33: v[0] = 4096; break;          /* MAX_TEXTURE_SIZE */
    case 0x0D3A: v[0] = 4096; v[1] = 4096; break; /* MAX_VIEWPORT_DIMS */
    default: v[0] = 0; v[1] = 0; v[2] = 0; v[3] = 0;
    }
}
const unsigned char *glGetString(unsigned int n) {
    if (n == 0x1F02) return (const unsigned char *)GL_VERSION;
    if (n == 0x1F00) return (const unsigned char *)GL_VENDOR;
    if (n == 0x1F01) return (const unsigned char *)GL_RENDERER;
    if (n == 0x1F03) return (const unsigned char *)GL_EXT;
    return (const unsigned char *)"";
}
int glGetUniformLocation(unsigned int a, const char *b) { (void)a; (void)b; return -1; }
void glPixelStorei(unsigned int a, int b) { (void)a; (void)b; }
void glReadPixels(int a, int b, int c, int d, unsigned int e, unsigned int f, void *g) {
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g;
}
void glTexSubImage2D(unsigned int a, int b, int c, int d, int e, int f, unsigned int g, unsigned int h, const void *i) {
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; (void)i;
}
void glUniform1i(int a, int b) { (void)a; (void)b; }
void glUniformMatrix4fv(int a, int b, unsigned char c, const float *d) { (void)a; (void)b; (void)c; (void)d; }
void glVertexAttribPointer(unsigned int a, int b, unsigned int c, unsigned char d, int e, const void *f) {
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
}
void glViewport(int a, int b, int c, int d) { (void)a; (void)b; (void)c; (void)d; }


unsigned int glCreateProgram(void) { return 1; }
unsigned int glCreateShader(unsigned int a) { (void)a; return 1; }
void glAttachShader(unsigned int a, unsigned int b) { (void)a; (void)b; }
void glCompileShader(unsigned int a) { (void)a; }
void glLinkProgram(unsigned int a) { (void)a; }
void glUseProgram(unsigned int a) { (void)a; }
void glGetProgramiv(unsigned int a, unsigned int b, int *v) { (void)a; (void)b; if (v) v[0] = 1; }
void glGetShaderiv(unsigned int a, unsigned int b, int *v) { (void)a; (void)b; if (v) v[0] = 1; }
void glGenTextures(int n, unsigned int *ids) { for (int i = 0; i < n; i++) ids[i] = (unsigned int)(i + 1); }
void glShaderSource(unsigned int a, int n, const char *const *s, const int *l) { (void)a; (void)n; (void)s; (void)l; }
void glTexImage2D(unsigned int a, int b, int c, int d, int e, int f, unsigned int g, unsigned int h, const void *i) {
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; (void)i;
}
void glTexParameterf(unsigned int a, unsigned int b, float c) { (void)a; (void)b; (void)c; }

/* ---------------- 假 JNI ---------------- */
#define MAXOBJ 4096
static void *g_names[MAXOBJ];
static char g_namestr[MAXOBJ][96];
static int g_names_n;
static int g_verbose_jni = 1;

static void *mkobj(const char *name) {
    for (int i = 0; i < g_names_n; i++) if (strcmp(g_namestr[i], name) == 0) return g_names[i];
    if (g_names_n >= MAXOBJ) return NULL;
    int i = g_names_n++;
    g_names[i] = (void *)(uintptr_t)(0x70000000u + i * 16);
    strncpy(g_namestr[i], name, 95);
    return g_names[i];
}

static void *m_GetVersion(void *e) { (void)e; return (void *)0x00010006; }
static void *m_FindClass(void *e, const char *name) {
    (void)e;
    if (g_verbose_jni) fprintf(stderr, "[jni] FindClass %s\n", name);
    return mkobj(name);
}
static void *m_GetObjectClass(void *e, void *o) { (void)e; (void)o; return mkobj("java/lang/Object"); }
static void *m_GetSuperclass(void *e, void *c) { (void)e; (void)c; return mkobj("super"); }
static int m_IsAssignableFrom(void *e, void *a, void *b) { (void)e; (void)a; (void)b; return 1; }
static void *m_GetMethodID(void *e, void *c, const char *n, const char *s) {
    (void)e;
    if (g_verbose_jni) fprintf(stderr, "[jni] GetMethodID %s %s\n", n, s);
    return mkobj(n);
}
static void *m_GetStaticMethodID(void *e, void *c, const char *n, const char *s) {
    (void)e;
    if (g_verbose_jni) fprintf(stderr, "[jni] GetStaticMethodID %s %s\n", n, s);
    return mkobj(n);
}
static void *m_GetFieldID(void *e, void *c, const char *n, const char *s) {
    (void)e;
    if (g_verbose_jni) fprintf(stderr, "[jni] GetFieldID %s %s\n", n, s);
    return mkobj(n);
}
static void *m_GetStaticFieldID(void *e, void *c, const char *n, const char *s) {
    (void)e;
    if (g_verbose_jni) fprintf(stderr, "[jni] GetStaticFieldID %s %s\n", n, s);
    return mkobj(n);
}
static void *m_NewStringUTF(void *e, const char *s) { (void)e; return strdup(s ? s : ""); }
static int m_GetStringUTFLength(void *e, void *s) { (void)e; return s ? (int)strlen((char *)s) : 0; }
static int m_GetStringLength(void *e, void *s) { (void)e; return s ? (int)strlen((char *)s) : 0; }
static void *m_GetStringUTFChars(void *e, void *s, unsigned char *cp) {
    (void)e; if (cp) *cp = 0; return s;
}
static void m_ReleaseStringUTFChars(void *e, void *s, const char *c) { (void)e; (void)s; (void)c; }
static void *m_NewGlobalRef(void *e, void *o) { (void)e; return o; }
static void m_DeleteGlobalRef(void *e, void *o) { (void)e; (void)o; }
static void m_DeleteLocalRef(void *e, void *o) { (void)e; (void)o; }
static int m_IsSameObject(void *e, void *a, void *b) { (void)e; return a == b; }
static void *m_NewLocalRef(void *e, void *o) { (void)e; return o; }
static int m_ExceptionCheck(void *e) { (void)e; return 0; }
static void *m_ExceptionOccurred(void *e) { (void)e; return NULL; }
static void m_ExceptionClear(void *e) { (void)e; }
static void m_ExceptionDescribe(void *e) { (void)e; }
static int m_Throw(void *e, void *o) { (void)e; (void)o; return 0; }
static int m_ThrowNew(void *e, void *c, const char *m) {
    (void)e; (void)c;
    fprintf(stderr, "[jni] ThrowNew %s\n", m ? m : "?");
    return 0;
}
static int m_PushLocalFrame(void *e, int n) { (void)e; (void)n; return 0; }
static void *m_PopLocalFrame(void *e, void *o) { (void)e; return o; }
static void *m_AllocObject(void *e, void *c) { (void)e; return c; }

/* 调用返回值桩 */
static void *m_CallObjectA(void *e, void *o, void *m, void *a) { (void)e; (void)o; (void)m; (void)a; return NULL; }
static int m_CallIntA(void *e, void *o, void *m, void *a) { (void)e; (void)o; (void)m; (void)a; return 0; }
static long long m_CallLongA(void *e, void *o, void *m, void *a) { (void)e; (void)o; (void)m; (void)a; return 0; }
static float m_CallFloatA(void *e, void *o, void *m, void *a) { (void)e; (void)o; (void)m; (void)a; return 0; }
static double m_CallDoubleA(void *e, void *o, void *m, void *a) { (void)e; (void)o; (void)m; (void)a; return 0; }
static void m_CallVoidA(void *e, void *o, void *m, void *a) { (void)e; (void)o; (void)m; (void)a; }
static int m_CallBoolA(void *e, void *o, void *m, void *a) { (void)e; (void)o; (void)m; (void)a; return 0; }

static void *m_NewObjectA(void *e, void *c, void *m, void *a) {
    (void)e; (void)m; (void)a;
    if (g_verbose_jni) fprintf(stderr, "[jni] NewObject class=%s\n", "obj");
    return c;
}
static void *m_NewArray(void *e, int n) {
    (void)e;
    if (n < 0) n = 0;
    void *p = calloc(1, (size_t)n + 16);
    return p;
}
static int m_GetArrayLength(void *e, void *a) { (void)e; (void)a; return 0; }
static void *m_GetArrayElems(void *e, void *a, unsigned char *c) {
    (void)e; (void)a;
    if (c) *c = 0;
    static unsigned char zero[4096];
    return zero;
}
static void *m_GetByteArrayElems(void *e, void *a, unsigned char *c) {
    (void)e; (void)a;
    if (c) *c = 0;
    static unsigned char zero[4096];
    return zero;
}
static void m_ReleaseArrayElems(void *e, void *a, void *p, int m) { (void)e; (void)a; (void)p; (void)m; }
static int m_GetArrayRegion(void *e, void *a, int s, int l, void *b) { (void)e; (void)a; (void)s; (void)l; (void)b; return 0; }
static void m_SetArrayRegion(void *e, void *a, int s, int l, const void *b) { (void)e; (void)a; (void)s; (void)l; (void)b; }

static void *g_tab[233];
static void *g_env[1] = { g_tab };

static void mock_init(void) {
    g_tab[JQS_GetVersion] = (void *)m_GetVersion;
    g_tab[JQS_FindClass] = (void *)m_FindClass;
    g_tab[JQS_GetObjectClass] = (void *)m_GetObjectClass;
    g_tab[JQS_GetSuperclass] = (void *)m_GetSuperclass;
    g_tab[JQS_IsAssignableFrom] = (void *)m_IsAssignableFrom;
    g_tab[JQS_GetMethodID] = (void *)m_GetMethodID;
    g_tab[JQS_GetStaticMethodID] = (void *)m_GetStaticMethodID;
    g_tab[JQS_GetFieldID] = (void *)m_GetFieldID;
    g_tab[JQS_GetStaticFieldID] = (void *)m_GetStaticFieldID;
    g_tab[JQS_NewStringUTF] = (void *)m_NewStringUTF;
    g_tab[JQS_GetStringUTFLength] = (void *)m_GetStringUTFLength;
    g_tab[JQS_GetStringLength] = (void *)m_GetStringLength;
    g_tab[JQS_GetStringUTFChars] = (void *)m_GetStringUTFChars;
    g_tab[JQS_ReleaseStringUTFChars] = (void *)m_ReleaseStringUTFChars;
    g_tab[JQS_NewGlobalRef] = (void *)m_NewGlobalRef;
    g_tab[JQS_DeleteGlobalRef] = (void *)m_DeleteGlobalRef;
    g_tab[JQS_DeleteLocalRef] = (void *)m_DeleteLocalRef;
    g_tab[JQS_NewLocalRef] = (void *)m_NewLocalRef;
    g_tab[JQS_IsSameObject] = (void *)m_IsSameObject;
    g_tab[JQS_ExceptionCheck] = (void *)m_ExceptionCheck;
    g_tab[JQS_ExceptionOccurred] = (void *)m_ExceptionOccurred;
    g_tab[JQS_ExceptionClear] = (void *)m_ExceptionClear;
    g_tab[JQS_ExceptionDescribe] = (void *)m_ExceptionDescribe;
    g_tab[JQS_Throw] = (void *)m_Throw;
    g_tab[JQS_ThrowNew] = (void *)m_ThrowNew;
    g_tab[JQS_PushLocalFrame] = (void *)m_PushLocalFrame;
    g_tab[JQS_PopLocalFrame] = (void *)m_PopLocalFrame;
    g_tab[JQS_AllocObject] = (void *)m_AllocObject;
    g_tab[JQS_NewObjectA] = (void *)m_NewObjectA;
    g_tab[JQS_NewObjectV] = (void *)m_NewObjectA;
    g_tab[JQS_NewObject] = (void *)m_NewObjectA;
    void *fns[10] = {(void *)m_CallObjectA, (void *)m_CallBoolA, (void *)m_CallIntA,
                     (void *)m_CallIntA, (void *)m_CallIntA, (void *)m_CallIntA,
                     (void *)m_CallIntA, (void *)m_CallLongA, (void *)m_CallFloatA,
                     (void *)m_CallDoubleA};
    for (int t = 0; t < 10; t++) {
        g_tab[JQS_CallObjectMethodA + t * 3] = fns[t];
        g_tab[JQS_CallObjectMethodV + t * 3] = g_tab[JQS_CallObjectMethodA + t * 3];
        g_tab[JQS_CallObjectMethod + t * 3] = g_tab[JQS_CallObjectMethodA + t * 3];
        g_tab[JQS_CallNonvirtualObjectMethodA + t * 3] = g_tab[JQS_CallObjectMethodA + t * 3];
        g_tab[JQS_CallNonvirtualObjectMethodV + t * 3] = g_tab[JQS_CallObjectMethodA + t * 3];
        g_tab[JQS_CallNonvirtualObjectMethod + t * 3] = g_tab[JQS_CallObjectMethodA + t * 3];
        g_tab[JQS_CallStaticObjectMethodA + t * 3] = g_tab[JQS_CallObjectMethodA + t * 3];
        g_tab[JQS_CallStaticObjectMethodV + t * 3] = g_tab[JQS_CallObjectMethodA + t * 3];
        g_tab[JQS_CallStaticObjectMethod + t * 3] = g_tab[JQS_CallObjectMethodA + t * 3];
    }
    g_tab[JQS_NewBooleanArray] = (void *)m_NewArray;
    g_tab[JQS_NewByteArray] = (void *)m_NewArray;
    g_tab[JQS_NewCharArray] = (void *)m_NewArray;
    g_tab[JQS_NewShortArray] = (void *)m_NewArray;
    g_tab[JQS_NewIntArray] = (void *)m_NewArray;
    g_tab[JQS_NewLongArray] = (void *)m_NewArray;
    g_tab[JQS_NewFloatArray] = (void *)m_NewArray;
    g_tab[JQS_NewDoubleArray] = (void *)m_NewArray;
    g_tab[JQS_GetArrayLength] = (void *)m_GetArrayLength;
    for (int i = 0; i < 8; i++) {
        g_tab[JQS_GetBooleanArrayElements + i] = (void *)m_GetByteArrayElems;
        g_tab[JQS_ReleaseBooleanArrayElements + i] = (void *)m_ReleaseArrayElems;
        g_tab[JQS_GetBooleanArrayRegion + i] = (void *)m_GetArrayRegion;
        g_tab[JQS_SetBooleanArrayRegion + i] = (void *)m_SetArrayRegion;
    }
    g_tab[JQS_NewObjectArray] = (void *)m_NewArray;
    g_tab[JQS_GetObjectArrayElement] = (void *)m_CallObjectA;
    g_tab[JQS_SetObjectArrayElement] = (void *)m_CallVoidA;
    g_tab[JQS_GetIntField] = (void *)m_CallIntA;
    g_tab[JQS_GetLongField] = (void *)m_CallLongA;
    g_tab[JQS_GetFloatField] = (void *)m_CallFloatA;
    g_tab[JQS_GetDoubleField] = (void *)m_CallDoubleA;
    g_tab[JQS_GetObjectField] = (void *)m_CallObjectA;
    g_tab[JQS_GetBooleanField] = (void *)m_CallBoolA;
    g_tab[JQS_GetByteField] = (void *)m_CallIntA;
    g_tab[JQS_GetCharField] = (void *)m_CallIntA;
    g_tab[JQS_GetShortField] = (void *)m_CallIntA;
    g_tab[JQS_GetStaticIntField] = (void *)m_CallIntA;
    g_tab[JQS_GetStaticObjectField] = (void *)m_CallObjectA;
    g_tab[JQS_SetIntField] = (void *)m_CallVoidA;
    g_tab[JQS_SetLongField] = (void *)m_CallVoidA;
    g_tab[JQS_SetFloatField] = (void *)m_CallVoidA;
    g_tab[JQS_SetDoubleField] = (void *)m_CallVoidA;
    g_tab[JQS_SetObjectField] = (void *)m_CallVoidA;
    g_tab[JQS_SetBooleanField] = (void *)m_CallVoidA;
    g_tab[JQS_SetStaticIntField] = (void *)m_CallVoidA;
    g_tab[JQS_SetStaticObjectField] = (void *)m_CallVoidA;
}

static void crash(int sig, siginfo_t *si, void *ctx) {
    (void)ctx;
    fprintf(stderr, "[test] 信号 %d @ %p\n", sig, si->si_addr);
    _exit(90);
}

int main(int argc, char **argv) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crash;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);

    mock_init();
    fprintf(stderr, "[test] jq_init()...\n");
    jq_init();
    fprintf(stderr, "[test] init 完成, base=%p sp=0x%x\n", (void *)g_jq_base, jq_shadow_sp());

    void *cls = mkobj("com/cnvcs/App");
    void *act = mkobj("android/app/Activity");

    fprintf(stderr, "[test] OnCreate...\n");
    void *view = Java_com_cnvcs_App_OnCreate((void *)g_env, cls, act);
    fprintf(stderr, "[test] OnCreate 返回 %p\n", view);

    fprintf(stderr, "[test] OnGraphCreate...\n");
    Java_com_cnvcs_App_OnGraphCreate((void *)g_env, cls);
    fprintf(stderr, "[test] OnGraphChange 480x800\n");
    Java_com_cnvcs_App_OnGraphChange((void *)g_env, cls, 480, 800);

    int n = 3;
    for (int i = 1; i < argc; i++) if (!strcmp(argv[i], "--timer") && i + 1 < argc) n = atoi(argv[i + 1]);
    for (int i = 0; i < n; i++) {
        fprintf(stderr, "[test] OnTimer #%d\n", i);
        Java_com_cnvcs_App_OnTimer((void *)g_env, cls);
    }
    fprintf(stderr, "[test] OnPause...\n");
    Java_com_cnvcs_App_OnPause((void *)g_env, cls);
    fprintf(stderr, "[test] OnResume...\n");
    Java_com_cnvcs_App_OnResume((void *)g_env, cls);
    fprintf(stderr, "[test] OnUpdate...\n");
    Java_com_cnvcs_App_OnUpdate((void *)g_env, cls);
    fprintf(stderr, "[test] OnDestroy...\n");
    Java_com_cnvcs_App_OnDestroy((void *)g_env, cls);
    fprintf(stderr, "[test] 全部通过\n");
    return 0;
}
