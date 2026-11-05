# 第8章 使用 Unicorn 模拟 Linker：so 的链接过程

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的链接阶段

## 痛点：为什么"链接"这一步有 7 种库、3 类重定位要处理？

"链接"是加载和初始化之间的环节，军棋实测在这里有 3 个深坑。

### 痛点 1：依赖库不止 libc

军棋 so 依赖 7 个库（`libc`/`libstdc++`/`libm`/`liblog`/`libdl`/`libGLESv1_CM`/`libGLESv2`），
其中两个 GL 库要真 GPU——PC 上没有，必须挂桩（stub，假的返回 0 的函数）才能过。

- **大白话卡点**：7 家供应商里 2 家（GL）只"活在"真机 GPU 上，你 PC 没 GPU 就没这两家。
- **为什么卡**：`libGLESv1_CM`/`libGLESv2` 是真机图形库，PC 不存在。
- **不解决会怎样**：递归加载 7 个依赖时卡在 GL 库，链接中断。

### 痛点 2：3 类重定位语义不同，漏一类就崩

RELATIVE（相对地址加基址）、JUMP_SLOT（跳板填号）、GLOB_DAT（全局变量填号）——
军棋分别是 5312 / 214 / 6 个，unidbg 要全实现，漏一类后面调用就 SIGSEGV（段错误）。

- **大白话卡点**：3 类"填号"规则不一样（加基址 / 填函数号 / 填变量号），漏一类对应
  的引用就指到 0。
- **为什么卡**：3 类重定位语义不同，unidbg 得逐个实现。
- **不解决会怎样**：漏 JUMP_SLOT → 214 个外部函数跳板全指 0 → 一调用就崩。

### 痛点 3：军棋比书内样本大 1MB，复杂度全翻上来

书内样本是小型 C 库，军棋是 1MB ARM so，依赖 GL 渲染 + C++ 运行时（`std::vector`），
链接阶段要做的事是书内样本的好几倍。

- **大白话卡点**：书里样本是"一室一厅"，军棋是"整栋楼"，配电话线的工作量翻几倍。
- **为什么卡**：1MB so 的 5312/214/6 重定位条目数远超书内样本。
- **不解决会怎样**：照书内样本的简化思路走，军棋这种大 so 的链接阶段处理不全。

### 痛点 4：填完 3 类重定位后还有"18 个运行时地址"静态读不出

军棋 14 个 JNI 入口"可运行"卡在**运行时 18 个地址**（15 `.text` 常量池 + 2 `.data`，
共 72B）。这些值**仅 dlopen 重定位填**（`.so` 未重定位，`.got` 槽为零），
**不是壳自解密**（已证 `.rodata` 静态即明文、无自解密例程）。静态 `.so` 读不出这 18 地址，
拿法唯一：redroid 跑原版 → dlopen 后 dump 这 18 地址 72B 运行时值。

- **大白话卡点**：电话簿填好了 3 类重定位（5312/214/6），但还有 18 个"号"是
  "开机后才发"的（dlopen 重定位才填），静态文件里是占位符，读不出来。
- **为什么卡**：这 18 个不是 3 类重定位能填的（RELATIVE/JUMP_SLOT/GLOB_DAT 都填过了），
  是 `.text` 常量池里的"密文占位"，要 dlopen 时运行时才解析。
- **不解决会怎样**：`OnCreate` 内部 `ldr rX,[pc,#N]` 读不到 18 个值 → 依赖它们的逻辑全错。

## 为什么这样做：把"链接"拆成"查依赖→递归加载→填号"三步

类比给新办公室配电话系统：

1. **查依赖**（读 `.dynamic` 里的 `DT_NEEDED`）：列出"要接哪 7 家供应商"。
2. **递归加载**：7 家供应商各自的依赖再查一遍（`libc` 依赖谁？`libstdc++` 依赖谁？），
   层层展开，直到全部就位。
3. **填号**（3 类重定位）：把内部 5312 处引用 + 外部 214 个函数 + 6 个全局变量的
   "占位符"全换成真实地址。

**为什么"递归"？** 7 家供应商自己也依赖别家（libc 依赖 libdl，libstdc++ 依赖 libc），
不递归展开就漏掉"供应商的供应商"。军棋实测三步全通（红线 §R6），但卡点在"填号"之后——
`.got` 里还有 18 个值（15 个 `.text` 常量池 + 2 个 `.data`）只有真机 dlopen 才填，
静态文件读不出（不是加密，是"linker 重定位才写"的值）。

**预判追问："为什么 unidbg 不直接用真 Bionic linker？"**
真 Bionic 是 C++ 写的系统级链接器，跑在真 Linux 内核上，依赖真内核的 mmap/mprotect。
unidbg 模拟的是"用户态"（so 代码本身），没有真内核，mmap 是 unidbg Java 端模拟的
（内存全在 Java 堆）。所以 unidbg 写了"简化 Linker"（`AndroidElfLoader`），
复刻 Bionic 的链接阶段逻辑（递归加载 + 3 类重定位），但用 Java 的 mmap mock 替代。
军棋 5312/214/6 全通 = 简化 Linker 做对了，但补不全所有运行时语义（§124 三项阻塞）。

**预判追问："递归加载会不会死循环（A 依赖 B、B 依赖 A）？"**
不会。ELF 链接器有"依赖去重"机制（同一个库只加载一次，用哈希/地址判重）。
军棋 7 个依赖里 libc 被 libstdc++/liblog 同时依赖，但只加载一次（地址相同就跳过）。
unidbg 的 `AndroidElfLoader` 也做了去重（`moduleMap` 按 so 路径判重），军棋递归加载
无死循环（实测全通）。

## 8.1 依赖库加载过程（7 件"家具"谁负责配）


> **本节核心**：
> 1. 7 件依赖库递归加载：libc → libm → libdl → libstdc++ → liblog + 2 GL
> 2. 每个依赖自己又有依赖，unidbg 要递归全配齐
>
> **本节难点**：
> 1. 「查依赖 → 递归加载 → 填号」三步
> 2. 依赖库递归是链接核心，漏一个 PLT 跳板找不到目标
> 3. 军棋 7 依赖（含 liblog）全在 unidbg 当虚拟 module 挂载

![军棋 7 依赖库处理（图名沿用 6lib，内容已扩到 7）](assets/ch08_deps_6lib.png)



军棋 so 的 7 个依赖库（readelf `-d` 实测）在 unidbg/AndroidResolver(23) 下的处理：

| 依赖 | 军棋用途 | unidbg 处理 | 大白话解释 |
|---|---|---|---|
| `libc.so` | 标准 C 库 | AndroidResolver 提供 | 自带，配好 |
| `libstdc++.so` | C++ 运行时（std::vector 等） | AndroidResolver 提供 | 自带，配好 |
| `libm.so` | 数学函数 | AndroidResolver 提供 | 自带，配好 |
| `liblog.so` | Android 日志 | AndroidResolver 提供 / mock | 自带，配好 |
| `libdl.so` | dlopen 等 | AndroidResolver 提供 | 自带，配好 |
| `libGLESv1_CM.so` | GL1 渲染 | **需挂桩**（无真 GPU） | PC 没显卡，贴个假屏幕 |
| `libGLESv2.so` | GL2 备用 | 需挂桩 | 同上 |

**军棋 6 个 GLOB_DAT 符号从 docker 真 ARM libc 解析（红线 §R6 已验证）。**
**军棋 7 个依赖库全解析**（libc/libstdc++/libm/liblog/libdl 由 AndroidResolver(23)
提供，libGLESv1_CM/libGLESv2 挂桩）——递归加载时 7 个 `DT_NEEDED` 一个不缺
（含 liblog，32 位 so 的调试日志依赖）。

**为什么 2 个 GL 库"挂桩"而不是"删掉依赖"？** 删 `.so` 的依赖要改二进制（重签名、
破坏原版），不现实。挂桩（stub）只给 GL 函数返回 0，让 so 以为"有 GPU"，
不真执行渲染——军棋 32 位 so 的 GL 渲染本来就不是在 unidbg 里要验证的部分
（验证"能玩"要真机 GPU，见 redroid 限制）。

## 8.2 so 的动态链接（三步填号，军棋实测）


> **本节核心**：
> 1. 3 类重定位填号：5312 RELATIVE + 214 JUMP_SLOT + 6 GLOB_DAT
>
> **本节难点**：
> 1. 漏一类崩：RELATIVE 漏指针全错 / JUMP_SLOT 漏跳板跳 0 / GLOB_DAT 漏 .got 零值
> 2. 「填号」是链接核心，军棋 5312/214/6 实测全通
> 3. 填号后 18 个运行时地址仍缺（dlopen 才填，静态读不出）

```
1. 遍历 .dynamic 段，收集 DT_NEEDED（7 个依赖库）
   // 读电话簿索引：要接 7 家供应商
2. 递归加载 7 个依赖库（AndroidResolver 自带 5 个 + 挂桩 2 个 GL）
   // 把 7 家供应商都接上（供应商的供应商也展开）
3. 处理重定位（3 类填号）：
   - R_ARM_RELATIVE 5312 槽（addr += base）✓
   // 5312 处"文件第 N 字节"→"基址+N 字节"
   - R_ARM_JUMP_SLOT 214 PLT 槽（PLT→GOT→符号）✓
   // 214 个外部函数跳板填号
   - R_ARM_GLOB_DAT 6 GOT 槽（符号解析地址，从 docker 真 ARM libc 解析）✓
   // 6 个全局变量填号
```
**三类重定位为什么要分别处理？** 它们的"填号公式"不同：
- RELATIVE：`值 = 基址 + 文件偏移`（so 内部自引用）
- JUMP_SLOT：`GOT[n] = 外部函数真实地址`（调 libc/C++ 函数）
- GLOB_DAT：`GOT[n] = 全局变量真实地址`（读 libc/C++ 全局变量）
漏一类，对应引用就指 0。军棋 5312+214+6 三类全通（红线 §R6）。
**"递归展开依赖"为什么必要（大白话）**：
7 家供应商（libc/libstdc++/libm/liblog/libdl/GL1/GL2）自己也依赖别家：
- `libc` 依赖 `libdl`（动态加载）
- `libstdc++` 依赖 `libc`（基础 C 函数）
- `liblog` 依赖 `libc`（printf 格式化）
不递归展开，"供应商的供应商"没加载 → 调它们就崩。军棋递归到全部就位
（5 个 AndroidResolver 自带 + 2 个 GL 挂桩），再填 3 类重定位。
**"递归到全部就位"再"填号"（大白话）**：
第 2 步递归加载（把 7 家 + 它们依赖的都接上）→ 第 3 步填号（3 类重定位）。
顺序不能反：先"把所有供应商都接上"（递归加载），再"把号填进电话簿"（填号）。
军棋递归加载全通（5 自带 + 2 挂桩），填号 5312+214+6 全通。
## 8.3 初尝试：unidbg 模拟执行 so（军棋实测路径）


> **本节核心**：
> 1. 军棋 1MB so 比书内样本大，复杂度全翻上来（18 常量 + C++ 对象图）
> 2. 初尝试 = 先跑通加载，再调 OnCreate（卡 §124）
>
> **本节难点**：
> 1. 军棋 so 1MB + 7 依赖 + C++ 对象图，模拟执行路径长
> 2. 「初尝试」到「能调」之间还差 18 常量 + §124

军棋 so 是 1MB ARM so，比书内样本复杂得多。书内样本（小型 C 库）的模拟路径：
```java
// 书内样本：小型 C 库，走 AndroidResolver 就够
AndroidEmulator emulator = AndroidEmulatorBuilder.for32Bit()
        .backend(Backend.UNICORN2).build();
emulator.getMemory().setLibraryResolver(new AndroidResolver(23));
VM vm = emulator.createDalvikVM(new File("simple.apk"));
DalvikModule dm = vm.loadLibrary(new File("libsimple.so"), true);
long ret = dm.getModule().callFunction(emulator, addr, args);  // 调导出函数
```
军棋 so 的对应路径（实际已验证）：
```java
// 军棋：依赖 7 个库，要挂桩 GL + 补 JNIEnv/VM
VM vm = emulator.createDalvikVM(new File("junqi.apk"));
DalvikModule dm = vm.loadLibrary(new File("lib/armeabi/libjunqi.so"), true);
// 12 init_array 全通 ✓
// 5312 RELATIVE + 214 PLT + 6 GLOB_DAT 全通 ✓
// 下一步：补 JNIEnv/VM（DiagJni 已实证）→ 调 14 个 JNI 入口
```
**军棋比书内样本多做了什么？** 多 4 件事：挂桩 2 个 GL 库、解析 6 个 GLOB_DAT、
补 JNIEnv/VM、处理 14 个 JNI 入口。书内样本是"纯 C 库"，军棋是"C++ so + JNI + GL 渲染"，
复杂度高一个量级。
军棋 so 的 14 个 `Java_com_cnvcs_App_On*` 导出符号（`.dynsym` 里唯一的"门牌号"，
readelf `--dyn-syms` 实测）：OnCreate / OnDestroy / OnResult / OnResume / OnEvent /
OnKey / OnUpdate / OnGraphChange / OnBluetooth / OnPause / OnGraphCreate / OnTimer /
OnInput / OnTouch。全在 0x43ed8 处的 std::vector 对象图卡住（§124 ② C++ 对象图语义
缺失，std::vector begin/end/capacity 未初始化）。
## 8.4 探究 unidbg 的 Linker 代码细节（6 段对应 Bionic 简化版）
![unidbg Linker 代码 6 段](assets/ch08_linker_code_flow.png)
### 8.4.1 入口：加载 so 的代码入口


> **本节核心**：
> 1. 加载 so 代码入口：unidbg loadLibrary → jelf 解析 → module 对象
> 2. 军棋 libjunqi.so 1MB 走这个入口
>
> **本节难点**：
> 1. 入口是加载起点，后续 5 段全从这出发

`Memory.loadLibrary(file, init)` → `AndroidElfLoader` 读 ELF 段（类比"搬进办公室"）。
### 8.4.2 载入内存：mmap 两个 PT_LOAD 段


> **本节核心**：
> 1. mmap 两个 PT_LOAD 段（r-x + rw-）
> 2. 军棋 base=0x40000000，段坐标按 PHDR
>
> **本节难点**：
> 1. 载入内存是「按图纸搬」，段权限摆错全乱

- mmap r-x 段（代码+只读）+ rw- 段（可读写），计算基址（默认 `0x40000000`）。
- 军棋 r-x 段 0x001000-0x155000、rw- 段 0x155000-0x15a000（4.2 节图纸）。
### 8.4.3 处理依赖库


> **本节核心**：
> 1. 递归加载 7 依赖（含 liblog）
> 2. 每个依赖递归查自己的 DT_NEEDED
>
> **本节难点**：
> 1. 依赖库递归是链接核心，军棋 7 件家具全配

- 解析 `DT_NEEDED`，递归加载 7 个依赖库（libc / libstdc++ / libm / liblog / libdl /
  libGLESv1_CM / libGLESv2）。
- 前 5 个 AndroidResolver 提供，后 2 个 GL 挂桩（"假屏幕"）。
### 8.4.4 重定位操作（填号）


> **本节核心**：
> 1. 填 3 类重定位（5312/214/6）
> 2. RELATIVE 填 base+offset，JUMP_SLOT 填符号地址，GLOB_DAT 填 .got
>
> **本节难点**：
> 1. 重定位是「接电话」，填号后 so 才能调 libc

- RELATIVE 5312 槽：`addr = base + offset`
- JUMP_SLOT 214 槽：PLT → GOT → 符号
- GLOB_DAT 6 槽：从真 ARM libc 解析
### 8.4.5 初始化信息 + 生成 module 对象


> **本节核心**：
> 1. 生成 module 对象 + 填 init_array 列表
> 2. 军棋 12 init_array 全跑
>
> **本节难点**：
> 1. module 对象是「档案」，记录 base + 依赖 + init_array

- `.preinit_array`（1 个）→ `.init_array`（12 个）全通
- 生成 `module = {base, so, dynamic, ...}`（登记档案）
### 8.4.6 执行初始化（通水电）


> **本节核心**：
> 1. 跑 12 init_array（通水电）
>
> **本节难点**：
> 1. 通水电后 ≠ 能调用（还差 18 常量 + DalvikVM）
> 2. init[9..11] 触发 bx bug（§124h）

- 跑 12 个 init_array + 1 个 preinit_array
- 返回到 `loadLibrary` 调用者
**6 段为什么对应"Bionic 简化版"？** 真 Bionic linker 这 6 段全有，unidbg 把它简化成
6 段流程（加载→mmap→依赖→重定位→module→init），军棋实测 6 段全走通。
## 8.5 逐环节"能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **依赖库加载** | 7 个 `DT_NEEDED` 全解析（5 自带 + 2 GL 挂桩），递归去重无死循环 | GL 真渲染（无真 GPU，挂桩返回 0） |
| **RELATIVE 重定位** | 5312 槽全通（`addr += base`，内部自引用换算） | 读 18 个 `.text` 常量（RELATIVE 只填内部自引用，常量池需 dlopen 重定位） |
| **PLT/GOT 重定位** | 214 PLT + 6 GLOB_DAT 全通（PLT→GOT→符号 + 全局变量填号） | 填 `.got` 真值到静态文件（unidbg 只在内存里填，静态文件全 0） |
| **读 18 常量** | 早期 redroid dump 72B 注入（`jq_load_runtime_values` 适配层）；**★ §124x-z 后 = 静态位移固化 `jqt_mem.c` `g_jq_got_tbl`（免 dump）** | 从静态 `.so` 直接读（未重定位，读不出） |
| **跑通 OnCreate** | Java 侧电话全接通（DiagJni 推进到 0x43ed8） | 突破 0x43ed8（§124 ② C++ 对象图语义缺失，std::vector 未初始化） |

**军棋实测对照**：
- 依赖库加载 ✓（7 个全解析，2 GL 挂桩，递归去重无死循环）
- RELATIVE ✓（5312/5312 全通）
- PLT/GOT ✓（214/214 + 6/6 全通）
- 读 18 常量 ⚠️（静态读不出；早期需 redroid dump 72B 注入，**★ §124x-z 后 = 静态固化 `jqt_mem.c` 免 dump**）
- 跑通 OnCreate ✗（卡在 §124 ② C++ 对象图 0x43ed8，Java 侧全接通后卡点转 native）

## 8.6 军棋实战验证："填号"后的 18 个运行时地址

军棋 14 个 JNI 入口"可运行"卡在**运行时 18 个地址**（15 `.text` 常量池 + 2 `.data`，
共 72B）。这些值**仅 dlopen 重定位填**（`.so` 未重定位，`.got` 槽为零），
**不是壳自解密**（已证 `.rodata` 静态即明文、无自解密例程）。静态 `.so` 读不出这 18 地址，
拿法唯一：redroid 跑原版 → dlopen 后 dump 这 18 地址 72B 运行时值。

工具三件套（红线 §R6）：`frida-dump/redroid_jni_dump.py`（精准读 18 地址）+
`redroid_dump_jni.sh`（一键触发）+ `gen_runtime_table.py`（JSON→C 注入表）→
`jni_aligned.c` 的 `jq_load_runtime_values`。

**为什么这 18 个是链接阶段的"遗留"？** 它们不是 3 类重定位能填的（RELATIVE/JUMP_SLOT/
GLOB_DAT 都填过了），是 `.text` 常量池里的"密文占位"，要 dlopen 时运行时才解析——
属于"链接阶段之后、业务调用之前"的中间状态，unidbg 静态模拟填不出，只能 redroid dump。

## 8.7 本章小结（一句话版）

> 军棋动态链接 3 步：收集 7 依赖 → 递归加载（5 个自带 + 2 个 GL 挂桩）→ 处理 3 类重定位
> （5312+214+6 全通）。
> 6 个 GLOB_DAT 符号从 docker 真 ARM libc 解析（已验证）。
> 军棋 1MB so 比书内样本复杂，但 unidbg/unicorn2 加载全通；卡在"填号"后的 18 个运行时值
> （需 redroid dump，非壳自解密）。

---

## 视频
