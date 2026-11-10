# 第3章 unidbg 补环境、Hook 与 Patch

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地实战靶机：军棋 `lib/armeabi/libjunqi.so`（ELF32 / ARM / EABI5 / stripped）

## 痛点：为什么"补环境"是逆向里最脏最累的活？

你在 unidbg 里调军棋 `OnCreate`，它内部干了三类事，每类都在你的模拟环境里"缺应答"：

**痛点 1：so 读文件，你的 PC 上没文件。**
军棋 so 启动 `open("data/config/ui.xml")` / `read` / `open("data/lay/00.lay")`，
你的 x86_64 开发机上没这些路径 → `open` 返回 -1 → so 走"读不到配置"分支 → 崩。

**痛点 2：so 调 libc / C++ 运行时，模拟环境要应答。**
军棋 so 调 `malloc` / `strlen` / `__cxa_atexit`（C++ 析构注册）。
其中 `__cxa_atexit` 会读 `.got` 里的全局指针——军棋 `.got` 未重定位时是**零值**，
读零指针 → SIGSEGV。这是军棋 unidbg 的第一个崩点。

**★ 澄清（C.1 三层框架，N44/N45 呼应）**：`.got` 零值属于**"数据与指令对齐"缺口（外层）**——
**值静态读得出（`.got` 槽的门牌号 = 偏移 0x15xxxx），跟着门牌号进去的"房间真值"（重定位后的真地址）才缺**。
军棋 `.got` 的 6 个 GLOB_DAT 槽（指向 libc/libstdc++ 函数）**静态读得出门牌号，房间真值从 docker 真 ARM libc 解析填**（3.2 卡点 1）。
**别和 C.1 内层"场景对象图全构造"混淆**（那是 std::list/vector 运行时布局，补 .got 解不了，需真机 C++ 构造链）。

**痛点 3：so 回调 Java，你的 PC 上没 Dalvik VM。**
军棋 so 调 `FindClass("com/baidu/mobads/AdView")`（广告层引用）+ 14 个 JNI 入口。
你的 PC 上没有真 Dalvik/ART，so 调 `FindClass` 没人应答 → 返回垃圾 → so 崩。

**★ 双方向桥（ch02 痛点 2 已讲，这里点一下防混）**：`FindClass` 是 **"so 调 Java"（方向 ②）**——
走 `JNIEnv*` 的 232 槽 C API 表（VM 提供，unidbg 用模拟表 + `setJni` 应答），**不是 "Java 调 so"（方向 ①，`Java_` 导出符号）**。
补环境时 `findClass` 应答补的就是方向 ② 的"VM 侧电话"。

**痛点 4：补环境是"打补丁"，没规范就乱了。**
你补了 100 个卡点，每个"能跑"，但某天"改 A 崩 B"——补丁之间互相干扰。
补环境要有优先级（系统调用 > Java 方法 > 指令 patch）和"补到哪算够"的停止条件。

**★ 停止条件（C.4 增强循环的"补到哪算够"）**：军棋补环境不是"补到能玩"，而是**补到"指令能跑 + 数据对齐到 0x43ed8"**——
前 3 卡点（.got/GL/18 常量房间真值）全通 = 停止条件（**指令级跑通**）；
§124 三项（BL bug/数据对齐/mmap）**不在补环境范围**（是模拟器精度 + 数据缺口，补环境解不了）→ 停止，换路线（unidbg 做数据提取 / 翻译体做逻辑 / 真机做验收）。
**"能玩"的判据门在真机（段 3），不在 unidbg 补环境**（N44/N45：数据缺口外层可推，内层需真机）。

## 为什么这样做：Hook 拦截 + Patch 改指令，两类工具对应两类痛点

**Hook（拦截）**：so 调某函数 → 你拦住，不真执行，返回 mock 值。
- 适合"功能太复杂 / 没有真环境"的调用（军棋 `libGLESv1_CM` 无真 GPU、`__cxa_atexit` 读零 `.got`）
- 保留原逻辑（只是应答），不破坏 so 字节

**Patch（改指令）**：把 so 里某条指令改写成"直接返回 0"。
- 适合"函数必崩 / 想跳过整段逻辑"（军棋 `__cxa_atexit` 读零 `.got` 崩）
- 破坏原逻辑（跳过了），是最后手段

**为什么 Hook 优先于 Patch？** Hook 保留原逻辑（只是应答），Patch 破坏原逻辑（跳过了）。
军棋红线反复强调"要么纯指令翻译，要么反汇编成 C，函数级回归"——Patch 是最后手段。

**预判追问："Hook 和 Patch 怎么选？"** 军棋原则：
1. 先 Hook（保留逻辑，只是 mock 应答）
2. Hook 不住（比如函数内部死循环）再 Patch（跳过整段）
3. 都不行就退 redroid 真跑原版 so（军棋 18 个 `.text` 常量的"房间真值"就是这条路——N45：门牌号静态可读，房间真值需 dlopen 才填对）

**★ 预判追问："Hook 的是代码还是数据？"** 军棋补环境要分清 **Hook 的三类对象**（N45/N44 呼应）：
| 对象 | 军棋实例 | 补的是"门牌号"还是"房间真值" |
|---|---|---|
| **Hook libc 函数** | `__cxa_atexit` 返 0 | 补"代码能跑"（跳过 C++ 析构注册，不读零 `.got`） |
| **Hook JNI 方法** | `FindClass(AdView)` 返 null | 补"VM 侧电话"（方向 ② so→Java，232 槽 C API 表） |
| **Hook 系统调用** | `open` 映射宿主文件 | 补"门牌号→宿主文件"（I/O 重定向，军棋 `.rodata` 路径门牌号 → 宿主 `./junqi_data/`） |
**核心**：Hook 补的是**"代码路径能跑"**（应答/mock），**不补"数据对齐"**（N44 外层缺口需 frida dump 真值 / 真机构造链）——**Hook 让指令不崩，数据对不齐仍读错位值**。

## 3.1 为 so 添加交互：JNI 接口应答（军棋 FindClass 去广告）


> **本节核心**：
> 1. FindClass(AdView) 返 null = 补环境 + 去广告一石二鸟（既补 FindClass 应答又跳广告初始化）
> 2. 军棋红线：全删 smali/com/baidu/* 前必须处理 native FindClass 这个引用点
> 3. setJni 实现里 AdView 返 null，其他 7 类 resolveClass 正常
>
> **本节难点**：
> 1. 「一石二鸟」是军棋去广告关键：AdView null 让 so 走「广告不可用」分支不崩
> 2. FindClass 不能全返 null：java/lang/String 等必须正常，全 null → NPE

书里让 so 导出 `Java_xxx_md5(JNIEnv*, jclass, jstring)`，unidbg 端用 `Jni` 接口应答。
军棋对应场景：so 里 `FindClass("com/baidu/mobads/AdView")`（广告层引用）。
**痛点**：军棋红线——全删 `smali/com/baidu/*` 前必须处理这个 native 引用点，
否则 so 找不到类 → `FindClass` 返回 null → so 崩。
**做法**：在 unidbg 端补一个 `FindClass` 应答，军棋 AdView 返回 null（去广告目标）：
```java
vm.setJni(new Jni() {
    @Override public DvmClass<?> findClass(String name) {
        // 军棋广告层拆除：com/baidu/mobads/AdView 返回 null（跳过广告初始化）
        // 一石二鸟：既补环境（FindClass 有应答不崩）又去广告（AdView 不初始化）
        if (name.equals("com/baidu/mobads/AdView")) return null;
        // 其他类正常应答（军棋 7 个 Java 类：App/GLView/SiteView/AdLis/BtReceiver/TextDlg/InputDlg）
        return vm.resolveClass(name);
    }
});
```
## 3.2 补环境：军棋三大卡点（大白话版）


> **本节核心**：
> 1. 三大卡点：.got 零值（填真 ARM libc）/ GL 无 GPU（挂桩）/ 18 常量（redroid dump 注入）
> 2. 前 3 个补环境能解，§124 三项阻塞（BL bug/对象图/mmap）补不了
> 3. .got 零值：__cxa_atexit 读零指针 SIGSEGV，填 6 个 GLOB_DAT（docker 真 ARM libc）
>
> **本节难点**：
> 1. GL 挂桩：libGLESv1_CM/v2 无真 GPU，glGetError/glBegin 返 0
> 2. 「能跑 ≠ 能玩」卡 §124：补环境是「指令能跑」工具，非「功能一致」工具（红线 §122）

![军棋三大补环境卡点](assets/ch03_env_fallback.png)



军棋 `libjunqi.so` 在 unidbg 里跑，卡三处（红线 §R6 实测）：

**★ 先分清两层（N44/N45 框架，防混）**：
- **外层（补环境能解）**：3 卡点 = "指令路径能跑 + 门牌号→房间真值填对"（.got/GL/18 常量）——**值静态读得出（门牌号），缺的是"房间真值"**
- **内层（补环境解不了）**：§124 三项 = "数据与指令对齐 + 模拟器精度"（BL bug/对齐/mmap）——**外层补了内层仍卡，需 frida dump 真值 / 真机 C++ 构造链**

| 卡点 | 大白话解释 | 怎么办 | 军棋实测 |
|---|---|---|---|
| `.got` 零值 | so 里的"电话簿"没填号（重定位没做，静态文件 `.got` 槽是零）；**N44：属"数据与指令对齐"外层缺口（门牌号静态可读，房间真值需填）** | 用 docker 真 ARM libc 解析 6 个 GLOB_DAT 填进去（填的是"房间真值"） | ✓ 全通 |
| `libGLESv1_CM` | 军棋要 GL1 渲染，但 PC 上没真 GPU | Hook 桩函数（`glGetError` / `glBegin` 等返回 0） | ✓ 挂桩 |
| 18 个 `.text` 常量 | so 内嵌的"门牌号"（15 `.text` + 2 `.data` = 72B）；**N45：门牌号静态可读，房间真值只有 dlopen 才填对** | 早期 = redroid 跑原版 dump 72B 房间真值注入（工具三件套）；**★ §124x-z 后演进 = 静态位移固化 `jqt_mem.c` `g_jq_got_tbl`（免运行时 dump，`jni_runtime_values.json` 已删）** | ⚠️ 注入后指令能跑 |
| **§124 三项阻塞** | 即使 3 卡点全通，`OnCreate` 推进到 0x43ed8 仍卡；**N44：② 是"数据与指令对齐"缺口（非"C++ 对象图不可知"），内层场景对象图属 C.1** | ① 上下文相关 BL 解码 bug（模拟器精度）② 数据与指令对齐对应缺口（N44）③ 288MB PROT_NONE mmap | ❌ 没通（补环境解不了） |

**关键区分**：前 3 个卡点是"补环境能解决的"（填 `.got` / 挂桩 GL / dump 常量），
第 4 个（§124 三项阻塞）是"补环境解决不了的"（模拟器精度 + 数据与指令对齐语义 + mmap mock），
军棋"能跑"≠"能玩"卡就卡在这（诚实边界，红线 §R6 + §124）。

## 3.3 脱离编译器，命令行编译 so（军棋场景不适用）


> **本节核心**：
> 1. 军棋是逆向现成 so（libjunqi.so **1.43MB/1435156B** strip），不是写新 so（书内 NDK 场景不适用）
> 2. 军棋 64 位简化重写版（rev64/src/junqi.cpp）才是 NDK r25b 编译（路线 B）
> 3. 「脱离编译器」= 不用 NDK 写 so，直接调现成 so（军棋 32 位走这条路）
>
> **本节难点**：
> 1. 军棋 32 位 so 是 API 19 时代产物（v152/1.52），无源码只有 ~7 个 smali 类

书里讲用 `clang --target=armv7a-linux-androideabi` 编译。
军棋是**逆向现有 so**（`libjunqi.so` **1.43MB/1435156B** 已 strip），不是写新 so，所以这节跳过。
但军棋 64 位简化重写版（`rev64/src/junqi.cpp`，NDK r25b）正是这么编译的（路线 B，红线 §64 位 native）。
## 3.4 unidbg 的 Hook（军棋实战代码）


> **本节核心**：
> 1. 三类 Hook 对应三层补环境：libc（__cxa_atexit 返 0）/ JNI（FindClass AdView 返 null）/ 系统调用（open 映射宿主）
> 2. Hook 保留原逻辑（只是应答），不破坏 so 字节；军棋 .text 100% 对齐基本不用 Patch
> 3. __cxa_atexit Hook 返 0 = 跳过 C++ 析构注册，避免读零 .got 崩
>
> **本节难点**：
> 1. Hook FindClass AdView 返 null = 去广告 + 补环境一石二鸟
> 2. Hook 系统调用 open/read/close = I/O 重定向（第 16 章），军棋 7 config XML + one.sjg 落宿主

![Hook / Patch / 补环境三件套](assets/ch03_hook_patch.png)



```java
// 1. Hook __cxa_atexit（避免读零 .got 崩）
// 军棋 .got 未重定位时是零值, __cxa_atexit 读零指针 → SIGSEGV
// Hook 它直接返回 0 = "跳过 C++ 析构注册", 崩溃就没了
int hookId = emulator.getBackend().hook(
    module.findSymbol("__cxa_atexit").address,
    (hook) -> hook.setReturn(0L));   // 直接返回 0，跳过真调

// 2. Hook FindClass("com/baidu/mobads/AdView")（广告层拆除）
// 军棋红线:全删 smali/com/baidu/* 前必须处理这个 native 引用点
emulator.getMemory().findExport("FindClass").when((hook) -> {
    if (hook.getArg(1).readString().equals("com/baidu/mobads/AdView")) return 0L; // 返 null 去广告
    return 0L;
});

// 3. Hook 系统调用（军棋读 data/config/*.xml）
// 军棋 so 发 open("/data/data/com.cnvcs.junqi/files/ui.xml")
// QEMU 拦下来, unidbg Java 端返回宿主 ./junqi_data/ 里的文件(第 16 章 I/O 重定向)
emulator.getMemory().setSvcHandler(SYS_open, (emulator, svcNumber, regs) -> {
    // 把军棋虚拟路径 /data/data/... 映射到宿主 ./junqi_data/
    String path = regs.readString();
    if (path.startsWith("/data/data/com.cnvcs.junqi/")) {
        path = "./junqi_data/" + path.substring("/data/data/com.cnvcs.junqi/".length());
    }
    return openHostFile(path);  // 返回宿主 fd
});
```

## 3.5 unidbg 的 Patch（军棋极少用）


> **本节核心**：
> 1. Patch 改指令（军棋极少用，.text 100% 对齐 1174/1174 PASS）
> 2. 唯一 Patch 场景：0x138xxx 高熵索引表（压缩表非加密代码）跳掉避免误读
>
> **本节难点**：
> 1. Patch 破坏原逻辑（跳过了），是最后手段；军棋优先 Hook + Debugger
> 2. 改 .text 字节需重签名（v1+v2），军棋 .text 对齐无需改

```java
// 把某条指令改成 nop（军棋场景：跳过必崩的函数）
// 军棋 .text 100% 对齐(1174/1174 PASS), 基本不需要 patch 指令
// 唯一 patch 场景: 0x138xxx 高熵索引表(压缩表, 非加密代码), patch 掉避免误读
emulator.getMemory().patch(module.base + 0x138000, new byte[]{0x00,0x00,0x00,0x00}, 4);
```
**为什么军棋少用 Patch？** 军棋 `.text` 是明文（熵 7.10 = 正常机器码），
指令语义 100% 对齐（1174/1174 PASS，红线 §R6 回归），**不需要 patch 指令**。
Patch 只用来"跳过 0x138xxx 的高熵索引表"（那是压缩/索引表，非加密代码）。
## 3.6 逐环节"我们能干什么 / 不能干什么"


> **本节核心**：
> 1. 7 行能/不能表：Hook 系统调用/libc/JNI/fork + Patch + 补 .got + 拿 18 常量
> 2. Hook 能拦截不能改控制流；Patch 能跳过不能改语义
>
> **本节难点**：
> 1. 补 .got 能从 docker 真 ARM libc 解析，不能静态 .so 读（零值未重定位）
> 2. 拿 18 常量 = 早期 redroid dump 72B 注入 / **★ §124x-z 后静态固化 `jqt_mem.c` `g_jq_got_tbl` 免 dump**，不能 unidbg 读（跑不到解包点）

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **Hook 系统调用** | 拦 `open`/`read`/`close`，返回宿主文件（I/O 重定向，第 16 章） | 改变 so 内部逻辑（Hook 只应答，不改控制流） |
| **Hook libc 函数** | `__cxa_atexit` 返 0（跳过 C++ 析构注册）、`malloc` mock | 绕过 §124 ③ 288MB PROT_NONE mmap（unidbg mmap2 mock 没做对） |
| **Hook JNI（DalvikVM）** | `FindClass(AdView)` 返 null（去广告）、`GetMethodID` 返 0（跳方法） | 让 14 个 `On*` 入口"真跑通 OnCreate 到 0x43ed8"（§124 ①② 卡住） |
| **Hook 系统调用 fork** | 返 0（军棋广告后台进程"走通但退"，第 21 章） | 模拟真子进程（unidbg 单线程，无 COW） |
| **Patch 指令** | 跳过 0x138xxx 高熵索引表（军棋极少用） | 用 Patch 改 `.text` 指令逻辑（军棋 `.text` 100% 对齐，不 patch） |
| **补 `.got`**（N44 外层数据对齐） | 用 docker 真 ARM libc 解析 6 个 GLOB_DAT 填"房间真值" | 从静态 `.so` 读 `.got` 运行时值（门牌号可读，房间真值未重定位是零） |
| **拿 18 常量**（N45 门牌号 vs 房间真值） | 门牌号静态可读；房间真值早期 redroid dump 72B 注入 / **★ §124x-z 后 = 静态位移固化 `jqt_mem.c` `g_jq_got_tbl`（不同进程基址通用，免 dump）** | unidbg 静态/动态读"房间真值"（门牌号能读，dlopen 才填对，unidbg 跑不通 0x43ed8） |
## 3.7 军棋实战验证（Hook + Patch + 补环境实测）


> **本节核心**：
> 1. 7 项实测：前 6 项全通（init_array + RELATIVE + PLT + GLOB_DAT + Hook + I/O），第 7 项 §124 没通
> 2. Hook __cxa_atexit 返 0 消除 .got 零值崩点；挂桩 GL 通；I/O 重定向通
>
> **本节难点**：
> 1. 突破 0x43ed8 卡 §124 ①②③（BL bug + 对象图 + 288MB mmap）
> 2. 「能跑 ≠ 能玩」诚实边界：补环境前 6 项全通，「功能一致」需 redroid + 真 arm64 GPU

军棋 32 位 so 在 unidbg（unicorn2）下的补环境实测（红线 §R6）：
| 补环境动作 | 军棋实测 | 结果 |
|---|---|---|
| Hook `__cxa_atexit` 返 0 | 避免读零 `.got` 崩（补"代码路径"，不补数据对齐） | ✅ 崩点消除 |
| Hook `FindClass(AdView)` 返 null | 去广告 + 补环境一石二鸟（补方向 ② so→Java 的 VM 侧电话） | ✅ 通 |
| 挂桩 `libGLESv1_CM` | `glGetError`/`glBegin` 返 0 | ✅ 通（无真 GPU） |
| 填 `.got` 6 GLOB_DAT | 从 docker 真 ARM libc 解析（填"房间真值"，N44 外层数据对齐） | ✅ 通 |
| I/O 重定向（open/read） | 7 config XML + one.sjg 落宿主（门牌号→宿主文件，N45） | ✅ 通 |
| dump 18 `.text` 常量 | 早期 redroid dump 72B 房间真值注入；**★ §124x-z 后 = 静态固化 `jqt_mem.c` `g_jq_got_tbl` 免 dump** | ⚠️ 注入后指令能跑 |
| 突破 0x43ed8（§124 ①②③） | BL 解码 bug + 数据与指令对齐（N44）+ PROT_NONE mmap | ❌ 没通（补环境解不了，"能跑"≠"能玩"） |
**诚实边界**：军棋补环境前 6 项全通（12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT），
但"能玩"卡 §124 三项阻塞——**Hook/Patch 是"指令能跑"的工具（补外层门牌号→房间真值），不是"数据与指令对齐 + 功能一致"的工具**（红线 §122，N44：外层补了内层仍卡）。
## 本章小结（一句话版）

> 补环境 = 把 so 假设的"真 Android 环境"一个个补上。
> 军棋三大卡点：`.got` 零值（填真 ARM libc 的"房间真值"，N44 外层数据对齐）、libGLESv1_CM 无 GPU（挂桩）、
> 18 个 `.text` 常量（N45 门牌号静态可读，房间真值 redroid dump 注入 / §124x-z 静态固化 `jqt_mem.c` 免 dump）——前 3 个补环境能解决。
> 但军棋"能玩"卡 §124 三项阻塞（① BL 解码 bug（模拟器精度）② **数据与指令对齐对应（N44，外层缺口，内层场景对象图属 C.1）** ③ 288MB mmap），
> 这是补环境解决不了的"模拟器精度 + 数据对齐"问题（**Hook 补"代码路径能跑"，不补"数据对齐"**）。
> Hook 拦截优先，Patch 改指令是最后手段（军棋 `.text` 100% 对齐，基本不 patch）。

## 视频

- `videos/ch03_intro.mp4`（~10s 讲解动画：Hook 拦截 vs Patch 改指令 / 三大卡点 / 补环境优先级）
