#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1942 音效播放链实锤判据（设备无关）

固化"音效播放链（sfx_play 入口 → WAV 资源可解码）"口径（SPEC §5
事件→音效映射 + 播放入口全事件覆盖 + WAV 资源完整性，与 §1935 音效
映射资源一致性互补）：
  - §1935 = 音效映射资源一致性（64 位 14 WAV 名 = SPAK 14 资源对照）
  - 本判据 = 音效播放链实锤（sfx_play 全事件入口覆盖 + WAV 资源
    可解码性：非 0 字节 + 合法 WAV/RIFF 头）
  - sfx_play 事件入口（jni_bridge.c）：win/lose/draw/pop/close/start/
    move/select/flag/over（distinct 10 事件入口）
判据（4 项全 PASS）：
  ① sfx_play 事件入口全在位（jni_bridge.c，win/lose/draw 胜负入口 +
     pop/close UI 入口 + start/move/select 走子入口）
  ② SPAK 14 WAV 资源非 0 字节（spak_res/*.wav 全 >0）
  ③ SPAK 14 WAV 资源合法 RIFF/WAV 头（magic 0x52494646 "RIFF"）
  ④ sfx_play 事件名 ⊆ SPAK 资源名（data_ 前缀对照，无缺漏）

用法：python3 rev64/tools/sfx_play_chain_check.py
"""
import glob, os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BRIDGE = os.path.join(ROOT, 'rev64/forward/src_rs/jni_bridge.c')
SPAK = os.path.join(ROOT, 'rev64/dist/spak_res')

def main():
    fails = []
    ref = open(BRIDGE, errors='ignore').read()

    # ① sfx_play 事件入口全在位
    evs = sorted(set(re.findall(r'sfx_play\(env,\s*\"([a-z]+)\"', ref)))
    need = ['win','lose','draw','start','move','select','close','pop']
    miss_ev = [e for e in need if e not in evs]
    if miss_ev:
        fails.append('① sfx_play 事件入口缺 %s（实有 %s）' % (miss_ev, evs))
    else:
        print('① sfx_play 事件入口全在位 OK（%d 事件入口：%s）' % (len(evs), ', '.join(evs)))

    # ② SPAK 14 WAV 资源非 0 字节
    wavs = glob.glob(os.path.join(SPAK, '*.wav'))
    if len(wavs) != 14:
        fails.append('② SPAK WAV = %d ≠ 14' % len(wavs))
    else:
        empty = [os.path.basename(p) for p in wavs if os.path.getsize(p) == 0]
        if empty:
            fails.append('② SPAK WAV 0 字节 %s' % empty)
        else:
            print('② SPAK 14 WAV 资源全非 0 字节 OK（%d 资源全 >0）' % len(wavs))

    # ③ SPAK 14 WAV 资源合法 RIFF/WAV 头
    badhdr = []
    for p in wavs:
        with open(p, 'rb') as f:
            hdr = f.read(8)
        if len(hdr) < 8 or hdr[:4] != b'RIFF' or hdr[8-4:8-4] == b'' :
            pass
        ok = hdr[:4] == b'RIFF' and len(hdr) >= 8
        # WAV 标准：RIFF + size + "WAVE"
        wave_ok = hdr[:4] == b'RIFF' and hdr[8:12] == b'WAVE' if len(hdr) >= 12 else False
        full = open(p,'rb').read(12)
        if not (full[:4] == b'RIFF' and full[8:12] == b'WAVE'):
            badhdr.append('%s（magic=%s）' % (os.path.basename(p), full[:4].hex()))
    if badhdr:
        fails.append('③ SPAK WAV 非法 RIFF/WAV 头 %s' % badhdr)
    else:
        print('③ SPAK 14 WAV 资源合法 RIFF/WAVE 头 OK（全 14 资源 magic=RIFF...WAVE）')

    # ④ sfx_play 事件名 ⊆ SPAK 资源名（data_ 前缀对照）
    spak_strip = set(os.path.basename(p)[5:] if os.path.basename(p).startswith('data_') else os.path.basename(p) for p in wavs)
    miss_res = [e for e in evs if ('junqi_' + e + '.wav' not in spak_strip) and (e + '.wav' not in spak_strip) and ('ui_' + e + '.wav' not in spak_strip)]
    # sfx_play 事件名 → SPAK 资源名映射：junqi_X.wav / ui_X.wav / X.wav
    # 实际 14 SPAK = 12 data_junqi_* + 2 data_ui_*；sfx_play 10 事件全应能映射
    mappable = []
    for e in evs:
        cand = ['junqi_%s.wav' % e, 'ui_%s.wav' % e, '%s.wav' % e]
        if not any(c in spak_strip for c in cand):
            miss_res.append(e)
    if miss_res:
        fails.append('④ sfx_play 事件无对应 SPAK 资源 %s' % miss_res)
    else:
        print('④ sfx_play %d 事件名全映射 SPAK 资源 OK（data_ 前缀对照无缺漏）' % len(evs))

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（音效播放链实锤 4 项：sfx_play 全事件入口 + SPAK 14 WAV 非 0 + 合法 RIFF/WAVE 头 + 事件名映射无缺漏）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
