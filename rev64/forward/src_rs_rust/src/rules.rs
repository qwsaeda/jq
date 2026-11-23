//! rules — 走子/吃子/翻棋/困毙/胜负规则引擎（规格 §3-§5，对齐 C rules.c 行为）
//!
//! 公开 API（Rust 版，&State 参数化规则开关 st.cfg）：
//!   can_move(st, from, to) -> bool     当前回合走子合法性（规格 §3）
//!   can_move_any(st, from, to) -> bool  忽略回合（困毙判定用）
//!   try_move(st, from, to) -> Result<Ev, MoveError>  走子执行 + 结算（规格 §4）
//!   can_flip(st, idx) -> bool          翻棋合法性（规格 §5）
//!   flip(st, idx) -> bool              翻棋执行（规格 §5）
//!   side_stalemated(st, side) -> bool  某方困毙（规格 §4.1）
//!   side_has_normal(st, side) -> bool  某方仍有普通子（规格 §4.1 双方无普通子→Draw）

use crate::board::{cell_col, cell_index, cell_row, is_base, is_camp, is_rail, N_CELLS};
use crate::state::{
    rank_of, side_valid, Cell, Ev, Rank, RANK_BOMB, RANK_FLAG, RANK_MINE, RANK_TOP, RANK_WORKER,
    Side, SIDE_BLUE, SIDE_RED, SIDE_UNKNOWN, Stage, State, Winner, MODE_FLIP,
};

/// 走子失败原因（Rust 规范化：用有语义的 Error enum 替代 `Result<Ev, ()>`）。
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum MoveError {
    /// from/to 越界或 from==to
    BadIndices,
    /// 回合/阶段非法
    IllegalTurn,
    /// 规则层拒绝（走子不合法）
    IllegalMove,
}
impl std::fmt::Display for MoveError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{:?}", self)
    }
}
impl std::error::Error for MoveError {}


// ---- 铁路通行性（中间格/目标格可走判定）----
/// 铁路滑行中间格是否可通行（空格 + 非行营 + 非大本营；目标格允许占用=吃子）
fn walkable(st: &State, row: i32, col: i32) -> bool {
    if !(0..12).contains(&row) || !(0..5).contains(&col) {
        return false;
    }
    if is_camp(row, col) || is_base(row, col) {
        return false; // 行营/大本营非铁路不可穿行
    }
    // 中间格须为空（滑行路径不能穿子）
    let c = &st.cells[cell_index(row as usize, col as usize)];
    c.is_empty()
}

/// 工兵铁路滑行（可拐弯，BFS 铁路网）
fn rail_path_engineer(st: &State, fr: i32, fc: i32, tr: i32, tc: i32) -> bool {
    // 目标格自身须可走（可通行 or 可吃子）；滑行终点须为空格（规格 §3：工兵滑行不吃子）
    if !is_rail(tr, tc) {
        return false;
    }
    // 目标格被占（吃子）→ 走相邻一步分支（:364），工兵滑行此处只处理空格目标
    let dst = &st.cells[cell_index(tr as usize, tc as usize)];
    if !dst.is_empty() {
        return false; // 工兵滑行终点须为空（吃子走相邻专属分支）
    }
    // BFS 铁路网（起点→终点，可拐弯）——用标准库 VecDeque + HashSet（惯用 Rust，
    // 替代原 C 风格手写 [i32;64] 双数组队列 + head/tail 游标 + [[bool;5];12]）。
    // 行为逐点等价：起点直接入队（不查 walkable）、4 邻接顺序、目标格免 walkable、
    // 入队即标记 visited、容量上限 64（原 tail<64 截断）均保留。
    use std::collections::{HashSet, VecDeque};
    if fr == tr && fc == tc {
        return false; // from==to 入口已拒
    }
    // ★ 起点不查 walkable（工兵自身有子为合法起点）——保留原语义
    if !is_rail(fr, fc) {
        return false;
    }
    let mut visited: HashSet<(i32, i32)> = HashSet::new();
    let mut queue: VecDeque<(i32, i32)> = VecDeque::with_capacity(64);
    visited.insert((fr, fc));
    queue.push_back((fr, fc));
    const ADJ: [(i32, i32); 4] = [(1, 0), (-1, 0), (0, 1), (0, -1)];
    while let Some((r, c)) = queue.pop_front() {
        if r == tr && c == tc {
            return true;
        }
        for &(dr, dc) in &ADJ {
            let (nr, nc) = (r + dr, c + dc);
            if !(0..12).contains(&nr) || !(0..5).contains(&nc) {
                continue;
            }
            if visited.contains(&(nr, nc)) || !is_rail(nr, nc) {
                continue;
            }
            if nr == tr && nc == tc {
                return true; // 目标格免 walkable（可吃子）
            }
            if !walkable(st, nr, nc) {
                continue;
            }
            visited.insert((nr, nc));
            if queue.len() < 64 {
                queue.push_back((nr, nc)); // 容量上限 64，对齐原 tail<64 截断
            }
        }
    }
    false
}

/// 非工兵沿铁路直走（不拐弯）
fn rail_path_straight(st: &State, fr: i32, fc: i32, tr: i32, tc: i32) -> bool {
    if fr != tr && fc != tc {
        return false; // 必须同行或同列
    }
    if !is_rail(fr, fc) || !is_rail(tr, tc) {
        return false;
    }
    if fr == tr {
        // 横向滑行：整行须是横向铁路行（row0/5/6/11）
        if !(fr == 0 || fr == 5 || fr == 6 || fr == 11) {
            return false;
        }
    } else {
        // 纵向滑行：整列须是纵向铁路（唯一纵贯线 col2）
        if fc != 2 {
            return false;
        }
        // 纵线中段遇行营/大本营阻断
        let step = if tr > fr { 1 } else { -1 };
        let mut rr = fr + step;
        while rr != tr {
            // ★ RRULE-008：删恒假死条件 is_base(rr,2)——is_base 要求 col∈{1,3}，此处 col 固定 2
            //   恒 false（C 版 B01-097 已核实 (0,2)/(11,2) 非大本营）。is_camp 仍生效。
            if is_camp(rr, 2) {
                return false;
            }
            rr += step;
        }
    }
    // 中间格须可通行（空格）
    let sr = if tr > fr { 1 } else if tr < fr { -1 } else { 0 };
    let sc = if tc > fc { 1 } else if tc < fc { -1 } else { 0 };
    let mut r = fr + sr;
    let mut c = fc + sc;
    while r != tr || c != tc {
        if !walkable(st, r, c) {
            return false;
        }
        r += sr;
        c += sc;
    }
    true
}

/// 走子合法性核心（参数 turn：当前方 0/1，-1=忽略回合（困毙用））
fn move_legal(st: &State, from: usize, to: usize, turn: Option<Side>) -> bool {
    use crate::state::Stage;
    if st.stage != Stage::Play {
        return false;
    }
    if from == to {
        return false;
    }
    if from >= N_CELLS || to >= N_CELLS {
        return false;
    }
    // None = 忽略回合（原 -1）；Some(t) 校验阵营域（原 >1 拒收）。
    if let Some(t) = turn {
        if !t.is_valid() {
            return false;
        }
    }

    let src = &st.cells[from];
    let dst = &st.cells[to];
    if src.is_empty() {
        return false;
    }
    let sr = rank_of(src);

    // 回合校验（None=忽略回合时跳过）
    if let Some(t) = turn {
        if src.side != t {
            return false;
        }
    }
    // 军旗/地雷不可动
    if sr == RANK_FLAG || sr == RANK_MINE {
        return false;
    }
    // 源格在大本营 + cfg1 开 → 冻结不可动
    if st.cfg[1] == 1 && is_base(cell_row(from) as i32, cell_col(from) as i32) {
        return false;
    }
    // 暗棋：未翻不可动（src 未揭示 或 side 未定）
    if !src.revealed || src.side == SIDE_UNKNOWN {
        return false;
    }
    let me = src.side;

    // 目标被己方占 → 拒
    if !dst.is_empty() && dst.side == me {
        return false;
    }
    // 行营安全岛：敌方子不能吃行营中子（空行营可一步进入）
    let tr = cell_row(to) as i32;
    let tc = cell_col(to) as i32;
    if is_camp(tr, tc) && !dst.is_empty() && dst.side != me {
        return false;
    }
    // 暗棋 + cfg4 开：大本营安全岛（敌方不可吃大本营普通子，雷/旗除外）
    if st.mode == crate::state::MODE_FLIP
        && st.cfg[4] == 1
        && is_base(tr, tc)
        && !dst.is_empty()
        && dst.side != me
    {
        let dbr = rank_of(dst);
        if dbr != RANK_MINE && dbr != RANK_FLAG {
            return false;
        }
    }
    // 暗棋 + cfg4 开：雷/旗仅工兵可攻（旗须已挖满 3 雷）
    {
        let dr = rank_of(dst);
        if st.mode == crate::state::MODE_FLIP
            && st.cfg[4] == 1
            && !dst.is_empty()
            && (dr == RANK_MINE || dr == RANK_FLAG)
        {
            if dr == RANK_MINE {
                if sr != RANK_WORKER {
                    return false; // 雷仅工兵可吃
                }
            } else {
                if sr != RANK_WORKER {
                    return false; // 旗仅工兵可扛
                }
                if side_valid(me) && st.mines_cleared[me as usize] < 3 {
                    return false; // 须已挖满 3 雷
                }
            }
        }
    }
    // 大本营目标格：空格恒拒（布局期仅旗/雷可入，对局期空大本营无落点）
    if is_base(tr, tc) {
        if dst.is_empty() {
            return false;
        }
        // 暗棋：大本营内普通子不可被吃（仅雷/旗）——审计 A2：须受 cfg4 门控，cfg4=0 退明棋语义
        let d_rank = rank_of(dst);
        if st.mode == crate::state::MODE_FLIP
            && st.cfg[4] == 1
            && d_rank != RANK_MINE
            && d_rank != RANK_FLAG
        {
            return false;
        }
    }
    // 一步相邻（公路线 4 邻接）
    let fr = cell_row(from) as i32;
    let fc = cell_col(from) as i32;
    let dr = tr - fr;
    let dc = tc - fc;
    let adjacent = (dr == 0 && (dc == 1 || dc == -1)) || (dc == 0 && (dr == 1 || dr == -1));
    // 行营落点仅允许一步相邻（非铁路不可穿行）
    if is_camp(tr, tc) && !adjacent {
        return false;
    }
    if adjacent {
        return true;
    }
    // 铁路滑行
    if sr == RANK_WORKER {
        return rail_path_engineer(st, fr, fc, tr, tc);
    }
    rail_path_straight(st, fr, fc, tr, tc)
}

// ---- 公开 API ----

/// 当前回合走子合法性（规格 §3）
pub fn can_move(st: &State, from: usize, to: usize) -> bool {
    move_legal(st, from, to, Some(st.turn))
}

/// 忽略回合的走子合法性（困毙判定用，turn=-1）
pub fn can_move_any(st: &State, from: usize, to: usize) -> bool {
    move_legal(st, from, to, None)
}

/// 走子执行 + 结算（规格 §4）。返回事件码；失败（非法）返回 Err。
pub fn try_move(st: &mut State, from: usize, to: usize) -> Result<Ev, MoveError> {
    if from == to || from >= N_CELLS || to >= N_CELLS {
        return Err(MoveError::BadIndices);
    }
    if !move_legal(st, from, to, Some(st.turn)) {
        return Err(MoveError::IllegalMove);
    }
    let src = &st.cells[from];
    let sr = rank_of(src);
    let dst = &st.cells[to];
    let dr = rank_of(dst);
    let me = src.side;
    if !side_valid(me) {
        return Err(MoveError::IllegalTurn); // 外部脏 turn 降级为明确失败
    }
    // 快照两侧 cell（避免后续改 st 字段时持 &st.cells 冲突）
    let src_cell = *src;
    let dst_cell = *dst;

    let ev = if dst_cell.is_empty() {
        // 空目标：互换（from←to=0，to←src）
        st.cells[to] = src_cell;
        st.cells[from] = Cell::empty();
        st.moves_since_eat += 1;
        Ev::Move
    } else if sr == RANK_BOMB || dr == RANK_BOMB {
        // 炸弹同归（含炸弹撞旗）
        let dst_side = dst_cell.side;
        st.cells[from] = Cell::empty();
        st.cells[to] = Cell::empty();
        st.moves_since_eat = 0;
        // 炸弹炸军旗 → 扛旗方胜（dst_side 有效才判，0xFF 未定不判）
        if dr == RANK_FLAG && side_valid(dst_side) {
            st.winner = winner_from_side(me);
            st.stage = Stage::Over;
            st.total_moves += 1;
        }
        Ev::Bomb
    } else if dr == RANK_FLAG {
        // 吃旗
        if st.cfg[3] == 1 {
            // cfg3 开：扛旗即胜
            on_flag_capture(st, me);
            if st.stage == Stage::Over {
                st.cells[to] = src_cell;
                st.cells[from] = Cell::empty();
                st.total_moves += 1;
                st.moves_since_eat = 0;
            }
            return Ok(Ev::Flag);
        } else {
            // cfg3 关：走子正常收口，不判胜
            st.cells[to] = src_cell;
            st.cells[from] = Cell::empty();
            st.turn = me.opposite();
            st.total_moves += 1;
            st.moves_since_eat = 0;
            stalemate_after(st);
            return Ok(Ev::Flag);
        }
    } else if dr == RANK_MINE {
        // 地雷
        if sr == RANK_WORKER && st.cfg[2] == 1 {
            // 工兵挖雷（cfg2 开）：工兵留位，雷消失
            st.cells[to] = src_cell;
            st.cells[from] = Cell::empty();
            st.mines_cleared[me as usize] += 1;
            st.moves_since_eat = 0;
            Ev::Mine // 挖雷（码 EV_MINE=5，名级 Junqi.Move 与 EV_MOVE 同名不同码）
        } else {
            // 非工兵撞雷 或 cfg2 关：送死（雷不消失，无子被吃 → 不清零 moves_since_eat）
            st.cells[from] = Cell::empty();
            Ev::Dead
        }
    } else {
        // 普通吃子比较（rank 小=强）
        if sr < dr {
            // 大压小：src 吃 dst
            st.cells[to] = src_cell;
            st.cells[from] = Cell::empty();
            st.moves_since_eat = 0;
            Ev::Eat
        } else if sr > dr {
            // src 被吃
            st.cells[from] = Cell::empty();
            st.moves_since_eat = 0;
            Ev::Dead
        } else {
            // 同级互杀（双格清空，EV_EAT 非 BOMB——对齐原版事件名）
            st.cells[from] = Cell::empty();
            st.cells[to] = Cell::empty();
            st.moves_since_eat = 0;
            Ev::Eat
        }
    };

    // 通用收口（非终局路径：切 turn + 计数 + 收口）
    if st.stage == Stage::Play {
        st.turn = me.opposite();
        st.total_moves += 1;
        stalemate_after(st);
    }
    Ok(ev)
}

/// ★ 触摸首点选中判定（抽成桌面可测纯函数，根治 jni.rs android-only 测试盲区）
/// 已翻己方子可选走；暗棋模式下未翻暗子只要可翻即选中（二次点同格 → flip）。
/// 旧 jni.rs 内联判定只认 `side==turn && revealed`，导致默认暗棋开局全 0xFF 未翻
/// → 首点永远选不中 → 翻棋链不可达 → 人类与 AI 双双死锁，对局根本无法开始（P0）。
pub fn can_select_for_touch(st: &State, cell: usize) -> bool {
    if cell >= N_CELLS || st.cells[cell].is_empty() {
        return false;
    }
    let c = &st.cells[cell];
    if c.side == st.turn && c.revealed {
        return true; // 己方已翻子：可选走子
    }
    if st.mode == MODE_FLIP && !c.revealed {
        return can_flip(st, cell); // 暗棋未翻子：可翻即可选（定色归翻子方）
    }
    false
}

/// 翻棋合法性（规格 §5：白名单只排军旗，0xFF 当前方定色）
pub fn can_flip(st: &State, idx: usize) -> bool {
    use crate::state::Stage;
    if idx >= N_CELLS || st.stage != Stage::Play {
        return false;
    }
    // ★ RRULE-003 修复：翻棋门只应在暗棋模式生效——与 can_select_for_touch:403
    //   （要求 MODE_FLIP）对齐，消除同模块两套翻棋门判定相左。明棋模式正常无未翻子
    //   （全 revealed=true），此门不影响正常对局/困毙扫描。
    if st.mode != crate::state::MODE_FLIP {
        return false;
    }
    let c = &st.cells[idx];
    if c.is_empty() || c.revealed {
        return false;
    }
    let r = rank_of(c);
    if r == RANK_FLAG {
        return false; // 军旗不可翻
    }
    // 0xFF 未定色：当前回合方可翻（定色归翻子方）
    if c.side == SIDE_UNKNOWN {
        return side_valid(st.turn);
    }
    // 己方暗子可翻（回合门）
    c.side == st.turn
}

/// 翻棋执行（规格 §5：定色 + revealed + 切 turn + 收口，不计步）
pub fn flip(st: &mut State, idx: usize) -> bool {
    if !can_flip(st, idx) {
        return false;
    }
    let c = &mut st.cells[idx];
    if c.side == SIDE_UNKNOWN {
        c.side = st.turn; // 先手翻子定色（归翻子方）
    }
    c.revealed = true;
    st.turn = st.turn.opposite();
    stalemate_after(st); // 翻棋收口（和棋+困毙），不切计步（§4.2）
    true
}

/// 某方困毙（规格 §4.1：无子可动/可翻 → 该方负）
pub fn side_stalemated(st: &State, side: Side) -> bool {
    if !side_valid(side) || st.stage != Stage::Play {
        return false;
    }
    if matches!(st.winner, Winner::Blue | Winner::Red | Winner::Draw) {
        return false; // 已终局
    }
    // 还有未翻暗子可翻 → 未困毙
    // ★ F1 裁定（2026-11-22）：以原版 32 位 0x30320 反汇编为唯一事实源——
    //   `cmp r1,#11 beq` = **仅排地雷**，军旗不排（0x30350 未翻军旗计入可翻）。
    //   旧实现"排旗+雷"是过时的 C 64 位口径（C 正向 rules.c:1050 已于 2026-11-14
    //   对齐原版改为仅排地雷，REVERSE_LOG §124y-1950）。Rust/SPEC 曾落后于此，本批对齐原版。
    for i in 0..N_CELLS {
        let c = &st.cells[i];
        let cn = rank_of(c);
        if !c.is_empty()
            && !c.revealed
            && cn != RANK_MINE // 原版 0x30320：仅排地雷；军旗保持可翻（算未困毙）
            && (c.side == SIDE_UNKNOWN || c.side == side)
        {
            return false;
        }
    }
    // 还有可动子（忽略回合扫描）→ 未困毙
    for f in 0..N_CELLS {
        let c = &st.cells[f];
        if c.is_empty() {
            continue;
        }
        let cr = rank_of(c);
        if cr == RANK_FLAG || cr == RANK_MINE {
            continue; // 旗/雷不可动
        }
        // 暗棋：源须已揭示 + 侧匹配（0xFF 双方）
        if !c.revealed || c.side == SIDE_UNKNOWN {
            continue;
        }
        if c.side != side {
            continue;
        }
        for t in 0..N_CELLS {
            if move_legal_ignore_flip_gate(st, f, t) {
                return false;
            }
        }
    }
    true
}

/// 困毙扫描专用：走子合法性（跳过暗棋 0xFF 源不可动前置——困毙要看"该方已翻子能否走"）
fn move_legal_ignore_flip_gate(st: &State, from: usize, to: usize) -> bool {
    // 复用 move_legal 的回合忽略 + 暗棋源揭示门：move_legal 已含 `!src.revealed||side==0xFF→false`，
    // 困毙扫描需"已揭示己方子能否走"——直接调 move_legal(-1) 即可（前置门自动拦 0xFF/未翻）
    move_legal(st, from, to, None)
}

/// 某方仍有普通子（规格 §4.1：双方无普通子→Draw；存在性非可动性）
pub fn side_has_normal(st: &State, side: Side) -> bool {
    for i in 0..N_CELLS {
        let c = &st.cells[i];
        if c.is_empty() {
            continue;
        }
        if c.side == SIDE_UNKNOWN {
            return true; // 未翻暗子：潜在普通子，续局
        }
        if c.side != side {
            continue;
        }
        let mut r = rank_of(c);
        // ★ RRULE-005 标注：rank_of（state.rs）已保证返回 rank ≤ RANK_TOP(12)，
        //   故下面 r > RANK_TOP 分支在正常路径恒假（红方 +0xc 编码在 rank_of 里已还原）。
        //   保留此分支作脏数据兜底 + C-faithful 对齐，勿误以为它处理常规红方 rank。
        if side == SIDE_RED && r > RANK_TOP {
            r = Rank(r.0 - 0x0c); // 红方 +0xc 归一（正常路径不可达，脏值兜底）
        }
        if r == RANK_MINE || r == RANK_FLAG {
            continue;
        }
        return true; // rank 1-10（含炸弹）= 普通子
    }
    false
}

/// 终局收口（规格 §4.1：吃旗/炸旗即时终局 > 和棋 > 困毙）
fn stalemate_after(st: &mut State) {
    if st.stage != Stage::Play {
        return;
    }
    // 和棋（cfg0 开）：70 步无吃子 或 总 1000 步
    if st.cfg[0] == 1 && (st.moves_since_eat >= 70 || st.total_moves >= 1000) {
        on_draw(st);
    }
    // 双方无普通子 → Draw（存在性）
    if st.stage == Stage::Play && !side_has_normal(st, SIDE_BLUE) && !side_has_normal(st, SIDE_RED) {
        on_draw(st);
    } else if st.stage == Stage::Play && side_stalemated(st, st.turn) {
        // 当前方困毙 → 对方胜
        on_stalemate(st, st.turn);
    }
}

/// 和棋（规格 §4.1：winner=Draw, stage=Over）
/// ★ C 双守卫（B01-065/066）：stage!=PLAY 拒收 + winner!=NONE 拒收（防脏 winner 被覆盖/重复终局副作用）
pub fn on_draw(st: &mut State) {
    if st.stage != Stage::Play {
        return;
    }
    if !matches!(st.winner, Winner::None) {
        return;
    }
    st.winner = Winner::Draw;
    st.stage = Stage::Over;
}

/// 吃旗胜（规格 §4.1：攻方胜）
fn on_flag_capture(st: &mut State, me: Side) {
    if st.stage != Stage::Play {
        return; // 双终局叠加：stage 已 OVER 拒收（攻子入位不回滚）
    }
    st.winner = winner_from_side(me);
    st.stage = Stage::Over;
}

/// 困毙（规格 §4.1：无棋可走方负，对方胜）
pub fn on_stalemate(st: &mut State, loser_side: Side) {
    // ★ F6: 对齐 C jq_on_stalemate 的 SIDE_VALID 守卫——Unknown 传入应拒绝而非误判红胜
    if !loser_side.is_valid() {
        return;
    }
    if st.stage != Stage::Play {
        return;
    }
    if !matches!(st.winner, Winner::None) {
        return;
    }
    st.winner = winner_from_side(loser_side.opposite());
    st.stage = Stage::Over;
}

// ---- 阵营→Winner 辅助（自由函数，跨模块不给 state::Winner 加 impl）----
fn winner_from_side(s: Side) -> Winner {
    if s == SIDE_BLUE {
        Winner::Blue
    } else {
        Winner::Red
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::state::{MODE_FLIP, MODE_LAY};

    /// ★ P0 回归：默认暗棋开局全 0xFF 未翻，首点必须可选→二次点翻棋。
    ///   旧 jni.rs 内联选中门（side==turn && revealed）使暗子永远选不中 → 对局死锁；
    ///   该逻辑在 android-only 文件里，桌面测试此前完全覆盖不到（结构性盲区）。
    #[test]
    fn touch_select_dark_chess_first_flip() {
        let st = crate::gametest::make_flip_layout();
        assert_eq!(st.mode, MODE_FLIP, "默认应暗棋开局");
        let mut can_flip_n = 0;
        for i in 0..N_CELLS {
            if st.cells[i].is_empty() { continue; }
            if can_flip(&st, i) {
                assert!(can_select_for_touch(&st, i), "可翻暗子 cell{} 首点应可选", i);
                can_flip_n += 1;
            }
        }
        assert!(can_flip_n >= 40, "暗棋开局可翻子数 {}", can_flip_n);
        // 选中→翻棋链真执行（绕开 android-only 的 G_SEL，验证规则层可达）
        let first = (0..N_CELLS).find(|&i| can_flip(&st, i)).unwrap();
        let mut s = st.clone();
        assert!(can_select_for_touch(&s, first));
        assert!(flip(&mut s, first), "首点选中后翻棋应成功");
        assert_eq!(s.turn, SIDE_RED, "翻后切红方（AI/对手应着）");
        // 明棋未翻子（非暗棋模式）不应被当暗子选中
        let mut ming = setup_ming();
        ming.mode = MODE_LAY;
        assert!(!can_select_for_touch(&ming, 25 + 1), "明棋空/敌格不应可选");
    }

    /// 构造明棋简单盘（蓝 1 子 (5,0)=idx25 rank3，红 1 子 (6,0)=idx30 rank8）
    fn setup_ming() -> State {
        let mut s = State::new();
        s.mode = MODE_LAY; // 明棋
        s.cfg = [1, 1, 1, 1, 0]; // 暗棋门槛关（明棋场景）
        // 蓝 (5,0)=idx25 rank3 revealed=1 side=BLUE
        s.cells[25] = Cell { rank: Rank(3), side: SIDE_BLUE, revealed: true };
        // 红 (6,0)=idx30 rank8 revealed=1 side=RED
        s.cells[30] = Cell { rank: Rank(8), side: SIDE_RED, revealed: true };
        s.turn = SIDE_BLUE;
        s
    }

    // §3 相邻一步合法（同列相邻）
    #[test]
    fn move_legal_adjacent() {
        let mut s = setup_ming();
        s.turn = SIDE_RED;
        // 红 (6,0)=30 → 空 (7,0)=35 相邻一步
        assert!(can_move(&s, 30, 35), "红 30→35 相邻合法");
    }

    // §3 回合不符拒（蓝回合走红子）
    #[test]
    fn move_legal_turn_gate() {
        let mut s = setup_ming();
        s.turn = SIDE_BLUE;
        // 蓝回合走红子 30 → 拒
        assert!(!can_move(&s, 30, 35), "蓝回合走红子拒");
    }

    // §4 吃子（蓝 rank3 吃红 rank8，30 格）
    #[test]
    fn settle_eat() {
        let mut s = setup_ming();
        s.turn = SIDE_BLUE;
        // 蓝 25(rank3) → 红 30(rank8)：相邻 25(5,0)→? 30(6,0) 同列相邻一步
        let ev = try_move(&mut s, 25, 30).unwrap();
        assert_eq!(ev, Ev::Eat, "rank3 吃 rank8 = EV_EAT");
        assert_eq!(s.cells[30].rank, Rank(3), "吃子后蓝子落 30");
        assert!(s.cells[25].is_empty(), "源格清空");
        assert_eq!(s.turn, SIDE_RED, "切到红方");
    }

    // §4 同级互杀（双格清空，EV_EAT 非 BOMB）
    #[test]
    fn settle_tie() {
        let mut s = State::new();
        s.mode = MODE_LAY;
        s.cfg[4] = 0;
        s.cells[25] = Cell { rank: Rank(5), side: SIDE_BLUE, revealed: true };
        s.cells[30] = Cell { rank: Rank(5), side: SIDE_RED, revealed: true };
        s.turn = SIDE_BLUE;
        let ev = try_move(&mut s, 25, 30).unwrap();
        assert_eq!(ev, Ev::Eat, "同级互杀 = EV_EAT（非 BOMB）");
        assert!(s.cells[25].is_empty() && s.cells[30].is_empty(), "双格清空");
    }

    // §4 炸弹同归（EV_BOMB，双格清空）
    #[test]
    fn settle_bomb() {
        let mut s = State::new();
        s.mode = MODE_LAY;
        s.cfg[4] = 0;
        s.cells[25] = Cell { rank: RANK_BOMB, side: SIDE_BLUE, revealed: true };
        s.cells[30] = Cell { rank: Rank(5), side: SIDE_RED, revealed: true };
        s.turn = SIDE_BLUE;
        let ev = try_move(&mut s, 25, 30).unwrap();
        assert_eq!(ev, Ev::Bomb, "炸弹同归 = EV_BOMB");
        assert!(s.cells[25].is_empty() && s.cells[30].is_empty());
    }

    // §4.1 吃旗胜（cfg3 开：扛旗即胜，stage=Over）
    #[test]
    fn flag_capture_win() {
        let mut s = State::new();
        s.mode = MODE_LAY;
        s.cfg[4] = 0; // 暗棋门槛关（明棋任意子可扛旗）
        s.cfg[3] = 1; // 吃旗开关开
        s.cells[25] = Cell { rank: Rank(2), side: SIDE_BLUE, revealed: true };
        s.cells[30] = Cell { rank: RANK_FLAG, side: SIDE_RED, revealed: true };
        s.turn = SIDE_BLUE;
        let ev = try_move(&mut s, 25, 30).unwrap();
        assert_eq!(ev, Ev::Flag);
        assert_eq!(s.stage, crate::state::Stage::Over, "cfg3 开扛旗终局");
        assert_eq!(s.winner, Winner::Blue, "蓝方扛旗胜");
    }

    // §5 翻棋（暗棋 0xFF 定色 + revealed）
    #[test]
    fn flip_0xff_fix_side() {
        let mut s = State::new();
        s.mode = MODE_FLIP;
        s.cfg[4] = 1;
        s.cells[25] = Cell { rank: Rank(3), side: SIDE_UNKNOWN, revealed: false };
        s.turn = SIDE_BLUE;
        assert!(can_flip(&s, 25), "0xFF 当前方可翻");
        assert!(flip(&mut s, 25));
        assert_eq!(s.cells[25].side, SIDE_BLUE, "定色归翻子方（蓝）");
        assert!(s.cells[25].revealed);
        assert_eq!(s.turn, SIDE_RED, "翻棋切回合");
    }

    // §5 军旗不可翻
    #[test]
    fn flip_flag_rejected() {
        let mut s = State::new();
        s.mode = MODE_FLIP;
        s.cells[25] = Cell { rank: RANK_FLAG, side: SIDE_BLUE, revealed: false };
        s.turn = SIDE_BLUE;
        assert!(!can_flip(&s, 25), "军旗不可翻");
    }

    // §4.1 困毙（单方无子可动可翻）
    #[test]
    fn stalemate_no_moves() {
        let mut s = State::new();
        s.mode = MODE_FLIP;
        s.cfg[4] = 1;
        // 红方全空 + 已翻 + 无子 → 红困毙
        s.turn = SIDE_RED;
        // 红无任何子（全空），蓝有 1 已翻子
        s.cells[25] = Cell { rank: Rank(3), side: SIDE_BLUE, revealed: true };
        assert!(side_stalemated(&s, SIDE_RED), "红无子可动=困毙");
        assert!(!side_stalemated(&s, SIDE_BLUE), "蓝有子=未困毙");
    }

    // §4.1 双方无普通子 → Draw
    #[test]
    fn draw_no_normal() {
        let mut s = State::new();
        s.mode = MODE_LAY;
        s.cfg[0] = 1;
        s.cells[25] = Cell { rank: RANK_MINE, side: SIDE_BLUE, revealed: true };
        s.cells[30] = Cell { rank: RANK_FLAG, side: SIDE_RED, revealed: true };
        assert!(!side_has_normal(&s, SIDE_BLUE), "蓝无普通子");
        assert!(!side_has_normal(&s, SIDE_RED), "红无普通子");
        stalemate_after(&mut s);
        assert_eq!(s.winner, Winner::Draw, "双方无普通子=和棋");
    }

    // §4.2 计步真值表（S2）：撞雷送死 mse 不变（非 ++ 非 0）
    #[test]
    fn settle_mine_dead_mse_unchanged() {
        let mut s = State::new();
        s.mode = MODE_LAY;
        s.cfg[2] = 1;
        s.cells[25] = Cell { rank: Rank(5), side: SIDE_BLUE, revealed: true };
        s.cells[30] = Cell { rank: RANK_MINE, side: SIDE_RED, revealed: true };
        s.turn = SIDE_BLUE;
        s.moves_since_eat = 5;
        let ev = try_move(&mut s, 25, 30).unwrap();
        assert_eq!(ev, Ev::Dead, "普通子撞雷送死=EV_DEAD");
        assert_eq!(s.moves_since_eat, 5, "撞雷送死 mse 不变");
        assert_eq!(s.cells[30].rank, RANK_MINE, "雷不消失");
    }

    // §4.2 计步真值表（S2）：普通送死（源弱被吃）mse 清零
    #[test]
    fn settle_normal_dead_mse_zero() {
        let mut s = State::new();
        s.mode = MODE_LAY;
        s.cfg[4] = 0;
        s.cells[25] = Cell { rank: Rank(8), side: SIDE_BLUE, revealed: true };
        s.cells[30] = Cell { rank: Rank(1), side: SIDE_RED, revealed: true };
        s.turn = SIDE_BLUE;
        s.moves_since_eat = 7;
        let ev = try_move(&mut s, 25, 30).unwrap();
        assert_eq!(ev, Ev::Dead, "源弱被吃=EV_DEAD");
        assert_eq!(s.moves_since_eat, 0, "普通送死 mse 清零");
    }

    // §5.3 开局初始化（S8）：明棋红方 +0xc 编码
    #[test]
    fn init_mingqi_red_encoded() {
        let s = State::new();
        let mut s2 = State::new();
        s2.cells[30] = Cell { rank: Rank(15), side: SIDE_RED, revealed: true }; // 红 rank3 编码 3+0xc
        assert_eq!(rank_of(&s2.cells[30]), Rank(3), "明棋红方 rank+0xc 还原");
        assert_eq!(s.turn, SIDE_BLUE, "开局蓝先手");
        assert_eq!(s.stage, crate::state::Stage::Play);
        assert_eq!(s.winner, Winner::None);
    }

    // §5.3 暗棋开局全 0xFF（先手翻子定色）
    #[test]
    fn init_flip_all_unknown() {
        let mut s = State::new();
        s.mode = crate::state::MODE_FLIP;
        s.cfg[4] = 1;
        s.cells[25] = Cell { rank: Rank(3), side: SIDE_UNKNOWN, revealed: false };
        s.cells[30] = Cell { rank: Rank(5), side: SIDE_UNKNOWN, revealed: false };
        assert_eq!(s.cells[25].side, SIDE_UNKNOWN, "暗棋开局未定色 0xFF");
        assert!(flip(&mut s, 25));
        assert_eq!(s.cells[25].side, SIDE_BLUE, "蓝首翻定色蓝");
    }
}
