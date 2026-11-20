//! asset — 纯 Rust SPAK 原版资源解码（png crate）+ CPU 合成棋盘/棋子画面
//! 资源用 include_bytes! 编进 so（免运行时 assets I/O），解码后合成 RGBA → Canvas 显示。
#[cfg(target_os = "android")]
mod imp {
    use std::io::Cursor;

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
    ) -> Vec<u8> {
        let mut out = vec![0u8; w * h * 4];
        // ① 背景（bg_menu 720×1280 → 拉伸铺满 w×h）
        if let Some((bg, bw, bh)) = decode_png(include_bytes!("../assets/bg_menu.png")) {
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
        // ② 棋盘 12×5 格（上半 62%）+ 棋子图集贴图
        let atlas = decode_png(include_bytes!("../assets/piece_atlas.png"));
        let board_h = (h as f32 * 0.62) as usize;
        let cw = w as f32 / 5.0;
        let ch = board_h as f32 / 12.0;
        if let Some(st) = board {
            for device_row in 0..12usize {
                for col in 0..5usize {
                    let idx = device_row * 5 + col;
                    let c = &st.cells[idx];
                    if c.is_empty() { continue; }
                    // 屏幕 row：device_row0=蓝底线在棋盘最下 → screen_row = 11-device_row
                    let screen_row = 11 - device_row;
                    let x0 = (col as f32 * cw) as usize;
                    let y0 = (screen_row as f32 * ch) as usize;
                    let pw = cw as usize;
                    let ph = ch as usize;
                    // 棋子颜色（蓝/红），有图集则用图集对应 rank 切片近似
                    let base: [u8; 3] = if c.side == crate::state::SIDE_RED { [205, 55, 45] } else { [80, 150, 225] };
                    let (atile, aw, ah): (&[u8], u32, u32) = atlas.as_ref().map(|(d,w,h)| (d.as_slice(),*w,*h)).unwrap_or((&[][..],0,0));
                    // 图集 212×280 = 6列×6行：前3行红方(橙)，后3行蓝方；rank 1-12 → 图集 idx 0-11（2行×6列）
                    let rank = crate::state::rank_of(c) as usize;
                    let idx = rank.saturating_sub(1).min(11); // rank1→idx0 ... rank12→idx11
                    let cell_w = aw / 6;
                    let cell_h = ah / 6;
                    let sc = idx % 6;
                    // 红方=图集上半(idx行0-1)，蓝方=图集下半(行3-4)
                    let sr = if c.side == crate::state::SIDE_RED { idx / 6 } else { 3 + idx / 6 };
                    for py in 0..ph {
                        for px in 0..pw {
                            let dx = x0 + px;
                            let dy = y0 + py;
                            if dx >= w || dy >= h { continue; }
                            // 图集采样（椭圆区内用图集色，外透明露背景）
                            let nx = px as f32 / pw as f32;
                            let ny = py as f32 / ph as f32;
                            let in_piece = ((nx - 0.5).powi(2) / 0.16 + (ny - 0.5).powi(2) / 0.22) < 1.0;
                            if in_piece {
                                let di = (dy * w + dx) * 4;
                                if aw > 0 && ah > 0 {
                                    let ax = (sc * cell_w as usize + (px * cell_w as usize / pw)) .min(aw as usize - 1);
                                    let ay = (sr * cell_h as usize + (py * cell_h as usize / ph)).min(ah as usize - 1);
                                    let ai = (ay * aw as usize + ax) * 4;
                                    if ai + 2 < atile.len() { out[di] = atile[ai]; out[di+1] = atile[ai+1]; out[di+2] = atile[ai+2]; out[di+3] = 255; continue; }
                                }
                                out[di] = base[0]; out[di+1] = base[1]; out[di+2] = base[2]; out[di+3] = 255;
                            }
                        }
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
pub fn compose_board_rgba(_w: usize, _h: usize, _b: Option<&crate::state::State>) -> Vec<u8> { vec![] }
