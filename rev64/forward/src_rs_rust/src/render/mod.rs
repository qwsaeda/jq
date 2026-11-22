#![allow(dead_code)]
//! render — wgpu 渲染层（规格 §12 出屏，对齐 WGPU_RENDER_DESIGN.md）
//!
//! 仅 android target 编译（wgpu 依赖仅 android）。桌面不编译（纯逻辑层 41 测试不受影响）。
//!
//! ★ 诚实边界（设计 §7）：wgpu 出屏**必须真机验收**（设备 10.0.182.146，PENDING）。
//! 本模块编译期保证**类型可解析 + 14 JNI 符号可链接**（交付物①）；
//! 完整 render pass（EGL surface 创建/纹理上传/顶点绘制）真机可达时补全（设计 §3/§6 5 判据）。
//! wgpu 30 的 request_adapter/request_device 是 async（返回 Future），真机走 pollster 轮询，
//! 此处用惰性 Option 骨架保编译，不实际执行 wgpu 调用（避免 cross-compile 无 GPU 环境）。

mod gles;
mod geom;
mod cpu;
mod coord;
pub(crate) mod quad;
#[cfg(target_os = "android")]
pub(crate) mod gpu;

// 对外 API 重组（与原 render.rs 完全一致）
#[cfg(not(target_os = "android"))]
pub fn _render_desktop_noop() {}

#[cfg(target_os = "android")]
pub use gles::gles_render::{view_to_cell, RenderCtx};
// 平台无关触摸基准坐标（供桌面测试/逻辑复用；生产 view_to_cell 内部也用同一真值）
pub use coord::touch_base_coord;
#[cfg(test)]
mod tests {
    // 坐标映射是纯几何（view 坐标 → cell），平台无关。
    // 把核心几何函数从 gles_render 提为平台无关（不依赖 wgpu 类型），本机可测。
    // 此处测试 touch_base_coord 的等比缩放 + yoff 逻辑（手工验证 720×1280 view）
    #[test]
    fn base_coord_identity_720() {
        // view=720×1280 时，缩放 s=1.0，yoff=0 → base 坐标 = view 坐标
        use crate::render::coord::touch_base_coord;
        let (bx, by) = touch_base_coord(360, 640, 720, 1280);
        assert!((bx - 360.0).abs() < 1.0, "720×1280 基准 x 不变");
        assert!((by - 640.0).abs() < 1.0, "720×1280 基准 y 不变");
    }
}

// ★ 2026-11-22 审计：删除 deprecated 历史几何函数的死 re-export（UI-09 已标注
//   "未引用，生产在 asset.rs/ui.rs"）——此前它触发 android target 编译 2 个 warning，
//   使 rust_all.sh [3/4] 零警告门禁 FAIL。全仓 grep 确认除本行外无任何引用。
//   （原 `#[cfg(target_os="android")] pub use geom::geom::{draw_rects_rgba,menu_rects};` 已删）
