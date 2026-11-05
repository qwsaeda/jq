#!/usr/bin/env python3
# frida_utils.py — 军棋逆向 frida 工作流公共库（模板化，去重复）
#
# 用法（被 frida_run.py 和其他脚本 import）：
#   from frida_utils import *
#   sess = full_attach()          # 重启app+轮询pid/shadow+attach，返回 (session, shadow)
#   msgs = run_js(sess, "send('hi');")   # 收所有 send payload
#   slots = read_slots(sess, shadow, [0x162400, 0x15c0a0])
#   obj = read_obj(sess, 0xc64ceed0, 128)
#
# 关键约定：
#   - 设备：MuMu adb 10.0.182.146:5555
#   - frida-server：x86_64 版（17.24.3），junqi 进程是 32 位但需 x86_64 server
#   - g_jqt_shadow：翻译体进程 OnCreate 打 logcat，用 grep g_jqt_shadow 抓（日志滚动需重启）
#   - frida 17 API：Memory.readU32 不存在，用 new NativePointer(a).readU32()/writeU32()
#   - 原版 so 基址：0x0c068000（原版进程，/proc/maps 抓）

import frida, time, subprocess, sys, re, os

ADB = 'adb -s 10.0.182.146:5555'
PORT = 27042
PKG = 'com.cnvcs.junqi'
ACTIVITY = PKG + '/com.cnvcs.App'

# 原版 so 基址（原版进程，可被 /proc/maps 抓到；翻译体进程用 shadow）
ORIG_BASE = 0x0c068000

# ---- JS 公共片段（frida 17 兼容：NativePointer.readU32）----
JS_HELPERS = r"""
function hex(x){ return '0x' + (x>>>0).toString(16); }
function u32(a){ try { return new NativePointer(a).readU32(); } catch(e){ return null; } }
function read_obj(addr, n){
    var out = [];
    for(var i=0;i<n;i+=4){
        var f = u32(addr+i);
        if(f!==null) out.push('+0x'+i.toString(16)+' = '+hex(f));
    }
    return out;
}
"""


def sh(cmd, t=10):
    """执行 adb shell 命令，返回 stdout（去 CR）。"""
    try:
        r = subprocess.run(cmd, timeout=t, capture_output=True, shell=True)
        return r.stdout.decode(errors='replace').replace('\r', '')
    except subprocess.TimeoutExpired:
        return ''


def adb(cmd, t=10):
    return sh('%s %s' % (ADB, cmd), t)


def connect():
    """连接 frida-server（remote device）。"""
    dm = frida.get_device_manager()
    return dm.add_remote_device('127.0.0.1:%d' % PORT)


def get_pid():
    """当前 junqi pid，无则 ''。"""
    return adb('shell pidof %s' % PKG).strip()


def restart_app(wait=8):
    """force-stop + am start，轮询 pid 直到出现。返回 pid 或 None。"""
    adb('shell "am force-stop %s; sleep 0.3; am start -n %s"' % (PKG, ACTIVITY))
    t0 = time.time()
    while time.time() - t0 < wait:
        pid = get_pid()
        if pid:
            return pid
        time.sleep(0.3)
    return None


def get_shadow(pid=None, retries=25):
    """从 logcat grep g_jqt_shadow 抓 shadow 地址。返回 int 或 None。
    日志滚动后抓不到，需重启 app 后立即抓。"""
    pid = pid or get_pid()
    for _ in range(retries):
        out = adb('logcat -d -t 200 | grep g_jqt_shadow | tail -1')
        m = re.search(r'0x[0-9a-fA-F]+', out)
        if m:
            return int(m.group(0), 16)
        time.sleep(0.2)
    return None


def attach(pid=None):
    """attach junqi。返回 session 或抛错。"""
    pid = pid or get_pid()
    if not pid:
        raise RuntimeError('no junqi pid')
    return connect().attach(int(pid))


def full_attach(restart=True, wait=8):
    """重启 app + 轮询 pid/shadow + attach。返回 (session, shadow, pid)。
    restart=False 时不重启（用现有进程，shadow 需日志未滚动）。"""
    pid = None
    if restart:
        pid = restart_app(wait)
        shadow = get_shadow(pid)
    else:
        pid = get_pid()
        shadow = get_shadow(pid)
    if not pid:
        raise RuntimeError('no junqi pid after restart')
    sess = connect().attach(int(pid))
    return sess, shadow, pid


def run_js(session, js, timeout=4, helpers=True):
    """加载 JS（可带公共 helpers），收所有 send payload，返回 (messages, errors)。
    messages 是 list[str]，errors 是 list[str]。"""
    code = (JS_HELPERS if helpers else '') + '\n' + js
    msgs, errs = [], []
    def on_msg(msg, data):
        if msg['type'] == 'send':
            msgs.append(msg['payload'])
        elif msg['type'] == 'error':
            errs.append(msg.get('stack', str(msg))[:300])
    script = session.create_script(code)
    script.on('message', on_msg)
    script.load()
    time.sleep(timeout)
    try:
        session.detach()
    except Exception:
        pass
    return msgs, errs


def read_slots(session, shadow, slots, timeout=3):
    """读多个 shadow 偏移槽 + 每个槽指向对象展开。返回 {off: (val, vtable, fields)}。"""
    js = 'var shadow = %d;\n' % shadow
    js += 'var slots = [%s];\n' % ','.join('0x%x' % s for s in slots)
    js += r"""
slots.forEach(function(off){
    var v = u32(shadow + off);
    send('SLOT ' + off.toString(16) + ' ' + (v===null?'ERR':hex(v)));
    if(v && v >= 0xa0000000 && v <= 0xf0000000){
        var vt = u32(v);
        send('  VT ' + (vt===null?'ERR':hex(vt)));
        read_obj(v, 64).forEach(function(l){ send('  ' + l); });
    }
});
send('DONE');
"""
    msgs, errs = run_js(session, js, timeout)
    return msgs, errs


def read_obj(session, addr, n=128, timeout=3):
    """读一个对象 n 字节展开。返回 list[line]。"""
    js = 'var a = %d; var n = %d;\n' % (addr, n)
    js += r"""
send('OBJ ' + hex(a));
read_obj(a, n).forEach(function(l){ send(l); });
send('DONE');
"""
    msgs, errs = run_js(session, js, timeout)
    return msgs, errs


def screenshot(path='/tmp/opencode/shot.png'):
    """截图 + PIL 颜色分析（top colors + center）。返回 dict。"""
    adb('exec-out screencap -p > %s' % path)
    try:
        from PIL import Image
        import collections
        im = Image.open(path).convert('RGB')
        w, h = im.size
        cnt = collections.Counter()
        for y in range(0, h, 60):
            for x in range(0, w, 60):
                cnt[im.getpixel((x, y))] += 1
        return {'size': (w, h), 'top': cnt.most_common(6),
                'center': im.getpixel((w // 2, h // 2))}
    except Exception as e:
        return {'error': str(e)}


def get_module_base(pid=None):
    """从 /proc/maps 找原版 libjunqi.so 基址（原版进程）。返回 int 或 None。"""
    pid = pid or get_pid()
    out = adb('shell "su 0 cat /proc/%s/maps 2>/dev/null | grep -i junqi | head -1"' % pid)
    m = re.match(r'([0-9a-fA-F]+)-', out)
    return int(m.group(1), 16) if m else None


def dump_texture_truth(session, renderer, tex, renderer_vt, tex_vt, n=256, vtslots=60):
    """dump 渲染器+纹理完整真值（原版进程）。返回 dict（地址/值数组）。"""
    js = r"""
var R=%d,T=%d,RVT=%d,TVT=%d;
var rv=[],tv=[],rvv=[],tvv=[];
for(var i=0;i<%d;i+=4){ rv.push(u32(R+i)); tv.push(u32(T+i)); }
for(var i=0;i<%d;i++){ rvv.push(u32(RVT+i*4)); tvv.push(u32(TVT+i*4)); }
send('REND:'+rv.map(function(v){return v===null?'-':(v>>>0).toString(16);}).join(','));
send('TEX:'+tv.map(function(v){return v===null?'-':(v>>>0).toString(16);}).join(','));
send('RVT:'+rvv.map(function(v){return v===null?'-':(v>>>0).toString(16);}).join(','));
send('TVT:'+tvv.map(function(v){return v===null?'-':(v>>>0).toString(16);}).join(','));
send('DONE');
""" % (renderer, tex, renderer_vt, tex_vt, n, vtslots)
    msgs, errs = run_js(session, js, timeout=4)
    out = {}
    for m in msgs:
        for k in ('REND', 'TEX', 'RVT', 'TVT'):
            if m.startswith(k + ':'):
                out[k] = m.split(':', 1)[1]
    return out
