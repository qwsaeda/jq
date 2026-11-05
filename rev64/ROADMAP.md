# 军棋 32位 native → arm64 完整逆向移植 路线图

> 目标：把原版 `lib/armeabi/libjunqi.so`（ELF32/ARM，1.4MB）的**完整玩法逻辑**
> 逆向移植为 arm64-v8a 的等价实现，行为/规则/资源与原版完全一致，
> 让 64 位 only 真机（荣耀等）无警告、能玩、不崩。

**诚实边界**：这是**月级工程**，不是单次对话可完成。本文档是分阶段推进的
总纲，每阶段有独立目标/文档/规范/经验沉淀，供后续阶段参考复用。

---

## ⚠️ 执行路线变更（已决定 + 关键阻塞）

### 技术路线变更：手写重写 → 二进制翻译

原计划 R1-R6 为**手写重写**（`src/jni_arm64.c`/`src/game_state.c`/`src/game_logic.c`/
`src/render_gles1.c`/`src/bt_spak.c`，逐一还原 7480 函数语义）。

**已否决**（用户确认）：不手工重写 7480 函数，改用 **ARM Thumb→AArch64 二进制
翻译器**（`tools/t32_to_a64.py`）机械翻译原版 `.text` 全部指令到 arm64。

### R0-R2 已完成（二进制翻译路线）

- **R0**：分析（func_boundaries 7480 / callgraph 24642 / rodata_strings.map 1785 /
  CONVENTIONS / LESSONS L1-L15）。
- **R1**：翻译器核心，覆盖 99.78%（484,510 条，未处理 1017 全为字面量池数据）；
  全量汇编/链接 0 错误；add-pc 配对 12148；GOT/PLT/兼容层已修。
- **R2**：`dist/libjunqi_arm64.so`（2.58MB，ELF64 aarch64，14 JNI 导出）结构完整
  可链接可加载。详见 `phases/R2_got_plt.md`。

### ⛔ R2 后根本阻塞：`.data.rel.ro` 内嵌指针

**机械翻译到 arm64 可链接/可加载，但不可正确运行**，因为：

- `.data.rel.ro` 有 **4800 个 R_ARM_RELATIVE 内嵌 32 位指针**
  （4432→.text 函数指针/vtable、336→.rodata、32→自引用，**0 个 null**）。
- 64 位下指针须 8 字节 → **全部结构体布局偏移失效**（vtable dispatch
  `ldr rX,[rX,#imm]` 6879 处 + `blx rX` 3689 处）。
- 587 段指针运行 + **1323 个非指针槽**（int/float/char）交错 → 无法用统一
  "偏移×2" 或 "32位加载+加base" 启发式，须**完整类布局分析（=反编译）**。

**唯一可靠路径**：全量反编译 + 重写（回到原 R1-R6 手写路线），月级工程。
机械二进制翻译**到此为止**，无法产生可玩的 arm64 移植。

### 手写路线推进（R1-R6 完成，R5 联机协议未逆向）

二进制翻译硬墙后**切换手写路线**（`src/*.c`），详见 `phases/hand_route.md`：

| 阶段 | 内容 | 状态 |
|------|------|------|
| R1 | JNI 入口绑定（14 个 On*，签名对齐 App.smali） | ✅ `src/junqi_jni.c` |
| R2 | 游戏状态（12×5 网格、25 子/方、行营/大本营、明暗棋） | ✅ `src/game_state.c` |
| R3 | 核心规则（走子/吃子/炸弹/军旗/地雷/翻棋/胜负） | ✅ `src/game_logic.c` |
| R4 | 渲染 GLES2 + 真实棋盘纹理注入 + JNI OnGraph* 驱动 | ✅ `src/render_gles2.c` |
| R5 | 联机/蓝牙/SPAK | ✅ 资源层完成（SPAK 883 资源全解包）+ default.lay 布局接入；⛔ 联机协议未逆向 |
| R6 | 打包 APK（arm64+armeabi+x86_64，v1+v2+v3 签名验证通过） | ✅ `dist/phase_hand.apk` |

**交付物**：
- `dist/libjunqi_arm64_hand.so`（27KB，14 JNI 导出，未定义符号纯 Android 库）。
- `dist/phase_hand.apk`（14.4MB，minSdk 19，v1/v2/v3 签名 `Verifies`，
  `lib/arm64-v8a/libjunqi.so` STORE，MD5 与编译产物一致）。

**诚实声明**：本手写路线 arm64 库是**标准中国军棋规则实现骨架**，**非**原版 32 位
逐函数逆向产物。行为为标准军棋规则（可玩），非原版精确行为（特定变体/AI/资源路径/
SPAK 加密）。不冒充"完整版"。完整匹配原版需从 `.text` 逐函数逆向（月级工程）。

**R5 资源层完成**：SPAK 加密已完全解密（算法=XOR(key[j%32])+zlib，key @ `.rodata` 0x12a658，
`hash=MD5(路径)`）。**883 个资源全部解包**（408 XML + 310 PNG + 18 WAV + 3 TTF 等），
含原版音效/字体/棋盘素材/配置/规则文本/布局方案。工具 `tools/spak_unpack.py` + C 侧
`src/spak_load.c`（`spak_load`/`spak_get`，已编译链接）。详见 `analysis/spak_analysis.md`。

**R4/R5 本轮接入**：
- **default.lay 真实布局**：30 字节/方，值编码**降序**（1=司令40…9=工兵32,10=炸弹,
  11=地雷,12=军旗），25 子/方分布与标准军棋精确匹配。`jq_load_layout` 蓝方行0-5
  （军旗行0）/红方行6-11（军旗行11）镜像对称。JNI OnCreate 优先用布局，回退暗棋。
- **渲染接入**：`junqi_render_init/frame/set_board_texture`，JNI OnGraphCreate/Change/
  Update 驱动（互斥锁保护）。GLES1 立即模式已从新 NDK 移除→GLES2。
- **触屏交互**：OnTouch 坐标→格映射 + 选中→走子/翻棋状态机（`handle_tap`），
  选中格高亮 `junqi_render_set_selected`。
- **单机对弈**：蓝方玩家触屏，红方 `jq_ai_move` 在 OnUpdate 自动回应。
- **真实棋盘渲染**：`png_decode`（仅 zlib）+ `spak_load_assetmanager`（AAssetManager）+
  `spak_get_by_hash`（棋盘 datah#188）→ `junqi_render_set_board_texture`。
- **PNG 解码**：`png_decode.c`（8bit RGB/RGBA/灰，非隔行，滤波器 0-4），仅依赖 zlib。

**R5 剩余**：蓝牙联机协议字段未逆向（需从 `.text` 逐函数），`bt_spak.c` 为占位。

---

## 规模基准（决定性事实）

| 项目 | 数值 |
|------|------|
| 原版 native 架构 | ELF32 / ARM / EABI5 / stripped |
| `.text` 段范围 | `0x233e0` – `0x1311f4` |
| `.text` 段字节 | 1,105,428 字节 |
| 反汇编指令总数 | 514,762 条（Thumb） |
| 函数总数（push lr 入口） | 7,480 个 |
| JNI 导出入口 | 14 个 `Java_com_cnvcs_App_On*` |
| 加密字符串池 | `.rodata`（高熵 6.76，运行时自解密）|
| 明文 dump 已获 | `frida-dump/rodata_dec.bin`（118KB）+ 1729 条字符串 |

函数大小分布（7480 个）：
```
<32B:1535  32-64B:2002  64-128B:1752  128-256B:1147
256-512B:690  512-1K:254  1K-4K:90  >4K:10（最大 12204B @0x12790c）
```

---

## 阶段划分（按依赖顺序推进，不可跳）

### R0 · 基础设施与静态分析（当前阶段）
**目标**：建立可复用的分析管线、函数边界表、调用图、.rodata 明文映射、
JNI 接口签名，为移植提供地基。
**产出**：
- `analysis/func_boundaries.txt`（7480 函数边界）✅
- `analysis/callgraph.txt`（bl/blx 调用图）
- `analysis/rodata_strings.map`（明文串 ↔ .rodata 偏移）
- `analysis/jni_api.md`（14 个 JNI 入口签名 + App.smali 对应）
- `docs/R0_analysis.md`
**规范**：函数命名 `sub_XXXXXX`；地址统一用原版 .so 虚拟地址；Thumb 位 = addr|1。
**经验**：objdump `force-thumb` 能跨数据；capstone 遇非法指令停止，须用 objdump。

### R1 · JNI 入口层移植
**目标**：14 个 `Java_com_cnvcs_App_On*` 在 arm64 下可 dlopen、可注册、可调用，
返回正确的 GLView / 布尔值，不崩。
**产出**：`src/jni_arm64.c` + `lib/arm64-v8a/libjunqi.so`（骨架，能加载）。
**验收**：mumu(64位桥) / 真机 arm64 上 `System.loadLibrary` 成功、OnCreate 不崩。
**规范**：JNI 签名必须与 `smali/com/cnvcs/App.smali` 完全一致（红线：不改签名）。

### R2 · 核心数据结构与内存模型
**目标**：还原游戏状态机（棋盘 10×9、38 子、红蓝方、行走/翻棋/炸弹/军旗/地雷/
悔棋/联机状态）、全局对象布局、资源路径表。
**产出**：`src/game_state.c` + `analysis/state_layout.md`。
**依据**：`.rodata` 明文（`Junqi.Move/Flip/Flag/Bomb/Eat/Dead`、`data/config/*.xml`、
`data/{320,480,480x,720,720s,800}/`、`updateUrl/statHost/cfgHost`）。

### R3 · 核心玩法逻辑移植
**目标**：走子/吃子/炸弹/军旗/地雷/悔棋规则、胜负判定、简单 AI（blue 方）、
事件派发（`Junqi.*` 字符串对齐 32 位明文）。
**产出**：`src/game_logic.c`（纯逻辑，无 GL 依赖）。
**验收**：arm64 下能完整下一局（翻棋→走子→吃子→胜负）。

### R4 · 渲染层移植（GLES1）
**目标**：GLView 渲染管线（GLES1，原版依赖 `libGLESv1_CM.so`）、
`OnGraphCreate/OnGraphChange`、纹理/字体/图片资源加载。
**产出**：`src/render_gles1.c`。
**注意**：原版用 GLES1（非 GLES2），arm64 需对应 libGLESv1_CM 或转 GLES2。
**经验**：redroid 无 GPU，GL 不可用——渲染验收须在 mumu/真机。

### R5 · 联机 / 蓝牙 / SPAK 层
**目标**：蓝牙联机协议（`OnBluetooth`/`OnEvent`）、SPAK 加密 key、服务器通信。
**产出**：`src/bt_spak.c`。
**难点**：SPAK 解密 key 未提取，需从 32 位 `.text` 逆向定位。

### R6 · 集成与验收
**目标**：arm64 全功能（装→进对局→下一局→退出→重进不崩），
双 ABI 包（armeabi 原版 + arm64-v8a 移植版）。
**验收判据**（统一）：装上→进主界面→下一局（本地或蓝牙）→正常退出→重进不崩。

---

## 全局红线（延续 AGENTS.md，移植时不可违）

1. **不把 arm64 简化版说成"完整版"**——本工程 R0-R6 全完成前，交付物只能是
   "移植中"，不得声称完整。
2. **不改 `smali/com/cnvcs/App.smali` 的 native 调用签名**。
3. **不混用 ABI**（dlopen 32位库到64位进程 = 死路）。
4. **重打包必须用新 keystore**（`/disk/workspaces/jq/junqi.keystore`）。
5. **每阶段必须留文档**（本目录 `docs/` + 每阶段 `phases/Rx_*.md`），供后续复用。

---

## 复用指引（后续阶段如何参考本阶段）

- 读 `docs/Rx_*.md` 了解该阶段目标/结论/坑。
- 读 `phases/Rx_*.md` 了解该阶段具体操作与验证记录。
- 用 `analysis/func_boundaries.txt` 定位任意函数的地址范围。
- 用 `analysis/callgraph.txt` 追调用链。
- 用 `analysis/rodata_strings.map` 还原字符串语义（`Junqi.*` 事件名等）。
- 任何阶段遇到"能玩/不能玩/崩"的问题，先查 `docs/LESSONS.md`（踩坑经验）。
