/*
 * jni.h — 桌面构建用 **C 模式最小 JNI 头**（src_rs/android_stub/）
 *
 * 用途：让 src_rs 的 native 源文件（jni_bridge.c / glview_gl.c / audio.c …）能在
 *   桌面 x86_64 编译（渗透轮 fuzz 探针：fuzz_jni 等）。Android NDK 构建走真 <jni.h>
 *   （真机构建不受影响：本目录仅在桌面构建时通过 -Iandroid_stub 加入搜索路径）。
 *
 * 为什么不用系统 JDK/NDK 的 jni.h：
 *   - JDK 的 jni.h 在 C++ 模式是**成员函数包装类**（JNINativeInterface_ 的字段是
 *     成员函数指针），与 src_rs 的 C 写法 (*env)->FindClass(env,...) 不兼容；
 *     C 模式又需要 jni_md.h（平台相关）
 *   - NDK 的 jni.h 需配合 NDK sysroot 头链（会遮蔽宿主 libc 头）
 *   → 自带最小 C 模式头 = 自包含 + 无外部依赖 + CI 可复现
 *
 * ★ 2026-11-20 诚实标注（看雪文章 JNI 函数表索引纪律核对）：
 *   **本 stub 的 JNINativeInterface 槽位顺序并非 JNI 标准顺序**（GetStringUTFChars 在
 *   索引 67，标准应为 169/ARM32 偏移 676）——仅为让 src_rs 的 `(*env)->Func()` 符号名
 *   调用在桌面编译通过。src_rs 不硬编码槽位偏移（grep 实锤：全用 (*env)->Func 符号名），
 *   故 stub 顺序错**不影响 fuzz 判据**。但任何"靠 JNIEnv 函数表硬编码偏移取函数"的场景
 *   （如看雪文章样本 `v4+676` 的 GetStringUTFChars）**不能用本 stub 验证**——那需要
 *   完整标准 JNINativeInterface（表头 4 reserved + 全部槽位按顺序）。
 *
 * 覆盖范围：src_rs 实际用到的 JNI 函数（jni_bridge.c / glview_gl.c / audio.c 全量
 * 扫描 = 21 个函数 + 4 个常量）；未覆盖的 JNI 函数在结构体里留 NULL（so 侧若误用
 * → 空指针调用 → 崩溃 = 探针会暴露，属"探针覆盖缺口"而非静默通过）。
 */
#ifndef ANDROID_STUB_JNI_H
#define ANDROID_STUB_JNI_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>   /* jni_bridge.c 用 FILE*（真机经 NDK 链间接引入；桌面 stub 直接给） */

#define JNIEXPORT
#define JNICALL
#define JNI_VERSION_1_2 0x00010002
#define JNI_VERSION_1_4 0x00010004
#define JNI_VERSION_1_6 0x00010006

typedef int             jint;
typedef long            jlong;
typedef unsigned char   jboolean;
typedef unsigned char   jbyte;
typedef unsigned short  jchar;
typedef short           jshort;
typedef int             jsize;
typedef float           jfloat;
typedef double          jdouble;

typedef void           *jobject;
typedef jobject         jclass;
typedef jobject         jstring;
typedef jobject         jarray;
typedef jarray          jbyteArray;
typedef jobject         jthrowable;
typedef jobject         jweak;
typedef void           *jobjectRef;   /* 内部占位（不与 jobject 混用） */

typedef struct _jvalue {
    union {
        jboolean z;
        jbyte    b;
        char     c;
        short    s;
        jint     i;
        jlong    j;
        jfloat   f;
        jdouble  d;
        jobject  l;
    } l;
} jvalue;

struct JNIEnv;
struct JNIInvokeInterface;
typedef const struct JNIInvokeInterface *JavaVM;
struct JNIInvokeInterface {
    void *reserved0;
    void *reserved1;
    void *reserved2;
    void *reserved3;
    jint (*DestroyJavaVM)(JavaVM *);
    jint (*AttachCurrentThread)(JavaVM *, void **, void *);
    jint (*DetachCurrentThread)(JavaVM *);
    jint (*GetEnv)(JavaVM *, void **, jint);
};
typedef struct JNINativeInterface JNINativeInterface;
typedef const JNINativeInterface *JNIEnv;
typedef struct _jmethodID *jmethodID;

typedef union jvalueUnion { jboolean z; jbyte b; char c; short s; jint i; jlong j;
                            jfloat f; jdouble d; jobject l; } jvalueUnion;

#define JNI_FALSE 0
#define JNI_TRUE  1
#define JNI_OK    0
#define JNI_ERR   (-1)
#define JNI_EDETACHED (-2)
#define JNI_EVERSION (-3)

struct JNINativeInterface {
    void *reserved0;
    void *reserved1;
    void *reserved2;
    void *reserved3;

    jclass    (JNICALL *FindClass)(JNIEnv *, const char *);
    jclass    (JNICALL *DefineClass)(JNIEnv *, const char *, jbyte *, jsize);
    jmethodID (JNICALL *GetMethodID)(JNIEnv *, jclass, const char *, const char *);
    jmethodID (JNICALL *GetStaticMethodID)(JNIEnv *, jclass, const char *, const char *);
    jmethodID (JNICALL *GetFieldID)(JNIEnv *, jclass, const char *, const char *);
    jmethodID (JNICALL *GetStaticFieldID)(JNIEnv *, jclass, const char *, const char *);

    jobject   (JNICALL *NewGlobalRef)(JNIEnv *, jobject);
    void      (JNICALL *DeleteGlobalRef)(JNIEnv *, jobject);
    void      (JNICALL *DeleteLocalRef)(JNIEnv *, jobject);
    jboolean  (JNICALL *IsSameObject)(JNIEnv *, jobject, jobject);
    jobject   (JNICALL *NewLocalRef)(JNIEnv *, jobject);
    jint      (JNICALL *EnsureLocalCapacity)(JNIEnv *, jint);

    jobject   (JNICALL *AllocObject)(JNIEnv *, jclass);
    jobject   (JNICALL *NewObject)(JNIEnv *, jclass, jmethodID, ...);
    jobject   (JNICALL *NewObjectV)(JNIEnv *, jclass, jmethodID, va_list);
    jobject   (JNICALL *NewObjectA)(JNIEnv *, jclass, jmethodID, const jvalue *);

    jclass    (JNICALL *GetObjectClass)(JNIEnv *, jobject);
    jboolean  (JNICALL *IsInstanceOf)(JNIEnv *, jobject, jclass);

    jobject   (JNICALL *CallObjectMethod)(JNIEnv *, jobject, jmethodID, ...);
    jobject   (JNICALL *CallObjectMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jobject   (JNICALL *CallObjectMethodA)(JNIEnv *, jobject, jmethodID, const jvalue *);
    jboolean  (JNICALL *CallBooleanMethod)(JNIEnv *, jobject, jmethodID, ...);
    jboolean  (JNICALL *CallBooleanMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jboolean  (JNICALL *CallBooleanMethodA)(JNIEnv *, jobject, jmethodID, const jvalue *);
    jbyte     (JNICALL *CallByteMethod)(JNIEnv *, jobject, jmethodID, ...);
    jbyte     (JNICALL *CallByteMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jbyte     (JNICALL *CallByteMethodA)(JNIEnv *, jobject, jmethodID, const jvalue *);
    jchar     (JNICALL *CallCharMethod)(JNIEnv *, jobject, jmethodID, ...);
    jchar     (JNICALL *CallCharMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jchar     (JNICALL *CallCharMethodA)(JNIEnv *, jobject, jmethodID, const jvalue *);
    jshort    (JNICALL *CallShortMethod)(JNIEnv *, jobject, jmethodID, ...);
    jshort    (JNICALL *CallShortMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jshort    (JNICALL *CallShortMethodA)(JNIEnv *, jobject, jmethodID, const jvalue *);
    jint      (JNICALL *CallIntMethod)(JNIEnv *, jobject, jmethodID, ...);
    jint      (JNICALL *CallIntMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jint      (JNICALL *CallIntMethodA)(JNIEnv *, jobject, jmethodID, const jvalue *);
    jlong     (JNICALL *CallLongMethod)(JNIEnv *, jobject, jmethodID, ...);
    jlong     (JNICALL *CallLongMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jlong     (JNICALL *CallLongMethodA)(JNIEnv *, jobject, jmethodID, const jvalue *);
    jfloat    (JNICALL *CallFloatMethod)(JNIEnv *, jobject, jmethodID, ...);
    jfloat    (JNICALL *CallFloatMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jfloat    (JNICALL *CallFloatMethodA)(JNIEnv *, jobject, jmethodID, const jvalue *);
    jdouble   (JNICALL *CallDoubleMethod)(JNIEnv *, jobject, jmethodID, ...);
    jdouble   (JNICALL *CallDoubleMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    jdouble   (JNICALL *CallDoubleMethodA)(JNIEnv *, jobject, jmethodID, const jvalue *);
    void      (JNICALL *CallVoidMethod)(JNIEnv *, jobject, jmethodID, ...);
    void      (JNICALL *CallVoidMethodV)(JNIEnv *, jobject, jmethodID, va_list);
    void      (JNICALL *CallVoidMethodA)(JNIEnv *, jobject, jmethodID, const jvalue *);

    jobject   (JNICALL *CallNonvirtualObjectMethod)(JNIEnv *, jobject, jclass, jmethodID, ...);
    jint      (JNICALL *CallNonvirtualIntMethod)(JNIEnv *, jobject, jclass, jmethodID, ...);
    void      (JNICALL *CallNonvirtualVoidMethod)(JNIEnv *, jobject, jclass, jmethodID, ...);

    void      (JNICALL *CallStaticVoidMethod)(JNIEnv *, jclass, jmethodID, ...);
    void      (JNICALL *CallStaticVoidMethodV)(JNIEnv *, jclass, jmethodID, va_list);
    void      (JNICALL *CallStaticVoidMethodA)(JNIEnv *, jclass, jmethodID, const jvalue *);
    jint      (JNICALL *CallStaticIntMethod)(JNIEnv *, jclass, jmethodID, ...);
    jint      (JNICALL *CallStaticIntMethodV)(JNIEnv *, jclass, jmethodID, va_list);
    jint      (JNICALL *CallStaticIntMethodA)(JNIEnv *, jclass, jmethodID, const jvalue *);

    jstring   (JNICALL *NewString)(JNIEnv *, const jchar *, jsize);
    jsize     (JNICALL *GetStringLength)(JNIEnv *, jstring);
    const jchar *(JNICALL *GetStringChars)(JNIEnv *, jstring, jboolean *);
    void      (JNICALL *ReleaseStringChars)(JNIEnv *, jstring, const jchar *);
    jstring   (JNICALL *NewStringUTF)(JNIEnv *, const char *);
    jsize     (JNICALL *GetStringUTFLength)(JNIEnv *, jstring);
    const char *(JNICALL *GetStringUTFChars)(JNIEnv *, jstring, jboolean *);
    void      (JNICALL *ReleaseStringUTFChars)(JNIEnv *, jstring, const char *);

    jsize     (JNICALL *GetArrayLength)(JNIEnv *, jarray);
    jbyteArray (JNICALL *NewByteArray)(JNIEnv *, jsize);
    jbyte    *(JNICALL *GetByteArrayElements)(JNIEnv *, jbyteArray, jboolean *);
    void      (JNICALL *ReleaseByteArrayElements)(JNIEnv *, jbyteArray, jbyte *, jint);
    void      (JNICALL *GetByteArrayRegion)(JNIEnv *, jbyteArray, jsize, jsize, jbyte *);
    void      (JNICALL *SetByteArrayRegion)(JNIEnv *, jbyteArray, jsize, jsize, const jbyte *);

    jboolean  (JNICALL *ExceptionCheck)(JNIEnv *);
    void      (JNICALL *ExceptionClear)(JNIEnv *);
    void      (JNICALL *ExceptionDescribe)(JNIEnv *);
    void      (JNICALL *FatalError)(JNIEnv *, const char *);
    jint      (JNICALL *PushLocalFrame)(JNIEnv *, jint);
    jobject   (JNICALL *PopLocalFrame)(JNIEnv *, jobject);
    jobject   (JNICALL *NewWeakGlobalRef)(JNIEnv *, jobject);
    void      (JNICALL *DeleteWeakGlobalRef)(JNIEnv *, jweak);
    jboolean  (JNICALL *ExceptionOccurred)(JNIEnv *);
    jobject   (JNICALL *ExceptionOccurredClear)(JNIEnv *);
    void      (JNICALL *ReleaseStringUTFCharsStub)(JNIEnv *, jstring, const char *);
    jint      (JNICALL *GetVersion)(JNIEnv *);
};

#endif /* ANDROID_STUB_JNI_H */
