//! evt — 事件契约（规格 §11：7 事件码 + 名级映射 + 码/名分流统计）
//!
//! 对齐 C evt55_dispatch.c jq_evt55_count_7code + junqi.h JQ_EVENT_NAMES[7]：
//!   事件码 0-6 → 事件名级（音效按名匹配 14 WAV）
//!   事件码     → 统计计数（码分流，EV_MINE 与 EV_MOVE 同名但码不同，计数按码）
//!
//! 关键：EV_MINE(5) 名级 = "Junqi.Move"（与 EV_MOVE(0) 同名不同码）；
//! EV_FLIP(6) 名级 = "Junqi.Flip"。分发成功 ≠ 业务实现（部分事件为分发桩）。

use crate::state::Ev;

/// 7 事件名级映射（规格 §11，对齐 C ev7_name[7]）
/// 索引 = 事件码值 0-6
pub const EV_NAME: [&str; 7] = [
    "Junqi.Move",   // EV_MOVE=0
    "Junqi.Eat",    // EV_EAT=1
    "Junqi.Bomb",   // EV_BOMB=2
    "Junqi.Flag",   // EV_FLAG=3
    "Junqi.Dead",   // EV_DEAD=4
    "Junqi.Move",   // EV_MINE=5（名级与 EV_MOVE 同，码不同）
    "Junqi.Flip",   // EV_FLIP=6
];

/// 事件码 → 事件名级（音效分流用）
pub fn ev_name(ev: Ev) -> &'static str {
    EV_NAME[ev.as_u8() as usize]
}

impl Ev {
    /// 事件码值（0-6，对齐 C EV_* 枚举序）
    pub fn as_u8(self) -> u8 {
        match self {
            Ev::Move => 0,
            Ev::Eat => 1,
            Ev::Bomb => 2,
            Ev::Flag => 3,
            Ev::Dead => 4,
            Ev::Mine => 5,
            Ev::Flip => 6,
        }
    }

    /// 事件码值 → Ev（越界返 None）
    pub fn from_u8(v: u8) -> Option<Ev> {
        match v {
            0 => Some(Ev::Move),
            1 => Some(Ev::Eat),
            2 => Some(Ev::Bomb),
            3 => Some(Ev::Flag),
            4 => Some(Ev::Dead),
            5 => Some(Ev::Mine),
            6 => Some(Ev::Flip),
            _ => None,
        }
    }
}

/// 事件统计器（规格 §11：码分流计数 + 总计数）
pub struct EventCounter {
    /// 7 码计数 [EV_MOVE..EV_FLIP]
    pub by_code: [u64; 7],
    /// 总事件数
    pub total: u64,
}

impl Default for EventCounter {
    fn default() -> Self {
        Self { by_code: [0; 7], total: 0 }
    }
}

impl EventCounter {
    pub fn new() -> Self {
        Self::default()
    }

    /// 记录一个事件（码分流 + 总计数）
    pub fn record(&mut self, ev: Ev) {
        let i = ev.as_u8() as usize;
        self.by_code[i] += 1;
        self.total += 1;
    }

    /// 某事件码计数（如统计挖雷次数 = by_code[EV_MINE]）
    pub fn count(&self, ev: Ev) -> u64 {
        self.by_code[ev.as_u8() as usize]
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // §11 事件名级映射（EV_MINE 名级=Junqi.Move 与 EV_MOVE 同名）
    #[test]
    fn ev_name_mapping() {
        assert_eq!(ev_name(Ev::Move), "Junqi.Move");
        assert_eq!(ev_name(Ev::Eat), "Junqi.Eat");
        assert_eq!(ev_name(Ev::Bomb), "Junqi.Bomb");
        assert_eq!(ev_name(Ev::Flag), "Junqi.Flag");
        assert_eq!(ev_name(Ev::Dead), "Junqi.Dead");
        assert_eq!(ev_name(Ev::Mine), "Junqi.Move", "EV_MINE 名级=Junqi.Move（与 MOVE 同名）");
        assert_eq!(ev_name(Ev::Flip), "Junqi.Flip");
    }

    // §11 事件码值
    #[test]
    fn ev_code_values() {
        assert_eq!(Ev::Move.as_u8(), 0);
        assert_eq!(Ev::Eat.as_u8(), 1);
        assert_eq!(Ev::Bomb.as_u8(), 2);
        assert_eq!(Ev::Flag.as_u8(), 3);
        assert_eq!(Ev::Dead.as_u8(), 4);
        assert_eq!(Ev::Mine.as_u8(), 5);
        assert_eq!(Ev::Flip.as_u8(), 6);
        assert_eq!(Ev::from_u8(5), Some(Ev::Mine));
        assert_eq!(Ev::from_u8(7), None, "越界返 None");
    }

    // §11 统计器（码分流 + 总数）
    #[test]
    fn counter_by_code() {
        let mut c = EventCounter::new();
        c.record(Ev::Move);
        c.record(Ev::Move);
        c.record(Ev::Mine); // 挖雷（码 5，名 Junqi.Move 但码独立）
        c.record(Ev::Eat);
        assert_eq!(c.count(Ev::Move), 2, "EV_MOVE 计数 2");
        assert_eq!(c.count(Ev::Mine), 1, "EV_MINE 计数 1（与 MOVE 码分流）");
        assert_eq!(c.count(Ev::Eat), 1);
        assert_eq!(c.total, 4, "总数 4");
    }
}
