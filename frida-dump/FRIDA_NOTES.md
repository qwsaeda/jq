# 军棋 Frida 工作流参考（干活会话系统参考）

> **定位**：本文档是军棋项目**已落地 + 待补**的 Frida 用法系统参考，供"干活会话"随时查。
> 跟 unidbg 课程（`course_unidbg/`）的关系：unidbg = **离线模拟 so**（x86 搬进 unicorn），
> Frida = **真机 hook/trace so**（真 GPU/真对象图/真实运行时值）。两条路互补，军棋两条都建。
>
> **为什么独立成文**（不混进 unidbg 课程）：Frida 跟军棋干的工作**强相关**（`frida-dump/` 已在用），
> 但它是另一套工具栈（真机 vs 离线模拟），混进 unidbg 课程会模糊"离线 vs 真机"的边界。
> 陈佳林 3 本 Frida 书里只有第 3 本《Frida Android SO 逆向深入实践》（SO/Native 层）跟军棋同赛道，
> 前 2 本（Java 层抓包/协议分析）军棋是单机游戏不吸收。

---

## 一、军棋 Frida 在整体工作流里的位置

军棋 32 位 `libjunqi.so` 逆向有**两条拿"运行时真值"的路**：

| 路线 | 工具 | 场景 | 军棋现状 |
|---|---|---|---|
| **离线模拟** | unidbg（unicorn2 后端） | 无真机、x86 跑 32 位 so | §124 三项阻塞卡死（BL bug/对象图/288MB mmap） |
| **真机 trace** | Frida（attach/spawn + hook） | 真机/redroid 跑 32 位 so | **`frida-dump/` 已落地**（读 18 地址 + dump .data/.bss）；**★ 18 地址值现静态固化在 `jqt_mem.c`，无需运行时 dump（§124x-z 路线演进）** |

**关键认知**：unidbg 静态/模拟读不出的**运行时 18 个地址值**（dlopen 重定位才填），
Frida 真机 attach 后**直接读内存**就能拿到——这是军棋拿这 72B 唯一路线（AGENTS.md 红线 §"14 JNI 入口可运行卡 18 地址"）。

---

## 二、`frida-dump/` 已落地工具（干活会话直接用）

### 2.1 目录结构

```
frida-dump/
├── README.md                  # 机制说明（.text 明文/.rodata 已 dump 结论）
├── redroid_jni_dump.py       # ★ 读 18 地址运行时值（15 .text 常量池 + 2 .data + 1 .rodata 指针表）
├── redroid_dump_jni.sh       # ★ 一键触发（adb connect → am start → frida-server → attach dump）
├── gen_runtime_table.py      # ★ jni_runtime_values.json → C 注入表（拼进 jni_aligned.c）
├── data_dump.py              # spawn 模式 dump .data(0x314) + .bss(0x7008) → redroid_data/bss.bin
├── data_dump2.py             # attach 模式 dump（am start 后 attach，更稳）
├── dump_shell.py             # 32 位真机模板（找 base + 断点壳入口 + 等自解密 + dump 段）
├── shell_probe.c/.so         # 32 位探针（验证 redroid 能否 dlopen 32 位 so）
├── rodata_dec.bin            # .rodata 118832B（与静态 .so 逐字节相同，非加密）
├── rodata_plain_full.json    # 118832B 的 JSON 化（0x138xxx 压缩索引表）
├── rodata_plain_strings.txt  # 86KB 明文字符串（含 2209 真实函数指针 + .data.rel.ro 263 个）
├── rodata_strings.txt        # 1715 条早期 dump
└── jni_runtime_values.json   # ★ 产物（18 项 {off, runtime_value}）——已删（18 值改静态固化在 jqt_mem.c g_jq_got_tbl）
```

> **★ 路线演进（§124x-z 后，重要）**：早期"redroid dump → `jni_runtime_values.json` → JSON→C 注入"路线**已废弃**。
> 18 个地址值现**静态固化在 `rev64/src_jni_translated/jqt_mem.c` 的 `g_jq_got_tbl`**（静态位移，非 redroid 绝对值，
> 不同进程基址通用），**无需运行时 dump**。`jni_runtime_values.json` 文件已删。
> 下面的 2.2 流程 + 工具三件套**保留作备查**（万一要重 dump / 换基址仍适用），但**当前干活会话直接用 `jqt_mem.c` 固化值，别找/生成 json**。

### 2.2 读 18 地址的完整流程（`redroid_jni_dump.py` + `redroid_dump_jni.sh`）

**18 个 base 相对偏移**（来自 `/tmp/jni_dump_targets.json`，`TARGETS` 列表）：
- **15 .text 常量池**：`0x0233f0` ~ `0x023608`（14 JNI 入口读的内嵌加密常量池，dlopen 重定位才填）
- **2 .data**：`0x15be80` / `0x15be84`
- **1 .rodata 指针表**：（`rodata_plain_strings.txt` 里的 2209 真实函数指针，若需另加 base 偏移）

**流程**（`redroid_dump_jni.sh` 一键）：
```
[1] adb connect localhost:5555   # 连 redroid 容器
[2] am start -n com.cnvcs.junqi/.App   # 触发 so dlopen + init_array（OnCreate 会 NPE 崩，但 native 自解密在崩前完成）
[3] su 0 起 32 位 frida-server（/data/local/frida-server-32）
[4] frida attach com.cnvcs.junqi -l redroid_jni_dump.py
       → JS 找 libjunqi.so base（Process.enumerateModulesSync）
       → 读 18 个 base+off 的 readU32
       → send 回宿主 → 落盘 jni_runtime_values.json
[5] 校验 18/18 读到 + 喂 jni_aligned.c
```

**喂注入层**（`gen_runtime_table.py`）：
```
jni_runtime_values.json → 生成 g_jq_runtime[JQ_GLOB_COUNT] C 表
→ 替换 rev64/src_jni_translated/jni_aligned.c 占位表
→ bash rev64/build_aligned.sh 重编 → 14 入口读运行时值可运行
```

**AGENTS.md 红线对应**：这 18 地址"**仅 dlopen 重定位填**（.so 未重定位，.got 槽为零），
不是壳自解密（.rodata 静态即明文、无自解密例程）"。**★ 当前路线（§124x-z 后）：18 值已静态固化在
`rev64/src_jni_translated/jqt_mem.c` 的 `g_jq_got_tbl`（静态位移，非 redroid 绝对值，不同进程基址通用），
无需运行时 dump；`jni_runtime_values.json` 已删**。上面的 redroid dump 72B 是早期路线，保留备查。

### 2.3 三个 dump 脚本的差异（按需选）

| 脚本 | 模式 | 用途 | 军棋场景 |
|---|---|---|---|
| `redroid_jni_dump.py` | **attach**（`frida.attach('com.cnvcs.junqi')`） | 读 18 地址运行时值 | **主力**（AGENTS 红线拿 72B） |
| `data_dump.py` | **spawn**（`dev.spawn` + 轮询 dlopen + resume） | dump .data+.bss 全段 | 拿 .data/.bss 真值对比 unidbg 垃圾指针 |
| `data_dump2.py` | **attach**（am start 后 attach） | 同上但更稳 | 备选（spawn 模式在 redroid 偶发 attach 失败时） |
| `dump_shell.py` | 32 位真机模板（root + 断点壳入口 0x3dae5） | dump .text/.rodata 段 | **已过时**（.text/.rodata 静态即明文，无需 dump；留作参考） |

---

## 三、Jnitrace：看 JNI 调用流（§124q 动态调用链真机替代路）

### 3.1 为什么军棋需要 Jnitrace

§124q 的"动态调用链追踪"在 unidbg 下**被 unicorn2 BL 解码 bug 卡死**（0x23578 `bl 0xbdacc` 被解码成 0x13c576 崩）。
**Frida Jnitrace 真机 trace 不经过 unicorn 模拟器** → **绕开 BL bug**，是 §124q 动态调用链在 BL bug 未修期间的唯一真机替代路。

### 3.2 Jnitrace 是什么（来自《Frida SO 逆向深入实践》）

**Jnitrace = Frida 的 JNI 调用流 tracer**（hook `RegisterNatives` + 所有 `CallXxxMethod`/`FindClass`/`GetMethodID`）：
- **看什么**：so 在 JNI 边界调了哪些 Java 方法（`FindClass("com/baidu/mobads/AdView")`、`CallStaticVoidMethod(OnCreate)` 等）
- **跟 §124q 调用链判据的关系**：Jnitrace 输出 = **JNI 边界调用链**（so → Java 的调用序列），
  正好是 §124q 方法论点 4（"so 内部 vs Java 交互边界"）的**真机动态验证**——
  看 14 入口实际调了哪些 JNI 函数（哪些是纯 native、哪些需 Java 交互）

### 3.3 军棋 Jnitrace 脚本（待建，模板）

`frida-dump/` 当前**没有 Jnitrace 脚本**（只有 18 地址 dump + .data/.bss dump）。
可补一个 `jni_trace.py`（hook `RegisterNatives` + 14 入口的 `CallXxxMethod`）：

```python
# jni_trace.py 模板（真机/redroid 跑 32 位 so 看 JNI 调用流）
import frida, sys

js = """
// 1. hook RegisterNatives：看 14 入口在 so 里注册到哪些 Java 方法
Java.perform(function() {
    var RegisterNatives = Java.use('art::JNI_RegisterNatives'); // 或 JNI 函数表
    // 简化：hook 14 个 JNI 函数表项（FindClass/GetMethodID/CallXxxMethod）
    var FindClass = Java.use('android.app.ActivityThread').currentApplication; // 占位
});
// 2. 用 Frida 内置 Jnitrace（frida-tools 的 objection jnitrace）
//    或手动 Interceptor.attach JNIEnv 函数表各槽（FindClass/NewGlobalRef/CallStaticVoidMethod...）
// 3. 14 入口触发：am start 后 OnCreate 被调，Jnitrace 记录 FindClass(GLView) + CallStaticVoidMethod(OnCreate) 序列
"""

dev = frida.get_device('usb' if 'usb' in sys.argv else 'localhost:5555')
session = dev.attach('com.cnvcs.junqi')
script = session.create_script(js)
script.on('message', lambda m: print('[jni]', m))
script.load()
print('[+] Jnitrace 记录 JNI 调用流（§124q 动态调用链真机替代）')
input('Ctrl-C 停止')
```

**更省事的替代**：用 `objection` 一行命令（社区工具，Frida 底层）：
```bash
objection -g com.cnvcs.junqi --track jnitrace   # hook 14 入口全部 JNI 调用，输出调用流
```

### 3.4 Jnitrace 跟 unidbg 看 JNI 调用流的对照

| 维度 | unidbg traceCode | Frida Jnitrace |
|---|---|---|
| **看 JNI 调用** | ✅（DalvikVM 接 FindClass/CallXxx，13.2/13.4 已讲） | ✅（hook JNI 函数表，本节） |
| **运行时真值** | ❌（mock，非真实对象图） | ✅（真机真实值） |
| **受 BL bug 影响** | ✅（§124①，0x23578→0x13c576 崩） | ❌（不经过 unicorn） |
| **军棋可用期** | §124 BL bug 修好后 | **现在就能用**（BL bug 未修期间唯一动态路） |
| **输出** | 指令级 trace（bl 目标序列） | JNI 边界调用流（CallXxx 序列） |

**怎么配合**：§124q 动态调用链 = **so 内部 bl 目标序列**（unidbg traceCode 的"bl 序列"）
+ **JNI 边界调用流**（Jnitrace 的"CallXxx 序列"）。两条拼起来才是完整调用链。
unidbg BL bug 未修 → **JNI 边界用 Jnitrace（真机）+ so 内部用静态调用图（capstone）兜**。

---

## 四、frida-server 部署（redroid 容器）

**redroid 跑 32 位 so 需要 32 位 frida-server**（x86 宿主 + houdini 转译 ARM32）：
```bash
# 1. 推 32 位 frida-server 到容器
adb -s localhost:5555 push frida-server-32 /data/local/
adb -s localhost:5555 shell "su 0 chmod +x /data/local/frida-server-32 && su 0 /data/local/frida-server-32 &"
# 2. 宿主 frida 连（frida-tools 已装）
frida -s localhost:5555 -l redroid_jni_dump.py
# 或 32 位真机（root）
adb root && adb push frida-server-32 /data/local/ && adb shell /data/local/frida-server-32 &
frida -U -l redroid_jni_dump.py
```

**frida-server 跟 so 的位宽匹配**：32 位 `libjunqi.so` → **32 位 frida-server**（`frida-server-32`）；
64 位 so → 64 位 frida-server。军棋 32 位 so 一律用 32 位 server（`redroid_dump_jni.sh` 第 3 步已处理）。

---

## 五、军棋 Frida 能干什么 / 不能干什么（诚实边界）

| 能干什么（✓） | 不能干什么（✗） |
|---|---|
| 读 18 地址运行时值（72B，喂 jni_aligned.c）→ 14 入口可运行 | 让 unidbg §124 三项阻塞消失（Frida 是**真机**拿真值，不修 unidbg 模拟器 bug） |
| Jnitrace 看 JNI 调用流（§124q 动态路替代，绕开 BL bug） | 让 32 位 so 在 64 位 only 真机跑（Frida 不改变 so 位宽，arm64 only 设备装不了 32 位 so） |
| dump .data/.bss 真值（对比 unidbg 垃圾指针） | dump 后"游戏能玩"（Frida 只拿**数据**，"能玩"要真 GPU 真机渲染，redroid 无 GPU 验证不了） |
| 32 位真机/redroid attach 看 so 真实行为 | 让 unidbg 离线重放调用链（Jnitrace 是真机一次性 trace，不可离线重放；离线重放要 unidbg BL bug 修好） |

**一句话**：Frida 跟 unidbg 是**互补两条路**——unidbg 拿"离线可重放的模拟执行"，Frida 拿"真机真实值 + JNI 调用流"。
军棋现状（§124 卡死）下，**Frida 是拿 18 地址运行时值 + §124q 动态调用链的唯一可用路**；
unidbg BL bug 修好后，两条路才能完整拼出"离线重放 + 真机验证"。

---

## 六、跟陈佳林 3 本 Frida 书的关系

| 书 | 层 | 军棋吸收 | 说明 |
|---|---|---|---|
| 《安卓 Frida 逆向与抓包实战》（2021） | Java 层（Hook/抓包/okhttp） | ❌ 不吸收 | 军棋单机游戏无 HTTP 协议栈 |
| 《安卓 Frida 逆向与协议分析》（2022） | 协议层（注册登录/付费绕过） | ❌ 不吸收 | 军棋无服务器协议 |
| 《Frida Android SO 逆向深入实践》（2023.10） | **SO/Native 层** | ✅ 吸收 1 处（Jnitrace） | 同赛道，Jnitrace 看 JNI 调用流补 ch13 13.4b + 本文档第三节 |

**只第 3 本相关**（军棋是 Native/SO 层逆向，前 2 本是 Java/协议层）。

---

## 七、待办（干活会话可推进）

1. **建 `frida-dump/jni_trace.py`**（Jnitrace 模板，第三节 3.3）→ 真机看 14 入口 JNI 调用流
2. ~~**跑 `redroid_dump_jni.sh` 拿 18 地址值** → 落 `jni_runtime_values.json`~~ **★ 已过时（§124x-z）**：18 值现**静态固化在 `jqt_mem.c` 的 `g_jq_got_tbl`**（静态位移，跨进程通用），无需运行时 dump；`jni_runtime_values.json` 已删。仅当要重 dump / 换基址时才用 `redroid_dump_jni.sh` 备查。
3. **§124q 动态调用链**：unidbg BL bug 修好前，用 Jnitrace（JNI 边界）+ 静态调用图（so 内部 bl 序列，capstone）拼完整链
4. **真机"能玩"验证**：Frida 只能拿数据 + trace，"能玩"要真 GPU 真机（redroid 无 GPU，替代不了）
5. **★ unidbg + Frida 一对一逐步对齐（第八节，§124ah 棋盘破题方向）**：Frida（MuMu/redroid 真机 attach，**32 位 frida-server**）dump 对象图构造后的真值（vector begin/end、渲染器对象 vtable=**0x1595c0**、纹理 `[this+48]`、顶点 buffer、`.got` 真值）→ 逐字段填进 unidbg mock / 翻译体影子内存 → 把 unidbg 的"零值对象图"（**`.got` 全零**）对齐成"真对象图" → 棋盘破题（§124ah 对象图分歧=核心阻塞）
6. **建 `frida-dump/stalker_trace.py`**（第九节 Stalker 指令级追踪模板，看雪 x1ny 银行加固实战文沉淀）→ 真机 MuMu 上跟踪 `libjunqi.so` 内指定偏移函数执行指令（§124ah 渲染对象图/构造链真值采集，unidbg BL bug 未修期间唯一真机动态路；只吸收套路，不吸收原文壳样本目标）

---


> **★ 路线边界实证（会话 2 sp28-SCALE-9b，"frida 帮不了回填"的最终结论，务必先读）**：
>
> 第八节"unidbg + Frida 一对一对齐"是**破题方向**，但会话 2 实证了 frida dump 路线的**真实边界**：
> - **frida 在 MuMu 上可用**（frida-server 在 `/data/local/tmp/frida-server`，27042 可达）；原版 32 位 so 真 dlopen + 场景构造成功
> - **dump 成功**：`dd /proc/PID/mem` 抓 `.bss` 28680B（**79% 非零 = 场景对象图运行时构造态**）+ `.data` 28288B
> - **但 .bss 整段回填实证失败**（sp28-SCALE-9c）：把 dump 的 .bss memcpy 进翻译体影子 .bss → **GLThread SIGSEGV SEGV_MAPERR 0x3a**。根因 = .bss 运行时值里的指针指向 **MuMu 原版进程的 scudo 堆**（0xcfxxxxxx），翻译体进程的 `jqt_mem_valid` 真映射区表无此区 → 解引用越界
> - **frida dump 路线的真实边界**：能 dump .bss/.data 的"**值**"（含 load_base），但**回填不可行**——①scudo 堆指针跨进程失效；②要的是"对象图"（堆里构造的 std::list 节点/std::string 对象），**dump 值带不回来**；③翻译体能填的只有"翻译体自身构造的对象"（F_9d3d4 渲染器单例，sp28-ROOT 已证可行）
> - **结论**：**frida 路线 = 能提取数据（.rodata 明文已做），不能回填对象图**。"一对一对齐"要成立，**Frida 真值必须喂给"能真正 dlopen 原版 so 的同一进程"（如 redroid 真跑原版），跨进程 dump 值回填翻译体 = 走不通**

## 八、unidbg + Frida 一对一逐步对齐（§124ah 棋盘破题方向，§124z→§124ag/ah 演进）

**核心思路**（用户提出）：unidbg 负责"模拟加载 + 调用链分析 + so 逻辑"（数据侧读不出运行时真值），
Frida 负责"真实填充对象图真值"，**两个工具一对一逐点把 unidbg 的"零值对象图"填成"真对象图"**——
这是军棋棋盘绘制破题方向（§124z 黑屏根因 → §124ag/ah 三层阻塞，核心 = 对象图分歧）。

**★ §124ag/ah 最新状态（v178ck，本方案为什么现在更关键）**：
- **§124ag 里程碑**：真实 glClear（**F_9d458**，渲染器 vtable **0x1595c0** slot13）已 dispatch 到渲染循环
  （F_23eec L2400c 强制 dispatch，i2f 存根修复）→ **首帧清屏色（蓝）达成**，绿/蓝双轮实证。诊断 glClear 已移除。
- **§124ah 三层阻塞**（棋盘未画，诚实边界）：
  1. **draw dispatch 未达**：F_23eec 仅 slot13(glClear) dispatch，无 slot14/45/69；F_9ef20(draw,slot69) 无 BL/`ldr [rX,#0x114]`/`add.w #0x114` dispatch 点。
  2. **对象图分歧（核心）**：渲染器对象（vtable **0x1595c0**）+ 纹理 `[this+48]` + 顶点 buffer 未构造。
     F_9ef20 需 `[this+48]` 非零 + r4 有效 vtable slot13/14/15 返回非零。F_23eec 原 slot13 目标=垃圾（0x920fc=memcpy 中间）。
  3. **触摸→对局路径未翻译**：`jqt_OnEvent`(→F_bd914 存根)、`jqt_OnTouch`(→F_bdd2c 存根)；F_bd914/F_bdd2c + 5 callee（F_b26ac/F_b690c/F_b6948/F_bd580/F_bd778）**在 shadow gen 已完整**，待复制进编译版。
- **关键洞察**：`0x1595f4 = 0x1595c0 + 52 = slot13 槽地址`（非 vtable 基址）。**渲染器 vtable 基址 = 0x1595c0**
  （slot13=0x9d459 F_9d458 glClear，slot14=0x9d43d→F_9e760 glDrawArrays，slot45=0x9e3e5→glDrawArrays@0x9e5cc，slot69=0x9ef21 draw）。
- **Frida dump 对象图真值 = 破题方向**：§124ag 强制 glClear 是 workaround（清屏色可见，非棋盘）；
  真实棋盘需对象图真值（渲染器对象+纹理+顶点 buffer 构造后）——这正是 Frida dump + 逐字段填的用武之地。

**★ 用户确认的两个关键前提**（§124ah 本方案成立的基础）：
1. **MuMu 能直接 Frida hook 来"填充真值"**——**★ §124am-2 权威纠正（推翻本节早期"32 位 frida-server"说法）**：MuMu 是 **x86_64 宿主 + houdini/libnb 转译 ARM32**（junqi 进程 = app_process32），但 **32 位 frida-server attach 转译进程会崩**（agent 注入干扰 houdini，attach+detach 后 2s 进程死；frida spawn 启动 1s 即死）。**MuMu 实测可用 = x86_64 frida-server（17.24.3）**，attach OK + 进程稳定。frida 17 API 变化：`Memory.readU32` 不存在 → 用 `NativePointer.readU32()`。**frida-server 位数 ≠ so 位数（转译环境下宿主桥接决定，跟 redroid"32 位 so→32 位 server"不同）**
   （下节 8.1 的"32 位 frida-server 跟 redroid 同套路"表已按此标注纠正）
2. **unidbg 的 `.got` 全是零值**——这是"缺真值"的根源：unidbg 模拟加载后 `.got`/`.data` 重定位未完整执行，
   所有经 GOT 间接的调用/数据读到 0 → 对象图零值 → 黑屏。**Frida 真机 dump 的 `.got` 真值正是 unidbg 缺的**。

### 8.1 为什么 MuMu 能直接用 Frida hook（不需要另搭环境）

MuMu 跟 redroid **同套路**——都是 **x86 宿主 + houdini/libnb 转译 ARM32**，所以 Frida hook 32 位 so 都用 **32 位 frida-server**：

| 环境 | 宿主 | ARM 转译 | Frida hook 32 位 so 用什么 | 军棋现状 |
|---|---|---|---|---|
| redroid | x86 容器 | houdini 转译 ARM32 | 32 位 frida-server（`frida-server-32`，已落地 `frida-dump/`） | 已用（读 18 地址 + dump .data/.bss） |
| **MuMu** | **x86_64 PC** | **houdini/libnb 转译 ARM32**（`/system/lib/libhoudini.so` + `ro.dalvik.vm.isa.arm=x86`） | **★ x86_64 frida-server（§124am-2 权威纠正，推翻早期"32 位同 redroid"说法）**：32 位 frida-server attach 转译进程崩（注入干扰 houdini），x86_64 版（17.24.3）attach OK 进程稳定 | **已建（§124am-2/3 frida 读/构造打通）** |
| 真 ARM 设备 | ARM 真机 | 不转译 | 跟 so 同位宽 frida-server（32 位 so → 32 位 server） | — |

**关键**：MuMu 是 **x86 PC + houdini 转译**，所以 32 位军棋 so 在 MuMu 上跑的是"转译后的 ARM32"，
frida hook 跟 redroid 完全同套路（32 位 frida-server + `frida attach com.cnvcs.junqi`）。**不需要另搭环境**——
MuMu 本身就是"真 GPU + 32 位转译 + 能 dlopen 军棋"的现成环境（§124u-z 已实测 32 位翻译体在 MuMu 渲染主菜单；§124ag 首帧清屏色）。

### 8.2 unidbg + Frida 一对一逐步对齐（3 角色分工）

| 角色 | 工具 | 干什么 | 军棋现状 |
|---|---|---|---|
| **模拟加载** | unidbg | 离线把 so 搬进 x86 + 分析调用链（§124q 判据）+ so 逻辑（1174/1174 PASS） | 加载全通（12/5312/214/6），卡 §124 三项 |
| **真实填充** | Frida（MuMu 真机 attach） | 读**运行时真值**（对象图构造后的 vector begin/end、渲染器对象 vtable=0x1595c0、纹理/顶点 buffer、18 常量、`.got` 真值） | 18 常量已静态固化（§124z-3）；**对象图真值还没 dump**（待办#5） |
| **一对一逐步对齐** | unidbg mock 值 vs Frida 真值 逐点比 | 把 unidbg 读 0 的对象图字段，逐个填上 Frida 真值 → 对象图对齐 | **这就是 §124ah 三层阻塞（对象图分歧=核心）的破题路** |

**逐步对齐流程（"一对一"的含义）**：
```
1. MuMu 跑原版军棋（32 位）→ 游戏"能玩"的状态（对象图构造完、渲染出帧）
2. Frida attach → dump 对象图真值（vector begin/end/cap、渲染器对象 vtable=0x1595c0、纹理/顶点 buffer、渲染对象 0x15be88 等）
   → 落 object_graph_values.json（跟 jni_runtime_values.json 同套路，但 dump 的是对象图字段，非 18 常量）
3. unidbg 侧（或翻译体影子内存）逐字段填真值：
   - 把 unidbg 读到 0 的 vector.begin/end 填上 Frida 真值
   - 把渲染器对象 vtable=0x1595c0 的构造链（0x23eec）喂真值
   - 把渲染对象 0x15be88、纹理 [this+48]、顶点 buffer 填上 Frida 真值
4. 比"unidbg 填真值后" vs "Frida 真机跑" → 逐字段对齐（调用链 + 对象图 + 返回值三判据，§124q）
5. 对齐 = 对象图对齐 = 棋盘破题（渲染器对象+纹理+顶点 buffer 构造出来 → 渲染出帧 → 能玩）
```

**★ unidbg `.got` 零值 → Frida 真值填充（用户观察，方案核心动机）**：
- unidbg 模拟加载后，`.got`/`.data` 重定位槽**未完整执行**（或读 0），所有经 GOT 的间接调用/数据读到 **0** →
  对象图零值（vector.begin=0/end=0、vtable=0、纹理指针=0）→ 黑屏。
- **Frida 真机 dump 的 `.got` 真值**（重定位后运行时的真实地址）+ 对象图字段真值，正是 unidbg 缺的——
  **一对一填进 unidbg/翻译体影子内存**，让"零值对象图"变"真对象图"。
- 例证（§124ag）：F_23eec 原 slot13 目标=垃圾（0x920fc=memcpy 中间），非 0x9d459(glClear)——
  这不是翻译器错，是**渲染器对象 vtable 槽存的是垃圾指针**（对象图未构造）。Frida dump 真值后填对即解。

**"一对一逐步对齐"的核心判据（§124q 4 要点 + 对象图）**：
1. **调用链先对齐**（bl 目标序列逐层下钻，unidbg 静态调用图 + Frida 真机 trace 双验证）
2. **对象图逐字段对齐**（vector.begin/end/cap、vtable 槽、渲染对象，unidbg mock 值 vs Frida 真值）
3. **每个 bl 子调用返回值 A/B 逐点比**（unidbg 返回值 vs Frida 真机返回值）
4. **so 内部 vs Java 交互边界**（bl 目标是 JNI 还是 so 内部，定函数边界）

**为什么 unidbg 单独做不到"数据侧"**（unidbg 短板，Frida 补上）：
- unidbg 跑到 0x43ed8 崩（§124 ② 对象图语义缺失）→ **读不出"对象图构造后"的运行时真值**
- unidbg 的 mock 值是"填 0 / 填固定值"，不是"真机跑出来的真值"
- **Frida 真机跑出来的对象图真值，是 unidbg mock 不了的**（"能玩"状态才有的真值）
- 所以 **unidbg（模拟加载 + 调用链 + 逻辑）+ Frida（真实填充对象图真值）= 一对一对齐**

### 8.3 跟棋盘破题方向的关系（§124z→§124ag/ah 演进）

**§124z 旧状态**：黑屏根因 = **渲染器对象（vtable 0x1595f4 当时认为）未构造**（渲染循环跑但 glClear dispatch 不到）。

**§124ag/ah 最新状态（v178ck）**：
- **§124ag 已锁定/修复一部分**：渲染器 vtable 基址 = **0x1595c0**（`0x1595f4 = 0x1595c0+52 = slot13 槽`，非基址），
  slot13=0x9d459 **F_9d458 glClear** 已确认 + 强制 dispatch 到渲染循环 → **首帧清屏色（蓝）达成**（绿/蓝双轮实证，诊断移除）。
- **§124ah 剩余阻塞（棋盘未画）**：
  1. **draw dispatch 未达**（F_23eec 仅 slot13，无 slot14/45/69）
  2. **对象图分歧（核心）**：渲染器对象 + 纹理 `[this+48]` + 顶点 buffer 未构造（F_9ef20 需 `[this+48]` 非零）
  3. **触摸→对局路径未翻译**（jqt_OnEvent/jqt_OnTouch 存根）
- **对象图真值 dump + 逐字段填**正是破题方向：
  - Frida 在 MuMu 跑"能玩"的军棋（原版 32 位 so）→ dump 渲染器对象构造链真值（vtable 槽 0x9d459、渲染对象 0x15be88、
    纹理 `[this+48]`、顶点 buffer、0x23eec 构造链）
  - unidbg/翻译体侧逐字段填 → 渲染器对象+纹理+顶点 buffer 构造出来 → 渲染出帧 → **棋盘破题**

**诚实边界**：
- Frida dump 的是"真机跑出来的对象图真值"，填进 unidbg/翻译体是"模拟真值"，**不等于"能玩"**（"能玩"要 GL 出帧 + 触摸进对局，仍需真机验证）
- MuMu 上 Frida attach 要 **32 位 frida-server**（跟 redroid 同套路），**不是 64 位**
- 对象图真值 dump 比 18 常量复杂（18 常量是 72B 固定地址，对象图是"构造链运行时才知道的真值"，需逆向对象图布局才能 dump 全）

### 8.4 MuMu 上 Frida 部署（跟 redroid 同套路）

```bash
# MuMu 是 x86_64 PC + houdini 转译 ARM32，跟 redroid 同套路
# 1. MuMu 装 32 位军棋（原版 libjunqi.so）
# 2. 推 32 位 frida-server 到 MuMu
adb -s <MuMu 地址> push frida-server-32 /data/local/
adb -s <MuMu 地址> shell "su 0 chmod +x /data/local/frida-server-32 && su 0 /data/local/frida-server-32 &"
# 3. 宿主 frida attach
frida -U com.cnvcs.junqi -l frida-dump/object_graph_dump.py   # dump 对象图真值
# 或 MuMu 用 redroid 容器时（localhost:5555）
frida -s localhost:5555 com.cnvcs.junqi -l frida-dump/object_graph_dump.py
```

**注**：MuMu 是 PC 模拟器（非 redroid 容器），frida-server 部署走 adb（跟真机同），不用 docker。

---

## 九、Stalker 指令级追踪模板（2026-11-03 从看雪 x1ny 银行加固实战文沉淀）

来源：看雪公众号文《GLM-5.3 辅助逆向实战：基于 Frida Stalker 对高防护加固样本的反调试链路溯源分析》。
军棋对应关系：军棋 so **无加固壳**，但 Stalker 模板（`android_dlopen_ext` 装载点 hook + 模块基址/大小过滤 +
30ms 批量输出 + 逐层上找）可直接迁到**真机 trace 渲染对象图构造链 / 14 JNI 入口内部 bl 序列 / 触摸→对局路径**
（§124ah 待办，unibg BL bug 未修期间的唯一真机动态路）。

### 9.1 军棋侧可直接复用的 5 个脚本套路（该文的"套路"本身，不针对其壳）

1. **dlopen 装载点做 hook 总入口**：`Interceptor.attach(Module.getExportByName("libdl.so","android_dlopen_ext"), ...)`，
   `onEnter` 读 `args[0]` 路径判断是否目标 so，`onLeave` 成功（非 null）后取 `Process.findModuleByName(...)` 的
   base/size，再挂后续 hook。军棋目标 = `libjunqi.so`（32 位，MuMu/redroid 上）。
2. **排除法定位元凶线程**：hook `pthread_create`，用 `Process.findModuleByAddress(startRoutine)` 判来源 so，
   `startRoutine.sub(mod.base)` 得偏移；`ALLOW_OFFSETS` 逐个放行 + 返回值 replace 0 假成功（线程不创建），
   观察 app 行为定位关键线程。军棋可用于定位"渲染线程/对局线程"。
3. **逐层上找检测/分叉源头**：从最终跳板函数 → 逐层上找"返回值决定分支"的函数 → hook 打印参数
   （`args[0..3]`）→ 静态反汇编/IDB 看上游 → 循环。军棋版：从"黑屏/未出帧"上找第一个返回异常值的渲染函数。
4. **30ms 批量输出 + 模块过滤**：`bufferLog` 数组 + `setTimeout/30ms` join 一次 IPC 输出；Stalker transform 里
   用 `instruction.address` 与 `mod.base`/`mod.base.add(mod.size)` 比较只输出目标模块指令，避免 ART/libc 刷屏；
   结束时 `Stalker.unfollow(tid)` + `Stalker.flush()`（尾部指令不丢）。
5. **`/proc/self/maps` 扫描型逻辑的通用绕过点**：该壳在 sscanf 解析 maps 行后、白名单过滤前有一个路径指针落
   栈点（`[sp+0x1E0]`），hook 该点把可疑路径改写为白名单路径即可让后续过滤放行。军棋无壳无 maps 扫描，
   但**"在两个判定函数之间找一个'中间数据落栈点'改数据"的手法**对对象图调试通用：在渲染器构造链的
   中间函数挂点改 `[sp+off]` 里的字段值，看下游 dispatch 是否恢复。

### 9.2 该文的 3 个技术坑（军棋 Stalker 脚本直接避）

- **SIGSEGV 型退出的捕获**：壳用 `mov sp,#0; mov lr,#0; br xN`（清 sp/lr 跳未映射地址）退出，
  不走 exit/abort，`Interceptor` 钩不到 → **必须 Stalker 看退出前最后几条指令**才回溯得到跳板函数。
  军棋真机 trace 若出现"线程突然死、无信号日志"，同法捕获。
- **`retval.replace(-1→0)` 只在 `onLeave` 生效**：`Interceptor.attach` 的 `onLeave` 里改返回值，
  `Interceptor.replace`（整函数替换）里改 NativeCallback 的 return。两个 API 别混用。
- **`android_dlopen_ext` 在 `libdl.so`**（新 ART 上 `dlopen` 经 `libdl` 走 `android_dlopen_ext`），
  老设备 `libdl` 可能不存在 → 军棋 32 位 so 在 MuMu（Android 12 级系统）上优先 `libdl.so`，
  找不到再 fallback `libc.so` 的 `dlopen`。

### 9.3 军棋版 Stalker 追踪渲染链（模板，待建 `frida-dump/stalker_trace.py`）

```python
# 目标：真机 MuMu 上 Stalker 跟踪 libjunqi.so 内指定偏移函数的执行指令，
#       用于 §124ah 渲染对象图/构造链真值采集（unibg BL bug 未修期间唯一真机动态路）
JS = r"""
var SO = 'libjunqi.so';
var TRACE_OFF = parseInt('%s');           // 目标函数偏移（32 位 so 内偏移）
Interceptor.attach(Module.getExportByName('libdl.so', 'android_dlopen_ext'), {
    onEnter: function (a) { this.name = a[0] ? a[0].readCString() : ''; },
    onLeave: function (ret) {
        if (this.name.indexOf(SO) === -1 || ret.isNull()) return;
        var mod = Process.findModuleByName(SO);
        if (!mod) return;
        var base = mod.base, end = mod.base.add(mod.size);
        var buf = [], tid = -1, timer = null;
        function flush() { if (buf.length) { send(buf.join('\n')); buf = []; } }
        var f = Interceptor.attach(base.add(TRACE_OFF), {
            onEnter: function (args) {
                send('>>> ' + SO + '+0x' + TRACE_OFF.toString(16) + ' arg0=' + args[0] + ' arg1=' + args[1]);
                tid = Process.getCurrentThreadId();
                timer = setInterval(function(){ flush(); }, 30);   // 30ms 批量 IPC
                Stalker.follow(tid, { transform: function (it) {
                    var ins = it.next();
                    while (ins) {
                        var a = ins.address;
                        if (a.compare(base) >= 0 && a.compare(end) < 0)
                            buf.push('0x' + a.sub(base).toString(16) + ' : ' + ins.mnemonic + ' ' + ins.opStr);
                        it.keep(); ins = it.next();
                    }
                }});
            },
            onLeave: function (ret) {
                if (timer) clearInterval(timer);
                Stalker.unfollow(tid); Stalker.flush(); flush();
                send('<<< ret=' + ret + ' (指令 ' + buf.length + ' 条已输出)');
            }
        });
        send('[*] Stalker hook installed @ ' + SO + '+0x' + TRACE_OFF.toString(16));
    }
});
"""
```

**使用约束**（同 §124ah 诚实边界）：
- MuMu 上 32 位 so → **32 位 frida-server**（x86 宿主 + houdini，见 §8.4），`frida -H 127.0.0.1:27042`
- 逐条 Stalker 会拖慢进程，只针对**单函数**短 trace（`onLeave` 自动 unfollow），不全程跟
- 产出喂给 unidbg/影子侧"对象图真值逐字段填"（§8.2），**不等于"能玩"**（"能玩"仍需真 GPU 设备验收）
- 该节模板只吸收"套路/手法"，不吸收原文的壳样本目标（银行 app 的 `libDexHelper.so` 偏移表）

### 9.4 文章 1（看雪课程）宣称的实测验证（2026-11-03）

课程卖点是"三大自研工具（自研 Frida 改版 / xiaojia-hide KPM 内核模块 / 魔改 LSPosed）内置最强过检测逻辑"。
逐条验证结果（**可验证部分已做，工具本体未获取**）：

| 宣称 | 验证 | 结论 |
|---|---|---|
| **自研 Frida 改版**（工具1） | 宿主 frida-tools **17.24.3**（pip）可连 `127.0.0.1:27042` 完整握手：`enumerate_processes()` 返回 **79 个 MuMu 进程**（zygote/system_server/surfaceflinger 全在）；宿主监听方是 `adb` pid=2539327（MuMu ADB 端口转发的 frida-server） | **可用基线验证通过**（frida 17 + MuMu）。"改版/隐藏"本体（改端口/线程名/SELinux 上下文）**未获取**（课程付费工具，开源替代 = AsenOsen/frida-stealth patch 集） |
| **xiaojia-hide KPM 内核模块**（工具2） | 宿主内核 6.17.0-14 + 头 6.17.0-14 匹配 → **KPM 探针模块编译成功**（`/tmp/kpm_probe/kpm_test.ko`，333KB，platform_driver probe/remove + seq_file maps 行过滤手法）+ **`insmod` 加载成功**（lsmod 可见 `kpm_test 12288 0`，dmesg 有 probe 日志；CapEff=000001ffffffffff 全 cap） | **编译侧+加载侧双重可复现**（KPM 核心手法=过滤 /proc/self/maps 行，M02 节）；**xiaojia-hide 本体未获取**（仓库搜不到；KPM 系是 bmax121/APatch 模块机制，宿主可编译加载=验证"KPM 路线在宿主内核可行"） |
| **魔改 LSPosed**（工具3） | LSPosed GitHub API 可达（release id 124462221，仓库正常）；**宿主无 Magisk/KernelSU 环境** → LSPosed 模块**无法在本机实际安装/加载** | **可获取性验证通过**（源码/发行版在），**运行环境验证失败**（无 root 手机/模拟器侧 Magisk）——LSPosed 是 Java Hook 层，军棋是 native 层，本身不在军棋主赛道 |
| **APatch**（课程 01 章 root 原理工具） | `curl bmax121/APatch`（搜索确认真实 org）仓库存在；**无 boot.img/ramdisk 操作环境**（宿主是 Docker 容器，无 fastboot） | **可获取性验证通过**，实操环境缺失 |
| **maps 扫描检测手法**（工具2 的检测面对手） | 宿主侧脚本复现文章 `sub_50450` 三类特征（`memfd:` / 非 dalvik 的 `[anon:*]` / 路径含 frida）扫 `/proc/self/maps`：纯 python 进程 **0 行可疑**（基线干净）；frida attach 后才会出现 memfd:/gadget 映射 → **检测逻辑本身可离线复现**（不需要课程工具） | **手法可复现，工具非必需** |

**军棋侧结论（诚实边界）**：
- 军棋 so **无壳无反调试**（AGENTS.md 已证 `.text`/`.rodata` 明文），课程"过检测"主打的 DexProtect/Promon/银行壳**与军棋不同赛道**——军棋不需要工具 1/2/3 的过检测逻辑
- 真正相关的是**方法**：①frida 17 + MuMu 真机 trace（§9.3 Stalker 模板已落地 `frida-dump/stalker_trace.py`）②KPM 观察面过滤 maps 行（宿主可编译加载验证）——**方法可吸收，付费工具本体不吸收**（¥4888 课程，军棋逆向不需要）
- 宿主 KPM 探针模块（`/tmp/kpm_probe/`）留作 §M02 手法参考；**军棋 32 位 so 无内核模块检测**（非银行级壳），KPM 在军棋场景**非必要**

### 9.4 文章 1（看雪课程）宣称的实测验证（2026-11-03）

课程卖点是"三大自研工具（自研 Frida 改版 / xiaojia-hide KPM 内核模块 / 魔改 LSPosed）内置最强过检测逻辑"。
逐条验证结果（**可验证部分已做，工具本体未获取**）：

| 宣称 | 验证 | 结论 |
|---|---|---|
| **自研 Frida 改版**（工具1） | 宿主 frida-tools **17.24.3**（pip）可连 `127.0.0.1:27042` 完整握手：`enumerate_processes()` 返回 **79 个 MuMu 进程**（zygote/system_server/surfaceflinger 全在）；宿主监听方是 `adb` pid=2539327（MuMu ADB 端口转发的 frida-server） | **可用基线验证通过**（frida 17 + MuMu）。"改版/隐藏"本体（改端口/线程名/SELinux 上下文）**未获取**（课程付费工具，开源替代 = AsenOsen/frida-stealth patch 集） |
| **xiaojia-hide KPM 内核模块**（工具2） | 宿主内核 6.17.0-14 + 头 6.17.0-14 匹配 → **KPM 探针模块编译成功**（`/tmp/kpm_probe/kpm_test.ko`，333KB，platform_driver probe/remove + seq_file maps 行过滤手法）+ **`insmod` 加载成功**（lsmod 可见 `kpm_test 12288 0`，dmesg 有 probe 日志；CapEff=000001ffffffffff 全 cap） | **编译侧+加载侧双重可复现**（KPM 核心手法=过滤 /proc/self/maps 行，M02 节）；**xiaojia-hide 本体未获取**（仓库搜不到；KPM 系是 bmax121/APatch 模块机制，宿主可编译加载=验证"KPM 路线在宿主内核可行"） |
| **魔改 LSPosed**（工具3） | LSPosed GitHub API 可达（release id 124462221，仓库正常）；**宿主无 Magisk/KernelSU 环境** → LSPosed 模块**无法在本机实际安装/加载** | **可获取性验证通过**（源码/发行版在），**运行环境验证失败**（无 root 手机/模拟器侧 Magisk）——LSPosed 是 Java Hook 层，军棋是 native 层，本身不在军棋主赛道 |
| **APatch**（课程 01 章 root 原理工具） | `curl bmax121/APatch`（搜索确认真实 org）仓库存在；**无 boot.img/ramdisk 操作环境**（宿主是 Docker 容器，无 fastboot） | **可获取性验证通过**，实操环境缺失 |
| **maps 扫描检测手法**（工具2 的检测面对手） | 宿主侧脚本复现文章 `sub_50450` 三类特征（`memfd:` / 非 dalvik 的 `[anon:*]` / 路径含 frida）扫 `/proc/self/maps`：纯 python 进程 **0 行可疑**（基线干净）；frida attach 后才会出现 memfd:/gadget 映射 → **检测逻辑本身可离线复现**（不需要课程工具） | **手法可复现，工具非必需** |

**军棋侧结论（诚实边界）**：
- 军棋 so **无壳无反调试**（AGENTS.md 已证 `.text`/`.rodata` 明文），课程"过检测"主打的 DexProtect/Promon/银行壳**与军棋不同赛道**——军棋不需要工具 1/2/3 的过检测逻辑
- 真正相关的是**方法**：①frida 17 + MuMu 真机 trace（§9.3 Stalker 模板已落地 `frida-dump/stalker_trace.py`）②KPM 观察面过滤 maps 行（宿主可编译加载验证）——**方法可吸收，付费工具本体不吸收**（¥4888 课程，军棋逆向不需要）
- 宿主 KPM 探针模块（`/tmp/kpm_probe/`）留作 §M02 手法参考；**军棋 32 位 so 无内核模块检测**（非银行级壳），KPM 在军棋场景**非必要**

---

---

*最后更新：2026-11-03（第八节"unidbg + Frida 一对一逐步对齐"更新到 §124ah：§124ag 真实 glClear 已 dispatch（vtable 0x1595c0 slot13，首帧清屏色蓝达成）；§124ah 棋盘三层阻塞（draw dispatch 未达/对象图分歧=核心/触摸→对局未翻译）；用户确认两前提：①MuMu hook 必须 32 位 frida-server（x86+houdini）②unidbg `.got` 全零=缺真值根源；18 值改静态固化在 jqt_mem.c g_jq_got_tbl；待办#5 更新；新增第九节 Stalker 指令级追踪模板（看雪 x1ny 银行加固实战文沉淀，dlopen 总入口/排除法元凶线程/逐层上找/30ms批量+模块过滤/maps 中间落栈点改数据 5 套路 + SIGSEGV 型退出捕获坑 + 军棋版 stalker_trace.py 模板）*

## 十二、.bss 全量 dump 对照基准（§124y-99/100/101 本轮固化）

> **新增对照基准**（2026-11 本轮）：.bss 16 关键槽 + 中间区非零比 + scudo 指针特征 +
> JNI 根对象槽归属，供后续 frida dump / unidbg 补运行时对齐时自动判读。

### 12.1 `bss_dump_align.py`（.bss dump 自动对照基准）
- 输入：.bss dump 28680B（0x7008 全段，`jq_bss_runtime_converted.bin` 或 `data_dump.py` 的 `redroid_bss.bin`）
- 输出：16 关键槽（低区 6 惰性 + 0x15c062 五元组 + 高区 7 构造器区 + locale 2 槽）逐个值 +
  中间区非零比 + scudo 堆指针特征计数 + 判读
- 判据：
  - 低区 6 槽非 0 = 惰性单例已构造 / 高区 7 区非 0 = .init_array 2-8 已跑 /
    中间区 ≥70% 非零 = 场景对象图构造态（MuMu 实测 87%）
- 诚实边界（FRIDA_NOTES §实锤不变）：.bss 整段回填翻译体**失败**（scudo 跨进程指针失效），
  dump 仅拿"值"作对照基准；翻译体需自身 F_9d3d4 构造（sp28-ROOT 已证可行）

### 12.2 MuMu .bss 运行时 dump 实锤值（jq_bss_runtime_converted.bin，对照基准样例）
| 槽 | .bss 偏移 | 运行时值 | 判读 |
|---|---|---|---|
| 0x15be80 | 0x0000 | 0xcf8d78f0 | 场景根对象槽（scudo 堆指针）非 0 ✓ |
| 0x15beb0 | 0x0030 | 0xf32dbb90 | F_3ab80①惰性构造 ✓ |
| 0x15beb8 | 0x0038 | 0xd08296f0 | F_3ab80⑥惰性构造 ✓ |
| 0x15bfd8 | 0x0158 | 0xecc44320 | F_5a104 容器池 ✓ |
| 0x15bfe4 | 0x0164 | 0xecb8c220 | 5 底层共用 ✓ |
| 0x15bfec | 0x016c | 0xd0829730 | F_3ab64 single ✓ |
| 0x15c062 | 0x01E2 | 0xc570f325 | 构造器 0 五元组 ✓ |
| 0x162554-0x162640 | 0x654-0x740 | 非 0 ×7 | .init_array 2-8 已跑（**修正 §124y-66/72 "清零"认知**：清零仅各对象特定字段 str 0,[r3+0/4/8]，整区运行时被 C++ 后续构造填非 0） |
| 0x162660/0x162668 | 0x7E0/0x7E8 | 非 0 ×2 | locale 构造 ✓ |

### 12.3 JNI 根对象槽归属（§124y-101 实锤）
- **14 JNI 入口（0x233e0-0x235d4）= 读 .bss 0x15be80 起点区（场景/游戏根对象槽）**：
  - 12 入口（OnEvent/OnInput/OnTouch/OnKey/OnBluetooth/OnUpdate/OnGraphCreate/OnGraphChange/OnResult/OnTimer/OnPause/OnResume）共用 **0x15be80**（根对象首指针）
  - OnCreate 读 **0x15be84**（+4，GLView 构造参数区）/ OnDestroy 读 **0x15be82**（+2，销毁标志区）
  - 然后 bl 内部 0xbdxxx C++ 处理函数（游戏逻辑/生命周期）
- **.rodata 0x138xxx 区 = PC 相对重定位 addend 区**（非字符串常量，§124y-100 误判已纠正）：
  - 各 JNI 入口的 `ldr rN,[pc,#K]` pool 值（0x138xxx）= addend，`add rN,pc` 后落 .bss 0x15be80 区
- **.bss 未构造（全 0）时 = 各 JNI 入口读根对象槽 0 → 内部 C++ 处理函数走空路径**
  （§124y-33/35 实锤 ok=0 路径，4 路径回归 PASS 即此路径）

### 12.4 待 dump 的运行时值（frida 对照基准，本轮累积）
- .bss 0x15be80 根对象槽的 scudo 块完整结构（场景管理器 C++ 对象图，中间区 87% 非零对应）
- .got 458 REL 槽中 100 个 .bss addend 的运行时 scudo 指针（§124y-92 实锤分布）
- 4800 .data.rel.ro vtable 槽完整重定位（2563 唯一 addend，92.3% .text，§124y-82 实锤）
- SharedPreferences 运行时值（`com.cnvcs.junqi.removead` / adHost / updateUrl 实际域名，§124y-86/87 去广告链）
- 74 事件名（72 Junqi.* + 2 Misc.*，§124y-88 实锤）与事件码的逐行映射（jq.game_test 钩子扩量触发）
- **事件表 17 行 × 17 字段位掩码表**（.bss 0x4704 区 = load_base+0x160584，§124y-109 实锤：
  .got[0x57c](0x15b644) REL 槽 addend 0x160584 → .bss 0x4704 C++ vector；外层 ip=0x121=289 字节
  = 行宽累计上限(17 行×17)，非 289 行；字段值 asrs#4 = [0x44] 事件码(非事件名前缀 asr4)
  匹配；MuMu dump 17 行全 asr4=0 = 事件码 0 匹配表，事件码 1-16 需 frida 触发实锤）
- 事件对象 [0x48] 匹配字段值（§124y-102 新实锤，asr4 = [0x44] 事件码）的运行时取值

## 十三、事件分发完整数据模型（§124y-30/48/88/102/109/112 全链固化）

> **定位**：.bss dump 对照基准（§十二）之上的"事件分发全链"数据模型，
> 供 frida dump / unidbg 补运行时对齐时按全链判读（非单点对照）。

### 13.1 事件分发全链（静态实锤，§124y-112 收表）

```
事件对象构造
  F_3432c 字符串构造 (.got 0x15b0c8+0x510 索引 → C++ 常量, §124y-89/90)
  + F_5a104 创建工厂 (bl F_855c4 allocator + .bss 0x15bfd8 容器池, §124y-38/76)
  + .bss 0x15be80 根对象槽 (14 JNI 入口共用读, §124y-101)
    ↓
事件表匹配
  F_3a92c (vtable 虚调用 blx [this+off], 0 直接 bl + 0 .data 槽, §124y-112)
  → .bss 0x4704 区 17×17 位掩码表 (.got[0x57c]=0x15b644 REL addend 0x160584, §124y-103/109)
  → [0x35] 类型 ≠ 0 → 17 行(事件码 0..16) × 17 字段, 字段 asrs#4 = [0x44] 事件码
  → 匹配成功 ret 1 / 全表无匹配 ret 0 (通配 0 跳过)
    ↓
事件码分发
  F_55d40 switch 42 code (§124y-30/31 实锤 codes 0-41+53-65)
  = 17 事件码 (0..16) × 子类型展开
  → 24 handler 忠实体 (jqt_vtbl.c, §124y-33/36)
    ↓
事件名 (74 条, §124y-88 修正版)
  72 Junqi.* (0x120e24-0x123364) + 2 Misc.* (RemoveAdFail/RemoveAdOk, 0x1233fc/0x123410)
  = 17 事件码 × 子类型的命名枚举 (多对一映射)
```

### 13.2 关键数据点对照基准（frida dump / unidbg 按此判读）

| 数据点 | 位置 | 静态实锤 | 运行时待实锤 |
|---|---|---|---|
| 事件表 17×17 基址 | .got[0x57c]=0x15b644 | REL addend 0x160584 | .got 槽 4 字节真值 |
| 事件表 17×17 内容 | .bss 0x4704 区 | 17 行全 asr4=0（事件码 0 表） | 事件码 1-16 行（frida 触发） |
| [0x44] 事件码 | 事件对象 | asrs#4 匹配事件码 | 运行时取值 0..16 |
| [0x35] 类型 | 事件对象 | ≠ 0 才匹配 | 运行时取值 |
| 14 JNI 根对象 | .bss 0x15be80 | 0xcf8d78f0（scudo，§124y-99） | scudo 块结构 |
| 事件表匹配器 | F_3a92c | dispatch offs[316] 已收 | vtable this 偏移 |
| [0x44] 直接事件码层 | 事件表 17×17 (.bss 0x4704) | 17 事件码 0..16, 字段 asrs#4 匹配 | 事件码 1-16 行 (frida 触发) |
| C++ 枚举事件类型层 | F_55700 switch (cmp 155) | 枚举边界值 155 (0x9b), .so stripped 无枚举名 | 枚举 155 对应事件类型 (frida 运行时值) |
| .got 100 槽 .bss addend | .got 0x15b0c8-0x15bb6c | 高区 74 (locale 堆) + 中间区 21 (C++ 对象堆) + 低区 5 (事件底层), §124y-128 | 100 槽 scudo 指针 (frida dump .bss 全量) |
| .bss 0x4704 事件表 | .got[0x57c]=0x15b644 → 0x160584 | 17×17 位掩码表, 17 行全 asr4=0 (事件码 0) | 事件码 1-16 行 + 事件表基址真值 |
| 0xbdxxx JNI 桥接层 7 函数 | .text 0xbd5xx-0xbdcxx | 8 函数 1 收表 (0xbe9cc OnBluetooth) + 7 未收表 (OnUpdate/OnGraph×2/OnKey/OnTimer/OnPause/OnResume), §124y-130 | 7 函数完整 C++ 逻辑 (JNI 桥接层, 非 vtable dispatch, frida 追踪) |
| 0xbdxxx vs F_3ab80 两层机制 | .text 0xbdxxx / .bss 单例槽 | JNI 桥接层 (读 .bss 0x15be80 根对象) vs 事件底层 (F_3ab80 填单例槽) 两层独立, §124y-130 | 桥接如何触发事件底层 (frida 调用链实锤) |

### 13.3 修正链（§124y-60→102→109 三轮误判记录，防复发）

| 轮 | 误判 | 修正 | 根因 |
|---|---|---|---|
| §124y-60/62 | 事件表 18×17 | §124y-102: ip=0x121 行数 | 未读 pool3=0x121 语义 |
| §124y-102 | 0x121(289) 行 × 17 | §124y-109: 17 行 × 17（ip=289 字节=行宽上限） | 0x121 误读为行数（实为字节累计上限） |
| §124y-106 | .bss 0x4704 非事件表 | §124y-108: dump 偏移对齐正确（dump[0]=.bss[0]） | dump 布局误判 |
| §124y-89 | 事件码 asr4=6（Misc） | §124y-89/112: 前缀 asr4 全=4（J/M 高 4 位均 4），[0x44] 事件码 ≠ 前缀 asr4 | 事件码与前缀 asr4 混淆 |
| §124y-120 | 155 = 事件码常量 | §124y-121: 155 = C++ 枚举事件类型码（非 [0x44] 直接码），事件分发两层编码 | 155 超 [0x44] 上限 16 + 事件名 74，是 enum 底层值 |

**防复发公式（§124y-105 经验区，本轮补强）**：
1. pc 相对读：`ldr rN,[pc,#K]` pool = 指令地址+4+K；`add rN,pc` pc = add 地址+4
2. 段归属必查：`so_probe.py seg <va>`（§124y-111 新 seg 子命令）
3. 行宽/行数区分：外层循环 `cmp r6,ip` 的 ip = **行宽累计字节上限**（非行数），行数 = ip/行宽
4. 事件码 [0x44]（事件对象字段）≠ 事件名前缀 asr4（字符串编码），勿混淆
5. C++ thunk 区 4 模式（§124y-117 实锤）：直接（bl+pop）/ 寄存器搬移（lsls r1,#2+ldr[r1,r0]，字段索引复用）
   / 带条件（cmp r2,#0+bne，子状态 2 路径）/ 多字段批量（ldr[r5,#0/4/8/12]×4，4 字段各调 handler）
   多 code 共用 1 handler = 同一 handler 的字段/子状态变体（非独立逻辑）
6. dump 文件偏移 = 段偏移对齐（§124y-108 实锤）：dump[0] = .bss[0]（段起点值核对），
   dump 偏移 0xNNNN = 段偏移 0xNNNN（dump 从段 0 开始，非全 so 偏移）
7. C++ 枚举编码 ×N 移位（§124y-123 实锤）：`movs r1,#K; lsls r1,#N` = 枚举值 K×2^N（如 155×2=310）
   是 C++ enum 2 字节/多字节对齐编码，读枚举值时须 lsls 还原（勿把编码后值 310 当直接枚举值 155）
8. .bss 区 0 REL 槽（§124y-71/125 实锤）：.bss 全段 0 REL（4800+458+42 全在 .bss 外），
   .bss 目标一律 NOBITS scudo 运行时填，静态读 = 全 0 / 读到 .comment 字节（§124y-32 坑），勿静态判值
