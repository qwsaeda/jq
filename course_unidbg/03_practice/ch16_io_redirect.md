# 第16章 unidbg 实战：I/O 重定向

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的 `data/config/*.xml` 读取 + `one.sjg` 存档

## 痛点：为什么 so 一读文件就崩？

军棋 so 一启动就 `open("data/config/ui.xml")` 读配置，但你的 PC 上没这个文件。
I/O 重定向解决的就是"so 以为路径在真 Android 上，实际要在 PC 上读到文件"这件事。
军棋 14 个 JNI 入口（readelf 实测：OnCreate / OnDestroy / OnResult / OnResume /
OnEvent / OnKey / OnUpdate / OnGraphChange / OnBluetooth / OnPause /
OnGraphCreate / OnTimer / OnInput / OnTouch，Thumb 奇地址如 OnCreate 0x233e1）
里 `OnCreate`/`OnPause` 直接读写这些文件。军棋实测 3 个卡点。

### 卡点 1：你的 PC 上没这个文件

so 默认它跑在真 Android 上，文件在 `/data/data/com.cnvcs.junqi/`，
你的 PC 根本没这路径 → `open` 返回 -1 → so 崩。

- **大白话卡点**：so 要读"它家冰箱里的 7 个配置文件"，你 PC 上没这个冰箱。
- **为什么卡**：模拟环境的 `/data/data/com.cnvcs.junqi/` 是虚的，宿主没这目录。
- **不解决会怎样**：`open` 失败 → 配置读不到 → so 崩。

**7 个 config XML 是什么（大白话）**：

| 文件 | 大白话 |
|---|---|
| `ui.xml` | 界面布局（按钮/对话框位置） |
| `font.xml` | 字体（棋子用 53×40 格子渲染） |
| `image.xml` | 图片（棋盘 737×751 十字形 + 棋子图集 212×280） |
| `sound.xml` | 音效（Ui 组 Click/Close/Pop + Junqi 组 12 个） |
| `string.xml` | 字符串（"军旗""地雷""悔棋"等中文） |
| `path.xml` | 路径（分辨率目录选择） |
| `uitag.xml` | UI 标签（菜单项） |

军棋 `OnCreate` 一启动就读这 7 个，缺一个配置就错。

### 卡点 2：so 写存档到设备路径

对局保存写 `/data/data/com.cnvcs.junqi/files/one.sjg`，PC 上没这目录 → 写失败 → 崩。

- **大白话卡点**：so 要"把对局存档写进它家存档柜"（`one.sjg`），你 PC 上没这柜子。
- **为什么卡**：`one.sjg` 路径是设备路径，宿主没准备。
- **不解决会怎样**：`OnPause` 保存失败 → 崩溃 / 存档丢。

### 卡点 3：so 可能 printf 打日志，排查时"哑巴吃黄连"

没重定向 stdout/stderr，日志丢失 → 你排查不到内部逻辑。

- **大白话卡点**：so 打日志（printf/logcat），你没接住 → 看不到它内部在干嘛。
- **为什么卡**：日志没重定向到宿主，全丢。
- **不解决会怎样**：排查 so 内部逻辑时看不见"它读到哪个配置、走到哪一步"。

**军棋 so 的日志**：32 位 so 有 `liblog`（`__android_log_print`，军棋 7 个
DT_NEEDED 之一），真机打 logcat。模拟环境没接 logcat，日志全丢——
排查"读哪个配置崩的"时看不见。unidbg 可把日志接到宿主文件（logCallback）。

## 为什么这样做：把"虚拟路径"映射到"宿主目录"

I/O 重定向的思路：so 以为自己在读 `/data/data/com.cnvcs.junqi/`，
实际读的是你 PC 上的 `./junqi_data/`。

类比：so 是"住 1 楼的租客"，你给它一套"1 楼房子"的钥匙（宿主目录），
它打开门就拿到真东西（你准备的文件）。

```java
// FileSystem 把宿主目录挂到虚拟路径（给租客一把"1 楼钥匙"，指向你 PC 的真房子）
FileSystem fs = emulator.getFileSystem();
fs.link(new File("/data/data/com.cnvcs.junqi"), new File("./junqi_data"));
// so 读 /data/data/com.cnvcs.junqi/data/config/ui.xml
//   → 实际读 ./junqi_data/data/config/ui.xml（你 PC 上准备的真文件）
```

**为什么是 link（挂）不是 copy（拷）？** link 是"虚拟路径指向宿主真实目录"，
so 读写的都是你 PC 上你放好的真文件；copy 是"复制一份进模拟环境"，改了存档
也落不回宿主。军棋要读 7 个 config XML + 写 `one.sjg` 存档，用 link 才能"双向"
（读配置、写存档都落在宿主）。

**预判追问："直接用宿主绝对路径不挂载行不行？"** 不行。so 里写的是
`/data/data/com.cnvcs.junqi/`（绝对路径硬编码在 `.rodata` 明文里），
你不可能让 so 改用 PC 路径——so 是 stripped 二进制，改字节就破坏原版
（军棋红线 §R6：不碰 so 字节）。所以只能"so 报什么路径，你就把那个路径
映射到宿主真目录"，路径在 so 侧固定、映射在宿主侧灵活。

**link 的"双向"（大白话）**：
- so 读 `one.sjg` → 宿主 `./junqi_data/files/one.sjg`（你放的对局存档，so 读续局）
- so 写 `one.sjg` → 宿主 `./junqi_data/files/one.sjg`（so 保存落回你 PC）
读写都落宿主，改存档也落得回 PC，真机体验一致。

## 16.1 军棋的 I/O 路径清单（红线 §R6 已 dump）


> **本节核心**：
> 1. 军棋 I/O 路径清单（红线 §R6 已 dump）：7 config XML + one.sjg 存档
> 2. so 读虚拟路径（/data/data/com.cnvcs.junqi/files/ui.xml）映射宿主（./junqi_data/...）
>
> **本节难点**：
> 1. 7 config XML：data/config/{ui,font,image,sound,string,path,uitag}.xml
> 2. 「虚拟路径映射宿主」是 I/O 重定向核心
> 3. so 写存档 one.sjg 到设备路径，PC 上要映射宿主目录

![军棋 I/O 路径](assets/ch16_io_paths.png)


![unidbg I/O 重定向流程](assets/ch16_redirect_flow.png)

军棋 so 读这些文件（`.rodata` 明文 1715 条字符串清单里提取，静态即可见）：

| 路径 | 大白话 | 军棋用途 |
|---|---|---|
| `data/config/ui.xml` | 界面布局 | 按钮/对话框位置 |
| `data/config/font.xml` | 字体 | 军棋棋子用 53×40 格子渲染 |
| `data/config/image.xml` | 图片 | 棋盘 737×751 十字形 + 棋子图集 212×280 |
| `data/config/sound.xml` | 音效 | Ui 组 Click/Close/Pop + Junqi 组 12 个 |
| `data/config/string.xml` | 字符串 | "军旗""地雷""悔棋"等中文 |
| `data/config/path.xml` | 路径 | 分辨率目录选择 |
| `data/config/uitag.xml` | UI 标签 | 菜单项 |
| `data/{320,480,480x,720,720s,800}/` | 分辨率资源 | 按设备屏选 |
| `files/one.sjg` | 对局存档 | 续局 / 保存 |

**7 个 config XML 是"界面四件套"**：ui（布局）+ font（字）+ image（图）+ sound（音）
+ string（文）+ path（路径选择）+ uitag（菜单标签）。`data/{320,480,480x,720,720s,800}/`
是 6 个分辨率目录，按设备屏幕选一套资源（军棋 10×9 棋盘、38 子布局都在这套里）。

**6 个分辨率目录按什么选？** 按设备屏宽（dp），军棋逆向（红线 §R6）：
`320/480/480x/720/720s/800` 各对应一套资源，真机按屏宽挑一套。模拟时固定选一套
（比如 720），so 读 `data/720/` 这套。

**预判追问："这些文件从哪拿？so 里不就包着吗？"** 军棋 APK 的 `assets/` 里有
`data*.pak`（SPAK 打包资源，算法 `XOR(key[j%32]) + zlib`，key 32B 在
`.rodata` VA 0x12a658 明文已逆向，9 包 883 条目可解包，R5 确证）。`.so` 里只有路径字符串（`.rodata` 明文），资源实体在 APK 侧解包后
放到 `./junqi_data/` 供 so 读——这就是"挂宿主目录"要准备的物料。

## 16.2 存档文件 `one.sjg`（军棋对局保存）

军棋 so 写存档 `one.sjg`（红线 §R6 已逆向格式）：

- `board`（60 格全盘同步）+ `turn/stage/winner`（轮到谁/阶段/胜负）
- `flags/mines/pieces`（军旗/地雷/棋子位置）+ `moves`（走子记录）
- `OnCreate` 续局读 `one.sjg`，`OnPause` 保存，对局结束清理

```java
// 重定向存档路径（PC 上放 one.sjg，so 读写都落在宿主）
emulator.getFileSystem().link(
    new File("/data/data/com.cnvcs.junqi/files"),
    new File("./junqi_data/files"));
// so 读 one.sjg → 实际读 ./junqi_data/files/one.sjg
// so 写 one.sjg → 实际写 ./junqi_data/files/one.sjg（OnPause 保存落宿主）
```

**`one.sjg` 格式（大白话）**：

| 字段 | 大白话 |
|---|---|
| `board`（60 格） | 全盘 60 格（棋盘每格一个棋子位） |
| `turn/stage/winner` | 轮到谁 / 对局阶段 / 胜负 |
| `flags/mines/pieces` | 军旗/地雷/棋子位置 |
| `moves` | 走子记录（悔棋靠它） |

**为什么 `one.sjg` 要重定向到宿主？** 军棋 `OnCreate`（14 个 JNI 入口之一，
Thumb 奇地址 0x233e1）要读它续局、`OnPause` 要写它保存。不重定向，这两个
JNI 入口一读一写都失败（路径不存在）。重定向后，你放一个真实 `one.sjg`
在 `./junqi_data/files/`，so 就能"读上局、存这局"。

## 16.3 逐环节"我们能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **虚拟路径挂载** | `fs.link` 把 `/data/data/com.cnvcs.junqi` 挂到宿主 `./junqi_data`，读 7 个 config XML + `one.sjg` 全通 | 改 so 里的路径硬编码（路径在 `.rodata` 明文里，改字节破坏原版，红线 §R6 禁止） |
| **读 config** | 静态 `.rodata` 直接可见路径清单（1715 条），按清单备料 | 让 so 读"宿主任意路径"（so 只认它自己拼出来的虚拟路径） |
| **写存档** | link 双向：`OnPause` 写 `one.sjg` 落回宿主，改存档留得下 | 模拟真机的文件锁/权限语义（宿主没有 Android SELinux，fd 行为不完全等价） |
| **日志** | `liblog`（`__android_log_print`）接宿主文件 | 拿到真机 logcat 完整流（模拟只有接住的那部分） |
| **验证读通** | 7 个 config + 6 分辨率目录 + `one.sjg` 全通后 `OnCreate` 推进到 0x43ed8 | 把"文件读到了"当成"能玩"——`OnCreate` 仍卡 C++ 对象图（§124 的 ②），见 16.4 |

## 16.4 军棋实战验证：link 后 config 读取全通


> **本节核心**：
> 1. link 后 config 读取全通（7 XML 落宿主）
>
> **本节难点**：
> 1. I/O 重定向是补环境第一层（系统调用），全通才到 Java 方法层
> 2. 「能读配置」= 第一层补环境通
> 3. I/O 全通是「能加载」前提，「能调用」还差 §124

军棋 unidbg I/O 重定向实测（红线 §R6，unicorn2 后端 + AndroidResolver(23)）：
| 军棋读/写 | 重定向前 | 重定向后 | 大白话 |
|---|---|---|---|
| 7 个 config XML | `open` 失败（宿主没目录） | 读 `./junqi_data/data/config/*.xml` ✓ | 7 个配置文件读到了 |
| 6 个分辨率目录 | 路径虚 | 读 `./junqi_data/data/{320..800}/` ✓ | 资源目录选得到 |
| `one.sjg` 存档 | 写失败 | 写 `./junqi_data/files/one.sjg` ✓ | 存档落到宿主 |
| `liblog` 日志 | logcat 没接（丢） | 接宿主文件 ✓ | 排查看得见日志 |
**关键认知**：I/O 重定向解决的是"读不到文件"（路径虚），但军棋真正的卡点不在这里
——7 个 config XML 读到了，`OnCreate` 仍推进到 `std::vector` 拷贝构造点 0x43ed8 卡住
（§124 的 ② 数据与指令对齐对应缺口，N44）。重定向是"补文件"，不是"补运行时对齐"，两者要分清。
**"补文件 vs 补对齐"（大白话）**：
- 补文件（I/O 重定向）：把"它家冰箱的食材"准备齐（7 个 config + `one.sjg`）
- 补对齐（运行时内存布局）：把"它做菜的手艺"补对（`std::vector` begin/end/capacity）
军棋"食材"齐了（重定向全通），但"手艺"补不对（§124 的 ②），所以"能读配置"
≠ "能跑通 `OnCreate`"。
## 16.5 本章小结（一句话版）

> so 读文件崩 = 它以为路径在真 Android 上。
> I/O 重定向 = 把虚拟 `/data/data/com.cnvcs.junqi/` 映射到 PC 的 `./junqi_data/`，
> so 拿"虚拟钥匙"开门就拿到真文件（link 双向：读配置 + 写存档都落宿主）。
> 军棋要准备的：7 个 config XML + 6 个分辨率目录 + `one.sjg` + `liblog` 日志。
> 重定向解决"读不到文件"，但 `OnCreate` 仍卡数据与指令对齐 0x43ed8（§124 的 ②），
> 别把"文件读到了"误读成"能玩了"。

## 视频
