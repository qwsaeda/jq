#!/usr/bin/env python3
"""so_link_check.py — 检查 .so 的 UND GL 符号能否被 MuMu 系统库解析
用法:
  python3 so_link_check.py <so路径>                 # 离线核对（用本地缓存的 MuMu 符号表）
  python3 so_link_check.py <so路径> --update-syms    # 从设备拉取 MuMu 库更新符号缓存
缓存: /tmp/opencode/mumu_und_syms/（libGLESv1_CM.so 等 symbols）
判据: 每个 UND gl*/egl* 符号必须在 MuMu 某库的导出表里，否则 dlopen "failed to link"
"""
import subprocess, sys, re, os, json, glob

MUMU_LIBS = {
    'libEGL':       '/system/lib64/arm64/libEGL.so',
    'libGLESv1_CM': '/system/lib64/arm64/libGLESv1_CM.so',
    'libGLESv2':    '/system/lib64/arm64/libGLESv2.so',
    'libandroid':   '/system/lib64/libandroid.so',
    'libc':         '/system/lib64/libc.so',
}
# 本地已有缓存的 MuMu 64 位库（之前 pull 过，直接用，避免重复 pull）
# 本地已有缓存的 MuMu 库（优先用本地，避免重复 pull；路径不存在则 fallback 到 adb pull）
LOCAL_MUMU = {
    'libGLESv1_CM': '/tmp/opencode/mumu_glesv1_cm.so',
    'libGLESv2':    '/tmp/opencode/mumu_x86_GLESv2.so',
    'libc':         None,   # libc 不需要（UND 里的 libc 符号必然可解析）
}
# MuMu 设备库路径（adb pull 用；libEGL 等没有本地缓存的走这里）
DEV = '127.0.0.1:5555'
CACHE = '/tmp/opencode/mumu_und_syms'

def get_exports(so_file):
    """提取 .so 的动态导出符号集合（nm -D --defined-only，去掉 @@VERSION 后缀）"""
    out = subprocess.run(['nm', '-D', '--defined-only', so_file],
                         capture_output=True, text=True).stdout
    syms = set()
    for line in out.splitlines():
        # 格式: "0000000000006468 T glActiveTexture@@LIBGLESV1_CM"
        parts = line.split()
        if len(parts) >= 3 and parts[1] in ('T', 't', 'W', 'i'):
            syms.add(parts[-1].split('@@')[0])  # 去掉版本标签
    return syms

def update_syms():
    os.makedirs(CACHE, exist_ok=True)
    all_syms = {}
    for name, path in MUMU_LIBS.items():
        src = LOCAL_MUMU.get(name, '')
        if src and os.path.exists(src):
            syms = get_exports(src)
            all_syms[name] = sorted(syms)
            print(f"  {name}: 本地 {src}（{len(syms)} 导出符号）")
        elif src is None:
            # libc 等必然可解析，跳过
            all_syms[name] = []
        else:
            # adb pull（libEGL/libandroid 无本地缓存）
            local_pull = os.path.join(CACHE, name + '_pulled.so')
            r = subprocess.run(['adb', '-s', DEV, 'pull', path, local_pull],
                               capture_output=True, text=True)
            if os.path.exists(local_pull):
                syms = get_exports(local_pull)
                all_syms[name] = sorted(syms)
                print(f"  {name}: adb pull {path}（{len(syms)} 导出符号）")
            else:
                all_syms[name] = []
                print(f"  {name}: adb pull 失败（{r.stderr.strip()[:60]}），跳过")
    with open(os.path.join(CACHE, 'all_syms.json'), 'w') as f:
        json.dump(all_syms, f, indent=2)
    total = sum(len(v) for v in all_syms.values())
    print(f"★ 已更新 MuMu 符号缓存: {CACHE}/（共 {total} 个导出符号）")
    return all_syms

def load_syms():
    f = os.path.join(CACHE, 'all_syms.json')
    if not os.path.exists(f):
        return {}
    with open(f) as fh:
        return json.load(fh)

def main():
    if len(sys.argv) < 2:
        print(__doc__); return 1
    so = sys.argv[1]
    update = '--update-syms' in sys.argv
    if update:
        all_syms = update_syms()
    else:
        if not os.path.exists(CACHE):
            print(f"缓存不存在，先跑: python3 {sys.argv[0]} {so} --update-syms")
            return 1
        all_syms = load_syms()
        if not all_syms:
            print(f"缓存为空，先跑: python3 {sys.argv[0]} {so} --update-syms")
            return 1

    # 提取 so 的 UND 符号（nm -D 不截断）
    out = subprocess.run(['nm', '-D', so], capture_output=True, text=True).stdout
    und = []
    for line in out.splitlines():
        parts = line.split()
        # UND 行: "                 U AAsset_close"  → parts = ['U', 'AAsset_close']
        # 带版本: "                 U calloc@LIBC"   → parts = ['U', 'calloc@LIBC']
        if len(parts) == 2 and parts[0] == 'U':
            und.append(parts[1].split('@')[0])
        # 也兼容 3 列带地址格式（已定义符号，非 UND）

    # 所有 MuMu 库导出符号合并
    all_exported = set()
    for syms in all_syms.values():
        all_exported.update(syms)

    missing = [s for s in und if s not in all_exported]
    gl_missing = [s for s in missing if s.startswith(('gl', 'egl'))]
    print(f"so: {so}")
    print(f"UND 总数: {len(und)}, 其中 GL/EGL: {sum(1 for s in und if s.startswith(('gl','egl')))}")
    if gl_missing:
        print(f"\n★ FAIL: {len(gl_missing)} 个 GL/EGL 符号在 MuMu 无导出（dlopen 会 failed to link）:")
        for s in gl_missing:
            print(f"    缺: {s}")
        return 2
    else:
        print("★ PASS: 所有 GL/EGL UND 符号 MuMu 可解析（dlopen 链接安全）")
        return 0

if __name__ == '__main__':
    sys.exit(main())
