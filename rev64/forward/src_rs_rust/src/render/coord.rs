//! 触摸坐标纯几何（从 render.rs 拆分，逐字搬运）
pub mod gles_render_test {
    /// view 坐标 → 720×1280 基准坐标（等比缩放 + yoff 居中，对齐 C OnTouch g_touch_s/yoff）
    pub fn touch_base_coord(vx: i32, vy: i32, view_w: i32, view_h: i32) -> (f32, f32) {
        let sx = view_w.max(1) as f32 / 720.0;
        let sy = view_h.max(1) as f32 / 1280.0;
        let s = sx.min(sy).max(1e-6);
        let yoff = (view_h as f32 - 1280.0 * s) * 0.5;
        let xoff = (view_w as f32 - 720.0 * s) * 0.5;
        let bx = vx as f32 / s - xoff.max(0.0);
        let by = vy as f32 / s - yoff.max(0.0);
        (bx, by)
    }
}
