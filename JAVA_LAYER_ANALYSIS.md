# 原版军棋 APK Java 层（smali）逐类/逐方法分析

> 2026-11-20。回答"apk 层的 Java 代码分析了没有"——本文把原版 `com.cnvcs.*` 的
> Java 层（从 `军棋.apk` apktool 解出的 smali）逐类、逐方法分析，聚焦与 native
> `libjunqi.so` 的 JNI 契约、渲染驱动链、UI/蓝牙/广告壳。
> 数据源：`/tmp/opencode/jq_apk/dec/smali/com/cnvcs/*.smali`（原版 APK 解码产物）。
> 与 `jni_reversal_report.md`（native JNI 入口逆向）互补：那份分析 native 侧，本文分析 Java 侧。

## 执行摘要：Java 层的"业务逻辑"到底是什么？
**一句话：Java 层没有游戏业务逻辑——棋盘、规则、走子、吃子、AI、胜负全在 native so；Java 只是"壳 + 转发器"。**

| 问题 | 答案 |
|---|---|
| 棋盘/棋子状态在哪？ | native `libjunqi.so`（`.bss` 全局 `jq_state`），Java 侧**零**棋盘数组 |
| 走子/吃子/胜负规则在哪？ | native（14 JNI 入口 + 规则函数），Java 只把触摸坐标转给 `OnTouch` |
| AI 在哪？ | native（OnUpdate/OnTimer 驱动），Java Handler 只负责 100ms 周期的"闹钟" |
| Java 自己实现的逻辑有哪些？ | ① 生命周期桥 ② GLThread 渲染驱动（标准 GLSurfaceView 模板）③ 输入转发 ④ 辅助 UI：SiteView(WebView 网页) / InputDlg·TextDlg(文本输入弹窗) / AdLis(广告回调) / BtReceiver(蓝牙广播转发) |
| 有专门文档吗？ | 本文即 Java 层专门分析；native 侧见 `jni_reversal_report.md`；业务规则见 `rev64/forward/docs_rs/SPEC_BUSINESS.md` |

**实证**：App.smali 28 方法中 12 处 `invoke-static App;->On*`（转发 native），14 个 native 声明无方法体；SiteView/AdLis 对 App.native 调用 0 次（属独立 UI/广告逻辑，不碰游戏状态）；Java 侧搜不到棋盘数组/Canvas 游戏绘制（只有 GL 模板的 renderer 调用）。

## 0. 整体架构（Java 层是薄壳）

原版游戏核心逻辑全在 native `libjunqi.so`（14 个 JNI 导出，见 `jni_reversal_report.md`）。
Java 层只做四件事：
1. **生命周期转发**：Activity 生命周期 → native（OnCreate/OnPause/OnResume/OnDestroy/OnResult）
2. **渲染驱动**：GLView（标准 GLSurfaceView 模板）的 GLThread 每帧调 renderer.onDrawFrame → native OnUpdate
3. **输入转发**：onTouch → native OnTouch；onKeyDown/Up → native OnKey
4. **辅助 UI/广告/蓝牙**：SiteView（WebView 网页）、InputDlg/TextDlg（文本输入）、AdLis（百度广告回调）、BtReceiver（蓝牙广播 → native OnBluetooth）

**类清单**（com/cnvcs，按方法数）：
| 类 | 方法数 | 行数 | 职责 |
|---|---|---|---|
| GLView$GLThread | 19 | 1910 | GL 渲染线程（标准 GLSurfaceView 模板） |
| App | 28 | 326 | 主 Activity + 14 native 声明 + 生命周期/输入转发 |
| GLView | 34 | 678 | SurfaceView 本体 + API |
| GLView$EglHelper | 9 | 657 | EGL context/surface 创建与 swap |
| GLView$GLThreadManager | 9 | 384 | GLThread 单例管理 |
| SiteView | 5 | 447 | WebView 网页视图（官网/更新 URL） |
| InputDlg | 5 | 415 | 输入框对话框 |
| TextDlg | 5 | 406 | 文本展示对话框 |
| GLView$ComponentSizeChooser | 3 | 359 | EGL 配置选择 |
| GLView$BaseConfigChooser | 4 | 226 | EGL 配置基类 |
| AdLis | 7 | 73 | 百度广告事件回调（7 个 onAd*） |
| BtReceiver | 2 | 30 | 蓝牙广播接收 → native OnBluetooth |
| 其余 GLView$* / SiteView$N | — | — | access$ 合成访问器 / 内部监听器 |

---

## 1. App.smali 逐方法（28 个，逐条）

### 1.1 静态初始化与构造器
| 方法 | 行 | 逻辑 |
|---|---|---|
| `<clinit>` | 16-27 | `System.loadLibrary("junqi")` —— 加载 32 位 native so（唯一加载点） |
| `<init>` | 29-37 | 调 `Activity.<init>`，无业务逻辑 |

### 1.2 14 个 native 声明（39-79，均为 `public static native`，无方法体）
| 声明 | 签名 | native 侧入口（jni_reversal_report.md） |
|---|---|---|
| `OnBluetooth` | `(Intent)V` | Java_com_cnvcs_App_OnBluetooth @ 0x23424 |
| `OnCreate` | `(Activity)GLView` | @ 0x23560 |
| `OnDestroy` | `()V` | @ 0x235d4 |
| `OnEvent` | `(String,String)V` | @ 0x233e0 |
| `OnGraphChange` | `(II)V` | @ 0x23488 |
| `OnGraphCreate` | `()V` | @ 0x2349c |
| `OnInput` | `(String)V` | @ 0x233f4 |
| `OnKey` | `(KeyEvent)Z` | @ 0x2344c |
| `OnPause` | `()V` | @ 0x234c4 |
| `OnResult` | `(IIIntent)V` | @ 0x23408 |
| `OnResume` | `()V` | @ 0x234b0 |
| `OnTimer` | `()V` | @ 0x23460 |
| `OnTouch` | `(MotionEvent)Z` | @ 0x23438 |
| `OnUpdate` | `()Z` | @ 0x23474 |

### 1.3 生命周期/输入/渲染转发（逐方法）
| 方法 | 行 | 逐条逻辑 |
|---|---|---|
| `handleMessage(Message)` | 83-113 | 实现 `Handler.Callback`：① `OnTimer()` ② `removeMessages(0)` ③ `sendMessageDelayed(msg(0), 100ms)` —— **100ms 周期定时器**，驱动 native AI/动画 |
| `onActivityResult` | 115-127 | 转发 `OnResult(requestCode,resultCode,data)` |
| `onCreate(Bundle)` | 129-163 | ① super.onCreate ② `view = OnCreate(this)`（native 返回 GLView，可能为 null → Java 兜底）③ `view.setOnTouchListener(this)` ④ `view.setRenderer(this)` ⑤ 建 Handler(this) ⑥ `handleMessage(null)` 启动 100ms 定时链 |
| `onDestroy` | 165-187 | ① `removeMessages(0)` ② `OnDestroy()` ③ super ④ `System.exit(0)` —— **退出直接杀进程** |
| `onDrawFrame(GL10)` | 189-200 | `return OnUpdate()` —— 每帧驱动 native 更新+渲染（GLThread 调用） |
| `onKeyDown` | 202-228 | `OnKey(event)` 返 true → 消费返回 true；否则 super.onKeyDown（keyCode 透传） |
| `onKeyUp` | 230-256 | 同 onKeyDown（注意：else 分支误调 `onKeyDown` 而非 `onKeyUp`——原版小 bug） |
| `onPause` | 258-270 | super 后 `OnPause()`（存档 one.sjg） |
| `onResume` | 272-284 | super 后 `OnResume()` |
| `onSurfaceChanged(GL10,w,h)` | 286-298 | `OnGraphChange(w,h)` —— 视口尺寸同步 |
| `onSurfaceCreated(GL10,EGLConfig)` | 300-311 | `OnGraphCreate()` —— GL 上下文就绪通知 |
| `onTouch(View,MotionEvent)` | 313-326 | `OnTouch(event)`（返回值被丢弃）**恒返回 true** —— 触摸全消费 |

### 1.4 App 实现接口
`OnTouchListener`（onTouch）、`Handler.Callback`（handleMessage）、`GLView.Renderer`（onDrawFrame/onSurfaceCreated/onSurfaceChanged）——三者都把事件转给 native。

---

## 2. GLView 渲染驱动链（Java → native OnUpdate 的因果链）

GLView 是标准 GLSurfaceView 模板（Android SDK 的 GLSurfaceView 复制改造）。关键链：
```
App.onCreate → OnCreate(this) [native 建 GLView 并返回]
            → view.setRenderer(this) / setOnTouchListener(this)
            → GLThread.start()
GLThread.run (GLThread.smali:1615+):
  - :844  mRenderer.onSurfaceCreated(gl, config) → App.onSurfaceCreated → native OnGraphCreate
  - :879  mRenderer.onSurfaceChanged(gl, w, h)  → App.onSurfaceChanged  → native OnGraphChange
  - :892  mRenderer.onDrawFrame(gl)             → App.onDrawFrame       → native OnUpdate (每帧)
EglHelper (EglHelper.smali):
  - :99   createSurface(SurfaceHolder) → eglCreateWindowSurface
  - :567  swap() → eglSwapBuffers（每帧出屏）
```
`App.handleMessage` 的 100ms 定时链是**独立于 GL 帧**的第二条 native 驱动（OnTimer → AI/计时）。

**对逆向的意义**：native OnUpdate 被 GLThread 每帧调用（不是主线程），故 native 侧渲染线程与 UI 线程有竞态——这解释了 native 侧 `g_mutex`/EGL 加锁的必要性（见 jni_reversal_report.md §八）。

---

## 3. 辅助类分析

### 3.1 BtReceiver（蓝牙广播 → native）
`onReceive(Context,Intent)`（11-12 行）：**仅一行** `App.OnBluetooth(intent)` —— 蓝牙事件全部交给 native 处理，Java 侧不解析。

### 3.2 SiteView（WebView 网页视图）
- `create(Activity,url,title,w,h,bool)`（58-190）：init →  inflate 布局 → `findViewById` 取 TextView/Button 设标题/颜色/可见性 → `m_web.loadUrl(url)` + `requestFocus()`
- `destroy()`（191-250）：销毁 WebView
- `init(Activity)`（251-447）：静态初始化 WebView（setWebViewClient / JS 开关等）
- 调用点：native `.rodata` 明文有 `updateUrl/statHost/cfgHost` 等 URL，native 通过 JNI 回调 Java 的 SiteView.create 显示网页（版本更新/官网）。**这属于广告/更新通道，正向开发砍掉。**

### 3.3 InputDlg / TextDlg（对话框壳）
- 均含 `EditText`/`TextView` + "确定"(0x786e5a)/"取消"(0x53d6e6b5) 按钮
- `onTouch`/按钮点击 → `Editable.toString()` 取文本 → native `OnInput(String)` 回传
- native 侧 `FindClass("com/cnvcs/InputDlg")` 引用它们（弹窗让用户输入，如联机昵称）

### 3.4 AdLis（百度广告回调）
7 个 `onAd*` 回调（onAdClick/onAdClose/onAdFailed/onAdReady/onAdShow/onAdSwitch）：百度 SDK 广告事件 → 空实现/日志。**广告层，正向开发拆除目标。**

### 3.5 manifest 组件
- Activity：`com.cnvcs.App`（主，portrait）、`com.baidu.mobads.AppActivity`（广告）
- Receiver：BtReceiver（蓝牙）
- 权限：INTERNET/NETWORK/WIFI/STORAGE/PHONE/LOCATION/BLUETOOTH(_ADMIN) —— 广告+更新+蓝牙

---

## 4. 与 jni_reversal_report.md 的互补关系
| 维度 | jni_reversal_report.md | 本文 |
|---|---|---|
| 视角 | native so（14 入口汇编级） | Java smali（薄壳） |
| OnCreate | native 内部建 GLView 的 0x23560 汇编流程 | Java onCreate 如何调它、拿返回值、setRenderer |
| OnUpdate | native 0x23474 汇编 | GLThread 每帧 onDrawFrame 的驱动链 |
| OnTimer | native 0x23460 汇编 | Handler 100ms 定时链 |
| OnBluetooth | native 0x23424 汇编 | BtReceiver.onReceive 一行转发 |

两份合起来 = 完整的 Java↔native 边界图。

## 5. 诚实边界
- Java 层是**薄壳**，游戏逻辑 100% 在 native——本文逐方法分析了 App 的 28 个方法 + 核心辅助类，但 GLView 的 GLSurfaceView 模板（GLThread 1910 行 / EglHelper 657 行）只分析了渲染驱动三角（onSurfaceCreated/Changed/onDrawFrame）和 EGL surface/swap，未逐行分析模板的每个 wait/notify 状态机（那是 Android SDK 标准代码，无业务逻辑）。
- SiteView/InputDlg/TextDlg/AdLis 属广告/更新/输入通道，正向开发（人机对战范围）**砍掉**，本文分析仅作证据链存档。
- 原版 `onKeyUp` 的 else 分支误调 `onKeyDown` 是原版 smali 事实（未修正），本文如实记录。

## 6. 可复现验证
```bash
# 原版 APK 解码（若 /tmp 已清）
apktool d -f 军棋.apk -o /tmp/opencode/jq_apk/dec
# App 方法数/签名
grep -c '^\.method' /tmp/opencode/jq_apk/dec/smali/com/cnvcs/App.smali   # 28
grep '^\.method.*native' /tmp/opencode/jq_apk/dec/smali/com/cnvcs/App.smali  # 14 个 native
# 渲染驱动三角
grep -n 'onDrawFrame\|onSurfaceCreated\|onSurfaceChanged' /tmp/opencode/jq_apk/dec/smali/com/cnvcs/GLView\$GLThread.smali
```
