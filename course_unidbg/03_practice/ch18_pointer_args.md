# 第18章 unidbg 实战：指针参数与 Debugger

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的指针参数 + 加密识别

## 痛点：为什么指针参数是逆向的"硬骨头"？

军棋 14 个 JNI 入口（`Java_com_cnvcs_App_On*`，readelf 实测：OnCreate /
OnDestroy / OnResult / OnResume / OnEvent / OnKey / OnUpdate / OnGraphChange /
OnBluetooth / OnPause / OnGraphCreate / OnTimer / OnInput / OnTouch）全部是
`(JNIEnv *env, jobject obj)` 形态，要两个指针参数。
**逆向者要"跟指针走到真值"，这是最难的活。** 军棋实测 3 个卡点。

### 痛点 1：`JNIEnv*` 不是"真指针"，是"函数表"

`JNIEnv*` 指向一张 **232 个函数指针的表**（JNI 接口表，`GetVersion` 起共
232 槽）。so 调 `(*env)->FindClass(env, "java/lang/String")` 时，其实是
"从表里第 N 个槽取函数指针，再调它"。

- **大白话卡点**：你以为 `env` 是"地址"，其实是"电话簿"（232 个函数号），
  簿子里任何一个号是垃圾，so 一拨就跳飞。
- **为什么卡**：真机上这张表由 ART 运行时填好；unidbg 里你得手动构造这张表
  （`vm.getJNIEnv()` 返回的指针指向模拟的 232 槽表），so 调 `FindClass` 时，
  表里的槽要能真调 unidbg 的 Java 模拟方法。
- **不解决会怎样**：so 取到垃圾函数指针 → 跳飞 → SIGSEGV。军棋 `OnCreate`
  第一步就调 `(*env)->...`，表没构造好直接崩在入口。

### 痛点 2：`jobject` 是"哈希"，不是"真对象"

`jobject` 在 native 层是一个"整型哈希"（指向 DalvikVM 里对象的引用），
不是 C++ 对象指针。

- **大白话卡点**：so 拿 `obj` 调 `(*env)->GetObjectClass(obj)`，要能查回
  GLView 类——你得先"注册"这个哈希，查表才出得来。
- **为什么卡**：军棋 `OnCreate(env, obj)` 的 `obj` 是 `GLView` 的 Java 对象哈希，
  unidbg 里用 `vm.addGlobalObject(...)` 注册拿哈希，DalvikVM 维护
  "哈希 ↔ 对象"的映射；没注册就传个裸 int，so 查表落空。
- **不解决会怎样**：`GetObjectClass` 返回空 → so 后续逻辑全乱。

### 痛点 3：加密算法藏在指针后面

so 把密钥、SBOX 表都放在指针指向的内存里，你看不懂就识别不出算法。

- **大白话卡点**：密钥（SPAK 32B key）和 256 字节置换表都是"指针后面那串字节"，
  不跟指针走到那块内存，算法认不出。
- **为什么卡**：军棋 SPAK key 内嵌 `.rodata`（静态明文可 dump），但"解包函数"
  要跑一下才知道 9 包 883 条目的结构；`XOR(key[j%32]) + zlib` 算法的 key 藏在指针
  后面，跟指针走到才能看到真表。
- **不解决会怎样**：资源（棋子图集/棋盘）解包全乱码。

## 为什么这样做：用指针"喂"参数 + 用 Debugger 看指针指向什么

指针参数 = 把"真值"藏在内存里，参数只是地址。逆向要"跟指针走到真值"。

类比：指针是一张"门牌号"，你要拿着门牌号去"那扇门"（内存地址）看里面的东西
（真值）。unidbg 的 `getJNIEnv` / `addGlobalObject` 就是"给你门牌号"，
Debugger 的 `step` / `showRegs` 就是"帮你走到门后看"。

**预判追问："JNIEnv* 直接传个 0 或随便一个地址行不行？"** 不行。so 入口前几
条指令就是 `ldr ip, [r0, #N]`（从表第 N 槽取函数指针再 `blx ip`），传 0 或垃圾
地址 = 表全是 0 → 第一个 JNI 调用就 `blx 0` 崩。军棋 232 槽表里最常用的是
`FindClass`/`GetMethodID`/`CallStaticXxxMethod`（14 入口 + 广告 AdView 查询），
这些槽必须指向 unidbg 的真实现（`setJni` 应答器），其余槽给存根（返回 0）即可。

**预判追问："jobject 不注册，直接传个随机 int 会怎样？"** DalvikVM 查不到
这个哈希对应的对象 → `GetObjectClass` 应答落空 → so 拿 null 类继续走 →
要么崩要么逻辑错。军棋 `addGlobalObject(new DvmObject<>(vm.resolveClass(
"com/cnvcs/GLView")))` 注册的是 GLView 类替身，so 查回"GLView"类，
跟真机 `GLView` 对象的行为对得上。

**"指针 vs 值参数"（大白话）**：值参数（int/long）直接放寄存器；指针参数是
"地址放寄存器，真值在内存里"。军棋 14 入口全是"两个指针"，两个门牌号
（env 表 + obj 哈希）都得先备好，所以调 14 入口前永远先 `getJNIEnv` +
`addGlobalObject` 两步。

## 18.1 指针参数的使用（军棋 `OnCreate` 实战）
![军棋指针参数](assets/ch18_pointer_args.png)
### 18.1.1 对 App 进行分析（军棋 smali 层）


> **本节核心**：
> 1. 军棋 smali 层分析（App/GLView 调 native）
>
> **本节难点**：
> 1. 军棋 7 个 Java 类，App 调 14 个 On* native
> 2. smali 层看 Java 怎么调 native（com.cnvcs.App → libjunqi.so）
> 3. 军棋 smali 极小（~7 类几百行），native 层是核心

军棋 32 位 so 的 JNI 入口（readelf 实测 14 个，名字必须用这 14 个）：
```
OnCreate / OnDestroy / OnResult / OnResume / OnEvent
OnKey / OnUpdate / OnGraphChange / OnBluetooth / OnPause
OnGraphCreate / OnTimer / OnInput / OnTouch
```
`OnCreate` 的 JNI 签名（`smali/com/cnvcs/App.smali`，红线 §R6 不可改）：
```
Java_com_cnvcs_App_OnCreate(JNIEnv *env, jobject obj)
  - env:JNIEnv*(JNI 函数表指针, 232 槽)
  - obj:jobject(GLView 的 Java 对象哈希)
```
### 18.1.2 使用 unidbg 进行模拟执行（构造指针参数）


> **本节核心**：
> 1. 构造指针参数（env + obj）：env=getJNIEnv()（JNI 函数表 232 槽）
> 2. obj=addGlobalObject(DvmObject)（哈希非 C++ 对象指针）
>
> **本节难点**：
> 1. 指针参数是逆向硬骨头：JNIEnv* 不是真指针是函数表
> 2. 军棋 14 入口签名不统一，逐个 marshal

```java
// 1. 拿 JNIEnv* 指针(232 函数表,unidbg 模拟构造)
Pointer jniEnv = vm.getJNIEnv();
// 2. 注册 jobject(GLView 对象哈希)
// addGlobalObject:把 DvmObject 注册到 DalvikVM,返回整型哈希
int objHash = vm.addGlobalObject(vm.resolveClass("com/cnvcs/GLView").newObject());
// 3. 调 OnCreate(0x233e1,军棋 14 个 JNI 入口之一,Thumb 奇地址)
long ret = vm.getMemory().call(emulator, 0x233e1L, 0, jniEnv, objHash).singleValue();
// 军棋 DiagJni:OnCreate 推进到 std::vector 数据与指令对齐点 0x43ed8(§124 的 ② 卡在这)
```
**为什么 `0x233e1` 是奇地址？** 军棋 so 是 ARM/Thumb 混合编译，
JNI 入口在 Thumb 模式下，函数地址 bit0=1（奇数）表示"跳 Thumb 模式"。
unidbg 调用前 `&~1` 对齐到偶指令（红线 §R6：JNI 入口是 Thumb 奇地址，
`translate_func` 起点要 bisect 对齐到最近偶指令，否则可达集空 → 0KB body）。
**14 入口全是这种形态**：不止 `OnCreate` 0x233e1，其余 13 个（`OnDestroy`/
`OnEvent`/`OnBluetooth`...）同样是 Thumb 奇地址，调任何一个都要先 `&~1`。
军棋机械化逆向回归里"14 JNI 入口"的可运行验证全部卡在这类地址对齐 + 18 个
`.text` 常量池（早期需 redroid dump 72B 注入；**★ §124x-z 后 = 静态位移固化 `jqt_mem.c` `g_jq_got_tbl`，免 dump**）上。
## 18.2 快速识别加密算法（军棋 SPAK + XOR+zlib）
![加密识别](assets/ch18_aes_identify.png)
### 18.2.1 对 App 进行分析（军棋加密层，R5 确证）


> **本节核心**：
> 1. 军棋 SPAK 加密层（R5 确证 XOR+zlib，非魔改 Base64）
>
> **本节难点**：
> 1. key 32B 在 .rodata VA 0x12a658 明文；9 包 883 条目
> 2. 「识别加密算法」军棋对应 SPAK（XOR(key[j%32]) + zlib inflate）
> 3. 原书「识别 AES」军棋改成 SPAK（军棋实际算法，合理映射）

军棋 32 位 so 的加密层（红线 §R6 + rev64/reverse/phases/R5_spak.md 确证）：
- **SPAK key 32B**：`.rodata` VA `0x12a658`（静态明文，`frida-dump/rodata_dec.bin` 已 dump）
- **算法 `XOR(key[j%32]) + zlib inflate`**（**纠正早期"魔改 Base64"推测**）
- **9 包 883 条目**：data 121 / datah 272 / datam 189 / datax 301（不是"4 组"）
- **data 组按 `MD5(资源路径)` 寻址**；datah/datam/datax 按 UI 布局 XML 索引
- **棋子图集**：zlib 压缩（212×280，4列×7行，53×40/格，来自 datah 组）
- **棋盘**：XOR 未压缩（737×751 十字形，中央竖带 12行×5列，39×59/格）
**军棋"魔改"在哪？** 不是"魔改 Base64"（标准 Base64 是字符编码，军棋没用）。
实际是 **`XOR(key[j%32]) + zlib`**——每个字节与 `key[j % 32]` 异或，再 zlib 解压。
标准 `base64 -d` / 普通 zlib 都解不出来，得先按 key 长度 32 做 XOR 还原。
### 18.2.2 使用 unidbg 进行模拟执行（Hook 解包入口识别算法）


> **本节核心**：
> 1. Hook 解包入口识别算法（军棋 SPAK XOR+zlib）
>
> **本节难点**：
> 1. unidbg 跑不到解包点（卡 §124），只能 Hook 入口识别；解包走 redroid
> 2. Hook 解包入口 = 看 so 怎么解 SPAK
> 3. 军棋 key 静态可读（.rodata 非壳），解包需 redroid

```java
// Hook SPAK 解包函数入口, 捕获 key + 密文
emulator.getBackend().hook(module.base + SPAK_DECRYPT_ADDR, (hook) -> {
    byte[] spakKey = hook.readMemory(keyPtr, 32);        // 32B key(.rodata VA 0x12a658)
    byte[] cipher  = hook.readMemory(dataPtr, 883);      // 某包 883 字节条目区
    // 实际算法: XOR(key[j%32]) + zlib inflate(非魔改 Base64)
    byte[] plain   = xorThenZlibInflate(spakKey, cipher);
    System.out.println("解密后首 16 字节: " + hex(plain, 16));
});
```
**军棋识别特征对照**（书内 §18.2 的 AES 特征 vs 军棋实际，R5 确证）：
| 算法特征 | 书内 AES 识别 | 军棋实际（R5） |
|---|---|---|
| 置换表（256 字节） | AES SBOX | 军棋无（**不是魔改 Base64，是 XOR+zlib**） |
| inverse 表 | AES 逆置换 | 军棋无 |
| 轮常量（11 个） | AES RCON | 军棋无（非 AES） |
| SPAK key 32B | — | **军棋特有**（`XOR(key[j%32])` 驱动 9 包 883 条目解包） |
| 压缩 | AES 无 | zlib inflate（压缩资源） |
**关键结论**：军棋**既不是 AES 也不是魔改 Base64**，是 **`XOR(key[j%32]) + zlib`**。
书内 §18 的"快速识别"方法论（Hook 解包入口 + 捕获 key/明文）对军棋适用，
但识别结果是"XOR + zlib 自定义打包"而非"AES / 魔改 Base64"（R5 实测纠正）。
**预判追问："key 在 .rodata 明文里，为什么不静态读 key 直接解？"** key 确实
静态能读（`.rodata` 非壳已证，118KB 静态即明文，VA 0x12a658 可直接 dump），但
"用 key 解包"是解包函数的运行时逻辑，9 包 883 条目的"包间关系"要跑一遍才知道
（哪包走 zlib、哪包走 XOR，data 组按 MD5 寻址、datah/datam/datax 按 UI 索引，
R5 专门验证过 38+ 个路径）。静态 key + 动态 Hook 配合，才是完整识别。
## 18.3 逐环节"我们能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **构造 JNIEnv\*** | `vm.getJNIEnv()` 拿模拟的 232 函数表，常用槽（FindClass/GetMethodID/CallStaticXxx）接 `setJni` 应答器 | 用真机的 `JNIEnv*`（模拟环境没有 ART 运行时） |
| **构造 jobject** | `addGlobalObject(GLView 替身)` 注册拿哈希，`GetObjectClass` 查回 GLView | 传真 C++ 对象（DalvikVM 里是"哈希"不是对象指针） |
| **调 14 入口** | 14 个 `On*` 全部地址可调用（DiagJni 实证推进） | 拿到真 C++ 对象图（§124 的 ② std::vector 0x43ed8 语义缺失） |
| **跟指针看算法** | 识别"XOR(key[j%32]) + zlib + SPAK 32B key"（R5 确证，非 AES / 非魔改 Base64），9 包 883 条目 | 靠 unidbg Hook 跑通解包函数（卡 0x43ed8 跑不到解包点，退 redroid） |

## 本章小结（一句话版）

> 指针参数 = 真值藏在内存里，参数只是地址。逆向要"跟指针走到真值"。
> 军棋 14 入口（`OnCreate`...`OnTouch`）全是"两个指针"：`env` = 232 槽函数表
> （`getJNIEnv` 构造，Thumb 奇地址 `&~1` 对齐后调），`obj` = GLView 哈希
> （`addGlobalObject` 注册）。
> 加密识别 = 跟指针走到解包入口，军棋实际是 **`XOR(key[j%32]) + zlib`** + SPAK 32B key
> （`.rodata` VA 0x12a658，R5 确证，**不是 AES / 不是魔改 Base64**），
> 9 包 883 条目（data 121 / datah 272 / datam 189 / datax 301）解包棋子图集 / 棋盘。

## 视频
