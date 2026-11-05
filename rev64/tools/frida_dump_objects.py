#!/usr/bin/env python3
"""frida_dump_objects.py — 从 MuMu 原版进程 dump 15 个 .bss 指针槽的对象内容

用法：
  python3 frida_dump_objects.py [--pid PID] [--bss-dump /tmp/bss_now.bin]

流程：
  1. 读 /proc/PID/mem 的 .bss 段（0x15be80-0x162e88）→ 当前进程槽值
  2. 对 15 个 scudo 指针槽，读指针指向的对象内容（各 512B）
  3. 落盘 /tmp/opencode/objects/（每对象一个 bin + 值 JSON）

依赖：frida 17.x（pip install frida-tools），frida-server 已在 MuMu 跑。
"""
import frida, sys, json, os, struct, argparse

BSS_LO = 0x15be80
BSS_HI = 0x162e88
# 15 个 scudo 指针槽（0x162400 渲染器单例已构造，跳过）
PTR_SLOTS = [
    0x1620b0, 0x1620b4, 0x1620b8, 0x1620bc, 0x1621bc,
    0x1622c4, 0x1623d8, 0x1623fc, 0x162448, 0x16244c,
    0x162660, 0x162664, 0x162668, 0x16266c, 0x162844,
]

def read_mem(pid, addr, size):
    """读进程内存（/proc/PID/mem，需 root）"""
    with open(f'/proc/{pid}/mem', 'rb', 0) as f:
        f.seek(addr)
        return f.read(size)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--pid', type=int, default=0)
    ap.add_argument('--outdir', default='/tmp/opencode/objects')
    ap.add_argument('--dump-bss', default='/tmp/opencode/bss_now.bin')
    args = ap.parse_args()
    if not args.pid:
        # 自动找 pid
        import subprocess
        pid = subprocess.run(['adb','-s','10.0.182.146:5555','shell','pidof com.cnvcs.junqi'],
                             capture_output=True,text=True).stdout.strip()
        if not pid:
            print('错误：找不到 com.cnvcs.junqi 进程，先 am start')
            sys.exit(1)
        args.pid = int(pid)
    print(f'PID = {args.pid}')
    os.makedirs(args.outdir, exist_ok=True)
    session = frida.get_device_manager().enumerate_devices()
    # 用 adb 设备
    dev = frida.get_device('usb' if not session else 'local', timeout=10)
    # 附加到进程
    proc = dev.attach(args.pid)
    sc = proc.create_script('''
        rpc.exports = {
            readmem: function(addr, size) {
                var buf = Memory.readByteArray(ptr(addr), size);
                return buf;
            }
        };
    ''')
    sc.load()
    # 读 .bss 段
    bss = sc.exports_sync.readmem(BSS_LO, BSS_HI - BSS_LO)
    bss_bytes = bytes(bss)
    open(args.dump_bss, 'wb').write(bss_bytes)
    print(f'.bss dump → {args.dump_bss} ({len(bss_bytes)}B)')
    # 读 15 个指针槽 + 对象内容
    result = {}
    for slot in PTR_SLOTS:
        off = slot - BSS_LO
        ptr = struct.unpack_from('<I', bss_bytes, off)[0]
        entry = {'slot': slot, 'ptr': ptr}
        if 0xc0000000 <= ptr < 0xf4000000:
            try:
                obj = sc.exports_sync.readmem(ptr, 512)
                obj_bytes = bytes(obj)
                objfile = os.path.join(args.outdir, f'obj_{slot:08x}.bin')
                open(objfile, 'wb').write(obj_bytes)
                entry['obj_file'] = objfile
                entry['obj_size'] = len(obj_bytes)
                # 对象头 16 字分析
                hdr = [struct.unpack_from('<I', obj_bytes, i*4)[0] for i in range(16)]
                entry['hdr'] = [hex(h) for h in hdr]
            except Exception as e:
                entry['obj_err'] = str(e)
        else:
            entry['note'] = '非 scudo 指针（跳过对象读）'
        result[f'0x{slot:08x}'] = entry
        print(f'  槽 0x{slot:08x} = 0x{ptr:08x} ' + (f'对象头: {entry.get("hdr",[])[:6]}' if 'hdr' in entry else entry.get('note','')))
    json.dump(result, open(os.path.join(args.outdir,'objects.json'),'w'), indent=1)
    print(f'\n对象清单 → {args.outdir}/objects.json')
    sc.unload()
    proc.detach()

if __name__ == '__main__':
    main()
