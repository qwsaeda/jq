/*
 * bt_net.c — R6 蓝牙联机网络层（native JNI 直调 Java 蓝牙 API）
 *
 * 方案A：native JNI 直调 Java BluetoothAdapter/Device/Socket（RFCOMM SPP +
 * InputStream/OutputStream 帧收发），最小 Java 改动（App.smali native 签名不变）。
 *
 * 协议（REVERSE_NOTES §89/§92 确证）：
 *   - UUID `ce1404fe-0863-4278-86c2-c9c560f0e215`（rodata 0x121334）
 *   - 服务名 `junqi`（rodata 0x120d18）
 *   - 版本 v2（0x26efc），帧 "v2\n%d,%d,%d,%d,%d,%d,%d\n"（0x26f24）
 *   - OnBluetooth 主逻辑 0xbe9cc（getAction/getIntExtra/getAddress）
 *
 * 连接流程：
 *   - 服务端：bt_host → listenUsingRfcommWithServiceRecord("junqi",UUID) → accept 线程
 *   - 客户端：bt_discover → ACTION_FOUND → device → createRfcommSocket → connect 线程
 *   - 连接建立 → 双端置 CONNECTED；客户端发 SYNC_REQ，服务端回 SYNC（全盘同步）
 *   - reader 线程常驻读帧 → 行缓冲 → v2 帧/C 行解析 → jq_bt_frame_cb（应用 g_state）
 *   - 本地走子/翻棋 → jq_bt_send_frame（g_bt_mutex 保护写）
 *
 * 线程：connect/accept/reader 均为 pthread（阻塞 IO），JNI 经 AttachCurrentThread。
 * 无真机蓝牙验证：实现记录"编译通过+逻辑完整，真机验证待做"（REVERSE_NOTES §95）。
 */
#include <jni.h>
#include <android/log.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "junqi_core.h"
#include "bt_spak.h"
#include "bt_net.h"

#define LOG_TAG "junqi64bt"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define JQ_BT_UUID      "ce1404fe-0863-4278-86c2-c9c560f0e215"
#define JQ_BT_NAME      "junqi"
#define JQ_BT_READ_BUF  512
#define JQ_BT_LINE_BUF  4096
#define JQ_BT_SYNC_BUF  2048
#define JQ_BT_REQ_ENABLE       1001
#define JQ_BT_REQ_DISCOVERABLE 1002

/* 连接状态机 */
typedef enum {
    JQ_BT_IDLE = 0,
    JQ_BT_CONNECTING,
    JQ_BT_LISTENING,
    JQ_BT_CONNECTED,
    JQ_BT_ERROR
} jq_bt_state_t;

/* ---- 全局（bt_net 私有，g_bt_mutex 保护连接对象/状态） ---- */
static JavaVM *g_vm = NULL;
static jobject g_activity = NULL;   /* 全局引用 Activity */
static jobject g_adapter  = NULL;   /* 全局引用 BluetoothAdapter */
static jobject g_socket   = NULL;   /* 全局引用已连 BluetoothSocket */
static jobject g_in       = NULL;   /* 全局引用 InputStream */
static jobject g_out      = NULL;   /* 全局引用 OutputStream */
static jq_bt_state_t g_bt_state = JQ_BT_IDLE;
static pthread_t g_io_thread;       /* connect/accept 线程 */
static pthread_t g_read_thread;     /* reader 线程 */
static int g_thread_running = 0;
static int g_read_started = 0;
static pthread_mutex_t g_bt_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ---- 帧回调（junqi_jni.c 注册） ---- */
static jq_bt_frame_cb g_frame_cb = NULL;
static void *g_frame_user = NULL;

/* ---- 内部：清 pending Java 异常（JNI 调用安全） ---- */
static int jq_bt_exception(JNIEnv *env) {
    if (!env) return 0;
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return 1;
    }
    return 0;
}

const char *jq_bt_state_str(void) {
    switch (g_bt_state) {
    case JQ_BT_IDLE:       return "IDLE";
    case JQ_BT_CONNECTING: return "CONNECTING";
    case JQ_BT_LISTENING:  return "LISTENING";
    case JQ_BT_CONNECTED:  return "CONNECTED";
    case JQ_BT_ERROR:      return "ERROR";
    default:               return "?";
    }
}

void jq_bt_set_vm(JavaVM *vm) { g_vm = vm; }

void jq_bt_set_frame_cb(jq_bt_frame_cb cb, void *user) {
    g_frame_cb = cb; g_frame_user = user;
}

int jq_bt_connected(void) {
    pthread_mutex_lock(&g_bt_mutex);
    int c = (g_bt_state == JQ_BT_CONNECTED && g_out);
    pthread_mutex_unlock(&g_bt_mutex);
    return c;
}

/* ---- 内部：把 Java 串拷为 C（malloc，需 free） ---- */
static char *bt_jstr(JNIEnv *env, jstring js) {
    if (!js) return NULL;
    const char *c = (*env)->GetStringUTFChars(env, js, NULL);
    if (!c) return NULL;
    size_t n = strlen(c);
    char *o = (char *)malloc(n + 1);
    if (o) { memcpy(o, c, n); o[n] = 0; }
    (*env)->ReleaseStringUTFChars(env, js, c);
    return o;
}

/* ---- 内部：输出流写入（g_bt_mutex 保护）----
   buf 二进制安全（含 '\0' 帧内字符？帧为 ASCII，无内嵌 0，但按 len 写）。 */
static int jq_bt_write(const char *buf, int len) {
    if (!g_vm || !buf || len <= 0) return -1;
    JNIEnv *env = NULL;
    int detach = 0;
    jint tr = (*g_vm)->GetEnv(g_vm, (void **)&env, JNI_VERSION_1_6);
    if (tr == JNI_EDETACHED) {
        if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) != JNI_OK) return -1;
        detach = 1;
    }
    pthread_mutex_lock(&g_bt_mutex);
    if (!g_out) { pthread_mutex_unlock(&g_bt_mutex); if (detach) (*g_vm)->DetachCurrentThread(g_vm); return -1; }
    jclass os = (*env)->GetObjectClass(env, g_out);
    jmethodID write = (*env)->GetMethodID(env, os, "write", "([B)V");
    jmethodID flush = (*env)->GetMethodID(env, os, "flush", "()V");
    jbyteArray jb = (*env)->NewByteArray(env, len);
    int rc = -1;
    if (write && jb) {
        (*env)->SetByteArrayRegion(env, jb, 0, len, (const jbyte *)buf);
        (*env)->CallVoidMethod(env, g_out, write, jb);
        if (flush) (*env)->CallVoidMethod(env, g_out, flush);
        if (!jq_bt_exception(env)) rc = 0;
    }
    if (jb) (*env)->DeleteLocalRef(env, jb);
    pthread_mutex_unlock(&g_bt_mutex);
    if (detach) (*g_vm)->DetachCurrentThread(g_vm);
    return rc;
}

/* ---- 发送语义帧 ---- */
int jq_bt_send_frame(int type, int a, int b, int c, int d, int e, int f) {
    if (g_bt_state != JQ_BT_CONNECTED) return -1;
    char buf[80];
    int n = jq_bt_frame_encode(buf, sizeof(buf), type, a, b, c, d, e, f);
    if (n <= 0) return -1;
    return jq_bt_write(buf, n);
}

/* ---- 发送全盘同步（SYNC 帧 c=stage,d=turn + 60 格 C 行） ---- */
int jq_bt_send_sync(JNIEnv *env, const jq_state *st) {
    (void)env;
    if (!st) return -1;
    int r = jq_bt_send_frame(JQ_BT_T_SYNC, -1, -1, st->stage, st->turn, -1, -1);
    if (r != 0) return r;
    char buf[JQ_BT_SYNC_BUF];
    int n = jq_bt_frame_sync_encode(buf, sizeof(buf), st, 0, JQ_CELLS);
    if (n <= 0) return -1;
    return jq_bt_write(buf, n);
}

/* ---- 内部：从行缓冲消费一条完整帧/C 行，返回消费字节数（0=需更多数据） ---- */
static int bt_consume(char *buf, size_t len, JNIEnv *env) {
    /* v2 帧优先：找 "v2\n" ... "\n" */
    char *p = strstr(buf, "v2\n");
    if (p) {
        char *nl = strchr(p + 3, '\n');
        if (nl) {
            char save = *nl; *nl = 0;
            int f[7];
            if (jq_bt_frame_decode(p, &f[0], &f[1], &f[2], &f[3], &f[4], &f[5], &f[6])) {
                if (g_frame_cb)
                    g_frame_cb(g_frame_user, env, f[0], f[1], f[2], f[3], f[4], f[5], f[6]);
            } else {
                LOGI("BT recv: 非 v2 帧丢弃: %.40s", p);
            }
            *nl = save;
            return (int)(nl + 1 - buf);
        }
    }
    /* C 行：行首 'C'，至 '\n' */
    if (buf[0] == 'C') {
        char *nl = memchr(buf, '\n', len);
        if (nl) {
            char save = *nl; *nl = 0;
            int idx, rank, side, rev;
            if (jq_bt_cell_decode(buf, &idx, &rank, &side, &rev)) {
                if (g_frame_cb)
                    g_frame_cb(g_frame_user, env, JQ_BT_T_SYNC_CELL, idx, rank, side, rev, -1, -1);
            }
            *nl = save;
            return (int)(nl + 1 - buf);
        }
    }
    return 0;  /* 需更多数据 */
}

/* ---- reader 线程：常驻读帧，解析后回调 ---- */
static void *bt_read_thread(void *arg) {
    (void)arg;
    if (!g_vm) return NULL;
    JNIEnv *env = NULL;
    if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) != JNI_OK) return NULL;
    LOGI("BT reader 线程启动");
    pthread_mutex_lock(&g_bt_mutex);
    jobject in = g_in;   /* 不复制全局引用；reader 生命周期 < socket 生命周期 */
    pthread_mutex_unlock(&g_bt_mutex);
    if (!in) { (*g_vm)->DetachCurrentThread(g_vm); return NULL; }
    jclass is = (*env)->GetObjectClass(env, in);
    jmethodID read = (*env)->GetMethodID(env, is, "read", "([B)I");
    jbyteArray jbuf = (*env)->NewByteArray(env, JQ_BT_READ_BUF);
    char lbuf[JQ_BT_LINE_BUF];
    size_t llen = 0;
    int running = 1;
    while (running) {
        if (!read || !jbuf) break;
        jint n = (*env)->CallIntMethod(env, in, read, jbuf);
        if (jq_bt_exception(env)) {   /* 对端断开/IO 错误 */
            LOGI("BT reader: read 异常（对端断开）");
            break;
        }
        if (n <= 0) { LOGI("BT reader: EOF (n=%d)", (int)n); break; }
        if (llen + (size_t)n > sizeof(lbuf)) { llen = 0; }  /* 溢出防御：丢弃 */
        (*env)->GetByteArrayRegion(env, jbuf, 0, n, (jbyte *)(lbuf + llen));
        llen += (size_t)n;
        /* 消费所有完整帧 */
        int consumed;
        while (llen > 0 && (consumed = bt_consume(lbuf, llen, env)) > 0) {
            memmove(lbuf, lbuf + consumed, llen - (size_t)consumed);
            llen -= (size_t)consumed;
            if (consumed == 0) break;
        }
        if (!g_thread_running) running = 0;
    }
    if (jbuf) (*env)->DeleteLocalRef(env, jbuf);
    LOGI("BT reader 线程退出");
    /* 断开：关 socket，置 IDLE */
    pthread_mutex_lock(&g_bt_mutex);
    if (g_socket) {
        jclass scl = (*env)->GetObjectClass(env, g_socket);
        jmethodID close = (*env)->GetMethodID(env, scl, "close", "()V");
        if (close) (*env)->CallVoidMethod(env, g_socket, close);
        jq_bt_exception(env);
    }
    g_bt_state = JQ_BT_IDLE;
    g_read_started = 0;
    pthread_mutex_unlock(&g_bt_mutex);
    (*g_vm)->DetachCurrentThread(g_vm);
    return NULL;
}

/* ---- 内部：连接建立成功公共路径（存储流/状态，启动 reader） ---- */
static void bt_established(JNIEnv *env, jobject socket) {
    jclass scl = (*env)->GetObjectClass(env, socket);
    jmethodID getIn  = (*env)->GetMethodID(env, scl, "getInputStream", "()Ljava/io/InputStream;");
    jmethodID getOut = (*env)->GetMethodID(env, scl, "getOutputStream", "()Ljava/io/OutputStream;");
    jobject in  = getIn  ? (*env)->CallObjectMethod(env, socket, getIn)  : NULL;
    jobject out = getOut ? (*env)->CallObjectMethod(env, socket, getOut) : NULL;
    if (jq_bt_exception(env) || !in || !out) {
        LOGE("BT 连接建立失败：getInputStream/OutputStream 不可用");
        return;
    }
    pthread_mutex_lock(&g_bt_mutex);
    /* 清旧引用（防重连泄漏） */
    if (g_socket) { jclass c2 = (*env)->GetObjectClass(env, g_socket); jmethodID cl = (*env)->GetMethodID(env, c2, "close", "()V"); if (cl) (*env)->CallVoidMethod(env, g_socket, cl); (*env)->DeleteGlobalRef(env, g_socket); }
    if (g_in)   (*env)->DeleteGlobalRef(env, g_in);
    if (g_out)  (*env)->DeleteGlobalRef(env, g_out);
    g_socket = (*env)->NewGlobalRef(env, socket);
    g_in     = (*env)->NewGlobalRef(env, in);
    g_out    = (*env)->NewGlobalRef(env, out);
    g_bt_state = JQ_BT_CONNECTED;
    g_thread_running = 1;
    pthread_mutex_unlock(&g_bt_mutex);
    LOGI("BT 连接建立（RFCOMM）");
    /* 启动 reader（先于发送，避免对端立即回帧无消费） */
    pthread_create(&g_read_thread, NULL, bt_read_thread, NULL);
    /* 客户端（发起方）发全盘同步请求 */
    jq_bt_send_frame(JQ_BT_T_SYNC_REQ, -1, -1, -1, -1, -1, -1);
}

/* ---- connect 线程：客户端连设备 ---- */
static void *bt_connect_thread(void *arg) {
    jobject device = (jobject)arg;   /* 全局引用（由启动方建立） */
    if (!g_vm || !device) { if (device) (*g_vm)->DetachCurrentThread(g_vm); return NULL; }
    JNIEnv *env = NULL;
    if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) != JNI_OK) return NULL;
    LOGI("BT connect 线程启动");
    /* UUID.fromString */
    jclass ucl = (*env)->FindClass(env, "java/util/UUID");
    jmethodID fromStr = (*env)->GetStaticMethodID(env, ucl, "fromString",
                                                  "(Ljava/lang/String;)Ljava/util/UUID;");
    jstring ju = (*env)->NewStringUTF(env, JQ_BT_UUID);
    jobject uuid = fromStr ? (*env)->CallStaticObjectMethod(env, ucl, fromStr, ju) : NULL;
    (*env)->DeleteLocalRef(env, ju);
    jq_bt_exception(env);
    if (!uuid) { LOGE("BT connect: UUID 构造失败"); goto fail; }
    jclass dcl = (*env)->GetObjectClass(env, device);
    jmethodID create = (*env)->GetMethodID(env, dcl,
        "createRfcommSocketToServiceRecord",
        "(Ljava/util/UUID;)Landroid/bluetooth/BluetoothSocket;");
    jobject socket = create ? (*env)->CallObjectMethod(env, device, create, uuid) : NULL;
    (*env)->DeleteLocalRef(env, uuid);
    if (jq_bt_exception(env) || !socket) { LOGE("BT connect: createRfcommSocket 失败"); goto fail; }
    jclass scl = (*env)->GetObjectClass(env, socket);
    jmethodID connect = (*env)->GetMethodID(env, scl, "connect", "()V");
    (*env)->CallVoidMethod(env, socket, connect);
    if (jq_bt_exception(env)) {
        LOGE("BT connect: socket.connect 失败（设备未配对/不可达）");
        jmethodID cl = (*env)->GetMethodID(env, scl, "close", "()V");
        if (cl) (*env)->CallVoidMethod(env, socket, cl);
        jq_bt_exception(env);
        goto fail;
    }
    bt_established(env, socket);
    (*env)->DeleteLocalRef(env, socket);
    pthread_mutex_lock(&g_bt_mutex);
    g_bt_state = (g_bt_state == JQ_BT_CONNECTED) ? JQ_BT_CONNECTED : JQ_BT_ERROR;
    pthread_mutex_unlock(&g_bt_mutex);
fail:
    (*env)->DeleteGlobalRef(env, device);
    (*g_vm)->DetachCurrentThread(g_vm);
    return NULL;
}

/* ---- accept 线程：服务端监听 ---- */
static void *bt_accept_thread(void *arg) {
    (void)arg;
    if (!g_vm) return NULL;
    JNIEnv *env = NULL;
    if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) != JNI_OK) return NULL;
    LOGI("BT accept 线程启动（监听 %s）", JQ_BT_NAME);
    pthread_mutex_lock(&g_bt_mutex);
    jobject adapter = g_adapter;
    pthread_mutex_unlock(&g_bt_mutex);
    if (!adapter) { LOGE("BT accept: 无适配器"); goto out; }
    jclass ucl = (*env)->FindClass(env, "java/util/UUID");
    jmethodID fromStr = (*env)->GetStaticMethodID(env, ucl, "fromString",
                                                  "(Ljava/lang/String;)Ljava/util/UUID;");
    jstring ju = (*env)->NewStringUTF(env, JQ_BT_UUID);
    jobject uuid = fromStr ? (*env)->CallStaticObjectMethod(env, ucl, fromStr, ju) : NULL;
    (*env)->DeleteLocalRef(env, ju);
    jq_bt_exception(env);
    jclass acl = (*env)->GetObjectClass(env, adapter);
    jmethodID listen = (*env)->GetMethodID(env, acl,
        "listenUsingRfcommWithServiceRecord",
        "(Ljava/lang/String;Ljava/util/UUID;)Landroid/bluetooth/BluetoothServerSocket;");
    jstring jname = (*env)->NewStringUTF(env, JQ_BT_NAME);
    jobject server = (listen && uuid) ? (*env)->CallObjectMethod(env, adapter, listen, jname, uuid) : NULL;
    (*env)->DeleteLocalRef(env, jname);
    if (uuid) (*env)->DeleteLocalRef(env, uuid);
    if (jq_bt_exception(env) || !server) {
        LOGE("BT accept: listenUsingRfcommWithServiceRecord 失败");
        goto out;
    }
    jclass svcl = (*env)->GetObjectClass(env, server);
    jmethodID accept = (*env)->GetMethodID(env, svcl, "accept",
                                           "()Landroid/bluetooth/BluetoothSocket;");
    jmethodID closeSrv = (*env)->GetMethodID(env, svcl, "close", "()V");
    jobject socket = accept ? (*env)->CallObjectMethod(env, server, accept) : NULL;
    if (jq_bt_exception(env) || !socket) {
        LOGE("BT accept: accept 失败（超时/取消）");
        if (closeSrv) (*env)->CallVoidMethod(env, server, closeSrv);
        jq_bt_exception(env);
        goto out;
    }
    if (closeSrv) (*env)->CallVoidMethod(env, server, closeSrv);  /* 停止监听 */
    bt_established(env, socket);
    (*env)->DeleteLocalRef(env, socket);
    pthread_mutex_lock(&g_bt_mutex);
    g_bt_state = (g_bt_state == JQ_BT_CONNECTED) ? JQ_BT_CONNECTED : JQ_BT_ERROR;
    pthread_mutex_unlock(&g_bt_mutex);
out:
    (*g_vm)->DetachCurrentThread(g_vm);
    return NULL;
}

/* ---- 内部：注册 BtReceiver（原版不在 manifest 声明 <receiver>，native 动态注册）----
   原版 smali 只有 BtReceiver 类（BroadcastReceiver→App.OnBluetooth），manifest 无声明，
   故原版 32 位 native 经 Context.registerReceiver 动态注册。arm64 在 init 时注册，
   使 ACTION_FOUND/DISCOVERY/SCAN_MODE 广播到达 OnBluetooth → jq_bt_handle_intent。 */
static void bt_register_receiver(JNIEnv *env) {
    if (!g_activity) return;
    jclass rcls = (*env)->FindClass(env, "com/cnvcs/BtReceiver");
    if (!rcls) { jq_bt_exception(env); LOGE("BT 注册接收器: BtReceiver 类缺失"); return; }
    jmethodID rctor = (*env)->GetMethodID(env, rcls, "<init>", "()V");
    jobject recv = rctor ? (*env)->NewObject(env, rcls, rctor) : NULL;
    if (!recv) { jq_bt_exception(env); return; }
    jclass fcls = (*env)->FindClass(env, "android/content/IntentFilter");
    jmethodID fctor = (*env)->GetMethodID(env, fcls, "<init>", "()V");
    jmethodID addAction = (*env)->GetMethodID(env, fcls, "addAction",
                                              "(Ljava/lang/String;)V");
    jobject filter = fctor ? (*env)->NewObject(env, fcls, fctor) : NULL;
    if (filter && addAction) {
        /* 原版 action 清单（REVERSE_NOTES §91）：FOUND/SCAN_MODE_CHANGED/DISCOVERY_STARTED/FINISHED */
        const char *acts[] = {
            "android.bluetooth.device.action.FOUND",
            "android.bluetooth.adapter.action.SCAN_MODE_CHANGED",
            "android.bluetooth.adapter.action.DISCOVERY_STARTED",
            "android.bluetooth.adapter.action.DISCOVERY_FINISHED",
            "android.bluetooth.adapter.action.CONNECTION_STATE_CHANGED",
            "android.bluetooth.adapter.action.STATE_CHANGED",
            NULL
        };
        for (int i = 0; acts[i]; i++) {
            jstring js = (*env)->NewStringUTF(env, acts[i]);
            (*env)->CallVoidMethod(env, filter, addAction, js);
            (*env)->DeleteLocalRef(env, js);
        }
    }
    jclass actcl = (*env)->GetObjectClass(env, g_activity);
    jmethodID reg = (*env)->GetMethodID(env, actcl, "registerReceiver",
        "(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/content/Intent;");
    if (reg && filter) {
        (*env)->CallObjectMethod(env, g_activity, reg, recv, filter);
        jq_bt_exception(env);
        LOGI("BT 注册接收器: BtReceiver 已注册（广播→OnBluetooth）");
    } else {
        LOGI("BT 注册接收器: registerReceiver 缺失（走 OnEvent 驱动路径）");
    }
}

/* ---- OnCreate 初始化 ---- */
void jq_bt_init(JNIEnv *env, jobject activity) {
    if (activity && !g_activity)
        g_activity = (*env)->NewGlobalRef(env, activity);
    /* 注册 BtReceiver（原版 native 动态注册，广播→OnBluetooth→jq_bt_handle_intent） */
    bt_register_receiver(env);
    jclass acl = (*env)->FindClass(env, "android/bluetooth/BluetoothAdapter");
    if (!acl) { LOGE("BT init: BluetoothAdapter 类缺失（无蓝牙）"); return; }
    jmethodID gda = (*env)->GetStaticMethodID(env, acl, "getDefaultAdapter",
                                              "()Landroid/bluetooth/BluetoothAdapter;");
    jobject ad = gda ? (*env)->CallStaticObjectMethod(env, acl, gda) : NULL;
    jq_bt_exception(env);
    if (ad) {
        g_adapter = (*env)->NewGlobalRef(env, ad);
        LOGI("BT init: 默认适配器 OK");
    } else {
        LOGI("BT init: getDefaultAdapter=null（无蓝牙硬件/未开启）");
    }
}

/* ---- OnDestroy / 断开 ---- */
void jq_bt_shutdown(JNIEnv *env) {
    pthread_mutex_lock(&g_bt_mutex);
    g_thread_running = 0;
    if (g_socket && env) {
        jclass scl = (*env)->GetObjectClass(env, g_socket);
        jmethodID close = (*env)->GetMethodID(env, scl, "close", "()V");
        if (close) (*env)->CallVoidMethod(env, g_socket, close);
        jq_bt_exception(env);
    }
    if (g_socket) { (*env)->DeleteGlobalRef(env, g_socket); g_socket = NULL; }
    if (g_in)     { (*env)->DeleteGlobalRef(env, g_in);     g_in     = NULL; }
    if (g_out)    { (*env)->DeleteGlobalRef(env, g_out);    g_out    = NULL; }
    g_bt_state = JQ_BT_IDLE;
    g_read_started = 0;
    pthread_mutex_unlock(&g_bt_mutex);
    LOGI("BT 关闭");
}

/* ---- 请求开启蓝牙（startActivityForResult ACTION_REQUEST_ENABLE） ---- */
int jq_bt_request_enable(JNIEnv *env) {
    if (!g_activity || !g_adapter) return -1;
    jclass acl = (*env)->GetObjectClass(env, g_adapter);
    jfieldID f = (*env)->GetStaticFieldID(env, acl, "ACTION_REQUEST_ENABLE",
                                          "Ljava/lang/String;");
    jstring jact = f ? (jstring)(*env)->GetStaticObjectField(env, acl, f)
                     : (*env)->NewStringUTF(env, "android.bluetooth.adapter.action.REQUEST_ENABLE");
    jclass icl = (*env)->FindClass(env, "android/content/Intent");
    jmethodID ictor = (*env)->GetMethodID(env, icl, "<init>", "(Ljava/lang/String;)V");
    jobject intent = ictor ? (*env)->NewObject(env, icl, ictor, jact) : NULL;
    if (f && jact) (*env)->DeleteLocalRef(env, jact);
    jclass actcl = (*env)->GetObjectClass(env, g_activity);
    jmethodID safr = (*env)->GetMethodID(env, actcl, "startActivityForResult",
                                         "(Landroid/content/Intent;I)V");
    if (safr && intent) {
        (*env)->CallVoidMethod(env, g_activity, safr, intent, (jint)JQ_BT_REQ_ENABLE);
        jq_bt_exception(env);
        LOGI("BT 请求开启蓝牙");
        return 0;
    }
    return -1;
}

/* ---- 请求可发现（ACTION_REQUEST_DISCOVERABLE + 时长 extra） ---- */
int jq_bt_request_discoverable(JNIEnv *env, int seconds) {
    if (!g_activity || !g_adapter) return -1;
    if (seconds <= 0) seconds = 120;
    jclass acl = (*env)->GetObjectClass(env, g_adapter);
    jfieldID fa = (*env)->GetStaticFieldID(env, acl, "ACTION_REQUEST_DISCOVERABLE",
                                           "Ljava/lang/String;");
    jstring jact = fa ? (jstring)(*env)->GetStaticObjectField(env, acl, fa)
                      : (*env)->NewStringUTF(env, "android.bluetooth.adapter.action.REQUEST_DISCOVERABLE");
    jfieldID fd = (*env)->GetStaticFieldID(env, acl, "EXTRA_DISCOVERABLE_DURATION",
                                           "Ljava/lang/String;");
    jstring jdur = fd ? (jstring)(*env)->GetStaticObjectField(env, acl, fd)
                      : (*env)->NewStringUTF(env, "android.bluetooth.adapter.extra.DISCOVERABLE_DURATION");
    jclass icl = (*env)->FindClass(env, "android/content/Intent");
    jmethodID ictor = (*env)->GetMethodID(env, icl, "<init>", "(Ljava/lang/String;)V");
    jmethodID putExtra = (*env)->GetMethodID(env, icl, "putExtra",
                                             "(Ljava/lang/String;I)Landroid/content/Intent;");
    jobject intent = ictor ? (*env)->NewObject(env, icl, ictor, jact) : NULL;
    if (putExtra && intent && jdur)
        intent = (*env)->CallObjectMethod(env, intent, putExtra, jdur, (jint)seconds);
    if (fa && jact) (*env)->DeleteLocalRef(env, jact);
    if (fd && jdur) (*env)->DeleteLocalRef(env, jdur);
    jclass actcl = (*env)->GetObjectClass(env, g_activity);
    jmethodID safr = (*env)->GetMethodID(env, actcl, "startActivityForResult",
                                         "(Landroid/content/Intent;I)V");
    if (safr && intent) {
        (*env)->CallVoidMethod(env, g_activity, safr, intent, (jint)JQ_BT_REQ_DISCOVERABLE);
        jq_bt_exception(env);
        LOGI("BT 请求可发现 %d 秒", seconds);
        return 0;
    }
    return -1;
}

/* ---- 服务端：监听接受线程 ---- */
int jq_bt_start_host(JNIEnv *env) {
    (void)env;
    if (!g_adapter) { LOGE("BT host: 无适配器"); return -1; }
    pthread_mutex_lock(&g_bt_mutex);
    if (g_bt_state == JQ_BT_CONNECTED) { pthread_mutex_unlock(&g_bt_mutex); LOGI("BT host: 已连接"); return -1; }
    g_bt_state = JQ_BT_LISTENING;
    g_thread_running = 1;
    pthread_mutex_unlock(&g_bt_mutex);
    if (pthread_create(&g_io_thread, NULL, bt_accept_thread, NULL) != 0) {
        pthread_mutex_lock(&g_bt_mutex); g_bt_state = JQ_BT_ERROR; pthread_mutex_unlock(&g_bt_mutex);
        return -1;
    }
    LOGI("BT host: 监听线程启动");
    return 0;
}

/* ---- 客户端：开始发现 ---- */
int jq_bt_start_discovery(JNIEnv *env) {
    if (!g_adapter) return -1;
    jclass acl = (*env)->GetObjectClass(env, g_adapter);
    jmethodID sd = (*env)->GetMethodID(env, acl, "startDiscovery", "()Z");
    jboolean ok = sd ? (*env)->CallBooleanMethod(env, g_adapter, sd) : JNI_FALSE;
    jq_bt_exception(env);
    LOGI("BT 开始发现: %s", ok ? "ok" : "fail");
    return ok ? 0 : -1;
}

/* ---- 内部：连 BluetoothDevice（启动 connect 线程） ---- */
int jq_bt_connect_device(JNIEnv *env, jobject device) {
    if (!device) return -1;
    pthread_mutex_lock(&g_bt_mutex);
    if (g_bt_state == JQ_BT_CONNECTED) { pthread_mutex_unlock(&g_bt_mutex); LOGI("BT connect: 已连接"); return -1; }
    g_bt_state = JQ_BT_CONNECTING;
    g_thread_running = 1;
    pthread_mutex_unlock(&g_bt_mutex);
    jobject gdev = (*env)->NewGlobalRef(env, device);
    if (pthread_create(&g_io_thread, NULL, bt_connect_thread, gdev) != 0) {
        (*env)->DeleteGlobalRef(env, gdev);
        pthread_mutex_lock(&g_bt_mutex); g_bt_state = JQ_BT_ERROR; pthread_mutex_unlock(&g_bt_mutex);
        return -1;
    }
    LOGI("BT connect 线程启动");
    return 0;
}

/* ---- 按地址连已配对设备（getBondedDevices 遍历） ---- */
int jq_bt_connect_addr(JNIEnv *env, const char *addr) {
    if (!g_adapter || !addr || !*addr) return -1;
    jclass acl = (*env)->GetObjectClass(env, g_adapter);
    jmethodID gbd = (*env)->GetMethodID(env, acl, "getBondedDevices", "()Ljava/util/Set;");
    jobject set = gbd ? (*env)->CallObjectMethod(env, g_adapter, gbd) : NULL;
    jq_bt_exception(env);
    if (!set) { LOGI("BT connect_addr: 无已配对设备"); return -1; }
    jclass setcl = (*env)->GetObjectClass(env, set);
    jmethodID iter = (*env)->GetMethodID(env, setcl, "iterator", "()Ljava/util/Iterator;");
    jobject it = iter ? (*env)->CallObjectMethod(env, set, iter) : NULL;
    int rc = -1;
    if (it) {
        jclass itcl = (*env)->GetObjectClass(env, it);
        jmethodID hasNext = (*env)->GetMethodID(env, itcl, "hasNext", "()Z");
        jmethodID next = (*env)->GetMethodID(env, itcl, "next", "()Ljava/lang/Object;");
        while (hasNext && next && (*env)->CallBooleanMethod(env, it, hasNext)) {
            jobject dev = (*env)->CallObjectMethod(env, it, next);
            if (!dev) continue;
            jclass dcl = (*env)->GetObjectClass(env, dev);
            jmethodID ga = (*env)->GetMethodID(env, dcl, "getAddress", "()Ljava/lang/String;");
            jstring ja = ga ? (jstring)(*env)->CallObjectMethod(env, dev, ga) : NULL;
            char *a = bt_jstr(env, ja);
            if (a && strcmp(a, addr) == 0) {
                rc = jq_bt_connect_device(env, dev);
                free(a);
                (*env)->DeleteLocalRef(env, dev);
                break;
            }
            if (a) free(a);
            (*env)->DeleteLocalRef(env, dev);
        }
    }
    if (rc != 0) LOGI("BT connect_addr: %s 未找到/连接失败", addr);
    return rc;
}

/* ---- OnBluetooth(Intent) 派发 ---- */
void jq_bt_handle_intent(JNIEnv *env, jobject intent) {
    if (!intent) return;
    jclass cls = (*env)->GetObjectClass(env, intent);
    jmethodID getAction = (*env)->GetMethodID(env, cls, "getAction", "()Ljava/lang/String;");
    jmethodID getParcelable = (*env)->GetMethodID(env, cls, "getParcelableExtra",
                                                  "(Ljava/lang/String;)Landroid/os/Parcelable;");
    jmethodID getIntExtra = (*env)->GetMethodID(env, cls, "getIntExtra",
                                                "(Ljava/lang/String;I)I");
    jstring jaction = getAction ? (jstring)(*env)->CallObjectMethod(env, intent, getAction) : NULL;
    char *action = bt_jstr(env, jaction);
    if (!action) return;
    if (strstr(action, "FOUND")) {
        /* 发现设备：EXTRA_DEVICE → BluetoothDevice → 连接 */
        if (getParcelable) {
            jstring key = (*env)->NewStringUTF(env, "android.bluetooth.device.extra.DEVICE");
            jobject dev = (*env)->CallObjectMethod(env, intent, getParcelable, key);
            (*env)->DeleteLocalRef(env, key);
            if (dev) {
                jclass dcl = (*env)->GetObjectClass(env, dev);
                jmethodID ga = (*env)->GetMethodID(env, dcl, "getAddress", "()Ljava/lang/String;");
                jstring ja = ga ? (jstring)(*env)->CallObjectMethod(env, dev, ga) : NULL;
                char *a = bt_jstr(env, ja);
                LOGI("BT FOUND dev=%s → 连接", a ? a : "?");
                if (a) free(a);
                jq_bt_connect_device(env, dev);
                (*env)->DeleteLocalRef(env, dev);
            }
        }
    } else if (strstr(action, "SCAN_MODE_CHANGED")) {
        /* 可发现状态变化：若进入 SCAN_MODE_CONNECTABLE_DISCOVERABLE(3)，服务端可接受连接 */
        jstring key = (*env)->NewStringUTF(env, "android.bluetooth.adapter.extra.SCAN_MODE");
        int mode = getIntExtra ? (*env)->CallIntMethod(env, intent, getIntExtra, key, (jint)0) : -1;
        (*env)->DeleteLocalRef(env, key);
        jq_bt_exception(env);
        LOGI("BT SCAN_MODE_CHANGED mode=%d", mode);
        if (mode == 3 && g_bt_state == JQ_BT_IDLE) {
            LOGI("BT 可发现 → 自动启动服务端监听");
            jq_bt_start_host(env);
        }
    } else if (strstr(action, "DISCOVERY_STARTED")) {
        LOGI("BT DISCOVERY_STARTED");
    } else if (strstr(action, "DISCOVERY_FINISHED")) {
        LOGI("BT DISCOVERY_FINISHED");
    } else if (strstr(action, "CONNECTION_STATE_CHANGED")) {
        LOGI("BT CONNECTION_STATE_CHANGED");
    } else {
        LOGI("BT action=%s", action);
    }
    free(action);
}
