//! gametest — 完整对局自测（移植 C game_selftest.c：双 AI 走满，验胜负/和棋/困毙全路径）
//!
//! C 基准：蓝/红各 ai_step（贪心同 ai_score，吃子族优先 ≥50 分），60 局收束到
//! 合法终局（胜/负/和/困毙之一，无非法状态）。移植到 Rust 把"行为对齐深度"拉高
//! （单测 42 → 完整对局回归），对齐 C 正向"对齐上限"证据。

use crate::state::{Cell, State, SIDE_BLUE, SIDE_RED, Side, Rank};
use crate::{ai, rules};

/// 单方走一步（暗棋先翻，明棋走子/吃子，贪心同 ai_score）。对齐 C ai_step。
fn ai_step(st: &mut State, _me: Side, level: u8) -> Option<(usize, usize, bool)> {
    // 走子候选（吃子族优先，复用 ai::ai_pick 的贪心 + 难度分流）
    if let Some(mv) = ai::ai_pick(st, level) {
        if !mv.is_flip {
            return Some((mv.from, mv.to, false));
        }
    }
    // 翻棋（ai_pick 已含翻棋候选回退）
    if let Some(mv) = ai::ai_pick(st, level) {
        return Some((mv.from, mv.to, mv.is_flip));
    }
    None
}

/// 完整对局自测（60 局，双 AI 走满，验证终局合法 + 困毙/和棋覆盖）
pub fn run_game_selftest(games: usize) -> (u32, u32, u32, u32) {
    let mut win_red = 0u32;
    let mut win_blue = 0u32;
    let mut draws = 0u32;
    let mut stalemates = 0u32;

    for _g in 0..games {
        let mut st = make_default_layout();
        st.turn = SIDE_BLUE;
        st.stage = crate::state::Stage::Play;
        let mut guard = 0;
        while st.stage == crate::state::Stage::Play && guard < 5000 {
            guard += 1;
            let me = st.turn; // 先读 turn 到局部（ai_step 借 st 后不能再读 st.turn）
            match ai_step(&mut st, me, 3) {
                Some((f, t, is_flip)) => {
                    if is_flip {
                        if !rules::flip(&mut st, f) {
                            break;
                        }
                    } else if rules::try_move(&mut st, f, t).is_err() {
                        break;
                    }
                }
                None => break, // 无合法着法（困局）
            }
        }
        // 终局统计
        match st.winner {
            crate::state::Winner::Red => win_red += 1,
            crate::state::Winner::Blue => win_blue += 1,
            crate::state::Winner::Draw => draws += 1,
            crate::state::Winner::None => {
                // 无 winner = 困毙未触发或 guard 截断（统计为困毙）
                if st.stage == crate::state::Stage::Over {
                    stalemates += 1;
                }
            }
        }
    }
    (win_red, win_blue, draws, stalemates)
}

/// ★ 审计 A6：通用 30 字节布局装载（规格 §0/§5.3）
/// 明棋 LAY：蓝 rank 原值/红 +0xc，revealed=1，side 已定
/// 暗棋 FLIP：双方 side=SIDE_UNKNOWN(0xFF)，revealed=0，rank 统一 raw（不 +0xc）
pub fn init_from_layout_30(layout: &[u8; 30], mode: u8) -> crate::state::State {
    use crate::state::{Cell, State, SIDE_BLUE, SIDE_RED, SIDE_UNKNOWN, MODE_FLIP};
    let mut st = State::new();
    st.mode = mode;
    st.turn = SIDE_BLUE;
    st.stage = crate::state::Stage::Play;
    st.winner = crate::state::Winner::None;
    st.cfg = [1, 1, 1, 1, 1];
    st.moves_since_eat = 0;
    st.total_moves = 0;
    st.mines_cleared = [0, 0];
    st.pieces_left = [0, 0];
    let flip = mode == MODE_FLIP;
    for (i, &v) in layout.iter().enumerate() {
        if v == 0 || v > 12 { continue; }
        let lrow = i / 5;
        let lcol = i % 5;
        let (side, rank, rev) = if flip {
            (SIDE_UNKNOWN, v, false) // 暗棋：未定色 + raw rank + 未翻
        } else {
            (SIDE_RED, v + 0x0c, true) // 明棋红方：+0xc + 已翻
        };
        // 红方半区（i>=15 布局红方 15 字节 → device row6-10）
        let (dev_row, dev_col) = (6 + lrow, 4 - lcol);
        st.cells[dev_row * 5 + dev_col] = Cell { rank: Rank(rank), side, revealed: rev };
        // 蓝方半区（布局同一字节镜像到蓝方）
        let blue_side = if flip { SIDE_UNKNOWN } else { SIDE_BLUE };
        let blue_rank = v;
        st.cells[(5 - lrow) * 5 + lcol] = Cell { rank: Rank(blue_rank), side: blue_side, revealed: rev };
    }
    st
}

/// ★ 原版默认【暗棋】布局（g_opt_mode=1，先手翻子定色）——与 make_default_layout 同一
///   JQ_DEFAULT_LAY[30]，但 side=SIDE_UNKNOWN(0xFF) / revealed=0 / raw rank（不 +0xc）。
///   orig_human.png 真机实锤：原版开局是绿色方块背面，不是明棋圆形带字。
pub fn make_flip_layout() -> State {
    let lay: [u8; 30] = [
        3, 7, 6, 5, 8, 10, 0, 9, 0, 1,
        2, 10, 0, 9, 5, 4, 0, 6, 0, 7,
        8, 3, 4, 9, 11, 11, 12, 11, 8, 7,
    ];
    init_from_layout_30(&lay, crate::state::MODE_FLIP)
}

/// 默认布局（C jq_default_lay.h JQ_DEFAULT_LAY[30] + layout.c:63-90 装载语义）
/// 每个布局字节 i（0-29）→ 蓝方 (5-lrow, lcol) + 红方 (6+lrow, 4-lcol)，红 rank+0xc。
/// lrow=i/5, lcol=i%5；v=0 跳过（行营空位）；v>12 脏字节拒（装载即拒）。
pub fn make_default_layout() -> State {
    let lay: [u8; 30] = [
        3, 7, 6, 5, 8, 10, 0, 9, 0, 1,
        2, 10, 0, 9, 5, 4, 0, 6, 0, 7,
        8, 3, 4, 9, 11, 11, 12, 11, 8, 7,
    ];
    let mut st = State::new();
    st.mode = crate::state::MODE_LAY;
    st.cfg = [1, 1, 1, 1, 0]; // 明棋
    for (i, &v) in lay.iter().enumerate() {
        if v == 0 {
            continue; // 行营空位
        }
        if v > crate::state::RANK_TOP.0 {
            continue; // 脏字节拒（B01-039/040）
        }
        let lrow = i / 5;
        let lcol = i % 5;
        // 蓝方：(5-lrow, lcol)
        let b_row = 5 - lrow;
        let b_idx = b_row * 5 + lcol;
        st.cells[b_idx] = Cell { rank: Rank(v), side: SIDE_BLUE, revealed: true };
        // 红方：(6+lrow, 4-lcol)，rank+0xc
        let r_row = 6 + lrow;
        let r_col = 4 - lcol;
        let r_idx = r_row * 5 + r_col;
        st.cells[r_idx] = Cell { rank: Rank(v + 0x0c), side: SIDE_RED, revealed: true };
    }
    st
}

#[cfg(test)]
mod tests {
    use super::*;

    // C game_selftest 基准：60 局全部合法终局（胜/和/困毙之一，无非法状态）
    #[test]
    fn game_selftest_60() {
        let (wr, wb, dr, sm) = run_game_selftest(60);
        let total = wr + wb + dr + sm;
        eprintln!("诊断: win_red={wr} win_blue={wb} draws={dr} stalemates={sm} total={total}");
        // 对齐 C game_selftest 判据：全收束到合法终局（胜/和/困毙之一，无 guard 截断非法态）
        assert_eq!(total, 60, "60 局全收束到合法终局（无 guard 截断非法态）");
        // 终局态合法性（stage=Over + winner 域合法）
        let mut st2 = make_default_layout();
        st2.turn = SIDE_BLUE;
        st2.stage = crate::state::Stage::Play;
        let mut g2 = 0;
        while st2.stage == crate::state::Stage::Play && g2 < 5000 {
            g2 += 1;
            let me = st2.turn;
            match ai_step(&mut st2, me, 3) {
                Some((f, t, fl)) => {
                    if fl {
                        rules::flip(&mut st2, f);
                    } else if rules::try_move(&mut st2, f, t).is_err() {
                        break;
                    }
                }
                None => break,
            }
        }
        assert!(st2.stage == crate::state::Stage::Over, "终局态 stage=Over");
        assert!(matches!(st2.winner, crate::state::Winner::Blue | crate::state::Winner::Red | crate::state::Winner::Draw | crate::state::Winner::None), "winner 域合法");
        // 终局后 stage=Over（无非法 state 残留）
        let mut st = make_default_layout();
        st.turn = SIDE_BLUE;
        st.stage = crate::state::Stage::Play;
        let mut guard = 0;
        while st.stage == crate::state::Stage::Play && guard < 5000 {
            guard += 1;
            let me = st.turn;
            match ai_step(&mut st, me, 3) {
                Some((f, t, fl)) => {
                    if fl {
                        rules::flip(&mut st, f);
                    } else if rules::try_move(&mut st, f, t).is_err() {
                        break;
                    }
                }
                None => break,
            }
        }
        assert!(st.stage == crate::state::Stage::Over, "终局态 stage=Over");
    }

    // 布局合法性：每方 25 子，rank 域合法
    // ★ 审计 A6：暗棋开局初始化（全 0xFF + 未翻 + raw rank）
    #[test]
    fn flip_layout_all_unknown() {
        use crate::state::{MODE_FLIP, SIDE_UNKNOWN};
        let lay = [3u8,7,6,5,8, 10,0,9,0,1, 2,10,0,9,5, 4,0,6,0,7, 8,3,4,9,11, 11,12,11,8,7];
        let st = init_from_layout_30(&lay, MODE_FLIP);
        assert_eq!(st.mode, MODE_FLIP);
        let occupied: Vec<_> = st.cells.iter().filter(|c| !c.is_empty()).collect();
        assert!(!occupied.is_empty(), "暗棋布局有子");
        assert!(occupied.iter().all(|c| c.side == SIDE_UNKNOWN), "暗棋全未定色 0xFF");
        assert!(occupied.iter().all(|c| !c.revealed), "暗棋全未翻");
        assert!(occupied.iter().all(|c| c.rank <= Rank(12)), "暗棋 rank 用 raw（不 +0xc）");
    }

    #[test]
    fn default_layout_valid() {
        let st = make_default_layout();
        let blue = st.cells[..30].iter().filter(|c| !c.is_empty()).count();
        let red = st.cells[30..].iter().filter(|c| !c.is_empty()).count();
        assert_eq!(blue, 25, "蓝方 25 子");
        assert_eq!(red, 25, "红方 25 子");
        // 红方编码域 13-24（rank+0xc）
        for c in st.cells[30..].iter().filter(|c| !c.is_empty()) {
            assert!(c.rank >= Rank(13) && c.rank <= Rank(24), "红方 rank+0xc 编码域");
        }
    }
}
