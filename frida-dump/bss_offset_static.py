#!/usr/bin/env python3
"""
静态 .bss 偏移提取 (基于 F_79858 反汇编).
方法: 提取 F_79858 内所有 "str r0, [sp, #N]" 后接 "bl jqt_mem_addr" 的模式,
      追踪 sp 偏移的初始值 (入口参数/局部初始化).
限制: ARM 栈帧分析, 无法完全静态还原, 但可提取高频模式.
"""
import re, subprocess
from collections import Counter

SO = "rev64/dist/libjunqi_arm32_aligned.so"
F_79858_START = 0x335258  # 推测入口
JQT_MEM_ADDR = 0x328a88

def main():
    out = subprocess.run(
        ["arm-none-eabi-objdump", "-d", SO],
        capture_output=True, text=True
    ).stdout
    
    lines = out.split("\n")
    
    # 提取 F_79858 范围 (向前找入口, 向后找结束)
    # 简化: 提取 0x335000-0x34a000 范围
    in_range = []
    for line in lines:
        m = re.match(r'\s*([0-9a-f]{6}):', line)
        if m:
            addr = int(m.group(1), 16)
            if 0x335000 <= addr < 0x34a000:
                in_range.append((addr, line))
    
    # 找所有 "str rX, [sp, #N]" 后 5 条指令内有 "bl jqt_mem_addr"
    patterns = []
    for i, (addr, line) in enumerate(in_range):
        if "bl" in line and "328a88" in line:
            # 向前找 5 条内的 str r0, [sp, #N]
            for j in range(max(0, i-5), i):
                prev_addr, prev_line = in_range[j]
                m2 = re.search(r'str\s+r(\d),\s*\[sp,\s*#(\d+)\]', prev_line)
                if m2:
                    reg = m2.group(1)
                    sp_off = int(m2.group(2))
                    patterns.append({
                        "call_site": "0x%x" % addr,
                        "sp_offset": sp_off,
                        "register": "r" + reg,
                        "prev_line": prev_line.strip()
                    })
                    break
                # 也检查 ldr r0, [sp, #N] + bl 模式
                m3 = re.search(r'ldr\s+r0,\s*\[sp,\s*#(\d+)\]', prev_line)
                if m3:
                    sp_off = int(m3.group(1))
                    patterns.append({
                        "call_site": "0x%x" % addr,
                        "sp_offset": sp_off,
                        "register": "r0 (ldr)",
                        "prev_line": prev_line.strip()
                    })
                    break
    
    print(f"=== F_79858 范围内 jqt_mem_addr 调用点: {len(patterns)} ===")
    
    # 统计 sp_offset 分布
    sp_counts = Counter(p["sp_offset"] for p in patterns)
    print(f"\n=== sp 偏移分布 (Top 20) ===")
    for sp_off, count in sp_counts.most_common(20):
        print(f"  [sp, #{sp_off:3d}] : {count} 次")
    
    # 输出 JSON
    import json
    out_file = "frida-dump/bss_offsets_static.json"
    with open(out_file, "w") as f:
        json.dump({
            "method": "static_disasm_F_79858",
            "patterns": patterns,
            "sp_offset_counts": dict(sp_counts),
        }, f, indent=2)
    print(f"\n[*] 写入 {out_file}")

if __name__ == "__main__":
    main()
