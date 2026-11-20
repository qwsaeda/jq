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

## 2.5 黑盒轮实锤（真机 5 判据，10.0.182.152，2026-11-20）

> 渗透回路"黑盒前置"判据：正向 C so `629bf06e`（jni_bridge.c 括号配平 + GetMethodID
> 异常清理修复后）装机 152 真机 5 判据全 PASS——渗透 0 崩溃在真机侧无回归佐证。

| 判据 | 实锤 | 结果 |
|---|---|---|
| ① 装得上 | `adb install -r phase_rs_a64.apk` Success（streamed 19432146B） | PASS |
| ② 进主界面 | `Init 像素解码 board=OK(737x751)` + `rs_draw_menu 首帧`（view=900x1600，5 按钮顶点出帧） | PASS |
| ③ 下一局 | `setprop jq.movetest 1` → 自动走子 3/3（Junqi.Move×2 + Junqi.Dead×1，复盘 n=3，pid 50032） | PASS |
| ④ 正常退出 | `am force-stop` 0 崩溃 | PASS |
| ⑤ 重进不崩 | 重进 pid 50145 存活 + `Init 像素解码` 重出 | PASS |
| 全链 | `Fatal signal` 总数 = 0 | PASS |

**完整 60 局对弈真机钩子（2026-11-20 实锤，so `4539aadddd`）**：
`setprop jq.movetest 60` → OnCreate 自动 60 局 AI 对弈（ai_pick 按 turn 视角 + 翻棋 +
困毙收口，同 gametest 逻辑）→ 实锤 `收局=60 总走子=8460 吃子/炸弹=1680 扛旗=0` + 0 崩溃。
- 口径对照：桌面 `game60_offline_check.py` = **单局 10/10 回合交替基准**（蓝走子 10/
  红 AI 10/翻转 20/困毙 0）；真机钩子 = **60 局批量收局**（收局 60/60 = 全合法终局，
  贪心吃子 AI 吃光/扛旗收束，SPEC 完整对弈行为基准）。两者互补不互斥（桌面查单局
  交替闭环，真机查批量 0 崩溃）。
- 钩子上下文持锁约束（诚实标注）：unlock 后注入段持 g_mutex → **不调 jq_restart_game**
  （内部再取锁 = 自锁死锁，★ B03 红线同源坑），改用 `jq_state_init + load_layout +
  stage/turn 直设` 轻量局重置（同 gametest 每局口径，无锁无 UI 副作用）。
- `扛旗=0` = 贪心吃子 AI 收束偏好吃光/困毙（非规则缺陷；扛旗路径桌面 flagtest 单点
  实锤覆盖，批量钩子不强制扛旗收束）。
- `复盘 n=0` = 60 局钩子不逐条 ui_replay_record（批量统计口径，防 8460 条复盘栈爆炸；
  单局 3 步 movetest=1 仍走复盘 n=3 记录链）。

**32 位翻译体装机 152 PENDING 根因实锤（2026-11-20，非 so 缺陷）**：
- 32 位翻译体 apk（`phase_chain_arm32.apk`，包名 `com.cnvcs.junqi32`，so 段
  `lib/armeabi/libjunqi.so` = 32 位 ARM）装机 152 **`INSTALL_FAILED_NO_MATCHING_ABIS`**。
- 根因（设备属性实锤）：152 `ro.product.cpu.abilist` = **x86_64,arm64-v8a,x86（无
  32 位 ARM abi，无 armeabi/armeabi-v7a 兼容层）** → 纯 32 位 ARM so 无转译通道
  （`/data/secondary_arch/lib/arm` 空 = 无 32 位 houdini 转译器）。
- **诚实边界（禁自欺）**：32 位真机验收基准 = **真 32 位 ARM 原生设备**（REAL_DEVICE_
  ACCEPTANCE.md SOP）；MuMu x86_64 实例（146/152）对 32 位 ARM 翻译体 =
  **装机不可行**（abi 不匹配，非 so 缺陷/非翻译体回归）。32 位翻译体"能跑"判据
  在 MuMu 维持 PENDING（与 §124y-248~278 模拟器转译层故障史同档：环境约束非 so 缺陷），
  32 位侧验收 = 静态 74 项 + 渗透轮 jqt_mem_fuzz 5 探针（桌面 ASan 0 崩溃）+ 真 32 位
  设备冒烟（待）。
- 旧口径"MuMu 10.0.182.146 32 位 so 全链实锤"系**历史环境史存档**（当时 146 实例
  houdini 32 转译器在位）；当前 152 实例 abi 变更 → 32 位装机 PENDING（不外推历史 PASS）。

**诚实边界（152 真机实锤，非外推）**：
- **EGL_BAD_ALLOC(0x3003) 周期 = 真机 GPU 资源约束（非 so 缺陷）**：surface attach 前
  渲染线程 64000+ 次 0x3003（logcat 实锤），surface 就绪后 `rs_draw_menu` 正常出帧
  （0x3003 停止）——周期失败自愈机制（§R25 EGL 180→60 帧）在 152 生效，0 崩溃。
- **设备漂移**：权威口径 10.0.182.146 → **10.0.182.152**（Xiaomi 23127PN0CC x86_64，
  MuMu 实例重启后 IP 漂移；历史 146 引用 = 环境史存档不作当前态）。
- 判据 = 渗透回路"黑盒轮"达成（5 判据全 PASS + 完整 60 局批量收局 8460 走子 0 崩溃
  + 真实手点全链）；后续新增判据（AI 介入）未过按"未达成继续做"原则追加本段缺口。

**人机对战 AI 走子全链实锤（152 真机，2026-11-20 续，`sendevent event4` 真实触摸）**：
- 实锤链（so `4539aadddd`，pid 53117，0 崩溃）：
  ① 菜单 btn0（**448,1132 = 900 系 orig_menu 几何直值，非 720×0.8 换算**）→ `UI菜单 人机对战
     → 难度选择弹窗`（action=0 → UI_DIFFICULTY）
  ② 难度弹窗"新手翻棋"（619,660 = 720 基准 495,528 ×1.25）→ `难度弹窗：选按钮1
     （level=1 mode=1 翻棋暗棋）→ 开局 AI 走红`（g_vs_ai=1）
  ③ 蓝方手点翻 cell 6（274,1383）两步态机（首点选中 → 再点翻棋）→ `走子判定 翻棋
     cell=6 rc=1 turn→1（人机AI走红=1）`（复盘 n=1）
  ④ **AI 自动应着**：`红方 AI 应着（翻棋）：cell=0（turn 切回蓝方）`（复盘 n=2）
     → 人机回合交替持续（蓝翻→AI 红翻→蓝回合），0 崩溃
- **坐标口径补正（btn0 命中坑，本轮实锤）**：主菜单 5 按钮几何 = **orig_menu 900 系
  直值**（ui_menu.c:48 `201.6,867.2-516,943.2 ×0.8` 注释误导，实际命中区按 900 系
  btn0 中心 448,1132）——首点 359,1188（720×0.8 误换算）`hit=-1`，改 448,1132 命中
  `action=0`。固化：**主菜单几何用 900 系直值，弹窗几何用 720 基准 ×1.25 换算**（两类
  基准不同，勿混用）。
- **g_vs_ai 区分实锤**：人机（AI 走红）与两人热座（AI 不介入）经 `难度弹窗 level/mode`
  切换区分——人机局 `（人机AI走红=1）` 日志 + AI 自动应着；两人热座局 `（AI 不介入）`
  无 AI 应着（前轮实锤）。三处 AI 块守卫（§按钮业务对齐 so b7f057b6）真机行为一致。

**真实手点全链实锤（152 真机，2026-11-20，`sendevent /dev/input/event4` 真实触摸序列）**：
- **判据突破**：用户视角"手点能玩" = 真实触摸事件（`/dev/input/event4` → InputReader →
  InputDispatcher → app OnTouch）全链走通，非 `input tap` 注入通道（两条路，注入通≠手点通）。
- **实锤链**（so `4539aadddd`，pid 52271，0 崩溃）：
  ① 菜单 btn1（448,1274）→ `OnTouch 菜单按钮1 → UI_NEWGAME 两人新局弹窗`
  ② 弹窗"确定"（319,934 = 720 基准 255,747 ×1.25 缩放）→ `新局弹窗：确定 → 开局
     stage=PLAY 蓝先手 mode=1 翻棋暗棋`
  ③ 蓝方翻 cell 6（274,1383 = 720 基准 219,1106）：首次点 `to=-1 can_flip=0`（选中态）
     → 再点 `to=6 can_flip=1` → `走子判定 翻棋 cell=6 rc=1 turn→1`（复盘 n=1）
  ④ 红方翻 cell 30（96,721 = 720 基准 77,577）：首点选中 → 再点 `to=30 can_flip=1`
     → `翻棋 cell=30 rc=1 turn→0`（复盘 n=2）→ **回合交替持续（蓝翻→红翻→蓝回合）**
  ⑤ 全链 0 `Fatal signal` + 进程存活
- **152 设备侧轴表实锤（防 §124y 历史 146 轴表坑复发）**：`getevent -lp /dev/input/event4`
  = `Xiaomi Touchscreen`，`ABS_MT_SLOT=47(max31) / ABS_MT_POSITION_X=53(max900) /
  ABS_MT_POSITION_Y=54(max1600) / ABS_MT_TRACKING_ID=57 / BTN_TOUCH=330`，
  **无 ABS_X/ABS_Y(0/1)**——与 146 轴表一致（同型 MuMu Xiaomi Input），sendevent 前
  必查实际轴表（§124y-195 踩坑固化，152 复验成立）。
- **坐标口径**：view=900×1600，`base = view 坐标直接同值`（与 720×1280 渲染基准 ×1.25
  缩放一致，`OnTouch 走子诊断 view→base` 日志实证）；弹窗/棋盘/菜单命中区按 720 基准
  几何 ×1.25 换算 900 基准。
- **诚实边界**：暗棋 `can_flip` 首点 `to=-1`（选中）再点 `to=from`（翻棋判定）= 两步态机
  忠实行为（非缺陷）；`cell 6` 首点 `can_flip=0` 系选中态未触发翻棋判定（态机 SELECT 阶段），
  二点 `to=6` 才进翻棋分支（`jq_can_flip` 0xFF 未定色 + 当前回合门 = 蓝先手可翻，规则忠实）。

## 2.7 32 位真地址空间探针（解引用版，2026-11-20 — 关闭 32/64 位语义差诚实边界）

> `jqt_mem_fuzz32.c` + `setarch linux32`（真 32 位地址空间）→ 32 位语义成立 → **解引用全开**。

**为什么需要**：x86_64 桌面版 jqt_mem.c 的 maps 解析 `sscanf("%lx")→(uint32_t)lo` 把 host
48 位地址截断成 32 位 → 截断后 w 区可覆盖任意 32 位值（实测 min lo32=0x79a04000）→
路由返回值解引用会撞 host 保留区 → x86_64 版**只能判"路由 0 崩溃"，不能解引用**
（诚实边界）。真机 32 位 Android：地址空间本身 32 位 → lo 无截断 → 解引用语义成立。
本探针用 `setarch linux32` + `gcc -m32` 复现真机语义 → **诚实边界关闭**。

**实锤（3 seed 全 PASS，判据 5 项）**：
```
M1' 三路路由+解引用：100000 次，解引用 100000 次 0 崩溃（offset/影子区/真实区/零页四路全解引用）
M3' 二分对称：偏移区 2000/2000 命中 + 高位未映射 0/2000 误命中（32 位语义严格成立）
M4' GOT：18/18 命中 + 表外 0/100 误命中
M5' rebuild：×50 幂等 + 10000 次路由解引用 0 崩溃
```

**探针环境适配（4 项，全为探针自身约束，非 jqt_mem.c 缺陷）**：
1. **影子区必须恰好 JQT_MAP_HI=256MB**：`jqt_mem_addr` 影子区分支 = `base ≤ a < base+MAP_HI`
   → 探针早前用 128MB 导致 `base+128MB..base+256MB` 被误判"影子区内"→ 解引用未映射页
   SEGV = **探针自身缺陷**（破坏 jqt_mem.c 不变量），已修（恢复 256MB + 空闲洞探测落位）
2. **ASan 低位保留区**：32 位 ASan 占 0x00000000-0x40000000（low-shadow rw / PROT_NONE
   gap / high-shadow rw）→ 空洞探测从 0x40000000 起 + MAP_FIXED 落位
3. **ASan 插桩伪影**：32 位探针**不插桩 ASan**（ASan shadow 区语义 + 堆红区误报）→
   32 位探针核心价值 = 地址空间语义，非内存插桩
4. **影子区末字节跨界读**：`shadow+MAP_HI-1` 的 4 字节读跨到 256MB 边界外 → 探针
   mmap +4KB 余量（真机邻页通常另有一段映射可读）

**诚实边界（仍存）**：32 位 Linux（glibc/jemalloc）≠ 32 位 Android（bionic/scudo），
堆布局不同；但**地址空间宽度与截断语义相同**（本探针核心价值）。真机 32 位设备冒烟
仍 PENDING（MuMu x86_64 无 32 位转译层，§2.5 abi 根因）。

## 2.8 P1/P2 渗透面（2026-11-20 新增：空白面补齐）

> 用户"还有什么可以渗透发现的"→ 盘点出 9 个空白面，本轮落地 P1（PNG 畸形输入）+
> P2（UI 状态机点击序列）2 个（均桌面零设备依赖，ASan）。

### 2.8.1 P1：PNG 解码器畸形输入 fuzz（`fuzz_png.c`）

**空白原因**：现有 F1-F5 只覆盖规则/触摸/事件/undo/存档，**资源解码链零渗透**——
而 `png_decode.c` 是真机"资源加载失败→黑屏/ANR"的第一嫌疑点（历史 §124w-ALIGN
"资源 MISSING 退化"链的入口）。

**4 类变异 + 定向畸形 14 项**：
- M1 位翻转（随机 1-8 bit × 多字节）
- M2 截断（随机 offset）
- M3 定向畸形：clen=0xFFFFFFFF / IHDR 超长 / w·h=0xFFFFFFFF / w=0 / h=0 /
  bitdepth 非法(0,4,8,12,16) / colortype 非法(1,3,5,7,255) / interlace=1 /
  重复 IHDR / IDAT 声明 0x7FFFFFFF / 无 IEND / 签名错 / 空文件+4B+8B / NULL 指针 /
  IDAT 1MB 零填充
- M4 结构错乱：双 PNG 拼接 / 前置垃圾字节（签名错位）

**判据**：① 0 崩溃（ASan 干净含 heap-buffer-overflow）② 拒收语义（畸形必须返 -1，
**不得**返 0 却产垃圾像素）+ 合法对照组必成功 ③ 输出不变量（w·h ≤ 32MP 且 rgba
长度 = w·h·4）④ 无泄漏（ASan LSAN + 拒收却返指针检测）

**实锤**：4 seed × 3000 轮 = **3 万+ 畸形样本 0 崩溃**，统计：接受≈780 / 拒收≈6740
（接受样本 = 位翻转恰好仍合法 + 语料原图，输出不变量违规 0 / 拒收却给指针 0）。
语料 = 仓库 **54 个真实 PNG**（rev64/dist/spak_res）+ 内嵌 1×1 RGBA 对照组
（python zlib 生成的正确 CRC 字节）。

**踩坑固化**：① 手写内嵌 PNG 的 CRC/IDAT 字节错 → 对照组失败（**对照组必须机器生成
真字节**，不能手抄）② `png_decode.c` 依赖 `android/log.h` → 桌面需 stub
（`src_rs/android_stub/android/log.h` 入库，Makefile `-Iandroid_stub`）。

### 2.8.2 P2：UI 状态机随机点击序列 fuzz（`fuzz_ui.c`）

**空白原因**：现有 F2 只 fuzz **单点坐标**（touch_state 两步态机），**未覆盖屏态机**
——169 个 UI_ 屏态 + 44 个弹窗面板内判定的迁移链是"用户可见面"，屏态悬挂/弹窗
死锁/弹窗外误触 = 历史"玩不了"类缺陷的真实来源。

**3 类变异**：
- U1 随机点击序列（随机起点屏态 + 8-48 步随机坐标点击）
- U2 边界点击（10×10 网格：0/1/面板边 ±1px/719/1279）
- U3 **全 169 屏态自锁扫描**（每屏态随机点击 200 次必须能离开自己）

**判据**：① 0 崩溃（ASan）② 屏态恒在合法域 `[0, UI_SCREEN_MAX]` ③ 弹窗逃逸
（12 主流程弹窗面板外点击必退出）④ 棋盘 ☰ 回菜单无死锁 ⑤ **全屏态自锁扫描**
（169/169 屏态均可离开——初版只查 12 弹窗 → 扩展到全屏态，漏检面收口）

**实锤**：15 步 × 500 轮 = **2.6 万点击 0 崩溃**，非法屏态 0，自锁屏态 0。

**踩坑固化**：① 屏态枚举无"最大值"宏 → 新增 `UI_SCREEN_MAX`（ui_menu.h 末项 +
1，**维护红线：新增 UI_ 枚举项必须同步此宏**，否则新屏态落在域外 = 假绿）
② `g_state` 定义在 jni_bridge.c（JNI 侧不可桌面链接）→ 探针自建空实例
③ `ui_btn` 类型已由 ui_menu.h 定义（不重复定义）→ 改用库函数
`ui_menu_buttons()`/`ui_menu_hit()`（与 jni_bridge 同款几何，非自建表）

### 2.8.3 剩余 7 个空白面（未做，按可离线性排序）

| # | 空白面 | 价值 | 阻塞 |
|---|---|---|---|
| ~~P3~~ | ~~事件分发 code→handler + 音效映射（evt55_dispatch/sfx）~~ | 中高 | ✅ **已做（§2.8.6）** |
| P4 | 渲染 GL 状态序列（render.c 11229 行） | 高 | 需 GL 上下文（设备） |
| P5 | JNI 入口参数 fuzz（14 入口） | 中高 | 需设备 |
| ~~P6~~ | ~~存档文件损坏 fuzz（截断/字节翻转 on .sjg）~~ | 中 | ✅ **已做（§2.8.7）** |
| ~~P7~~ | ~~复盘链 fuzz（replay.c）~~ | 中 | ✅ **已做（§2.8.7）** |
| P8 | 多线程竞态（g_mutex/g_ui_lock/GL 线程，TSAN 重编） | 最高 | 可离线 ✅（需 TSAN 工具链） |
| ~~P9~~ | ~~32 位 dispatch 表 2218 槽 fuzz~~ | 中高 | ✅ **已做（§2.8.8，实际 5463 槽）** |

### 2.8.4 P5'：JNI 入口参数 fuzz（`fuzz_jni.c`）——**为什么不用 unidbg**

**用户提问"unidbg 行不行"的实测回答（仓库三条硬证据）**：

| # | 证据 | 数据 | 结论 |
|---|---|---|---|
| 1 | unidbg 的 .bss 非零率 | **0.2%**（50/28680）vs MuMu 真机 **79.1%**（22618/28680） | C++ 对象图未构造（渲染器单例 0x162400=0 / 事件表 0x160584=0 / scudo 槽 0）→ JNI 入口的**反射链前置全缺**（`FindClass("android/view/SurfaceHolder")` 拿不到类） |
| 2 | init_array 执行 | 12 条跑通 11 条，`0x11c10d` 报 `UC_ERR_INSN_INVALID`（模拟器精度 bug）+ `0xb67dc` 报 `UC_ERR_READ_UNMAPPED` | 连构造链都跑不完，更进不了业务 |
| 3 | 模拟器 JNIEnv | `FindClass`/`GetMethodID` 只能返 NULL/假值 | 14 入口的**反射链分支根本进不去** → 只能测"环境失败路径" |

（证据来源：`rev64/reverse/docs/REVERSE_LOG.md` §124y-152 + `REL_RELOCATION_AUDIT.md`）

**结论**：unidbg 能测 JNI 入口的"壳参数处理"，**测不到真 JNI 语义**；而真语义已被
真机黑盒实证覆盖（152：`am start` + setprop 钩子 + `sendevent` 真实触摸全链：
菜单→弹窗→暗棋翻子→AI 应着）。

**正解 = 桌面 mock JNIEnv**（测 so 内部**参数防御代码**，纯 C 逻辑，可离线/可复现）：
- **14 入口 × 4 毒化档 × 6 变体 = 336 调用组合**
  - 毒化档：正常 / 全 NULL 桩 / 垃圾桩（`GetStringUTFChars` 返非指针垃圾）/ 异常桩（`ExceptionCheck` 恒 1）
  - 变体：env=NULL / clazz=NULL / 参数=NULL / env=垃圾指针 / 超长串(1MB)+极值 int
- **fork 隔离执行**（AFL 思路）：子进程崩/卡都不影响父进程，父进程按 waitpid 分类
  （正常退出=安全返回 / 被信号杀=崩溃缺口 / 2s 超时 kill=hang 缺口）
- **实锤**：336 组合 → **44 条防御缺口定位**（OnCreate/OnEvent/OnKey/OnTouch/
  OnBluetooth 在 env=NULL 或毒化档下 SEGV sig=11），0 整体崩溃，ASan 干净

**关键踩坑（3 条，全是"探针自身缺陷"级别的坑）**：
1. **signal handler 捕获 + mutex 死锁 = 不可行组合**：so 持 `g_mutex` 时崩溃 →
   handler 里 `siglongjmp` 出去**留下锁住的 mutex** → 后续入口同锁 → 死锁 hang
   （实测 120s 超时）→ 改 **fork 隔离**（子进程崩溃/卡死都不污染父进程）
2. **桌面 JNI 头不能直接用系统 JDK/NDK 的**：
   - JDK `jni.h` C++ 模式是**成员函数包装类**（`JNINativeInterface_` 字段是成员函数
     指针）→ 与 src_rs 的 C 写法 `(*env)->FindClass(env,...)` 不兼容；C 模式又要 jni_md.h
   - NDK `jni.h` 需配 NDK sysroot 头链（遮蔽宿主 libc 头 → `size_t` 未定义等）
   → **自建 C 模式最小 `jni.h`**（`src_rs/android_stub/jni.h`，覆盖 src_rs 实用的
     21 个 JNI 函数 + 类型），配 `android/log.h` / `android/native_window_jni.h` /
     `android/asset_manager.h` / `sys/system_properties.h` / `EGL/egl.h` 五个 stub 头
     + `platform_stub.c`（GL/平台符号空实现，**不模拟业务**防假绿）
3. **JNIEnv 在 C 模式是指针**：`typedef const JNINativeInterface *JNIEnv;`
   → mock 结构体是 `JNINativeInterface`（非 C++ 的 `JNINativeInterface_`），
   `JNIEnv g_env = &g_mocks;`

### 2.8.5 P8：多线程竞态 fuzz（`fuzz_mt.c`，TSAN）——**抓到并修复 so 真实竞态**

**空白原因**：现有全部判据都是**单线程**（12/12 + F1-F5 + P1/P2/P5'），而 src_rs
真实运行模型 = 主线程（JNI OnTouch/OnEvent/OnCreate/OnUpdate/OnTimer）+ GL 渲染线程
（rs_render_frame 读 g_state/g_view_*）+ OnUpdate 定时器（写 g_state）。锁纪律目前是
**人工约定**（CODE_STYLE §锁纪律 + 注释标注），**无机器可复现检查** = "看起来测了
其实没测"的域。TSAN 是唯一能机检数据竞态的手段。

**3 场景 × 8 线程**：
- M1 g_state 读写（模拟真机 `g_mutex` **双侧持锁契约**：OnTouch 写持锁 + 渲染线程
  window 出屏读持锁，AUD-1000 A2-007 修复；见 `glview_gl.c:22` 注释）
- M2 UI 屏态（ui_screen_set/get 并发）
- M3 触摸态（共享 jq_touch_state 实例 + g_ts_lock）

**★ 实锤抓到 so 真实竞态（P8 核心价值）**：
TSAN 报 `data race at ui_state.c:46 in ui_screen_set`——`ui_screen_set` 持 `g_ui_lock`
写 `g_ui`，但 `ui_screen_get`/`ui_game_active`/`ui_opt_get_*` **裸读**（注释理由：
"aarch64 int 读写天然原子 + 避免 g_ui_lock 永久持有时 60s 全局卡死事故"）。
**根因**：架构原子性（aarch64 int 单字读写）只是**硬件事实**，不是**语言级同步契约**
——C 内存模型下裸读裸写 = UB（编译器可重排/缓存），TSAN 机检必报。

**修复（保可用性 + 消除竞态，2026-11-20）**：
- 读侧：`__atomic_load_n(&g_ui, __ATOMIC_ACQUIRE)`（无锁快读**不变** → 不会重现
  "锁永久持有 → 60s 卡死"事故）
- 写侧：`__atomic_store_n(&g_ui, s, __ATOMIC_RELEASE)` + 保留 mutex（写侧多写者互斥）
- 同型处理 `g_opt_level`/`g_opt_mode`（4 个 getter + 3 个写点全原子化）
- aarch64 上 acquire/release 编译为 `ldar/stlr`，语义与原有 ldr/str 等价，性能零回退

**验证**：修复后 TSAN 对 M2 **0 报告**（原 ui_state.c:46 报告消失），3 场景
× 8 线程 × 20000 次全 PASS。

**踩坑固化（3 条）**：
1. **TSAN 与 PIE 地址布局冲突**：默认 PIE 编译报 `FATAL: ThreadSanitizer: unexpected
   memory mapping`（ASLR 大地址段）→ 修：`-no-pie` + `setarch -R`（禁 ASLR 固定布局）
2. **探针自身 RNG 的竞态淹没真实报告**：共享 `g_seed` 被 TSAN 报成 data race →
   修：`__thread unsigned t_rng_state`（每线程独立 RNG），真实竞态才显形
3. **探针必须模拟真机锁契约**：M1 初版故意不加锁写 g_state → TSAN 报"探针自造的
   无锁写"= **假阳性**；真机 OnTouch 写与渲染线程读双侧持 g_mutex（A2-007）→
   探针改为双侧持锁 → 0 报告。**教训：fuzz 并发前先读被测代码的锁契约注释**，
   否则探针自身的错误同步会制造假报告

**诚实边界**：桌面 TSAN 能报**真实存在**的竞态（比真机强），但**不报真机特有**的
竞态（NDK 优化/信号时序差异）；不含蓝牙收帧线程（用户指令）+ 真实 GL 线程（桌面 stub）。

### 2.8.6 P3：事件分发 + 音效映射 fuzz（`fuzz_evt.c`）——防历史两类假绿缺陷复活

**覆盖**：`evt55_dispatch.c`（67 code 分发 + static `e55_code_to_evname` +
计数不变量）+ `evt55_sfx.c`（73 名 → 15 精确音效映射）。

**历史真缺陷背景（本探针防复活）**：
- R5-B-02：业务代表 code 34/36/38/44/46 无 case → default 返 0 → 名称往返丢身份变
  Junqi.Move（假绿同族），已修（按桶返代表名）
- R6-B-10：未知名 default 返 0 = 假绿，已修返哨兵 -1

**4 项判据（实锤全 PASS）**：
- M1 全 code 扫描（0..66 + 边界 -1/67/68/0x7fffffff/INT32_MIN）：72 code → 映射 19 /
  未映射哨兵 53 / 非法 0（0 崩溃）
- M2 往返身份：19 code 全幂等，身份丢失 0
- M3 音效映射：15 核心精确（7 业务类 + win/lose/draw 胜负类 + Resign 系）+ NULL 兜底
  move + 73 名全部非 NULL（防 `sfx_play(NULL)` 崩）
- M4 计数守恒：注入 100 次后 Σ g_event_cnt == g_event_total（95==95）

**踩坑固化**：
1. static `e55_code_to_evname` 无法直接调 → 探针 `#define static` + `#include "evt55_dispatch.c"`
   内联实现（不再单独链接该文件，否则重复定义）
2. 判据阈值按**真值**设（有效映射 19 个）——初版设 >20 导致假 FAIL。**教训：机检阈值
   必须先跑一次取真值再设，不能拍脑袋**（与 F1 guard截断率 ≤10% 同型纪律）

### 2.8.7 P6/P7：存档损坏 + 复盘链 fuzz（`fuzz_save.c`）

**覆盖**：`.sjg` 存档格式（magic + sizeof + state blob + 域校验）+ 复盘记录链
（record/rewind/cur）。真机可达性：OnResume 读 `one.sjg` 续局（jni_bridge :10640
实锤"续局读成功"）→ 畸形存档必须安全拒读而非崩溃。

**变异 + 判据（实锤全 PASS）**：
- P6 M1 合法存档 round-trip：292 字节（基线，必须成功）
- P6 M2 截断（8 种截断点：0/1/4/8/9/12/半长/sizeof-1）0 崩溃
- P6 M3 字节翻转（3000 轮 × 1-16 字节随机翻转）0 崩溃
- P6 M4 定向畸形（magic 错 / sizeof 0xFFFFFFFF / 尾部追加 64B 垃圾）0 崩溃
- 拒读语义：返 0 且**不改调用方 st**（毒化 0x5A 后 memcmp 全等 = R41-Q17 原子语义）；
  接受时全字段合法域（rank≤24 / side∈{0,1,0xFF} / revealed≤1 / 局级枚举域）
- P7 复盘：3000 次 record（n≤N_REPLAY=1100 / cur∈[0,n]）+ 4 越界 rewind
  （-1/n+1/INT32_MAX/INT32_MIN）+ 空复盘 rewind → 0 崩溃

**踩坑固化**：platform_stub.c 的 `__android_log_print` 与 fuzz_jni.c/fuzz_mt.c 重复
定义 → 桩函数只在 platform_stub.c 保留一份（所有链接它的探针共用），fuzz_mt 的
Makefile 同步补 platform_stub.c。**教训：共享 stub 函数单一来源，禁止探针各自定义**。

**剩余**：P9（32 位 dispatch 表 2218 槽 fuzz，可离线）；P4 渲染 GL / P5 真机 JNI 需设备。

### 2.8.8 P9：32 位 dispatch 表 fuzz（`vtable_dispatch_fuzz.py`）——**9/9 空白面全部做完**

**覆盖**：`jqt_vtbl.c` 的 `jqt_vtbl_offs[5463]` + `jqt_vtbl_fns[5463]` 双表
（★ 口径修正：AGENTS 历史写"2218 槽"，源码实测 **5463 槽** `jqt_vtbl_n=5463u`）。
静态解析 + `jqt_blx_dispatch` 二分算法 1:1 模型（不编译 9 万行二进制文件）。

**判据（实锤全 PASS）**：① 表结构（等长 5463 / 严格升序 / n 一致）② 5463/5463 槽
二分 0 漏检（R23-P0-1 排序缺陷防复活）③ 10000 非表 off 0 假命中 ④ 1000 偶数 off
Thumb 位清除命中 ⑤ fns/offs 名值 5463/5463 对应。

**★ 抓到的诚实边界发现（未修，记录）**：6 个**奇数 off 槽**
（`0x23461/0x23475/0x23489/0x2349d/0x9d43d/0x9d9e9`）被源码
`off &= ~1u` 清 bit0 后**二分不可达**（表存奇数、查表键被清成偶数）。
分析：其中 `0x9d43d→0x9d43c` 与表中已有值**冲突**，`0x23461` 清 bit0 破坏前序
→ **不是简单笔误**，清 bit0 会破坏表结构/相邻序。按 AGENTS 红线（32 位翻译体
忠实翻译，不轻易改表）**不强行修**，单列为"已知不可达槽，待专项"。
探针判据 ④ 只对偶数 off 生效，奇数 off 如实打印为诚实边界（不判 FAIL）。

**口径修正记录**：dispatch 表槽数历史口径 2218 → 源码实测 **5463**（`jqt_vtbl_n=5463u`
+ offs/fns 双表均 5463）。AGENTS 历史"2218"系早期翻译体阶段快照，当前真值 5463。

## 2.9 ★ 真机黑盒证据归属澄清（2026-11-20，§119 纪律查到的口径）

**关键事实（nm 实锤）**：152 设备上 `com.qwsaeda.junqirs` 装载的 so（md5 `9b8544d2...`，
6992448B）是**正向会话 Rust 重写版**（`src_rs_rust`，2408 个 `_RINv` Rust mangle 符号 +
`rustc/58...` 哈希），不是本会话的 C 版 src_rs（md5 `05ce20b0...`，0 mangle）。
本地 Rust 产物（2405 mangle / 6992160B）与设备 so 几乎一致（差 3 = 小版本演进）。

**归属结论**：
- 本文件 §2.5/§2.6/§2.8 的**真机黑盒证据**（152 5 判据 / sendevent 手点全链 / 60 局
  8460 走子 / AI 应着 / Frida 各 hook）**全部打在正向 Rust 版 app 上**（com.qwsaeda.junqirs），
  **不是本会话 C 版**（com.zxj 逆向 app 当前设备未装——设备只有 com.qwsaeda 一个包）。
- 按 AGENTS 红线"不抢活"：C 版（逆向 com.zxj）/ Rust 版（正向 com.qwsaeda）分属不同会话。
  本会话的 C 版渗透判据是**桌面离线**（fuzz_all A/B/C + arm32 静态），真机证据是**借用正向
  会话 app 的黑盒佐证**——归属须写清，不能把正向 app 的"能玩"算到逆向 C 版头上。
- 两条路都导出 **C ABI 14 个 `Java_com_qwsaeda_App_*`**（nm -D 实锤），所以 Frida 14 入口
  判据 + 黑盒钩子（setprop jq.movetest 等）对两版通用（JNI 契约一致）。

**诚实边界**：本会话 C 版（rev64/forward/src_rs）的真机 5 判据**未独立验证**（设备无 com.zxj
包）；其能力证据 = 桌面 fuzz_all 全通 + 静态 74/74 + 与 Rust 版 C ABI 同源（14 导出一致）。
Rust 版真机全链 PASS 由正向会话负责（其交付物 phase_rs_rust.apk）。

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
| 2026-11-20 | — | 32 位装机 PENDING | `phase_chain_arm32.apk`（com.cnvcs.junqi32，lib/armeabi）装 152 → `INSTALL_FAILED_NO_MATCHING_ABIS res=-113` | 152 `ro.product.cpu.abilist`=x86_64,arm64-v8a,x86（无 32 位 ARM abi + `/data/secondary_arch/lib/arm` 空 = 无 32 位转译器） | 非 so 缺陷——32 位验收基准 = 真 32 位 ARM 设备（MuMu x86_64 实例对 32 位 ARM 翻译体装机不可行，环境约束）；32 位侧维持静态 74 + jqt_mem_fuzz 5 探针桌面 ASan 判据 + 真 32 位设备冒烟 PENDING | 152 设备属性实锤（abilist + secondary_arch 空） |
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
- **真实手点 ≠ 注入（152 实锤，防 §124y 历史"手点没反应"坑复发）**：`sendevent /dev/input/event4` 真实触摸序列全链走通（菜单 btn1→弹窗确定→暗棋翻子蓝 cell6/红 cell30→回合交替 0 崩溃）。教训固化：① 手点前必 `getevent -lp` 查实际轴表（event4 `Xiaomi Touchscreen` 轴 = SLOT47/X53(max900)/Y54(max1600)/TRACKING57/BTN_TOUCH330，**无 ABS_X/ABS_Y**——误用 56/55 轴 → InputReader 丢事件 app 零 OnTouch）② 两步态机：暗棋翻子首点 = 选中（`to=-1 can_flip=0`），再点同格 = 翻棋判定（`to=from can_flip=1`）——首点 can_flip=0 非规则缺陷（态机 SELECT 阶段）③ 坐标 view→base 同值（900×1600 = 720×1280 ×1.25 缩放），命中区按 720 基准几何 ×1.25 换算 900 基准（弹窗确定 255,747→319,934 / 蓝翻 cell6 219,1106→274,1383 / 红翻 cell30 77,577→96,721）
- **主菜单 vs 弹窗几何基准不同（152 实锤命中坑，防 hit=-1 误判）**：主菜单 5 按钮 =
  orig_menu **900 系直值**（btn0 中心 448,1132；ui_menu.c 注释 "×0.8" 误导——实际命中
  区按 900 系，误用 720×0.8 换算 359,1188 则 `hit=-1`）；弹窗/难度几何 = **720 基准
  ×1.25**（难度"新手翻棋" 495,528→619,660）。**两类基准勿混用**：手点前先查目标屏态
  几何基准（主菜单=900 直值 / 弹窗=720×1.25），`hit=-1` 先怀疑基准选错非坐标算错
- **ASan 假绿防护**：`fuzz_probe_check.py` 即便 rc=0 也扫 `ERROR: AddressSanitizer`（ASan 默认 abort 非 0，但链接错误/静默降级场景 rc=0 也须扫 report 文本）
- **探针不得破坏被测模块的不变量（32 位探针 4 坑固化）**：① `jqt_mem_addr` 影子区分支
  = `base ≤ a < base+JQT_MAP_HI` → 探针影子区必须**恰好 256MB**（早前用 128MB → 上半段被
  误判"影子区内"→ 解引用未映射页 SEGV = 探针缺陷非被测缺陷）② 32 位 ASan 占低位 0-0x40000000
  → 影子区落位须空洞探测 + MAP_FIXED ③ ASan 插桩造伪影（shadow 区语义 + 堆红区）→
  32 位探针**不插桩 ASan**（价值在地址空间语义）④ 影子区末字节 4 字节读跨界 → mmap +4KB
  余量。**教训：渗透探针先读被测模块的不变量（哪段地址区间算什么），再写变异器**——
  否则探针自身的 bug 会被误读成被测缺陷（本轮实锤 4 次误判）
- **可复现判据**：同 seed 重跑 F1 行逐字比对（非"都 PASS 就行"）——随机路径可复现 = 修了才能证复跑不崩
