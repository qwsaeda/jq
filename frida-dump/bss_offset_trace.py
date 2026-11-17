#!/usr/bin/env python3
"""
Frida .bss 偏移追踪: hook jqt_mem_addr (0x328a88), 记录所有 r0 偏移值.
用法:
  frida -f com.cnvcs.junqi32 -l bss_offset_trace.js
  # 或附加到已运行进程:
  frida -n com.cnvcs.junqi32 -l bss_offset_trace.js
输出: bss_offsets.json (偏移 -> 访问次数 + 调用点)
"""
import frida, json, sys, time

JQT_MEM_ADDR = 0x328a88  # jqt_mem_addr 入口

JS_CODE = r"""
var results = {};  // offset_str -> {count, callSites: []}

function record(offset, caller_pc) {
    var key = "0x" + (offset + 0).toString(16).padStart(8, "0");
    if (!results[key]) {
        results[key] = {count: 0, callSites: []};
    }
    results[key].count++;
    if (results[key].callSites.length < 5) {
        results[key].callSites.push("0x" + (caller_pc + 0).toString(16).padStart(8, "0"));
    }
}

// Hook jqt_mem_addr (ARM32: 入口偏移 0, 参数 r0)
Interceptor.attach(Process.pointer("jqt_mem_addr").readPointer ? "base" : null, {})  // placeholder

// 实际: 通过 base address + offset
var base = Module.findBaseAddress("libjunqi.so");
if (!base) {
    console.log("[*] libjunqi.so not loaded, waiting...");
    var timer = setInterval(function() {
        base = Module.findBaseAddress("libjunqi.so");
        if (base) { clearInterval(timer); doHook(base); }
    }, 500);
} else {
    doHook(base);
}

function doHook(base) {
    var target = base.add(0x328a88);  // jqt_mem_addr
    Interceptor.attach(target, {
        onEnter: function(args) {
            var offset = args[0].toInt32();  // r0
            // 过滤: 只记录 0x150000-0x1a0000 范围 (.data/.bss)
            if (offset >= 0x150000 && offset < 0x1a0000) {
                var caller = this.context.lr ? this.context.lr.toInt32() : 0;
                record(offset, caller - base.toInt32());
            }
        }
    });
    console.log("[+] Hooked jqt_mem_addr at " + target);
}

// 定期 dump
setInterval(function() {
    var keys = Object.keys(results);
    console.log("[DUMP] " + keys.length + " unique offsets, total " +
        keys.reduce(function(s, k) { return s + results[k].count; }, 0) + " accesses");
    if (keys.length > 20) {
        // 只 dump 前 20 个
        keys.slice(0, 20).forEach(function(k) {
            console.log("  " + k + ": " + results[k].count + "x, callers=" + results[k].callSites.join(","));
        });
    }
}, 10000);

// 导出函数
rpc.exports = {
    get: function() {
        return results;
    },
    flush: function() {
        var keys = Object.keys(results);
        var summary = {};
        keys.forEach(function(k) {
            summary[k] = {count: results[k].count, callSites: results[k].callSites};
        });
        console.log("[FLUSH] " + JSON.stringify(summary));
        results = {};
    }
};
"""

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    
    dev = frida.get_device(sys.argv[1] if sys.argv[1] != "local" else None)
    
    if len(sys.argv) > 2 and sys.argv[2] == "attach":
        print(f"[*] Attaching to {sys.argv[3] if len(sys.argv) > 3 else 'com.cnvcs.junqi32'}...")
        pid = dev.get_process("com.cnvcs.junqi32").pid
        session = dev.attach(pid)
    else:
        print("[*] Spawning com.cnvcs.junqi32...")
        pid = dev.spawn(["com.cnvcs.junqi32"])
        session = dev.attach(pid)
        dev.resume(pid)
    
    script = session.create_script(JS_CODE)
    
    def on_message(message, data):
        if message["type"] == "send":
            payload = message["payload"]
            if isinstance(payload, dict) and "FLUSH" in str(payload):
                print(payload)
        elif message["type"] == "console":
            print(f"[{message['level']}] {message['payload']}")
    
    script.on("message", on_message)
    script.load()
    
    print("[*] Running... Ctrl+C to stop and dump results")
    try:
        time.sleep(60)  # 跑 60 秒
    except KeyboardInterrupt:
        pass
    
    # 最终 dump
    result = script.exports_sync.get()
    out = "frida-dump/bss_offsets.json"
    with open(out, "w") as f:
        json.dump(result, f, indent=2, sort_keys=True)
    print(f"\n[*] Dumped {len(result)} unique offsets to {out}")
    
    # 汇总
    total = sum(v["count"] for v in result.values())
    print(f"[*] Total accesses: {total}")
    top = sorted(result.items(), key=lambda x: -x[1]["count"])[:20]
    print("[*] Top 20 offsets:")
    for off, info in top:
        print(f"    {off}: {info['count']}x, callers={info['callSites'][:3]}")
    
    session.detach()

if __name__ == "__main__":
    main()
