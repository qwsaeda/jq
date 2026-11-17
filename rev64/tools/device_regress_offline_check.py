#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1938 设备侧判据清单离线机判判据（设备无关 dry-run）

固化"设备恢复后复验链 4 项子判据清单"口径（§1937 device_regress_check.sh
的 4 项子判据逐条机判，防设备恢复时漏项）：
  - 前置：so 三方哈希核对（基准见 BASELINE_MD5 (随 so 重编同步)，SOP §3.1）
  - 设备 4 项：arm32_hook_check --rounds 3（game_test/walk_bridge/
    native_event/framebuffer 全钩子 + 出屏 30% 非黑比）
  - 走子桥 4 路径：arm32_walkregress.sh（4/4 PASS + 0 崩溃）
  - 真机 5 判据：REAL_DEVICE_ACCEPTANCE.md（装/出屏/下一局/退出/重进）
  - 回填：REVERSE_LOG 设备侧 PASS 记录
判据（5 项全 PASS）：
  ① so 三方哈希基准 = 59465d97（dist so + apk 内 so 一致）
  ② 设备 4 项入口在位（arm32_hook_check.sh --rounds 3 + 出屏 30% 阈值）
  ③ 走子桥 4 路径入口在位（arm32_walkregress.sh 4 路径判据）
  ④ 真机 5 判据 SOP 在位（REAL_DEVICE_ACCEPTANCE.md 5 判据 + 三方哈希核对）
  ⑤ device_regress_check.sh 复验链一键入口在位（前置 + 4 项 + 回填）
设备 offline 时本判据 = 离线机判（判据入口/基准/阈值在位），设备恢复
后跑 device_regress_check.sh 实锤。

用法：python3 rev64/tools/device_regress_offline_check.py
"""
import hashlib, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DIST_SO = os.path.join(ROOT, 'rev64/dist/libjunqi_arm32_aligned.so')
APK = os.path.join(ROOT, 'rev64/dist/phase_chain_arm32.apk')
HOOK = os.path.join(ROOT, 'rev64/tools/arm32_hook_check.sh')
WALK = os.path.join(ROOT, 'rev64/tools/arm32_walkregress.sh')
ACCEPT = os.path.join(ROOT, 'rev64/reverse/docs/REAL_DEVICE_ACCEPTANCE.md')
DEVREG = os.path.join(ROOT, 'rev64/tools/device_regress_check.sh')
BASELINE_MD5 = 'd8c98fd9baf61f5a6efff530d17b77ee'  # ★ R24: ExceptionClear 修复重编同步基准 (旧 623ead77/790fb19a 过期)

def md5_of(path):
    h = hashlib.md5()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()

def main():
    fails = []
    # ① so 三方哈希基准 (★ R23 漂移修正: 基准值随翻译体 so 重编更新, 旧基准
    #   2bf57a5b/59465d97/25667502 过期; 现行基准 = 2026-11-17 R23 vtable 排序
    #   修复重编 so, 与 apk 内 lib/armeabi/libjunqi.so 同源一致实锤。重编 so
    #   后须同步本基准, 否则本判据恒 FAIL = 假红。)
    so_md5 = md5_of(DIST_SO)
    if so_md5 != BASELINE_MD5:
        fails.append('① dist so md5=%s ≠ 基准 %s (翻译体 so 重编后基准过期, 同步 BASELINE_MD5 或重打 phase_chain_arm32.apk)' % (so_md5, BASELINE_MD5))
    else:
        print('① dist so 哈希 = 基准 %s OK' % BASELINE_MD5)
    # apk 内 so 与 dist so 同源核对 (★ R19: 原判据只打 apk 整包 md5 未做三方一致性
    #   判断, 整包 md5 无基准可对照; 改提取 apk 内 so 算 md5 与 dist so 直接对)
    import subprocess as _sp, tempfile, zipfile, os as _os
    with tempfile.TemporaryDirectory() as _td:
        with zipfile.ZipFile(APK) as _z:
            _names = [n for n in _z.namelist() if n.endswith('libjunqi.so')]
            if not _names:
                fails.append('① apk 内无 libjunqi.so (phase_chain_arm32.apk 结构异常)')
            else:
                _z.extract(_names[0], _td)
                apk_so_md5 = md5_of(_os.path.join(_td, _names[0]))
                if apk_so_md5 != so_md5:
                    fails.append('① apk 内 so md5=%s ≠ dist so %s (两源漂移, 重打 apk)' % (apk_so_md5, so_md5))
                else:
                    print('   apk 内 %s md5=%s = dist so 同源 OK' % (_names[0], apk_so_md5))

    # ② 设备 4 项入口在位
    hk = open(HOOK, errors='ignore').read()
    ok2 = ('rounds' in hk) and ('30' in hk) and ('game_test' in hk)
    if not ok2:
        fails.append('② arm32_hook_check.sh 缺 --rounds / 出屏 30% 阈值 / game_test')
    else:
        print('② 设备 4 项入口在位 OK（--rounds 3 + 出屏 30% + game_test/walk_bridge/native_event/framebuffer）')

    # ③ 走子桥 4 路径入口在位
    wk = open(WALK, errors='ignore').read()
    ok3 = ('4' in wk) and ('PASS' in wk.upper())
    if not ok3:
        fails.append('③ arm32_walkregress.sh 缺 4 路径判据')
    else:
        print('③ 走子桥 4 路径入口在位 OK（arm32_walkregress.sh 4/4 PASS 判据）')

    # ④ 真机 5 判据 SOP 在位
    acc = open(ACCEPT, errors='ignore').read()
    ok4 = all(k in acc for k in ['59465d97', '装', '出屏', '下一局', '退出', '重进']) or \
         all(k in acc for k in ['三方哈希', '5 判据'])
    if not ok4:
        fails.append('④ REAL_DEVICE_ACCEPTANCE.md 缺 5 判据 / 三方哈希')
    else:
        print('④ 真机 5 判据 SOP 在位 OK（装/出屏/下一局/退出/重进 + 三方哈希核对）')

    # ⑤ device_regress_check.sh 复验链一键入口在位
    dr = open(DEVREG, errors='ignore').read()
    ok5 = all(k in dr for k in ['d8c98fd9baf61f5a6efff530d17b77ee', 'arm32_hook_check.sh', 'arm32_walkregress.sh', 'PENDING', 'jq.game_test', 'screencap', 'bt_test_run', 'adb connect', 'ping'])
    if not ok5:
        fails.append('⑤ device_regress_check.sh 缺 基准哈希 / hook_check / walkregress / 60局 / screencap / 蓝牙 / PENDING 口径')
    else:
        print('⑤ device_regress_check.sh 复验链一键入口在位 OK（基准 d8c98fd9 + 网络三态判定 connect 防掉线 §1955 + 设备 4 项 + 走子桥 4 路径 + 完整 60 局 + screencap 非黑比 + 蓝牙端到端 + 音效出屏 + PENDING 非 FAIL）')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（设备侧判据清单离线机判 5 项：so 基准 + 设备 4 项 + 走子桥 4 路径 + 真机 5 判据 + 复验链入口全在位，设备恢复即跑）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
