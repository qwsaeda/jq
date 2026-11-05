# 第30章 Anti-unidbg 系列：unidbg 常规检测总结

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的常规检测清单

## 痛点：为什么"常规检测"是逆向者的"安检关卡"？

军棋 so（以及带风控的 so）启动时会做一堆"环境自检"，目的是判断
"我是跑在真手机上，还是被人用 unidbg/Frida 在 PC 上分析"。
**痛点 = 你越用 unidbg 模拟，特征越多，越容易被查出来 → 走异常分支
（崩 / 返回假数据）。** 军棋实测 3 个卡点。

### 痛点 1：特征太多，逐项查太麻烦

so 能查的特征有 8 个维度（基地址 / JNI / 类 / 文件 / uname / 时间 /
CPU 模拟器 / Frida），逐项查每项都要单独写 mock。

- **大白话卡点**：8 个"安检口"，过每个口要单独准备"随身物品"，
  漏一个就被拦。
- **为什么卡**：特征分散在不同系统调用上，没有统一清单就容易漏。
- **不解决会怎样**：过了 7 个口，第 8 个口（比如 `gettimeofday` 返 0）
  被拦，前面全白做。

### 痛点 2：不知道"哪些特征才是 unidbg 真正暴露的"

军棋 32 位 so 实测 8 个维度里 5 个"本来正常"（基址 0x40000000 跟真机一致、
`.text` 明文、214 PLT 槽真填），真正暴露的只有 2-3 个（`.got` 零值、
dynarmic mask、288MB mmap）——**不知道哪些"不用反"，就会把力气花在
"本来就不暴露"的维度上**。

- **大白话卡点**：8 个口全加固 = 过劳；该加固的不加固 = 被拦。
- **为什么卡**：32 位 so 吃"基址巧合一致"的福利，很多维度天然正常，
  不实测就分不清"要反的"和"不用反的"。
- **不解决会怎样**：资源错配（比如去 mock `uname`，其实军棋 so 根本
  不查 uname；不去 mock 288MB mmap，真卡点没处理）。

### 痛点 3："反检测全通"会被误读成"能玩"

8 个维度全过了（so 没查出来是 unidbg），就以为"so 在 unidbg 上能跑了"。
但军棋实测：反检测全通 ≠ `OnCreate` 跑完——还卡在 §124 的三项阻塞
（bx bug / 全局未初始化 / 288MB mmap）。

- **大白话卡点**："没被查出"和"能完整跑"是两回事。
- **为什么卡**：反检测处理的是"so 的自检逻辑"，不处理"so 的执行深度"
  （对象图语义 / 大内存映射）。
- **不解决会怎样**：把"反检测通过"写进交付物当"能玩"——军棋红线
  明确禁止（"指令能跑 ≠ 能玩"，"简化版不要说成完整版"）。

## 为什么这样做：8 维度"查 vs 反"矩阵（逐项过）

把 8 个检测点列成矩阵，每个点：
1. **so 查什么**（特征）
2. **特征为什么暴露"是 unidbg"**
3. **unidbg 怎么反**（规避）

类比：so 是"安检员"，矩阵是"8 个安检口"——每个口查什么、你随身带的
什么会暴露、怎么改随身物品才过安检。

**预判追问："unidbg 怎么填 `.got` 的值？是 unidbg 自己算的吗？"**
不是"算"的，是"填"的，且三类值来源不同（军棋实测口径，22/28 章同源）：
- **`.got` 214 PLT + 6 GLOB_DAT**：unidbg 加载时自动重定位（`loadLibrary`
  跑完填好），填的是 **unidbg 模拟侧的地址**（AndroidResolver(23) 的
  5 个系统库桩 + 挂桩 GL）——值"存在"但不是真机 libc 地址。
  军棋 so 本身不做"反查 libc 段"的深度校验（非风控 so），填通即过。
- **18 个 `.text` 常量 + 2 个 `.data`（共 72B）**：真值来自
  **redroid 跑原版 so 后 dump**（dlopen 重定位才填），unidbg 算不出
  （静态 `.so` 里是零/占位）→ `jq_load_runtime_values` 注入。
- **vector 的 begin/end**：不填"真值"（真实堆布局拿不到），vecfix 填
  scratch = 走空路径的安全阀（25 章）。

**预判追问："换 arm64 宿主机 / 64 位 unidbg 有没有用？"** 对军棋 32 位
so **没用**（28 章追问的结论）：军棋 so 是 **ELF32**（`readelf -h` 实测），
"指令位宽 vs ELF 数据位宽"是两个独立维度——换 arm64 宿主只改指令位宽，
不改 ELF 数据位宽，64 位 unidbg 加载 ELF32 so 直接架构不符被拒。
军棋走 64 位模拟的唯一路是"arm64 影子库"（rev64 简化重写版，非原版
汇编级逆向，红线 §64 位 native 的诚实边界）。

**8 个安检口**（军棋 32 位 so 实测）：
1. **基地址**（.so 加载在哪个虚拟地址）
2. **JNI 调用**（14 个 On* 方法调通没）
3. **类检测**（`FindClass` 查哪些类）
4. **文件描述符**（`open`/`read`/`close` 通不通）
5. **`uname`**（是不是 `armv7l`）
6. **运行时间**（时间戳合不合理）
7. **检测 Unicorn**（CPU 模拟器特征）
8. **检测 Frida**（`LD_PRELOAD` 带 frida 没）

## 30.1 检测说明（军棋 8 维度矩阵）


> **本节核心**：
> 1. 军棋 8 维度矩阵（查 vs 反）
>
> **本节难点**：
> 1. 基址/JNI/类/fd/uname/时间/Unicorn/Frida 逐项过
> 2. 「反检测全通」≠「能玩」（还卡 §124）
> 3. 8 维度全通是「能加载 + 反检测」，「能玩」需 redroid + 真 GPU

![军棋 unidbg 常规检测 8 维度矩阵](assets/ch30_detect_matrix.png)



军棋 32 位 so 的 unidbg 常规检测 8 维度（红线 §R6 + 军棋实测）：

| 维度 | so 查什么 | 军棋特征 | unidbg 规避 | 大白话 |
|---|---|---|---|---|
| 30.2 基地址 | `.so` 加载在哪个虚拟地址 | 默认 `0x40000000` | 真机 Bionic 也是 `0x40000000`，**正常**（不用改） | "你住几层楼" |
| 30.3 JNI 调用 | 14 个 On* 方法调通没 | 14 入口 + 14 On* | `setJni` 模拟全部（DiagJni 实证） | "你的电话打得通吗" |
| 30.4 类检测 | `FindClass` 查哪些类 | `com.cnvcs.App` 等 7 类 + AdView | `resolveClass` 模拟（AdView 返 null 去广告） | "查你的身份证" |
| 30.5 文件描述符 | `open`/`read`/`close` 通不通 | 读 `data/config/*.xml` | 系统调用 mock + I/O 重定向 | "查你的门牌号" |
| 30.6 `uname` | 是不是 `armv7l` | 检测 `machine=armv7l` | `uname` hook 返回 `armv7l` | "查你的系统版本" |
| 30.7 运行时间 | 时间戳合不合理 | 检测 `gettimeofday` | mock 合理时间戳（非 0） | "查你啥时候进的" |
| 30.8 检测 Unicorn | CPU 模拟器特征 | dynarmic PC off-by-2 | **切 unicorn2 后端**（已实证修复） | "查你是不是被模拟了" |
| 30.9 检测 Frida | `LD_PRELOAD` 带 frida 没 | 检测 `LD_PRELOAD` | `LD_PRELOAD=""` 绕过 | "查你有没有 hook 工具" |

**逐环节"我们能干什么 / 不能干什么"**：

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **基地址（30.2）** | 32 位 so 吃"0x40000000 跟真机一致"的福利，不用反 | 64 位 so 的 0x7xxx ASLR 基址（unidbg 默认值直接暴露，要 mock ASLR） |
| **JNI/类（30.3/30.4）** | `setJni` 14 入口 + 7 类 + AdView null 全通 | 靠 8 维度反检测突破 §124 三项阻塞（bx bug / 全局未初始化 / 288MB mmap 与检测无关） |
| **Unicorn 特征（30.8）** | 切 unicorn2 修 PC off-by-2（DiagTracePC 0xa37fc=0x159880 实证） | 修 §124c 的 init_array 后 bx bug（独立 bug，unicorn2 修不了，军棋用 code cave + no-init_array 绕过） |
| **Frida（30.9）** | `LD_PRELOAD=""` + mock `/proc/self/maps` | 完全消除 unidbg 特征（288MB mmap 等"执行深度"特征仍在，反检测只处理"so 自检"） |

## 30.2 基地址检测（军棋：正常，不用改）


> **本节核心**：
> 1. 基地址检测（军棋：正常，不用改）
>
> **本节难点**：
> 1. 军棋 base=0x40000000 是真 Android 基址（非 0 分析指纹）
> 2. 「基地址」是 unidbg 默认指纹
> 3. 军棋 base 0x40000000 模拟真 Android ASLR，不用改

军棋 32 位 so 的基地址（红线 §R6）：
- unidbg 默认 `0x40000000`
- **真机 Bionic linker 也是 `0x40000000`**（32 位 ARM 默认 ASLR 基址）
- 军棋 so 检测基址 = `0x40000000`（**正常特征**，不是 unidbg 暴露点）
**为什么不用改？** 因为 unidbg 默认基址跟真机一致。如果 so 检测
"基址 ≠ 0x40000000 就崩"，unidbg 默认值刚好躲过去。**这是个"巧合的
福利"**——32 位 ARM 的 Bionic linker 和 unidbg 都用 `0x40000000`，
so 检测不到异常。
**对比 64 位**：arm64 真机基址是 `0x7xxx_xxxx`（随机 ASLR），
unidbg arm64 默认 `0x40000000` 就不一样，arm64 检测基址会暴露。
军棋 32 位没这个问题。

### 30.1b 吸收自 4 本书（揉合去重）：二进制加固全谱系（RELRO/PIE/NX/ASLR/Canary/FORTIFY）

> **本节来源**：取《Linux 系统安全基础》Ch6.1（加固全谱系最全，含 ASCII-Armor，817 页教材第 6 章）。详见 `ELF_FORMAT_SUPPLEMENT.md` 第六节。

**30.1 的 8 维度是"检测项"，本节补"加固机制全谱系"**（so 查加固作反检测检测点）：

| 加固机制 | 原理 | gcc 选项 | 军棋 so | 课程通用 |
|---|---|---|---|---|
| **NX**（6.1.3） | 栈不可执行（防 shellcode 注入栈） | `-z noexecstack` | 军棋栈 NX | ✅ unidbg 模拟时 mock NX |
| **Canary 栈保护**（6.1.4） | 栈帧塞随机值，溢出先改 canary（非返回地址）→ 检测到溢出退出 | `-fstack-protector` | 军棋 so 编译时开 | ✅ 反检测维度（so 查 canary 在不在） |
| **RELRO 部分**（6.1.5） | 重定位后 `.got` 改只读（但 `.got.plt` 仍可写，延迟绑定） | `-z relro` | 军棋 `.got` rw-（未锁，unidbg 未跑完重定位） | ✅ 跟 ch04 `PT_GNU_RELRO` 呼应 |
| **RELRO 完全**（6.1.5） | 重定位时全解析（非延迟绑定）+ `.got` 全只读 | `-z relro -z now` | 军棋走延迟绑定（PLT 214，部分 RELRO） | ✅ |
| **PIE**（6.1.7） | 代码段随机基址（ASLR 配合）→ 攻击者难定位 | `-pie` | **军棋 so 是 ET_DYN（PIE，dlopen 时随机基址）** | ✅ **跟"18 常量静态位移"关联（基址随机，值用位移非绝对）** |
| **ASLR**（6.1.6） | 堆/栈/共享库映射随机化 | （内核 + 链接器配合） | 军棋 so 在 ASLR 下基址随机 | ✅ |
| **FORTIFY**（6.1.11） | 编译时把 `memcpy`/`sprintf` 换强检查版本 | `-D_FORTIFY_SOURCE` | 军棋 so 编译时可能开 | ✅ 反检测维度 |
| **ASCII-Armor**（6.1.12） | 在可执行段前插 ASCII 字符（防跳板代码） | 自定义 | 军棋无 | ✅（军棋不用） |

**军棋 "18 常量静态位移"跟 PIE/ASLR 的关系**（ch01 1.4.3★ 已讲，此处补强）：
- 军棋 so 是 **PIE**（ET_DYN，dlopen 时随机基址）+ **ASLR**
- 18 个 `.text` 常量池是 **PC 相对位移**（`ldr rX,[pc,#N]`）→ **注入值用静态位移**（`0x00138918` 类），**不用绝对地址**（基址随机，绝对值每次不同）
- 这就是 `jqt_mem.c` `g_jq_got_tbl` 用**静态位移**（跨进程通用）而非 redroid dump 绝对值的原因（§124z-3）
- **课程通用**：跑 PIE so 时，所有"常量注入"都得用**静态位移**（不能用绝对地址），否则 ASLR 随机化后值全错

**漏洞利用提一句（课程通用，军棋跳过）**：《Linux 系统安全基础》6.2 讲 ret2shellcode/ret2libc/ROP/`.got` 覆盖——这些是"攻击者利用溢出"场景，军棋明文非壳无利用场景，**课程提一句"加固全谱系 = 防这些利用"**即可，不展开。

## 30.3 JNI 环境之 JNI 调用（军棋 14 入口 + 14 On*）


> **本节核心**：
> 1. JNI 调用（军棋 14 入口 + 14 On*）
>
> **本节难点**：
> 1. 14 入口方法号正常应答；无 OnResize/OnQuit/OnSave/OnRestore
> 2. 「JNI 调用」是 14 入口方法号应答
> 3. 军棋 14 入口 readelf 实测名必须用对

军棋 32 位 so 的 JNI 调用（红线 §R6）：
- 14 个 JNI 入口（`Java_com_cnvcs_App_On*`，readelf 实测）：
  ```
  OnCreate / OnDestroy / OnResult / OnResume / OnEvent
  OnKey / OnUpdate / OnGraphChange / OnBluetooth / OnPause
  OnGraphCreate / OnTimer / OnInput / OnTouch
  ```
- unidbg `setJni` 模拟全部（军棋 DiagJni 实证：`createDalvikVM +
  resolveClass + callStaticJniMethodObject`；§124f JunqiJni 全面 handler
  处理 37+ JNI 调用）
**so 怎么查"JNI 调通没"？** so 启动时调 `FindClass` + `GetMethodID`，
查"14 个 On* 方法注册了没"。没注册（返回 0）→ so 怀疑 JNI 环境不完整
→ 走异常分支。unidbg 的 `setJni` 把 14 个方法都注册好，so 查完正常。
## 30.4 JNI 环境之类检测（军棋 7 类 + AdView）


> **本节核心**：
> 1. 类检测（军棋 7 类 + AdView）
>
> **本节难点**：
> 1. 7 类 resolveClass；AdView 返 null 去广告
> 2. 「类检测」是 7 类 + AdView 应答
> 3. 军棋 7 类（App/GLView/SiteView/AdLis/BtReceiver/TextDlg/InputDlg）

军棋 32 位 so 的类检测（红线 §R6）：
- `com.cnvcs.junqi`（军棋包名）
- `com.cnvcs.App` / `GLView` / `SiteView` / `AdLis` / `BtReceiver` /
  `TextDlg` / `InputDlg`（~7 个 Java 类，军棋 smali 层）
- `com/baidu/mobads/AdView`（广告层，拆除目标）
**so 怎么查"类检测"？** so 调 `FindClass` 查一堆类，看"这些类在不在"。
- 军棋 7 个 Java 类：`resolveClass` 模拟正常返回
- `com/baidu/mobads/AdView`：`resolveClass` 返回 null（去广告）
**关键**：类检测跟"JNI 调用"配合——14 个 On* 方法注册在 `com.cnvcs.App`
上，so 查"App 类 + 14 个方法都在" → 正常；"App 类在但方法没注册" → 异常。
## 30.5 文件描述符（军棋读 config XML）


> **本节核心**：
> 1. fd（军棋读 config XML）
>
> **本节难点**：
> 1. 7 config XML + one.sjg；fd 正常
> 2. 「fd」是 I/O 路径应答
> 3. 军棋 I/O 路径清单（红线 §R6 dump）

军棋 32 位 so 的文件描述符（红线 §R6）：
- `open` / `read` / `close`（读 `data/config/*.xml` 7 个配置文件）
- `mmap` / `munmap`（加载 so 段）
- 军棋 so 检测 fd 表（`/proc/self/fd`，看开了哪些文件）
**so 怎么查"文件描述符"？** so 读 `/proc/self/fd`（当前进程打开的文件
列表），看"有没有异常 fd"（比如 unidbg 开的 mock fd）。
unidbg 规避：I/O 重定向（虚拟路径 → 宿主目录，16 章），fd 看起来跟真机一样。
## 30.6 `uname`（军棋查系统版本）

军棋 32 位 so 的 `uname` 检测（红线 §R6）：
- 检测 `machine = armv7l`（32 位 ARM 正常）
- 检测 `sysname = Linux`（Bionic 正常）
- unidbg mock：`uname` hook 返回 `armv7l` + `Linux`

**so 怎么查 `uname`？** so 调 `uname()` 系统调用，返回结构体
（`sysname`/`nodename`/`release`/`version`/`machine`/`domainname`）。
- 真机 32 位 ARM：`machine=armv7l` + `sysname=Linux`
- unidbg 模拟环境有时 `uname` 返回空 / 全 0 → so 怀疑

**规避**：hook `uname` 系统调用，返回"真机 32 位 ARM 的样子"
（`armv7l` + `Linux`）。

## 30.7 运行时间检测（军棋查时间戳）


> **本节核心**：
> 1. 运行时间（军棋查时间戳）
>
> **本节难点**：
> 1. 军棋 mock 时间戳（防「运行太快」检测）
> 2. 「运行时间」是反检测维度
> 3. unidbg 模拟运行比真机快，时间戳异常会被检测

军棋 32 位 so 的运行时间检测（红线 §R6）：
- `gettimeofday` / `clock_gettime`（拿当前时间戳）
- 检测时间戳是否合理（反 unidbg 时间戳 0）
- unidbg mock：`gettimeofday` hook 返回合理时间戳
**so 怎么查"运行时间"？** so 启动时拿时间戳，检查"是否合理"：
- 真机：时间戳 = 当前 Unix 时间
- unidbg 模拟：时间戳可能是 0（没初始化）或固定值 → so 怀疑
**规避**：hook `gettimeofday` 返回"当前真实 Unix 时间"（跟真机一样），
让 so 拿到合理时间戳。
## 30.8 检测 Unicorn（军棋：切 unicorn2 后端已规避 PC off-by-2）


> **本节核心**：
> 1. 检测 Unicorn（军棋切 unicorn2 已规避 PC off-by-2）
>
> **本节难点**：
> 1. dynarmic PC off-by-2 是 Unicorn 指纹；unicorn2 修（0xa37fc 实证）
> 2. 「检测 Unicorn」是后端指纹
> 3. 军棋切 unicorn2 修 PC off-by-2（红线 §R6）

军棋 32 位 so 的 CPU 模拟器特征检测（红线 §R6 已实证）：
- dynarmic 后端有 PC off-by-2 bug（`add rX, pc` 用 `(addr+4)&~3` mask，
  与真实 ARM 差 2 字节）
- **unicorn2 后端修掉**（用 `(addr+4)` 不 mask，与真实 ARM 一致）
- 军棋 `DiagTracePC` 实证 0xa37fc 存 0x159880 = 不 mask 值
  （unicorn2 的正确行为）→ 11 个 MISMATCH 函数归零（红线 §122）
**so 怎么查"是不是被 CPU 模拟了"？** so 执行一条 `add rX, pc` 指令，
检查 `rX` 算出来的地址对不对：
- 真 ARM / unicorn2：`rX = addr + 4`（正确）
- dynarmic：`rX = (addr + 4) & ~3`（差 2 字节，PC off-by-2）
**规避**：切 unicorn2 后端（军棋已做，DiagTracePC 实证通过）。
**注意 §124 的两代"分支 bug"区分**：
1. **PC off-by-2（dynarmic）**：`add rX,pc` 用 mask，unicorn2 已修
   （30.8 这一维度处理的就是它）。
2. **init_array 后 bx bug（§124c，unicorn2 自身）**：跑完 12 个
   init_array 后，任意分支指令（bx r3/b.w/ldr pc）跳错目标
   （0xfff1a33c / WRITE_UNMAPPED）——**这不是 PC off-by-2，
   是 unicorn2 的全局分支损坏**（0.9.8/0.9.9 都复现，非版本问题）。
   军棋的应对不是"反检测"，而是 **no-init_array 路径 + code cave
   trampoline**（§124d-g，25 章）：跳过 init_array 避开 bx bug，
   代价是全局 C++ 对象未初始化（§124g），两条路互为死锁。
## 30.9 检测 Frida（军棋：`LD_PRELOAD=""` 绕过）


> **本节核心**：
> 1. 检测 Frida（军棋 LD_PRELOAD="" 绕过）
>
> **本节难点**：
> 1. 军棋 so 查 LD_PRELOAD 非空 = Frida；mock 空绕过
> 2. 「检测 Frida」是 LD_PRELOAD 指纹
> 3. 军棋 mock LD_PRELOAD 空绕过 Frida 检测

军棋 32 位 so 的 Frida 检测（红线 §R6）：
- `LD_PRELOAD` 检测（`/proc/self/maps` 找 frida-server）
- `open` / `read` 系统调用追踪（Frida hook 特征）
- unidbg 规避：`LD_PRELOAD=""` + 系统调用 mock
**so 怎么查"是不是被 Frida hook 了"？** so 读 `/proc/self/maps`
（当前进程加载的内存映射），找 `frida-server` / `frida-gadget` 的
加载痕迹。
- 真机（没 Frida）：maps 里没有 frida → 正常
- Frida 注入：maps 里有 `frida-gadget.so` → so 查到 → 走异常分支
**规避**：
1. `LD_PRELOAD=""`（27 章）——不让 Frida 库被预加载
2. mock `/proc/self/maps` 内容——把 frida 那行删掉
3. 系统调用 mock——`open("/proc/self/maps")` 返回"干净"的内容
## 本章小结（一句话版）

> 常规检测 8 维度 = so 的"8 个安检口"（基地址 / JNI 调用 / 类检测 /
> 文件描述符 / `uname` / 运行时间 / 检测 Unicorn / 检测 Frida）。
> 军棋实测：基地址 0x40000000 正常（32 位巧合福利，不用改），
> JNI/类/FD/uname/时间用 mock，**检测 Unicorn 靠切 unicorn2 后端**
> （修 PC off-by-2，DiagTracePC 0xa37fc=0x159880 实证），
> 检测 Frida 靠 `LD_PRELOAD=""` + mock `/proc/self/maps`。
> **反检测全通 ≠ 能玩**：§124 的三项阻塞（init_array 后 bx bug /
> no-init_array 全局未初始化 / 288MB PROT_NONE mmap）在 8 维度之外，
> 反检测处理的是"so 的自检"，不是"so 的执行深度"。

## 视频
