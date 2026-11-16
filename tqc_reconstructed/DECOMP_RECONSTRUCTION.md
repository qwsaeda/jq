# Python 反编译重建 — 方法论 / 判据 / 防假绿规范

> 姊妹篇：`DECOMP_EXPERIENCE.md` 讲**工具层**（pycdc 补丁 / 3.11 异常表 / 解包链 / 参考）。
> 本篇讲**重建方法论 + 验收判据 + 防假绿纪律**——把 PyLingual/pycdc 出的残源码，
> 可靠地修成"字节码可对照、语义可机判"的源码，且不自我骗。
> 适用：任何"反编译 → 重建 → 验收"的逆向场景（本项目 = PCTQC_V1.9 exe，Python 3.12）。

---

## 〇、一句话带走（先读这个）

**"能 parse" ≠ "对"**。`ast.parse` 干净、能 `import`、甚至能跑，都**不等于**语义忠实于原
`.pyc`。反编译重建的唯一硬判据是**与原字节码逐指令对照**（seq/sem 双判据）+
**code object 不缺**（missing_inventory）。PyLingual 会静默产生"语法合法但语义错"的潜藏
bug（极性反转 / 漏 guard / 错异常布局 / 漏注解），**必须逐函数对 GT 反汇编实锤**，
不许以"能跑"收尾。

---

## 一、三层判据（缺一即假绿）

重建质量 = 三个正交维度，**全过才算达成**：

| 判据 | 工具 | 查什么 | 不查什么（假绿源） |
|---|---|---|---|
| **parse 层** | `ast.parse` + `-W error` | 语法合法 + 零 SyntaxWarning | 语义对不对（`return` 极性、guard、异常布局） |
| **结构层** | `missing_inventory.py` | code object **缺没缺**（函数/嵌套/类体） | 在但语义错（★ 最大假绿源） |
| **语义层** | `verify_all_modules.py`（seq + sem 双比对） | 计算流逐指令 | 纯 codegen-noise（下节白名单） |

**★ 核心教训（本轮实锤）**：`missing_inventory` 报告 0 缺失 ≠ 0 缺陷。它只查"code object
在不在"，**不查"在的语义对不对"**。真正的潜藏 bug（PyLingual cflow 断裂后拼出的残体）
全在语义层，**必须 sem 比对 GT 才能暴露**。本轮 workflow_steps 17→0、client_runner 2→0、
第四步 2→0、第五步 6→3 的真实修复，**没有一个**是 missing_inventory 报出来的，全是 sem 比对命中。

### 1.1 seq vs sem（两级"计算流"判据）

对每个 code object，把指令序列归一化成 token 流，与原 pyc 比对：

- **`seq`（字节码逐指令一致）**：最严。把 jump 归一成 TGT、`LOAD_CONST`→repr、
  `MAKE_FUNCTION`→flag、`CALL`→n=、frozenset/set 排序。差异 = 真不一致。
  用于"能不能 1:1 还原"。
- **`sem`（计算流一致，去 codegen 噪声）**：在 seq 基础上**再**剥离
  `NOP / END_FOR / END_FINALLY / EXTENDED_ARG` + 异常展开机制指令
  （`PUSH_EXC_INFO / CHECK_EXC_MATCH / POP_EXCEPT / RERAISE / WITH_EXCEPT_START /
  BEFORE_WITH / POP_BLOCK / COPY / SWAP`）。差异 = 可能仍有噪声，需人工判读。

**用法**：sem 用于**批量 triage 排序**（哪些模块还差多少）；seq 用于**最终验收**
（哪些函数真正字节码等价）。验收报告同时给两个数。

### 1.2 GT（ground truth）提取

判据的对照源**必须是原 pyc 的字节码，不是任何反编译器的输出**：
- `x/main_pyz/<module>.pyc` 是**裸 marshal**（无 16 字节头）：`marshal.loads(data)` 直接读；
  `marshal.loads(data[16:])` 会失败（坑：误加 16 字节头偏移）。
- 用 `dis.get_instructions(code)` 拿指令流；`code.co_consts` 递归取嵌套 code object。
- 找某函数：`find(c, name)` 递归 `co_consts`，比 `co_name`。
- 类方法：`find_class(c, 'ClassName', 'method')` 两层下钻。
- 注解/默认值在**类体/父函数的 `co_consts` 里**（`BUILD_TUPLE` + `LOAD_NAME str` +
  `MAKE_FUNCTION 5`），不在方法 code object 自身 —— 查注解缺失要去父级看。

---

## 二、codegen-noise 白名单（判"真差异 vs 噪声"的判读表）

sem 会因纯编译期差异报"不同点"，**不能一律当真 bug**。以下类别**是噪声，不必修**
（修了反而引入真差异）。每报一个 sem~ 先过这张表：

| 类别 | 表现 | 为何是噪声 |
|---|---|---|
| **注解默认值** | 类体方法缺 `: str`/`-> bool` 注解；`defaults` 元组 vs 无 | GT 有注解 → 编译出 `BUILD_TUPLE`+`LOAD_NAME`；recon 无注解 → 指令流短一截。**加注解即可对齐**，加完是 seq 差异但语义等价 |
| **类体方法顺序** | 方法/类属性 STORE 顺序不同 | 顺序不影响运行语义（Python 类体是执行序但语义等价） |
| **异常展开 handler 顺序** | except/finally 块里 `STORE_FAST e`/`DELETE_FAST e`/`POP_TOP` 交织顺序不同；RETURN vs RERAISE | 3.11+ 异常表展开，同一 try/except 的不同写法定序不同，**语义等价** |
| **`RETURN_CONST` vs `RETURN_VALUE`** | 尾返回 `''`/`False`/`None` 字面量 vs 先存变量再返回 | 值相同，编译路径不同（GT 源码里赋了局部变量） |
| **短路展开** | `a and b and c` 的 `COPY`/`POP_TOP` 插入位置 | 值保持 vs 分支短路，结果等价 |
| **`PUSH_NULL` 位置** | 3.12 的 `LOAD_METHOD`/调用前 `PUSH_NULL` 插缝 | 编译器内部，无语义 |
| **listcomp 内联** | 3.12 PEP 709：listcomp 不再独立 code object；genexpr 仍独立 | recon 写 `(x for ...)` 生 `<genexpr>` code object，GT 写 `[x for ...]` 内联无 code object → 重独有 1 个。**用 listcomp 对齐 GT** |

**★ 长度比启发式（隔离真 bug）**：对每个 sem~，算 `len(recon_sem)/len(orig_sem)`：
- `< 0.6x` 或 `> 1.4x` → **★ 标真差异**，必查（几乎必是漏了 guard / 错异常布局 / 整段缺失或过写）
- `0.9–1.1x` 且 diff 点全落上表白名单 → **噪声，不修**
- 中间值 → 逐 diff 点人工判读

**★ 已实锤为噪声的，记进判据台账**（避免下次重复怀疑）：如 `runner_worker/main`
尾段 `SystemExit`/`Exception` handler 的 `STORE_FAST e` 顺序、`cloud_handshake` 的
`('False','')` vs `None` handler-unwind —— 这些实锤过"尾段逐指令一致 / 仅 handler 序不同"。

---

## 三、批量处理规范（用户"按顺序做 / 批量处理"指令的执行纪律）

这是本项目反复出现的指令模式：**把剩余业务件逐个做到"可执行 + 字节码可对照"，按顺序推进**。
固化成可复现流程（每模块一轮）：

### 3.1 单模块 6 步（不可跳）

1. **定位**：`grep -n "def <fn>\|class <cls>" reconstructed/<mod>.py` 找重建体位置。
2. **拉 GT**：`dump_fn.py`/内联脚本 `find(orig,'<fn>')` + `dis.dis()`，拿到原字节码
   指令流 + `co_varnames`/`co_names`/`co_consts`/默认值。
3. **sem 比对**：seq/sem token 流 diff，列"原 N / 重 M / K 不同点"。
4. **判读**：每个 diff 点过第二节白名单 —— 真 bug 还是噪声？真 bug 看 GT 反汇编定写法
   （尤其：guard 有没有、异常是 try/except 还是 try/finally、f-string 还是 `+` 拼接、
   变量名、默认值、注解、类体顺序）。
5. **修**：`edit` 工具改 reconstructed/，保持缩进/风格与周边一致。**只改真 bug**，
   噪声不动（动了引入新差异）。
6. **复验**：重跑 sem 比对到 0（或全噪声）→ 跑 `verify_all_modules.py` 看模块级总数变化。

### 3.2 批量推进顺序（性价比排序）

- **先降总 sem~ 最多的模块**（本轮：workflow_steps 17 → client_ui 16 → runner_worker 3 …）。
- **★ 标记（长度比越界）优先于 0.9–1.1x 噪声**——越界必是漏段/过写。
- 同模块内**先修主函数，再修嵌套**（嵌套改动常是父函数改动的连带）。
- **一轮跑完所有模块再固化文档**，不要每改一个就写 doc（避免文档抖动）。

### 3.3 批量防假绿（★ 本轮最大的纪律缺口）

- **每模块必须"sem 到 0 或全噪声"才能写"已做"**。"能 parse / 能 import" 不算做。
- **修完立即复验该模块 sem**（不要攒到最后一次全跑，中间态会漏）。
- **verify 总数下降 = 验收**；总数不降就回查（可能修了 A 引入了 B 的新噪声，或改错了方向）。
- **重独有 code object（recon 有、GT 无）必须清零**：它们几乎都是 PyLingual 误拆的
  杂散顶层函数（`_map_of`/`install`/`_make_select`…）或 listcomp 误写成 genexpr。
  逐一 grep 引用点：无人引用 → 删；是误写 → 改成 GT 写法。**杂散函数是 cflow 断裂残留**。
- **文档只在批量全过且 verify 总数确认下降后才固化**（本轮：5 模块全过 → 才升 v19）。

### 3.4 文档固化点（批量完成时）

- `reconstructed/README.md`：模块清单 + 终态指标（seq%/sem%/重独有）。
- `reconstructed/CURRENT_STATE.md`：vN 里程碑 + 本轮真实修复清单（GT 驱动，逐条列函数 + 改了什么）。
- 顶层 `CURRENT_STATE.md`：同步版本号 + 一句话终态。
- **修复清单只记"真 bug"，不记噪声**（噪声记在第二节白名单台账里）。

---

## 四、高频真 bug 模式（PyLingual cflow 断裂的产物，GT 驱动可复现修）

本轮实锤的"语法合法但语义错"类别，下次遇到直接按 GT 判：

| 模式 | 症状 | GT 定写法 |
|---|---|---|
| **极性反转** | 边界/条件判定结果反了（`_比例坐标` 越界返坐标、在界返 None） | 看 GT `POP_JUMP_IF_*` 目标 + 尾部 `RETURN_VALUE` 取的是哪个 |
| **漏 guard** | 缺 `if not username: return` 早退（`_is/_mark_ammo_exchanged_today`、`_local_db_path`） | GT 开头 `POP_JUMP_IF_TRUE` 跳过的就是 guard |
| **异常布局错** | try/except/else 混用、except 该在 try 内还是外、finally 该不该有（`_run_with_aid`、`_install_capture_guard` 4 独立 try） | 看 GT `co_exceptiontable` + handler 落在哪 |
| **f-string vs 拼接** | `'a' + X + 'b'` vs `f'a{X}b'`（`_mark_ammo_exchanged_today`） | GT 有 `FORMAT_VALUE`/`BUILD_STRING` = f-string；`BINARY_OP+` = 拼接 |
| **变量名漂移** | loop 变量 `attempt` vs `_`；`val` vs `num`（`_read_local_final`、`_parse_haf_coins`） | 看 GT `co_varnames` 精确名（**sem 敏感，必须对齐**） |
| **默认值/注解漂移** | 缺默认值、默认值类型错（`within=180` vs 无）、注解全缺（client_ui 7 类） | 看 GT 父级 `co_consts` 的 defaults 元组 + `BUILD_TUPLE` 注解 |
| **杂散顶层函数** | 重建体多出 GT 没有的函数（`_map_of`/`install`/`_make_select`） | cflow 断裂残留，grep 引用点确认后删 |
| **漏 `r` 前缀** | Win 路径/regex 非 raw 字符串 → SyntaxWarning（`\Valve`、`\s`） | GT `co_consts` 存 `\\V`（双反斜杠）= 源是 raw，加 `r` |
| **class 属性顺序** | 类属性定义位置错（`StepContext` 的 `_ERROR_DIALOG_*`） | 看 GT 类体 STORE_NAME 顺序，挪到位 |

**判读锚点**：几乎每个真 bug 都能在 GT 反汇编里找到**确定性证据**（某条指令、某常量、
某 varname）。**没有 GT 证据的"我觉得应该这样"不算修复依据**。

---

## 五、诚实边界（维持）

- **Nuitka `.pyd` 引擎函数体 = C 机器码，不可还原为 Python**（最终边界，见 `ENGINE_MODULES.md`）。
  本方法论只适用于 **`.pyc`（业务层）**；`.pyd` 只能字符串/符号级提取。
- 全静态，未执行；GUI/线程/云端端到端需 PySide6 运行时，静态无法验证运行时行为。
- 第三方库不反编译，只记用法（`LIBRARY_NOTES.md`）。
- **不要在交付物里把"行为级忠实重建"说成"逐字节等价原版"** —— 34 个 sem~ 噪声是编译器
  固有差异，不是缺陷，但"seq✓ 87.8%"就是诚实口径。

---

## 六、与工具层的衔接

- 本篇（方法论/判据/防假绿）+ `DECOMP_EXPERIENCE.md`（工具/补丁/解包）= 完整的
  "反编译 → 重建 → 验收"闭环。
- 判据脚本：`verify_all_modules.py`（seq+sem 总数）、`missing_inventory.py`（结构层）、
  `dump_fn.py`（GT 反汇编 dumper）、`verify_semantics.py`（单函数 sem）。
- 重建产物：`reconstructed/*.py`（16 模块）；判据台账/修复清单：`reconstructed/CURRENT_STATE.md`。

## 七、`sem_triage.py` 自动噪声归类（v20 新增）与诚实局限

### 7.1 自动判据（可机读"31/31 噪声, 0 真差异"）—— 3 信号

对每个 sem~ code object，**三条**信号判噪声：
1. **长度比 0.88–1.12**（越界 `<0.6`/`>1.4` 标 `CHECK_LEN` 必查）；
2. **两侧 `LOAD_CONST` 值多重集相等**（常量值不漂 = 没改语义，差异纯指令重排/注解/handler）；
3. **★ `co_names`/`co_varnames` 集合相等**（v21 新增，最强真-bug 指标）：真语义改动
   （漏 guard / 错属性 / 漏类属性 / 变量改名 / 注解引用漂移）必改引用标识符集，
   两侧 names/varnames 不等 = 疑真必查。

`python3 sem_triage.py` 全模块跑，输出每模块"噪声 N / 疑真 M / ★越界 K" + 逐函数子标签
（`[names漂移]`/`[vars漂移]` 标出第 3 信号命中的项）。

### 7.2 第 3 信号实锤的 3 个真实缺陷（v21，前 2 轮 2-signal 分类器漏报为噪声）

| 函数 | 缺陷 | 修 |
|---|---|---|
| `第五步/_is_in_time_window` | 局部 `time` 遮蔽模块 `time`（GT 用 `from datetime import time as dtime` 别名） | 改 `dtime` 别名（vars 漂移清除） |
| `client_ui/RestoreWorker._payload` | 缺 `item: tuple -> tuple` 注解（GT class 体引用 `tuple`） | 补注解（names 漂移清除） |
| `client_config/ClientConfig.load` | `-> ClientConfig` 被求值为名字（GT 用字符串注解 `-> 'ClientConfig'`，const 非 LOAD_NAME） | 改字符串注解（names 漂移清除） |

### 7.3 诚实局限（★ 防假新绿 —— 不能只信自动归类）

- **小 guard 删除漏报**（回归测试实锤）：删 `if not username: return True`，sem 只 74→72
  （0.973x 仍在噪声带）→ 自动归"噪声"，**假阴性**。auto-classifier 只抓"长度越界 + 常量值漂移
  + names/varnames 漂移"三类，**抓不到不改长度带、不改常量表、不改引用标识符集的小 guard 删除**。
- **对策**：auto-classifier 是**初筛排序器**（告诉你哪些模块还差多少、优先查谁），**不是终验**。
  终验仍需：① 关键函数逐条对 GT 反汇编（§3.1 六步）；② seq（逐指令）判据过关键路径；
  ③ 改动后必跑 `missing_inventory`（结构）+ `verify_all_modules`（seq 总数）。
- **★ 越界项**是"长度比 `<0.6`/`>1.4`"，但 **0.9–1.1x 内的真差异它照归噪声**（除非命中第 3 信号）
  —— 所以"0 越界 / 0 疑真"接近但不等于"0 缺陷"，关键函数仍需 seq 终验。
- 已实锤为噪声的（`RETURN_VALUE`/`RETURN_CONST`、handler 展开、注解默认值重排、短路
  `COPY`/`POP_TOP`、`PUSH_NULL` 位置、`names/varnames` 全等的指令重排）记入 §2 白名单台账。



## 八、模块级 (<module>) 判据盲区（v22 新增，本轮实锤 54 个真缺陷）

> **★ 核心教训**：三层判据（parse/结构/函数级语义）全部**不含 `<module>` 根 code object**——
> 历史 `verify_all_modules.py` 的 `collect()` 只收 `co_consts` 里的嵌套 code object，根对象不在内。
> 结果：**模块级常量/导入/注解/默认值/定义顺序缺失 = 运行时 NameError 级真缺陷，前 4 轮判据 0 覆盖**。
> 本轮一次性暴露 54 个（`client_ui` threading / `workflow_steps` 9 个 / `第一步` 29 个 / `第四步` 23 个…）。

### 8.1 模块级判据（新增第 4 层）

| 判据 | 工具 | 查什么 |
|---|---|---|
| **全局引用完整性** | `global_ref_check.py` | 编译模块取所有 `LOAD_GLOBAL` argval，与模块级定义名（顶层 + 顶层 if/try/with/循环体内赋值）求差；再与 GT 模块级 `STORE_NAME` 交叉：GT 有 recon 无 = **★真缺陷**；GT 也无 = 运行期 `global` 赋值（需人工确认） |
| **模块级语义** | `verify_all_modules.py`（已扩 `<module>` 比对） | 根 code object 的 seq/sem 逐指令对照 GT；模块级名差集（GT 有 recon 无 = NameError 风险） |

**★ 判据盲区根因**：`collect()` 从 `co_consts` 递归收嵌套对象，**根 code object 是 `compile()` 的返回值本身，不在任何 `co_consts` 里**——所以模块级定义从未被比对。

### 8.2 模块级真缺陷的 6 类模式（本轮实锤，全部 GT 字节码驱动修）

| 模式 | 实例 | 根因 |
|---|---|---|
| **模块级常量整组缺失** | `第一步` 29 个（STEAM_守卫_* / TIMEOUT_* / _SUB_STEPS / _OWN_WIN…）、`第四步` 23 个（TIMEOUT/VK_ESCAPE/参考分辨率…） | PyLingual cflow 断裂后重建只写了函数体，模块级常量没还原 |
| **模块级 import 缺失** | `client_ui` 缺 `import threading`、`第四步` 缺 `from workflow_steps import CaptchaError` | 同上 |
| **模块级常量值错** | `workflow_steps._AMMO_EXCHANGE_TABLE` 写成 `'ammo_exchange'`（GT=`'ammo_exchange_log'`） | 凭记忆写错 |
| **STEP_NAME 值错** | `第四步`/`第一步`/`第五步` 写成短名（GT=`'第四步 · 特勤处设施'` 等长名） | 凭记忆写错 |
| **注解/默认值缺失** | `client_config` 缺 `-> Path` 注解、`第五步` 8 处注解 over-build（GT 无注解，recon 加多了） | 注解是 sem 可见的（类体 `LOAD_NAME`+`BUILD_TUPLE`+`MAKE_FUNCTION` flag），必须对齐 GT 的有无 |
| **定义顺序错** | `第一步`/`研发扫AW`/`第五步` 顶层块顺序与 GT 不一致（语义等价但字节码不一致） | 重建时按自己习惯排，非 GT 序 |
| **运行期 `global` 赋值误读** | `研发扫AW._CFG`/`_QTY_RE`、`workflow_steps._HAF_RE` = 函数内 `global X; X = …`，**非模块级定义**（GT 模块级无 STORE_NAME） | 静态查 LOAD_GLOBAL 会误报"缺失"，需与 GT 交叉确认 |

### 8.3 模块级修复工具链（v22 固化）

- **`global_ref_check.py`**：LOAD_GLOBAL 差集 + GT 交叉（★真缺陷 / 运行期赋值 二分）
- **`extract_module_consts.py`**：GT 模块级 `STORE_NAME` 序列 → 逐名还原赋值语句（const/import/复合字面量）
- **`reorder_module_level.py`**：按 GT 模块级 `STORE_NAME` 顺序重排顶层块（★ 带装饰器的函数须用 `min(decorator.lineno)` 定位块起点，否则丢装饰器——本轮实锤踩坑）
- **GT 提取技巧**：模块级常量真值 = 原 pyc 的 `LOAD_CONST`/`BUILD_CONST_KEY_MAP`/`BUILD_TUPLE` 指令流（`dis.get_instructions`）；dict 类常量按 `LOAD_CONST key` + `LOAD_CONST value` + `MAP_ADD` 序列还原，**值类型须逐一对齐（tuple vs list vs str vs int，GT 用 `BUILD_LIST` 包 tuple = list）**

### 8.4 模块级诚实边界（防假新绿）

- **模块级 sem 11/14 一致**（严格逐指令）：剩余 3 个（`client_ui`/`workflow_steps`/`研发扫AW`）的差异 = ① 注解 over-build（recon 加而 GT 无）② 默认值位置（`BUILD_TUPLE` 默认元组 vs `MF` flag 布局）③ 顶层块顺序——**全部语义等价，无运行时缺陷**。
- **函数级 sem 621/652（95.2%）**：剩余 31 处 = §2 白名单 codegen-noise（handler 展开/短路/`PUSH_NULL`/`RETURN_CONST`）+ 4 处 names/vars 漂移已清零。
- **★ 运行期 `global` 赋值**（`_CFG`/`_QTY_RE`/`_HAF_RE`）：模块级查不到 STORE_NAME，但函数内 `global X` + `STORE_GLOBAL X` 已对齐 GT（`_QTY_RE = None` 已加回 `_加载配置` 前，`_HAF_RE` 走函数内 `global`）——**非缺陷，是 Python `global` 声明机制**。
