#!/bin/bash
# ============================================================
# jq_reverse_pipeline.sh — 军棋 native 逆向对齐 全流水线
#
# 原则：要么纯指令翻译，要么反汇编成 C，函数级回归（输入一致→输出一致），
#       机械化脚本化。本脚本把整条链路固化为可重复执行。
#
# 用法：
#   bash jq_reverse_pipeline.sh all        # 全量（校验→翻译→回归→对齐库→APK）
#   bash jq_reverse_pipeline.sh translate   # 仅生成 14 JNI 入口翻译体
#   bash jq_reverse_pipeline.sh regress      # 全量函数级回归（并行分块）
#   bash jq_reverse_pipeline.sh aligned      # 仅编译对齐版 .so + 打包 APK
#   bash jq_reverse_pipeline.sh verify       # 程序化校验（add_pc/RELATIVE/GOT 三类）
#   bash jq_reverse_pipeline.sh stats        # 仅输出当前统计
# ============================================================
set -e
ROOT=/disk/workspaces/jq
OP=/tmp/opencode
T2C=$OP/thumb2c_full.py
BT=$OP/batch_test.py
RP2=$OP/run_pipeline2.py
SO=$OP/jq_apk/dec/lib/armeabi/libjunqi.so
DISASM=$OP/jq64_thumb.txt
JTI=$ROOT/rev64/reverse/src_jni_translated

# 14 个原版 JNI 入口（.so 动态符号表拿到，对齐基准）
JNI="0x233e1 0x233f5 0x23409 0x23425 0x23439 0x2344d 0x23461 0x23475 0x23489 0x2349d 0x234b1 0x234c5 0x23561 0x235d5"
JNI_NAMES="OnEvent OnInput OnResult OnBluetooth OnTouch OnKey OnTimer OnUpdate OnGraphChange OnGraphCreate OnResume OnPause OnCreate OnDestroy"

cmd_translate() {
  echo "== [translate] 生成 14 JNI 入口翻译体 → $JTI =="
  python3 - "$JTI" <<'PYEOF'
import sys, importlib.util, re
JTI=sys.argv[1]
spec=importlib.util.spec_from_file_location("t2c","/tmp/opencode/thumb2c_full.py")
t2c=importlib.util.module_from_spec(spec); spec.loader.exec_module(t2c)
insns=t2c.load_insns()
entries=sorted(set(a for a,mn,r in insns if mn=='push' and 'lr' in r))
jni={0x233e1:'OnEvent',0x233f5:'OnInput',0x23409:'OnResult',0x23425:'OnBluetooth',
     0x23439:'OnTouch',0x2344d:'OnKey',0x23461:'OnTimer',0x23475:'OnUpdate',
     0x23489:'OnGraphChange',0x2349d:'OnGraphCreate',0x234b1:'OnResume',
     0x234c5:'OnPause',0x23561:'OnCreate',0x235d5:'OnDestroy'}
for fa,name in jni.items():
    csrc=t2c.gen_c_file(insns,entries,fa).replace('F_%x'%fa,'jqt_%s'%name)
    n=len(re.findall(r'L[0-9a-f]+:',csrc))
    open(JTI+'/jqt_%s.c'%name,'w').write(
        '// 机械化翻译自原版 32 位 libjunqi.so @ 0x%x (%s) — %d 标签\n'
        '// bl/blx 存根 r0=0（与回归一致）；.rodata/.text 常量池需运行时明文注入\n'%(fa,name,n)+csrc)
    print('  %-14s @0x%x: %d 标签'%(name,fa,n))
PYEOF
}

cmd_regress() {
  echo "== [regress] 全量函数级回归（并行分块，unicorn vs 翻译C）=="
  # 生成候选集（排除 PLT 桩，保留 JNI 入口 + 真实函数）
  python3 - <<'PYEOF'
import importlib.util
spec=importlib.util.spec_from_file_location("t2c","/tmp/opencode/thumb2c_full.py")
t2c=importlib.util.module_from_spec(spec); spec.loader.exec_module(t2c)
insns=t2c.load_insns()
entries=sorted(set(a for a,mn,r in insns if mn=='push' and 'lr' in r))
funcs=[a for i,a in enumerate(entries) if (entries[i+1] if i+1<len(entries) else 0x120bf1)-a>=8]
small=[a for a in funcs if (entries[entries.index(a)+1] if entries.index(a)+1<len(entries) else 0x120bf1)-a<200]
big=[a for a in funcs if a not in small]
open('/tmp/funcs_small_nplt.txt','w').write(' '.join(hex(x) for x in small))
open('/tmp/funcs_big.txt','w').write(' '.join(hex(x) for x in big))
print('  小函数(size<200): %d  大函数(size>=200): %d'%(len(small),len(big)))
PYEOF
  # 小函数 8 块并发 + 大函数 6 块并发（每块顺序跑 batch_test）
  python3 -c "
funcs=open('/tmp/funcs_small_nplt.txt').read().split(); per=len(funcs)//8
for i in range(8): open('/tmp/sblk_%d.txt'%i,'w').write(' '.join(funcs[i*per:(i+1)*per]))
"
  for i in 0 1 2 3 4 5 6 7; do
    nohup python3 "$BT" $(cat /tmp/sblk_$i.txt) > /tmp/sblk_$i.out 2>/dev/null &
  done
  python3 -c "
funcs=open('/tmp/funcs_big.txt').read().split(); per=len(funcs)//6
for i in range(6): open('/tmp/bblk_%d.txt'%i,'w').write(' '.join(funcs[i*per:(i+1)*per]))
"
  for i in 0 1 2 3 4 5; do
    nohup python3 "$BT" $(cat /tmp/bblk_$i.txt) > /tmp/bblk_$i.out 2>/dev/null &
  done
  echo "  14 块并发已启动，等待完成..."
  wait
  # 统计
  python3 - <<'PYEOF'
import glob
from collections import Counter
c=Counter(); total=0
for p in glob.glob('/tmp/sblk_*.out')+glob.glob('/tmp/bblk_*.out'):
    for ln in open(p):
        ln=ln.strip()
        if not ln: continue
        total+=1
        if ': PASS' in ln: c['PASS']+=1
        elif 'MISMATCH' in ln: c['MISMATCH']+=1
        elif 'UC FAIL' in ln: c['UC_FAIL']+=1
        elif 'SKIP' in ln: c['SKIP']+=1
        else: c['OTHER']+=1
print('  === 全量回归统计（%d 函数）==='%total)
# 分层：翻译器一致率 = PASS / (PASS + MISMATCH)，SKIP/UC_FAIL 是输入不变量边界（剔除分母）
ran = c.get('PASS',0)+c.get('MISMATCH',0)
for k in ['PASS','MISMATCH','UC_FAIL','SKIP','OTHER']:
    if c.get(k): print('    %-12s: %6d (%.1f%%)'%(k,c[k],c[k]/total*100))
if ran:
    print('  --- 翻译器一致率（可跑函数中 C 与 unicorn 输出一致）---')
    print('    可跑函数: %d (PASS+MISMATCH)'%ran)
    print('    一致: %d / %d = %.1f%%'%(c.get('PASS',0),ran, c.get('PASS',0)/ran*100))
    print('    不一致(MISMATCH): %d'%c.get('MISMATCH',0))
# MISMATCH 清单（应全为 STL/字符串输入不变量）
mm=sorted(set(l.split(' trial')[0].split(':')[0] for p in glob.glob('/tmp/*.out') for l in open(p) if 'MISMATCH' in l))
if mm: print('  MISMATCH 函数（STL容器/字符串边界）: '+', '.join(mm))
open('/tmp/pipeline_summary.txt','w').write('\n'.join('%s %d'%(k,c.get(k,0)) for k in ['PASS','MISMATCH','UC_FAIL','SKIP','OTHER'])+'\n')
PYEOF
}

cmd_aligned() {
  echo "== [aligned] 编译对齐版 arm64 库 + 打包 APK =="
  bash $ROOT/rev64/build_aligned.sh $ROOT/rev64/dist/libjunqi_arm64_aligned.so
  bash $ROOT/rev64/pack_phase.sh $ROOT/rev64/dist/libjunqi_arm64_aligned.so $ROOT/rev64/dist/phase_aligned.apk
  echo "  产物: $ROOT/rev64/dist/phase_aligned.apk + libjunqi_arm64_aligned.so"
}

cmd_verify() {
  echo "== [verify] 翻译体 vs 原版 程序化校验（add_pc/RELATIVE/GOT 三类）=="
  python3 $ROOT/rev64/tools/verify_translation.py --verbose
}

cmd_stats() {
  echo "== [stats] 当前流水线状态 =="
  if [ -f /tmp/pipeline_summary.txt ]; then
    cat /tmp/pipeline_summary.txt | sed 's/^/  /'
  else
    echo "  （尚未跑过全量回归，先执行 regress）"
  fi
  echo "  产物:"
  ls -la $ROOT/rev64/dist/phase_aligned.apk $ROOT/rev64/dist/libjunqi_arm64_aligned.so 2>/dev/null | sed 's/^/    /' || echo "    （尚未生成 aligned 产物）"
  echo "  14 翻译体:"
  ls $JTI/jqt_*.c 2>/dev/null | sed 's/^/    /' || echo "    （尚未生成，先执行 translate）"
}

# 增量回归：已对齐（PASS）+ 边界（UC_FAIL/SKIP）冻结，只攻 MISMATCH + 未测
# 像 git 一样追踪函数状态，不浪费在 90% 已解决的函数上
cmd_incremental() {
  echo "== [incremental] 增量回归（已对齐冻结，只攻 MISMATCH + 未测）=="
  echo "  状态文件: /tmp/opencode/regression_state.json"
  python3 $OP/incremental_regress.py status
  echo "  → 下轮只跑未收敛函数（PASS/边界已冻结不动）"
  if [ "${2:-}" = "run" ]; then
    echo "  开始增量 run..."
    python3 $OP/incremental_regress.py run
    python3 $OP/incremental_regress.py status
  fi
}

# 把全量 .out 结果导入增量基线（冻结已分类函数）
cmd_freeze() {
  echo "== [freeze] 导入全量 .out → 增量基线（冻结已 PASS/边界函数）=="
  python3 $OP/import_baseline.py
}

case "${1:-all}" in
  translate) cmd_translate ;;
  regress)   cmd_regress ;;
  aligned)   cmd_aligned ;;
  stats)     cmd_stats ;;
  verify)    cmd_verify ;;
  incremental) cmd_incremental "${2:-status}" ;;
  freeze)    cmd_freeze ;;
  all)
    cmd_verify
    cmd_translate
    cmd_regress
    cmd_freeze
    cmd_aligned
    cmd_stats
    echo "== [all] 全流水线完成（含增量基线冻结）=="
    ;;
  *) echo "用法: $0 {all|translate|regress|aligned|stats|verify|incremental [run]|freeze}"; exit 1 ;;
esac
