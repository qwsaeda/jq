# 附录 B：ELF 是怎么打包出来的（C/C++/Rust 三个 so 真布局对比，从"正向生成"讲"逆向"）

> **定位**：unidbg 课程是"逆向"课（读 so），但**缺"so 是怎么来的"（正向生成）的知识**。
> 不懂"编译器/链接器怎么生成 so + 重定位怎么填 `.got`"，就理解不了"为什么 unidbg 要模拟重定位"、"为什么 `.got` 静态是占位"。
> 本附录用**真编译的 3 个最小 so**（C/C++/Rust）做**真实对比**（`readelf` 实读，非凭空讲），
> 从"正向生成"讲"逆向 so"。跟 `ELF_FORMAT_SUPPLEMENT.md`（4 本书揉合的 ELF 全字段）互补：
> 那份讲"ELF 段属性/重定位全谱系"（4 本书知识），本附录讲"so 怎么一步步打包出来 + 3 种语言差异"（正向流程）。
>
> **3 个真实 so**（`/tmp/opencode/` 真编译，`readelf` 实读，64 位 x86_64）：
> - `so_c.so`（C：1 个全局变量 + 2 个导出函数，最简）
> - `so_cpp.so`（C++：`std::string` 静态对象 + 类，多 C++ 特性）
> - `so_rust.so`（Rust：`rustc 1.99.1` 真编译 `--crate-type cdylib`，4.5MB，std 静态链）

---

## B.0 先讲地基：so / ELF 的段布局（不懂这个，后面全悬空）

**一个 `.so` 文件 = 一组"段（section）"拼起来的**。编译器/链接器把源码编译链接后，**按"数据的属性"分到不同段**——
每个段有"名字 + 权限 + 放什么"。**逆向读 so，第一步就是"认段"**（`readelf -S` 列段表）。

### B.0.1 段分 4 类（按"放什么 + 什么权限"）

| 类 | 段（真实 so 里看到的） | 权限 | 放什么 | 三个 so 实测 |
|---|---|---|---|---|
| **代码段** | `.text`（+ `.init`/`.fini` 入口） | `R E`（可读+可执行） | **所有函数的机器码**（`c_add`/`rust_add`/军棋 `OnCreate` 都在 `.text`） | C/C++/Rust 全有 |
| **只读数据** | `.rodata`（+ `.data.rel.ro` 重定位后锁只读） | `R` | **字符串常量 + 数字常量 + C++ 的 vtable/类型信息** | C so 没 `.rodata`（常量少，塞 `.data`）；C++/Rust 有 |
| **读写数据** | `.data`（已初始化全局）+ `.bss`（未初始化全局，文件里 0 字节，运行时才分配） | `RW` | **已初始化的全局变量 + 未初始化全局（`.bss`）** | 三个全有 |
| **动态链接** | `.got`/`.got.plt`（门牌占位）+ `.plt`（跳板）+ `.dynamic`（依赖清单）+ `.rela.dyn`/`.rela.plt`（重定位表）+ `.dynsym`/`.dynstr`（符号） | `RW` | **"外部符号门牌 + 跳板 + 重定位表 + 导出符号"** | C/C++/Rust/军棋全有 |

**记一段心法**：**代码进 `.text`，常量进 `.rodata`，已初始化全局进 `.data`，未初始化全局进 `.bss`，"调外部函数"的门牌进 `.got`，门牌要填谁的重定位进 `.rela.*`，对外导出的符号进 `.dynsym`**。**记一段心法**：**代码进 `.text`，常量进 `.rodata`，已初始化全局进 `.data`，未初始化全局进 `.bss`，"调外部函数"的门牌进 `.got`，门牌要填谁的重定位进 `.rela.*`，对外导出的符号进 `.dynsym`**。**记一段心法**：**代码进 `.text`，常量进 `.rodata`，已初始化全局进 `.data`，未初始化全局进 `.bss`，"调外部函数"的门牌进 `.got`，门牌要填谁的重定位进 `.rela.*`，对外导出的符号进 `.dynsym`**。**记一段心法**：**代码进 `.text`，常量进 `.rodata`，已初始化全局进 `.data`，未初始化全局进 `.bss`，"调外部函数"的门牌进 `.got`，门牌要填谁的重定位进 `.rela.*`，对外导出的符号进 `.dynsym`**。

**★ 追问 3 连（用户："函数指令只能放 .text 吗 / 能自定义段吗 / 要自己写加载器吗"）**：

**Q1：函数指令默认放 `.text`，能放其它段吗？→ 能（代码放任何 `R E` 段）**
- "代码"放任何 `R E`（可执行）权限的段，**`.text` 是主体代码段，但不是唯一**
- 实测：军棋 so 的 **`.plt` 段也是代码（`AX`，PLT 跳板机器码）**，不在 `.text`；C so 的 **`.init`/`.fini`（胶水代码）也不在 `.text`**（B.0.2 讲过）
- 所以"函数指令"分布：`.text`（主体）+ `.plt`（跳板）+ `.init`/`.fini`（入口胶水）

**Q2：可以自定义段吗？→ 能（2 种）**
- **① 编译器自动生成的特殊段**（你不用管，链接器按架构生成）：军棋 so 实测有 **`.ARM.extab`/`.ARM.exidx`**（ARM 异常展开表，类似 x86 `.eh_frame`）+ **`.hash`**（ARM 旧式符号哈希，B.2 讲 MuMu 只支持它不支持 `.gnu.hash`）+ `.ARM.attributes`——**全是"非 `.text`/`.data` 的自定义段"（编译器生成的）**
- **② 你手写的自定义段**（GCC `section` 属性 + 链接脚本）：
  ```c
  __attribute__((section(".hot"))) int hot_func() {...}  // 函数机器码进 ".hot" 段
  __attribute__((section(".cfg"))) int tbl[] = {...};    // 数据进 ".cfg" 段
  ```
  嵌入式/Android 加固壳常用（放加密数据/自定义逻辑段，4 本书"自定义段 + libbfd 自写加载器"就是干这个，ch07 7.2b）
- 军棋 so 本身没用 ②（`.text`/`.rodata`/`.data`/`.got` 全标准段），但 **① 的 ARM 特殊段（`.ARM.extab`/`.hash`）就是"非标准段"实例**

**Q3：需要自己的程序做加载器吗？→ 分 2 层**

| 你做的事 | 要不要自己写加载器 | 谁加载 |
|---|---|---|
| **自定义 `R E`/`R W` 段（标准 ELF 机制）** | **不用** | **内核 linker（`dlopen` 按 `PT_LOAD` 程序段自动 `mmap` + 重定位）**。你只管 `section` 属性 + 链接脚本，内核帮你加载 |
| **绕过内核 linker（壳/加固/JIT/模拟）** | **要** | **自己写加载器**（读你的段 + 手动重定位 + 手动跑构造器，绕过内核） |

- **军棋原版 so = 标准 ELF**（`.text`/`.rodata`/`.data`/`.got`/`.ARM.extab` 全内核认）→ **不用自己写加载器**（内核 `dlopen` 直接加载）
- **unidbg/军棋翻译体 = "自己写加载器"**（模拟内核 linker：手动 `mmap` 段 + 手动重定位 `.got` 5539 条 + 手动跑 `.init_array`，**绕过真内核**）→ 这就是"自己的程序做加载器"（ch07 7.2 unidbg 简化 Linker + 军棋翻译体 `jqt_mem.c` 影子运行时 + 4 本书 libbfd 自写加载器）
- **为什么模拟/翻译体要"自己写加载器"**：没真内核（unidbg 用户态模拟 / 翻译体影子内存不是真 `mmap`），内核 linker 那套（`mmap`+重定位+跑构造器）不存在，**只能自己模拟**

**核心（"段" vs "程序段" 的区分）**：
- **段（section，so 文件里的，能自定义）vs 程序段（`PT_LOAD`，运行时内核按这个 `mmap`）**
- 自定义段 → 链接器合并进 `PT_LOAD` → **内核自动加载（不用自己写加载器）**
- **只有"不用内核 linker"（模拟/JIT/壳）才要自己写加载器**

**★ 5 个存储区（用户追问"全局放堆？常量不变？全局跟堆一样？"，精确区分）**：

| 存储区 | 段/位置 | 放什么 | 可改？ | 生命周期 | 怎么分配 |
|---|---|---|---|---|---|
| **栈** | 运行时栈区 | 局部变量、函数参数 | ✅ | **函数内（临时，return 就销毁）** | 自动（进函数分配，出函数销毁） |
| **堆** | 运行时堆区 | `malloc`/`new` 的对象 | ✅ | **malloc→free（对象级，可中途 free）** | 手动（`malloc`/`new` + `free`/`delete`） |
| **全局已初始化** | **`.data` 段**（静态区，**不是堆**） | `g_counter`/`g_val`（有初值） | ✅ | **进程全程（不可 free，跟程序同生共死）** | 编译期定地址，启动 mmap |
| **全局未初始化** | **`.bss` 段**（静态区，**不是堆**） | `int g_x;`（无初值） | ✅ | **进程全程（不可 free）** | 同上（文件 0 字节占位） |
| **常量** | **`.rodata` 段**（只读） | `"x"` 字面量、数字常量 | ❌ **只读** | **进程全程（不可改）** | 编译期定，启动 mmap |

**纠正 3 个常见混淆**（用户话"全局放堆 / 常量不变 / 全局跟堆一样"）：
1. **"全局放堆" ❌**：全局/静态变量放 **`.data`/`.bss` 段（静态区）**，**不是堆**——`.data`/`.bss` 是 ELF 段（编译期定地址，进程启动 mmap 就位），**堆是运行时 `malloc`（动态区）**。`g_counter`@0x400c 在 `.data`（实测），不在堆。
2. **"常量区不变" ✅ 但只对 `.rodata`**：字符串/数字常量在 `.rodata`（**只读，进程全程不变**）；**但全局变量（`.data`/`.bss`）不是常量**（可读写，`g_counter++` 就改了它）。
3. **"全局和堆一样" ❌（生命周期不同）**：**堆能中途 `free`（对象级，程序员控制），全局不能 `free`（进程级，跟程序同生共死）**——这是两者本质区别。全局生命周期比堆**更长、更固定**。
4. **"不像栈临时" ✅**：栈（局部变量）`return` 就销毁（临时）；堆/全局都不临时（堆到 free，全局到进程退出）。


### B.0.2 三个真实 so 的段表实读（`readelf -S`，地基）

**C so（`so_c.so`，15184B，最简）** 段表：
```
[ 7] .init        初始化胶水代码（__init，一段机器码，dlopen 时跑）
[ 8] .plt / .plt.got   跳板（调外部函数）
[10] .text        代码（c_add/c_get_counter 的机器码）
[12] .eh_frame    异常展开表
[14] .init_array  构造函数指针表（每个槽 8B=一个函数地址，C so 是空的）
[15] .fini_array  析构函数指针表
[16] .dynamic     依赖清单
[17] .got / [18] .got.plt  外部符号门牌（占位）
[19] .data        已初始化全局（g_counter@0x400c）
[20] .bss         未初始化全局（0 字节占位）
[22] .symtab      完整符号表（strip 前，含所有函数名）
[24] .shstrtab    段名表
```

**★ `.init` 跟 `main` 没关系（纠正"so 有 main 才有 .init"的误解）**：
- 三个 so（C/C++/Rust）**都没有 `main` 函数**（so 是库，不是可执行文件），**但全有 `.init`/`.fini` 段**（实测纯 so 也有）
- **`main` 是可执行文件（exe）的入口；so 没有 main，so 的"初始化入口"是 `.init`/`.init_array`（dlopen 时跑）**
- **`.init`（胶水机器码）vs `.init_array`（函数指针表）是两套东西，别混**：
  - **`.init`** = 一段机器码（`__init` 函数，实测 C so 前 12 字节 `f30f1efa48...`），作用是"跑 `.init_array` 里的构造函数"
  - **`.init_array`** = **构造函数指针表**（每个槽 8 字节 = 一个函数地址，dlopen 时逐个跑）。C so 表空（没 C++ 静态对象）；**C++ so 表里有 `std::string g_str` 的构造器**（C++ 静态对象要 dlopen 时构造）
- **so vs exe 的入口区别**：exe 入口 = `_start → main`；so 入口 = `dlopen → .init → 跑 .init_array → 返回`（跟有没有 main 无关）

**注意 C so 没有 `.rodata` 段**——C 的小 so 字符串常量太少，链接器直接塞 `.data`（实测 `.data` 前 16 字节含常量数据）。

**C++ so（`so_cpp.so`，23272B，多 C++ 特性）** 比 C 多：
```
[16] .rodata           字符串常量（"basic_string: construction from..."）
[19] .gcc_except_table C++ 异常表（比 C 多）
[25] .data + .bss      （比 C 多 C++ 运行时符号，如 _ZNSt... 相关全局）
[20] .init_array       ★ 比 C 多：C++ 静态对象（std::string g_str）的构造器在这（C so 的 .init_array 是空的）
[6]  .gnu.version      + [7] .gnu.version_r   符号版本表（C++ 依赖 libstdc++ 的符号版本）
```
**C++ 的 `.init_array` 跟 C 的关键区别**（B.0.2 讲了 `.init`/`.init_array` 是 dlopen 入口）：
- **C so 的 `.init_array` 空**（C 没需要 dlopen 时构造的静态对象）
- **C++ so 的 `.init_array` 有 `std::string g_str` 的构造器**（C++ 静态对象要 dlopen 时构造 + `__cxa_guard` 保护）——这就是"C++ so 比 C 复杂"的具体体现，也是军棋"§124 ② 数据与指令对齐对应"的根（`std::vector`/`std::string` 全局要 dlopen 时构造）
**Rust so（`so_rust.so`，4503704B，std 静态链 + 调试段）** 比 C/C++ 多：
```
[10] .rodata         Rust 的常量/字符串
[17] .tdata / [18] .tbss   线程局部存储（Rust 比 C/C++ 多）
[21] .data.rel.ro    重定位后锁只读（Rust std 的类型元数据/vtable）
[30-35] .debug_*     调试信息（.debug_line/.debug_str 等，占了 so 体积的大头！）
```
**关键发现（实测）**：
- **C so 没 `.rodata`**（常量少，塞 `.data`）；**C++/Rust 有独立 `.rodata`**（字符串常量多）
- **Rust so 4.5MB 里，`.debug_*` 段（调试信息）占了绝大部分**（`readelf -S` 段总表 27MB，`.debug_*` 占大头）——**Rust 默认带调试段，`-O` 也没去**，这是 Rust so 比 C/C++ 大的主因
- **C++ so 多 `.gnu.version`**（依赖 libstdc++ 的符号版本匹配，Rust/C 没）

---

## B.1 三个语言编译完，哪些数据/指令放哪些段（核心，你的要求）

**"先讲 so 布局（B.0 段表），再讲三个语言编译完哪些数据/指令落哪些段"**——这才是"so 怎么来的"。

### B.1.1 C 语言：`so_c.c` 编译完的东西落哪

**完整真源码**（跟 `/tmp/opencode/so_c.c` 编译用的逐字一致，编译命令 `gcc -shared -fPIC -o so_c.so so_c.c`）：

```c
#include <string.h>
static int g_counter = 0;  // .data（已初始化全局）
int c_add(int a, int b) { g_counter++; return a + b; }  // 导出
int *c_get_counter() { return &g_counter; }  // 导出（返回 .data 地址）
```

**逐行标"编译后落哪段"**：

| 源码这一行 | 编译后落在哪个段 | 实测（`readelf`/`objdump`） |
|---|---|---|
| `#include <string.h>` | 不产生段（头文件只是声明，**调 `strlen` 才产生 PLT/`.got` 调用点**，但本代码没调） | — |
| `static int g_counter = 0;` | **`.data`**（已初始化全局；`=0` 其实也可能进 `.bss`，但显式 `=0` 编译器放 `.data`） | `readelf -s`：`g_counter`@0x400c（4B），在 `.data`（0x4000 起） |
| `int c_add(int a,int b) { g_counter++; return a+b; }` 函数体 | **`.text`**（机器码）；`g_counter++` 就是读写 `.data` 的 0x400c 那 4 字节 | `readelf -s`：`c_add`@0x10f9（39B），在 `.text`（0x1040 起） |
| `int *c_get_counter() { return &g_counter; }` 函数体 | **`.text`**（机器码：把 `.data` 的 0x400c 地址装进返回值） | `readelf -s`：`c_get_counter`@0x1120（17B） |
| 导出函数名 `c_add`/`c_get_counter` | **`.dynsym`**（动态符号表，dlopen 按名找） | `readelf --dyn-syms` 看到这两个 |

**C 语言的"布局规则"一句话**：**函数（机器码）进 `.text`，已初始化全局进 `.data`，未初始化全局进 `.bss`，外部函数调用的门牌进 `.got`，导出函数名进 `.dynsym`**（本 C so 没调外部函数，所以 `.got` 全 0）。

### B.1.2 C++ 语言：`so_cpp.cpp` 编译完的东西落哪

**完整真源码**（跟 `/tmp/opencode/so_cpp.cpp` 编译用的逐字一致，编译命令 `g++ -shared -fPIC -o so_cpp.so so_cpp.cpp`）：

```cpp
#include <string>
#include <vector>
static int g_val = 42;           // .data
static std::string g_str("x");  // C++ 静态对象（要 __cxa_guard 保护）
extern "C" int cpp_add(int a, int b) { g_str.clear(); return a + b + g_val; }  // 导出（C 接口避免 mangling）
```

**逐行标"编译后落哪段"**：

| 源码这一行 | 编译后落在哪个段 | 跟 C 的区别 |
|---|---|---|
| `#include <string>` / `<vector>` | 不产生段（头文件只是声明，**用 `std::string` 才引入 C++ 运行时**） | C 没 C++ 运行时 |
| `static int g_val = 42;` | **`.data`**（已初始化全局，跟 C 的 `g_counter` 一样） | 同 C |
| `static std::string g_str("x");` | ★ **复杂对象，分 4 处**：① 对象本体在 `.data`/`.bss`（**32 字节**，实测 `g_str`@0x5100）；② **构造器进 `.init_array`**（dlopen 时跑，把 0x5100 初始化为 "x"）；③ **`__cxa_guard` 变量保护**（防多线程重复构造）；④ **`std::string` 的 vtable/类型信息进 `.data.rel.ro` + 异常表进 `.eh_frame`/`.gcc_except_table`** | C 没有（C 静态就是 `.data`/`.bss`，不用 guard/vtable/异常表） |

**`g_str` 这个 `std::string` 变量的一生（实测 + SSO 纠正）**：
- **静态文件**：`g_str` 占 **32 字节在 `.bss`**（实测 `g_str`@0x5100，落在 `.bss` Addr=0x50e0 Size=0x40 范围；**不在 `.data`**，因为最终值是运行时构造器写的，静态 0 字节）；`"x"` 字面量在 `.rodata`（**编译期原料**）
- **dlopen**：`.init_array` 里的构造器跑 → 查 `.got` 门牌拿 **libstdc++ 的 `std::string` 构造器** → 用 `"x"` 把 `.bss` 的 `g_str`（32B）初始化
- **★ SSO 纠正（用户提"cpp 常量默认放堆"方向反了）+ 精确机制**：
  - **为什么**：`std::string` 的 32 字节里有个 16 字节的"内嵌缓冲区"（`_M_buf[16]`），**短字符串（≤15B + NUL）直接把字节塞进去，`_M_p` 指向自己，省掉一次堆分配**（用户理解："字节太小，没必要再走一次堆分配，直接拿现成的 32 字节空间存数据"）
  - **`std::string`（libstdc++ 64 位）的 32 字节精确布局**：
    | 偏移 | 字段 | 大小 | 短串（≤15B） | 长串（>15B） |
    |---|---|---|---|---|
    | 0 | `char* _M_p` | 8 | 指向自己 +16（`_M_buf`） | **堆地址**（`malloc` 返回） |
    | 8 | `size_t _M_string_length` | 8 | 长度 | 长度 |
    | 16 | `union { char* _M_capacity; char _M_buf[16] }` | 16 | **`_M_buf[0..15]` 直接存字节 + NUL** | **`_M_capacity`（capacity）** |
    | 23 | `char _M_local_capacity_or_heap_capacity`（判别哨兵） | 1 | 0~15（实际 capacity） | **16（sentinel，表"指针指向堆"）** |
  - **判别法**（`std::string` 内部）：`_M_local_capacity_or_heap_capacity == 16` → 长串（堆）；否则短串（SSO，`_M_buf` 存数据）
  - **`"x"`（1 字节）走 SSO**：`_M_p = &g_str._M_buf`（指向自己内部）、`_M_string_length = 1`、`_M_buf[0]='x'`、`_M_buf[1]=0`、哨兵字节 = 1 → **零次 malloc，对象自包含（单缓存行）**
  - **只有长串（>15B）才 `malloc` 堆**（`_M_p` 指向堆，union 存 capacity，析构要 `free`）
  - **C++11 标准规定 `_S_local_capacity`（实现自定义，libstdc++/GCC/MSVC = 15，libc++ = 22）**
- **运行时**：`g_str.clear()` 经 **PLT/`.got`** 调 **libstdc++ 的 `clear`**；全程 `'x'` 活在 `g_str` 的 32 字节里（SSO 内部 buf），**不堆、不引用 `.rodata`**（`.rodata` 那份只是编译期原料，构造完就无用）

**★ `g_str("x")` 是"构造变量"，不是"调用 g_str() 函数"（易混淆，C 程序员必踩）**：
- **`static std::string g_str("x")`** = **声明 + 构造** `g_str` 这个 `std::string` 对象（32 字节数据区），初始内容 = `"x"`。**`g_str` 是变量（`readelf -s` 实测 `OBJECT` 类型，32B，@0x5100），不是函数**（`FUNC` 类型才是函数，如 `cpp_add`）
- **C++ 的"构造语法"**：变量名 + 括号(参数) = 用参数构造这个变量（`Point p(3,5)` = 构造 p，x=3 y=5）。**跟"函数调用"写法撞了，但语义不同**：
  | 写法 | 真实语义 |
  |---|---|
  | `std::string g_str("x")`（**变量声明处**） | **构造** g_str 这个 std::string 对象，初始值 = "x"（不是调函数） |
  | `g_str.clear()` | **调用** g_str 的方法（这才是真"对象 + 方法"调用，`call` 指令经 PLT） |
  | `cpp_add(a, b)` | **调用** cpp_add 函数 |
- **`std::string` 的 32 字节布局**（libstdc++ ABI）：偏移 0 `char* ptr` + 偏移 8 `size_t size` + 偏移 16 `union{char* cap; char buf[16]}`。`"x"` 是短字符串 → 存内嵌 buf（`buf[0]='x'`，不分配堆）
| `extern "C" int cpp_add(...)` 函数体 | **`.text`**（机器码）；`g_str.clear()` 经 **PLT/`.got` 调 libstdc++ 的 `std::string::clear`** | C 没 C++ 运行时调用 |
| **`extern "C"` 的作用** | `.dynsym` 里符号名 `cpp_add` **原样（不 mangle）** | **不加 `extern "C"` 的话，`cpp_add` 被 mangle 成 `_Z7cpp_addii`**（C++ name mangling，`.symtab`/`.dynsym` 里变 `_Z...`） |
| **字符串常量（`std::string` 内部 `"x"` + 异常消息）** | **独立 `.rodata` 段**（"basic_string: construction from..." 实测在 `.rodata`） | C so 没 `.rodata`（常量少，塞 `.data`） |
| **依赖 libstdc++ 的符号版本** | **`.gnu.version` + `.gnu.version_r`**（`libstdc++` 的 `GLIBCXX_3.4` 等版本标记） | C 没（只依赖 libc） |

**C++ 的"布局规则"一句话**：**比 C 多 `.rodata`（字符串独立段）+ `.data.rel.ro`（vtable/类型）+ `.init_array`（C++ 静态对象构造器）+ `.gcc_except_table`（异常）+ `.gnu.version`（符号版本）+ name mangling（`_ZN...`）**。军棋 32 位 so 是 C++（依赖 `libstdc++.so`），**所以军棋有 C++ 这些复杂段**（§124 ② 数据与指令对齐对应缺口）。

**"一个符号（如 `cpp_add`）在 so 里出现在哪 3 处"**（实测 `so_cpp.so`）：

| 在哪 | 是什么 | 实测（`so_cpp.so`） |
|---|---|---|
| **`.text`（机器码）** | `cpp_add` 的**函数体（真正的代码）** | 虚拟地址 `0x2419`（文件偏移 `0x2419`，`.text` 段 0x2360 起）；反汇编开头 `endbr64`/`push rbp`/.../`call 0x22e0`（调 `g_str.clear()` 经 PLT） |
| **`.symtab`（完整符号表）** | `cpp_add` 的**名字 + 地址**（strip 前全量符号，含内部 `g_val`/`g_str` + C++ 运行时 `_Z...`） | `readelf -s`：`22: 0x2419 51 FUNC GLOBAL ... cpp_add`；内部 `_ZL5g_val`/`_ZL5g_str`（mangle）也在 `.symtab` |
| **`.dynsym`（导出符号表）** | `cpp_add` 作为**导出符号**（dlopen 按名找的就是这个） | `readelf --dyn-syms`：`cpp_add` GLOBAL（因 `extern "C"` 不 mangle）+ 依赖 `GLIBC`/`GLIBCXX` 的 UND 符号 |

**"`.dynsym` 是什么"（跟 `.symtab` 的区别，B.2.3 的核心）**：
- **`.symtab` = 完整符号表**（所有符号：内部 + 导出 + C++ 运行时），**strip 删的是这个**（省空间，内部函数名全没）
- **`.dynsym` = 动态符号表**（只有**导出符号 + 外部依赖符号**），**strip 不删**（dlopen/linker 要按名找导出符号 + 解析依赖）
- **为什么 so 要两个符号表**：`.symtab` 是"全量调试用"（发布 strip 掉），`.dynsym` 是"对外接口"（dlopen 靠它，永不删）
- **军棋对应**：军棋原版 so（1.4MB，strip 了）**`.symtab` 删了**（内部业务函数无名），**`.dynsym` 保留 14 个 `Java_com_cnvcs_App_On*`**（导出符号）——**这就是"strip 后内部函数只能按地址调，但 14 个 JNI 入口能按名调"的原因**

**SSO 跟军棋 unidbg"§124 ② 数据与指令对齐对应缺口"的关系（为什么这个点重要）**：
- 军棋 so 的全局 `std::string`（`updateUrl`/`statHost`/`cfgHost` 等配置串）分两类：
  - **短串（≤15B，SSO）**：对象自包含（32 字节里存数据），**不依赖堆地址** → unidbg 模拟容易
  - **长串（>15B，堆）**：`_M_p` 指向 `malloc` 返回的堆地址（ASLR 每次不同）→ **unidbg 模拟不到真堆地址，读到 0/垃圾 → 运行时对齐分歧**（§124z 黑屏根因之一）
- **所以 SSO 直接影响"unidbg 能模拟到哪个层级"**：短串安全（SSO 自包含），长串要堆（模拟不全 → 运行时对齐卡点）
- **逆源码路线（用户"逻辑补齐就能开发"）不受此影响**：重写后 `std::string` 跑起来 SSO/堆自动分配（程序自己跑），不用逆向堆地址（"要毛线运行时堆分配"）

### B.1.3 Rust 语言：`so_rust.rs` 编译完的东西落哪

**完整真源码**（跟 `/tmp/opencode/so_rust.rs` 编译用的逐字一致，编译命令 `rustc --crate-type cdylib -O so_rust.rs -o so_rust.so`，`cdylib` = 共享库）：

```rust
#[no_mangle]
pub extern "C" fn rust_add(a: i32, b: i32) -> i32 { a + b }

#[no_mangle]
pub extern "C" fn rust_get_sum() -> i32 {
    static mut S: i32 = 0;   // Rust 静态全局（链接期初始化，不像 C++ 要 __cxa_guard）
    unsafe { S += 100; S }
}
```

**逐行标"编译后落哪段"**：

| 源码这一行/特性 | 编译后落在哪个段 | 跟 C/C++ 的区别 |
|---|---|---|
| `#[no_mangle]` 导出函数 `rust_add`/`rust_get_sum` 函数体 | **`.text`**（机器码）+ **`.dynsym`**（符号名 `rust_add`/`rust_get_sum` 原样，**不 mangle**） | C++ mangle（`_ZN...`），Rust 用 `#[no_mangle]` 不 mangle |
| `static mut S: i32 = 0` 静态全局 | **`.data`**（**链接期初始化，不用 `__cxa_guard`**） | C++ 静态对象（`std::string`）要 `__cxa_guard` 保护 + `.init_array` 构造器；Rust 静态整数不用 |
| **Rust 标准库（`std::alloc`/`std::panic`/...）** | **全静态链进 `.text`**（so 体积 4.5MB 的主因，实测 `readelf -S` 段总表 27MB 里 `.debug_*`+std 代码占大头） | C/C++ 动态依赖 libc/libstdc++，Rust 自带 std（静态链进 so） |
| **线程局部存储（`thread_local`，Rust 特有）** | **`.tdata` + `.tbss`**（实测 Rust so 有这俩段） | C/C++ 没专门 `.tdata` 段 |
| **默认带调试段** | **`.debug_line`/`.debug_str`/`.debug_aranges` 等**（实测占 so 体积大头，`-O` 也没去） | C/C++ 默认不带调试段（要 `-g` 才有） |
| **依赖** | `libgcc_s.so.1` + `libc.so.6`（实测 `readelf -d` 的 `DT_NEEDED`，**不依赖 C++ 运行库**） | C++ 依赖 `libstdc++.so` |

**Rust 的"布局规则"一句话**：**比 C 多 `.tdata`/`.tbss`（线程局部）+ `.debug_*`（默认带调试段，体积大）+ std 全静态链进 `.text`（4.5MB）；比 C++ 简单（静态全局不用 `__cxa_guard`，不依赖 C++ 运行库）**。

### B.1.4 三个语言 + 军棋 so 的"数据/指令落段"总对照

| 维度 | C so | C++ so | Rust so | **军棋 so（C++ 32 位）** |
|---|---|---|---|---|
| **函数（机器码）** | `.text` | `.text` + vtable 虚函数 | `.text`（+ std 静态链） | `.text`（**1.43MB/1435156B**，14 个 `On*` 入口 + C++ 业务函数） |
| **字符串常量** | 塞 `.data`（无 `.rodata`） | 独立 `.rodata` | 独立 `.rodata` | **独立 `.rodata`（118832B 明文，1715 字符串已 dump）** |
| **已初始化全局** | `.data`（`g_counter`） | `.data` + `.data.rel.ro`（vtable） | `.data`（std 类型元数据） | `.data` + `.data.rel.ro`（C++ vtable，§124z 黑屏根因） |
| **未初始化全局** | `.bss` | `.bss` | `.bss` | `.bss`（MemSiz 0x4a364 远大于 FileSiz） |
| **外部符号门牌** | `.got`/`.got.plt`（全 0，无依赖） | `.got`（占位，依赖 libstdc++） | `.got`（占位） | **`.got`（214 PLT + 6 GLOB_DAT，占位，dlopen 填）** |
| **重定位表** | `.rela.dyn`（7 条） | `.rela.dyn`+`.rela.plt`（~10 条） | `.rela.dyn`+`.rela.plt`（491+49+3） | **`.rel.dyn`+`.rel.plt`（5539 条：RELATIVE 5312 + JUMP 214 + GLOB 6 + ABS 7）** |
| **导出符号** | `.dynsym`（`c_add`） | `.dynsym`（`_ZN...` mangle） | `.dynsym`（`rust_add` 不 mangle） | **`.dynsym`（14 个 `Java_com_cnvcs_App_On*`）** |
| **符号版本** | 无 | `.gnu.version` | `.gnu.version` | 有（API 19 时代） |
| **体积** | 15KB | 23KB | **4.5MB（std 静态链 + 调试段）** | **1.4MB（C++ 运行时 + 反检测）** |

---

## B.2 机制：`.got` 占位 / 重定位 / strip（建立在 B.0 段布局之上）

**B.0 讲了"so 有哪些段"，B.1 讲了"三个语言的东西落哪些段"。现在讲这些段在运行时怎么"活起来"——3 个机制**。



**★ PLT 为什么存在（第一性原理，用户点出）：解耦外部 so 函数地址**

> **用户判断："PLT 是 so/ELF 的一个机制，主要是解耦与外部 so 函数地址"——对，这是 PLT 设计的第一性原理。**

**PLT 解决的根本问题**：你的 so 调 libc 的 `strlen`，但 **`strlen` 的真实地址 dlopen 前不知道**（libc 被加载到哪个基址由 ASLR 定，每次不同）。如果编译时就把 `call 0x7f001234`（`strlen` 地址）**写死进 `.text`**，**libc 基址一变（ASLR）代码就全崩**。

**PLT 的解耦方案（"代码写跳板编号 + 地址放门牌"）**：
```
不写死外部函数地址 → 拆成"跳板 + 门牌占位"：
  .text 里：call .plt[N]      （跳板编号，跟外部 so 地址无关，编译期就定死，永不改）
  .plt[N]：跳板机器码         （查 .got.plt[N] 再跳，地址运行时才定）
  .got.plt[N]：占位（0）      （dlopen 时 linker 才填真地址 = libc 基址 + strlen 偏移）
  .rela.plt[N]：JUMP_SLOT     （记录"这个跳板要填哪个外部符号"）
```

**"解耦"的精确含义**：**PLT 把"调外部函数"拆成"跳板（编译期定，代码里，跟外部 so 地址无关）+ 门牌（运行时填，数据里）"**：
- **代码（`.text`）和外部依赖地址分离**：代码里只有 `call .plt[N]`（跳板编号 N，编译期定死），**不写外部 so 的真实地址**
- **填地址的时机推迟到 dlopen**：外部 so 加载到哪（ASLR 基址）只影响 `.got.plt` 门牌填的值，**不影响已编译的代码**（`call .plt[N]` 永远不变）
- **好处**：① 同一个 `.text` 代码，libc 加载到不同基址都能用（门牌填不同值）；② **延迟绑定**（ch06 6.2）：`.got.plt` 不是 dlopen 全填，而是**首次调用才填**（跳板首查门牌发现没填 → 调 linker resolver → 填上 → 二次直接跳），**没真正调用的外部函数可以永远不填**（省 dlopen 时间）

**PLT 的 3 层（是什么/为什么/怎么解耦）**：

| 层 | 内容 |
|---|---|
| **是什么** | `.plt` 段的跳板机器码（`AX` 可执行，"压序号 + 跳主跳板查 `.got.plt`"） |
| **为什么** | **解耦外部 so 函数地址**（外部 so 基址未知 + ASLR 每次不同，代码不能写死地址） |
| **怎么解耦** | 代码写跳板编号（编译期定死）+ 地址放门牌（运行时 dlopen 填）→ 代码与外部 so 地址分离 |

**先澄清：PLT 是什么（不是"标准库的 so"，是你 so 里的跳板机器码）**：

> **用户追问："PLT 是指什么？是标准库的 so？"→ 不是。PLT 在你 so 里（`.plt` 段，`AX` 可执行），标准库 so 是 PLT 跳板"指向的目标"。**

**PLT = Procedure Linkage Table（过程链接表）**：
- **PLT 跳板在你 so 的 `.plt` 段**（`AX` 可执行代码，跟 `.text` 并列，B.0.1 的"代码段"之一），**不是标准库 so**
- **标准库 so（libc/libstdc++.so）是独立文件**（`DT_NEEDED` 依赖），**PLT 跳板的作用是"帮你找到标准库 so 里函数的真地址"**
- **PLT 干的事**：你 so 调 `strlen`（在 libc 里，地址 dlopen 前不知道）时，**编译器不让你直接跳 `strlen`，而是跳你自己 so 的 PLT 跳板**，跳板查 `.got`（门牌）拿真地址再跳

**PLT 跳板长什么样（实测 `so_cpp.so` 的 `.plt` 段，反汇编）**：
```
PLT[0] (0x2020)：主跳板（resolver stub）
  push qword [rip+0x2fca]   ← 压 PLT 序号（0）
  jmp qword [rip+0x2fcc]    ← 跳 .got.plt[0]（查真地址；没填就跳 resolver 找 linker）
PLT[1] (0x2030)：调"第 1 个外部函数"的跳板
  endbr64 / push 0 / jmp 0x2020   ← 压序号 0 → 跳回主跳板查 .got.plt[0]
PLT[2] (0x2040)：调"第 2 个外部函数"的跳板
  endbr64 / push 1 / jmp 0x2020   ← 压序号 1 → 跳回主跳板查 .got.plt[1]
```
**PLT 跳板 = "压序号 + 跳主跳板查 `.got.plt`"**——整段机器码在你 so 的 `.plt` 段（`AX`），不是标准库 so。

**PLT 跟 `.got`/`.dynsym`/标准库 so 的 4 角色分工**（回应"PLT 是标准库 so 吗"）：

| 角色 | 在哪 | 是什么 | 是标准库 so 吗 |
|---|---|---|---|
| **PLT 跳板** | **你 so 的 `.plt` 段**（`AX`） | 跳板机器码（压序号 + 跳 `.got`） | ❌ 不是，是你 so 自己的代码 |
| **`.got.plt` 门牌** | **你 so 的 `.got.plt` 段**（`RW`） | 占位（dlopen 填真地址） | ❌ 不是，是你 so 的门牌 |
| **`.rela.plt` 重定位** | **你 so 的 `.rela.plt` 段** | "哪个 PLT 跳板填哪个外部函数"（JUMP_SLOT） | ❌ 不是，是你 so 的重定位表 |
| **libc.so/libstdc++.so** | **独立文件**（`DT_NEEDED` 依赖） | 真函数体（`strlen` 的机器码在这） | ✅ **这才是标准库 so**（PLT 跳板指向的目标） |

**PLT 完整链路**（你 so 调标准库函数 `strlen`）：
```
① 编译时：c_add 里 "call strlen" → 改成 "call .plt[N]"（跳你自己 so 的 PLT 跳板）
② dlopen 时：linker 按 .rela.plt（JUMP_SLOT）填 .got.plt[N] = libc.so 里 strlen 的真地址
③ 运行时首调：跳 .plt[N] → 查 .got.plt[N]（已填真地址）→ 跳 libc.so 的 strlen
④ 运行时二调：.plt[N] 直接跳 .got.plt[N] 真地址（延迟绑定，ch06 6.2）
```

**军棋实测**：军棋 so 的 **214 个 PLT 跳板在军棋 so 自己的 `.plt` 段**（`.rel.plt` 214 条 `JUMP_SLOT`）；军棋的 **7 个 `DT_NEEDED` 标准库 so（libc/libstdc++/libm/liblog/libdl/2 GL）是 PLT 跳板"指向的目标"**——PLT 不是标准库 so，标准库 so 是 PLT 跳过去拿真函数的地方。

### B.2.1 `.got` = 外部符号门牌占位（不是"没数据"，是"占位值"）

**纠正"`.got` 静态全 0 / 没数据"的错误表述**（你最早点的，B.0 地基后重讲）：

| 错误表述 | 正确表述 |
|---|---|
| "`.got` 静态全 0（没数据）" | "`.got` 静态是**占位值**（多数 0 或相对偏移，代表'还没填号'），**dlopen 重定位后填 基址+偏移 = 真实地址**" |

**你的"小区第三间房"类比（B.0 段布局下的精确定义）**：
```
so 文件里的 .got 段（B.0.1 的"动态链接"类）：
  .got[foo] = 占位值（"第 N 间房"的偏移/重定位标记，不是真地址）
dlopen 时（linker 按 .rela.plt 重定位表）：
  查 .dynsym 拿 foo 真地址 → 填 .got[foo] = 基址+偏移（"小区地址 + 第 N 间房" = 完整地址）
so 调 foo：
  跳 .plt[foo]（跳板）→ 查 .got[foo]（已填真地址）→ 跳真 foo
```

**三个 so 的 `.got` 实测（B.1.4 表里的"外部符号门牌"列）**：
- **C so（无外部依赖）：`.got` 全 0**（没东西要重定位，占位=0）
- **C++/Rust so（有依赖）：`.got` 占位值 = 相对偏移**（dlopen 后 `+= 基址` 才是真地址）
- **军棋 so（7 依赖 + 214 PLT）：`.got` 占位值 = 相对偏移**（5539 条重定位要填，dlopen 后填真地址；★ "33 PLT" 是早期小 so 口径笔误，实测 `.rel.plt` 214 条 JUMP_SLOT = 214 个 PLT 跳板）

**"占位 vs 没数据"的区别**：`.got` **有数据（占位值）**，只是"还没填小区名"。**unidbg 要做的就是"模拟 dlopen 填 .got 占位"**（不是"造数据"，是"填真值"）。

### B.2.2 重定位表（`.rela.dyn`/`.rela.plt`）= "哪些槽要填谁"

**先用三个 so 的真实重定位条目讲（`readelf -r` 实读），再上军棋**——重定位表记"哪个槽要填什么值"，dlopen 时 linker 照表填。

**重定位条目长什么样**（`readelf -r` 输出）：
```
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
000000003e68  000000000008 R_X86_64_RELATIVE                    10f0
  ↑              ↑             ↑
  要填的槽地址    槽序号        重定位类型 + 要填什么
```
- **Offset**：要填的槽在 so 里的虚拟地址（如 `.init_array`/`.got`/`.data.rel.ro` 的某槽）
- **Type**：重定位类型（决定"填什么值"）
- **Sym. Name + Addend**：要填的符号（外部 so 的函数/变量）或偏移量（self-relative）

**3 种核心重定位类型（"填什么值"的区别）**：

| 类型 | 填什么值 | 在哪个表 | 作用 |
|---|---|---|---|
| **`RELATIVE`** | **基址 + 偏移**（so 自己的地址，self-relative） | `.rela.dyn` | 填 `.init_array`/`.data`/`.data.rel.ro` 的"指向 so 自身某处"的槽（vtable 指针/构造器指针） |
| **`JUMP_SLOT`** | **外部 so 里函数的真地址** | `.rela.plt` | 填 `.got.plt` 门牌（走 B.2.1 的 PLT 跳板，调外部函数） |
| **`GLOB_DAT`** | **外部 so 里全局变量的真地址** | `.rela.dyn` | 填 `.got` 门牌（直接读外部全局变量，不走 PLT） |

**三个 so 的真实重定位条目（`readelf -r` 实读，逐一对照 B.1.1/B.1.2/B.1.3 的源码）**：

**C so（`so_c.so`，.rela.dyn 7 条，无 .rela.plt）**——C so 没调外部函数，所以**只有 `RELATIVE` + `GLOB_DAT`，没有 `JUMP_SLOT`**：
```
.rel.dyn（7 条）：
  0x3e68  RELATIVE  10f0      ← .init_array[0] 填 0x10f0（.init 胶水函数地址，so 自己的）
  0x3e70  RELATIVE  10b0      ← .fini_array[0] 填 0x10b0（.fini 胶水函数地址）
  0x4000  RELATIVE  4000      ← .data 段基址（self-relative）
  0x3fc8  GLOB_DAT  __cxa_finalize   ← .got 填 libc 的 __cxa_finalize（全局变量/函数门牌）
  0x3fd0  GLOB_DAT  _ITM_registerTMClone
  0x3fd8  GLOB_DAT  _ITM_deregisterTMClones
  0x3fe0  GLOB_DAT  __gmon_start__
（无 .rela.plt——C so 没调外部函数，没 PLT 跳板要填）
```
**对应 B.1.1 的 C 源码**：`g_counter`/`c_add`/`c_get_counter` 全在自己 so 里（不依赖外部函数），所以**重定位只有"填自己 so 的槽"（RELATIVE）+ 链接器注入的固定门牌（GLOB_DAT `__cxa_finalize` 等）**，**没有调外部函数的 `JUMP_SLOT`**。

**C++ so（`so_cpp.so`，.rela.dyn 10 条 + .rela.plt 25 条）**——C++ 用了 `std::string`，**多了一堆 `JUMP_SLOT`（调 libstdc++ 的 `std::string` 方法）**：
```
.rel.dyn（10 条）：
  0x4dc8  RELATIVE  2410      ← .init_array 填 so 自己的构造器
  0x4dd0  RELATIVE  2512
  0x4dd8  RELATIVE  23d0
  0x4fc0  GLOB_DAT  __cxa_finalize          ← 同 C so
  0x4fc8  GLOB_DAT  _ZNSt7__cxx1112ba...@GLIBCXX_3.4.21  ← C++ 运行时全局（libstdc++ 的 std::basic_string 相关）
  0x50d8  R_X86_64_64  __gxx_personality_v0@CXXABI_1.3   ← 异常处理 personality（C++ 特有）
  ...
.rel.plt（25 条 JUMP_SLOT，全填 libstdc++/libc 的外部函数地址）：
  0x5008  JUMP_SLOT  strlen@GLIBC_2.2.5       ← 调 libc 的 strlen（.got.plt 门牌）
  0x5048  JUMP_SLOT  __cxa_atexit@GLIBC_2.2.5 ← 注册 C++ 析构（B.1.2 的 g_str 构造器要 __cxa_atexit）
  0x5078  JUMP_SLOT  __stack_chk_fail@GLIBC_2.4 ← 栈金丝雀（-fstack-protector）
  0x50a0  JUMP_SLOT  _Unwind_Resume@GCC_3.0    ← C++ 异常展开
  ... 其余 21 条全是 _ZNSt7__cxx11...（libstdc++ 的 std::basic_string 方法，B.1.2 的 g_str.clear() 经这些门牌调）
```
**对应 B.1.2 的 C++ 源码**：`g_str`（`std::string`）调 `clear()`/构造器等 → **经 `.rela.plt` 的 25 条 `JUMP_SLOT` 填 `.got.plt` 门牌 → 走 B.2.1 的 PLT 跳板 → 调 libstdc++ 的 `std::basic_string` 方法**。**C 没有这些（C so 没 `.rela.plt`），C++ 有（用了 `std::string`）**——这就是 B.1.4 表里"C++ 重定位 10+25 vs C 7"的来由。

**Rust so（`so_rust.so`，.rela.dyn 543 条 + .rela.plt 3 条，Rust 特有）**：
```
.rel.dyn（543 条，Rust 比 C/C++ 多——std 全静态链，self-relative 槽多）：
  491 条 RELATIVE（so 自己的 vtable/常量池/类型元数据，B.1.3 的 std 静态链）
  49 条 GLOB_DAT（外部全局变量）
  3 条 DTPMOD（★ Rust 特有：取模地址，thread_local/跨 so 偏移）
.rel.plt（3 条 JUMP_SLOT，Rust 调的外部函数少）：
  0x...  JUMP_SLOT  _Unwind_Resume@GCC_3.0   ← Rust panic 展开（libgcc_s）
  0x...  JUMP_SLOT  ...（libc 的 2 个）
```
**对应 B.1.3 的 Rust 源码**：Rust `std` 全静态链进 `.text`（4.5MB）→ **self-relative 槽特别多（491 RELATIVE）**；**Rust 用 `thread_local` 所以有 3 条 `DTPMOD`（C/C++ 没有，B.1.3 讲的 `.tdata`/`.tbss` 线程局部）**；Rust 调外部函数少（`.rela.plt` 只 3 条 `JUMP_SLOT`，不像 C++ 的 25 条——Rust 的 `std` 自带实现，不依赖 libstdc++）。

**军棋 so（B.2.2 原版，`readelf -r` 实测，ARM 32 位）**：
```
.rel.dyn（5312 + 6 + 7 条）：
  5312 条 R_ARM_RELATIVE   ← .data/.data.rel.ro 的 self-relative（vtable 指针/常量池，§124 ② 数据与指令对齐对应缺口在这）
  6 条 R_ARM_GLOB_DAT      ← .got 的全局变量门牌（stdout 等）
  7 条 R_ARM_ABS
.rel.plt（214 条）：
  214 条 R_ARM_JUMP_SLOT   ← .got.plt 的外部函数门牌（__aeabi_fadd/sem_init/glClear 等，走 PLT 调 7 个 DT_NEEDED 标准库 so）
```
**军棋 vs 三 so 的重定位差异**（逆向视角）：

| 类型 | C so | C++ so | Rust so | **军棋 so（ARM 32 位）** |
|---|---|---|---|---|
| **`RELATIVE`**（self-relative） | 3 | 4 | **491**（std 静态链多） | **5312**（**1.43MB/1435156B** so，vtable/常量池多） |
| **`JUMP_SLOT`**（调外部函数，走 PLT） | **0**（没调外部） | 25（`std::string` 方法） | 3（panic 展开） | **214**（调 libc/GL/libstdc++ 214 个函数） |
| **`GLOB_DAT`**（读外部全局变量） | 4 | 5 | 49 | 6 |
| **`DTPMOD`**（Rust 特有） | 无 | 无 | **3** | 无 |
| **合计** | 7 | 35 | 546 | **5539** |

**重定位表 = "哪些门牌要填谁"的完整链路**（B.2.1 的 PLT + B.1b 的外部符号串起来）：
```
编译期（B.1）：源码调外部函数（C++ 的 g_str.clear()）→ 链接器生成
  .rela.plt[JUMP_SLOT]：".got.plt[N] 要填 strlen 的地址"（B.2.2 重定位表）
  .plt[N]（跳板）：查 .got.plt[N] 再跳（B.2.1 PLT，解耦外部 so 地址）
  .dynsym（UND 符号）：strlen 的符号名（查 libc.so 解析）
dlopen 时（B.2）：linker 按 .rela.plt 填 .got.plt[N] = libc 基址 + strlen 偏移
运行时：调 g_str.clear() → 跳 .plt[N] → 查 .got.plt[N]（已填）→ 跳 libstdc++ 的 std::basic_string::clear
```

**"外部符号依赖对应版本/架构的 so"（架构 + 版本双匹配，B.0 依赖清单 `.dynamic` 的 `DT_NEEDED`）**：
- **架构匹配**：军棋 32 位 so 的 `DT_NEEDED libc.so` 必须解析到 **32 位 `libc.so`**（不是 64 位的）；重定位表里的 `R_ARM_*` 类型也是 32 位 ARM 的（`R_X86_64_*` 是 64 位 x86，三 so 用）
- **版本匹配**：依赖 so 的符号版本要对得上（C++ so 的 `JUMP_SLOT` 带 `@GLIBCXX_3.4.21`/`@GLIBC_2.2.5` 版本标记，军棋 API 19 时代）
- **不匹配的后果**：64 位 only 设备没 32 位的 7 个依赖库 → `DT_NEEDED` 解析不到 → dlopen 失败（**军棋"64 位装不上"根因**，不是装不了 APK，是没 32 位依赖库）

### B.2.3 strip = 删 `.symtab`（调试符号），留 `.dynsym`（导出符号）

**B.0 段表里 `.symtab`（完整符号表，所有函数名）vs `.dynsym`（导出符号表）**：

```
strip 前（so_c.so）：
  [22] .symtab  SYMTAB   → 完整符号表（c_add/c_get_counter/g_counter 全名，10 FUNC）
  [ 4] .dynsym  DYNSYM   → 导出符号表
strip 后（so_c_stripped.so）：
  [ 4] .dynsym  DYNSYM   → 保留（导出符号 dlopen 要按名找）
  （.symtab 没了）
```

**strip 删 `.symtab`（调试符号，内部函数名），留 `.dynsym`（导出符号）**——这是"内部函数怎么找"的关键：
- **strip 后**：内部函数名全删（`.symtab` 没了），**只能按地址调**（`0x39e14` 不知道是"走子判定"）
- **导出函数（`.dynsym`）保留**：军棋 14 个 `On*` 在 `.dynsym`（Java 层要按名调，strip 不删）

**先用三个 so 的真实 strip 前后对比讲（`readelf` 实读，跟 B.2.2 一样三 so 先行）**：

**strip 删什么、留什么（`so_c.so` 实测）**：

| 符号表 | strip 前 | strip 后 | 实测（`readelf`） |
|---|---|---|---|
| **`.symtab`（完整符号表）** | ✅ 在（19 符号：`c_add`/`c_get_counter`/`g_counter` + 运行时全量） | ❌ **删了**（段数=0） | `readelf -S` 段数 1→0 |
| **`.dynsym`（导出符号表）** | ✅ 在（2 导出） | ✅ **保留**（段数=1） | `readelf -S` 段数 1→1 |
| **体积** | 15184 B | **14016 B**（变小） | `stat` 实测 |

**strip 后 `.dynsym` 精确留了什么（`so_c_stripped.so` 实测）**：
```
.dynsym 还剩 7 条：
  UND 外部依赖（strip 不删，dlopen 解析）：__cxa_finalize/_ITM_registerTMClone/...
  导出函数（GLOBAL，strip 不删，dlopen 按名找）：c_get_counter@0x1120 / c_add@0x10f9
  被删的（.symtab 才有，strip 删了）：g_counter（static 内部变量，不在 .dynsym，彻底没名）
```

**"strip 后只能按地址调内部函数"的精确含义（实测）**：
- **导出函数（`c_add`，`.dynsym` 留）** → **还能按名调**（dlopen 找得到）✅
- **内部函数/变量（`g_counter`，`.symtab` 删了）** → **只能按地址**（名字没了，`0x400c` 不知道是 `g_counter`）❌
- **外部依赖（`__cxa_finalize`，`.dynsym` UND 留）** → **还能解析**（dlopen 要）✅

**三个 so 的 strip 差异（语言特性决定 `.dynsym` 内容，印证 B.1.2/B.1.3 name mangling）**：

| so | `.dynsym` 导出符号（strip 不删） | 外部依赖（UND，strip 不删） | 特点 |
|---|---|---|---|
| **C so** | `c_add`/`c_get_counter`（原样，C 不 mangle） | `__cxa_finalize`/`__gmon_start__`（4 个） | 最简 |
| **C++ so** | `cpp_add`（`extern "C"` 不 mangle；没的话是 `_Z7cpp_addii`） | **`_ZNSt7__cxx11...`（libstdc++ 的 `std::string` 方法，mangle + `@GLIBCXX_3.4.21`）** + `strlen`/`__cxa_atexit` | 多 C++ 运行时依赖 |
| **Rust so** | `rust_add`/`rust_get_sum`（`#[no_mangle]` 不 mangle） | `_Unwind_Resume`（panic）+ libc（**不依赖 C++ 运行库**） | 无 `_ZN...` C++ 依赖 |

**军棋原版 so vs 翻译体 so（★ 别混，逆向用原版，实测 `readelf`）**：**军棋原版 so vs 翻译体 so（★ 别混，逆向用原版，实测 `readelf`）**：

| so | 大小 | strip 状态（实测） | 符号 |
|---|---|---|---|
| **原版**（`军棋.apk` 解出，`/tmp/opencode/orig_apk/lib/armeabi/libjunqi.so`） | 1.4MB | **strip 了**（`.symtab` 段数=0，实测删了） | 14 `On*`（`.dynsym` 保留）+ C++ 运行时符号（`_ZNSt...`），**内部业务函数无名** |
| **翻译体**（`dec/` 重打包，`/tmp/opencode/jq_apk/dec/lib/armeabi/libjunqi.so`） | 5.4MB | **没 strip**（`.symtab` 段数=1，实测还在） | 含 **10 个 `g_jqt_*`**（翻译体自加全局，`jqt_mem.c`）+ 14 `On*` |

**strip 对"内部函数怎么找"的影响（军棋实测）**：
- **原版 so（strip）**：`.symtab` 删了 → 内部业务函数（走子判定等）**没名字**，**只能按地址调**（`0x39e14` 不知道是"走子判定"，要反汇编 + `.rodata` 字符串交叉推断）；**`.dynsym` 的 14 个 `Java_com_cnvcs_App_On*` 保留**（Java 层按名调，strip 不删）
- **翻译体 so（未 strip）**：`.symtab` 还在（编译生成），**含 10 个 `g_jqt_*` 自加符号**（影子运行时的全局），**但这是"逆向产物重新编译的"，不是原版 so**——逆向军棋要用**原版 so**（strip 的），别拿翻译体 so 当原版

**strip 的 2 个符号表（B.1.2 "符号 3 处"呼应）**：
- **`.symtab`（完整符号表）**：所有符号（内部 + 导出 + C++ 运行时），**strip 删它**（省空间，内部函数名全没）
- **`.dynsym`（导出符号表）**：只有导出符号 + 外部依赖（UND），**strip 不删**（dlopen 按名找，军棋 14 个 `On*` 靠它）

---

## B.3 一个 so 是怎么"打包"出来的（正向 4 步，逆向反着读）

**B.0（段布局）+ B.1（数据/指令落段）+ B.2（机制）之后，把"正向生成 so"串成 4 步**（逆向 = 反着读这 4 步）：

```
① 编译（gcc -c / rustc）：.c/.rs → .o
   编译器生成：函数机器码（.text）+ 全局变量（.data/.bss）+ 常量（.rodata）+ 重定位占位（.rela.*，"哪些槽要 linker 填"）
   C/C++/Rust 差异：C 常量塞 .data；C++ 多 .rodata/.data.rel.ro/异常；Rust 多 .tdata/调试段 + std 静态链

② 链接（gcc -shared / rustc cdylib）：.o → .so
   链接器合并 .o 的段 → 生成 .so 的段表（B.0 的 4 类段）
   - 外部函数调用 → .got 占位 + .plt 跳板 + .rela.plt（"要填 strlen"）
   - 导出函数 → .dynsym（"so 对外暴露的"）
   - 依赖清单 → .dynamic 的 DT_NEEDED（7 个，军棋）

③ strip（可选）：删 .symtab（调试符号，内部函数名）→ 省空间
   保留 .dynsym（导出符号，dlopen 按名找）
   军棋原版 so：strip 了（.symtab 删，内部函数无名）

④ dlopen（运行时）：linker 读段表 + 重定位表 → mmap 段 + 填 .got 占位（基址+偏移）→ 跑 init_array
   真机：内核 linker 填 .got
   unidbg：简化 Linker 模拟填 .got（ch06 6.2 的 12/5312/214/6 全通）
```

**"逆向 so" = 反着读这 4 步**（B.1.4 表是"逆向 4 步"的落地）：
- ④ 读：`readelf -l/-S/-r`（段表 + 重定位表）→ "so 要填哪些 .got 槽"
- ③ 读：`readelf -s`（.symtab，strip 前后）+ `readelf --dyn-syms`（.dynsym，导出符号）
- ② 读：`readelf -S`（段表，B.0 的 4 类）+ `objdump -d`（.text 机器码）
- ① 读：`objdump -d`/capstone（反汇编 .text）→ "函数逻辑是什么"

---

## B.4 军棋 32 位 ARM so 的真实对比（B.0 段表 + B.1 落段 + B.2 机制全用上）

军棋 so（`军棋.apk` 解出的 1.4MB 原版，32 位 ARM，`readelf` 实读）跟 B.1.4 的 C/C++/Rust 小 so 对比：

```
军棋 so 段表（readelf -l 实读）：
  LOAD R     0x0        → 头部 + .rodata（118832B 明文，1715 字符串）
  LOAD R E   0x14c050   → .text（**1.43MB/1435156B** 明文，熵 7.10，非壳）
  LOAD RW    0x509a90   → .data/.got（.got 占位，214 PLT + 6 GLOB_DAT）
  LOAD RW    0x50c318   → .bss（MemSiz 0x4a364 远大于 FileSiz 0x1f7c = 未初始化）
  DYNAMIC    0x50b1d8   → 7 个 DT_NEEDED（libc/libstdc++/libm/liblog/libdl/2 GL）
  GNU_RELRO  0x509a90   → 重定位后锁 .got/.data.rel.ro 只读（ch30 30.1b）
  GNU_STACK  0x0        → 栈 NX
  NOTE       0x174      → .note（build-id）
```

**军棋 vs C/C++/Rust 的关键差异（逆向视角，全用 B.0-B.2 的概念）**：

| 维度 | C so | C++ so | Rust so | **军棋 so（C++ 32 位）** |
|---|---|---|---|---|
| **架构** | x86_64 | x86_64 | x86_64 | **ARM 32 位**（`e_machine=40`，Thumb/ARM 混合指令） |
| **代码（`.text`）** | 15KB 里几百字节 | 几 KB | 4.5MB（std 静态链） | **1MB 明文**（14 个 `On*` + C++ 业务函数，§124 阻塞点） |
| **字符串（`.rodata`）** | 塞 `.data` | 独立 `.rodata` | 独立 `.rodata` | **独立 `.rodata`（118832B，1715 字符串已 dump 到 `frida-dump/`）** |
| **C++ 对象图** | 无 | 有（`__cxa_guard`/vtable） | 无（简单静态） | **有**（`std::vector`/`std::string`，依赖 `libstdc++`，§124 ② 数据与指令对齐对应缺口） |
| **外部符号门牌（`.got`）** | 全 0（无依赖） | 占位（依赖 libstdc++） | 占位 | **占位（214 PLT + 6 GLOB_DAT，B.2.1 的"小区第 N 间房"）** |
| **重定位表（`.rela`）** | 7 条 | ~10 条 | 491+49+3 | **5539 条（RELATIVE 5312 + JUMP 214 + GLOB 6 + ABS 7，B.2.2）** |
| **导出符号（`.dynsym`）** | `c_add` | `_ZN...` mangle | `rust_add` 不 mangle | **14 个 `Java_com_cnvcs_App_On*`（B.2.3，strip 不删）** |
| **反检测** | 无 | 无 | 无 | **8 维度**（基址/JNI/类/fd/uname/时间/Unicorn/Frida，ch27-30） |

**一句话**：军棋跟 C/C++/Rust 小 so 的差异**不在"段布局"（B.0 的 4 类段全同构），在"复杂度 + 架构 + C++ 对象图 + 反检测"**（**1.43MB/1435156B** vs 几百字节，32 位 ARM vs 64 位 x86，有 C++ 对象图 + 8 维度反检测）。**逆向军棋难就难在这个复杂度，不在"看不懂段"**。


## B.5 军棋 so 逆向的"正向生成"知识落点（跟 31 章 + 附录 A 呼应）

| 军棋逆向点 | 正向生成知识（本附录） | 课程落点 |
|---|---|---|
| 军棋 `.got` 重定位（5539 条） | ② 链接生成 .got 占位 + .rel.* 重定位表 | ch06 6.2（重定位全谱系 + 延迟绑定） |
| 军棋 14 个 `On*` 在 `.dynsym` | ② 链接导出符号到 .dynsym（dlopen 按名字找） | ch02 2.2（地址调用）+ B.3.3 |
| 军棋 **原版 so strip**（`.symtab` 删，内部函数无名）；**翻译体 so 没 strip**（含 `g_jqt_*`） | ③ strip 删 .symtab（**原版 strip**，发布版；翻译体是重新编译的，`.symtab` 保留） | B.3.3 + ch04 4.3b（ELF 全字段） |
| 军棋 数据与指令对齐（§124 ②） | ① C++ 编译器生成 `__cxa_guard`/vtable/.data.rel.ro | B.2.2（C++ so 特性） |
| 军棋 32 位 ARM（`e_machine=40`） | ① 编译器 target（ARM vs x86，指令集不同） | B.2.4（军棋 vs C/C++/Rust 对比） |
| 军棋 unidbg 模拟 dlopen | ④ 正向 dlopen 填 .got + 跑 init_array | ch07 7.2（模拟 Linker 加载 12/5312/214/6） |

**"先懂正向生成，再逆向"的价值**：
- **军棋 `.got` 不是"全 0 没数据"**（B.0 纠正）——是"重定位占位"（正向②链接器生成），dlopen 填真值（正向④）
- **军棋原版 so 是 strip 的**（B.3.3 纠正）——内部业务函数无名（strip 删了 `.symtab`），只能反汇编 + 逆向推断功能名；翻译体 so 才没 strip（含 `g_jqt_*`）
- **军棋含 C++**（B.2.2）——"§124 ② 数据与指令对齐对应"的根因（C++ 静态对象/vtable 构造复杂，unidbg 模拟不全）

---

## B.5b "能不能通过 `.got` 符号还原 C 源码？"（分层回答，核心问题）

**结论：不能。`.got` 符号只能还原"调了哪些外部库的哪些函数"，还原不了"军棋自己的 C 源码"。** 分 4 层讲清：

### ① `.got`/`.dynsym` 能还原什么（只到"外部符号名"这一层）

| `.got`/`.dynsym` 能给 | 不能给 |
|---|---|
| "军棋 so 调了 libc 的 `strlen`/`malloc`/`pthread_mutex_lock`，调了 GL 的 `glClear`/`glDrawArrays`，调了 libstdc++ 的 C++ 运行时函数"（**调用了哪些外部库的哪些函数**） | "军棋 so **内部**函数叫什么 / 干什么"（`.got` 只有**外部**符号，**内部函数不在 `.got`**，且原版 strip 后内部函数名全删了） |

**`.got` 是"外部符号的门牌"**（B.1b）——只告诉你"军棋调了外部库的哪些函数"，**不告诉你"军棋自己写了哪些函数 + 函数体逻辑"**。要"还原业务逻辑"得反汇编 `.text`（capstone，不是 `.got`）。

### ② 为什么"还原 C 源码"不可能（4 个根本原因）

| 原因 | 说明 | 军棋现状 |
|---|---|---|
| **符号名 ≠ 源码** | `.got`/`.dynsym` 给"外部函数名"（`strlen`/`glClear`），**不是"军棋源码里的函数名"**（军棋自己的 `judge_move`/`flip_piece` 被 strip 删了，`.got` 里没有）；**就算有名字，名字也不等于源码**（"judge_move"不告诉你"怎么判定走子"） | 原版 so strip 了，内部业务函数全无名 |
| **汇编 ≠ C 源码（编译丢信息）** | C 源码 → 编译 → 汇编 → **反汇编回汇编能 1:1 还原汇编**（军棋 1174/1174 PASS 是"指令语义 1:1"），**但汇编 ≠ C 源码**：编译丢了变量名（`int x` 变 `r0`）、函数名（`judge_move` 变地址 0x39e14）、注释、源文件结构、`#define`/`enum`、循环写法（`for` vs `while` 编译后一样） | 军棋 `thumb2c_full.py` 翻的是"指令级 C"（机械翻译），不是"可读 C 源码" |
| **优化让 C → 汇编多对一** | 同一 C 源码，不同优化级别（-O0/-O2）/不同编译器（gcc/clang）生成不同汇编；**汇编 → C 是多对一**（同一汇编可能来自 10 种 C 写法）→ **不可能唯一还原源码** | 军棋 so 带优化编译（**1.43MB/1435156B**，strip，从 `军棋.apk` 直接抽取），反汇编出的"伪 C"是启发式推断 |
| **C++ 对象图不在 so 里** | 军棋 so 含 C++（`std::vector`/`std::string` + vtable），**对象图是运行时堆指针拓扑，不在 so 文件里**（堆对象运行时才创建）；反汇编还原不了"函数算的对象长什么样" | §124 ② 数据与指令对齐对应缺口，逆不回来（§124z 黑屏根因 = glClear 类对象未构造） |

### ③ "能还原到哪一级"（6 层，跟 `CORE_CHALLENGES.md` 4 层呼应）

| 还原层 | 能从 so 还原吗 | 军棋现状 | 工具 |
|---|---|---|---|
| **① 外部符号名**（调了哪些 libc/GL/C++ 函数） | ✅ 能（`.got`/`.dynsym` 直接给） | `.got` 214 PLT + 6 GLOB_DAT 全读了 | `readelf -r` / `--dyn-syms` |
| **② 内部函数结构**（有哪些函数 / 调用谁） | ✅ 能（反汇编 `.text` + 控制流） | 7428 函数（capstone 反汇编） | `thumb2c_full.py` / IDA |
| **③ 函数指令语义**（函数怎么算，1:1） | ✅ 能（指令级 C 翻译） | 1174/1174 PASS（指令 100% 对齐） | unicorn2 回归 |
| **④ 函数"业务含义"**（"这是走子判定"） | ⚠️ 半能（反汇编 + 逆向推断，**不是从 so 直接读**） | "走子判定 0x39e14"是逆向者起的名 | 人工逆向 + `.rodata` 字符串（`Junqi.Move` 等） |
| **⑤ C 源码（变量名/注释/源文件结构）** | ❌ 不能（编译丢了，多对一） | 还原不了 | — |
| **⑥ 运行时对象图**（C++ 堆指针拓扑） | ❌ 不能（不在 so 里，运行时才建） | §124 ② 数据与指令对齐对应缺口，逆不回来 | 真机 dump（unidbg+Frida 一对一对齐） |

### ④ 跟"系统 so（libc/libc++）"的关系（你说的"其它全是系统的 so"）

**对——军棋只有一个自己的 so（`libjunqi.so`），其它全依赖系统 so**（libc/libc++/libm/liblog/libdl/2 GL）。这些系统 so 的"还原"跟军棋不同：

| so | 能不能还原 C 源码 | 原因 |
|---|---|---|
| **军棋 `libjunqi.so`**（自己的 so） | ❌ ⑤ 层（C 源码）还原不了；✅ ①②③④ 能（外部符号/函数结构/指令语义/业务含义半还原） | strip 了 + 带优化 + C++ 对象图 |
| **系统 so（libc/libc++/libm...）** | ✅ **部分能**——系统 so **通常不 strip 或带调试符号**（Android SDK 里 libc.so 有 `.symtab`），且 libc/libc++ 是**开源的**（AOSP/Bionic/libstdc++ 源码公开） | 源码公开 + 不 strip → "还原 C 源码" = 直接查 AOSP 源码（不是逆向） |

**关键区别**：
- **军棋 so = 私有 so**（strip + 优化 + 源码不公开）→ 逆向只能到 ④ 层（业务含义半还原），⑤⑥ 还原不了
- **系统 so（libc/libc++）= 开源 so**（AOSP/Bionic 源码公开）→ "还原 C 源码" = **查公开源码**（不用逆向，`android-ndk` 里就有 `libc.c`/`libstdc++` 源码）

**所以"通过 `.got` 还原 C 源码"对军棋 so 不成立（私有 strip so），对系统 so 部分成立（开源查源码，不是逆向）**。`.got` 符号只能告诉你"军棋调了 libc 的 `strlen`"，**`strlen` 的 C 源码去哪看？** → 查 AOSP/Bionic 的 `libc/bionic/strings.cpp`（公开），**不是从军棋 so 逆出来的**。

### ⑤ "军棋 so 嵌入 C++ 运行时符号"（B.3.2 发现，跟"系统 so"关系）

原版 so（1.4MB，`/tmp/opencode/jq_orig_check.so` readelf 实测）的 **1479 个 .dynsym 符号**里，**大部分是 libstdc++ 的 C++ 运行时函数**（`std::money_put`/`std::hashtable`/`std::locale`...，`_ZNSt...` mangle 名）——**军棋 so 静态链接了一段 C++ 运行时**（不只动态 `libstdc++.so`，so 里还嵌了 C++ 运行时的部分代码，符号名 `_ZNSt...` 保留）。

**这跟"系统 so"的关系**：
- 军棋 so **既动态依赖 `libstdc++.so`**（`.got` 的 GLOB_DAT/PLT 填 `libstdc++` 的符号），**又静态嵌了一段 C++ 运行时**（`_ZNSt...` 符号在军棋 so 的 `.text` 里）
- "静态嵌 C++ 运行时" = 编译时 `g++` 把部分 `std::` 代码直接链进军棋 so（不是全动态 `libstdc++`）
- **逆向军棋 so 时**：`_ZNSt...` 符号（C++ 运行时）**跟 AOSP 的 libstdc++ 源码对应**（公开，可查），**军棋自己的业务函数**（strip 删了名）**只能反汇编 + 推断**

---

## B.5c "逆 so 还原源码"的两条路线 + 6 步（用户方法论，总纲）

**逆 so 还原 C 源码，有两条独立路线，最终"真机对比"比的是"行为"（不是源码）**。用户提出的完整方法论：

```
┌─ 路线 1：外部依赖还原（.got/.dynsym → 标准库）
│   "军棋纯依赖 C/C++ 标准库（7 个系统 so），这部分可还原"
│
└─ 路线 2：内部函数调用链（反汇编 → 识别 → 补全 → stub → 真机对比）
    "反汇编 → 识别函数/struct/class/常量 → 补标准库修正 → unidbg 补调用链/地址或符号调用 → 补完 stub → 真机对比"
```

### B.5c.1 路线 1：外部依赖还原（军棋 = 纯依赖标准库）

军棋外部依赖**全在 7 个系统 so**（`libc/libstdc++/libm/liblog/libdl/libGLESv1_CM/libGLESv2`），**全是公开语言的 C/C++ 标准库**（不是私有第三方库）。"可还原"精确分 3 层：

| 层 | 能不能还原 | 军棋现状 | 工具 |
|---|---|---|---|
| **调了哪些外部函数**（`strlen`/`glClear`/`std::vector`） | ✅ 能（`.got`/`.dynsym` 直接给） | 214 PLT + 6 GLOB_DAT 全读 | `readelf -r`/`--dyn-syms` |
| **外部函数 C 实现**（`strlen` 怎么写的） | ✅ 能查（标准库公开，AOSP/Bionic/libstdc++） | 查公开源码（**不是逆向**） | 查 AOSP |
| **军棋源码里"调用点"写法**（`int n = strlen(s)`） | ❌ 不能（调用点在 `.text`，得反汇编） | 反汇编 `.text` 拿调用关系 | capstone |

**"外部依赖可还原"精确表述**：能还原到"军棋用了标准库的哪些函数"（①层，`.got`/`.dynsym`）；标准库本身怎么实现**查公开源码**（不是逆）；但"军棋源码里怎么调的"在 `.text`（反汇编，不是 `.got` 给的）。

### B.5c.2 路线 2：内部函数调用链 6 步（每步卡点）

```
反汇编 .text ─► 识别函数/struct/class/常量 ─► 补标准库修正 ─► unidbg 补内部调用链 ─► 补完 stub ─► 真机对比
   │                │                          │                     │                  │                │
 ✅ 能做          ⚠️ 函数能识别            ✅ 能做          ⚠️ 补不全        ⚠️ 补不完        ✅ 能做
 (capstone)      class/struct 难          (查公开源码)      (3 根因)       (296 stub+对象图)   (对比行为)
```

**逐卡点（军棋现状）**：

| 步骤 | 能做吗 | 卡点（军棋） |
|---|---|---|
| **反汇编 `.text`** | ✅ | 7428 函数全反汇编（capstone）；但**原版 strip 了，反汇编出的是"机器码 + 寄存器"，没函数/变量名** |
| **识别函数/struct/class/常量** | ⚠️ | **函数边界**能识别（7428）；**常量**`.rodata` 明文（1715 字符串已 dump）；**struct/class 难**（`std::vector`/`vtable` 是"偏移 + 指针"，**对象图在运行时堆，不在 `.so` 里**，§124 ② 根因）；**class vtable** 能识别 `.data.rel.ro` 段，但**指向的函数名 strip 后无名** |
| **补标准库修正** | ✅ | 军棋外部依赖 = C/C++ 标准库（全公开）= 查 AOSP/Bionic/libstdc++（不用逆向）；unidbg 用 `AndroidResolver(23)` 自带 5 系统库 + 2 GL 挂桩 |
| **unidbg 补内部调用链** | ⚠️ | 你说的"unidbg 补全 / 地址或符号调用逐个补逻辑"——方向对，但**补不全，3 根因卡**：①上下文相关 BL 解码 bug（unicorn2，`0x7965a`→`0x43ed8` vs `0x6eaea`）②数据与指令对齐对应缺口（`std::vector` 0x43ed8 / §124z 黑屏 glClear 类对象未构造）③288MB `PROT_NONE` `mmap`（`mmap2` mock 失败）。**"符号调用"对原版 so 不成立**（strip 了内部函数无名，**只能"地址调用"**；JNI 入口 14 个有符号，内部 0x39e14 只能按地址） |
| **补完所有 stub** | ⚠️ | 军棋现状 **1334 完整 + 296 stub**（§124z-5）；3698 调用边 = **3604 完整（97.5%）/ 87 stub（2.4%）/ 7 缺失（0.2%）**；**"补完"做不到**——296 stub 里**296 存根 + 运行时对齐卡点**（§根因②）**补不完**（stub 不是"函数体没翻译"，是"翻译了但依赖的 C++ 对象未构造"） |
| **真机对比** | ✅ | **对比的不是"源码"（还原不了 C 源码），是"行为"**：①渲染出帧（§124z 黑屏 vs 真机能出帧）②走子逻辑（`Junqi.Move`）③对象图（glClear 类对象）——**不是"逐行对比 C 源码"** |

### B.5c.3 "逆源码到哪个层级"总表（B.5b 的 6 层，补"路线"维度）

| 还原层 | 路线 1（外部依赖） | 路线 2（内部调用链） | 军棋现状 |
|---|---|---|---|
| **① 外部符号名**（调了哪些标准库函数） | ✅ `.got`/`.dynsym` 直接给 | — | 已做（214 PLT + 6 GLOB_DAT） |
| **② 内部函数结构**（有哪些函数 / 调用谁） | — | ✅ 反汇编 `.text` + 控制流 | 7428 函数（capstone） |
| **③ 函数指令语义**（怎么算，1:1） | — | ✅ 反汇编 + unidbg 回归（**capstone**） | 1174/1174 PASS |
| **④ 函数"业务含义"**（"这是走子判定"） | — | ⚠️ 半能（**IDA**：自动起名 `Sub_`/`loc_`/`v1` + 控制流图 + 伪 C，名字是 IDA 猜的 + 手动改；**不是 so 直接读**） | "走子判定 0x243E0"是逆向起的名（capstone 只到 ③ 层"机器码 + 寄存器"，IDA 到 ④ 层"带名字的结构化伪代码"） |
| **⑤ C 源码**（变量名/注释/源文件结构） | 标准库 ✅ 查公开源码 | ❌ 不能（编译丢了，多对一） | 军棋私有 so ⑤ 逆不了；标准库 ⑤ 查 AOSP |
| **⑥ 运行时对象图**（`std::vector` 堆指针拓扑） | — | ❌ 不能（不在 so 里，运行时才建） | §124 ② 数据与指令对齐对应缺口（§124z 黑屏根因） |

### B.5c.3b "反汇编 .so 出源码 + 结合 C++ 标准库 + 其它段信息"（用户方法论总结，R-route 落地）

**用户方法论（4 块）**：军棋可以**反汇编 `.text` 出③层指令语义 + 结合 C++ 标准库（⑤ 标准库部分查 AOSP 源码）+ `.rodata`/`.data.rel.ro`/`.got` 其它段信息（④ 业务含义）**，**出"③④ 层行为级等价 C 源码"**。4 块各自能逆到哪层：

| 4 块 | 用什么 | 能逆到哪层 | 军棋实测 |
|---|---|---|---|
| **① 反汇编 `.text`** | capstone | **③ 指令语义 1:1**（寄存器级 C） | 7428 函数，1174/1174 PASS |
| **② 结合 C++ 标准库** | 查 AOSP/libstdc++ 公开源码 | **⑤ 层"标准库部分"**（查源码，不是逆） | `.got` 214 PLT + 6 GLOB_DAT 全调 libstdc++/libc/GL（libstdc++ 开源可查） |
| **③ `.so` 其它段信息** | `.rodata` 字符串 / `.data.rel.ro` vtable / `.got` 外部调用 | **④ 业务含义**（字符串 + vtable + 调用关系佐证，起业务名） | 1715 字符串（`Junqi.Move` 等已 dump） |
| **④ "出源码"** | 上面 3 块合起来重写 | **③④ 行为级等价 C/C++**（**不是 ⑤ C 源码原貌**） | R-route 重写（`rev64/forward/src_rs/`） |

**"出源码"精确表述**：出的是 **③④ 层行为级等价 C 源码**（走子/翻棋/吃子/悔棋/炸弹的规则 C 逻辑），**不是 ⑤ 层 C 源码原貌**（变量名/注释/优化差异/源文件结构，编译单向性逆不出）。**"结合 C++ 标准库"的作用**：军棋 so 依赖 `libstdc++.so`（`.got` 门牌全调它），**这些外部函数的 C 实现查 AOSP/libstdc++ 公开源码**（不是逆），把"军棋调 `std::vector::push_back`"对齐到"libstdc++ 怎么实现的"——**这是 ⑤ 层"标准库部分"能逆的**（查源码，不是逆军棋私有 so）；**军棋私有 so 自己的业务函数（`.text` 里）逆不出 ⑤ 层**（strip + 优化）。

**军棋 R-route 重写（AGENTS.md 新增红线，用户方法论 4 块的落地，`rev64/forward/src_rs/` 5549 行 C）**：

| 模块 | 文件 | 规模 | 干什么 |
|---|---|---|---|
| **规则引擎** | `board.c`/`rules.c`/`layout.c`/`save.c` | ~610 行 | 走子/吃子/炸弹/悔棋/翻棋/12 布局/one.sjg 存档（对齐原版 0x39e14/0x39f20/0x2a228/0x6a3b0/0x302b4，capstone 反汇编） |
| **触摸/状态机** | `touch.c`/`touch_state.c`/`ui_state.c`/`ui_menu.c` | ~300 行 | 两步走子态机（对齐原版 0xbdd2c/0x7bf80）+ UI_MENU↔UI_GAME + 菜单 5 按钮 |
| **渲染/GL** | `render.c`/`render_obj.c`/`render_scene.c`/`glview_gl.c`/`png_decode.c` | ~2000 行 | GLES1 固定管线 + SPAK 纹理 + vtable 对象图 + GLView EGL 线程出帧 |
| **JNI 桥** | `jni_bridge.c` | 1303 行 | 14 个 On*（签名与 App.smali 一致）+ 音效（Java MediaPlayer）+ 蓝牙 v2 帧 + 触摸分流 |
| **蓝牙/音频** | `bt_protocol.c`/`bt_session.c`/`audio.c` | ~300 行 | v2 7 字段帧 + 帧状态机（7 种帧型 + 坐标校验 + jq_try_move）+ 14 WAV |

**产物（实测）**：
- `dist/jq_run`（桌面 headless 走一局）+ `dist/libjunqi_rs.so`（**NDK arm64，~91-95KB**（随 evverify/事件夹具等新增演进，2026-11-04 实测 94824B），14 个 On*）
- `dist/phase_rs_a64.apk`（**~40.8KB 64 位**（rs168_wav.apk 资产固化版含 14 WAV + SPAK 纹理后实测 ~6.9MB，"40.8KB" 是早期无资产版），双 ABI，v1+v2 签名，`com.cnvcs.junqirs` 包名）
- **回归 `src_rs_regress.sh` 一条命令 9 项全绿**：编译 / 蓝牙自测 / 桌面走局 / 规则回归 N 局 / 红线审计 / 链接审计 / 布局加载 / 触摸命中 / 完整对局自测（蓝 vs 红贪心 AI 60 局，全路径覆盖胜负/和棋/困毙）

**真机（MuMu 64 位）实证闭环（AGENTS.md 沉淀）**：
- **走子**：EAT（rank3 吃 rank8）+ BOMB（同级同归于尽）+ MOVE 真机实证，`Junqi.Move`/`Junqi.Eat`/`Junqi.Bomb` 事件全对齐 `.rodata`
- **音效**：MediaPlayer 播放链全通（14 WAV 落地 MuMu），按事件分类（EV_EAT→eat / EV_BOMB→bomb / ...）
- **AI 应着**：红方贪心 AI 主线程同步（防 MuMu 停帧），不主动送死（sc 表：吃子 700-1000 > 同级/炸弹 > 空移 2 > 送死 1）
- **诚实边界**：GL 出屏画面需真机 arm64 GPU 验收（MuMu PBuffer 离屏出像素已证，window surface 通路未通）；音效真机出声待验；蓝牙 SPP socket IO 由 Java 宿主承担，无真机蓝牙验证

**CONSISTENCY.md 的"一致测试口径"（B.5b 还原分层的军棋实证落地，教科书级诚实边界）**：
```
| 层 | 方法 | 状态 |
|---|---|---|
| ① 布局字节静态对照 | 原版 data_lay_default.lay 30B vs 重写 JQ_DEFAULT_LAY | ✅ 逐字节一致（rank 分布 {0:5,1:1,...}） |
| ② 重写侧规则自洽 | 固定基准序列 → 事件码 + 终局态 | ✅ 9 行逐行一致（翻棋/走子/非法/悔棋/认输全事件面） |
| ③ 原版侧走子序列对拍 | unidbg 驱动原版 OnEvent("move",...) | ❌ 驱动不了（下，4 函数全崩） |
| ④ 渲染/音频/蓝牙 | 原版数据与指令对齐缺口 + 重写桩 | ❌ 两边都非"真渲染"，不纳入可比面（红线） |
```

**unidbg 驱动不了原版走子链的 4 个函数根因（§124 三项阻塞的函数级展开）**：

| 函数 | 作用 | unidbg 裸调 | 根因 |
|---|---|---|---|
| `0x39f20` 走子执行器 | 执行走子 | ❌ 崩 | 读全局单例 `0x121f4a`（init_array 构造，unidbx 跑不通） |
| `0x32a2c` 合法判定 | 走子判定 | ❌ 崩 | `ldr r3,[r0]; blx vtable[8/4/0xc/0x10]`（vtable 对象图未构造） |
| `0x2a340` 布局载入 | 读 lay | ❌ 崩 | 写 state +0x78 计数表（布局容器对象） |
| `0x39e14` 目标判定 | rank 比较 | ❌ 崩 | 读 `.rodata` 高熵解密态（0x3e5bc→0x11cafe 索引表） |

**"R-route 已落地 ①②③④"的精确含义（更新版）**：
- ✅ **① 外部符号**：14 个 On* 对齐 `.dynsym` + 7 依赖 `.got` 全读
- ✅ **② 内部函数结构**：规则引擎 + 触摸态机 + 渲染链（capstone 反汇编 0x39e14 等 5 函数）
- ✅ **③ 指令语义**：规则 40/40 全绿 + 12/12 回归全绿 + 60 局 AI 对弈合法终局
- ✅ **④ 业务含义**：走子/吃子/炸弹/悔棋/翻棋事件对齐 `.rodata`（Junqi.Move/Eat/Bomb...）
- ❌ **⑤ C 源码原貌**：逆不出（编译单向性）——但**"行为级等价 C 源码"已达成**（5549 行，可读、能编译、能跑、能回归，**不是"还原了原始 C"，是"重写的行为等价 C"**）
- ⚠️ **真机"能玩"验收**：GL 出屏/音效出声/蓝牙联机需真机 arm64 GPU + 音频 + 蓝牙设备（MuMu 替代不了）

**一句话（更新版）**：**"反汇编 .so 出源码 + 结合 C++ 标准库 + 其它段信息" = 出 ③④ 层行为级等价 C 源码（军棋 R-route 已做：5549 行 + 12/12 回归全绿 + 真机走子/音效实证，CONSISTENCY.md 口径），不出 ⑤ 层 C 源码原貌（编译单向性），"能玩"需真机 GPU/音频/蓝牙验收**

### B.5c.4 一句话结论（"源码是不是差不多了"）

**不是"差不多了"，是"到不了 C 源码"**。路线 1 + 路线 2 合起来能到 **①②③（外部符号 + 内部函数结构 + 指令语义 1:1）**，**④ 半还原**（业务含义靠推断），**⑤⑥ 逆不了**（C 源码 + 对象图）。**"真机对比"对比的是"行为"（③④），不是"源码"（⑤）**。

**"逆 so 还原"的现实目标 = 行为级等价（指令 1:1 + 业务含义半还原），不是源码级还原**：
- ✅ 军棋能做到：外部依赖清单（①）+ 内部函数指令语义（③，1174/1174）+ 走子规则业务含义（④，`Junqi.Move` 等 `.rodata` 字符串 + 反汇编推断）
- ❌ 军棋做不到：可读 C 源码（⑤，编译丢变量名/注释/优化差异，多对一）+ 运行时对象图（⑥，`std::vector` 堆指针拓扑，不在 so 里）
- **真机验证"能玩" = 行为级等价（③④），不是"还原了 C 源码"（⑤⑥）**

### B.5c.5 真实例子：军棋 `.text` 反汇编（"机器码 + 寄存器，不是函数/变量名"具象化）

**军棋 `.text` 里一个普通内部函数（虚拟地址 `0x243E0`，strip 后**没函数名**，capstone 真反汇编）**：

```
0x0243E0:  b        #0x247b6        ← 只有地址，没有 "judge_move" 这种函数名（strip 删了 .symtab）
0x0243E2:  ldr      r3, [pc, #0xf8]  ← 只有寄存器 r3，没有 "piece" 这种变量名（编译丢了）
0x0243E6:  ldr      r3, [r4, r3]
0x0243EA:  str      r6, [r3]
0x0243EE:  cmp      r3, #0           ← "r3 跟 0 比"，不知道 r3 是"棋子合法性标志"（要人推断）
0x0243F0:  bne      #0x243f4         ← "r3≠0 就跳"，不知道这是"非法走子就 return"（要人推断）
```

**对比军棋 JNI 入口（`.dynsym` 保留名字，`0x0233F5`）**：

| | JNI 入口（`OnKey`，有名） | 内部函数（`0x243E0`，无名） |
|---|---|---|
| **函数名** | ✅ `Java_com_cnvcs_App_OnKey`（`.dynsym` 保留） | ❌ **没**（strip 删了 `.symtab`，只剩地址 `0x243E0`） |
| **变量名** | ❌ **没**（编译丢，`r0`/`r1` 是寄存器） | ❌ **没**（同左，`r3`/`r5` 是寄存器） |
| **机器码语义（③ 层）** | ✅ capstone 直接给（`bl`/`ldr` 等） | ✅ 同左（**军棋 1174/1174 PASS 就是 ③ 层**） |
| **业务含义（④ 层）** | ⚠️ "OnKey = 按键事件"（Java 层传参，能推断） | ⚠️ "这是走子判定"（**逆向者起的名**，`Junqi.Move` 字符串佐证，但 `0x243E0` 就是走子判定要靠控制流 + 字符串交叉推断） |

**"机器码 + 寄存器" vs "函数/变量名"的精确区别**（③ vs ④ 层分界）：

| 层 | 反汇编/逆向能给 | 例子（军棋 `0x243E0`） |
|---|---|---|
| **③ 指令语义**（1:1 机器码） | ✅ 自动给（capstone） | `ldr r3,[r4,r3]` = "把 r4+r3 地址的值读进 r3"（**机器码语义，capstone 直接解**） |
| **④ 业务含义**（"这是走子判定"） | ⚠️ 半能（**人要推断**，so 没直接写） | "r3≠0 就 return" → **推断**"非法走子就 return"（**人看的**，so 里没写"走子判定"这个名） |

**关键**：**"机器码 + 寄存器"是 ③ 层（capstone 自动给，军棋 1174/1174 PASS）**；**"函数/变量名 + 业务含义"是 ④ 层（人推断，军棋"走子判定"是逆向起的名）**。**strip 让内部函数连 ④ 层的"函数名"都没了**（只剩地址，`0x243E0` 不知道是什么函数，要控制流 + 字符串交叉推断）。

### B.5c.5b capstone（③ 层）vs IDA（④ 层）：同一代码两种产物

上面 B.5c.5 举的是 **capstone 产物**（纯"机器码 + 寄存器"，③ 层）。**同一段代码（`0x243E0`），IDA 反汇编会多 3 样东西**——这是"③ 层（capstone）vs ④ 层（IDA）"的精确分界：

**IDA 反汇编同一段（`0x243E0`，模拟 IDA 真实产物形态）**：

```
0x0243E0   BL      Sub_247B6      ; ★ IDA 给"无名函数"自动起名：Sub_247B6（按地址）
0x0243E2   LDR     R3, [PC+#0xF8]
0x0243E6   LDR     R3, [R4,R3]
0x0243EA   STR     R6, [R3]
0x0243EE   CMP     R3, #0
0x0243F0   B.NE    loc_243F4      ; ★ IDA 给"标签"自动起名：loc_243F4（按地址）
```

**capstone vs IDA 精确对照**：

| 维度 | capstone（③ 层，你贴的） | IDA（④ 层） |
|---|---|---|
| **函数名** | `bl #0x247b6`（**只有地址，无名字**） | `BL Sub_247B6`（**IDA 自动起名 `Sub_`+地址**，可手动改成 `judge_move`） |
| **标签** | `bne #0x243f4`（**只有地址**） | `B.NE loc_243F4`（**IDA 自动起名 `loc_`+地址**） |
| **寄存器** | `r3`/`r4`/`r6`（**纯寄存器**） | `r3`/`r4`/`r6`（IDA 先用寄存器，**手动重命名后变 `piece`/`board`**） |
| **控制流** | 平铺指令（`cmp`+`bne`） | **IDA 画 if/else/while 结构图**（`if (v1 != 0)`） |
| **数据引用** | 无 | **IDA 跟踪 `[pc,#0xf8]` 指向的常量池**，标注"读 0x5Cxxxx 的数据" |

**IDA 的"伪代码"（按 F5，模拟真实产物）**：

```c
int Sub_243E0()          // ← IDA 按地址起名（源码真名 strip 删了，`Sub_243E0` 是猜的）
{
    int v1;             // ← IDA 推断的局部变量（"v1"是 IDA 按出现顺序起的，不是源码变量名）
    Sub_247B6();        // ← IDA 识别出这是函数调用（capstone 只看到 bl #0x247b6）
    v1 = *word_5Cxxxx; // ← IDA 把 [pc,#0xf8] 还原成"读全局变量 word_5Cxxxx"
    if ( v1 )          // ← IDA 把 cmp r3,#0 + bne 还原成 if
    {
        Sub_243F4();
    }
    return 0;
}
```

**"IDA 伪代码 ≠ 源码"的 3 个区别**（④ 层 vs ⑤ 层）：

| IDA 伪代码 | 真 C 源码 | 区别（为什么 IDA 逆不出 ⑤ 层） |
|---|---|---|
| `int v1`（**IDA 自动起局部变量名**） | `int isLegal`（**源码变量名**） | **`v1` 是 IDA 猜的（按出现顺序 v1/v2/v3），`isLegal` 是源码里的，strip + 优化后逆不出** |
| `if ( v1 )`（**IDA 还原控制流**） | `if (piece.isValid())`（**源码写法**） | **IDA 还原出"有 if"，但"if 里是 piece.isValid() 还是 legalCheck"逆不出（优化丢了写法，多对一）** |
| `Sub_247B6`（**按地址起名**） | `judge_move`（**源码函数名**） | **`Sub_247B6` 是 IDA 按地址起的，`judge_move` 是源码名，strip 删了，逆不出** |

**军棋现状（③ 层已做，④ 层可推进，⑤ 层逆不出）**：
- **③ capstone**（军棋已做）：`thumb2c_full.py` 反汇编 7428 函数（**机器码 + 寄存器**，1174/1174 PASS，**就是你贴的那种**）
- **④ IDA**（**军棋可推进**）：IDA 自动起名字（`Sub_`/`loc_`/`v1`）+ 控制流图 + 伪 C，**把"机器码"升级成"带名字的结构化伪代码"**（比 capstone 可读 10 倍，但**名字是 IDA 猜的**，要人手动改成业务名）
- **⑤ 源码**（**逆不出**）：strip + 优化 + C++ 对象图，**编译单向性**（`v1`/`judge_move`/`piece.isValid()` 都逆不回源码真名）

**"capstone（③）→ IDA（④）→ 源码（⑤）"三步精确关系**：

```
③ capstone（军棋已做）
   机器码 + 寄存器（bl #0x247b6 / r3/r4/r6，无名字）
   1174/1174 PASS（指令语义 1:1）
        ↓  升级（工具换 IDA）
④ IDA（军棋可推进）
   带名字的结构化伪代码（BL Sub_247B6 / loc_243F4 / if(v1)，名字是 IDA 猜的 + 手动改）
   控制流图（if/while）+ 数据引用跟踪
        ↓  逆不出（编译单向性）
⑤ 源码（逆不出）
   源码真名（judge_move / isLegal / piece.isValid()）+ 注释 + 源文件结构
   strip 删了函数名 + 优化丢了写法（多对一）
```

**核心**：**capstone 给"机器码"（③ 层，自动）**；**IDA 给"带名字的结构化伪代码"（④ 层，名字是 IDA 猜的 + 你手动改）**；**源码（⑤ 层）逆不出**（strip + 优化 + 对象图）。**"逆 so 还原"的现实终点 = ④ 层（IDA 伪代码 + 手动业务名），不是 ⑤ 层（C 源码）**。

### B.5c.5d Rust 例子（函数 vs 对象分离 + 用户纠正"常量在 so 里"的精确边界）

用 Rust 极简游戏 so 举例（比 C++ `std::vector` 更清晰）：

```rust
// rust_game.rs —— 模拟军棋的"函数 + 依赖对象"
#[no_mangle]
pub extern "C" fn start_game() {
    let mut board: Vec<Piece> = Vec::new();   // 对象：堆上的 Vec<Piece>
    board.push(make_piece(RED, 0, 0));         // 函数体：push
    board.push(make_piece(BLUE, 3, 3));
    let winner = check_winner(&board);
    println!("winner = {}", winner);
}
#[no_mangle]
pub extern "C" fn check_winner(board: &Vec<Piece>) -> u32 {
    let mut red = 0;
    for p in board.iter() { if p.side == RED { red += 1; } }
    if red == 0 { 0 } else { 1 }
}
fn make_piece(side: u32, x: u32, y: u32) -> Piece { Piece { side, x, y } }
struct Piece { side: u32, x: u32, y: u32 }
```

**"函数补齐 vs 对象补齐"的精确边界（用户纠正：常量/指令在 so 里，运行时快照不在 so 里）**：

| 东西 | 在 so 里吗 | 在哪 | 说明 |
|---|---|---|---|
| **`make_piece(RED,0,0)` 的机器码** | ✅ **有** | `.text` | `mov r0,#RED`/`bl make_piece` 这种指令，反汇编直接看（**用户点出：这是 so 里的指令，不是我编造的**） |
| **`RED`/`BLUE`/`0`/`3` 常量** | ✅ **有** | `.rodata` | 静态常量池，编译期写死（**用户点出：这些静态常量在 so 段里**） |
| **`vec_push` 的机器码** | ✅ **有** | `.text` | "往 Vec 末尾写元素 + 移 end 指针"的指令（**so 里有的**） |
| **`Vec` 布局（begin/end/cap 三指针）** | ✅ **有** | `.text` 里"怎么访问三指针"的指令 | 反汇编能推结构体布局 |
| **`start_game`/`check_winner` 函数体** | ✅ **有** | `.text` | 指令 1:1（军棋 1630 函数全补的就是这个） |
| **`board` 指向的堆地址**（`0xea58feb0`） | ⚠️ 分路线 | 运行时 `malloc` 返回（ASLR 每次不同） | **逆源码路线：重写后跑起来 `malloc` 自己返回，不用逆**；**模拟路线：要喂真值给 unidbg**（别混，见下） |
| **堆里"实际 push 了几个 Piece"**（`end-begin=2`） | ⚠️ 分路线 | 运行时 push 次数才成立 | **逆源码路线：重写后 `push` 自己跑，不用逆**；**模拟路线：要喂** |
| **堆里每个槽"实际存的是 `RED/0/0` 还是 `BLUE/3/3`"** | ⚠️ 分路线 | 运行时 push 顺序决定 | **`.rodata` 有 `RED/0/0` 常量（用户点出，对）**；"board[0] 槽里实际写了什么"**逆源码路线不用逆（跑起来自然填），模拟路线要喂** |

**★ 用户纠正（关键，别混两条路线）**：我前几轮把"运行时堆快照"列成"so 里没有的缺口"——**错在把"模拟路线的缺口"安到了"逆源码路线"头上**。两条路线：

| 路线 | 堆地址/快照是缺口吗 | 说明 |
|---|---|---|
| **逆源码路线**（用户："逻辑补齐了就能正常开发"） | ❌ **不是缺口** | 逆出 `Vec::new()`+`push` 的逻辑，**重写成能编译的 C/Rust，跑起来堆自己分配**（`malloc`/`Vec` 自己跑），**根本不用"逆向堆地址"**。"要毛线运行时堆分配"（用户原话，对） |
| **模拟路线**（unidbg 不重编译，直接模拟原版 so 跑） | ✅ **是缺口** | 不重编译，模拟原版 so 的 `malloc` 返回值 + 数据与指令对齐（运行时值），unidbg 模拟不全 → §124z 黑屏（**"模拟没模拟对"，不是"逆不出来"**） |

**"函数补齐 vs 对象没补齐"的精确含义（按两条路线重述）**：
- **"函数补齐"** = 把 `.text` 里的机器码（`make_piece`/`vec_push`/`start_game`）+ `.rodata` 常量（`RED/0/0`）翻成 C → ✅ so 里有（指令 + 常量都在段里），能补（**逆源码路线全用得上**）
- **"对象没补齐"只在模拟路线成立** = unidbg 不重编译直接模拟原版 so，**模拟不到运行时的堆地址/数据与指令对齐** → 黑屏（§124z）；**逆源码路线不存在"对象没补齐"**——重写后程序跑起来堆自己填
- **区分关键**：**"构造逻辑"（怎么 new/push，so 里有，指令级，逆源码路线全有）** vs **"运行时快照"（new/push 完堆里实际长什么样，**只有模拟路线要**；逆源码路线重写后跑起来自然填，不用逆**）

**军棋 §124z 黑屏的精确含义（按用户"逻辑补齐就能开发，堆分配不是缺口"的标准重述）**：
- **渲染函数（glClear 类）补齐了** = `.text` 里"怎么调 glClear" + "怎么构造渲染器对象"的**指令（so 里有，反汇编看得清）**
- **卡点不是"堆地址逆不出"（那是假问题，用户纠正）**——堆地址是程序跑起来 `malloc` 自然返回的，**逆源码路线里根本不是缺口**
- **真卡点 = "模拟（unidbg）跑起来后，渲染器对象在运行时没被正确构造"**（**模拟路线的 ④ 层行为分歧**，不是"逆不出来"）：
  - 军棋翻译体（`jqt_mem.c` 影子运行时）= 模拟原版 so 跑的"机械翻译寄存器级 C"（③ 层指令 1:1）
  - **模拟跑起来时**，渲染器对象构造链（`0x23eec` 那条）因**运行时对齐分歧**（`this` 传错/寄存器污染）**没走通** → 渲染器没构造 → glClear dispatch 不到 → 黑屏
  - **这是"模拟的逻辑跑起来没跑对"（③ 层指令 1:1 但 ④ 层行为没对上），不是"堆地址逆不出"（逆源码路线里不存在）**
- **怎么补（分路线）**：
  - **逆源码路线**：重写逻辑，**跑起来堆自己填，不用补对象图**（用户路线，不被黑屏卡住）
  - **模拟路线**：修"模拟跑起来的行为"（让渲染器构造链走通）；Frida dump 的是**"运行时对象图真值"**（对象实际构造状态，glClear 类对象的 vtable/字段），**喂给 unidbg 模拟**——**跟"堆地址逆不出"不同（堆地址是程序自己跑的，逆源码路线不用管）**

**一句话（两条路线别混）**：
- **逆源码路线**（用户："逻辑补齐就能正常开发"）：so 里有 = 函数体 + 静态常量 + 结构体布局 + 调用关系（反汇编 1:1）；**堆地址/快照不是缺口**（重写后程序跑起来 `malloc`/`push` 自己填，"要毛线运行时堆分配"）；唯一逆不出的是"变量名/函数名的源码原貌"（⑤ 层，不影响开发，起个名就行）
- **模拟路线**（unidbg 直接模拟原版 so）：so 里有指令 1:1，但**模拟不到运行时的堆地址/数据与指令对齐** → §124z 黑屏（**"模拟没模拟对"，不是"逆不出来"**）；Frida dump 对象图真值是**喂给模拟用的**，不是"逆向缺口"
- **军棋"能跑进程/渲染 UI"（③ 层指令 1:1，模拟路线）但"GL 棋盘黑屏"（模拟路线 ④ 层行为分歧）**——**逆源码路线不被黑屏卡住（逻辑逆出来重写开发，堆自己填）**

### B.5c.5c "IDA 伪代码能不能跑"（用户追问：上面那段伪代码编译得了吗）

**不能直接编译跑。** 3 个原因 + 改完能跑的代价：

| 原因 | 说明 |
|---|---|
| **`word_5Cxxxx` 是占位符 + IDA 命名** | `word_` 前缀是 IDA 对全局变量的命名习惯（不是 C 标准标识符），`xxxx` 是 IDA 没还原的常量（我写的示例占位）；编译要手动改成真变量（`int *p = (int*)0x5Cxxxx;`） |
| **`Sub_247B6()` 是按地址起的函数名** | `Sub_` 是 IDA 按地址起的（不是源码函数名），**编译时要声明原型 + 真身是 ARM 机器码**（不是 C 函数），C 编译器认不了"函数体" |
| **`v1`/`if(v1)` 是 IDA 推断的语义，不是真 C 逻辑** | `v1` 的真语义（"棋子合法性标志"）编译丢了（多对一），`if(v1)` 还原出"有 if"但**还原不出 if 里的真逻辑**（优化丢了写法） |

**"改完能跑"的代价**（把伪代码改成真 C）：

```c
// IDA 伪代码（直接编译不了）：
int Sub_243E0() { int v1; Sub_247B6(); v1=*word_5Cxxxx; if(v1) Sub_243F4(); }

// 改成"能跑的 C"（但那是"把 ARM 地址硬塞进 C"，不是"还原源码"）：
extern int Sub_247B6();                       // 要手动声明原型
extern int Sub_243F4();
int Sub_243E0() {
    int v1;
    Sub_247B6();                               // 真身是 ARM 机器码（要 .so 链接）
    v1 = *(int*)0x5Cxxxx;                     // 按地址读全局（不是真 C 变量）
    if (v1) Sub_243F4();
    return 0;
}
```

**关键**：**"改完能跑"的 C 还是 ④ 层（带名字的机器码调用），不是 ⑤ 层（还原的源码）**——`Sub_247B6`/`0x5Cxxxx`/`v1` 全是"按地址/IDA 猜的"，**不是源码里的函数名/变量名/写法**。

**"逆出来能跑的" 4 条路径对照**（军棋现状）：

| 路径 | 能不能跑 | 跑的是什么 | 军棋现状 |
|---|---|---|---|
| **A. IDA 伪代码直接编译** | ❌ 不能 | `word_`/`Sub_` 是 IDA 命名 + 占位符 | 要手动改（改完 ④ 层，不是 ⑤ 层） |
| **B. capstone 指令级 C** | ✅ 能跑 | "机器码 1:1 翻成 C"（`r3`/`r4` 寄存器直接用） | **军棋 1174/1174 PASS**（③ 层，"寄存器级 C"不是"可读源码"） |
| **C. 翻译体 so** | ✅ 能跑 | "指令 1:1"编译成真 so（arm32/arm64） | **军棋 §124u-z**（`libjunqi_arm32_aligned.so`，MuMu 渲染主菜单 UI，GL 棋盘黑屏） |
| **D. 简化重写 C++** | ✅ 能跑 | "手写的行为级等价"（不是"逆出来的"） | **军棋 arm64 `junqi.cpp`**（97KB，10×9 棋盘 + 走子规则，**简化版非完整版**） |

**"能跑"的诚实边界**（AGENTS.md 红线"别包装成完整版"）：
- **③ 层"指令 1:1 能跑"≠ ⑤ 层"还原源码"**（B 路径，军棋 1174/1174 PASS 是"构造输入下指令语义一致"，不是"任意输入/真实游戏流程一致"）
- **④ 层"业务含义能跑"= 行为级等价**（C/D 路径，军棋翻译体渲染主菜单 UI，**GL 棋盘未出帧 + 进对局路径未通**（§124z 黑屏），简化重写**丢了 38 子布局/地雷/悔棋/蓝牙**（简化版））
- **"军棋能玩"的真机验证还没达成**：32 位真机（原版 so ✅ 完整版）/ MuMu（翻译体 ⚠️ 主菜单 UI）/ 64 位真机（简化重写 ⚠️ 能渲染但非完整版）

**一句话**：**"逆出来能跑的" = ③ 层指令 1:1（能跑，但是"寄存器级 C"/"翻译体 so"，不是"还原源码"）+ ④ 层行为级等价（能跑，但简化版/黑屏，不是"完整版能玩"）**；**⑤ 层（C 源码）逆不出，"还原的 C 源码能跑"是假的（编译不了 + 逆不出）**。

### B.5c.6 "unidbg 干不了，Frida 真机能干"（用户反驳 + 落点）

unidbg 的 3 根因（§124 天花板），**Frida 真机确实能突破 2-3 个**（因为 Frida 真跑不模拟）：

| unidbg 根因 | Frida 真机能干吗 | 原因 |
|---|---|---|
| **① 上下文相关 BL 解码 bug**（unicorn2） | ✅ 能干（**根本不用模拟**） | Frida 真机 CPU 跑原版 so，**不经过 unicorn**，BL 指令天然对 |
| **② 数据与指令对齐对应缺口**（`std::vector`/glClear 类） | ✅ 能干（**Frida 核心强项**） | 真机运行时**堆上真对象图**，Frida hook/dump 拿**真值**（把 unidbg 零值对象图填成真值） |
| **③ 288MB `PROT_NONE` mmap** | ✅ 能干（**真内核真 mmap**） | Frida 真机 attach，**内核级内存行为是真的**，不用 mock |

**"unidbg + Frida 一对一对齐"方案（FRIDA_NOTES §八 / ch13 13.4c）的价值**：
- **unidbg 负责**：离线批量 + 回归（③ 层，1174/1174 PASS）+ 加载 so + 跑调用链（**运行时对齐是零值**）
- **Frida 真机负责**：dump 对象图真值（⑥ 层，glClear 类对象 / `std::vector` 指针）+ 真跑验证行为（**不经过模拟器 bug**）
- **一对一逐步对齐**：unidbg 跑到哪，Frida 真机 dump 到哪，逐字段把"零值对象图"填成"真对象图" → 渲染出帧

**但"能"有个前提 + 边界**：
- **Frida 要 attach 真机**：军棋 32 位 so 在 **MuMu（32 位 ARM 转译 + 真 GPU）**上可 dump（**32 位 so 对象图**）；**64 位翻译体**（arm64）不是原版 so，Frida dump 的是翻译体对象图，**不是原版**
- **Frida 不能替代 unidbg 的"离线批量回归"**：unidbg 能 1174/1174 PASS（批量 + 可复现），Frida 真机**不能批量回归**（要真机 + 单点 dump）
- **Frida 补的是 ⑥ 层（对象图真值），不是 ⑤ 层（C 源码）**：**"unidbg 干不了的对象图，Frida 能补"对，但"补的是运行时对象图（⑥），不是原始 C 源码（⑤）**"——⑤ 层（C 源码）**编译时就丢了**（变量名/注释/优化差异，多对一），**不是 unidbg/Frida 的模拟问题，是"编译单向性"的根问题**，**Frida 也拿不出 ⑤ 层**

**一句话**：**Frida 真机能突破 unidbg 的 ①②③ 模拟天花板（真跑不模拟）+ 补 ⑥ 层（对象图真值），但补不了 ⑤ 层（C 源码，编译单向性根问题）**。**"unidbg + Frida 对齐" = 行为级等价（③④⑥），不是源码级还原（⑤）**。

---

## B.6 诚实边界

- **C/C++/Rust 三个 so 全 `/tmp/opencode/` 真编译**（`so_c.so`/`so_cpp.so`/`so_rust.so`，`readelf` 实读，非凭空）；Rust 用 `rustc 1.99.1 --crate-type cdylib` 编译（4.5MB，std 静态链），**实测修正了"Rust 无 `.eh_frame`"的旧推测（Rust 真编译有 `.eh_frame`，panic 栈展开用）+ "Rust 不依赖 C++ 运行库"（DT_NEEDED 只有 libgcc_s/libc/ld-linux）**
- **军棋原版 so 实读自 `军棋.apk` 解出的 `lib/armeabi/libjunqi.so`**（1435156B，32 位 ARM，strip 的，`.symtab` 删）；**翻译体 so 是 `dec/` 重打包的 5.4MB**（含 `g_jqt_*`，没 strip）——**两个 so 别混**（B.3.3 纠正）
- **3 个 so 是"最小 so"**（几十字节），军棋 so 是"1.4MB 复杂 so"（7 依赖 + C++ + 反检测），**布局同构（PT_LOAD/DYNAMIC/NOTE/RELRO），复杂度差 100 倍**（军棋 5539 重定位 vs C so 7 条）
- **B.5b 的"系统 so 能查公开源码"指 AOSP/Bionic/libstdc++ 开源**，不是"从军棋 so 逆出来的"——军棋 so 私有（strip），系统 so 公开（查源码），两者"还原 C 源码"的方式不同

---

*最后更新：2026-11-03（新建附录 B：ELF 正向生成流程 + C/C++/Rust 三个真实 so 布局对比（readelf 实读）+ 纠正".got 全 0"错误表述 + 纠正"原版 so strip vs 翻译体 so 没 strip"混淆（B.3.3）+ 新增 B.5b"能不能通过 .got 还原 C 源码"分层回答 + 新增 B.5c"逆 so 还原源码"两条路线 + 6 步总纲（路线 1 外部依赖还原标准库 + 路线 2 内部调用链；现实目标=行为级等价①②③④，逆不了 C 源码⑤+对象图⑥）+ B.5c.5 真实反汇编例子（军棋 0x243E0 无名函数，机器码 + 寄存器 vs 函数/变量名，③ vs ④ 层分界）+ B.5c.6"unidbg 干不了 Frida 真机能干"（Frida 突破 ①②③ 模拟天花板 + 补 ⑥ 对象图真值，但补不了 ⑤ C 源码，编译单向性根问题）+ 3 语言 C/C++/Rust 全真编译对比（Rust 实测修正：有 .eh_frame + 不依赖 C++ 运行库 + 4.5MB std 静态链）；跟 31 章 + 附录 A（安卓游戏开发）+ ELF_FORMAT_SUPPLEMENT.md（4 本书揉合）互补）*
