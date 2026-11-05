# 第17章 unidbg 实战：Debugger 自吐

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的 Debugger 自吐

## 痛点：为什么"模拟执行"看不到内部发生了什么？

你在 unidbg 里调军棋 `OnCreate`（14 个 JNI 入口之一，Thumb 奇地址 0x233e1，
调用前要 `&~1` 对齐到偶指令），它返回了 0。你不知道"0 是成功还是崩了被兜住了"。
军棋实测 3 个卡点。

### 卡点 1：0 到底意味着"成功"还是"内部崩了但被你兜住了"？

你不知道。军棋 so 一崩就返回 0（被安全阀兜住），但你分不清"0=正常返回"还是
"0=崩了被兜成 0"。

- **大白话卡点**：函数返回 0，你分不清"正常 0"还是"崩了被兜成 0"。
- **为什么卡**：unidbg 黑盒模拟，崩了也返回 0，没日志。
- **不解决会怎样**：一直以为"能跑"，实际内部早崩了——军棋影子运行时的
  生命周期安全阀（递归深度阀超 0x1000 `longjmp` 放弃返回 0、每入口调用预算
  0x200000 耗尽返回 0、越界内存导 4KB 零页）全是"把崩变成 0"的设计。

**军棋"安全阀兜底"（红线 §R6 生命周期安全阀三件套）**：
军棋影子运行时有 3 道"兜底"：递归深度阀（超 0x1000 → `longjmp` 放弃返回 0）、
每入口调用预算（depth=0 重置 0x200000，耗尽 → 同上 longjmp）、越界内存阀
（垃圾指针导到 4KB 零页 `g_jq_scratch`）。so 崩了被兜成 0，你看返回值 0 以为
正常，实际内部早崩了——必须自吐才能分清。

### 卡点 2：想下断点看某条指令的寄存器值，没有真调试器

unidbg 是黑盒模拟，不像 IDA/Frida 那样能单步、下断点看寄存器。

- **大白话卡点**：想看某条指令执行时 r0 是什么值，黑盒里没这个"显微镜"。
- **为什么卡**：unidbg 默认不追踪内部状态（追踪每条指令开销极大，军棋 **1.43MB/1435156B**
  so 指令百万条）。
- **不解决会怎样**：排错只能猜，看不到"哪条指令读了什么值"——军棋 §124 的
  ① 上下文相关 BL 解码 bug（BL 0x7965a 错位到 0x6eaea 而非 0x43ed8）就是这么
  靠针对性追踪（DiagTracePC）才发现的，全量追踪不现实。

**为什么 unidbg 默认"不追踪"？** 追踪（每条指令都记）开销极大，默认关着。
要排错才开（DiagTracePC/DiagJni 是军棋的"针对性追踪"，只追关键点，不全追）。

### 卡点 3：想"让 so 自己把关键值打印出来"（自吐），so 没 printf，你加不了

军棋 so 是 stripped 二进制，你没源码、加不了 printf 让它"自己吐值"。

- **大白话卡点**：你想让 so"把关键值喊出来"，但 so 没 printf、你也没法往里加。
- **为什么卡**：so 是编译好的二进制，加不了打印语句。
- **不解决会怎样**：想看 18 个 `.text` 常量池的值（15 个 `.text` + 2 个 `.data`
  共 72B），静态读不出（dlopen 重定位后才填），动态 Hook 又卡 0x43ed8。

**为什么"加不了 printf"？** so 是 strip 的（符号表删了、没源码），你在 unidbg 侧
只能"外挂"（Hook 入口/Debugger 追踪），不能"内嵌"（改 so 加打印）。军棋红线
§R6 强调"不碰 so 字节"，自吐走"外挂"路线。

## 为什么这样做：用 Debugger 模块"自吐" so 的内部状态

"自吐" = 让被模拟的 so 自己把关键数据写出来（寄存器/内存/返回值）。
类比：黑盒里有个小机关，你按下开关（Hook/Debugger），机关把内部零件（寄存器值）
吐到外面给你看。

**为什么 Debugger 比"改 so 加 printf"好？** 改 so 要重签名破坏原版；Debugger 是
"外挂"在 unidbg 侧追踪，so 本体一字节不动。军棋红线 §R6 强调"纯指令翻译/
反汇编 C"，不碰 so 字节——Debugger 自吐正好符合（外挂追踪，不改 so）。

**预判追问："直接 Hook 每条指令记录日志不就全看到了？"** 可以但代价极高：
军棋 so 跑 `OnCreate` 一链下来指令执行量是百万级，全量 Hook 日志会把模拟
拖到分钟级甚至 hang（军棋 C 端 harness 的教训：长循环函数要加 8s 超时归 SKIP）。
实践上走"三点式"：`runTo` 到关键地址 → `step` 关键指令 → `showRegs` 拍现场照。

**"外挂"和"内嵌"（大白话）**：
- 内嵌（改 so 加 printf）：往 so 里塞打印语句，破坏原版 + 要重签名
- 外挂（Debugger/Hook）：在 unidbg 侧"拦下来看"，so 本体不动
军棋全程走外挂（Hook 两大卡口 + Debugger 四镜头），so 本体 100% 对齐不破坏。

unidbg 的 Debugger 模块提供 3 种自吐方式：

| 方式 | 大白话 | 军棋场景 |
|---|---|---|
| `runTo(addr)` | 跑到某地址停下 | 跑到 `OnCreate` 入口 0x233e1 |
| `step()` | 单步走一条指令 | 看 `ldr r0,[pc,#N]` 读了哪个常量 |
| `showRegs()` | 打印当前寄存器 | 看 r0-r12 + pc 值 |

**3 种方式配合（大白话）**：`runTo` 先"开到路口"（OnCreate 入口），
`step` 再"一步一慢动作"（看每条指令），`showRegs` 最后"拍现场照"（看寄存器值）。

## 17.1 军棋 Debugger 自吐实战


> **本节核心**：
> 1. 军棋 Debugger 自吐实战（OnCreate 卡点 0x43ed8）
> 2. so 没 printf 加不了，Hook 入口打印；军棋 0x43ed8 std::vector 拷贝构造点
>
> **本节难点**：
> 1. 自吐 = 让 so 自己把关键值吐出来（Hook 入口打印）
> 2. 军棋卡点 0x43ed8 靠 Debugger 单步定位

![Debugger 自吐](assets/ch17_debugger_selfdump.png)



```java
// 军棋 32 位 so 的 Debugger 自吐（让 so 把内部状态吐出来）
Debugger debugger = new Debugger(emulator);
debugger.runTo(0x233e1);   // 跑到 OnCreate 入口（军棋 DiagJni 实证）
debugger.step();           // 单步走一条指令（慢动作）
debugger.showRegs();       // 看寄存器（拍"现场照片"）
// 军棋 DiagJni:OnCreate 推进到 std::vector 拷贝构造点 0x43ed8（§124 ② "数据与指令对齐对应"卡点，N44）
// Debugger 能看到 r0 指向 0x43ed8(vector 首元素地址)
```

**军棋 14 个入口名（readelf 实测，自吐时可 runTo 任一入口）**：
OnCreate / OnDestroy / OnResult / OnResume / OnEvent / OnKey / OnUpdate /
OnGraphChange / OnBluetooth / OnPause / OnGraphCreate / OnTimer / OnInput / OnTouch。
注意这些都是 Thumb 奇地址（如 OnCreate 0x233e1），`runTo` 前同样要 `&~1`
对齐（军棋红线 §R6：JNI 入口是 Thumb 奇地址，不对齐可达集空）。

**为什么军棋用 Debugger 自吐？** 军棋 `OnCreate` 卡在 §124 的 3 项阻塞
（① 上下文相关 BL 解码 bug：BL 0x7965a 被 unicorn2 解码成 0x6eaea（memset
循环）而非真实目标 0x43ed8（std::vector 构造）② **数据与指令对齐对应缺口**（N44：std::vector 的
begin/end/capacity 未初始化 = 运行时内存布局没对上，非"对象图缺失"）③ 288MB PROT_NONE mmap unidbg 处理失败），
Debugger 能看到"它读到了 0x43ed8"，但读到的 begin/end 是未初始化值 → 写未映射
地址崩。Debugger 自吐帮你定位"卡在哪条指令"，但定位≠解决。

**0x43ed8 是什么（大白话）**：`std::vector`（C++ 动态数组）拷贝构造点（首元素地址）。
军棋 `OnCreate` 推进到这，vector 的 begin/end/capacity 未初始化（§124 的 ②
数据与指令对齐对应缺口，N44）→ 读未初始化值 → 写未映射地址崩。Debugger 自吐帮你
确认"卡在这"，但"补运行时内存布局"是 §124 的 ②，unidbg 路线补不了。

## 17.2 自吐的 3 条路径（书内 §17）


> **本节核心**：
> 1. 自吐 3 路径：Debugger 单步（定位崩点）/ Patch 指令（军棋少用）/ Hook 函数（入口打印）
>
> **本节难点**：
> 1. Patch 军棋少用（.text 100% 对齐，改 so 字节破坏原版 + 要重签名）
> 2. Hook 打印 18 常量走不通（OnCreate 卡 0x43ed8 跑不到解包点，退 redroid）
> 3. 3 路径选择：定位崩点 → Debugger；拿运行时常量 → Hook（军棋走不通）；改行为 → Patch（军棋基本不用）
> 4. Patch 自吐 = 改写 ldr rX,[pc,#N] 成 mov rX,#val 自吐常量值（军棋场景少用）

![自吐 3 路径](assets/ch17_selfdump_flow.png)



| 路径 | 大白话 | 军棋适用 |
|---|---|---|
| Debugger 模块 | 单步 + 看寄存器 | ✅ 定位 `OnCreate` 卡点 0x43ed8 |
| Patch 指令 | 改写某条指令把值写进内存 | ⚠️ 少用（军棋 .text 100% 对齐，改 so 字节破坏原版） |
| Hook 函数 | 函数入口/返回时打印参数/返回值 | ✅ 打印 18 个 .text 常量（这条路卡住，退 redroid） |

**为什么"Patch 指令"军棋少用？** 军棋 `.text` 已 100% 对齐（可跑函数
1174/1174 PASS，红线 §122），Patch 会破坏原版指令流 + 要重签名。军棋优先用
Debugger（追踪）+ Hook（入口打印），Patch 是最后手段（改 so 字节，破坏原版）。

**为什么 Hook 函数适合"打印 18 个 `.text` 常量"？** 18 个常量池值只有 dlopen 后
才填（`.so` 未重定位，`.got` 槽为零，静态文件读不出），Hook 在解包函数入口跑通后
读 18 地址 → 吐出来。军棋这条路走不通（`OnCreate` 卡 0x43ed8，跑不到解包点），
退而用 redroid dump（工具三件套：`frida-dump/redroid_jni_dump.py` 精准读
18 地址 + `redroid_dump_jni.sh` 一键触发 + `gen_runtime_table.py` JSON→C 注入表）。

**3 条路径怎么选（大白话）**：
- 定位崩点 → Debugger 单步（看哪条指令读错值）
- 拿"只有运行时才有的值" → Hook 入口打印（跑通后读）
- 改行为（军棋基本不用）→ Patch（破坏原版，最后手段）

## 17.3 逐环节"我们能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **runTo** | 跑到 14 个 `On*` 入口（Thumb 奇地址先 `&~1`） | 单步穿越 §124 三项阻塞（0x43ed8 前后读未初始化值必崩，被安全阀兜成 0） |
| **step / showRegs** | 看 r0-r12 + pc 定位"哪条指令读了 0x43ed8" | 靠单步"走通"拷贝构造点（崩在数据缺失，不在指令，单步再细也补不出 std::vector 运行时内存布局） |
| **Hook 打印** | 14 入口入口/返回参数可打印（DiagJni 实证） | 打印 18 个 `.text` 常量（跑不到解包点，静态读不出，只能 redroid dump 72B） |
| **自吐定位** | 定位 §124 的 ②（0x43ed8 std::vector 数据与指令对齐卡点） | 把定位当解决（定位 ≠ 修复，三项阻塞 unidbg 补不了） |

## 17.4 军棋实战验证：Debugger 定位 OnCreate 卡点


> **本节核心**：
> 1. Debugger 定位 OnCreate 卡点 0x43ed8
>
> **本节难点**：
> 1. 「0 到底是成功还是内部崩了被你兜住」：Hook __cxa_atexit 返 0 跑通可能内部已崩
> 2. 单步看寄存器才知道真实状态
> 3. Debugger 是「行车记录仪」，Hook 兜住的「0」可能掩盖内部崩溃

军棋 unidbg Debugger 自吐实测（红线 §R6，unicorn2 后端）：
| 自吐动作 | 军棋结果 | 大白话 |
|---|---|---|
| runTo(0x233e1) | 跑到 `OnCreate` 入口 ✓ | 定位到入口 |
| step + showRegs | 看到 r0 指向 0x43ed8 | 单步看到拷贝构造点 |
| Hook 打印 18 常量 | 卡（`OnCreate` 没跑通） | 18 常量需 redroid |
**Debugger 看到的 0x43ed8 是什么？** `std::vector`（C++ 动态数组）拷贝构造点（首元素
地址）。军棋 `OnCreate` 推进到这，vector 的 begin/end/capacity 未初始化（§124 的
② 数据与指令对齐对应缺口，N44）→ 读未初始化值 → 写未映射地址崩。Debugger 自吐帮你
确认"卡在这"，但"补运行时内存布局"是 §124 的 ②，unidbg 路线补不了。
**"能定位 ≠ 能解决"（诚实边界）**：Debugger 自吐帮军棋**定位**了卡点（0x43ed8
向量拷贝构造点），但定位后"补运行时内存布局"是 §124 的 ② 数据与指令对齐对应缺口，unidbg
模拟不了真实 C++ 布局。能定位卡点 ≠ 能解决卡点。
## 17.5 本章小结（一句话版）

> 模拟执行是黑盒，自吐 = 让 so 自己把内部状态吐出来（Debugger 追踪 + Hook 入口打印）。
> 军棋用 Debugger（runTo 0x233e1 / step / showRegs）定位 `OnCreate` 卡在
> std::vector 0x43ed8，再用 Hook 打印 18 个 `.text` 常量（这条路卡住，退用 redroid）。
> §124 的 3 项阻塞：① 上下文相关 BL 解码 bug（BL 0x7965a→0x6eaea 错位）
> ② 数据与指令对齐对应缺口（std::vector 0x43ed8，N44：业务数据 apk 静态可推，缺运行时对齐）③ 288MB PROT_NONE mmap。
> Patch 军棋少用（.text 100% 对齐，改 so 字节是最后手段）。

## 视频
