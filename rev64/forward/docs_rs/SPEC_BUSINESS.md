# 军棋业务规格说明书（src_rs 正向开发基准）

> 定位：本文件是 **src_rs 正向开发的业务规格**（玩法/规则/UI 流程/事件/音效的系统描述），
> 从 `R-route_reversal.md` 逆向证据链 + `CONSISTENCY.md` 一致测试口径 + 经验库沉淀提炼。
> 后续正向开发（UI/音效/规则完善/联机）以本规格为基准，**规格变更必须同步代码 + 回归**。
>
> 诚实边界：src_rs 是**逆向驱动重写**（规则引擎对齐原版 0x39e14/0x39f20/0x302b4 等），
> 非规格先行的纯正向设计；本规格是"**从已对齐实现反推 + 验证**"的业务描述，非独立设计稿。
> ★ **口径总表（R7-D-33 单一事实源，防多文档漂移）**：
> | 口径 | 真值 | 基准/复现命令 |
> |---|---|---|
> | 事件名全集 | **76 = Junqi 73 + Misc 3**（Misc 3 归广告拆除红线，不入门禁） | `python3 rev64/tools/event_code_map_check.py`（0 缺口 PASS） |
> | 门禁主链 | **73 Junqi 名**（`jq_event_names_73.c`，含 `Junqi.GameType%d` 格式串形态） | `python3 rev64/tools/event_names_alignment_check.py`（PASS） |
> | 历史口径 | "74 名"= §124y-114 早期 10 大类求和快照（Misc 区仅 2 条时期） | 仅作演变注，不作判据 |
> | evverify 断言 | **174/174（事件/业务 42 + 几何 132）**（★ R32-Q56/Q66/Q93 快照修正：旧 172/172 为 R27 前历史快照，+run_switch_matrix(R27-Q92)+run_evt55_names_smoke(R27-Q68)=42 事件项；与 §8 SSOT 表 L190/L197 一致） | `cd rev64/forward/src_rs && ./dist/jq_evverify --all`（SUMMARY 动态真值） |
> | 双端探针 | **36 case**（基线 4 坐实分叉 R-073/008/004/018 内不阻断） | `python3 rev64/tools/rule_probe_compare.py --offline` |
> | SPAK 资源 | 原版解包全量 **18 WAV / 883 资源**；仓内实物 **14 WAV / 970 文件**（spak_res 交付口径） | `ls rev64/dist/spak_res/*.wav \| wc -l` |
> | 暗棋桌面用例 | **86/86**（含判据自检） | `bash rev64/forward/src_rs/dark_align/dark_align_regress.sh` |


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
| 军旗 | 明棋：任何已翻子扛旗即胜（rank 12）；**暗棋（mode=FLIP && cfg4=1 前置门槛）：仅工兵（rank 9）且已挖满 3 雷（mines_cleared[me]>=3）可扛旗**——★ R26-Q4 + R32-Q2 文档修正（cfg3/cfg4 职责分标：cfg4=暗棋雷/旗前置门槛（工兵+挖3雷），cfg3=吃旗开关控扛旗是否判胜（关则入位不终局），旧条款混写"cfg3 控扛旗"未区分双开关） | 0x39e14 军旗特判（rules.c:325 cfg4 暗棋工兵+挖3雷门槛 + rules.c:753-795 cfg3 吃旗开关控判胜，双开关交互） |
| 走子 | 铁路（十字 + 双方底线 row0/11）直线任意格；普通格一步 | `is_rail`（board.c） |
| 翻棋 | 点未翻己方子 → 翻开（暗棋玩法）；明棋全子 revealed=1 | 0x39f20 from==to→翻棋 |
| 暗棋未翻目标 | 暗棋（mode=FLIP）下未翻普通暗子（rank 1-9 revealed=0）**可撞**（翻出子可走/吃到未翻暗子，撞子即翻暴露 rank）——★ R26-Q7/Q89 文档修正（2026-11-13 移除旧 B01-053"不可撞"限制，对齐原版 0x6a45c：翻出子可走路线不少于原版，用户"翻棋后可走路线少"指正）。旧 B01-053"未翻目标不可撞"为 AUD-1000 加的"防信息泄露"，已移除（信息不泄露非原版 0x6a45c 语义） | 0x6a45c 实证（rules.c:283-291 移除注释），evverify 暗棋基准已同步 |
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
  （★ R7-D-24/25 口径定案：§124y-114 早期 10 大类**求和 74 = 当时快照**
  （Misc 区早期仅 2 条）；.rodata 0x120xxx 全量实测**当前真值 76**
  = Junqi 73 + Misc 3（Misc 区后补 1 条，§124y-163 事件名映射固化）。
  单一口径锚点：**76 = 73 Junqi + 3 Misc**；"74" 仅历史演变注保留防复发。
  门禁真值 = 73 Junqi 主链（`event_names_alignment_check.py` PASS，
  Misc 3 条归广告拆除红线不进门禁）。
- 76 事件名 = 17 事件码（0..16）× 子状态命名枚举（多对一映射，§124y-115）

**src_rs 简化对齐边界**（§124y-124 固化）：
- src_rs 的 `jq_try_move` 0-5 事件码 = 层 1 核心 7 类（Move/Eat/Bomb/Flag/Dead/Flip/Mine）
  的简化实现（非原版 17 事件码 + 55 code 全量）
- 原版 76 事件名（★ B09-006 与 §3.1 口径同步：74→76 已被 §124y-163 取代）（登录/注册/联机/摆子类）= 账号/网络/布局子系统，src_rs 未对齐（AGENTS.md 顶层红线"简化版"诚实边界, ★ R20 修正旧 "§AGENTS.md" 前缀——AGENTS.md 无 § 章节）
- 原版事件分发两层编码（[0x44] 直接码 + C++ 枚举 155）= 逆向实锤认知，src_rs 用简化事件码契约（0-5）


## 4. UI 流程

```
UI_MENU（5 按钮：人机/两人/联网/联机/四国）
  → 点按钮 action 0/1/2/3 → UI_GAME + 开局（jq_restart_game：default.lay + 暗棋（g_opt_mode=1
     默认，ui_state.c:31）+ stage=PLAY 蓝先手；明棋/暗棋可在功能设置 ☰ 切换）
  → 点按钮 action 4（四国）→ UI_NPLAYER 人数选择弹窗（datah_089 2/3/4 人），
     选人后显示诚实边界对话框（4 方规则未实现，不静默开 2 人局）
  → 对局：两步走子态机（点格 A 选中 → 点格 B 走子/翻棋）；可走格绿框高亮
  → 对局结束（stage=OVER）：胜负横幅（winner 蓝/红/和棋灰）+ "再来一局"按钮
  → 点"再来一局"/左上角重开按钮/restartgame → jq_restart_game（不重启 app）
```

- 菜单 5 按钮几何 720×1280 基准（`ui_menu_buttons`），触摸命中归一化 [0,1]²。
- 触摸坐标：★ 旧换算链（view=900×1468, native_y=screen_y-132, x=0.366×900+(col+0.5)×47.7）**已作废**
  （ui_menu.c:29 标注, 2026-11-17 R18 同步）；现行 = `rs_ui_scale` 等比 `s=min(vw/720,vh/1280)`
  + yoff（render.c:190）+ 触摸同源公式 `g_touch_s/g_touch_yoff`（jni_bridge.c R8-F-36/37）。
- 悔棋：`undo_push`（完整 jq_state 快照）/`undo_pop`；**连续 pop 到 turn==BLUE**（撤销 AI 应着 + 玩家走子）。

## 5. 音效映射（14 WAV）

> ★ AUD-1000-ALL A6-02（P2 标注）：`audio.c`（`jq_audio_init`/`jq_wav_load`/`jq_audio_play`
>   14 WAV 表）为**未接线死模块**——全仓 0 调用点、Makefile 未纳入任何 target。
>   实际音频通路 = `jni_bridge.c` `sfx_wav()` 14 项 strcmp 双源 + `sfx_for_move(ev)`
>   事件码分流。本 §5 描述的是 **sfx_wav/sfx_for_move 实际通路**，audio.c 为
>   早期设计残留（诚实边界：勿将 audio.c 当真通路）。

| 组 | 音效 | 触发 | 入口 |
|---|---|---|---|
| Junqi 12 | move/eat/bomb/flag/dead/flip | 走子事件分类 | `sfx_for_move(ev)` |
| Junqi | win/lose/draw | 对局结局（winner 0/1/2） | `sfx_game_over(stage, winner)` |
| Junqi | start/over/select | 开局/重开/选中 | `sfx_play` 直接 |
| UI 2 | close/pop | 弹窗关闭/按钮点击 | `sfx_play` 直接 |

- **音效分流单一事实源（★ 本轮）**：`sfx_for_move(ev)` 7 事件码 → 原版 73 事件名
  （`jq_event_names_73`）→ `jq_evt55_sfx_for_name`（`evt55_sfx.c` 集中式音效分类表）。
  原版按事件名分流音效（§5 音效分流节, ★ R18 修正死引用 "§5.2" 不存在）, 64 位 7 事件码映射到对应原版事件名后走同一分类表，
  消除"改分类表漏改硬编码"双源不一致风险。
- 诚实边界：`jq_evt55_sfx_for_name` 仅覆盖核心 7 类 + 胜负/和棋/投降 + 兜底 move；
  账号/注册/联机类 44 名原版无独立业务 WAV（兜底 move，不造新音效）。

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
- 帧状态机 `bt_session`：10 种帧类型（MOVE/FLIP/UNDO/SYNC_CELL/SYNC/RESTART/RESIGN/DRAW/CHAT/CAP，★ R32-Q34 快照修正：旧 7 种为 v2 基线，已扩 v3 双栈 10 类型）回合闸口 + 坐标校验 + `jq_try_move` 验证对方走子 + 回合翻转。
- 诚实边界：SPP socket IO 由 Java 宿主承担，无真机蓝牙验证；7 字段语义为行为级约定。

## 7. 对局状态

- `jq_state`：`cells[60]`（rank/side/revealed/flags）+ `turn`（0=蓝 1=红）+ `stage`（PLAY/OVER）+ `winner`（-1/0/1/2）+ `moves_since_eat`/`total_moves`。
- `jq_cell`：rank（raw）/side/revealed/flags（JQF_MINE/JQF_FLAG）。
- 和棋判据：`moves_since_eat >= 70 || total_moves >= 1000`（走子后自动检查）。
- 困毙判据：`jq_side_stalemated(st, turn)`（走子后自动检查）。

## 8. 诚实边界（红线）

- 渲染/音频为桩（OnCreate 返回 NULL，Java 侧 new GLView 兜底）。**★ R30-Q80 文档修正：蓝牙 SPP socket 非桩**——`BtSpp.java` 已接线 SPP 读写线程（host 监听/client 连接/帧重组/UNDO 收帧闭环）+ `App.smali` btHost/btClient/btSend 接线 + `jni_bridge.c` 发送/启动/CAP 全链；诚实边界 = **无 BT 硬件端到端验证**（桌面桩验证协议状态机，非联机可玩）。
- 7 字段帧语义为行为级约定（非原版 C++ 类布局逐字段逆向）。
- "能玩"需真机 GPU 设备验收（MuMu GLES1 几何出帧受限，PBuffer 离屏已证、window surface 通路未通）。
- 不包装成"完整版/可玩"——本规格描述已对齐实现，非独立设计稿。

**★ R31-Q34/Q92（P0 当前状态 SSOT + 边界登记表，防互斥结论并存/误判已对齐为已实现）**：

| 能力域 | 当前状态（2026-11-19） | 证据 | 诚实边界 |
|---|---|---|---|
| 规则引擎（走子/吃子/炸弹/雷/旗/翻棋/暗棋/困毙/和棋） | **已实现**（行为级对齐，非汇编级） | evverify 174 + dark_align 86 + rule_probe 36 case | 32/64 位基线 4 分叉（R-073/008/004/018）归因不阻断 |
| 事件分发（73 名→12 业务码→55 code） | **分发已对齐，业务未实现**（账号/联机 47 名无 g_state 变更） | run_evt55_names_smoke 73/73 + jq_evt55_no_state_change（R31-Q12） | 分发成功≠业务实现（evt55_do_code 恒返 1，无状态变更） |
| 渲染（GL2 window 出屏/EGL 自愈/stage 门控） | **已接线，视觉未真机验证** | glview_gl window 路径 + R25 EGL 60 帧自愈 + R26 stage 门控 | 非黑比≠棋盘几何出屏（设备 PENDING，MuMu GLES1 受限） |
| 音频（WAV 解码/三路径优先级） | **已接线，实播真机未验** | jni_bridge sfx_wav 三路径 + 14 WAV 落地 | 解码库能力 vs JNI 实播能力分层（无硬件端到端） |
| 蓝牙 SPP（host/client/帧重组/UNDO 闭环） | **已接线，硬件端到端 PENDING** | BtSpp.java 读写线程 + jni_bridge 发送/启动/CAP | 无 BT 硬件验证（桌面桩=协议状态机，非联机可玩） |
| 四国军棋/联网对战/账号/版本更新/本地提和/联机聊天 | **未实现**（入口存在→边界对话框） | jni_bridge/render 多处"边界对话框/占位帧" | UI 可达≠功能可用（分发对齐/协议对齐≠业务闭环） |
| 存档（one.sjq/one.sjg） | **行为级重建，非原版逐字节** | save.c 裸 sizeof 序列化 + magic/size 校验 | 非原版兼容（混合版本互读未回归） |
| 桌面 vs 设备验证分层 | **桌面 0 回归≠设备 100% 达成** | evverify 174 + bt_semtest 152 + dual 43 | 设备冒烟 5 判据 PENDING（10.0.182.146 No route to host） |

**口径红线（防误判"已对齐"为"已实现"）**：
- 行为级对齐 ≠ 汇编级等价（原版 .text 1,037,940B > 重写 728,584B，信息论下界）
- 分发成功 ≠ 业务实现（evt55 47 名无 g_state 变更，jq_evt55_no_state_change=1 防误判）
- 桌面 0 回归 ≠ 设备达成（board_px 非黑比需真机截图，PENDING）
- 修复 ≠ 不复发（已修项以代码实证为准，非注释宣称；EGL 自愈/stage 门控/0x502/纹理泄漏 4 项防复发需真机长跑）

### 3.2 原版行为移植（本轮落地）

**原版 73 事件名常量表（`jq_event_names_73.h/.c`）**：
- 从 32 位 so `.rodata` 0x120xxx 区提取（frida-dump/rodata_strings.txt 实锤 73 个 Junqi.*）
- 12 事件码（0-11）映射：走子/吃子(0) / 炸弹旗死(1) / 翻棋(2) / 胜负(3) / 和棋(4) / 投降(5) / 悔棋(6) / 登录(7) / 注册(8) / 联机(9) / 摆子布局(10) / 系统杂项(11)
- code 12-16 预留（SPEC §3.1 口径 17 事件码，实测 73 名归 12 大类）

**64 位 `on_event` 新增 11 分支（原版行为移植）**：
| 事件名 | 64 位处理 | 原版语义 | 诚实边界 |
|---|---|---|---|
| regret_req | 发 UNDO 帧 | Junqi.RegretReq | 本地不 pop，等收帧闭环 |
| regret_agree | 收帧侧 undo_pop | Junqi.RegretAgree | 简化版不独立处理同意帧 |
| regret_disagree | 无操作 | Junqi.RegretDisagree | 同上 |
| draw_req | 发 DRAW 帧 | Junqi.DrawReq | 本地提和判定未实现 |
| draw_agree | jq_on_draw 和棋 | Junqi.DrawAgree | 需对端协议（简化版本地判定） |
| draw_disagree | 无操作 | Junqi.DrawDisagree | 同上 |
| join_game | 边界对话框 | Junqi.JoinGame | 联机服务器协议未逆向 |
| leave_game | 退对局回主菜单 | Junqi.LeaveGame | — |
| game_over | 终局确认 | Junqi.GameOver | — |
| resign_confirm | 本机败+RESIGN 帧 | Junqi.ResignConfirm | — |

**音效分流对齐**：`sfx_for_event_name()` 按 73 事件名首字段映射 WAV（Move/Eat/Bomb/Flag/Dead/Flip 6 类，其余兜底 move）；账号/联机类无独立音效（诚实边界）。
★ 2026-11-17 R17 注: `sfx_for_event_name` 已删 (L242 3 死函数清理, 现行 = `jq_evt55_sfx_for_name`), 本段保留为设计演变记录。

**事件计数**：`g_event_cnt` 7→73（对齐 `jq_event_names_73` 契约）；战况统计 UI 显示 7 事件码核心计数 + 73 事件名非零分布（账号/联机类恒 0 不刷屏）。

**红线（不包装）**：73 事件名 = 常量表对齐 + 核心 7 类行为移植；账号/注册/联机类 66 名 = 诚实边界（业务逻辑未移植，仅常量表对齐），简化版 ≠ 完整版。

### 3.3 55 code→24 handler 全链分发引擎（本轮落地）

**移植范围**：32 位 so `F_55d40` switch（§124y-29/31/33/35 实锤 67 case）→ 64 位 `evt55_dispatch.c`。

**核心设计**：
- 64 位 handler 操作 `g_state`（真实游戏状态），不复刻 32 位 .bss C++ 事件单例（诚实边界）
- 全 67 code 均返回成功（`g_state` 变更在 `rules.c`/`jni_bridge.c` 各调用点完成，`evt55_dispatch` 幂等分发确认）
- 账号/注册/联机类 53 code = 分发成功（无状态变更，简化版不实现服务器协议）
- 核心 7 类（code 0-7）→ 73 事件名索引（Move/Eat/Bomb/Flag/Dead/Mine/Flip）→ `g_event_cnt` 跨文件计数

**事件计数 7→73 扩容对齐**：
- `g_event_cnt` 去 static（`jni_bridge.c` 定义，`evt55_dispatch.c` 跨文件计数）
- `ev7_to_name_idx()`：7 事件码 → 73 事件名索引映射（code 0-4 → name 0-4，code 5 MINE → name 0，code 6 FLIP → name 5）
- 全 7-code 直接计数路径（`OnTouch`/`OnUpdate`/AI 走子）统一走 `ev7_to_name_idx` 映射

**接入点**：
| 调用点 | code | 事件名 |
|---|---|---|
| on_event move | 0-4,7 | Junqi.Move/Eat/Bomb/Flag/Dead/Move(Mine) |
| on_event flip | 6 | Junqi.Flip |
| on_event regret_req | 42 | Junqi.RegretReq |
| on_event draw_req | 25 | Junqi.DrawReq |
| on_event draw_agree | 26 | Junqi.DrawAgree |
| on_event game_over | 16 | Junqi.GameOver |

**诚实边界**：55 code 全链 = 事件分发对齐（code→handler 映射 + 73 名计数）；账号/联机/摆子类 53 code 业务逻辑未移植（简化版不实现服务器协议），简化版 ≠ 完整版。

**§3.3 收尾（on_event 全分支 73 名计数对齐）**：
- resign→code 24 分发；resign_confirm→name idx 19 计数（ResignConfirm 无独立 55code，兜底 24）
- regret_agree/disagree→name idx 21/22 计数（同意/拒绝帧简化版不独立处理，仅计数对齐）
- join_game/leave_game→name idx 50/58 计数（联机/离开，业务逻辑未移植，73 名计数对齐）
- 3 死函数清理（sfx_for_event_name/event_count_by_name/ev_code_to_name 被 ev7_to_name_idx + jq_evt55_* 取代）

### 3.4 76 事件名 → 12 业务码 → 55 code 全链（原版行为移植深水区落地）

**`evt55_business.c`（本轮新增）**：
- 76 事件名（Junqi.* 73 + Misc 3）→ 12 业务码（原版 `event_code_map.json` 12 大类）→ 55 code（原版 F_55d40 switch 实锤 representative code）
- `jq_evt55_name_to_55code()`：事件名字符串 → 55 code（全链入口）
- `jq_evt55_name_to_biz()`：事件名 → 12 业务码（诊断/统计）
- `jq_evt55_name_to_code()` 改调 `jq_evt55_name_to_55code`（替代旧 11 项硬编码映射）

**12 业务码 → 55 code 映射**（representative，诚实边界）：
| 业务码 | 事件名族 | 55 code | 64 位 g_state 变更 |
|---|---|---|---|
| 0 走子/吃子 | Move/Eat/Dead | 0 | jq_try_move 已完成 |
| 1 炸弹/旗 | Bomb/Flag | 2 | jq_try_move 已完成 |
| 2 翻棋 | Flip | 6 | jq_flip 已完成 |
| 3 胜负/终局 | GameOver/Over/Win/Lose/RedWin/BlueWin | 16 | jq_on_draw/jq_on_stalemate 已完成 |
| 4 和棋 | Draw/DrawReq/Agree/Disagree/Confirm/Send | 25 | jq_on_draw 已完成 |
| 5 投降 | Resign/ResignConfirm | 24 | winner 翻转已完成 |
| 6 悔棋 | RegretReq/Agree/Disagree/Confirm/Send/Fail | 42 | undo_pop 已完成 |
| 7 登录 | Input*/Login*/Email*/User*/Name*/Pwd*/New*/Ori*/Reg* | 34 | 诚实边界：无 g_state 变更 |
| 8 注册 | RegExist/RegFail/RegOk 等 | 36 | 同上 |
| 9 联机 | JoinGame/Ready/Start/Peer*/LeaveGame/Player*/Reconn*/Connect*/Exit* | 38 | 同上 |
| 10 摆子/布局 | LayError/High*/Mid*/Zha*/Lei*/Qi* | 44 | 同上 |
| 11 系统/杂项 | Select/GameType/Over | 46 | 同上 |

**红线**：76 事件名全链 = 事件名→业务码→55code 分发对齐；账号/注册/联机/摆子类 53 code 业务逻辑未移植（无服务器协议 + 无 .bss C++ 事件单例），简化版 g_state 语义下 = 分发成功无状态变更，**不包装成完整版**。
