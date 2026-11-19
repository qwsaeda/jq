//! state — 对局状态数据模型 + 悔棋（对齐 junqi.h jq_state）
//!
//! 规格 §1/§8。棋子 rank 编码：1-9 普通（1 最强），10 炸弹，11 地雷，12 军旗。
//! 阵营：蓝 0 / 红 1 / 未知 0xFF（暗棋未定色）。

use crate::board::N_CELLS;

// ---- 棋子等级（rank）----
pub const RANK_EMPTY: u8 = 0;
pub const RANK_BOMB: u8 = 10;
pub const RANK_MINE: u8 = 11;
pub const RANK_FLAG: u8 = 12;
pub const RANK_TOP: u8 = 12;
pub const RANK_WORKER: u8 = 9;

// ---- 阵营 ----
pub const SIDE_BLUE: u8 = 0;
pub const SIDE_RED: u8 = 1;
pub const SIDE_UNKNOWN: u8 = 0xFF;
pub const SIDE_VALID: [u8; 2] = [SIDE_BLUE, SIDE_RED];

/// 阵营是否有效（0/1；0xFF 未定色不在此列）
pub fn side_valid(s: u8) -> bool {
    s == SIDE_BLUE || s == SIDE_RED
}

// ---- 对局模式 ----
pub const MODE_LAY: u8 = 0;  // 明棋布局
pub const MODE_FLIP: u8 = 1; // 暗棋翻棋

// ---- 阶段 ----
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Stage {
    Play,
    Over,
}
// SETUP 为死枚举（规则引擎仅对 PLAY 放行，无布阵编辑 API），Rust 版省略

// ---- 胜负 ----
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Winner {
    None, // -1 未定
    Blue, // 0
    Red,  // 1
    Draw, // 2
}
impl Default for Winner {
    fn default() -> Self {
        Winner::None
    }
}
impl Default for Stage {
    fn default() -> Self {
        Stage::Play
    }
}

// ---- 事件码（音效/统计分流，规格 §11：7 类，EV_MINE 与 EV_MOVE 同名不同码）----
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Ev {
    Move, // EV_MOVE=0 空移（名 Junqi.Move）
    Eat,  // EV_EAT=1 普通吃子（名 Junqi.Eat）
    Bomb, // EV_BOMB=2 炸弹同归（名 Junqi.Bomb）
    Flag, // EV_FLAG=3 吃旗（名 Junqi.Flag）
    Dead, // EV_DEAD=4 被吃（名 Junqi.Dead）
    Mine, // EV_MINE=5 工兵挖雷（名 Junqi.Move，与 MOVE 同名不同码）
    Flip, // EV_FLIP=6 翻棋（名 Junqi.Flip）
}

// ---- 单个格子 ----
#[derive(Clone, Copy, Debug, Default)]
pub struct Cell {
    pub rank: u8,      // 0-12（红方已翻开存 +0xc，用 rank_of 还原）
    pub side: u8,      // SIDE_BLUE/SIDE_RED/SIDE_UNKNOWN
    pub revealed: u8,  // 0=背面（暗棋未翻）1=正面
}

impl Cell {
    pub fn empty() -> Self {
        Self { rank: 0, side: 0, revealed: 0 }
    }
    pub fn is_empty(&self) -> bool {
        self.rank == RANK_EMPTY
    }
}

/// 阵营感知 rank 还原（§1.1 rank 编码：红方已翻开存 rank+0xc）
/// 蓝方/未定色 rank 必须 ≤12；红方编码域 [13,24]。
pub fn rank_of(c: &Cell) -> u8 {
    if c.rank > RANK_TOP {
        if c.side == SIDE_RED && c.rank <= RANK_TOP + 0x0c {
            return c.rank - 0x0c; // 红方 +0xc 还原
        }
        return RANK_TOP; // 蓝/未定色 >12 = 脏值哨兵
    }
    c.rank
}

// ---- 对局状态 ----
#[derive(Clone, Debug)]
pub struct State {
    pub cells: [Cell; N_CELLS],
    pub turn: u8,        // 当前方 0/1
    pub stage: Stage,
    pub winner: Winner,
    pub moves_since_eat: i32, // 连续无吃子步数（和棋 70 步）
    pub total_moves: i32,     // 总步数（和棋 1000 步）
    pub mines_cleared: [i32; 2], // 暗棋工兵挖雷计数 [蓝,红]
    pub pieces_left: [i32; 2],   // 各方存活子数（吃子/炸弹 EV 递减，和棋/困毙路径计数）
    pub mode: u8,          // MODE_LAY / MODE_FLIP
    /// 规则开关快照（规格 §2：cfg[0]和棋 cfg[1]大本营 cfg[2]吃雷 cfg[3]吃旗 cfg[4]暗棋门槛）
    /// 开局快照语义（UI 层改开关写此字段，规则引擎读快照，避免 C 版运行时全局竞态）
    pub cfg: [u8; 5],
}

impl State {
    /// 初始化空盘（全 60 格 RANK_EMPTY，蓝先手，PLAY，明棋）
    pub fn new() -> Self {
        Self {
            cells: [Cell::empty(); N_CELLS],
            turn: SIDE_BLUE,
            stage: Stage::Play,
            winner: Winner::None,
            moves_since_eat: 0,
            total_moves: 0,
            mines_cleared: [0, 0],
            pieces_left: [0, 0],
            mode: MODE_LAY,
            // 规则开关默认全开（对齐原版：和棋/大本营/吃雷/吃旗/暗棋门槛）
            cfg: [1, 1, 1, 1, 1],
        }
    }

    /// 整盘恢复（悔棋）：全字段覆盖（规格 §6）
    pub fn undo_restore(&mut self, snap: &State) {
        if std::ptr::eq(self, snap) {
            return;
        }
        *self = snap.clone();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rank_of_red_encoded() {
        // 红方 rank 8 已翻开存 8+0xc=20，rank_of 还原 8
        let c = Cell { rank: 20, side: SIDE_RED, revealed: 1 };
        assert_eq!(rank_of(&c), 8);
        // 蓝方 rank 8 存 8，rank_of 还原 8
        let b = Cell { rank: 8, side: SIDE_BLUE, revealed: 1 };
        assert_eq!(rank_of(&b), 8);
    }

    #[test]
    fn undo_roundtrip() {
        let mut s = State::new();
        s.turn = SIDE_RED;
        s.cells[0].rank = 1;
        let snap = s.clone();
        s.turn = SIDE_BLUE;
        s.cells[0].rank = 0;
        s.undo_restore(&snap);
        assert_eq!(s.turn, SIDE_RED);
        assert_eq!(s.cells[0].rank, 1);
    }
}
