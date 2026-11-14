#!/usr/bin/env python3
"""§124y-1911 SPAK 资源可再生性判据（设备无关，离线可跑）

用法：python3 rev64/tools/spak_regen_check.py
判据：
  ① 解包成功 883 资源（spak_unpack.py 四组 data/datah/datam/datax）
  ② 在库 spak_res 883 核心资源全量 md5 一致（可再生性 = 产物非一次性）
  ③ key 三方一致性：原版 so VA 0x12a658 现读 32B == 工具 SPAK_KEY == 解包成功本身
  ④ manifest.tsv 落盘（883 行，组/idx/hash/offset/size 全链可追溯）
退出码 0 = 全 PASS，1 = FAIL。
"""
import hashlib, os, re, subprocess, sys, tempfile

WS = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
TOOLS = os.path.join(WS, 'rev64', 'tools')
ORIG_SO = '/tmp/orig/lib/armeabi/libjunqi.so'
ASSETS = '/tmp/opencode/jq_apk/dec/assets'
STORED = os.path.join(WS, 'rev64', 'dist', 'spak_res')
N_RES = 883

def md5(p):
    return hashlib.md5(open(p, 'rb').read()).hexdigest()

def main():
    fails = []

    # ① 解包重跑（临时目录）
    tmp = tempfile.mkdtemp(prefix='spak_regen_')
    r = subprocess.run(['python3', os.path.join(TOOLS, 'spak_unpack.py'), ASSETS, tmp],
                       capture_output=True, text=True)
    files = [f for f in os.listdir(tmp) if not f.endswith('.tsv')]
    if r.returncode != 0 or len(files) != N_RES:
        fails.append('① 解包 %d 资源（期望 %d）rc=%d %s' % (len(files), N_RES, r.returncode, r.stderr[-200:]))
    else:
        print('① 解包 %d 资源 OK' % len(files))

    # manifest 存在
    mf = os.path.join(tmp, 'manifest.tsv')
    if not os.path.exists(mf):
        fails.append('④ manifest.tsv 缺失')
    else:
        rows = sum(1 for _ in open(mf)) - 1
        if rows != N_RES:
            fails.append('④ manifest.tsv %d 行（期望 %d）' % (rows, N_RES))
        else:
            print('④ manifest.tsv %d 行 OK' % rows)

    # ② 在库全量 md5 对照
    mism = 0
    for f in files:
        sp = os.path.join(STORED, f)
        if not os.path.exists(sp) or md5(sp) != md5(os.path.join(tmp, f)):
            mism += 1
    if mism:
        fails.append('② 在库 %d 个 md5 不一致' % mism)
    else:
        print('② 在库 spak_res 全量 md5 一致 %d/%d OK' % (N_RES, N_RES))

    # ③ key 三方一致性
    so = open(ORIG_SO, 'rb').read()
    key_in_so = so[0x12a658:0x12a658 + 32].hex()
    su = open(os.path.join(TOOLS, 'spak_unpack.py')).read()
    m = re.search(r"SPAK_KEY = bytes\.fromhex\('([0-9a-f]{64})'\)", su)
    key_in_tool = m.group(1) if m else ''
    if key_in_so != key_in_tool:
        fails.append('③ key 不一致 so=%s tool=%s' % (key_in_so, key_in_tool))
    else:
        print('③ key 三方一致（so VA 0x12a658 == 工具 SPAK_KEY，且解包成功）OK')

    for f in fails:
        print('FAIL:', f)
    print('判定: %s' % ('PASS（SPAK 资源全链可再生，离线判据收口）' if not fails else 'FAIL'))
    sys.exit(0 if not fails else 1)

if __name__ == '__main__':
    main()
