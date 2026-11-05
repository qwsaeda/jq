#!/usr/bin/env python3
"""
redroid/frida 精准 dump：14 JNI 入口运行时全局值（18 地址 × 4B = 72B）

背景：14 JNI 入口的翻译体读 18 个全局地址（15 .text 常量池 + 2 .data + 6 .rodata 指针表，
合并去重后 = 18 个 base 相对偏移）。这些值静态 .so / frida-dump .rodata 都还原不了
（dlopen 重定位 + 壳自解密才填）。本脚本在 32 位 app 的 libjunqi.so 自解密跑完后，
精确读这 18 个地址的运行时 4 字节，落盘 jni_runtime_values.json。

触发方式（二选一）：
  A) 红roid 容器：先 am start 让壳 dlopen 跑完 init_array（自解密完成），
     再 attach（OnCreate NPE crash 前），dump 这 18 地址。
  B) 32 位真机：frida attach 到已运行 app（壳自解密后），dump 这 18 地址。

用法：
  frida -U com.cnvcs.junqi -l redroid_jni_dump.py
  或红roid：adb connect; am start -n com.cnvcs.junqi/.App & sleep; frida -U com.cnvcs.junqi -l redroid_jni_dump.py

产物：OUT 目录下 jni_runtime_values.json（18 项 {off, runtime_value}）
     → 喂给 rev64/src_jni_translated/jni_aligned.c 的注入层，14 入口可运行
"""
import frida, json, os, time, sys

OUT = os.environ.get("OUT", "/disk/workspaces/jq/frida-dump")
os.makedirs(OUT, exist_ok=True)

# 18 个 base 相对偏移（来自 /tmp/jni_dump_targets.json，运行时 = base + off）
# 15 .text 常量池（0x23xxx）+ 2 .data（0x15be80/84）。.rodata 指针表若需另加。
TARGETS = [
    0x0233f0, 0x023404, 0x023420, 0x023434, 0x023448, 0x02345c,
    0x023470, 0x023484, 0x023498, 0x0234ac, 0x0234c0, 0x0234d4,
    0x0235c4, 0x0235cc, 0x023604, 0x023608,
    0x15be80, 0x15be84,
]

js = """
(function() {
    // 1. 找 libjunqi.so base（Module 枚举）
    var base = null;
    var mods = Process.enumerateModulesSync();
    for (var i = 0; i < mods.length; i++) {
        if (mods[i].name.indexOf('libjunqi') !== -1) { base = mods[i].base; break; }
    }
    if (base === null) { send({type:'err', msg:'libjunqi.so not found (壳未 dlopen)'}); return; }

    // 2. 确认自解密完成：壳 .init_array[0]=0x3dae5 入口跑完后 .text/.rodata 才明文。
    //    attach 到"已运行"的 app 时通常已跑完。读 0x233e0 处指令判断是否仍是桩（0x0000 桩）。
    var t0 = base.add(0x233e0).readU32();

    // 3. 读 18 个目标地址的运行时 4 字节
    var targets = %s;
    var vals = [];
    for (var i = 0; i < targets.length; i++) {
        var off = targets[i];
        var p = base.add(off);
        var ok = p.readable && p.readU32 !== undefined;
        try { vals.push({off: off, runtime_value: p.readU32()}); }
        catch(e) { vals.push({off: off, runtime_value: null, err: e.message}); }
    }
    send({type:'ok', base: '0x'+base, text0: '0x'+t0.toString(16), values: vals});
})()
""" % json.dumps(TARGETS)

def on_message(msg, data):
    if msg['type'] != 'send':
        return
    p = msg['payload']
    if p.get('type') == 'err':
        print('[!] ' + p['msg']); return
    print("[*] libjunqi.so base = " + p['base'])
    print("[*] .text[0x233e0] = " + p['text0'] + "（非 0x00000000/桩 = 自解密已完成）")
    out = []
    for v in p['values']:
        rv = v.get('runtime_value')
        hexv = '0x%08x' % rv if rv is not None else 'READ_FAIL'
        print("    off=0x%06x  runtime=0x%s" % (v['off'], hexv))
        out.append({'off': v['off'], 'runtime_value': rv})
    jpath = os.path.join(OUT, 'jni_runtime_values.json')
    json.dump(out, open(jpath, 'w'), indent=2)
    print("[+] 落盘 %s （喂给 jni_aligned.c 注入层）" % jpath)

dev = frida.get_device('usb')
pid = dev.get_process('com.cnvcs.junqi') if False else None
session = frida.attach('com.cnvcs.junqi')
script = session.create_script(js)
script.on('message', on_message)
script.load()
# 等 send（精准读不需要长时间跑）
time.sleep(3)
script.unload()
session.detach()
print("[done]")
