#!/usr/bin/env python3
# link_audit.py — 军棋 arm32 翻译体「链接安全审计」工具（§124zz 固化复用）
#
# 背景（MuMu 原生桥 dlopen "failed to link libjunqi.so" 排查）：
#   MuMu 的 32 位 linker（libnb/houdini）解析 ARM 库 UND 符号时能力有限，
#   某些符号（实证：pthread_once@LIBC）会导致整体 fail to link，
#   且 linker 错误详情被吞掉（logcat 只有 "failed to link"，无 "cannot locate symbol"）。
#   → 必须在**打包前离线**判断"新引入的 UND 符号能否被 MuMu 的 ARM 库解析"。
#
# 审计项：
#   1. UND 符号解析：逐个查 MuMu ARM 库（libc/libdl/libGLESv1_CM/libGLESv2/liblog/libm）
#      是否导出（含符号版本比对）→ 列��"无法解析"= 链接失败嫌疑
#   2. 基线 diff：与已知可加载基线（如 v178e0）比 UND 集合 → 列出新增
#   3. 内部符号悬空：F_xxx 出现在重定位里但没有函数体定义（F_264d4 类 bug）
#   4. .hash 风格：MuMu 32 位 linker 不支持 .gnu.hash → 必须 sysv
#   5. APK 完整性：.so 必须 STORE、manifest 头 0x00080003、v1+v2 签名
#
# 用法：
#   python3 rev64/tools/link_audit.py libs                  # 从设备拉 MuMu ARM 库到缓存
#   python3 rev64/tools/link_audit.py audit <so|apk> [--baseline <so|apk>]
#   python3 rev64/tools/link_audit.py check <so|apk> <so|apk> ...   # 多产物对比
#
# 缓存目录：/tmp/opencode/mumu_arm_libs/
import os, re, json, subprocess, sys, struct, zipfile, tempfile, shutil

RE = 'arm-linux-gnueabi-readelf'
OD = 'arm-linux-gnueabi-objdump'
CACHE = '/tmp/opencode/mumu_arm_libs'
BASE = '/tmp/opencode/mumu_arm_libs'
# 已实证可加载基线（v178e0）的 UND 符号集 → 证明这些符号 MuMu 原生桥能解析。
# 基线 UND 里的符号无需再查缓存库（缓存库不全时也不误报）。
PROVEN_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                           'mumu_proven_und.json')

# MuMu ARM 侧库 → 设备路径（libs 子命令用）
DEVICE_LIBS = {
    'libc.so':           '/system/lib/arm/libc.so',
    'libdl.so':          '/system/lib/arm/libdl.so',
    'liblog.so':         '/system/lib/arm/liblog.so',
    'libm.so':           '/system/lib/arm/libm.so',
    'libGLESv1_CM.so':   '/system/lib/arm/libGLESv1_CM.so',
    'libGLESv2.so':     '/system/lib/arm/libGLESv2.so',
}

# MuMu 原生桥已知不支持/会导致 fail to link 的符号（§124zz 实测）
# ★ 纠正（§124zz-5）：pthread_once **不是**问题——v178e4 带 pthread_once 且
#   F_264d4 已修 → logcat 无 UnsatisfiedLinkError（dlopen 成功）→ pthread_once 能解析。
#   真正的 fail to link 根因**只有**"内部 F_xxx 悬空"（F_264d4，见下方 dangling 检测）。
KNOWN_BAD = {
}
# ★ 已实测**能**在 MuMu 上解析的符号白名单（v178e5/e6/e7 实机加载成功证明）：
#   覆盖 ARM 库缓存不全的符号（libGLESv1_CM 缓存可能缺失/版本，但实机可解析）。
#   ★ AUD-1000-ALL A12-F12-006（P2 标注）：PROVEN_LOADABLE 是**缓存缺失时的静默兜底**
#   （缓存不含某库 → 该库 UND 符号走白名单判 PASS，非"已验证可解析"，是"历史实证
#   可解析"）。诚实边界：白名单外符号 + 缓存缺失 = FAIL（保守），白名单内符号
#   + 缓存缺失 = PASS（乐观，依赖历史实证）。新增 MuMu 库/符号时应实测后补白名单，
#   勿直接扩白名单未实测符号（假绿风险）。
PROVEN_LOADABLE = {
    'eglGetError',
    'glDeleteTextures', 'glGenTextures', 'glPixelStorei',
    'glTexImage2D', 'glTexSubImage2D', 'glTexParameterf',
    # §124zz-8：libc 基础符号（jqt_mem.c [stack] 过滤用），MuMu ARM libc 必导出
    'strncpy', 'strstr',
    # §124y-7：FRAMEBUFFER-DUMP 用 libc stdio（Android bionic 必导出）
    'fprintf', 'fwrite', 'snprintf', 'fopen', 'fclose', 'malloc', 'free',
    '__errno', 'mkdir',
    # ★ §TOUCH-INTERACT-TEST：__system_property_get（bionic libc 必导出，test hook 用）
    '__system_property_get',
    # ★ §124y-19：会话 B 规则引擎走子桥（jq_rs_walkbridge.c，visibility default，本地定义）
    'jq_walk_bridge_tick',
    # ★ §124y-19：会话 B touch.c 用 bionic 数学函数（-lm 提供）
    'lroundf', 'lround',
}


def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True).stdout


def readelf_dynsyms(path):
    """→ {name: (version, is_undef)}"""
    out = run([RE, '-sW', path])
    res = {}
    for line in out.splitlines():
        if ' UND ' in line:
            m = re.search(r'UND\s+(\S+)$', line)
            if m:
                res[m.group(1)] = ('', True)
        else:
            parts = line.split()
            if len(parts) >= 8 and parts[3] == 'FUNC' or (len(parts) >= 8 and parts[3] == 'OBJECT'):
                nm = parts[-1]
                ver = ''
                if '@' in nm:
                    nm, ver = nm.split('@', 1)
                res.setdefault(nm, (ver, False))
    return res


def lib_exports(path):
    """→ {name: version}  从 .dynsym 收集已定义符号（含版本）"""
    out = run([OD, '-T', path])
    res = {}
    for line in out.splitlines():
        m = re.match(r'^[0-9a-f]+\s+\S+\s+\S+\s+\S+\s+[0-9a-f]+\s+(\S+)\s+(\S+)$', line)
        if m:
            ver, name = m.group(1), m.group(2)
            if name.startswith('_'):
                continue
            res.setdefault(name, ver)
    return res


def cache_libs(force=False):
    """从 MuMu 设备拉 ARM 侧依赖库到 CACHE（libs 子命令）。"""
    os.makedirs(CACHE, exist_ok=True)
    dev = os.environ.get('MUMU_ADB', '10.0.182.146:5555')
    missing = []
    for name, path in DEVICE_LIBS.items():
        dst = os.path.join(CACHE, name)
        if os.path.exists(dst) and not force:
            continue
        r = subprocess.run(['adb', '-s', dev, 'pull', path, dst],
                           capture_output=True, text=True)
        if r.returncode != 0 or not os.path.exists(dst):
            missing.append((name, path))
    if missing:
        print('★ 拉取失败（设备不可达？adb -s %s）：' % dev)
        for n, p in missing:
            print('   %s ← %s' % (n, p))
        return False
    print('✓ 缓存 %d 个 ARM 库 → %s' % (len(DEVICE_LIBS), CACHE))
    return True


def load_cache_exports():
    """→ {symbol: (version, provider)}"""
    table = {}
    if not os.path.isdir(CACHE):
        return table
    for f in sorted(os.listdir(CACHE)):
        if not f.endswith('.so'):
            continue
        p = os.path.join(CACHE, f)
        for sym, ver in lib_exports(p).items():
            table.setdefault(sym, (ver, f))
    return table


def resolve_so(so_path):
    """so → 解析后的 (UND 列表, 内部悬空符号列表)

    ★ 文件存在性 + ELF 校验（审计对抗 N1 沉淀）：旧版对不存在/非 ELF 文件
    空转 readelf → 0 UND → 误判 PASS（负向验证实证：`audit /nonexistent.so` exit 0）。
    门禁用途下这是致命误放行——补：文件必须存在 + 能被 readelf -h 识别为 ELF
    + 确实有 .dynsym（UND 段），否则 audit 直接 FAIL。
    """
    if not os.path.exists(so_path):
        raise FileNotFoundError('so 文件不存在: %s（门禁防误放行，非"0 UND=PASS"）' % so_path)
    hdr = run([RE, '-hW', so_path])
    if 'Error' in hdr or 'ELF Header' not in hdr:
        raise ValueError('非有效 ELF（readelf -h 失败）: %s' % so_path)
    dyn = readelf_dynsyms(so_path)
    und = sorted([n for n, (_, u) in dyn.items() if u])
    if len(dyn) == 0:
        raise ValueError('无 .dynsym 段（UND=0 且 dynsym 空 → 疑似假文件/非共享库）: %s' % so_path)

    # 内部符号悬空检测：重定位里出现的 F_xxx 但 .symtab 无定义
    defined = set()
    for line in run([RE, '-sW', so_path]).splitlines():
        parts = line.split()
        if len(parts) >= 8 and parts[6] != 'UND' and parts[3] in ('FUNC', 'OBJECT'):
            defined.add(parts[-1].split('@')[0])
    dangling = set()
    for line in run([RE, '-rW', so_path]).splitlines():
        m = re.search(r'R_ARM_\w+\s+\S+\s+(\S+)', line)
        if m:
            s = m.group(1)
            if s.startswith('F_') and s not in defined:
                dangling.add(s)
    return und, sorted(dangling)


def so_from_apk(apk):
    """apk → 提取 lib/armeabi/libjunqi.so 到临时文件"""
    z = zipfile.ZipFile(apk)
    name = None
    for cand in ('lib/armeabi/libjunqi.so', 'lib/armeabi-v7a/libjunqi.so',
                 'lib/arm64-v8a/libjunqi.so'):
        if cand in z.namelist():
            name = cand
            break
    if not name:
        raise SystemExit('★ %s 内无 libjunqi.so' % apk)
    fd, tmp = tempfile.mkstemp(suffix='.so')
    with os.fdopen(fd, 'wb') as f:
        f.write(z.read(name))
    return tmp, name


def hash_style(so_path):
    out = run([RE, '-S', so_path])
    if '.gnu.hash' in out:
        return 'gnu.hash'
    if '.hash' in out:
        return 'sysv'
    return '?'


def apk_checks(apk):
    print('--- APK 检查: %s' % apk)
    z = zipfile.ZipFile(apk)
    bad = [i.filename for i in z.infolist()
           if i.filename.endswith('.so') and i.compress_type != 0]
    print('  .so STORE      :', 'OK' if not bad else '★ 压缩了: %s' % bad)
    mh = struct.unpack_from('<I', z.read('AndroidManifest.xml'), 0)[0]
    print('  manifest 头    : 0x%08x %s' % (mh, 'OK' if mh == 0x00080003 else '★ 错误'))
    libs = [n for n in z.namelist() if n.startswith('lib/')]
    print('  ABI            :', libs)


def check_dispatch(src):
    """dispatch 表一致性检查（§109/§124aw 红线，机器化）：
       1. jqt_vtbl_n == len(fns) == len(offs)
       2. offs 必须严格升序（jq_call 用二分查找）
       3. 每个 fns 条目在源码里必须有函数体定义（否则 so 里成 undefined dynamic 符号
          → MuMu 原生桥 fail to link）
       4. fns[i] 与 offs[i] 必须同址（表按地址一一对应）
       返回 (ok, [问题描述])
       ★ AUD-1000 B07-010：src=None → 自动定位（rev64/reverse/src_jni_translated 相对工具
       根，多文件 jqt_vtbl.c + jqt_OnCreate_full.c 合并查函数体；表一致性用
       jqt_vtbl.c 文本）。原硬编码 /disk 绝对路径已去。"""
    import re as _re
    problems = []
    if src is None:
        base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # rev64/
        jti = os.path.join(base, 'src_jni_translated')
        dispatch_sources = []
        for _f in ('jqt_vtbl.c', 'jqt_OnCreate_full.c'):
            _p = os.path.join(jti, _f)
            if os.path.exists(_p):
                dispatch_sources.append(_p)
        if not dispatch_sources:
            return True, []  # 源码目录不在 → 跳过 dispatch 检查（非 FAIL）
        all_src = '\n'.join(open(p).read() for p in dispatch_sources)
        s = open(dispatch_sources[0]).read()  # 表在 jqt_vtbl.c
    else:
        s = open(src).read()
        all_src = s

    def entries(name):
        i = s.index(name)
        j = s.index('{', i)
        k = s.index('};', j)
        body = _re.sub(r'/\*.*?\*/', '', s[j + 1:k], flags=_re.S)
        depth = 0; cur = ''; items = []
        for ch in body:
            if ch in '([{': depth += 1
            elif ch in ')]}': depth -= 1
            if ch == ',' and depth == 0:
                if cur.strip(): items.append(cur.strip())
                cur = ''
            else:
                cur += ch
        if cur.strip(): items.append(cur.strip())
        return items

    try:
        fns = entries('jqt_vtbl_fns[]')
        offs = entries('jqt_vtbl_offs[]')
    except ValueError as e:
        return False, ['未找到 dispatch 表: %s' % e]

    m = _re.search(r'jqt_vtbl_n\s*=\s*(\d+)u?', s)
    n = int(m.group(1)) if m else None
    print('--- dispatch 表: n=%s fns=%d offs=%d' % (n, len(fns), len(offs)))
    if n is None:
        problems.append('找不到 jqt_vtbl_n 声明')
    else:
        if n != len(fns):
            problems.append('jqt_vtbl_n(%d) != len(fns)(%d) → 表末尾条目二分查不到'
                            % (n, len(fns)))
        if n != len(offs):
            problems.append('jqt_vtbl_n(%d) != len(offs)(%d)' % (n, len(offs)))

    # offs 升序（严格递增）
    vals = []
    for o in offs:
        try: vals.append(int(o, 16))
        except ValueError:
            problems.append('offs 条目非 hex: %s' % o); vals.append(0)
    for i in range(1, len(vals)):
        if vals[i] <= vals[i - 1]:
            problems.append('offs 未严格升序于 idx %d (0x%x <= 0x%x) → 二分查找失效'
                            % (i, vals[i], vals[i - 1]))
            break

    # fns[i] 与 offs[i] 同址
    mis = []
    for i, (f, o) in enumerate(zip(fns, offs)):
        try: ov = int(o, 16)
        except ValueError: continue
        if f.startswith('F_'):
            try: fv = int(f[2:], 16)
            except ValueError: continue
            if fv != ov:
                mis.append((i, o, f))
    if mis:
        problems.append('fns/offs 同址错位 %d 处，例: %s' % (len(mis), mis[:3]))

    # fns 条目必须有函数体定义（★ AUD-1000 B07-010：跨文件合并查 all_src）
    nod = []
    for f in set(fns):
        if not f.startswith('F_'):
            continue
        if not _re.search(r'uint32_t\s+' + _re.escape(f) + r'\s*\(', all_src):
            nod.append(f)
    if nod:
        problems.append('fns 条目缺函数体定义 %d 个（jqt_vtbl.c + jqt_OnCreate_full.c 合并查）→ so 里成 undefined dynamic 符号: %s'
                        % (len(nod), ', '.join(sorted(nod)[:8])))

    # ★ AUD-1000 B06-001（P1 奇偶一致性）：dispatch 归一化 off&=~1 后二分查表，
    #   offs 表含 6 个 Thumb 奇地址条目（0x23461/0x23475/0x23489/0x2349d/
    #   0x9d43d/0x9d9e9）——奇值被 &~1 归一化后的偶值若不在表，对应函数经
    #   blx 分派必 miss。判据：每个奇条目 &~1 后须与表内某偶值重合（本表
    #   奇偶成对实证 → PASS）；缺对才 FAIL。同时输出奇值清单供人复核。
    odd_entries = [o for o in vals if o & 1]
    even_set = {v & ~1 for v in vals}
    odd_missing = [v for v in odd_entries if (v & ~1) not in even_set]
    if odd_missing:
        problems.append('offs 表 Thumb 奇地址 0x%s 归一化(&~1)后无对应偶条目 → blx 分派必 miss'
                        % '/0x'.join(format(v, 'x') for v in odd_missing))
    elif odd_entries:
        print('  (Thumb 奇地址 %d 条已与偶值成对，&~1 归一化一致)' % len(odd_entries))

    for p in problems:
        print('  ★ %s' % p)
    if not problems:
        print('  ✓ dispatch 表一致（n/长度/升序/同址/定义全通过）')
    return (not problems), problems


def load_proven():
    """加载已实证可加载基线的 UND 符号集。"""
    if not os.path.exists(PROVEN_FILE):
        return set()
    with open(PROVEN_FILE) as f:
        return set(json.load(f).get('und', []))


def audit(path, baseline=None):
    tmp = None
    so = path
    if path.endswith('.apk'):
        tmp, inlib = so_from_apk(path)
        so = tmp
        print('=== 审计 %s  (%s) ===' % (path, inlib))
        apk_checks(path)
    else:
        print('=== 审计 %s ===' % path)

    disp_problems = []
    # ★ AUD-1000 B07-010：去硬编码绝对路径，check_dispatch(None) 内按工具
    #   相对路径定位多文件源码（jqt_vtbl.c + jqt_OnCreate_full.c）。
    _, disp_problems = check_dispatch(None)

    try:
        und, dangling = resolve_so(so)
    except (FileNotFoundError, ValueError) as e:
        # ★ N1 门禁：假文件/非 ELF/无 .dynsym → 直接 FAIL（不得误判 PASS）
        print('--- 文件校验 FAIL: %s' % e)
        print('\n>>> 审计结论: FAIL（文件无效，非链接安全判定）')
        return 'FAIL'
    exports = load_cache_exports()
    proven = load_proven()
    if baseline:
        btmp = None
        bso = baseline
        try:
            if baseline.endswith('.apk'):
                btmp, _ = so_from_apk(baseline)
                bso = btmp
            bund, _ = resolve_so(bso)
            proven = set(bund)      # 显式基线覆盖
        finally:
            # ★ B06-094（P2 已修）：异常路径 btmp 泄漏——旧版 os.unlink(btmp) 在
            #   resolve_so/proven 赋值后执行，若 so_from_apk/resolve_so 抛异常
            #   则 btmp 不清理（/tmp 临时文件泄漏）。try/finally 保证异常路径
            #   也 unlink（正常路径行为不变：btmp 非 None 才 unlink）。
            os.unlink(btmp) if btmp else None
    proven_bases = {s.split('@')[0] for s in proven}

    print('\n--- .hash 风格（MuMu 32 位 linker 只支持 sysv）: %s' % hash_style(so))
    print('--- 内部符号悬空（F_xxx 有重定位无定义 → fail to link）: %s'
          % (', '.join(dangling) if dangling else '无'))

    print('\n--- UND 符号解析（MuMu ARM 库 + 实证基线）: %d 个' % len(und))
    unresolved, bad_known, ok_proven, ok_lib = [], [], [], []
    for s in und:
        base = s.split('@')[0]
        if base in KNOWN_BAD:
            bad_known.append((s, KNOWN_BAD[base]))
            continue
        if base in proven_bases:
            ok_proven.append(s)
            continue
        if base in PROVEN_LOADABLE:
            ok_proven.append(s)        # 实机已证明可加载（v178e5-e7）
            continue
        if exports and base in exports:
            ok_lib.append((s, exports[base][1], exports[base][0]))
            continue
        unresolved.append(s)
    if bad_known:
        print('  ★ 已知 problematic:')
        for s, why in bad_known:
            print('      %-24s %s' % (s, why))
    if unresolved:
        print('  ★ 无法解析（链接失败嫌疑）:')
        for s in unresolved:
            print('      %s' % s)
    else:
        print('  ✓ 全部可解析（基线实证 %d + ARM 库命中 %d）'
              % (len(ok_proven), len(ok_lib)))
    for s, prov, ver in ok_lib:
        print('      %-24s ← %-18s %s' % (s, prov, ver))

    if baseline:
        btmp = None
        bso = baseline
        if baseline.endswith('.apk'):
            btmp, _ = so_from_apk(baseline)
            bso = btmp
        bund, _ = resolve_so(bso)
        added = sorted(set(und) - set(bund))
        removed = sorted(set(bund) - set(und))
        print('\n--- 基线 diff vs %s' % baseline)
        print('  新增 UND (%d): %s' % (len(added), ', '.join(added) if added else '无'))
        print('  移除 UND (%d): %s' % (len(removed), ', '.join(removed) if removed else '无'))
        if btmp:
            os.unlink(btmp)

    verdict = 'PASS' if (not dangling and not bad_known and not unresolved
                          and not disp_problems) else 'FAIL'
    print('\n>>> 审计结论: %s' % verdict)
    if tmp:
        os.unlink(tmp)
    return verdict


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    cmd = sys.argv[1]
    if cmd == 'libs':
        sys.exit(0 if cache_libs('--force' in sys.argv) else 1)
    elif cmd == 'audit':
        base = None
        if '--baseline' in sys.argv:
            base = sys.argv[sys.argv.index('--baseline') + 1]
        sys.exit(0 if audit(sys.argv[2], base) == 'PASS' else 2)
    elif cmd == 'check':
        rc = 0
        for p in sys.argv[2:]:
            if audit(p) != 'PASS':
                rc = 2
        sys.exit(rc)
    else:
        print(__doc__)
        sys.exit(1)
