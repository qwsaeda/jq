# 课程参考资料全景（unidbg 一手来源 + 各章对照）

> 课程主对齐：陈佳林《unidbg 逆向工程：原理与实践》（机工社 2024，31 章教材框架）。
> **但最权威的一手资料是 unidbg 官方代码库 + 作者 r0ysue 的博客**（r0ysue 是 unidbg 维护者，原书是教材、博客是作者实操笔记）。
> 本清单把"哪章该参考哪个一手来源"标清，避免课程只对齐一本书、漏了官方一手资料。

## 资料全景（按一手程度排）

| 层级 | 资料 | 地址 | 跟军棋课程的关系 |
|---|---|---|---|
| **① 官方代码库** | `r0ysue/unidbg` | github.com/r0ysue/unidbg | **第 4-15 章（原理/源码解析）的真源**：jelf/AndroidEmulator/DalvikVM/Memory/Hook 源码对着这个库读，不是只看原书 |
| **② 作者配套课件** | `r0ysue/UnidbgBook` | github.com/r0ysue/UnidbgBook（Chap01-31） | 原书配套代码示例：第 1-3/16-31 章的实战代码可对着它核对 |
| **③ 作者博客（原理深讲）** | r0ysue 博客园系列 | cnblogs.com/r0ysue | 第 5-9 章（Unicorn/Linker/R0dbg）一手原理，比原书细：`Unidbg 文档慢更(一/二)` `Unidbg-Linker 源码分析(上/下)` `Unidbg 的底层支持-Unicorn` `Android Linker 详解(一/二)` |
| **④ 官方 AutoDocs（API 级）** | unidbg 仓内 `_autodocs/` | context7.com/zhkl0228/unidbg（收录） | 第 2/12/13 章查 API 参数签名：`loadLibrary`/`callFunction`/`getJNIEnv`/`newObject` 的 API 级细节 |
| **⑤ 社区实战文** | FreeBuf/Ayuge/geekbyte/CN-SEC | 见下 | 第 16-31 章（补环境/反检测/部署）"别人怎么实战"：快手 stat64 补文件夹、绿洲 unidbg 模板、微博 hook_RegisterNative |
| **⑥ 原书（教材框架）** | 陈佳林《unidbg 逆向工程》 | 机工社 2024 | 课程 31 章结构主对齐对象 |
| **⑦ 视频课** | 偏马站"Unidbg 原理与实操"71 课时 | （凯神/肉丝） | 跟原书 31 章对应的视频版（军棋课程视频是 AI 生成，这个是教材人讲版） |

### ⑤ 社区实战文明细

| 文章 | 来源 | 对应军棋章节 |
|---|---|---|
| "Unidbg 补环境实战：补环境入门"（FreeBuf） | r0ysue/AndroidSecurityStudy 配套 | 第 22 章（补环境入门） |
| "unidbg 补环境"（Ayuge 部落阁） | ayuge.top | 第 22-23 章（补环境 + 标识记录） |
| "app 逆向 native 层安全逆向分析（八）：unidbg 补前置环境 + io 重定向" | geekbyte/博客园 | 第 16 章（I/O 重定向） |
| "从案例中学习 unidbg 补系统调用（快手 stat64 补文件夹）" | gla2xy 博客 | 第 16/24 章（系统调用 + 设备风控） |
| "记一次使用 unidbg 的安卓逆向过程"（CN-SEC，含 traceCode 三层） | cn-sec.com | 第 15 章（Hook/Debugger） |
| "SO 逆向之 Unidbg 模拟执行入门"（绿洲案例） | onejane.github.io | 第 2/3 章（模拟执行 + 补环境） |

## 各章一手来源对照（课程该"对着哪读"）

| 章 | 主对齐（原书） | 一手来源（补充该看） | 备注 |
|---|---|---|---|
| 1-3（基础） | 原书 1-3 | ② UnidbgBook Chap01-03 + ④ AutoDocs（API 签名） | 军棋把书内 md5 样本映射成军棋 so |
| 4-9（原理：ELF/Unicorn/Linker/R0dbg） | 原书 4-9 | **① unidbg 代码库**（jelf/AndroidElfLoader 源码）+ **③ r0ysue 博客**（Linker 源码分析上/下、Unicorn 底层） | **军棋第 4-9 章讲源码，真源是 unidbg 代码库，不是只引原书** |
| 10-15（源码解析） | 原书 10-15 | **① unidbg 代码库**（AndroidEmulator/DalvikVM/Memory/Hook 源码目录） | 军棋第 10-15 章的"源码解析"应对着 unidbg 源码读 |
| 16-26（实战/补环境） | 原书 16-26 | ⑤ 社区实战文（快手/绿洲/微博案例）+ ② UnidbgBook Chap16-26 | 军棋把样本映射成军棋 32 位 so |
| 27-31（反检测/部署） | 原书 27-31 | ⑤ 社区反检测文 + ① unidbg 代码库（xHook/InlineHook 实现） | 军棋 32 位 so 反检测 8 维度 |
| 军棋独有（§124q 调用链判据） | **原书无** | 军棋 `rev64/reverse/phases/REVERSE_NOTES_04_shadow_r6.md` §124q | 军棋做 32→64 影子运行时时自创，原书"补环境跑原版"场景无"对齐判据"需求 |

## 关键提醒（军棋课程 vs 原书/一手来源）

1. **原书是"教材框架"，①③④ 是"一手真源"**：军棋课程第 4-15 章讲源码，读者应**对着 `r0ysue/unidbg` 代码库读**（原书只给"教材视角"，代码库给"实现真源"）。
2. **unidbg 官方没有独立"文档站"**：一手 API 文档在 unidbg 仓内 `_autodocs/`（context7.com 收录），不是单独的 doc 网站。
3. **军棋 §124q 调用链判据是增量**：原书/社区都没"调用链作为函数对齐判据"（原书场景是"跑通原版 so + 补环境"，无"A vs B 是否一致"问题；军棋做了 32→64 影子运行时才有对齐需求）。这是军棋自创方法论，课程已标注（ch05 5.4.1 + CHAPTER_DIFF_REPORT.md）。
4. **军棋比原书样本简单**：原书覆盖加壳/花指令/VM 样本，军棋是**明文 so 非壳**（.rodata 静态=运行时 0/118832 不同，无自解密例程），反汇编直接可做。

## 军棋课程独有数据（不在任何书里）

| 数据 | 来源 | 课程落点 |
|---|---|---|
| 14 个 JNI 入口（readelf 实测） | `rev64/reverse/phases/` + readelf | 全课程（1.5/12/23/29/30） |
| 7 个 DT_NEEDED（含 liblog） | readelf | 第 4/6/8/10 章 |
| SPAK = XOR(key[j%32])+zlib，key VA 0x12a658 | `rev64/reverse/phases/R5_spak.md` | 第 18/19 章 |
| 军棋棋盘 60 格 38 子 + 走子判定 0x39e14 | `rev64/reverse/phases/R3_rules.md` + REVERSE_NOTES | 第 20 章 |
| §124 三项阻塞 + OnCreate 90% 里程碑 | `rev64/reverse/phases/REVERSE_NOTES_04_shadow_r6.md` | 第 3/10/25/26 章 |
| §124q 调用链判据 + OnCreate 12 目标 | 同上 §124q | 第 5 章 5.4.1 |
| 影子库 14MB 黑屏（对象图错位） | `rev64/dist/` | 第 1 章 1.4.3 |

## 陈佳林 Frida 系列 3 本评估（unidbg 课程的姊妹书）

unidbg 课程作者陈佳林另有 3 本 Frida 系列书。军棋是**单机游戏 + Native/SO 层逆向**，逐本评估是否吸收：

| 书 | 出版 | 层 | 军棋吸收 | 理由 |
|---|---|---|---|---|
| 《安卓 Frida 逆向与抓包实战》 | 2021，清华 | Java 层（Hook/HTTP 抓包/okhttp 自吐/Socket） | ❌ **不吸收** | 军棋单机游戏**无 HTTP 协议栈**，抓包/okhttp 跟军棋 so 逆向不搭 |
| 《安卓 Frida 逆向与协议分析》 | 2022，清华 | 协议层（注册登录/付费绕过/聊天机器人） | ❌ **不吸收** | 军棋**无服务器协议**（翻棋/吃子 + 蓝牙联机，非注册登录/付费），协议分析场景不存在 |
| 《Frida Android SO 逆向深入实践》 | 2023.10，清华 | **SO/Native 层**（NDK/IDA/联合调试 JNI/Jnitrace/算法保护） | ✅ **吸收 1 处** | **跟军棋同赛道**（都是 Native/SO 层）。吸收点 = **Jnitrace 看 JNI 调用流**（补 unidbg 课程 ch13 13.4b + 独立参考文档） |

**只第 3 本相关**：军棋是 32 位 `libjunqi.so` 的 Native 层逆向，前 2 本是 Java/协议层（军棋无此场景）。

**第 3 本吸收的 1 处（Jnitrace）落点**：
- **ch13 13.4b**：unidbg 侧 vs Frida Jnitrace 侧看 JNI 调用流对照表（§124q 动态调用链被 unidbg BL bug 卡死时，Frida 真机 trace 是替代路）
- **`frida-dump/FRIDA_NOTES.md`（独立参考文档）**：军棋工作流已落地的 Frida 用法（18 地址 dump / Jnitrace / frida-server 部署 / 能干什么不能干什么）——**不混进 unidbg 课程**（Frida 是真机工具栈，跟 unidbg 离线模拟是两条互补路；独立成文让干活会话可系统参考）

**为什么 Frida 跟军棋强相关**：军棋 `frida-dump/` 目录**已在用 Frida**（`redroid_jni_dump.py` attach 读 18 地址运行时值 → `jni_runtime_values.json` → 喂 `jni_aligned.c` 注入层）。Frida 不是"参考"，是**拿 18 地址 72B 运行时值的唯一路线**（AGENTS.md 红线：这 18 值静态读不出，仅 dlopen 重定位才填）。

## 二进制分析 4 本书评估（ELF 格式 / 花指令 / 指令重排 / 二进制插桩，补强课程）

用户提出用 4 本二进制分析书补全课程（尤其"`.rodata`/`.got` 没结合 ELF 文件格式讲" + "花指令/指令重排是不是自定义段"）。逐本评估是否吸收（我没有逐本读过这些书，以下是按书名/目录推断的核心知识点，**吸收前需逐本核对原书**）：

| 书 | 出版 | 核心知识点 | 军棋吸收 | 理由 |
|---|---|---|---|---|
| 《二进制分析实战：打造自己的二进制插桩、分析和反汇编的 Linux 工具》（2021） | 机工社 | **二进制插桩（Dobby/inline hook 原理）** + 自写反汇编工具 + 字节码操作 | ✅ **吸收 2 处** | 插桩原理补 ch15 Hook 章；"自写反汇编"跟军棋 `thumb2c_full.py`（capstone 反汇编→机械翻译）直接相关 |
| 《Linux 二进制分析》（2017） | 人邮 | **ELF 格式深讲**（段属性/段类型/自定义段/动态段/重定位类型全谱系） | ✅ **吸收 3 处** | 军棋 ch04 只讲"9 个 PT_LOAD 段"，缺 ELF 段属性（`PHDR` flags/`SHDR`/自定义段 `PT_GNU_RELRO`/花指令自定义段），需补 |
| 《Linux 系统安全基础：二进制代码安全性分析基础与实践》（2023） | 电子 | **二进制代码安全**（栈溢出/ret2stack/CFI/relro/`PT_GNU_RELRO`/`PT_GNU_STACK`） | ⚠️ **部分吸收** | 军棋 so 是**明文非壳**（无自解密），安全分析场景（ret2stack 利用）不直接相关；但 `PT_GNU_RELRO` 段（ELF 格式）+ `__cxa_guard_acquire`（C++ 静态初始化保护）可补 ch04/ch10 |
| 《有趣的二进制：软件安全与逆向分析》（2017） | 人邮 | **逆向分析入门** + 花指令/指令重排基础 + ARM 汇编基础 | ⚠️ **部分吸收** | 花指令/指令重排**不是自定义段**（是**指令层混淆**，军棋 so 无壳，不用管）；但"花指令 vs 自定义段"的区分值得补进 ch04（避免读者误以为花指令=自定义段） |

**4 本书的核心知识点 → 课程落点**：

| 知识点 | 落点 | 军棋价值 |
|---|---|---|
| **ELF 段属性全谱系**（`PHDR` flags R/W/X/`SHDR`/自定义段 `PT_GNU_RELRO`/`PT_GNU_STACK`/`PT_INTERP`） | ch04 4.2/4.3（补"9 个 PT_LOAD 段"之外的段） | 军棋 ch04 只讲 PT_LOAD，缺段属性（`.got` 为何 rw-、`.relro` 段为何只读） |
| **重定位类型全谱系**（`R_ARM_RELATIVE`/`R_ARM_JUMP_SLOT`/`R_ARM_GLOB_DAT`/`R_ARM_32`/`R_ARM_PC32`...） | ch06 6.2（补 3 类之外的重定位类型） | 军棋 ch06 只讲 3 类（RELATIVE/JUMP_SLOT/GLOB_DAT），缺全谱系 |
| **花指令 vs 自定义段 的区分**（花指令 = **指令层混淆**，自定义段 = **ELF 格式层**，两者不同层） | ch04 新增 4.x + ch07（补"军棋 so 无壳，不用管花指令"） | 军棋 so 是明文非壳，花指令场景不存在；但课程该讲清"花指令不是自定义段"（避免读者混淆） |
| **二进制插桩原理**（Dobby inline hook 的"5 字节跳板 + 函数序言" vs xHook GOT hook 的"改 PLT 槽"） | ch15 Hook 章（补插桩两种实现的字节码级差异） | 军棋 ch15 讲"Hook/Patch 框架"，缺"inline hook 怎么改 `.text` 前 5 字节跳板" |
| **`PT_GNU_RELRO` 段**（RELRO = 重定位完成后把 `.got`/`.data` 改只读，防 GOT 覆盖） | ch04 4.2（补"`.got` 为何 rw- 不是 r--"）+ ch10 | 军棋 `.got` 段是 `rw-`（可写），`PT_GNU_RELRO` 段决定"重定位完成后锁不锁" |
| **`__cxa_guard_acquire`（C++ 静态初始化保护）** | ch04/ch10（补"C++ so 的静态对象构造"） | 军棋 so 含 C++（`libstdc++.so` 依赖），`__cxa_guard_acquire` 是静态对象构造保护（军棋翻译体 `jqt_crt.c` 自实现） |

**4 本书评估结论**：
- **《二进制分析实战》**：✅ 吸收 2 处（插桩原理 + 自写反汇编跟 `thumb2c_full.py` 相关）
- **《Linux 二进制分析》**：✅ 吸收 3 处（ELF 段属性/重定位全谱系/`PT_GNU_RELRO`）
- **《Linux 系统安全基础》**：⚠️ 部分吸收（`PT_GNU_RELRO` + `__cxa_guard_acquire`，ret2stack 利用场景不相关）
- **《有趣的二进制》**：⚠️ 部分吸收（花指令 vs 自定义段区分 + ARM 汇编基础，逆向入门场景军棋已超）

**新建文档**：`course_unidbg/ELF_FORMAT_SUPPLEMENT.md`（4 本书的 ELF 格式知识点 + 军棋 ch04/ch06/ch10/ch15 补强）
