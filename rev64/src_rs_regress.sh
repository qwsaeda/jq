#!/bin/bash
# src_rs_regress.sh — 源码重写路线一键全回归
# 用法: bash rev64/forward/src_rs_regress.sh [games]
# 跑完所有可机验的项：编译 / 蓝牙自测 / 桌面走局 / 规则回归 / 红线审计 / 链接审计
# 全部 PASS 才退出 0；任何一项 FAIL 则标红汇总
set -u
GAMES=${1:-20}
RS=/disk/workspaces/jq/rev64/forward/src_rs
TOOLS=/disk/workspaces/jq/rev64/tools
FAILED=()

run() {
    local name=$1; shift
    echo "=== $name ==="
    if "$@" ; then
        echo "  PASS: $name"
    else
        echo "  FAIL: $name"
        FAILED+=("$name")
    fi
}

# 1. 编译（desk + so）
run "[1/12] make desk + so" bash -c "cd $RS && make desk && NDK=/tmp/opencode/android-ndk-r25b make so"

# 2. 蓝牙帧协议 + 状态机桌面自测
run "[2/12] 蓝牙自测 (make bttest)" bash -c "cd $RS && make bttest 2>&1 | grep '全部通过'"
# 2.5 ★ P0-1 审计修复 (AUD_ALIGN_R17 承接 R16 P0)：bt_semtest 152 条断言池此前未挂 12/12
#   回归（make bttest 与 bt_semtest 源文件列表同源但 target 不同，[2/12] 只跑 bttest →
#   断言池扩容 145→152 后门禁不感知，静默降级）。挂上后 152/43 条进 12/12 覆盖。
run "[2.5/12] 蓝牙会话语义自测 (make bt_semtest)" bash -c "cd $RS && make bt_semtest 2>&1 | grep '152 条 PASS'"
run "[2.6/12] 双端语义自测 (make bt_semtest_dual)" bash -c "cd $RS && make bt_semtest_dual 2>&1 | grep '43 条 PASS'"

# 3. 桌面 headless 走一局
run "[3/12] 桌面走局 (jq_run)" bash -c "cd $RS && ./dist/jq_run >/dev/null 2>&1"

# 4. 规则引擎 N 局回归
run "[4/12] 规则回归 $GAMES 局 + AI 评分单测" bash -c "cd $RS && python3 regress.py --games $GAMES && make aittest 2>&1 | grep '全部通过'"

# 5. src_rs 红线审计（8+2 项）
run "[5/12] src_rs_check.py 红线审计" python3 $TOOLS/src_rs_check.py

# 6. 链接安全审计（UND 符号 MuMu 可解析）
run "[6/12] 布局 12 预设校验 (make laytest)" bash -c "cd $RS && make laytest 2>&1 | grep '12/12'"
run "[7/12] 触摸命中 + 两步态机 (make touchtest)" bash -c "cd $RS && make touchtest 2>&1 | grep '全部通过'"
run "[8/12] 完整对局自测 (make gametest，60 局全路径)" bash -c "cd $RS && make gametest 2>&1 | grep '全部通过'"
# 8.5 基准新鲜度门禁（★ AUD-1000 B08-037/B08-039：test_expected.txt 过期即 FAIL，
#   rules.c 改动后强制 jq_consistency 实际输出 == 文件基准，防内置 BENCH_EXPECT
#   双源漂移假绿——旧版无门禁"基准过期无人拦"）
run "[8.5/12] 基准新鲜度 (make benchcheck)" bash -c "cd $RS && make benchcheck 2>&1 | grep '✓ benchcheck'"
# ★ 渗透轮（2026-11-20 新增）：正向 C 5 变异器（make fuzzprobe ASan 桌面 + fuzz_probe_check.py
#   单 seed 快版 ~10s）。PENDING（binary 未构建）不阻断（12/12 门禁不因工具缺失假红）；
#   FAIL（渗透 0 崩溃破）阻断 = so 逻辑回归白盒定位回路入口。深版 3 seed × 500 局 =
#   arm32_verify_all 总入口 / 手动 `fuzz_probe_check.py` 全量判据。
run "[8.6/12] 正向渗透轮 5 变异器 (make fuzzprobe + fuzz_probe_check)" bash -c "cd $RS && make fuzzprobe >/dev/null 2>&1; python3 $TOOLS/fuzz_probe_check.py --seed 7 --games 100 --touch 1000 --events 200 --saves 50 --offline"
# ★ P1/P2 渗透面（2026-11-20）：PNG 畸形输入 fuzz + UI 状态机随机点击 fuzz
#   两者都是桌面零设备依赖（ASan），失败 = so 逻辑/资源链回归 → 阻断装机
run "[8.7/12] P1 渗透面 PNG 畸形输入 fuzz (make fuzzpng)" bash -c "cd $RS && make fuzzpng SEED=7 ROUNDS=1500 2>&1 | grep '判据: PASS'"
run "[8.8/12] P2 渗透面 UI 状态机点击 fuzz (make fuzzui)" bash -c "cd $RS && make fuzzui SEED=7 STEPS=800 2>&1 | grep '判据: PASS'"
# ★ P5' 渗透面：JNI 入口参数 fuzz（mock JNIEnv + fork 隔离，桌面零设备）
#   14 入口 × 4 毒化档 × 6 变体 = 336 调用组合；缺口清单 = 入口防御缺口（信息产出）
run "[8.9/12] P5' 渗透面 JNI 入口参数 fuzz (make fuzzjni)" bash -c "cd $RS && make fuzzjni SEED=7 ROUNDS=1 2>&1 | grep '判据: PASS'"
run "[9/12] so_link_check.py 链接审计" python3 $TOOLS/so_link_check.py $RS/dist/libjunqi_rs.so
# 10. 走子预验证工具（三件套之一，防新工具回归）
run "[10/12] 走子预验证 (make walkverify)" bash -c "cd $RS && make walkverify SEQ='25,30 27,32 28,33' 2>&1 | grep '3/3 步合法'"
run "[11/12] 真实长路径走子 (make walkverify --longrun 5)" bash -c "cd $RS && make walkverify LONG='5' 2>&1 | grep '5/5 回合 turn 交替 持续'"
# 12. 桌面事件验证（EAT/BOMB/MOVE/FLAG/MINE/UNDO/DRAW/RESIGN/STALEMATE 9 断言 + 120 弹窗几何 + 规则回归，防事件夹具回归）
# ★ AUD-1000 B07-001/B08-049（P1 判据口径同步）：evverify 退码分级——事件项
#   FAIL 退 1（阻断 12/12）；仅几何项 FAIL 退 2（警告，单弹窗几何漂移不再
#   过度敏感阻断全回归）。判据仍 grep "全对"（动态真值，SUMMARY 实时生成——★ P1-5 审计修复:
#   注释不再硬编码断言数, 历史 8/8→167/172 三代口径漂移教训, 以 SUMMARY 行为准），退 2 时
#   "全对" 不出现（几何 FAIL 行）→ [12/12] FAIL 但日志可区分"事件阻断"
#   与"几何警告"（stdout 含"退 2（警告"字样）。
run "[12/12] 桌面事件验证 (make evverify)" bash -c "cd $RS && make evverify MODE=--all 2>&1 | grep '全对'"

echo
if [ ${#FAILED[@]} -eq 0 ]; then
    echo "★ 全回归 PASS（12/12）"
    exit 0
else
    echo "★ 回归有 ${#FAILED[@]} 项 FAIL: ${FAILED[*]}"
    exit 1
fi
