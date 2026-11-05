# 第11章 unidbg 源码解析：DalvikVM

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的 Java 侧

## 痛点：为什么 so 一调 Java 方法就空指针？

so 想"调 Java 方法"就得给 Java 侧"打电话"（JNI 调用），但 PC 上没这个"客服"。
军棋实测踩过 3 个坑。

### 痛点 1：so 假设跑在 Android 上，PC 上没 Dalvik 虚拟机

军棋 so 内部 `FindClass("com/baidu/mobads/AdView")`、调 `GLView` 的方法——
这些是"给 Java 侧打电话"。你 PC 上没 Dalvik（Android 的 Java 字节码虚拟机），
电话没人接 → 返回 null → 空指针崩。

- **大白话卡点**：so 想给"Java 客服"打电话（查类、调方法），你 PC 上没这个客服，
  电话没人接 → 空指针。
- **为什么卡**：unidbg 没建 DalvikVM（Java 虚拟机替身），JNI 交互面没人应答。
- **不解决会怎样**：so 调 `FindClass` 返回 null → 拿 null 调方法 → 空指针崩。

**"打电话"为什么必须有人接（大白话）**：so 调 `FindClass("...GLView")` 是"给
Java 侧打电话问 GLView 类在哪"。真机有 Dalvik 接电话（返回类指针），PC 上没
Dalvik → 电话没人接 → 返回 null → so 拿 null 调方法 → 崩。unidbg 的 DalvikVM
就是"替 Dalvik 接这个电话"（返回模拟的 `DvmClass` 替身）。

### 痛点 2：军棋 Java 侧极小，容易被忽略

军棋只有 ~7 个 Java 类（`App`/`GLView`/`SiteView`/`AdLis`/`BtReceiver`/`TextDlg`/
`InputDlg`，几百行 smali），核心逻辑全在 native 层。但"小"不等于"能少模拟"
——so 调哪个类、哪个方法，模拟侧就得应答哪个。

- **大白话卡点**：军棋 Java 侧只有 7 个"客服"，看着少，但每个都得"接得通"，
  少接一个对应调用就崩。
- **为什么卡**：7 个类 + 14 个 JNI 入口 + 广告类，每类都要模拟应答。
- **不解决会怎样**：`GLView` 没模拟 → `OnCreate` 调它的方法崩。

**"小"为什么"不能少模拟"（大白话）**：军棋 Java 侧 7 个类看着"就这几个"，但
so 调哪类哪方法，模拟侧就得应答哪个。`GLView`（渲染类）少模拟 → `OnCreate` 调
它的方法崩；`AdLis`（广告监听）没处理 → 广告层崩。"小"是"类少"，但"每类都得
接得通"，一个都不能漏。

### 痛点 3：`FindClass` 是广告层拆除的关键引用点

军棋 native 层 `FindClass("com/baidu/mobads/AdView")` 直接引用广告 SDK 类
（红线：全删 `smali/com/baidu/*` 前必须处理这个引用点）。DalvikVM 模拟侧决定
"返回真类还是 null"。

- **大白话卡点**：so 查"广告客服在不在"，你决定"在"（初始化广告）还是"不在"
  （跳过广告）——这正是去广告的开关。
- **为什么卡**：全删 baidu smali 前不处理这个引用，so 崩。
- **不解决会怎样**：删了 baidu smali 后 so 调 `FindClass(AdView)` 崩。

**"去广告开关"为什么在 `FindClass`（大白话）**：军棋 so 查 `AdView` 类是"广告
初始化的入口"——查到了就初始化广告（出浮层），查不到就跳过（去广告）。
全删 baidu smali（广告类没了）前，必须让 `FindClass(AdView)` 返回 null
（"查不到"），否则 so 崩。这是军棋"补环境 + 去广告"的关键引用点。

## 为什么这样做：在 PC 上"建一个假 Java 虚拟机"

`DalvikVM`（unidbg 模拟的 Java 虚拟机，类比"一个只回答 so 关心的电话的假电话总机"）
的设计动机：

- **不跑真字节码**：unidbg 不模拟 Dalvik 字节码执行（太复杂），只模拟 **JNI 交互面**
  ——so 调 `FindClass`、`GetMethodID`、`CallXxxMethod` 这 4 类电话，你应答这 4 个端口即可。
- **按类应答**：`resolveClass("com.cnvcs.GLView")` 返回一个模拟的 `DvmClass`
  （Java 类的替身），`findMethodID` 返回模拟的方法号。
- **军棋场景**：so 调 14 个 `On*` JNI 入口 + `FindClass` 广告类。模拟侧要应答的就是
  这几类电话。

**为什么"不跑真字节码"就够？** 军棋 Java 侧极小（7 类），so 调的是 JNI 入口
（native 函数），不是"跑一段 Java 字节码逻辑"。模拟"查类、查方法号、调静态
方法"这几个交互面，就能接住 so 的所有 JNI 电话。真字节码执行是"Java 逻辑跑
在哪"，军棋核心逻辑在 native，用不上。

**"假电话总机"设计（大白话）**：
- 不跑真字节码 = 不真"执行 Java 逻辑"（太复杂，军棋用不上）
- 只接 4 类电话 = 查类（`FindClass`）/查方法号（`GetMethodID`）/调方法
  （`CallXxxMethod`）/查对象（`GetObject`），只应答这 4 个端口
- 按类应答 = 每个类给个替身（`DvmClass`），每个方法号给个模拟值
军棋 7 个类 + 14 个 `On*` 入口 + 广告类，全在"4 类电话"应答范围内。

**军棋实测**（红线 §R6 DiagJni）：`createDalvikVM` + `resolveClass` +
`callStaticJniMethodObject` 三步走通，`OnCreate` 推进到 `std::vector` 对象图
0x43ed8——**Java 侧电话全接上了，卡点转移到 native 侧的 C++ 对象图**。

**预判追问："为什么不直接用真 Dalvik/ART 虚拟机？"**
真 Dalvik/ART 是 C++ 写的系统级虚拟机，跑在真 Linux 内核上，依赖真内核的内存管理
+ 真 Java 类加载（dex 文件）。unidbg 模拟的是"用户态"，没有真内核，mmap 是 Java
端模拟的。所以 unidbg 写了"简化 DalvikVM"，只模拟 JNI 交互面（4 类电话），不模拟
字节码执行（军棋用不上）。真 Dalvik = "全系统模拟"（太重），简化 DalvikVM =
"只接电话"（军棋够用了）。

## 11.1 分析 createDalvikVM()（建"假电话总机"的 4 步）


> **本节核心**：
> 1. createDalvikVM 建假 Java VM 4 步（军棋 7 个 Java 类建替身）
> 2. so 假设跑 Android 上 PC 没 Dalvik → 建假 VM 接 Java 方法
>
> **本节难点**：
> 1. 军棋 7 类：App/GLView/SiteView/AdLis/BtReceiver/TextDlg/InputDlg
> 2. DalvikVM 接通后卡点从 JNI 应答转到 native 内部（§124）

![createDalvikVM 流程](assets/ch11_dalvikvm_flow.png)



```java
VM vm = emulator.createDalvikVM(new File("junqi.apk"));
// 内部 4 步（建总机实体 → 配齐 4 个端口）：
// 1. 解析 APK 的 AndroidManifest.xml（com.cnvcs.junqi 这个包名）
//    类比：先看"这家公司叫什么"（军棋包名 com.cnvcs.junqi）
// 2. 创建 BaseVM + DalvikVM 对象（建总机实体）
// 3. 模拟 Java 类加载（军棋只有 ~7 个 Java 类，逐个建 DvmClass）
//    类比：给 7 个"客服"都建个岗位
// 4. 分配 JNIEnv / JavaVM 指针（so 手里要握的"电话本首地址"）
```

军棋 32 位 so 的 Java 侧（红线 §R6 已确认）：
- `com.cnvcs.App` / `GLView` / `SiteView` / `AdLis` / `BtReceiver` / `TextDlg` / `InputDlg`
- 几百行 smali，军棋核心逻辑在 native 层

**为什么 4 步里"建 7 个 DvmClass"是军棋关键？** 军棋 so 的 14 个 JNI 入口都注册在
`com.cnvcs.App` 上，`GLView` 是渲染类——这两个类没建好，so 调任何 `On*` 入口或渲染
方法都崩。7 个类逐个建 `DvmClass` 才"接得通"。

**"先查包名再建类"（大白话）**：第 1 步解析 `com.cnvcs.junqi`（军棋包名），
第 3 步才按包名建 7 个类（`com.cnvcs.App` 等）。包名是"哪家公司"，7 个类是
"公司里的 7 个客服"，先定公司再建客服。

**DalvikVM 3 层实现（回应"DalvikVM 怎么模拟的？加了 apk？还是做了简单的 androidSDK 的 stub 实现？还是反射 apk 相关的 java 类动态代理？"）**：

不是"动态代理"，是**静态 3 层实现**：

| 层 | 实现 | 军棋现状 | 不是动态代理 |
|---|---|---|---|
| **① 加载 apk（建类表）** | `createDalvikVM(new File("junqi.apk"))` → **静态解析** apk 里的 `classes.dex`（7 个 smali 类）→ 建"类表"（`DvmClass` + 方法表 + 字段表，按类名索引） | 解析 `com.cnvcs.junqi` + 建 7 个 `DvmClass`（App/GLView/SiteView/AdLis/BtReceiver/TextDlg/InputDlg） | **静态加载**（`createDalvikVM` 时就解析完 dex 建类表，**不动态反射**——动态代理是 Java 侧的 `java.lang.reflect.Proxy`，跟 unidbg 模拟 DalvikVM 无关） |
| **② 提供 JNI 函数表** | `vm.getJNIEnv()` 返回 **232 槽的函数指针表**（FindClass/GetMethodID/CallXxxMethod/... 全实现） | so 调 `FindClass("com/baidu/mobads/AdView")` → 查类表 → 返回 `DvmClass`（AdView 返 null 去广告） | **静态函数表**（232 槽在 unidbg 编译时定死，不是运行时动态代理） |
| **③ 按方法签名路由** | so 调 `CallStaticVoidMethod(OnCreate)` → **按 signature 路由**到 `JunqiJni.callStaticVoidMethodV` 重写 | 军棋 `AbstractJni` 重写缺失 case（广告层 + GLView 渲染回调），通用 case 交给 `AbstractJni` | **按 signature 路由**（静态方法签名匹配，不是 `Proxy.newProxyInstance` 动态代理） |

**类比**：DalvikVM = "假电话总机"——
- **类表** = 电话号码簿（按类名查 `DvmClass`，静态加载 apk dex 建的）
- **JNI 函数表** = 总机（so 拨 232 号全接通，静态函数表）
- **方法路由** = 按"拨的号码 + 拨的方式"（signature + 静态/实例/虚）路由到对应实现（**静态路由，非动态代理**）

**关键**：DalvikVM **不跑真字节码**（不真"执行 Java 逻辑"），只模拟 **JNI 交互面**（4 类电话：查类/查方法号/调方法/查对象）。军棋 Java 侧极小（7 类），核心逻辑在 native，所以"只接电话"就够（军棋 7 类 + 14 入口 + 广告类全在 4 类电话应答范围内）。

## 11.2 Dvm 相关类介绍（总机的 4 种"电话端口"）
![Dvm 类体系](assets/ch11_dvm_classes.png)
### 11.2.1 BaseVM（总机的底座）


> **本节核心**：
> 1. BaseVM = 总机底座（军棋 7 个 Java 类全建替身）
>
> **本节难点**：
> 1. BaseVM 是 DalvikVM 底层，记录所有 DvmObject/DvmClass
> 2. 军棋 7 类 resolveClass 全通

`BaseVM` 是 unidbg 的虚拟机基类，提供：
- `loadLibrary(file, init)` → 加载 so（"把 so 搬进公寓"）
- `callJNI_OnLoad(emulator, module)` → 跑 `JNI_OnLoad`（so 的"开机自检"）
- `addGlobalObject` / `getObject` → 模拟 Java 全局对象
### 11.2.2 DalvikVM（Java 类交互端口）


> **本节核心**：
> 1. DalvikVM = Java 类交互端口（军棋 FindClass AdView 返 null 去广告）
>
> **本节难点**：
> 1. FindClass(java/lang/String) 必须正常（返 null → NPE）
> 2. 军棋 AdView 返 null 是「去广告 + 补环境」一石二鸟

`DalvikVM` 继承 `BaseVM`，模拟 Dalvik 字节码环境的 JNI 面：
- `resolveClass(name)` → 模拟 `FindClass`（"查电话簿里有没有这个类"）
- `findMethodID(cls, name, sig)` → 模拟 `GetMethodID`（"查这个类的方法号"）
- `callStaticJniMethod*` → 模拟 JNI 调用（"替 so 拨这个号"）
军棋 32 位 so 的 JNI 交互（红线 §R6）：
- `FindClass("com/baidu/mobads/AdView")`（广告层拆除目标：删 baidu smali 前必须让它
  返回 null 不崩）
- 14 个 JNI 入口（`Java_com_cnvcs_App_On*`）：OnCreate / OnDestroy / OnResult /
  OnResume / OnEvent / OnKey / OnUpdate / OnGraphChange / OnBluetooth / OnPause /
  OnGraphCreate / OnTimer / OnInput / OnTouch
**军棋 14 个入口的"电话分工"（大白话）**：
- 生命周期：`OnCreate`（开局）/`OnDestroy`（收局）/`OnPause`（暂停）/`OnResume`（恢复）
- 输入：`OnKey`（键盘）/`OnInput`（输入）/`OnTouch`（触摸）
- 事件：`OnEvent`（事件）/`OnUpdate`（更新）/`OnGraphChange`（棋盘变化）/`OnResult`（结果）
- 联机/定时：`OnBluetooth`（蓝牙）/`OnTimer`（定时）/`OnGraphCreate`（棋盘创建）
每类电话 DalvikVM 都要"接得住"（方法号非 0、类非 null）。
### 11.2.3 DvmObject（Java 对象替身）


> **本节核心**：
> 1. DvmObject = Java 对象替身（军棋 GLView 对象是哈希非 C++ 指针）
>
> **本节难点**：
> 1. addGlobalObject 注册 GLView 返哈希（0x1234abcd）
> 2. 「哈希非指针」是军棋 marshal 特有：obj 不是 C++ 堆对象

`DvmObject` 模拟 Java 对象：
- `StringObject(vm, "hello")` → 模拟 `new String("hello")`（一个字符串对象替身）
- `DvmClass` → 模拟 `Class<?>`（一个类替身）
- 军棋 32 位 so 的 `OnCreate` 推进到 `std::vector` 对象图 0x43ed8（DiagJni 实证）
  ——Java 侧电话接通后，卡点转到 native 数据与指令对齐（N44）
**为什么 DvmObject 重要？** so 调 `OnCreate(env, obj)` 时，`obj` 是个 Java 对象
（`GLView` 实例）。unidbg 用 `addGlobalObject` 注册一个 `DvmObject` 替身给 `obj`，
so 拿这个替身的 hash 才能操作。军棋 `GLView` 实例就是这个替身。
### 11.2.4 DvmClass（Java 类替身）


> **本节核心**：
> 1. DvmClass = Java 类替身（军棋 7 类 resolveClass + AdView 返 null）
>
> **本节难点**：
> 1. resolveClass 查「类名 ↔ DvmClass」映射
> 2. AdView 返 null = 去广告目标（红线：删 baidu smali 前必须处理）

`DvmClass` 模拟 Java 类：
- 军棋 `com.cnvcs.App` 类（14 个 `On*` JNI 方法）
- 军棋 `com.cnvcs.GLView` 类（渲染入口）
**4 个端口为什么够？** so 的 JNI 交互就 4 类：查类（`resolveClass`/`DvmClass`）、
查方法号（`findMethodID`）、调方法（`callStaticJniMethod*`）、全局对象（`DvmObject`）。
军棋实测这 4 类全接上，`OnCreate` 才推进到 0x43ed8。
**"4 端口"为什么覆盖军棋所有 JNI（大白话）**：军棋 so 调 Java 就 4 种"电话"
（查类/查方法号/调方法/查对象），4 端口一一对应。军棋 7 类 + 14 入口 + 广告类
全在这 4 端口应答范围内，没有第 5 种电话。
## 11.3 逐环节"能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **建 DalvikVM** | 解析 APK 包名（com.cnvcs.junqi）+ 建 7 个 DvmClass（App/GLView/SiteView/AdLis/BtReceiver/TextDlg/InputDlg） | 跑真 Dalvik 字节码（只模拟 JNI 交互面，不执行 Java 逻辑） |
| **应答 FindClass** | `resolveClass` 返回 DvmClass 替身；AdView 返 null 去广告（拆除广告关键开关） | 让 AdView 返真类（要初始化广告 → 广告浮层，去广告目标相反） |
| **应答 GetMethodID** | 14 个 `On*` 方法正常返方法号（核心入口能调通） | 查内部方法号（strip 后 .dynsym 只剩 14 个导出符号） |
| **调静态 JNI 方法** | `callStaticJniMethodObject` 全通（DiagJni 推进到 0x43ed8） | 突破 0x43ed8（§124 ② 数据与指令对齐对应缺口（N44），std::vector 未初始化） |
| **构造 jobject** | `addGlobalObject` 注册 DvmObject 替身拿 hash（32 位是 int） | 传真 C++ 对象（DalvikVM 里是"哈希"不是对象指针，§124 ②） |

**军棋实测对照**：
- 建 DalvikVM ✓（解析包名 + 建 7 类）
- 应答 FindClass ✓（AdView 返 null 去广告，核心类正常 resolveClass）
- 应答 GetMethodID ✓（14 个 `On*` 方法正常返方法号）
- 调静态 JNI 方法 ✓（DiagJni 推进到 0x43ed8）
- 构造 jobject ✓（addGlobalObject 拿 hash）
- **✗ 突破 0x43ed8**：§124 ② 数据与指令对齐对应缺口（N44）（Java 侧电话全接通后卡点转 native）

## 11.4 军棋实战验证：DalvikVM 接通后卡点转移


> **本节核心**：
> 1. DalvikVM 接通后卡点转移：JNI 应答 → native 内部（§124 三项阻塞）
>
> **本节难点**：
> 1. DalvikVM 全通 = 前 2 层补环境（系统调用 + Java 方法）通
> 2. 第 3 层指令 + 第 4 层 C++ 对象图还卡 §124

军棋 unidbg 的 DalvikVM 实测（红线 §R6 DiagJni）：
| 步骤 | 军棋结果 | 大白话 |
|---|---|---|
| createDalvikVM(junqi.apk) | 解析包名 + 建 7 类 ✓ | 总机建好，7 个客服就位 |
| resolveClass("com.cnvcs.GLView") | 返回 DvmClass ✓ | 查类接得通 |
| FindClass("com/baidu/mobads/AdView") | 返回 null（去广告） | 广告客服"不在"，跳过 |
| callStaticJniMethodObject(OnCreate) | 推进到 0x43ed8 ✓ | 调 OnCreate 接通 Java 侧 |
**关键结论**：DalvikVM 接通后，军棋卡点从"Java 侧没人接电话"转移到"native 侧
C++ 对象图没补全"（§124 的 ② std::vector 0x43ed8 + ① 上下文相关 BL 解码 bug +
③ 288MB PROT_NONE mmap）。**Java 电话全接 ≠ 能玩**，瓶颈在 native 对象图。
**"接通 ≠ 能玩"（诚实边界）**：DalvikVM 接通是"电话全接"（Java 侧应答），但
军棋"能玩"还差 native 数据与指令对齐（N44）（§124 的 ②）。`OnCreate` 推进到 0x43ed8 卡住
（std::vector 未初始化，N44 外层缺口），unidbg 补不了"数据与指令对齐"——只能 redroid 真跑（有 GPU）
或 arm64 对齐版（红线 §R6）验证"能玩"。
## 11.5 本章小结（一句话版）

> `createDalvikVM` 4 步：解析 APK → 建 VM → 模拟类加载（军棋 ~7 类）→ 分配 JNIEnv/JavaVM。
> `DalvikVM` 只模拟 JNI 交互面（`resolveClass`/`findMethodID`/`callStaticJniMethod*`），
> 不跑真字节码。
> 军棋 Java 侧电话全接通后，卡点转到 native 侧 C++ 对象图（`OnCreate` 到 0x43ed8）。
> §124 纠正后剩余 3 项阻塞：① 上下文相关 BL 解码 bug（unicorn2 精度，BL 0x7965a 错位
> 到 0x6eaea）② 数据与指令对齐对应缺口（N44）（std::vector 0x43ed8）③ 288MB PROT_NONE mmap。
> `FindClass` 广告类是拆除前必处理的引用点。

---

## 视频
