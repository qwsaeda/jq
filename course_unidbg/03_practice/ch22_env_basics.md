# 第22章 unidbg 补环境实战：补环境入门

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的补环境入门

## 痛点：为什么"补环境"是逆向里最耗时的活？

你在 unidbg 里跑军棋 so（14 个 JNI 入口 readelf 实测：OnCreate / OnDestroy /
OnResult / OnResume / OnEvent / OnKey / OnUpdate / OnGraphChange /
OnBluetooth / OnPause / OnGraphCreate / OnTimer / OnInput / OnTouch），
它会一路崩下去，每崩一次你补一个，补完再崩下一个。军棋实测 3 个卡点。

### 卡点 1：崩点很多

军棋 so 读文件、调 libc、回调 Java、起子进程……每个都要补。

- **大白话卡点**：so 像"挑剔的租客"，要一堆"真房东"服务（读文件/调库/调 Java），
  每个都要补。
- **为什么卡**：军棋 so 读 7 个 config XML、经 214 个 PLT 跳板调 libc 函数、
  14 个 JNI 入口回调 Java、依赖 7 个 DT_NEEDED 库，每个都是"崩点"。
- **不解决会怎样**：补一个崩一个，没完没了（军棋从"加载崩"补到"0x43ed8 对象图
  崩"，一路补了 12 个 init_array + 5312 个 RELATIVE + 214 个 PLT + 6 个 GLOB_DAT）。

### 卡点 2：补的顺序有讲究

先补 A 崩 B，再补 B 崩 C……你得按 so 的执行顺序一个个来。

- **大白话卡点**：补的顺序有讲究——先水电（系统调用）、再物业（Java 方法）、
  最后改合同（patch 指令），乱序补就反复崩。
- **为什么卡**：so 按执行顺序崩，补错顺序就"补了白补"——比如 `__cxa_atexit`
  走 `.got` 零槽崩，你不填 6 个 GLOB_DAT 就急着 patch 指令，patch 完一跑，
  下一个读 `.got` 的地方又崩。
- **不解决会怎样**：补 A 崩 B 补 B 崩 C，来回折腾。

### 卡点 3：补到最后发现"语义不对"

军棋的 C++ 运行时布局（`std::vector`）不是"没补"，是"补了也不对"——这是最难的。

- **大白话卡点**：前三层（文件/库/Java）补了能过，但第四层"数据与指令对齐对应"
  补了也不对（std::vector 的 begin/end/capacity 没初始化，N44：业务数据 apk 静态可推，缺运行时对齐）。
- **为什么卡**：`std::vector` 运行时布局不是"补个值"能解决的，要理解真实
  C++ 运行时布局（begin 指向堆首元素、end=begin+size、capacity 预留），
  这些值来自 dlopen 后的真实运行时，静态/模拟都读不出。
- **不解决会怎样**：`OnCreate` 推进到 0x43ed8 卡住（§124 的 ②），
  补环境到顶也突破不了。

## 为什么这样做：按"系统调用 → Java 方法 → 指令 → 数据与指令对齐"四层补

补环境的核心思路：**so 假设它在真 Android 上，你一层层把"真 Android 的应答"补上。**

类比：so 是个"挑剔的租客"，它要"真房东"的服务（系统调用/Java/指令）。
你当"假房东"，一项项把服务补上：先补水电（系统调用）、再补物业（Java 方法）、
最后改合同（patch 指令）。

军棋补环境 3+1 类（按难度升序）：

| 层 | 大白话 | 军棋场景 | 难度 |
|---|---|---|---|
| 系统调用 mock | 拦住 `open`/`read`，返回 mock 值 | 读 `data/config/*.xml` | 低 |
| Java 方法 mock | 拦住 `FindClass`，返回 mock 类 | 广告层 `AdView` 返 null | 中 |
| 指令 patch | 改写某条指令直接返回 0 | `__cxa_atexit` 跳零 `.got` | 高 |
| **数据与指令对齐补全** | 给未初始化对象补 begin/end/capacity | std::vector 0x43ed8 | **最高（补不了）** |

**预判追问："指令 patch 和 Hook 有什么区别？为什么军棋基本不 patch？"**
Hook 是"函数入口/返回处插一脚"（so 原指令流不动，旁路记录/拦截），
patch 是"直接改 so 的字节"（比如把 `bl __cxa_atexit` 改成 `bx lr`）。
军棋 `.text` 已 100% 对齐（红线 §122），改字节就破坏"原版 100% 对齐"承诺
+ 要重签名，所以能用 Hook 的绝不用 patch（军棋实测指令 patch 层基本闲置，
`__cxa_atexit` 走填 `.got` 真值而非 patch）。

**预判追问："前三层都通了，第四层能不能也'补个值'糊弄过去？"** 不能。
前三层补的是"应答值"（open 返 fd、FindClass 返类），值错了还能重跑修正；
第四层补的是"数据与指令对齐不变式"（std::vector 的 begin/end/capacity 三指针要
满足"指向同一堆区 + end=begin+size + capacity≥size"），这个不变式来自
**真实 C++ 运行时布局**（谁 new 的、堆在哪、几号块），unidbg 模拟环境里
没人知道——手工填 begin/end/capacity 只能填一次，对象后续演化（拷贝构造、
扩容）全乱。这就是 §124 的 ② "数据与指令对齐对应缺口"补不了的原因。

**"挑剔租客"的四层需求（大白话）**：
- 第 1 层"水电"（系统调用）：读文件/调 libc → mock 值，最易
- 第 2 层"物业"（Java 方法）：查类/调方法 → DalvikVM 应答，中等
- 第 3 层"改合同"（patch 指令）：改指令直接返 0 → 少用（军棋基本不 patch）
- 第 4 层"手艺"（数据与指令对齐）：std::vector 真实布局 → 补不了（unidbg 天花板）
军棋"水电 + 物业"全补（前三层通），"手艺"补不了（第四层卡 0x43ed8）。

## 22.1 军棋补环境现状（红线 §R6 实测）


> **本节核心**：
> 1. 军棋补环境现状（红线 §R6 实测）
>
> **本节难点**：
> 1. 前 3 层（系统调用/Java/指令）全通，第 4 层（C++ 对象图）补不了 → §124
> 2. 「补环境现状」是军棋补环境到顶的诚实边界
> 3. 前 3 层通 = 能加载 + 能跑指令，第 4 层卡 = 不能玩

已通的：
- 12 个 `.init_array` 全通 ✓
- 5312 RELATIVE 重定位全通 ✓
- 214 PLT→GOT→符号映射全通 ✓
- 6 GLOB_DAT 从 docker 真 ARM libc 解析 ✓
- 7 个 DT_NEEDED 依赖全解析（libc/libstdc++/libm/liblog/libdl + 挂桩
  libGLESv1_CM/libGLESv2）✓
- JNIEnv/VM 补环境实证可行（DiagJni：createDalvikVM + resolveClass +
  callStaticJniMethodObject，OnCreate 从空 env 入口推进到 std::vector
  对象图 0x43ed8）✓
**剩余阻塞（§124 纠正后 3 项）**：
- **① 上下文相关 BL 解码 bug**（模拟器精度问题，非翻译/逻辑 bug）：
  加载 .so 后 `BL 0x7965a` 被 unicorn2 解码成目标 `0x6eaea`（memset 循环），
  而真实目标是 `0x43ed8`（std::vector 构造）。隔离环境 BL 全对，
  说明是"上下文相关"的解码错位。workaround：换 dynarmic 后端 /
  code hook patch 目标 / unicorn2 版本修复。
- **② 数据与指令对齐对应缺口**：std::vector 的 `begin`/`end`/`capacity` 来自
  未初始化对象，模拟器填不出"真实运行时"的对象布局（N44：业务数据 apk 静态可推，缺运行时对齐）。
- **③ 288MB PROT_NONE mmap**：unidbg mmap2 对 288MB PROT_NONE 处理失败。
**3 项阻塞为什么都"补不了"？** ① 是模拟器精度 bug（unicorn2 解码错），
不是"缺环境"；② 是运行时内存布局（std::vector 真实布局要真机构造链），不是
"补个值"；③ 是 unidbg mmap2 限制（288MB 太大）。三者都不是"再补点
环境"能解决的，是 unidbg 路线的天花板。
**"补环境"和"补对齐"的分界（大白话）**：
- 补环境（前三层）= 把"真 Android 的应答"补上（系统调用/Java/指令），军棋全通
- 补对齐（第四层）= 把"数据与指令运行时对齐"补对（std::vector 的 begin/end/
  capacity 要"运行时真实布局"，不是"补个值"能解决的）
军棋补环境到第三层就触到天花板（第四层对齐补不了），unidbg 路线到此为止，
"能玩"得靠真机 GPU 或 redroid dump + arm64 对齐版。
## 22.2 军棋补环境实战（按原书 5 小节拆，恢复细度）
![军棋补环境 3 大类](assets/ch22_env_fallback.png)
> 原书 22.3 拆 5 个小节（参数获取 / 代码初始化 / 目标函数调用 / 补环境说明 / 补环境实战），
> 课程早期并成"3 步代码"丢了细度，现拆回。

### 22.2.1 参数获取（军棋 `OnCreate` 的 `env` / `obj` 从哪来）


> **本节核心**：
> 1. 参数获取（env/obj 从哪来）：env=vm.getJNIEnv()（JNI 函数表）
>
> **本节难点**：
> 1. obj=vm.addGlobalObject(DvmObject)（哈希非指针）
> 2. 「obj 是哈希不是指针」：DalvikVM 假 VM 里 GLView 用整型哈希引用
> 3. 军棋 OnCreate(env, obj) 两参数逐个获取

军棋 `OnCreate(JNIEnv *env, jobject obj)` 要两个参数，参数获取路径：
```java
// env:JNIEnv*(JNI 函数表指针, 232 槽)
Pointer jniEnv = vm.getJNIEnv();
// 军棋:vm.getJNIEnv() 返回模拟的 JNI 函数表, 里面 FindClass/GetMethodID/CallXxxMethod
//      全接 unidbg 的 setJni 应答器(AdView 返 null 去广告)
// obj:jobject(GLView 的 Java 对象哈希)
// 军棋:GLView 是 so 渲染入口, DalvikVM 里是个 DvmObject
DvmClass<?> glviewCls = vm.resolveClass("com/cnvcs/GLView");
int objHash = vm.addGlobalObject(glviewCls.newObject());
// addGlobalObject:把 GLView 对象注册到 DalvikVM, 返回整型哈希(不是 C++ 对象指针)
```
**为什么 `obj` 是"哈希"不是"对象指针"**：DalvikVM 是 unidbg 的假 Java VM，
军棋 `GLView` 对象在这假 VM 里用整型哈希引用（`0x1234abcd`），不是 C++ 堆上的对象。
so 拿 `obj` 调 `(*env)->GetObjectClass(obj)` 时，DalvikVM 查"哈希 ↔ 对象"映射查回 `GLView`。
### 22.2.2 代码初始化（unicorn2 + AndroidResolver + 挂桩）


> **本节核心**：
> 1. 代码初始化（unicorn2 + AndroidResolver + 挂桩）
>
> **本节难点**：
> 1. 配水电（后端+库）+ 配物业（DalvikVM）；缺一个崩
> 2. 「代码初始化」是补环境第 2 步
> 3. unicorn2 修 PC off-by-2；AndroidResolver(23) 自带 5 系统库

```java
// 1. 选后端:unicorn2(修 PC off-by-2, 军棋红线 §R6 已实证)
AndroidEmulator emulator = AndroidEmulatorBuilder.for32Bit()
        .backend(Backend.UNICORN2).build();
// 2. 配系统库:AndroidResolver(23) 自带 5 个 32 位库
//    (libc/libstdc++/libm/liblog/libdl, 军棋 7 依赖里的前 5 个)
emulator.getMemory().setLibraryResolver(new AndroidResolver(23));
// 3. 挂桩 GL:军棋 7 依赖里 libGLESv1_CM/libGLESv2 无真 GPU, 挂桩函数
emulator.getMemory().symbolResolver().resolve("glGetError").when((h) -> 0L);
emulator.getMemory().symbolResolver().resolve("glBegin").when((h) -> 0L);
// ... 挂桩其他 GL 函数(军棋 GL 渲染桩, 详见第 7 章)
// 4. 建 Java 侧替身:军棋 7 个类(App/GLView/SiteView/AdLis/BtReceiver/TextDlg/InputDlg)
VM vm = emulator.createDalvikVM(new File("junqi.apk"));
```
**"代码初始化"为什么是补环境第 2 步**：第 1 步（选后端 + 配库）是"配水电"，
第 2 步（建 DalvikVM）是"配物业"——so 调 Java 方法时物业（DalvikVM）要有人接。
### 22.2.3 目标函数调用（军棋 14 入口按签名调）


> **本节核心**：
> 1. 目标函数调用（14 入口按签名调）
>
> **本节难点**：
> 1. 军棋 14 入口签名不统一，逐个 marshal（同 13.1.1）；OnCreate @ 0x233e1
> 2. 「按签名调」是 JNI 调用核心
> 3. 14 入口签名差异：OnTouch(int,int)/OnEvent(String,String)/OnCreate(obj)

```java
// 军棋 14 个 JNI 入口按签名调(详见第 13 章 13.1.1 参数 marshal)
// 主入口 OnCreate(Thumb 奇地址 0x233e1, &~1 对齐)
long ret = vm.getMemory().call(emulator, 0x233e1L, 0, jniEnv, objHash).singleValue();
// 军棋 DiagJni:OnCreate 推进到 std::vector 0x43ed8(vector 拷贝构造点, §124 三项阻塞)
// 其他 13 入口按各自签名调(OnTouch(int,int) / OnEvent(String,String) / ...)
```
### 22.2.4 补环境说明（军棋三层补 + 第四层补不了）


> **本节核心**：
> 1. 补环境说明（四层：系统调用→Java→指令→C++ 对象图）
>
> **本节难点**：
> 1. 前 3 层补环境能解，第 4 层 std::vector 0x43ed8 补不了（语义缺失）
> 2. 「四层补」是军棋补环境方法论
> 3. 第 4 层 std::vector 元素数填不出（C++ 运行时语义）

军棋补环境按"系统调用 → Java 方法 → 指令 → C++ 对象图"四层补：
| 层 | 补什么 | 军棋实测 |
|---|---|---|
| 1 系统调用 | `open`/`read`/`close`（读 7 config XML + one.sjg） | ✓ 全通（I/O 重定向，第 16 章） |
| 2 Java 方法 | `FindClass`/`GetMethodID`（14 入口 + AdView 去广告） | ✓ 全通（`setJni`，第 3/13 章） |
| 3 指令 | `__cxa_atexit` Hook 返 0（避免读零 `.got` 崩） | ✓ 全通（Hook，第 3/15 章） |
| 4 数据与指令对齐 | std::vector 0x43ed8 的 begin/end/capacity | ✗ **补不了**（§124 ② 数据与指令对齐对应缺口，N44） |
**"第四层补不了"为什么**：std::vector 是 C++ 对象（按 32 位布局），unidbg 模拟的是
"指令执行"不是"运行时内存布局"——`begin/end/capacity` 三个指针来自未初始化对象，
unidbg 填不出"这个 vector 有多少元素"（那是真实 C++ 运行时才有的布局）。
### 22.2.5 补环境实战（军棋 18 常量 + §124 三项阻塞）


> **本节核心**：
> 1. 补环境实战（18 常量 + §124 三项阻塞）
>
> **本节难点**：
> 1. 前三层全通推进到 0x43ed8，§124 ① BL bug ② 对象图 ③ mmap 卡住
> 2. 「补环境天花板 = §124」是军棋诚实边界
> 3. unidbg 拿「能玩」的路：前 3 层 unidbg（已通）+ 第 4 层 redroid 跑原版

军棋补环境实战到"前三层全通 + 第四层卡住"：
```
前三层全通:
  系统调用 ✓(open/read/close) + Java 方法 ✓(FindClass/GetMethodID) + 指令 ✓(Hook cxa_atexit)
  → OnCreate 推进到 std::vector 0x43ed8
第四层卡住(§124 三项阻塞):
  ① 上下文相关 BL 解码 bug:加载 .so 后 BL 0x7965a 被 unicorn2 解码成 0x6eaea(真实 0x43ed8)
  ② 数据与指令对齐对应缺口:std::vector 0x43ed8 的 begin/end/capacity 未初始化(N44:业务数据 apk 静态可推,缺运行时对齐)
  ③ 288MB PROT_NONE mmap:unidbg mmap2 mock 处理失败
军棋拿"能玩"的路:
  前三层 = unidbg 补环境(已通)
  第四层 = redroid 跑原版 so + 真 arm64 GPU 设备验收(不是 unidbg 能解的)
```
**为什么 3 步是"最小闭环"（大白话）**：
"建模拟器 → 加载自检 → 调入口"三步缺一不可：没建模拟器（1）后面没法加载，
没加载自检（2）OnCreate 没注册，没调入口（3）验证不了补环境对不对。
军棋 3 步全通（DiagJni 实证），但推进到 0x43ed8 卡（§124 的 ②）——
"最小闭环"通 ≠ "业务"通，卡点在 native 对象图。
## 22.3 补环境实战：redroid dump → unidbg 注入


> **本节核心**：
> 1. redroid dump → unidbg 注入（工具三件套）
>
> **本节难点**：
> 1. frida-dump/redroid_jni_dump.py 读 18 地址 + redroid_dump_jni.sh 触发 + gen_runtime_table.py 注入
> 2. 「redroid dump」是 18 常量**早期**拿法（**★ §124x-z 后 = 静态固化 `jqt_mem.c` 免 dump**）
> 3. unidbg 注入后指令能跑，但 §124 仍卡

军棋 18 个 `.text` 常量池的**房间真值**（15 `.text` + 2 `.data`，共 72B；门牌号静态可读，房间真值静态读不出（N45），
需 dlopen 后 dump）：
```
redroid 容器跑原版 32 位 so → dlopen → dump 18 地址 72B
  → 注入 unidbg 适配层 jq_load_runtime_values
  → unidbg 模拟时读到正确常量
```
工具三件套（红线 §R6）：
- `frida-dump/redroid_jni_dump.py`（精准读 18 地址）
- `redroid_dump_jni.sh`（一键触发）
- `gen_runtime_table.py`（JSON→C 注入表）
**预判追问："unidbg 怎么填这 18 个值？它自己算出来不就行了？"** 算不出来。
这 18 个值**仅 dlopen 重定位时填**（`.so` 未重定位，`.got` 槽为零），
真值来自真实 32 位 libc 链接结果，不是 unidbg 能"算"的——unidbg 模拟的是
"重定位执行"，但军棋 `.text` 内嵌常量池（`ldr rX,[pc,#N]` 读的密文/占位）
在 unidbg 里读出来的就是静态文件值（零/密文），跟真机 dlopen 后填的真值
对不上。**早期**拿法：redroid（真 32 位 Android 容器，真 dlopen + 真重定位）
跑原版 so → dump 这 18 地址 72B → 注入 unidbg 适配层（`jq_load_runtime_values`），
模拟时读到正确常量；**★ §124x-z 后 = 静态位移固化 `jqt_mem.c` `g_jq_got_tbl`（免 dump，`jni_runtime_values.json` 已删）**。**值来自真实 32 位库，不是 unidbg 算的**——这是
"unidbg 补环境"边界最典型的一例。
**"redroid dump vs unidbg 模拟"分工（大白话）**：
- unidbg：模拟 so 跑（但卡数据与指令对齐（N44），18 常量池**房间真值**读不出（门牌号静态可读，N45））
- redroid：真跑原版 so（真 dlopen 填 `.text` 常量池）→ dump 出 18 值
- 注入：把 dump 出的 72B 注入 unidbg 适配层 → unidbg 模拟时读到正确值
"redroid 拿数据 + unidbg 注入"是军棋拿 18 常量**早期**路（unidbg 自己读不出**房间真值**，N45：门牌号静态可读，房间真值需 dlopen 才填对）；**★ §124x-z 后 = 静态位移固化 `jqt_mem.c` `g_jq_got_tbl`（免 dump，`jni_runtime_values.json` 已删）**。
## 22.4 逐环节"我们能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **系统调用 mock** | 7 个 config XML + 分辨率目录 + `one.sjg` 全通；`__cxa_atexit` 走 `.got` 填真值（docker 真 ARM libc 解析 6 GLOB_DAT） | 模拟真机 COW 子进程语义（fork/vfork/clone 只"假装"，见 21 章） |
| **Java 方法 mock** | `setJni` 应答 14 入口 + `FindClass(AdView)` 返 null 去广告（DiagJni 实证） | 完整 C++ 对象图（第 4 层） |
| **指令 patch** | 方法上可用（军棋基本闲置，优先填 `.got`） | 靠 patch 绕过 §124 三项阻塞（是精度/语义/内存限制，不是"指令写错"） |
| **C++ 对象图** | 给 std::vector 手工填 begin/end/capacity（一次性，填完 `OnCreate` 再多推进一点） | 把"填一次"当"对象图正确"（后续拷贝构造/扩容全乱，§124 的 ②） |
| **注入 18 常量** | redroid dump 72B → `jq_load_runtime_values` 注入，模拟读到正确值 | unidbg 自己"算出"18 值（值来自真实 32 位 libc 重定位结果，非模拟值） |

## 22.5 军棋实战验证：补环境天花板 = §124 的 3 项


> **本节核心**：
> 1. 补环境天花板 = §124 的 3 项
>
> **本节难点**：
> 1. unidbg 拿「能玩」的路：前 3 层 unidbg + 第 4 层 redroid（非 unidbg 能解）
> 2. 「天花板」是诚实边界：补环境到此为止
> 3. §124 ① BL bug ② 对象图 ③ 288MB mmap 是 unidbg 解不了的

军棋 unidbg 补环境实测（红线 §R6 + §124，unicorn2 后端）：
| 层 | 军棋结果 | 大白话 |
|---|---|---|
| 系统调用 mock | 全通 ✓ | 读文件/调 libc 都接上了 |
| Java 方法 mock | 全通 ✓（DiagJni） | 14 入口 + AdView 都应答 |
| 指令 patch | 不用（.text 100% 对齐） | 基本不 patch |
| 数据与指令对齐 | **卡 0x43ed8** | §124 的 ②，补不了 |
**结论**：军棋补环境到第三层（指令 patch 都不用），卡在第四层"数据与指令对齐对应"。
unidbg 路线的天花板就是 §124 的 3 项阻塞（① 上下文相关 BL 解码 bug
② 数据与指令对齐对应缺口 ③ 288MB PROT_NONE mmap）——这三项 unidbg 补不了，
军棋"能玩"得靠真机 GPU 设备（redroid 无 GPU，只能 dump 数据不能验证"能玩"）。
## 22.6 本章小结（一句话版）

> 补环境 = 按"系统调用 → Java 方法 → 指令 → 数据与指令对齐"四层补。
> 军棋前三层已通（12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT
> + JNIEnv），卡在最难的第四层：数据与指令对齐（std::vector begin/end/capacity）补初始值。
> §124 的 3 项阻塞（① 上下文相关 BL 解码 bug ② 数据与指令对齐对应缺口
> ③ 288MB PROT_NONE mmap）是 unidbg 路线天花板，18 常量池退用 redroid dump。

## 视频
