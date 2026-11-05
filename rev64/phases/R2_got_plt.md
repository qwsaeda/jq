# R2 · 外部调用 / GOT / 兼容层 阶段文档

> 目标：让机械翻译的 arm64 `.so` **可链接、可加载、外部符号正确解析**，
> 并在数据宽度（32→64 位）问题上给出**基于证据的诚实结论**。

## 目标（四要素）
- **目标**：外部符号（PLT/GOT）在 arm64 下正确解析 + 数据宽度 32→64 处理。
- **文档**：本文件 + `docs/LESSONS.md` L11-L15。
- **规范**：`blx <PLT>`→`bl <符号>`（链接器自动建 PLT/GOT）；GOT 槽 8 字节 + `lsl`；
  `__aeabi_*` 用 arm64 原生助手；`.data.rel.ro` 内嵌指针**不能机械加宽**（硬墙）。
- **经验**：L11（GOT 槽 32→64）、L12（blx→PLT）、L13（__aeabi_*）、L14（NEON=字面量）、
  L15（.data.rel.ro 硬墙）。

## 结论摘要

- ✅ **PLT 外部调用已解决**：6928 处 `blx <PLT>` → `bl <符号>`，链接器自动建 PLT/GOT。
- ✅ **GOT 已 64 位化**：681 槽×8 字节，代码访问改 `lsl x16,#1` + `ldr x,[xBase,x16]`。
- ✅ **`__aeabi_*` 兼容层已提供**：34 个 ARM EABI 助手（软浮点/除），arm64 原生实现。
- ✅ **NEON/协处理器"指令"全为字面量池数据**（非真实代码），nop 占位无害。
- ❌ **`.data.rel.ro` 内嵌指针是根本硬墙**：4800 个 32 位指针在 64 位下须 8 字节，
  全部结构体布局偏移失效（vtable dispatch 6879 处全错）。**机械翻译无法跨越。**

## 交付物

- `dist/libjunqi_arm64.so`：2.58MB，ELF64 aarch64，14 个 `Java_com_cnvcs_App_On*` 导出，
  外部符号（gl*/libc/pthread）经 `.plt`/GOT 动态解析，结构完整可加载。
- `dist/rev64_full.S` / `dist/rev64_full.o`：全量翻译源 / 对象（484,510 指令）。
- `tools/aeabi_compat.S`：`__aeabi_*` 兼容实现。

---

## 1. 外部函数调用（blx→PLT → bl<符号>）

**原版事实**（R2 关键发现）：

- 外部调用全部是 `blx <PLT地址>`，共 **6928 处**；PLT 入口 `0x229d4 + 12*i`，
  `i ↔ .rel.plt[i]`。PLT 段 `0x229c0-0x233dc`（0xa1c）。
- 直接 `bl`（非 PLT）：24642 处，全部到 `.text` 内部（0 处到 PLT）。
- `blx <立即数>` 到 PLT：6928；`blx <寄存器>`（间接/vtable）：3689。

**翻译修正**（`tools/t32_to_a64.py` `mn=='blx'` 分支）：

```
blx <PLT地址>  →  bl <外部符号>     // PLT_MAP 查表
```

- PLT_MAP 由 `readelf -r` 的 `.rel.plt`（R_ARM_JUMP_SLOT）按 `0x229d4+12*i` 构建，
  214 项全命中（213 个被实际调用，1 个未用）。
- 链接后 `.plt` 段自动生成（132 桩），**9477 处 `bl` 指向 PLT 范围**，
  每处对应一个 `R_AARCH64_JUMP_SLOT`。

**验证**：`bl malloc` 链接出 `R_AARCH64_JUMP_SLOT malloc@GLIBC_2.17`。

---

## 2. GOT 64 位化

**原版 GOT**：`.got` 0x15b0c8-0x15bb6c（0xaa4 = 681 槽×4 字节）。
完整重定位表（`analysis/gotslot_full.txt`）：

| 槽类型 | 数量 | 说明 |
|--------|------|------|
| `data` (R_ARM_RELATIVE) | 458 | addend 指向 `.text`93 / `.rodata`26 / `.bss`100 / `.data`23 / `.data.rel.ro`216 |
| `func` (R_ARM_JUMP_SLOT) | 214 | 外部函数 |
| `glob` (R_ARM_GLOB_DAT) | 6 | `__stack_chk_guard`/`__dso_handle`/`_ctype_`/`__sF` 等 |
| 保留槽 | 3 | 0x15b0c8/0xcc/0xd0（代码不访问） |

**GOT 访问模式**（数据流分析，`analysis/got_accesses.txt`，880 处 / 160 槽）：

- 全部为 `ldr rX,[rBase,rOff]`（rBase=GOT_BASE，rOff=寄存器偏移常量）；
  少量 `[rBase,#imm]`。
- 访问槽类型：`data` 156 + `glob` 4；func 槽经 `blx <PLT>` 直接调用（不走 GOT 加载）。

**64 位 GOT 设计**：槽 i（原偏移 4i）→ 新偏移 8i（偏移×2）。

```
ldr rX,[rBase,rOff]  →  lsl x16,rOff,#1 ; ldr xX,[rBase,x16]
ldr rX,[rBase,#imm]  →  ldr xX,[rBase,#imm*2]
```

**槽内容**（`tools/t32_to_a64.py` GOT 段生成）：
- func/glob/abs 槽：`.xword <符号>`（动态重定位）。
- data 槽：指向 `.text` → `.xword .Lf_<addend&~1>`；指向数据 → `.xword .Lg_<addend>`。

**验证**：objdump 见 `lsl x16,#1` 917 处 + `ldr x,[xN,x16]` 917 处（一一对应）。

---

## 3. `__aeabi_*` ARM EABI 兼容层

**问题**：原版 armeabi（软浮点）调用 34 个 ARM EABI 助手
（`__aeabi_uidiv/idiv/ldivmod/uldivmod/lmul` + `fadd/fsub/fmul/fdiv/fcmp*`
+ `f2iz/f2d/i2f/i2d/ui2f/ui2d/l2f/ul2d/d2f/d2lz/d2ulz` + `atexit`），
arm64 **没有这些符号**（浮点/除法原生）。

**实现**（`tools/aeabi_compat.S`，aarch64 原生指令）：

- 整数除/模：`udiv/sdiv` + `msub`，结果在 w0/w1（32 位）或 x0/x1（64 位）。
- 单精度浮点：**位模式经 w0/w1 传参**，`fmov s0,w0; op; fmov w0,s2` 返回位模式。
  - **关键**：不能写成 C `float __aeabi_fadd(float,float)`（返回 s0，ARM EABI 约定
    32 位浮点结果在 r0 的**位模式**，翻译代码用 w0 读 → 用 C 会读到 0）。
- 双精度浮点：位模式经 x0/x1。
- `__aeabi_atexit` → 转发 `__cxa_atexit`（参数重排）。

**验证**：链接后 `__aeabi_fadd/uidiv/ldivmod/f2iz` 均为 **DEF**（非 UND），
`.plt` 不再引用它们；`dist/rev64_full.so` 剩余 52 个未定义符号全为纯 Android/libc
库符号（gl*/acosf/cosf/sinf/expf/__android_log_print/__errno/_ctype_ 等），真机 dlopen 解析。

---

## 4. NEON / 协处理器"指令" = 字面量池数据（关键结论）

**现象**：1510 条 `vrhadd/vtbx/stc2/mcr2/cdp2/ldc2/vaddl/...` 被 objdump 反汇编出来，
看似 NEON 代码。

**判定证据**：
- `vrhadd.u16 d14,d12,d31` 在 ≥8 个不同地址出现**完全相同**（=同一 4 字节常量被
  字面量池多处引用）。
- `mcr/mrc/stc2` 位于 `pop`（函数尾）与 `push`（下函数头）之间（=字面量池）。
- 地址分布在 0x23000-0x120000 全 `.text` 范围，非集中。

**结论**：游戏是**软浮点 ARM 代码，不用 NEON/VFP/协处理器**；这些"指令"是
objdump 把字面量池数据误译为指令。翻译器中 nop 占位**无害**——字面量数据经
`.Llit_` 池完整保留，无需实现 NEON。

---

## 5. `.data.rel.ro` 内嵌指针 = 32→64 位根本硬墙（R2 边界）

### 5.1 规模（决定性事实）

`.data.rel.ro`（0x155030-0x155fe0，0x5fb0 = 6132 个 4 字节槽）：

| 项目 | 数值 |
|------|------|
| R_ARM_RELATIVE 内嵌指针 | **4800** |
| 指向 `.text`（函数指针/vtable 方法） | **4432** |
| 指向 `.rodata`（数据指针） | 336 |
| 指向 `.data.rel.ro`（自引用/嵌套） | 32 |
| 值为 0（null） | **0**（全部是真实指针） |
| 指针连续运行 | 587 段 |
| 非指针槽（int/float/char 字段 gap） | **1323** |
| 最长 vtable | 130 槽 |

### 5.2 消费侧（代码访问这些指针的规模）

- **6879 处** `ldr rX,[rX,#imm]`（同一寄存器，imm 全部 <512）= vtable 方法取址。
- **3689 处** `blx rX`（间接调用，多为 vtable 方法）。
- 这些 `#imm` 是**硬编码偏移**，在 32 位下按 4 字节指针排布。

### 5.3 为什么机械翻译无法跨越

64 位下指针必须 8 字节。若把 `.data.rel.ro` 的 4800 个槽加宽为 8 字节：

1. **结构体布局全变**：每个含指针的 struct/vtable 内所有字段偏移重排。
2. **硬编码偏移全部失效**：`ldr rX,[rX,#40]` 原取 vtable 第 10 个方法（40/4），
   加宽后取的是第 5 个方法的低半 + 第 6 个方法高半 = 垃圾 64 位指针。
3. **混合字段无法区分**：1323 个非指针槽（int/float/char）与指针槽交错，
   无法用统一"偏移×2"规则——须知道每个字段的类型（**完整类布局分析 = 反编译**）。

若**保持 4 字节槽不加宽**：
- `ldr xX,[rY,#imm]`（64 位加载）会读 2 个相邻槽 = 垃圾。
- 改 `ldr wX,[rY,#imm]`（32 位零扩展）读到的只是**链接时 image 虚拟地址**
  （如 0x23e57c），非运行时地址（64 位进程加载在 >4GB，且动态链接器对
  R_ARM_RELATIVE 槽会加 load_base）。须 `add xX,xX,xBase`——但仅对**指针槽**成立，
  1323 个 int 字段会被错误加 base。
- 且对象自身的 vptr 字段（指向 vtable 的指针）也在加宽之列 → 对象布局偏移连锁失效。

### 5.4 可行路径评估

| 方案 | 可行性 | 理由 |
|------|--------|------|
| 机械翻译 + 加宽 `.data.rel.ro` | ❌ | 需完整类布局分析（=反编译），违背机械翻译初衷 |
| 保持 4 字节 + 逐槽"32位加载+加base"启发 | ❌ | 1323 混合 int 字段无法区分；对象 vptr 字段加宽连锁失效 |
| box64 式 32 位地址空间模拟 | ❌ | Android 系统链接器控制加载地址，无法强制 <4GB；且违背"原生 64 位"目标 |
| 全量反编译 + 重写 | ✅（唯一可靠） | 需逐函数还原语义，月级工程，非二进制翻译 |

**结论**：机械二进制翻译到 arm64 **在 R2 可链接/可加载但不可正确运行**。
`.data.rel.ro` 的 32→64 位指针加宽是**根本阻塞**，机械翻译无法跨越；
要得到可玩的 arm64 移植必须走**反编译重写**（等价于原 ROADMAP R0-R6 手写路线）。

---

## 6. 翻译器覆盖率（R1+R2 收尾）

- 484,510 条指令，未处理 1017（0.21%），全为字面量池数据/伪指令 → nop 占位无害。
- add-pc 配对（全局数据地址）：**12148**（前向常量传播数据流，跨 bl 保留
  callee-saved r4-r11，mov 拷贝跟踪）；剩余 **66 处** adr 回退（非 PIC 模式）。
- `.Lg_` 标签 3728（全局数据）。
- 汇编 0 错误 / 链接 0 错误。

## 7. 验证记录

- **redroid**：无 arm64 native bridge，无法测 arm64 dlopen（环境限制，已知）。
- **静态校验**（本次，全部通过）：
  - `file` = ELF64 aarch64 shared object。
  - JNI 导出 = 14。
  - 外部符号 191 个 UND（52 个纯 Android/libc，其余版本化）。
  - 动态重定位 817（ABS64/GLOB_DAT/JUMP_SLOT/RELATIVE）。
  - `lsl x16,#1` 917 + `ldr x,[xN,x16]` 917（GOT scale×2 一致）。
  - `bl` 到 `.plt` 9477 处（外部调用解析）。

## 8. 文档

- `docs/LESSONS.md`：新增 L12（blx→PLT→bl符号）、L13（__aeabi 兼容层）、
  L14（NEON=字面量池数据）、L15（.data.rel.ro 硬墙）。
