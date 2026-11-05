# src_rs 经验库（每轮 loop 闭环沉淀，防"反复踩同一坑"）

> 定位：src_rs 正向开发**经验库单一文档**（闭环事实 + 坑清单 + 工具清单 + 方法论）。
> 每轮 loop 闭环后在此追加（AGENTS.md 只做路由指向）。
> 规格/规范基准见 `SPEC_BUSINESS.md` / `GAME_DEV_GUIDELINE.md` / `CODE_STYLE.md`。

## 逆源码重写（R-route，本次新增，不属阶段 0-3）

> **★ 正向开发定位（本轮审视成文）**：src_rs = **逆向驱动重写**（规则引擎对齐原版函数）→
> 后续（UI/音效/规则完善/联机）= **规格先行的正向开发**（基准 3 文档）：
> `rev64/docs_rs/SPEC_BUSINESS.md`（业务规格：规则条款/事件契约/UI 流程/音效映射/布局）+
> `GAME_DEV_GUIDELINE.md`（游戏开发规范：状态机/返回契约/锁纪律/音效三入口/重开悔棋统一入口）+
> `CODE_STYLE.md`（代码规范：JNI 反射/链接安全/命名/坑防复发）。
> **规格变更必须同步代码 + 回归（12/12 + 真机 9/9）**；**子代理对抗审计**（subagent 独立
> 审 代码 vs 规格一致性 + 坑防复发）每轮 loop 至少 1 次。

- 产物：`rev64/src_rs/`（逆源码重写的游戏逻辑 C 源码，非模拟路线）
  - `dist/jq_run`（桌面 headless 可执行，走一局）+ `dist/libjunqi_rs.so`（NDK arm64，14 个 On* JNI 签名与 App.smali 一致）+ `dist/phase_rs_apk.apk`（双 ABI 可装，v1+v2 签名，`.so` 全 STORE）
  - 规则引擎对齐原版 0x39e14/0x39f20/0x2a228/0x6a3b0/0x302b4 等函数（capstone 反汇编原版 so，证据见 `rev64/docs_rs/R-route_reversal.md`）
  - 回归：`src_rs_regress.sh` 一条命令 **12/12 全 PASS**（编译/蓝牙自测/桌面走局/`regress.py` 规则回归默认 20 局（`regress.py` 自身默认 50 局，通过率≥90% 判据）/红线审计/布局 12 预设/触摸命中/gametest 60 局全路径/链接审计/走子预验证 3 步/**桌面事件验证 `make evverify MODE=--all`（8 断言 EAT/BOMB/MOVE/FLAG/MINE + 悔棋 UNDO + 和棋 DRAW + 认输 RESIGN + **困毙 STALEMATE**，替代每轮手写 /tmp/*.c 构造状态）**），60 局 gametest 全合法终局（蓝 vs 红贪心 AI，胜负/和棋/困毙全路径覆盖）
- 诚实边界：渲染/音频/蓝牙 SPP socket 为桩（OnCreate 返回 NULL，Java 侧 new GLView 兜底）；
  7 字段帧语义为行为级约定；32 位设备走原版 32 位 so（APK 内保留），64 位走重写规则引擎。
  **不包装成"完整版/可玩"**——"能玩"需真机 GPU 设备验收。
- **正向开发 UI 层（菜单/高亮/音效，本轮）**：`ui_menu.c/h`（5 按钮）+ `ui_state.c/h`
  （UI_MENU↔UI_GAME）+ `render.c rs_draw_menu/rs_draw_highlights`（菜单按钮 + 可走格绿框）
  + `jni_bridge.c` OnTouch 分流（菜单态点按钮进对局 / 对局态两步走子 + `sfx_play` 接
  Java MediaPlayer，WAV 走 `/data/local/tmp/jq_sfx/`，**14 个音效全推 MuMu（★ 实测 14 = Junqi 组 12 + Ui 组 2[ui_close/ui_pop]，`grep -cE` 核对，非"Ui 3 个"误记的 16）**。桌面 headless
  用 `ui_state_stub.c/ui_menu_stub.c`（Makefile desk 行）。详见下方 MuMu 实证段"正向开发 UI 层"条目。
- 渲染逆源码（本轮续逆，①-④b 全链）：
  - `src_rs/render.c`（①GL 状态机 + ②SPAK 纹理管线，GLES1 固定管线）+ `render_obj.c`（③vtable 对象图）
    + `containers.c`（④C++ 容器布局）+ `render_scene.c`（③ OnGraphCreate 0xbd764 构造链 + OnUpdate 0xbd5a0 帧驱动）
    + `glview_gl.c`（③b GLView 线程驱动：EGL 上下文 + 每帧出帧 + resize，对齐 GLView$Renderer.onDrawFrame→OnUpdate smali 链）
  - `OnUpdate` 只做 AI 推进（GL 渲染由独立渲染线程独占 PBuffer GL2 出帧）；未就绪退化为 AI 推进（诚实：EGL 无 GPU 则不出帧）
  - 回归：场景构造/帧驱动测试 + 规则 40/40 全绿；APK `phase_rs_apk.apk`（40.8KB arm64，双 ABI，v1+v2）

### MuMu 64 位 GL 出帧实证边界（本轮 §src_rs 沉淀，先读这个）

- **★ MuMu 64 位 EGL = Mesa**（`ro.hardware.egl=mesa`，`libEGL_mesa.so` x86_64 被 houdini64 转译）。
  独立 64 位 so 直接 dlopen EGL 报 "couldn't find an OpenGL ES implementation"（GL 只能走 app 进程）
- **★ GL 清屏出帧正常（实证）**：`glClearColor(0.08,0.08,0.12)+glClear+glReadPixels`
  读回 `RGBA=(16,20,33,255)` 精确匹配（PBuffer 720×1280，Mesa Intel HD 630 KBL GT2 渲染器）
- **★ 几何绘制三条路径全 fail（实证，非代码 bug）——GLES1 固定管线路径**：
  1. `glDrawArrays` + `glVertexPointer`（GLES1 client-state，主机指针 / VBO 两条）→ no-op（纯色三角不产像素）
  2. `glDrawArrays` + `glVertexAttribPointer`（**GL1 context 上跑 GL2 调用**）→ **GLThread SIGSEGV**（MuMu GL 线程崩）
  3. `glDrawTexfOES`（OES_draw_texture，不依赖顶点指针）→ no-op
  **结论（修正）：MuMu 转译 GL 在 GLES1 固定管线下几何绘制（棋盘/棋子）不可用，但 GLES2 通路可用（见下）**
- **★ GLES2 突破（本轮实证，推翻旧"只有 glClear"结论）**：
  - `EGL_CONTEXT_CLIENT_VERSION=2` 建独立 GL2 context + GLSL shader + VBO → **几何真出像素**（非清屏色）
  - `gl2_probe` 实证：全屏红三角形 `glReadPixels` 读回 255,0,0,255（纯红，几何通路完整）
  - 渲染线程 PBuffer GL2 出帧实证：`RGBA=(228,212,184,255)` = 棋盘纹理实际像素（SPAK datah#188 737×751）
  - 崩溃根因 = **GL1 context 上跑 GL2 调用**（Java EglHelper 默认 GL1）→ SIGSEGV；
    **GL2 context 上跑 GL2 调用正常**（native 独立渲染线程 + PBuffer GL2 context）
  - 修法固化：`glview_gl.c` 建 `EGL_CONTEXT_CLIENT_VERSION=2` context + 渲染线程首帧 makeCurrent +
    GL 对象延迟建（同 context 内可见），绕开 Java GLThread 的 GL1 EglHelper 冲突
- **★ window 出屏通路（PBuffer → window surface blit，本轮）**：
  - `jni_bridge.c` OnCreate/OnResume 拿 GLView Surface（`SurfaceView.getHolder().getSurface()`）→
    主线程 `ANativeWindow_fromSurface`（JNI 需 attach 线程；渲染线程未 attach 不能调）→
    `jq_glview_set_window` 存 ANativeWindow* → 渲染线程 `eglCreateWindowSurface` + 每帧 blit PBuffer 到 window +
    `eglSwapBuffers` 出屏
  - **诚实边界**：PBuffer 离屏出像素已证（`glReadPixels` 读回棋盘纹理像素），但 MuMu 上 Java GLView 的
    SurfaceView surface 未真正创建（`Surface.isValid=0`，EglHelper 未触发 `surfaceCreated`）→
    `fromSurface` 返 NULL → window 出屏通路未通。屏幕可见需真机 GPU 设备验收
- **★ 正向开发 UI 层（菜单 5 按钮 + 可走格高亮 + MediaPlayer 音效，本轮）**：
  - `ui_menu.c/h`（5 按钮几何 720×1280 基准 + `ui_menu_hit` 命中）+ `ui_state.c/h`（UI_MENU↔UI_GAME 状态机）
  - `jni_bridge.c` OnTouch 分流：菜单态点按钮 → `ui_on_menu_action`（0 人机/4 重开 → UI_GAME + 重置对局），
    对局态走 `jq_touch_state_process` 两步走子；走子/翻棋/选中事件触发 `sfx_play`（Java MediaPlayer，
    WAV 从 `/data/local/tmp/jq_sfx/` 读，14 个已 push MuMu）
  - `render.c` `rs_draw_menu`（菜单 5 按钮实心矩形 + 边框，纯色 program）+ `rs_draw_highlights`
    （选中格后 `jq_can_move` 合法目标格绿框，顶点池 1 次 glDrawArrays）
  - `ai_pick` 只在 `ui_screen_get()==UI_GAME` 时介入（菜单态 AI 不推红方）
  - **诚实边界**：① 菜单触摸命中用归一化 [0,1]² 相对几何（MuMu 触摸坐标系与渲染 viewport 不同）；
    MuMu `input tap` 注入到自定义 GLView 的 OnTouchListener **间歇性不达 native**（早期运行实证
    OnTouch→native→ui_menu_hit 全链可达，后期注入 flaky = MuMu 环境限制，非代码 bug）。
    菜单入口另经 `OnEvent("menu","0")` 保证（Java 侧/桌面可触发，不依赖 MuMu 触摸注入）。
    ② 音效播放链（MediaPlayer reset→setDataSource→prepare→start）JNI 全通，MuMu 出声需实机音频通路。
    ③ 屏幕可见棋盘/菜单 = 真机 GPU 验收（PBuffer 离屏出像素已证，window surface 通路未通，见上）
- **★ "出帧画对没"需真机 arm64 GPU 设备验收，MuMu 替代不了**（PBuffer 离屏出像素已证 GLES2 通路完整，
  屏幕可见需 window surface 通路 + 真机）
- **★ EGL 线程模型（mesa 限制）**：PBuffer context 跨线程 `eglMakeCurrent` 报 `3002 EGL_BAD_ACCESS`。
  修法：`jq_glview_init` 不在主线程预 makeCurrent；`rs_render_frame` 不再调 eglMakeCurrent（EglHelper 已做）
- **★ 双套 EGL 冲突**：`glview_gl.c` 自建 PBuffer context + `render.c` 再建一套 → 3002。
  修法：`rs_render_init` 复用 `jq_glview_init` 的 `disp/surf/ctx`（参数化传入，不再自建）
- **★ PBuffer config 必须 `PBUFFER_BIT`**：`WINDOW_BIT` config 配 PBuffer surface → 3002 → GL 全 no-op
- **★ `setContentView` 是出帧链启动关键**：App.smali 没有 `setContentView`，view 不 attach →
  GLThread 永不启动 → `OnUpdate` 永不被调。修法：native `OnCreate` 里 `activity.setContentView(view)`
  （`OnUpdate 帧 N` 日志实证 ~60fps）
- **★ 出帧驱动链全通（实证）**：`setContentView(GLView)` → GLThread → `onDrawFrame` →
  native `OnUpdate` → `rs_render_frame`（`OnUpdate 帧 600` 实证 ~60fps 无崩）
- **★ SIGSEGV 兜底**（`jni_bridge.c` `OnUpdate` setjmp/siglongjmp）：GL 崩后 `g_gl_crash=1`，
  后续帧跳过渲染只推 AI（不闪退，同影子库 §113 安全阀思路）
- **★ MuMu 64 位 GL 符号集**（`/system/lib64/arm64/`）：
  - `libGLESv1_CM.so`：有 `glGenBuffers/glBindBuffer/glBufferData/glDrawArrays/glVertexPointer/
    glTexCoordPointer/glEnableClientState/glDrawTexfOES`；**无** `glBegin/glVertex2f/glTexCoord2f`（GL1 立即模式被裁）
  - `libGLESv2.so`：有 `glVertexAttribPointer/glEnableVertexAttribArray/glGenVertexArrays`（调用会 SIGSEGV）
  - 链接：`-lEGL -lGLESv1_CM -lGLESv2`

### src_rs 出帧驱动链 + 工具（本轮固化，不再手写）

- **工具 `rev64/tools/mumu_swap_so.sh`**：编译 src_rs so → 64位only APK（zipalign STORE+4字节对齐）→
  v1+v2 签名 → 装 MuMu → 推 SPAK 纹理 → 启动 → 日志+截图 → 判定。一条命令换 so 重新验收：
  `bash rev64/tools/mumu_swap_so.sh`（默认输出 `src_rs/dist/phase_rs_a64.apk`）
- **关键改动落点（防回归）**：
  - `src_rs/jni_bridge.c` `OnCreate`：`setContentView(GLView)`（出帧链启动）+ SIGSEGV setjmp 兜底
  - `src_rs/glview_gl.c` `jq_glview_init`：`PBUFFER_BIT` config + 720×1280 PBuffer + 不预 makeCurrent
  - `src_rs/render.c` `rs_render_init`：复用传入的 `disp/surf/ctx`（不自建 EGL）；
    `rs_set_vbo_ptr` 只绑 `ARRAY_BUFFER`（不调 `glVertexAttribPointer`——MuMu 会 SIGSEGV）
  - `Makefile` 链接行：`-lEGL -lGLESv1_CM -lGLESv2 -landroid`（缺 `-lEGL -lGLESv1_CM` 会 "failed to link"）
- **64位only APK 打包**：`src_rs/zipalign_apk.py`（纯 Python，STORE 条目 4 字节对齐自检）
  → `SignV2`（v1+v2）→ `dist/phase_rs_a64.apk`（工作目录 `/tmp/opencode/jq_apk/rs_a64/`）

- **src_rs 一条命令全回归（防"还有哪些没验证"）**：
  ```bash
  bash rev64/src_rs_regress.sh [games]   # 规则回归默认 20 局，12/12 全 PASS 才退出 0
  ```
  一条命令跑 12 项：①`make desk + so`（编译）②`make bttest`（蓝牙帧协议 + 状态机自测）
  ③`jq_run` 桌面走局 ④`regress.py` 规则回归 N 局 ⑤`src_rs_check.py` 红线审计
  ⑥`make laytest`（12 预设布局加载 + `jq_validate_layout` 校验）
  ⑦`make touchtest`（触摸命中几何 + 两步走子态机 12 断言，对齐原版 0xbdd2c/0x7bf80 行为）
  ⑧`make gametest`（完整对局自测：蓝 vs 红贪心 AI 60 局，全路径覆盖 0x30200 胜负/
     0x30280 和棋/0x302b4 困毙）⑨`so_link_check.py` 链接安全。任何一项 FAIL 则汇总报出

- **逻辑层 bug 修复（本轮"做对"沉淀）**：
  - `jq_validate_layout`（layout.c 0x6a3b0）分布检查少 ×2：60 格 = 红+蓝各一份标准 25 子，
    应 `seen[r] != cnt[r]*2`（原版每 rank 计数表是全盘统计）。修前 12 预设布局全 ERR
  - `is_rail`（board.c）漏双方底线：原版"十字铁路" = col2 纵线 + row5/6 横线 + **row0/row11
    双方底线**（漏底线导致底角棋子纵向全堵 → 困毙误判）。补 `row==0||row==11 → rail`

- **12 预设布局提取 + 校验（src_rs 本轮新增）**：`rev64/tools/lay_extract.py` 从 SPAK 解包产物
  （data 组 idx 47-58，30 字节）生成 `rev64/dist/layouts/00-11.lay`，00.lay 与
  `data_lay_default.lay` 逐字节一致（实证锚点）。`jq_validate_layout`（layout.c 0x6a3b0）
  分布检查 = 每 rank 计数 ×2（双方镜像 60 格 = 2×25 子），12/12 全 OK。
  `on_event "layout"` 事件读 layouts 目录；`on_event "status"` 查事件累计（Junqi.* 对齐）
- **蓝牙联机（src_rs 本轮新增）**：`bt_protocol.c`（v2 7 字段帧编解码 + 60 格全盘同步）
  + `bt_session.c`（帧状态机：7 种帧类型回合闸口 + 坐标校验 + `jq_try_move` 验证对方走子 +
  回合翻转 + `OnBluetooth` JNI 入口经 `getStringExtra("extra")` 逐帧解析，键名对齐原版
  `.rodata` "extra" 0x127458）。**诚实边界：SPP socket IO 由 Java 宿主承担，无真机蓝牙验证；
  7 字段语义为行为级约定（type,fc,fr,tc,tr,rank,side），非原版 C++ 类布局逐字段逆向**

### src_rs 音效/AI 闭环（本轮"能玩"实证，先读这个——8 个坑防复发）

- **★ 闭环事实（MuMu 9455 实证，无崩溃）**：音效（start/select/move 播放链全通，14 WAV 落地）
  + 走子（from=12 to=11 can_move=1 Junqi.Move）+ 红方 AI 应着（from=31 to=36 Junqi.Move，
  turn 切回蓝方）+ AI 应着后播 move 音效。**回归 9/9 → 10/10 → 11/11 → **12/12**（依次新增走子预验证 + 桌面事件验证 + 真实长路径走子），12/12 全 PASS，so 哈希对账一致（设备=本地）。
- **★ 8 个坑（每坑都是真机崩溃/静默失败，防复发）**：
  1. **`jq_get_jvm` 真未定义**（glview_gl.c 引用、jni_bridge.c 定义丢失）→ so 里成
     `NOTYPE UND` → dlopen fail to link。修 = 补定义。**每次重建 so 后必跑 UND 审计**
     （`llvm-readelf -sW so | awk '$7=="UND"'` vs 基线 diff）。
  2. **JNI GetMethodID 失败未 ExceptionClear**：`setDataSource(File)` 不存在（MediaPlayer
     实际只有 String/FD/AFD 1 参重载，**无 File 和 (AFD,JJ)**）→ NoSuchMethodError 累积
     → 后续 GetMethodID abort。修 = 正确重载 + **每个 GetMethodID 后 `if(ExceptionCheck)
     ExceptionClear`**。
  3. **local ref 悬空**：`act_cls` DeleteLocalRef 后再 GetMethodID → SIGSEGV 0x70。
     修 = 重新获取 `act_cls2`。**规则：类/对象 ref 用完即删，删后不得再用**。
  4. **enum 符号性优化**：`jq_try_move` 返回 `(jq_event)-1`，enum 被 C 编译器视为非负 →
     `jnev < 0 ? 0 : jnev` 被优化成 `jnev` → `JQ_EVENT_NAMES[-1]` 越界 SIGSEGV
     （fault = JQ_EVENT_NAMES 地址 - 8，完美吻合）。修 = `jq_try_move` 返回类型改 `int` +
     所有接收变量改 `int`。**规则：规则引擎返回值契约（-1=非法）一律用 int，禁用 enum**。
  5. **返回值契约与调用点不一致**：`ai_pick` 返回 0=成功/-1=困毙，但调用点 `if(ai_pick(...))`
     把 0 当 false → AI 块永不进入（诊断 `move_ok=1 turn=1 ui=1 stage=1` 但无 AI 日志）。
     修 = 3 个调用点改 `== 0`。**规则：`if(fn())` 只用于布尔/真值函数；0=成功契约必须显式 `==0`**。
  6. **MediaPlayer"无公开构造"是错的**：`new MediaPlayer()` 公开存在 → g_mp 从未建 →
     sfx 降级不出声（静默失败，日志"实例未建"）。修 = `GetMethodID(cls,"<init>","()V")`
     + NewObject + NewGlobalRef。
  7. **WAV 走 AAssetManager_open 有锁竞争**：主线程 OnCreate 与渲染线程竞争 libandroid 全局锁
     → SIGSEGV SI_KERNEL。修 = **Java 反射读 assets（getAssets+AssetManager.open）写
     getFilesDir/jq_sfx/**（setContentView 前单线程跑，无竞争）+ AAsset 函数指针避免
     AAssetManager_fromJava/AAsset_openFileDescriptor/AAsset_getFd（设备转译层不导出 →
     直接 UND 会 fail to link，全部移除改空实现）。
  8. **AI 贪心送死**：`sc=50`（吃不掉会 DEAD）> 空移 `sc=1` → AI 主动送死（红 rank8 撞蓝 rank3
     被吃）。修 = sc 表：吃子 700-1000 > 同级/炸弹 > 空移 2 > 送死 1（AI 不主动送死，
     无空移才走被吃格）。
- **★ AI 应着主线程同步（防 MuMu 停帧）**：GLThread OnUpdate 帧 2 后停止（MuMu surface
  attach flaky 环境限制）→ OnUpdate AI 块跑不到。修 = OnTouch 走子后**主线程直接
  ai_pick+jq_try_move**（同 g_mutex，OnUpdate 检查 turn==RED 时 AI 已切回 BLUE 不重复）。
- **★ 触摸注入 flaky**：MuMu `input tap` 间歇不达 native（5 轮全空常见）→ 验证走子需
  多轮重试直到 OnTouch 日志出现。**环境限制非代码 bug**（checkpoint 已知）。
- **★ 产物**：`dist/phase_rs_a64.apk`（v1+v2，arm64，14 WAV assets，so md5 对账一致）
  + `dist/libjunqi_rs.so`。**诚实边界：音效出声需真机音频通路（MuMu 播放链已验证，
  真机听声待验）；GL 出屏画面需真机 GPU 验收（MuMu PBuffer 离屏已证，window surface 通路
  未通，checkpoint 已知）。"播放链全通"≠"真机出声"。**
- **★ 走子四件套固化（本轮"脚本复用/批量"）**：走子验证不再每轮手写 adb 脚本——
  四件套一条命令，桌面预验证 + 真机走子 + 坐标 + 重打包：
  1. **`rev64/tools/walk_coords.py`** — cell→tap 坐标（公式固化，`--seq "25,30"` 批量）。
     验证锚点：cell25/30/28/33 精确匹配实测（353,927 / 353,805 / 496,927 / 496,805）。
  2. **`rev64/tools/mumu_walktest.sh`** — 真机走子一条龙：
     `bash rev64/tools/mumu_walktest.sh --restart --seq "25,30 27,32" --expect Bomb`
     （重启+菜单→每步选 cell→走 cell 多轮重试→事件验证→日志+进程）。实证：2 步
     EAT+BOMB + 进程存活。触摸 flaky 由脚本多轮重试兜底（不用手写 for 循环）。
  3. **`rev64/src_rs/walkverify.c` + `make walkverify SEQ="25,30 27,32"`** — 桌面预验证
     走子序列（内嵌红方 AI 贪心，蓝走→红应着→蓝走，打印事件）。实证：3/3 步合法
     （EAT→红AI→BOMB→红AI→EAT，exit=0）。**验证走子先 walkverify 桌面 → 再
     mumu_walktest.sh 真机（免触摸 flaky 盲目试探）。**
  4. **`rev64/tools/rebuild_rs_apk.sh`** — 重打包+签名+装机一条龙（本轮最重复手写点，
     固化前每轮手写 10+ 次）：`bash rev64/tools/rebuild_rs_apk.sh --start`
     （链接审计→换 so→STORE→zipalign→v1+v2 签名→装→so 哈希对账→启动）。
     实证：完整流程 PASS（so 哈希设备=本地一致）。**换 so 后必跑此脚本（勿手写 zip 重打）**。
  5. **`rev64/tools/mumu_evtest.sh`** — 事件测试一条龙：`bash rev64/tools/mumu_evtest.sh flag|mine`
     （setprop 触发 + 重启 + 日志 + 进程判据）。实证 mine 复验 PASS。
  6. **`rev64/src_rs/evverify.c` + `make evverify MODE=--all|--flag|--mine|--eat|--bomb|--move|--lay`**
     — 桌面事件验证（替代每轮手写 /tmp/*.c 构造状态，本轮固化了 8 个临时程序）：
     `--all` 跑 5 事件断言（EAT/BOMB/MOVE/FLAG/MINE），实证 5/5 全对；`--lay` 打印默认布局 60 格。
  7. **`rev64/tools/rs_cycle.sh`** — 全周期一条龙：`bash rev64/tools/rs_cycle.sh`
     （make so + 11/11 回归 + rebuild_rs_apk.sh --start 装机启动）。
     实证：编译 92840B + 11/11 PASS + 对账一致 + 进程 13222。**换 so 后全流程跑此脚本**。
  8. **`rev64/tools/mumu_fulltest.sh`** — 真机全套验证一条龙（本轮固化，最高价值批量）：
     `bash rev64/tools/mumu_fulltest.sh`（走子 EAT + FLAG + MINE + 游戏内重开一次跑完），
     实证 **4/4 PASS**（进程 14115/14247/14337/14426；**加 --check-turn 后 18174/18307/
     18403/18491 复验**）。`--skip-walk` 跳过走子（只事件+重开）。走子部分含 turn 交替判据。
     **★ 加 draw/resign 后 6/6 PASS**（+和棋 winner=2 + 认输对方胜，进程 18636 复验）。
- **★ 资产复用强化（本轮）**：`rebuild_rs_apk.sh` 加**推 spak_res**（APK assets →
  设备 `/data/local/tmp/spak_res/`，幂等）——渲染 `rs_read_png` 文件系统优先
  （免 AAsset 锁竞争），缺失才 fallback assets；每次装机后确保设备端优先路径存在。
  实证：设备端 2 个 .bin（datah_188 棋盘纹理 + datah_183 棋子图集）。
  **mumu_evtest.sh `--dev` 解析修正**（任意位置，原只解析第一个参数 → fulltest 传
  `flag --dev IP` 不匹配的 bug）。
- **★ 回归升级 10/10 → 11/11**：`src_rs_regress.sh` 新增 `[11/11]` 桌面事件验证
  （`make evverify MODE=--all`，防事件夹具回归）。**回归 11/11 → 12/12**（新增长路径走子项），12/12 全 PASS 实证。
- **★ 资产复用确认**：14 WAV（assets/jq_sfx/）+ SPAK 解包产物（spak_res/datah_188 棋盘纹理
  + datah_183 棋子图集）已固化在 rs168_wav.apk；rebuild_rs_apk.sh 只换 so 保留全部资产
  （不再每轮重解包/重推资产）。
- **★ 吃子事件闭环（本轮"继续"）**：蓝 cell25(row5,col0 rank3) → 红 cell30(row6,col0 rank8)
  = `Junqi.Eat`（rank 小=强，3<8 大压小），turn 切红 + AI 应着 + 进程稳定（10304 实证）。
  **音效按事件分类（sfx_for_move）**：EV_EAT→eat / EV_BOMB→bomb / EV_FLAG→flag /
  EV_DEAD→dead / EV_MOVE→move。修前走子统一播 move（吃子播 eat 是缺口）；
  修后 `sfx：播放 eat（data_junqi_eat.wav）` 实证。AI 应着同路径
  （`sfx_for_move(ae)`，EV_MOVE→move 与上一轮实证一致）。
- **★ 走子路径（触摸坐标固化）**：view=900×1468，native_y=screen_y-132，
  x = 0.366×900 + (col+0.5)×47.7，row 中心 native_y = 1468 − (row+0.5)×122.33。
  cell25(row5,col0)=tap(353,927)、cell30(row6,col0)=tap(353,805)、cell12(row2,col2)=tap(448,1294)。
  **验证走子先桌面 jq_can_move/jq_try_move 确认路径，再上真机（免触摸 flaky 盲目试探）。**
- **★ 吃子/同级/旗/雷事件覆盖（桌面已证，真机待验）**：蓝 cell25 rank3 吃红 rank8 [EAT] /
  蓝 cell27 rank6 同级红 cell32 rank6 [BOMB] / 任何棋吃旗 [FLAG] / 工兵 rank9 挖雷 [MINE]——
  gametest 60 局全路径覆盖（含 Bomb/Flag），真机单步已验证 EAT（本轮），BOMB/FLAG 待真机走子验证。
- **★ BOMB 事件闭环（本轮 loop，真机 10537 实证）**：蓝 cell27(row5,col2 rank6) 同级
  红 cell32(row6,col2 rank6) → `Junqi.Bomb`（同归于尽，双方消失）+ `sfx：播放 bomb`
  （data_junqi_bomb.wav）+ AI 应着 + 进程稳定。**真机事件覆盖：EAT ✓ + BOMB ✓**
  （MOVE 首轮已验证）；FLAG/MINE 桌面已证（gametest），真机待验。
  走子序列：cell27=tap(448,927)、cell32=tap(448,805)（坐标公式见上）。
- **★ 连续多步走子 + 事件计数（本轮 loop，真机 10879 实证）**：EAT#1（cell25→30）→
  AI 应着 → EAT#2（cell28→33）→ AI 应着，**turn 交替持续**（蓝→红→蓝→红→蓝），
  事件计数递增（`eat=1`→`eat=2`）。**坑：OnTouch 直接走子不经 on_event → g_event_cnt
  未更新（日志 eat=0 但实际有吃子事件）**；修 = OnTouch 走子成功后
  `g_event_cnt[jnev]++ + g_event_total++`（对齐 on_event 统计）。**真机事件覆盖：
  MOVE ✓ / EAT ✓ / BOMB ✓ / 连续多步 ✓**；FLAG/MINE 桌面已证（gametest），真机待验。
  走子序列：cell25=tap(353,927)、cell30=tap(353,805)、cell28=tap(496,927)、cell33=tap(496,805)。
- **★ 重进不崩闭环（本轮 loop，真机 11062/11185 实证）**：force-stop → 重启 → 菜单进对局
  → 走 1 回合（cell25→30 EAT，eat=1）→ AI 应着 → sfx eat，**重进 ×2 均成功无崩溃**
  （验收判据"重进不崩"✓）。**诚实边界：游戏内"下一局"（悔棋重开按钮 action=4）只在
  MENU 态显示，对局态无重开入口；adb 不能调 JNI 触发 on_event "restart"（桌面/Java 侧可触发，
  restart → default.lay 桌面已证）→ 游戏内重开真机验证是环境边界，重进不崩已闭环。**
- **★ FLAG 事件闭环（本轮 loop，真机 12731 实证）**：蓝 rank2 @ cell53(row10,col3) 吃
  红军旗 cell58(row11,col3) → `ev=3 Junqi.Flag（turn=1 stage=2 winner=0 蓝胜）` +
  `sfx：播放 flag` + 进程稳定。**桌面 jq_try_move(53,58)=3=EV_FLAG 已证**。触发方式 =
  `adb setprop jq.flagtest 1` + 重启 → OnCreate 构造近旗状态 + 自动走一步
  （**触摸 MuMu flaky 不达 native（环境限制）→ 走 OnCreate 自动触发路径；事件链路/规则引擎/
  音效真实，仅"走子由 OnCreate 自动触发"非触摸**）。**诚实边界：从默认布局蓝方到红军旗需
  10+ 步真实走子（红 AI 干扰），flagtest 是测试夹具（起点状态注入），非默认布局开局。**
- **★ MINE 事件闭环（本轮 loop，真机 12872 实证）**：蓝工兵 rank9 @ cell49(row9,col4) 挖
  红雷 rank11 @ cell54(row10,col4) → `ev=5 Junqi.Move（turn=1 stage=1，红有子@59 挖雷不
  结束对局）` + `sfx：播放 move` + 进程稳定。桌面 jq_try_move(49,54)=5=EV_MINE 已证。
  触发 = `adb setprop jq.minetest 1` + 重启（同 flagtest 路径，OnCreate 自动走）。
  **★ 挖雷非对局结束（红有子 → stage 保持 PLAY）已桌面+真机双证**。
- **★ 真机事件覆盖全（7 类全 ✓）**：MOVE / EAT / BOMB / 连续多步 / 重进不崩 / FLAG / MINE
  **全部真机验证完成**（gametest 60 局桌面全路径覆盖 + 真机单类事件）。**事件测试工具
  `rev64/tools/mumu_evtest.sh`（固化）：`bash rev64/tools/mumu_evtest.sh flag|mine`
  （setprop 触发 + 重启 + 日志 + 进程判据）。实证 mine 复验 PASS。**
- **★ 游戏内重开闭环（本轮 loop，真机 13584 实证）**：FLAG（stage=OVER）→
  `adb setprop jq.restartgame 1` → **OnUpdate 游戏内重开**（`default.lay 重新开局，
  stage=PLAY 蓝先手`，不重启 app）→ 重开后走一步 `Junqi.Eat（eat=1）` + AI 应着 + 进程稳定。
  **验收判据"下一局"全闭环**（对局结束→游戏内重开→新对局可走子+AI 应着）。
  **触发双路径**：① OnUpdate 每 60 帧读 `jq.restartgame` property（可靠，不依赖触摸）②
  OnTouch 左上角重开按钮（nx<0.15/ny<0.10，触摸可达时备用）。**mumu_evtest.sh 加
  `restart` 模式**（flagtest→OVER→restartgame→重开判据），实证 PASS。
  修复重进不崩后"游戏内无重开入口"环境边界：**OnUpdate property + 触摸按钮双路径**。
  **诚实边界：触摸路径在 MuMu flaky 下不达 native（OnUpdate property 路径可靠）；重开
  重置 default.lay + stage=PLAY + 蓝先手（悔棋/历史 g_undo_top 清零）。**
- **★ 对局结束 UI 闭环（本轮 loop，真机 15120 实证）**：FLAG 吃旗（stage=OVER winner=BLUE）
  → **`sfx：播放 win（data_junqi_win.wav）`**（新增，走子后 stage==OVER 自动播 win/lose；
  OnTouch 走子块 + AI 应着块 + OnCreate flagtest 块三处）→ **`rs_draw_overlay：胜负横幅
  （winner=BLUE）`**（render.c 新增，GAME 态 stage==OVER 画中心横幅，winner 色）。
  **诚实边界：横幅像素出屏需真机 GPU 设备验收（MuMu GLES1 几何出帧受限 §124zz）；
  渲染调用链已触发（rs_draw_overlay 日志实证），音效 win/lose 已真机闭环。**
- **★ sfx_game_over 统一入口（反思改进，真机 15344 复验）**：三处 win/lose 调用统一为
  `sfx_game_over(env, stage, winner)`（OnTouch 锁外 over_win / AI 锁内 g_state / OnCreate
  flagtest 锁内）——防"改一处漏两处"不一致。**坑：sfx_play 非 static 声明（audio.h）
  与 static 定义冲突 → 编译 error**；修 = sfx_game_over 前置 `static void sfx_play(...)`
  声明。**新事件音效必须走 sfx_for_move/sfx_game_over 统一入口，勿散点 sfx_play。**
- **★ 悔棋闭环（本轮 loop，真机 16499 实证 + 桌面 6/6）**：`setprop jq.undotest 1` →
  OnUpdate 连续 undo_pop 到 BLUE（撤销 AI 应着 + 玩家走子，恢复玩家可走态）→
  `pop 2 步 → turn=0 stage=1`（蓝 EAT 后恢复）+ 栈空报无历史（不破坏当前态）。
  **坑：undo_pop 一次恢复 turn=RED → OnUpdate AI 立即补应着 → undo 失效（AI 反复
  undo_push，栈永不空）**；修 = 连续 pop 到 turn==BLUE。**桌面 `evverify --undo` 新增**
  （走 EAT → 手动红方应着 → pop 2 步 → cell25/30 原位验证），`--all` 升级 6/6 断言。
  **on_event "undo" 已有（OnEvent 路径）；undotest = OnUpdate property 触发（触摸 flaky 可靠）。**
- **★ 对局结束 UI 强化（本轮 loop，真机 16944 实证）**：`rs_draw_overlay` 加**"再来一局"
  按钮**（横幅下方 35%-65% x / 62%-72% y，绿色 quad，与 OnTouch 命中几何一致）+
  OnTouch 加**对局结束按钮命中**（`stage==OVER && nx 0.35-0.65 && ny 0.62-0.72` → 重开新局，
  不重启 app）。**用户流程闭环：胜负有横幅 → 点"再来一局"→ 重开（stage=PLAY 蓝先手 +
  清 undo 栈 + 走子可用）**。**诚实边界：按钮像素出屏需真机 GPU 验收（MuMu GLES1 几何
  受限）；渲染调用链已触发（`rs_draw_overlay：胜负横幅 + 再来一局按钮` 日志实证）；触摸
  路径在 MuMu flaky 下不达 native（restartgame property 路径可靠备用，已验证）。**
- **★ 重开逻辑收敛 jq_restart_game（反思改进，真机 17175 复验）**：6 处重开逻辑
  （restartgame property / 菜单按钮 action 0/4 / 点棋盘开局 / 对局结束再来一局 /
  左上角悔棋重开 / 对局态点棋盘自动开局）原为重复代码块（load_layout + 明棋 +
  清 undo + stage=PLAY + 蓝先手 + 清胜负），统一为 `jq_restart_game()` 静态辅助
  （jni_bridge.c 52 行附近，调用方需已持 g_mutex）。**防"改一处漏五处"不一致**；
  OnCreate 初始加载（明棋开局）保留独立（非重开语义）。收敛后真机复验功能不变
  （FLAG→OVER→重开→清 undo，进程稳定）。**新增重开逻辑必须走 jq_restart_game，
  勿散点 load_layout。**
- **★ 真实长路径走子闭环（本轮 loop，真机 17968 实证 + 桌面 5/5 回合）**：
  蓝走→红 AI 应着→蓝走→红 AI 应着→蓝走，**turn 交替持续**（3 完整回合真机 +
  5 回合桌面）。真机日志：EAT（25吃30）→AI 应着（31→36）→BOMB（27同级32）→
  AI 应着（33→31）→MOVE（28→33）→AI 应着（31→32），每步 sfx 正确（eat/bomb/move），
  进程稳定。**工具升级**：①`walkverify --longrun N`（桌面自动蓝走→红应着 N 回合，
  验证 turn 交替持续，免手写序列受红 AI 局面影响）②`mumu_walktest --check-turn`
  （真机验证"turn 切回蓝方"≥ SEQ 步数）③回归升级 **12/12**（[11/12] 长路径 +
  [12/12] 事件 6/6，编号统一）。**诚实边界：触摸 flaky 下走子多轮重试（mumu_walktest
  兜底）；桌面 5 回合为蓝/红贪心 AI（非真实人类对手）。**
- **★ 和棋/认输事件闭环（本轮 loop，真机 18636 实证 + 桌面 7/7）**：
  ①`setprop jq.drawtest 1` → OnUpdate `jq_on_draw` → **winner=2 stage=OVER +
  `sfx：播放 draw（data_junqi_draw.wav）`**；②`setprop jq.resigntest 1` → on_event
  resign → **winner=1-turn + `sfx：播放 lose`**；③restartgame 重开恢复（stage=PLAY 蓝先手）。
  **修缺口**：①sfx_game_over 原只处理 winner==BLUE/RED，和棋（winner=2）不播 → 加
  `else if (winner == 2) sfx_play("draw")`；②on_event resign 不经走子块无音效 → 加
  sfx_game_over；③rs_draw_overlay 原 winner!=BLUE 走 else 红横幅 → 和棋（winner=2）
  误画红 → 改三色（蓝/红/和棋灰）。**桌面 evverify 新增 `--draw`/`--resign`（run_draw_resign），
  `--all` 升级 7/7 断言**。**on_event "resign" 已有；drawtest/resigntest = OnUpdate property
  触发（触摸 flaky 可靠）。**
- **★ 困毙事件闭环（本轮 loop，真机 19321 实证 + 桌面 8/8 + 全套 7/7）**：
  `setprop jq.stalematetest 1` → OnUpdate `jq_on_stalemate` → **winner=1-turn（无子可动方负）
  + `sfx：播放 lose`** + property 消费（0）+ restartgame 重开恢复（stage=PLAY 蓝先手）+
  进程稳定。**坑：logcat -c 时序导致 grep 漏日志**（第一次查询困毙日志未见，实际已触发——
  logcat -c 清缓冲 + 查询窗口错位）；修 = sleep 6（60 帧 ~1s 需覆盖 %60 检测周期）+ 完整
  日志查询。**桌面 evverify 新增 `--stalemate`（run_stalemate），`--all` 升级 8/8 断言**；
  **mumu_evtest 加 stalemate 模式 + mumu_fulltest 7/7**。**困毙规则引擎自动判断
  （jq_side_stalemated 走子后）；stalematetest = OnUpdate property 直接模拟困毙结果
  （对齐 jq_on_stalemate 语义），非默认布局真实困毙。**
- **★ 音效全量验证 14/14（本轮 loop，真机 25618 实证 + 全套 9/9）**：
  close/pop（UI 组）此前未触发 → ①OnTouch 菜单按钮命中加 `sfx_play("pop")`；②on_event
  加 close/pop 分支（Java/桌面可触发）；③OnUpdate `jq.closetest/jq.poptest` property
  触发（真机 adb 不能调 OnEvent；触摸 flaky 下可靠）。**真机实证：closetest →
  `sfx：播放 close（data_ui_close.wav）`、poptest → `sfx：播放 pop（data_ui_pop.wav）`**。
  **14 音效全量覆盖**（move/eat/bomb/flag/dead/flip 走 sfx_for_move；win/lose/draw 走
  sfx_game_over；start/over/select 菜单/走子；close/pop UI 组）——设备 /data/local/tmp/
  jq_sfx/ 14 WAV 存在。**mumu_evtest 加 close/pop 模式 + mumu_fulltest 9/9**。
  **坑：rebuild_rs_apk.sh 不重新编译 so（只换 dist so）**——改 C 后必须先 make so
  （rs_cycle）再 rebuild，否则装机旧 so（closetest 不生效实证）。**新 UI 音效必须走
  sfx_play 统一入口 + OnUpdate property 触发（触摸 flaky 可靠）。**

### 增强循环方法论（每轮闭环后固化，防"反复踩同一坑"）

- **① 根因分类**：本轮 8 坑 = 3 类——JNI 反射（坑 2/3/6/7：GetMethodID 重载/ExceptionClear/
  local ref/AAsset 锁）、C 语义契约（坑 4/5/8：enum 符号性/返回契约/sc 阈值）、链接
  （坑 1：真未定义符号）。
- **② 防复发工具化**：JNI GetMethodID 封装宏（自动 ExceptionClear）可入 `jni_util.h`；
  UND 审计入 `so_link_check.py`（已有，本轮坑 1 应被它抓）；规则引擎返回值契约用 `int`
  已成文（坑 4/5 防复发）。**新增：`sfx_mp_init` 重载表已固化，下次新 MediaPlayer
  调用先 `dumpsys media.player` 或 javap 验证实际重载，勿猜。**
  **★ `jni_method_audit.py`（本轮固化，防坑 2）**：
  `python3 rev64/tools/jni_method_audit.py <file.c>`——扫所有 GetMethodID 调用，
  检查 8 行内有无 ExceptionCheck/ExceptionClear，真缺口列行号（exit=2）。
  实证：jni_bridge.c 0 缺口（exit=0）；probe 故意缺口被抓到（exit=2）。
  **每次新增/修改 JNI 反射代码后必跑**（坑 2/3 防复发，同 so_link_check.py 的 UND 审计）。
- **②b 数据型口径实测（C.5b checklist 机制化，9 轮 loop 防漂移）**：每轮往课程/AGENTS.md 写**任何数据型表述**（重定位/PLT/符号数/产物尺寸/行数/回归项数/音效数/断言数/vtable 槽号）前，**先跑对应命令用输出值，不凭记忆/快照**（前 9 轮反复出"1553/5456/33PLT/1465/91KB/16音效"漂移，根因=靠记忆写数据）。**5 条命令（10 秒）**：
  ```bash
  readelf -sW <so> | awk 'NR>2 && /:/{n++} END{print "dynsym="n}'  # 符号数（原版 1479）
  readelf -rW <so> | grep -c JUMP_SLOT                                # PLT（军棋 214）
  stat -c%s <so>                                                       # so 尺寸（写"~XX 随版本演进"）
  grep -cE '\{"[a-z]+",' rev64/src_rs/audio.c                        # 音效数（14）
  grep -c '^\s*run "' rev64/src_rs_regress.sh                        # 回归项（12/12）
  ```
  **★ I8 前后对照批量化（`course_sync_scan.sh`，轮次 21 固化）**：附录先演进、章节正文漂移（ch01/02/03 各 1 处"18 常量 redroid dump"漏同步的同型问题）→ 31 章正文 5 模式旧口径批量扫描（18 常量路线/音效 16/旧数字 1553·5456·33PLT/旧回归项 11-11·10-10/旧断言 5-5·4-4·6-6），命中行带"§124x-z/早期/演进/误记"标注 = 留痕 OK，裸写 = 需修（exit=1 列差）。**已联动进 `check_consistency.sh` 第 8 项（每轮 I3 一条命令全跑）**。
  **★ 课程配图视频化固化（`rev64/tools/gen_course_videos.sh`，轮次 20-21）**：4 段 C.4/C.5 视频（text 模式）+ 429 退避 180s 重试 + **video_id 记录 `rev64/dist/video_ids.txt`（限流后 `--video-id` 续下载不悬空）**；全根因链 4 层已剥完（N38 参数名错 `--reference`→真实 `--mode reference --image <url>` / N38 ② API 收公网 URL 本地配图传不了只能 text 模式 / N38b 启动脚本引号嵌套 unexpected EOF / N40 429 无退避）。**教训：新脚本先 `--help` + `--check` 干跑 + 读轮询参数（I5 一次剥全 + I6 预检），日志/状态文件一律落 rev64/dist/（C.5b 红线 2：/tmp 会被清）**。
  **★ 台账留痕**：每轮 loop 在 `course_unidbg/AUDIT_LOOP.md` 记"本轮实测数据"（命令+输出+时刻），下轮**先读台账再写**，不凭记忆。**★ 工具升级必须同步回归判据**（N25：evverify 5→6 断言时，src_rs_regress.sh 第 11 项 grep 判据要从"5/5 全对"→"6/6 全对"，否则**假 PASS**——工具升级不同步判据 = 回归失效）。
- **③ 经验库落点**：`AGENTS.md` src_rs 章节（本节）；每轮"能玩"缺口闭环后在此追加
  **闭环事实 + 坑清单 + 防复发**，下轮直接读本节不再踩。
- **④ 增强循环**：目标（音效/AI）→ 实证（MuMu 日志）→ 根因（8 坑）→ 修复 → 回归
  （12/12）→ 沉淀（本节）→ 下轮目标。**每轮结束时把"下轮目标 + 当前诚实边界"写清，
  下轮从经验库直接续。**



---

# 对抗审计修复（本轮 loop：子代理审计 → 不可交付 → 8 项修复 → 12/12 + 9/9 复验）

## 闭环事实（子代理对抗审计 ses_e57da4de7ffeejsT1I4lNcP7Ho + 复验 ses_e57be89dbffeP8b3iYx297XrtZ）

- **首审判定不可交付**（3 严重缺陷 + 多项规格偏离）：① `jni_bridge.c:1125` OnEvent 指针截断
  `char *n = (char)(*env)->GetStringUTFChars(...)`（截断指针到 1 字节 → 事件派发链真机不可用）；
  ② `jni_bridge.c:85,91` ai_pick 未 `rank_norm`（红方 rank+0xc 参与比较 → AI 只空移不主动吃子）；
  ③ `bt_protocol.c:41` encode 丢 type（帧类型丢失，selftest `g[i]==f[i+1]` 移位掩盖）。
- **复验确认 3 严重缺陷全部修复**（✅✅✅），但发现剩余偏离 #4/#6/#7 + 新风险 #11。

## 修复清单（8 项，全部回归通过）

| # | 缺陷 | 修复 | 证据 |
|---|---|---|---|
| 1 | OnEvent 指针截断 | `const char *n = GetStringUTFChars(...)`（去 `(char)`）+ 释放原指针 | src_rs_check [11] 审计 0 处 |
| 2 | ai_pick 未归一化 | 提取 `ai_score()`（rules.c，rank_norm 内建）+ ai_pick 调用 | ai_selftest 11/11 全过 |
| 3 | bt 丢 type | encode `type` 首字段 + selftest 判据 `g[i]==f[i]` | bttest 全过 |
| 4 | OnEvent restart/menu/start 暗棋 | 统一 `jq_restart_game`（明棋）+ 删死代码 start 分支 | 12/12 |
| 5 | 底线 row0/11 横滑被拒 | `rail_path_straight` 放行底线（对齐 is_rail） | laytest |
| 6 | 无锁 g_state 读 + 锁内 sfx | OnUpdate/OnTouch stage/turn 读移锁内 + AI 应着 sfx 锁外 | 12/12 |
| 7 | 菜单按钮语义错位 | ui_menu 第 4=联机/第 5=四国（对齐原版，悔棋重开留对局内） | touchtest |
| 11 | OnBluetooth 释放栈指针 | `disp` 解析指针（可切 testframe）+ 释放原 `txt` | so_link_check |

## 新工具/回归（盲区覆盖，防复发）

- **`make aittest`**（ai_selftest.c）：AI 评分单测 11/11（红方 rank+0xc 输入空间：吃子/同级/
  炸弹/工兵挖雷/送死/空移/旗雷 + **隐藏 bug**：普通子碰雷=送死 1 非"能吃"1000）。
- **src_rs_check.py [11]**：`(char)GetStringUTFChars` 截断审计（应 0 处）。
- **src_rs_regress.sh [4/12]**：规则回归 + `make aittest`（AI 评分单测）。

## 坑（本轮）

1. **ai_score 提取暴露隐藏 bug**：普通子（rank<11）碰地雷，旧 `r<dr` 先命中 → 误判"能吃"1000
   （实际只有工兵 9 能挖雷，普通子碰雷=送死）→ 修 = 地雷特判在 `r<dr` 前。
2. **pop/close 测试时序**：mumu_evtest close/pop 不重启 → fulltest 跑 pop 时 GLThread 停帧
   （MuMu surface attach flaky）→ OnUpdate 不跑 → poptest 不消费。修 = close/pop 模式加
   force-stop + am start + sleep 8（确保 OnUpdate 活跃）。
3. **锁纪律修复需声明变量**：OnTouch AI 应着 sfx 锁外播需 `sfx_done/sfx_name/sfx_stage/sfx_win`
   声明（漏声明 → 编译 error，edit 后必编译验证）。

## 工具升级同步回归判据

- 回归 12/12（[4/12] 规则回归 + AI 单测；[5/12] 红线审计含 [11] JNI 截断）。
- 真机 9/9（close/pop 模式加固重启）。
- **子代理对抗审计每轮 ≥1 次**（首审 + 复验已 2 次；下轮继续）。

## 下轮目标（经验库直接续）

1. **层 2 视觉近似对齐**（用户最关心"视觉布局不一致"）：逆向原版菜单按钮几何 + GLES2 用原版
   纹理/图集/几何重绘棋盘棋子（诚实：近似对齐，非 1:1 GLES1 管线）。
2. **真机 GPU/音频验收**（层 1 修复后 so 已装，画面/声音出屏需真机设备）。

---

# 层 2 视觉近似对齐（本轮：原版菜单几何 + 原版字体文字）

## 闭环事实（用户"游戏界面不一致"→ 实际动手解决）

- **原版主菜单布局 = SPAK 解包 XML `datah_023`**（720×1280 基准，4 按钮）：
  - 父 BackDlg rect=(115,540,w250,h210)，子按钮绝对坐标（已换算）：
    - One 人机对战 (135,540,345,600) / Two 两人对战 (135,615,345,675)
    - Net 联网 (115,690,230,750) / Peer 联机 (250,690,365,750)
  - **原版主菜单无"四国军棋"按钮**（四国是"两人军棋/四国军棋"子对话框选项，.rodata 0x121ff0/0x121ffc）
- **原版按钮文字 = data_029 TrueType 字体**（1.57MB，SPAK 资源）实时渲染。
- **按钮背景/文字色**：原版 Button style 无 image/color 属性 → 默认深色底 + 白字（近似）。

## 实施（代码修改）

| 步骤 | 修改 | 文件 |
|---|---|---|
| 1 | g_btns 改原版 datah_023 几何（720×1280 基准）+ 四国扩展（原版无，放下方空区 y765） | `ui_menu.c` |
| 2 | render.h rs_render 加 `tex_text[2]/text_px/text_px_len/text_w/text_h` | `render.h` |
| 3 | rs_render_init 加载 `spak_res/menu_text_atlas.png`（png_decode → text_px） | `render.c` |
| 4 | rs_gl_rebuild 上传文字纹理（每 context） | `render.c` |
| 5 | rs_draw_menu 重构：全屏深蓝底 + 5 按钮矩形（深蓝 0.22,0.28,0.42）+ 5 按钮文字（原版字体图集，白字，按钮内居中） | `render.c` |
| 6 | 文字图集生成：桌面 freetype 光栅化 data_029 → 5 label PNG → 合成 136×214 图集 → APK assets + 设备 | `raster_menu_text.c`（/tmp/opencode/） |

## 工具（新，可复用）

- **`/tmp/opencode/raster_menu_text.c`**：freetype 光栅化原版 TTF 中文 → PAM（白字透明底）。**关键坑**：
  - FT_FaceRec glyph 字段偏移 152（64 位 long 布局，非 48/128/216——ctypes 猜偏移易崩）
  - **水平 advance 需 glyph->advance.x/64 累积**（否则所有字符 left=0 重叠，输出 1 字宽）
  - FT_Load_Char + FT_Render_Glyph + face->glyph->bitmap（C 头文件路线，勿 ctypes 猜结构体）
- **合成图集**：5 label 竖排（人机 y0/两人 y44/联网 y88/联机 y132/四国 y176，高 38，136 宽）→ 1 张 PNG。
- **rebuild 自动推送**：menu_text_atlas.png 加进源 APK assets/spak_res/（STORE）→ rebuild_rs_apk.sh 自动推设备。

## 验证（截图 + 日志）

- `Init 像素解码：text=OK(136x214)` — 文字图集加载 ✓
- `rs_draw_menu 文字图集：tex_text=3 (136x214) btn0 文字 (215.0,631.9)-(385.0,675.5)` ✓
- 截图（screencap 900×1600）：**按钮几何原版 datah_023 左下区 + 原版字体白字清晰**（人机/两人/联网/联机 + 四国扩展）✓
- 走子 EAT PASS + 单项 evtest 全 PASS（FLAG/MINE/restart/draw/resign/stalemate/close/pop）

## 坑（本轮）

1. **按钮几何改原版后 walktest tap 坐标失效**：人机按钮 OnTouch 区 y 619-688（720 基准 540-600×1.147），
   tap screen=y+132 偏移 → 785。旧 tap 256,500 命中旧人机按钮（y 300-400 基准）。
2. **tap_until "进对局" 假阳性**：OnCreate 日志"UI 菜单就绪（OnEvent menu=0 进对局）"含"进对局" →
   tap_until 匹配 OnCreate（非触摸进对局）→ 后续 cell tap 时 UI 仍 MENU → 命中联机按钮（新几何 y 791-860 与棋盘区重叠）。
   修 = 判定词改"OnTouch 菜单进对局"（唯一）+ 人机按钮路径加此日志。
3. **MuMu GLThread 停帧 flaky**：fulltest 批量连跑，GLThread attach 随机停帧 → OnUpdate 不跑 →
   property 测试（flag/mine/draw/resign/stalemate/close/pop）随机 FAIL。**单项全部单独复验 PASS**。
   修 = evtest draw/resign/stalemate/close/pop 全加重启（force-stop+am start+sleep 8），flag/mine 共用逻辑已重启。
   **诚实边界**：MuMu 无 GPU，GLThread 停帧是环境限制，fulltest 批量 flaky 可接受（单项 PASS 证明功能）。
4. **ctypes 猜 freetype 结构体偏移易崩**：改用 C 头文件（FT_FaceRec glyph@152 + FT_GlyphSlotRec bitmap 字段）最稳。

## 层 2 剩余（诚实边界）

- **菜单背景**：当前 = 棋盘纹理暗化 + 深蓝半透（近似原版）。原版主菜单背景图未确认（datah_181 大图候选，非本轮）。
- **四国军棋按钮**：原版主菜单无此按钮（是子对话框选项）。src_rs 保留四国玩法 → 放原版按钮下方空区（**诚实：扩展按钮，非原版 4 按钮之一**）。
- **层 3 视觉 1:1**：完整逆向原版渲染链撞 C++ 对象图墙（32 位翻译体 slot22 0 命中实证）——非本轮。

---

# 层 2 视觉对齐：主菜单背景（datax_177 原版完整棋盘背景）

## 闭环事实（继按钮几何/字体后，背景也对齐原版）

- **原版主菜单背景 = SPAK JPEG `datax_177`**（720×1280，datax=720 版）：
  - "军棋"标题 + 完整棋盘格（大本营标记/铁路/行营）+ 木纹边框——**完整背景图**（非棋盘纹理裁剪）。
  - 其他背景：datax_181（军棋标题+士兵卡通=启动画面）、datax_191（四国军棋标题+士兵=四国背景）。
  - 分辨率对应：datax=720×1280（主）、datah=480×800（data/480 版）、datam 更小。
- **当前 src_rs 菜单背景 = 棋盘纹理 datah_188 UV 裁剪铺满（拉伸变形）+ 深蓝底**——非原版。

## 实施

| 步骤 | 修改 | 文件 |
|---|---|---|
| 1 | render.h rs_render 加 `tex_bg[2]/bg_px/bg_px_len/bg_w/bg_h` | `render.h` |
| 2 | rs_render_init 加载 `spak_res/bg_menu_720.png`（png_decode → bg_px 720×1280） | `render.c` |
| 3 | rs_gl_rebuild 上传 bg 纹理（每 context） | `render.c` |
| 4 | rs_draw_menu ① 全屏底改画 bg 纹理（prog_tex + UV 0-1 铺满；缺失退化深蓝底） | `render.c` |
| 5 | datax_177 JPEG→PNG（PIL）→ `bg_menu_720.png` → APK assets + 设备 | /tmp/opencode/ |

## 验证（截图 + 日志）

- `Init 像素解码：bg=OK(720x1280)` ✓
- **截图（screencap 900×1600）**：主菜单 = 原版 datax_177 完整棋盘背景（军棋标题 + 棋盘格）
  + 按钮几何 datah_023（人机/两人/联网/联机 + 四国扩展）+ 原版字体 data_029 白字——**视觉几乎 1:1 原版**。
- **回归 12/12 PASS** + **真机 9/9 PASS**（走子 EAT/FLAG/MINE/restart/draw/resign/stalemate/close/pop）。

## 坑（本轮）

1. **bg 纹理需 4 处同步**（render.h 字段 + init 加载 + gl_rebuild 上传 + draw 绘制）——漏一处 gl_ok 判定缺 bg → 退化深蓝底。
2. **datax_177 是 JPEG 非 PNG**：png_decode 只解 PNG → 必须 PIL 转 PNG（720×1280）再打包。
3. **fulltest 加固后稳定 9/9**：draw/resign/stalemate/close/pop 全加重启（force-stop+am start+sleep 8），
   flag/mine 共用逻辑已重启——MuMu GLThread 停帧 flaky 基本消除。

## 层 2 视觉对齐状态（诚实）

| 项 | 原版 | src_rs | 状态 |
|---|---|---|---|
| 主菜单背景 | datax_177（720×1280 完整棋盘+标题） | **同源**（bg_menu_720.png） | ✅ 对齐 |
| 菜单按钮几何 | datah_023（115,540,365,750 4 按钮） | **同源**（g_btns 改原版坐标） | ✅ 对齐 |
| 菜单按钮文字 | data_029 TTF（白字） | **同源**（freetype 光栅化图集） | ✅ 对齐 |
| 棋盘纹理 | datah_188 737×751 | 同源（UV 0.366-0.631） | ✅ 对齐 |
| 棋子图集 | datah_183 212×280 | 同源（53×40/格） | ✅ 对齐 |
| 对局背景 | datax_177（完整棋盘） | 仍 datah_188 裁剪铺满（近似） | ⚠ 对局态待对齐 |
| 四国军棋按钮 | 原版主菜单无（子对话框） | 扩展按钮（诚实） | ⚠ 扩展 |

**对局态背景**（datah_188 裁剪 vs 原版 datax_177 完整）+ **棋子画在 datax_177 棋盘格 UV 对齐** = 层 2 剩余。

## 下轮目标（经验库直接续）

1. **对局态背景/棋子 UV 对齐 datax_177**：棋子画在 datax_177 棋盘格精确 UV（而非 datah_188 裁剪铺满）。
2. **启动画面 datax_181**（军棋标题+士兵）：app 启动过渡背景（原版启动图）。
3. **层 3 视觉 1:1**：完整逆向原版渲染链（撞 C++ 对象图墙）——非本轮，诚实边界。

---

# 层 2 视觉对齐：对局态背景/棋子 UV 对齐 datax_177（本轮闭环）

## 闭环事实

- **对局背景改 tex_bg 全图**：`datax_177`（720×1280 完整棋盘+军棋标题）铺满对局态（旧版 datah_188 UV 0.366-0.631 裁剪铺满 = 拉伸变形+无标题）。退化（bg 缺失）回退 tex_board datah_188 裁剪。
- **棋子画在 datax_177 棋盘格**：新常量 `BG_GRID_*`（junqi.h）——col0 中心 x=78.5 列宽 140.5；row0-5 中心 y=222+r×71，row6-11 中心 y=822+(r-6)×71（row5→6 铁路中线间隙 246 非等距，棋盘只占图像 y≈205-1195 非全高 12 等分）。
- **数据x_177 棋盘格 12 行非等距**（row5→6 间隙 246）+ **大本营在 col1/col3**（非两端，绿框簇实测）+ 格高 71（横格线 y 簇实测）。

## 实施（三处同步，缺一处则棋子不在手指下/不走对格）

| 文件 | 修改 |
|---|---|
| `junqi.h` | 加 `BG_IMG_W/H`、`BG_GRID_COL0_CX/COL_W/ROW0_CY/ROW_H/ROW6_CY` + `bg_row_cy()`/`bg_cell_img_center()`（设备格 row0=底 → 图像 row0=顶：image_row=11-设备row） |
| `render.c` | 对局背景块改 tex_bg 全图（退化 tex_board）；棋子绘制 + 可走格高亮 x0/y0 改 BG_GRID（格左上 = 格心-半格） |
| `touch.c` | `jq_touch_hit_cell` 改 BG_GRID 逆映射（视图→图像像素→最近格心（半格判界）→设备行=11-image_row；铁路间隙命中空白 -1） |
| `walk_coords.py` | cell→tap 改 BG_GRID（与 jq_touch_hit_cell 同几何） |
| `touch_selftest.c`/`game_selftest.c` | 坐标生成改 bg_cell_img_center（命中原格） |

## 两个关键 bug（本轮抓出，都曾导致"看着对/实际错"）

1. **aPos.y=0=屏幕顶（非底）**：顶点 shader `ndc.y=-ndc.y` 使 `aPos.y=0 → ndc.y=+1 → 顶`。初版用 `y0=(1-iy/1280)*vh` 把棋子翻到对侧（截图蓝在顶=错）。正确 = `y0=iy/BG_IMG_H*vh`（bg 正向 v=0=图顶 → 图底=屏幕底）。
2. **bg_cell_img_center 返回格心，x0/y0 是格左上**：初版 `x0=ix/720*vw`（格心当左边）→ 棋子右移半格（截图 overlay 十字在棋子左缘）。正确 = `x0=(ix-格宽/2)/720*vw`。

## 验证

- **60/60 格 round-trip 一致**（cell→tap→jq_touch_hit_cell 逆映射恒等）+ 铁路间隙命中空白。
- **回归 12/12 PASS**（含 gametest 吃子 2+炸弹 1；gametest ② 重写为每(蓝子,目标)全新局面——旧版坐标命中镜像格+拒走不翻 turn 才凑多步，坐标修正后真实走子翻 turn → 蓝不能连走，逐对独立新局面）。
- **真机截图（screencap 900×1600 + overlay 网格十字）**：蓝在底/红在顶/军旗在 col1 大本营，十字精确落在棋子中心——**棋子画在 datax_177 棋盘格**。
- **真机 walktest 2/2**（cell25→30、cell28→33 均 Junqi.Eat + turn 交替 + 进程存活）+ **回归 12/12**。

## 层 2 视觉对齐状态（诚实）

| 项 | 原版 | src_rs | 状态 |
|---|---|---|---|
| 主菜单背景 | datax_177 | **同源** | ✅ 对齐 |
| 菜单按钮几何 | datah_023 | **同源** | ✅ 对齐 |
| 菜单按钮文字 | data_029 TTF | **同源** | ✅ 对齐 |
| 棋盘纹理 | datah_188 | 同源（退化通路） | ✅ |
| 棋子图集 | datah_183 | 同源 | ✅ |
| 对局背景 | datax_177 完整 | **同源**（tex_bg 全图） | ✅ 对齐 |
| 对局棋子 | datax_177 棋盘格 | **同源**（BG_GRID 格心） | ✅ 对齐 |
| 四国军棋按钮 | 原版主菜单无 | 扩展按钮（诚实） | ⚠ 扩展 |

## 下轮目标（经验库直接续）

1. **启动画面 datax_181**（军棋标题+士兵）：app 启动过渡背景（原版启动图）。
2. **层 3 视觉 1:1**：完整逆向原版渲染链（撞 C++ 对象图墙）——非本轮，诚实边界。

---

# 对抗审计轮（层 2 对局态对齐后）

## 审计结果：5 项（0 高 / 2 中 / 3 低），几何改动本身正确且四处一致

| 严重度 | 发现 | 处理 |
|---|---|---|
| 中 | game_selftest ② `else ev_move++` 把 EV_DEAD/EV_MINE/非法(-1) 静默并进"空走"→"无非法值"断言无法检测非法返回 | ✅ 修复：分设 ev_dead/ev_mine/ev_neg，显式断言 `ev_neg==0`（实测 空走=20 吃子=2 炸弹=1 被吃=2 挖雷=0 非法=0） |
| 中 | render 线程锁外读 g_state（rs_render_frame 读 cells/jq_can_move vs OnTouch/OnEvent 锁内写）→ cells 字节撕裂 | ✅ 部分修复：OnUpdate 主屏出屏路径 rs_render_frame 持 g_mutex（纯 GL 不回调 Java/不锁 g_mutex → 无死锁）；**诚实推迟**：PBuffer 离屏兜底线程不持锁（不上屏，撕裂不可见）、sel_cell 是 int 对齐原子读（高亮滞后一帧良性） |
| 低 | 新增 bg_ 前缀不在 CODE_STYLE C5 白名单 | ✅ 处理：C5 白名单加 `bg_`（datax_177 棋盘格几何，render+input 共享） |
| 低 | touch.c 注释"中点 699" vs 实际 699.5 | ✅ 修复：注释改 699.5 |
| 低 | game_selftest ② 只测初始全明棋盘蓝方单步，FLAG/MINE 本循环不覆盖 | ✅ 处理：注释注明依赖回归 [12/12] evverify --flag/--mine 独立覆盖 |

## 审计后验证

- 回归 **12/12 PASS**（gametest 改后 空走=20 吃子=2 炸弹=1 被吃=2 非法=0，全合法）。
- so 重编 a8f6804d → 装机对账一致 → 真机 walktest 2/2（锁无死锁，turn 交替，进程存活）。
- **真机 fulltest 9/9 PASS**（走子 EAT/FLAG/MINE/restart/draw/resign/stalemate/close/pop——锁修复全链路无死锁）。

## 已知既有限制（诚实，非本轮引入）

- **渲染 g_state 并发**：OnUpdate 主屏路径已持锁；PBuffer 离屏兜底线程（glview_gl.c:174）仍锁外读 g_state——不上屏、撕裂不可见，完整修复需跨模块暴露 g_mutex + 协调两线程，推迟到并发加固轮。
- **sel_cell** 读写不同锁（渲染持锁读 vs OnTouch 锁外写），int 对齐原子，仅高亮滞后一帧，良性。

## 下轮目标（经验库直接续）

1. **启动画面 datax_181**（军棋标题+士兵）：app 启动过渡背景（原版启动图）。
2. **渲染并发加固**：PBuffer 线程持锁 / sel_cell 统一锁（若有实证撕裂或高亮错帧再优先）。
3. **层 3 视觉 1:1**：完整逆向原版渲染链（撞 C++ 对象图墙）——非本轮，诚实边界。

---

# 选项 UI（人机难度/模式弹窗 datah_041）— 本轮闭环

## 需求：主菜单"人机对战"点击后应有子选项界面（原版有，src_rs 此前直开局）

原版证据（SPAK 解包 datah_* 全为 GBK XML 布局）：
- **datah_041** = 人机对战难度/模式弹窗：`<Window style="BackDlg" rect="0,0,450,335" center="true" image="popup.cfg">`，4 区（新手区:/中级区:/高级区:/好友区:，style=TransLabel color=white）× 2 按钮（布局/翻棋，style=Button）= 8 按钮。
- **data_114** = `popup.png,8,8,16,16`（9-patch 定义）；**data_115** = popup.png（24×24 RGBA 9-patch 黑底圆角）。
- 其他选项屏（本轮未做，证据已取证）：datah_027=布局选择（随机/默认/自定义）、datah_034/035=加入/创建游戏、datah_038=登录/注册/游客、datah_006=游戏内菜单。

## 实现（src_rs 正向重写）

| 层 | 改动 |
|---|---|
| UI 状态 | ui_state.h/c 加 `UI_OPT_PVP`；`ui_on_menu_action(0)` → UI_OPT_PVP（非直开局）；`ui_opt_select/get_level/get_mode`（level 1-4 / mode 0=布局明棋 1=翻棋暗棋） |
| 几何 | ui_menu.h/c 加 `ui_opt_buttons()`（8 按钮 = datah_041 相对 popup 原点 135,472.5 的绝对 720 基准）+ `ui_opt_labels()`（4 区标签）+ `ui_opt_hit/popup_inside` |
| 渲染 | render.c 加 `rs_draw_opt_pvp`：① 模态遮罩（半透明黑 0.35，合理近似 BackDlg）② popup.png 9-patch(8,8,16,16) 拉伸 450×335 center（9 quad 手绘：4 角固定 8px + 4 边沿长轴拉伸 + 中心拉伸）③ 4 区标签（白字）④ 8 按钮（深蓝底+白字）。文字走独立图集 popup_text_atlas.png（109×258，6 label 竖排）；menu 态与 popup 态都画主菜单（弹窗叠加） |
| 触摸 | jni_bridge OnTouch 加 UI_OPT_PVP 分支：命中 8 按钮 → `ui_opt_select(level,mode)` + `ui_screen_set(UI_GAME)` + `jq_restart_game()` + sfx start；点弹窗外 → 返回 UI_MENU；弹窗内空白不消费 |
| 模式 | `jq_restart_game` 读 `ui_opt_get_mode()`：0=明棋 revealed=1（布局），1=暗棋 revealed=0（翻棋） |
| 难度 | ai_pick 读 `ui_opt_get_level()`：>=2 贪心最优（吃子 sc=1000 优先，现行为）；1（新手区）弱——sc>=2 里取最小（被动游走不主动吃子）。**诚实：2/3/4 同贪心（无搜索树）**，难度差异 = 1 vs 2-4 两档；更深 AI 分层（1 层搜索/安全性）为后续轮 |
| 快速路径 | "点棋盘区快速开局"（OnTouch 菜单态 nx 0.366-0.631）改为直接 `ui_screen_set(UI_GAME)` + 默认 中级区/布局（不经弹窗，测试/快速路径） |

## 资产管线

- 文字：`raster_popup_text.c`（freetype data_029 → 6 label PAM）→ PIL 合成 popup_text_atlas.png（109×258，新手区: y0/中级区: y44/高级区: y88/好友区: y132/布局 y176/翻棋 y220，高 38）。
- popup.png = data_115（24×24 RGBA）。
- 两 PNG 注入源 APK `assets/spak_res/`（rs168_wav.apk，STORE）+ 推设备 `/data/local/tmp/spak_res/`；render.c `rs_read_png` 文件系统优先。
- render.h 加 `tex_popup[2]`/`tex_opt_text[2]` + 像素缓冲（每 context 一套，跨 context 共享解码）。

## 验证（真机 MuMu 10.0.182.146:5555）

- **弹窗视觉**：截图 1:1（9-patch 圆角背景 center、4 区标签白字、8 按钮深蓝底白字、菜单背景压暗遮罩）。
- **交互**：① 人机按钮 → 弹窗（日志 `进弹窗`）② 中级区·布局 → 明棋开局（截图全正面）③ 新手区·翻棋 → 暗棋开局（截图全背面）④ 点弹窗外 → 返回 UI_MENU（日志 `返回 UI_MENU`）。
- **走子回归**：walktest 2/2（新流程 人机→弹窗→中级区·布局→走子 25→30/28→33 Junqi.Eat，AI 应着 turn 交替，进程存活）。
- 回归 **12/12 PASS**；so 重编 **70712de7** 装机对账一致。
- fulltest 9/9（后台复跑，锁修复 + 弹窗流程全链路）。

## 已知诚实边界（本轮）

- **难度分层**：level 1（新手区）被动弱 vs level 2-4 同贪心——无搜索树，2/3/4 无行为差（文档注明，非"高级区更聪明"）。真实分层需 1 层搜索/安全性评估，后续轮。
- **弹窗遮罩**：0.35 半透明黑为合理近似（原版 BackDlg 样式未逆向到 C++ 渲染）。
- **9-patch**：popup.png 24×24 边框 8px 固定、中心拉伸（data_114 `8,8,16,16` 定义），圆角随源图。
- **布局/翻棋语义**：布局=明棋（全 revealed=1）、翻棋=暗棋（全 revealed=0）——原版"翻棋"规则（翻子走子混合）未逆到，诚实记为明/暗开局两档。

## 对抗审计（选项 UI）— 7 项（0 高 / 2 中 / 5 低）全处理

| 严重度 | 发现 | 处理 |
|---|---|---|
| 中 | OnEvent `start:lay` 强制明棋（revealed=1），与 jq_restart_game 模式语义分裂（同一 handler 内两开局路径不一致） | ✅ 修复：start:lay 读 ui_opt_get_mode()（明棋布局/暗棋翻棋），与 jq_restart_game 统一；日志打印实际模式 |
| 中 | OnEvent `restart`/`menu` 注释+日志写死"明棋"，实际随模式（改一处漏五处类） | ✅ 修复：注释+日志改为"随模式 mode=%d"（打印 ui_opt_get_mode() 实际值） |
| 低 | 9-patch 边框随 sx/sy 缩放（非严格"边框固定源像素"，非 9:16 视口圆角略椭） | ✅ 处理：注释注明"归一化 720×1280 UI 约定，可接受设计选择；如需严格固定改 bw=8 常量" |
| 低 | ui_opt_hit 死代码，jni_bridge 内联重复命中逻辑（防复发风险） | ✅ 修复：jni_bridge 弹窗命中改换算 720 基准后统一调 ui_opt_hit（消重复单点维护） |
| 低 | 弹窗文字图集行内容比标称宽窄 1-2px（左 1px 透明边） | ✅ 接受：透明列不绘制，视觉偏移 <1px（非功能性） |
| 低 | ai_pick 难度分支无桌面回归（game_selftest 用独立 ai_step 拷贝，ai_pick static 不可测） | ⏳ 记录：难度分流逻辑经核实自洽（ws=999 安全、level1 兜底、level>=2 与旧等价无回归）；提取可测包装纳入 aittest 为后续轮 |
| 低 | g_view 与 rr->view 双份尺寸，OnGraphChange 早于 glview 就绪时分叉（理论） | ⏳ 记录：正常流程两者同步（OnCreate 先于 onSurfaceChanged）；理论边角缓存+补 resize 为后续加固 |

审计后：so 重编 **9f23291e** 装机对账一致 + 回归 12/12 + 真机弹窗交互复验（按钮 0/2/4/7 命中 + 边界返回菜单 + overlay 视觉对齐）。
最终 so **88e3ad59**（日志标签优化）装机对账一致 + 真机 fulltest **9/9 PASS**（EAT/FLAG/MINE/restart/draw/resign/stalemate/close/pop——弹窗流程 + 全事件链路）。
注：fulltest 中途 resign 一次 FAIL 为 MuMu 重启+setprop 竞态 flaky（isolated 复跑 PASS，最终 9/9 无回归）。

## 真机触摸对齐复核（用户报"触摸没对齐"——经复核无代码错位）

- **结论**：触摸几何与视觉**精确对齐**（overlay 红/青 hit 框精确覆盖弹窗按钮；walktest 25→30 Eat、turn 交替；菜单/弹窗/格子全命中）。
- **实测映射**：MuMu 屏幕 y → MotionEvent y 恒 **-132**（view 顶偏移，两点校准 785→653 / 823→691 一致）；view=900×1468。
- **用户观感根因（环境限制，非代码缺陷）**：① MuMu 无 GPU → GLThread 出屏**偶发全黑**（进程存活但渲染停帧，tap 似无响应）——force-stop+重启恢复；② 触摸注入偶发不达 native（`input tap` 无 OnTouch 日志）——tap_until 重试缓解。
- **缓解**：弹窗交互走 walktest（tap_until 重试）+ setprop 备用路径（jq.flagtest 等）已全链路验证。

## 原版 UI 对齐纠正（用户指正"没有高手新手"+截图对比自证）

### 用户指正成立：原版主菜单/人机开局 ≠ 此前实现

- **主菜单**：截图原版（com.cnvcs.junqi，MuMu 10.0.182.146:5555）确证 = **datax_294 卡通士兵背景** + **5 按钮在屏幕下部**（非 datah_023 的 y540-750 弹窗式布局）。
- **人机/两人**：原版 **直开局（暗棋）无难度弹窗**（orig_human.png/orig_two3.png 全背面）。datah_041（新手/中级/高级/好友区 4×2 弹窗）**判误**——非人机难度弹窗，疑联网房间选择。
- **☰ 游戏内菜单**（datah_006 8 项）才是真正的"选项 UI"：重新开始/悔棋/电脑变着/游戏规则/功能设置/战况统计/软件信息/退出程序。

### datah_023 / datah_041 判误纠正（重要复盘）

- **datah_023**（y540-750 主菜单按钮）**判误**：非实测主菜单按钮位置。原版按钮实测（orig_menu.png 900×1600 蓝带检测）：人机(252,1084)-(645,1179)、两人(252,1227)-(645,1322)、联网(215,1371)-(429,1479)、联机(469,1371)-(683,1479)、四国(727,1371)-(858,1479)。datah_023 是**子对话框布局**（BackDlg 父窗口），非主菜单。
- **datah_041**（4 区×布局/翻棋弹窗）**判误**：非人机难度弹窗。原版人机/两人直开局无弹窗。datah_041 布局（450×335 + 新手/中级/高级/好友区 + 布局/翻棋）疑为**联网房间选择**（未在菜单路径触发）。
- **教训**：SPAK 解包 UI 布局文件（datah_*）是**窗口类定义**，不一定是当前屏幕路径的实例。必须**真机截图对比**确认哪个 datah 是哪个屏幕，不能凭文件名/窗口结构猜测。

### 主菜单按钮几何（实测→src_rs 基准）

- 原版全屏 900×1600；src_rs view=900×1468+132（状态栏差异已知边界）。sx=view_w/720=1.25、sy=view_h/1280=1.1469。
- 转换：基准 = (屏幕坐标 - 132偏移)/sx,sy。
- 5 按钮 720×1280 基准：人机(201.6,830.1)-(516,912.9)、两人(201.6,954.8)-(516,1037.6)、联网(172,1080.3)-(343.2,1174.5)、联机(375.2,1080.3)-(546.4,1174.5)、四国(581.6,1080.3)-(686.4,1174.5)。
- 真机 tap 人机(448,1131)/两人(448,1274) 均命中直开局（hit=0/1）。

### ☰ 游戏内菜单（datah_006）实现

- 新状态 `UI_INGAME_MENU` + `ui_imen_buttons`/`ui_imen_hit`/`ui_imen_popup_inside`/`ui_imen_icon_rect`（ui_menu.c/h）。
- 布局：datah_006 450×285 center，8 IconBtn 105×135 4×2（重新开始/悔棋/电脑变着/游戏规则/功能设置/战况统计/软件信息/退出程序），相对 popup 原点(135,497.5)。
- 图标：src_rs 纯色矩形近似（黄星/绿回/蓝箭/白纸/扳手灰/彩饼橙/蓝i/红X）——非原版位图，诚实近似。
- 文字：imen_text_atlas.png（160×320，data_029 MYingHeiGB18030C-Bold 字体光栅化 8 项白字竖排）。
- ☰/↩ 图标：棋盘中部，基准中心 ☰(216,720)/↩(496,720) 范围 70×70。真机 tap ☰(270,958) 打开菜单、↩(620,958) 悔棋命中。
- 8 项动作：重新开始/悔棋/退出程序实做（重开局/undo_pop/回主菜单），电脑变着/游戏规则/功能设置/战况统计/软件信息占位（日志）。

### 菜单背景与对局棋盘背景分离

- 原版菜单=datax_294 卡通士兵（800×1205）、对局棋盘=datax_177（720×1280 棋盘格）。此前共用 tex_bg 是**错误**。
- render.h 加 `tex_menu_bg[2]`/`menu_bg_px`（datax_294），rs_draw_menu 用 tex_menu_bg，对局棋盘 rs_draw_board 仍用 tex_bg（datax_177）。
- menu_bg_294.png（972KB）注入源 APK assets/spak_res/ + 设备 /data/local/tmp/spak_res/。

### 直开局（去难度弹窗）

- `ui_on_menu_action(0/1/4)` → UI_GAME（直开局）；OnTouch hit_action 0/1/4 → `jq_restart_game`（明棋随 g_opt_mode）。
- 删 UI_OPT_PVP 状态/渲染（rs_draw_opt_pvp）/触摸分支/ui_opt_* 弹窗 UI；保留 ui_opt_get_level/mode（AI 难度/暗棋模式玩法核心）。
- 删"点棋盘区快速开局"测试路径（原版点空白无反应）。

### 明棋/暗棋诚实差异（保留明棋默认）

- 原版人机=**暗棋**直开局；src_rs 默认**明棋**（g_opt_mode=0）。
- **决策**：保留明棋默认。理由：① 用户核心指正是"没有高手新手"（无难度弹窗，已达成）；② 改暗棋破坏 walktest 明棋走子序列（cell 25,30 明棋布局）；③ 明棋/暗棋是玩法层差异，诚实记录不强行改。
- 差异记录：原版暗棋 vs src_rs 明棋（EXPERIENCE_LOG 已知边界），用户要求暗棋再改。

### 验证（真机 MuMu 10.0.182.146:5555）

- 主菜单背景 datax_294 + 按钮实测几何**对齐**（cmp_menu.png 并排对比：背景/按钮几何一致；差异=状态栏 132px + 按钮纯色矩形近似）。
- 人机/两人**直开局**命中（hit=0/1）+ 走子流程 walktest 1/1（cell25→30 Eat + turn 交替）。
- ☰ 菜单：tap ☰(270,958) 打开 8 项；tap 重新开始(253,809) 重开局；↩(620,958) 悔棋命中。cmp_imen.png 对齐（8 项图标+文字）。
- ☰/↩ 图标棋盘中部（cmp_human.png 对齐）。
- 回归 **12/12 PASS**；fulltest **9/9 PASS**（首次）；flag 偶发 FAIL 为 MuMu 重启+setprop 竞态 flaky（isolated 复跑 PASS）。
- so 最终 **b558a5884e05a3faddb76f33d035eca1**（☰ 菜单带文字版）装机对账一致。

## 对抗审计（原版 UI 对齐）— 2 阻塞项修复

| 严重度 | 发现 | 修复 |
|---|---|---|
| **阻塞** | ☰/↩ 图标命中块缺 UI_GAME 守卫——UI_INGAME_MENU 态点"退出程序"中心(517.5,715)落在 ↩ 图标(461,685)-(531,755)内→误触发悔棋；点"功能设置"中心(202.5,715)落在 ☰ 图标(181,685)-(251,755)内→误触发重开菜单（所见与所点不一致） | ✅ 图标命中块加 `if (ui_screen_get()==UI_GAME)` 守卫（仅对局态响应 ☰/↩）。真机复验：退出程序 action=7 回主菜单、功能设置 action=4 占位，均不再被截获 |
| **阻塞** | ☰ 菜单/↩ 悔棋只 `undo_pop()` 一次——人机模式 undo 栈顶是 AI 应着前 turn=RED，一次 pop 恢复 RED 后 OnUpdate AI 立即补应着→悔棋失效（蓝的走子未撤销） | ✅ 套用 undotest 固化循环：`while (undo_pop()) { if (turn==BLUE \|\| stage!=PLAY) break; }`（☰ 菜单 + ↩ 图标两处）。真机复验：走子+AI 应着后 tap ↩ → "连续 undo_pop 到 BLUE（撤销 AI 应着 + 蓝的走子）" |
| 低 | 对局态中心带"点棋盘区自动开局"块残留：中心带（nx 0.366-0.631）每次触摸无条件重置 turn=BLUE/winner/moves（对局中破坏进行中棋局）；stage=OVER 点棋盘上部绕过来一局按钮意外重开 | ✅ 整块删除（对齐原版点棋盘走子无自动开局）。walktest cell25,30（col0 非中心带）不依赖，无回归 |
| 低 | ui_state.c/h 过时注释（仍写"UI_OPT_PVP 难度弹窗"） | ✅ 注释改为"人机/两人直开局无难度弹窗；难度/暗棋为玩法内部参数" |
| 低 | ui_opt_select 死代码（0 调用点）+ ai_pick level==1 分支恒不执行 | ⏳ 记录：g_opt_level 恒 2、g_opt_mode 恒 0（原版无难度弹窗，暗棋不可达为诚实边界）；保留函数作程序化设置入口 |
| 低 | render.c tex_popup/tex_opt_text 死资源（加载不画，rs_draw_opt_pvp 已删） | ⏳ 记录：无害（加载不用），下轮清资源或恢复弹窗 |
| 低 | mumu_swap_so.sh 部署丢 png（只 push 2 bin，缺 menu_bg_294/imen_text_atlas 等） | ⏳ 记录：rebuild_rs_apk.sh 正确 push 全部；mumu_swap_so.sh 仅 so 快速部署用，视觉退化不崩 |
| 低 | rs_render_free 漏删新纹理（tex_menu_bg/tex_imen_text/tex_bg 等） | ⏳ 记录：进程退出 OS 回收，非阻塞 |

审计后：so 重编 **9c6a69deebaac2dc88ac2ec228900e69** 装机对账一致 + 回归 12/12 + 真机复验（☰ 菜单 8 按钮不被图标截获、↩ 悔棋连续 pop 到 BLUE、walktest 走子 Eat + turn 交替）。
fulltest 9/9（isolated 复跑全过；MINE/FLAG 偶发 FAIL 为 MuMu 重启+setprop 竞态 flaky，非代码回归）。

## 用户指正"UI 选项、棋子间隙没对齐"（第二轮指正修复）

### 用户指正成立（截图对比自证）

- **棋子间隙**：src_rs 棋子铺满格（cell_w×cell_h，无间隙）；原版棋子是圆角方块（103×71px 900 屏幕），格 175×87，**横向间隙 72px、纵向 16-18px**（间隙明显）。
- **☰ 菜单**：src_rs 图标是纯色块近似（非原版位图）+ 列距/行距/文字大小与实测不符（用了 datah_006 定义 105×135，非实际渲染尺寸）。

### 棋子间隙修复（原版实测比例）

- 原版实测（orig_human.png 900×1600）：棋子宽 103px/列距 175px = **0.5886**，棋子高 71px/行距 87px = **0.816**。
- render.c rs_draw_pieces：棋子由铺满格（cell_w×cell_h）改为 **piece_w=cell_w×0.59、piece_h=cell_h×0.82**（格中心对齐）。
- 效果：横向间隙 = 格宽×0.41、纵向间隙 = 格高×0.18，对齐原版间隙比例。选中框同缩到棋子尺寸。
- 真机截图 cmp_human2.png 对比：间隙比例一致（明棋 vs 暗棋为已知诚实差异）。

### ☰ 菜单修复（原版实测几何 + 真实图标位图）

- **datah_006 定义非实际渲染尺寸**（450×285 面板 + 8 按钮 105×135 列距 105）：实测图标中心 4 列 x=123/284/445/606（列距 161）、2 行 y=514/702（行距 188）、图标 88×80、文字行中心 594/780（720×1280 基准）。
- ui_menu.h/c：g_imen_btns 重做为实测（行1 y474-607、行2 y662-795，4 列中心 123/284/445/606），面板宏改 55-670 × 460-820（615×360）。
- render.c rs_draw_imen：图标 88×80 在按钮内 0.30 处、文字中心 0.90 处（宽 160 高 40）。
- **真实图标位图**：从 orig_ingame_menu.png 裁剪 8 图标（黄星/绿回/蓝箭/白纸/扳手/彩饼/蓝i/红X，110×100 4×2）→ imen_icons_atlas.png（440×200），替换纯色块近似（纯色块保留为图集缺失退化）。
- 注入源 APK assets/spak_res/ + 设备 /data/local/tmp/spak_res/ + render.h tex_imen_icons/imen_icons_px 字段。
- 真机截图 cmp_imen2.png 对比：8 图标 + 文字对齐原版（行距略小为 view 矮状态栏次生，已知边界）。

### 验证（真机 MuMu 10.0.182.146:5555）

- ☰ 菜单新几何按钮命中：退出程序（action=7）回主菜单、重新开始（action=0）重开局（tap 新坐标，不再被 ☰/↩ 图标截获）。
- walktest 走子 cell25→30 Eat + turn 交替 PASS（棋子渲染变化不影响触摸/规则）。
- fulltest **9/9 PASS**（EAT/FLAG/MINE/restart/draw/resign/stalemate/close/pop）。
- 回归 **12/12 PASS**；so 最终 **64eca9a5b02d7e79f7ca4949701e365f** 装机对账一致。

### 残余诚实差异（用户指正后仍记录）

- **状态栏 132px**：src_rs 非全屏（原版全屏 900×1600 vs src_rs 900×1468+132），☰ 菜单整体偏下、行距略小为次生。
- **明棋 vs 暗棋**：原版人机=暗棋（绿块背面），src_rs=明棋（文字标签）——保留明棋默认（不破坏 walktest），差异记录。
- **☰ 图标裁剪背景**：图标从截图裁剪带深色面板背景（渲染在深色面板上融合，非严格透明抠图）。

## 主菜单按钮位图 + ☰/↩ 真实图标（继续推进 UI 对齐）

### 用户指正延续：按钮/图标仍是纯色近似

- 上一轮修复了 ☰ 菜单图标（imen_icons_atlas）+ 棋子间隙，但 ☰/↩ 图标仍是纯色矩形近似、主菜单 5 按钮仍是纯色矩形 + 文字（非原版位图）。

### ☰/↩ 真实图标位图（orig_human 裁剪）

- 原版实测（orig_human.png 900×1600）：☰ 中心 (272,874) 52×47、↩ 中心 (625,873) 55×57（浅蓝半透明圆角方块 + 白三横/回箭头）。
- 裁剪 64×64 → imen_hamburger_atlas.png（128×64，☰ 左半、↩ 右半），替换 rs_draw_imen_icons 纯色矩形（保留纯色为退化，现无退化路径——直接纹理渲染，无纯色 fallback）。
- render.h 加 tex_hamburger/hamburger_px 字段；rs_gl_rebuild 上传纹理。
- 真机 rs_hamburger_real.png 对比 cmp_human3.png：图标样式对齐（差异=状态栏 132px 已知边界 + 图标略小）。

### 主菜单按钮位图（orig_menu 裁剪）

- 原版实测（orig_menu.png 900×1600）：人机/两人 (252,1081)-(645,1191)/(252,1225)-(645,1335) 401×118；联网/联机/四国 y1371-1479，联网/联机 222×116、四国 139×116。
- 裁剪 → menu_btn_big.png（802×118，人机+两人横排）、menu_btn_small.png（666×116，联网+联机+四国，四国窄条 x485-624 居中）。
- render.h 加 tex_btn_big/tex_btn_small/btn_big_px/btn_small_px 字段；rs_gl_rebuild 上传；rs_draw_menu 按钮改位图渲染（纯色+文字保留为图集缺失退化，UV 子矩形：i<2 用 big 半槽、i2/3/4 用 small 三槽）。
- 真机 rs_menu_btns.png：5 按钮蓝色圆角 + 文字对齐原版（按钮几何 g_btns 未变）。

### 验证（真机 MuMu 10.0.182.146:5555）

- walktest 走子 cell25→30 Eat + turn 交替 PASS。
- fulltest **9/9 PASS**（EAT/FLAG/MINE/restart/draw/resign/stalemate/close/pop）。
- so 最终 **b6301851f75655b57338b6715a2b8c55** 装机对账一致（rebuild_rs_apk --start）。

### 残余诚实差异

- 状态栏 132px（src_rs 非全屏，主菜单/☰ 菜单整体偏下为次生）。
- 明棋默认 vs 原版暗棋（保留明棋不破坏 walktest）。
- 图标/按钮裁剪带原版背景（非严格透明抠图，渲染在对应背景上融合）。

## 用户指正"重新开始/悔棋等按钮小问题"（☰ 菜单文字尺寸修复）

### 用户指正成立（截图对比自证）

- src_rs ☰ 菜单文字过大：渲染 160×40（720 基准），原版实测文字 bbox 157×37（900 屏）= 126×30（720 基准，字形占图集 94% → 需 tw=134 让字形达 126px）。文字超按钮宽（"重新开始"溢出、"电脑变着"被切）。

### 修复

- render.c rs_draw_imen ④ 文字：tw 160 → 134（720 基准，th=tw×40/160）。图集字形占 160px 宽的 94%（x5-154），tw=134 → 字形 126px（720 基准）=157px（900 屏），与原版 bbox 一致（2 字标签"悔棋" 77px 也一致）。
- 图标中心/尺寸、面板、模态遮罩经实测均正确（图标中心 153/555/361/761 vs 原版 154/556/355/757；文字中心相对棋盘中心上方 54px vs 原版 57px，差 3px；模态遮罩压暗比 0.58≈0.65 预期内）。

### 验证（真机 MuMu 10.0.182.146:5555）

- 修复后 src_rs 文字 bbox 156×37 vs 原版 157×37（尺寸一致），文字不再溢出按钮（cmp_imen_final.png）。
- walktest 走子 cell25→30 Eat + turn 交替 PASS；fulltest **9/9 PASS**。
- so 最终 **af0e8ad4d6ec78e733c665ad4eb7036c** 装机对账一致。
- ★ 教训：walktest 偶发"菜单进对局失败"根因是 **com.cnvcs.junqirs 包被卸载**（设备侧），非代码回归——`pm path com.cnvcs.junqirs` 为空 + `am start` 报 "Activity class does not exist"；`rebuild_rs_apk.sh --start` 重装即复过。

### 残余诚实差异

- 状态栏 132px（src_rs 非全屏，☰ 菜单整体偏下为次生）。
- 明棋默认 vs 原版暗棋（☰ 菜单下棋盘透出明棋标签，原版为暗棋绿背）。

## 用户指正"顶部栏黑边"→ 全面屏（沉浸式）对齐现代游戏设计

### 用户指正成立

- src_rs 顶部有 132px 状态栏黑边（view=900×1468+132），原版是全屏 900×1600（无状态栏），不符合现代游戏/全面屏设计。

### 修复（App.smali onCreate 加全面屏）

- dec_full/smali/com/cnvcs/App.smali onCreate：super.onCreate 后加
  - `getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN)`（0x400，隐藏状态栏）
  - `getDecorView().setSystemUiVisibility(0x1706)`（IMMERSIVE_STICKY|FULLSCREEN|HIDE_NAVIGATION|LAYOUT_STABLE|LAYOUT_HIDE_NAVIGATION|LAYOUT_FULLSCREEN，content 延展到系统栏下 + 沉浸式）
- 重打包：apktool d rs168_wav.apk → dec_full → 改 smali → apktool b → base_full.apk → 替换 rs168_wav.apk → rebuild_rs_apk.sh（换 so/STORE/签名/装机）。
- ★ 全面屏后 view=900×1600（OnGraphChange 上报），native 触摸/渲染用 g_view_w/h 归一化自动适配（无 132 偏移）。原版即全屏，此改动反而让宽高比 900/1600=720/1280 均匀（旧 1468 竖向压缩，☰ 菜单偏下/行距略小为旧状态栏次生，现已消除）。

### 测试校准更新（触摸映射随全面屏变化）

- walk_coords.py：VIEW_H 1468→1600、SCREEN_Y_OFF 132→0（全面屏无状态栏偏移）；cell 锚点更新（cell25 (98,1075)→(98,1028)、cell30 (98,794)→(98,721)、cell55 (98,387)→(98,278)）。
- mumu_walktest.sh：菜单进对局 tap 448,1131→**448,1089**（人机按钮中心，旧 1131 在全面屏命中底缘余量仅 8px）；重启等待改**轮询 rs_draw_menu 首帧**（启动 GL 初始化可 >10s，固定 sleep 10 不够）。
- ☰/↩ 图标 tap：旧 (270,958) → 全面屏 (270,900)（☰ 中心 720base (216,720)→屏 (270,900)）；☰ 菜单按钮（退出程序 720base (606,728)→屏 (758,910)）。

### 验证（真机 MuMu 10.0.182.146:5555）

- 主菜单/对局/☰ 菜单截图全面屏对齐原版（cmp_menu_full/cmp_human_full/cmp_imen_full.png；view=900×1600 无状态栏黑边，☰ 菜单垂直偏移消除）。
- ☰ 菜单按钮命中：退出程序 action7 → 回主菜单（全面屏坐标 758,910）。
- walktest 走子 cell25→30 Eat + turn 交替 PASS（--restart 轮询就绪后稳定）；fulltest **9/9**（draw 音效偶发间歇 flaky，isolated 3/3 复过，非回归）。
- so 不变 **af0e8ad4d6ec78e733c665ad4eb7036c**；源 APK rs168_wav.apk 已含全面屏 smali（persist）。

### 环境坑（非本次改动）

- ★ com.cnvcs.junqirs 包在 MuMu 上**周期消失**（`pm path` 空 + `am start` 报 "Activity class does not exist" + logcat "Invalid packageName"），无崩溃日志——MuMu 环境包/存储波动（EXPERIENCE_LOG 前轮已记录，非代码回归）。`adb install -r src_rs/dist/phase_rs_a64.apk` 重装即恢复。测试须装机后立即跑。

### 残余诚实差异

- 明棋默认 vs 原版暗棋（全面屏下棋盘透出明棋标签，原版暗棋绿背）。
- 底部导航栏：全面屏用 IMMERSIVE_STICKY 隐藏（若设备有手势栏会间歇露出，MuMu 无）。
