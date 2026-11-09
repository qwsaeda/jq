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

    /* ★ AUD-1000 B09-018（P1 已修）：监听 socket 提为静态字段（旧版为
       startHost 局部变量，disconnect 无法关闭监听端口） */
    private static android.bluetooth.BluetoothServerSocket s_server;
    private static BluetoothSocket s_socket;
    private static OutputStream s_out;
    private static Thread s_readThread;
    private static boolean s_connected = false;

    /** 主机：listen + accept（阻塞在后台线程）。回调 App.btState("host_started")。 */
    public static void startHost() {
        try {
            BluetoothAdapter a = BluetoothAdapter.getDefaultAdapter();
            if (a == null || !a.isEnabled()) { fail("BT 不可用或未使能"); return; }
            s_server = a.listenUsingRfcommWithServiceRecord(SERVICE, SPP_UUID);  // 静态监听 socket（disconnect 可关）
            final BluetoothServerSocket server = s_server;
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
            synchronized (BtSpp.class) {
                s_socket = s;
                s_out = s.getOutputStream();
                s_connected = true;   /* ★ AUD-1000-ALL A2-011（P1 已修）：共享字段写侧 synchronized（4 线程：accept/connect/读线程/sendFrame），旧版裸写与 sendFrame 读 s_connected/s_out TOCTOU 丢帧→双端棋盘发散 */
            }
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
                        /* 噪声保护：无 "v2\n" 头且缓冲过长 → 丢弃（防无限增长）
                           ★ AUD-1000 B09-013（P1 已修）：半帧悬挂绝对上限——payload
                           尾 '\n' 未到时 buf 保留半帧等下次读，若对端只发半帧断连，
                           缓冲随读无限增长。32KB 上限（单帧最大 = 60 格 SYNC_CELL
                           串接 ~6KB，5 倍余量）。 */
                        if (buf.length() > 0 && buf.indexOf("v2\n") < 0 && buf.length() > 256) {
                            buf.setLength(0);
                        }
                        if (buf.length() > 32768) {
                            Log.w(TAG, "帧重组缓冲超 32KB（半帧悬挂增长）→ 清空");
                            buf.setLength(0);
                        }
                    }
                } catch (Exception e) {
                    /* ★ AUD-1000 B09-012（P1 已修）：读线程异常（含 IOException 断连）
                       补日志归因（旧空 catch 吞掉所有异常，断连原因不可诊断）。 */
                    Log.e(TAG, "读线程异常: " + e);
                }
                synchronized (BtSpp.class) { s_connected = false; }   /* ★ A2-011：写侧同锁 */
                App.btState("disconnected");
            }
        });
        s_readThread.setDaemon(true);
        s_readThread.start();
    }

    /** 写出帧（native 发送侧经 App.btSend → 此处）。
        ★ AUD-1000 B09-016（P1 已修）：帧自带尾 '\n'（native jq_bt_frame_encode
        产出 "v2\npayload\n" 两行格式，跨语言契约测试 bt_selftest §9 实证），
        旧实现再追加 '\n' → 双尾（对端重组靠 payload 内首 '\n' 定界不受影响，
        但帧尾多 1 空行污染对端缓冲 + 噪声保护 256B 阈值被双尾帧稀释）。
        修：追加前判帧是否已含尾 '\n'，含则直写。 */
    public static void sendFrame(String frame) {
        try {
            synchronized (BtSpp.class) {
                if (!s_connected || s_out == null) { fail("sendFrame：未连接"); return; }
                String out = frame.endsWith("\n") ? frame : frame + "\n";
                s_out.write(out.getBytes("UTF-8"));
                s_out.flush();   /* ★ A2-011（P1 已修）：写侧同锁（防 disconnect 并发置 null） */
            }
            Log.i(TAG, "sendFrame: " + frame);
        } catch (Exception e) { fail("sendFrame 失败: " + e); }
    }

    /** 断开（退出联机时调）。
        ★ AUD-1000 B09-018（P1 已修）：startHost 的 BluetoothServerSocket 在
        Runnable 局部（accept 阻塞持有），旧 disconnect 只 close 已接受的
        s_socket——监听 socket 永不关，重开 host 前旧监听占端口。提为静态
        字段 s_server，disconnect 时先 close 监听（accept 线程 IOException
        退出）+ interrupt 读线程（旧版读线程无 interrupt，socket close 后
        仍阻塞在 in.read 直到超时）。 */
    public static void disconnect() {
        synchronized (BtSpp.class) {   /* ★ A2-011（P1 已修）：disconnect 写侧同锁（防与 sendFrame/connect 并发读半置状态） */
            s_connected = false;
        }
        try { if (s_server != null) s_server.close(); } catch (Exception e) {}
        s_server = null;
        synchronized (BtSpp.class) {
            try { if (s_socket != null) s_socket.close(); } catch (Exception e) {}
            s_socket = null; s_out = null;
        }
        if (s_readThread != null) s_readThread.interrupt();
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
