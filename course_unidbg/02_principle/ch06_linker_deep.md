# 第6章 深入 Linker：so 的加载、链接、初始化

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so + unidbg/unicorn2

## 痛点：为什么"加载 so"要拆成三段（加载/链接/初始化）？

很多人以为"加载 so"就是把文件复制进内存完事。军棋实测证明不是——
拆成三段，每段漏一步 so 都跑不起来。

### 痛点 1：so 不是"复制进内存"就算完

`.so` 文件里存的都是"相对地址"（偏移量），真正跑起来要加基址才成"绝对地址"。
这个换算叫重定位——不做，程序就按 0 地址乱跳。

- **大白话卡点**：文件里写的是"距大门第 N 米"，搬进新址（基址 0x40000000）后得改成
  "新址 + N 米"。不改，程序按旧偏移找东西全找错地方。
- **为什么卡**：ELF 文件里的地址都是"文件内偏移"，运行时要换算成"基址+偏移"。
  军棋 5312 处这种换算，漏一处就错一处。
- **不解决会怎样**：程序按 0 地址/旧偏移乱跳 → 段错误。

### 痛点 2：函数内部还有"没填号的电话"

军棋 so 调用 libc 的 214 个函数（`malloc`、`strlen`…），这些调用先经过 PLT
（跳板区，函数调用的"跳板"）转到 `.got`（电话簿，跳板后面记真号的簿子），
号没填就是 0，一调用就 SIGSEGV（段错误）。

- **大白话卡点**：so 里 214 个"打外部电话"的跳板，每个跳板后面挂的电话簿格子里
  写的是"要拨哪个号"。电话簿没填（=0），拨号就拨到 0 → 崩。
- **为什么卡**：PLT/GOT 是动态链接的核心，静态文件里 `.got` 槽全是 0（未重定位）。
- **不解决会怎样**：任何走 PLT 跳板的调用（malloc/strlen…）都跳地址 0 → SIGSEGV。

### 痛点 3：加载完还要"通水电"

so 里有 12 个初始化函数（`.init_array`），跑 `OnCreate` 之前必须全执行完。
漏跑任何一个，全局变量就是脏的，后面逻辑全错。

- **大白话卡点**：so 全局变量（比如"棋盘对象"）要靠 12 个构造函数初始化。
  跳过任何一个，拿到的就是未初始化的脏值，后面逻辑全错。
- **为什么卡**：`.init_array` 是 dlopen 时自动跑的一次性构造函数，unidbg 得逐个执行。
- **不解决会怎样**：全局变量脏 → `OnCreate` 读到脏值 → 对象图错乱。

### 痛点 4：填完号之后还有"18 个运行时地址"静态读不出

军棋 14 个 JNI 入口"可运行"卡在**运行时 18 个地址**（15 `.text` 常量池 + 2 `.data`，
共 72B）。这些值**仅 dlopen 重定位填**（`.so` 未重定位，`.got` 槽为零），
**不是壳自解密**（已证 `.rodata` 静态即明文、无自解密例程）。静态 `.so` 读不出这 18 地址，
拿法唯一：redroid 跑原版 → dlopen 后 dump 这 18 地址 72B 运行时值。

- **大白话卡点**：电话簿填好了 3 类重定位（5312/214/6），但还有 18 个"号"是
  "开机后才发"的（dlopen 重定位才填），静态文件里是占位符，读不出来。
- **为什么卡**：这 18 个不是 3 类重定位能填的（RELATIVE/JUMP_SLOT/GLOB_DAT 都填过了），
  是 `.text` 常量池里的"密文占位"，要 dlopen 时运行时才解析。
- **不解决会怎样**：`OnCreate` 内部 `ldr rX,[pc,#N]` 读不到 18 个值 → 依赖它们的逻辑全错。

## 为什么这样做：把 Linker 拆成"搬进→接电话→通水电"三步

类比搬一家公司进新楼：

1. **加载 = 搬进办公室**（把文件内容按图纸放进内存固定坐标）
2. **链接 = 接电话线**（把 214 个外部函数号填进电话簿，把 5312 个内部引用号换算成真实地址）
3. **初始化 = 通水电**（跑 12 个构造函数，把全局变量、单例对象初始化好）

**为什么拆成三步而不是一步到位？** 因为三步有严格依赖：搬进（定基址）→ 接电话
（重定位要用基址）→ 通水电（init 要用填好号的电话簿）。乱序 = 崩。

unidbg 把这三步全自动化了，但你要**知道它每步做了什么**，才能在卡住时判断是
"没搬进"（mmap 失败）、"电话没接上"（重定位漏）还是"水电没通"（init 没跑）
——军棋实测这三步**全通**（红线 §R6）。

**预判追问："为什么 unidbg 不直接用真 Bionic linker，要自己写简化版？"**
真 Bionic 是 C++ 写的系统级链接器，跑在真 Linux 内核上，依赖真内核的 mmap/mprotect。
unidbg 模拟的是"用户态"（so 代码本身），没有真内核，mmap 是 unidbg Java 端模拟的
（内存全在 Java 堆）。所以 unidbg 写了"简化 Linker"（`AndroidElfLoader`），
复刻 Bionic 的三阶段逻辑（加载→重定位→init），但用 Java 的 mmap mock 替代。
军棋 12/5312/214/6 全通 = 简化 Linker 做对了，但补不全所有运行时语义（§124 三项阻塞）。

## 6.1 so 的加载过程（搬进办公室，6 步）


> **本节核心**：
> 1. so 加载 6 步：mmap 两段 → 填 3 类重定位 → 跑 init_array → 建 module → 填 .got → 应答 JNI
> 2. 「搬进办公室」：段 mmap 到 base，坐标 + 权限按 PHDR
> 3. 加载 6 步全通 = 12/5312/214/6 军棋实测
>
> **本节难点**：
> 1. 加载是「搬进」，链接是「接电话」，初始化是「通水电」，三段缺一不可
> 2. 军棋 6 步全通但 OnCreate 仍卡 §124（加载 ≠ 调用）

![Bionic so 加载 6 步](assets/ch06_load_6step.png)



Bionic linker 加载一个 `.so` 的 6 步（军棋 `libjunqi.so` 实际走一遍）：

| 步 | 动作 | 大白话 | 军棋实际 |
|---|---|---|---|
| 1 | 打开文件 `open()` | 拿到钥匙开门 | `open("/data/app/.../libjunqi.so")` |
| 2 | 读 ELF 头 | 看房证（房型对不对） | `e_machine==EM_ARM(40)`、`e_phnum==9` |
| 3 | 计算基址 | 给房子定坐标 | `base=0x40000000`（对齐 0x1000） |
| 4 | mmap PT_LOAD | 按图纸把房间搬进去 | r-x 段 0x001000-0x155000 + rw- 段 0x155000-0x15a000 |
| 5 | 解析 PT_DYNAMIC | 读电话簿索引 | `DT_NEEDED`(7 依赖)、`DT_REL`、`DT_JMPREL`、`DT_PLTREL` |
| 6 | 生成 module 对象 | 登记这家公司档案 | `{base=0x40000000, so=/.../libjunqi.so, dynamic=...}` |

**为什么 r-x 和 rw- 两个段要分开 mmap？** 权限不同（代码段只读可执行、数据段可读写），
分开映射才能让"代码不许改、数据可写"。军棋 4.2 节图纸里这两个 PT_LOAD 就是按权限拆的。

## 6.2 so 的链接过程（接电话线，3 类重定位）
![3 类重定位对照](assets/ch06_reloc_3type.png)
重定位 = 把文件里的"占位符"换成运行时真实地址。军棋实测三类全通：

### 6.2.1 R_ARM_RELATIVE（相对重定位，军棋 5312 个）


> **本节核心**：
> 1. R_ARM_RELATIVE（军棋 5312 个）：相对偏移重定位，填 base + offset
>
> **本节难点**：
> 1. 漏 → 程序指针全错（所有相对地址算错）
> 2. 军棋 5312 全通（unicorn2 后端下）
> 3. RELATIVE 是最大类，算错影响面最广

```c
addr += base;  // 段内地址 = 文件里的偏移 + 运行时基址
// 大白话：so 里有 5312 处写着"我在文件第 N 字节"，
//        加载后要改成"在内存 0x40000000 + N 处"
```
军棋 5312 个 RELATIVE 槽全通（红线 §R6）。这是三类里**数量最大**的一类——
so 内部所有"指向自己"的引用（比如常量池、vtable 指针）都要做这个换算。
### 6.2.2 R_ARM_JUMP_SLOT（PLT 跳板重定位，军棋 214 个）


> **本节核心**：
> 1. R_ARM_JUMP_SLOT（军棋 214 个）：PLT 跳板重定位，填符号地址
>
> **本节难点**：
> 1. 漏 → PLT 跳板跳 0 → SIGSEGV
> 2. 军棋 214 全通
> 3. JUMP_SLOT 是「函数调用跳板」，so 调 libc 时先跳这里

```c
PLT[entry] → GOT[entry] → 符号地址
// 大白话：调 libc 函数走 214 个跳板，跳板指向电话簿，
//        电话簿里填的是 libc 真实地址
```
军棋 PLT→GOT→符号映射 214/214。214 个跳板 = so 调 libc/C++ 运行时的 214 个外部函数。
军棋 14 个 JNI 入口（`Java_com_cnvcs_App_On*`，readelf `--dyn-syms` 实测）：
OnCreate / OnDestroy / OnResult / OnResume / OnEvent / OnKey / OnUpdate /
OnGraphChange / OnBluetooth / OnPause / OnGraphCreate / OnTimer / OnInput / OnTouch。
14 个入口在 5312 个 RELATIVE + 214 个 PLT 填号全通后，才能按地址调（`&~1` 对齐 Thumb 奇地址）。
### 6.2.3 R_ARM_GLOB_DAT（全局数据重定位，军棋 6 个）


> **本节核心**：
> 1. R_ARM_GLOB_DAT（军棋 6 个）：全局数据重定位，填符号地址到 .got
>
> **本节难点**：
> 1. 漏 → __cxa_atexit 读零 .got 崩
> 2. 军棋 6 个从 docker 真 ARM libc 解析（静态 .so .got 槽为零）
> 3. GLOB_DAT 是「.got 填号」，军棋第一个崩点

```c
GOT[entry] = 符号解析地址
// 大白话：6 个全局变量（比如 libc 里的 stdout）的地址要填进电话簿
```
军棋 6 个 GLOB_DAT 符号从 docker 真 ARM libc 解析（红线 §R6）。
**为什么只 6 个还要单独列？** 它们不是函数（JUMP_SLOT 管函数），是全局变量地址，
解析源不同（libc 二进制里读，不是跳板填号），所以单独一类。
**"为什么 6 个 GLOB_DAT 单独解析"（大白话）**：
214 个 JUMP_SLOT 填的是"函数地址"（调 libc 函数），unidbg AndroidResolver 自带；
6 个 GLOB_DAT 填的是"全局变量地址"（如 `stdout`、`stderr`），是数据不是函数，
AndroidResolver 不直接给真地址，得从真 ARM libc 二进制里读出来。军棋 6 个已
实证解析成功（红线 §R6）——"从 docker 真 ARM libc 解析"就是"读真 C 库二进制
拿到变量地址再填"。
**"函数 vs 变量"为什么分开填（大白话）**：
- JUMP_SLOT（214）= 填"函数跳板"（调 `malloc`/`strlen` 走跳板，跳板填函数真地址）
- GLOB_DAT（6）= 填"变量地址"（读 `stdout`/全局表，填变量真地址）
两类填号公式不同（函数走 PLT、变量直填 GOT），所以 214 和 6 分开处理。
军棋 5312 RELATIVE + 214 JUMP_SLOT + 6 GLOB_DAT = 5532 处填号全通。

### 6.2.4 吸收自 4 本书（揉合去重）：重定位全谱系 + 延迟绑定

> **本节来源**：取《Linux 二进制分析》Ch2.5（重定位 + 动态链接最专，280 页），《二进制分析实战》Ch2.3.4 延迟绑定补充。详见 `ELF_FORMAT_SUPPLEMENT.md` 第四节。

**重定位类型全谱系**（6.2 只讲 3 类，补全谱系 + "为什么军棋 3 类够"）：

| 类型 | 含义 | 课程通用 | 军棋 |
|---|---|---|---|
| `R_ARM_NONE`(0) | 不重定位（占位/对齐） | ✅ | 有（段内对齐） |
| `R_ARM_32`(1) | 绝对地址（`[S+A]`） | ✅ | 有（`.data` 指向 `.rodata` 的绝对指针） |
| `R_ARM_PC32`(2) | PC 相对（`[S+A-PC]`） | ✅ | 有（`ldr rX,[pc,#N]` 常量池引用） |
| **`R_ARM_RELATIVE`(3)** | **相对重定位（加 load_base）** | ✅（**unidbg Linker 核心**） | **5312 个（全通）** |
| `R_ARM_ABS32`(4) | 绝对 32 位 | ✅ | 有 |
| **`R_ARM_GLOB_DAT`(204)** | **全局数据重定位（GOT 槽）** | ✅ | **6 个（全通）** |
| **`R_ARM_JUMP_SLOT`(207)** | **PLT 跳板（延迟绑定）** | ✅（**unidbg Linker 核心**） | **214 个（全通）** |
| `R_ARM_IGN`(255) | 忽略（占位） | ✅ | 有 |

**"延迟绑定"机制**（《二进制分析实战》2.3.4，6.2 没讲）：
```
首次调 libc.foo()：
  .plt[foo] → .got.plt[foo]（空）→ 跳 _dl_runtime_resolve（linker）
  → 查 .dynsym 解析 foo 真地址 → 填 .got.plt[foo]
  → 二次调 foo：直接 .got.plt[foo]（已填，不跳 linker）
```
**课程通用**：真机 dlopen 走"首次跳 linker 解析"（延迟绑定），unidbg 模拟时**全填**（AndroidResolver 直接解析符号填 `.got.plt`，不需要真"首次跳 linker"）。

**"军棋 3 类重定位为什么够"（大白话）**：
- 军棋 so 是**静态链接 `libstdc++` + 动态链接 libc/GL**（so 文件里已定好"哪些符号要动态解析"）
- `R_ARM_RELATIVE`（5312）= "所有要加 load_base 的相对指针"（`.got`/`.data` 内部指针）
- `R_ARM_JUMP_SLOT`（214）= "所有外部函数调用"（延迟绑定 PLT 跳板）
- `R_ARM_GLOB_DAT`（6）= "所有外部全局变量"
- `R_ARM_32`/`R_ARM_PC32`/`R_ARM_ABS32` 在军棋 `.so` 里**已重定位完**（so 文件里的值是真值，不靠 dlopen 再填）→ unidbg 不用管
- **18 个 `.text` 常量池不是重定位**（是 `ldr rX,[pc,#N]` 读的内嵌常量，跟重定位类型无关，只有 dlopen 才填值）

**军棋 `.got` 全零 = "门牌号没填"**（跟 ch04 4.3b 呼应）：静态 `.so` 未重定位，`.got` 槽全 0；unidbg 模拟时 AndroidResolver + docker 真 ARM libc 填 5532 处（5312+214+6）→ "填号完成"。
## 6.3 so 的初始化操作（通水电）


> **本节核心**：
> 1. init_array「通水电」：军棋 12 个 init_array 全通
>
> **本节难点**：
> 1. 通水电后 ≠ 能调函数：还差 18 常量 + DalvikVM + §124
> 2. init_array 跑 C++ 静态构造 + 全局初始化，军棋 12 个全跑
> 3. init[9..11] 触发 bx bug（§124h），最大安全集 = init[0..8]

军棋 so 的 init 阶段：
```
1. .preinit_array[0]   → 1 个构造函数   （最早通的水电，C++ 全局对象构造）
2. .init              → _init 函数      （系统自带的入口）
3. .init_array[0..11] → 12 个构造函数   （公司自己的 12 个初始化）
4. 返回到 dlopen 调用者
```
**顺序为什么是 preinit → init → init_array？** preinit 最靠前（最早的构造），
init 是系统入口，init_array 是 so 自己注册的一批构造函数。军棋 12 个 init_array
全通（unidbg/unicorn2 实测，红线 §R6）。漏跑任何一个，全局变量就是脏的。
**军棋 14 个 `On*` 入口的"入口地址"（readelf 实测，供对照）**：
`OnCreate` 0x233e1（Thumb 奇地址）等 14 个，全是"唯一没撕的门牌"
（strip 后 `.dynsym` 保留）。找入口 = 查这 14 个名字 → 拿地址 → `&~1` 对齐。
**"12 个 init_array 是什么"（大白话）**：
so 的全局变量、单例对象、C++ 运行时（`libstdc++` 初始化）要靠 12 个构造函数
初始化。比如军棋"棋盘对象"是个全局单例，它的构造函数就在 12 个 `init_array`
里。漏跑任何一个 → 全局变量脏 → `OnCreate` 读到脏值。军棋 12 个全跑完
（unidbg/unicorn2 实测，红线 §R6），全局变量才"干净"。
**"通水电"的 3 段为什么有顺序（大白话）**：
- `.preinit_array`（1 个）最靠前：C++ 全局对象的构造（最早的水电）
- `.init`（`_init`）：系统自带入口（水电总闸）
- `.init_array`（12 个）：so 自己注册的 12 个构造函数（各房间的水电）
先通"最早的水电"（preinit）→ 开"总闸"（init）→ 通"各房间"（init_array 12 个），
顺序不能乱（preinit 依赖的还没初始化，init_array 就要 init 先开总闸）。
军棋 12+1 全通 = "水电全通"，`OnCreate` 才能跑。
## 6.4 逐环节"能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **依赖库加载** | 7 个 `DT_NEEDED` 全解析（libc/libstdc++/libm/liblog/libdl + 2 GL 挂桩） | 加载 GL 真渲染（无真 GPU，挂桩返回 0） |
| **RELATIVE 重定位** | 5312 槽全通（`addr += base`） | 读 18 个 `.text` 常量（RELATIVE 只填内部自引用，常量池需 dlopen 重定位） |
| **PLT/GOT 重定位** | 214 PLT 槽 + 6 GLOB_DAT 全通（PLT→GOT→符号） | 填 `.got` 真值到静态文件（静态文件全 0，unidbg 只在内存里填） |
| **跑 init_array** | 12 init_array + 1 preinit 全通（通水电） | 跑通 `OnCreate` 业务逻辑（卡在 §124 ② C++ 对象图 0x43ed8） |
| **读 18 常量** | 早期 redroid dump 72B 注入（`jq_load_runtime_values`）；**★ §124x-z 后 = 静态位移固化 `jqt_mem.c` `g_jq_got_tbl`（免 dump）** | 从静态 `.so` 直接读（未重定位，读不出） |

**军棋实测对照**：
- 依赖库加载 ✓（7 个全解析，2 GL 挂桩）
- RELATIVE ✓（5312/5312 全通）
- PLT/GOT ✓（214/214 + 6/6 全通）
- init_array ✓（12+1 全跑完）
- **✗ 读 18 常量**：静态 `.so` 读不出（未重定位）；早期需 redroid dump 72B 注入，**★ §124x-z 后 = 静态固化 `jqt_mem.c` `g_jq_got_tbl` 免 dump**
  （`frida-dump/redroid_jni_dump.py` 三件套）

## 6.5 军棋 so 加载完整路径（实测全链路）


> **本节核心**：
> 1. 军棋加载全链路实测：mmap → 5312/214/6 → 12 init_array → module → .got → JNI
>
> **本节难点**：
> 1. 「能加载 ≠ 能调用」：12/5312/214/6 全通但 OnCreate 卡 §124
> 2. 全链路是「加载验证」，「调用验证」需 redroid + 真 GPU
> 3. 军棋加载路径是红线 §R6 实测数据，课程里反复引用

```
0. 真机 dlopen → Bionic linker 填好 .got（12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT）
1. 静态 .so 文件（未重定位）→ .got 槽为零    ← 你手里的文件是这个状态
2. unidbg/unicorn2 加载静态 .so：
   - 12 init_array 全通 ✓
   - 5312 RELATIVE 槽全通 ✓
   - 214 PLT→GOT→符号映射全通 ✓
   - 6 GLOB_DAT 符号从 docker 真 ARM libc 解析 ✓
3. JNIEnv/VM 补环境（DiagJni 实证）：
   - createDalvikVM + resolveClass + callStaticJniMethodObject ✓
   - OnCreate 推进到 std::vector 对象图 0x43ed8 ✓
4. 剩余阻塞（§124 纠正后 3 项）：
   - ① 上下文相关 BL 解码 bug（加载 .so 后 BL 0x7965a 被 unicorn2 解码成 0x6eaea，
     真实目标 0x43ed8，模拟器精度问题，非翻译/逻辑 bug）
   - ② C++ 对象图语义缺失（std::vector begin/end/capacity 未初始化）
   - ③ 288MB PROT_NONE mmap（unidbg mmap2 处理失败）
```
**怎么读这张全链路图？** 第 0-2 步是"能加载"（全通），第 3 步是"能接通 Java 电话"
（全通），第 4 步才是真正卡住的地方（3 项阻塞）。军棋"能加载"早已达成，
"能跑通 OnCreate"卡在 native 层 C++ 对象图，不在加载/链接阶段。
**军棋 so 不是壳**（三条证据）：
- `.text` 明文（熵 7.10 = 正常机器码，壳会加密成高熵）
- `.rodata` 明文（118KB，与 redroid 运行时 dump 逐字节相同，0/118832 不同，没有自解密例程）
- 唯一高熵点 0x138000 是**压缩/索引表**（值非有效地址），**非加密代码**
## 6.6 军棋实战验证：逐项核对 12/5312/214/6

军棋 unidbg 加载的逐项实测（红线 §R6 已验证可突破）：

| 项 | 数量 | 军棋实测 | 大白话 |
|---|---|---|---|
| `.init_array` | 12 | 全通 ✓ | 12 个构造函数跑完 |
| `RELATIVE` | 5312 | 全通 ✓ | 5312 处内部引用换算 |
| `PLT` | 214 | 全通 ✓ | 214 个外部函数跳板 |
| `GLOB_DAT` | 6 | 全通 ✓ | 6 个全局变量填号 |

**为什么这 4 个数是"军棋指纹"？** 它们是军棋 1MB so 的固定特征（重定位表条目数），
后续任何"加载对没对"的核对都用这 4 个数当基准。12/5312/214/6 全通 = 加载阶段无 bug。

## 6.7 本章小结（一句话版）

> 加载 6 步（搬进）→ 链接 3 类重定位（接电话：5312 RELATIVE + 214 PLT + 6 GLOB_DAT）
> → 初始化 12+1 个函数（通水电），军棋全通。
> 军棋 so 不是壳：`.rodata` 明文 118KB 已 dump，高熵点 0x138000 是压缩索引表。
> 卡在 3 项阻塞（§124 纠正后）：① 上下文相关 BL 解码 bug（unicorn2 精度，
> BL 0x7965a 错位到 0x6eaea）② C++ 对象图语义缺失（std::vector 0x43ed8）③ 288MB PROT_NONE mmap。

---

## 视频
