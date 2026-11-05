# 课程索引：《unidbg 逆向工程：原理与实践》31 章配套课程

> 讲师：基于陈佳林《unidbg 逆向工程：原理与实践》（机工社 2024）
> 实战靶机：军棋 `lib/armeabi/libjunqi.so`（ELF32 / ARM / EABI5 / stripped，红线 §R6 已实证）

## 这本课程怎么读（痛点导读）

每个章节开头都有「痛点：为什么 X?」段，先讲清楚"卡在哪、为什么卡"，
再用大白话 + 生活类比讲"为什么这样做"，最后用军棋 `libjunqi.so` 的真实卡点 / 实测数据验证。
全课程主线就一个痛点：**军棋 so 是 ARM 二进制，你的 PC 是 x86，怎么在 PC 上跑通它、看懂它、批量调它。**

四大痛点对应四部分：

| 部分 | 核心痛点 | 章节 |
|---|---|---|
| 第一部分（入门） | ARM so 在 PC 上跑不了，怎么"搬进模拟房间"？ | 1-3 章 |
| 第二部分（原理） | "搬进来"后，加载/链接/初始化到底怎么跑的？崩在哪？ | 4-15 章 |
| 第三部分（实战） | so 读文件/调 Java/查设备，缺的"环境"一个个怎么补？ | 16-26 章 |
| 第四部分（反制+部署） | so 反过来查"你是不是模拟器"，怎么反？怎么部署成服务？ | 27-31 章 |

## 目录结构

```
course_unidbg/
├── 01_basics/                  # 第一部分 进入 unidbg 的世界（Ch 1-3）
│   ├── ch01_env_setup.md       # 第1章 环境准备与快速上手
│   ├── ch02_sim_exec.md        # 第2章 模拟执行初探
│   └── ch03_hook_patch.md      # 第3章 补环境、Hook 与 Patch
├── 02_principle/               # 第二部分 unidbg 原理（Ch 4-15）
│   ├── ch04_elf_view.md        # 第4章 ELF 文件执行视图解析
│   ├── ch05_unicorn_linker.md  # 第5章 Unicorn 初级使用与初探 Linker
│   ├── ch06_linker_deep.md     # 第6章 深入 Linker：so 加载、链接、初始化
│   ├── ch07_sim_linker_load.md # 第7章 模拟 Linker：so 的加载过程
│   ├── ch08_sim_linker_link.md # 第8章 模拟 Linker：so 的链接过程
│   ├── ch09_r0dbg_findkey.md   # 第9章 R0dbg 实战与 Unidbg_FindKey
│   ├── ch10_emulator.md        # 第10章 源码解析：AndroidEmulator
│   ├── ch11_dalvikvm.md        # 第11章 源码解析：DalvikVM
│   ├── ch12_sim_trace.md       # 第12章 源码解析：模拟执行流程追踪
│   ├── ch13_jni_trace.md       # 第13章 源码解析：JNI 交互流程追踪
│   ├── ch14_memory.md          # 第14章 源码解析：Memory
│   └── ch15_hook.md            # 第15章 源码解析：Hook
├── 03_practice/                # 第三部分 模拟执行与补环境实战（Ch 16-26）
│   ├── ch16_io_redirect.md     # 第16章 实战：I/O 重定向
│   ├── ch17_debugger_selfdump.md # 第17章 实战：Debugger 自吐
│   ├── ch18_pointer_args.md    # 第18章 实战：指针参数与 Debugger
│   ├── ch19_base64.md          # 第19章 实战：魔改 Base64 还原
│   ├── ch20_mem_data.md        # 第20章 实战：动态分析内存中的数据
│   ├── ch21_fork.md            # 第21章 实战：主动调用 fork 进程
│   ├── ch22_env_basics.md      # 第22章 补环境入门
│   ├── ch23_identifier.md      # 第23章 补环境：标识记录
│   ├── ch24_device_risk.md     # 第24章 补环境：设备风控
│   ├── ch25_env_enhance.md     # 第25章 补环境加强
│   └── ch26_env_summary.md     # 第26章 补环境总结
├── 04_counterpart/             # 第四部分 反制与生产环境部署（Ch 27-31）
│   ├── ch27_env_detect.md      # 第27章 Anti-unidbg：环境变量检测
│   ├── ch28_xhook.md           # 第28章 Anti-unidbg：xHook 检测
│   ├── ch29_jni_reversal.md    # 第29章 Anti-unidbg：JNI 层常见函数处理
│   ├── ch30_detect_summary.md  # 第30章 Anti-unidbg：unidbg 常规检测总结
│   └── ch31_deploy.md          # 第31章 unidbg 生产环境部署
├── assets/                     # 配图（AI 生成，agnes-image-2.5-flash）
│   ├── ch01_unidbg_architecture.png
│   ├── ch01_load_flow.png
│   ├── ...（每章 1-2 张）
└── videos/                     # 讲解视频（AI 生成，agnes-video-2.5-flash）
    ├── ch01_intro.mp4
    ├── ch02_intro.mp4
    ├── ...（每章 1 段 ~10s 视频）

# 配套参考文档（非 31 章，独立讲义）
├── REFERENCES.md               # 资料全景（7 层 + 4 本书 + Frida 3 本）
├── ELF_FORMAT_SUPPLEMENT.md    # ELF 格式补充讲义（4 本书核心知识点，补 ch04/06/10/15）
├── APPENDIX_A_ANDROID_GAME_DEV.md  # 附录 A：安卓游戏开发基础（逆向视角速查，补"so 本身是什么"）
├── APPENDIX_B_ELF_BUILDING.md      # 附录 B：ELF 正向生成（C/C++/Rust 三个真实 so 布局对比，纠正".got 全0"错误）
├── APPENDIX_C_REVERSE_ENGINEERING.md  # 附录 C：逆向工程化方法论（代码缺口 vs 数据缺口 + Frida 边界 + 审计工具三件套 + 增强循环 + 验收判据拆解）
├── CHAPTER_DIFF_REPORT.md      # 7 处原书偏差修正记录
├── CORE_CHALLENGES.md          # 31 章核心+难点速查 + 5 大跨章卡点 + 两条路对照
└── INDEX.md                    # 课程索引 + 军棋实测对照表
```

## 课程约定

- **讲义**：每章 1 篇 markdown（约 50-150 行），含 3-6 节
- **配图**：每章 1-2 张 PNG（AI 生成，agnes-image-2.5-flash，2K 16:9）
- **视频**：每章 1 段 MP4（AI 生成，agnes-video-2.5-flash，10s 16:9）
- **军棋对照**：每章末尾把书内通用原理映射到军棋 32 位 so 的实测结果（红线 §R6）

## 军棋 32 位 so 的实测结果（课程引用清单）

来自 `/disk/workspaces/jq/AGENTS.md` 红线 §R6：

| 实测项 | 结果 | 对应章节 |
|---|---|---|
| 12 个 `.init_array` 全通 | ✓ | Ch 7 / 8 / 10 |
| 5312 个 RELATIVE 重定位全通 | ✓ | Ch 6 / 8 |
| 214 个 PLT→GOT→符号映射全通 | ✓ | Ch 6 / 8 |
| 6 个 GLOB_DAT 符号从 docker 真 ARM libc 解析 | ✓ | Ch 8 |
| `.got` 零值槽（未重定位） | 已证 | Ch 4 / 14 / 15 |
| PC off-by-2 修复（unicorn2 后端） | ✓ DiagTracePC 0xa37fc = 0x159880 | Ch 5 / 7 / 30 |
| JNIEnv/VM 补环境实证可行 | ✓ DiagJni 推进到 std::vector 对象图 0x43ed8 | Ch 11 / 12 / 13 / 22 |
| 14 个 JNI 入口 + 14 个 On* 静态方法（`Java_com_cnvcs_App_On*`） | ✓ | Ch 2 / 12 / 13 / 30 / 31 |
| 7 个 DT_NEEDED 依赖库（含 liblog） | ✓ | Ch 8 / 14 |
| 18 个运行时地址（15 `.text` 常量池 + 2 `.data`；★§124z-3 后改**静态位移**固化在 `jqt_mem.c`，无需 redroid dump） | 已证 | Ch 9 / 17 / 19 / 5(5.4.2) |
| `.rodata` 明文 118KB（静态即明文，无自解密） | 已证 | Ch 4 / 20 |
| `0x138xxx` 高熵索引表（非加密代码） | 已证 | Ch 4 / 19 |
| `FindClass("com/baidu/mobads/AdView")` 广告层引用 | 拆除目标 | Ch 15 / 29 |
| 剩余阻塞（§124 纠正后 3 项） | ① 上下文相关 BL 解码 bug（unicorn2 精度，BL 0x7965a→0x6eaea）+ ② 数据与指令对齐对应缺口（std::vector begin/end）+ ③ 288MB PROT_NONE mmap（unidbg mmap2 处理失败） | Ch 5 / 6 / 11 / 12 / 22 / 25 / 26 / 31 |
| ★ 下一轮主攻：C++ 对象图补全（std::vector begin/end/capacity 补初始值） | 教程待办 | Ch 25 / 26 |
| SPAK 解密 key（32B，内嵌 `.rodata`） | 已 dump | Ch 19 / 20 |
| 对局存档 `one.sjg`（60 格棋盘） | 已逆向 | Ch 16 / 23 / 24 |
| **★ 32→32 翻译体（§124u-z 突破 + §〇d-2 gate-set 后续）**：32 位 ARM → 32 位 C → armv7a 编译回 32 位 so → MuMu 渲染主菜单 UI 层；**gate-set 直接置位闸口字节 → F_76b14 全链走通（破写侧数据缺口一层，工具 patch_gate_chain.py 幂等）** | 实测（GL 棋盘区未出帧，**slot22 纹理创建仍 0 命中** = 对象图"数据"未填充，gate-set 突破部分但完整构造链未通） | Ch 1(1.4.3★) / 5(5.4.2) / 附录 C.1(gate-set 第三路) |
| **★ unidbg+Frida 一对一逐步对齐（§124z 黑屏破题方向）**：unidbg 模拟加载+调用链+逻辑，Frida（MuMu 32 位 frida-server）真实填充对象图真值，逐字段对齐 | 对象图对齐 = glClear 类对象构造 → 渲染出帧 | Ch 13(13.4c) / `frida-dump/FRIDA_NOTES.md` §八 |

## 课程推进约定（军棋红线）

- **不在交付物里把 arm64 简化版说成"完整版"**（红线 §64 位 native 的诚实边界）
- **指令语义 100% 对齐 ≠ 能跑 ≠ 能玩**（红线 §122 诚实降级）
- 军棋 32 位 so 的 unidbg 路线**不是壳**（`.rodata` 静态即明文，无自解密例程）
- 军棋 64 位"能玩"需真实 arm64 GPU 设备（redroid 无 GPU / 64 位 native bridge 不完整）
- **★ 路线纠偏（§124u-z）**：32→64 翻译体黑屏（对象图错位，死路）；**32→32 翻译体渲染主菜单 UI 层（活路，ch01 1.4.3★ / ch05 5.4.2）**——"能渲染 UI"≠"能玩"（GL 出帧 + 触摸进对局还差）
- **★ 18 常量路线演进（§124z-3）**：redroid dump 绝对值（旧）→ **静态位移固化 `jqt_mem.c`（新，跨进程通用）**，`jni_runtime_values.json` 已删（见 `frida-dump/FRIDA_NOTES.md`）

## 课程三条路对照（unidbg / 翻译体 / R-route 正向重写，2026-11 更新）

军棋"把 so 的逻辑跑起来 / 还原"有**三条路**，课程全覆盖（前两条是"跑原版 so"，第三条是"重写逻辑"）：

| | unidbg（模拟执行原版） | 翻译体（32→32 重编译跑原版逻辑） | **R-route（正向重写逻辑，`src_rs/`）** |
|---|---|---|---|
| **路线** | x86 宿主 + unicorn2 动态翻译 32 位 ARM | 32 位 ARM → 32 位 C → armv7a 编回 32 位 so | **反汇编出逻辑 → 重写成可读 C（行为级等价）→ 64 位 so** |
| **对象图** | 32 位原样保留（但 unidbg 模拟不出数据与指令对齐） | 32 位布局原样保留 | **不用原版对象图**（重写的 C 容器，自己构造） |
| **军棋现状** | 卡 §124 三项阻塞（BL bug/数据与指令对齐/mmap） | MuMu 渲染 UI 层；全量 4245 场景闭包已翻译，**但 slot22（棋盘纹理创建）0 命中，画不出棋盘**（对象图数据未填充） | **5549 行 C + 12/12 回归全绿 + 真机走子 EAT/BOMB/音效实证**（规则引擎跑通，渲染/出屏/蓝牙桩） |
| **能到哪层** | ③ 指令 1:1（卡 ④ 数据与指令对齐） | ③ 指令 1:1 + ④ 部分（渲染 UI，卡对象图数据） | **③④ 行为级等价（规则/走子/吃子/音效跑通），⑤ 源码原貌不出** |
| **课程落点** | Ch 4-15 + 16-31 | Ch 1(1.4.3★) + Ch 5(5.4.2) + `frida-dump/FRIDA_NOTES.md` | **附录 B.5c.3b（R-route）+ CONSISTENCY.md（一致测试口径）** |
| **诚实边界** | ① BL bug ② 数据与指令对齐 ③ 288MB mmap（三墙） | "补 stub"（30/2/全量 4245）**都破不了 slot22**（瓶颈=运行时 C++ 对象图数据，非代码）；frida dump .bss 值可提但**跨进程 scudo 指针不可回填** | ⑤ 源码原貌逆不出；GL 出屏/音效出声/蓝牙需真机 arm64 GPU/音频/蓝牙验收 |

**三条路的关系（关键认知）**：
- **unidbg / 翻译体 = "跑原版 so"**（对象图是原版的，模拟/重编译都保真 32 位布局，但**数据与指令对齐填充**是共同天花板——unidbg 模拟不出、翻译体补 stub 补不出）
- **R-route = "不跑原版 so，重写逻辑"**（绕开对象图天花板——重写的 C 自己构造容器，不依赖原版 vtable/对象图）；代价是**"行为级等价"而非"原版指令 1:1"**（⑤ 源码原貌仍逆不出）
- **"画棋盘/能玩"的真机验收**：三条路都撞同一面墙——**真机 arm64 GPU 设备**（MuMu GLES1 几何出帧受限，PBuffer 离屏已证 GLES2 通路，window surface 未通）

**工程化方法论（会话 2 沉淀，防"反复踩同一坑"）→ 附录 C**：
- **代码缺口 vs 数据缺口**（"补 stub 破不了对象图"，全量 4245 实证）→ `APPENDIX_C_REVERSE_ENGINEERING.md` C.1
- **frida dump 路线边界**（跨进程 scudo 指针不可回填）→ C.2
- **机械化审计工具三件套**（`link_audit.py`/`jni_method_audit.py`/`tex_pipeline.py` + 走子三件套）→ C.3
- **增强循环方法论**（8 坑 3 类 + 防复发）→ C.4
- **军棋"能玩"验收判据拆解**（重进不崩闭环）→ C.5

**会话 2（翻译体）方向定论（吸进课程的核心教训）**：
- **"补 stub"不是对象图缺口的解**——全量 4245 场景闭包翻译（代码 100% 对齐）+ 渲染器单例构造 + glClear 每帧，**slot22 仍 0 命中**（帧循环只调 slot23 清理，slot22 只在场景初始化调一次，而场景对象图数据未填充）
- **插桩优先于静态追链**——静态分析"可能活路径"的 F_77840/F_77b10 插桩 0 次（不在活路径），插桩 F_23738（场景对象 slot11）确认活路径
- **RPI 公式修正**（`pool = ((addr+4) & ~3) + disp`，Thumb PC=addr+4）——旧公式差 4 字节造成 4078"超边界"假象，真缺口只有 57 个 .bss 槽
- **frida dump 边界**：能 dump .bss/.data 的值（含 load_base），但**跨进程 scudo 堆指针不可回填**（dump 是"值"不是"对象图"）

---

## 生成工具

- 生图脚本：`/disk/workspaces/treasureIsl/scripts/agnes_gen.py`（agnes-image-2.5-flash）
- 生视频脚本：`/disk/workspaces/treasureIsl/scripts/agnes_video.py`（agnes-video-2.5-flash）
- 生图 API：`https://apihub.agnes-ai.com/v1/images/generations`
- 生视频 API：`https://apihub.agnes-ai.com/v1/videos`（异步：创建 → 轮询 → 下载）
- API KEY：环境变量 `AGNES_API_KEY`（不写脚本/不入库）

## 课程使用建议

- **入门（Ch 1-3）**：跑通一个 unidbg 项目，加载军棋 32 位 so，理解"符号调用 / 地址调用"
- **原理（Ch 4-15）**：ELF 执行视图 + Linker 原理 + unidbg 源码 + JNI 交互
- **实战（Ch 16-26）**：I/O 重定向 + Debugger 自吐 + 补环境 4 大场景
- **反检测 + 部署（Ch 27-31）**：xHook 检测 + 8 维度常规检测 + Spring Boot 生产部署

每章配套：讲义 1 篇 + 配图 1-2 张 + 讲解视频 1 段（~10s）

## 参考资料全景（不只对齐一本书）

课程主对齐原书（陈佳林《unidbg 逆向工程》），但**最权威一手来源是 unidbg 官方代码库 + 作者 r0ysue 博客**（r0ysue 是 unidbg 维护者）。详见 `REFERENCES.md`：

- **① 官方代码库** `r0ysue/unidbg`（GitHub）：第 4-15 章讲源码的**真源**（jelf/AndroidEmulator/DalvikVM/Memory/Hook）
- **② 作者配套课件** `r0ysue/UnidbgBook`（GitHub，Chap01-31）：原书配套代码示例
- **③ 作者博客** r0ysue 博客园（Unidbg 文档慢更 / Linker 源码分析 / Unicorn 底层）：第 5-9 章一手原理（比原书细）
- **④ 官方 AutoDocs** unidbg 仓内 `_autodocs/`（context7.com 收录）：API 级参数签名
- **⑤ 社区实战文**（FreeBuf/Ayuge/geekbyte/CN-SEC）：第 16-31 章补环境/反检测案例
- **军棋独有**：§124q 调用链判据（原书无，军棋做 32→64 影子运行时时自创，见 ch05 5.4.1）

> 关键提醒：原书是"教材框架"，①③④ 是"一手真源"——读第 4-15 章源码时**对着 unidbg 代码库读**，不要只引原书。
