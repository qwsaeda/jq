//! egl_render — native EGL 渲染线程（C jq_glview_start_render_thread 的 Rust 重写）
//! 直接链接 libEGL/libGLESv2，eglCreateWindowSurface(ANativeWindow) + makeCurrent + 画 + swap。
//! C 已实锤此路径在 houdini 下出屏 98%+（wgpu create_surface 失败但裸 EGL 成功）。
#[cfg(target_os = "android")]
mod imp {
    use std::ffi::c_void;
    use std::sync::atomic::{AtomicBool, Ordering};

    type EGLDisplay = *mut c_void;
    type EGLConfig = *mut c_void;
    type EGLSurface = *mut c_void;
    type EGLContext = *mut c_void;

    #[link(name = "EGL")]
    unsafe extern "C" {
        fn eglGetDisplay(d: *mut c_void) -> EGLDisplay;
        fn eglInitialize(d: EGLDisplay, major: *mut i32, minor: *mut i32) -> i32;
        fn eglChooseConfig(d: EGLDisplay, attrs: *const i32, configs: *mut EGLConfig, n: i32, out: *mut i32) -> i32;
        fn eglCreateWindowSurface(d: EGLDisplay, cfg: EGLConfig, win: *mut c_void, attrs: *const i32) -> EGLSurface;
        fn eglCreateContext(d: EGLDisplay, cfg: EGLConfig, share: EGLContext, attrs: *const i32) -> EGLContext;
        fn eglMakeCurrent(d: EGLDisplay, draw: EGLSurface, read: EGLSurface, ctx: EGLContext) -> i32;
        fn eglSwapBuffers(d: EGLDisplay, s: EGLSurface) -> i32;
        fn eglGetError() -> i32;
    }
    #[link(name = "GLESv2")]
    unsafe extern "C" {
        fn glViewport(x: i32, y: i32, w: i32, h: i32);
        fn glClearColor(r: f32, g: f32, b: f32, a: f32);
        fn glClear(mask: u32);
    }

    const EGL_NONE: i32 = 0x3038;
    const EGL_SURFACE_TYPE: i32 = 0x3033;
    const EGL_WINDOW_BIT: i32 = 0x0004;
    const EGL_RENDERABLE_TYPE: i32 = 0x3040;
    const EGL_OPENGL_ES2_BIT: i32 = 0x0004;
    const EGL_RED_SIZE: i32 = 0x3024;
    const EGL_GREEN_SIZE: i32 = 0x3023;
    const EGL_BLUE_SIZE: i32 = 0x3022;
    const EGL_ALPHA_SIZE: i32 = 0x3021;
    const EGL_DEPTH_SIZE: i32 = 0x3025;
    const EGL_PBUFFER_BIT: i32 = 0x0001;
    const EGL_CONTEXT_CLIENT_VERSION: i32 = 0x3098;
    const GL_COLOR_BUFFER_BIT: u32 = 0x4000;

    static RUNNING: AtomicBool = AtomicBool::new(false);

    /// 启动 native EGL 渲染线程（attach JVM 轮询 Surface→ANativeWindow，持 window surface 画+swap）
    /// vm_addr: JavaVM 指针（JNI_OnLoad 保存）, view_addr: GLView global ref
    pub fn start_render_thread_jni(vm_addr: usize, view_addr: usize, w: i32, h: i32) {
        if RUNNING.swap(true, Ordering::SeqCst) {
            return; // 已在跑
        }
        std::thread::spawn(move || {
            // ① attach JVM + 轮询拿 Surface（setContentView 后异步创建）
            let mut window: *mut c_void = std::ptr::null_mut();
            unsafe {
                let vm = jni::JavaVM::from_raw(vm_addr as *mut jni::sys::JavaVM).ok();
                crate::log::setprop("jq.egl", "thread_attached");
                if let Some(vm) = vm {
                    if let Ok(mut env) = vm.attach_current_thread() {
                        let view = jni::objects::JObject::from_raw(view_addr as jni::sys::jobject);
                        for _ in 0..120 { // 轮询 ~4s 等 Surface 创建
                            let surf = env.call_method(&view, "getHolder", "()Landroid/view/SurfaceHolder;", &[])
                                .ok().and_then(|v| v.l().ok())
                                .and_then(|h| env.call_method(h, "getSurface", "()Landroid/view/Surface;", &[]).ok().and_then(|s| s.l().ok()));
                            if let Some(so) = surf {
                                window = crate::log::surface_to_window(&mut env as *mut _ as *mut _, so.into_raw());
                                if !window.is_null() { break; }
                            }
                            std::thread::sleep(std::time::Duration::from_millis(33));
                        }
                    }
                }
                if window.is_null() {
                    crate::log::setprop("jq.egl", "window_timeout");
                    RUNNING.store(false, Ordering::SeqCst);
                    return;
                }
                crate::log::setprop("jq.egl", "window_acquired");
            }
            unsafe {
                // EGL 初始化（C 同款：EGL_DEFAULT_DISPLAY）
                let disp = eglGetDisplay(std::ptr::null_mut());
                let mut major = 0;
                let mut minor = 0;
                if eglInitialize(disp, &mut major, &mut minor) == 0 {
                    crate::log::setprop("jq.egl", "init_fail");
                    RUNNING.store(false, Ordering::SeqCst);
                    return;
                }
                crate::log::setprop("jq.egl", "init_ok");
                // config：WINDOW + ES2
                let attrs = [
                    EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
                    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
                    EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 16,
                    EGL_NONE,
                ];
                let mut cfg: EGLConfig = std::ptr::null_mut();
                let mut n = 0;
                if eglChooseConfig(disp, attrs.as_ptr(), &mut cfg, 1, &mut n) == 0 || n < 1 {
                    crate::log::setprop("jq.egl", "config_fail");
                    RUNNING.store(false, Ordering::SeqCst);
                    return;
                }
                crate::log::setprop("jq.egl", "config_ok");
                // window surface
                let surf = eglCreateWindowSurface(disp, cfg, window, std::ptr::null());
                if surf.is_null() {
                    crate::log::setprop("jq.egl", &format!("surface_fail:0x{:x}", eglGetError()));
                    RUNNING.store(false, Ordering::SeqCst);
                    return;
                }
                crate::log::setprop("jq.egl", "surface_ok");
                // context ES2
                let ctx_attrs = [EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE];
                let ctx = eglCreateContext(disp, cfg, std::ptr::null_mut(), ctx_attrs.as_ptr());
                if ctx.is_null() {
                    crate::log::setprop("jq.egl", "ctx_fail");
                    RUNNING.store(false, Ordering::SeqCst);
                    return;
                }
                crate::log::setprop("jq.egl", "ctx_ok");
                if eglMakeCurrent(disp, surf, surf, ctx) == 0 {
                    crate::log::setprop("jq.egl", &format!("makecurrent_fail:0x{:x}", eglGetError()));
                    RUNNING.store(false, Ordering::SeqCst);
                    return;
                }
                crate::log::setprop("jq.egl", "render_loop_start");
                // 渲染循环（亮红验证 → 后续换 wgpu offscreen 纹理）
                let mut frame = 0u64;
                while RUNNING.load(Ordering::SeqCst) {
                    glViewport(0, 0, w, h);
                    glClearColor(1.0, 0.0, 0.0, 1.0);
                    glClear(GL_COLOR_BUFFER_BIT);
                    eglSwapBuffers(disp, surf);
                    frame += 1;
                    if frame == 3 {
                        crate::log::setprop("jq.egl", "swapped_3frames");
                    }
                    std::thread::sleep(std::time::Duration::from_millis(16));
                }
            }
        });
    }
}
#[cfg(target_os = "android")]
pub use imp::start_render_thread_jni;
#[cfg(not(target_os = "android"))]
pub fn start_render_thread_jni(_vm: usize, _view: usize, _w: i32, _h: i32) {}
