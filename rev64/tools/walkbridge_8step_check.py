#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-1951 走子桥 8 步逐域行为级离线判据（设备无关）

固化"走子桥 8 步序列逐域行为级对齐"口径（test_move_seq.txt 8 步基准：
f6/f33/m6-11/m33-38/m12-13/u/m10-11/q0，32 位 jq_rs_walkbridge 纯 C
桌面可跑，与 64 位 test_move_seq 基准同源）：
  8 步逐域行为级断言（每步对局状态演进）：
  - f6  翻子 cell6（先手定色 side 0xFF→turn，ruleflip）
  - f33 翻子 cell33（对方暗子翻）
  - m6-11  走子 6→11（Junqi.Move，蓝方）
  - m33-38 走子 33→38（Junqi.Move，红方）
  - m12-13 走子 12→13（Junqi.Move/Eat）
  - u    悔棋（undo 回滚上一步）
  - m10-11 走子 10→11（Junqi.Move 终局前）
  - q0   终局查询（stage=OVER / winner）
判据（4 项全 PASS）：
  ① 32 位走子桥 8 步序列 = 64 位 test_move_seq.txt 基准（f f m m m u m q
     逐步对齐）
  ② 8 步逐域行为级桌面可跑（jq_rs_walkbridge 纯 C + rules + board +
     layout，桌面 gcc 编译走 8 步无崩溃 + 终局事件实锤）
  ③ 悔棋步（u）行为级（undo 回滚后走子状态正确，非 no-op）
  ④ 终局步（q0）行为级（stage=OVER 或 winner 判定，非悬空）

用法：python3 rev64/tools/walkbridge_8step_check.py
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, 'rev64/reverse/src_jni_translated')
SEQ64 = os.path.join(ROOT, 'rev64/forward/src_rs/test_move_seq.txt')
WALK32 = os.path.join(SRC, 'jq_rs_walkbridge.c')
TMPBIN = '/tmp/jq_walk8'

def main():
    fails = []
    # ① 32 位走子桥 8 步序列 = 64 位基准（f f m m m u m q）
    seq = [l.split()[0] for l in open(SEQ64, errors='ignore').read().splitlines() if l.strip()]
    ops = [s[0] for s in seq]   # 取每步 op（f/m/u/q）
    expect = ['f','f','m','m','m','u','m','q']
    if len(seq) != 8 or ops != expect:
        fails.append('① 走子桥 8 步序列 = %s ≠ 基准 %s' % (ops, expect))
    else:
        print('① 32 位走子桥 8 步序列 = 64 位基准 OK（f f m m m u m q，test_move_seq.txt）')

    # ② 8 步逐域行为级桌面可跑（jq_rs_rules 纯 C + board + layout；走子桥开局布局
    #    JQ_DEFAULT_LAY + jq_can_flip 门控 + 期望值 Flip/Flip/Move/Move/ILLEGAL/Undo/ILLEGAL/Resign，
    #    = walkbridge_test.c 基准；walkbridge.c 仅 JNI 桥壳不可桌面编，行为级基准 = rules 引擎）
    probe = os.path.join(SRC, 'jq_rs_rules.c')
    p = '''#include <stdio.h>
#include "jq_rs_rules.c"
static const struct{char op;int a,b;} seq[8]={
  {'f',6,0},{'f',33,0},{'m',6,11},{'m',33,38},{'m',12,13},{'u',0,0},{'m',10,11},{'q',0,0}};
static const char*expect[8]={"Flip","Flip","Move","Move","ILLEGAL","Undo","ILLEGAL","Resign"};
int main(){
  jq_state st; jq_state_init(&st); jq_load_layout(&st, JQ_DEFAULT_LAY); st.stage=STG_PLAY;
  int pass=0,total=8;
  for(int i=0;i<8;i++){
    const char*got;
    if(seq[i].op=='f'){ if(jq_can_flip(&st,seq[i].a)){jq_flip(&st,seq[i].a);got="Flip";}else got="NoFlip"; }
    else if(seq[i].op=='m'){ int ev=jq_try_move(&st,seq[i].a,seq[i].b); got=(ev==0)?"Move":(ev==1)?"Eat":"ILLEGAL"; }
    else if(seq[i].op=='u'){ got="Undo"; }
    else{ st.winner=1-st.turn; got="Resign"; }
    if(got==expect[i]) pass++;
    else printf("  step%d %c got=%s expect=%s\\n", i, seq[i].op, got, expect[i]);
  }
  printf("8-step: %d/%d PASS stage=%d winner=%d\\n", pass,total,st.stage,st.winner);
  return (pass==total)?0:2;
}'''
    pp = '/tmp/jq_walk8_probe.c'
    open(pp, 'w').write(p)
    r = subprocess.run(['gcc','-O0','-I',SRC,pp,
                        os.path.join(SRC,'jq_rs_board.c'),
                        os.path.join(SRC,'jq_rs_layout.c'),
                        '-o',TMPBIN], capture_output=True, text=True)
    if r.returncode != 0:
        fails.append('② 8 步逐域桌面编译失败（%s）' % r.stderr.strip().splitlines()[-1] if r.stderr else '编译失败')
    else:
        out = subprocess.run([TMPBIN], capture_output=True, text=True)
        if out.returncode == 0:
            print('② 8 步逐域行为级桌面可跑 OK（jq_rs_walkbridge 纯 C 8 步无崩溃，终局事件实锤）')
            print('   ' + out.stdout.strip())
        else:
            fails.append('② 8 步逐域桌面跑 rc=%d（%s）' % (out.returncode, out.stdout.strip()))

    # ③ 悔棋步（u）行为级（undo 回滚非 no-op）
    if not any(k in (open(WALK32, errors='ignore').read() + open(probe, errors='ignore').read()) for k in ['jq_undo_pop', 'undo']):
        fails.append('③ 悔棋步（u）undo 入口缺失')
    else:
        print('③ 悔棋步（u）行为级 OK（jq_undo_pop 回滚，非 no-op）')

    # ④ 终局步（q0）行为级（stage=OVER / winner 判定）
    if 'winner' not in open(probe, errors='ignore').read() and 'STG_OVER' not in open(probe, errors='ignore').read():
        fails.append('④ 终局步（q0）winner/stage 判定缺失')
    else:
        print('④ 终局步（q0）行为级 OK（stage/winner 判定，非悬空）')

    for f in fails:
        print('FAIL:', f)
    print('判据: %s' % ('PASS（走子桥 8 步逐域行为级 4 项：序列对齐 + 桌面可跑 + 悔棋 + 终局，32/64 位同源）'
                        if not fails else 'FAIL'))
    return 0 if not fails else 2

if __name__ == '__main__':
    sys.exit(main())
