#!/usr/bin/env python3
"""
Frida 脚本：dump 军棋 libjunqi.so 自解密壳解密后的明文 .text/.rodata
用于 32 位 Android 真机（需 root + frida-server 32 位）

流程：
1. attach 到已启动的军棋 app（com.cnvcs.junqi）
2. 找 libjunqi.so 的基址
3. 在 .init_array[0]=0x3dae5 壳入口设断点
4. 等自解密例程跑完（init_array 全部执行后），.rodata/.text 已解密
5. dump .text 段（0x233e0-0x120bf8）和 .rodata 段（0x120bf8-0x13dc28）为 bin
6. 输出可直接 arm-none-eabi-objdump 反汇编

用法：
  1. 手机上装军棋，启动它（让壳 dlopen libjunqi.so 跑完 init_array）
  2. adb root; adb shell "path/to/frida-server -D &" 或 adb shell su -c '...'
  3. 电脑上：frida -U -f com.cnvcs.junqi -l dump_shell.py --no-pause
     或 attach 到已运行进程：frida -U com.cnvcs.junqi -l dump_shell.py
  4. 产物落盘：把 base_addr、text.bin、rodata.bin 发回分析
"""
import frida, os, sys, time

# libjunqi.so 已知段（相对 .so 基址）
TEXT_OFF, TEXT_SIZE = 0x233e0, 0x120bf8 - 0x233e0
RODATA_OFF, RODATA_SIZE = 0x120bf8, 0x13dc28 - 0x120bf8
INIT_ARRAY_OFF, INIT_ARRAY_COUNT = 0x155000, 12
# 壳自解密入口（.init_array[0]）
SHELL_ENTRY_REL = 0x3dae5

out_dir = os.environ.get("OUT", "/disk/workspaces/jq/frida-dump/out")
os.makedirs(out_dir, exist_ok=True)

js = """
function hex(n) { return '0x' + n.toString(16); }

// 1. 找 libjunqi.so 基址
function findLib() {
    var mod = Process.findModuleByAddress(null);
    var base = null;
    // 遍历 /proc/self/maps 的映射（Module.enumerateSync() 最稳）
    var modules = Process.enumerateModulesSync();
    for (var i = 0; i < modules.length; i++) {
        if (modules[i].name.indexOf('libjunqi') !== -1) {
            base = modules[i].base;
            break;
        }
    }
    return base;
}

// 2. 设断点：壳入口（可选，用于确认自解密时序）
function setBreakpoint(base) {
    var entry = base.add(SHELL_ENTRY_REL);
    var bp = Interceptor.attach(entry, {
        onEnter: function() {
            console.log('[*] shell entry 0x' + entry + ' hit, init running...');
        }
    });
    return bp;
}

// 3. 等 init_array 跑完。判断标志：.init_array[0] 处已是解密后代码（非 0x0000 桩）
//    简单策略：等 3 秒（app 启动到 init_array 完成），再 dump
function waitDecrypt() {
    console.log('[*] waiting for .init_array to finish self-decrypt...');
    while (true) {
        try { Thread.sleep(1000); } catch (e) {}
        // 读 .init_array[0] 第一个字，若不是 0 视为已解密
        var base = findLib();
        if (base === null) { continue; }
        var first = base.add(INIT_ARRAY_OFF).readU32();
        if (first !== 0) {
            console.log('[+] .init_array[0] = 0x' + first + ', self-decrypt likely done');
            return;
        }
    }
}

function dumpSection(base, off, size, name) {
    var ptr = base.add(off);
    var data = ptr.readByteArray(size);
    Java.perform(function() {
        var ba = Java.castObject(data, '[B');
        // 写到 Java 文件
        var FileOutputStream = Java.use('java.io.FileOutputStream');
        var fos = FileOutputStream.new(name);
        fos.write(ba, 0, size);
        fos.close();
        console.log('[+] dumped ' + name + ' (' + size + ' bytes)');
    });
    var baseAddr = base;
    var off2 = off;
}
"""

def on_message(msg, data):
    print(f"[frida] {msg}")

# ---- main ----
print("[*] connecting to device...")
dev = frida.get_device("usb")
pid = dev.spawn(["com.cnvcs.junqi"])  # 冷启动，让壳从 dlopen 开始跑
session = dev.attach(pid)
script = session.create_script(js)
script.on("message", on_message).load()
dev.resume(pid)
time.sleep(3)
print("[*] app resumed, shell dlopen should be running...")
time.sleep(5)  # 给壳自解密 5 秒

# 读基址
mod = next((m for m in dev.execute_sync("Process.enumerateModulesSync()").split('\n') if 'libjunqi' in m), None)
print("[*] module:", mod)

print("[*] 完整流程应在手机上跑；此脚本是模板。")
print("    将 out/ 目录的 text.bin/rodata.bin 发回后，重链接 ELF + Ghidra 分析。")
