# 第26章 unidbg 补环境实战：总结

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的补环境总结

## 痛点：为什么补环境要有"规范"？

补环境是"打补丁"：你补了 100 个卡点，每个都"能跑"了，但某天发现"改 A 崩 B"
——补丁之间互相干扰（军棋实测：vecfix 改 r0/r1/r2 的时机不对，vector 函数
从"走空路径"变"写未映射地址"；JunqiJni handler 覆盖不全，JNI 调用漏一个
就 `UnsupportedOperationException`）。军棋 14 个 JNI 入口（readelf 实测：
OnCreate / OnDestroy / OnResult / OnResume / OnEvent / OnKey / OnUpdate /
OnGraphChange / OnBluetooth / OnPause / OnGraphCreate / OnTimer / OnInput /
OnTouch）共用同一套补环境框架。**没有统一规范，补环境就是无限打地鼠。**
军棋实测 3 个卡点。

### 痛点 1：补 100 个卡点，改 A 崩 B

- **大白话卡点**：补丁互相干扰，没有"补环境的施工顺序"。
- **为什么卡**：军棋补环境涉及 7 步框架 + code cave + vecfix + JunqiJni
  handler + 18 常量注入，每个补丁的作用域/时机不对就互相破坏。
- **不解决会怎样**：反复回归（补完再崩、改了再回退），没有可复用流程。

### 痛点 2：崩溃信号（SIGSEGV/SIGFPE/READ_PROT）分不清哪层

- **大白话卡点**：so 崩了只知道"崩"，不知道崩在"补环境第几层"。
- **为什么卡**：军棋 `.got` 零槽（`__cxa_atexit` 读零崩）、C++ 对象图
  （std::vector 垃圾 begin/end 写未映射）、§124g 全局未初始化（垃圾调用链
  READ_PROT）——三种崩看起来都是 SIGSEGV，不分类就没法定位。
- **不解决会怎样**：按错层补（拿"补 .got"的药治"对象图"的病），越补越乱。

### 痛点 3：补到顶发现"语义补不了"，不知道该停还是继续补

- **大白话卡点**：前三层（系统调用/Java/指令）补了全通，第四层"数据与指令对齐"
  补不了——什么时候该停？
- **为什么卡**：军棋 §124d-g 证明：no-init_array 路径推进到 90% 后，
  最后 10% 卡在"全局 C++ 对象未初始化"（std::string 函数指针表），
  这不是"再补个值"能解决的。
- **不解决会怎样**：无限补下去（vecfix 补 37 次还在补），浪费资源，
  且"能调"被误读成"能玩"。

## 为什么这样做：按"初始化 → 适用场景 → 规范"三层组织

补环境总结 = 把第 22-25 章的"散点经验"收成一套可复用的流程：
1. **初始化 7 步**（搭好环境底座，§124d-g 修订版）
2. **适用场景**（崩溃信号 → 该补哪层）
3. **规范**（补的顺序/优先级/红线/停止条件）

类比：补环境像"装修"，要先打地基（初始化）、再按房间选材料（适用场景）、
最后按施工规范（优先级/顺序）来，不然返工。

**预判追问："规范是不是'补到 OnCreate 返回 GLView 为止'？"** 不是——
军棋的"停止条件"是**诚实边界**，不是"补到能玩"：
1. 前三层（系统调用/Java/指令）补全 = 补环境完成。
2. 第四层（对象图）补到 vecfix/JunqiJni 能让 `OnCreate` 推进到 ~90%
   = 补环境加强的天花板（§124g：最后 10% 被全局未初始化挡住）。
3. **"能玩"不在补环境范围内**——最后 10%（bx bug / 全局未初始化）+
   288MB mmap 是 unidbg 路线的天花板，补环境到此为止，
   继续补只会"无限打地鼠"。规范里必须写明"补到哪算完"，否则没有终态。

**预判追问："那 unidbg 到底'怎么填值'？18 常量、.got、vector 的 begin
分别谁填？"** 三类值来源完全不同（军棋实测口径）：
- **18 个 `.text` 常量 + 2 个 `.data`（共 72B）**：真值来自
  **redroid 跑原版 so 后 dump**（dlopen 重定位才填的号），unidbg 算不出
  （静态 `.so` 里是零/占位）→ 注入 `jq_load_runtime_values`。
- **`.got` 槽（214 PLT + 6 GLOB_DAT）**：unidbg 加载时自动重定位
  （AndroidResolver(23) 的 5 个系统库 + 挂桩 GL），填的是 unidbg 侧
  的模拟地址——真机填真 libc 地址，两者"值不同但语义等价"。
- **vector 的 begin/end**：**不填真值**（真值来自真实堆布局，拿不到），
  vecfix 填 scratch 空区 = "走空路径"安全阀，不是"补运行时内存布局"。
"填值"三个字，军棋三条路三种语义——规范里必须写清"哪个值谁填"。

## 26.1 补环境初始化 7 步（军棋 §124d-g 修订版）


> **本节核心**：
> 1. 补环境 7 步（军棋 §124d-g 修订版）
>
> **本节难点**：
> 1. 选后端→配库→挂桩→建 VM→调入口→填常量→破 §124；前 6 步全通第 7 步卡
> 2. 「7 步」是补环境全链路
> 3. 第 7 步破 §124 卡住（unidbg 解不了）

![军棋补环境总结](assets/ch26_summary.png)



| 步 | 大白话 | 军棋代码 |
|---|---|---|
| 1 架构选择 | 32 位 ARM → 32 位模拟器 | `for32Bit()` |
| 2 后端选择 | 选 unicorn2（修 PC off-by-2，DiagTracePC 0xa37fc 存 0x159880 实证；bx bug 已知代价） | `.backend(UNICORN2)` |
| 3 解析器 | AndroidResolver(23) 提供 5 系统库（libc/libstdc++/libm/liblog/libdl，军棋 7 个 DT_NEEDED 里的前 5 个） | `setLibraryResolver` |
| 4 GL 挂桩 | 7 依赖里 2 个 GL 库挂桩（无真 GPU） | `symbolResolver.resolve("glGetError").when(0L)` |
| 5 DalvikVM | 模拟 Java 侧（军棋 7 个类） | `createDalvikVM(junqi.apk)` |
| 6 加载 so | 跑 12 个 `.init_array` + 重定位 5312 RELATIVE + 214 PLT + 6 GLOB_DAT；**init_array 可选**（no-init_array 路径 §124d 突破） | `loadLibrary(so, init)` |
| 7 加强项 | code cave（0xc3a68 trampoline）+ vecfix（0x43ed8）+ JunqiJni handler + 18 常量注入 | 见 25.2 |

## 26.2 补环境适用场景（崩溃信号 → 该补哪层）


> **本节核心**：
> 1. 崩溃信号 → 该补哪层
>
> **本节难点**：
> 1. SIGSEGV（读零 .got/段摆错）补 1 层；SIGFPE（除 0/移位）补指令；READ_PROT（保护段）补 §124 ③
> 2. 「崩溃信号分类」是补环境定位
> 3. SIGSEGV 同信号可能不同根因（.got 零值/段权限/§124 ③）

| 信号 | 大白话 | 该补哪层 | 军棋触发点 |
|---|---|---|---|
| `SIGSEGV`（读零） | 读空指针崩 | 第 1 层 `.got` 填真值 | `__cxa_atexit` 读零 `.got` |
| `WRITE_UNMAPPED` | 写未映射地址 | 第 4 层 vecfix | std::vector 垃圾 begin/end（0x43ed8） |
| `READ_PROT` | 读保护内存 | 第 4 层全局对象图（补不了） | §124g：0x23b7a `ldrb r2,[r4,r3]` std::string 拷贝循环读垃圾源 |
| `SIGFPE` | 除零崩 | 指令语义（`__aeabi_uidivmod` 除以余数而非除数） | 军棋影子运行时局部变量存商修 |
| `UnsupportedOperationException` | JNI handler 覆盖不全 | 第 2 层 JunqiJni handler | §124e：`KeyEvent->ACTION_DOWN:I` getStaticIntField |
| `mmap2` 失败 | 大内存映射 | 第 1 层 syscall hook（288MB PROT_NONE） | DiagJni5：`mem_map 0x3743f000, 302243840, perms=0` |
**何时补（顺序）**：
1. 系统调用（`open`/`read`/`close`）→ 补 mock 文件（I/O 重定向）
2. Java 方法（`FindClass`/`GetMethodID`/静态字段）→ 补 mock 类（JunqiJni 全面 handler）
3. 指令（`__cxa_atexit`）→ Hook 返回 0 / 填 `.got` 真值
4. C++ 对象图（std::vector）→ vecfix 补 scratch（安全阀，非语义补全）
5. 全局对象图（std::string 函数指针表）→ **补不了**（§124g，停止条件）
## 26.3 补环境规范（军棋 4 条红线 + 1 条停止条件）


> **本节核心**：
> 1. 军棋 4 条红线 + 1 条停止条件
>
> **本节难点**：
> 1. ① 不包简化版当完整版 ② 不 dlopen 32 位库 ③ 删 baidu smali 前处理 AdView ④ 不改 native 签名 ⑤「能跑≠能玩」停止
> 2. 「规范」是补环境纪律
> 3. 停止条件：补到顶发现语义补不了（std::vector 元素数）

1. **优先级**：系统调用 > Java 方法 > 指令 patch（军棋 `.text` 100% 对齐，
   基本不 patch）
2. **`.got` 未重定位** → 读零指针崩（`__cxa_atexit`），用 docker 真 ARM libc
   解析 6 GLOB_DAT 填
3. **`libGLESv1_CM` 无真 GPU** → 挂桩（`glGetError` 等返回 0）
4. **18 个 `.text` 运行时常量** → 静态读不出；早期需 redroid dump 72B 注入，**★ §124x-z 后 = 静态位移固化 `jqt_mem.c` `g_jq_got_tbl` 免 dump**
   （值来自真实 32 位库，不是 unidbg 算的）
5. **停止条件（★新增，§124g）**：前三层补全 + vecfix/JunqiJni 能让
   `OnCreate` 推进到 ~90% = 补环境完成；最后 10%（全局未初始化）+
   288MB mmap 是天花板，**不继续补**（无限打地鼠无终态）

**6. 补环境代码写法规范（社区实战沉淀，吸收自 geekbyte/绿洲案例）**：

| 写法 | 适用 | 军棋选择 |
|---|---|---|
| ① 全在用户类补（`extends AbstractJni`，重写缺失 case） | 可移植性最好（防 unidbg 更新破坏） | ✅ **军棋用这条**：军棋去广告要删 baidu smali，补环境代码放用户类，不动 `AbstractJni` 源码 |
| ② 通用方法补 `AbstractJni`、自定义补用户类 | 通用方法一次补齐，多项目复用 | 通用 `java/lang/*`（String/Integer/ByteArray）`AbstractJni` 已补，军棋只需补 so 特有的（广告层 + GLView 回调） |

**要点**：补环境代码写**自己的类**（`JunqiJni extends AbstractJni`），不修改 unidbg 源码（`AbstractJni`）——
这样 unidbg 升级/项目迁移时补环境代码不丢。通用 case 交给 `AbstractJni`，只重写 so 特有的缺失方法。
## 26.3.1 逐环节"我们能干什么 / 不能干什么"（补环境全链路总表）

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **初始化 7 步** | 架构/后端/解析器/GL 挂桩/DalvikVM/加载/加强项全通（unicorn2 + AndroidResolver(23) + 2 GL 挂桩） | 靠初始化"修"§124c bx bug（init_array 后全局分支损坏是模拟器 bug，初始化只能"选择跳不跳 init_array"） |
| **崩溃信号分类（26.2）** | 按 SIGSEGV/WRITE_UNMAPPED/READ_PROT/SIGFPE/UnsupportedOp/mmap2 失败定位到具体层 | 把"信号分类"当"根因修复"（§124g 的 READ_PROT 根因是全局未初始化，分类只能指到"第 4 层"，补不了） |
| **规范 4 红线 + 停止条件（26.3）** | 优先级/填值口径/GL 挂桩/18 常量注入/补到 ~90% 即停，有可复用流程 | 把"停止条件"当"业务完成"（停止 = unidbg 路线天花板，不是"能玩"达成） |
| **值来源三类** | `.got` 重定位填（模拟桩地址）+ 18 常量 redroid dump 注入 + vector vecfix scratch | 让 unidbg"算出"18 常量真值（值来自真实 32 位 libc 重定位结果，不是模拟值） |

**军棋全链路实测对照**（红线 §R6 + §124d-g）：

| 环节 | 军棋实测 | 结果 |
|---|---|---|
| 初始化 7 步 | 12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT + JNIEnv（DiagJni 实证） | ✅ 全通 |
| 加强层（25 章） | code cave + vecfix 37 次 + JunqiJni 37+ JNI 调用，`OnCreate` 推进到 ~90% | ✅ 到 90% |
| 最后 10% | §124g：0x23b7a READ_PROT（std::string 拷贝读垃圾源，LR=垃圾 0x39a63305） | ❌ 卡全局未初始化 |
| 停止条件 | 补到 90% 即停（bx bug / 全局未初始化 / 288MB mmap 三阻塞不继续补） | 诚实边界 |

## 26.4 ★ 下一轮 unidbg 主攻方向：死锁按序处理（§124d-g 修订）

> 对齐 AGENTS.md §R6 第 5 条 + §124d-g（2026-11-01）

**军棋 `OnCreate` 的死锁结构（§124g 确认）**：

```
路径 A: 跑 init_array(真机正常路径)
  → 12 个 init_array 全通, 但 unicorn2 出现全局 bx bug(§124c):
    跑完 init_array 后任意分支指令(bx r3/b.w/ldr pc)跳错目标(0xfff1a33c)
  → 模拟器深层 bug, 不可修复(0.9.8/0.9.9 都复现)

路径 B: 跳过 init_array(§124d 突破)
  → code cave(0xc3a68 trampoline) + vecfix 37 次 + JunqiJni handler
  → OnCreate 推进到 ~90%(GLView/EGL/setContentView/registerReceiver×3)
  → 最后 10% 卡: 全局 C++ 对象(std::string 函数指针表)未初始化
    (init_array 就是干这个的, 跳过它 = 全局是垃圾)
  → §124g 实证: 0x23b7a READ_PROT, LR=垃圾 0x39a63305, 垃圾调用链,
    任何单点 workaround 都无法推进(patch r6=0 变无限循环/重定向到 scratch
    仍反复垃圾调用)
```

**两个出路（按工作量）**：
1. **修 bx bug（路径 A 复活）**：等 unicorn2 上游修"init_array 后全局分支
   损坏"，或 code hook 逐 BL patch（§124b 的 cave 方法在完整环境下
   因 .text 只读写入静默失败，需先解决可写性）。
2. **逆向 std::string 全局地址表手动初始化（路径 B 收尾）**：
   找到 no-init_array 路径下"未初始化的全局函数指针表"在 `.data`/`.bss`
   的偏移，按真机 init_array 的初始化结果逐槽填入（需红roid dump
   init_array 后的全局值）。工作量大，是"补环境加强的最后一块拼图"。

**诚实边界**：3 项阻塞（bx bug / 全局未初始化 / 288MB mmap）都不影响
"指令语义 100% 对齐"（红线 §122：可跑函数 1174/1174 PASS + 副作用
crc32 比较）的结论，但**"能调" ≠ "能玩"**。完整"能玩"仍需真 arm64 GPU
设备验收（redroid 无 GPU，MuMu 影子库稳定运行但游戏板黑屏——数据与指令对齐分歧）。

## 26.5 本章小结（一句话版）

> 补环境总结 = 初始化 7 步 + 适用场景 6 信号 + 规范 4 红线 + 1 停止条件。
> 军棋已通 1-3 层（系统调用/Java/指令）+ 加强层（code cave/vecfix/JunqiJni），
> 加载全通（12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT + 7 依赖），
> `OnCreate` 推进到 ~90%（§124d-g）；卡在最后 10%：init_array 路径 bx bug
> （§124c 模拟器 bug）vs no-init_array 路径全局未初始化（§124g 垃圾调用链
> 0x23b7a READ_PROT / LR=垃圾 0x39a63305），两者互为死锁。
> 值来源三类必须分清：`.got` 重定位填（模拟桩地址）+ 18 常量 redroid dump
> 72B 注入（真值来自真实 32 位库，非 unidbg 算）+ vector vecfix scratch（安全阀，
> 非数据与指令对齐补全）。停止条件：补到 90% 即停，"能玩"需真机 GPU。

## 视频
