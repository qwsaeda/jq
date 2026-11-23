//! ai — 人机对战 AI 贪心决策（规格 §9）
//!
//! 评分表（ai_score）+ 暗棋不透视语义。对齐 C rules.c ai_score（§9.2 评分表）。

use crate::board::N_CELLS;
use crate::rules;
use crate::state::{rank_of, RANK_BOMB, RANK_EMPTY, RANK_FLAG, RANK_MINE, RANK_TOP, RANK_WORKER, SIDE_RED, SIDE_UNKNOWN, State};

/// 阵营感知 rank 还原（裸 rank_norm：>24 钳 RANK_TOP；>12 还原 -0xc）
fn rank_norm(v: i32) -> i32 {
    if v > RANK_TOP.0 as i32 + 0x0c {
        RANK_TOP.0 as i32
    } else if v > RANK_TOP.0 as i32 {
        v - 0x0c
    } else {
        v
    }
}

/// AI 决策结果（走子或翻棋）
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Move {
    pub from: usize,
    pub to: usize,
    pub is_flip: bool,
}

/// 贪心单步决策（规格 §9.1：遍历所有合法 (from,to) 取 ai_score 最高分者）。
///
/// 参数：
///   - `st`：当前对局状态（turn 须为 AI 方=RED，调用方守卫，§9.3 门控）
///   - `level`：难度（1=新手区弱/被动游走，≥2=贪心最优；对齐 C ui_opt_get_level）
///
/// 返回：
///   - `Some(Move)`：选中走子（is_flip=false）或翻棋（is_flip=true, from==to）
///   - `None`：无合法着法（困局兜底，调用方判困毙）
///
/// 语义（对齐 C jni_bridge.c ai_pick:309）：
///   ① 走子候选：遍历 AI 方已翻子（side==me 且 revealed），跳过旗/雷源，
///      对每个合法目标（rules::can_move + 暗棋未翻暗子目标过滤）算 ai_score，
///      取最高分（bs）；新手区额外记"sc>=2 取最小"（ws）被动游走。
///   ② 难度分流：level==1 → 新手区优先 ws（被动），无则 bs 兜底；level>=2 → 贪心 bs。
///   ③ 无走子 → 翻棋候选（遍历未翻子，接受 0xFF 定色归 me + 己方暗子，只排军旗）。
pub fn ai_pick(st: &State, level: u8) -> Option<Move> {
    let me = st.turn; // 调用方须传 AI 方（RED）
    let mut bf = None; // 贪心最优走子 (from, to)
    let mut bs: i32 = -1;
    let mut wf = None; // 新手区被动（sc>=2 取最小）
    let mut ws: i32 = 999;

    for i in 0..N_CELLS {
        let c = &st.cells[i];
        if c.rank == RANK_EMPTY || c.side != me || !c.revealed {
            continue;
        }
        let r = i32::from(rank_of(c));
        if r == RANK_FLAG.0 as i32 || r == RANK_MINE.0 as i32 {
            continue; // 旗/雷源不可走
        }
        for t in 0..N_CELLS {
            if t == i {
                continue;
            }
            if !rules::can_move(st, i, t) {
                continue;
            }
            let d = &st.cells[t];
            // 暗棋：未翻暗子目标不入候选（与 move_legal 拦截语义一致，双重保险）
            if st.mode == crate::state::MODE_FLIP && d.rank != RANK_EMPTY && !d.revealed {
                continue;
            }
            let sc = match ai_score(c.rank.0 as i32, d.rank.0 as i32, d.revealed) {
                Some(v) => v,
                None => continue, // 脏输入/不可走源（原 -2/-1 哨兵）跳过，不污染
            };
            if sc > bs {
                bs = sc;
                bf = Some((i, t));
            }
            if sc >= 2 && sc < ws {
                ws = sc;
                wf = Some((i, t));
            }
        }
    }

    // 难度分流
    if level == 1 {
        if let Some((f, t)) = wf {
            return Some(Move { from: f, to: t, is_flip: false });
        }
        if let Some((f, t)) = bf {
            if bs >= 1 {
                return Some(Move { from: f, to: t, is_flip: false });
            }
        }
    } else if let Some((f, t)) = bf {
        if bs >= 1 {
            return Some(Move { from: f, to: t, is_flip: false });
        }
    }

    // 无走子 → 翻棋候选（接受 0xFF 定色归 me + 己方暗子，只排军旗）
    for i in 0..N_CELLS {
        let c = &st.cells[i];
        if c.rank == RANK_EMPTY || c.revealed {
            continue;
        }
        let r = rank_of(c);
        if r == RANK_FLAG {
            continue; // 军旗不可翻（地雷可翻，与 can_flip/stalemate 白名单一致）
        }
        if c.side != me && c.side != SIDE_UNKNOWN {
            continue;
        }
        if rules::can_flip(st, i) {
            return Some(Move { from: i, to: i, is_flip: true });
        }
    }

    // 空移兜底
    if let Some((f, t)) = bf {
        return Some(Move { from: f, to: t, is_flip: false });
    }
    None
}

/// AI 介入门控（规格 §9.3：仅人机 vs_ai 且 AI 方回合且 PLAY 阶段）
pub fn ai_enabled(st: &State, vs_ai: bool) -> bool {
    st.turn == SIDE_RED
        && matches!(st.stage, crate::state::Stage::Play)
        && vs_ai
}

/// 贪心单步评分（规格 §9.2）。
///
/// 返回分数：
///   +1000 吃子（src 强压 dst 弱，含炸弹撞旗 10<12）
///    +900 同级互杀（含炸弹撞炸弹）
///    +800 炸弹同归（炸弹撞普通子/炸弹撞雷）
///    +700 工兵挖雷
///       2 空移 / 暗棋未翻暗子（不透视）
///       1 送死（src 弱碰 dst 强 / 普通子撞雷）
///      -1 军旗/地雷源（不可走）
///      -2 脏输入（负 rank / src>24 / 空源）
///
/// 暗棋不透视：dst_rank 非空 且 dst_revealed=0 → 恒给 2（不按真 rank 透视敌子）。
pub fn ai_score(src_rank: i32, dst_rank: i32, dst_revealed: bool) -> Option<i32> {
    // 脏输入拦截（规格 §9.2 哨兵 -2）
    if src_rank < 0 || dst_rank < 0 {
        return None;
    }
    if src_rank > 24 {
        return None;
    }
    // ★ RRULE-009 修复：dst_rank 上界守卫（与 src 对称）。save.rs:153 契约 rank∈[0,24]，
    //   脏目标 rank（存档/BT 注入 >24，如 255）此前经 rank_norm 钳到 12 被当军旗评 1000 分，
    //   AI 基于损坏数据决策。正常对局 rank≤24 不受影响。
    if dst_rank > 24 {
        return None;
    }
    if src_rank == 0 {
        return None; // 空源哨兵
    }
    let r = rank_norm(src_rank);
    // 军旗/地雷源不可走
    if r == RANK_FLAG.0 as i32 || r == RANK_MINE.0 as i32 {
        return None;
    }
    // 暗棋未翻暗子目标：不透视，恒 2
    if dst_rank != 0 && !dst_revealed {
        return Some(2);
    }
    // 空格目标：空移 2
    if dst_rank == 0 {
        return Some(2);
    }
    // 有目标子（已翻开）
    let dr = rank_norm(dst_rank);
    if dr == RANK_MINE.0 as i32 {
        // 雷特判
        if r == RANK_WORKER.0 as i32 {
            return Some(700); // 工兵挖雷
        }
        if r == RANK_BOMB.0 as i32 {
            return Some(800); // 炸弹撞雷同归
        }
        return Some(1); // 普通子撞雷送死
    }
    // 吃子 / 同级 / 炸弹 / 送死
    if r < dr {
        return Some(1000); // 大压小（含炸弹撞旗）
    }
    if r == dr {
        return Some(900); // 同级互杀
    }
    if r == RANK_BOMB.0 as i32 {
        return Some(800); // 炸弹撞弱普通子
    }
    Some(1) // 送死
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::state::{Cell, Rank, MODE_LAY, SIDE_BLUE, SIDE_RED};

    // 规格 §9.2 评分表全档（对照 C ai_selftest 期望值）
    #[test]
    fn score_table() {
        // 明棋（revealed=true）
        assert_eq!(ai_score(3, 8, true), Some(1000), "红 rank3 吃蓝 rank8");
        assert_eq!(ai_score(6, 6, true), Some(900), "同级互杀");
        assert_eq!(ai_score(RANK_BOMB.0 as i32, 1, true), Some(800), "炸弹撞普通子");
        assert_eq!(ai_score(RANK_WORKER.0 as i32, RANK_MINE.0 as i32, true), Some(700), "工兵挖雷");
        assert_eq!(ai_score(2, 1, true), Some(1), "rank2 撞 rank1 送死");
        assert_eq!(ai_score(2, RANK_MINE.0 as i32, true), Some(1), "普通子撞雷送死");
        assert_eq!(ai_score(RANK_BOMB.0 as i32, RANK_MINE.0 as i32, true), Some(800), "炸弹撞雷");
        assert_eq!(ai_score(3, 0, true), Some(2), "空移");
        assert_eq!(ai_score(12, 0, true), None, "军旗源不可走");
        assert_eq!(ai_score(RANK_MINE.0 as i32, 0, true), None, "地雷源不可走");
        // 暗棋不透视
        assert_eq!(ai_score(3, 8, false), Some(2), "暗棋未翻暗子不透视");
        assert_eq!(ai_score(6, 6, false), Some(2), "暗棋未翻同级不透视");
        assert_eq!(ai_score(RANK_BOMB.0 as i32, 8, false), Some(2), "暗棋未翻炸弹不透视");
        // 炸弹撞旗（10 < 12 → 1000 吃子档）
        assert_eq!(ai_score(RANK_BOMB.0 as i32, RANK_FLAG.0 as i32, true), Some(1000), "炸弹撞旗=大压小 1000");
    }

    // 规格 §9.2 脏输入哨兵
    #[test]
    fn score_dirty_input() {
        assert_eq!(ai_score(-1, 8, true), None, "负 src");
        assert_eq!(ai_score(3, -1, true), None, "负 dst");
        assert_eq!(ai_score(0, 8, true), None, "空源");
        assert_eq!(ai_score(25, 8, true), None, "src>24 脏");
    }

    // 规格 §9.1 ai_pick 贪心：明棋 AI 主动吃子（rank 大压小优先）
    #[test]
    fn pick_greedy_eat() {
        let mut s = State::new();
        s.mode = MODE_LAY;
        s.cfg[4] = 0; // 明棋
        // AI=RED，蓝 rank1(强) 在 30，红 rank5(弱) 在 35，红可走 35→? 吃蓝 30
        // 设蓝 30=rank1 revealed，红 35=rank5 revealed，30/35 同列相邻（(6,0)/(7,0)）
        s.cells[30] = Cell { rank: Rank(1), side: SIDE_BLUE, revealed: true };
        s.cells[35] = Cell { rank: Rank(5), side: SIDE_RED, revealed: true };
        s.turn = SIDE_RED;
        let mv = ai_pick(&s, 3).unwrap();
        // 红 35(rank5) 吃蓝 30(rank1)：5>1 送死档 1；但若有更优（吃更强子）应优先
        assert_eq!(mv.from, 35, "AI 走红 35");
        assert!(!mv.is_flip);
    }

    // 规格 §9.3 ai_enabled 门控（非 AI 方/非 PLAY/vs_ai=0 不介入）
    #[test]
    fn ai_enabled_gate() {
        let mut s = State::new();
        s.turn = SIDE_RED;
        assert!(ai_enabled(&s, true), "AI 方+PLAY+vs_ai=1 介入");
        s.turn = SIDE_BLUE;
        assert!(!ai_enabled(&s, true), "非 AI 方不介入");
        s.turn = SIDE_RED;
        s.stage = crate::state::Stage::Over;
        assert!(!ai_enabled(&s, true), "终局不介入");
    }
}
