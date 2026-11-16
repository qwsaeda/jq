# AUD-ALIGN R5 — 正向 64 位「对齐」对抗审计台账（Round 5，子代理 3/4 路 + 自跑补位，100 条）

> ★ **本轮子代理 3 路成功（B/D 回执 + C 自跑补位）+ 1 路限流失败**：原始回报
> **A 26 + B 28 + D 25 = 79 条子代理 + 21 条自跑补位 = 100 条入账**。
> 承接 R4（`AUD_ALIGN_R4.md`：100 条入账 / 撤回 4 误报；待修队列 58 条，其中
> P1 协议级蓝牙族群 C-13~C-25 为本轮主攻对象）。
> 类型：[实证] 已读码核实 / [判读] 需确认 / [覆盖缺口] 场景无验证。
> **★ 本轮已修 12 条**（阶段 1：BT 协议族 P1 四修 + 事件契约三修 + ev 初始化；阶段 2：困毙缓存 per-side + 注释纠错 3 处，
> so `af4521c7`→`72922a62` 装机 127.0.0.1:5556 冒烟 2/2 + 0 崩溃，
> 转译器间歇恢复窗口实锤，§108 固化）。

---

## 零、本轮环境定性（★ 最高优先，先于台账条目）

**127.0.0.1:5556（redroid12 x86_64 容器）启动崩溃 = 转译器间歇性故障，非 so/代码回归**：

- **现象**：`com.qwsaeda.junqirs`（lib/**arm64**/libjunqi.so）启动即
  `java.lang.UnsatisfiedLinkError: No implementation found for ... OnCreate`；
  3 连跑 + 设备重启后复测均复现（pid 45456/45771/45825/46108/46190/46311 全 ULE）。
- **排查链（全部排除 so/包/符号问题）**：
  ① 设备 so 与 APK 内 so 哈希一致（sha1 `eeb3da4a`，14 个 `Java_com_qwsaeda_App_*`
     符号 T 导出齐全，llvm-readelf 确认 FUNC GLOBAL 非弱符号）；
  ② smali 包名 / dex 包名 / manifest 一致（com.qwsaeda），`loadLibrary("junqi")` 正确；
  ③ 设备无 `libhoudini.so`（redroid 用 Google **ndk_translation** 组件：
     `libndk_translation.so` + 25 个 `libndk_translation_proxy_*.so`，
     日志 `I ndk_translation: Initialized NDK translation (aarch64)`）；
  ④ 对照实验：`com.cnvcs.junqi32`（**lib/arm**/armeabi-v7a）dlopen 被 linker 直接拒
     （`is for EM_ARM (40) instead of EM_386 (3)`）= 该容器 32 位库根本不支持
     （与 §124y-279 口径一致，32 位验收基准=真机）；
  ⑤ **决定性证据**：同 so 同 APK，**设备重启（转译器状态刷新）+ 新装机后**
     pid 46750 正常启动（渲染线程存活 `fromSurface 未 ready（第 13 次）`，
     无 ULE 无 FATAL）→ **转译器 aarch64 通道间歇性符号解析失败**
     （flaky，窗口可恢复）。
- **口径固化（§124y-279 同源）**：127.0.0.1:5556 冒烟 FAIL 且 ULE =
  **转译器 flaky 期**，须 `adb reboot` 刷新 + 重跑 1-2 次容错；**权威 5 判据挂起
  至红果 10.0.182.146:5555 恢复**（当前 offline）。模拟器结论不可外推真机。
- **诚实边界**：本轮 so 改动（bt_session.c / jni_bridge.c / evt55_dispatch.c /
  rules.c）**非崩溃根因**（ULE 发生在 dlopen/符号解析层，早于 JNI_OnLoad，
  与业务代码无关）；转译器故障属 redroid 容器环境问题。

---

## 一、A 组 规则引擎与暗棋泄露（26 条，子代理回执）

| ID | P | 类型 | 位置 | 未对齐摘要 | 基准 | 状态 |
|---|---|---|---|---|---|---|
| R5-A-01 | P1 | 实证 | rules.c:121-122,136-137,407 | 工兵自**行营（非铁路格）**可发起铁路 BFS（起点未受 is_rail 约束，直走路径 :155 有） | rulelay 铁路行走 / 0x39e14 | ❌**撤回**（复勘：行营格可作铁路滑行**目标**，工兵进空行营合法——B01-001 walkable 行营守护 + 目标空格判定与既有行为自洽，行营驻子不可再动由 move_legal 行营/大本营分支兜底；无行为缺陷） |
| R5-A-02 | P1 | 实证 | rules.c:138-141 | 工兵铁路 BFS 到达**占位目标格**返 walkable（目标须空格）vs 非工兵直走路径不查目标格 → 两路径判据差异 | 原版设计分派 | ❌**撤回**（复勘+实锤：dark_align 3 桌面用例定案 dark_rail_cross_eat/flagmine/r5r6_worker_eat 均实证工兵横贯滑行到交叉格吃雷/扛旗**拒**（BFS walkable 拒非空格 can=0）为既有定案行为；工兵滑行终点须空格+吃子由一步相邻专属分支覆盖=原版设计，非缺陷。曾按本条改 return true → 3 用例 FAIL → 已回滚） |
| R5-A-03 | P1 | 判读 | rules.c:265 | 源格未翻/未定色一律拒（`!src->revealed \|\| src->side==0xFF`）——**未泄露**（入口前置屏障，与 dst-rank 读取点形成边界）；记录为"无缺陷"锚点 | ruleflip 走子前提 | ✅ 撤回（非缺陷，记录定案） |
| R5-A-04 | P1 | 判读 | rules.c:300-305 | 暗棋大本营安全岛分支对 0xFF 未翻目标读真实 `rank_of(dst)` 判雷/旗例外 → 未翻子身份可观测泄露（与 :269-276 放行口径张力） | ruleflip / 0x39e14 | 待定案（32 位口径） |
| R5-A-05 | P1 | 判读 | rules.c:314-337 | 暗棋地雷/军旗捕获限制按真实 rank 预判（雷仅工兵 :322-324 / 旗仅工兵+挖满3雷 :327-334）→ 未翻雷/旗身份+可行性泄露面 | 0x39e14/0x39f20 | 待定案（与 A-04 同批） |
| R5-A-06 | P2 | 判读 | rules.c:361-399 | 大本营目标块 :398-399 `src->revealed==0` 分支因 :265 前置守卫**不可达（死分支）** + 注释以 src->revealed 作暗棋信号与前置守卫矛盾 | 单一判定点 | 待修(R6)（删死分支/改 mode 判定） |
| R5-A-07 | P1 | 实证 | rules.c:646,753 | `jq_try_move` 的 `ev` **未初始化**，`return ev` 若分支漏赋值即读 UB（P1 崩溃风险） | 终局快照一致性 | ✅**已修**（显式初始化 `jq_event ev = EV_MOVE`） |
| R5-A-08 | P1 | 实证 | rules.c:690-693,736-738,849-856 | 终局 turn 快照不对称 + 终局路径**跳过 `stale_dirty=1`/stalemate_after 预刷**（:854/:855 仅 PLAY 路径）→ 终局后缓存/dirty 位停留旧值 | 0x30200 快照一致性 | ✅**已修**（R6 自跑主攻：EV_BOMB 撞旗终局补 `total_moves++`——4 条终局置 OVER 路径对称性扫描实锤唯一缺口；commit `990af71`） |
| R5-A-09 | P1 | 实证 | rules.c:766-767,789-793 | 挖雷成功工兵**移位存活**（`*dst=*src`）但 `pieces_left[me]--` + 注释"工兵消失"与 `jq_refresh_movable_counts` 口径（可动子=存在合法目标的非旗非雷子）三方矛盾（R4-A-13 限定性重提：新证据角度=存活 vs 注释消失） | B01-008 / layout.c:255,264-268 | 待修(R6)（注释纠错+计数口径定案） |
| R5-A-10 | P1 | 实证 | rules.c:770,777 vs layout.c:264 | EV_MINE 扣减雷方 `pieces_left` 但刷新口径**从不计地雷**（可动子定义排旗雷）→ 增量/刷新定义不一致 | layout.c:255 | 待修(R6)（与 A-09 同批） |
| R5-A-11 | P1 | 实证 | rules.c:671,675,681 | EV_BOMB 同归对 dst 侧按 side 扣 `pieces_left`，dst 为旗/雷时与刷新口径冲突（旗雷不在可动子集合却扣） | 同 A-10 | 待修(R6) |
| R5-A-12 | P1 | 实证 | rules.c:1090-1097 | `jq_on_flag_capture` 扣被吃方 `pieces_left` 但军旗被刷新排除 → AUD-R-054 注释"旗是普通子之一"与刷新定义矛盾 | 0x30260 / layout.c:264 | 待修(R6)（pieces_left 语义统一） |
| R5-A-13 | P1 | 实证 | rules.c:245,747-749 | 吃旗开关关路径攻子落大本营 → "进大本营不可再动"使其永久冻结 + 可动子计数未随冻结同步（R4-A-15 新后果） | rulelay 大本营 | 待定案 |
| R5-A-14 | P1 | 实证 | rules.c:961-964,1003-1030 | `jq_side_stalemated` 单一 `stale_dirty` + "任一 cache 非零即有效"判定 → **仅算被查询 side 却宣告双方有效**，对端陈旧缓存可被误命中 | 0x302b4 实时正确 | ✅**已修**（per-side 缓存槽：stale_cache[2]→stale_v[2]+stale_c[2]，每侧独立失效/有效，全扫重算双写） |
| R5-A-15 | P1 | 实证 | rules.c:564,568,576-577 | `stalemate_after` 意图预刷对端：先判本方（置 dirty=0）→ 判对端时命中旧缓存未真重算 → 与注释契约不符 | A10-D1-01 注释 | ✅**已修**（per-side 后 stalemate_after 双侧重算各写各槽；stale_dirty 字段废弃保留） |
| R5-A-16 | P2 | 实证 | rules.c:950-954,961 | `jq_side_stalemated` **未校验 side 合法性**（非 SIDE_VALID 入参作 2 元素数组索引 → OOB）；同族 API 已有守卫但查询 API 缺 | API 防御一致性 | 待修(R6)（入口 `!SIDE_VALID` 拒） |
| R5-A-17 | P1 | 判读 | rules.c:318-324 vs :657,685 | 暗棋+cfg(4) 下 move_legal 一律拒非工兵撞雷（**含炸弹**），但执行器炸弹分支可表达 bomb-mine 同归 → 合法性闸口与执行器/规则条款（"炸弹与任何棋子相遇双方都消失"）冲突 | rulelay 炸弹 / 0x39e14 | 待定案（32 位口径） |
| R5-A-18 | P2 | 实证 | rules.c:133-136 | 注释"横移仅中线 row5/6"与代码 `r==0\|\|5\|\|6\|\|11`（含底线）矛盾（board.c:68 横向铁路=row0/5/6/11） | 注释-代码一致 | ✅**已修**（注释纠错，行为 0 变化） |
| R5-A-19 | P2 | 实证 | rules.c:416-422,617 | 注释称"from==to 走子原地判定（jq_try_move from==to 自增）"，但 :617 直接 `return -1` 无自增路径 → 注释描述的行为不可达 | 0x39f20 澄清 | ✅**已修**（注释纠错，行为 0 变化） |
| R5-A-20 | P2 | 实证 | rules.c:520-521,925-939 | 注释"与翻棋路径（jq_flip 同序自增）对称"但 `jq_flip` 明确不增加 moves_since_eat/total_moves → 注释矛盾 | AUD-R-086 翻棋不计步 | ✅**已修**（注释纠错，行为 0 变化） |
| R5-A-21 | P1 | 实证 | layout.c:116-142 vs board.c:57-59 | 布局校验蓝半区无位置约束 + `is_base` 四格对称拓扑 vs 校验仅红半区施加旗/雷/炸弹位置约束 → 蓝方越界位置可放行（R4-A-23 补证据链） | 0x2a228 | 待定案 |
| R5-A-22 | P2 | 实证 | layout.c:60-97,148-174 | 布局加载/校验**不拒行营格驻子**（is_camp 坐标映射位非 0 可生成非常规态） | board.c:40-48 行营拓扑 | 待修(R6) |
| R5-A-23 | P2 | 实证 | layout.c:106-133 + rules.c:245 | 蓝半区布局直通 + 走子期仅靠 :245 限制"自大本营出发" → 蓝旗/关键子位置异常时规则引擎依赖运行期分支（布局期校验缺失） | 0x2a228 布局期 | 待定案 |
| R5-A-24 | P1 | 实证 | rules.c:1228-1230 vs :322-324 | AI 评分给"炸弹撞地雷"800 同归分，但暗棋+cfg(4) 下 move_legal 拒非工兵（含炸弹）攻雷 → 评分层给非法着法正向评价（契约错位） | 评分基于合法着法集 | 待修(R6)（与 A-17 定案联动） |
| R5-A-25 | P1 | 实证 | rules.c:564,578,961,1003-1030 | `stale_dirty` 双边共享有效位：任一结论返回即置 0=宣告双方缓存有效，但仅写当前 side → 走子后可复用对端陈旧结果（与 A-14 同根因） | 困毙缓存正确性 | ✅**已修**（与 A-14 同批：per-side 槽根治） |
| R5-A-26 | P2 | 实证 | rules.c:406-408,121-122,155 | 工兵恒走 BFS（:407）而"铁路起点在铁路上"仅在直走路径入口强制（:155）→ 同一铁路滑行能力因棋子类型落不同起点约束 | rulelay 铁路前提 | ❌**撤回**（A-01/A-02 撤回后本条失去基础：工兵 BFS 起点约束与行营/占位目标语义自洽，直走路径 :155 端点 is_rail 校验是直滑专属（非工兵须两端在铁路），两者差异=棋子类型分派设计，非缺陷） |

## 二、B 组 事件契约 / 音效（28 条，子代理回执）

| ID | P | 类型 | 位置 | 未对齐摘要 | 基准 | 状态 |
|---|---|---|---|---|---|---|
| R5-B-01 | P1 | 实证 | evt55_business.c:24-74 | 【清单型】12 码桶 vs 业务桶 vs 代表 55code 三方全量不一致对照（73 名逐条，含 Junqi.GameType%d 键身份分裂） | event_code_map.json / jq_event_names_73.c | 待修(R6)（单一事实源三元组+一致性断言） |
| R5-B-02 | P1 | 实证 | evt55_dispatch.c:43-61,120-136 | 代表 55code 34/36/38/44/46 反查缺失 → default 0 身份丢失为 Junqi.Move（未映射 code 全记 Move） | 73 名身份可逆 | ✅**已修**（补 5 桶代表名 case 30/37/50/65/71） |
| R5-B-03 | P1 | 实证 | evt55_dispatch.c:106-113 | 注释"账号/联机类计数恒 0"与实现**无条件累加**矛盾（注释-行为二选一） | SPEC §3 计数口径 | 待修(R6)（修注释或加过滤） |
| R5-B-04 | P1 | 实证 | evt55_dispatch.c:43-61 + jni_bridge.c:1088 | 7 事件码空间 vs 55code 空间在 code=5/7 语义冲突 + 调用点跨空间传参 | 55code 语义表 | ✅**已修**（ev7_to_55code 显式转换，B-22 同批） |
| R5-B-05 | P1 | 实证 | evt55_business.c:44-45 + dispatch:55 | Resign/ResignConfirm 塌缩 55code=24，反查仅还原 Resign（idx 18）→ ResignConfirm（idx 19）身份丢失 | event_code_map code5 | 待修(R6)（55code 子态或名级计数） |
| R5-B-06 | P1 | 实证 | evt55_business.c:46-50 + dispatch:59 | 悔棋 6 名全塌缩 55code=42，反查仅还原 RegretReq（idx 20） | code6 六名 | 待修(R6)（同 B-05 同批） |
| R5-B-07 | P1 | 实证 | jni_bridge.c:1146-1165,1182-1188 | draw_disagree 分支**无计数无分发**（resign_disagree/join/leave 走手动扫描）→ 73 名计数路径不统一 | 分发即计数契约 | ✅**已修**（补 `jq_evt55_dispatch(27,"Junqi.DrawDisagree")`） |
| R5-B-08 | P1 | 实证 | jni_bridge.c:1088-1097,1101-1103 | 事件码→73 名索引多路径（ev7_to_name_idx / dispatch evname 覆盖 / 手动线性查找）且 EV_MINE 计入 Move 名索引 → 名级计数与事件语义脱节 | AUD-R-062 同名定案 | 待修(R6)（双口径一致性校验） |
| R5-B-09 | P1 | 实证 | evt55_sfx.c:13-31 | 【清单型】73 名→音效完整覆盖矩阵（13 专音 / 5 错配（Over/GameOver 丢 over.wav）/ 55 落兜底 move，含 start/select/close/pop 资源不可达） | 14 WAV 资源集 | 待修(R6)（补 start/select/over/UI 键映射或 SPEC 标注不可达） |
| R5-B-10 | P2 | 实证 | audio.c:24-39 | 死模块 EV2WAV 14 项表与活通路（sfx_wav/evt55_sfx）双实现漂移 | SPEC §5 单一音效管线 | 待修(R6)（下线 audio.c 或统一源） |
| R5-B-11 | P2 | 实证 | jni_bridge.c:605-627,919-944 | sfx_wav 覆盖 start/over/select/close/pop 但事件名分流不可达这些键 → 资源能力与分流契约断层 | 14 WAV / SPEC §5 | 待修(R6)（与 B-09 同批） |
| R5-B-12 | P1 | 实证 | jni_bridge.c:1043-1044 + dispatch:108-112 | 事件计数双写可分叉（`g_event_cnt[73]+g_event_total` 跨文件裸 extern，无 Σ==total 约束） | 计数契约 | 待修(R6)（封装单一计数 API） |
| R5-B-13 | P2 | 实证 | evt55_business.c:52-60 | code8 13 名被前缀规则拆到 BIZ_LOGIN（User*）与 BIZ_REG（其余）→ 12 码单桶 vs 业务桶分裂（B-05 同型补充） | code8 全量 | 待修(R6)（与 B-01 同批） |
| R5-B-14 | P1 | 实证 | jq_event_names_73.c:77-79 + event_code_map.json:130-137 | 73 名表 `"Junqi.GameType%d"`（idx 72）vs 12 码表 `"Junqi.GameType"`（无 %d）→ 映射键身份分裂 | .rodata 真值（含 %d） | 待修(R6)（json 同步带 %d） |
| R5-B-15 | P2 | 实证 | jni_bridge.c:1101-1103,1119-1135 | on_event 核心事件走 dispatch 计数，RegretAgree/JoinGame/LeaveGame/ResignConfirm 走手动扫描 → 73 名计数覆盖不一致 | 触发即计数统一 | 待修(R6)（单入口） |
| R5-B-16 | P1 | 实证 | evt55_dispatch.c:89-104 | evname 非空但未命中 73 表时保留 code 默认 idx（0）并计数 → 未知名事件误归因 Move | 73 名闭集契约 | 待修(R6)（未命中拒计数/unknown 槽） |
| R5-B-17 | P2 | 实证 | evt55_business.c:38-42 | 终局 Draw 与和棋协商族共用 55code=25 → 结果/协商不可区分（R4-B-04 全链延伸） | code3 vs code4 | 待修(R6)（Draw 独立 55code） |
| R5-B-18 | P2 | 实证 | evt55_business.c:61-66 | code9 联机 15 名全折叠 55code=38 → Start/Ready/Peer*/Exit* 子态无粒度 | code9 子事件语义 | 待修(R6)（关键子态独立） |
| R5-B-19 | P2 | 实证 | evt55_business.c:51-74 | 大桶宽前缀 strncmp + 分支顺序敏感（User* 先于 Name*/Pwd*）→ 桶身份依赖顺序，不可扩展 | 12 码精确归属 | 待修(R6)（显式查表） |
| R5-B-20 | P2 | 实证 | evt55_dispatch.c:43-61,129-136 | 未覆盖 55code 默认返 Move（`default: return 0` + `return "Junqi.Move"`）→ 日志/计数/诊断污染 | 未知应显式隔离 | ✅**部分已修**（B-02 补 5 桶代表；default 仍 0，R6 改哨兵） |
| R5-B-21 | P2 | 实证 | jni_bridge.c:605-617 + evt55_sfx.c | sfx_for_move 硬编码 EV_MINE→"Junqi.Move"（AUD-R-062 定案）但无契约化判据 → 未来独立 MINE 名时漏改 | AUD-R-062 | 待修(R6)（契约固化） |
| R5-B-22 | P1 | 实证 | jni_bridge.c:1088-1093 | on_event move 路径 **7 码直传 55 分发器**（数值巧合依赖） | 显式转换表 | ✅**已修**（ev7_to_55code：BOMB/FLAG→2 FLIP→6 余→0，flip 路径同改） |
| R5-B-23 | P2 | 实证 | evverify.c:69,203,3573-3609 | 172 断言中**无 73 名计数契约验证**（g_event_cnt/g_event_total/evt55_dispatch 路径 0 覆盖） | 事件计数契约 | 待修(R6)（补桌面断言） |
| R5-B-24 | P2 | 实证 | jq_event_names_73.c:82-95 + json | `jq_event_code_names` code11 `{71,72,12,-1}`（含 Over 双桶+GameType%d）vs json 文本 `"Junqi.GameType"` 且 Over 双桶 → 基准与 json 不一致 | .rodata 真值 | 待修(R6)（与 B-14 同批） |
| R5-B-25 | P1 | 实证 | evt55_business.c:76-94 + jni_bridge 多处 | 事件名→55code 查表被多处绕过（on_event 硬编码字面 24/42/25/26/16）→ 同名事件可走不同计数路径 | SPEC §3.4 单一入口 | 待修(R6)（统一 jq_evt55_name_to_55code） |
| R5-B-26 | P2 | 实证 | audio.c:54-62 + jni_bridge:605-627 | 音效资源覆盖判定分散（audio.c 14 项 / sfx_wav 14 键 / evt55_sfx 仅 10 键子集）无 73 名→WAV 一致性门禁 | 14 WAV | 待修(R6)（三层覆盖矩阵门禁化） |
| R5-B-27 | P2 | 实证 | evt55_dispatch.c:120-126 | `jq_evt55_name_to_code` 对未覆盖/别名名兜底 55code=0（Move）→ 未知静默归因 | 未知应显式失败 | 待修(R6)（错误码+告警） |
| R5-B-28 | P2 | 实证 | jni_bridge.c:1048-1055,1088-1097 | `ev7_to_name_idx` 越界兜底 0 与 dispatch/log 计数耦合 → 非法码静默计 Move（R4-D-11 补调用点证据） | R4-D-11 | 待修(R6)（独立错误分支） |

## 三、C 组 交互 / 态机 / 会话 / 蓝牙（21 条，自跑补位 + R4 台账承接）

> ★ C 路子代理（蓝牙协议族专项）因免费额度限流失败 → **自跑补位**：
> 主攻 R4 最大未闭合 P1 协议级族群（C-13~C-18/C-22），本轮**已修 5 条**，
> 余下 16 条按 R4 台账状态承接 + 新增 5 条（C-26/C-28~C-31/C-33~C-40 编号重排）。

| ID | P | 类型 | 位置 | 未对齐摘要 | 基准 | 状态 |
|---|---|---|---|---|---|---|
| R5-C-01 | P1 | 实证 | bt_session.c:143-149（旧） | 收端 RESTART 走 `jq_state_init`（60 格全清零不载布局）→ 收端空棋盘永久软锁 | 原版重开=重载布局 | ✅**已修**（按 ui_opt_get_mode 重载 JQ_DEFAULT_LAY） |
| R5-C-02 | P1 | 实证 | bt_session.c:143-149（旧） | RESTART 无条件 `state=OUR_TURN` → 主机重开后收端也变 OUR_TURN，双端互发首手被闸口丢弃 | SPEC §6 角色分流 | ✅**已修**（we_blue→OUR_TURN / 我红→THEIR_TURN） |
| R5-C-03 | P1 | 实证 | bt_session.c:116-124（旧） | SYNC_CELL 只写 `peer_state` 死影子（全仓读取点仅 bt_semtest）→ 全盘同步收敛通道失效 | SPEC §6 60 格同步 | ✅**已修**（同时回写 `st->cells`；诚实边界：发送侧 send_sync 无运行时调用点） |
| R5-C-04 | P1 | 实证 | jni_bridge.c OnTouch（旧） | 对局触摸无 BT 回合闸口 → THEIR_TURN 期间可自由改本地 g_state（发送侧拒 → 双端发散 + 暗棋布局泄露） | SPEC §6 闸口 | ✅**已修**（sess≠OUR_TURN 短路两步态机+清选中态） |
| R5-C-05 | P1 | 实证 | jni_bridge.c 4 重开入口（旧） | 仅 ☰ 菜单 BT 路径发 RESTART，其余 3 入口**静默单边重开** → 双端永久错位 | SPEC §6 RESTART 同步 | ✅**已修**（bt_broadcast_restart helper + 17 处调用点） |
| R5-C-06 | P1 | 实证 | bt_semtest.c:353-363（旧） | 既有断言**编码缺陷语义**（"棋盘清空+恒 OUR_TURN"）→ 修复后 2 条 FAIL | 测试契约随行为演进 | ✅**已修**（断言改正确语义 + 新增"我红收端 THEIR_TURN"/"SYNC 回写权威态" 2 断言，87/87） |
| R5-C-07 | P1 | 实证 | bt_session.c switch | UNDO 收端 pop 本端旧手 / 发送端 BT 态恒不 pop → 双端必然发散（R4-C-17 承接） | SPEC §6 悔棋帧 | 待修(R6) |
| R5-C-08 | P1 | 实证 | bt_session.c:152-165 | 对端认输收帧无音效/无 evt55 派发/无计数（R4-C-11 承接） | SPEC §5 认输闭环 | 待修(R6) |
| R5-C-09 | P1 | 实证 | bt_session.c switch | BT_T_DRAW/BT_T_CHAT 无收帧分支（落 default 拒收）→ 联机提和/聊天无应答（R4-C-19 承接） | SPEC §6 提和/聊天 | 待修(R6) |
| R5-C-10 | P1 | 实证 | jni_bridge.c:490-540 | 断连镜像断链（Java 无 `jq.bt_conn` setprop 消费）→ g_bt_connected 恒 1、断连后卡死（R4-C-20 承接） | SPEC §6 断连镜像 | 待修(R6) |
| R5-C-11 | P2 | 实证 | jni_bridge.c:10480-10488 | 帧推进 `strchr(p,'\n')+1` 停在 **v2 行尾**（帧形 `v2\n7字段\n` 两行）→ p 指向数据行 → 下轮 decode 首字节锚定 `strncmp(p,"v2\n")` 失败 → break → **多帧串接仅首帧处理**（UNDO 后 60 帧 SYNC_CELL 丢 59 帧；B09-038 注释宣称"从帧尾推进"与实现不符=注释-实现矛盾） | SPEC §6 帧格式（bt_protocol.c 0x26efc/0x26f24 两行帧实锤） | ✅**已修**（★ R6-C-05：`strspn(p,"v2\n")` 跳 v2 行头 → 数据行尾换行后推进=整帧消费；桌面 frame_adv_test 4 场景实锤：旧 60 帧仅 1（丢 59）→ 新 60/60 + 2 帧串接 2/2 + 单帧 1/1 + 垃圾缓冲 0（无越界）） |
| R5-C-12 | P2 | 实证 | bt_session.c 收帧 MOVE 拒绝 | 收帧拒绝无 NACK/回执 → 首次分叉后联机永久卡死且不可诊断（R4-C-24 承接） | SPEC §6 帧状态机 | 待修(R6) |
| R5-C-13 | P2 | 实证 | bt_session.c:152-165 | RESTART 帧不携带 mode 字段 → 双端可能一明一暗（本轮修复后按各自 ui_opt_get_mode，选项不一致仍发散）（R4-C-25 承接） | SPEC §7 mode | 待修(R6)（帧协议扩字段） |
| R5-C-14 | P1 | 实证 | jni_bridge.c:2756-2776 | 续局只回灌 g_state，不恢复 undo 栈 / 不重置 g_ts / 不 ui_replay_init → 复盘基线以残局为准（R4-C-08 承接） | §104 续局一致性 | 待修(R6) |
| R5-C-15 | P1 | 实证 | jni_bridge.c:2763-2776 | OnResume 续局强制 stage=PLAY → 已终局对局复活（R4-C-07 承接） | SPEC §7 续局 | 待修(R6) |
| R5-C-16 | P1 | 实证 | jni_bridge.c:230-247 + replay.c | 悔棋不截断复盘序列（seq 只增不减）→ 复盘重建错位（R4-C-09 承接） | datah_039/107 | 待修(R6) |
| R5-C-17 | P2 | 实证 | jni_bridge.c:10327-10337 | 蓝牙收帧 MOVE/FLIP 不计事件 → 战况统计只含本地着法（R4-C-12 承接） | SPEC §3 事件计数 | 待修(R6) |
| R5-C-18 | P2 | 实证 | jni_bridge.c:3582+9844 | 联机态 BACK → 直接认输确认（唯一退出路径）→ 误触白送一局（R4-C-21 承接） | 退出契约 | 待修(R6) |
| R5-C-19 | P1 | 实证 | jni_bridge.c:265-276 | `jq.mode` 属性全仓无清除通道 → 测试钩子残留永久覆盖弹窗明暗选择（R4-C-06 承接） | §2f8e5930 残留坑 | 待修(R6)（一次性消费） |
| R5-C-20 | P1 | 实证 | jni_bridge.c:10316-10340 | 收帧 MOVE 时本端 g_state 被直接改写（无"对端权威 vs 本端校验"）→ 与 SYNC_CELL 权威回写（本轮 C-03 已修）形成双写竞争 | SPEC §6 权威态 | 待修(R6)（与 C-03 后续同批：收 MOVE 前校验本端 cells） |
| R5-C-21 | P1 | 实证 | jni_bridge.c OnTouch | 回合闸口（本轮 C-04 已修）后，**SELECT 态跨回合残留**：闸口短路只 reset g_ts 于"非本方"判定后，但 BT_ST_CONNECTED（会话未起）态触摸无任何处理 → 会话未起时触摸被静默吞 | SPEC §6 会话态 | 待修(R6)（闸口补 CONNECTED 态日志/提示） |

## 四、D 组 文档 / 判据 / 工具（25 条，子代理回执）

| ID | P | 类型 | 位置 | 未对齐摘要 | 基准 | 状态 |
|---|---|---|---|---|---|---|
| R5-D-01 | P1 | 实证 | align_gate.sh:7,44 vs src_rs_regress:53 vs GUIDE:64 vs EL:2413 | evverify 期望数 **172/170/169 三口径并存**（R4-D-24 未闭合） | 单一锚点 | 待修(R6)（统一数字+门禁标签同步） |
| R5-D-02 | P1 | 实证 | gen_align_coverage.py:118 | evverify 断言数统计正则 `\bchk\(|\bassert` 命中 0（run_case 自定义写法）→ 矩阵写 0 与 172/170/169 矛盾（R3-N12/R4-D-06 未闭合） | 统计口径 | 待修(R6)（改 run_case/ev_total 计数） |
| R5-D-03 | P2 | 实证 | rules_fork_check.py:47 | 64 位基准"evverify 167/167 + regress 50/50"与最新 169/172 漂移 | 单一数字 | 待修(R6) |
| R5-D-04 | P2 | 实证 | SPEC:37,159 vs consistency_desktop.c:33-40 | SPEC 和棋"70/1000 步"未统一"翻棋不计数"（0x302f0 实锤口径）（R4-D-22 未闭合） | 0x302f0 | 待修(R6) |
| R5-D-05 | P2 | 实证 | SPEC:82-85 | 事件名"10 大类求和=74" vs "全量 76" 内部矛盾（R4-D-12 未闭合） | SPEC §3 | 待修(R6)（统一 76+演变注） |
| R5-D-06 | P2 | 实证 | SPEC:52,237-251,127-132 | 业务码→55code 映射冲突 + 音效两套命名 + WAV 14/18/20 三值并存 | SPEC §3/§5 | 待修(R6)（单表收口） |
| R5-D-07 | P2 | 实证 | BUSINESS_RISK:26,59 + EL:6194,6301 + AGENTS:107 | 双端探针数 19/20/28/29/32/36 多值漂移 | 单一锚点 | 待修(R6) |
| R5-D-08 | P2 | 实证 | mumu_align_smoke.sh:54-65 vs AUD_ALIGN_R3 | 冒烟 3 次容错已落地但 R3 台账仍列 D-09 待修 → 台账状态滞后 | §106 口径 | 待修(R6)（台账同步） |
| R5-D-09 | P1 | 实证 | event_names_alignment_check.py:80,70-73 | 防护阈值 `len(o)>=70`（低于真实 73）+ 集合-only diff（无顺序/多重度）+ 未纳 Misc/%d（R3-N17/N18/N19 未闭合） | 73 名门禁强度 | 待修(R6)（≥73+序/多重度+Misc/%d） |
| R5-D-10 | P2 | 实证 | event_code_map_check.py:44-49 | 只核 Junqi 主链（Misc 3 条归因广告拆除）口径未显式标注（R3-N18 承接） | 口径标注 | 待修(R6) |
| R5-D-11 | P1 | 实证 | dark_align_regress.sh:29,192 | D-05 修后 `pending` 无累加点但汇总仍输出 `MISS=$pending` → 字段语义漂移 | D-05 修复遗留 | 待修(R6)（汇总改 FAIL 口径） |
| R5-D-12 | P2 | 实证 | Makefile:56-68 vs consistency_desktop.c:33-40 | benchcheck 只比 test_expected.txt，内嵌 BENCH_EXPECT 双源无同步断言（R4-D-15 未闭合） | 双源防线 | 待修(R6) |
| R5-D-13 | P2 | 实证 | AGENTS.md:563-573 | 工具路由表漏登 align_gate / event_names_alignment_check / gen_align_coverage / AUD_ALIGN_R1~R4 / 覆盖矩阵（R4-D-01/R2-N18 未闭合） | 路由纪律 | 待修(R6) |
| R5-D-14 | P2 | 实证 | AGENTS.md:107,409,437,512 | AGENTS 自述"170/170 / 20 用例 19 一致"与台账最新（172/169/36/73）漂移 | R4-D-24 相关 | 待修(R6) |
| R5-D-15 | P1 | 实证 | align_gate.sh:17 | 头仅 `set -u` 缺 `set -o pipefail` → 改写 run_gate 后易复发管道吞码假绿（CODE_STYLE §9 三型） | §9 假绿防线 | 待修(R6)（pipefail+FAIL return 1） |
| R5-D-16 | P2 | 实证 | AUD_ALIGN_R3.md:89 vs :80 | "六层验证"记账 vs align_gate 实跑 7 项（R4-D-25 未闭合） | 层数单一 | 待修(R6) |
| R5-D-17 | P2 | 实证 | src_rs_regress.sh:42-45,51-56 | [8.5/12] 编号分母错位（13 个 run 块）+ 退码分级说明未同步（R4-D-18 相关） | 回归枚举 | 待修(R6) |
| R5-D-18 | P2 | 实证 | GAME_DEV_GUIDELINE.md:62-66 | "12/12"回归项枚举 11 项 vs src_rs_regress 实际条目不一致（R4-D-18 未闭合） | 回归枚举 | 待修(R6) |
| R5-D-19 | P2 | 实证 | evverify.c:2833-2847 | SUMMARY 先打"全对 N/N"再分列事件/几何，文案与 ret1/ret2 退码语义需校准（[判读]） | 退码分级 | 待修(R6) |
| R5-D-20 | P2 | 实证 | rule_probe_compare.py:37 vs mumu_align_smoke.sh:11 | 默认设备红果（权威）vs 127.0.0.1:5556（redroid 禁用）口径漂移（redroid 禁用后） | CODE_STYLE §95-96 | 待修(R6)（统一红果） |
| R5-D-21 | P2 | 实证 | Makefile:56-68 | benchcheck 双源同步断言缺失（= D-12，同批） | R4-D-15 | 待修(R6) |
| R5-D-22 | P2 | 实证 | event_code_map_check.py:58 | 主文"只核 Junqi 主链" vs regen_check Misc 另判 → 两路径口径不一（= D-10 同批） | R3-N18 | 待修(R6) |
| R5-D-23 | P2 | 实证 | UI_COMPARE.md:1-10,395-414 | 基准"900 系×1.25" vs "720×0.8" 两套并存 + "12 项全对齐"汇总（R4-D-23 未闭合） | UI_COMPARE 基准 | 待修(R6) |
| R5-D-24 | P2 | 实证 | EXPERIENCE_LOG.md:1713 | 同文件内"171/172"与大量"172/172"混用（五代断言口径 8→47→60→167→169 叠加今 172/170/169） | 单一锚点 | 待修(R6) |
| R5-D-25 | P2 | 实证 | EXPERIENCE_LOG §104~§107 | 四轮台账承接链（R1→R2→R3→R4）状态/编号自洽性不足（交叉状态漂移） | 台账状态机 | 待修(R6) |

## 五、Round 5 汇总

- **子代理 3 路成功**（A 26 + B 28 + D 25 = 79 条）+ **C 路子代理限流失败自跑补位**
  （21 条，主攻 R4 P1 蓝牙协议族群）= **100 条入账**（A26/B28/C21/D25）。
- **★ 本轮已修 12 条（阶段 1 八条 + 阶段 2 四条，均含验证）**：
  - **R5-C-01/C-02**（P1 协议级）：RESTART 收端清空棋盘→重载默认布局 + 双端争先手→角色分流；
  - **R5-C-03**（P1 协议级）：SYNC_CELL 死影子→回写权威态 `st->cells`（发送侧接线留 R6）；
  - **R5-C-04**（P1 协议级）：OnTouch 无回合闸口→THEIR_TURN 短路两步态机+清选中；
  - **R5-C-05**（P1 协议级）：4 重开入口仅 1 广播→`bt_broadcast_restart` + 17 处调用点；
  - **R5-C-06**：bt_semtest 断言语义更新（87/87 PASS）；
  - **R5-B-02**（P1 假绿同族）：55code 34/36/38/44/46 反查缺失补桶代表名；
  - **R5-B-07**（P1）：draw_disagree 补计数分发；
  - **R5-B-22/B-04**（P1 数值巧合依赖）：`ev7_to_55code` 显式转换（BOMB/FLAG→2 FLIP→6 余→0）
    + on_event move/flip 两分发点改造；
  - **R5-A-07**（P1 崩溃风险）：`jq_try_move` 的 ev 未初始化→显式 `EV_MOVE` 初始化。
  （commit `64e5a24` 阶段 1 so `af4521c7`→`72922a62`；阶段 2 追加 4 条，commit `b4e7134`：
  - **R5-A-14/A-15/A-25（P1 同根因一次关三条）**：困毙缓存单一 `stale_dirty`+
    "任一 cache 非零即有效" → 仅算被查询 side 却宣告双方有效，对端陈旧缓存可被误命中。
    修：`stale_cache[2]` 升级为 `stale_v[2]+stale_c[2]` **per-side 槽**（v=有效位/
    c=可行动值），走子/翻棋后双侧 v=0 强制重算，全扫重算双写（stalemate_after
    对端预刷同改）；
  - **R5-A-18/A-19/A-20（P2 注释纠错批，行为 0 变化）**：横移注释 row5/6→row0/5/6/11 /
    from==to"自增"（实为 :617 直接拒，无自增路径）/ "翻棋同序自增"（实为翻棋不计步）
    三处注释与代码矛盾全部纠正。
  （阶段 2 全门禁 7/7 + dark_align 86/86 + 冒烟 2/2 PASS）
- **验证实锤（本轮全 PASS）**：align_gate **7/7** + dark_align **86/86** +
  bt_semtest **87/87** + 设备冒烟 127.0.0.1:5556 **2/2**（转译器恢复窗口，pid 46430→46750
  渲染线程存活，0 崩溃）。
- **环境定性（★ 本轮最大非代码发现）**：127.0.0.1:5556 启动崩溃 ULE =
  **redroid ndk_translation aarch64 转译器间歇性符号解析失败**（flaky，设备重启可刷新，
  同 so 同 APK 重启后正常）；**非 so/代码回归**（排查链 5 项全排除：so 哈希一致/14 符号
  T 导出/smali 包名一致/loadLibrary 正确/junqi32 32 位库 EM_ARM 拒载对照）。
  权威 5 判据挂起红果 10.0.182.146（offline）。
- **撤回 4 条误报/复勘撤回**：R5-A-03（源格前置守卫"未泄露"被报成问题 → 实为无缺陷锚点，
  撤回并记录定案）+ 阶段 2 复勘撤回 **R5-A-01/A-02/A-26**（铁路族 3 条：A-02 曾按
  子代理建议改 BFS 目标 `return true` → dark_align 3 用例 FAIL 实锤"工兵滑行到交叉格
  吃雷/扛旗拒（can=0）"=既有定案 → 回滚；工兵滑行终点须空格 + 吃子由一步相邻专属分支
  覆盖 = 原版设计分派，非缺陷；A-01/A-26 随之撤回）。教训复用 R4：子代理结论必须回读
  注释既有定案再入账；**规则引擎改动必跑桌面用例**（dark_align 86 用例为规则层权威
  判据，子代理推断与桌面判据冲突时判据赢）。
- **R6 主攻方向（阶段 2 后更新，按价值排序）**：
  ① ~~困毙缓存 per-side~~（✅ R5-A-14/15/25 已修）/ ~~② 铁路起点统一谓词~~
  （❌ R5-A-01/02/26 撤回——桌面用例定案"滑行终点须空格"=原版设计）
  ② pieces_left 语义统一（A-09~A-13 同批，含 B01-008 注释纠错；**新增 R5-A-16
     jq_side_stalemated side 合法性入口守卫**）
  ③ 蓝牙 R6 族：UNDO 方向（C-07）+ 断连镜像（C-10）+ 帧推进（C-11）+ NACK（C-12）
     + 收 MOVE 前校验本端 cells（C-20，与已修 C-03 同批）
  ④ 事件契约单一入口（B-25/B-16/B-27 同批：name_to_55code 全量 + 未知名隔离）
  ⑤ 数字口径统一（D-01~D-07/D-24：evverify 172/170/169 → 单一锚点）
  ⑥ 暗棋 rank 泄露定案（A-04/A-05 需 32 位 0x39e14 口径）+ 终局字段一致性（A-08）
  ⑦ 注释纠错残余（B-03 计数注释矛盾 / B-21 MINE 契约固化）
