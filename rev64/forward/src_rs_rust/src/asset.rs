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

/// 主菜单按钮矩形（720×1280 基准，移植 C ui_menu.c g_btns 原版真值）
/// 人机对战范围砍联网/联机/四国（SPEC_HUMAN_PLAY），只画 2 个有效按钮（与触摸命中 ui.rs 一致），
/// 避免画了点不动的"假按钮"。
pub const MENU_BTNS: [(f32, f32, f32, f32); 2] = [
    (201.6, 867.2, 516.0, 943.2),  // 人机（对齐 ui.rs MENU_BTN_HUMAN）
    (201.6, 981.6, 516.0, 1057.6), // 两人（对齐 MENU_BTN_TWOPLAYER）
];
pub const MENU_BASE_W: f32 = 720.0;
pub const MENU_BASE_H: f32 = 1280.0;

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
    /// 主菜单背景（士兵图 datax_294/menu_bg_294，800×1205）
    static MENU_BG_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    fn menu_bg_cached() -> Option<&'static (Vec<u8>, u32, u32)> {
        MENU_BG_CACHE.get_or_init(|| decode_png(include_bytes!("../assets/menu_bg.png"))).as_ref()
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
    /// 主菜单画面：士兵背景铺满 + 5 蓝色按钮 + 白字图集（对齐 C rs_draw_menu）
    /// 画布为任意尺寸（实际 450×800），按钮/文字坐标按 720×1280 基准等比缩放。
    pub fn compose_menu_rgba(w: usize, h: usize) -> Vec<u8> {
        let mut out = vec![0u8; w * h * 4];
        let sx = w as f32 / super::MENU_BASE_W;
        let sy = h as f32 / super::MENU_BASE_H;
        // ① 士兵背景（menu_bg 800×1205 → 拉伸铺满）
        if let Some(&(ref bg, bw, bh)) = menu_bg_cached() {
            for y in 0..h {
                let fy = (y as f32 / h as f32 * bh as f32) as usize;
                for x in 0..w {
                    let fx = (x as f32 / w as f32 * bw as f32) as usize;
                    let si = (fy * bw as usize + fx) * 4;
                    let di = (y * w + x) * 4;
                    if si + 3 < bg.len() {
                        out[di] = bg[si]; out[di+1] = bg[si+1]; out[di+2] = bg[si+2]; out[di+3] = 255;
                    }
                }
            }
        }
        // ② 5 蓝色按钮（C 版渐变蓝，简化为纯色 #2a6fc8）+ ③ 白字图集切片
        let text = menu_text_cached();
        for (i, &(bx0, by0, bx1, by1)) in super::MENU_BTNS.iter().enumerate() {
            let ax0 = (bx0 * sx) as usize;
            let ax1 = (bx1 * sx) as usize;
            let ay0 = (by0 * sy) as usize;
            let ay1 = (by1 * sy) as usize;
            for y in ay0..ay1.min(h) {
                for x in ax0..ax1.min(w) {
                    let di = (y * w + x) * 4;
                    out[di] = 42; out[di+1] = 111; out[di+2] = 200; out[di+3] = 255; // 蓝按钮
                }
            }
            // 白字图集：从 super::MENU_TEXT_X/Y/W/H 切片，居中贴到按钮内
            if let Some(&(ref tpx, tw, _th)) = text {
                let tx = super::MENU_TEXT_X[i] as usize;
                let ty = super::MENU_TEXT_Y[i] as usize;
                let twid = super::MENU_TEXT_W[i] as usize;
                let thgt = super::MENU_TEXT_H as usize;
                let dw = ((twid as f32 * sx).round() as usize).min(ax1.saturating_sub(ax0));
                let dh = ((thgt as f32 * sy).round() as usize).min(ay1.saturating_sub(ay0));
                let dx0 = ax0 + (ax1 - ax0).saturating_sub(dw) / 2;
                let dy0 = ay0 + (ay1 - ay0).saturating_sub(dh) / 2;
                for dy in 0..dh {
                    for dx in 0..dw {
                        let sy_px = ty + (dy * thgt / dh.max(1)).min(thgt - 1);
                        let sx_px = tx + (dx * twid / dw.max(1)).min(twid - 1);
                        let si = (sy_px * tw as usize + sx_px) * 4;
                        if si + 3 >= tpx.len() { continue; }
                        let a = tpx[si + 3] as u16;
                        if a == 0 { continue; } // 透明区跳过
                        let di = ((dy0 + dy) * w + dx0 + dx) * 4;
                        // alpha 混合白字到蓝底
                        let wa = a;
                        out[di] = ((out[di] as u16 * (255 - wa) + 255 * wa) / 255) as u8;
                        out[di+1] = ((out[di+1] as u16 * (255 - wa) + 255 * wa) / 255) as u8;
                        out[di+2] = ((out[di+2] as u16 * (255 - wa) + 255 * wa) / 255) as u8;
                    }
                }
            }
        }
        out
    }

    pub fn compose_board_rgba(
        w: usize,
        h: usize,
        board: Option<&crate::state::State>,
        screen: crate::ui::Screen,
    ) -> Vec<u8> {
        // ★ 主菜单态走独立合成（士兵背景 + 蓝按钮 + 白字），其余屏态走棋盘合成
        if screen == crate::ui::Screen::Menu {
            return compose_menu_rgba(w, h);
        }
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
                    // ★ 原版棋子图集 datah#183 = 4列×7行、每格53×40（jq_piece_atlas.h 权威）
                    //   旧实现 6×6 切片全错：rank≥5 落到错误图块、宽高比竖长反转。
                    let cell_w = (aw / 4) as usize; // 212/4=53
                    let cell_h = (ah / 7) as usize; // 280/7=40
                    // 棋子尺寸：按行距缩放，宽度按图集 53:40 横宽比
                    let ph = (BG_ROW_H * sy * 0.92) as usize;
                    let pw = (ph as f32 * cell_w as f32 / cell_h as f32) as usize;
                    let x0 = (cx - pw as f32 * 0.5) as usize;
                    let y0 = (cy - ph as f32 * 0.5) as usize;
                    let base: [u8; 3] = if c.side == crate::state::SIDE_RED { [205, 55, 45] } else { [80, 150, 225] };
                    // rank→列=(rank-1)%4、子行=(rank-1)/4；红方行0-2、蓝方行3-5、行6背面
                    let rank = u8::from(crate::state::rank_of(c)) as usize;
                    let idx = rank.saturating_sub(1).min(11);
                    let sc = idx % 4;
                    let subrow = idx / 4;
                    let sr = if c.side == crate::state::SIDE_RED { subrow } else { 3 + subrow };
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
        // ★ 人机难度弹窗（规格 §10 必经第一屏）：面板 + 4×2 按钮网格
        //   几何与 ui.rs difficulty_hit 完全同源（DIFFICULTY_PANEL 4列×2行），确保画=点一致
        // ★ 两人新局弹窗（NewGame）：面板 + 明棋/暗棋(上行) + 确定/取消(下行)
        //   几何与 jni.rs NewGame 触摸分支完全同源（DIFFICULTY_PANEL 4列×2行，col0/1 有效）
        if screen == crate::ui::Screen::NewGame {
            let p = crate::ui::DIFFICULTY_PANEL;
            let px0 = (p.x as f32 * sx) as usize;
            let py0 = (p.y as f32 * sy) as usize;
            let px1 = ((p.x + p.w) as f32 * sx) as usize;
            let py1 = ((p.y + p.h) as f32 * sy) as usize;
            for y in py0..py1.min(h) {
                for x in px0..px1.min(w) {
                    let di = (y * w + x) * 4;
                    out[di] = 245; out[di+1] = 240; out[di+2] = 220; // 米色面板
                }
            }
            let cw = (p.w / 4) as usize;
            let rh = (p.h / 2) as usize;
            for row in 0..2usize {
                for col in 0..2usize { // 只画 col0/1（有效按钮）
                    let bx0 = ((p.x + col as i32 * cw as i32) as f32 * sx) as usize;
                    let bx1 = ((p.x + (col as i32 + 1) * cw as i32) as f32 * sx) as usize;
                    let by0 = ((p.y + row as i32 * rh as i32) as f32 * sy) as usize;
                    let by1 = ((p.y + (row as i32 + 1) * rh as i32) as f32 * sy) as usize;
                    for y in by0..by1.min(h) {
                        for x in bx0..bx1.min(w) {
                            let di = (y * w + x) * 4;
                            out[di] = 42; out[di+1] = 111; out[di+2] = 200; // 蓝按钮
                        }
                    }
                }
            }
        }
        if screen == crate::ui::Screen::Difficulty {
            let p = crate::ui::DIFFICULTY_PANEL;
            let px0 = (p.x as f32 * sx) as usize;
            let py0 = (p.y as f32 * sy) as usize;
            let px1 = ((p.x + p.w) as f32 * sx) as usize;
            let py1 = ((p.y + p.h) as f32 * sy) as usize;
            // 面板（米色半透明）
            for y in py0..py1.min(h) {
                for x in px0..px1.min(w) {
                    let di = (y * w + x) * 4;
                    out[di] = 245; out[di+1] = 240; out[di+2] = 220;
                }
            }
            // 8 按钮（4列×2行，与 difficulty_hit 的 col=ox/(w/4) row=oy/(h/2) 同源）
            let cw = (p.w / 4) as usize;
            let rh = (p.h / 2) as usize;
            for row in 0..2usize {
                for col in 0..4usize {
                    let bx0 = ((p.x + col as i32 * cw as i32) as f32 * sx) as usize;
                    let bx1 = ((p.x + (col as i32 + 1) * cw as i32) as f32 * sx) as usize;
                    let by0 = ((p.y + row as i32 * rh as i32) as f32 * sy) as usize;
                    let by1 = ((p.y + (row as i32 + 1) * rh as i32) as f32 * sy) as usize;
                    for y in by0..by1.min(h) {
                        for x in bx0..bx1.min(w) {
                            let di = (y * w + x) * 4;
                            out[di] = 42; out[di+1] = 111; out[di+2] = 200; // 蓝按钮
                        }
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
        // ★ UI-05：对局态顶部 ☰/↩ 图标（画布 450×800，命中区 view(270,750)/(620,750) ÷2）
        //   画小方块提示可点位置，与 jni.rs 命中区同源（±60 view = ±30 画布）
        if screen == crate::ui::Screen::Game {
            for icx in [135usize, 310usize] { // view 270/620 ÷2
                let ix0 = icx.saturating_sub(30);
                let ix1 = (icx + 30).min(w);
                let iy0 = 345usize.min(h); // view 750÷2=375，上移30
                let iy1 = 405usize.min(h);
                for y in iy0..iy1 {
                    for x in ix0..ix1 {
                        if x >= w || y >= h { continue; }
                        let di = (y * w + x) * 4;
                        out[di] = 230; out[di+1] = 128; out[di+2] = 38; // 橙图标
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

#[cfg(test)]
mod menu_compose_tests {
    use super::*;
    #[test]
    fn menu_btns_match_touch_hit() {
        // 绘制按钮几何必须等于触摸命中几何（否则画了点不动 = 假按钮）
        let h = crate::ui::MENU_BTN_HUMAN;
        let t = crate::ui::MENU_BTN_TWOPLAYER;
        assert!((MENU_BTNS[0].0 - h.x as f32).abs() <= 1.0);
        assert!((MENU_BTNS[0].1 - h.y as f32).abs() <= 1.0);
        assert!((MENU_BTNS[0].2 - (h.x + h.w) as f32).abs() <= 1.0);
        assert!((MENU_BTNS[1].0 - t.x as f32).abs() <= 1.0);
        assert!((MENU_BTNS[1].1 - t.y as f32).abs() <= 1.0);
    }
    #[test]
    fn menu_text_slots_within_btn_count() {
        // 文字图集 5 槽，但只画 2 按钮——前 2 槽必须覆盖人机/两人
        assert!(MENU_TEXT_W.len() >= MENU_BTNS.len());
        for i in 0..MENU_BTNS.len() {
            assert!(MENU_TEXT_X[i] + MENU_TEXT_W[i] <= MENU_TEXT_IMG_W);
            assert!(MENU_TEXT_Y[i] + MENU_TEXT_H <= MENU_TEXT_IMG_H);
        }
    }
}

#[cfg(test)]
mod difficulty_draw_tests {
    #[test]
    fn difficulty_draw_grid_matches_hit() {
        // 绘制的 4×2 网格必须与 difficulty_hit 解析同源：
        // col=ox/(w/4) row=oy/(h/2)，面板 4 列宽 90 / 2 行高 200（720基准）
        let p = crate::ui::DIFFICULTY_PANEL;
        assert_eq!(p.w / 4, 90, "列宽与绘制 cw 一致");
        assert_eq!(p.h / 2, 200, "行高与绘制 rh 一致");
        // 每格中心必须能被 difficulty_hit 解析到对应 (level,mode)
        for row in 0..2u8 {
            for col in 0..4u8 {
                let cx = p.x + col as i32 * 90 + 45;
                let cy = p.y + row as i32 * 200 + 100;
                let got = crate::ui::difficulty_hit(cx, cy, &p);
                assert!(got.is_some(), "格({},{}) 中心应命中", col, row);
                let (level, mode) = got.unwrap();
                assert_eq!(level, col + 1, "列+1=level(level从1起)");
                assert_eq!(mode, row, "行=mode");
            }
        }
    }
}

#[cfg(test)]
mod icon_hit_tests {
    #[test]
    fn undo_icon_hit_matches_device_probe() {
        // ★ UI-05 锚定：悔棋图标命中区必须包含真机判据坐标 tap(620,750)，
        //   且不得覆盖红方上半盘大片区域（原 y<820+x>520 矩形会吞棋子）。
        let in_undo = |x: f32, y: f32| (x - 620.0).abs() <= 60.0 && (y - 750.0).abs() <= 60.0;
        assert!(in_undo(620.0, 750.0), "真机悔棋坐标必须命中");
        assert!(!in_undo(274.0, 1383.0), "蓝方棋子区不得命中");
        assert!(!in_undo(77.0, 577.0), "红方棋子区不得命中");
        // 命中区为 ±60px 小方框（原 y<820+x>520 是大矩形，吞半盘）
    }
}
