//! canvas — JNI 桥：RGBA 像素 → Java Bitmap → Surface.lockCanvas/drawBitmap/unlockCanvasAndPost
//! 绕过 wgpu EGL surface（houdini 限制），用 wgpu offscreen readback + Java 2D Canvas 出屏
#[cfg(target_os = "android")]
mod imp {
    use jni::objects::{JObject, JValue};
    use jni::JNIEnv;

    /// 把 RGBA 字节数组画到 Surface（JNI 调 Java Bitmap + Canvas）
    /// rgba: [r,g,b,a,...]，w/h 尺寸
    pub fn draw_rgba_to_surface(
        env: &mut JNIEnv,
        surface: JObject<'_>,
        rgba: &[u8],
        w: i32,
        h: i32,
    ) -> Result<(), String> {
        // ★ 崩溃根治（Surface released 后 lockCanvas 抛 IllegalStateException）：
        //   JNI 异常若挂着不清理，下一次 JNI 调用会 "No pending exception expected" abort。
        //   入口先清残留异常；lockCanvas 失败时清异常再返回 Err（由调用方跳过该帧）。
        if env.exception_check().unwrap_or(false) {
            let _ = env.exception_clear();
        }
        // ① int[] 像素（Java int ARGB_8888 = 0xAARRGGBB）
        let n = (w * h) as usize;
        let mut pixels = vec![0i32; n];
        for i in 0..n {
            let r = rgba.get(i * 4).copied().unwrap_or(0) as i32;
            let g = rgba.get(i * 4 + 1).copied().unwrap_or(0) as i32;
            let b = rgba.get(i * 4 + 2).copied().unwrap_or(0) as i32;
            let a = rgba.get(i * 4 + 3).copied().unwrap_or(255) as i32;
            pixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
        let arr = env.new_int_array(n as jni::sys::jsize).map_err(|e| e.to_string())?;
        env.set_int_array_region(&arr, 0, &pixels).map_err(|e| e.to_string())?;

        // ② Bitmap.createBitmap(int[], w, h, Bitmap.Config.ARGB_8888)
        let bm_cls = env.find_class("android/graphics/Bitmap").map_err(|e| e.to_string())?;
        let config_cls = env.find_class("android/graphics/Bitmap$Config").map_err(|e| e.to_string())?;
        let argb = env.get_static_field(config_cls, "ARGB_8888", "Landroid/graphics/Bitmap$Config;")
            .map_err(|e| e.to_string())?.l().map_err(|e| e.to_string())?;
        let bitmap = env.call_static_method(bm_cls, "createBitmap",
                "([IIILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;",
                &[JValue::Object(&arr), JValue::Int(w), JValue::Int(h), JValue::Object(&argb)])
            .map_err(|e| e.to_string())?.l().map_err(|e| e.to_string())?;

        // ③ Surface.lockCanvas() → Canvas
        // Android 公开 API = lockCanvas(Rect dirty)；无参版 @hide（targetSdk 35 拒）
        // 传 null Rect = 锁整个 surface
        let canvas = match env.call_method(&surface, "lockCanvas", "(Landroid/graphics/Rect;)Landroid/graphics/Canvas;",
                &[JValue::Object(&JObject::null())]) {
            Ok(v) => v.l().map_err(|e| e.to_string())?,
            Err(e) => {
                // Surface 已释放/无效 → 清 JNI 异常，返回 Err 让调用方跳过该帧（不 abort）
                if env.exception_check().unwrap_or(false) { let _ = env.exception_clear(); }
                return Err(format!("lockCanvas failed: {}", e));
            }
        };

        // ④ Bitmap.createScaledBitmap(bitmap, 900, 1600, false) → 缩放小图到全屏（降 CPU 合成量）
        let scaled = match env.call_static_method("android/graphics/Bitmap", "createScaledBitmap",
                "(Landroid/graphics/Bitmap;IIZ)Landroid/graphics/Bitmap;",
                &[JValue::Object(&bitmap), JValue::Int(900), JValue::Int(1600), JValue::Bool(0)])
            .ok().and_then(|v| v.l().ok()) {
                Some(b) => b,
                None => return Err("createScaledBitmap failed".into()),
            };
        // ⑤ Canvas.drawBitmap(scaled, 0,0, null)
        env.call_method(&canvas, "drawBitmap", "(Landroid/graphics/Bitmap;FFLandroid/graphics/Paint;)V",
            &[JValue::Object(&scaled), JValue::Float(0.0), JValue::Float(0.0), JValue::Object(&JObject::null())])
            .map_err(|e| e.to_string())?;

        // ⑤ Surface.unlockCanvasAndPost(canvas)
        env.call_method(&surface, "unlockCanvasAndPost", "(Landroid/graphics/Canvas;)V",
            &[JValue::Object(&canvas)]).map_err(|e| e.to_string())?;
        Ok(())
    }
}
#[cfg(target_os = "android")]
pub use imp::draw_rgba_to_surface;
#[cfg(not(target_os = "android"))]
pub fn draw_rgba_to_surface<J>(
    _env: J,
    _surface: J,
    _rgba: &[u8],
    _w: i32,
    _h: i32,
) -> Result<(), String> {
    Err("desktop".into())
}
