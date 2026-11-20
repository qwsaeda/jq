#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""RegisterNatives hook（看雪文章方法论 2026-11-20）—— 纯 native 路径拿 so 函数真实地址

★ 来源：用户分享看雪学苑文章《记录第一次的 so 文件逆向分析》
  (https://mp.weixin.qq.com/s/rO80pkDap7ZptLNK-cd5cg)
  核心方法：Java bridge 不可用/被检测时，**不调 Java.perform**，改 hook libart 的
  `RegisterNatives`，从 JNINativeMethod 注册表直接拿 native 函数真实 fnPtr，再
  Interceptor.attach。绕开"模块枚举找不到符号"（正是我 guest so 不可见问题的正解方向）。

文章关键纪律（内嵌到脚本）：
  1. RegisterNatives 要在目标方法注册前挂上（spawn 或 app 未注册时 attach）
  2. JNINativeMethod 布局：{name_ptr, sig_ptr, fnPtr}，每项 3 个指针宽（ARM32=12B / ARM64=24B）
  3. ARM32 注册地址含 Thumb 位 → fn.and(0xfffffffe) 才是代码地址（不能用于 ARM64）
  4. jstring 不能 readUtf8String() → 必须 JNIEnv 函数表 GetStringUTFChars（索引 169）
     + ReleaseStringUTFChars（索引 170）配对；函数表索引 167/169/170，表头 4 reserved 计数
     - ARM32: 169*4=676 / ARM64: 169*8=1352
  5. 不靠函数名猜，靠注册表 + 运行日志确认映射

用法：python3 rev64/tools/frida_registernatives_hook.py [--dev 10.0.182.152:5555] [--wait 15]
前置：frida-server 在设备运行（本环境不稳，脚本写好后换稳定环境/时机可跑）
"""
import argparse, sys, time

try:
    import frida
except ImportError:
    print('PENDING（frida 模块缺失）'); sys.exit(3)

# 文章的纯 native JS（兼容新旧 Frida API + Thumb 去位 + JNI 函数表偏移）
JS = r'''
'use strict';
const hooked = {};

function readJniString(env, obj) {
  if (obj.isNull()) return '<null>';
  const table = env.readPointer();
  const ps = Process.pointerSize;
  // JNI 函数表索引 169=GetStringUTFChars / 170=ReleaseStringUTFChars（表头 4 reserved 计数）
  const getUtf8 = new NativeFunction(table.add(169 * ps).readPointer(), 'pointer',
                                     ['pointer', 'pointer', 'pointer']);
  const releaseUtf8 = new NativeFunction(table.add(170 * ps).readPointer(), 'void',
                                        ['pointer', 'pointer', 'pointer']);
  const p = getUtf8(env, obj, ptr(0));
  if (p.isNull()) return '<null>';
  try { return p.readUtf8String(); } finally { releaseUtf8(env, obj, p); }
}

function hookNative(name, sig, fn) {
  const code = Process.arch === 'arm' ? fn.and(ptr('0xfffffffe')) : fn;
  const mod = Process.findModuleByAddress(code);
  console.log('[+] ' + name + sig + ' fn=' + fn + ' module=' + (mod ? mod.name : '?') +
              ' code_off=' + (mod ? code.sub(mod.base) : '?'));
  const key = fn.toString();
  if (hooked[key]) return;
  hooked[key] = true;
  try {
    Interceptor.attach(fn, {
      onEnter: function (args) {
        this.env = args[0];
        const nargs = (sig.match(/L/g) || []).length;  // 粗估对象参数（仅日志）
        console.log('[+] ' + name + ' entered, env=' + args[0] + ' objargs~' + nargs);
        // 打印所有非 env/clazz 的 jstring 参数（按签名逐个读，文章纪律 4）
        for (let i = 2; i < 8; i++) {
          try { console.log('    arg[' + i + '] = ' + readJniString(args[0], args[i])); }
          catch (e) { /* 非 jstring 参数跳过 */ }
        }
      },
      onLeave: function (retval) {
        try { console.log('[+] ' + name + ' returned jstring=' + readJniString(this.env, retval)); }
        catch (e) { console.log('[+] ' + name + ' returned (non-jstring) ' + retval); }
      }
    });
  } catch (e) { console.log('[-] attach ' + name + ' failed: ' + e); }
}

// hook RegisterNatives（host 侧 libart 导出，可见）
const syms = (typeof Module !== 'undefined' && Module.enumerateSymbolsSync)
  ? Module.enumerateSymbolsSync('libart.so')
  : Process.getModuleByName('libart.so').enumerateSymbols();
const regSym = syms.find(function (s) {
  return s.name.indexOf('RegisterNatives') >= 0 && s.name.indexOf('CheckJNI') < 0;
});
if (!regSym) throw new Error('RegisterNatives not found');
console.log('[+] RegisterNatives @ ' + regSym.address);

Interceptor.attach(regSym.address, {
  onEnter: function (args) {
    const methods = args[2], count = args[3].toInt32(), ps = Process.pointerSize;
    for (let i = 0; i < count; i++) {
      try {
        const entry = methods.add(i * ps * 3);   // 文章纪律 2：每项 3 指针
        const name = entry.readPointer().readUtf8String();
        const sig = entry.add(ps).readPointer().readUtf8String();
        const fn = entry.add(ps * 2).readPointer();
        if (name && name.indexOf('Java_') < 0) hookNative(name, sig, fn);
      } catch (e) {}
    }
  }
});
rpc.exports = { hooks: () => Object.keys(hooked) };
'''

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dev', default='10.0.182.152:5555')
    ap.add_argument('--wait', type=int, default=15)
    a = ap.parse_args()
    try:
        dev = frida.get_device(a.dev, timeout=8)
    except Exception as e:
        print('PENDING（frida-server 不可达: %s）' % e); return 3
    procs = [p for p in dev.enumerate_processes()
             if p.name == 'com.qwsaeda.junqirs' or p.name == '新军棋' or 'junqi' in p.name.lower()]
    if not procs:
        print('FAIL: app 未运行'); return 2
    print('== RegisterNatives hook（看雪方法论，pid=%d）==' % procs[0].pid)
    try:
        sess = dev.attach(procs[0].pid)
        sc = sess.create_script(JS)
        msgs = []
        sc.on('message', lambda m, d: msgs.append(str(m)))
        sc.load()
    except Exception as e:
        print('FAIL: attach/脚本: %s' % e); return 2
    print('   RegisterNatives hook 已装（需在 native 方法注册前 attach；已注册则抓不到）')
    time.sleep(a.wait)
    print('   捕获日志 (%d 条):' % len(msgs))
    for m in msgs[:20]: print('     ' + m[:160])
    try:
        hooks = sc.exports_sync.hooks()
    except Exception:
        print('判据: PENDING（script 销毁/环境不稳）'); return 3
    sess.detach()
    if not hooks:
        print('FAIL: 0 native 函数被 hook（app 已注册完，需 spawn 或在注册前 attach）')
        print('判据: FAIL（非 so 缺陷——时机问题）'); return 2
    print('== 全部通过 ==')
    print('判据: PASS（RegisterNatives hook：%d native 函数真实地址 + 纯 native JNI 字符串读取）' % len(hooks))
    return 0

if __name__ == '__main__':
    sys.exit(main())
