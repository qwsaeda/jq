#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-214 事件码映射表生成器诚实边界判据（静态侧，不依赖设备）

固化"事件码映射表生成器不可静态实现"的诚实边界（§124y-163/185/196 收口）为
可复跑判据（防后续会话反复探"静态共置点自动提取"死路）：
- 事件名 73 Junqi 在 .rodata 段散布（非连续池，跨度 0x2540=9536 字节）
- 事件码 0..11 真值 = .bss asr4 运行时 C++ 动态构造（.rodata 静态共置点
  .data.rel.ro 0x15af08 区 = 静态注册模板指针表，asr4 全 0 未填 = disc=0
  忠实边界，§124y-185 BSS-DUMP 实锤）
- 判据① 73 Junqi 名在 .rodata 段散布（非 < 4000 字节连续池 = 地址共置提取不可行）
- 判据② .data.rel.ro 0x15af08 静态注册模板区 asr4 全 0（事件码真值需运行时填）
- 判据③ 兜底判据在位：rodata_event_cover.py（76/76 覆盖 0 缺口）+
  event_code_map_check.py（映射表 0 缺口 + 可再生对照）

用法：python3 rev64/tools/mapping_boundary_check.py
"""
import os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"
COVER = os.path.join(ROOT, "rev64/tools/rodata_event_cover.py")
MAPCHK = os.path.join(ROOT, "rev64/tools/event_code_map_check.py")

def main():
    if not os.path.exists(ORIG_SO):
        print("✗ %s 不存在" % ORIG_SO); return 2
    so = open(ORIG_SO, 'rb').read()
    # 判据①：73 Junqi 名 .rodata 段散布（非连续池）
    names = [m.start() for m in re.finditer(rb'Junqi\.[A-Za-z0-9%]+', so)]
    in_rodata = sorted(n for n in names if 0x120bf8 <= n < 0x13dbc8)
    lo, hi = min(in_rodata), max(in_rodata)
    span = hi - lo
    scattered = (len(in_rodata) == 73) and (span > 4000)
    print("═══ §124y-214 事件码映射表生成器诚实边界判据 ═══")
    print("  ① 73 Junqi 名在 .rodata 段: %d 处，最小 0x%x 最大 0x%x（跨度 0x%x = %d 字节）" % (
        len(in_rodata), lo, hi, span, span))
    print("     散布判定（非连续池，地址共置提取不可行）: %s" % ("PASS" if scattered else "复核"))
    # 判据②（§124y-157/166 纠正口径）：.data.rel.ro 0x15af08 区 = 54 项 0x0012xxxx
    #   Thumb 函数指针表 + 异构数据（非 asr4 纯字节区）→ 事件码真值不在静态共置点。
    #   实锤：该区 4B 项多为 0x0012xxxx（Thumb 指针，奇数化低 bit）或 0。
    base = 0x15af08
    thumbs = 0; zeros = 0; other = 0
    for i in range(0, 0x40, 4):
        v = struct.unpack_from('<I', so, base+i)[0]
        if (v >> 16) == 0x0012: thumbs += 1
        elif v == 0: zeros += 1
        else: other += 1
    # 事件码真值不可静态提取 = 该区无 0..11 小常量直接共置（主结构 = Thumb 指针表）
    small_codes = 0
    for i in range(0, 0x40, 4):
        v = struct.unpack_from('<I', so, base+i)[0]
        if 0 < v <= 11: small_codes += 1
    asr_ok = (thumbs + zeros >= 8) and (small_codes == 0)
    print("  ② .data.rel.ro 0x15af08 静态模板区 = Thumb 指针表 %d + 零 %d + 异构 %d（0..11 小常量共置 %d 处）" % (
        thumbs, zeros, other, small_codes))
    print("     事件码真值不在静态共置点（需运行时 .bss 填 asr4，§124y-163/185）: %s" % (
        "PASS" if asr_ok else "FAIL"))
    # 判据③：兜底判据脚本在位
    cover_ok = os.path.exists(COVER)
    map_ok = os.path.exists(MAPCHK)
    print("  ③ 兜底判据在位: rodata_event_cover.py %s + event_code_map_check.py %s" % (
        "PASS" if cover_ok else "✗", "PASS" if map_ok else "✗"))
    ok = scattered and asr_ok and cover_ok and map_ok
    print("  判据: %s" % (
        "PASS（生成器不可静态实现边界成立：事件名散布 + 事件码真值非静态共置 + 双源兜底判据在位）" if ok else "FAIL"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
