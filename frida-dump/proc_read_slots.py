#!/usr/bin/env python3
# proc_read_vt2.py — 轮询 pid+shadow，立即读 0x162400/0x15be88/0x15c0a0 对象 vtable
import subprocess, struct, time, sys
ADB = "adb -s 10.0.182.146:5555"

def sh(cmd, timeout=10):
    r = subprocess.run(cmd, timeout=timeout, capture_output=True, shell=True)
    return r.stdout.decode(errors='replace')

def read_mem(pid, addr, length):
    r = subprocess.run([f"{ADB} shell su 0 dd if=/proc/{pid}/mem bs=1 skip={addr} count={length} 2>/dev/null"],
                       timeout=10, capture_output=True, shell=True)
    return r.stdout

def read_u32(pid, addr):
    b = read_mem(pid, addr, 4)
    return struct.unpack_from('<I', b, 0)[0] if len(b) >= 4 else None

sh(f'{ADB} shell "am force-stop com.cnvcs.junqi; sleep 0.2; am start -n com.cnvcs.junqi/com.cnvcs.App"', 20)

# 轮询 pid + shadow（最多 6 秒）
pid = None; shadow = None; t0 = time.time()
while time.time() - t0 < 6:
    pid = sh(f'{ADB} shell pidof com.cnvcs.junqi').strip()
    shadow_str = sh(f'{ADB} logcat -d -t 20 | grep g_jqt_shadow | tail -1 | grep -o "0x[0-9a-f]*" | head -1').strip()
    if pid and shadow_str:
        try:
            shadow = int(shadow_str, 16); pid = int(pid); break
        except: pass
    time.sleep(0.3)
if not pid or not shadow:
    print('ERR no pid/shadow'); sys.exit(1)
print(f'pid={pid} shadow=0x{shadow:x}')

for slot_off in [0x162400, 0x15be88, 0x15c0a0]:
    v = read_u32(pid, shadow + slot_off)
    if v is None:
        print(f'[shadow+0x{slot_off:x}] ERR'); continue
    print(f'[shadow+0x{slot_off:x}] = 0x{v:08x}')
    if v and 0xa0000000 <= v <= 0xd0000000:
        vt = read_u32(pid, v)
        print(f'   obj vtable = 0x{vt:08x}' if vt is not None else '   vtable ERR')
        b = read_mem(pid, v, 16)
        if len(b) >= 16:
            for i in range(0, 16, 4):
                print(f'   +0x{i:x} = 0x{struct.unpack_from("<I", b, i)[0]:08x}')

alive = sh(f'{ADB} shell pidof com.cnvcs.junqi').strip()
print(f'alive: {alive or "DEAD"}')
