# 附录 A：安卓游戏开发基础（逆向视角速查，军棋 `libjunqi.so` 对照）

> **定位**：unidbg 课程是"逆向"课（把 so 跑起来 + 补环境 + 反检测），**缺"正向"知识**——
> 不知道"正常游戏怎么渲染/怎么跑"，就理解不了"为什么 mock 错就黑屏"。
> 本附录补"安卓游戏开发基础"（**逆向视角**：只讲跟"理解 so 行为/补环境"相关的部分，不是完整游戏开发教程）。
> **跟 31 章的关系**：31 章讲"怎么逆向 so"，附录 A 讲"so 本身是个什么东西"（安卓游戏的 GL 渲染/游戏循环/Activity 生命周期）。
> **军棋对照**：每节标注军棋 `libjunqi.so` 实际用到的（17 个 GL 函数 + GLView 渲染入口 + 7 分辨率资源目录）。

---

## A.1 安卓 App 的生命周期（Activity → so 加载 → 渲染）

**逆向视角**：so 是被"App 生命周期"触发的——`Activity.onCreate` → `GLView`（`GLSurfaceView`）创建 → `dlopen so` → so 的 `OnCreate` 被调。
**不懂生命周期，就不知道"so 什么时候被加载 + 渲染循环什么时候起"**（军棋 §124z 的"渲染循环跑但 glClear 类对象未构造"黑屏，根因就在这条链上）。

```
Activity.onCreate
  → setContentView(GLView)          ← GLView 是 GLSurfaceView 子类
  → GLView 起 GLThread（独立线程）  ← 渲染循环在 GLThread 跑，不在主线程
  → GLThread.onSurfaceCreated       ← EGL 初始化
  → GLThread.onDrawFrame            ← 渲染循环（每帧调 so 的 OnUpdate/OnGraphChange）
  → dlopen libjunqi.so             ← so 在 GLThread 第一次 onDrawFrame 时加载
  → so OnCreate(env, GLView obj)    ← 军棋 14 入口之一（创建渲染对象）
  → so OnUpdate/OnGraphChange       ← 渲染循环每帧调（走子/刷新棋盘）
```

**军棋对照**：
- `com.cnvcs.App` 的 `onCreate`（Java）→ `App.OnCreate(Activity)`（native，军棋 14 入口之一）→ 返 `GLView`
- `GLView` 是 `GLSurfaceView` 子类（`GLView$GLThread` = 渲染循环线程，军棋 smali 里 `GLView$GLWrapper` 接口）
- `setRenderer(App)`（`App implements GLView$Renderer`）→ GLThread 起渲染循环
- **so 在 GLThread 跑**（不是主线程）——这就是军棋 §124z 的"GLThread 25 正常（阻塞 Object.wait）但 GL 区纯白"：渲染循环在跑，但 glClear 类对象未构造 → 出不了帧

**"为什么补环境要 mock `GLView`"**（跟 ch11 DalvikVM 呼应）：
- so 的 `OnCreate` 返 `GLView`（Java 对象），unidbg 要 mock 这个对象（`newObjectV(GLView)` 返替身）
- 不懂"GLView 是什么"（渲染线程的视图），就不知道"OnCreate 返 null 会导致渲染循环起不来"

---

## A.2 OpenGL ES 渲染管线（军棋 so 调的 17 个 GL 函数）

**逆向视角**：so 调的 GL 函数 = "渲染管线各阶段"。不懂管线，就不知道"为什么 mock 错某个函数就黑屏/白屏"（军棋 §124z 的"glClear 类对象未构造 → 场景黑"就是渲染管线第 1 阶段没跑）。

**OpenGL ES（移动端 GL）渲染管线 5 阶段**：
```
① 清屏（glClear）         ← 把帧缓冲清空（军棋 glClear/glClearColor）
② 设置视口（glViewport）   ← 定义"屏幕哪块区域画"（军棋 glViewport）
③ 画几何（glDrawArrays）   ← 画三角形/矩形（军棋 glDrawArrays，画棋盘格子）
④ 画纹理（glBindTexture + glTexImage2D）← 把图片贴到几何上（军棋 棋子/士兵纹理）
⑤ 显示（eglSwapBuffers）   ← 把画好的帧送到屏幕（军棋 GLThread 每帧调）
```

**军棋 so 实际调的 17 个 GL 函数**（按管线阶段归类，来自 `jqt_vtbl.c` PLT 真实化）：
| 阶段 | 军棋 GL 函数 | 大白话 | 补环境 mock |
|---|---|---|---|
| **① 清屏** | `glClear` / `glClearColor` | 把帧缓冲清成某颜色（清成黑 → 场景黑） | 返 0（挂桩） |
| **② 视口** | `glViewport` | 设"画哪块"（720×... 像素） | 返 0 |
| **③ 几何** | `glDrawArrays` / `glEnable` / `glDisable` / `glFlush` | 画三角形 + 开/关混合 + 刷显存 | 返 0 |
| **④ 纹理** | `glGenTextures` / `glBindTexture` / `glTexImage2D` / `glTexSubImage2D` / `glDeleteTextures` / `glTexParameterf` | 生成/绑定/上传/更新/删纹理 + 设纹理参数（棋子图） | 返 0 |
| **⑤ 查询** | `glGetIntegerv` / `glGetString` | 查 GL 参数/版本串（so 做反检测/选资源时用） | 返固定值 |

**"为什么 mock GL 函数返 0 就能'不崩'但'黑屏'"**（跟 §124z 黑屏呼应）：
- 军棋翻译体把 GL 函数**挂桩**（`glClear` 等返 0，不真画）→ 不崩（GL 调用不会段错误）
- 但**不真画 = 没有像素送到屏幕 = 黑/白屏**（"运行不崩 ≠ 能渲染"，§124z 诚实边界）
- **要"出帧"**：要么真机真 GPU（MuMu 的 Adreno 640 能真画），要么翻译体把 glClear 类对象构造出来（§124z 下一步）

**"纹理（`glTexImage2D`）跟军棋棋子的关系"**：
- 军棋棋子（军旗/炸弹/地雷/工兵）是**图片纹理**（`datah` 212×280 图集，4 列×7 行，53×40/格）
- 渲染时 `glGenTextures`（建纹理槽）→ `glBindTexture`（绑定）→ `glTexImage2D`（上传棋子图）→ `glDrawArrays`（画到棋盘格）
- **不懂纹理，就不知道"棋子是图片，不是代码画出来的"**（跟 ch19 SPAK 资源解密呼应：棋子图集在 `datah` 压缩包里，SPAK 解包才能拿到）

---

## A.3 游戏循环（Game Loop：update + render）

**逆向视角**：游戏是"循环"跑的（不是"跑一次就结束"）。**不懂循环，就不知道"so 的 OnUpdate/OnTimer 为什么反复被调"**（军棋 §124z 的"渲染循环跑但 glClear 类对象未构造"，根因是循环里的某个对象没建出来）。

```
while (running) {
  update();     ← 游戏逻辑（走子/AI/碰撞检测，军棋 so 的 OnUpdate/OnGraphChange）
  render();     ← 渲染（画一帧，军棋 so 的 GL 函数）
  wait();       ← 等下一帧（60fps = 每 16ms 一帧）
}
```

**军棋对照**：
- `GLView$GLThread` = 游戏循环线程（`onDrawFrame` 每帧调 so 的 `OnUpdate`/`OnGraphChange`）
- **so 不是"跑一次"**，是"GLThread 每帧反复调"——这就是为什么"对象图没构造"会在**循环里反复触发**（§124z 的"GLThread 活跃但黑屏"）
- **"OnTimer"入口**（军棋 14 入口之一）= 游戏循环里的"定时事件"（比如 AI 走子的 70 步计时，ch20 的"70 步/1000 步判和"）

**"为什么 unidbg 模拟'游戏循环'很难"**：
- unidbg 是"调一次 OnCreate"（静态快照），**不是"跑循环"**（动态反复调）
- 军棋 so 的对象图是"循环里逐步构造"的（OnCreate 建部分，OnUpdate 建渲染对象），**unidbg 调一次 OnCreate 读不到"循环跑完后"的对象图**
- 这就是"unidbg 数据侧不行"（ch13 13.4c 的"unidbg + Frida 一对一对齐"破题方向）——Frida 真机跑循环，dump"循环跑完"的对象图真值

---

## A.4 安卓分辨率 / 资源目录（军棋 7 个 `data/{分辨率}/`）

**逆向视角**：so 按"屏幕分辨率"选**资源目录**（不同分辨率用不同大小的贴图）。**不懂分辨率选资源，就不知道"为什么 mock 错分辨率 → 选错资源目录 → 渲染崩"**（军棋 ch24 24.4 的"补设备分辨率"）。

**军棋 7 个分辨率资源目录**（AGENTS.md 红线，so 按屏宽选）：
```
data/320/     ← 320 宽屏（小屏手机）
data/480/     ← 480 宽屏
data/480x/    ← 480 宽屏（横屏变体）
data/720/     ← 720 宽屏（军棋 unidbg mock 固定选这套）
data/720s/    ← 720 宽屏（小字变体）
data/800/     ← 800 宽屏（平板）
```
**so 怎么选**（军棋 `OnCreate` 实测，ch24 24.4）：
```
so 读 getDefaultDisplay().getWidth()（分辨率）
  → 按屏宽匹配 7 个目录之一（比如 720 宽 → data/720/）
  → 加载 data/720/{棋盘图/棋子图集/字体/音效}
```
**补环境 mock**（ch24 24.4）：
- 模拟时固定选一套（比如 720），so 就选 `data/720/` 这套资源
- **选哪套不影响"能玩"**（资源内容一样，只是分辨率不同）——但 **mock 错 → 选错目录 → 渲染崩**（ch24 难点 2）

**"为什么 mock 分辨率 = 选资源目录"**（跟 A.2 纹理呼应）：
- 棋子图集在 `data/{分辨率}/datah`（SPAK 压缩），分辨率不同 → 图集尺寸不同
- 720 宽的图集跟 320 宽的不一样（720 更清晰），**选错 = 画出来的棋盘模糊/错位**

---

## A.5 JNI 入口 vs 游戏功能（军棋 14 入口的"游戏语义"）

**逆向视角**：军棋 14 个 JNI 入口不是"随便 14 个函数"，是**"游戏功能 → Java 回调"的映射**。不懂每个入口"对应游戏什么功能"，就不知道"调哪个入口能触发什么"（军棋 14 入口在 ch02 讲"怎么调"，本附录讲"调了会干什么"）。

**军棋 14 入口的游戏语义**（readelf 实测 + 逆向推断）：
| 入口 | 游戏语义 | 触发时机 | 军棋逆向点 |
|---|---|---|---|
| `OnCreate` | 开局（建棋盘 + 渲染对象 + 读配置） | Activity.onCreate | §124 卡 0x43ed8（对象图） |
| `OnDestroy` | 收局（清理对象） | Activity.onDestroy | 生命周期收尾 |
| `OnResult` | 对局结果（胜负判定） | 一方被将死/判和 | 走子规则（ch20） |
| `OnEvent` | 事件（布局选择/认输/求和） | 玩家点菜单 | `OnEvent("layout","0-11")` 选布局 |
| `OnUpdate` | 刷新（走子后更新棋盘） | 每帧/走子后 | 渲染循环（A.3） |
| `OnKey` | 键盘输入 | 按键 | — |
| `OnInput` | 输入 | 文本输入 | — |
| `OnTouch` | 触摸（点格子走子） | 玩家点棋盘 | 走子判定 0x39e14（ch20） |
| `OnGraphChange` | 棋盘变化（走子/吃子后） | 走子触发 | 渲染对象更新（§124z 黑屏点） |
| `OnBluetooth` | 蓝牙联机 | 联机同步 | UUID `ce1404fe-...` + v2 帧协议 |
| `OnPause` | 暂停（存对局存档） | 切后台 | 写 `one.sjg`（ch16） |
| `OnGraphCreate` | 棋盘创建 | 开局 | 建 60 格 38 子棋盘 |
| `OnTimer` | 定时（AI 走子计时） | 游戏循环 | 70 步判和（ch20 规则） |
| `OnResume` | 恢复 | 回前台 | 读 `one.sjg` 续局 |

**"为什么懂入口语义对逆向有用"**：
- **选哪个入口测**：想测"走子规则"→ 调 `OnTouch`（点格子）+ `OnGraphChange`（看棋盘更新），不是 `OnCreate`
- **为什么调 `OnCreate` 崩**：`OnCreate` 是"建对象"入口，崩在"建对象时对象图未初始化"（§124 ②），跟"走子规则"无关
- **§124z 黑屏跟"渲染入口"的关系**：渲染入口 = `OnGraphChange`/`OnUpdate`/`OnTimer`（循环里反复调），黑屏根因是这些入口里的"glClear 类对象未构造"（A.3 游戏循环 + A.2 渲染管线）

---

## A.6 安卓逆向 vs 游戏开发的"知识边界"（本附录讲什么/不讲什么）

**本附录只讲"跟 so 逆向相关"的安卓游戏开发知识**（"理解 so 行为 + 补环境 mock"够用），**不展开完整游戏开发**：

| 讲（跟逆向相关） | 不讲（完整游戏开发） |
|---|---|
| Activity 生命周期（so 何时加载） | UI 设计/布局/动画（军棋用不到 UI 逆向） |
| GL 渲染管线 5 阶段（so 调的 17 函数） | 复杂 3D shader/光照/后处理（★ §124zz-2 纠正：军棋原版 draw 走 **GLES2 顶点属性路径**——`glVertexAttribPointer×3 + glDrawArrays(GL_TRIANGLE_STRIP)` + 纹理对象自带绘制方法，**非纯 GLES1 立即模式**；只是不用 3D 光照/后处理 shader） |
| 游戏循环（so 反复调的原因） | 物理引擎/碰撞检测算法（军棋走子是规则判定，非物理） |
| 分辨率/资源目录（mock 选资源） | 资源打包格式细节（SPAK 在 ch19 讲） |
| JNI 入口 vs 游戏功能（14 入口语义） | Java 业务逻辑（军棋核心逻辑在 native，Java 层只有 7 类） |

**"为什么附录不讲完整游戏开发"**：
- unidbg 课程是"逆向"课（把 so 跑起来），不是"开发"课（写游戏）
- 附录只补"理解 so 行为"需要的**最小知识集**（生命周期/GL 管线/循环/分辨率/入口语义），多了是"开发知识"不是"逆向知识"
- **军棋是 2D 棋盘游戏**（走子/吃子规则 + 图片纹理），不用**复杂 3D shader（光照/后处理）/物理/音效引擎**——比"3D 游戏"简单得多，附录只讲军棋用到的。**★ 精确口径（§124zz-2 详细笔记实证）**：原版 draw 函数（0x9e760）实测 GL 序列 = `glVertexAttribPointer ×3（2/4/2 分量）→ glUniform1i ×2 → glBindTexture → glDrawArrays(mode=4=GL_TRIANGLE_STRIP)`——**原版走 GLES2 顶点属性 + 纹理对象绘制方法路径，不是"不用顶点/几何绘制"的纯立即模式**；"不用 shader"应理解为"不用 3D 光照/后处理 shader"，**不是"不用顶点属性/几何绘制"**

**跟 31 章 + 4 本书的关系**：
- 31 章讲"怎么逆向 so"（unidbg 加载/补环境/反检测）
- 4 本书（ELF_FORMAT_SUPPLEMENT.md）讲"二进制分析工具/格式"（ELF/重定位/加壳/插桩）
- **附录 A 讲"so 本身是什么"（安卓游戏的 GL/循环/生命周期）**——三者互补：
  - 不懂 A（游戏开发），就不知道"so 为什么调 glClear"（理解目标行为）
  - 不懂 4 本书（二进制工具），就不知道"so 的 ELF 段怎么读"（分析 so 本身）
  - 不懂 31 章（unidbg），就不知道"怎么把 so 跑起来"（执行 so）

---

## A.7 军棋 `libjunqi.so` 游戏开发知识速查表（逆向对照）

| 逆向目标 | 需要的游戏开发知识 | 本附录落点 |
|---|---|---|
| 理解"so 何时加载" | Activity 生命周期 + GLView 创建 | A.1 |
| 理解"so 调 glClear 干嘛" | GL 渲染管线 5 阶段 | A.2 |
| 理解"so 为何反复调 OnUpdate" | 游戏循环（update + render） | A.3 |
| 理解"so 选哪个资源目录" | 分辨率 + 资源目录 | A.4 |
| 理解"调哪个入口测走子" | 14 入口的游戏语义 | A.5 |
| "为什么 mock 错就黑屏" | A.1 + A.2 + A.3 组合 | §124z 黑屏根因（glClear 类对象未构造） |
| "为什么 unidbg 数据侧不行" | 游戏循环 + 对象图逐步构造 | A.3 + ch13 13.4c（unidbg+Frida 一对一对齐） |

**一句话**：附录 A 补"so 本身是什么"（安卓 2D 棋盘游戏的 GL 渲染 + 游戏循环 + Activity 生命周期 + 14 入口语义），**跟 31 章（怎么跑 so）+ 4 本书（二进制工具）互补**——三者合起来才是"完整理解军棋 so + 能逆向"的知识图谱。

---

*最后更新：2026-11-03（新建附录 A：安卓游戏开发基础逆向视角速查，7 节——Activity 生命周期 / GL 渲染管线（军棋 17 GL 函数）/ 游戏循环 / 分辨率资源目录 / 14 入口游戏语义 / 知识边界 / 速查表；跟 31 章 + 4 本书互补）*
