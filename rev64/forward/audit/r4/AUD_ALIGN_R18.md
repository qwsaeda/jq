# AUD_ALIGN_R18: 三轮对抗审计 (代码本体/工具链/文档交叉) P0+P1 修复
# 生成: 2026-11-17
# 方法: 3 子代理并行审计 (代码本体质量 / 构建工具链 / 文档-代码交叉) + 主会话核实修正

## P0 (★ 已修, 全回归 12/12 实锤)

### P0-1 glview_gl.c:446 渲染线程 `while(glGetError())` 无 context 判空 → 活锁
- **根因**: EGL surface 被系统回收/驱动异常时 "current context 失效但 g_gl_current 已置" 窗口内,
  `glGetError()` 无 context 下恒返非零 → while 死循环占满渲染线程 (假死黑屏),
  下方 0x3000 自愈段 (:480-491) 永远执行不到 = 自愈失效
- **修**: ① `eglGetCurrentContext() != EGL_NO_CONTEXT` 判前置 ② 16 次上限
  (正常 0 残留; 上限触发 = context 失效 → 走下方 0x3000 surface 重建自愈)
- **实锤**: `make so` 编译通过 + 全回归 12/12 PASS

### P0-2 render.c:11188 `rs_render_free` 漏 `free(rr->house_px)`
- **根因**: house_px (:739 png_decode malloc) 在 :11188 仅置 NULL 未 free, 且不在 px_list;
  二次 create/destroy 循环 (onPause→onResume) 每次泄漏 ~84KB (120×175×4)
- **修**: `free(rr->house_px); rr->house_px = NULL;` 加入 board/piece free 段
- **实锤**: 全回归 PASS

### P0-3 (子代理报 P0, 核实后降级 P1) rodata_path_map.py "0 缺口" 判据仅 11 主链
- **核实**: 73 名逐名对齐已有独立门禁 = align_gate ⑦ `event_names_alignment_check.py` (存在且挂 8/8)
  + ⑧ `sfx_coverage_check.py` (73 名→14 WAV) → 62 名非主链**非无门禁**, 子代理漏查
- **残留 (P1)**: rodata_path_map.py 自身判据口径弱 (11/11 即 "0 缺口 PASS"), 与 ⑦⑧ 职能重叠,
  建议 R19 标注 "本脚本 = 11 主链快速路径, 全量 73 名以 ⑦ 为准"

## P1 (★ 已修)

### P1-1 jni_bridge.c UNDO seq 跳号 2 → 走子闸口永久冻结 (regret_req + ☰菜单悔棋 2 处)
- **根因**: 钩子预取号 `++tx_seq` + 登记 pending_seq 后调 `jq_bt_ctrl_send`, 后者内部**再** `++tx_seq`
  取帧 seq → 帧 seq ≠ pending_seq (差 1) → 收 UNDO_ACK 时 :10727 `pending_seq == u_ref` 永不匹配
  → `undo_pending` 永不解锁 → :10378 走子闸口冻结
- **修**: ① `jq_bt_ctrl_send` 内部对 UNDO 型统一登记 pending_seq/last_req_seq/undo_pending
  ② :1268-1275 (regret_req 钩子) + :10096-10103 (☰菜单悔棋) 两处预取号段删除, 改传 seq=0
- **实锤**: 全回归 PASS (bt_semtest 152/152 + bt_semtest_dual 43/43 覆盖 UNDO 闭环断言)

### P1-2 jni_bridge.c JQ_EVENT_NAMES[e<0?0:e] 3 处 AI 块漏上界守卫
- **根因**: `JQ_EVENT_NAMES[7]` 数组, `e>=7` 时 OOB 读 (UB); 全仓 8 处同款中 5 处有 `ev<7?ev:0`
  上界 (:1223/2523/2557/2619 等), AI 块 :3002/:3706 漏 (对照 B03 注释 "唯此漏" 修复未同步到 AI 块)
- **修**: 2 处改 `JQ_EVENT_NAMES[(e >= 0 && e < 7) ? e : 0]`
- **实锤**: 全回归 PASS

### P1-3 UI_COMPARE.md:421 "未用 152" 旧快照漂移
- **根因**: 152 = 2026-11-10 快照 (272 datah - 当时 120 映射); §13 扩到 121 映射后未同步,
  与 UI_BUTTON_LEDGER "272 总量/111 按钮" 跨文档矛盾
- **修**: 改 "272 总量, 已映射 ~121/120, 无按钮 ~151" + 旧口径溯源注

### P1-4 UI_COMPARE.md:361 难度弹窗口径矛盾 (L361 "不弹" vs §11 "已对齐")
- **根因**: 11-10 实测 "原版记忆上次难度直开局" 口径 + 后续 src_rs 实现 UI_DIFFICULTY 弹窗路径
  (正向增强), 两口径并存无标注
- **修**: L361 加 R18 注: 原版 "不弹" = 记忆行为, src_rs "弹" = 首次选择增强, 非对齐目标

### P1-5 SPEC_BUSINESS.md:124 作废触摸公式 + R-route "14 On*" 口径
- **修**: L124 旧公式 (900×1468) 标注作废 + 现行 rs_ui_scale 同源公式; R-route L54/L90 统一
  "14 个 App On* + JNI_OnLoad (smali 15 native 方法)" 口径

### P1-6 工具链假绿防护缺口 (子代理工具链审计, 部分核实)
| 项 | 位置 | 状态 |
|----|------|------|
| arm32_static_evidence.sh "脚本不存在→PASS+1" | :150-185 | ★ 核实: 判据脚本实际**都存在** (子代理误判), 风险 = 未来误删即假 PASS 的**模式**; R19 改 "缺失→FAIL" |
| mumu_evtest.sh 无 pipefail, wait_log/fresh_log 裸管道 grep 掩码 | :14, 26-41 | R19 (设备侧, 加 `set -o pipefail` + adb rc 显式取) |
| arm32_verify_all.sh `--static-only` SKIPPED 无 exit 非 0 | :55-59 | R19 (加 SKIPPED 汇总段 `exit 2` 提示, 防当全 PASS) |
| pack_arm32.sh:58 apksigner 验证裸 grep "Verified" (同型 sign_apk.sh D6-#20 已修未同步) | :58 | R19 (grep "Verified.*: true") |
| so_probe.py/thunk_code_map.py 默认 so 路径无存在性守卫 | :32-38 | R19 (缺 so → return 1 判据码) |

## P2 (登记, 非阻塞)

- P2-1 jni_bridge.c:10546 OnBluetooth `GetStringUTFChars` NULL 早退不清 exception/不释放 extra
  (OOM 边缘, ART 边界回收 local ref 非累积; 修法 = 加 ExceptionCheck+DeleteLocalRef)
- P2-2 SPEC L144 死引用 "§5.2" (★ 已顺手修) / L106 "§AGENTS.md" 前缀误导 (登记)
- P2-3 CONSISTENCY.md "加密常量池" 旧误口径 (★ 已顺手修: 改 "高熵索引表, 非加密" 对齐 AGENTS 结论)
- P2-4 arm32_hook_check.sh:434 `exit "$FAIL"` >255 时低 8 位归零 (当前 4 项无影响, 钳位 1)

## 台账状态

- 总条目: P0×3 (1 降级) + P1×6 + P2×4 = 13 条
- 已修: **8** (P0-1/P0-2 代码 + P1-1/P1-2 代码 + P1-3/P1-4/P1-5 文档 + P2-1 顺手修 SPEC/CONSISTENCY 2 处)
- R19 输入: **5** (P0-3 口径标注 + P1-6 工具链 5 项 + P2-2/P2-4)
- 撤回: 1 (工具链 P0-1 "脚本缺失静默 PASS" 核实为误判, 降级模式风险 P1)
- P0: 2 (全修), P1: 6 (代码 2 + 文档 3 全修, 工具链 1 项入 R19), P2: 4 (2 顺手修 + 2 登记)

## 门禁基线 (R18 闭环, 2026-11-17 桌面实测)

- src_rs_regress.sh **12/12 + [2.5] + [2.6] 全 PASS** (P0-1/P0-2/P1-1/P1-2 修复后实跑)
- align_gate 8/8 + evverify 172/172 + bt_semtest 152/152 + bt_semtest_dual 43/43
- make so 编译通过 (0 新增告警)
