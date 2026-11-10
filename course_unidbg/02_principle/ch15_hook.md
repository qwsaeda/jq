# 第15章 unidbg 源码解析：Hook

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的 Hook / Patch

## 痛点：为什么"不改 so 字节"也能让它跑通？

so 崩了，改 so 本体要重签名、破坏原版。军棋实测用 Hook（拦截）解决 3 个坑。

### 痛点 1：so 读了个零指针就崩，但你不该改 so 本体

军棋 `.got` 未重定位（零值槽，红线 §R6），`__cxa_atexit`（C++ 析构注册函数）
一执行就读 0 指针 → SIGSEGV（段错误）。改 so 字节要重新签名、破坏原版；
更好的办法是"拦住这个调用、返回 0 跳过"——这就是 Hook（拦截）。

- **大白话卡点**：`__cxa_atexit` 走 `.got` 读 0 指针就崩，但你不想改 so 本体。
  在"路口"拦住它、喂 0 跳过，比改 so 字节干净。
- **为什么卡**：`.got` 零值槽读 0 → 段错误；改 so 要重签名破坏原版。
- **不解决会怎样**：`__cxa_atexit` 一执行就段错误。

### 痛点 2：广告层引用删不掉，但能"喂假答案"

军棋 native 层 `FindClass("com/baidu/mobads/AdView")` 直接引用广告类
（红线：全删 baidu smali 前必须处理这个引用点）。全删前，模拟侧可以 Hook
`FindClass` 返回 null，让 so"以为找不到广告类"→ 跳过广告初始化，不崩。

- **大白话卡点**：广告类引用删不掉（so 本体引用它），但能 Hook `FindClass`
  喂 null，让 so"以为找不到"→ 跳过广告。
- **为什么卡**：全删 baidu smali 前不处理这个引用，so 崩。
- **不解决会怎样**：`FindClass(AdView)` 返回真类 → 初始化广告 → 广告浮层。

### 痛点 3：崩了不知道崩在哪，缺"慢动作回放"

军棋 unidbg 路线早期 PC off-by-2 bug（dynarmic 的坑），要定位"哪条指令存错值"，
得能单步、看寄存器、追踪指令流——这就是 Debugger（调试器，类比行车记录仪 + 慢动作回放）。

- **大白话卡点**：崩了不知道崩在哪条指令，得"装行车记录仪"（Debugger）单步回放。
- **为什么卡**：黑盒模拟看不到内部指令流，得追踪/单步。
- **不解决会怎样**：PC off-by-2 这类 bug 定位不了。

## 为什么这样做：Hook = "在关键路口设卡"，Debugger = "装行车记录仪"

- **Hook（设卡拦截）**：在 so 调用的"路口"（某个函数地址）设卡，车辆（指令执行流）
  一到就拦下，喂假答案放行，不真执行。军棋两大卡口：`__cxa_atexit`（喂 0 跳过 C++
  析构注册）、`FindClass`（喂 null 跳过广告类查找）。
  - 类比：高速公路收费站——你不去真的收费站（真执行 libc），卡口直接放行（返回 mock 值）。
- **Debugger（行车记录仪）**：装四个"镜头"（InterruptHook 拦截系统调用 / CodeTrace
  追踪指令 / MemoryTrace 追踪内存读写 / Console 交互式调试），出问题慢动作回放。
  军棋实证：`DiagTracePC` 拍到 0xa37fc 存 0x159880（unicorn2 修复 PC off-by-2 的铁证）、
  `DiagJni` 拍到 `OnCreate` 推进到 `std::vector` 0x43ed8。

**为什么 Hook 优先于 Patch（改指令）？** 军棋红线 §R6 强调"要么纯指令翻译，要么反汇编
成 C，函数级回归"——Patch 会破坏原版指令流，是最后手段。Hook 保留原逻辑（只拦截
应答），军棋 `.text` 明文 100% 对齐（1174/1174 PASS），基本不 patch。

**为什么军棋两大卡口选 Hook 不选 Patch？** `__cxa_atexit`/`FindClass` 都是"调外部函数"，
Hook 在跳板处拦下喂假值即可，不用改 so 指令；Patch 要改 `.text` 字节（破坏原版 + 重签名），
军棋 `.text` 已 100% 对齐，Patch 反而破坏对齐成果。

**预判追问："为什么不用 LD_PRELOAD 拦截（像真机那样），要改 unidbg 代码？"**
真机的 LD_PRELOAD 靠"内核 dlopen 时先加载 preload 库，符号解析时优先用 preload 的"。
unidbg 模拟的是"用户态"，没有真内核 dlopen，LD_PRELOAD 机制不适用。unidbg 的拦截
是 Java 端 mock：在 unicorn2 的指令流里挂 hook（`hook_add`），函数一调就拦下喂假值。
本质区别：真机 LD_PRELOAD = "内核符号解析层"，unidbg Hook = "指令流拦截层"（更底层，
直接拦 ARM 指令，连符号解析都不走）。

### 15.0 吸收自 4 本书（揉合去重）：二进制插桩 SBI/DBI + 代码注入 + 反模拟检测

> **本节来源**：插桩取《二进制分析实战》Ch9（SBI 2 方法 + DBI Pin 最全）、代码注入取《二进制分析实战》Ch7（LD_PRELOAD/GOT/PLT 劫持）、反模拟检测取《Linux 二进制分析》Ch5.7（检测模拟器 3 手段）。详见 `ELF_FORMAT_SUPPLEMENT.md` 第七/八节。

**二进制插桩 SBI vs DBI**（《二进制分析实战》9.1.2，补 15.1 的"Hook 底层机制"）：

| 类型 | 原理 | 军棋对应 |
|---|---|---|
| **SBI（静态插桩）** | 永久改磁盘二进制（重写 `.text` 前 N 字节塞跳板/int3），不破坏引用 | 军棋 Patch（ch03 3.5，改 `.text` 字节） |
| **DBI（动态插桩）** | 运行时监视指令流，插桩代码注入指令流（不改磁盘），Pin 引擎 | 军棋 unidbg `traceCode`（动态 trace） |

**SBI 的 2 种实现**（《二进制分析实战》9.2）：
- **int3 方法**：函数前塞 `int 3`（x86）/断点指令 → 调试器断在插桩点 → 跳板（trampoline）恢复原指令继续跑。**缺点**：跟"正在调试"的程序不兼容
- **跳板方法**（trampoline）：复制原代码到跳板区，**跳板区里插桩**（原代码不动），用 `jmp` 把原代码重定向到跳板副本。**优点**：不破坏原引用（原代码地址不变）

**军棋 unidbg Hook = "跳板方法"**（15.1.2 讲的"改 PLT 跳板"）：改 `.text` 前 N 字节塞跳板，原代码复制到 trampoline——跟《二进制分析实战》第 9 章原理一致。

**代码注入/钩子 5 种**（《二进制分析实战》第 7 章，补"Hook 能拦的位置"）：
| 注入方式 | 原理 | 军棋对应 |
|---|---|---|
| `LD_PRELOAD`（7.2） | 内核 dlopen 时先加载 preload 库，符号解析优先用 preload 的 | 军棋 unidbg 不用（无真内核，15 章"为什么不用 LD_PRELOAD"） |
| 注入代码节（7.3） | 往 so 里插新 `.text` 节 + 改入口 | 军棋不用（so 非壳，不需注入） |
| 劫持构造函数（7.4.2） | 改 `.init_array` 让自定义构造先跑 | 军棋 12 init_array 全通（ch7 7.2） |
| **劫持 GOT 条目**（7.4.3） | **改 `.got` 槽指向自定义函数（GOT hook）** | **军棋 xHook GOT hook（ch28）** |
| **劫持 PLT 条目**（7.4.4） | **改 PLT 跳板指向自定义函数（PLT hook）** | **军棋 214 PLT 跳板可 Hook（15.1.2）** |

**反模拟检测 3 手段**（《Linux 二进制分析》5.7，补"so 查'是不是模拟器'怎么查"，跟 ch27 反检测 8 维度呼应）：
| 手段 | 原理 | 军棋反检测 8 维度 |
|---|---|---|
| 系统调用检测（5.7.1） | so 调 `uname`/`syscall` 查内核特征（`/proc/self/maps` 找 `libunicorn`/`frida` 模块） | "Unicorn/Frida 检测"项 |
| CPU 不一致检测（5.7.2） | 查 `/proc/cpuinfo`（模拟器 CPU 型号 vs 真机） | "uname"项 |
| 指令时延检测（5.7.3） | 特定指令间计时（模拟器比真机慢） | "时间"项 |

**军棋 32 位 so 反检测 8 维度**（AGENTS.md 红线）：基址/JNI/类/fd/uname/时间/Unicorn/Frida——**"Unicorn/Frida 检测"正对应存根反模拟检测**（so 查"是不是在 Unicorn/Frida 里跑"，查 `/proc/self/maps` 的 `libunicorn`/`frida` 模块 + 查 Unicorn 的 `svc` 指令特征）。

**课程通用价值**：跑壳/加固 so 时，存根的反模拟检测（查 Unicorn/Frida/ptrace）是**最大障碍**——so 查 `/proc/self/maps` 找 `libunicorn.so`/`frida-server` 模块，找到就拒绝跑。unidbg 要 mock `/proc/self/maps`（隐藏 Unicorn 模块）+ mock `ptrace`（返 EPERM）才能过。军棋 so 非壳但有反检测 8 维度（广告层 + 加固），课程 ch27/ch28 补"反模拟检测"这条。

## 15.1 unidbg 的 Hook 框架
![军棋 Hook 两大场景](assets/ch15_hook_scenes.png)
### 15.1.1 Hook 框架的使用（军棋两大卡口）


> **本节核心**：
> 1. 军棋两大卡口：__cxa_atexit（Hook 返 0）+ FindClass AdView（Hook 返 null）
>
> **本节难点**：
> 1. Hook 保留逻辑（只是应答）；缺一个崩
> 2. 两大卡口是军棋补环境核心
> 3. Hook 优先于 Patch（军棋 .text 100% 对齐不 patch）

```java
// 卡口 1：Hook __cxa_atexit（避免读零 .got 指针崩，红线 §R6）
// 在 __cxa_atexit 的 PLT 跳板处设卡，调用一到就拦下、返回 0（跳过真调）
// 为什么返回 0？= "跳过 C++ 析构注册"，崩溃就没了
int hookId = emulator.getBackend().hook(
    module.base + 0x0,  // __cxa_atexit PLT 跳板（基址+跳板偏移）
    (hook) -> {
        hook.setReturn(0L);   // 喂假答案：直接返回 0
    }
);
// 卡口 2：Hook FindClass（广告层拆除，红线 §R6）
// so 查广告类 com/baidu/mobads/AdView 时，喂 null（找不到）
// 全删 baidu smali 前必须先处理这个引用点，否则 so 崩
emulator.getMemory().findExport("FindClass").when((hook) -> {
    // 查的参数是广告类名 → 返回 0（null class，so 以为找不到）
    if (hook.getArg(1).getString().equals("com/baidu/mobads/AdView")) {
        return 0L;   // null = 广告类不存在，跳过广告初始化
    }
    return 0L;       // 其他类也返回 0（军棋核心类用 resolveClass 正常应答）
});
```
**为什么两大卡口都"喂假答案"而不是"真执行"？** `__cxa_atexit` 真执行要读 `.got`
（零值槽）→ 崩；`FindClass` 真执行要建 AdView（广告类）→ 广告浮层。喂假答案
（0/null）直接跳过，既补环境（不崩）又去广告（不初始化），一石二鸟。
### 15.1.2 Hook 源码分析（设卡的底层机制）


> **本节核心**：
> 1. 改 PLT 跳板（R_ARM_JUMP_SLOT 填的号改成 Hook 桩地址）
>
> **本节难点**：
> 1. so 调 libc 先跳 Hook 桩
> 2. Hook 底层 = 改写 PLT 跳板，不碰 .text
> 3. 军棋 214 个 PLT 跳板全可 Hook

unidbg 的 Hook 框架基于 unicorn2 的 `hook_add` 回调：
```
hook_add(uc, UC_HOOK_CODE, callback)
  → unicorn2 在 ARM 指令流里拦截（车辆到路口）
  → callback(regs, ...) 拿寄存器 + 修改返回（喂假答案放行）
```
军棋 32 位 so 的 Hook 关键位置（红线 §R6）：
- `__cxa_atexit` PLT（读零 `.got` 崩点 → 喂 0 跳过）
- `FindClass` JNI 调用（广告层拆除 → 喂 null）
- 14 JNI 入口（Thumb 奇地址 0x233e1 等，是追踪对象，非拦截对象）
**军棋 14 个入口名（readelf 实测，Hook 追踪对象）**：OnCreate / OnDestroy /
OnResult / OnResume / OnEvent / OnKey / OnUpdate / OnGraphChange / OnBluetooth /
OnPause / OnGraphCreate / OnTimer / OnInput / OnTouch。军棋 7 个依赖库（libc/
libstdc++/libm/liblog/libdl + 2 GL）加载全通后，14 个入口才能按地址调。
**为什么 14 入口是"追踪对象"不是"拦截对象"？** 14 个 `On*` 入口要真执行（调通业务），
Hook 是"拦下喂假答案"——对入口不该拦（拦了就不调了），该追踪（看它执行到哪）。
**"设卡喂假值"为什么比"真执行"干净（大白话）**：
- 真执行 `__cxa_atexit`：走 PLT 跳板读 `.got`（零值槽）→ 跳 0 → 段错误
- Hook 喂 0：在跳板处拦下，直接返回 0（跳过真调）→ 不读 `.got` → 不崩
- 真执行 `FindClass(AdView)`：建广告类 → 初始化广告 → 出浮层
- Hook 喂 null：在 `FindClass` 处拦下，返回 null（"查不到"）→ 跳过广告 → 不崩
"设卡喂假值"不改 so 字节（保留原版），只在跳板处拦下——军棋 `.text` 100% 对齐
不破坏，Hook 是"干净"的补环境手段。
**"军棋两大卡口"为什么选 Hook 不选 Patch（大白话）**：
- `__cxa_atexit`/`FindClass` 都是"调外部函数"，Hook 在跳板处拦下喂假值即可
- Patch 要改 `.text` 字节（破坏原版 + 重签名），军棋 `.text` 已 100% 对齐
  （1174/1174 PASS，红线 §122），Patch 反而破坏对齐成果
- Hook 保留原逻辑（只拦截应答），军棋基本不 patch（红线 §R6 强调"不碰 so 字节"）
## 15.2 Debugger 模块解析（行车记录仪 + 慢动作回放）
![Debugger 4 组件](assets/ch15_debugger_4module.png)
### 15.2.1 Console Debugger 的使用（军棋单步/看寄存器）


> **本节核心**：
> 1. 军棋单步/看寄存器（runTo/step/showRegs/breakpoint 四镜头）
> 2. 定位 0x43ed8 卡点（std::vector 拷贝构造点）
>
> **本节难点**：
> 1. Debugger 是「行车记录仪」，单步看寄存器定位崩点
> 2. 军棋 OnCreate 崩点 0x43ed8 靠 Debugger 定位

```java
// 军棋 32 位 so 的 Debugger 用法（慢动作回放）
Debugger debugger = new Debugger(emulator);
debugger.runTo(0x233e1);  // 跑到军棋 OnCreate 入口（Thumb 奇地址）
debugger.step();          // 单步执行一条指令（慢动作走一步）
debugger.showRegs();      // 看 r0-r12 + pc 寄存器值（拍"现场照片"）
```
军棋 32 位 so 的 Debugger 关键场景：
- 跟踪 `OnCreate` 推进到 `std::vector` 对象图 0x43ed8（DiagJni 实证）
- 排错 PC off-by-2 bug（dynarmic mask vs unicorn2 不 mask）
### 15.2.2 Debugger 源码分析（四个"镜头"）


> **本节核心**：
> 1. 四个镜头：runTo/step/showRegs/breakpoint
>
> **本节难点**：
> 1. 单步穿越 §124 三项阻塞必崩（读未初始化值被安全阀兜成 0）
> 2. Debugger 源码是「慢动作回放」，军棋定位卡点
> 3. 单步不能补对象图（崩在数据缺失非指令）

unidbg 的 Debugger 模块（基于 r0dbg 思路），四个"镜头"各拍一类信息：
| 镜头 | 大白话 | 军棋实证 |
|---|---|---|
| `InterruptHook`（拦截系统调用） | so 发系统调用时拦下喂假答案 | 拦 `open`/`uname` 喂 mock |
| `CodeTrace`（指令追踪） | 一条一条拍指令执行流 | 定位"崩在哪条指令" |
| `MemoryTrace`（内存读/写追踪） | 拍哪个内存被读写了 | 定位"读了零指针" |
| `Console`（交互式调试） | 暂停时手动看寄存器、改值 | 单步看 0x43ed8 对象图 |
军棋 32 位 so 的 Debugger 实证（红线 §R6）：
- `DiagTracePC` 0xa37fc 存 0x159880 = 不 mask 值（unicorn2 修复 PC off-by-2 的铁证）
- `DiagJni` OnCreate 推进到 `std::vector` 对象图 0x43ed8（Java 侧接通后卡在 §124 的
  3 项阻塞：① 上下文相关 BL 解码 bug ② 数据与指令对齐未初始化 ③ 288MB PROT_NONE mmap）
## 15.3 逐环节"能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **mmap 段** | 2 个 PT_LOAD（r-x/rw-）全映射到 base=0x40000000 | 真 Linux mmap 语义（Java 堆 mock，PROT_NONE 是"假的"，§124 ③） |
| **栈/堆** | 栈 0x40000（256KB）够用；堆模拟 `malloc` | 补数据与指令对齐（std::vector begin/end 未初始化，§124 ②） |
| **填 .got** | 重定位时填 5312 RELATIVE + 214 PLT + 6 GLOB_DAT（内存里填） | 从静态 `.got` 读运行时值（静态文件全 0，需 redroid dump 18 地址 72B） |
| **Hook 拦截** | Hook `__cxa_atexit`（喂 0 跳过读零 `.got`）、`FindClass`（喂 null 去广告）、I/O 重定向 | 拦截 288MB PROT_NONE mmap（unidbg mmap2 处理失败，§124 ③） |
| **Debugger 追踪** | `DiagTracePC` 拍到 0xa37fc=0x159880（PC off-by-2 修好）；`DiagJni` 拍到 OnCreate 推进到 0x43ed8 | 单步穿越 §124 三项阻塞（BL 解码 bug / C++ 对象图 / 288MB mmap） |

**军棋实测对照**：
- mmap 段 ✓（r-x + rw- 全映射）
- 栈/堆 ✓（栈 0x40000，堆模拟 malloc）
- 填 .got ✓（5312/214/6 全填，内存里填）
- Hook 拦截 ✓（两大卡口全设好，`__cxa_atexit` 喂 0 + `FindClass` 喂 null）
- Debugger 追踪 ✓（DiagTracePC + DiagJni 全实证）
- **✗ 绕过 §124 ③ 288MB mmap**：unidbg mmap2 mock 对 288MB 级 PROT_NONE 占位映射
  处理失败（Java 堆"假的"权限语义，Hook 也拦不了这种"内核级占位"）

## 15.4 军棋实战验证：Hook 两大卡口 + Debugger 定位


> **本节核心**：
> 1. Hook 两大卡口 + Debugger 定位 0x43ed8
>
> **本节难点**：
> 1. 前 3 层补环境全通，第 4 层 C++ 对象图补不了 → §124
> 2. Hook + Debugger 是「指令能跑」工具，非「功能一致」工具

军棋 unidbg 的 Hook/Debugger 实测（红线 §R6）：
| 手段 | 军棋目标 | 实测 |
|---|---|---|
| Hook `__cxa_atexit` | 避免读零 `.got` 崩 | 喂 0 跳过 ✓ |
| Hook `FindClass` | 去广告（AdView 返 null） | 喂 null ✓ |
| Debugger DiagTracePC | 验证 PC off-by-2 修复 | 0xa37fc=0x159880 ✓ |
| Debugger DiagJni | 定位 OnCreate 卡点 | 推进到 0x43ed8 ✓ |
**Hook + Debugger 配合排错**：Hook 设卡（让 so 不崩）→ Debugger 追踪（看它执行到哪）。
军棋"能加载"靠 Hook 两大卡口（不崩），"定位卡点"靠 Debugger（看 0x43ed8 对象图）。
## 15.5 本章小结（一句话版）

> unidbg Hook = "在路口设卡喂假答案"（军棋两大卡口：`__cxa_atexit` 喂 0 跳过读零
> `.got` + `FindClass` 喂 null 跳广告类），一石二鸟（补环境 + 去广告）。
> unidbg Debugger = "行车记录仪四镜头"（InterruptHook/CodeTrace/MemoryTrace/Console），
> 军棋实证 DiagTracePC（PC off-by-2 修复）+ DiagJni（OnCreate 到 0x43ed8，
> 卡在 §124 的 3 项阻塞：① 上下文相关 BL 解码 bug ② 数据与指令对齐未初始化
> ③ 288MB PROT_NONE mmap）。
> 军棋 18 个 `.text` 常量池的**房间真值**静态读不出（门牌号静态可读，N45），需 redroid dump 房间真值；Hook 优先、Patch 是最后手段
> （军棋 `.text` 100% 对齐基本不 patch）。

---


> **延伸阅读（附录 C.3 机械化审计工具）**：本章讲"怎么 Hook/插桩 so"，**但"Hook 后怎么防回归"（改一处崩另一处）是工程化维度**——军棋会话 2 沉淀的 `link_audit.py`（链接安全审计）/ `jni_method_audit.py`（JNI 反射 ExceptionClear 审计）/ `tex_pipeline.py`（vtable/dispatch 机械化分析）就是"Hook/插桩的逆向版"（不注入，离线 diff 防回归）。**"同类手改模式先机械化成工具再动手"（§124zz-8 红线）**。详见 `APPENDIX_C_REVERSE_ENGINEERING.md`。

## 视频
