//! rule_probe_bridge — 32 位原版 rule_probe_shared.inc 真值 → 直接测【生产规则函数】
//! 补 rprobe.rs 缺失的 11 个 case（A2/D2/E3-E9/F3/G1），消除"生产规则在这 11 场景
//! 与 64 位真值有无分叉"的判据盲区。真值来自 /tmp/opencode/probe64.txt（C 64 位探针实跑）。
//! 场景摆棋逐行对照 rev64/forward/src_rs/rule_probe_shared.inc，RANK 编码用 probe_red_rank。
use junqi_rs::board::cell_index;
use junqi_rs::rules;
use junqi_rs::state::{Cell, Rank, Side, State, SIDE_BLUE, SIDE_RED, SIDE_UNKNOWN, MODE_LAY, MODE_FLIP};

fn clear(st: &mut State) {
    for c in st.cells.iter_mut() { *c = Cell::empty(); }
    st.turn = SIDE_BLUE;
}
fn put(st: &mut State, row: usize, col: usize, rank: u8, side: Side, rev: bool) {
    st.cells[cell_index(row, col)] = Cell { rank: Rank(rank), side, revealed: rev };
}
fn red_rank(r: u8) -> u8 { r + 0x0c }
fn lay() -> State { let mut s = State::new(); s.mode = MODE_LAY; s.cfg = [1,1,1,1,0]; clear(&mut s); s }
fn flip() -> State { let mut s = State::new(); s.mode = MODE_FLIP; s.cfg = [1,1,1,1,1]; clear(&mut s); s }

// 64 位真值（probe64.txt 实测）
// A2: 底线横滑 row0 col0(工兵 rank9 蓝) → col1(红 rank1 明) = CM 1
#[test]
fn case_a2_bottom_row() {
    let mut st = lay();
    put(&mut st, 0, 0, 9, SIDE_BLUE, true);
    put(&mut st, 0, 1, red_rank(1), SIDE_RED, true);
    assert!(rules::can_move(&st, cell_index(0,0), cell_index(0,1)), "A2 真值 CM=1");
}

// D2: 困毙 + 可吃旗共存 —— 红方仅军旗且蓝方下一手能吃 → SM_red=1（64 位）
#[test]
fn case_d2_stalemate_with_flag() {
    let mut st = lay();
    put(&mut st, 5, 0, 1, SIDE_BLUE, true);
    put(&mut st, 5, 1, red_rank(12), SIDE_RED, true);
    assert!(rules::side_stalemated(&st, SIDE_RED), "D2 真值 SM_red=1");
}

// E3: 工兵 col2 纵贯跨 row8 隔断 row7→row9 = CM 0
#[test]
fn case_e3_worker_cross_row8() {
    let mut st = lay();
    put(&mut st, 7, 2, 9, SIDE_BLUE, true);
    assert!(!rules::can_move(&st, cell_index(7,2), cell_index(9,2)), "E3 真值 CM=0");
}

// E4-E7: 工兵横向进 col2 纵线（row5 各列 → (5,2)）= CM 1
#[test]
fn case_e4_to_e7_worker_into_col2() {
    for col in [0usize, 4, 1, 3] {
        let mut st = lay();
        put(&mut st, 5, col, 9, SIDE_BLUE, true);
        assert!(rules::can_move(&st, cell_index(5,col), cell_index(5,2)), "E col{}→col2 真值 CM=1", col);
    }
}

// E8: 工兵 row5 col0，己方子 row5 col2 阻断直滑 → (5,4) 仍可绕？真值 CM=1（(5,0)→(5,4)）
#[test]
fn case_e8_worker_r5_blocked_own() {
    let mut st = lay();
    put(&mut st, 5, 0, 9, SIDE_BLUE, true);
    put(&mut st, 5, 2, 1, SIDE_BLUE, true);
    assert!(rules::can_move(&st, cell_index(5,0), cell_index(5,4)), "E8 真值 CM=1");
}

// E9: 工兵 row6 col0，己方子 row6 col2 → (6,4) 真值 CM=1
#[test]
fn case_e9_worker_r6_around_to_c4() {
    let mut st = lay();
    put(&mut st, 6, 0, 9, SIDE_BLUE, true);
    put(&mut st, 6, 2, 1, SIDE_BLUE, true);
    assert!(rules::can_move(&st, cell_index(6,0), cell_index(6,4)), "E9 真值 CM=1");
}

// F3: 暗棋先手翻蓝(5,0)定色 side=0，红方回合翻(5,1)定色 side=1，两次 flip 均成功
#[test]
fn case_f3_flip_both_sides() {
    let mut st = flip();
    put(&mut st, 5, 0, 1, SIDE_UNKNOWN, false);
    put(&mut st, 5, 1, 1, SIDE_UNKNOWN, false);
    assert!(rules::flip(&mut st, cell_index(5,0)), "F3 首次 flip 成功");
    assert_eq!(st.cells[cell_index(5,0)].side, SIDE_BLUE, "F3 先手定色=蓝");
    assert!(rules::flip(&mut st, cell_index(5,1)), "F3 二次 flip 成功");
    assert_eq!(st.cells[cell_index(5,1)].side, SIDE_RED, "F3 对方定色=红");
}

// G1: 未定色暗子(side=0xFF)被己方(蓝)吃 → CM 1（64 位真值）
#[test]
fn case_g1_unrevealed_eat() {
    let mut st = flip();
    put(&mut st, 5, 0, 1, SIDE_BLUE, true);
    put(&mut st, 5, 1, 1, SIDE_UNKNOWN, false);
    assert!(rules::can_move(&st, cell_index(5,0), cell_index(5,1)), "G1 真值 CM=1");
}
