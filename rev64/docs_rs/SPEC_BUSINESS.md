# 军棋业务规格说明书（src_rs 正向开发基准）

> 定位：本文件是 **src_rs 正向开发的业务规格**（玩法/规则/UI 流程/事件/音效的系统描述），
> 从 `R-route_reversal.md` 逆向证据链 + `CONSISTENCY.md` 一致测试口径 + 经验库沉淀提炼。
> 后续正向开发（UI/音效/规则完善/联机）以本规格为基准，**规格变更必须同步代码 + 回归**。
>
> 诚实边界：src_rs 是**逆向驱动重写**（规则引擎对齐原版 0x39e14/0x39f20/0x302b4 等），
> 非规格先行的纯正向设计；本规格是"**从已对齐实现反推 + 验证**"的业务描述，非独立设计稿。

## 1. 棋盘与棋子

- 棋盘 **12 行 × 5 列**（row 0-11 / col 0-4，十字铁路 + 双方底营），共 **60 格**（每方 30 格）。
  ★ AUD-R-076（P2 已修）：原写"10×9（row 0-11 / col 0-8）"与实现 12×5（board.c/junqi.h 确证
  棋盘 12 行 5 列、60 格）矛盾，尺寸以 board.c 实现为准（原"10×9"系把 6 格/方 30 格误写成
  10 行 9 列，"共 60 格"又与 10×9=90 自相矛盾，本轮修正）。
- 每方 **25 子**（5 空位，镜像分布）：rank 编码 **0-9 普通（小=强）**、**10 炸弹**、**11 地雷**、**12 军旗**。
- 原始字节（raw）用 `+0xc` 区分 side（红方 raw=rank+0xc），加载时还原（`jq_load_layout` 0x2a340）。
- 默认布局 `JQ_DEFAULT_LAY`（30B）与原版 `data_lay_default.lay` 逐字节一致。

## 2. 规则条款（对齐原版 rulelay/ruleflip + 逆向函数）

| 条款 | 规则 | 逆向证据 |
|---|---|---|
| 吃子 | rank 小=强（3 吃 8）；目标 rank=12 军旗 → 吃旗胜 | 0x39e14 `jq_check_target` |
| 炸弹 | 任何子碰炸弹同归于尽（双方消失） | 0x39e14 `0xa` 特判 |
| 地雷 | 仅工兵（rank 9）可挖；地雷不可动 | 0x39e14 `0xb`（`from_row>3` 工兵） |
| 军旗 | 任何已翻子扛旗即胜（rank 12） | 0x39e14 `0xc`（`from_row∈{1,3,5}` 特判） |
| 走子 | 铁路（十字 + 双方底线 row0/11）直线任意格；普通格一步 | `is_rail`（board.c） |
| 翻棋 | 点未翻己方子 → 翻开（暗棋玩法）；明棋全子 revealed=1 | 0x39f20 from==to→翻棋 |
| 暗棋未翻目标 | 暗棋（mode=FLIP && 开关 cfg(4)=1）下未翻普通暗子（rank 1-9 revealed=0）不可作为走子目标（仅可翻/不可撞），非雷非旗目标必须已翻才可吃 | B01-053 本轮落地，ruleflip 信息不泄露语义 |
| 暗棋开局 | 暗棋开局=jq_dark_setup 镜像满盘 60 子（30 子 ×2 蓝红镜像，全 0xFF 未定色），非 50 子随机；main.c 50 子随机路径仅桌面参考不接线 | B01-044 SPEC 定案（原版 32位 0x39e14 实证行为） |
| 规则开关跨线程 | ui_rule_cfg_get 整函数走 g_ui_lock（5 开关聚合读防竞态），规则层读 UI 开关无裸读 | B01-072 落地（ui_state.c 锁化） |
| 终局原子性 | EV_FLAG 扛旗分支先快照 src，jq_on_flag_capture 成功后再写格+turn/计数；开关关闭路径先写格再 stalemate_after；双终局叠加（EV_BOMB 撞旗先 OVER）时攻子入位不回滚 | B01-064 重构落地（rules.c） |
| 行营 | 安全岛：己方子可自由走出（行营非陷阱）；敌方不可吃行营内子；行营非铁路不可穿行 | rulelay「行营安全岛」，is_camp（board.c） |
| 大本营 | 底线两角（蓝 row0 col1/3，红 row11 col1/3）：军旗落点；空大本营不可入（布局期军旗/地雷目标格） | is_base（board.c） |
| 铁路 | 纵向 col2 贯穿；横向 row5/6 中线 + row0/row11 底线；工兵可拐弯滑行，其它子直线滑行 | is_rail + rail_path_*（board.c/rules.c） |
| 和棋 | **70 步无吃子 / 总 1000 步** | 0x30280 `jq_on_draw` |
| 困毙 | 当前方无子可动/可翻 → 该方负 | 0x302b4 `jq_on_stalemate` status=3 |

- 布局校验（`jq_validate_layout` 0x6a3b0）：60 格 = 每 rank 计数 ×2（双方镜像）；炸弹 `idx<=34` 限位、地雷 `idx<=49`、军旗 `idx∈{56,58}`（0x2a228 阈值 0x1d/0x22/0x31/0x38/0x3a）。
- 胜负分发（0x30200）：winner 0=蓝胜 / 1=红胜 / 2=和棋。

## 3. 事件契约（Junqi.* 7 类）

| 事件 | 名称 | 触发 |
|---|---|---|
| `EV_MOVE` | `Junqi.Move` | 普通走子/挖雷 |
| `EV_EAT` | `Junqi.Eat` | 吃子（rank 小压大） |
| `EV_BOMB` | `Junqi.Bomb` | 同级同归/碰炸弹 |
| `EV_FLAG` | `Junqi.Flag` | 扛军旗（对局结束） |
| `EV_DEAD` | `Junqi.Dead` | 子被吃/同归 |
| `EV_FLIP` | `Junqi.Flip` | 翻棋 |
| `EV_MINE` | `Junqi.Move` | 工兵挖雷（不结束对局，红有子则 stage 保持 PLAY） |

- **同名提示（AUD-R-062）**：`EV_MINE`(5) 与 `EV_MOVE`(0) 共用 "Junqi.Move" 事件名——对齐原版 32 位
  .rodata 设计（挖雷不结束对局，音效按名分流）；调用方按**事件码**分流统计（`g_event_cnt[0]` 与
  `[5]` 分开计数），按**事件名**分流音效。
- **返回契约**：`jq_try_move` 返回事件码（0-5），**-1 = 非法**（不改变状态）。`jq_flip` 返回 bool
  （翻棋不走 jq_try_move 事件码通道，EV_FLIP=6 计数由调用点承担，AUD-R-061）。
- **事件计数**：`g_event_cnt[7]` + `g_event_total`，OnTouch 直接走子也要递增（不经 on_event）。

### 3.1 原版事件分发全链（§124y-34~123 逆向实锤，对齐参考）

**原版 .so 事件分发 = 两层编码机制**（src_rs 简化的 `jq_try_move` 0-5 事件码 仅对齐层 1 核心）：

```
事件对象 [0x35] 类型 ≠ 0
  ├─ 层 1（直接码匹配）: [0x44] 直接事件码 0..16 → 事件表 .bss 0x4704 区 17×17 位掩码
  │    (F_3a92c vtable 虚调, 字段 asrs#4 = [0x44], §124y-109/112)
  │
  └─ 层 2（枚举分发）: C++ 枚举事件类型 (如 155=0x9b) → 编码 ×2 (310=0x136)
       → F_55700 switch cmp 阈值 + PC 跳转表 (§124y-120/123)
       → thunk 体 4 模式 (直接/寄存器搬移/带条件/多字段批量, §124y-117)
       → 55 code (0-41+53-65) → 24 handler 子状态展开 (§124y-115)
       → 写事件对象 [4] 字段 (C++ 事件类型存储, §124y-123)
```

**原版 76 事件名 10 业务大类**（★ AUD-1000 B09-005 口径统一：§124y-163 实锤
.rodata 0x120xxx 区 Junqi 73 + Misc 3 = 76 条，EXPERIENCE_LOG 单一口径=76；
旧"74 事件名"（§124y-114 早期 10 大类求和 74）已被 §124y-163 取代，头注保留
"74→76 口径演变"防复发）：
- 走子/吃子/翻牌 7 + 胜负/终局 7 + 悔棋 6 + 和棋 5 + 投降 2 + 登录/账号 10
  + 注册 12 + 联机 11 + 摆子/布局 8 + 系统/杂项 4 + Misc 去广告 2 = 74 条
  （§124y-114 大类求和；.rodata 全量实测 76 = Junqi 73 + Misc 3，
  差 2 条 = Misc 区补录，§124y-163 事件名映射固化）
- 76 事件名 = 17 事件码（0..16）× 子状态命名枚举（多对一映射，§124y-115）

**src_rs 简化对齐边界**（§124y-124 固化）：
- src_rs 的 `jq_try_move` 0-5 事件码 = 层 1 核心 7 类（Move/Eat/Bomb/Flag/Dead/Flip/Mine）
  的简化实现（非原版 17 事件码 + 55 code 全量）
- 原版 76 事件名（★ B09-006 与 §3.1 口径同步：74→76 已被 §124y-163 取代）（登录/注册/联机/摆子类）= 账号/网络/布局子系统，src_rs 未对齐（§AGENTS.md 红线"简化版"诚实边界）
- 原版事件分发两层编码（[0x44] 直接码 + C++ 枚举 155）= 逆向实锤认知，src_rs 用简化事件码契约（0-5）


## 4. UI 流程

```
UI_MENU（5 按钮：人机/两人/联网/联机/四国）
  → 点按钮 action 0/4 → UI_GAME + 开局（jq_restart_game：default.lay + 明棋 + stage=PLAY 蓝先手）
  → 对局：两步走子态机（点格 A 选中 → 点格 B 走子/翻棋）；可走格绿框高亮
  → 对局结束（stage=OVER）：胜负横幅（winner 蓝/红/和棋灰）+ "再来一局"按钮
  → 点"再来一局"/左上角重开按钮/restartgame → jq_restart_game（不重启 app）
```

- 菜单 5 按钮几何 720×1280 基准（`ui_menu_buttons`），触摸命中归一化 [0,1]²。
- 触摸坐标：view=900×1468，native_y=screen_y-132，x=0.366×900+(col+0.5)×47.7，row 中心 native_y=1468−(row+0.5)×122.33。
- 悔棋：`undo_push`（完整 jq_state 快照）/`undo_pop`；**连续 pop 到 turn==BLUE**（撤销 AI 应着 + 玩家走子）。

## 5. 音效映射（14 WAV）

| 组 | 音效 | 触发 | 入口 |
|---|---|---|---|
| Junqi 12 | move/eat/bomb/flag/dead/flip | 走子事件分类 | `sfx_for_move(ev)` |
| Junqi | win/lose/draw | 对局结局（winner 0/1/2） | `sfx_game_over(stage, winner)` |
| Junqi | start/over/select | 开局/重开/选中 | `sfx_play` 直接 |
| UI 2 | close/pop | 弹窗关闭/按钮点击 | `sfx_play` 直接 |

- WAV 源三路径（优先级 ①>②>③）：① 内部存储 `getFilesDir/jq_sfx/`（OnCreate 反射落地，
  sfx_wav_ensure_private 14 WAV 拷贝，纯本地读免 AAsset 锁竞争 SIGSEGV）② APK
  `assets/jq_sfx/`（AAsset_openFileDescriptor 一次性拿 fd，g_assets 可达时）③ 文件
  系统 `/data/local/tmp/jq_sfx/`（MuMu/桌面回退，sfx_wav 返回路径）。
- 音效持久化（★ 对抗审计 P0-R10 闭环）：`g_sound_on` 总开关存
  `getFilesDir/jq_sound_on.txt`（OnCreate sfx_sound_load、☰ 功能设置"确定"按钮
  sfx_sound_save 落盘，接入 sfx_play 门控——原死代码已修）；静音模式
  （ui_snd_mute）走 `jq_settings.txt` 双轨——g_sound_on=声音总开关、
  ui_snd_mute=弹窗内静音切换，sfx_play 两者叠加门控。
- **音效三入口各司其职**：走子事件走 sfx_for_move、结局走 sfx_game_over、菜单/UI 走 sfx_play——勿散点。

## 6. 蓝牙联机（行为级约定，非 C++ 类逆向）

- UUID `ce1404fe-0863-4278-86c2-c9c560f0e215` + 服务名 `junqi` + 版本 v2。
- 7 字段帧 `v2\n%d,%d,%d,%d,%d,%d,%d\n`（type,fc,fr,tc,tr,rank,side）+ 60 格全盘同步。
- ★ B04-002 定案：ev（事件码）不编入第 7 字段 f7——f7 为兼容旧调用方保留位（恒 0 透传），
  事件分类走 jni_bridge 收帧侧 `jq_try_move` 返回值映射（原版 32位 帧格式无 ev 槽，
  7 字段定案=实证 0x26f5e-0x26f84 同步循环 + 帧格式逆向）。
- 帧状态机 `bt_session`：7 种帧类型回合闸口 + 坐标校验 + `jq_try_move` 验证对方走子 + 回合翻转。
- 诚实边界：SPP socket IO 由 Java 宿主承担，无真机蓝牙验证；7 字段语义为行为级约定。

## 7. 对局状态

- `jq_state`：`cells[60]`（rank/side/revealed/flags）+ `turn`（0=蓝 1=红）+ `stage`（PLAY/OVER）+ `winner`（-1/0/1/2）+ `moves_since_eat`/`total_moves`。
- `jq_cell`：rank（raw）/side/revealed/flags（JQF_MINE/JQF_FLAG）。
- 和棋判据：`moves_since_eat >= 70 || total_moves >= 1000`（走子后自动检查）。
- 困毙判据：`jq_side_stalemated(st, turn)`（走子后自动检查）。

## 8. 诚实边界（红线）

- 渲染/音频/蓝牙 SPP socket 为桩（OnCreate 返回 NULL，Java 侧 new GLView 兜底）。
- 7 字段帧语义为行为级约定（非原版 C++ 类布局逐字段逆向）。
- "能玩"需真机 GPU 设备验收（MuMu GLES1 几何出帧受限，PBuffer 离屏已证、window surface 通路未通）。
- 不包装成"完整版/可玩"——本规格描述已对齐实现，非独立设计稿。
