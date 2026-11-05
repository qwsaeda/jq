# 第21章 unidbg 实战：使用 unidbg 主动调用 fork 进程

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的 fork 场景

## 痛点：为什么 fork 在模拟环境里是"死路"？

军棋 so 的 ad SDK（`com.baidu.mobads` 广告框架）启动时会 `fork()` 一个子进程
干后台活（统计上报、预加载广告素材）。unidbg 单线程模拟，fork 语义根本表达不了。
军棋 14 个 JNI 入口（readelf 实测：OnCreate / OnDestroy / OnResult / OnResume /
OnEvent / OnKey / OnUpdate / OnGraphChange / OnBluetooth / OnPause /
OnGraphCreate / OnTimer / OnInput / OnTouch）里，fork 场景与 `OnCreate`
（广告后台启动）相关，核心功能（`OnBluetooth`/`OnTimer`/`OnInput`...）不用
fork。军棋实测 3 个卡点。

### 痛点 1：unidbg 是"单线程模拟"，fork 要起第二个"人"

真 Android 上 `fork()` 是 Linux 系统调用，内核直接复制当前进程出一个子进程，
父进程继续、子进程独立跑。unidbg 里你模拟的是"一个线程"在跑，没有内核，
没有"第二个进程"的概念。so 一调 `fork`，unidbg 不知道该把"子进程"模拟到哪。

- **大白话卡点**：你一个人在家做饭，so 说"再叫个人来端盘子"（fork），
  你叫不出第二个人 → 卡死。
- **为什么卡**：unidbg 是单地址空间模拟，没有进程表、没有 PID 命名空间。
- **不解决会怎样**：so 调 `fork` 系统调用无人应答 → unidbg 直接报错/崩。

### 痛点 2：fork 返回值有两种"身份"，unidbg 只能模拟一种

真 `fork` 的精髓：

```c
pid = fork();
if (pid == 0)      { /* 我是子进程，干后台活（广告统计）*/ }
else if (pid > 0)  { /* 我是父进程，pid 是子进程号，继续 */ }
else               { /* fork 失败 */ }
```

- **大白话卡点**：同一个 `fork` 调用，父进程看到子 PID（非 0），子进程看到 0，
  两个方向各走各的分支。unidbg 只能"返回一个值"，选哪个方向就是替 so 选分支。
- **为什么卡**：unidbg 单线程，"模拟子进程"还是"模拟父进程"只能二选一，
  选错方向 so 就走错分支（选父 → 后面 `waitpid` 卡死；选子 → 广告后台逻辑空跑）。
- **不解决会怎样**：广告后台分支卡 `waitpid`（没真子进程可等）→ 主流程被拖住。

### 痛点 3：子进程和父进程"共享内存"，unidbg 模拟不了

真 `fork` 后，子进程是父进程的"副本"（copy-on-write），俩有自己的地址空间
但内容一样。unidbg 只有一个"地址空间"。

- **大白话卡点**：so 以为"我和子进程各有一份内存、写互不影响"，你只有
  一份内存，子进程写 = 父进程内存被改。
- **为什么卡**：COW（写时复制）语义 unidbg 没有，fork 后"两个进程共享一块
  内存"它根本表达不了。
- **不解决会怎样**：子进程读到的内存跟父进程混着，逻辑全错。

## 为什么这样做：不模拟 fork，直接"跳过"（军棋场景的正确解法）

关键判断：**军棋的 fork 是"要去掉的广告 SDK 后台进程"，不是核心功能。**

类比：so 是"住老宅的房东"，他有个"雇工"（fork 子进程）帮他"跑腿买酱油"
（广告统计）。你的目标是"让老宅能住"（跑通核心游戏逻辑），而不是"让雇工也
跑起来"。所以：

- **不要试图模拟"雇工"（子进程）**——unidbg 模拟不了真 fork
- **直接告诉房东"没雇工了"（Hook fork 返回 0，假装子视角）**——so 拿 0
  后走子进程分支，但那个分支是广告后台逻辑（本来就要去掉），不崩就行

**为什么返回 0（子视角）而不是返回非 0（父视角）？**
看军棋 ad SDK 的代码逻辑：父进程拿到子 PID 后是"等子进程干完活再收尸"
（`waitpid`），unidbg 没子进程，`waitpid` 会卡住。返回 0 = "我是子进程"，
so 走子进程分支干完后台活就 `exit`，不会卡 `waitpid`。**关键：广告后台逻辑
本来就是"去广告"要删的，走通它就没事。**

**预判追问："返回 -1（fork 失败）不更干净？"** 可以，但 so 拿 -1 会走
"错误分支"——ad SDK 的错误分支可能是打日志 + 清理 + `abort`，比"子视角
干完就退"更容易踩雷。返回 0 走的是"正常子进程"路径，语义最接近真机里
"子进程视角"，副作用最小。军棋实测 0 全搞定（广告后台走通但没真跑，
等于去广告）。

**预判追问："clone 也返回 0，音效线程不就也不播了？"** 对——unidbg 里
"子线程返回 0 + 线程干完就退"意味着音效线程**没有真跑**（没第二份执行流）。
这不是缺陷，是 unidbg 单线程模拟的固有边界：fork/vfork/clone 三者都只能
"假装起了、实际没起"。军棋核心玩法（走子/吃子判定）在 `OnUpdate`/`OnEvent`
主线程逻辑里，不在子进程里，所以这个边界不影响"能玩"路径。

## 21.1 样本情景复现（军棋 ad SDK 的 fork 场景）


> **本节核心**：
> 1. 军棋 ad SDK fork 场景（广告后台进程）
>
> **本节难点**：
> 1. unidbg 单线程模拟不了 fork 起第二个「人」（子进程）
> 2. 「fork 是死路」：unidbg 单线程，fork 要起子进程
> 3. 军棋去广告目标：fork 后台不跑正合适

军棋 32 位 so 的 fork 场景（红线 §R6 已确认）：
| 场景 | 大白话 | 是不是要去掉 |
|---|---|---|
| 广告 SDK 后台进程（`com.baidu.mobads.AppActivity`） | 统计上报 / 预加载广告 | ✅ **要去掉**（广告层拆除目标，manifest 里 baidu AppActivity + UMENG_APPKEY） |
| 蓝牙联机（`OnBluetooth` JNI 入口） | 蓝牙传输棋子走法 | ❌ 保留（核心功能，走 JNI 不走 fork） |
| 音效播放（`sound.xml` 12 个音效） | 播"吃子"音效 | ❌ 保留（核心功能，走线程池） |
**关键**：军棋要 fork 的是**广告 SDK 后台进程**，这是"去广告"红线要删的。
所以正确做法是"Hook 掉 fork，让广告后台进程不起"，一石二鸟（补环境 + 去广告）。
## 21.2 样本反编译分析（smali 层 + native 层）


> **本节核心**：
> 1. smali + native 双层分析（军棋 ad SDK 在 smali + native 都有 fork）
>
> **本节难点**：
> 1. fork/vfork/clone 三兄弟：fork 返 0（子）/正数（父），vfork 共享地址，clone 全控
> 2. 「fork 三兄弟」是系统调用层概念
> 3. 军棋 ad SDK fork 后台进程，去广告 = 不跑 fork

军棋 32 位 so 的反编译分析（军棋 Java 层只有 ~7 个类，几百行 smali）：
```
Java 层(smali):
  com.cnvcs.App        ← 入口
  com.cnvcs.GLView     ← GL 渲染
  com.cnvcs.SiteView   ← 站点
  com.cnvcs.AdLis      ← 广告监听(去广告目标)
  com.cnvcs.BtReceiver ← 蓝牙接收
  com.cnvcs.TextDlg    ← 文本对话框
  com.cnvcs.InputDlg   ← 输入对话框
核心逻辑在 native 层:
  lib/armeabi/libjunqi.so(**1.43MB/1435156B**,strip,EABI5)
    → ad SDK 后台进程: fork() 起子进程干广告统计
    → 蓝牙联机: OnBluetooth JNI 入口(不用 fork)
    → 音效播放: 线程(用 clone,不用 fork)
```
**为什么广告用 fork 而蓝牙/音效不用？** 广告统计是"后台脏活"（耗时、可丢），
fork 个独立进程跑不卡主流程；蓝牙联机是"实时交互"（不能卡），用 JNI 同步调；
音效是"短时任务"（播完就完），用线程池。
## 21.3 so 中代码的分析（fork / vfork / clone 三兄弟）


> **本节核心**：
> 1. fork/vfork/clone 三兄弟在 so 里的调用
>
> **本节难点**：
> 1. fork 返回两种身份（子 0/父 PID），unidbg 只模拟父
> 2. 「fork 在 so 里」= native 层调 fork 系统调用
> 3. COW 共享内存 unidbg 模拟不了

军棋 so 的 fork 相关系统调用（32 位 ARM 的 `sys_*` 号，`.text` 明文，
objdump 可反汇编）：
| 系统调用 | 号 | 大白话 | 军棋用途 |
|---|---|---|---|
| `fork()` | `sys_fork = 2` | 复制整个进程（内存也复制，慢） | ad SDK 后台进程 |
| `vfork()` | `sys_vfork = 322` | 复制进程但共享内存（更快） | 广告预加载 |
| `clone()` | `sys_clone = 120` | 创建线程（Linux 线程就是 clone） | 音效播放线程 |
**为什么军棋用 fork 而不是 pthread？** ad SDK 是 C 代码（native 层），
直接用 Linux `fork` 系统调用比 C++ `std::thread` 更"底层"。
而音效播放是 C++ 代码（`std::thread`），走 `clone`。
## 21.4 使用 unidbg 对 fork 进程做处理（军棋实战代码）


> **本节核心**：
> 1. 军棋实战代码：fork 返 0 走「退」分支（不模拟子进程）
>
> **本节难点**：
> 1. 「不模拟 fork 直接跳过」是军棋场景正确解法
> 2. fork 返 0 = 让 so 走「子进程退」分支，不真起子进程
> 3. 跳过 fork 代价：ad SDK 后台逻辑不跑（军棋去广告目标不跑正合适）

![军棋 fork 处理](assets/ch21_fork_call.png)



```java
// 1. 拦截 fork 系统调用(32 位 ARM: sys_fork = 2)
// 军棋 ad SDK 的 fork → 返回 0(子进程视角),让 so 走子进程分支干广告后台活
emulator.getMemory().setSvcHandler(SYS_fork, (emulator, svcNumber, regs) -> {
    return 0L;  // 广告后台进程"走通就退",不真起子进程
});

// 2. 拦截 vfork / clone(军棋其他场景)
emulator.getMemory().setSvcHandler(SYS_vfork, (emulator, svcNumber, regs) -> 0L);
emulator.getMemory().setSvcHandler(SYS_clone, (emulator, svcNumber, regs) -> {
    return 0L;  // 子线程视角, 音效线程"走通就退"
});
```

**为什么三个系统调用都返回 0？**
- `fork` 返回 0 = 我是子进程 → 走广告后台分支（去广告目标）
- `vfork` 返回 0 = 同上
- `clone` 返回 0 = 我是子线程 → 音效线程干完就退（不卡主流程）

**军棋的"一石二鸟"**：
1. 补环境：so 调 fork 不崩（被 Hook 拦截）
2. 去广告：ad SDK 后台进程走子进程分支后直接退，广告统计没真跑（等于去广告）

## 21.5 逐环节"我们能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **Hook fork/vfork/clone** | 拦截 3 个系统调用返回 0（子视角），广告后台进程"走通但退"（去广告） | 起真子进程（unidbg 单地址空间 + 单线程，无 COW 语义、无 PID 命名空间） |
| **选返回值** | 返回 0 避开 `waitpid` 卡死；返回 -1 走错误分支 | 返回非 0 父视角（`waitpid` 等不到真子进程，主流程拖住） |
| **父进程后续** | 主流程继续跑（广告分支已退） | 模拟"子进程独立跑、父进程等收尸"的父子交互（没有第二执行流） |
| **核心功能** | 蓝牙（`OnBluetooth` JNI）/ 音效（线程池）不受 fork 处理影响 | 验证"真子进程行为"（音效线程实际没真跑，只走了"子视角路径"） |

## 21.6 军棋实战验证（fork 场景的 unidbg 处理）


> **本节核心**：
> 1. fork 场景 unidbg 处理验证（返 0 走退分支）
>
> **本节难点**：
> 1. fork 返回两种身份（子 0/父 PID）unidbg 只模拟父；COW 共享内存模拟不了
> 2. 「fork 死路」军棋场景解法：跳过 fork
> 3. fork 是 unidbg 单线程模拟的固有边界

军棋 32 位 so 在 unidbg/unicorn2 下的 fork 处理实测（红线 §R6）：
| 系统调用 | 军棋触发点 | unidbg 处理 | 结果 |
|---|---|---|---|
| `fork` (sys=2) | ad SDK 起后台进程 | Hook 返回 0 | 广告后台进程"走通但退"（去广告） |
| `vfork` (sys=322) | ad 预加载 | Hook 返回 0 | 同上 |
| `clone` (sys=120) | 音效播放线程 | Hook 返回 0 | 音效线程"走通但退"（不影响主流程） |
**关键结论**：军棋的 fork 场景全是"要去掉的广告层"或"短线程"，
**都不需要真模拟子进程**，Hook 返回 0 全搞定。
只有当 so 用 fork 做"核心逻辑"（军棋蓝牙联机用的是 JNI 不是 fork）时才需要真模拟。
**§124 关联**：军棋 unidbg 路线当前的 3 项阻塞（① 上下文相关 BL 解码 bug
② C++ 对象图 ③ 288MB PROT_NONE mmap）跟 fork 无关——fork 已 Hook 搞定。
fork 是军棋"广告层拆除"的一环，不在"能玩"的主路径上。
## 本章小结（一句话版）

> fork 在模拟环境是"死路"（unidbg 单线程模拟不了真子进程）。
> 军棋的 fork 场景全是"要去掉的广告后台进程"，正确做法是**不模拟、直接跳过**：
> Hook `fork`/`vfork`/`clone` 都返回 0（子进程视角），so 走子进程分支干完广告活就退，
> 一石二鸟（补环境 + 去广告）。军棋核心功能（蓝牙/音效）不用 fork，不受影响。

## 视频
