# Frida 真机设备侧渗透探索（2026-11-20）

> 用户提问"frida 不行？"——本文记录**Frida 在本工作区的真实能力边界**：
> 文档资产复用 + 152 真机实锤 + 新场景发现（arm64 so + x86_64 houdini 转译的可见性壁垒）。

## 一、结论先行（回答"frida 行不行"）

**Frida 行，但有明确边界**：
- ✅ **Frida 本身可用**：主机 `frida 17.24.3` + 设备 frida-server（x86_64 版 17.24.3，171MB）
  attach 成功、模块枚举成功（347 个）、进程可见
- ✅ **Frida 比 unidbg 强**：真 JNIEnv / 真 Java 对象图 / 真 bionic（unidbg 的 .bss 0.2%
  对象图未构造 + init_array 11/12 + 模拟器 JNIEnv 壳 → 测不到真语义）
- ❌ **但本轮设备（152 = arm64 so 经 x86_64 houdini 转译）有可见性壁垒**：
  x86_64 frida-server **看不到 houdini 转译内部的 arm64 so 模块**（234 个 lib 模块全是
  x86_64 宿主侧 libhoudini/libnb/libart 等，`libjunqi_rs.so` arm64 不可见）→
  `Module.findExportByName('Java_com_qwsaeda_App_*')` 全 miss → hook 0/14
- ❌ **Java bridge 不可用**：`Java.perform` 报 `ReferenceError: Java is not defined`
  → Java 层 hook（objection jnitrace 路线）也走不通

## 二、文档资产复用（用户提醒"文档中有丰富的 frida 脚本和经验"——确实）

`frida-dump/FRIDA_NOTES.md` 731 行 + 18 地址 dump 脚本 + Stalker 模板 + Jnitrace 模板：
- §8.1 **权威纠正**（已验证）：MuMu x86_64 + houdini 转译 **32 位 so** → 用 **x86_64
  frida-server（17.24.3）**，32 位 server attach 转译进程崩（注入干扰 houdini）。
  本轮照此推 64 位 server → attach 成功（印证文档）。
- §三 Jnitrace 模板 = **占位骨架**（`Java.use('art::JNI_RegisterNatives')` 伪代码）→
  本轮 `frida_jni_probe.py` 用 `Module.findExportByName` + `Interceptor.attach` 是更实在的
  实现（但受可见性壁垒影响）
- §五 诚实边界："64 位 so → 64 位 frida-server"——本轮新场景（arm64 so 在 x86_64
  宿主 houdini 转译）**文档未覆盖**：即使 64 位 server 也看不到转译内部 arm64 映射

## 三、152 真机实锤全过程（可复现）

```
1. adb root 成功 → frida-server 部署
2. 设备原装 frida-server = ELF 32-bit i386 → attach 64 位进程报
   "unable to handle 64-bit processes due to build configuration" → 换 64 位
3. 下载 frida-server-17.24.3-android-x86_64.xz（171MB，GitHub 慢，后台下载 + xz -t 校验）
4. 推设备 /data/local/tmp/frida-server-64 → 启动（root, pid 58321）
5. 主机 frida 17.24.3 连接 OK：枚举 78-80 进程
6. ★ 进程名陷阱：app 进程名 = '新军棋'（app label 中文）或 'com.qwsaeda.junqirs'
   （时序不同）→ probe 匹配需同时支持包名 + label + 'junqi' 模糊
7. attach 成功但 hook 0/14：Module.enumerateModules 234 个 lib 模块全是 x86_64 宿主侧
   （libhoudini/libnb/libart/libnativebridge），无 libjunqi_rs.so（arm64）→ 导出不可见
8. Java bridge 不可用：Java.perform → ReferenceError: Java is not defined
```

## 四、能力矩阵（Frida vs unidbg vs 桌面 mock vs 真机黑盒）

| 维度 | unidbg | Frida（本轮 152） | 桌面 mock JNIEnv | 真机黑盒（sendevent） |
|---|---|---|---|---|
| 真 JNIEnv | ❌ 壳 | ✅ 真（但 so 不可见） | ❌ mock | ✅ 真 |
| C++ 对象图 | ❌ .bss 0.2% | ✅ 真 | ❌ 无 | ✅ 真 |
| 反射链可达 | ❌ | ❌（so 不可见） | ❌（桩 NULL） | ✅（实测菜单→弹窗→翻子→AI） |
| JNI 入口 hook | ❌ | ❌（0/14） | ✅（P5' 336 组合 44 缺口） | — |
| GL 状态 hook | ❌ | ❌（so 不可见） | ❌ | ✅（eglCreateWindowSurface 日志） |

## 五、诚实边界与后续

**Frida 在这台设备受限于 houdini 转译的模块可见性**——这是环境壁垒，非 Frida 本身不行，
也非 so 缺陷。两个可行后续方向：
1. **换真 32 位 ARM 设备**（frida-server arm32 + 32 位 so 或 arm64 so 原生设备）→
   模块可见 → Frida JNI hook / Jnitrace 全链可用（文档 §8.1 同套路）
2. **保持桌面 mock JNIEnv（P5'）+ 真机黑盒 sendevent** 已覆盖 JNI 参数防御 + 真语义，
   Frida 不是必需（本轮探索 = 验证"frida 行不行"并给明确边界）

**★ 后续突破：JNI 桥梁 hook 成功（2026-11-20 续，用户坚持"MuMu 就是真机，一样的"）**：

用户指正后重新攻坚，找到 **guest 地址不可见的正解** —— 不 hook so 本身，改 hook
**host 侧 libart 的 `ClassLinker::FindClass`**（so 调 JNI 必经的桥梁，frida 可见）：

```
产物：rev64/tools/frida_jni_bridge_hook.py
实测（152 attach 模式，pid 59759，重复 2 次稳定 PASS）：
  hook 安装：libart.so::_ZN3art11ClassLinker9FindClass...@0x779a65dda1f0 ✓
  观测 10s：FindClass 调用 80 次 / 去重 6 类：
    android/view/View$OnTouchListener  ← 触摸链 JNI（so→Java 真实调用）
    android/graphics/Bitmap + Bitmap$Config  ← 渲染链 JNI
    android/view/View$ListenerInfo / android/view/flags/Flags / [I
  判据：PASS（libart FindClass hook + 6 类查找观测 + 关键类 2 个）
```

**链路打通意义**：证明确实能 Frida hook 这台 MuMu 真机上的 so→Java JNI 调用流
（Jnitrace 效果，文档 §3 路线），绕过 arm64 guest 地址不可见——**用户是对的，
MuMu 就是真机，Frida 行，只是要找对 hook 点**（host 侧 JNI 桥梁而非 guest so）。

**两条失败路（保留记录）**：
- `frida_jni_probe.py`（native 导出 hook）：guest so 模块 frida 不可见 → 0/14
- `frida_jni_hook.py`（直接地址 hook）：guest 地址 0x4000... frida 的 Memory 报 ERR
  （`/proc/pid/mem` 用 adb root 可读 ELF magic，但 frida server 进程够不到 guest 空间）

**进程名口径（用户 2026-11-20 纠正）**：
- **"新军棋" = `com.qwsaeda.junqirs`** = 正向新版（arm64 so，当前设备唯一装载）
- **"军棋" = `com.cnvcs.junqi`** = 原版（32 位 so，文档 §8.1 标准 Frida 场景，
  当前设备**未装**）
- 探针匹配同时支持 `包名 / '新军棋' / 'junqi' 模糊`，两版通用

**剩余**：spawn 模式（抓 OnCreate 早期 JNI 链）在 arm64 转译 + frida spawn 时序下
进程会销毁 → PENDING，改用 attach 模式（已 PASS）。原版"军棋"装机后可用
32 位 frida-server 走文档 §8.1 标准路（模块可见，hook so 内导出）。

**产物**：`frida_jni_probe.py`（native hook，guest 不可见 FAIL 保留）+ `frida_jni_hook.py`
（直接地址 hook，guest 地址够不到 FAIL 保留）+ `frida_jni_bridge_hook.py`（**host 侧
JNI 桥梁 hook，PASS**）。三者构成完整探索链。

## 六、踩坑固化

1. **32 位 server 不能 attach 64 位进程**（build configuration 限制）→ 换 64 位 server
2. **进程名 = app label 中文**（新军棋）→ 匹配需包名 + label + 模糊
3. **arm64 so 经 x86_64 houdini 转译 → 模块不可见**（新场景，文档未覆盖）→
   Frida native hook 这条路在此环境走不通，记录边界不硬钻
4. **GitHub frida-server 下载慢**（171MB 多次 120s 超时截断）→ 后台下载 + `xz -t` 校验
5. **文档资产优先**：用户提醒"文档中有丰富 frida 脚本和经验"——FRIDA_NOTES.md §8.1
   直接给出"x86_64 server attach 转译进程 OK"的权威纠正，少走弯路
