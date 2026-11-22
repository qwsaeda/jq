//! wgpu_thread — 纯 Rust native 渲染线程：attach JVM 轮询 Surface → ANativeWindow → wgpu surface → 循环渲染
#[cfg(target_os = "android")]
mod imp {
    use std::sync::atomic::{AtomicBool, Ordering};
    static RUNNING: AtomicBool = AtomicBool::new(false);
    static STOP: AtomicBool = AtomicBool::new(false);
    /// 渲染线程 handle（Mutex 安全，替代 static mut；start 写 / stop take+join 串行）
    static JOIN: std::sync::Mutex<Option<std::thread::JoinHandle<()>>> = std::sync::Mutex::new(None);

    /// 停止渲染线程（OnDestroy 调用，防退出 SIGABRT）
    pub fn stop() {
        STOP.store(true, Ordering::SeqCst);
        // ★ JNI-04: 不无界 join（启动期 150×33ms≈5s + block_on 会让 UI 线程 ANR）。
        //   轮询 RUNNING 最多 2s；超时则放弃 join（线程自行退出，不阻塞 UI）。
        for _ in 0..200 {
            if !RUNNING.load(Ordering::SeqCst) { break; }
            std::thread::sleep(std::time::Duration::from_millis(10));
        }
        if let Ok(mut guard) = JOIN.lock() {
            if let Some(h) = guard.take() {
                if !RUNNING.load(Ordering::SeqCst) { let _ = h.join(); }
                // 超时未退出则不 join，drop handle 分离线程（避免 ANR）
            }
        }
    }

    /// 启动 wgpu 渲染线程（native 独占 window，无 Java GLThread 竞争）
    pub fn start(vm_addr: usize, view_addr: usize, w: u32, h: u32) {
        // ★ JNI-05: 先抢占 RUNNING，成功翻转后才清 STOP（旧顺序会让旧线程逃逸生命周期）
        if RUNNING.swap(true, Ordering::SeqCst) {
            return; // 已有线程在跑，不动 STOP
        }
        STOP.store(false, Ordering::SeqCst);
        let h = std::thread::spawn(move || {
            // ★ JNI-11: 删除全局 panic hook——它静默所有线程日志；catch_unwind 已阻止 panic 跨线程 abort
            let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                unsafe { run(vm_addr, view_addr, w, h) }
            }));
            if result.is_err() {
                crate::log::setprop("jq.wgpu_state3", "panicked");
            }
            RUNNING.store(false, Ordering::SeqCst);
        });
        if let Ok(mut guard) = JOIN.lock() { *guard = Some(h); }
    }

    unsafe fn run(vm_addr: usize, view_addr: usize, _w: u32, _h: u32) {
        let vm = match jni::JavaVM::from_raw(vm_addr as *mut jni::sys::JavaVM) {
            Ok(v) => v,
            Err(_) => { crate::log::setprop("jq.wgpu_state3", "vm_fail"); return; }
        };
        let mut env = match vm.attach_current_thread() {
            Ok(e) => e,
            Err(_) => { crate::log::setprop("jq.wgpu_state3", "attach_fail"); return; }
        };
        crate::log::setprop("jq.wgpu_state3", "attached");

        // ① 轮询等 Surface 有效（setContentView 后异步创建）
        let mut window: *mut std::ffi::c_void = std::ptr::null_mut();
        let view = jni::objects::JObject::from_raw(view_addr as jni::sys::jobject);
        for _ in 0..150 {
            if STOP.load(Ordering::SeqCst) { return; } // ★ JNI-04: 启动期也响应停止，防 stop() join 长时间阻塞→ANR
            let surf = env.call_method(&view, "getHolder", "()Landroid/view/SurfaceHolder;", &[])
                .ok().and_then(|v| v.l().ok())
                .and_then(|holder| env.call_method(holder, "getSurface", "()Landroid/view/Surface;", &[]).ok().and_then(|s| s.l().ok()));
            if let Some(so) = surf {
                let valid = env.call_method(&so, "isValid", "()Z", &[]).ok().and_then(|v| v.z().ok()).unwrap_or(false);
                if valid {
                    window = crate::log::surface_to_window(&mut env as *mut _ as *mut _, so.into_raw());
                    if !window.is_null() { break; }
                }
            }
            std::thread::sleep(std::time::Duration::from_millis(33));
        }
        if window.is_null() {
            crate::log::setprop("jq.wgpu_state3", "window_timeout");
            return;
        }
        crate::log::setprop("jq.wgpu_state3", "window_ok");

        // ② wgpu device（offscreen 渲染已实锤可用，不依赖 create_surface）
        let (_dev, _q) = match crate::render::RenderCtx::create_wgpu_device() {
            Some(v) => v,
            None => { crate::log::setprop("jq.wgpu_state3", "device_fail"); return; }
        };
        crate::log::setprop("jq.wgpu_state3", "device_ok");
        // ★ 不再缓存 Surface global ref：系统回收/重建后旧 ref 失效会让 lockCanvas abort。
        //   改为每帧从 view 重取当前 Surface（见下方渲染循环）。
        crate::log::window_release(window); // 释放轮询时的临时 ANativeWindow 引用

        // ③ 渲染循环：wgpu offscreen RGBA → Java Surface.lockCanvas 显示
        // ★ 崩溃根治：每帧从 view 重取当前 Surface（系统回收/重建后自动恢复），
        //   不再用一次性 surf_global（失效后 lockCanvas 抛 IllegalStateException abort）。
        let mut frame = 0u64;
        loop {
            if STOP.load(Ordering::SeqCst) { break; }
            // ★ JNI 异常安全：本线程 attach JVM 后长期运行，上一帧若某次 JNI 调用抛异常
            //   会挂到本帧 → 下次 JNI 调用 "No pending exception expected" abort。
            //   每帧开头清一次，保证帧间异常不累积（canvas.rs 内部也清 lockCanvas 异常）。
            if env.exception_check().unwrap_or(false) { let _ = env.exception_clear(); }
            crate::jni::run_test_hooks(); // 真机验证钩子（setRenderer 移除后 OnUpdate 不调，改由此线程驱动）
            let r = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                if let Some(rgba) = {
                    let (board, screen) = match crate::jni::board_snapshot() { Some(b) => b, None => return };
                    Some(crate::asset::compose_board_rgba(450, 800, Some(&board), screen))
                } {
                    // 每帧重取当前 Surface（local ref，用完自动释放）
                    let cur = env.call_method(&view, "getHolder", "()Landroid/view/SurfaceHolder;", &[])
                        .ok().and_then(|v| v.l().ok())
                        .and_then(|holder| env.call_method(holder, "getSurface", "()Landroid/view/Surface;", &[]).ok().and_then(|s| s.l().ok()));
                    if let Some(surface) = cur {
                        let valid = env.call_method(&surface, "isValid", "()Z", &[])
                            .ok().and_then(|v| v.z().ok()).unwrap_or(false);
                        if valid {
                            // ★ 状态属性反映"当前帧"结果：成功写 rendering、失败写 canvas_err。
                            //   旧实现只在 frame==3 写一次 rendering、失败才写 err →
                            //   一次偶发失败就把属性永久卡在 canvas_err（画面其实正常），判据假 FAIL。
                            if crate::canvas::draw_rgba_to_surface(&mut env, surface, &rgba, 450, 800).is_ok() {
                                crate::log::setprop("jq.wgpu_state3", "rendering");
                            } else {
                                crate::log::setprop("jq.wgpu_state3", "canvas_err");
                            }
                        } else {
                            crate::log::setprop("jq.wgpu_state3", "surface_invalid");
                        }
                    }
                }
            }));
            if let Err(payload) = &r {
                let msg = payload.downcast_ref::<&str>().copied()
                    .or_else(|| payload.downcast_ref::<String>().map(|s| s.as_str()))
                    .unwrap_or("?");
                crate::log::setprop("jq.err", &msg.chars().take(40).collect::<String>());
                crate::log::setprop("jq.wgpu_state3", "frame_panic"); break;
            }
            frame += 1;
            if frame == 3 { crate::log::setprop("jq.wgpu_state3", "rendering"); }
            std::thread::sleep(std::time::Duration::from_millis(400));
        }
    }
}
#[cfg(target_os = "android")]
pub use imp::{start, stop};
#[cfg(not(target_os = "android"))]
pub fn start(_vm: usize, _view: usize, _w: u32, _h: u32) {}
#[cfg(not(target_os = "android"))]
pub fn stop() {}
