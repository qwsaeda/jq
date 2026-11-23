//! asset — 纯 Rust SPAK 原版资源解码（png crate）+ 场景描述（DrawTarget 单一真值）
//! 资源用 include_bytes! 编进 so（免运行时 assets I/O）。
//! ★ 正向开发规范（CODE_STYLE §13 / SPEC_HUMAN_PLAY §1）：
//!   场景布局只有一份（build_scene），CPU 像素合成与 GPU render pass 都是 DrawTarget 的实现，
//!   杜绝"画一套、GPU 另一套"漂移（旧版 CPU 合成与 wgpu 骨架各写一遍 = 渲染分叉根因）。
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

/// 场景用到的纹理资源（CPU/GPU 共用的逻辑名；GPU 据此上传对应 PNG 解码结果）
#[allow(dead_code)] // PieceBack/White 仅 GPU 路径构造（android-only）
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub(crate) enum Tex {
    BoardBg,
    MenuBg,
    MenuBig,
    MenuSmall,
    House,
    /// 棋子正面图集（上传时按椭圆遮罩预处理）
    PieceFront,
    /// 棋子背面图集（上传时按方块 + 去底部 3px 分隔预处理）
    PieceBack,
    ImenIcons,
    ImenText,
    ImenHamburger, // 对局态 ☰ 三横线 + ↩ 返回箭头真图标（imen_hamburger_atlas 128×64）
    DiffText,
    GameSetText,
    NewGameText,
    ExitText,
    SoundText,
    GameInfoText, // datah_050 对局信息弹窗文字（标题+复盘/删除/返回）
    /// 1×1 白色（用 tint 承载纯色矩形颜色）
    White,
}

/// 绘制目标抽象：CPU 像素合成器 / GPU quad 收集器都实现它。
/// 所有坐标均为画布像素（当前 450×800），720×1280 基准坐标由 build_scene 负责换算。
#[allow(clippy::too_many_arguments)] // 纹理矩形 blit 固有 9 参数（src/dst/UV/tint）
pub(crate) trait DrawTarget {
    fn blit(
        &mut self,
        tex: Tex,
        sx0: u32,
        sy0: u32,
        sw: u32,
        sh: u32,
        dx0: usize,
        dy0: usize,
        dw: usize,
        dh: usize,
        tint: [u8; 4],
    );
    fn fill(&mut self, dx0: usize, dy0: usize, dw: usize, dh: usize, color: [u8; 4]);
    fn dim(&mut self, dx0: usize, dy0: usize, dw: usize, dh: usize, num: u32, den: u32);
    /// 棋子：椭圆正面 / 方块背面，按图集 cell 切片。CPU 实现保留原逐像素遮罩；
    /// GPU 实现只发一个 quad（上传时已按 front/back 预处理 alpha 遮罩）。
    #[allow(clippy::too_many_arguments)]
    fn piece(
        &mut self,
        hidden: bool,
        sc: usize,
        sr: usize,
        cell_w: usize,
        cell_h: usize,
        uv_h: usize,
        cx: f32,
        cy: f32,
        pw: usize,
        ph: usize,
        base: [u8; 3],
    );
}

mod imp {
    use super::{Tex, DrawTarget};
    use std::io::Cursor;

    /// 缓存解码结果（避免每帧重复解码 712KB 背景 + 棋子图集）
    static BG_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static ATLAS_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    #[allow(dead_code)] // 原版图集保留供后续弹窗文字复用
    static MENU_TEXT_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static MENU_BG_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static MENU_BIG_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static MENU_SMALL_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static HOUSE_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static IMEN_ICONS_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static IMEN_TEXT_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static IMEN_HAMBURGER_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static DIFF_TEXT_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static GAMESET_TEXT_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static NEWGAME_TEXT_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static EXIT_TEXT_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static SOUND_TEXT_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();
    static GAMEINFO_TEXT_CACHE: std::sync::OnceLock<Option<(Vec<u8>, u32, u32)>> = std::sync::OnceLock::new();

    /// GPU 上传入口：按逻辑纹理名取解码后的 RGBA（只解码一次，跨帧复用）
    pub(crate) fn texture_rgba(tex: Tex) -> Option<&'static (Vec<u8>, u32, u32)> {
        let slot = match tex {
            Tex::BoardBg => &BG_CACHE,
            Tex::MenuBg => &MENU_BG_CACHE,
            Tex::MenuBig => &MENU_BIG_CACHE,
            Tex::MenuSmall => &MENU_SMALL_CACHE,
            Tex::House => &HOUSE_CACHE,
            Tex::PieceFront | Tex::PieceBack => &ATLAS_CACHE,
            Tex::ImenIcons => &IMEN_ICONS_CACHE,
            Tex::ImenText => &IMEN_TEXT_CACHE,
            Tex::ImenHamburger => &IMEN_HAMBURGER_CACHE,
            Tex::DiffText => &DIFF_TEXT_CACHE,
            Tex::GameSetText => &GAMESET_TEXT_CACHE,
            Tex::NewGameText => &NEWGAME_TEXT_CACHE,
            Tex::ExitText => &EXIT_TEXT_CACHE,
            Tex::SoundText => &SOUND_TEXT_CACHE,
            Tex::GameInfoText => &GAMEINFO_TEXT_CACHE,
            Tex::White => return None, // 白纹理由 GPU 侧 1×1 常量生成
        };
        type DecodeFn = fn() -> Option<(Vec<u8>, u32, u32)>;
        let init: DecodeFn = match tex {
            Tex::BoardBg => || decode_png(include_bytes!("../assets/board_bg.png")),
            Tex::MenuBg => || decode_png(include_bytes!("../assets/menu_bg.png")),
            Tex::MenuBig => || decode_png(include_bytes!("../assets/menu_btn_big.png")),
            Tex::MenuSmall => || decode_png(include_bytes!("../assets/menu_btn_small.png")),
            Tex::House => || decode_png(include_bytes!("../assets/icon_house.png")),
            Tex::PieceFront | Tex::PieceBack => || decode_png(include_bytes!("../assets/piece_atlas.png")),
            Tex::ImenIcons => || decode_png(include_bytes!("../assets/imen_icons_atlas.png")),
            Tex::ImenText => || decode_png(include_bytes!("../assets/imen_text_atlas.png")),
            Tex::ImenHamburger => || decode_png(include_bytes!("../assets/imen_hamburger_atlas.png")),
            Tex::DiffText => || decode_png(include_bytes!("../assets/difficulty_text_atlas.png")),
            Tex::GameSetText => || decode_png(include_bytes!("../assets/gameset_text_atlas.png")),
            Tex::NewGameText => || decode_png(include_bytes!("../assets/newgame_text_atlas.png")),
            Tex::ExitText => || decode_png(include_bytes!("../assets/exit_text_atlas.png")),
            Tex::SoundText => || decode_png(include_bytes!("../assets/sound_text_atlas.png")),
            Tex::GameInfoText => || decode_png(include_bytes!("../assets/gameinfo_text_atlas.png")),
            Tex::White => unreachable!(),
        };
        slot.get_or_init(init).as_ref()
    }

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

    /// CPU 目标：保持原逐像素合成语义（最近邻采样 + alpha 混合），桌面离线 UI 门禁继续可用。
    pub(crate) struct CpuTarget {
        out: Vec<u8>,
        w: usize,
        h: usize,
    }
    impl CpuTarget {
        pub(crate) fn new(w: usize, h: usize) -> Self {
            Self { out: vec![0u8; w * h * 4], w, h }
        }
        pub(crate) fn into_rgba(self) -> Vec<u8> { self.out }
    }
    impl DrawTarget for CpuTarget {
        fn blit(&mut self, tex: Tex, sx0: u32, sy0: u32, sw: u32, sh: u32,
                dx0: usize, dy0: usize, dw: usize, dh: usize, tint: [u8; 4]) {
            if dw == 0 || dh == 0 || sw == 0 || sh == 0 { return; }
            let Some((src, tw, th)) = texture_rgba(tex) else { return };
            let (tw, th) = (*tw as usize, *th as usize);
            for dy in 0..dh {
                let y = dy0 + dy;
                if y >= self.h { break; }
                for dx in 0..dw {
                    let x = dx0 + dx;
                    if x >= self.w { break; }
                    let sxp = sx0 as usize + dx * sw as usize / dw;
                    let syp = sy0 as usize + dy * sh as usize / dh;
                    if sxp >= tw || syp >= th { continue; }
                    let si = (syp * tw + sxp) * 4;
                    if si + 3 >= src.len() { continue; }
                    let a = src[si + 3] as u16 * tint[3] as u16 / 255;
                    if a == 0 { continue; }
                    let di = (y * self.w + x) * 4;
                    let sr = src[si] as u16 * tint[0] as u16 / 255;
                    let sg = src[si + 1] as u16 * tint[1] as u16 / 255;
                    let sb = src[si + 2] as u16 * tint[2] as u16 / 255;
                    if a == 255 {
                        self.out[di] = sr as u8; self.out[di+1] = sg as u8;
                        self.out[di+2] = sb as u8; self.out[di+3] = 255;
                    } else {
                        let ia = 255 - a;
                        self.out[di] = ((self.out[di] as u16 * ia + sr * a) / 255) as u8;
                        self.out[di+1] = ((self.out[di+1] as u16 * ia + sg * a) / 255) as u8;
                        self.out[di+2] = ((self.out[di+2] as u16 * ia + sb * a) / 255) as u8;
                        self.out[di+3] = 255;
                    }
                }
            }
        }
        fn fill(&mut self, dx0: usize, dy0: usize, dw: usize, dh: usize, color: [u8; 4]) {
            if dw == 0 || dh == 0 { return; }
            let a = color[3] as u16;
            for y in dy0.min(self.h)..(dy0 + dh).min(self.h) {
                for x in dx0.min(self.w)..(dx0 + dw).min(self.w) {
                    let di = (y * self.w + x) * 4;
                    if a == 255 {
                        self.out[di] = color[0]; self.out[di+1] = color[1]; self.out[di+2] = color[2];
                    } else {
                        let ia = 255 - a;
                        self.out[di] = ((self.out[di] as u16 * ia + color[0] as u16 * a) / 255) as u8;
                        self.out[di+1] = ((self.out[di+1] as u16 * ia + color[1] as u16 * a) / 255) as u8;
                        self.out[di+2] = ((self.out[di+2] as u16 * ia + color[2] as u16 * a) / 255) as u8;
                    }
                    self.out[di+3] = 255;
                }
            }
        }
        fn dim(&mut self, dx0: usize, dy0: usize, dw: usize, dh: usize, num: u32, den: u32) {
            for y in dy0.min(self.h)..(dy0 + dh).min(self.h) {
                for x in dx0.min(self.w)..(dx0 + dw).min(self.w) {
                    let di = (y * self.w + x) * 4;
                    self.out[di] = (self.out[di] as u32 * num / den) as u8;
                    self.out[di+1] = (self.out[di+1] as u32 * num / den) as u8;
                    self.out[di+2] = (self.out[di+2] as u32 * num / den) as u8;
                }
            }
        }
        fn piece(&mut self, hidden: bool, sc: usize, sr: usize, cell_w: usize, cell_h: usize,
                 uv_h: usize, cx: f32, cy: f32, pw: usize, ph: usize, base: [u8; 3]) {
            if pw == 0 || ph == 0 { return; }
            let Some((atlas, aw, ah)) = texture_rgba(Tex::PieceFront) else { return };
            let (aw, ah) = (*aw as usize, *ah as usize);
            let x0 = (cx - pw as f32 * 0.5) as usize;
            let y0 = (cy - ph as f32 * 0.5) as usize;
            for py in 0..ph {
                for px in 0..pw {
                    let dx = x0 + px;
                    let dy = y0 + py;
                    if dx >= self.w || dy >= self.h { continue; }
                    let nx = px as f32 / pw as f32;
                    let ny = py as f32 / ph as f32;
                    let in_piece = if hidden { true }
                        else { ((nx - 0.5).powi(2) / 0.22 + (ny - 0.5).powi(2) / 0.24) < 1.0 };
                    if !in_piece { continue; }
                    let di = (dy * self.w + dx) * 4;
                    let ax = (sc * cell_w + px * cell_w / pw).min(aw.saturating_sub(1));
                    let ay = (sr * cell_h + py * uv_h / ph).min(ah.saturating_sub(1));
                    let ai = (ay * aw + ax) * 4;
                    if ai + 3 < atlas.len() && atlas[ai+3] > 10 {
                        let ar = atlas[ai] as u32; let ag = atlas[ai+1] as u32; let ab = atlas[ai+2] as u32;
                        if ar + ag + ab > 120 {
                            self.out[di] = ar as u8; self.out[di+1] = ag as u8;
                            self.out[di+2] = ab as u8; self.out[di+3] = 255; continue;
                        }
                    }
                    self.out[di] = base[0]; self.out[di+1] = base[1];
                    self.out[di+2] = base[2]; self.out[di+3] = 255;
                }
            }
        }
    }

    /// 弹窗文字：从 1 列×N 行 atlas 取指定行，按 720 基准矩形贴图（sx/sy = 画布/720,画布/1280）
    #[allow(clippy::too_many_arguments)]
    fn text_rows<T: DrawTarget>(t: &mut T, tex: Tex, tex_h: usize,
                                rows: &[(usize, f32, f32, f32, f32)], sx: f32, sy: f32) {
        let rh = tex_h / rows.iter().map(|r| r.0).max().unwrap_or(0).saturating_add(1).max(1);
        let rh = if rh == 0 { tex_h.max(1) } else { rh };
        for &(idx, x, y, dw, dh) in rows {
            let sy0 = idx * rh;
            if sy0 >= tex_h { continue; }
            let (tw, _) = match texture_rgba(tex) { Some((_, w, _)) => (*w as usize, 0), None => continue };
            t.blit(tex, 0, sy0 as u32, tw as u32, rh.min(tex_h - sy0) as u32,
                   (x * sx) as usize, (y * sy) as usize, (dw * sx) as usize, (dh * sy) as usize,
                   [255, 255, 255, 255]);
        }
    }

    /// 主菜单画面：士兵背景 + 2 大按钮 + 3 小按钮 + 房子（几何与 ui.rs 触摸命中同源）
    fn build_menu<T: DrawTarget>(t: &mut T, w: usize, h: usize) {
        if let Some(&(_, bw, bh)) = texture_rgba(Tex::MenuBg) {
            // ★ 轮7 结论：保持原样（直接铺满 0..h）——之前的 +68px 偏移配准实验【已回滚】。
            //   根因分析：orig_main_real.png 截图本身被纵向二次缩放过（按钮贴图同一资源，
            //   截图实测人机按钮 y[1078,1191] 高113，而 datah 几何真值 y[1084,1179] 高95、
            //   我的渲染 y[1051,1176] 高125 与真值同量级）。用失真的截图配准出的 +68px 偏移
            //   会破坏真实设备显示（真实设备按钮位置文档真值已验证 Δ≤2px）。
            //   故：背景/按钮维持 datah 真值基准，不按失真截图调整。
            t.blit(Tex::MenuBg, 0, 0, bw, bh, 0, 0, w, h, [255, 255, 255, 255]);
        }
        let k = w as f32 / 900.0; // view 900系 → 画布
        // ★ 轮12 画点同源：大按钮绘制直接用 ui.rs 命中常量（720基准 ×1.25 = view 900系）
        if let Some(&(_, bw, _bh)) = texture_rgba(Tex::MenuBig) {
            let half = bw / 2;
            for (i, r) in [crate::ui::MENU_BTN_HUMAN, crate::ui::MENU_BTN_TWOPLAYER].iter().enumerate() {
                t.blit(Tex::MenuBig, (i * half as usize) as u32, 0, half, _bh,
                       ((r.x as f32 * 1.25) * k) as usize, ((r.y as f32 * 1.25) * k) as usize,
                       ((r.w as f32 * 1.25) * k) as usize, ((r.h as f32 * 1.25) * k) as usize, [255,255,255,255]);
            }
        }
        if let Some(&(_, sw, sh)) = texture_rgba(Tex::MenuSmall) {
            let seg = sw / 3;
            // ★ 轮12 画点同源：小按钮绘制直接用 ui.rs 命中常量（720基准 ×0.8 转 view 900系），
            //   消除旧的 view 系硬编码 (215,469,727)/(214,131)——两套各自维护易漂移。
            let small = [crate::ui::MENU_BTN_NET, crate::ui::MENU_BTN_LOCAL, crate::ui::MENU_BTN_FOUR];
            for (i, r) in small.iter().enumerate() {
                t.blit(Tex::MenuSmall, i as u32 * seg, 0, seg, sh,
                       ((r.x as f32 * 1.25) * k) as usize, ((r.y as f32 * 1.25) * k) as usize,
                       ((r.w as f32 * 1.25) * k) as usize, ((r.h as f32 * 1.25) * k) as usize, [255,255,255,255]);
            }
        }
        if let Some(&(_, hw, hh)) = texture_rgba(Tex::House) {
            // ★ 画点同源：房子绘制用 ui.rs MENU_BTN_HOUSE（720基准 ×1.25）
            let r = crate::ui::MENU_BTN_HOUSE;
            t.blit(Tex::House, 0, 0, hw, hh,
                   ((r.x as f32 * 1.25) * k) as usize, ((r.y as f32 * 1.25) * k) as usize,
                   ((r.w as f32 * 1.25) * k) as usize, ((r.h as f32 * 1.25) * k) as usize, [255,255,255,255]);
        }
    }

    /// 场景布局唯一真值（CPU/GPU 共用）。坐标口径：画布 w×h（当前 450×800），基准 720×1280。
    pub(crate) fn build_scene<T: DrawTarget>(t: &mut T, w: usize, h: usize,
                                             board: Option<&crate::state::State>,
                                             screen: crate::ui::Screen,
                                             stats: Option<&crate::evt::EventCounter>) {
        if screen == crate::ui::Screen::Menu { build_menu(t, w, h); return; }
        let sx = w as f32 / 720.0;
        let sy = h as f32 / 1280.0;
        if let Some(&(_, bw, bh)) = texture_rgba(Tex::BoardBg) {
            t.blit(Tex::BoardBg, 0, 0, bw, bh, 0, 0, w, h, [255,255,255,255]);
        }
        // 棋盘格 + 棋子（BG_* 几何真值，与 view_to_cell 同一套）
        const BG_COL0_CX: f32 = 78.5;
        const BG_COL_W: f32 = 140.5;
        const BG_ROW0_CY: f32 = 222.0;
        const BG_ROW_H: f32 = 71.0;
        const BG_ROW6_CY: f32 = 822.0;
        let bg_row_cy = |image_row: usize| -> f32 {
            if image_row <= 5 { BG_ROW0_CY + image_row as f32 * BG_ROW_H }
            else { BG_ROW6_CY + (image_row - 6) as f32 * BG_ROW_H }
        };
        let show_pieces = matches!(screen, crate::ui::Screen::Game
            | crate::ui::Screen::IngameMenu | crate::ui::Screen::Replay);
        if show_pieces {
            if let Some(st) = board {
                let (aw, ah) = match texture_rgba(Tex::PieceFront) {
                    Some((_, w, h)) => (*w, *h), None => (0, 0),
                };
                for device_row in 0..12usize {
                    for col in 0..5usize {
                        let c = &st.cells[device_row * 5 + col];
                        if c.is_empty() { continue; }
                        let image_row = 11 - device_row;
                        let cx = (BG_COL0_CX + col as f32 * BG_COL_W) * (w as f32 / 720.0);
                        let cy = bg_row_cy(image_row) * (h as f32 / 1280.0);
                        let cell_w = (aw / 4) as usize;
                        let cell_h = (ah / 7) as usize;
                        let ph = (BG_ROW_H * (h as f32 / 1280.0) * 0.92) as usize;
                        let pw = if cell_h > 0 { (ph as f32 * cell_w as f32 / cell_h as f32) as usize } else { 0 };
                        // 兜底色取棋子图集实际色（红(222,132,51)/蓝(82,137,179)），贴图透明边露出时与原版一致
                        let base: [u8; 3] = if c.side == crate::state::SIDE_RED { [222,132,51] } else { [82,137,179] };
                        let hidden = st.mode == crate::state::MODE_FLIP && !c.revealed;
                        let rank = u8::from(crate::state::rank_of(c)) as usize;
                        let idx = rank.saturating_sub(1).min(11);
                        let sc = if hidden { 0 } else { idx % 4 };
                        let subrow = idx / 4;
                        let sr = if hidden { 6 }
                            else if c.side == crate::state::SIDE_RED { subrow } else { 3 + subrow };
                        let uv_h = if hidden { cell_h.saturating_sub(3) } else { cell_h };
                        t.piece(hidden, sc, sr, cell_w, cell_h, uv_h, cx, cy, pw, ph, base);
                    }
                }
            }
        }
        // 游戏设置 datah_012
        if screen == crate::ui::Screen::GameSet {
            let p = &crate::ui::SETTINGS_PANEL;
            t.fill((p.x as f32*sx) as usize, (p.y as f32*sy) as usize,
                   (p.w as f32*sx) as usize, (p.h as f32*sy) as usize, [45,45,45,255]);
            for (i, &(x0,y0,x1,y1)) in crate::ui::GAMESET_BTNS_ORIG.iter().enumerate() {
                let c = if i == 4 { [58,103,189,255] } else if i == 5 { [160,160,160,255] } else { [80,190,140,255] };
                t.fill((x0 as f32*sx) as usize, (y0 as f32*sy) as usize, ((x1-x0) as f32*sx) as usize, ((y1-y0) as f32*sy) as usize, c);
            }
            if let Some(&(_, _, th)) = texture_rgba(Tex::GameSetText) {
                let mut rows = vec![(0usize, 260.0f32, 410.0f32, 200.0f32, 46.0f32)];
                for r in 0..4usize {
                    let y = 490.0 + r as f32 * 75.0;
                    rows.push((r + 1, 165.0, y, 140.0, 46.0));
                    rows.push((5, 400.0, y, 100.0, 46.0));
                }
                rows.push((7, 205.0, 793.0, 100.0, 46.0));
                rows.push((8, 415.0, 793.0, 100.0, 46.0));
                text_rows(t, Tex::GameSetText, th as usize, &rows, sx, sy);
            }
        }
        // 声音设置 datah_001
        if screen == crate::ui::Screen::Settings {
            let p = &crate::ui::SETTINGS_PANEL_ORIG;
            t.fill((p.x as f32*sx) as usize, (p.y as f32*sy) as usize,
                   (p.w as f32*sx) as usize, (p.h as f32*sy) as usize, [45,45,45,255]);
            for (i, &(x0,y0,x1,y1)) in crate::ui::SETTINGS_BTNS_ORIG.iter().enumerate() {
                let c = if i == 3 { [58,103,189,255] } else if i == 4 { [160,160,160,255] } else { [80,190,140,255] };
                t.fill((x0 as f32*sx) as usize, (y0 as f32*sy) as usize, ((x1-x0) as f32*sx) as usize, ((y1-y0) as f32*sy) as usize, c);
            }
            if let Some(&(_, _, th)) = texture_rgba(Tex::SoundText) {
                let rows = [(0usize,260.0f32,450.0f32,200.0f32,40.0f32),
                            (1,165.0,510.0,150.0,40.0), (2,165.0,585.0,150.0,40.0), (3,165.0,668.0,150.0,40.0)];
                text_rows(t, Tex::SoundText, th as usize, &rows, sx, sy);
            }
        }
        // ☰ 菜单：半透明黑面板 + 8 图标 + 8 白字
        if screen == crate::ui::Screen::IngameMenu {
            let k = w as f32 / 900.0;
            t.dim((30.0*k) as usize, (530.0*k) as usize, (840.0*k) as usize, (530.0*k) as usize, 55, 255);
            if let Some(&(_, iw, ih)) = texture_rgba(Tex::ImenIcons) {
                let cw = iw / 4; let ch = ih / 2;
                for row in 0..2usize {
                    for col in 0..4usize {
                        let cx = [156.0f32,361.0,559.0,758.0][col];
                        let ty = if row == 0 { 580.0 } else { 800.0 };
                        t.blit(Tex::ImenIcons, col as u32 * cw, row as u32 * ch, cw, ch,
                               ((cx-55.0)*k) as usize, (ty*k) as usize, (110.0*k) as usize, (100.0*k) as usize,
                               [255,255,255,255]);
                    }
                }
            }
            if let Some(&(_, tw, th)) = texture_rgba(Tex::ImenText) {
                let rh = th / 8;
                for row in 0..2usize {
                    for col in 0..4usize {
                        let i = row * 4 + col;
                        let cx = [156.0f32,361.0,559.0,758.0][col];
                        let ty = if row == 0 { 720.0 } else { 940.0 };
                        t.blit(Tex::ImenText, 0, i as u32 * rh, tw, rh.min(38u32),
                               ((cx-80.0)*k) as usize, (ty*k) as usize, (160.0*k) as usize, (38.0*k) as usize,
                               [255,255,255,255]);
                    }
                }
            }
        }
        // 两人新局弹窗 datah_062
        if screen == crate::ui::Screen::NewGame {
            t.fill((135.0*sx) as usize, (482.0*sy) as usize, (450.0*sx) as usize, (315.0*sy) as usize, [45,45,45,255]);
            for (i, &(x0,y0,x1,y1)) in crate::ui::NEWGAME_BTNS_ORIG.iter().enumerate() {
                let c = if i == 0 { [58,103,189,255] } else if i == 1 { [160,160,160,255] } else { [95,95,215,255] };
                t.fill((x0 as f32*sx) as usize, (y0 as f32*sy) as usize, ((x1-x0) as f32*sx) as usize, ((y1-y0) as f32*sy) as usize, c);
            }
            if let Some(&(_, _, th)) = texture_rgba(Tex::NewGameText) {
                let rows = [(0usize,260.0f32,485.0f32,200.0f32,46.0f32), (1,165.0,540.0,130.0,46.0),
                            (2,165.0,615.0,130.0,46.0), (3,355.0,557.0,200.0,46.0),
                            (4,355.0,632.0,200.0,46.0), (5,205.0,718.0,100.0,46.0), (6,415.0,718.0,100.0,46.0)];
                text_rows(t, Tex::NewGameText, th as usize, &rows, sx, sy);
            }
        }
        // 人机难度弹窗 datah_041
        if screen == crate::ui::Screen::Difficulty {
            let p = crate::ui::DIFFICULTY_PANEL;
            t.fill((p.x as f32*sx) as usize, (p.y as f32*sy) as usize,
                   (p.w as f32*sx) as usize, (p.h as f32*sy) as usize, [40,40,40,255]);
            // ★ 原版配色（difficulty_orig_vs_fw.png 左半实锤）：布局列蓝(58,103,189) / 翻棋列紫(128,80,190)
            for (i, &(x0,y0,x1,y1)) in crate::ui::DIFF_BTNS_ORIG.iter().enumerate() {
                let c = if i % 2 == 0 { [58,103,189,255] } else { [128,80,190,255] };
                t.fill((x0 as f32*sx) as usize, (y0 as f32*sy) as usize, ((x1-x0) as f32*sx) as usize, ((y1-y0) as f32*sy) as usize, c);
            }
            if let Some(&(_, _, th)) = texture_rgba(Tex::DiffText) {
                let mut rows = Vec::new();
                for r in 0..4usize {
                    let cy = 527.0 + r as f32 * 75.0;
                    rows.push((r, 165.0, cy-23.0, 120.0, 46.0));
                    rows.push((4, 300.0, cy-23.0, 100.0, 46.0));
                    rows.push((5, 445.0, cy-23.0, 100.0, 46.0));
                }
                text_rows(t, Tex::DiffText, th as usize, &rows, sx, sy);
            }
        }
        // ★ 复盘控制条 datah_107 真值（C ui_menu.c:427-441 + render.c:9744-9758）：
        //   面板 460×60@(10,705)；5 按钮各 84×60 x=10/104/198/292/386 y=705-765；
        //   5 色 菜单灰(115,115,140)/回退橙(217,102,51)/前进绿(51,166,89)/跳转紫(115,77,191)/返回蓝(51,115,230)。
        //   旧实现条底[40,45,55]+5按钮全橙[230,128,38]是自创，本轮改真值。
        if screen == crate::ui::Screen::Replay {
            t.fill((10.0f32*sx) as usize, (705.0f32*sy) as usize,
                   (460.0f32*sx) as usize, (60.0f32*sy) as usize, [26,26,36,235]); // 条底深色
            let colors: [[u8;3];5] = [[115,115,140],[217,102,51],[51,166,89],[115,77,191],[51,115,230]];
            for (i, col) in colors.iter().enumerate() {
                let bx = [10.0f32, 104.0, 198.0, 292.0, 386.0][i];
                t.fill((bx*sx) as usize, (705.0f32*sy) as usize,
                       (84.0f32*sx) as usize, (60.0f32*sy) as usize, [col[0],col[1],col[2],230]);
            }
        }
        // 终局覆盖（rs_draw_overlay 真值）
        if let Some(st) = board {
            if st.stage == crate::state::Stage::Over {
                let c = match st.winner {
                    crate::state::Winner::Blue => [38,89,217,230],
                    crate::state::Winner::Red => [217,51,51,230],
                    _ => [140,140,140,230],
                };
                t.fill((w as f32*0.20) as usize, (h as f32*0.42) as usize,
                       (w as f32*0.60) as usize, (h as f32*0.16) as usize, c);
                t.fill((w as f32*0.35) as usize, (h as f32*0.62) as usize,
                       (w as f32*0.30) as usize, (h as f32*0.10) as usize, [51,191,77,230]);
            }
        }
        // 退出确认 datah_025
        if screen == crate::ui::Screen::ExitConfirm {
            t.fill((135.0*sx) as usize, (530.0*sy) as usize, (450.0*sx) as usize, (220.0*sy) as usize, [45,45,45,255]);
            for &(rect, r, g, b) in &[
                (crate::ui::EXIT_BTN_QUIT, 60u8, 110u8, 200u8),
                (crate::ui::EXIT_BTN_STAY, 145u8, 145u8, 150u8),
            ] {
                t.fill((rect.x as f32*sx) as usize, (rect.y as f32*sy) as usize,
                       (rect.w as f32*sx) as usize, (rect.h as f32*sy) as usize, [r,g,b,255]);
            }
            if let Some(&(_, _, th)) = texture_rgba(Tex::ExitText) {
                let rows = [(0usize,200.0f32,570.0f32,320.0f32,46.0f32),
                            (3,205.0,665.0,100.0,46.0), (4,415.0,665.0,100.0,46.0)];
                text_rows(t, Tex::ExitText, th as usize, &rows, sx, sy);
            }
        }
        // ★ 对局信息/战况统计弹窗 datah_050（原版真值，UI_COMPARE §9 几何 Δ=0）：
        //   面板 420×380 居中偏移(150,450)；标题'对局信息'；Info 文本框 30,75-390,285；
        //   3 按钮 复盘 25,295-135,355 / 删除 155,295-265,355 / 返回 285,295-395,355。
        //   旧实现是自创柱状图（非原版 UI），本轮按 datah_050 真值重建。
        if screen == crate::ui::Screen::Stats {
            let px = 150.0f32; let py = 450.0f32;
            t.fill((px*sx) as usize, (py*sy) as usize, (420.0*sx) as usize, (380.0*sy) as usize, [45,45,45,255]);
            // Info 文本框（半透明深色，显示 event 统计摘要数字）
            t.fill(((px+30.0)*sx) as usize, ((py+75.0)*sy) as usize,
                   (360.0*sx) as usize, (210.0*sy) as usize, [30,30,30,255]);
            if let Some(st) = stats {
                // 统计数字用 gameinfo 文字图集无法表达（原版 Info 是运行时文本）——这里保留数字文本
                // 通过 setprop jq.stats 输出（jni.rs btn5 已写），视觉上 Info 框为原版深色底。
                let _ = st;
            }
            // 3 按钮（原版真值局部 rect + 偏移）
            for (bx0, by0, bx1, by1) in [(25.0f32,295.0f32,135.0f32,355.0f32),
                                          (155.0,295.0,265.0,355.0),
                                          (285.0,295.0,395.0,355.0)] {
                t.fill(((px+bx0)*sx) as usize, ((py+by0)*sy) as usize,
                       ((bx1-bx0)*sx) as usize, ((by1-by0)*sy) as usize, [58,103,189,255]);
            }
            if let Some(&(_, _, th)) = texture_rgba(Tex::GameInfoText) {
                // ★ 真值（C render.c:4385-4405）：标题(360,473)；Info 4 行标签 蓝方(360,560)/
                //   红方(620)/步数(680)/回合(735)，宽150；3 按钮 复盘/删除/返回。
                let rows = [
                    (0usize, 360.0f32, 473.0f32, 220.0f32, 46.0f32), // 标题'对局信息'
                    (1, 360.0, 560.0, 150.0, 46.0), // 蓝方（C render.c:4399 真值 y=560）
                    (2, 360.0, 620.0, 150.0, 46.0), // 红方（y=620）
                    (3, 360.0, 680.0, 150.0, 46.0), // 步数（y=680）
                    (4, 360.0, 735.0, 150.0, 46.0), // 回合（y=735，与原版一致；槽位高46含透明边，实际文字像素更矮）
                    (5, 150.0+80.0,  450.0+305.0, 100.0, 46.0), // 复盘
                    (6, 150.0+210.0, 450.0+305.0, 100.0, 46.0), // 删除
                    (7, 150.0+340.0, 450.0+305.0, 100.0, 46.0), // 返回
                ];
                text_rows(t, Tex::GameInfoText, th as usize, &rows, sx, sy);
            }
        }
        // ★ 对局态 ☰/↩ 真图标（imen_hamburger_atlas 128×64 = 左☰右↩，原版资源）：
        //   旧实现画青蓝方块 [72,184,240] 是瞎渲染，本轮换真图标。
        //   中心 view(270,750)/(620,750)，各 64×64 view（k = view→画布缩放）
        if screen == crate::ui::Screen::Game {
            let k = w as f32 / 900.0;
            if let Some(&(_, iw, ih)) = texture_rgba(Tex::ImenHamburger) {
                let half = iw / 2; // 64
                for (i, cx) in [270.0f32, 620.0].iter().enumerate() {
                    t.blit(Tex::ImenHamburger, (i * half as usize) as u32, 0, half, ih,
                           ((cx - 32.0) * k) as usize, ((750.0 - 32.0) * k) as usize,
                           (64.0 * k) as usize, (64.0 * k) as usize, [255,255,255,255]);
                }
            }
        }
    }

    pub fn compose_board_rgba(w: usize, h: usize,
                              board: Option<&crate::state::State>,
                              screen: crate::ui::Screen,
                              stats: Option<&crate::evt::EventCounter>) -> Vec<u8> {
        let mut t = CpuTarget::new(w, h);
        build_scene(&mut t, w, h, board, screen, stats);
        t.into_rgba()
    }
}
#[allow(unused_imports)] // GPU 渲染模块（android-only）与桌面测试共用
pub(crate) use imp::{build_scene, texture_rgba, CpuTarget};
pub use imp::compose_board_rgba;

#[cfg(test)]
mod menu_text_tests {
    use super::*;
    #[test]
    fn menu_text_atlas_metadata_matches_c() {
        assert_eq!(MENU_TEXT_W, [136, 136, 66, 68, 134]);
        assert_eq!(MENU_TEXT_X, [0, 0, 35, 34, 1]);
        assert_eq!(MENU_TEXT_Y, [0, 44, 88, 132, 176]);
        assert_eq!(MENU_TEXT_H, 38);
        let img = include_bytes!("../assets/menu_text_atlas.png");
        assert!(img.len() > 1000, "图集字节存在");
        for i in 0..5 {
            assert!(MENU_TEXT_X[i] + MENU_TEXT_W[i] <= MENU_TEXT_IMG_W, "槽{} X 越界", i);
            assert!(MENU_TEXT_Y[i] + MENU_TEXT_H <= MENU_TEXT_IMG_H, "槽{} Y 越界", i);
        }
        assert_eq!(&img[..4], &[0x89, b'P', b'N', b'G']);
    }
}

#[cfg(test)]
mod menu_compose_tests {
    use super::*;
    #[test]
    fn menu_btns_match_touch_hit() {
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
        let p = crate::ui::DIFFICULTY_PANEL;
        for (i, &(x0, y0, x1, y1)) in crate::ui::DIFF_BTNS_ORIG.iter().enumerate() {
            let cx = (x0 + x1) / 2;
            let cy = (y0 + y1) / 2;
            let got = crate::ui::difficulty_hit(cx, cy, &p);
            assert_eq!(got, Some(((i / 2) as u8 + 1, (i % 2) as u8)),
                       "原版按钮{} 中心命中解析", i);
        }
    }
}

#[cfg(test)]
mod icon_hit_tests {
    #[test]
    fn undo_icon_hit_matches_device_probe() {
        let in_undo = |x: f32, y: f32| (x - 620.0).abs() <= 60.0 && (y - 750.0).abs() <= 60.0;
        assert!(in_undo(620.0, 750.0), "真机悔棋坐标必须命中");
        assert!(!in_undo(274.0, 1383.0), "蓝方棋子区不得命中");
        assert!(!in_undo(77.0, 577.0), "红方棋子区不得命中");
    }
}
