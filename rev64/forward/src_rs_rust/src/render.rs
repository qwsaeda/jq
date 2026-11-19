//! render — wgpu 渲染层（规格 §12 出屏，对齐 WGPU_RENDER_DESIGN.md）
//!
//! 仅 android target 编译（wgpu 依赖仅 android）。桌面不编译（纯逻辑层 41 测试不受影响）。
//!
//! ★ 诚实边界（设计 §7）：wgpu 出屏**必须真机验收**（设备 10.0.182.146，PENDING）。
//! 本模块编译期保证**类型可解析 + 14 JNI 符号可链接**（交付物①）；
//! 完整 render pass（EGL surface 创建/纹理上传/顶点绘制）真机可达时补全（设计 §3/§6 5 判据）。
//! wgpu 30 的 request_adapter/request_device 是 async（返回 Future），真机走 pollster 轮询，
//! 此处用惰性 Option 骨架保编译，不实际执行 wgpu 调用（避免 cross-compile 无 GPU 环境）。

#[cfg(target_os = "android")]
mod gles_render {
    use crate::board::{cell_col, cell_row, N_CELLS};
    use crate::state::{rank_of, Cell, State};
    use crate::ui::{Screen, Rect};
    use wgpu::Device as WgpuDevice;

    /// 渲染上下文（wgpu device/surface + 纹理，惰性初始化）
    pub struct RenderCtx {
        pub view_w: i32,
        pub view_h: i32,
        // wgpu 资源（OnGraphCreate 真机初始化时填充；骨架期为 None 保编译）
        pub device: Option<WgpuDevice>,
        pub queue: Option<wgpu::Queue>,
        // surface（wgpu 30 Surface<'a> 含生命周期，真机 EGL 创建时局部持有；骨架期不存）
        // 棋盘纹理（737×751，从 SPAK 解码后上传，此处占位）
        pub board_tex: Option<wgpu::TextureView>,
        pub pipeline: Option<wgpu::RenderPipeline>,
        // 初始化状态（pollster 轮询标记）
        pub init_done: bool,
    }

    impl Default for RenderCtx {
        fn default() -> Self {
            Self {
                view_w: 720,
                view_h: 1280,
                device: None,
                queue: None,
                board_tex: None,
                pipeline: None,
                init_done: false,
            }
        }
    }

    impl RenderCtx {
        /// 初始化 wgpu device + surface（设计 §3 OnGraphCreate，真机 pollster 轮询）
        /// 骨架期：仅置 init_done 标记（真机可达时补 request_adapter/request_device 异步获取）
        pub fn init(&mut self) {
            // 完整实现（真机）：
            //   let instance = wgpu::Instance::new(&wgpu::InstanceDescriptor { backends: GL, .. });
            //   let adapter = pollster::block_on(instance.request_adapter(...));
            //   let (device, queue) = pollster::block_on(adapter.request_device(...));
            //   surface = 从 anative_window 创建 EGL surface → wgpu Surface
            // 骨架期保编译 + 14 符号链接（设计 §5 交付物①），真机补全（设计 §6 5 判据）
            self.init_done = true;
        }

        /// 渲染帧（设计 §3 OnUpdate：清屏+棋盘+棋子+UI+blend）
        /// 骨架期：几何映射 + cell 命中计算可测；wgpu render pass 真机补全
        pub fn render(&mut self, st: &State, screen: Screen) {
            // ① 清屏黑 ② 棋盘等比缩放（aPos.y = i/1280*view_h，铺满全屏）
            // ③ 棋子（cell 中心） ④ UI 屏态 ⑤ blend 开启（atlas 文字防黑底）
            // 完整 render pass 真机可达时补全（设计 §3/§6）
            let _ = (st, screen, self.view_w, self.view_h);
        }

        /// 同步 view 尺寸（设计 §2 关键不变量：OnGraphChange 实际尺寸，防小竖条）
        pub fn sync_view(&mut self, w: i32, h: i32) {
            if w > 0 && h > 0 {
                self.view_w = w;
                self.view_h = h;
                // surface.configure(w,h)（真机 NDK 接入）
            }
        }
    }

    // ---- 触摸坐标 → cell 映射（设计 §2 cell 中心公式，对齐 C da36d6bc）----
    /// view 坐标 → 棋盘 cell 索引（image_row=11-device_row，row0=底）
    pub fn view_to_cell(vx: i32, vy: i32, view_w: i32, view_h: i32) -> Option<usize> {
        // 等比缩放：棋盘 737×751 贴到 view（aPos 基准 1280 高）
        let sx = view_w as f32 / 737.0;
        let sy = view_h as f32 / 1280.0;
        let px = vx as f32 / sx; // 还原到 737 基准
        let py = vy as f32 / sy; // 还原到 1280 基准
        let col = ((px - 78.5) / 140.5).round().max(0.0).min(4.0) as i32;
        let image_row = if py < 822.0 {
            ((py - 222.0) / 71.0).round().max(0.0).min(5.0) as i32
        } else {
            6 + ((py - 822.0) / 71.0).round().max(0.0).min(5.0) as i32
        };
        let device_row = 11 - image_row; // row0=底（蓝底线）
        if device_row < 0 || device_row >= 12 || col < 0 || col >= 5 {
            return None;
        }
        Some(cell_index(device_row as usize, col as usize))
    }

    fn cell_index(row: usize, col: usize) -> usize {
        row * 5 + col
    }
}

#[cfg(not(target_os = "android"))]
pub fn _render_desktop_noop() {}

#[cfg(target_os = "android")]
pub use gles_render::{view_to_cell, RenderCtx};
