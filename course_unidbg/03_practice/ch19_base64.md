# 第19章 unidbg 实战：SPAK 资源解密还原

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的 SPAK 解密（rev64/phases/R5_spak.md 确证）
>
> **原书标题**："魔改 Base64 还原"（书内样本是魔改 Base64 + 256 字节置换表）。
> **军棋实际**：SPAK 打包（**`XOR(key[j%32]) + zlib inflate`**，key 32B 在 `.rodata`
> VA `0x12a658`），**不是魔改 Base64**——R5_spak.md 逆向确证，推翻课程早期推测。
> 本章按军棋实际 SPAK 讲，保留"魔改 Base64"作为原书方法论对照。

## 痛点：为什么 SPAK 资源解密难？

军棋的棋子图集/棋盘资源用 SPAK 打包（`XOR(key[j%32]) + zlib`，**不是**标准 Base64、
**不是**普通 ZIP），解密要 3 步，每步都有坑。资源在 14 个 JNI 入口（readelf 实测：
OnCreate / OnDestroy / OnResult / OnResume / OnEvent / OnKey / OnUpdate /
OnGraphChange / OnBluetooth / OnPause / OnGraphCreate / OnTimer / OnInput /
OnTouch）里的 `OnCreate` 解包阶段被读取。军棋实测 3 个卡点。

### 卡点 1：算法不是"标准 base64/zip"，是 XOR + zlib

SPAK 每个字节与 `key[j % 32]` 异或，再 zlib 解压。标准 `base64 -d` / `unzip`
直接失效。

- **大白话卡点**：军棋把资源"XOR 异或 + zlib 压缩"打包（不是"普通话"的标准编码），
  用普通话（标准 base64 / unzip）听不懂。
- **为什么卡**：标准 base64 用 64 字符表，军棋根本没用 base64；标准 zip 有
  自己的魔数，军棋 SPAK 魔数是 `0x4b415053`（"SPAK" 倒序）+ 自定义结构。
- **不解决会怎样**：`base64 -d` / `unzip` 解出乱码，棋子/棋盘资源全错。

**为什么"XOR + zlib"比"标准编码"难？** XOR 得按 key 长度 32 做循环异或
（`key[j % 32]`），zlib 还得先拿到 XOR 还原后的字节流才解压，两步缺一不可。
军棋逆向时就踩过"把棋盘当 zlib 解"的错分支（R5_spak.md 修正：棋盘走 XOR 不走 zlib）。

### 卡点 2：钥匙在哪？

解密要 SPAK key（32 字节），你得先找到它在 so 里的位置。

- **大白话卡点**：钥匙（32B SPAK key）藏在 so 里，不找到钥匙打不开锁。
- **为什么卡**：key 是 XOR 解密的驱动，找不到就没法解。
- **不解决会怎样**：资源全解不开，棋子/棋盘图显示乱码。

**军棋 key 在哪（R5 实测）**：SPAK key 在 `.rodata` VA `0x12a658`
（`c2 cb e5 e7 ce 5c 4a c4 11 ca 6c 99 06 f4 76 f8 6d 4f cb b6 26 21 de 0e 6a 67 87 4c 93 f0 d5 23`），
`.rodata` 非壳、静态即明文（0/118832 不同），`objdump` 直接 dump 就拿到。
"钥匙挂在墙上"（明文可见），不用 unidbg 搜。

### 卡点 3：9 包 883 条目的结构看不懂

资源分 **9 包**（data 121 / datah 272 / datam 189 / datax 301 条目），包间关系不明，
直接读是乱码。

- **大白话卡点**：资源打包成 9 包共 883 条目，包间关系不明，直接读是乱码。
- **为什么卡**：9 包是"打包结构"，得按结构解。
- **不解决会怎样**：乱码，资源不可用。

**9 包 883 条目对应什么（R5 实测，大白话）**：
- **data 包**（121 条目）：按 `MD5(资源路径)` 寻址 —— `data/lay/00-11.lay`、`data/config/*.xml`、`data/font/*.ttf`、`data/junqi/*.wav`
- **datah 包**（272 条目）：按 UI 布局 XML 索引（**非**路径 MD5）—— 含棋子图集 212×280
- **datam 包**（189 条目）：同上，按 UI 布局 XML 索引
- **datax 包**（301 条目）：同上，按 UI 布局 XML 索引
- 内容分类（R5 魔数扫描）：310 PNG / 408 XML / 18 WAV / 20 字体 / 127 其他（含 127 条广告 `id`/`app` 数据，拆除目标）

**文件表条目 28B**：`hash16 + off4 + size4 + usize4`，`hash = MD5(资源路径)`（data 组 29/29 验证通过）。

## 为什么这样做：静态读 key + 动态 Hook 解包

军棋的 SPAK key **在 `.rodata` 静态明文**（红线 §R6 已证：`.rodata` 118KB
静态即明文，与运行时 dump 逐字节相同 0/118832 不同，无自解密例程）。所以：

- **key 静态读**：不用 unidbg，直接 `readelf` / `objdump` 从 `.rodata` 拿 32B key
- **解包动态 Hook**：用 unidbg 跑 so 的解包函数，Hook 它的入口，捕获 key + 密文
  → 在 PC 上还原

类比：key 是"挂在墙上的钥匙"（明文可见，直接拿），但"锁"（解包函数）得真转
一下才能开（Hook 跑）。

**预判追问："key 都静态可见了，为什么还要动态 Hook 一遍？"** 因为 key 只是
"字典"，"怎么把密文按字典解回原文"是解包函数的控制流（哪组走 zlib、哪组
走 XOR，条目顺序怎么排）。静态能拿 key 和表，拿不到"解包后每个条目的内容"
和"分支走对没走对"——军棋逆向时就踩过把棋盘当 zlib 解的错分支（红线 §R6
专门修正）。动态 Hook 跑一遍是"验分支"，不是"拿 key"。

**预判追问："Hook 解包函数入口，不还得先把 OnCreate 跑通？"** 对。解包函数
是 so 的一部分，要跑它得先把 so 加载 + `OnCreate` 推进到解包点。军棋
`OnCreate` 卡 0x43ed8（§124 的 ② C++ 对象图语义缺失）→ 没到解包点，
unidbg 路线 Hook 不了 → 退而 redroid 跑原版 so（真 dlopen + 真重定位，
解包能推进）dump 数据。

**"钥匙 vs 锁"（大白话）**：
- 钥匙（key）= 明文挂在墙上，直接拿（静态 dump 32B）
- 锁（解包函数）= 得真转一下才开（redroid 跑 / 真机跑）
钥匙静态拿、锁动态转，两者配合才解出资源。

## 19.1 军棋 SPAK 解密流程（红线 §R6 + R5 已逆向）


> **本节核心**：
> 1. 军棋 SPAK 解密流程（XOR(key[j%32]) + zlib inflate，R5 确证）
> 2. key 32B .rodata VA 0x12a658（明文 c2 cb e5 e7 ce 5c ...）
>
> **本节难点**：
> 1. 9 包 883 条目（data 121 / datah 272 / datam 189 / datax 301）
> 2. 「XOR+zlib」三步：读 key → XOR 解包 → zlib inflate 出明文资源
> 3. 原书「魔改 Base64 还原」是书内样本，军棋实际 SPAK（章标题保留对照）

![军棋 SPAK 资源解密流程（`XOR(key[j%32]) + zlib`，key VA 0x12a658，9 包 883 条目）](assets/ch19_spak_flow.png)


![军棋 SPAK 资源解密流程（`XOR(key[j%32]) + zlib`，key VA 0x12a658，9 包 883 条目）](assets/ch19_spak_flow.png)

**SPAK 格式实际确证（rev64/phases/R5_spak.md + REVERSE_NOTES §85）**：
- **不是"魔改 Base64"**——实际算法是 **`XOR(key[j%32]) + zlib inflate`**（XOR 异或 + zlib 解压）
- **key 32B**：`.rodata` VA `0x12a658`（明文态，`c2 cb e5 e7 ce 5c 4a c4 11 ca 6c 99 06 f4 76 f8 6d 4f cb b6 26 21 de 0e 6a 67 87 4c 93 f0 d5 23`）
- **9 包 883 条目**（不是"4 组"）：data 121 / datah 272 / datam 189 / datax 301 = 883
- **文件表条目 28B**：`hash16 + off4 + size4 + usize4`
- **寻址语义**：data 组按 `MD5(资源路径)` 寻址（38+ 个验证通过）；datah/datam/datax 按 UI 布局 XML 索引
- **内容分类**：310 PNG / 408 XML / 18 WAV / 20 字体 / 127 其他（含 127 条广告 `id`/`app` 数据，拆除目标）

```
SPAK key (32B, .rodata VA 0x12a658 静态明文)   ← 钥匙（墙上挂着，直接拿）
  → 驱动 9 包 883 条目解包(XOR + zlib inflate)  ← 按 9 包结构解
  → data 组按 MD5(资源路径) 寻址(data/lay/00-11.lay, config/*.xml...)
  → datah/datam/datax 按 UI 布局 XML 索引
  → 310 PNG + 408 XML + 18 WAV + 20 字体 + 127 广告数据
  → 棋子图集 212×280（4列×7行，53×40/格，来自 datah 组）
  → 棋盘 737×751 十字形（中央竖带 12行×5列，39×59/格）
```

**9 包 883 条目对应什么？**（rev64/phases/R5_spak.md 实际确证）
- **data 包**（121 条目）：按 `MD5(资源路径)` 寻址 —— `data/lay/00-11.lay`、`data/config/*.xml`、`data/font/*.ttf`、`data/junqi/*.wav`、`data/ui/*.cfg`
- **datah 包**（272 条目）：按 UI 布局 XML 索引（**不是**路径 MD5，0 匹配确证）—— 含棋子图集 212×280
- **datam 包**（189 条目）：同上，按 UI 布局 XML 索引
- **datax 包**（301 条目）：同上，按 UI 布局 XML 索引
- 棋子图集 212×280（4 列 × 7 行，53×40/格，rank→列/子行，side→行；row6=背面/选中/空）
- 棋盘 737×751 十字形（中央竖带 12 行 × 5 列，39×59/格，行营/大本营坐标绿框检测确认）

**XOR + zlib inflate 双分支（大白话，R5 实际算法）**：
- **不是"魔改 Base64"**——实际算法是 **`XOR(key[j%32]) + zlib inflate`**
  （每个字节与 `key[j % 32]` 异或，再 zlib 解压）
- 压缩过的资源（如棋子图集 PNG）= XOR 后 zlib 再解：先 XOR 还原、再 zlib inflate
- 未压缩资源（如棋盘几何）= 直接 XOR 还原（不走 zlib）
- 解包时要走对分支（红线 §R6 已修正——之前把棋盘当 zlib 解错）

**"走对分支"为什么重要？** 把棋子图集当 XOR 解（或棋盘当 zlib 解），解出来全是
乱码。军棋逆向时修正过这个分支（§R6），arm64 对齐版逐条对齐。

## 19.2 SPAK 的 3 个实际特征（R5 确证，纠正"魔改 Base64"说法）


> **本节核心**：
> 1. data 组按 MD5(资源路径) 寻址（38+ 路径验证）
>
> **本节难点**：
> 1. datah/datam/datax 按 UI 布局 XML 索引（非路径 MD5）
> 2. SPAK key 静态明文（.rodata VA 0x12a658），非加密壳
> 3. 「9 包 883 条目」是军棋 SPAK 实际结构（推翻早期「4 组 883」推测）
> 4. 内容分类：310 PNG + 408 XML + 18 WAV + 20 字体 + 127 广告数据

**先纠正一个课程早期说法**：军棋 SPAK **不是"魔改 Base64 + 256 字节置换表"**，
实际算法是 **`XOR(key[j%32]) + zlib inflate`**（R5_spak.md 逆向确证）。
"魔改 Base64"这个说法是课程早期推测，被 R5 的实测推翻——XOR 异或（不用 Base64
字符表）+ zlib 解压，跟 Base64 没关系。
| 特征 | 大白话 | 军棋对应（R5 实测） |
|---|---|---|
| SPAK key 32B | 驱动解密的钥匙（`XOR(key[j%32])`） | `.rodata` VA `0x12a658` 静态明文，32 字节：`c2 cb e5 e7 ce 5c 4a c4 ...` |
| 9 包 883 条目 | 资源打包结构（不是"4 组"） | data 121 / datah 272 / datam 189 / datax 301 = 883 |
| `XOR + zlib` | 加密 + 压缩两件套 | 先 XOR 还原、再 zlib inflate（压缩资源）；未压缩资源只 XOR |
| data 组 MD5 寻址 | 文件表条目 hash = `MD5(资源路径)` | 38+ 个路径验证通过（`data/lay/00-11.lay` 等） |
| datah/datam/datax 索引寻址 | 按 UI 布局 XML 索引（非路径 MD5） | 0 路径 MD5 匹配确证 |
**"key + 包 + 算法"三件套（大白话）**：
- key（32B）= 钥匙（`XOR(key[j%32])` 怎么把密文异或还原）
- 包（9 包 883 条目）= 打包清单（密文怎么分 9 包、每包多少条目）
- 算法（XOR + zlib）= 怎么解（压缩资源 zlib inflate，未压缩直接 XOR）
缺一个都解不出来，军棋三件套全摸清（红线 §R6 + R5）。
**内容分类（R5 魔数扫描）**：310 PNG / 408 XML / 18 WAV / 20 字体 / 127 其他
（含 127 条广告 `id`/`app` 数据，是去广告拆除目标）。
## 19.3 unidbg 辅助分析（Hook 解包入口）


> **本节核心**：
> 1. Hook 解包入口识别 SPAK 算法（XOR+zlib）
>
> **本节难点**：
> 1. unidbg 跑不到解包点（卡 §124），解包走 redroid 跑原版 so
> 2. 「unidbg 辅助」= 看 so 怎么解，非 unidbg 自己解
> 3. redroid 跑原版 so 完成 SPAK 解包，unidbg 只能识别算法

```java
// 1. 静态读 key（不用 unidbg，直接从 .rodata dump 32B）
//    军棋 key 在 .rodata 静态明文，objdump 直接拿
byte[] spakKey = readFile("frida-dump/rodata_dec.bin", OFFSET, 32);
// 2. unidbg Hook SPAK 解包函数入口, 捕获密文(跑一下"锁"才能开)
emulator.getBackend().hook(module.base + SPAK_DECRYPT_ADDR, (hook) -> {
    byte[] cipher = hook.readMemory(dataPtr, 883);  // 某包 883 字节条目区
    // 实际算法: XOR(key[j%32]) + zlib inflate(不是魔改 Base64)
    byte[] plain  = xorThenZlibInflate(spakKey, cipher); // XOR 还原 + zlib 解压
});
```
**军棋这条路为什么"key 静态 ✓、解包卡"？** key 在 `.rodata` 明文直接 dump ✓；
但 Hook 解包函数入口要跑通 so（`OnCreate` 推进到 0x43ed8 卡住，§124 的 ②
C++ 对象图语义缺失）→ 退而 redroid 跑原版 so 真解包。军棋资源（棋子图集/
棋盘）实际靠 redroid dump 出的 `.rodata` 明文 + 已逆向的解包结构还原，
不是 unidbg Hook。
**为什么 Hook 解包"跑通"要 `OnCreate` 先通？** 解包函数是 so 的一部分，
要跑解包函数得先把 so 加载起来 + `OnCreate` 推进到解包点。军棋 `OnCreate`
卡 0x43ed8（§124 的 ②），没到解包点，Hook 不了。redroid 是真跑原版 so
（32 位 Android 容器，有真实 dlopen/重定位），`OnCreate` 能推进到解包，
能 dump 资源——但 redroid 无 GPU，只能 dump 数据、不能验证"能玩"。
## 19.4 逐环节"我们能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **找 key（静态 .rodata）** | SPAK 32B key 静态 dump（`.rodata` VA 0x12a658 明文，`frida-dump/rodata_dec.bin` 118KB 落地） | 把"key + 9 包结构怎么配合"静态推出来（"哪包走 zlib、哪包走 XOR、data 组按 MD5 寻址"是解包函数逻辑，静态看不出组合关系） |
| **Hook 解包入口（动态）** | 方法上可 Hook（解包入口捕获 key + 密文） | 军棋实际跑通——`OnCreate` 卡 0x43ed8（§124 的 ②）到不了解包点，退 redroid |
| **还原资源** | 识别 `XOR(key[j%32]) + zlib` 算法、zlib/XOR 双分支修正、9 包结构 | 9 包 883 条目"全语义"对照（条目内容需真机/redroid 跑解包后核对，unidbg 路线给不出） |

## 19.5 军棋实战验证：key 已提取、解包走 redroid


> **本节核心**：
> 1. key 已提取（.rodata VA 0x12a658），解包走 redroid
>
> **本节难点**：
> 1. key 静态可读（.rodata 非壳 0/118832 不同）
> 2. 18 常量运行时才填走 redroid dump 72B
> 3. 「key 已提取」是静态读，「解包」是 redroid 跑原版

军棋 SPAK 解密实测（红线 §R6 + R5 确证）：
| 环节 | 军棋结果 | 大白话 |
|---|---|---|
| SPAK key 32B | `.rodata` VA 0x12a658 静态明文已 dump ✓ | 钥匙拿到 |
| 算法 `XOR(key[j%32]) + zlib` | 逆向确证（**非魔改 Base64**） | 异或 + 解压 |
| 9 包 883 条目 | data 121 / datah 272 / datam 189 / datax 301 | 打包结构摸清 |
| data 组 MD5(路径) 寻址 | 38+ 路径验证通过 | 文件定位 |
| 棋子图集 212×280 | zlib 解压还原 ✓（datah 组） | 棋子图解出 |
| 棋盘 737×751 十字形 | XOR 还原 ✓ | 棋盘解出 |
| unidbg Hook 解包 | 卡 0x43ed8（§124 的 ②） | 退用 redroid dump |
**诚实边界**：军棋资源还原靠"key 静态 dump + 解包结构逆向 + redroid 真跑"，
不靠 unidbg Hook。unidbg 路线卡在 §124 的 ② C++ 对象图语义缺失，
Hook 解包入口这条路走不通。arm64 对齐版（红线 §R6 + R5）已把棋子图集/棋盘几何
逐条对齐，但 32 位 `.rodata` 明文的 18 个 `.text` 常量池仍需 redroid dump
72B 注入。
## 19.6 本章小结（一句话版）

> 军棋 SPAK **不是魔改 Base64**（早期推测已被 R5 推翻），实际算法
> **`XOR(key[j%32]) + zlib inflate`**（key 32B 在 `.rodata` VA 0x12a658 静态明文）。
> 9 包 883 条目（data 121 / datah 272 / datam 189 / datax 301），
> data 组按 `MD5(资源路径)` 寻址、datah/datam/datax 按 UI 布局索引。
> key 静态 dump 拿到，但"解包函数"得跑一遍才能开——军棋卡 C++ 对象图（§124 的 ②），
> 退用 redroid 真解包。解密后 = 棋子图集（zlib，212×280）+ 棋盘（XOR，737×751 十字形）。

## 视频
