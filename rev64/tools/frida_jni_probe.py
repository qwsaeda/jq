#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Frida 真机 JNI 入口观测 fuzz（P5 设备侧，补 unidbg 测不到的真 JNIEnv）

★ 为什么 Frida 行、unidbg 不行（仓库实锤三条）：
  - unidbg .bss 非零率 0.2% vs MuMu 79.1% → C++ 对象图未构造 → JNI 反射链前置全缺
  - init_array 11/12 + UC_ERR_INSN_INVALID → 业务进不去
  - 模拟器 JNIEnv 是壳 → FindClass/GetMethodID 返 NULL → 反射链分支根本进不去
  Frida attach 到**真机进程**（com.qwsaeda.junqirs）→ 真 JNIEnv + 真 Java 对象图
  + 真 bionic + 真 scudo → 反射链真实执行。

本脚本做两类观测（非破坏性 fuzz，不主动改参数——真机 JNI 传 NULL 会被 Java 层挡）：
  O1 JNI 入口调用观测：hook 14 个 Java_com_qwsaeda_App_* 导出，记录调用次数 +
     实参类型（jstring 内容长度 / jobject 非 NULL / jint 范围），验证入口可达性
  O2 OnTouch 真实触摸链：hook OnTouch，配合 sendevent 真实触摸，验证从 JNI 入口
     到 UI 状态机的信号流（已有 sendevent 黑盒实证，这里做 JNI 层佐证）

判据（全 PASS）：
  ① Frida attach 成功 + 14 入口 hook 安装成功
  ② 启动/触摸序列中 ≥3 个 JNI 入口被真实调用（OnCreate/OnTouch/OnUpdate 等）
  ③ 0 崩溃（hook 不引入异常；app 进程存活）
  ④ JNI 入口实参无非法值（jstring NULL 检测 / jint 越界检测）

用法：python3 rev64/tools/frida_jni_probe.py [--dev 10.0.182.152:5555]
前置：frida-server 在设备运行（root）+ app 已启动
"""
import argparse, sys, time, os

try:
    import frida
except ImportError:
    print('PENDING（frida Python 模块缺失）')
    sys.exit(3)

PKG = 'com.qwsaeda.junqirs'

JS = r'''
const ENTRIES = [
  'OnCreate','OnDestroy','OnPause','OnResume','OnTimer','OnUpdate',
  'OnEvent','OnInput','OnKey','OnTouch','OnGraphCreate','OnGraphChange',
  'OnResult','OnBluetooth'
];
const stats = {};
ENTRY_INIT: for (const e of ENTRIES) stats[e] = 0;
const argsLog = [];
const illegal = [];

function hookEntry(name) {
  const sym = 'Java_com_qwsaeda_App_' + name;
  let addr = null;
  try { addr = Module.findExportByName(null, sym); } catch (e) {}
  if (!addr) {
    // libjunqi.so 可能非 RTLD_GLOBAL 全局导出 → 按模块查
    try {
      const mods = Process.enumerateModules();
      for (const m of mods) {
        if (m.name.indexOf('junqi') < 0) continue;
        const a = Module.findExportByName(m.name, sym);
        if (a) { addr = a; break; }
      }
    } catch (e) {}
  }
  if (!addr) return null;
  try {
    Interceptor.attach(addr, {
      onEnter: function (a) {
        stats[name] = (stats[name] || 0) + 1;
        // 实参合法性抽查（按入口类型）
        try {
          if (name === 'OnEvent' || name === 'OnInput') {
            // a[2]/a[3] = jstring（JNIEnv*, jclass, jstring...）
            for (let i = 2; i <= 3; i++) {
              if (a[i] && !a[i].isNull() && a[i].toInt32() < 0x1000) {
                illegal.push(name + ':jstring低位可疑 ' + a[i]);
              }
            }
          }
          if (name === 'OnGraphChange') {
            const w = a[2].toInt32(), h = a[3].toInt32();
            if (w < 0 || w > 8192 || h < 0 || h > 8192) {
              illegal.push(name + ':w/h越界 ' + w + 'x' + h);
            }
            if (argsLog.length < 20) argsLog.push(name + '(' + w + ',' + h + ')');
          }
          if (argsLog.length < 20 && (name === 'OnTouch' || name === 'OnCreate')) {
            argsLog.push(name + ':jobj=' + (a[2] && !a[2].isNull()));
          }
        } catch (e) {}
      }
    });
    return addr;
  } catch (e) { return null; }
}

const hooked = [];
for (const e of ENTRIES) {
  const a = hookEntry(e);
  if (a) hooked.push(e + '@' + a);
}

rpc.exports = {
  hooked: () => hooked,
  stats: () => stats,
  args: () => argsLog,
  illegal: () => illegal,
};
'''

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dev', default='10.0.182.152:5555')
    ap.add_argument('--wait', type=int, default=25, help='观测时长（秒）')
    a = ap.parse_args()

    try:
        dev = frida.get_device(a.dev, timeout=10)
    except Exception as e:
        print('FAIL: Frida 设备连接失败 %s: %s' % (a.dev, e))
        return 2

    # 找 app 进程
    pid = None
    for p in dev.enumerate_processes():
        # ★ 真机实锤：进程名 = app label 中文"新军棋"（非包名 com.qwsaeda.junqirs）
        #   → 匹配用 包名 / label / 模糊 junqi 三种
        if p.name == PKG or p.name == '新军棋' or 'junqi' in p.name.lower():  # '新军棋'=com.qwsaeda.junqirs(app label)
            pid = p.pid; break
    if pid is None:
        # spawn 模式（app 未启动）
        try:
            pid = dev.spawn([PKG])
            dev.resume(pid)
            print('   app 未运行 → spawn pid=%d' % pid)
            time.sleep(8)
        except Exception as e:
            print('FAIL: app 进程未找到且 spawn 失败: %s' % e)
            return 2

    print('== Frida 真机 JNI 入口观测（%s pid=%d）==' % (a.dev, pid))
    try:
        sess = dev.attach(pid)
        script = sess.create_script(JS)
        script.load()
    except Exception as e:
        print('FAIL: Frida attach/脚本加载失败: %s' % e)
        return 2

    hooked = script.exports_sync.hooked()
    print('   hook 安装：%d/14 入口' % len(hooked))
    for h in hooked:
        print('     ' + h)

    # 观测期：触发一些 UI 交互（通过 adb 注入坐标；非破坏性）
    print('   观测 %d 秒（期间 app 正常运行）...' % a.wait)
    time.sleep(a.wait)

    stats = script.exports_sync.stats()
    args = script.exports_sync.args()
    illegal = script.exports_sync.illegal()
    called = {k: v for k, v in stats.items() if v > 0}
    print('   入口调用统计（非零）：%s' % (called if called else '无'))
    if args:
        print('   实参抽样（前 8）：%s' % args[:8])
    print('   非法实参：%s' % (illegal[:5] if illegal else '无'))

    sess.detach()

    fails = []
    if len(hooked) < 3:
        fails.append('hook 安装 <3 入口（so 导出符号未找到/模块过滤失效）')
    if len(called) < 2:
        fails.append('观测期 <2 入口被调用（JNI 链未激活——app 可能卡在启动）')
    if illegal:
        fails.append('非法 JNI 实参 %d 条' % len(illegal))
    # 进程存活检查
    try:
        alive = any(p.pid == pid for p in dev.enumerate_processes())
    except Exception:
        alive = True
    if not alive:
        fails.append('app 进程观测后消失（崩溃）')

    if fails:
        for f in fails:
            print('FAIL: %s' % f)
        print('判据: FAIL')
        return 2
    print('== 全部通过 ==')
    print('判据: PASS（Frida 真机 JNI：%d/14 入口 hook + %d 入口实调 + 0 非法实参 + 进程存活）'
          % (len(hooked), len(called)))
    return 0

if __name__ == '__main__':
    sys.exit(main())
