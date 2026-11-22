//! log — Rust → Android logcat 桥（__android_log_print FFI）
//! 出屏攻坚真机诊断用（wgpu device/surface 状态），对齐 C LOGI 标签 junqi_rs
#[cfg(target_os = "android")]
mod imp {
    use std::ffi::c_char;
    unsafe extern "C" {
        fn __android_log_print(prio: i32, tag: *const c_char, fmt: *const c_char, ...) -> i32;
    }
    const ANDROID_LOG_INFO: i32 = 4;
    const ANDROID_LOG_ERROR: i32 = 6;
    unsafe extern "C" {
        fn __system_property_set(name: *const c_char, value: *const c_char) -> i32;
        fn __system_property_get(name: *const c_char, value: *mut c_char) -> i32;
        fn ANativeWindow_fromSurface(env: *mut std::ffi::c_void, surface: jni::sys::jobject) -> *mut std::ffi::c_void;
        fn ANativeWindow_release(window: *mut std::ffi::c_void);
    }
    /// 写系统属性（adb getprop 可读，houdini 转译下比 logcat 可靠）
    pub fn setprop(name: &str, value: &str) {
        // ★ JNI-10: 内嵌 NUL 时 CString::new 返 Err，旧 unwrap 会 panic；
        //   setprop 在渲染线程的 run_test_hooks 中调用，panic 会杀掉整个渲染线程 → 永久黑屏。
        let n = std::ffi::CString::new(name).unwrap_or_else(|_| std::ffi::CString::new("invalid").unwrap());
        let v = std::ffi::CString::new(value).unwrap_or_else(|_| std::ffi::CString::new("(invalid)").unwrap());
        unsafe { __system_property_set(n.as_ptr(), v.as_ptr()); }
        // 文件探针（最可靠，adb cat 可读）：写 /data/local/tmp/jq_native_probe
        use std::io::Write;
        if let Ok(mut f) = std::fs::File::create("/data/user/0/com.qwsaeda.junqirs/files/jq_native_probe") {
            let _ = write!(f, "{}={}", name, value);
        }
    }

    /// 写结果文件（adb cat 读，比 setprop 可靠）
    pub fn write_result(name: &str, value: &str) {
        let path = format!("/data/user/0/com.qwsaeda.junqirs/files/jq_res_{}", name);
        let _ = std::fs::write(&path, value.as_bytes());
    }

    /// 文件钩子（adb echo 1 > /data/local/tmp/jq_hook_<name> 触发，native 读后删除）
    /// houdini 下 __system_property_get 读不到 adb 设的属性，文件更可靠
    pub fn hook_triggered(name: &str) -> bool {
        let path = format!("/data/local/tmp/jq_hook_{}", name);
        // 用 open 读判断（不依赖 Path::exists，houdini 下更可靠）
        match std::fs::read(&path) {
            Ok(_) => { let _ = std::fs::remove_file(&path); true }
            Err(_) => false,
        }
    }

    /// 读系统属性（真机钩子触发，对齐 C setprop jq.movetest 范式）
    pub fn getprop(name: &str) -> String {
        let n = std::ffi::CString::new(name).unwrap_or_else(|_| std::ffi::CString::new("invalid").unwrap());
        let mut buf = [0u8; 128];
        let len = unsafe { __system_property_get(n.as_ptr(), buf.as_mut_ptr() as *mut c_char) };
        if len <= 0 { return String::new(); }
        let bytes: Vec<u8> = buf[..len as usize].to_vec();
        String::from_utf8_lossy(&bytes).into_owned()
    }

    fn log_buf(prio: i32, msg: &str) {
        let tag = b"junqi_rs\0";
        // 用 %s 传字符串（避免 msg 内 % 被当格式符）
        let fmt = b"%s\0";
        let cmsg = std::ffi::CString::new(msg).unwrap_or_else(|_| std::ffi::CString::new("(invalid)").unwrap());
        unsafe {
            __android_log_print(
                prio,
                tag.as_ptr() as *const c_char,
                fmt.as_ptr() as *const c_char,
                cmsg.as_ptr(),
            );
        }
    }

    /// 从 Java Surface 对象拿 ANativeWindow 指针（需配对 ANativeWindow_release）
    pub unsafe fn surface_to_window(env: *mut std::ffi::c_void, surface: jni::sys::jobject) -> *mut std::ffi::c_void {
        ANativeWindow_fromSurface(env, surface)
    }
    pub unsafe fn window_release(window: *mut std::ffi::c_void) {
        ANativeWindow_release(window)
    }
    pub fn info(msg: &str) { log_buf(ANDROID_LOG_INFO, msg); }
    pub fn error(msg: &str) { log_buf(ANDROID_LOG_ERROR, msg); }
}
#[cfg(target_os = "android")]
pub use imp::{error, getprop, hook_triggered, info, setprop, surface_to_window, window_release, write_result};
#[cfg(not(target_os = "android"))]
pub fn info(_msg: &str) {}
#[cfg(not(target_os = "android"))]
pub fn error(_msg: &str) {}
#[cfg(not(target_os = "android"))]
pub fn setprop(_name: &str, _value: &str) {}
#[cfg(not(target_os = "android"))]
pub fn getprop(_name: &str) -> String { String::new() }
#[cfg(not(target_os = "android"))]
pub fn hook_triggered(_name: &str) -> bool { false }
#[cfg(not(target_os = "android"))]
pub fn write_result(_n: &str, _v: &str) {}
