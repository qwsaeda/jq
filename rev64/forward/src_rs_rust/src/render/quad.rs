//! quad — 平台无关的场景显示列表（DrawTarget → GPU quad 批次）
//! 与 CPU 像素合成共用 asset::build_scene（布局单一真值，CODE_STYLE §13）。
use crate::asset::{self, Tex};

/// 一个纹理矩形（dst 为画布像素坐标，src 为纹理像素坐标）
#[derive(Clone, Debug)]
pub(crate) struct Quad {
    pub(crate) tex: Tex,
    pub(crate) sx0: u32,
    pub(crate) sy0: u32,
    pub(crate) sw: u32,
    pub(crate) sh: u32,
    pub(crate) dx0: f32,
    pub(crate) dy0: f32,
    pub(crate) dw: f32,
    pub(crate) dh: f32,
    /// 线性 tint（RGB 乘法 + alpha），纯色 quad 用 White 纹理承载
    pub(crate) tint: [f32; 4],
}

/// 收集 asset::build_scene 发出的所有绘制指令，供 GPU 一次性上传/绘制
#[derive(Default)]
pub(crate) struct QuadTarget {
    pub(crate) quads: Vec<Quad>,
}

/// 同一纹理且连续的 quad 合并为一次 draw（减少 draw call，保持图层顺序）
/// 返回 (纹理, quad 起始索引, quad 数量)——GPU 侧据此算顶点 buffer slice 偏移
pub(crate) fn quad_batches(quads: &[Quad]) -> Vec<(Tex, usize, usize)> {
    let mut out: Vec<(Tex, usize, usize)> = Vec::new();
    let mut i = 0;
    while i < quads.len() {
        let tex = quads[i].tex;
        let mut j = i + 1;
        while j < quads.len() && quads[j].tex == tex { j += 1; }
        out.push((tex, i, j - i));
        i = j;
    }
    out
}

impl asset::DrawTarget for QuadTarget {
    fn blit(&mut self, tex: Tex, sx0: u32, sy0: u32, sw: u32, sh: u32,
            dx0: usize, dy0: usize, dw: usize, dh: usize, tint: [u8; 4]) {
        if dw == 0 || dh == 0 || sw == 0 || sh == 0 { return; }
        self.quads.push(Quad {
            tex, sx0, sy0, sw, sh,
            dx0: dx0 as f32, dy0: dy0 as f32, dw: dw as f32, dh: dh as f32,
            tint: [tint[0] as f32 / 255.0, tint[1] as f32 / 255.0,
                   tint[2] as f32 / 255.0, tint[3] as f32 / 255.0],
        });
    }
    fn fill(&mut self, dx0: usize, dy0: usize, dw: usize, dh: usize, color: [u8; 4]) {
        if dw == 0 || dh == 0 { return; }
        self.quads.push(Quad {
            tex: Tex::White, sx0: 0, sy0: 0, sw: 1, sh: 1,
            dx0: dx0 as f32, dy0: dy0 as f32, dw: dw as f32, dh: dh as f32,
            tint: [color[0] as f32/255.0, color[1] as f32/255.0,
                   color[2] as f32/255.0, color[3] as f32/255.0],
        });
    }
    fn dim(&mut self, dx0: usize, dy0: usize, dw: usize, dh: usize, num: u32, den: u32) {
        // CPU 行为 = RGB × num/den；等价 GPU 黑半透明覆盖 alpha = 1-num/den
        let alpha = 1.0 - (num as f32 / den.max(1) as f32);
        self.quads.push(Quad {
            tex: Tex::White, sx0: 0, sy0: 0, sw: 1, sh: 1,
            dx0: dx0 as f32, dy0: dy0 as f32, dw: dw as f32, dh: dh as f32,
            tint: [0.0, 0.0, 0.0, alpha],
        });
    }
    fn piece(&mut self, hidden: bool, sc: usize, sr: usize, cell_w: usize, cell_h: usize,
             _uv_h: usize, cx: f32, cy: f32, pw: usize, ph: usize, _base: [u8; 3]) {
        if pw == 0 || ph == 0 || cell_w == 0 || cell_h == 0 { return; }
        self.quads.push(Quad {
            tex: if hidden { Tex::PieceBack } else { Tex::PieceFront },
            sx0: sc as u32 * cell_w as u32,
            sy0: sr as u32 * cell_h as u32,
            sw: cell_w as u32,
            sh: cell_h as u32,
            dx0: cx - pw as f32 * 0.5, dy0: cy - ph as f32 * 0.5,
            dw: pw as f32, dh: ph as f32,
            tint: [1.0, 1.0, 1.0, 1.0],
        });
    }
}

/// 生成场景 quad 列表（布局唯一真值在 asset::build_scene）
pub(crate) fn scene_quads(w: usize, h: usize,
                   board: Option<&crate::state::State>,
                   screen: crate::ui::Screen,
                   stats: Option<&crate::evt::EventCounter>) -> Vec<Quad> {
    let mut t = QuadTarget::default();
    asset::build_scene(&mut t, w, h, board, screen, stats);
    t.quads
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn menu_scene_has_background_and_buttons() {
        let q = scene_quads(450, 800, None, crate::ui::Screen::Menu, None);
        assert!(q.len() >= 6, "菜单应含背景+2大按钮+3小按钮+房子，实际 {}", q.len());
        assert_eq!(q[0].tex, Tex::MenuBg);
        assert!(q.iter().any(|x| x.tex == Tex::MenuBig));
        assert!(q.iter().any(|x| x.tex == Tex::House));
        for x in &q {
            assert!(x.dx0 >= -1.0 && x.dy0 >= -1.0, "quad 不得负坐标");
            assert!(x.dw > 0.0 && x.dh > 0.0, "quad 尺寸必须正");
        }
    }
    #[test]
    fn game_scene_has_board_background() {
        let q = scene_quads(450, 800, None, crate::ui::Screen::Game, None);
        assert!(!q.is_empty());
        assert_eq!(q[0].tex, Tex::BoardBg, "第一层必须是棋盘背景");
    }
    #[test]
    fn game_scene_emits_one_quad_per_piece() {
        let layout = [3u8,7,6,5,8, 10,0,9,0,1, 2,10,0,9,5,
                      4,0,6,0,7, 8,3,4,9,11, 11,12,11,8,7];
        let st = crate::gametest::init_from_layout_30(&layout, crate::state::MODE_LAY);
        let n = st.cells.iter().filter(|c| !c.is_empty()).count();
        assert!(n > 0, "开局必须有子");
        let q = scene_quads(450, 800, Some(&st), crate::ui::Screen::Game, None);
        let pieces = q.iter().filter(|x| x.tex == Tex::PieceFront || x.tex == Tex::PieceBack).count();
        assert_eq!(pieces, n, "棋子 quad 数必须等于棋盘非空子数");
    }
    #[test]
    fn gpu_prewarm_covers_all_tex_variants() {
        // ★ 防静默降级（CODE_STYLE §9）：asset::Tex 每加一个变体，gpu.rs 的 ALL_TEX 必须同步预热，
        //   否则该纹理的 quad 在 build_vertices 取不到尺寸 → GPU 帧失败 → 静默回落 CPU。
        let asset = include_str!("../asset.rs");
        let gpu = include_str!("gpu.rs");
        let enum_body = asset
            .split("pub(crate) enum Tex {")
            .nth(1)
            .and_then(|s| s.split('}').next())
            .expect("Tex 枚举存在");
        let variants: Vec<&str> = enum_body
            .lines()
            .map(|l| l.split("//").next().unwrap_or("").trim().trim_end_matches(',').trim())
            .filter(|n| !n.is_empty() && n.chars().next().map(|c| c.is_ascii_uppercase()).unwrap_or(false))
            .collect();
        assert!(variants.len() >= 15, "Tex 变体数异常: {}", variants.len());
        let all_tex = gpu.split("const ALL_TEX: &[Tex] = &[").nth(1)
            .and_then(|s| s.split("];").next()).expect("ALL_TEX 存在");
        for v in variants {
            assert!(all_tex.contains(&format!("Tex::{}", v)),
                    "纹理 {} 未加入 gpu.rs ALL_TEX（会导致该屏态静默降级 CPU）", v);
        }
    }

    #[test]
    fn dim_is_black_alpha_overlay() {
        let layout = [3u8,7,6,5,8, 10,0,9,0,1, 2,10,0,9,5,
                      4,0,6,0,7, 8,3,4,9,11, 11,12,11,8,7];
        let st = crate::gametest::init_from_layout_30(&layout, crate::state::MODE_LAY);
        let q = scene_quads(450, 800, Some(&st), crate::ui::Screen::IngameMenu, None);
        let d = q.iter().find(|x| x.tint[0] == 0.0 && x.tint[3] > 0.0 && x.tint[3] < 1.0);
        assert!(d.is_some(), "☰ 菜单必须有半透明黑遮罩（对应 CPU dim）");
    }
}
