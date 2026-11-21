//! asset — 纯 Rust SPAK 原版资源解码（png crate）+ CPU 合成棋盘/棋子画面
//! 资源用 include_bytes! 编进 so（免运行时 assets I/O），解码后合成 RGBA → Canvas 显示。
/// 主菜单文字图集切片元数据（移植 C render.c:1496-1499 T_W/T_X/T_Y/T_H，业务真值）
/// 5 槽 = 人机/两人/联网/联机/四国；图集 136×214 竖排，每槽高 38
pub const MENU_TEXT_W: [u32; 5] = [136, 136, 66, 68, 134];
pub const MENU_TEXT_X: [u32; 5] = [0, 0, 35, 34, 1];
pub const MENU_TEXT_Y: [u32; 5] = [0, 44, 88, 132, 176];
pub const MENU_TEXT_H: u32 = 38;
pub const MENU_TEXT_IMG_W: u32 = 136;
pub const MENU_TEXT_IMG_H: u32 = 214;

#[cfg(target_os = "android")]
mod imp {
    use std::io::Cursor;

    /// 缓存解码结果（避免每帧重复解码 712KB 背景 + 棋子图集）
    static BG_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static ATLAS_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    /// 主菜单按钮文字图集（原版 menu_text_atlas.png 136×214，5 行白字）
    static MENU_TEXT_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    fn bg_cached() -> Option<&'static (Vec<u8>, u32, u32)> {
        BG_CACHE.get_or_init(|| decode_png(include_bytes!("../assets/board_bg.png"))).as_ref()
    }
    fn atlas_cached() -> Option<&'static (Vec<u8>, u32, u32)> {
        ATLAS_CACHE.get_or_init(|| decode_png(include_bytes!("../assets/piece_atlas.png"))).as_ref()
    }
    fn menu_text_cached() -> Option<&'static (Vec<u8>, u32, u32)> {
        MENU_TEXT_CACHE.get_or_init(|| decode_png(include_bytes!("../assets/menu_text_atlas.png"))).as_ref()
    }

    /// 解码 PNG 为 RGBA (w,h)
    /// ★ 用 png crate 的 Transformations 直接输出 RGBA8（替代手写 RGB→RGBA 循环）：
    ///   EXPAND|ALPHA|STRIP_16 让 RGB 源自动补 alpha=255、RGBA 源原样、16bit 降 8bit。
    ///   实证（/tmp/pngcmp）：对 board_bg(RGBA)/piece_atlas(RGB) 两图，输出与旧手动转换逐字节一致。
    fn decode_png(bytes: &[u8]) -> Option<(Vec<u8>, u32, u32)> {
        let mut decoder = png::Decoder::new(Cursor::new(bytes));
        decoder.set_transformations(
            png::Transformations::EXPAND
                | png::Transformations::ALPHA
                | png::Transformations::STRIP_16,
        );
        let mut reader = decoder.read_info().ok()?;
        let mut buf = vec![0u8; reader.output_buffer_size()];
        let info = reader.next_frame(&mut buf).ok()?;
        Some((buf[..info.buffer_size()].to_vec(), info.width, info.height))
    }

    /// 合成画面：背景缩放铺满 + 棋盘格 + 棋子图集贴图（纯 Rust CPU）
    pub fn compose_board_rgba(
        w: usize,
        h: usize,
        board: Option<&crate::state::State>,
        screen: crate::ui::Screen,
    ) -> Vec<u8> {
        let mut out = vec![0u8; w * h * 4];
        // ① 背景（board_bg 720×1280 = 原版 datax_177 棋盘底图 → 拉伸铺满 w×h）
        if let Some(&(ref bg, bw, bh)) = bg_cached() {
            for y in 0..h {
                let sy = (y as f32 / h as f32 * bh as f32) as usize;
                for x in 0..w {
                    let sx = (x as f32 / w as f32 * bw as f32) as usize;
                    let si = (sy * bw as usize + sx) * 4;
                    let di = (y * w + x) * 4;
                    if si + 3 < bg.len() {
                        out[di] = bg[si]; out[di+1] = bg[si+1]; out[di+2] = bg[si+2]; out[di+3] = 255;
                    }
                }
            }
        }
        // ② 棋盘 12×5 格 + 棋子图集贴图
        // ★ 业务对齐（2026-11-20）：棋子位置改用原版 BG_* 几何真值（与 touch view_to_cell 同一套），
        //   不再用自创的均分公式（cw=w/5, ch=board_h/12）——触摸/渲染几何一致。
        let atlas = atlas_cached().map(|(d,w,h)| (&d[..], *w, *h));
        // 原版基准 720×1280（board_geom.h BG_IMG_W/H + BG_GRID_* 真值）
        const BG_IMG_W: f32 = 720.0;
        const BG_IMG_H: f32 = 1280.0;
        const BG_COL0_CX: f32 = 78.5;
        const BG_COL_W: f32 = 140.5;
        const BG_ROW0_CY: f32 = 222.0;
        const BG_ROW_H: f32 = 71.0;
        const BG_ROW6_CY: f32 = 822.0;
        // 基准→当前渲染尺寸缩放
        let sx = w as f32 / BG_IMG_W;
        let sy = h as f32 / BG_IMG_H;
        let bg_row_cy = |image_row: usize| -> f32 {
            if image_row <= 5 { BG_ROW0_CY + image_row as f32 * BG_ROW_H }
            else { BG_ROW6_CY + (image_row - 6) as f32 * BG_ROW_H }
        };
        // ★ 审计 A10：仅对局/终局态画棋子；菜单/弹窗态只画背景（屏态分屏）
        let show_pieces = matches!(screen, crate::ui::Screen::Game | crate::ui::Screen::IngameMenu | crate::ui::Screen::Replay);
        if show_pieces {
        if let Some(st) = board {
            for device_row in 0..12usize {
                for col in 0..5usize {
                    let idx = device_row * 5 + col;
                    let c = &st.cells[idx];
                    if c.is_empty() { continue; }
                    // ★ 原版映射：device_row → image_row = 11-row（与 bg_cell_img_center 一致）
                    let image_row = 11 - device_row;
                    let cx = BG_COL0_CX + col as f32 * BG_COL_W; // 列中心（原版真值）
                    let cy = bg_row_cy(image_row);                // 行中心（原版真值）
                    // 缩放到当前渲染尺寸
                    let cx = cx * sx;
                    let cy = cy * sy;
                    // 棋子按图集 cell 宽高比（35×46）渲染
                    let (atile, aw, ah): (&[u8], u32, u32) = atlas.unwrap_or((&[][..],0,0));
                    let cell_w = (aw / 6) as usize; // ≈35
                    let cell_h = (ah / 6) as usize; // ≈46
                    // 棋子尺寸：按行距（BG_ROW_H×sy）缩放，宽度按图集比例
                    let ph = (BG_ROW_H * sy * 0.92) as usize;
                    let pw = (ph as f32 * cell_w as f32 / cell_h as f32) as usize;
                    let x0 = (cx - pw as f32 * 0.5) as usize; // 中心对齐（原版 cell 中心）
                    let y0 = (cy - ph as f32 * 0.5) as usize;
                    let base: [u8; 3] = if c.side == crate::state::SIDE_RED { [205, 55, 45] } else { [80, 150, 225] };
                    // 图集 idx：rank1-12 → 0-11
                    let rank = u8::from(crate::state::rank_of(c)) as usize;
                    let idx = rank.saturating_sub(1).min(11);
                    let sc = idx % 6;
                    let sr = if c.side == crate::state::SIDE_RED { idx / 6 } else { 3 + idx / 6 };
                    for py in 0..ph {
                        for px in 0..pw {
                            let dx = x0 + px;
                            let dy = y0 + py;
                            if dx >= w || dy >= h { continue; }
                            let nx = px as f32 / pw as f32;
                            let ny = py as f32 / ph as f32;
                            // 椭圆棋子外形
                            let in_piece = ((nx - 0.5).powi(2) / 0.22 + (ny - 0.5).powi(2) / 0.24) < 1.0;
                            if in_piece {
                                let di = (dy * w + dx) * 4;
                                if aw > 0 && ah > 0 {
                                    // UV 直接线性映射整个 cell（含透明边），1:1 比例不拉伸
                                    let ax = (sc * cell_w + px * cell_w / pw).min(aw as usize - 1);
                                    let ay = (sr * cell_h + py * cell_h / ph).min(ah as usize - 1);
                                    let ai = (ay * aw as usize + ax) * 4;
                                    if ai + 3 < atile.len() && atile[ai+3] > 10 {
                                        let ar = atile[ai] as u32; let ag = atile[ai+1] as u32; let ab = atile[ai+2] as u32;
                                        // 跳过图集 cell 间的黑色分隔（亮度<45）：用阵营底色
                                        if ar + ag + ab > 120 {
                                            out[di] = ar as u8; out[di+1] = ag as u8; out[di+2] = ab as u8; out[di+3] = 255; continue;
                                        }
                                    }
                                }
                                out[di] = base[0]; out[di+1] = base[1]; out[di+2] = base[2]; out[di+3] = 255;
                            }
                        }
                    }
                }
            }
        }
        } // end show_pieces（含内部 if let Some(st)）
        // ★ 业务对齐：终局胜负横幅（对齐 C rs_draw_overlay 真值）
        //   蓝(0.15,0.35,0.85)/红(0.85,0.20,0.20)/和棋灰(0.55,0.55,0.55)，中心横幅 + 半透明
        // ★ 复盘态底部控制条（对齐 C datah_107 底部 5 按钮控制条）
        // ★ 设置类弹窗面板（GameSet 4 开关 / Settings 音效）——屏态分屏绘制
        if matches!(screen, crate::ui::Screen::GameSet | crate::ui::Screen::Settings) {
            // 面板（居中 60%×40%）
            let px0 = (w as f32 * 0.20) as usize;
            let px1 = (w as f32 * 0.80) as usize;
            let py0 = (h as f32 * 0.30) as usize;
            let py1 = (h as f32 * 0.70) as usize;
            for y in py0..py1 {
                for x in px0..px1 {
                    let di = (y * w + x) * 4;
                    out[di] = 245; out[di+1] = 240; out[di+2] = 220; // 米色面板
                }
            }
        }
        // ★ 游戏内菜单（☰）8 按钮面板（原版 4×2 几何，720×1280 基准 → 缩放）
        if screen == crate::ui::Screen::IngameMenu {
            for &(x0, y0, x1, y1) in crate::ui::INGAME_BTNS.iter() {
                let ax0 = (x0 as f32 * sx) as usize;
                let ay0 = (y0 as f32 * sy) as usize;
                let ax1 = (x1 as f32 * sx) as usize;
                let ay1 = (y1 as f32 * sy) as usize;
                for y in ay0..ay1.min(h) {
                    for x in ax0..ax1.min(w) {
                        let di = (y * w + x) * 4;
                        out[di] = 230; out[di+1] = 128; out[di+2] = 38; // 橙色按钮
                    }
                }
            }
        }
        if screen == crate::ui::Screen::Replay {
            let bar_y0 = (h as f32 * 0.88) as usize;
            let bar_y1 = (h as f32 * 0.96) as usize;
            for y in bar_y0..bar_y1 {
                for x in (w as f32 * 0.05) as usize..(w as f32 * 0.95) as usize {
                    let di = (y * w + x) * 4;
                    out[di] = 40; out[di+1] = 45; out[di+2] = 55; // 深灰控制条
                }
            }
            // 5 按钮（菜单/回退/前进/跳转/返回）
            for i in 0..5 {
                let bx0 = (w as f32 * (0.08 + i as f32 * 0.17)) as usize;
                let bx1 = bx0 + (w as f32 * 0.14) as usize;
                for y in (bar_y0 + 4)..(bar_y1 - 4) {
                    for x in bx0..bx1.min(w) {
                        let di = (y * w + x) * 4;
                        out[di] = 230; out[di+1] = 128; out[di+2] = 38; // 橙色按钮
                    }
                }
            }
        }
        if let Some(st) = board {
            if st.stage == crate::state::Stage::Over {
                let (br, bg, bb): (u8, u8, u8) = match st.winner {
                    crate::state::Winner::Blue => (38, 89, 217),   // 0.15,0.35,0.85 ×255
                    crate::state::Winner::Red => (217, 51, 51),    // 0.85,0.20,0.20 ×255
                    _ => (140, 140, 140),                           // 和棋灰 0.55 ×255
                };
                // 中心横幅（C rs_draw_overlay 真值：x20%-80% y42%-58%）
                let by0 = (h as f32 * 0.42) as usize;
                let by1 = (h as f32 * 0.58) as usize;
                let bx0 = (w as f32 * 0.20) as usize;
                let bx1 = (w as f32 * 0.80) as usize;
                for y in by0..by1 {
                    for x in bx0..bx1 {
                        let di = (y * w + x) * 4;
                        // alpha 0.9 混合
                        out[di] = ((out[di] as u16 * 1 + br as u16 * 9) / 10) as u8;
                        out[di+1] = ((out[di+1] as u16 * 1 + bg as u16 * 9) / 10) as u8;
                        out[di+2] = ((out[di+2] as u16 * 1 + bb as u16 * 9) / 10) as u8;
                    }
                }
                // ★ 对齐原版：终局"再来一局"绿色按钮（C rs_draw_overlay x35%-65% y62%-72%）
                let gy0 = (h as f32 * 0.62) as usize;
                let gy1 = (h as f32 * 0.72) as usize;
                let gx0 = (w as f32 * 0.35) as usize;
                let gx1 = (w as f32 * 0.65) as usize;
                for y in gy0..gy1 {
                    for x in gx0..gx1 {
                        let di = (y * w + x) * 4;
                        // 绿色 0.20,0.75,0.30 ×255 alpha 0.9（对齐 C glUniform4f）
                        out[di] = ((out[di] as u16 + 51 * 9) / 10) as u8;
                        out[di+1] = ((out[di+1] as u16 + 191 * 9) / 10) as u8;
                        out[di+2] = ((out[di+2] as u16 + 77 * 9) / 10) as u8;
                    }
                }
            }
        }
        out
    }
}
#[cfg(target_os = "android")]
pub use imp::compose_board_rgba;
#[cfg(not(target_os = "android"))]
pub fn compose_board_rgba(_w: usize, _h: usize, _b: Option<&crate::state::State>, _s: crate::ui::Screen) -> Vec<u8> { vec![] }

#[cfg(test)]
mod menu_text_tests {
    use super::*;
    #[test]
    fn menu_text_atlas_metadata_matches_c() {
        // C render.c:1496-1499 真值锚定（防改错切片位置）
        assert_eq!(MENU_TEXT_W, [136, 136, 66, 68, 134]);
        assert_eq!(MENU_TEXT_X, [0, 0, 35, 34, 1]);
        assert_eq!(MENU_TEXT_Y, [0, 44, 88, 132, 176]);
        assert_eq!(MENU_TEXT_H, 38);
        // 每槽切片必须在图集 136×214 内（防越界）
        let img = include_bytes!("../assets/menu_text_atlas.png");
        assert!(img.len() > 1000, "图集字节存在");
        for i in 0..5 {
            assert!(MENU_TEXT_X[i] + MENU_TEXT_W[i] <= MENU_TEXT_IMG_W, "槽{} X 越界", i);
            assert!(MENU_TEXT_Y[i] + MENU_TEXT_H <= MENU_TEXT_IMG_H, "槽{} Y 越界", i);
        }
        // 图集 PNG 头（验证是有效 PNG）
        assert_eq!(&img[..4], &[0x89, b'P', b'N', b'G']);
    }
}
