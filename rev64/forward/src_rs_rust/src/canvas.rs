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
        //   用 chunks_exact 按像素分组 + u32/I50 移位，替代手写索引循环（等价且更 Rust）
        let n = (w * h) as usize;
        let mut pixels: Vec<i32> = rgba
            .chunks_exact(4)
            .take(n)
            .map(|p| {
                let (r, g, b, a) = (p[0], p[1], p[2], p[3]);
                (((a as u32) << 24) | ((r as u32) << 16) | ((g as u32) << 8) | b as u32) as i32
            })
            .collect();
        pixels.resize(n, 0); // rgba 短于 w*h 时补 0（等价旧版 unwrap_or(0/255) 的尾部行为）
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
                None => {
                    // ★ RJNI-007 修复：JNI 规范——有 pending exception 时不得调其他 JNI 函数。
                    //   旧实现先 unlockCanvasAndPost 后 clear → 持异常 unlock 常失败 → Surface 永久锁死
                    //   / "No pending exception expected" abort。改为先清异常再 unlock。
                    if env.exception_check().unwrap_or(false) { let _ = env.exception_clear(); }
                    let _ = env.call_method(&surface, "unlockCanvasAndPost", "(Landroid/graphics/Canvas;)V", &[JValue::Object(&canvas)]);
                    return Err("createScaledBitmap failed".into());
                }
            };
        // ⑤ Canvas.drawBitmap(scaled, 0,0, null)
        if let Err(e) = env.call_method(&canvas, "drawBitmap", "(Landroid/graphics/Bitmap;FFLandroid/graphics/Paint;)V",
            &[JValue::Object(&scaled), JValue::Float(0.0), JValue::Float(0.0), JValue::Object(&JObject::null())]) {
            // ★ RJNI-007：先清挂起异常再 unlock（顺序不可颠倒，持异常调 JNI 非法）
            if env.exception_check().unwrap_or(false) { let _ = env.exception_clear(); }
            let _ = env.call_method(&surface, "unlockCanvasAndPost", "(Landroid/graphics/Canvas;)V", &[JValue::Object(&canvas)]);
            return Err(format!("drawBitmap failed: {}", e));
        }

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
