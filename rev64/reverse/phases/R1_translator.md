# R1：二进制翻译器 + 可链接 arm64 libjunqi.so

## 目标
把原版 32 位 `libjunqi.so`（ELF32/ARM/Thumb，`.text` 0x233e0-0x1311f4，514,762 条指令）
用**机械二进制翻译**转成可加载的 arm64 `.so`，保留完整逻辑，14 个 JNI 入口可导出。

## 结果
- **翻译器**：`tools/t32_to_a64.py`（ARM Thumb T32 → AArch64，覆盖 **99.78%**，1017/484510 未处理均 nop 占位）。
- **全量汇编 0 错误**：484,510 条指令 → 6.5MB `.o`。
- **全量链接 0 错误**：`dist/libjunqi_arm64.so`（2.57MB，ELF64 aarch64，14 个 `Java_com_cnvcs_App_On*` 导出）。
- **全局数据访问**：11352 个 `ldr rX,[pc,#imm]`+`add rX,pc` 配对 → `adrp/add .Lg_<目标>`（3444 个 `.Lg_` 标签，覆盖 .bss/.rodata/.data.rel.ro/.data/.got 基址/.text 字面量）。

## 关键设计（翻译规范，后续参考）
- **寄存器映射**：r0-r12→x0-x12，sp→x13（影子 SP），lr→x14（影子 LR），pc→x15（不用），sl/fp/ip→x10/x11/x12。
- **32 位语义**：所有算术/内存用 w 寄存器（`ldr wD`/`str wD`/`adds w`），匹配 Thumb 32 位寄存器；避免 `ldr xD` 读写相邻 4 字节。
- **影子栈**：x13 作 Thumb SP 影子（`push`→`str w,[x13,#-4]!`，`pop`→`ldr w,[x13],#4`），真实 x31 不动（16 对齐保持给外部调用）。
- **JNI 入口序言**：每个 `Java_com_*` 入口 `mov x13,sp; mov x14,x30`（影子 SP/LR），14 个 `.global` 导出。
- **全局数据（PC 相对）**：`ldr rX,[pc,#imm]; add rX,pc` 配对 → `adrp xX,.Lg_<target>; add xX,xX,:lo12:.Lg_<target>`。**关键**：32 位字面量放不下 64 位绝对地址，必须用 adrp/add 直算目标，不能沿用原 PC 相对偏移。
- **字面量池**：独立常量加载用 `.Llit_<addr>`（`.word` 原值）；PC 相对字面量加载用 `adrp x17,.Llit; add x17,x17,:lo12; ldr wD,[x17]`（x17 未映射，安全临时）。
- **数据段**：`.rodata/.data.rel.ro/.data/.bss` 按原字节+`.Lg_<addr>` 标签复制；GOT 槽单独处理。
- **IT 块**：`itet/ittt/...` 条件执行 → `b.cond` 包裹（arm64 无 IT 块）。

## 翻译器操作规范（复用要点）
- **覆盖率**：99.78%（1017/484510 未处理均 nop 占位，集中在 NEON/协处理器）。
- **输入**：`arm-linux-gnueabi-objdump -D -b elf32-littlearm -m arm -M force-thumb` 全量反汇编。
- **输出**：arm64 汇编 → `aarch64-linux-android-clang -c` 汇编 → 链接。
- **验证**：全量汇编 0 错误、全量链接 0 错误；`.Lg_` 标签数、add-pc 配对数作覆盖率指标。
- **红线**：RE_REG 必须非捕获组 `(?:...)`；标签 8 位零填充；分支目标 0x22000-0x170000 过滤。

## 踩坑与修复（重要经验）
1. **RE_REG 必须非捕获组** `(?:...)`：否则多寄存器 regex 的 group 编号错位 → 生成错误代码（如 `subw r0,r0,#2065` 生成 `#r0`）。这是全局正确性关键。
2. **标签零填充**：`.Lf_{addr:08x}`（8 位零填充）vs 分支引用 `.Lf_<raw>`（无填充）→ 大量 undefined 符号。`lf()` 必须 `t.zfill(8)`。
3. **分支目标过滤**：反汇编数据误读产生伪目标（`bl 0xffc19040`）→ 用 `valid_tgt()`（0x22000-0x170000）过滤为 nop。
4. **分支目标占位**：objdump 未反汇编的地址（数据洞/`bl` 中部）被分支引用 → 内联 `.Lf_<t>:.inst 0xd503201f` 占位，保持地址顺序使分支在 ±1MB 范围内。
5. **条件分支集合正则**：`re.match(r'b\.', 'bvc.n')` 为 False（助记符是 `bvc.n` 非 `b.eq`）→ 用 `[a-z]+\.(n|w)`。
6. **ldr/str 对齐**：arm64 严格对齐（`ldr w` 偏移须 %4==0），Thumb 宽松 → 非对齐/超范围用 `add x16,xN,#off; ldr wD,[x16]` 中转。
7. **add-pc 配对放宽**：不能只看紧邻，须向前扫描（同一基本块，rX 未改写）——OnEvent 中 `ldr r3,[pc]; adds r1,r0,#0; add r3,pc` 有间隔指令。放宽后 2638→11352 对。
8. **`.w` 后缀保守 nop**：`stmia.w`/`and.w`/`sbc.w` 等宽指令带移位/大立即数，语义复杂 → 保守 nop（不生成错误代码）。
9. **逻辑立即数中转**：`and/eor/orr/bic #imm` 的 arm64 编码受限 → `mov w16,#imm; op wD,wN,w16`。
10. **移位 #32→mod 32**：`lsr w5,w0,#32` 超 arm64 范围 → `%32`。
11. **subw 须在 sub 前**：`subw r0,r0,#2065` 若 sub 分支先命中会误解析。
12. **`pop pc`→`ret`**：不能 `ldr x15`（x15 非真实 PC），用 `ldr w30,[x13],#4; ret`。
13. **`add rd,pc` 3 元素形式**：`add rd,rn,pc` 也需 adrp/add 处理（不只 2 元素）。

## 已知限制（R2+ 待解决）
- **GOT 函数调用**：外部调用走 `ldr rX,[rBase,#off]; blx rX`（GOT 槽 32 位加载）。arm64 GOT 槽须 64 位（8 字节），原槽 4 字节间距 → **地址空间/布局不匹配**。需数据流跟踪 + GOT 重映射，或改写为 `:got:` 直取符号。
- **NEON/协处理器**：约 1000 条（`vrhadd.u16`/`vtbx.8`/`stc2`/`mcr2` 等）nop 占位，其中在 .rodata 自解密路径 → 解密可能失败（游戏数据仍加密）。
- **数据段内嵌指针**：`.data.rel.ro`（vtable/函数指针表）是 32 位原地址，需重定位到 arm64 函数（64 位）。
- **运行时验证**：redroid 无 arm64 native bridge；需**真实 arm64 设备**（有 GPU）测 dlopen + OnCreate。
- **`.so` 未签名/未打包**：需后续集成到 APK（`lib/arm64-v8a/libjunqi.so`）+ v1/v2 签名。

## 下一步（R2）
1. GOT 64 位化：外部符号 `.xword` 动态重定位（R_AARCH64_GLOB_DAT/JUMP_SLOT）+ 槽访问改 64 位。
2. NEON/加密指令实现（.rodata 解密路径）。
3. 数据段指针重定位（post-link fixup）。
4. 真 arm64 设备 dlopen 验证。
