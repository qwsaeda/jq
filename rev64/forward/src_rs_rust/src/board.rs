//! board — 棋盘几何 + 铁路/行营/大本营拓扑（对齐 C board.c 行为）
//!
//! 规格 §1：12×5=60 格；蓝方 row0-5 / 红方 row6-11。
//! 纯函数，无状态。

pub const BOARD_ROWS: usize = 12;
pub const BOARD_COLS: usize = 5;
pub const N_CELLS: usize = BOARD_ROWS * BOARD_COLS; // 60

/// 行营（安全岛）10 格坐标：蓝 (2,1)(2,3)(3,2)(4,1)(4,3) + 红镜像 (7,1)(7,3)(8,2)(9,1)(9,3)
/// 敌方不能**吃**行营中子；空行营可一步进入；非铁路不可穿行。
pub fn is_camp(row: i32, col: i32) -> bool {
    matches!(
        (row, col),
        (2, 1) | (2, 3) | (3, 2) | (4, 1) | (4, 3)
            | (7, 1) | (7, 3) | (8, 2) | (9, 1) | (9, 3)
    )
}

/// 大本营 4 格：蓝 (0,1)(0,3) / 红 (11,1)(11,3)。军旗落点；进后不可动（cfg1）；空不可入。
pub fn is_base(row: i32, col: i32) -> bool {
    (row == 0 || row == 11) && (col == 1 || col == 3)
}

/// 铁路：纵 col2 贯穿；横 row0/5/6/11 整行。行营/大本营在铁路外。
pub fn is_rail(row: i32, col: i32) -> bool {
    if is_camp(row, col) || is_base(row, col) {
        return false;
    }
    col == 2 || row == 5 || row == 6 || row == 0 || row == 11
}

/// 索引 = row*5 + col（无越界守卫，调用方契约 0..N_CELLS，BFS 性能敏感）
pub fn cell_index(row: usize, col: usize) -> usize {
    row * BOARD_COLS + col
}
pub fn cell_row(idx: usize) -> usize {
    idx / BOARD_COLS
}
pub fn cell_col(idx: usize) -> usize {
    idx % BOARD_COLS
}

#[cfg(test)]
mod tests {
    use super::*;

    // §1 棋盘几何
    #[test]
    fn geometry() {
        assert_eq!(N_CELLS, 60);
        assert_eq!(cell_index(0, 0), 0);
        assert_eq!(cell_index(11, 4), 59);
        assert_eq!(cell_row(29), 5);
        assert_eq!(cell_col(29), 4);
        assert_eq!(cell_row(59), 11);
        assert_eq!(cell_col(59), 4);
    }

    // §1.1 行营 10 格
    #[test]
    fn camp() {
        assert!(is_camp(2, 1) && is_camp(2, 3) && is_camp(3, 2) && is_camp(4, 1) && is_camp(4, 3));
        assert!(is_camp(7, 1) && is_camp(7, 3) && is_camp(8, 2) && is_camp(9, 1) && is_camp(9, 3));
        assert!(!is_camp(0, 0) && !is_camp(5, 5) && !is_camp(-1, 2));
    }

    // §1.1 大本营 4 格
    #[test]
    fn base() {
        assert!(is_base(0, 1) && is_base(0, 3) && is_base(11, 1) && is_base(11, 3));
        assert!(!is_base(0, 0) && !is_base(11, 2) && !is_base(6, 1));
    }

    // §1.1 铁路拓扑（col2 纵 + row0/5/6/11 横，行营/大本营外）
    #[test]
    fn rail() {
        // 纵线 col2（row0/11 交点 (0,2)(11,2) 是纵线 + 底线双重身份，is_rail 返 true）
        assert!(is_rail(0, 2) && is_rail(5, 2) && is_rail(11, 2));
        // 非铁路格（(1,1) 普通格；(3,0) 非铁路行非铁路列）
        assert!(!is_rail(3, 0) && !is_rail(1, 1));
        // (6,4) 在 row6 横铁路整行 → 是铁路（对照 C board.c is_rail row==6）
        assert!(is_rail(6, 4));
        // 底线横铁路（row0/11 整行，col 非 2 且非行营/大本营 → true）
        assert!(is_rail(0, 0) && is_rail(0, 4) && is_rail(11, 0) && is_rail(11, 4));
        // 中线的整行
        assert!(is_rail(5, 0) && is_rail(6, 4));
        // 行营阻断
        assert!(!is_rail(3, 2) && !is_rail(8, 2));
        // 大本营阻断（(0,1)(0,3)(11,1)(11,3) 虽在 row0/11 但 is_base 排 → false）
        assert!(!is_rail(0, 1) && !is_rail(0, 3) && !is_rail(11, 1) && !is_rail(11, 3));
    }
}
