# unidbg 逆向工程 31 章配套课程

> 基于陈佳林《unidbg 逆向工程：原理与实践》(机工社 2024) 按章节输出
> 实战靶机:军棋 `lib/armeabi/libjunqi.so`(ELF32 / ARM / EABI5 / stripped)
> 课程目录:`/disk/workspaces/jq/course_unidbg/`

## 这本课程怎么读(痛点导读)

每章开头都有「## 痛点:为什么 X?」段——先讲"卡在哪、为什么卡",
再用大白话 + 生活类比讲"为什么这样做",最后用军棋 so 的真实卡点/实测数据验证。
全课程主线就一个痛点:**军棋 so 是 ARM 二进制,你的 PC 是 x86,怎么在 PC 上跑通它、看懂它、批量调它。**

四大痛点对应四部分:

| 部分 | 核心痛点 | 章节 |
|---|---|---|
| 第一部分(入门) | ARM so 在 PC 上跑不了,怎么"搬进模拟房间"? | 1-3 |
| 第二部分(原理) | "搬进来"后,加载/链接/初始化到底怎么跑的?崩在哪? | 4-15 |
| 第三部分(实战) | so 读文件/调 Java/查设备,缺的"环境"一个个怎么补? | 16-26 |
| 第四部分(反制+部署) | so 反过来查"你是不是模拟器",怎么反?怎么部署成服务? | 27-31 |

### ★ 当前卡点状态(§124 纠正后,2026-11-01)

军棋 32 位 so 的 unidbg 路线已通到:12 init_array + 5312 RELATIVE + 214 PLT + 6 GLOB_DAT 全重定位、
DiagJni 把 OnCreate 推进到 `std::vector` 数据与指令对齐点 0x43ed8。剩余 **3 项阻塞**
(之前认为"剩 1 项 C++ 对象图"是误判,§124 纠正):

1. **① 上下文相关 BL 解码 bug(新发现,需 workaround)**:加载 .so 后
   `BL 0x7965a` 被 unicorn2 解码成目标 `0x6eaea`(memset 循环),而真实目标是
   `0x43ed8`(std::vector 构造)。这是**模拟器精度 bug**(unicorn2 上下文相关),
   非翻译/逻辑 bug。待确认:换 dynarmic 后端 / code hook patch 目标 / unicorn2 版本修复。
2. **② 数据与指令对齐对应缺口**:`std::vector` 对象图的 `begin/end/capacity`
   来自未初始化对象,需真实运行时/GPU。
3. **③ 288MB PROT_NONE mmap**:unidbg mmap2 对 288MB PROT_NONE 处理失败。

**注意**:之前"BL off-by-2 -0xa"的判断是**误读**——`F000 F800` 这条 BL 的正确目标
是 `0x10000004`(非 `0x10000020`),隔离环境 BL 全对,真正的问题是加载 .so 后的
上下文相关解码错位(§124)。课程里相关章节(ch05/ch06/ch11/ch12/ch22/ch25/ch26/ch31)
已统一更新为 3 项阻塞。

## 快速导航

```
course_unidbg/
├── INDEX.md                  # 课程索引 + 军棋实测对照表 + 工具说明
├── COURSE_CORRECTIONS.md     # 军棋实战修正记录(R6 红线对齐)
├── 01_basics/                # 第一部分 进入 unidbg 的世界
│   ├── ch01_env_setup.md       第1章 环境准备与快速上手
│   ├── ch02_sim_exec.md        第2章 模拟执行初探
│   └── ch03_hook_patch.md      第3章 补环境、Hook 与 Patch
├── 02_principle/             # 第二部分 unidbg 原理
│   ├── ch04_elf_view.md        第4章 ELF 文件执行视图解析
│   ├── ch05_unicorn_linker.md  第5章 Unicorn 初级使用与初探 Linker
│   ├── ch06_linker_deep.md     第6章 深入 Linker:so 加载/链接/初始化
│   ├── ch07_sim_linker_load.md 第7章 模拟 Linker:so 加载过程
│   ├── ch08_sim_linker_link.md 第8章 模拟 Linker:so 链接过程
│   ├── ch09_r0dbg_findkey.md   第9章 R0dbg 实战与 Unidbg_FindKey
│   ├── ch10_emulator.md        第10章 源码解析:AndroidEmulator
│   ├── ch11_dalvikvm.md        第11章 源码解析: DalvikVM
│   ├── ch12_sim_trace.md       第12章 源码解析:模拟执行流程追踪
│   ├── ch13_jni_trace.md       第13章 源码解析:JNI 交互流程追踪
│   ├── ch14_memory.md          第14章 源码解析:Memory
│   └── ch15_hook.md            第15章 源码解析:Hook
├── 03_practice/              # 第三部分 模拟执行与补环境实战
│   ├── ch16_io_redirect.md     第16章 实战:I/O 重定向
│   ├── ch17_debugger_selfdump.md 第17章 实战:Debugger 自吐
│   ├── ch18_pointer_args.md    第18章 实战:指针参数与 Debugger
│   ├── ch19_base64.md          第19章 实战:魔改 Base64 还原
│   ├── ch20_mem_data.md        第20章 实战:动态分析内存中的数据
│   ├── ch21_fork.md            第21章 实战:主动调用 fork 进程
│   ├── ch22_env_basics.md      第22章 补环境入门
│   ├── ch23_identifier.md      第23章 补环境:标识记录
│   ├── ch24_device_risk.md     第24章 补环境:设备风控
│   ├── ch25_env_enhance.md     第25章 补环境加强
│   └── ch26_env_summary.md     第26章 补环境总结
├── 04_counterpart/           # 第四部分 反制与生产环境部署
│   ├── ch27_env_detect.md      第27章 Anti-unidbg:环境变量检测
│   ├── ch28_xhook.md           第28章 Anti-unidbg:xHook 检测
│   ├── ch29_jni_reversal.md    第29章 Anti-unidbg:JNI 层常见函数处理
│   ├── ch30_detect_summary.md  第30章 Anti-unidbg:unidbg 常规检测总结
│   └── ch31_deploy.md          第31章 unidbg 生产环境部署
├── assets/                   # 44 张配图(AI 生成 agnes-image-2.5-flash)
├── videos/                   # 31 段讲解视频(AI 生成 agnes-video-2.5-flash,~10s/段)
└── retry_videos*.sh          # 视频重试脚本(429 限流补跑用)
```

## 军棋 32 位 so 课程对照要点

| 类别 | 数据 | 对应章节 |
|---|---|---|
| JNI 导出符号 | 14 个 `Java_com_cnvcs_App_On*` | Ch2/12/13/30/31 |
| so 依赖库 | 7 个 DT_NEEDED(libc/libstdc++/libm/liblog/libdl/libGLESv1_CM/libGLESv2) | Ch8/14/28 |
| init_array | 12 个全通 | Ch6/8/10 |
| RELATIVE 重定位 | 5312 槽全通 | Ch6/8 |
| PLT 重定位 | 214 槽全通 | Ch6/8 |
| GLOB_DAT | 6 符号(docker 真 ARM libc 解析) | Ch8 |
| 后端 | unicorn2(修 PC off-by-2,DiagTracePC 0xa37fc=0x159880) | Ch5/7/30 |
| JNIEnv/VM 补环境 | DiagJni 实证:OnCreate 推进到 std::vector 对象图 0x43ed8 | Ch11/12/13/22 |
| .rodata | 明文 118KB(静态即明文,无自解密,0x138xxx 高熵索引表非加密代码) | Ch4/20 |
| .text 常量池 | 18 个(需 redroid dump,unidbg 静态读不出) | Ch9/17/19 |
| 广告层 | `FindClass("com/baidu/mobads/AdView")` 拆除目标 | Ch15/29 |
| SPAK | 32B key 内嵌 .rodata + 4 组 883 条目 + 魔改 Base64 | Ch19/20 |
| 对局存档 | one.sjg(60 格棋盘 + turn/stage/winner + flags/mines/pieces + moves) | Ch16/23/24 |
| ★下一轮主攻 | C++ 对象图补全(给 std::vector 补 begin/end/capacity 初始值) | Ch25 §25.4.1 / Ch26 §26.4.1 |

## 生成工具(都在 TreasureIsl 项目里)

- 生图:`/disk/workspaces/treasureIsl/scripts/agnes_gen.py`(agnes-image-2.5-flash,2K 16:9)
- 生视频:`/disk/workspaces/treasureIsl/scripts/agnes_video.py`(agnes-video-2.5-flash,~10s,16:9)
- 多模态看图(可选质检):`/disk/workspaces/treasureIsl/scripts/agnes_vision.py`(agnes-2.5-flash)
- API KEY:环境变量 `AGNES_API_KEY`(从 `/disk/workspaces/treasureIsl/.env` 取,不入库)

## 使用说明

1. 讲义:按 01_basics → 02_principle → 03_practice → 04_counterpart 顺序读
2. 配图:每章 1-2 张,与讲义同目录的 `assets/chNN_*.png` 对应
3. 视频:每章 1 段 ~10s 的讲解动画,`videos/chNN_intro.mp4`
4. 修正记录:看 `COURSE_CORRECTIONS.md`(军棋 R6 红线对齐清单)
5. 总索引:看 `INDEX.md`(军棋实测对照表 + 课程推进约定)

## 诚实边界(军棋红线)

- 课程里的军棋数据**全部**对齐 AGENTS.md §R6 红线 + `rev64/` 实测产物
- "指令语义 100% 对齐 ≠ 能跑 ≠ 能玩"(红线 §122 诚实降级)
- 军棋"能玩"需真实 arm64 GPU 设备验收(redroid 无 GPU,MuMu 影子库稳定运行但游戏板黑屏)
- ★ 下一轮 unidbg 主攻方向:C++ 对象图补全(std::vector begin/end/capacity)
