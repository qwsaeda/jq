# 第28章 Anti-unidbg 系列：xHook 检测

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的 xHook / Inline Hook 检测

## 痛点：为什么 so 要检测"xHook / Inline Hook"？

xHook 是一个 Android 上的 inline hook 框架（把目标函数入口改写成跳板，
劫持它的执行）。逆向者常用 xHook hook 掉 so 里的校验函数，让 so 走"正常"
分支。**痛点 = so 反过来查"我的函数是不是被 hook 了"**。它检查自己
`.text` 段的字节，看某个函数入口是不是被改成了"跳板指令"
（`LDR PC, [PC, #4]` + 立即数）。一旦发现被改 → 说明有人 hook 我 →
走异常分支（崩溃 / 返回假数据）。军棋实测 3 个卡点。

### 卡点 1：`.text` 被改字节会被 so 自查出来

军棋 32 位 so 的 `.text` 是明文（红线 §R6，熵 7.10 = 正常机器码），
逆向者能用 unidbg Hook。so 会"自查"自己 `.text` 字节，看某函数入口
是不是被改成了跳板。

- **大白话卡点**：so 定期"照镜子"（查自己 `.text` 字节），看入口有没有
  被改成跳板。
- **为什么卡**：inline hook 改了 `.text` 前 4 字节，so 一查就发现"我被动过"。
- **不解决会怎样**：so 走异常分支（崩 / 返回假数据）。

**"照镜子"自查怎么做到（大白话）**：so 在 `.text` 里留一个"已知函数入口"
（它编译时就知道这入口前 4 字节长啥样），运行时读这 4 字节跟编译时值比对。
被 inline hook 改了 → 比对不上 → 发现"被动过" → 走异常分支。军棋
`.text` 明文（熵 7.10），so 能读到这些字节做自查。

### 卡点 2：unidbg 填 `.got` 的值会被查

军棋 `.got` 零值槽（静态未重定位 = 典型 unidbg 特征），so 查 `.got` 槽值。

- **大白话卡点**：unidbg 填的 `.got` 值和真机不一样（比如填了模拟地址），
  so 一查"电话簿里的号"就发现不对劲。
- **为什么卡**：`.got` 静态是零，unidbg 填的是"模拟侧地址"，真机填的是
  "真 libc 地址"。
- **不解决会怎样**：so 怀疑被分析 → 异常分支。

**为什么"`.got` 零值"是 unidbg 特征（大白话）**：真机 dlopen 后 `.got`
填的是真 libc 地址（非零）；unidbg 没重定位时是静态零值。so 查
"电话簿里的号"，零值 = "没真 linker 填过" = unidbg 特征。

**预判追问："unidbg 不是做了 214 PLT + 6 GLOB_DAT 重定位吗，`.got`
不就填了？"** 填了，但填的是 **unidbg 模拟地址**（AndroidResolver(23)
的 libc 桩地址），不是真机 libc 地址。如果 so 拿"`.got` 里的地址 +
基址换算"反查"这个 libc 是不是真 libc"，模拟地址会穿帮。军棋 so
本身不做这种深度校验（非风控 so），所以 214/214 填通即过；
带风控的 so 会查"地址落在哪个 .so 段"，得把 `.got` 填成"指向 bundled
真 32 位 libc 段"的模拟地址才像。

### 卡点 3：unidbg 默认的内存基址是"分析环境"指纹

军棋 base=0x40000000（unidbg 默认），so 查 `mmap` 基址。

- **大白话卡点**：so 查"我被加载在哪个地址"，unidbg 默认 0x40000000
  是"分析环境"指纹（真机 Bionic 32 位也在这附近，32 位不算异常，
  但 64 位会暴露）。
- **为什么卡**：32 位 ARM unidbg 基址跟真机一致（30 章会讲），
  但 64 位 arm64 真机基址是 0x7xxx（随机 ASLR），unidbg 默认
  0x40000000 就不一样。
- **不解决会怎样**：64 位 so 查基址就暴露是 unidbg。

**32 位 vs 64 位基址差异（大白话）**：
- 32 位 ARM：真机 Bionic 基址默认 0x40000000 附近，unidbg 也默认这 → 一致，不暴露
- 64 位 arm64：真机 ASLR 随机（0x7xxx），unidbg 默认 0x40000000 → 不一致，直接暴露

**预判追问："换 arm64 宿主机跑 64 位 unidbg 不就解决了基址暴露？"**
解决不了。拆两个独立维度看：
- **指令位宽**（CPU 模拟器执行 32 位还是 64 位指令）：换 arm64 宿主跑
  arm64 unidbg，确实能执行 64 位指令。
- **ELF 数据位宽**（被加载的 so 是 ELF32 还是 ELF64）：军棋 so **永远是
  ELF32**（`readelf -h` 实测 ELF32/ARM/EABI5），换宿主不改变 so 文件的
  位宽——你拿 64 位 unidbg 去加载 ELF32 的 `libjunqi.so`，加载器直接
  拒绝（架构不符）。
换 arm64 宿主只改"指令位宽"，不改"ELF 数据位宽"。军棋要走 64 位模拟，
得先有 64 位 so（红线 §R6：arm64 影子库是简化重写版，非原版汇编级逆向，
且资源/规则与 32 位原版不一致）。所以"换 64 位宿主"对军棋 32 位 so
**没有用**——so 是 ELF32，模拟路径还是 32 位。

## 为什么这样做：从 7 个维度检测"我是不是被 hook / 被分析"

类比：so 是"被盯防的保险柜"，它从 7 个角度查"小偷来过没有"：

| 维度 | 类比 | 大白话 |
|---|---|---|
| 1 ELF 文件格式 | 保险柜的出厂标签 | 查 `DT_FLAGS` 有没有分析标记 |
| 2 Linker 内存布局 | 保险柜放哪层楼 | 查 `.got` 是否被填（值对不对） |
| 3 重定向追踪 | 开门记录 | 查 `mmap` 基址（哪层楼） |
| 4 内存检索 | 柜里东西被动没动 | 查 `.text` 字节（动没动） |
| 5 跳转验证 | 锁芯有没有被撬 | 查 214 个 PLT 槽（跳板对不对） |
| 6 Inline Hook 检测 | 柜门铰链是不是被改了 | 查 5 字节跳板（入口改没改） |
| 7 xHook 检测 | 柜子上有没有贴纸标记 | 查 PLT/GOT 被改没 |

**为什么 7 个维度？** 每个维度查一个"小偷特征"：ELF 标签（出厂）、
Linker 布局（放哪）、重定向（开门记录）、内存检索（东西动没动）、
跳转验证（锁芯）、Inline Hook（铰链）、xHook（贴纸）。so 综合 7 个
维度判断"你是不是分析环境"。军棋 32 位 so 实测大部分维度**正常**
（`.text` 明文 100% 对齐、214 PLT 槽填对），主要特征是 `.got` 零值 +
基址 0x40000000。

### 28.1b 吸收自 4 本书（揉合去重）：反调试/反模拟/ptrace + 代码注入钩子

> **本节来源**：反模拟检测取《Linux 二进制分析》Ch5.7（检测模拟器 3 手段）、ptrace 反调试取《Linux 二进制分析》Ch3.12+Ch4.7、代码注入钩子取《二进制分析实战》Ch7（GOT/PLT 劫持）。详见 `ELF_FORMAT_SUPPLEMENT.md` 第八节。

**28.2 的"7 维度检测"是 xHook/Inline Hook 检测，本节补 3 类更底层的检测**（跟 ch27 反检测 8 维度呼应）：

**反调试/反模拟检测**（《Linux 二进制分析》3.12/4.7/5.7）：
| 检测手段 | 原理 | 军棋反检测 8 维度 |
|---|---|---|
| `PTRACE_TRACEME`（4.7.1） | so 调 `ptrace(PTRACE_TRACEME)` 查"有没有调试器 attach" → 有则拒绝跑 | "Unicorn/Frida 检测"项（Frida 用 ptrace） |
| `SIGTRAP` 处理（4.7.2） | so 处理 `SIGTRAP`（断点信号）检测硬件断点 | "Unicorn 检测"项（Unicorn 单步触发 SIGTRAP） |
| `/proc/self/status`（4.7.3） | so 查 `/proc/self/status` 的 `TracerPid` 字段（= 调试器 PID，0 = 无调试器） | "Unicorn/Frida 检测"项 |
| **系统调用检测模拟**（5.7.1） | so 调 `uname`/`syscall` 查内核特征（`/proc/self/maps` 找 `libunicorn`/`frida` 模块） | "Unicorn/Frida 检测"项 |
| **CPU 不一致检测**（5.7.2） | 查 `/proc/cpuinfo`（模拟器 CPU 型号 vs 真机） | "uname"项 |
| **指令时延检测**（5.7.3） | 特定指令间计时（模拟器比真机慢） | "时间"项 |

**代码注入/钩子检测**（《二进制分析实战》第 7 章 + 《Linux 二进制分析》7.3.4）：
| 检测手段 | 原理 | 军棋 |
|---|---|---|
| **GOT/PLT 钩子检测**（7.3.4） | so 查 `.got`/`.plt` 槽是否被 xHook 改过（跳板指向非正常段） | 军棋 214 PLT 槽正常（28.2 维度 5/7） |
| **函数蹦床检测**（6.2.3） | so 查 `.text` 是否有 trampoline 跳板（inline hook 改了前 5 字节） | 军棋 `.text` 100% 对齐没改字节 → 检测正常（28.2 维度 4/6） |

**课程通用价值**：unidbg 跑壳/加固 so 时，**反模拟检测（查 Unicorn/Frida）是最大障碍**——so 查 `/proc/self/maps` 找 `libunicorn.so`/`frida-server` 模块，找到就拒绝跑。unidbg 要：
1. mock `/proc/self/maps`（隐藏 `libunicorn`/`frida` 模块，伪装成真机）
2. mock `ptrace`（返 `EPERM`，so 以为没有调试器）
3. mock `/proc/self/status` 的 `TracerPid=0`（so 以为无调试器）
4. mock `uname`（返真机 CPU 型号）

**军棋 32 位 so 反检测 8 维度**（AGENTS.md 红线）：基址/JNI/类/fd/uname/时间/Unicorn/Frida——**"Unicorn/Frida 检测"正对应存根反模拟检测**（so 查 `/proc/self/maps` 找模拟器模块）。军棋 so 非壳但有反检测 8 维度（广告层 + 加固），**"检测正常" = 军棋 unidbg mock 对了 8 维度**（28.2 军棋特征列）。

## 28.2 xHook 检测的 7 维度（军棋 so 实测）


> **本节核心**：
> 1. xHook 检测 7 维度（军棋 so 实测）
>
> **本节难点**：
> 1. .text 改字节 / unidbg .got / 默认基址 / Hook 跳板 / 符号表缺失 / 反汇编特征 / 时间戳
> 2. 「7 维度」是 xHook 反检测核心
> 3. 军棋 .text 100% 对齐没改字节 → 检测正常；影子运行时改 .text → 命中
> 4. 换 arm64 宿主没用：指令位宽 vs 数据位宽，数据位宽不变检测照命中

![军棋 xHook 检测 7 维度](assets/ch28_xhook_detect.png)



| 维度 | 大白话 | 军棋特征 |
|---|---|---|
| 1 ELF `DT_FLAGS` | 查出厂标签 | 军棋 `.dynamic` 7 个 DT_NEEDED（正常） |
| 2 Linker 布局 | 查 `.got` 是否被填 | 军棋 `.got` 零值槽（未重定位 = unidbg 特征） |
| 3 重定向追踪 | 查 `mmap` 基址 | 军棋 base=0x40000000（unidbg 默认，32 位正常） |
| 4 内存检索 | 查 `.text` 字节 | 军棋 `add rX,pc`（dynarmic mask = 模拟器特征，unicorn2 已修） |
| 5 跳转验证 | 查 214 个 PLT 槽 | 军棋 PLT 槽被 unidbg 填（214/214，正常） |
| 6 Inline Hook | 查 5 字节跳板 | 军棋 `.text` Thumb 指令（跳板需 4 字节对齐） |
| 7 xHook 检测 | 查 PLT/GOT 被改 | 军棋 PLT 槽指向正常段 |

**军棋 7 维度里哪些"正常"哪些"异常"？** 维度 1/3/5/7 都正常
（`.text` 明文、214 PLT 槽填对、基址 32 位正常）；维度 2（`.got` 零值）
和维度 4（dynarmic mask）是 unidbg 特征——维度 4 已被 unicorn2 后端修掉
（PC off-by-2 消失，DiagTracePC 实证 0xa37fc 存 0x159880 = 不 mask 值，
与真实 ARM 一致），维度 2 要"填 `.got` 真地址"去掉零值特征。

## 28.3 逐环节"我们能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **填 `.got`** | 214 PLT + 6 GLOB_DAT 全填（AndroidResolver(23) 模拟地址），去"零值"特征（维度 2） | 填"真机 libc 地址"（模拟环境没有真 libc，填的是桩地址；带风控 so 查"地址落在哪个段"仍可能穿帮） |
| **`LD_PRELOAD` 空** | `setEnv("LD_PRELOAD", "")`（维度 7 反 Frida） | 让 so 完全不知道"是 unidbg"（288MB mmap / 基址等特征仍在） |
| **挂桩 GL** | 挂桩 `libGLESv1_CM`（去"无真 GPU"特征） | 模拟真 GPU 渲染（挂桩只过加载，画不出画面） |
| **维度 4 mask** | 切 unicorn2 后端修 PC off-by-2（DiagTracePC 实证） | 修 §124c 的 init_array 后 bx bug（是"分支损坏"不是"mask"，unicorn2 修不了） |

**注意 §124c 与维度 4 的区分**：维度 4（`add rX,pc` mask）是 dynarmic
已知 bug，unicorn2 已修（军棋 11 个 MISMATCH 归零）；§124c 的
"init_array 后全局分支损坏"（bx r3/b.w/ldr pc 全跳错目标）是另一个
独立的模拟器精度 bug，unicorn2 修不了，军棋用 code cave + no-init_array
绕过（§124d-g，见 25 章）。

## 28.4 unidbg 反 xHook 检测（军棋 3 招）


> **本节核心**：
> 1. unidbg 反 xHook 3 招
>
> **本节难点**：
> 1. 补 .got 值（模拟 dlopen）/ 改内存基址（模拟 ASLR）/ 跳过 Hook 桩
> 2. 「3 招」是 unidbg 反 xHook 手段
> 3. unidbg 默认基址 0x40000000 是分析指纹，mock ASLR 随机基址绕过

![Inline Hook 检测](assets/ch28_inline_hook.png)



```java
// 1. 填 .got(重定位 214 PLT + 6 GLOB_DAT, 去掉"零值"特征)
emulator.getMemory().setLibraryResolver(new AndroidResolver(23));
DalvikModule dm = vm.loadLibrary(new File("lib/armeabi/libjunqi.so"), true);
// loadLibrary 跑完, .got 槽已填(模拟地址)

// 2. 设 LD_PRELOAD=""(反 Frida 检测, 见 27 章)
emulator.getMemory().setEnv("LD_PRELOAD", "");

// 3. 挂桩 libGLESv1_CM(去"无真 GPU"特征)
emulator.getMemory().symbolResolver().resolve("glGetError").when((h) -> 0L);
```

**3 招分别去掉什么特征？**
- 招 1（重定位填 `.got`）：去掉"`.got` 零值 = unidbg 没重定位"特征（维度 2）
- 招 2（`LD_PRELOAD=""`）：去掉"frida-gadget 被预加载"特征（维度 7）
- 招 3（挂桩 GL）：去掉"无真 GPU"特征（so 以为有 GPU）

**为什么军棋"3 招 + 切 unicorn2"够用？** 军棋 7 维度里 5 个本来就正常
（`.text` 明文、214 PLT 槽真填、基址 32 位），要反的只有维度 2
（`.got` 零值）+ 维度 4（dynarmic mask）。招 1 去维度 2、
unicorn2 后端去维度 4，3 招补齐。

## 28.5 军棋实战验证：32 位 so 的 xHook 检测"基本正常"


> **本节核心**：
> 1. 军棋 32 位 so xHook 检测基本正常（.text 明文没改字节）
>
> **本节难点**：
> 1. 但影子运行时改 .text 字节（机械翻译 C）→ 检测命中
> 2. 「基本正常」是原版 32 位 so；影子运行时（64 位翻译体）改 .text → 命中
> 3. 军棋 32 位 so 反检测 8 维度基本通过

军棋 32 位 so 的 xHook 检测实测（红线 §R6）：
| 维度 | 军棋实测 | 结论 |
|---|---|---|
| 维度 1 ELF 标签 | 7 DT_NEEDED，正常 | ✓ 正常 |
| 维度 2 `.got` | 静态零值 → loadLibrary 后填模拟地址 | 招 1 去掉零值特征 |
| 维度 3 基址 | 0x40000000（32 位正常） | ✓ 正常 |
| 维度 4 `.text` mask | dynarmic mask（已切 unicorn2） | unicorn2 修掉 ✓ |
| 维度 5 PLT 槽 | 214/214 填对 | ✓ 正常 |
| 维度 6 Inline Hook | Thumb 指令，未改 | ✓ 正常 |
| 维度 7 xHook | PLT 指向正常段 | ✓ 正常 |
**关键认知**：军棋 32 位 so 的 xHook 检测**大部分维度正常**（因为
`.text` 明文、214 PLT 槽真填、基址 32 位跟真机一致）。真正的 unidbg
特征只有 2 个：`.got` 零值（维度 2）+ dynarmic mask（维度 4）。
这两项分别用"重定位填 `.got`" + "切 unicorn2 后端"解决。
**军棋 32 位 so 的反检测难度远低于 64 位 arm64 so**（64 位基址 0x7xxx
随机 ASLR，unidbg 默认 0x40000000 直接暴露，30 章）。
**"能反检测" ≠ "能玩"（诚实边界）**：xHook 7 维度全过了，只证明
"so 没查出来是 unidbg"，不证明"so 能完整跑 OnCreate"——§124d-g
的三项阻塞（init_array 路径 bx bug / no-init_array 路径全局未初始化 /
288MB PROT_NONE mmap）与环境变量/hook 检测无关，反检测全通也突破不了
对象图卡点。
## 28.6 本章小结（一句话版）

> xHook 检测 = so 查"我是不是被 hook 了"。
> 军棋 so 7 维度查：ELF / Linker / 重定向 / 内存 / 跳转 / Inline / xHook。
> 军棋 32 位 so 大部分维度正常（`.text` 明文、214 PLT 槽填对、
> 基址 0x40000000），真正 unidbg 特征只有 `.got` 零值 + dynarmic mask。
> unidbg 反检测 3 招：重定位填 `.got`（去零值）+ 切 unicorn2（去 mask）+
> `LD_PRELOAD=""` + 挂桩 GL（去无 GPU 特征）。
> 32 位 so 反检测难度远低于 64 位（基址巧合一致吃福利）；
> 换 arm64 宿主对 32 位 so 没用（ELF 数据位宽不变，军棋 so 永远 ELF32）。


> **延伸阅读（附录 C.4 增强循环方法论）**：本章讲"XHook 注入式 Hook"（正向对抗视角），军棋会话 2 的**"每轮闭环→沉淀→下轮"增强循环**是 XHook 类工具在"逆向侧"的工程化——每轮"能玩"缺口闭环后，把"下轮目标 + 当前诚实边界"写清（AGENTS.md 红线），下轮从经验库直接续，**防"反复踩同一坑"**（8 坑 3 类：JNI 反射 / C 语义契约 / 链接）。详见 `APPENDIX_C_REVERSE_ENGINEERING.md` C.4。

## 视频
