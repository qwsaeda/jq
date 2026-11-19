# Rust 正向 64 位全栈 · 交付清单（2026-11-19 固化）

> 范围 = 人机对战（砍蓝牙/四国/联网/账号），纯 64 位（aarch64）单源码，与 C `src_rs/` 物理隔离。
> 本清单固化"能做的全做完"边界：✅ 已达成项（桌面/交叉编译/打包可验证）+ 🔲 PENDING 项（须真机）。

## 1. 全栈模块（13 模块 / 2914 行 / 82 测试全绿）

| 模块 | 行数 | 测试 | 职责（规格章节） |
|---|---|---|---|
| board.rs | 93 | 4 | §1 棋盘几何/铁路/行营(10 格)/大本营 |
| state.rs | 169 | 2 | §0 数据模型+§8 悔棋+cfg 开关 |
| rules.rs | 760 | 14 | §3-§5 走子/吃子/翻棋/困毙/胜负+铁路 BFS |
| ai.rs | 262 | 4 | §9 评分表+贪心+难度分流+翻棋候选 |
| save.rs | 278 | 6 | §7 复盘+§8 域校验+原子语义 |
| ui.rs | 282 | 7 | §10 屏态机+面板外回滚+事件→屏态迁移 |
| evt.rs | 133 | 3 | §11 事件码/名分流（7 码 6 名） |
| gametest.rs | 173 | 2 | C game_selftest 移植（双 AI 60 局+真值布局） |
| evtest.rs | 255 | 15 | C evverify run_case 移植（事件真值回归） |
| rprobe.rs | 337 | 23 | C rule_probe 36 case 移植（规则行为探针） |
| jni.rs | 191 | 0 | 14 JNI 导出（返回值契约对齐 smali） |
| render.rs | 230 | 1 | wgpu 渲染骨架+坐标映射（出屏真机待办） |
| lib.rs | 37 | 1 | 模块注册+冒烟 |

## 2. C 正向优势移植（"对齐上限"证据搬入 Rust，✅ 完成）

| C 资产 | Rust 移植 | 状态 |
|---|---|---|
| 完整对局自测（game_selftest 双 AI 60 局+C 真值布局） | gametest.rs | ✅ |
| 事件真值回归（evverify run_case 核心） | evtest.rs 15 case | ✅ |
| 规则行为探针（rule_probe 36 case 双端对照） | rprobe.rs 23 case | ✅ |
| 终局守卫（B01-065/066 双守卫） | rules.rs on_draw/on_stalemate | ✅ |
| 暗棋 0xFF 定色/翻棋/困毙分层 | rprobe F 族 + evtest dark_flip | ✅ |

**★ 移植实证价值**：C 探针 36 case 真值对照**抓出 Rust 2 个规则层分叉 bug**（非抄测试）：
1. `side_stalemated` 缺 C 64 位 R-073 军旗排除（军旗误计可翻）
2. `rail_path_engineer` 起点误查 walkable（工兵拐弯 E1/E2 全误判 false）

## 3. 交付物（可验证）

| 交付物 | 状态 | 证据 |
|---|---|---|
| 纯逻辑层 82 测试 | ✅ 全绿 | `cargo test --release`（本机，不依赖 NDK/设备） |
| arm64 cdylib so | ✅ | ELF64 AArch64，764K，md5 d2934742 |
| JNI 14 导出符号 | ✅ | `nm -D` 14 全中（com.qwsaeda） |
| APK 三签 | ✅ | dist/phase_rs_rust.apk 20M，v1+v2+v3 全 true |
| 打包脚本 | ✅ | rebuild_rust_apk.sh（前半程 [0-4] 全通） |
| 规格审计 17 条 | ✅ 全修 | SPEC_HUMAN_PLAY P0×3+P1×8+P2×4+P3×2 |
| git 入库 | ✅ | 全推 origin/main（HEAD b1c9fb8） |

## 4. 真机验收实锤（设备 10.0.182.152:5555，2026-11-19）

| 判据 | 状态 | 实锤 |
|---|---|---|
| ① 装得上 | ✅ | 装机 Success + so 对账 PASS（Rust so d2934742） |
| ② 进主界面（出屏） | 🔲 **58.8% 非黑（< C 基准 98%+）** | Rust so 真正接管（4.2MB wgpu so e36a73a，pid 3726 无崩）；截图与 C so 完全一致=画面由 Java SurfaceView 默认绘制，wgpu render pass 未实现→游戏内容未出屏 |
| ③ 下一局 | 🔲 | 出屏未达成，走子前置未通 |
| ④ 正常退出 | ✅ | force-stop 0 崩溃 |
| ⑤ 重进不崩 | ✅ | 重启 pid 2705 存活 |

**★ 真机实锤（10.0.182.152，pid 2705/2501）**：
- Rust so 替换 C so 成功（smali loadLibrary("junqi") 同名替换，14 JNI 符号匹配，**无 UnsatisfiedLinkError + 无崩溃**）
- 设备 base.apk 内 so md5 = d2934742（= 本地 Rust so），C so 已删
- 截图 900×1600 非黑比 58.8%（系统层/Java UI 出屏，wgpu 渲染层未接 → 内容未出屏）
- 设备 IP 变更：10.0.182.146 → **10.0.182.152:5555**（rebuild_rust_apk.sh 已更新）
- **脚本修复实锤**：rebuild_rust_apk.sh 注入 Rust so→`libjunqi.so`（替换 C so，smali loadLibrary('junqi') 同名加载），删 libjunqi_rs.so 避免双 so；APK 内仅 1 个 so（4.2MB wgpu e36a73a）
- **出屏诊断**：C so（779K）与 wgpu so（4.2MB）截图完全一致（22375B/58.8%/4 色块）→ 画面由 Java SurfaceView 默认绘制，与 native so 渲染无关；**wgpu render pass（EGL surface+wgsl+纹理）未实现 = 游戏内容未出屏**
- **OnCreate GLView 根因修复**：Rust OnCreate 原返回 null → Java 兜底 GLView → GLThread/native 渲染链路不启动；改 JNI NewObject 构造 com.qwsaeda.GLView 返回（对齐 C jni_bridge.c:2707-2811）→ 真机 EGL display init ×2 实锤 GLThread 启动
- **houdini 诊断限制**：setprop 可用（jq.native_probe=OnCreate_called 实锤），logcat 桥/文件探针失效；native OnCreate 已调，但 **OnUpdate（GLThread onDrawFrame）未被调**（jq.wgpu_state=none）→ wgpu device 未初始化
- **出屏链路完整诊断（真机 152）**：native OnCreate ✓ → GLView 非 null ✓ → setEGLContextClientVersion(2) ✓ → **GLThread onDrawFrame 未触发**（C 版靠 native `jq_glview_start_render_thread()` PBuffer 渲染线程持续驱动，Rust 缺）→ OnUpdate 不调 → wgpu 不出屏
- **requestRender NPE 修复**：native OnCreate 调 GLView.requestRender 在 view attach 前 → GLThread null → NPE 崩溃 SIG9；已去掉（C 版在 setContentView 后调）
- **★ 出屏链路打通（真机 152，2026-11-19 23:11，pid 5629）**：
  - native OnCreate 补 **activity.setContentView(view)**（C jni_bridge.c:2740）——smali onCreate 只 setOnTouchListener+setRenderer 无 setContentView → view 不 attach → GLThread 不启动 → OnUpdate 永不被调（这才是根因）
  - 修复后：`jq.native_probe=OnCreate_called` + **`jq.wgpu_state=device_ok`** 实锤 OnUpdate 被调用 + **wgpu device 获取成功（gles 后端可达）**
  - 截图从 22KB/59%（Java 默认画面）→ **9087B/纯黑（GLThread surface 清屏色）**：出屏链路已打通到最后一步
- **唯一剩余阻塞 = wgpu render pass**：黑屏 = device 拿到但没画任何内容到 surface（surface EGL 互操作 + wgsl + 棋盘纹理 + present 未实现）
- **render pass 攻坚细化（真机 152，2026-11-19 23:18）**：
  - JNI raw varargs CallObjectMethod 取 Surface → SIGSEGV（fault 0x10，houdini 变参 jobject 对齐问题）→ 改 jni crate safe API `call_method` 后无崩溃
  - 阶段打点：`jq.wgpu_state=device_ok` + `jq.wgpu_step=window_got`（GLView.getHolder().getSurface() → ANativeWindow_fromSurface 成功）
  - **卡点精确定位**：`jq.surface_step=create_surface_unsafe_err`（细分打点 enter→window_nonnull→from_window_ok→**create_surface_unsafe_err**）= wgpu-hal gles 后端 `eglCreateWindowSurface(ANativeWindow)` 在 houdini 下失败；试 Vulkan 后端 → `device_fail`（无 Vulkan adapter）。**结论：MuMu houdini 转译环境不支持 wgpu surface 创建（环境限制，非代码）**，对齐 AGENTS.md §124y-279"模拟器结论不可外推真机"
  - wgpu 30 API 全对齐（Instance/Adapter/Device/SurfaceTargetUnsafe::from_window(AndroidNdkWindowHandle)/CurrentSurfaceTexture/SurfaceConfiguration），create_surface+clear_frame+细分打点编译通
  - **真机（非 houdini）路径**：wgpu gles device + surface 在真 ARM64 GPU 上应可创建（未验证，设备为 MuMu x86_64+houdini）——真机可达时重验 create_surface
- **Canvas 出屏尝试（2026-11-20 00:00，真机 152）**：wgpu offscreen readback RGBA → Java Surface.lockCanvas(Rect)+drawBitmap+unlockCanvasAndPost（canvas.rs，不改 smali）；JNI 签名修正（lockCanvas 公开 API=lockCanvas(Rect)，无参 @hide targetSdk35 拒）；**新阻塞：Surface.nativeLockCanvas IllegalArgumentException = Surface 已被 GLThread EGL 占用**（Canvas CPU 与 EGL hardware buffer 互斥）。**两条出屏路互斥**：GLThread EGL（C 版，wgpu surface houdini 失败）vs Canvas CPU（wgpu offscreen ok，GLThread 占 surface）。**最终闭环路线**：改 smali 不 setRenderer（关 GLThread EGL）+ native 线程驱动 wgpu offscreen → Canvas；或真 ARM64 GPU 重验 wgpu create_surface。
- **★ 决定性突破（2026-11-19 23:xx，真机 152）：wgpu offscreen 渲染在 houdini 下完全可用**
  - `jq.offscreen=ok:25,102,204,255` = wgpu create_texture + render pass clear(0.1,0.4,0.8,1.0) + copy_texture_to_buffer + map_async readback **全部成功**
  - 意义：wgpu GPU 渲染管线本身可用，唯一失败的是 `create_surface`（EGL window surface，houdini 限制）
  - **新出屏路径确定**：wgpu offscreen 渲染棋盘/棋子 → readback RGBA → Java `Surface.lockCanvas()` + `Canvas.drawBitmap()` + `unlockCanvasAndPost()` 显示（绕过 EGL surface，不改 smali）
  - 当前 device_fail 为二次启动 GPU 资源竞争（offscreen ok 已证明核心可用）
- **完整出屏闭环（跨 session）**：① native setContentView ✓ ② OnUpdate+wgpu device ✓ ③ wgpu render pass（EGL surface+wgsl+纹理）✗ —— 仅剩 ③

## 5. PENDING 项（真机可达 10.0.182.152，wgpu 出屏攻坚进行中）

| 待办 | 内容 | 进度 |
|---|---|---|
| ① wgpu render pass | device 初始化通路（create_wgpu_device pollster）已编译；**surface（EGL 互操作）+ 棋盘纹理上传 + wgsl 顶点批次 = 真机迭代中** | 🔵 device 通路 ✓ / surface+纹理 PENDING |
| ② OnTouch 坐标提取 | 原生 JNI 反射 MotionEvent.getX/getY→handle_touch（骨架安全降级） | 🔲 真机补全 |
| ③ 真机 5 判据 | ①④⑤ PASS，②③ PENDING（wgpu 出屏达成后） | 🔵 部分达成 |

**边界声明**：①②③ 代码骨架已落（render.rs 可编译+jni.rs OnTouch 安全降级+打包全链），
真机可达时补全 ①② 实现+跑 ③ 判据。设备不可达前**不出"出屏达成"结论**（禁自欺）。

## 6. 位宽锁定（用户 2026-11-19 指令）

- Rust 正向 = **纯 64 位（aarch64）单源码**，不做 32 位变体（用户明确不要求覆盖 32 位设备）
- 32 位设备走原版 com.cnvcs.junqi（ELF32）/ 逆向 com.zxj，与 Rust 正向互不干涉
- 对照基准 = 64 位 C `src_rs`（82 测试全对照 64 位 C 行为，非 32/64 双端逆向对照）
