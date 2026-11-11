# REL_RELOCATION_AUDIT — 32 位 so 重定位核对（2026-11-11）

> 本审计用 ARM ELF 权威文本（看雪《GOT，PLT 和动态链接》《so 逆向基础全图》）核对
> 工作区已有 32 位 so 重定位理解（`jq_rel_data.h` / `gen_shadow.py` / `jqt_mem.c`），
> 找出被误读处。**结论：重定位核心逻辑零代码偏差，仅 3 处文档措辞需补强。**

## 1. 审计对象

| 对象 | 位置 | 说明 |
|---|---|---|
| `jq_rel_data.h` | `rev64/reverse/src_jni_translated/jq_rel_data.h` | 自动生成：`jq_data_blob[28288]` + `jq_rel_tbl[5312]` |
| `gen_shadow.py` | `rev64/tools/gen_shadow.py` | blob/REL 表生成脚本（IMG_LO=0x233e0） |
| `jqt_mem.c` | `rev64/reverse/src_shadow/` | MEM 重定位实现：memcpy blob + 每槽 `+= g_jqt_shadow` |

原版 so：`军棋.apk → lib/armeabi/libjunqi.so`（ELF32/ARM/EABI5/stripped，1435156B）

## 2. 我们已对（权威核对结论）

### 2.1 R_ARM_RELATIVE 每槽加 load_base = 规范正确

`jqt_mem.c` 重定位序：
```c
memcpy(g_jqt_shadow + JQ_DATA_VA, jq_data_blob, JQ_DATA_SZ);   // 先拷原始 blob
for (i = 0; i < JQ_REL_N; i++)
    *(uint32_t*)(g_jqt_shadow + jq_rel_tbl[i]) += (uint32_t)g_jqt_shadow;  // 再加 base
```
- `R_ARM_RELATIVE` 语义 = `加载基址 B + 槽内静态值 S`，**不依赖运行地址 P**。
- 我们 `+= g_jqt_shadow`（影子 load_base）加一次得 `B+S`，符合规范。**没加两次，没加错。**
- 与看雪 `R_ARM_RELATIVE = B + S` 公式一致。

### 2.2 5312 槽与 readelf 精确匹配（零偏差）

- `jq_rel_tbl[5312]` 与 `readelf -rW libjunqi.so | grep R_ARM_RELATIVE` 条目**逐一精确匹配**。
- 可复现判据：`readelf -rW 军棋.apk 解出的 so | grep -c R_ARM_RELATIVE` = 5312
- `tbl − orig = ∅`（我们表里没有原版没有的槽）、`orig − tbl = ∅`（原版有而我们漏掉的槽 = 0）

### 2.3 `.got[0/1/2/3]` 固定项理解无遗漏

原版 `.got` 第 0-3 槽原始值：
- `.got[0]` = 0x15afe0（= `.dynamic` 段 VA，PLTGOT tag 值）
- `.got[1]` = 0（link_map 保留槽）
- `.got[2]` = 0（resolver 保留槽）
- `.got[3]` = 0x229c0（= `.plt` 起点，JUMP_SLOT 第 1 槽）

这 4 槽**不在 5312 REL 表内**（已验证 0x15b0c8/0x15b0cc/0x15b0d0 不在 REL 目标集）。
linker 也确实**不对**它们做 RELATIVE 重定位（是 dlopen 固定初始化的 `_dynamic`/保留槽）。
blob 保留原始值即忠实。**无遗漏。**

### 2.4 `.bss` 从 `img_end=0x15be80` 清零

`.bss` 是 NOBITS（0x15be80..0x162e88），blob 到 0x15be80 为止不含 `.bss`。
`gen_shadow.py` 从 `img_end` 起清零 = dlopen 零初始化的忠实镜像。**正确。**

## 3. 潜在误读（文档措辞需补强，非代码错）

### 3.1 AGENTS.md L57 "`.got` 零槽位（未重定位）" 混淆两套独立机制

- `R_ARM_RELATIVE`（5312 条）重定位的是 `.rel.dyn` 的**目标槽**（分布在
  `.init_array`/`.data.rel.ro`/`.got` 区 458 条 / `.data` / `.bss` 开头 5 条），
  加 `load_base` 即可，**不依赖符号解析**。
- `.got` 里真正的"零槽位" = **6 GLOB_DAT**（`__stack_chk_guard`/`__sF`/`_toupper_tab_`/
  `_tolower_tab_`/`_ctype_`/`__dso_handle`，文件原始值全 0）+ **214 JUMP_SLOT**，
  这些需 linker **符号解析**填，**不在 5312 REL 表内**（REL∩GD=∅，REL∩JS=∅，已验证）。
- **unidbg 失败精确根因**（§124y-152 实锤）= ① unicorn2 上下文相关 BL 解码 bug
  + ② C++ 对象图未构造（`.bss` 非零率 0.2% vs MuMu 79.1%）+ ③ 288MB PROT_NONE mmap 失败。
  "`.got` 零槽位未重定位"是 §84 前的**粗归因，已被精确化推翻**。
- **AGENTS.md L57 已补强（2026-11-11）：** 区分两套机制 + 三根因精确化。

### 3.2 `jq_rel_data.h` L1 注释 "28288B `.data` 0x155000-0x15be80" 段名不精确

术语偏差（非功能错）：0x155000-0x15be80 实为 **5 段拼合**：
- `.init_array`（0x155000..0x155030，12 指针）
- `.data.rel.ro`（0x155030..0x15afe0）
- `.dynamic`（0x15afe0..0x15b0c8）
- `.got`（0x15b0c8..0x15bb6c，681 槽）
- `.data`（0x15bb6c..0x15be80）

整段 memcpy + 整段 REL 加 base 都正确，仅术语需精确。**`jq_rel_data.h` L1 已补强（2026-11-11）。**

### 3.3 "12 个 init_array 全通" 措辞需精确为 "11/12"

`jq_rt.c` L333-336：`for(i=0;i<n;i++) jq_call(init_array[i])` = **数组正序**，
符合看雪规范（_init → init_array[0..n] 正序 → 业务 → fini_array 逆序 → _fini）。
原版 `.dynamic` 只有 `INIT_ARRAY`/`INIT_ARRAYSZ` tag，**无 `.fini_array` 段、无 FINI tag**
→ 无需逆序处理，无缺。

**unicorn2 路径精确口径（§124y-152）：** init[0..10] 全通；init[11]=0x11c10d
（`moneypunct<wchar_t>::do_negative_sign`，函数中间偏移起跳）报 `UC_ERR_INSN_INVALID`
= **模拟器精度 bug**，非顺序/重定位错。AGENTS.md 旧口径"12 个 init_array 全通"
应补注"unicorn2 路径 11/12，init[11] 精度 bug"。**AGENTS.md L101 已补强（2026-11-11）。**

### 3.4 GLOB_DAT 6 槽 + ABS32 7 槽（`__cxa_pure_virtual`）翻译体未填

已知诚实边界（非误读）：`jqt_mem.c` 对 `_ctype_`/`_toupper_tab_`/`__cxa_pure_virtual`
无符号解析 stub，`.bss` 全 0 忠实兜底（§124y-152）；游戏主链不触发纯虚/异常路径，**非阻塞**。

## 4. 可复现判据（防假绿）

| 判据 | 命令 | 期望 |
|---|---|---|
| REL 槽数 | `readelf -rW <so> \| grep -c R_ARM_RELATIVE` | 5312 |
| tbl 匹配 | 逐一比 `jq_rel_tbl[i]` vs readelf REL offset | `tbl−orig=∅` 且 `orig−tbl=∅` |
| REL∩GD 不交 | REL 目标集 ∩ 6 GLOB_DAT 槽 | 空集 |
| .got[0..3] 原始值 | 读 so 文件 .got 区 4 槽 | 0x15afe0 / 0 / 0 / 0x229c0 |
| init_array 全奇数半字 | 读 so .init_array 12 指针 | 全奇数（Thumb \|1 标志） |
| .bss 起始 | readelf -S | 0x15be80（= blob 末） |

## 5. 结论

重定位核心逻辑（5312 REL 每槽加 base / `.got` 固定项 / init_array 正序 / `.bss` 清零）
**全部正确、零代码偏差**。补强的仅是 3 处文档措辞（AGENTS.md L57/L101 + jq_rel_data.h L1）。
无代码改动需求。
