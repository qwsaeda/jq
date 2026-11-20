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

    /// 播放事件音效（Activity.getAssets().openFd → AssetFileDescriptor → setDataSource(fd,offset,len)）
    /// ★ 正确 API：file:///android_asset/ 在 setDataSource(String) 不支持，须走 AssetManager.openFd
    pub fn play(env: &mut JNIEnv, context: &jni::objects::JObject<'_>, sfx_name: &str) -> bool {
        if wav_bytes(sfx_name).is_none() { return false; }
        let prefix = if sfx_name == "close" || sfx_name == "pop" { "data_ui" } else { "data_junqi" };
        let asset_path = format!("jq_sfx/{}_{}.wav", prefix, sfx_name);
        // ① Context.getAssets() → AssetManager
        let assets = match env.call_method(context, "getAssets", "()Landroid/content/res/AssetManager;", &[])
            .ok().and_then(|v| v.l().ok()) { Some(a) => a, None => return false };
        // ② AssetManager.openFd(String) → AssetFileDescriptor
        let path_obj = match env.new_string(&asset_path) { Ok(s) => s, Err(_) => return false };
        let afd = match env.call_method(&assets, "openFd", "(Ljava/lang/String;)Landroid/content/res/AssetFileDescriptor;", &[JValue::Object(&path_obj)])
            .ok().and_then(|v| v.l().ok()) { Some(f) => f, None => return false };
        // ③ AssetFileDescriptor.getFileDescriptor() → FileDescriptor
        let fd = match env.call_method(&afd, "getFileDescriptor", "()Ljava/io/FileDescriptor;", &[])
            .ok().and_then(|v| v.l().ok()) { Some(f) => f, None => return false };
        // ④ getStartOffset() / getLength()
        let offset = env.call_method(&afd, "getStartOffset", "()J", &[]).ok().and_then(|v| v.j().ok()).unwrap_or(0);
        let length = env.call_method(&afd, "getLength", "()J", &[]).ok().and_then(|v| v.j().ok()).unwrap_or(-1);
        // ⑤ MediaPlayer.setDataSource(FileDescriptor, long, long)
        let mp_cls = match env.find_class("android/media/MediaPlayer") { Ok(c) => c, Err(_) => return false };
        let mp = match env.new_object(&mp_cls, "()V", &[]) { Ok(o) => o, Err(_) => return false };
        let ok_ds = env.call_method(&mp, "setDataSource", "(Ljava/io/FileDescriptor;JJ)V",
            &[JValue::Object(&fd), JValue::Long(offset), JValue::Long(length)]).is_ok();
        if !ok_ds { return false; }
        let ok_prep = env.call_method(&mp, "prepare", "()V", &[]).is_ok();
        let ok_start = env.call_method(&mp, "start", "()V", &[]).is_ok();
        ok_prep && ok_start
    }
}
#[cfg(target_os = "android")]
pub use imp::play;
#[cfg(not(target_os = "android"))]
pub fn play<E, C>(_env: E, _context: C, _name: &str) -> bool { false }
