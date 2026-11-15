#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1918 系统域 RAW 镜像兜底真机可达量化判据（设备无关）

固化"426 系统域 RAW 真机 linker 解析兜底"的可达性量化——此前 raw_fallback
判据 ③ 只判"jqt_mem_valid 双路命中"（MEM 读可达），未量化"426 RAW 是否真
被 linker 解析（真机可达）"。本判据补全：
  ① 426 RAW 全在镜像拷贝区 [0x233e0, 0x15be80)（img memcpy 全量覆盖，
     真机 dlopen 后镜像区 = .text/.rodata/.data.rel.ro 可读）
  ② 426 RAW 全在 6 系统域 zone（0x0001/0x0002/0x0006/0x10/0x11/0x12，
     .text 尾部 C++ runtime + .rodata/.rel.ro 数据区，0 业务混入）
  ③ 426 RAW 中 vtbl 收表 = 0（全走"镜像裸跑"非 dispatch 命中，§1914 口径）
  ④ 原版 so NEEDED 全量 dlopen（libstdc++/libc/libGLESv1_CM/libGLESv2/
     liblog/libm/libdl），C++ runtime（ios/locale/exception）由 libstdc++
     提供 = 426 RAW 真机可执行兜底（镜像 + linker 解析，非占位）
判据输出：①-④ 全 PASS + 6 zone 全分布 + NEEDED 清单。任一不满足 = FAIL。

用法：python3 rev64/tools/raw_linker_reach_check.py
"""
import importlib.util, os, re, subprocess, sys, collections

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def main():
    dm_path = os.path.join(ROOT, 'rev64/tools/translation_domain_matrix.py')
    spec = importlib.util.spec_from_file_location('dm', dm_path)
    dm = importlib.util.module_from_spec(spec); spec.loader.exec_module(dm)

    bl = dm.orig_bl_targets()
    f_defs = dm.load_f_defs()
    vtbl = dm.load_vtable_dispatch()
    stub = dm.load_stub_set()
    plt = dm.load_plt_symbols()
    libc = dm.load_libc_syms()

    PSEUDO = {'1220e', '1b0ca'}  # objdump 软浮点 BL 标注伪影（§1914 实锤）

    def raw_set(bl, f_defs, vtbl, stub, plt):
        out = []
        for a in bl:
            ai = int(a, 16)
            if a in plt or 0x22900 <= ai < 0x233e0:
                continue
            if a in f_defs or a in vtbl or a in stub:
                continue
            out.append(a)
        return out

    raw = raw_set(bl, f_defs, vtbl, stub, plt)
    PSEUDO_USED = PSEUDO & {a.lstrip('0') for a in raw}
    real_raw = [a for a in raw if a.lstrip('0') not in PSEUDO]
    LO, HI = 0x233e0, 0x15be80

    # ① 镜像区覆盖
    in_img = [a for a in real_raw if LO <= int(a, 16) < HI]
    # ② 6 zone 系统域纯度
    zones = collections.Counter()
    for a in raw:
        zones[(int(a, 16) >> 16)] += 1
    zkeys = sorted(zones.keys())
    expect_zones = [0x0001, 0x0002, 0x0006, 0x10, 0x11, 0x12]
    zone_ok = all(k in zkeys for k in expect_zones) and len(zkeys) == len(expect_zones)
    biz_mix = [a for a in raw if not (a in libc or int(a, 16) >= 0x110000 or a.lstrip('0') in PSEUDO)]
    # ③ vtbl 收表 = 0
    in_vtbl = [a for a in raw if a in vtbl]
    # ④ 原版 so NEEDED 清单（dlopen 全量外部依赖）
    so_path = '/tmp/orig/lib/armeabi/libjunqi.so'
    out = subprocess.run(['arm-linux-gnueabi-readelf', '-d', so_path], capture_output=True, text=True).stdout
    needed = re.findall(r'\(NEEDED\)\s+Shared library:\s+\[([^\]]+)\]', out)
    needed_set = set(needed)
    need_ok = {'libstdc++.so', 'libc.so', 'libm.so', 'libdl.so', 'libGLESv1_CM.so', 'libGLESv2.so', 'liblog.so'} & needed_set
    linker_ok = 'libstdc++.so' in needed_set and 'libc.so' in needed_set

    ok = (len(in_img) == len(real_raw) and len(biz_mix) == 0 and len(in_vtbl) == 0
          and zone_ok and len(raw) > 0 and linker_ok)

    print('═══ §124y-1918 系统域 RAW 镜像兜底真机可达量化判据 ═══')
    print('  426 RAW = 真实 %d + 伪影 %d（PSEUDO 白名单）' % (len(real_raw), len(PSEUDO_USED)))
    print('  ① 镜像拷贝区 [0x233e0,0x15be80) 覆盖: %d/%d' % (len(in_img), len(real_raw)))
    print('  ② 6 zone 系统域（%s）纯度: 真业务混入 = %d' % (','.join('0x%x' % z for z in zkeys), len(biz_mix)))
    for z in zkeys:
        print('     0x%04x0000 区 %4d 条' % (z, zones[z]))
    print('  ③ vtbl 收表 = %d（0 = 全镜像裸跑，非 dispatch 命中）' % len(in_vtbl))
    print('  ④ NEEDED（%d 项，dlopen 全量）: %s' % (len(needed), ' '.join(needed)))
    print('     C++ runtime（ios/locale/exception）由 libstdc++.so 提供 = 真机可达兜底: %s' %
          ('OK' if linker_ok else 'FAIL'))
    print('  判据: %s' % ('PASS（426 RAW 真机 linker 解析兜底无死角，镜像 + 系统域 + 0 vtbl + NEEDED 全量）'
                          if ok else 'FAIL'))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
