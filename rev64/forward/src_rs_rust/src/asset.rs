//! asset — 纯 Rust SPAK 原版资源解码（png crate）+ CPU 合成棋盘/棋子画面
//! 资源用 include_bytes! 编进 so（免运行时 assets I/O），解码后合成 RGBA → Canvas 显示。
#[cfg(target_os = "android")]
mod imp {
    use std::io::Cursor;

    /// 缓存解码结果（避免每帧重复解码 712KB 背景 + 棋子图集）
    static BG_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static ATLAS_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    fn bg_cached() -> Option<&'static (Vec<u8>, u32, u32)> {
        BG_CACHE.get_or_init(|| decode_png(include_bytes!("../assets/bg_menu.png"))).as_ref()
    }
    fn atlas_cached() -> Option<&'static (Vec<u8>, u32, u32)> {
        ATLAS_CACHE.get_or_init(|| decode_png(include_bytes!("../assets/piece_atlas.png"))).as_ref()
    }

    /// 解码 PNG 为 RGBA (w,h)
    fn decode_png(bytes: &[u8]) -> Option<(Vec<u8>, u32, u32)> {
        let decoder = png::Decoder::new(Cursor::new(bytes));
        let mut reader = decoder.read_info().ok()?;
        let mut buf = vec![0u8; reader.output_buffer_size()];
        let info = reader.next_frame(&mut buf).ok()?;
        let w = info.width;
        let h = info.height;
        // 统一转 RGBA8
        let rgba = match info.color_type {
            png::ColorType::Rgba => buf[..info.buffer_size()].to_vec(),
            png::ColorType::Rgb => {
                let mut out = Vec::with_capacity(w as usize * h as usize * 4);
                for px in buf[..info.buffer_size()].chunks_exact(3) {
                    out.extend_from_slice(&[px[0], px[1], px[2], 255]);
                }
                out
            }
            _ => return None,
        };
        Some((rgba, w, h))
    }

    /// 合成画面：背景缩放铺满 + 棋盘格 + 棋子图集贴图（纯 Rust CPU）
    pub fn compose_board_rgba(
        w: usize,
        h: usize,
        board: Option<&crate::state::State>,
        screen: crate::ui::Screen,
    ) -> Vec<u8> {
        let mut out = vec![0u8; w * h * 4];
        // ① 背景（bg_menu 720×1280 → 拉伸铺满 w×h）
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
        let show_pieces = matches!(screen, crate::ui::Screen::Game | crate::ui::Screen::IngameMenu);
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
                    let rank = crate::state::rank_of(c) as usize;
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
        out
    }
}
#[cfg(target_os = "android")]
pub use imp::compose_board_rgba;
#[cfg(not(target_os = "android"))]
pub fn compose_board_rgba(_w: usize, _h: usize, _b: Option<&crate::state::State>, _s: crate::ui::Screen) -> Vec<u8> { vec![] }
