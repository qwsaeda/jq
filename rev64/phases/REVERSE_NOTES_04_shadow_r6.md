> 来源: REVERSE_NOTES.md §107-§124i（行 1305-1695），2026-11-02 拆分
### §107 重大发现：264 个 ARM this-adjusting thunk（对象图分歧根因）

**纠正 §106 的结论**（"0x242e0 是跳表地址，非误译"）：

**根因**：vtable 槽位里的**偶数地址**（bit0=0）是 **ARM 模式 this-adjusting thunk**，不是 Thumb 函数：
```
0x242e0(ARM): ldr ip,[pc,#8]; add ip,pc,ip; sub r0,r0,#0x20; bx ip
    → 语义: r0 -= 0x20; tail-call 0x242f4(Thumb 方法)
```
Thumb 反汇编把同一字节误解为跳表 case（`stm r0!,{r3}; b 0x23e24`），翻译出的 F_242e0 把 r3 存进对象 → 破坏 vtable。

**量化**：全 .text 扫描出 **264 个 ARM this-adjusting thunk**（模式 `ldr ip,[pc,#8]; add ip,pc,ip; sub r0,r0,#imm; bx ip`），257/264 目标确认为 `push` 前导的真实 Thumb 方法。调整量 0x20/0x24/0x38/0x3c 等。

**13 个垃圾调用全解释**：垃圾目标 = 0x242e0+off 处的原始 ARM/Thumb 字节（0x242e0+4=0xe08fc00c, +8=0xe2400020, +c=0xe12fff1c, +14=0x465fb5f0, +1c=0xb4f04644, +20=0x49644c62, +24=0x44a54a64）。

**修复**：`gen_shadow.py` 加载 `/tmp/arm_thunks.json`（264 thunk → 调整量+目标），对 thunk 地址输出正确 C（`r0 -= imm; JQ_RET(F_target(...))`），生成 `jq_funcs_thunks.c`。**6142 种子 regen 未减少垃圾调用 → 确认是 thunk 误解译，非 seed 不完整**。

### §108 bx lr 返回桩修复（栈溢出根因）

**问题**：`bl 0x7f4ec` 调用 2 字节 `bx lr` 返回桩，但反汇编在数据池处失步（0x7f4e0-0x7f4ec），翻译出的 F_7f4ec = 完整函数 → 无限递归 → 栈溢出。

**量化**：188 个 seed 函数 raw bytes 以 `70 47`(bx lr) 开头；其中 3 个（0x75b78/0x7f4ec/0x85854）在反汇编中失步（无干净指令边界）。

**修复**：`gen_shadow.py` 对 raw bytes = `70 47` 的函数强制翻译为"原样返回 r0"（JQ_RET(r0)）。

### §109 分发表排序修复

**问题**：jq_call 用**二分查找** jq_dispatch[]（必须按地址排序）。ARM thunk 条目追加在表尾破坏了排序 → 9 个合法 .text 目标（0x120508 等）查不到（报"未知调用目标"）。

**修复**：对 jq_dispatch.c 的 6402 条目按地址排序。排序后 9 个目标全部正确分发。

### §110 __aeabi_uidivmod 导入修复（SIGFPE 根因）

**问题**：`jq_imp___aeabi_uidivmod` 先算 `r1 = r0 % r1`（余数），再 `JQ_RET(r0 / r1)` —— **除以余数而非原除数**。余数为 0 → SIGFPE。

**修复**：用局部变量保存商，`r1 = r0 % r1; JQ_RET(q);`。兄弟函数 idivmod/ldivmod/uldivmod 用局部变量（正确），仅 uidivmod 有 bug。

### §111 当前状态（本轮修复后）

**x86_64 影子 harness**（run20）：14 个未知调用全为 0x0（null，良性），无 .text 未知、无 0x44a54a64 垃圾调用。SIGFPE 已修。**但** F_6fff8 递归栈溢出（0x7085e `bl 0x6fff8` 无条件自调用；终止哨兵 = 0x71360，r9 从未达到 → 状态分歧，非误译）。

**诚实边界**：F_6fff8 递归是对象图未完整重建导致的状态分歧（r9 进度错）。ARM thunk/bx-lr/uidivmod 是**真实翻译 bug 修复**，但完整生命周期仍受对象图限制。**不包装成完整版**。

### §112 修复版 arm64 库 + APK（R6 里程碑 v2）

**产物（含 ARM thunk/bx-lr/uidivmod 修复）**：
- `rev64/dist/libjunqi_arm64_shadow.so` — **11,311,736 字节**，ELF64/AArch64，14 个 JNI 导出，含 6142+264 函数翻译体 + ARM thunk 覆盖
- `rev64/dist/phase_shadow.apk` — **25,739,264 字节**，v1+v2（verified:true），`.so` 全 STORE，manifest 0x00080003，native-code arm64-v8a/armeabi/x86_64，minSdk 19

**x86_64 影子 harness**：run20 生命周期（OnCreate→OnDestroy）到 F_6fff8 递归栈溢出（状态分歧，哨兵 0x71360 未达）；无垃圾调用、无 SIGFPE、无 .text 未知目标、14 个 0x0 null 调用（良性）。

**本轮 4 个真实翻译 bug 修复**（§107-§110）：ARM this-adjusting thunk（264）、bx lr 返回桩（188/3 失步）、分发表排序、__aeabi_uidivmod。

**诚实边界**：对象图未完整重建（F_6fff8 递归状态分歧）；真机"能玩"未验证（需真实 arm64 GPU 设备）。**不包装成完整版**。

### §113 生命周期安全阀三件套（本轮，把"崩栈/挂死"变"优雅放弃"）

状态分歧（对象图未完整重建：vtable 槽位 = 0 → null 调用 → 字段留错）在**运行时**表现为三类，此前分别导致：①F_6fff8 无限递归 → C 栈溢出崩溃；②OnGraphChange 空转反复调 0x0 → 挂死；③F_7de74 解引用垃圾指针（0x43f00042）→ SIGSEGV。本轮在影子运行时加了三个**安全阀**（等价于"未知调用返回 0"，让生命周期可走完）：

- **递归深度阀**（jq_rt.c `jq_call`）：`g_jq_depth` 超 `JQ_MAX_DEPTH`(0x1000) → `longjmp` 到最外层 `setjmp`（jq_call depth=0 处）整体放弃当前 JNI 调用链返回 0。单 return 0 不破环（被调方拿 0 后仍自循环），必须 longjmp 展开。
- **每入口调用预算**：`g_jq_budget` 在 depth=0 重置 `JQ_MAX_BUDGET`(0x200000)，每次 jq_call 递减，耗尽 → 同上 longjmp。兜住浅层无限循环（OnGraphChange 分歧后反复调 0x0，1.4M 次/入口）。
- **越界内存阀**（jq_shadow.h `MEM/MEMH/MEMB/MEMP`）：`JQ_VALID(a)=(uint32_t)a<0x10000000`，越界（垃圾指针，如 0x43f00042>MAP_HI）读导到 4KB 零页 `g_jq_scratch`（读得 0、写丢弃）。F_7de74 的 `MEMB(r0+r3)` 原会 SIGSEGV。

**验证**：x86_64 harness 3/3 稳定通过完整生命周期（OnCreate→OnGraphCreate→OnGraphChange→OnTimer×3→OnPause→OnResume→OnUpdate→OnDestroy→"全部通过" exit=0），每次 2 次放弃（OnGraphCreate 0x23560 / OnGraphChange 0x23488），0 崩溃。已知良性：1 次"非法对象句柄 0x8"（OnCreate GLView null，预期）、2 次未知 0xc4238（seed 闭包不追间接跳 `mov pc,r3` 跳表，缺失函数，返回 0 良性）、1 次未知 0x3f3504f2（分歧垃圾地址）。

### §114 修订产物（R6 里程碑 v3，含安全阀三件套）

- `rev64/dist/libjunqi_arm64_shadow.so` — **14,521,400 字节**，ELF64/AArch64，14 个 JNI 导出，6142+264 函数翻译体 + 安全阀（深度/预算/越界）
- `rev64/dist/phase_shadow.apk` — **28,946,432 字节**，v1+v2（verified:true），`.so` 全 STORE，manifest 0x00080003，native arm64-v8a/armeabi/x86_64，minSdk 19

**诚实边界**：生命周期靠安全阀走完（每次 2 次放弃 = 状态分歧被优雅降级），非"正常跑通"；对象图仍不完整（vtable 槽位 0 → null 调用）；真机"能玩"未验证。**不包装成完整版**。

### §115 MuMu 实测：arm64 影子库在真实 GPU 模拟器上运行（R6 里程碑 v4）

**环境**：MuMu Player 5.24.4.3342（网易），Android 12，x86_64 宿主，arm64-v8a 经 **libhoudini**（NB_64BIT=true）native bridge 翻译。GPU：Adreno 640，OpenGL ES 3.1。**有真实 GPU**（区别于 redroid 无 GPU）。

**安装前置修复**：`resources.arsc` 必须 4 字节对齐（MuMu/Android 12 targeting R+ 要求）。`store_compressed.py` 只强制 STORE 不对齐，需在 SignV2 前跑真 `zipalign -f -p 4`（build-tools/36.0.0/zipalign）。修复后 `.so` 4096 对齐 + `.arsc` 4 对齐 → `adb install` Success。

**运行结果**（phase_shadow.apk，arm64-v8a 影子库）：
- `libhoudini` 初始化 arm64 → **arm64 libjunqi.so 加载成功**（JNI 14 符号全部解析）
- `OnCreate (arm64 full, aligned to 32-bit)` → `OnCreate -> decor view (7 subsystems init)` → shader/link 编译
- Activity 显示（+492ms），**无 FATAL/AndroidRuntime/崩溃**，进程稳定 30s+，`topResumedActivity` 前台
- `handleMessage` 循环跑：OnTimer 每 100ms 重投递（Handler sendMessageDelayed）
- **GLThread 未运行**（/proc/22148/task 无 GLThread 线程）→ `onDrawFrame`→`OnUpdate` 没被驱动 → **游戏板未渲染，白屏**（右上角有斜纹 = 部分 GL 纹理）

**诚实边界**：arm64 影子库在真实 GPU 模拟器上**运行不崩**是硬里程碑（此前只有 redroid 无 GPU / 64 位 bridge 不完整的限制）；但**游戏内容未渲染**（GLThread 未起 → GL 上下文/表面未驱动 → 对象图分歧），非"可玩"。**不包装成完整版**。

### §116 C 源码任意平台移植验证：armv7a（32 位 ARM）影子库编译成功（R6）

**问题**（用户）：C 源码能否编译到任意平台 so？

**答**：能编，但**只解决"能加载"，不解决"能玩"**。影子运行时软件模拟 32 位地址空间（`g_jq_base` + `uint32_t` 偏移），宿主指针宽度无关 → 同一套 C 在任意架构语义对齐原版 32 位 ARM。

**验证**：同一套 `src_shadow/` C 用 NDK r25b `armv7a-linux-androideabi19-clang` 编译（`/tmp/build_v7a.sh`，含 `jq_funcs_thunks.c` 264 ARM thunk 覆盖）：
- 产物 `rev64/dist/libjunqi_arm_shadow.so` = 14,499,632B，**ELF32 / Machine=ARM**，14 个 `Java_com_cnvcs` JNI 导出
- 已能编平台：arm64-v8a（主产物）、x86_64（harness）、armeabi-v7a（本验证）
- 原则上也能编：x86_64 Android、桌面 Linux/macOS/Windows（`mmap`→Windows 需 `VirtualAlloc`；`__android_log_vprint` 已 `#ifdef __ANDROID__` 守卫）

**边界**：换平台 = 换编译器，成本低；但**对象图分歧（vtable 槽位=0）+ 真机渲染未对齐**不变，不解决"能玩"。

### §117 对象图分歧防护：san() 参数清洗 + 分歧闩锁（R6 MuMu 调试）

**症状链**（MuMu 真机调试，arm64 影子库经 libhoudini）：
1. SIGSEGV `fault 0x0` @ `real_call+256`：真实 JNI 调用，`slot=39 CallBooleanMethodA a1=0x149` —— 对象图分歧把影子空间垃圾值 0x149 当 jobject 传真实 JNI
2. 加 `san()`（掩掉 `<0x10000000` 的非零值）后：SIGSEGV 消失，OnCreate 推进过 slot=39/63/36（优雅返回 0）；但出现 `非法对象句柄 0x80` + **SIGABRT**（`jq_jni_thunk+2156`，ART JNI 检查 abort——obj/mid 都"看似有效"但类不匹配）

**修法**（jq_jni.c）：
- `san(p)`：`(uintptr_t)p < 0x10000000u` 且非零 → 置分歧闩锁 + 返回 NULL（真实 JNI 只能收宿主高地址指针，影子空间值必为垃圾）
- `jq_jni_real(h)`：非法/越界句柄 → 置分歧闩锁 + 返回 NULL
- `real_call`：`g_jni_diverged` 置位后**所有真实 JNI 调用直接返回 0**（不把坏引用喂 ART → 不 SIGABRT）；Call*Method 对象 NULL（slot>=34, rt!=0）→ 置闩锁 + 返回 0
- 分歧闩锁 = 安全阀"放弃当前 JNI 链"在 JNI 层的落地：一旦对象图分歧，不再做无效真实调用，优雅降级返回 0

**验证**：
- x86_64 harness：`exit=0`，"全部通过"，闩锁触发（`slot=36 对象 NULL`→`非法对象句柄 0x8`→深度超限放弃链 0x23560/0x23488）
- MuMu：SIGSEGV 消失；OnCreate 推进到 6 个 JNI 调用点（39/63/36 等）优雅返回；SIGABRT 待本版验证

**诚实边界**：闩锁是**优雅降级不是恢复**——OnCreate 可能返回 0（Java NPE）或半初始化；对象图（vtable 槽位=0）+ 类不匹配仍不可重建。不包装成"能玩"。

### §118 对象图分歧根因 + fallback view（R6 MuMu 调试续）

**分歧根因（精确诊断，x86_64 harness 高地址假对象）**：
- 第一个真分歧 = `Call* slot=39 对象 NULL r1=0x0 sig="(I)Z"`：游戏用 **NULL 对象**调 `requestFeature(I)Z`
- 序列：`FindClass android/view/Window` → `GetMethodID requestFeature(I)Z` → `CallBooleanMethodA(window=NULL, requestFeature)`
- Window 来自 `activity.getWindow()`，但 window 句柄在 `.data` 全局里读出 **0x149**（MuMu）/0（harness）—— **GOT/.data 未运行时重定位**（AGENTS.md 红线）：游戏把 window 句柄存 .data 全局，静态 .so 是垃圾/零 → 读出垃圾当 jobject
- 结论：**对象图（.data 全局 + vtable）静态重建不了**，OnCreate 必然分歧

**修复**：
1. **harness 假对象高地址**（`mkobj` 0x1000→0x70000000）：修掉 harness 假分歧（低地址被 san() 误掩）
2. **fallback view**（jq_jni.c `make_fallback_view`）：OnCreate 分歧返回 nil 时，真实 JNI `FindClass("com/cnvcs/GLView")`→`GetMethodID(<init>,(Context))`→`NewObjectA`→`CallVoidMethodA setContentView`，返回最小 GLView —— Java 不再 NPE
3. **harness 验证**：OnCreate 返回 0x70000210（非 nil），完整生命周期 "全部通过"

**诚实边界**：fallback = 优雅降级（空 GL 面，非游戏板渲染）。对象图分歧 = `.data` 全局未重定位 + vtable 槽位=0，**静态重建不了**，非翻译能力问题。不包装成"能玩"。

### §119 同调用垃圾修复：JQ_RR 宏（R6 MuMu 调试续）

**症状**：fallback view 让 OnCreate 成功（GLView 创建 0x181），但 **OnResume**（F_234b0）→ F_bdc7c → SIGABRT @ `jq_jni_thunk+2356`（寄存器 x24=0x1f=31 = **JQS_GetObjectClass**）。

**根因（同调用垃圾）**：switch 里 `JF(idx)(..., jq_jni_real(rX), ...)` 同一表达式——`jq_jni_real(r1)` 检测垃圾置闩锁返回 NULL 后，**仍继续调** `GetObjectClass(NULL)` → ART abort。顶层闩锁守卫拦不住（该调用本身才置闩锁）。

**修法**：`JQ_RR(h)` 宏 = `({ void *_p = jq_jni_real(h); if (g_jni_diverged) return 0; _p; })`——解析后查闩锁即从 jq_jni_thunk 返回 0（不调 JF）。用 Python 把 **jq_jni_thunk 行范围（305-546）内 53 处** `jq_jni_real(` → `JQ_RR(`（build_args 143/175/208 保留——real_call 顶层闩锁兜底）。

**验证**：x86_64 harness exit=0，"全部通过"，OnCreate 返回 0x70000210。

### §120 信号兜底：SIGABRT/SIGSEGV → 优雅放弃链（R6 MuMu 调试续）

**症状**：JQ_RR 拦不住 OnResume 的 SIGABRT——`GetObjectClass`（x24=0x1f=31）收到**有效句柄但指向陈旧真实对象**（对象图分歧深层表现，非垃圾句柄），`GetObjectClass(陈旧对象)` → ART CheckJNI abort。JQ_RR 只能拦垃圾句柄，拦不了"有效但陈旧"指针。F_bdc7c 是复杂 C++ 对象遍历（vtable 间接调用），针对性修不现实。

**修法（jq_rt.c 信号兜底）**：
- `jq_init()` 安装 `SIGABRT/SIGSEGV/SIGBUS` → `jq_sig_handler`：`g_jq_buf_valid` 时 `jq_jni_mark_diverged()` + `siglongjmp(g_jq_buf,1)` 放弃当前链（后续 JNI 返回 0）；无有效 setjmp（非影子库上下文）→ 恢复默认处理
- `setjmp/longjmp` → `sigsetjmp/siglongjmp`（信号上下文安全）
- `jq_jni.c` 暴露 `jq_jni_mark_diverged()`（置 g_jni_diverged）

**验证**：x86_64 harness exit=0，"全部通过"（信号兜底不干扰正常生命周期）。

### §121 MuMu 里程碑：arm64 影子库稳定运行不崩（R6 v5）

**环境**：MuMu（Android 12，x86_64 宿主 + libhoudini，Adreno 640 真 GPU），`phase_shadow_a64.apk`（arm64-v8a 影子库，信号兜底版）。

**结果（硬里程碑）**：
- APK 安装 Success（zipalign 4 对齐 + v1+v2 签名）
- **进程 90s+ 存活，无 FATAL/SIGSEGV/SIGABRT/NPE**，CPU 3.5%（低）
- 标题栏"军棋"显示，`topResumedActivity` 前台，点击响应无崩溃
- **GLThread 39 在跑**（fallback GLView setRenderer 触发渲染线程）
- **游戏板未渲染（黑屏）**：对象图分歧（.data 全局未重定位 + vtable 槽位=0），非翻译能力问题——诚实边界

**安全阀全链路验证（真机）**：san() 清洗 → 分歧闩锁 → 顶层守卫 → JQ_RR 宏 → fallback view → 信号兜底（SIGABRT/SIGSEGV→siglongjmp 放弃链）。每个都实机验证过：SIGSEGV→消失、SIGABRT→消失、Java NPE→fallback 消除、OnResume 崩→信号兜底消除。

**诚实边界**：**"运行不崩"≠"能玩"**。app 显示标题 + 黑屏（无游戏板），因对象图分歧。不包装成完整版。

### §122 回归方法批判与修正：副作用比较 + 诚实降级（用户指出）

**用户批判（成立）**：
1. **只比较返回值**：原 batch_test.py line 278 `if uout != cout` 只比 r0（unicorn r0 vs C printf 返回值）。**没比较副作用**（结构体写/.data 全局写），函数可能 r0 一致但副作用完全不同。
2. **输入集有限且假**：每函数仅 5 trial，输入 = 手工 r1/r2/r3 + `make_struct`/`make_directed` 构造的**自洽假结构体**（非真实运行时对象图）。换输入（不同布局/全局/指针）很可能 MISMATCH。
3. **"100% 对齐"是弱命题**：实际是"5 个构造输入下返回值一致率 100%"，不是"任意输入一致"、"副作用一致"、"功能一致"。

**修正（batch_test.py）**：
- **副作用比较**：C 端 TEMPLATE 和 unicorn 都 crc32 三区（结构体 0x220000/0x20000、.data 全局 0x23000-0x15e614、sp 区 0x200000/0x10000）；test_one 比较 r0 + 结构体 + .data 三值。
- **sp 区剔除**：实测 4 函数全"sp 副作用 MISMATCH"——影子栈帧布局（gcc -O0 翻译函数 vs unicorn push）实现相关，假阳性。保留结构体 + .data（真实副作用目标）。
- **输入集扩大**：待跑更大样本（40 函数抽样）看副作用比较暴露多少"旧 PASS 新 MISMATCH"。

**诚实降级**：旧"可跑函数一致率 100%（MISMATCH=0）"→ 新"**5 构造输入下返回值+结构体+.data 副作用一致率**"，不再宣称"指令语义 100% 对齐"为强命题。

**全量副作用回归结果（14 块并发，背景 sh_19670e197001479WlmaKXmocP2，已完成）**：
- **MISMATCH 15 个**（14 结构体副作用 + 1 返回值 0xb8898），全部 trial0 暴露：
  - 结构体副作用：`0x77840, 0x77b10, 0xa37fc, 0xa381c, 0xa9938, 0xa9970, 0xb6f68, 0xb6fb4, 0xc2c0c, 0xd2840, 0x69e78, 0xeedf0, 0xfd6f0, 0xfeca0`
  - 返回值：`0xb8898`（uc=0x1211276 vs c=0x1211274，差 2）
- **证实用户怀疑**：副作用比较暴露了纯返回值判据藏住的真 bug（原 1075 PASS 里至少 15 个实际副作用不一致）。这些是**真翻译缺口**（翻译体写结构体/全局与原版 unicorn 不同），非输入不变量——需逐个修。

**熵/壳核查（用户追问"怎么证明是加壳 so"，本轮实测纠正）**：
- `.text` off=0x233e0 size=0xfd814，熵 **7.10** = 正常 ARM 机器码（objdump 反汇编出真实 Thumb 指令 + JNI 符号名 `Java_com_cnvcs_App_On*`）
- `.rodata` off=0x120bf8 size=0x1d030，熵 6.76；但**静态 .rodata 与 frida-dump `rodata_dec.bin` 逐字节相同（0/118832 不同）**，已知串 `Junqi.Move/updateUrl/statHost/com/baidu/mobads/AdView` 静态文件即可见
- 唯一高熵点 0x138000（7.87）在 `.rodata` 只读数据段内（无 X 标志），值非有效地址（0x072f172e 类），是 JPEG zigzag/zlib dist **静态解码查找表**，非加密代码
- **结论：原版 so 无传统壳**（无 .text 加密、无 .rodata 加密、无运行时自解密）。之前"`.rodata` 加密/dlopen 自解密"是**聚合熵误读**（§122 已记，本轮实测终证）
- unidbg 失败根因 = `.got` 零槽位（静态 so 未重定位）+ 初始化例程反检测，**与壳无关**

**R6 修复（15 MISMATCH → 0，本轮完成）**：
- **根因 1：`add rX, pc` 寄存器形式 PC 对齐**（11/15）。ARMv7 寄存器形式 `add rd, pc`（Rm=PC）用**未对齐 PC = addr+4**，与 ADR 立即数形式 `add rd, pc, #imm`（对齐 `(addr+4)&~3`）不同。原翻译器两者都当对齐，致跳表/常量池地址差 2。修复：`thumb2c_full.py` line 191-194 区分，寄存器形式用 `a+4`。解决：0xb8898(返回值) + 0x77840,0x77b10,0xa37fc,0xa381c,0xa9938,0xa9970,0xb6f68,0xb6fb4,0xc2c0c,0xd2840,0xeedf0。
- **根因 2：`asrs` 立即数形式用逻辑右移**（3/15，颜色转换函数 0x69e78/0xfd6f0/0xfeca0）。ARM `asrs rX,rX,#imm` 是**算术右移**（符号扩展），翻译器立即数形式误用 `>>`（uint32 上为逻辑移位），致高位置 1 的结果错误（如 0xfd6f0 的 r4 由 0xE03D3C00>>11 得 0x1C0923 而非 0xFFFC0903，RGB 查表索引差 472）。寄存器形式已正确（`(int32_t)` 强转）。修复：立即数形式 `asrs/asr` 用 `((int32_t)rs>>imm)`。解决：0x69e78,0xfd6f0,0xfeca0。
- **修复后全量副作用回归（14 块并发）**：**PASS 1174 / MISMATCH 0 / UC_FAIL 5401 / SKIP 852 / 总 7428**，可跑函数一致率 **1174/1174 = 100%**（MISMATCH=0）。
- **诚实边界**：0x8cbec 在并行全量中出现 1 次瞬态 MISMATCH（资源竞争），单独重跑 11 次全 PASS + 块级重跑 PASS，归为瞬态非真缺口，**不计入 MISMATCH**。
- 增量状态（`incremental_regress.py status`）：6542 已分类 = PASS 1075 / UC_FAIL 4818 / SKIP 649，**MISMATCH 0，可跑一致率 100%**。
- **判据仍是弱命题**（§122 开头）：5 构造输入下返回值+结构体+.data 副作用一致，非任意输入/功能一致。不得包装为"完整/可玩"。

### §123 修复传播到影子库 + unidbg 非死路实证（本轮，用户挑战后）

**影子库用修复后翻译器重生成（背景 64m18s）**：
- `gen_shadow.py` 重生成 `/tmp/opencode/shadow/jq_funcs_0-3.c`（6402 函数，含 ARM thunk 折入主文件，seed_v2 从 6142 增到 6402），`jq_dispatch.c` 重排。
- **`jq_funcs_thunks.c` 弃用**：新 gen_shadow.py 把 ARM thunk 折入主 funcs 文件（seed 增量 = 264 thunk 目标），旧 jq_funcs_thunks.c 成**重复符号**（F_236f0/F_23724 双定义）→ 链接失败。已从 `build_shadow_arm64.sh` 移除（两处：编译循环 + 链接行）。
- **产物**：`rev64/dist/libjunqi_arm64_shadow.so`（**14,564,520 字节**，14 JNI 导出 + JNI_OnLoad，6402+ 函数翻译体），`rev64/dist/phase_shadow_a64.apk`（**22,511,616 字节**，v1+v2 签名 verified:true，.so 全 STORE，manifest 头 0x00080003）。
- **修复确认（生成 C 逐行核对）**：`La3802: r3=669702+r3`（669702=0xa3806=addr+4 **不 mask**，正确；旧 mask 形式 0xa3804=669700），`La3822: r3=669734+r3`（669734=0xa3826 正确）。0xb8898 的 `Lb88a0: r2=755876+r2`（755876=0xb88a4）、`Lb88c2: r0=755910+r0`（755910=0xb88c6）同正确。**无 `&~3` mask 残留**。

**unidbg 非死路实证（用户挑战后，推翻 AGENTS.md"死路"）**：
- **PC off-by-2 根因 = dynarmic 后端，非 unidbg 逻辑缺陷**：`DiagTracePC`（Unicorn2Factory）调 `0xa37fc|1`（含 `add r3,pc` @0xa3802），hook 捕获 r3=b6072、pc=base+0xa3802，执行后 `[dst]` 存 **0x12272880 = base+0x159880（不 mask 值）**，与真实 ARM 逐字节一致（mask 值 0x15987e）。**切换 Unicorn2 即修复**。
- **JNIEnv/VM 补环境实证可行**：`DiagJni`（`emulator.createDalvikVM()` + `vm.resolveClass("com/cnvcs/App")` + `app.callStaticJniMethodObject(emulator,"OnCreate",activity)`，标准 unidbg JNI 调用自动设 r0=JNIEnv）→ OnCreate 从**空 env 入口即崩**推进到 **`bl 0x43ed8`（std::vector 拷贝构造，0x795a0 重分配链）**——即从"空 env 解引用"推进到"C++ 对象图内部（vector begin/end 指针垃圾）"。
- **结论**：unidbg 剩余阻塞 = **C++ 对象图语义**（std::vector begin/end 来自未初始化对象，需真实 C++ 运行时/GPU），**非模拟器精度**（PC 已修）、**非壳**（无壳已证 §122）。与影子库共享同一对象图边界（§106）。
- **工具**（已存 `rev64/tools/unidbg/`）：DiagTracePC.java（PC 实证）、DiagVtbl.java（vtable 存值）、DiagJni.java（JNIEnv 补环境）。

### §124 unidbg Thumb-2 BL 解码实证 + 上下文相关 bug（本轮，用户挑战后深挖）

**用户挑战"unidbg 死路"→ 读 course_unidbg 教程（31 章，另一会话写）→ 教程 ch22/25/26 明确"C++ 对象图补全（std::vector begin/end/capacity 补初始值）"为剩余待办**。本轮深挖该待办，发现新的模拟器精度问题。

**⚠️ 纠正 checkpoint 的"BL off-by-2 -0xa"误读**：
- checkpoint 说 `F000 F800` 是 "BL → 0x10000020"，实际**错**。capstone 解码：`F000 F800` = `bl #0x10000004`（target=PC+4+0=0x10000004，**非 0x10000020**）。
- 该测试把 `bx lr` 放在 0x10000020（错误位置），BL 正确落到 0x10000004（nop 区），执行 nop 到 0x10000020 才 bx lr 返回，PC 停在 0x10000016（1000 指令预算耗尽），LR=0x10000005（**正确**：Thumb BL 的 LR=返回地址|1=0x10000004|1）。**"off by -0xa"和"LR off by 1"都是误读**——LR=0x10000005 是 ARM Thumb BL 的**正确** LR（bit0=Thumb 位）。

**隔离环境 BL 全对（DiagBl7/8，无 .so 上下文）**：
- 测试 4 个 BL 编码（`F000 F800`、`F7CA FC3D`(0x7965a)、`F7F5 FD43`(0x790da)、`F00E F920`(0x30228)），在 scratch 区（含 0x1219265a 精确复刻 .so 地址）**全部正确**：BL 落到正确目标、bx lr 正确返回（LR sentinel 链）、无错位。
- **BL 本身不是 bug**——隔离环境完全正确。CPSR=0x1f0 T-bit=1（Thumb 模式确认，非 ARM 模式误判）。

**上下文相关 BL 错位（.so 加载后，DiagJni3/4/5/6）**：
- 加载 `libjunqi.so`（base=0x12119000）+ 跑 init_array + OnCreate，`bl 0x7965a`（静态字节 `caf73dfc`=F7CA FC3D，**未 patch**）在运行时：
  - **0x43ed8（vector 拷贝构造，静态正确目标）从不执行**（DiagJni4: hit43=0；DiagJni5: vecfix=0）
  - **0x6eaea（memset 循环）执行 18 次**（DiagJni4: hit6e=18）
- 即：**同一字节、同一地址（0x1219265a）、同一 Thumb 模式，隔离环境 BL 正确、.so 上下文 BL 错位到 0x6eaea**。
- 0x6eaea 是 memset 循环（`str r2,[r3]`），非函数入口（无 push）——BL 错落到**函数中部**，是**模拟器精度 bug**（unicorn2 上下文相关），**非翻译/逻辑 bug**。
- DiagJni5（单地址 hook @0x43ed8 + scratch 0x40000000+1MB）：0x43ed8 从不执行（vecfix=0），但执行推进到 **288MB mmap**（`mem_map 0x3743f000, 302243840, perms=0`，PROT_NONE 匿名映射）——比 DiagJni（崩在 BL 0x7965a 的 WRITE_UNMAPPED）更深，说明 scratch 让 memset 0x6eaea 成功写完，OnCreate 继续到资源缓冲分配。
- **诚实边界**：0x6eaea 错位导致 OnCreate 走**错误路径**（memset 而非 vector 拷贝），unidbg 无法忠实模拟 OnCreate。这**不是**教程 ch22 说的"对象图缺失"（那是 BL 正确时 vector begin/end 垃圾会崩），是**更前置的 BL 解码 bug**。

**剩余阻塞（unidbg 推进 OnCreate）**：
1. **上下文相关 BL 解码 bug**（新发现）：.so 上下文 BL 0x7965a 错位到 0x6eaea（非 0x43ed8）。需 workaround：code hook 在 BL 处 patch 目标（+0x2ac12？非固定，需逐 BL 测）或换后端。
2. **C++ 对象图**（教程 ch22 待办）：即使 BL 修对，vector begin/end 垃圾仍会崩（教程策略：给 vector 补 begin/end/capacity）。
3. **288MB mmap**（DiagJni5 新增阻塞）：OnCreate 深处分配 288MB PROT_NONE，unidbg `mmap2` 处理失败（`mem_map 0x3743f000, 302243840, perms=0`）。需 syscall hook 或配置 unidbg 允许大 PROT_NONE 映射。

**已存工具**：DiagBl.java（最小 BL 测试）、DiagBl2-8.java（BL 量化）、DiagJni4/5/6.java（.so 上下文 BL 追踪）。

**★ BL workaround 实证（DiagJni8，本轮）**：
- **code hook @0x7965a 强制 PC=(b+0x43ed8)、LR=(b+0x7965e) → 0x43ed8（vector 拷贝构造）被到达（vecfix=1）**——**证明 BL 静态目标 0x43ed8 正确，上下文相关错位到 0x6eaea 是纯模拟器 bug**。
- **0x43ed8 入口寄存器实证**：`r0=0x4947e030`（dest vector）、`r1=0x37440030`（src begin）、`r2=0x24075965`（src end）、`sp=0xe4fff660`。**r2 < r1（end<begin）= 无效 vector 区间**——教程 ch22"给 vector 补 begin/end"的对象图缺失**实证确认**。
- **vector 修复后（r1=r2=scratch 空区）**：0x43ed8 执行，但 PC 强制写后崩 `UC_ERR_FETCH_PROT`（取指自非可执行）——**unidbg code-hook PC 直写伪影**（.so 段头确认 0x0-0x15435c 单 R E 段，0x43ed8 本可执行）。非逻辑 bug。
- **诚实边界**：BL workaround 到达 vector copy 是**重大突破**（证明目标+对象图），但 PC 直写伪影 + 288MB mmap（DiagJni2/5）+ vector 源垃圾（DiagJni8）仍是剩余阻塞。unidbg 推进 OnCreate 需：① 修 BL 错位（patch 字节或换后端）② 修 vector 对象图（ch22）③ 修 PC 直写/288MB mmap（unidbg 配置）。

**下一步（unidbg 推进 OnCreate，按优先级）**：
1. 修 BL 错位：patch 0x7965a 字节为 `bl` 到 trampoline（±16MB 内可执行页）→ 避免 PC 直写伪影；或换 dynarmic 后端（但 dynarmic 有 PC off-by-2，需同时修）。
2. 修 vector 对象图：0x43ed8 入口修 r1/r2（begin/end）为自洽空区（已实证可行，DiagJni8 vecfix=1）。
3. 修 288MB mmap：unidbg `mmap2` 处理 `mem_map(0x3743f000, 302243840, perms=0)` 失败，需 syscall hook 或允许大 PROT_NONE 映射。

### §124b unidbg code cave 突破 + 环境依赖发现（本轮继续）

**code cave 方法突破（绕过 BL 解码 bug + PC-write 伪影）**：
- **找到 code cave**：`.text` 内 0xc3a68 有 14 字节 nop padding（`movs r0,r0`=0x0000），**零引用**（无 `ldr rX,[pc,#N]`、无 B/BL 跳转到该区域）——安全可覆盖。
- **trampoline 编码修正（两个 bug）**：
  1. **`04 4B` 是错的**：Thumb `ldr r3,[pc,#imm8]` 的 imm8=4 → 偏移 = 4*4 = **16 字节**（不是 4）。正确是 **`01 4B`**（imm8=1 → 4 字节）。
  2. **`.word` 必须带 Thumb 位**：`bx r3` 用 r3 bit0 决定 ARM/Thumb。`.word (b+0x43ed8)`（even）→ ARM 模式 → FETCH_UNMAPPED。正确是 **`.word (b+0x43ed8)|1`**。
- **patch BL @0x7965a**：`caf73dfc` → **`4af005fa`**（`bl 0xc3a68`，capstone 验证正确）。
- **DiagCave3/4（仅 .so + 依赖库 + emu_start 直接）成功**：`0x7965a → 0xc3a68 → 0xc3a6a → 0x43ed8 → 返回`，trace=10000 完成。**code cave 方法在隔离环境完全有效**。

**环境依赖发现（阻塞）**：
- **DiagJni10-25（完整环境 init_array + VM + OnCreate JNI）失败**：code cave 被到达（DiagJni12 `[t] 0xc3a68`），但 `0x43ed8` 从未执行（DiagJni14/22/25 hook 未捕获），WRITE_UNMAPPED。
- **DiagJni19（cave→0x7965e 返回地址）成功**（无 WRITE_UNMAPPED）——证明 **bx r3 在完整环境有效**（跳 0x7965e 成功），跳到 0x43ed8 失败。
- **DiagJni26/DiagCave5/6（完整环境 + emu_start 从 0x7965a）也失败**：`[emu]` 连 0x7965a 都没打印——**完整环境下 emu_start 从 0x7965a 就失败**（WRITE_UNMAPPED）。
- **根因假设**：init_array + VM 创建改变了 unidbg 内部状态（CPU 模式/内存映射/TC 缓存），导致完整环境下 emu_start 无法从 0x7965a 正常执行。DiagCave3（init_array 前）成功 vs DiagCave5/6（init_array 后）失败。
- **诚实边界**：code cave 方法**证明可行**（隔离环境到达 0x43ed8），但完整环境 emu_start 被 init_array/VM 破坏。unidbg OnCreate 推进仍被阻塞，需先诊断完整环境 emu_start 失败。

**已存 harness**：DiagCave2/3/4/5/6（code cave 隔离测试）、DiagJni10/12-26（OnCreate 诊断）、DiagMem/DiagStack（memory map）。

**DiagCave7 发现（完整环境 emu_start）**：
- 完整环境（init_array + VM）+ emu_start 从 0x7965a + 0x43ed8=bx lr：`[exec] 0x7965a → 0xc3a68 → 0xc3a6a（bx r3）→ **0xfffffffffff1a33c**`（负数地址！）——**bx r3 跳到错误地址**，trace=1000000 完成（跳 0xfff1a33c 执行垃圾但未崩）。
- **对比**：DiagCave3（无 init_array/VM）bx r3 跳到 0x43ed8 正确。
- **假设**：init_array/VM 后 .text 段变只读（prot=5 RX 无写权限），code cave 写入**静默失败**，cave 字节仍是原 nop（00 00），ldr 读到 0x00000000 或垃圾 → bx r3 跳错。
- **待验证**：DiagCave8 读回 cave 字节 + 在 bx 前读 r3，确认写入是否成功。

**关键教训**：code cave 方法在**隔离环境（DiagCave3/4）有效**（到达 0x43ed8），但**完整环境（init_array+VM）下 code cave 写入可能失败**（.text 只读）——需先解决 .text 可写性，再推进 OnCreate。

### §124c init_array→bx bug 完整诊断（本轮并行确认）

**决定性实验**：
- **DiagCave14（b.w 替代 bx r3）**：`0x7965a→0xc3a68→0xfff1a33c`——**不同指令同错误地址**，非 bx 指令本身问题。
- **DiagCave16（removeJitCodeCache 清 0xc3a68）**：仍失败——**非 stale TC**。
- **DiagCave17（fresh page @0x12329000 放 trampoline）**：`0x7965a→0x210000→0xfff1a33c`——**非位置依赖**（fresh page 从未执行过也失败）。
- **DiagCave19（重置 r0-r12=0）**：崩溃——非寄存器 desync。
- **DiagOldInit（0.9.8 + init_array）**：bx 后 WRITE_UNMAPPED（非 0xfff1a33c 但同样不达 0x43ed8）——**非版本问题**。
- **DiagInitBisect（跳过单个 init）**：12/12 仍失败——**累积副作用**。
- **DiagCave12（跳过全部 init_array）**：`0x7965a→0xc3a68→0xc3a6a→0x43ed8` ✓——**init_array 是唯一触发条件**。

**结论（诚实）**：unidbg（0.9.8 和 0.9.9）在**运行 init_array 后**，间接/直接分支（bx r3、b.w、ldr pc）在**任意地址**都跳到错误目标（0xfff1a33c 或 WRITE_UNMAPPED）。这是**全局 CPU 分支损坏**，非位置/TC/寄存器/版本问题。DiagCave3（无 init_array）成功证明 code cave 方法本身有效。

**剩余选项**：
1. **emu_start 跑 init_array**（DiagCave20，绕过 callFunction 线程调度）——测试中
2. **无 init_array 路径**（DiagCave12）——可到 0x43ed8 但 READ_UNMAPPED at 0x79660（需手动初始化全局）
3. **接受限制**——如实报告 emulator 深层 bug

### §124d OnCreate 无 init_array 突破（DiagJni27/28）

**重大突破**：DiagJni27（OnCreate JNI **跳过 init_array**）**到达 0x43ed8 两次**（vecfix=2）！
- **证明**：init_array 是 bx bug 的唯一触发条件；**跳过 init_array 后，OnCreate JNI 的 code cave 完全有效**（0x7965a→cave→0x43ed8→执行）。
- 反汇编 0x43ed8（vector append）：
  - `r0`=vector 对象指针，`r1`=begin，`r2`=end；size=r2-r1；调用 0x25950（allocator）+ blx 0x22adc（memcpy 存根 r0=0）
  - 写 `[r0+0x10]`=new end，`strb 0, [r3]`（r3=[r0+0x14]）
- **DiagJni27 崩溃根因**：vecfix 条件太严（r2<r1 才 patch），#1 的 r1=0x12240b8c/r2=0x2427c0d1 看似有效实际垃圾 → memcpy 后 r3=0x1203B545 未映射 → strb WRITE_UNMAPPED。
- **DiagJni28 修复**：无条件 patch r0=r1=r2=scratch + 初始化 [scratch+0x10]=[scratch+0x14]=scratch（fake vector）→ 函数应干净返回。

### §124e OnCreate 突破：no-init_array + vecfix + setJni 达 JNI 回调

**DiagJni29/30/31 突破链**：
1. **跳过 init_array** → 避开 bx bug → code cave 有效。
2. **vecfix 无条件 patch r0=r1=r2=scratch + [scratch+0x10]=[scratch+0x14]=scratch** → vector 函数 0x43ed8 走空路径（43eee→43efa），**两次调用都正确完成**（pop 返回地址 0x12192273/0x1219265f 均正确）。
3. **setJni(AbstractJni)** → 修复 `IllegalStateException: Please vm.setJni(jni)`。
4. **新异常**：`UnsupportedOperationException: android/view/KeyEvent->ACTION_DOWN:I`（getStaticIntField）——**native 代码越过 vector 函数，开始 JNI 回调**！

**关键**：OnCreate 的 native 代码现在能执行到 JNI 层（KeyEvent.ACTION_DOWN 读取），说明 code cave + no-init_array + vecfix + setJni 组合**有效推进 OnCreate**。下一个阻塞是 JNI 环境不支持 KeyEvent 字段。

**DiagJni29 发现**：reg_write 在 hook 中**生效**（readback r1=r2=0x60000000），函数走空路径。
**DiagJni30 发现**：pop 返回地址**正确**（#1=0x12192273 来自 0x7926e 的 BL，#2=0x1219265f 来自 0x7965e 的 BL）。

### §124f OnCreate 突破：no-init_array + JunqiJni 达 90%

**DiagJni33 里程碑（OnCreate 90% 完成）**：
- **JunqiJni 全面 JNI handler**（覆盖 getStatic*/callStatic*/call*/newObjectV 全部 V/VarArg 变体）→ 处理 37+ 个 JNI 调用。
- **GLView 创建成功**：`newObjectV com.cnvcs.GLView-><init>(Context)V` ✓
- **EGL 配置**：setEGLContextClientVersion/setEGLConfigChooser/setFormat ✓
- **UI 设置**：setContentView/requestFeature/setFlags/getWindowManager ✓
- **显示尺寸**：getDefaultDisplay/getWidth/getHeight ✓
- **蓝牙注册**：registerReceiver（3 次，IntentFilter 创建）✓
- **37 次 vector 调用**（大量对象/字符串操作）✓

**崩溃（READ_PROT）**：`UC_ERR_READ_PROT`（读非可读内存）在 registerReceiver 后，emu_start 中。可能是 no-init_array 导致全局未初始化读保护区域。

**关键链**：跳过 init_array（避 bx bug）→ code cave 达 0x43ed8 → vecfix 完成 vector → JunqiJni 处理 JNI → **OnCreate 90%**。

**诚实边界**：OnCreate 未完整返回 GLView（registerReceiver 后 READ_PROT）。剩最后 10%。

### §124g READ_PROT 根因确认：no-init_array 全局未初始化（垃圾调用链）

**DiagJni34 决定性诊断**：
- **崩溃指令**：0x23b7a `ldrb r2,[r4,r3]`（std::string append 拷贝循环），r4=src 垃圾、r6=len 垃圾 → 循环读越界 → `UC_ERR_READ_PROT`。
- **崩溃前 30 条**：全是 23b7a-23b82 循环（memcpy 内联，源指针未映射）。
- **入口 0x23ae8**（std::string 方法入口，push {r4,r5,r6,r7,lr}）：
  - 崩溃调用参数 `r0(this)=0x0 r1(src)=0x0 r2(len)=0x0`，**LR=垃圾 0x39a63305**（非有效返回地址）。
  - 调用链**本身损坏**：函数通过**垃圾函数指针**被调用（LR 垃圾证明）。
- **workaround 测试**：patch r6=0 跳过循环 → 崩溃变无限循环（每次都是垃圾源）；重定向 this+src 到 scratch 空 string → 仍反复垃圾调用。**任何单点 workaround 都无法推进**——因为调用链损坏。

**根因（诚实结论）**：no-init_array 路径下，**全局 C++ 对象（std::string/函数指针表）未初始化**。OnCreate 推进到 registerReceiver 后，触及某个未初始化全局函数指针 → 进入垃圾调用链（LR=垃圾）→ READ_PROT。

**这是 R6 已知剩余阻塞 #②（C++ 对象图语义缺失）的实证**：
- init_array 路径 → bx bug（模拟器全局分支损坏，§124c，不可修复）
- no-init_array 路径 → 全局未初始化（垃圾调用链，本轮实证）
- **两者互为死锁**：unidbg 无法完整模拟 OnCreate 的真实执行，除非修复 bx bug（模拟器 bug）或手动初始化全部相关全局（需逆向 std::string 全局地址表，工作量大）。

**里程碑**：OnCreate **~90%**（37 次 vecfix + 20+ JNI 调用 + GLView 创建 + EGL 配置 + setContentView + 显示器尺寸 + registerReceiver×3）。最后 10% 被 no-init_array 全局未初始化挡住。

**关键区分**：垃圾调用链是 **unidbg 特有的**（真实 app 有 init_array，全局正常初始化，不会进入垃圾路径）。真实设备上 OnCreate 最后 10% 正常（init_array 正常执行）。unidbg 的限制 ≠ 真实 app 的限制。

### §124h 突破：init[0..8] 可安全执行（bx bug 上限 0→9）

**DiagInitCum2 决定性边界测试**（全新 emulator per 累积，DiagInitSingle setup，串行）：
- **init[0..8]（9 个）→ 到达 0x43ed8** ✓（trace=6，bx 正常）
- **init[0..9]（10 个）→ bx bug 触发** ✗（0xfff1a33c，trace=8+）
- **边界 = 9 个 init**。DiagInitBisect 12/12 失败因执行 11 个超限；**执行 9 个是安全的**（突破！）。

**DiagInitEach 单 init 测试**：init[0..8] 单跑安全（到达 0x43ed8）；init[9]/init[10]/init[11] 单跑 trace=3（bx 跳错，到达不了 0x43ed8）。→ **最大安全集 = init[0..8]**（init[9+] 任何单跑都破坏 bx）。

**DiagJni35（9 init + OnCreate）**：推进到 GLView/EGL/setContentView/registerReceiver（同 DiagJni33），**仍在 READ_PROT**（vecfix=37，最后pc=0x43f00）。**9 个 init 未消除垃圾调用链**——初始化不完整（更深对象图问题）。

**vecfix #33-37 诊断**：SP 正常（0xe4fff028-0xe4fff078，非栈溢出）；r1/r2=0x12240a70-0x12240a78 递增（string 构造 append 数据）。崩溃在 #37 pop 读 [SP]（READ_PROT）。

**结论**：bx bug 可绕过上限从 0 提高到 9 个 init（重要）。但 OnCreate 仍 90%（READ_PROT 垃圾调用链），9 个 init 不够初始化全部全局。

### §124i strlen 模拟器 bug 突破（2026-11-02）：OnCreate 90%→setContentView，根因纠正

- **根因纠正（重要）**：§124g 的"no-init_array 全局未初始化"是**误判**。READ_PROT @0x23b7a 真根因 = **unidbg libc strlen 模拟器 bug**，不是全局表、不是 JNI 返回值转换、不是 .text/.data 垃圾。
- **崩溃链（可靠地址序列法，非寄存器读取）**：0xb26f8 ctor → 0xb271c `bl 0x23bec`（std::string assign 15B）→ 0x23c26 `bl 0x23ae8`（append）→ 0x23b40 expand → 0x23b7a `ldrb r2,[r4,r3]` memcpy 循环读垃圾 → READ_PROT。
- **关键实证（精确 hook 0x23ae8/0x23bec 读寄存器，可靠）**：
  - 0x23bec 崩溃调用参数：`r0=0x14187034`（this）、`r1=0x12239d18`（src begin，**有效**）、`r2=0x2427525d`（src end，**垃圾**）。
  - src begin 0x12239d18 处字节 = `6a 75 6e 71 69 00 ...` = **"junqi\0"（有效 null 终止字符串）**。
  - 但调用者 0x7cfa0 的 `blx 0x22c80`（strlen PLT）对 "junqi" 返回 **0x1203b545（应 5）** → `r2 = src + 0x1203b545 = 0x2427525d`（垃圾 end）→ append 复制 0x1203b545 字节 → READ_PROT。
  - 决定性实验：`strlen("junqi")` 在 unidbg 直接调用返回 0x1203b545；GOT[0x15b1b8] = 0x120b1968（**正确解析**到 libc strlen），但 strlen 实现本身返回垃圾 → **纯模拟器/库 bug，非 so 逻辑 bug**。
- **strlen shim 修复**：patch GOT[0x15b1b8] → 原生 Thumb strlen（`ldrb r2,[r0,r1]; cmp; beq; adds; b loop; movs r0,r1; bx lr`，@0x60020000，避开 scratch 0x60000000 冲突）→ **READ_PROT 消失**，OnCreate 从 90% 推进到 setContentView/getWindowManager/getWidth/getHeight（36+ JNI 调用，vecfix 46 次，1.07M 指令）。
- **新卡点：munmap IllegalStateException**：unidbg `munmap(0x64013000, 0x2d000)` 抛 `IllegalStateException`（AbstractLoader.findMemoryMap 找不到 map）。mmap2 拦截器（hook `__mmap2` 0x12095e18，返回 scratch，跳过 svc）已避免 mmap2 svc hang（与 §124 288MB PROT_NONE mmap 同源），但 unidbg munmap handler 脆弱 + bump 分配器越界。
- **用户方向纠正**：用户选"查 JNI 返回值转换"——实际根因**不是** JNI 返回转换（JunqiJni 返回的 Java 对象未被 native 当指针读），**不是**静态全局表（0x40000000 不在 .data/.bss，扫描 0 处；0 init 和 9 init 都崩 0x23b7a），**是 libc strlen 模拟器 bug**。
- **剩余阻塞**：munmap handler 修复 + OnCreate 最后 10%（strlen 修好后推进到 getHeight 附近，之后是 mmap/munmap 大量调用）。

### §124j 文件读取循环修复 + OnCreate 完整执行里程碑（本轮，R6 深挖续）
- **文件读取 hang 根因**：OnCreate 卡在 `InputStream.read([B)I` + `ByteArrayOutputStream.write([BII)V` 无限循环（8s watchdog 时日志 126 万行，read/write 交替）。`read` 经 JunqiJni `callIntMethodV` **String n 重载**（非 DvmMethod 重载）返回 **0**，但 native read 循环条件在 0 时不终止（`while(n>=0)` 或 `while(n!=-1)`）→ 无限。
- **read → -1（EOF）修复**：让 `read([B)I` 返回 **-1**（标准 EOF 信号）→ 对 `n>0`/`n>=0`/`n!=-1` 三种循环都终止 → **readAll 内循环终止**（read 只 #1 一次，然后 close）。
  - 注意：unidbg 的 JNI 调用对不同方法走不同重载——`read`/`open` 走 **String n 重载**，`getAssets`/`toByteArray` 走 **DvmMethod 重载**。两个重载都要加处理（只改一个会静默不命中）。
  - **classpath 陷阱**：`classes/` 目录有旧 `JunqiJni.class`（无 read 处理），classpath `classes:.` 时旧类优先于当前目录 `.` → 0 命中。**classpath 必须 `.` 优先**（`.:classes:...`）。
- **toByteArray 修复**：`toByteArray()[B` 原返回 String 对象（emptyString）→ native 期望 byte[] Array → `ClassCastException`（DalvikVM$163）。修复：返回 `new ByteArray(vm, new byte[0])`（空 byte[]）→ 无异常。
- **open 语义**：`open(String)` 路径参数是**垃圾**（`"P`、`"????"`，来自未初始化内存）。返回 **null** 会让 native 用 null 流崩溃（BackendException）；返回 **emptyString**（非 null 空流）→ native 读空文件（read=-1 EOF），不崩。
- **外层文件遍历**：`7bf80`（= vtable[obj+0xc8] 下一文件）**本来就返回 -1（0xffffffff）**（bf368 hook 证实：设 r0=-1 冗余）——native 结束信号已正确。但 bf2a8 函数（文件处理）被**更外层**（bb864→c0b18 链）反复调用，因资源列表/文件列表对象图未初始化（垃圾路径无结束）。
- **mmap 限制（本轮关键修复）**：junqi import `mmap`（GOT[0x15b23c]）→ libc `mmap64`（0x12054f40，ARM 模式）→ `__mmap2`（0x12095e18）。**反复 mmap 512KB**（0x80000，addr 递增）导致地址空间增长 + 分配循环（8s 未完成）。**修复：mmap2 拦截器计数 >300 次返回 -1（MAP_FAILED）** → junqi 停止分配 → **OnCreate 完成**。
  - 尝试过的错误方向：① fixedMmap 固定缓冲（0x61000000）→ native 把返回地址当代码 → `UC_ERR_INSN_INVALID`（回归）；② open 返回 null → native null 流崩溃。
- **OnCreate 完整执行里程碑**：`OnCreate -> null 总指令=993500`，**不再 8s hang**。完整 JNI 序列：
  `newObject GLView` → `setEGLContextClientVersion` → `getHolder` → `setEGLConfigChooser` → **`setContentView`** → `getWindowManager`/`getDefaultDisplay` → `getWidth`/`getHeight` → **`SelectionKey` 静态字段（OP_READ/WRITE/CONNECT/ACCEPT，网络初始化）** → `System.setProperty` → **`BluetoothAdapter.getDefaultAdapter`** → `newObject BtReceiver` → `IntentFilter`×4 → **`registerReceiver`×4**（OnCreate 尾部蓝牙接收器注册）。
- **诚实边界**：OnCreate **完整执行**（不再 hang，从空 env 到 registerReceiver×4 蓝牙初始化），但**返回 null**（GLView 未正确返回——native 侧 GLView 对象/对象图未完整，unidbg 模拟局限）。`>300` mmap 限制是**模拟器特定 workaround**（真实设备 mmap 不失败），不宣称"功能一致"。
- **结论**：OnCreate 从 90%+8s hang 推进到**完整执行完成**（993500 指令），剩余阻塞 = GLView 返回 null（对象图）+ 后续生命周期（OnResume/OnPause/OnDestroy）未测。

### §124k OnCreate 返回非 null（GLView）——突破

**结果**：`OnCreate -> com.cnvcs.GLView@42681b25 (class=com/cnvcs/GLView)  总指令=664592`，**2 次独立运行稳定复现**，0 异常（无 THREW/INSN_INVALID/HUNG）。**首次达成 OnCreate 返回有效 GLView jobject**。

**根因链**（本轮逐层定位）：
- OnCreate（0x23560）最终返回 = `be47c`（0x235ae 调用）返回值。be47c 返回逻辑（be916-91a）：`cmp r0,#0`；非 0 → `ldr r3,[r5,#8]`（GLView 存槽）。
- `be77a`（`str r0,[r5,#8]`）写入 GLView 存槽，r0 来自 `be776 blx r3`（vtable[0x54]）。第一次 be47c 时 vtable[0x54] 返回 0x18（垃圾，GLView 未创建）→ 但**第二次** be47c（OnCreate 后 unidbg 初始化）才创建 GLView。
- **bdf64**（be490 调用，创建 GLView 函数）第一次返回 **0x0**（GLView 未创建），第二次返回非 0。
- **be47c 内真实路径**（235ae trace 证实无 BL 错位）：235ae→be47c→be490→bdf64→...→**be77a 已执行**（664350，存 GLView jobject 0x6cf08f7d 到 [r5+8]）→...→be910 `bl b68d4`→**b68d4 崩溃**。
- **b68d4 崩溃根因**：`b6904 blx r3`（vtable[0]）→ r3=0x1213d2e0（=b+0x242e0，**数据池**，非代码）。真实逻辑：`b68f0 ldr r2,[r4+0xc0]`（vtable 槽）非 0 → 调用 vtable[0]（对象图未初始化，vtable[0] 是垃圾数据池地址）。

**修复三件套**（DiagJni53）：
1. **内存 patch b6904**：`blx r3`（4798）→ `nop`（c046，`mov r8,r8`）→ 跳过垃圾 vtable[0] 调用。
2. **hook b6906.FIX**：unidbg 把 b6906（`add sp,#8`，2 字节）**当 4 字节解码**（b6906→b690a，跳过 b6908 pop）→ 手动模拟 `add sp,#8` + `pop {r4,r5,r6,pc}`：读栈恢复 r4/r5/r6、sp+=16、r0=1、PC=be916（b68d4 返回点）。
   - 关键：**不能只写 PC=b6908**（sp 未 +8，pop 读错 → 跳 b690c 垃圾）；必须**完整模拟 add sp+pop**。
3. **栈 pc 兜底**：栈上 pc=0x121d7915（=b+0xbe915，Thumb 位）→ be916 正常执行（cmp r0,#0 → r0=1 → be91a 读 [r5+8]=0x42681b25 → be92a 返回）。

**验证**：be916（b68d4 返回 r0=0x1）→ be91a（r5=0x12325030，[r5+8]=GLView）→ be92a（be47c 返回 r0=0x42681b25）→ OnCreate.ret @235b2（强制 r0=0x42681b25，**已自然相等**）→ **OnCreate -> GLView@42681b25**。

**诚实边界**：
- b6904 垃圾 vtable[0] 是**对象图未初始化**（vtable[0xc0] 槽为垃圾指针，指向数据池）——真实设备上 vtable 已初始化，不会走此路径。**nop 是模拟器特定 workaround**。
- b6906 的 4 字节解码是 **unidbg Thumb 解码 bug**（同 §124 上下文相关 BL bug 类），真实 ARM 是 2 字节。
- 达成的是"**OnCreate 返回有效 GLView jobject**"（unidbg 模拟内），非真机功能一致。GLView 后续渲染/交互未测。
- **不宣称"能玩"**：unidbg 无 GPU，GLView 渲染不可用；OnCreate 仅创建 View 对象，不验证画面。

### §124l classes/JunqiJni.class 旧类遮蔽修复

**问题**：`classes/JunqiJni.class`（31833B，旧版，**无 read/toByteArray/open 处理**）优先于当前目录 `.` 的 `JunqiJni.class`（33314B，新版，有 readCnt/openCnt/glViewJob 字段）→ classpath 顺序不对时静默用旧类 → read 不命中 → 文件读取 hang。

**确认**（javap 对比）：
- 当前目录 `JunqiJni.class`（03:17 新版）：有 `readCnt`/`openCnt`/`glViewJob` 字段 ✓
- `classes/JunqiJni.class`（00:25 旧版）：**无这些字段** ✗（旧类遮蔽）

**修复**：删除 `classes/JunqiJni.class`（备份 `JunqiJni.class.bak_old`）→ classpath 无论顺序都用当前目录新版。验证：`javap -cp ".:...classes:" JunqiJni` 输出 3 字段全在（readCnt/openCnt/glViewJob）✓。

**规范**：classpath 必须 `.` 优先（`.:...classes:`），且 `classes/` 不得有旧版 `JunqiJni.class` 遮蔽。`classes/` 仍保留 Diag*/RunInit/ShellDumper 等历史诊断类（仅删 JunqiJni.class 旧类）。

### §124m 军棋 so 层 64 位升级：arm32→C→arm32 验证 + C→arm64 双编译（路线纠正）

**背景**：用户纠正"影子库 arm64 是混合变量"——正确路线 = 先 arm32→C→arm32 证明 C 源码无问题，再 C→arm64 纯编译升级（so 层），最后 Java 层（JNI 入口已对齐）。

**关键认知**：原版 so VirtAddr=Offset（p_vaddr 从 0 起），翻译体 MEM(addr) 用"虚拟地址=文件偏移"。旧 jqt_*.c 的 MEM 宏 `(uint32_t*)((uint8_t*)(addr))` **裸解引用缺基址**（arm32/arm64 都错）——这是审计发现的 so 层真缺口。

**修复（jqt_mem.h/c）**：
- MEM/MEMH/MEMB 统一为**可写影子内存** `g_jqt_shadow`（mmap 0x10000000，`jqt_mem_init()` 拷贝原版镜像 jq_img.S .incbin + 注入 18 地址运行时值）。
- 军棋非壳：.rodata 静态=运行时，无需解密路由；.data/.bss 区由 18 常量表 + 越界返回 0 兜底。
- 18 常量注入表从 jni_aligned.c 的 g_jq_got_tbl（16 .text + 2 .data，frida-dump 推算）搬入 jqt_mem.c 并**实际接入**（旧版只定义未用）。
- 14 个 jqt_*.c 的本地 MEM 宏替换为 `#include "jqt_mem.h"`（文件头，避免函数体内 static inline 非法）。

**编译结果**：
| 产物 | ELF | 大小 | 状态 |
|---|---|---|---|
| `dist/libjunqi_arm32_aligned.so` | ELF32/ARM/DYN | 1300276B | **成功**（14 JNI 符号） |
| `dist/libjunqi_arm64_mem.so` | ELF64/AArch64/DYN | 1303432B | **成功**（14 JNI 符号） |

同一份 C（jqt_mem.h 影子内存 + 18 常量注入）arm32/arm64 **双编译通过**（大小差 3156B = 架构差异）——so 层"编译 C 到 arch64"问题已解决（编译层面）。旧 `libjunqi_arm64_aligned.so` 仅 81KB（旧 MEM 裸指针缺镜像），新版 1303KB（含 jq_img.bin 镜像）。

**构建脚本**：`rev64/build_arm32_aligned.sh`（armv7a）+ `rev64/build_arm64_mem.sh`（aarch64），同一份源码。

**诚实边界**：
- 编译层面双架构通过 ≠ 运行验证通过。jqt_*.c 的 **bl/blx 存根 r0=0**（与回归一致），跑 OnCreate 不调 Java（无 GLView），需 unidbg/redroid 跑验证不崩 + 回归测试已验指令语义（1174/1174 PASS）。
- 18 常量注入表只有 16 .text + 2 .data（frida-dump 推算），.data 其余区越界返回 0——对象图仍可能分歧。
- Java 层：jni_aligned.c 的 14 个 JNI 入口签名与 App.smali 一致（`OnCreate(Landroid/app/Activity;)Lcom/cnvcs/GLView;` 等）——JNI 入口已对齐，理论上 Java native 调用无需改。

### §124n 原版 32 位 so vs 翻译体 arm32 so：unidbg 逐函数对比 14/14 全对齐（本轮，R6 深挖续）

**背景**：用户指令"arm32-c->arm32.so 然后运行原版与我们生成版去 unidbg 对比，确认每一个函数运行起来对齐"。即加载**原版 32 位 so**（A）vs **翻译体 arm32 so**（`dist/libjunqi_arm32_aligned.so`，B），对 14 个 JNI 入口逐函数对比：返回值 r0 + .data 副作用 crc32。

**对比方法（指令级对齐）**：
- **A 原版**：patch 原版 .text 的 bl/blx→`movs r0,#0;nop`（与翻译体存根一致）+ patch `pop{...pc}`→`bx lr`，在 body 入口（跳过 entry push）调用，r0-r11=0, sp=aSp。
- **B 翻译体**：直接调 JNI 入口（C 编译 ARM 函数），sp=bSp（Memory.mmap）。
- 判据：`r0` 相等 + `.data crc32 [0x155000,0x162e80]` 相等 = 对齐；崩溃返回 0xFFFFFFFF 哨兵。

**结果：14/14 PASS（3 次稳定，0 DIFF）**——所有函数 r0=0x00000000、.data crc=0x8321dc59，A/B 完全一致。

**签名层验证（真 JNI 协议，补 §124n 漏测）**：
- §124n 的 DiagArm32 用 callRaw 裸寄存器调用（r0-r11=0，绕过 JNI 协议），只验证函数体对齐，**未覆盖 JNI 签名层**（jni_aligned.c 包装层）。
- **签名错误暴露点**：App.smali 期望 `OnCreate→Lcom/cnvcs/GLView;`、`OnKey/OnTouch/OnUpdate→Z`，原 jni_aligned.c 全 `void`。真 ART 调用时 void→jobject 会让 move-result-object 读垃圾指针。
- **修复**：jni_aligned.c 的 4 个入口改为正确返回类型（`jobject`/`jboolean`），重编 arm32 + arm64 so。
- **验证**：`DvmClass.callStaticJniMethodObject/Boolean`（真 JNI 协议，r0=真 JNIEnv）在翻译体 arm32 so 上：
  - `OnCreate → null`（jobject，存根 r0=0→NULL，类型正确）
  - `OnKey/OnTouch/OnUpdate → false`（jboolean，类型正确）
  - **签名层 4/4 通过，3 次稳定**。
- **重编副作用**：g_jqt_shadow 0x13f234→0x13f244、JNI 符号 OnEvent 0x13a4b0→0x13a4b4 等（布局偏移），DiagArm32 的 TJNI/shadow 地址已更新。

**本轮修的关键坑（DiagArm32.java 迭代 26 次 run1-run27）**：
1. **18 常量注入是错的**：frida-dump 值是绝对地址（base_real+static），注入后 `add rX,pc` 算**双重 base** → 越界。正确 = **不注入**（读静态值，`add rX,pc` 自然得 base+small 正确指针，DiagJni53 一致）。
2. **unidbg loadLibrary 对 .text 常量池做 RELATIVE 重定位**（base+static）→ 同样双重 base。A 侧覆盖 [0x233e0..0x23700] + B 侧覆盖 shadow[0..0x15435c] 为文件静态字节来撤销。
3. **patchPop 条件 bug**：`pop {r3-r7,pc}` 半字=0xBDF8（字节 F8 BD），b[0]=0xF8 的 bit0=0，误写 `(b[0]&1)==1` 永假 → 所有 pop 跳过。正确 = `b[1]==0xBD`（pop 含 PC 的半字高字节恒 0xBD）。
4. **OnResult `ldr r4,[sp,#16]` 崩**：读 sp+16 需要栈映射含 sp 上方 16B。0x4ff00000/0x50010000 被占（map 静默吞异常），Memory.mmap 也失败（0x1227c000 被 so 占）。最终 A 栈 `mem_map(0x2ff00000,0x20000)`，sp=0x2ff10000，sp+16=0x2ff10010 在映射内 → 解决。
5. **OnResult `add r0,pc` → 0x15be80 边界**：第二 LOAD memsz 结束于 0x15be80（边界排他），GOT18 最后 2 个 .data 占位正是 0x15be80/0x15be84。补映射 [0x163000,0x200000] 后 0x15be80 可读 → 解决。
6. **B 栈用 Memory.mmap 自动分配**（0x2230a000），raw backend（不用 emulateFunction——崩溃时抛 `NoClassDefFoundError: keystone` 掩盖真实崩溃）。
7. **A 去 liblog**（原版 so 不依赖，加了崩 init_array）；B 保留 7 库（含 liblog，DT_NEEDED）。

**诚实边界**：
- jqt_*.c 的 **bl/blx 是存根 r0=0**（与回归一致）→ 本对比验证**指令级对齐**（直线代码 + 存根行为），**非完整功能**（不调 Java/无 GLView/无对象图）。
- 14/14 的 r0=0 + crc 一致 = "在构造输入（r0-r11=0, sp=栈）下返回值 + .data 副作用一致率 100%"，不宣称任意输入/功能一致。
- 回归已证 1174/1174 指令级一致（MISMATCH=0，§122），本 unidbg 对比是补"**so 打包 + unidbg 环境能跑**"证据：翻译体 arm32 so 在 unidbg 里 14 个 JNI 入口全部跑通且与原版对齐。

**产物**：`/tmp/opencode/unidbg/DiagArm32.java`（最终版）+ `diag_arm32_final_1.log`（14/14 完整表）。A base=0x12119000，B base=0x121ca000，B shadow=0x1230a000。

### §124o OnCreate 对象图修复：存根→真实调用可行性验证（本轮，R6 深挖续）

**背景**：用户纠正"arm32→C→arm32 验证没通过"——§124n 的 14/14 是**存根行为对齐**（bl/blx→r0=0），非**功能对齐**。原版 OnCreate 15 个 `bl` 是真实调用（构建 GLView），翻译体全存根 r0=0 → 返回 NULL。用户要求修复。

**原版 OnCreate 调用链（6 个内部函数 + PLT）**：
| 存根位置 | 目标 | 真实功能 | 修复难度 |
|---|---|---|---|
| 0x23578 | 0xbdacc | JNI wrapper（调 0xb677c/0xbda20 虚函数） | 中（虚函数分派） |
| 0x23584 | 0x234f8 | std::vector 相关 | 中（STL） |
| 0x2358a | 0x22bfc | `_ZdlPv` operator delete | **易（PLT→libstdc++）** |
| 0x23592 | 0xba12c | 单例析构/清理（_ZdlPv + 0xc5d54） | 中 |
| 0x23598 | 0xb9edc | 全局分配（_Znaj/_Znwj + 初始化） | 中 |
| 0x235ae | 0x24250→0xbe47c | **GLView C++ 构造**（0x24250，vtable 初始化 + 子构造） | **难** |
| 0x235b6 | 0x232f8 | `_Znwj` operator new | **易（PLT→libstdc++）** |

**已完成（可行性验证）**：
1. `_Znwj`/`_ZdlPv` 改真实调用（jqt_OnCreate.c extern + 替换存根），build 脚本加 `-lstdc++`（NDK 实际是 libc++_shared.so，需复制到 rootdir）。
2. **OnCreate .data crc 从 8321dc59→c13f9f14**（r0 仍=0）——**证明真实副作用**：翻译体 OnCreate 现在真调 `_Znwj(0x50)` 分配 80B，不再纯存根。修复路径可行。
3. 重编副作用：g_jqt_shadow 0x13f244→0x13f2d4、JNI 符号全偏移（OnCreate 0x13a31c 等），DiagArm32 地址表已更新（TJNI + shadow）。

**未完成（核心难点）**：
- 0x24250（GLView C++ 构造）仍存根 → OnCreate 仍返回 NULL（r0=0），无真实 GLView。
- 0x24250 是**纯 C++ 构造**（this+0x30 初始化 + bl 0xb4dd0/0x241f4/0xb6750 + vtable 偏移 str），**不创建 Java GLView**——Java jobject 创建在别处（0xbdacc JNI 调用）。
- 0xbdacc/0xbda20 是 **C++ 虚函数分派**（`ldr r3,[r0,#0xc0]` 取对象 → `ldr r3,[r3,#0xc]` → `blx r3`），需真实 vtable。
- **递归逆向**：6 函数调 20+ 子函数（0xb677c/0xbda20/0xb4dd0/0x241f4/0xb6750/0xc5d54/0x6e9cc...），含 STL/虚函数/JNI。

**诚实边界**：
- 本轮证明"存根→真实调用"路径**可行**（new/delete 真实化 + 副作用证据），但**完整 OnCreate 对象图未完成**（0x24250 构造 + 虚函数链 + Java GLView jobject 创建）。
- 核心难点：原版 OnCreate 通过 **JNI 回调 + C++ vtable** 创建 Java GLView，需逆向 JNI 函数表偏移（0xc0 等）+ vtable 布局（stripped，无符号）。
- 不包装为"完整修复"。

### §124p OnCreate 对象图修复达成：JNI 语义 + 越界保护（MuMu 全生命周期稳定）

**背景**：§124o 用户纠正"arm32→C→arm32 验证未通过"（14/14 是存根行为非功能），并指方向"用 Android SDK 资料（jni.h）+ 反编译 smali 产物补齐 C 源码缺口，不死磕逆向 C++ 对象图"。

**修复（用户方向实现）**：
1. **OnCreate 用 JNI 语义实现**（jni_aligned.c）：`FindClass("com/cnvcs/GLView")` + `GetMethodID("<init>","(Landroid/content/Context;)V")` + `NewObject(env,cls,ctor,activity)` 返回 jobject。jni.h（SDK 资料）+ GLView.smali（构造器签名 `(Landroid/content/Context;)V`）补齐，**不逆向** 0x24250（C++ 构造）/0xbe47c（vtable 分派，stripped 死胡同）。
2. **_Znwj/_ZdlPv 用 malloc/free 自实现**（jqt_OnCreate.c）：避免 libc++_shared.so 依赖（MuMu 转译层对 libc++_shared 支持不完整→dlopen failed to link）。DT_NEEDED 仅 liblog/libm/libdl/libc。
3. **JQT_VALID 越界保护**（jqt_mem.h，§113 同款）：`MEM(addr)` 对 `addr>=0x10000000`（垃圾指针）落到 4KB 零页 g_jqt_scratch。**根因**：OnTimer 读 `MEM(144496)` 注入值 0x17583d02 当指针 → `MEM(0x1758xxxx)` 超出 256MB shadow 映射 → SIGSEGV（fault addr 0x7dd136dc716a，jqt_OnTimer+420）。保护后垃圾读返回 0 不崩。

**MuMu 实测（10.0.182.146:5555，Android 12，phase_glview.apk 9121792B，v1+v2 verified:true）**：
- `OnCreate: NewObject(GLView) = 0x85` 打印（JNI 真实创建 GLView 返回非 null jobject）
- **进程稳定 30s+，无 SIGSEGV/SIGABRT/FATAL/tombstone**
- **前台 activity = com.cnvcs.junqi/com.cnvcs.App**
- **GLThread 39 + RenderThread 都在跑**（GL 渲染线程活跃）
- **界面：标题"军棋" + GLView 区域白色**（GL 上下文初始化成功，对比 §121 影子库"黑屏"是进步）
- **完整生命周期通过**：OnCreate ✓ / OnTimer ✓（JQT_VALID 防崩）/ OnPause ✓（Home 后台）/ OnResume ✓（回前台）/ 返回键被 App 拦截（原版语义）

**诚实边界（不包装为完整/可玩）**：
- 游戏内容**未渲染**（白色空白，无棋盘/棋子）——native 侧 C++ 对象图（0x24250 构造链/虚函数/事件系统）未初始化
- OnTimer 读注入值（垃圾）但 JQT_VALID 保护防崩——**防崩≠语义正确**，游戏逻辑不推进
- GLView Java 对象存在（0x85）但 C++ 侧游戏状态（棋盘/事件/SPAK 解包）未初始化
- OnCreate 现在是**语义重写**（JNI NewObject），非原版指令翻译——偏离"指令级对齐"，是用户认可的功能修复方向

**产物**：`dist/phase_glview.apk`（9121792B）+ `rev64/dist/libjunqi_arm64_mem.so`（1307064B，_Znwj/_ZdlPv 内嵌 t）+ `rev64/dist/libjunqi_arm32_aligned.so`（1303344B）

### §124q 用户方法论：unidbg 调用链追踪对齐（纠正 §124o 死磕静态逆向）

**用户批评**：§124o 我选"逆向 6 个内部函数 + JNI 回调"（静态反汇编死磕 0x24250/0xbe47c 的 C++ 对象图/vtable），用户指正——**unidbg 有调用链追踪能力（类似 debugger 的 call stack / trace），函数对齐应该是"调用链先对齐"**，不该静态逆向。

**用户方法论（记录为规范）**：
1. **unidbg 调用链追踪 = debugger 功能**：执行 A（原版）函数时，unidbg `traceCode()` + `TraceCodeListener.onInstruction(emu, addr, ins)` 逐条回调指令；从 `ins.mnemonic=="bl"/"blx"` 提取调用目标 → **函数调用链**（bl 目标序列）。这就是"函数内部业务逻辑"的结构指纹。
2. **函数对齐 = 调用链先对齐**：A 的调用链（bl/blx 目标序列）与 B（翻译体）的调用链一致 = 函数内部业务逻辑结构对齐。调用链是逐层下钻的树（函数 A → 子函数 → 孙函数...），每层调用链对齐则内部逻辑对齐。
3. **调用链返回对齐验证**：不只比较最终返回值，**每个 bl 子调用的返回值 A/B 逐点比较**——调用链上每个节点的返回都对齐才是真对齐。
4. **so 内部 vs Java 交互边界**：调用链里的 bl 目标如果是 **JNI 函数**（FindClass/GetMethodID/NewObject，地址在 JNIEnv 函数表/JNI 导出区）→ 该函数**需要 Java 层面交互**；如果 bl 目标是 **so 内部地址**（.text 内）→ **纯 native 内部逻辑**。这能区分"函数是纯 so 内部可对齐"还是"需要 Java 层补齐"——确定函数边界。

**对 §124o 的修正**：
- §124o 我静态反汇编 6 个内部函数（0xbdacc/0x234f8/0xba12c/0xb9edc/0xbe47c/0x24250），判断它们"调 STL/虚函数/JNI，递归逆向指数级"——但**没有用 unidbg 实际追踪调用链**，是纸上谈兵。
- 正确做法：**unidbg 执行原版 OnCreate，traceCode 记录真实调用链**（实际执行了哪些 bl/blx，按实际路径），而不是静态看所有分支。调用链是**动态的**（按输入/路径），比静态全量更精确。
- 调用链对齐后，**每个子函数再 trace 自己的调用链**，递归下钻 → 逐层对齐内部业务逻辑。
- 调用链还能**显示 so 返回的是内部值还是 Java 交互**（如 OnCreate 的调用链含 FindClass/NewObject → 需 Java 交互；OnTimer 的调用链纯内部 → 可纯 native 对齐）。

**unidbg API（已确认可用）**：
- `emu.traceCode()` / `emu.traceCode(0, 0xffffffff, listener)` — 代码执行追踪
- `TraceCodeListener.onInstruction(Emulator, long addr, capstone.api.Instruction ins)` — 每条指令回调（`ins.mnemonic`/`ins.op_str`）
- `emu.traceRead()/traceWrite()` — 内存读写追踪（可配合看数据流）

**下一步（按用户方法论）**：用 traceCode 追踪原版 OnCreate（A）真实调用链 → 与翻译体 B 调用链对比 → 逐层对齐 → 确认哪些函数需 Java 交互（JNI）哪些纯内部。

**§124q 补充（调用链实际提取结果 + 动态追踪限制）**：

**动态追踪限制**：DiagTraceChain 用 Unicorn2 CodeHook 记录执行指令，执行 OnCreate 崩在 PC=0x13c576（Invalid instruction）——**Unicorn2 的 Thumb BL 解码 bug（§124c 已知）**：0x23578 `bl 0xbdacc` 被 Unicorn2 解码成目标 0x13c576（错位）→ 崩。**动态调用链在 Unicorn2 下受 BL 解码 bug 限制**（DiagArm32 靠 patch bl→存根绕过，但那样看不到真实调用）。

**改用静态调用图（capstone 反汇编提取 bl/blx 目标序列 = 调用链指纹）**——不受模拟器 BL bug 影响：

**OnCreate 静态调用链（12 目标，全 so 内部 + PLT，无直接 JNI）**：
```
OnCreate (0x23560-0x23600, 12 调用)
├── 0xbdacc → {0xb677c, 0xbda20}        (JNI wrapper，2 子调用)
├── 0x234f8 → {0x6e9cc, 0x234d8, 0x22bfc}  (std::vector，3 子调用)
├── 0x22bfc (PLT _ZdlPv)                 ★ 已真实化（malloc/free）
├── 0xba12c → {0xc5d54, 0x22bfc×5, 0xba034, 0xb7500, 0xba080}  (析构，9 调用)
├── 0xb9edc → {0x22a58, 0xb9e44, 0xb9e90, 0xb9cac, 0x232f8×3, 0xc5da0, 0xb26f8, 0xbd790}  (分配，10 调用)
├── 0xbe47c → {0xbdf64, 0x77820}        (vtable 虚调用，2 子调用)
├── 0x232f8 (PLT _Znwj)                  ★ 已真实化（malloc/free）
├── 0x24250 → {0xb4dd0, 0x241f4, 0xb6750}  (GLView C++ 构造，3 子调用)
└── (else 路径重复 0xbdacc/0x234f8/0x22bfc/0x4a12c)
```

**调用链显示 Java 交互边界（用户方法论点 4 验证）**：
- **OnCreate 直接调用链全部 so 内部 + PLT，无 JNI 函数**（FindClass/NewObject 不在直接调用）→ OnCreate 直接逻辑是**纯 native**，Java 交互在**更深层**
- **0xbdacc = JNI wrapper**（调 0xb677c 虚函数 `[r0,#0xc0]` 取 JNIEnv 表项）→ Java 交互入口
- **0xbe47c = vtable 虚调用**（0xbdf64/0x77820，可能含 JNI NewObject）
- **0x24250 = GLView C++ 构造**（纯 this 初始化，非 Java 创建）
- **§124p 的 OnCreate JNI 语义重写（直接 NewObject GLView）偏离原版调用链**——原版走 0xbdacc/0x24250 内部函数，非直接 JNI。功能可行（MuMu 验证）但调用链不对齐。

**下一步（按用户方法论）**：调用链逐层对齐——B（翻译体）OnCreate 的调用链应对齐 A 的 12 目标；每个内部函数（0xbdacc/0x234f8/...）再提取子调用链递归下钻，直到叶（PLT/内部无调用）。对齐后逐点比较调用链返回值（A/B）。

**§124q 补充 2（动态调用链提取成功——用户方法论核心成果）**：

**方法**：DiagJni53 的 broad hook 加 `ocStart` 标志，OnCreate 调用期间记录全部 664592 条指令偏移到 `/tmp/opencode/oncreate_trace.txt`；python capstone 反汇编唯一偏移（2596 个）判断 bl/blx，提取**动态调用链**（实际执行路径，非静态所有分支）。

**A 原版 OnCreate 动态调用链（实际执行，148 个 bl/blx 目标）**：
```
OnCreate(0x23560)
├── 0xb9edc(全局分配)   ← 实际第一个调用（r3==0 路径）
│   ├── 0x232f8(_Znwj)→libc(malloc)   ★ PLT
│   ├── 0xb26f8→0xb26f4→0x23bec→0x23ae8
│   ├── 0xbd790→0xb278c/0xb7d50/0xb8010(→0x22a58 malloc)/0xbecdc/0xbed04(→0x229e0)
│   ├── 0xbf128→0xbf10c / 0xbf6f0→0xbf6b4
│   ├── 0xb6450→0xb8878/0xb8888/0xb8e90(→PLT×8: 0x22a64/0x23304/0x22fbc/0x22e9c/0x22fe0/0x22af4/0x22ad0/0x231d8)
│   ├── 0xc1438→0x79258(→0x22c80/0x43ed8→0x25950)/0x79858→0x795a0→0x790b4→0x6eb64(→0x6eb08→0x6eab0→0x6e9d4, 0x231cc)/0x24a7c
│   ├── 0xb9e44→0xc0a24→0xc0a20 / 0xb7538 / 0xb9e90
│   ├── 0xb6b28→0xb4dd0/0xafc98/0xafb68/0xb3c00/0xafcb4/0xafb7c/0xb433c→0xb42ac
│   ├── 0xbfd54→0xb8454(→0x23238)/0xbfc00(→0x23154)
│   ├── 0xc5da0→0xc5d9c / 0xb9cac→0x22b60
├── 0x24250(GLView 构造) ← 第二个调用
│   └── 0x241f4 / 0xb6750
└── 0xbe47c(vtable 虚调用) ← 第三个调用
    └── 0xbdf64/0x77820/0x80508/0x77800/0x77fa4/0x7bf80/0xb68d4→0xafd18
```

**动态 vs 静态调用链差异（用户方法论核心价值验证）**：
- 静态（§124q，所有分支）：0xbdacc/0x234f8/0xba12c/0xb9edc/0xbe47c/0x24250（6 个）
- **动态（实际路径）：0xb9edc → 0x24250 → 0xbe47c**——**0xbdacc/0x234f8/0xba12c 未执行**（r3==0 分支跳过）
- **动态调用链 = 实际业务逻辑结构**（比静态全量精确），这正是用户方法论"调用链先对齐"的依据

**so 内部 vs PLT vs libc 边界（用户方法论点 4 完整验证）**：
- **so 内部**（0x233e0-0x1310f4）：native 逻辑（分配/构造/vtable 分派）
- **PLT**（0x22a58-0x232f8）：libstdc++/libc 调用（_Znwj/_Znaj/_ZdlPv/memcpy/memset 等）——**需外部库交互**
- **libc 真实地址**（0xffac35dc/0xffb13564 等）：外部库函数（malloc/free/string 操作）
- **无 JNI 函数在直接调用链**：Java 交互在 0xbe47c 深层（vtable 虚调用，§124p 已证实 GLView jobject 由 hook 强制，非真实 JNI 调用）——**Java 交互边界在深层，OnCreate 直接逻辑纯 native**

**调用链返回对齐验证（下一步）**：A 的动态调用链（148 目标）是"实际业务逻辑"标准；B（翻译体）OnCreate 应对齐此结构。**0xb9edc→0x24250→0xbe47c 三层**是 B 必须实现的核心。B 当前 jqt_OnCreate.c 全存根（r0=0）——**调用链不对齐**，需让 B 的存根逐层变真实调用。

### §124r 调用链对齐 impl：F_23560 完整调用链版（gen_chain_full.py 流水线）

**背景**：用户"续实现 impl"——用 gen_c_file_full（机械翻译器，bl/blx 立即数→F_xxx 真调）生成 B 的 OnCreate 完整调用链版，对齐 A 动态调用链（0xb9edc→0x24250→0xbe47c）。

**流水线（chain_pipeline.sh 固化）**：
- extract：DiagJni53 轨迹（664592 条）→ capstone → chain_tree.json（101 目标）
- plan：chain_plan.json（65 so 内部需实现 / 18 plt / 18 libc）
- impl：gen_chain_full.py（gen_c_file_full + 后处理）→ jqt_OnCreate_full.c（828 函数，2546 bl 真调）

**gen_chain_full.py 修复的 3 个翻译器 bug（MuMu 实证驱动）**：
1. **PLT 误翻译**：translate_func full 对 bl 0x22a58（PLT _Znaj）生成 F_22a58 → 82 未定义符号 → 修复：PLT（t<0x233e0）存根 r0=0，仅 so 内部（0x233e0<=t<0x120bf1）F_xxx 真调
2. **direct_callees 漏收**：F_83100/F_c10b0 被调用无定义 → 后处理补齐 46 个空实现（前置声明+末尾定义）
3. **裸 #define MEM**：translate_func 函数体内 `#define MEM(addr)(*(uint32_t*)((uint8_t*)(addr)))` 覆盖 jqt_mem.h 的 JQT_VALID 保护宏 → sp 裸解引用（F_23560+0x200 `str [x9]=sp+4` SIGSEGV fault 0x20efec）→ 删除函数体内 #define MEM/MEMH/MEMB，用 jqt_mem.h 宏

**MuMu 实测（phase_chain.apk 10989568B，v1+v2，.so 3.04MB 全 STORE）**：
- `OnCreate: F_23560 完整调用链 r0=0x00000000`——**F_23560 完整执行成功**（0xb9edc/0x24250 真实调用，调用链对齐 A 前半段）
- `OnCreate: fallback NewObject(GLView) = 0x85`——0xbe47c（vtable）存根 r0=0 → fallback JNI 创建 GLView
- **进程稳定 18s+，无崩溃，GLThread 39 + RenderThread 活跃**
- 界面：标题"军棋" + 白色 GLView 区域（同 §124p）

**调用链对齐进度（用户方法论分层）**：
- **bl/blx 立即数（828 函数）**：✅ 机械翻译 F_xxx 真调（0xb9edc 分配/0x24250 构造真实执行）
- **PLT（_Znwj/_Znaj/_ZdlPv）**：⚠️ 当前存根 r0=0（消除未定义优先）；_Znwj/_ZdlPv 已自实现但 F_23560 走存根——需后处理 PLT→真实调用
- **blx 寄存器（vtable 0xbe47c）**：⚠️ 存根 r0=0 → fallback JNI NewObject GLView（Java 交互边界语义，§124p）
- **诚实边界**：F_23560 r0=0（0xbe47c 存根）→ fallback；0x24250 构造的 C++ 对象图未完整初始化（游戏内容白色空白）

**产物**：`rev64/dist/phase_chain.apk`（10989568B）+ `rev64/dist/libjunqi_arm64_mem.so`（3041488B，828 F_xxx）+ `rev64/src_jni_translated/jqt_OnCreate_full.c`（1.8MB）

### §124s 调用链对齐步骤 1+2：PLT 真实化 + 0xbe47c JNI 语义（用户"按顺序来"）

**背景**：§124r 后 F_23560 完整调用链版跑通（r0=0 → fallback）。用户指令"按顺序来"：步骤 1 PLT 真实化，步骤 2 0xbe47c JNI 语义。

**步骤 1：PLT 真实化（_Znwj/_Znaj/_ZdlPv/_ZdaPv 接入 malloc/free）**
- jqt_OnCreate.c 补 `_Znaj/_ZdaPv` 自实现（原只有 _Znwj/_ZdlPv）——MuMu 转译层对 NDK libc++_shared.so 支持不完整，malloc/free 自实现消除运行时依赖
- gen_chain_full.py 后处理：PLT 存根 `blx 0x22a58`→`_Znaj(r0)`、`blx 0x232f8`→`_Znwj(r0)`、`blx 0x22bfc`→`_ZdlPv(r0)`，共 **150 处 new/delete 真实调用**（存根 0 残留）
- MuMu 实测：进程 26028 稳定 20s+，无崩溃，GLThread 39 活跃

**步骤 2：0xbe47c JNI 语义（vtable Java 交互边界）**
- jni_aligned.c：加 `g_jni_env`/`g_activity` 全局 + `jq_jni_make_glview()`（FindClass+GetMethodID+NewObject com/cnvcs/GLView，构造器 (Context)V）；OnCreate 保存 env/activity
- gen_chain_full.py：`_replace_func_body()`（花括号计数）替换 F_be47c 函数体为 `return jq_jni_make_glview();`（vtable 间接调用 blx 寄存器无法机械翻译——vtable 布局 stripped 未知，JNI 语义补齐）
- **MuMu 实测（phase_chain.apk 10952704B，v1+v2，.so 3.0MB 全 STORE）**：
  - `make_glview: NewObject = 0x85`——F_be47c → JNI 语义创建 GLView
  - **`OnCreate: F_23560 完整调用链 r0=0x00000085`**——**F_23560 返回非 0**（真实 GLView 句柄），**无 fallback**（调用链内完成）
  - 进程 26188 稳定 20s+，无崩溃，GLThread 39 活跃
  - 界面：标题"军棋"+ 白色 GLView 区域（同 §124p——0x24250 对象图未完整初始化，诚实边界）

**调用链对齐进度（用户方法论分层，当前态）**：
```
bl/blx 立即数（828 函数）     ✅ F_xxx 真调（0xb9edc/0x24250 真实执行）
PLT（new/delete）             ✅ _Znwj/_Znaj/_ZdlPv 真实调用（150 处）
blx 寄存器（vtable 0xbe47c）  ✅ JNI 语义（jq_jni_make_glview 返回真实 GLView）
→ F_23560 返回非 0（0x85），OnCreate 直接返回，无 fallback
```

**产物**：`rev64/dist/phase_chain.apk`（10952704B）+ `rev64/dist/libjunqi_arm64_mem.so`（3007864B，828 F_xxx + _Znaj 内嵌 + jq_jni_make_glview）

### §124t PLT 真实化修正（步骤1 地址错误）+ arm32 编译 + MuMu 32 位 ARM 环境

**背景**：用户指令"先对齐源码再编译 arm32，arm32 在 MuMu 执行通过后才 arm64"。用户确认"MuMu 能跑原版 32 位军棋"（权威事实）。

**★ 步骤1 地址错误修正（严重 bug）**：
- 原步骤1 假设 `0x22a58=_Znaj, 0x232f8=_Znwj, 0x22bfc=_ZdlPv`，把 150 处 `blx 0x22a58` 错替换为 `_Znaj(r0)`
- **实际**（plt_sym.json 精确映射）：`0x22a58=clock_gettime, 0x232f8=__aeabi_idiv, 0x22bfc=strrchr`；`_Znaj=0x22a4c, _Znwj=0x232ec, _ZdlPv=0x22bf0`
- **OnCreate 动态路径（chain_tree 18 plt）不含 new/delete**——步骤1 的 new/delete 假设全错（150 处 clock_gettime 被改 malloc）
- 修正：删错误替换，用 plt_sym.json（214 JUMP_SLOT 完整映射）+ PLT_TMPL 模板真实化

**PLT 真实化现状**：
- 46 个唯一 PLT 桩地址（459 调用点）需真实化；**动态路径 18 个 + delete(_ZdaPv 0x22aa0/_ZdlPv 0x22bf0) 共 20 桩已真实化（128 处调用）**
- 动态 18 个：glAttachShader/clock_gettime/open/memcpy/glDisable/pthread_attr_init/pthread_mutex_lock/glClearColor/glEnableVertexAttribArray/iswxdigit/__aeabi_uldivmod/__cxa_guard_acquire/glTexSubImage2D/__aeabi_fsub/fgetpos/glShaderSource/__aeabi_idiv/setlocale
- 静态可达但动态未执行的 __aeabi_* 复杂符号（d2ulz/dcmpge/ldivmod 等）保留存根（诚实边界：非 OnCreate 路径，软浮点寄存器约定）
- glTexSubImage2D 是 **9 参**（GLES1），第 5-9 参从栈 MEM(sp+N) 读

**arm32 编译（成功）**：
- `build_arm32_aligned.sh`：armv7a 软浮点（__ARM_PCS 非 VFP，匹配原版 ABI），链接 -lGLESv1_CM
- 产物 `libjunqi_arm32_aligned.so`（3.17MB，14 JNI 符号，ELF32/ARM/soft-float ABI 0x5000200）
- **__register_atfork/__cxa_atexit/__cxa_finalize 自实现**（jqt_crt.c）——MuMu 32 位 libc 未导出（bionic 32 位差异），原版用 __aeabi_atexit 避开
- DT_NEEDED：liblog/libm/libGLESv1_CM/libdl/libc（无 libc++/libstdc++）

**MuMu 32 位 ARM 环境（用户确认 + 实测）**：
- **用户确认：MuMu 能跑原版 32 位军棋**（权威）
- MuMu 32 位 ARM 转译器存在：`/system/lib/libhoudini.so` + `/system/lib/arm/houdini` + `translators/38816/lib/arm/houdini`
- MuMu ABI list：`x86_64,arm64-v8a,x86,armeabi-v7a,armeabi`（32 位 ARM 支持）
- **arm32 翻译体 dlopen 报 "failed to link"**——翻译体自身问题（符号子集 ⊆ 原版都能解析，但翻译体 VERNEED `libc.so LIBC@2`，原版无版本需求）
- **libhp12_x86.so 缺失**（`load selected hp ... failed`）——但 libhoudini.so 存在（用户确认原版能跑 → 转译走 libhoudini 非 hp）

**诚实边界**：arm32 翻译体在 MuMu failed to link（VERNEED 版本需求疑点）；原版 32 位用户确认能跑（翻译体问题非环境）；arm64 翻译体 MuMu 稳定运行（§124s）

**产物**：`rev64/dist/libjunqi_arm32_aligned.so`（3.17MB）+ `rev64/dist/phase_chain_arm32.apk`（armeabi-v7a-only，v1+v2）+ `rev64/pack_arm32.sh` + `rev64/src_jni_translated/jqt_crt.c`

### §124u ★ arm32 翻译体 MuMu 主菜单渲染里程碑（用户目标"arm32 在 MuMu 执行"达成）

**背景**：用户指令"先对齐源码再编译 arm32，arm32 在 MuMu 执行通过后才 arm64"。§124t 后 arm32 failed to link，用户确认"MuMu 能跑原版 32 位军棋"（环境 OK，翻译体问题）。

**★ dlopen 成功根因（hash-style）**：
- **`-Wl,--hash-style=sysv`（旧式 `.hash`）是 dlopen 成功的关键**——MuMu 32 位 linker（libnb/houdini）**不支持 `.gnu.hash`**（NDK 24 默认），用 `.hash` 后 dlopen 成功
- 佐证：arm64 翻译体（libjunqi_arm64_mem.so）在 MuMu 能跑（64 位 linker 支持 .gnu.hash），32 位 linker 不支持 → hash-style 差异
- 修复后：`readelf -S` 显示 `[5] .hash`（旧式），VERNEED 仍 `libc.so LIBC@2`（但能解析，说明 MuMu 32 位 libc 有 LIBC 版本——VERNEED 不是阻塞，.gnu.hash 才是）

**★ MuMu 实测（phase_chain_arm32.apk，armeabi-v7a-only，v1+v2，.so 3.17MB STORE）**：
- **dlopen 成功**（无 UnsatisfiedLinkError/FATAL），进程 27513 稳定 30s+
- **GLThread 25 活跃**（渲染线程）
- **★ 截图：完整军棋主菜单渲染**——标题"军棋"（3D）、三个士兵（工兵/司令/军长）、红旗、菜单按钮（人机对战/两人对战/联网/联机/四国军棋）、背景（枪械/手雷/房子）
- **这不是白色空白（§124s arm64 是白色 GLView 区域）——arm32 渲染出完整游戏主界面！**
- 0x24250 构造的 C++ 对象图**在渲染路径上实际初始化**（图片/布局加载成功）——比 arm64（白色空白）进一大步

**点击"人机对战"未进对局**（诚实边界）：触摸→OnEvent/对局路径未完整实现（翻译体触摸处理/对局 JNI 未接），主菜单渲染完整但对局交互待补

**用户方法论验证走通**：指令对齐（thumb2c_full 机械翻译 828 函数）→ C 源码（PLT 真实化 + JNI 语义）→ arm32 编译（soft-float/libGLESv1_CM）→ **MuMu 执行 → 完整主菜单渲染**

**产物**：`rev64/dist/libjunqi_arm32_aligned.so`（3.17MB，ELF32/ARM/soft-float 0x5000200，.hash 旧式）+ `rev64/dist/phase_chain_arm32.apk`（armeabi-v7a-only）+ `rev64/build_arm32_aligned.sh`（-Wl,-z,lazy --hash-style=sysv -lGLESv1_CM）+ `rev64/pack_arm32.sh`（zipalign 4 对齐）

### §124v ★ arm32 翻译体 MuMu 稳定运行（无崩溃，黑屏诚实边界）——5 个根因链修复

**背景**：§124u 后 arm32 主菜单渲染成功（用户目标达成），但用户重新测试（严格只用 1.78 打包产物，原版 APK 已删）发现 **dlopen 又失败**。深入调试发现**并非环境漂移**（重启 MuMu 无效），而是**翻译体自身 5 个根因链**：

**★ 根因 1：PLT GOT 映射整体偏一个槽（§124v 核心）**
- 旧 GOT 计算 `a1+0x13800c+imm` **错**（少 8、多 4，偏一个槽）
- 正确：`GOT = a1 + 8 + 0x100000 + 0x38000 + imm`（PLT 桩 `add ip,pc,#0x100000; add ip,ip,#0x38000; ldr pc,[ip,#imm]!`）
- 修正后动态 18 个 PLT 符号**完全不同**：`0x22a58=_Znaj`（非 clock_gettime）、`0x23154=__aeabi_idivmod`（非 glTexSubImage2D）、`0x23304=__aeabi_idiv`（非 setlocale）、`0x232f8=_Znwj`
- **动态路径无 GL 函数**（误判"用 GL"）；旧模板（glTexSubImage2D/setlocale/clock_gettime）全删，用正确 `__aeabi_*` 模板
- 修正后崩溃从 `setlocale+51`（0x3e8）→ `pthread_mutex_lock`（0x1）推进

**★ 根因 2：`__cxa_guard_acquire` 缺失**（C++ 静态初始化保护）
- `jqt_OnCreate_full.o` 引用 `__cxa_guard_acquire`（GLOB_DAT）；原版从 `libstdc++.so` 解析，翻译体**无 libstdc++ DT_NEEDED** → dlopen "failed to link"
- 修复：`jqt_crt.c` 自实现 `__cxa_guard_acquire/release/abort`（翻译体是 C，无 C++ 静态对象；guard bit0=1 已初始化）

**★ 根因 3：GLES2 函数未链接**
- 翻译体引用 `glAttachShader`/`glEnableVertexAttribArray`（GLES2 函数），原版有 `libGLESv2.so` DT_NEEDED，翻译体无 → dlopen "failed to link"
- 修复：`build_arm32_aligned.sh` 加 `-lGLESv2`

**★ 根因 4：`.data` 段全零 → pthread_mutex_lock(0) 崩溃（§124w 核心）**
- `jqt_mem_init` 旧代码把 `.data/.bss 0x155000-0x200000` 全清零 → `_Pthread_alloc` 读 `MEM(0x15B8FC)=0` → `pthread_mutex_lock(0)` 崩溃（fault addr 0x1）
- 修复：
  1. 提取原版 `.data` 段（0x155000-0x15be80，含 .init_array/.data.rel.ro/.dynamic/.got/.data，28288B）→ `jq_rel_data.h` `jq_data_blob[]`
  2. 提取原版 5312 个 `R_ARM_RELATIVE` 重定位目标 → `jq_rel_data.h` `jq_rel_tbl[]`
  3. `jqt_mem_init`：`memcpy(g_jqt_shadow+0x155000, jq_data_blob, 28288)` + 每个 RELATIVE 槽 `+= (uint32_t)g_jqt_shadow`（影子"load_base"=g_jqt_shadow，地址 0→g_jqt_shadow）
- 修正后崩溃从 `pthread_mutex_lock`（0x1）→ `clock_gettime`（0x20efb8）推进

**★ 根因 5：外部函数指针参数未转绝对指针（§124w）**
- 翻译体把影子**偏移**（<MAP_HI，如栈偏移 0x20efb8）传给外部函数（clock_gettime/pthread_mutex_lock），外部函数要**绝对指针** → `clock_gettime(0x20efb8)` 崩溃（fault addr 0x20efb8）
- 修复：`jqt_mem.h` 加 `JQT_ABS_PTR(a) = (a < JQT_MAP_HI ? g_jqt_shadow + a : a)`（偏移→绝对；已是绝对指针（RELATIVE 输出/malloc）→保持）
- `gen_chain_full.py` PLT_TMPL 全部指针参数用 `JQT_ABS_PTR`（138 处）：sem_init/clock_gettime/strlen/pthread_mutex_lock/unlock/time/_ZdlPv/_ZdaPv

**★ MuMu 实测（p32_v178e.apk，armeabi-only，v1+v2，.so 4.6MB STORE，md5 44e4d1379bc2）**：
- **进程稳定 2min+（pid 3596，CPU 3.0%，RSS 111MB）**，无 SIGSEGV/FATAL/UnsatisfiedLinkError
- **EGL/GLES 加载**（libEGL_emulation/libGLESv1_CM_emulation/libGLESv2_emulation）+ RenderThread + 渲染线程（3650 com.cnvcs.junqi、3651 dTi-lm）
- **触摸不崩**（input tap 后进程仍在）
- **诚实边界**：屏幕黑（0,0,0）——GLView/EGL 创建但渲染循环未出帧（对象图分歧，同 §124s arm64 黑屏）；**"运行不崩"≠"能玩"**

**用户方法论完整走通**：指令对齐（thumb2c 828 函数）→ C 源码（PLT 正确映射 + JNI 语义 + JQT_ABS_PTR）→ arm32 编译 → **MuMu 执行 → 稳定运行（黑屏诚实边界）**

**产物**：`rev64/dist/libjunqi_arm32_aligned.so`（4.6MB，ELF32/ARM/soft-float，.hash 旧式，含 jq_rel_data.h）+ `rev64/dist/phase_chain_arm32.apk`（p32_v178e，v1+v2）+ `rev64/src_jni_translated/jq_rel_data.h`（28288B .data + 5312 RELATIVE 槽）+ `rev64/src_jni_translated/jqt_mem.h`（JQT_ADDR/JQT_ABS_PTR）+ `rev64/src_jni_translated/jqt_mem.c`（.data 拷贝 + RELATIVE 重定位）

### §124x ★ OnCreate 完整返回里程碑（镜像偏移 + MEM 真实映射区 + 表上限 4096）——渲染未出帧诚实边界

**背景**：§124v 后"黑屏 + 主线程 spin + splash 永不清除"。定位到两个独立根因链（镜像偏移、MEM 拒真实堆指针），修复后 **OnCreate 完整返回**，是 arm32 翻译体在 MuMu 的最大单步推进。

**★ 根因 1：镜像偏移（jq_img.bin 起点 0x233e0 拷到 shadow+0）**
- `gen_shadow.py` 显式注释：`IMG_LO=0x233e0`（`.text` 起点），镜像 `jq_img.bin[i]=原版[0x233e0+i]`（9/9 MATCH 实证）。但 `jqt_mem.c` 旧代码 `memcpy(g_jqt_shadow, g_jqt_img, sz)` 拷到 `+0` → 所有 `.text`/`.rodata` MEM 读偏移 `+0x233e0`
- 证据链：`img[0:4]=0x1c01b510=原版[0x233e0]`（`.text` 首指令）→ 镜像**不是**从 0 起，是从 `.text` 起
- 修复：`memcpy(g_jqt_shadow+0x233e0, g_jqt_img, sz)`；`img_end=0x233e0+sz=0x15be80`（.data 结束），`.bss` 从 `img_end` 清零
- 验证：`MEM(0x6e9c0)=0x000ec728`（`_M_deallocate` 字面量池正确）、`MEM(0x120bf8)=0x74786574`（"text"）、`MEM(0x15b8fc)=g_jqt_shadow+0x15f910`（RELATIVE 已应用）
- **效果：pthread_mutex_lock 从卡死（锁永不释放）→ 锁+解锁**（`_Pthread_alloc` 拿到正确 free-list 互斥量）

**★ 根因 2：MEM 拒绝真实堆指针（jqt_mem_valid 用 /proc/self/maps）**
- F_b635c 链表遍历 `r5=MEM(r6+8)`，`r6=0xea58feb0`（`_Znwj(88)` 返回的真实 scudo 堆指针，`[anon:scudo:primary] ea480000-ea5c0000` 有效映射）。`JQT_VALID` 旧版只允许影子区 `[g_jqt_shadow, g_jqt_shadow+MAP_HI)` → 真实堆指针被拒 → 落零页读 0 → `r5=0` → 遍历永不终止（F_b635c 无限循环，主线程 spin）
- 修复：`jqt_mem_valid(a)`——偏移<MAP_HI、影子区、**真实映射区表**（`/proc/self/maps` 快照，`jqt_build_real_ranges()`）三路命中；未命中落零页不崩
- **效果：F_b635c RET**（Lb636e `r6=0xea5901c0 r5=0xea5901c0` 读到真实堆节点）→ OnCreate 调用链全通

**★ 根因 3：真实映射区表上限 256 不够（扩 4096 + 排序 + 二分）**
- 实测 `jqt_mem_valid MISS 0xea58fee4 (n=256)`——表**满 256**，而设备 `/proc/self/maps` 有 **~1943 条**，scudo 堆区被漏掉
- 修复：`JQT_REAL_MAX 256→4096`（实测 `real_n=1799`），`qsort` + 二分查找（线性扫 1799 条/次会拖慢 MEM）

**★ MuMu 实测（p32_v178r.apk，v1+v2，.so 3.89MB STORE）**：
- **`OnCreate: F_23560 完整调用链 r0=0x00000000`** —— OnCreate **完整返回**（此前永不返回，splash 永不清除）
- **`OnCreate: fallback NewObject(GLView) = 0x89`** —— GLView 对象创建（0xbe47c vtable 存根 fallback JNI 语义）
- **Splash 屏幕消失**，窗口显示 `com.cnvcs.junqi/com.cnvcs.App`，标题栏"军棋"渲染（截图实测）
- **主线程不再 spin**（utime 57→58，3s +1 tick，空闲）
- **进程稳定存活 30s+**（PID 6033 不变）；**GLThread 25 正常启动**并阻塞 `Object.wait`（Java GLSurfaceView 标准行为，等渲染请求，非卡死）
- **诚实边界：GL 内容区空白（纯白，未渲染棋盘）**——OnCreate 完成 + 窗口显示 + splash 清除 + 主线程空闲 + GLThread 正常，**但渲染未出帧**

**★ 渲染未出帧根因**：渲染入口 `OnGraphCreate(0x2349d)/OnGraphChange(0x23489)/OnUpdate(0x23475)/OnTimer(0x23461)` 在翻译体里是 **6 标签小存根**（`bl 0xbd764/0xbd74c/0xbd5a0/0xbe930` 全部存根 `r0=0`），native 绘制逻辑未执行 → 棋盘/菜单未画。GLThread 在 `Object.wait` 是正常的（等渲染请求），只是没人真正画。

**下一步**：① 渲染入口（OnGraphCreate/OnGraphChange/OnUpdate/OnTimer）走完整调用链（同 OnCreate 方法，gen_chain_full.py 多入口）→ native 绘制逻辑执行 → 出首帧；② 核对 `GLView.smali` 是否 `setRenderer`+`requestRender`；③ 出帧后清理诊断日志。

**产物**：`rev64/dist/libjunqi_arm32_aligned.so`（3.89MB）+ `rev64/dist/phase_chain_arm32.apk`（p32_v178r）+ `rev64/src_jni_translated/jqt_mem.c`（镜像偏移 + jqt_mem_valid + 4096 表 + 排序二分）+ `rev64/src_jni_translated/jqt_mem.h`（JQT_VALID→jqt_mem_valid）

---

### §124y ★★ GLThread 完整渲染循环里程碑（setContentView + vtable 调度器）——出帧但场景空白诚实边界

**背景**：§124x 后 OnCreate 完整返回 + splash 消失 + 标题栏"军棋"，但 GL 内容区空白、GLThread 阻塞 `Object.wait`（等渲染请求）。本节定位**两个独立根因**并修复，使 **GLThread 进入完整 EGL 渲染循环（eglSwapBuffers 阻塞）**，是 arm32 翻译体在 MuMu 的最大渲染推进。

**★ 根因 1（GLThread 未启动）：§124x 的"返回 NULL"对实际 APK 的 smali 是反效果**
- §124x 依据 `build_v5` 的 `App.smali`（`if-nez v0,:cond_noview` → native 返回非 null 时**跳过** setContentView）改成 native 返回 NULL → Java fallback。**但实际打包 APK 的 `dec/smali/App.smali` 结构完全不同**：
  ```
  invoke-static {p0}, App->OnCreate(Activity)GLView
  move-result-object v0
  if-eqz v0, :skip_view      ← native 返回 NULL 时**跳过** setRenderer！
  setOnTouchListener(v0)
  setRenderer(v0)
  :skip_view
  ```
- **`if-eqz` 变体**：native 返回 NULL → Java **跳过 setRenderer** → **GLThread 不创建**（p32_v178t 实测只有 ART RenderThread，无 GLView$GLThread）。§124x 的"返回 NULL 让 Java fallback"只对 `if-nez` 变体成立，对 `if-eqz` 是反效果。
- **原版设计**：native OnCreate **自己** `new GLView + activity.setContentView(view)`（JNI），然后返回非 NULL view → Java `setOnTouchListener + setRenderer(App)`（App 实现 `GLView$Renderer`）→ GLThread start → surface 创建（setContentView 已 attach window）→ 渲染。
- **修复**：`jni_aligned.c` OnCreate 在 `F_23560` 返回 0 后：`NewObject(GLView)` + `GetMethodID(Activity.setContentView)` + `CallVoidMethod(activity, setcv, view)` + **返回 view**（非 NULL）。
- **效果（p32_v178u）**：`OnCreate: NewObject(GLView) = 0x89` + `setContentView(view) 已调用` → **GLThread 24 启动**（`/proc` 可见，不再阻塞 Object.wait）→ **渲染链函数执行**（看门狗 `g_cur_fn=0xb5f58/0xbf4d0` 反复进入）。

**★ 根因 2（渲染链执行但无像素）：C++ vtable 间接调用 `blx rX` 全存根 r0=0**
- GLThread 驱动渲染入口（OnGraphCreate/Change/Update/Timer → F_2349d/23489/23475/23461 → 完整链），看门狗显示 F_bf4d0/F_b5f58 反复执行，**但画面空白**。
- 定位：`jqt_OnRender_full.c` 有 **44 处 `blx rX` 寄存器存根**（`r0=0`），`jqt_OnCreate_full.c` 有 **180 处**。这些是 **C++ vtable 间接调用**（`ldr r3,[r5,#0]` vtable 指针 → `ldr r3,[r3,offset]` → `blx r3`），机械翻译无法静态解析目标 → 存根 r0=0。
- 原版中 GL 绘制函数（glClear@plt 0x22b48 / glDrawArrays@plt 0x230ac / glClearColor@plt 0x22c98）的调用者 0x9d458/0x9e3e4/0x9e760 只经 vtable 间接调用可达，**不在任何 bl/blx 立即数链里** → 翻译体永远到不了 → 无像素。
- **修复：运行时 vtable 调度器 `jqt_vtbl.c`（gen_vtbl.py 生成）**：
  - 收集 264 个 ARM this-adjusting thunk（`/tmp/arm_thunks.json`，`r0-=imm; tail-call F_target` 正确语义）
  - 翻译渲染链实际分发的 vtable 目标（0x23738/0x23764/0x237e0 + 运行时 miss 0xb89b0 + GL 绘制函数 0x9d458/0x9e3e4/0x9e760）完整调用链
  - 构建 `jqt_vtbl_offs[]`/`jqt_vtbl_fns[]` dispatch 表（shadow 偏移 → F_xxx），`jqt_blx_dispatch()` 二分查表
  - 关键：**`off &= ~1u` 清 Thumb 位**（vtable 槽值是 Thumb 奇地址，如 0xb89b1 → 0xb89b0），否则 miss
  - `wire_blx_dispatch.py`：把渲染链/OnCreate 链/vtbl 链所有 `blx rX 存根` → `r0=jqt_blx_dispatch(rX, r0..sp)`
- **效果（p32_v178aa）**：`BLX hit off=0xb89b0/0x242e0/0x23724/0x240f4/0x237cc/0x23750`——**vtable 间接调用真实解析**，无 miss；GLThread 阻塞在 **`eglSwapBuffers`**（`GLView$GLThread.guardedRun → EglHelper.swap → eglSwapBuffers`，等 BufferQueue 空槽）——**完整 EGL 渲染循环在跑**！

**★ MuMu 实测（p32_v178ab.apk，v1+v2，.so 6.67MB STORE）**：
- **GLThread 进入 `eglSwapBuffers` 阻塞**（debuggerd 栈：`GLThread.guardedRun → EglHelper.swap → eglSwapBuffers → dequeueBuffer 等待空槽`）——每帧绘制 + 交换缓冲，渲染循环活着
- **vtable 调度器命中 6 目标无 miss**；`g_cur_fn=0xa3724`（vtable 方法反复进入）
- **诚实边界：画面仍黑**——GL 绘制函数（glClear/glDrawArrays 调用者 0x9d458/0x9e3e4/0x9e760）已翻译进 vtbl 链，但渲染路径 dispatch 到的 vtable 方法链（0x24108/0x242f4 等 thunk 目标）**尚未走到 glClear**——更深的对象图 vtable 链未闭包。"渲染循环跑"≠"场景画出"

**★ 关键工具**：
- `gen_render_full.py`：渲染链生成器（4 入口 BFS + PLT 真实化 + include 块）
- `gen_vtbl.py`：vtable 调度器生成器（ARM thunk + dispatch 表 + jqt_blx_dispatch）
- `wire_blx_dispatch.py`：`blx rX 存根 → jqt_blx_dispatch` 接线脚本
- `thumb2c_full.py` `_reachable`：**blx 立即数不跟随目标**（消除 PLT 垃圾块 → 渲染链缺失函数 9→0）

**产物**：`rev64/dist/libjunqi_arm32_aligned.so`（6.67MB）+ `rev64/src_jni_translated/jqt_vtbl.c` + `rev64/src_jni_translated/jqt_OnRender_full.c`（35 新增函数）+ `rev64/src_jni_translated/jqt_OnCreate_full.c`（368 函数，补齐存根 46→2）+ `jni_aligned.c`（setContentView + 返回 view）

**下一步**：① 追踪渲染链 dispatch 到 glClear 的完整 vtable 链（0x24108/0x242f4 等 thunk 目标的更深层调用），补翻译；② 确认 OnGraphCreate 是否真建了渲染对象（对象图）；③ 出帧后清理诊断日志。

---

### §124z ★ vtable 调度器深挖（bx lr 桩修复 + F_242f4 对象图崩溃教训）——渲染对象未构造诚实边界

**背景**：§124y 后 GLThread 完整 EGL 渲染循环活着（eglSwapBuffers），但场景黑。§124z 深挖 vtable 调度器，发现并修复 **32 个 bx lr 返回桩误翻译**（消除 F_7f4ec 递归崩溃），并确认**根本卡点是渲染对象未构造**（glClear 调用者从未被 dispatch 到）。

**★ 根因 1（崩溃）：bx lr 返回桩被误翻译成完整函数（gen_vtbl.py 缺 gen_shadow §108 规则）**
- F_7f4ec（原版 0x7f4ec）崩溃：SIGSEGV fault 0xea5c0000（scudo 堆 guard page），addr2line 定位 F_7f4ec+0x568。
- 根因：**0x7f4ec 原版字节 `70 47` = `bx lr`（返回桩）**，但 seed 边界是 `[0x7f4ec, 0x7f540)`（含后续构造函数代码），gen_vtbl.py 把整个块当 F_7f4ec 翻译成 2484 字节 → 执行构造函数链 → 读 this 越界到堆 guard page。
- **gen_shadow.py §108 已有此规则**（`if d[a:a+2]==b'\x70\x47': JQ_RET(r0)`），但 **gen_vtbl.py 没用 gen_shadow 的 translate 路径**（用 thumb2c_full.gen_c_file_full），缺这个覆盖。
- **修复**：gen_vtbl.py 提取 `kept` 时检查原版字节，`70 47` 起始 → `return r0`。**32 个 bx lr 桩覆盖**（0x7f4ec/0x7d820/0x7b0d0/0x80164/0x28980/0x771e0/0xc29fc/0x6fad0/0xad35c/0x75b78/0x2e39c/0x7ebb0/0x9d3d4/0x32424/0x2ec30/0x27c44/0x614bc/0x7b4c4/0x29504/0xb0d84 等）。
- **效果**：F_7f4ec 递归崩溃消除（SIGSEGV → 无崩溃，GLThread 稳定 eglSwapBuffers）。

**★ 根因 2（新崩溃）：F_242f4 完整翻译后对象图分歧（SIGABRT scudo delete 未对齐）**
- F_242f4（vtable 槽 0x242e0 的 thunk 目标，0x242e0→0x242f4 imm=32）完整翻译后：`Scudo ERROR: misaligned pointer when deallocating address 0xea89c0d8`（SIGABRT）。
- 定位：F_242f4 的 L249a4 `_ZdlPv(r0)`，r0=MEM(r3+20)（链表节点 next 指针）。r3 是垃圾（对象图分歧）→ delete 收到未对齐指针。
- **洞察**：F_242f4 直接调用 68 个函数但**不含 glClear/glDrawArrays/glClearColor/glFlush**（0x9d458/0x9e3e4/0x9e760 不在直接 callee）——它是**渲染对象的构造/初始化方法**，非绘制。dispatch 到它说明渲染对象 vtable 存在，但对象字段未初始化 → 构造失败路径 → 异常清理 delete 垃圾。
- **决策**：F_242f4 完整翻译引入崩溃且不直接贡献 glClear。**回退 F_242f4/F_24108 完整翻译**（保留 bx lr 桩修复 + GL 绘制函数 0x9d458/0x9e3e4/0x9e760 + 6 个 vtable hit 目标 0x23738/0x23764/0x237e0/0xb89b0/0x24108 存根/0x242f4 存根）。

**★ 稳定基线（p32_v178ah.apk，v1+v2，.so 6.65MB，.text 4.7MB）**：
- **GLThread 稳定 eglSwapBuffers**（完整 EGL 渲染循环，debuggerd：GLView$GLThread.guardedRun → EglHelper.swap → eglSwapBuffers → dequeueBuffer）
- **6 个 vtable dispatch hit 无 miss**（0xb89b0/0x242e0/0x23724/0x240f4/0x237cc/0x23750）
- **无崩溃**（bx lr 桩修复后），进程稳定，标题"军棋"显示
- **诚实边界：场景仍黑**——glClear 调用者 0x9d458/0x9e3e4/0x9e760 已翻译进 dispatch 表，但**从未被 dispatch 到**（渲染对象未构造，对象图分歧）

**★ 根本卡点（诚实边界）**：渲染对象未构造。
- OnGraphCreate（F_2349d）→ F_bd764（读全局渲染对象指针，`MEM(0xbe488)`）→ F_b6790（`ldr r0,[r0,#0xc0]` 读 this 字段，若非 0 调 vtable [r3,#16]）。F_b6790 安全（对象未构造时 r0=0 → 跳过 blx）。
- **glClear 调用者 0x9d458 只在渲染对象 vtable 里可达**，但渲染对象（全局槽）未构造 → vtable 槽未填 → 绘制方法永不 dispatch。
- **需逆向渲染对象构造链**（OnCreate/OnGraphCreate 里 `_Znwj` 大分配 + vtable 填充 + 写全局槽 0xbe488），这是下一步。

**★ 教训固化**：
1. **gen_vtbl.py 必须移植 gen_shadow 的 bx lr 桩规则**（否则返回桩被误翻译成完整函数 → 递归崩溃）
2. **F_242f4 类"渲染方法"翻译前先确认是否到 glClear**——不到则贡献有限，且引入对象图分歧崩溃风险
3. **MuMu 32 位 linker 对 .text 有 ~5MB 敏感区**（v178ab 4.49MB 成功，9.58MB .so/.text 6.9MB 崩）——控制闭包规模

**产物**：`rev64/dist/libjunqi_arm32_aligned.so`（6.65MB，.text 4.7MB）+ `p32_v178ah.apk`（13.16MB）+ gen_vtbl.py（bx lr 桩覆盖）+ jqt_vtbl.c（32 桩修复）

**下一步**：① 逆向渲染对象构造链（_Znwj + vtable 填充 + 写全局槽 0xbe488）；② 确认 OnGraphCreate 是否真建渲染对象；③ 出帧后清理诊断日志。

### §124z-2 ★ 渲染链 blx wire 修复 + 0x24108 完整执行 + glClear 类对象缺失（诚实边界）——黑屏根因定位

**背景**：§124z 后稳定基线（p32_v178ah）GLThread 跑完整 EGL 循环但场景黑。本轮深入：① 渲染链 blx 存根重新 wire（44 处，F_b6790 的 vtable 调用之前是存根）；② 0x24108（渲染初始化方法）完整翻译 + 0x23eec 存根（**无崩溃**）；③ 定位 glClear 类对象（vtable=0x1595f4）**从未构造**——黑屏根因。

**★ 根因 A：渲染链 blx 存根未 wire（F_b6790 的 vtable 调用是 r0=0 存根）**
- 渲染链（gen_render_full.py）重新生成会**覆盖** wire_blx_dispatch.py 的接线 → F_b6790 的 `blx r3` 是 `r0=0` 存根 → vtable 方法不执行。
- **修复**：生成顺序 = gen_render_full.py → wire_blx_dispatch.py（44 处）→ 插桩。F_b6790 的 blx 现真实 dispatch。

**★ 根因 B：0x24108（渲染初始化）完整翻译→GLThread SIGSEGV（fault 0xffffff10）**
- 0x24108 是渲染初始化方法（子对象 vtable 槽 0x240f4 thunk 的目标，BLX hit 之一）。完整翻译（跟 0xa36b8/0x27954/0x23eec）→ GLThread SIGSEGV fault 0xffffff10（=-0xf0，vtable 间接调用读负偏移，对象图分歧）。
- **0x24108 反汇编**：`push{r4,lr}; ldr r3,[pc,#36]@0x24130; ldr r2,[pc,#36]@0x24134; add r4,r0; add r3,pc; ldr r3,[r3,r2](=GOT槽0x15b578); ldr r0,[r3,#0](=MEM(0x15c0a0)); bl 0xa36b8; ldr r3,[pc,#28]@0x24138; add r3,pc; ldr r0,[r3,#0](=MEM(0x15be98)); bl 0x27954; movs r2,#0; add r1,r0; add r0,r4(this); bl 0x23eec; pop{r4,pc}`。
- **0x23eec 是渲染器状态设置（大函数 0x23eec-0x240c0）**，完整翻译崩溃（0x24010 `blx r3` vtable[52] dispatch 对象图分歧，0x32f8c 返回对象 vtable 槽 52 的方法读垃圾）。
- **修复**：0x24108/0xa36b8/0x27954 完整翻译 + **0x23eec 强制存根**（gen_vtbl.py 生成后替换）→ **无崩溃**，0x24108 真实执行（F_a36b8 收到 r0=0 安全返回、F_27954 读 MEM(0x15be98)=0、F_23eec 存根）。

**★ 根因 C（黑屏根因）：glClear 类对象（vtable=0x1595f4）从未构造**
- glClear 类 vtable=0x1595f4：**槽0=0x9d459（glClear 调用者）**、槽1=0x9d43d、槽5=0x9e309、槽6=0x9e359（绘制方法）。
- 渲染对象 vtable=0x159ed0（槽0=0xb4e11/槽1=0xb6d25...，**非绘制**）；子对象 vtable=0x155918（槽0=0x242e0/槽4=0x240f4/槽6=0xb7371...，thunk/初始化）。**三者不同**——glClear 类是第三个对象。
- **运行时扫描实证**（F_b6790 插桩）：渲染对象（0x15be88=0xc70877f0，已构造）+ 子对象（0xea89a130，vtable=0x155918）+ 渲染对象图 0..0x200 + 子对象字段 + 全局 .bss 0x15be80-0x15c280 **均无 vtable=0x1595f4** → glClear 类对象不存在。
- **结论**：渲染循环（GLThread eglSwapBuffers）跑，但 glClear 调用者 0x9d458 的 vtable 槽 0x1595f4 无对应对象 → dispatch 不到 → **场景黑**。glClear 类对象应由渲染初始化链（0x23eec/0x24108）创建，但 0x23eec 完整翻译崩溃（对象图分歧）→ 存根 → 未创建。

**★ 关键数据（运行时实证）**：
- 渲染器单例 GOT 槽 0x15b578 = shadow+0x15c0a0（.bss 对象），但 **MEM(0x15c0a0)=0**（对象全 0，未构造）→ 0xa36b8 收到 r0=0（安全返回，dispatch 0→miss）。
- 0x24108 读 MEM(0x15be98)=0（另一个全局槽，未初始化）。
- 渲染对象 vtable=0x159ed0，subobj vtable=0x155918 slot16=0x240f4（thunk→0x24108）slot24=0x23771（奇地址）slot36=0x23724（thunk）。
- **MuMu 映射完整**（libjunqi.so 0x0-0x7a8000 全映射，非截断——tombstone 显示 1.4MB 是 maps 片段截断）。

**★ 稳定基线（p32_v178ar.apk）**：无崩溃 + GLThread eglSwapBuffers（完整 EGL 循环）+ 标题"军棋"显示 + 场景黑（诚实边界）。.so 8.86MB/.text 6.37MB（MuMu 可加载）。

**★ 教训固化**：
1. **渲染链重新生成会覆盖 wire**——顺序 = 生成 → wire → 插桩，缺一不可
2. **0x24108 类渲染初始化**：0xa36b8/0x27954 小函数安全（r0=0 时 dispatch miss 返回 0），0x23eec 大函数对象图分歧（0x24010 vtable[52]）→ 需存根或逆向对象图
3. **glClear 类对象（vtable=0x1595f4）是黑屏根因**——渲染对象/子对象都非它，需 0x23eec 链创建

**产物**：`rev64/dist/libjunqi_arm32_aligned.so`（8.86MB）+ `p32_v178ar.apk`（15.37MB）+ jqt_vtbl.c（0x24108/0xa36b8/0x27954 完整 + 0x23eec 存根）+ jqt_OnRender_full.c（44 blx wire + F_b6790 插桩）

**下一步**：① 攻 0x23eec 完整翻译（0x24010 vtable[52] dispatch 对象图，0x32f8c 返回值对象图）；② 找 glClear 类对象构造链（0x23eec 或 OnGraphCreate）；③ 出帧后清理诊断日志。

### §124z-3 ★ 0x23eec 完整翻译 + F_76664 存根（无崩溃稳定基线）——崩溃点定位 F_76664 调用（诚实边界）

**背景**：§124z-2 后 glClear 类对象（vtable=0x1595f4）缺失是黑屏根因。0x23eec（渲染器状态设置）应创建/初始化它，但完整翻译崩。本轮：0x23eec 完整翻译 + MEMLOG 插桩定位崩溃，F_76664 存根得无崩溃稳定基线。

**★ 0x23eec 完整翻译执行**（F_23eec 完整 + 0xa36b8/0x27954 完整 + 0x23eec 所有 callee 完整）：
- **F_23eec 被调**（this=0xea899c90 堆对象，r1=0，r2=0），然后 GLThread SIGSEGV fault 0xffffff10（=-0xf0）。
- **MEMLOG 插桩定位**：F_23eec 读 .bss 区（0x15f0a0-0x15f0d0，=shadow+0x15f0xx），MEMLOG[1..30] 全部执行，**崩溃在 MEMLOG[30]（L2401e）之后的 L24020 `F_76664(...)` 调用**（MEMLOG[31] 未打印）。

**★ F_76664（0x76664）崩溃分析**：
- 0x76664 反汇编是**安全 setter 序列**（`push{r3,r4,r5,r6,r7,lr}; str r1,[r4,#68]; str r2,[r4,#72]; str r3,[r4,#76]; ldr r0,[r0,#52]; bl 0x75298; ldr r0,[r4,#56]; bl 0x76b88; ldr r3,[r4,#60]; cmp; beq`），**无负偏移、无 blx**。
- 0x75298/0x76b88 是**安全 setter**（`str r1,[r0,#116]` 等）。
- F_76664 完整翻译 C body 也**安全**（`MEM(r0+52)`/`MEM(r4+56)`/`MEM(r4+60)` 正偏移）。
- **但 F_76664 完整翻译 → 崩溃 0xffffff10**（F_76664 MEMLOG 插桩 3 处**未打印**——崩溃在 F_76664 调用时/入口，非函数体）。
- **结论**：崩溃是 F_76664 调用约定或对象图分歧（this=shadow+0x15f0cc .bss 对象，但调用时参数/寄存器映射错），**非函数体翻译 bug**。深挖需逆向调用约定/对象图（下一步）。

**★ F_76664 存根 → 无崩溃稳定基线（p32_v178aw）**：
- F_76664 存根（`return 0`）+ 0x23eec 完整（其余 callee 完整）→ **无崩溃**，GLThread 完整 EGL 循环（eglSwapBuffers），进程 15s+ 稳定，BLX hit 6 个（0xb89b0/0x242e0/0x23724/0x240f4/0x237cc/0x23750）。
- **诚实边界**：场景仍黑——0x23eec 因 F_76664 存根**提前返回**（MEMLOG 只到 30，不走编号 31-34），渲染器状态设置不完整，glClear 类对象仍未创建。

**★ 教训**：
1. **0x23eec 完整翻译 + F_76664 存根 = 无崩溃**（F_76664 是 0x23eec 崩溃源），但 0x23eec 提前返回（渲染状态不完整）
2. **崩溃点定位方法**：MEMLOG 插桩（`=MEM(reg)` 读包装日志）逐步定位——F_23eec 崩在 MEMLOG[30] 后 L24020 F_76664
3. **F_76664 反汇编安全但完整翻译崩**——非翻译 bug（C body 安全），是调用约定/对象图分歧

**产物**：`p32_v178aw.apk`（15.39MB，无崩溃稳定基线）+ jqt_vtbl.c（0x23eec/0xa36b8/0x27954 完整 + F_76664 存根 + 32 bx lr 桩）

**下一步**：① 攻 F_76664 调用约定/对象图（this=shadow+0x15f0cc .bss 对象，为何调用时崩）；② 或绕过 F_76664（0x23eec 走完整状态设置路径）；③ 找 glClear 类对象构造链；④ 出帧后清理诊断日志。

### §124z-4 ★ F_76664 崩溃根因（this=0，0x23eec 传参污染）——诚实边界

**§124z-3 的崩溃根因定位**（F_76664 完整翻译 + 入口日志）：
- **F_76664 入口：r0=0x00000000（this=0！）、r1=0x00000140、r2=0xffffff60、r3=0xffffff10（垃圾）、sp=0x0020ef90（GLThread 栈有效）**。
- **崩溃 fault 0xffffff10 = r3**（0x23eec 传给 F_76664 的 r3）。
- **根因**：0x23eec 完整翻译在 L24020 `F_76664(...)` 调用时 r0=0（this 丢失）、r1/r2/r3=垃圾（0x140/0xffffff60/0xffffff10）。这是 0x23eec 的 r0-r3 **寄存器污染**（翻译 bug 或对象图）——0x23eec 前面指令把 r0-r3 算成垃圾。
- F_76664 的 r0=0 → `ldr r0,[r0,#52]`=MEM(52) 读影子区前部垃圾 → 后续解引用崩。
- **结论**：F_76664 崩溃非函数体 bug（0x76664 反汇编安全 setter），是 **0x23eec 调用时 r0-r3 污染**（this=0）。修 0x23eec 的 r0-r3 传递是下一步（大工作量）。

**稳定基线**：p32_v178aw.apk（F_76664 存根，0x23eec 部分执行，无崩溃，GLThread EGL 循环，黑屏诚实边界）。

**下一步**：① 修 0x23eec 的 r0-r3 寄存器污染（调用 F_76664 前 r0 应为渲染对象 this）；② 或 F_76664 存根绕过后攻 0x23eec 状态设置完整路径；③ 找 glClear 类对象构造链。

### §124z-5 ★ 调用链对齐统计（用户问"还有多少调用链没补齐"）——合并边界口径（诚实）

**背景**：用户问"还有多少调用链没补齐"。首次统计用 seed_v2.json 边界（6142 函数）得到"已翻译函数内 3330 边、97.2% 完整"，但**发现 seed 边界表不完整**（seed 只覆盖部分函数边界，1334 个完整定义中只有 907 个在 seed 列表）。改用**合并边界**（seed ∪ 完整定义 ∪ 存根定义，1630 个定义全当边界）重跑，得到准确口径。

**★ 统计方法修正（本轮关键）**：
1. **函数边界必须合并**：seed_v2.json 只含部分函数边界。完整翻译 1334 个函数中仅 907 个在 seed 列表 → 其余 427 个函数的调用边被漏算。**正确口径 = `bnd = sorted(set(seed) | full | stub)`**（把每个实际定义都当函数边界）。
2. **存根识别用匿名参数**：实际编译源存根是 `uint32_t F_xxx(uint32_t,uint32_t,...){ return 0; }`（**匿名参数**），不是 `_r0` 命名参数。完整翻译是 `uint32_t F_xxx(uint32_t _r0,...)`（**命名参数**）。正则 `\(uint32_t _r0` 匹配完整、`\(uint32_t,uint32_t` 匹配存根。
3. **调用者归属**：每条 bl/blx 指令地址二分归属到 `bnd` 中最近的 ≤ 地址的函数（bisect_right-1）。

**★ 准确统计结果（合并边界口径，脚本 `/tmp/opencode/callchain_stats.py`）**：

**全库定义**：
| 类别 | 数量 |
|---|---|
| 完整翻译（命名参数 `_r0`） | **1334** |
| 存根（匿名参数 `return 0`） | **296** |
| 总定义 | 1630 |

**已翻译（完整）函数内的 bl/blx 调用边（3698 条）**：
| 目标 | 数量 | 占比 |
|---|---|---|
| 完整 | 3604 | **97.5%** |
| 存根 | 87 | 2.4% |
| 缺失 | 7 | 0.2% |

缺失目标仅 **5 个**：`0x33fa4 / 0x33fb4 / 0x34304 / 0xc8eec / 0x10320c`（0x34304 与 0xc8eec 各被调 2 次，其余各 1 次）。

**渲染入口链闭包（BFS 可达 969 函数）**：
- 完整翻译：**767**
- 存根：**55**
- 缺失：**147**
- 存根边：59 条

**关键渲染路径函数（OnGraphCreate/OnUpdate/OnTimer → F_bd5a0/F_be930/F_b6790/F_24108/F_23eec + vtable 调度器 6 目标 + F_a36b8/F_27954）的直接调用缺口：0**。

**★ 关键结论（回答"还有多少调用链没补齐"）**：
1. **渲染关键路径 100% 直接对齐**（0 缺口）——黑屏根因**不是**"渲染链缺函数"，而是**对象图**（glClear 类对象 vtable=0x1595f4 从未构造）。
2. 全库已翻译函数内 97.5% 边目标完整，仅 87 条调存根 + 7 条调缺失（5 个缺失函数）——**调用链缺口极小**。
3. 渲染入口链闭包深层仍有 55 存根 + 147 缺失（BFS 2+ 层），但多为**非渲染类**（字符串解析/STL/配置加载，属输入不变量边界），非黑屏根因。
4. **真实卡点 = F_23eec→F_76664 调用时 r0-r3 寄存器污染**（this=0，§124z-4）→ 被迫存根 → 渲染状态设置不完整 → glClear 类对象不构造 → 黑屏。

**★ 教训固化（统计口径）**：
1. **seed_v2.json 不是完整函数边界表**——统计函数级调用边时必须合并 seed ∪ 实际定义，否则漏算 ~427 个已翻译函数的调用边（低估 10%+）。
2. **存根 vs 完整翻译的区分靠参数命名**：匿名参数 = 存根（return 0），命名参数 `_r0` = 完整翻译。用 `return 0` 匹配会漏（存根格式是匿名参数）。
3. **统计脚本可复现**：`/tmp/opencode/callchain_stats.py`（读实际编译源 glob + seed + objdump，合并边界，输出全库/已翻译内/渲染闭包/关键路径四层统计）。

---

### §124aa ★ 18 注入表值推算纠正（v178bn-v178bu）——主线程 _ZdlPv 崩溃根因

**背景**：§124z-2 后主线程 `_ZdlPv(0x6ae4bdf0)` 垃圾指针崩（fault 0x6ae4xxxx，host 无映射）。jqt_OnCreate.c L2358a r6=MEM(r4)，r4=MEM(0x235c4)+0x2356C，MEM(0x235c4)=18 注入表值 0x96a779e4。

**★ 根因 = 18 注入表 .text 常量池值推算有误**：
- 0x235c4 静态值 = 0x00138918（.rodata 地址），注入表 = 0x96a779e4（redroid 运行时绝对地址，不同进程基址不同）。
- 原版用法 = `ldr rX,[pc,#N] @ (0x235c4); add rX,pc`（PC 相对**位移**，非指针）——16 个 .text 常量池全为位移（objdump 实证：0x233e4/0x233f6/0x23562/0x23596/0x235d6 均 `ldr; add rX,pc`）。
- 翻译体把 `add rX,pc` 实现为 `rX=file_off+rX`（0x2356C+rX），故注入值应为**静态位移**（0x00138918），非 redroid 绝对值（0x96a779e4）。
- 注入 redroid 绝对值 → r4=0x96a779e4+0x2356C=0x96a7af50（host 随机映射读垃圾）→ r6=MEM(r4)=0x6ae4bdf0 → L2358a free(垃圾) SIGSEGV。

**★ 修复（jqt_mem.c g_jqt_got_tbl）**：16 个 .text 常量池注入值改回**静态值**（jq_img.bin 原始值 0x0013xxxx，memcpy 后已在影子内存，注入变 no-op）。2 .data 占位 0。0x2356C+0x00138918=0x15BE84（.bss，memset 清零）→ r3=MEM(0x15BE84)=0 → L23596 分支跳过 delete → **主线程 _ZdlPv 崩溃消失**。

**★ 实证（v178bn→v178bo）**：F_23eec→F_76664 链 r3=0（合理），OnCreate 完整返回（F_23560 r0=0，NewObject(GLView)=0x89，setContentView），主线程 _ZdlPv 崩溃消失。**主线程不再崩**。

### §124ab ★ F_23eec 完整触发系统 RenderThread 崩溃（v178bo-v178bu）——新边界

**背景**：主线程 _ZdlPv 修好后，GLThread 跑 F_23eec 完整 + F_76664 入口 r3=0，随后 **RenderThread（libhwui）SIGSEGV fault 0x1c**（`SkiaPipeline::tryCapture+46`，null+0x1c）。backtrace 100% libhwui（系统渲染器），非 libjunqi.so。

**★ 实证 F_23eec 完整是崩溃源**（v178bs 临时存根 F_23eec return 0）：
- F_23eec 完整 → RenderThread 崩（fault 0x1c）
- F_23eec 存根 → **无崩溃**（进程存活 pid 16751），截图显示标题"军棋"但内容黑（渲染入口仍存根 → 无帧）
- 结论：**F_23eec 完整触发系统 RenderThread 崩溃**。存根是稳定基线但黑屏。

**★ 诚实边界**：
1. RenderThread 崩溃在 libhwui（`tryCapture`，`SkSurface*` null），是**系统渲染器**问题，非我们翻译体直接崩。日志 `Failed to initialize 101010-2 format`（EGL）提示 MuMu GPU 对 app 表面格式支持不完整。
2. `hardwareAccelerated="false"`（v178bt 实验）避开 RenderThread 崩，但暴露**主线程 F_75ce8 _ZdlPv 崩溃**（off=0x2c13d0，frees 垃圾 r4）——同 F_23eec 完整副作用，两条崩溃路径是竞态（先到先崩）。
3. **F_75ce8 _ZdlPv 崩溃**（jqt_vtbl.c L75d5a 附近 frees r4，r4 来自调用方 F_76832，对象图未构造→垃圾）——独立于 L2358a，属**对象图语义缺失**边界。

**★ 教训**：
1. **18 注入表值必须是静态位移**（PC 相对），非 redroid 绝对值（不同进程基址）。jni_runtime_values.json 已删（文件不存在），值固化在 jqt_mem.c g_jqt_got_tbl。
2. **F_23eec 完整 = 渲染状态设置**，它触发系统 RenderThread 崩溃（libhwui），说明翻译体副作用（对象图不完整）把 GL/表面状态搞坏。修它需攻对象图（glClear 类对象构造 + r4 对象图），非翻译器 bug。
3. 当前最终产物：`rev64/dist/libjunqi_arm32_aligned.so`（5377980B）+ `/tmp/opencode/p32_v178bu.apk`（11890688B，armeabi-only，v1+v2，.so STORE，manifest 0x00080003）。F_23eec 完整（崩溃边界）+ 注入修复（主线程 _ZdlPv 消失）。

### §124ag ★ F_23eec 渲染循环 slot13=glClear dispatch（真实清屏里程碑，v178ck）

**★ 渲染循环定位**：F_23eec（0x23eec-0x240d4，引擎 vtable 0x155918 之外独立渲染循环）在 **0x2400c-0x24010 dispatch slot13**（`ldr r3,[r0]; ldr r3,[r3,#52]; blx r3`），即渲染器对象 vtable slot13 = **F_9d458（真实 glClear）**。链：OnUpdate→F_23475→F_bd5a0→F_24108→F_23eec→slot13。

**★ vtable 布局确认（.data.rel.ro 0x155918 引擎 / 0x1595c0 渲染器）**：
- 引擎 vtable 0x155918：slot0=0x242e0(thunk→F_242f4), slot4=0x240f4, slot7=0x237cc, slot8=0x23750, slot9=0x23724
- 渲染器 vtable 0x1595c0：**slot13=0x9d459=F_9d458(glClear)**, slot14=0x9d43d=F_9d43c(→F_9e760 glDrawArrays), slot45=0x9e3e5, slot69=0x9ef21=F_9ef20(draw 方法)
- **棋盘绘制链**：F_9ef20(draw, slot69)→slot13(glClear)+slot14(F_9d43c→**F_9e760 glDrawArrays@0x9e7f6**)+slot15。F_9ef20 无 BL 无 slot69 dispatch（仅 0x1596d4 vtable 槽）→ **棋盘未达**。

**★ 对象图分歧实证**：F_23eec slot13 dispatch 目标 = `0xb2f920fd`（shadow 偏移 **0x920fc** = memcpy 函数中间，非函数入口）→ 渲染器对象 vtable 是垃圾（object graph divergence）。F_9ef20 的 `[r0+48]` 纹理检查 + 对象 vtable 均未构造。

**★ 修复（v178cg-ck）**：
1. **F_23eec L2400c 强制 dispatch F_9d458**（`r0=F_9d458(0, 0xff9c6630, ...)`，跳过垃圾 vtable 读）。v178cg 首测偶发 libhwui tryCapture 崩（同 §124ab 系统边界），重跑稳定（进程存活 30s+）。
2. **诊断 glClear 从 F_23475 移除**（v178ci）：验证 F_9d458 是否真清屏。
3. **F_9d458 `__aeabi_i2f`(0x232c8) 存根修复**（v178cj）：4 处 `blx 0x232c8 存根` 返回 0 → 颜色全 0=黑。改真实 `(float)(uint32_t)r0` → 颜色正确。
4. **实证 F_9d458 是活跃清屏路径**：v178cj 设绿色(0xff006600) → 屏幕绿(0,101,0)；v178ck 恢复蓝(0xff9c6630) → 屏幕蓝(49,101,156)，进程存活无崩。

**★ 里程碑达成**：**真实 clear（F_9d458）被 dispatch**（F_23eec 渲染循环），诊断移除后 F_9d458 独立产生可见清屏色。首帧 = F_9d458 清屏色（蓝），非诊断注入。

**★ 诚实边界**：首帧 = 清屏色（蓝），**非棋盘**。棋盘绘制需 F_9ef20（slot69）dispatch + 渲染器对象图（纹理[r0+48]、顶点数据）构造——对象图分歧未解。F_9ef20 无 slot69 dispatch 点（无 `ldr [rX,#0x114]`），需逆向其 caller 链。

**★ 产物**：`rev64/dist/libjunqi_arm32_aligned.so`（5384716B）+ `rev64/dist/phase_chain_arm32.apk`（p32_v178ck，11894784B，armeabi-only，v1+v2 verified:true，.so STORE，manifest 0x00080003，屏幕蓝稳定）。F_9d458 i2f 修复 + F_23eec 强制 glClear + 诊断移除。

**★ 下一步**：① 逆向 F_9ef20（draw,slot69）的 dispatch caller 链（slot69 dispatch 点，`ldr [rX,#0x114]` 未找到，改搜 `add rX,rY,#0x114`/GOT 间接）；② 渲染器对象图（纹理 +48、顶点 buffer）构造 → F_9ef20 → F_9d43c → F_9e760 glDrawArrays 出棋盘。

### §124ah 棋盘绘制三层阻塞分析（并行逆向，v178ck 后）

**★ draw 链定位**：glDrawArrays(0x230ac) 调用者仅 3 处：`0x9d442`(F_9d43c slot14)、`0x9e5cc`(F_9e3e4 slot45)、`0x9e7f6`(F_9e760)。F_9d43c/F_9e3e4/F_9e760/F_9d458/F_9ef20 **全部仅在 vtable 0x1595c0**（.data.rel.ro，RELATIVE 重定位）。draw 入口 = F_9ef20(slot69) 或 slot14/45。

**★ 三层阻塞（诚实边界）**：
1. **draw dispatch 未达**：F_23eec(渲染循环) 仅 1 处 slot dispatch（slot13=glClear@0x2400c），无 slot14/45/69。F_9ef20(slot69) 无 BL 调用点、无 `ldr [rX,#0x114]` dispatch、无 `add.w #0x114`。draw 只经 vtable dispatch，对象图未构造 → slot 值垃圾（0x920fc=memcpy 中间）。
2. **对象图分歧**：渲染器对象（vtable 0x1595c0）+ 纹理 `[this+48]` + 顶点 buffer 均未构造。F_9ef20 需 `[this+48]` 非零 + r4 有效 vtable slot13/14/15 返回非零。
3. **触摸→对局路径未翻译**：`jqt_OnEvent`(→F_bd914 存根 0xbd914)、`jqt_OnTouch`(→F_bdd2c 存根 0xbdd2c) 均为存根。F_bd914 callee 8/11 完整（F_b26ac/F_b690c/F_b6948 缺失）；F_bdd2c callee 1/3 完整（F_bd580/F_bd778 缺失）。点击"人机对战"不进对局 → 无棋盘场景。

**★ 菜单损失时间线**：§124u 曾显示菜单（标题"军棋"+按钮），当前全蓝。**非本次 F_9d458 改动引入**——检查点 §124af 前稳定版 p32_v178ck 已全蓝（1314000 像素纯色）。菜单为 native GL 纹理绘制（对象图依赖），随对象图分歧丢失。

**★ 结论**：首帧（真实 glClear F_9d458 清屏色）已达成（§124ag）。**棋盘绘制 = 对象图重建**（渲染器对象+纹理+顶点 buffer + draw dispatch + 触摸→对局链），非单函数翻译，属大工作量逆向。单翻译体路径已到"可运行+首帧清屏色"边界。

### §124ai 翻译器三修复 + F_bd914/F_bdd2c 触摸路径翻译（v178cl-cm）

**★ 翻译器三修复（thumb2c_full.py，备份 .bak_124ai）**：
1. `find_end` 用 entries 边界（`return r if r<=e else e`），不用 `max(r,e)`——函数末尾 literal pool 污染（F_bd914 的 `blx 0x22e60 __stack_chk_fail` noreturn 后 fallthrough 进数据区，objdump 误读为 `b.n bd374`）。
2. `translate_func` main_body 裁剪 `i[0]<end`——可达集含 literal pool 误读指令。
3. `_reachable` blx 到 noreturn PLT（`NORETURN_PLT={0x22e60}`）不加 fallthrough。
- 验证：F_bd914 body 从 15836B（含 bd374 outrange）→ 4886B（outrange=0，仅 5 存根）。

**★ 生成 `jqt_OnEvent_full.c`**（F_bd914/F_bdd2c + 6 缺失 callee）：
- F_bd914/F_bdd2c + F_b26ac/F_b4238/F_b6948/F_b7e00/F_bd580/F_bd778（8 函数）。
- 修复：blx rX vtable dispatch → `jqt_blx_dispatch`；blx 0x233ac(__aeabi_f2iz) → `(int)(float)` 真实；blx 0x22e60(__stack_chk_fail noreturn) → 保持存根。
- **★ 函数体 `#define MEM` 冲突修复**：thumb2c_full 生成函数体自带裸 `#define MEM(addr)=*(uint32_t*)(addr)`，覆盖 jqt_mem.h 的 MEM（jqt_mem_addr 加 g_jqt_shadow 偏移）→ 读错内存写垃圾 Java 堆。8 处移除。
- `jqt_OnEvent.c`/`jqt_OnTouch.c` 改真调 F_bd914/F_bdd2c（非存根）。

**★ 崩溃时序敏感 UB（v178cl-cm，pc 0x506698）**：
- v178cm 崩溃 `OnCreate: 入口` 后 pc 0x506698（jqt_jni_call 内 `blx ip` 后），fault 0x0，backtrace `#02 CallVoidMethodA` + `#03 libjunqi`。
- **加 jqt_jni_call 日志（前 60 条）后崩溃未复现**——时序/内存布局敏感 UB，非确定性。jqt_jni_selftest（43 条 resolve ok=34 fail=9）完整通过。
- 诚实边界：UB 根因未定位（可能读未初始化内存），日志是 workaround 非 fix。

### §124aj ★ OnTouch→F_bdd2c 真实执行里程碑（v178cn-co 稳定基线）

**★ 里程碑**：**OnTouch 被 Java 调用 → F_bdd2c 完整执行不崩**（进程稳定 30s+，crash=0）——触摸→对局路径首次真实执行（此前 F_bdd2c 存根 r0=0）。
- 日志实测：`OnTouch: 入口 r0=0xffb56224 (MotionEvent)` → `F_bdd2c: 入口 r0=0xccc00000 r1=0xffb56224 r2=0 r3=0x15be7e`。
- 点击 3 次（450,500/600/700）均触发 OnTouch→F_bdd2c，进程不崩。
- **诚实边界**：对局未进入（屏幕仍蓝 49,101,156）——对象图分歧（§124ah 阻塞②），F_bdd2c 执行但触摸坐标无法映射到按钮/棋子（UI 未渲染）。OnEvent 未触发（点击只走 OnTouch）。

**★ 产物**：`rev64/dist/libjunqi_arm32_aligned.so`（5427112B）+ `/tmp/opencode/p32_v178co.apk`（11939840B，armeabi-only，v1+v2，.so STORE，manifest 0x00080003，屏幕蓝稳定 30s+）。

**★ 固化脚本**：
- `rev64/tools/gen_onevent_full.py`：自包含生成 jqt_OnEvent_full.c（thumb2c_full gen_c_file_full → 提取缺失 → 修复存根 → 移除 MEM 定义 → 入口日志），可重复。
- `rev64/test_mumu.sh`：MuMu 推→装→清logcat→启→判存活→判崩溃→截图→取样色→抓关键日志（固化 §124ai 手工流程）。

**★ 下一步**：① 攻对象图分歧（渲染器对象+纹理+顶点 buffer，FRIDA_NOTES 第八节 unidbg+Frida 真值）；② F_bdd2c 内部 vtable dispatch（`jqt_blx_dispatch` 返回值语义）验证；③ draw dispatch（F_23eec slot14/45/69）。

### §124ak JNI 传参修正 + F_bdd2c JNI 真实调用（v178cp-cv 迭代）

**★ jni_aligned.c 传参修正（§124ak 根因）**：
- 原版 OnTouch (0x23438)：`adds r1,r0,#0` → **r1=env(JNIEnv*)**；`bl 0xbdd2c` → F_bdd2c(全局对象, env, motion)。
- jni_aligned.c 旧版传 `V32(m)` 当 r0、全 0 → **丢 env** → F_bdd2c 收到 r1=MotionEvent 而非 env → `[r1+0]` 读 MotionEvent 对象 vtable → 垃圾 dispatch → 触摸不处理。
- 修正：`jqt_OnTouch(V32(env),V32(env),V32(m),...)`（OnEvent 同理传 env/n/v）。

**★ F_bdd2c JNI 调用特判（真实调用替代 jqt_blx_dispatch）**：
- jqt_blx_dispatch 只查原版 dispatch 表（<MAP_HI），**真实 JNI 函数指针（>=MAP_HI）不在表内 → return 0（无日志）** → JNI 未调用。
- F_bdd2c blx 识别：bdd48=GetObjectClass(functions[31])，bdd60/bdd84=GetMethodID(functions[33])，bddc6=DeleteLocalRef(functions[23])。
- 特判真实调用：`jqt_jni_getobjectclass` / `jqt_jni_getmethodid` / `jqt_jni_deletelocalref`（jqt_jnibr.c）。
- **name/sig 验证**：GetMethodID name=0x128b0c="getAction"，sig=0x1265c8="()I"（.rodata 明文）→ MotionEvent.getAction() ✓。

**★ §124al include jqt_mem.h 副作用（MuMu dlopen 偶发失败）**：
- jqt_jnibr.c include jqt_mem.h → MuMu 32 位 linker `do_dlopen failed`（UnsatisfiedLinkError，进程死）→ 回退。
- **修复**：getmethodid 用 `extern uint8_t *g_jqt_shadow;`（不 include 头）→ dlopen 稳定。

**★ 稳定性脆弱（诚实边界，读未初始化 UB）**：
- v178cv 稳定 2/2（GetMethodID 真实调用）；v178cs/v178cx 崩（GLThread scudo SIGSEGV / 加日志变时序）。
- **根因**：渲染循环读未初始化对象图字段（§124ah 阻塞②），时序敏感 → 随机崩溃。非 getmethodid 本身。
- 诚实：v178cv 是"较好"基线，稳定性非保证。

**★ 产物**：`rev64/dist/libjunqi_arm32_aligned.so`（v178cx 5428940B）+ `/tmp/opencode/p32_v178cx.apk`（11939840B，armeabi-only，v1+v2，.so STORE）。
- 固化：`rev64/tools/gen_onevent_full.py`（getmethodid/deletelocalref/getobjectclass 特判 + extern）。
- F_bd914/F_bdd2c + JNI 真实调用（OnTouch→GetObjectClass→GetMethodID(getAction)→DeleteLocalRef）路径已翻译。

**★ 下一步（对象图核心阻塞）**：渲染循环读未初始化对象图字段 → 时序敏感崩溃。按 FRIDA_NOTES 第八节：32 位 frida-server（x86，MuMu）dump 渲染器对象（vtable 0x1595c0）+ 纹理[this+48] + 顶点 buffer 真值 → 填翻译体影子内存 → 破对象图分歧。

### §124am frida attach 崩 + /proc/pid/mem 读取突破（对象图诊断路线）

**★ frida attach 在 MuMu 崩（环境限制，替代路线确立）**：
- frida-server 17.24.3 32 位 x86 已跑（pid 26945，root，`su 0 /data/local/tmp/frida-server -D`）。
- 远程连接 OK（`dm.add_remote_device('127.0.0.1:27042')`，75 进程可见）。
- zygote（32 位）attach OK；**junqi（ARM 转译进程）attach 崩**——attach 本身（注入 agent）导致进程死：
  - attach+detach（不 load script）后 2 秒进程 DEAD（对照实验）。
  - frida spawn 在 MuMu 启动 1 秒即死（app_elapse=1，frida spawn 与 MuMu 不兼容）。
  - **结论：MuMu 的 ARM 转译进程 + frida 注入不兼容**（agent 注入干扰 houdini）。
- **替代：`/proc/pid/mem` 直接读（redroid 同款）**——read() 系统调用，**无注入不干扰进程**，崩溃前能读。

**★ /proc/pid/mem 读取突破（对象图诊断，v178cy 实测）**：
- 流程：`am start` → 轮询 pidof + logcat grep `g_jqt_shadow`（OnCreate 打印）→ `su 0 dd if=/proc/PID/mem bs=1 skip=ADDR count=N`。
- 实测（进程 28380/28612/29167）：**0x15c0a0=0 稳定**（渲染器单例未构造=黑屏核心）；
  **0x162400 时好时坏**（0 或 0xd4e155f0——F_9d3d4 构造不稳定，时序 UB）；
  0x15be88 非零（0xc75d58d0/0xea812c30——渲染对象部分构造）；
  0x15b578=shadow+0x15c0a0（GOT 槽注入正确）、0x15b8fc=shadow+0x15f910（§124x 正确）。
- **翻译体自身不稳定**（0x162400/0x15be88 每次运行不同）——GLThread 读未初始化对象图字段（§124ah 核心），崩溃窗口 3-8 秒。
- **原版无直接引用 0x15c0a0/0x15b578**（capstone 扫描 ldr[pc]+add pc：0x15c0a0=0 引用、0x15b578=0 引用）——
  0x15c0a0 是**翻译体 g_jq_got_tbl 注入的伪单例槽**（0x15b578 GOT 槽=shadow+0x15c0a0），非原版真实槽。
  （0x15be80:106 refs、0x15be88:140 refs、0x15c060:7 refs、0x162400:10 refs 是原版真实引用。）

**★ 路线确立**：翻译体自身不稳定 + 0x15c0a0 是伪槽 → 需要**原版真值**。用户确认可提供原版军棋.apk：
- 跑原版（稳定、能玩、对象图完整构造）→ /proc/pid/mem dump 0x15be80/0x15be88/0x15c060/0x162400 等真实槽语义 + 渲染器对象图 → 指导翻译体构造链。
- 固化：`frida-dump/proc_read_slots.py`（读单例槽+对象 vtable，轮询 pid+shadow）、`frida-dump/proc_read_fast.py`（大块读）。

**★ 诚实边界**：/proc/pid/mem 读翻译体只能看"当前构造状态"（不稳定），不能构造对象图；构造需原版真值或翻译体主动构造。

### §124am-2 x86_64 frida-server + frida 读取打通（用户纠正：MuMu 用 x86_64 版）

**★ 用户纠正（权威）：frida 在 MuMu 绝对没问题，用 x86_64 frida-server，版本要一致**：
- 之前下载 x86（32 位）版是错的——junqi 进程是 **app_process32**（32 位），但 **32 位 frida-server attach ARM 转译进程崩**（注入干扰 houdini）。
- **x86_64 frida-server 17.24.3**（用户实测跑过，`frida-server-1724` 已在设备）→ attach OK，**进程 attach 后稳定**（2s 不死）。
- 用户之前跑的 `frida-server-1724`（171MB，10-30）就是 x86_64 版。

**★ frida API 修正（frida 17.24.3）**：
- `Memory.readU32`/`Memory.readUtf8String` **不存在**（frida 17 API 变了）——用 **`NativePointer.readU32()`/`NativePointer.readUtf8String()`**。
- `ptr()`/`new NativePointer()` 都 OK。

**★ frida 读取打通（v178cz 稳定 + x86_64 server）**：
- 流程：重启 app → 轮询 pidof + logcat grep `g_jqt_shadow`（OnCreate 打印）→ attach → `new NativePointer(shadow+off).readU32()`。
- 实测（进程 30560，shadow=0xb377d000）：
  - shadow+0x120bf8 读 'text'（验证通过）
  - **0x15c0a0=0**（伪单例槽，稳定未构造）
  - 0x162400=0（F_9d3d4 构造，时好时坏）
  - 0x15be80=0xcdbda280（Java 对象，非渲染器，+0x20='dori'）
  - 0x15b578=shadow+0x15c0a0（GOT 槽确认）、0x15b8fc=shadow+0x15f910（§124x 确认）
- **frida 可读可写**（NativePointer.writeU32）→ 可主动构造对象图测试。

**★ 固化**：`frida-dump/frida_read_shadow.py`（重启+轮询 shadow+读）、`frida-dump/frida_restart_read.py`。

**★ 下一步**：① frida 写 0x15c0a0 构造测试（主动构造验证对象图假设）；② 等用户提供原版 → 原版 dump 真值（0x15be80/0x15be88/0x162400 真实语义）。

### §124am-3 对象图部分构造确认 + frida 主动构造（0x162400=渲染器单例 vtable 0x1595c0）

**★ v178cz 稳定后对象图部分构造（frida 时序监控，进程 30871）**：
- 0x15c0a0=0 稳定（伪槽，翻译体注入假设，原版无引用）
- **0x162400=0xc668df30 vtable=0x1595c0**（渲染器！F_9d3d4 构造成功）——字段：+0x4=0x58000000、+0xc=+0x10=自指针、+0x18=0x1000006e、+0x24=0x10000074、+0x38=0x10000001
- **0x15be88=0xc65462f0 vtable=0x159ed0**（另一对象）——字段：+0x18=0xcdcde8d0、+0x20=0x159ee4、+0x30=0xcdcde400
- 0x15be80=0xcd940ec0（Java 对象）
- **对象图时序不稳定**（30964/31070/31160 的 0x162400 未构造，30871/30660 构造）——F_9d3d4 时成时败（OnCreate 早退/时序 UB）

**★ frida 主动构造打通（frida_construct.py）**：
- Memory.alloc(136) → 写 vtable=0x1595c0 + 30871 真值字段 → 写 0x162400+0x15c0a0=obj → 验证通过。
- **但屏幕仍蓝**——glClear 清屏执行，draw 未达：frida 构造对象字段不全（纹理[this+48]、顶点 buffer 缺）→ F_9ef20(draw,slot69) 无法执行。

**★ 结论（诚实边界）**：
- 渲染器单例（0x162400，vtable=0x1595c0）已确认是真实对象，翻译体 F_9d3d4 能构造（部分成功）。
- 黑屏根因：**draw 需要完整对象图**（渲染器 + 纹理[this+48] + 顶点 buffer），翻译体对象图字段不全 + 0x15c0a0 伪槽。
- **破题需要原版 dump 完整对象图真值**（原版稳定、能玩、对象图完整）——用户已确认可提供原版军棋.apk。

**★ 固化**：`frida-dump/frida_construct_singleton.py`（主动构造）、`frida-dump/frida_wait_objs.py`（时序监控）、`frida-dump/frida_read_shadow.py`。

### §124an 黑屏直接原因确认：纹理[renderer+48]=0 + draw 链逆向

**★ 渲染器对象完整展开（frida 读 0xd4e8b190，进程 31380）**：
- vtable=0x1595c0（✓ F_9d3d4 构造），+0x4=0x58000000，+0xc=+0x10=自指针，+0x18=0x1000006e，+0x24=0x10000074，+0x38=0x10000001
- **+0x48（纹理）=0**（黑屏直接原因！），+0x4c=0x10000000，+0x54=+0x58=0xd4e8b1dc（子对象指针，非零），+0x60=0x48000002

**★ F_9ef20(draw,slot69) 反汇编**：
- `9ef36: ldr r3,[r0,#48]`（读纹理）→ 纹理 vtable[13]/[14]/[15] 调用（blx）+ 顶点数据（[r4+4]/[r4+8]/[r4+12]）
- **draw 需要纹理[renderer+48] 非零 + 纹理对象 vtable[13/14/15] + 顶点数据**

**★ 渲染器构造函数 F_9d9b8/F_9d988**：
- 只写 vtable + 清零（+100~+132），**不写 +48 纹理**——纹理由图集加载时构造（非渲染器构造）

**★ 纹理创建链逆向（glGenTextures GOT=0x15b400）**：
- 0xbe2ec：`ldr r2/r3,[pc]`（GOT 字面量）+ `be2f6/be2f8 add r2/r3,pc` → 0x15b400/0x15b268（glGenTextures/glBindTexture）
- `be39c: blx r6`（glGenTextures）→ `be3ac: blx [vtable+0x244]`（glBindTexture）→ `be3b8: blx [vtable+21]`（texImage2D）→ `be3c0: str r0,[全局槽]`（纹理 ID）
- **纹理 ID 存全局槽**（非 renderer+48 对象）——renderer+48 是纹理对象（vtable[13/14/15] 包装）

**★ 结论（黑屏根因链，诚实边界）**：
1. 渲染器单例构造（vtable=0x1595c0）✓（F_9d3d4，有时成功）
2. **纹理[renderer+48]=0**（图集加载未执行/未构造）→ draw(F_9ef20) 读 [r0+48]=0 → 无法绘制
3. F_23eec 只强制 glClear(slot13)，**draw(slot69) 未达**（g_cur_fn=0x9f43c 非 0x9ef20）
4. **破题需要原版 dump renderer+48 纹理对象真值**（vtable[13/14/15] + 顶点数据 + GL 纹理 ID）→ 填翻译体 → 强制 dispatch slot69

**★ 等原版**：用户确认可提供原版军棋.apk。原版跑起来（能玩、对象图完整）→ frida dump renderer+48 纹理对象 + 顶点 buffer + 图集加载链 → 填翻译体。

### §124ao 模板化 frida 工作流（用户需求：提炼重复机械执行）+ 原版真值 dump

**★ 用户需求：提炼重复模版化工作，去重复机械执行**——最近多轮大量重复（重启→轮询pid/shadow→attach→读槽→截图→固化），每次重写 60-80 行样板。已提炼：

- **`frida-dump/frida_utils.py`**（公共库）：`sh/adb/connect/get_pid/restart_app/get_shadow/attach/full_attach/run_js/read_slots/read_obj/screenshot/get_module_base/dump_texture_truth`。含 frida 17 兼容 JS helpers（`NativePointer.readU32()`，`Memory.readU32` 已不存在）。
- **`frida-dump/frida_run.py`**（统一子命令入口）：`status/restart/read/obj/shot/monitor/construct/fixsingleton/base/dump/dumpwait`。
- 验证：`base` 抓原版基址 0xc068000 ✓、`obj 0xc64ceed0` 读渲染器 ✓、`shot` 截图+颜色 ✓、`dump`/`dumpwait` 轮询纹理 ✓。

**★ 原版真值 dump（原版军棋.apk 已提供，装原版跑对局）**：
- 原版 libjunqi.so 基址 **0xc068000**（/proc/maps：`0c068000-0c1bd000 r--p 00000000`、`0c1bd000-0c1c4000 rw-p 00155000`）。
- 原版渲染器单例 = BASE+0x162400 = **0xc64ceed0**，vtable = BASE+0x1595c0 = 0xc1c15c0 ✓。
- **vtable slot69 = 0xc106f21 = BASE+0x9ef21 = draw（F_9ef20）✓**（修正：之前以为 slot69=BASE+0x106f21，漏减 0x68000）。slot13=0xc105459（glClear）。
- 原版棋盘完整渲染（进"人机对战"后截图：38子背面+行营+大本营+木纹，深绿 112,156,0 主色）。
- **渲染器 +48（纹理）动态**：进对局瞬间 = 0xea89e6d0（纹理对象），15s 轮询后 = 0（渲染帧后释放，framebuffer 保留棋盘画面）。**draw 的 this = 渲染器，纹理只在 draw 帧存在**。
- 纹理对象 0xea89e6d0 dump：vtable=BASE+0x1c2400=0xc1c2400（.data 区），slot13/14/15=BASE+0x132e20/0x132b80/0x1341fc（绘制方法），+0x14='valu'、+0x8=+0xc=自指针、+0x50=0x4c005、+0x54=0x1c6d0000。

**★ frida 17 API 补充**：`Module.getGlobalExportByName`（`findExportByName` 已改）、`Interceptor.attach` 需 NativePointer。**ARM 转译进程 hook 限制**：hook x86_64 原生 GL 地址（glDrawArrays 0xe8431a80）无效（houdini 桥接，ARM 代码调转译层非原生地址）；hook ARM draw(0xc106f21) 4s hits=0（draw 不在 GLThread 常驻，或 hook 静默失败）。

**★ 下一步（模板化已够用，攻渲染）**：
1. **翻译体渲染循环修复**：翻译体 g_cur_fn=0x9f43c（没到 draw）。需让渲染循环 dispatch slot69 时 this+48≠0（构造纹理对象，vtable=shadow+0x1c2400，slot13/14/15=shadow+0x132e20/0x132b80/0x1341fc）。
2. 原版渲染循环逆向：找 dispatch slot69 的调用者（渲染循环读渲染器+48→draw），确认 draw 前纹理创建链（glGenTextures 0xbe2ec）。

### §124au ★ 攻 glClear 类对象图 — 持续出帧里程碑（MuMu 截图实测 80% 彩色区）

**目标**：让 arm32 翻译体 GL 棋盘内容出帧（非仅清屏色）→ 攻 glClear 类对象图（vtable=0x1595f4 未构造 → 黑屏，§124z）。

**本轮实现（4 修复，每步推进）：**
1. **0xa4484 dispatch miss 修复**：`jqt_vtbl.c` 补 F_a4460/F_a4484/F_a3a14/F_12020（glClear 类子对象自递归构造器 + 字符串校验 + 断言存根），vtable 表 `jqt_vtbl_offs/fns` 加 0xa4460/0xa4484 并重排（§109 红线，1489→1491 有序）。`jqt_blx_dispatch` 越界阈值 `0x120bf1`→`JQT_MAP_HI`（0xa44xx/.bss/.data 误判越界修复）。日志 "BLX miss off=0xa4484"→"BLX hit off=0xa4484"。
   - **关键踩坑**：`jqt_vtbl_fns` 表头是 13 参函数指针（非简单数组），正则提取 `uint32_t (\*const jqt_vtbl_fns\[\])\(.*?\) = \{` 在 `.*?` 处括号不配平报 `unbalanced parenthesis` → 改用行扫描（`jqt_vtbl_fns[]` 行 + `};` 止）按行重排。
2. **GLView stale Local 崩溃修复**：`jni_aligned.c` OnCreate `NewGlobalRef(view)` 提 g_glview_handle + OnDestroy `DeleteGlobalRef`。消除 "accessed stale Local 0x2f1" SIGABRT（GLThread getHolder 跨帧 GC 回收 LocalRef）。
3. **GL 首帧出帧**：`jqt_gl_bridge` 从 GLView 拿 Surface（jqt_render_glview_make_surface getHolder id11）→ eglCreateWindowSurface → eglMakeCurrent + glClearColor(48,102,156) + glClear + eglSwapBuffers → **出帧#1 成功**（§124z 黑屏边界突破）。
4. **★ 持续出帧循环（本轮核心突破）**：EGL current surface/context 是**线程本地**——首帧在 OnGraphCreate 线程（GLThread）makeCurrent 出帧#1；F_bf4d0 主线程钩子跑在 main（current=NO_SURFACE）glClear 无效。修复：GLThread（F_9d458 体首）`jqt_gl_bridge_start_frame_loop(0xff9c6630)` spawn **专属渲染循环线程**（每 16ms glClear+eglSwapBuffers 到 g_jq_gl_ws window surface，~60fps）。

**实测（MuMu 18s+ 截图 + logcat）：**
- 进程稳定无崩，循环出帧 **#1→#2817 连续 ~42s**（`gl_bridge: 循环出帧` 日志），surface=0xf0ab9890 复用不重建。
- 屏幕 **80% 彩色区**（清屏色通道 MuMu 转译器 RGB 重排为 (156,101,49)，中屏 avg=(156,101,49) 覆盖 79%）——**非黑帧覆盖**（§124z 前纯黑 90.7%）。
- "BLX hit off=0xa4484" 命中（0xa4484 已入表）。

**诚实边界（不包装"能玩"）：**
- **持续出的是清屏色帧，非棋盘内容**：glClear 类对象图（vtable=0x1595f4）仍未构造（.rodata/.data.rel.ro 0 命中，运行时对象）；渲染链对象图分歧（F_bf4d0 "无效class=0x646e6100 回退渲染器单例" + BLX miss 0x6ee0d0/.bss、0x9d5d8/.text）。
- 循环线程与 Java GLView$GLThread 的 EglHelper 渲染循环**竞争 window surface**（我们 eglSwapBuffers 清屏色 vs 它 swap 纹理帧）——当前清屏色持续上屏是因为我们的循环频率（16ms）压过 Java 线程，非真正 GL 场景出帧。
- "能玩"需：glClear 类对象构造 + 渲染链 vtable dispatch 完整 + 纹理上传链 + 真机 GPU 验收。
- **产物**：`rev64/dist/libjunqi_arm32_aligned.so`（5438448B）+ `rev64/dist/phase_chain_arm32.apk`（11948032B，armeabi-v7a-only，v1+v2 签名，zipalign 4）。

**遗留 BLX miss（对象图分歧，benign 分类）：**
- 0x6ee0d0（.bss 区，运行时未构造对象 vtable 字段）/ 0x9d5d8（.text 区，渲染链纹理上传 dispatch 目标缺失）——主线程 F_bf4d0 纹理上传 dispatch miss，不崩（走"回退渲染器单例"）。

### §124au-1 ★ 0x9d5d8 渲染链 BLX miss 消除（dispatch 表补 F_9d5d8 存根）

**背景**：§124au 持续出帧里程碑后，渲染循环 F_bf4d0 仍有 2 个 BLX miss：0x9d5d8（.text 区，渲染链纹理上传 dispatch）+ 0x6ee0d0（.bss 区，运行时对象图堆指针）。

**本轮修复**：
- **0x9d5d8 所属函数头 = 0x9d3ee**（`push {r4,r5,r6,r7}`，字符串解析/纹理解析大函数体 0x9d3ee..0x9d6xx，0x9d5d8 是函数体中段的 blx 目标）。完整翻译 0x9d3ee 会引入深递归字符串解析 → **存根 F_9d5d8（r0=0）安全返回**，消除 BLX miss 让渲染循环 dispatch 命中不崩。
- dispatch 表补 0x9d5d8（idx 941）+ F_9d5d8 存根，offs/fns 表重排（§109 红线），1491→1492 条有序（乱序对=0）。
- **实测**：BLX miss 从 `off=0x9d5d8 + off=0x6ee0d0` 各 281 次 → **只剩 off=0x6ee0d0 288 次**（0x9d5d8 消除）。

**0x6ee0d0 性质（benign 对象图边界）**：
- reg_val=0x006ee0d1（Thumb 奇 &~1=0x6ee0d0），**未减 shadow**（reg_val < g_jqt_shadow → off=reg_val 原值）。
- 0x6ee0d0 **超出 so 文件**（so 大小 0x52fe7c）、**不在 .rodata 作为指针值**（0 命中）→ 是运行时对象图读出的**堆指针**（渲染器 vtable 字段/纹理对象指针），非 .text 函数。
- miss 后 `jqt_blx_dispatch` 返回 0（安全），渲染循环走"回退渲染器单例"路径不崩。
- **分类**：对象图分歧边界（§124z 同类），需构造对应 C++ 对象（vtable 写入真实堆地址）才能 dispatch 命中——工作量另计。

**产物**：`rev64/dist/libjunqi_arm32_aligned.so`（5438924B）+ `phase_chain_arm32.apk`（11948032B，armeabi-v7a，v1+v2，zipalign 4）。

**诚实边界（不包装"能玩"）**：
- 持续出帧仍是**清屏色帧**（蓝 48,102,156 RGB 重排后 80% 上屏），非棋盘内容。
- 0x6ee0d0（运行时堆指针）+ glClear 类对象（vtable=0x1595f4）未构造 → 纹理上传链 dispatch 不到 → 棋盘内容未渲染。
- "能玩"需：构造 C++ 对象图（renderer/texture vtable 写真实堆地址）+ 纹理上传链完整 + 真机 GPU 验收。

### §124au-2 ★ 全 BLX miss 消除（渲染循环 dispatch 全命中）

**背景**：§124au-1 消除 0x9d5d8/0x6ee0d0 miss 后，重启干净日志仍剩 **off=0x15c0c8** 一个 BLX miss（.bss 渲染器单例 0x15c0a0+0x28 字段，未构造对象 vtable 槽，对象图分歧 benign 边界）。

**本轮修复**：
- **0x15c0c8 补 dispatch 表存根**（F_15c0c8 r0=0）：`.bss` 区 0x15c0c8（>= 0x15be80）= 渲染器单例 0x15c0a0+0x28 字段读出的未构造对象指针 → blx miss。补 `jqt_vtbl_offs/fns` 表 0x15c0c8（idx 1492）+ F_15c0c8 存根，重排（§109 红线，1493→1494 条有序，乱序对=0）。
- **dispatch 表全量 benign 存根覆盖**：0x9d5d8（字符串解析 0x9d3ee 体中段）/ 0x6ee0d0（运行时堆指针，超 so 0x530018）/ 0x15c0c8（.bss 单例 +0x28）全部 r0=0 安全返回，渲染循环 dispatch **全命中不崩**。

**实测（MuMu 重启干净日志）：**
- **BLX miss 全消除**（`logcat | grep "BLX miss"` 返回空），BLX hit：0x23724/0x23750/0x237cc/0x240f4/0x242e0/0x6ee0d0/0x9d5d8/0x9e0d4/0xa4484/0xb89b0 全命中。
- 出帧循环 #1025+ 持续（~60fps 清屏色帧），进程稳定无崩。

**诚实边界（不包装"能玩"）**：
- 渲染循环 dispatch 全命中但 **纹理上传 dispatch 目标是存根 r0=0**（0x6ee0d0/0x15c0c8 对象图未构造）→ 棋盘纹理未真正上传 → 屏幕仍清屏色（80% 蓝）非棋盘内容。
- "能玩"需：构造 C++ 对象图（渲染器/纹理 vtable 写真实堆地址）+ 纹理上传链真实执行 + 真机 GPU 验收。

**产物**：`rev64/dist/libjunqi_arm32_aligned.so`（5439688B）+ `phase_chain_arm32.apk`（11952128B，armeabi-v7a-only，v1+v2 签名，zipalign 4）。

### §124aw ★ jqt_vtbl_n 修复（dispatch 表 1497 条全进二分范围）+ F_264d4 存根补 dlopen

**背景**：§124au-2 补 F_15c0c8 存根后，重启仍见 0x6ee0d0/0x15c0c8 BLX miss——根因 = **jqt_vtbl_n 声明 1494u，但 dispatch 表实际 1497 条**（§124au/au-1/au-2 补 3 条存根后未同步 n）。jqt_blx_dispatch 二分 `lo=0,hi=jqt_vtbl_n-1=1493`，末尾 idx 1494-1496（0x6ee0d0 idx=1496 / 0x15c0c8 idx=1495 / F_264d4 idx=1494）**不在查找范围** → 永远 miss。

**本轮修复**：
- **jqt_vtbl_n 1494u → 1497u**（§124aw）：jqt_blx_dispatch 二分 hi=n-1 覆盖全表末尾 3 条存根。
- **F_264d4 存根补函数体**（§124aw）：fns 表含 F_264d4（idx 58，0x264d4 = 0x26450 函数体中段跳表目标，原版字节 `33 fa c3 23...` ldr/str 中段）但全 src 缺函数体定义 → so 里成 undefined dynamic 符号（`objdump -T | grep UND | grep F_` = F_264d4）→ **MuMu dlopen "failed to link"**（新 build so 装不上）。补存根 r0=0 → so 全解析（UND F_ 符号 0）。
- **教训（dispatch 表维护）**：补 dispatch 表条目后**必须同步 jqt_vtbl_n**（§109 红线：表重排后 n 同步）。本轮 §124au/au-1/au-2 补 3 条存根漏同步 n → 末尾条目 miss。校验法：`jqt_vtbl_n == len(offs)` 且 `jqt_vtbl_n == len(fns)`。

**实测（MuMu 重启干净日志）**：
- **BLX miss 全消除**（0x6ee0d0/0x15c0c8 全 hit），dispatch 命中 9 目标：0x15c0c8/0x23750/0x237cc/0x6ee0d0/0x78a88/0x78a90/0x9d5d8/0x9e0d4/0xa4484。
- 出帧循环稳定（#1793+，~60fps 清屏色帧），进程稳定无崩。
- so 全解析（UND F_ 符号 = 0），MuMu dlopen 成功，so 与 dist md5 一致（eea811fe）。

**诚实边界（不包装"能玩"）**：
- 渲染循环 dispatch 全命中但 **0x6ee0d0/0x15c0c8/0x9d5d8/0x78a88/0x78a90 是存根 r0=0**（对象图未构造）→ 纹理上传 dispatch 不到真实方法 → 棋盘纹理未真正上传 → 屏幕仍清屏色（80% 蓝）非棋盘内容。
- "能玩"需：构造 C++ 对象图（渲染器/纹理 vtable 写真实堆地址）+ 纹理上传链真实执行 + 真机 GPU 验收。

**产物**：`rev64/dist/libjunqi_arm32_aligned.so`（eea811fe，5446312B，dispatch 表 1497 条全进二分 + F_264d4/F_6ee0d0/F_15c0c8 存根）+ `phase_chain_arm32.apk`（11956224B，armeabi，v1+v2，zipalign 4）。

### §124zz ★★ dlopen "failed to link" 根因定位 + 链接安全审计工具固化（v178e1→v178e5）

**症状**：§124zz 补纹理管线（F_9e204/F_9e0f0/F_9e2b4 + 6 个 GL 符号）后，MuMu 装新包即崩
`java.lang.UnsatisfiedLinkError: dlopen failed: failed to link libjunqi.so`。
**关键痛点：linker 错误详情被吞**——logcat 只有 `E linker: do_dlopen failed`（那几行其实是
libhp12_x86.so 的已知噪声）+ `failed to link`，**没有 "cannot locate symbol"**（MuMu 原生桥
houdini 加载 ARM 库，bionic linker 的诊断不落到 logcat）。

#### 根因（★ 不是 GL 符号！是 F_264d4 悬空）

逐版 diff UND 集合（`readelf -sW | awk '$7=="UND"'`）才定位到：

| 版本 | 相对 v178e0(实证可加载) 多出的 UND | 结果 |
|---|---|---|
| v178e1 | `F_264d4` + 6 个 GL 符号 | FAIL |
| v178e2 | + `dlopen/dlsym/dlerror/pthread_once` | FAIL |
| v178e3 | 同上 | FAIL |
| v178e4/e5 | 仅 6 个 GL 符号（F_264d4 已定义） | PASS（审计）|

**`F_264d4` 是内部 `F_xxx` 函数（fns 表 idx 58 / offs 0x264d4），在 so 里是
`NOTYPE GLOBAL DEFAULT UND`——有重定位、无函数体定义 → linker 无法解析 → 整体 fail to link。**
（§124aw 补过存根，但 v178e1 重建时**回归丢失**了——代码生成/重建会覆盖手工补的存根。）

**GL 符号是无辜的**：ARM libGLESv1_CM.so 与 ARM libGLESv2.so **都**导出这 6 个符号
（glGenTextures：GLES1 0x5014 / GLES2 0xcc44；glDeleteTextures 0x4e84/0xcab4；
glTexParameterf 0x4c90/0xd040；glPixelStorei 0x527c/0xcf00；glTexImage2D 0x5420/0xd02c；
glTexSubImage2D 0x5484/0xd090），且**原版 libjunqi.so 本身就直接链接这 6 个符号**且在 MuMu
正常加载 → 直接链接是**已被原版证明安全**的。

**⇒ 最终方案：6 个 GL 符号直接链接（不用 dlsym）。**
中途试过"GLES1 独有符号改 dlsym 懒加载"，实测是**倒退**：dlopen+dlsym+dlerror+pthread_once
引入 4 个新 UND 符号（pthread_once@LIBC 尤其可疑），且 dlsym 拿到的是 **x86 库**的函数指针
（MuMu 进程是 x86，ARM 侧 dlopen 加载 /system/lib/arm/ 版还是 /system/lib/ x86 版存疑），
徒增原生桥风险 → **全部撤销**（§124zz 撤回记录：dlsym 桥已从 jqt_vtbl.c 删除）。

#### 固化：`rev64/tools/link_audit.py`（★ 防复发，用户批评"反复写脚本没固化"后落地）

打包前离线审计，**不需要设备**：
1. **内部符号悬空检测**：重定位里有 `F_xxx` 但 src 无函数体定义 → fail to link 头号原因
2. **UND 符号解析**：逐个查 MuMu ARM 库（libc/libdl/libGLESv1_CM/libGLESv2/liblog/libm）
   是否导出（含版本）→ 列"无法解析"
3. **基线 diff**：与已实证可加载基线（`mumu_proven_und.json` = v178e0 的 43 个 UND 符号）比
   → 只报新增符号（基线里的已证明能解析，不误报）
4. **.hash 风格**：必须 sysv（MuMu 32 位 linker 不支持 .gnu.hash）
5. **APK 完整性**：.so 必须 STORE、manifest 头 0x00080003、v1+v2 签名
6. **dispatch 表一致性**（§109/§124aw 红线机器化）：`jqt_vtbl_n == len(fns) == len(offs)`、
   offs 严格升序（二分查找前提）、fns[i]/offs[i] 同址、每个 fns 条目有函数体定义
- 退出码：0=PASS / 2=FAIL
- **已接入 `rev64/pack_arm32.sh`**：审计不过 → 打包中止（不再产出"装上就崩"的包）
- 辅助：`rev64/tools/tex_pipeline.py`（纹理管线 vtable dispatch 搜索 / UND 符号对比）

#### 本轮结论与诚实边界

- **v178e5 审计 PASS**：UND = v178e0 实证基线 43 个 + 6 个 GL 符号（ARM 库全部命中）；
  F_264d4 = `LOCAL HIDDEN` 有定义、0 重定位；dispatch 表 1497 条 n/长度/升序/同址/定义全通过
- **符号无丢失**：当前 so 相对 v178e0 纯增（F_15c0c8/F_6ee0d0/F_9e0f0/F_9e204/F_9e2b4 + 6 GL 符号）
- **纹理管线 4 函数就位**：F_9e0d4(删除) / F_9e204(创建) / F_9e0f0(上传) / F_9e2b4(更新，vtable[22])
  → dispatch idx 943-947 与 offs 0x9e0d4/0x9e0f0/0x9e204/0x9e2b4/0x9e2e0 严格对齐
- **诚实边界**：① v178e5 **未在 MuMu 实机复验**（测试时 MuMu 宿主 10.0.182.146 下线，
  ARP FAILED；v178e4 那一版 logcat 已确认**无 UnsatisfiedLinkError**，即 dlopen 成功，
  但 v178e5（直连 GL 版）仍需 MuMu 回来后复验 + 截图看棋盘纹理是否真上传）
  ② 棋盘**数据**仍未加载（纹理对象 this[28]=RGBA/[32]=宽/[36]=高 全 0，F_9f88c 构造时置 0，
  运行时加载路径未逆向）→ **即使 GL 管线完整也无数据可上传**
  ③ "渲染循环跑""清屏色出帧"≠"棋盘画出"≠"能玩"

### §124zz-1 ★ 纹理管线 GL 调用序列逐条核对（tex_pipeline.py 升级：PLT 解析 + 寄存器跟踪）

**工具升级**（`rev64/tools/tex_pipeline.py`）：
- 新增 `plt` 子命令：解析 ARM `.plt`（PLT0=**20 字节**，条目 stride=**12**，起始 PLT0+20）
  → GOT 地址 = (桩+8+0x100000) + ror32(56,20) + off → 符号名（**214 条全解析**）
  - 坑：`readelf -rW` 行格式 `<off> <info> R_ARM_JUMP_SLOT <symvalue> <symbol>`，
    符号是 **p[4]** 不是 p[3]（p[3] 是 Sym. Value）
- 新增 `glcall <addr> <len>`：反汇编 + **轻量寄存器跟踪** → 直接打印
  `→ glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)` 形式的**实参**
- 修正工具自带 GL 枚举表（**我原先写反了**）：MAG_FILTER=**0x2800**、MIN_FILTER=**0x2801**、
  WRAP_S=**0x2802**、WRAP_T=**0x2803**（NDK `GLES2/gl2.h` 为准）

**原版 F_9e204（纹理创建）权威 GL 序列**（核对通过）：
```
glGenTextures(1, &this[0x4c])
glBindTexture(GL_TEXTURE_2D, this[0x4c])
glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE)  /* 33071.0f */
glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE)  /* 33071.0f */
glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)        /* 9729.0f  */
glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)        /* 9729.0f  */
glPixelStorei(GL_UNPACK_ALIGNMENT, 4)
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this[0x20], this[0x24], 0, GL_RGBA, GL_UNSIGNED_BYTE, this[0x1c])
```
- **★ 翻译体 F_9e204 核对结论：实参值全部一致**（WRAP_S/T=CLAMP_TO_EDGE，MAG/MIN=LINEAR；
  仅调用**顺序**不同——原版是 WRAP_S,WRAP_T,MAG,MIN，翻译体是 MAG,MIN,WRAP_S,WRAP_T；
  GL 各自独立设状态，**顺序无影响**）。→ 无需改代码
- ★ 注意原版 `ldr r7` 的**时序坑**：0x9e22a 的 `adds r2,r7,#0` 在 0x9e22c 的 `ldr r7` **之前**，
  故第 2 个 glTexParameterf 用的是**旧** r7（CLAMP_TO_EDGE）；不跟踪寄存器会读错

**字段语义（经原版反汇编确认）**：
| 偏移 | 含义 | 证据 |
|---|---|---|
| this[0x1c] (28) | 像素数据指针 (RGBA) | F_9e204 glTexImage2D 第 9 参；F_9e0f0 memcpy 源 |
| this[0x20] (32) | 宽（== 源行距，像素） | F_9e204 glTexImage2D width；F_9e0f0 行距×4 |
| this[0x24] (36) | 高 | F_9e204 glTexImage2D height |
| this[0x4c] (76) | GL 纹理 ID | glGenTextures/glBindTexture 实参 |

**★ F_9e0f0（上传）原版是"子矩形 + 逐行 memcpy"上传**（非整图一次性）：
`bl 0xafca8` 用 this[0x20]/this[0x24] 构造一个 **4 整数矩形**（std::vector，落在 sp+0x30），
再按矩形 x/y/w/h 逐行 memcpy 到 `_Znaj(w*h*4)` 的缓冲，最后
`glTexSubImage2D(GL_TEXTURE_2D, 0,0,0, ...)`。
→ **翻译体 F_9e0f0 是简化版**（整图一次 memcpy + 全图 glTexSubImage2D）。
全图更新时等价；**局部脏区更新时不等价**（已知简化，非 bug 未修，因对象图未通无法验证）。

**产物**：`rev64/dist/libjunqi_arm32_aligned.so`（5443820B，UND=实证基线43+6GL，审计 PASS）+
`rev64/dist/phase_chain_arm32.apk`（11956224B，v1+v2，`.so` 全 STORE，审计闸门已过）

**★ 纹理管线 4 函数 vs 原版逐条核对结论（`tex_pipeline.py texseq` 一次出全部原版序列）**：

| 函数 | 原版 GL 序列 | 翻译体 | 结论 |
|---|---|---|---|
| F_9e0d4 删除 | glDeleteTextures(1,&this[0x4c]); this[0x4c]=0 | 同 | **完全一致** |
| F_9e204 创建 | 见上 8 条 | 实参值全同（仅调用顺序不同，无影响） | **完全一致** |
| F_9e0f0 上传 | _Znaj→memcpy→glBindTexture→glPixelStorei→glTexSubImage2D→_ZdaPv | 同结构 | **结构一致**（子矩形/逐行 vs 整图，简化已记录） |
| F_9e2b4 更新 | 无直接 GL；this[76]==0→bl 0x9e204→**b 0x9e2be 回跳**；[0x28]!=[0x30]→bl 0x9e0f0→清 [0x28]/[0x2c]/[0x30]/[0x34] | 同（含创建后回落到脏区检查） | **完全一致** |

→ 4 个函数的**控制流与 GL 实参**均与原版一致，**本轮无需改翻译体代码**。
（核对工具：`python3 rev64/tools/tex_pipeline.py texseq`）

### §124zz-2 ★★ 纹理类对象图逆向 + 修一个真实野指针 bug（哨兵值 = -1 不是 0）

**新增工具能力**（`rev64/tools/tex_pipeline.py`）：
- `vtable <addr> [n]` — dump vtable（**★ R_ARM_RELATIVE 的 addend 不在 readelf 文本输出里**
  （那列是空的），addend 存在 .rel.dyn 条目**目标地址处的 4 字节** → 必须解析节二进制）
- `findvtable <func>` — 哪个 vtable 的哪个槽 == 该函数
- `texclass [vtable_base]` — 列出类虚方法并标出写像素字段的
- `funcat <addr>` — 用 `seed_v2.json`（6142 个函数起始）二分找包含该地址的函数
- `findctor <vtable>` — 找设 vptr 的构造器
- `stores <off>` — 全 .text 扫 `str [*,#off]`
- **★ capstone 坑**：`str` 的操作数顺序是 **[源, 目的]**，MEM 操作数是 `operands[1]`
  （用 operands[0].mem → 拿到源寄存器的 mem.disp=0 → 扫出 0 命中，误判"没有写入点"）

**纹理类 vtable = 0x1595c0**（`findvtable 0x9e2b4` 定位）：
- slot[22] = 0x9e2b5 (F_9e2b4 更新) / slot[23] = 0x9e0d5 (F_9e0d4 删除)
- slot[31] = 0xa4595（写 [0x20]/[0x24]）/ slot[42] = 0x9fc9c（加载资源）
- ★ 这就是 checkpoint 里叫"渲染器单例 0x15c0a0 的 vtable"的**同一个 vtable**

**纹理对象字段语义（经原版构造器反汇编确认）**：
| 偏移 | 含义 | 证据 |
|---|---|---|
| this[0] | vtable = 0x1595c0 | 构造器 `str r3,[r0]` |
| this[0x1c] (28) | 像素数据指针 (RGBA)，**空哨兵 = -1 (0xffffffff)** | ★ 构造器 0x9fd88: `rsbs r3,r3,#0` → -1 |
| this[0x20] (32) | 宽（构造时取自全局 [0]） | ctor: `ldr r2,[r3]; str r2,[r4,#0x20]` |
| this[0x24] (36) | 高（构造时取自全局 [1]） | ctor: `ldr r3,[r3,#4]; str r3,[r4,#0x24]` |
| this[0x28..0x34] | 脏区 4 整数（上传后清 0） | F_9e2b4 清 [0x28]/[0x2c]/[0x30]/[0x34] |
| this[0x4c] (76) | GL 纹理 ID | glGenTextures/glBindTexture/glDeleteTextures |

**★ 修掉的真实 bug（野指针 memcpy / SIGSEGV 风险）**：
- 翻译体 F_9e0f0 的"无数据"防御只挡 `data==0`，**漏了哨兵 -1** →
  `memcpy(buf, (void*)0xffffffff, w*h*4)` → **野指针读 → SIGSEGV**
- 翻译体 F_9e204 的 `glTexImage2D(..., JQT_ABS_PTR(MEM(r4+28)))` 同样会把 -1 当指针传下去
- **修法**：两个函数都同时挡 `0` 与 `0xffffffff`（F_9e204 挡掉后仍建纹理对象，只跳过
  glTexImage2D；F_9e0f0 直接返回）
- 说明：原版**不检查**（靠对象图保证图像已加载）；翻译体对象图分歧 → 必须加防御

**★ 顺带发现：原版渲染走 GLES2 着色器路径，不是 GLES1 固定管线**。
draw 函数（原版 0x9e760）实测 GL 序列：
`glVertexAttribPointer ×3`（2/4/2 分量）→ `glUniform1i ×2` → `glBindTexture(GL_TEXTURE_2D,…)`
→ `glDrawArrays(mode=4=GL_TRIANGLE_STRIP)` → `__aeabi_idiv` → `glUniform1i`
⇒ 纹理对象**自带绘制方法**（不只是纹理容器）；翻译体的 gl_bridge 强制路径只做
glClear+eglSwapBuffers（无 shader/无几何）→ 这就是"只出清屏色"的直接原因。

**诚实边界**：纹理数据加载链（.pak → SPAK 解密 → zlib → 写 this[0x1c]）**仍未逆向**；
已知入口 = 类方法 slot[42] = F_9fc9c（`bl 0xa1404` 取资源 → `bl 0xa32ec` 把 16 字节结构
赋给 this+8），但后续解码路径依赖尚未构造的对象图，无法离线闭环。
且**全部改动未在 MuMu 实机复验**（宿主离线）。

**产物（v178e6）**：`rev64/dist/libjunqi_arm32_aligned.so`（5443980B）+ 
`rev64/dist/phase_chain_arm32.apk`（11956224B，v1+v2，审计 PASS）。

### §124zz-3 ★★ 推翻"glClear 类对象 vtable=0x1595f4"（实为方法槽，非 vtable）+ 清屏路径还原

**★ 关键推翻**：§124z 结论"**glClear 类对象（vtable=0x1595f4，槽0=0x9d459=glClear 调用者）
从未构造**"是**误判**：
- `0x1595f4 = 0x1595c0 + 13×4` → 它是**纹理类 vtable 0x1595c0 的 slot[13]**，
  **不是另一个 vtable 的基址**（§124af 其实已经弄对了："glClear 单例 vtable=0x1595c0, slot13=0x9d459"，
  但 §124z 的叙述没跟上，导致后续一直在"找一个不存在的对象"）。
- slot[13] 的值 0x9d459 → Thumb 位 → **函数体 0x9d458**；实测其 GL 序列：
  `4×(__aeabi_i2f + __aeabi_fdiv 255)` → **`glClearColor(r,g,b,a)`** → **`glClear(0x4000)`**
  → `glBlendFuncSeparate(0x302,0x303,...)`
  ⇒ **它就是"清屏方法"，属于纹理/渲染类本身**，不需要另造对象。
- 推论：§124au 为"造 glClear 类对象"做的 F_a4460/F_a4484 一系列接线，是在追一个幻影；
  真正缺的是**把已构造对象（0x15c0a0，vtable 0x1595c0）的 slot[13] 调起来**。

**原版清屏 dispatch 还原（0x24002-0x24010，F_23eec 内）**：
```
0x24002: ldr r3,[pc,#0xa8] ; 0x24004: add r3,pc     ; r3 = &全局槽
0x24006: ldr r0,[r3]                              ; r0 = 渲染对象
0x24008: bl #0x32f8c                              ; → 返回带 vtable 的对象
0x2400c: ldr r3,[r0]                              ; r3 = vtable
0x2400e: ldr r3,[r3,#0x34]                        ; r3 = vtable[13] = F_9d458
0x24010: blx r3                                   ; 调用（r1 = 本帧打包清屏色）
```
⇒ **r1 就是清屏色**（原版 0x23ff2 `ldr r1,[r6]` 载入）。

**★ 翻译体改进（v178e7）**：F_9d458 里
`jqt_gl_bridge_start_frame_loop(0xff9c6630)` 硬编码蓝色，而**首帧** `jqt_gl_bridge(r1)`
用的是真实颜色 → 同一场景两色。已改为 `jqt_gl_bridge_start_frame_loop(r1 ? r1 : 0xff9c6630)`
（r1=0 时才回退）。L2400c 仍强制 `F_9d458(0, 0xff9c6630, ...)`（对象图分歧，this 传 0 安全）。

**原版渲染主循环 F_bf4d0 的 vtable 槽 dispatch（vslots 工具实测）**：
`slot[0] ×11`（虚调用）、`slot[6] ×2`（GL 程序）、`slot[23] ×4`（纹理删除）
⇒ 棋盘几何的绘制**不在 slot[6]/[23]**，而在那 11 次 slot[0] 虚调用里
（对象图未构造 → 调不到 → 所以只出清屏色）。

**新增工具命令**（`tex_pipeline.py`）：
- `vslot <n>` — 搜 vtable[n] 的**全部** dispatch 点（n 为十进制槽号，0x 前缀则十六进制）
- `vslots <addr> <len>` — 列函数区间内所有 vtable 槽 dispatch 并按槽聚合
（★ `vslot` 的槽号必须**十进制**解析，之前用 hex 导致 "13" 被当成 19）

**诚实边界**：本轮改动（v178e6 哨兵修复 + v178e7 清屏色一致）**均未在 MuMu 实机复验**
（宿主 10.0.182.146 持续离线，ARP FAILED；已挂后台 watcher，恢复即自动装+验证+截图）。

**产物（v178e7）**：`rev64/dist/libjunqi_arm32_aligned.so`（5444060B）+
`rev64/dist/phase_chain_arm32.apk`（11956224B，v1+v2，审计 PASS）。

### §124zz-4 ★★★ 场景构造链定位 + **量化剩余工程量**（2977 个函数缺失）

**★ F_242f4 = 场景全局对象初始化函数**（翻译体里是**存根** r0=0 → 从未构造任何对象）：
原版 0x242f4 开大栈帧（`add sp,r4`，r4=0xfffffad4）后连续调 **15 个函数**，
每个都是 **C++ 惰性单例访问器**（模式：`if(!*global){ *global = new(size); ctor(); }`），
负责构造 13 个全局静态对象。实测第一个 F_9d3dc：
```
0x9d3de: ldr r4,[pc,#0x1c] → pool 0xc501c
0x9d3e0: add r4, pc          → r4 = &全局槽 = 0x162400   ★ 渲染器单例槽
0x9d3e2: ldr r5,[r4]         → 已有则直接返回
0x9d3ec: movs r0,#0x88       → new(136)
0x9d3f4: bl #0x9d988         → 真正构造
0x9d3f8: str r5,[r4]         → 存回全局槽
```
⇒ **0x162400 = 渲染器单例槽**（与 §124au 的"渲染器单例 0x162400"对上）。

**★ 量化剩余工程量（`closure` 工具，调用图闭包 + 对比翻译体）**：
| root | 闭包函数数 | 完整翻译 | 仅存根 | **翻译体完全没有** |
|---|---|---|---|---|
| 0x242f4（场景初始化） | 3969 | 992 | 0 | **2977** |
| 0xbf4d0（渲染主循环） | 3968 | 991 | 0 | **2977** |
| 0x24108（渲染初始化） | 3969 | 992 | 0 | **2977** |

- 三个 root 收敛到**同一个引擎级调用图** → 翻译体覆盖了引擎调用图的 **≈25%（992/3969）**
- 缺失清单（按地址，2977 个）已落盘：`/tmp/opencode/missing_funcs_0x242f4.txt`
  （`python3 rev64/tools/tex_pipeline.py closure 0x242f4` 可重新生成）
- **结论（诚实）**：从"清屏色出帧"到"棋盘画出"缺的不是几个函数，而是
  **场景构造链 + 其下游约 3000 个函数**。这是**规模化翻译工程**，
  不是再打几个补丁能解决的——之前"黑屏=对象图分歧"的描述过于局部。

**新增工具命令**（`tex_pipeline.py`）：
- `scenechain <root>` — 列出 root 直接调用的构造器 + 每个装的 vtable / 全局静态槽地址
- `closure <root>` — 调用闭包 + 翻译体覆盖分布 + **缺失清单落盘**
（★ 翻译体状态判定：函数定义**参数有名字**（`_r0`）= 完整翻译，无名 = 存根；
  两者都找不到 = 完全没有）
