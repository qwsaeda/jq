# unidbg 课程实战修正记录(R6 红线对齐)

> 配合 `/disk/workspaces/jq/AGENTS.md` 红线 §R6,本文件记录《unidbg 课程》军棋对照部分的修正项。
> 修正原则:课程讲义里的军棋数据要与 AGENTS.md / frida-dump / rev64 / dist 真实数据一致。

## 修正 1:JNI 导出符号数 = 14(不是 15)

- **课程原写**:"15 个 `Java_com_cnvcs_App_On*`"
- **实测**(readelf `--dyn-syms`):14 个 JNI 导出符号
- 修正:所有课程章节中"15 个 On*"统一改"14 个 On*"

## 修正 2:so 依赖库 = 7 个(不是 6 个)

- **课程原写**:"6 个依赖库(clic/libstdc++/libm/libdl/libGLESv1_CM/libGLESv2)"
- **实测**(readelf `-d`):7 个 DT_NEEDED
  ```
  libc.so / libstdc++.so / libm.so / liblog.so / libGLESv1_CM.so / libGLESv2.so / libdl.so
  ```
  课程漏了 **liblog.so**。
- 修正:所有课程章节中"6 依赖库"统一改"7 依赖库(含 liblog)"

## 修正 3:军棋 smali 层只有 7 个 Java 类(原课程正确)

- `com.cnvcs.App` / `GLView` / `SiteView` / `AdLis` / `BtReceiver` / `TextDlg` / `InputDlg`
- 课程 Ch11 已正确(7 个 Java 类)

## 修正 4:PC off-by-2 修复 = unicorn2 后端(原课程 Ch1/5/7/30 已对齐)

- Ch1 表里 `AndroidEmulator` 行已改"unicorn2(首选)/dynarmic"
- Ch5/Ch7/Ch30 已对齐(`DiagTracePC` 0xa37fc 存 0x159880)

## 修正 5:OnCreate 推进边界 = std::vector 对象图 0x43ed8(原课程 Ch11/12/13/22 已对齐)

- DiagJni 实证:`createDalvikVM + resolveClass + callStaticJniMethodObject`
- 剩余阻塞:"C++ 对象图语义缺失"(std::vector begin/end 来自未初始化对象)

## 修正 6:18 个 `.text` 常量 + 2 个 `.data` = 72B 运行时数据

- 仅 dlopen 重定位填,**非壳自解密**(`.rodata` 静态即明文)
- 拿法唯一:redroid 跑原版 → dlopen 后 dump 18 地址 72B
- 工具三件套:`frida-dump/redroid_jni_dump.py` + `redroid_dump_jni.sh` + `gen_runtime_table.py`

## 修正 7:军棋"能玩"的真机边界

- redroid 无 GPU(只能 dump 数据,不能验证"能玩")
- MuMu Android 12 + Adreno 640 真 GPU:arm64 影子库稳定运行不崩(90s+),**但游戏板未渲染(黑屏)**——对象图分歧
- "运行不崩" ≠ "能玩"(军棋 AGENTS.md 红线)

## 修正 8:★ 卡点描述更新(§124 纠正,AGENTS.md 2026-11-01 第二次更新)

**重要纠正**:之前课程讲义里的卡点描述停在"② C++ 对象图 + ① 18 个 .text 常量",
但 §124 做了两个关键纠正:

1. **"BL off-by-2 -0xa"是误读**:之前 checkpoint 里看到的"BL off-by-2 -0xa"
   (0x10000020 → 0x10000004)实际是**误读**——`F000 F800` 这条 BL 在**隔离环境**
   (单独跑 0x43ed8,无 so 加载)里算出的目标 0x10000004 是**正确的**。
   **加载 .so 后**才会出现上下文相关错位:BL 0x7965a 在加载 .so 后被 unicorn2
   解码成目标 0x6eaea(memset 循环,非函数入口),而真实目标是 0x43ed8(std::vector 构造)。

2. **剩余阻塞从 1 项变 3 项**(§124 重新梳理):
   - **① 上下文相关 BL 解码 bug**(新发现,需 workaround):
     加载 .so 后,unicorn2 对 0x7965a 的 `F000 F800` 解码结果随上下文变化
     (0x43ed8 → 0x6eaea),是**模拟器精度 bug**(非翻译/逻辑 bug)。
     待确认:换 dynarmic 后端 / code hook patch 目标 / unicorn2 版本修复。
   - **② C++ 对象图语义缺失**(原有):std::vector 0x43ed8 的 begin/end/capacity 未初始化
   - **③ 288MB PROT_NONE mmap**(新发现):unidbg mmap2 对 288MB PROT_NONE 处理失败

**课程对齐**:
- 所有提到"剩余阻塞"的章节(ch02/ch05/ch22/ch25/ch26/ch31)已改为 3 项阻塞
- "下一轮 unidbg 主攻方向"改为 3 步:先攻 ①(换后端/code hook)→ 再补 ②(vector 3 指针)→ 最后 ③(PROT_NONE mmap)

**诚实边界**:这 3 项阻塞都不影响"指令语义 100% 对齐"的结论,但"能跑"≠"能玩"

## 课程章节修正清单(逐章扫一遍)

| 章节 | 修正项 | 状态 |
|---|---|---|
| Ch1 | `AndroidEmulator` 表 unicorn2 表述 + 14 个 On* | ✅ 已改 |
| Ch4 | `.dynsym` 14 个 On* | ✅ 已改 |
| Ch5 | PC off-by-2 修复 + JNIEnv/VM 补环境实证 | ✅ 已改 |
| Ch7 | 同上 | ✅ 已改 |
| Ch8 | 依赖库 6→7(加 liblog) | ✅ 已改 |
| Ch10 | SvcMemory 描述 | ✅ 正确 |
| Ch11 | 7 个 Java 类 + 14 个 On* | ✅ 已改 |
| Ch12 | 14 JNI 入口 | ✅ 正确 |
| Ch13 | JNI 注册 14 个 + On* 调用 14 个 + 导出符号 14 个 | ✅ 已改 |
| Ch14 | loader 7 依赖 + DT_NEEDED 7 个 | ✅ 已改 |
| Ch15 | Hook 场景 | ✅ 正确 |
| Ch16 | I/O 路径(7 个 config XML) | ✅ 正确 |
| Ch17 | Debugger 自吐 | ✅ 正确 |
| Ch18 | 指针参数 + AES 识别 | ✅ 正确 |
| Ch19 | SPAK key 32B / 4 组 883 条目 + 14 个 On* | ✅ 已改 |
| Ch20 | 内存数据动态验证 + 14 个 On* | ✅ 已改 |
| Ch21 | fork 场景(广告 SDK) | ✅ 正确 |
| Ch22 | 补环境 3 大类 | ✅ 正确 |
| Ch23 | 标识记录 2 案例 + 14 个 On* | ✅ 已改 |
| Ch24 | 设备风控 | ✅ 正确 |
| Ch25 | 补环境加强 7 步 + §25.4.1 下一轮主攻(C++ 对象图补全) | ✅ 已加 |
| Ch26 | 补环境总结 + §26.4.1 下一轮主攻(C++ 对象图补全) | ✅ 已加 |
| Ch27 | 环境变量检测 | ✅ 正确 |
| Ch28 | xHook 检测 + DT_NEEDED 7 个 | ✅ 已改 |
| Ch29 | FindClass / methodID 反制 | ✅ 正确 |
| Ch30 | 8 维度检测矩阵 + 14 入口 + 14 个 On* | ✅ 已改 |
| Ch31 | Spring Boot 部署 | ✅ 正确 |
| INDEX | 军棋对照表:14 个 On* + 7 个 DT_NEEDED + C++ 对象图补全 | ✅ 已改 |

## 修正 9:14 个 JNI 入口的真实命名(readelf 实测,重要)

**课程原写**(凭想象,有误):"OnCreate / OnDestroy / OnUpdate / OnResize / OnKey /
OnTouch / OnEvent / OnGraphChange / OnBluetooth / OnPause / OnResume / OnQuit /
OnSave / OnRestore" —— 其中 **OnResize / OnQuit / OnSave / OnRestore 实际不存在**。

**实际(readelf -W --dyn-syms libjunqi.so 实测)**:
```
Java_com_cnvcs_App_OnCreate
Java_com_cnvcs_App_OnDestroy
Java_com_cnvcs_App_OnResult      ← 课程原写 OnSave
Java_com_cnvcs_App_OnResume
Java_com_cnvcs_App_OnEvent
Java_com_cnvcs_App_OnKey
Java_com_cnvcs_App_OnUpdate
Java_com_cnvcs_App_OnGraphChange
Java_com_cnvcs_App_OnPause
Java_com_cnvcs_App_OnBluetooth
Java_com_cnvcs_App_OnGraphCreate ← 课程原写 OnResize
Java_com_cnvcs_App_OnTimer       ← 课程原写 OnQuit
Java_com_cnvcs_App_OnInput
Java_com_cnvcs_App_OnTouch
```

**已修正章节**:ch12(§12.2 入口列表)、ch13(§13.1 导出符号列表)、ch24(§24.4 改用 OnTimer)
**全局复查**:`grep -rhoE "On[A-Z][A-Za-z]*"` 统计,不存在的名字(OnResize/OnQuit/OnSave/OnRestore)= 0

## 修正 10:18 个 .text 常量 = 15 .text + 2 .data(共 72B)

- 课程 ch02 §2.3 原写"18 个地址"(含糊),已改为"15 .text + 2 .data(共 18 地址 × 4B = 72 字节)"
- 与 AGENTS.md §R6 红线一致:15 .text + 2 .data = 18 地址 72B,仅 dlopen 重定位填

## 修正 11:统一通俗化改写(用户要求:讲明白痛点 + 通俗易懂 + 为什么这样做)

- 31 章讲义全部按「痛点 → 大白话类比 → 为什么这样做 → 军棋实测验证 → 一句话小结」结构改写
- 改写规范:`_REWRITE_SPEC.md`(保留项:配图/视频路径 + 军棋实测数据 + 红线引用)
- 第 4-15 章由 subagent 批量改写,第 1-3 / 16-31 章手动改写
- 保留数据:12 init_array / 5312 RELATIVE / 214 PLT / 6 GLOB_DAT / 14 JNI 入口 /
  7 DT_NEEDED / 0x43ed8 / 0x159880 / 0x233e1 / unicorn2 / redroid / 红线 §R6
