# 第2章 unidbg 模拟执行初探

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地实战靶机：军棋 `lib/armeabi/libjunqi.so`（ELF32 / ARM / EABI5 / stripped）

## 痛点：为什么"加载好了"不等于"能调函数"？

你按第 1 章把军棋 so 搬进 unidbg（mmap + 重定位 + 跑 12 个 init_array 全通），
觉得"能跑了"。但一调 `OnCreate` 就发现**"加载成功"和"能调通"是两回事**。

**痛点 1：函数入口在哪？（strip 后符号表空）**
军棋 so 是 stripped（删除符号信息），`.dynsym` 里只剩 14 个 `Java_com_cnvcs_App_On*` 导出符号，
内部函数（比如"翻棋判定"）全没名字。你想调一个内部函数，得先**静态分析找出它的入口地址**，
再用"地址调用"。

**痛点 2：参数怎么传？（指针要手动构造）**
军棋 `OnCreate(JNIEnv *env, jobject obj)` 要两个指针参数：
- `JNIEnv*`：一张 232 个函数指针的表（JNI 接口表），不是普通指针
- `jobject`：Java 对象的"哈希"（DalvikVM 里对象的引用），不是 C++ 对象指针

模拟环境里这两个都要**手动构造**，传错了 so 就崩。

**痛点 3：so 内部调 Java / 读文件 / 调 libc，谁来应答？（缺一个就崩）**
so 跑到一半 `FindClass("java/lang/String")` / `open("data/config/ui.xml")` / `strlen(...)`，
你的 unidbg 环境没人应答就崩。这是"补环境"的核心。

**痛点 4：军棋 14 个 JNI 入口是"Thumb 奇地址"，调用前要 `&~1` 对齐**（这是军棋特有的坑，下面细讲）。

## 为什么这样做：符号调用 vs 地址调用，军棋只能走"地址调用"

书里两种调法：

| 调法 | 适合 | 军棋能不能用 |
|---|---|---|
| **符号调用**（`findSymbol("funcName")`） | 符号表完整的 so（书内样本） | ❌ 军棋 strip 后符号表只剩 14 个 `On*`，内部函数没名字 |
| **地址调用**（`call(0x233e1, ...)`） | strip 的 so / 调内部函数 | ✅ 军棋走这条（14 个 `On*` 入口有地址，内部函数靠 IDA 找） |

**军棋为什么 14 个入口是"奇地址"（关键坑）**：

军棋 so 是 ARM/Thumb 混合编译。JNI 注册表（`.dynsym`）里存的函数地址是"奇数"
（比如 `OnCreate = 0x233e1`），bit0=1 表示"跳 Thumb 模式"。

```
0x233e1 (奇数, Thumb 模式)
  bit0 = 1  →  告诉 CPU "下一条指令是 16 位 Thumb 指令"
  实际执行起点 = 0x233e1 & ~1 = 0x233e0 (偶数)
```

**调用前必须 `&~1` 对齐到偶指令**，否则翻译器（unidbg 的指令分析）可达集为空 →
跑出 0KB 函数体（军棋红线 §R6：JNI 入口是 Thumb 奇地址，不在偶指令表，
`translate_func` 起点要 bisect 对齐到最近偶指令，否则可达集空 → 0KB body）。

类比：门牌号是"奇数"（带 Thumb 标志），但开门要按"偶数"门（实际指令起点），
拿错门牌进不去。

## 2.1 第一个 NDK 项目（书内做法，军棋简化）


> **本节核心**：
> 1. 书内写 C 函数 md5 调 unidbg；军棋直接拿现成 libjunqi.so（1MB strip）地址调用
> 2. 4 维度差异表（符号表/参数/依赖/C++ 对象图）：军棋 strip 后符号表空，只能按地址
> 3. 军棋无 NDK 写 so 环节，但 64 位简化重写版（rev64/src/junqi.cpp）是 NDK r25b 编译
>
> **本节难点**：
> 1. strip so 调函数前需 IDA 找入口地址，军棋 14 入口地址在 .dynsym
> 2. 「已知入口 + 已知参数」是调通前提，军棋 18 常量是「只有运行时才填的值」

书里教你"写个 C 函数 md5，在 unidbg 里调"——先有个"已知入口地址、已知参数"的简单 so，
把 unidbg 调用流程跑通。军棋没有"自写 so"环节——军棋的目标 so 是**现成的 `libjunqi.so`**
（1MB，strip），直接拿来调。
**但书内样本和军棋的关键差异**：
| 维度 | 书内样本 | 军棋 `libjunqi.so` |
|---|---|---|
| 函数入口 | 完整符号表（按名字调） | **strip 后符号表空**（只能按地址调） |
| 参数 | 简单类型（int / String） | **指针参数**（`JNIEnv*` 232 函数表 + `jobject` 哈希） |
| 依赖 | 常见 libc | **7 个依赖含 libGLESv1_CM**（无真 GPU 要挂桩） |
| 内部逻辑 | 无 C++ 对象图 | **std::vector 对象图 0x43ed8**（§124 三项阻塞卡点） |
所以 2.1 跳过"写 so"环节，直接看 2.2 的"怎么调现成 strip so"。
## 2.2 军棋地址调用实战（逐环节能干什么）


> **本节核心**：
> 1. 4 步：findSymbol(&~1) → 构造参数（env + obj）→ call → 验返回
> 2. 军棋 14 入口是 Thumb 奇地址（0x233e1），调用前 &~1 对齐，否则可达集空
> 3. OnCreate(env, obj)：env=getJNIEnv()（JNI 函数表 232 槽），obj=addGlobalObject(DvmObject)
>
> **本节难点**：
> 1. 军棋 DiagJni：OnCreate 推进到 std::vector 0x43ed8（vector 拷贝构造点，§124 卡点）
> 2. 「&~1 对齐」是军棋特有坑：奇地址不在偶指令表，translate_func 起点 bisect 对齐

![符号调用 vs 地址调用](assets/ch02_symbol_vs_addr.png)


![主动调用 4 步前置](assets/ch02_call_prepare.png)

军棋调 `OnCreate` 的完整路径（军棋实测可跑通）：

```java
// 前置 4 步(军棋实测全通)
// 1. 加载 so(跑 12 个 .init_array + 重定位)
DalvikModule dm = vm.loadLibrary(new File("lib/armeabi/libjunqi.so"), true);

// 2. 注册 Java 侧应答(找类/找方法/调方法)
vm.setJni(this);  // this 实现 Jni 接口:findClass/getMethodID/callXxxMethod*

// 3. 跑 JNI_OnLoad(so 的开机自检,注册 14 个 On* 方法)
vm.callJNI_OnLoad(emulator, dm.getModule());

// 4. 构造参数 + 按地址调 OnCreate(Thumb 奇地址 0x233e1)
Pointer jniEnv = vm.getJNIEnv();            // JNIEnv*(232 函数表)
int objHash = vm.addGlobalObject(new DvmObject<>());  // jobject(哈希)
long ret = vm.getMemory().call(emulator, 0x233e1L, 0, jniEnv, objHash).singleValue();
// 军棋 DiagJni:OnCreate 推进到 std::vector 对象图 0x43ed8(§124 三项阻塞卡在这)
```

### 逐环节"我们能干什么 / 不能干什么"


> **本节核心**：
> 1. 能/不能表：地址调用能找 14 入口、构造指针参数、验返回值
> 2. 不能：单步穿越 §124 三项阻塞（读未初始化值必崩被安全阀兜成 0）
>
> **本节难点**：
> 1. 地址调用「能调」≠「调通」：调 OnCreate 推进到 0x43ed8 后 §124 卡住
> 2. 构造参数错（jobject 当 int 传）就崩，军棋 14 入口签名不统一

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **找入口** | 按地址调 14 个 `On*`（strip 后唯一门牌）；内部函数用 IDA 找地址 | 按符号调内部函数（符号表空） |
| **对齐入口** | `&~1` 把 Thumb 奇地址对齐到偶指令（军棋红线 §R6） | 不对齐直接调（可达集空 → 0KB body） |
| **构造 JNIEnv\*** | `vm.getJNIEnv()` 拿模拟的 232 函数表 | 用真机的 `JNIEnv*`（模拟环境没有） |
| **构造 jobject** | `vm.addGlobalObject(new DvmObject<>())` 注册拿哈希 | 传真 C++ 对象（DalvikVM 里是"哈希"不是对象指针） |
| **调函数** | 地址调用 `call(0x233e1, ...)` | 突破 0x43ed8（§124 三项阻塞：① BL 解码 bug ② C++ 对象图 ③ 288MB mmap） |
| **应答内部调用** | `setJni` 实现 `findClass`/`getMethodID`/`callXxxMethod`（军棋 AdView 返 null 去广告） | 完整 C++ 对象图（std::vector 语义缺失） |
## 2.3 军棋的卡点：18 个 `.text` 常量（静态读不出）


> **本节核心**：
> 1. 18 个 .text 常量（15 .text + 2 .data = 72B）只有 dlopen 后才填真值
> 2. .so 未重定位，.got 槽为零 → 静态文件读不出；unidbg 动态也读不出（跑不到 0x43ed8）
>
> **本节难点**：
> 1. 两个方向都断 → 早期拿法 = redroid 跑原版 dump 72B 注入（工具三件套）；**★ §124x-z 后演进为"静态位移固化 `jqt_mem.c` `g_jq_got_tbl`"（无需运行时 dump，见 2.3 路线演进表）**
> 2. 18 个常量是「仅 dlopen 重定位填」，非壳自解密（已证 .rodata 静态即明文）
> 3. redroid dump 工具：frida-dump/redroid_jni_dump.py 读 18 地址 + gen_runtime_table.py JSON→C

军棋 `OnCreate` 一调，就卡在**运行时 18 个地址**上（红线 §R6）：
```
军棋 14 个 JNI 入口内部:
  ldr rX, [pc, #N]   →  读 .text 内嵌常量池 0xbd10f…
  
  这些常量 = dlopen 重定位后才填的真值(.so 静态文件里是零/密文)
  军棋 15 个 .text 常量池地址 + 2 个 .data 地址 = 共 18 地址 × 4B = 72 字节
```
**为什么"静态读不出"**：
- `.rodata` 是明文（已证 0/118832 不同，无自解密）——**这部分静态能读**
- 但 `.text` 内嵌常量池（`ldr rX,[pc,#N]` 读的）**只有 dlopen 重定位后才填真值**
  ——**这部分静态读不出**（`.so` 文件里是零/密文）
**军棋拿这 18 个值的办法（红线 §R6 工具三件套）——★ 路线演进（§124x-z 后，ch02 原文写"唯一办法 = redroid dump"已废弃）**：

| 阶段 | 办法 | 现状 |
|---|---|---|
| **早期（工具三件套）** | redroid 容器跑原版 32 位 so → dlopen → dump 18 个 .text 常量池 + 2 .data（共 72B 运行时值）→ 注入 unidbg。工具：`frida-dump/redroid_jni_dump.py` + `redroid_dump_jni.sh` + `gen_runtime_table.py`（JSON→C） | **保留作备查**（能 dump 但 redroid 32 位 so 不被加载 + 无 GPU，见 AGENTS.md 终局反思） |
| **★ 当前（§124x-z 静态固化）** | **18 值已静态固化在 `rev64/src_jni_translated/jqt_mem.c` 的 `g_jq_got_tbl`（静态位移，如 0x00138918，非 redroid 绝对值，不同进程基址通用），无需运行时 dump** | **当前干活会话直接用 `jqt_mem.c` 固化值，别再找/生成 `jni_runtime_values.json`（已删）** |

**★ 为什么改静态位移**：翻译体把 `add rX,pc` 实现为 `rX=file_off+rX`（PC 相对位移），故注入值应为**静态位移**（jq_img.bin 原始值），非 redroid 运行时绝对地址（`0x96a779e4` 类——不同进程基址不同，注入即错）。旧表用 redroid 绝对值 → 落到 host 随机映射读垃圾 → 主线程 `_ZdlPv(垃圾)` SIGSEGV。

**注意 §124 纠正**：拿到这 18 个值只是"指令能跑"，"能玩"还卡在 §124 的 3 项阻塞
（① 上下文相关 BL 解码 bug ② C++ 对象图语义缺失 ③ 288MB PROT_NONE mmap）。
## 2.4 军棋实战验证（地址调用 + 18 常量 + §124 阻塞）


> **本节核心**：
> 1. 地址调用 + 18 常量 + §124 阻塞全链路：OnCreate 推进 0x43ed8 卡住
> 2. 前 3 层补环境全通（系统调用/Java/指令），第 4 层 C++ 对象图补不了
>
> **本节难点**：
> 1. 「能加载 ≠ 能调用」：12 init_array 全通但 OnCreate 卡 §124
> 2. §124 三项：① BL bug ② 对象图 ③ 288MB mmap，补环境解决不了

军棋 32 位 so 在 unidbg（unicorn2）下的地址调用实测（红线 §R6）：
| 环节 | 军棋实测 | 结果 |
|---|---|---|
| 加载 so | 12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT | ✅ 全通 |
| JNI_OnLoad | 注册 14 个 `On*` 方法 + FindClass(AdView) | ✅ 通（AdView 返 null 去广告） |
| 地址调 OnCreate | `0x233e1`（Thumb 奇地址，`&~1` 对齐） | ✅ 能调 |
| 18 个 `.text` 常量 | 需 redroid dump 72B 注入（**★ §124x-z 后 = 静态位移固化 `jqt_mem.c`，免 dump**） | ⚠️ 注入后指令能跑 |
| 突破 0x43ed8 | std::vector 对象图 | ❌ 卡 §124 三项阻塞 |
**结论**：军棋"地址调用 + 18 常量注入"能推进 OnCreate 到 0x43ed8，
但"能玩"卡的是 §124 的对象图语义（不是"调不通"，是"调通了但数据错位"）。
## 本章小结（一句话版）

> "加载 so"只是把程序搬进房间，"调函数"还要**找入口、传参数、补环境**三步。
> 军棋 strip 后只能走**地址调用**（14 个 `On*` 入口 + `&~1` 对齐 Thumb 奇地址），
> 调通后卡在 **18 个 `.text` 常量**（静态读不出；早期需 redroid dump 72B 注入，**★ §124x-z 后 = 静态位移固化 `jqt_mem.c` `g_jq_got_tbl` 免 dump**）
> 和 **§124 三项阻塞**（① BL 解码 bug ② C++ 对象图 ③ 288MB mmap）——
> 指令能跑 ≠ 能玩。

## 视频
