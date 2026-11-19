//! wgpu_thread — 纯 Rust native 渲染线程：attach JVM 轮询 Surface → ANativeWindow → wgpu surface → 循环渲染
#[cfg(target_os = "android")]
mod imp {
    use std::sync::atomic::{AtomicBool, Ordering};
    static RUNNING: AtomicBool = AtomicBool::new(false);
    static STOP: AtomicBool = AtomicBool::new(false);

    /// 停止渲染线程（OnDestroy 调用，防退出 SIGABRT）
    pub fn stop() { STOP.store(true, Ordering::SeqCst); }

    /// 启动 wgpu 渲染线程（native 独占 window，无 Java GLThread 竞争）
    pub fn start(vm_addr: usize, view_addr: usize, w: u32, h: u32) {
        STOP.store(false, Ordering::SeqCst);
        if RUNNING.swap(true, Ordering::SeqCst) {
            return;
        }
        std::thread::spawn(move || {
            std::panic::set_hook(Box::new(|_| {})); // 线程内 panic 静默，不 abort 进程
            let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                unsafe { run(vm_addr, view_addr, w, h) }
            }));
            if result.is_err() {
                crate::log::setprop("jq.wgpu_thread", "panicked");
            }
            RUNNING.store(false, Ordering::SeqCst);
        });
    }

    unsafe fn run(vm_addr: usize, view_addr: usize, w: u32, h: u32) {
        let vm = match jni::JavaVM::from_raw(vm_addr as *mut jni::sys::JavaVM) {
            Ok(v) => v,
            Err(_) => { crate::log::setprop("jq.wgpu_thread", "vm_fail"); return; }
        };
        let mut env = match vm.attach_current_thread() {
            Ok(e) => e,
            Err(_) => { crate::log::setprop("jq.wgpu_thread", "attach_fail"); return; }
        };
        crate::log::setprop("jq.wgpu_thread", "attached");

        // ① 轮询等 Surface 有效（setContentView 后异步创建）
        let mut window: *mut std::ffi::c_void = std::ptr::null_mut();
        let view = jni::objects::JObject::from_raw(view_addr as jni::sys::jobject);
        for _ in 0..150 {
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
            crate::log::setprop("jq.wgpu_thread", "window_timeout");
            return;
        }
        crate::log::setprop("jq.wgpu_thread", "window_ok");

        // ② wgpu device（offscreen 渲染已实锤可用，不依赖 create_surface）
        let (dev, q) = match crate::render::RenderCtx::create_wgpu_device() {
            Some(v) => v,
            None => { crate::log::setprop("jq.wgpu_thread", "device_fail"); return; }
        };
        crate::log::setprop("jq.wgpu_thread", "device_ok");
        // 保存 Java Surface global ref（lockCanvas 显示用）；setRenderer 已移除，GLThread 不占 window
        let surf_global: jni::sys::jobject = {
            let so = env.call_method(&view, "getHolder", "()Landroid/view/SurfaceHolder;", &[])
                .ok().and_then(|v| v.l().ok())
                .and_then(|h| env.call_method(h, "getSurface", "()Landroid/view/Surface;", &[]).ok().and_then(|s| s.l().ok()));
            let g = match so {
                Some(s) => match env.new_global_ref(s) { Ok(g) => g, Err(_) => return },
                None => return,
            };
            let raw = jni::objects::JObject::from_raw(g.as_obj().as_raw()).into_raw(); std::mem::forget(g); raw
        };
        crate::log::window_release(window); // 释放轮询时的临时引用，用 surf_global

        // ③ 渲染循环：wgpu offscreen RGBA → Java Surface.lockCanvas 显示
        let mut frame = 0u64;
        loop {
            if STOP.load(Ordering::SeqCst) { break; }
            let r = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                if let Some(rgba) = Some(crate::render::cpu_board_rgba(900, 1600)) {
                    let surface = jni::objects::JObject::from_raw(surf_global);
                    if crate::canvas::draw_rgba_to_surface(&mut env, surface, &rgba, 900, 1600).is_err() {
                        crate::log::setprop("jq.wgpu_thread", "canvas_err");
                    }
                }
            }));
            if let Err(payload) = &r {
                let msg = payload.downcast_ref::<&str>().copied()
                    .or_else(|| payload.downcast_ref::<String>().map(|s| s.as_str()))
                    .unwrap_or("?");
                crate::log::setprop("jq.err", &msg.chars().take(40).collect::<String>());
                crate::log::setprop("jq.wgpu_thread", "frame_panic"); break;
            }
            frame += 1;
            if frame == 1 { crate::log::setprop("jq.wgpu_thread", "rendering"); }
            if frame >= 2 { break; } // 静态验证：只画 1 帧（排查每帧重建资源 validation）
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
