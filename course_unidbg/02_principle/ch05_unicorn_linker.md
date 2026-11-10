# 第5章 Unicorn 初级使用与初探 Linker

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so + unicorn2 后端

## 痛点：为什么 unidbg 跑军棋 so 会"跑偏"？

这一章的痛点全来自"模拟出来的结果和真机对不上"。军棋实测踩过 3 个深坑。

### 痛点 1：PC 差了 2 字节，程序就乱跳

军棋用 dynarmic（一个把 ARM 指令翻译成 x86 的 CPU 模拟库）做后端时，Thumb（ARM 的
16 位指令模式）指令 `add rX, pc`（取"程序计数器 + 偏移"算地址）被算错 2 字节——
Dynarmic 用 `(addr+4)&~3`（向下对齐到 4 字节）取 PC，真实 ARM 用 `(addr+4)` 不 mask。

- **大白话卡点**：算"门牌号"时，真 ARM 是"4 楼 4 号"，dynarmic 给你算成"4 楼 2 号"。
  差 2 字节，后面所有"按门牌号找人"全找错房间。
- **为什么卡**：dynarmic 翻译 Thumb `add rX,pc` 时多做了一步 `&~3` 掩码（对齐到 4 字节），
  真实 ARM 在寄存器形式下不做这个 mask。
- **不解决会怎样**：军棋 0xa37fc 处存进 `.data` 的地址差 2，后面所有跳转全乱，
  模拟结果和真机对不上（DiagTracePC 实测 dynarmic 存 `0x159880 & ~3`，错）。

### 痛点 2：JNI 调用一执行就崩，不知道崩在哪

军棋 `OnCreate` 要调 `FindClass`、`callStaticJniMethod`（JNI 层函数，so 调 Java 侧的"电话"），
模拟环境里没人应答这些"Java 侧的电话"，直接空指针。

- **大白话卡点**：so 想给"Java 客服"打电话（`FindClass` 查类），你 PC 上没这个客服，
  电话没人接 → 返回 null → 拿 null 调方法 → 空指针崩。
- **为什么卡**：unidbg 没建 DalvikVM（Java 虚拟机替身），JNI 交互面没人应答。
- **不解决会怎样**：`OnCreate` 一步 `FindClass` 就崩，根本推进不到游戏逻辑。

### 痛点 3：不知道 unidbg 的 Linker（链接器）到底做没做对

军棋 12 个 `.init_array`（初始化数组，加载时跑的一次性构造函数）、5312 个 RELATIVE
（相对重定位）、214 个 PLT 槽、6 个 GLOB_DAT 槽，这些"填号"动作到底过没过？
需要逐阶段对照验证。

- **大白话卡点**：so 搬进内存后有一堆"占位符"要填真实地址，填漏一个就崩。
  你得逐项核对"5312 个填了没、214 个填了没"。
- **为什么卡**：unidbg 是简化 Linker，补不全所有运行时语义；得手动逐项验证。
- **不解决会怎样**：以为"加载成功"其实填漏了重定位，跑 `OnCreate` 就段错误。

## 为什么这样做：换 unicorn2 后端 = 换一个"翻译更准的 CPU"

unicorn（开源的 CPU 指令模拟库，QEMU 的用户态版）做 unidbg 的 CPU 后端，
类比"把翻译错误的同声传译员换成一个靠谱的"：

- **dynarmic**：快，但 Thumb 的 `add rX,pc` 用了 `&~3` 掩码 → 军棋实证 off-by-2 bug（PC 差 2 字节）。
- **unicorn2**：把 `(addr+4)` 原样存，与真实 ARM 一致 → 军棋 DiagTracePC（追踪 PC 的调试工具）
  实证 0xa37fc 存 `0x159880`（不 mask 值），bug 消失。

军棋红线 §R6 结论：**32 位 ARM 一律用 unicorn2 后端**，别省这个事。

**为什么是"换后端"而不是"改 dynarmic 代码"？** 改 dynarmic 源码要跟版本走、维护成本高；
unicorn2 是独立后端，换起来一行 `.backend(Backend.UNICORN2)`，且精度对军棋已实证通过。

**预判追问："那 unicorn2 就完全没问题了？"**
不。unicorn2 修的是 dynarmic 的 PC off-by-2（`add rX,pc` 寄存器形式），但 §124 新发现的
**上下文相关 BL 解码 bug** 是 unicorn2 自己的精度问题（加载 `.so` 后 BL 0x7965a 的
`F000 F800` 解码结果随上下文变化：隔离环境算对 0x10000004，加载 so 后错位到 0x6eaea，
真实目标 0x43ed8）。这是**模拟器精度 bug**（非翻译/逻辑 bug），军棋退而用
redroid dump + arm64 对齐版验证。unicorn2 不是"万能钥匙"，它修了 PC off-by-2，
但卡在 §124 的 ①（上下文相关 BL 解码）。

另外两条"先看清 Linker 再做"的原则：

- **静态 `.so` 文件 ≠ 跑起来的 so**。真机 dlopen 时 Bionic Linker（Android 自带的动态
  链接器）会重定位、填 `.got`；你手里的静态文件是"没填号的电话簿"。unidbg 的简化
  Linker 能补 RELATIVE/PLT/GLOB_DAT，但补不全所有运行时语义。
- **跑不动 ≠ 是壳（加壳保护）**。军棋 `.rodata` 静态文件与 redroid（32 位 Android 容器）
  运行时 dump 逐字节相同（0/118832 不同），没有自解密例程——卡点在 3 项阻塞（§124 纠正后）：
  ① 上下文相关 BL 解码 bug（加载 .so 后 BL 0x7965a 被 unicorn2 解码成 0x6eaea，真实目标 0x43ed8）
  ② 数据与指令对齐对应缺口（std::vector begin/end 未初始化）
  ③ 288MB PROT_NONE mmap（unidbg mmap2 处理失败）

## 5.1 Unicorn 的初级使用（手动搭一个最小模拟环境）


> **本节核心**：
> 1. Unicorn 最小模拟环境：手动 mmap + 跑指令（不经过 unidbg 高层 API）
> 2. 军棋 7 依赖 + 2 GL 库无真 GPU → 挂桩函数（glGetError/glBegin 返 0）
>
> **本节难点**：
> 1. Unicorn 是底层 CPU 模拟器，unidbg 在其上建文件系统 + DalvikVM
> 2. 挂桩 GL 是军棋特色：无真 GPU，GL 函数全返 0（渲染不可用但 so 不崩）
> 3. 初级使用 = 手动搭，军棋走 unidbg 高层（for32Bit + AndroidResolver）

![unicorn 模拟 + Hook 流程](assets/ch05_unicorn_flow.png)



```python
# 手动搭一个最小 ARM 模拟环境（理解 unidbg 内部在干什么）
from unicorn import *
from unicorn.arm_const import *

# 1. 创建 unicorn 实例：ARM 架构、小端（32 位 ARM 字节序）
uc = Uc(UC_ARCH_ARM, UC_MODE_LITTLE_ENDIAN)
# 2. 预留 2MB 虚拟内存给 so（放 **1.43MB/1435156B** 的军棋 so + 栈 + 余量）
uc.mem_map(0x40000000, 0x200000)
# 3. 把 so 文件内容写到 0x40000000（unidbg 会自动做，这里手动演示）
#    真实用 unidbg 时这步被 loadLibrary 内部封装，你不用手写
# 4. 设置程序计数器（PC）到 OnCreate 入口
pc = 0x40000000 + 0x1000 + 0x233e1            # 军棋 OnCreate 入口（Thumb 奇地址）
uc.reg_write(UC_ARM_REG_PC, pc)
# 5. 从入口跑到 .got 区域停下（end 设为 .got 起点）
uc.start(pc, 0x40000000 + 0x155000)
```

> 注意：实际 unidbg 里你不用手写这段——`loadLibrary` 内部就做了 mem_map + 设置 PC +
> 跑 init_array。这里拆开看是为了理解"模拟执行"到底在干什么。
> 0x233e1 是**奇地址**（Thumb 模式 bit0=1），真实调用前要 `&~1` 对齐到偶指令（红线 §R6）。

## 5.2 初探 Android 源码（native 侧的 Linker 三阶段）


> **本节核心**：
> 1. Android Linker 三阶段：加载（mmap）/ 链接（3 类重定位）/ 初始化（init_array）
> 2. 军棋 12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT（真机 vs 模拟逐项打勾）
>
> **本节难点**：
> 1. Bionic Linker 是 Android 真机加载 so 的实现，unidbg 模拟简化版
> 2. 三阶段全通 = 加载全通；但「能加载 ≠ 能调用」（§124 还卡）

![Bionic Linker 三阶段对照](assets/ch05_linker_3phase.png)



unidbg 的 Linker 是**对 `Android Bionic linker`（Android 系统自带的动态链接器）的简化
复刻**，不跑真 `dynelf`（Bionic 旧版链接器）。真实 Bionic 的三大阶段，类比"搬新家"：

| 阶段 | 大白话 | 军棋对应 |
|---|---|---|
| 1. so 加载 = 搬进房间 | mmap PT_LOAD 段 + 对齐到基址（把 so 内容放到固定坐标） | base=0x40000000 |
| 2. so 链接 = 接电话线 | 遍历 `.dynamic` 解析依赖 + 重定位（填 RELATIVE / PLT / GLOB_DAT 号） | 5312+214+6 |
| 3. so 初始化 = 通水电 | 跑 `.preinit_array` → `.init` → `.init_array`（初始化函数） | 12 个 |

**为什么拆成三段？** 加载（搬进来）必须先行，链接（接电话线）依赖加载定的基址，
初始化（通水电）要在链接填好号之后跑——三段有严格先后，漏任何一段 so 都跑不起来。
军棋 so 的 12 个 `.init_array` + 5312 个 `RELATIVE` 槽全重定位（红线 §R6，已验证可突破）。

**预判追问："为什么不用真 Bionic linker，要自己写简化版？"**
真 Bionic 是 C++ 写的系统级链接器，跑在真 Linux 内核上，依赖真内核的 mmap/mprotect。
unidbg 模拟的是"用户态"（so 代码本身），没有真内核，mmap 是 unidbg Java 端模拟的
（内存全在 Java 堆）。所以 unidbg 写了"简化 Linker"（`AndroidElfLoader`），
复刻 Bionic 的三阶段逻辑（加载→重定位→init），但用 Java 的 mmap mock 替代。
军棋 12/5312/214/6 全通 = 简化 Linker 做对了，但补不全所有运行时语义（§124 三项阻塞）。

## 5.3 军棋 so 的 Linker 行为对照（真机 vs 模拟，逐项打勾）


> **本节核心**：
> 1. 军棋 so 真机 vs unidbg 逐项打勾：12/5312/214/6 全通
>
> **本节难点**：
> 1. 「能加载」是必要不充分：12 init_array 全通 ≠ OnCreate 能调（还卡 §124）
> 2. 真机 dlopen 填 18 常量，unidbg 填不了（跑不到解包点）
> 3. Linker 行为对照是「加载验证」，「调用验证」需 redroid + 真 GPU

| 阶段 | 真 Android Bionic | unidbg 模拟 | 大白话解释 |
|---|---|---|---|
| mmap PT_LOAD | `mmap` + `mprotect` | unicorn `mem_map` + `mem_protect` | 把 so 段搬进内存，设好读写权限 |
| RELATIVE 重定位 | 填 `.got` + `.data.rel.ro` | 同（5312/5312 ✓） | "相对地址"换成"基址+偏移"，相当于"门牌号填上真实楼层" |
| PLT 重定位 | PLT → GOT → 符号 | 214/214 ✓ | 跳板区 214 个号全填好 |
| GLOB_DAT 6 符号 | 从真 ARM libc 解析 | 从 docker 真 ARM libc 解析 ✓ | 6 个全局变量号从真实 C 库解析出地址 |
| `.init_array` 12 个 | dlopen 跑 | 全通 ✓ | 12 个初始化函数一个个执行完 |
| **`.got` 未重定位** | 真实 linker 填好 | **静态 .so 是零**（未重定位） | 你手里的文件电话簿全是空号 |
**为什么 GLOB_DAT 要"从 docker 真 ARM libc 解析"？** 这 6 个符号是 `libstdc++`/`libc`
里的全局变量（如 `stdout`），unidbg 自带的 AndroidResolver 不给它们填真地址，
得从真 ARM libc 二进制里解析出运行时地址再填。军棋 6 个已实证解析成功（红线 §R6）。
**军棋 so 不是壳**（`.rodata` 静态与运行时 dump 逐字节相同，0/118832 不同，无自解密例程）。
unidbg 跑不动的根因是 **`.got` 未重定位 + `libGLESv1_CM` 无真 GPU + OnCreate 对象语义缺失**，
与壳无关（红线 §R6 全部修正）。
## 5.4 Keystone 与 Capstone（军棋工作区已用，和 unidbg 两条线交叉验证）


> **本节核心**：
> 1. Keystone（汇编）+ Capstone（反汇编）军棋工作区已用，跟 unidbg 两条线交叉验证
> 2. Capstone 反汇编 .text → 机械翻译 C（Keystone 验证指令语义）
>
> **本节难点**：
> 1. 军棋 7428 函数全可翻译 + 编译通过（影子运行时 6402 函数）
> 2. Keystone/Capstone 是「指令语义验证」工具，非「反检测绕过」工具

军棋 `rev64` 红线 §R6 用了 `thumb2c_full.py`（capstone 反汇编 → 机械 C 翻译）：
- **capstone**（开源反汇编库）：二进制 → 汇编文本（**反汇编**，"这行机器码写了什么"）。
- **unicorn2**：动态模拟执行，"这些指令跑起来变成什么"。
- **Keystone**（开源**汇编**库，capstone 的逆操作）：汇编文本 → 二进制（**汇编**，"把人话翻回机器码"）。

**Keystone 是汇编器不是反汇编器（高频混淆）**：Capstone 干"1 字节 → 读成指令"（反），Keystone 干"写 1 条指令 → 1 字节"（正）。军棋用 Keystone 的地方是**改指令**：反汇编出一条 → 改操作数/立即数 → Keystone 重新汇编回字节 → 写回内存（"自吐 3 路径"里的 Patch 自吐用）。

两条线交叉验证：静态翻译出的 C 和动态跑出来的结果必须一致，不一致就是翻译 bug。
军棋 78 叶基线"可跑函数一致率"就是用这套（静态 C 翻译 vs unicorn 动态）逐函数回归出的
（MISMATCH=0，见 REVERSE_NOTES §122）。**为什么两条线必须一致？** 一条线是"读代码"、
一条线是"跑代码"，结果对不上说明至少一条线错了——回归就是抓这种错。

**Keystone 在军棋的 3 个坑（"有问题"多指用法，不是库 bug）**：

| 坑 | 说明 | 军棋影响 |
|---|---|---|
| ① 只管编码不管语义 | 喂它 `mov r0,#0x123` 给 4 字节机器码，**不保证在军棋 so 上下文里语义对**（立即数范围/寄存器约束） | 改出的指令"能编码"但"逻辑可能错"，要回归验证 |
| ② 改 .text 字节 = 破坏原版 | 军棋红线 §R6：`.text` 100% 对齐（1174/1174 PASS），**基本不该改指令**；Patch 是最后手段 | 用 Keystone 改 .text 破坏原版指令流 + 要重签名（v1+v2） |
| ③ 影子运行时 64 位化编码位宽错 | 把 32 位 ARM 指令用 Keystone 编成 **aarch64**，但数据区还是 32 位布局 → 对象图错位（ABI 约束，同 1.4.3） | 影子库黑屏根因之一 |

**所以"Keystone 有问题"的判断要分两种**：指"用它改军棋 .text 风险高" → 对（军棋红线，.text 明文已 100% 对齐不需要 patch）；指"Keystone 库本身有 bug" → 基本不成立（成熟库，坑在用法不在库）。**军棋主力是 Capstone（反汇编 7428 函数），Keystone 只是配角（Patch 自吐时重编码）**。
**dynarmic 和 unicorn2 到底差在哪（大白话）**：
两个后端都是"把 ARM 指令翻译成 x86 跑"，差别在"翻译精度"：
- dynarmic：翻译快（性能高），但 Thumb `add rX,pc` 多做了 `&~3` 掩码 → 差 2 字节
- unicorn2：翻译稍慢（QEMU 后端），但 `(addr+4)` 原样存 → 与真实 ARM 一致
军棋选 unicorn2 是"精度优先"（军棋要"跑对"不要"跑快"），差 2 字节在军棋
0xa37fc 这种"存值到 `.data` 再跳转"的场景就成 bug。
**"精度优先 vs 速度优先"（大白话）**：
- 速度优先（dynarmic）：翻译快，但差 2 字节 → 军棋跳转全乱（跑不对）
- 精度优先（unicorn2）：翻译慢，但 1:1 对 → 军棋 0xa37fc 存对值（跑得对）
军棋要"验证指令语义 100% 对齐"，选精度优先（unicorn2）。

**5.4.1 吸收自 4 本书（揉合去重）：Capstone 自定义反汇编（取《二进制分析实战》Ch8，1 本）**

> **本节来源**：取《二进制分析实战》Ch8（Capstone 最专，682 页教材第 8 章"自定义反汇编"）。详见 `ELF_FORMAT_SUPPLEMENT.md` 第七节。

上面 5.4 讲"Keystone/Capstone 交叉验证"（军棋已用），**本节补"Capstone 反汇编的 2 种模式"**（《二进制分析实战》8.2）——军棋 `thumb2c_full.py` 的真源：

| Capstone 模式 | 干什么 | 军棋 `thumb2c_full.py` 对应 |
|---|---|---|
| **线性反汇编**（8.2.2） | 逐条指令顺序解（快，但控制流分支可能解错） | `load_insns`（逐条解 7428 函数） |
| **递归反汇编**（8.2.4） | 跟控制流递归解（准，能处理跳转/函数调用） | `translate_func`（可达集分析，递归跟 bl/blx） |
| **代码混淆反汇编**（8.1.1） | 处理不透明分支（混淆器塞的假分支） | ❌ 军棋非壳，跳过（原书样本有壳时用） |

**军棋红线 §R6 的"JNI 入口是 Thumb 奇地址，translate_func 起点要 bisect 对齐到最近偶指令"= 递归反汇编的控制流处理**（《二进制分析实战》8.2.4 递归反汇编处理"指令起点不是函数边界"的问题）。

**课程通用价值**：自写反汇编工具（`thumb2c_full.py` 类）= Capstone 线性（读指令）+ 递归（跟控制流）+ 混淆反汇编（处理壳 so 的假分支，军棋非壳跳过）。跑壳/混淆 so 时，递归反汇编 + 控制流分析（第 6 章 6.4.2）是核心——军棋明文非壳，线性+递归就够。

**跟 Keystone 的关系**（5.4 已讲）：Capstone 反汇编（二进制→汇编，读）+ Keystone 汇编（汇编→二进制，写）= "读 + 写"一对；军棋 `thumb2c_full.py` 用 Capstone 读（反汇编 7428 函数），Keystone 只在 Patch 自吐时写（重编码）。

## 5.5 逐环节"能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **选后端** | unicorn2 修 PC off-by-2（dynarmic 的 `add rX,pc` 掩码 bug）；32 位 ARM 一律用它 | 绕过 §124 ① 上下文相关 BL 解码 bug（unicorn2 自己的精度问题，加载 so 后 BL 0x7965a 错位） |
| **加载 so** | 12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT 全通（简化 Linker 做对了） | 读 `.got` 真值（静态文件未重定位，全 0；需 redroid dump 18 地址 72B） |
| **跑 init_array** | 12 个构造函数 + 1 个 preinit 全跑完（通水电） | 跑通 `OnCreate` 业务逻辑（卡在 §124 ② C++ 对象图 0x43ed8） |
| **交叉验证** | capstone 静态翻译 + unicorn2 动态模拟，1174/1174 PASS（MISMATCH=0） | 验证"能玩"（指令 100% 对齐 ≠ 能玩，需真机 GPU + 真 C++ 对象图） |
| **调用链工程化（5.4.2）** | 获取（动态/静态/编译时 3 粒度）+ 存根（PLT 真实化 + blx wire vtable + thunk + bx lr 桩）+ 验证（L1 单函数→L4 真机渲染 4 层）+ 流水线（`jq_reverse_pipeline.sh`） | 让 GL 棋盘区出帧（调用链关键路径缺口=0，但黑屏根因=对象图 glClear 类对象未构造，§124z） |

**军棋实测对照**：
- 选后端 ✓（unicorn2 修 PC off-by-2，DiagTracePC 0xa37fc 存 0x159880）
- 加载 so ✓（12/5312/214/6 全通）
- 跑 init_array ✓（12+1 全跑完）
- 交叉验证 ✓（1174/1174 PASS，MISMATCH=0，§122）
- **✗ 绕过 §124 三项阻塞**：① BL 解码 bug（unicorn2 精度，加载 so 后 BL 0x7965a 错位到
  0x6eaea，真实目标 0x43ed8）② C++ 对象图（std::vector begin/end 未初始化，0x43ed8）
  ③ 288MB PROT_NONE mmap（unidbg mmap2 处理失败）

军棋 14 个 JNI 入口（readelf `--dyn-syms` 实测，unicorn2 下全通）：OnCreate /
OnDestroy / OnResult / OnResume / OnEvent / OnKey / OnUpdate / OnGraphChange /
OnBluetooth / OnPause / OnGraphCreate / OnTimer / OnInput / OnTouch。

## 5.6 军棋实战验证：unicorn2 后端切换的实证


> **本节核心**：
> 1. unicorn2 后端切换实证 PC off-by-2 修复：dynarmic 0xa37fc 存 0x15987c（&~3 错）
> 2. unicorn2 存 0x159880（不 mask 对），与真实 ARM 一致（红线 §R6）
>
> **本节难点**：
> 1. 切 unicorn2 修指令位宽翻译，但 §124 ① BL bug 仍上下文相关
> 2. unicorn2 是军棋唯一能修 PC off-by-2 的后端（dynarmic/unicorn 都不行）

军棋 unidbg 路线的关键突破就是"换 unicorn2 后端"，实测证据：
| 验证项 | dynarmic 结果 | unicorn2 结果 | 大白话 |
|---|---|---|---|
| 0xa37fc 存的值 | `0x159880 & ~3`（错，差 2 字节） | `0x159880`（对） | 门牌号算准了 |
| PC off-by-2 | 有 bug | 消失 | 跳转不再乱跳 |
| 12 init_array | 全通 | 全通 | 初始化没受后端影响 |
| 5312 RELATIVE | 全通 | 全通 | 重定位没受后端影响 |
**结论**：换后端只改"指令翻译精度"，不改"加载/重定位逻辑"。军棋 12/5312/214/6 在两个
后端下都全通，证明卡点不在加载阶段，而在 `OnCreate` 的 C++ 对象图（§124 三项阻塞）。
**为什么军棋"加载全通"是重要里程碑（大白话）**：
军棋 12/5312/214/6 全通 = "加载阶段无 bug"（搬进/接电话/通水电都做了），
但"能加载"≠"能跑通业务"。军棋"能跑通"还差 3 项阻塞（§124 纠正后）：
① 上下文相关 BL 解码 bug（unicorn2 精度）② 数据与指令对齐对应缺口 ③ 288MB
PROT_NONE mmap。unicorn2 切换只解决"指令翻译精度"（① 的前置），
没解决 ②③——②③ 是 unidbg 路线天花板，军棋退而用 redroid dump + arm64 对齐版。
**unicorn2 切换前后对比（大白话）**：
| 阶段 | dynarmic 后端 | unicorn2 后端 |
|---|---|---|
| 加载（12/5312/214/6） | 全通 ✓ | 全通 ✓ |
| 0xa37fc 存的值 | `0x159880 & ~3`（差 2 字节） | `0x159880`（对） |
| `OnCreate` 推进 | 卡 0x43ed8（vector 对象图） | 卡 0x43ed8（同） |
| 结论 | 指令精度错 | 指令精度对，但卡 native 对象图 |
unicorn2 把"指令精度"修了，但"native 对象图"（§124 的 ②）两个后端都卡——
证明卡点不在指令翻译，在数据与指令对齐。
## 5.4.1 调用链追踪：函数对齐的新判据（军棋 §124q 自创方法论，原书无）

> **定位说明**：本节是**军棋项目自创的方法论**（R6 影子运行时对齐 32→64 时发明），**原书《unidbg 逆向工程》没有"调用链作为函数对齐判据"这一层**。原书只在第 9 章（R0dbg 指令追踪排错）+ 第 15 章（Debugger 模块）讲了 trace 工具，但定位是"排错/看执行流"，没升级到"对齐判据"。军棋因为做了 32→64 影子运行时（有"原版 A vs 翻译体 B 是否一致"问题），才发明了这套"调用链判据"。

前面"回归判据"（§122）比的是**单函数的 返回值 + crc32(.data)**，但单函数对不齐"内部业务逻辑结构"。
§124q 引入**调用链（call chain）作为对齐判据**——函数对齐 = **调用链先对齐**（逐层下钻的树，每层对齐则内部逻辑对齐）。

**unidbg 调用链追踪 = debugger 功能**：

| 工具 | 用法 | 军棋现状 |
|---|---|---|
| **动态**（`traceCode()` + `CodeHook`） | 执行原版 so，逐条回调指令，从 `bl/blx` 提取调用目标 = 真实调用链（按实际路径，比静态全量更精确） | ❌ 受 unicorn2 BL 解码 bug 限制（见下） |
| **静态**（capstone 反汇编提取 `bl/blx` 目标序列） | 反汇编 `.text` 提调用目标 = 调用链**指纹**（静态全量，含没走到的分支） | ✅ 成功提取 OnCreate 12 目标 |

**4 条方法论要点**（§124q 记录为规范）：
1. **调用链 = 函数内部业务逻辑的结构指纹**：bl 目标序列对，函数内部逻辑结构才对齐
2. **函数对齐 = 调用链先对齐**：A（原版）调用链 vs B（翻译体）调用链逐层比
3. **返回对齐验证**：不只比最终返回值，**每个 bl 子调用的返回值 A/B 逐点比较**
4. **so 内部 vs Java 交互边界**：调用链里 bl 目标是 **JNI 函数**（FindClass/NewObject，地址在 JNIEnv 函数表）→ 需 Java 交互；bl 目标是 **so 内部地址**（.text 内）→ 纯 native 内部逻辑。**这条能定函数边界**——哪些函数纯 native 可对齐、哪些要 Java 层补齐

**军棋 OnCreate 实测（§124q 补充）**：

```
OnCreate (0x23560-0x23600, 12 调用)
├── 0xbdacc → {0xb677c, 0xbda20}         (JNI wrapper，2 子调用)
├── 0x234f8 → {0x6e9cc, 0x234d8, 0x22bfc} (std::vector，3 子调用)
├── 0x22bfc (PLT _ZdlPv)                 ★ 已真实化（malloc/free）
├── 0xba12c → {0xc5d54, 0x22bfc×5, 0xba034, 0xb7500, 0xba080}  (析构，9 调用)
├── 0xb9edc → {0x22a58, 0xb9e44, ...}    (分配，10 调用)
├── 0xbe47c → {0xbdf64, 0x77820}         (vtable 虚调用，2 子调用)
├── 0x232f8 (PLT _Znwj)                  ★ 已真实化（malloc/free）
├── 0x24250 → {0xb4dd0, 0x241f4, 0xb6750} (GLView C++ 构造，3 子调用)
└── (else 路径重复 0xbdacc/0x234f8/0x22bfc/0x4a12c)
```

**调用链显示的 Java 交互边界（方法论点 4 验证）**：
- **OnCreate 直接调用链全部 so 内部 + PLT，无 JNI 函数**（FindClass/NewObject 不在直接调用）→ OnCreate **直接逻辑是纯 native**，Java 交互在**更深层**
- **0xbdacc = JNI wrapper**（调 0xb677c 虚函数 `[r0,#0xc0]` 取 JNIEnv 表项）→ Java 交互入口
- **0x24250 = GLView C++ 构造**（纯 this 初始化，非 Java 创建）
- **§124p 的 OnCreate JNI 语义重写（直接 NewObject GLView）偏离原版调用链**——原版走 0xbdacc/0x24250 内部函数，非直接 JNI。功能可行（MuMu 验证）但**调用链不对齐**

**动态 trace 受 BL bug 限制，改用静态调用图**（§124q 实测）：
- 动态 trace（DiagTraceChain 用 unicorn2 CodeHook）执行 OnCreate **崩在 PC=0x13c576**（Invalid instruction）——**unicorn2 的 Thumb BL 解码 bug（§124c 已知）**：`0x23578 bl 0xbdacc` 被 unicorn2 解码成目标 `0x13c576`（错位）→ 崩
- **动态调用链在 unicorn2 下受 BL 解码 bug 限制**（patch bl→存根能绕过但看不到真实调用）
- **改用静态调用图（capstone 反汇编提 bl/blx 目标序列）= 不受模拟器 BL bug 影响** → 成功提取（上面的 12 目标）

**所以调用链追踪是"回归判据"的升级**：
- 旧判据（§122）：单函数 返回值 + crc32(.data) 一致率 100%（1174/1174）
- 新判据（§124q）：**调用链逐层对齐**（bl 目标序列 + 每个 bl 返回值 A/B 逐点比）+ **Java 交互边界识别**（bl 目标是 JNI 还是 so 内部）
- 动态 trace 是"精确"的（按实际路径），但**受 unicorn2 BL bug 限制**；静态调用图是"全量"的（含没走到的分支），但**不受 BL bug 影响**——军棋现状用静态调用图兜

**对 §124o 的修正**：§124o 静态反汇编 6 个内部函数（0xbdacc/0x234f8/...）判断"递归逆向指数级"是**纸上谈兵**（没真追调用链）。§124q 的调用链追踪把"哪些函数需 Java 交互、哪些纯 native"从猜测变成**有调用链指纹支撑**——但动态追踪仍受 BL bug 限制，静态调用图是当前可用判据。

## 5.4.2 调用链工程化：获取 → 存根 → 验证 → 流水线（§124u-z 沉淀，干活会话可复用）

5.4.1 讲"调用链是什么判据"，本节讲**"怎么把调用链变成一条可重复跑的工程流水线"**——
军棋 §124u-z 把"翻译体调用链"从手工 debug 升级成**机械化流水线 + 自动统计验证**，沉淀 5 个可复用脚本：

### ① 获取调用链：3 种粒度（动态/静态/编译时）

| 粒度 | 工具 | 军棋现状 | 适用 |
|---|---|---|---|
| **动态（运行时）** | unidbg `traceCode` + CodeHook（从 `bl/blx` 提取） | ❌ 受 unicorn2 BL bug 限制（0x23578→0x13c576 崩） | BL bug 修好后用（按实际路径，最精确） |
| **静态（反汇编）** | capstone/objdump 提 `bl/blx` 目标序列 | ✅ `callchain_stats.py`（objdump -d + bisect 归属调用者） | 现在就能用（全量含没走到的分支） |
| **编译时（生成器）** | `gen_render_full.py` 把渲染链函数完整翻译 + `wire_blx_dispatch.py` 把 `blx rX` 存根 wire 到 vtable 调度器 | ✅ 渲染链 44 处 blx wire + `jqt_vtbl.c` dispatch 表 | 翻译体要"真调子函数"而非"全存根"时 |

**调用链边（call edge）= (调用者函数, bl/blx 目标)**。`callchain_stats.py` 的做法：
1. 读 `jqt_*.c` 里**实际编译源定义**（`full`=命名参数 `_r0`，`stub`=匿名参数 `(uint32_t,uint32_t,...)` 存根，**参数命名是区分关键**）
2. 函数边界 `bnd = sorted(set(seed_v2) | full | stub)`（**必须合并，seed 单独漏 427 个定义**）
3. `objdump -d` 反汇编原版 .so，每条 `bl/blx` 用 `bisect` 归属到最近的 `bnd` 调用者
4. 每条边分类目标 = `full`（已完整翻译）/ `stub`（存根）/ `miss`（缺失）

### ② 存根（stub）逻辑：为什么全存根 + 怎么真实化

**全存根**（`bl/blx 立即数 → r0=0`）是回归的**输入不变量**——C 端和 unicorn 端双向对齐
（否则深递归 STL 链 C/unicorn 不对称 → 假 MISMATCH，红线 §R6）。但**翻译体要真调子函数时**，
存根不够，要**选择性真实化**：

| 真实化类型 | 工具/机制 | 军棋做法 |
|---|---|---|
| **PLT 真实化**（C 库/GL 函数） | `PLT_TMPL`（gen_vtbl.py 70+ 模板） | `memcpy/clock_gettime/glClear/_Znwj/_ZdlPv...` 真实调；**指针参数用 `JQT_ABS_PTR` 转绝对**（偏移→shadow+off，绝对保持） |
| **vtable 间接调用**（`blx rX`） | `wire_blx_dispatch.py` → `jqt_blx_dispatch(reg, r0..sp)` | `blx r3` 存根改成查 `jqt_vtbl.c` 的 dispatch 表（shadow 偏移 → F_xxx），vtable 方法真执行 |
| **ARM this-adjusting thunk** | `gen_arm_thunks.py` → 264 个 `r0-=imm; tail-call F_target` | vtable 偶数槽是 ARM 模式 thunk（非 Thumb 函数），**别当完整函数翻**（递归栈溢出） |
| **`bx lr` 返回桩** | 原版字节 `70 47` 强制 `JQ_RET(r0)` | 数据池失步处 `bx lr` 被误翻成完整函数 → 无限递归 → 栈崩；强制返回（红线 §108/§109） |

**真实化顺序是工程关键**（§124z 教训）：`gen_render_full.py`（生成）→ `wire_blx_dispatch.py`（wire 44 处）→ 插桩。
**渲染链重新生成会覆盖 wire**——顺序颠倒则 blx 还是 `r0=0` 存根，vtable 方法不执行。

### ③ 验证：4 层判据（单函数 → 调用链 → 边统计 → 真机渲染）

| 层 | 判据 | 军棋工具 | 当前结果 |
|---|---|---|---|
| **L1 单函数指令语义** | 返回值 + crc32(结构体+.data)，5 构造输入一致率 | `batch_test.py` + `regression_state.json` | 1174/1174 PASS，MISMATCH=0 |
| **L2 调用链逐层对齐** | 每个 bl 子调用返回值 A/B 逐点比 + Java 交互边界识别 | §124q（5.4.1） | OnCreate 12 目标全 so 内部+PLT，无直接 JNI |
| **L3 调用链边统计** | 已翻译函数内 bl/blx 边：目标 full/stub/miss 占比 + **关键路径直接调用缺口** | `callchain_stats.py` | 3698 边：full 97.5% / stub 2.4% / miss 0.2%；**关键路径缺口=0** |
| **L4 真机渲染** | MuMu 跑翻译体，看主菜单/棋盘是否出帧 | MuMu + 截图 | 主菜单 UI 层渲染（§124u），GL 棋盘区未出帧（§124z 对象图边界） |

**L3 边统计是"调用链工程化"的核心验证**（`callchain_stats.py` 输出）：
```
完整翻译 1334 + 存根 296 = 1630 定义
已翻译函数内 3698 条 bl/blx 边：目标完整 3604（97.5%）/ 存根 87（2.4%）/ 缺失 7（0.2%）
渲染入口链 BFS 闭包 969：完整 767 / 存根 55 / 缺失 147 / 存根边 59
关键路径直接调用缺口：0  ← ★ 这条=黑屏根因不是"缺函数"，是"对象图"（glClear 类对象未构造）
```
**"关键路径缺口=0"是决定性证据**：如果缺口>0，黑屏归因"函数没翻译全"；缺口=0 → 黑屏归因**对象图**
（纹理类 vtable 0x1595c0 的 glClear 方法所在对象未构造，§124z；★ §124zz-3 纠正：旧"vtable=0x1595f4"是幻影，实为 0x1595c0 的 slot[13]）。**调用链统计把"黑屏"从"缺指令"排除，锁定"缺对象图"——这就是工程化验证的价值**。

### ④ 流水线化：一条命令可重复（`jq_reverse_pipeline.sh`）

```bash
rev64/jq_reverse_pipeline.sh all        # 翻译 → 回归 → 对齐库 → APK（全量）
rev64/jq_reverse_pipeline.sh translate  # 仅生成 14 JNI 入口翻译体
rev64/jq_reverse_pipeline.sh regress    # 全量函数级回归（14 块 subprocess 并发，非 fork Pool——红线 §R6 死锁坑）
rev64/jq_reverse_pipeline.sh freeze     # 全量 .out → 增量基线（冻结已 PASS/边界函数）
rev64/jq_reverse_pipeline.sh incremental run  # 增量回归（只攻 MISMATCH + 未测，git 式追踪）
```
**流水线 4 件套**（翻译体渲染链，区别于上面的"对齐版"流水线）：
```
gen_render_full.py → wire_blx_dispatch.py（44 处 blx wire）→ gen_vtbl.py（dispatch 表 + PLT 真实化 + thunk + bx lr 桩）→ build_arm32_aligned.sh（--hash-style=sysv）→ MuMu 执行
```
**工程化思想 3 条**（干活会话可复用）：
1. **合并边界 + 参数命名区分存根**：`bnd=seed∪full∪stub`；`full` 用命名参数 `_r0`，`stub` 用匿名 `(uint32_t,...)`——**别用 `return 0` 匹配**（会漏）
2. **subprocess 分块并发，不用 fork Pool**：深递归 STL 函数拖死 `pool.map`（红线 §R6）；C 端 `subprocess.run(exe)` 必加 8s 超时（长循环 hang）
3. **生成顺序固化**：生成 → wire → 插桩（渲染链重新生成会覆盖 wire，顺序颠倒则 vtable 方法不执行）

### ⑤ 跟 §124q 的关系 + 诚实边界

- 5.4.1（§124q）= **"调用链是什么判据"**（理论）；本节（§124u-z）= **"调用链怎么做成流水线"**（工程）
- **动态调用链**（L2 逐点比）在 unidbg 下被 BL bug 卡死，**真机替代路 = Frida Jnitrace**（见 `frida-dump/FRIDA_NOTES.md` 第三节 + ch13 13.4b）
- **诚实边界**：L3 "关键路径缺口=0" ≠ "能玩"——黑屏根因是**对象图**（glClear 类对象未构造），不是缺函数；
  调用链对齐 + 对象图对齐才"能玩"。指令 100% 对齐 ≠ 能玩（红线 §R6）

## 5.7 本章小结（一句话版）

> 军棋用 **unicorn2** 后端（dynarmic 有 PC off-by-2 bug，已实证修复：0xa37fc 存 0x159880）。
> unidbg 的 Linker 是简化版 Bionic：mmap → 重定位 → init_array 三阶段，军棋 12/5312/214/6 全通。
> 军棋 so 不是壳；`OnCreate` 推进到 `std::vector` 对象图 0x43ed8（DiagJni 实证）。
> §124 纠正后，剩余卡点是 3 项阻塞：① 上下文相关 BL 解码 bug（unicorn2 精度，
> 加载 .so 后 BL 0x7965a 错位到 0x6eaea）② 数据与指令对齐对应缺口 ③ 288MB PROT_NONE mmap。
> **调用链工程化（5.4.2，§124u-z）**：获取（动态 traceCode/静态 objdump/编译时 gen_render_full）
> → 存根（PLT 真实化 JQT_ABS_PTR + blx wire vtable 调度器 + ARM thunk + bx lr 桩）
> → 验证（L1 单函数回归 1174 PASS / L2 调用链逐点 / L3 边统计关键路径缺口=0 / L4 MuMu 渲染）
> → 流水线（jq_reverse_pipeline.sh + 渲染链 4 件套生成顺序固化）。
> **路线纠偏**：32→64 翻译体黑屏（对象图错位），**32→32 翻译体渲染主菜单 UI 层**（ch01 1.4.3★，活路）。

---

## 视频
