# 军棋业务规格说明书（src_rs 正向开发基准）

> 定位：本文件是 **src_rs 正向开发的业务规格**（玩法/规则/UI 流程/事件/音效的系统描述），
> 从 `R-route_reversal.md` 逆向证据链 + `CONSISTENCY.md` 一致测试口径 + 经验库沉淀提炼。
> 后续正向开发（UI/音效/规则完善/联机）以本规格为基准，**规格变更必须同步代码 + 回归**。
>
> 诚实边界：src_rs 是**逆向驱动重写**（规则引擎对齐原版 0x39e14/0x39f20/0x302b4 等），
> 非规格先行的纯正向设计；本规格是"**从已对齐实现反推 + 验证**"的业务描述，非独立设计稿。

## 1. 棋盘与棋子

- 棋盘 **10×9**（row 0-11 / col 0-8，中央 10×5 十字 + 双方底营），共 **60 格**（每方 30 格）。
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

- **返回契约**：`jq_try_move` 返回事件码（0-5），**-1 = 非法**（不改变状态）。
- **事件计数**：`g_event_cnt[6]` + `g_event_total`，OnTouch 直接走子也要递增（不经 on_event）。

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

**原版 74 事件名 10 业务大类**（§124y-114 实锤，.rodata 0x120xxx 区）：
- 走子/吃子/翻牌 7 + 胜负/终局 7 + 悔棋 6 + 和棋 5 + 投降 2 + 登录/账号 10
  + 注册 12 + 联机 11 + 摆子/布局 8 + 系统/杂项 4 + Misc 去广告 2 = 74 条
- 74 事件名 = 17 事件码（0..16）× 子状态命名枚举（多对一映射，§124y-115）

**src_rs 简化对齐边界**（§124y-124 固化）：
- src_rs 的 `jq_try_move` 0-5 事件码 = 层 1 核心 7 类（Move/Eat/Bomb/Flag/Dead/Flip/Mine）
  的简化实现（非原版 17 事件码 + 55 code 全量）
- 原版 74 事件名（登录/注册/联机/摆子类）= 账号/网络/布局子系统，src_rs 未对齐（§AGENTS.md 红线"简化版"诚实边界）
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

- WAV 源：`/data/local/tmp/jq_sfx/`（设备 14 WAV）+ APK assets/jq_sfx/（fallback）。
- **音效三入口各司其职**：走子事件走 sfx_for_move、结局走 sfx_game_over、菜单/UI 走 sfx_play——勿散点。

## 6. 蓝牙联机（行为级约定，非 C++ 类逆向）

- UUID `ce1404fe-0863-4278-86c2-c9c560f0e215` + 服务名 `junqi` + 版本 v2。
- 7 字段帧 `v2\n%d,%d,%d,%d,%d,%d,%d\n`（type,fc,fr,tc,tr,rank,side）+ 60 格全盘同步。
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
