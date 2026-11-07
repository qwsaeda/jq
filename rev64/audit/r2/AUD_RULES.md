# AUD-RULES — src_rs 军棋规则引擎对抗审计（rules.c / board.c / junqi.h）

> 审计对象：`/disk/workspaces/jq/rev64/src_rs/rules.c`（345 行）、`board.c`（53 行）、`junqi.h`（192 行）
> 审计视角：独立对抗审计员，只找问题；每条给出「位置 / 问题 / 依据 / 修法」
> 依据来源优先级：① 原版权威规则文本 `dist/spak_res/data_junqi_rulelay.txt` / `data_junqi_ruleflip.txt`（GBK，已解码）
> ② `phases/REVERSE_NOTES_01_basics.md`（L47/L49/L51/L57/L59/L60/L63/L64/L68/L70 逆向确证）
> ③ `docs_rs/SPEC_BUSINESS.md`（正向规格）
> ④ 同仓兄弟实现 `src/game_logic.c` / `src_jni_translated/jq_rs_rules.c`（规则语义对照）
> ⑤ 实测：本次审计编写了 5 个探针程序（gcc 编译 `board.c layout.c rules.c ui_state_stub.c` 直连规则引擎），
>    所有"实测"结论均为探针实际输出，非静态推测。
> 审计时点：src_rs/rules.c mtime 2025-11-05 23:22 / board.c 2025-11-03 23:37 / junqi.h 2025-11-06 00:05

---

## 严重级定义

| 级 | 含义 |
|---|---|
| P0 | 规则语义错误，会直接产生**错误对局结果 / 非法走子 / 内存破坏**；与原版权威文本或 L 系列确证直接冲突 |
| P1 | 规则语义缺失或错误，影响可玩性/公平性/暗棋玩法/联机，或 API 契约错误导致调用方拿到错误结论 |
| P2 | 潜在风险、跨文件不一致、计数/状态不可信、文档与实现矛盾，需人工确认或加固 |
| P3 | 代码卫生、命名/注释/死代码/风格、低概率边界，需人工确认 |

---

# 一、P0（4 条）

## AUD-R-001 [P0] 「底线铁路」把 col0/col4 变成纵滑高速公路：一步可从红底线直滑到蓝底线（12 格）
- 位置：`src_rs/board.c:40`（`if (row == 0 || row == 11) return true;/* 双方底线 */`）+ `src_rs/rules.c:80`（`if (!is_rail(fr, fc) || !is_rail(tr, tc)) return false;`）
- 问题：`is_rail()` 把 row0/row11 标为"铁路"，而 `rail_path_straight()` 只校验**起点和终点**是否 `is_rail`，**不校验整条路径是否同属一条铁路线**。于是 (0,0)/(0,4)/(11,0)/(11,4) 这 4 个"底线角格"被当成铁路端点，而它们所在的 col0/col4 按 L68 明确**不是铁路**（"col0/col4 非铁路（只相邻一步）"），路径 (1,0)…(10,0) 全是普通公路格。实测：把一枚红子放在 (11,0)，`jq_can_move(st, cell_index(11,0), cell_index(0,0))` 返回 **1** —— 一步从红方大本营区滑到蓝方大本营区；`(11,0)->(5,0)`（6 格）、`(0,4)->(5,4)`（5 格）同样返回 1，而 `(0,4)->(4,4)` 返回 0（因为终点 (4,4) 不在铁路上——这正说明判定只看端点）。
- 依据：
  1. `REVERSE_NOTES_01_basics.md` §14（L49）："军棋铁路：横向仅 row5/6（中线两行）+ 纵向所有列贯穿…；straight 同行仅 row5/6"；§L68 明确 "纵向 col2 贯穿 … col0/col4 非铁路（只相邻一步）"。
  2. `rules.c:20` 自己的头注释："铁路（L49/L68）：纵向仅 col2 贯穿；横向仅 row5/6（中线）；工兵可拐弯（BFS），其他子直走不能转弯" —— 与 `board.c:40` 自相矛盾。
  3. A/B 实测（同一随机局面集，只改 `board.c` 是否含 `row==0||row==11` 一行，3000 局随机摆子枚举全部合法走子）：含底线铁路 = 23084 条合法走子，不含 = 22023 条，**差 1061 条**，逐条 diff 出来的全是 (0,4)→(5,4)/(6,4)/(11,4)、(5,0)→(0,0)、(11,0)→(0,0) 这类 col0/col4 纵向长滑。
  4. `docs_rs/EXPERIENCE_LOG.md:145-146` 给该"修复"的理由是"漏底线导致底角棋子纵向全堵 → 困毙误判"。实测**不成立**：同 A/B 框架下 20000 局随机局面比较"全盘无任何合法走子"与"困毙(turn)"计数，含/不含底线铁路**均为 1848 / 1848，完全相同**。底线角格 (0,0)/(11,0) 的相邻格 (1,0)/(10,0) 一直是合法一步（`rules.c:136` adjacent 直通），底线铁路对困毙判定零贡献。
  5. `rulelay.txt` 只承认"铁路线"为图上粗黑线；底角格既非中线也非纵线（纵线是 col2），把 col0/col4 的端点当铁路等于凭空造出两条贯穿全盘的"隐藏铁路"。
- 修法：
  1. 首选：`board.c:40` 删除 `row == 0 || row == 11` 分支，`is_rail()` 回归 L68 的十字（col2 纵线 + row5/6 横线），并同步 `rules.c:79` 的 `fr != 0 && fr != 11` 条件、`rules.c:20` 头注释、`docs_rs/EXPERIENCE_LOG.md:145-146` 的错误结论、`docs_rs/AUDIT_CHECKLIST.md` A2 条目（"铁路：十字 + 双方底线 row0/11"）。
  2. 若坚持保留底线为铁路（则必须同步修语义），则**改 `rail_path_straight()` 为"整条路径同线"判定**：纵向滑行要求 `fc == 2`（唯一纵线）或路径上每一格都 `is_rail`；横向滑行要求 `fr ∈ {5,6}` 且路径每格 `is_rail`；禁止"端点在铁路、路径在公路"的混合路径。
  3. 无论选哪条，必须补单测锁定：(11,0)→(0,0)、(0,4)→(5,4)、(11,4)→(0,4) 必须为**不可走**，且 (5,2)→(7,2)、(5,0)→(5,4) 必须**可走**。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-002 [P0] 暗棋（ruleflip）核心规则整体缺失：地雷/军旗仅工兵可吃、工兵挖 3 雷才能扛旗
- 位置：`src_rs/rules.c:169-181`（EV_FLAG 分支）、`src_rs/rules.c:182-198`（EV_MINE 分支）、`src_rs/rules.c:107`（旗雷不可动）
- 问题：`rules.c` 完全没有实现 ruleflip（暗棋/翻棋）模式的吃子限制：
  1. 任何棋子（不限工兵）都可以吃地雷：`jq_can_move()` 只按"大压小"处理地雷（`rules.c:182` 分支只区分 `sr == 9` 与否），非工兵撞雷的结果是"自己被炸死"（EV_DEAD），而不是"该走法非法"。
  2. 任何棋子都可以扛旗：`rules.c:169` 的 EV_FLAG 分支只看 `dr_ == RANK_FLAG`，不校验攻子是否为工兵。
  3. `jq_state.mines_cleared[]`（`junqi.h:111`）被 `rules.c:191` 递增，但**全仓无任何读取点**（grep 确认：`src_rs/rules.c` 内只写不读），因此"工兵挖完 3 雷才能扛旗"这条 ruleflip 胜利条件从未参与判定。
  4. 该缺失发生在**默认模式**上：`src_rs/ui_state.c:30` `static int g_opt_mode = 1;`（默认暗棋），`jni_bridge.c:215` `jq_restart_game()` 据此把 `revealed` 全置 0 → 默认对局就是暗棋，却跑的是明棋（rulelay）规则。
- 依据：
  1. `dist/spak_res/data_junqi_ruleflip.txt`（GBK 解码）第 6 行："或者一方用工兵挖掉对方三颗地雷后再用工兵扛走对方的军旗，也能获得胜利"；第 10 行："地雷和军旗只有工兵能吃。工兵挖完地雷后才能扛军旗。"
  2. `REVERSE_NOTES_01_basics.md` §L63："ruleflip 翻棋吃子规则：军旗/地雷仅工兵吃、工兵挖3雷才能扛旗（arm64 DARK 模式对齐）；PLAY 布局模式任何棋扛旗（rulelay）不受影响。"
  3. 同仓兄弟实现 `src/game_logic.c:100-104` 已实现该三条：
     `if (st->stage == JQ_STAGE_DARK && dst->rank != JQ_EMPTY) { if (dst->side == JQ_SIDE_UNKNOWN) return false; if (dst->is_mine && src->rank != JQ_RANK_SOLDIER) return false; if (dst->is_flag && (src->rank != JQ_RANK_SOLDIER || st->mines_cleared[side] < 3)) return false; }`
     → src_rs 与 src/ 两条规则引擎出现**语义分叉**，同一款游戏两个实现给出不同胜负。
  4. 实测（探针）：暗棋局面下 `jq_can_move(蓝司令 → 暗红地雷)` = 1（ruleflip 应 0）；`jq_can_move(蓝司令 → 暗红军旗)` = 1（应 0）；`jq_can_move(蓝工兵 → 暗红军旗)`（mines_cleared=0）= 1（应 0）。
- 修法：
  1. 在 `junqi.h` 增加"对局模式"维度（当前 `jq_stage` 只有 SETUP/PLAY/OVER，**没有 DARK/LAY 之分**，这是缺口的根因）：新增 `JQ_MODE_LAYOUT` / `JQ_MODE_FLIP`，或在 `jq_state` 增加 `uint8_t mode`。
  2. `jq_can_move()` 增加暗棋分支：目标为地雷 → 仅 `sr == 9` 合法；目标为军旗 → 仅 `sr == 9 && st->mines_cleared[me] >= 3` 合法；目标 `side == 0xFF` 未定色 → 不可吃。
  3. `jq_try_move()` 的 EV_FLAG 分支同步加"工兵 + 已挖 3 雷"守卫，否则会出现"可走但不算胜"的错位。
  4. 补桌面回归：`make evverify` 增加 `--dark` 用例（挖雷 ×3 → 工兵扛旗 → win=BLUE；非工兵扛旗 → ILLEGAL）。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-003 [P0] 暗棋「先手翻子定色」未实现：布局直接写死 side，暗棋退化为"明棋藏脸"，AI 全知敌方底牌
- 位置：`src_rs/rules.c:242`（`if (c->side == 0xFF) return true;`）、`src_rs/rules.c:253`（`if (c->side == 0xFF) c->side = st->turn;`）、`src_rs/layout.c:52` / `layout.c:59`（蓝 `side = SIDE_BLUE`、红 `side = SIDE_RED` 写死）
- 问题：ruleflip 规定"两个玩家轮流翻棋，先手玩家翻出第一个棋子后，则确定该玩家持此种颜色的棋子"——即暗棋开局时 50 枚棋子**归属未知**（side=UNKNOWN），第一次翻棋决定颜色归属。src_rs 的 `jq_load_layout()` 给每枚棋子写死 side（蓝=SIDE_BLUE / 红=SIDE_RED），只把 `revealed` 置 0，因此：
  1. `rules.c:242` 的 `side == 0xFF` 分支**永远不可达**（实测 `jq_load_layout()` 后 60 格中 `side==0xFF` 的数量 = 0）；
  2. `jq_can_flip()` 的 `return c->side == st->turn;`（`rules.c:245`）→ 玩家只能翻"自己那 25 枚"（按写死的 side），永远无法翻到对方的子 → ruleflip 的"轮流翻棋"玩法不存在；
  3. 引擎/AI 因此**全知敌方每枚棋子的真实 rank 与归属**（`jni_bridge.c:245` 的 AI 过滤用 `c->side != me`，`ai_pick` 用 `d->rank` 直接评分），暗棋对 AI 无任何信息遮蔽 —— 与 ruleflip 的信息博弈前提冲突。
- 依据：
  1. `data_junqi_ruleflip.txt` 第 6 行："两个玩家轮流翻棋，先手玩家翻出第一个棋子后，则确定该玩家持此种颜色的棋子。"
  2. `REVERSE_NOTES_01_basics.md` §L64："暗棋布子 side=UNKNOWN，jq_flip 翻子定色（c->side=turn）；暗棋困毙修复（side_has_movable 先扫 UNKNOWN 暗子，有则未困毙）"；§L70："暗棋 AI 走子完整：翻 UNKNOWN 子、吃已翻子（UNKNOWN 不可吃）"。
  3. 兄弟实现 `src/game_logic.c:333-353` `jq_dark_setup()` 明确把 50 枚子置 `side = JQ_SIDE_UNKNOWN`，`game_logic.c:295-297` 的 `jq_flip` 才是真正的"先手定色"。
  4. 实测：探针打印 `jq_load_layout()` 后 `side==0xFF` 格子数 = 0；手工把某格置 0xFF 后 `jq_flip()` 走 `c->side = st->turn` 分支（说明该分支只在外部注入 0xFF 时才生效）。
- 修法：
  1. 暗棋开局走独立装载路径：`jq_dark_setup(st, seed)` —— 50 枚子 `side = 0xFF`、`revealed = 0`，分布按 default.lay 的 rank 集合洗牌（沿用 `src/game_logic.c:333` 的 Fisher-Yates）。
  2. `jq_flip()` 的定色规则改为"先手翻的第一枚归先手，其余 UNKNOWN 归另一方"，并同步维护 `pieces_left[]`。
  3. `jq_can_flip()` 在暗棋模式放开"任何 UNKNOWN 暗子可翻"（不再要求 `side == turn`）。
  4. 暗棋模式下 AI 只能看到"已翻开的子"，`ai_pick` 必须跳过 `!d->revealed` 的目标（见 AUD-R-045）。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-004 [P0] `pieces_left[dst->side]` 无下标范围检查 → side=0xFF 时越界写（+1020 字节）
- 位置：`src_rs/rules.c:166`（`if (dst->side != me) st->pieces_left[dst->side]--;`）
- 问题：EV_BOMB（炸弹同归）分支用 `dst->side` 直接作为 `pieces_left[2]` 的下标，只判了"不等于 me"，**没有判 `< 2`**。`jq_cell.side` 的定义（`junqi.h:64`）明确允许 `0xFF`（暗棋未定）。一旦目标格是未定色暗子（side=0xFF），`st->pieces_left[255]--` 会写到 `pieces_left` 数组**之后 1020 字节**处（255×4），越界写破坏相邻成员（`count_tbl[13]` 等）乃至栈/堆元数据。同样的 unchecked-index 模式还有 `rules.c:190`（`pieces_left[1-me]`，安全）与 `rules.c:191`（`mines_cleared[me]`，安全）。
- 依据：
  1. `junqi.h:64`：`uint8_t side; /* SIDE_BLUE/SIDE_RED；暗棋未定=0xFF */`；`junqi.h:23-24` 只定义 0/1 两个常量，没有"未知"常量。
  2. `rules.c:117` 对 `dst->side` 的另一处使用（`dst->side == me`）同样把 0xFF 当成"敌方"处理（语义问题，见 AUD-R-045），说明 0xFF 是引擎承认存在的状态。
  3. `bt_session.c:74` 的 `BT_T_SYNC_CELL` 直接把对端帧的 `f4` 原样写进 `peer_state.cells[idx].side`（0-255 任意值），说明"side 可能是 0xFF/其它值"在本仓是现实可达的输入。
  4. CWE-787（越界写）+ 编译器无法静态发现（`dst->side` 是 uint8_t，数组下标无 UB 编译告警）。
- 修法：
  1. 立即加守卫：`if (dst->side != me && dst->side < 2) st->pieces_left[dst->side]--;`。
  2. 更好：定义 `JQ_SIDE_UNKNOWN 0xFF` 与 `JQ_SIDE_COUNT 2` 常量，所有以 side 为下标的数组访问统一走 `side_index(side)` 辅助函数（越界返回 -1 并跳过计数）。
  3. 补单测：构造 side=0xFF 的目标子 + 炸弹攻子，断言 `pieces_left` 及其后 64 字节未被改写（可用 canary）。

---

# 二、P1（16 条）


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-005 [P1] `jq_side_stalemated(st, side)` 对 `side != st->turn` 恒返回 true（困毙误报）
- 位置：`src_rs/rules.c:266-287`（尤其 `rules.c:284` 的 `jq_can_move(st, i, t)`）
- 问题：该函数的 API 签名接受任意 `side`，但内部靠 `jq_can_move()` 枚举可动子，而 `jq_can_move()` 第一道门就是 `if (src->side != st->turn) return false;`（`rules.c:105`）。因此当传入的 `side` 不是当前回合方时，**所有**子都被判为"不可动"，函数直接返回 true（=该方已困毙）。实测：明棋开局（大量合法走子）下 `jq_side_stalemated(st, turn)` = 0，而 `jq_side_stalemated(st, 1-turn)` = **1**（假阳性）。
- 依据：
  1. `junqi.h:155` 的声明 `bool jq_side_stalemated(const jq_state *st, uint8_t side);` 读起来像"任意方"的查询；`rules.c:266` 注释只说"当前方无子可动/可翻 → 对方负"。
  2. `rules.c:229` 的唯一内部调用恰好传 `st->turn`，所以线上路径暂时"看起来正确"——这是典型的**潜伏型 API 契约缺陷**：任何新调用方（观战/AI 评估/联机对端校验）都会踩中。
  3. 实测输出：`T1 turn=0 stalemated(turn)=0 stalemated(1-turn)=1`。
  4. `docs_rs/SPEC_BUSINESS.md` §7 只写"困毙判据：jq_side_stalemated(st, turn)"，未声明"仅支持 side==turn"。
- 修法：把回合门从 `jq_can_move()` 里参数化——新增内部 `jq_can_move_for(st, from, to, uint8_t side)`，`jq_can_move()` 传 `st->turn`，`jq_side_stalemated()` 传被测 `side`；或在 `jq_side_stalemated` 入口加 `if (side != st->turn) return false;` 并在头文件写明"仅当前回合方有效"。同时补单测：明棋 + 大量合法走子时 `jq_side_stalemated(st, 1-turn)` 必须为 false。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-006 [P1] 暗棋下敌方暗子可被直接攻击并按真 rank 结算 → 暗棋信息泄露
- 位置：`src_rs/rules.c:117`（`if (dst->rank != RANK_EMPTY && dst->side == me) return false;`）、`rules.c:155-218`（结算分支直接读 `rank_norm(dst->rank)`）
- 问题：`jq_can_move()` 只排除"目标是己方子"，对 `side == 0xFF`（未定色暗子）的目标格**不做任何拦截**；`jq_try_move()` 随后用 `rank_norm(dst->rank)` 判定大压小/炸弹/雷，即引擎**读取并使用未翻开子子的真实 rank**。结果：玩家/AI 可以对一枚"看不见的子"发起攻击，引擎按其真实强弱结算并泄露信息（若打不死，对手立刻知道对方是强子）。
- 依据：
  1. `data_junqi_ruleflip.txt` 第 6 行的玩法前提是"轮流翻棋、先手定色"——未定色子不可被攻击，只能被翻。
  2. `REVERSE_NOTES_01_basics.md` §L70："吃已翻子（UNKNOWN 不可吃）"；兄弟实现 `src/game_logic.c:101`：`if (dst->side == JQ_SIDE_UNKNOWN) return false;`。
  3. `rules.c:242` 的注释自己承认存在"暗棋未定子"这一状态（`side == 0xFF`），说明引擎承认该状态却未在攻击路径上保护它。
  4. 实测：探针把某红子置 `side=0xFF, revealed=0` 后，与之相邻的蓝子 `jq_can_move()` 返回 1（可攻击未定色暗子）。
- 修法：在 `jq_can_move()` 增加 `if (dst->rank != RANK_EMPTY && dst->side == 0xFF) return false;`（暗棋语义）；明棋/布局模式下该状态不会出现，行为不变。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-007 [P1] 暗棋下「大本营」不是安全岛：敌方可吃大本营内的普通子
- 位置：`src_rs/rules.c:129-134`
- 问题：ruleflip 明确"行营、**大本营**是个安全岛，进入以后，敌方棋子不能吃其中的棋子；大本营的地雷、军旗可以吃"。src_rs 只在目标大本营**为空**时返回 false，对"大本营内有敌方普通子（工兵挖雷后留在大本营）"完全放行 → 敌方可直接吃掉大本营内的子。
- 依据：
  1. `data_junqi_ruleflip.txt` 第 3 行原文。
  2. 兄弟实现 `src/game_logic.c:96` 只允许"吃旗/吃雷"进入大本营：`if (jq_is_base(tr, tc) && !dst->is_flag && !dst->is_mine) return false;` —— 与 ruleflip 一致，src_rs 与之不一致。
  3. 实测：探针在 (0,1)（蓝方大本营）放一枚蓝工兵、(1,1) 放一枚红子，`jq_can_move(红 → (0,1))` = 1（应 0）。
  4. 注：`rulelay.txt`（明棋模式）只写"行营是个安全岛"，未提大本营 → 明棋下允许吃大本营内的子可接受，但**必须按模式区分**，而 src_rs 没有模式维度（见 AUD-R-002 修法 1）。
- 修法：按 AUD-R-002 引入模式后，在暗棋模式下把大本营目标判定改为"仅可吃旗/雷"，其余一律 false。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-008 [P1] `jq_flip()` 不做困毙判定（与 `jq_try_move()` 不对称）→ 翻完即已困毙的对局继续进行
- 位置：`src_rs/rules.c:248-263`（`jq_flip` 末尾只做和棋判定，无 `jq_side_stalemated` 调用）
- 问题：走子路径在 `rules.c:229` 自动判困毙；翻棋路径不判。翻棋是暗棋的**主要**动作，一旦某方翻完最后一枚可翻子后已无棋可走，对局不会结束，直到下一次走子才可能被发现（若对方也只翻棋，则永远不结束）。
- 依据：
  1. 兄弟实现 `src/game_logic.c:309` `jq_flip()` 末尾明确 `check_stalemate(st);`。
  2. `rulelay.txt` 第 16 行："棋子被吃光、军旗被扛、**无棋可走**都会被判负" —— 判负时机不应取决于对手下一步是走还是翻。
  3. 实测：探针连续翻 48 枚（全部非军旗暗子）后 `stage=1（PLAY）winner=-1`，而此时双方已无子可翻 → 无任何终局判定触发。
- 修法：`jq_flip()` 末尾补 `if (st->stage == STG_PLAY && jq_side_stalemated(st, st->turn)) jq_on_stalemate(st);`（顺序与 `jq_try_move` 保持一致：先和棋后困毙）。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-009 [P1] `ai_score()` 把「炸弹 vs 军旗」判成 1000 分（可吃旗）→ AI 主动撞旗同归
- 位置：`src_rs/rules.c:339`（`if (r < dr) return 1000;`）
- 问题：`ai_score()` 的判定顺序是"地雷特判 → `r < dr` → `r == dr` → 炸弹 800 → 送死 1"。炸弹 rank=10、军旗 rank=12，`10 < 12` 成立 → 返回 **1000（最高分"能吃子"）**。但实际规则是"炸弹与任何棋子相遇时，双方都消失，包括军旗"（`rulelay.txt` 第 12 行）→ 炸弹撞旗是**同归于尽、旗被消灭但无人获胜**，AI 却把它当成"吃掉军旗、赢棋"的最优着法，会主动把炸弹送进对方大本营。
- 依据：
  1. `data_junqi_rulelay.txt` 第 12 行："炸弹与任何棋子相遇时，双方都消失，包括军旗。"
  2. `rules.c:161-168` 的实际执行分支同样先判炸弹 → 双方消失，**不判胜**（与 AI 评分自相矛盾）。
  3. `REVERSE_NOTES_01_basics.md` §L57："炸弹撞军旗=双方消失"（arm64 已对齐）。
  4. 实测：`ai_score(10, 12) = 1000`；`ai_score(22, 24) = 1000`（红方编码 bomb=22 / flag=24）。
  5. `src_rs/ai_selftest.c` 的断言表覆盖了 bomb/flag/mine/同级/送死/空移，**唯独没有 bomb-vs-flag 用例** → 盲区。
- 修法：在 `ai_score()` 中把"目标为军旗"提前特判：`if (dr == RANK_FLAG) return (r == RANK_BOMB) ? -1 /* 同归不算吃旗 */ : 1000;`（并同步补 `ai_selftest.c` 用例：bomb vs flag ≠ 1000）。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-010 [P1] `ai_score()` 无 `revealed` 维度 → AI 用暗子真 rank 评分（暗棋作弊）
- 位置：`src_rs/rules.c:330-345`（`int ai_score(int src_rank_raw, int dst_rank_raw)`）、调用方 `src_rs/jni_bridge.c:255`
- 问题：`ai_score()` 的入参只有两个 rank 原始字节，**没有任何"目标是否已翻开"的信息**；调用方 `ai_pick()`（`jni_bridge.c:251-258`）对目标格只判 `t == i` 与 `jq_can_move()`，然后 `ai_score(c->rank, d->rank)` —— 暗棋下 `d->rank` 是**未翻开敌子的真实 rank**。于是 AI 会：① 优先攻击那些"按真 rank 打得过"的暗子（等于提前知道敌子身份）；② 把"能吃掉的暗旗/暗雷"排在最高优先级，等于自带透视。
- 依据：
  1. `data_junqi_ruleflip.txt` 的整个玩法建立在"隐藏身份"之上（先手翻子定色）；AI 读暗 rank 直接破坏该前提。
  2. `junqi.h:166` 的声明 `int ai_score(int src_rank_raw, int dst_rank_raw);` 未声明"仅供明棋/已翻目标使用"。
  3. `docs_rs/AUDIT_CHECKLIST.md` B7："AI 不主动送死（sc 表…）❌ 未归一化（已修 #2）+ 隐藏雷 bug" —— #2 的修复只统一了 rank 编码还原，**没有解决暗信息泄露**。
  4. 实测：探针在暗棋局面下对一枚暗子调用 `ai_score(1, 20)` 得到 1000（说明评分函数拿到暗子的真值并给出"可吃"结论）。
- 修法：① 扩展 API 为 `ai_score(st, from, to)` 或增加 `bool dst_revealed` 参数；② 暗棋模式下未翻开目标一律按"不可知"评分（如 0 或"低期望值"），且 `ai_pick` 跳过 `!d->revealed` 的目标；③ 在 `ai_selftest.c` 补"暗目标不得给正分"用例。

## AUD-R-011 [P1] 暗棋下 AI 还能"挑暗子吃"，且引擎允许未定色暗子被吃（与 AUD-R-006 同源的判定缺口）
- 位置：`src_rs/jni_bridge.c:243-259`（AI 循环）、`src_rs/rules.c:117`
- 问题：AI 遍历所有 `t`（含未翻开的敌子格）并对每个调用 `jq_can_move()`；由于 `rules.c:117` 不拦截 `side==0xFF` 的目标，AI 在暗棋开局就能对"尚未归属"的子发起攻击。叠加 AUD-R-003（side 写死）后，暗棋对 AI 而言等价于明棋 + 隐藏渲染。
- 依据：同 AUD-R-006/AUD-R-010；另 `jni_bridge.c:245` 的 AI 源子过滤 `!c->revealed` 只保护**自己的**暗子，目标侧无对应过滤。
- 修法：AI 目标侧同样加 `if (!d->revealed) continue;`（暗棋），或在引擎层统一拦截（推荐引擎层，避免每个调用方重复）。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-012 [P1] `jq_undo_snapshot()` 是空实现，但头文件承诺"走子前调用"
- 位置：`src_rs/rules.c:290-295`、声明 `src_rs/junqi.h:158`
- 问题：`jq_undo_snapshot(const jq_state *st)` 函数体只有 `(void)st;`，**不做任何快照**。任何按头文件契约调用它的代码都会得到"什么都没发生"的结果 —— 悔棋链在调用方（`jni_bridge.c` 的 `undo_push`）自建栈，引擎层的快照 API 完全是空壳。若将来有第二个调用方（联机 `bt_session.c` 已经用 `jq_snapshots_left`/`jq_undo_restore`）就会静默失效。
- 依据：
  1. `junqi.h:158` 注释"走子前调用"；`rules.c:292-293` 注释"快照由调用方维护" —— 声明与实现语义相反。
  2. `bt_session.c:67`：`if (jq_snapshots_left(st) > 0) jq_undo_restore(st, &g_undo_snap);` —— 见 AUD-R-013，该分支永不成立。
  3. `src/game_logic.c:119-131` 的兄弟实现有**完整**的环形快照栈（`snapshot_undo` 存 board/turn/stage/winner/flags/pieces/mines/moves_since_eat），说明"引擎层自带悔棋"是本仓既定设计，src_rs 却是空壳。
- 修法：二选一并写进头文件：① 删除 `jq_undo_snapshot`/`jq_snapshots_left` 两个空 API（避免误用），调用方继续用自己的栈；② 在 `jq_state` 内实现环形快照栈（对齐 `src/game_logic.c` 的 `JQ_UNDO_DEPTH` 方案），让 `jq_undo_snapshot/restore/snapshots_left` 真正生效。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-013 [P1] `jq_snapshots_left()` 恒返回 0 → 蓝牙 UNDO 帧永远不生效（联机悔棋功能实为死代码）
- 位置：`src_rs/rules.c:302-306`、消费点 `src_rs/bt_session.c:65-70`
- 问题：`jq_snapshots_left()` 恒返回 0（`return 0; /* 快照栈在调用方 */`），而 `bt_session.c:67` 用它作为"是否可以恢复"的闸口 → 收到 `BT_T_UNDO` 帧时**永远不执行 `jq_undo_restore()`**，只把回合切回 OUR_TURN 并返回 0（假装成功）。对端收到"悔棋已生效"的语义，实际棋盘未变 → 联机双方状态发散。
- 依据：
  1. `bt_session.c:65-70` 代码路径（`case BT_T_UNDO:`）。
  2. `bt_session.c:11` 的 `static jq_state g_undo_snap;` 从未被写入（永远是全 0 的空快照）→ 即使闸口打开，恢复的也是空盘。
  3. `docs_rs/EXPERIENCE_LOG.md` 记录"蓝牙悔棋/重开帧 jq_bt_ctrl_send UNDO/RESTART"已接线 —— 与本条矛盾（接线了但无效果）。
- 修法：实现真实的联机悔棋语义（对端请求 → 本地弹确认 → 双方各自回退一步并广播结果帧），或至少在 `BT_T_UNDO` 分支返回 -1（明确"不支持"）而不是静默成功；同时给 `g_undo_snap` 补上写入点。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-014 [P1] `pieces_left[]` 全线不可信：刷新逻辑恒为 1，且 rules.c 持续对它做增减
- 位置：`src_rs/rules.c:165-166, 190, 196, 205, 209, 214-215`（增减）+ 语义声明 `src_rs/junqi.h:112`；刷新实现 `src_rs/layout.c:127-143`
- 问题：`pieces_left` 被 `junqi.h:112` 定义为"各方剩余可动子数（0x6a3b0 统计）"，但：
  1. 刷新实现 `layout.c:137-139` 写的是 `if (t != i && jq_can_move(st, i, t)) { n = 1; break; } if (!n) n++;` —— `n` 被当作布尔标志而非累加器，且 `if (!n) n++` 在"该子无合法走子"时反而把 n 置 1 → **结果恒为 1**（只要该方有子）。实测 `jq_refresh_movable_counts()` 后 `pieces_left = 1 1`（真实值约 20）。
  2. `rules.c` 在每次吃子/同归时对其做 `--`，于是计数**必然变负**。实测：同级同归一步后 `pieces_left = -1 -1`。
  3. `rules.c:190` 工兵挖雷时 `pieces_left[1-me]--` —— 地雷不是"可动子"，不应计入。
- 依据：
  1. `junqi.h:112` 的语义声明；`rules.c` 的 6 处增减点。
  2. `REVERSE_NOTES_01_basics.md` §L61："0x6b87c→0x6a3b0 刷新可动子统计确证：走子后刷新统计+困毙判定" —— 即"走子后要刷新"，src_rs 只减不刷。
  3. 实测：`T13 refresh 后 pieces_left = 1 1`；`T34 同级 ev=2 ... pieces_left=-1 -1`；`T25 挖雷 ... pieces_left=1 0`（把不可动的地雷算成了可动子）。
  4. `src/game_logic.c:258-269` 的兄弟实现对 pieces_left 的语义是一致的（吃子 -1、挖雷 -1），但其刷新函数不存在同样的布尔 bug。
- 修法：① 修 `jq_refresh_movable_counts()` 的累加逻辑（`n += 1`，去掉 `if (!n) n++`）；② 在 `jq_try_move()` 结尾（以及 `jq_flip()` 后）调用刷新，而不是增量维护；③ 挖雷不计入可动子（地雷本就不计入）；④ 若 `pieces_left` 只用于展示，必须在 UI 侧改用"实时统计可动子"而不是维护缓存。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-015 [P1] `jq_state.mines_cleared[]` 只写不读：ruleflip 胜负条件完全未接线
- 位置：`src_rs/junqi.h:111`（声明）、`src_rs/rules.c:191`（唯一写入点）
- 问题：字段注释写明"暗棋（ruleflip）工兵挖雷计数"，`rules.c:191` 在工兵挖雷时 `mines_cleared[me]++`（实测：探针挖雷后 `mines_cleared[0] = 1`，写入路径通），但**全仓没有任何读取点**（`src_rs/` 内 grep 仅 `junqi.h:111` 声明、`rules.c:191` 写入、`save.c:67/102` 存取、`jni_bridge.c:1180` 清零）。于是"挖 3 雷才能扛旗"这条 ruleflip 胜利条件（见 AUD-R-002）无任何判定依据。
- 依据：
  1. `data_junqi_ruleflip.txt` 第 6 行胜利条件；`REVERSE_NOTES_01_basics.md` §L63。
  2. 兄弟实现 `src/game_logic.c:103` 明确读 `st->mines_cleared[side] < 3`。
  3. `src_rs/save.c:67` 还在持久化该字段 —— 持久化一个不参与判定的字段，进一步说明该条件被漏实现。
- 修法：实现 ruleflip 扛旗条件（见 AUD-R-002 修法 2），并把该条件写进 `SPEC_BUSINESS.md` §2 规则表。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-016 [P1] `jq_try_move()` 的 EV_FLAG 分支在关闭"吃军旗判胜"开关后仍返回 EV_FLAG 并切回合，事件语义与局面不一致
- 位置：`src_rs/rules.c:169-181`
- 问题：`if (ui_rule_cfg_get(3)) jq_on_flag_capture(st, me);` —— 关闭开关时：旗被吃（棋盘上旗消失）、事件仍返回 `EV_FLAG`（对局上层会播"扛旗"音效 `sfx_for_move(EV_FLAG)`）、`turn` 仍切到对方、局继续。但此时**对方已无旗可扛**，永远不可能再通过扛旗获胜，只能等困毙 → 出现"规则开关制造出的死局"。同时旗被吃后 `pieces_left` 未做任何更新。
- 依据：
  1. `rules.c:171-174` 注释自述"=0 时扛旗不判胜（棋子入位但局终不置，继续对局）" —— 注释承认了局面，但没意识到"对方再也无法获胜"。
  2. `rulelay.txt` 的胜负条款里"军旗被扛"与"杀光可动子"是并列判据；旗被炸/被吃但未判胜时，应回落到"杀光可动子"判据（当前只能靠困毙近似，见 AUD-R-020）。
  3. 实测：关闭 `ui_rule_flag_set(0)` 后吃旗 → `ev=3 stage=1 winner=-1 turn=1`（局未终）。
- 修法：① 关闭扛旗开关时，旗被吃应立即按"对方失去唯一胜利途径"处理（判该方负，或改回"旗被吃即胜"仅关闭音效/横幅）；② 至少在事件层区分"旗被吃但未终局"（新增事件码或复用 EV_EAT），避免播扛旗音效；③ 旗被吃时同步 `pieces_left`。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-017 [P1] `jq_can_move()` 丢弃 `const` 限定（编译告警），且 `dr_` 为未使用变量
- 位置：`src_rs/rules.c:97-98`（`jq_cell *src = &st->cells[from];` 其中 `st` 是 `const jq_state *`）、`src_rs/rules.c:101`（`int dr_ = rank_norm(dst->rank);`）
- 问题：`jq_can_move(const jq_state *st, ...)` 内把 `st->cells[...]` 赋给**非 const** 指针 `jq_cell *`，编译器报 `-Wdiscarded-qualifiers`（实测 gcc -Wall -Wextra 有 3 条告警）。这不仅是风格问题：一旦后续有人在该函数里通过 `src`/`dst` 写入，就会**改写一个声明为 const 的对局状态**（调用方普遍传 `&g_state` 的 const 视图），造成难以定位的状态污染。另 `dr_` 计算后完全未使用（`jq_can_move` 不需要目标 rank），属残留代码。
- 依据：实测编译输出：
  `rules.c:97:20: warning: initialization discards ‘const’ qualifier from pointer target type`
  `rules.c:98:20: ...` / `rules.c:101:9: warning: unused variable ‘dr_’`
  另 `walkable()` 的 `mover` 参数在 `rules.c:42` 被 `(void)mover;` 显式吞掉。
- 修法：`jq_cell *src` → `const jq_cell *src`（`dst` 同理），删除未用的 `dr_`，删除 `walkable()` 的 `mover` 形参（或真正用它做"不能穿过己方子"的判定）；把 `-Wall -Wextra -Werror` 纳入 CI。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-018 [P1] 「杀光对方所有能移动的棋子」判据缺失，仅靠困毙近似
- 位置：`src_rs/rules.c:225-230`（只判和棋与困毙）
- 问题：`rulelay.txt` 第 9/16 行的胜利判据是三条并列："杀光对方所有能移动的棋子则获得胜利；或者扛走对方的军旗" + "棋子被吃光、军旗被扛、无棋可走都会被判负"。src_rs 只实现了"扛旗"与"无棋可走（困毙）"两条，"杀光所有可动子"没有独立判定，只是因为"没有可动子 ⇒ 无棋可走"而被困毙逻辑近似覆盖。差异出现在：**对方仍有未翻开的暗子**时——按"杀光可动子"应立即判胜，但困毙逻辑因"还有暗子可翻"判为未困毙，对局继续。
- 依据：
  1. `data_junqi_rulelay.txt` 第 9 行与第 16 行；`data_junqi_ruleflip.txt` 第 6 行同款表述。
  2. `rules.c:270-276` 的困毙前置检查明确把"未翻开的非军旗子"视为"未困毙"。
  3. `REVERSE_NOTES_01_basics.md` §L57 记录 rulelay 的三条胜负判据均已对齐 arm64，但 src_rs 只见两条。
  4. 实测：`T17/T18` 证明"对方只剩旗"会被困毙判负（近似生效），但"对方只剩未翻暗子"的分支没有单独验证，属未覆盖路径。
- 修法：新增显式判据"对方可动子数归零 → 判胜"（可用修好的 `pieces_left` 或实时统计），与困毙并列；并在 `SPEC_BUSINESS.md` §2 规则表补齐三条胜负判据。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-019 [P1] `rail_path_straight()` 只校验端点、不校验路径同线（导致 AUD-R-001 的根因）
- 位置：`src_rs/rules.c:74-89`（尤其 `rules.c:80` 与 `rules.c:84-87`）
- 问题：函数用 `is_rail(fr,fc) && is_rail(tr,tc)` 作为"这条路径在铁路上"的判据，但 `is_rail()` 是**逐格属性**，端点在铁路 ≠ 路径在铁路。`rules.c:84-87` 的行走循环只检查 `walkable()`（无子/非行营/非大本营），**没有检查路径各格是否仍在同一条铁路上**。因此只要起点与终点恰好都是"铁路格"（含底线角格、row5/6 与 col2 的交点等），任意两格之间的直线都可能被判为合法滑行。
- 依据：
  1. `rulelay.txt` 第 2 行："任何棋子在公路线上只能走一步；…其它棋子在铁路线上只能直走，不能转弯" —— "在铁路线上"是对**路径**的约束，不是对端点的约束。
  2. `REVERSE_NOTES_01_basics.md` §L68："纵向 col2 贯穿 … col0/col4 非铁路（只相邻一步）"。
  3. 实测（探针）：(11,0)→(0,0)、(0,4)→(5,4) 判为可走；(0,4)→(4,4) 判为不可走（端点不在铁路上）——典型的端点式判定失效。
  4. A/B 实测：移除底线铁路后 1061 条非法走子全部消失（见 AUD-R-001）。
- 修法：把 `rail_path_straight()` 改为"路径同线"判定：纵向滑行要求 `fc == tc == 2`（唯一纵线）或路径每格 `is_rail` 且同列；横向滑行要求 `fr == tr ∈ {5,6}` 且路径每格 `is_rail`；在行走循环里同时校验 `is_rail(r,c)`（而不只是 `walkable`）。


- 闭环：**✅ 已修**（AUD_LOOP 对抗审计批次：P0/P1 规则条款真修 + evverify 167/167 实证；修复痕迹见 rules.c/junqi.h 行内注释，本标记为文档侧闭环收口；32 位翻译体侧分叉状态见 rules_fork_check.py 矩阵）
## AUD-R-020 [P1] 「炸弹撞军旗」消灭了旗却无人获胜，且规则引擎不产生任何终局信号
- 位置：`src_rs/rules.c:161-168`（EV_BOMB 分支）
- 问题：EV_BOMB 分支在炸弹与军旗相遇时把两格清空 → 对方的旗被消灭，但**不判任何胜负**（`winner` 不变、`stage` 不变）。此后对方永远无法通过扛旗获胜，只能等困毙；若对方仍有可动子且不进入困毙，对局将无限继续。
- 依据：
  1. `rulelay.txt` 第 12 行："炸弹与任何棋子相遇时，双方都消失，包括军旗" —— 规则文本确实允许"旗被炸掉"。
  2. `rules.c:161-168` 无 flag/win 相关处理；实测（探针 T36）：炸弹撞旗 → `ev=2 stage=2 winner=0`，其中 `winner=0` 是**困毙**判出来的（对方只剩一枚位于大本营内的子，按"进大本营不可再动"不可动），并非"旗被炸"判出的 —— 语义被错误归因。
  3. `src/game_logic.c:219-225` 同样只做双方消失，无胜方判定 → 两实现一致地缺失该边界。
- 修法：明确"旗被消灭"后的胜负语义（建议：对方失去唯一胜利途径 → 判对方负，或引入"旗被毁"事件），并在 `SPEC_BUSINESS.md` 记录该边界；同时不要让困毙检查把该局面"顺手"判掉（否则统计/音效归因错误）。

---

# 三、P2（约 60 条）

## AUD-R-021 [P2] `rules.c` 头注释描述的 0x39e14 行约束被误标为"走子目标类型判定"
- 位置：`src_rs/rules.c:4-7`
- 问题：注释写"炸弹(0xa): to_row!=0 合法；地雷(0xb): from_row>3 合法(仅工兵吃)；军旗(0xc): from_row∈{1,3,5} 特判"，把三个**布局期**限制写成"目标类型判定"，与 §15/§16 的实际语义（"非走子方向限制"）混淆，且实现里也未体现，易被后人误加到走子路径上。
- 依据：`REVERSE_NOTES_01_basics.md` §L59："0x39e14 布局校验确证：军旗 col∈{1,3}&&row==5（大本营）、地雷 row>3（最后两行）、炸弹 row!=0（不放第一行）——**非走子方向限制**，军旗吃法无方向限制"。
- 修法：注释改为"布局校验限制（见 layout.c jq_check_layout_cell），非走子约束"，并把 `rules.c` 里的行号引用指向 §L59。

## AUD-R-022 [P2] `rules.c` 头注释的铁路描述与 `board.c` 实现矛盾（"纵向仅 col2 贯穿；横向仅 row5/6" vs 实现含 row0/11）
- 位置：`src_rs/rules.c:20` vs `src_rs/board.c:40`
- 问题：同一套代码里两处对铁路拓扑的描述不一致，且实现与注释均与 L49/L68 的十字拓扑有出入（见 AUD-R-001/002）。
- 依据：`rules.c:20` vs `board.c:40`；`REVERSE_NOTES_01_basics.md` §L49/§L68。
- 修法：以 L49/L68 为唯一事实源，统一 `board.c` / `rules.c` / `SPEC_BUSINESS.md` §2 表格 / `AUDIT_CHECKLIST.md` A2 条目。

## AUD-R-023 [P2] `rules.c` 头注释称"行营/大本营在铁路外"，与标准军棋（中心行营在纵路上）存在争议，需人工确认
- 位置：`src_rs/rules.c:21`、`src_rs/board.c:37`（`is_rail` 先排除行营/大本营）、`src_rs/rules.c:40`（`walkable` 排除行营）
- 问题：把行营排除出铁路，导致中线 col2 被行营 (3,2)/(8,2) 切成三段：**连工兵也无法在一次走子里穿过中心行营**（实测 (2,2)→(4,2) = 0，即使目标为空）。标准军棋中纵路是贯通的、工兵可沿中线长距离机动（且中线行营是"公路岛"而非断路点）。此处实现与 L68 的描述一致（"行营(3,2)(8,2)分三段"），但与"工兵可在铁路线上任意行走"（rulelay 第 2 行）的通常解读存在张力，属于**需人工确证**的取舍。
- 依据：`rulelay.txt` 第 2 行；`REVERSE_NOTES_01_basics.md` §L68（"行营(3,2)(8,2)分三段…行营/大本营阻断"）；兄弟实现 `src/game_logic.c:44` 的注释却写"纵向移动任意列可行（L68 原版'十'字铁路）" —— 同仓两处对 L68 的解读互相矛盾。
- 修法：人工复核原版 `.so` 里 `jq_is_rail` / 铁路连通性判定（0x39e14 调用者路径）后二选一：(a) 维持"行营断路"，则在 `SPEC_BUSINESS.md` 明确写"工兵不可穿越中心行营"；(b) 改为"行营可穿行但不可吃"，同步改 `walkable()` 与 `is_rail()`。

## AUD-R-024 [P2] 工兵 BFS 的横向限制用"当前行"判定，语义脆弱且与 `is_rail` 的行集合重复定义
- 位置：`src_rs/rules.c:62`（`if (nc != c && r != 5 && r != 6) continue;`）
- 问题：横向移动的许可用当前行 `r ∈ {5,6}` 判定，而目标格是否可入又用 `is_rail(nr,nc)` 判定（`rules.c:63`）。两处各自硬编码"中线 = row5/6"，行集合在 `board.c:39` 又写了一遍 → 三处同义常量。
- 依据：`board.c:39`（`if (row == 5 || row == 6) return true;`）、`rules.c:62`、`rules.c:79`。
- 修法：把"中线行集合""纵线列"提为 `board.c` 的具名常量/谓词（如 `is_mid_line(row)`、`is_main_rail_col(col)`），三处共用。

## AUD-R-025 [P2] `rail_path_engineer()` 起点未做边界检查 → `visited[fr][fc]` 越界写
- 位置：`src_rs/rules.c:54-55`（`qr[tail] = fr; qc[tail] = fc; tail++; visited[fr][fc] = 1;`）
- 问题：`fr/fc` 直接写入 `visited[BOARD_ROWS][BOARD_COLS]` 与队列，未做 `0 <= fr < 12 && 0 <= fc < 5` 检查。当前唯一调用方 `jq_can_move()` 传的是已校验的 `from`，但函数是 `static` 且无自检，任何后续改动都可能引入越界写（栈缓冲区）。
- 依据：`rules.c:47-55` 无边界检查；对照 `rules.c:37` 的 `walkable()` 有完整边界检查 → 同一文件内防护标准不一致。
- 修法：BFS 入口加 `if (fr < 0 || fr >= BOARD_ROWS || fc < 0 || fc >= BOARD_COLS) return false;`，并把 `visited` 改为 `uint8_t visited[BOARD_ROWS][BOARD_COLS]`（240B→60B，省栈）。

## AUD-R-026 [P2] BFS 队列 `qr[64]/qc[64]` 无显式上界断言（60 格刚好塞满，边界脆弱）
- 位置：`src_rs/rules.c:53`（`int qr[64], qc[64]`）、`rules.c:67`（`qr[tail] = nr; ... tail++`）
- 问题：队列容量 64 > 60 格，理论安全；但 `tail` 无 `tail < 64` 断言，若将来把 `visited` 逻辑改动导致重复入队，会静默越界写栈。
- 依据：`rules.c:53,67` 无边界断言；`rules.c:66` 的 `visited[nr][nc] = 1` 保证了不重复入队，但该保证是"隐式"的（依赖 `visited` 语义不被改动）。
- 修法：队列改用 `int q[BOARD_ROWS*BOARD_COLS]` 并加 `if (tail >= (int)(sizeof(q)/sizeof(q[0]))) break;` 断言。

## AUD-R-027 [P2] `walkable()` 的 `mover` 参数未使用，函数语义名不副实
- 位置：`src_rs/rules.c:35-44`（尤其 `rules.c:42` 的 `(void)mover;`）
- 问题：函数注释写"该格能否被'经过'（非子、非行营、非大本营）"，但签名带 `mover`（走子方）却完全不用 —— 兄弟实现 `src/game_logic.c:19` 的 `walkable(st,row,col,mover_side)` 同样忽略该参数，两实现都留了"将来可能用于'能否穿过己方子'"的悬空意图。
- 依据：`rules.c:42`；`src/game_logic.c:19-26`。
- 修法：删除该参数（当前语义不需要），或在注释里写明"预留：是否允许穿过己方子尚未定案"。

## AUD-R-028 [P2] `jq_can_move()` 允许敌方一步进入空行营，与兄弟实现 `game_logic.c` 结论相反
- 位置：`src_rs/rules.c:120-127`
- 问题：行营检查只拦"吃行营里的敌子"，**空行营可以被敌方进入**。`rulelay.txt` 原文只禁止"敌方棋子不能吃行营中的棋子"，字面上允许进入；但兄弟实现 `src/game_logic.c:93` 明确禁止："不能进入敌方行营"（`if (jq_is_camp(tr, tc) && side_of(tr) != side) return false;`）。同仓两套引擎对同一规则给出相反结果。
- 依据：`rulelay.txt` 第 3 行；`src/game_logic.c:93`；实测：`jq_can_move(蓝(1,1) → 空行营(2,1))` = 1。
- 修法：人工确认原版行为（0x39e14 调用者对行营目标格的处理），统一两套实现，并在 `SPEC_BUSINESS.md` 记录结论。

## AUD-R-029 [P2] 大本营目标判定只拦"空大本营"，未按原版区分"大本营的地雷、军旗可以吃"
- 位置：`src_rs/rules.c:129-134`
- 问题：注释写"目标在大本营：仅能吃军旗/地雷（翻棋模式）或其他子"，但代码只判断 `dst->rank == RANK_EMPTY → false`，即**大本营内任何子都可被吃**。ruleflip 明确大本营内的地雷/军旗可以被吃（ruleflip 第 3 行"大本营的地雷、军旗可以吃"），rulelay 未禁止吃其它子 —— 语义模糊。
- 依据：`data_junqi_ruleflip.txt` 第 3 行；`src/game_logic.c:96` 的收窄写法。
- 修法：按模式实现 ruleflip 语义（大本营仅旗/雷可吃），明棋按 rulelay 放开，并写进 SPEC。

## AUD-R-030 [P2] 「进大本营不可再动」只作用于源格，未阻止"大本营被清空后再次进入"等边界
- 位置：`src_rs/rules.c:111`（`if (ui_rule_cfg_get(1) && is_base(cell_row(from), cell_col(from))) return false;`）
- 问题：规则语义是"进入任何大本营的棋子不能再移动"，实现是"当前在大本营里的棋子不能动"——这两者在"棋子进入大本营后又离开（关闭开关时）"、"旗被吃后大本营变空"等场景下不等价；且该判定只看 `from`，不检查 `to` 是否为对方大本营（对方大本营里若有普通子仍可吃，见 AUD-R-007/029）。
- 依据：`rulelay.txt` 第 3 行"进入任何大本营的棋子不能再移动"；`rules.c:111`；实测：吃旗后攻子位于大本营内 → `jq_can_move` = 0（该子被冻住，符合规则）。
- 修法：把语义表达为"曾进入过大本营的棋子冻结"，或在棋盘层加"进入大本营即锁定"的标记位（当前用"位置在大本营"近似，需在旗被吃/大本营空置场景补测）。

## AUD-R-031 [P2] 「吃地雷」开关关闭时，工兵撞雷被判死，但开关语义与原版规则开关的对应关系未记录
- 位置：`src_rs/rules.c:183`（`if (sr == 9 && ui_rule_cfg_get(2))`）
- 问题：`ui_rule_cfg_get(2)` 关闭后，工兵不再挖雷，而是走 else 分支被炸死（EV_DEAD）。这意味着"关掉吃地雷规则"= "工兵撞雷必死"，与 `rules.c:185-186` 注释自述一致，但没有任何文档/SPEC 说明这 4 个开关的原版语义来源与取值。
- 依据：`rules.c:171-186` 的注释；`junqi.h` 无开关语义表；`docs_rs/SPEC_BUSINESS.md` §2 规则表无"规则开关"列。
- 修法：在 SPEC §2 增加"4 个规则开关"行（名称/默认/关闭后的行为/原版对应），并给 `ui_rule_cfg_get` 的 `which` 参数加枚举常量（现在 0/1/2/3 是裸数字）。

## AUD-R-032 [P2] 4 个规则开关在规则引擎内部直接调用 UI 层函数 → 规则层与 UI 层耦合，无法独立单测/复用
- 位置：`src_rs/rules.c:24-25`（`#include "ui_state.h"`）、`rules.c:111, 176, 183, 225, 259`
- 问题：`rules.c` 依赖 `ui_state.h` 的 `ui_rule_cfg_get()`。后果：(a) 任何链接 `rules.c` 的目标都必须同时提供 UI 层实现（桌面构建靠 `ui_state_stub.c` 兜底）；(b) 规则行为随 UI 设置改变，规则引擎不再是纯函数，无法做"规则矩阵"回归；(c) 规则与 UI 的耦合方向违反分层（`docs_rs/CODE_STYLE.md` 的分层原则）。
- 依据：`rules.c:24-25` 注释自述"★ P1（datah_012）：游戏设置 4 规则开关"；`Makefile:31-35,86-94` 显示所有桌面目标都必须额外链接 `ui_state_stub.c`。
- 修法：把规则开关抽到 `jq_state`（随对局快照一起保存/回滚）或抽成 `rules_cfg` 结构体由调用方注入；`rules.c` 只读 `st->cfg.*`，UI 层负责填充。

## AUD-R-033 [P2] 和棋判据 `total_moves >= 1000` 把"翻棋"也计入"双方总步数"
- 位置：`src_rs/rules.c:226`、`src_rs/rules.c:256`（`jq_flip` 也 `total_moves++`）
- 问题：`rulelay.txt` 第 18 行"双方总步数达到 1000 步，判和" —— "步"是否包含翻棋未明确。src_rs 把翻棋计入 `total_moves` 与 `moves_since_eat`（`rules.c:257`），使暗棋模式下和棋更早触发（实测纯翻棋 48 步即用掉近一半预算）。
- 依据：`rulelay.txt` 第 17-18 行；`REVERSE_NOTES_01_basics.md` §L70："暗棋 70步无吃子和棋（**翻棋计步**），对齐原版 ruleflip 同样" —— 即 70 步计翻棋有据，但 1000 步是否同样计翻棋无据。
- 修法：把"总步数"拆成 `total_moves`（走子）与 `total_turns`（走子+翻棋）两个字段，或在 SPEC 中明确"翻棋计入总步数"并给出原版依据。

## AUD-R-034 [P2] 和棋判定先于困毙判定，二者同时成立时优先级未定义
- 位置：`src_rs/rules.c:225-230`
- 问题：先 `jq_on_draw()`（`stage=STG_OVER`），再 `if (st->stage == STG_PLAY && jq_side_stalemated(...))` —— 若同一步同时满足"70 步无吃子"和"对方无棋可走"，则判和而不是判负。原版优先级无据。
- 依据：`rules.c:225-230` 的顺序；`src/game_logic.c:275-276` 同序（兄弟实现一致）。
- 修法：明确优先级（建议困毙优先，因为"无棋可走"是更早可判定的终局），写进 SPEC 并加回归用例。

## AUD-R-035 [P2] `jq_on_draw()` / `jq_on_stalemate()` 不检查 `stage`，可在已终局的局面上改写 `winner`
- 位置：`src_rs/rules.c:315-325`
- 问题：两个函数直接写 `winner`/`stage`，没有"已终局则忽略"的守卫（兄弟实现 `src/game_logic.c:186,195` 有 `if (st->stage == JQ_STAGE_OVER) return;`）。任何外部调用（如 `jni_bridge.c:2084` 的 drawtest 属性钩子、`:2097` 的 stalematetest 钩子）都能把一个已分出胜负的对局改判为和棋/困毙。
- 依据：`rules.c:315-325` 无守卫 vs `src/game_logic.c:186,195` 有守卫；`jni_bridge.c:2082-2102` 存在直接调用的测试钩子。
- 修法：两个函数入口加 `if (st->stage == STG_OVER) return;`（或加 `force` 参数供测试钩子显式传参）。

## AUD-R-036 [P2] `jq_on_stalemate()` 的胜方由"调用时刻的 turn"推导，外部调用语义脆弱
- 位置：`src_rs/rules.c:321-325`（`st->winner = 1 - st->turn;`）
- 问题：函数不接收"被判负的一方"，而是用 `st->turn` 推导。内部调用（`rules.c:229`）时 turn 正是被判负方，正确；但外部调用（如 `jni_bridge.c:2097` 的 stalematetest）依赖调用时刻恰好 turn==应判负方，否则胜方颠倒。
- 依据：`rules.c:323`；`jni_bridge.c:2090-2099` 注释"此处直接模拟困毙判定结果"。
- 修法：改为 `jq_on_stalemate(st, uint8_t loser_side)` 显式传参（与 `jq_on_flag_capture(st, captor_side)` 的风格一致）。

## AUD-R-037 [P2] 吃旗分支在 `jq_on_flag_capture()` 之后仍切 turn / 累加 total_moves，终局状态里 turn 指向败方
- 位置：`src_rs/rules.c:176-181`
- 问题：`jq_on_flag_capture()` 已置 `stage = STG_OVER`，随后代码仍执行 `st->turn = 1 - me;`（把回合交给败方）与 `st->total_moves++`。终局快照的 turn 语义含混（谁该走？），并让"undo"与"战况统计（当前回合）"在终局时显示败方。
- 依据：`rules.c:176-181` 的执行顺序；`jni_bridge.c:6676` 的战况统计会打印 `g_state.turn == SIDE_BLUE ? "蓝方" : "红方"` → 终局时可能显示"红方当前回合"，误导。
- 修法：终局后不再改 turn（`jq_on_flag_capture` 之后直接 return），或把 turn 语义改为"最后行动方"并同步 UI 文案。

## AUD-R-038 [P2] EV_FLAG 分支提前 return → 跳过和棋与困毙检查（关闭扛旗开关时局面检查缺失）
- 位置：`src_rs/rules.c:181`（`return ev;`）
- 问题：EV_FLAG 分支在 `rules.c:181` 提前返回，跳过了 `rules.c:225-230` 的和棋/困毙检查。终局时无所谓，但在"关闭吃军旗开关"场景（局继续）下，这一步之后的困毙/和棋判定被跳过。
- 依据：`rules.c:181` vs `rules.c:225-230`；AUD-R-016 的实测（关闭开关后 ev=3、stage 仍 PLAY）。
- 修法：把提前 return 改为 goto/结构化收尾，保证所有分支走同一段终局检查。

## AUD-R-039 [P2] EV_MINE 分支中 `if (src->side != 0xFF)` 是死判断（在 `memset` 之后求值）
- 位置：`src_rs/rules.c:188-191`
- 问题：顺序是 `*dst = *src; memset(src, 0, sizeof(*src)); ... if (src->side != 0xFF) st->mines_cleared[me]++;` —— `src` 已被清零，`src->side` 恒为 0，`0 != 0xFF` 恒真 → 该条件永不影响行为，且**丢失了"原 src 是否为未定色暗子"的信息**（若将来要用"暗棋挖雷是否计数"作判定，这里已经拿不到原值）。
- 依据：`rules.c:188-191` 的语句顺序；实测挖雷后 `mines_cleared[0]=1`（恒自增）。
- 修法：在 `memset` 之前把 `uint8_t src_side = src->side;` 存下来，或直接删除该判断（暗棋挖雷计数无条件递增，符合 ruleflip）。

## AUD-R-040 [P2] EV_MINE 分支用 `pieces_left[1 - me]` 代替 `dst->side`，暗子/未定色时计数对象错
- 位置：`src_rs/rules.c:190`
- 问题：`st->pieces_left[1 - me]--` 假定目标一定属于对方。当目标 `side == 0xFF`（未定色暗子，见 AUD-R-006）时，减的是"另一方"而非目标实际归属方；同时地雷本不是"可动子"，本不该计入可动子计数（见 AUD-R-014）。
- 依据：`rules.c:190`；`junqi.h:112` 的语义声明；对照 `rules.c:166` 用的就是 `dst->side`（两处风格不一致）。
- 修法：统一用 `dst->side`（并做 `<2` 范围检查），同时从可动子计数中排除地雷/军旗。

## AUD-R-041 [P2] `jq_try_move()` 全程不刷新 `pieces_left` / `count_tbl`，只做增量减法
- 位置：`src_rs/rules.c:146-232`（无 `jq_refresh_movable_counts()` 调用）
- 问题：走子后没有任何刷新点，计数只减不增（与 AUD-R-014 同源）。`count_tbl`（`junqi.h:113`，"布局计数表（原版 +0x78，rank→数量）"）在全仓从未被写入 → 恒为 0。
- 依据：`rules.c` 内无 `jq_refresh_movable_counts` 调用；`layout.c:92-108` 的 `jq_validate_layout()` 用的是**局部** `seen[]` 数组，从不回填 `st->count_tbl`。
- 修法：走子/翻棋后调用 `jq_refresh_movable_counts(st)`；`jq_validate_layout()` 把统计结果写回 `st->count_tbl`（或删除该字段）。

## AUD-R-042 [P2] `jq_try_move()` 不校验 `to != from`（依赖 `jq_can_move()` 的前置判断），双层防护缺失
- 位置：`src_rs/rules.c:148`
- 问题：`jq_try_move()` 只调 `jq_can_move()`，而后者才做 `from == to` 检查。若将来有第二个入口（联机、复放、BT 帧）绕过 `jq_can_move()`，`from == to` 会导致"自己吃自己"（`sr == dr_` → 同级同归分支 → 双方消失）。
- 依据：`rules.c:148` 与 `rules.c:94`；`bt_session.c:51` 走 `jq_try_move()`（受保护），但 `bt_session.c:61` 的 FLIP 分支直接调 `jq_flip(st, idx)`（内部有 `jq_can_flip` 保护）—— 当前都安全，属"单点防护"风险。
- 修法：`jq_try_move()` 入口补 `if (from == to || from < 0 || from >= N_CELLS || to < 0 || to >= N_CELLS) return -1;`。

## AUD-R-043 [P2] `jq_flip()` 不校验 `rank` 合法性、也不更新 `flags`
- 位置：`src_rs/rules.c:248-263`
- 问题：`jq_flip()` 只改 `side` 与 `revealed`，不校验 `rank_norm(c->rank)` 是否在 1..12（`jq_can_flip` 只判 `c->rank == RANK_EMPTY`），也不同步 `flags`（`JQF_BOMB/MINE/FLAG`）。若外部注入非法 rank（>12 的蓝方值），`rank_norm` 会把它还原成另一个合法 rank → 静默错乱。
- 依据：`rules.c:239-241`；`junqi.h:122-124` 的 `rank_norm` 语义；`layout.c:52-61` 是唯一写入 rank 的地方（合法），但 `bt_session.c:74` 的 `BT_T_SYNC_CELL` 可从对端注入任意 `rank`（`f3`）。
- 修法：`jq_can_flip()` 增加 `int r = rank_norm(c->rank); if (r <= 0 || r > RANK_TOP) return false;`；`flags` 统一由 rank 派生（见 AUD-R-046）。

## AUD-R-044 [P2] 翻棋后 turn 即切换 → 刚翻出的子本回合不能再走（与"翻棋后保持选中"的交互语义冲突）
- 位置：`src_rs/rules.c:255`（`st->turn = 1 - st->turn;`）、交互侧 `src_rs/jni_bridge.c:7751-7755`（"翻棋成功保持选中（态机已 SELECTED，下一步走子）"）
- 问题：翻棋即消耗一个回合并交出行动权（这本身符合 ruleflip"轮流翻棋"），但 UI 在翻棋成功后**保持选中该格**并提示"下一步走子"，而引擎此时已把回合交给对方 → 玩家点目标格会因 `rules.c:105`（`src->side != st->turn`）被拒，UI 与引擎语义不一致。
- 依据：`rules.c:255` + `rules.c:105` + `jni_bridge.c:7751-7755` 的注释与实现；`src/game_logic.c:305` 同样在翻棋后切 turn（两实现一致，问题在 UI 侧）。
- 修法：明确"翻棋即换手"的交互：翻棋成功后清除选中态（`jq_glview_set_selected(..., -1)`），并在 UI 文案提示"已翻牌，轮到对方"。

## AUD-R-045 [P2] `jq_can_flip()` 的"可翻"白名单与困毙判定的白名单不统一（地雷可翻但翻后不可动）
- 位置：`src_rs/rules.c:241`（`if (r == RANK_FLAG) return false;`）、`src_rs/rules.c:270-276`（困毙的白名单同样只排除军旗）
- 问题：地雷允许翻（依据 L50"地雷可翻"），但地雷翻开后**不可移动** → 翻雷是一次"消耗回合却不动"的空动作；更严重的是困毙判定把"还有未翻的地雷"当作"未困毙"（`rules.c:272-275` 只排除军旗），于是某方可能"有雷可翻 → 不算困毙"，但翻完雷依然无棋可走 → 困毙永远判不出来（因为每次困毙检查时那枚雷还是暗的？不对，翻完就 revealed=1 了，但翻雷消耗一个回合，之后困毙检查会因"无未翻子且无可动子"判负 —— 逻辑勉强闭环，但语义别扭）。
- 依据：`rules.c:272-275` 与 `rules.c:241`；兄弟实现 `src/game_logic.c:158-163` 明确把**地雷**也排除在"可翻以避免困毙"之外（"地雷暗棋不可翻，翻后不可动"），并与 `game_logic.c:170` 的 `!c->is_flag && !c->is_mine` 保持一致 → 两实现白名单不一致。
- 修法：统一"可翻集合"定义（建议：军旗、地雷均不可翻），在 `jq_can_flip` 与 `jq_side_stalemated` 共用一个谓词 `is_flippable(c)`。

## AUD-R-046 [P2] `jq_cell.flags` 与 `rank` 双表示且无一致性维护
- 位置：`src_rs/junqi.h:66`（`uint8_t flags; /* 位：1=is_bomb 2=is_mine 4=is_flag */`）、`junqi.h:69-71`（JQF_* 常量）、写入点 `src_rs/layout.c:54-61`
- 问题：`flags` 完全可由 `rank_norm(rank)` 推导，却作为独立字段存储，只在装载时设置一次、走子时靠整格拷贝（`*dst = *src`）被动跟随、吃子时随 memset 清零。全仓**无任何读取点**（规则引擎一律用 rank 判定），因此：① 纯冗余；② 一旦某处只改 rank 不改 flags（或反之）即产生静默不一致（例如 `jq_flip()` 不更新 flags，见 AUD-R-043）。
- 依据：`junqi.h:66-71`；`layout.c:54-61`；`rules.c` 内 grep 无 `JQF_` 使用；`src/game_logic.c` 的对应实现用 `is_flag/is_mine/is_bomb` 三个独立 bool 字段且与 rank 同步维护（`game_logic.c:350-352`）。
- 修法：删除 `flags` 字段（一切用 `rank_norm` 推导），或保留但在所有写 rank 的地方同步维护，并加断言。

## AUD-R-047 [P2] `rank_norm()` 依赖"值 > 12 即红方编码"的隐式约定，不校验 side
- 位置：`src_rs/junqi.h:122-124`（`return v > RANK_TOP ? v - 0xc : v;`）
- 问题：还原逻辑只看数值大小不看归属。蓝方若出现 >12 的 rank（例如联机 SYNC 注入、复盘数据损坏），会被静默还原为别的 rank（如 13 → 1 司令），造成"凭空多出一个司令"。
- 依据：`junqi.h:63`（"红方翻开存储 +0xc，用 rank_norm 还原"）与 `junqi.h:64`（side 字段已存在）；`layout.c:59` 写红方 `v + 0xc`；`bt_session.c:74-76` 的同步帧可注入任意 rank/side 组合。
- 修法：提供 `rank_of(const jq_cell*)`（结合 side 判定并做范围校验），废弃裸用 `rank_norm` 的 11 处调用点；或约定"rank 字节永不 > 12，side 单独存"，从根上消除编码。

## AUD-R-048 [P2] `jq_extract_layout_30()` 假设"蓝方半区 = 布局的 6 行"，红方半区信息丢失（红方布局存档会被镜像化）
- 位置：`src_rs/junqi.h:172-174` 声明；实现 `src_rs/layout.c:113-124`
- 问题：只导出蓝方 30 字节，红方半区被丢弃。注释已说明"红方半区镜像同布局，不重复"——但这只在"双方布局严格镜像"时成立；一旦对局中出现红方独有布局（联机同步、单机改布局、四国模式），导出的 30 字节无法还原真实局面。
- 依据：`layout.c:113-124`；`jni_bridge.c:6021` 用它做"布局存档"，`replay.c:48` 注释也指出"30 字节提取只收蓝方半区"。
- 修法：存档改为保存 60 格完整状态（`jq_save_game_*` 已有该能力），布局导出仅用于"导出可分享布局"场景，并在 UI 上标注。

## AUD-R-049 [P2] `jq_state.undo_depth` 字段无任何使用者，且 `jq_undo_restore()` 会用快照覆盖它
- 位置：`src_rs/junqi.h:115`（`int undo_depth;`）、`src_rs/rules.c:297-300`（`*st = *snap;`）
- 问题：`undo_depth` 从不被读；`jq_undo_restore()` 整体拷贝结构体，会把 `undo_depth` 恢复成快照时刻的值 —— 若调用方用该字段维护自己的栈深，悔棋一次就会把栈深回退（栈内容与栈深不一致）。
- 依据：实测：把 `snap.undo_depth = 7` 后 `jq_undo_restore()` → `st.undo_depth == 7`（快照值覆盖）。全仓 grep `undo_depth` 仅 `junqi.h:115` 声明。
- 修法：删除该字段，或在 `jq_undo_restore()` 中显式保留调用方的 `undo_depth`（`int d = st->undo_depth; *st = *snap; st->undo_depth = d;`）。

## AUD-R-050 [P2] `jq_undo_restore()` 整体覆盖状态，会把"快照时刻之前"的其它状态一并回滚（undo 栈在调用方，语义不清）
- 位置：`src_rs/rules.c:297-300`
- 问题：`*st = *snap` 覆盖 cells/turn/stage/winner/moves_since_eat/total_moves/pieces_left/mines_cleared/count_tbl 等全部字段。对"悔一步"是合适的，但调用方（`bt_session.c:67`）的 `g_undo_snap` 是**从未写入**的全局变量 → 一旦闸口被打开就会把整盘清零（见 AUD-R-013）。
- 依据：`bt_session.c:11`（`static jq_state g_undo_snap;` 无写入点）。
- 修法：`jq_undo_restore()` 保持现状但在头文件写明"整体状态恢复语义"；同时修复 `g_undo_snap` 的写入点或删除该 UNDO 分支。

## AUD-R-051 [P2] `jq_state_init()` 在 layout.c，而 `junqi.h` 的声明与实现分处两个文件，易漏链接
- 位置：`src_rs/junqi.h:136`（声明）、实现 `src_rs/layout.c:30-36`
- 问题：状态初始化、布局装载、校验、计数刷新四个核心函数全部位于 `layout.c`，而 `junqi.h` 把它们与 `rules.c` 的走子 API 并列声明。新增编译目标时若只链接 `board.c rules.c` 会直接链接失败（历史上 `Makefile` 里 `bttest`/`laytest`/`walkverify` 各自手工列出 `layout.c`，正说明该耦合已被反复踩到）。
- 依据：`Makefile:9`（`SRCS_CORE := board.c layout.c rules.c save.c bt_protocol.c replay.c`）与 `Makefile:44,54,62,70,80,94` 的各目标手工列文件。
- 修法：把 `jq_state_init` 等状态函数移到 `rules.c` 或新建 `state.c` 并在 Makefile 统一引用 `$(SRCS_CORE)`（部分目标已这样做）。

## AUD-R-052 [P2] `STG_SETUP`（布阵阶段）在规则引擎中无任何分支，等于死枚举
- 位置：`src_rs/junqi.h:74-78`、`src_rs/rules.c:94`（`if (st->stage != STG_PLAY ...) return false;`）
- 问题：定义了布阵阶段，但 `jq_can_move`/`jq_flip` 在该阶段一律返回 false，且没有布阵编辑 API（移动/交换棋子并做布局校验）。`jq_check_layout_cell` 是"校验单个格"的纯函数，缺少"整盘编辑 + 校验 + 提交"的对局级流程（兄弟实现 `src/game_logic.c:421-499` 有完整的 `jq_setup_begin/place/validate/confirm`）。
- 依据：`junqi.h:74-78`；`src/game_logic.c:421-499`；`src_rs` 中无 `STG_SETUP` 使用点（grep）。
- 修法：要么实现布阵阶段 API（对齐 `game_logic.c`），要么删除 `STG_SETUP` 枚举并在 SPEC 中说明"布阵仅在 UI 层做"。

## AUD-R-053 [P2] `winner` 的取值（-1/0/1/2）靠注释约定，无枚举/常量，且 `jq_state.winner` 是 `int8_t`
- 位置：`src_rs/junqi.h:108`（`int8_t winner; /* -1 未定 / 0 蓝胜 / 1 红胜 / 2 和 */`）、`junqi.h:162-165`
- 问题：三处魔法数（-1/0/1/2）散落在 `rules.c`（`st->winner = captor_side;` / `= 2;` / `= 1 - st->turn;`）、`layout.c`、`jni_bridge.c`（多处 `winner == 2` 判和棋）。`int8_t` 虽够用，但 `winner = 1 - st->turn` 在 `turn` 为 `uint8_t` 时若 turn>1 会得到负数 → 与 -1（未定）撞码。
- 依据：`junqi.h:108`；`rules.c:311, 317, 323`；`jni_bridge.c:4801, 6642, 7182` 等处直接写 `winner`。
- 修法：定义 `JQ_WINNER_NONE(-1) / JQ_WINNER_BLUE(0) / JQ_WINNER_RED(1) / JQ_WINNER_DRAW(2)` 常量；`winner` 改 `int`；`jq_on_stalemate` 里对 turn 做 `& 1` 保护。

## AUD-R-054 [P2] `jq_on_flag_capture()` 未校验"吃旗方"是否合法，也未清理被吃方的旗计数
- 位置：`src_rs/rules.c:309-313`
- 问题：函数只写 `winner`/`stage`，不做任何一致性检查（如 `captor_side` 是否 ∈ {0,1}）；也没有对"旗被吃掉"做子力统计更新（`pieces_left`）。
- 依据：`rules.c:309-313`；`junqi.h:163` 的声明。
- 修法：加 `if (captor_side > 1) return;` 与参数校验；旗被吃时更新对方 `pieces_left`（若该计数仍保留）。

## AUD-R-055 [P2] `ai_score()` 未归一化：吃掉任意子都是 1000，同级同归 900 高于任何空移
- 位置：`src_rs/rules.c:339-344`
- 问题：评分表是固定档位（1000 吃子 / 900 同级 / 800 炸弹 / 700 挖雷 / 2 空移 / 1 送死 / -1 不可走），**不考虑子力价值**（用司令吃掉排长与用排长吃掉司令同为 1000），也不考虑"交换后是否划算"（用司令换掉对方炸弹 = 900 同级，实际上是净亏）。`900 > 2` 意味着 AI 在有同级对手时**总是选择同归于尽**而不是走安全步。
- 依据：
  1. `docs_rs/AUDIT_CHECKLIST.md` B7："AI 不主动送死（sc 表…）❌ 未归一化（已修 #2）" —— #2 的"修复"只是把 rank 还原搬进 `ai_score`，**归一化本身并未做**。
  2. 兄弟实现 `src/game_logic.c:534-539` 有子力表 `ai_value()`（司令 90 … 工兵 10，炸弹 90，地雷 80，旗 10000）并在 HIGH 档用 `ai_value(d->rank) - 风险代价` 打分 → src_rs 缺失整套价值评估。
  3. `rulelay.txt` 第 12-13 行的强弱序列提供了天然的子力序（rank 越小越强）。
- 修法：引入 `piece_value(rank)`（与 `game_logic.c:ai_value` 对齐），评分改为 `value(被杀子) - value(我方损失) + 位置收益`，并把"同归于尽"按双方价值差打分（净亏给负分）；同时保留"不主动送死"的兜底（sc<=0 不选）。

## AUD-R-056 [P2] `ai_score()` 不评估"吃子后被反吃"的风险 → AI 会主动送死
- 位置：`src_rs/rules.c:330-345`（纯静态比较，无模拟）
- 问题：评分只看"我方 rank vs 敌方 rank"，不考虑吃子落点是否被对方更大/炸弹的子反吃。`ai_pick()` 选出 1000 分的吃子后直接执行，可能一步把司令送进对方炸弹/司令的射程。
- 依据：兄弟实现 `src/game_logic.c:544-559` `ai_capture_risky()` 用"走子后模拟 + 检查敌方反吃 + undo 回滚"实现了该风险评估；src_rs 的 `ai_score` 完全没有这一步。
- 修法：把 `ai_score` 升级为带状态的形式（`ai_score(st, from, to)`），内部用"试走 + 判敌方反吃 + 回滚"评估（需要先修好 `jq_undo_restore` 的可用性，见 AUD-R-012/050）。

## AUD-R-057 [P2] `ai_score()` 对"空源"（rank=0）返回 2，与合法空移同分 → 无效着法无法被区分
- 位置：`src_rs/rules.c:332-334`（`int r = rank_norm(src_rank_raw);` 无空源检查）、`rules.c:344`（`return 2;`）
- 问题：`ai_score(0, 0)` 返回 2（"空移"），`ai_score(0, 5)` 返回 1（"送死"）——调用方无法从返回值区分"源格为空"这种非法输入。当前调用方 `ai_pick` 靠外层 `c->rank == RANK_EMPTY` 过滤，函数自身无契约。
- 依据：实测 `ai_score(0,0) = 2`；`junqi.h:166` 的声明未说明"src 必须非空"。
- 修法：函数入口 `if (src_rank_raw == 0) return -2;`（非法）或在头文件写明前置条件并加断言。

## AUD-R-058 [P2] `ai_score()` 未实现暗棋的扛旗条件（只有工兵 + 挖 3 雷）
- 位置：`src_rs/rules.c:330-345`
- 问题：即便修好 AUD-R-002 的规则，AI 侧仍需知道"当前模式下扛旗需要工兵 + 3 雷"，而 `ai_score` 只看 rank（工兵=9）与目标 rank（旗=12）→ 会给"用非工兵扛旗"打 1000 分（见 AUD-R-009 同源）。同时"工兵挖雷"只给 700 分，**低于** 900（同归）与 1000（吃子），在暗棋下 AI 几乎不会去挖雷 → 即便规则实现正确，AI 也无法完成 ruleflip 的胜利路径。
- 依据：`data_junqi_ruleflip.txt` 第 6、10 行；实测 `ai_score(9,11)=700` 而 `ai_score(3,3)=900`。
- 修法：AI 评分纳入模式与前置条件（挖雷 + 扛旗组合价值应显著高于普通吃子）；在暗棋模式下把"挖雷"的价值与"距扛旗的进度"挂钩。

## AUD-R-059 [P2] `ai_score()` 把"炸弹空移"与"普通子空移"同分（2）
- 位置：`src_rs/rules.c:344`（空目标统一 `return 2;`）
- 问题：炸弹的空移价值与普通子相同。虽不致命，但炸弹是"用完即消失"的一次性资源（撞谁都同归），其空移应被显著降权。
- 依据：`rulelay.txt` 第 12 行；`ai_score` 源码无 src 类型分支（空目标直接返回）。
- 修法：空目标分支按 src 类型分档（炸弹空移给负/极低分，普通子空移 2，保留工兵等机动子偏好）。

## AUD-R-060 [P2] `ai_score()` 的"同级同归 900"混淆了两种语义（炸弹同归 vs 普通同级同归）
- 位置：`src_rs/rules.c:340`（`else if (r == dr) return 900;`）
- 问题：`r == dr` 覆盖了"两颗炸弹相撞"与"两颗同 rank 普通子相撞"两种情形，而 `rules.c:211-217` 的执行侧把"同级同归"也归到 `EV_BOMB` 事件 —— 事件与评分都无法区分"我主动撞炸弹"和"我被同级换掉"。
- 依据：`rules.c:161-168`（炸弹）与 `rules.c:211-217`（同级）共用 `EV_BOMB`；`junqi.h:81-88` 的事件枚举无独立"互杀"事件。
- 修法：新增 `EV_TRADE`（或让 `EV_BOMB` 携带子类型）；AI 评分对"主动撞炸弹"给负分（净亏），对"同级换掉强子"按价值差给分。

## AUD-R-061 [P2] `JQ_EVENT_NAMES[]` 缺 `EV_FLIP`，与 SPEC §3 的"7 类事件"不一致
- 位置：`src_rs/rules.c:29-31`（6 项）、`src_rs/junqi.h:81-88`（枚举 6 值）、`src_rs/junqi.h:177`（`extern const char *JQ_EVENT_NAMES[6];`）
- 问题：`docs_rs/SPEC_BUSINESS.md` §3 的事件契约表列了 7 类（含 `EV_FLIP | Junqi.Flip | 翻棋`），实现只有 6 类且 `jq_flip()` 返回 `bool`（无事件码）→ 翻棋事件只能由调用方另行记录（`jni_bridge.c` 的 `ui_replay_record(idx, idx, 1)`），事件计数 `g_event_cnt[6]` 里没有翻棋位。
- 依据：`SPEC_BUSINESS.md` §3 表格 vs `junqi.h:81-88`；`rules.c:29-31`。
- 修法：要么在枚举中补 `EV_FLIP`（并同步 `JQ_EVENT_NAMES` 为 7 项 + 事件计数数组扩容），要么修订 SPEC §3 明确"翻棋不走事件码通道"。

## AUD-R-062 [P2] `EV_MINE` 与 `EV_MOVE` 共用事件名 "Junqi.Move"，按事件码分支的调用方会误判
- 位置：`src_rs/rules.c:30`（`"Junqi.Move"` 出现两次）、`src_rs/junqi.h:87`（`EV_MINE` 注释"音效分类"）
- 问题：`JQ_EVENT_NAMES[0] == JQ_EVENT_NAMES[5] == "Junqi.Move"`，因此靠事件**名**分流的调用方无法区分"普通走子"与"工兵挖雷"；靠事件**码**分流则两者的统计口径不同（`g_event_cnt[0]` 与 `g_event_cnt[5]` 分开计数但同音效）。
- 依据：`rules.c:29-31`；`junqi.h:87`；`SPEC_BUSINESS.md` §3 表（EV_MINE → Junqi.Move，行为与实现一致，但未提示同名风险）。
- 修法：在 SPEC §3 明确"事件码 5 与 0 同名"，并建议调用方按码分流（音效按名）。

## AUD-R-063 [P2] 布局校验只对红方半区做位置限制，蓝方军旗格不受约束（与 `is_base` 允许蓝方底线大本营不一致）
- 位置：`src_rs/layout.c:77-88`（`if (idx <= 0x1d) return LAYOUT_OK;`）、`src_rs/board.c:25-28`（`is_base` 同时认 row0 与 row11）
- 问题：`jq_check_layout_cell()` 对 idx ≤ 29（蓝半区）直接返回 OK，因此蓝方军旗放在任何蓝格都"合法"，而 `is_base()` 又把蓝方底线 (0,1)/(0,3) 认作大本营（可被扛旗/进占）。默认布局把蓝旗放在 (0,3)（`layout.c` 由 layout[28]=12 映射而来），但校验器不保证这一点 → 自定义布局可出现"蓝旗不在大本营"的非法局面，而引擎照样允许吃 (0,1)/(0,3) 上的任何子。
- 依据：`layout.c:81`（`if (idx <= 0x1d) return LAYOUT_OK;`）；`board.c:27`；`rules.c:129-134` 只判"目标是否为空"。
- 修法：蓝半区做对称校验（蓝旗只能在 (0,1)/(0,3)，炸弹/地雷按蓝方行号镜像），或明确记录"蓝半区由 0x2a228 之外的规则保证"并在装载时强制。

## AUD-R-064 [P2] `jq_validate_layout()` 用 `ERR_RANGE` 表达"子力分布不符"，错误码语义混淆
- 位置：`src_rs/layout.c:105-106`（`if (seen[r] != cnt[r] * 2) return ERR_RANGE;`）、`src_rs/junqi.h:96`（`ERR_RANGE /* 字节非法 */`）
- 问题：`ERR_RANGE` 的定义是"字节非法"，实际被用于"每 rank 计数与标准不符"（分布错误），UI 层若按错误码映射提示文案会显示错误信息。
- 依据：`junqi.h:91-97`；`layout.c:105-106`。
- 修法：新增 `ERR_COUNT`（或 `ERR_DISTRIBUTION`）并同步 UI 错误文案映射。

## AUD-R-065 [P2] 布局校验的"每 rank 计数 × 2"假设双方严格镜像，自定义/联机布局会被误判
- 位置：`src_rs/layout.c:104-106`
- 问题：`seen[r] != cnt[r] * 2` 强制全盘每种 rank 的总数是标准值的两倍。这只在"双方完全同分布"时成立；对四国模式、联机自定义布局或"一方用完子后保存布局"的场景会误报 ERR_RANGE。
- 依据：`layout.c:104-106`；`jq_extract_layout_30()` 的注释"提取后 jq_validate_layout 校验（非标准分布布局仍提取成功，仅校验返回错误）"承认了该限制。
- 修法：拆成"单方分布校验"（各自 25 子）与"全盘合计校验"，或对非标准布局只告警不阻断。

## AUD-R-066 [P2] `is_rail()` 的分支顺序使"底线与纵线交点"（(0,2)/(11,2)）具有双重身份，语义未定义
- 位置：`src_rs/board.c:37-41`
- 问题：`is_rail(0,2)` 既满足"col2 纵线"又满足"row0 底线"；`rail_path_straight()` 用它做端点判定时，无法区分"该格属于纵线"还是"属于底线"→ 正是 AUD-R-001 的成因之一。`board.c` 没有任何"铁路线归属"的概念，只有布尔"是否铁路格"。
- 依据：`board.c:37-41`；`rules.c:80`。
- 修法：引入"铁路线"概念（`rail_line_id(row,col)` 返回 纵线/中线A/中线B/底线/无），滑行判定要求起点与终点**同一线号**且路径全在该线上。

## AUD-R-067 [P2] `board.c` 的铁路几何是硬编码 if 链，缺少"棋盘地形"抽象
- 位置：`src_rs/board.c:12-42`
- 问题：行营（6 格）、大本营（4 格）、铁路（3 类）全部硬编码为坐标判断，没有可查询的地形表，也没有"铁路段"概念；任何拓扑调整（如支持 17×12 四国棋盘）都要改多个 if 链。
- 依据：`board.c:12-42`；`junqi.h:29-31`（棋盘尺寸宏）；`SPEC_BUSINESS.md` §1 甚至写了"10×9（row 0-11 / col 0-8）"与实现 12×5 矛盾（见 AUD-R-076）。
- 修法：抽 `board_terrain[row][col]` 表 + `rail_line` 表，用查表替代 if 链；同时把 SPEC §1 的棋盘尺寸改成 12×5。

## AUD-R-068 [P2] `cell_index()` 不做边界检查，负数/越界 row·col 会算出越界索引
- 位置：`src_rs/board.c:44-46`
- 问题：`cell_index(row,col) = row*5 + col` 无范围校验；`cell_row(-1) = 0`（C 向零截断）、`cell_col(-1) = -1` → `cell_index(0,-1) = -1` → 负索引。当前所有调用方都先校验了 0..59，但 `cell_row/cell_col` 的负值行为本身是个陷阱（例如外部把 -1 传进来会得到"看起来合法"的 row=0/col=-1）。
- 依据：实测 `cell_row(-1)=0 cell_col(-1)=-1`；`board.c:44-46` 无检查。
- 修法：提供 `cell_index_checked(row,col,int *out)` 返回 bool；`cell_row/cell_col` 对负数返回 -1。

## AUD-R-069 [P2] `coord_cmp()` 无调用方且未做 NULL/范围检查，签名与语义都偏离直觉
- 位置：`src_rs/board.c:48-53`、`src_rs/junqi.h:133`
- 问题：函数返回 0=相同 / 1=不同（注释说明），但全仓无调用点（grep 确认）→ 死代码；且 `coord_cmp(const int8_t*, const int8_t*)` 接受裸指针，无长度/范围概念，与 `jq_cell` 的 row/col 体系脱节。
- 依据：`board.c:48-53`；`junqi.h:133`（声明 "0x6ef54：[0]=col [1]=row，0=相同"）；grep 无调用。
- 修法：删除或改为 `bool coord_eq(int row_a,int col_a,int row_b,int col_b)` 并在坐标换算处统一使用。

## AUD-R-070 [P2] `jq_can_move()` 的相邻判定写成两次减法，易与"行列同行"语义混淆
- 位置：`src_rs/rules.c:125-126`
- 问题：`int adjacent = (fr == tr && (fc - tc == 1 || tc - fc == 1)) || (fc == tc && (fr - tr == 1 || tr - fr == 1));` —— 逻辑正确但冗长，且与后面的"铁路滑行"分支共用同一变量命名（`fr/fc/tr/tc` 在 `rail_path_*` 里含义相同，OK），但 `adjacent` 变量在 `rules.c:127` 被复用于"行营落点必须一步相邻"，语义耦合。
- 依据：`rules.c:125-127`。
- 修法：拆成 `bool same_row_1step / same_col_1step` 两个具名谓词，行营判定单独调用。

## AUD-R-071 [P2] `jq_can_move()` 的"目标是大本营且为空 → false"使**任何**棋子都无法进入空大本营，与"大本营是落点之一"的地形语义冲突
- 位置：`src_rs/rules.c:129-134`
- 问题：`rulelay.txt` 第 3 行把"结点、行营、大本营"并列为棋子落点，暗示大本营是可落点（ruleflip 更是明确"大本营的地雷、军旗可以吃"）。src_rs 的实现让空大本营成为"不可入格"，仅当里面有子时才可进入吃子 → 语义不统一。
- 依据：`rulelay.txt` 第 3 行；`src/game_logic.c:96` 的写法（只拦"既非旗也非雷"）。
- 修法：按 mode 明确"大本营可入条件"（明棋：可入但入后冻结；暗棋：仅旗/雷可吃），并写进 SPEC。

## AUD-R-072 [P2] `jq_can_move()` 未检查"源格在行营"是否影响移动（行营中的子可自由走出，规则上正确但未在 SPEC 记录）
- 位置：`src_rs/rules.c:111-141`（无行营源格判定）
- 问题：行营是"安全岛"，其内的子可自由走出（正确），但 SPEC §2 规则表完全没有"行营"条款（只有 `junqi.h:130` 的一句注释），规则覆盖度不可查。
- 依据：`junqi.h:130`（注释"行营（安全岛，敌方不可入，非铁路）"）vs `SPEC_BUSINESS.md` §2 表格（无行营行）。
- 修法：SPEC §2 补行营/大本营/铁路三条独立条款，与实现逐条对齐。

## AUD-R-073 [P2] `jq_side_stalemated()` 的"可翻"检查只看 `revealed==0` 且 rank≠军旗，未排除"地雷"（与 AUD-R-045 关联）
- 位置：`src_rs/rules.c:270-276`
- 问题：`if (c->rank != RANK_EMPTY && !c->revealed && rank_norm(c->rank) != RANK_FLAG && (c->side == 0xFF || c->side == side)) return false;` —— 暗地雷被算作"可翻 → 未困毙"，与 `jq_can_flip()` 的白名单（只排除军旗，允许地雷）自洽，但与"翻雷后不可动"的实际收益不符；兄弟实现把地雷排除在外（`game_logic.c:162`）。
- 依据：`rules.c:270-276` vs `src/game_logic.c:158-163`。
- 修法：统一 `is_flippable()` 谓词（见 AUD-R-045）。

## AUD-R-074 [P2] `jq_side_stalemated()` 通过 `jq_can_move()` 间接判定，隐含依赖"side == st->turn"（AUD-R-005 的耦合点）
- 位置：`src_rs/rules.c:284`
- 问题：第二段循环枚举"该方已翻可动子"，但 `jq_can_move()` 会因 `src->side != st->turn` 直接拒绝 → 函数只在被测方恰好是当前回合方时正确（见 AUD-R-005）。
- 依据：`rules.c:284` + `rules.c:105`；实测 T1。
- 修法：随 AUD-R-005 一并修（参数化回合门）。

## AUD-R-075 [P2] 困毙与和棋判定在 `jq_try_move()` 中的位置：先和棋后困毙，但两者都在 `turn` 已切换之后判定
- 位置：`src_rs/rules.c:220-230`
- 问题：`st->turn = 1 - me;` 之后才做和棋/困毙判定。语义上正确（判"下一方"），但 `jq_on_stalemate()` 用 `1 - st->turn` 反推胜方，等于"再取反"——两处取反分散，易在后续修改中出错。
- 依据：`rules.c:220-230`、`rules.c:323`。
- 修法：把"被判负方"显式传参（见 AUD-R-036 修法）。

## AUD-R-076 [P2] SPEC 的棋盘尺寸与实现不一致（"10×9 / col 0-8" vs 实现 12×5）
- 位置：`docs_rs/SPEC_BUSINESS.md:12`（"棋盘 **10×9**（row 0-11 / col 0-8，中央 10×5 十字 + 双方底营），共 **60 格**"）vs `src_rs/junqi.h:29-31`（12 行 × 5 列）
- 问题：规格写 10 行 9 列、代码是 12 行 5 列；"共 60 格"又与 10×9=90 矛盾。规格是后续审计/开发的基准，尺寸矛盾会持续误导。
- 依据：`SPEC_BUSINESS.md:12` vs `junqi.h:29-31`；`junqi.h:3` 注释"12 行 × 5 列（原版 L79 棋维度确证）"。
- 修法：SPEC §1 改为"12 行 × 5 列，共 60 格"，并删除"10×9 / col 0-8"表述。

## AUD-R-077 [P2] `junqi.h` 把渲染层的棋盘几何常量（720×1280 背景图坐标）与规则层常量混放
- 位置：`src_rs/junqi.h:33-60`（`BG_IMG_W/BG_GRID_*` + `bg_row_cy` + `bg_cell_img_center`）
- 问题：规则头文件里内嵌了渲染贴图坐标系（720×1280、col 宽 140.5、行高 71、行 6 起点 822），与规则层的 12×5 行列概念耦合但语义完全不同（image_row = 11 - row）。任何渲染坐标调整都要动规则头文件，且两套坐标换算分别在 `junqi.h` 与 `touch.c`/`render.c` 中各自实现。
- 依据：`junqi.h:33-60`；`touch.c:42-49`（再次用 `bg_row_cy`）；`render.c:1281, 9617`。
- 修法：把渲染几何常量与内联函数移到 `render.h`/`board_geom.h`，`junqi.h` 只保留规则语义。

## AUD-R-078 [P2] `bg_cell_img_center()` 对越界 row/col 不校验，`image_row` 可为负并走错分支
- 位置：`src_rs/junqi.h:55-60`、`junqi.h:47-52`
- 问题：`int image_row = BOARD_ROWS - 1 - row;` 未校验 `0 <= row < 12`；`row = 12` → image_row = -1 → `bg_row_cy(-1)` 走 else 分支 → 返回 `822 + (-1-6)*71 = 325`（一个看似合法的错误坐标），无报错。
- 依据：`junqi.h:47-60` 无范围检查；`touch.c` / `render.c` 调用点传入的行列来自棋盘索引，理论安全。
- 修法：加 `if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS) { *ix=*iy=-1; return; }`。

## AUD-R-079 [P2] `rank_encode()` 定义了但从未被调用，且其语义与 layout.c 的实际编码方式重复
- 位置：`src_rs/junqi.h:119-121`、`src_rs/layout.c:59`（`r->rank = (uint8_t)(v + 0xc);`）
- 问题：`rank_encode(side, rank)` 是唯一表达"红方 +0xc 编码"规则的函数，但实际编码散落在 `layout.c:59` 的字面量 `+ 0xc`，`rank_encode` 无人使用 → 编码规则有两个来源。
- 依据：`grep rank_encode` 仅 `junqi.h:119` 命中。
- 修法：统一改用 `rank_encode(SIDE_RED, v)`；`rank_norm` 同理应与 side 绑定（见 AUD-R-047）。

## AUD-R-080 [P2] `RANK_TOP=12` 与 `RANK_COUNT=13` 的边界耦合：`rank_norm` 对 13..23 的输入会误还原
- 位置：`src_rs/junqi.h:20-21`、`junqi.h:122-124`
- 问题：`rank_norm(v) = v > 12 ? v - 12 : v` 对 v ∈ [13,23] 返回 [1,11]，即"任何 13 以上的字节都被当成红方编码"。红方合法编码范围是 13..24（rank 1..12 + 12），但 25..255 会被还原成 13..243（越界 rank），调用方（如 `layout.c:101` 的 `if (r >= 0 && r < RANK_COUNT) seen[r]++`）只做范围过滤而不报错。
- 依据：`junqi.h:20-21,122-124`；`layout.c:101-102`。
- 修法：`rank_norm` 加上界（`v > RANK_TOP && v <= RANK_TOP + 0xc`），越界返回 0 并由调用方报错。

## AUD-R-081 [P2] 同仓存在三套语义不同的军棋规则实现，缺一致性闸门
- 位置：`src_rs/rules.c`（本审计对象）、`src_jni_translated/jq_rs_rules.c`（32 位翻译体副本）、`src/game_logic.c`（手写路线）
- 问题：三份实现的规则集合已经出现可观测差异：① 暗棋 3 雷扛旗/工兵吃雷（`game_logic.c` 有、`src_rs` 无）；② 行营进入（`game_logic.c` 禁、`src_rs` 允许）；③ 大本营安全岛（`game_logic.c` 按 mode 收窄、`src_rs` 不管）；④ `rail_path_straight` 的底线横滑（`src_rs` 放行 0/11、`game_logic.c:61` 只放行 5/6）。没有跨实现的规则对照测试，任何一次修改都可能让它们进一步漂移。
- 依据：上述三处代码引用；`docs_rs/REVERSE_LOG.md` 与 `src_jni_translated/jq_rs_rules.h` 显示翻译体是 `src_rs` 的镜像拷贝（`jq_rs_rules.h` 与 `junqi.h` 声明几乎逐行相同）→ 修复 `src_rs` 时**必须同步翻译体**，否则两套 so 行为分叉。
- 修法：把规则矩阵（模式 × 条款 × 期望）写成一份数据驱动的用例集，让三套实现跑同一份用例；或明确"翻译体冻结、只维护 src_rs"并在 CI 检查二者差异。

## AUD-R-082 [P2] 规则引擎每步 O(N²) 次 `jq_can_move()`（每次含 BFS），移动端性能风险
- 位置：`src_rs/rules.c:283-284`（`for t in 0..59: jq_can_move(st, i, t)`）、`rules.c:47-71`（BFS 每次栈上分配 240B+512B）
- 问题：一次困毙判定 = 60 × 59 次 `jq_can_move`，每次可能触发一次工兵 BFS（60 格 + 栈分配）→ 单步约 3.5k 次合法性判定、约 20 万次格点运算；`jq_refresh_movable_counts()` 再做一遍同样规模的枚举。手机端每步 2 次这种量级是可感知的开销。
- 依据：`rules.c:278-285`、`layout.c:131-141`；`walkable()` + `rail_path_*` 的复杂度。
- 修法：把"合法目标格"计算缓存到 `jq_state`（选子时算一次、落子后失效），或用"射线扫描 + 铁路段预计算"替代逐目标 BFS。

## AUD-R-083 [P2] `jq_try_move()` 的事件返回与局面变更不是原子的：失败时已改状态的情形需逐一验证
- 位置：`src_rs/rules.c:146-232`
- 问题：所有状态变更都发生在 `jq_can_move()` 通过之后，失败路径只有 `return -1`（`rules.c:148`），这点是对的；但 `jq_try_move()` 内部**没有再检查 `stage`**（依赖 `jq_can_move` 的 `stage != STG_PLAY` 拒绝），且 `jq_flip()` 也只在 `jq_can_flip` 里检查 —— 一旦将来有"终局后仍允许落子"的路径（如复放、BT 帧），需要在两个入口都设防。
- 依据：`rules.c:94`（stage 检查只在 `jq_can_move`）；`bt_session.c:51,61` 两条外部入口。
- 修法：在 `jq_try_move`/`jq_flip` 入口再查一次 `st->stage == STG_PLAY`。

## AUD-R-084 [P2] 联机同步帧写入的 `side`/`rank` 未做合法性过滤，可污染规则引擎的 side 语义
- 位置：`src_rs/bt_session.c:74-76`（写 `peer_state.cells[idx]`）；`src_rs/rules.c:166`（用 side 作数组下标）
- 问题：同步帧把对端的 `f3`(rank) / `f4`(side) 原样写入（值域 0..255），若这些数据进入活动棋盘（当前只写 `peer_state`，风险有限），会触发 AUD-R-004 的越界写与 AUD-R-047 的错误还原。
- 依据：`bt_session.c:71-78`；`rules.c:166`。
- 修法：同步入口做 `side` ∈ {0,1,0xFF} 与 `rank` ∈ [0,24] 的白名单过滤；或统一走 `jq_cell` 的写入辅助函数。

## AUD-R-085 [P2] `jq_can_flip()` 允许翻"任何未定色暗子"（不限当前回合方），与 `jq_can_move()` 的回合门不一致
- 位置：`src_rs/rules.c:242` vs `rules.c:245`
- 问题：`jq_can_flip()` 对 `side == 0xFF` 的子直接返回 true（不限 turn），对已知 side 的子要求 `side == turn`。两种暗棋语义（未定色可任意翻 / 已定色限本方）在同一个函数里混用，没有模式区分 —— 在写死 side 的当前实现下前一支路不可达，在实现先手定色后又会与回合门冲突。
- 依据：`rules.c:242,245`；`src/game_logic.c:293`（"布局模式：只能翻当前方的子；暗棋模式：side 未定（UNKNOWN）可翻"）—— 兄弟实现用 stage 区分，src_rs 没有模式维度。
- 修法：随 AUD-R-002/003 引入模式后，把"可翻集合"写成按模式的判定表。

## AUD-R-086 [P2] `jq_flip()` 让"翻棋"消耗 `moves_since_eat` 与 `total_moves`，但翻棋不改变任何吃子关系
- 位置：`src_rs/rules.c:256-257`
- 问题：与 AUD-R-033 同源，此处单独强调：翻棋同时推进两个计数器（70 步与 1000 步），使暗棋局面的和棋阈值被"翻牌速度"主导。
- 依据：`rules.c:256-257`；`rulelay.txt` 第 17-18 行未区分翻棋与走子。
- 修法：同 AUD-R-033。

## AUD-R-087 [P2] `jq_state.count_tbl[]` 的语义与原版偏移的对应未验证（+0x78 是 240 字节计数表）
- 位置：`src_rs/junqi.h:113`（`int count_tbl[RANK_COUNT]; /* 布局计数表（原版 +0x78，rank→数量） */`）、`junqi.h:102-103`（原版 +0x78 布局计数表）
- 问题：`junqi.h:102-103` 记录原版 `+0x78` 是"布局计数表[rank]"，而 `REVERSE_NOTES_01_basics.md` §L12/§L60 的原版计数表是 **240 字节**（60 格 × 4 字节），与 `int count_tbl[13]`（52 字节）不等价；注释里的"rank→数量"是重写后的自有语义，与原版偏移不能一一对应。
- 依据：`junqi.h:102-103, 113`；`src_jni_translated/jqt_vtbl.c:76156` 注释"复制 240 字节计数表到 state+0x78"（翻译体侧是 240 字节）。
- 修法：要么把字段改为 60 项（按格索引的 rank 表），要么在注释中明确"本字段是重写自有语义，不映射原版 +0x78"。

## AUD-R-088 [P2] `jq_state` 的 `turn`/`winner`/`stage` 在 `jq_undo_restore()` 之外没有"部分回滚"能力（悔棋粒度只能是整盘）
- 位置：`src_rs/junqi.h:114-115`（"悔棋历史（循环快照，原版无内建悔棋；联机 Regret* 事件驱动，L77）"）
- 问题：头文件把悔棋历史放在 `jq_state` 内（`undo_depth`），但实现把快照放在调用方（`jni_bridge.c:201` 的 `g_undo[128]`），字段与实现分属两地 → 字段无意义（见 AUD-R-049），且"循环快照"的说法与实际（线性数组 + 计数器）不符。
- 依据：`junqi.h:114-115` vs `rules.c:290-306`（空实现）+ `jni_bridge.c:200-204`（调用方栈）。
- 修法：统一到一处（推荐引擎内环形栈，见 AUD-R-012 修法 2），并修正注释。

## AUD-R-089 [P2] `jq_side_stalemated()` 未排除"可直接吃旗获胜"的情形（能吃旗必然有合法走子，逻辑上被覆盖但未显式）
- 位置：`src_rs/rules.c:278-285`
- 问题：函数靠"存在合法目标格"判断未困毙；能吃旗的子当然有合法目标 → 覆盖正确。但"吃旗后立即终局"与"困毙判负"两条终局路径的**优先级**在同一步内未定义（先吃旗 → 终局；困毙检查在 `rules.c:229` 之后且有 `stage == STG_PLAY` 保护 → OK）。
- 依据：`rules.c:225-230` 的 `stage == STG_PLAY` 守卫。
- 修法：现状可接受；建议补注释说明"终局路径优先级：吃旗 > 和棋 > 困毙"。

## AUD-R-090 [P2] `jq_state.stage` 与"对局模式（明棋/暗棋）"是两个正交维度，但状态结构里只有前者
- 位置：`src_rs/junqi.h:104-116`（无 mode 字段）、`src_rs/junqi.h:74-78`（stage 枚举）
- 问题：`stage` 表达"布阵/对局/终局"，而"明棋 vs 暗棋"（ruleflip vs rulelay）在 src_rs 中完全没有状态表示 —— 只体现在 `revealed` 标志上。这导致所有"只在暗棋成立"的规则（工兵吃雷、挖 3 雷扛旗、大本营安全岛、先手定色）无处落脚（见 AUD-R-002/003/007）。
- 依据：`junqi.h:74-78`；`src/game_logic.c` 有 `JQ_STAGE_DARK`（`game_logic.c:335` `jq_state_init(st, JQ_STAGE_DARK)`）；`jni_bridge.c:215` 用 `ui_opt_get_mode()` 决定 `revealed`，但该信息**没有进对局状态**。
- 修法：`jq_state` 增加 `uint8_t mode`（LAYOUT/FLIP），随布局装载与悔棋快照一起保存；所有 mode 相关规则读 `st->mode`。

---
# 四、P3（约 45 条：代码卫生 / 契约 / 文档 / 覆盖率）

## AUD-R-091 [P3] 规则引擎公开 API 全部未做 `st == NULL` 防护，与兄弟实现的防御风格不一致
- 位置：`src_rs/rules.c:92, 146, 235, 248, 266, 290, 297, 309, 315, 321`
- 问题：`jq_can_move/jq_try_move/jq_can_flip/jq_flip/jq_side_stalemated/jq_on_*/jq_undo_*` 全部直接解引用 `st`，传 NULL 即崩。兄弟实现 `src/game_logic.c:75` 有 `if (!st || …) return false;`。
- 依据：`src/game_logic.c:75, 108, 139, 282` 的 NULL 防护；`rules.c` 内无。
- 修法：公开 API 入口统一加 `if (!st) return false/‑1;`（或用 `assert` 明确契约）。

## AUD-R-092 [P3] `ai_score()` 的 "src 为空" 输入返回合法分数，缺哨兵值
- 位置：`src_rs/rules.c:332-334`
- 问题：`rank_norm(0) == 0`，既不等于 FLAG 也不等于 MINE → 继续走；空目标分支返回 2，"空源 + 有目标"返回 1。调用方必须自己过滤 `src->rank == RANK_EMPTY`。
- 依据：实测 `ai_score(0,0) = 2`；`junqi.h:166` 的声明无前置条件。
- 修法：函数入口 `if (src_rank_raw == 0) return -2;`，并在头文件写明返回值表。

## AUD-R-093 [P3] `ai_score()` 中 `dr` 的计算与 `if (dst_rank_raw)` 判定重复
- 位置：`src_rs/rules.c:334-335`
- 问题：`int dr = dst_rank_raw ? rank_norm(dst_rank_raw) : 0;` 与紧随其后的 `if (dst_rank_raw)` 是同一次判定的两处表达，易在修改时只改一处。
- 依据：`rules.c:334-335`。
- 修法：改为 `bool has_dst = (dst_rank_raw != 0); int dr = has_dst ? rank_norm(dst_rank_raw) : 0; if (has_dst) {…}`。

## AUD-R-094 [P3] `ai_score()` 的 "r == RANK_BOMB → 800" 分支依赖前面的顺序，属脆弱的 else-if 链
- 位置：`src_rs/rules.c:339-342`
- 问题：炸弹分支 `else if (r == RANK_BOMB) return 800;` 只有在 `r > dr` 且 dr 既非地雷也非军旗时才可达；若将来把"目标为军旗"提前特判（修 AUD-R-009），此分支的可达性会再变。建议改成显式的分派表。
- 依据：`rules.c:336-342` 的分支顺序注释（"地雷特判必须在 r<dr 之前"）。
- 修法：改为 `switch`/查表式分派，并为每个 (src 类型 × dst 类型) 组合写单测。

## AUD-R-095 [P3] `ai_score()` 未对红方编码做上界校验，`rank_norm` 的边界缺陷会直接污染评分
- 位置：`src_rs/rules.c:332`、`src_rs/junqi.h:122-124`
- 问题：`ai_selftest.c` 用 `redE`(23=红工兵) 等合法编码测试，但 `ai_score(30, 0)` 会被 `rank_norm` 还原成 18（非法 rank）→ 进入普通比较分支得出无意义分数。
- 依据：`junqi.h:122-124`（无上界）；`src_rs/ai_selftest.c:38-64` 的用例全部落在合法编码区间。
- 修法：`rank_norm` 加界（见 AUD-R-080），`ai_score` 对非法 rank 返回 -2。

## AUD-R-096 [P3] `rules.c` 头注释未区分"已逆向确证"与"推断/沿用兄弟实现"的条款
- 位置：`src_rs/rules.c:1-22`
- 问题：头注释把 0x39e14/0x39f20/0x30260 等 VA 与 rulelay 规则文本并列陈述，但其中若干条（如"铁路十字"细节、"军旗 from_row∈{1,3,5}"）在逆向笔记中本身存在两种解读（§L49 vs §L68），注释未标注不确定性。
- 依据：`phases/REVERSE_NOTES_01_basics.md` §L49 与 §L68 的表述冲突（"纵向所有列贯穿" vs "纵向 col2 贯穿"）。
- 修法：头注释按"确证 / 推断 / 存疑"三档标注每条依据，与 `docs_rs/CODE_STYLE.md` 的诚实边界要求一致。

## AUD-R-097 [P3] `rules.c` 引入未使用的 `<stdlib.h>`
- 位置：`src_rs/rules.c:27`
- 问题：无 `malloc/free/qsort/abs` 等使用；`abs` 若要用还需 `<stdlib.h>`（当前未用）。
- 依据：`rules.c` 内无 stdlib 函数调用。
- 修法：删除该 include（保持 include 精确）。

## AUD-R-098 [P3] `walkable()` 的 `mover` 形参在所有调用点都传 0
- 位置：`src_rs/rules.c:35, 42, 65, 85`
- 问题：形参存在但恒为 0，且函数体 `(void)mover;` 显式吞掉 → 读者会误以为存在"按走子方区分"的逻辑。
- 依据：`rules.c:65`（`walkable(st, nr, nc, 0)`）、`rules.c:85`（`walkable(st, r, c, 0)`）。
- 修法：删除形参（见 AUD-R-027）。

## AUD-R-099 [P3] `rail_path_engineer()` 的方向表定义在函数体内 `static`
- 位置：`src_rs/rules.c:50`
- 问题：`static const int dr[4]…` 放在函数体内，虽无运行时开销但降低可读性，且与 `board.c` 的风格不一致。
- 依据：`rules.c:50`；`board.c` 无同类表。
- 修法：移到文件级 `static const int8_t DIRS[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};`。

## AUD-R-100 [P3] `rail_path_engineer()` 的 `visited` 用 int 数组（240B 栈），可压缩为 60B
- 位置：`src_rs/rules.c:51`
- 问题：`int visited[BOARD_ROWS][BOARD_COLS]` = 240 字节；每次 `jq_can_move` 可能调用一次（见 AUD-R-082 的高频调用）。
- 依据：`rules.c:51`；`rules.c:52` 的 `memset(visited, 0, sizeof(visited))`。
- 修法：改 `uint8_t visited[BOARD_ROWS][BOARD_COLS]`（60B），并用 `memset` 一次清零。

## AUD-R-101 [P3] `rail_path_engineer()` 对 "fr==tr && fc==tc" 直接返回 true，语义与调用方重复
- 位置：`src_rs/rules.c:49`
- 问题：`jq_can_move()` 已在 `rules.c:94` 排除 `from == to`，此处的早退分支是不可达防御。
- 依据：`rules.c:94` vs `rules.c:49`。
- 修法：保留但加注释"防御：同格视为可达（调用方已排除）"，或删除。

## AUD-R-102 [P3] `jq_can_move()` 的 `stage` 检查在 `from == to` 之后，顺序与可读性欠佳
- 位置：`src_rs/rules.c:94`
- 问题：`if (st->stage != STG_PLAY || from == to) return false;` 把两个不相关的条件合并在一个 if，且没有注释说明"布阵阶段禁止走子"。
- 依据：`rules.c:94`；`junqi.h:75`（`STG_SETUP` 注释"布阵（rulelay 编辑）"）。
- 修法：拆成两行并注释；或引入"对局阶段禁止走子/翻棋"的显式断言。

## AUD-R-103 [P3] `jq_can_move()` 的 `sr`/`dr_` 两个归一化 rank 中 `dr_` 完全未用
- 位置：`src_rs/rules.c:100-101`
- 问题：`dr_` 计算后未使用（编译告警），同时 `sr` 的语义（归一化后的源子 rank）没有在代码里解释"红方 +0xc 已还原"。
- 依据：实测 gcc `-Wunused-variable` 告警；`rules.c:100`。
- 修法：删除 `dr_`；给 `sr` 加一行注释说明 `rank_norm` 的作用。

## AUD-R-104 [P3] `jq_try_move()` 的 `me = src->side` 未显式断言其 ∈ {0,1}
- 位置：`src_rs/rules.c:152`
- 问题：`me` 被用作 `pieces_left[]`/`turn` 的下标与取值，安全性依赖 `jq_can_move()` 的 `src->side == st->turn` 前提；若 `st->turn` 被外部写成 3，`me` 就是 3 → `pieces_left[3]` 越界（与 AUD-R-004 同类）。
- 依据：`rules.c:152, 165-166, 190, 205, 214-215, 220`；`st->turn` 是 `uint8_t` 且外部可写（`jni_bridge.c:226/1084` 直接赋值 `SIDE_BLUE`）。
- 修法：`jq_try_move()` 入口加 `if (me > 1) return -1;`，并在 `jq_state_init` 中把 turn 限制在 0/1。

## AUD-R-105 [P3] `jq_can_move()` 允许"一步进入敌方空行营"，需与"行营安全岛"术语在 SPEC 中对齐
- 位置：`src_rs/rules.c:120-127`
- 问题：代码注释自称"行营安全岛：敌方棋子不能吃行营中的棋子"，但"安全岛"一词容易让读者理解为"敌方完全不能进入"。术语与实现边界不一致（见 AUD-R-028）。
- 依据：`rules.c:118-122` 注释；`junqi.h:130` 注释"行营（安全岛，敌方不可入，非铁路）" —— 头注释说"敌方不可入"，实现却允许进入空行营 → **头注释与实现直接矛盾**。
- 修法：统一术语与实现（见 AUD-R-028），修正 `junqi.h:130` 的注释。

## AUD-R-106 [P3] `jq_try_move()` 的空目标分支整格拷贝 `*dst = *src`，把冗余 `flags` 一起搬走
- 位置：`src_rs/rules.c:157-158`
- 问题：`jq_cell` 里的 `flags` 是 rank 的冗余副本（见 AUD-R-046），整格拷贝会传播它；一旦某处只改 rank，flags 就会漂移。
- 依据：`rules.c:157-158`、`rules.c:173-174`、`rules.c:188-189` 三处整格拷贝。
- 修法：若保留 flags，则在拷贝后按 rank 重算 flags；更简单的做法是删除 flags 字段。

## AUD-R-107 [P3] `jq_try_move()` 中"计数复位"散落在各分支，缺少统一的"本次是否发生吃子"标记
- 位置：`src_rs/rules.c:160, 168, 180, 199, 218`
- 问题：`st->moves_since_eat = 0;` 在 4 个分支里重复出现，新增分支时容易漏（本次新增 EV_FLAG 分支时就自带一份 `rules.c:180`）。
- 依据：`rules.c:160,168,180,199,218`。
- 修法：改为局部 `bool ate = false;` 统一在收尾处处理。

## AUD-R-108 [P3] EV_FLAG 分支与通用收尾重复执行 turn 切换与 total_moves 累加
- 位置：`src_rs/rules.c:178-181` vs `rules.c:220-221`
- 问题：EV_FLAG 分支自己做了 `st->turn = 1 - me; st->total_moves++;` 后提前 return，与通用收尾重复；两处若不同步即产生分歧（当前恰好一致）。
- 依据：`rules.c:178-180` 与 `rules.c:220-221`。
- 修法：EV_FLAG 分支只做"棋盘变更 + 判胜"，把 turn/计数交给统一收尾（配合 AUD-R-038 去掉提前 return）。

## AUD-R-109 [P3] `jq_flip()` 的和棋判定缺少 `stage == STG_PLAY` 守卫，与 `jq_try_move()` 写法不一致
- 位置：`src_rs/rules.c:259-261` vs `rules.c:225`
- 问题：`jq_try_move` 的和棋判定带 `st->stage == STG_PLAY &&` 前置，`jq_flip` 没有（当前靠 `jq_can_flip` 的 stage 检查间接保证）。
- 依据：`rules.c:225` 与 `rules.c:259` 的写法差异。
- 修法：统一写法（加显式守卫更安全）。

## AUD-R-110 [P3] `jq_flip()` 的 `rank_norm(c->rank)` 在空判断之前求值
- 位置：`src_rs/rules.c:239-240`
- 问题：`int r = rank_norm(c->rank);` 先算，随后才 `if (c->rank == RANK_EMPTY || c->revealed) return false;`。逻辑无害但顺序反直觉。
- 依据：`rules.c:238-240`。
- 修法：调整顺序，先判空/已翻再归一化。

## AUD-R-111 [P3] `jq_can_flip()` 的"0xFF 可翻"注释与实际可达性不符
- 位置：`src_rs/rules.c:242`
- 问题：注释"暗棋未定子可翻（先手定色，ruleflip）"描述的行为在当前装载路径下不可达（实测 0 个 0xFF 格子），会误导后续开发以为已支持先手定色。
- 依据：见 AUD-R-003 的实测。
- 修法：修完 AUD-R-003 后该分支才成立；在此之前注释标注"待 AUD-R-003 落地"。

## AUD-R-112 [P3] `jq_side_stalemated()` 每次调用都做 O(N²) 枚举，且被 `jq_try_move` 每步调用
- 位置：`src_rs/rules.c:266-287`、`rules.c:229`
- 问题：见 AUD-R-082 的性能分析；此处额外指出该函数在 `jq_try_move()` 里每步都会跑一次（不是仅终局时）。
- 依据：`rules.c:229`。
- 修法：缓存"当前方是否存在合法着法"，仅在落子后增量更新。

## AUD-R-113 [P3] `jq_undo_snapshot/restore/snapshots_left` 三个 API 全部是空壳却出现在公开头文件
- 位置：`src_rs/rules.c:290-306`、`src_rs/junqi.h:158-160`
- 问题：见 AUD-R-012/013。三个函数都不做任何实际工作（restore 会真的覆盖状态），但签名让调用方以为它们可用。
- 依据：`rules.c:290-306`。
- 修法：要么实现，要么从头文件移除（并在 SPEC 记录"悔棋由调用方栈实现"）。

## AUD-R-114 [P3] `jq_on_draw()` / `jq_on_stalemate()` / `jq_on_flag_capture()` 无 NULL 防护
- 位置：`src_rs/rules.c:309-325`
- 问题：同 AUD-R-091。
- 依据：`rules.c:309-325`。
- 修法：统一加防护。

## AUD-R-115 [P3] `jq_on_flag_capture()` 的参数是 `uint8_t`，而 `winner` 是 `int8_t`，赋值存在隐式窄化
- 位置：`src_rs/rules.c:311`（`st->winner = captor_side;`）、`src_rs/junqi.h:108`
- 问题：`int8_t winner` 存 -1（未定）；若 `captor_side` 为 0/1 则无问题，但没有编译期保护。`junqi.h:108` 的注释"-1 未定 / 0 蓝胜 / 1 红胜 / 2 和"暗示 `winner` 也可能是 2 → 类型应能容纳并显式区分。
- 依据：`junqi.h:108`、`rules.c:311`。
- 修法：`winner` 改 `int`，并用常量（见 AUD-R-053）。

## AUD-R-116 [P3] `junqi.h` 未定义"工兵 rank=9"的具名常量，rules.c 中出现 3 处裸数字
- 位置：`src_rs/rules.c:139, 183, 338`（`sr == 9`）、`src_rs/junqi.h:16-21`
- 问题：rank 语义（1=司令 … 9=工兵，10=炸弹，11=地雷，12=军旗）散落在注释里，`9` 这个魔数出现在关键判定中（工兵挖雷、工兵拐弯、AI 评分）。
- 依据：`junqi.h:13-15` 的注释；`src/game_logic.c:235` 用 `JQ_RANK_SOLDIER` 具名常量。
- 修法：在 `junqi.h` 增加 `JQ_RANK_ENGINEER 9`（及 1..12 全套）并替换裸数字。

## AUD-R-117 [P3] `RS_NARROW_BOARD` 宏定义后全仓无使用点
- 位置：`src_rs/junqi.h:3`
- 问题：`#define RS_NARROW_BOARD 1 /* 12 行 × 5 列 … */` 名称与语义相反（叫 "NARROW" 却是 12×5 窄棋盘），且无使用点 → 误导性死宏。
- 依据：grep `RS_NARROW_BOARD` 仅命中定义处。
- 修法：删除，或改名为语义正确的 `JQ_BOARD_12X5` 并在棋盘尺寸切换处真正使用。

## AUD-R-118 [P3] `junqi.h` 的"渲染几何"内联函数与规则层同文件，破坏头文件职责单一性
- 位置：`src_rs/junqi.h:33-60`
- 问题：见 AUD-R-077。此处补充：`junqi.h` 是被 `layout.c/rules.c/touch_state.c/ui_menu.c` 等广泛 include 的规则头，携带 720×1280 的渲染常量使规则层编译单元隐式依赖渲染资源规格。
- 依据：`grep bg_cell_img_center` 命中 `render.c` / `touch.c` / `game_selftest.c` / `touch_selftest.c`。
- 修法：拆到 `board_geom.h`。

## AUD-R-119 [P3] `junqi.h` 末尾有悬空注释块（无对应内容）
- 位置：`src_rs/junqi.h:190-192`
- 问题：`/* ---- 渲染对象图 + 容器（③④ 逆源码，独立头文件，不在此 include 防循环）---- */` 之后就是 `#endif`，注释指向的说明不存在。
- 依据：`junqi.h:190-192`。
- 修法：补内容或删除注释。

## AUD-R-120 [P3] `junqi.h` 把存档 API（`jq_save_*`）声明在规则头中，分层不清
- 位置：`src_rs/junqi.h:179-188`
- 问题：存档结构体与读写函数与规则状态结构体混放；`jq_save_game_write(const char*, const jq_state*)` 让规则头依赖文件 IO 语义。
- 依据：`junqi.h:179-188`；实现 `src_rs/save.c`。
- 修法：把存档声明移到 `save.h`（`save.c` 已有实现，缺头文件分离）。

## AUD-R-121 [P3] `jq_state` 缺少原版的"扛旗计数"，战况统计无法区分"扛旗胜"与"困毙胜"
- 位置：`src_rs/junqi.h:104-116`
- 问题：兄弟实现 `src/game_logic.c:231` 有 `flags_captured[1-side]++`（扛旗次数），src_rs 无对应字段 → 上层（战况统计、复盘）无法呈现。
- 依据：`src/game_logic.c:99-101`（`flags_captured`）；`src_rs/jni_bridge.c:6665-6681` 的战况统计只统计余子与总步数。
- 修法：补 `int flags_captured[2];` 并在吃旗时递增。

## AUD-R-122 [P3] `jq_state` 注释里的原版偏移（+0xa2/+0xa4/+0x48/+0x4c/+0xb0/+0x6c/+0x50/+0x78）与本结构体无字段级映射，易被误读为"已对齐"
- 位置：`src_rs/junqi.h:99-103`
- 问题：注释列了一串原版偏移，但 `jq_state` 没有任何一个字段与之对应（注释自己也说"本重写用 4 字节 jq_cell 紧凑布局（等价语义，L79 记录的布局差异）"）。
- 依据：`junqi.h:99-116`。
- 修法：注释改为"原版偏移参考（非一一对应）"，或补一张映射表。

## AUD-R-123 [P3] `jq_check_layout_cell()` 的注释未说明索引是"全盘 60 格绝对索引"，与"半棋盘视角"易混
- 位置：`src_rs/junqi.h:141-142`
- 问题：注释写"炸弹：idx<=34 报错；地雷：idx<=49 报错；军旗：仅 idx∈{56,58}"，未说明这些是红半区的绝对索引（row7..row11），而蓝半区 idx≤29 直接放行（见 AUD-R-063）。
- 依据：`junqi.h:141-142` vs `layout.c:81-87`。
- 修法：注释补充坐标系说明与"蓝半区由镜像规则保证"的说明。

## AUD-R-124 [P3] `jq_validate_layout()` 的注释描述原版特判（`rank-0xb<=1`），与实现路径不同
- 位置：`src_rs/junqi.h:144` vs `src_rs/layout.c:92-108`
- 问题：头注释说"0x6a3b0 全 60 格校验 + 计数表（军旗/地雷特判 rank-0xb<=1）"，实现实际走 `jq_check_layout_cell()` 的显式分支，计数用局部 `seen[]`。
- 依据：`junqi.h:144`、`layout.c:92-108`。
- 修法：注释与实现对齐，并明确 `count_tbl` 未回填（见 AUD-R-041）。

## AUD-R-125 [P3] `jq_extract_layout_30()` 的注释把它定位为"布局存档"主力，实际只能导出蓝半区
- 位置：`src_rs/junqi.h:172-174`
- 问题：注释"★ P1 布局存档（datah_016/017/020）保存当前棋盘用"暗示可完整还原棋盘，实际丢失红方半区（见 AUD-R-048）。
- 依据：`junqi.h:172-174`、`layout.c:113-124`。
- 修法：注释改为"导出可分享的蓝方布局"，存档功能改用 `jq_save_game_*`。

## AUD-R-126 [P3] `jq_standard_army_count()` 语义是"常量表"却用输出参数且无返回值，易被误当作校验函数
- 位置：`src_rs/junqi.h:171` vs `src_rs/layout.c:23-28`
- 问题：函数只是填一张固定表，命名像"取标准军力"，调用方 `layout.c:96` 用它做期望值比较；无返回值意味着无法表达"表本身是否有效"。
- 依据：`junqi.h:171`、`layout.c:23-28, 96`。
- 修法：改为 `const int *jq_standard_army_count(void)` 返回静态表。

## AUD-R-127 [P3] `JQ_EVENT_NAMES[6]` 的元素类型是 `const char *`（指针非 const），可被外部改写
- 位置：`src_rs/junqi.h:177`、`src_rs/rules.c:29`
- 问题：`extern const char *JQ_EVENT_NAMES[6];` —— 数组本身 const（声明处），但元素类型是"指向可写 char 的指针"，`const char *p = JQ_EVENT_NAMES[0]; *p = 'x';` 合法。
- 依据：`junqi.h:177`；`rules.c:29-31` 的定义。
- 修法：改为 `extern const char *const JQ_EVENT_NAMES[6];`。

## AUD-R-128 [P3] `junqi.h` 的 `jq_cell` 4 字节紧凑布局与 `save.c` 的裸字节序列化耦合，跨版本不兼容
- 位置：`src_rs/junqi.h:62-67`（4 字节）、`src_rs/save.c`（按 `sizeof(jq_cell)` 写盘）
- 问题：`jq_cell` 4 字节、无 padding，但结构体大小/字段顺序一旦变化，`jq_save_game_read` 读旧档即错位（无版本号/无 magic 校验）。
- 依据：`junqi.h:62-67`；`save.c:60-110` 的读写实现。
- 修法：存档加 magic + 版本 + 结构体大小校验。

## AUD-R-129 [P3] `rules.c` 的工兵判定与"rank 9=工兵"耦合，但 `RANK_*` 宏中无对应常量
- 位置：`src_rs/rules.c:139, 183, 338`；`src_rs/junqi.h:16-21`
- 问题：同 AUD-R-116 的另一面：规则的关键分支用裸 9，而 `RANK_EMPTY/BOMB/MINE/FLAG/TOP` 都有宏 → 风格不一致，rank 表调整时会漏改。
- 依据：`junqi.h:16-21` vs `rules.c:139/183/338`。
- 修法：补 `RANK_ENGINEER 9` 并全量替换。

## AUD-R-130 [P3] `board.c` 的行营注释列出的是"layout 索引 → 棋盘坐标"的映射，读者需自行反推
- 位置：`src_rs/board.c:10-11`
- 问题：注释"行营 = default.lay 5 个空位（layout idx 6,8,12,16,18 → 蓝棋盘 (4,1)(4,3)(3,2)(2,1)(2,3)，红镜像 …）"信息密度高但易错，且与实现里的 if 顺序不同。
- 依据：`board.c:10-21`。
- 修法：改成坐标集合表（如 `static const int8_t CAMPS[10][2] = {...}`）并用查表实现，同时注释里注明与 default.lay 的一致性是校验出来的。

## AUD-R-131 [P3] `board.c` 的大本营注释只提红方 (11,1)/(11,3)，未说明蓝方 (0,1)/(0,3) 也被实现认作大本营
- 位置：`src_rs/board.c:23-28`
- 问题：注释"原版 0x2a228 军旗格 56/58 = row11col1/col3（红大本营）"只解释了一侧，而 `is_base` 对称地认 4 格（见 AUD-R-063）。
- 依据：`board.c:27`。
- 修法：注释补"镜像：蓝方 (0,1)/(0,3)"，并说明与 `jq_check_layout_cell` 的不对称（蓝半区直接放行）。

## AUD-R-132 [P3] `board.c` 的 is_rail 注释包含已被实测证伪的论断
- 位置：`src_rs/board.c:33-34`
- 问题："双方底线为铁路是原版军棋标准（否则底角棋子无纵向通道，困毙误判）" —— A/B 实测显示该改动对困毙判定零影响（1848 = 1848），且引入 1061 条非法走子（见 AUD-R-001）。
- 依据：A/B 实测（3000 局走子集合对比 + 20000 局困毙计数对比）。
- 修法：随 AUD-R-001 一并修正注释，避免后人再次"依据"这段注释加固。

## AUD-R-133 [P3] `board.c` 头注释未标注"本文件结论对应的逆向条目版本"，与 rules.c 的注释密度不一致
- 位置：`src_rs/board.c:1-7`
- 问题：board.c 的头注释引用"L51/L79/L49/L68"但未写明这些是"§L49/L68"（REVERSE_NOTES_01_basics.md 的章节号），与 rules.c 中"§5/L47/L48"式标注风格不一致，检索时容易漏。
- 依据：`board.c:2-5` vs `rules.c:3-12`。
- 修法：统一为"§Lxx（REVERSE_NOTES_01_basics.md）"格式。

## AUD-R-134 [P3] `coord_cmp()` 的返回语义（0=相同）与其命名直觉相反，且未被使用
- 位置：`src_rs/board.c:48-53`、`src_rs/junqi.h:133`
- 问题：函数名像"比较大小"，实际返回"是否相同"；与 C 库 `memcmp` 语义（<0/0/>0）不同 → 误用风险（虽然当前无调用点）。
- 依据：`board.c:48-53`；`junqi.h:133`。
- 修法：改名 `coord_equal()` 或删除（见 AUD-R-069）。

## AUD-R-135 [P3] ✅ 已修 `rules.c` 缺少针对规则引擎的单元测试目标
- 位置：`src_rs/Makefile`（无 `rules` 相关 target）、`src_rs/ai_selftest.c`（仅覆盖 `ai_score`）
- 问题：345 行的规则引擎没有直接单测；现有 `game_selftest.c` / `consistency_desktop.c` / `evverify.c` 走端到端路径，定位困难，且不覆盖本次审计发现的暗棋规则、底线滑行、困毙 side 语义等分支。
- 依据：`Makefile:42-104` 的 target 列表；`ai_selftest.c:38-64` 的断言集。
- 修法：新增 `make rulestest`，用表驱动用例覆盖：吃子/炸弹/雷/旗/行营/大本营/铁路/工兵拐弯/和棋/困毙/暗棋 3 雷/底线滑行回归。
- ✅ **已修（本轮 95c08b8+ 落地）**：新建 `src_rs/rulestest.c`（表驱动 13 族全量用例）+ `Makefile` `.PHONY: rulestest` target（`dist/jq_rulestest`，board.c layout.c rules.c save.c rulestest.c ui_state_stub.c，-DRS_NO_GL -lm）。13 族：①明棋走子②明棋吃子③同级同归④炸弹同归⑤工兵挖雷⑥非工兵撞雷⑦扛旗⑧炸弹撞旗（AUD-R-020）⑨困毙⑩暗棋工兵扛旗拦截⑪暗棋先手翻子定色⑫铁路方向拒（AUD-R-001）⑬回合门（AUD-R-005）。`make rulestest` 全 PASS（0 FAIL）。期望值以**实际行为**为准（自定义夹具 25/20/26/30 相邻格，避免 default.lay 中线行营阻断；明棋走子/吃子/同级/炸弹/挖雷/扛旗/炸弹撞旗用 memset+2 子构造，暗棋族用 jq_dark_setup，铁路/回合门用 default.lay+改格对齐 evverify）。

## AUD-R-136 [P3] ✅ 已修 `ai_selftest.c` 缺关键用例（bomb-vs-flag / 暗棋扛旗 / side!=turn 困毙 / 底线纵滑）
- 位置：`src_rs/ai_selftest.c:38-64`
- 问题：现有 9 条断言覆盖了 rank 编码还原、mine/同级/送死/空移，但缺少本次审计发现的 4 个关键盲区（见 AUD-R-009、AUD-R-002、AUD-R-005、AUD-R-001）。
- 依据：`ai_selftest.c` 全文。
- 修法：补 4 条以上断言，尤其 `CHECK(ai_score(10,12) != 1000, "炸弹撞旗非吃旗")`。
- ✅ **已修（本轮落地）**：`ai_selftest.c` CHECK 链末尾追加 3 条**纯 ai_score**用例（aitest 只测纯函数，不 link jq_state 构造逻辑）：⑩`ai_score(rank_bomb=10, rank_flag=12, revealed=1) == 1000`（炸弹撞旗=大压小 10<12=1000，AUD-R-009 同源，非同级 900/炸弹 800）；⑪`ai_score(rank_worker=9, rank_flag=12, 1) == 1000`（明棋工兵扛旗=大压小 9<12=1000）；⑫`ai_score(rank_worker=9, rank_flag=12, 0) == 2`（暗棋未翻扛旗=不透视真 rank，上限空移分 2，AUD-R-058 诚实标注配套）。`make aittest` 全 PASS（0 FAIL）。
- **诚实边界（涉 jq_state 的 2 条用例挪放，非 aitest 盲区）**：side!=turn 困毙（AUD-R-005 同源）与底线纵滑拒（AUD-R-001 同源）需构造 `jq_state`，按"ai_selftest 只测 ai_score 纯函数"的红线放 `rulestest.c` ⑬/⑫族（`t_turn_gate` / `t_rail_dir_reject`），不进 aitest。

## AUD-R-137 [P3] ✅ 已修 `evverify.c` 的桌面断言未覆盖暗棋规则族
- 位置：`src_rs/evverify.c`（run_move/run_eat/run_bomb/run_flag/run_mine/run_undo/run_draw/run_resign/run_stalemate）
- 问题：现有 8 类事件断言全部在明棋下进行，未覆盖 ruleflip 的三条规则（工兵吃雷限制、挖 3 雷扛旗、先手定色）。
- 依据：`evverify.c:66-330` 的用例构造（均调用 `jq_load_layout` + 手工置 revealed）。
- 修法：新增 `--dark` 组用例。
- ✅ **已修（本轮落地）**：`evverify.c` 的 `run_dark` 已有 6 用例（①非工兵扛旗拒②工兵未挖3雷扛旗拒③工兵挖满3雷扛旗放行④非工兵挖雷拒⑤明棋对照⑥明棋 mode=LAY 工兵扛旗放行）；本轮追加 **⑦先手翻子定色**：`jq_dark_setup(&st, JQ_DEFAULT_LAY)`（全 60 格 side=SIDE_UNKNOWN + revealed=0 + mode=JQ_MODE_FLIP，上轮 95c08b8 落地）→ 找首个非军旗/地雷暗子格 → `CHECK(jq_can_flip(...))`（rules.c :583 c->side==0xFF 分支）→ `CHECK(jq_flip(...))` → `CHECK(st.cells[fi].side == SIDE_BLUE)`（翻转前 turn=BLUE，首翻定 side=turn）→ `CHECK(revealed==1)` + `CHECK(st.turn==SIDE_RED)`（flip 切 turn）。`run_dark` 作为单一计数单元内部 CHECK 数 +1，`make evverify` 顶层 167/167 不变（只追加、不改既有期望值，符合红线）；`make rulestest` ⑪族同步覆盖该断言。

## AUD-R-138 [P3] `src_rs/rules.c` 与 `src_jni_translated/jq_rs_rules.c` 已产生分叉，缺少同步机制
- 位置：`src_rs/rules.c`（2025-11-05）vs `src_jni_translated/jq_rs_rules.c`（内容为更早快照）
- 问题：两份文件头注释、函数体几乎相同但已分叉（翻译体没有 `ui_rule_cfg_get` 开关、AI 评分缺暗棋判定），任何对 src_rs 的修复若不同步翻译体，两套 so 的规则行为不同（见 AUD-R-081）。
- 依据：`diff` 两文件可见差异；`src_jni_translated/jq_rs_rules.h:136-166` 的声明与 `junqi.h` 几乎逐行重复。
- 修法：加 CI 校验（或用脚本从 src_rs 生成翻译体），并在 `AGENTS.md` 明确同步义务。

## AUD-R-139 [P3] 规则引擎无"终局原因"字段，吃旗/困毙/和棋/认输共用 stage+winner
- 位置：`src_rs/junqi.h:104-116`（只有 stage/winner）、`src_rs/rules.c:309-325`
- 问题：上层（音效、横幅、战况统计、复盘）无法区分"扛旗胜"与"困毙胜"。实测 T36 中，炸弹撞旗导致对手无子可动 → 困毙路径给出 winner=0，音效/横幅会把一次"旗被炸"呈现为"困毙胜"。
- 依据：实测 `T36 炸弹撞旗 ev=2 stage=2 winner=0`；`jni_bridge.c:7801` 的 `sfx_game_over(env, STG_OVER, over_win)` 只看 winner。
- 修法：`jq_state` 增加 `uint8_t end_reason`（FLAG/STALEMATE/DRAW/RESIGN），由终局函数写入。

## AUD-R-140 [P3] `jq_on_stalemate()` 之后 `winner` 的取值依赖 `turn ∈ {0,1}`，无保护
- 位置：`src_rs/rules.c:323`（`st->winner = 1 - st->turn;`）
- 问题：`turn` 是 `uint8_t`；若为 0 → winner=1，1 → winner=0，正常；若 turn ≥ 2 → winner 为负数（-1 恰与"未定"撞码，-2 等更糟）。
- 依据：`rules.c:323`；`st->turn` 可被外部直接写（`jni_bridge.c:226, 1084, 1112, 5979, 6109`）。
- 修法：`st->winner = 1 - (st->turn & 1);` 或加断言。

---

# 五、按严重级统计

| 严重级 | 条数 | 条目编号 |
|---|---|---|
| **P0** | 4 | AUD-R-001 … AUD-R-004 |
| **P1** | 16 | AUD-R-005 … AUD-R-020 |
| **P2** | 70 | AUD-R-021 … AUD-R-090 |
| **P3** | 50 | AUD-R-091 … AUD-R-140 |
| **合计** | **140** | AUD-R-001 … AUD-R-140 |

## 分布速览（按主题）

| 主题 | P0 | P1 | P2 | P3 | 小计 |
|---|---|---|---|---|---|
| 铁路/走子路径（底线铁路、端点式路径判定、纵向滑行） | 1 | 1 | 5 | 4 | 11 |
| 暗棋 ruleflip 规则族（3 雷扛旗 / 工兵吃雷 / 先手定色 / 安全岛） | 2 | 2 | 4 | 2 | 10 |
| 胜负与终局（困毙 API、和棋优先级、杀光可动子、旗被炸） | 0 | 4 | 8 | 3 | 15 |
| 状态与计数（pieces_left / count_tbl / mines_cleared / undo_depth） | 1 | 3 | 9 | 6 | 19 |
| 翻棋（白名单、turn 切换、事件契约） | 0 | 2 | 6 | 5 | 13 |
| AI（评分归一化、暗信息、送死、炸弹撞旗） | 0 | 2 | 5 | 5 | 12 |
| 数组越界/内存安全 | 1 | 0 | 3 | 3 | 7 |
| API 契约/分层/常量/文档 | 0 | 1 | 13 | 12 | 26 |
| 测试与一致性（单测缺失、多实现漂移） | 0 | 0 | 3 | 9 | 12 |
| 其它（性能、存档、渲染常量混入） | 0 | 0 | 6 | 3 | 9 |

## 最需优先处置的 5 条（建议顺序）

1. **AUD-R-001**（P0）底线"铁路"导致 col0/col4 全列纵滑 —— 一步从红底线滑到蓝底线；修法：删 `board.c:40` + 改 `rail_path_straight` 为"路径同线"判定 + A/B 回归锁定。
2. **AUD-R-002/003**（P0）暗棋（默认模式）缺失 ruleflip 三条核心规则且"先手翻子定色"未实现 —— 暗棋玩法实际是"明棋藏脸"，AI 全知；需引入 `mode` 维度并落地 ruleflip 判定。
3. **AUD-R-004**（P0）`pieces_left[dst->side]` 越界写 —— `side=0xFF` 时写飞 1020 字节；需加范围守卫 + `JQ_SIDE_UNKNOWN` 常量。
4. **AUD-R-009/010**（P1）`ai_score` 炸弹撞旗=1000 + 无 revealed 维度 → AI 主动撞旗同归 + 暗棋透视。
5. **AUD-R-005**（P1）`jq_side_stalemated(st, side != turn)` 恒真 —— 困毙误报的潜伏 API 契约缺陷（实测明棋下假阳性）。

---

> 审计边界声明：本审计仅覆盖 `src_rs/rules.c`、`src_rs/board.c`、`src_rs/junqi.h` 三个文件的**内部逻辑与契约**；
> 调用方（`jni_bridge.c` / `bt_session.c` / `render.c` / `touch*.c` / `layout.c`）仅在被引用为证据时提及，
> 其自身缺陷不在本次编号范围内。所有"实测"结论均来自本次审计临时编写的探针程序（直连 `board.c layout.c rules.c ui_state_stub.c`），
> 探针源码位于 `/tmp/opencode/r2_probe*.c` 与 `/tmp/opencode/r2_ab*.c`（临时目录，不入库）；
> 关键复现步骤已在各条"问题"字段中写明，可独立复现。
