# Python 3.12 字节码反编译 — 经验/规范/复用/参考资料

> 来源：给 `zrax/pycdc` 打补丁让它正确反编译 Python 3.12（`.pyc`），过程中踩过的坑、
> 沉淀的规范、可复用的产物、以及网上参考资料。
> 适用：任何需要从 `.pyc`/PyInstaller/Nuitka 包还原 Python 源码的逆向场景。

---

## 一、经验总结（踩坑 → 定位 → 修法）

### 1.1 工具选型：先排雷再动手

| 工具 | 支持 3.12？ | 备注 |
|---|---|---|
| `uncompyle6` / `decompyle3` | ❌ 最高 3.8 | 已弃维护 |
| `zrax/pycdc`（Decompyle++） | ⚠️ 部分 | C++，**需打补丁**（本文主题） |
| `zrsx/pycdc`（fork） | ⚠️ 部分 | 有 PR 在跟进 3.11 异常表 |
| `ejfkdev/pycdc-rs`（Rust 重写） | ✅ 到 3.15 | 较新，号称 528/528 语义覆盖 |
| `syssec-utd/pylingual`（ML） | ✅ 3.6–3.12 | 75% 完美还原；需 torch + 模型，重 |

**经验**：先 `pip` 试 `decompyle3`（确认不行）→ 上 `pycdc` → 不够就补丁 → 还不够再考虑 `pylingual`。
`pycdc` 是 C++ 单二进制、可离线、可改，**适合补丁路线**；`pylingual` 适合追求完美还原且有 GPU/网络。

### 1.2 解包链：搞清楚 .pyc 在哪一层

PyInstaller 包常见 3 层，每一层的解法不同：

```
exe（PyInstaller bootloader）
 └─ CArchive（footer magic "MEI\0c\0b\0a\0b\0e"，TOC 在尾）
    └─ PYZ（magic "PYZ\0" + pyc magic + 4 字节 toc offset）
       └─ marshal code object（每个模块一个）
```

- **CArchive TOC 解析**：entry = `[structlen:4][name\0][offset:4][csize:4][usize:4][flag:1][typecode:1]`，typecode `s`=source、`m`=module、`z`=PYZ、`o`=runtime option。**坑**：3.11+ 的 entry 布局没变，但 `structlen` 的语义是"本 entry 总字节数（含 structlen 自身 4 字节）"。
- **PYZ 解析**：`marshal.loads(blob[toc_off:])` → 得到 `list[(name, (typecode, offset, length))]`；每个模块 `blob[off:off+len]` zlib 解压后是**裸 marshal code object**（无 16 字节 pyc 头）。
- **补 pyc 头**：用本机 `importlib.util.MAGIC_NUMBER`（3.12 = `cb0d0d0a`）+ `\0\0\0\0`×3 拼到裸 marshal 前，pycdc 才认。
- **Nuitka**：核心模块编成 `.pyd`（PE32+，含 `__nuitka__` 标记），**不可还原为 Python**；只能 `strings` + 符号提取。Cython 是 `__pyx_` 标记，区别开。

### 1.3 pycdc 的核心缺陷：3.11+ 零成本异常表

这是**最大的坑**，也是 pycdc 3.12 支持不完整的根因。

**背景**（CPython `InternalDocs/exception_handling.md`）：
- 3.11 起，`try/except` 不再用 `SETUP_FINALLY`/`POP_BLOCK` 指令，改成 `co_exceptiontable` 元数据。
- **所有 except handler 集中排在函数尾部**，顺序与源码嵌套**由外到内**一致。
- 表项格式（**全部以 code unit 为单位，1 code unit = 2 字节**）：
  ```
  start  (varint, 7-bit)   保护区起点（inclusive）
  size   (varint, 7-bit)   保护区长度
  target (varint, 7-bit)   handler 偏移
  depth<<1 | lasti (varint)
  ```
  varint 编码：`S X dddddd`（S=start-of-entry 标记，X=continue，低 6 位是数据）。

**pycdc 的问题**：它的块机沿用 ≤3.10 模型，假设"handler 紧跟 try 体"。结果：
1. except 子句被渲染进 try 体内（缩进错）；
2. try 与 except 一一错配（外层 try 反而缺 except，多余的 except 挤在内层）；
3. try 套 for/while 时，**同一个 try 被拆成多个 try 块**。

### 1.4 关键修法（按踩坑顺序）

| # | 现象 | 根因 | 修法 |
|---|---|---|---|
| 1 | 所有类体截断 `# WARNING: Decompyle incomplete` | `MAKE_CELL`/`COPY_FREE_VARS`（3.11+ 闭包）无 case | 加 no-op case |
| 2 | `super().m()` 报 unsupported | `LOAD_SUPER_ATTR`（3.12）无 case | 实现：pop self/class/super，push `ASTBinary(ASTCall(super), attr, BIN_ATTR)`；顺手 pop NULL marker |
| 3 | 类体出现 `__classcell__ = None` | 合成变量泄露 | STORE_NAME 时 `if name=="__classcell__": break` |
| 4 | `@dataclass class X` 渲染为 `X = <NODE:12>()` | 装饰器用 `CALL 0` 套住 ASTClass，pycdc 当普通 call | ASTClass 加 `decorators` 列表；CALL 时若 func 是 NODE_CLASS 且无参 → 取栈上的装饰器挂到类，push 回 class |
| 5 | `with` 体出现 `None(None, None)` | `SWAP_A` 是空实现（推两个空 tuple） | 重写：3.11+ SWAP n 是**配对交换**（swap TOS↔2nd, 3rd↔4th, ...） |
| 6 | except 体 `return None`（应是真实值） | `PUSH_EXC_INFO`/`CHECK_EXC_MATCH`/`POP_EXCEPT` 栈效应错 | PUSH_EXC_INFO push 占位；CHECK_EXC_MATCH **只 pop type，push 占位 exc + compare**（3.11+ 异常对象在栈上）；POP_EXCEPT pop 1 |
| 7 | try/except 结构全乱 | handler 在函数尾部，pycdc 当紧跟 | **延迟 except 机制**：try 体结束时把 container 收口并登记到 `pending_handler[target]`；遍历到 target 时再创建 except 块 |
| 8 | try 块吞掉后续代码 | try 的 end 用了 handler 偏移（在尾部） | try 的 end 改用**保护区终点** `entry.end_offset` |
| 9 | try 套 for 时一个 try 变多个 | for 循环把保护区拆成多条同 target 表项 | **合并同 target 的 depth=0 表项**（取 min_start / max_end） |
| 10 | `except E as e` 截断 client_ui（153 行） | `LOAD_FAST_AND_CLEAR`（3.12）无 case | push ASTName(local) |
| 11 | `assert` / async 残留 | `LOAD_ASSERTION_ERROR`/`END_SEND`/`CLEANUP_THROW`/`BEFORE_ASYNC_WITH` 无 case | 加简单 case |
| 12 | 字典/集合字面量、`f(*args)` 不全 | `MAP_ADD`/`SET_ADD`/`DICT_MERGE`/`CALL_FUNCTION_EX`/`CALL_INTRINSIC_1/2` 无 case | 实现（CALL_INTRINSIC_1 = `list.append(x)` 之类的特化调用） |
| 13 | `x is None` 跳转缺失 | `POP_JUMP_IF_NONE/NOT_NONE`（3.12）无 case | 复用 POP_JUMP_IF_FALSE，cond 改成 `ASTCompare(x, None, CMP_IS/CMP_IS_NOT)` |

**最关键的两个洞察**：
- **#7+#8**：3.11+ 的 handler 不在 try 体后面，必须"延迟到 target 才创建 except 块"，且 try 的 end 必须是保护区终点而非 handler 偏移。
- **#9**：循环会把一个 try 的保护区拆成多条同 target 表项，**必须按 target 合并**，否则 pycdc 给每次迭代建一个 try。

### 1.5 调试方法论（高效定位）

1. **最小复现**：手写 5–10 行 `.py`，`py_compile` 编译，pycdc 跑，对比输出。比直接拿大文件调试快 10×。
2. **`dis.dis(code_object)`**：拿到精确字节码 + `ExceptionTable`，是 ground truth。
3. **`code.co_exceptiontable` + 手写 varint 解析**：独立验证 pycdc 的表解析是否正确（我一度怀疑 pycdc 的 varint 有 bug，独立解析后确认它是对的，省得改错地方）。
4. **stderr 调试输出**：在 ASTree.cpp 关键分支 `fprintf(stderr, "[DBG] ...\\n", pos)`，跑一个文件看流。**用完一定删掉**（`-Werror` 会因 unused 变量炸）。
5. **回归测试**：每改一处，重跑最小复现 + 一个已知能过的简单文件，确认没退化。
6. **量化验收**：`ast.parse` 全量统计可解析率 + `grep "Unsupported opcode"` 残留计数。

### 1.6 诚实边界（不掩盖）

- pycdc 的块机本质是单遍、基于 `blocks` 栈的树构建；3.11+ 异常的"handler 在尾部"破坏了这个假设。我的延迟机制修了**多数**场景，但 **try 套 for/while 再套 try** 时仍有错位（内层 try 的保护区终点落在外层控制流块内部，收口时序对不齐）。
- **生成器表达式 / lambda** 渲染为 `(lambda .0: for p in .0: ...)` —— 内容在，语法不合法。这是 pycdc 的老问题，3.12 没改善。
- 残留 ~66% 文件不能直接 `ast.parse` 通过，但**语义内容（docstring/常量/调用/控制流基本块）100% 保留**。

---

## 二、规范（下次再做时的纪律）

### 2.1 流程规范

1. **先识别源码形态**：纯 `.pyc` / PyInstaller / Nuitka / Cython — 决定能否还原。
2. **解包到 marshal code object 层**，统一补 pyc 头，再喂反编译器。
3. **保留语义提取作为 ground truth**：即便反编译器输出不能 ast.parse，也要产出"docstring + 全部字符串常量 + names + disassembly"的语义级文档（100% 准确），作为反编译输出的校验基准。
4. **反编译器输出 + 语义提取 + 字节码 dis**，三件套交付，不单独依赖任何一件。

### 2.2 pycdc 补丁规范

- **改 ASTree.cpp 加 case**：放在逻辑相近的已有 case 旁边（如 `LOAD_FAST_AND_CLEAR` 放 `LOAD_FAST` 附近）。
- **栈效应必须精确**：改之前先查 CPython `dis` 文档或 `Python/ceval.c` 的 `TARGET(X)` 实现，确认 pop/push 数量。**栈失衡会让后续全乱**，且很难定位。
- **3.11+ 分支用 `mod->verCompare(3, 11) >= 0` 守卫**，不要破坏 ≤3.10 路径。
- **改完跑回归**：最小复现 + `ast.parse` 全量统计。
- **`-Werror`**：不要留 unused 变量、不要拿 `std::string` 和 `const char*` 比较（用 `strValue()`）。
- **`std::stack` 无下标**：要遍历就拷到 `std::vector`（`while(!tmp.empty()){snap.insert(begin, tmp.top()); tmp.pop();}`，注意 insert begin 让 snap[0]=bottom）。
- **`PycRef<T>` 无隐式转 `PycRef<U>`**：用 `x.cast<U>()`。
- **`PycExceptionTableEntry` 无默认构造**：`std::map::operator[]` 会炸，用 `emplace`。

### 2.3 文档规范

- **补丁清单表**：每条补丁写明"现象 / 根因 / 修法 / 影响模块数"。
- **可复现命令**：`cd build && cmake .. && make`，写明依赖（cmake/g++）。
- **诚实口径**：覆盖率、可解析率、已知缺陷全部量化写出，不写"已支持"如果实际有边角 case 不行。

---

## 三、复用产物

### 3.1 已打补丁的 pycdc（可复用）

位置：`/tmp/opencode/pycdc/`
- `ASTree.cpp` +395 行、`ASTNode.h` +11 行（共 13 处补丁，见上表）
- 复现：`cd /tmp/opencode/pycdc/build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)`
- 用法：`./build/pycdc <file.pyc>` → stdout 出 `.py`
- **适用 Python 3.11/3.12**（3.13 未测，opcode 表有差异）

### 3.2 Python 端解包脚本（可复用模式）

```python
# 从 PyInstaller exe 提取所有 .pyc（核心模式）
import marshal, zlib, struct, importlib.util
exe = open('app.exe','rb').read()
MAGIC = b'MEI\x0c\x0b\x0a\x0b\x0e'
i = exe.rfind(MAGIC)
_, pkg_len, toc_off, toc_len, _, _ = struct.unpack_from('!8siiii64s', exe, i)
base = <overlay_start>  # PE 末尾
p = base + toc_off
while p < base + toc_off + toc_len:
    elen, = struct.unpack_from('!i', exe, p); p += 4
    entry = exe[p:p+elen]; p += elen
    nul = entry.index(b'\0')
    name = entry[:nul].decode()
    off, clen, ulen = struct.unpack_from('!iii', entry, nul+1)
    flag = entry[nul+13]; tc = chr(entry[nul+14])
    if tc == 'z':  # PYZ
        pyz = exe[base+off:base+off+clen]
        toc = marshal.loads(pyz[struct.unpack_from('!i',pyz,8)[0]:])
        for mod,(tc2,o,l) in toc:
            raw = pyz[o:o+l]
            data = zlib.decompress(raw) if ... else raw
            # 补 pyc 头
            with open(mod+'.pyc','wb') as f:
                f.write(importlib.util.MAGIC_NUMBER + b'\0'*12)
                f.write(data)  # 裸 marshal code object
```

### 3.3 语义提取模板（可复用）

```python
import marshal, types, dis
def load(p):
    d = open(p,'rb').read()
    return marshal.loads(d[16:])  # 跳过 16 字节 pyc 头
def walk(co, path=""):
    yield path, co
    for c in co.co_consts:
        if isinstance(c, types.CodeType):
            yield from walk(c, path+"."+c.co_name)
for name, c in walk(load('mod.pyc')):
    strs = [k for k in c.co_consts if isinstance(k,str)]
    print(f"--- {name} args={c.co_varnames[:c.co_argcount]}")
    for s in strs: print(f"    | {s!r}")
    print(f"    > {c.co_names}")
```

**为什么这个有用**：pycdc 输出可能语法不合法，但 docstring/常量/names 是 code object 的直接属性，**100% 准确**。安全分析（找 URL、密钥、API、IO）靠这个就够，不依赖反编译器的语法正确性。

### 3.4 质量量化脚本（可复用）

```python
import ast, os
ok=bad=0
for f in os.listdir('src'):
    if not f.endswith('.py'): continue
    try: ast.parse(open(f' src/{f}',encoding='utf-8',errors='replace').read()); ok+=1
    except SyntaxError: bad+=1
print(f"{ok}/{ok+bad} ({ok*100//(ok+bad)}%)")
```

---

## 四、参考资料（网上有）

### 4.1 工具

| 资源 | 链接 | 用途 |
|---|---|---|
| **zrax/pycdc**（Decompyle++） | https://github.com/zrax/pycdc | C++ 反编译器，本文补丁基础。**上游 2026-04 合并了 PR #599 "Support Python 3.11 exception table try/except flow"**，与本文工作重叠，可对比 |
| zrsx/pycdc（fork） | https://github.com/zrsx/pycdc | 活跃 fork，PR 跟进 3.11+ |
| **ejfkdev/pycdc-rs**（Rust 重写） | https://github.com/ejfkdev/pycdc | 号称 528/528 语义、663/663 行为，支持到 3.15；新选择 |
| **syssec-utd/pylingual** | https://github.com/syssec-utd/pylingual | ML 反编译器，3.6–3.12，75% 完美还原（S&P'25 论文）。在线版 https://pylingual.io |
| decompyle3 | https://pypi.org/project/decompyle3/ | 最高 3.8，已过时 |

### 4.2 字节码 / 异常表规范（权威）

| 资源 | 链接 | 关键内容 |
|---|---|---|
| **CPython InternalDocs/exception_handling.md** | https://github.com/python/cpython/blob/main/InternalDocs/exception_handling.md | **异常表格式权威说明**：7-bit varint、(start,size,target,depth<<1\|lasti)、code unit 单位、handler 在尾部的原因 |
| CPython `Lib/dis.py` `_parse_exception_table` | 同上仓库 | Python 端解析实现，可直接参考 |
| CPython `Python/assemble.c` `assemble_exception_table` | 同上 | 表的生成端 |
| CPython `Python/ceval.c` `get_exception_handler` | 同上 | 运行时查表逻辑 |
| Python issue 40222 | https://bugs.python.org/issue40222 | "Zero cost" exception handling 设计提案 |
| `dis` 模块文档 | https://docs.python.org/3/library/dis.html | 每个 opcode 的栈效应说明（改 pycdc 必查） |

### 4.3 教程 / 博客

| 资源 | 链接 | 内容 |
|---|---|---|
| idafchev — Decompiling python above 3.9 | https://idafchev.github.io/blog/Decompile_python/ | 实操：pycdc 构建 + 用法 |
| skuznetsov Medium 系列 | https://medium.com/@skuznetsov/python-bytecode-decompilation-comprehensions-exceptions-and-async-part-2-3-6bc8b3cf3b05 | 理论：模拟原理、comprehension/异常/async 的反编译思路 |
| StackOverflow — What is the ExceptionTable in dis | https://stackoverflow.com/questions/77542619/what-is-the-exceptiontable-in-the-output-of-dis | 3.10 vs 3.11 字节码对比（SETUP_FINALLY → 异常表） |
| S&P'25 PyLingual 论文 | https://www.computer.org/csdl/proceedings-article/sp/2025/223600a052/21B7QZB86cg | ML 反编译的方法论 |

### 4.4 我这次没走但值得记的路线

- **直接读 CPython 源码**：`Python/ceval.c` 每个 `TARGET(OPCODE)` 的实现就是栈效应的权威定义。改 pycdc 前查这个比查文档更准。
- **`dis.stack_effect(op, arg)`**：Python 3.12 自带，能直接给出某 opcode 的净栈效应，用来验证 pycdc 的实现是否匹配。
- **PyLingual 在线版**（pylingual.io）：单文件可直接上传，适合快速验证某个 .pyc 能否完美还原，再决定要不要本地搭。

---

## 五、一句话带走

> Python 3.11+ 的零成本异常表把 try/except 的 handler 搬到了函数尾部，**这是 pycdc（以及多数老反编译器）3.12 支持不完整的根因**。修法是"延迟 except + try 用保护区终点 + 合并循环拆分的同 target 表项"。如果不想改 C++，用 `ejfkdev/pycdc-rs` 或 `pylingual`。无论用哪个，**保留 marshal 级语义提取作为 ground truth**，不要单靠反编译输出。

> **姊妹篇（方法论层）**：本篇只讲工具（补丁/解包/参考）。"重建后怎么验收、怎么防假绿、
> 批量处理纪律"见 **`DECOMP_RECONSTRUCTION.md`** —— 核心一句："能 parse ≠ 对"，
> 唯一硬判据是逐指令对照原字节码（seq/sem）+ code object 不缺，PyLingual 的潜藏
> 语义 bug 必须逐函数对 GT 反汇编实锤。

### 4.5 模块级（<module>）判据盲区（v22 固化）

> **★ 最隐蔽的假绿源**：三层判据（parse/结构/函数级 sem）**全都不含根 code object**。
> 模块级常量/导入/注解/默认值/定义顺序缺失 = 运行时 NameError，但 `missing_inventory`
> （只查 code object 在不在）+ 函数级 sem（只查嵌套对象）**双双 0 命中** → 误判"闭环"。
> 本轮一次性暴露 **54 个**（`第一步` 29 / `第四步` 23 / `workflow_steps` 9 / `client_ui` 2…）。

**固化进判据的 3 个工具**（`/tmp/opencode/tqc_exe/`）：

| 工具 | 作用 | 用法 |
|---|---|---|
| `global_ref_check.py` | LOAD_GLOBAL 差集（模块级定义名 − 内建）× GT 模块级 STORE_NAME 交叉 | `python3 global_ref_check.py`（★真缺陷 / 运行期赋值 二分） |
| `extract_module_consts.py` | GT 模块级 STORE_NAME → 逐名还原赋值（const/import/复合字面量） | `python3 extract_module_consts.py 第四步 --missing` |
| `reorder_module_level.py` | 按 GT 模块级 STORE_NAME 序重排顶层块 | `python3 reorder_module_level.py 第五步 --apply` |

**★ 踩坑固化**：
1. **`reorder_module_level` 丢装饰器**：ast 的 `FunctionDef.lineno` 指向 `def` 行，装饰器行在其之前——
   必须用 `min(decorator.lineno)` 作块起点，否则 `@contextlib.contextmanager` 被裁掉（本轮实锤）。
2. **dict 类常量值类型须逐一对齐**：GT `BUILD_LIST` + tuple 常量 = **list**（非 tuple）；
   `recon` 写 tuple → sem 的 `BUILD_LIST` 缺失 → 假差异。值类型看 GT 构建指令，不看语义等价。
3. **运行期 `global` 赋值误读为"缺失"**：`研发扫AW._CFG`/`_QTY_RE`、`workflow_steps._HAF_RE` 在
   GT 模块级**无** STORE_NAME（函数内 `global X; X = …`）——静态 LOAD_GLOBAL 查会误报，
   须与 GT 模块级 STORE_NAME 交叉后才知"非缺陷"。
4. **注解是 sem 可见的**：类体/函数注解 → `LOAD_NAME`+`BUILD_TUPLE`+`MAKE_FUNCTION` flag，
   加注解（GT 无）= over-build，删注解（GT 有）= 漏；**以 GT 的 `MAKE_FUNCTION` flag +
   注解元组为准**，不以"代码可读性"为准。

## 六、与 DECOMP_RECONSTRUCTION 的路由

> 本篇 = **工具层**（pycdc 补丁 / 3.11 异常表 / 解包 / 模块级判据盲区工具链 / 参考）。
> `DECOMP_RECONSTRUCTION.md` = **方法论层**（判据 / 防假绿 / 批量纪律 / 高频真 bug / 诚实边界 /
> §八 模块级判据盲区完整规范）。
> **改动落点路由**：
> - 工具/补丁/解包脚本 → 本篇（§3 复用产物 / §4 参考 / §4.5 模块级工具链）
> - 判据/白名单/批量纪律/真 bug 模式/诚实边界 → `DECOMP_RECONSTRUCTION.md`
> - 当前状态/进度台账 → `reconstructed/CURRENT_STATE.md`
> - 全量判据一键 → `verify_all_modules.py` + `global_ref_check.py` + `sem_triage.py`
