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
