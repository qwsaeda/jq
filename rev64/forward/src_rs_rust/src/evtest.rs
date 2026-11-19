//! evtest — 事件副作用真值回归（移植 C evverify.c run_case 核心 case）
//!
//! 把 C 正向的"事件契约对齐"证据（evverify 174 断言核心）搬进 Rust：
//! 走子/吃子/扛旗/挖雷 → 事件码 + stage + winner 真值断言，对齐 C 行为基准。

use crate::state::{Cell, Ev, State, Stage, Winner, SIDE_BLUE, SIDE_RED};
use crate::rules;

/// C mk_flag 夹具：cell53 蓝 rank2 + cell58 红旗（rank12+0xc=24）
fn mk_flag() -> State {
    let mut st = State::new();
    st.turn = SIDE_BLUE;
    st.stage = Stage::Play;
    st.winner = Winner::None;
    st.cells[53] = Cell { rank: 2, side: SIDE_BLUE, revealed: 1 };
    st.cells[58] = Cell { rank: 24, side: SIDE_RED, revealed: 1 }; // 红旗编码
    st
}

/// C mk_mine 夹具：cell49 蓝工兵 rank9 + cell54 红雷（11+0xc=23）+ cell59 红 rank3
fn mk_mine() -> State {
    let mut st = State::new();
    st.turn = SIDE_BLUE;
    st.stage = Stage::Play;
    st.winner = Winner::None;
    st.cells[49] = Cell { rank: 9, side: SIDE_BLUE, revealed: 1 };
    st.cells[54] = Cell { rank: 23, side: SIDE_RED, revealed: 1 }; // 雷编码
    st.cells[59] = Cell { rank: 15, side: SIDE_RED, revealed: 1 };
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
        assert_eq!(st.cells[54].rank, 9, "工兵留位（雷格=工兵 rank9）");
        assert_eq!(st.cells[49].rank, 0, "源格清空");
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
        st.cells[25] = Cell { rank: 3, side: SIDE_BLUE, revealed: 1 };
        st.cells[30] = Cell { rank: 8 + 0x0c, side: SIDE_RED, revealed: 1 };
        assert_eq!(rules::try_move(&mut st, 25, 30).unwrap(), Ev::Eat, "强吃弱=EV_EAT");
        // 弱被吃（rank8 蓝 撞 rank1 红）= EV_DEAD
        let mut st2 = State::new();
        st2.turn = SIDE_BLUE;
        st2.cfg[4] = 0;
        st2.cells[25] = Cell { rank: 8, side: SIDE_BLUE, revealed: 1 };
        st2.cells[30] = Cell { rank: 1 + 0x0c, side: SIDE_RED, revealed: 1 };
        assert_eq!(rules::try_move(&mut st2, 25, 30).unwrap(), Ev::Dead, "弱被吃=EV_DEAD");
        // 同级互杀（rank5 撞 rank5）= EV_EAT（非 BOMB，C 0x31de0 实证）
        let mut st3 = State::new();
        st3.turn = SIDE_BLUE;
        st3.cfg[4] = 0;
        st3.cells[25] = Cell { rank: 5, side: SIDE_BLUE, revealed: 1 };
        st3.cells[30] = Cell { rank: 5 + 0x0c, side: SIDE_RED, revealed: 1 };
        assert_eq!(rules::try_move(&mut st3, 25, 30).unwrap(), Ev::Eat, "同级=EV_EAT");
    }

    // 炸弹同归 EV_BOMB
    #[test]
    fn ev_bomb() {
        let mut st = State::new();
        st.turn = SIDE_BLUE;
        st.cfg[4] = 0;
        st.cells[25] = Cell { rank: 10, side: SIDE_BLUE, revealed: 1 }; // 炸弹
        st.cells[30] = Cell { rank: 5 + 0x0c, side: SIDE_RED, revealed: 1 };
        assert_eq!(rules::try_move(&mut st, 25, 30).unwrap(), Ev::Bomb, "炸弹=EV_BOMB");
        assert!(st.cells[25].is_empty() && st.cells[30].is_empty(), "双格清空");
    }
}
