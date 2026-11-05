#!/usr/bin/env python3
"""redroid attach dump .data+.bss（am start 后 libjunqi.so dlopen + init_array 完成）"""
import frida, sys, time, os

ADDR = "localhost:5555"
OUT = "/disk/workspaces/jq/frida-dump"
os.makedirs(OUT, exist_ok=True)

js = r"""
rpc.exports = {
    dumpData: function(){
        var mods=Process.enumerateModulesSync();
        var m=null;
        for(var i=0;i<mods.length;i++) if(mods[i].name.indexOf('libjunqi')!==-1) m=mods[i];
        if(!m) return {err:'no libjunqi'};
        var base=m.base;
        var data=base.add(0x15bb6c).readByteArray(0x314);
        var bss=base.add(0x15be80).readByteArray(0x7008);
        return {base:base.toString(), data:data, bss:bss};
    }
};
"""

dev = frida.get_device(ADDR, timeout=15)
print("[*] device:", dev)
# 尝试 am start（让 dlopen 完成），然后 attach
import subprocess
subprocess.run(["adb","-s",ADDR,"shell","su","0","am","start","-n","com.cnvcs.junqi/com.cnvcs.App"],capture_output=True)
time.sleep(4)
# 枚举进程找 junqi
sess = None
for attempt in range(4):
    try:
        procs = dev.enumerate_processes()
        target = None
        for p in procs:
            if 'junqi' in p.name or 'cnvcs' in p.name:
                target = p; break
        if not target:
            print("[*] 未找到 junqi 进程，重试...", attempt)
            time.sleep(2); continue
        print("[*] attach to", target.name, target.pid)
        sess = dev.attach(target.pid)
        break
    except Exception as e:
        print("[!] attach err:", e); time.sleep(2)
if sess is None:
    print("[!] 无法 attach"); sys.exit(1)
script = sess.create_script(js); script.load()
r = script.exports_sync.dumpData()
if 'err' in r:
    print("[!] dump 失败:", r['err']); sys.exit(1)
print("[*] libjunqi base:", r['base'])
open(OUT+"/redroid_data.bin","wb").write(bytes(r['data']))
open(OUT+"/redroid_bss.bin","wb").write(bytes(r['bss']))
print("[+] 已存 redroid_data.bin (0x314) + redroid_bss.bin (0x7008)")
print("[+] 总大小:", len(r['data'])+len(r['bss']), "bytes")
sess.detach()
