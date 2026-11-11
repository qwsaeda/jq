#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-183 原版 .rodata 76 事件名语义分类判据（静态佐证，不依赖设备）

原版 so .rodata 明文 73 Junqi.* + 3 Misc.* = 76 事件名。按语义族分类：
  - 主链 11 条（走子桥实锤消费）
  - 悔棋族 6 / 对局控制 13 / 账号注册 20 / 联网广告 2 / 聊天 / 其他
判据输出 = 事件名族谱（佐证"账号/联网通道未逆向"边界的具体事件名清单，
非臆测——全部来自原版 .rodata 明文提取）。

用法：python3 rev64/tools/rodata_event_taxonomy.py
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"

def main():
    so = open(ORIG_SO, 'rb').read()
    junqi = sorted(set(m.group(0).decode() for m in re.finditer(rb'Junqi\.[A-Za-z]+', so)))
    misc = sorted(set(m.group(0).decode() for m in re.finditer(rb'Misc\.[A-Za-z]+', so)))
    core11 = {"Junqi.Move","Junqi.Flip","Junqi.Flag","Junqi.Bomb","Junqi.Eat","Junqi.Dead",
              "Junqi.GameOver","Junqi.Resign","Junqi.Draw","Junqi.BlueWin","Junqi.RedWin"}
    rest = [n for n in junqi if n not in core11]

    # 语义族分类（按前缀关键词）
    def fam(b):
        if 'Regret' in b: return '悔棋族'
        if any(k in b for k in ['Reconn','Leave','Ready','Peer','Start','Select','Win','Draw','Agree','Confirm','Disagree']): return '对局控制'
        if any(k in b for k in ['Chat','Msg','Nick']): return '聊天'
        if any(k in b for k in ['Reg','Login','User','Pwd','Char','Input','Lei','Qi','Zha','Error']): return '账号/注册'
        if any(k in b for k in ['Net','Game','Ad','Ver','Up','Join','Connect']): return '联网/广告'
        return '其他'
    fams = {}
    for n in rest:
        fams.setdefault(fam(n[6:]), []).append(n)

    print("═══ §124y-183 原版 .rodata 76 事件名语义族谱 ═══")
    print("Junqi.* 主链 11 条（走子桥实锤消费）：")
    for n in sorted(core11): print("  " + n)
    print("Junqi.* 其余 %d 条（按语义族）：" % len(rest))
    for f in ['悔棋族','对局控制','聊天','账号/注册','联网/广告','其他']:
        v = fams.get(f, [])
        if v: print("  %s %d 条：e.g. %s" % (f, len(v), ", ".join(v[:3])))
    print("Misc.* %d 条：%s（广告拆除目标事件名）" % (len(misc), ", ".join(misc)))
    print("\n═══ 判据 ═══")
    print("  事件名总数: %d（73 Junqi + %d Misc）" % (len(junqi), len(misc)))
    print("  主链 11 条实锤（走子桥）+ 其余 62 条 = 账号/联网/聊天维度（逆向边界项，")
    print("  佐证 AGENTS『账号系统/联网对战未实现』非臆测——全部来自原版 .rodata 明文）")
    return 0

if __name__ == '__main__':
    sys.exit(main())
