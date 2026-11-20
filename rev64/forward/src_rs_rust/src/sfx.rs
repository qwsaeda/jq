//! sfx — 原版 14 WAV 音效播放（业务真值：事件→WAV，对齐 C evt55_sfx + sfx_wav）
//! WAV 用 include_bytes! 编进 so（200K），落地到 app 私有目录后用 MediaPlayer 播放。
#[cfg(target_os = "android")]
mod imp {
    use jni::objects::{JObject, JValue};
    use jni::JNIEnv;
    use std::io::Write;

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

    /// 播放事件音效（WAV 落地 + MediaPlayer.setDataSource + start）
    pub fn play(env: &mut JNIEnv, sfx_name: &str) -> bool {
        let bytes = match wav_bytes(sfx_name) { Some(b) => b, None => return false };
        // 落地到 app 私有目录（MediaPlayer 需要文件路径）
        let path = format!("/data/user/0/com.qwsaeda.junqirs/files/jq_sfx_{}.wav", sfx_name);
        if let Ok(mut f) = std::fs::File::create(&path) {
            let _ = f.write_all(bytes);
        } else { return false; }
        // MediaPlayer.create(context, Uri) 需 context——简化用 setDataSource(path)
        let mp_cls = match env.find_class("android/media/MediaPlayer") { Ok(c) => c, Err(_) => return false };
        let mp = match env.new_object(&mp_cls, "()V", &[]) { Ok(o) => o, Err(_) => return false };
        // setDataSource(String)
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
