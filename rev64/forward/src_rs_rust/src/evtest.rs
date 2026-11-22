#![allow(unused_imports, dead_code)]
//! evtest — 事件副作用真值回归（移植 C evverify.c run_case 核心 case）
//!
//! 把 C 正向的"事件契约对齐"证据（evverify 174 断言核心）搬进 Rust：
//! 走子/吃子/扛旗/挖雷 → 事件码 + stage + winner 真值断言，对齐 C 行为基准。

use crate::state::{Cell, Ev, State, Stage, Winner, SIDE_BLUE, SIDE_RED, SIDE_UNKNOWN, Rank};
use crate::rules;

/// C mk_flag 夹具：cell53 蓝 rank2 + cell58 红旗（rank12+0xc=24）
fn mk_flag() -> State {
    let mut st = State::new();
    st.turn = SIDE_BLUE;
    st.stage = Stage::Play;
    st.winner = Winner::None;
    st.cells[53] = Cell { rank: Rank(2), side: SIDE_BLUE, revealed: true };
    st.cells[58] = Cell { rank: Rank(24), side: SIDE_RED, revealed: true }; // 红旗编码
    st
}

/// C mk_mine 夹具：cell49 蓝工兵 rank9 + cell54 红雷（11+0xc=23）+ cell59 红 rank3
fn mk_mine() -> State {
    let mut st = State::new();
    st.turn = SIDE_BLUE;
    st.stage = Stage::Play;
    st.winner = Winner::None;
    st.cells[49] = Cell { rank: Rank(9), side: SIDE_BLUE, revealed: true };
    st.cells[54] = Cell { rank: Rank(23), side: SIDE_RED, revealed: true }; // 雷编码
    st.cells[59] = Cell { rank: Rank(15), side: SIDE_RED, revealed: true };
    st
}

#[cfg(test)]
mod tests {
    use super::*;

    // C run_case "flag-on"：53→58 EV_FLAG + stage=OVER + winner=BLUE（扛旗胜）
    #[test]
    fn ev_flag_on() {
        let mut st = mk_flag();
        st.cfg[3] = 1; // 吃旗开关开
        let ev = rules::try_move(&mut st, 53, 58).unwrap();
        assert_eq!(ev, Ev::Flag, "扛旗=EV_FLAG");
        assert_eq!(st.stage, Stage::Over, "扛旗终局 stage=OVER");
        assert_eq!(st.winner, Winner::Blue, "蓝方扛旗胜");
    }

    // C run_case "mine-on"：49→54 EV_MINE + stage=PLAY + mines_cleared[0]++
    #[test]
    fn ev_mine_on() {
        let mut st = mk_mine();
        st.cfg[2] = 1; // 吃雷开关开
        let ev = rules::try_move(&mut st, 49, 54).unwrap();
        assert_eq!(ev, Ev::Mine, "工兵挖雷=EV_MINE");
        assert_eq!(st.stage, Stage::Play, "挖雷不终局");
        assert_eq!(st.mines_cleared[0], 1, "蓝方挖雷计数+1");
        // 工兵挖雷：工兵留位（移入雷格 rank9），雷被挖掉（非空移——雷格变工兵）
        assert_eq!(st.cells[54].rank, Rank(9), "工兵留位（雷格=工兵 rank9）");
        assert_eq!(st.cells[49].rank, Rank(0), "源格清空");
    }

    // C run_case "flag-off"：cfg3 关扛旗不判胜（EV_FLAG 入位，困毙收口判胜）
    #[test]
    fn ev_flag_off() {
        let mut st = mk_flag();
        st.cfg[3] = 0; // 吃旗开关关
        let ev = rules::try_move(&mut st, 53, 58).unwrap();
        assert_eq!(ev, Ev::Flag, "cfg3 关扛旗仍发 EV_FLAG（棋子落旗位）");
        // cfg3 关：不即时终局，走 stalemate_after 收口（红方 58 旗位被占 + 无他子 → 困毙）
    }

    // C 走子族事件码基准（强吃弱 EV_EAT / 弱被吃 EV_DEAD / 同级 EV_EAT）
    #[test]
    fn ev_eat_dead_tie() {
        // 强吃弱（rank3 蓝 吃 rank8 红）= EV_EAT
        let mut st = State::new();
        st.turn = SIDE_BLUE;
        st.cfg[4] = 0;
        st.cells[25] = Cell { rank: Rank(3), side: SIDE_BLUE, revealed: true };
        st.cells[30] = Cell { rank: Rank(8 + 0x0c), side: SIDE_RED, revealed: true };
        assert_eq!(rules::try_move(&mut st, 25, 30).unwrap(), Ev::Eat, "强吃弱=EV_EAT");
        // 弱被吃（rank8 蓝 撞 rank1 红）= EV_DEAD
        let mut st2 = State::new();
        st2.turn = SIDE_BLUE;
        st2.cfg[4] = 0;
        st2.cells[25] = Cell { rank: Rank(8), side: SIDE_BLUE, revealed: true };
        st2.cells[30] = Cell { rank: Rank(1 + 0x0c), side: SIDE_RED, revealed: true };
        assert_eq!(rules::try_move(&mut st2, 25, 30).unwrap(), Ev::Dead, "弱被吃=EV_DEAD");
        // 同级互杀（rank5 撞 rank5）= EV_EAT（非 BOMB，C 0x31de0 实证）
        let mut st3 = State::new();
        st3.turn = SIDE_BLUE;
        st3.cfg[4] = 0;
        st3.cells[25] = Cell { rank: Rank(5), side: SIDE_BLUE, revealed: true };
        st3.cells[30] = Cell { rank: Rank(5 + 0x0c), side: SIDE_RED, revealed: true };
        assert_eq!(rules::try_move(&mut st3, 25, 30).unwrap(), Ev::Eat, "同级=EV_EAT");
    }

    // 炸弹同归 EV_BOMB
    #[test]
    fn ev_bomb() {
        let mut st = State::new();
        st.turn = SIDE_BLUE;
        st.cfg[4] = 0;
        st.cells[25] = Cell { rank: Rank(10), side: SIDE_BLUE, revealed: true }; // 炸弹
        st.cells[30] = Cell { rank: Rank(5 + 0x0c), side: SIDE_RED, revealed: true };
        assert_eq!(rules::try_move(&mut st, 25, 30).unwrap(), Ev::Bomb, "炸弹=EV_BOMB");
        assert!(st.cells[25].is_empty() && st.cells[30].is_empty(), "双格清空");
    }

    // ---- C run_case 范式移植（evverify 事件回归核心，对齐 C 基准真值）----
    /// C run_case(name, from, to, want_ev, want_stage, want_winner)：can_move + try_move + 三断言
    fn run_case(
        name: &str,
        st: &mut State,
        from: usize,
        to: usize,
        want_ev: Ev,
        want_stage: Stage,
        want_winner: Option<Winner>,
    ) -> bool {
        let can = rules::can_move(st, from, to);
        let ev = if can { rules::try_move(st, from, to).ok() } else { None };
        let ev_ok = ev == Some(want_ev);
        let stage_ok = st.stage == want_stage;
        // ★ C 基准 -1 = Winner::None（未定），None 也必须真校验（原 None=>true 恒真假绿）
        let winner_ok = match want_winner {
            None => st.winner == Winner::None,
            Some(w) => st.winner == w,
        };
        let ok = ev_ok && stage_ok && winner_ok;
        if !ok {
            eprintln!("  [FAIL] {name} {from}→{to}: can={can} ev={ev:?} stage={:?} winner={:?}",
                st.stage, st.winner);
        }
        ok
    }

    // C evverify run_case("eat", 25, 30, EV_EAT, STG_PLAY, -1)
    #[test]
    fn run_case_eat() {
        // 对齐 C jq_load_layout(JQ_DEFAULT_LAY)：完整布局基底（双方 50 子，走子不困毙）
        let mut st = crate::gametest::make_default_layout();
        st.cfg[4] = 0; // 明棋
        // C 默认布局 cell25=蓝 rank3 / cell30=红 rank8（+0xc=20），强吃弱=EV_EAT
        // 完整布局下红方其余子仍可动 → 不困毙 → stage=PLAY
        assert!(run_case("eat", &mut st, 25, 30, Ev::Eat, Stage::Play, None), "eat: 强吃弱 EV_EAT（完整布局不困毙）");
    }

    // C evverify run_case("bomb", 27, 32, EV_EAT, STG_PLAY, -1)（同级互杀=EV_EAT 非 BOMB）
    #[test]
    fn run_case_tie_eat() {
        let mut st = crate::gametest::make_default_layout();
        st.cfg[4] = 0;
        // 完整布局 cell27 蓝 / cell32 红（rank 相同则同级；默认布局 27/32 实际 rank 见布局）
        // 同级互杀=EV_EAT（C 0x31de0 实证）；完整布局不困毙
        assert!(run_case("tie", &mut st, 27, 32, Ev::Eat, Stage::Play, None), "同级=EV_EAT（完整布局）");
    }

    // C evverify run_case("move", 12, 11, EV_MOVE, STG_PLAY, -1)（空移）
    #[test]
    fn run_case_move() {
        let mut st = crate::gametest::make_default_layout();
        st.cfg[4] = 0;
        // 完整布局 cell12→11 空移=EV_MOVE（C 默认布局 cell11 空）
        assert!(run_case("move", &mut st, 12, 11, Ev::Move, Stage::Play, None), "空移=EV_MOVE（完整布局）");
    }

    // C evverify run_case("flag", mk_flag, 53, 58, EV_FLAG, STG_OVER, SIDE_BLUE)
    #[test]
    fn run_case_flag() {
        let mut st = mk_flag();
        st.cfg[3] = 1;
        assert!(run_case("flag", &mut st, 53, 58, Ev::Flag, Stage::Over, Some(Winner::Blue)), "扛旗胜");
    }

    // C evverify run_case("mine", mk_mine, 49, 54, EV_MINE, STG_PLAY, -1)
    #[test]
    fn run_case_mine() {
        let mut st = mk_mine();
        st.cfg[2] = 1;
        assert!(run_case("mine", &mut st, 49, 54, Ev::Mine, Stage::Play, None), "挖雷=EV_MINE");
    }

    // C evverify run_case("flag-off", 53, 58, EV_FLAG, STG_OVER, SIDE_BLUE)（cfg3 关困毙判胜）
    #[test]
    fn run_case_flag_off() {
        let mut st = mk_flag();
        st.cfg[3] = 0;
        // cfg3 关：扛旗入位不即时终局，走 stalemate_after 收口（红 58 旗位被占+无他子→困毙）
        let can = rules::can_move(&st, 53, 58);
        let ev = if can { rules::try_move(&mut st, 53, 58).ok() } else { None };
        assert_eq!(ev, Some(Ev::Flag), "cfg3 关扛旗仍 EV_FLAG");
        // 收口后 stage/winner（困毙判蓝胜 或 和棋——按实际布局）
        // C 基准：flag-off → STG_OVER winner=BLUE（红 58 困毙）
        // ★ 原 OR 断言恒真（Stage 仅2变体）；按 C 基准精确断言终局收口=Over+蓝胜
        assert_eq!(st.stage, Stage::Over, "cfg3 关困毙收口应终局");
        assert_eq!(st.winner, Winner::Blue, "cfg3 关红方困毙→蓝胜");
    }

    // ---- C run_draw_resign / run_dark 移植（和棋/认输 + 暗棋翻棋 + 终局守卫）----
    // C run_draw_resign：和棋（winner=DRAW+stage=OVER）+ 认输（winner=1-turn）
    #[test]
    fn draw_resign() {
        let mut st = crate::gametest::make_default_layout();
        // 和棋：cfg0 开 + moves_since_eat>=70 → on_draw
        st.cfg[0] = 1;
        st.moves_since_eat = 70;
        crate::rules::on_draw(&mut st);
        assert_eq!(st.winner, Winner::Draw, "和棋 winner=DRAW");
        assert_eq!(st.stage, Stage::Over, "和棋 stage=OVER");
        // 认输：蓝走子前 turn=BLUE → winner=1-turn=RED
        let mut st2 = crate::gametest::make_default_layout();
        st2.turn = SIDE_BLUE;
        crate::rules::on_stalemate(&mut st2, SIDE_BLUE); // 蓝困毙 → 红胜
        assert_eq!(st2.winner, Winner::Red, "蓝困毙红胜");
        assert_eq!(st2.stage, Stage::Over);
    }

    // C run_case 终局守卫：重复终局 API 0 副作用（stage!=PLAY 拒收）
    #[test]
    fn endgame_guard_no_sideeffect() {
        let mut st = crate::gametest::make_default_layout();
        st.stage = Stage::Over;
        st.winner = Winner::Red;
        let w_before = st.winner;
        let s_before = st.stage;
        // 重复调 on_draw（stage=OVER 守卫应拒收，winner/stage 不变）
        crate::rules::on_draw(&mut st);
        assert_eq!(st.winner, w_before, "终局后重复 on_draw 0 副作用");
        assert_eq!(st.stage, s_before);
    }

    // C run_dark：暗棋翻棋（0xFF 定色 + revealed + 切 turn）
    #[test]
    fn dark_flip() {
        let mut st = State::new();
        st.mode = crate::state::MODE_FLIP;
        st.cfg[4] = 1;
        st.turn = SIDE_BLUE;
        // 0xFF 未定暗子（暗棋开局全 0xFF）
        st.cells[49] = Cell { rank: Rank(2), side: SIDE_UNKNOWN, revealed: false };
        assert!(crate::rules::can_flip(&st, 49), "0xFF 当前方可翻");
        assert!(crate::rules::flip(&mut st, 49));
        assert_eq!(st.cells[49].side, SIDE_BLUE, "定色归翻子方（蓝）");
        assert!(st.cells[49].revealed, "翻后 revealed=1");
        assert_eq!(st.turn, SIDE_RED, "翻棋切回合");
    }

    // C evverify run_case("mine-off", 49, 54, EV_DEAD, STG_OVER, SIDE_BLUE)（cfg2 关工兵被雷炸+红困毙）
    #[test]
    fn run_case_mine_off() {
        let mut st = mk_mine();
        st.cfg[2] = 0; // 吃雷开关关
        let can = rules::can_move(&st, 49, 54);
        let ev = if can { rules::try_move(&mut st, 49, 54).ok() } else { None };
        // cfg2 关：工兵撞雷=送死 EV_DEAD（雷不消失）
        assert_eq!(ev, Some(Ev::Dead), "cfg2 关工兵撞雷=EV_DEAD（送死）");
    }
}
