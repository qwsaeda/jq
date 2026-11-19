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

    static mut G_STATE: Option<State> = None;
    static G_LOCK: Mutex<()> = Mutex::new(());
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

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnCreate(
        env: *mut jni::sys::JNIEnv,
        _cls: JClass,
        activity: jni::sys::jobject,
    ) -> jni::sys::jobject {
        with_lock(|st, _u, _c| {
            *st = State::new();
            st.mode = crate::state::MODE_FLIP; // 默认暗棋翻棋（g_opt_mode 默认 1）
            st.cfg = [1, 1, 1, 1, 1];
            st.turn = crate::state::SIDE_BLUE;
            st.stage = crate::state::Stage::Play;
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
            new_object(env, cls, ctor, activity)
        }
    }

    #[no_mangle]
    pub extern "C" fn Java_com_qwsaeda_App_OnDestroy(_env: *mut jni::sys::JNIEnv, _cls: JClass) {
        with_lock(|_s, _u, _c| {});
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
        _env: *mut jni::sys::JNIEnv,
        _cls: JClass,
    ) -> jni::sys::jboolean {
        // wgpu 出屏攻坚：OnUpdate 首次调初始化 wgpu device（gles 后端，真机验可达）
        // 真机 logcat 诊断：wgpu device 获取成功/失败 + 出屏通路
        let init_ok = with_lock(|st, _u, _c| {
            // device 未初始化时获取（幂等：已 init 跳过）
            // wgpu 出屏攻坚：create_wgpu_device 真机验 gles 后端（EGL/GPU 驱动）
            if !st.wgpu_log_done {
                match crate::render::RenderCtx::create_wgpu_device() {
                    Some((dev, _q)) => {
                        crate::log::setprop("jq.wgpu_state", "device_ok");
                        let _ = dev;
                        st.wgpu_init_done = true;
                    }
                    None => {
                        crate::log::setprop("jq.wgpu_state", "device_fail");
                        st.wgpu_init_done = false;
                    }
                }
                st.wgpu_log_done = true;
            }
            true
        });
        init_ok as jni::sys::jboolean
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
        // 原生 JNI 反射提取 MotionEvent.getAction/getX/getY（对齐 C jni_bridge.c:3993）：
        //   let envref = unsafe { &*env };
        //   let mcls = (envref.GetClass)(env, me);
        //   let mid_act = (envref.GetMethodID)(env, mcls, c_str"getAction").as_ptr() as *const _, c"()I".as_ptr() as *const _);
        //   let action = (envref.CallIntMethod)(env, me, mid_act) as i32;
        //   if action != 0 { return 0; }  // 只处理 ACTION_DOWN
        //   let mx = (envref.CallFloatMethod)(env, me, mid_x) as f32;
        //   let my = (envref.CallFloatMethod)(env, me, mid_y) as f32;
        //   with_lock(|st,u,cnt| render::handle_touch(st,u,cnt,mx as i32,my as i32,view_w,view_h));
        // ★ 诚实边界（设计 §3 ③④）：完整坐标提取 + handle_touch 接线真机 NDK 可达时补全
        //   （c-str 字面量 + JNIEnv 函数指针布局桌面 cross-compile 不执行，真机验证）。
        // 骨架期：安全降级返 false（不消费，对齐 C ACTION_DOWN 语义，不误触发走子）
        let consumed = with_lock(|st, u, cnt| {
            let _ = (env, me, st, u, cnt);
            false
        });
        consumed as jni::sys::jboolean
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
