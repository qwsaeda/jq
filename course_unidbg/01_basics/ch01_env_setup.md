# 第1章 unidbg 环境准备与快速上手

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地实战靶机：军棋 `lib/armeabi/libjunqi.so`（ELF32 / ARM / EABI5 / stripped）

## 痛点：为什么需要 unidbg？

**痛点 1：真机环境搭不起来。** 你要逆向分析一个 ARM `.so` 的内部逻辑（比如军棋的走子/吃子规则、SPAK 解密），但你的开发机是 x86_64 Linux，跑不了 ARM 二进制。用红米手机一台台试？太慢、不可复现、没法下断点。

**痛点 2：模拟器太重。** QEMU 全系统模拟要几分钟起一个 Android 虚拟机，对"我就想跑个函数看看返回值"来说太重了。

**痛点 3：ARM 用户态的"环境"很复杂。** `.so` 不是孤立的——它依赖 libc、调系统调用、还要**通过 JNI 回调 Java 方法**。模拟执行要把这些"环境"都补上，这就是"补环境"的核心难题。

**调用方向澄清**（避免混淆）：
- **Java → so（正向调用）**：Java 代码里 `native` 方法（如军棋 `com.cnvcs.App` 里的 14 个 `On*`）通过 JNI 的 `RegisterNatives`/符号查找，**由 Java 侧发起调用**，跳进 so 的 native 函数。这是 so 的"被调方"。
- **so → Java（反向回调）**：so 内部用 `FindClass` / `GetMethodID` / `CallXxxMethod`（JNI 接口表的函数指针）**反过来调 Java 方法**（如军棋 so 调 `GLView` 的渲染回调、广告 SDK 调 `AdView.onAttachedToWindow`）。这是 so 的"主动调方"，需要模拟环境里有个"假 Dalvik VM"来接。

unidbg 模拟的两类场景：
| 场景 | 调用方向 | 模拟层 | 军棋例子 |
|---|---|---|---|
| 调 so 函数 | Java → so | unidbg `loadLibrary` + `call()` | 调 `OnCreate` |
| so 回调 Java | so → Java | unidbg `DalvikVM` 接 `FindClass`/`CallXxxMethod` | so 调 `GLView` 渲染 / `AdView` 广告 |

**unidbg 就是解决这三个痛点的工具：用 Java 在 x86 宿主上模拟 ARM 用户态（不是完整 OS，只模拟用户态），把 `.so` 需要的环境逐步补齐，让你在 PC 上"像跑真机一样"调用 so 函数。**

## 为什么这样做：思路类比

把 unidbg 想象成**"给 ARM 程序搭一个虚拟房间"**：

- 你要在这个房间里跑一个 ARM 程序，但房间的地面（CPU）不匹配——ARM 指令跑不了。→ 装一台 **unicorn2/dynarmic 后端**（CPU 模拟器），把 ARM 指令**逐条动态翻译**成宿主 ISA 执行（x86_64 宿主译成 x86_64，arm64 宿主译成 aarch64）。
- 房间里得有桌子椅子（内存/栈/堆/ELF 段）→ **Memory 模块**帮你 mmap、分配栈。
- ARM 程序里有些调用是"去楼下小卖部"（Java 方法、系统调用）→ **DalvikVM** 模拟 Java 侧，**SvcMemory** 拦截系统调用。
- 你要"控制"这个房间（下断点、改指令、改返回值）→ **Hook/Patch 框架**。

一句话：unidbg = **CPU 模拟 + 内存模拟 + Java 模拟 + 系统调用拦截**，让 ARM so "以为自己在真 Android 上跑"。

## 1.2 核心概念（大白话版）


> **本节核心**：
> 1. unidbg = CPU 模拟器 + 虚拟内存 + 文件系统 + DalvikVM 四层拼成的假 Android 环境
> 2. 军棋走 32 位路径：for32Bit() 构建，arm64 宿主机跑 ARM32 靠 Houdini/QEMU 动态翻译指令流
> 3. strip 后只能按地址调用：14 个 On* 入口是 .dynsym 里仅剩的导出符号，内部函数无名
>
> **本节难点**：
> 1. 四层模拟各自有对应物：CPU(指令翻译)/内存(mmap 段)/文件(open 映射宿主)/Java(DalvikVM 假 VM)，缺一层 so 就崩一层
> 2. 「换 arm64 宿主没用」是高频误解：指令位宽在翻译层可变，但 ELF 数据位宽（.rodata/.got 字节）不变，反检测照样命中
> 3. 军棋 **1.43MB/1435156B** so + 7 依赖 + C++ 对象图复杂度远超书内样本，4 层全配齐只是「能加载」，离「能调用」还差 §124 三项阻塞

![unidbg 四层架构](assets/ch01_unidbg_architecture.png)



| 概念 | 一句话解释 | 军棋对照 |
|---|---|---|
| AndroidEmulator | 总入口，组装 CPU+内存+VM | 军棋 32 位 so 的运行环境 |
| DalvikVM | 模拟 Java 侧（军棋 smali 只有 7 个类） | `com.cnvcs.App` 等 |
| Memory | 虚拟内存/栈/堆，承载 so 的段 | `.got` 是零值槽（红线） |
| Hook/Patch | 拦截系统调用或改写指令 | 反检测 + 补环境 |
| 符号调用 | 按函数名调（导出符号表） | 14 个 `Java_com_cnvcs_App_On*` |
| 地址调用 | 按函数入口地址调（军棋 strip 后符号表空） | 14 JNI 入口 Thumb 奇地址 0x233e1 等 |

## 1.3 第一个 unidbg 项目（军棋实战）


> **本节核心**：
> 1. 4 步骨架：for32Bit() + AndroidResolver(23) + loadLibrary(so) + 调 OnCreate
> 2. 军棋 loadLibrary 自动跑 12 个 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT（红线 §R6 实测）
> 3. 军棋无「写 so」环节，直接拿现成 libjunqi.so（**1.43MB/1435156B** strip）调，跳过书内 NDK 编译
>
> **本节难点**：
> 1. AndroidResolver(23) 自带 5 个 32 位系统库，但军棋 7 依赖里的 2 个 GL 库（libGLESv1_CM/v2）无真 GPU 必须挂桩
> 2. loadLibrary 返回 ≠ 函数能调：12 init_array 全通只是加载，OnCreate 还卡 18 常量 + §124

![so 加载五步流水线](assets/ch01_load_flow.png)



```java
// 32 位 ARM → unicorn2 后端（x86_64 宿主）
// 为什么用 unicorn2 不用 dynarmic？dynarmic 对 Thumb `add rX,pc` 用了
// (addr+4)&~3 的 mask，与真实 ARM 差 2 字节 → 军棋 PC off-by-2 bug。
// unicorn2 用 (addr+4) 不 mask，与真实 ARM 一致。
AndroidEmulator emulator = AndroidEmulatorBuilder.for32Bit()
        .backend(Backend.UNICORN2).build();

// AndroidResolver(23) 自带 Android 7 的系统库（libc/libstdc++/libm/liblog/libdl）
emulator.getMemory().setLibraryResolver(new AndroidResolver(23));

// 模拟军棋这个 App 的 Java 侧（com.cnvcs.junqi）
VM vm = emulator.createDalvikVM(new File("junqi.apk"));
vm.setVerbose(false);

// 加载军棋 32 位 so，init=true 表示跑 .init_array
DalvikModule dm = vm.loadLibrary(new File("lib/armeabi/libjunqi.so"), true);
vm.callJNI_OnLoad(emulator, dm.getModule());
```

**加载这一下，后台悄悄做了 5 件事**（类比"把 so 搬进房间"）：
1. **认门牌**（解析 ELF 头 + 程序头表）：确认这是 32 位 ARM（EM_ARM=40），有 9 个段
2. **摆家具**（mmap PT_LOAD 段到 0x40000000）：`.text` 放 r-x 区、`.got` 放 rw- 区
3. **搬依赖**（递归加载 7 个 DT_NEEDED 库）：libc/libstdc++/libm/liblog/libdl/libGLESv1_CM/libGLESv2
4. **填地址**（重定位）：RELATIVE 5312 槽 + PLT 214 槽 + GLOB_DAT 6 槽（军棋实测全通）
5. **开门**（跑 .init_array）：12 个构造函数 + 1 个 preinit（军棋实测全通）

**补 Java 方法的标准代码骨架（`extends AbstractJni`，社区绿洲/微博案例沉淀）**：

军棋 so 内部会回调 Java（FindClass/GetMethodID/CallXxxMethod），`loadLibrary` 只解决"so 搬进来"，
**Java 侧应答要自己补**——标准做法是继承 `AbstractJni`（unidbg 已补好通用 case），
重写缺失的 `newObjectV`/`callObjectMethodV`/`callIntMethodV`：

```java
// 军棋补 Java 方法的标准骨架(绿洲/微博 unidbg 案例沉淀)
public class JunqiJni extends AbstractJni {
    private final AndroidEmulator emulator;
    private final VM vm;
    private final Module module;

    JunqiJni() {
        emulator = AndroidEmulatorBuilder.for32Bit()
                .setProcessName("com.cnvcs.junqi")  // 规避进程名校验
                .build();
        emulator.getMemory().setLibraryResolver(new AndroidResolver(23));
        vm = emulator.createDalvikVM(new File("junqi.apk"));
        module = vm.loadLibrary(new File("lib/armeabi/libjunqi.so"), true).getModule();
        vm.setJni(this);          // 关键: 把"我"设成 JNI 应答器
        vm.setVerbose(true);
        dm.callJNI_OnLoad(emulator);
    }

    // 补 so 回调的 Java 方法: 按 signature 重写缺失的 case
    @Override
    public DvmObject<?> newObjectV(BaseVM vm, DvmClass c, String signature, VaList a) {
        // 军棋 AdView 去广告(见 3.1): 返 null 走"广告不可用"分支
        if (signature.equals("com/baidu/mobads/AdView-><init>()V"))
            return vm.resolveClass("com/baidu/mobads/AdView").newObject(null);
        return super.newObjectV(vm, c, signature, a);  // 通用 case 交给 AbstractJni
    }
}
```

**为什么用 `extends AbstractJni` 而不是从零写**：`AbstractJni` 已补好 `java/lang/*` 通用
case（String/Integer/ByteArray 等），你只重写 so 特有的（军棋 = 广告层 + GLView 渲染回调）。
补环境代码写**自己的类**（不动 `AbstractJni` 源码），可移植性最好（防 unidbg 更新破坏）——
详见 22/26 章"补环境两种写法"规范。

## 1.4.1 unidbg 支持 32 位 / 64 位（军棋走 32 位路径）


> **本节核心**：
> 1. for32Bit() 指定指令位宽 32，unidbg 模拟 32 位 ARM CPU + 32 位地址空间
> 2. 军棋 so 是 ELF32/ARM/EABI5，必须 for32Bit（arm64 so 才用 for64Bit）
> 3. 32 位 vs 64 位是 unidbg 构建参数，不是 so 自带属性——同一个 so 不能被两种 for 构建
>
> **本节难点**：
> 1. 军棋阶段 2 arm64 简化重写版（for64Bit）不是原版逆向，红线要求不包装成完整版
> 2. 32 位设备走 v7a 原版完整逻辑，64 位 only 设备装得上但 32 位库加载失败（预期内）

**unidbg 不是"只支持 32 位"**，它有两条路径（unidbg 0.9.8 实测）：
| 路径 | 写法 | 军棋对应 |
|---|---|---|
| 32 位 ARM | `AndroidEmulatorBuilder.for32Bit()` | **军棋 `libjunqi.so`（ELF32/ARM，EM_ARM=40）走这条** |
| 64 位 AArch64 | `AndroidEmulatorBuilder.for64Bit()` | 军棋 64 位简化重写版（红线 §64 位 native，非 unidbg 模拟） |
**军棋为什么走 32 位路径**：军棋 `libjunqi.so` 是 ELF32/ARM 二进制（`readelf` 显示 `EM_ARM=40`、`EI_CLASS=1`），
所以必须用 `for32Bit()` + `AndroidResolver(23)`（自带 32 位 `lib/armeabi-v7a/` 系统库）。
unidbg 的 `unidbg-android-0.9.8.jar` 里同时带了 32 位 `lib/armeabi-v7a/` 和 64 位 `lib64/` 两套系统库。
**CPU 后端 3 个可选**（`unidbg-api` 提供）：
- **unicorn2**（QEMU 用户态模拟器，C 写、Java 经 JNI 调）：与真实 ARM 一致，军棋用它（修 PC off-by-2）
- **dynarmic**（纯 Java 翻译器）：快，但 `add rX,pc` 用 `(addr+4)&~3` 掩码，与真实 ARM 差 2 字节（军棋 PC off-by-2 bug）
- **hypervisor**（真 hypervisor，KVM 加速）：需宿主支持，军棋 x86 宿主不可用
**结论**：军棋 unidbg 路线 = `for32Bit()` + **unicorn2 后端** + `AndroidResolver(23)`。
不是 unidbg 只支持 32 位，而是军棋样本恰好是 32 位 ARM 二进制。
## 1.4.2 ARM32 怎么在 arm64 宿主机上跑起来？（逐环节讲清）

先破一个混淆：**"转译成宿主 ISA"是 CPU 指令层面，"so 是 32 位还是 64 位"是数据格式层面，两者独立**。
unidbg 把军棋 ARM32 指令转译成宿主 ISA 执行（指令位宽变），但**军棋 so 永远是 ELF32**
（数据指针 4 字节、std::vector 按 32 位布局、`.got` 32 位槽、依赖 32 位库）——**不会变成 64 位 ELF**。

### 环节 1：指令位宽怎么转（CPU 层）


> **本节核心**：
> 1. CPU 层逐条翻译 32 位 ARM 指令为宿主指令，指令流保存为影子运行时
> 2. dynarmic 对 Thumb add rX,pc 用 (addr+4)&~3 mask，与真实 ARM (addr+4) 不一致 → PC off-by-2
> 3. unicorn2 用 (addr+4) 不 mask，与真实 ARM 一致 → 军棋 0xa37fc 实证修 PC off-by-2
>
> **本节难点**：
> 1. 指令位宽可换（32↔64），但翻译是逐条做，非「重编译整个 so」——保存的是翻译产物
> 2. dynarmic off-by-2 是上下文相关 bug：隔离环境 BL 全对，加载 .so 后 BL 0x7965a 错位到 0x6eaea（§124 ①）

**unidbg 的"运行时动态翻译" vs 你们的"静态翻译工具"（关键区分）**：

| 维度 | unidbg（unicorn2/dynarmic） | 你们 `rev64/reverse/src_shadow` 翻译工具 |
|---|---|---|
| **翻译时机** | **运行时逐条译**：每条 ARM 指令执行前才译，译完即弃 | **静态批量译**：整个 `.text` 段一次翻成 C 源码，再编译成 so |
| **翻译产物** | 不留存（指令流在内存里，跑完就没） | `jq_funcs_0-3.c` + `libjunqi_arm64_shadow.so`（14MB 静态文件） |
| **能改指令吗** | 不能（译的是原始 ARM 指令，改不了逻辑） | 能（C 源码可改，可 patch、可补环境） |
| **谁译的** | QEMU/unicorn2 C 库（**几十年 ARM/Android 设备考验**） | 你们 `thumb2c_full.py`（**自己写的，需回归验证**） |
| **跑的时候** | unidbg JVM 里（Java 堆模拟内存） | **真机 arm64 进程**（MuMu 红米 90s+ 不崩） |

**unicorn 译成"宿主当前架构的指令"**（不是固定译 x86）：

| 宿主架构 | unicorn 译成什么 | 军棋现状 |
|---|---|---|
| x86_64 Linux/macOS（开发机） | **x86_64 指令** | ✅ 军棋在 x86_64 PC 跑（译 x86_64，性能最好） |
| aarch64（真 ARM 服务器） | aarch64 指令 | 军棋没走这条（32 位 so 要 32 位模拟器，成本高） |

**关键**：unicorn 是"动态翻译 ARM 指令 → 宿主当前 ISA"——宿主是 x86 就译 x86，宿主是 aarch64 就译 aarch64，**没有"固定译 x86"这回事**。军棋选 x86_64 PC 是因为开发机是 x86_64（译成 x86_64 性能最好），不是"只支持 x86"。

**跟你们翻译工具的区别**（"宿主译成什么"无关，关键在"逐条译 vs 批量译"）：
- unidbg 是"翻译+执行一体"（逐条译、不留存、不能改）
- 你们是"先翻译成 C 再编译执行"（批量译、留 C 源码、能改逻辑）

**你们担心的"翻译指令错误"已被回归流水线兜住**（红线 §R6 §122）：
- 翻译器 `thumb2c_full.py` 把每条 ARM 指令翻成 C，**unicorn2 模拟器当裁判**做函数级回归
- 喂相同输入，比 返回值 + crc32(结构体) + crc32(.data)
- **可跑函数 1174/1174 PASS，一致率 100%**（15 个 MISMATCH 全修了，2 个翻译器 bug：`add rX,pc` PC 对齐 + `asrs` 算术右移）
- **诚实边界**：① 回归是 5 个构造假结构体，非真实 C++ 对象图 ② 验指令语义 ≠ 验"能玩"（§124 三项阻塞会让影子库黑屏）

**"函数逻辑对齐"的本质 = 副作用 4 类对齐**（红线 §122 降级判据）：

| 类别 | 例子 | 怎么验 | 现状 |
|---|---|---|---|
| **① 返回值** | 函数 ret r0 | 直接比 r0 | ✅ 1174 全验 |
| **② 写自身 `.data`/堆** | 改 `global_x`、`vector->data` | crc32(.data 区) + crc32(结构体区) | ✅ 验了，但只验"函数自身"的结构体 + .data 段 |
| **③ 写"别的函数"的内存** | A 函数写 B 函数的栈帧/全局（跨调用） | 没验 | ❌ 单函数回归不追 BL 链（BL 立即数存根 r0=0 双向对齐） |
| **④ 数据与指令对齐** | `std::vector` 的 begin/end/capacity 三指针 + 元素 | 没验 | ❌ §124 ② 卡点（32→64 布局变了，两边输入其实不同） |

**"100% 一致率"的真实含义**（诚实降级）：
> 在 5 个构造输入下，**单函数**的 **① 返回值 + ② 自身 .data/结构体副作用** 一致率 100%。
> **不是**：③ 跨调用副作用一致、④ 数据与指令对齐一致、或"任意输入/功能一致"。

**unidbg 为什么没这个问题（关键对比）**：

| 维度 | 你们（32→64 翻译） | unidbg（直接跑 32 位 so） |
|---|---|---|
| **跑的是啥** | 翻成的 64 位 C 编译体 | **原始 32 位 `.text` 字节**（unicorn2 逐条译宿主 ISA） |
| **对象图** | 32→64 布局变了（vector 12B→24B），④ 验不了 | **32 位布局原样保留**，对象图不破坏 |
| **③④ 副作用** | 翻译层就错位（指令全对也分叉） | **不存在翻译错位**（没翻译，直接跑原版） |
| **卡在哪** | 翻译错位 + §124（影子库黑屏） | **§124 三项阻塞**（模拟器精度 + 数据与指令对齐未初始化，非翻译错位） |

unidbg 的强项是"补环境 + 跑原始字节"：壳/花指令/VM 都是"跑原始字节时遇到什么就模拟什么"（第 4/9 章详讲），**因为不翻译，所以没有"翻译错位"风险**。你们做翻译，错位风险在翻译层；unidbg 不翻译，错位风险在模拟器精度层（§124）——**军棋是明文 so 非壳，比原书覆盖的加壳/花指令/VM 样本简单得多**，但 §124 三项阻塞（① BL bug ② 数据与指令对齐缺口（N44）③ 288MB mmap）仍卡住"能玩"。

```
军棋 ARM32 指令(libjunqi.so 的 .text, **1.43MB/1435156B** 二进制)
   │
   ▼  unicorn2 后端(QEMU 用户态, C 库, Java 经 JNI 调)
   │
   ├─ 宿主是 x86_64(你现在这台机器):  ARM32 指令 → 转译成 x86_64 指令 → x86_64 CPU 执行
   └─ 宿主是 arm64(假设的 arm64 机器): ARM32 指令 → 转译成 arm64 指令 → arm64 CPU 执行
```
**我们在这个环节能干什么**：
- **选后端**：军棋用 `unicorn2`（跟真实 ARM 一致）；`dynarmic` 有 PC off-by-2 bug（§R6 已修）
- **不能干**：改转译目标（宿主是 x86_64 就只能转 x86_64；宿主是 arm64 就转 arm64）
- **军棋红线**：unicorn2 修了 dynarmic 的 PC off-by-2（DiagTracePC 0xa37fc=0x159880 实证）
### 环节 2：数据位宽不变（ELF 层）—— 这是"换 arm64 宿主没用"的根因


> **本节核心**：
> 1. ELF 数据位宽不变：.rodata/.got/.data 的字节是 so 文件原样，换宿主不重编译就永远 32 位
> 2. 这是「换 arm64 宿主没用」的根因：xHook 检测查 .got/.rodata，数据位宽不变照样命中
> 3. 军棋 .rodata 118832B 明文，.got 零值未重定位，数据位宽决定反检测指纹
>
> **本节难点**：
> 1. 指令位宽（翻译层）vs 数据位宽（ELF 文件）是两回事，混淆即「换宿主能绕反检测」误解
> 2. 影子运行时（64 位翻译体）是「指令转译 + 数据保留 32 位」混合产物，非「整个 so 转 64 位」

**`.rodata`/`.got` 是什么（跟 4 本书 ch04 4.3b ELF 全字段呼应）**：

| 段 | 是什么 | 大白话 | 军棋 |
|---|---|---|---|
| **`.rodata`**（Read-Only Data，只读数据段） | 程序里所有"不变的常量/字符串/资源表"（`const` 变量、字符串字面量） | "锁死的资料架"——so 里的明文字符串（`Junqi.Move`/`data/config/*.xml`/SPAK key 等） | 118KB 明文（静态=运行时 0/118832 不同，非壳） |
| **`.got`**（Global Offset Table，全局偏移表） | "外部函数/全局变量的门牌号"——so 调 libc/GL 函数时先查 `.got` 拿真地址 | "电话簿"——6 GLOB_DAT + 214 PLT 跳板查 `.got`，**静态全 0**（未重定位） | 零值槽（dlopen 重定位才填，4 本书 ch06 6.2 延迟绑定） |

**"动态加载库的指令都是相对位置，只有加载后确定内存地址后，固定点的地址+偏移就是真实地址"（你说的，ch06 6.2.4 重定位全谱系呼应）**：
- `.got`/`.data` 里的指针 = **相对偏移**（"文件里第 N 字节"，不是真地址）
- dlopen 时 linker **重定位**（ch06 6.2）：给每个相对偏移 `+= 基址 0x40000000` → 变**真实内存地址**
- **军棋 `.got` 全 0 = "门牌号没填"**（静态文件未重定位）；**真机 dlopen / unidbg 模拟 / Frida 真机 attach 都会填**
- **"用真实加载动态运行取 `.rodata`+`.got`"**（你说的）：`.rodata` 静态=运行时（明文，redroid dump 已落 `frida-dump/rodata_dec.bin`）；`.got` 静态全 0，**真实运行（unidbg 模拟 / Frida 真机）才填真值**——**"unidbg 或 frida 跑原版"正是拿 `.got` 真值的方法**（ch13 13.4c unidbg+Frida 一对一对齐）

```
军棋 libjunqi.so(ELF32, 指针 4 字节, .got 32 位槽)
   │
   ▼  不管宿主是 x86_64 还是 arm64
   │
   还是 ELF32(数据位宽没变)
   ├─ 指针 4 字节(std::vector 按 32 位布局)
   ├─ .got 32 位槽(填的是 32 位地址)
   └─ 依赖 32 位库(bundled/libc.so = 32 位 ARM ELF, 310KB)
```
**我们在这个环节能干什么**：
- **只能模拟 32 位 so**：军棋只有 32 位原版，没有 64 位原版 so（红线 §64 位 native）
- **要"64 位 so"只能两条路**：① 人工翻译 32→64（军棋路线 A，14MB 影子库，黑屏）② 简化重写（军棋阶段 2，97KB，玩法不一致）
- **不能干**：让 unidbg 把 32 位 so "自动变 64 位"（unidbg 只转译指令位宽，不改 ELF 类）

### 环节 3：内存映射（SvcMemory 层）—— `mmap`/`munmap`/`mprotect` 是 Linux OS 系统调用（非 ELF 概念）


> **本节核心**：
> 1. SvcMemory 按 PHDR 摆段：r-x（代码）+ rw-（数据）两段 mmap，坐标 + 权限
> 2. 军棋 base=0x40000000、栈 0x40000，段权限摆错一个全乱
> 3. mmap 是「摆家具」，段坐标决定后续所有指针运算的基准
>
> **本节难点**：
> 1. 288MB PROT_NONE 段是 §124 ③ 卡点：unidbg mmap2 mock 处理失败（保护段语义缺失）
> 2. 段坐标错 → RELATIVE 重定位算错 → 程序指针全错；权限错 → 写 .text 崩

**`mmap`/`munmap`/`mprotect` 是什么（你说的"这是 ELF 概念还是 Linux OS 概念"——是 Linux OS 系统调用，非 ELF）**：

| 系统调用 | 干什么 | 跟 ELF 的关系 | 跟 unidbg 的关系 |
|---|---|---|---|
| **`mmap`** | 把文件/内存映射到虚拟地址空间（"给这段代码/数据在内存里分配坐标"） | 读 ELF 的 `PT_LOAD` 段（`p_offset`/`p_vaddr`/`p_memsz`）→ 映射进内存 | unidbg **模拟** `mmap`（`SvcMemory.mmap`，不真调 OS，自己分配 x86 虚拟内存） |
| **`munmap`** | 取消映射（"收回内存"） | 释放 ELF 段 | unidbg 模拟 `munmap` |
| **`mprotect`** | 改已映射内存的权限（r-x↔rw-，"锁/解锁这段"） | ELF 的 `PT_GNU_RELRO` 段决定"重定位后锁哪些"（ch04 4.3b），`mprotect` 执行"锁" | unidbg 模拟 `mprotect`（军棋 RELRO 锁 `.got` 只读） |

**"ELF 声明 + OS 执行"的关系（大白话）**：
- **ELF 文件** = "施工图纸"（声明：`.text` 要 r-x、`.got` 要 rw-、基址 0x40000000）
- **OS 的 `mmap`/`mprotect`** = "施工队"（照图纸把段映射进内存、设权限）
- **unidbg** = "假施工队"（在 x86 上模拟"施工"，不真调 Linux 内核，自己分配虚拟内存 + 模拟权限）

**为什么 unidbg 要模拟这些**：军棋 so 跑在 unidbg（x86 模拟），**不是真 Linux 内核**——so 调 `mmap`/`mprotect`/`munmap` 时，unidbg 的 `SvcMemory` **拦截 + 模拟**（在 x86 虚拟内存里分配，不真调 OS 系统调用）。**这是"模拟环境"跟"真机"的核心差异之一**（真机 dlopen 走内核 `mmap`，unidbg 走 `SvcMemory` 模拟）。

```
军棋 .so + 7 个 32 位依赖库
   │
   ▼  unidbg 的 SvcMemory(Java 的 byte[])
   │
   ├─ .text/.rodata → 映射到虚拟地址 0x40000000(Java 堆里, 不是真 Linux 内存)
   ├─ .got(32 位零值槽) → 重定位时填
   ├─ 栈/堆 → 全在 Java 堆
   └─ 7 个依赖库(libc/libstdc++/libm/liblog/libdl/GL×2) → 从 jar 的 android/lib/armeabi-v7a/ 读入
```
**我们在这个环节能干什么**：
- **挂桩**：`libGLESv1_CM`/`libGLESv2` 无真 GPU → 挂桩函数（`glGetError` 返回 0）
- **重定向**：I/O 路径（`data/config/*.xml`）→ 宿主 `./junqi_data/`（第 16 章）
- **补 `.got`**：32 位零值槽 → 用 docker 真 ARM libc 解析 6 个 GLOB_DAT 填（§R6 已做）
- **不能干**：改 `.got` 位宽（32 位 so 的 `.got` 永远是 32 位槽）
### 环节 4：系统调用拦截（Hook / DalvikVM 层）


> **本节核心**：
> 1. 系统调用拦截：Hook open/read/close + DalvikVM 接 FindClass/GetMethodID
> 2. 军棋 so 调 open("/data/data/com.cnvcs.junqi/files/ui.xml")，PC 上没路径必须映射宿主
> 3. 每缺一个应答 so 崩一次：open 返 -1 → 读不到配置分支；FindClass 返 null → NPE
>
> **本节难点**：
> 1. 系统调用层是补环境第一层，全通才到 Java 方法层（第 11/13 章）
> 2. Hook 系统调用「走通但退」fork：军棋 ad SDK fork 返 0 让 so 走「退」分支不模拟子进程

```
军棋 ARM 代码发系统调用
   │
   ▼  QEMU 拦下来(不走真内核), unidbg Java 端 mock 应答
   │
   ├─ open/read/close(读 config XML) → 返回宿主文件
   ├─ __cxa_atexit(读零 .got) → Hook 返回 0(崩点消除)
   ├─ FindClass("com/baidu/mobads/AdView") → DalvikVM 返 null(去广告)
   └─ 14 个 JNI 入口(OnCreate 等) → 手动构造 JNIEnv* + jobject 哈希
```
**我们在这个环节能干什么**：
- **Hook 任意函数**：改返回值 / 跳过崩点 / 抓参数（第 3/15 章）
- **DalvikVM 模拟 Java 侧**：军棋 7 个 Java 类（App/GLView/...）全在假 VM 里（第 11 章）
- **主动取值**：Hook SPAK 解密函数，喂密文、抓明文（第 19 章）
- **不能干**：绕开 §124 的 3 项阻塞（① 上下文相关 BL 解码 bug ② C++ 对象图 ③ 288MB PROT_NONE mmap）
### 四个环节的关系（一句话串起来）


> **本节核心**：
> 1. 四环节串：CPU 翻译（指令位宽）→ ELF 数据（数据位宽）→ 内存映射 → 系统调用
> 2. 「Java 模拟」4 本质：指令模拟(非真 CPU)/内存模拟(非真 MMU)/文件模拟(非真 fs)/Java 模拟(非真 DalvikVM)
>
> **本节难点**：
> 1. 四环节全对 ≠ 能跑：还差 18 常量 + §124 三项阻塞
> 2. 混淆「指令模拟」和「真跑 ARM」即认为 unidbg 等价真机（不等价，无 GPU/无数据与指令对齐）

![ARM32 在 arm64 宿主的 4 环节原理图（指令位宽变 / ELF 数据位宽不变 / 内存映射 / 系统调用拦截，每环节标"能干什么"）](assets/ch01_arm32_on_arm64.png)



```
环节 1(指令位宽转译) 决定"能不能在宿主 CPU 上跑"  →  宿主 x86_64/arm64 都能
环节 2(ELF 数据位宽) 决定"so 是 32 位还是 64 位"    →  军棋永远 ELF32(没 64 位原版)
环节 3(内存映射)     决定"段/栈/堆/依赖放哪"        →  Java 堆 + 挂桩 + 重定向
环节 4(系统调用拦截) 决定"so 调 libc/Java 谁应答"    →  Hook + DalvikVM mock

军棋卡点分布:
  环节 1 已通(unicorn2 修 PC off-by-2)
  环节 2 卡(没 64 位原版 so → "能玩"需真机 GPU 或简化重写)
  环节 3 已通(挂桩 + 重定向 + .got 填)
  环节 4 卡 §124 的 ②③(C++ 对象图 + PROT_NONE mmap)
```

### "Java 模拟"的 4 个本质（跟"真跑 ARM"的区别）
![Java 模拟三层原理（CPU/内存/系统调用）](assets/ch01_java_sim_principle.png)
1. **不跑真 ARM 指令**——unicorn2 把军棋 ARM32 指令**动态翻译**成宿主 ISA 执行（军棋 **1.43MB/1435156B** ARM 代码"虚拟执行"，比真机慢 10-100 倍）。
2. **内存全在 Java 堆**——军棋 `.so` 的段、栈、堆、`.got` 全映射到 Java 的 `byte[]`/`Pointer`，不是真 Linux `mmap`。真内核的 `PROT_NONE`/`PROT_EXEC` 在 Java 堆里是"假的"（§124 的 ③ 288MB PROT_NONE mmap 处理失败 = unidbg 的 `mmap2` mock 没做对）。
3. **系统调用全 mock**——军棋 ARM 代码发 `open("/data/.../ui.xml")`，QEMU 拦下来，unidbg Java 端决定"返回什么"（I/O 重定向是"Java 端 mock 文件路径"，不是真文件 IO）。
4. **Java 侧全模拟**——`FindClass`/`GetMethodID` 用 `DalvikVM`（Java 写的假 Dalvik VM）应答，军棋 7 个 Java 类全在这个假 VM 里，so 调 Java 方法时是"假 VM 应答"，不是真 Dalvik。

### "运行时动态翻译 + 保存下来 = 转译指令？"（你的追问）


> **本节核心**：
> 1. unidbg 逐指令翻译（指令流保存为影子运行时），非「重编译整个 so」
> 2. 保存下来的是翻译产物（6402 函数翻译体），非原 so；军棋 7428 函数全可翻译
>
> **本节难点**：
> 1. 「转译指令」追问的答案：是逐条翻译 + 保存指令流，不是把 so 重编译成 64 位
> 2. 影子运行时保留 32 位数据（.rodata/.got），指令转 64 位，混合产物

**追问"那把 unidbg 翻译的 aarch64 全存下来不就行了"** → 见 1.4.3（军棋 R6 真做了，14MB 影子库能跑不崩但黑屏，4 个致命坑：① 对象图 32→64 错位（内层，C.1 场景对象图） / 不带运行时值 / 不带 QEMU 模拟状态 / 只存实际执行路径）。

**为什么军棋 PC off-by-2 是"模拟器精度 bug"而非翻译 bug**（§124 关键）：
- dynarmic 翻译 `add rX,pc` 时用了 `(addr+4)&~3` 掩码（对齐到 4 字节）
- 真实 ARM / unicorn2 不掩码（`addr+4`）
- 军棋 `DiagTracePC` 实证：unicorn2 在 0xa37fc 存 `0x159880`（不掩码值，正确）
- §124 新发现的"上下文相关 BL 解码 bug"（剩余阻塞 ①）也是这个原理：unicorn2 对 0x7965a 的 `BL` 指令解码结果**随上下文变化**（隔离环境算对，加载 `.so` 后偏移到 0x6eaea）——模拟器在"有/无 so 映射"两种上下文下行为不一致。
## 1.4.3 为什么不直接"翻译/重编译成 arm64"？（军棋已验证的死路）

**直觉上**：军棋 `libjunqi.so` 是 32 位 ARM，我的 PC 是 x86_64，
"干脆把它翻译成 arm64 / x86_64 二进制不就好了？" 为什么 unidbg 偏要"模拟"而不"转译"？

**追问：那把 unidbg（unicorn2）翻译的 aarch64 指令全存下来不就行了？**
（你的思路：unicorn2 跑 32 位 so → 把翻译成的 aarch64 全存下来 → 存成静态 aarch64 so → 真机直接跑）

**这条路军棋 R6 真做了（`rev64/reverse/src_shadow/`，14MB 影子库，6402 函数翻译体），结果：能跑不崩但游戏板黑屏**。4 个致命坑：

| 坑 | 说明 | 后果 |
|---|---|---|
| ① 对象图 32→64 错位 | 32 位 `std::vector` 12B（三个 32 位指针）→ 翻成 aarch64 变 24B（三个 64 位指针） | 渲染线程读 vector 读到错地址 → 黑屏 |
| ② 不带 `.rodata`/`.got` 运行时值 | 翻译产物只有指令流，不带 18 个 `.text` 常量 / `.got` 重定位值 / 2209 `.rodata` 函数指针 | 必须手动注入（`jq_rodata_plain.h` + `jni_aligned.c`） |
| ③ 不带 QEMU 模拟状态 | 翻译产物依赖 QEMU 模拟寄存器 + 模拟 Java 堆；存成静态 so 加载到真机没有这些 | 真机跑需真实 C++ 运行时兜底（MuMu 能跑但布局错位） |
| ④ 只存实际执行路径 | unidbg 逐条翻译只翻"实际走的分支"；没执行的分支没存 | OnCreate 走 A 存了 A，OnTouch 走 B 没存 B → 崩 |

**所以"存 aarch64"≠"能玩"**：它只能覆盖"翻译时实际执行的路径 + 指令语义"，**不能覆盖"所有分支 + 数据与指令对齐"**。
unidbg 故意不存翻译产物（现场翻译 + 现场执行，指令流跑完就没）——**为了"数据与指令对齐保真"**（32 位布局原样存，不翻成 64 位），代价是慢 10-100 倍 + 卡 §124 三项阻塞。
你们影子库"存下来"是为了真机快跑，但**丢了数据与指令对齐**（32→64 布局错位）——**这就是"指令 100% 对齐 ≠ 能玩"的根因**（红线 §R6 §121/§123/§124）。

**两条"翻译"路线，军棋都试过了**：

### 路线 A：汇编级机械翻译（32 位 ARM → 64 位 arm64）


> **本节核心**：
> 1. 32 位 ARM 汇编 → 64 位 arm64 汇编（机械翻译，纯指令转译）
> 2. 军棋 7428 函数 100% 可翻译 + 编译通过，可跑函数 1174/1174 一致率 100%
> 3. 翻译保留 .rodata/.got 32 位数据，只转指令
>
> **本节难点**：
> 1. 18 个 .text 常量 + §124 三项阻塞是翻译也解不了的（模拟器精度 + 对象图 + mmap）
> 2. 影子运行时在 MuMu 稳定运行 90s+ 不崩（GLThread 在跑），但黑屏（数据与指令对齐分歧）

**"对象图"是什么 + 为什么 32→64 翻译会丢失它**（路线 A 黑屏根因）：

**对象图（object graph）= 堆上 C++ 对象之间的指针拓扑**（不是单个变量，是"一堆对象互相指来指去"的结构）。军棋 `std::vector` 在 32 位内存里：

```
vector 对象 (12B, 32 位布局):
  begin 指针 (4B) ──→ 堆上元素区 [0x40200000]
  end 指针   (4B) ──→ 堆上元素区 [0x40200040]
  capacity (4B) ──→ 容量值 (比如 10)

元素区 (堆上): 0x40200000: [军旗][炸弹][地雷][工兵]...
  每个棋子又是 12B 结构体（rank/颜色/坐标），棋子间又互相引用（吃子/布局关系）
```

**32→64 翻译后，对象图物理布局全变了**：

| 层 | 32 位 so | 64 位翻译体 | 一样吗 |
|---|---|---|---|
| 指令逻辑 | `mov r0,[r4]`（读 32 位） | `mov w0,[x4]`（读 64 位） | 语义对，位宽不同 |
| vector 大小 | 12B（3×4B 指针） | 24B（3×8B 指针） | **不一样** |
| 字段偏移 | begin=+0, end=+4, cap=+8 | begin=+0, end=+8, cap=+16 | **不一样** |
| 元素区地址 | 0x40200000（4B） | 0x700000000000（8B） | **不一样** |

**你们翻译体把 32 位 vector（12B）当成 64 位 vector（24B）去读**——`end` 指针偏移从 `+4` 变 `+8`，**字段全错位**。渲染线程读 `vector[0]` 读到的是 `end` 指针的值，不是元素 → **黑屏**。

**"逻辑一样为什么逆不回来"的 4 层拆解**（回应"变量名 a/b 没意义但逻辑一样"）：

| 层 | 能不能逆 | 军棋现状 |
|---|---|---|
| ① 逻辑（函数内指令） | ✅ 能 | 已对齐 1174/1174 PASS |
| ② 静态数据（.rodata/.data） | ✅ 能 | 已 dump 118KB |
| ③ **对象图（运行时堆指针拓扑）** | ❌ 逆不回来 | **32→64 翻译丢失**（vector 12B→24B 错位） |
| ④ 跨调用副作用（A 写 B 的堆） | ❌ 验不了 | 单函数回归不追 BL 链 |

**关键**：对象图是**运行时堆结构**，**不在 so 文件里**（so 只有函数 + 静态数据，堆对象运行时才创建），**不能静态逆向**。32→64 翻译把它的物理布局变了（字段偏移错位）→ 渲染读错地址 → 黑屏。

**unidbg 为什么能保对象图**：unidbg **不翻译，直接跑 32 位原版**，vector 还是 12B 原样。你们翻译体丢了对象图，是"翻译 32→64"的**固有代价**（不是 bug）。**"逻辑一样 + 数据一样" ≠ "能玩"——还差"对象图一样"，而对象图是 32→64 翻译的固有丢失**（红线 §R6 §121/§123/§124 + §124 ②）。

**指令位宽必须 = 数据位宽（ABI 根本约束）+ "32 位 C 编译 arm64"为什么不行**：

| 数据区元素 | 32 位 | 64 位 | 兼容吗 |
|---|---|---|---|
| 纯 POD（`int`/`float`/`double`/`char`） | 4/4/8/1 B | 4/4/8/1 B | ✅ 位宽不变 |
| **指针**（`void*`/`int*`） | **4B** | **8B** | ❌ **位宽变了** |
| **C++ 容器**（`std::vector`） | **12B**（3×4B 指针） | **24B**（3×8B 指针） | ❌ **大小 + 字段偏移全变** |
| **含指针的结构体**（`int* p; int x;`） | 8B（4+4） | 16B（8+8 含对齐） | ❌ 对齐规则不同 |
| **虚函数类（vtable）** | 多 4B vptr | 多 8B vptr | ❌ 位宽变了 |

**"指令 64 位 + 数据 32 位"的混合方案违反 ABI 根本约束**：64 位指令读 `[x4]` = 读 8 字节，但 32 位 vector 的 `begin` 只占 4B → **64 位指令把 `begin(4B)+end(4B)` 当一个 64 位值读进来 → 完全错位**。

**"32 位 so 逆向出 C → 编译 arm64 不就行了"的两个前提错**：

| 前提 | 为什么错 | 军棋现状 |
|---|---|---|
| ① 逆向出"业务级 C" | 你们 `thumb2c_full.py` 翻的是**指令级 C**（`r0=r4+4` 机械翻译），不是业务级 C（`if(board[x][y]==EMPTY)return true`）。指令级 C 里有**寄存器名 `r0/r1` + 直接内存读写**，**不是合法 64 位 C**，`gcc -m64` 编译不出来 | 7428 函数全是指令级 C |
| ② 就算有"业务级 C" | 编译 arm64 后 `State*` 变 8B 指针，但原版 so 的 `State` 结构体在内存里是 **32 位布局**（字段偏移按 4B 指针算）→ **你的 64 位 C 按 8B 访问 → 字段全错位 → 还是黑屏** | 对象图错位是 32→64 固有代价，跟 C 级别无关 |

**"32 位 C 编译回 32 位"有没有用**：能编，但**编出来的 so ≠ 原版 so**（函数顺序/布局/优化全变了）。价值是**当回归裁判**（验证逆向正确性），不是"还原原版"（redroid dump 要跑原版 so）。军棋没做这件事（成本：7428 函数 + 补全数据 + 整理业务级 C，工作量巨大）。

**唯一"对象图保真"的方案**：unidbg **不翻译，直接跑 32 位原版**（指令 32 位 + 数据 32 位，对象图原样保留）——但卡 §124 三项阻塞（模拟器精度 + 数据与指令对齐未初始化，非翻译问题）。

**"IDA 反汇编 + unidbg 逐函数修正 = 还原"——对，但还原 = ① 已做 + ③④ 待补**（回应"一个一个修正函数不就还原了吗"）：

你们 R6 流水线**确实做了**"IDA 反汇编 → 逐函数回归（unicorn2 当裁判）→ 修到 1174/1174 PASS"，**指令级还原做到了**。但"能玩"还差 2 层没还原：

| 还原层 | 做到吗 | 军棋现状 | 怎么补 |
|---|---|---|---|
| **① 函数体指令逻辑** | ✅ 100% | 1174/1174 PASS（你说的"函数都修正"） | 已做 |
| **② 函数内嵌常量** | ⚠️ 补进来的 | 18 个 `.text` 常量 redroid dump 注入（非逆出） | 已补（工具三件套） |
| **③ 函数依赖的运行时对象图** | ❌ 没对齐 | 32 位布局假设 vs 64 位实际（vector 12B→24B 错位，黑屏根因） | 让 64 位翻译体**按 32 位布局存对象**（`#pragma pack`/手动对齐） |
| **④ 跨函数数据流** | ❌ 没验 | 单函数回归不追 BL 链（A 写 B 的堆没验） | 改追**调用链**（OnCreate 整条链喂真实输入，比终态） |

**关键区分**：**"函数都对齐"（①）≠"程序能玩"**——还差"函数依赖的运行时数据"（③对象图 + ④跨函数）。你们还原了"函数怎么算"，没还原"函数算的对象长什么样 + 函数间数据怎么传"。

**"先 32 位验证再 64 位编译"为什么更稳（你描述的流程）**：

| 验证层 | 32 位 C 编译能验吗 | 64 位 C 编译能验吗 |
|---|---|---|
| ① 函数体指令逻辑 | ✅ | ✅ |
| ③ 对象图布局 | ✅ **验得了**（32 位 C 的 vector 还是 12B，跟原版同布局，动态内容一比就对得上） | ❌ **验不了**（64 位 C 的 vector 变 24B，跟原版 12B 不一致，比了也是假的） |
| ④ 跨函数数据流 | ✅ | ❌（64 位调用链布局全变） |

**关键**：你们 R6 **直接 64 位编译，跳过了"先 32 位验证 ③④"**——32 位 C 跟原版 so 同布局，③④ 能验；64 位 C 布局变了，③④ 验不了。你们 64 位编译用 64 位标准库（vector 24B）→ 对象图错位 → 黑屏。

**正确路径（3 步）**：
1. **32 位 C 验证 ①③④**：反汇编 → 翻译成 32 位 C → 编译 32 位 so → unidbg 跑原版 so 对比动态内容（同布局，③④ 能验）
2. **64 位编译保持 32 位布局**：`#pragma pack(4)` + 自定义 12B vector + 指针强制 4B 对齐（**不用 64 位标准库**）
3. **再验 64 位翻译体**：喂 32 位布局输入，比动态内容

**unidbg 当裁判的边界**：unidbg 跑原版 so 到 0x43ed8 就崩（§124 ② 数据与指令对齐未初始化），**dump 不出完整动态内容**——裁判自己先崩了，只能靠 unicorn2 单函数回归 + redroid dump 18 常量凑。所以"动态对比"实际是"unicorn2 单函数比 + redroid dump 常量注入"，不是"unidbg 整链比"。

**so 层 64 位升级 checklist（只搞 so，Java 层另说）**：

"32 位 C 跑对了"= 指令语义对齐了，但**你们 `thumb2c_full.py` 翻的是"指令级 C"（机械翻译），不是"能直接 `gcc -m64` 编译的业务级 C"**。指令级 C 里有裸指针 + 直接地址算术，`gcc -m64` 后指针自动变 8B、容器变 64 位布局 → 对象图错位。所以 so 层 64 位升级要补 2 步：

| 步骤 | 做什么 | 不做的后果 |
|---|---|---|
| **① C 源码改显式 32 位布局** | 自定义 12B vector（`int32_t*` 指针 + `__attribute__((packed))`）+ 指针全用 `int32_t*`（4B）+ `#pragma pack(4)`，**不用 64 位标准库** | `gcc -m64` 后对象图 12B→24B 错位 → 黑屏（你们影子库现状） |
| **② redroid dump 18 常量注入** | 跑原版 32 位 so → dump 14 个 JNI 入口内嵌的 18 个 `.text` 常量（15 `.text`+2 `.data`=72B，只有 dlopen 才填）→ 注入 64 位翻译体 | 常量槽是零，函数读常量读错 → 崩 |

**两步做完，so 层 64 位升级齐了**（对象图布局 = 32 位原版 + 18 常量填真值）。**Java 层（JNI 入口签名 / DalvikVM 回调）另说**。

**落地检查点（另一会话可执行验证）**：
1. C 源码扫描：`grep -rn "std::vector\|std::string" rev64/reverse/src_shadow/` → 全换自定义 12B 布局类型
2. 指针位宽：确认全用 `int32_t*`/`uint32_t*`（4B），无裸 `int*`（64 位变 8B）
3. 结构体加 `__attribute__((packed))` + `#pragma pack(4)`
4. 18 常量注入：`gen_runtime_table.py` JSON→C 注入表接进 64 位翻译体（`jni_aligned.c` 的 `jq_load_runtime_values`）
5. 回归：`jq_test` harness 生命周期 OnCreate→OnDestroy 不崩
6. 真机验收：MuMu 看游戏板是否渲染（不只"不崩"）

**诚实边界**：① unidbg 当裁判跑到 0x43ed8 崩（§124 ②），动态对比实际 = unicorn2 单函数回归 + redroid dump 18 常量，非 unidbg 整链比 ② 1174/1174 PASS 是"5 构造输入 + 单函数 + 指令语义"一致率，非任意输入/功能一致 ③ 军棋明文 so（非壳），反汇编直接可做。

军棋 R6 阶段真做了（`rev64/reverse/src_shadow/`，产物 `libjunqi_arm64_shadow.so` 14MB）：
- 把原版 32 位 `.text` 逐条指令机械翻译成 arm64 C 代码（6402 函数翻译体）
- 在 x86_64 harness 上跑，生命周期 OnCreate→OnDestroy 通过
- **结果**：能跑不崩，但**游戏板没渲染（黑屏）**——因为 32 位 so 的 C++ 对象图
  （std::vector 等）按 32 位布局假设，翻译成 arm64 后对象语义错位（红线 §R6 §121/§123/§124）
- **结论**：指令 100% 对齐 ≠ 能玩，**对象图物理布局是"翻译"丢掉的**（逻辑/数据都对齐了，③ 对象图逆不回来）

#### 1.4.3★ 路线 A 的活路：32 位 ARM → 32 位 C → 编译回 32 位 armv7a（§124u-z 突破，课程定位纠偏）

**上面"路线 A = 32→64 = 死路"的表述需要纠偏**：死路是"**32→64**"（对象图 12B→24B 错位）。
但军棋 §124u-z 实测出了**路线 A 的活路——"32→32"翻译体**：

```
32 位 ARM 指令（原版 .text）
  → capstone/thumb2c_full.py 机械翻译成 32 位 C（指令级，r0..sp 全寄存器 + Z/N/C/V 标志）
  → armv7a-linux-androideabi24-clang 编译回 **32 位 armv7a .so**（不是 arm64！）
  → MuMu 32 位 ARM 转译（libhoudini）跑起来 → 渲染主菜单 UI 层（标题+三士兵+红旗+5 按钮）
```

**关键认知纠偏**：
| 翻译目标位宽 | 对象图布局 | 军棋结果 | 死/活 |
|---|---|---|---|
| 32→64（arm64） | 12B vector 变 24B，**错位** | 影子库黑屏（§121/§123/§124s） | ❌ 死 |
| **32→32（armv7a）** | **32 位布局原样保留，不错位** | **MuMu 渲染主菜单 UI 层**（§124u） | ✅ 活 |

**为什么 32→32 是活的**：ABI 根本约束（前面讲的"指令位宽=数据位宽"）——32 位指令 + 32 位数据，
vector 还是 12B、指针还是 4B、vtable 还是 4B vptr，**跟原版 so 同布局**，对象图物理布局不丢。
前面"路线 A 死路"的结论**只成立于"翻译成 64 位"**；**翻译成同位宽（32 位）就绕开了对象图错位根因**。

**翻译体在老环境 dlopen 的 3 个编译级坑（§124u 实测，简讲 + 指向细节）**：

> **注**：这 3 个坑是**"翻译体 C 代码怎么编译成 32 位 so + 怎么在老 32 位 Android 环境 dlopen"的 NDK/链接器知识**（不是"逆向"问题，跟 unidbg 模拟跑原版 so 无关）。军棋 §124u-z 翻译体路线踩了这 3 个坑才 dlopen 成功，**详细原理见 §124u-z 实战记录（`rev64/reverse/phases/REVERSE_NOTES_04_shadow_r6.md`），此处只列"坑名 + 一句话"**：

| 坑 | 一句话 | 不修的后果 |
|---|---|---|
| **① `.hash` 旧式**（`-Wl,--hash-style=sysv`） | 老 32 位 Android linker（MuMu 的 libnb/houdini）只认旧式 `.hash`，NDK 24 默认生成新式 `.gnu.hash` 不认 | dlopen 报 "failed to link" |
| **② `libstdc++` 符号缺失**（`jqt_crt.c` 自实现） | 翻译体是 C（无 C++ 静态对象），但 `-shared` 链接引 `crtbegin_so` 要 `__cxa_guard_acquire` 等符号，原版从 `libstdc++` 解析，翻译体无 | dlopen 报 "undefined symbol `__cxa_guard_acquire`" |
| **③ GL 库显式链接**（`-lGLESv1_CM -lGLESv2`） | 军棋 so 依赖 2 个 GL 库，翻译体要显式链接（真机有真 GPU 的 GL 实现） | 调 `glClear` 等 GL 函数时 "undefined symbol" |

**"为什么叫'编译级前置'"**：这 3 个坑跟"逆向 so"无关，是"翻译体怎么编译 + 怎么在老 32 位 Android 环境 dlopen"——**NDK 编译 + 老 32 位 linker（libnb/houdini）的底层知识**。unidbg 路线（模拟跑原版 so）**不用管**这 3 个坑（不需要"编译翻译体"）。

**翻译体内存模型（`jqt_mem.c`，§124x/v/w 三个根因修复沉淀）**：
翻译体不直接读原版 so，而是建一块**可写影子内存 `g_jqt_shadow`**（mmap），把原版镜像 + 重定位 + 常量注入进去，
所有 `MEM(addr)` 访问都走影子。3 个根因：
1. **镜像偏移规范**：`jq_img.bin` 从 `.text` 起点 `0x233e0` 开始（`img[i]=原版[0x233e0+i]`），
   **必须 `memcpy(shadow+0x233e0, img)`**（不是 `+0`），使 `shadow[VA]=原版[VA]`。
   拷到 `+0` 会让所有 `.text`/`.rodata` MEM 读偏移 `+0x233e0` → pthread_mutex_lock 卡死（splash 永不清除）。
2. **MEM 三路命中**（`jqt_mem_valid`）：`a<MAP_HI`（影子偏移）→ `shadow+a`；`a∈影子区` → 直接；
   **`a∈/proc/self/maps 真实映射区`**（malloc/scudo 堆，JNI 对象绝对指针）→ 直接解引用。
   真实映射区表 `JQT_REAL_MAX 4096`（设备 ~1943 条，256 满表漏 scudo 堆）+ `qsort` + 二分。
   **不补这路**：`_Znwj` 返回的真实堆指针被拒落零页读 0 → 链表遍历永不终止（主线程 spin）。
3. **`.data` 段 + RELATIVE 重定位**（`jq_rel_data.h`）：`memcpy(shadow+0x155000, .data 28288B)` +
   每槽 `+= g_jqt_shadow`（影子"load_base"）→ 原版 dlopen 时 linker 给 5312 个 `R_ARM_RELATIVE` 槽加基址的等价物。
   `.data` 全零 → `_Pthread_alloc` 读 `MEM(0x15B8FC)=0` → `pthread_mutex_lock(0)` 崩溃。

**18 地址常量"静态位移"修正（§124z-3，重要认知）**：
16 个 `.text` 常量池是 **PC 相对位移**（原版 `ldr rX,[pc,#N]; add rX,pc`，翻译体把 `add rX,pc` 实现为
`rX=file_off+rX`），故注入值应为**静态位移**（如 `0x00138918`），**非 redroid 运行时绝对地址**
（`0x96a779e4` 等——不同进程基址不同，注入即错）。旧表用 frida-dump 推算的 redroid 绝对值 →
落到 host 随机映射读垃圾 → 主线程 `_ZdlPv(垃圾)` SIGSEGV。修正后值固化在 `jqt_mem.c` 的 `g_jqt_got_tbl`
（静态位移，跨进程通用），**无需运行时 dump**（`jni_runtime_values.json` 路线已废弃，见 `frida-dump/FRIDA_NOTES.md`）。

**调用链获取 + stub + 验证的工程化流水线（§124z 系列，详见 ch05 5.6/§124q）**：
```
gen_render_full.py（翻译渲染链完整调用链）
  → wire_blx_dispatch.py（blx rX 存根 → jqt_blx_dispatch vtable 间接调用，44 处）
  → gen_vtbl.py（vtable 调度器 + 32 个 bx lr 返回桩强制 JQ_RET + PLT 真实化 + ARM thunk 折入）
  → 编译（--hash-style=sysv）→ MuMu 执行 → 看主菜单是否渲染
验证判据：callchain_stats.py（合并边界 seed∪full∪stub，关键路径直接调用缺口=0 → 黑屏根因=对象图非缺函数）
```

**路线 A 活路的"能干什么 / 不能干什么"**：

| 能干什么（✓） | 不能干什么（✗） |
|---|---|
| 32→32 翻译体保对象图布局（vector 12B 原样）→ MuMu 渲染主菜单 UI 层 | 让 GL 棋盘区出帧（渲染循环跑但 glClear 类对象未构造，§124z 黑屏诚实边界） |
| 指令级 100% 对齐 + 调用链关键路径 0 缺口（§124z-5 统计） | "能玩"（点击人机对战未进对局，触摸→对局路径未完整） |
| `.hash` 编译适配老 32 位 linker（libnb/houdini） | 让 64 位 only 设备装（32 位 so，64 only 设备装不了） |
| 影子内存三路命中（偏移/影子/真实堆）不崩 | 让 unidbg 离线重放（翻译体是真机执行，unidbg 是离线模拟，两条独立路） |
| **gate-set 直接置位闸口字节 → F_76b14 全链走通**（§〇d-2，会话 2 后续突破，破"写侧数据缺口"一层） | **slot22（棋盘纹理创建 F_9e2b4）仍 0 命中**（对象图"数据"未填充，非布局错位——gate-set 突破"构造标志未置位"一层，但类 A/B slot0 调用者 + 纹理上传链仍未构造；4 个翻译缺口 F_78890/F_a4594/F_9d5b0/F_9de24 需恢复翻译器） |

**一句话纠偏**：路线 A 不是"死路"，是"**32→64 死、32→32 活**"。军棋 R6 影子库（32→64）黑屏是对象图错位；
§124u-z 翻译体（32→32）渲染出主菜单 UI 层是对象图保真的活路。**"能渲染 UI"≠"能玩"**（GL 出帧 + 触摸进对局还差），
但这是"翻译体路线"迄今最接近"能玩"的实测突破。**★ 后续 §〇d-2 gate-set 进一步突破"写侧数据缺口"一层**（直接置位 0x15be90 闸口字节 → F_76b14→F_76a98→F_782d4→类 B 构造全链走通，工具 `patch_gate_chain.py` 幂等固化），**但"画棋盘"仍需完整 C++ 对象图数据工程 + 真机 arm GPU 验收**（诚实边界不变）。

**"unidbg 能还原数据层吗"的 3 个前提条件（回应"C++ 对象图 + 常量池 unidbg 都有"的追问）**：
unidbg **能还原数据层**（.rodata/.data/.got 全在内存里），但"还原出可用数据"需要 3 个前提**全满足**：

| 前提 | 军棋现状 | 缺了会怎样 |
|---|---|---|
| ① `.got` 重定位完成 | 5312 RELATIVE + 214 PLT + 6 GLOB_DAT 全通（已做） | 读 `.got` 零值 → `__cxa_atexit` 崩 |
| ② 18 常量池填真值 | unidbg 侧：redroid dump 72B 注入（已做）；翻译体侧：静态位移固化 `jqt_mem.c`（已做） | 函数读常量读 0/垃圾 → 崩 |
| ③ **C++ 对象构造完**（vector begin/end 指针填真值） | **卡 §124 ②（std::vector 0x43ed8 未初始化）** → 数据与指令对齐"零值图" | vector 读 0 → 渲染读错地址 → 黑屏 |

**关键**：unidbg 路线的天花板 = **前提 ③（数据与指令对齐构造）**。unidbg 能填 ①②，但 ③ 被 §124 三项阻塞（BL bug/数据与指令对齐语义/mmap）卡死。这就是为什么"unidbg 能跑指令"≠"unidbg 能还原数据与指令对齐"。

**"C 也有对象图"的认知（回应"rust/cpp 最终与汇编没区别，都能翻译成 C，C 也有对象图"）**：
对——**对象图 = 指针（指向堆/栈）+ 函数指针（vtable 槽）**，跟"高级语言的面向对象"无关。C 的对象图 = "这些指针指向的影子内存 + 这些函数指针指向的 F_xxx 函数"。但**翻译体要跑起来，这些指针得先被"构造"**（`_Znwj` 分配堆 + 填 vtable 槽 + 填 begin/end）——**这就是 §124z 的黑屏根因——纹理类 vtable 0x1595c0 的 glClear 方法（slot[13]=0x9d459）所在对象未构造（★ §124zz-3 纠正：旧"glClear 类对象 vtable=0x1595f4"是幻影，0x1595f4 实为 0x1595c0 的 slot[13]）**。

| 概念 | 大白话 | 军棋现状 |
|---|---|---|
| **指令语义**（"函数怎么算"） | 1:1 翻译指令 → C（军棋已做） | ✅ 1174/1174 PASS |
| **对象图**（"函数算的对象长什么样"） | **运行时**才创建的堆指针拓扑（vector 的 begin/end 指向堆上元素区） | ❌ 逆不回来（不在 so 文件里，运行时才创建） |

**"理论上 rust/cpp 最终与汇编没区别，区别在对指令编排及数据层的'语言规则'"**——对。但军棋卡住**不是"语言规则"问题**，是**"对象图构造"问题**（运行时才创建的堆指针拓扑，不在 so 文件里）。所以"翻译成 C"（指令层）够，**但 C 代码里的"对象指针得先被构造"**（运行时，§124z 卡点）。

> **★ 路线 A 的活路 = "32→32 重编译翻译体"（§124u-z，本节下方 `1.4.3★` 详述）**：上面"路线 A = 32→64 = 死路"只成立于"翻译成 64 位"；**军棋 §124u-z 后续实做了"32→32"（编回 32 位 armv7a so → MuMu 转译层真跑）这条活路**——保对象图布局（vector 12B 原样），渲染主菜单 UI 层。完整对照（3 编译坑 / 内存模型三路命中 / 18 常量静态位移 / 调用链流水线 / gate-set 突破）见**下方 `1.4.3★` 小节**。三条路（unidbg / 32→32 翻译体 / R-route 正向重写）对照见 INDEX.md"三条路对照"节 + 附录 C.1。

### 路线 B：用 `binutils-32` 交叉编译（军棋阶段 2 arm64 简化重写）


> **本节核心**：
> 1. binutils-32 交叉编译 32 位 so → 64 位（军棋阶段 2 简化重写）
> 2. rev64/reverse/src/junqi.cpp ~97KB，14 个 JNI 入口（与原版 readelf 实测一致，见 1.4.3★ 手简版说明）+ 军棋棋盘 10×9 简化版
> 3. 不是原版汇编级逆向：玩法/规则/资源与原版不一致（红线 §64 位诚实边界）
>
> **本节难点**：
> 1. 简化版无 GL 依赖（liblog/libm/libandroid），arm64 设备可渲染可交互
> 2. 完整对齐需 32 位 .rodata 明文逻辑移植进 arm64（地雷/炸弹/悔棋规则/蓝牙/SPAK key），工作量另计

军棋阶段 2 真做了（`rev64/reverse/src/junqi.cpp`，`libjunqi_arm64.so` ~97KB）：
- 重新用 C++ 写一个简化版 64 位 so（10×9 棋盘 + 自动走子 + GLES2 渲染）
- **结果**：能在 64 位设备"跑起来"，但玩法/规则/资源**与原版不一致**
- **红线**：这是"简化重写版"，**不是** 32 位 **1.43MB/1435156B** native 的汇编级逆向
- **结论**：重写版只能替代"能装"，不能替代"原版逻辑"（红线 §64 位 native 诚实边界）
**为什么 unidbg 选"模拟"而非"翻译"**：
1. **保真度**：unidbg 模拟的是**原版 32 位 `.so` 本身**（不重写），
   走子/吃子/翻棋规则 = 原版逻辑（军棋 32 位 v7a 在 unidbg 下能跑通 init_array + 重定位）
2. **快**：unidbg 把 32 位 `.so` 直接用 unicorn2 翻译成 x86 动态执行，
   不用"人工重写成 arm64"（路线 B 是人工重写，慢且丢逻辑）
3. **对象图保真**：unidbg 模拟时 `.so` 的内部 C++ 对象（std::vector）
   按 32 位布局原样存在，不像路线 A 翻译成 arm64 后对象错位（黑屏根因）
4. **军棋的"能玩"真机验证**：32 位 so 在 unidbg（unicorn2）下能跑通重定位 + init_array +
   JNIEnv/VM（DiagJni 推进到 std::vector 0x43ed8），但卡在 §124 的 3 项阻塞
   （① 上下文相关 BL 解码 bug ② 数据与指令对齐对应缺口（N44）③ 288MB PROT_NONE mmap）——
   这些是"模拟器精度 + 数据与指令对齐语义"问题，不是"翻译丢逻辑"问题
**一句话**：unidbg = "模拟原版 32 位 so"，路线 A/B = "人工翻译成 arm64"。
军棋验证过翻译是死路（对象图错位 / 简化重写丢逻辑），所以 32 位样本走 unidbg 模拟，
64 位"能玩"靠真机（redroid 无 GPU / MuMu 影子库黑屏——红线 §64 位 native 诚实边界）。
**军棋 OnCreate 调用链实测（§124q，补"能玩"卡点最新数据）**：
- 静态调用图（capstone 提 bl/blx）提 OnCreate 12 目标：**全 so 内部 + PLT，无直接 JNI**（0xbdacc = JNI wrapper，Java 交互在更深层）
- **动态 trace 受 unicorn2 BL 解码 bug 限制**（0x23578 `bl 0xbdacc` 被解码成 0x13c576 → 崩），改用**静态调用图兜**（不受 BL bug 影响）
- **§124p 的 OnCreate JNI 语义重写（直接 NewObject GLView）偏离原版调用链**——原版走 0xbdacc/0x24250 内部函数，功能可行但**调用链不对齐**（详见 ch05 5.4.1）

## 1.5 符号调用 vs 地址调用（军棋的取舍）


> **本节核心**：
> 1. strip 后 .dynsym 只剩 14 个 Java_com_cnvcs_App_On* 导出，只能按地址调
> 2. 14 入口是 Thumb 奇地址（如 0x233e1），调用前必须 &~1 对齐
> 3. 内部函数（翻棋判定等）无名字，需 IDA 静态分析找入口地址再按地址调
>
> **本节难点**：
> 1. findSymbol 只查 14 个 On*，内部函数 findSymbol 返 null → 崩
> 2. 地址调用 4 维度差异（符号表/参数/依赖/C++ 对象图）军棋全比书内复杂

```java
// 符号调用：按导出符号表找函数（军棋 strip 后符号表空，走不通）
Pointer p = memory.findSymbol(module, "Java_com_cnvcs_App_OnCreate", true);
// 地址调用：直接按入口地址跳（军棋 14 个 JNI 入口是 Thumb 奇地址）
// 注意：奇地址要 &~1 对齐到偶指令，否则可达集空（红线）
long onCreateAddr = 0x233e1L;  // 军棋 OnCreate 入口（Thumb 奇地址）
long ret = vm.getMemory().call(emulator, onCreateAddr, 0, jniEnv, objHash).singleValue();
```

**"符号调用最终是不是地址调用？符号怎么跟地址对上？符号是反汇编得到的吗？"（详细讲清，跟 4 本书 ch04 ELF 全字段呼应）**：

**核心澄清**：
- **符号调用最终确实是"按地址调"**——"符号"只是"名字的索引"，调的时候还是"符号 → 查符号表 → 拿地址 → 调地址"
- **符号不是"反汇编得到的"**——符号是**编译/链接阶段生成的**（`.symtab`/`.dynsym` 节里存"符号名 ↔ 地址"映射），不是"反汇编 so 推断出来的"
- **strip 后符号没了**——`.symtab`（完整符号表）被 `strip` 删了，但 `.dynsym`（动态符号表，导出符号）保留（so 要对外导出的函数名必须在，否则 dlopen 找不到）

**"符号 → 地址"的完整链路**（4 本书《二进制分析实战》第 2 章 ELF 节讲全）：
```
编译阶段（gcc/clang）
  → 每个函数生成"符号名"（如 OnCreate/走子判定）
  → 存进 .o 的 .symtab（符号名 ↔ 偏移，比如 "OnCreate" ↔ 0x233e1）

链接阶段（ld）
  → .symtab 合并进 so 的 .symtab（完整符号表，含所有函数/变量名）
  → 导出符号（so 对外暴露的，如 JNI 入口）复制进 .dynsym（动态符号表）
  → .dynsym 跟 .dynstr（字符串表）配对：.dynsym[i].st_name = .dynstr 里的偏移（"OnCreate" 的字符串位置）
  → 每个符号有 st_value（地址，比如 0x233e1）

strip 阶段（release 版）
  → 删 .symtab（内部函数名全没了，省空间）
  → 保留 .dynsym（导出符号必须保留，否则 dlopen 按名字找不到）

运行时（dlopen）
  → Java 侧 RegisterNatives("com.cnvcs.App", {{"OnCreate", 0x233e1}})
  → so 的 JNI_OnLoad 把 14 个 On* 符号名 + 地址注册给 Java
  → Java 调 OnCreate → 按名字查 .dynsym → 拿地址 0x233e1 → 调

unidbg（模拟）
  → "符号调用" = findSymbolByName("OnCreate") → 查 .dynsym → 拿 0x233e1 → call(0x233e1)
  → "地址调用" = call(0x233e1) 直接按地址（strip 后内部函数没名字，只能按地址）
  → 两者最终都是"按地址调"（符号是"名字索引"，地址才是"真正跳的位置"）
```

**"符号怎么跟地址对上"（4 本书 ch04 4.3b 的 ELF 全字段，`.dynsym` 是表）**：
- `.dynsym` 是**表**，每行 = `Sym` 结构（`st_name`/`st_value`/`st_size`/...）
- `st_name` = 字符串在 `.dynstr` 的偏移（"OnCreate" 在 `.dynstr` 第 N 字节）
- `st_value` = **符号的地址**（比如 0x233e1，军棋 `OnCreate` 的 Thumb 奇地址）
- **"符号名 ↔ 地址"对上了**：查 `.dynsym` 找 `st_name=="OnCreate"` → 拿 `st_value=0x233e1` → 调

**"反汇编能拿到符号吗"（跟 capstone/`thumb2c_full.py` 呼应）**：
- **反汇编（capstone/objdump）拿的是"指令 + 地址"**（"0x233e1: push {r4,lr}"），**不直接拿"符号名"**
- **符号名在 `.dynsym`/`.symtab`**（ELF 节，readelf/nm 读，不是反汇编读）
- **strip 后 `.symtab` 删**，反汇编看不到内部函数名（只能看到"0x39e14: push..."，不知道这是"走子判定"）
- **军棋 14 个 `On*` 在 `.dynsym`**（readelf `--dyn-syms` 能拿到名字 + 地址），**内部函数（走子判定 0x39e14）不在 `.dynsym`**（strip 删了 `.symtab`，只能按地址 + 逆向推断功能名）

**"符号调用 vs 地址调用"详细对照**：

| 维度 | 符号调用 | 地址调用 |
|---|---|---|
| **怎么定位函数** | 按名字查 `.dynsym`/`.symtab`（"OnCreate" → 0x233e1） | 直接给地址（"0x233e1"） |
| **strip 后能用吗** | ❌ `.symtab` 删，内部函数没名字（只有 `.dynsym` 的导出符号还能按名调） | ✅ 地址还在（strip 不删地址） |
| **军棋现状** | 14 个 `On*` 在 `.dynsym`（按名能调），内部函数（走子判定 0x39e14）无名字（按名调不了） | 14 个 `On*` + 所有内部函数**都能按地址调**（地址不删） |
| **最终怎么执行** | **查符号表 → 拿地址 → 调地址**（"符号"只是索引） | **直接调地址** |
| **跟反汇编的关系** | 符号名在 `.dynsym`（readelf 读），反汇编拿不到名字 | 反汇编拿"地址 + 指令"（objdump/capstone），军棋 `thumb2c_full.py` 按地址反汇编 7428 函数 |

**关键认知**：**"符号调用"和"地址调用"最终都是"按地址调"**（符号是"名字索引"，查表拿地址）。**区别在"怎么拿到地址"**：
- 符号调用："我知道函数叫 OnCreate" → 查 `.dynsym`（OnCreate 在 0x233e1）→ 调 0x233e1
- 地址调用："我知道函数在 0x39e14"（反汇编/逆向拿到的地址）→ 直接调 0x39e14（这个地址**没有名字**，是逆向推断"这是走子判定"）

**军棋为什么走"地址调用"**：strip 后内部函数（走子判定/翻棋/吃子）**没名字**（`.symtab` 删），**只能按地址调**（反汇编拿到地址 0x39e14 + 逆向推断"这是走子判定"）。14 个 `On*` 有名字（`.dynsym` 保留），**能按名调也能按地址调**（两者等价，地址 0x233e1 等）。

## 1.6 军棋 so 与书内样本的差异（为什么军棋"难搞"）


> **本节核心**：
> 1. 军棋 **1.43MB/1435156B** so vs 书内小样本：7 依赖 + C++ 对象图 + GL 无 GPU + 18 常量
> 2. 军棋 .text 明文（熵 7.10）+ .rodata 明文（0/118832 不同），非壳
> 3. 军棋 14 入口签名不统一（int/jobject/jstring 混），逐个 marshal
>
> **本节难点**：
> 1. 书内样本 7 依赖 3 类重定位「小样本」，军棋全翻上来
> 2. 军棋 12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT 全通（红线 §R6）

| 维度 | 书内样本 | 军棋 `libjunqi.so` |
|---|---|---|
| 符号表 | 完整 | **strip 后空**（只能地址调用） |
| `.got` | 真 linker 填好 | **静态文件是零值**（未重定位） |
| 依赖 | 常见 libc | **7 个依赖含 libGLESv1_CM**（无真 GPU） |
| `.rodata` | 常规 | **明文 118KB**（静态即明文，无自解密，已 dump） |
| `.text` 常量池 | 静态读得出**门牌号**（N45） | **18 个"房间真值"需 dlopen 重定位才填对**（静态读出的是门牌号占位，非房间真值） |
## 1.7 本章小结（一句话版）

> unidbg = 在 x86 PC 上"搭一个 ARM 用户态房间"，让你能调用 so 函数。
> 军棋的"难"在于：符号表空（只能地址调）、`.got` 未重定位（零值崩）、libGLESv1_CM 无真 GPU（挂桩）、18 个 `.text` 常量需 redroid dump。

## 视频
