#!/usr/bin/env python3
"""redroid dump .data+.bss（init_array 后全局真值）→ 对比 unidbg 垃圾"""
import frida, sys, time, os, json

ADDR = "localhost:5555"
DATA_OFF, DATA_SIZE = 0x15bb6c, 0x314        # .data
BSS_OFF, BSS_SIZE = 0x15be80, 0x7008          # .bss
OUT = "/disk/workspaces/jq/frida-dump"
os.makedirs(OUT, exist_ok=True)

js = r"""
function hex(n){return '0x'+n.toString(16);}
function findLib(){
    var mods=Process.enumerateModulesSync();
    for(var i=0;i<mods.length;i++) if(mods[i].name.indexOf('libjunqi')!==-1) return mods[i];
    return null;
}
rpc.exports = {
    dumpData: function(){
        var m=findLib();
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
# spawn app（让 libjunqi.so dlopen + init_array 完成）
pid = dev.spawn(["com.cnvcs.junqi"])
print("[*] spawned pid", pid)
session = dev.attach(pid)
script = session.create_script(js)
script.load()
print("[*] waiting for libjunqi.so dlopen + init_array...")
# 轮询找 libjunqi（最多 30s）
found = False
for i in range(60):
    time.sleep(0.5)
    try:
        r = script.exports_sync.dumpData()
        if 'err' not in r:
            found = True
            break
    except Exception as e:
        pass
if not found:
    print("[!] libjunqi.so 未加载，尝试 resume + 等待")
    try: dev.resume(pid)
    except: pass
    time.sleep(8)
    r = script.exports_sync.dumpData()
else:
    try: dev.resume(pid)
    except: pass
if 'err' in r:
    print("[!] dump 失败:", r['err']); sys.exit(1)
print("[*] libjunqi base:", r['base'])
open(OUT+"/redroid_data.bin","wb").write(bytes(r['data']))
open(OUT+"/redroid_bss.bin","wb").write(bytes(r['bss']))
print("[+] 已存 redroid_data.bin (0x314) + redroid_bss.bin (0x7008)")
print("[+] 总大小:", len(r['data'])+len(r['bss']), "bytes")
session.detach()
