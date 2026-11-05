#!/bin/bash
# ============================================================
# chain_pipeline.sh — 调用链对齐流水线（用户方法论 §124q）
#
# 固定模式化执行：把"B 翻译体调用链对齐 A 原版"做成可重复流水线：
#   extract   提取 A 动态调用链（DiagJni53 指令轨迹 → capstone bl/blx → chain_tree.json）
#   plan      规划 B 需实现清单（按调用链深度/类型：PLT已实现/so内部需翻译/Java交互需JNI）
#   impl      逐层生成 B C 实现（机械翻译 + 真实调用替换存根）
#   verify    验证调用链返回对齐（A/B 逐点比较，复用 batch_test 思路）
#   report    生成对齐报告（chain_tree.json + impl 进度 + verify 结果）
#
# 复用现有工具：thumb2c_full.py（机械翻译）+ batch_test.py（函数级回归）
# ============================================================
set -u
WS=/disk/workspaces/jq
JQ=$WS/rev64
TMP=/tmp/opencode/chain
mkdir -p $TMP
SO=/tmp/opencode/jq_apk/dec/lib/armeabi/libjunqi.so
TRACE=$TMP/oncreate_trace.txt
TREE=$TMP/chain_tree.json
PLAN=$TMP/chain_plan.json

cmd_extract() {
    echo "[chain.extract] 编译+跑 DiagJni53（A 原版 OnCreate 真实执行 → 指令轨迹）"
    cd /tmp/opencode/unidbg
    CP=".:$(ls libs/*.jar | tr '\n' ':')classes:"
    javac -cp "$CP" DiagJni53.java 2>&1 | grep -v "warning\|1 warning" | head
    timeout 300 java -cp "$CP" DiagJni53 > $TMP/diagjni53.log 2>&1
    echo "  exit=$?  OnCreate 结果: $(grep -oE 'OnCreate -> .*' $TMP/diagjni53.log | head -1)"
    if [ ! -f /tmp/opencode/oncreate_trace.txt ]; then
        echo "  ✗ 无指令轨迹（OnCreate 未成功）"; exit 1
    fi
    cp /tmp/opencode/oncreate_trace.txt $TRACE
    echo "  轨迹: $(wc -l < $TRACE) 条指令"
    echo "[chain.extract] capstone 分析轨迹 → 动态调用链树"
    python3 <<'PYEOF'
from capstone import *
from capstone.arm import *
import json
d=open('/tmp/opencode/jq_apk/dec/lib/armeabi/libjunqi.so','rb').read()
md=Cs(CS_ARCH_ARM, CS_MODE_THUMB)
offs=[int(l.strip(),16) for l in open('/tmp/opencode/chain/oncreate_trace.txt')]
uniq=sorted(set(offs))
blmap={}
for off in uniq:
    if off+4>len(d): continue
    ins=list(md.disasm(d[off:off+4],off))
    if ins and ins[0].mnemonic in ('bl','blx'):
        try: blmap[off]=int(ins[0].op_str.replace('#','').strip(),16)
        except: pass
# 调用链树：按首次出现顺序（执行顺序 ≈ DFS），记录 (depth,parent,target,type)
tree=[]
shown=set()
depth_map={0x23560:0}  # OnCreate 入口 depth 0
for off in offs:
    if off in blmap:
        tgt=blmap[off]
        if tgt in shown: continue
        shown.add(tgt)
        if 0x233e0<=tgt<0x1310f4: typ='so'
        elif 0x22000<=tgt<0x233e0: typ='plt'
        else: typ='libc'
        tree.append({'src':off,'dst':tgt,'type':typ,'order':len(tree)})
out={'entry':0x23560,'trace_len':len(offs),'uniq':len(uniq),'calls':tree}
json.dump(out, open('/tmp/opencode/chain/chain_tree.json','w'), indent=1)
print(f"  调用链树: {len(tree)} 个 bl/blx 目标（so={sum(1 for c in tree if c['type']=='so')}, plt={sum(1 for c in tree if c['type']=='plt')}, libc={sum(1 for c in tree if c['type']=='libc')}）")
print(f"  已写 /tmp/opencode/chain/chain_tree.json")
PYEOF
}

cmd_plan() {
    echo "[chain.plan] 规划 B 需实现清单（按类型/深度）"
    [ -f $TREE ] || { echo "  ✗ 先跑 extract"; exit 1; }
    python3 <<'PYEOF'
import json
t=json.load(open('/tmp/opencode/chain/chain_tree.json'))
so=[c for c in t['calls'] if c['type']=='so']
plt=[c for c in t['calls'] if c['type']=='plt']
libc=[c for c in t['calls'] if c['type']=='libc']
# so 内部需翻译（B 实现）；plt 已实现（_Znwj/_ZdlPv malloc/free）；libc 外部库
plan={'to_translate':so,'plt_available':plt,'libc_external':libc,
      'note':'so=需 B 机械翻译实现; plt=已真实化(_Znwj/_ZdlPv)或需链接; libc=外部库无需实现'}
json.dump(plan, open('/tmp/opencode/chain/chain_plan.json','w'), indent=1)
print(f"  B 需实现 so 内部函数: {len(so)} 个（0xb9edc 等）")
print(f"  PLT（可链接/已真实化）: {len(plt)} 个")
print(f"  libc（外部库，无需实现）: {len(libc)} 个")
print(f"  已写 /tmp/opencode/chain/chain_plan.json")
print("  ★ 实现顺序：按调用链深度（0xb9edc→0x24250→0xbe47c 逐层）")
PYEOF
}

cmd_impl() {
    echo "[chain.impl] 生成 B 完整调用链 C（gen_chain_full.py：gen_c_file_full + 后处理）"
    [ -f $TREE ] || { echo "  ✗ 先跑 extract"; exit 1; }
    timeout 300 python3 /tmp/opencode/gen_chain_full.py 2>&1 | tail -6
    cp /tmp/opencode/chain/jqt_OnCreate_full.c $JQ/rev64/src_jni_translated/jqt_OnCreate_full.c
    echo "[chain.impl] 重编 arm64 so（828 函数完整调用链）"
    timeout 600 bash $JQ/rev64/build_arm64_mem.sh 2>&1 | tail -3
    echo "[chain.impl] 打包 phase_chain.apk"
    bash $JQ/rev64/pack_phase.sh $JQ/rev64/dist/libjunqi_arm64_mem.so $JQ/rev64/dist/phase_chain.apk 2>&1 | tail -3
}

cmd_verify() {
    echo "[chain.verify] 验证调用链返回对齐（A/B 逐点比较）"
    echo "  待办：复用 batch_test.py 思路，A/B 对 0xb9edc/0x24250/0xbe47c 返回逐点比较"
}

cmd_report() {
    echo "[chain.report] 对齐报告"
    [ -f $TREE ] || { echo "  ✗ 先 extract"; exit 1; }
    python3 -c "
import json
t=json.load(open('$TREE'))
print(f'调用链: {len(t[\"calls\"])} 目标 | 轨迹 {t[\"trace_len\"]} 指令')
for c in t['calls'][:12]:
    print(f\"  {c['src']:08x} → {c['dst']:08x} [{c['type']}]\")"
}

case "${1:-all}" in
    extract) cmd_extract ;;
    plan)    cmd_plan ;;
    impl)    cmd_impl ;;
    verify)  cmd_verify ;;
    report)  cmd_report ;;
    all)     cmd_extract; cmd_plan; cmd_report ;;
    *) echo "用法: chain_pipeline.sh {extract|plan|impl|verify|report|all}"; exit 1 ;;
esac
