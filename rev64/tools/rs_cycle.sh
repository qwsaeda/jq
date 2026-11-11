#!/bin/bash
# rs_cycle.sh — src_rs 全周期一条龙（编译 + 回归 + 装机 + 启动）
# 固化：每轮手写 "make so + gametest + rebuild_rs_apk.sh --start" 序列 → 一条命令。
# 用法:
#   bash rev64/tools/rs_cycle.sh              # make so + 12/12 回归 + rebuild --start
#   bash rev64/tools/rs_cycle.sh --no-start   # 只装机不启动
#   bash rev64/tools/rs_cycle.sh --no-regress # 跳过回归（快速换 so）
#   bash rev64/tools/rs_cycle.sh --dev IP:PORT
# 判据: 编译无 error + 10/10 全 PASS + so 哈希对账一致 + 启动进程存活
set -u
RS=/disk/workspaces/jq/rev64/forward/src_rs
TOOLS=/disk/workspaces/jq/rev64/tools
DEV=${DEV:-10.0.182.146:5555}
START=1
REGRESS=1
DEVREGRESS=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-start) START=0; shift;;
    --no-regress) REGRESS=0; shift;;
    --device-regress) DEVREGRESS=1; shift;;
    --dev) DEV="$2"; shift 2;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done

log() { echo "[rs_cycle] $*"; }
log "=== [1/3] 编译 src_rs so ==="
cd $RS
rm -f dist/libjunqi_rs.so
NDK=/tmp/opencode/android-ndk-r25b make so >/tmp/opencode/rs_cycle_build.log 2>&1
if [ $? -ne 0 ]; then
  log "★ 编译失败"; grep -E "error:" /tmp/opencode/rs_cycle_build.log | head -5; exit 1
fi
log "编译完成: $(stat -c%s dist/libjunqi_rs.so)B md5=$(md5sum dist/libjunqi_rs.so | cut -d' ' -f1)"

if [[ $REGRESS -eq 1 ]]; then
  log "=== [2/3] 12/12 全回归 ==="
  bash /disk/workspaces/jq/rev64/forward/src_rs_regress.sh >/tmp/opencode/rs_cycle_regress.log 2>&1
  if [ $? -ne 0 ]; then
    log "★ 回归有 FAIL"; grep -E "FAIL" /tmp/opencode/rs_cycle_regress.log | head -5; exit 1
  fi
  grep "全回归" /tmp/opencode/rs_cycle_regress.log | tail -1
else
  log "=== [2/3] 跳过回归（--no-regress）==="
fi

log "=== [3/3] rebuild_rs_apk.sh 装机+启动（--start=$START）==="
# ★ 对抗审计 P3-J9（错误吞噬修正）：原 `| grep -E "对账|启动 app|进程|完成|FAIL"` 过滤
#   后即丢退出码——rebuild 内部失败但 grep 不命中关键词（如 "so 哈希不一致" 含"不一致"
#   不在 pattern）→ 假绿。现：先落盘全文 + 保留 pipeline 退出码，再 grep 展示；
#   失败即中止（不静默继续到设备回归）。
if [[ $START -eq 1 ]]; then
  bash $TOOLS/rebuild_rs_apk.sh --start --dev $DEV > /tmp/opencode/rs_cycle_rebuild.log 2>&1
  RC=$?
  grep -E "对账|启动 app|进程|推送|完成|FAIL|不一致|✗" /tmp/opencode/rs_cycle_rebuild.log | head -12
  [ $RC -ne 0 ] && { log "★ rebuild_rs_apk 失败（rc=$RC），日志 /tmp/opencode/rs_cycle_rebuild.log"; exit 1; }
else
  bash $TOOLS/rebuild_rs_apk.sh --dev $DEV > /tmp/opencode/rs_cycle_rebuild.log 2>&1
  RC=$?
  grep -E "对账|推送|完成|FAIL|不一致|✗" /tmp/opencode/rs_cycle_rebuild.log | head -12
  [ $RC -ne 0 ] && { log "★ rebuild_rs_apk 失败（rc=$RC），日志 /tmp/opencode/rs_cycle_rebuild.log"; exit 1; }
fi

if [[ $DEVREGRESS -eq 1 ]]; then
  log "=== [4/4] 设备事件回归（mumu_regress.sh，含 btconn）==="
  bash $TOOLS/mumu_regress.sh --dev $DEV > /tmp/opencode/rs_cycle_devregress.log 2>&1
  RC=$?
  grep -E "✓|✗|~|汇总" /tmp/opencode/rs_cycle_devregress.log | tail -14
  # ★ FLAKY/FAIL 均非全绿（rc=2 flaky / rc=1 fail）→ 中止，不静默
  # ★ AUD-1000 B07-003（P1 已修，flaky 处置细化）：rc=2（FLAKY，首跑瞬时时序
  #   重跑过）改为**记录 flaky 项 + 标记继续构建**而非直接 exit——旧版
  #   FLAKY 即中止使"真慢日志 20-25s 才出"的设备（MuMu 冷启 GLThread
  #   attach 慢）在 so 已装机成功的场景下整条 rs_cycle 白跑（重新装机+
  #   重回归）。现：rc=2 打印 flaky 项清单（grep '~' 行）→ 继续到
  #   "✓ rs_cycle 完成（含 flaky 项，建议人工复核）"；rc=1（FAIL）仍
  #   中止（真回归/双跑失败不可放过）。
  case $RC in
    0) : ;;
    2)
      FLAKY_ITEMS=$(grep -E "^  ~ " /tmp/opencode/rs_cycle_devregress.log | sed 's/^  ~ //')
      log "⚠ 设备回归 FLAKY（rc=2，记录 flaky 项不中止构建）：$FLAKY_ITEMS"
      log "  （B07-003：B07 批 wait_log 窗口 40s + fresh_log PID 新鲜度校验后，flaky 应为真瞬时时序；连续 2 次 flaky 同项 = 真回归嫌疑，人工复核）"
      ;;
    *)
      log "★ 设备事件回归 FAIL（rc=$RC）"; exit $RC ;;
  esac
fi
if [[ $DEVREGRESS -eq 1 && $RC -eq 2 ]]; then
  log "✓ rs_cycle 完成（含 FLAKY 项，so 已装机；flaky 项建议人工复核后重跑 --device-regress）"
  exit 0
fi
log "✓ rs_cycle 完成"
exit 0
