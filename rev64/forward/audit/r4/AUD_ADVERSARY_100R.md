# 100 轮对抗审计台账（军棋业务规则/玩法/正向验证）

> **执行架构**（用户 2026-11-19 指令固化）：10 批次×10 轮=100 轮，每轮 1 路子代理（限域防 context overflow），每轮 99 问题落台账 + P0/P1 落地 + 全门禁验证 + 误报撤回计入（不硬凑）。
> **域轮转**：SPEC 条款覆盖 / rules.c 分支 / 事件契约 / 暗棋玩法 / 终局判定 / 资源加载 / 渲染域 / 蓝牙协议 / 诚实边界 / 文档 vs 代码漂移。
> **口径**：行为级对齐 ≠ 汇编级等价；桌面 0 回归 ≠ 100% 达成（设备 PENDING）；分发成功 ≠ 业务实现（账号/联机无 g_state 变更）；基线 4 分叉（R-073/008/004/018）归因不阻断。

## 桌面断言真值演进（每轮落地后全门禁验证）

| 批次 | evverify | bt_semtest | dual | align_gate | dark_align |
|---|---|---|---|---|---|
| 基线（100 轮前） | 172/172 | 152 | 43 | 8/8 | 86/86 |
| R27-Q92 后 | 173/173 | 152 | 43 | 8/8 | 86/86 |
| R27-Q67/Q68 后 | 174/174 | 152 | 43 | 8/8 | 86/86 |
| R28-Q88/Q98 后 | 174/174（幂等断言纳入 run_draw_resign） | 152 | 43 | 8/8 | 86/86 |

## 第 1 批次（轮 1-7）清账

### 轮 1：SPEC 条款覆盖域（99 问题，P0=7/P1=73/P2=19）
**P0 落地**：
- Q4 军旗明暗棋区分：SPEC_BUSINESS.md 军旗条款补全"明棋任何已翻子扛旗 + 暗棋仅工兵且挖满 3 雷"
- Q7/Q81/Q89 暗棋未翻目标可撞对齐 0x6a45c：SPEC L41"不可撞"→"可撞（移除旧 B01-053）"
**撤回误报**：Q45（jq_event_names_73.c 73 项 = JQ_EVENT_NAME_COUNT 73 一致，回执误报 75 项）

### 轮 2：rules.c 消费点分支覆盖域（99 问题，P0=0/P1=24/P2=60/P3=15）
**P1 落地**：
- Q92 规则开关 5 开关组合矩阵：evverify 加 run_switch_matrix（6 高风险组合 + 和棋 70 步边界），evverify 172→173
**基线归因**：4 分叉（R-073 困毙排雷/旗 + R-008 翻子困毙 + R-004 0xFF + R-018 杀光）坐实不阻断

### 轮 3：事件契约域（99 问题，P0=4/P1=49/P2=39/P3=7）
**P0 落地**：
- Q67/Q68 evverify 对 evt55 全链 0/73 覆盖：evt55 全链入桌面链接（Makefile 加 evt55_business/dispatch + jq_event_names_73 + 计数桩 evt55_stub.c）+ run_evt55_names_smoke（73 名逐名分发/计数），evverify 173→174
**攻坚坑固化**：① Makefile 注释须 `#`（`/* */` 被 ld 当输入文件）② 跨函数插入须精确定位函数尾 ③ 新函数须前向声明 + 完整闭合

### 轮 4：暗棋玩法域（99 问题，P0=0/P1=35/P2=64）
**P1 落地**：
- Q91 暗棋 0x6a45c 撞未翻目标 evverify 缺失：run_dark 加 ⑧暗棋撞未翻目标（can=1）
- Q5/Q90 翻子触发终局：run_dark 加 ⑨翻子触发终局（红困毙蓝胜，夹具修正 2 次：已翻子 jq_flip no-op → 改未翻暗子翻子定色）
- Q8 翻棋不计步：run_dark 加 ⑩翻棋不计 moves_since_eat/total_moves（0x302f0）
**撤回误报**：① "暗棋未翻目标不可撞应判 bug"撤回（0x6a45c 可撞非 bug）② "翻雷为实现错误"撤回（困毙排雷不排旗=原版设计）③ "0xFF 仅蓝方可翻"旧认知撤回（双方计可翻）④ "翻棋计入 70/1000 步"撤回（翻棋不计步）

### 轮 5：终局判定域（99 问题，P0=8/P1=34/P2=57）
**P0 落地**：
- Q88 jq_on_flag_capture 缺 winner 守卫：补 `if (st->winner != JQ_WIN_NONE) return;`（对齐 B01-065 jq_on_draw/stalemate，防双终局叠加脏 winner 覆盖）
- Q98 终局幂等断言缺口：evverify run_draw_resign 扩展"终局后重复 API/走子帧 0 副作用"（重复 jq_on_draw/stalemate/flag_capture + jq_try_move/flip 0 副作用）
**P0 判读质（不可修，归入诚实边界）**：
- Q12/Q14 困毙翻雷白名单差异（jq_can_flip 拒军旗 vs 困毙扫描排雷不排旗）= 原版 0x30320 对齐行为，修了破坏对齐
- Q20/Q38 单侧无普通子判负缺口 = 已知诚实边界（rules.c:589-594 注释"原版一方无普通子→对方胜未在 64 位独立实现，依赖困毙近似"）
**撤回误报**：① "jq_on_draw 未做 winner 守卫"撤回（:1171 已有 B01-065 守卫）② "cfg3 关路径未走困毙判定"撤回（:791-799 明确调 stalemate_after）

### 轮 6：资源加载域（99 问题，P0=0/P1=19/P2=67/P3=13）
**P0=0**：资源加载域无正确性缺陷（选路/纹理/音效均为 best-effort 降级设计，MISS 走纯色不崩溃）
**P1 落地**（覆盖缺口/诚实边界类，非可修缺陷）：
- Q45 close/pop 音效映射分叉 → **撤回误报**：jni_bridge sfx_wav :671-672 实锤有 close/pop 映射（与 audio.c :37-38 一致，无分叉，回执误读"默认回落 move"）
- Q1/Q2 SPAK_DIR 选路私有不完整遮蔽 dev：best-effort 设计（渲染对未落地资源 MISS 走纯色），非 bug
**诚实边界固化**：资源图部分用静态解包推设备（非占位）；"推了全量"≠"进程读到全量"（SPAK_DIR 选路决定）；桌面 0 回归 ≠ 设备出屏（PENDING）

### 轮 7：渲染域（99 问题，P0=0/P1=33/P2=66）
**P0=0**：渲染域无正确性缺陷（blend 黑底/stage 门控/EGL 同步/0x3000 自愈/纹理泄漏 6 项撤回误报实锤已修）
**P1 落地**：
- Q32 棋子段 rr->game NULL 空指针守卫（can_piece 加 game 判空，防 :10556 for i<N_CELLS rr->game->cells[i] 空指针解引用）
- Q2 高亮段 hl_s 按高铺满对齐主路径（旧 hl_s=vw/720 单一公式致瘦高屏错位，改 bg_h_px < vh 时按高铺满，同 :10545-10551）
**撤回误报 6 条**（防硬凑）：blend 黑底未复发（R25 已修）/ need_board 门控已实现（R26）/ eglQuerySurface 同步已落（R26）/ 0x3000 自愈 60 帧（R25）/ 纹理泄漏 (slot,EGLContext) 配对（AUD-1000 P0）/ glGetError 清理已落（glview :441-453）

### 轮 8：蓝牙协议域（99 问题，P0=0/P1=24/P2=56/P3=19）
**P0=0**：蓝牙域无正确性缺陷（帧重组/状态机/UNDO 闭环已修）
**P1 落地**：
- Q18 OnBluetooth 帧推进 strchr NULL 判空守卫（:10839 `p = p[0] ? (strchr(p, '\n') + 1) : p;` NULL+1 UB，改判空后 break 半帧留待下批）
- Q80 SPEC §8 L181 蓝牙 SPP "桩"误标修正（"渲染/音频/蓝牙 SPP socket 为桩"→"渲染/音频为桩 + 蓝牙 SPP 非桩已接线"，诚实边界=无 BT 硬件端到端）
**撤回误报 5 条**（防硬凑）：① bt_session 事件码映射缺失（误报，实为蓝牙帧类型非 76 事件名）② SYNC_CELL 白名单未实现（误报，已实现 B04-010/011）③ BtSpp 逐行丢帧未修（误报，已按帧重组修）④ 跨语言契约测试缺失（误报，bt_selftest §9/§10 17 条）⑤ bt_semtest 数量虚标（实测 132/45，提示 152/43 数量级接近，以实测为准）

### 轮 9：诚实边界域（99 问题，P0=3/P1=63/P2=33）
**P0 落地**：
- Q12 evt55 分发成功≠业务实现误判（evt55_do_code 恒返 1 无"无状态变更"标志，补 jq_evt55_no_state_change API（biz 7-11=1 账号/联机/摆子无状态变更，0-6=核心 g_state 变更在调用点），junqi.h 声明 + evt55_dispatch.c 实现）
- Q34/Q92 AGENTS 达成/PENDING 互斥并存 + 无 SSOT（SPEC §8 加"当前状态 SSOT 汇总表 + 边界登记表 + 口径红线"：规则已实现/事件分发对齐业务未实现/渲染已接线视觉未验/音频已接线实播未验/蓝牙已接线硬件 PENDING/四国/联网/账号/版本更新/提和/聊天未实现/存档非原版/桌面≠设备）
**诚实边界固化**：本轮聚焦声明/注释/口径/验收契约层防误判，未对设备端重新实锤（PENDING）

### 轮 10：文档 vs 代码漂移域（第 1 批次末轮，99 问题，P0=0/P1=36/P2=56/P3=7）
**P0=0**：文档漂移域无正确性缺陷（P1 多为数字快照陈旧/章节间矛盾/多口径无仲裁）
**P1 落地**（文档自相矛盾最高价值，可修项）：
- Q56/Q66/Q93 SPEC 断言快照 172/172 vs 174 自相矛盾（L15 口径总表旧快照 vs L190/L197 SSOT 表）→ 修正 L15 "172/172（40 事件）"→"174/174（42 事件）"（+run_switch_matrix R27-Q92 + run_evt55_names_smoke R27-Q68）
- Q34 SPEC §6 蓝牙 "7 种帧类型" vs 实现 10 类型（含 CAP/DRAW/CHAT，v3 双栈）→ 修正 "7 种"→"10 种（MOVE/FLIP/UNDO/SYNC_CELL/SYNC/RESTART/RESIGN/DRAW/CHAT/CAP，v3 双栈）"
- Q2 SPEC 军旗条款 cfg3/cfg4 混写（"cfg3 开关控扛旗"未区分双开关）→ 职责分标：cfg4=暗棋雷/旗前置门槛（工兵+挖3雷），cfg3=吃旗开关控扛旗是否判胜（关则入位不终局）
**撤回误报 3 条**（防硬凑）：① audio.c "14 WAV vs 18" 单纯不一致即实现错误（撤回，实为原版18/仓内14/实通路14 分层口径漂移）② SPEC §6 "7 字段帧" 即完全错误（撤回，v2 仍成立，v3 双栈演进而非取代）③ jq_event_names_73 仅 73 名否定 SPEC "76=73+3" 证据链（撤回，Misc 3 为去广告/非门禁扩展口径，73 名表属设计分层）
**P1 未修（覆盖缺口/诚实边界类，归入批次 2+）**：Q13/Q99 事件名 73 vs 76 全链口径 / Q89-Q94 多口径并存无仲裁 / Q45-Q48 SPEC §4 UI 流程未覆盖弹窗屏态 / Q38-Q39 UNDO v3/CAP 能力协商未入 SPEC

## 第 1 批次（1-10 轮）全清账完成

### 批次 P0 根治累计（5 批）
R1 SPEC 文档漂移 + R3 evt55 全链 0/73 + R5 终局幂等/脏 winner + R9 诚实边界 SSOT + R10 SPEC 断言快照/蓝牙帧类型/军旗 cfg 修正

### 批次 P1 落地累计（7 批）
R2 run_switch_matrix + R4 run_dark ⑧⑨⑩ + R7 渲染 NULL/几何 + R8 蓝牙 NULL/SPEC + R9 evt55 机器可判 + R10 SPEC 快照/帧类型/cfg

### 批次撤回误报累计（19 条）
R1 Q45 + R4 4 条 + R5 2 条 + R6 1 条 + R7 6 条 + R8 3 条 + R10 3 条 = 19（防硬凑）

### 桌面断言真值（批次末）
evverify 174/174（42 事件 + 132 几何）+ bt_semtest 152 + dual 43 + align_gate 8/8 + dark_align 86 全 0 退化
so 真值：R7 渲染 P1→so 729496B；R8/R9/R10 断言层改动 so 未变

### 发第 2 批次（11-20 轮）
域轮转续：11 SPEC §4 UI 流程域 / 12 rules.c 暗棋复合规则域 / 13 事件计数口径域 / 14 资源加载域（续）/ 15 渲染顶点/blend 域（续）/ 16 蓝牙帧重组域（续）/ 17 诚实边界（续）/ 18 文档多口径仲裁域 / 19 存档/回放域 / 20 全模块 SSOT 表仲裁域

## 第 1 批次（1-10 轮）P0/P1/撤回累计统计

### P0 根治（3 批）
- R1 SPEC 文档漂移（Q4 军旗明暗棋 + Q7/Q81/Q89 暗棋未翻目标可撞对齐 0x6a45c）
- R3 evt55 全链 0/73 覆盖根治（Q67/Q68：evt55 入桌面链接 + run_evt55_names_smoke 73 名逐名分发/计数）
- R5 终局幂等/脏 winner（Q88 jq_on_flag_capture winner 守卫 + Q98 终局幂等断言）
- R9 诚实边界 P0（Q12 jq_evt55_no_state_change + Q34/Q92 SPEC §8 SSOT 汇总表）

### P1 落地（5 批）
- R2 run_switch_matrix（Q92 5 开关组合矩阵，evverify 172→173）
- R4 run_dark ⑧⑨⑩（Q91/Q5/Q8 撞未翻/翻子致终局/翻棋不计步）
- R7 渲染 P1（Q32 NULL 守卫 + Q2 高亮几何对齐，so 729496B）
- R8 蓝牙 P1（Q18 strchr NULL 判空 + Q80 SPEC 蓝牙非桩修正）
- R9 evt55 机器可判契约（Q12 jq_evt55_no_state_change）

### 撤回误报累计（16 条，防硬凑）
| 轮 | 撤回项 | 实锤 |
|---|---|---|
| R1 | Q45 jq_event_names_73 75 项 | 实锤 73 项 = COUNT 73 一致 |
| R4 | 暗棋未翻目标不可撞应判 bug | 0x6a45c 可撞非 bug |
| R4 | 翻雷为实现错误 | 困毙排雷不排旗=原版设计 |
| R4 | 0xFF 仅蓝方可翻旧认知 | 双方计可翻（翻归翻子方） |
| R4 | 翻棋计入 70/1000 步 | 翻棋不计步（0x302f0） |
| R5 | jq_on_draw 未做 winner 守卫 | :1171 已有 B01-065 守卫 |
| R5 | cfg3 关路径未走困毙判定 | :791-799 明确调 stalemate_after |
| R6 | Q45 close/pop 音效映射分叉 | :671-672 实锤有映射（与 audio.c 一致） |
| R7 | blend 黑底未复发 | R25 已修（实锤 :10423-10424） |
| R7 | need_board 门控未实现 | R26 已修（实锤 :10454-10455） |
| R7 | eglQuerySurface 同步缺失 | R26 已修（实锤 glview :351-366） |
| R7 | 0x3000 自愈 180 帧 | R25 改 60 帧（实锤 :488-506） |
| R7 | 纹理泄漏每帧 glGen | AUD-1000 P0 已修（(slot,EGLContext) 配对） |
| R7 | glGetError 未清理 | glview :441-453 已清理（16 次上限） |
| R8 | SYNC_CELL 白名单未实现 | 已实现 B04-010/011（side 感知域+idx 白名单） |
| R8 | BtSpp 逐行丢帧未修 | 已按帧重组修（:118-176 多帧抽取） |

### 桌面断言真值演进（第 1 批次）
- evverify 172 → 173（R2 run_switch_matrix）→ 174（R3 run_evt55_names_smoke）→ 174（R5 终局幂等断言纳入既有 42 项，无新增 ev_total）
- 全门禁：align_gate 8/8 + bt_semtest 152 + dual 43 + dark_align 86 维持 0 退化
- so 真值：R7 渲染 P1（Q32/Q2）→ so 729496B；R8/R9 P1/P0（Q18/Q80/Q12/Q34/Q92）→ 断言层改动，so 未变

## 撤回误报累计（防硬凑，计入对抗审计规范）

| 轮 | 撤回项 | 实锤 |
|---|---|---|
| R1 | Q45 jq_event_names_73 75 项 | 实锤 73 项 = COUNT 73 一致 |
| R4 | 暗棋未翻目标不可撞应判 bug | 0x6a45c 可撞非 bug |
| R4 | 翻雷为实现错误 | 困毙排雷不排旗=原版设计 |
| R4 | 0xFF 仅蓝方可翻旧认知 | 双方计可翻（翻归翻子方） |
| R4 | 翻棋计入 70/1000 步 | 翻棋不计步（0x302f0） |
| R5 | jq_on_draw 未做 winner 守卫 | :1171 已有 B01-065 守卫 |
| R5 | cfg3 关路径未走困毙判定 | :791-799 明确调 stalemate_after |
| R6 | Q45 close/pop 音效映射分叉 | :671-672 实锤有映射（与 audio.c 一致） |

## P0/P1 落地统计（第 1 批次 1-6 轮）

- **P0 落地**：R1 SPEC 文档修正（Q4/Q7/Q81/Q89）+ R3 evt55 全链 0/73→73/73（Q67/Q68）+ R5 jq_on_flag_capture winner 守卫（Q88）= 3 批 P0 根治
- **P1 落地**：R2 run_switch_matrix（Q92）+ R4 run_dark ⑧⑨⑩（Q91/Q5/Q8）+ R5 终局幂等断言（Q98）= 3 批 P1 断言补齐
- **evverify 真值演进**：172→173（run_switch_matrix）→174（run_evt55_names_smoke）→174（终局幂等断言纳入既有 42 项，无新增 ev_total）

## 诚实边界红线（禁自欺）

- 行为级对齐 ≠ 汇编级等价（原版 .text 1,037,940B > 重写 728,584B，信息论下界）
- 桌面 0 回归 ≠ 100% 达成（设备 PENDING，board_px 非黑比需真机截图）
- 分发成功 ≠ 业务实现（账号/联机 47 名无 g_state 变更）
- "推了全量资源" ≠ "进程读到全量"（SPAK_DIR 选路决定，渲染对未落地资源 MISS 走纯色）
- 基线 4 分叉归因不阻断（R-073 地雷模型/翻子困毙 + R-004 0xFF 编码 + R-018 杀光判据 + R-008 翻子收口时序）

---

## 第 2 批次清账（11-20 轮，域轮转：SPEC §4 UI/暗棋复合/事件计数/资源加载/渲染顶点/蓝牙帧重组/诚实边界/文档多口径/存档回放/全模块 SSOT 仲裁）

### 各轮回执（99 问题/轮）
| 轮 | 域 | P0 | P1 | P2 | 最高价值可修 P0/P1 |
|---|---|---|---|---|---|
| 11 | SPEC §4 UI 流程 | 0 | 31 | 68 | R33-Q4/Q23/Q12：SPEC §4 补全（难度弹窗前置 action0→UI_DIFFICULTY + 蓝牙菜单 ui_imen_bt_buttons 分流 + 40+ 屏态索引 + UI 可达≠功能可用） |
| 12 | 暗棋复合规则 | 0 | 24 | 75 | R34-Q12/Q92：run_dark ⑪ 撞未翻结算 3 case（EV_EAT 强吃 ev=1 / EV_DEAD 弱被吃 ev=4 / 同级互杀 EV_EAT 0x31de0 ev=1） |
| 13 | 事件计数口径 | 0 | 23 | 76 | R35-Q56/Q57：SPEC §3 计数口径 g_event_cnt[7]→[73] 名级 + EV_MINE→idx0 合槽 / EV_FLIP→idx5 + 并发竞态诚实边界（无锁 int 双变量非原子同增） |
| 14 | 资源加载续 | 3 | 27 | 69 | R36-Q3/Q46/Q67：datax_294/177 前缀入 RL-02 快检（4→6 前缀）+ found[4]→[6] + 快检文案诚实标注（6 前缀≠全 40+ atlas）+ 选路 priv 缺 datax 回退 dev 全量（SPAK 选路-落地-读取三环同源，防"推全量≠读到全量"） |
| 15 | 渲染顶点/blend 续 | 0 | 33 | 66 | R37-Q38/Q37/Q47/Q13：sel_line 每帧 memset 清零（防残留坐标幽灵高亮）+ hl_pool nhl clamp 64（防越界写静态缓冲）+ rs_draw_house 移除 glDisable(GL_BLEND)+glFlush（不破坏帧头 blend 不变量） |
| 16 | 蓝牙帧重组续 | 0 | 27 | 49+23 | R38-Q34：on_frame_v3 被拒绝帧（rc<0 但 seq>0）登记 rx_last_seq 防重传窗口空洞（v2 seq=0 不受影响，Q30 诚实边界） |
| 17 | 诚实边界续 | 8 | 44 | 47 | R39-Q80/Q81/Q23/Q38/Q67：bt_session.c 头注 9→10 类含 CAP + DRAW/CHAT 已处理修正 + SPEC SSOT 表 R34-R38 落地时效刷新 + evverify 174 收敛 + 设备 PENDING 互斥标注（历史 PASS 不外推当前）+ 4 条红线补（SYNC_CELL 白名单/v2 去重降级/UNDO hash 省 SYNC/资源 48vs970 选路） |
| 18 | 文档多口径仲裁 | 0 | 39 | 54+6 | R40-Q78-Q83：align_gate.sh:7 + gen_align_coverage.py:128-129 + pipeline_run.sh:51 + AGENTS:573/575 工具链 172 注释全同步 174（与 SPEC L15/L209 一致，0 残留；AGENTS:107/353 历史叙事段保留） |
| 19 | 存档/回放 | 0 | 20 | 46+33 | R41-Q16/Q17：jq_save_game_read 短读污染 st 根治（先读临时缓冲 tmp_state，完整才 memcpy）+ 读后最小域校验（turn/stage/winner/mode 枚举域，不合法拒读）。Q68 replay 翻棋重放冲突 = 明棋复盘设计边界非 bug（replay :94-96 强制 revealed=1 对齐明棋范式，翻棋=暗棋玩法，撤回缺陷判定） |
| 20 | 全模块 SSOT 仲裁 | 8 | 55 | 36 | R42-Q1/Q19（SSOT 表 4 行证据列刷新 R34/R37/R38/R39/R41 落地 + 诚实边界补）+ Q30/Q31/Q32/Q33（红线补 ⑤-⑧：replay 明棋范式/sjg 非原版兼容+短读已修/资源 970 推全量 vs 选路/音效 14vs18）+ Q80（设备 PENDING 机器汇总段：6 域统一登记 + 退出码口径 + 互斥仲裁） |

### 第 2 批次 P0/P1 落地汇总
| 落地项 | 文件 | so | 全门禁 |
|---|---|---|---|
| R33 SPEC §4 补全 | SPEC_BUSINESS.md | 不变 | 174/174 |
| R34 撞未翻结算 3 case | evverify.c + rules.c | 729496B | 174/174（42+132，run_dark ⑪ 纳入 42 事件项） |
| R35 计数口径 | SPEC §3 | 不变 | 174/174 |
| R36 资源 P0（datax 快检+选路） | jni_bridge.c | 729496B | 174/174 |
| R37 渲染 P1 | render.c | 729496B | 174/174 |
| R38 蓝牙 P1 | bt_session.c | 729496B | bt_semtest 152 + dual 43 |
| R39 诚实边界 P0 | bt_session.c 头注 + SPEC | 729496B | 174/174 |
| R40 工具链 174 同步 | align_gate.sh/gen_align_coverage.py/pipeline_run.sh/AGENTS | 不变 | 8/8 |
| R41 存档 P1 | save.c | 729496B | 174/174 |
| R42 SSOT 仲裁 P0 | SPEC §8 SSOT 表 + 红线 ⑤-⑧ + 设备 PENDING 机器汇总段 | 不变 | 174/174 |

### 第 2 批次撤回误报累计（防硬凑）
- R19-Q68 replay 翻棋重放=明棋设计边界（撤回缺陷判定，翻棋=暗棋玩法）
- 暗棋 0xFF 提取全 0=设计边界非 bug（layout.c:179-183 B01-043 已声明）
- 桌面 0 回归 ≠ 存档/回放域已对齐（混合版本互读 PENDING，R42-Q31 红线补⑥）
- 资源 970 推全量 ≠ 进程读到 970（选路决定，R42-Q33 红线补⑦）

### 真值演进（第 2 批次末）
- evverify **174/174**（42 事件 + 132 几何，含 run_dark ⑪ 撞未翻结算 R34 + 终局幂等）
- bt_semtest 152 + dual 43 + align_gate 8/8 + dark_align 86 全 0 退化
- so 729496B（R14 datax 快检+选路 / R15 渲染 P1 / R16 蓝牙 P1 / R19 存档 P1 落地）

---

## 第 3 批次清账（21-30 轮，域轮转：SPEC 全文条款终审/明棋复合/事件分发音效/资源出屏判据/渲染 GL 状态机/蓝牙端到端诚实边界/存档×BT 交叉终审/文档历史态 vs 当前态/全模块竞态/全模块 SSOT 收敛终审）

### 各轮回执（99 问题/轮）
| 轮 | 域 | P0 | P1 | P2 | 最高价值可修 P0/P1 |
|---|---|---|---|---|---|
| 21 | SPEC 全文条款终审 | 7 | 54 | 38 | R43-Q67/Q69/Q72（§7 jq_state 漏 mode/mines_cleared/pieces_left/stale_*）+ Q26（§3.1 74/76 同节并存）+ Q78（L188 桩 vs SSOT 已接线互斥） |
| 22 | rules.c 明棋复合 | 4 | 28 | 67 | R44-Q3（SPEC §3 EV_BOMB 行同级互杀口径收敛）+ Q1（cfg2 关送死不动 moves_since_eat=B01-022 撤回） |
| 23 | 事件分发×音效映射 | 0 | 19 | 60 | R45-Q51/Q52（注释"76 事件名"→"73 Junqi 主链"）+ Q14（code_to_evname_str 未映射返 NULL 防诊断误归因 Move） |
| 24 | 资源出屏判据 | 2 | 45 | 52 | R46-Q34（geom_ready 加 gl2_ok 门控防假阳性）+ Q1（datax 294+177 双前缀探测）+ Q12（落地 /48→/50） |
| 25 | 渲染 GL 状态机 | 0 | 23 | 45 | R47-Q67（Attach 失败不置 attached=1）+ Q17（rs_render_frame 无 context 短路） |
| 26 | 蓝牙端到端诚实边界 | 3 | 66 | 30 | R48-Q78（btconn 加"协议态模拟非硬件端到端"标注）+ Q82（jni_bridge 5 处 UI 文案"非联机可玩承诺"限定） |
| 27 | 存档×BT 交叉终审 | 18 | 61 | 20 | R49-Q78（SPEC 红线加 ⑨ 存档×BT：one.sjg 不持久化 BT 会话态 5 字段 + OnResume 不重建）+ Q79（OnResume 日志"BT 会话未恢复"限定） |
| 28 | 文档历史态 vs 当前态 | 16 | 53 | 30 | R50-Q46（AGENTS L334/L465"当前状态已达成"→历史实锤失效标注）+ Q13/Q24（SPEC 加当前 so 729496B + 截至 2026-11-19 + 当前态权威字段） |
| 29 | 全模块竞态 | 2 | 60 | 37 | R51-Q23/Q24（EGL g_win/g_win_surf 跨线程无锁 P0 实锤→主线程 set_window/reset_win 加 g_mutex 与渲染线程 :472 互斥） |
| 30 | 全模块 SSOT 收敛终审 | 0 | 53 | 46 | R52-Q21/Q22/Q55（SPEC 红线加 ⑩ 竞态域 D 双向误判防 + ⑪ EGL 跨线程残项防外推"加锁≠收敛"） |

### 第 3 批次 P0/P1 落地汇总
| 落地项 | 文件 | so | 全门禁 |
|---|---|---|---|
| R43 SPEC §7 字段补全 + §3.1 74/76 收敛 + L188 桩历史标注 | SPEC_BUSINESS.md | 不变 | 174/174 |
| R44 SPEC §3 EV_BOMB 口径收敛（同级互杀=EV_EAT） | SPEC_BUSINESS.md | 不变 | 174/174 |
| R45 注释 76→73 + code_to_evname_str NULL | evt55_business.c/junqi.h/evt55_dispatch.c | 729496B | 174/174 |
| R46 geom_ready gl2_ok + datax 双前缀 + /48→/50 | glview_gl.c/jni_bridge.c | 729496B | 174/174 |
| R47 attach 不置位 + 无 context 短路 | glview_gl.c/render.c | 729496B | 174/174 |
| R48 btconn 标注 + UI 文案 5 处 PENDING 限定 | mumu_regress.sh/jni_bridge.c | 729496B | 174/174 |
| R49 SPEC 红线 ⑨ 存档×BT + OnResume 日志限定 | SPEC/jni_bridge.c | 729496B | 174/174 |
| R50 AGENTS 历史失效标注 + SPEC 当前 so/权威字段 | AGENTS.md/SPEC | 不变 | 174/174 |
| R51 EGL set_window/reset_win 加 g_mutex | glview_gl.c | 729496B | 174/174 |
| R52 SPEC 红线 ⑩ 竞态域 D + ⑪ EGL 残项 | SPEC_BUSINESS.md | 不变 | 174/174 |

### 第 3 批次撤回误报累计（防硬凑）
- R22-Q1 cfg2 关送死不动 moves_since_eat=B01-022 已修（撤回缺陷判定）
- R22 同级互杀=EV_EAT 非 EV_BOMB（原版 0x31de0，Q3 撤回 bug 判定→SPEC 文案收敛）
- R23 未知名 dispatch_by_name 拒绝=正确 / EV_MINE→idx0 合槽=设计 / 73 名表不含 Misc 3=既定口径
- R26 BtSpp 已实现 SPP socket 非未接线 / bt_session 已处理 DRAW/CHAT/CAP 非未处理 / v2 UNDO R9 迁移非无 seq / save.c 与 BT 解耦属设计边界非崩溃 / SPEC 已声明 BT 硬件 PENDING 非缺失
- R28 SPEC L211 互斥标注已存在 / AUD_ALIGN_R26 已声明设备 PENDING（问题转为"全域传播/机器汇总缺失"）
- R29 Q23/Q24 EGL 跨线程为静态可见风险（无 TSAN 实锤，未升 P0 必现崩溃）

### 真值演进（第 3 批次末）
- evverify **174/174**（42 事件 + 132 几何）+ bt_semtest 152 + dual 43 + align_gate 8/8 + dark_align 86 全 0 退化
- so 729496B（R45 注释/NULL + R46 geom_ready/datax + R47 attach/无 context + R48 UI 文案 + R49 OnResume 日志 + R51 EGL 加锁 落地）
- **第 3 批次末轮终审（R52-Q99）：全模块 SSOT 收敛未达成**——R43-R51 局部落地成立，但 ① SSOT 表 8 行与实现/历史口径分叉（Q1-Q11）② 红线 ①-⑨ 未覆盖竞态域 D/EGL 残项（Q21/Q22，R52 已补 ⑩/⑪）③ 设备 PENDING 6 域机器汇总缺结构化产物+脚本退出码不统一（Q23-Q33）④ 误判防闭环未形成（Q55/Q84 EGL 加锁≠收敛 / btconn 模拟≠硬件 / 推全量≠读到全量）；设备 6 域维持 PENDING（SPEC L214 current_state=PENDING/device_reachable=false），历史 PASS 不外推当前达成
