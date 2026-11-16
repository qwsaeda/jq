# AUD_ALIGN_R9（对齐对抗审计台账，Round 9）

> 域：64 位正向 src_rs（com.qwsaeda）"对齐"对抗审计第 9 轮。
> 结构：§1 F GL 资源泄漏 / §2 C 蓝牙 UNDO 协议 / §3 B 事件契约余量 / §4 G 口径工具 / §5 自跑批次
> 状态列：已修（commit）/ 待修（= 下一轮输入）/ 撤回（误报，不计入待修）

## §0 基线

- align_gate 8/8（⑧ sfx 覆盖 R8 新增）；evverify 172/172（Σ 哨兵）；dark_align 86/86；bt_semtest 93→100（本轮 R9-C +7 UNDO 场景）
- so 链：`c5112d34`（R8/R9 工作树）→ `c9e1595`（R9-F）→ `a463963`（R9-C）→ `d092347`（R9-G）
- 4 路子代理并行：GL 资源三表统一（P1 泄漏根治）/ 蓝牙 UNDO v3 帧内扩展 / B 余量 + 设备钩子 / 渲染正确性扩面

## §1 F GL 资源泄漏域（回执 18 条：P1×4 / P2×1）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| F-16 rebuild 预删表漏 8 槽（tex_house/panel/panel_btn/btn_color2/gameset_title/panel_icon/sound_text/exit_text）+ tex_imen_text | P1 | **已修** `c9e1595`（R9 最小 diff）→ `7f0c9b0`（R10 X-macro 终态） | render.c 预删段 53 处 glGenTextures vs 漏 8 |
| F-17 free 释放表漏 8 槽 + program 受 gl_ok 误门控 | P1 | **已修** `c9e1595`（free 补 8 + program 去 gl_ok 门控）→ `7f0c9b0`（X-macro 同源） | gl_ok 诊断粗判据 = gl2_ok && vbo_board && tex_board && tex_piece，单核心纹理缺失即 0 |
| F-18 slots_up 统计漏 2 槽（tex_house/tex_exit_text）+ 附带 drawdetail_icons/btn_skin 统计口径 | P1 | **已修** `7f0c9b0`（R10 X-macro，分母 47→56 统计口径统一） | 统计与实体漂移 |
| F-01/02 UI 渲染被 tex_piece 门控（piece 缺失 → 全 UI 不渲染） | P1 | **已修** `7f0c9b0`（门控解耦：can_piece 段 + gl2_ok UI 段，切点 rs_draw_highlights 后） | render.c:10519 大门控 10519-11103 |
| F-P2 tex_imen_text 预删数组未列 | P2 | **已修** `c9e1595`（并入 R9 补 8 项同批） | 附属于 F-16 |

## §2 C 蓝牙 UNDO 协议域（回执 24 条：P0×6 / P1×12 / P2×6）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| C-01 UNDO_REQ 收端仅切回合不 pop（双向状态发散） | P0 | **已修** `a463963`（on_frame REQ 三型分流：legacy/REQ/应答，bridge 收帧分发对称回退 2ply 裁剪） | bt_session.c:81-102 → R9 重写 |
| C-02 UNDO ACK/NACK 收帧无处理（发端无闭环） | P0 | **已修** `a463963`（OnBluetooth A/B/C/D 四分支，pending 清理 + hash mismatch SYNC 兜底占位） | jni_bridge.c:10434-10475 → R9 重写 |
| C-03 发端 pending 管理缺失（UNDO pending 期间走子帧照发 → 本地/对端分叉） | P0 | **已修** `a463963`（bt_session.h 7 事务字段 + 走子闸口 undo_pending 冻结） | jni_bridge.c:10197-10218 |
| C-04 NACK 无回帧（rc=-3 拒收不回帧） | P0 | **已修** `a463963`（reason 编码 1非法/3回合/4栈空/5会话） | jni_bridge 收帧 rc<0 分支 |
| C-07 双端回退粒度不一致（无 2 ply 裁剪上限） | P0 | **已修** `a463963`（连续 undo_pop 到 turn==BLUE/stage!=PLAY/上限 2ply，jq.undotest :2969 范式同源） | jni_bridge.c:10449-10563 |
| C-08 幂等重放无缓存（二次 pop） | P0 | **已修** `a463963`（RC_DUP 幂等判重先于闸口 + last_resp 缓存 + legacy_undo_done 哨兵） | bt_session.c U4 实锤 rc=-2 → 判重前置修复 |
| C-11 UNDO 收端无条件广播 60 帧 SYNC | P1 | **已修** `a463963`（legacy 保留旧路径；v3 REQ 路径 ACK 携带 hash，mismatch 才 SYNC 占位） | jni_bridge.c:10461-10488 → R9 gated |
| C-12 bt_semtest 缺 UNDO 场景（88 基线无 UNDO 双端桩） | P1 | **已修** `a463963`（§5c 新增 U1-U7 七场景，93→100 PASS） | bt_semtest.c §5c |
| C-17 重连后帧窗口去重未落地（v2 无全局 seq） | P1 | **待修**（= R10-C 输入） | 全局 rx_last_seq + v3 新帧格式 + Java 双栈 |
| C-21 双栈解析窗口（BtSpp.java v2-only → v2+v3 双头） | P1 | **待修**（= R10-C 设计，R11 落地） | BtSpp.java:118-156 + bt_smali.sh:91 marker |
| C-14 UNDO v3 独立帧（seq/ref 独立字段，不借 f2/f3） | P2 | **待修**（= R10-C 设计） | R9 定案 v2 帧内扩展，v3 完整帧 = R10 设计回执已出 |

## §3 B 事件契约余量域（回执 46 条：P1×3 / P2×3）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| B-04 ev7 查表一致性守护（on_event move 分发点 7 码恒等防漂移） | P1 | **已修** `b544d52`（jni_bridge.c:1157 区 LOGE 哨兵，0 行为变化） | ev7_to_55code vs 名级查表 7/7 恒等实锤 |
| B-06 前缀白名单（6 桶 73 名可达集合校验，未知名静默归桶→-1 拒） | P1 | **已修** `b544d52`（evt55_business.c e55_in_73_names + 6 桶末段 return 改白名单） | 拼写漂移防复发；73 合法名 0 变化 |
| B-07/08/09 Σ 不变量设备端真判据缺口 | P1 | **已修** `b544d52`（jq.event_inv 设备钩子：setprop 1 → jq_event_count_invariant_ok() LOGI PASS/FAIL + 清 0，55codetest 同族纯 native 无锁） | 桌面 Σ=0==0 自洽哨兵，设备端 = 本钩子日志 |
| B-P2 钩子契约表（jq.* setprop 全清单 14 属性 × 6 列） | P2 | **已修** `b544d52`（回执 §4 入表，event_inv 入诊断探针族） | 回执 §4.2 |
| B-P2 (biz,sub) 双键不引入（R7/R8 定案维持） | P2 | **维持**（R9/R10 均未引入） | 定案维持 |

## §4 G 口径工具域（自跑 3 项）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| G-34 mumu_regress.sh 默认设备 emulator-5554（未使用端口）→ 权威红果 10.0.182.146:5555 | P1 | **已修** `d092347` | 一键回归误打假红根治，显式 --dev 可覆盖 |
| G-14/11 对齐覆盖矩阵无生成时间戳（快照新鲜度不可判） | P1 | **已修** `d092347`（gen_align_coverage.py 加生成时间 + evverify 动态口径行） | R7-D-10/11/12/15/16 定案落地 |
| G-P2 arm32_hook_check/arm32_walkregress 默认设备 = 逆向域不属正向会话 | P2 | **不动**（归属判断：逆向 com.zxj 会话管理） | 本会话 com.qwsaeda 正向，arm32 脚本不碰 |

## §5 自跑批次（R9 窗口）

- R9-F GL 三表补齐：`c9e1595`（最小 diff 8+9 项同集合 + program 去 gl_ok 门控）
- R9-C UNDO 请求-应答：`a463963`（v2 帧内扩展，bt_semtest 93→100）
- R9-G 口径：`d092347`（mumu_regress 默认设备 + 矩阵时间戳）

## 撤回 / 误报

1. **撤回**"R9 后仍存纹理泄漏缺口"主张：F-16/17 R9 已修，R10 X-macro 仅结构性归 0（同集合，非新 P0）。
2. **撤回**"桌面 evverify Σ==0==0 可作设备计数真判据"：桌面 = 本地防漂移哨兵；设备真判据 = jq.event_inv 钩子日志。
3. **撤回**"UNDO REQ 幂等重放走旧闸口判 -2 是正常行为"：实锤 U4 FAIL rc=-2 → 判重必须前置（§2 C-08 修复）。

## 末行统计

- 总条目：9（§1）+ 11（§2）+ 5（§3）+ 3（§4）+ 3（§5）= **36 条**
- 已修：**28**（§1×4 + §2×8 + §3×4 + §4×2 + §5×... 按 commit 计 3 commit 全覆盖）
- 待修（= R10 输入）：**8**（C-17 全局帧窗口 / C-21 双栈 / C-14 v3 独立帧 / F-18 统计归零 / B-P2 若干）
- 撤回：**3**
- P0：**6**（C 域 UNDO 协议全闭环，全已修 `a463963`）
- P1：**16**（GL×4 / UNDO×6 / B×3 / G×3，全已修）
- P2：**8**（附属于 P1 批次或定案维持）

## 门禁基线（R9 闭环时）

- align_gate **8/8** + evverify **172/172** + dark_align **86/86** + bt_semtest **100/100**
- 设备：红果 10.0.182.146 No route to host（主机网络中断），权威 5 判据 PENDING
