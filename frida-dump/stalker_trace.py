#!/usr/bin/env python3
# stalker_trace.py — 军棋 Stalker 指令级追踪模板（军棋版）
#
# 来源：看雪 x1ny 银行加固实战文套路沉淀（frida-dump/FRIDA_NOTES.md 第九节）。
# 目标：MuMu 真机上跟踪 libjunqi.so（32 位）内指定偏移函数的执行指令，
#       用于 §124ah 渲染对象图/构造链真值采集（unidbg BL bug 未修期间唯一真机动态路）。
#
# 用法：
#   python3 stalker_trace.py stalker 0xXXXXX [0xYYYYY ...]   # so 内 32 位偏移
#   示例：python3 stalker_trace.py stalker 0xbd764  # OnGraphCreate（渲染场景构造）
#
# 前置（frida_utils.py 约定，MuMu）：
#   adb -s 10.0.182.146:5555 ...，frida-server 宿主 127.0.0.1:27042
#
# 坑（FRIDA_NOTES §9.2）：
#   - 只 trace 单函数（onLeave 自动 unfollow），别全程跟（IPC 会拖死进程）
#   - android_dlopen_ext 在 libdl.so（MuMu Android 12 级系统 OK；老设备 fallback libc dlopen）
#   - SIGSEGV 型退出（清 sp/lr 跳未映射地址）Interceptor 钩不到，必须看退出前最后几条指令
#   - trace 完 app 保持运行；再跑一次可继续读；adb shell am force-stop 结束

import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from frida_utils import *


def cmd_stalker():
    offs = [int(x, 16) for x in sys.argv[2:]]
    if not offs:
        print('用法：python3 stalker_trace.py stalker <off_hex> [off_hex ...]')
        return
    sess, shadow, pid = full_attach(restart=True)
    print('pid=%s shadow=%s' % (pid, hex(shadow) if shadow else '-'))
    print('[i] Stalker trace offsets: ' + ', '.join('0x%x' % o for o in offs))
    js = r"""
var SO = 'libjunqi.so';
var TRACE_OFFS = [%s];
var buf = [], tid = -1, timer = null;
function flush(){ if(buf.length){ send(buf.join('\n')); buf = []; } }
function traceFn(mod, off) {
    var base = mod.base, end = mod.base.add(mod.size);
    Interceptor.attach(base.add(off), {
        onEnter: function (args) {
            send('>>> ' + SO + '+0x' + off.toString(16) + ' arg0=' + args[0] + ' arg1=' + args[1]);
            tid = Process.getCurrentThreadId();
            if (timer === null) timer = setInterval(function(){ flush(); }, 30);
            Stalker.follow(tid, { transform: function (it) {
                var ins = it.next();
                while (ins) {
                    var a = ins.address;
                    if (a.compare(base) >= 0 && a.compare(end) < 0)
                        buf.push('0x' + a.sub(base).toString(16) + ' : ' + ins.mnemonic + ' ' + ins.opStr);
                    it.keep(); ins = it.next();
                }
            }});
        },
        onLeave: function (ret) {
            if (timer !== null) { clearInterval(timer); timer = null; }
            Stalker.unfollow(tid); Stalker.flush(); flush();
            send('<<< ' + SO + '+0x' + off.toString(16) + ' ret=' + ret);
        }
    });
}
// dlopen 总入口装载后再挂（跟 §9.1 套路 1 一致）
try {
    var dlopenExt = Module.getExportByName('libdl.so', 'android_dlopen_ext');
    Interceptor.attach(dlopenExt, {
        onEnter: function (a) { this.name = a[0] ? a[0].readCString() : ''; },
        onLeave: function (ret) {
            if (this.name && this.name.indexOf(SO) !== -1 && !ret.isNull()) {
                var mod = Process.findModuleByName(SO);
                if (!mod) return;
                for (var i = 0; i < TRACE_OFFS.length; i++) traceFn(mod, TRACE_OFFS[i]);
                send('[*] Stalker hooks installed @ ' + SO + ' base=' + mod.base + ' size=' + mod.size);
            }
        }
    });
    send('[*] 已挂 android_dlopen_ext（libdl.so）');
} catch (e) {
    send('ERR libdl.so/android_dlopen_ext: ' + e);
}
send('DONE');
""" % ','.join('0x%x' % o for o in offs)
    msgs, errs = run_js(sess, js, timeout=6)
    for m in msgs:
        print(m)
    for e in errs:
        print('ERR', e)
    print('[i] app 保持运行；再跑一次可读后续；`adb -s %s shell am force-stop %s` 结束' % (
        '10.0.182.146:5555', PKG))


if __name__ == '__main__':
    if len(sys.argv) >= 2 and sys.argv[1] == 'stalker':
        cmd_stalker()
    else:
        print('用法：python3 stalker_trace.py stalker <off_hex> [off_hex ...]')
        print('示例：python3 stalker_trace.py stalker 0xbd764  # OnGraphCreate（渲染场景构造）')
