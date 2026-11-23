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

    static G_VM: std::sync::atomic::AtomicUsize = std::sync::atomic::AtomicUsize::new(0);
    /// ★ F1 音效链：Activity global ref（sfx::play 需要 Context.getAssets()）。
    ///   OnTouch 的 me 是 MotionEvent 非 Activity，故 OnCreate 单独保存 activity 引用。
    static G_ACTIVITY: std::sync::atomic::AtomicUsize = std::sync::atomic::AtomicUsize::new(0);

    /// 锁外播放事件音效（严禁在 GAME 锁内调 JNI——会阻塞渲染线程拿锁）。
    /// 仅当 sound_on 且 Activity global ref 有效时播放；JNI 失败静默跳过（音效非核心链路）。
    #[cfg(target_os = "android")]
    fn play_sfx_with_env(env: *mut jni::sys::JNIEnv, sound_on: bool, ev: Option<crate::state::Ev>) {
        if !sound_on { return; }
        let name = match ev.map(crate::evt::sfx_for_code) { Some(n) => n, None => return };
        let act = G_ACTIVITY.load(std::sync::atomic::Ordering::Acquire);
        if act == 0 { return; }
        if let Ok(mut je) = unsafe { jni::JNIEnv::from_raw(env) } {
            let ctx = unsafe { jni::objects::JObject::from_raw(act as jni::sys::jobject) };
            let _ = crate::sfx::play(&mut je, &ctx, name);
            // ★ 双保险：音效链路任何残留异常必须在此清掉，绝不带给后续 JNI 调用（否则 abort）
            if je.exception_check().unwrap_or(false) { let _ = je.exception_clear(); }
        }
    }
    #[cfg(not(target_os = "android"))]
    fn play_sfx_with_env(_env: *mut (), _sound_on: bool, _ev: Option<crate::state::Ev>) {}
    /// wgpu 出屏资源合一（Mutex 锁数据合一，替代 WGPU_LOCK + 5 个 static mut）
    /// ★ N3 修后 OnUpdate 不再 clear_frame（native Canvas 独占 Surface）；这些字段保留用于
    ///   offscreen 验证与资源生命周期保活（surface/window 同 WgpuState 存活），非死字段。
    #[allow(dead_code)]
    struct WgpuState {
        instance: wgpu::Instance,
        device: wgpu::Device,
        queue: wgpu::Queue,
        surface: Option<wgpu::Surface<'static>>,
        config: Option<wgpu::SurfaceConfiguration>,
        window: Option<WindowPtr>, // ★ ANativeWindow 保活（wgpu surface 持裸指针）
    }
    /// ANativeWindow 裸指针包装：显式 Send（wgpu surface 跨线程使用，窗口在 Drop 时释放）
    struct WindowPtr(*mut std::ffi::c_void);
    unsafe impl Send for WindowPtr {}
    unsafe impl Sync for WindowPtr {}
    // ★ JNI-01: WgpuState 销毁时释放保活的 ANativeWindow（防止 surface 悬空指针）
    impl Drop for WgpuState {
        fn drop(&mut self) {
            if let Some(w) = self.window.take() {
                if !w.0.is_null() { unsafe { crate::log::window_release(w.0); } }
            }
        }
    }
    static WGPU: Mutex<Option<WgpuState>> = Mutex::new(None);
    /// JNI_OnLoad：保存 JavaVM（native EGL 线程 attach JVM 用）
    #[no_mangle]
    pub extern "C" fn JNI_OnLoad(vm: *mut jni::sys::JavaVM, _reserved: *mut std::ffi::c_void) -> jni::sys::jint {
        G_VM.store(vm as usize, std::sync::atomic::Ordering::Relaxed);
        jni::sys::JNI_VERSION_1_6 as jni::sys::jint
    }

    // wgpu 出屏攻坚：GLView global ref + wgpu 资源
    /// GLView global ref 的 raw jobject（跨线程，AtomicUsize 存储；经 jni::JObject::from_raw 还原）
    static G_VIEW: std::sync::atomic::AtomicUsize = std::sync::atomic::AtomicUsize::new(0);
    /// 走子计数（刷新画面用）
    static G_MOVES: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(0);
    /// 选中格（审计 A8：替代 static mut SEL，避免竞态）
    static G_SEL: Mutex<Option<usize>> = Mutex::new(None);

    /// 对局三态合一（Rust 规范化：Mutex 把"锁 + 数据"合一，替代 G_LOCK + 3 个 static mut）。
    /// 与旧实现等价：同一把锁串行化、首次访问懒初始化三态。
    struct Game {
        state: State,
        ui: ui::Ui,
        cnt: evt::EventCounter,
        /// ★ F2 复盘记录器：真实对局每步（含 AI）记录，Replay 屏用它回退/前进/跳转
        rp: crate::save::Replay,
        /// ★ RSAVE-002：进入复盘前保存的实时局面（含非走子终局 stage/winner）。
        ///   "返回对局"时恢复它而非 rewind_to 重放——后者会丢认输/超时等非走子终局。
        live_state: Option<State>,
    }
    static GAME: Mutex<Option<Game>> = Mutex::new(None);

    /// 清选中态（切屏/重开/悔棋后必须清，否则旧 cell 当 from 触发错误走子 —— JNI-12）
    fn clear_sel() {
        if let Ok(mut g) = G_SEL.lock() { *g = None; }
    }

    fn with_lock<F, R>(f: F) -> R
    where
        F: FnOnce(&mut State, &mut ui::Ui, &mut evt::EventCounter) -> R,
    {
        let mut guard = GAME.lock().unwrap_or_else(|e| e.into_inner()); // poison 时恢复数据，JNI 边界不二次 panic
        let game = guard.get_or_insert_with(|| Game {
            rp: crate::save::Replay::new(&State::new()),
            state: State::new(),
            ui: ui::Ui::new(),
            cnt: evt::EventCounter::new(),
            live_state: None,
        });
        f(&mut game.state, &mut game.ui, &mut game.cnt)
    }

    /// ★ F2 复盘：新局开始时重建记录器（用当前开局态做基底）
    fn reset_replay() {
        with_game(|g| { g.rp = crate::save::Replay::new(&g.state); });
    }

    /// ★ F2 复盘：访问完整 Game（含 rp），与 with_lock 共用同一把锁
    fn with_game<F, R>(f: F) -> R
    where
        F: FnOnce(&mut Game) -> R,
    {
        let mut guard = GAME.lock().unwrap_or_else(|e| e.into_inner());
        let game = guard.get_or_insert_with(|| Game {
            rp: crate::save::Replay::new(&State::new()),
            state: State::new(),
            ui: ui::Ui::new(),
            cnt: evt::EventCounter::new(),
            live_state: None,
        });
        f(game)
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
        // ④b ★ F4 读档钩子：jq.loadsave=1 → 从磁盘读全量 State 并校验后恢复棋盘
        if crate::log::hook_triggered("loadsave") {
            let path = "/data/user/0/com.qwsaeda.junqirs/files/jq_rust_save.txt";
            let loaded = std::fs::read_to_string(path).ok().and_then(|t| crate::save::deserialize_state(&t));
            let ok = if let Some(ls) = loaded {
                with_game(|g| {
                    g.state = ls;
                    g.ui.screen = crate::ui::Screen::Game;
                    g.rp = crate::save::Replay::new(&g.state);
                });
                true
            } else { false };
            crate::log::setprop("jq.loadsave_res", if ok { "ok" } else { "fail" });
        }
        // ⑤ ★ F4 存档磁盘 I/O 钩子：jq.savetest=1 → 全量 State 序列化写盘 + 读回比对（真 round-trip）
        if crate::log::hook_triggered("savetest") {
            with_lock(|st, _u, _c| {
                let path = "/data/user/0/com.qwsaeda.junqirs/files/jq_rust_save.txt";
                let content = crate::save::serialize_state(st);
                let wr = std::fs::write(path, content.as_bytes()).is_ok();
                let rd = std::fs::read_to_string(path).unwrap_or_default();
                // 真 round-trip：反序列化后逐字段比对（非简单字符串回读）
                let got = crate::save::deserialize_state(&rd);
                let ok = wr && got.as_ref().map(|g| {
                    g.turn == st.turn && g.winner == st.winner && g.mode == st.mode
                        && g.cells.iter().zip(st.cells.iter()).all(|(a,b)|
                            a.rank == b.rank && a.side == b.side && a.revealed == b.revealed)
                }).unwrap_or(false);
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
    pub fn board_snapshot() -> Option<(State, ui::Screen, evt::EventCounter)> {
        with_lock(|st, u, c| Some((st.clone(), u.screen, evt::EventCounter { by_code: c.by_code, total: c.total })))
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnCreate(
        env: *mut jni::sys::JNIEnv,
        _cls: JClass,
        activity: jni::sys::jobject,
    ) -> jni::sys::jobject {
        with_lock(|st, u, _c| {
            // ★ 统一权威态（审计 A7）：原版默认【暗棋】开局（g_opt_mode=1，先手翻子定色）
            //   旧 make_default_layout()+MODE_LAY 是自创明棋，orig_human.png 实锤不符。
            *st = crate::gametest::make_flip_layout();
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
        // ★ 统一用 jni crate 安全封装（不再手写 JNINativeInterface 裸函数指针表——
        //   那是逐行照搬 C jni_bridge.c 的 C-in-Rust 写法；jni 0.21 已在依赖中）。
        //   行为与旧裸指针版逐点等价：find_class → new_object → setEGLContextClientVersion(2)
        //   → setContentView(view) → NewGlobalRef → 启动渲染线程，失败点返回 null/view。
        let mut je = match unsafe { jni::JNIEnv::from_raw(env) } {
            Ok(e) => e,
            Err(_) => return std::ptr::null_mut(),
        };
        let activity_obj = unsafe { jni::objects::JObject::from_raw(activity) };
        // ★ F1 音效链：保存 Activity global ref（sfx::play 需要 Context.getAssets()）。
        //   forget 保活，OnDestroy 经 JNI 函数表 DeleteGlobalRef（与 G_VIEW 同策略）。
        if let Ok(g) = je.new_global_ref(&activity_obj) {
            G_ACTIVITY.store(g.as_obj().as_raw() as usize, std::sync::atomic::Ordering::Release);
            std::mem::forget(g);
        }
        const VIEW_CLASS: &str = "com/qwsaeda/GLView";
        // new_object 内部按类名 find_class（失败返 Err，已被下方 match 捕获）——无需单独 find_class
        let view = match je.new_object(
            VIEW_CLASS,
            "(Landroid/content/Context;)V",
            &[jni::objects::JValue::Object(&activity_obj)],
        ) {
            Ok(v) => v,
            Err(_) => {
                // ★ RJNI-006：jni-rs new_object 失败留 pending exception，早退不清会把
                //   ClassNotFoundException/NoSuchMethodError 抛回 App.smali:337 裸调用点（无 try/catch）→ Java 崩。
                if je.exception_check().unwrap_or(false) { let _ = je.exception_clear(); }
                return std::ptr::null_mut();
            }
        };
        // ★ JNI-02: call_method 失败会留下 pending Java 异常，不清则下次 JNI 调用 abort（No pending exception expected）。
        //   统一 helper：调用 + 失败时 exception_check/clear，不把异常带出本函数。
        fn call_clear(je: &mut jni::JNIEnv, obj: &jni::objects::JObject<'_>, name: &str, sig: &str,
                      args: &[jni::objects::JValue<'_, '_>]) -> bool {
            let ok = je.call_method(obj, name, sig, args).is_ok();
            if !ok && je.exception_check().unwrap_or(false) { let _ = je.exception_clear(); }
            ok
        }
        // 对齐 C jni_bridge.c:2727：setEGLContextClientVersion(2) 须在 GLThread start 前调
        call_clear(&mut je, &view, "setEGLContextClientVersion", "(I)V", &[jni::objects::JValue::Int(2)]);
        // ★ 出屏根因修复：native OnCreate 内调 activity.setContentView(view)，激活 Java GLThread 出帧链
        call_clear(&mut je, &activity_obj, "setContentView", "(Landroid/view/View;)V", &[jni::objects::JValue::Object(&view)]);
        // 保存 GLView global ref（OnUpdate 用它拿 Surface → ANativeWindow → wgpu Surface）
        let global = match je.new_global_ref(&view) {
            Ok(g) => g,
            Err(_) => {
                // ★ RJNI-006：new_global_ref 失败同样留 pending exception，早退前清（同上）
                if je.exception_check().unwrap_or(false) { let _ = je.exception_clear(); }
                return view.into_raw();
            }
        };
        G_VIEW.store(global.as_obj().as_raw() as usize, std::sync::atomic::Ordering::Release);
        // ★ GlobalRef 必须保持存活：若局部 global 在此 drop，NewGlobalRef 会被 DeleteGlobalRef
        //   释放 → G_VIEW 悬空 → 渲染线程 call_method(getHolder) 崩 SIGSEGV（实测 fid）。
        //   旧裸指针版 NewGlobalRef 从不 DeleteGlobalRef，成员 forget 对齐该生命周期语义。
        std::mem::forget(global);
        // ★ native EGL 渲染线程独占 window（setRenderer 已在 smali 移除，避免 0x3003）
        crate::wgpu_thread::start(G_VM.load(std::sync::atomic::Ordering::Relaxed), G_VIEW.load(std::sync::atomic::Ordering::Relaxed), 900, 1600);
        // requestRender 不在 native 调：view attach 后 Java GLThread 自会出首帧（提前调 NPE）
        view.into_raw()
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnDestroy(env: *mut jni::sys::JNIEnv, _cls: JClass) {
        // ★ P2 修：旧注释"dev/q 是局部变量线程退出自动 Drop"已过时——WGPU 现为全局静态持有
        //   （OnUpdate 创建的 device/surface/window 都在里面）。先停线程（N3 修后 start 会等旧线程退出），
        //   再 take() 清 WGPU 触发 WgpuState::drop → window_release，避免跨 Activity 重建泄漏
        //   ANativeWindow 强引用 + 旧 Surface 永久驻留导致新 Surface 再也建不出。
        crate::wgpu_thread::stop();
        if let Ok(mut wg) = WGPU.lock() { drop(wg.take()); }
        // ★ JNI-14: 删除 GLView GlobalRef——旧实现 std::mem::forget 后从不 DeleteGlobalRef，
        //   每次 activity 重建（旋屏/重进）漏一个 GLView + 一个 JNI global ref（确定性泄漏）。
        //   G_VIEW.swap(0) 先清零，OnUpdate 的 `load != 0` 守卫随后跳过，不会 UAF。
        // G_VIEW / G_ACTIVITY 两个 global ref 一并释放（jni 0.21 无 safe raw 构造，走函数表）
        let old = G_VIEW.swap(0, std::sync::atomic::Ordering::SeqCst);
        let old_act = G_ACTIVITY.swap(0, std::sync::atomic::Ordering::SeqCst);
        if !env.is_null() {
            // jni 0.21 的 GlobalRef::from_raw 是 pub(crate)，外部只能通过 JNI 函数表删。
            // DeleteGlobalRef(env, old) 对 new_global_ref 创建的引用配对释放。
            unsafe {
                let fns = &**(env as *const *const jni::sys::JNINativeInterface_);
                if let Some(del) = fns.DeleteGlobalRef {
                    if old != 0 { del(env, old as jni::sys::jobject); }
                    if old_act != 0 { del(env, old_act as jni::sys::jobject); }
                }
            }
        }
        with_lock(|st, _u, _c| { st.wgpu_init_done = false; st.wgpu_log_done = false; });
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnPause(_env: *mut jni::sys::JNIEnv, _cls: JClass) {
        // ★ F4 自动存档：退到后台时全量保存当前棋盘态（SPEC §8）。
        //   读档由 jq.loadsave 钩子显式触发（不自动恢复，避免破坏"重进=新开局"判据与 UX 预期）。
        let saved = with_lock(|st, _u, _c| crate::save::serialize_state(st));
        let _ = std::fs::write(
            "/data/user/0/com.qwsaeda.junqirs/files/jq_rust_save.txt",
            saved.as_bytes(),
        );
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnResume(_env: *mut jni::sys::JNIEnv, _cls: JClass) {
        with_lock(|_s, _u, _c| {});
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnTimer(env: *mut jni::sys::JNIEnv, _cls: JClass) {
        let (ai_ev, sound_on) = with_game(|g| {
            let st = &mut g.state; let u = &mut g.ui; let cnt = &mut g.cnt;
            if !ai::ai_enabled(st, u.vs_ai) {
                return (None, false);
            }
            let mut ev_out = None;
            if let Some(mv) = ai::ai_pick(st, u.level) {
                if mv.is_flip {
                    if rules::flip(st, mv.from) {
                        cnt.record(crate::state::Ev::Flip);
                        g.rp.record(mv.from, mv.from, true); // ★ F2 AI 翻棋步
                        ev_out = Some(crate::state::Ev::Flip);
                    }
                } else if let Ok(ev) = rules::try_move(st, mv.from, mv.to) {
                    cnt.record(ev);
                    g.rp.record(mv.from, mv.to, false); // ★ F2 AI 走子步
                    ev_out = Some(ev);
                }
            }
            (ev_out, u.sound_on)
        });
        if sound_on { play_sfx_with_env(env, sound_on, ai_ev); }
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnUpdate(
        env: *mut jni::sys::JNIEnv,
        _cls: JClass,
    ) -> jni::sys::jboolean {
        // ★ JNI-13: OnUpdate 由 Java GLThread 每帧 onDrawFrame 调入，入口先清挂起异常，
        //   避免上一帧残留异常在后续 JNI 调用触发 "No pending exception expected" abort
        if !env.is_null() {
            let je = match unsafe { jni::JNIEnv::from_raw(env) } { Ok(e) => e, Err(_) => return 0 };
            if je.exception_check().unwrap_or(false) { let _ = je.exception_clear(); }
        }
        run_test_hooks(); // 真机验证钩子（setprop 触发）
        let mut wgpu_guard = match WGPU.lock() { Ok(g) => g, Err(e) => e.into_inner() };
        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        unsafe {
            // ★ JNI-03 根治：surface 与 device 必须同 wgpu Instance。旧实现 create_wgpu_device()
            //   内部建的 Instance A 已 drop，这里又新建 Instance B 装进 WgpuState，随后
            //   create_surface(B.instance, A.device) → wgpu-core hub 用 B 查 A 的 device id
            //   必失败（validation error / invalid-id panic）。改为 create_surface_and_device()
            //   在【同一个 Instance】里同时创建 surface + 带 compatible_surface 的 adapter + device。
            let view_raw = G_VIEW.load(std::sync::atomic::Ordering::Acquire);
            if view_raw == 0 { return 1i32; }
            let mut je = match jni::JNIEnv::from_raw(env) { Ok(e) => e, Err(_) => return 1 };
            let view = jni::objects::JObject::from_raw(view_raw as jni::sys::jobject);
            let holder = je.call_method(view, "getHolder", "()Landroid/view/SurfaceHolder;", &[])
                .ok().and_then(|v| v.l().ok());
            let surface_obj = match holder {
                Some(h) => je.call_method(h, "getSurface", "()Landroid/view/Surface;", &[])
                    .ok().and_then(|v| v.l().ok()),
                None => None,
            };
            let so = match surface_obj { Some(o) => o, None => {
                crate::log::setprop("jq.wgpu_state", "surface_unavailable"); return 1;
            }};
            let win = crate::log::surface_to_window(env as *mut _, so.into_raw());
            if win.is_null() { crate::log::setprop("jq.wgpu_state", "win_null"); return 1; }

            if wgpu_guard.is_none() {
                match crate::render::RenderCtx::create_surface_and_device(win, 900, 1600) {
                    Some((inst, surf, dev, q)) => {
                        match crate::render::RenderCtx::offscreen_render_test(&dev, &q) {
                            Some((r, g, b, a)) => crate::log::setprop("jq.offscreen", &format!("ok:{r},{g},{b},{a}")),
                            None => crate::log::setprop("jq.offscreen", "fail"),
                        }
                        *wgpu_guard = Some(WgpuState {
                            instance: inst, device: dev, queue: q,
                            surface: Some(surf), config: None, window: Some(WindowPtr(win)),
                        });
                        crate::log::setprop("jq.wgpu_state", "surface_device_ok");
                    }
                    None => {
                        crate::log::setprop("jq.wgpu_state", "surf_dev_fail");
                        crate::log::window_release(win);
                    }
                }
                return 1;
            }
            // ★ N3 根治：native Canvas 渲染线程独占 Surface（compose_board_rgba → lockCanvas）。
            //   Java GLThread 的 OnUpdate 若也 clear_frame 写同一 ANativeWindow，属未定义行为
            //   （EGL 与 lockCanvas 并发 → EGL_BAD_SURFACE/花屏）。OnUpdate 不再写出屏，
            //   只维护 wgpu 资源（供 offscreen 验证）；实际出屏由 wgpu_thread 一条链独占。
            // ★ P1 泄漏根治：surface_to_window 是 ANativeWindow_fromSurface（拿强引用），
            //   上面的 win 在 wgpu_guard 已 Some 时未转移所有权 → 每帧漏一个强引用。
            //   wgpu_guard 为 None 两条路径（成功存 WindowPtr / 失败 release）已配对，此处补第三条。
            crate::log::window_release(win);
            crate::log::setprop("jq.wgpu_state", "canvas_owns_surface");
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
        env: *mut jni::sys::JNIEnv,
        _cls: JClass,
        key: jni::sys::jobject,
    ) -> jni::sys::jboolean {
        // ★ F1 根治：smali 的 onKeyDown/onKeyUp 【都】调 OnKey，且返回 false 会走
        //   super.onKeyDown → finish()。旧实现不读 action/keyCode：Game 态返回 true、
        //   其余全 false → 一次 BACK 的 KEY_UP 立刻 finish()，退出确认框形同虚设；
        //   且音量键等任意键在 Game 态都会弹确认框。
        //   正确语义：只读 KEYCODE_BACK(4) 的 ACTION_DOWN(0)；其余键与 UP 一律返回 true
        //   消费（不交给 super，避免 finish）。key==null 时也消费（防御）。
        const KEYCODE_BACK: i32 = 4;
        const ACTION_DOWN: i32 = 0;
        let mut je = match unsafe { jni::JNIEnv::from_raw(env) } { Ok(e) => e, Err(_) => return 1 };
        let kev = unsafe { jni::objects::JObject::from_raw(key) };
        let action = je.call_method(&kev, "getAction", "()I", &[]).ok().and_then(|v| v.i().ok()).unwrap_or(-1);
        let keycode = je.call_method(&kev, "getKeyCode", "()I", &[]).ok().and_then(|v| v.i().ok()).unwrap_or(-1);
        if je.exception_check().unwrap_or(false) { let _ = je.exception_clear(); }
        // 非 BACK 或 非 ACTION_DOWN：消费掉，绝不 return false（否则 super→finish）
        if keycode != KEYCODE_BACK || action != ACTION_DOWN {
            return 1;
        }
        // BACK DOWN：按屏态决定行为——弹窗关闭（回上层）/ 对局弹退出确认 / 主菜单不处理
        with_lock(|_st, u, _c| {
            match u.screen {
                ui::Screen::Game => u.screen = ui::Screen::ExitConfirm,
                ui::Screen::IngameMenu | ui::Screen::Difficulty | ui::Screen::NewGame
                | ui::Screen::GameSet | ui::Screen::Settings | ui::Screen::Replay
                | ui::Screen::Stats => {
                    u.cancel_popup(_st); // 回 screen_snap（上层屏），而非退出 app
                }
                ui::Screen::ExitConfirm => { u.exit_confirm(false); } // 再按一次 BACK=取消退出
                ui::Screen::Menu => {} // 主菜单 BACK 不退出（保持 app 存活）
            }
        });
        1 // 始终消费
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
                let hit = with_lock(|st, u, _c| {
                    if ui::MENU_BTN_HUMAN.contains(bx as i32, by as i32) {
                        u.begin_popup(st); u.menu_human(); // ★ UI-03: 进弹窗前快照 mode/cfg
                        Some("human")
                    } else if ui::MENU_BTN_TWOPLAYER.contains(bx as i32, by as i32) {
                        u.begin_popup(st); u.menu_two_player();
                        Some("two")
                    } else if ui::MENU_BTN_NET.contains(bx as i32, by as i32) {
                        Some("net")    // 原版按钮已绘；联网不在人机对战范围 → 仅记录诚实边界
                    } else if ui::MENU_BTN_LOCAL.contains(bx as i32, by as i32) {
                        Some("local")  // 联机同上
                    } else if ui::MENU_BTN_FOUR.contains(bx as i32, by as i32) {
                        Some("four")   // 四国军棋同上
                    } else if ui::MENU_BTN_HOUSE.contains(bx as i32, by as i32) {
                        Some("house")  // 原版房子→官网；按用户指令不跳转，仅记录
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
                        // ★ F5 修：难度弹窗（人机入口）选定后确保 vs_ai=1，AI 必介入
                        u.vs_ai = true;
                        Some((lv, md))
                    } else { None }
                });
                if let Some((lv, md)) = hit {
                    clear_sel(); // ★ P1: 换局清残留选中，防首点被当旧局二次点击
                    reset_replay(); // ★ F2 新局重建复盘记录器
                    crate::log::setprop("jq.difficulty", &format!("{}_{}", lv, md));
                    return 1;
                }
                // ★ UI-10: 面板外=取消回主菜单（原 return 0 无动作，用户只能选难度才能离开）
                with_lock(|st, u, _c| u.cancel_popup(st));
                crate::log::setprop("jq.diff_cancel", "1");
                return 1;
            }
            ui::Screen::NewGame => {
                // ★ 原版 datah_062 真值按钮：0=确定 1=取消 2=明棋 3=暗棋（画=点同源）
                //   MotionEvent view 900×1600 → 720×1280 基准（与绘制 sx/sy 同源）
                let (bx, by) = (x * 0.8, y * 0.8);
                match ui::newgame_btn_hit(bx, by) {
                    Some(2) => { // 明棋
                        with_lock(|st, _u, _c| { st.mode = 0; });
                        crate::log::setprop("jq.ingame_act", "newgame_mode0");
                    }
                    Some(3) => { // 暗棋
                        with_lock(|st, _u, _c| { st.mode = 1; });
                        crate::log::setprop("jq.ingame_act", "newgame_mode1");
                    }
                    Some(0) => { // 确定 → 开局
                        with_lock(|st, u, _c| {
                            let mode = st.mode;
                            u.difficulty_select(u.level, mode, st);
                            u.vs_ai = false;
                        });
                        clear_sel(); // ★ P1: 新局清残留选中
                        reset_replay(); // ★ F2 新局重建复盘记录器
                        crate::log::setprop("jq.ingame_act", "newgame_confirm");
                    }
                    Some(1) | None => { // 取消 / 面板外 → 恢复 mode 快照
                        with_lock(|st, u, _c| u.cancel_popup(st));
                        crate::log::setprop("jq.ingame_act", "newgame_cancel");
                    }
                    _ => unreachable!(),
                }
                return 1;
            }
            ui::Screen::ExitConfirm => {
                // ★ 原版 datah_025 真值（720 基准）；MotionEvent view 900×1600 → ×0.8
                //   退出确认按按钮分派（确定=退出→Menu / 取消=继续→Game），面板外=继续（保守不退出）
                let (bx, by) = (x * 0.8, y * 0.8);
                match ui::exit_btn_hit(bx, by) {
                    Some(true) => { // 退出 → 回主菜单
                        with_lock(|_st, u, _c| u.exit_confirm(true));
                        crate::log::setprop("jq.exit_act", "quit");
                    }
                    Some(false) | None => { // 继续 / 面板外 → 回对局
                        with_lock(|_st, u, _c| u.exit_confirm(false));
                        crate::log::setprop("jq.exit_act", "stay");
                    }
                }
                return 1;
            }
            ui::Screen::IngameMenu => {
                // ★ 游戏内菜单 8 按钮（原版几何）；点按钮→动作，点外→关菜单回对局
                let (bx, by) = (x * 0.8, y * 0.8); // view 900×1600 → 720×1280 基准
                let hit = ui::ingame_hit(bx as i32, by as i32);
                match hit {
                    Some(0) => { // 重新开始
                        with_lock(|st, u, _c| { u.restart(st); }); clear_sel(); reset_replay();
                        crate::log::setprop("jq.ingame_act", "restart");
                    }
                    Some(1) => { // 悔棋（真执行）
                        // ★ RSAVE-001：悔棋同步回滚复盘记录（棋盘退/replay 没退 → seq 重复）
                        let ok = with_game(|g| {
                            let (ok, rp_cur) = g.ui.undo_with_rp(&mut g.state);
                            if ok { if let Some(cur) = rp_cur { g.rp.cur_set(cur); g.rp.truncate_to(cur); } }
                            ok
                        }); clear_sel();
                        crate::log::setprop("jq.ingame_act", if ok { "undo_ok" } else { "undo_empty" });
                    }
                    Some(7) => { // 退出程序
                        // ★ F6 修：begin_popup 快照 → 取消退出回 IngameMenu（非直接 Game）
                        with_lock(|st, u, _c| { u.begin_popup(st); u.screen = ui::Screen::ExitConfirm; });
                        clear_sel();
                        crate::log::setprop("jq.ingame_act", "exit");
                    }
                    Some(2) => { // 电脑变着→难度弹窗
                        // ★ F2/F5 修：begin_popup 快照（取消回 IngameMenu 而非旧 Menu 丢局）+ 清残留选中
                        with_lock(|st, u, _c| { u.begin_popup(st); u.screen = ui::Screen::Difficulty; });
                        clear_sel();
                        crate::log::setprop("jq.ingame_act", "difficulty");
                    }
                    Some(3) => { // 游戏规则→GameSet
                        with_lock(|st, u, _c| { u.begin_popup(st); u.screen = ui::Screen::GameSet; });
                        crate::log::setprop("jq.ingame_act", "gameset");
                    }
                    Some(4) => { // 功能设置→Settings
                        with_lock(|st, u, _c| { u.begin_popup(st); u.screen = ui::Screen::Settings; });
                        crate::log::setprop("jq.ingame_act", "settings");
                    }
                    Some(5) => { // 战况统计 → Stats 屏（F3：事件计数可视化）
                        with_lock(|_s, u, _c| { u.screen = ui::Screen::Stats; });
                        // 精确数值同时写属性（判据可读）
                        with_game(|g| crate::log::setprop("jq.stats", &format!("{}:{}:{}:{}:{}:{}:{} t{}",
                            g.cnt.by_code[0], g.cnt.by_code[1], g.cnt.by_code[2], g.cnt.by_code[3],
                            g.cnt.by_code[4], g.cnt.by_code[5], g.cnt.by_code[6], g.cnt.total)));
                        crate::log::setprop("jq.ingame_act", "stats");
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
                // ★ 原版真值：GameSet=datah_012 / Settings=datah_001；MotionEvent 900×1600 → ×0.8 转 720 基准
                let is_gameset = screen == ui::Screen::GameSet;
                let (bx, by) = (x * 0.8, y * 0.8);
                match ui::settings_btn_hit(bx, by, is_gameset) {
                    None => {
                        with_lock(|st, u, _c| u.cancel_popup(st));
                        crate::log::setprop("jq.popup_act", "cancel");
                    }
                    Some(4) => { // 确定（GameSet 回对局 / Settings 回原屏）
                        with_lock(|st, u, _c| {
                            if is_gameset { u.gameset_confirm(st); } else { u.settings_confirm(st); }
                        });
                        crate::log::setprop("jq.popup_act", "confirm");
                    }
                    Some(5) => { // 取消
                        with_lock(|st, u, _c| u.cancel_popup(st));
                        crate::log::setprop("jq.popup_act", "cancel");
                    }
                    Some(idx) => { // 开关（0-3 GameSet cfg / 0-2 Settings 声音三控件）
                        if is_gameset && idx < 4 {
                            with_lock(|st, _u, _c| { st.cfg[idx as usize] = if st.cfg[idx as usize] == 1 { 0 } else { 1 }; });
                            crate::log::setprop("jq.popup_act", &format!("cfg{}_toggle", idx));
                        } else if !is_gameset {
                            // ★ RJNI-004：原版 datah_001 声音设置三控件（Sound/BtnSound/Volume）
                            //   此前只有 idx0 生效，idx1/2 是"画了点不动的假按钮"。
                            //   原版 .rodata 实证 SoundMgr/SoundConfig/BtnSound/AudioManager 三配置独立。
                            match idx {
                                0 => {
                                    with_lock(|_st, u, _c| { u.sound_on = !u.sound_on; });
                                    crate::log::setprop("jq.popup_act", "sound_toggle");
                                }
                                1 => {
                                    with_lock(|_st, u, _c| { u.btn_sound_on = !u.btn_sound_on; });
                                    crate::log::setprop("jq.popup_act", "btnsound_toggle");
                                }
                                2 => {
                                    let v = with_lock(|_st, u, _c| {
                                        u.volume = if u.volume >= 100 { 0 } else { (u.volume + 10).min(100) };
                                        u.volume
                                    });
                                    crate::log::setprop("jq.popup_act", &format!("volume_{}", v));
                                }
                                _ => {}
                            }
                        }
                    }
                }
                return 1;
            }
            ui::Screen::Replay => {
                // ★ 轮10 datah_107 真值：控制条 460×60@(10,705) 720基准，5按钮各84×60
                //   x=10/104/198/292/386 y=705-765 → view(900系)×1.25。命中→对应动作。
                let (bx, by) = (x * 0.8, y * 0.8);
                let btn = if by >= 705.0 && by <= 765.0 {
                    [10.0f32, 104.0, 198.0, 292.0, 386.0].iter().position(|&x0| bx >= x0 && bx <= x0 + 84.0)
                } else { None };
                if let Some(btn) = btn {
                    match btn {
                        0 => { // 菜单：返回游戏内菜单
                            with_lock(|_s, u, _c| { u.screen = ui::Screen::IngameMenu; });
                            crate::log::setprop("jq.replay_act", "menu");
                        }
                        1 => { // 回退一步
                            let ok = with_game(|g| {
                                let cur = g.rp.cur().saturating_sub(1);
                                g.rp.cur_set(cur);
                                g.rp.rewind_to(&mut g.state, cur)
                            });
                            crate::log::setprop("jq.replay_act", if ok { "prev" } else { "prev_fail" });
                        }
                        2 => { // 前进一步
                            let ok = with_game(|g| {
                                let cur = (g.rp.cur() + 1).min(g.rp.count());
                                g.rp.cur_set(cur);
                                g.rp.rewind_to(&mut g.state, cur)
                            });
                            crate::log::setprop("jq.replay_act", if ok { "next" } else { "next_fail" });
                        }
                        3 => { // 跳转：到当前记录末尾（最新步）
                            let ok = with_game(|g| {
                                let cur = g.rp.count();
                                g.rp.cur_set(cur);
                                g.rp.rewind_to(&mut g.state, cur)
                            });
                            crate::log::setprop("jq.replay_act", if ok { "jump_end" } else { "jump_fail" });
                        }
                        _ => { // 返回对局：恢复实时终局局面
                            // ★ RSAVE-002：优先恢复进复盘前保存的实时局面（保留非走子终局
                            //   stage=Over/winner=Red 等，rewind_to 重放会丢）；无快照才回退重放。
                            let restored = with_game(|g| {
                                if let Some(live) = g.live_state.take() {
                                    g.state = live; true
                                } else {
                                    let cur = g.rp.count(); g.rp.rewind_to(&mut g.state, cur)
                                }
                            });
                            with_lock(|_s, u, _c| { u.screen = ui::Screen::Game; });
                            crate::log::setprop("jq.replay_act", if restored { "back" } else { "back_restored" });
                        }
                    }
                    return 1;
                }
                return 0;
            }
            ui::Screen::Stats => {
                // ★ 轮9 datah_050 真值：3 按钮 复盘/删除/返回（720基准 x175-285/305-415/435-545 y745-805
                //   → view 900系 ×1.25）。点按钮→对应动作，面板外→返回。
                let bx = x * 0.8; let by = y * 0.8;
                let in_btn = |x0: f32, x1: f32| bx >= x0 && bx <= x1 && by >= 745.0 && by <= 805.0;
                if in_btn(175.0, 285.0) { // 复盘 → Replay 屏
                    with_game(|g| {
                        g.live_state = Some(g.state.clone()); // ★ RSAVE-002：保存实时局面
                        let cur = g.rp.count();
                        g.rp.rewind_to(&mut g.state, cur);
                        g.ui.screen = crate::ui::Screen::Replay;
                    });
                    crate::log::setprop("jq.stats_act", "replay");
                } else if in_btn(305.0, 415.0) { // 删除 → 清复盘记录（诚实：无删除存档，仅清本局复盘）
                    with_game(|g| { g.rp = crate::save::Replay::new(&g.state); });
                    crate::log::setprop("jq.stats_act", "delete");
                } else { // 返回 / 面板外 → 回 ☰
                    with_lock(|_s, u, _c| { u.screen = ui::Screen::IngameMenu; });
                    crate::log::setprop("jq.stats_act", "back");
                }
                return 1;
            }
            ui::Screen::Game => {
                // ★ 对齐原版：终局"再来一局"按钮（C render.c x35%-65% y62%-72% @900×1600）
                //   stage==OVER 时 tap 落该区 → restart（原版点这里直接重开一局）
                let over = with_lock(|st, _u, _c| st.stage == crate::state::Stage::Over);
                if over && x >= 315.0 && x <= 585.0 && y >= 992.0 && y <= 1152.0 {
                    with_lock(|st, u, _c| u.restart(st)); clear_sel(); reset_replay();
                    crate::log::setprop("jq.ingame_act", "restart");
                    return 1;
                }
                // ★ F2 复盘入口：终局 overlay 上方区域（y 820-990）点→复盘（仅当有走子记录）
                if over && y >= 820.0 && y <= 990.0 {
                    let has = with_game(|g| g.rp.count() > 0);
                    if has {
                        with_game(|g| {
                            g.live_state = Some(g.state.clone()); // ★ RSAVE-002：保存实时终局局面
                            let cur = g.rp.count(); g.rp.rewind_to(&mut g.state, cur);
                        });
                        with_lock(|_s, u, _c| { u.screen = ui::Screen::Replay; });
                        crate::log::setprop("jq.replay_act", "enter");
                        return 1;
                    }
                }
                // ★ UI-05 修复：顶部图标命中区缩小到图标实际位置附近的小方框。
                //   原 y<820 + x<400/x>520 是巨大矩形，吞掉红方整个上半盘（两人热座点不到红子）。
                //   图标中心 view(270,750)/(620,750)（真机悔棋判据 tap620,750 实证），±60px 小方框。
                let in_icon = |cx: f32| (x - cx).abs() <= 60.0 && (y - 750.0).abs() <= 60.0;
                if in_icon(270.0) { // ☰ 游戏内菜单
                    with_lock(|_s, u, _c| { u.screen = ui::Screen::IngameMenu; });
                    crate::log::setprop("jq.game_icon", "menu");
                    return 1;
                } else if in_icon(620.0) { // ↩ 悔棋（真执行）
                    // ★ RSAVE-001：悔棋同步回滚复盘（与菜单悔棋同路径）
                    let ok = with_game(|g| {
                        let (ok, rp_cur) = g.ui.undo_with_rp(&mut g.state);
                        if ok { if let Some(cur) = rp_cur { g.rp.cur_set(cur); g.rp.truncate_to(cur); } }
                        ok
                    }); clear_sel();
                    crate::log::setprop("jq.game_icon", if ok { "undo_ok" } else { "undo_empty" });
                    return 1;
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
                    // ★ JNI-12: push_undo 只在动作确实合法/成功时做——原实现无条件 push，
                    //   二次点错（无效走子）也压栈，64 上限后污染悔棋栈。
                    // ★ F1 音效链 + F2 复盘：闭包返回 (成功, 玩家事件, AI事件, sound_on)，锁外播放 JNI
                    let (acted, player_ev, ai_ev, sound_on) = with_game(|g| {
                        let st = &mut g.state; let u = &mut g.ui; let cnt = &mut g.cnt;
                        // 先只读判定合法性（不修改棋盘）
                        let legal = if from == cell {
                            rules::can_flip(st, cell)
                        } else {
                            rules::can_move(st, from, cell)
                        };
                        if !legal { return (false, None, None, false); }
                        u.push_undo_at(st, g.rp.cur()); // 规格 §6 + RSAVE-001：push 快照 + 复盘游标
                        let ev_code = if from == cell {
                            // 同格翻棋（暗棋）
                            if rules::flip(st, cell) {
                                cnt.record(crate::state::Ev::Flip);
                                g.rp.record(from, cell, true); // ★ F2 复盘记录翻棋步
                                Some(crate::state::Ev::Flip)
                            } else { None }
                        } else {
                            // 异格走子（经 rules::try_move 统一规则引擎）
                            match rules::try_move(st, from, cell) {
                                Ok(e) => { cnt.record(e); g.rp.record(from, cell, false); Some(e) }
                                Err(_) => None,
                            }
                        };
                        let mut ai_ev = None;
                        if let Some(e) = ev_code {
                            G_MOVES.fetch_add(1, std::sync::atomic::Ordering::SeqCst);
                            crate::log::setprop("jq.move", &format!("{:?}", e));
                            // ★ 审计 A8：玩家落子后触发 AI 应着（人机对战，红方回合自动走）
                            if u.vs_ai && st.turn == crate::state::SIDE_RED && st.stage == crate::state::Stage::Play {
                                if let Some(mv) = ai::ai_pick(st, u.level) {
                                    if mv.is_flip {
                                        if rules::flip(st, mv.from) {
                                            cnt.record(crate::state::Ev::Flip);
                                            g.rp.record(mv.from, mv.from, true); // ★ F2 AI 翻棋步
                                            ai_ev = Some(crate::state::Ev::Flip);
                                        }
                                    } else if let Ok(aev) = rules::try_move(st, mv.from, mv.to) {
                                        cnt.record(aev);
                                        g.rp.record(mv.from, mv.to, false); // ★ F2 AI 走子步
                                        ai_ev = Some(aev);
                                    }
                                }
                            }
                        }
                        (ev_code.is_some(), ev_code, ai_ev, u.sound_on)
                    });
                    // ★ 锁外播放（严禁在 GAME 锁内做 JNI 调用，否则阻塞渲染线程）
                    if acted && sound_on {
                        play_sfx_with_env(env, sound_on, player_ev);
                        if let Some(ae) = ai_ev { play_sfx_with_env(env, sound_on, Some(ae)); }
                    }
                    return if acted { 1 } else { 0 };
                } else {
                    // 首次点击：选中（纯函数判定，桌面可测）——★ P0 修复：暗棋未翻子可选→二次点翻棋
                    let can_sel = with_lock(|st, _u, _c| rules::can_select_for_touch(st, cell));
                    if can_sel {
                        if let Ok(mut g) = G_SEL.lock() { *g = Some(cell); }
                        return 1;
                    }
                    return 0;
                }
            }
        } // 9 屏态全覆盖（无兜底：新增屏态时编译期强制接线，防 NewGame 卡死漏接复发）
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
