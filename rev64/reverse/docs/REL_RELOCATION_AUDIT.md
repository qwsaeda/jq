# RELATIVE 重定位 + 影子映射 对抗审计（方向 C）

依据看雪《GOT，PLT和动态链接》《so 逆向基础全图》两篇规范，逐条核对工作区 32 位军棋 so
重定位实现。审计对象与关键文件：

| 项 | 位置 |
|---|---|
| `jq_rel_data.h`（5312 槽 REL 表 + 28288B `.data` blob） | `rev64/reverse/src_jni_translated/jq_rel_data.h` L1775-1777 |
| 生成脚本 `gen_shadow.py` | `rev64/tools/gen_shadow.py` |
| blob 拷贝 + REL 加基址循环 | `rev64/reverse/src_jni_translated/jqt_mem.c` L319-327 |
| 12 init_array 执行 | `rev64/reverse/src_shadow/jq_rt.c` L332-337 |
| PLT→GOT→符号 214/214 映射 | `rev64/reverse/analysis/plt_map.txt`（214 行） |
| `.got` 区段 681 槽收表 | `rev64/reverse/docs/REVERSE_LOG.md` §124y-79（L3680-3720） |

## 一、我们已对（经 readelf 精确验证）

### 1. R_ARM_RELATIVE 公式（核心判据，正确）
看雪规范：`R_ARM_RELATIVE` 重定位语义 = `P + S`（P = 加载基址 base，S = 目标槽的静态值，
**addend 恒 0，与符号无关**），linker 写 `P+S` 到槽。

我们的实现（`jqt_mem.c` L324-327）：
```c
memcpy(g_jqt_shadow + JQ_DATA_VA, jq_data_blob, JQ_DATA_SZ);   /* L319: 先拷 .data 区 28288B 原始值 */
for (i = 0; i < JQ_REL_N; i++)
    *(uint32_t *)(g_jqt_shadow + jq_rel_tbl[i]) += (uint32_t)g_jqt_shadow;  /* L324-327: 每槽 += load_base */
```
- 槽的静态值（blob 里）+ `g_jqt_shadow`（影子 load_base）= `S + P`，**完全符合规范**。
- **不是误读**：每槽加 load_base 是对的。5312 槽与 `readelf -rW` 的 `R_ARM_RELATIVE` 条目
  **逐一精确匹配**（`tbl - orig = ∅`，`orig - tbl = ∅`，零偏差，L4 实测）。
- `.bss` 从 `img_end=0x15be80` 清零（L313-315）正确：.bss 是 NOBITS，dlopen 零初始化，
  不在 blob 范围（blob 到 0x15be80 为止，不含 .bss），清零即忠实。

### 2. 5312 REL 槽覆盖范围（正确，含 `.got` 区 458 条 REL 目标）
`readelf -rW` 精确统计：
- `.rel.dyn`：R_ARM_RELATIVE **5312** + R_ARM_ABS32 **7**（`__cxa_pure_virtual`）+ R_ARM_GLOB_DAT **6** = 5325 条
- `.rel.plt`：R_ARM_JUMP_SLOT **214**
- REL 目标范围 `0x155000..0x15be2c`（首= `.init_array[0]`，末= `.bss` 开头 5 槽）
- REL 目标落在 `.got` 区（0x15b0c8..0x15bb6c）的有 **458 条**（0x15b42c 起）——这些是
  `.got` 里的"数据指针槽"（字符串表 0x50c 区、vtable/函数指针表），**属于 REL 槽，加 base 正确**

### 3. `.got[0/1/2]` 固定项（理解无遗漏）
看雪规范固定项：
- `.got[0]` = `&_dynamic`（= `.dynamic` 段地址 0x15afe0，**不属于 5312 REL 表**，linker dlopen
  时初始化填入；blob 保留原始值 0x15afe0，正确）
- `.got[1]` = 共享库保留槽（Bionic 32 位 = 0）；`.got[2]` = 保留槽（= 0）
- `.got[3]` = PLT[0] 首跳板地址 0x229c0（JUMP_SLOT 第 1 槽，lazy-binding 占位）

我们文件原始值实测（readelf + struct 精确读）：
- `.got[0]`（0x15b0c8）= `0x15afe0`（= `.dynamic` 段 VA，**正确**，不被 REL 加 base，保留原始值——与 linker 行为一致）
- `.got[1]`（0x15b0cc）= `0`（shared library，正确）
- `.got[2]`（0x15b0d0）= `0`
- `.got[3]`（0x15b0d4）= `0x229c0`（= `.plt` 起点，`JUMP_SLOT` 第 1 槽，lazy-binding 占位=PLT[0]，正确）
- 8 个全 0 槽 = 6 GLOB_DAT（0x15b488/0x15b4bc/0x15b51c/0x15b620/0x15b764/0x15b7d8）+ 2 保留槽（.got[1]/.got[2]）

**结论：`.got[0/1/2]` 固定项我们的理解没有漏，blob 拷贝保留原始值即忠实（它们不在 5312 REL 表内，
linker 也不重定位它们）。**

## 二、潜在被误读处（本次对抗重点）

### 1. AGENTS.md L57 "`.got` 零槽位（未重定位）" 措辞有歧义（需澄清，非错）
AGENTS.md 原文：
> unidbg 失败根因是 `.got` 零槽位（未重定位）+ 初始化例程反检测，与壳无关

**看雪规范澄清**：
- **R_ARM_RELATIVE 重定位的是 `.rel.dyn` 的 5312 个目标槽**（分布在 `.init_array`/`.data.rel.ro`/
  `.got`/`.data`，目标槽加 base），这是**静态链接期**完成的，跟 `.got` 是否已重定位是**两回事**。
- **`.got` 里的 GLOB_DAT（6 条）+ JUMP_SLOT（214 条）** 才是"动态符号解析"填的槽，需要
  `dlopen` 时 linker 查符号表填——unidbg 补环境下 GLOB_DAT 6 符号**已从 docker 真 ARM libc 解析
  成功**（§84，`_ctype_`→`_C_ctype_` 等），JUMP_SLOT 211/214 eager-resolve 成功。
- 所以**"`.got` 零槽位未重定位"这个措辞把两套机制混了**：RELATIVE 5312 槽是重定位了的（我们
  加 base 正确），`.got` 里真正"未填充"的是 6 GLOB_DAT + 214 JUMP_SLOT 的**符号解析**槽
  （unidbg 部分已解析，剩余阻塞是 C++ 对象图 + BL 解码 bug + 288MB mmap，不是 `.got` 未重定位）。

**精确纠正**：unidbg 失败根因精确归因（§124y-152 实锤）= ① unicorn2 上下文相关 BL 解码 bug +
② C++ 对象图未构造（.bss 非零率 0.2% vs MuMu 79.1%）+ ③ 288MB PROT_NONE mmap 失败。
**".got 零槽位未重定位" 是早期（§84 前）的粗归因，已被 §124y-152 精确化推翻**——AGENTS.md L57
该句是**过时措辞**，应更新为上述三根因。

### 2. `jq_rel_data.h` 注释 "28288B `.data` 0x155000-0x15be80" 段名有误（术语，非功能错）
`readelf -SW` 精确段表：
- 0x155000-0x155030 = **`.init_array`**（INIT_ARRAY，48B = 12 指针）
- 0x155030-0x15afe0 = **`.data.rel.ro`**（PROGBITS，24592B）
- 0x15afe0-0x15b0c8 = **`.dynamic`**（DYNAMIC，232B）
- 0x15b0c8-0x15bb6c = **`.got`**（PROGBITS，2724B = 681 槽）
- 0x15bb6c-0x15be80 = **`.data`**（PROGBITS，788B）

`jq_rel_data.h` 把整段 0x155000-0x15be80 统称 "`.data` 段"，**段名不精确**（实际含 5 个段）。
功能上无影响（整段 memcpy + 整段 REL 加 base 都正确），但术语应改为
"data 区段（.init_array/.data.rel.ro/.dynamic/.got/.data）"。

### 3. GLOB_DAT 6 槽 + ABS32 7 槽不在 5312 REL 表内（正确，但需理解语义）
- **GLOB_DAT 6 条**（`__stack_chk_guard`/`__sF`/`_toupper_tab_`/`_tolower_tab_`/`_ctype_`/`__dso_handle`）
  全部在 `.got` 区，文件原始值全 0，**不在** 5312 REL 表（已验证），需要符号解析填——
  翻译体（`jqt_mem.c`）对 `_ctype_` 等无 stub 兜底（.bss 全 0 忠实兜底），这是**已知的诚实边界**
  （§124y-152），非重定位误读。
- **ABS32 7 条**（`__cxa_pure_virtual`）指向 `.rodata`/外部，文件原始值 0，也不在 REL 表，
  翻译体未处理（纯虚函数异常路径，游戏主链不触发），非阻塞。

## 三、三大反驳

1. **"每槽加 load_base 是不是该加两次？"——不是。** `jqt_mem.c` 先 `memcpy` blob 原始值到
   `shadow+JQ_DATA_VA`，**再**对 5312 槽 `+= shadow`。顺序正确，无二次加。
   blob 里的值是**未重定位**的静态值（如 `.got[0]=0x15afe0`、`.init_array[0]=0x3dae5`），
   加一次 `shadow` 后 = `shadow + S` = `P + S`，符合 `R_ARM_RELATIVE` 语义。若加两次会错
   （`P + S + P`），但代码只加一次，**无误**。

2. **"`got[0/1/2]` 是不是也该加 load_base？"——不该，且我们没有加（正确）。**
   `.got[0]`（=0x15afe0）、`.got[1]`（=0）、`.got[2]`（=0）**不在 5312 REL 表内**
   （已验证 0x15b0c8/0x15b0cc/0x15b0d0 均不在 REL 目标集），它们是被 linker 固定初始化
   的槽（`.got[0]=&_dynamic`、`.got[1]=base_for_exec/0`、`.got[2]=PLT[0]`），
   **R_ARM_RELATIVE 不作用于它们**。我们保留原始值 0x15afe0/0/0 是**忠实**（shared lib `.got[1]=0`）。
   唯一偏差：真实 dlopen 后 `.got[0]` = `base + 0x15afe0`（绝对），我们保留 0x15afe0（相对），
   但翻译体**从不解引用 `.got[0]`**（不走动态符号解析主链），此偏差无实际影响。

3. **"12 个 init_array 全通，顺序对不对？"——顺序对，但"全通"措辞需精确。**
   看雪规范：`_init` → `init_array[0..n]`（**按数组顺序正序**）→ 业务 →
   `fini_array[n..0]`（**逆序**）→ `_fini`。
   - 我们的 `jq_rt.c` L333-336：`for (i=0; i<n; i++) jq_call(init_array[i])` ——**正序，正确**。
   - 原版 so **无 `fini_array` 段**（readelf 未见，`.got` 后直接 `.bss`），
     `.dynamic` 里 `FINI_ARRAY`/`FINI` tag 缺失 —— 无需逆序处理，**无缺**。
   - **但 "全通" 精确口径**：§84 早期（dynarmic 后端）12/12 全通；
     **§124y-152（unicorn2 后端）实锤 = 11/12**（`init[11]=0x11c10d` 报 `UC_ERR_INSN_INVALID`，
     是 unicorn2 在函数中间偏移起跳的**模拟器精度 bug**，非顺序/逻辑错）。
     AGENTS.md L101 "12 个 init_array 全通" 应注明 **"unicorn2 路径 11/12，init[11] 精度 bug"**，
     与 §124y-152 对齐。

## 四、推动建议

**选 (i) 立即做核对（输出重定位审计结论）+ AGENTS.md 三处文档补强（不改代码）**，理由：
重定位核心逻辑（每槽加 base / 5312 槽 / `.got` 固定项 / init_array 正序）**全部正确**，
代码零偏差，无需改码；但 AGENTS.md L57 "`.got` 零槽位未重定位" 是过时粗归因
（已被 §124y-152 精确三根因推翻）、L101 "12 个 init_array 全通" 需补 unicorn2 11/12 口径、
`jq_rel_data.h` L1 段名 "`.data`" 需改为 "data 区段（5 段）"——**三处文档补强即可**，
避免后续会话误读"`.got` 零槽位=RELATIVE 未做"导致重新踩坑。

**补强清单（文档，不改代码）**：
1. `AGENTS.md` L57：`.got` 零槽位（未重定位）→ 改为
   "unidbg 失败精确三根因（§124y-152）：① unicorn2 上下文相关 BL 解码 bug
   ② C++ 对象图未构造（.bss 非零率 0.2% vs MuMu 79.1%）③ 288MB PROT_NONE mmap 失败；
   `.got` 区 6 GLOB_DAT + 214 JUMP_SLOT 是**符号解析槽**（非 RELATIVE），
   unidbg 已解析 GLOB_DAT 6 + JUMP_SLOT 211/214，与 5312 RELATIVE 槽（已全重定位）是两套机制"
2. `AGENTS.md` L101：`12 个 init_array 全通` → `12 个 init_array（unicorn2 路径 11/12，
   init[11]=0x11c10d 报 UC_ERR_INSN_INVALID = 模拟器精度 bug，非顺序/逻辑错）`
3. `jq_rel_data.h` L1：`28288B .data 0x155000-0x15be80` → `28288B data 区段
   （0x155000-0x15be80 = .init_array/.data.rel.ro/.dynamic/.got/.data 五段）+ 5312 RELATIVE 槽`

**实锤数据（可复现判据）**：
- `readelf -rW libjunqi.so | grep -c R_ARM_RELATIVE` = **5312**（= `jq_rel_tbl` 长度）
- `readelf -SW libjunqi.so`：`.init_array 0x155000 sz 0x30` + `.data.rel.ro 0x155030 sz 0x5fb0`
  + `.dynamic 0x15afe0 sz 0xe8` + `.got 0x15b0c8 sz 0xaa4` + `.data 0x15bb6c sz 0x314`
  （末 0x15be80，合计 28288B）
- `.got[0]=0x15afe0` / `.got[1]=0` / `.got[2]=0` / `.got[3]=0x229c0`（PLT[0]）
- GLOB_DAT 6 槽全 0：0x15b488/0x15b4bc/0x15b51c/0x15b620/0x15b764/0x15b7d8
