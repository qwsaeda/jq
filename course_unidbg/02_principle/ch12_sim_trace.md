# 第12章 unidbg 源码解析：模拟执行流程追踪

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的 JNI 调用

## 痛点：为什么"加载好了"却调不出想要的函数？

加载（第 6-8 章）成功了，但想调 `OnCreate` 又崩了。军棋实测 3 个坑。

### 痛点 1：函数入口在哪？strip 后符号表空

军棋 so 删掉符号表后，你不知道 `OnCreate` 在哪个地址。只能先静态分析
（IDA/objdump）定位入口（军棋 14 个 JNI 入口是 Thumb 奇地址 0x233e1 等），
再按地址调。

- **大白话卡点**：门牌号全撕了（strip），`OnCreate` 在哪间房不知道，得先量出地址。
- **为什么卡**：军棋 14 个 JNI 入口是 Thumb 奇地址（bit0=1），不在偶指令表。
- **不解决会怎样**：调函数第一步地址就错，或可达指令集为空（跑出 0KB body）。

**"门牌号全撕"为什么还能调（大白话）**：军棋 strip 删了普通符号表（`.symtab`），
但 `.dynsym`（动态符号表）保留了 14 个 `On*` 导出符号（so 对外要导出的 JNI 入口）。
这 14 个是"唯一没撕的门牌"，能按符号名查到地址。其他函数（strip 后没符号）
只能靠 objdump 反汇编"量"出地址。

### 痛点 2：函数要参数，参数怎么构造？

军棋 `OnCreate(env, obj)` 要一个 `JNIEnv*`（JNI 环境指针）和一个 Java 对象
（`GLView` 实例）。模拟环境里这些指针要手动构造（`getJNIEnv()` + 对象 hash），
不能"随便填个值"。

- **大白话卡点**：调 `OnCreate` 得带两个"行李"（env + obj），随便填个值 → 空指针。
- **为什么卡**：`JNIEnv*` 是"电话本首地址"、`obj` 是"Java 对象替身"，都得构造。
- **不解决会怎样**：参数错 → so 内部读空指针崩。

**"两个行李"怎么构造（大白话）**：
- `env`（`JNIEnv*`）= 电话本首地址，`vm.getJNIEnv()` 拿
- `obj`（Java 对象）= `GLView` 实例替身，`vm.addGlobalObject` 注册拿 hash
两个行李都构造好，`OnCreate` 才"接得住"。随便填 0 → so 读 env/obj 空指针崩。

### 痛点 3：函数内部又调 Java、又读内存，谁来应答？

调 `FindClass` 要 DalvikVM 应答（第 11 章），`std::vector` 操作要 C++ 运行时补全。
军棋 `OnCreate` 一执行就推进到 `std::vector` 对象图 0x43ed8 卡住
（数据与指令对齐未初始化，红线 §R6）。

- **大白话卡点**：`OnCreate` 内部要接"Java 电话" + 操作"C++ 对象图"，
  每样都得有人应答，缺一个就崩。
- **为什么卡**：`std::vector`（C++ 动态数组）数据与指令对齐没补全。
- **不解决会怎样**：推进到 0x43ed8 卡住（§124 的 ② 数据与指令对齐对应缺口）。

**"内部电话 + C++ 对象图"为什么难（大白话）**：`OnCreate` 不只是"调个函数"，
内部要：① 接 Java 电话（`FindClass`/`callMethod`，DalvikVM 应答）② 操作
C++ 对象图（`std::vector` 容器，`libstdc++` 语义）③ 读内存（配置/存档）。
军棋 ① 全通（DiagJni），② 卡住（0x43ed8 vector 未初始化）——C++ 对象图是
unidbg 路线的天花板（§124 的 ②）。

## 为什么这样做：把"调用一个函数"拆成"找入口→构造参数→应答内部调用"三步

类比"给一条狗指令让它去送信"（三步递进）：

1. **找入口**（函数在哪）：strip 后没门牌（符号表空），只能先量出地址
   （军棋 14 个 JNI 入口 0x233e1 等，Thumb 奇地址需 `&~1` 对齐到偶指令）。
2. **构造参数**（信要带啥）：`JNIEnv*`（电话本首地址）+ Java 对象（信的收件人）。
   `vm.getJNIEnv()` 拿环境指针，对象用 `addGlobalObject` 注册拿 hash。
3. **应答内部调用**（送信路上要接的电话）：`FindClass`、`std::vector` 操作、文件读
   ——每个都要模拟侧应答，缺一个就崩。

**为什么拆三步？** 三步有依赖：入口对（1）才谈得上调；参数对（2）才能进函数；
内部调用应答（3）才不崩在函数里。军棋实测：第 1、2 步全通（DiagJni 实证
`createDalvikVM + resolveClass + callStaticJniMethodObject`），第 3 步卡在
§124 纠正后 3 项阻塞：① 上下文相关 BL 解码 bug（加载 .so 后 BL 0x7965a 被
unicorn2 解码成 0x6eaea，真实目标 0x43ed8，模拟器精度问题）② 数据与指令对齐对应缺口
（`std::vector` begin/end/capacity 未初始化）③ 288MB PROT_NONE mmap
（unidbg mmap2 处理失败，红线 §R6 + §124）。

**"送信三步"和军棋对应（大白话）**：
- 找入口 = 量出 `OnCreate` 地址 0x233e1（strip 后唯一门牌，`.dynsym` 保留）
- 构造参数 = 带 env（电话本首地址）+ obj（`GLView` 替身）两个"行李"
- 应答内部 = 送信路上接"Java 电话"（DalvikVM）+ 操作"C++ 对象图"（`libstdc++`）
军棋前两步全通，第三步卡 C++ 对象图（§124 的 ②）。

**预判追问："为什么不能按符号调 OnCreate，要按地址调？"**
军棋 strip 后 `.symtab`（普通符号表）被删，但 `.dynsym`（动态符号表）保留了 14 个
`On*` 导出符号。按符号调 `findSymbol("Java_com_cnvcs_App_OnCreate")` 能查到地址，
但查到的地址是 **Thumb 奇地址 0x233e1**（bit0=1 是 Thumb 模式标记），调用前必须
`&~1` 对齐到偶指令（0x233e0），否则翻译器可达指令集为空 → 跑出 0KB body。
所以军棋"按符号查地址 + `&~1` 对齐"是两步：先 `findSymbol` 拿奇地址，再 `&~1` 调。
内部函数（strip 后没符号）只能靠 objdump/IDA 量出地址再调。

## 12.1 编写含 JNI 交互的 so 并模拟执行（书内样本 → 军棋对应）


> **本节核心**：
> 1. 军棋对应：现成 libjunqi.so 地址调用（14 个 On* &~1 对齐）
>
> **本节难点**：
> 1. 书内写 JNI so → 军棋直接调现成 strip so
> 2. 军棋 14 入口是 .dynsym 仅剩导出符号

书内样本：一个 NDK so，导出 `Java_xxx_md5(JNIEnv*, jclass, jstring)`，
unidbg 端用 `Jni` 接口实现 `findClass` / `getMethodID` / `callXxxMethod*`（应答"电话"）。
```java
// 军棋 32 位 so 的对应实现：应答 so 拨来的 JNI 电话
// findClass：so 问"com/baidu/mobads/AdView 类在哪？"（广告层拆除：返回 null 跳过广告初始化）
// getMethodID：so 问"这个方法号是多少？"
// callIntMethod：so 要调一个返回 int 的 Java 方法
vm.setJni(new Jni() {
    @Override public DvmObject<?> findClass(BaseVM vm, String name) {
        // 广告类 AdView 返回 null（去广告）；其他类正常 resolveClass
        if (name.equals("com/baidu/mobads/AdView")) return null;
        return vm.resolveClass(name);
    }
    @Override public long getMethodID(BaseVM vm, DvmClass<?> cls, String name, String sig) {
        // 军棋 14 个 On* 方法正常返回方法号
        return super.getMethodID(vm, cls, name, sig);
    }
    @Override public long callIntMethod(BaseVM vm, DvmObject<?> obj, String name, String sig, Object... args) {
        // 调一个返回 int 的 Java 方法（军棋走子/超时事件）
        return 0L;
    }
    // ...（callLongMethod/callObjectMethod 等按军棋实际调用补）
});
```
**军棋 14 个入口名（readelf 实测）**：OnCreate / OnDestroy / OnResult / OnResume /
OnEvent / OnKey / OnUpdate / OnGraphChange / OnBluetooth / OnPause / OnGraphCreate /
OnTimer / OnInput / OnTouch。
**"应答电话"三条规则（大白话）**：
- `findClass`：广告类（AdView）返 null（去广告），核心类正常（`resolveClass`）
- `getMethodID`：14 个 `On*` 方法正常返方法号（核心入口能调通）
- `callIntMethod`：军棋走子/超时事件，返 0（占位）
三条规则 = "核心正常接、广告 null 挡"，和第 29 章 JNI 反制一致。
## 12.2 模拟执行流程追踪：寻找函数（军棋 14 个 JNI 入口）


> **本节核心**：
> 1. 军棋 14 个 JNI 入口按地址找（strip 后内部函数无名字）
>
> **本节难点**：
> 1. IDA 静态分析找入口地址，再按地址调
> 2. 14 入口 = OnCreate/OnDestroy/OnResult/OnResume/OnEvent/OnKey/OnUpdate/OnGraphChange/OnBluetooth/OnPause/OnGraphCreate/OnTimer/OnInput/OnTouch（readelf 实测）

![军棋 14 JNI 入口](assets/ch12_find_func.png)



军棋 32 位 so 的 14 个 JNI 入口（`Java_com_cnvcs_App_On*`，导出符号表里唯一的"门牌号"）。

unidbg 的"寻找函数"路径（三步定位）：

```java
// 1. 加载 so → 解析导出符号表（.dynsym，strip 后只剩这 14 个 On*）
DalvikModule dm = vm.loadLibrary(new File("lib/armeabi/libjunqi.so"), true);
// 2. 按符号名找入口地址
Symbol onCreate = dm.getModule().findSymbol("Java_com_cnvcs_App_OnCreate", true);
long entryAddr = onCreate.getAddress();
// 3. 军棋 14 个 JNI 入口是 Thumb 奇地址（0x233e1 等），要 &~1 对齐到偶指令
//    不对齐 → 翻译器可达指令集为空 → 跑出 0KB body（红线 §R6，14 个入口全踩过）
entryAddr = entryAddr & ~1L;
```

**为什么 14 个入口都要 `&~1`？** 它们全是 Thumb 模式奇地址（bit0=1 是 Thumb 标记），
翻译器/模拟执行从偶指令起点开始，起点不对齐可达集就空。军棋 14 个入口
（`OnCreate` 0x233e1 等）个个是奇地址，调用前必须对齐。

**"Thumb 奇地址"为什么 `&~1`（大白话）**：ARM 有两种指令模式——ARM（32 位，偶地址）
和 Thumb（16 位，奇地址，bit0=1 标记）。军棋 14 个 `On*` 入口编译成 Thumb，
地址是奇数（如 0x233e1）。模拟执行要"从偶指令起点"跑，奇地址 `&~1`（清掉
bit0）变偶（0x233e0）。不清 bit0 → 翻译器找不到偶指令 → 可达集空 → 0KB body。

## 12.3 模拟执行流程追踪：处理参数并模拟执行


> **本节核心**：
> 1. 军棋 OnCreate(env, obj)：env=JNI 函数表（232 槽），obj=DvmObject 哈希
>
> **本节难点**：
> 1. 指针参数构造：getJNIEnv() + addGlobalObject()
> 2. 「应答内部调用」是补环境核心：军棋 so 内部调 Java/文件/libc，每缺一个崩

![模拟执行 3 步](assets/ch12_call_flow.png)



军棋 `OnCreate` 的 JNI 参数：
```
(JNIEnv *env, jobject obj)  // env=JNI 环境指针；obj=军棋 GLView 的 Java 对象
```

unidbg 调用（构造 2 个参数 + 调函数）：

```java
// 1. 拿 JNIEnv 指针（军棋 DiagJni 已实证 createDalvikVM + resolveClass 可行）
Pointer jniEnv = vm.getJNIEnv();            // env 参数（电话本首地址）
// 2. 构造 Java 对象替身（obj 参数，GLView 实例）
int objHash = vm.addGlobalObject(new DvmObject<?>());  // 注册全局对象拿 hash
// 3. 调 OnCreate（0x233e1，注意是 Thumb 奇地址，调用前已 &~1 对齐）
long ret = vm.getMemory().call(emulator, 0x233e1L, 0, jniEnv, objHash).singleValue();
// 4. 军棋 DiagJni 实测：OnCreate 推进到 std::vector 数据与指令对齐点 0x43ed8
//    （Java 侧全接通，卡在 native 数据与指令对齐未初始化）
```

**为什么 `objHash` 不是真实 GLView？** 军棋 7 个 Java 类在 unidbg 里是"替身"，
`addGlobalObject` 注册的是模拟对象，so 拿它的 hash 操作。这不是"真的 `GLView`
实例"，是"`GLView` 的 Java 对象替身"（第 11 章 `DvmObject`）。

**"两个行李"构造细节（大白话）**：
- `jniEnv` = `vm.getJNIEnv()`：拿 `JNIEnv*` 指针（电话本首地址），so 靠它调 Java
- `objHash` = `vm.addGlobalObject(...)`：注册 `GLView` 替身拿 hash（32 位是 int）
- `call(emulator, 0x233e1L, 0, jniEnv, objHash)`：调 OnCreate，传 env + obj 两个行李
两个行李构造好，OnCreate 才"接得住"。

## 12.4 逐环节"能干什么 / 不能干什么"

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **找入口** | 按符号查 14 个 `On*` 地址（`.dynsym` 保留）；内部函数用 IDA/objdump 量地址 | 按符号调内部函数（strip 后 `.symtab` 空，内部函数没符号） |
| **对齐入口** | `&~1` 把 Thumb 奇地址对齐到偶指令（0x233e1 → 0x233e0，红线 §R6） | 不对齐直接调（可达指令集为空 → 跑出 0KB body） |
| **构造参数** | `getJNIEnv` 拿模拟的 232 函数表；`addGlobalObject` 注册 DvmObject 替身拿 hash | 用真机 `JNIEnv*` / 传真 C++ 对象（模拟环境没有；DalvikVM 里是"哈希"不是对象指针） |
| **调函数** | 地址调用 `call(0x233e1, ...)` 14 个入口全通（DiagJni 推进到 0x43ed8） | 突破 0x43ed8（§124 ② 数据与指令对齐对应缺口，std::vector begin/end 未初始化） |
| **应答内部调用** | `setJni` 实现 `findClass`/`getMethodID`/`callXxxMethod`（AdView 返 null 去广告） | 补数据与指令对齐（std::vector 语义缺失，§124 ②；unidbg 补不了） |

**军棋实测对照**：
- 找入口 ✓（14 个 `On*` 全按符号查到地址）
- 对齐入口 ✓（14 个全 `&~1` 对齐）
- 构造参数 ✓（getJNIEnv + addGlobalObject 全通）
- 调函数 ✓（14 个入口全通，DiagJni 推进到 0x43ed8）
- 应答内部调用 ✓（AdView 返 null 去广告，核心类正常）
- **✗ 突破 0x43ed8**：§124 ② 数据与指令对齐对应缺口（Java 侧全接通后卡点转 native）

## 12.5 军棋实战验证：三步全通但卡在第 3 步


> **本节核心**：
> 1. 三步全通但卡第 3 步（§124）
>
> **本节难点**：
> 1. 第 3 步 = 应答内部调用（JNI/文件/libc），军棋卡 §124
> 2. 「找入口 → 构造参数 → 应答内部」三步，前 2 步通第 3 步卡

军棋 unidbg 调 `OnCreate` 的三步实测（红线 §R6 + §124）：
| 步骤 | 军棋结果 | 大白话 |
|---|---|---|
| 找入口（0x233e1 对齐） | 全通 ✓ | 14 入口按地址调，奇地址对齐 |
| 构造参数（env + obj） | 全通 ✓ | getJNIEnv + addGlobalObject |
| 应答内部调用 | 卡 0x43ed8 | 推进到 std::vector 数据与指令对齐点后停 |
**第 3 步卡在哪？** §124 的 3 项阻塞：① 上下文相关 BL 解码 bug（unicorn2 精度，
加载 .so 后 BL 0x7965a 解码成 0x6eaea 而非真实目标 0x43ed8）② 数据与指令对齐对应缺口
（std::vector begin/end/capacity 未初始化，0x43ed8）③ 288MB PROT_NONE mmap
（unidbg mmap2 处理失败）。**Java 侧全接通后，瓶颈全在 native 数据与指令对齐（N44）**，
unidbg 路线走不通这 3 项 → 早期只能 redroid dump（第 8/9 章）；**★ §124x-z 后 = 静态位移固化 `jqt_mem.c` `g_jq_got_tbl` 免 dump**。
**"三步全通"的 4 个证据（红线 §R6）**：
1. 找入口：14 个 `On*` 全按地址找到（`.dynsym` 保留）
2. 奇地址对齐：14 入口全 `&~1` 对齐（0x233e1 等）
3. 构造参数：getJNIEnv + addGlobalObject 全通（DiagJni）
4. 应答内部：Java 电话全接（resolveClass/callStaticJniMethodObject）
前 4 条全 ✓ = 前两步通，第三步卡 native 对象图。
## 12.6 本章小结（一句话版）

> 军棋 14 个 JNI 入口（`Java_com_cnvcs_App_On*`）是 unidbg 主动调用目标，按地址调
> （strip 后符号表空，`.dynsym` 保留 14 个门牌）。
> "找函数"路径：loadLibrary → findSymbol → 入口地址（Thumb 奇地址 0x233e1 需 `&~1` 对齐）。
> 军棋 DiagJni 实证 Java 侧全接通，`OnCreate` 推进到 `std::vector` 对象图 0x43ed8。
> §124 纠正后剩余卡点 = 3 项阻塞：① 上下文相关 BL 解码 bug（unicorn2 精度，
> BL 0x7965a 错位到 0x6eaea）② 数据与指令对齐对应缺口（std::vector 0x43ed8）
> ③ 288MB PROT_NONE mmap。

---

## 视频
