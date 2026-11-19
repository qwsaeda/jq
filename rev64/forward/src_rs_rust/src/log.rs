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
    }
    /// 写系统属性（adb getprop 可读，houdini 转译下比 logcat 可靠）
    pub fn setprop(name: &str, value: &str) {
        let n = std::ffi::CString::new(name).unwrap();
        let v = std::ffi::CString::new(value).unwrap();
        unsafe { __system_property_set(n.as_ptr(), v.as_ptr()); }
        // 文件探针（最可靠，adb cat 可读）：写 /data/local/tmp/jq_native_probe
        use std::io::Write;
        if let Ok(mut f) = std::fs::File::create("/data/user/0/com.qwsaeda.junqirs/files/jq_native_probe") {
            let _ = write!(f, "{}={}", name, value);
        }
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
    pub fn info(msg: &str) { log_buf(ANDROID_LOG_INFO, msg); }
    pub fn error(msg: &str) { log_buf(ANDROID_LOG_ERROR, msg); }
}
#[cfg(target_os = "android")]
pub use imp::{error, info, setprop};
#[cfg(not(target_os = "android"))]
pub fn info(_msg: &str) {}
#[cfg(not(target_os = "android"))]
pub fn error(_msg: &str) {}
#[cfg(not(target_os = "android"))]
pub fn setprop(_name: &str, _value: &str) {}
