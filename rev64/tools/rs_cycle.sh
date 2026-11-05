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
RS=/disk/workspaces/jq/rev64/src_rs
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
  bash /disk/workspaces/jq/rev64/src_rs_regress.sh >/tmp/opencode/rs_cycle_regress.log 2>&1
  if [ $? -ne 0 ]; then
    log "★ 回归有 FAIL"; grep -E "FAIL" /tmp/opencode/rs_cycle_regress.log | head -5; exit 1
  fi
  grep "全回归" /tmp/opencode/rs_cycle_regress.log | tail -1
else
  log "=== [2/3] 跳过回归（--no-regress）==="
fi

log "=== [3/3] rebuild_rs_apk.sh 装机+启动（--start=$START）==="
if [[ $START -eq 1 ]]; then
  bash $TOOLS/rebuild_rs_apk.sh --start --dev $DEV 2>&1 | grep -E "对账|启动 app|进程|完成|FAIL"
else
  bash $TOOLS/rebuild_rs_apk.sh --dev $DEV 2>&1 | grep -E "对账|完成|FAIL"
fi

if [[ $DEVREGRESS -eq 1 ]]; then
  log "=== [4/4] 设备事件回归（mumu_regress.sh，含 btconn）==="
  bash $TOOLS/mumu_regress.sh --dev $DEV 2>&1 | grep -E "✓|✗|汇总"
fi
log "✓ rs_cycle 完成"
exit 0
