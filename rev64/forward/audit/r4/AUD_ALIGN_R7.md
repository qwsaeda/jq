# AUD_ALIGN_R7（对齐对抗审计 R7，2026-11-16）

> 承接 R6 待修队列（A 12 / B 27 / C 23 / D 33 + 新发现）；4 路子代理并行（A 18 / B 20 / C 23 / D 35）
> + 会话自跑主攻 ①④（D 口径批 + A 方案 A 落地）。状态列：已修 / 已定案 / 撤回 / 待修(R8) / 维持。
> 门禁基线（R7 落地后全绿）：align_gate 7/7 + dark_align 86/86 + evverify 172/172 +
> bt_semtest 88/88（67 静态 CHECK + 21 循环展开：§3 ok[7]×2+bad[6]...实锤 88 ok 行）+
> benchcheck 双源 3 步 + 0 崩溃。

## §0 本轮关键定案（先读）

1. **pieces_left 方案 A 定案（R7-A-00，已落地 b0a96a1）**：可动子数缓存，单一真值源 =
   `jq_refresh_movable_counts`（stalemate_after 全量刷新）；全仓 13 处增量扣减清零
   （grep `pieces_left[.*][+-]+` 归零）；0xFF 域外不扣、旗雷排除域不扣、终局快照冻结；
   撤回 B01-008/009/057部分/058/085/086/095/R-054增量记账。
2. **bt_semtest 基线真值 = 88 运行时检查**（67 静态 `CHECK(` 调用 + 循环展开 21）——
   R7-C 子代理"67"口径系 grep-only，运行实锤 88；台账以 88 为准。
3. **bt_semtest_dual.c 12 用例设计（R7-C，10 纯 C 桌面可测 + D12 需真机 BT）**——
   v3 协议落地批，R8 主攻。
4. **事件单一入口设计（R7-B，R8 主攻）**：`jq_evt55_dispatch_by_name` + 名级 code 覆盖表
   （DrawAgree→26/DrawDisagree→27，消除 handler 诊断漂移）+ Σ==total 不变量 +
   sfx_coverage_check.py 作为 align_gate 第 ⑧ 项。
5. **R7-D 口径批已落地（8e04770 + 本批）**：gen_align_coverage 正则校准 / benchcheck 双源 /
   SPEC 74/76 定案 / 默认设备统一 / AGENTS 路由补登 6 项 / WAV 两口径分标 / 矩阵真值 172。

## §A pieces_left 语义族（R7-A-01..18，18 条）

| ID | 条目 | 基线证据 | 状态 |
|---|---|---|---|
| A-01 | R6-A-01 挖雷工兵存活 vs `me--`（B01-008 矛盾） | rules.c:839 已删 | **已修（撤回 B01-008）** |
| A-02 | R6-A-02 挖雷扣雷方 vs refresh 排旗雷 | rules.c:817/824 已删 | **已修** |
| A-03 | R6-A-03 EV_BOMB 同归扣 dst 含旗雷 + 撞旗终局分叉 | rules.c:711/715/721 已删 | **已修** |
| A-04 | R6-A-04 cfg-on/off 吃旗路径分裂（off 不扣） | 增量全删后分叉消失 | **已修（收口级）** |
| A-05 | R6-A-05 进大本营冻结子未同步 | 方案 A 下=刷新时机问题，不引入增量 | **已定案（终局冻结覆盖）** |
| A-06 | R6-A-13 flag-off 移除旗不扣 | 同 A-04 根因 | **已修** |
| A-07 | R6-A-14 0xFF 移除归 BLUE vs refresh 从未入集 | rules.c:721/817/863 已删 | **已修（B01-009 撤回）** |
| A-08 | R6-A-15 挖雷三动作内生会计矛盾 | :817/824/839 全删 | **已修** |
| A-09 | R6-A-16 增量不区分是否曾入集 | 增量全删后天然闭合 | **已修** |
| A-10 | R6-A-19 终局后 flag_capture 仍扣 | rules.c:1163 已删 + 冻结定案 | **已修（B01-026 隐患消除）** |
| A-11 | R6-A-20 0xFF 同归扣 BLUE 分叉 | 同 A-07 | **已修** |
| A-12 | R6-A-21 可动性事实源分裂（3 套） | pieces_left=展示缓存/stale_v/stale_c=困毙加速/实时扫描=权威 | **已定案（注释三处声明）** |
| A-13 | R6-A-22 挖雷无单一定义可对应 | 方案 A 放弃增量记账 | **已修** |
| A-14 | R6-A-23 "工兵留"vs"工兵消失"注释自相矛盾 | B01-008 注释体撤回 | **已修** |
| A-15 | 同级互杀 0xFF 分叉（B01-086 vs B01-009） | rules.c:884/885 已删 | **已修** |
| A-16 | save/replay/bt/undo 契约漂移 | save.c 钳位维持 / replay.c 基底 0 注释对齐 / undo 整结构恢复（含缓存，刷新重建） | **已定案（replay.h 注释已修）** |
| A-17 | junqi.h/rules.c AUD-R-014 过时注释 | :148-151/:447 已回写 | **已修** |
| A-18 | replay.h:29 与 replay.c 实现不符 | 注释已对齐（基底 0 + 刷新重建） | **已修** |

## §B 事件契约族（R7-B-01..20，20 条；B 单一入口批 = R8 主攻）

| ID | 条目 | 基线证据 | 状态 |
|---|---|---|---|
| B-01 | 单一入口 `jq_evt55_dispatch_by_name` 设计 | 查表+evname 精确计数 0 变化约束 | **已设计（R8 落地）** |
| B-02 | on_event 6 处硬编码 55code 替换清单 | jni_bridge.c:1162/1176/1197/1207/1215/1250 | **待修(R8)**（DrawAgree/Disagree 需名级 code 覆盖表） |
| B-03 | 55codetest 11 处硬编码 | jni_bridge.c:1639-1649 | **待修(R8)**（与 B-02 同批） |
| B-04 | ev7_to_55code 查表一致性 assert | jni_bridge.c:1087-1144 | **待修(R8)**（断言批，0 行为变化） |
| B-05 | `_checked` 未知名错误语义（NULL/非73名 不伪造 MOVE） | evt55_business.c:27/73/98-102 | **待修(R8)** |
| B-06 | 前缀 strncmp 无 73 白名单 | evt55_business.c:52-72 | **待修(R8)**（与 B-05 同批） |
| B-07 | Σ g_event_cnt[i] == g_event_total 不变量 | 双写点 :130-133 + 手工散点 | **待修(R8)**（evverify end-of-run 断言 + jq.event_inv 钩子设计） |
| B-08 | 手工查名计数旁路 4+1 处收敛 | :1181-1190/1223-1240/1265-1274 | **待修(R8)**（count-only API，"无独立55code"诚实边界维持） |
| B-09 | 7 码直写计数 8 处收敛 | :2391/2425/2864/3566/9981/10154/10176/10247 | **待修(R8)**（逐链审计防双计） |
| B-10 | Resign/ResignConfirm 子态塌缩定案 | 名级计数精确（idx18/19）+ 55code 代表 24 保持 | **已定案（不引入 (biz,sub) 双键，R8+ 需 32 位证据再开）** |
| B-11 | 悔棋 6 名子态塌缩定案 | 代表 42→idx20，名级精确依赖 evname | **已定案（同 B-10）** |
| B-12 | DRAW 族子态码 25/26/27 并存 + by_name 名级 code 覆写 | evt55_dispatch.c:56-58 vs name→biz 代表 25 | **已定案（覆写表设计，R8 落地）** |
| B-13 | Over 双桶（json code3+code11 均含 idx12） | 单桶 code3 定案 | **已修（8e04770 前批：json code11 移除 + C 表 {71,72,-1} 同步 + 0 缺口）** |
| B-14 | 73 名→音效映射 60/73 兜底 move 显式化 | evt55_sfx.c:13-31 | **已定案（白名单+门禁，R8）** |
| B-15 | sfx_for_move vs sfx_wav 双路径无覆盖门禁 | jni_bridge.c:571-629/940-980 | **待修(R8)**（B-16 三层矩阵覆盖） |
| B-16 | sfx_coverage_check.py 门禁（align_gate ⑧）设计 | 骨架+判据+插入位已给 | **已设计（R8 落地为 ⑧ 项）** |
| B-17 | evt55_dispatch "计数恒 0"注释矛盾 | 防御计数定案 | **已修（本会话批，evt55_dispatch.c:126-133 注释回写）** |
| B-18 | "g_event_cnt 是 static"过时注释 2 处 | :14/:102 | **已修（本会话批，2 处注释回写）** |
| B-19 | 回归风险面 0 变化约束（计数/音效/日志三域） | dispatch evname 精确命中同 idx | **已定案（R8 落地门禁清单 7 项）** |
| B-20 | align_gate ⑧ 项纳入设计 | run_gate 插入位（⑦ 后） | **已设计（R8）** |

## §C 蓝牙 v3 族（R7-C-01..23，23 条；v3 协议批 = R8 主攻）

| ID | 条目 | 基线证据 | 状态 |
|---|---|---|---|
| C-01 | UNDO 方向错位（发端不 pop / 收端单边 pop） | jni_bridge.c:1171-1178/9914-9922/10434-10475 | **已定案（v3 请求-应答+双端对称回退，R8 落地）** |
| C-02 | UNDO 无 ACK/NACK/应答态 | bt_protocol.h:16 + bt_session.c:81-102 | **已定案（v3 UNDO_ACK/NACK，R8）** |
| C-03 | UNDO 幂等缺失（seq/token） | 帧循环 :10420-10490 无去重键 | **已定案（v3 seq+last_req_seq，R8）** |
| C-04 | 栈空拒收无 NACK 回帧 | bt_session.c:96-101 rc=-3 | **已定案（NACK reason=4，R8）** |
| C-05 | UNDO 后 SYNC 锁外读 g_state 竞态窗口 | :10460-10475 A2-015 诚实标注 | **已定案（v3 原子点→快照→编码顺序，R8）** |
| C-06 | bt_semtest UNDO 桩与真实现脱节 | :327-351 §5 断言 | **待修(R8)**（v3 后 §5 全重写） |
| C-07 | v3 UNDO 目标语义（REQ/ACK/NACK 全表） | 状态机迁移表已给 | **已设计（R8）** |
| C-08 | 双端回退粒度（1 完整回合 2 ply 裁剪） | undo_pop 到 BLUE 范式 :2969/9907 | **已设计（R8）** |
| C-09 | 幂等 token 设计 | seq uint32 + 重发上次 ACK/NACK | **已设计（R8）** |
| C-10 | UNDO 状态机迁移表 | 7 迁移全表 | **已设计（R8）** |
| C-11 | UNDO 与 SYNC 收敛关系（hash_after 兜底） | 避免每次 60 帧放大 | **已设计（R8）** |
| C-12 | NACK 原因码表 8 项 | BAD_COORD/ILLEGAL/TURN/EMPTY_UNDO/SESSION/HASH/MODE/UNSUPPORTED | **已设计（R8）** |
| C-13 | NACK 收端 UI/日志联动 | jq_show_dialog 复用 + LOG 带 ref/reason | **已设计（R8）** |
| C-14 | RESTART 无 mode/阵营字段（一明一暗分叉） | bt_session.c:153-183 + :170-171 自承 | **已定案（方案 A 帧内扩字段 mode+host_is_blue，R8）** |
| C-15 | RESTART 扩字段方案 A/B 对比 | 帧内 vs SYNC_META 前置 | **已定案（推荐 A 单帧原子，R8）** |
| C-16 | RESTART 断连语义（终态拒开局） | :159 rc=-2 | **已定案（重连后需重建会话，R8）** |
| C-17 | 原子复位集 6 类（对局态/悔棋栈/会话/UI/音效/帧窗口） | 全仓 17 处 broadcast 调用点清单 | **已定案（bt_restart_reset_all 统一序列，R8）** |
| C-18 | 复位原子性与锁序（g_mutex/g_ui_lock） | :368/1062 单向锁序 | **已定案（同一临界序列，R8）** |
| C-19 | RESIGN 收端未走统一终局入口 | bt_session.c:184-196 vs 本地 :1160-1170 | **已定案（bt_apply_terminal 统一，R8）** |
| C-20 | RESIGN 统一终局入口设计 | stage/winner/evt55/计数/sfx/UI 同序 | **已设计（R8）** |
| C-21 | v3 破坏面（C/Java/OnBluetooth 三端同步清单） | :21 全链 6 点 | **已定案（双栈解析窗口，R8）** |
| C-22 | DRAW/CHAT 交互未定义（v3 扩展约束） | :197-212 最小消费 | **已定案（R8 协议域一致性项）** |
| C-23 | SPEC §6 与实现漂移（7 类→9 类） | SPEC:145-154 | **待修(R8)**（v3 落地同步 §6） |

## §D 口径/文档族（R7-D-01..35，35 条；本会话自跑已修 14 条）

| ID | 条目 | 状态 |
|---|---|---|
| D-01 | PIPELINE.md:91 169/169+39+130 → 172/172+40+132 | **已修（8e04770）** |
| D-02 | pipeline_run.sh:51 169/169 硬编码 | **已修（→172/172 动态真值文案）** |
| D-03 | BUSINESS_RISK_LEDGER:20 无时间锚点 | **已修（2026-11-12 快照标注）** |
| D-04 | BUSINESS_RISK_LEDGER:62 同上 | **已修** |
| D-05..07 | AGENTS:353/409/437 裸数值 | **已修（（当时口径）前缀批）** |
| D-08 | align_gate:8 注释硬编码 40/40+132/132 | **已修（SUMMARY 动态真值文案）** |
| D-09 | align_gate:44 门禁名 "evverify 172" 硬编码 | **已修（"evverify（SUMMARY 动态真值）"）** |
| D-10 | gen_align_coverage 矩阵无生成时间戳 | **已定案（R8：main() 头写时间戳 + evverify 实测口径行）** |
| D-11 | align_coverage_matrix.md 无生成上下文 | **已定案（同 D-10）** |
| D-12 | 门禁缺矩阵新鲜度约束 | **已定案（R8：align_gate 可选 ⑧' 再生+diff 判据）** |
| D-13 | gen_align_coverage evverify 统计正则命中 0 | **已修（ev_total++/geo_total++ = 172，8e04770）** |
| D-14 | 矩阵 evverify 行失真 | **已修（随 D-13，矩阵现 172）** |
| D-15 | 矩阵无时间戳 | **已定案（同 D-10/11）** |
| D-16 | 矩阵未入门禁 | **已定案（同 D-12）** |
| D-17 | benchcheck 双源无同步断言 | **已修（bench_c_extract.py + Makefile 第三步 + 负向验证 FAIL 实锤）** |
| D-18 | 同 D-17 | **已修（合并）** |
| D-19 | rules_fork_check.py:16 167/167+50/50 过期 | **已修（→172/172+36 case）** |
| D-20 | rules_fork_check.py:257 同口径再硬编码 | **已修（2 处全替）** |
| D-21 | BUSINESS_RISK_LEDGER:6 探针 19/20 | **已修（36 case 基线 4 分叉文案）** |
| D-22 | BUSINESS_RISK_LEDGER:26/59 19/20 | **已修** |
| D-23 | EXPERIENCE_LOG 历史计数缺快照声明 | **已修（文首声明 + 4 处行内标注，余 2130/2174 由声明覆盖）** |
| D-24 | SPEC 74/76 矛盾 | **已修（76=73 Junqi+3 Misc 单一锚点段 + 顶部口径总表 7 行）** |
| D-25 | SPEC :83-86 混写 | **已修（同上）** |
| D-26 | AGENTS:108 "18 WAV" vs 仓内 14 | **已修（两口径分标：SPAK 解包 18 / spak_res 实物 14/970）** |
| D-27 | 默认设备漂移（smoke 127.0.0.1:5556 vs 10.0.182.146） | **已修（mumu_align_smoke 默认改权威红果 + 注释）** |
| D-28 | AGENTS 路由漏登 | **已修（补登 6 项：align_gate/event_names/gen_align_coverage/mumu_smoke/rule_probe/AUD_ALIGN 族）** |
| D-29 | 台账-日志交叉映射缺失 | **已定案（R8：REVERSE_LOG §AUD 索引表 + 台账头部回链）** |
| D-30 | gen_dialog_batch.py:700 注释 170/170 过期 | **已修（当时口径标注）** |
| D-31 | UI_COMPARE.md:536 170/170 无快照 | **已修（历史快照标注）** |
| D-32 | BUSINESS_RISK_LEDGER:59 重复 19/20 | **已修（随 D-22 2 处全替）** |
| D-33 | 事件名/资源多口径缺一处总表 | **已修（SPEC 顶部 7 行口径总表）** |
| D-34 | 矩阵再生现状更正（R6-D-13/14 未复现撤回） | **撤回（8e04770 正则修后矩阵 172 一致，残留 D-15/16 时间戳/门禁项）** |
| D-35 | rules_fork_check 静态基线单一事实源 | **已修（随 D-19/20 → evverify 172/172 + 36 case 引用）** |

## §E 新发现（R7 追加，5 条）

| ID | 条目 | 状态 |
|---|---|---|
| E-01 | 全仓 `domain_behavior_align_check.py` 19/20 漏网（子代理未列） | **已修（36 case 基线文案 2 处）** |
| E-02 | check_consistency.sh:40 注释示例 170/170 过期 | **已修（N/N 泛化）** |
| E-03 | EV_EAT 分支 R7-A 删扣减后 if/else 悬挂语法错误 | **已修（结构合并 + gcc -fsyntax-only 实锤）** |
| E-04 | 挖雷分支 `dst_side` 死变量告警（R7-A 删扣减后无消费者） | **已修（变量+旧注释清除，make 0 告警实锤）** |
| E-05 | bt_semtest "67" vs "88" 口径漂移（子代理 grep-only vs 运行时） | **已定案（88 运行时真值入台账口径，67=静态 CHECK 调用点）** |

## §撤回/误报（R7 汇总）

| 项 | 撤回理由 |
|---|---|
| B01-008（挖雷工兵"消失"扣减） | 盘面事实 = 工兵存活移位（`*dst=*src`），扣减与 refresh 域双矛盾 |
| B01-009（0xFF 归蓝池） | refresh 从未入集 0xFF + 同级互杀 `1-me` 扣 RED 分叉 + "未定色"非真实归属 |
| B01-057（0xFF 落点部分） | 已定色 `dst->side` 方向保留精神；0xFF 归蓝池落点随 B01-009 撤回 |
| B01-058/085/086/095 增量记账族 | 方案 A 下全退场（刷新单一真值源） |
| R-054 增量记账（flag_capture `--`） | 守卫（SIDE_VALID 防脏 winner）保留；扣减撤回 + 终局冻结 |
| R-014 "刷新恒 1/未接线" 描述 | layout.c 已实装真累加；核心"规则不读字段/勿判胜"维持 |
| R7-C "基线 67 条" | 运行时实锤 88（67 静态 + 21 循环展开），台账以 88 为准 |
| R6-D-13/14（矩阵 evverify 命中 0） | 8e04770 正则修后未复现，撤回（残留 D-15/16 防回退项） |

## 末行统计

- 条目：A 18 + B 20 + C 23 + D 35 + E 5 = **101**（台账口径 100 条整 + E-05 口径定案并入 §E）
- 本会话已修：**31 条**（A 族 18 全 + D 族 14 + E 3，B-13/B-17/B-18 3 条）
- 已定案/设计（R8 落地输入）：B 14 + C 21 + D 7（D-10/11/12/15/16/29 + 矩阵时间戳族）
- 撤回/误报：**8 条**
- 门禁基线（全绿）：align_gate 7/7 + dark_align 86/86 + evverify 172/172 + bt_semtest 88/88 + benchcheck 双源
- so 哈希链：`72922a62`(R5) → `d9d1c325`(R6) → R7 工作树构建（b0a96a1，未重打包）
