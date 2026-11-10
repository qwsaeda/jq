# 第4章 ELF 文件执行视图解析

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 `lib/armeabi/libjunqi.so`（ELF32 / ARM / EABI5 / stripped）

## 痛点：为什么需要读懂 ELF 文件？

在动手模拟军棋 so 之前，你得先"看懂它长什么样"。这一章的痛点全来自
"对着一个 **1.43MB/1435156B** 的二进制文件两眼一抹黑"。

### 痛点 1：拿到 `.so` 不知道它长什么样

军棋 `libjunqi.so`（`.so` = shared object，共享对象，可被多个程序复用的代码块）
是一个 **1.43MB/1435156B** 的 32 位 ARM 二进制。你想知道它有哪些段、依赖什么库、入口在哪——
不解析 ELF（Executable and Linkable Format，可执行与可链接格式，二进制文件的"骨架描述"）
头就两眼一抹黑。

- **大白话卡点**：你手里只有"一堆 0/1"，不知道"这套房子几室几厅、哪个房间住人"。
- **为什么卡**：ELF 头（前 52 字节）+ 程序头表 + 动态节区，三层结构藏着所有信息，
  不逐层读就全瞎。
- **不解决会怎样**：后续 unidbg 加载、重定位、调函数全无从下手——连"基址该定多少"都不知道。

### 痛点 2：不知道哪段能执行、哪段能写

`.text`（代码段）是 r-x 只读可执行，`.got`/`.data`（数据段）是 rw- 可读写。
模拟内存映射（mmap，把文件内容映射到虚拟地址空间）时给错权限，程序跑一半就崩。

- **大白话卡点**：代码区是"锁死的档案柜"（能看不能改），数据区是"工作台"（能读能写）。
  你把档案柜设成"可写"或把工作台设成"不可写"，搬进来第一秒就出事故。
- **为什么卡**：段权限（r-x / rw-）写在程序头表的 `p_flags` 里，模拟时不照抄就乱。
- **不解决会怎样**：跑指令时触发非法写保护 → SIGSEGV（段错误），模拟直接崩。

### 痛点 3：strip（删除符号信息）后"符号表空了"

军棋 so 删掉符号表后，你只能按地址调函数，但哪个地址对应 `OnCreate`？
必须先读懂 ELF 结构，定位 `.dynsym`（动态符号表，保留的导出符号）里的 14 个
`Java_com_cnvcs_App_On*` 入口。

- **大白话卡点**：一栋楼把所有门牌号都撕了（strip 删符号表），你想找"哪间房是机房"，
  只剩墙上贴的 14 张"对外公示"（`.dynsym` 导出的 14 个 `On*`）。
- **为什么卡**：军棋 so 是 stripped（剥离版），普通符号表 `.symtab` 被删，
  只剩动态符号表保留导出的 JNI 入口。
- **不解决会怎样**：14 个 JNI 入口（`OnCreate`/`OnDestroy`/…）的地址全靠手翻反汇编猜，
  调函数第一步就错。

### 痛点 4：`.got` 是零值，**房间真值**静态读不出（门牌号静态可读，N45）

**★ N45 精确化**：`.got` 槽**静态读得出门牌号**（`.so` 文件偏移如 `0x15xxxx`），**房间真值**（重定位后的真地址）只有 dlopen 才填对。类比 `open("/data/img/1.png")`：路径=门牌号（静态可读），像素=房间真值（open 后才拿到）。

军棋 `.got`（电话簿）在静态文件里全 0（未重定位）。你拿它当"运行时值"用，
跳地址 0 就崩。这是 unidbg 加载军棋的第一个实卡点（红线 §R6）。

- **大白话卡点**：你手里拿的是"空号电话簿"（全 0），拨号就拨到 0 → 段错误。
- **为什么卡**：`.got` 槽真值只有 dlopen 重定位才填（linker 填号），静态文件是 0。
- **不解决会怎样**：`__cxa_atexit` 等走 `.got` 的调用读 0 指针 → SIGSEGV（军棋实测崩点）。

## 为什么这样做：把 so 当"带家具清单的毛坯房"

一个 ELF 文件就像一套毛坯房加家具清单，三层结构各司其职：

- **ELF 头（前 52 字节）= 房证**：写明了户型（32 位/64 位）、朝向（大小端）、几室几厅（段数量）。
- **程序头表（PHDR）= 施工图纸**：告诉动态链接器"哪些房间要能住人（可执行）、
  哪些要通水电（可读写）"——这是"执行视图"，程序跑起来时真正看的。
- **节区表（SHDR）= 家具清单**：`.text` 是代码柜、`.rodata` 是只读资料架、
  `.got` 是电话簿（动态重定位要填号的地方）。这是"链接视图"，链接器和调试工具看的。

**为什么 unidbg 走"执行视图"（程序头表）而不是"链接视图"（节区表）？**
因为你模拟的是"把 so 搬进内存跑起来"（执行），不是"重新链接一次"（链接）。
真机 dlopen 走的就是程序头表，unidbg 照抄这条路径，才和真机行为一致。

类比：搬家时你按"施工图纸"（哪些房间通水电）把家具摆进新址，而不是按
"家具清单"（柜子叫什么名字）摆——清单是给你盘点用的，图纸才是搬家用。

**预判追问："为什么不能把节区表也读一遍？"**
节区表是"链接视图"（给链接器看的），程序头表是"执行视图"（给运行时看的）。
so 编译时两条路径都生成了，但执行时只看程序头表（真机 dlopen 也只看它）。
unidbg 模拟的是"执行"，所以只读程序头表，节区表只在你做"链接"（第 6 章）时才用。
军棋两条视图都完整，但执行模拟只用程序头表（`readelf -l` 看的那张）。

**预判追问："为什么读 ELF 要分三层（头/程序头/节区）而不是一把读完？"**
三层各有分工：头（房证）确认"这是什么文件"，程序头（图纸）告诉"哪些要搬进内存"，
节区（清单）告诉"每段叫什么名"。一次读三层 = 把房证、图纸、清单全过一遍，
少读一层就"不知道搬什么/搬到哪"。军棋 unidbg 按这个顺序逐层解析（4.4 节代码）。

## 4.1 ELF 文件结构（房证：前 52 字节）


> **本节核心**：
> 1. ELF 头前 52 字节：魔数 + 位宽 + 入口 + PHDR 偏移（军棋 ELF32/ARM/EABI5/stripped）
> 2. 军棋入口 0x233e1（Thumb 奇地址，需 &~1 对齐）
>
> **本节难点**：
> 1. ELF 头是「房证」：声明 so 的位宽/字节序/入口/段表位置
> 2. strip 后 .symtab 删，.dynsym 留 14 个 On*（动态符号表）
> 3. ELF 头位宽决定 for32Bit/for64Bit 构建选择（军棋 ELF32 → for32Bit）

| 偏移 | 字段 | 军棋 libjunqi.so 实际值 | 大白话解释 |
|---|---|---|---|
| 0x00 | `e_ident[16]` | `7f 45 4c 46 01 01 01 ...` | "ELF 魔数"：确认这是 ELF 文件；`01`=32 位，`01`=小端 |
| 0x14 | `e_type` | ET_DYN (3) | 共享对象（.so），不是可执行文件 |
| 0x16 | `e_machine` | EM_ARM (40) | 32 位 ARM 芯片 |
| 0x18 | `e_version` | 1 | ELF 格式版本 |
| 0x1c | `e_entry` | 0x?? (实际 dlopen 入口) | dlopen（动态加载 .so 的函数）后代码从哪跳 |
| 0x20 | `e_phoff` | 52 | 程序头表紧挨着头（偏移 52） |
| 0x28 | `e_phentsize` | 32 (32 位 ELF) | 每条程序头 32 字节 |
| 0x2a | `e_phnum` | 9 | 9 个程序头（段） |
| 0x2c | `e_shoff` | ... | 节区表在文件末尾 |
| 0x32 | `e_shentsize` | 40 | 每条节区头 40 字节 |
| 0x34 | `e_shnum` | ~30 | ~30 个节区 |
**怎么快速验证一个文件是不是军棋这种 32 位 ARM so？** 用 `readelf -h` 或 `file` 看
`e_machine`。军棋 `libjunqi.so` 读出来是 `EM_ARM (40)` + `EI_CLASS=1`（32 位）+
`EI_DATA=1`（小端），跟 ELF 头逐字段对上。
军棋 `.text` 段是明文（熵 7.10 = 正常机器码，不是加密的），`.rodata`（只读数据段）
段明文（118KB）。这就是"so 不是壳"的第一条证据：代码段熵值正常，没有加密伪装。
## 4.2 程序头部表（PHDR，施工图纸：给链接器的"内存摆法"）


> **本节核心**：
> 1. PHDR 给链接器的「内存摆法」：PT_LOAD 段（r-x + rw-）+ 权限 + 文件偏移 + 内存地址
> 2. 军棋 r-x（代码）+ rw-（数据）两段 mmap，权限摆错全乱
>
> **本节难点**：
> 1. PHDR 是「施工图纸」：链接器按它把段摆到内存坐标
> 2. PT_LOAD 段权限决定能否执行/写：r-x 段写崩，rw- 段执行崩
> 3. 军棋 base=0x40000000 是 ASLR 基准，段坐标 = base + PHDR 偏移

![PT_LOAD / PT_DYNAMIC 段布局](assets/ch04_pt_load.png)



32 位 ELF 的 `PT_LOAD`（要映射进内存的段）/ `PT_DYNAMIC`（动态链接信息）/
`PT_HASH`（符号哈希）/ `PT_GNU_RELRO`（重定位后锁死只读，防篡改）：

| 段类型 | 军棋实际 | 说明 | 大白话解释 |
|---|---|---|---|
| PT_LOAD (r-x) | 0x001000 | `.text` + `.rodata` 合并段 | 代码+只读数据，搬进内存后不许改 |
| PT_LOAD (rw-) | 0x155000 | `.data` + `.bss` | 可读写区，全局变量住这里 |
| PT_DYNAMIC | 0x155000 | `.dynamic`（GOT 入口） | 动态链接器查依赖库、做重定位的"索引目录" |
| PT_GNU_RELRO | 0x155000-0x155xxx | 重定位保护 | 重定位做完后锁死，别人改不了 |

**军棋 `.got`（Global Offset Table，全局偏移表 = 程序里的"电话簿"）槽在静态文件里是零**
（未重定位，linker 没来填号），dlopen 后才填（红线 §R6）。这就是军棋在 unidbg 里的
第一个卡点：你手里的是"没填号的电话簿"。读 `.got` 里的零值 → 跳地址 0 → SIGSEGV。

**为什么要关心 PT_GNU_RELRO？** 它决定重定位完成后哪些区域被 `mprotect` 锁成只读。
unidbg 模拟时要照抄这个"锁死范围"，否则锁早了（把还没填号的 `.got` 锁死）或锁晚了
（允许了不该改的区域），行为就和真机对不上。

**预判追问："为什么 RELRO 要锁死？不锁不行吗？"**
RELRO（"Relocatable Read-Only"）是安全机制：重定位做完后，`.got` 区域被 `mprotect`
锁成只读，防止后续攻击者篡改"电话簿"里的地址（跳板被改掉 → 攻击者控制程序流向）。
不锁 = 电话簿随时可被改 → 安全风险。unidbg 模拟时照抄这个"锁死范围"，
让"重定位后 `.got` 不可写"和真机行为一致（军棋 4.2 图纸里 0x155000-0x155xxx 就是锁区）。

## 4.3 动态节区 _DYNAMIC 段（依赖清单）


> **本节核心**：
> 1. _DYNAMIC 段 = 依赖清单（DT_NEEDED 7 个含 liblog）
> 2. 军棋 7 依赖：libc/libstdc++/libm/liblog/libdl/libGLESv1_CM/libGLESv2
>
> **本节难点**：
> 1. _DYNAMIC 是「依赖清单」：链接器按它递归加载依赖库
> 2. 7 依赖递归：libc → libm → libdl → libstdc++ → liblog + 2 GL
> 3. DT_NEEDED 漏一个 → PLT 跳板找不到目标 → SIGSEGV

`DT_NEEDED`（依赖库列表）军棋 so 实际 7 个：
```
DT_NEEDED  libc.so         ← C 标准库（malloc/strlen/printf…）
DT_NEEDED  libstdc++.so    ← C++ 运行时（std::vector 等容器）
DT_NEEDED  libm.so         ← 数学函数
DT_NEEDED  libdl.so        ← dlopen/dlsym（动态加载别的 .so）
DT_NEEDED  liblog.so       ← Android 日志（__android_log_print）
DT_NEEDED  libGLESv1_CM.so ← 真机 GL1 渲染（军棋用 OpenGL ES 1.0 画棋盘）
DT_NEEDED  libGLESv2.so    ← 备用
```
**为什么 7 个里两个 GL 库是"麻烦户"？** `libGLESv1_CM`/`libGLESv2` 是真机 GPU 才有的
图形库。你的 x86 开发机没 GPU，这两个库"挂了桩"（返回 0 的假函数）才能过加载。
前 5 个（libc/libstdc++/libm/libdl/liblog）unidbg 的 AndroidResolver 自带。
`DT_SYMTAB`（符号表）/ `DT_STRTAB`（字符串表）：军棋 strip 后**符号表近乎为空**
（`.dynsym` 仅保留 14 个导出符号 `On*`，函数名之外的都删了）。
`DT_PLTREL` / `DT_JMPREL`：PLT（Procedure Linkage Table，过程链接表 = "跳板"区）重定位表。
`DT_DEBUG`：0（非调试构建）。
**预判追问："strip 删了符号表，为什么 `.dynsym` 还留着 14 个？"**
strip 删的是"普通符号表 `.symtab`"（给调试器/GDB 用的，含所有函数/变量名），
但 `.dynsym`（动态符号表）是"导出符号"——so 要对外导出的 JNI 入口（14 个 `On*`）
必须保留在 `.dynsym` 里（否则 dlopen 后 Java 侧按名字找不到这些 native 方法）。
军棋 strip 只删 `.symtab`（内部函数名全没了），`.dynsym` 保留 14 个导出符号。
这就是"门牌号撕了（.symtab），但 14 张对外公示（.dynsym）还在"。

## 4.3b 吸收自 4 本书（揉合去重）：ELF 全字段 + 花指令 vs 自定义段 + libbfd

> **本节来源**：4 本书揉合去重（按知识点取"哪本最全"，重叠的不重复吸收）。详见 `ELF_FORMAT_SUPPLEMENT.md` + `REFERENCES.md` 4 本书评估。

### ① ELF 全字段（取《二进制分析实战》Ch2 最全，3 本重叠）

4.1/4.2 讲了 ELF 头 + 军棋 9 个 PT_LOAD，**但缺全字段**。《二进制分析实战》第 2 章（682 页教材，ELF 讲最全）补全：

**节全谱系**（`SHDR`，4.2 只讲 PT_LOAD，补 14 种节）：
| 节 | 含义 | 课程通用 | 军棋 |
|---|---|---|---|
| `.text` | 代码（r-x） | ✅ | **1.43MB/1435156B** 明文（熵 7.10 正常机器码，非壳） |
| `.rodata` | 只读数据（r--） | ✅ | 118KB 明文（静态=运行时 0/118832 不同） |
| `.data` | 已初始化可写（rw-） | ✅ | 28288B（`jq_rel_data.h` 拷贝） |
| `.bss` | 未初始化可写（rw-，文件 0 字节） | ✅ | 0x15be80 起（影子内存清零） |
| `.plt`/`.got`/`.got.plt` | 跳板/全局偏移表（**延迟绑定**） | ✅ | 214 PLT + 6 GLOB_DAT |
| `.init_array`/`.fini_array` | 构造/析构函数 | ✅ | 12 init_array（全通） |
| `.dynamic` | 动态链接信息（DT_NEEDED/DT_RELATAG） | ✅ | 7 依赖 + 5312 RELATIVE |
| `.symtab`/`.strtab` | 符号表 + 字符串表（**strip 删**） | ✅ | 军棋 strip（`.symtab` 删，`.dynsym` 留 14） |
| `.dynsym`/`.dynstr` | 动态符号表 + 动态字符串表（**strip 不删**） | ✅ | 军棋 14 个 `On*` 在这 |
| **`.note`/`PT_NOTE`** | **自定义段（`.note.gnu.gold-version` 等）** | ✅ | 军棋有（全标准，非混淆） |

**`.got` 为何 rw- 不 r--（大白话，4.2 已讲此处补全）**：
- `.got` = "外部函数/全局变量的门牌号"
- **加载时**（dlopen 重定位）：linker 要**写** `.got`（填 libc/GL 函数地址）→ 必须 `rw-`
- **重定位完成后**：`PT_GNU_RELRO` 段把 `.got` 改 `r--`（锁，防 GOT 覆盖攻击）
- **军棋 `.got` 全零**（未重定位）= "门牌号没填"，linker 要填但 unidbg 跑不到（§124 卡点）

### ② 花指令 vs 自定义段（取《二进制分析实战》Ch8，区分两层）

**课程通用**：跑混淆/加壳 so 要区分"指令层混淆"和"ELF 格式层自定义段"（处理手段不同）。
**军棋靶机**：军棋 so 明文非壳，**没有花指令/自定义混淆段**，但课程该讲清区分（原书样本有壳）。

| 维度 | 花指令（OBSO） | 自定义段（ELF 格式层） |
|---|---|---|
| **所在层** | **指令层**（`.text` 里塞无意义指令/不透明分支） | **ELF 格式层**（`PHDR`/`SHDR` 的段，如 `PT_GNU_RELRO`/`PT_NOTE`） |
| **目的** | 反汇编器乱（让 IDA/Ghidra 解不出正确基本块） | 段属性/权限/重定位（OS 加载器用） |
| **处理手段** | **递归反汇编 + 控制流分析**（Capstone 递归跟控制流，处理假分支） | **读 PHDR/SHDR**（readelf 看段属性，跟指令无关） |
| **军棋现状** | ❌ 无（军棋 `.text` 熵 7.10 正常，反汇编直接可做） | ✅ 有（9 个 PT_LOAD + `PT_DYNAMIC`/`PT_INTERP`/`PT_PHDR`/`PT_NOTE`，全标准） |

**"不透明分支"（《二进制分析实战》第 8 章脚注 3）**：混淆器用"始终为真/始终为假"的判定代码构造分支，
让静态反汇编器误判基本块边界——**这是花指令的高级形式，也不是自定义段**。
**课程通用价值**：跑壳/混淆 so 时，Capstone 递归反汇编 + 控制流分析（第 8 章 6.4.2）处理不透明分支；军棋非壳，跳过。

**区分方法**：`readelf -l`（看 PHDR 段乱 = 自定义段）vs `objdump -d`（看指令乱 = 花指令）。

### ③ libbfd 自写 so 加载器（取《二进制分析实战》Ch4，1 本）

**课程通用**：unidbg 简化 Linker 的底层 = "自写 so 加载器"（读 ELF 段 + 建符号表 + 建节表），`libbfd` 是 GNU 的 ELF 加载库（等价于自写加载器）。
**军棋靶机**：军棋 unidbg 简化 Linker（12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT）= libbfd 加载器的简化版。

| libbfd 概念（《二进制分析实战》4.2） | 含义 | 军棋 unidbg 对应 |
|---|---|---|
| `Binary` 类（4.2.1） | 解析 ELF 头 + 段 | unidbg `jelf`（4.4 讲，解析 ELF 头 + 程序头表） |
| `Section` 类（4.2.2） | 节表（`.text`/`.data`/`.got`...） | unidbg Linker mmap PT_LOAD 段 |
| `Symbol` 类（4.2.3） | 符号表（`.symtab`/`.dynsym`） | unidbg 简化 Linker 解析 `.dynsym`（军棋 14 JNI 入口） |

**课程通用价值**：unidbg 简化 Linker = "自写 so 加载器"（不依赖 OS linker，自己读 ELF + mmap + 重定位 + init_array）。`libbfd` 是 GNU 的参考实现，4.4 的 jelf 解析等价于 libbfd 的 `load()`（4.3.1-4.3.4）。

## 4.4 jelf 代码解析（unidbg 版）


> **本节核心**：
> 1. jelf = unidbg ELF 解析器（对应 Bionic elfparse.c）
> 2. 军棋 12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT 全通（红线 §R6）
>
> **本节难点**：
> 1. jelf 解析 ELF 头 + PHDR + _DYNAMIC，生成 module 对象
> 2. 军棋 12 init_array 是「通水电」，全通但 ≠ 能调函数（还差 18 常量 + DalvikVM）

unidbg 的 jelf 模块（`com.github.unidbg.elf`）解析顺序，类比"验房师按房证→图纸→家具清单过一遍"：
```java
// jelf 解析军棋 so 的 5 步（验房师流程）
// 1. 读 ELF 头 → 确认房型（32 位 ARM）
ElfHeader header = ElfFile.readFile(so).getHeader();
if (header.e_machine != EM_ARM) throw ...;        // 房型不对直接拒
// 2. 遍历程序头表，按 p_flags 分 r-x / rw-（按图纸分房间属性）
for (ProgramHeader ph : header.programHeaders) {
    if (ph.p_type == PT_LOAD) mapSegment(ph);      // 该搬的房间搬进去
}
// 3. 读 Dynamic[] 段，提取 DT_NEEDED/DT_RELRO/DT_PLTREL/DT_JMPREL（查家具清单）
for (Dynamic dyn : header.dynamics) {
    if (dyn.d_tag == DT_NEEDED) addNeeded(...);    // 记下依赖的 7 个库
}
// 4. 计算基址 0x40000000（给"房子"定固定坐标，对齐 0x1000）
long base = 0x40000000L;
// 5. mmap 各 PT_LOAD 段到虚拟内存（把图纸上的房间真搭出来）
memory.mmap(base + ph.p_offset, ph.p_memsz, ph.p_flags);
```
**为什么基址定 0x40000000？** 这是 unidbg 给 32 位 ARM 默认的加载基址，且**真机 Bionic
linker 32 位 ARM 默认也在这附近**（30.2 章会再讲）。基址定错，所有"基址+偏移"的相对地址
全指到错误位置，重定位一步错步步错。
**为什么"能加载"是后面一切的前提（大白话）**：军棋 14 个 JNI 入口要"按地址调"，
地址是"基址 + 文件偏移"算的。基址定 0x40000000，文件偏移 0x233e1，
运行时地址 = 0x40000000 + 0x233e1。基址定错（比如 0x4000000），
OnCreate 调到 0x4000000+0x233e1（错位置）→ 段错误。
**"基址 + 偏移"换算示例（大白话）**：
- `OnCreate` 文件偏移 0x233e1，基址 0x40000000 → 运行时地址 0x40233e1
- `.got` 文件偏移 0x155000，基址 0x40000000 → 运行时地址 0x4155000
- `.rodata` 文件偏移 0x120bf8，基址 0x40000000 → 运行时地址 0x4120bf8
所有"文件偏移 + 基址 = 运行时地址"的换算都靠基址定对。军棋 5312 个 RELATIVE
重定位就是"5312 处文件偏移 + 0x40000000"的换算。
## 4.5 军棋 so 的 ELF 段地图（家具清单全貌）


> **本节核心**：
> 1. 军棋段地图：.text（明文熵 7.10）+ .rodata（明文 118832B）+ .got（零值）+ .data（含 2 运行时常量）
> 2. .text 明文 = 非壳；.rodata 静态=运行时 0/118832 不同 = 无自解密
>
> **本节难点**：
> 1. 段地图是「家具清单全貌」：哪段能执行、哪段能写、哪段零值
> 2. .got 零值是第一个崩点：__cxa_atexit 读零 → SIGSEGV
> 3. .rodata 唯一高熵点 0x138000（熵 7.87）是压缩索引表非加密，别误判成壳

![军棋 so ELF 段地图](assets/ch04_elf_sections.png)



```
0x001000  .interp         (动态链接器路径)         ← 告诉系统用哪个链接器
0x002000  .hash           (GOT hash)              ← 符号快速查找的哈希表
0x003000  .dynsym        (导出符号，strip 后仅 14 个 On*)   ← 唯一留的"门牌号"
0x003xxx  .dynstr        (字符串表)                ← 符号名的字符串池
0x004000  .rel.plt       (PLT 重定位表，214 槽)    ← 214 个跳板要填号
0x010000  .text          (明文 ARM/Thumb 代码，熵 7.10)
0x120bf8  .rodata        (明文，118832B；0x138xxx 是高熵索引表，非加密)
0x155000  .got          (零值槽，dlopen 填)         ← 电话簿，静态时全是空号
0x155xxx  .data.rel.ro   (只读全局变量)
0x156000  .dynamic       (依赖/重定位索引)
0x157000  .data          (可读写全局)
0x158000  .bss           (未初始化全局，占位不占文件)
0x15a000  .init_array    (12 个)                  ← 加载时要跑的 12 个初始化函数
0x15a0xx  .preinit_array (1 个)                   ← 更早跑的 1 个预初始化
```

**这张地图是后面所有章的"底图"**：第 6 章重定位填 `.got`/`.rel.plt`，第 7 章 mmap 这些段，
第 14 章按这张图摆内存，第 15 章在 `.got` 卡口设 Hook。先看全貌再分头打。

## 4.6 逐环节"能干什么 / 不能干什么"


> **本节核心**：
> 1. 能/不能：静态读 ELF 头/PHDR/_DYNAMIC/.text/.rodata；不能静态读 .got **房间真值**（门牌号静态可读，N45）
>
> **本节难点**：
> 1. 动态读（unidbg）跑不到 0x43ed8 → .got/.text 常量**房间真值**也读不出（N45）
> 2. 静态 + 动态两方向**房间真值**都断 → 18 常量早期 redroid dump（**★ §124x-z 后 = 静态固化 `jqt_mem.c` 免 dump**）
> 3. objdump/readelf 可核对段地图，但「房间真值读不出」和「段地图」是两回事（N45）

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **读 ELF 头** | 确认 32 位 ARM（`EM_ARM=40`、`EI_CLASS=1`）、9 个段、~30 个节区 | 确认 64 位 so（军棋是 ELF32，要读 64 位 so 得换 readelf 路径） |
| **读程序头表** | 定位 2 个 PT_LOAD（r-x 0x001000 / rw- 0x155000）、PT_DYNAMIC、PT_GNU_RELRO | 从程序头直接读 `.got` 真值（程序头只说"这段要映射"，不填值） |
| **读动态节区** | 定位 7 个 `DT_NEEDED`（libc/libstdc++/libm/liblog/libdl/GL1/GL2）、214 个 PLT 重定位 | 从 `.dynsym` 查内部函数名（strip 后只剩 14 个 `On*` 导出符号） |
| **读段地图** | 定位 14 入口（`.dynsym`）、7 依赖、`.got` 零值槽、12 init_array | 从 `.got` 直接读运行时值（未重定位，静态文件全 0，需 dlopen 填） |
**军棋实测对照**：
- 读 ELF 头 ✓（`readelf -h` 输出 `EM_ARM=40`、`e_phnum=9`）
- 读程序头 ✓（`readelf -l` 输出 2 PT_LOAD + PT_DYNAMIC + PT_GNU_RELRO）
- 读动态节区 ✓（`readelf -d` 输出 7 `DT_NEEDED`）
- 读段地图 ✓（4.5 节地图逐条核对 `objdump -h`；14 个 JNI 入口（`Java_com_cnvcs_App_On*`，
  readelf `--dyn-syms` 实测）：OnCreate / OnDestroy / OnResult / OnResume / OnEvent /
  OnKey / OnUpdate / OnGraphChange / OnBluetooth / OnPause / OnGraphCreate / OnTimer /
  OnInput / OnTouch）
- **✗ 从 `.got` 读运行时值**：静态 `.got` 全 0（未重定位），军棋 18 个 `.text` 常量池
  值只有 dlopen 重定位才填（需 redroid dump，见第 8 章 18 地址 72B）
## 4.7 军棋实战验证：用 objdump/readelf 核对这张地图


> **本节核心**：
> 1. objdump/readelf 核对段地图：12 init_array / 5312 RELATIVE / 214 PLT / 6 GLOB_DAT
> 2. strip 后 .dynsym 只剩 14 个 On*，内部函数无名字
>
> **本节难点**：
> 1. readelf -l 看 PHDR 段表，readelf -d 看 _DYNAMIC 7 依赖
> 2. 段地图核对是「能加载」前置，「能调用」还差 §124

军棋 so 不是壳，所以静态文件里就能直接读这张地图（红线 §R6 已验证）：
```bash
# 1. 看 ELF 头（房证）
readelf -h lib/armeabi/libjunqi.so | egrep 'Class|Machine|Type'
#   Class: ELF32   Machine: ARM   Type: DYN(Shared object)
# 2. 看程序头（施工图纸，哪段 r-x 哪段 rw-）
readelf -l lib/armeabi/libjunqi.so | egrep 'LOAD|DYNAMIC|RELRO'
# 3. 看依赖库（家具清单，7 个 DT_NEEDED）
readelf -d lib/armeabi/libjunqi.so | grep NEEDED
#   NEEDED libc.so / libstdc++.so / libm.so / libdl.so / liblog.so / libGLESv1_CM.so / libGLESv2.so
# 4. 看导出符号（唯一留的门牌号，14 个 On*）
readelf --dyn-syms lib/armeabi/libjunqi.so | grep 'Java_com_cnvcs'
#   Java_com_cnvcs_App_OnCreate / OnDestroy / OnResult / ... 共 14 个
```
实测核对结果（与 4.5 地图逐条对上）：
- 7 个依赖库 = 4.3 节的 7 个 `DT_NEEDED`，一个不多一个不少。
- 14 个 `On*` 导出 = 4.1 节说的 strip 后唯一保留的"对外公示"。
- `.got` 槽在静态文件里读出来**全是 0**（未重定位），印证"你手里是没填号的电话簿"。
## 4.8 本章小结（一句话版）

> ELF 头（房证）+ 程序头表（施工图纸）+ 动态节区（家具清单）是 unidbg 加载 so 的三根支柱。
> 军棋的两大坑：`.got` 没重定位（电话簿全是空号）+ 依赖 `libGLESv1_CM`（PC 没 GPU 渲染）。
> 军棋 `.rodata` 是明文（118KB 已 dump），`0x138xxx` 高熵是压缩索引表，**不是加密代码**。
> 军棋 so 不是壳：静态 `readelf`/`objdump` 直接读得出 7 依赖 + 14 入口 + 段地图，与运行时一致。

---

## 视频
