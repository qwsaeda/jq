# 踩坑经验（LESSONS）——后续阶段复用的关键教训

> 每条都是真金白银的返工换来的。**移植/验证前必读。**

## L1 · 触摸失效 vs 崩溃：`[sl+76]` 标志
- 大函数 `0x24400` 路径：`246ba strcmp` 判定 → `246c0 beq 246c4`(设 `[sl+76]=1`) /
  `246c2 b 24864`(内部 strcmp，`!=0` 走 `24872 b 246c6` 设 `[sl+76]=0`)。
- **`[sl+76]=1` 禁用触摸**；`[sl+76]=0` 正常。强制走 `246c4`（patch 0x246c0）=
  触摸失效（实测 t19fix4 证实）。**原版正确路径是 `24864` 分支且 strcmp!=0**。
- 教训：改分支前先确认目标路径是"正常路径"还是"错误路径"，不能凭"名字像成功"判断。

## L2 · 跳过签名校验的副作用
- 完全跳过 `0xbcad8`（签名校验）会导致其副作用缺失，`r7/r6` 未初始化。
- targetSdk26 下 `0x24868 strcmp(r7,r6)` 读垃圾 → **SIGSEGV**。
- targetSdk19 下恰好不崩（分支不同），造成"能玩"假象——**不同 targetSdk 行为不同**。
- 教训：native 行为依赖 targetSdk（读 ActivityInfo.targetSdkVersion 分支），
  改 targetSdk 必须重测全部路径。

## L3 · 字节 patch 破坏 ELF 尺寸（最阴险）
- patch `0x24866`（2字节 `adds r1,r6`）时误写 4 字节 `bf00bf00`，
  **bytearray 切片赋值长度不匹配 → 插入 2 字节 → 文件变 1435158 → dlopen failed**。
- 症状：`UnsatisfiedLinkError: dlopen failed`，但 zip 里 lib 仍 STORE、ELF 头正常、
  签名验证通过——**一切静态检查都过，就是加载失败**。
- 教训：patch 前先 `assert d[a:b]==预期长度`，patch 后 `assert len(d)==原长`。
  切片赋值必须 `slice_len == len(new_bytes)`。

## L4 · objdump 输出解析（R0 反复踩）
- 指令行：`   233e0:\tb510      \tpush\t{r4, lr}` = 3空格 + 7位hex + `:` + tab。
- body 按 `\t` 拆分：`[0]=字节hex [1]=助记符 [2]=操作数`。
- 用 `startswith('push')` 或 `startswith('bl')` 会失败（body 以字节hex开头）。
- **教训：任何 objdump 解析都必须按 tab 拆结构，不能对整行做前缀匹配。**

## L5 · 安装失败排查顺序（AGENTS.md 已载，实测再证）
① `.so` 是否 zip 压缩（必须 STORE）→ ② 是否缺 v2 签名 → ③ 32位库在64位only设备
→ ④ manifest 是否 0x00080003（apktool 误打包成文本）。
验证 manifest 头：`python3 -c "import struct,zipfile;print(hex(struct.unpack_from('<I',zipfile.ZipFile('X.apk').read('AndroidManifest.xml'),0)[0]))"`。

## L6 · mumu 输入注入
- `input tap`（DOWN+UP 2事件）native 收到但可能不触发；`input swipe 同点 200ms`
  产生完整 DOWN+MOVE+UP 序列更可靠。
- 军棋不在真正前台时 `input tap` 被 InputDispatcher 丢弃（"no touchable window"），
  **须先 `am start` 确认 topResumedActivity=军棋** 再点击。
- 主界面 1154249B / 对局 749672B / 带弹窗 677498B / 翻棋后 714710B（截图判据）。

## L7 · 真机 32 位警告
- 64 位处理器真机装纯 32 位包会弹"未适配64位处理器"警告（系统提示，非硬拒绝），
  但用户不愿接受 → 触发本次 arm64 完整移植工程。

## L8 · targetSdk19 vs 26 触摸差异（R1 前必记）
- `ctl_skip`(t19+3patch) 能玩；同 patch 换 targetSdk26 触摸失效/崩。
- App.smali 方案B（`if-nez v0`+`new GLView`+`setContentView`）也破坏触摸，
  **必须用原版 App.smali**（直接 `setOnTouchListener(v0)`，无 if-nez 块）。

## L9 · ARM Thumb→AArch64 二进制翻译器关键修复（R1）
1. **RE_REG 必须非捕获组 `(?:...)`**：多寄存器 regex 若用捕获组，外层 group 编号错位
   → 生成错误代码（`subw r0,r0,#2065`→`#r0`）。全局正确性关键。
2. **标签零填充**：`.Lf_{addr:08x}`（零填充）与分支引用 `.Lf_<raw>`（无填充）不一致
   → undefined 符号。`lf()` 用 `t.zfill(8)`。
3. **伪分支目标过滤**：反汇编数据误读产生 `bl 0xffc19040` 等 → `valid_tgt()`（0x22000-0x170000）过滤为 nop。
4. **分支目标占位**：objdump 未反汇编地址（数据洞/`bl` 中部）被分支引用 → 内联 `.Lf_:.inst nop` 占位，保持地址顺序（否则条件分支超 ±1MB 范围）。
5. **条件分支集合正则**：`re.match(r'b\.','bvc.n')`=False（助记符 `bvc.n`）→ 用 `[a-z]+\.(n|w)`。
6. **ldr/str 对齐**：arm64 严格对齐（`ldr w` 须 %4==0）vs Thumb 宽松 → 非对齐用 `add x16,xN,#off` 中转。
7. **add-pc 配对放宽**：不能只紧邻，须向前扫描（同基本块 rX 未改写）；OnEvent `ldr r3;adds;add r3,pc` 有间隔。2638→11352 对。
8. **`.w` 宽指令保守 nop**（`stmia.w`/`and.w`/`sbc.w` 带移位/大立即数）。
9. **逻辑立即数中转**：`and/eor/orr/bic #imm` 编码受限 → `mov w16,#imm` 中转。
10. **移位 #32→mod32**；**subw 在 sub 前**；**pop pc→ret**（x15 非真实 PC）；**add rd,rn,pc 3 元素形式**。

## L10 · 32 位 PC 相对全局数据在 64 位下的正确翻译（R1）
- 32 位 `.so` 的 `ldr rX,[pc,#imm]; add rX,pc` 计算**全局数据地址**（PC 相对偏移）。
- 32 位字面量（`.word offset`）放不下 64 位绝对地址 → **不能沿用原偏移**（会得 `arm64_pc+old_offset`，错）。
- 正确做法：配对后 `add rX,pc` → `adrp xX,.Lg_<target>; add xX,xX,:lo12:.Lg_<target>`（64 位绝对直算），
  前面的 `ldr rX,[pc,#imm]` 变 nop（死加载，值被覆盖）。
- 目标段分布：.bss 1180 / .rodata 1219 / .got 21（GOT 基址）/ .data.rel.ro 81 / .data 5 / .text 38。

## L11 · 32 位 GOT 槽 vs 64 位地址空间（R2 前必记）
- 外部函数调用走 `ldr rBase,[pc,#imm]; add rBase,pc; ldr rX,[rBase,#off]; blx rX`
  （rBase=GOT 基址 0x15b0c8，`ldr rX,[rBase,#off]` 是 **32 位加载**）。
- 64 位 Android 上函数/数据地址是 64 位 → 32 位 GOT 槽加载**截断**。
- 原 GOT 槽 4 字节间距 vs 64 位槽须 8 字节 → **布局不匹配**。需数据流跟踪 + 槽重映射，或改写 `:got:` 直取。
- 结论：**GOT 64 位化是 R2 硬任务**，不做则外部调用在运行时必崩。

## L12 · 外部函数调用走 blx→PLT（R2 关键发现）
- 32 位 .so 的外部调用是 `blx <PLT地址>`（6928 处，PLT 入口 0x229d4+12*i，i↔.rel.plt[i]）。
- 翻译后应 `blx <plt>` → `bl <符号>`（arm64 直接符号调用，链接器自动建 PLT/GOT/JUMP_SLOT）。
- 已验证：`bl malloc` 链接出 R_AARCH64_JUMP_SLOT，`.plt` 段自动生成（132 桩），9477 bl 到 PLT 范围。
- **不要**为 PLT 建 arm64 桩再跳（浪费且易错）；直接 `bl <符号>` 即可。

## L13 · __aeabi_* ARM EABI 助手在 arm64 不存在（R2）
- 原版 armeabi(软浮点)调用 __aeabi_fadd/fsub/fmul/fdiv/fcmp/uidiv/idiv/ldivmod/f2iz/...（34 个）。
- arm64 无这些符号（浮点/除原生）→ 必须提供兼容实现 `tools/aeabi_compat.S`。
- **关键**：32 位 EABI 浮点结果在 r0(位模式)；翻译代码用 w0 读结果。兼容函数须
  `fmov s0,w0; ...; fmov w0,s2`（经 w0 传位模式），**不能**用 C `float __aeabi_fadd()`（返回 s0，错）。

## L14 · NEON/协处理器"指令"全是字面量池数据（R2 关键发现）
- 1510 条 vrhadd/vtbx/stc2/mcr2/cdp... 是 objdump **把字面量池数据误译为指令**。
- 判定：`vrhadd.u16 d14,d12,d31` 在 8 个不同地址完全相同（=同一 4 字节常量）；
  `mcr` 位于 `pop`(函数尾) 与 `push`(下函数头) 之间（=字面量池）。
- **结论**：游戏是软浮点 ARM 代码，**不用 NEON/VFP/协处理器**；nop 占位无害，
  字面量数据经 `.Llit_` 池保留。无需实现 NEON。

## L15 · .data.rel.ro 内嵌指针是 32→64 位的根本硬墙（R2 边界）
- `.data.rel.ro` 有 **4800 个 R_ARM_RELATIVE 内嵌 32 位指针**（vtable/函数指针表/数据指针）。
- 64 位下这些指针须 8 字节 → **结构体布局全变，所有硬编码偏移（vtable dispatch `ldr rX,[rX,#40]`）全错**。
- 这是**机械二进制翻译无法跨越**的（非 GOT 那种均匀 4 字节槽可 scale×2）；
  需完整重析每个 C++ 类布局或全量反编译重写。**R2 之后为硬阻塞**。

## L16 · SPAK 加密 = XOR(key[j%32]) + zlib，key 在 .rodata 明文态（R5 关键）
- 原版资源 `.pak` 用 SPAK：`data[j] ^= key[j%32]`（`cc630`），**不是**单常数异或，**也不是**只异或——**异或后还要 zlib 解压**。
- 只 XOR 不出 ASCII → 先试 zlib 解压（`c3270` 把 f2 字节扩展为 f4 字节，就是 zlib inflate 的证据）。
- **key 在 `.rodata`（VA 0x12a658），不在 `.text` 字面量池**。`cc636: ldr r6,[pc,#36]; cc63c: add r6,pc` → `0xcc640+0x5e018=0x12a658`。
- 关键：**key 区字节在原始 .so 与 redroid 解密 dump 中一致**（该区未被二次加密），可从原始 .so 直接读，不必依赖 dump。

## L17 · SPAK 文件表在"合并流末尾"，`f1+f2 = 合并流大小`（R5）
- `.pak` 按组**合并**：`data=data0+data1`、`datah=datah0+datah1`、`datam=datam`、`datax=datax0..3`。
- 24 字节头：`SPAK`+version+**f1(文件表偏移)+f2(加密长)**+f3(校验)+f4(条目数×28)。
- **判据**：`f1+f2 == 合并流大小`（4 组全精确成立）→ 文件表在合并流末尾 f2 字节。
- 若 f1 超单文件大小，**不要**以为越界——它是合并流内偏移。

## L18 · 文件表 hash = MD5(资源路径)（R5 资源定位的通用规律）
- 28 字节条目 = `hash[16]+off[4]+size[4]+usize[4]`，`hash == MD5(资源路径)`（29/29 精确验证）。
- **用途**：给定 `data/junqi/move.wav`，`MD5(path)` 即条目 hash → 资源定位。native 用路径查资源即 MD5(路径)。
- 验证方法：`MD5('data/font/all.ttf')=b85fc09b...` == data#29 hash。C 侧 RFC1321 实现与 Python 一致。
- **注意**：部分图片路径 native 用 `data/{分辨率}/` 前缀运行时拼接（`data/junqi/piece.png` 直接 MD5 不匹配）。

## L19 · 28 字节条目 offset 链递推（R5 数据区解包）
- 数据区 = 合并流[24:f1]，条目 `off[i] = off[i-1] + size[i-1]`（首项 off=24）。
- 数据资源 = 数据区[off-24 : off-24+size]，XOR 解密 + zlib 解压；解压失败则原样（未压缩资源）。
- 校验：条目数 = f4/28，且 `offset[i]==24+sum(size[:i])` 全链一致即解包正确。

## L20 · redroid 验证边界 + 真机"能玩"需 arm64 有 GPU 设备（R5 复盘）
- redroid 容器**无 GPU**：EGL/GLES 初始化不完整，`OnCreate` 返回 null GLView → NPE（环境限制，非 app 问题）。
- redroid **64 位 native bridge 不完整**：arm64 `.so` dlopen 失败（也是环境限制）。
- redroid 只用于**验证 APK 可装 + 32 位库可 dlopen + 壳自解密提取明文**；"能玩"须真实 arm64 设备（有 GPU + arm64 原生）。
- 安装：redroid `/sdcard` 对 shell 用户 660，须 `su 0 pm install`。

## L21 · default.lay 是"每方 6行×5列=30格"且值编码为**降序**（R4/R5 关键）
- 真实 `data/lay/default.lay` = **30 字节**，不是 25。30 = 每方 6行×5列（含行营空位）。
- 值 0=空（行营/特殊格）；**值1=司令40最高，值9=工兵32最低**（**降序**，值越大子力越弱）。
- 完整映射：1=司令40,2=军长39,3=师长38,4=旅长37,5=团长36,6=营长35,7=连长34,8=排长33,9=工兵32,10=炸弹,11=地雷,12=军旗。
- **验证判据**：25 子/方分布（司令1军长1师长2旅长2团长2营长2连长3排长3工兵3炸弹2地雷3军旗1）与标准中国军棋**精确匹配**。
- 反推法：统计 default.lay 各值出现次数 → 与标准军棋子力分布对照 → 唯一确定升降序。
- 军旗在 layout 行5col1（底线），地雷在行4col4/行5col0/行5col2（护卫军旗），炸弹在行1col0/行2col1。

## L22 · 蓝红双方镜像放置时"军旗必须各在自己底线"（R4）
- 若蓝方占棋盘行0-5、红方占行6-11，蓝军旗应映射到**行0**（蓝自己底线），红军旗到**行11**。
- 常见错误：把双方 layout 都正序铺 → 双方军旗都跑中间（面对面），布局不对称。
- 正确：蓝方 `行=5-lrow`（翻转）；红方 `行=6+lrow, 列=4-lcol`（镜像翻转）。
- 验证：`st.board[jq_cell_index(0,1)].is_flag && st.board[jq_cell_index(11,3)].is_flag`。

## L23 · 走子规则自检清单（R3/R4 边界）
- 空位相邻移动=可走；己方子=不可；敌方行营=不可入；军旗/地雷=不可动。
- 铁路直线：须同行或同列 + **中间格全空/非敌方行营/非大本营**；工兵可拐弯（BFS）。
- 吃子：大压小；同级/炸弹=同归于尽；工兵挖雷；其他撞雷=被炸；吃旗=胜。
- 低撞高是**允许**的走法（会送死）——`jq_can_move` 返回 1，`jq_try_move` 才判负。
- 大本营内子不可动（军旗除外，军旗本就不动）；行营内子可出不可被吃（敌方不可入）。

## L24 · OnTouch 坐标→格映射 + 选中→走子状态机（R4 关键）
- Android `MotionEvent.getX()/getY()` 返回像素坐标（float），ACTION_DOWN=0 时处理。
- 映射：`col = (int)(x * COLS / viewW)`；`row = (int)(y * ROWS / viewH)`（全屏近似）。
  必须用 `int` 截断（格中心命中），不能四舍五入。
- 状态机：`g_selected`（-1=无）。点己方子→选中（高亮）；再点目标→`jq_try_move`；
  点未翻子→`jq_flip`；非法目标→取消选中。
- **验证判据**：60 格中心坐标全部映射回正确索引（主机测试）。
- 高亮：`junqi_render_set_selected(idx)` → 渲染时选中格颜色亮化（`(R+1)/2`）。
- 反汇编确认：OnTouch 内 `bl jq_cell_index/jq_try_move/jq_flip/junqi_render_set_selected`。

## L25 · 单机对弈：玩家（蓝）触屏 + AI（红）OnUpdate 自动回应（R3/R4）
- 状态机在 JNI `OnUpdate`：`turn==RED && !game_over` → `jq_ai_move()`。
- AI 贪心：优先吃子（遍历己方子×对方子，`jq_can_move` 可吃→`jq_try_move`）；
  其次前进一格（红向上 dr=-1，蓝向下 dr=+1）；暗棋先翻。
- **验证**：主机模拟对弈 29 步不卡死（蓝红互吃），stage 持续 PLAY。
- 注意：`jq_ai_move` 返回 false 表示无子可动（对局僵持，可判和）。

## L26 · PNG 解码：Z_FINISH 在 avail_out 恰好耗尽时返回 Z_BUF_ERROR（R4 关键）
- 极简 PNG 解码器（无 libpng，仅 zlib）：签名+IHDR+IDAT(zlib)+滤波器重建。
- **关键坑**：`inflate(Z_FINISH)` 当 `avail_out == raw_len` 且 IDAT 流末尾有填充/
  校验字节时返回 **Z_BUF_ERROR（-5）**，但 `total_out == raw_len` 已完全解压。
- **判据**：`total_out == raw_len` 即算成功（不能只看 rc==Z_STREAM_END）。
- 滤波器：0=None 1=Sub 2=Up 3=Average 4=Paeth（逐行首字节是 filter type）。
- 仅支持 8bit RGB(2)/RGBA(6)/灰(0)/灰+α(4)，非隔行；转 RGBA 4 字节/像素。

## L27 · Android assets 无文件路径 → 用 AAssetManager 读 .pak（R5 部署）
- APK 的 `assets/` 通过 `AAssetManager` 读（`Activity.getAssets()`），**无直接文件路径**。
- `spak_load_assetmanager(AAssetManager*)`：`AAssetManager_open(am, name, AASSET_MODE_BUFFER)`
  + `AAsset_read` + `AAsset_close`，复用目录版解析逻辑。
- 棋盘 PNG 路径需运行时拼接（`data/{分辨率}/` 前缀），`spak_get(path)` 按 MD5 找不到 →
  用 `spak_get_by_hash(hash)` 按 16 字节 hash 确定性加载（datah#188=0x21d95e03...）。
- 棋盘纹理：`OnGraphCreate`（GL 上下文就绪）时解码注入 `junqi_render_set_board_texture`。

## L28 · OpenSL ES 音效播放（minSdk 19 兼容，AAudio 需 API 26+）
- AAudio 需 API 26+，minSdk 19 须用 **OpenSL ES**（API 9+，`libOpenSLES`，`-lOpenSLES`）。
- 流程：`slCreateEngine`→`Realize`→`GetInterface(SL_IID_ENGINE)`→`CreateOutputMix`→
  `CreateAudioPlayer(SL_IID_BUFFERQUEUE)`→`Enqueue`→`SetPlayState(PLAYING)`。
- WAV 解析：RIFF/fmt/data；8bit 无符号→16bit（`(v-128)<<8`），双声道混单声道（平均）。
- 采样率：OpenSL 用 **milliHz**（`rate*1000`），PCM 格式 `SL_DATAFORMAT_PCM` 16bit 单声道。
- 缓冲须保持存活（static `g_pcm`），Enqueue 后由播放器异步消费。

## L29 · 悔棋 = 移动前快照棋盘（R3 玩法完整性）
- `jq_try_move`/`jq_flip` 成功前 `snapshot_undo`（存 board/turn/stage/winner/flags/pieces）。
- `jq_undo` 恢复快照，`has_undo` 单步（二次悔棋返回 false）。
- **验证判据**：悔棋后源格 rank/目标格 rank/剩余子数/当前方 全部恢复原值。
- `snapshot_undo` 必须在 `jq_try_move` 修改 board 前调用（指针 src/dst 指向 board，改后失效）。

## L30 · 棋子贴图渲染（真实 PNG 兵种贴图，R4 渲染闭环）
- 棋子贴图是 **72×72 PNG**（41 个，SPAK 解包），蓝方 datah#124-129/红方 #140-145 为兵种贴图，
  #134-135 绿色半透明 = 棋子背面。
- 渲染管线：shader 需 `a_uv`(attribute) + `u_tex`(sampler) + `u_has_tex`(int) 三个字段，
  片元 `if(u_has_tex==1) color=texture2D(u_tex,v_uv)`。
- 贴图注册：`junqi_render_set_piece_texture(side,rank,rgba,w,h)` 存 `g_piece_tex[side][slot]`，
  slot 按 rank 去重（`piece_slot`）。已 revealed 且有贴图 → 单独纹理四边形通道（与色块分离）。
- **hash 必须是完整 16 字节**（manifest.tsv 第 3 列 32 hex），不是 8 字节前缀——`spak_get_by_hash` 按 16 字节匹配。
- rank 降序近似映射（40→第一张...35→第六张）；原版精确 rank→贴图映射未逆出（诚实边界）。

## L31 · 胜负通知 = native→Java Toast（可玩闭环）
- 原版 native→Java 事件经 `android/widget/Toast`（`Toast.makeText(ctx,msg,LENGTH_SHORT).show()`），
  引用在解密 rodata（`(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;`）。
- JNI 保存全局 Activity 引用：OnCreate `NewGlobalRef(activity)`，OnDestroy `DeleteGlobalRef`。
- `notify_game_over(env)`：stage==OVER 且未通知过 → Toast；`g_over_notified` 防重复，
  restart 时重置。胜负判定仍用 `g_state.winner`。
- Toast 内部经主线程 Handler 派发，GL 线程调用安全（原版同机制）。

## L32 · HUD 像素字模（当前方 + 剩余子数，无 TTF）
- 原版用 TTF（`data_font_all.ttf`），但无 FreeType，自写 TTF 渲染是月级工程。
- 替代：**5×7 像素字模**（`FONT_DIGIT[11][7]`，0-9+空格），`draw_digit/draw_int` 用四边形拼点阵，
  不依赖任何字体库。
- HUD：顶部半透明条（`bar_y0=0.93`）+ 当前方彩块 + 双方剩余子数（`st->pieces_left`）。
- **注意**：`draw_quad` 定义在 draw_hud 之后 → 需前向声明（`static void draw_quad(...)`），
  否则 C99 `-Wimplicit-function-declaration` 报错。

## L33 · 困毙判定（标准军棋规则完整性）
- 胜负两途径：①吃军旗 → 胜；②**对方无子可动 → 胜（困毙）**。
- `side_has_movable(st, side)`：遍历 side 子，非空/非军旗/非地雷且已revealed 且有合法目标 → 可动；
  **有未revealed 暗棋也算可动**（可翻），防误判。
- `check_stalemate(st)`：move/flip 后 turn 切换时调用，若 `!side_has_movable(st, turn)` → winner=1-turn。
- **枚举值注意**：`JQ_STAGE_PLAY=1`、`JQ_STAGE_OVER=2`（不是 0/1）——测试断言易错。
- 验证：翻棋后 stage=PLAY(1)/winner=0xFF；红方全灭可动子后 stage=OVER(2)/winner=蓝(0)。

## L34 · 大本营规则（军旗固定，其他子禁入）
- 大本营：蓝行0 col1/col3、红行11 col1/col3（`jq_is_base`）。军旗（rank=2,is_flag=1）固定在其中一个。
- `jq_can_move` 补：目标是大本营 且 **非军旗** → 禁止（`jq_is_base(tr,tc) && !dst->is_flag`）。
- `walkable` 已禁止大本营通行（铁路滑行不穿大本营）。
- 验证：蓝大本营(0,1)=军旗 rank2；蓝师长进蓝/红大本营均禁止。

## L35 · 棋盘 12×5 验证 + 居中渲染（格方形）+ touch 同步
- **棋盘模型确认 12行×5列（60格）**：default.lay 30字节/方（25子+5空），蓝行0-5/红行6-11，
  军旗 layout 行5→蓝行0/红行11（底线）。格子方形约 68×62px，PNG 737×751 中央 340px 宽。
- 棋盘 PNG（datah#188 737×751）是**十字棋盘**：上下阵营（12行5列）+ 左右铁路支线，
  棋盘主体在 PNG 中央（x 0.269-0.730），全高。
- 渲染修复：棋盘**居中 + 保持格方形**（5:12 比例，`board_region`），非全屏拉伸（原先格子 2.4:1 扁长）。
  棋盘主体纹理 uv=(0.269-0.730, 0-1)。
- **touch 同步**：`junqi_render_touch_cell(px,py,w,h)` 与渲染共用 `board_region`，棋盘外返回 -1。
  修复前 touch 全屏映射（点棋盘外也算格）→ 现与渲染一致。
- 验证：1080×1920 中心→行5列2、左上→行1列0、屏幕角→-1。

## L36 · AI 返回动作类型（音效区分）
- `jq_ai_move` 返回 int：0=无,1=移动,2=翻棋,3=吃子（原先 bool）。
- 头文件声明同步（`int jq_ai_move`），OnUpdate 按返回值播 flip/eat/move/win/lose。
- **注意**：改签名须同步 junqi_core.h 声明 + game_logic.c 实现 + junqi_jni.c 调用三处，
  重复旧声明（bool）需删除（否则重定义错误）。
- AI vs AI 完整对局 43 步终局（winner=蓝，困毙/吃旗），不无限循环。

## L37 · 暗棋开局 + 困毙暗棋修正（关键玩法 bug）
- **bug**：`jq_place` 设 `revealed=1`，default.lay 加载后所有子已翻开，暗棋模式失效。
  修复：`jq_load_layout` 末尾统一 `revealed=0`（50 子全暗棋开局）。
- 暗棋规则：未翻不可走；翻棋消耗回合（turn 切换）；AI 先翻自己暗棋再走。
- **困毙修正**：`side_has_movable` 的暗棋可翻计数须排除军旗/地雷（暗棋军旗/地雷翻了也不能走），
  否则红只剩军旗+地雷时误判"未困毙"。
- 综合回归 **15/15 通过**：暗棋开局/未翻禁走/翻棋切turn/困毙/AI 64步终局。

## L38 · 棋子渲染修正：UI 图标误判 → rank 数字（诚实）
- **误判**：72×72 的 #124-145 是 **UI 图标**（信息/箭头/扳手/红叉/勾/星），非棋子兵种贴图。
  原版棋子是 `data/sg/piece.png` sprite sheet（`piece.png,%d,%d,%d,%d,clip`），但 SPAK hash≠MD5(路径)，
  883 资源无明确棋子 sprite，无法可靠定位。
- **诚实回退**：移除 UI 图标棋子贴图，已 revealed 子画**白色 rank 数字**（像素字模 5×7，draw_int），
  未 revealed 画**绿色背面**（0.35/0.55/0.35）。
- `junqi_render_set_piece_texture` 保留为空操作（兼容头文件）；删除 PIECE_HASH/load_piece_textures/piece_slot。
- 验证：回归 15/15（rank 数字渲染不影响逻辑）。

## L39 · 军棋规则完整验证（吃子/炸弹/工兵挖雷）
- 大压小：38 占 36 格（源清空）；同级同归于尽。
- 炸弹：`is_bomb` 与任意子同归于尽（双方清空）。
- 工兵挖雷：`JQ_RANK_SOLDIER` 吃 `JQ_RANK_MINE`（工兵占雷格）。
- 非工兵撞雷：雷保留，攻击子消失。
- 验证：6/7（悔棋断言笔误 rank 36→33，逻辑正确）。

## L40 · 多步悔棋（循环栈，深度 8）
- `jq_state` 悔棋历史改**循环栈**：`undo_board[8][JQ_CELLS]` + turn/stage/winner/flags/pieces 数组。
- `snapshot_undo` 压栈（`undo_head=(head+1)%8`），`jq_undo` 弹栈（恢复最近快照）。
- 每动作（翻棋/走子）各压一层；悔棋逐动作弹出，可连续悔到初始（耗尽后返回 false）。
- 验证：10 次动作悔 8 层（截断），耗尽恢复 50 子/turn=蓝；回归 15/15。

## L41 · 终局输入禁用 + 悔棋选中重置（可玩性）
- `handle_tap` 开头：`stage==OVER` → 忽略输入（清选中，不翻/不走），防止终局后误操作。
- OnEvent "undo"：悔棋成功后重置 `g_selected=-1`（避免选中已失效格）。
- 验证：编译 14 JNI，打包 Verifies v1/v2/v3，MD5 一致。

## L42 · 空格不画色块（棋盘纹理露出）
- 色块通道只画**有子格**（背面绿/蓝/红），空格跳过——棋盘纹理的格线/行营/大本营标记露出。
- 此前空格画绿/蓝/黄色块完全覆盖棋盘纹理（纹理白加载）。
- 修复后：棋盘 PNG（737×751，十字棋盘）作背景 + 子格浮于其上 + rank 数字。

## L43 · AI 策略改进（吃子→前进→翻棋优先序）
- 原 AI 单循环混合：遇到暗棋即翻（中断吃子），翻棋无选择。
- 改进三阶段：①遍历已revealed 子找可吃（大压小/吃旗/炸弹/挖雷）；②向对方方向前进；
  ③翻棋选**最靠近敌方前沿**的暗棋（红方 row 最小/蓝方 row 最大）。
- 效果：AI 完整对局 47 步终局（蓝18/红6）；玩家+AI 93 步终局（翻36 走34 吃29）。
- 回归 15/15 通过（吃子优先不因暗棋中断）。

## L44 · 棋盘主体 uv 精确（5 列中央）
- 棋盘 12×5（格 39×62 竖长，default.lay 30/方=6×5 自洽）；棋盘主体在 PNG **x 0.366-0.631**（5 列中央），y 全高。
- 左右铁路支线延伸到 PNG 边缘（不属于 12×5 格），uv 应只采棋盘主体（原 0.269-0.730 偏宽）。
- 修正 uv = (0.366,0)-(0.631,1)，棋盘纹理与格线对齐（视觉）。

## L45 · 军旗明置（暗棋规则修正，防速胜漏洞）
- **bug**：军旗暗置且"翻对方军旗即胜"→ 玩家翻到对方军旗格即速胜（漏洞）。
- 修正：**军旗明置**（load_layout 时 `is_flag` 子 revealed=1），`jq_can_flip` 排除军旗（不可翻）。
  吃旗仍走 `jq_try_move`（dst->is_flag → 胜）。
- 暗棋开局：48 暗 + 2 军旗明置（非 50 全暗）。
- 验证：军旗 2 个明置、不可翻、其他暗棋可翻；回归 15/15（更新断言）。

## L46 · 特殊子符号渲染 + 可走格提示（视觉）
- **特殊子符号**：军旗/炸弹/地雷用 5×7 像素符号（FONT_FLAG/FONT_BOMB/FONT_MINE），
  普通子仍 rank 数字。符号按方着色（蓝=浅蓝，红=浅红）对色块清晰。
- **可走格提示**：选中己方可动子（已revealed且非旗/雷）→ 高亮其可走/可吃目标格（黄色边框 4 条细边），
  用 `jq_can_move` 判定。未选/选中不可动子不显示。
- 验证：编译 14 JNI，回归 15/15，AI 47 步终局，Verifies v1/v2/v3，MD5 一致。

## L47 · 对齐原版 rank 字节语义（逆向 0x6a3a4 / 0x39e14）
- **逆向工具链**：capstone Thumb 全段扫描（`find_ref.py` 定位 PIC 引用、`disasm_thumb.py` 反汇编）。
  JNI 符号以 `.symtab` 为准（`llvm-nm -D` 给错桩地址）；`.text` Off==VA=0x233e0。
- **原版 rank 语义**（0x6a3a4 映射 + 0x39e14 走子判定）：`0=空, 1-9=普通 rank（1=司令最强...9=工兵最弱）,
  10=炸弹, 11=地雷, 12=军旗`；红方已翻开=字节+0xc。
- **arm64 对齐**：`JQ_RANK_*` 改成原版字节值（MINE=11/BOMB=10/FLAG=12/SOLDIER=9/MIN=1/MAX=9），
  `layout_rank` 直接返回字节值（不再 40..32 降序），吃子比较改 `src<dst`（rank 小=强）。
- 布局加载：30 字节顺序（6×5），蓝方 `5-lrow` / 红方 `6+lrow,4-lcol`（军旗 lrow5→行0/11 底线）。
- 验证：对齐验证 5/5（军旗明置rank12/司令1吃工兵9/工兵9不敌司令1）；回归 15/15；
  AI 47 步终局；玩家+AI 93 步终局；APK Verifies v1/v2/v3。

## L48 · 对齐确证：军旗位置/吃旗胜/AI难度菜单/翻棋规则
- **军旗位置对齐**：arm64 军旗落位蓝格1(row0col1)/红格58(row11col3)=大本营，
  与原版 `0x2a228`（军旗合法格 56/58 红方大本营）一致。
- **吃旗胜对齐**：蓝司令可吃红军旗格58（`jq_can_move`=1）→ stage=OVER/winner=蓝；
  己方军旗不可吃（`jq_can_move`=0）。原版 `0x39e14` 军旗特判（to==1/3, from==5）一致。
- **翻棋规则**：原版 `0x39e14` 同格翻棋——军旗特判（from==to==1/3 报错→**军旗不可翻**）✓；
  地雷可翻（红地雷行10-11 to_row>3）；炸弹 to_row==0 特判。arm64 军旗明置不可翻、地雷/炸弹可翻——对齐。
- **AI 难度菜单**：原版 `0x485de/48632` 引用 LowLay/MidLay（`bl 0x82d3c` 字符串比较 + `bl 0xb4edc` 事件派发）。
  难度布局是**菜单选择**（非自动）；arm64 无 Java UI 入口，诚实边界（需 UI 才可接入难度）。
- 验证：军旗位置测试（蓝格1/红格58 对齐）；吃旗胜测试（可吃红军旗→胜）；回归 15/15。

## L49 · 铁路规则修复（jq_is_rail 恒 true → 真实铁路拓扑）
- **问题**：`jq_is_rail` 恒 `true`（简化），导致非铁路行（row0-4/7-11）也能横向滑行（斜线走子 bug）。
- **修复**（对齐原版棋盘铁路）：
  - `jq_is_rail`：排除行营/大本营（行营/大本营是特殊格，非铁路）。
  - `walkable`：行营一律不通行（原只拦敌方行营，现己方行营也拦——行营是离铁路安全格，不可穿行）。
  - `rail_path_engineer` BFS：**横向移动（nc!=c）仅限铁路中线 row5/6**；纵向任意列。
  - `rail_path_straight` 已有 `fr==tr && fr!=5&&fr!=6` 横向限制，无需改。
- **验证**：铁路规则 10/10（横向 row5 滑行合法、row0 横向非法、斜线非法、纵向全列合法、
  工兵拐弯合法、非工兵拐弯非法、相邻一步合法、军旗不可动、进敌方行营非法）。
  回归 15/15；AI 95 步终局；玩家+AI 109 步终局；APK Verifies v1/v2/v3（arm64 44104B STORE）。
- **教训**：军棋铁路是"横向仅 row5/6 + 纵向任意列"，行营/大本营在铁路外。
  "恒 true"这类过度简化会悄悄引入斜线走子 bug，须按真实棋盘拓扑实现。

## L50 · 翻棋 turn 守卫 + 原版翻棋规则对齐
- **原版 0x39e14 翻棋判定**（r1=打包坐标, r2=目标 rank）：
  - `r2==0xc 军旗`：`if to∈{1,3} and from==5 合法 else 报错`——**军旗不可翻**（翻棋 from==to，条件永假）。
  - `r2==0xb 地雷`：`if from>3 合法`——地雷可翻（后场 from>3）。
  - `r2==0xa 炸弹`：`if from!=0 合法`——炸弹可翻。
  - 普通 1-9：`return 1`（合法）。
- **arm64 对齐**：`jq_can_flip` 排除军旗（明置不可翻），地雷/炸弹/普通可翻。
- **turn 守卫（修复）**：`jq_flip` 增加 `board[idx].side != st->turn → false`（原版 turn 语义由调用者强制，
  arm64 曾允许翻敌方子——JNI 未查 turn）。AI 只翻己方（side!=me 过滤），JNI 触屏翻子现在受守卫保护。
- **验证**：翻棋规则 13/13（军旗不可翻/地雷炸弹可翻/普通可翻/已翻开不可翻/空位不可翻/
  翻切 turn/翻非当前方失败）；回归 15/15；铁路 10/10；对齐 5/5。APK Verifies v1/v2/v3。

## L51 · 压力测试 5000 局 + 军旗/坐标约定确认
- **压力测试**：5000 局随机对弈，1,610,201 步，**全部终局（5000/5000），0 非法状态**。
  校验每步棋盘一致性（rank 与 is_* 标志、pieces_left 计数、空位无子、rank∈1..12）。
- **0x6ef54 语义**：坐标比较，`[0]=col,[1]=row`（col 在前），返回 0=相同。
- **军旗特判（0x39e14）**：目标 `col∈{1,3} && row==5`（大本营列+半棋盘第5行）。
- **诚实边界**：吃子 rank 比较在更深调用链（非直接走子路径），arm64 `1=司令最强` 为合理推断。

## L52 · rank 方向确证（default.lay 分布 = 标准军棋子力）
- **决定性证据**：原版 `default.lay` 30 字节值分布与标准中国军棋单方 25 子**完全匹配**：
  `1司令×1, 2军长×1, 3师长×2, 4旅长×2, 5团长×2, 6营长×2, 7连长×3, 8排长×3, 9工兵×3,`
  `10炸弹×2, 11地雷×3, 12军旗×1`（25 子，值 0×5=行营）。
- **结论**：**值越小越强（1=司令最强，9=工兵最弱）**——arm64 `JQ_RANK_MIN=1`/`JQ_RANK_MAX=9`/`src<dst`
  为**原版直接证据**（非推断）。军旗@layout idx26(row5col1)=蓝方底线。
- **诚实边界更新**：REVERSE_NOTES §12 从"合理推断"改为"已确证（default.lay 分布佐证）"。
  吃子 rank 比较函数在事件分发深层（0x9b658→0x9c318），未逐指令逆向，但 default.lay 分布是决定性佐证。
- 验证：分布 12/12 值匹配（25 子）、军旗位置、回归 15/15、对齐 5/5、铁路 10/10、翻棋 13/13。

## L53 · 困毙规则验证（无子可动 → 对方胜）
- **困毙测试修复**：`jq_load_layout` 后棋子暗置（军旗除外），`side_has_movable` 只认
  已 reveal 的非军旗/地雷子可动 + 非军旗/地雷暗子可翻；setup 需先翻一个蓝方子再验证。
- **验证 4/4**：红方只留军旗(明置)+地雷(暗置) → 红方无子可动；蓝方翻一暗子 →
  turn 切红方 → `check_stalemate` 触发 → 蓝方胜（winner=BLUE, stage=OVER）。
- **原版等价**：吃旗胜 0x30260（调用者 0x51cac 已确证）、和 0x30280、困毙 0x302b4
  （status=3，事件分发）。arm64 吃旗胜 + 困毙已对齐；和棋未实现（下一步）。

## L54 · turn 状态机确证（无连吃）+ 和棋/困毙事件分发诚实边界
- **0x56e6c / 0x57b58 是 UI/渲染刷新**（调 0x39850/0x39308 取状态、0x854a8/0x57ab0 打包），**非 turn 逻辑**。
- **0x39f20（走子）尾部 0x3a1ce**：`movs r3,#1; b 0x39f90`（返回路径）——走子完成后**直接返回**，
  **无连吃循环**（若允许连吃会分支回"当前方继续"）。支持"无连吃"（每行动回合结束 → 切 turn）。
- **0x39ea0（翻棋）尾部**：发事件 → 调刷新 → 返回，同样每翻一子 = 一回合。
- **结论**：原版 turn 切换在事件分发层（0x9c318 链），每次成功行动（走子/翻棋）→ 回合结束。
  arm64 `jq_try_move`/`jq_flip` 仅在**成功**后切 turn（失败不切）——标准规则等价。
- **诚实边界**：原版和棋（0x30280）/困毙（0x302b4）事件处理器按**字符串名分发**（非函数指针表），
  确切条件在深层事件链，未逐指令确证。arm64 已实现困毙（L53 验证 4/4）+ 吃旗胜（0x51cac→0x30260 确证）；
  **和棋规则未实现**（变体歧义大，标准"双方均无法取胜"待定）。

## L55 · 和棋规则（双方无子可动 → 和）+ 重打包
- **和棋实现**：`check_stalemate` 改——turn 方无子可动时，检查对方；双方都无子可动 → `JQ_WINNER_DRAW`(2)，
  否则对方胜。`junqi_core.h` 加 `JQ_WINNER_DRAW 2`；JNI 层 `notify_game_over`/音效处理和棋消息。
- **和棋测试 4/4**：手动构造双方均无子可动（各只剩军旗+暗地雷），蓝方翻暗地雷 → turn 切红方 →
  红方无子可动 → 蓝方也无子可动 → 和棋。
- **原版映射**：0x30280（和棋触发，result=2）走事件分发字符串名分发；0x30260（吃旗胜 result=1/2）
  /0x302b4（困毙 status=3）同样。arm64 已实现吃旗胜 + 困毙 + 和棋三路终局。
- **重打包**：新编译 arm64 `.so`（44KB，14 JNI）→ dec 替换 → apktool b → store_compressed(STORE) → SignV2。
  产物 `phase_hand.apk` 14.47MB：Verifies v1/v2/v3、manifest 0x00080003、4 个 `.so` 全 STORE。
- 回归全绿：困毙 4/4、和棋 4/4、回归 15/15、对齐 5/5、铁路 10/10、翻棋 13/13、布局 PASS、
  军旗吃旗胜、地雷/炸弹 PASS、压力 5000 局（161 万步、0 非法、全终局）。

## L56 · default.lay 字节级一致确证（原版 SPAK 实际布局 = arm64 布局）
- **直接确证**：SPAK 解包 `dist/spak_res/data_lay_default.lay`（30 字节）与 arm64 使用的
  `{3,7,6,5,8,10,0,9,0,1,2,10,0,9,5,4,0,6,0,7,8,3,4,9,11,11,12,11,8,7}` **完全一致**（字节级）。
- **意义**：arm64 布局来自**原版实际资源文件**（非推断/自编）——消除"简化版布局"诚实边界。
- **0x485d0 确证为菜单 UI 构建**（逐项 `bl 0x82d3c` 字符串比较 + `bl 0xb4edc` 添加菜单项），
  非 AI 算法。原版 AI 走子在深层（诚实边界）。
- **0x30260 吃旗胜 result 编码**：两档（1/2）= 两胜方，arm64 winner(0蓝/1红) 语义等价
  （蓝吃红旗 → winner=0 已在测试确证）。
- 布局来源：原版 `assets/data*.pak`（SPAK）内 `data_lay_default.lay`，arm64 spak_load 解包一致。

## L57 · 原版规则文本权威确证（rulelay/ruleflip GBK）+ 3 项规则修正
- **决定性确证**：SPAK 解包 `data_junqi_rulelay.txt`/`ruleflip.txt`（GBK）为原版**权威规则文本**：
  - 吃子：司令>军长>师长>旅长>团长>营长>连长>排长>工兵；炸弹与任何棋相遇**双方都消失（包括军旗）**；
    地雷<工兵、>所有其他棋；**任何棋子都能扛军旗**。
  - 布局：炸弹不放第一行；地雷只放最后两行；**军旗必须放**大本营；**进大本营不可再动**；行营=安全岛（敌方不能吃行营棋子）。
  - 胜负：棋子被吃光、军旗被扛、**无棋可走→判负**；**连续70步无吃子→和**；**双方总步数达1000→和**。
- **3 项 arm64 修正**（对齐原版权威文本）：
  1. **和棋**：移除"双方无子可动→和"（原版无此规则），改为 `moves_since_eat>=70` 或 `moves>=1000` → 和。
  2. **炸弹撞军旗**：`src->is_bomb` 分支提到 `dst->is_flag` 之前——炸弹撞军旗**双方消失**（非吃旗胜）。
  3. **困毙**：`check_stalemate` 改回**单方无子可动→该方负**（原版"无棋可走判负"，无双方和）。
- **新字段**：`moves_since_eat`（连续无吃子步数）+ undo 快照保存/恢复；`jq_try_move` 吃子重置=0、空目标走子++；
  `jq_flip` 也参与计数。
- 验证：和棋(70步无吃子)4/4、炸弹撞军旗6/6、困毙4/4、回归15/15、对齐5/5、铁路10/10、翻棋13/13、
  布局PASS、军旗吃旗胜、地雷/炸弹PASS、压力5000局（全终局、0非法）。
- 重打包：新 arm64 .so（14 JNI）→ dec → apktool b → STORE → SignV2；phase_hand.apk 14.47MB
  （Verifies v1/v2/v3、manifest 0x80003、arm64 STORE）。
- 产物：dist/phase_hand.apk、dist/libjunqi_arm64_hand.so（44KB）、LESSONS 57、REVERSE_NOTES 159 行。

## L58 · 大本营地雷可吃修正（原版"大本营的地雷、军旗可以吃"）
- **bug 修正**：`jq_can_move` 95 行原阻止非军旗进大本营——但原版 default.lay 红方大本营 **格56=地雷**
  （default.lay[28]=11），原版 ruleflip 明确"大本营的地雷、军旗可以吃"。工兵挖不到大本营地雷。
- **修正**：`if (jq_is_base(tr,tc) && !dst->is_flag && !dst->is_mine) return false`——允许进大本营吃
  军旗或地雷，其他子不可进（大本营仅放军旗/地雷）。
- **验证 7/7**：工兵(格51)挖大本营地雷(格56)→工兵占格56、源空、红方-1；大本营内子不可再动（原版 rulelay"进入大本营不可再动"）。
- 回归全绿：困毙 4/4、和棋 4/4、炸弹撞军旗 6/6、回归 15/15、对齐 5/5、铁路 10/10、翻棋 13/13、
  布局 PASS、军旗吃旗胜、地雷/炸弹 PASS、压力 5000 局（全终局 0 非法）。
- 重打包：arm64 .so（14 JNI）→ dec → apktool b → STORE → SignV2；phase_hand.apk 14.47MB
  （Verifies v1/v2、arm64 STORE）。

## L59 · 0x39e14 布局校验确证 + JQ_PIECES 38→25 修正
- **0x39e14 完整逆向确证**：是**布局校验**（非走子方向限制）——
  - 军旗：`col∈{1,3} && row==5`（大本营列+底线行）→ 军旗必须在大本营；
  - 地雷：`row>3`（半棋盘第4/5行=最后两行）；炸弹：`row!=0`（不放第一行）。
  - **军旗吃法无方向限制**（0x39e14 只验证军旗位置在大本营，arm64 相邻任意方向吃旗**对齐**）。
- **JQ_PIECES 38→25 修正**：原版 default.lay（30字节-5行营=25 子，L56 确证）；`ARMY_RANKS` 只初始化 25 个，
  旧 `JQ_PIECES=38` 导致 `jq_dark_setup` 放 rank=0 子（数组 [25-37]=0）——**暗棋模式 bug**。
  改 25 后暗棋 25蓝+25红+10空+2军旗、pieces[25,25] 对称 PASS。
- 验证：暗棋 PASS、困毙 4/4、和棋 4/4、炸弹撞军旗 6/6、大本营地雷 7/7、回归 15/15、对齐 5/5、
  铁路 10/10、翻棋 13/13、布局 PASS、军旗吃旗胜、地雷/炸弹 PASS、压力 5000 局（全终局 0 非法）。
- 重打包：arm64 .so（14 JNI）→ dec → apktool b → STORE → SignV2；phase_hand.apk 14.47MB（Verifies v1/v2）。

## L60 · 布局加载/映射/可动子统计确证（0x2a340/0x6a3a4/0x6a3b0）
- **0x2a340 布局加载确证**：30 字节循环（(804-444)/12=30），每字节调 `0x6a3a4(字节, state[64])`，
  存 `state[444+i*12]`（12 字节/子）。arm64 `jq_load_layout`（30 字节 default.lay）**对齐**。
- **0x6a3a4 映射确证**：`r0==0 → 0`（空格）；`state[64]==0 → r0+0xc`（红方标记）；否则原 rank（蓝方）。
  = 蓝原 rank / 红 rank+0xc。arm64 用 side 字段区分（语义等价）。
- **0x6a3b0 可动子统计确证**：遍历 60 格，跳过空格、rank>0xc→-0xc 归一、跳过 rank 0xb/0xa（地雷/炸弹）
  ——**原版困毙/吃光可动子判定**，对齐 arm64 `side_has_movable`（排除空格/地雷/炸弹/军旗）。
- 布局模式确认：OnCreate 优先 `spak_get("data/lay/default.lay")`→布局（rulelay）；缺失才回退暗棋（ruleflip 简化）。

## L61 · 刷新可动子统计确证（0x6b87c→0x6a3b0，原版困毙链路）
- **0x6b87c 确证**：走子后**刷新可动子统计**——遍历 60 格、统计 rank（地雷计数 0x22 偏移）、
  调 0x69c2c（每 6 格评估）、0x6aca0、0x6a3b0（可动子统计）、0x6b3d8。
  = 原版"刷新棋盘统计 + 困毙判定"（走子后调用）。
- **对齐**：arm64 `check_stalemate`（走子/翻棋后调 `side_has_movable`）——语义等价。
- **AI 走子**：不在 0x6b87c（是统计刷新）。AI 走子算法在事件分发深层（0x9c318 链），未定位（诚实边界）。
- 规则层原版函数链路已确证：0x39e14 布局校验、0x2a340 布局加载、0x6a3a4 映射、
  0x6a3b0/0x6b87c 可动子统计——**arm64 全部对齐**。

## L62 · AI 难度菜单确证（0x485d0）+ 诚实边界
- **0x485d0 确证**：菜单构建，**9 项**（bl 0xb4edc ×9 + bl 0x82d3c ×9 标签比较）。标签为 `add r1,pc` 字符串。
- **rodata 明文**：`sgleveldlg.xml`/`leveldlg.xml`/`levelexdlg.xml`（难度对话框 UI 布局）+ `difficulty`/`level` 标签
  ——原版有 AI 难度选择概念。
- **sgleveldlg.xml 不在 SPAK**（解包 915 资源无）——难度档数未确证（诚实边界）。
- **诚实边界**：arm64 AI 单档贪心（L43，95步终局），原版多档难度（档数未确证）。AI 走子算法在事件分发深层未定位。
- L59-L62 为规则层原版函数链路确证（0x39e14/0x2a340/0x6a3a4/0x6a3b0/0x6b87c/0x485d0），全部对齐或标注边界。

## L63 · ruleflip 翻棋模式吃子规则对齐（军旗/地雷仅工兵吃、扛旗需挖3雷）
- **ruleflip 权威文本**（L57 解码）："地雷和军旗只有工兵能吃。工兵挖完地雷后才能扛军旗"。
- **arm64 暗棋模式（DARK）对齐**：
  - `jq_can_move` 加 DARK 分支：`dst->is_mine && src非工兵 → 禁`；`dst->is_flag && (src非工兵 || mines_cleared<3) → 禁`。
  - `jq_try_move` 工兵挖雷（DARK）`mines_cleared[side]++`。
  - 新字段 `mines_cleared[2]` + undo 快照/恢复。
- **PLAY 布局模式不受影响**（DARK 分支仅 stage==DARK；布局模式任何棋扛旗 = rulelay 已对齐）。
- **验证 6/6**：工兵未挖3雷不能扛旗、工兵可挖雷、非工兵不能扛旗/挖雷、挖3雷后可扛旗、挖雷计数+1。
- 回归全绿：暗棋 25/25、困毙 4/4、和棋 4/4、炸弹撞军旗 6/6、大本营地雷 7/7、回归 15/15、对齐 5/5、
  铁路 10/10、翻棋 13/13、布局 PASS、军旗吃旗胜、地雷/炸弹 PASS、压力 5000 局（全终局 0 非法）。
- 重打包：arm64 .so（14 JNI）→ dec → apktool b → STORE → SignV2；phase_hand.apk 14.47MB（Verifies v1/v2/v3）。

## L64 · ruleflip 先手翻子定色（side=UNKNOWN → 翻子定色）+ 暗棋困毙修复
- **先手翻子定色**：暗棋模式（ruleflip）布子 side=UNKNOWN（50 子无归属），`jq_flip` 翻子时
  `c->side = turn` 定色 + `pieces_left[turn]++`。新字段 `JQ_SIDE_UNKNOWN=0xFF`。
- **暗棋困毙修复（关键）**：第一次翻子后 turn 切对方，`check_stalemate` 查 `side_has_movable(对方)`
  ——但暗棋所有子 side=UNKNOWN，对方无已翻子 → 误判对方无子可动 → 提前负。
  **修复**：`side_has_movable` 暗棋模式先扫 UNKNOWN 暗子（非军旗）→ 有则当前方必可翻 → 未困毙。
- **验证 9/9**：50子side未定、初始pieces=0、先手蓝、先手翻子、先手翻子定色蓝、蓝pieces=1、
  turn切红、后手翻子、红pieces=1。
- 回归全绿：ruleflip 6/6、暗棋 25/25、困毙 4/4、和棋 4/4、炸弹撞军旗 6/6、大本营地雷 7/7、
  回归 15/15、对齐 5/5、铁路 10/10、翻棋 13/13、布局 PASS、军旗吃旗胜、地雷/炸弹 PASS、
  压力 5000 局（全终局 0 非法）、AI vs AI 200 局全终局。
- 重打包：arm64 .so（14 JNI）→ dec → apktool b → STORE → SignV2；phase_hand.apk 14.47MB（Verifies v1/v2/v3）。

## L65 · 0x39f20 移动执行器完整确证 + AI/蓝牙深度诚实边界
- **0x39f20 移动执行器完整确证**（非 AI）：
  - 读 `state[0xa2]`（选中坐标）+ `state[0xa3]`（目标坐标）+ `state[0x80+i*6+4]`（格子 rank）；
  - `0x6ef54` 坐标比较（[0]=col,[1]=row）；`0x39e14` 布局校验（军旗大本营/地雷/炸弹）；
  - 走子 → `0x9b658`（事件发送）+ `0x56e6c`/`0x57b58`（UI/渲染刷新）。**纯用户输入走子，无 AI 分支**。
- **AI 走子深度确证**：OnEvent(0x233e0)→`0xbd914`（C++ 全局事件分发 vtable `[vtable+0x2a4]` 调用）。
  **AI 走子是 C++ 对象方法（vtable 分发 + 栈帧 0x2a4）**，非 JNI 桩直达——静态逆向月级。
- **蓝牙深度确证**：OnBluetooth(0x23424)→`0xbe9cc`（C++ 对象方法，vtable `[+0x7c]`/`[+0x84]` 分发 + 0x77820/0xbf19c/0x7bf80）。
  rodata 有完整蓝牙 API（BluetoothAdapter/Device/ServerSocket/RfcommSocket）——**标准 RFCOMM SPP**，
  但消息格式在 C++ 对象链深处，静态逆向月级。
- **诚实边界**：arm64 AI 单档贪心（L43，95步终局）与蓝牙占位（bt_spak.c）为简化；
  AI 多档难度（sgleveldlg.xml 不在 SPAK，L62）与协议消息格式未确证。
- 验证：AI vs AI 200 局全终局；规则层（L53-L64）全部对齐。

## L66 · AI 困毙兜底修复（jq_ai_move 无可走子 → check_stalemate）
- **jq_ai_move 三段走子**（吃子/前进/翻棋）全失败时**返回 0 且不调 check_stalemate**——困毙判定缺失。
- **修复**：结尾加 `check_stalemate(st)`（无可走子/可翻子 → 无子可动 → 该方负，L57）。
- **验证**：AI 困毙（蓝剩地雷+军旗不可动）winner=1 红胜 PASS（原 FAIL winner=255）；
  AI 边界（蓝剩炸弹+军旗）1步终局 PASS；AI vs AI 200 局全终局；压力 5000 局（全终局 0 非法）；
  困毙 4/4、和棋 4/4、先手定色 9/9。
- 重打包：arm64 .so（14 JNI）→ dec → apktool b → STORE → SignV2；phase_hand.apk 14.47MB（Verifies v1/v2/v3）。

## L67 · default.lay 完整字节映射确证（25子/方 rank 分布 + 军旗位置）
- **原版 default.lay 30 字节**（SPAK 解包 `data_lay_default.lay`）：`03 07 06 05 08 0a 00 09 00 01 02 0a 00 09 05 04 00 06 00 07 08 03 04 09 0b 0b 0c 0b 08 07`。
  = 每方 6行×5列=30 格（行主序），0=空（行营 5 格/方）。
- **arm64 `jq_load_layout` 映射确证**（字节级对齐原版）：
  - 蓝方：layout 行 r → 棋盘行 (5-r)，军旗(lrow5)→棋盘行0；红方：行 (6+r)，列镜像（4-lcol）。
  - **军旗位置 PASS**：蓝军旗=格1(row0,col1)、红军旗=格58(row11,col3)——与原版一致。
  - **rank 分布 PASS**：每方 25 子（司令1军长1师长2旅长2团长2营长2连长3排长3工兵3炸弹2地雷3军旗1），
    蓝=红=25、空=10。
- 验证：军旗位置、25子/方 rank 分布、default.lay 布局 PASS；此前 L56 已确证 30 字节一致。

## L68 · 原版"十"字铁路几何确证（纵向 col2 + 横向 row5/6）
- **原版铁路真值确证**（标准军棋"十"字铁路）：
  - **纵向 col2 贯穿所有行**（被行营 (3,2)(8,2) 分成三段：0-2/4-7/9-11）；
  - **横向 row5/6（中线）贯穿所有列**；
  - **行营/大本营阻断**（row2/4/7/9 行营、row0/11 大本营在铁路行上阻断）；
  - **col0/col4 非铁路**（只相邻一步，不能纵向滑行）。
- **修正**：`jq_is_rail` 旧"全铁路"（L48 简化）→ 原版十字；`rail_path_straight` 横向限 row5/6；
  `rail_path_engineer` 横向限 row5/6。
- **验证 9/9**：row5/6 横向全滑、col2 纵向上段滑、col2 穿行营非法、col0 纵向非法、相邻一步合法、
  row0 横向非法（非铁路+大本营）、工兵穿行营非法、普通子拐弯非法。
- 回归全绿：布局/军旗/规则/翻棋/暗棋/回归/对齐/AI/压力 5000 局（全终局 0 非法）。
- 重打包：arm64 .so（14 JNI）→ dec → apktool b → STORE → SignV2；phase_hand.apk 14.47MB（Verifies v1/v2/v3）。

## L69 · 悔棋 undo 完整确证（走子/吃旗/炸弹/翻棋恢复）
- **arm64 undo 快照**：board/turn/stage/winner/flags_captured/pieces_left/mines_cleared/moves_since_eat（L63-L66 加字段已含）。
- **验证 17/17**：
  - 走子 undo（格20→21 恢复格20有子/格21空/turn）；
  - 吃军旗 undo（恢复军旗/终局回 PLAY）；
  - 炸弹同归于尽 undo（恢复炸弹/司令）；
  - 翻棋 undo（恢复 revealed/side）。
- **原版悔棋**：rodata `Junqi.RegretReq/RegretAgree/RegretDisagree`（悔棋请求/同意/拒绝，联机协议）——arm64 本地 undo 对齐悔棋语义。
- 回归：走子/吃子/炸弹/翻棋全部可 undo，无状态残留。

## L70 · 暗棋 AI 走子完整（翻 UNKNOWN 子/吃已翻子/任一可动兜底/自身困毙）
- **AI 阶段3 修复**：暗棋模式翻 side=UNKNOWN 子（先手定色后，原只翻 side==me 子→卡住）。
- **AI 阶段2b 兜底**：AI 前方一格被围时，遍历全格找任一合法走子（含铁路/横向）。
- **暗棋吃子限制**：只能吃已翻子（side 已定）；UNKNOWN 未翻子不可吃（无 owner，ruleflip）。
- **AI 自身困毙**：jq_ai_move 开头 side_has_movable(me) → 判负（原只查 turn，AI 方困毙漏判）。
- **side_has_movable 暗棋**：UNKNOWN 非军旗/地雷暗子可翻；地雷/军旗暗棋不可翻（不计）。
- **验证**：暗棋 AI vs AI 70步终局（winner=2 和棋，70步无吃子——翻棋计步，原版 ruleflip 同样）；
  AI vs AI 200 局全终局、AI 困毙/边界 PASS、ruleflip 6/6、先手定色 9/9。

## L71 · 走子事件分类（eat_type 输出）对齐原版 Junqi.Move/Eat/Bomb/Flag/Dead 音效
- **原版走子事件**（rodata）：`Junqi.Move/Flip/Flag/Bomb/Eat/Dead`——6 类，对应 SPAK 12 个 wav
  （move/flip/eat/bomb/flag/dead/draw/win/lose/select/start/over）。
- **arm64 jq_try_move 加 eat_type 输出**：0=move(空移) 1=eat(普通吃) 2=bomb(炸弹同归/同rank同归)
  3=flag(扛旗) 4=dead(被吃/被雷炸) 5=mine(工兵挖雷)。
- **JNI 音效映射**：sfx[6]={"move","eat","bomb","flag","dead","eat"}——覆盖原版 6 走子事件；
  胜负优先 win/lose/draw；AI 走子 act==3 用 eat（简化）。
- **验证 7/7**：空移 move、普通吃 eat、炸弹同归 bomb、扛旗 flag、被雷炸 dead、工兵挖雷 mine、同rank同归 bomb。
- 回归：炸弹撞军旗 6/6、大本营地雷 7/7、回归 15/15、对齐 5/5、ruleflip 6/6、悔棋 17/17、
  军旗吃旗胜、地雷/炸弹 PASS、压力 5000 局（205 万步全终局 0 非法）、AI 200 局全终局。

## L72 · 规则完整性审计（rulelay/ruleflip 全条款逐条对齐确证）
- **rulelay 全条款确证**：
  - 公路线1步/铁路工兵任意/其他直走（L68 十字铁路）；
  - 行营安全岛/军旗大本营/进大本营不可再动；
  - 炸弹不放第一行/地雷最后两行/军旗大本营（L59 布局校验）；
  - 25枚棋子摆兵站大本营/杀光对方/扛旗胜；
  - 司令>...>工兵/炸弹同归含军旗/地雷<工兵>其他/任何棋扛旗；
  - 吃光/扛旗/无棋可走判负/70步和/1000步和。
- **ruleflip 全条款确证**：
  - 行营大本营安全岛/大本营地雷军旗可吃（L63）；
  - 先手翻子定色/杀光/工兵挖3雷扛旗（L63/L64）；
  - 司令>...>工兵/炸弹同归/地雷军旗仅工兵吃/挖完扛旗；
  - 吃光/扛旗/无棋可走判负/70步和/1000步和。
- **规则层原版完整对齐**：两模式全部条款逐条落地并验证（L53-L71 各测试全绿）。
- **审计方法**：SPAK `data_junqi_rulelay.txt`/`ruleflip.txt` GBK 解码逐条比对 arm64 实现。

## L73 · 0x485d0 菜单标签完整确证（AI 难度档数 = 3 档 + Rank）
- **0x485d0 菜单构建**：9 组 `ldr r1,[pc,#imm]` + `add r1,pc` + `bl 0x82d3c`（字符串比较），
  ldr 字面量表 @0x48760-0x4878c（值 0xda268/0xda246/0xda224/0xda202/0xda1e2/0xda1c2 递减 0x22）。
- **8 个难度/模式标签确证**（rodata_dec 0x12284c 起）：
  - `LowLay`@0x12284c / `LowFlip`@0x122854 — 低难度 × 布局/翻棋
  - `MidLay`@0x12285c / `MidFlip`@0x122864 — 中难度 × 布局/翻棋
  - `HighLay`@0x12286c / `HighFlip`@0x122874 — 高难度 × 布局/翻棋
  - `RankLay`@0x122880 / `RankFlip`@0x122888 — 排名模式 × 布局/翻棋
- **结论**：原版单机难度 = **3 档（Low/Mid/High）+ Rank（排名）**，每档有 Lay/Flip 两种玩法。
  0x485d0 是单机难度选择菜单（sgleveldlg 对应项），`bl 0x82d3c` 为字符串比较（选中高亮）。
- **第 9 组 ldr**（target 0x121cae）落在 'Menu' 附近，非难度标签（函数内其他引用）。
- **arm64 边界**：单档贪心 AI（诚实）；3 档难度 + Rank 需逐档实现（difficulty 存档字段已确证 L71）。
- **解析方法**：切片反汇编（0x485d0-0x48760）→ ldr 字面量表 → `target = val + (add_addr+4)&~3`；
  全段扫描会因数据字面量池提前终止，必须按函数切片。第一项 val=0xda268 target=0x12284c='LowLay' 确证。

## L74 · 存档 one.sjq 格式完整确证（读写函数 + 字段偏移）
- **读取函数 0x37664** / **写入函数 0x379ec**（one.sjq 读写，`bl 0xb2d84/0xb2da8` 读字段、`0xb3638/0xb38c0` 写字段）。
- **存档字段偏移确证**（场景对象 r6，0x3772e-0x37792 的 `str r0,[r6,#imm]`）：
  - `[r6,#0x38]` = time（0x3772e）
  - `[r6,#0x3c]` = **difficulty**（0x3773a）—— AI 难度
  - `[r6,#0x40]` = level（0x37748）—— 关卡
  - `[r6,#0x44]` = mark（0x3775a）—— 分数
  - `[r6,#0x48]` = resultEx[4]（0x37792 循环 4 次 ×4 字节，0x37788-0x3779c）—— 4 局结果
- **与 rodata 字符串完全一致**：`one.sjq` + `time/difficulty/level/mark/resultEx` + `%d;%d;%d;%d`（resultEx 4 值）。
- **字段读取方式**：`bl 0xb2d84/0xb2da8`（字段名 → 值）、`bl 0xb2bf4`（格式化）；写入 `bl 0xb3638/0xb38c0`。
- **arm64 边界**：无存档（诚实）；原版存档含 time/difficulty/level/mark/resultEx 5 字段。
- **difficulty 值语义**：菜单 0x485d0 经函数指针回调（非直接 bl），选中值在 UI 框架层——静态确证档数=3（L73）但具体值范围（0/1/2）在回调，诚实边界。

## L75 · 棋子 PNG 图集确证（datah_124-174 + datax_124-174）
- **棋子 PNG 图集确证**：SPAK（.pak）解包有 41 个 72×72 + 36 个 108×108 RGBA PNG，
  集中在 `datah_124-174`（72×72）+ `datax_124-174`（108×108）——连续索引 = 棋子图集。
- **内容分析确证**：72×72 PNG 中心为圆形棋子（alpha 通道 ASCII 可见圆形轮廓 + 中央 rank 符号）；
  主色分布以蓝方（datah_124-174 大多蓝色>红色）+ 少量红方（datah_165/171/172 红>蓝）。
- **.pak 来源确证**：`assets/datah0.pak`/`datah1.pak`（棋盘+棋子 72×72）、`datax0-3.pak`（xhdpi 108×108）、
  `data0.pak`/`data1.pak`（低清）、`datam.pak`（中清）——原版资源包，arm64 spak_load.c 解包。
- **arm64 边界**：当前用 rank 数字+符号渲染（L56 诚实边界）；可改进为加载 datah_124-174 按 rank/side 索引渲染，
  但 rank→索引映射在 C++ 渲染层（0xbd914 vtable），诚实边界。
- **棋盘 PNG**：datah_188（737×751）+ datax_297/298（736×736）已确证（arm64 render_gles2.c 已用）。

## L76 · 原版 15 个 JNI 完整确证（含 OnUpdate/OnTimer AI 入口）
- **原版 JNI 符号完整**（.symtab，奇数=Thumb）：
  `OnEvent=0x233e1/OnInput=0x233f5/OnResult=0x23409/OnBluetooth=0x23425/OnTouch=0x23439/
   OnKey=0x2344d/OnTimer=0x23461/OnUpdate=0x23475/OnGraphChange=0x23489/OnGraphCreate=0x2349d/
   OnResume=0x234b1/OnPause=0x234c5/OnDestroy=0x235d5/OnCreate=0x23561`（14 个导出 + 内部）。
- **AI 走子入口确证**：`OnUpdate`（0x23474）/`OnTimer`（0x23460）为 JNI 桩，均 `bl 0x8598`（JNI 分发核心）
  ——原版 AI 走子由 Java 定时器 → OnTimer/OnUpdate 触发（native 内不轮询）。
- **arm64 对齐**：junqi_jni.c 已实现 OnUpdate（AI 自动走子，jq_ai_move）——与原版 OnTimer 触发机制一致；
  OnEvent 事件分发（Junqi.* → 动作）已对齐（L71）。
- **JNI 桩共性**：OnEvent/OnTimer/OnUpdate 都 `bl 0x8598`（JNI 参数解析/分发），返回后处理。
- **差异说明**：原版 OnTimer（Java 定时器，低频）+ OnUpdate（每帧）；arm64 统一 OnUpdate 每帧触发 AI（行为近似）。

## L77 · 原版 Junqi.* 事件全集确证（73 个）+ arm64 覆盖审计
- **原版事件全集确证**（rodata_dec，73 个 `Junqi.*`）：
  - **走子 6 个**：Move/Flip/Flag/Bomb/Dead/Eat（arm64 eat_type 全覆盖，L71）。
  - **胜负 6 个**：RedWin/BlueWin/Win/Lose/Draw/Over（arm64 win/lose/draw 覆盖）。
  - **布局错误 4 个**：ZhaError(炸弹第一行)/LeiError(地雷非最后两行)/QiError(军旗非大本营)/LayError。
  - **悔棋/联机 15+**：RegretReq/RegretAgree/RegretDisagree/RegretSend/RegretConfirm/RegretFail、
    PeerStart/PeerLayFinish/LeaveGame/DrawReq/ConnectFail/Reconnecting/JoinGame/Ready/PlayerLeave。
  - **登录/注册 20+**：LoginFail/RegFail/InputUser/InputPassword/InputEmail/RegOk/RegExist 等。
- **arm64 覆盖审计**：走子 6 个 ✓ + 胜负 6 个 ✓（音效映射 L71）+ 悔棋（jq_undo ✓）；
  **布局错误/联机/登录注册** 未做字符串提示（诚实边界）。
- **布局编辑差异确证**：原版 rulelay 有玩家布局编辑（拖 25 子摆位）+ 0x39dac/0x39d44 校验
  （返回校验结果，ZhaError/LeiError/QiError 由 UI 层显示）；arm64 OnCreate 固定 default.lay
  （spak_get("data/lay/default.lay")），**无玩家布局编辑**——功能差异（诚实边界）。
- **0x39e14 校正**：L59 记"0x39e14 布局校验"，实为 0x39dac/0x39d44 附近函数体（0x39e14 是数据）；
  布局校验入口实际是 0x39e02 bl 0x39dac + 0x39e30 bl 0x39d44（校验+保存链）。

## L78 · arm64 JNI 接口完整对齐确证（14/14 与原版一致）
- **arm64 14 JNI 导出**（readelf --dyn-syms）逐一匹配原版：
  `OnBluetooth/OnCreate/OnDestroy/OnEvent/OnGraphChange/OnGraphCreate/OnInput/OnKey/
   OnPause/OnResult/OnResume/OnTimer/OnTouch/OnUpdate`——**14/14 完全一致**。
- **junqi_jni.c 实现全部分发**：OnTimer(161)/OnGraphCreate(189)/OnGraphChange(198)/OnResult(208)/OnKey(272)
  等均有实现（OnTimer 为日志桩，OnGraph* 渲染尺寸，OnResult/OnKey 事件透传）。
- **AI 触发差异**：arm64 OnUpdate 每帧触发 AI（jq_ai_move）；原版 OnTimer（Java 定时器）触发——行为近似
  （原版低频定时 vs arm64 每帧，均自动走子，诚实边界）。
- **JNI 接口完整**：15 个 JNI 原版导出 14 个（On*），arm64 14 个逐一匹配——**接口层 100% 对齐**。

## L79 · 原版棋盘维度/格子布局确证（12×5，每格 8 字节）
- **原版棋盘 index 确证**（0x6ef82-0x6ef94）：`r7*40 字节 = r7*5格×8字节`（每行 5 格 × 8 字节/格），
  行内偏移 `#0x18`（第 3 格）——**原版棋盘 12 行 × 5 列，每格 8 字节**。
- **arm64 对齐**：`JQ_BOARD_ROWS=12, JQ_BOARD_COLS=5, JQ_CELLS=60, jq_cell_index(row,col)=row*5+col`
  ——维度/索引**完全一致**。
- **格子布局差异**：原版每格 8 字节（[0]=col,[1]=row 坐标缓存 + rank + 状态，L68）；
  arm64 `jq_cell`=6 字节（rank/side/revealed/is_flag/is_mine/is_bomb 紧凑，**无 col/row 缓存**，用 index 寻址）。
  差异为内存布局（arm64 更紧凑），规则逻辑一致（测试全绿）。
- **棋盘 PNG**：datah_188 737×751（12×5 格子映射，arm64 render_gles2.c 已用）。
- **rank 编码**：arm64 jq_cell.rank 存 JQ_RANK（1-9/10/11/12），原版 0x6a3a4 蓝rank/红rank+0xc（L60）——语义一致。

## L80 · 布局加载函数校正（L60 修正：0x2a340 是 vector insert，实际在 0x2a37c 场景构造）
- **L60 校正**：0x2a340 实为 vector insert（0x2a310-0x2a374，同 0x39f50 模式），非布局加载；
  **布局加载在 0x2a37c 场景构造**（bl 0x2a310 vector init + bl 0x8598 JNI 调用）。
- **default.lay 加载链确证**：场景构造 0x2a37c → 0x2a310（30 字节容器 init）→ 0x8598（JNI 读 default.lay）
  → 0x6a3a4（rank 映射，蓝 rank/红 rank+0xc，L60）→ 布局落子。
- **GOT 引用**：0x2a38c/0x2a3a2 引用 GOT 变量（default.lay 路径动态构造），非字符串直接引用。
- **arm64 对应**：jq_load_layout（spak_get("data/lay/default.lay") → 30 字节 → layout_rank 映射）——对齐原版链路。
- **军旗位置**：蓝方 layout 行 5 → 棋盘行 0（蓝1），红方 layout 行 5 → 棋盘行 11 列镜像（红58）——已确证（L67/L79）。

## L81 · arm64 布局编辑（布阵阶段）——原版 0x2a228 逐格校验 + 触屏拖子
- **原版校验语义直接反汇编重读（0x2a228）**：笔记 §10 的"蓝方区域合法"标注**反了**——
  实际 `r1<=0x1d(29)` 返回 0（非法），红方自定义布局仅允许行6-11。教训：布局索引语义必须
  直接读指令，不能凭笔记注释推断。
- **"第一行"=前线路（非底线）**：0x2a228 炸弹禁 `r1<=0x22(34)`=禁行6（红方前线路），
  非"禁底线"。红方对称到蓝方=禁行5。default.lay 炸弹在行3-4（非行5）印证。
- **红方校验对称映射**：炸弹禁行6/地雷仅行10-11/军旗格56/58
  → 蓝方（row0=底线）炸弹禁行5/地雷仅行0-1/军旗行0列1/3。
- **rulelay 全条款落地**：炸弹禁第一行、地雷仅最后两行、军旗仅大本营（data_junqi_rulelay.txt GBK，L57/L72）。
- **arm64 API**：jq_setup_begin/place/validate/confirm/is_setup + JQ_LAYOUT_ZHA/QI/LEI/COUNT/BAD_POS。
  place 用原版逐格语义（源/目标双向校验，交换整格）；validate 全量（25 子标准军力分布 + 位置规则）；
  confirm 校验通过才进 PLAY（双方暗置、军旗明置、先手蓝）。
- **UI 约定**：布阵阶段蓝方 25 子全翻开可编辑、红方暗置预览；底部确认按钮（绿底白勾，FONT_CHECK 5×7）；
  棋盘缩小（board_region bh 0.92→0.62）留按钮条；hit-test 与绘制共用 setup_button_rect 避免漂移。
- **诚实边界**：arm64 布阵=蓝方拖子（原版 0x2a5f0 红方自定义布局，蓝方对称）；无难度布局菜单（§13）；
  暗棋(ruleflip)无布阵（L63 随机）。
- **验收**：test_setup 28/28 PASS；既有回归全绿；arm64 49.6KB 14 JNI；phase_hand.apk Verifies v1/v2/v3，
  manifest 0x80003，.so 全 STORE，arm64 MD5 匹配。

## L82 · arm64 AI 三档难度（原版 Low/Mid/High/Rank 菜单）
- **原版档数确证**：L73 菜单 8 标签（LowLay/LowFlip/MidLay/MidFlip/HighLay/HighFlip/RankLay/RankFlip）
  → 3 难度 + Rank；难度决策在场景 vtable（运行时填充），静态无法确证 → arm64 实现**三档决策强度**。
- **难度布局文件未提取**：jq1/jq2/jq3.lay、data/lay/%02d.lay 不在 SPAK（仅 default.lay），
  且扫描 .so rodata 全段无 30 字节布局数组（标准军力分布匹配）→ 三档共用 default.lay（诚实边界）。
- **分档结构**：LOW=随机失误（50% ai_random 全动作等概率）；MID=原贪心（L43，默认，行为不变）；
  HIGH=价值+安全最优吃子（ai_value + ai_capture_risky 反吃模拟）。
- **HIGH 安全模拟**：jq_try_move 快照→敌方反吃检查→jq_undo 恢复。关键：jq_try_move 自快照 undo，
  jq_undo 恢复 board/turn/stage/winner/flags/pieces/mines/moves_since_eat 且 moves 增后自减——干净往返，
  不污染 undo 历史（每候选一次快照+一次恢复，净 0）。
- **子价值表**：rank 越小越强（1 司令=90...9 工兵=10），炸弹=90、地雷=80、军旗=10000（吃旗=胜）。
- **验收**：test_ai_level 9/9 PASS；三档各 5 局 AI vs AI 全终局（LOW 865/MID 745/HIGH 730 步，HIGH 更高效）；
  test_ai_multi 200 局全终局（MID 默认未回归）；arm64 51.9KB 14 JNI；phase_hand.apk Verifies v1/v2、
  manifest 0x80003、.so 全 STORE、arm64 MD5 匹配、SPAK 9 包完整。
- **API**：jq_ai_set_level(0/1/2)/jq_ai_get_level；OnEvent "ai_level"=<n>。

## L83 · arm64 存档 one.sjq（原版 0x37664 读 / 0x379ec 写）
- **字段确证（rodata VA）**：time=0x121744/difficulty=0x12174c/mark=0x121760/resultEx=0x121768；
  resultEx 序列化 `"%d;%d;%d;%d"`=0x121774。L74 字段偏移 0x38/0x3c/0x40/0x44/0x48 全对上。
- **原版写逻辑复杂度**：0x379ec 是长链（写 time/difficulty/level/mark 后还有 0x2bc/0x2c0 高级字段 +
  resultEx[4] 循环 0x11 次）→ 键值分隔符/顺序未逐字节确证 → arm64 用 `key=value\n` 重建（诚实边界）。
- **arm64 实现**：src/save.c `jq_save_write/read`（fopen/fgets 逐行解析），
  `jq_save_data{time,difficulty,level,mark,resultEx[4]}`；缺失字段保持默认（容错）。
- **JNI**：OnCreate getFilesDir()/one.sjq；OnEvent save/load；胜负记 resultEx；OnUpdate 60 帧/秒计 time。
- **验收**：test_save 16/16 PASS（round-trip 全字段/文本字段名/%d;%d;%d;%d/缺失容错/读不存在=0）；
  arm64 54.7KB 14 JNI；phase_hand.apk Verifies v1/v2/v3、manifest 0x80003、.so 全 STORE、arm64 MD5 匹配。
- **API**：jq_save_write(path,sd)/jq_save_read(path,sd)；OnEvent "save"/"load"；resultEx=[玩家胜,AI胜,和,总盘]。
