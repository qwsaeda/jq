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
#[derive(Default)]
pub struct EventCounter {
    /// 7 码计数 [EV_MOVE..EV_FLIP]
    pub by_code: [u64; 7],
    /// 总事件数
    pub total: u64,
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

/// ★ 原版 73 事件名表（业务真值，逐名对齐 C jq_event_names_73.c / 原版 .rodata）
/// 索引 = 73 名表索引（注意：与 7 码域 EV_* 的码值不同层）
pub const JQ_EVENT_NAMES_73: [&str; 73] = [
    "Junqi.Move", // [0]
    "Junqi.Eat", // [1]
    "Junqi.Bomb", // [2]
    "Junqi.Flag", // [3]
    "Junqi.Dead", // [4]
    "Junqi.Flip", // [5]
    "Junqi.Win", // [6]
    "Junqi.Lose", // [7]
    "Junqi.RedWin", // [8]
    "Junqi.BlueWin", // [9]
    "Junqi.Draw", // [10]
    "Junqi.GameOver", // [11]
    "Junqi.Over", // [12]
    "Junqi.DrawReq", // [13]
    "Junqi.DrawAgree", // [14]
    "Junqi.DrawDisagree", // [15]
    "Junqi.DrawConfirm", // [16]
    "Junqi.DrawSend", // [17]
    "Junqi.Resign", // [18]
    "Junqi.ResignConfirm", // [19]
    "Junqi.RegretReq", // [20]
    "Junqi.RegretAgree", // [21]
    "Junqi.RegretDisagree", // [22]
    "Junqi.RegretConfirm", // [23]
    "Junqi.RegretSend", // [24]
    "Junqi.RegretFail", // [25]
    "Junqi.InputUser", // [26]
    "Junqi.InputEmail", // [27]
    "Junqi.InputPassword", // [28]
    "Junqi.InputConfirmPwd", // [29]
    "Junqi.LoginFail", // [30]
    "Junqi.LoginAuthFail", // [31]
    "Junqi.LoginMaxUser", // [32]
    "Junqi.LoginVerOld", // [33]
    "Junqi.EmailError", // [34]
    "Junqi.EmailLenError", // [35]
    "Junqi.InputName", // [36]
    "Junqi.RegExist", // [37]
    "Junqi.RegFail", // [38]
    "Junqi.RegOk", // [39]
    "Junqi.UserCharError", // [40]
    "Junqi.UserLenError", // [41]
    "Junqi.NameCharError", // [42]
    "Junqi.NameLenError", // [43]
    "Junqi.PwdLenError", // [44]
    "Junqi.PwdNotMatch", // [45]
    "Junqi.NewFail", // [46]
    "Junqi.NewPwdLenError", // [47]
    "Junqi.NewPwdNotMatch", // [48]
    "Junqi.OriPwdLenError", // [49]
    "Junqi.JoinGame", // [50]
    "Junqi.Ready", // [51]
    "Junqi.Start", // [52]
    "Junqi.PeerStart", // [53]
    "Junqi.PeerFirst", // [54]
    "Junqi.PeerLast", // [55]
    "Junqi.PeerLayFinish", // [56]
    "Junqi.PeerWait", // [57]
    "Junqi.LeaveGame", // [58]
    "Junqi.PlayerLeave", // [59]
    "Junqi.Reconnecting", // [60]
    "Junqi.ReconnFail", // [61]
    "Junqi.ConnectFail", // [62]
    "Junqi.ExitFail", // [63]
    "Junqi.ExitConfirm", // [64]
    "Junqi.LayError", // [65]
    "Junqi.HighAreaFail", // [66]
    "Junqi.MidAreaFail", // [67]
    "Junqi.ZhaError", // [68]
    "Junqi.LeiError", // [69]
    "Junqi.QiError", // [70]
    "Junqi.Select", // [71]
    "Junqi.GameType%d", // [72] 原版 .rodata 真值（AUD-1000-ALL A4-04：截断版 strcmp 永不命中）
];

/// 按名查 73 表索引（未命中返 None）
pub fn name_to_73_index(name: &str) -> Option<usize> {
    JQ_EVENT_NAMES_73.iter().position(|&n| n == name)
}
#[cfg(test)]
mod names_73_tests {
    use super::*;
    #[test]
    fn names_73_count_and_key() {
        assert_eq!(JQ_EVENT_NAMES_73.len(), 73, "原版 73 名表完整");
        assert_eq!(JQ_EVENT_NAMES_73[0], "Junqi.Move");
        assert_eq!(JQ_EVENT_NAMES_73[5], "Junqi.Flip");
        assert_eq!(name_to_73_index("Junqi.BlueWin"), Some(9));
        assert_eq!(name_to_73_index("Junqi.NotExist"), None);
    }
}

/// ★ 原版事件名→音效 WAV 映射（业务真值，对齐 C evt55_sfx.c jq_evt55_sfx_for_name）
/// 7 码事件 → WAV 名；未知/空 → "move"（原版默认）
pub fn sfx_for_code(code: Ev) -> &'static str {
    match code {
        Ev::Move => "move",
        Ev::Eat => "eat",
        Ev::Bomb => "bomb",
        Ev::Flag => "flag",
        Ev::Dead => "dead",
        Ev::Mine => "move", // 挖雷名级 Junqi.Move → move 音效（原版同名）
        Ev::Flip => "flip",
    }
}

/// 原版事件名字符串→WAV 名（73 名表映射，终局类归并）
pub fn sfx_for_name(name: &str) -> &'static str {
    match name {
        "Junqi.Move" => "move",
        "Junqi.Eat" => "eat",
        "Junqi.Bomb" => "bomb",
        "Junqi.Flag" => "flag",
        "Junqi.Dead" => "dead",
        "Junqi.Flip" => "flip",
        "Junqi.Win" | "Junqi.RedWin" | "Junqi.BlueWin" => "win",
        "Junqi.Lose" | "Junqi.Resign" | "Junqi.ResignConfirm" => "lose",
        "Junqi.Draw" | "Junqi.GameOver" | "Junqi.Over" => "draw",
        _ => "move", // 原版默认
    }
}


#[cfg(test)]
mod sfx_tests {
    use super::*;
    #[test]
    fn sfx_mapping_matches_original() {
        assert_eq!(sfx_for_code(Ev::Move), "move");
        assert_eq!(sfx_for_code(Ev::Eat), "eat");
        assert_eq!(sfx_for_code(Ev::Bomb), "bomb");
        assert_eq!(sfx_for_code(Ev::Mine), "move"); // 挖雷同 move
        assert_eq!(sfx_for_name("Junqi.BlueWin"), "win");
        assert_eq!(sfx_for_name("Junqi.Draw"), "draw");
        assert_eq!(sfx_for_name("Junqi.Unknown"), "move"); // 默认
    }
}
