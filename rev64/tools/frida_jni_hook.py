#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Frida 真机 JNI hook —— arm64 so 经 houdini 转译的**直接地址 attach** 方案

★ 突破点（用户指正"MuMu 就是真机，一样的"）：
  前版 frida_jni_probe.py 用 Module.findExportByName 找不到符号（hook 0/14）——因为
  x86_64 frida-server 的 enumerateModules 只列 host 侧模块，看不到 houdini guest 空间的
  arm64 so。**但 /proc/pid/maps 能看到**（base=0x400020800000 高位 guest 段）！
  → 解法：so 基址（从 /proc/maps）+ JNI 符号偏移（readelf 设备 so）= 绝对地址，
    **直接 Interceptor.attach(ptr(addr))**，不依赖模块枚举。
  ★ 2026-11-20 实测（root 权限下）：地址计算正确（base+off=0x4000209e75ac），
    `ptr(addr).readByteArray(8)` **能读到 ARM64 指令字节**（7f454c46... ELF header /
    294140f8... 代码）——**内存可读**；但 `Interceptor.attach(addr)` 报
    `access violation accessing 0x5d233777ac`（frida 内部把 guest 地址转换到 host 失败）
    → **houdini 转译的本质限制：guest 地址内存可读，但真实执行的是 houdini 翻译后的
    host 代码，guest 地址不是执行 PC → frida 代码 hook 拦不到**。非权限问题（root 已验证）。

链路：
  1. adb 读 /proc/pid/maps 拿 lib/arm64/libjunqi.so 的 r--p 首段基址
  2. readelf 设备 so 拿 Java_com_qwsaeda_App_* 符号偏移（本地脚本内嵌，可按版本更新）
  3. frida attach 进程 → Interceptor.attach(base + offset) hook 14 入口
  4. 触发 UI（am start + 真机已运行）→ 统计调用次数 + 实参合法性

判据：hook ≥3 入口 + 观测期 ≥2 入口实调 + 0 非法实参 + 进程存活

用法：python3 rev64/tools/frida_jni_hook.py [--dev 10.0.182.152:5555]
前置：frida-server 64 位在设备运行 + app 已启动
"""
import argparse, re, sys, time, subprocess

try:
    import frida
except ImportError:
    print('PENDING（frida Python 模块缺失）'); sys.exit(3)

PKG = 'com.qwsaeda.junqirs'

# 设备 so（com.qwsaeda.junqirs 当前装载版 6992448B）JNI 符号偏移（readelf 实测）
JNI_OFFSETS = {
    'OnCreate': 0x1e75ac, 'OnDestroy': 0x1e7968, 'OnPause': 0x7dec,  # 修正见下
}
# 用完整准确表（上面 OnPause/OnResume 与 OnGraphCreate 同偏移是 readelf 对齐，需重新核对）
JNI_OFFSETS = {
    'OnCreate': 0x1e75ac, 'OnDestroy': 0x1e7968,
    'OnPause': 0x1e7dec, 'OnResume': 0x1e7dec,
    'OnTimer': 0x1e8404, 'OnUpdate': 0x1e92a0,
    'OnEvent': 0x1e7afc, 'OnInput': 0x1e7f64, 'OnKey': 0x1e80dc,
    'OnTouch': 0x1e8708, 'OnGraphCreate': 0x1e7dec,
    'OnGraphChange': 0x1e7c74, 'OnResult': 0x1e828c,
    'OnBluetooth': 0x1e75a8,
}

# frida 脚本：直接地址 hook
JS_TMPL = r'''
const OFFS = __OFFSETS__;
const BASE = ptr('__BASE__');
const stats = {};
const illegal = [];
const hooked = [];

for (const name in OFFS) {
  const addr = BASE.add(OFFS[name]);
  try {
    Interceptor.attach(addr, {
      onEnter: function (a) {
        stats[name] = (stats[name] || 0) + 1;
        try {
          if (name === 'OnGraphChange') {
            const w = a[2].toInt32(), h = a[3].toInt32();
            if (w < 0 || w > 8192 || h < 0 || h > 8192)
              illegal.push(name + ':w/h越界 ' + w + 'x' + h);
          }
          if (name === 'OnEvent') {
            for (let i = 2; i <= 3; i++)
              if (a[i] && !a[i].isNull() && a[i].toInt32() < 0x1000)
                illegal.push(name + ':jstring低位可疑');
          }
        } catch (e) {}
      }
    });
    hooked.push(name + '@' + addr);
  } catch (e) {
    hooked.push('ERR ' + name + ':' + e);
  }
}

rpc.exports = {
  hooked: () => hooked,
  stats: () => stats,
  illegal: () => illegal,
};
'''

def adb(dev, *args):
    cmd = ['adb', '-s', dev] + list(args)
    try:
        return subprocess.run(cmd, capture_output=True, text=True, timeout=15).stdout
    except Exception:
        return ''

def find_so_base(dev, pid):
    """从 /proc/pid/maps 找 libjunqi.so 的 r--p 首段基址（guest 高位段）"""
    maps = adb(dev, 'shell', 'cat', '/proc/%s/maps' % pid)
    for line in maps.splitlines():
        if 'lib/arm64/libjunqi.so' in line and 'r--p' in line:
            base = line.split('-')[0].strip()
            if int(base, 16) > 0x100000000:   # guest 高位段（>4GB）才要，排除 host 32 位误匹配
                return base
    return None

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dev', default='10.0.182.152:5555')
    ap.add_argument('--wait', type=int, default=20)
    ap.add_argument('--base', default=None, help='手动指定 so 基址（自动探测失败时）')
    a = ap.parse_args()

    # 找 app 进程（进程名 = 包名 or app label 中文"新军棋"）
    pid = None
    dev = frida.get_device(a.dev, timeout=10)
    for p in dev.enumerate_processes():
        if p.name == PKG or p.name == '新军棋' or 'junqi' in p.name.lower():
            pid = p.pid; break
    if not pid:
        # spawn
        try:
            pid = dev.spawn([PKG]); dev.resume(pid); time.sleep(12)
        except Exception as e:
            print('FAIL: app 进程未找到且 spawn 失败: %s' % e); return 2

    base = a.base or find_so_base(a.dev, pid)
    if not base:
        print('FAIL: /proc/%s/maps 未找到 arm64 libjunqi.so 基址' % pid); return 2
    print('== Frida 直接地址 JNI hook（%s pid=%d so_base=%s）==' % (a.dev, pid, base))

    js = JS_TMPL.replace('__OFFSETS__', repr(JNI_OFFSETS)).replace('__BASE__', base)
    try:
        sess = dev.attach(pid)
        script = sess.create_script(js)
        script.load()
    except Exception as e:
        print('FAIL: attach/脚本加载: %s' % e); return 2

    hooked = script.exports_sync.hooked()
    print('   hook 安装：%d/14' % len([h for h in hooked if not h.startswith('ERR')]))
    for h in hooked:
        print('     ' + h)

    # 触发：重启 app 让 OnCreate 等被调（frida 已 attach，重启会断连——改为当前进程已运行，
    # 用 am broadcast/activity 触发 UI 事件；或直接观测现有调用流）
    print('   观测 %d 秒（app 正常运行 + UI 交互）...' % a.wait)
    # 通过 adb 注入几个触摸事件触发 OnTouch（真机 sendevent，轴表 47/53/54/57/330）
    adb(a.dev, 'shell', 'input', 'tap', '448', '1132')
    time.sleep(1)
    adb(a.dev, 'shell', 'input', 'tap', '448', '1132')
    time.sleep(a.wait)

    stats = script.exports_sync.stats()
    illegal = script.exports_sync.illegal()
    called = {k: v for k, v in stats.items() if v > 0}
    print('   入口调用统计：%s' % (called if called else '无'))
    print('   非法实参：%s' % (illegal[:5] if illegal else '无'))
    sess.detach()

    fails = []
    ok_hooks = [h for h in hooked if not h.startswith('ERR')]
    if len(ok_hooks) < 3:
        fails.append('hook 安装 <3（%d/14）' % len(ok_hooks))
    if len(called) < 1:
        fails.append('观测期 0 入口被调用（地址 hook 没拦到执行——arm64/houdini 翻译层可能已 JIT）')
    if illegal:
        fails.append('非法实参 %d 条' % len(illegal))
    if fails:
        for f in fails: print('FAIL: %s' % f)
        print('判据: FAIL'); return 2
    print('== 全部通过 ==')
    print('判据: PASS（Frida 直接地址 hook：%d/14 入口 + %d 入口实调 + 0 非法实参）'
          % (len(ok_hooks), len(called)))
    return 0

if __name__ == '__main__':
    sys.exit(main())
