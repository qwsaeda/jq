# 第23章 unidbg 补环境实战：标识记录

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的标识记录

## 痛点：为什么 so 要"标识记录"？

军棋 so 启动时干了两件事：① 记录"我是谁"（`JNI_OnLoad` 注册 14 个 `On*` 方法 +
广告 `AdView` 类）；② 记录"我上次玩到哪"（读 `one.sjg` 存档，还原上局对局）。
军棋实测 3 个卡点。

### 卡点 1：so 不"记录身份"就不知道调谁

14 个 JNI 入口要注册好，so 才知道"对局事件该调哪个函数"。

- **大白话卡点**：so 要有"工牌"（14 个 `On*` 方法注册好），才知道
  "对局事件该找谁"。
- **为什么卡**：`JNI_OnLoad` 不注册，so 不知道调哪个入口。
- **不解决会怎样**：`OnCreate` / `OnUpdate` / `OnBluetooth` 等 14 个入口调不通，
  整局游戏的走子/吃子/联机事件全断。

**"工牌"是什么（大白话）**：so 的 14 个 `On*` 入口像"14 个岗位"，
`JNI_OnLoad` 是"岗前培训"（把 14 个岗位的方法号登记到 JVM）。不登记，
so 不知道"走子事件该调 `OnUpdate` 还是 `OnEvent`"——14 个入口没注册
= 14 个岗位没人应。

### 卡点 2：so 不"读存档"就忘干净

军棋玩家上次下到一半的棋局，重启后要靠 `one.sjg` 续上。

- **大白话卡点**：so 要有"打卡记录"（`one.sjg` 存档），重启才能续上局。
- **为什么卡**：`OnCreate` 读 `one.sjg`，没存档就"忘干净"。
- **不解决会怎样**：玩家下了一半重启 → 全丢，体验直接崩。

**"打卡记录"为什么关键（大白话）**：军棋 `OnCreate` 一启动就读 `one.sjg`
（续局），读到"上局下到一半"就把 board 60 格 + 走子记录还原。没这
"打卡记录"，每次重启都从头（`one.sjg` 空）——玩家下到一半重启就全丢。

### 卡点 3：广告层要"假装存在"

so 启动会 `FindClass("com/baidu/mobads/AdView")` 查广告类，找不到就崩
——但你又不想要广告。

- **大白话卡点**：so 查"广告客服在不在"，你不想有广告但又不想崩——
  只能"假装存在但返回 null"。
- **为什么卡**：军棋红线——全删 `smali/com/baidu/*` 前必须处理 native 里
  `FindClass("com/baidu/mobads/AdView")` 引用，不处理直接崩。
- **不解决会怎样**：`FindClass(AdView)` 崩（类真删了）或初始化广告（类留着）。

**"假装存在"怎么做到（大白话）**：so 问"AdView 在不在"，你应答"在"
（返回真类）→ so 初始化广告 → 出广告浮层（不想要）；应答"不在"（返回 null）
→ so 跳过广告 → 不崩也不出广告（去广告目标）。军棋全删 baidu smali 前，
必须让 `FindClass(AdView)` 返回 null，否则删了 smali 后 so 崩。

## 为什么这样做：分两层补"标识记录"

- **JNI_OnLoad 环境**：把 14 个 `On*` 方法 + `AdView` 类注册好
  （`AdView` 返回 null = 跳过广告）
- **文件标识**：把 `one.sjg` 存档 + 7 个 config XML 准备到虚拟目录，so 读"上局记录"

类比：so 是"新来的员工"，要给他"工牌"（JNI 注册）+ "他上次干到哪的
打卡记录"（`one.sjg`）。

**预判追问："14 个入口都是 native 函数，'注册'不是反了吗？so 导出它们，
Java 侧按名字调不就行？"** 两条路都存在，军棋实测两条都在跑：
1. **按名字**：Java 侧 `native` 方法声明 + `System.loadLibrary` 后 ART 按
   `Java_com_cnvcs_App_On*` 符号表找（军棋 `.dynsym` 保留 14 个导出符号，
   这条路静态就能通）。
2. **按 `RegisterNatives`**：so 的 `JNI_OnLoad` 里主动调
   `(*env)->RegisterNatives` 把方法号**主动登记**到 JVM（"工牌"路径），
   之后 so 内部事件分发不走符号表，走登记的方法号。
unidbg 里 `setJni` 的 `registerNatives` 应答的就是第 2 条路——so 在
`JNI_OnLoad` 里登记 14 个方法号，JVM 侧要接住。两条路都要应答对，
14 入口才"接得住"。

**预判追问："AdView 直接删掉 smali 类不就干净了？为什么还要应答 null？"**
顺序问题：军棋 32 位 native 在 `JNI_OnLoad` 阶段（早于 Java 类加载完成）
就 `FindClass("com/baidu/mobads/AdView")` 了（红线 §R6：全删 baidu smali
前必须先处理这个引用点）。删 smali 后 `FindClass` 应答"类不存在"，
如果 unidbg 侧不显式应答 null（默认 resolveClass 会报找不到类异常），
so 拿异常走崩。所以**先让 `setJni` 对 AdView 应答 null**，再删 smali——
这是"去广告"的红线顺序。

**两层"工牌 vs 打卡"（大白话）**：
- 工牌（JNI 身份）：14 个 `On*` 方法 + 7 个 Java 类注册好（so 知道"我是谁"）
- 打卡（文件状态）：`one.sjg` 存档 + 7 个 config XML（so 知道"我干到哪"）
工牌 + 打卡都补全，so 才"接得住"（`OnCreate` 推进 + 读存档续局）。

## 23.1 样本一：JNI_OnLoad 环境补全（军棋 DiagJni 实证）


> **本节核心**：
> 1. JNI_OnLoad 环境补全（军棋 DiagJni 实证）
>
> **本节难点**：
> 1. 14 入口 + 7 类 + AdView 全 resolveClass；AdView 返 null 去广告
> 2. 「标识记录第一层」= JNI_OnLoad 环境补全
> 3. DiagJni 实证：JNI_OnLoad 跑通后 14 入口录进

![军棋标识记录](assets/ch23_identifier.png)



```java
// 1. 注册 14 个 On* 静态方法（军棋 14 个 JNI 入口）
//    findClass：广告类 AdView 返回 null（去广告）；其他类正常 resolveClass
vm.setJni(new Jni() {
    @Override public DvmClass<?> findClass(BaseVM vm, String name) {
        if (name.equals("com/baidu/mobads/AdView")) return null; // 跳过广告初始化
        return vm.resolveClass(name);
    }
    @Override public long getMethodID(BaseVM vm, DvmClass<?> cls, String name, String sig) {
        return super.getMethodID(vm, cls, name, sig); // 14 个 On* 正常返回方法号
    }
    @Override public long registerNatives(BaseVM vm, long module, long methods, int count) {
        return 0L; // 登记成功
    }
});

// 2. 跑 JNI_OnLoad（军棋 14 个 JNI 入口在这注册, 开机自检）
vm.callJNI_OnLoad(emulator, dm.getModule());
```

**军棋 14 个入口名（readelf 实测，必须用这些，别用 OnResize/OnQuit/OnSave/OnRestore）**：
OnCreate / OnDestroy / OnResult / OnResume / OnEvent / OnKey / OnUpdate /
OnGraphChange / OnBluetooth / OnPause / OnGraphCreate / OnTimer / OnInput / OnTouch。

**为什么 AdView 返回 null？** 军棋红线：全删 `smali/com/baidu/*` 前必须处理
native 的 `FindClass("com/baidu/mobads/AdView")` 引用。返回 null =
"广告类不存在，别初始化"，so 跳过广告逻辑（去广告目标）。一石二鸟：
补环境（不崩）+ 去广告（不初始化）。

**14 个入口的"岗前培训"分工（大白话）**：
- 生命周期类（`OnCreate`/`OnDestroy`/`OnPause`/`OnResume`）：开收局 + 暂停恢复
- 输入类（`OnKey`/`OnInput`/`OnTouch`）：玩家操作
- 事件类（`OnEvent`/`OnUpdate`/`OnGraphChange`/`OnResult`）：走子/更新/棋盘/结果
- 联机类（`OnBluetooth`）：蓝牙同步（UUID `ce1404fe-0863-4278-86c2-c9c560f0e215`）
- 定时类（`OnTimer`）：超时/计时（70 步/1000 步判和）
`registerNatives` 把这 14 个方法号登记到 JVM，so 才知道"对局事件该调哪个"。

## 23.2 样本二：文件标识（存档 + 配置）


> **本节核心**：
> 1. 文件标识（存档 + 配置）：军棋 7 config XML + one.sjg 全落宿主
>
> **本节难点**：
> 1. 读不到走新游戏分支
> 2. 「标识记录第二层」= 文件标识
> 3. one.sjg 续局（OnCreate 读 + OnPause 写）

军棋 so 的文件标识（红线 §R6）：
| 文件 | 大白话 | 用途 |
|---|---|---|
| `one.sjg` | 对局存档（board 60 格 + turn/stage/winner + flags/mines/pieces + moves） | 续上局 |
| `data/config/{ui,font,image,sound,string,path,uitag}.xml` | 7 个配置 | 界面/字体/图/音/字/路径/标签 |
| `data/{320,480,480x,720,720s,800}/` | 6 个分辨率目录 | 按设备屏选资源 |
```java
// 准备虚拟目录（so 读 one.sjg → 实际读 ./junqi_data/one.sjg）
emulator.getFileSystem().link(
    new File("/data/data/com.cnvcs.junqi"),
    new File("./junqi_data"));
```
**`one.sjg` 格式（红线 §R6 已逆向）**：
- `board`（60 格全盘同步）+ `turn/stage/winner`（轮到谁/阶段/胜负）
- `flags/mines/pieces`（军旗/地雷/棋子位置）+ `moves`（走子记录）
- `OnCreate` 续局读 `one.sjg`，`OnPause` 保存，对局结束清理
**"打卡记录"的 5 个字段（大白话）**：
| 字段 | 大白话 |
|---|---|
| `board`（60 格） | 全盘 60 格（棋盘每格一个棋子位） |
| `turn/stage/winner` | 轮到谁 / 对局阶段 / 胜负 |
| `flags/mines/pieces` | 军旗/地雷/棋子位置 |
| `moves` | 走子记录（悔棋靠它） |
| 清理 | 对局结束删 `one.sjg`（`OnCreate` 读到空 = 新局） |
## 23.3 逐环节"我们能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **JNI_OnLoad 注册** | 14 个 `On*` 入口 `RegisterNatives` 全登记（DiagJni 实证 `createDalvikVM + resolveClass + callStaticJniMethodObject` 全通） | 让广告类"正常初始化"——军棋目标是 AdView 返 null 跳过，"正常初始化"与去广告目标相反 |
| **AdView 应答** | `FindClass` 返 null，so 跳广告分支不崩（补环境 + 去广告一石二鸟） | 删 baidu smali 后再改应答策略（native 引用点固定在 `JNI_OnLoad`，删 smali 前必须先固定"返 null"） |
| **文件标识** | `one.sjg` 读（续局）+ 写（`OnPause` 保存）经 I/O 重定向落宿主 | 让 `one.sjg` 承载"完整对局语义"（格式已逆向，但"存档读到后棋盘还原正确性"卡 C++ 对象图 §124 的 ②，读到了 ≠ 还原对） |

## 23.4 军棋实战验证：两层补全 so"接得住"


> **本节核心**：
> 1. 两层补全 so「接得住」（JNI_OnLoad + 文件标识）
>
> **本节难点**：
> 1. 14 入口名 readelf 实测（无 OnResize/OnQuit/OnSave/OnRestore）
> 2. 「接得住」是补环境验证
> 3. 军棋 14 入口真实名必须用 readelf 实测，别用不存在的名字

军棋 unidbg 标识记录实测（红线 §R6）：
| 标识 | 军棋结果 | 大白话 |
|---|---|---|
| JNI 身份（14 入口注册） | DiagJni 全通 ✓ | "工牌"办好了 |
| AdView 类（返回 null） | 跳过广告 ✓ | "假装不存在" |
| `one.sjg` 存档 | 重定向可读 ✓ | "打卡记录"拿到了 |
| 7 config XML | 重定向可读 ✓ | 配置读到了 |
**两层补全后 so"接得住"**：`OnCreate` 接通 Java 侧（14 入口注册）+ 读到
`one.sjg` 续局 + 读到 7 个 config。但推进到 `std::vector` 对象图 0x43ed8
仍卡（§124 的 ② C++ 对象图语义缺失）——标识记录补的是"我是谁/我干到哪"，
不补"native 对象图语义"。
**"接得住 vs 跑得通"（诚实边界）**：标识记录补全是"接得住"（`OnCreate`
接通 Java、读存档、读配置都通），但"跑得通"（游戏能玩）还差 native 对象图
语义（§124 的 ② std::vector 0x43ed8）+ redroid dump 的 18 常量池。
标识记录是"接电话"，对象图是"聊业务"，接得住 ≠ 聊得通。
## 23.5 本章小结（一句话版）

> 标识记录 = so 的"工牌"（JNI 注册 14 个 `On*` + AdView）+ "打卡记录"
> （`one.sjg` 存档）。
> 军棋两层补全：JNI_OnLoad 已实证（DiagJni），AdView 返回 null 去广告，
> `one.sjg` 续局 + 7 config XML 重定向可读。
> 标识记录补的是"身份 + 状态"，不补"native 对象图语义"——`OnCreate` 仍卡
> 0x43ed8（§124 的 ②）。

## 视频
