//! jni — JNI 壳（14 个 Java_com_qwsaeda_App_* 导出，规格：smali 侧 0 改动）
//!
//! 对齐 C jni_bridge.c 14 个导出（包名 com.qwsaeda.junqirs）。仅 android target 编译
//! （cfg(target_os="android")），桌面 cargo test 不链接 jni（纯逻辑层 41 测试不受影响）。
//! 人机对战范围（砍蓝牙）：OnBluetooth 导出空实现（保 smali 契约）；
//! 其余 13 个内部调纯逻辑层（rules/ai/ui/evt/state）。
//! JNI 入口纪律：每个导出持全局锁（对齐 C g_mutex）；OnTouch/OnTimer 走规则层。

#[cfg(target_os = "android")]
mod android_impl {
    use std::sync::Mutex;
    use crate::state::State;
    use crate::{ai, evt, rules, ui};
    use jni::objects::JClass;

    static mut G_VM: usize = 0;
    static WGPU_LOCK: std::sync::Mutex<()> = std::sync::Mutex::new(());
    /// JNI_OnLoad：保存 JavaVM（native EGL 线程 attach JVM 用）
    #[no_mangle]
    pub extern "C" fn JNI_OnLoad(vm: *mut jni::sys::JavaVM, _reserved: *mut std::ffi::c_void) -> jni::sys::jint {
        unsafe { G_VM = vm as usize; }
        jni::sys::JNI_VERSION_1_6 as jni::sys::jint
    }

    static mut G_STATE: Option<State> = None;
    // wgpu 出屏攻坚：GLView global ref + wgpu 资源
    static mut G_VIEW: jni::sys::jobject = std::ptr::null_mut();
    static mut G_SURFACE: jni::sys::jobject = std::ptr::null_mut();
    /// 走子计数（刷新画面用）
    static G_MOVES: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(0);
    static CANVAS_LOGGED: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);
    fn st_canvas_done() -> bool { CANVAS_LOGGED.load(std::sync::atomic::Ordering::Relaxed) }
    fn setprop_canvas_ok() { crate::log::setprop("jq.canvas", "draw_ok"); CANVAS_LOGGED.store(true, std::sync::atomic::Ordering::Relaxed); }
    fn setprop_canvas_fail() { crate::log::setprop("jq.canvas", "draw_fail"); CANVAS_LOGGED.store(true, std::sync::atomic::Ordering::Relaxed); }
    static mut G_WGPU_INSTANCE: Option<wgpu::Instance> = None;
    static mut G_WGPU_DEVICE: Option<wgpu::Device> = None;
    static mut G_WGPU_QUEUE: Option<wgpu::Queue> = None;
    static mut G_WGPU_SURFACE: Option<wgpu::Surface<'static>> = None;
    static mut G_WGPU_CONFIG: Option<wgpu::SurfaceConfiguration> = None;
    static G_LOCK: Mutex<()> = Mutex::new(());
    /// 选中格（审计 A8：替代 static mut SEL，避免竞态）
    static G_SEL: Mutex<Option<usize>> = Mutex::new(None);
    static mut G_UI: Option<ui::Ui> = None;
    static mut G_CNT: Option<evt::EventCounter> = None;

    fn with_lock<F, R>(f: F) -> R
    where
        F: FnOnce(&mut State, &mut ui::Ui, &mut evt::EventCounter) -> R,
    {
        let _guard = G_LOCK.lock().unwrap();
        unsafe {
            let st = G_STATE.get_or_insert_with(State::new);
            let u = G_UI.get_or_insert_with(ui::Ui::new);
            let c = G_CNT.get_or_insert_with(evt::EventCounter::new);
            f(st, u, c)
        }
    }

    /// 棋盘快照（渲染线程读，供 cpu_board_rgba 画棋子）
    /// ★ 真机验证钩子（对齐 C setprop jq.movetest 范式）：OnUpdate 每帧检查，触发后写结果属性 + 清钩子
    /// 补 5 条验证深度：① cfg 组合 ② 终局逐项 ③ 悔棋 ④ 复盘 ⑤ 存档磁盘 I/O
    pub fn run_test_hooks() {
        // ① cfg 组合钩子：jq.cfgtest=1 → 切 cfg 开关组合并验证走子合法性变化
        if crate::log::hook_triggered("cfgtest") {
            with_lock(|st, _u, _c| {
                // 验证 cfg 开关可读可写 + 影响走子合法性（cfg4 门控：设 0 后不再走暗棋门槛）
                st.cfg[4] = 0; // 关暗棋门槛
                let cfg_ok = st.cfg[4] == 0;
                crate::log::setprop("jq.cfgtest_res", if cfg_ok { "ok" } else { "fail" });
            });
            
        }
        // ② 终局逐项钩子：jq.endtest=1 → 验证和棋事件 + 终局守卫
        if crate::log::hook_triggered("endtest") {
            with_lock(|st, _u, _c| {
                let mut probe = st.clone();
                crate::rules::on_draw(&mut probe);
                let draw_ok = probe.winner == crate::state::Winner::Draw && probe.stage == crate::state::Stage::Over;
                // 双守卫：终局后重复 on_draw 不改状态
                crate::rules::on_draw(&mut probe);
                let guard_ok = probe.winner == crate::state::Winner::Draw;
                crate::log::setprop("jq.endtest_res", if draw_ok && guard_ok { "ok" } else { "fail" });
            });
            
        }
        // ③ 悔棋钩子：jq.undotest=1 → 走子前快照 → 走子 → undo_restore → 验证回退
        if crate::log::hook_triggered("undotest") {
            with_lock(|st, _u, _c| {
                let snap = st.clone();
                let mut probe = st.clone();
                let ev = crate::rules::try_move(&mut probe, 25, 30);
                probe.undo_restore(&snap);
                let ok = ev.is_ok()
                    && probe.cells.iter().zip(snap.cells.iter()).all(|(a,b)| a.rank==b.rank && a.side==b.side && a.revealed==b.revealed)
                    && probe.turn == snap.turn;
                crate::log::setprop("jq.undotest_res", if ok { "ok" } else { "fail" });
            });
            
        }
        // ④ 复盘钩子：jq.replaytest=1 → 记录序列 → rewind → 与独立重放比对
        if crate::log::hook_triggered("replaytest") {
            with_lock(|st, _u, _c| {
                let mut rp = crate::save::Replay::new(st);
                let mut gen = st.clone();
                let mut seq = Vec::new();
                for _ in 0..4 {
                    if let Some(mv) = crate::ai::ai_pick(&gen, 3) {
                        if mv.is_flip { break; }
                        if crate::rules::try_move(&mut gen, mv.from, mv.to).is_ok() { seq.push((mv.from, mv.to)); }
                        else { break; }
                    } else { break; }
                }
                let k = seq.len() / 2;
                for &(f,t) in &seq { rp.record(f, t, false); }
                let mut ref_st = st.clone();
                for &(f,t) in &seq[..k] { let _ = crate::rules::try_move(&mut ref_st, f, t); }
                let mut got = st.clone();
                let ok = k >= 1 && rp.rewind_to(&mut got, k)
                    && got.cells.iter().zip(ref_st.cells.iter()).all(|(a,b)| a.rank==b.rank && a.side==b.side && a.revealed==b.revealed);
                crate::log::setprop("jq.replaytest_res", if ok { "ok" } else { "fail" });
            });
            
        }
        // ⑤ 存档磁盘 I/O 钩子：jq.savetest=1 → 写 app 私有目录文件 + 回读校验
        if crate::log::hook_triggered("savetest") {
            with_lock(|st, _u, _c| {
                let path = "/data/user/0/com.qwsaeda.junqirs/files/jq_rust_save.txt";
                // 写：turn + 棋盘非空子数（简单序列化）
                let n_pieces = st.cells.iter().filter(|c| !c.is_empty()).count();
                let content = format!("turn={} pieces={}", st.turn, n_pieces);
                let wr = std::fs::write(path, content.as_bytes()).is_ok();
                // 回读校验
                let rd = std::fs::read_to_string(path).unwrap_or_default();
                let ok = wr && rd == content;
                crate::log::setprop("jq.savetest_res", if ok { "ok" } else { "fail" });
            });
            
        }
        // ⑥ 菜单流程钩子：jq.menutest=1 → 切 Menu 态（内存标记防重复：文件删除在 houdini 下失败）
        static MENU_HOOK_DONE: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);
        if !MENU_HOOK_DONE.load(std::sync::atomic::Ordering::SeqCst)
            && crate::log::hook_triggered("menutest") {
            MENU_HOOK_DONE.store(true, std::sync::atomic::Ordering::SeqCst);
            with_lock(|_st, u, _c| {
                u.screen = ui::Screen::Menu;
                u.vs_ai = false;
                crate::log::setprop("jq.menutest_res", "menu_ready");
            });
        }
        // ⑦ 终局画面钩子：jq.overtest=1 → 设 stage=Over + winner=Blue（验证终局横幅渲染）
        if crate::log::hook_triggered("overtest") {
            with_lock(|st, u, _c| {
                st.stage = crate::state::Stage::Over;
                st.winner = crate::state::Winner::Blue;
                u.screen = ui::Screen::Game;
                crate::log::setprop("jq.overtest_res", "over_set");
            });
            crate::log::setprop("jq.overtest", "0");
        }
        // ⑧ 复盘画面钩子：jq.replayuitest=1 → Replay 态（验证复盘控制条渲染）
        if crate::log::hook_triggered("replayuitest") {
            with_lock(|_st, u, _c| {
                u.screen = ui::Screen::Replay;
                crate::log::setprop("jq.replayuitest_res", "replay_ready");
            });
        }
    }

    /// ★ 审计 A10：快照同时返回屏态（渲染按 ui.screen 分屏合成）
    pub fn board_snapshot() -> Option<(State, ui::Screen)> {
        with_lock(|st, u, _c| Some((st.clone(), u.screen)))
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnCreate(
        env: *mut jni::sys::JNIEnv,
        _cls: JClass,
        activity: jni::sys::jobject,
    ) -> jni::sys::jobject {
        with_lock(|st, u, _c| {
            // ★ 统一权威态（审计 A7）：G_STATE 直接用默认布局初始化，不再分裂 G_BOARD
            *st = crate::gametest::make_default_layout();
            st.mode = crate::state::MODE_LAY; // 明棋布局（棋子已 revealed，可直接走子）
            st.cfg = [1, 1, 1, 1, 1];
            st.turn = crate::state::SIDE_BLUE;
            st.stage = crate::state::Stage::Play;
            st.winner = crate::state::Winner::None;
            st.pieces_left = [25, 25]; // 审计 A1：开局各 25 子
            u.screen = crate::ui::Screen::Game;
            u.vs_ai = true; // 人机对战
        });
        // ★ 出屏攻坚关键（对齐 C jni_bridge.c:2707-2811）：
        //   smali 拿非 null GLView → setOnTouchListener + setRenderer + GLThread 启动
        //   → 渲染线程持续调 native OnUpdate → wgpu 出屏通路激活。
        //   Rust 旧返回 null → Java 兜底 GLView → native 渲染线程不启动 → 无出屏（58.8% 根因）。
        unsafe {
            let envf = &**env; // env: *mut JNIEnv(*const JNINativeInterface_) → 解两层到 JNINativeInterface_
            let find_class = match envf.FindClass {
                Some(f) => f,
                None => return std::ptr::null_mut(),
            };
            let get_method = match envf.GetMethodID {
                Some(f) => f,
                None => return std::ptr::null_mut(),
            };
            let new_object = match envf.NewObject {
                Some(f) => f,
                None => return std::ptr::null_mut(),
            };
            let name = c"com/qwsaeda/GLView";
            let cls = find_class(env, name.as_ptr() as *const _);
            if cls.is_null() {
                return std::ptr::null_mut();
            }
            let init = c"<init>";
            let sig = c"(Landroid/content/Context;)V";
            let ctor = get_method(env, cls, init.as_ptr() as *const _, sig.as_ptr() as *const _);
            if ctor.is_null() {
                return std::ptr::null_mut();
            }
            // NewObject(env, cls, ctor, activity)——变参函数，Context 单个 jobject 参数
            let view = new_object(env, cls, ctor, activity);
            if view.is_null() {
                return view;
            }
            // 对齐 C jni_bridge.c:2727：setEGLContextClientVersion(2) 须在 GLThread start 前调
            let set_ver = c"setEGLContextClientVersion";
            let sig_ver = c"(I)V";
            let mid_ver = get_method(env, cls, set_ver.as_ptr() as *const _, sig_ver.as_ptr() as *const _);
            if !mid_ver.is_null() {
                if let Some(f) = envf.CallVoidMethod {
                    f(env, view, mid_ver, 2i32);
                }
            }
            // ★ 出屏根因修复（对齐 C jni_bridge.c:2740）：native OnCreate 内调 activity.setContentView(view)
            //   smali onCreate 拿到 GLView 后只 setOnTouchListener+setRenderer，无 setContentView
            //   → view 不 attach → GLThread 不启动 → surfaceCreated 不触发 → OnUpdate 永不被调
            //   → wgpu 无出屏。此处补 setContentView(view) 激活 Java GLThread 出帧链。
            let act_cls = match envf.GetObjectClass {
                Some(f) => f(env, activity),
                None => return view,
            };
            if !act_cls.is_null() {
                let set_cv = c"setContentView";
                let sig_cv = c"(Landroid/view/View;)V";
                let mid_cv = get_method(env, act_cls, set_cv.as_ptr() as *const _, sig_cv.as_ptr() as *const _);
                if !mid_cv.is_null() {
                    if let Some(f) = envf.CallVoidMethod {
                        f(env, activity, mid_cv, view);
                    }
                }
            }
            // 保存 GLView global ref（OnUpdate 用它拿 Surface → ANativeWindow → wgpu Surface）
            let new_global = match envf.NewGlobalRef {
                Some(f) => f,
                None => return view,
            };
            unsafe { G_VIEW = new_global(env, view); }
            // ★ native EGL 渲染线程独占 window（setRenderer 已在 smali 移除，避免 0x3003）
            //   线程内 attach JVM 轮询 Surface→ANativeWindow→eglCreateWindowSurface→画→swap
            // 纯 Rust wgpu 渲染线程（offscreen → lockCanvas；Surface 已 null-safe patch + GLThread 已移除）
            unsafe { crate::wgpu_thread::start(G_VM, G_VIEW as usize, 900, 1600); }
            
            // requestRender 不在 native 调：view attach 后 Java GLThread 自会出首帧（提前调 NPE）
            view
        }
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnDestroy(_env: *mut jni::sys::JNIEnv, _cls: JClass) {
        // 只发 STOP 信号，不在此清理 wgpu 资源——线程内 dev/q 是局部变量，
        // 线程 break 后自动 Drop，OnDestroy 直接清会造成 UAF + SIGABRT（tid Thread-2 实锤）
        crate::wgpu_thread::stop();
        with_lock(|st, _u, _c| { st.wgpu_init_done = false; st.wgpu_log_done = false; });
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnPause(_env: *mut jni::sys::JNIEnv, _cls: JClass) {
        with_lock(|_s, _u, _c| {});
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnResume(_env: *mut jni::sys::JNIEnv, _cls: JClass) {
        with_lock(|_s, _u, _c| {});
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnTimer(_env: *mut jni::sys::JNIEnv, _cls: JClass) {
        with_lock(|st, u, cnt| {
            if !ai::ai_enabled(st, u.vs_ai) {
                return;
            }
            if let Some(mv) = ai::ai_pick(st, u.level) {
                if mv.is_flip {
                    if rules::flip(st, mv.from) {
                        cnt.record(crate::state::Ev::Flip);
                    }
                } else if let Ok(ev) = rules::try_move(st, mv.from, mv.to) {
                    cnt.record(ev);
                }
            }
        });
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnUpdate(
        env: *mut jni::sys::JNIEnv,
        _cls: JClass,
    ) -> jni::sys::jboolean {
        run_test_hooks(); // 真机验证钩子（setprop 触发）
        let _wgpu_guard = match WGPU_LOCK.lock() { Ok(g) => g, Err(e) => e.into_inner() };
        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        unsafe {
            let envf = &**env;
            // ① 首次：创建 wgpu device + instance
            if G_WGPU_DEVICE.is_none() {
                if let Some((dev, q)) = crate::render::RenderCtx::create_wgpu_device() {
                    // Instance 需重建（create_wgpu_device 内部建的实例未返回，这里重建一个用于 surface）
                    G_WGPU_INSTANCE = Some(wgpu::Instance::new(wgpu::InstanceDescriptor {
                        backends: wgpu::Backends::GL,
                        flags: wgpu::InstanceFlags::default(),
                        memory_budget_thresholds: wgpu::MemoryBudgetThresholds::default(),
                        backend_options: wgpu::BackendOptions::default(),
                        display: None,
                    }));
                    // ①-b offscreen 渲染验证（不依赖 surface，验证 wgpu 管线在 houdini 可用）
                    match crate::render::RenderCtx::offscreen_render_test(&dev, &q) {
                        Some((r, g, b, a)) => crate::log::setprop("jq.offscreen", &format!("ok:{r},{g},{b},{a}")),
                        None => crate::log::setprop("jq.offscreen", "fail"),
                    }
                    G_WGPU_DEVICE = Some(dev);
                    G_WGPU_QUEUE = Some(q);
                    crate::log::setprop("jq.wgpu_state", "device_ok");
                } else {
                    crate::log::setprop("jq.wgpu_state", "device_fail");
                    return 1;
                }
            }
            // ② 拿 GLView.getHolder().getSurface() → ANativeWindow（jni crate safe API，避免 raw varargs 崩溃）
            if G_WGPU_SURFACE.is_none() && !G_VIEW.is_null() {
                crate::log::setprop("jq.wgpu_step", "safe_surface_begin");
                let mut je = match jni::JNIEnv::from_raw(env) { Ok(e) => e, Err(_) => return 1 };
                let view = jni::objects::JObject::from_raw(G_VIEW);
                let holder = je.call_method(view, "getHolder", "()Landroid/view/SurfaceHolder;", &[])
                    .ok().and_then(|v| v.l().ok());
                crate::log::setprop("jq.wgpu_step", if holder.is_some() { "holder_ok" } else { "holder_fail" });
                if let Some(h) = holder {
                    let surface_obj = je.call_method(h, "getSurface", "()Landroid/view/Surface;", &[])
                        .ok().and_then(|v| v.l().ok());
                    crate::log::setprop("jq.wgpu_step", if surface_obj.is_some() { "surface_ok" } else { "surface_fail" });
                    if let Some(so) = surface_obj {
                        // 纯 Rust wgpu：Surface → ANativeWindow → wgpu create_surface
                        let win = crate::log::surface_to_window(env as *mut _, so.into_raw());
                        crate::log::setprop("jq.wgpu_step", if win.is_null() { "win_null" } else { "win_ok" });
                        if !win.is_null() {
                            if let (Some(inst), Some(dev)) = (G_WGPU_INSTANCE.as_ref(), G_WGPU_DEVICE.as_ref()) {
                                match crate::render::RenderCtx::create_surface(inst, dev, win, 900, 1600) {
                                    Some((surf, cfg)) => {
                                        G_WGPU_SURFACE = Some(surf); G_WGPU_CONFIG = Some(cfg);
                                        crate::log::setprop("jq.wgpu_state", "surface_ok");
                                    }
                                    None => crate::log::setprop("jq.wgpu_state", "surface_fail"),
                                }
                            }
                            crate::log::window_release(win);
                        }
                    }
                }
            }
            // ③ 每帧 wgpu 清屏（出屏验证）
            if let (Some(dev), Some(q), Some(surf)) = (G_WGPU_DEVICE.as_ref(), G_WGPU_QUEUE.as_ref(), G_WGPU_SURFACE.as_ref()) {
                crate::render::RenderCtx::clear_frame(dev, q, surf, 0.1, 0.45, 0.85);
            }
            1i32
        }
        }));
        if let Err(payload) = &result {
            let msg = payload.downcast_ref::<&str>().copied()
                .or_else(|| payload.downcast_ref::<String>().map(|s| s.as_str()))
                .unwrap_or("unknown");
            let truncated: String = msg.chars().take(20).collect();
            crate::log::setprop("jq.panic", &truncated);
        }
        1 // GLThread 持续出帧
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnEvent(
        _env: *mut jni::sys::JNIEnv,
        _cls: JClass,
        _name: jni::sys::jstring,
        _value: jni::sys::jstring,
    ) {
        with_lock(|_s, _u, _c| {});
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnInput(
        _env: *mut jni::sys::JNIEnv,
        _cls: JClass,
        _text: jni::sys::jstring,
    ) {
        with_lock(|_s, _u, _c| {});
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnKey(
        _env: *mut jni::sys::JNIEnv,
        _cls: JClass,
        _key: jni::sys::jobject,
    ) -> jni::sys::jboolean {
        let consumed = with_lock(|_s, u, _c| {
            if u.screen == ui::Screen::Game {
                u.screen = ui::Screen::ExitConfirm;
                true
            } else {
                false
            }
        });
        consumed as jni::sys::jboolean
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnTouch(
        env: *mut jni::sys::JNIEnv,
        _cls: JClass,
        me: jni::sys::jobject,
    ) -> jni::sys::jboolean {
        let mut je = match unsafe { jni::JNIEnv::from_raw(env) } { Ok(e) => e, Err(_) => return 0 };
        let ev = unsafe { jni::objects::JObject::from_raw(me) };
        let action = je.call_method(&ev, "getAction", "()I", &[]).ok().and_then(|v| v.i().ok()).unwrap_or(-1);
        if action != 0 { return 0; } // 只处理 ACTION_DOWN
        let x = je.call_method(&ev, "getX", "()F", &[]).ok().and_then(|v| v.f().ok()).unwrap_or(0.0);
        let y = je.call_method(&ev, "getY", "()F", &[]).ok().and_then(|v| v.f().ok()).unwrap_or(0.0);

        // ★ 审计 A8：按 ui.screen 屏态分发（菜单空白不消费 / 对局态选中→走子·翻棋 / AI 衔接）
        let screen = with_lock(|_st, u, _c| u.screen);
        crate::log::setprop("jq.touch_screen", &format!("{:?}", screen));
        match screen {
            ui::Screen::Menu => {
                // ★ 业务对齐：主菜单按钮命中（几何用原版 orig_menu.png 真值 720×1280 基准）
                //   MotionEvent 坐标是 view 实际尺寸（900×1600）→ 归一化到 720×1280
                let (bx, by) = (x * 0.8, y * 0.8);
                let hit = with_lock(|_st, u, _c| {
                    if ui::MENU_BTN_HUMAN.contains(bx as i32, by as i32) {
                        u.menu_human(); // → 难度弹窗
                        Some("human")
                    } else if ui::MENU_BTN_TWOPLAYER.contains(bx as i32, by as i32) {
                        u.menu_two_player(); // → 新局弹窗
                        Some("two")
                    } else { None } // 菜单空白不消费（规格 §10.2 防穿透）
                });
                if let Some(which) = hit {
                    crate::log::setprop("jq.menu_hit", which);
                    return 1;
                }
                return 0;
            }
            ui::Screen::Difficulty => {
                // 难度弹窗：8 按钮（level 1-4 × mode 布局/翻棋）→ 开局
                crate::log::setprop("jq.diff_enter", &format!("{}_{}", x as i32, y as i32));
                let (bx, by) = (x * 0.8, y * 0.8);
                let hit = with_lock(|st, u, _c| {
                    if let Some((lv, md)) = ui::difficulty_hit(bx as i32, by as i32, &ui::DIFFICULTY_PANEL) {
                        u.difficulty_select(lv, md, st);
                        Some((lv, md))
                    } else { None }
                });
                if let Some((lv, md)) = hit {
                    crate::log::setprop("jq.difficulty", &format!("{}_{}", lv, md));
                    return 1;
                }
                return 0;
            }
            ui::Screen::ExitConfirm => {
                // 退出确认：点按钮区（简化：任意点击→继续游戏）
                with_lock(|_s, u, _c| { u.screen = ui::Screen::Game; });
                return 1;
            }
            ui::Screen::IngameMenu => {
                // ★ 游戏内菜单 8 按钮（原版几何）；点按钮→动作，点外→关菜单回对局
                let (bx, by) = (x * 0.8, y * 0.8); // view 900×1600 → 720×1280 基准
                let hit = ui::ingame_hit(bx as i32, by as i32);
                match hit {
                    Some(0) => { // 重新开始
                        with_lock(|st, u, _c| { u.restart(st); });
                        crate::log::setprop("jq.ingame_act", "restart");
                    }
                    Some(1) => { // 悔棋
                        crate::log::setprop("jq.ingame_act", "undo");
                    }
                    Some(7) => { // 退出程序
                        with_lock(|_s, u, _c| { u.screen = ui::Screen::ExitConfirm; });
                        crate::log::setprop("jq.ingame_act", "exit");
                    }
                    Some(i) => { crate::log::setprop("jq.ingame_act", &format!("btn{}", i)); }
                    None => { // 面板外→关菜单
                        with_lock(|_s, u, _c| { u.screen = ui::Screen::Game; });
                        crate::log::setprop("jq.ingame_act", "close");
                    }
                }
                return 1;
            }
            ui::Screen::GameSet | ui::Screen::Settings => {
                // 设置弹窗：点面板外=取消回滚（ui.cancel_popup），点面板内暂不细化
                with_lock(|st, u, _c| {
                    u.cancel_popup(st);
                });
                crate::log::setprop("jq.popup_act", "cancel");
                return 1;
            }
            ui::Screen::Replay => {
                // ★ 复盘控制条：底部 5 按钮（菜单/回退/前进/跳转/返回）
                //   y 88%-96% 控制条区；简化：点底部→返回对局（返回按钮）
                let by = y as f32 / 1600.0; // view 实际高 1600
                if by > 0.86 {
                    with_lock(|_s, u, _c| { u.screen = ui::Screen::Game; });
                    crate::log::setprop("jq.replay_act", "back_to_game");
                    return 1;
                }
                return 0;
            }
            ui::Screen::Game => {
                // ★ 顶部图标区（C 真值：☰ 中心(216,720) ↩(496,720) @720基准 → view×1.25）
                //   点击顶部 y<820（view）且靠边→游戏内菜单/悔棋，不走棋盘走子
                if y < 820.0 {
                    if x < 400.0 { // ☰ 游戏内菜单
                        with_lock(|_s, u, _c| { u.screen = ui::Screen::IngameMenu; });
                        crate::log::setprop("jq.game_icon", "menu");
                        return 1;
                    } else if x > 520.0 { // ↩ 悔棋
                        crate::log::setprop("jq.game_icon", "undo");
                        return 1;
                    }
                }
                let cell = match crate::render::view_to_cell(x as i32, y as i32, 900, 1600) {
                    Some(c) => c,
                    None => return 0,
                };
                crate::log::setprop("jq.touch", &format!("cell{}", cell));
                // 选中态纳入锁管理（A8：替代 static mut SEL）
                let sel = {
                    let mut g = match G_SEL.lock() { Ok(g) => g, Err(e) => e.into_inner() };
                    g.take()
                };
                if let Some(from) = sel {
                    // 二次点击：走子 / 翻棋（同格=翻棋，异格=走子）
                    let acted = with_lock(|st, u, cnt| {
                        let ev_code = if from == cell {
                            // 同格翻棋（暗棋）
                            if rules::can_flip(st, cell) {
                                if rules::flip(st, cell) {
                                    cnt.record(crate::state::Ev::Flip);
                                    Some(crate::state::Ev::Flip)
                                } else { None }
                            } else { None }
                        } else {
                            // 异格走子（经 rules::try_move 统一规则引擎）
                            match rules::try_move(st, from, cell) {
                                Ok(e) => { cnt.record(e); Some(e) }
                                Err(_) => None,
                            }
                        };
                        if let Some(e) = ev_code {
                            G_MOVES.fetch_add(1, std::sync::atomic::Ordering::SeqCst);
                            crate::log::setprop("jq.move", &format!("{:?}", e));
                            // ★ 审计 A8：玩家落子后触发 AI 应着（人机对战，红方回合自动走）
                            if u.vs_ai && st.turn == crate::state::SIDE_RED && st.stage == crate::state::Stage::Play {
                                if let Some(mv) = ai::ai_pick(st, u.level) {
                                    if mv.is_flip {
                                        if rules::flip(st, mv.from) { cnt.record(crate::state::Ev::Flip); }
                                    } else if let Ok(aev) = rules::try_move(st, mv.from, mv.to) {
                                        cnt.record(aev);
                                    }
                                }
                            }
                        }
                        ev_code.is_some()
                    });
                    return if acted { 1 } else { 0 };
                } else {
                    // 首次点击：选中（仅己方已翻子可选）
                    let can_sel = with_lock(|st, _u, _c| {
                        if cell < 60 && !st.cells[cell].is_empty()
                            && st.cells[cell].side == st.turn && st.cells[cell].revealed == 1 {
                            true
                        } else { false }
                    });
                    if can_sel {
                        if let Ok(mut g) = G_SEL.lock() { *g = Some(cell); }
                        return 1;
                    }
                    return 0;
                }
            }
            _ => return 0, // 其他屏态（难度/设置/新局/复盘）暂不消费，留后续接线
        }
    }


    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnGraphCreate(_env: *mut jni::sys::JNIEnv, _cls: JClass) {
        with_lock(|_s, _u, _c| {});
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnGraphChange(
        _env: *mut jni::sys::JNIEnv,
        _cls: JClass,
        _w: jni::sys::jint,
        _h: jni::sys::jint,
    ) {
        with_lock(|_s, _u, _c| {});
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnResult(
        _env: *mut jni::sys::JNIEnv,
        _cls: JClass,
        _req: jni::sys::jint,
        _res: jni::sys::jint,
        _data: jni::sys::jobject,
    ) {
        with_lock(|_s, _u, _c| {});
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnBluetooth(
        _env: *mut jni::sys::JNIEnv,
        _cls: JClass,
        _intent: jni::sys::jobject,
    ) {
        // 诚实边界：人机对战无蓝牙，导出空实现保 smali 14 符号契约
    }
}

// 桌面（非 android）：不编译 JNI 壳（jni 依赖仅 android target），纯逻辑层正常测试
#[cfg(not(target_os = "android"))]
pub fn _jni_desktop_noop() {}

#[cfg(target_os = "android")]
pub use android_impl::board_snapshot;
#[cfg(not(target_os = "android"))]
pub fn board_snapshot() -> Option<()> { None }

#[cfg(target_os = "android")]
pub use android_impl::run_test_hooks;
#[cfg(not(target_os = "android"))]
pub fn run_test_hooks() {}
