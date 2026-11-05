# 第14章 unidbg 源码解析：Memory

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的内存管理

## 痛点：为什么"内存管理"是 unidbg 里最容易崩的一环？

内存管理是 so 跑起来的"地基"，军棋实测在这里踩了 3 个深坑。

### 痛点 1：so 的段要摆到正确坐标，摆错就全乱

军棋 so 的 `.text` 在 0x001000、`.got` 在 0x155000（文件内偏移），加载后要搬到
base=0x40000000 起（运行时坐标）。搬错坐标，所有交叉引用（跳板、电话簿）全指错地方。

- **大白话卡点**：家具要摆对坐标（base+偏移），摆错位置，跳板/电话簿全指错地方。
- **为什么卡**：文件内偏移要加基址才算运行时地址，基址定错全乱。
- **不解决会怎样**：所有交叉引用指错 → 段错误。

### 痛点 2：栈/堆要够，且权限要对

军棋 32 位 so 默认 0x40000（256KB）栈。栈开小了递归一深就溢出；`malloc` 模拟得对，
C++ 对象图（`std::vector`）才建得起来——军棋 `OnCreate` 推进到 0x43ed8 就卡在
§124 的 3 项阻塞：① 上下文相关 BL 解码 bug（unicorn2 精度，加载 .so 后 BL 0x7965a
被解码成 0x6eaea 而非 0x43ed8）② C++ 对象图语义缺失（`std::vector` begin/end/
capacity 未初始化，堆上对象图没补全）③ 288MB PROT_NONE mmap（unidbg mmap2 处理失败）。

- **大白话卡点**：桌面（栈）开小了递归一深就溢出；仓库（堆）里 C++ 对象图没补好，
  一操作就崩。
- **为什么卡**：`std::vector`（C++ 动态数组）对象图语义要补全。
- **不解决会怎样**：OnCreate 推进到 0x43ed8 卡住。

### 痛点 3：`.got` 是零值槽，读它就是段错误

静态 so 的 `.got`（电话簿）没被 linker 填号，全是 0。军棋 `__cxa_atexit`
（C++ 析构注册函数）等走 `.got` 的调用一执行就读 0 指针 → SIGSEGV（段错误）。
Memory 要预留 `.got` 段（零值槽，未重定位状态），这是军棋专属卡点。

- **大白话卡点**：电话簿 214 格全空（=0），`__cxa_atexit` 一读就跳 0 → 崩。
- **为什么卡**：静态文件 `.got` 未重定位，unidbg 要预留这些零值槽。
- **不解决会怎样**：走 `.got` 的调用读 0 指针 → SIGSEGV。

### 痛点 4：288MB 的 PROT_NONE mmap 处理不了

军棋 so 加载时要做一次 288MB 的 `PROT_NONE` mmap（虚拟内存占位，不实际用）。
unidbg 的 mmap2 mock 对这种"大 PROT_NONE 映射"处理失败（§124 ③），导致加载断掉。

- **大白话卡点**：so 要"占 288MB 空地"（PROT_NONE 只是占位，不实际读写），
  你的"虚拟公寓"没这么大空地 → 占位失败 → 崩。
- **为什么卡**：unidbg mmap2 mock 对 288MB 级 PROT_NONE 映射没做对。
- **不解决会怎样**：加载中断，进不了 init_array。

## 为什么这样做：把 Memory 拆成"4 个组件 + 6 步 loader"

类比"一个公寓的管理处"，4 个组件各管一块：

| 组件 | 管什么 | 大白话 | 军棋对应 |
|---|---|---|---|
| SvcMemory | 虚拟内存 + 栈 + 堆（家具摆放） | 摆家具 | base=0x40000000，栈 0x40000 |
| AndroidElfLoader | ELF 段映射（按图纸搬家具） | 按图纸搬 | mmap r-x + rw- 两个 PT_LOAD 段 |
| 符号表解析器 | 找函数入口（门牌号查询） | 查门牌号 | 14 个 `On*` 导出符号 |
| Hook/Patch 框架 | 拦截/改写（改电路） | 改电路 | 拦截 `__cxa_atexit`、`FindClass` |

**6 步 loader 流程**（军棋实测全通，红线 §R6）：读 ELF → mmap 段 → 递归加载
7 依赖 → 3 类重定位（5312 RELATIVE + 214 PLT + 6 GLOB_DAT）→ 跑 12+1 init_array
→ 生成 module。

**为什么 Memory 要拆 4 组件？** 4 组件各管一块（摆家具/按图纸搬/查门牌/改电路），
军棋实测"缺一个组件崩一类"。6 步 loader 把 4 组件串起来，军棋全通。

**预判追问："为什么不用真 Linux mmap，要自己写 Java 端 mock？"**
unidbg 模拟的是"用户态"，内存全在 Java 堆（`byte[]`），不是真 Linux `mmap`。
真内核的 `PROT_NONE`/`PROT_EXEC` 在 Java 堆里是"假的"——这就是 §124 ③
288MB PROT_NONE mmap 处理失败的根因（unidbg mmap2 mock 没做对 288MB 级占位映射）。
Java 端 mock = "模拟"（不真占内核内存），好处是跨平台（x86/arm64 宿主都能跑），
代价是"权限语义不真"（PROT_NONE 只是占位标记，不真锁内存）。

## 14.1 Memory 模块的创建（管理处的 4 个窗口）


> **本节核心**：
> 1. Memory = 管理处 4 窗口：SvcMemory/AndroidElfLoader/符号表解析器/Hook-Patch
>
> **本节难点**：
> 1. 军棋 4 组件全配齐 = 6 步 loader 全通；缺一个对应一步崩
> 2. 4 组件全配齐是「能加载」底座，「能调用」还差 §124

![Memory 模块 4 组件](assets/ch14_memory_modules.png)



```java
Memory memory = emulator.getMemory();
// 内部组装 4 个组件（管理处的 4 个窗口）：
// 1. 创建 SvcMemory（虚拟内存 + 栈 + 堆）—— 摆家具
// 2. 创建 AndroidElfLoader（ELF 段映射）—— 按图纸搬
// 3. 创建符号表解析器（findSymbol）—— 查门牌号
// 4. 创建 Hook/Patch 框架 —— 改电路
```

军棋 32 位 so 的 Memory 关键配置（红线 §R6）：
- `base = 0x40000000`（默认对齐 0x1000，给 so 定固定坐标）
- `.got` 段（零值槽，未重定位——军棋专属卡点）
- 12 init_array（全通）+ 5312 RELATIVE + 214 PLT + 6 GLOB_DAT（全通）

**为什么 base 定 0x40000000？** 这是 unidbg 32 位 ARM 默认加载基址，且**真机 Bionic
linker 32 位 ARM 默认也在这附近**（第 30 章基地址检测会再讲）。基址定错，所有
"基址+偏移"的相对地址全指错位置，重定位一步错步步错。

## 14.2 AndroidElfLoader 的方法实现（按图纸搬 + 查门牌 + 建档案）

### 14.2.1 内存相关方法实现（摆家具）


> **本节核心**：
> 1. 摆家具：段 mmap + 栈堆（军棋 base=0x40000000、栈 0x40000）
>
> **本节难点**：
> 1. 段权限摆错全乱；288MB PROT_NONE 是 §124 ③
> 2. 内存相关是「摆家具」，段坐标决定重定位基准

```java
// 映射 PT_LOAD 段到 base=0x40000000（按图纸把家具摆进对应坐标）
// r-x 段：代码+只读，不许改（军棋 .text+.rodata）
memory.mmap(base + 0x001000, 0x154000, PROT_READ | PROT_EXEC);
// rw- 段：可读写数据（军棋 .data+.bss）
memory.mmap(base + 0x155000, 0x5000, PROT_READ | PROT_WRITE);
// 军棋 .got 段（零值槽，未重定位状态——这是卡点所在，读它就是段错误）
memory.mmap(base + 0x155000, 0x1000, PROT_READ | PROT_WRITE);
```
**为什么 r-x 和 rw- 分开 mmap？** 权限不同（代码段只读可执行、数据段可读写），
分开映射才能让"代码不许改、数据可写"（4.2 节图纸里两个 PT_LOAD 按权限拆的）。
### 14.2.2 栈空间相关方法实现（给每个线程开独立房间）


> **本节核心**：
> 1. 每线程独立房间（unidbg 单线程，栈固定）
>
> **本节难点**：
> 1. 军棋 7 依赖递归加载共用栈
> 2. 栈空间是「线程房间」，unidbg 单线程简化

```java
// 每个线程独立栈（军棋 32 位 so 默认 0x40000 = 256KB 字节）
// 栈开小了递归一深就溢出；C++ 对象图（std::vector）也靠这块内存
memory.allocStack(0x40000);
```
**为什么军棋栈开 0x40000（256KB）？** 军棋 32 位 so 递归较深（C++ 调用链），
256KB 够用。栈开小了（比如 0x10000）递归一深就溢出；开太大（288MB 那种 PROT_NONE）
unidbg 又处理不了（§124 的 ③ 卡点）。
### 14.2.3 用户常用方法解析（查门牌号：找 14 个 JNI 入口）


> **本节核心**：
> 1. 查门牌：找 14 个 On* 入口地址（strip 后 findSymbol 只查 14 个）
>
> **本节难点**：
> 1. 内部函数无名字，需 IDA 找地址
> 2. 查门牌是「符号表解析」，军棋 14 入口全查通

```java
// 军棋 14 个 JNI 入口（Java_com_cnvcs_App_On*，Thumb 奇地址）
// 按符号查入口地址（strip 后 .dynsym 只剩这 14 个导出）
long onCreate = memory.findSymbol(module, "Java_com_cnvcs_App_OnCreate", true).address;
// 关键：对齐到偶指令（红线 §R6）
// Thumb 模式地址是奇数（bit0=1 表示 Thumb），偶指令起点要 &~1
onCreate = onCreate & ~1L;
// 不对齐 → 翻译器可达指令集为空 → 跑出 0KB body（军棋 0x233e1 等 14 个入口全踩过）
```
**14 个入口名（readelf 实测）**：OnCreate / OnDestroy / OnResult / OnResume / OnEvent /
OnKey / OnUpdate / OnGraphChange / OnBluetooth / OnPause / OnGraphCreate / OnTimer /
OnInput / OnTouch。
### 14.2.4 虚拟模块（建档案：登记这家 so）


> **本节核心**：
> 1. 建档案：登记这家 so（module.base + dynamic）
>
> **本节难点**：
> 1. 军棋 module.base=0x40000000 + dynamic=7 依赖
> 2. 虚拟模块是「档案」，记录 so 身份 + 依赖

```java
// 军棋 32 位 so 的虚拟模块对象（档案）
module.base = 0x40000000;
module.so = new File("lib/armeabi/libjunqi.so");
module.dynamic = ...;  // DT_NEEDED 7 个依赖库（libc/libstdc++/libm/liblog/libdl/GL1/GL2）
```
**"为什么 4 组件对应 4 类'家具'"（大白话）**：
- SvcMemory = "摆家具"（虚拟内存 + 栈 + 堆，军棋 base=0x40000000、栈 0x40000）
- AndroidElfLoader = "按图纸搬"（ELF 段映射，r-x + rw- 两段）
- 符号表解析器 = "查门牌"（findSymbol 找 14 个 `On*` 入口地址）
- Hook/Patch 框架 = "改电路"（拦截 `__cxa_atexit`/`FindClass`）
4 组件全配齐 = "管理处 4 个窗口都开了"，军棋 6 步 loader 才能全通。
缺一个组件，对应的"一步"就崩（查不了门牌 / 拦不了调用）。
**"管理处 4 窗口"为什么必须都开（大白话）**：
军棋 so 要"摆家具 + 按图纸搬 + 查门牌 + 改电路"4 样都做到才能跑：
- 摆家具（SvcMemory）：段搬进去、栈/堆分配
- 按图纸搬（AndroidElfLoader）：r-x + rw- 两段 mmap
- 查门牌（符号表解析器）：14 个 `On*` 按名字查地址
- 改电路（Hook/Patch）：拦 `__cxa_atexit`/`FindClass` 喂假值
4 窗口全开 = 军棋 6 步 loader 全通（红线 §R6）。
## 14.2.5 虚拟模块（virtual module，原书 14.2.4 的延伸）

> 注：14.2.4 讲"给军棋 so 建档案（module 对象）"，本节 14.2.5 讲"虚拟模块"
> ——把"不是真 ELF so 的内存块"也挂成 module。原书 14.2.4 专讲 virtual module，
> 课程把它拆成"14.2.4 建档案 + 14.2.5 虚拟模块"两节恢复细度。

原书 14.2.4 专讲 **virtual module（虚拟模块）**——把"不是真 ELF so 的内存块"
也挂成 module，让 unidbg 能像查 so 符号表一样查它。军棋大量用到：

| 虚拟模块 | 军棋对应 | 为什么是"虚拟" |
|---|---|---|
| **7 个依赖库**（libc/libstdc++/libm/liblog/libdl/libGLES×2） | unidbg `AndroidResolver(23)` 自带 | 不是"军棋自己的 so"，是"系统库"，但 unidbg 把它当 module 挂（查符号/重定位） |
| **挂桩的 GL 函数**（`glGetError` 等） | 军棋无真 GPU | 桩函数不是真 so，是"假 module"（返回 0） |
| **DalvikVM 的 JNI 函数表**（232 槽） | 军棋 `getJNIEnv()` | JNI 函数表不是 ELF，但当成 module 查 `FindClass`/`GetMethodID` |

**为什么军棋要挂 7 个虚拟模块**：军棋 `.so` 的 7 个 `DT_NEEDED`（含 liblog）
全在 unidbg 里当虚拟 module 挂载。挂载后，军棋 so 调 `strlen`（libc）/
`__cxa_atexit`（libstdc++）时，unidbg 按虚拟 module 查符号表找地址，
不查就 SIGSEGV（`PLT` 跳板找不到目标）。

**"虚拟模块"跟"真 so"的区别**（大白话）：
- 真 so（`libjunqi.so`）= 你拿到的 1MB ELF 文件，有完整 ELF 头 + `.dynsym`
- 虚拟模块（libc 等）= 没有 ELF 头，只有一张"符号表 + 入口地址"的简表
- unidbg 对两者一视同仁（都挂进 `SvcMemory`），但虚拟模块"查符号"更快
  （没有完整 ELF 段，只有符号表）

## 14.3 加载 so 的 loader 功能（军棋 6 步全链路）


> **本节核心**：
> 1. 军棋 6 步全链路：mmap 两段 → 填 3 类重定位 → 跑 12 init_array → 建 module → 填 .got → 应答 JNI
>
> **本节难点**：
> 1. 6 步全通 = 12/5312/214/6（红线 §R6）
> 2. loader 是「加载全链路」，「能加载 ≠ 能调用」

![军棋 so loader 6 步](assets/ch14_loader_flow.png)



军棋 32 位 so 的 loader 路径（unidbg 实测）：

```
1. loadLibrary(libjunqi.so)
   → AndroidElfLoader.readELF()
   → 解析 e_machine == EM_ARM(40), e_phnum == 9        （看房证）
2. mmap PT_LOAD 段（r-x + rw-）
   → base = 0x40000000                                  （按图纸搬家具）
3. 递归加载 7 依赖库（libc / libstdc++ / libm / liblog / libdl / libGLESv1_CM / libGLESv2）
   → AndroidResolver(23) 提供前 5 个（自带）
   → 挂桩 libGLESv1_CM / libGLESv2（GL 无真 GPU，贴假屏幕）
4. 重定位（3 类填号）
   → R_ARM_RELATIVE 5312 槽 ✓
   → R_ARM_JUMP_SLOT 214 PLT 槽 ✓
   → R_ARM_GLOB_DAT 6 槽 ✓（从 docker 真 ARM libc 解析）
5. 跑 .preinit_array(1) + .init_array(12)
   → 全通 ✓                                              （通水电）
6. 生成 module 对象（登记档案：{base, so, dynamic}）
```

## 14.4 逐环节"能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **mmap 段** | 2 个 PT_LOAD（r-x 0x001000-0x155000 / rw- 0x155000-0x15a000）全映射到 base=0x40000000 | 真 Linux mmap 语义（Java 堆 mock，PROT_NONE/PROT_EXEC 是"假的"，§124 ③） |
| **栈/堆** | 栈 0x40000（256KB）够用；堆模拟 `malloc`（`std::vector` 在这里分配） | 补 C++ 对象图语义（std::vector begin/end/capacity 未初始化，§124 ②） |
| **填 .got** | 重定位时填 5312 RELATIVE + 214 PLT + 6 GLOB_DAT（内存里填，不是写回静态文件） | 从静态 `.got` 读运行时值（静态文件全 0，未重定位；需 redroid dump 18 地址 72B） |
| **查门牌** | `findSymbol` 查 14 个 `On*` 导出符号（`.dynsym` 保留），`&~1` 对齐偶指令 | 查内部函数符号（strip 后 `.symtab` 空，内部函数没符号） |
| **拦截/改电路** | Hook `__cxa_atexit`（喂 0 跳过读零 `.got`）、`FindClass`（喂 null 去广告） | 绕过 §124 ③ 288MB PROT_NONE mmap（unidbg mmap2 处理失败） |

**军棋实测对照**：
- mmap 段 ✓（r-x + rw- 全映射到 base=0x40000000）
- 栈/堆 ✓（栈 0x40000，堆模拟 malloc）
- 填 .got ✓（5312/214/6 全填，内存里填）
- 查门牌 ✓（14 个 `On*` 全查到，`&~1` 对齐）
- 拦截/改电路 ✓（Hook 两大卡口全设好）
- **✗ 绕过 §124 ③ 288MB mmap**：unidbg mmap2 mock 对 288MB 级 PROT_NONE 占位映射
  处理失败（Java 堆"假的"权限语义）

## 14.5 军棋实战验证：Memory 是 12/5312/214/6 全通的底座


> **本节核心**：
> 1. Memory 是 12/5312/214/6 全通底座（.got 零值是第一个崩点）
>
> **本节难点**：
> 1. 「能加载」必要不充分，OnCreate 卡 §124
> 2. Memory 全通 = 加载通，调用还差 18 常量 + DalvikVM + §124

军棋 unidbg Memory 实测（红线 §R6）：
| Memory 组件 | 军棋对应 | 实测 |
|---|---|---|
| SvcMemory（摆家具） | base=0x40000000 + 栈 0x40000 + 堆 | mmap 全通 |
| AndroidElfLoader（按图纸搬） | r-x + rw- 两段 | 段映射全通 |
| 符号表解析器（查门牌） | 14 个 `On*` 导出 | findSymbol 全通 |
| Hook/Patch（改电路） | 拦截 `__cxa_atexit`/`FindClass` | 设卡可用 |
军棋 12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT 全通，全靠 Memory 这 4 组件
配齐。**`.got` 零值槽是军棋专属卡点**——静态文件没重定位，Memory 预留这些槽，
但填真值要 redroid dump（第 8 章 18 个运行时地址）。
## 14.6 本章小结（一句话版）

> unidbg Memory = 4 组件（SvcMemory 摆家具 + AndroidElfLoader 按图纸搬 +
> 符号表查门牌 + Hook/Patch 改电路）。
> 军棋 loader 6 步全通：读 ELF → mmap → 递归 7 依赖 → 3 类重定位（5312/214/6）
> → 12+1 init_array → module。
> 军棋 14 个 JNI 入口是 Thumb 奇地址，要 `&~1` 对齐；`.got` 零值槽（未重定位）是
> 卡点所在，栈 0x40000、C++ 对象图堆（§124 的 ②）是军棋专属关注点。

---

## 视频
