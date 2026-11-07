#!/usr/bin/env python3
"""thunk_code_map.py — 0x55d40 事件业务 thunk 区 73 code→handler 提取 + switch 生成

背景（§124y-31 固化）：本轮反复手敲"objdump 0x55d40-0x55ff4 区 → grep bl handler →
cat -n 按 thunk 序映射"，每次 5 条命令。本工具固化成一条命令：
  1. 提取 0x55d40 区全量 thunk 的 code→handler 映射（bl 出现顺序 = code 顺序）
  2. 生成 F_55d40 switch 的 case 体（直接可粘贴进 jqt_vtbl.c）
  3. 统计不同 handler 数 + 查哪些 handler 已收表（jqt_vtbl.c / jqt_OnCreate_full.c）

用法：
  python3 thunk_code_map.py                    # 提取 + 打印映射 + 生成 switch
  python3 thunk_code_map.py --gen-switch       # 只生成 switch case 体（供粘贴）
  python3 thunk_code_map.py --check            # 查 24 handler 收表状态

依赖：arm-linux-gnueabi-objdump（本机已装）+ 默认 so /tmp/orig/lib/armeabi/libjunqi.so
"""
import re, subprocess, sys, os, argparse

SO = os.environ.get('JQ_SO', '/tmp/orig/lib/armeabi/libjunqi.so')
OBJDUMP = 'arm-linux-gnueabi-objdump'
LO, HI = 0x55d40, 0x56200   # 0x55d40 区 73 thunk + 扩展区（§124y-161: HI 从 0x55ff4 扩到 0x56200, 覆盖扩展区 25 bl）
JQT_VTBL = os.path.join(os.path.dirname(__file__), '..', 'src_jni_translated', 'jqt_vtbl.c')
JQT_OC   = os.path.join(os.path.dirname(__file__), '..', 'src_jni_translated', 'jqt_OnCreate_full.c')

def extract():
    """提取 0x55d40 区全量 thunk 的 code→handler 映射（bl 出现顺序 = code 顺序）

    坑（本轮实证）：objdump 的 --start-address/--stop-address 必须带 0x 前缀
    （写 '55d40' 直接报 'bad number: 55d40'，输出 0 行 → 误判"无 thunk"）。
    正则用 r'bl\\s+([0-9a-f]+)'（objdump 行格式 '   55d46:\\tf7ff fd45 \\tbl\\t557d4 <...>'，
    bl 后是 handler 十六进制偏移，无 0x 前缀）。"""
    r = subprocess.run([OBJDUMP, '-d', f'--start-address=0x{LO:05x}',
                        f'--stop-address=0x{HI:05x}', SO],
                       capture_output=True, text=True)
    bls = re.findall(r'bl\s+([0-9a-f]+)', r.stdout)
    handlers = [int(h, 16) for h in bls]
    mapping = {i: h for i, h in enumerate(handlers)}
    return mapping, handlers

def gen_switch(mapping):
    """生成 F_55d40 switch 的 case 体（code→handler）"""
    lines = []
    for code in sorted(mapping):
        lines.append(f"      case {code}: h = 0x{mapping[code]:05x}; break;")
    lines.append("      default: break;")
    return "\n".join(lines)

def check_handlers(handlers):
    """查 handler 收表状态（jqt_vtbl.c + jqt_OnCreate_full.c 定义体）"""
    srcs = []
    for f in (JQT_VTBL, JQT_OC):
        if os.path.exists(f):
            with open(f, encoding='utf-8') as fh:
                srcs.append(fh.read())
    print(f"=== {len(set(handlers))} 个不同 handler 的收表状态 ===")
    missing = []
    for h in sorted(set(handlers)):
        fn = f"F_{h:05x}"
        n = sum(s.count(f"uint32_t {fn}(uint32_t _r0") for s in srcs)
        mark = "✓" if n > 0 else "★缺"
        if n == 0:
            missing.append(fn)
        print(f"  {mark} {fn}")
    if missing:
        print(f"\n=== {len(missing)} 个 handler 缺定义体（需补全）===")
        for fn in missing:
            print(f"  {fn}")
    return missing

# §124y-33：24 handler 忠实返回语义表（原版反汇编实锤，.text 无 movs r0,#2）
RETURN_SEM = {
    0x557d4: ("getter", "ldr r0,[r0,#0x84]; bx lr → [single+0x84]（.bss 未构造=0）"),
    0x5586c: ("bl链",  "末尾 bl b3ae4/b2bf4/8548c → 返回 bl 目标 ret（字符串/字段填充）"),
    0x557d0: ("getter", "adds r0,#0x6c; bx lr → single+0x6c"),
    0x557cc: ("getter", "adds r0,#0x54; bx lr → single+0x54"),
    0x8571c: ("getter", "0x38==0?ret=0:bl 0x94d80(+=0x70) → v+0x70（bl 0x94d80 区 5 独立小 getter）"),
    0x85798: ("bl链",  "0x38!=0→bl 0x8c8c8(事件底层方法 0x44/0x54/0x68 三调用)→返回底层 ret；0x38=0→早退"),
    0x857b0: ("bl链",  "写 0x3c 字段 + (0x38!=0→blx [底层+0x44]) → 返回 0x85348 ret"),
    0x85348: ("blx",   "0x38!=0→blx [底层vtable+0x44]（vtable 方法调用）→ 返回方法 ret；0x38=0→ret=0"),
    0xb549c: ("single", "str 4 字段（vptr+0/4/8/12）→ pop r0=r4（入参 single）"),
    0x85704: ("getter", "0x38!=0→bl 0x94da0(=vtable 0xd8 槽 ldr) → 返回 0xd8 槽值；0x38=0→ret=0"),
    0xb54dc: ("single", "str r1,[r0+4/8] → pop r0=r4（入参 single）"),
    0xb52a0: ("构造",  "复杂 C++ 构造（b5a8c 多次）→ 返回构造结果（.bss 未构造=0）"),
    0x33878: ("single", "bl 0x33848 + 0x33724 + [r4] 坐标增减 → pop 末态 r0=r4（入参 single）"),
    0x85744: ("bl链",   "bl 0x8c8c8（事件底层方法）→ 返回底层 ret"),
    0x85730: ("getter", "strlen 类（adds r2,r4,r0 循环）→ 返回长度"),
    0x85758: ("bl链",   "bl 0x8426c（底层方法）→ 返回底层 ret"),
    0x55dc4: ("bl链",   "事件对象构造链（bl b549c/85704/b54dc/b52a0 + bl 85730/85744/85758）→ 返回 85758 ret（.bss 未构造=0）"),
    0x55ba4: ("single", "bl 85858 + 字段填充 str [r4+0x44/40/48/4c/84/88/90/94] + bl b278c×2 + bl 55ad0 → pop 末态 r0=r4（入参 single）"),
    0x557c8: ("single", "str r1,[r0+#0x50]; bx lr → pop r0=r0（入参 single）"),
    0x82d3c: ("构造",   "memcpy 业务（ldr [r5,r6] 拷贝循环）→ 返回目标指针"),
    0xb4edc: ("构造",   "复杂 C++ 构造（0x44/0x54/0x68/0x84/0x94/0xa0 字段填充）→ 返回构造结果"),
    0x3deb0: ("bl链",   "ldr [pc]→bl 0x279cc（ldrb r0,[r0,#0x60] 字节 getter）→ 返回 [base+0x60]"),
    0x853c0: ("bl链",   "0x38!=0→bl 0x84118（底层深层）→ 返回底层 ret；0x38=0→早退 ret=0"),
    0x3f8f4: ("bl链",   "字符串构造（bl 0x23bec）+ bl 0x857b0（写 0x3c + blx [底层+0x44]）→ 返回 857b0 ret"),
}

def return_sem(handlers):
    """§124y-33：code→handler→返回语义 三列对照表（复用）"""
    mapping, _ = extract()
    print("=== 24 handler 忠实返回语义表（原版 .text 无 movs r0,#2——全量 handler 无一真返回 2）===")
    print(f"{'code':>4} {'handler':>10} {'类别':<6} 返回语义")
    for code in sorted(mapping):
        h = mapping[code]
        cls, sem = RETURN_SEM.get(h, ('?','未收表'))
        print(f"{code:>4}  0x{h:05x}  {cls:<6} {sem}")
    print("\n★ F_3ab64 ok 判读（subs r3,r0,#2; negs; adcs abs 序列）：ret==2→1（成功，设备实锤）；忠实化后")
    print("  .bss 未构造时各 handler 返回 0/指针 → code4（F_8571c）ret=0 → ok=0（忠实失败路径，非 2 兜底）")
    return 0

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--gen-switch', action='store_true', help='只生成 switch case 体')
    ap.add_argument('--check', action='store_true', help='查 handler 收表状态')
    ap.add_argument('--return-sem', action='store_true',
                   help='§124y-33 code→handler→返回语义 三列对照表')
    args = ap.parse_args()
    mapping, handlers = extract()
    if args.return_sem:
        return return_sem(handlers)
    print(f"=== 0x55d40 区 {len(mapping)} 个 code 的 handler 映射（{len(set(handlers))} 个不同 handler）===")
    for code in sorted(mapping):
        print(f"  code={code:2d} → 0x{mapping[code]:05x} (F_{mapping[code]:05x})")
    if args.gen_switch:
        print(f"\n=== F_55d40 switch case 体（{len(mapping)} case，供粘贴）===")
        print(gen_switch(mapping))
    if args.check:
        check_handlers(handlers)
    return 0

if __name__ == '__main__':
    sys.exit(main())
