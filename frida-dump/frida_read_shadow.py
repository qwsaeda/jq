#!/usr/bin/env python3
# frida_read_final.py — 重启 app + 轮询抓 shadow + frida 读对象图（v178cz 稳定版）
import frida, time, subprocess, sys, re

def sh(cmd, t=10):
    return subprocess.run(cmd, timeout=t, capture_output=True, shell=True).stdout.decode()

dm = frida.get_device_manager()
d = dm.add_remote_device('127.0.0.1:27042')

# 重启 app
sh('adb -s 10.0.182.146:5555 shell "am force-stop com.cnvcs.junqi; sleep 0.3; am start -n com.cnvcs.junqi/com.cnvcs.App"')

# 轮询 pid + shadow
pid = shadow = None
t0 = time.time()
while time.time() - t0 < 8:
    pid = sh('adb -s 10.0.182.146:5555 shell pidof com.cnvcs.junqi').strip()
    if pid:
        out = sh('adb -s 10.0.182.146:5555 logcat -d -t 150 | grep g_jqt_shadow | tail -1')
        m = re.search(r'0x[0-9a-fA-F]+', out)
        if m:
            shadow = int(m.group(0), 16)
            break
    time.sleep(0.3)
if not pid or not shadow:
    print('ERR no pid/shadow'); sys.exit(1)
print(f'pid={pid} shadow=0x{shadow:x}')

session = d.attach(int(pid))
print('attached')
js = r"""
var shadow = %d;
function hex(x){ return '0x' + (x>>>0).toString(16); }
function u32(a){ try { return new NativePointer(a).readU32(); } catch(e){ return null; } }
// 验证 shadow
try {
    var txt = new NativePointer(shadow + 0x120bf8).readUtf8String(8);
    send('shadow验证 +0x120bf8 = "' + txt + '"');
} catch(e){ send('shadow验证 ERR: ' + e); }
var slots = [0x15c0a0, 0x162400, 0x15be88, 0x15be80, 0x15b578, 0x15c0cc, 0x15b8fc];
slots.forEach(function(off){
    var v = u32(shadow + off);
    if(v === null){ send('slot 0x'+off.toString(16)+' ERR'); return; }
    send('slot 0x'+off.toString(16) + ' = ' + hex(v));
    if(v && v >= 0xa0000000 && v <= 0xd0000000){
        var vt = u32(v);
        send('   obj vtable = ' + hex(vt===null?0:vt));
        for(var i=4;i<40;i+=4){
            var f = u32(v+i);
            if(f!==null) send('   +0x'+i.toString(16)+' = '+hex(f));
        }
    }
});
send('DONE');
""" % shadow

def on_msg(msg, data):
    if msg['type'] == 'send': print(msg['payload'])
    elif msg['type'] == 'error': print('ERR', msg.get('stack','')[:200])

script = session.create_script(js)
script.on('message', on_msg)
script.load()
time.sleep(3)
session.detach()
