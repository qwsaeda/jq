//! sfx — 原版 14 WAV 音效播放（业务真值：事件→WAV，对齐 C evt55_sfx + sfx_wav）
//! WAV 用 include_bytes! 编进 so（200K），落地到 app 私有目录后用 MediaPlayer 播放。
#[cfg(target_os = "android")]
mod imp {
    use jni::objects::JValue;
    use jni::JNIEnv;

    /// 事件名→WAV 字节（原版 14 WAV）
    fn wav_bytes(name: &str) -> Option<&'static [u8]> {
        match name {
            "move" => Some(include_bytes!("../assets/wav/data_junqi_move.wav")),
            "eat" => Some(include_bytes!("../assets/wav/data_junqi_eat.wav")),
            "bomb" => Some(include_bytes!("../assets/wav/data_junqi_bomb.wav")),
            "flag" => Some(include_bytes!("../assets/wav/data_junqi_flag.wav")),
            "dead" => Some(include_bytes!("../assets/wav/data_junqi_dead.wav")),
            "flip" => Some(include_bytes!("../assets/wav/data_junqi_flip.wav")),
            "win" => Some(include_bytes!("../assets/wav/data_junqi_win.wav")),
            "lose" => Some(include_bytes!("../assets/wav/data_junqi_lose.wav")),
            "draw" => Some(include_bytes!("../assets/wav/data_junqi_draw.wav")),
            "over" => Some(include_bytes!("../assets/wav/data_junqi_over.wav")),
            "select" => Some(include_bytes!("../assets/wav/data_junqi_select.wav")),
            "start" => Some(include_bytes!("../assets/wav/data_junqi_start.wav")),
            "close" => Some(include_bytes!("../assets/wav/data_ui_close.wav")),
            "pop" => Some(include_bytes!("../assets/wav/data_ui_pop.wav")),
            _ => None,
        }
    }

    /// 播放事件音效（APK assets → MediaPlayer，绕开 native 文件 I/O 限制）
    /// assets/jq_sfx/ 已有 14 WAV（apktool 解包自带），用 file:///android_asset/ URI 直接读
    pub fn play(env: &mut JNIEnv, sfx_name: &str) -> bool {
        // 校验是有效音效名（映射表 sfx_for_* 输出的 11 类）
        if wav_bytes(sfx_name).is_none() { return false; }
        let prefix = if sfx_name == "close" || sfx_name == "pop" { "data_ui" } else { "data_junqi" };
        let path = format!("file:///android_asset/jq_sfx/{}_{}.wav", prefix, sfx_name);
        let mp_cls = match env.find_class("android/media/MediaPlayer") { Ok(c) => c, Err(_) => return false };
        let mp = match env.new_object(&mp_cls, "()V", &[]) { Ok(o) => o, Err(_) => return false };
        // setDataSource(String) —— file:///android_asset/ URI 由系统 AssetManager 解析
        let path_obj = match env.new_string(&path) { Ok(s) => s, Err(_) => return false };
        let ok_ds = env.call_method(&mp, "setDataSource", "(Ljava/lang/String;)V", &[JValue::Object(&path_obj)]).is_ok();
        if !ok_ds { return false; }
        let ok_prep = env.call_method(&mp, "prepare", "()V", &[]).is_ok();
        let ok_start = env.call_method(&mp, "start", "()V", &[]).is_ok();
        ok_prep && ok_start
    }
}
#[cfg(target_os = "android")]
pub use imp::play;
#[cfg(not(target_os = "android"))]
pub fn play<E>(_env: E, _name: &str) -> bool { false }
