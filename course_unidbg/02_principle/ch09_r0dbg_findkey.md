# 第9章 R0dbg 实战与 Unidbg_FindKey

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的初始化 + 指令追踪排错

## 痛点：为什么 so 跑崩了却查不出"崩在哪"？

so 一崩，你只知道"崩了"，不知道"崩在哪条指令、读的哪个值"。军棋实测踩过 3 个坑。

### 痛点 1：崩了不知道崩在哪条指令

军棋 `OnCreate` 一调就崩，但崩点是"读了个零指针"还是"跳了个错地址"？
不追踪指令执行流（R0dbg 做的事），只能猜。

- **大白话卡点**：程序崩了，你只知道"病人死了"，不知道"死在哪个器官"。
  得拍 X 光（指令追踪）一条一条看执行到哪崩的。
- **为什么卡**：so 是黑盒，不追踪就看不到内部指令流。
- **不解决会怎样**：只能反复试错猜崩点，效率极低。

### 痛点 2：so 会"检查环境"，发现是模拟器就崩

军棋 so 有 `uname`（读系统信息的系统调用）等反检测手段——它一看 PC 环境不对
（/proc 没东西、没有真 GPU），主动 abort（主动终止自己）。得拦截系统调用"喂"它假数据。

- **大白话卡点**：so 会"验明正身"（查你是不是模拟器），查出来是 unidbg 就主动崩。
- **为什么卡**：反检测靠读系统调用结果（`uname`/`/proc`），模拟环境给不出真值。
- **不解决会怎样**：so 主动 abort，根本进不了业务逻辑。

### 痛点 3：找不到"密钥"在哪

军棋有 18 个 `.text` 常量池地址（`ldr rX,[pc,#N]` 读密文），只有 dlopen 后才填值。
静态读不出来——unidbg_FindKey 的"跑通再 hook 取值"思路对军棋**部分**适用
（OnCreate 对象图没补全，仍卡住）。

- **大白话卡点**：钥匙（18 个常量池值）挂在墙上但你够不着（静态读不出），
  得"跑起来把钥匙拿到手上再拍张照"（hook 取值）。
- **为什么卡**：这 18 个值只在 dlopen 运行时填，静态文件是占位。
- **不解决会怎样**：读不到 18 个值，依赖它们的逻辑全错。

## 为什么这样做：把"排错"拆成"追踪→拦截→主动取值"三层

类比医生看病（三层递进）：

- **追踪（R0dbg）**：拍 X 光，看指令一条一条执行到哪、寄存器什么值——找出崩在哪条。
- **拦截（Hook 系统调用）**：so 问"你是什么系统？"，你喂它假答案（拦截 `uname`
  返回正常值），骗过反检测。
- **主动取值（Unidbg_FindKey）**：跑通 dlopen 后，在常量池地址挂 hook，直接读运行时
  值——相当于"等钥匙插上后拍张照"。

**三层为什么是这个顺序？** 追踪（看清崩在哪）是基础；拦截（喂假数据）解决反检测；
主动取值（hook 常量池）是最后一步拿"只有运行时才有的值"。军棋前两层全通，
第三层卡住（见下）。

**军棋诚实边界**：第 3 层"主动取值"军棋**走不通**——因为 `OnCreate` 推进到
`std::vector` 对象图 0x43ed8 就卡住（§124 纠正后 3 项阻塞：① 上下文相关 BL 解码 bug
② 数据与指令对齐未初始化 ③ 288MB PROT_NONE mmap，红线 §R6 + §124），拿不到 18 地址的完整
运行时上下文。所以军棋 18 地址唯一拿法仍是 **redroid 跑原版 so → dlopen 后 dump
72 字节**（`frida-dump/redroid_jni_dump.py` 三件套）。

**预判追问："为什么 unidbg_FindKey 拿不到 18 常量，redroid 却能？"**
unidbg_FindKey 思路是"跑通 so → hook 常量池地址 → 读运行时值"，但军棋 unidbg
跑不通（§124 三项阻塞：① BL 解码 bug ② 数据与指令对齐对应（N44，原"对象图"）③ 288MB mmap），到不了
"填 18 常量"的时机。redroid 是真跑原版 32 位 so（有 GPU + 真 dlopen + 真内核），
`.text` 常量池被 linker 填上真值，dump 出 72B 注入 unidbg 适配层（`jq_load_runtime_values`）
→ 模拟时读到正确值。**本质区别：redroid 是真跑，unidbg 是模拟跑（模拟跑不全）**。

## 9.1 模拟 Linker：so 的初始化过程（军棋实测）


> **本节核心**：
> 1. 军棋 12 init_array 全通（通水电）
> 2. R0dbg 单步追踪 init_array，崩了知道崩在哪
>
> **本节难点**：
> 1. init_array 跑 C++ 静态构造 + 全局初始化
> 2. init[9..11] 触发 bx bug（§124h），最大安全集 init[0..8]

军棋 so 的 init 阶段（unidbg/unicorn2 实测，红线 §R6）：
```
1. .preinit_array[0]   → 1 个构造函数   （最早通的水电）
2. .init              → _init 函数      （系统自带入口）
3. .init_array[0..11] → 12 个构造函数   （公司自己的 12 个初始化）
4. 返回 dlopen 调用者
```
12 个 init_array 全通 ✓。**R0dbg 在这里的角色**：init 跑完 12 个构造函数后，
再用 R0dbg 追踪后续 `OnCreate` 的指令流，定位崩点。init 全通是"能排错"的前提——
连初始化都没跑通，追踪出来的崩点全是 init 阶段的假象。
## 9.2 指令追踪与排错

### 9.2.1 TLS 线程局部存储（Thread Local Storage，线程私有变量区）


> **本节核心**：
> 1. TLS 线程局部存储：线程私有变量区
> 2. 军棋 12 init_array 里有 TLS 设置；unidbg 单线程 TLS 地址固定
>
> **本节难点**：
> 1. TLS 是「线程私有」，unidbg 单线程模拟 TLS 地址不变
> 2. 军棋 TLS 在 base+0x155000 类，固定地址

军棋 so 没有 `.tbss` 段（32 位 ARM 这版没用 TLS），TLS 初始化可跳过。
unidbg 的 `Memory` 会分配线程局部存储，但军棋 so 不强依赖。
**为什么提 TLS？** 追踪排错时常遇到"线程私有变量"（每个线程一份的全局变量，
存在线程栈的 TLS 区）。军棋这版没用，跳过；但其他 so 用了 TLS，追踪时就要
把 TLS 区也映射好，否则会读到脏值。军棋这条"省事了"，别在其他 so 上照抄。
### 9.2.2 R0dbg 对系统调用进行拦截（喂假数据骗过反检测）
![系统调用拦截](assets/ch09_syscall_hook.png)
军棋 so 的系统调用：
- `open` / `read` / `close`（读 `data/config/*.xml` 配置文件）
- `mmap` / `mprotect`（加载 so 段）
- `uname`（检测系统，军棋有反检测：发现 /proc 异常就崩）

R0dbg（看雪作者的 r0 框架，一个"追踪 + 拦截"工具）的拦截方式：

```java
// 拦截系统调用：看寄存器 r7（ARM 系统调用号寄存器）是不是 open
// r7 == SYS_open 时返回一个 mock 的文件描述符（假文件号，不真开文件）
hook = backend.hook(0x0, (hook) -> {
    // r7 是 ARM 的系统调用号寄存器（调 svc 前放调用号进去）
    if (hook.registers().r7() == SYS_open) {
        return 1;   // 返回 fd=1，骗它"文件打开了"
    }
    return 0;       // 没拦住（这个调用号不归我管），正常走
});
```

**为什么拦截返回"假 fd"而不是"真开文件"？** 军棋 so 读的是虚拟路径
（`/data/data/com.cnvcs.junqi/...`），你 PC 上没这文件。真开会失败（-1），
所以直接返回假 fd=1 骗它"开好了"——真读文件走 I/O 重定向（第 16 章）。

## 9.3 使用 R0dbg 模拟执行 so（军棋执行路径）


> **本节核心**：
> 1. 军棋执行路径：12 init_array → 0x233e1 OnCreate → 0x43ed8（§124 卡点）
> 2. R0dbg 单步定位 OnCreate 崩点
>
> **本节难点**：
> 1. 执行路径是「加载后调用」的完整 trace
> 2. OnCreate 推进到 0x43ed8 后 §124 卡住

军棋 so 在 R0dbg 下的执行路径：
```
dlopen(libjunqi.so) → Bionic linker → 12 init_array 全跑完 → JNI_OnLoad
→ Java_com_cnvcs_App_OnCreate (0x233e1, Thumb 奇地址)
```
**14 个入口名（readelf 实测）**：OnCreate / OnDestroy / OnResult / OnResume / OnEvent /
OnKey / OnUpdate / OnGraphChange / OnBluetooth / OnPause / OnGraphCreate / OnTimer /
OnInput / OnTouch。
> 注意：0x233e1 是**奇地址**（Thumb 模式标记 bit0=1），追踪时起点要 `&~1` 对齐到偶指令，
> 否则可达指令集为空（红线 §R6）。军棋 14 个 JNI 入口全是这种 Thumb 奇地址，
> 追踪/翻译前都要对齐。
## 9.4 Unidbg_FindKey 牛刀小试（军棋的"找密钥"场景）


> **本节核心**：
> 1. 军棋 SPAK key 静态明文（.rodata VA 0x12a658），不用 FindKey 动态找
>
> **本节难点**：
> 1. 18 常量运行时才填，FindKey 式动态找走不通（退 redroid）
> 2. FindKey 是「主动找密钥」，军棋 key 静态可读
> 3. SPAK key 32B 明文 c2 cb e5 e7 ce 5c ...，.rodata 非壳

![军棋 FindKey 策略](assets/ch09_findkey_strategy.png)



Unidbg_FindKey 是 unidbg 框架作者的工具，用于**主动搜索密钥**（跑通 so →
hook 关键地址 → 读运行时值）。军棋 so 的"密钥"场景有两个：

1. **SPAK 解密 key**（32 字节，内嵌 `.rodata`）：静态 `.rodata` 明文可 dump，
   **无需 unidbg 搜索**（已提取，见第 19 章）。

2. **18 个 `.text` 常量池**（`ldr rX,[pc,#N]` 读 0xbd10f… 密文）：
   - 静态 `.so` 读不出（只有 dlopen 重定位后才填值，**不是壳自解密**——
     `.rodata` 静态即明文已证）
   - **unidbg_FindKey 思路**：跑通 dlopen → hook 常量池地址 → 取运行时值
   - **但军棋走不通**：`OnCreate` 对象图未补全（推进到 `std::vector` 0x43ed8 卡住，
     §124 纠正后 3 项阻塞：① 上下文相关 BL 解码 bug ② 数据与指令对齐未初始化
     ③ 288MB PROT_NONE mmap）→ 仍需 **redroid 跑原版 dump 这 18 地址 72 字节**
     （工具三件套 `redroid_jni_dump.py` + `redroid_dump_jni.sh` + `gen_runtime_table.py`，
     红线 §R6）

**为什么 SPAK key 不用 FindKey、18 常量池却要用？** 前者在 `.rodata` 静态明文（直接 dump），
后者是"只有运行时才填的值"（静态读不出）。FindKey 专攻"运行时才有的值"，
军棋后者拿不到 → 退而用 redroid dump。

**"18 地址为什么是 72B"（大白话）**：
15 个 `.text` 常量池（每个 4 字节 = 60B）+ 2 个 `.data`（每个 4 字节 = 8B）
= 共 72B。这 18 个值"只有 dlopen 重定位才填"，静态 `.so` 读不出（占位符）。
军棋"能跑"唯一拿法 = redroid 跑原版 so → dlopen → dump 这 18 地址 72B。

**"为什么 unidbg 拿不到这 18 地址"（大白话）**：
unidbg 模拟 so，但 §124 的 ① 上下文相关 BL 解码 bug（unicorn2 精度）+
② 数据与指令对齐对应缺口（N44） + ③ 288MB PROT_NONE mmap 让 unidbg 跑不到"填 18 地址"
的时机。redroid 是真跑原版 so（有 GPU + 真 dlopen），`.text` 常量池被填上真值，
dump 出 72B 注入 unidbg 适配层（`jq_load_runtime_values`）→ 模拟时读到正确值。

**"18 地址 + redroid dump"三件套（大白话）**：
- `redroid_jni_dump.py` = 精准读 18 地址（定位 `.text` 常量池 + `.data`）
- `redroid_dump_jni.sh` = 一键触发（跑容器 + dlopen + dump）
- `gen_runtime_table.py` = JSON→C 注入表（dump 出的 72B 生成 C 表）
3 件套配合：dump → 注入 → unidbg 模拟时读到正确 18 常量（非壳自解密，是重定位填值）。

## 9.5 逐环节"能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **指令追踪（R0dbg）** | 一条一条拍指令执行流，定位崩在哪条（12 init_array 全跑完、JNI_OnLoad 跑通） | 单步穿越 §124 三项阻塞（BL 解码 bug / C++ 对象图 / 288MB mmap） |
| **系统调用拦截** | 拦 `open`/`uname`/`mmap` 喂假数据骗过反检测（fd=1、uname 正常值） | 给真 `mmap` 288MB PROT_NONE（unidbg mmap2 处理失败，§124 ③） |
| **主动取值（FindKey）** | 跑通 so 后 hook 关键地址读运行时值（SPAK key 32B 静态 dump 也行） | 拿 18 个 `.text` 常量（unidbg 跑不通，§124 ①②③，需 redroid dump） |
| **定位崩点** | OnCreate 推进到 `std::vector` 对象图 0x43ed8（DiagJni 实证），PC off-by-2 修好（0xa37fc=0x159880） | 突破 0x43ed8（§124 ② 数据与指令对齐对应缺口（N44），std::vector begin/end 未初始化） |

**军棋实测对照**：
- 指令追踪 ✓（R0dbg 拍到 12 init_array 全跑完、JNI_OnLoad 跑通、OnCreate 推进到 0x43ed8）
- 系统调用拦截 ✓（`open`/`uname` 喂假数据）
- 主动取值 ⚠️（SPAK key 静态 dump ✓；18 常量 unidbg 拿不到，需 redroid）
- 定位崩点 ✓（0x43ed8 + PC off-by-2 修好）
- **✗ 突破 0x43ed8**：§124 ② 数据与指令对齐对应缺口（N44）（std::vector 未初始化，N44 外层缺口）

## 9.6 军棋实战验证：R0dbg 追踪 OnCreate 崩点

军棋 unidbg 排错实测（红线 §R6）：

| 追踪对象 | R0dbg 看到 | 大白话 |
|---|---|---|
| 12 init_array | 全跑完 ✓ | 初始化无崩点 |
| JNI_OnLoad | 跑通 ✓ | 注册 14 入口 |
| OnCreate（0x233e1） | 推进到 `std::vector` 0x43ed8 后卡 | 卡在 native 数据与指令对齐（N44） |
| 0xa37fc（PC off-by-2） | dynarmic 存 `&~3`（错），unicorn2 存 0x159880（对） | 后端切换铁证 |

**结论**：R0dbg 把"崩在哪"看得清清楚楚——不是 init、不是 JNI 注册，是 `OnCreate`
推进到 0x43ed8（std::vector 拷贝构造点（数据与指令对齐，N44））后卡住，加上 §124 的 ① 上下文相关 BL 解码 bug
（加载 .so 后 BL 0x7965a 被 unicorn2 解码成 0x6eaea 而非真实目标 0x43ed8）。

## 9.7 本章小结（一句话版）

> R0dbg = 追踪 + 拦截，能定位崩点、喂假数据骗过反检测（军棋 12 init_array 全通后用它排错）。
> Unidbg_FindKey 的"跑通再 hook 取值"对军棋 18 个 `.text` 常量**走不通**
> （OnCreate 卡 0x43ed8，§124 纠正后 3 项阻塞：① 上下文相关 BL 解码 bug
> ② 数据与指令对齐未初始化 ③ 288MB PROT_NONE mmap），唯一拿法是 redroid dump。
> 军棋 PC off-by-2 已用 unicorn2 后端修复（DiagTracePC 实证 0xa37fc = 0x159880）。

---

## 视频
