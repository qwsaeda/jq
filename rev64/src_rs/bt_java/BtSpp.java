package com.cnvcs;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.util.Log;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;

/**
 * BtSpp — 蓝牙联机 SPP socket 宿主（Java 侧，诚实边界：无真机蓝牙验证）。
 *
 * 职责：联网=主机（accept 入连）/ 联机=客户端（connect 对端）+ 读线程喂 App.OnBluetooth +
 *       sendFrame 写出。帧协议 = v2 7 字段（\n 结尾，bt_protocol.c 编解码）。
 *
 * 安全：全部 try/catch（BT 不可用/未使能/连接失败 → 日志 + 回调失败，不崩 app）；
 *       读线程 daemon。UUID 对齐 bt_protocol.c（ce1404fe-0863-4278-86c2-c9c560f0e215）。
 */
public class BtSpp {
    private static final String TAG = "junqi_bt";
    private static final UUID SPP_UUID =
            UUID.fromString("ce1404fe-0863-4278-86c2-c9c560f0e215");
    private static final String SERVICE = "junqi";

    private static BluetoothSocket s_socket;
    private static OutputStream s_out;
    private static Thread s_readThread;
    private static boolean s_connected = false;

    /** 主机：listen + accept（阻塞在后台线程）。回调 App.btState("host_started")。 */
    public static void startHost() {
        try {
            BluetoothAdapter a = BluetoothAdapter.getDefaultAdapter();
            if (a == null || !a.isEnabled()) { fail("BT 不可用或未使能"); return; }
            final BluetoothServerSocket server =
                    a.listenUsingRfcommWithServiceRecord(SERVICE, SPP_UUID);
            Log.i(TAG, "host：监听 " + SPP_UUID);
            Thread t = new Thread(new Runnable() {
                public void run() {
                    try {
                        BluetoothSocket s = server.accept();
                        server.close();
                        attach(s);
                        App.btState("host_connected");
                    } catch (Exception e) { serverClose(server); fail("host accept 失败: " + e); }
                }
            });
            t.setDaemon(true); t.start();
        } catch (Exception e) { fail("host 启动失败: " + e); }
    }

    /** 客户端：连接对端地址（后台线程）。addr 为空/空串 → 自动用第一个已配对设备。 */
    public static void startClient(final String addr) {
        try {
            BluetoothAdapter a = BluetoothAdapter.getDefaultAdapter();
            if (a == null || !a.isEnabled()) { fail("BT 不可用或未使能"); return; }
            String target = addr;
            if (target == null || target.length() == 0) {
                java.util.Set<android.bluetooth.BluetoothDevice> bonded = a.getBondedDevices();
                if (bonded == null || bonded.isEmpty()) { fail("无已配对设备，请先在系统蓝牙配对"); return; }
                target = bonded.iterator().next().getAddress();
                Log.i(TAG, "client：自动选已配对设备 " + target);
            }
            final BluetoothSocket s =
                    a.getRemoteDevice(target).createRfcommSocketToServiceRecord(SPP_UUID);
            Log.i(TAG, "client：连接 " + target);
            Thread t = new Thread(new Runnable() {
                public void run() {
                    try {
                        s.connect();
                        attach(s);
                        App.btState("client_connected");
                    } catch (Exception e) { try { s.close(); } catch (Exception e2) {} fail("client 连接失败: " + e); }
                }
            });
            t.setDaemon(true); t.start();
        } catch (Exception e) { fail("client 启动失败: " + e); }
    }

    private static void attach(BluetoothSocket s) {
        try {
            s_socket = s;
            s_out = s.getOutputStream();
            s_connected = true;
            startRead(s);
        } catch (Exception e) { fail("attach 失败: " + e); }
    }

    /** 读线程：按原版帧格式重组帧 → Intent extra → App.OnBluetooth（复用原生接收路径）。
     *
     * ★★ 对抗审计 P0 修复（批6）：原实现"按 \n 逐行分发"与 native 帧格式不匹配 ——
     *   native 帧 = "v2\n" + payload + "\n"（两行：首行版本、次行 7 字段，
     *   原版确证 0x26efc 构造 "v2\n" + sprintf 0x120e94），逐行分发会把
     *   "v2" 和 "payload" 当成两个独立帧分别下发 → native jq_bt_frame_decode
     *   找不到 "v2\n" 前缀 → **收帧全丢**（端到端蓝牙同步失效）。
     *   隐患：native 端 encode→decode 自测（bt_selftest/bt_semtest）两端自洽，
     *   永远发现不了这个跨语言契约错位。
     *   现按帧格式重组：需 "v2\n" 头 + payload + 尾 '\n' 才算一帧；
     *   支持一次读到多帧（逐帧抽取，缓冲剩余部分）→ 多帧载荷（60 格 SYNC_CELL
     *   批量同步）也能正确拆分。
     */
    private static void startRead(final BluetoothSocket s) {
        s_readThread = new Thread(new Runnable() {
            public void run() {
                StringBuilder buf = new StringBuilder();
                try {
                    InputStream in = s.getInputStream();
                    byte[] tmp = new byte[512];
                    int n;
                    while ((n = in.read(tmp)) > 0) {
                        for (int i = 0; i < n; i++) {
                            char c = (char) tmp[i];
                            if (c != '\r') buf.append(c);
                        }
                        /* 逐帧抽取：buf 内可能有多帧（v2\npayload\nv2\npayload\n） */
                        int vs;
                        while ((vs = buf.indexOf("v2\n")) >= 0) {
                            int pe = buf.indexOf("\n", vs + 3);   /* payload 终止符 */
                            if (pe < 0) break;                     /* 帧未收全，等下次读 */
                            String frame = buf.substring(vs, pe);
                            buf.delete(0, pe + 1);
                            if (frame.length() > 0) {
                                Intent it = new Intent();
                                it.putExtra("extra", frame);
                                App.OnBluetooth(it);
                            }
                        }
                        /* 噪声保护：无 "v2\n" 头且缓冲过长 → 丢弃（防无限增长） */
                        if (buf.length() > 0 && buf.indexOf("v2\n") < 0 && buf.length() > 256) {
                            buf.setLength(0);
                        }
                    }
                } catch (Exception e) { /* 断开 */ }
                s_connected = false;
                App.btState("disconnected");
            }
        });
        s_readThread.setDaemon(true);
        s_readThread.start();
    }

    /** 写出帧（native 发送侧经 App.btSend → 此处）。 */
    public static void sendFrame(String frame) {
        try {
            if (!s_connected || s_out == null) { fail("sendFrame：未连接"); return; }
            s_out.write((frame + "\n").getBytes("UTF-8"));
            s_out.flush();
            Log.i(TAG, "sendFrame: " + frame);
        } catch (Exception e) { fail("sendFrame 失败: " + e); }
    }

    /** 断开（退出联机时调）。 */
    public static void disconnect() {
        s_connected = false;
        try { if (s_socket != null) s_socket.close(); } catch (Exception e) {}
        s_socket = null; s_out = null;
        App.btState("disconnected");
    }

    private static void serverClose(BluetoothServerSocket s) {
        try { if (s != null) s.close(); } catch (Exception e) {}
    }

    private static void fail(String msg) {
        Log.e(TAG, msg);
        App.btState("failed:" + msg);
    }
}
