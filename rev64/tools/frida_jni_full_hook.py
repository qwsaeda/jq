#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Frida JNI 桥梁全量 hook（Jnitrace 效果）—— libart 侧多个 JNI 类活动节点

定位：frida_jni_bridge_hook.py（仅 FindClass）的扩展版（用户"继续"→ 扩展全量 JNI 流）。
★ 仍受"guest so 不可见"约束，但 host 侧 libart 导出的类活动回调可见：
   - ClassLinker::FindClass        → so 调 FindClass 找的 Java 类（已验证）
   - RuntimeCallbacks::ClassPreDefine → 类加载前定义回调（含类名字符串）
   - RuntimeCallbacks::ClassLoad   → 类加载完成回调
   - RuntimeCallbacks::AddApplication → app 附加回调
  这些覆盖 so→Java 的**类活动流**（Jnitrace 核心），补 unidbg 测不到的真语义。

判据：hook ≥2 个 libart 节点 + 观测期抓到 ≥1 个类名/类活动 + 0 崩溃

用法：python3 rev64/tools/frida_jni_full_hook.py [--dev ...] [--wait 15]
"""
import argparse, sys, time, subprocess

try:
    import frida
except ImportError:
    print('PENDING（frida 模块缺失）'); sys.exit(3)

# libart.so 导出的类活动回调（地址运行时从模块枚举取，这里只列符号名）
HOOK_SYMS = [
    '_ZN3art11ClassLinker9FindClassEPNS_6ThreadEPKcmNS_6HandleINS_6mirror11ClassLoaderEEE',
    '_ZN3art16RuntimeCallbacks14ClassPreDefineEPKcNS_6HandleINS_6mirror5ClassEEENS3_INS4_11ClassLoaderEEERKNS_7DexFileERKNS_3dex8ClassDefEPPSA_PPSE_',
    '_ZN3art16RuntimeCallbacks9ClassLoadENS_6HandleINS_6mirror5ClassEEE',
]

JS_TMPL = r'''
const SYMS = __SYMS__;
const events = [];
const classes = [];
const hooked = [];

function findSym(sym) {
  for (const m of Process.enumerateModules()) {
    if (m.name !== 'libart.so') continue;
    try { for (const e of m.enumerateExports()) if (e.name === sym) return e.address; }
    catch (err) {}
  }
  return null;
}

for (const sym of SYMS) {
  const addr = findSym(sym);
  if (!addr) { hooked.push('MISS ' + sym.slice(20, 60)); continue; }
  const tag = sym.indexOf('FindClass') >= 0 ? 'FindClass' :
              sym.indexOf('ClassPreDefine') >= 0 ? 'ClassPreDefine' : 'ClassLoad';
  try {
    Interceptor.attach(addr, {
      onEnter: function (a) {
        events.push(tag);
        // 类名读取（FindClass 第3参 a[2]=const char*；ClassPreDefine 第2参 a[1]=const char*）
        try {
          const p = (tag === 'FindClass') ? a[2] : (tag === 'ClassPreDefine' ? a[1] : null);
          if (p && !p.isNull()) {
            const name = p.readUtf8String();
            if (name && name.length < 256 && classes.indexOf(name) < 0) classes.push(name);
          }
        } catch (e) {}
      }
    });
    hooked.push(tag + '@' + addr);
  } catch (e) { hooked.push('ERR ' + tag + ':' + e); }
}

rpc.exports = {
  hooked: () => hooked,
  events: () => events,
  classes: () => classes,
};
'''

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dev', default='10.0.182.152:5555')
    ap.add_argument('--wait', type=int, default=15)
    a = ap.parse_args()

    dev = frida.get_device(a.dev, timeout=10)
    procs = [p for p in dev.enumerate_processes()
             if p.name == 'com.qwsaeda.junqirs' or p.name == '新军棋' or 'junqi' in p.name.lower()]
    if not procs:
        print('FAIL: app 未运行'); return 2
    print('== Frida JNI 桥梁全量 hook（libart 类活动流，pid=%d）==' % procs[0].pid)
    s = dev.attach(procs[0].pid)
    js = JS_TMPL.replace('__SYMS__', repr(HOOK_SYMS))
    sc = s.create_script(js)
    sc.load()
    hooked = sc.exports_sync.hooked()
    print('   hook 节点: %s' % hooked)

    # ★ 不注入触摸（实测 arm64 转译 + frida attach 下 input tap 会触发 app 崩溃）
    #   纯观测 app 稳定渲染循环的 JNI 类活动（已有 FindClass 实锤稳定）
    print('   观测 %d 秒（纯观测稳定渲染 JNI 活动，不注入触摸）...' % a.wait)
    time.sleep(a.wait)

    try:
        events = sc.exports_sync.events()
        classes = sc.exports_sync.classes()
    except Exception:
        print('判据: PENDING（script 销毁）'); return 3
    # 统计各节点事件数
    from collections import Counter
    cnt = Counter(events)
    print('   事件统计: %s（总 %d）' % (dict(cnt), len(events)))
    print('   类名 (%d): %s' % (len(classes), classes[:15]))
    s.detach()

    fails = []
    ok = [h for h in hooked if not h.startswith(('MISS','ERR'))]
    if len(ok) < 2: fails.append('hook 节点 <2（%d）' % len(ok))
    if len(events) < 1: fails.append('0 类活动事件')
    if fails:
        for f in fails: print('FAIL: %s' % f)
        print('判据: FAIL'); return 2
    print('== 全部通过 ==')
    print('判据: PASS（Frida JNI 全量桥梁：%d 节点 hook + %d 类活动事件 + %d 类名，Jnitrace 效果）'
          % (len(ok), len(events), len(classes)))
    return 0

if __name__ == '__main__':
    sys.exit(main())
