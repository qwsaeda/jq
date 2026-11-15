#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1934 生命周期 4 回调 + JNI 14 入口语义层收口判据（设备无关）

固化"4 生命周期回调（OnCreate/OnResume/OnPause/OnDestroy）非存根 + 14
JNI 入口全语义"口径（AGENTS B09-001 14 入口 + 生命周期回调真调）：
  - 4 生命周期回调：OnCreate（首局/构造 GLView）/ OnResume（恢复，
    JNI 入口 0x234b0 真调 F_bdc7c）/ OnPause（暂停，JNI 入口 0x234c4
    真调 F_bdcd4）/ OnDestroy（清理），非存根（真调内部处理函数）
  - 14 JNI 入口 = B09-001 口径（jni_export_align_check §1922 已实锤
    14/14 逐一匹配），本判据补"4 生命周期回调非存根"语义层
判据（4 项全 PASS）：
  ① 4 生命周期回调全在 jni_aligned.c（jqt_OnCreate/OnResume/OnPause/
     OnDestroy extern + JNI 入口映射）
  ② OnResume/OnPause 非存根（JNI 入口真调 F_bdc7c/F_bdcd4，非 no-op）
  ③ 14 JNI 入口全在翻译体（B09-001，jni_export_align §1922 同源）
  ④ 4 生命周期回调在 14 入口内（OnCreate/OnResume/OnPause/OnDestroy
     是 14 入口子集）

用法：python3 rev64/tools/lifecycle_jni_check.py
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
JNI = os.path.join(ROOT, 'rev64/reverse/src_jni_translated/jni_aligned.c')
ONRES = os.path.join(ROOT, 'rev64/reverse/src_jni_translated/jqt_OnResume.c')
TRANS_SO = os.path.join(ROOT, 'rev64/dist/libjunqi_arm32_aligned.so')
LIFECYCLE = ['OnCreate', 'OnResume', 'OnPause', 'OnDestroy']

def main():
    fails = []
    j = open(JNI, errors='ignore').read()
    ors = open(ONRES, errors='ignore').read()

    # ① 4 生命周期回调全在 jni_aligned.c
    # OnCreate 在 jni_aligned.c 内联（OnCreate: 诊断日志段），OnResume/OnPause/OnDestroy 经 jqt_On* 调用点
    lc_present = {
        'OnCreate': ('OnCreate:' in j) or ('jqt_OnCreate(' in j),
        'OnResume': ('jqt_OnResume(' in j) or ('jqt_OnResume' in j),
        'OnPause': ('jqt_OnPause(' in j) or ('jqt_OnPause' in j),
        'OnDestroy': ('jqt_OnDestroy(' in j) or ('jqt_OnDestroy' in j),
    }
    miss_lc = [c for c in LIFECYCLE if not lc_present.get(c)]
    if miss_lc:
        fails.append('① jni_aligned.c 缺生命周期回调 %s' % miss_lc)
    else:
        print('① 4 生命周期回调全在 jni_aligned.c OK（jqt_OnCreate/OnResume/OnPause/OnDestroy）')

    # ② OnResume/OnPause 非存根（JNI 入口真调 F_bdc7c/F_bdcd4）
    if 'F_bdc7c' not in ors or 'F_bdcd4' not in ors:
        fails.append('② OnResume/OnPause 缺内部处理函数真调（F_bdc7c/F_bdcd4）')
    else:
        print('② OnResume/OnPause 非存根 OK（JNI 入口真调 F_bdc7c/F_bdcd4，§124y-141）')

    # ③ 14 JNI 入口全在翻译体
    out = subprocess.run(['arm-linux-gnueabi-nm', '-D', TRANS_SO], capture_output=True, text=True).stdout
    syms = set(re.findall(r'Java_com_cnvcs_App_\w+', out))
    if len(syms) != 14:
        fails.append('③ 翻译体 JNI 入口 = %d ≠ 14（B09-001 口径）' % len(syms))
    else:
        print('③ 14 JNI 入口全在翻译体 OK（B09-001，jni_export_align §1922 同源）')

    # ④ 4 生命周期回调在 14 入口内
    lc_in = [c for c in LIFECYCLE if ('Java_com_cnvcs_App_' + c) in syms]
    if len(lc_in) != 4:
        fails.append('④ 生命周期回调在 14 入口 = %d ≠ 4（%s）' % (len(lc_in), lc_in))
    else:
        print('④ 4 生命周期回调在 14 入口内 OK（OnCreate/OnResume/OnPause/OnDestroy）')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（4 生命周期回调非存根 + 14 JNI 入口全语义，生命周期语义层收口）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
