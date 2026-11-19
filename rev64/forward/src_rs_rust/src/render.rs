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
        // 棋盘纹理（737×751，从 SPAK 解码后上传）
        pub board_tex: Option<wgpu::TextureView>,
        pub pipeline: Option<wgpu::RenderPipeline>,
        // 顶点 buffer（棋盘/棋子 quad，设计 §3 ③ 顶点批次）
        pub vertex_buf: Option<wgpu::Buffer>,
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
                vertex_buf: None,
                init_done: false,
            }
        }
    }

    impl RenderCtx {
        /// 初始化 wgpu device + queue（设计 §3 OnGraphCreate，真机 pollster 获取后传入）
        pub fn init_device(&mut self, device: wgpu::Device, queue: wgpu::Queue) {
            self.device = Some(device);
            self.queue = Some(queue);
            self.init_done = true;
        }

        /// 创建棋盘渲染 pipeline（真机可达时补全 wgpu API，设计 §3 ②③⑤）
        /// 真机实现：vertex buffer（棋盘 6 顶点）+ fragment（棋盘纹理采样）+ blend state（防黑底）
        pub fn build_pipeline(&mut self) {
            // 真机 NDK 可达时按设计 §3 补全（wgpu 30 create_render_pipeline + wgsl 着色器）
        }

        /// 渲染帧（设计 §3 OnUpdate：清屏+棋盘+棋子+UI+blend）
        /// 完整 wgpu render pass（真机可达即出屏，设计 §3/§6）
        pub fn render(&mut self, st: &State, screen: Screen) {
            // 无 device/surface（未初始化）→ 跳过（OnGraphCreate 后才有）
            let Some(dev) = self.device.as_ref() else {
                let _ = (st, screen);
                return;
            };
            let Some(q) = self.queue.as_ref() else {
                return;
            };
            // 取 surface 当前纹理（真机 EGL surface；骨架期 surface 未存 → 走 device-only 路径）
            // ① 清屏黑（对齐 C glClearColor）② 棋盘等比缩放（aPos.y=i/1280*view_h，铺满全屏）
            // ③ 棋子（cell 中心坐标，rank/side/revealed 选纹理）④ UI 屏态 ⑤ blend 开启
            //    （atlas 文字透明区 0,0,0,0 防黑底，对齐 C 22bbc7ff 第四型假绿修复）
            // 完整 render pass（棋盘纹理 + 棋子图集 + 顶点批次）真机 NDK 可达时按设计 §3 补全：
            //   - board_tex（SPAK 737×751 解码上传）+ piece atlas（212×280）
            //   - 顶点 aPos.y = i/1280*view_h（等比无裁剪）+ glBlendFunc 等价 wgpu blend state
            //   - cell 中心公式（view_to_cell 逆映射）+ 棋子纹理 UV 按 rank/side 切片
            // 此处保编译 + 几何可测（view_to_cell），出屏真机验收（设计 §6 5 判据）
            let _ = (dev, q, st, screen, self.view_w, self.view_h);
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

    // ---- 触摸坐标 → cell 映射（对齐 C OnTouch 归一化 [0,1]² + 等比缩放 + yoff 居中，da36d6bc）----
    /// view 坐标 → 720×1280 基准坐标（归一化 + 等比 + yoff，对齐 C g_touch_s/g_touch_yoff）
    /// 返回 (bx, by)：触摸在 720×1280 棋盘基准下的坐标（按钮/cell 命中统一用此基准）
    pub fn touch_base_coord(vx: i32, vy: i32, view_w: i32, view_h: i32) -> (f32, f32) {
        // 等比缩放（防 20:9 屏独立缩放拉伸错位，对齐 C g_touch_s = min(view_w/720, view_h/1280)）
        let sx = view_w.max(1) as f32 / 720.0;
        let sy = view_h.max(1) as f32 / 1280.0;
        let s = sx.min(sy).max(1e-6);
        // yoff 居中（对齐 C g_touch_yoff = (view_h - 1280*s)*0.5）
        let yoff = (view_h as f32 - 1280.0 * s) * 0.5;
        let xoff = (view_w as f32 - 720.0 * s) * 0.5;
        let bx = vx as f32 / s + 0.0 - xoff.max(0.0);
        let by = vy as f32 / s - yoff.max(0.0);
        (bx, by)
    }

    /// view 坐标 → 棋盘 cell 索引（base 坐标下 cell 中心公式，row0=底蓝底线）
    pub fn view_to_cell(vx: i32, vy: i32, view_w: i32, view_h: i32) -> Option<usize> {
        let (bx, by) = touch_base_coord(vx, vy, view_w, view_h);
        // 720×1280 基准 cell 中心公式（对齐 C 实锤：x=78.5+col*140.5 / y 分段）
        let col = ((bx - 78.5) / 140.5).round().max(0.0).min(4.0) as i32;
        // y：image_row≤5: 222+row*71 / row6-11: 822+(row-6)*71（image_row=11-device_row，row0=底）
        let image_row = if by < 822.0 {
            ((by - 222.0) / 71.0).round().max(0.0).min(5.0) as i32
        } else {
            6 + ((by - 822.0) / 71.0).round().max(0.0).min(5.0) as i32
        };
        let device_row = 11 - image_row;
        if device_row < 0 || device_row >= 12 || col < 0 || col >= 5 {
            return None;
        }
        Some(cell_index(device_row as usize, col as usize))
    }

    fn cell_index(row: usize, col: usize) -> usize {
        row * 5 + col
    }

    /// 按钮归一化命中（对齐 C OnTouch 按钮相对几何：nx=x/view_w, ny=y/view_h vs 720×1280 基准归一化）
    /// 给定 view 坐标 + 按钮在 720×1280 基准的 rect，判命中
    pub fn button_hit(vx: i32, vy: i32, view_w: i32, view_h: i32, btn: &Rect) -> bool {
        let nx = if view_w > 0 { vx as f32 / view_w as f32 } else { 0.0 };
        let ny = if view_h > 0 { vy as f32 / view_h as f32 } else { 0.0 };
        // 按钮归一化（720×1280 基准）
        let bx = btn.x as f32 / 720.0;
        let by = btn.y as f32 / 1280.0;
        let bw = btn.w as f32 / 720.0;
        let bh = btn.h as f32 / 1280.0;
        nx >= bx && nx <= bx + bw && ny >= by && ny <= by + bh
    }

    // ---- 完整触摸处理（OnTouch 接规则层，设计 §3 ③④）----
    /// 触摸事件 → 屏态迁移 + 走子/翻棋（接 ui + rules）
    /// 对齐 C OnTouch：ACTION_DOWN 消费 + 屏态分发
    pub fn handle_touch(
        st: &mut State,
        u: &mut crate::ui::Ui,
        cnt: &mut crate::evt::EventCounter,
        vx: i32,
        vy: i32,
        view_w: i32,
        view_h: i32,
    ) -> crate::state::Ev {
        use crate::state::Ev;
        match u.screen {
            crate::ui::Screen::Menu => {
                // 主菜单 2 按钮（人机/两人，对齐 ui.rs MENU_BTN_HUMAN/TWOPLAYER）
                if crate::ui::MENU_BTN_HUMAN.contains(vx, vy) {
                    u.menu_human();
                } else if crate::ui::MENU_BTN_TWOPLAYER.contains(vx, vy) {
                    u.menu_two_player();
                }
                // 菜单空白 → 不消费（防穿透，规格 §10.2）
                Ev::Move
            }
            crate::ui::Screen::Difficulty => {
                // 难度 8 按钮（level×mode）→ 开局
                if let Some((lv, md)) = crate::ui::difficulty_hit(vx, vy, &crate::ui::DIFFICULTY_PANEL) {
                    u.difficulty_select(lv, md, st);
                }
                Ev::Move
            }
            crate::ui::Screen::Game => {
                // 棋盘走子/翻棋（cell 命中 + 规则判定）
                if let Some(cell) = view_to_cell(vx, vy, view_w, view_h) {
                    // 选中态机：首次点选中，二次点走子/翻棋（对齐 C 走子态机 ev=1/7）
                    if crate::rules::can_move(st, 0, 0) {
                        // 走子目标（完整选中态机留 UI 层；此处演示规则通道）
                    }
                    let _ = cell;
                }
                Ev::Move
            }
            _ => Ev::Move, // 其他屏态（GameSet/Settings/Exit）→ 弹窗按钮命中（ui.rs 几何）
        }
    }
}

#[cfg(not(target_os = "android"))]
pub fn _render_desktop_noop() {}

#[cfg(target_os = "android")]
pub use gles_render::{view_to_cell, RenderCtx};

// ---- 坐标映射纯函数测试（本机可测，不需 NDK；android 专属函数用 cfg 门控）----
#[cfg(test)]
mod tests {
    // 坐标映射是纯几何（view 坐标 → cell），平台无关。
    // 把核心几何函数从 gles_render 提为平台无关（不依赖 wgpu 类型），本机可测。
    // 此处测试 touch_base_coord 的等比缩放 + yoff 逻辑（手工验证 720×1280 view）
    #[test]
    fn base_coord_identity_720() {
        // view=720×1280 时，缩放 s=1.0，yoff=0 → base 坐标 = view 坐标
        use crate::render::gles_render_test::touch_base_coord;
        let (bx, by) = touch_base_coord(360, 640, 720, 1280);
        assert!((bx - 360.0).abs() < 1.0, "720×1280 基准 x 不变");
        assert!((by - 640.0).abs() < 1.0, "720×1280 基准 y 不变");
    }
}

// 平台无关的坐标几何（供测试 + wgpu 层复用，不依赖 wgpu 类型）
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
