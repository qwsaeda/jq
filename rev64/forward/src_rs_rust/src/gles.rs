//! gles — JNI 桥：在 GLThread 的 EGL context 内用 GLES20 画内容
//! 替代 lockCanvas（与 GLThread EGL 互斥）。native OnUpdate 被 GLThread.onDrawFrame 调用时
//! EGL context 已 makeCurrent，可直接调 GLES20 静态方法，swap 由 EglHelper 完成。
#[cfg(target_os = "android")]
mod imp {
    use jni::objects::JValue;
    use jni::JNIEnv;

    /// GLES20.glClearColor(r,g,b,a) + glClear(GL_COLOR_BUFFER_BIT=0x4000)
    pub fn gles20_clear(env: &mut JNIEnv, r: f32, g: f32, b: f32, a: f32) -> Result<(), String> {
        env.call_static_method("android/opengl/GLES20", "glClearColor", "(FFFF)V",
            &[JValue::Float(r), JValue::Float(g), JValue::Float(b), JValue::Float(a)])
            .map_err(|e| e.to_string())?;
        env.call_static_method("android/opengl/GLES20", "glClear", "(I)V",
            &[JValue::Int(0x4000)]) // GL_COLOR_BUFFER_BIT
            .map_err(|e| e.to_string())?;
        // glFinish 确保提交到当前 context（GLES 命令异步）
        env.call_static_method("android/opengl/GLES20", "glFinish", "()V", &[])
            .map_err(|e| e.to_string())?;
        Ok(())
    }

    /// GLES20.glViewport(x,y,w,h)
    pub fn gles20_viewport(env: &mut JNIEnv, x: i32, y: i32, w: i32, h: i32) -> Result<(), String> {
        env.call_static_method("android/opengl/GLES20", "glViewport", "(IIII)V",
            &[JValue::Int(x), JValue::Int(y), JValue::Int(w), JValue::Int(h)])
            .map_err(|e| e.to_string())?;
        Ok(())
    }
}
#[cfg(target_os = "android")]
pub use imp::{gles20_clear, gles20_viewport};
#[cfg(not(target_os = "android"))]
pub fn gles20_clear<E>(_env: E, _r: f32, _g: f32, _b: f32, _a: f32) -> Result<(), String> { Err("desktop".into()) }
#[cfg(not(target_os = "android"))]
pub fn gles20_viewport<E>(_env: E, _x: i32, _y: i32, _w: i32, _h: i32) -> Result<(), String> { Err("desktop".into()) }
