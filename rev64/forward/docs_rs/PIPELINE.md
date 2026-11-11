# 军棋逆向 → 正向开发 流水线（PIPELINE）

> 把两会话已验证的路线固化成**可复用、可批量、可回归**的工程流水线：
> `arm32 so → 翻译成 C（对齐）→ 验证 C → 正向开发（src_rs）→ 与原版补差异（unidbg/frida）`。
> 每一段 = 一条命令 + 一组判据 + 一个诚实边界。**不包装"简化版"成"完整版"**。
> 数据口径全部 C.5b 实测（2026-11-05），随版本演进。

---

## 0. 全景：四段流水线 + 三个回路

```
┌─────────────────────────────────────────────────────────────────────────┐
│ 段 1  逆向证据提取（原版 32 位 so）                                        │
│   .rodata 明文 dump → 1715 串 + 214 PLT + 5312 RELATIVE + 1479 dynsym    │
│   核心函数反汇编（0x39e14/0x6a3b0/0x302b4…）→ 规则/布局/事件名常量          │
│        │                                                                │
│        ▼                                                                │
│ 段 2  正向开发 src_rs（行为级重写 C，arm64）                                │
│   规则引擎 + UI + 音效 + 蓝牙帧 + 渲染（EGL/GLSL 桩）                        │
│   一条命令回归 12/12 + 桌面事件 168/168 + 真机全套 9/9                          │
│        │                                                                │
│        ▼                                                                │
│ 段 3  真机验收（"能玩"的唯一判据）                                           │
│   MuMu 64 位 GLES1 几何受限 → PBuffer 离屏出像素已证，屏幕可见需真机 arm GPU    │
│        │                                                                │
│        ▼                                                                │
│ 段 4  差异补齐（unidbg/frida 喂对象图真值）                                 │
│   补"行为级重写"覆盖不到的：原版 C++ 对象图 / STL 布局 / 字节级渲染细节        │
└─────────────────────────────────────────────────────────────────────────┘
  回路 A：段 1↔段 2  逆向出新常量 → 更新 src_rs 规则/布局 → 回归（rs_cycle）
  回路 B：段 3↔段 4  真机崩/分叉 → frida 抓对象图真值 → 注入/补 stub → 回归
  回路 C：全程     check_consistency + course_sync_scan 双脚本防口径漂移
```

**核心方法论（每段通用）**：
1. **要么纯指令翻译，要么反汇编成 C，函数级回归**（输入一致→输出一致，≥90% 可机械化）
2. **判据含副作用**：不只比返回值，比 `crc32(结构体区)+crc32(.data)`（sp 区剔除）
3. **诚实边界三件套**：能跑≠能玩 / 指令 100% 对齐≠功能一致 / 简化版≠完整版

---

## 段 1：逆向证据提取（原版 32 位 so）

### 1.1 数据源（已固化，静态即明文，非壳）
| 证据 | 值 | 提取方式 | 落点 |
|---|---|---|---|
| `.text` / `.rodata` 明文 | 熵 7.10（正常机器码） | objdump 反汇编 / 直接读 | `frida-dump/text_dec.bin` |
| `.rodata` 字符串 | 1715 条（Junqi.Move/Flip/Flag/Bomb/Eat/Dead + data/config/*.xml） | `dd /proc/PID/mem`（redroid 原版 so dlopen） | `frida-dump/rodata_dec.bin` |
| PLT | **214** JUMP_SLOT | `readelf -rW so | grep -c JUMP_SLOT` | `tex_pipeline.py plt` |
| dynsym | **1479** 符号 | `readelf -sW so` | 同上 |
| RELATIVE | **5312** 槽 | `.rel.dyn` 解析 | `jqt_mem.c` `jq_rel_data.h` |
| 18 个 `.text` 常量 | 72B（15 `.text`+2 `.data`） | **★ §124x-z 后静态位移固化 `jqt_mem.c` `g_jq_got_tbl`**（免 redroid dump） | `jqt_mem.c` |

### 1.2 核心函数 → 规则/布局（src_rs 逆向证据链）
`rev64/forward/docs_rs/R-route_reversal.md` 固定 4 张表（**这是段 1 的核心产物**）：
- **控制流**：走子/吃子/炸弹/悔棋/困毙的分支（0x39e14/0x39f20/0x2a228/0x6a3b0/0x302b4）
- **常量**：rank 序 / 50 子（12×5）布局 / 地雷炸弹 / 70步1000步判和
- **结构布局**：棋盘 60 格 / 事件累计 / undo 栈
- **业务名**：`Junqi.*` 事件契约（与 32 位 `.rodata` 明文逐字对齐）

> **思考发现 1**：段 1 的"明文"性质（非壳）是本流水线成立的前提——
> `.text`/`.rodata` 静态即明文（0/118832 字节不同），**无运行时自解密**，
> 所以"反汇编直接看"是性价比最高路线，unidbg/红roid dump 仅为确认。

---

## 段 2：正向开发（src_rs 行为级重写 C）

### 2.1 产物（arm64，14 JNI 签名与 App.smali 一致）
- `rev64/forward/src_rs/`（~5549 行 C，规则引擎 + UI 状态机 + 音效 + 蓝牙帧 + EGL/GLSL 渲染桩）
- `dist/libjunqi_rs.so` + `dist/phase_rs_a64.apk`（com.cnvcs.junqirs，v1.6.8/168，双 ABI，v1+v2）

### 2.2 一条命令全回归（`rs_cycle.sh`，防"还有哪些没验证"）
`bash rev64/tools/rs_cycle.sh` = make so + **12/12 回归** + rebuild 装机启动。
`src_rs_regress.sh` 12 项（**判据全部 C.5b 实测，工具升级必同步 N25**）：

| # | 项 | 判据 | 验证层 |
|---|---|---|---|
| 1 | make desk + so | 编译 0 错 | 静态 |
| 2 | 蓝牙自测 bttest | "全部通过" | 帧协议 |
| 3 | 桌面走局 jq_run | 走一局 | 规则 |
| 4 | 规则回归 N 局 + AI 单测 | 通过率≥90% | 规则 |
| 5 | src_rs_check.py 红线审计 | 0 红线 | 架构 |
| 6 | 布局 12 预设 laytest | 12/12 | 布局 |
| 7 | 触摸命中 + 两步态机 touchtest | "全部通过" | 交互 |
| 8 | 完整对局 gametest 60 局 | "全部通过" | 全路径 |
| 9 | so_link_check.py 链接审计 | 0 悬空 UND | 链接 |
| 10 | 走子预验证 walkverify 3 步 | "3/3 步合法" | 走子 |
| 11 | 长路径走子 walkverify --longrun 5 | "5/5 回合 turn 交替持续" | 走子 |
| 12 | 桌面事件 evverify --all | **"169/169 全对"**（★ AUD-1000 B07-033/B08-049 口径同步：桌面事件断言面 169 项 = 事件/业务 39（基础事件 20 + 弹窗业务分支 3 代表 + 规则回归等）+ 几何 130（120 弹窗几何 + 3 确认框）；旧"8/8 全对"系断言面扩容前的口径，工具升级必同步判据 N25 红线） | 事件 |

### 2.3 三级验证链（八件套，防"盲目真机试探"）
```
⑥ 桌面预验证（evverify 8 断言 + walkverify，免真机 flaky）
   ↓ 通过才上真机
③⑤ 真机单点（mumu_walktest 触摸 + mumu_evtest setprop 夹具，多轮重试兜底）
   ↓ 单点通过
⑧ 真机全套批量（mumu_fulltest 9/9：走子 EAT + FLAG + MINE + 重开 + 和棋 + 认输 + 困毙 + UI 关闭/弹窗音效，一次跑完）
```
> **思考发现 2**："先桌面→再真机单点→后真机全套"是**成本递增的过滤网**——
> 桌面 168 断言 0 成本就能抓规则/事件回归（★ B07-033 口径：168 = 事件/业务 38 + 几何 130），真机 9/9 才花 MuMu 时间。
> 触摸 flaky 由 `mumu_walktest` 多轮重试兜底，**不手写 for 循环**。

### 2.4 链接/反射安全门禁（每轮重建必跑，防"改一处崩全链"）
- `so_link_check.py`：UND 符号 vs 基线 diff（坑 1：悬空 `F_xxx` → failed to link）
- `jni_method_audit.py`：所有 GetMethodID 后 8 行内有无 ExceptionCheck/ExceptionClear（坑 2/3）
- 判据：JNI 反射代码新增/修改后 **两个都 exit=0 才放行**

> **思考发现 3**：12 坑 3 类（JNI 反射 2/3/6/7 + C 语义契约 4/5/8/10/11 + 链接 1）
> 已全部工具化拦截（so_link_check + jni_method_audit + 返回契约 int 化），
> **"坑"从"人记得"变成"脚本卡住"**——这是流水线化的核心收益。

---

## 段 3：真机验收（"能玩"唯一判据）

### 3.1 验收判据（统一，四阶段通用）
`装上 → 进主界面 → 下一局（本地/蓝牙）→ 正常退出 → 重进不崩`

### 3.2 MuMu 能力边界（诚实，不夸大）
| 能力 | 实证 | 限制 |
|---|---|---|
| 64 位 EGL 出帧 | Mesa 转译，PBuffer 离屏 `glReadPixels` 出像素（棋盘 737×751） | GLES1 固定管线路径几何 no-op；**GL2 context + VBO 通路完整** |
| 音效播放链 | MediaPlayer reset→setDataSource→prepare→start JNI 全通 | **出声需真机音频通路** |
| 触摸注入 | OnTouch→native 全链可达 | MuMu `input tap` 间歇 flaky（环境限制非代码 bug） |
| 屏幕可见 | window surface 通路未通（Java SurfaceView 未真正创建） | **屏幕可见需真机 arm GPU 验收** |

#### 3.2.1 MuMu 资源通路被系统权限挡死（2026-11-10 实锤，"玩不了"的确切根因）
- **实证**：MuMu（`127.0.0.1:5556`，redroid12_x86_64 容器）装 phase3.apk 后 `JNI_OnLoad` 成功（so `c3192e57` LOGE 修复后 dlopen 通）、window surface 创建成功、GL slot0 编译成功，**但资源全 MISS**（board/piece/menu_bg 纯色块退化）：
  - 通路 ① `/data/local/tmp/spak_res`：目录 `drwxrwx--x shell:shell`，app uid `u0_a71` 无 x 权限进入 → `fopen` 读不到（**红果 x86_64 上同目录权限宽松才读到 135 色块**）
  - 通路 ② `/sdcard/spak_res`：属 `u0_a68:media_rw`（私有媒体区），app uid `u0_a71` 被 `MediaProvider: Permission to access file ... is denied` 拦（SELinux 媒体区隔离）
- **结论**：MuMu 上 app 进程**两条资源通路都被系统权限机制挡死** → 无法读 SPAK 位图 → 画面纯色块，**达不到真机出图验证标准（"玩不了"= 画面退化的确切实锤，非 so 逻辑 bug）**
- **修复方向（未落地）**：把 SPAK 资源打进 APK `assets/`（代码 `rs_read_one` 已有 AAsset 兜底通路 `asset_path="spak_res/xxx"`，`g_assets` 可达时命中），免设备端推目录。当前 `g_assets=NULL`（assets 管线未启用）→ 需启用 AAssetManager 资产管线

> **思考发现 4（关键诚实边界）**：
> "PBuffer 离屏出像素" ≠ "画到屏幕" ≠ "能玩"。
> **段 3 的"能玩"判据只有真机 arm64/32 位 arm GPU 设备能下**，MuMu/redroid[已禁用 2026-11-09]/unidbg 都替代不了。
> 流水线里段 3 是**判据门**，不是"跑通 MuMu 就通过"。
>
> **★ 2026-11-10 MuMu 真机验证未达成（实锤）**：MuMu 装 phase3.apk 后 so 加载/window surface/GL 编译全通，
> 但 SPAK 资源两条通路（/data/local/tmp 无 x 权限 + /sdcard 媒体区 SELinux denied）均被系统权限挡死 →
> 资源全 MISS → 画面纯色块，**达不到"进主界面→下一局"的出图判据**。修复需启用 APK assets 管线（g_assets 可达），
> 当前目标未达成，继续攻坚。红果 x86_64（10.0.182.146）/data/local/tmp 权限宽松，出图已实证 135 色块（权威口径）。

---

## 段 4：差异补齐（unidbg/frida 喂对象图真值）

### 4.1 定位（不重复段 2 已解决的）
段 2 正向开发已覆盖**行为级规则/事件/交互**。段 4 补的是**行为级重写覆盖不到的**：
- 原版 C++ 对象图（std::list/vector 真实布局 + 场景对象 vtable + 运行时字段填充）
- 字节级渲染细节（原版 GL 几何/纹理/光照，简化版是占位）
- 原版独有玩法（50 子（12×5）精确布局 / 地雷炸弹细则 / 蓝牙 7 字段 C++ 类逐字段）

### 4.2 三条路对照（选哪条补差异）
| 路线 | 强项 | 墙（已实证） | 适用 |
|---|---|---|---|
| **unidbg/unicorn2 模拟原版 so** | 补环境/反检测/花指令/壳 | ①上下文相关 BL 解码 bug ②数据与指令对齐对应缺口 ③288MB PROT_NONE mmap | 静态提取 + 指令级对齐验证 |
| **frida/redroid dump** | 提 `.rodata`/`.bss` 运行时值 | 32 位 so 不被 64 位 bridge 加载 + 无 GPU + 跨进程指针不可回填；★ redroid 容器已禁用（2026-11-09 镜像/容器已删，仅存静态产物 frida-dump/*.bin） | 数据提取（对象图"值"带不回；redroid 路线作废，frida 深度 dump 仍需真机 32 位设备） |
| **32→32 翻译体（gate-set）** | 指令语义 100% 对齐 + 写侧对象图构造 | 场景对象图数据（std::list node next / 字段）需真实 C++ 构造链 | 渲染链/对象图构造攻坚 |

### 4.3 工具（已固化，防手写）
- `frida_dump_objects.py`（frida 抓对象图真值）
- `gen_shadow.py`（指令翻译 → C 影子运行时，含 ARM thunk 覆盖 + bx lr 桩）
- `tex_pipeline.py`（vtable/GL 序列/closure 量化，`closure <addr>` 一键算缺口）
- `spak_unpack.py`（SPAK 资源解包，棋盘/棋子/音效）
- **回归红线**：补差异时"可跑函数一致率"= PASS/(PASS+MISMATCH)，STL 随机输入归 SKIP 剔分母（§122）

> **思考发现 5（流水线最深层认知）**：
> **"补齐 stub"补的是代码，破不了"运行时 C++ 对象图数据"**。
> 段 4 的真正瓶颈不是翻译器（指令 100% 对齐），而是**真实 C++ 构造链跑通 + 对象图填充**。
> 所以段 4 的退出条件 = "对象图数据填充 + 真机 arm GPU 验收"，不是"函数全翻译"。

---

## 回路（流水线不是单向，三个反馈回路）

### 回路 A：段 1↔段 2（逆向 → 正向 的常量流）
`R-route_reversal.md` 出新常量 → 更新 `src_rs` 规则/布局 → `rs_cycle.sh` 回归 12/12。
**防"正向开发凭感觉"**：每条规则/布局必须能指到段 1 的某个 `.rodata` 串或反汇编函数。

### 回路 B：段 3↔段 4（真机 → 补差异 的 bug 流）
真机崩/分叉 → `frida_dump_objects.py` 抓对象图真值 → 注入/补 stub → 回归。
**防"真机盲改"**：先 frida 定位差异点，再定向补，不撒网式改。

### 回路 C：全程（口径防漂移）
`check_consistency.sh`（AGENTS↔课程 N/M 口径，含回归项/evverify 断言/fulltest 项/so 尺寸/旧口径残留）
+ `course_sync_scan.sh`（31 章正文 5 模式旧口径批量扫）
**双脚本 = 每轮 loop 开头一条命令全跑**（I3 机制化）。

---

## 流水线化 5 条固化原则（本轮总结，防"反复手写"）

1. **每段一条命令 + 一组判据 + 一个诚实边界**（rs_cycle / arm32_cycle / mumu_fulltest / check_consistency）
   —— 不手写多段 adb/apktool/zipalign 序列（§arm32_cycle 已固化"改→重编→重打包→装机→验证"）
2. **工具升级必同步判据（N25 红线）**：evverify 断言面扩容时（5→…→8→169），`src_rs_regress.sh` 第 12 项 grep 判据同步（★ AUD-1000 B07-033 口径：现判据 grep "全对"，断言面 169 项 = 事件/业务 39 + 几何 130；旧"8/8 全对"为扩容前口径，已被 §124y-163 批次 169 取代）；evverify 退码分级（B08-049：事件 FAIL 退 1 / 仅几何 FAIL 退 2）——不同步 = **假 PASS**（回归失效）
3. **坑 = 脚本卡住，不是人记得**：12 坑 3 类全部工具化拦截（so_link_check UND diff + jni_method_audit ExceptionClear + 返回契约 int 化）
4. **数据写前跑命令（C.5b 红线）**：写任何数据型表述（符号数/PLT/so 尺寸/回归项/断言数）前先跑 `readelf/stat/grep -c/make`，不凭记忆/快照（前 9 轮漂移全因"靠记忆写数据"）
5. **诚实边界三件套不松动**：能跑≠能玩 / 指令对齐≠功能一致 / 简化版≠完整版。**段 3"能玩"只有真机 arm GPU 能判**，MuMu/redroid[已禁用]/unidbg 都替代不了

---

## 当前时点状态（C.5b 实测 2026-11-05，随版本演进）

| 段 | 状态 | 证据 |
|---|---|---|
| 段 1 逆向提取 | ✅ 完成（明文 + 1715 串 + 214 PLT + 1479 dynsym + 5312 RELATIVE 全固化） | `frida-dump/` + `R-route_reversal.md` |
| 段 2 正向开发 | ✅ 行为级全闭环（12/12 回归 + 桌面 168/168 + 真机 9/9，14 JNI 签名一致） | `src_rs_regress.sh` + `mumu_fulltest.sh` |
| 段 3 真机验收 | ⚠️ MuMu 出帧/音效/触摸已证，**屏幕可见 + 出声 + 蓝牙 = 真机 arm GPU 待验收** | `mumu_fulltest` 9/9 + GLES2 PBuffer 离屏像素 |
| 段 4 差异补齐 | ⚠️ 三墙卡对象图（unidbg）/ 跨进程指针不可回填（frida）/ gate-set 写侧突破 1 层但场景对象图未填充（翻译体） | `../../reverse/docs/REVERSE_LOG.md` 终局反思 |
| 回路 C 口径 | ✅ 双脚本全绿（check_consistency EXIT=0 + course_sync_scan 0 残留） | `rev64/tools/` |

**"能玩"下一步唯一剩路 = 真机 arm64/32 位 arm GPU 设备验收 + 段 4 场景对象图数据工程**。
流水线已把"逆向→正向→验收→补差异"全部一条命令化，**剩余是"数据工程 + 真机"，非"流程"缺口**。
