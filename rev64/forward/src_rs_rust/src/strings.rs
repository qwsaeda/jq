//! strings — 原版 UI 文案真值（对齐 C jq_strings.h，源自原版 .rodata GBK 字符串池）
//! 范围 = 人机对战相关文案；砍掉域（联网/账号/蓝牙）文案不纳入。
//! ★ 业务对齐原则：对齐文案内容（业务真值），不照搬 C 的字符串表实现。

// ---- 终局文案 ----
pub const BLUE_WIN: &str = "蓝方胜利！";   // JQ_STR_BlueWin
pub const RED_WIN: &str = "红方胜利！";    // JQ_STR_RedWin
pub const DRAW: &str = "和棋！";           // JQ_STR_Draw
pub const LOSE: &str = "失败！";           // JQ_STR_Lose
pub const GAME_OVER: &str = "游戏已结束";  // JQ_STR_GameOver

// ---- 玩法文案（难度弹窗模式标签）----
pub const GAME_TYPE_FLIP: &str = "翻棋 - 暗棋"; // JQ_STR_GameType1
pub const GAME_TYPE_LAY: &str = "布局 - 暗棋";  // JQ_STR_GameType3

// ---- 布局错误文案 ----
pub const LEI_ERROR: &str = "地雷只能放在最后两行"; // JQ_STR_LeiError
pub const QI_ERROR: &str = "军旗只能放在大本营";   // JQ_STR_QiError
pub const LAY_ERROR: &str = "还有未摆放的棋子";    // JQ_STR_LayError

// ---- 退出/悔棋确认 ----
pub const EXIT_CONFIRM: &str = "真的要退出吗？";     // JQ_STR_ExitConfirm
pub const REGRET_CONFIRM: &str = "您真的要悔棋吗？"; // JQ_STR_RegretConfirm

/// 终局结果 → 原版文案（业务对齐：事件结果映射到正确文案）
pub fn result_text(blue_win: bool, red_win: bool, draw: bool) -> &'static str {
    if draw { DRAW }
    else if blue_win { BLUE_WIN }
    else if red_win { RED_WIN }
    else { LOSE }
}

#[cfg(test)]
mod tests {
    use super::*;

    // 原版文案真值锚定（防误改）
    #[test]
    fn strings_match_original() {
        assert_eq!(BLUE_WIN, "蓝方胜利！");
        assert_eq!(RED_WIN, "红方胜利！");
        assert_eq!(DRAW, "和棋！");
        assert_eq!(GAME_TYPE_FLIP, "翻棋 - 暗棋");
        assert_eq!(QI_ERROR, "军旗只能放在大本营");
    }

    // 终局结果→文案映射
    #[test]
    fn result_text_mapping() {
        assert_eq!(result_text(true, false, false), BLUE_WIN);
        assert_eq!(result_text(false, true, false), RED_WIN);
        assert_eq!(result_text(false, false, true), DRAW);
    }
}
