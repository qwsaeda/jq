//! state — 对局状态数据模型 + 悔棋（对齐 junqi.h jq_state）
//!
//! 规格 §1/§8。棋子 rank 编码：1-9 普通（1 最强），10 炸弹，11 地雷，12 军旗。
//! 阵营：蓝 0 / 红 1 / 未知 0xFF（暗棋未定色）。

use crate::board::N_CELLS;

// ---- 棋子等级（rank）----
/// 棋子等级 newtype：封装裸 u8，数值语义 100% 保留原版（含红方 +0xc 编码、脏 rank 域），
/// 杜绝 u8/i32 魔法数在比较中混用；行为等价。
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug, Default)]
pub struct Rank(pub u8);
impl Rank {
    pub const EMPTY: Rank = Rank(0);
    pub const BOMB: Rank = Rank(10);
    pub const MINE: Rank = Rank(11);
    pub const FLAG: Rank = Rank(12);
    pub const TOP: Rank = Rank(12);
    pub const WORKER: Rank = Rank(9);
}
impl From<u8> for Rank { fn from(v: u8) -> Self { Rank(v) } }
impl From<Rank> for u8 { fn from(r: Rank) -> Self { r.0 } }
impl From<Rank> for i32 { fn from(r: Rank) -> Self { r.0 as i32 } }
/// 兼容常量别名（值语义与 newtype 一致，类型从裸 u8 升级为 Rank）
pub const RANK_EMPTY: Rank = Rank::EMPTY;
pub const RANK_BOMB: Rank = Rank::BOMB;
pub const RANK_MINE: Rank = Rank::MINE;
pub const RANK_FLAG: Rank = Rank::FLAG;
pub const RANK_TOP: Rank = Rank::TOP;
pub const RANK_WORKER: Rank = Rank::WORKER;

// ---- 阵营（类型安全 enum，替代 u8 0/1/0xFF 哨兵）----
/// 阵营：蓝 / 红 / 未定色（暗棋翻棋前）。非法判别值在反序列化边界用 `from_raw` 拒绝。
#[derive(Clone, Copy, PartialEq, Eq, Debug, Default)]
pub enum Side {
    #[default]
    Blue,
    Red,
    Unknown,
}
impl Side {
    /// 存档原始判别值（对齐原版：蓝 0 / 红 1 / 未定色 0xFF）
    pub fn to_raw(self) -> u8 {
        match self {
            Side::Blue => 0,
            Side::Red => 1,
            Side::Unknown => 0xFF,
        }
    }
    /// 从存档原始值构造；非法值（非 0/1/0xFF）返回 None（域校验前移到此边界）
    pub fn from_raw(v: u8) -> Option<Self> {
        match v {
            0 => Some(Side::Blue),
            1 => Some(Side::Red),
            0xFF => Some(Side::Unknown),
            _ => None,
        }
    }
    /// 已定色（蓝/红；未定色不在此列）
    pub fn is_valid(self) -> bool {
        !matches!(self, Side::Unknown)
    }
    /// 对方阵营（原 `1 - side` 运算，类型安全）
    pub fn opposite(self) -> Self {
        match self {
            Side::Blue => Side::Red,
            Side::Red => Side::Blue,
            Side::Unknown => Side::Unknown,
        }
    }
}
impl std::fmt::Display for Side {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.to_raw())
    }
}
// 兼容常量别名：减少 217 处调用点改动，值语义与 enum 完全一致
pub const SIDE_BLUE: Side = Side::Blue;
pub const SIDE_RED: Side = Side::Red;
pub const SIDE_UNKNOWN: Side = Side::Unknown;
pub const SIDE_VALID: [Side; 2] = [Side::Blue, Side::Red];

/// 阵营是否有效（Blue/Red；Unknown 不在此列）——保留 API，改收 Side
pub fn side_valid(s: Side) -> bool {
    s.is_valid()
}

// ---- 对局模式 ----
pub const MODE_LAY: u8 = 0;  // 明棋布局
pub const MODE_FLIP: u8 = 1; // 暗棋翻棋

// ---- 阶段 ----
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
#[derive(Default)]
pub enum Stage {
    #[default]
    Play,
    Over,
}
// SETUP 为死枚举（规则引擎仅对 PLAY 放行，无布阵编辑 API），Rust 版省略

// ---- 胜负 ----
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
#[derive(Default)]
pub enum Winner {
    #[default]
    None, // -1 未定
    Blue, // 0
    Red,  // 1
    Draw, // 2
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
    pub rank: Rank,     // newtype（红方已翻开存 +0xc，用 rank_of 还原）
    pub side: Side,     // 类型安全 enum（原 u8 0/1/0xFF 哨兵）
    pub revealed: bool, // false=背面（暗棋未翻）true=正面（类型安全，非法值无法表达）
}

impl Cell {
    pub fn empty() -> Self {
        Self { rank: RANK_EMPTY, side: SIDE_BLUE, revealed: false }
    }
    pub fn is_empty(&self) -> bool {
        self.rank == RANK_EMPTY
    }
}

/// 阵营感知 rank 还原（§1.1 rank 编码：红方已翻开存 rank+0xc）
/// 蓝方/未定色 rank 必须 ≤12；红方编码域 [13,24]。
pub fn rank_of(c: &Cell) -> Rank {
    if c.rank > RANK_TOP {
        if c.side == SIDE_RED && c.rank <= Rank(RANK_TOP.0 + 0x0c) {
            return Rank(c.rank.0 - 0x0c); // 红方 +0xc 还原
        }
        return RANK_TOP; // 蓝/未定色 >12 = 脏值哨兵
    }
    c.rank
}

// ---- 对局状态 ----
#[derive(Clone, Debug)]
pub struct State {
    pub cells: [Cell; N_CELLS],
    pub turn: Side,      // 当前方（类型安全 enum，原 u8 0/1）
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
    /// wgpu 出屏初始化标记（OnUpdate 首次 create_wgpu_device 后置 true）
    pub wgpu_init_done: bool,
    /// wgpu device 诊断日志只打一次
    pub wgpu_log_done: bool,
}

impl Default for State {
    fn default() -> Self { Self::new() }
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
            wgpu_init_done: false,
            wgpu_log_done: false,
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
        let c = Cell { rank: Rank(20), side: SIDE_RED, revealed: true };
        assert_eq!(rank_of(&c), Rank(8));
        // 蓝方 rank 8 存 8，rank_of 还原 8
        let b = Cell { rank: Rank(8), side: SIDE_BLUE, revealed: true };
        assert_eq!(rank_of(&b), Rank(8));
    }

    #[test]
    fn undo_roundtrip() {
        let mut s = State::new();
        s.turn = SIDE_RED;
        s.cells[0].rank = Rank(1);
        let snap = s.clone();
        s.turn = SIDE_BLUE;
        s.cells[0].rank = RANK_EMPTY;
        s.undo_restore(&snap);
        assert_eq!(s.turn, SIDE_RED);
        assert_eq!(s.cells[0].rank, Rank(1));
    }
}
