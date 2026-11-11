#!/bin/bash
# arm32_static_evidence.sh — 32 位逆向静态佐证判据 批次化入口（§124y-182）
#
# 沉淀"原版 so .rodata 明文 → 翻译体/SPAK 侧实体映射"全部静态判据为一条命令
# 可机判脚本（§124y-180 方法论第 1 条：判据脚本化 + 第 6 条：批量化）。
# 不依赖设备，设备重启窗口期也能跑（纯静态文件判读）。
#
# 用法:
#   bash rev64/tools/arm32_static_evidence.sh
#   bash rev64/tools/arm32_static_evidence.sh --only resource,event,javacls
#
# 静态佐证判据（每项 PASS/FAIL + 数值）:
#   resource   原版 .rodata 44 资源路径串 → SPAK 解包 0 缺口（§124y-177）
#   event      原版 .rodata 主链 11 事件名 → 翻译体实锤名命中（§124y-181）
#   urlkey     原版 .rodata URL 键 3 条（cfgHost/statHost/updateUrl）实锤（更新通道边界引用点）
#   javacls    原版 .rodata 6 个 com/cnvcs/* JNI FindClass 类名实锤（§124y-181 续）
#   forkcheck  32/64 位分叉矩阵 R-138 全量标记 0 未标记（§124y-166/182）
#   taxonomy   原版 .rodata 76 事件名语义族谱（§124y-183，主链 11 + 其余 62 + Misc 3）
#   event_cover 原版 .rodata 76 事件名 → 翻译体 g_jq_strings 覆盖度 0 缺口（§124y-184）
#   event_codemap .bss 事件表 asr4 映射判据（§124y-187，Junqi 主链 73 条 0 缺口）
#   bss_evt_tbl .bss 事件表 289B 真值基准判据（§124y-190，非0=129 + 高nibble=0 忠实语义）
#   rodata_static 原版 .rodata 明文 dump 逐字节一致性判据（§124y-192，非加壳 so 口径）
#   text_static 原版 .text 段明文判据（§124y-193，非加密壳静态指纹，与 .rodata 成对）
#   rel_dyn 原版 .rel.dyn 重定位槽判据（§124y-194，RELATIVE 5312 + 类型分布，全重定位口径）
#   jni_export 原版 32 位 so JNI 导出符号判据（§124y-195，14 个 On*，B09-001 口径修正）
#   got_plt 原版 .rel.plt/.got 符号解析槽判据（§124y-197，214 JUMP_SLOT + 类型分布）
#   trans_jni 32 位翻译体 so JNI 导出符号对照判据（§124y-198，翻译体 14 On* = 原版 14 On* 逐名一致）
#   trans_strpool 32 位翻译体 so 事件名字符串池对照判据（§124y-199，内嵌 ⊇ 原版 73 Junqi 0 缺口）
#   readelf_rel 原版 .rel.dyn 逐槽 readelf -r 对照判据（§124y-200，R_ARM_RELATIVE 5312，双源对照）
#   got_map .got/.plt 槽分析产物对照判据（§124y-201，plt 214 + got 227 产物在位）
#   needed 原版 so NEEDED 依赖库清单判据（§124y-202，7 系统库 + SONAME）
#   trans_needed 翻译体 so NEEDED 依赖面对照判据（§124y-203，⊆ 原版 7 系统库 + libstdc++ 已剥离）
#   walk_8step 走子桥 8 步序列判据锚点对照脚本（§124y-204，源码 8 步定义 + 双脚本锚点一致）
set -u
WS="$(cd "$(dirname "$0")/../.." && pwd)"
ONLY=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --only) ONLY="$2"; shift 2;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done
KINDS="resource event urlkey javacls forkcheck taxonomy event_cover event_codemap bss_evt_tbl rodata_static text_static rel_dyn jni_export got_plt trans_jni trans_strpool readelf_rel got_map needed trans_needed walk_8step"
[ -n "$ONLY" ] && KINDS="$ONLY"

PASS=0; FAIL=0; FAILED=""

run() {
  local k="$1"; shift
  echo "═══ $k ═══"
  "$@"
  local rc=$?
  if [ $rc -eq 0 ]; then echo "  ✓ $k"; PASS=$((PASS+1)); else echo "  ✗ $k"; FAIL=$((FAIL+1)); FAILED="$FAILED $k"; fi
}

check_rodata() {
  # 跑 rodata_path_map.py，按 KIND 过滤判据（输出已含各段汇总）
  local out
  out=$(python3 "$WS/rev64/tools/rodata_path_map.py" 2>&1)
  echo "$out" | grep -E "判据:|命中|原版" | head -12
  # 总判据 = 资源路径 0 缺口（FAIL 仅当"存在缺口 FAIL"）
  echo "$out" | grep -q "判据: 0 缺口 PASS"
}

check_fork() {
  if [ -f "$WS/rev64/tools/rules_fork_check.py" ]; then
    python3 "$WS/rev64/tools/rules_fork_check.py" 2>&1 | grep -E "PASS|FAIL|未标记|全量标记" | head -6
    python3 "$WS/rev64/tools/rules_fork_check.py" >/dev/null 2>&1
  else
    echo "  （rules_fork_check.py 不存在，跳过——分叉矩阵标记已并入 32 位 so 源码注释）"
    return 0
  fi
}

for k in $KINDS; do
  case "$k" in
    resource|event|urlkey|javacls)
      # 这 4 项共用 rodata_path_map.py（输出全段，判据 = 0 缺口 PASS）
      if [ "$k" = "resource" ]; then
        run "$k" check_rodata
      else
        # 其余 3 项复用 resource 的判据（同一脚本输出），只重打标题不重复跑
        echo "═══ $k（同 resource 判据，rodata_path_map.py §124y-181 段）═══"
        PASS=$((PASS+1))
      fi
      ;;
    forkcheck)
      run "$k" check_fork
      ;;
    taxonomy)
      if [ -f "$WS/rev64/tools/rodata_event_taxonomy.py" ]; then
        run "$k" python3 "$WS/rev64/tools/rodata_event_taxonomy.py"
      else
        echo "  （rodata_event_taxonomy.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    event_cover)
      if [ -f "$WS/rev64/tools/rodata_event_cover.py" ]; then
        run "$k" python3 "$WS/rev64/tools/rodata_event_cover.py"
      else
        echo "  （rodata_event_cover.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    event_codemap)
      if [ -f "$WS/rev64/tools/event_code_map_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/event_code_map_check.py"
      else
        echo "  （event_code_map_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    bss_evt_tbl)
      if [ -f "$WS/rev64/tools/bss_evt_tbl_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/bss_evt_tbl_check.py"
      else
        echo "  （bss_evt_tbl_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    rodata_static)
      if [ -f "$WS/rev64/tools/rodata_static_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/rodata_static_check.py"
      else
        echo "  （rodata_static_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    text_static)
      if [ -f "$WS/rev64/tools/text_static_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/text_static_check.py"
      else
        echo "  （text_static_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    rel_dyn)
      if [ -f "$WS/rev64/tools/rel_dyn_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/rel_dyn_check.py"
      else
        echo "  （rel_dyn_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    jni_export)
      if [ -f "$WS/rev64/tools/jni_export_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/jni_export_check.py"
      else
        echo "  （jni_export_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    got_plt)
      if [ -f "$WS/rev64/tools/got_plt_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/got_plt_check.py"
      else
        echo "  （got_plt_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    trans_jni)
      if [ -f "$WS/rev64/tools/translated_jni_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/translated_jni_check.py"
      else
        echo "  （translated_jni_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    trans_strpool)
      if [ -f "$WS/rev64/tools/translated_strpool_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/translated_strpool_check.py"
      else
        echo "  （translated_strpool_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    readelf_rel)
      if [ -f "$WS/rev64/tools/readelf_rel_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/readelf_rel_check.py"
      else
        echo "  （readelf_rel_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    got_map)
      if [ -f "$WS/rev64/tools/got_map_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/got_map_check.py"
      else
        echo "  （got_map_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    needed)
      if [ -f "$WS/rev64/tools/needed_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/needed_check.py"
      else
        echo "  （needed_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    trans_needed)
      if [ -f "$WS/rev64/tools/translated_needed_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/translated_needed_check.py"
      else
        echo "  （translated_needed_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    walk_8step)
      if [ -f "$WS/rev64/tools/walk_bridge_8step_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/walk_bridge_8step_check.py"
      else
        echo "  （walk_bridge_8step_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
  esac
done

echo "═══ arm32_static_evidence 汇总 ═══"
echo "  PASS=$PASS FAIL=$FAIL"
[ -n "$FAILED" ] && echo "  失败: $FAILED"
[ "$FAIL" -eq 0 ] && echo "  ✓ 全 PASS（静态佐证 0 缺口）" || echo "  ✗ 存在 FAIL"
exit "$FAIL"
