# 军棋游戏开发规范（src_rs 正向开发准则）

> 定位：src_rs 正向开发（UI/音效/规则完善/联机）的**强制准则**，从经验库隐性规范沉淀成文。
> 违反任一准则 = 回归必挂或真机崩（每准则附"违反后果"实证）。

## 1. 状态机准则

- 屏幕态：`UI_MENU ↔ UI_GAME`（`ui_state.c`，g_ui_lock 保护）。菜单态 AI 不介入。
- 对局态：`STG_PLAY → STG_OVER`（走子/和棋/困毙/认输触发）。STG_OVER 后画胜负横幅 + "再来一局"按钮。
- 触摸两步态机：`jq_touch_state`（IDLE → SELECTED → MOVE）。点自身 = 翻棋；点空格 = 走子。
- **违反后果**：AI 在菜单态介入会推红方（对局未开始就走子）。

## 2. 规则引擎返回值契约

- `jq_try_move` / `jq_can_move` / `ai_pick` **一律返回 int**（**禁用 enum 作返回值**）。
- **0=成功契约必须显式 `==0`**（`if(fn())` 只用于布尔/真值函数）。
- `jq_try_move` 返回事件码 0-5，**-1 = 非法**（不改变状态）。
- **违反后果**：enum 符号性优化（`jnev<0?0:jnev` 被优化成 `jnev`）→ `JQ_EVENT_NAMES[-1]` 越界 SIGSEGV；`if(ai_pick())` 把 0 当 false → AI 永不介入。

## 3. 锁纪律（g_mutex）

- `g_state` 读写 + `jq_can_move/jq_try_move/jq_flip` + `undo_push/undo_pop` + `jq_restart_game` **全部在 g_mutex 内**。
- `sfx_play` 锁外调（不读 g_state）；但 MediaPlayer 调用可锁内（NewGlobalRef 跨线程，已实证无崩）。
- 渲染线程 OnUpdate 与 OnTouch 主线程持同一把 g_mutex（g_state 并发防野指针 SIGSEGV）。
- **违反后果**：g_state 裸读写 + 并发 → 野指针 0x80b5dbe78 SIGSEGV（第二手走子崩）。

## 4. 音效统一入口

- 走子事件 → `sfx_for_move(ev)`（EV_EAT→eat / EV_BOMB→bomb / EV_FLAG→flag / EV_DEAD→dead / EV_MOVE→move）。
- 对局结局 → `sfx_game_over(env, stage, winner)`（winner 0→win / 1→lose / 2→draw，**三态完整**）。
- 菜单/UI → `sfx_play` 直接（start/over/select/close/pop）。
- **新音效必须走统一入口，勿散点 sfx_play**（防"改一处漏两处"）。
- **违反后果**：sfx_game_over 只处理 winner==BLUE/RED → 和棋不播音效；三处重复不一致。

## 5. 重开/悔棋统一入口

- 重开：`jq_restart_game()`（6 处收敛：restartgame property / 菜单按钮 / 点棋盘开局 / 再来一局 / 左上角 / 自动开局）。
- 悔棋：`undo_push` 快照 + **连续 `undo_pop` 到 turn==BLUE**（撤销 AI 应着 + 玩家走子）。
- **违反后果**：undo_pop 一次恢复 turn=RED → AI 立即补应着 → undo 失效（栈永不空）。

## 6. AI 准则

- `ai_pick`（红方贪心）：吃子 1000 > 同级 900 > 炸弹 800 > 工兵挖雷 700 > 空移 2 > 送死 1（★ AUD-1000 B09-012 口径纠正：旧"吃子 700-1000"误把 700 当吃子下界——700 是工兵挖雷档（rules.c ai_score 收表：1000/900/800/700/2/1/-1/-2，B01-079 已固化）；**AI 不主动送死**）。
- `ai_step` 必须接受 `jq_state*` 参数（**不读全局 st**，防状态串味）。
- AI 只在 `ui_screen_get()==UI_GAME` 介入；OnUpdate 检查 turn==RED 时 AI 已切回 BLUE 不重复。
- **违反后果**：sc=50 送死（AI 主动撞被吃）；ai_step 读全局 st → 状态串味 gametest FAIL。

## 7. 触摸/输入准则

- OnTouch 必须**过滤 ACTION_DOWN**（非 0 返回 JNI_FALSE，防一次 tap 触发 SELECT+MOVE 两次）。
- 触摸坐标归一化 [0,1]²（消除 MuMu 坐标系与渲染 viewport 缩放差异）。
- 走子/事件真机验证走 **OnUpdate/OnCreate property 触发**（触摸 MuMu flaky 不达 native——环境限制）。

## 8. 布局/资产准则

- 布局校验：`jq_validate_layout`（60 格 = 每 rank 计数 ×2；`seen[r] != cnt[r]*2`）。
- 资产复用：rebuild_rs_apk.sh 推 spak_res（设备 /data/local/tmp/spak_res/ 优先，免 AAsset 锁竞争）。
- 音效/纹理走文件系统优先（/data/local/tmp/），缺失才 fallback assets（dlsym 规避 UND）。

## 9. 回归准则

- `src_rs_regress.sh` **12/12**（编译/蓝牙/桌面走局/规则回归/红线审计/布局/触摸/gametest 60 局/链接审计/走子预验证/长路径/事件验证 170/170）。
  ★ A6-07（P3 已修）：旧注释"事件验证 8/8"为 2026-10 前口径；现行 = 桌面事件验证
  170/170（事件/业务 40/40 + 几何 130/130），grep 判据"全对"（170/170 全对）。
- 真机 `mumu_fulltest.sh` **9/9**（走子+FLAG+MINE+重开+和棋+认输+困毙+close/pop）。
- **工具升级必须同步回归判据**（N25：evverify 5→6 断言时 grep 判据同步，否则假 PASS）。
- **换 so 后必跑 rs_cycle**（make so + 12/12 + rebuild 装机，勿只 rebuild——装机旧 so 坑）。
