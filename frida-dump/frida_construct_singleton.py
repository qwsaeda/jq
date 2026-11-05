#!/usr/bin/env python3
# frida_construct.py — frida 主动构造渲染器单例 + 写两槽 + 截图
import frida, time, subprocess, sys, re

def sh(cmd, t=10):
    return subprocess.run(cmd, timeout=t, capture_output=True, shell=True).stdout.decode()

dm = frida.get_device_manager()
d = dm.add_remote_device('127.0.0.1:27042')
sh('adb -s 10.0.182.146:5555 shell "am force-stop com.cnvcs.junqi; sleep 0.3; am start -n com.cnvcs.junqi/com.cnvcs.App"')

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
try {
    // 构造渲染器对象（136 字节，vtable=0x1595c0，字段取 30871 真值）
    var obj = Memory.alloc(136);
    var op = new NativePointer(obj);
    op.writeU32(0x1595c0);          // vtable
    op.add(4).writeU32(0x58000000);
    op.add(8).writeU32(0);
    op.add(0xc).writeU32(op.add(4).toUInt32()); // 自指针
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
    send('构造 obj=' + hex(obj) + ' vtable=0x1595c0');
    // 写 0x162400（真实单例槽）和 0x15c0a0（伪槽）
    var objv = obj.toUInt32();
    new NativePointer(shadow + 0x162400).writeU32(objv);
    new NativePointer(shadow + 0x15c0a0).writeU32(objv);
    send('已写 0x162400=' + hex(objv) + ' 0x15c0a0=' + hex(objv));
    // 验证
    send('验证 0x162400=' + hex(new NativePointer(shadow+0x162400).readU32()));
    send('验证 0x15c0a0=' + hex(new NativePointer(shadow+0x15c0a0).readU32()));
    send('DONE');
} catch(e){ send('ERR: '+e); }
""" % shadow

def on_msg(msg, data):
    if msg['type'] == 'send': print(msg['payload'])
    elif msg['type'] == 'error': print('ERR', msg.get('stack','')[:200])

script = session.create_script(js)
script.on('message', on_msg)
script.load()
time.sleep(4)
session.detach()
# 截图
sh('adb -s 10.0.182.146:5555 exec-out screencap -p > /tmp/opencode/construct.png')
try:
    from PIL import Image
    im = Image.open('/tmp/opencode/construct.png').convert('RGB')
    w, h = im.size
    print('屏幕 center=', im.getpixel((w//2,h//2)), 'q1=', im.getpixel((w//4,h//4)), 'q3=', im.getpixel((3*w//4,3*h//4)))
except Exception as e:
    print('截图解析:', e)
