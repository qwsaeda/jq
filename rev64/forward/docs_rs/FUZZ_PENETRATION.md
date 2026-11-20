# 渗透轮判据台账（FUZZ_PENETRATION）

> 定位：AGENTS 渗透回路（黑盒→白盒→渗透→白盒→综合）的**渗透轮**判据 + 经验库。
> 入口：`python3 rev64/tools/fuzz_probe_check.py`（3 seed × 4 变异器，设备无关离线，零 token 成本）
> 基准 binary：`rev64/forward/src_rs/dist/jq_fuzzprobe`（`make fuzzprobe`，ASan 默认开防假绿）
> ★ 2026-11-20 用户指令固化：渗透轮**不含蓝牙**（帧变异器移除，fuzz_probe F1-F4 4 变异器）。

## 0. 与既有审计体系的关系（防重复建设）

| 层 | 入口 | 性质 | 本台账补什么 |
|---|---|---|---|
| 黑盒 | `REAL_DEVICE_ACCEPTANCE.md` 5 判据 + `mumu_regress.sh` | 真机行为验收 | 渗透前前置 |
| 白盒 | `arm32_verify_all.sh` 静态 73 项 + `align_gate.sh` 7 项 | 静态判据 | 渗透命中崩溃后定位 |
| **渗透** | **`fuzz_probe_check.py`（本台账）** | **随机变异 0 崩溃** | **随机路径崩溃/越界内存/状态机异常** |
| 综合 | 回路：黑→白→渗透→白盒修→黑复验 | 闭环 | 每轮渗透 FAIL 必须白盒定位根因再修，禁止只加 guard 掩盖 |

**教训（防假绿，承接 CODE_STYLE §9 三型）**：
1. **判据恒真型**：fuzz_probe F1 曾出现"总走子=0 仍 PASS"——随机抽样漏候选导致全困毙收束（合法终局掩盖死循环）。修：64 次随机抽样 + 全量兜底枚举双段（漏候选不误报困毙），`stats[2]` 单列无候选计数 + guard 截断率 ≤10% 双判据。
2. **可复现性**：xorshift32 首版实现 bug（`g_seed=x` 在变换前保存 → 恒返回同值）→ 3 seed 输出完全一致被误当"一致性 PASS"。修：`g_seed` 在变换后保存 + 同 seed 重跑 F1 行逐字比对（② 判据）。**教训：随机数生成器自身必须先单测（/tmp/rngtest 模式），再入 fuzz 主链。**
3. **rank 存储域真值**：红方已翻开 rank 编码 `+0xc`（`RANK_TOP+0xc=24` 为存储域上限），首版断言 `r<=15` 误报。修：`junqi.h` 真值域 `[0,24]`，`>24` 才是引擎写坏。

## 1. 5 变异器判据（无蓝牙）

| 变异器 | 覆盖 | 判据 | 规模（默认） |
|---|---|---|---|
| **F1 随机对弈** | 规则引擎 + 终局分发（`jq_can_move`/`jq_try_move`/`jq_flip`/`jq_on_stalemate`） | 0 崩溃（ASan 干净）+ `rank/side/stage` 全合法 + guard 截断率 ≤10% + 明棋 90%/暗棋 10% 混合 | 500 局 ×3 轮（3 seed） |
| **F2 触摸坐标** | 触摸状态机 `jq_touch_state_process`（越界/NaN/边界 7 类坐标） | 0 崩溃 + `ts.mode ∈ [0,2]` + `sel ∈ [-1,60)` 状态机合法态 | 20000 次 |
| **F3 事件注入** | 事件名表 73 条 × 随机 `%d` 负载（`jq_event_names_73` 真值表） | 0 崩溃 + 负载 256B 无溢出 + 前缀匹配命中统计 | 5000 条 |
| **F4 悔棋栈** | `UNDO_MAX` 边界连续灌/抽/满拒/空拒（契约：栈满拒 `push=0`/空栈拒 `pop=0`） | 0 崩溃 + 满拒/空拒契约成立 + `top ∈ [0,UNDO_MAX]` | 100000 次 |
| **F5 存档 round-trip** | 持久化路径 `jq_save_game_write/read`（契约1 返 1 成功 / 契约2 枚举域拒读 / 契约3 越域格归零） | 0 崩溃 + 合法域 round-trip 零漂移 + 脏枚举（winner=3/-5）拒读 + 越域格归零可预期 | 200 局 + 越域 20 格 |

**判据 = `fuzz_probe_check.py` 5 项全 PASS + ② 可复现性（同 seed 重跑 F1 行逐字一致）+ ASan 无 `ERROR: AddressSanitizer`（rc=0 也判 FAIL，假绿防护）**

## 2. 用法（渗透轮 SOP）

```bash
# 渗透轮入口（默认 3 seed × 4 变异器 + 可复现性）
python3 rev64/tools/fuzz_probe_check.py

# 快跑（单 seed，改 fuzz_probe.c 后快速回归）
python3 rev64/tools/fuzz_probe_check.py --seed 7 --games 200 --touch 2000 --events 500

# binary 缺失降级（CI 离线期）
python3 rev64/tools/fuzz_probe_check.py --offline    # rc=3 PENDING 非 FAIL

# 自定义规模（深 fuzz：局数/触摸次数拉大）
python3 rev64/tools/fuzz_probe_check.py --games 2000 --touch 100000 --events 20000
```

**FAIL 处置（承接"渗透命中→白盒定位"回路）**：
1. `fuzz_probe_check.py` 输出 `证据:` 行（FAIL/AddressSanitizer/ERROR 原文摘录）→ 记台账本文件 §4
2. 白盒定位：ASan report 的 stack trace → 对应 `src_rs/*.c:行`（`rules.c`/`touch_state.c`/`layout.c` 为主）
3. 修 → 复跑 `fuzz_probe_check.py`（同 seed 必复现原 FAIL 路径，防"随机侥幸修好"）
4. 全 PASS 后才允许写"渗透轮达成"

## 3. 诚实边界

- **fuzz 只查崩溃/非法态，不查行为正确性**——不崩 ≠ 行为对齐原版（行为对照靠 `rule_probe_compare.py` 双端 36 case + `align_gate.sh` 事件/布局判据）
- F2 坐标基准 720×1280（渲染几何），越界值 ±1e6 级；真实设备 view 尺寸（900×1600）由 `mumu_regress.sh` 黑盒轮覆盖，fuzz 不重复
- F3 事件名 73 条 = `jq_event_names_73` 真值表 + 随机 `%d` 替换，未覆盖非法字符/超长负载（负载 256B 上限内）
- **F5 存档契约 = save.c 实现真值**：契约1 `write/read 返 1 成功 0 失败`；契约2 枚举域（turn∈{0,1}/stage∈[0,2]/winner∈[-1,2]/mode∈{0,1}，越域拒读返 0 + st 不污染）；契约3 cells 越域格归零（rank>24/side 非法/revealed>1 → 强制 rank=0+side=0xFF+revealed=0+flags=0，**设计行为非缺陷**）。随机化仅限合法域（② 零漂移）；脏枚举单列 ③ 验拒读，不混入合法域零漂移判据（避免契约3 归零漂移误判 FAIL）。
- **不含蓝牙**（用户 2026-11-20 指令）；蓝牙帧变异若后续要加，单列 `bt_fuzz` 不入本 5 变异器

## 4. FAIL 台账（命中→定位→修→复跑闭环记录，最新在上）

> 格式：`日期 | seed | 变异器 | ASan/FAIL 原文摘录 | 白盒定位(文件:行) | 修复 commit | 复跑结果`
> （当前 0 FAIL——首跑 3 seed 全 PASS，本表待命中填充）

| 日期 | seed | 变异器 | 证据摘录 | 定位 | 修复 | 复跑 |
|---|---|---|---|---|---|---|
| 2026-11-20 | — | F5（首版契约误用） | "随机局 write 失败"误报（实为契约1 返 1 成功，首版判 `==0 成功`） | save.c:113 `return 1`（write 契约）+ fuzz_probe.c 首版 F5 `CHECK(jq_save_game_write(...) == 0)` 判据反 | 契约1 改 `==1 成功` + 随机合法域 200 局零漂移 + 脏枚举（winner=3/-5）单列拒读 + 越域格归零契约3 + 毒化 back 判据 | 3 seed 全 PASS（84ac5ff 全判据回归 + fuzz_probe_check 复跑） |
| 2026-11-20 | — | F1/F2/F3 历史 | 首跑 3 seed 全 PASS（0 真崩溃 FAIL） | — | — | — |
| 2026-11-20 | — | **32 位 M2/M3（flaky 判据误设）** | jqt_mem_fuzz M2"影子区+1MB 截断后 valid 必 1" + M3"区内命中率低"间歇 FAIL（10 连跑 1 次） | jqt_mem.c 桌面 32/64 位语义差：shadow 高位截断 <MAP_HI 撞真实表 w区 → mincore 路径（页未驻留合法 0，flaky 非缺陷） | M2 删"截断必 1"判据（改报告性）+ M3 区内判据改**纯偏移区**（a<MAP_HI 必 1，不涉 mincore） | 3 seed × 6 连跑 0 FAIL（jqt_mem_fuzz_check.sh PASS） |

## 5. 经验库（渗透轮专用，防复发）

- **随机数生成器单测前置**：xorshift32 实现 bug 曾被 3 seed 输出一致性掩盖（§0.2）。任何 fuzz 主链改 rng 必须 `/tmp` 独立单测 5 值序列再入主链
- **抽样漏候选 ≠ 死循环**：F1 随机抽样漏 → 误报困毙（合法终局）掩盖真死循环。判据必须双段（随机抽样 + 全量兜底）+ `guard 截断` 与 `无候选` 分开计数（§0.1）
- **rank/side 存储域用 `junqi.h` 真值**：红方已翻 `rank+0xc`（域 `[13,24]`），side `0xFF`=UNKNOWN 合法（§0.3）
- **存档契约1 假绿（首版踩坑）**：`jq_save_game_write/read` 返回 **1=成功 0=失败**（save.c 末 return 1），首版误用 `==0 成功` → "write 失败"误报 + round-trip 恒 0 假绿。教训：**fuzz 判据必须先用"合法用例必 PASS"正向校准契约（write 返 1），再上随机化**——随机合法域 200 局全零漂移 + 脏枚举单列拒读判据（§1 F5 + §3 诚实边界 F5 契约段）
- **脏存档拒读 + 毒化 back 判据**：`memset(&back, 0xFF)` 毒化后再 `jq_save_game_read`，拒读契约（返 0）时 back 必保留 0xFF（st 不污染原子语义）；若 back 被半读污染则 `(uint8_t*)&back[0] != 0xFF` → FAIL（R41-Q16 短读污染 st 根治判据同源）
- **ASan 假绿防护**：`fuzz_probe_check.py` 即便 rc=0 也扫 `ERROR: AddressSanitizer`（ASan 默认 abort 非 0，但链接错误/静默降级场景 rc=0 也须扫 report 文本）
- **可复现判据**：同 seed 重跑 F1 行逐字比对（非"都 PASS 就行"）——随机路径可复现 = 修了才能证复跑不崩
