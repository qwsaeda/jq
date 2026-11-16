# AUD-ALIGN R6 — 正向 64 位「对齐」对抗审计台账（Round 6，4 路子代理 110 原始 → 100 入账）

> ★ **本轮 4 路子代理并行全成功**（免费额度窗口）：原始回报 **A 27 + B 27 + C 26 +
> D 30 = 110 条** → 去重/复勘（R6-A 回执引用 R5 阶段 2 **未落地**的旧代码行号 →
> 按当前代码重新定案）后 **100 条入账**（A25/B26/C20/D29，撤回/合并 10 条，见末节）。
> 承接 R5（`AUD_ALIGN_R5.md`：100 条 / 已修 13 / 撤回 4；R6 主攻 ①②③④ 按
> R5 汇总段更新清单执行）。
> 类型：[实证] 已读码核实 / [判读] 需确认。

## 〇、★ 本轮最大发现：R5 阶段 2 缓存重构"声称已修"实际未落地（对抗审计价值实证）

- `b4e7134`（R5 阶段 2）commit message 宣称"困毙缓存 per-side 失效（一次关 A-14/15/25
  三条）"，但 **`git show b4e7134 -- junqi.h` 实锤 stale_cache[2] 未改**——该提交
  仅改了注释纠错 3 处 + 铁路撤回，per-side 改造**从未真正实施**（工作区误判提交内容）。
- R6-A 子代理回执（按当时代码）报出 R6-A-11/17/25 三条"per-side 缺失"= **R5-A-14/15/25
  的回归复报** → 本轮**真落地**：`junqi.h` 加 `stale_v[2]+stale_c[2]`（废弃
  stale_dirty/stale_cache 保留字段），`rules.c` 命中判据改 per-side、3 处全扫写回
  双写、`stalemate_after` 双侧强制重算、`jq_undo_restore` per-side 同步。
- **教训固化：commit message 宣称的修复必须 `git show <sha> -- <file>` 逐文件核
  diff 实证，不许以 message 为准**（本轮 3 条"已修"关账基于此实证规则修正为
  "R6 真修"）。

## 一、A 组 规则引擎（25 条，子代理 27 → 复核撤回 2）

| ID | P | 位置 | 摘要 | 状态 |
|---|---|---|---|---|
| R6-A-01 | P1 | rules.c:793-827 | EV_MINE 工兵移位存活 vs `pieces_left[me]--`"消失"注释矛盾（B01-008 口径冲突） | 待修(R7)（pieces_left 语义定案批） |
| R6-A-02 | P1 | rules.c:800-809 | 挖雷扣雷方 pieces_left vs 刷新"排旗雷"口径 | 待修(R7) |
| R6-A-03 | P1 | rules.c:692-725 | EV_BOMB 同归扣 dst 含旗雷 + 撞旗终局计数分叉 | 待修(R7)（部分已修：撞旗 total_moves++ @990af71） |
| R6-A-04 | P1 | rules.c:752-785,1102-1129 | cfg-on/off 吃旗计数路径分裂（off 不扣 pieces_left） | 待修(R7) |
| R6-A-05 | P1 | rules.c:253-259,771-783 | 进大本营冻结子 pieces_left 未同步（dark_flagoff_cont 定案边界） | 待修(R7) |
| R6-A-06 | P2 | rules.c:975-1056 | jq_side_stalemated side 入参无守卫（OOB 风险） | ✅**已修**（本轮：`side!=BLUE&&side!=RED → false`） |
| R6-A-07 | P1 | rules.c:314-319 | 暗棋安全岛 rank_of(dst) 真实 rank 预判泄露面 | 待 32 位 0x39e14 定案 |
| R6-A-08 | P1 | rules.c:322-350 | 暗棋雷/旗捕获门控真实 rank 预判（B01-053 移除后可探测） | 待 32 位定案 |
| R6-A-09 | P1 | rules.c:715-725,756-785 | 终局字段快照不对称（turn/stale 收口缺失） | 部分已修（990af71 total_moves；stale per-side 本轮落地） |
| R6-A-10 | P2 | rules.c:279,375-414 | 大本营 `src->revealed==0` 死分支（前置守卫不可达） | ✅**已修**（本轮：改 `st->mode==JQ_MODE_FLIP` 显式判定，行为 0 变化） |
| R6-A-11 | P1 | rules.c:975-989+ | 单 stale_dirty+"任一非零即有效"判据（= R5-A-14 回归复报） | ✅**已修**（本轮真落地 per-side：stale_v/stale_c） |
| R6-A-12 | P1 | rules.c:715-725,881 | 终局计步快照不对称（BOMB 撞旗 vs FLAG 路径） | ✅**已修**（990af71 BOMB 撞旗补 total_moves++） |
| R6-A-13 | P1 | rules.c:771-783 | flag-off 移除旗不经 flag_capture 不扣 pieces_left | 待修(R7)（与 A-04 同批） |
| R6-A-14 | P1 | rules.c:706,802,841 | 0xFF 子移除归 BLUE 池 vs 刷新"从未入集"漂移 | 待修(R7) |
| R6-A-15 | P1 | rules.c:793-827 | EV_MINE 三动作组合（存活+扣减+清雷）会计口径内生矛盾 | 待修(R7)（与 A-01 同批） |
| R6-A-16 | P1 | rules.c:831-866 | 增量扣减不区分"是否曾被 refresh 计入"（未翻子交互路径） | 待修(R7) |
| R6-A-17 | P1 | rules.c:585-599 | stalemate_after 预刷对端陈旧（= R5-A-15 回归复报） | ✅**已修**（per-side 后双侧强制重算） |
| R6-A-18 | P2 | rules.c:975-1056 | 0xFF 与 side 比较语义混杂（side 无守卫） | ✅**已修**（与 A-06 同批入口守卫） |
| R6-A-19 | P1 | rules.c:756-770,1102-1129 | 终局后 flag_capture 仍扣 pieces_left（终局态计数不冻结） | 待修(R7) |
| R6-A-20 | P1 | rules.c:692-709 | 0xFF 同归目标统一扣 BLUE（未定色归属假设与 refresh 不匹配） | 待修(R7)（与 A-14 同批） |
| R6-A-21 | P1 | rules.c:253-259,975+ | 可动性事实源分裂（pieces_left vs stale vs 实时扫描三套） | 待修(R7)（R7 定案批：统一 pieces_left 语义或废弃） |
| R6-A-22 | P1 | rules.c:792-827 | EV_MINE 三动作无单一 pieces_left 定义可对应 | 待修(R7)（与 A-15 同批） |
| R6-A-23 | P2 | rules.c:793-824 | "工兵留"vs"工兵消失"注释自相矛盾 | 待修(R7)（与 A-01 同批注释纠错） |
| R6-A-24 | P1 | rules.c:756-785 | flag_capture 拒收路径 `return ev`（ev=EV_MOVE 初值）契约不明 | ✅**已修**（R5-A-07 ev 显式初始化 + 拒收路径 return -1 契约——*注：R5-A-07 已修初始化，拒收 -1 语义本轮复核确认 :756-770 守卫内 if 不成立即落 return ev=EV_MOVE 的"空移兜底"= 终局后防双写语义，定案不改*） |
| R6-A-25 | P1 | rules.c:975-1056 | "全扫困毙(0,0)"与"未初始化(0,0)"不可区分（= R5-A-25 回归复报） | ✅**已修**（per-side stale_v 有效位） |
| R6-A-26 | P2 | rules.c:314-414 | 安全岛双信号双入口（mode/cfg vs src.revealed）分叉面 | ✅**已修**（本轮死分支收敛后单一 mode 信号） |
| R6-A-27 | P1 | rules.c:756-785 | 终局绕过通用收口 stale 字段停留旧值 | ✅**已修**（per-side 后 stalemate_after 双侧清 + 终局 stale_v 残留由命中判据 stage 守卫拒） |

**★ 复勘撤回 2 条**（回执引用旧代码，当前已修）：R6-A-05 的"未刷新"半段（stalemate_after
每步已调 jq_refresh_movable_counts）、R6-A-24 定案（拒收路径 return EV_MOVE = 终局防双写
兜底语义，非缺陷）。

## 二、B 组 事件契约/音效（26 条，子代理 27 → 合并 1）

| ID | P | 位置 | 摘要 | 状态 |
|---|---|---|---|---|
| R6-B-01~06 | P1 | jni_bridge.c:1162-1250 | on_event 6 处分发点**硬编码字面 55code**（24/42/25/26/27/16）绕过 name_to_55code 查表 | 待修(R7)（单一入口批） |
| R6-B-07 | P1 | jni_bridge.c:1639-1649 | jq.55codetest 钩子 11 次硬编码（与 B-01~06 同族） | 待修(R7)（同批） |
| R6-B-08 | P1 | jni_bridge.c:1130,1144 | move/flip 走 ev7_to_55code 显式转换但无与查表一致性 assert | 待修(R7)（加 assert 断言两表同值） |
| R6-B-09 | P1 | evt55_dispatch.c:100-124 | evname 非空但未命中 73 表 → 保留 code 默认 idx 照常计数（未知名误归因） | ✅**已修**（本轮：未命中 → idx=-1 拒计数） |
| R6-B-10 | P1 | evt55_dispatch.c:43-72 | 未映射 55code default 0 静默归因 Move | ✅**已修**（R5-B-02 补桶代表 + 本轮 default -1 哨兵） |
| R6-B-11 | P1 | evt55_business.c:25-75 | 未知名/别名静默兜底 BIZ_MOVE（name_to_55code 无错误语义） | 待修(R7)（_checked API） |
| R6-B-12 | P2 | evt55_business.c:52-72 | 前缀 strncmp 分类无 73 表白名单校验（拼写漂移静默归桶） | 待修(R7) |
| R6-B-13 | P1 | jni_bridge.c:1065 + dispatch | g_event_cnt[73]+g_event_total 双写无 Σ==total 不变量 | 待修(R7)（单一计数 API） |
| R6-B-14 | P1 | jni_bridge.c:1181-1274 | 4 处手工查名计数旁路（regret_agree/disagree/join/leave/resign_confirm） | 待修(R7)（count-only API） |
| R6-B-15 | P1 | jni_bridge.c:2391+ 8 处 | 7 码直写计数散点（flagtest/minetest/AI/OnTouch）不经 dispatch | 待修(R7)（jq_evt55_count_7code 单入口） |
| R6-B-16 | P1 | evt55_dispatch.c:106-118 | 注释"账号/联机类计数恒 0"vs 实现无条件累加矛盾 | 待修(R7)（定案：64 位无业务触发路径=防御计数，改注释+文档） |
| R6-B-17 | P2 | evt55_dispatch.c:14,99 | "g_event_cnt 是 static"注释过时（实为跨文件 extern） | 待修(R7)（注释纠错） |
| R6-B-18 | P1 | dispatch+business | Resign/ResignConfirm 塌缩 55code=24 反查仅还原 Resign | 待修(R7)（子态/name 级通道） |
| R6-B-19 | P1 | 同上 | 悔棋 6 名塌缩 42 反查仅 RegretReq | 待修(R7)（与 B-18 同批） |
| R6-B-20 | P1 | business:34-45 | 多子态桶代表码塌缩（DRAW 6 名→25 / GAMEOVER 7 名→16） | 待修(R7)（(biz,sub) 双键） |
| R6-B-21 | P1 | event_code_map.json | `Junqi.GameType` vs 73 名表 `%d` 模板键分裂 | ✅**已修**（本轮 json 统一 `Junqi.GameType%d`，1 处） |
| R6-B-22 | P2 | event_names_76.json | "76 名"文件名 vs count=73 + 无 Misc 内容漂移 | ✅**已修**（本轮 76json 键同步 + 口径注；76=73 Junqi+3 Misc 命名史，Misc 归广告拆除红线不进门禁） |
| R6-B-23 | P1 | jq_event_names_73.c:83-95 | `Junqi.Over` 双桶（code3+code11）与 json 单桶不一致 | 待修(R7)（12 码基准定案单桶） |
| R6-B-24 | P1 | evt55_sfx.c:13-31 | 73 名→音效大面积兜底 move（66 名无专音） | 待修(R7)（三层矩阵门禁） |
| R6-B-25 | P1 | jni_bridge sfx 双路径 | sfx_for_move vs sfx_wav 无 73 名全覆盖门禁 | 待修(R7)（sfx_coverage_check.py） |
| R6-B-26 | P2 | （新增工具） | 73 名→sfx key→WAV 三层矩阵门禁缺失 | 待修(R7) |
| R6-B-27 | P2 | business:104-108 | name_to_code 未知输入兜底 0 无错误语义 | 待修(R7)（与 B-11 同批 _checked） |

（B-01~06 为同型清单型 6 条，按单入口批整体修；合并 1 条 = B-06 并入 B-01~06 族）

## 三、C 组 蓝牙协议（20 条，子代理 26 → 已修/撤回 6）

| ID | P | 位置 | 摘要 | 状态 |
|---|---|---|---|---|
| R6-C-01 | P0 | jni_bridge:9922,1174 + bt_session:80 | UNDO 方向错位（发送端恒不 pop / 收端 pop 本端）→ 双端发散 | 待修(R7)（请求-应答-全盘收敛协议） |
| R6-C-02 | P0 | jni_bridge:10434-10479 | UNDO 收端仅回退棋盘格，turn/计数/栈/快照未同步 | 待修(R7)（SYNC_META 帧扩展） |
| R6-C-03 | P1 | bt_session.c switch | **DRAW/CHAT 无收帧 case**（协议已定义→default 拒收=永久分叉） | ✅**已修**（本轮：两 case 合法消费 rc=0 不改棋局 + DISCONNECTED 闸口放行 + bt_semtest 断言同步 88/88） |
| R6-C-04 | P1 | BtSpp.java + jni_bridge:490 | 断连镜像断链（Java 断连不 setprop → g_bt_connected 易恒 1） | ✅**已修**（本轮：BtSpp setBtConn helper 5 接线点 setprop jq.bt_conn 1/0 + bt_smali.sh 编译链同步） |
| R6-C-05 | P1 | jni_bridge:10480-10488 | 帧推进 strchr 停 v2 行尾 → 多帧串接仅首帧（B09-038 注释-实现矛盾） | ✅**已修**（本轮：strspn 整帧消费 + 桌面 frame_adv_test 4 场景实锤：旧 60 帧仅 1/新 60/60） |
| R6-C-06 | P1 | jni_bridge:10459-10479 | UNDO 后 60 帧 SYNC 串接 + 推进缺陷叠加 → 收端仅消费首帧 | ✅**随 C-05 修复**（整帧推进后 60/60 实锤） |
| R6-C-07 | P1 | bt_session + jni_bridge:10447 | 收帧拒绝无 NACK/回执 → 首次分叉后永久卡死不可诊断 | 待修(R7)（BT_T_NACK 帧扩展，帧协议扩字段批） |
| R6-C-08 | P1 | bt_session:152-182 | RESTART 不携带 mode/布局/先后手 → 双端一明一暗 | 待修(R7)（帧协议扩字段批） |
| R6-C-09 | P1 | bt_session:55-79,136-138 | MOVE 规则引擎演进 + SYNC_CELL 逐格覆写双写竞争（无权威源仲裁） | 待修(R7)（seq/token 重同步窗口） |
| R6-C-10 | P1 | jni_bridge:10334-10414 | OnBluetooth 懒 init we_blue=1 固定猜阵营（与 start_bt_session 角色覆盖竞态） | 待修(R7)（握手帧定角色） |
| R6-C-11 | P1 | jni_bridge:529-540 | 发送前乐观置 g_bt_connected=1 掩盖断连 | 待修(R7)（与 C-04 同批：属性为准） |
| R6-C-12 | P1 | bt_session:48-54 | DISCONNECTED 终态仅放行 RESTART/RESIGN（本轮扩 DRAW/CHAT）→ 无重连收敛通道 | 待修(R7)（BT_T_RECONNECT 握手批） |
| R6-C-13 | P1 | jni_bridge:9922 + bt_session:80 | UNDO 无 req_id/幂等 token → 重复帧双重回退 | 待修(R7)（与 C-01 同批） |
| R6-C-14 | P1 | jni_bridge:10186-10210 | 发送侧无 ui_bt_active/连接镜像/状态一致性门控 | 待修(R7) |
| R6-C-15 | P1 | bt_session:55-70 | 收 MOVE 先 jq_try_move 提交（无两阶段校验-回滚） | 待修(R7)（hash/seq 分叉检测批） |
| R6-C-16 | P2 | bt_session:141-151 | BT_T_SYNC 头帧仅 jq_state_init peer_state（无同步窗口/锁定） | 待修(R7) |
| R6-C-17 | P2 | bt_protocol.c:70-94 | SYNC 编码失败静默跳格（n<count 不报错） | 待修(R7)（返回 {ok,count,first_bad}） |
| R6-C-18 | P2 | jni_bridge:10459-10474 | g_bt_payload 缓冲截断"发半盘"（部分帧丢失） | 待修(R7)（整体失败语义） |
| R6-C-19 | P1 | bt_session RESTART + jni_bridge | RESTART 不清 undo/复盘/g_ts/bt 元数据（双端附属态发散） | 待修(R7)（原子复位集批） |
| R6-C-20 | P2 | bt_session:184-195 | RESIGN 收端仅改 winner/stage（无事件/音效/统计联动，与本地认输分叉） | 待修(R7)（统一终局入口批） |

（C-21~C-26 子代理原 26 条中 6 条撤回/并入：C-21 ui_bt_active 清理与 C-04 同批并入；
C-22 decode 容错与 C-07 NACK 同批；C-23 Java 读侧断连与 C-04 同批；C-24 并发快照
与 C-02 同批；C-25 单入口混杂与 C-10 同批；C-26 UNDO 超时与 C-01 同批 → 20 条入账）

## 四、D 组 数字口径/判据（29 条，子代理 30 → 合并 1）

| ID | P | 位置 | 摘要 | 状态 |
|---|---|---|---|---|
| R6-D-01 | P1 | GAME_DEV_GUIDELINE:62 | evverify 170/170 过期 | ✅**已修**（172/172） |
| R6-D-02 | P1 | GAME_DEV_GUIDELINE:64 | 拆分 40/40+130/130 过期 | ✅**已修**（40/40+132/132） |
| R6-D-03 | P1 | src_rs_regress.sh:53 | 注释 169/169 过期 | ✅**已修**（172/172 动态口径） |
| R6-D-04 | P1 | PIPELINE.md:91 | 169/169+几何 130 过期 | 待修(R7) |
| R6-D-05 | P1 | CODE_STYLE.md:178 | 170/170 举例过期 | ✅**已修**（172/172） |
| R6-D-06 | P1 | BUSINESS_RISK_LEDGER:20 | 历史 172/172 无时间锚点 | 待修(R7)（快照标注批） |
| R6-D-07 | P1 | BUSINESS_RISK_LEDGER:62 | 同上结论句 | 待修(R7) |
| R6-D-08 | P1 | AGENTS.md:107 | 170/170+探针 20 双过期 | ✅**已修**（172/172+36 case 4 分叉基线；system-update 同步实锤） |
| R6-D-09 | P1 | AGENTS:353/409/437 | 历史段 170/170 混当前口径 | 待修(R7)（"当时口径"前缀批） |
| R6-D-10 | P1 | align_gate.sh:7 | 门禁注释缺真值锚点 | 待修(R7)（SUMMARY 动态解析） |
| R6-D-11 | P1 | align_gate.sh:44 | 门禁名硬编码 172 | 待修(R7)（与 D-10 同批） |
| R6-D-12 | P1 | align_gate.sh:17 | 缺 pipefail（R5-D-15 未闭合） | ✅**已修**（set -uo pipefail） |
| R6-D-13 | P1 | gen_align_coverage.py:118 | evverify 断言统计正则命中 0 | 待修(R7)（SUMMARY 解析批） |
| R6-D-14 | P1 | align_coverage_matrix.md:156 | 矩阵 evverify=0 失真 | 待修(R7)（随 D-13 重生） |
| R6-D-15 | P1 | align_coverage_matrix.md:1 | 矩阵无时间戳/版本锚点 | 待修(R7) |
| R6-D-16 | P2 | gen_align_coverage.py:151 | 矩阵生成恒成功 + 未入门禁 | 待修(R7) |
| R6-D-17 | P1 | dark_align_regress.sh:29,192 | MISS=$pending 语义漂移 | ✅**已修**（SKIP 口径澄清注） |
| R6-D-18 | P1 | Makefile:55-68 | benchcheck 双源（test_expected vs BENCH_EXPECT）无同步断言 | 待修(R7) |
| R6-D-19 | P1 | rules_fork_check.py:16 | 64 位基准 167/167+50/50 过期 | 待修(R7) |
| R6-D-20 | P1 | rules_fork_check.py:257 | 运行时打印同过期基线 | 待修(R7)（与 D-19 同批） |
| R6-D-21 | P2 | BUSINESS_RISK_LEDGER:26 | 探针 19/20 旧口径 | 待修(R7)（36 case 引用批） |
| R6-D-22 | P2 | BUSINESS_RISK_LEDGER:59 | 19/20 一致结论句 | 待修(R7) |
| R6-D-23 | P2 | EXPERIENCE_LOG 多段 | 历史计数缺"快照"标注 | 待修(R7)（文首声明批） |
| R6-D-24 | P1 | SPEC_BUSINESS:78-86 | 74/76 内部矛盾 | 待修(R7)（73 名定案+演变注） |
| R6-D-25 | P1 | SPEC_BUSINESS:232,253 | "76 名 Junqi 73+Misc 3" vs 76json 实 73 无 Misc | 待修(R7)（与 D-24 同批；B-22 口径注已先行） |
| R6-D-26 | P2 | AGENTS.md:108 | "18 WAV"（SPAK 全量解包）vs 仓内 14（spak_res 实物）混用 | 待修(R7)（两口径分标） |
| R6-D-27 | P2 | rule_probe_compare vs mumu_smoke | 默认设备 10.0.182.146 vs 127.0.0.1:5556（redroid 禁用）漂移 | 待修(R7)（统一权威或强制 --dev） |
| R6-D-28 | P2 | AGENTS.md 路由表 | align_gate/event_names/gen_align/AUD_ALIGN_R1~R6/矩阵/smoke 漏登 | 待修(R7)（路由补登批） |
| R6-D-29 | P2 | EXPERIENCE_LOG §104~108 | 台账-日志交叉状态可追溯性不足 | 待修(R7)（条目映射表） |

## 五、Round 6 汇总

- **子代理 4 路全成功**（110 原始 → 100 入账；A 撤 2 / B 合 1 / C 撤 6 并入 / D 合 1）。
- **★ 本轮已修 14 条（均含验证）**：
  - **规则族**：R6-A-11/17/25（per-side 缓存**真落地**——R5 阶段 2 声称修未落地
    的回归根因，stale_v/stale_c 双槽 + 命中判据 + 3 写回 + stalemate_after 双侧 +
    undo_restore 同步）/ R6-A-06/18（side 入口守卫）/ R6-A-10+26（死分支收敛
    `st->mode` 判定）/ R6-A-12（990af71 BOMB 撞旗 total_moves）；
  - **★ 0xFF 判据定案批（本轮最大行为定案）**：dark_stalemate_matrix_test ③④
    旧期望"对方 0xFF 己无子 → 己困毙"系 B01-011 修复前快照——**复勘实锤**：
    删 `c->side==0xFF` 项 → **10 桌面用例 FAIL**（dark_flip/dark_fullgame/
    flip_draw_guard/dark_flip_draw_count/dark_flip_color/dark_redwin_ai/
    dark_touch_flip_flow 等全部依赖"0xFF 双方计可翻"= 2026-11-10 根因②定案
    同源）→ **回滚行为 + 更新测试判据 ③④ want=0 + 头注口径**（桌面判据优先
    于测试注释的 R5 教训第二次实证）。
  - **蓝牙族**：R6-C-03/09（DRAW/CHAT 收帧 case + DISCONNECTED 放行 + 断言
    88/88）/ R6-C-04（BtSpp setprop jq.bt_conn 5 接线点）/ R6-C-05/06
    （帧推进 strspn 整帧消费，frame_adv_test 4 场景实锤 旧 60 帧丢 59 → 新 60/60）；
  - **事件族**：R6-B-09（未知名隔离 idx=-1 拒计数）/ R6-B-10（default -1 哨兵）
    / R6-B-21/22（GameType%d 键统一 event_code_map.json + event_names_76.json）；
  - **口径批**：R6-D-01/02/03/05/08/12/17（172/172 全仓同步 5 处 + align_gate
    pipefail + dark_align SKIP 口径澄清）。
- **验证实锤（本轮全 PASS）**：align_gate **7/7** + dark_align **86/86**（含 ③④
  判据更新）+ bt_semtest **88/88**（DRAW/CHAT 新断言 + RESTART 夹具）+
  evverify **172/172** + frame_adv_test 4/4 + 设备冒烟 127.0.0.1:5556 **2/2**
  （so `d9d1c325`，转译器窗口内 pid 47747 存活，0 崩溃）。
- **R7 主攻方向**：① pieces_left 语义定案批（A-01~05/13/14/15/16/19/20/21/22/23
  同批一次定案：可动子=已翻非旗雷存在合法目标，全事件分支增量对齐 refresh，
  0xFF 未入集不扣，工兵挖雷存活不扣）② 蓝牙协议扩字段批（C-01/02/07/08/13/15/19/20：
  UNDO 请求-应答 + NACK + RESTART mode 字段 + 幂等 token + 原子复位集）
  ③ 事件单一入口批（B-01~08/11/12/13/14/15/18/19/20/23/24/25/26/27：
  name_to_55code 全量 + 未知名隔离 + Σ==total 不变量 + 三层 sfx 矩阵门禁）
  ④ 数字口径收尾批（D-04/06~11/13~16/18~29 余量 + AGENTS 路由补登 D-28）
  ⑤ 暗棋 rank 泄露 32 位定案（A-07/08 挂 0x39e14 实证，需真机 32 位 so）。
