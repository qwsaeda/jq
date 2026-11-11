# R-route · 逆源码重写（src_rs）阶段记录

> 目标：把原版 32 位 `libjunqi.so`（1435156B，strip）的【游戏逻辑】逆出来，
> 重写成【能正常编译运行的 C 源码】（能开发、能跑、能玩）——逆源码路线（非模拟路线）。
> 本文件记录逆向证据链 + 产物 + 诚实边界。

## 1. 逆向证据来源（原版 .so，非翻译体）

- 原版 so：`军棋.apk` 解出的 `lib/armeabi/libjunqi.so`（1435156B，ELF32/ARM/EABI5，strip）。
  工作副本：`/tmp/opencode/rs/orig.so`（VA==Off，`.text` 0x233e0 起）。
- 翻译体 so（`rev64/dist/libjunqi_arm32_aligned.so` 5.4MB，含 `g_jqt_*`）仅用于对比，**不**作逆向源。
- 反汇编工具：capstone（`/tmp/opencode/disasm_rs.py`，Thumb + PIC 字面量解析）。
- 字符串锚点：`frida-dump/rodata_strings.txt`（1715 条，含 `Junqi.*` 事件名）。
- 规则文本：SPAK 解包 `dist/spak_res/data_junqi_rule{lay,flip}.txt`（GBK 原文，权威规则）。
- 布局：SPAK `data/lay/00..49.lay`（50 个 30 字节预设，`00.lay==default.lay`，L52 分布确证）。
  ★ AUD-1000 B09-008 口径：SPAK 含 50 个布局预设，src_rs 实现 12 个（00-11，
  make laytest 12/12 + OnEvent layout 只接受 0-11）——50 为资源全量口径，
  12 为 src_rs 已实现口径（剩余 38 个未接入，诚实边界），非口径矛盾。

## 2. 逆向出的核心函数（控制流 + 常量 + 结构布局 + 业务名）

| 原版 VA | 业务名（重写） | 逆向控制流（capstone 确证） |
|---|---|---|
| 0x39e14 | `jq_check_target`（目标类型判定） | `r2>0xc` 非法；`0xa` 炸弹(`to_row!=0`)；`0xb` 地雷(`from_row>3` 工兵)；`0xc` 军旗(`from_row∈{1,3,5}` 特判 0x39e7a)；普通 1-9 合法（0x39e54 `mov r0,#1`） |
| 0x39f20 | `jq_try_move`（走子执行器） | from==to→翻棋（0x39f90 返回路径）；`0x6ef54` 坐标比较（[0]=col [1]=row）；格子 +0x80+idx、rank 在格 +4（L65/L79）；成功 `mov r3,#1; b 0x39f90`（无连吃，L54） |
| 0x3a15c | `swap_cell_rank`（走子交换） | from rank↔to rank 互换（`ldrb/strb` 链） |
| 0x2a228 | `jq_check_layout_cell`（单格校验） | `idx<=29` 合法；`raw>0xc` 还原 `-0xc`；炸弹 `idx<=34`→ZhaError；地雷 `idx<=49`→LeiError；军旗 `idx∈{56,58}` 否则 QiError（§10/L81 逐指令） |
| 0x6a3b0 | `jq_validate_layout`（60 格校验 + 计数表） | 循环 60 格，`rank>0xc` 还原，`rank-0xb<=1` 军旗/地雷特判，计数表 +0x78（§7） |
| 0x2a340 | `jq_load_layout`（布局加载） | 循环 30 字节，经 0x6a3a4 映射（`byte==0→0; side==0→byte+0xc; else byte`，§3） |
| 0x6a3a4 | `rank_map`（字节→rank 映射） | 蓝原值 / 红 +0xc |
| 0x30200 | 胜负分发 | `r3=0/1/2`（蓝胜/红胜/和）→ `Junqi.Win/Draw` 事件（§12） |
| 0x30260 | `jq_on_flag_capture`（吃旗胜） | 计数 +1 → 0x301f0 |
| 0x30280 | `jq_on_draw`（和棋） | 字段 `-(count-1)+2` → 0x301f0 |
| 0x302b4 | `jq_on_stalemate`（困毙） | `status=3`（L53） |
| 0x6ef54 | `coord_cmp`（坐标比较） | `[0]=col [1]=row`，0=相同（L51/L79） |
| 0x26f24/0x26efc | `jq_bt_frame_encode`（v2 7 字段帧） | `sprintf "v2\n%d,%d,...\n"`（§92） |

> 注：.text 字面量池在原版 .so 中**部分为编码值（enc）**（0xe0000-0xf0000 段，运行时
> 经 0xb9c98 解码），非直接字符串指针。本重写**不**依赖 enc 解码——规则常量直接
> 取自 `rodata_strings.txt` 明文（`Junqi.*` 事件名、布局校验阈值 0x2a228 的
> `0x1d/0x22/0x31/0x38/0x3a` 立即数）+ SPAK 解包的规则文本，证据链完整。

## 3. 重写产物（rev64/forward/src_rs/）

| 文件 | 职责（对应原版） |
|---|---|
| `junqi.h` | 公共定义：rank 语义（0x6a3a4）、棋盘 12×5（L79）、`jq_state`（0x39f20 偏移对齐） |
| `board.c` | 铁路/行营/大本营（L49/L68 十字铁路）、坐标比较（0x6ef54） |
| `layout.c` | 布局加载（0x2a340）/ 单格校验（0x2a228）/ 全格校验（0x6a3b0）/ default.lay |
| `rules.c` | 走子（0x39f20）/ 吃子 / 翻棋（0x39e14）/ 困毙（0x302b4）/ 和棋（0x30280）/ 悔棋 |
| `save.c` | one.sjq 进度（0x37664/0x379ec L74）/ one.sjg 对局存档（0x390ac L89） |
| `bt_protocol.c` | 蓝牙 v2 帧（0x26f24）+ 60 格全盘同步（§92，UUID/服务名/版本确证） |
| `main.c` | 桌面 headless 可执行（走一局 + 悔棋 + 暗棋翻棋） |
| `jni_bridge.c` | 14 个 On* JNI 入口（签名与 App.smali 一致）+ 规则引擎接线 |
| `regress.py` | 回归：N 局 AI 对弈 + 悔棋 + 吃子事件覆盖断言 |
| `build_rs.sh` | 构建：desk（桌面）/ so（NDK arm64）/ apk（可安装双 ABI） |

## 4. 验收（AGENTS 判据：能编译 + 能跑一局 + 走子/吃子/悔棋对齐原版）

- **能编译**：`make desk`（x86_64）+ `make so`（NDK arm64）双通过，0 error。
- **能跑一局**：`regress.py --games 60` → **60/60 合法终局**；事件累计
  move 1200 / eat 300 / bomb 60 / dead 60；悔棋 20/20。
- **对齐原版规则**（`rodata_strings.txt` + SPAK 规则文本 + 0x2a228 立即数）：
  - rank 语义（0x6a3a4 蓝原值/红+0xc，L47）；25 子/方标准分布（L52 default.lay 确证）
  - 炸弹/地雷/军旗布局校验阈值（0x2a228：`0x1d/0x22/0x31/0x38/0x3a`，§10/L81）
  - 铁路十字拓扑（L68：col2 纵线 + row5/6 横线；行营/大本营在铁路外）
  - 和棋 70 步无吃子 / 总 1000 步（rulelay 原文 + 0x30280）
  - 困毙无棋可走判负（rulelay 原文 + 0x302b4 status=3）
  - 蓝牙 v2 7 字段帧 + UUID + 服务名 + 60 格同步（§89/§92）

## 5. 诚实边界（红线：不包装成"完整版"）

- **渲染 / 音频 / 蓝牙 SPP socket 为桩**：`jni_bridge.c` 的 OnTouch/OnKey/OnUpdate 返回
  不渲染的桩；OnCreate 返回 NULL（Java 侧 new GLView 兜底）。原版 C++ 场景对象图
  （GL 渲染 / AudioTrack / BluetoothSocket）月级，非本次"逆源码重写游戏逻辑"范围。
- **7 字段帧语义为行为级约定**（type,fc,fr,tc,tr,rank,side）；原版字段 offset
  81/82/83/84/808/1780/1784 需 C++ 类布局逆向（AGENTS 红线：不伪装已实现）。
- **one.sjq 键值文本格式为重建**（原版 0x379ec 是 C++ 长链，未逐字节确证分隔符，L83）。
- **APK 双 ABI**：`armeabi` 保留原版 32 位翻译体（5.4MB），`arm64-v8a` 用本次重写
  （26KB）。32 位设备走 v7a 原版逻辑；64 位设备走重写规则引擎（无渲染桩）。
- **C 源码原貌（变量名/注释/源文件结构）逆不出**（编译多对一），本交付是
  "行为级等价 C 源码"（指令语义 + 控制流 + 常量 + 结构布局 + 业务名），非逐行还原。

## 6. 复现

```bash
cd rev64/forward/src_rs
make desk                          # dist/jq_run（桌面 headless）
python3 regress.py --games 60      # 走一局 + 悔棋 + 吃子回归
NDK=/tmp/opencode/android-ndk-r25b make so   # dist/libjunqi_rs.so（arm64，14 On*）
NDK=... ./build_rs.sh apk dist/libjunqi_rs.so  # dist/phase_rs_apk.apk（可安装）
```
