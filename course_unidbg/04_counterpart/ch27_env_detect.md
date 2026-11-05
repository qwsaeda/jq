# 第27章 Anti-unidbg 系列：环境变量检测

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的反检测

## 痛点：为什么 so 要查"环境变量"？

军棋 so（以及带风控的 so）启动时干一堆"环境自检"，目的是判断
"我是跑在真手机上，还是被人用 unidbg/Frida 在 PC 上分析"。
军棋实测 3 个卡点。

### 痛点 1：`LD_PRELOAD` 反 Frida

`LD_PRELOAD` 是个环境变量，能强制"把某个 .so 加到所有进程开头加载"。
真 Android 上一般没人设它（或设空）；Frida 注入时 `LD_PRELOAD` 里会带
`frida-gadget.so`。

- **大白话卡点**：so 查 `getenv("LD_PRELOAD")`，看到里面有 `frida` →
  "有人 hook 我！" → 走异常分支。
- **为什么卡**：`LD_PRELOAD` 是反注入的第一道"身份证"，逆向环境设了
  就自曝。
- **不解决会怎样**：so 查到 frida → 崩 / 返回假数据 / 不跑核心逻辑。

### 痛点 2：`ANDROID_DATA` 反 unidbg

真 Android 上 `ANDROID_DATA=/data`（用户数据目录）。
unidbg 模拟环境如果没设这变量，so 读到 null → "这不像真 Android！"。

- **大白话卡点**：so 查"数据目录在哪"，没设 = 不是真 Android。
- **为什么卡**：Bionic 启动时设的 `ANDROID_*` 变量是"真机指纹"，
  模拟环境默认全没设。
- **不解决会怎样**：so 怀疑分析环境 → 走异常分支。

### 痛点 3：环境变量组合是 so 的"身份证核验"

so 把一堆环境变量组合起来当"身份证"：`ANDROID_DATA=/data` +
`ANDROID_ROOT=/system` + `ANDROID_ASSETS=/system/app/...` + `LD_PRELOAD` 空。
这套"身份证"对不上"真 Android 的样子"，so 就怀疑你是不是模拟器。

- **大白话卡点**：单查一个变量可能"蒙混过关"，组合查就露馅。
- **为什么卡**：so 一次查 4 个变量，缺一个/错一个就"不像真机"。
- **不解决会怎样**：组合核验不过 → 走异常分支。

**痛点本质**：so 在"验明正身"——查你的环境变量，看是不是"真 Android 的样子"。

## 为什么这样做：补一套"真 Android 的环境变量"

类比：so 是"查身份证的保安"，环境变量是"身份证上的字段"（哪个系统、
哪个用户、哪个数据目录）。你当"假身份"，把字段填成"真 Android 的样子"
（`ANDROID_DATA=/data`，`LD_PRELOAD` 空着），so 验完放行。

**关键原则**：
1. **设得跟真机一模一样**（`ANDROID_DATA=/data` 而非 `/data_xxx`）
2. **该空的就空**（`LD_PRELOAD=""` 而非不设——unset 和 empty 是两回事，
   见 27.4 的追问）
3. **别设"可疑值"**（别设 `LD_PRELOAD=frida`）

军棋 so 的环境变量（32 位 Bionic，红线 §R6）：

| 变量 | 真 Android 值 | 军棋查它的目的 | 怎么补 |
|---|---|---|---|
| `ANDROID_DATA` | `/data` | 找数据目录（反 unidbg） | `setEnv("ANDROID_DATA", "/data")` |
| `ANDROID_ROOT` | `/system` | 找系统目录 | `setEnv("ANDROID_ROOT", "/system")` |
| `ANDROID_ASSETS` | `/system/app/com.cnvcs.junqi/assets` | 找资源 | `setEnv` 填路径 |
| `LD_PRELOAD` | 一般空 | 反 Frida（查到 frida 就崩） | `setEnv("LD_PRELOAD", "")` |

**预判追问："unset（不设）和 set 空字符串 `""` 有什么区别？so 查得出吗？"**
查得出，且两者语义不同：
- `getenv("LD_PRELOAD")` 在 unset 时返回 `NULL`（指针为空）；
  在 `""` 时返回指向空串的指针（非 NULL）。
- 真机 Android 上 `LD_PRELOAD` 一般是 unset（NULL）；Frida 注入时是
  非空串（带 frida-gadget）。
- so 的检测逻辑通常是 `if (p && strstr(p, "frida"))` —— unset（NULL）
  和 `""` 都过（不会进 frida 分支）。所以军棋实测 `setEnv("LD_PRELOAD", "")`
  够用（返回空串，`strstr` 搜不到 frida）。
- **真正的红线是"别设成含 frida 的串"**——那是自曝。

**预判追问："这些变量 unidbg 是'算出来'还是'手工填'？"** 手工填。
环境变量不是 so 执行能"算"出来的值（不像重定位有确定算法），它是
**进程启动时由宿主注入的环境状态**。unidbg 的 `setEnv(k, v)` 就是
"替你注入一个假宿主环境"，值是你在 Java 侧硬编码的"真 Android 的样子"。
军棋 4 个变量的值（`/data`/`/system`/资源路径/空）全部来自
"真机实测的常量"，不是 unidbg 推出来的。

## 27.0 吸收自 4 本书（揉合去重）：加壳 vs 非壳判断 + 壳 so 脱壳前置

> **本节来源**：加壳/脱壳取《二进制分析实战》Ch9.5（Pin DBI 脱壳器原理最全）、混淆取《二进制分析实战》Ch8（递归反汇编处理混淆最专）。详见 `ELF_FORMAT_SUPPLEMENT.md` 第五节。

**课程通用**：unidbg 跑**壳 so** 的前置——壳 so 先脱壳（dump 解密后 `.text`）才能补环境。军棋 so 明文非壳，**加壳/脱壳用不上**，但"加壳 vs 非壳判断"方法军棋用上。

### 27.0.1 "加壳 vs 非壳判断"（课程通用 + 军棋用上）

**判断 so 是否加壳**（4 本书揉合）：
| 判据 | 壳 so | 非壳 so（军棋） |
|---|---|---|
| `.text` 熵 | 高熵（加密态，>7.5） | **7.10（正常机器码）** |
| `.rodata` 静态 vs 运行时 | 不同（自解密例程跑完才明文） | **0/118832 不同（静态=运行时，无自解密）** |
| 入口 | 壳存根（PT_LOAD 加密段） | 原入口 |
| `readelf -l` | 有加密数据段（非常规） | 全标准 9 段 |

**军棋 so 非壳证据**：`.text` 熵 7.10（正常机器码）+ `.rodata` 静态=运行时 0/118832 不同（无自解密）→ **反汇编直接可做**（`objdump` 出真实指令 + JNI 符号名）。

### 27.0.2 壳 so 脱壳前置（课程通用，军棋跳过）

**脱壳器原理**（《二进制分析实战》9.5.2，Pin DBI）：
```
Pin DBI 引擎 → 监控所有指令 → 检测"向一次性写内存区（OWMR）的间接控制流跳转"
  → OWMR = 加壳时存根解密代码写入的内存区（解密后写一次）
  → 间接跳转 OWMR = "跳到 OEP（原始入口点）" = 脱壳完成信号
  → dump OEP 周围连续字节 = 脱壳产物（解密后 .text）
```
**课程通用价值**：跑壳 so 先脱壳（dump 解密后 `.text`），再 unidbg 补环境（否则 unidbg 读壳存根读不出原代码）。
**军棋跳过**（军棋非壳，反汇编直接可做，不需脱壳）。

### 27.0.3 "重定位" vs "壳自解密"区分（易混淆，课程通用）

| | 重定位 | 壳自解密 |
|---|---|---|
| **谁做** | linker（dlopen 时） | 壳存根（解密例程） |
| **干什么** | 给 `.got`/`.data` 加 load_base（`.got` 槽从 0 变真地址） | 把加密的 `.text`/`.rodata` 解密成明文（XOR/decrypt 例程） |
| **是壳吗** | ❌ 非壳（标准 dlopen 行为） | ✅ 壳 |
| **军棋** | 18 个 `.text` 常量池"只有 dlopen 才填" = **重定位**（linker 加 load_base） | 军棋**无**（`.rodata` 静态=运行时，无 decrypt 例程） |

**军棋 §124 三项阻塞全非壳特征**（判断用）：
| §124 阻塞 | 是壳特征吗 | 军棋实际 |
|---|---|---|
| ① BL 解码 bug | ❌ 非壳（unicorn2 模拟器精度问题） | unidbg 的 bug，不是 so 的壳 |
| ② C++ 对象图语义缺失 | ❌ 非壳（C++ 运行时，堆对象运行时创建） | 翻译体 32→64 对象图错位 |
| ③ 288MB PROT_NONE mmap | ⚠️ 像壳但实际是 so 内保护段 | unidbg mmap2 mock 失败（非壳自解密） |

## 27.1 Linux 中的环境变量（基础概念）


> **本节核心**：
> 1. Linux 环境变量基础（LD_PRELOAD 反 Frida / ANDROID_DATA 反 unidbg）
>
> **本节难点**：
> 1. 「环境变量」是反检测入口
> 2. LD_PRELOAD 非空 = Frida 注入；ANDROID_DATA 不存在 = unidbg
> 3. so 查环境变量做「身份证核验」

Linux（Bionic 也类似）的环境变量是"键值对"，存在进程的环境块里。
so 通过 libc 的 `getenv()` 函数读（底层是 `execve` 传入的环境块，
或读 `/proc/self/environ`）。
军棋 32 位 so 用到的 Linux 环境变量（32 位 ARM Bionic）：
```
PATH=/sbin:/vendor/bin:/system/bin:...   ← 命令搜索路径
LD_LIBRARY_PATH=/system/lib:...          ← 库搜索路径
HOME=/root                               ← 用户主目录
ANDROID_DATA=/data                       ← Android 特有
```
**关键**：so 读环境变量走的是 `getenv()`（libc 函数），unidbg 的
`SvcMemory` 拦截系统调用能 mock `getenv`，`setEnv` 把值存进模拟进程
的环境块。
## 27.2 Android 中的环境变量


> **本节核心**：
> 1. Android 环境变量（ANDROID_DATA/ANDROID_ROOT）
>
> **本节难点**：
> 1. 军棋 mock ANDROID_DATA=/data + ANDROID_ROOT=/system + LD_PRELOAD=""
> 2. 「Android 环境变量」是真 Android 指纹
> 3. 军棋 so 查 3-5 个变量组合，全 mock 才不崩

Android（Bionic）在 Linux 基础上多了一组 `ANDROID_*` 变量：
| 变量 | 真 Android 值 | 大白话 |
|---|---|---|
| `ANDROID_DATA` | `/data` | 用户数据根目录（每个 app 的数据在 `/data/data/<pkg>/`） |
| `ANDROID_ROOT` | `/system` | 系统根目录 |
| `ANDROID_ASSETS` | `/system/app/<pkg>/assets` | app 资源目录 |
| `ANDROID_BOOT_LOG_TAG` | `boot` | 启动日志标签 |
军棋 so 可能检测这些变量"是否存在 + 值对不对"。
- 存在且值对 → 像真 Android
- 不存在 / 值不对 → 怀疑是分析环境
## 27.3 环境变量源码解析
![军棋环境变量检测](assets/ch27_env_detect.png)
### 27.3.1 Java 层环境变量读取和设置


> **本节核心**：
> 1. Java 层 System.getenv（军棋 7 个 Java 类无 System.getenv 调用，走 native）
>
> **本节难点**：
> 1. 「Java 层环境变量」军棋不走
> 2. 军棋环境变量查在 native 层（getenv）

军棋 smali 层（Java 侧）读环境变量：
```java
// smali 反编译（伪代码）
const-string v0, "ANDROID_DATA"
invoke-static {v0}, Ljava/lang/System;->getenv(Ljava/lang/String;)Ljava/lang/String;
move-result-object v0    // v0 = "/data"
```
**但军棋核心逻辑在 native 层**（32 位 `libjunqi.so`，14 个 JNI 入口），
Java 层读环境变量只是"辅助"，so 真正查环境走 native 的 `getenv`。
### 27.3.2 Native 层环境变量读取和设置


> **本节核心**：
> 1. Native 层 getenv（军棋 so 查 3-5 个变量组合）
>
> **本节难点**：
> 1. 全 mock 才不崩（漏一个走检测命中）
> 2. 「Native 层环境变量」是军棋反检测核心
> 3. 军棋 so getenv 查 LD_PRELOAD/ANDROID_DATA 等

```c
// 军棋 32 位 so（native 层）
const char *p = getenv("ANDROID_DATA");
if (p == NULL) {
    // 没设 → 怀疑分析环境
    // 走异常分支（崩 / 返回假数据）
}
const char *q = getenv("LD_PRELOAD");
if (q && strstr(q, "frida")) {
    // 查到 frida → 反 Frida
    // 走异常分支
}
```
**为什么 native 层查？** 风控逻辑放 native（C/C++）比放 Java（smali）
难逆，且 so 启动早（Java 层还没起来就能查）。
**注意（军棋诚实边界）**：军棋 32 位 so **不是风控 so**（红线 §R6 已证
无壳、`.rodata` 明文、1715 条字符串清单里没有风控特征串）。本章的环境
变量检测讲的是"带风控的 so 怎么查 + unidbg 怎么补"的通用方法论；
军棋本身查这几项主要是"找数据目录"（功能），不是"反分析"（风控）。
补这套变量是"以防万一"的通用补环境动作。
### 27.3.3 情景模拟（军棋的反检测场景）


> **本节核心**：
> 1. 情景模拟（军棋反检测场景）
>
> **本节难点**：
> 1. 军棋 32 位 so 反检测 8 维度（第 30 章），环境变量是其中一类
> 2. 「情景模拟」是反检测验证
> 3. 军棋 8 维度全 mock 绕过（但 ≠ 能玩，还卡 §124）

军棋 32 位 so 的环境变量情景（红线 §R6）：
```
1. 启动 → getenv("ANDROID_DATA")
   - 真机: 返回 "/data" → 正常
   - unidbg 没设: 返回 NULL → 路径拼接错
2. 启动 → getenv("LD_PRELOAD")
   - 真机: 返回 NULL(没设) → 正常
   - Frida: 返回 "frida-gadget.so" → 反 Frida
3. 启动 → getenv("ANDROID_ROOT")
   - 真机: 返回 "/system" → 正常
   - unidbg 没设: 返回 NULL → 路径拼接错
```
## 27.4 逐环节"我们能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **`setEnv` 补变量** | `ANDROID_DATA=/data`、`ANDROID_ROOT=/system`、`ANDROID_ASSETS`、`LD_PRELOAD=""` 全补，`getenv` 应答"真 Android 的样子" | 用 `setEnv` 绕过 §124 三项阻塞（bx bug / 全局未初始化 / 288MB mmap 与环境变量无关，环境变量补得再全也救不了） |
| **unset vs 空串** | `LD_PRELOAD=""`（空串，`strstr` 搜不到 frida，过反 Frida 检查） | 靠 unset"更像真机"（`getenv` 返回 NULL，部分 so 对 NULL 走异常分支，空串更安全） |
| **值来源** | 手工填"真机实测常量"（`/data`/`/system`/资源路径） | 期望 unidbg"算出"变量值（环境变量是宿主注入的状态，不是 so 能算的值——同 22 章"18 常量由 redroid dump 填"的逻辑） |

## 27.5 本章小结（一句话版）

> 环境变量 = so 的"身份证核验"（查 `ANDROID_DATA` / `ANDROID_ROOT` /
> `LD_PRELOAD`）。
> 军棋查 4 个：`ANDROID_DATA`（数据目录）/ `ANDROID_ROOT`（系统目录）/
> `ANDROID_ASSETS`（资源）/ `LD_PRELOAD`（反 Frida）。
> unidbg 补法：`setEnv` 把字段填成"真 Android 的样子"（`ANDROID_DATA=/data`，
> `LD_PRELOAD=""` 空字符串）——跟真机一模一样，so 验完放行。
> 注意：军棋本身不是风控 so，补变量是"通用补环境动作"，不解决 §124
> 三项阻塞（bx bug / 全局未初始化 / 288MB mmap）。

## 视频
