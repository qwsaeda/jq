# 第7章 使用 Unicorn 模拟 Linker：so 的加载过程

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so + unicorn2 后端

## 痛点：为什么模拟环境里"加载"这一步就会崩？

"加载"听起来是最简单的一步（把 so 搬进内存），但军棋实测在这里就栽过 3 个跟头。

### 痛点 1：依赖库 PC 上不全

军棋 so 依赖 7 个库（`libc`、`libstdc++`、`libm`、`libdl`、`liblog`、`libGLESv1_CM`、
`libGLESv2`），其中两个 GL（图形渲染）库需要真 GPU，你的 x86 开发机没有——
加载到这一步就断。

- **大白话卡点**：so 要"接 7 家供应商的电话线"，其中 2 家（GL 库）只在真机 GPU 上存在，
  你 PC 上没这两家 → 线接不上。
- **为什么卡**：`libGLESv1_CM`/`libGLESv2` 是真机才有，PC 没有。
- **不解决会怎样**：加载到解析 7 个依赖就断，so 进不了初始化阶段。

**7 个依赖库分工（大白话）**：

| 库 | 大白话 | 军棋用在哪 |
|---|---|---|
| `libc.so` | 标准 C 库（malloc/strlen/printf） | 内存分配、字符串处理 |
| `libstdc++.so` | C++ 运行时（std::vector 容器） | 棋盘 38 子容器、走子记录 |
| `libm.so` | 数学函数 | 渲染坐标计算 |
| `libdl.so` | 动态加载（dlopen/dlsym） | 军棋 so 自身被 dlopen |
| `liblog.so` | Android 日志（`__android_log_print`） | 调试日志（军棋 32 位 so 有） |
| `libGLESv1_CM.so` | GL1 渲染（真机 GPU 才有） | 棋盘/棋子 GLES1 渲染 |
| `libGLESv2.so` | GL2 渲染（备用） | 备用渲染路径 |

前 5 个 AndroidResolver(23) 自带，后 2 个 GL 挂桩。

### 痛点 2：`.got` 是零值，一跑就段错误

静态 so 文件里的 `.got`（电话簿）没被 linker 填号（全是 0），任何走 PLT 跳板的调用
都会跳到地址 0 → SIGSEGV（段错误）。

- **大白话卡点**：电话簿 214 格全空（=0），拨号就拨到 0 → 崩。
- **为什么卡**：你拿的是静态文件（未重定位），`.got` 槽没被填。
- **不解决会怎样**：任何走 PLT 的调用（malloc/strlen…）都跳 0 → SIGSEGV。

**为什么静态文件 `.got` 是零？** 静态 so 是"编译好但没被 linker 填号"的状态，
真机 dlopen 时 Bionic linker 才把 214 个 PLT 槽 + 6 个 GLOB_DAT 槽填上真地址。
你手里没跑过 linker，`.got` 自然全 0。unidbg 的简化 Linker 要替 Bionic 填这些号，
军棋 5312 RELATIVE + 214 PLT + 6 GLOB_DAT 全通（红线 §R6）。

### 痛点 3：PC 算错 2 字节，程序乱跳（dynarmic 的坑）

dynarmic 后端对 Thumb `add rX,pc` 做了 `&~3` 掩码，与真实 ARM 差 2 字节——
军棋 0xa37fc 存错值，后续跳转全错。

- **大白话卡点**：算门牌号差 2 字节，按门牌号找东西全找错房间。
- **为什么卡**：dynarmic 翻译 Thumb 寄存器形式 PC 时多做了 `&~3` 对齐。
- **不解决会怎样**：0xa37fc 存 `0x159880 & ~3`（错），后续跳转全乱。

**为什么 Thumb 的 PC 要"加 4"？** ARM 是"取指 + 执行"流水线，执行某条指令时
PC 已经指到"下一条指令"。Thumb 16 位指令 2 字节、ARM 32 位指令 4 字节，
`add rX,pc` 算地址时要取"PC+4"（真实 ARM 寄存器形式），dynarmic 多做了 `&~3`
对齐（把 4 字节对齐成偶数），与真实 ARM 差 2 字节。

## 为什么这样做：给模拟环境"配齐家具"再开机

unidbg 模拟 so 加载 = 给 ARM 程序配齐四样东西再开机（类比"搬进毛坯房前先配齐家具"）：

| 家具 | 大白话 | 军棋作用 |
|---|---|---|
| **CPU 模拟器**（unicorn2） | 房间的"电力"（把 ARM 指令翻译成 x86 跑） | 选它是因为修掉了 dynarmic 的 PC off-by-2 bug |
| **系统库解析器**（`AndroidResolver(23)`） | 自带 Android 7 的 5 个库（libc/libstdc++/libm/liblog/libdl） | 相当于"配齐 5 件家具" |
| **GL 挂桩**（stub，假的返回 0 的函数） | "贴个假屏幕蒙混过关" | GL 库没有真 GPU 就挂桩，`glGetError` 返回 0 |
| **虚拟内存**（`mem_map`） | 给 so 段定坐标 | base=0x40000000 |

**关键认知**：配齐这四样只是"能加载"，不等于"能调用"。军棋实测加载全通
（12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT），但 `OnCreate` 调用仍卡在
18 个运行时地址（红线 §R6）。"能加载"和"能跑通业务"是两回事，别混。

**为什么"配齐家具"是"加载"的前提？** 军棋 32 位 so 要跑，得：CPU（unicorn2 翻指令）
+ 系统库（7 个依赖）+ 内存（mmap 段）三者齐备。缺 CPU 翻译不了指令、缺系统库
重定位填不了号、缺内存段搬不进去——"加载"= 三样配齐 + 段搬进去 + 重定位填号。

**预判追问："为什么 GL 库要挂桩，不能直接删掉？"**
删 `.so` 的依赖要改二进制（重签名、破坏原版），不现实。挂桩（stub）只给 GL 函数
返回 0，让 so 以为"有 GPU"，不真执行渲染——军棋 32 位 so 的 GL 渲染本来就不是
在 unidbg 里要验证的部分（验证"能玩"要真机 GPU，见 redroid 限制）。
GL 挂桩 = "贴个假屏幕"，`glGetError` 返回 0（无错误），`glBegin`/`glEnd` 返回 0。
so 以为渲染成功了，实际什么都没画（军棋 unidbg 里 GL 渲染不验证"能画"，只验证"不崩"）。

**预判追问："为什么选 API level 23 而不是军棋的 19？"**
军棋 so 构建于 API 19 时代，但用 23（Android 7）的库兼容性更好，且 unidbg 自带
23 的系统库最完整。选低了（19）库不全（缺 liblog 等），选高了（28+）符号对不上
（libc 版本差异）。军棋选 23 = "兼容性 + 完整性"的平衡点。

## 7.1 模拟 Linker 环境准备（军棋 32 位 so 的最低配置）


> **本节核心**：
> 1. 军棋 32 位 so 最低配置：unicorn2 后端 + AndroidResolver(23) + 挂桩 GL
>
> **本节难点**：
> 1. 缺一个：缺 GL 桩渲染崩 / 缺 AndroidResolver .got 零值崩 / 缺 unicorn2 PC off-by-2
> 2. 「配齐家具再开机」：加载前配好 7 依赖 + 2 挂桩
> 3. AndroidResolver(23) 自带 5 系统库，2 GL 库挂桩（无真 GPU）
> 4. 最低配置是「能加载」前提，「能调用」还差 18 常量 + §124

```java
// 32 位 ARM 用 unicorn2 后端（x86_64 宿主跑 32 位 ARM 指令）
// 为什么不是 dynarmic？dynarmic 对 Thumb `add rX,pc` 用了
// (addr+4)&~3 的 mask，与真实 ARM 差 2 字节 → 军棋 PC off-by-2 bug。
// unicorn2 用 (addr+4) 不 mask，与真实 ARM 一致（DiagTracePC 实证 0xa37fc 存 0x159880）。
AndroidEmulator emulator = AndroidEmulatorBuilder.for32Bit()
        .backend(Backend.UNICORN2).build();
// 自带 Android 7 系统库：libc/libstdc++/libm/liblog/libdl（5 件家具配齐）
// AndroidResolver(23) 里的 23 = API level，对应 Android 7
emulator.getMemory().setLibraryResolver(new AndroidResolver(23));
// 挂 libGLESv1_CM 桩（军棋 GL1 渲染依赖，无真 GPU 时贴个假屏幕）
// 军棋 GL 函数 stub，so 以为有 GPU
emulator.getMemory().symbolResolver().resolve("glGetError").when((h) -> 0L);
```
军棋 so 的 7 个依赖库（红线 §R6）：
```
libc.so / libstdc++.so / libm.so / libdl.so / liblog.so / libGLESv1_CM.so / libGLESv2.so
（前 5 个 AndroidResolver 提供，后 2 个 GL 库挂桩）
```
**为什么 API level 选 23？** 军棋 so 构建于 API 19 时代，但用 23（Android 7）的库
兼容性更好，且 unidbg 自带 23 的系统库最完整。选低了库不全，选高了符号对不上。
## 7.2 模拟 Linker：so 的加载（逐项打勾实测）


> **本节核心**：
> 1. 逐项打勾实测：12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT
> 2. 三大崩点：依赖库 PC 不全 / .got 零值 / dynarmic PC 算错
>
> **本节难点**：
> 1. 加载全通 = 军棋 12/5312/214/6（红线 §R6）
> 2. 「能加载」必要不充分，OnCreate 卡 §124

![DiagTracePC PC off-by-2 实证](assets/ch07_diagtracepc.png)



军棋 so 加载到 unidbg 的完整路径（实测结果）：

| 阶段 | 军棋实测 | 大白话解释 |
|---|---|---|
| 12 个 `.init_array` | 全通 ✓ | 12 个初始化函数一个个跑完 |
| 5312 个 RELATIVE 重定位 | 全通 ✓ | 5312 处"相对地址"全换算成"基址+偏移" |
| 214 个 PLT→GOT→符号映射 | 全通 ✓ | 214 个跳板全接到电话簿，电话簿填好号 |
| 6 个 GLOB_DAT 符号 | 从 docker 真 ARM libc 解析 ✓ | 6 个全局变量号从真 C 库解析出地址 |

**为什么 6 个 GLOB_DAT 要"从 docker 真 ARM libc 解析"？** 这 6 个是 `libstdc++`/`libc`
的全局变量地址（非函数），unidbg AndroidResolver 不直接给真地址，得从真 ARM libc
二进制里读出运行时值再填。军棋 6 个已实证解析成功（红线 §R6）。

**4 个数为什么是"军棋指纹"？** 12/5312/214/6 是军棋 **1.43MB/1435156B** so 的固定特征
（重定位表条目数），后续任何"加载对没对"的核对都用这 4 个数当基准。
全通 = 加载阶段无 bug。

## 7.2b 吸收自 4 本书（揉合去重）：libbfd 自写 so 加载器

> **本节来源**：取《二进制分析实战》Ch4（libbfd 自写加载器最专，1 本）。详见 `ELF_FORMAT_SUPPLEMENT.md` 第九节。

**课程通用**：unidbg 简化 Linker 的底层 = "自写 so 加载器"（读 ELF 段 + 建符号表 + 建节表 + mmap），
`libbfd`（GNU 的 ELF 加载库）是参考实现。军棋 unidbg 简化 Linker（ch4 4.4 的 jelf 解析）= libbfd 加载器的简化版。

**libbfd 加载器的 3 个类（《二进制分析实战》4.2）**：
| 类 | 干什么 | 军棋 unidbg 对应（ch4 4.4 jelf） |
|---|---|---|
| `Binary`（4.2.1） | 解析 ELF 头 + 段（"这是什么文件 + 有哪些段"） | `ElfFile.readFile(so).getHeader()`（确认 32 位 ARM + 9 段） |
| `Section`（4.2.2） | 节表（`.text`/`.data`/`.got`...，"每段叫什么名 + 权限"） | unidbg Linker 遍历 PT_LOAD 段（mmap 各段到 0x40000000） |
| `Symbol`（4.2.3） | 符号表（`.symtab`/`.dynsym`，"哪些函数对外导出"） | unidbg 解析 `.dynsym`（军棋 14 个 `On*` 导出符号） |

**libbfd 的 `load()` 4 步（《二进制分析实战》4.3.1-4.3.4）vs unidbg jelf 解析（ch4 4.4）**：
| libbfd 步骤 | 干什么 | 军棋 unidbg jelf 对应 |
|---|---|---|
| 4.3.1 初始化 + 打开 | 读 ELF 文件 | `ElfFile.readFile(so)` |
| 4.3.2 解析基础属性 | ELF 头（位宽/架构/入口） | 确认 `EM_ARM=40` + `EI_CLASS=1`（32 位） |
| 4.3.3 加载符号 | `.symtab`/`.dynsym` | 解析 14 个 `On*` 导出符号（strip 后 `.symtab` 删，`.dynsym` 留） |
| 4.3.4 加载节信息 | `.text`/`.data`/`.got` 段 + 权限 | mmap PT_LOAD 段（r-x + rw- 两段）到 0x40000000 |

**课程通用价值**：写 unidbg 简化 Linker（不依赖 OS linker）= "自写 so 加载器"。`libbfd` 是 GNU 的参考实现，
理解了它，就理解 unidbg 简化 Linker（ch4 4.4 + ch7 7.2 的 12/5312/214/6 全通）在做什么——
**读 ELF + mmap + 重定位 + 跑 init_array = 自写加载器的 4 步**（libbfd 4.3 的 4 步等价）。

**军棋现状**：军棋 unidbg 简化 Linker 12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT 全通 = "自写加载器做对了"（ch7 7.2 实测）。

## 7.3 动态调试 Linker，探究 so 内存布局（加载后长什么样）


> **本节核心**：
> 1. 加载后内存布局：军棋 base=0x40000000，r-x + rw- 两段 mmap
>
> **本节难点**：
> 1. 段权限摆错全乱；调试看段坐标 + 权限
> 2. 动态调试 = 看加载后 so 在模拟内存里长什么样
> 3. 段布局是 RELATIVE 重定位基准，摆错全乱

![军棋 so unidbg 内存布局](assets/ch07_unidbg_memmap.png)



军棋 so 在 unidbg 加载后的内存布局（base=0x40000000）：

```
0x40001000  .interp / .hash / .dynsym / .dynstr   （动态链接信息 + 14 个导出符号）
0x40004000  .rel.plt (214 PLT 槽)                  （214 个跳板要填号）
0x40010000  .text (明文 ARM/Thumb，熵 7.10)        （代码段，明文）
0x4120bf8   .rodata (118KB 明文，0x138xxx 是高熵索引表)
0x4155000   .got (零值槽，静态文件未重定位)          （电话簿，静态时全是 0）
0x4155xxx   .data.rel.ro (只读全局)
0x4156000   .dynamic
0x4157000   .data
0x4158000   .bss
0x415a000   .init_array (12 个) + .preinit_array (1 个)
```

**这张布局怎么读？** 文件内偏移（4.5 节地图里的 0x001000/0x155000）加上基址
0x40000000，就是运行时虚拟地址（0x40001000/0x4155000）。`.got` 在 0x4155000，
静态时全 0——印证"你手里是没填号的电话簿"。

**r-x 段和 rw- 段的分界（大白话）**：
- r-x 段（0x40001000 起，`.text`+`.rodata`）= 代码+只读数据，"锁死的档案柜"
- rw- 段（0x4155000 起，`.got`+`.data`+`.bss`）= 可读写数据，"工作台"
分界点 0x4155000，`.got` 就在这个分界处（零值槽所在）。

**DiagTracePC 实证**：unicorn2 后端把 0xa37fc 存 `0x159880`（= 不 mask 值，
与真实 ARM 一致），PC off-by-2 bug 已修复（红线 §R6 更新）。

> 怎么验证"修没修好"？在 0xa37fc 这条指令上挂 trace（跟踪执行到哪），看它存进
> `.data` 的值：dynarmic 存的是 `0x159880 & ~3`（对齐值，错），unicorn2 存
> `0x159880`（不 mask，对）。军棋实测就是这条。

## 7.4 逐环节"能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **选后端** | unicorn2 修 PC off-by-2（dynarmic 的 `add rX,pc` 掩码 bug） | 绕过 §124 ① 上下文相关 BL 解码 bug（unicorn2 加载 so 后 BL 0x7965a 错位到 0x6eaea） |
| **加载 so** | 12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT 全通 | 读 `.got` 真值到静态文件（unidbg 只在内存里填，静态文件全 0） |
| **填依赖库** | 7 个 `DT_NEEDED` 全解析（5 自带 + 2 GL 挂桩） | GL 真渲染（无真 GPU，挂桩返回 0） |
| **跑 init_array** | 12+1 全跑完（通水电） | 跑通 `OnCreate` 业务逻辑（卡在 §124 ② 数据与指令对齐 0x43ed8（N44，原"对象图"）） |
| **内存布局** | base=0x40000000 全段映射，`.got` 零值槽预留 | 绕过 §124 ③ 288MB PROT_NONE mmap（unidbg mmap2 处理失败） |

**军棋实测对照**：
- 选后端 ✓（unicorn2，DiagTracePC 0xa37fc 存 0x159880）
- 加载 so ✓（12/5312/214/6 全通）
- 填依赖库 ✓（7 个全解析，2 GL 挂桩；含 liblog，32 位 so 的调试日志依赖）
- 跑 init_array ✓（12+1 全跑完）
- 内存布局 ✓（base=0x40000000，`.got` 零值槽预留）
- **✗ 绕过 §124 三项阻塞**：① BL 解码 bug（unicorn2 精度）② 数据与指令对齐（std::vector
  未初始化，0x43ed8）③ 288MB PROT_NONE mmap（unidbg mmap2 处理失败）

军棋 14 个 JNI 入口（readelf `--dyn-syms` 实测，加载全通后按地址调）：OnCreate /
OnDestroy / OnResult / OnResume / OnEvent / OnKey / OnUpdate / OnGraphChange /
OnBluetooth / OnPause / OnGraphCreate / OnTimer / OnInput / OnTouch。

## 7.5 军棋实战验证：加载全通但"能加载 ≠ 能调用"


> **本节核心**：
> 1. 加载全通但「能加载 ≠ 能调用」：12/5312/214/6 全通，OnCreate 卡 §124
> 2. 12 init_array 全通但第 4 层 C++ 对象图补不了
>
> **本节难点**：
> 1. 「能加载」是军棋 12/5312/214/6 实测，「能调用」需 redroid + 真 GPU
> 2. §124 三项阻塞是加载后调用前的最后一堵墙

军棋 unidbg 加载的"能加载 vs 能调用"分界（红线 §R6）：
| 维度 | 军棋实测 | 大白话 |
|---|---|---|
| 能加载（12/5312/214/6） | 全通 ✓ | 搬进来、接电话线、通水电都做了 |
| 能接通 Java（JNI_OnLoad） | DiagJni 全通 ✓ | 电话打给 Java 客服有人接了 |
| 能跑通 OnCreate | **卡 3 项阻塞** | 卡在 native 数据与指令对齐（N44） |
**关键认知**：军棋"能加载"早已达成，真正的卡点在 `OnCreate` 调用阶段
（§124 的 ① 上下文相关 BL 解码 bug ② 数据与指令对齐对应缺口（N44）③ 288MB PROT_NONE mmap）。
所以别把"加载全通"误读成"能玩了"——能加载只是"把 so 搬进内存"，离"游戏能跑"还差
数据与指令对齐语义（第 12/22 章展开）。
**"能加载全通"的 4 个证据（红线 §R6）**：
1. 12 个 init_array 全跑完（初始化函数无崩点）
2. 5312 RELATIVE 全换算（内部引用指对位置）
3. 214 PLT 槽全填（外部函数跳板接对）
4. 6 GLOB_DAT 全解析（全局变量填对真地址）
4 条全 ✓ = 加载阶段无 bug，卡点在加载之后（`OnCreate` 调用阶段）。
军棋 14 个 JNI 入口（readelf `--dyn-syms` 实测，"能调用"阶段才用到）：OnCreate /
OnDestroy / OnResult / OnResume / OnEvent / OnKey / OnUpdate / OnGraphChange /
OnBluetooth / OnPause / OnGraphCreate / OnTimer / OnInput / OnTouch。
## 7.6 本章小结（一句话版）

> unidbg/unicorn2 加载军棋 32 位 so：12 init_array + 5312 RELATIVE + 214 PLT +
> 6 GLOB_DAT 全通。
> 军棋 `.got` 是零值槽（静态文件未重定位），必须靠真机 dlopen 后 dump 才能填全。
> 军棋 so 不是壳（`.rodata` 明文 118KB 已 dump）；unicorn2 后端修掉 dynarmic 的
> PC off-by-2，是军棋 unidbg 路线的关键突破。
> "能加载"≠"能调用"，`OnCreate` 仍卡 §124 的 3 项阻塞
> （① 上下文相关 BL 解码 bug ② 数据与指令对齐对应缺口（N44）③ 288MB PROT_NONE mmap）。

---

## 视频
