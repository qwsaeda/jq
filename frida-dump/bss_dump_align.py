#!/usr/bin/env python3
"""frida dump .bss 结果 与 §124y-69/70/72 静态收表的自动对照基准。

用法:
  python3 bss_dump_align.py redroid_bss.bin [base_addr]

对照基准 (静态实锤, §124y-69/70/72):
  - .bss 段全量 = base+0x15be80 - base+0x162e88 (28672B = 0x7008, NOBITS)
  - 低区 5 惰性单例槽: 0x15be80/0x15beb0/0x15beb8/0x15bfd8/0x15bfe4/0x15bfec
    (.bss 偏移 0x00/0x30/0x38/0x158/0x164/0x16c, C++ 惰性构造 scudo 指针)
  - 中间区 0x170-0x6654 (~24KB, C++ 全局对象堆, scudo 运行时填)
  - 高区构造器 2-8 清零区 0x6654-0x6740 (.bss 偏移, .init_array 2-8 直接写 7 对象
    0x162554/0x162558/0x162564/0x162574/0x162580/0x1625bc/0x162640)
  - 高区 locale 2 槽 0x162660/0x162668 (.bss 偏移 0x67E0/0x67E8, .init_array 9-11 C++ locale)
  - 0x15c062 (.bss 偏移 0x1E2, 构造器 0 填 5 元组区, §124y-72 修正版)

判据:
  - 低区 5 槽非 0 = 惰性单例已构造 (scudo 指针)
  - 高区 7 构造器区非 0 = .init_array 2-8 已跑 (清零/标志)
  - 0x15c062 五元组非 0 = 构造器 0 已跑 (§124y-32 实锤)
  - 中间区非零比 (79% = 场景对象图构造态, FRIDA_NOTES §实锤)
"""
import sys, struct

SLOTS = [
    # (offset, va_label, 语义, 期望)
    (0x00,    "0x15be80", ".bss 起点单例槽(析构器5)",  "非0=构造"),
    (0x30,    "0x15beb0", "F_3ab80①事件底层写入",      "非0=惰性构造"),
    (0x38,    "0x15beb8", "F_3ab80⑥事件底层发送",      "非0=惰性构造"),
    (0x158,   "0x15bfd8", "F_5a104 事件容器池槽",      "非0=惰性构造"),
    (0x164,   "0x15bfe4", "5 底层函数共用单例",         "非0=惰性构造"),
    (0x16c,   "0x15bfec", "F_3ab64 single 事件容器",    "非0=惰性构造"),
    (0x1E2,   "0x15c062", "构造器0 填 5 元组区",        "非0=init[0]跑"),
    (0x654,   "0x162554", "构造器2 清零区",             "非0=init[2]跑"),
    (0x658,   "0x162558", "构造器3 清零区",             "非0=init[3]跑"),
    (0x664,   "0x162564", "构造器4 清零区",             "非0=init[4]跑"),
    (0x674,   "0x162574", "构造器5 清零区",             "非0=init[5]跑"),
    (0x680,   "0x162580", "构造器6 清零区",             "非0=init[6]跑"),
    (0x6BC,   "0x1625bc", "构造器7 清零区",             "非0=init[7]跑"),
    (0x740,   "0x162640", "构造器8 清零区",             "非0=init[8]跑"),
    (0x7E0,   "0x162660", "locale 对象堆(slot10)",     "非0=locale构造"),
    (0x7E8,   "0x162668", "locale 对象堆(slot11)",     "非0=locale构造"),
]

BSS_SIZE = 0x7008
MID_LO, MID_HI = 0x170, 0x6654

def main():
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(1)
    path = sys.argv[1]
    base = int(sys.argv[2], 16) if len(sys.argv) > 2 and sys.argv[2] != "auto" else None
    data = open(path, 'rb').read()
    assert len(data) == BSS_SIZE, f".bss dump 大小 {len(data)} != 0x{BSS_SIZE:x}"

    print(f"=== frida .bss dump 对照基准 (§124y-69/70/72) ===")
    print(f"dump: {path} ({BSS_SIZE}B), base={base}")
    print()

    # 1. 关键槽逐个读
    print("--- 16 关键槽 (.bss 偏移 → 值 → 判据) ---")
    constructed_lazy = constructed_init = 0
    lazy_offs = [0, 0x30, 0x38, 0x158, 0x164, 0x16c]
    for off, va, sem, exp in SLOTS:
        v = struct.unpack_from('<I', data, off)[0]
        mark = "★非0" if v else "0   "
        if off in lazy_offs and v:
            constructed_lazy += 1
        if 0x654 <= off <= 0x740 and v:
            constructed_init += 1
        print(f"  偏移0x{off:04x} ({va}): 0x{v:08x} {mark}  [{sem}]")

    # 2. 中间区非零比
    mid = data[MID_LO:MID_HI]
    mid_nz = sum(1 for i in range(0, len(mid), 4) if struct.unpack_from('<I', mid, i)[0] != 0)
    mid_total = len(mid)//4
    pct = mid_nz*100//mid_total
    print(f"\n--- 中间区 0x{MID_LO:04x}-0x{MID_HI:04x} ({mid_total} 槽) ---")
    print(f"  非零比: {mid_nz}/{mid_total} = {pct}% ({'79% 场景对象图构造态实锤' if pct>=70 else ('低, 未构造/未 dump 运行时' if pct<30 else '中间态')})")

    # 3. 事件表 17×17 位掩码区 (0x4704, §124y-109/§124y-153 实锤, .got[0x57c] 基址)
    #    §124y-153 修正: stride = 4B/行 (17-bit 掩码 uint32, 17 行 × 4B = 68B), 非 17 字节/行
    #    ref 原版 so 进程实锤: 行 2/3/6/7/11/12/15/16 非 0 (17-bit 掩码, bit N = 子事件码 N+1)
    EV_LO, EV_ROWS, EV_FIELDS = 0x4704, 17, 4
    ev_off = EV_LO
    rows_with_code = []
    for row in range(EV_ROWS):
        r_off = ev_off + row*EV_FIELDS
        val = int.from_bytes(data[r_off:r_off+4], 'little')
        bits = [n+1 for n in range(17) if (val >> n) & 1]
        if bits:
            rows_with_code.append((row+1, bits))
    print(f"\n--- 事件表 17×17 位掩码区 (偏移 0x4704, stride=4B, §124y-153) ---")
    if rows_with_code:
        for rn, bits in rows_with_code:
            print(f"  行{rn:2d} (主码{rn:2d}): 命中子码 {bits}")
        print(f"  非 0 行 {len(rows_with_code)}/17 = 事件对象图已构造 (ref 原版 so 进程运行时真值)")
    else:
        print(f"  全 17 行 = 0 (翻译体 .bss 全 0 兜底态, C++ 对象图未构造)")
    print(f"  判读: ref 基准 = 行 2/3/6/7/11/12/15/16 非 0 (8 主码), 子码列 1/9/17 非 0 (3 列)")

    # 4. 全段 scudo 指针特征 (0xcf/0xea/0xe8 高位 = scudo 堆)
    scudo_cnt = 0
    for i in range(0, BSS_SIZE, 4):
        v = struct.unpack_from('<I', data, i)[0]
        hi = (v >> 28) & 0xF
        if hi in (0xC, 0xE, 0xF, 0xB):  # 0xcf/0xea/0xe8/0xbb 高位 (scudo/heap)
            scudo_cnt += 1
    print(f"\n--- scudo 堆指针特征 (0xB/0xC/0xE/0xF 高位) ---")
    print(f"  疑似 scudo 堆指针槽: {scudo_cnt} (真值需 cross-check 设备 maps)")

    print(f"\n=== 判读 ===")
    print(f"  低区 6 惰性单例槽非0: {constructed_lazy}/6 ({'全构造' if constructed_lazy==6 else ('部分构造' if constructed_lazy else '全未构造(.bss 未跑/未 dump 运行时)')})")
    print(f"  高区 7 构造器区非0: {constructed_init}/7 ({'init_array 2-8 已跑' if constructed_init==7 else ('部分' if constructed_init else '未跑')})")
    print(f"  中间区非零 {pct}% = {'场景对象图已构造' if pct>=70 else '未构造/非运行时 dump'}")
    print(f"\n诚实边界 (FRIDA_NOTES §实锤): .bss 整段回填翻译体失败(scudo 跨进程指针失效),")
    print(f"  dump 仅拿'值'作对照基准, 非回填源; 翻译体需自身 F_9d3d4 渲染器单例构造")
    print(f"  (sp28-ROOT 已证可行, §124y 系列事件底层单例仍需 unidbg 补 .init_array 12 构造器)")

if __name__ == '__main__':
    main()
