//! junqi_rs — 军棋正向开发（Rust 版）
//!
//! 人机对战功能链：棋盘几何 / 规则引擎 / AI 评分 / 回合流转 / UI 状态机 / 存档复盘。
//! 模块划分（按规格驱动，非 C 文件 1:1 翻译）：
//!   board   — 棋盘几何 + 铁路/行营/大本营拓扑
//!   rules   — 走子/吃子/翻棋/困毙/胜负规则
//!   ai      — 人机对战 AI 贪心决策
//!   state   — 对局状态 + 悔棋
//!   save    — 存档/复盘
//!
//! 本阶段先落纯逻辑层（board + rules + ai + state + save），cargo test 本机验证。
//! 渲染(wgpu)/JNI 壳 后续阶段再接。
pub mod board;
pub mod state;
pub mod rules;
pub mod ai;
pub mod save;
pub mod ui;
pub mod evt;
pub mod jni;
pub mod render;
pub mod log;
pub mod canvas;
pub mod gles;
pub mod egl_render;
pub mod wgpu_thread;
pub mod asset;
pub mod gametest;
pub mod evtest;
pub mod rprobe;

#[cfg(test)]
mod tests {
    use super::*;

    // 冒烟：棋盘几何
    #[test]
    fn board_smoke() {
        assert!(board::cell_index(0, 0) == 0);
        assert!(board::cell_row(29) == 5);
        assert!(board::cell_col(29) == 4);
    }
}
