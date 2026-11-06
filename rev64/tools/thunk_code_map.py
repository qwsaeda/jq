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
LO, HI = 0x55d40, 0x55ff4   # 0x55d40 区 73 thunk（0x12 × 73 = 0x2B4 → 0x55FF4）
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

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--gen-switch', action='store_true', help='只生成 switch case 体')
    ap.add_argument('--check', action='store_true', help='查 handler 收表状态')
    args = ap.parse_args()
    mapping, handlers = extract()
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
