/*
 * bt_net.h — R6 蓝牙联机网络层（native JNI 直调 Java 蓝牙 API）
 *
 * 方案A（任务选定）：native JNI 直调 Java BluetoothAdapter/Device/Socket，
 * 最小 Java 改动（App.smali native 签名不变，红线）。
 *
 * 流程（REVERSE_NOTES §89/§91/§92 确证）：
 *   1. OnBluetooth(Intent) 广播 → jq_bt_handle_intent（getAction/EXTRA_DEVICE）
 *   2. ACTION_FOUND → BluetoothDevice → RFCOMM 连接线程（UUID ce1404fe + 服务名 junqi）
 *   3. 连接建立 → 帧收发线程（InputStream/OutputStream）→ v2 帧状态机 → g_state 同步
 *   4. 走子/翻棋 → jq_bt_send_frame 构造 v2 帧发送
 *
 * 线程模型：connect/accept 用阻塞 IO 放 pthread；reader 常驻读帧；
 * 帧应用经 jq_bt_frame_cb 回调到 junqi_jni.c（g_mutex 保护 g_state）。
 */
#ifndef BT_NET_H
#define BT_NET_H

#include <jni.h>
#include "junqi_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 注册 JavaVM（JNI_OnLoad 调用，供 attach 线程） */
void jq_bt_set_vm(JavaVM *vm);

/* 初始化（OnCreate：保存 activity + 获取默认蓝牙适配器） */
void jq_bt_init(JNIEnv *env, jobject activity);

/* 关闭（OnDestroy / 断开）：关 socket、停线程、清全局引用 */
void jq_bt_shutdown(JNIEnv *env);

/* 帧回调（junqi_jni.c 注册）：reader 解析 v2 帧/C 行后应用到 g_state。
   type = JQ_BT_T_*（SYNC_CELL 为 C 行伪类型），a..f 为帧字段。 */
typedef void (*jq_bt_frame_cb)(void *user, JNIEnv *env, int type,
                               int a, int b, int c, int d, int e, int f);
void jq_bt_set_frame_cb(jq_bt_frame_cb cb, void *user);

/* 是否已连接（OnUpdate 关 AI / 本地走子后发帧用） */
int jq_bt_connected(void);

/* 发送语义帧（type + 6 字段，jq_bt_frame_encode 编码后经 OutputStream 写） */
int jq_bt_send_frame(int type, int a, int b, int c, int d, int e, int f);

/* 发送全盘同步（SYNC 帧：c=stage,d=turn + 60 格 C 行） */
int jq_bt_send_sync(JNIEnv *env, const jq_state *st);

/* OnBluetooth(Intent) 派发：action → 状态机（FOUND→连接 / SCAN_MODE→监听等） */
void jq_bt_handle_intent(JNIEnv *env, jobject intent);

/* OnEvent bt_* 命令（arm64 简化版无完整 UI，经事件驱动联机流程） */
int jq_bt_request_enable(JNIEnv *env);
int jq_bt_request_discoverable(JNIEnv *env, int seconds);
int jq_bt_start_host(JNIEnv *env);       /* 服务端：listenUsingRfcomm 接受线程 */
int jq_bt_start_discovery(JNIEnv *env);  /* 客户端：adapter.startDiscovery */
int jq_bt_connect_addr(JNIEnv *env, const char *addr);   /* 按地址连已配对设备 */
int jq_bt_connect_device(JNIEnv *env, jobject device);   /* 连 BluetoothDevice */

/* 状态字符串（日志） */
const char *jq_bt_state_str(void);

#ifdef __cplusplus
}
#endif
#endif /* BT_NET_H */
