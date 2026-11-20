//! 纯 CPU 棋盘 RGBA 合成（从 render.rs 拆分，逐字搬运）
#[cfg(target_os = "android")]
pub fn cpu_board_rgba(w: usize, h: usize, board: Option<&crate::state::State>) -> Vec<u8> {
    let mut out = vec![0u8; w * h * 4];
    for y in 0..h {
        for x in 0..w {
            let i = (y * w + x) * 4;
            let fy = y as f32 / h as f32; // 0=顶 1=底
            let fx = x as f32 / w as f32;
            let (r, g, b) = if fy < 0.62 {
                // 上部棋盘区（60%）：12×5 格，深绿/浅绿交替 + 棋子
                let row = ((fy / 0.62) * 12.0) as usize;
                let col = (fx * 5.0) as usize;
                let mut base = if (row + col) % 2 == 0 { (38, 128, 52) } else { (26, 96, 40) };
                // 棋子：从 board 状态取（device_row = 11-row, idx = device_row*5+col）
                if let Some(st) = board {
                    let device_row = 11usize.saturating_sub(row);
                    let idx = device_row * 5 + col.min(4);
                    if idx < 60 && !st.cells[idx].is_empty() {
                        let c = &st.cells[idx];
                        // 蓝方=浅蓝圆点近似（方色块），红方=红；棋子底色覆盖格子中心区
                        let local_x = (fx * 5.0).fract();
                        let local_y = ((fy / 0.62) * 12.0).fract();
                        if local_x > 0.25 && local_x < 0.75 && local_y > 0.2 && local_y < 0.8 {
                            base = if c.side == crate::state::SIDE_RED { (210, 60, 50) } else { (90, 160, 230) };
                        }
                    }
                }
                base
            } else {
                // 下部菜单区（40%）：5 个橙色按钮
                let by = ((fy - 0.62) / 0.38) * 5.0;
                let btn = by as usize;
                let local = by - btn as f32;
                if fx > 0.15 && fx < 0.85 && local > 0.15 && local < 0.8 && btn < 5 {
                    (230, 128, 38) // 按钮橙色
                } else {
                    (20, 30, 45) // 菜单背景深蓝
                }
            };
            out[i] = r; out[i + 1] = g; out[i + 2] = b; out[i + 3] = 255;
        }
    }
    out
}
#[cfg(not(target_os = "android"))]
pub fn cpu_board_rgba(_w: usize, _h: usize, _b: Option<&crate::state::State>) -> Vec<u8> { vec![] }
