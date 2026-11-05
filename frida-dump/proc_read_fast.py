#!/usr/bin/env python3
# proc_read_fast.py — /proc/pid/mem 大块读（启动后快速读对象图）
import subprocess, struct, time, sys

ADB = "adb -s 10.0.182.146:5555"

def sh(cmd, timeout=15):
    r = subprocess.run(cmd, timeout=timeout, capture_output=True, shell=True)
    return r.stdout.decode(errors='replace')

def read_mem_block(pid, addr, length):
    """读 /proc/pid/mem 一块，返回 bytes"""
    r = subprocess.run([f"{ADB} shell su 0 dd if=/proc/{pid}/mem bs=1 skip={addr} count={length} 2>/dev/null"],
                       timeout=15, capture_output=True, shell=True)
    return r.stdout

# 启动
sh(f'{ADB} shell "am force-stop com.cnvcs.junqi; sleep 0.2; am start -n com.cnvcs.junqi/com.cnvcs.App"', 20)
time.sleep(1.2)
pid = sh(f'{ADB} shell pidof com.cnvcs.junqi').strip()
shadow_str = sh(f'{ADB} logcat -d -t 50 | grep g_jqt_shadow | tail -1 | grep -o "0x[0-9a-f]*" | head -1').strip()
if not pid or not shadow_str:
    print('ERR no pid/shadow'); sys.exit(1)
pid = int(pid)
shadow = int(shadow_str, 16)
print(f'pid={pid} shadow=0x{shadow:x}')

# 读两块：0x15b000 起 8KB（覆盖 0x15c0a0 等）+ 0x162000 起 4KB（覆盖 0x162400）
blk1 = read_mem_block(pid, shadow + 0x15b000, 0x2000)
blk2 = read_mem_block(pid, shadow + 0x162000, 0x1000)
print(f'blk1={len(blk1)}B blk2={len(blk2)}B')

def u32(blk, off):
    if off + 4 <= len(blk):
        return struct.unpack_from('<I', blk, off)[0]
    return None

# 解析 blk1（基址 shadow+0x15b000）
print('=== 单例槽（blk1）===')
for off in [0x15c0a0, 0x15be88, 0x15be80, 0x15b578, 0x15c0cc, 0x15b8fc]:
    v = u32(blk1, off - 0x15b000)
    if v is not None:
        print(f'  [shadow+0x{off:x}] = 0x{v:08x}')
# 解析 blk2（基址 shadow+0x162000）
print('=== 单例槽（blk2）===')
for off in [0x162400]:
    v = u32(blk2, off - 0x162000)
    if v is not None:
        print(f'  [shadow+0x{off:x}] = 0x{v:08x}')
        # 对象展开
        if 0xa0000000 <= v <= 0xd0000000:
            print(f'  --- 对象 0x{v:08x} 前 64B ---')
            for i in range(0, 64, 4):
                r = read_mem_block(pid, v + i, 4)
                if len(r) >= 4:
                    print(f'    +0x{i:x} = 0x{struct.unpack_from("<I", r, 0)[0]:08x}')
        else:
            print(f'  (非堆地址，可能是偏移 0x{v:08x})')

# 进程存活
alive = sh(f'{ADB} shell pidof com.cnvcs.junqi').strip()
print(f'alive: {alive or "DEAD"}')
