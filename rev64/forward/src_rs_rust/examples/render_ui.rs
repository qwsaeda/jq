//! 离线确定性 UI 渲染门禁：把 asset.rs 纯 Rust 合成器在桌面出图（900×1600），
//! 与原版真实截图 uicmp/orig_*.png 逐像素/逐几何对齐。零设备依赖、可重复。
//! 用法：cargo run --release --example render_ui
use junqi_rs::asset::compose_board_rgba;
use junqi_rs::state::State;
use junqi_rs::ui::Screen;
use png::Encoder;
use std::fs;

fn save_png(path: &str, rgba: &[u8], w: u32, h: u32) {
    let file = fs::File::create(path).expect("create png");
    let mut enc = Encoder::new(file, w, h);
    enc.set_color(png::ColorType::Rgba);
    enc.set_depth(png::BitDepth::Eight);
    let mut writer = enc.write_header().expect("png header");
    writer.write_image_data(rgba).expect("png data");
}

fn main() {
    let out = "/tmp/opencode/ui_offline";
    fs::create_dir_all(out).expect("mkdir");
    let st = State::new(); // 默认暗棋（原版 g_opt_mode=1）
    let screens: [(&str, Screen); 10] = [
        ("menu", Screen::Menu),
        ("game", Screen::Game),
        ("ingame", Screen::IngameMenu),
        ("difficulty", Screen::Difficulty),
        ("newgame", Screen::NewGame),
        ("gameset", Screen::GameSet),
        ("settings", Screen::Settings),
        ("exitconfirm", Screen::ExitConfirm),
        ("replay", Screen::Replay),
        ("stats", Screen::Stats),
    ];
    for (name, screen) in screens {
        let rgba = compose_board_rgba(900, 1600, Some(&st), screen, None);
        assert_eq!(rgba.len(), 900 * 1600 * 4, "{} RGBA 尺寸", name);
        let p = format!("{}/{}.png", out, name);
        save_png(&p, &rgba, 900, 1600);
        println!("rendered {}", p);
    }
}
