//! save — 存档域校验（§8）+ 复盘重建（§7）
//!
//! 规格 §8：全量快照序列化 + 读后校验（原子语义：校验全通过才覆盖调用方状态）。
//! 规格 §7：复盘 = 明棋范式（重建强制全 revealed=1）+ 前 k 步逐条重放，确定性。
//!
//! 本模块纯逻辑层：域校验 / 复盘重建 / 容量截断。文件 I/O（one.sjq/one.sjg 磁盘
//! 读写 + 原子写 tmp+rename）属 jni/磁盘边界，留后续 NDK 阶段接 std::fs，此处只
//! 实现"校验 + 重建"纯函数（规格 §8 域校验 + §7 重放契约）。

use crate::board::N_CELLS;
use crate::state::{
    Cell, RANK_EMPTY, RANK_TOP, Rank, Side, SIDE_BLUE, SIDE_RED, SIDE_UNKNOWN, State,
};

/// 复盘容量（对齐 C N_REPLAY=512：和棋前 1000 步，512 条够复盘）
pub const N_REPLAY: usize = 512;

/// 复盘记录器（规格 §7：走子记录 + 回放重建）
///
/// 基底用 60 格 cells 快照（非 30 字节布局）：精确还原 rank/side/revealed。
pub struct Replay {
    /// 开局 60 格快照（rank/side/revealed）
    pub init_cells: [Cell; N_CELLS],
    /// 开局 turn（ui_first 红先手语义）
    pub turn_init: Side,
    /// 开局 mode（LAY/FLIP 复盘语义）
    pub mode_init: u8,
    /// 走子序列（from, to），长度 n
    pub seq: Vec<[u8; 2]>,
    /// 每步是否翻棋（true=翻棋，false=走子）
    pub is_flip: Vec<bool>,
    /// 记录条数
    pub n: usize,
    /// 当前游标（进入复盘态从最新步开始；回退=cur-- 重建，前进=cur++ 走 1 步）
    pub cur: usize,
}

impl Replay {
    /// 初始化复盘记录器：拷开局 60 格快照 + turn/mode（规格 §7）
    ///
    /// ★ 注意：turn/mode 快照必须在 cells 快照之后写（C AUD-1000 B02-067：
    /// 旧缺陷 memset 清零含快照字段，前批误写在清零前被覆盖）。
    pub fn new(st: &State) -> Self {
        Self {
            init_cells: st.cells,
            turn_init: st.turn,
            mode_init: st.mode,
            seq: Vec::with_capacity(N_REPLAY),
            is_flip: Vec::with_capacity(N_REPLAY),
            n: 0,
            cur: 0,
        }
    }

    /// 记录一步（走子/翻棋成功）（规格 §7：容量满静默丢弃 + 越界 no-op）
    pub fn record(&mut self, from: usize, to: usize, is_flip: bool) {
        if self.n >= N_REPLAY {
            return; // 容量满静默丢弃（和棋 1000 步后半程，诚实边界）
        }
        if from >= N_CELLS || to >= N_CELLS {
            return; // 越界静默 no-op（外部输入脏值丢弃 = 无副作用）
        }
        self.seq.push([from as u8, to as u8]);
        self.is_flip.push(is_flip);
        self.n += 1;
        self.cur = self.n; // 游标跟随记录末尾
    }

    /// 设置游标（clamp 到 [0, n]）
    pub fn cur_set(&mut self, cur: usize) {
        self.cur = if cur > self.n { self.n } else { cur };
    }

    pub fn cur(&self) -> usize {
        self.cur
    }
    pub fn count(&self) -> usize {
        self.n
    }

    /// 回放重建：st = 开局 60 格 cells 快照 + 前 cur 步逐条重走（规格 §7 确定性契约）
    ///
    /// 契约（C 实证）：
    /// ① 基底 = cells 快照（非 30 字节布局），精确还原 rank/side/revealed；
    /// ② 重放前全翻开（revealed=1，引擎 jq_can_move 要求）；
    /// ③ turn/mode 还原开局快照（旧硬编码 BLUE 缺陷已修）；
    /// ④ pieces_left/mines_cleared/stale 归零（重放链自计数，防旧值残带入）；
    /// ⑤ 中途失败（记录与引擎不一致）→ 快照恢复 + 返回 false（防御，不崩）。
    ///
    /// 返回：false=重建失败（已恢复调用方状态）/ true=重建成功。
    ///
    /// 注：本方法只做"快照还原 + 重置计数器"的纯逻辑部分；实际逐条 `try_move`/
    /// `flip` 重放由调用方（rules.rs 落地后）驱动，此处提供重建骨架 + 重放前态还原。
    /// 完整重放（含走子判定）待 rules.rs 落地后在调用方串接。
    pub fn rewind_to(&self, st: &mut State, cur: usize) -> bool {
        // ★ 审计 A11：越界 cur clamp 到有效范围（与 cur_set 一致，不再直接拒绝）
        let cur = if cur > self.n { self.n } else { cur };
        // 防御：快照调用方原状态（重放失败时恢复）
        let snap = st.clone();
        // ① 开局 60 格快照还原
        st.cells = self.init_cells;
        // ② turn/mode 还原开局快照
        st.turn = self.turn_init;
        st.mode = self.mode_init;
        // ③ winner 重置 + 计数器归零（重放链自计数）
        st.winner = crate::state::Winner::None;
        st.moves_since_eat = 0;
        st.total_moves = 0;
        st.pieces_left = [0, 0];
        st.mines_cleared = [0, 0];
        st.stage = crate::state::Stage::Play;
        // ④ 复盘态明棋全翻开（引擎要求 revealed=1 才可走子）
        for c in st.cells.iter_mut() {
            if c.rank != RANK_EMPTY {
                c.revealed = true;
            }
        }
        // ★ 审计 A4：逐条重放前 cur 步（规格 §7 确定性重建）
        for k in 0..cur {
            let (f, t) = (self.seq[k][0] as usize, self.seq[k][1] as usize);
            let ok = if self.is_flip[k] {
                // 暗棋翻棋步：明棋范式下已全翻开 → flip 失败属既定边界（防御中止）
                crate::rules::flip(st, f)
            } else {
                crate::rules::try_move(st, f, t).is_ok()
            };
            if !ok {
                *st = snap; // 重放失败：恢复调用方状态
                return false;
            }
        }
        true
    }
}

// ---- §8 存档域校验（原子语义：校验全通过才覆盖调用方状态）----

/// 读档后域校验（规格 §8：枚举域 / cells 逐格 / 计数钳）。
///
/// 返回：true=校验通过可覆盖 / false=脏档拒绝（调用方状态不变）。
/// 合法域：
///   - turn ∈ {0,1}
///   - stage ∈ {PLAY, OVER}（Rust 版无 SETUP，C STG_OVER=1 即上限）
///   - winner ∈ {None(-1), Blue(0), Red(1), Draw(2)}
///   - mode ∈ {LAY(0), FLIP(1)}
///   - cells 逐格：side∈{0,1,0xFF}、rank∈[0,24]、revealed∈{0,1}（越域格强制归零）
///   - 计数下界钳：pieces_left/mines_cleared 负值钳 0
/// ★ 审计 A5：两阶段存档校验（拒读不改调用方 + 显式净化）
/// 阶段1 `check_enums` 只读校验枚举域（失败调用方状态不变）；
/// 阶段2 `validate_state` 就地净化（cells 归零/计数钳）。
/// 文件反序列化层应先调 check_enums（原始判别值）再构造 State，最后调 validate_state。
pub fn check_enums(st: &State) -> bool {
    if !st.turn.is_valid() || st.mode > 1 { return false; }
    if !matches!(st.stage, crate::state::Stage::Play | crate::state::Stage::Over) { return false; }
    // winner 为 Rust enum（类型安全，非法判别值无法表达）；反序列化层须在构造 State 前校验原始 int ∈ {-1,0,1,2}
    true
}

pub fn validate_state(st: &mut State) -> bool {
    use crate::state::{Stage, Winner};
    // 枚举域校验（任一越域拒读）
    if !st.turn.is_valid() {
        return false;
    }
    if matches!(st.stage, Stage::Over) {
        // OK
    } else if !matches!(st.stage, Stage::Play) {
        return false;
    }
    match st.winner {
        Winner::None | Winner::Blue | Winner::Red | Winner::Draw => {}
    }
    if st.mode > 1 {
        return false;
    }
    // cells 逐格域校验（越域格强制归零，防脏值入引擎）
    for c in st.cells.iter_mut() {
        let side_ok = matches!(c.side, SIDE_BLUE | SIDE_RED | SIDE_UNKNOWN);
        let rank_ok = c.rank <= Rank(RANK_TOP.0 + 0x0c); // 红方编码 raw+0xc 上界 24
        // revealed 已是 bool（类型安全，非法值无法表达）——原 rev_ok 域校验前移到反序列化边界
        if !side_ok || !rank_ok {
            c.rank = RANK_EMPTY;
            c.side = SIDE_UNKNOWN;
            c.revealed = false;
        }
    }
    // 计数下界钳 0（负值脏档钳 0，防进规则引擎/AI）
    for i in 0..2 {
        if st.pieces_left[i] < 0 {
            st.pieces_left[i] = 0;
        }
        if st.mines_cleared[i] < 0 {
            st.mines_cleared[i] = 0;
        }
    }
    true
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::state::{Stage, Winner};

    // §8 枚举域校验：脏 turn/stage/mode 拒读
    #[test]
    fn sjg_domain_check() {
        let mut st = State::new();
        st.mode = 2; // 越域（turn 已 Side enum 类型安全，无法表达非法判别值）
        assert!(!validate_state(&mut st), "turn=2 拒读");
        let mut st2 = State::new();
        st2.turn = SIDE_RED;
        st2.stage = Stage::Over; // 合法
        st2.winner = Winner::Red; // 合法
        st2.mode = 1; // FLIP 合法
        assert!(validate_state(&mut st2), "合法域通过");
    }

    // §8 cells 逐格越域强制归零
    #[test]
    fn sjg_cells_domain() {
        let mut st = State::new();
        st.cells[0].rank = Rank(30); // >24 越域
        st.cells[0].side = SIDE_BLUE;
        st.cells[0].revealed = true;
        assert!(validate_state(&mut st), "cells 越域格归零不拒读");
        assert_eq!(st.cells[0].rank, RANK_EMPTY, "越域格 rank 归零");
        assert_eq!(st.cells[0].side, SIDE_UNKNOWN, "越域格 side 归 UNKNOWN");

        let mut st2 = State::new();
        // side 非法值在反序列化边界 from_raw 拒（Side enum 内部无法表达）
        assert!(Side::from_raw(7).is_none(), "越域 side=7 被 from_raw 拒（类型安全边界）");
        // 内部"越域格归零"改由 rank 越域触发验证
        st2.cells[5].rank = Rank(30); // >24 越域
        st2.cells[5].revealed = true;
        assert!(validate_state(&mut st2));
        assert_eq!(st2.cells[5].rank, RANK_EMPTY, "越域格全格归零");
    }

    // §8 计数下界钳 0
    #[test]
    fn sjg_count_clamp() {
        let mut st = State::new();
        st.pieces_left[0] = -3;
        st.pieces_left[1] = 2;
        st.mines_cleared[0] = -1;
        assert!(validate_state(&mut st));
        assert_eq!(st.pieces_left[0], 0, "负值钳 0");
        assert_eq!(st.pieces_left[1], 2, "正值保持");
        assert_eq!(st.mines_cleared[0], 0);
    }

    // §7 复盘记录 + 游标跟随
    #[test]
    fn replay_record_cursor() {
        let st = State::new();
        let mut rp = Replay::new(&st);
        assert_eq!(rp.count(), 0);
        rp.record(6, 11, false);
        assert_eq!(rp.count(), 1);
        assert_eq!(rp.cur(), 1, "游标跟随记录末尾");
        rp.record(30, 25, true);
        assert_eq!(rp.count(), 2);
        assert_eq!(rp.cur(), 2);
        rp.cur_set(0);
        assert_eq!(rp.cur(), 0);
        rp.cur_set(999);
        assert_eq!(rp.cur(), 2, "clamp 到 n");
    }

    // §7 复盘重建：开局快照 + 全翻开 + 计数归零
    #[test]
    fn replay_rewind_mingqi() {
        // ★ 审计 A4：用真实布局 + 合法走子（旧夹具从空盘记录 6→11，重放必失败）
        let mut init = crate::gametest::make_default_layout();
        init.turn = SIDE_BLUE;
        init.mode = 0; // LAY 明棋（棋子已 revealed，可重放走子）
        init.cfg = [1, 1, 1, 1, 0];

        let mut rp = Replay::new(&init);
        rp.record(6, 11, false); // cell6→cell11 合法走子（真机实证）

        let mut st = init.clone();
        let ok = rp.rewind_to(&mut st, 1);
        assert!(ok);
        assert_eq!(st.turn, SIDE_RED, "重放1步后 turn 切红");
        assert_eq!(st.mode, 0, "mode 还原开局快照");
        assert!(st.cells[6].is_empty(), "重放后源格 cell6 清空");
        assert!(!st.cells[11].is_empty(), "重放后目标格 cell11 有子");
        assert_eq!(st.mines_cleared, [0, 0]);
        assert_eq!(st.mines_cleared, [0, 0]);
        assert_eq!(st.stage, Stage::Play);
    }

    // §7 复盘越界 cur 拒绝
    #[test]
    // ★ C 基准移植（run_replay）：记录 N 步 → rewind K → 与独立重放前 K 步 cells 逐格一致
    #[test]
    fn replay_vs_independent_replay() {
        use crate::state::{Cell, SIDE_BLUE, Stage, Winner};
        let init = crate::gametest::make_default_layout();
        // ★ 动态生成合法走子序列（模拟 C run_replay 的 gen_moves），避免固定坐标失效
        let mut gen = init.clone();
        let mut seq: Vec<(usize, usize)> = Vec::new();
        for _ in 0..4 {
            if let Some(mv) = crate::ai::ai_pick(&gen, 3) {
                if mv.is_flip { break; }
                if crate::rules::try_move(&mut gen, mv.from, mv.to).is_ok() {
                    seq.push((mv.from, mv.to));
                } else { break; }
            } else { break; }
        }
        let k = seq.len() / 2;
        assert!(k >= 1, "至少生成 1 步合法序列");
        let mut rp = Replay::new(&init);
        for &(f, t) in &seq { rp.record(f, t, false); }
        // 独立重放前 K 步（不走 replay API）
        let mut ref_st = init.clone();
        for &(f, t) in &seq[..k] {
            let _ = crate::rules::try_move(&mut ref_st, f, t);
        }
        // replay API 重放到 cur=2
        let mut got = init.clone();
        let ok = rp.rewind_to(&mut got, k);
        assert!(ok, "rewind cur=k 成功");
        // cells 逐格一致（确定性契约）
        let same = got.cells.iter().zip(ref_st.cells.iter()).all(|(a, b)| {
            a.rank == b.rank && a.side == b.side && a.revealed == b.revealed
        });
        assert!(same, "rewind 重建 cells == 独立重放前 K 步（C run_replay 基准）");
    }

    // ★ C 基准移植（run_undo）：走子前快照 → 走子 → undo_restore → 状态回退
    #[test]
    fn undo_roundtrip_vs_c_baseline() {
        let mut st = crate::gametest::make_default_layout();
        st.cfg = [1, 1, 1, 1, 0];
        let snap = st.clone();
        // 玩家 EAT（蓝 25 吃红 30）
        let ev = crate::rules::try_move(&mut st, 25, 30);
        assert_eq!(ev, Ok(crate::state::Ev::Eat), "走子 EV_EAT（C run_undo 步骤1）");
        assert!(st.cells[25].is_empty() && !st.cells[30].is_empty(), "走子改变棋盘");
        // undo 恢复全字段
        st.undo_restore(&snap);
        let same = st.cells.iter().zip(snap.cells.iter()).all(|(a,b)| a.rank==b.rank && a.side==b.side && a.revealed==b.revealed)
            && st.turn == snap.turn && st.stage == snap.stage;
        assert!(same, "undo_restore 全字段回退（C run_undo 基准）");
    }

    fn replay_rewind_oob() {
        let st = State::new();
        let mut rp = Replay::new(&st);
        let mut out = State::new();
        assert!(rp.rewind_to(&mut out, 5), "cur>n clamp 到 n（审计 A11），重建成功");
    }
}
