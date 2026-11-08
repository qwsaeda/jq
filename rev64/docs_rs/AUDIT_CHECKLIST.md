# 对抗审计 checklist（src_rs 可复用清单）

> 定位：子代理对抗审计的**标准检查清单**（从首审 ses_e57da4de7ffeejsT1I4lNcP7Ho +
> 复验 ses_e57be89dbffeP8b3iYx297XrtZ 经验固化）。每次 subagent 审计直接套用本清单，
> 逐项报 ✅/⚠/❌ + 文件行号 + 证据。**审计必须独立视角，不偏向开发方。**

## A. 代码 vs 业务规格（SPEC_BUSINESS.md）

| # | 检查项 | 关键文件 | 首审教训 |
|---|---|---|---|
| A1 | 规则条款：吃子小压大 / 炸弹同归 / 工兵挖雷 / 扛旗胜 | `rules.c jq_check_target` | ✅ 对齐 |
| A2 | 铁路：十字 + **双方底线 row0/11** | `board.c is_rail` + `rules.c rail_path_straight` | ⚠ 底线横滑被拒（已修 #5） |
| A3 | 和棋 70 步 / 困毙无子可动 | `rules.c jq_on_draw/jq_on_stalemate` | ✅ 对齐 |
| A4 | 事件契约：Junqi.* 7 类 + jq_try_move 返回 -1 | `rules.c` + `jni_bridge.c JQ_EVENT_NAMES` | ✅ 对齐 |
| A5 | UI 流程：菜单 → 对局（**明棋**）→ 横幅 → 再来一局 | `jni_bridge.c` + `render.c rs_draw_overlay` | ❌ 暗棋路径（已修 #4） |
| A6 | 音效 14 WAV 三入口 | `audio.c` + `jni_bridge.c sfx_*` | ⚠ flip 不经 sfx_for_move |
| A7 | 蓝牙 v2 7 字段（type 首字段） | `bt_protocol.c` | ❌ 丢 type（已修 #3） |

## B. 代码 vs 游戏开发规范（GAME_DEV_GUIDELINE.md）

| # | 检查项 | 关键文件 | 首审教训 |
|---|---|---|---|
| B1 | 状态机 UI_MENU/UI_GAME + STG_PLAY/OVER | `ui_state.c` + `rules.c` | ✅ |
| B2 | 规则引擎返回 int（禁 enum）+ 0=成功契约显式 `==0` | `rules.c` + `jni_bridge.c ai_pick` | ⚠ layout 返回 enum（低） |
| B3 | 锁纪律：g_state 读写 + jq_try_move + undo 在 g_mutex 内 | `jni_bridge.c OnTouch/OnUpdate` | ❌ 锁外读 + 锁内 sfx（已修 #6） |
| B4 | 音效统一入口（sfx_for_move/sfx_game_over/sfx_play） | `jni_bridge.c` | ✅ |
| B5 | 重开统一 jq_restart_game（6 处收敛，勿内联 load_layout） | `jni_bridge.c on_event` | ❌ 3 处绕过（已修 #4） |
| B6 | 悔棋连续 pop 到 BLUE | `jni_bridge.c undotest` | ✅ |
| B7 | AI 不主动送死（sc 表：吃子 > 同级/炸弹 > 空移 > 送死） | `rules.c ai_score` | ❌ 未归一化（已修 #2）+ 隐藏雷 bug |
| B8 | 触摸 ACTION_DOWN 过滤 | `jni_bridge.c OnTouch` | ✅ |

## C. 代码 vs 代码规范（CODE_STYLE.md）

| # | 检查项 | 关键文件 | 首审教训 |
|---|---|---|---|
| C1 | GetMethodID 后 ExceptionCheck/ExceptionClear | `jni_bridge.c`（jni_method_audit.py） | ✅ 0 缺口 |
| C2 | local ref 用完即删，删后不再用 | `jni_bridge.c` | ✅ |
| C3 | 内部符号悬空（UND 审计） | `so_link_check.py` | ✅ |
| C4 | **GetStringUTFChars 严禁 `(char)` 截断** | `jni_bridge.c OnEvent/OnBluetooth` | ❌ 截断（已修 #1）+ 栈指针释放（已修 #11） |
| C5 | 命名前缀 jq_/rs_/ui_/bt_ | 全 src_rs | ⚠ static 辅助无前缀（低） |

## D. 防复发（回归盲区教训）

| # | 检查项 | 固化 |
|---|---|---|
| D1 | 设备端 AI 路径盲区 | `make aittest`（ai_score 11/11）+ `regress [4/12]` |
| D2 | JNI OnEvent 入口盲区 | `src_rs_check.py [11]`（截断审计 0 处） |
| D3 | 桌面回归 ≠ 设备端 JNI/AI | 审计必查 JNI 入口 + 设备端路径，勿只看桌面 12/12 |
| D4 | 子代理对抗审计每轮 ≥1 次 | AGENTS.md 路由表 |

## 首审教训（防"回归通过 ≠ 缺陷不存在"）

1. **回归 12/12 覆盖不到 JNI OnEvent 入口 + 设备端 ai_pick**（桌面直接调 on_event/ai_step 绕过）——
   审计才抓到 3 严重缺陷。**任何 JNI/设备端路径改动必须专项回归或审计，勿信桌面 12/12。**
2. **规格内部矛盾也是审计发现**（SPEC §1 棋盘 10×9 vs 60 格 / §3 7 类 vs 6 槽）——审计不只查代码，也查规格自洽。
3. **提取内联逻辑为可单测函数暴露隐藏 bug**（ai_score 提取发现普通子碰雷误判）——内联评分逻辑建议提取。

## E. 横向 8 域对抗审计扩展清单（§2026-11-08 AUD-1000，546 新发现 + 398 存量）

> 定位：A-D 是"代码 vs 规格/规范"单域；E 是**横向块 × 功能**交叉的 8 域对抗审计标准域划分 +
> 每域固定判据（防子代理"泛泛而谈"）。每域给 10 类判据，每条发现必须 `文件:行 — 问题 — 证据摘录(≤60字)`。

| 域 | 覆盖 | 固定判据（10 类） | 本轮结果 |
|---|---|---|---|
| D1 代码质量 | src_jni_translated/ 全 .c/.h | 括号配平/重复定义/悬空 extern/多线程无锁 static/MEM 解引用无守护/日志无阀/注释矛盾/魔法数字/死代码/include | 106 条 |
| D2 工具链门禁 | rev64/tools/ + 顶层 sh/py | 静默失败(grep吞错/\|\|true)/幂等破坏/门禁 bypass/判据弱grep锚/设备包名硬编码/固定sleep/路径假设/三件套缺项/循环依赖/文档行为不一 | 100 条 |
| D3 文档一致性 | docs_rs/ + AGENTS.md | 路由声称vs实际/被推翻未标作废/跨文档数值不一/诚实边界vs后文实锤/SPEC条款vs代码/术语不一/死链/重复内容/红线vs脚本/日期过期 | 103 条 |
| D4 渲染/GL | F_23eec/F_9d43c/F_9e760/gl_bridge | GL状态泄漏/顶点索引语义/0x502误skip/纹理行对齐/shader生命周期/viewport/矩阵NDC/双批游标/centerpx判读/跨线程context | 43 条 |
| D5 事件/规则 | F_3a92c/F_3ab64/F_55d40/jq_rs_* | switch vs thunk矛盾/兜底语义冲突/handler返回值/事件码来源/事件表disc=0功能缺失/R-073分叉/走子桥互串/事件对象字段/单例惰性/终局重开 | 57 条 |
| D6 交付物/包管理 | dist/phase*+三件套+keystore | 三件套完整/签名v3/minSdk矩阵/gitignore vs 构建依赖/双源漂移/verify假绿/包名口径/BT双树/覆盖安装/产物入库 | 46 条 |
| D7 翻译忠实度 | jqt_vtbl.c 81750 行 | 归一化常量/守护丢弃/stub函数/跳过bl/.bss全0兜底/handler真译vs stub/指令断点/寄存器ABI/PC-relative &~3 | 60 条 |
| D8 git 卫生 | 全仓 git | 应入未入/commit message失真/红线违规/大文件历史/分支/gitignore效果/脏状态/杂物/dist跟踪/原子性 | 31 条 |

### 审计纪律（本轮固化，防复发）

1. **数量目标 vs 诚实边界**：用户要"1000 个问题"≠ 硬凑 1000。全仓可复现池估算 850-950，
   缺口如实标注"该类未开域"（B06-B10 = 32 位大表/测试盲区/文档 Java/全仓边角）。
   凑数 = 重复/噪声，违背红线。**审计产出以"可复现定位（文件:行）"为硬标准，条数虚高无价值。**
2. **撤回误报必须计入**：原始 558 → 撤回 12 → 净 546（D1 括号配平原始计数 9 误报——注释/字符串
   里的括号被误统计；D4 viewport 1024 假设与当前代码不符；D6 #30/#41 self-retract）。
   **撤回数与有效数并列写进台账，不藏误报。**
3. **P 级只给精确实测的数字**：存量 B 批次 grep 实测（P0=31/P1=204/P2=152/P3=11=398）；
   新域按类别输出的不做精确 P 级拆分（避免假精度），P0 逐条列、P1+ 标"待下轮人工核定"。
4. **跨域重复归主域**：8 域独立扫描必有交叉（D1 多线程竞态 ↔ D5 走子桥互串 ↔ D7 ABI 偏差），
   归并到主域，不重复计数。
5. **P0 明细 = 本轮修复输入**：AUD_1000.md "P0 明细 22 条 + Top 10 优先级"即下轮开发起点，
   按"1 条修通多域"排序（EGL share_context / F_3ab64 ok 判据 / 门禁管道吞码 为最高杠杆）。
6. **台账落盘**：`rev64/audit/r3/AUD_1000.md`（域划分表 + P0 明细 + 分级口径 + 撤回清单 + Top 10）。
