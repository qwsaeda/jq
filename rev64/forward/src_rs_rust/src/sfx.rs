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

    /// 播放事件音效：WAV 字节 include_bytes 编进 so → 写 app 私有目录 → MediaPlayer.setDataSource(path)
    /// ★ 不依赖 APK assets 的 STORE/压缩（旧 openFd 方案因 WAV 被压缩抛 FileNotFoundException，
    ///   异常未清 → 后续 JNI abort 崩溃）。每步失败先清 pending 异常再返回，音效失败只静默跳过，
    ///   绝不把异常带出到主链（音效非核心功能）。
    /// volume: 0-100（原版 AudioManager.getMinVolume/getMaxVolume + setStereoVolume 语义；
    ///   正向走 MediaPlayer，对应 MediaPlayer.setVolume(float,float)，0..1 线性映射）
    pub fn play(env: &mut JNIEnv, _context: &jni::objects::JObject<'_>, sfx_name: &str, volume: u8) -> bool {
        let wav = match wav_bytes(sfx_name) { Some(b) => b, None => return false };
        // ① 写 WAV 字节到私有目录（纯 Rust，无 JNI）
        let path = format!("/data/user/0/com.qwsaeda.junqirs/cache/jq_sfx_{}.wav", sfx_name);
        if std::fs::write(&path, wav).is_err() { return false; }
        // ② MediaPlayer.setDataSource(String path) + prepare + start，全在 local frame 内，失败清异常
        let result = env.with_local_frame(8, |env| -> Result<bool, jni::errors::Error> {
            let clear = |env: &mut JNIEnv| { if env.exception_check().unwrap_or(false) { let _ = env.exception_clear(); } };
            let mp_cls = match env.find_class("android/media/MediaPlayer") {
                Ok(c) => c, Err(_) => { clear(env); return Ok(false); }
            };
            let mp = match env.new_object(&mp_cls, "()V", &[]) {
                Ok(o) => o, Err(_) => { clear(env); return Ok(false); }
            };
            let path_obj = match env.new_string(&path) {
                Ok(s) => s, Err(_) => { clear(env); return Ok(false); }
            };
            let ok_ds = env.call_method(&mp, "setDataSource", "(Ljava/lang/String;)V",
                &[JValue::Object(&path_obj)]).is_ok();
            if !ok_ds { clear(env); return Ok(false); }
            let ok_prep = env.call_method(&mp, "prepare", "()V", &[]).is_ok();
            if !ok_prep { clear(env); return Ok(false); }
            // ★ 音量路由（原版 setStereoVolume 对应 MediaPlayer.setVolume(float,float)）
            let vol = (volume.min(100) as f32) / 100.0;
            let _ = env.call_method(&mp, "setVolume", "(FF)V",
                &[JValue::Float(vol), JValue::Float(vol)]);
            let ok_start = env.call_method(&mp, "start", "()V", &[]).is_ok();
            if !ok_start { clear(env); }
            Ok(ok_start)
        });
        result.unwrap_or(false)
    }
}
#[cfg(target_os = "android")]
pub use imp::play;
#[cfg(not(target_os = "android"))]
pub fn play<E, C>(_env: E, _context: C, _name: &str, _volume: u8) -> bool { false }

#[cfg(test)]
mod audio_route_tests {
    /// 原版 data/config/sound.xml（SPAK 解包 data_config_sound.xml 真值）声效名覆盖判据：
    /// Junqi SoundSet 12 项 + Ui SoundSet 3 项（Click 资源缺失，用 pop 替代）。
    /// 正向 wav_bytes 必须覆盖所有可落地名，防音频路由退化成"假按钮"。
    #[test]
    fn sound_xml_names_covered() {
        // wav_bytes 是 android 私有函数，桌面用文件存在性验证（assets/wav 目录）
        let junqi = ["start","flip","select","move","eat","flag","dead","bomb","win","lose","draw","over"];
        let ui = ["close", "pop"]; // click.wav 原版 SPAK 解包缺失，用 pop 替代
        let dir = concat!(env!("CARGO_MANIFEST_DIR"), "/assets/wav/");
        for n in junqi.iter().chain(ui.iter()) {
            let p = format!("{}data_{}_{}.wav", dir, if ui.contains(n) { "ui" } else { "junqi" }, n);
            assert!(std::path::Path::new(&p).exists(), "sound.xml 音效 {} 缺 wav: {}", n, p);
        }
    }

    /// 音量线性映射（原版 setStereoVolume 对应 MediaPlayer.setVolume 0..1）
    #[test]
    fn volume_mapping_linear() {
        let vol = |v: u8| (v.min(100) as f32) / 100.0;
        assert!((vol(0) - 0.0).abs() < 1e-6);
        assert!((vol(50) - 0.5).abs() < 1e-6);
        assert!((vol(100) - 1.0).abs() < 1e-6);
        assert!((vol(200) - 1.0).abs() < 1e-6); // clamp
    }
}
