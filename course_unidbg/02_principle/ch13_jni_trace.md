# 第13章 unidbg 源码解析：JNI 交互流程追踪

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的 JNI 层

## 痛点：为什么 so 和 Java 之间"打电话"总有一头没人接？

JNI 是 so ↔ Java 的"电话线"，军棋实测两头都得接好才通。3 个坑。

### 痛点 1：JNI 是 so 调 Java 的"电话线"，两头必须都有人接

军棋 so 导出 14 个 `Java_com_cnvcs_App_On*` 入口，Java 侧 `com.cnvcs.App` 调它们；
反过来 so 内部又 `FindClass`/`callXxxMethod` 回调 Java。任一头没接
（模拟环境没建 VM、或 so 没导出），电话就断。

- **大白话卡点**：电话线两头都得有人接——so 这头（14 个 `On*` 入口）+ Java 这头
  （DalvikVM 应答）。缺一头电话就断。
- **为什么卡**：unidbg 没建 VM 时，so 调 Java 没人应答；so 没导出入口时，
  Java 调不到 so。
- **不解决会怎样**：`OnCreate` 调不通或 `FindClass` 返回 null 崩。

**JNI 双向"电话线"（大白话）**：
- **Java → so**：Java 侧 `com.cnvcs.App` 调 `OnCreate`/`OnTouch` 等 14 个入口
  （so 导出 `Java_com_cnvcs_App_On*`，Java 按名字找到调）
- **so → Java**：so 内部 `FindClass`/`callXxxMethod` 回调 Java（so 主动"打回来"）
两个方向都要接住，unidbg 用 DalvikVM 接 so → Java 方向，用导出符号接
Java → so 方向。

### 痛点 2：`JNI_OnLoad` 是"开机自检"，跳过它 so 可能起不来

so 加载后 Android 自动调 `JNI_OnLoad`，里面注册方法、建全局对象。军棋 so 的
`JNI_OnLoad` 里还有 `FindClass("com/baidu/mobads/AdView")`（广告层引用）
——没处理就崩。

- **大白话卡点**：so 开机先"自检"（`JNI_OnLoad`），自检里还查广告类。
  查广告类没处理 → 崩。
- **为什么卡**：`JNI_OnLoad` 是加载后自动跑的，军棋它查了 `AdView`。
- **不解决会怎样**：`FindClass(AdView)` 返回真类 → 初始化广告 → 广告浮层。

**为什么 `JNI_OnLoad` 要"自动跑"？** 它是 so 的"开机自检"，dlopen 后系统自动调，
在里面注册 native 方法（`RegisterNatives`）、建全局对象。军棋 `JNI_OnLoad` 录了
14 个 `On*` 方法 + 查了 `AdView`。不跑它，14 个入口没注册、广告引用没处理。

### 痛点 3：18 个 `.text` 常量池是"只有开机后才填的号"

军棋 so 里 `ldr rX,[pc,#N]` 读的 18 个地址（15 个 `.text` 常量池 + 2 个 `.data`，
共 72B）只有 dlopen 重定位后才填值，静态文件**房间真值**读不出（门牌号静态可读，N45；不是壳自解密，`.rodata` 静态
即明文已证，红线 §R6）。JNI 调用要读这些值时就卡住。

- **大白话卡点**：这 18 个"号"是"开机后才发"的，静态文件里是占位符。
- **为什么卡**：它们不在 3 类重定位里（RELATIVE/JUMP_SLOT/GLOB_DAT 都填过了），
  是运行时才解析的常量池值。
- **不解决会怎样**：读不到 18 个**房间真值**（N45：门牌号静态可读，房间真值需 dlopen 才填对），依赖它们的逻辑全错，JNI 调用卡住。

**为什么 18 个常量池"只有开机才填"？** `ldr rX,[pc,#N]` 读的是".text 里的常量池"
（编译时把常量放常量池、代码里用 `pc` 相对偏移读）。常量池值在 dlopen 重定位
时填（linker 把相对偏移换算成绝对地址），静态文件是占位。军棋 18 个地址
（15 `.text` + 2 `.data`，72B）属这类——**静态读得出门牌号，读不出房间真值（N45）**，房间真值早期只能 redroid dump（第 8/9 章）；**★ §124x-z 后 = 静态位移固化 `jqt_mem.c` `g_jq_got_tbl` 免 dump**。

## 为什么这样做：把 JNI 交互拆成"注册→执行→应答"三阶段

类比"公司总机系统"（三阶段递进）：

1. **注册 = 拨号规则录入**（`JNI_OnLoad`）：so 告诉虚拟机"我有 14 个方法要注册"，
   虚拟机记下来。军棋 `JNI_OnLoad` 里还查了广告类（`FindClass("com/baidu/mobads/AdView")`）。
2. **执行 = 实际通话**（14 个 `On*` 入口被 Java 侧调用）：`OnCreate` → GLView native
   层处理 → 内部可能再 `FindClass` 广告类。
3. **应答 = 总机接电话**（unidbg 的 DalvikVM/Jni 接口）：`getJNIEnv`/`getJavaVM`/
   `callStaticJniMethod*`——so 拨来的每个电话，模拟侧都要应答。

**为什么拆三阶段？** 注册（so 告诉 VM 它有哪些方法）→ 执行（Java 调这些方法）→
应答（VM 接 so 回调的电话），三阶段有严格先后。军棋实测（红线 §R6 DiagJni）：
注册 + 执行 + 应答三步全通，**`OnCreate` 路径**推进到 `std::vector` 对象图 0x43ed8
（0x43ed8 是 OnCreate 的 code cave 下游——DiagJni33/35 实证 `createDalvikVM +
resolveClass + callStaticJniMethodObject` 全通后，OnCreate 链推进到该地址的
std::vector 构造点；**OnUpdate/OnEvent 等其它 JNI 入口不走 0x43ed8**，各自有自己的
数据与指令对齐卡点）。卡点不在 JNI 层（电话全接通了），在 native 层 数据与指令对齐未初始化。

**三阶段对应"总机"的哪部分？**
- 注册 = 总机"录入拨号规则"（so 告诉 VM：我有哪些方法、签名是什么）
- 执行 = 总机"实际通话"（Java 按规则拨号，so 处理）
- 应答 = 总机"接回调电话"（so 调 Java，VM 应答）

**预判追问："为什么 unidbg 不直接跑真 Dalvik/ART 接 JNI 电话？"**
真 Dalvik/ART 是 C++ 写的系统级虚拟机，跑在真 Linux 内核上，依赖真内核的内存管理
+ 真 Java 类加载（dex 文件）。unidbg 模拟的是"用户态"，没有真内核。所以 unidbg
写了"简化 DalvikVM"，只模拟 JNI 交互面（4 类电话：查类/查方法号/调方法/查对象），
不模拟字节码执行。军棋核心逻辑在 native（so 调的是 JNI 入口，不是"跑一段 Java
字节码逻辑"），简化 DalvikVM 够用了。真 Dalvik = "全系统模拟"（太重），简化 =
"只接电话"（军棋够用）。

## 13.1 JNI 注册（`JNI_OnLoad`：拨号规则录入）


> **本节核心**：
> 1. JNI_OnLoad 拨号规则录入（14 入口 + 7 类 + AdView）
>
> **本节难点**：
> 1. 跳过 JNI_OnLoad so 起不来；军棋 callJNI_OnLoad 录 14 入口 + 查 AdView
> 2. JNI_OnLoad 是「开机自检」，跑 14 入口注册 + 查 AdView 去广告
> 3. 军棋 14 入口全注册才可调

![军棋 JNI 注册](assets/ch13_jni_register.png)



JNI 注册是 `JNI_OnLoad` 的核心任务之一。军棋 32 位 so 的 JNI 注册：

```c
// 军棋 so 的 JNI_OnLoad（类比：开机自检时录入拨号规则）
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    // 拿到"电话本首地址"（so 后面调 Java 都靠它）
    (*vm)->GetEnv(vm, &env, &jni_version);
    // 注册 14 个 On* 静态方法（录 14 条拨号规则）
    // FindClass("com/baidu/mobads/AdView")（查广告类——拆除目标，返回 null 跳过）
    return JNI_VERSION_1_6;                  // 声明支持 JNI 1.6
}
```

军棋 32 位 so 的 14 个 `Java_com_cnvcs_App_On*` 导出符号（红线 §R6 已确认，
readelf 实测）：
- OnCreate / OnDestroy / OnResult / OnResume / OnEvent
- OnKey / OnUpdate / OnGraphChange / OnBluetooth / OnPause
- OnGraphCreate / OnTimer / OnInput / OnTouch

**为什么 `JNI_OnLoad` 要查 `AdView`？** 军棋 so 在自检时就把广告类的引用"录进去"
（为后续广告初始化做准备）。返回 null = "广告类不存在"，so 跳过广告初始化
（去广告目标）。这是军棋"一石二鸟"的关键点（补环境 + 去广告）。

**14 个 `On*` 的方法签名（大白话）**：全是静态方法（注册在 `com.cnvcs.App` 类上），
入参 `(JNIEnv*, jclass, ...)`。`OnCreate`/`OnDestroy` 是生命周期（开/收局），
`OnKey`/`OnInput`/`OnTouch` 是输入，`OnEvent`/`OnUpdate`/`OnGraphChange` 是事件，
`OnBluetooth` 是联机，`OnTimer` 是定时，`OnPause`/`OnResume` 是暂停/恢复，
`OnResult` 是结果，`OnGraphCreate` 是棋盘创建。

## 13.1.1 参数 marshal（JNI 函数表怎么把 Java 参数传给 native）


> **本节核心**：
> 1. DalvikVM 调 native 前按签名把 Java 参数转 native 指针
> 2. 军棋 14 入口签名不统一（int/jobject/jstring 混），转错（jobject 当 int）就崩
>
> **本节难点**：
> 1. env = JNI 函数表指针（232 槽），obj = DvmObject 哈希（非 C++ 对象指针）
> 2. marshal 是「军棋特有的坑」：签名不统一，逐个按签名转

原书 13.2"JNI 指令执行"的核心不只是"14 个入口被调"，还有**参数 marshal**——
`RegisterNatives` 注册后，DalvikVM 调 native 函数时，怎么把 Java 参数转成 native 指针。
**军棋 `OnCreate(JNIEnv *env, jobject obj)` 的 marshal 过程**：
```
Java 侧 (DalvikVM):
  com.cnvcs.App.onCreate(GLView obj)
    ↓ DalvikVM 查注册表, 找到 native 函数指针 (OnCreate @ 0x233e1 &~1)
    ↓ 构造参数:
       - JNIEnv* = DalvikVM 内部的 JNI 函数表指针(vm.getJNIEnv())
       - jobject = GLView 对象的哈希(vm.addGlobalObject(...) 返回的 int)
    ↓ 跳 native 函数:
       0x233e0:  OnCreate(JNIEnv* env, int objHash)
         → env 指向 JNI 函数表(232 槽), 里面 FindClass/GetMethodID/CallXxxMethod 全接 unidbg 应答
         → objHash 是 DalvikVM 里 GLView 对象的引用(不是 C++ 对象指针)
```
**军棋 14 个入口的 marshal 差异**：
| 入口 | Java 参数 | marshal 成 native | 军棋注意 |
|---|---|---|---|
| `OnCreate` | `GLView obj` | `jobject`(哈希) | 主入口，推进到 0x43ed8 |
| `OnTouch` | `int x, int y` | 直接 int | 无 marshal 开销 |
| `OnEvent` | `String name, String arg` | `jstring`(哈希) + `jstring` | 布局选择走这里 |
| `OnBluetooth` | `String action` | `jstring` | 蓝牙联机 |
| `OnTimer` | `int period` | 直接 int | 定时事件 |
| ... 14 个 | 见 `.dynsym` 签名 | 按签名 marshal | 全部 `&~1` 对齐 |
**为什么 marshal 是"军棋特有的坑"**：军棋 14 个入口签名不全统一（有的 `int` 有的 `jobject` 有的 `jstring`），
DalvikVM 调 native 前得按签名把参数转对——转错（比如 `jobject` 当 `int` 传）就崩。
unidbg 的 `setJni` 实现里 `callXxxMethod*` 要按军棋 14 个入口的签名逐个 marshal。
军棋 32 位 so 的 JNI 指令执行路径：
```
Java_com_cnvcs_App_OnCreate(env, obj)   ← Java 侧 com.cnvcs.App 调这个入口
  → GLView native 层处理（棋盘初始化、读配置）
  → FindClass("com/baidu/mobads/AdView")  ← 广告层引用（拆除前必须处理，否则崩）
  → 14 个 On* 静态方法调用
```
unidbg 的 JNI 指令执行（军棋 DiagJni 实证，4 步接电话）：
```java
// 1. 拿 JNIEnv 指针（电话本首地址）
Pointer jniEnv = vm.getJNIEnv();
// 2. 拿 JavaVM 指针（总机实体）
Pointer javaVm = vm.getJavaVM();
// 3. 模拟静态 JNI 方法调用（替 so 拨这个号）
long ret = vm.callStaticJniMethodObject(dm, "OnCreate", jniEnv, objHash);
// 4. 军棋 DiagJni：OnCreate 推进到 std::vector 0x43ed8
//    （电话全接通，卡点转 native 数据与指令对齐（N44））
```
**14 个入口的分工（大白话）**：
| 入口 | 大白话 | 对局闭环角色 |
|---|---|---|
| `OnCreate`/`OnDestroy` | 开局/收局 | 生命周期起止 |
| `OnResult`/`OnEvent`/`OnUpdate` | 结果/事件/更新 | 走子后刷新 |
| `OnKey`/`OnInput`/`OnTouch` | 键盘/输入/触摸 | 玩家操作 |
| `OnGraphChange`/`OnGraphCreate` | 棋盘变化/创建 | 棋盘渲染 |
| `OnBluetooth` | 蓝牙联机 | 联机同步（UUID `ce1404fe-...`） |
| `OnTimer`/`OnPause`/`OnResume` | 定时/暂停/恢复 | 计时与状态 |
## 13.3 逐环节"能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **JNI 注册（JNI_OnLoad）** | 录 14 个 `On*` 静态方法 + 查 AdView（返 null 去广告） | 让 AdView 返真类（要初始化广告 → 广告浮层，去广告目标相反） |
| **JNI 执行（14 入口被调）** | `callStaticJniMethodObject` 14 个入口全通（DiagJni 推进到 0x43ed8） | 突破 0x43ed8（§124 ② 数据与指令对齐对应缺口（N44），std::vector 未初始化） |
| **JNI 应答（DalvikVM）** | `getJNIEnv`/`getJavaVM`/`callStaticJniMethod*` 全通；AdView 返 null | 补数据与指令对齐（Java 侧全接通后卡点转 native，§124 ②） |
| **读 18 常量**（N45：门牌号静态可读，房间真值需 dlopen 才填对） | 门牌号静态可读（`.so` 偏移如 `0x0235c4`→`0x00138918`）；早期 redroid dump 72B 房间真值注入；**★ §124x-z 后 = 静态位移固化 `jqt_mem.c` `g_jq_got_tbl`（免 dump）** | 从静态 `.so` 直接读**房间真值**（门牌号能读，房间真值未重定位是占位；unidbg 跑不通 0x43ed8） |

**军棋实测对照**：
- JNI 注册 ✓（14 入口录好 + AdView 查 null）
- JNI 执行 ✓（`OnCreate` 推进到 0x43ed8）
- JNI 应答 ✓（DalvikVM 全接，getJNIEnv/getJavaVM 全通）
- 读 18 常量"房间真值" ⚠️（静态读得出**门牌号**，读不出**房间真值**（N45）；早期需 redroid dump 72B 房间真值注入，**★ §124x-z 后 = 静态位移固化 `jqt_mem.c` `g_jq_got_tbl` 免 dump**）
- **✗ 突破 0x43ed8**：§124 ② 数据与指令对齐对应缺口（N44）（JNI 层全通后卡点转 native）

## 13.4 军棋实战验证：JNI 三阶段全通但卡 native


> **本节核心**：
> 1. JNI 三阶段全通但卡 native（§124 三项阻塞）
>
> **本节难点**：
> 1. 三阶段（注册/执行/应答）全通，native 内部卡 §124
> 2. 「能注册 ≠ 能调通」：注册通，调用卡 0x43ed8

军棋 unidbg 的 JNI 交互实测（红线 §R6 DiagJni）：
| 阶段 | 军棋结果 | 大白话 |
|---|---|---|
| 注册（`JNI_OnLoad`） | 14 入口录好 + `AdView` 查 null ✓ | 拨号规则录入 |
| 执行（`OnCreate` 被调） | 推进到 0x43ed8 ✓ | 实际通话接通 |
| 应答（DalvikVM 接电话） | 全接 ✓ | 总机有人接 |
| 卡点 | native 数据与指令对齐 0x43ed8（N44，原"对象图"） | 卡在 native 层 |
**关键认知**：军棋 JNI 层（电话）全接通了，瓶颈转移到 native 层（C++ 对象图）。
§124 的 3 项阻塞（① 上下文相关 BL 解码 bug ② 数据与指令对齐对应缺口（N44） ③ 288MB
PROT_NONE mmap）全在 native 层，不在 JNI 层。
**"JNI 全通 ≠ 能玩"（诚实边界）**：军棋 JNI 三阶段全通（DiagJni 实证），
`OnCreate` 推进到 0x43ed8。但这只是"Java 电话接通"，离"游戏能跑"还差
native 数据与指令对齐（N44）（§124 的 ②）+ redroid dump 的 18 常量池。
JNI 通是"接通电话"，对象图通才是"能聊业务"。

## 13.4b 看 JNI 调用流：unidbg 侧 vs Frida Jnitrace 侧（吸收自《Frida Android SO 逆向深入实践》）

上面 13.2/13.4 全从 **unidbg 侧**看 JNI 调用流（DalvikVM 接 FindClass/GetMethodID/CallXxx）。
同一件事在**真机侧**有另一个工具：**Frida Jnitrace**（hook `RegisterNatives` + JNI 函数调用流）。
这是**同一个问题的另一个工具**（同看"so 在 JNI 边界调了什么 Java 方法"），军棋两条路都能走：

| 维度 | unidbg 侧（13.2/13.4 已讲） | Frida Jnitrace 侧（本节） |
|---|---|---|
| **工具** | `DalvikVM` trace / `callStaticJniMethod*` | Frida `Jnitrace`（hook `RegisterNatives` + `CallXxxMethod`） |
| **场景** | 离线模拟 so（无真机、无 GPU） | 真机跑 app（真对象图、真 GPU） |
| **看什么** | so 调了哪些 JNI 函数（FindClass/NewObject/...） | 同左，但**运行时真实值**（不是 mock） |
| **军棋现状** | ✅ JNI 三阶段全通（DiagJni），但动态 trace 受 §124① BL bug 限制 | ✅ **绕开 unidbg BL bug 的真机替代路**（真机跑 32 位 so，JNI 调用流真实） |
| **跟 §124q 调用链判据的关系** | 动态调用链在 unidbg 下被 BL bug 卡死（0x23578 bl 0xbdacc → 0x13c576 崩） | Frida 真机 trace **不受 unidbg BL bug 影响**（不经过 unicorn 模拟器）——**§124q 动态调用链的真机替代路** |

**为什么军棋两条路都建**：unidbg 是"离线把 so 搬进 x86 跑"（军棋 §124 三项阻塞卡死），Frida 是"真机跑原版 so、hook 看调用流"。
军棋 32 位 so 在 **32 位真机（或 redroid + 32 位 frida-server）** 上能 dlopen + 跑 init_array（frida-dump 已实证，见 `frida-dump/`），
所以 **Frida Jnitrace 真机 trace 是 §124q 动态调用链在 unidbg BL bug 未修期间的唯一可用路**（静态调用图是 capstone 反汇编兜底，Jnitrace 是真机动态）。

**军棋工作流已落地的 Frida 用法**（详见 `frida-dump/FRIDA_NOTES.md`，独立参考文档）：
- `redroid_jni_dump.py`：frida attach 读 18 个运行时地址（15 .text 常量池 + 2 .data + 1 .rodata 指针表）→ `jni_runtime_values.json` → 喂 `jni_aligned.c` 的 `jq_load_runtime_values` 注入层
- `Jnitrace`（未建脚本，可补）：hook `RegisterNatives` + 14 入口的 `CallXxxMethod` → 真机 JNI 调用流 → §124q 动态调用链真机替代

**Jnitrace vs unidbg trace 怎么选（大白话）**：
- **要"不崩的运行时调用链"且能上真机/redroid** → Frida Jnitrace（真机跑，不经过 unicorn，无 BL bug）
- **要"离线可重放的调用链"且 unidbg BL bug 已修** → unidbg traceCode（修好前走静态调用图 + Jnitrace 兜底）

## 13.4c unidbg + Frida 一对一逐步对齐（§124z 黑屏破题方向，用户方法论）

> **路线限定（别混，附录 B.5c.5d 用户纠正）**：本节是**模拟路线**（unidbg 不重编译直接模拟原版 so 跑，要喂运行时对象图真值）。
> **不是逆源码路线**（反汇编 so → 逆出逻辑 → 重写成能编译的 C/C++ → 正常开发）——逆源码路线重写后堆/对象图程序自己跑，**不需要 Frida dump 对象图**（"要毛线运行时堆分配"）。
> 两节对照见 `APPENDIX_B_ELF_BUILDING.md` B.5c（两条路线 + 6 步）。

13.4b 讲"Frida 是动态调用链的真机替代路"，本节讲**"unidbg + Frida 两个工具怎么配成一对，逐步对齐对象图"**——
这是军棋 §124z 黑屏根因（纹理类 vtable 0x1595c0 的 glClear 方法所在对象未构造；★ §124zz-3 纠正：旧"vtable=0x1595f4"是幻影，实为 0x1595c0 的 slot[13]）的破题方向（详见 `frida-dump/FRIDA_NOTES.md` 第八节）。

**为什么 unidbg 单独做不到"数据侧"**：
- unidbg 是**离线模拟**——能"加载 so + 分析调用链 + 跑 so 逻辑"（指令 100% 对齐，1174/1174 PASS），
  但**读不出"数据与指令对齐后的运行时真值"**（unidbg 跑到 0x43ed8 崩，§124 ② 数据与指令对齐对应缺口）
- unidbg 的 mock 值是"填 0 / 填固定值"，**不是"真机跑出来的真值"**
- 所以 unidbg 数据侧不行，**Frida 真机 dump 对象图真值补上**

**3 角色分工（"一对一"的含义）**：

| 角色 | 工具 | 干什么 | 军棋现状 |
|---|---|---|---|
| **模拟加载 + 调用链 + 逻辑** | unidbg | 离线搬 so 进 x86 + §124q 调用链判据 + 1174/1174 指令对齐 | 加载全通（12/5312/214/6），卡 §124 三项 |
| **真实填充对齐真值** | Frida（MuMu 真机 attach） | 读**运行时真值**（数据与指令对齐后的 vector begin/end、glClear 类对象 vtable、渲染布局） | 18 常量已静态固化（§124z-3）；**对齐真值还没 dump**（待办） |
| **一对一逐步对齐** | unidbg mock 值 vs Frida 真值 逐点比 | 把 unidbg 读 0 的对象图字段，逐个填上 Frida 真值 → 对象图对齐 | **§124z 黑屏破题路** |

**逐步对齐流程**：
```
1. MuMu 跑原版军棋（32 位）→ 游戏"能玩"的状态（数据与指令对齐构造完、渲染出帧）
2. Frida attach（MuMu 上 32 位 frida-server，跟 redroid 同套路）→ dump 对象图真值
   → object_graph_values.json（跟 jni_runtime_values.json 同套路，但 dump 对象图字段）
3. unidbg 侧（或翻译体影子内存）逐字段填真值：
   - vector.begin/end/cap 填 Frida 真值
   - 纹理类 vtable 0x1595c0 的 glClear 方法所在对象构造链（0x23eec）喂真值（§124zz-3：旧"0x1595f4 类对象"是幻影）
   - 渲染对象 0x15be88 填 Frida 真值
4. 比"unidbg 填真值后" vs "Frida 真机跑" → 逐字段对齐（§124q 调用链 + 对象图 + 返回值三判据）
5. 对齐 = 对象图对齐 = 黑屏破题（glClear 类对象构造出来 → 渲染出帧 → 能玩）
```

**为什么 MuMu 能直接用 Frida（不用另搭环境）**：
MuMu 跟 redroid **同套路**——都是 **x86 宿主 + houdini 转译 ARM32**，所以 Frida hook 32 位 so 都用 **32 位 frida-server**：
| 环境 | 宿主 | ARM 转译 | Frida hook 32 位 so |
|---|---|---|---|
| redroid | x86 容器 | houdini | 32 位 frida-server（已落地 `frida-dump/`） |
| **MuMu** | x86_64 PC | houdini/libnb（`ro.dalvik.vm.isa.arm=x86`） | **32 位 frida-server**（跟 redroid 同套路，待建） |

MuMu 是"x86 PC + 真 GPU + 能 dlopen 32 位军棋"的现成环境（§124u-z 已实测 32 位翻译体在 MuMu 渲染主菜单），
**推 32 位 frida-server + `frida attach com.cnvcs.junqi` 就行**。

**跟 §124q 调用链判据的关系**：
- 13.4.1 调用链判据（ch05 5.4.1）= "调用链先对齐"（bl 目标序列逐层下钻）
- 13.4c 一对一逐步对齐 = "**对象图逐字段对齐**"（调用链对齐 + 对象图对齐 + 返回值 A/B 逐点比）
- **调用链对齐 ≠ 对象图对齐**——§124z 已证"关键路径 0 缺口"（调用链对齐了）但黑屏（对象图没对齐），
  **13.4c 的"对象图逐字段填 Frida 真值"正是补这个缺口**

**诚实边界**：
- Frida dump 的是"真机跑出来的对象图真值"，填进 unidbg 是"模拟真值"，**不等于"能玩"**（"能玩"要 GL 出帧 + 触摸进对局，仍需真机验证）
- MuMu 上 Frida attach 要 **32 位 frida-server**（跟 so 位宽匹配），不是 64 位
- 对象图真值 dump 比 18 常量复杂（18 常量是 72B 固定地址，对象图是"构造链运行时才知道的真值"，需逆向对象图布局才能 dump 全）

## 13.5 本章小结（一句话版）

> 军棋 JNI 交互拆 3 阶段：注册（`JNI_OnLoad` 录 14 个 `On*` + 查广告类）→ 执行
> （`OnCreate` 被 Java 调）→ 应答（unidbg DalvikVM 接电话）。
> 军棋 DiagJni 实证三步全通，`OnCreate` 推进到 `std::vector` 对象图 0x43ed8；
> 卡点不在 JNI 层（电话全接通），转 native 层 §124 的 3 项阻塞：
> ① 上下文相关 BL 解码 bug（unicorn2 精度，BL 0x7965a 错位到 0x6eaea）
> ② 数据与指令对齐对应缺口（N44）（std::vector 0x43ed8）③ 288MB PROT_NONE mmap。
> 军棋 18 个 `.text` 常量池：静态读得出**门牌号**，**房间真值**只有 dlopen 重定位才填对（非壳自解密，N45），unidbg 读不出房间真值，
> 需 redroid dump。
> §124q 动态调用链被 BL bug 卡死时，**Frida Jnitrace 真机 trace 是替代路**（13.4b，
> 绕开 unidbg 模拟器，真机跑 32 位 so 看 JNI 调用流）；详见 `frida-dump/FRIDA_NOTES.md`。
> **13.4c：unidbg + Frida 一对一逐步对齐**（§124z 黑屏破题方向）——unidbg 负责"模拟加载 + 调用链 + 逻辑"
> （数据侧读不出运行时真值），Frida（MuMu 32 位 frida-server）负责"真实填充对象图真值"，
> **逐字段把 unidbg 的"零值对象图"填成"真对象图"** = 纹理类 vtable 0x1595c0 的 glClear 方法所在对象构造出来（§124zz-3：旧"0x1595f4 类"是幻影，slot[13]=0x9d459）→ 渲染出帧。

---

## 视频
