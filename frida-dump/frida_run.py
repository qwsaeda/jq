#!/usr/bin/env python3
# frida_run.py — 军棋逆向 frida 统一命令入口（模板化，去重复机械执行）
#
# 用法：
#   python3 frida_run.py status                 # pid/shadow/槽状态（现有进程）
#   python3 frida_run.py restart                # 重启app + 读槽
#   python3 frida_run.py read [off1,off2...]    # 读槽+对象（默认 0x162400,0x15c0a0,0x15be88）
#   python3 frida_run.py obj ADDR [N]           # 读对象 N 字节
#   python3 frida_run.py shot                   # 截图+颜色分析
#   python3 frida_run.py monitor [sec]          # 时序监控槽（每0.5s）
#   python3 frida_run.py construct              # 主动构造渲染器单例（写0x162400/0x15c0a0）
#   python3 frida_run.py base                   # 抓原版 so 基址（/proc/maps）
#   python3 frida_run.py dump                   # dump 原版渲染器+纹理真值（需已进对局）
#   python3 frida_run.py fixsingleton           # 检测0x162400非零→写0x15c0a0→截图
#
# 依赖 frida_utils.py（同目录）。

import sys, os, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from frida_utils import *

DEFAULT_SLOTS = [0x162400, 0x15c0a0, 0x15be88, 0x15be80]


def cmd_status():
    pid = get_pid()
    shadow = get_shadow(pid) if pid else None
    print('pid=%s shadow=%s' % (pid or '-', hex(shadow) if shadow else '-'))
    if pid and shadow:
        sess = connect().attach(int(pid))
        msgs, errs = read_slots(sess, shadow, DEFAULT_SLOTS)
        for m in msgs:
            print(m)
        for e in errs:
            print('ERR', e)


def cmd_restart():
    pid = restart_app()
    shadow = get_shadow(pid)
    print('pid=%s shadow=%s' % (pid or '-', hex(shadow) if shadow else '-'))
    if pid and shadow:
        sess = connect().attach(int(pid))
        msgs, _ = read_slots(sess, shadow, DEFAULT_SLOTS)
        for m in msgs:
            print(m)


def cmd_read():
    slots = [int(x, 16) for x in sys.argv[2:]] if len(sys.argv) > 2 else DEFAULT_SLOTS
    sess, shadow, pid = full_attach(restart=False)
    print('pid=%s shadow=%s' % (pid, hex(shadow) if shadow else '-'))
    msgs, errs = read_slots(sess, shadow, slots)
    for m in msgs:
        print(m)
    for e in errs:
        print('ERR', e)


def cmd_obj():
    addr = int(sys.argv[2], 16)
    n = int(sys.argv[3], 16) if len(sys.argv) > 3 else 128
    sess, shadow, pid = full_attach(restart=False)
    msgs, errs = read_obj(sess, addr, n)
    for m in msgs:
        print(m)
    for e in errs:
        print('ERR', e)


def cmd_shot():
    info = screenshot()
    print(info)


def cmd_monitor():
    secs = float(sys.argv[2]) if len(sys.argv) > 2 else 15
    sess, shadow, pid = full_attach(restart=False)
    print('pid=%s shadow=%s monitor %ss' % (pid, hex(shadow) if shadow else '-', secs))
    slots = DEFAULT_SLOTS
    js = 'var shadow = %d; var n = 0;\n' % shadow
    js += 'var slots = [%s];\n' % ','.join('0x%x' % s for s in slots)
    js += r"""
var timer = setInterval(function(){
    n++;
    var line = 't=' + n + ': ';
    slots.forEach(function(off){
        var v = u32(shadow + off);
        line += ' ' + off.toString(16) + '=' + (v===null?'ERR':hex(v));
    });
    send(line);
    if(n >= Math.floor(%f*2)){ clearInterval(timer); send('DONE'); }
}, 500);
""" % secs
    msgs, errs = run_js(sess, js, timeout=secs + 3)
    for m in msgs:
        print(m)
    for e in errs:
        print('ERR', e)


def cmd_construct():
    """主动构造渲染器单例（写0x162400/0x15c0a0）。"""
    sess, shadow, pid = full_attach(restart=False)
    js = 'var shadow = %d;\n' % shadow
    js += r"""
var obj = Memory.alloc(136);
var op = new NativePointer(obj);
op.writeU32(0x1595c0);          // 渲染器 vtable（shadow相对）
op.add(4).writeU32(0x58000000);
op.add(8).writeU32(0);
op.add(0xc).writeU32(op.add(4).toUInt32());
op.add(0x10).writeU32(op.add(4).toUInt32());
op.add(0x14).writeU32(0);
op.add(0x18).writeU32(0x1000006e);
op.add(0x1c).writeU32(0);
op.add(0x20).writeU32(0);
op.add(0x24).writeU32(0x10000074);
op.add(0x28).writeU32(0);
op.add(0x2c).writeU32(0);
op.add(0x30).writeU32(0);
op.add(0x34).writeU32(0);
op.add(0x38).writeU32(0x10000001);
op.add(0x3c).writeU32(0);
var objv = obj.toUInt32();
new NativePointer(shadow + 0x162400).writeU32(objv);
new NativePointer(shadow + 0x15c0a0).writeU32(objv);
send('构造+写 0x162400/0x15c0a0 = ' + hex(objv));
send('DONE');
"""
    msgs, errs = run_js(sess, js, timeout=4)
    for m in msgs:
        print(m)
    for e in errs:
        print('ERR', e)
    print(screenshot())


def cmd_fixsingleton():
    """检测0x162400非零(vtable=0x1595c0)→写0x15c0a0→截图。"""
    sess, shadow, pid = full_attach(restart=False)
    js = 'var shadow = %d;\n' % shadow
    js += r"""
var done = false, count = 0;
var timer = setInterval(function(){
    count++;
    if(done) return;
    var v = u32(shadow + 0x162400);
    if(v && v >= 0xa0000000 && v <= 0xd0000000){
        var vt = u32(v);
        if(vt === 0x1595c0){
            try {
                new NativePointer(shadow + 0x15c0a0).writeU32(v);
                send('写 0x15c0a0 = ' + hex(v) + ' (t=' + count + ')');
                done = true; clearInterval(timer); send('DONE');
            } catch(e){ send('write ERR: ' + e); }
        }
    }
    if(count >= 30){ clearInterval(timer); send('TIMEOUT'); }
}, 300);
"""
    msgs, errs = run_js(sess, js, timeout=12)
    for m in msgs:
        print(m)
    for e in errs:
        print('ERR', e)
    print(screenshot())


def cmd_base():
    base = get_module_base()
    print('原版 libjunqi.so base = %s' % (hex(base) if base else '-'))


def cmd_dump():
    """dump 原版渲染器+纹理真值（需原版已进对局，BASE+0x162400 槽非零）。"""
    base = get_module_base()
    if not base:
        print('ERR 找不到原版 so 基址'); return
    sess = connect().attach(int(get_pid()))
    renderer_slot = base + 0x162400
    js = 'var BASE=%d;\n' % base
    js += r"""
var slot = u32(BASE + 0x162400);
send('renderer_slot=' + (slot===null?'ERR':hex(slot)));
if(slot && slot >= 0xa0000000 && slot < 0xf0000000){
    var tex = u32(slot + 48);
    send('renderer=' + hex(slot) + ' [+48纹理]=' + (tex===null?'ERR':hex(tex)));
    send('RDONE');
}
send('DONE');
"""
    msgs, errs = run_js(sess, js, timeout=4)
    renderer = tex = None
    for m in msgs:
        print(m)
        if m.startswith('renderer='):
            renderer = int(m.split('=')[1].split(' ')[0], 16)
        if m.startswith('renderer=') and '[+48纹理]=' in m:
            t = m.split('[+48纹理]=')[1]
            if t != 'ERR' and t != '-':
                tex = int(t, 16)
    if renderer and tex:
        rvt = renderer_slot  # not used
        # 渲染器 vtable = 读 slot0；纹理 vtable = 读 tex slot0
        js2 = 'var R=%d; var T=%d;\n' % (renderer, tex)
        js2 += r"""
var rvt = u32(R), tvt = u32(T);
send('RVT0=' + hex(rvt) + ' TVT0=' + hex(tvt));
send('DONE');
"""
        msgs2, _ = run_js(sess, js2, timeout=3)
        rvt = tvt = None
        for m in msgs2:
            if m.startswith('RVT0='):
                parts = m.split()
                rvt = int(parts[0].split('=')[1], 16)
                tvt = int(parts[1].split('=')[1], 16)
        if rvt and tvt:
            out = dump_texture_truth(sess, renderer, tex, rvt, tvt)
            import json
            data = {'BASE': base, 'renderer': renderer, 'texture': tex,
                    'renderer_vtable': rvt, 'texture_vtable': tvt}
            data.update(out)
            path = '/tmp/opencode/orig_renderer_dump.json'
            with open(path, 'w') as f:
                json.dump(data, f, indent=1)
            print('已保存', path)
            print('REND:', data.get('REND', '')[:80])
            print('TEX:', data.get('TEX', '')[:80])


def cmd_dumpwait():
    """轮询渲染器+48纹理，非零时立即 dump 纹理完整真值（解决+48动态问题）。
    用法：frida_run.py dumpwait [secs]"""
    secs = float(sys.argv[2]) if len(sys.argv) > 2 else 30
    base = get_module_base()
    if not base:
        print('ERR 找不到原版 so 基址'); return
    print('base=%s 轮询纹理出现 %ss' % (hex(base), secs))
    sess = connect().attach(int(get_pid()))
    js = 'var BASE=%d; var n=0;\n' % base
    js += r"""
var timer = setInterval(function(){
    n++;
    var slot = u32(BASE + 0x162400);
    if(slot && slot >= 0xa0000000 && slot < 0xf0000000){
        var tex = u32(slot + 48);
        send('t='+n+' renderer=' + hex(slot) + ' [+48]=' + (tex===null?'ERR':hex(tex)));
        if(tex && tex >= 0xa0000000 && tex < 0xf0000000){
            // 纹理出现，dump 完整
            var rv=[],tv=[],rvt=[],tvt=[];
            var rvt0 = u32(slot), tvt0 = u32(tex);
            for(var i=0;i<256;i+=4){ rv.push(u32(slot+i)); tv.push(u32(tex+i)); }
            for(var i=0;i<60;i++){ rvt.push(u32(rvt0+i*4)); tvt.push(u32(tvt0+i*4)); }
            send('REND:'+rv.map(function(v){return v===null?'-':(v>>>0).toString(16);}).join(','));
            send('TEX:'+tv.map(function(v){return v===null?'-':(v>>>0).toString(16);}).join(','));
            send('RVT:'+rvt.map(function(v){return v===null?'-':(v>>>0).toString(16);}).join(','));
            send('TVT:'+tvt.map(function(v){return v===null?'-':(v>>>0).toString(16);}).join(','));
            send('RENDADDR:' + hex(slot) + ' TEXADDR:' + hex(tex) + ' RVTADDR:' + hex(rvt0) + ' TVTADDR:' + hex(tvt0));
            clearInterval(timer);
            send('DONE');
        }
    }
    if(n >= Math.floor(%f)){ clearInterval(timer); send('TIMEOUT'); }
}, 500);
""" % secs
    msgs, errs = run_js(sess, js, timeout=secs + 3)
    import json
    data = {}
    for m in msgs:
        print(m)
        for k in ('REND', 'TEX', 'RVT', 'TVT', 'RENDADDR'):
            if m.startswith(k + ':'):
                data[k] = m.split(':', 1)[1]
    if 'RENDADDR' in data:
        path = '/tmp/opencode/orig_renderer_dump.json'
        with open(path, 'w') as f:
            json.dump(data, f, indent=1)
        print('已保存', path)
    for e in errs:
        print('ERR', e)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    cmd = sys.argv[1]
    fn = {'status': cmd_status, 'restart': cmd_restart, 'read': cmd_read,
          'obj': cmd_obj, 'shot': cmd_shot, 'monitor': cmd_monitor,
          'construct': cmd_construct, 'fixsingleton': cmd_fixsingleton,
          'base': cmd_base, 'dump': cmd_dump, 'dumpwait': cmd_dumpwait}.get(cmd)
    if not fn:
        print('未知命令', cmd)
        print(__doc__)
        return
    fn()


if __name__ == '__main__':
    main()
