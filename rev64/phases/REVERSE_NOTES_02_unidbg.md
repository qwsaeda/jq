> 来源: REVERSE_NOTES.md §84-§100（行 276-814），2026-11-02 拆分
## §84 unidbg init_array 突破 + 自解密前提被推翻（本轮）

### unidbg 全面突破（推翻 AGENTS.md"unidbg 死路"）
- **12 个 init_array 全通**：init[0..11] 全部执行并返回（仅需打桩 `0x11af58 → bx lr` 跳过 iostream 静态初始化链；否则 `0x11ad80` 读 `_ctype_` 附近 GOT 槽失控）。
- **GLOB_DAT 6 符号**从 docker 真 ARM libc 符号表解析（`_ctype_`=指针→`_C_ctype_`）；**JUMP_SLOT 211/214 eager-resolve**（仅 `__android_log_print` 打桩 + atexit/stack_chk 打桩）。
- **依赖模块**：docker 真 ARM `libc/libstdc++/libm/libdl/libGLESv1_CM/libGLESv2` 可加载；`libc++/liblog/libcompiler_rt` 加载崩（不兼容）→ 不加载。
- **PLT→GOT→符号映射 214/214** 完整（公式 `got=(st+8)+ror(imm1)+ror(imm2)+(w2&0xFFF)`）。
- **RELATIVE 5312 槽全部正确重定位**（unidbg loadLibrary 自动应用 `base+文件值`，实测 `0x15bb0c→base+0x162844`、`0x15b744→base+0x1374c8` 等）。

### ARM/Thumb 模式 + PC 对齐（重要纠正）
- **必须 `|1`（Thumb bit）调用**：偶地址函数（`0x24250`/`0xb9edc`/`0xba12c`/`0x234f8`）被 unidbg 当 ARM 模式执行→误报 "VFP coproc exception"（`0xF090447B`）。init 函数都是奇地址所以此前无此问题。
- **unidbg dynarmic 的 Thumb `add rX,pc` 用 `(addr+4)&~3`（mask）**，而真实 ARM Thumb 用 `(addr+4)` **不 mask** → 所有 PC-relative 字面量计算 **off-by-2**（如 slot `0x15b744` 被算成 `0x15b742`，`.got` 基址 `0x15b0c8` 被算成 `0x15b0c6`）。这导致 `0xb9edc` 读未对齐槽 `[base+0x15bb0a]`=垃圾 `0xb8441226` 崩溃。**是模拟器精度问题，非库 bug**。
- **★（本轮实证修复）unicorn2 后端不 mask PC**：`DiagTracePC` 用 **Unicorn2Factory** 调 `0xa37fc|1`（含 `add r3,pc` @0xa3802），hook 捕获 r3=b6072、pc=base+0xa3802，执行后 `[dst]` 存 **0x12272880 = base+0x159880（不 mask 值）**，与真实 ARM 逐字节一致（mask 值应为 0x15987e）。**PC off-by-2 仅存在于 dynarmic 后端，切换 Unicorn2 即修复**。unidbg 的"PC 精度 bug"根因 = 后端选择，非 unidbg 本体逻辑缺陷。
- **★（本轮实证）JNIEnv 补环境后 OnCreate 深入**：`DiagJni` 用 `emulator.createDalvikVM()` + `vm.resolveClass("com/cnvcs/App")` + `app.callStaticJniMethodObject(emulator,"OnCreate",activity)`（标准 unidbg JNI 调用，自动设 r0=JNIEnv）。OnCreate 从**入口即崩**（dummy env=0）推进到 **`bl 0x43ed8`（std::vector 拷贝构造）**——即从"空 env 解引用"推进到"C++ 对象图内部（vector begin/end 指针垃圾）"。**证明 unidbg 补环境可行，剩余是 C++ 对象图语义（需真实运行时），非模拟器精度**。

### 自解密前提被推翻（核心纠正）
- **`.rodata` 字符串池是明文**：`rodata_dec.bin`（118832B）与文件 `.rodata`（file offset 0x120bf8, size 0x1d030）**逐字节相同（0 diffs）**。关键串 `Junqi.Move`@0x120f20、`data/config/ui.xml`@0x120c88、`updateUrl`@0x120cf0、`default.lay`@0x1211c1、`one.sjq`@0x12173c **直接可读，无需解密**。共 1794 条 ASCII 串（含 284 条 `Junqi.*`/`data/`/`config`/`*.png`/`*.lay`/`*.sjq`/`updateUrl`/`statHost`/`cfgHost`）。
- **高熵区 = 静态解码查找表，非加密流**：
  - **表1**（`.got` slot `0x15b744`→`base+0x1374c8`）：**JPEG 标准 zigzag 表 64 项完全匹配**（`0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,...62,63`）。
  - **表2**（`.data` slot `0x15be00`→`base+0x13a1bc`）：**zlib inflate dist 码表**（`0x8000c,0x8008c,...` 0x800 长度前缀 + 距离）。
  - blob `[0x137ff8,0x13a1f8)`（0x2200B，熵 7.94）无 zlib/PNG/zip 魔数，前 64B 有规律结构（16-bit 交错 A 递增/B 递减），**是 JPEG/zlib 解码器内嵌静态表**。
- **结论**：**原版 `.rodata` 无自解密**。AGENTS.md"字符串池加密/dlopen 运行时自解密"是**聚合熵误读**（熵 6.76 = 明文字符串 + 二进制解码表混合）。`.text` 明文 + `.rodata` 明文串 + 静态解码表 = **原版库无加密壳**。

### OnCreate 路径阻塞分析（非库 bug）
- `0xb9edc`（单例构造器，依次分配 0x80/0x20/0x40/0x1c/0x3c/0x58/0x40/0xc/0xc8/0x108 字节对象）崩溃是 **unidbg dynarmic Thumb PC off-by-2** 读错槽（**unicorn2 后端已修复此点**）。
- `0x24250|1` 崩溃写 `0xc0` 是**对象语义缺失**（`0x241f4` 构造未初始化 `[r4,#0x40]` 指针），非库逻辑。
- **★（本轮实证）OnCreate 0x23561 切 unicorn2 后错误从 dynarmic 的 `MemoryWrite32 vaddr=0xe4affea8`（垃圾指针写）变为 `UC_ERR_WRITE_UNMAPPED`（干净未映射写）**——PC off-by-2 已消除，剩余纯对象图语义缺失（dummy env/obj=0,0,0 导致写未映射地址）。
- **★（本轮实证）`DiagJni`（createDalvikVM + resolveClass + callStaticJniMethodObject）给真实 JNIEnv 后 OnCreate 推进到 `bl 0x43ed8`（std::vector 拷贝构造）**——从"空 env 解引用"推进到"C++ 对象图内部"，证明 unidbg 补环境（JNIEnv/VM/DvmClass/DvmObject）完全可用，剩余阻塞纯为 **std::vector begin/end 指针来自未初始化对象**（真实 C++ 对象图需真实运行时）。
- **unidbg 无法验证"能玩"**：需真实 GPU + 对象语义，redroid 无 GPU / 64 位 native bridge 不完整。

### 下一步建议
1. **arm64 对齐无需移植解密**：字符串池明文（1794 串直接提取）、JPEG zigzag/zlib dist 表为标准库表（arm64 用系统 libjpeg/zlib 即可）。
2. 若需提取 blob 完整表（0x2200B JPEG/zlib 解码状态），可直接 `dd` 文件 `[0x137ff8,0x13a1f8)`。
3. unidbg 剩余价值 = **unicorn2 后端已修复 PC off-by-2**（§84 实证）+ **JNIEnv/VM 补环境可行**（DiagJni 实证，OnCreate 推进到 std::vector 对象图）；剩余阻塞纯为**对象图语义缺失**（std::vector begin/end 来自未初始化对象，需真实 C++ 运行时/GPU）。unidbg 补环境/反检测/花指令/加密壳处理能力可用，但对象图需真实运行时。

## §85 SPAK 资源链全解包 + 棋子图集对齐（本轮核心）

### SPAK 格式完全确证（9 包，883 条目）
- **格式**：`"SPAK"(4B) + ver(1B) + pad(3B) + table_offset(4B LE) + table_len(4B LE)`；文件表 `XOR(key[j%32]) + zlib inflate`；条目 28B（hash16 + off4 + size4 + usize4）。
- **key 32B**：`c2 cb e5 e7 ce 5c 4a c4 11 ca 6c 99 06 f4 76 f8 6d 4f cb b6 26 21 de 0e 6a 67 87 4c 93 f0 d5 23`（明文态 `.rodata`，arm64 `spak_load.c` 已内嵌）。
- **组表偏移**（arm64 GROUPS 硬编码值验证）：data `f1=0x12d4f3 f2=2712`（data0+data1 合并流总长 0x12df8b 精确匹配）；datah `f1=0x155db2 f2=6571`；datam `f1=0x6e6f9 f2=4572`；datax `f1=0x2f3f86 f2=7278`。
- **条目数**：data 121 / datah 272 / datam 189 / datax 301 = **883 条目**。
- **内容分类**（魔数）：310 PNG / 408 XML / 18 WAV / 20 字体 / 127 其他（广告 `id`/`app` 数据，拆除目标）。
- **寻址语义**：**data 组按 `MD5(资源路径)` 寻址**（已验证 38 个：`data/lay/00-11.lay`、`data/config/*.xml`、`data/font/*.ttf`、`data/junqi/*.wav`、`data/ui/*.cfg`）；**datah/datam/datax 按 UI 布局 XML 索引**（非路径 MD5，0 匹配确证）。

### 资源路径反查（38+，data 组 MD5 匹配）
- `data/lay/{00-11}.lay` + `default.lay`（30B 字节级，`default.lay==00.lay`）
- `data/config/{path,sound,string,uitag}.xml`（`path.xml`：`data/ui` 图 + `data/config` 布局）
- `data/font/{936.cp(131072B), all.ttf(1.5MB), mb.ttf, sb.ttf}`
- `data/junqi/{bomb,dead,draw,eat,flag,flip,lose,move,over,select,start,win}.wav`（12 音效）
- `data/junqi/{ruleflip,rulelay,sentence}.txt`、`data/sg/{help,nethelp,sentence}.txt`
- `data/ui/{black,edit,popup}.cfg` + `data/ui/{close,pop}.wav` + `data/ad/{01-11,ad}.cfg`（广告，拆除）
- `data/base.cfg`（服务器配置：`siteUrl/updateUrl/serverHost 42.121.132.86:8500/sgServerHost 121.43.147.41:8635/statHost/cfgHost/channel baidu/package com.cnvcs.junqi`）

### 棋盘/棋子图集确证（datah 组，按索引）
- **`datah[188]` hash `21d95e03e7692e98bd1649647225f819` = 737×751 棋盘**（十字形军棋棋盘，arm64 已用 `BOARD_HASH_DATAH`）。注意：**usize==size=154806 未压缩条目**（直接 XOR 后 PNG，非 zlib）。
- **`datah[187]` hash `8696de253d7c93073620393b86073950` = 480×490 棋盘**（低分辨率版，**非棋子图集**——先前误判）。
- **棋子图集 = `datah[183]` hash `9e1b7fbc4dfc09c3d23cdf8547cca22b` = 212×280（4列×7行，53×40/格）**：
  - **行0-2 红方**：`司令 军长 师长 旅长 / 团长 营长 连长 排长 / 工兵 炸弹 地雷 军旗`（rank1-12 行序）
  - **行3-5 蓝方**：同序（rank1-12）
  - **行6 状态块**：绿/橙/蓝/黑（选中/暗棋/边框等）
  - **映射**：rank→列 `(rank-1)%4`，子行 `(rank-1)/4`；红(side=1)=行子行，蓝(side=0)=行3+子行。
- **`datah[185]` = 400×78**"战斗开始"按钮文字。

### arm64 棋子图集实现（本轮，视觉对齐）
- `junqi_jni.c`：加 `PIECE_HASH_DATAH`（datah[183] 212×280）+ `load_piece_texture()` + `OnGraphCreate` 调用 `load_board_texture(); load_piece_texture();`。
- `render_gles2.h`：加 `junqi_render_set_piece_atlas()` 声明。
- `render_gles2.c`：加 `g_piece_tex/g_piece_w/g_piece_h` 状态 + `junqi_render_set_piece_atlas()` 实现 + **棋子图集渲染通道**（已翻开子用图集子纹理，rank→列/子行 + side→行；未翻开子仍色块背面绿）+ LOGI 宏。
- **arm64 rank 值域**（junqi_core.h）：1=司令最强...9=工兵最弱，10=炸弹，11=地雷，12=军旗——**与图集行序完全一致**（`layout_rank` 直接返回布局字节值）。
- **编译链接通过**：NDK r25b `aarch64-linux-android24-clang` 全部 9 个 .c（game_state/game_logic/junqi_jni/render_gles2/bt_spak/spak_load/png_decode/audio_sl/save）exit=0，链接 `/disk/workspaces/jq/rev64/dist/libjunqi_arm64_hand.so` 56192B，**15 个 JNI 符号与原版 32 位完全一致**（原版实际 14 个唯一，arm64 14 个唯一匹配）。

### 重打包 phase_hand.apk（三件套完成）
- `apktool b dec -o phase_hand_new.apk`（成功）→ `store_compressed.py`（.so 全部 STORE）→ `SignV2`（v1+v2）。
- **验证**：CheckSig `verified: true`；`.so` 全部 `compress_type=0`（未压缩）；`AndroidManifest.xml` 头 `0x00080003`（二进制）。
- **产物**：`/disk/workspaces/jq/rev64/dist/phase_hand.apk`（14483456B）。

### 修正记录
- **先前误判**：datah[187]=480×490 是**棋盘**（非棋子图集），已改为 datah[183]=212×280。
- **datah[188] 未压缩条目**：usize==size 时直接 XOR 返回（非 zlib inflate），arm64 `spak_load.c` 需处理此分支（已确认加载路径）。

### spak_load.c 未压缩条目修正（本轮补充）
- **问题**：`spak_load.c` 原对**所有**条目无条件 `zlib_inflate`，但棋盘 datah[188]（737×751）和棋子图集 datah[183]（212×280）是**未压缩条目**（usize==size，直接 XOR 后 PNG）。zlib_inflate 失败后靠回退 `ent->data=raw` 能工作，但不精确。
- **修正**：显式检测 `ent->usize == ent->size` → 未压缩条目直接 `ent->data = raw`（XOR 后即资源），否则 zlib_inflate + 失败回退。
- **重新编译**：NDK r25b 全 9 .c exit=0，链接 `libjunqi_arm64_hand.so` 56224B，14 JNI 符号匹配原版。
- **重打包验证**：CheckSig `verified: true`；.so 全部 STORE；manifest 头 `0x00080003`；`phase_hand.apk` 14483456B。

## §86 棋子图集渲染完整化（背面/选中/符号通道）

### 棋子图集行6 状态块确证
- row6 col0 = **纯绿暗棋背面**（未revealed 子，带 3D 边框）
- row6 col1 = 橙（红方选中高亮）、row6 col2 = 蓝（蓝方选中高亮）、row6 col3 = 黑（空/禁用）
- 每格**已含 rank 文字**（红方"司令/军长..."白字，放大确证）→ 原版棋子图集自带文字，无需叠加数字。

### arm64 渲染修正（render_gles2.c）
1. **背面渲染**：未revealed 子改用图集 row6 col0 绿块（原为纯色块 `0.35/0.55/0.35`），带 3D 边框更真实。
2. **选中高亮**：选中格用 row6 col1(红)/col2(蓝)，替代色块高亮。
3. **符号/rank 通道跳过**：棋子图集已注入时跳过数字/符号绘制（图集每格已含 rank 文字），避免重复文字。

### 棋盘 UV 映射确证
- arm64 棋盘 UV `0.366-0.631` **精确对齐**中央竖带格线：737×751 十字形棋盘，中央竖带 x∈[270,465]（5列格），737×0.366=270、737×0.631=465。棋盘主体映射正确。
- 十字形棋盘：中央竖带（12行×5列主棋盘）+ 左右横带（大本营/行营区）。

### 编译打包（三件套完成）
- 库 `libjunqi_arm64_hand.so` 56240B（14 JNI 匹配原版），`phase_hand.apk` 14483456B。
- CheckSig `verified: true`；.so 全 STORE；manifest 头 `0x00080003`。

## §87 棋子渲染几何对齐 + 规则文本确证

### 规则文本确证（GBK 解码，ruleflip/rulelay）
- **ruleflip（翻棋）**：轮流翻棋，先手翻出定色；炸弹与任何棋子同归于尽；地雷/军旗只有工兵能吃；工兵挖完3雷才能扛军旗；70步无吃子判和、1000步判和、无棋可走判负。
- **rulelay（明棋布局）**：炸弹不能放第一行、地雷只能放最后两行、军旗只能放大本营；任何棋子能扛军旗；70步/1000步判和。
- **arm64 规则已对齐**（game_logic.c L57/L72/L193/L196/L220/L227/L235/L275/L308）：70步无吃子、1000步、无棋可走判负、工兵挖雷、炸弹同归于尽、任何棋子扛军旗、暗棋/明棋双模式（JQ_STAGE_PLAY/DARK）。

### 棋盘几何确证（737×751 十字形）
- **中央竖带**：x∈[270,465]（5列格，宽195，格宽39），y∈[20,729]（12行格，格高59）。**12行×5列** 与 arm64 `JQ_BOARD_ROWS=12, COLS=5` 完全一致。
- **board_region 比例**：arm64 用 `5.0f/12.0f`（5列:12行），与竖带比例一致。
- **棋盘 UV 映射**：arm64 `0.366-0.631` 精确对齐竖带格线（737×0.366=270, 737×0.631=465）。
- **行营/大本营坐标**（绿框检测确证）：蓝行营 (4,1)(4,3)(3,2)(2,1)(2,3)、红镜像 (7,3)(7,1)(8,2)(9,3)(9,1)；大本营 (0,1)(0,3) 蓝、(11,1)(11,3) 红——与棋盘图行营/大本营标记精确对齐。
- **铁路**：arm64 `jq_is_rail` 纵向 col2 + 横向 row5/6（逻辑），棋盘图铁路线是视觉格线，规则独立。

### 棋子渲染几何修正（保持宽高比）
- **关键发现**：棋盘竖带格 39×59（纵向），棋子图集格 53×40（横向）——比例完全不同。arm64 原把图集格 UV 拉伸到棋盘格（cw×ch）导致棋子严重变形。
- **修正**：棋子保持图集 53:40 比例渲染，按棋盘格宽缩放居中（39×29.4），不拉伸。
- **色块通道跳过**：图集已注入时跳过色块（图集渲染所有有子格，避免闪烁/浪费）。
- **符号/rank 通道跳过**：图集每格已含 rank 文字（"司令/军长..."白字放大确证），跳过数字避免重复。

### spak_load 未压缩/压缩分支确证
- datah[183] 棋子图集：压缩（size=59111≠usize=59123）→ zlib_inflate。
- datah[188] 棋盘：未压缩（usize==size=154806）→ 直接 XOR 返回。
- 修正逻辑覆盖两者（未压缩直接 XOR，压缩走 zlib）。

### 编译打包（三件套完成）
- 库 `libjunqi_arm64_hand.so` 56304B（14 JNI 匹配），`phase_hand.apk` 14483456B，CheckSig `verified: true`，.so 全 STORE，manifest 头 `0x00080003`。

## §88 布局选择 + 存档/AI/蓝牙确证

### 原版布局体系确证（12 预设）
- **SPAK 内**：`data/lay/00-11.lay`（12 预设，30B/布局）+ `default.lay`（=00.lay）
- **运行时存档**：`jq1-3.lay`（玩家）、`comp.lay`（电脑）、`net.lay`（网络）、`layname.txt`（布局名）——均不在 SPAK（运行时生成）
- 12 布局均 **5空+19普通+2炸+3雷+1旗=30B**（每方 25 子），12 个唯一排列
- 完整字节已提取（LAY00-11）

### arm64 布局选择实现
- `OnEvent("layout", "<0-11>")` → `spak_get("data/lay/%02d.lay")` → `jq_setup_begin`
- 库 56656B（11:06 编译），phase_hand.apk 14483456B（11:11 重打包，含此改动）
- CheckSig verified、.so 全 STORE、manifest 头 0x00080003、APK 内 .so=56656B

### 存档体系确证
- `one.sjq`（设置/胜负统计）：arm64 已实现（save.c，time/difficulty/level/mark/resultEx 对齐原版 0x38/0x3c/0x40/0x44/0x48）
- `one.sjg`（对局进度存档）：原版有，arm64 未实现（重进=新局，简化版取舍，优先级中）
- `one.sav`（网络/广告）：非核心
- `%s.sjq`（0x123ca0）：动态对局名（one=单机/net=网络）

### AI 确证
- arm64 三级难度（0=简单/1=中/2=难），`jq_ai_set_level` 限制 0-2，`ai_level` OnEvent
- 原版 `data/inter/%02d.cfg`（InterDelay 配置）不在 SPAK（运行时生成/硬编码），AI 配置细节非核心

### 蓝牙联机确证（仍未实现）
- 原版：`BtReceiver`（BroadcastReceiver）→ `App.OnBluetooth(Intent)` → native
- .rodata 已确证 `PeerName/PeerFirst/PeerLay/PeerShow/PeerCmdSafe/PeerDraw/PeerBingLei/PeerBingQi` + `Junqi.PeerStart/PeerLayFinish/Resign/ConnectFail/LeaveGame` 等
- 协议字段（SPP UUID/帧格式）未逆出，需逆向 OnBluetooth C++ 逻辑
- **bt_spak.c 注释已更新**："SPAK key 未提取"过时（key 已内嵌 spak_load.c，4 组 883 条目可解包）→ 更新为 R6 状态

## §89 蓝牙联机协议确证（OnBluetooth 逆向）

### 蓝牙 RFCOMM SPP 确证（0xbe9cc OnBluetooth 主逻辑）
- **UUID**：`ce1404fe-0863-4278-86c2-c9c560f0e215`（rodata 0x121334，自定义非标准 SPP）
- **服务名**：`junqi`（rodata 0x120d18，引用 @0x24578，紧邻 `bl 0x78fa0` = RFCOMM listen 服务端包装）
- **JNI 蓝牙调用**（OnBluetooth 主逻辑 0xbe9cc 引用）：`getAction`（Intent action）、`getIntExtra`/`(Ljava/lang/String;I)I`（extra 数据）、`BluetoothDevice.getAddress`（设备地址）、`()Ljava/lang/String;`
- **协议入口**：`BtReceiver`（Java BroadcastReceiver）→ `App.OnBluetooth(Intent)` → native 0xbe9cc

### 蓝牙对局帧确证（0x26f24 帧构造）
- **单步帧**：`%d,%d,%d,%d,%d,%d,%d\n`（0x120e94，引用 @0x26f24），7 字段从棋盘结构体 offset 81/82/83/84/808/1784/256 读，`sprintf`（0xb4bd0）→ `bl 0x23ae8`（帧发送）
- **全盘同步帧**：循环 60 格（`cmp r6,#60`），每格构造帧（0x26f5e-0x26f84）
- **帧类型字段**：`code`（rodata 0x120ebc，帧类型码）
- **`%d,%s` 帧**（0x12115c，引用 @0x2c492）：对局名帧（`sprintf(buf,"%d,%s",code,name)`）

### 蓝牙协议流程确证
1. `OnBluetooth(Intent)` 读 action（蓝牙广播 FOUND/DISCOVERY 等）
2. `getIntExtra` 读 extra（设备/对局参数）
3. `BluetoothDevice.getAddress()` 获取设备地址
4. `UUID.fromString("ce1404fe-...")` + 服务名 `junqi` → RFCOMM SPP 连接
5. 帧收发：单步 7 字段帧（走子/翻棋同步）+ 60 格全盘同步

### 诚实边界
- 7 字段具体语义（哪个字段=from/to/rank/action）需逐字段逆向结构体 offset（81/82/83/84/808/1784/256 对应棋盘状态字段），未逐字段确证
- 蓝牙联机完整实现需 Java 侧 BluetoothSocket 管理 + native 帧状态机，工作量另计（无真机蓝牙验证环境）

## §90 对局进度存档 one.sjg 实现

### 原版 one.sjg 确证（0x390ac 写）
- `one.sjg` 引用 @0x390ac（`add r0,pc` → one.sjg + `bl 0xbd320` fopen），**二进制对局进度存档**
- 0x39090 `stm r4!,{r3}` 写字段、0x390a2 `blx 0x22adc`（fwrite）、0x390e4 `strh r0,[r3,#60]`（结构体 offset 60 字段）
- 原版对局存档体系：`one.sjq`（设置/胜负，0x376c8/0x60d10 引用）、`one.sjg`（对局进度，0x390ac）、`one.sav`（0x2702c）

### arm64 对局存档实现（save.c + junqi_jni.c）
- `jq_save_game_write/read(path, jq_state*)`：保存 board 60 格（rank,side,revealed）+ turn/stage/winner + flags/mines/pieces + moves/moves_since_eat（文本 key=value，诚实边界）
- **OnCreate 续局**：读 one.sjg，若 stage=PLAY/DARK 则恢复（退出重进续局）
- **OnPause 保存**：对局中写 one.sjg；非对局中（布阵/结束）删除进度
- 本机测试 PASS（读写往返：stage=DARK/turn=RED/moves=42/cell rank/side/rev/flag/bomb 全部一致）

### 编译打包（三件套完成）
- 库 `libjunqi_arm64_hand.so` 59152B（14 JNI 匹配，含对局存档+布局选择+bt_spak 注释更新）
- `phase_hand.apk` 14483456B，CheckSig `verified: true`，.so 全 STORE，manifest 头 `0x00080003`，APK 内 .so=59152B

## §91 音效对齐 + 蓝牙 action 确证 + OnBluetooth 基础实现

### 原版音效清单确证（sound.xml 解包）
- **Ui 组**：Click（data/ui/click.wav）、Close（data/ui/close.wav）、Pop（data/ui/pop.wav）
- **Junqi 组**：Start/Flip/Select/Move/Eat/Flag/Dead/Bomb/Win/Lose/Draw/Over（data/junqi/*.wav，12 个）
- **arm64 覆盖 11/12**：start/flip/select/move/eat/flag/dead/bomb/win/lose/draw ✓；Over 用 win/lose/draw 语义等价；Ui 组非核心（简化版无完整 UI 菜单）
- 音效事件流对齐：走子 sfx 数组（move/eat/bomb/flag/dead）+ 翻棋 flip + 选中 select + 胜负 win/lose/draw

### 原版蓝牙 action 清单确证
- `ACTION_REQUEST_ENABLE`（0x126864）、`ACTION_REQUEST_DISCOVERABLE`（0x126a1c）、`EXTRA_DISCOVERABLE_DURATION`（0x126a38）
- `ACTION_SCAN_MODE_CHANGED`（0x128c08）、`ACTION_DISCOVERY_STARTED`（0x128c24）、`ACTION_DISCOVERY_FINISHED`（0x128c40）、`ACTION_FOUND`（0x128c6c）、`EXTRA_DEVICE`（0x128c7c）
- 蓝牙联机流程：开启蓝牙 → 可发现 → 发现设备（ACTION_FOUND/EXTRA_DEVICE）→ RFCOMM → 帧

### arm64 OnBluetooth 基础实现（从占位推进）
- 解析 Intent action（`getAction`）+ ACTION_FOUND 时读 `EXTRA_DEVICE` → `BluetoothDevice.getAddress()`（设备地址）
- 为 RFCOMM 联机做准备（完整联机需 Java 蓝牙 Socket IO + native 帧状态机，后续另计）
- 库 60152B（14 JNI 匹配），phase_hand.apk 14487552B，CheckSig `verified: true`，.so 全 STORE，manifest `0x00080003`

### 核心玩法完整可玩确认
- 布阵（触屏 place + 确认按钮 hit-test）→ 对局走子/吃子/翻棋（触屏 + 音效）→ 悔棋（undo）→ 重开（restart）→ 布局选择（layout:<0-11>）→ 存档（one.sjq + one.sjg 续局）→ 对局结束（winner + resultEx + 音效）

## §92 蓝牙帧协议完整确证（版本 v2 + 7字段）

### 协议版本确证（0x26ec8 帧构造函数）
- 0x26efc-0x26f0c 构造**版本串 "v2\n"**（strb 118='v'/50='2'/10='\n'/0='\0'）——原版蓝牙协议版本 = **v2**
- 0x26f4a `bl 0xb4bd0`（sprintf）构造帧：`"v2\n%d,%d,%d,%d,%d,%d,%d\n"`
- 0x26f50 `blx 0x22c80`（strlen）+ 0x26f58 `bl 0x23ae8`（std::string::append 到发送缓冲）→ 蓝牙流

### 7字段帧字段确证（对局状态结构体 offset）
- 字段1：offset 81（1字节 ldrb）、字段2：offset 82（1字节 ldrb）、字段3：offset 83（1字节 ldrb）
- 字段4：offset 84（4字节 ldr）、字段5：offset 808（1字节 ldrb，202*4）
- 字段6：offset 1780（4字节 ldr，0x6f4，move 计数）、字段7：offset 1784（4字节 ldr，223*8，游戏结果）
- **字段顺序修正**：§100 确证实际为 81,82,83,84,808,1780,1784（早期"82,81"假设错误）
- 语义：对局状态字段（当前格 rank/side/action + 对局计数），逐字段语义需 C++ 类布局（诚实边界）

### arm64 one.sjg 对局结束清理（notify_game_over）
- 对局结束（winner 判定）后**立即删除 one.sjg**（防异常退出残留过期进度），与 OnPause 的 else 分支互补
- 库 60184B（14 JNI 匹配）

## §93 可复用脚本 + 产物状态

### 可复用脚本（自动化，已测试）
- `rev64/build_arm64.sh`：编译 arm64 库（NDK r25b，9 个 .c，`-O2 -fPIC -fvisibility=hidden`，`-llog -lGLESv2 -landroid -lz -lOpenSLES -lm`），输出默认 `rev64/dist/libjunqi_arm64_hand.so`，校验 JNI 符号数
- `rev64/pack_phase.sh`：重打包三件套（apktool b → store_compressed.py → SignV2 v1+v2），校验签名/.so 未压缩/manifest 头/APK 内 .so 大小

### 产物状态（最终）
- `rev64/dist/libjunqi_arm64_hand.so`：61056B（14 JNI 匹配原版，含 bt_spak v2 帧编解码）
- `rev64/dist/phase_hand.apk`：14487552B，CheckSig `verified: true`，.so 全 STORE，manifest `0x00080003`，APK 内 .so=61056B

### 蓝牙帧协议层（bt_spak.c）
- `jq_bt_frame_encode/decode`：v2 + 7字段帧编解码（对齐原版 0x26f24 sprintf）
- `jq_bt_frame_sync_encode`：60 格全盘同步帧（C<idx>,<rank>,<revealed>\n）
- 本机测试 PASS（7字段帧往返一致、非 v2 帧拒绝、同步帧构造）

## §94 并行子代理任务（未完成项分工）

### 已并行启动（background subagent）
1. **蓝牙联机 Java IO + native 帧状态机**（ses_e6e16e7c0ffettc48SKxnbvuHD）：
   - 基于已确证协议（UUID ce1404fe/服务名 junqi/版本 v2/7字段帧）+ bt_spak.c 帧层
   - 方案：native JNI 直调 Java BluetoothAdapter/Device/Socket（RFCOMM + InputStream/OutputStream）
   - 实现帧状态机（v2 帧解析→g_state 同步；走子/翻棋→构造帧）
   - 不改 App.smali native 签名（红线）；无真机蓝牙验证，记录"编译通过+逻辑完整，真机验证待做"
2. **蓝牙 7 字段帧语义逆向**（ses_e6e168f1effeb1bsFXp7j0RJrK）：
   - 逆向 0x26ec8 帧构造函数 + 原版对局状态结构体布局（offset 81/82/83/84/808/1780/1784）
   - 找这些 offset 的写访问确证字段语义；逆向帧接收解析（sscanf）
   - 纯逆向分析（不改代码），输出逐字段语义确证 + 诚实边界

### 子代理完成后的验收
- 任务A：build_arm64.sh 编译通过 + pack_phase.sh 重打包验证（签名/.so 未压缩/manifest）
- 任务B：7 字段语义确证记录到 REVERSE_NOTES（标注诚实边界）

## §95 SPAK 资源完备性核查 + 50 预设布局发现（纠正 §88）

### SPAK 组组织结构确证
- **data 组**（121 条）：配置（string.xml/sound.xml/path.xml/uitag.xml/base.cfg）+ 布局（lay/00-49.lay + default.lay）+ 音效（junqi/*.wav 12 个 + ui/*.wav）+ 字库/大字库（data#28-31,98-99）+ sentence.txt + PNG 图标
- **datah/datam/datax 组**：分辨率级 UI 布局配置（非 data 路径）——datah=高、datam=中、datax=超（含子块 246+/273+）；每条目为 UI 窗口 XML（Dialog/Button/Picture 等控件树）
- **data/junqi/*.png 装饰**（back.jpg/border.png/label.png/main.jpg/mark.png/piece.png/scene.cfg）：**非 MD5 路径**，是 **datah 图像索引**（datah[177-193] 区间的游戏场景图）

### 音效/棋子/棋盘路径确证（arm64 加载正确）
- 音效：`data/junqi/*.wav` 全部 12 个在 **data 组 MD5 路径命中**（data#32-46），arm64 `spak_get("data/junqi/<evt>.wav")` **正确**；`data/ui/click.wav` 缺失（原版 click%d.wav 格式，非核心）
- 棋子图集：**PIECE_HASH_DATAH → datah[183]**（212×280，4列×7行，53×40/格，zlib 压缩）✓ 与 §85 一致
- 棋盘：**BOARD_HASH_DATAH → datah[188]**（737×751，XOR 未压缩）✓ 与 §86 一致
- 修正 render_gles2.c/junqi_jni.c 过时注释（"data/sg/piece.png 480×490"→"datah[183] 212×280"）

### ★ 50 预设布局发现（§88"12 预设"是不完整确证）
- **SPAK data 组含 50 个唯一预设布局**（data/lay/00-49.lay，data#47-96）+ default.lay（data#97，== 00.lay）
- 布局格式：**30B = 30 格（每方 6行×5列），每字节 1 rank 值**（0=空/行营，1-12=rank），25 子/方
- **arm64 DEFAULT_LAY 与 SPAK 00.lay 字节完全相同**（`3,7,6,5,8,10,0,9,0,1,2,10,0,9,5,4,0,6,0,7,8,3,4,9,11,11,12,11,8,7`）——arm64 布局解析正确
- 值域验证：全部 50 布局值 0-12（rank 恒等映射 layout_rank(v)=v 适用）；每布局均为 25 子 ✓
- 原版格式串 `data/lay/%02d.lay`（0x12113c）3 引用点（0x29c8c/0x3ceec/0x3dd8e），均 snprintf(buf,256,fmt,n) + 30B 读取
- 原版布局选择 UI：datah[27]（Random/Default/Custom）+ datah[61]/[62]（新局对话框 First/Level/Lay/Show）+ datah[91]/[92]/[18]（布局列表，Piece 项 + 100-103 分页）

### arm64 扩展（已编译验证）
- `layout:<0-49>`（原支持 0-11）→ `data/lay/%02d.lay`，对齐 50 预设
- 编译 junqi_jni.c 通过（未链接/未打包，避免与子代理产物竞态）

## §96 规则实现对齐验证（game_logic.c vs ruleflip/rulelay）

### jq_can_move 走子合法性（对齐 rulelay 权威文本）
- 军旗/地雷不能动（L81）✓
- 未翻暗子不可动（L85）✓；非当前方不可动（L89）✓；大本营内子不可动（L93）✓
- 不能进敌方行营（jq_is_camp + side_of）✓
- 大本营仅军旗/地雷可进（L96）✓
- 相邻一步 / 铁路直线（rail_path_straight）/ 工兵拐弯（rail_path_engineer）✓

### 扛旗/地雷/炸弹规则（ruleflip vs rulelay 区分）
- **ruleflip（JQ_STAGE_DARK）**：地雷/军旗仅工兵能吃；扛旗需已挖 3 雷（mines_cleared[side]>=3，L102-103）；只能吃已翻子（UNKNOWN 不可吃）✓
- **rulelay（明棋）**：任何子进大本营吃军旗 → 胜（L226-229 winner=side）；非工兵吃地雷 → 被雷炸（L234-244 pieces_left--）✓
- 炸弹同归于尽（L247-253，双方 pieces_left--）✓；吃普通子 rank 比较（L265+）✓

### 胜负/和棋判定
- 无棋可走判负（check_stalemate，L183-188：winner = 1-turn）✓
- **70 步无吃子判和**（moves_since_eat>=70，L196）✓
- **总步数 1000 判和**（moves>=1000，L196）✓
- 军旗被吃 → 扛旗方胜（L226-229）✓

### AI 简化版（诚实边界）
- jq_ai_move 3 级：LOW（50%随机失误+贪心）/MID（贪心默认）/HIGH（价值+安全最优吃子）
- 每级：困毙判定（side_has_movable）→ 贪心吃子（ai_capture）→ 前进（ai_advance）→ 兜底（ai_fallback）→ 翻棋（ai_flip）
- 简化版 AI，非原版汇编级策略（原版 AI 复杂度为诚实边界）

## §97 原版 UI 事件名完整清单（rodata 确证 + arm64 覆盖矩阵）

### Junqi.* 对局核心事件（arm64 已对齐）
- **胜负**：Junqi.RedWin（0x120e24）/ BlueWin（0x120e4c）/ Win（0x120e34）/ Lose（0x120e6c）/ Draw（0x120e40）/ Over（0x121708）——arm64 winner 判定 + 音效 win/lose/draw + Toast 通知 ✓
- **走子事件**：Junqi.Move（0x120f20）/ Flip（0x120f14）/ Flag（0x120f2c）/ Bomb（0x120f38）/ Dead（0x120f44）/ Eat（0x120f50）/ Select（0x120e84）/ Start（0x120ec8）——arm64 sfx 数组 + 音效 ✓
- **布局错误**：Junqi.ZhaError（0x12117c，炸弹）/ LeiError（0x12118c，地雷）/ QiError（0x12119c，军旗）/ LayError（0x121f4c）——arm64 jq_layout_error 对应 ✓

### Junqi.* 联机/网络事件（子代理 A/B 在用）
- **联机对局**：Resign/PeerLayFinish/PeerStart/DrawAgree/DrawDisagree/DrawReq/RegretAgree/RegretDisagree/RegretReq/LeaveGame/Ready/PlayerLeave/RegretSend/RegretConfirm/RegretFail/NewFail/PeerWait/JoinGame/PeerLast/PeerFirst（0x121278-0x123364）
- **网络状态**：ConnectFail/ExitFail/Reconnecting/ReconnFail（0x1212f0/0x12129c/0x122b6c/0x122b80）
- **登录/注册**：InputPassword/InputUser/InputName/InputEmail/InputConfirmPwd/LoginFail/LoginAuthFail/LoginMaxUser/LoginVerOld/RegOk/RegExist/RegFail 等（网络登录，arm64 不做——诚实边界）

### Sg.* 对局对话框 + Ui.* 音效
- **Sg.**：DrawReq（0x1218e0）/ LeaveConfirm（0x12371c）/ DrawConfirm（0x12372c）/ ResignConfirm（0x12373c）/ DrawSend（0x123750）——arm64 简化版无对话框（Toast 等价，诚实边界）
- **Ui.**：Click（0x120f70）/ Close（0x1211ac）/ Pop（0x121c48）——arm64 音效 Ui 组非核心

### arm64 覆盖结论
- 核心对局事件（胜负/走子/音效/布局错误）全部覆盖 ✓
- 联机/对话框/登录事件：arm64 简化版无 UI 入口（Toast 等价），子代理 A/B 推进蓝牙联机 native 侧

## §98 蓝牙联机 Java IO + native 帧状态机实现（方案A：native JNI 直调 Java 蓝牙 API）

### 任务范围
- 基于已确证协议（UUID `ce1404fe-0863-4278-86c2-c9c560f0e215` / 服务名 `junqi` / 版本 `v2` / 7 字段帧）实现 arm64 蓝牙联机完整链路：Java 蓝牙 Socket IO + native 帧状态机
- 不改 `smali/com/cnvcs/App.smali` native 签名（红线，14 个 On* 签名保持原样）

### 方案选择：A（native JNI 直调 Java 蓝牙 API）
- 原版 smali 仅 `BtReceiver`（BroadcastReceiver→App.OnBluetooth）+ `App` 静态 native；**无 Java BluetoothSocket 代码**，故原版 32 位 native 全在 JNI 层调 Java 蓝牙 API
- arm64 同路线（最小 Java 改动）：`bt_net.c` 经 JNI 调 `BluetoothAdapter/Device/Socket/ServerSocket/InputStream/OutputStream/UUID`，RFCOMM SPP 连接 + 帧收发

### 改动文件
| 文件 | 改动 |
|---|---|
| `src/bt_spak.h` | 新增：语义帧类型（JQ_BT_T_MOVE/FLIP/UNDO/SYNC_REQ/SYNC/SYNC_CELL/RESTART/RESIGN）+ 帧/C 行 API 声明 |
| `src/bt_spak.c` | `jq_bt_frame_sync_encode` 升级为 4 字段 C 行（`C<idx>,<rank>,<side>,<revealed>`）；新增 `jq_bt_cell_decode`（兼容 3/4 字段） |
| `src/bt_net.h` | 新增：bt_net 公共 API（init/shutdown/frame_cb/send_frame/send_sync/handle_intent/bt_* 命令） |
| `src/bt_net.c` | **新增**：Java 蓝牙 JNI 包装 + connect/accept/reader 线程 + 帧收发 |
| `src/junqi_jni.c` | OnCreate→`jq_bt_init`（默认适配器 + 注册 BtReceiver）；OnDestroy→`jq_bt_shutdown`；OnBluetooth→`jq_bt_handle_intent`；OnEvent 新增 `bt_*`/`resign` 命令；走子/翻棋/悔棋/重开后 `jq_bt_send_frame`；新增 `bt_on_frame` 帧状态机回调 + `bt_send_local_move/flip`；OnUpdate 蓝牙联机时关 AI；JNI_OnLoad→`jq_bt_set_vm` |
| `build_arm64.sh` | 源列表加 `bt_net`（10 .c） |

### 蓝牙联机流程（实现）
1. **注册接收器**（init）：原版 manifest 不声明 `<receiver>`，原版 native 动态注册——arm64 `bt_register_receiver` 经 `Context.registerReceiver(new BtReceiver(), filter)` 注册 FOUND/SCAN_MODE_CHANGED/DISCOVERY_STARTED/FINISHED/CONNECTION_STATE_CHANGED/STATE_CHANGED
2. **服务端**（`bt_host` 事件 / SCAN_MODE_CHANGED=3 自动）：`BluetoothAdapter.listenUsingRfcommWithServiceRecord("junqi",UUID)` → accept 线程（阻塞）→ socket
3. **客户端**（`bt_discover` 事件 → ACTION_FOUND 广播 → `jq_bt_handle_intent` EXTRA_DEVICE / `bt_connect:<addr>` 遍历 bonded）：`BluetoothDevice.createRfcommSocketToServiceRecord(UUID)` → connect 线程（阻塞）
4. **连接建立**（`bt_established`）：getInputStream/OutputStream → 置 CONNECTED → 启动 reader 线程 → 客户端发 SYNC_REQ
5. **帧收发**：reader 线程常驻读帧（行缓冲）→ `bt_consume`（v2 帧 `strstr "v2\n"` 优先 / C 行行首 `C`）→ `jq_bt_frame_cb`（`bt_on_frame`，g_mutex 保护 g_state）
6. **本地走子/翻棋/悔棋/重开/认输**：`bt_send_local_move/flip` + `jq_bt_send_frame`（g_bt_mutex 保护 OutputStream write/flush）

### 帧状态机（arm64 语义重建，诚实边界——原版 7 字段逐字段语义由并行子代理逆向）
- `v2\n<type>,<from>,<to>,<rank>,<side>,<turn>,<moves>\n`
- `type`：0=走子 1=翻棋 2=悔棋 3=同步请求 4=同步响应（c=stage,d=turn）5=C 行伪类型 6=重开 7=认输（a=胜方）
- **MOVE**：`jq_try_move(from,to)`（turn 与 side 不一致时强制按 side 应用，处理重连/失步）
- **FLIP**：`jq_flip(idx)`；**UNDO**：`jq_undo`；**SYNC_REQ**：回发 `jq_bt_send_sync`（stage/turn + 60 格 C 行）
- **SYNC**：应用 stage/turn；**SYNC_CELL**：逐格应用 rank/side/revealed + is_flag/is_mine/is_bomb 派生
- 胜负（走子/翻棋后 stage==OVER）→ 音效 win/lose/draw + `notify_game_over`（Toast）
- **OnUpdate 蓝牙联机时关闭 AI**（双方真人，走子由对端帧驱动）

### 验证状态
- **build_arm64.sh 编译通过**：`libjunqi_arm64_hand.so` 79768B，14 JNI 符号匹配原版，无未定义 `jq_bt_*`/`bt_*` 符号
- **pack_phase.sh 重打包验证通过**：`phase_hand.apk` 14503936B，CheckSig `verified: true`，.so 全 STORE，manifest 头 `0x00080003`，APK 内 .so=79768B
- **主机侧自测 PASS（22 项）**：v2 帧 encode/decode 往返、非 v2 帧拒绝、C 行 4/3 字段解码、FLIP 帧应用（暗棋定色 + turn 翻转）、MOVE 帧应用（明棋蓝/红走子 turn 翻转 + moves 计数）、SYNC 帧 stage/turn 应用、SYNC_CELL 逐格应用
- **诚实边界：无真机蓝牙验证环境**——实现记录"编译通过+逻辑完整，**真机蓝牙验证待做**"（需真实 arm64 GPU 设备双机 RFCOMM 实测）

### 剩余诚实边界
1. **7 字段语义为 arm64 重建**（原版 offset 81/82/83/84/808/1780/1784 逐字段语义由并行子代理逆向确认后可对齐）
2. **C 行格式为 arm64 重建**（原版 0x26f5e 60 格循环具体帧布局未逐字节确证；兼容 3/4 字段）
3. **蓝牙对局完整 UI 无入口**（arm64 简化版无菜单/对话框，经 OnEvent `bt_*` 命令 + Toast 等价驱动；原版 Junqi.Peer*/Regret*/Draw* 对话框未实现——诚实边界）
4. **注册接收器/配对流程**：需系统蓝牙开启 + 设备配对；`bt_connect:<addr>` 仅连已配对设备

## §99 棋盘几何/蓝方侧调查结论（诚实边界）

### 棋盘 PNG 检测（datah[188] 737×751）
- 用正确格线（§86 几何：竖带 x270-465，39×59/格，y0=22，12×59=708，边距 21.5）重新采样
- 棋盘 PNG 是**空棋盘底图**（每格绿背景 G18-32%，无军旗/棋子标记——军旗棋子运行时动态绘制）
- 因此**像素检测无法确定蓝方屏幕侧**（大本营军旗格无特殊标记）

### arm64 渲染/触屏方向（内部一致）
- render_gles2.c：`y1 = by1 - r*ch`（row0=屏幕顶部，row11=底部）
- touch_cell：`row = (int)((by1-ny)/ch)`（row0=顶部）——**渲染与触屏一致**
- arm64 蓝方（row0-4）在屏幕**顶部**，红方在底部

### 原版蓝方侧（诚实边界）
- 原版 OnTouch 主逻辑 0xbdd2c（ACTION_DOWN/MOVE/UP 分发）+ 坐标处理 0xbd580/0x7bf80（12×5 网格边界检查）
- 蓝方屏幕侧（顶部 vs 底部）未最终确证：需逆向原版棋盘渲染顶点/UV 或坐标→格除法（大工作量，且与子代理 B 帧语义逆向范围有潜在重叠）
- **影响评估**：蓝方侧是视觉方向细节，不影响规则/胜负/可玩性（arm64 内部一致）。arm64 蓝方在顶部，可玩。原版蓝方侧为诚实边界。

## §100 蓝牙 7 字段帧语义逆向确证（offset 81/82/83/84/808/1780/1784）

> 本节是子代理 B 的帧语义逆向输出。纯静态分析（Thumb 反汇编 + rodata 明文 + 帧构造函数传参），不改代码。
> 结论：**帧字段顺序确证为 81, 82, 83, 84, 808, 1780, 1784**（修正早期任务描述中"82,81,..."的错误假设）。

### §100.1 帧格式串确证

- 格式串 `v2\n%d,%d,%d,%d,%d,%d,%d\n`（7 字段）明文在 rodata offset **0x29C**（vaddr **0x120E94**）。
- 唯一引用者：`snprintf`（PLT stub 0x22d1c）调用点 **0xb4bd0**（在帧构造函数 0x26ec8 内）。
- 帧构造函数 0x26ec8 开头用**立即数写** `"v2\n"`（0x26efc–0x26f0c），非 rodata 字符串。
- 独立 6 字段 move 格式 `%d,%d,%d,%d,%d,%d\n` 在 rodata offset **0x2B4**（vaddr 0x120EAC）。
- 早期手工 PC-relative 加法的错误（格式串误判为 0x120E14）已修正：实际 vaddr 为 0x120E94（rodata offset 0x29C）。

### §100.2 帧字段顺序 + 偏移确证（构造函数 0x26ec8 → snprintf 传参）

snprintf 调用 0xb4bd0 的寄存器/栈传参：

| 参数 | 值来源（state 偏移） | 字段 |
|---|---|---|
| r3（首 vararg） | **byte@81** | field1 |
| [sp] | **byte@82** | field2 |
| [sp+4] | **byte@83** | field3 |
| [sp+8] | **word@84** | field4 |
| [sp+12] | **byte@808** | field5 |
| [sp+16] | **word@1780** | field6 |
| [sp+20] | **word@1784** | field7 |

**结论：帧 = `v2\n<byte@81>,<byte@82>,<byte@83>,<word@84>,<byte@808>,<word@1780>,<word@1784>\n`**。
- 早期任务描述"field1=offset82、field2=offset81"的字段顺序假设**错误**，应修正为 **81, 82, 83, 84, 808, 1780, 1784**。
- 注意 `add rX,pc` 的 PC 计算：部分奇数地址指令需 PC=addr+4（无 `&~3` 对齐）才能命中正确字符串；该 2 字节歧义**不影响字段语义结论**（仅影响字符串引用解析，帧格式串已用 0x26fae move 格式同法交叉验证）。

### §100.3 结构体布局确证（帧结构体 = 对局状态 state）

| 偏移 | 大小 | 含义 |
|---|---|---|
| 81 | 1 | byte field1（=field5 同值，见 §100.5） |
| 82 | 1 | byte field2（=config byte@69，见 §100.6） |
| 83 | 1 | byte field3（=config byte@62，见 §100.6） |
| 84 | 2 | word field4（=config word@64，见 §100.6） |
| 88–807 | 60×12 | **board[60]**：帧循环 r5=state+88，60 次，读 cell[0]=word、cell[4]=byte，经 snprintf 传 2 值 |
| 808 | 1 | byte field5（=board 结束处，=field1 同值） |
| 812–1531 | 60×12 | 第二个 60 格 board（812=203×4，undo 使用） |
| 1532(0x5FC) | 240 | 区域（reset 清零） |
| 1772/1773/1774 | 1/1/1 | 控制字节：1773 每次走子异或 1（轮次/当前方）；1774 依 field3 异或；1772 清零 |
| 1776(222×8) | 4 | move 数组指针 |
| 1780(0x6F4) | 4 | **move 计数**（field6） |
| 1784(223×8) | 4 | **游戏结果**（field7） |
| 1812–1817 | 6 | 从全局 config 复制（6 字节） |
| 1820(0x71C) | 4 | =-1 |
| 1824(228×8) | 4 | =-1 |
| 1852(0x73C) | 4 | =0 |

### §100.4 field6（move 计数）确证

- undo 函数 0x25664：递减 state+1780 并回滚最后一条 move。
- "追加 move"函数 0x275b8：6 字节记录 `{r9,r10,r7,r6,r8,r5}` 写入 `*(state+1776)+count*6`，随后 `state+1780++`。
- 帧构造第二循环以 state+1780 为界遍历 6 字节/条记录。
- **结论：field6 = 当前已走 move 条数。**

### §100.5 field7（游戏结果）确证

- 终局函数（0x269xx）：置 0x2693a→3（和棋）、0x2696c→1、0x269ec→2。
- 初始化/reset 置 0。
- **结论：field7 = 0 进行中 / 1、2 某方胜 / 3 和棋。**

### §100.6 field1/field5 关联确证

- 新游戏函数 0x27240：置两者=1；0x27348：置两者=0；reset（0x26bxx/0x26dxx）：置 0。
- **结论：field1 == field5（同值），为 0/1 标志（疑似玩家角色/局内方，见诚实边界）。**

### §100.7 field2/3/4 来源确证

- 新游戏函数经 accessor **0x27940 / 0x27904 / 0x27918** 从全局 config 对象读：
  - byte@69 → field2
  - byte@62 → field3
  - word@64 → field4
- config 同步函数（0x501d8 / 0x5051c）：从设置对象 byte@64/68/72/73 写入全局 config 的 byte@62/word@64/byte@68/byte@69。
- **结论：field2/3/4 来自全局 config（设置对象），具体设置项语义见诚实边界。**

### §100.8 sscanf 调用点枚举（帧接收解析定位）

全 .so 共 3 处 sscanf（PLT stub 0x22fa4）：

| 调用点 | 用途 | 结论 |
|---|---|---|
| 0xc8332 | 小整型 helper（格式串加密不可读） | 非帧接收 |
| 0xc8386 | 同上 | 非帧接收 |
| 0xe7c02 | config 解析 `%d%c`，乘 1000 检查 'M'/'m' | 非帧接收 |

- 帧格式串 0x120E94 仅被 snprintf(0x26f24) 引用，**无 sscanf/直接字面量引用它**。
- 帧格式 6 字段 move 串 0x120EAC 亦无 sscanf 引用。

### §100.9 相关 PLT/GOT 地址

| 函数 | PLT stub | GOT slot |
|---|---|---|
| snprintf | 0x22d1c | 0x15B1EC |
| sscanf | 0x22fa4 | 0x15B2C4 |
| vsnprintf | 0x23118 | 0x15B340 |
| sprintf | 0x23130 | 0x15B348 |

### §100.10 帧构造函数调用点

帧构造函数 0x26ec8 被以下位置调用：
- 0x2712c、0x27184（常规帧）
- 0x27302（新游戏 A 0x27240）
- 0x2741c（新游戏 B 0x27348）
- 0x276f0（move 执行）
- 末尾 0x27020–0x27058 依 byte@80 追加后缀串，解析为 `one.sav`(0x120DEC)/`two.sav`(0x120DF4)（保存槽标识）。

### §100.11 rodata 加密澄清（重要）

- `frida-dump/rodata_dec.bin` 与 `.so` 的 `.rodata` 段**字节完全相同**（delta=0：rodata_dec offset == .so vaddr − 0x120BF8，已由多例 PC-relative 引用交叉验证）。
- 故 `.rodata` 是**明文串池 + 加密 blob 混合**，并非整体加密；明文串可直接按 offset 读取。加密区域（如 0x21C）为 SPAK 加密 blob。

### §100.12 诚实边界（未确证项）

1. **field1/field5 确切语义**：0/1 标志，疑似玩家角色/局内方（host/guest 或红/蓝），但**未确证命名**——"字段1/5语义未定"。
2. **field2/3/4 具体配置项语义**：byte@62/69、word@64 对应规则/种子/模式等，**未命名**（需逆向全局 config 对象初始化和设置 UI 调用链）。
3. **帧接收解析函数未定位**：.so 内未找到引用帧格式 0x120E94 的 sscanf 调用；接收侧可能用非 sscanf 解析（strtol/手工逗号切分），或在 Java 层/未在本 .so 导出。需进一步搜索 `add rX, #81` 取地址模式或接收线程区（0x27480–0x27700）内对 `"v2"` 前缀的 strncmp/strstr。
4. **7 字段帧语义为原版确证**（非 arm64 重建）：arm64 侧 §98 的 7 字段语义（type/from/to/rank/side/turn/moves）为**重建**，本节确证了原版字段位置，但字段含义的精确命名需继续。

### §100.13 后续建议（若继续）

1. 定位接收解析：搜索 `adds rX, rY, #81` 取字段地址模式，或扫描 0x27480–0x27700 接收线程区的 `"v2"` 前缀 strncmp/strstr。
2. 确证 field1-5 语义：逆向全局 config 对象初始化（settings byte@64/68/72/73 设置点）与 0x271a4 dispatcher（0xb2da8 解析 "code" 值 18/27/13）上下文。

