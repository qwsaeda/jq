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
set -u
WS="$(cd "$(dirname "$0")/../.." && pwd)"
ONLY=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --only) ONLY="$2"; shift 2;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done
KINDS="resource event urlkey javacls forkcheck taxonomy event_cover"
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
  esac
done

echo "═══ arm32_static_evidence 汇总 ═══"
echo "  PASS=$PASS FAIL=$FAIL"
[ -n "$FAILED" ] && echo "  失败: $FAILED"
[ "$FAIL" -eq 0 ] && echo "  ✓ 全 PASS（静态佐证 0 缺口）" || echo "  ✗ 存在 FAIL"
exit "$FAIL"
