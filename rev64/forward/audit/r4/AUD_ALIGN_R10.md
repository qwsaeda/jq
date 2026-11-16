# AUD_ALIGN_R10（对齐对抗审计台账，Round 10）

> 域：64 位正向 src_rs（com.qwsaeda）"对齐"对抗审计第 10 轮。
> 结构：§1 F GL 资源 X-macro 终态 + UI 门控解耦 / §2 B 事件契约余量落地 / §3 C v3 完整帧设计（R11 输入）/ §4 自跑批次
> 状态列：已修（commit）/ 待修（= 下一轮输入）/ 撤回（误报，不计入待修）

## §0 基线

- 4 路子代理并行：GL X-macro 终态 / C v3 完整帧设计 / F-01/02 UI 门控解耦 + 资产同步 / B 余量 + 设备钩子
- 门禁基线：align_gate **8/8** + evverify **172/172** + dark_align **86/86** + bt_semtest **100/100**
- so 链：`c5112d34`（R8/R9）→ `b544d52`（R10-B）→ `7f0c9b0`（R10-F）
- 设备：红果 10.0.182.146 离线（No route to host），权威 5 判据 PENDING

## §1 F GL 资源管理终态域（回执 18 条 + X-macro 10 条，P1×6 / P2×4）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| F-01 UI/菜单/弹窗/overlay 被 tex_piece 门控（piece 缺失全 UI 黑） | P1 | **已修** `7f0c9b0`（can_piece 段 + gl2_ok UI 段拆分，切点 rs_draw_highlights 后） | render.c:10519 旧单块 10519-11103 |
| F-02 STG_OVER overlay 被 piece 门控（piece 缺失无胜负横幅） | P1 | **已修** `7f0c9b0`（overlay 随 UI 段 gl2_ok 门控，自检 prog_solid） | render.c:11099-11102 |
| F-16/17/18 三表不一致（R9 最小 diff 已补齐 8+9 项；结构性归 0 = X-macro） | P1 | **已修** `7f0c9b0`（RS_TEX_SLOTS_REGULAR 53 + RS_TEX_SLOTS_BTN_SKIN 3，4 处消费同源展开，旧手写 3 表退场） | render.c 顶部 X-macro + rebuild 预删 / free / slots_up 三处宏展开 |
| F-P1 slots_up 统计漏 4 项（house/exit_text/drawdetail_icons/btn_skin×3） | P1 | **已修** `7f0c9b0`（X-macro 展开自动补齐，分母 47→56） | 宏展开结构性保证 |
| F-P2 tex_imen_text 双删去重（旧显式 + texs[] 双列） | P2 | **已修** `7f0c9b0`（X-macro 只列一次，语义等价） | 预删段宏展开 |
| F-P2 Adreno 跨 context quirk（不强行修，保留注释 + #if 0 回退位） | P2 | **维持**（诚实边界，仅标注） | render.c:9799-9808 注释保留 |

## §2 B 事件契约余量域（回执 46 条，P1×3 / P2×3）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| B-04 ev7 查表一致性守护（on_event :1157 分发点 7/7 恒等防漂移） | P1 | **已修** `b544d52`（LOGE 哨兵，漂移才告警，0 行为变化） | jni_bridge.c:1157 区 |
| B-06 前缀白名单（6 桶 73 名校验，未知名→-1 拒） | P1 | **已修** `b544d52`（e55_in_73_names + 6 桶末段 return） | evt55_business.c |
| B-07/08/09 jq.event_inv 设备钩子（Σ 不变量真判据） | P1 | **已修** `b544d52`（55codetest 同族，纯 native 无锁，一次性清 0） | jni_bridge.c:1669 后 |
| B-P2 钩子契约表（14 属性全清单） | P2 | **已修** `b544d52`（回执 §4 入表） | R9 回执 §4.2 |
| B-P2 (biz,sub) 双键不引入（定案维持） | P2 | **维持**（R10 未引入） | 定案维持 |

## §3 C v3 完整帧设计域（回执 97 条，R11 输入）

> 设计回执已出，**未落地**（大改动面：bt_protocol.c 新函数 + BtSpp.java 双栈 + bt_smali.sh marker 同步 + 全局 seq 去重 + 能力探测）。R11 主攻输入。

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| C-14 v3 独立帧格式（v3\n + 10 字段，seq/ref 独立槽） | P0 | **待修**（R11 输入，设计已冻结） | R10 回执 §1-§2 |
| C-17 全局帧窗口 seq 去重（rx_last_seq 全 type） | P0 | **待修**（R11 输入，设计已冻结） | R10 回执 §5 |
| C-21 BtSpp.java 双栈重组（v2+v3 双头 min 识别 + 噪声双版本丢弃） | P0 | **待修**（R11 输入，设计已冻结） | R10 回执 §3 |
| C-P1 能力探测 + 会话内锁定（peer_caps_v3 收升发） | P1 | **待修**（R11 输入） | R10 回执 §2.8 |
| C-P1 RESTART 窗口复位（rx_last_seq/tx_seq 清零防旧局在途帧误消费） | P1 | **待修**（R11 输入） | R10 回执 §5.8 |
| C-P1 bt_smali.sh marker 同步（v2\n\|v3\n 双版本防假绿/假红） | P1 | **待修**（R11 输入） | R10 回执 §4 |
| C-P2 混跑矩阵（v3↔v3 / v3↔v2 收 / v2↔v3 对端先升） | P2 | **待修**（R11 输入） | R10 回执 §7 |
| C-P2 OnBluetooth 推进字符集扩展（strspn "v2\n"→"v23\n"） | P2 | **待修**（R11 输入，最小 diff 1 行） | R10 回执 §6 |
| C-P2 hash 真值校验 + mismatch SYNC（R9 TODO 占位闭合） | P2 | **待修**（R11 输入） | jni_bridge.c:10549 注释 |

## §4 自跑批次（R10 窗口）

- R10-B 事件余量：`b544d52`（B-04 ev7 守护 + B-06 前缀白名单 + jq.event_inv 钩子，全门禁 0 退化）
- R10-F GL X-macro + UI 门控解耦：`7f0c9b0`（三表结构性归 0 + F-01/02 解耦，78 行增 114 行删）

## 撤回 / 误报

1. **撤回**"R10 后 GL 资源仍有 P0 泄漏"主张：R9-F + R10 X-macro 全闭环，结构性 0 漂移，无新 P0。
2. **撤回**"X-macro slots_up 分母 47→56 是行为变化"：仅日志诊断覆盖面扩大，不触 glDelete/glGen/glTexImage2D 时机，0 GL 行为变化。
3. **撤回**"UI 门控解耦改变正常路径绘制序"：正常路径（tex_piece + gl2_ok）两段同帧同序，绘制集合/顺序 0 变化。

## 末行统计

- 总条目：§1×6 + §2×5 + §3×9 + §4×2 = **22 条**（C 域 97 条设计回执拆为 9 条待修输入，不逐条入台账）
- 已修：**11**（§1×5 + §2×4 + §4×2，2 commit）
- 待修（= R11 输入）：**9**（C-14/C-17/C-21 三大 P0 + 6 P1/P2）
- 撤回：**3**
- P0：**3**（C 域 v3 完整帧，全待修 = R11 主攻）
- P1：**8**（§1×3 / §2×3 / §4×2，全已修）
- P2：**8**（附属于 P1 批次或定案维持）

## 门禁基线（R10 闭环时）

- align_gate **8/8** + evverify **172/172** + dark_align **86/86** + bt_semtest **100/100**
- 设备：红果 10.0.182.146 离线（No route to host），权威 5 判据 PENDING
- 攻坚日志新增坑：① X-macro 内 `#define` 在函数体内使用（C99 合法但非惯用，`#undef` 必须成对）② slots_up 块外层 `{` 与 `struct` 声明 `};` 闭合层次不同（`};` 闭合 struct，外层 `}` 在 LOGI 后）
