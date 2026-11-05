# 第29章 Anti-unidbg 系列：JNI 层常见函数处理

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的 JNI 层反制

## 痛点：为什么 so 要靠 `FindClass` / `GetMethodID` 检测你？

so 跑在 native 层，但它想"调 Java 方法"或"查某个类存不存在"，必须通过
JNI 的 `FindClass`（找类）/ `GetMethodID`（找方法）/ `CallXxxMethod`（调方法）。
**痛点 = 逆向者要"屏蔽 so 的某个类/方法"（比如去掉广告 `AdView`），
就得在 unidbg 端应答这些 JNI 调用，决定返回真值还是假值。** 军棋实测 3 个卡点。

### 卡点 1：`FindClass` 应答错了就崩

so 查类，你应答 null（不存在）还是真类，决定了 so 走哪个分支。军棋查
`com/baidu/mobads/AdView`（广告类）——应答错就崩或出广告浮层。

- **大白话卡点**：so 问"广告客服在不在"，你答"在"就初始化广告（出浮层）、
  答"不在"就跳过（去广告），答错（异常）就崩。
- **为什么卡**：`FindClass` 是"查类"，应答 null/真类决定 so 分支。
- **不解决会怎样**：`AdView` 应答真类 → 广告浮层；应答异常 → 崩。

**"查类"为什么决定分支（大白话）**：so 拿 `FindClass` 返回的类指针去做
后续事（初始化广告 / 调广告方法）。返回 null = "类不存在，跳过"，
返回真类 = "类在，初始化"。军棋要"去广告"就应答 null（类不存在），
要"保留广告"就应答真类。

### 卡点 2：`GetMethodID` 应答 0 就跳过方法

so 查方法号，你应答 0（没有）就"不调这个方法"。军棋 `AdView.onCreate`
应答 0 = 跳过广告初始化。

- **大白话卡点**：so 问"广告初始化方法号是多少"，你答 0 = "没这个方法"，
  so 就不调它（跳过广告）。
- **为什么卡**：`GetMethodID` 返回 0，so 拿 0 调 `CallXxxMethod` 会失败
  但一般不崩（so 检查返回值）。
- **不解决会怎样**：方法号非 0 → so 调广告方法 → 出广告。

**"方法号 0"为什么"不崩只跳过"（大白话）**：`GetMethodID` 返回 0 表示
"方法不存在"，so 拿 0 调 `CallXxxMethod` 会返回"失败"（不崩，因为 so
检查返回值），只是"不调这个方法"。军棋 `AdView.onCreate` 应答 0 =
"这方法没"，so 跳过广告初始化。

**预判追问："GetMethodID 返 0 不更危险？so 崩掉怎么办？"** 军棋实测
不崩——so 的 JNI 调用点都检查 methodID（JNI 规范：拿 0 调用是 UB，
但 so 实际逻辑里先判 0 再跳）。军棋 §124e/f 的 JunqiJni 全面 handler
对 37+ 个 JNI 调用全应答（含 getStatic*/callStatic*/newObjectV 全变体），
漏一个才会 `UnsupportedOperationException`（那是 handler 覆盖不全，
不是 methodID 返 0 崩）。

### 卡点 3：14 个 `On*` 入口不能应答错

军棋 14 个 JNI 入口（`Java_com_cnvcs_App_On*`）是"核心功能"，应答错
（null/0）就调不通核心逻辑。

- **大白话卡点**：14 个 `On*` 入口是"核心业务的门"，应答错就进不去核心逻辑。
- **为什么卡**：14 入口要正常应答（方法号非 0），不能像 `AdView` 那样应答 null。
- **不解决会怎样**：14 入口应答错 → `OnCreate` 等核心逻辑调不通。

**"核心类 vs 广告类"怎么区分（大白话）**：
- 核心类（`com.cnvcs.*` 7 类 + 14 个 `On*`）：业务逻辑，应答要"正常"（真类/非 0 方法号）
- 广告类（`com/baidu/mobads/AdView`）：要去掉的，应答要"假"（null/0）
军棋 `setJni` 应答策略 = "核心正常 + 广告 null"，一石二鸟（补环境 + 去广告）。

## 为什么这样做：用 `setJni` 实现"JNI 应答器"

unidbg 的 `setJni(this)` 让你自定义"so 调 JNI 时该回什么"。
类比：so 是"打电话找客服"，`setJni` 就是"客服后台"——
你想"找 `AdView` 客服"就回"没人"（null），想找其他类就正常接。

**为什么"应答"而不是"改 so"？** 改 so 字节（让 so 不查 `AdView`）要
重签名破坏原版；`setJni` 是"在 unidbg 侧决定应答什么"，so 本体一字节
不动。军棋红线 §R6 强调"不碰 so 字节"，`setJni` 正合适。

**预判追问："不写 AdView 应答、直接删 baidu smali 不也一样？"** 不一样——
军棋 32 位 native 在 `JNI_OnLoad` 阶段就 `FindClass("com/baidu/mobads/AdView")`
了（红线 §R6：全删 baidu smali 前必须先处理这个引用点）。删 smali 后
`FindClass` 落空：如果 unidbg 侧不显式应答 null（默认 resolveClass 会报
"找不到类"异常），so 拿异常走崩。所以**先在 `setJni` 里对 AdView 应答
null，再删 smali**——这是"去广告"的红线顺序（23 章 23.1 的追问同源）。
阶段 3 删全量 baidu 后仅 arm64 native 可用（arm64 简化版不引用 AdView）。

**"一石二鸟"在哪？** 应答 `FindClass(AdView)=null` 既"补环境"（so 不崩）
又"去广告"（不初始化广告）。军棋 32 位 so 的 JNI 层反制，核心就是
"对核心类正常应答、对广告类应答 null"。

**"客服后台"应答规则（大白话）**：

| 来电 | 应答 | 大白话 |
|---|---|---|
| 找 `AdView`（广告客服） | "没人"（null） | 跳过广告 |
| 找 `com.cnvcs.GLView`（核心客服） | "在"（真类） | 接通核心 |
| 问广告方法号 | "没"（0） | 跳过广告方法 |
| 问 14 个 `On*` 方法号 | "有"（正常方法号） | 接通核心入口 |

规则就一条："核心正常接、广告 null/0 挡"。

## 29.1 FindClass 反制（军棋去广告关键）


> **本节核心**：
> 1. FindClass 反制（军棋去广告关键）
>
> **本节难点**：
> 1. AdView 返 null（去广告）；java/lang/String 必须正常（返 null → NPE 崩）
> 2. 「FindClass 反制」是去广告核心
> 3. 军棋红线：全删 baidu smali 前必须处理 native FindClass 引用

![军棋 FindClass 反制](assets/ch29_findclass_reversal.png)



```java
vm.setJni(new Jni() {
    @Override public DvmClass<?> findClass(BaseVM vm, String name) {
        // 军棋广告层拆除: com/baidu/mobads/AdView 返回 null
        // so 拿 null → 跳过 AdView 初始化 → 无广告浮层
        if (name.equals("com/baidu/mobads/AdView")) return null;
        // 其他类正常应答(com.cnvcs.App/GLView/... + 通用类)
        return vm.resolveClass(name);
    }
});
```

**军棋 3 类 `FindClass`（红线 §R6）**：

| 类名 | 大白话 | 应答 |
|---|---|---|
| `com/baidu/mobads/AdView` | 广告类（拆除目标） | **返回 null**（去广告） |
| `android/os/Environment` | 环境检测 | 正常返回 |
| `java/lang/System` | 通用 | 正常返回 |

**为什么 `AdView` 返回 null 是"去广告关键"？** 军棋红线：全删
`smali/com/baidu/*` 前必须处理 native 的 `FindClass("com/baidu/mobads/AdView")`
引用。返回 null = "广告类不存在，别初始化"，so 跳过广告逻辑。这是军棋
"补环境 + 去广告"一石二鸟的核心应答点。

## 29.2 methodID 反制（跳过某方法）


> **本节核心**：
> 1. methodID 反制（跳过某方法）
>
> **本节难点**：
> 1. 军棋广告方法 GetMethodID 返 0 → so 走方法不存在分支跳过广告
> 2. 「methodID 反制」是跳过广告方法
> 3. GetMethodID 返 0 是合法应答（JNI 规范允许方法不存在返 0）

![军棋 methodID 反制](assets/ch29_methodid_reversal.png)



```java
vm.setJni(new Jni() {
    @Override public long getMethodID(BaseVM vm, DvmClass<?> cls, String name, String sig) {
        // 军棋 AdView 的 onCreate 方法 → 返回 0(so 拿 0 不调)
        if (name.equals("onCreate") && cls.name.equals("com/baidu/mobads/AdView")) {
            return 0L;
        }
        // 其他方法正常返回(军棋 14 个 On* JNI 入口要正常)
        return super.getMethodID(vm, cls, name, sig);
    }
});
```

**军棋 14 个入口名（readelf 实测，方法号必须正常应答，别用 OnResize/OnQuit/OnSave/OnRestore）**：
OnCreate / OnDestroy / OnResult / OnResume / OnEvent / OnKey / OnUpdate /
OnGraphChange / OnBluetooth / OnPause / OnGraphCreate / OnTimer / OnInput / OnTouch。

**"方法号 0"和"类 null"配合（大白话）**：
- 类 `AdView` 应答 null（"广告类没人"）+ 方法 `onCreate` 应答 0（"这方法没"）
- 两层都"挡"，广告初始化彻底跳过
- 核心类 + 14 入口两层都"接"，业务逻辑能跑

## 29.3 逐环节"我们能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **FindClass** | `AdView` 返 null（去广告）；核心 7 类 + 通用类正常 resolveClass | 让"广告类正常初始化"（与去广告目标相反；阶段 3 删 smali 后更必须 null） |
| **GetMethodID** | 广告方法返 0 跳过；14 个 `On*` 方法号正常应答 | 14 入口应答 0/null（核心逻辑调不通） |
| **CallXxxMethod** | JunqiJni 全面 handler 应答 37+ JNI 调用（§124f 实证：GLView 创建 + EGL + setContentView + registerReceiver×3） | 靠 JNI 应答突破 §124 阻塞（14 入口全通后仍卡 bx bug / 全局未初始化 / 288MB mmap——JNI 层是"电话接得通"，native 对象图是"聊业务聊不拢"） |

**"14 入口全通" ≠ "OnCreate 跑完"（诚实边界）**：军棋 §124d-g 实测
14 入口的 JNI 应答全通（DiagJni + JunqiJni handler），但 `OnCreate`
最后 10% 被 no-init_array 全局未初始化挡住（§124g：0x23b7a READ_PROT，
std::string 拷贝循环读垃圾源，LR=垃圾 0x39a63305 → 垃圾函数指针调用链）。
JNI 层能"接电话"（全通），native 层"聊业务"（对象图）仍卡——
反 JNI 检测/应答的全通，不替代 §124 三项阻塞。

## 29.4 军棋实战验证：JNI 应答"核心正常 + 广告 null"


> **本节核心**：
> 1. JNI 应答核心正常 + 广告 null
>
> **本节难点**：
> 1. 14 入口方法号必须正常应答（readelf 实测名）；AdView 返 null
> 2. 「核心正常 + 广告 null」是军棋 JNI 应答策略
> 3. 14 入口名 readelf 实测，别用 OnResize/OnQuit/OnSave/OnRestore

军棋 unidbg JNI 层反制实测（红线 §R6）：
| JNI 调用 | 军棋应答 | 结果 |
|---|---|---|
| `FindClass(AdView)` | null | 跳过广告 ✓（去广告） |
| `FindClass(com.cnvcs.GLView)` | 正常 | 核心类接通 ✓（§124f GLView 创建成功） |
| `GetMethodID(AdView.onCreate)` | 0 | 跳过广告方法 ✓ |
| `GetMethodID(14 个 On*)` | 正常方法号 | 14 入口能调 ✓ |
| `getStaticIntField(KeyEvent.ACTION_DOWN)` 等 | JunqiJni handler | 37+ JNI 调用全通 ✓ |
**"核心正常 + 广告 null"是军棋 JNI 反制的核心原则**：
- 核心类（`com.cnvcs.*` 7 类 + 14 个 `On*`）正常应答 → 业务能跑
- 广告类（`AdView`）应答 null/0 → 去广告
- 一石二鸟：补环境（核心接通）+ 去广告（广告跳过）
## 29.5 本章小结（一句话版）

> JNI 反制 = 自定义"so 调 Java 时回什么"（`setJni` 当"客服后台"）。
> 军棋：`FindClass(AdView)` 返回 null（去广告），`GetMethodID(AdView.onCreate)`
> 返回 0（跳过），其他 14 个 `On*` JNI 入口正常应答。
> 原则："核心类正常应答 + 广告类应答 null/0"——补环境 + 去广告一石二鸟。
> 14 个入口名（OnCreate/OnDestroy/OnResult/OnResume/OnEvent/OnKey/OnUpdate/
> OnGraphChange/OnBluetooth/OnPause/OnGraphCreate/OnTimer/OnInput/OnTouch）
> 方法号必须正常应答。JNI 应答全通 ≠ OnCreate 跑完（§124g：最后 10% 卡在
> native 全局对象图，JNI 层救不了）。

## 视频
