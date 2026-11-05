# 第25章 unidbg 补环境实战：补环境加强

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的补环境加强（§124d-g 里程碑）

## 痛点：为什么"补环境补到 90% 卡住了"？

军棋 so 的补环境前面几步（12 init_array + 重定位 + 挂桩 GL）都通了，
`OnCreate`（14 个 JNI 入口之一，readelf 实测：OnCreate / OnDestroy /
OnResult / OnResume / OnEvent / OnKey / OnUpdate / OnGraphChange /
OnBluetooth / OnPause / OnGraphCreate / OnTimer / OnInput / OnTouch，
Thumb 奇地址 0x233e1）也推进到 ~90%（§124d-g 里程碑：GLView 创建 + EGL 配置 +
setContentView + registerReceiver×3），但**最后 10% 被两条路夹死**。
军棋实测 3 个卡点。

### 卡点 1：C++ 对象图不是"没补"，是"补了也不对"

`std::vector` 的 `begin`/`end`/`capacity` 三个指针来自未初始化对象。
unidbg 模拟时这仨是随机垃圾值 → 读 `begin` 指向的地址 = 写未映射内存 → SIGSEGV。
§124g 实证更深：std::string 拷贝循环（0x23b7a `ldrb r2,[r4,r3]`）的源指针
也是垃圾（LR=垃圾 0x39a63305，调用链本身损坏）。

- **大白话卡点**：vector/string 的"标签"（begin/end/capacity）是垃圾值，
  补一个值只能补一次，后续对象演化全乱。
- **为什么卡**：这些值来自真实 C++ 运行时布局（谁 new 的、堆在哪），
  模拟环境没人知道。
- **不解决会怎样**：`OnCreate` 最后 10% 读垃圾源 → READ_PROT（§124g 实证）。

### 卡点 2：init_array 路径的 bx bug（模拟器全局分支损坏）

§124c 决定性实验（DiagCave12/14/16/17/19/20）证明：
**unidbg 运行 init_array 后，间接/直接分支（bx r3、b.w、ldr pc）在任意地址
都跳到错误目标（0xfff1a33c 或 WRITE_UNMAPPED）**。这是全局 CPU 分支损坏，
非位置/TC/寄存器/版本问题（0.9.8 和 0.9.9 都复现）。

- **大白话卡点**：跑了 12 个 init_array 之后，so 里所有分支指令全错。
- **为什么卡**：unicorn2 后端在 init_array 副作用后分支解码损坏（模拟器精度 bug）。
- **不解决会怎样**：走 init_array 路径（真机 dlopen 的正常路径）直接不可用，
  只能走"跳过 init_array"的非常规路径。

### 卡点 3：no-init_array 路径的"全局未初始化"（两条路死锁）

跳过 init_array 能到 0x43ed8、能 vecfix 37 次、能推进到 90%，
但**全局 C++ 对象（std::string 函数指针表）没初始化**（init_array 就是干这
的）→ OnCreate 后半段触及未初始化全局函数指针 → 垃圾调用链 → READ_PROT。

- **大白话卡点**：跑 init_array = 分支全坏（bx bug）；不跑 = 全局没初始化。
  两条路互为死锁。
- **为什么卡**：bx bug 是模拟器深层 bug（修不了）；no-init_array 要手动
  逆向 std::string 全局地址表（工作量大）。
- **不解决会怎样**：`OnCreate` 停在 90%，最后 10% 补不了，unidbg 路线
  "能调" ≠ "能玩"。

## 为什么这样做：给 C++ 对象"补标签" + 换路径绕过模拟器 bug

补环境加强的两个动作：
1. **vecfix**：给 `std::vector` 0x43ed8 的 begin/end/capacity 手工填自洽值
   （§124e 实证：无条件 patch r0=r1=r2=scratch + [scratch+0x10]=[scratch+0x14]=scratch，
   vector 函数走空路径，两次调用都正确完成）。
2. **no-init_array + code cave**：跳过 init_array（避开 bx bug），
   用 `.text` 内 0xc3a68 的 14 字节 nop padding（零引用，安全可覆盖）
   当 trampoline，把错位的 BL 0x7965a patch 成 `bl 0xc3a68`
   （`4af005fa`），cave 里 `ldr r3,[pc,#4]` + `bx r3` 跳到真目标 0x43ed8。

`std::vector` 在内存里长这样（3 指针 + 1 大小）：

```
[ begin | end | capacity | size ]   ← 4 个指针/整型
```

正常 C++ 构造时 `begin` 指向堆首元素、`end = begin + size`、`capacity` 是预留
容量。模拟环境里这 4 个值没人初始化 → 补环境加强 = 手动给它们填自洽值，
让 `OnCreate` 能正常推进。

类比：`std::vector` 是"有 4 个空抽屉的柜子"，正常 C++ 会把抽屉标签
（begin/end/capacity/size）写好，模拟环境里标签没写（垃圾值），你手动补上
标签（全指向 scratch 空区 = 空 vector）→ 柜子走"空路径"，函数干净返回。

**预判追问："给 vector 补 begin/end 不就转译/还原了？为什么不算'补全对象图'？"**
vecfix 补的是"一次性快照"：0x43ed8 这次调用时把 r0/r1/r2 全指向 scratch，
函数走空路径返回。但**对象图后续演化**（别的 vector 构造、拷贝、扩容）
每个都要单独补，且"该补哪个 vector、size 是多少"来自真实运行时布局——
unidbg 给不了"真实布局"，只能给"这次调用别崩"。所以 vecfix 是"安全阀"
（37 次全补、崩不掉），不是"对象图补全"（语义正确）。§124g 的 std::string
全局函数指针表连 vecfix 这种"逐调用补"都补不了（调用链本身 LR 就是垃圾），
只能"逆向全局地址表手动初始化"（工作量大，未做）。

**预判追问："换 dynarmic 后端不就避开 bx bug 了？"** 换不了。dynarmic 有
PC off-by-2 已知 bug（`add rX,pc` 用 `(addr+4)&~3` mask，军棋 11 个函数
回归 MISMATCH 的根因之一，unicorn2 修的正是这个）。bx bug（§124c）和
PC off-by-2 是两个独立 bug：dynarmic 避开前者但引入后者，unicorn2 修了
后者但带前者（init_array 后）。军棋实测 0.9.8/0.9.9 都复现 bx bug，
**换后端是"拆东墙补西墙"**，workaround 只剩：a) code cave patch（§124b 已实证）
b) 跳过 init_array（§124d 已实证，代价是全局未初始化）c) 等模拟器上游修 bx bug。

## 25.1 军棋补环境现状（§124d-g 里程碑）


> **本节核心**：
> 1. 军棋补环境现状（§124d-g 里程碑）
>
> **本节难点**：
> 1. OnCreate 推进 ~90%（vecfix 37 次 + JNI 37+ + GLView 创建 + EGL 配置 + setContentView）
> 2. 「90% 里程碑」是军棋 unidbg 推进到最深点
> 3. 最后 10% 被 READ_PROT 垃圾调用链挡住（§124g/h）

已通的（比"12 init_array 全通"更进一步）：
- 加载 so：5312 RELATIVE + 214 PLT + 6 GLOB_DAT 全通 ✓
- 12 个 `.init_array` 全通 ✓（但注意 §124c：跑了 init_array 后分支损坏）
- **no-init_array 路径**（§124d 突破）：
  - code cave：0x7965a → 0xc3a68 → 0xc3a6a（bx r3）→ 0x43ed8 ✓
  - vecfix 37 次（vector 函数全走空路径）✓
  - JunqiJni 全面 handler 处理 37+ 个 JNI 调用 ✓
  - GLView 创建 + EGL 配置 + setContentView + 显示器尺寸 + registerReceiver×3 ✓
  - **`OnCreate` 达 ~90%**
**剩余阻塞（§124 纠正后，§124g 根因确认）**：
- **① init_array 路径的 bx bug**（模拟器全局分支损坏，§124c）：
  跑 init_array 后任意分支指令跳错目标（0xfff1a33c/WRITE_UNMAPPED），
  非位置/TC/寄存器/版本问题。workaround：code cave patch + 跳过 init_array。
- **② no-init_array 全局未初始化**（§124g 实证）：
  崩溃 0x23b7a `ldrb r2,[r4,r3]`（std::string 拷贝循环），
  入口 0x23ae8 LR=垃圾 0x39a63305 → 垃圾函数指针调用链 → READ_PROT。
  **任何单点 workaround 都无法推进**（patch r6=0 变无限循环；
  重定向 this+src 到 scratch 仍反复垃圾调用）——因为调用链本身损坏。
- **③ 288MB PROT_NONE mmap**（DiagJni5 实证）：
  OnCreate 深处 `mem_map 0x3743f000, 302243840(≈288MB), perms=0`，
  unidbg `mmap2` 处理失败。需 syscall hook 或配置允许大 PROT_NONE 映射。
**两条路径的死锁（§124g 诚实结论）**：
```
init_array 路径   → bx bug（模拟器 bug，不可修复）
no-init_array 路径 → 全局未初始化（需逆向 std::string 全局地址表，工作量大）
```
unidbg 无法完整模拟 `OnCreate` 的真实执行，除非修复 bx bug（模拟器深层
bug）或手动初始化全部相关全局。**这是 unidbg 路线的天花板**。
## 25.2 军棋补环境框架（7 步 + 2 加强项）


> **本节核心**：
> 1. 军棋补环境框架（7 步 + 2 加强项）
>
> **本节难点**：
> 1. 选后端→配库→挂桩→建 VM→调入口→填常量→破 §124
> 2. 加强项 = 上下文相关 BL + 全局初始化
> 3. 「7 步 + 2 加强项」是军棋补环境方法论

![军棋补环境 7 步框架](assets/ch25_env_enhance.png)



```
1. AndroidEmulator(unicorn2 后端 + AndroidResolver(23))
2. DalvikVM(com.cnvcs.junqi)
3. loadLibrary(libjunqi.so, init 可选)
   → no-init_array 路径(§124d 突破) 或 init_array 路径(§124c bx bug)
4. 挂桩 libGLESv1_CM / libGLESv2(无真 GPU)
5. 注入 18 个 .text 常量池 + 2 个 .data 值(redroid dump 72B)
6. Hook __cxa_atexit(返回 0, 避免读零 .got 崩)
7. 调 14 个 JNI 入口(OnCreate 等, Thumb 奇地址 &~1)
+ 加强 1: code cave(0xc3a68 trampoline, patch BL 0x7965a = 4af005fa)
+ 加强 2: vecfix(0x43ed8 r0/r1/r2 全指向 scratch, 37 次)
+ 加强 3: JunqiJni 全面 handler(getStatic*/callStatic*/call*/newObjectV 全变体)
```

## 25.3 补环境实操

### 25.3.1 getApplication 环境补充


> **本节核心**：
> 1. getApplication 环境补充（军棋 com.cnvcs.App 继承 Application）
>
> **本节难点**：
> 1. DalvikVM mock getApplication 返 App 替身
> 2. 「getApplication」是 ActivityThread 补环境一部分
> 3. 军棋 App 继承 Application，DalvikVM 要 mock

```java
// 军棋 so 调 getApplication() 拿 Application 对象
// unidbg 模拟: 补一个 Application 对象(占位)
DvmObject<?> app = vm.addGlobalObject(new DvmObject<>());
```
**为什么占位？** 军棋 so 读 `getApplication` 主要做"日志/统计"，不拿它做
对局逻辑，占位不崩即可（§124f 里 JunqiJni handler 对这类调用全应答）。
### 25.3.2 无障碍服务的补环境（原书 25.3.2，军棋不适用但通用框架需说明）


> **本节核心**：
> 1. 无障碍服务（原书 25.3.2，军棋不适用但通用框架需说明）
>
> **本节难点**：
> 1. 军棋走 GLView 渲染 + OnTouch 输入不依赖无障碍
> 2. 通用 mock FindClass(AccessibilityService) 返 null
> 3. 「无障碍服务」军棋无此场景，但 unidbg 通用框架需 mock
> 4. 军棋 7 个 Java 类里无 AccessibilityService 引用

原书 25.3.2 有完整的**无障碍服务补环境**：so 通过 `FindClass("android/accessibilityservice/AccessibilityService")`
+ `callXxxMethod` 调 `onAccessibilityEvent` / `onServiceConnected`，绕过 app 自身 UI 直接驱动系统。
unidbg 通用框架需 mock `AccessibilityService` 替身（`FindClass` 返 null / `GetMethodID` 返 0），
让调无障碍服务的 app 在模拟环境不崩。
**军棋无此场景**：军棋 32 位 so 未调 `AccessibilityService`（红线 §R6 实测 7 个 Java 类里无
无障碍服务引用，军棋走 `GLView` 渲染 + `OnTouch` 输入，不依赖无障碍驱动）。所以军棋
**不补无障碍服务**——但 unidbg 作为通用补环境框架，遇到调无障碍的 app 时需 mock 这个类。
**通用 mock（其他 app 适用）**：
```java
vm.setJni(new Jni() {
    @Override public DvmClass<?> findClass(String name) {
        if (name.equals("android/accessibilityservice/AccessibilityService"))
            return null;  // 返 null 让 so 走"无障碍不可用"分支, 不崩
        return super.findClass(name);
    }
});
```
## 25.4 逐环节"我们能干什么 / 不能干什么"


> **本节核心**：
> 1. C++ 对象图「补了也不对」（std::vector 元素数填不出）
>
> **本节难点**：
> 1. bx bug 可绕过 9 个 init（最大安全集 init[0..8]，§124h）
> 2. 「两条路死锁」：init[9+]→bx bug / init[0..8]→全局未初始化
> 3. 突破需修复 bx bug（模拟器深层 bug）或手动逆向 std::string 全局（工作量大）

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **code cave（§124b）** | 0xc3a68 nop padding 写 trampoline，patch BL 0x7965a → cave → 0x43ed8（隔离环境 + no-init_array 全通） | 在"完整环境（init_array 后）"写 cave（.text 变只读，写入静默失败，§124c DiagCave7/8 实证） |
| **vecfix（§124e）** | 0x43ed8 无条件 patch r0=r1=r2=scratch，37 次全补，vector 函数走空路径 | 补"对象图语义"（vecfix 是"逐调用安全阀"，不是"真实布局补全"；§124g 的 std::string 全局表连 vecfix 都够不着） |
| **JunqiJni handler（§124f）** | 37+ 个 JNI 调用全应答（getStatic*/callStatic*/newObjectV 全变体），GLView/EGL/setContentView/registerReceiver 全通 | 绕过 §124g 全局未初始化（JNI 应答再全，也救不了"native 全局函数指针表是垃圾"） |
| **no-init_array 路径** | `OnCreate` 推进到 ~90%（比 init_array 路径的"bx bug 直接坏"深得多） | 补完最后 10%（全局 std::string 地址表未初始化 → 垃圾调用链 READ_PROT，需逆向全局表手动初始化，工作量大未做） |
| **288MB mmap** | 定位到 `mem_map 0x3743f000, 302243840, perms=0`（DiagJni5 实证） | unidbg `mmap2` 直接处理（288MB PROT_NONE 映射失败，需 syscall hook 或配置） |
**"能干什么"的量化口径**：军棋"能推进 OnCreate 到 90%"= 37 次 vecfix +
37+ JNI 调用 + GLView/EGL/蓝牙注册全通。**"不能干什么"的量化口径**：
最后 10% 被两条路夹死（bx bug + 全局未初始化），unidbg 路线到此为止——
"指令语义 100% 对齐"（红线 §122）≠ "OnCreate 完整返回"≠ "能玩"。
## 25.5 本章小结（一句话版）

> 补环境加强 = 给 C++ 对象"补标签"（vecfix）+ 换路径绕过模拟器 bug
> （no-init_array + code cave）+ 全面 JNI handler（JunqiJni）。
> 军棋 `OnCreate` 推进到 ~90%（§124d-g：GLView 创建 + EGL 配置 +
> setContentView + registerReceiver×3），最后 10% 被两条路夹死：
> init_array 路径 bx bug（模拟器 bug）+ no-init_array 路径全局未初始化
> （§124g 垃圾调用链 READ_PROT）——**指令能跑 ≠ 能玩**，
> "能玩"需真机 GPU 验收（redroid 无 GPU，只能 dump 数据）。

## 视频
