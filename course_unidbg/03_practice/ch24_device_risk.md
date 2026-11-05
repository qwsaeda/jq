# 第24章 unidbg 补环境实战：设备风控

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的设备风控

## 痛点：为什么 so 要查"设备信息"？

军棋 so 启动时会读几样"设备特征"，用来判断"跑在什么设备上"。模拟环境没这些值，
so 拿 null 就崩。军棋实测 3 个卡点。

### 卡点 1：so 查 `android_id`（设备唯一标识）

真机有值（一串 hex），模拟环境没值 → so 拿到 null → 崩 / 走错分支。

- **大白话卡点**：so 查"你的设备身份证号"（`android_id`），模拟环境没这号 → null → 崩。
- **为什么卡**：`android_id` 是 `Settings.Secure` 里的设备指纹，走
  `FindClass("android/provider/Settings$Secure")` + `getStaticStringField`，
  模拟环境没设。
- **不解决会怎样**：so 拿 null 崩，或统计指纹全错。

**"设备身份证号"为什么 so 要查（大白话）**：真 Android 每个设备有唯一的
`android_id`（一串 hex），so 读它做"设备指纹"（统计/识别设备）。模拟环境没这号，
so 读 `Settings.Secure.get("android_id")` 拿 null → 崩或统计错。军棋 so 读它
主要做统计（不做逻辑分支），填个占位 hex 就行。

### 卡点 2：so 查目录路径（`getFilesDir`）

真机返回 `/data/data/com.cnvcs.junqi/files`，模拟环境没这路径 → 崩。

- **大白话卡点**：so 问"我的存档柜在哪"（`getFilesDir`），模拟环境没这目录 → 崩。
- **为什么卡**：`one.sjg` 存档路径是设备路径，宿主没准备（见 16 章 I/O 重定向）。
- **不解决会怎样**：`OnPause` 保存 `one.sjg` 失败。

**"存档柜路径"为什么查（大白话）**：军棋 `one.sjg` 存档在
`/data/data/com.cnvcs.junqi/files/`，so 调 `Context.getFilesDir()` 拿这路径。
模拟环境没这目录（虚的），`getFilesDir` 返 null → 存不了档。挂宿主
（`./junqi_data/files`）后，`getFilesDir` 返虚拟路径，读写都落宿主。

### 卡点 3：so 可能查"设备型号/分辨率"（选资源目录）

真机有设备分辨率字段，模拟环境要补。

- **大白话卡点**：so 查"你用的什么屏"（选分辨率目录），模拟环境没填 → 选错。
- **为什么卡**：`data/{320,480,480x,720,720s,800}/` 按设备分辨率选，没填就选错。
- **不解决会怎样**：资源目录选错，显示异常。

**"设备型号"为什么查（大白话）**：军棋按设备屏宽选 6 个分辨率目录
（`320/480/480x/720/720s/800`），so 读设备分辨率（`getDefaultDisplay/
getWidth/getHeight`，`OnCreate` 90% 里程碑里已实测应答）→ 选对应目录。
模拟环境没填 → 选错目录 → 资源显示异常。固定选一套（如 720）。

## 为什么这样做：补"设备身份"

设备风控补环境 = 给 so 一套"假设备身份"，让它以为跑在真 Android 上。

类比：so 是"查证件的保安"，你给它一套"假证件"（`android_id` / 设备型号 /
目录），它查完放行。

军棋设备风控 3 类（按难度）：

| 类 | 大白话 | 军棋场景 |
|---|---|---|
| `android_id` | 设备唯一标识（Settings.Secure） | 军棋读它做"设备指纹" |
| 目录获取 | `getFilesDir` / `getCacheDir` | 军棋存档 `one.sjg` 路径 |
| 设备分辨率 | `getDefaultDisplay` 宽高 | 军棋选 6 个分辨率目录 |

**预判追问："android_id 填随机 hex 就行，为什么还要走 FindClass + 静态字段
那一套？"** 因为 so 读 `android_id` 走的是 JNI 三步：
`FindClass("android/provider/Settings$Secure")` →
`GetStaticFieldID("ANDROID_ID", "Ljava/lang/String;")` →
`GetStaticStringField`。你在 unidbg 侧要三个都应答（前两个返正常
方法号/字段号，第三个才返你填的值）。只填"值"不补"类 + 字段号"，
so 在前两步就崩了。军棋 §124f 的 JunqiJni 全面 handler 就是干这个的
（覆盖 getStatic*/callStatic* 全部变体，实测 37+ 个 JNI 调用全应答）。

**预判追问："分辨率不是固定选一套吗？为什么不模拟'按屏宽自动选'？"**
模拟"自动选"需要喂真实 `getWidth/getHeight`（比如 1080×1920 → 选 800），
看似更真，但引入"选哪套取决于分辨率值"的分支——而军棋资源 6 套内容
（棋子图集/棋盘几何已逆向对齐），选哪套不影响"能玩"路径。固定选 720
是"最小必要"：少一个分支少一个崩点。要"更真"，就按真实机型喂
宽高，选对应目录——两者都行，军棋选简单方案。

**"假证件"三件套（大白话）**：
- `android_id`（身份证号）= 填占位 hex（"0000000000"），军棋不做分支，占位就行
- 目录（门牌号）= 挂宿主 `./junqi_data`（`one.sjg` 落宿主）
- 设备分辨率（住的什么房）= 固定选 720（`data/720/` 这套资源）
三件套补全，so 查"证件"全过，以为在真 Android 上。

## 24.1 补 `ActivityThread`（原书 24.1.1，军棋映射过简处）

原书 24.1.1 专讲 `ActivityThread` 补环境——so 通过 `Application.getApplication()` /
`ActivityThread.currentApplication()` 查"当前 app 是谁"，mock 它要返回一个 app 替身。

**军棋映射**：军棋 `com.cnvcs.App` 继承 `Application`（smali 层 7 类之一），DalvikVM
要 mock `getApplication()` 返回军棋 App 对象替身。但军棋 so 的设备风控点主要落在
`android_id` / `getFilesDir` / 分辨率（下面 24.2/24.3），**没走 ActivityThread 查设备**
（军棋 32 位 so 未调 `Application.getApplicationContext()` 查设备属性）。

**为什么补 ActivityThread**：unidbg 通用框架需 mock `ActivityThread` 类（其他 app 适用），
军棋场景下"补了但不用"—— DalvikVM 里建好 `ActivityThread` 替身（`getCurrentActivity`
返 null / `getApplication` 返军棋 App 替身），so 真调了就有人接，不调也不崩。

## 24.2 补 `android_id`（军棋）

```java
// 军棋 so 读 Settings.Secure.ANDROID_ID（设备唯一标识）
// unidbg 模拟: 把 Settings$Secure 类 + getStaticStringField 补上
vm.setJni(new Jni() {
    @Override public DvmClass<?> findClass(BaseVM vm, String name) {
        if (name.equals("android/provider/Settings$Secure")) {
            return vm.resolveClass(name);
        }
        if (name.equals("com/baidu/mobads/AdView")) return null; // 去广告(23 章)
        return vm.resolveClass(name);
    }
    // getStaticFieldID / getStaticStringField 走 JunqiJni 全面 handler
});
// android_id 值 = 占位（军棋不校验真值，填个 hex 即可）
// 模拟时 DvmObject 里 ANDROID_ID 字段 = "0000000000"
```

**为什么占位就行？** 军棋 so 读 `android_id` 主要做"设备指纹记录"（统计用），
不拿它做逻辑判断（不会"`android_id` 不对就走异常分支"），所以填个占位 hex
不崩即可（不像风控 so 会拿它做分支——军棋 `.rodata` 明文里没有风控特征串，
已证非壳非风控）。

## 24.3 补目录获取（军棋 `one.sjg` 路径）


> **本节核心**：
> 1. 补目录获取（军棋 one.sjg 路径）
>
> **本节难点**：
> 1. 军棋 getFilesDir → /data/data/com.cnvcs.junqi/files/；映射宿主 ./junqi_data/
> 2. 「目录获取」是 I/O 路径核心
> 3. one.sjg 存档路径是虚拟设备路径

```java
// 军棋 so 调 Context.getFilesDir() → 真机返回 /data/data/com.cnvcs.junqi/files
// unidbg 模拟: 返回虚拟路径(对应宿主 ./junqi_data/files)
emulator.getFileSystem().link(
    new File("/data/data/com.cnvcs.junqi/files"),
    new File("./junqi_data/files"));
// one.sjg 存档就在 ./junqi_data/files/one.sjg
// OnCreate 读它续局、OnPause 写它保存，都落在宿主
```
**为什么挂宿主而不是硬编码路径？** 挂宿主后，`one.sjg` 是"真文件"
（你放的对局存档），so 读写都落在宿主，改存档也落得回 PC。硬编码
"虚拟路径"则读写不到真文件（见 16 章 link vs copy 的追问）。
## 24.4 补设备分辨率（军棋选资源目录）


> **本节核心**：
> 1. 补设备分辨率（军棋选资源目录）
>
> **本节难点**：
> 1. 军棋 7 资源目录按分辨率（data/{320,480,480x,720,720s,800}/）
> 2. 分辨率 mock 错选错目录渲染崩
> 3. 「分辨率」是资源目录选择依据

军棋 so 读设备分辨率（`OnCreate` 90% 里程碑里 `getDefaultDisplay/getWidth/
getHeight` 已实测应答）→ 选 `data/{320,480,480x,720,720s,800}/`
哪个分辨率目录。
```java
// unidbg DalvikVM 提供 DisplayMetrics 模拟
// 军棋按设备分辨率选资源目录(红线 §R6 已逆向 6 个分辨率)
// 模拟时填一个固定分辨率(720), so 选 data/720/ 这套资源
vm.setJni(juniWithStatics()); // JunqiJni 全面 handler 应答 getDefaultDisplay 等
```
**军棋 6 个分辨率目录（红线 §R6 已逆向）**：`320/480/480x/720/720s/800`。
模拟时挑一个（比如 720），so 就选 `data/720/` 这套资源。真实设备按屏宽选，
模拟时固定选一套即可。
**"选一套"为什么够（大白话）**：真机按设备屏宽（dp）选 6 个目录之一，
模拟时没真设备，固定选一套（720）就够——so 读 `data/720/` 这套资源，
显示正常。选哪套不影响"能玩"（资源内容一样，只是分辨率不同）。

## 24.4b 补文件属性风控（`stat64` 读目录属性 + rootfs，社区快手案例）

**吸收自社区实战**（gla2xy"从案例中学习 unidbg 补系统调用"快手案例）：设备风控不只查
`android_id`/目录/分辨率，**还会用 `stat64` 读文件夹属性的"最后访问时间戳"做设备指纹**——
这是上面 24.2/24.3/24.4 没覆盖的**第 4 类设备风控**（文件属性级）。

**卡点（快手 libksse.so 实测）**：
- so 调 `stat64` 读 5 个文件夹的 `st_atime`（最后访问时间），把纳秒时间戳拼成 5 段 `::` 分隔串返回
- unidbg 默认没建虚拟文件系统根目录 → `stat64` 读不到 → 返回乱码时间戳（跟 Frida 真机 hook 结果对不上）
- **补法**：`AndroidEmulatorBuilder.setRootDir(new File(".../rootfs"))` 建虚拟根目录 + 在 rootfs 下放这 5 个文件（属性对齐真机）
- **补完结果**：返回值从乱码变"全 0"（文件属性 mock 上）——证明 so 确实在读这 5 个文件的属性

**军棋设备风控要补 4 类（本节补全第 4 类）**：

| 类 | 怎么查 | 怎么补 | 课程对应 |
|---|---|---|---|
| ① 设备标识 | `android_id` / 序列号 | mock 固定值 | 24.2 |
| ② 目录路径 | `getFilesDir`/`getExternalFilesDir` | link 宿主 | 24.3 |
| ③ 分辨率 | `getDefaultDisplay` | 固定选一套 | 24.4 |
| **④ 文件属性** | **`stat`/`stat64` 读目录 atime/mtime** | **`setRootDir` 建 rootfs + 放文件** | **24.4b（新增）** |

**军棋现状**：军棋 32 位 so 的设备风控主要走 ①②③（android_id/getFilesDir/分辨率），**是否走 ④（stat64 读文件属性）需 redroid 跑原版 trace 系统调用确认**——如果军棋 so 调 `stat`/`stat64` 读目录做指纹，unidbg 侧要 `setRootDir` 补 rootfs + 文件属性。这是 24 章当前漏的"第 4 类设备风控"（社区快手案例实证的坑）。

## 24.5 军棋 14 入口的设备风控取样点


> **本节核心**：
> 1. 军棋 14 入口里 4 个查设备（OnCreate/OnTouch/OnBluetooth/OnUpdate），10 个不查
>
> **本节难点**：
> 1. 「设备风控取样点」是 14 入口的设备查询分布
> 2. OnCreate 查 android_id + 分辨率，OnTouch 查分辨率（渲染）
> 3. 14 入口设备风控取样点全标

![军棋设备风控](assets/ch24_device_risk.png)



军棋 14 个 JNI 入口里（readelf 实测：OnCreate / OnDestroy / OnResult /
OnResume / OnEvent / OnKey / OnUpdate / OnGraphChange / OnBluetooth /
OnPause / OnGraphCreate / OnTimer / OnInput / OnTouch），设备风控集中在
两个取样点：

| 入口 | 风控取样 | 大白话 |
|---|---|---|
| `OnCreate` | `android_id` + 目录 + 分辨率 + registerReceiver×3（蓝牙） | 开局时"验证件"全在此 |
| `OnBluetooth` | 蓝牙设备配对（UUID `ce1404fe-0863-4278-86c2-c9c560f0e215` + 服务名 `junqi` + 版本 v2 + 7字段帧 `v2\n%d,%d,%d,%d,%d,%d,%d\n` + 60格全盘同步） | 联机时查"对端设备" |

```java
// unidbg 调 OnBluetooth(军棋 14 入口之一, 蓝牙同步)
// getJNIEnv + addGlobalObject + call, 和第 12/18 章调 OnCreate 同理
long ret = vm.getMemory().call(emulator, 0xBLUETOOTH_ADDR, 0, jniEnv, objHash, eventStr).singleValue();
```

**14 个入口的对局闭环（大白话）**：

| 阶段 | 入口 | 大白话 |
|---|---|---|
| 开局 | `OnCreate` | 棋盘初始化、读配置、设备风控全在此 |
| 玩家操作 | `OnInput`/`OnTouch`/`OnKey` | 点/触/键盘走子 |
| 走子刷新 | `OnUpdate`/`OnEvent`/`OnGraphChange` | 吃子/翻棋/棋盘变化 |
| 计时 | `OnTimer` | 超时/70 步判和 |
| 结果 | `OnResult` | 判胜负/悔棋 |
| 暂停/恢复 | `OnPause`/`OnResume` | 存档/续局 |
| 联机 | `OnBluetooth` | 蓝牙同步 |
| 收局 | `OnDestroy` | 清理 |

14 个入口走完 = 一局完整流程。设备风控只挡在 `OnCreate` 第一步（"验证件"），
过了 `OnCreate` 后面的入口就不再查设备了。

## 24.6 逐环节"我们能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **android_id** | 占位 hex（"0000000000"），FindClass + getStaticStringField 全应答（JunqiJni 37+ JNI 调用实证） | 拿真设备指纹（模拟环境没有真 `Settings.Secure` 数据源） |
| **目录获取** | 挂宿主 `./junqi_data`，`one.sjg` 读写落宿主（I/O 重定向） | 模拟真机 SELinux 文件权限（宿主是 PC 文件系统） |
| **分辨率** | 固定选一套（720），`getDefaultDisplay/getWidth/getHeight` 应答 | 按真实机型"自动选"（可以喂真实宽高，但军棋资源 6 套内容一致，选哪套不影响"能玩"） |

**军棋设备风控实测小结**：3 类都补得上（占位/挂宿主/固定选）——属"补环境"
范畴（能补）。但军棋真正的卡点不在设备风控：`OnCreate` 已达 ~90%
（§124d-g：no-init_array 路径 → code cave 0x7965a→0xc3a68→0x43ed8 →
vecfix 37 次 → JunqiJni 处理 20+ JNI 调用 → GLView 创建 + EGL 配置 +
setContentView + registerReceiver×3），最后 10% 被 **no-init_array 全局未
初始化**挡住（§124g：崩溃 0x23b7a `ldrb r2,[r4,r3]`，std::string 拷贝循环
读垃圾源，LR=垃圾 0x39a63305 → 垃圾函数指针调用链 → READ_PROT）。
设备风控是"验证件"（补环境能补），C++ 全局对象图是"查手艺"
（§124g 实证：init_array 路径 bx bug + no-init_array 路径全局未初始化，
两者互为死锁）——能玩卡在手艺不在证件。

## 24.7 本章小结（一句话版）

> 设备风控 = so 查"设备身份"（`android_id` / 目录 / 设备分辨率）。
> 军棋补 3 类：`android_id` 填占位（不做分支）、目录挂到 `./junqi_data`
> （`one.sjg` 存档）、设备分辨率选（6 个目录固定选 720）。
> 14 个入口以 `OnTimer`（定时）/ `OnInput`（输入）/ `OnResult`（结果）收尾，
> 构成"开局→操作→走子→超时→结果→收局"的对局闭环。
> 设备风控是"补环境"（能补）；C++ 全局对象图（§124g 实证：垃圾调用链
> READ_PROT）是"补不了"的天花板。

## 视频
