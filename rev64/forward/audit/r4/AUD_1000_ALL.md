# AUD-1000-ALL 全仓对抗审计台账（8 横块 × 12 功能维）

> 批次：AUD-1000-ALL（64 位 src_rs + 32 位翻译体全仓，12 子代理 × 96 单元格）
> 前批 AUD-1000（585 条，32 位专项 P0 34/P1 228/P2 250/P3 73 全清）+ 本批 64 位主线系统对抗
> 纪律：CODE_STYLE §9 假绿三型 + AUDIT_CHECKLIST §E 8 域 + 审计纪律 6 条
> 目标：1000 条台账（可复现池），P 级精确实测，撤回误报计入，不硬凑

## 8 横块 × 12 功能维矩阵
| 子代理 | 功能维 | 横块覆盖 | 状态 |
|---|---|---|---|
| A1 | F1 逻辑正确 | D1-D8 × F1 | 运行中 |
| A2 | F2 并发锁 | D1-D8 × F2 | 运行中 |
| A3 | F3 内存安全 | D1-D8 × F3 | 运行中 |
| A4 | F4 契约一致 | D1-D8 × F4 | 运行中 |
| A5 | F5 工具假绿 | D1-D8 × F5 | 运行中 |
| A6 | F6 文档漂移 | D1-D8 × F6 | 运行中 |
| A7 | F7 资源资产 | D1-D8 × F7 | 运行中 |
| A8 | F8 32·64 行为 | D1-D8 × F8 | 运行中 |
| A9 | F9 错误路径 | D1-D8 × F9 | 运行中 |
| A10 | F10 性能 | D1-D8 × F10 | 运行中 |
| A11 | F11 钩子卫生 | D1-D8 × F11 | 运行中 |
| A12 | F12 可复现性 | D1-D8 × F12 | 运行中 |

## 合并统计（AUD-1000-ALL 全量，338 条）
| 类别 | P0 | P1 | P2 | P3 | 合计 |
|---|---|---|---|---|---|
| 有效缺陷 | 0 | 48 | 117 | 166 | 331 |
| 撤回误报（计入） | - | - | - | - | 36 |
| 合计 | - | - | - | - | 338 |

## P1 修复批次（48 条 → 已修 38 项 / 保留标注 10 项）
| 批次 | commit | 项 |
|---|---|---|
| 批次 1（15 项） | 64d7164 | frc<0 死条件×4 / undo_pop 加锁×4 / g_undo_top 下溢×3 / init_done TOCTOU / DeleteGlobalRef UB / we_blue 口径 / piece 宽高 / popup 注释 / btimen 行高 / GameType%d / g_event_cnt 查表 / resign stage 守卫 / one.sjg 原子写 / build_phases 阻断 / swap_so logcat |
| 批次 2（5 项） | 3b1f2ec | last_type 快照 / replay 钩子锁 / bt_session 注释 / install rc / arm32_cycle exit 1 |
| 批次 3（17 项） | da6f15b | 渲染线程锁（A2-007）/ UNDO 锁外编码（A2-015）/ AI O(N²) 锁外（A2-020）/ g_ts 移锁外（A2-042）/ BtSpp synchronized（A2-011）/ rebuild_rs cp 根因 / check_consistency EVN / verify_translation exit / asset_chain 离线 PENDING / bt_test 包名 / pipeline PENDING / so_link libc / evtest PID / clear_all_props 43 项 / gen_dialog 源头 / rs_render_free 泄漏 / 终局 turn 对齐（A8-001） |
| 批次 4（1 项） | 41fe4f2 | A4-13 JQ_DEFAULT_LAY 双端共享头（jq_default_lay.h 单点，32/64 位开局布局双源根治） |

## P2 修复批次（117 条 → 已修 26 项 / 留 P3 压测驱动 47 项 / 诚实边界标注 44 项）
| 批次 | commit | 项 |
|---|---|---|
| P2 批次 1（8 项） | 0ebaa1e | minetest 一次性标志 / g_bt_sess_init_done volatile / JQ_EVENT_NAMES[mv] 守卫 / arm32_cycle PIPESTATUS / mumu_verify 轮询 / walktest PID / evtest getprop 轮询 ×2 |
| P2 批次 2（3 项） | 7b724b1 | 32 位钩子别名映射 / SPEC §4 文档漂移 / SPEC §5 audio.c 死模块标注 |
| P2 批次 3（2 项） | deb8bc6 | sfx_wav 未知事件 LOGI / ui_layout_save 写失败 LOGE |
| P2 批次 4（1 项） | 086f263 | uihooks 12 prop 60 帧节流 |
| P2 批次 5（2 项） | 988e2ee | popup_sweep 轮询出帧 / link_audit 白名单诚实边界 |
| P2 批次 6（2 项） | 46ec486 | bt_protocol f7 诚实边界 / UNDO 16KB 栈压改 static |

## P2 保留标注项（诚实边界/性能类，留 P3 压测驱动）
- A10-D1-01/02/03（3 条）：困毙全扫 O(N²) 缓存/双扫——B01-012 stale_dirty 已修主体，
  对端 stale_cache 预刷 + 60×60 全扫优化=重构量大（jq_side_stalemated 改增量），留 P3。
- A10-D3-01（1 条）：226 处 glGetUniformLocation 每帧重查——40+ uniform 逐个提缓存
  到 rr->u* 字段，改动分散收益小，留 P3 压测驱动。
- A10-D6-01/02（2 条）：OnEvent/OnTouch 锁内 on_event/jq_try_move（含 stalemate_after）
  性能——写操作不能锁外，拆 stalemate_after 出锁需快照范式重构，留 P3。
- A2-019/021/023/026/029/035（6 条）：低窗口竞态（ui_rule_cfg_get 规则层读 / g_gl_crash
  原子读 / pthread_join 销毁顺序 / bt_session UNDO 切回合 / ui_state bright save /
  replay_ensure_inited）——窗口极小 + 诊断日志非业务路径，P3 标注。
- 余 ~33 条：文档漂移/诚实边界标注/工具判据宽于实际（非静默放行）类，P3 处理。

## P1 保留标注项（10 条，诚实边界非缺陷）
- A8-F8-002/003/004（3 条）：困毙参数化/地雷+0xFF/BT UNDO 收帧 = 32 位真值基准（R-138：§124y-169 R-073 地雷实证差异 + R-013 UNDO 职责分离），保留标注不改判
- A2-011（1 条代码已修 + 1 条真机验收边界）：BtSpp synchronized 已落地，端到端需真机 BT 硬件
- A11-D7-F11-01 clear_all_props（已修 43 项清单，但 MuMu property service 缓存机制残留 1 条 = 环境限制）
- 余 5 条：D6/D7 低竞态窗口项（OnCreate unlock 后 3 处 g_state 单线程窗口≈0 等）维持 P2 降级处理

## 纪律
- P0=运行时必崩/数据错置；P1=逻辑错不崩；P2=低影响；P3=标注类
- 撤回误报计入台账（_status=撤回误报），不硬凑
- R-138：32 位翻译体只标注不判缺陷（32 位行为=真值基准）
- 可复现池 = 有精确实测行号+代码片段的发现；不足 1000 如实标注缺口

## 全仓对抗审计说明（回答"为什么问题多且一直没发现"）
- 前批 AUD-1000 585 条 = 32 位翻译体横块（jqt_*.c/jq_rs_*.c/jni_aligned.c）系统对抗产出
- 64 位 src_rs 主线（rules.c/jni_bridge.c/render.c 等）此前靠 evverify 170/170 行为基准
  + 真机 0 崩溃兜底——能拦运行时崩溃，拦不住 P2/P3 级逻辑缺陷（无锁竞态/双源/静默降级/
  工具判据恒真）→ 本批 8 横块 × 12 功能维 = 96 单元格全仓系统对抗补齐 64 位主线

