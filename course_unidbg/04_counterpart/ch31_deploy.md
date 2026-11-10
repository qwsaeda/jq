# 第31章 unidbg 生产环境部署

> 课程配套：《unidbg 逆向工程：原理与实践》陈佳林（机工社 2024）
> 本地对照：军棋 32 位 so 的生产部署

## 痛点：为什么"能调一个函数"还不够，要"部署成服务"？

你在 PC 上手工调通军棋 14 个 JNI 入口（OnCreate / OnDestroy / OnResult /
OnResume / OnEvent / OnKey / OnUpdate / OnGraphChange / OnBluetooth /
OnPause / OnGraphCreate / OnTimer / OnInput / OnTouch），
但"调通一个"到"稳定批量服务"之间隔着 3 个卡点。

### 痛点 1：手工调试不可复用

每次都要重新跑一遍 AndroidEmulator + DalvikVM + 补环境 7 步 + code cave
+ vecfix + JunqiJni handler，改个参数又从头来。

- **大白话卡点**：军棋补环境链很长（7 步框架 + 3 个加强项 + 18 常量注入），
  手工每次重建一遍，参数一改全量重跑。
- **为什么卡**：手工 harness 是一次性脚本，没有"服务化"封装。
- **不解决会怎样**：批量回放 1000 局 `one.sjg` 要手工跑 1000 遍，
  没复用、没并发、没统一入口。

### 痛点 2：批量跑不了

你想"批量回放 1000 局对局存档（one.sjg），看每局结果"，手工一次只跑一局，
1000 局要一天。

- **大白话卡点**：手工是"单线程串行"，批量要"并发 + 队列"。
- **为什么卡**：unidbg 模拟器实例不能随意共享（每实例有自己的内存空间、
  CPU 状态、JNI 环境），批量要么每局 new 一个实例（慢），要么
  实例池化（要管理生命周期）。
- **不解决会怎样**：批量分析没有可扩展性，"算胜率/复盘"等下游全卡住。

**预判追问："一个模拟器实例能同时服务多个请求吗？"** 不能直接共享——
unidbg 实例内部状态（寄存器/内存/JNI 全局对象表）是单"线程"语义的，
并发请求会互相污染（军棋机械回归的教训：fork Pool `pool.map` 死锁、
C 端 subprocess 必须加 8s 超时防 hang）。生产部署走"实例池"：
每个请求分配一个 fresh 实例（或"用完销毁"），请求结束回收。

### 痛点 3：结果没法给别人用

你调出来的"board 60 格 + turn + winner"是中间态，下游（"算胜率"
"复盘分析"）拿不到稳定的 API 调用。

- **大白话卡点**：中间态在内存里，不"端出去"（REST/IPC）下游拿不到。
- **为什么卡**：手工 harness 的结果是"打印在控制台"，没有稳定契约。
- **不解决会怎样**：每个下游各自写一套 unidbg harness，重复造轮子 +
  补环境代码漂移（A 补了 vecfix，B 没补，结果不一致）。

## 为什么这样做：把 unidbg 包成 REST 服务

类比：unidbg 是"厨房"，你能在厨房做菜（手工调试），但**做菜本身不是
目的，"把菜端出去"才是**。Spring Boot + unidbg-boot-server 就是
"把厨房变成外卖窗口"——别人（下游服务）POST 一份订单（对局存档），
你返回"成品菜"（board/turn/winner）。

军棋生产部署 = Spring Boot（外卖窗口） + unidbg（厨房） + 14 个 JNI 入口（菜品）：

```
POST /junqi/onCreate  ← 对局存档 + 布局选择
  → unidbg 跑军棋 so 的 OnCreate
  → 返回 { board: 60格, turn, stage, winner }
```

**预判追问："unidbg 怎么填 `.got` 值？服务化之后填法变了吗？"**
没变，且这是"值来源"问题不是"部署形态"问题（22/30 章同口径）：
- `.got`（214 PLT + 6 GLOB_DAT）：每个 unidbg 实例 `loadLibrary` 时
  自动重定位填好（填模拟侧桩地址），服务化后每实例独立填，行为一致。
- 18 个 `.text` 常量 + 2 `.data`（72B）：真值来自 **redroid 跑原版 so
  后 dump**（真实 32 位 libc 重定位结果，不是 unidbg 算的），
  注入 `jq_load_runtime_values` 表，服务启动时加载一次即可（静态表，
  全实例共享只读）。
- vector 的 begin/end：每实例 vecfix 填 scratch（25 章，逐调用安全阀）。

**预判追问："那换 arm64 宿主部署有没有用？"** 对军棋 32 位 so 没用——
军棋 so 是 **ELF32**（`readelf -h` 实测 ELF32/ARM/EABI5），"指令位宽
vs ELF 数据位宽"两个独立维度（28 章追问的结论）：换 arm64 宿主
只改指令位宽（CPU 模拟器能跑 64 位指令），不改 ELF 数据位宽（军棋 so
永远是 32 位数据格式，64 位 unidbg 加载它直接架构不符被拒）。
服务化也一样：32 位 so 就配 32 位 unidbg 实例，宿主是 x86 还是 arm64
不影响"so 是 ELF32"这个事实。

## 31.1 Spring Boot + unidbg 结合（军棋 7 步启动 + 3 加强项）


> **本节核心**：
> 1. Spring Boot + unidbg 7 步启动 + 3 加强项
>
> **本节难点**：
> 1. 选后端→配库→挂桩→建 VM→调入口→填常量→部署
> 2. 加强项 = 批量/异步/日志
> 3. 「7 步 + 3 加强项」是 unidbg 部署方法论

![军棋 unidbg-boot-server 架构](assets/ch31_boot_server.png)



```
Spring Boot 启动
  1. 创建 AndroidEmulator(unicorn2 后端 + AndroidResolver(23))   ← 选对后端(修 PC off-by-2)
  2. 创建 DalvikVM(com.cnvcs.junqi)                            ← 模拟 Java 侧
  3. loadLibrary(libjunqi.so, init)                            ← 跑 12 init_array + 重定位
       (注意: §124c — 跑 init_array 后 bx bug, 生产走 no-init_array 路径 + code cave)
  4. 挂桩 libGLESv1_CM / libGLESv2                             ← 无真 GPU
  5. 注入 18 个 .text 常量池 + 2 个 .data 值                   ← redroid dump 72B(jq_load_runtime_values)
  6. Hook __cxa_atexit(返回 0) + code cave(0xc3a68) + vecfix + JunqiJni handler
  7. 对外暴露 REST 接口(14 个 JNI 入口)                          ← 外卖窗口
```

**逐环节"我们能干什么 / 不能干什么"**：

| 环节 | 能干什么（✓） | 不能干什么（✗） |
|---|---|---|
| **Spring Boot 封装** | 7 步初始化 + REST 暴露 14 入口，实例池化并发 | 共享单实例并发（unidbg 实例单"线程"语义，并发会污染状态） |
| **批量调 14 入口** | 每请求 new/取实例池 → 调 `OnCreate`/`OnUpdate`/`OnBluetooth`... → 回收 | 批量"能玩"（`OnCreate` 只到 ~90%，§124g 最后 10% 全局未初始化挡住；14 入口可"调"，结果不是完整对局） |
| **值注入** | 18 常量表（redroid dump）启动加载一次 + 每实例 vecfix | 让 unidbg"算出"18 常量真值（值来自真实 32 位 libc 重定位，非模拟值） |
| **结果契约** | 稳定返回 {board, turn, stage, winner}（中间态快照） | 把"中间态快照"当"完整对局结果"（指令语义 100% 对齐 ≠ 功能一致，§122 口径） |

## 31.2 unidbg-boot-server 配置（军棋 application.yml）


> **本节核心**：
> 1. 军棋 application.yml（unidbg-boot-server）
>
> **本节难点**：
> 1. backend=unicorn2 / for32Bit / AndroidResolver(23) / 挂桩 GL / 18 常量注入
> 2. 「配置」是 unidbg 部署参数
> 3. 军棋 application.yml 全参数对齐红线 §R6 实测

```yaml
# application.yml
unidbg:
  arch: arm32
  backend: unicorn2        # 军棋实证修复 PC off-by-2(DiagTracePC 0xa37fc=0x159880)
  resolver: android-23     # 提供 libc/libstdc++/libm/liblog/libdl(军棋 7 依赖里前 5 个)
  process-name: com.cnvcs.junqi
  so-path: lib/armeabi/libjunqi.so
  jni-entries:             # 14 个 JNI 入口(军棋导出, readelf 实测名)
    - Java_com_cnvcs_App_OnCreate
    - Java_com_cnvcs_App_OnDestroy
    - Java_com_cnvcs_App_OnResult
    - Java_com_cnvcs_App_OnResume
    - Java_com_cnvcs_App_OnEvent
    - Java_com_cnvcs_App_OnKey
    - Java_com_cnvcs_App_OnUpdate
    - Java_com_cnvcs_App_OnGraphChange
    - Java_com_cnvcs_App_OnBluetooth
    - Java_com_cnvcs_App_OnPause
    - Java_com_cnvcs_App_OnGraphCreate
    - Java_com_cnvcs_App_OnTimer
    - Java_com_cnvcs_App_OnInput
    - Java_com_cnvcs_App_OnTouch
```
## 31.3 军棋 REST 接口示例


> **本节核心**：
> 1. REST 接口示例（POST /junqi/onCreate → 调 OnCreate → 返 GLView 对象）
>
> **本节难点**：
> 1. 「REST」是 unidbg 服务化
> 2. 军棋 14 入口逐个 REST 化
> 3. GLView 对象 JSON 序列化返客户端

```
POST /junqi/onCreate
{
  "layout": 0,           // data/lay/00.lay(5 空 + 19 普通 + 2 炸 + 3 雷 + 1 旗)
  "oneSjg": "<base64>"   // 对局存档(board 60 格 + turn/stage/winner + flags/mines/pieces + moves)
}
→ 200 OK {
  "board": [60 cells],   // 棋盘 60 格(38 子布局)
  "turn": 0,
  "stage": "init",
  "winner": null
}
```
**注意（诚实边界）**：返回的"board 60 格 + turn + winner"是 **OnCreate
推进到 ~90% 时的中间态快照**（§124f：GLView 创建 + EGL 配置 + setContentView
+ registerReceiver×3 全通），不是"完整跑完 OnCreate 返回 GLView"的结果
（最后 10% 被 §124g 全局未初始化挡住）。部署物里要标注"中间态快照"，
不要把 ~90% 的返回包装成"完整版 OnCreate"。
## 31.4 生产部署诚实边界（军棋红线 §R6 + §124d-g）


> **本节核心**：
> 1. 生产部署诚实边界（红线 §R6 + §124d-g）
>
> **本节难点**：
> 1. 军棋 31 章部署「能调一个函数」≠「能玩」，§124 三项阻塞仍卡
> 2. 「诚实边界」是部署不包装成完整版
> 3. 需 redroid + 真 arm64 GPU 设备验收（非 unidbg 能解）

军棋"能批量调 14 个 JNI 入口" ≠ "完整能玩"：
- **指令语义 100% 对齐**（红线 §122：可跑函数 1174/1174 PASS + 副作用
  crc32 比较，5 个构造输入空间内的一致率，不宣称任意输入/功能一致）
- **§124d-g 里程碑**：`OnCreate` 推进到 ~90%（no-init_array + code cave
  + vecfix 37 次 + JunqiJni 37+ JNI 调用 + GLView/EGL/setContentView/
  registerReceiver×3），最后 10% 被全局未初始化挡住（§124g：
  0x23b7a READ_PROT，LR=垃圾 0x39a63305 → 垃圾函数指针调用链）
- **剩余阻塞链**（部署后依然卡）：init_array 后 bx bug（§124c，模拟器
  深层 bug）+ no-init_array 全局未初始化（§124g）+ 288MB PROT_NONE
  mmap（DiagJni5）
- 完整"能玩"需**真实 arm64 GPU 设备验收**（redroid 无 GPU，只能 dump
  数据；MuMu 影子库稳定运行但游戏板黑屏——数据与指令对齐分歧）
- 军棋 64 位简化版（`rev64/src/junqi.cpp`）是简化重写（10×9 棋盘、
  颜色棋子、自动走子、GLES2 渲染），**非原版 1.43MB/1435156B native 的汇编级逆向**；
  SPAK 解密 key 已提取但资源图为占位，玩法/规则/资源与 32 位原版不一致
**不要在交付物里把"简化版/对齐版"说成"完整版"**（军棋红线）：
生产部署的 REST 接口返回"中间态快照"，文档必须标注：
"unidbg 模拟下 `OnCreate` ~90%（§124f-g），最后 10% 全局未初始化
未突破；'能调' ≠ '能玩'，完整验收需真机 GPU"。
## 31.5 本章小结（一句话版）

> 生产部署 = 把 unidbg"厨房"包成"外卖窗口"（Spring Boot REST + 实例池）。
> 军棋 7 步：选 unicorn2 → DalvikVM → loadLibrary → 挂桩 GL →
> 注入 18 常量（redroid dump 72B）→ Hook cxa_atexit + code cave +
> vecfix + JunqiJni → 暴露 14 个 JNI 入口。
> 能批量调 ≠ 能玩（§124d-g：OnCreate ~90%，最后 10% 全局未初始化 +
> 288MB mmap；"指令 100% 对齐" ≠ "功能一致"，§122 口径）。
> 换 arm64 宿主对 32 位 so 没用（ELF 数据位宽不变，军棋 so 永远 ELF32）；
> 18 常量真值来自 redroid dump 的真实 32 位库，不是 unidbg 算的。


> **延伸阅读（附录 C.5 军棋"能玩"验收判据拆解）**：本章讲"部署"，军棋的统一验收判据"装上→进主界面→下一局→正常退出→重进不崩"的**逐项现状实证**（R-route 正向开发，`src_rs/`）在 `APPENDIX_C_REVERSE_ENGINEERING.md` C.5——**"重进不崩"已闭环（force-stop→重启→走 1 回合→AI 应着，×2 无崩溃），但"GL 出屏/音效出声/游戏内重开"是真机环境边界**（不包装成"可玩"）。详见 C.5。

## 视频
