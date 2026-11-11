# 全课程 31 章：每章每节的核心 + 难点（速查）

> 配套：`/disk/workspaces/jq/course_unidbg/`（军棋实战版，原书《unidbg 逆向工程：原理与实践》陈佳林 机工社 2024）
> 用法：每章 3-6 行速查，"核心"= 该章必须记住的 1-3 个知识点，"难点"= 该章最容易卡住/讲串/踩坑的地方
> 军棋数据对齐：14 JNI 入口（readelf 实测）/ 7 依赖（含 liblog）/ SPAK XOR(key[j%32])+zlib / §124 三项阻塞 / .rodata 非壳
> 生成：2026-11-02，基于全 31 章小节结构 + 红线 §R6 + REVERSE_NOTES §85-§124

---

## 第 1 章 环境准备与快速上手（ch01_env_setup）

**核心**
- unidbg = "在 PC 上跑 Android so" 的模拟框架（CPU 模拟器 + 内存 + 文件系统 + DalvikVM 四层拼起来）
- 军棋走 32 位路径（`AndroidEmulatorBuilder.for32Bit()`），arm64 宿主跑 ARM32 靠 Houdini / QEMU 翻译，但 **ELF 数据位宽不变**（换宿主没用，根因在指令位宽 vs 数据位宽区别）
- 符号调用 vs 地址调用：军棋 strip 后符号表空，只能"按地址调"（14 个 `On*` 入口地址）

**难点**
- **"翻译/重编译成 arm64" 是军棋已验证的死路**（路线 A 汇编级机械翻译 32→64 工作量太大；路线 B 交叉编译只得到简化重写版，非原版）
- **★ 路线纠偏（§124u-z，1.4.3★）**：32→64 是死路，**32→32 是活路**——32 位 ARM → 32 位 C → armv7a 编译回 32 位 so（对象图 32 位布局原样保留），MuMu 渲染主菜单 UI 层；编译级前置 `--hash-style=sysv`（MuMu 32 位 linker 不支持 `.gnu.hash`）+ `jqt_crt.c` 自实现 libc 兼容符号。"能渲染 UI"≠"能玩"（GL 棋盘区未出帧，对象图边界）
- 4 个环节（CPU 指令位宽 / ELF 数据位宽 / SvcMemory 内存映射 / Hook 系统调用）必须逐层理解，否则"换 arm64 宿主"误解反复出现
- "运行时动态翻译 + 保存下来 = 转译指令？" 追问：unidbg 是**逐指令翻译**（指令流保存），不是"重编译整个 so"——保存下来的是翻译产物（影子运行时），非原 so

---

## 第 2 章 模拟执行初探（ch02_sim_exec）

**核心**
- "加载成功" ≠ "能调函数"：军棋 12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT 全通只是"加载"，调 `OnCreate` 还要 18 个 `.text` 常量 + DalvikVM 应答
- 地址调用要 `&~1` 对齐（军棋 14 个 JNI 入口是 Thumb 奇地址，如 `0x233e1`）
- 18 个 `.text` 常量（15 `.text` + 2 `.data`，共 72B）**静态读不出**（`.so` 未重定位，`.got` 槽为零），只能 redroid dump 注入

**难点**
- 军棋 `.text` 是明文（熵 7.10 = 正常机器码），但 18 个常量池值只有 dlopen 后才填——**两个方向都断**（静态读不出 + unidbg 动态也读不出，因为 unidbg 跑不到 0x43ed8），**唯一拿法 = redroid 跑原版 dump 72B**
- 地址调用 4 维度差异（书内样本 vs 军棋）：符号表 / 参数类型 / 依赖库 / C++ 对象图，军棋全比书内复杂

---

## 第 3 章 补环境、Hook 与 Patch（ch03_hook_patch）

**核心**
- 补环境 = 把 so 假设的"真 Android 环境"一个个补上；军棋三大卡点：`.got` 零值 / libGLESv1_CM 无 GPU / 18 个 `.text` 常量
- Hook（拦截保留逻辑）优先于 Patch（改指令破坏逻辑）；军棋 `.text` 100% 对齐（1174/1174 PASS），**基本不用 Patch**
- 军棋去广告：`FindClass("com/baidu/mobads/AdView")` 返 null（补环境 + 去广告一石二鸟）

**难点**
- **军棋"能跑" ≠ "能玩"卡 §124 三项阻塞**：① 上下文相关 BL 解码 bug ② 数据与指令对齐对应缺口 ③ 288MB PROT_NONE mmap——**这三项补环境解决不了**（模拟器精度 + 数据与指令对齐语义 + mmap mock）
- Hook 系统调用（open/read/close）→ I/O 重定向（第 16 章），Hook libc（`__cxa_atexit` 返 0）→ 避免读零 `.got` 崩，Hook JNI（`FindClass(AdView)` 返 null）→ 去广告——**三类 Hook 对应三层补环境**

---

## 第 4 章 ELF 执行视图解析（ch04_elf_view）

**核心**
- ELF 三段式：房证（前 52 字节头）+ 施工图纸（PHDR，给链接器的"内存摆法"）+ 依赖清单（_DYNAMIC，DT_NEEDED 7 个）
- 军棋 so ELF 段地图：`.text`（明文，熵 7.10）+ `.rodata`（明文 118832B）+ `.got`（零值未重定位）+ `.data`（含 2 个运行时常量）
- jelf 是 unidbg 的 ELF 解析器，对应 Android Bionic 的 `elfparse.c`

**难点**
- **`.got` 零值是军棋第一个崩点**：`__cxa_atexit` 读零 `.got` → SIGSEGV，必须填 6 个 GLOB_DAT（从 docker 真 ARM libc 解析）
- strip 后符号表空，军棋 `.dynsym` 只剩 14 个 `Java_com_cnvcs_App_On*` 导出符号——**内部函数无名字，只能按地址**
- 军棋 `.rodata` 唯一高熵点 0x138000（熵 7.87）是**压缩/索引表**（值非有效地址），**非加密代码**——别误判成壳

---

## 第 5 章 Unicorn 初级 + Linker 初探（ch05_unicorn_linker）

**核心**
- unidbg 底层 CPU 模拟器有 dynarmic / unicorn / unicorn2 三种后端；**军棋用 unicorn2**（修 PC off-by-2：dynarmic 对 Thumb `add rX,pc` 用 `(addr+4)&~3` mask，unicorn2 用 `(addr+4)` 不 mask，与真实 ARM 一致）
- Linker 三阶段：加载（mmap 两段）+ 链接（3 类重定位）+ 初始化（12 init_array）
- Keystone（汇编）+ Capstone（反汇编）军棋工作区已用，跟 unidbg 两条线交叉验证
- **★ 调用链追踪 = 函数对齐判据（5.4.1，§124q 自创）+ 调用链工程化（5.4.2，§124u-z 沉淀）**：获取（动态 traceCode/静态 objdump/编译时 gen_render_full 3 粒度）→ 存根（PLT 真实化 JQT_ABS_PTR + blx wire vtable 调度器 + ARM thunk + bx lr 桩）→ 验证（L1 单函数回归→L2 调用链逐点→L3 边统计关键路径缺口=0→L4 MuMu 渲染）→ 流水线（`jq_reverse_pipeline.sh` + 渲染链 4 件套生成顺序固化）

**难点**
- **PC off-by-2 是 dynarmic 的坑**：军棋实测 0xa37fc 存 0x159880 = 不 mask 值，dynarmic 算成 0x15987c（差 4）→ 程序乱跳；unicorn2 修后对
- 军棋 7 依赖（含 liblog）+ 2 个 GL 库（libGLESv1_CM/libGLESv2）无真 GPU → 挂桩函数（`glGetError`/`glBegin` 返 0）
- "模拟 Linker 做没做对" 验证：军棋 12/5312/214/6 逐项打勾（红线 §R6 实测）
- **★ 调用链工程化的 4 个工程坑（5.4.2）**：① 合并边界 `bnd=seed∪full∪stub`（seed 单独漏 427 个定义）② 存根识别用**参数命名**（`full` 命名 `_r0`，`stub` 匿名 `(uint32_t,...)`，用 `return 0` 匹配会漏）③ 生成→wire→插桩顺序固化（渲染链重新生成会覆盖 wire，颠倒则 vtable 方法不执行）④ 32→32 翻译体 `--hash-style=sysv`（MuMu 32 位 linker 不支持 `.gnu.hash`）
- **★ 路线纠偏（§124u-z）**：32→64 翻译体黑屏（对象图错位，死路）；**32→32 翻译体渲染主菜单 UI 层（活路）**——调用链关键路径缺口=0（`callchain_stats.py` 3698 边 97.5% full），黑屏根因=对象图（glClear 类对象未构造），非缺函数

---

## 第 6 章 深入 Linker（ch06_linker_deep）

**核心**
- so 加载拆三段：**搬进**（加载，6 步 mmap）→ **接电话**（链接，3 类重定位）→ **通水电**（初始化，12 init_array）
- 3 类重定位：R_ARM_RELATIVE（5312 个，相对偏移）+ R_ARM_JUMP_SLOT（214 个，PLT 跳板）+ R_ARM_GLOB_DAT（6 个，全局数据）
- 军棋 18 个运行时地址（15 `.text` + 2 `.data`）填完 3 类重定位后**仍静态读不出**（dlopen 才填）

**难点**
- **3 类重定位漏一类就崩**：RELATIVE 漏 → 程序指针全错；JUMP_SLOT 漏 → PLT 跳板跳 0；GLOB_DAT 漏 → `__cxa_atexit` 读零 `.got` 崩
- 军棋 18 个运行时地址是"18 个 `.text` 常量"——填完 3 类重定位后，这 18 个值**只在 dlopen 运行时填**，静态 `.so` 读不出，unidbg 也读不出（unidbg 跑不到 0x43ed8）
- init_array 12 个是"通水电"，军棋全通；但**通水电后不等于能调函数**（还要 18 常量 + DalvikVM）

---

## 第 7 章 模拟 Linker 加载过程（ch07_sim_linker_load）

**核心**
- 模拟环境"加载"这一步就会崩的三大原因：依赖库 PC 不全 / `.got` 零值 / dynarmic PC 算错
- 军棋 32 位 so 最低配置：unicorn2 后端 + AndroidResolver(23)（自带 5 系统库）+ 挂桩 2 GL 库
- 加载全通（12/5312/214/6）但**"能加载 ≠ 能调用"**——还差 18 常量 + DalvikVM + §124 三项阻塞

**难点**
- **依赖库递归加载**：军棋 7 依赖（libc → libm → libdl → libstdc++ → liblog + 2 GL），每个依赖自己又有依赖，unidbg 要递归全配齐
- 加载后内存布局：军棋 base=0x40000000，r-x 段 + rw- 段两段 mmap，**段权限摆错全乱**
- "能加载"是必要不充分条件：军棋 12 init_array 全通 ≠ OnCreate 能调（还卡 §124）

---

## 第 8 章 模拟 Linker 链接过程（ch08_sim_linker_link）

**核心**
- 链接 = "查依赖 → 递归加载 → 填号" 三步；军棋 7 依赖 + 3 类重定位全通
- unidbg Linker 代码 6 段对应 Bionic 简化版：入口 / 载入内存 / 处理依赖 / 重定位 / 初始化 + module 对象 / 执行初始化
- 填号后的 18 个运行时地址是"填完 3 类重定位仍缺"的最后一块

**难点**
- **军棋比书内样本大 1MB，复杂度全翻上来**：书内样本 7 依赖 3 类重定位"小样本"，军棋 **1.43MB/1435156B** so + 18 常量 + C++ 对象图，全翻上来
- unidbg Linker 是 Bionic 简化版，**漏的 4 类**（军棋用到）：TLS / 信号栈 / 异常处理 / vdso——军棋全没用到（否则还会崩）
- "填号"后 18 个运行时地址静态读不出 → redroid dump 72B 注入（工具三件套：`frida-dump/redroid_jni_dump.py` + `redroid_dump_jni.sh` + `gen_runtime_table.py`）

---

## 第 9 章 R0dbg + FindKey（ch09_r0dbg_findkey）

**核心**
- R0dbg = unidbg 的"单步调试器"，把"崩了不知道崩在哪"拆成"追踪 → 拦截 → 主动取值" 三层
- TLS（线程局部存储）= 线程私有变量区，军棋 12 init_array 里有 TLS 设置
- Unidbg_FindKey = "主动找密钥"：军棋 SPAK key 在 `.rodata` VA 0x12a658（32B 明文 `c2 cb e5 e7 ...`）

**难点**
- **反检测**：so 会"检查环境"（`LD_PRELOAD` / `ANDROID_DATA` / 时间戳），发现是模拟器就崩——R0dbg 要"喂假数据骗过反检测"（第 27-30 章详讲）
- TLS 是"线程私有"，unidbg 单线程模拟，TLS 地址固定（军棋 base+0x155000 类）
- FindKey 场景：军棋 SPAK key 是**静态明文**（`.rodata` 非壳，0/118832 不同），**不用 FindKey 动态找**（静态读 0x12a658 即可）；但 18 个 `.text` 常量是"运行时才填"，FindKey 式动态找（军棋这条路走不通，退 redroid）

---

## 第 10 章 源码：AndroidEmulator（ch10_emulator）

**核心**
- AndroidEmulator = 4 个"房间模块"拼成：FileSystem（储物柜）+ Backend（地砖）+ SvcMemory（家具）+ Hook/Patch（电路）
- 军棋 32 位 so 实际配置：`for32Bit()` + `unicorn2` + `AndroidResolver(23)` + 挂桩 GL
- 4 模块配齐才"能加载"，但"能调用"还差 §124 三项阻塞

**难点**
- **Backend 选错差 2 字节**：dynarmic PC off-by-2（军棋 0xa37fc 实证），unicorn2 修
- 4 模块缺一不可：缺 FileSystem 读不到 config XML；缺 Backend CPU 算错；缺 SvcMemory 段摆错坐标；缺 Hook/Patch 读零 `.got` 崩
- "能加载 ≠ 能调用"：军棋 4 模块全配齐跑通 12/5312/214/6，但 OnCreate 仍卡 §124（① BL bug ② 对象图 ③ mmap）

---

## 第 11 章 源码：DalvikVM（ch11_dalvikvm）

**核心**
- DalvikVM = "假 Java 虚拟机"，军棋 so 调 Java 方法（FindClass / GetMethodID / callXxxMethod）时应答
- 4 种"电话端口"：BaseVM（底座）/ DalvikVM（Java 类交互）/ DvmObject（Java 对象替身）/ DvmClass（Java 类替身）
- 军棋 7 个 Java 类（App/GLView/SiteView/AdLis/BtReceiver/TextDlg/InputDlg）全在 DalvikVM 里建替身

**难点**
- **so 假设跑在 Android 上，PC 上没 Dalvik**：军棋 so 调 `FindClass("com/baidu/mobads/AdView")` 没人应答 → 返 null 崩（去广告目标）
- `FindClass` 是广告层拆除关键引用点：全删 `smali/com/baidu/*` 前**必须处理 native 这个引用**（红线）
- DalvikVM 接通后"卡点转移"：军棋 DalvikVM 全通后，卡点从"JNI 应答"转到"native 内部"（§124 三项阻塞）

---

## 第 12 章 源码：模拟执行流程追踪（ch12_sim_trace）

**核心**
- "调用一个函数" = 三步：找入口（strip 后按地址）→ 构造参数（指针 marshal）→ 应答内部调用（JNI / 文件 / libc）
- 军棋 14 个 JNI 入口按地址调（`0x233e1` 等，`&~1` 对齐）
- 三步全通但卡在第 3 步（§124）

**难点**
- **strip 后函数入口无名字**：军棋 `.dynsym` 只剩 14 个 `On*`，内部函数（"翻棋判定"等）全没名字，只能按地址（IDA 静态分析找入口）
- 参数构造：军棋 `OnCreate(env, obj)` 的 env=JNI 函数表指针（232 槽），obj=DvmObject 哈希（不是 C++ 对象指针）
- "应答内部调用"是补环境核心：军棋 so 内部调 Java（FindClass）/ 读文件（open）/ 调 libc（malloc / __cxa_atexit），每缺一个应答就崩一次

---

## 第 13 章 源码：JNI 交互流程追踪（ch13_jni_trace）

**核心**
- JNI 三阶段：注册（`JNI_OnLoad` 录 14 入口）→ 执行（按签名调）→ 应答（DalvikVM 接 Java 方法）
- **13.1.1 参数 marshal**（原书 13.2 核心，课程补入）：DalvikVM 调 native 前，按签名把 Java 参数转 native 指针（军棋 14 入口签名不统一：`int` / `jobject` / `jstring` 混，转错就崩）
- 军棋 `OnCreate(env, obj)` 的 marshal：env=JNI 函数表指针，obj=DvmObject 哈希

**难点**
- **军棋 14 入口签名不统一**：有的 `int` 有的 `jobject` 有的 `jstring`，DalvikVM 调 native 前得**逐个按签名 marshal**——转错（比如 `jobject` 当 `int` 传）就崩
- `JNI_OnLoad` 是"开机自检"：跳过它 14 入口录不进，so 起不来；军棋 `callJNI_OnLoad` 跑 14 入口 + 查 AdView
- 18 个 `.text` 常量是"只有开机后才填的号"：军棋 `JNI_OnLoad` 跑通后，18 个 `.text` 常量才填进 `.text`，但 unidbg 跑不到解包点，静态读不出

---

## 第 14 章 源码：Memory（ch14_memory）

**核心**
- Memory = "管理处 4 窗口"：SvcMemory（摆家具）+ AndroidElfLoader（按图纸搬）+ 符号表解析器（查门牌）+ Hook/Patch（改电路）
- 军棋 6 步 loader 全链路：mmap 两段 → 填 3 类重定位 → 跑 12 init_array → 建 module 对象 → 填 .got → 应答 JNI
- **14.2.5 虚拟模块**（原书 14.2.4 延伸，课程补入）：7 依赖库 + GL 桩 + JNI 函数表 3 类"不是真 so"的内存块挂成 module

**难点**
- **`.got` 零值是段错误源**：军棋 `.got` 槽为零（未重定位），`__cxa_atexit` 读零 → SIGSEGV，必须填 6 GLOB_DAT
- **288MB PROT_NONE mmap 是 §124 ③**：军棋 so 里有 288MB 的 PROT_NONE 段（保护段），unidbg mmap2 mock 处理失败
- 段权限摆错全乱：军棋 r-x 段（代码）+ rw- 段（数据）两段 mmap，权限或坐标错一个全乱

---

## 第 15 章 源码：Hook（ch15_hook）

**核心**
- Hook = "在关键路口设卡"（拦截函数不真执行）；Debugger = "装行车记录仪"（单步 + 看寄存器）
- 军棋两大卡口：`__cxa_atexit`（Hook 返 0）+ `FindClass(AdView)`（Hook 返 null）
- Hook 框架底层：unidbg 在 CPU 模拟里"改写指令"，so 跳某函数时先跳 Hook 桩

**难点**
- **Hook 保留逻辑，Patch 破坏逻辑**：军棋 `.text` 100% 对齐，基本不用 Patch；Hook 优先
- Hook 源码"设卡底层机制"：改 PLT 跳板（R_ARM_JUMP_SLOT 填的号改成 Hook 桩地址），so 调 libc 时先跳 Hook
- Debugger"四个镜头"：runTo（跑到某地址）/ step（单步）/ showRegs（看寄存器）/ breakpoint（断点）——军棋定位 0x43ed8 卡点

---

## 第 16 章 实战：I/O 重定向（ch16_io_redirect）

**核心**
- I/O 重定向 = 把军棋"虚拟路径"（`/data/data/com.cnvcs.junqi/...`）映射到宿主目录（`./junqi_data/...`）
- 军棋 I/O 路径清单（红线 §R6 已 dump）：7 个 config XML（ui/font/image/sound/string/path/uitag）+ one.sjg 存档
- `one.sjg` 军棋对局保存：board 60 格 + turn/stage/winner + flags/mines/pieces + moves

**难点**
- **so 读文件路径是"虚拟的"**：军棋 so `open("/data/data/com.cnvcs.junqi/files/ui.xml")`，PC 上没这路径，必须映射宿主
- **so 写存档到设备路径**：`one.sjg` 写 `/data/data/.../one.sjg`，PC 上要映射宿主目录
- **so 可能 printf 打日志**：军棋 so 日志写到 `log`（liblog），排查时"哑巴吃黄连"，Hook printf 重定向宿主
- I/O 重定向是"补环境第一层"（系统调用层），全通后才到"Java 方法层"（第 11/13 章）

---

## 第 17 章 实战：Debugger 自吐（ch17_debugger_selfdump）

**核心**
- "自吐" = 让 so 自己把关键值打印出来（so 没 printf，你加不了，只能 Hook 入口打印）
- 自吐 3 路径：Debugger 单步（定位崩点）/ Patch 指令（军棋少用）/ Hook 函数（入口打印）
- 军棋 Debugger 定位 OnCreate 卡点 0x43ed8（std::vector 拷贝构造点）

**难点**
- **"0 到底是成功还是内部崩了被你兜住"**：军棋 Hook `__cxa_atexit` 返 0，"跑通"可能内部已崩（被 Hook 兜住），Debugger 单步看寄存器才知道
- **Patch 自吐军棋少用**（17.2 融入"自吐 3 路径"表）：军棋 `.text` 100% 对齐，改 so 字节破坏原版 + 要重签名，优先 Debugger + Hook
- **Hook 打印 18 个 `.text` 常量走不通**：军棋 OnCreate 卡 0x43ed8 跑不到解包点，退 redroid dump 72B

---

## 第 18 章 实战：指针参数（ch18_pointer_args）

**核心**
- 指针参数"硬骨头"：`JNIEnv*` 不是真指针是函数表（232 槽），`jobject` 不是真对象是哈希
- 军棋 `OnCreate(env, obj)` 构造：env=`vm.getJNIEnv()`（JNI 函数表指针），obj=`vm.addGlobalObject(glviewCls.newObject())`（DvmObject 哈希）
- 识别加密算法：军棋 SPAK（XOR+zlib）替代原书"识别 AES"

**难点**
- **`JNIEnv*` 232 槽函数表**：军棋 so 拿 env 调 `(*env)->FindClass(env, name)`，env 是函数表不是对象，转指针就崩
- **`jobject` 是哈希不是指针**：DalvikVM 里 GLView 对象用整型哈希（`0x1234abcd`）引用，so 拿 obj 调 `GetObjectClass` 时 DalvikVM 查"哈希↔对象"映射
- 军棋 14 入口签名不统一（`int` / `jobject` / `jstring`），指针参数要**逐个按签名 marshal**（同 13.1.1）

---

## 第 19 章 SPAK 资源解密还原（ch19_base64）

**核心**
- **军棋 SPAK 实际 = `XOR(key[j%32]) + zlib inflate`**（R5_spak.md 确证，推翻"魔改 Base64"推测）
- key 32B 在 `.rodata` VA `0x12a658`（明文 `c2 cb e5 e7 ce 5c ...`），**静态可读**（`.rodata` 非壳）
- 9 包 883 条目（data 121 / datah 272 / datam 189 / datax 301），data 组按 `MD5(资源路径)` 寻址（38+ 路径验证）

**难点**
- **原书标题"魔改 Base64 还原"是书内样本**，军棋实际是 SPAK（XOR+zlib），**章标题保留"魔改 Base64"作原书对照，正文按军棋 SPAK 讲**
- 9 包 883 条目的结构：datah/datam/datax 按 UI 布局 XML 索引（**非路径 MD5**），data 组才按 MD5
- 解包走 redroid：军棋 unidbg 跑不到解包点（卡 §124），SPAK 解包在 redroid 跑原版 so 时完成，unidbg 只能"Hook 解包入口识别算法"

---

## 第 20 章 动态分析内存数据（ch20_mem_data）

**核心**
- "静态看 IDA 有没有 + 动态跑 unidbg 用没用" 交叉验证；军棋 `.rodata` 静态 = 运行时（0/118832 不同）→ **证 `.rodata` 非壳**
- **军棋棋盘数据（R3/REVERSE_NOTES 实测，课程补入 20.4）**：60 格十字形（每格 12B），38 子/方（rank 40 司令…rank 32 工兵 + 3 雷 + 1 旗 + 2 炸弹），走子判定 `0x39e14` / 走子执行 `0x3a15c` / 布局校验 `0x6a3b0`
- rank 小 = 强（1 司令 vs 9 工兵，1 胜，REVERSE_NOTES §16 确证）

**难点**
- **18 个 `.text` 常量两个方向都读不出**：静态读不出（`.got` 零值未重定位）+ unidbg 动态读不出（跑不到 0x43ed8），**唯一拿法 = redroid dump 72B**
- 高熵点 0x138000（熵 7.87）是**压缩/索引表**（值非有效地址），**非加密代码**——别误判成壳
- 走子规则"按标准军棋对齐"是 R3 简化版实现（`rev64/reverse/src/junqi.cpp`），**不是原版汇编级逆向**（红线 §64 位 native 诚实边界）

---

## 第 21 章 主动调用 fork 进程（ch21_fork）

**核心**
- fork 在模拟环境是"死路"：unidbg 单线程模拟，fork 要起第二个"人"（子进程），模拟不了
- 军棋 ad SDK 的 fork 场景：广告 SDK 后台进程，unidbg 处理 = "不模拟 fork，直接跳过"（返 0 走"退"分支）
- fork / vfork / clone 三兄弟：fork 返回 0（子）/ 正数（父），vfork 共享地址空间，clone 全控

**难点**
- **fork 返回值两种身份**：子进程返 0，父进程返子 PID——unidbg 只能模拟一种（父进程视角），子进程"共享内存"COW 模拟不了
- 军棋场景"跳过 fork"：ad SDK fork 后台进程，unidbg 直接"走通但退"（返 0 让 so 走"退"分支），不模拟子进程
- 跳过 fork 的代价：ad SDK 后台逻辑不跑（军棋去广告目标，**不跑正合适**）

---

## 第 22 章 补环境入门（ch22_env_basics）

**核心**
- 补环境按"系统调用 → Java 方法 → 指令 → C++ 对象图" 四层补（军棋前 3 层全通，第 4 层补不了）
- **22.2 按原书 5 小节拆**（课程早期并成"3 步代码"丢了细度，已拆回）：参数获取 / 代码初始化 / 目标函数 / 补环境说明 / 补环境实战
- 军棋补环境天花板 = §124 三项阻塞（① BL bug ② 对象图 ③ mmap）

**难点**
- **补的顺序有讲究**：先系统调用（I/O）→ 再 Java 方法（JNI）→ 再指令（Hook）→ 最后 C++ 对象图（补不了）；顺序错"改 A 崩 B"
- **第四层补不了的原因**：std::vector 0x43ed8 的 begin/end/capacity 来自未初始化对象，unidbg 填不出"这个 vector 有多少元素"（那是真实 C++ 运行时语义）
- redroid dump → unidbg 注入：军棋 18 常量走这条路（`frida-dump/` 工具三件套），unidbg 拿到值后指令能跑，但 §124 仍卡

---

## 第 23 章 标识记录（ch23_identifier）

**核心**
- "标识记录" = 两层：JNI_OnLoad 环境补全（军棋 14 入口 + 7 类 + AdView）+ 文件标识（存档 + 配置）
- 军棋 DiagJni 实证：`JNI_OnLoad` 跑通后 14 入口录进、7 类 resolveClass、AdView 返 null
- 文件标识：军棋 7 个 config XML + one.sjg 存档全落宿主（I/O 重定向）

**难点**
- **14 个入口名必须 readelf 实测**：军棋 14 入口 = OnCreate/OnDestroy/OnResult/OnResume/OnEvent/OnKey/OnUpdate/OnGraphChange/OnBluetooth/OnPause/OnGraphCreate/OnTimer/OnInput/OnTouch（**无 OnResize/OnQuit/OnSave/OnRestore**）
- "广告层要假装存在"：军棋 AdView 返 null 但 FindClass 不能返 null（JNI 规范），要"FindClass 返 null + GetMethodID 返 0" 组合
- 文件标识"读存档忘干净"：军棋 one.sjg 续局（OnCreate 读 + OnPause 写），读不到走"新游戏"分支

---

## 第 24 章 设备风控（ch24_device_risk）

**核心**
- so 查"设备信息"三大类：android_id（设备唯一标识）/ getFilesDir（目录路径）/ 分辨率（选资源目录）
- **24.1 ActivityThread**（原书 24.1.1，课程补入）：军棋 `com.cnvcs.App` 继承 Application，DalvikVM mock getApplication 返 App 替身；军棋 so 未走 ActivityThread 查设备（补了不用）
- 军棋 14 入口设备风控取样点（24.5）：`OnCreate` 查 android_id + 分辨率，`OnTouch` 查分辨率（渲染），`OnBluetooth` 查设备标识

**难点**
- **军棋 7 个资源目录按分辨率选**（`data/{320,480,480x,720,720s,800}/`）：分辨率 mock 错 → so 选错资源目录 → 渲染崩
- ActivityThread "补了不用"：军棋 so 设备风控主要走 android_id / getFilesDir / 分辨率，不走 ActivityThread——**通用框架需 mock，军棋场景下不调用**
- 14 入口取样点要逐个标：军棋 14 入口里 4 个查设备（OnCreate/OnTouch/OnBluetooth/OnUpdate），10 个不查（OnEvent/OnKey 等）

---

## 第 25 章 补环境加强（ch25_env_enhance）

**核心**
- "补环境补到 90% 卡住"的三大卡点：C++ 对象图（补了也不对）/ init_array bx bug（模拟器全局分支损坏）/ no-init_array 全局未初始化（两条路死锁）
- 军棋 §124d-g 里程碑：OnCreate 推进到 ~90%（vecfix 37 次 + JNI 37+ 调用 + GLView 创建 + EGL 配置 + setContentView）
- **25.3.2 无障碍服务**（原书 25.3.2，课程补入）：军棋无此场景（走 GLView 渲染 + OnTouch 输入），但 unidbg 通用框架需 mock AccessibilityService（其他 app 适用）

**难点**
- **C++ 对象图"补了也不对"**：std::vector 0x43ed8 的 begin/end/capacity 三个指针来自未初始化对象，unidbg 填"能跑"但"语义不对"（vector 元素数填不出）
- **bx bug 可绕过 9 个 init**（§124h）：init[0..8] 安全到达 0x43ed8，init[9..11] 任何单跑都破坏 bx → 最大安全集 = init[0..8]
- **两条路死锁**：init[9+]→bx bug（模拟器 bug），init[0..8]→全局未初始化（std::string 全局地址表），突破需"修复 bx bug"或"手动逆向 std::string 全局"（工作量大）

---

## 第 26 章 补环境总结（ch26_env_summary）

**核心**
- 补环境 7 步（军棋 §124d-g 修订版）：选后端（unicorn2）→ 配库（AndroidResolver 23）→ 挂桩（GL）→ 建 VM（DalvikVM）→ 调入口（14 `On*`）→ 填常量（redroid 72B）→ 破 §124（卡住）
- 崩溃信号 → 该补哪层：SIGSEGV（读零 `.got` / 段摆错）→ 补 1 层；SIGFPE（除 0 / 移位）→ 补指令；READ_PROT（保护段）→ 补 §124 ③
- 军棋 4 条红线 + 1 条停止条件：① 不包简化版当完整版 ② 不 dlopen 32 位库 ③ 不删 baidu smali 前处理 AdView ④ 不改 native 调用签名 ⑤ "能跑 ≠ 能玩"停止

**难点**
- **"改 A 崩 B" 的补丁干扰**：军棋补 100 个卡点，每个"能跑"，但改 A 崩 B——补丁间互相干扰，要有优先级（系统调用 > Java 方法 > 指令 patch）
- **崩溃信号分不清哪层**：SIGSEGV 可能是"读零 `.got`"（补 1 层）也可能是"段权限摆错"（补加载）也可能是"§124 ③ mmap"（补不了）——信号相同，根因不同
- **停止条件**：军棋"补到顶发现语义补不了"（std::vector 元素数），该停还是继续补？→ 停止，转 redroid 跑原版

---

## 第 27 章 Anti：环境变量检测（ch27_env_detect）

**核心**
- so 查环境变量三大类：Linux（`LD_PRELOAD` 反 Frida）/ Android（`ANDROID_DATA` 反 unidbg）/ 组合（so 的"身份证核验"）
- 补一套"真 Android 环境变量"：军棋 mock `ANDROID_DATA=/data` + `ANDROID_ROOT=/system` + `LD_PRELOAD=""`
- 环境变量源码解析：Java 层（`System.getenv`）+ Native 层（`getenv`）

**难点**
- **`LD_PRELOAD` 反 Frida**：军棋 so 查 `LD_PRELOAD` 非空 = Frida 注入 → 崩；mock `LD_PRELOAD=""` 绕过
- **`ANDROID_DATA` 反 unidbg**：军棋 so 查 `ANDROID_DATA` 不存在 = unidbg → 崩；mock `ANDROID_DATA=/data`
- 环境变量组合是"身份证核验"：军棋 so 查 3-5 个变量组合，**全 mock 才不崩**（漏一个变量 so 走"检测命中"分支）

---

## 第 28 章 Anti：xHook 检测（ch28_xhook）

**核心**
- xHook 检测 7 维度：`.text` 被改字节 / unidbg 填 `.got` 的值 / unidbg 默认内存基址 / Hook 桩跳板 / 符号表缺失 / 反汇编工具特征 / 时间戳
- 军棋 32 位 so 的 xHook 检测"基本正常"：军棋 so 查 xHook 的 7 维度，**未全命中**（`.text` 明文，没被改字节）
- unidbg 反 xHook 3 招：补 `.got` 值（模拟 dlopen 重定位）/ 改内存基址（模拟真 Android ASLR）/ 跳过 Hook 桩

**难点**
- **".text 被改字节" 检测**：军棋 `.text` 100% 对齐（1174/1174 PASS），没被改 → 检测"正常"；但**影子运行时改 `.text` 字节**（机械翻译 C）→ 检测命中
- **换 arm64 宿主没用**：指令位宽 vs ELF 数据位宽区别，换宿主只改指令翻译，**ELF 数据（`.got` / `.rodata`）位宽不变** → xHook 检测照命中
- unidbg 默认基址 0x40000000 是"分析环境"指纹：军棋 so 查基址 = 0x40000000 → unidbg → 崩；mock ASLR 随机基址绕过

---

## 第 29 章 Anti：JNI 层常见函数处理（ch29_jni_reversal）

**核心**
- `FindClass` 反制：军棋 AdView 返 null（去广告），7 个 Java 类正常 resolveClass
- `GetMethodID` 反制：返 0 跳过某方法（军棋跳过广告方法）
- 14 个 `On*` 入口不能应答错：方法号必须正常应答（军棋 14 入口 readelf 实测名，别用 OnResize/OnQuit/OnSave/OnRestore）

**难点**
- **FindClass 应答错就崩**：军棋 so 调 `FindClass("java/lang/String")` 返 null → NPE 崩；必须 resolveClass 正常
- **GetMethodID 应答 0 跳过方法**：军棋广告方法（AdView.onAttachedToWindow）GetMethodID 返 0 → so 走"方法不存在"分支，跳过广告
- **14 入口不能应答错**：军棋 14 入口（OnCreate…OnTouch）方法号必须正常应答，应答错 → JNI 异常 → so 崩

---

## 第 30 章 Anti：常规检测总结（ch30_detect_summary）

**核心**
- 8 维度"查 vs 反"矩阵：基地址 / JNI 调用 / 类 / fd / uname / 运行时间 / Unicorn / Frida，逐项过
- 军棋 8 维度结果：基地址正常（不用改）/ JNI 14 入口正常 / 类 7 + AdView / fd config XML / uname mock / 时间戳 mock / **Unicorn 切 unicorn2 已规避 PC off-by-2** / Frida `LD_PRELOAD=""` 绕过
- "反检测全通" 会被误读成"能玩"：军棋 8 维度全通 ≠ OnCreate 能调（还卡 §124）

**难点**
- **unidbg 填 `.got` 的值会被查**：军棋填 6 GLOB_DAT（真 ARM libc），xHook 检测查 `.got` 值 = 真 libc 地址 → unidbg → 命中；mock 成 dlopen 重定位值绕过
- **检测 Frida = `LD_PRELOAD`**：军棋 so 查 `LD_PRELOAD` 非空 = Frida；mock 空绕过（第 27 章详讲）
- "反检测全通 ≠ 能玩"：军棋 8 维度全通 + 12/5312/214/6 全通，但 OnCreate 仍卡 §124（① BL bug ② 对象图 ③ mmap）——**检测通过是"能加载"，不是"能调用"**

---

## 第 31 章 生产环境部署（ch31_deploy）

**核心**
- 把 unidbg 包成 REST 服务（Spring Boot + unidbg-boot-server），军棋 7 步启动 + 3 加强项
- 军棋 application.yml（unidbg-boot-server 配置）：backend=unicorn2 / for32Bit / AndroidResolver(23) / 挂桩 GL / 18 常量注入
- REST 接口示例：`POST /junqi/onCreate` → 调军棋 OnCreate → 返结果

**难点**
- **生产部署诚实边界（红线 §R6 + §124d-g）**：军棋 31 章部署"能调一个函数" ≠ "能玩"，§124 三项阻塞仍卡
- 手工调试不可复用 → REST 服务可复用：军棋 14 入口逐个 REST，批量跑
- 结果没法给别人用 → REST + 文档：军棋 OnCreate 返 GLView 对象，REST 序列化为 JSON

---

## 全课程 5 大"卡点"速查（跨章）

| 卡点 | 军棋数据 | 涉及章 | 能不能解 |
|---|---|---|---|
| `.got` 零值 | 6 GLOB_DAT 未重定位 | 3/4/6/14/15 | ✅ 填真 ARM libc |
| 18 个 `.text` 常量 | 15 `.text` + 2 `.data` = 72B | 2/6/8/9/17/19/20/5(5.4.2) | ★ 分两条路：**unidbg 侧**仍 redroid dump 72B 注入（ch02/03/06/08）；**翻译体侧**（§124z-3）改**静态位移**固化 `jqt_mem.c` `g_jq_got_tbl`（跨进程通用，redroid dump 路线已废弃，见 `frida-dump/FRIDA_NOTES.md`） |
| §124 ① BL bug | 上下文相关 BL 0x7965a→0x6eaea | 3/5/10/15/25 | ❌ 模拟器精度 bug（unicorn2 上下文相关） |
| §124 ② 对象图 | std::vector 0x43ed8 未初始化 | 3/11/14/22/25/1(1.4.3★)/5(5.4.2) | ❌ C++ 语义缺失（unidbg 填不出）；★ 32→32 翻译体保对象图布局（活路） |
| §124 ③ mmap | 288MB PROT_NONE 段 | 14/25/26 | ❌ unidbg mmap2 mock 失败 |
| GL 渲染 | libGLESv1_CM/v2 无真 GPU | 3/5/7/10 | ⚠️ 挂桩（无真 GPU 不能验证渲染）；★ 32→32 翻译体在 MuMu 真 GPU 渲染主菜单 UI 层 |
| 反检测 8 维度 | 基址/JNI/类/fd/uname/时间/Unicorn/Frida | 27/28/29/30 | ✅ 全 mock 绕过（但 ≠ 能玩） |

---

## 全课程"两条路"对照（unidbg vs 翻译体，§124u-z 后升级，跨章把控）

军棋"把 32 位 so 跑起来"有**两条互补路**，课程两条都覆盖，读全课程时按此对照把控前后章节：

| 维度 | unidbg（模拟执行，离线） | 翻译体（重编译回 32 位，真机 MuMu） |
|---|---|---|
| **核心思想** | unicorn2 把 32 位 ARM 动态翻译成 x86，简化 Linker 补环境 | 32 位 ARM 指令 → 32 位 C（指令级）→ armv7a 编回 32 位 so |
| **对象图** | 32 位原样保留（不翻译） | 32→32 布局原样保留（32→64 才错位，故 32→64 影子库黑屏、32→32 活） |
| **课程落点** | Ch 4-15 原理/源码 + Ch 16-31 补环境/反检测 | Ch 1(1.4.3★ 路线 A 活路) + Ch 5(5.4.2 调用链工程化) + `frida-dump/FRIDA_NOTES.md` |
| **军棋现状** | 加载全通（12/5312/214/6），OnCreate 卡 §124 三项阻塞 | MuMu 渲染主菜单 UI 层（标题+三士兵+红旗+5 按钮），GL 棋盘区未出帧 |
| **通用判据** | §124q 调用链判据（Ch 5 5.4.1）+ 调用链工程化 L1-L4（Ch 5 5.4.2）两条路共用 | 同左 |
| **诚实边界** | "指令 100% 对齐 ≠ 能玩"（§122） | "能渲染 UI ≠ 能玩"（GL 出帧 + 触摸进对局还差，§124z） |

**两条路的"调用链"是同一套判据（§124q）的两个落地**：
- unidbg 侧：动态 traceCode（受 BL bug 限制）+ 静态 capstone 调用图
- 翻译体侧：`gen_render_full.py`（编译时生成）+ `callchain_stats.py`（边统计验证，关键路径缺口=0）
- **动态调用链在 unidbg 下被 BL bug 卡死时，真机替代路 = Frida Jnitrace**（`frida-dump/FRIDA_NOTES.md` 第三节 + Ch 13 13.4b）
- **★ 第三条路：unidbg + Frida 一对一逐步对齐（§124z 黑屏破题方向，Ch 13 13.4c）**——unidbg 负责"模拟加载 + 调用链 + 逻辑"（数据侧不行），Frida（MuMu 32 位 frida-server）负责"真实填充对象图真值"，**逐字段把 unidbg 的"零值对象图"填成"真对象图"** = 纹理类 vtable 0x1595c0 的 slot[13]（值 0x9d459 → glClear 方法）所在对象构造出来 → 渲染出帧（★ §124zz-3 纠正：`0x1595f4` 非 vtable 基址，是纹理类 0x1595c0 的 slot[13]，旧"glClear 类对象"是幻影）

**前后对应检查（审计用）**：
- Ch 1 1.4.3★（32→32 活路 + `.hash` 编译坑）↔ Ch 5 5.4.2（调用链工程化 5 节）↔ Ch 13 13.4b（Jnitrace 真机替代）↔ Ch 13 13.4c（unidbg+Frida 一对一对齐）四者数据一致（`libjunqi_arm32_aligned.so` MuMu 渲染 / `callchain_stats.py` 3698 边 97.5% / `--hash-style=sysv` / `frida-dump/FRIDA_NOTES.md` §八）
- Ch 5 5.4.1（§124q 调用链判据，理论）↔ Ch 5 5.4.2（§124u-z 工程化，落地）一理论一工程，不重复
- `frida-dump/FRIDA_NOTES.md` §八（unidbg+Frida 一对一对齐）↔ Ch 13 13.4c（课程版）↔ Ch 13 13.4b（Jnitrace 真机 trace）三者"真机 trace 替代 + 对象图逐字段填"表述一致
- **§124z 黑屏破题三判据**：① 调用链对齐（关键路径缺口=0，已证）② 对象图对齐（glClear 类对象逐字段填 Frida 真值，13.4c）③ 返回值 A/B 逐点比（§124q 点 3）

---

## 一句话总纲（全课程）

> unidbg = 在 PC 上"模拟一个假 Android"（CPU + 内存 + 文件 + Java VM 四层）；
> 军棋 32 位 so 的"能加载"（12/5312/214/6 全通）早解决，"能调用"卡 §124 三项阻塞
> （BL bug / 对象图 / mmap）——**补环境是"指令能跑"的工具，不是"功能一致"的工具**（红线 §122）。
> **两条路把 so 跑起来**：unidbg（离线模拟，卡 §124）+ 32→32 翻译体（真机 MuMu，渲染主菜单 UI 层）；
> **两条路共用一套判据**：§124q 调用链（Ch 5 5.4.1）+ 调用链工程化 L1-L4（Ch 5 5.4.2）。
> 课程 31 章 = "怎么把 so 搬进 PC + 怎么让它跑起来 + 怎么让它跑通 + 怎么部署"，
> 军棋实战验证到"能加载 + 能跑指令 + 主菜单 UI 渲染"，"能玩"（GL 棋盘出帧 + 触摸进对局）需继续攻对象图（§124②）。
