# 附录 C：逆向工程化方法论（防回归 / 审计工具 / 增强循环）

> **定位**：unidbg 课程 31 章讲"怎么逆 so"（单点技术），附录 B 讲"so 怎么来的 + 逆源码路线"。
> 本附录讲**"逆向工程不是一次性事件，是持续工程"**——两个会话（R-route 正向开发 + 32 位翻译体渲染攻击）
> 沉淀的核心教训：**"补 stub / 翻译函数"能解"代码缺口"，但解不了"对象图数据缺口"；防回归靠"机械化审计工具"，不靠"手工一个个找"**。
> 跟 31 章 + 附录 B（B.5c 逆源码路线）互补：那些讲"能逆到哪层"，本附录讲"逆的过程中怎么不翻车、怎么防复发"。

---

## C.0 核心教训（先立论，后展开）

**军棋两个会话反复撞的同一面墙 + 破法**：

| 误判（早期） | 实证（后期） | 教训 |
|---|---|---|
| "补齐 stub 就能出帧/画棋盘" | **sp28-SCALE-9** 全量 4245 场景闭包翻译（代码 100% 对齐；★ 早期 §124z-5 时点仅 25%/992，后放大到 4245）+ 渲染器单例构造 + glClear 每帧，slot22（棋盘纹理创建）曾 0 命中；**★ 〇d-3 gate-set 第三路突破（SLOT22-TRIGGER OnUpdate 一次性直调 F_9e2b4，texID 0→1，棋盘纹理已建 MuMu 20209 实证），最终阻塞收敛 = F_23eec 存根挡几何绘制** | **"补 stub"补的是代码，破不了"运行时 C++ 对象图数据"（std::list 节点 next 指针/场景对象 vtable/参数指针）** |
| "frida dump 对象图回填翻译体" | dump .bss 成功（79% 非零），**整段回填翻译体 → GLThread SIGSEGV**（scudo 堆指针跨进程失效） | **frida 能 dump"值"，但带不回"对象图"**（dump 的值里指针指向原版进程 scudo 堆，翻译体没有那片堆） |
| "手工找函数调用链" | 反复手改"补 30 个/2 个/4245 个"变体，撞同一面墙 | **"同类手改模式先机械化成工具再动手"**（tex_pipeline.py 的 dispatchfix/closure/vtable 命令） |
| "每轮手写 adb 走子脚本 / 重打包 / /tmp 构造状态" | 走子坐标公式 + 多轮重试 + 重打包 + 手写 /tmp/*.c 事件验证 反复出现 | **"脚本复用/批量"：走子/事件/全周期/真机全套八件套（①walk_coords ②walkverify ③mumu_walktest ④rebuild_rs_apk ⑤mumu_evtest 事件夹具 ⑥evverify 桌面事件验证（进 12/12 第 12 项）⑦rs_cycle 全周期 ⑧**mumu_fulltest 真机全套批量（★ 加 draw/resign 后 6/6 PASS，含和棋/认输）**）；资产固化 rs168_wav.apk 只换 so** |
| "逆向是单点事件" | 每轮"能玩"缺口闭环后踩下一轮新坑 | **"增强循环方法论"：目标→实证→根因→修复→回归→沉淀→下轮（每轮把'下轮目标+当前诚实边界'写清）** |

**一句话**：**逆向工程的现实瓶颈不是"翻译不出来"（指令语义 100% 对齐早达成），是"对象图数据填充"（运行时 C++ 构造，非代码）+ "防回归"（改一处崩另一处）。破法 = ① 认清"代码缺口 vs 数据缺口"边界 ② 机械化审计工具防回归 ③ 增强循环沉淀经验**。

---

## C.1 "代码缺口 vs 数据缺口"——破对象图卡点的第一性原理

**军棋 §124z 黑屏 / §124zz 卡点的根因分层（决定"能不能靠补代码解决"）**：

| 缺口类型 | 是什么 | 能不能"补代码/stub"解 | 军棋实例 |
|---|---|---|---|
| **代码缺口** | 某函数没翻译/没实现（指令缺失） | ✅ 能（翻译/补 stub/补完整体） | 渲染器单例 `F_9d3d4` 是存根 → 补完整体后**构造成功**（sp28-ROOT） |
| **数据缺口** | 代码在，但**运行时对象图数据未填充**（指针/字段/vtable 是 0/垃圾） | ❌ **不能靠补代码解**（补的是代码，数据要靠"真实 C++ 构造链跑通"或 Frida 喂真值） | 全量 4245 场景闭包翻译后，slot22 曾 0 命中（帧循环只调 slot23 清理）；**★ 〇d-3 gate-set 第三路：绕过 classB[8] 通用闸口，OnUpdate 直调 F_9e2b4 使 texID 离开 0（纹理已建），但几何绘制仍被 F_23eec 存根挡住**（"数据缺口"从"纹理未建"收敛为"绘制链存根"） |
| **环境缺口** | 工具/环境模拟不出（无 GPU/跨进程堆/转译层限制） | ❌ 不能（换真机） | MuMu GLES1 几何出帧受限；frida dump 跨进程 scudo 指针不可回填 |

**"补 stub 破不了对象图"的实证链（会话 2，32 位翻译体）**：
```
sp28-SCALE-6：补 30 个 slot22 最小闭包 → 翻译了但 0 次命中（帧循环不活调 slot22）
sp28-SCALE-7：接线场景入口 0xbe9cc + 全闭包 0xbf19c → 仍 slot22 0 命中（对象图数据未填）
sp28-SCALE-8：手动直调 F_9e2b4（slot22）→ ①主线程触发 SIGSEGV（scudo 堆未就绪）②OnUpdate 触发 dlopen 失败（缺 jqt_mem.h）
sp28-SCALE-9：全量 4245 场景闭包 + F_a4e80 38 环循环阀 → 不崩、出帧、填字段，但 slot22 仍 0 命中（**→ 后续 〇d-3 gate-set 第三路 SLOT22-TRIGGER 已突破：texID 0→1，棋盘纹理已建，最终阻塞收敛到 F_23eec 存根，见下**）
```
**结论（〇d-3 后修正）**：纯"补齐 stub"（30 个 / 全量 4245）无法触发 slot22（帧循环不活调它，瓶颈曾是运行时 C++ 对象图数据）；**★ 但 gate-set 第三路（〇d-2/〇d-3）直接置闸口字节 + OnUpdate 一次性 SLOT22-TRIGGER 直调 F_9e2b4，已使 texID 离开 0（棋盘纹理 glGen+glTexImage2D 已上传 737×751）**——**"触发"瓶颈被绕过，最终阻塞收敛为 F_23eec 存根（vtable[52] dispatch 对象图分歧 SIGSEGV，非"补 stub"可解）+ 真机 GPU 验收**。§124zz-4 红线"规模化数据工程"在"场景对象图全构造"层面仍成立，但"让 slot22 被调一次"已被 gate-set 实证破局。

**破数据缺口的两条路（都到边界了）**：
1. **填字段**（sp28-ROOT）：手工把渲染器单例 `this[76]`(texID)/`this[28]`(像素)/`this[32..36]`(宽高) 填真值 → 让 GL 方法"有数据可读"（**但只到"字段填对+不崩"，到不了"画棋盘"——slot22 仍要场景对象图完整构造**）
2. **frida 喂真值**：dump 原版 .bss 运行时值 → **跨进程 scudo 指针不可回填**（见 C.2）

**★ 第三路：gate-set 直接置位闸口字节（会话 2 最新突破，§〇d-2，2026-11-04）**：
- **思路**：写侧数据缺口（"构造标志没置位"）不一定非要跑通完整 C++ 构造链——**直接 OnCreate 置位闸口字节**（`0x15be90 byte[0x51]=1 byte[0x41]=1 [0x30]=10`）→ 触发下游 `F_76b14→F_76a98→F_782d4（r1==10 通过）→F_7876c（类 B 构造，写 vtable 0x157800）→F_7880c（类 B slot0）` **全链实证**
- **配套 SLOT2 workaround**：F_782d4 的 slot2 dispatch（`F_78890` 翻译缺口）失败 → 不提前返回，继续 L7830e 链 → `[0x3c]=classB` 非 0 → **F_77904/F_77a04 执行**（`[0x3c]!=0` 路径完整走通）
- **工具固化**：`rev64/tools/patch_gate_chain.py`（**幂等注入**全部手改补丁，防 regen_vtbl.py 覆盖丢失；实跑 `vtbl=0 jni=0 exit=0` = 全部已就位，幂等可复现）
- **★ 诚实边界（gate-set 仍未触发 slot22）**：F_77904/F_77a04 调的是**渲染器 vtable 方法**（slot6/31/33/35），**非 slot22**（F_9e2b4 纹理创建）；**★ 〇d-3 后更新：F_9e2b4/F_9e204 经 SLOT22-TRIGGER（OnUpdate 一次性直调，绕过 classB[8] 闸口）已各 1 次命中，texID(this[76]) 0→1（棋盘纹理已建，MuMu 19438/20209 实证，进程 120s+ 稳定）**。gate-set 路径 F_77904/F_77a04 调的渲染器 vtable 方法（slot6/31/33/35）仍非 slot22；classB[8] = slot22 触发链通用闸口（F_78890 slot2 = 其 setter，翻译缺口），SLOT22-TRIGGER 不依赖它直接调
- **★ 认知更新（C.1 数据缺口的细化）**：写侧数据缺口"**部分可破**"——**闸口字节置位（gate-set）能突破"构造标志未置位"这一层**（F_76b14 全链走通），但"**完整 C++ 对象图构造**（std::list 节点 + 类 A/B slot0 调用者 + 纹理上传）仍非 gate-set 能解"。**gate-set 是"数据缺口"的破局手段，不是"代码缺口"**——它填的是运行时标志字节，不是翻译函数

**★ 翻译缺口清单（gate-set 暴露的，§〇d-2 新增，非"几个补丁"）**：
`F_78890`（F_782d4 slot2）/ `F_a4594`（slot31）/ `F_9d5b0`（slot6）/ `F_9de24`（slot35）**全不在 seed_funcs.json**；`slot33=0x159644=0xffffffc4`（RELATIVE=False，静态垃圾指针，非函数）→ `jqt_blx_dispatch` 返回 0 不崩。**补这 4 个翻译缺口需恢复翻译器**（§sp28 红线：规模化翻译工程）**破数据缺口的两条路（都到边界了）**：
1. **填字段**（sp28-ROOT）：手工把渲染器单例 `this[76]`(texID)/`this[28]`(像素)/`this[32..36]`(宽高) 填真值 → 让 GL 方法"有数据可读"（**但只到"字段填对+不崩"，到不了"画棋盘"——slot22 仍要场景对象图完整构造**）
2. **frida 喂真值**：dump 原版 .bss 运行时值 → **跨进程 scudo 指针不可回填**（见 C.2）

**"代码 100% 对齐 ≠ 能跑"的精确含义**（AGENTS.md 红线）：
- 指令语义 1:1（1174/1174 PASS）= **代码层对齐**
- 对象图数据（运行时构造）= **数据层未对齐**（代码全对，但读的对象图是零值）
- **两者是不同层，"代码全对齐"不蕴含"能跑/能玩"**

---

## C.2 Frida dump 路线的真实边界（会话 2 实证，推翻"frida 帮不了"的旧结论）

**Frida 在军棋上的能力 + 边界（★ §124am-2 用户权威纠正：MuMu 用 x86_64 frida-server，非 32 位版）**：

> **★ 路线演进（§124am → §124am-2，详细笔记）**：早期用 **32 位 x86 frida-server** attach MuMu 的 ARM 转译进程（junqi = app_process32）**崩**（agent 注入干扰 houdini，attach+detach 后 2s 进程死；frida spawn 启动 1s 即死）。**用户权威纠正**：MuMu 用 **x86_64 frida-server（17.24.3）** attach OK + 进程稳定（attach 后 2s 不死）。**frida 17 API 变化**：`Memory.readU32` 不存在 → 用 `NativePointer.readU32()`。

| 能力 | 实证 | 边界 |
|---|---|---|
| **attach 转译进程** | x86_64 frida-server（17.24.3）attach MuMu ARM 转译进程 OK，进程稳定；32 位版崩（注入干扰 houdini） | **frida 版本/架构选型是环境敏感点**（§124am-2 纠正） |
| **原版 so 真 dlopen** | 原版 32 位 so 真 dlopen + 场景构造成功（进程 45s 稳定），基址 0xc068000（/proc/maps 实证） | — |
| **dump .bss/.data 值** | `dd /proc/PID/mem` 抓 .bss 28680B（**79% 非零 = 场景对象图运行时构造态**）+ .data 28288B | **能 dump"值"**（含 load_base、RELATIVE 重定位 diff 验证） |
| **dump 对象图（std::list/vector）** | ❌ | **带不回来**——dump 的值里指针指向原版进程 scudo 堆，翻译体进程没有那片堆 |
| **.bss 整段回填翻译体** | ❌ **GLThread SIGSEGV SEGV_MAPERR 0x3a** | **跨进程 scudo 堆指针失效**（.bss 值里 0xcfxxxxxx 是原版进程堆地址，翻译体 `jqt_mem_valid` 真映射区表无此区 → 解引用越界） |

**"unidbg + Frida 一对一对齐"（附录 B B.5c / ch13 13.4c / FRIDA_NOTES §八）的真实边界**：
- **前提**：Frida 真值必须喂给"**能真正 dlopen 原版 so 的同一进程**"（如 redroid 真跑原版）
- **跨进程 dump 值回填翻译体 = 走不通**（scudo 堆指针跨进程失效）
- **Frida 能做的**：①提取 .rodata 明文（已做，1715 字符串）②dump .bss/.data 的"标量值 + load_base"（12 标量可安全回填，15 指针槽需自建）③Jnitrace/Stalker 看调用流（指令级追踪）
- **★ 伪槽认知（§124am 详细笔记新发现，C.2 吸收）**：翻译体注入的 `0x15c0a0` 是**伪单例槽**（原版 capstone 扫描 ldr[pc]+add pc：0x15c0a0 = 0 引用，0x15be80/0x15be88/0x162400 才是原版真实引用 106/140/10 refs）——**翻译体注入的 GOT 槽 ≠ 原版真实槽**，frida 读翻译体的伪槽读的是"翻译体自己的构造状态"（时序不稳定），不是"原版真值"。原版真值要读**原版进程**（基址 0xc068000 + 槽偏移），跨进程仍不可回填（见下）

**Frida 做不到的**：**跨进程回填"对象图"**（指针拓扑带不回来）——这是"模拟路线天花板"的精确边界

**会话 2 的"最大可交付态"**：
- **sp28-SCALE-7 基线 + F_b4238/F_b7e00 完整体 + La4f0c 循环阀**（8.17MB so，1.78 版本号，MuMu 90s 稳定 + 出帧 + 填字段 + slot23 dispatch）
- **画棋盘/可玩**：需**真机 32 位 arm GPU 设备验收 + 场景对象图数据工程**（§124zz-4 红线，非 1 人本轮可闭环）

---

## C.3 机械化审计工具（防回归三件套，会话 2 核心沉淀）

**"同类手改模式必须先机械化成工具再动手"（§124zz-8 红线）——3 套审计工具 + 走子四件套 + 2 条一条龙（rebuild_rs_apk.sh / arm32_cycle.sh）**：

### C.3.1 链接安全审计（`link_audit.py`，打包前必跑）

**防"MuMu dlopen 'failed to link'"（头号根因 = 内部符号悬空）**：
```bash
python3 rev64/tools/link_audit.py audit <so|apk> [--baseline <so|apk>]
# 查：内部符号悬空 / UND 能否被 MuMu ARM 库解析 / 与实证基线的 UND diff / .hash 风格 / APK 完整性 / dispatch 表一致性
# 退出码 0=PASS / 2=FAIL（不过则中止打包）
```
**背景（MuMu 原生桥限制）**：MuMu 32 位 linker（libnb/houdini）解析 ARM 库 UND 符号能力有限，"cannot locate symbol" 不落 logcat（只有 `failed to link`）→ **排查只能离线 diff UND 集合**（`readelf -sW so | awk '$7=="UND"{print $8}'`）。

**8 个坑里的链接类（坑 1）**：内部 `F_xxx` 悬空（fns 表里没函数体定义）→ so 里成 `NOTYPE GLOBAL DEFAULT UND` → linker 无法解析 → **整体 fail to link**（v178e1~e3 实证）。**每次改表/重建必查**。

### C.3.2 JNI 反射审计（`jni_method_audit.py`，防坑 2/3 复发）

```bash
python3 rev64/tools/jni_method_audit.py <file.c>
# 扫所有 GetMethodID 调用，检查 8 行内有无 ExceptionCheck/ExceptionClear，真缺口列行号（exit=2）
# 实证：jni_bridge.c 0 缺口（exit=0）；故意缺口被抓到（exit=2）
```
**背景（坑 2/3/6/7，JNI 反射 4 类坑）**：
| 坑 | 现象 | 修法 |
|---|---|---|
| **GetMethodID 重载猜错**（坑 6） | `setDataSource(File)` 不存在（MediaPlayer 只有 String/FD/AFD 1 参重载）→ NoSuchMethodError 累积 → 后续 abort | 用 `javap`/`dumpsys` 验证实际重载，勿猜；**每个 GetMethodID 后 `if(ExceptionCheck) ExceptionClear`** |
| **local ref 悬空**（坑 3） | `act_cls` DeleteLocalRef 后再 GetMethodID → SIGSEGV 0x70 | 类/对象 ref 用完即删，删后不得再用；需复用则重新获取 |
| **pending exception 未清**（坑 2） | GetMethodID 失败未 ExceptionClear → ART abort | `jni_method_audit.py` 机械化查 |
| **AAsset 锁竞争**（坑 7） | 主线程 OnCreate 与渲染线程竞争 libandroid 全局锁 → SIGSEGV SI_KERNEL | Java 反射读 assets 写 getFilesDir/（单线程，无竞争）+ AAsset 函数指针避免 AAssetManager_fromJava 等（设备转译层不导出 → 直接 UND fail to link） |

**规则**：**每次新增/修改 JNI 反射代码后必跑 `jni_method_audit.py`**（同 `so_link_check.py` 的 UND 审计）。

### C.3.3 纹理/GL 机械化分析（`tex_pipeline.py`，防"手工找 vtable/dispatch"）

```bash
python3 rev64/tools/tex_pipeline.py <子命令>
# vtable22/vtable23（dispatch 搜索）/ funcs（常量池）/ plt（ARM .plt→符号 214 条）
# glcall（寄存器跟踪→GL 实参）/ texseq（纹理 4 函数原版 GL 序列一次出全）
# vtable（RELATIVE 重定位解析 vtable）/ findvtable/texclass/findctor/funcat/stores（类图/字段写入点分析）
# undiff / 机械化修复：vtabswrite / dispatchfix / i2f [--fix] / vtwrite <0x15xxxx>
```
**背景（§124zz 系列手改 4 类模式已固化为工具，下次直接跑，不再逐个手写补丁）**：
| 工具命令 | 干的活 | 防什么坑 |
|---|---|---|
| **`vtabswrite`** | 扫 `src_jni_translated/*.c` 里 `MEM(reg+0)=0x15xxxx` 未包 `JQT_ABS_PTR` 的 vtable 写入点 | vtable 写入必须存**绝对地址**（RELATIVE 重定位后）；0 处=正确 |
| **`dispatchfix [min] [max]`** | 列 .text 全部 vtable dispatch 点（`ldr rX,[reg,#disp]; blx rX`），按 disp 范围过滤（纹理类=0x58/0x5c） | 找"帧循环调哪个 slot"（slot22 vs slot23）机械化，非手工 |
| **`i2f [--fix]`** | 列/批量修 `blx 0x232c8`（`__aeabi_i2f`）的翻译体存根 | 40 处存根 → 真实 i2f 实现（v178e14 全量修完） |
| **`vtwrite <0x15xxxx>`** | 找写某 vtable 的构造器（`ldr rX,[pc]→add rX,pc→str rX,[this]` RPI 三连），查该 vtable 是否在 RELATIVE 列表（True→必须写 ABS） | vtable 构造器定位 + 绝对地址判定 |

**§124zz 系列关键认知（工具化后沉淀）**：
- **vtable 槽值 = 绝对地址，`JQT_ABS_PTR` 纯透传**（§124zz-7）：RELATIVE 重定位后 `.data.rel.ro` 槽值 = 基址+0x9e0d4（实指针），`JQT_ABS_PTR` 对 >=MAP_HI 的实指针透传；旧"强制 0x9e2e0"补丁假设槽值是 shadow 偏移 → 把正确绝对地址误改 → 调错函数（**v178e13 已撤掉全部 10 处强制补丁**，只留纯 ABS 透传）
- **"glClear 类对象"是幻影（§124zz-3 推翻）**：`0x1595f4` 不是 vtable 基址，而是纹理类 vtable `0x1595c0` 的 slot[13]（值 0x9d459 → 函数体 0x9d458 = glClearColor+glClear+glBlendFuncSeparate）→ **清屏方法就在已构造对象身上，不需要另造"glClear 类对象"**（§124au/§124z 为此追的幻影不要再追）
- **RPI 公式修正（§0b 推翻"4078 墙"）**：池地址 `pool = ((addr+4) & ~3) + disp`（**objdump 注解实证**：0xbe9d8→0xbec60、0x23426→0x23434、0x45b6a→0x45b84 全对）；旧公式 `(addr&~2)+disp`（未加 4，Thumb PC=addr+4）→ 差 4 字节 → 4078 条超边界里 **3767 条是假阳性**；**修正后真缺口 = 57 个 .bss 槽（15 scudo 自建 + 12 标量可回填）**——量级缩小 2 个数量级，可攻克

**★ 完整工具谱系（按职责分组，防"漏网工具"——5 轮审计逐一核实经验库 39 个工具）**：

| 职责 | 工具（会话） | 干什么 |
|---|---|---|
| **链接/反射审计** | `link_audit.py`（2）/ `jni_method_audit.py`（1）/ `so_link_check.py`（1）/ `src_rs_check.py`（1） | 打包前 UND 悬空/ExceptionClear/红线审计（1/2 会话对称） |
| **GL/纹理逆向** | `tex_pipeline.py`（2）+ 子命令（vtable/dispatchfix/texseq/closure/scenechain/slot22chain/...） | 机械追 vtable/dispatch/调用闭包 |
| **RPI/.bss 缺口** | `rpi_audit.py`（2，RPI 公式 `((addr+4)&~3)+disp` 审计）+ `bss_backfill.py`（2，57 槽分类+回填表） | 读侧缺口量化（C.1 读/写侧调和的工具化） |
| **翻译体生成** | `gen_shadow.py`/`gen_onevent_full.py`/`regen_vtbl.py`/`gen_arm_thunks.py`/`gen_jni_calls.py`/`wire_render_jni.py`（2） | 全量/增量翻译体生成（生成→wire→插桩顺序红线 §124z） |
| **sp 回退族（防误改基线）** | `revert_to_sp27.sh`/`revert_to_sp28root.sh`（2） | 一键回退到已实证稳定基线（sp28 崩溃→回 sp27 先例），**"放大后崩溃"必先回退再分析** |
| **frida 工作流** | `frida_utils.py`/`frida_run.py`/`proc_read_slots.py`/`frida_dump_objects.py`（1+frida-dump） | attach/读槽/主动构造/原版真值 dump（x86_64 frida-server，§124am-2） |
| **走子/事件/全周期（七件套）** | `walk_coords.py`/`walkverify.c`/`mumu_walktest.sh`/`rebuild_rs_apk.sh`/`mumu_evtest.sh`/`evverify.c`/`rs_cycle.sh`（1） | 桌面预验证→真机→重打包→事件夹具→全周期（C.3.4） |
| **翻译体全周期** | `arm32_cycle.sh`/`mumu_verify.sh`/`mumu_swap_so.sh`（2） | 编译→pack(含 link_audit 门禁)→装机→验证 |
| **截图/资源** | `shot_check.py`（2，截图分类 warm_brown/black/board）+ `spak_unpack.py`/`lay_extract.py`（1，SPAK/布局解包） | 屏幕态判读 + 资源固化 |
| **辅助** | `callchain_stats.py`（2，合并边界调用链统计）/ `disasm_rs.py`（1）/ `render_map.py`/`render_patch.py`（2） | 统计/反汇编/渲染补丁 |

**★ 漏网工具教训（5 轮审计暴露）**：早期 C.3 只列"3 套审计工具 + 走子四件套"，**漏了 7+ 个**（rpi_audit/bss_backfill/gen_onevent_full/frida 族/mumu_swap_so/回退族）。**根因 = 工具谱系只按"本轮新增"增量补，没按"全量清单"对齐**——**工具谱系必须全量盘点（本次 39 工具已列全），不靠"想到哪个补哪个"**。

**★ 本轮新增工具（会话 2 最新沉淀，2026-11-04 同步）**：
| 工具 | 干什么 | 沉淀点 |
|---|---|---|
| **`slot22chain`** | slot22 触发链一条命令机械化推导（dispatch 点→所属函数→vtable 归属→上游链） | 防"手工追 slot22 触发链"（§sp28-SCALE-6/7 反复手追的固化） |
| **`seed_funcs.json`（资产复用）** | 2157 函数起点表，`funcat`/`slot22chain`/`closure`/`scenechain` 自动回退用它 | **防 /tmp 丢失**：旧 `/tmp/opencode/seed_v2.json` 被清后丢失，seed 表固化在 `rev64/dist/`，**不依赖 /tmp 一次性文件** |
| **`mumu_verify.sh` PKG 自动推导** | `aapt dump badging` 从 APK 推导包名，不硬编码 | **防"验错包"**：旧版硬编码 `PKG=com.cnvcs.junqi` → 装 junqi32 后启动的是**原版 1.52**（验证结果全错）。**任何装机验证必须从 APK 推导包名，不得硬编码** |

**★ 审计工具自身的"对抗审计"（N1 门禁固化的副产品）**：`link_audit.py` 旧版对**假文件/非 ELF/无 .dynsym** 会空转 readelf → 0 UND → **误判 PASS**（负向验证实证：`audit /nonexistent.so` exit 0）。补了"文件存在 + readelf -h 识别 ELF + 有 .dynsym"三重校验 → 假文件/普通 ELF 可执行（如 `/bin/ls`）现在 FAIL exit 2。**教训：审计工具本身也要做"对抗测试"（喂假输入看会不会误放行），不能只测"真 so PASS"的正向路径**——门禁若误放行比没门禁更危险。N1 门禁已落 `pack_arm32.sh`（打包第 2.5 步，FAIL 则中止，`NO_AUDIT=1` 逃生口）：

```bash
# pack_arm32.sh 第 2.5 步（N1 固化）
if [ -z "$NO_AUDIT" ]; then
  python3 rev64/tools/link_audit.py audit "$SO" || { echo "✗ 链接审计 FAIL → 中止打包"; exit 1; }
fi
```

### C.3.4 走子/事件/全周期/真机全套八件套（脚本复用/批量，会话 1 沉淀，防"每轮手写 adb 脚本"）

> **★ 真实长路径走子闭环（会话 1 最新沉淀，12/12 回归 + walkverify --longrun + mumu_walktest --check-turn）**：蓝走→红 AI 应着→蓝走，**turn 交替持续**（3 完整回合真机 + 5 回合桌面）。真机日志：EAT（25吃30）→AI（31→36）→BOMB（27同级32）→AI（33→31）→MOVE（28→33）→AI（31→32），每步 sfx 正确（eat/bomb/move），进程稳定。**工具升级：①`walkverify --longrun N`（桌面自动 N 回合，免手写序列受红 AI 局面影响）②`mumu_walktest --check-turn`（真机验证 turn 切回蓝方≥SEQ 步数）③回归 11/11 → 12/12（[11/12] 长路径 + [12/12] 事件 6/6，编号统一）**。**诚实边界：触摸 flaky 下走子多轮重试（mumu_walktest 兜底）；桌面 5 回合为蓝/红贪心 AI（非真实人类对手）**。

**"验证走子先桌面预验证 → 再真机单点 → 真机全套批量 → 换 so 全周期"的固化八件套（①-④ 走子/重打包 + ⑤ 事件夹具 + ⑥ 桌面事件验证 + ⑦ 全周期 + ⑧ 真机全套批量，"本轮最重复手写点"= 重打包/手写 /tmp 构造状态/手写真机组合验证，已固化）**：

| 工具 | 干什么 | 用法 |
|---|---|---|
| **① `walk_coords.py`** | cell→tap 坐标（公式固化，`--seq "25,30"` 批量） | `python3 rev64/tools/walk_coords.py --seq "25,30 27,32"`（验证锚点：cell25/30/28/33 精确匹配 353,927 / 353,805 / 496,927 / 496,805） |
| **② `walkverify.c`（`make walkverify`）** | **桌面预验证**走子序列（内嵌红方 AI 贪心，蓝走→红应着→蓝走，打印事件）；**★ `--longrun N` 自动蓝走→红应着 N 回合（验证 turn 交替持续，免手写序列受红 AI 局面影响）** | `make walkverify SEQ="25,30 27,32"`（实证 3/3 步合法，exit=0）/ `make walkverify LONG="5"`（**--longrun 5 回合 turn 交替持续**，实证 5/5） |
| **★ `mumu_walktest --check-turn`** | 真机验证"turn 切回蓝方"≥ SEQ 步数（长路径走子真机判据） | `bash rev64/tools/mumu_walktest.sh --restart --seq "25,30 27,32" --check-turn`（真机 turn 交替持续） |
| **③ `mumu_walktest.sh`** | **真机走子一条龙**（重启+菜单→每步选 cell→走 cell 多轮重试→事件验证→日志+进程） | `bash rev64/tools/mumu_walktest.sh --restart --seq "25,30 27,32" --expect Bomb`（触摸 flaky 由脚本多轮重试兜底，不用手写 for 循环） |
| **④ `rebuild_rs_apk.sh`** | **重打包+签名+装机一条龙**（链接审计→换 so→STORE→zipalign→v1+v2 签名→装→so 哈希对账→启动） | `bash rev64/tools/rebuild_rs_apk.sh --start`（**换 so 后必跑此脚本，勿手写 zip 重打**——固化前每轮手写 10+ 次；实证 so 哈希设备=本地一致） |
| **⑤ `mumu_evtest.sh`（事件夹具触发）** | FLAG/MINE 事件真机验证（setprop 触发 OnCreate 自动走子 + 重启 + 日志 + 进程判据） | `bash rev64/tools/mumu_evtest.sh flag\|mine`（触摸 MuMu flaky 不达 native → FLAG/MINE 走 setprop 夹具路径；mine 复验 PASS） |
| **⑥ `evverify.c`（`make evverify`，桌面事件验证）** | **替代每轮手写 /tmp/*.c 构造状态**：`--all` 跑 **47 断言**（5 事件 EAT/BOMB/MOVE/FLAG/MINE + **UNDO 悔棋 + DRAW 和棋 + RESIGN 认输 + STALEMATE 困毙 + **★ 会话 1 扩 UI 对齐验证 43 断言（login/about/bright/chat/gameinfo/profile/room/setentry 等按钮几何 Δ=0 + replay 回退 + rules/imgset/create/ng/join/drawdetail 状态机 + **ui118 版本更新面板 / exitconfirm 退出确认 / resignconfirm 认输确认 / regretconfirm 悔棋确认**（几何简化近似，诚实边界标注））**）**，`--undo`/`--draw`/`--resign` 单跑夹具，`--lay` 打印默认布局 60 格 | `make evverify MODE=--all`（实证 **47/47 全对**：5 事件 eat ev=1/bomb ev=2/move ev=0/flag ev=3/mine ev=5 + undo 悔棋 + draw 和棋 winner=2 + resign 认输 winner=1 + stalemate 困毙 winner=1（无棋可走判负）+ **★ UI 对齐 43 断言（login 6 按钮几何 Δ=0 + about/bright/gameinfo/chat 弹窗 + replay 回退 3 路径 + rules 状态机 + **ui118 版本更新面板 + exit/resign/regretconfirm 3 个确认弹窗（几何简化近似，诚实边界）**）**）；**已进 12/12 回归第 12 项（判据"47/47 全对"，★ 会话 1 扩 UI 对齐验证后 8→44→**47**）** |
| **⑦ `rs_cycle.sh`（全周期一条龙）** | 会话 1 侧的"会话 2 对称件"补全：make so + **12/12 回归** + rebuild_rs_apk.sh --start 装机启动 | `bash rev64/tools/rs_cycle.sh`（实证：编译 92840B + 11/11 PASS + so 哈希对账一致 + 进程 13222）。**换 so 后全流程跑此脚本（对称会话 2 的 arm32_cycle.sh）** |
| **⑧ `mumu_fulltest.sh`（真机全套验证，最高价值批量）** | **走子 EAT + FLAG + MINE + 游戏内重开 + 和棋 + 认输 + 困毙 + UI 关闭/弹窗音效 一次跑完**（组合验证，防"每轮手写 走子→事件→重开 组合"）；**走子部分含 `--check-turn`（turn 交替判据）** | `bash rev64/tools/mumu_fulltest.sh`（**★ 加 UI 关闭/弹窗音效后 9/9 PASS**（走子 EAT + FLAG + MINE + 重开 + 和棋 + 认输 + 困毙 + **UI 关闭/弹窗音效**，19321+ 复验；`--skip-walk` 则 8/8 只跑事件）；演进：早期 4/4（走子+FLAG+MINE+重开，14115~14426）→ 加 --check-turn 后 18174~18491 → 加 draw/resign 后 6/6（18636）→ 加 stalemate 后 7/7（19321）→ **加 UI 关闭/弹窗音效后 9/9** |

**★ 七件套 → 八件套**：①walk_coords ②walkverify ③mumu_walktest ④rebuild_rs_apk ⑤mumu_evtest ⑥evverify ⑦rs_cycle ⑧**mumu_fulltest（真机全套批量）**——"桌面预验证 → 真机单点 → 真机全套批量"三级验证链完整（⑥ 桌面 **47 断言**（5 事件 + undo/draw/resign/stalemate + **★ 会话 1 扩 UI 对齐验证 43 断言**）/ ③⑤ 真机单点 / ⑧ 真机 **9 项**一次跑完）。

![军棋真机验证：八件套三级验证链](assets/c5_eight_tools_verify_chain.png)
> 🎬 动态版：[`videos/c5_eight_tools_verify_chain.mp4`](videos/c5_eight_tools_verify_chain.mp4)（10s 三级验证链动画）

**★ 资产复用强化（本轮）：`rebuild_rs_apk.sh` 推 spak_res 到设备文件系统优先路径**：每次装机后把 APK `assets/spak_res/*` 推到 `/data/local/tmp/spak_res/`（幂等，设备端 2 个 .bin = datah_188 棋盘纹理 + datah_183 棋子图集）——渲染 `rs_read_png` **文件系统优先**（免 AAsset 锁竞争，坑 7 的工程化落地），缺失才 fallback assets。`mumu_evtest.sh` `--dev` 解析修正（原只解析第一个参数，fulltest 传 `flag --dev IP` 不匹配的 bug）。

**"资产复用"配套（防"每轮重解包/重推资产"）**：14 WAV（assets/jq_sfx/）+ SPAK 解包产物（datah_188 棋盘纹理 + datah_183 棋子图集）**已固化在 rs168_wav.apk**；`rebuild_rs_apk.sh` 只换 so、保留全部资产。

**"回归升级 9/9 → 10/10 → 11/11 → 12/12"**：`src_rs_regress.sh` 新增 `[10/11]` 走子预验证（`make walkverify`，判据"3/3 步合法"）+ `[12/12]` 桌面事件验证（`make evverify MODE=--all`，判据"**47/47 全对**"（5 事件 + undo/draw/resign/stalemate + **★ 会话 1 扩 UI 对齐验证 43 断言**，★ 判据跟着 evverify 工具升级同步——旧 5/5、6/6、7/7、8/8、44/44 判据会分别假 PASS，N25 判据同步第 8 次执行），防事件夹具 + UI 对齐回归；**[11/12] 真实长路径走子（`walkverify --longrun 5`，判据"5/5 回合 turn 交替 持续"）**。12/12 全 PASS 实证（2026-11-04 EXIT=0）。

**★ walkverify 的双重职责（I3 审计区分，防台账串线）**：`walkverify.c` 既是**走子四件套的第 ② 件**（手工/批量走子验证的"桌面预验证"入口——`make walkverify SEQ=...` 验证任意坐标序列），又是 **12/12 回归的第 10 项**（自动化防回归里的固定序列 25,30 / 27,32 / 28,33）。**同一工具两个用途**：手工验证走"七件套第 ② 件"，防回归走"11/11 第 10 项"——职责不重叠，是"工具复用进回归"。

**★ 会话 2 的对称件（D5 统一"循环入口"概念）**：会话 1 = `rs_cycle.sh`（**全周期一条龙**：make so + 12/12 回归 + `rebuild_rs_apk.sh --start` 装机启动，实证进程 13222）+ `mumu_walktest.sh`（走子验证）；会话 2 = `arm32_cycle.sh`（翻译体 so 循环：编译→`pack_arm32.sh` 打包（含 link_audit 门禁）→`mumu_verify.sh` 验证，`--no-build/--no-verify/--wait N` 可拆段）。**两条路线各有一条"编译→回归→打包→装机→验证"全周期一条龙（`rs_cycle.sh` / `arm32_cycle.sh`），入口概念已统一（D5 落地），工具本身不合并（正向 64 位 / 翻译体 32 位，依赖不同）**。

**★ 审计自动化覆盖对称（第三类审计实证，2026-11-04 核实 + 轮次 2 闭环 N1）**：
- **会话 1（R-route）**：`src_rs_regress.sh` 第 5 项 `src_rs_check.py`（含 `jni_method_audit.py` 内嵌）+ 第 9 项 `so_link_check.py`——**审计工具已进 12/12 回归每轮跑**（实证 PASS）
- **会话 2（翻译体）**：`pack_arm32.sh` 第 2.5 步**已内嵌 `link_audit.py audit "$SO"`**（FAIL 中止打包，`NO_AUDIT=1` 逃生口；且修了 `link_audit.py` 自身假文件误放行 bug——三重校验：文件存在 + readelf -h 识别 ELF + 有 .dynsym）——**★ N1 已闭环，两路线审计自动化覆盖现已对称**
- **全周期对称件**：会话 1 `rs_cycle.sh`（make so + 12/12 回归 + rebuild_rs_apk.sh --start）对称会话 2 `arm32_cycle.sh`（编译 + pack_arm32.sh 含门禁 + mumu_verify.sh）——D5"统一循环入口"已两边落地

**坐标公式（固化）**：view=900×1468，native_y=screen_y-132；`x = 0.366×900 + (col+0.5)×47.7`，row 中心 `native_y = 1468 − (row+0.5)×122.33`。

---

## C.4 增强循环方法论（每轮闭环后固化，防"反复踩同一坑"）

**会话 1/2 共用的"能玩"缺口闭环流程（每轮结束把"下轮目标 + 当前诚实边界"写清，下轮从经验库直接续）**：

```
① 目标（音效/AI/走子/出帧...）
   ↓
② 实证（MuMu 日志 / 桌面回归 / 截图分类 shot_check.py classify）
   ↓
③ 根因（12 坑归类：JNI 反射 2/3/6/7 / C 语义契约 4/5/8/10/11 / 链接 1 / 一致工程 9/9b；★ 适用性见 C.4 表格"适用路线"列——坑 4/5/7/2/3/10/11/9/9b 通用，坑 8 R-route 专有，坑 1 两路线各一实例）
   ↓
④ 修复（补定义 / 正确重载+ExceptionClear / int 返回契约 / sc 表...）
   ↓
⑤ 回归（12/12 全回归 PASS + so 哈希对账，含走子预验证第 10 项）
   ↓
⑥ 沉淀（AGENTS.md src_rs/翻译体章节 + 本附录 C）
   ↓
⑦ 下轮目标（loop 入口，写清当前诚实边界）
```

**根因 3 类（8 坑归类，会话 1 沉淀）——★ 适用性标注（E2 审计：区分"通用"与"路线专有"，防经验库条目串线）**：

| 类 | 坑 | **适用路线** | 防复发 |
|---|---|---|---|
| **JNI 反射** | 坑 2（GetMethodID 重载猜错）/ 坑 3（local ref 悬空）/ 坑 6（MediaPlayer 构造）/ 坑 7（AAsset 锁） | **通用（凡 JNI 反射 + 多线程 so 都适用）**：R-route `jni_bridge.c` 实证，但 32 位翻译体只要动 `FindClass/GetMethodID` 同样踩（§124zz 翻译体 JNI 适配层同构）；坑 7 AAsset 锁竞争在"主线程 vs 渲染线程双读 assets"时通用 | `jni_method_audit.py` + `jni_util.h` 封装宏（自动 ExceptionClear）+ javap 验证重载 |
| **C 语义契约** | 坑 4（enum 符号性优化：`(jq_event)-1` 被优化成越界）/ 坑 5（`if(fn())` 把 0=成功当 false）/ 坑 8（AI 贪心送死 sc 阈值） | **坑 4/5 = 通用 C 语义**（凡写"返回值契约"的 C 代码都适用，与路线无关）；**坑 8 = R-route 专有**（AI 贪心 sc 表是 src_rs 的蓝/红对弈设计，翻译体无此 AI 层） | **规则引擎返回值契约一律用 `int`（-1=非法），禁用 enum**；`if(fn())` 只用于布尔/真值函数；sc 表固化（吃子 700-1000 > 同级/炸弹 > 空移 2 > 送死 1） |
| **链接** | 坑 1（内部 `F_xxx` 悬空 → UND → fail to link） | **路线分化**：R-route 表现为"JNI 适配层 `jq_get_jvm` 定义丢失"；**32 位翻译体表现为"翻译体 fns 表里的 `F_xxx` 无函数体 → 悬空 UND"（§124zz 头号根因，v178e1~e3 实证）**——**同一类坑，两条路线各有实例，审计工具通用（`link_audit.py` 对两边 so 都跑）** | `link_audit.py` + `so_link_check.py` UND 审计（每次重建 so 必跑） |

> 

**★ 翻译体 GL 管线逐条核对方法论（§124zz-1 详细笔记沉淀，第四层审计 2026-11-04）**：

纹理管线 4 函数（F_9e0d4 删除 / F_9e204 创建 / F_9e0f0 上传 / F_9e2b4 更新，vtable 0x1595c0 slot22/23）的"翻译体 vs 原版"逐条核对，`tex_pipeline.py texseq` 一次出全部原版 GL 序列，结论：

| 函数 | 原版 GL 序列 | 翻译体 | 核对结论 |
|---|---|---|---|
| **F_9e0d4 删除** | glDeleteTextures(1,&this[0x4c]); this[0x4c]=0 | 同 | **完全一致** |
| **F_9e204 创建** | glGenTextures→glBindTexture→4×glTexParameterf(CLAMP_TO_EDGE×2+LINEAR×2)→glPixelStorei(4)→glTexImage2D | 实参值全同（**仅调用顺序不同：原版 WRAP_S,WRAP_T,MAG,MIN，翻译体 MAG,MIN,WRAP_S,WRAP_T——GL 各自独立设状态，顺序无影响**） | **完全一致** |
| **F_9e0f0 上传** | _Znaj→**子矩形构造 + 逐行 memcpy**→glBindTexture→glPixelStorei→glTexSubImage2D→_ZdaPv | 整图一次 memcpy + 全图 glTexSubImage2D | **结构一致，局部脏区更新时不等价**（已知简化，非 bug 未修——对象图未通无法验证） |
| **F_9e2b4 更新** | this[76]==0→bl F_9e204(创建)→回跳脏区检查；[0x28]!=[0x30]→bl F_9e0f0(上传)→清脏区 4 整数 | 同（含创建后回落脏区检查） | **完全一致** |

**★ 方法论沉淀（通用，C.3.3 工具谱系补强）**：
- **"逐条核对 GL 序列"是"指令语义对齐"的 GL 特化**：不靠人工反汇编逐个比，靠 `tex_pipeline.py texseq`（PLT 解析 214 条 + 寄存器跟踪 → 打印 GL 实参）一次出全，再对照翻译体。这是 §124zz-1"用户批评反复写脚本没固化"后的工具化——**GL 调用序列核对是"行为级等价"验证的又一类判据（继 C.4 合并边界调用链统计之后）**
- **时序坑（寄存器跟踪必须做）**：原版 F_9e204 0x9e22a 的 `adds r2,r7,#0` 在 0x9e22c 的 `ldr r7` **之前** → 第 2 个 glTexParameterf 用的是**旧** r7；不跟踪寄存器会读错实参
- **工具 GL 枚举表纠错**：MAG_FILTER=0x2800 / MIN_FILTER=0x2801 / WRAP_S=0x2802 / WRAP_T=0x2803（NDK `GLES2/gl2.h` 为准，工具原先写反过）

**★ 字段语义表（经原版构造器反汇编确认，C.1 数据缺口的"字段级"细化）**：
| 纹理对象偏移 | 含义 | 空哨兵 |
|---|---|---|
| this[0] | vtable = 0x1595c0 | — |
| this[0x1c](28) | 像素数据指针 (RGBA) | **★ -1 (0xffffffff)，不是 0**（构造器 `rsbs r3,r3,#0` → -1） |
| this[0x20](32) / this[0x24](36) | 宽 / 高 | — |
| this[0x28..0x34] | 脏区 4 整数（上传后清 0） | — |
| this[0x4c](76) | GL 纹理 ID | — |

**★ 修掉的真实野指针 bug（§124zz-2，"对象图分歧"的工程化防御）**：
翻译体 F_9e0f0/F_9e204 的"无数据"防御只挡 `data==0`，**漏了哨兵 -1** → `memcpy(buf,(void*)0xffffffff,w*h*4)` 野指针读 SIGSEGV。**修法 = 同时挡 `0` 与 `0xffffffff`**。原版**不检查**（靠对象图保证图像已加载）；**翻译体对象图分歧 → 必须加防御**——这是 C.1"代码全对但数据缺口"的工程化应对（翻译体补防御 ≠ 原版行为，是"破对象图卡点"的临时手段）。

**★ 第四层审计新发现（§124zz-2 顺带发现，课程此前未覆盖）**：**原版渲染走 GLES2 着色器路径，不是 GLES1 固定管线**——draw 函数（原版 0x9e760）实测 GL 序列 `glVertexAttribPointer ×3（2/4/2 分量）→ glUniform1i ×2 → glBindTexture → glDrawArrays(mode=4=GL_TRIANGLE_STRIP)` ⇒ 纹理对象**自带绘制方法**（不只是纹理容器）；翻译体 gl_bridge 强制路径只做 glClear+eglSwapBuffers（无 shader/无几何）→ **"只出清屏色"的直接原因**。
- **★ 课程 B.2/附录 A 的口径修正**：附录 A 讲"军棋 2D 棋盘不用 shader"是**简化表述，不精确**——原版军棋 draw 路径实测**有** `glVertexAttribPointer`+`glDrawArrays(GL_TRIANGLE_STRIP)`（GLES2 几何 + 纹理自带绘制方法），只是"不用复杂 3D shader/光照"。**"不用 shader" ≠ "不用顶点属性/几何绘制"——精确表述 = 原版走 GLES2 顶点属性 + 纹理对象绘制方法路径，非纯 GLES1 立即模式**
- 诚实边界：纹理数据加载链（.pak→SPAK 解密→zlib→写 this[0x1c]）仍未逆向（入口 slot[42]=F_9fc9c 取资源，后续解码依赖未构造对象图）；**§124zz-2/3/4 全部改动未在 MuMu 实机复验（宿主离线）**

**★ 回归判据方法论（§124z-5 详细笔记沉淀，轮次 2 读侧审计补强 C.1/C.4）**：

![调用链对齐统计：合并边界 4 层口径](assets/c4_merged_boundary_stats.png)
> 🎬 动态版：[`videos/c4_merged_boundary_stats.mp4`](videos/c4_merged_boundary_stats.mp4)（10s 金字塔 4 层高亮动画）

军棋翻译体的"还有多少调用链没补齐"统计，**正确口径 = 合并边界**（`bnd = sorted(set(seed) | full | stub)`），不是"seed_v2.json 单边界"。详细笔记（`REVERSE_NOTES_04_shadow_r6.md` §124z-5）实证：

| 统计项 | 数值 | 说明 |
|---|---|---|
| 完整翻译（命名参数 `_r0`） | 1334 | 函数体有真实 C 代码 |
| 存根（匿名参数 `return 0`） | 296 | 函数体是 `uint32_t,uint32_t,...` 匿名参数 |
| 总定义 | 1630 | 合并边界 |
| 已翻译函数内 bl/blx 调用边 | 3698 条 | |
| 目标完整 | 3604（**97.5%**） | 调用链缺口极小 |
| 目标存根 | 87（2.4%） | |
| 目标缺失 | 7（0.2%）= 仅 5 个函数 | `0x33fa4/0x33fb4/0x34304/0xc8eec/0x10320c` |

**★ 教训（通用回归方法论）**：
- **函数边界必须合并**：seed 表只覆盖部分函数边界（1334 完整里仅 907 在 seed），其余 427 的调用边被漏算 → 低估 10%+。**正确 = 把每个实际定义都当边界**
- **存根 vs 完整靠参数命名区分**：匿名参数（`uint32_t,uint32_t,...`）= 存根；命名参数（`uint32_t _r0,...`）= 完整翻译。用 `return 0` 匹配会漏（存根格式是匿名参数，参数名不叫 `_r0`）
- **"调用链缺口小" ≠ "能跑"**：渲染关键路径直接调用缺口 = 0（100% 对齐），但**黑屏根因是对象图（vtable 未构造），不是缺函数**——再次印证 C.1"代码缺口 vs 数据缺口"（代码全对齐 ≠ 数据填充）
- **统计脚本可复现**：`/tmp/opencode/callchain_stats.py`（读编译源 glob + seed + objdump，合并边界，四层统计）

**★ 量化剩余工程量（详细笔记末尾，会话 2 终局反思一的对齐）**：
`tex_pipeline.py closure 0x242f4`（场景初始化闭包）→ 3969 函数，完整 992 / 完全没有 2977（**覆盖 ≈25%**）。**"清屏色出帧 → 棋盘画出"缺的不是几个函数，是场景构造链 + 其下游约 3000 个函数 = 规模化翻译工程**（C.1 的"数据缺口"在代码维度的量化对应——即使全翻译了 3000 个，仍要运行时 C++ 构造链填数据）。

**★ 25% 与 4245 不是矛盾，是"不同时点"（N17 调和，防"6 层口径漂移"）**：
- **25%（992/3969）= §124z-5 时点**（sp27 前）：当时只翻译了**渲染关键路径**（F_242f4 场景初始化闭包），覆盖引擎调用图 25%
- **4245 = sp28-SCALE-9 时点**（后续放大）：把场景构造入口 `F_242f4` + 全量场景闭包 `0xbe9cc` 喂进 `gen_vtbl.py`（BFS 扩到 4500），**生成 4245 完整函数体**（21MB jqt_vtbl.c，5478 FUNC 符号）
- **演进关系**：25%（992）→ 放大场景闭包 → **4245 全量**（覆盖 99%+，只剩 STL 容器边界函数因"随机输入必崩"归 SKIP）。**"4245 全量翻译"是 25% 的放大版**，两者都"代码对齐"但都"破不了对象图数据"（slot22 曾 0 命中；**★ 〇d-3 后 gate-set 第三路绕过 classB[8] 闸口直调 F_9e2b4，texID 已离开 0**——"触发"被破局，"画出棋盘"仍卡 F_23eec 存根 + 真机 GPU）
- **诚实口径**：C.0/C.1/INDEX 的"全量 4245 场景闭包已翻译"= sp28-SCALE-9 终态；C.4 的"25%"= §124z-5 早期量化。**引用时点不同，别混读**

**★ 本轮新增坑（会话 1 最新沉淀，C.4 8 坑 → 补 2 个）：

| 坑 | 现象 | 通用性 | 防复发 |
|---|---|---|---|
| **坑 9（统一入口缺失 → "改一处漏 N 处"）** | win/lose 音效三处散点调 `sfx_play`（OnTouch 锁外 / AI 锁内 / OnCreate flagtest 锁内），改逻辑易漏 | **通用**（凡"同一逻辑多处调用"都应统一入口） | **新事件音效必须走 `sfx_for_move`/`sfx_game_over` 统一入口，勿散点 `sfx_play`** |
| **坑 9b（★ 统一入口模式的第二实例：重开逻辑收敛 `jq_restart_game`）** | 6 处重开逻辑（restartgame property / 菜单 action 0/4 / 点棋盘开局 / 对局结束"再来一局" / 左上角悔棋重开 / 对局态点棋盘自动开局）原为**重复代码块**（load_layout + 明棋 + 清 undo + stage=PLAY + 蓝先手 + 清胜负），改重开规则要改 6 处 | **通用**（"同一逻辑多处重复代码"= 统一入口缺失，与坑 9 同源） | **收敛为 `jq_restart_game()` 静态辅助（调用方需已持 g_mutex），6 处全改调用；OnCreate 初始加载（明棋开局）保留独立（非重开语义）**——**新增重开逻辑必须走 `jq_restart_game`，勿散点 `load_layout`**（防"改一处漏五处"） |

**★ 坑 9/9b 共同沉淀（统一入口 = "一致工程"类，C.4 11 坑 → 12 坑）**：**"同一逻辑多处调用/重复代码" 是军棋反复踩的坑类**——① 音效 `sfx_play`（坑 9，3 处散点）② 重开 `load_layout`（坑 9b，6 处重复块）。**通用原则：发现"同一逻辑 2+ 处"就该抽静态辅助/统一入口，调用方约定持锁**；**"改一处漏 N 处"的根因 = 重复代码，防法 = 收敛 + 回归判据同步（N25）**。
| **坑 10（static 声明冲突 → 编译 error）** | `sfx_play` 在 audio.h 非 static 声明、定义处却标 `static` → 编译 error；`sfx_game_over` 调用前缺声明 | **通用 C 语义**（头/定义 static 一致性） | **跨文件调用的函数：声明与定义的 `static` 必须一致；前置 `static void sfx_play(...)` 声明** |
| **坑 11（单步 undo 失效 → AI 反复补应着，栈永不空）** | `undo_pop` 一次恢复 turn=RED → OnUpdate AI 立即补应着 → undo 失效（AI 反复 undo_push） | **C 语义契约 + 回合机制**（凡"撤销/回退 + 自动补应"并存的状态机都适用） | **撤销必须"连续 pop 到目标回合"（turn==BLUE），不能只 pop 一次**——"撤销一步"语义在 AI 应着场景下 = 撤销到"玩家可走态"，非"撤销一条记录" |

**★ 12 坑 3 类重归类**：JNI 反射（坑 2/3/6/7）+ C 语义契约（坑 4/5/8/**10/11**）+ 链接（坑 1）+ **一致工程（坑 9/9b 统一入口）**。坑 9/9b/10/11 是"能玩"闭环后新踩的（**音效统一入口 sfx_game_over / 重开收敛 jq_restart_game / static 声明 / 悔棋单步失效**），已进 AGENTS.md src_rs 段。**★ 一致工程类（坑 9/9b）的共同教训 = "同一逻辑 2+ 处就该抽统一入口，调用方约定持锁 + 回归判据同步"**。

**★ 经验库索引原则（E2 沉淀）**：AGENTS.md 里"8 坑"写在会话 1（src_rs）段，但**坑 2/3/4/5/7 是 JNI/C 通用知识，32 位翻译体会话同样适用**（会话 2 的 §124zz 链接坑 = 坑 1 的翻译体实例）。**读经验库时别只看"哪条路线写的"，要看"坑的类别"——跨路线复用防复发**。

**坑 4/5 的精确教训（C 语义契约，最易复发的类）**：
- **坑 4（enum 符号性优化）**：`jq_try_move` 返回 `(jq_event)-1`，enum 被 C 编译器视为**非负** → `jnev < 0 ? 0 : jnev` 被优化成 `jnev` → `JQ_EVENT_NAMES[-1]` 越界 SIGSEGV（fault = JQ_EVENT_NAMES 地址 - 8，**完美吻合**）。修 = `jq_try_move` 返回类型改 `int` + 所有接收变量改 `int`。**规则：规则引擎返回值契约（-1=非法）一律用 `int`，禁用 enum**
- **坑 5（返回值契约与调用点不一致）**：`ai_pick` 返回 0=成功/-1=困毙，但调用点 `if(ai_pick(...))` 把 0 当 false → **AI 块永不进入**（诊断 `move_ok=1 turn=1 ui=1 stage=1` 但无 AI 日志）。修 = 3 个调用点改 `== 0`。**规则：`if(fn())` 只用于布尔/真值函数；0=成功契约必须显式 `==0`**

---

## C.5 军棋"能玩"验收判据的精确拆解（会话 1 闭环实证）

**AGENTS.md 统一验收判据"装上→进主界面→下一局→正常退出→重进不崩"的军棋现状（R-route，会话 1 实证）**：

| 判据 | 现状 | 实证 |
|---|---|---|
| **装上** | ✅ 双 ABI 可装（arm64 91KB so，v1+v2） | `dist/phase_rs_a64.apk`（40.8KB 64 位） |
| **进主界面** | ✅ 菜单 5 按钮渲染（ GLES2 PBuffer 离屏出像素已证） | `rs_draw_menu` + MuMu 截图 |
| **下一局** | ⚠️ 走子/吃子/炸弹/旗/雷**逻辑全跑通**（**7 类真机事件全 ✓**：MOVE/EAT/BOMB/连续多步/重进不崩/FLAG/MINE）；**★ 游戏内重开已闭环**（`jq.restartgame` property → OnUpdate 游戏内重开 default.lay，对局结束→重开→新对局可走子 + AI 应着，不重启 app，MuMu 13584 实证） | `mumu_walktest.sh`（MOVE/EAT/BOMB 触摸走子）+ `mumu_evtest.sh flag|mine|restart`（**setprop 触发**，触摸 MuMu flaky 不达 native；restart 模式 = flagtest→OVER→restartgame→重开判据）+ 进程存活 |

**★ 游戏内重开闭环（AGENTS.md 最新沉淀，2026-11-04 15:2x 同步）**：

![军棋 7 类真机事件闭环 + 游戏内重开](assets/c5_seven_events_restart.png)
> 🎬 动态版：[`videos/c5_seven_events_restart.mp4`](videos/c5_seven_events_restart.mp4)（10s 环形 7 事件 + 重开闭环动画）
- **路径 ①（可靠）**：`adb setprop jq.restartgame 1` → OnUpdate 每 60 帧读 property → 游戏内重开（`default.lay` 重新开局，stage=PLAY 蓝先手，悔棋/历史 `g_undo_top` 清零），**不重启 app**
- **路径 ②（备用）**：OnTouch 左上角重开按钮（nx<0.15/ny<0.10，触摸可达时）
- **修复了旧"游戏内无重开入口"环境边界**（对局态原无重开入口 + adb 不能调 JNI on_event "restart"）——**验收判据"下一局"全闭环**
- **诚实边界**：触摸路径 MuMu flaky 下不达 native（**OnUpdate property 路径可靠**）；重开重置 default.lay + stage=PLAY + 蓝先手
- **工具**：`mumu_evtest.sh restart` 模式（flagtest→OVER→restartgame→重开判据），实证 PASS

**★ 悔棋闭环（AGENTS.md 最新沉淀，MuMu 16499 真机 + 桌面 6/6 实证）**：
- **触发**：`adb setprop jq.undotest 1` → OnUpdate 连续 `undo_pop` 到 **turn==BLUE**（撤销 AI 应着 + 玩家走子，恢复玩家可走态）；`pop 2 步 → turn=0 stage=1`（蓝 EAT 后恢复）+ 栈空报无历史（不破坏当前态）
- **★ 坑（N21）：undo_pop 一次恢复 turn=RED → OnUpdate AI 立即补应着 → undo 失效**（AI 反复 undo_push，栈永不空）；**修 = 连续 pop 到 turn==BLUE**（不能只 pop 一次）
- **双路径**：`on_event "undo"`（OnEvent 路径，已有）+ `undotest`（OnUpdate property 触发，**触摸 flaky 下的可靠路径**）
- **桌面预验证**：`evverify --undo`（走 EAT → 手动红方应着 → pop 2 步 → cell25/30 原位验证），`--all` 升级为 **47/47 断言**（+draw 和棋/resign 认输/stalemate 困毙 + **★ 会话 1 扩 UI 对齐验证 43 断言**）
- **诚实边界**：悔棋 = 撤销到"玩家可走态"（turn=BLUE），非"撤销到对局初始"；AI 应着会被一并撤销（连续 pop 语义）

**★ 对局结束 UI 闭环（AGENTS.md 最新沉淀，2026-11-04 15:3x 同步，MuMu 15120 实证）**：**★ 对局结束 UI 闭环（AGENTS.md 最新沉淀，MuMu 15120 + 16944 实证）**：

![军棋 对局结束 UI 闭环（win/lose/draw 三色音效 + 胜负横幅 + 再来一局按钮）](assets/c5_game_over_ui.png)
> 🎬 动态版：[`videos/c5_game_over_ui.mp4`](videos/c5_game_over_ui.mp4)（10s 音效/渲染双栏闭环动画）
- **FLAG 吃旗（stage=OVER winner=BLUE）→ 对局结束 UI 两件事**：
  - **音效 `sfx_game_over` 统一入口**（walk/吃子后 `stage==OVER` 自动播 **win/lose**）：`data_junqi_win.wav`/`data_junqi_lose.wav`；**3 处调用统一**（OnTouch 走子块锁外 / AI 应着块锁内 / OnCreate flagtest 块锁内）防"改一处漏两处"——**音效 win/lose 已真机闭环**
  - **`rs_draw_overlay` 胜负横幅**（render.c，GAME 态 `stage==OVER` 画中心横幅，winner 色：BLUE 蓝横幅/RED 红横幅）→ **渲染调用链已触发**（`rs_draw_overlay：胜负横幅（winner=BLUE）` 日志实证）
- **诚实边界**：横幅**像素出屏需真机 GPU 设备验收**（MuMu GLES1 几何出帧受限 §124zz）；**"渲染调用链已触发 + 音效 win/lose 真机闭环" ≠ "屏幕上画出胜负横幅"**（PBuffer 离屏像素已证，window surface 通路未通）
- **音效清单（★ N23 实测纠正，C.5b checklist 精神）**：`audio.c` 实为 **14 个映射 = 14 个 wav 文件**（`rev64/dist/spak_res/` 14 个，`grep -cE '{\'' rev64/src_rs/audio.c` 实测 14）。构成 = **Junqi 组 12**（move/eat/bomb/flag/dead/flip/**win/lose**/draw/start/over/select）+ **Ui 组 2**（ui_close/ui_pop）。**旧写"16 个"是误记（把 Ui 组 Click/Close/Pop 当 3 个，实为 Close/Pop 2 个 + junqi 12 = 14）**。win/lose 是对局结束新增（15120 实证），但**总数仍 14**（替代"Ui 组只有 Close"的早期 12 个 → 加 win/lose = 14）
- **★ 数据型口径红线（C.5b）**：音效数/wav 数**必须 `grep -c` 实测**，不靠"Ui 组 3 个"的记忆——本轮 N23 就是"14 vs 16"漂移，靠 `grep -cE` 实测 14 揪出
- **八件套"真机事件"口径升级**：`mumu_fulltest.sh` **★ 加 UI 关闭/弹窗音效后 9/9**（走子 EAT + FLAG + MINE + 重开 + 和棋 + 认输 + 困毙 + UI 关闭/弹窗音效，**走子含 `--check-turn` turn 交替判据**，19321+ 复验；演进 4/4→6/6→7/7→**9/9**）+ **对局结束 win/lose/draw 三色音效 + 三色横幅渲染链**（15120/18636 实证）- **工具**：`mumu_evtest.sh restart` 模式（flagtest→OVER→restartgame→重开判据），实证 PASS

**★ 和棋/认输/困毙事件闭环（AGENTS.md 最新沉淀，MuMu 18636+19321 真机 + 桌面 8/8 实证）**：
- **① 和棋 `jq.drawtest` property → `jq_on_draw` → winner=2 stage=OVER + `sfx：播放 draw`**（`data_junqi_draw.wav`）；**② 认输 `jq.resigntest` property → on_event resign → winner=1-turn + `sfx：播放 lose`**；**③ restartgame 重开恢复**（stage=PLAY 蓝先手）
- **★ 修 3 个缺口（sfx/渲染对和棋 winner=2 的处理缺失）**：① `sfx_game_over` 原只处理 winner==BLUE/RED，和棋（winner=2）不播 → 加 `else if (winner==2) sfx_play("draw")`；② on_event resign 不经走子块无音效 → 加 sfx_game_over；③ `rs_draw_overlay` 原 winner!=BLUE 走 else 红横幅 → 和棋误画红 → **改三色（蓝胜/红胜/和棋灰）**
- **桌面预验证**：`evverify --draw`/`--resign`（`run_draw_resign`：jq_on_draw → winner=2 stage=OVER；resign → winner=1-turn），`--all` 升级 **7/7 断言**
- **真机全套验证**：和棋/认输已进 `mumu_fulltest.sh`（**★ 加 draw/resign 后 6/6 PASS**，走子 EAT + FLAG + MINE + 重开 + 和棋 + 认输一次跑完，18636 复验）；**困毙（stalemate）= 桌面 47/47 断言之一（无棋可走判负，规则引擎覆盖，真机走子触发需特定残局；★ 会话 1 扩 UI 对齐验证后 8→44→47）**

**★ 困毙事件闭环（AGENTS.md 最新沉淀，MuMu 19321 真机 + 桌面 8/8 实证）**：
- **`jq.stalematetest` property → OnUpdate `jq_on_stalemate` → winner=1-turn（无子可动方负）+ `sfx：播放 lose`** + property 消费（0）+ restartgame 重开恢复（stage=PLAY 蓝先手）+ 进程稳定
- **★ 坑（N36）：logcat -c 时序导致 grep 漏日志**（第一次查询困毙日志未见，实际已触发——logcat -c 清缓冲 + 查询窗口错位）；**修 = sleep 6（60 帧 ~1s 需覆盖 %60 检测周期）+ 完整日志查询**
- **★ 诚实边界**：`jq.stalematetest` = OnUpdate property 直接模拟困毙结果（对齐 `jq_on_stalemate` 语义），**非默认布局真实困毙**（真实困毙需特定残局，规则引擎 `jq_side_stalemated` 走子后自动判断）
- **工具升级**：`evverify --stalemate`（`run_stalemate`），`--all` 升级 **47/47 断言**（含会话 1 扩 UI 对齐验证 43 项）；`mumu_evtest.sh stalemate` 模式 + **`mumu_fulltest.sh` 9/9**（加困毙 + UI 关闭/弹窗音效 close/pop，19321+ 复验）
- **诚实边界**：`on_event "resign"` 已有（OnEvent 路径）；`drawtest/resigntest` = OnUpdate property 触发（触摸 flaky 可靠路径）；**横幅三色像素出屏需真机 GPU**（渲染调用链已触发，同 rs_draw_overlay 诚实边界）
**★ 对局结束 UI 强化：'再来一局'按钮（AGENTS.md 最新沉淀，MuMu 16944 实证）****★ 对局结束 UI 强化：'再来一局'按钮（AGENTS.md 最新沉淀，MuMu 16944 实证）**：
- **`rs_draw_overlay` 加'再来一局'按钮**（横幅下方 35%-65% x / 62%-72% y，**绿色 quad**，与 OnTouch 命中几何一致）+ OnTouch 加**对局结束按钮命中**（`stage==OVER && nx 0.35-0.65 && ny 0.62-0.72` → 重开新局，**不重启 app**）
- **用户流程闭环**：胜负有横幅 → 点'再来一局' → 重开（stage=PLAY 蓝先手 + **清 undo 栈** + 走子可用）
- **★ 重开逻辑收敛 `jq_restart_game`（AGENTS.md 最新沉淀，MuMu 17175 复验）**：6 处重开逻辑（restartgame property / 菜单 action 0/4 / 点棋盘开局 / 对局结束"再来一局" / 左上角悔棋重开 / 对局态点棋盘自动开局）原为**重复代码块** → **统一为 `jq_restart_game()` 静态辅助（jni_bridge.c，调用方需已持 g_mutex）**——防"改一处漏五处"；OnCreate 初始加载（明棋开局）保留独立（非重开语义）。**新增重开逻辑必须走 `jq_restart_game`，勿散点 `load_layout`**（这是坑 9"统一入口"模式的第二实例，第一是 `sfx_game_over`）- **用户流程闭环**：胜负有横幅 → 点'再来一局' → 重开（stage=PLAY 蓝先手 + **清 undo 栈** + 走子可用）
- **诚实边界**：按钮**像素出屏需真机 GPU 验收**（MuMu GLES1 几何受限）；渲染调用链已触发（`rs_draw_overlay：胜负横幅 + 再来一局按钮` 日志实证）；触摸路径在 MuMu flaky 下不达 native（**`jq.restartgame` property 路径可靠备用，已验证**）
| **正常退出** | ✅ 进程稳定（MuMu 45s~150s 无崩溃/ANR） | 多轮实证 |
| **重进不崩** | ✅ **闭环**（force-stop → 重启 → 菜单进对局 → 走 1 回合 → AI 应着 → sfx eat，**重进 ×2 均成功无崩溃**） | MuMu 11062/11185 实证 |

**★ 事件验证"桌面预验证 → 真机"闭环（evverify ⑥ + 真机 ⑤，防"真机盲目试探"）**：
- **桌面**：`make evverify MODE=--all`（⑥ 件）**47 断言全对**（eat 25→30 ev=1 / bomb 27→32 ev=2 / move 12→11 ev=0 / flag 53→58 ev=3 / mine 49→54 ev=5 / **undo 悔棋：连续 pop 到 turn=BLUE + cell 原位** / **draw 和棋：winner=2 / resign 认输：winner=1-turn / stalemate 困毙：winner=1（无棋可走判负）** + **★ 会话 1 扩 UI 对齐验证 43 断言（+ui118/exit/resign/regretconfirm 4 弹窗）**），替代每轮手写 /tmp/*.c 构造状态；已进 **12/12 回归第 12 项（判据 47/47）**
- **真机**：MOVE/EAT/BOMB 走 `mumu_walktest.sh` 触摸；FLAG/MINE 走 `mumu_evtest.sh` setprop 夹具（触摸 MuMu flaky 不达 native）
- **方法论**："验证事件先桌面 evverify 确认规则引擎 → 再真机走对应触发路径"，免真机 flaky 盲目试探（同走子"先桌面 walkverify 再真机 mumu_walktest"的固化套路，本轮把 FLAG/MINE 也补进桌面 evverify）

**★ 真机事件覆盖全（7 类，会话 1 最新闭环，2026-11-04 同步）**：
| 事件 | 触发 | 实证 |
|---|---|---|
| **MOVE/EAT/BOMB** | `mumu_walktest.sh` 触摸走子（坐标固化） | 真机 EAT#1→AI→EAT#2 + BOMB 同归于尽（桌面 evverify 预验证全对） |
| **连续多步** | 触摸多轮（25,30 / 28,33） | turn 交替持续 + 事件计数递增 |
| **重进不崩** | force-stop ×2 | 无崩溃 |
| **FLAG（吃旗胜）** | `adb setprop jq.flagtest 1` + 重启 → OnCreate 构造近旗状态 + 自动走 | `ev=3 Junqi.Flag（turn=1 stage=2 winner=0 蓝胜）` + sfx flag，MuMu 12731 实证 |
| **MINE（工兵挖雷）** | `adb setprop jq.minetest 1` + 重启（同 flagtest 路径） | `ev=5 Junqi.Move（红有子@59 挖雷不结束对局，stage 保持 PLAY）` + sfx move，MuMu 12872 实证 |

**★ FLAG/MINE 触发的诚实边界（防误读）**：
- **触发路径 = `setprop + OnCreate 自动走`，非触摸**——MuMu 触摸注入 flaky 不达 native（环境限制，checkpoint 已知）；事件链路/规则引擎/音效**真实**，仅"走子由 OnCreate 自动触发"非触摸
- **flagtest/minetest 是测试夹具（起点状态注入）**：从默认布局蓝方到红军旗需 10+ 步真实走子（红 AI 干扰），夹具直接构造"近旗/近雷"起点，**非默认布局开局的真实对局路径**
- **挖雷非对局结束**：红有子@59 → 工兵 rank9 挖红雷 rank11 后 stage 保持 PLAY（桌面 `jq_try_move(49,54)=5=EV_MINE` + 真机双证）
- **工具固化**：`rev64/tools/mumu_evtest.sh flag|mine`（setprop 触发 + 重启 + 日志 + 进程判据），mine 复验 PASS

**诚实边界（不包装成"可玩"）**：
- **GL 出屏画面需真机 arm64 GPU 验收**（MuMu PBuffer 离屏出像素已证 GLES2 通路完整，但 Java GLView 的 SurfaceView surface 未真正创建（`Surface.isValid=0`）→ window surface 通路未通，屏幕可见需真机）
- **音效出声需真机音频通路**（MuMu 播放链已验证，真机听声待验）
- **蓝牙 SPP socket IO 由 Java 宿主承担，无真机蓝牙验证**；7 字段语义为行为级约定（type,fc,fr,tc,tr,rank,side），非原版 C++ 类布局逐字段逆向
- **"游戏内下一局"重开是真机环境边界**（对局态无重开入口，桌面/Java 侧可触发 restart→default.lay 已证，真机走子验证是环境限制）
- **"播放链全通"≠"真机出声"；"字段填对+不崩"≠"画棋盘"**（AGENTS.md 红线）

---

| **C.3.4 走子/事件/全周期七件套 + 资产复用** | ch31（部署） | "验证走子先桌面预验证 → 再真机 → 事件夹具验证 → 全周期一条龙"的全程脚本化（防"每轮手写 adb/zip//tmp 构造状态"）；资产固化在 APK 只换 so；八件套 = ①坐标 ②walkverify（含 --longrun）③mumu_walktest（含 --check-turn）④rebuild_rs_apk ⑤mumu_evtest ⑥evverify（进 12/12 回归）⑦rs_cycle 全周期 ⑧mumu_fulltest 真机全套（★ 加 draw/resign 后 6/6 PASS，含和棋/认输，走子含 --check-turn） |

## C.5b 数据型表述必读表 + 可执行 checklist（5 轮 loop 口径漂移根治，2026-11-04）

**根因**：5 轮对抗审计反复出"数据漂移"（1553→5539 / 5456→5549 / 33→214 PLT / 1465→1479 符号 / 91KB→94824B so / 40.8KB→6.9MB APK / 10/10→11/11）。**共同根因 = 靠"快照记忆"写数据，不实测**。课程/经验库里的数据型表述一旦脱离生成时刻的产物，必然漂移。

**根治方法（写任何数据型表述前，先跑对应命令，不靠记忆）**：

| 数据型表述 | 必读命令 | 军棋锚点（本轮实测值） |
|---|---|---|
| **重定位条数** | `arm-linux-gnueabi-readelf -rW <so>` | 军棋 5539（RELATIVE 5312 + JUMP 214 + GLOB 6 + ABS 7） |
| **PLT 跳板数** | `readelf -rW <so> \| grep -c JUMP_SLOT` | **214**（非 33，33 是早期小 so 笔误） |
| **.dynsym 符号数** | `readelf -sW <so> \| awk 'NR>2 && /:/{n++} END{print n}'` | 原版 **1479**（无 .symtab，strip） |
| **so 产物尺寸** | `stat -c%s <so>` | 翻译体 8171728B / src_rs 94824B（随版本演进，**写"~XX KB 随版本演进"不写死**） |
| **APK 产物尺寸** | `stat -c%s <apk>` | 翻译体 14943767B / src_rs 资产版 ~6.9MB |
| **行数口径** | `wc -l src_rs/*.c src_rs/*.h` | 5549 |
| **回归项数** | `grep -c '^\s*run "' rev64/src_rs_regress.sh` | **11/11**（第 12 项 evverify） |
| **.bss/.data 槽值** | `dd if=/proc/PID/mem bs=1 skip=ADDR count=N` + `readelf -rW` | 读侧 57 槽（RPI 公式 `((addr+4)&~3)+disp`） |
| **vtable 槽号** | `tex_pipeline.py vslot <十进制槽号>` | 纹理类 0x1595c0 slot[13]=清屏 / slot22/23=纹理创建/删除 |

**★ 红线（写课程/AGENTS.md 数据前自问）**：
1. **这个数是"此刻实测"还是"上轮快照"？** 快照必须加"随版本演进/XX 时刻实测"标注
2. **数据源在 /tmp 吗？** `/tmp` 会被清（seed_v2.json 丢失实证）——**持久数据固化进 `rev64/dist/`（seed_funcs.json 先例）**
3. **readelf/ls/wc 跑过没有？** 没跑过的数据型表述 = 漂移候选，下一轮必被审计抓出

**★ N26 教训机制化（课程内部前后对照查重，防"同主题讲两遍"）**：
11 轮 loop 抓的**最深矛盾（N26）= 课程内部前后重复**：给 ch01 1.4.3 加"32→32 重编译"时，**没 grep 该主题是否已存在**（ch01 已有 `1.4.3★ 路线 A 的活路`）→ 大段重复 + 术语冲突（INDEX 叫"32→32 翻译体"，新加段叫"路线 A'"）。**修法 = 加新内容前先 grep 该主题**：

```bash
# 给课程加"X 主题"前，先查 X 是否已在课程/INDEX 存在（防 N26 前后重复）：
grep -rn "32→32\|翻译体\|gate-set\|对象图" course_unidbg/ | grep -v "AUDIT_LOOP"   # 主题已存在则"改"不"加"
grep -n "路线 A\|路线 B" course_unidbg/01_basics/ch01_env_setup.md                # 术语是否冲突
```

**★ 红线（C.5b 补第 4 条，防课程内部前后重复）**：
4. **加新内容前先 grep 该主题是否已存在（课程内部）**——同主题"改"不"加"，术语统一（ch01/INDEX/C 同源），避免"路线 A'"vs"32→32 翻译体"式术语冲突 + 大段重复（N26 教训）

**★ 可执行 checklist（写数据型表述前，逐条跑，把"此刻实测值"写进台账——防第 7 轮再出漂移）**：

```bash
# 写任何"军棋 so 数据"前，先跑这 4 条（10 秒），用输出值，不用记忆：
readelf -sW <so> | awk 'NR>2 && /:/{n++} END{print "dynsym=" n}'     # 符号数（原版 1479）
readelf -rW  <so> | grep -c JUMP_SLOT                                  # PLT（军棋 214）
stat -c%s <so>                                                          # so 尺寸（写"~XX 随版本演进"）
wc -l rev64/src_rs/*.c rev64/src_rs/*.h | tail -1                       # 行数（5549）
# 回归项数：grep -c '^\s*run "' rev64/src_rs_regress.sh                # 11/11
# 写进台账时格式：「实测值（命令 + 时刻）+ 若源自 /tmp 则标注『已固化进 dist/』」
```

**★ 台账落点**：每轮 loop 在 `AUDIT_LOOP.md` 记"本轮实测数据"小节（命令 + 输出 + 时刻），下一轮**先读台账再写**，不凭记忆。

---

## C.6 跟 31 章 + 附录 B 的呼应## C.6 跟 31 章 + 附录 B 的呼应（C.3.4 走子四件套补充见上表）

| 本附录 C | 对应课程 | 关系 |
|---|---|---|
| **C.1 代码缺口 vs 数据缺口** | ch05（调用链判据）+ 附录 B.5c（逆源码路线） | "补 stub"是代码缺口（B.5c.2 的"补完 stub ⚠️"），"对象图数据"是 B.5c ⑥ 层（运行时，逆不出）——C.1 把两者分清 |
| **C.2 Frida dump 边界** | ch13（JNI trace）+ FRIDA_NOTES §八 + B.5c.6 | "unidbg+Frida 一对一对齐"的前提（同一进程喂真值）+ 跨进程回填不可行（C.2 实证） |
| **C.3 审计工具三件套 + 走子四件套** | ch28（Hook/XHook）+ ch30（加固）+ ch31（部署） | 审计工具是"Hook 的逆向版"（不注入，离线 diff UND/ExceptionCheck/vtable 写入点）；走子四件套（含 rebuild_rs_apk.sh 重打包装机 + 资产复用）是"部署/验收的脚本化" |
| **C.4 增强循环方法论** | ch26（环境补全总结） | "每轮闭环→沉淀→下轮"是 ch26"补环境"的工程化（补环境不是补一次，是持续补 + 防回归） |
| **C.5 验收判据拆解** | ch31（部署） | "能玩"的验收 = ch31 的"上线判据"在军棋的具体落地（装上/进主界面/下一局/退出/重进） |

**一句话**：**附录 C 补的是"逆向工程化"维度（31 章讲"怎么逆"，C 讲"逆的过程中怎么不翻车、怎么防回归、怎么沉淀下轮"**——**"代码 100% 对齐 ≠ 能跑"（C.1）+ "frida 跨进程回填不可行"（C.2）+ "机械化审计工具防回归"（C.3）+ "增强循环沉淀"（C.4）+ "能玩验收判据拆解"（C.5）**。

---

*最后更新：2026-11-04（轮次 1 闭环修订）——
- **新建**（同日）：逆向工程化方法论——C.1 代码缺口 vs 数据缺口（补 stub 破不了对象图，全量 4245 实证）+ C.2 Frida dump 路线边界（跨进程 scudo 指针不可回填）+ C.3 机械化审计工具（link_audit/jni_method_audit/tex_pipeline + 走子/事件/全周期七件套（+mumu_evtest/evverify/rs_cycle）+ rebuild_rs_apk.sh/arm32_cycle.sh 两条全周期）+ C.4 增强循环方法论（8 坑 3 类 + 防复发）+ C.5 军棋"能玩"验收判据拆解（重进不崩闭环）+ C.6 跟 31 章/附录 B 呼应；源自 R-route 正向开发（src_rs 5549 行 C，`wc -l *.c *.h` 实测）+ 32 位翻译体 sp27~sp28-SCALE-9 两个会话沉淀
- **轮次 1 审计修订**（AUDIT_LOOP.md 台账驱动）：
  - C.3.4 三件套→**四件套**（+mumu_evtest.sh 事件夹具 + evverify.c 桌面事件验证 + rs_cycle.sh 全周期一条龙；资产复用 rs168_wav.apk；回归 9/9→10/10→11/11→12/12）
  - C.4 8 坑加**"适用路线"列**（坑 2/3/4/5/7 通用，坑 8 R-route 专有，坑 1 两路线各一实例）+ 跨路线复用原则
  - C.3.4 加 **walkverify 双重职责区分**（四件套第 ② 件 vs 回归第 10 项）+ **会话 2 对称件 arm32_cycle.sh（D5）**
  - **★ 审计自动化覆盖不对称（第三类审计实证）**：会话 1 审计工具已进 12/12 回归（第 5/9 项，EXIT=0 实证）；会话 2 `pack_arm32.sh` **未内嵌 link_audit.py**（grep 0 命中，C.3.1"打包前必跑"是手工步骤，`arm32_cycle.sh` 每轮不保证跑）——待办：link_audit 进 pack_arm32.sh 第 3 步前
  - 口径厘清：`regress.py` 判据 = **≥90% 合法终局**（`assert ok >= games*0.9`），非 100%；"60 局"有两个来源（regress.py 的 60 vs gametest 的 60，判据不同）；src_rs 总行数 **5549**（旧稿写 5456，wc -l 实测修正）
- **课程数据同步（同轮，AUDIT_LOOP L1/D1/E1/D3/L3/E2）**：B.2.1/B.4/B.5 全量 "1553"→"5539"（6 处 + 旧分解 RELATIVE 1516+JUMP 33+GLOB 4 → 5312+214+6+7 ABS）；AGENTS.md R-route 段 "60 局全合法"→"12/12 全 PASS" 口径 + 第二个 "〇b"→"〇e" + "57 槽"读侧/写侧调和句；ch13 "0x43ed8" 标注"OnCreate 路径"（OnUpdate/OnEvent 不走该地址）
