# 第10章 unidbg 源码解析：AndroidEmulator

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so + unidbg/unicorn2

## 痛点：为什么一上来就写 `createDalvikVM` 会失败？

照书写 `createDalvikVM` 一跑就崩，但崩的原因藏在"模拟器由哪些模块拼成"里。
军棋实测踩过 3 个坑。

### 痛点 1：不知道"模拟器"由哪些模块拼成

`AndroidEmulator` 不是单一对象，而是 4 个模块的组合（Backend CPU + FileSystem +
SvcMemory + DalvikVM）。缺一个模块，某类调用就崩——你不知道缺的是哪个。

- **大白话卡点**：你以为 `AndroidEmulator` 是个"整体"，其实是 4 个"房间"拼起来的。
  缺哪个房间，住进对应"功能"就崩。
- **为什么卡**：4 个模块各司其职（CPU/文件/内存/Java），缺一个对应功能就崩。
- **不解决会怎样**：崩了不知道补哪个模块，反复试错。

### 痛点 2：so 要读文件，但你 PC 上没有那些文件

军棋 so 读 `data/config/ui.xml`、`data/480/xxx.png` 等资源路径（红线 §R6 已 dump
字符串清单）。模拟环境里这些路径是"虚的"，不挂载宿主目录，`open` 就失败。

- **大白话卡点**：so 要读"它家冰箱里的食材"，你 PC 上没这个冰箱（路径不存在）。
- **为什么卡**：模拟环境的 `/data/data/com.cnvcs.junqi/` 是虚的，宿主没这个目录。
- **不解决会怎样**：so `open` 文件失败 → 配置读不到 → 崩。

### 痛点 3：CPU 后端选错，结果就差 2 字节

dynarmic 有 PC off-by-2 bug（军棋实证），选错后端后面全乱。必须明确用 unicorn2。

- **大白话卡点**：CPU 后端是"地基"，地基选错（dynarmic），上面全歪（差 2 字节）。
- **为什么卡**：dynarmic 对 Thumb `add rX,pc` 多做 `&~3` 掩码。
- **不解决会怎样**：0xa37fc 存错值，后续跳转全乱。

### 痛点 4：4 个模块配齐了"能加载"，但"能调用"还差 §124 三项阻塞

军棋 4 模块全配齐后 12/5312/214/6 全通（能加载），但 `OnCreate` 调用仍卡在
§124 的 ① 上下文相关 BL 解码 bug ② C++ 对象图语义缺失 ③ 288MB PROT_NONE mmap。
"能加载"和"能跑通业务"是两回事，4 模块配齐 ≠ 能玩。

- **大白话卡点**：4 个房间（CPU/文件/内存/Java）全配齐了，但"游戏能跑"还差
  "C++ 对象图"这个第 5 个房间（unidbg 补不了，§124 ②）。
- **为什么卡**：§124 三项阻塞在 native 层（C++ 对象图 / BL 解码 / mmap），不在 4 模块。
- **不解决会怎样**：`OnCreate` 推进到 0x43ed8 卡住，"能加载"≠"能玩"。

## 为什么这样做：把模拟器拆成 4 个"房间模块"

`AndroidEmulator` 类比"一套完整的 ARM 公寓"，4 个模块各管一房：

| 模块 | 类比 | 大白话 | 军棋作用 |
|---|---|---|---|
| **Backend** | 房间的地砖（CPU） | 把 ARM 指令翻译成 x86 跑 | unicorn2 修 PC off-by-2 |
| **FileSystem** | 房间的储物柜（文件） | 把宿主目录挂到虚拟路径 | 挂 `/data/data/com.cnvcs.junqi`，so 读 config 能找到 |
| **SvcMemory** | 房间的家具（内存/栈/堆） | 映射 so 段、分配栈、模拟 malloc | mmap so 段、0x40000 栈、C++ 对象图堆 |
| **DalvikVM** | 房间的管家（Java 侧） | 模拟 FindClass/GetMethodID，应答 so 调 Java | 模拟 7 个 Java 类 + 14 JNI 入口 |

**军棋实际配置**（红线 §R6）：unicorn2 后端（修 PC off-by-2）+ AndroidResolver(23)
（自带 5 个系统库）+ 挂桩 `libGLESv1_CM`（GL 无真 GPU）+ DalvikVM 模拟 7 个 Java 类。

**为什么拆 4 模块而不是一体化？** 每个模块可独立替换/增强（换后端、换文件挂载、
补 JNIEnv），军棋实测就是"4 模块全配齐"才把 12/5312/214/6 全通的。缺任何一个模块，
对应的"一类调用"就崩（缺 DalvikVM → JNI 崩；缺 FileSystem → 读文件崩）。

**预判追问："为什么不用真 Android 系统跑，要自己拼 4 模块？"**
真 Android 系统是"全系统模拟"（内核 + 图形栈 + 进程管理），太重（QEMU 全系统起机
要几分钟）。unidbg 模拟的是"用户态"（so 代码本身 + 它需要的最小环境），4 模块就是
"最小环境"：CPU（翻指令）+ 文件（读 config）+ 内存（摆段/栈/堆）+ Java（接 JNI 电话）。
缺"内核/图形栈/进程管理"是因为军棋 so 不需要（它在 unidbg 里验证的是"逻辑"，
不是"渲染"——渲染要真机 GPU）。

## 10.1 创建 AndroidEmulator（军棋 32 位 so 的实际配置）


> **本节核心**：
> 1. 军棋 32 位 so 实际配置：for32Bit() + unicorn2 + AndroidResolver(23) + 挂桩 GL
> 2. AndroidEmulator = 4 模块拼成（FileSystem + Backend + SvcMemory + Hook/Patch）
>
> **本节难点**：
> 1. 「能加载」必要不充分：4 模块配齐跑通 12/5312/214/6，OnCreate 卡 §124
> 2. 4 模块拼成是「模拟器」，离「能调」还差 18 常量 + DalvikVM

![AndroidEmulator 4 模块](assets/ch10_emulator_4module.png)



```java
// 军棋 32 位 so 的实际配置：unicorn2 后端（不是 dynarmic，修 PC off-by-2 bug）
AndroidEmulator emulator = AndroidEmulatorBuilder.for32Bit()
        .backend(Backend.UNICORN2).build();
// build() 内部一次组装 4 个模块（建公寓时 4 个房间一起配齐）：
// 1. 创建 Backend（unicorn2）—— 地砖（CPU，修 PC off-by-2）
// 2. 创建 FileSystem —— 储物柜（模拟 /data、/system 等目录）
// 3. 创建 SvcMemory —— 家具（承载 ARM 虚拟内存 + 栈 + 堆）
// 4. 创建 DalvikVM —— 管家（模拟 Java 侧，军棋需要）
```

**for32Bit() 的 32 位是什么意思？** 军棋 so 是 32 位 ARM（EABI5），要用 32 位模式
模拟。64 位 so（arm64）用 for64Bit()，寄存器/指针都是 64 位。军棋 32 位 so 全程走
for32Bit 路径，别混成 64 位。

**"4 模块配齐"为什么缺一不可（大白话）**：
军棋 4 模块（Backend/FileSystem/SvcMemory/DalvikVM）缺一个就崩一类：
- 缺 Backend（CPU）：ARM 指令翻译不了，一行都跑不了
- 缺 FileSystem（文件）：so 读 7 个 config XML 崩
- 缺 SvcMemory（内存）：段搬不进去、重定位填不了号
- 缺 DalvikVM（Java）：14 个 JNI 入口接不通
军棋"能加载全通"（12/5312/214/6）= 4 模块全配齐的证明。缺任何一个模块，
对应的"一类调用"就崩（第 10 章"缺一个模块崩一类"）。

**"4 模块"对应军棋"4 类需求"（大白话）**：
- 跑指令（CPU）→ Backend(unicorn2)
- 读文件（config/存档）→ FileSystem
- 搬段填号（mmap/重定位）→ SvcMemory
- 调 Java（14 入口/查类）→ DalvikVM
军棋 4 类需求 4 模块各管一块，全配齐才"能加载 + 能接通 Java"。

## 10.2 创建 FileSystem（储物柜：so 读哪些文件）


> **本节核心**：
> 1. FileSystem = 储物柜：so 读哪些文件（军棋 7 config XML + one.sjg）
>
> **本节难点**：
> 1. 缺 → 读不到配置崩（open 返 -1）
> 2. 军棋 I/O 路径清单（红线 §R6 已 dump）：data/config/{ui,font,image,sound,string,path,uitag}.xml
> 3. FileSystem 映射虚拟路径到宿主（./junqi_data/），第 16 章详

军棋 so 读 `data/config/*.xml`（红线 §R6 已 dump 字符串清单）：
```java
// FileSystem 把宿主目录挂到虚拟路径（在储物柜里贴上你 PC 的真实文件）
FileSystem fs = emulator.getFileSystem();
fs.link(new File("/data/data/com.cnvcs.junqi"), new File("./junqi_data"));
// 军棋 so 读 /data/data/com.cnvcs.junqi/data/config/ui.xml
//   → 实际读 ./junqi_data/data/config/ui.xml（你 PC 上的真文件）
```
军棋 32 位 so 读路径（7 个配置 XML + 6 个分辨率目录）：
- `data/config/{ui,font,image,sound,string,path,uitag}.xml`
- `data/{320,480,480x,720,720s,800}/`（按屏幕分辨率选资源）
不挂载 → so `open` 失败 → 崩。**为什么是 link 不是 copy？** link 是"虚拟路径指向宿主
真实目录"，so 读写的是你 PC 上准备的真文件（比如你放好的 7 个 config XML），
改存档 `one.sjg` 也直接落在宿主（第 16 章展开）。
## 10.3 创建 Backend（地砖：为什么用 unicorn2）


> **本节核心**：
> 1. Backend = 地砖：为什么用 unicorn2（修 PC off-by-2）
> 2. dynarmic 0xa37fc 实证 &~3 mask 错，unicorn2 不 mask 对（红线 §R6）
>
> **本节难点**：
> 1. Backend 选错差 2 字节，军棋切 unicorn2 修
> 2. unicorn2 是军棋唯一能修 PC off-by-2 的后端

```java
Backend backend = emulator.getBackend();
// unicorn2 实现（CPU 模拟器）：
// - Uc(UC_ARCH_ARM, UC_MODE_LITTLE_ENDIAN) —— ARM 架构、小端
// - 内存映射（mem_map/mem_protect）—— 给 so 段定坐标、设权限
// - Hook/Trace（单点 hook + 内存 trace）—— 拦截调用、追踪执行
```
**关键对比**（军棋红线 §R6 实证）：
| 后端 | `add rX,pc` 的 PC 取值 | 军棋结果 |
|---|---|---|
| dynarmic | `(addr+4) & ~3`（对齐到 4 字节） | 差 2 字节 → 0xa37fc 存错值 |
| unicorn2 | `(addr+4)`（不 mask） | 与真实 ARM 一致 → 0xa37fc 存 0x159880 ✓ |
> 这是军棋 unidbg 路线的**关键突破**：把 dynarmic 换成 unicorn2，PC off-by-2 bug 消失。
> 军棋 32 位 ARM 一律用 unicorn2，别省这个事。
## 10.4 创建 SvcMemory（家具：虚拟内存 + 栈 + 堆）


> **本节核心**：
> 1. SvcMemory = 家具：虚拟内存 + 栈 + 堆（军棋 base=0x40000000、栈 0x40000）
>
> **本节难点**：
> 1. 段权限摆错全乱；288MB PROT_NONE 段是 §124 ③ 卡点
> 2. SvcMemory 是「摆家具」，段坐标 + 权限决定重定位基准
> 3. 288MB PROT_NONE mmap 处理失败（unidbg mmap2 mock）

![SvcMemory 内存布局](assets/ch10_svc_memory.png)



SvcMemory 是 unidbg 的内存管理模块，承载 4 样东西（类比"公寓的家具区"）：

| 承载 | 大白话 | 军棋对应 |
|---|---|---|
| **虚拟内存** | 给 so 段定坐标 | mmap PT_LOAD 段（base=0x40000000） |
| **栈空间** | 每个线程的"桌面" | 军棋 32 位 so 默认 0x40000 = 256KB |
| **堆空间** | C++ 对象的"仓库" | `malloc` 模拟，`std::vector` 在这里分配 |
| **模块管理** | 登记"住了哪些 so" | `loadLibrary` 入口（军棋 12 init_array 在这跑） |

军棋 so 的 SvcMemory 关键配置：

```java
// 加载 so 时自动跑 .preinit_array（1 个）+ .init_array（12 个）
dm = vm.loadLibrary(soFile, true);  // true = 跑 init（通水电）
// 分配 0x40000 字节栈（每个线程）
// 预留 .got 段（零值槽，静态文件未重定位——这是军棋的卡点）
```

**为什么堆空间是军棋卡点？** 军棋 `OnCreate` 推进到 `std::vector` 对象图 0x43ed8 时，
C++ 容器（动态数组）在堆上分配对象图。堆没补好（begin/end/capacity 未初始化）就崩——
这正是 §124 的 ② C++ 对象图语义缺失。SvcMemory 的"堆"能分配内存，但"语义"（对象图
怎么填值）要单独补（第 22 章展开）。

## 10.5 逐环节"能干什么 / 不能干什么"

（军棋 14 个 JNI 入口（readelf 实测，DalvikVM 要接的全在这里）：OnCreate / OnDestroy /
OnResult / OnResume / OnEvent / OnKey / OnUpdate / OnGraphChange / OnBluetooth /
OnPause / OnGraphCreate / OnTimer / OnInput / OnTouch）

| 环节（模块） | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **Backend（CPU）** | unicorn2 修 PC off-by-2（`add rX,pc` 不 mask，0xa37fc 存 0x159880）；ARM 指令翻 x86 执行 | 绕过 §124 ① 上下文相关 BL 解码 bug（加载 so 后 BL 0x7965a 错位到 0x6eaea） |
| **FileSystem（文件）** | 宿主目录 link 到虚拟路径（`/data/data/com.cnvcs.junqi` → `./junqi_data`），so 读 7 个 config XML | 真机文件语义（宿主文件 ≠ 真机 `/data`，存档 `one.sjg` 读写行为有差异） |
| **SvcMemory（内存）** | mmap 段（base=0x40000000）+ 栈 0x40000 + 堆模拟 + 预留 `.got` 零值槽 | 补 C++ 对象图语义（std::vector begin/end/capacity 未初始化，§124 ②）；绕过 288MB PROT_NONE mmap（§124 ③） |
| **DalvikVM（Java）** | 模拟 7 个 Java 类 + 14 JNI 入口（resolveClass/callStaticJniMethodObject 全通）；AdView 返 null 去广告 | 跑真 Dalvik 字节码（军棋核心逻辑在 native，Java 侧只接 JNI 电话） |

**军棋实测对照**：
- Backend ✓（unicorn2 修 PC off-by-2）
- FileSystem ✓（7 个 config XML 全读通）
- SvcMemory ✓（段映射 + 栈 + 堆 + `.got` 预留）
- DalvikVM ✓（14 入口接通，AdView 返 null）
- **✗ 绕过 §124 三项阻塞**：① BL 解码 bug（unicorn2 精度）② C++ 对象图（SvcMemory
  堆能分配但语义没补）③ 288MB mmap（SvcMemory mmap2 处理失败）

## 10.6 军棋实战验证：4 模块配齐才跑通 12/5312/214/6


> **本节核心**：
> 1. 4 模块配齐跑通 12/5312/214/6（红线 §R6）
>
> **本节难点**：
> 1. 「能加载」必要不充分，OnCreate 卡 §124
> 2. 4 模块全配齐 = 加载全通，调用还差 18 常量 + DalvikVM + §124

军棋 unidbg 实测（红线 §R6）的"模块 ↔ 功能"对应：
| 军棋功能 | 靠哪个模块 | 缺了会怎样 |
|---|---|---|
| ARM 指令翻译 | Backend(unicorn2) | 换 dynarmic → PC off-by-2，0xa37fc 存错 |
| 读 config XML | FileSystem | 没 link 宿主 → open 失败崩 |
| 12/5312/214/6 重定位 | SvcMemory | mmap 没做 → 段错误 |
| 14 个 JNI 入口接通 | DalvikVM | 没建 VM → FindClass 返回 null 崩 |
军棋 4 模块全配齐后：12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT 全通，
`OnCreate` 接通 Java 侧推进到 0x43ed8。**"缺一个模块崩一类"是本章核心**——
军棋踩过的每个坑都能对应到 4 模块里的某一个。
## 10.7 本章小结（一句话版）

> `AndroidEmulator` = 4 个模块（Backend 地砖 + FileSystem 储物柜 + SvcMemory 家具 +
> DalvikVM 管家），缺一个就崩一类。
> 军棋实际配置：unicorn2 后端（修 PC off-by-2）+ AndroidResolver(23) + 挂桩
> `libGLESv1_CM` + 模拟 7 个 Java 类。
> `.got` 零值槽（未重定位）是 SvcMemory 要预留的军棋专属卡点；堆空间（C++ 对象图）
> 是 §124 的 ② 卡点。

---

## 视频
