//! gles_render — 保留触摸坐标 → cell 映射（渲染几何真值，与 asset::build_scene 同源）
//! ★ 2026-11-23 死路径清理：旧的 wgpu surface/device/clear_frame/offscreen_render_* 全部删除，
//!   唯一 device 通路已迁入 render/gpu.rs::create_wgpu_device；出屏由 GpuRenderer 独占。
#[cfg(target_os = "android")]
pub(crate) mod gles_render {
    use crate::state::State;
    use crate::ui::Rect;

    // ---- 触摸坐标 → cell 映射（对齐 C OnTouch 归一化 [0,1]² + 等比缩放 + yoff 居中，da36d6bc）----
    // ★ touch_base_coord 唯一真值在 super::coord（消除双份漂移），view_to_cell 直接复用

    /// view 坐标 → 棋盘 cell 索引（base 坐标下 cell 中心公式，row0=底蓝底线）
    pub fn view_to_cell(vx: i32, vy: i32, view_w: i32, view_h: i32) -> Option<usize> {
        let (bx, by) = super::super::coord::touch_base_coord(vx, vy, view_w, view_h);
        // 720×1280 基准 cell 中心公式（对齐 C 实锤：x=78.5+col*140.5 / y 分段）
        let col = ((bx - 78.5) / 140.5).round().clamp(0.0, 4.0) as i32;
        // y：image_row≤5: 222+row*71 / row6-11: 822+(row-6)*71（image_row=11-device_row，row0=底）
        let image_row = if by < 822.0 {
            ((by - 222.0) / 71.0).round().clamp(0.0, 5.0) as i32
        } else {
            6 + ((by - 822.0) / 71.0).round().clamp(0.0, 5.0) as i32
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

    // ---- 完整触摸处理（历史参考实现，生产路径在 jni.rs::OnTouch）----
    /// 触摸事件 → 屏态迁移 + 走子/翻棋（接 ui + rules）
    /// ★ UI-09：此函数 0 引用（生产走 jni.rs::OnTouch），标注 deprecated 防未来误接。
    ///   坐标口径已修：view 900×1600 → ×0.8 转 720 基准（旧版直接拿 view 坐标喂 720 基准按钮
    ///   → 按钮整体错位，若将来接线会复现点不到的 bug）。
    #[deprecated(note = "未引用的历史参考实现，生产触摸链在 jni.rs::OnTouch；勿直接接线")]
    pub fn handle_touch(
        st: &mut State,
        u: &mut crate::ui::Ui,
        _cnt: &mut crate::evt::EventCounter,
        vx: i32,
        vy: i32,
        view_w: i32,
        view_h: i32,
    ) -> crate::state::Ev {
        #[allow(deprecated)] // 本函数自身已 deprecated，内部调用生产 helper 不受影响
        use crate::state::Ev;
        // ★ 坐标口径统一：view(900×1600) → 720 基准（与 jni.rs OnTouch 的 x*0.8 一致）
        let (bx, by) = (vx as f32 * 0.8, vy as f32 * 0.8);
        match u.screen {
            crate::ui::Screen::Menu => {
                // 主菜单 2 按钮（人机/两人，对齐 ui.rs MENU_BTN_HUMAN/TWOPLAYER）
                if crate::ui::MENU_BTN_HUMAN.contains(bx as i32, by as i32) {
                    u.menu_human();
                } else if crate::ui::MENU_BTN_TWOPLAYER.contains(bx as i32, by as i32) {
                    u.menu_two_player();
                }
                // 菜单空白 → 不消费（防穿透，规格 §10.2）
                Ev::Move
            }
            crate::ui::Screen::Difficulty => {
                // 难度 8 按钮（level×mode）→ 开局
                if let Some((lv, md)) = crate::ui::difficulty_hit(bx as i32, by as i32, &crate::ui::DIFFICULTY_PANEL) {
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
