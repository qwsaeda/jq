//! ui — 人机对战 UI 状态机（规格 §10，对齐 C ui_menu.h 核心屏态）
//!
//! 范围 = 人机对战核心屏态（砍掉蓝牙/四国/联网/账号）：
//!   MENU / GAME / INGAME_MENU / DIFFICULTY / NEWGAME / GAMESET / SETTINGS / EXIT_CONFIRM
//!
//! 设计：纯逻辑层（屏态迁移 + 触摸命中判定 + 面板外回滚通病）。
//! 几何（按钮 rect/面板尺寸）参数化进 `Ui` 结构体，渲染层（wgpu）消费；
//! 触摸 (x,y) → 命中判定 → 屏态迁移，可本机测试（不需 NDK/设备）。
//!
//! 规格 §10 关键不变量：
//!   - 菜单空白区 → 不消费（return false，防穿透到尾部游戏态机）
//!   - NEWGAME 取消/面板外 → 恢复切换前 mode（会话快照）
//!   - GAMESET 取消/面板外 → 恢复原 4 开关（ui_rules_load）
//!   - SETTINGS 取消/面板外 → 回滚（含 g_sound_on 双轨）
//!   - GAME 左上角重开 → stage==PLAY && UI_GAME 双守卫
//!   - 终局 OVER "再来一局" → restart

use crate::state::State;

// ---- 屏态（人机对战核心子集，对齐 C ui_menu.h 同名枚举）----
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Screen {
    Menu,      // 主菜单（人机/两人 2 按钮）
    Game,      // 对局（棋盘 + 触摸走子）
    IngameMenu, // ☰ 游戏内菜单（重新开始/悔棋/游戏规则/功能设置/软件信息/退出）
    Difficulty, // 人机难度弹窗（level × mode 8 按钮）
    NewGame,   // 两人新局弹窗（玩法/明暗棋 + 确定/取消）
    GameSet,   // 游戏设置弹窗（4 规则开关 + 确定/取消）
    Settings,  // 声音设置弹窗（音效开关 + 确定/取消）
    ExitConfirm, // 退出确认（退出/继续）
    Replay,    // 复盘回放（棋盘 + 底部控制条，规格 §7）
    Stats,     // 战况统计（☰ btn5：事件码计数可视化，规格 §11）
}

/// UI 状态机（人机对战）
pub struct Ui {
    pub screen: Screen,
    /// 人机对战标记（true=AI 介入红方；false=两人热座）
    pub vs_ai: bool,
    /// AI 难度（1 新手 / 2 中 / 3 高 / 4 好友）
    pub level: u8,
    /// 会话 mode 快照（NEWGAME 切换前保存，取消/面板外恢复，防全局 mode 残留）
    mode_snap: u8,
    /// 会话 4 规则开关快照（GAMESET 进弹窗前保存，取消/面板外恢复）
    cfg_snap: [u8; 5],
    /// 音效开关快照（SETTINGS 进弹窗前保存，取消/面板外恢复，双轨回滚）
    sound_snap: bool,
    /// ★ 审计 A9：进入弹窗前的原屏（取消/面板外返回原屏，非固定 Menu）
    screen_snap: Screen,
    /// ★ 审计 A9：独立音效开关状态（不再用 cfg[0] 伪装）
    pub sound_on: bool,
    /// 悔棋栈（规格 §6：整盘快照，走子/翻棋前 push）
    pub undo_stack: Vec<State>,
}

impl Ui {
    pub fn new() -> Self {
        Self {
            screen: Screen::Menu,
            vs_ai: false,
            level: 3,
            mode_snap: 0,
            cfg_snap: [1, 1, 1, 1, 1],
            sound_snap: true,
            screen_snap: Screen::Menu,
            sound_on: true,
            undo_stack: Vec::new(),
        }
    }

    /// 进入新局弹窗前快照（规格 §10：NEWGAME 取消/面板外恢复切换前 mode）
    pub fn begin_popup(&mut self, st: &State) {
        self.mode_snap = st.mode;
        self.cfg_snap = st.cfg;
        self.screen_snap = self.screen; // 记录进入弹窗前的原屏
        self.sound_snap = self.sound_on; // 独立音效态快照（不再用 cfg[0] 伪装）
    }

    /// 取消弹窗（恢复快照 + 回主菜单，规格 §10 面板外=取消语义）
    pub fn cancel_popup(&mut self, st: &mut State) {
        // 恢复会话快照（NEWGAME/GAMESET/SETTINGS 面板外=取消）
        st.mode = self.mode_snap;
        st.cfg = self.cfg_snap;
        self.sound_on = self.sound_snap; // 音效双轨回滚
        self.screen = self.screen_snap;  // 返回原屏（非固定 Menu）
    }

    /// 菜单"人机对战"→难度弹窗（规格 §10）
    pub fn menu_human(&mut self) {
        self.vs_ai = true;
        self.screen = Screen::Difficulty;
    }

    /// 菜单"两人对局"→新局弹窗（规格 §10）
    pub fn menu_two_player(&mut self) {
        self.vs_ai = false;
        self.screen = Screen::NewGame;
    }

/// 难度弹窗选（level, mode）→ 开局进对局（规格 §10：选完即开局）
    pub fn difficulty_select(&mut self, level: u8, mode: u8, st: &mut State) {
        // ★ P1 修复：换难度=开新局，必须清空上一局悔棋快照 + mode 快照，
        //   否则新局按 ↩ 会 undo 到上一局棋盘（整盘闪回错局）
        self.undo_stack.clear();
        self.mode_snap = mode;
        self.level = level;
        // ★ 业务对齐（规格 §5.3）：按 mode 真正重建开局棋盘
        //   mode=0 明棋（红+0xc/revealed=1）；mode=1 暗棋（全 0xFF/未翻/raw rank）
        let layout = [
            3u8,7,6,5,8, 10,0,9,0,1, 2,10,0,9,5,
            4,0,6,0,7, 8,3,4,9,11, 11,12,11,8,7,
        ];
        *st = crate::gametest::init_from_layout_30(&layout, mode);
        st.cfg = [1, 1, 1, 1, 1];
        st.turn = crate::state::SIDE_BLUE;
        st.stage = crate::state::Stage::Play;
        st.winner = crate::state::Winner::None;
        self.screen = Screen::Game;
    }

    /// 新局弹窗"确定"→ 开局进对局（两人热座，vs_ai=false）
    pub fn newgame_confirm(&mut self, st: &mut State) {
        // ★ P1 修复：两人新局同样清悔棋栈（防跨局 undo 污染）
        self.undo_stack.clear();
        st.turn = crate::state::SIDE_BLUE;
        st.stage = crate::state::Stage::Play;
        st.winner = crate::state::Winner::None;
        self.screen = Screen::Game;
    }

    /// 游戏设置"确定"→ 保存 4 开关（写入 st.cfg）+ 回对局（规格 §10）
    pub fn gameset_confirm(&mut self, _st: &mut State) {
        // st.cfg 已在弹窗内被用户切换（UI 层写），此处仅切屏
        self.screen = Screen::Game;
    }

    /// 声音设置"确定"→ 保存 + 回主菜单
    pub fn settings_confirm(&mut self, _st: &mut State) {
        self.screen = self.screen_snap; // 确定后回原屏
    }

    /// 走子/翻棋前 push 整盘快照（规格 §6）
    pub fn push_undo(&mut self, st: &State) {
        self.undo_stack.push(st.clone());
        if self.undo_stack.len() > 64 { self.undo_stack.remove(0); } // 容量上限
    }

    /// 悔棋：pop 快照 + 全字段恢复（规格 §6）
    pub fn undo(&mut self, st: &mut State) -> bool {
        if let Some(snap) = self.undo_stack.pop() {
            st.undo_restore(&snap);
            true
        } else { false }
    }

    /// 终局"再来一局"→ 重开（规格 §10：stage==OVER 区）
    /// ★ 对齐 C jq_restart_game：随当前 mode 重载布局（明棋 +0xc / 暗棋 0xFF），
    ///   旧实现只清空 cells → 重开是空棋盘，玩家无子可走（真 bug）。
    pub fn restart(&mut self, st: &mut State) {
        self.undo_stack.clear();
        let mode = st.mode;
        let layout = [
            3u8,7,6,5,8, 10,0,9,0,1, 2,10,0,9,5,
            4,0,6,0,7, 8,3,4,9,11, 11,12,11,8,7,
        ];
        *st = crate::gametest::init_from_layout_30(&layout, mode);
        st.cfg = [1, 1, 1, 1, 1];
        st.turn = crate::state::SIDE_BLUE;
        st.stage = crate::state::Stage::Play;
        st.winner = crate::state::Winner::None;
        self.screen = Screen::Game;
    }

    /// ☰ 游戏内菜单打开/关闭（对局态，规格 §10）
    pub fn open_ingame_menu(&mut self) {
        if self.screen == Screen::Game {
            self.screen = Screen::IngameMenu;
        }
    }

    /// 退出确认"退出"→ 回主菜单；"继续"→ 回对局
    pub fn exit_confirm(&mut self, quit: bool) {
        // ★ F6 修：取消退出回上层屏（若从 ☰ 菜单进入则回 IngameMenu），而非固定 Game
        self.screen = if quit {
            Screen::Menu
        } else if self.screen_snap == Screen::IngameMenu {
            Screen::IngameMenu
        } else {
            Screen::Game
        };
    }
}

/// ★ 原版 datah_041 难度弹窗真值（XML 450×335 居中 → 720 基准偏移 +135,+472.5）
/// 8 按钮：4 行(新手/中级/高级/好友区) × 2 列(布局/翻棋)，坐标已含偏移
pub const DIFF_BTNS_ORIG: [(i32, i32, i32, i32); 8] = [
(290, 497, 410, 557), (435, 497, 555, 557),  // 新手区 Lay/Flip
(290, 572, 410, 632), (435, 572, 555, 632),  // 中级区
(290, 647, 410, 707), (435, 647, 555, 707),  // 高级区
(290, 722, 410, 782), (435, 722, 555, 782),  // 好友区
];
/// 原版 datah_012 游戏设置真值（450×465 → 偏移 +135,+407.5）
/// 4 开关 / 确定 / 取消（720 基准）
pub const GAMESET_BTNS_ORIG: [(i32, i32, i32, i32); 6] = [
(345, 483, 555, 543),  // 和棋开关
(345, 558, 555, 618),  // 大本营
(345, 633, 555, 693),  // 吃地雷
(345, 708, 555, 768),  // 吃军旗
(165, 793, 345, 853),  // 确定
(375, 793, 555, 853),  // 取消
];
/// ★ 原版 datah_001 声音设置真值（面板 450×390 → 720 偏移 +135,+445）
/// 控件：Sound 220,75-420,135 / BtnSound 220,150-420,210 / Volume 120,232-427,280 /
///        Ok 30,305-210,365 / Cancel 240,305-420,365 —— 与 GameSet 几何不同（确定 y 750 vs 793）
pub const SETTINGS_BTNS_ORIG: [(i32, i32, i32, i32); 5] = [
(355, 520, 555, 580),  // 0=静音 Sound
(355, 595, 555, 655),  // 1=按键声音 BtnSound
(255, 677, 562, 725),  // 2=音量 Volume
(165, 750, 345, 810),  // 3=确定 Ok
(375, 750, 555, 810),  // 4=取消 Cancel
];
pub const SETTINGS_PANEL_ORIG: Rect = Rect { x: 135, y: 445, w: 450, h: 390 };
/// 原版 datah_062 两人新局真值（面板 450×315 → 720 偏移 +135,+482.5，取整 482）
/// 控件完整 rect（原版 XML）：Lay 220,75-420,135（玩法:/布局）、Show 220,150-420,210（明暗棋:）
///   Ok 30,235-210,295、Cancel 240,235-420,295 —— 两个玩法按钮是【同一列上下两行】非同一行
pub const NEWGAME_BTNS_ORIG: [(i32, i32, i32, i32); 4] = [
(165, 717, 345, 777),  // 0=确定 Ok
(375, 717, 555, 777),  // 1=取消 Cancel
(355, 557, 555, 617),  // 2=Lay 玩法（布局/明棋切换）
(355, 632, 555, 692),  // 3=Show 明暗棋
];


impl Default for Ui {
    fn default() -> Self {
        Self::new()
    }
}

// ---- 触摸命中 → 屏态迁移（几何参数化，渲染层消费）----

/// 命中区域（按钮/面板 rect，归一化 0-1 或像素，由调用方定标）
#[derive(Clone, Copy, Debug)]
pub struct Rect {
    pub x: i32,
    pub y: i32,
    pub w: i32,
    pub h: i32,
}

impl Rect {
    pub fn contains(&self, px: i32, py: i32) -> bool {
        px >= self.x && px < self.x + self.w && py >= self.y && py < self.y + self.h
    }
}

/// 菜单按钮几何（主菜单 2 按钮：人机/两人）
// ★ 业务对齐（2026-11-20）：主菜单按钮几何 = 原版 orig_menu.png 真机截图实测
//   （720×1280 基准，与 C ui_menu.c g_btns 共用真值；旧自创值 y=400/520 差 467px 已废弃）
pub const MENU_BTN_HUMAN: Rect = Rect { x: 202, y: 867, w: 314, h: 76 };     // 人机 (202,867)-(516,943)
pub const MENU_BTN_TWOPLAYER: Rect = Rect { x: 202, y: 982, w: 314, h: 76 }; // 两人 (202,982)-(516,1058)
/// ★ 原版主菜单 3 小按钮（UI_COMPARE §1 实测 900 系 ÷1.25 → 720 基准）
pub const MENU_BTN_NET: Rect = Rect { x: 172, y: 1097, w: 171, h: 86 };   // 联网 (215,1371)-(429,1479)
pub const MENU_BTN_LOCAL: Rect = Rect { x: 375, y: 1097, w: 171, h: 86 }; // 联机 (469,1371)-(683,1479)
pub const MENU_BTN_FOUR: Rect = Rect { x: 582, y: 1097, w: 104, h: 86 };  // 四国军棋 (727,1371)-(858,1479)
/// 原版左下红顶白墙房子 (60,1330)-(165,1485) 900 系 → 720 基准；按用户指令 tap 不跳转
pub const MENU_BTN_HOUSE: Rect = Rect { x: 48, y: 1064, w: 84, h: 124 };

/// ★ 原版 datah_041 面板 450×335 居中 → 720 基准偏移 (135,472.5)，取整 (135,472,450,335)
///   8 按钮真值见 DIFF_BTNS_ORIG；UI_COMPARE §11 真机实测 Δ≤0.5px 佐证
///   （已删除 2026-11-20 自创几何 (180,420)-(540,820) 过时注释——与真值矛盾）
pub const DIFFICULTY_PANEL: Rect = Rect {
    x: 135,
    y: 472,
    w: 450,
    h: 335,
};

/// 面板外 = 取消语义（规格 §10 通病根治：所有设置弹窗点面板外须回滚，与取消按钮对称）
///
/// 返回：true=命中面板外（须回滚快照 + 回主菜单）；false=命中面板内某控件
pub fn is_outside_panel(px: i32, py: i32, panel: &Rect) -> bool {
    !panel.contains(px, py)
}

/// ★ 原版 datah_012 游戏设置面板真值（450×465 → 720 偏移 +135,+407.5）
pub const SETTINGS_PANEL: Rect = Rect { x: 135, y: 407, w: 450, h: 465 };
/// ★ 原版 datah_025 退出确认真值（450×220 居中 → 720 偏移 +135,+530，整数化 530）
/// 确定=退出（Junqi.ExitConfirm「真的要退出吗？」），取消=继续对局
///   确定 Ok    局部 30,135-210,195 → 720 基准 165,665-345,725
///   取消 Cancel 局部 240,135-420,195 → 720 基准 375,665-555,725
pub const EXIT_BTN_QUIT: Rect = Rect { x: 165, y: 665, w: 180, h: 60 };
pub const EXIT_BTN_STAY: Rect = Rect { x: 375, y: 665, w: 180, h: 60 };
pub fn exit_btn_hit(x: f32, y: f32) -> Option<bool> {
    if EXIT_BTN_QUIT.contains(x as i32, y as i32) { Some(true) }  // 退出
    else if EXIT_BTN_STAY.contains(x as i32, y as i32) { Some(false) } // 继续
    else { None }
}

/// ★ 设置弹窗命中：GameSet 用 datah_012 真值，Settings（声音）用 datah_001 真值。
/// 统一语义索引：0-3=开关（Settings 仅 0/1/2 有效）、4=确定、5=取消
pub fn settings_btn_hit(x: f32, y: f32, is_gameset: bool) -> Option<u8> {
    if is_gameset {
        for (i, &(x0, y0, x1, y1)) in GAMESET_BTNS_ORIG.iter().enumerate() {
            if x >= x0 as f32 && x <= x1 as f32 && y >= y0 as f32 && y <= y1 as f32 {
                return Some(i as u8); // 0-3=cfg开关, 4=确定, 5=取消
            }
        }
    } else {
        for (i, &(x0, y0, x1, y1)) in SETTINGS_BTNS_ORIG.iter().enumerate() {
            if x >= x0 as f32 && x <= x1 as f32 && y >= y0 as f32 && y <= y1 as f32 {
                // datah_001 索引 3/4 = 确定/取消 → 映射到统一 4/5
                return Some(if i < 3 { i as u8 } else { (i + 1) as u8 });
            }
        }
    }
    None
}

/// 原版 datah_062 新局弹窗按钮命中：0=确定 1=取消 2=明棋 3=暗棋
pub fn newgame_btn_hit(x: f32, y: f32) -> Option<u8> {
    for (i, &(x0, y0, x1, y1)) in NEWGAME_BTNS_ORIG.iter().enumerate() {
        if x >= x0 as f32 && x <= x1 as f32 && y >= y0 as f32 && y <= y1 as f32 {
            return Some(i as u8);
        }
    }
    None
}

/// 难度弹窗 8 按钮（level 1-4 × mode 布局/翻棋）。按钮索引 0-7。
/// 命中判定 + 解析 (level, mode)。
pub fn difficulty_hit(px: i32, py: i32, _panel: &Rect) -> Option<(u8, u8)> {
    // ★ 原版 datah_041 真值命中：8 按钮坐标（已含中心偏移），逐按钮矩形判定
    for (i, &(x0, y0, x1, y1)) in DIFF_BTNS_ORIG.iter().enumerate() {
        if px >= x0 && px <= x1 && py >= y0 && py <= y1 {
            let level = (i / 2) as u8 + 1; // 行 0-3 → level 1-4
            let mode = (i % 2) as u8;      // 列 0=Lay / 1=Flip
            return Some((level, mode));
        }
    }
    None // 面板外/按钮间隙（调用方判取消）
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::state::{MODE_FLIP, MODE_LAY, SIDE_BLUE, State};

    // §10 菜单"人机对战"→难度弹窗
    #[test]
    fn menu_to_difficulty() {
        let mut ui = Ui::new();
        ui.menu_human();
        assert_eq!(ui.screen, Screen::Difficulty);
        assert!(ui.vs_ai);
    }

    // §10 难度选（level=1 新手 翻棋 mode）→ 开局进对局
    #[test]
    fn difficulty_start_game() {
        let mut ui = Ui::new();
        let mut st = State::new();
        ui.menu_human();
        ui.difficulty_select(1, MODE_FLIP, &mut st);
        assert_eq!(ui.screen, Screen::Game);
        assert_eq!(st.mode, MODE_FLIP, "翻棋 mode 应用");
        assert_eq!(st.turn, SIDE_BLUE, "蓝先手");
        assert_eq!(st.stage, crate::state::Stage::Play);
    }

    // §10 NEWGAME 取消/面板外 → 恢复切换前 mode（会话快照）
    #[test]
    fn newgame_outside_rollback() {
        let mut ui = Ui::new();
        let mut st = State::new();
        st.mode = MODE_LAY;
        ui.begin_popup(&st); // 快照 mode=LAY
        st.mode = MODE_FLIP; // 弹窗内切换（未确认）
        ui.cancel_popup(&mut st); // 面板外=取消
        assert_eq!(st.mode, MODE_LAY, "取消恢复快照 mode");
        assert_eq!(ui.screen, Screen::Menu);
    }

    // §10 GAMESET 取消 → 恢复原 4 开关
    #[test]
    fn gameset_outside_rollback() {
        let mut ui = Ui::new();
        let mut st = State::new();
        st.cfg = [1, 1, 1, 1, 1];
        ui.begin_popup(&st);
        st.cfg = [0, 0, 1, 1, 1]; // 弹窗内改开关
        ui.cancel_popup(&mut st);
        assert_eq!(st.cfg, [1, 1, 1, 1, 1], "取消恢复原开关");
    }

    // §10 终局"再来一局"→ 重开（对齐 C jq_restart_game：随 mode 重载布局 + 计数归零 + 回对局）
    #[test]
    fn restart_after_over() {
        let mut ui = Ui::new();
        let mut st = State::new();
        st.stage = crate::state::Stage::Over;
        st.total_moves = 50;
        ui.restart(&mut st);
        let pieces = st.cells.iter().filter(|c| !c.is_empty()).count();
        assert!(pieces == 50, "重开重载布局=50子（原版每方25）；旧实现清空成空棋盘是 bug");
        assert_eq!(st.total_moves, 0, "计数归零");
        assert_eq!(st.stage, crate::state::Stage::Play);
        assert_eq!(ui.screen, Screen::Game);
    }

    // §10 面板外判定
    #[test]
    fn outside_panel_detect() {
        let panel = Rect { x: 135, y: 445, w: 450, h: 390 };
        assert!(!is_outside_panel(200, 500, &panel), "面板内非 outside");
        assert!(is_outside_panel(10, 10, &panel), "面板外=outside");
    }

    // §10 难度 8 按钮命中解析（col+1=level, row=mode）
    #[test]
    fn difficulty_hit_parse() {
        // ★ 原版 datah_041 真值按钮中心：i=3 = 中级区 Flip → level=2 mode=1
        let (x0, y0, x1, y1) = DIFF_BTNS_ORIG[3];
        let cx = (x0 + x1) / 2;
        let cy = (y0 + y1) / 2;
        let panel = DIFFICULTY_PANEL;
        let got = difficulty_hit(cx, cy, &panel).unwrap();
        assert_eq!(got, (2, 1), "中级区翻棋 → level=2 mode=1");
    }
}
#[cfg(test)]
mod difficulty_init_tests {
    use super::*;
    #[test]
    fn difficulty_flip_inits_dark_board() {
        let mut ui = Ui::new();
        let mut st = crate::state::State::new();
        ui.difficulty_select(2, 1, &mut st); // mode=1 暗棋
        assert_eq!(st.mode, 1);
        let pieces: Vec<_> = st.cells.iter().filter(|c| !c.is_empty()).collect();
        assert!(!pieces.is_empty());
        assert!(pieces.iter().all(|c| c.side == crate::state::SIDE_UNKNOWN), "暗棋全 0xFF");
        assert!(pieces.iter().all(|c| !c.revealed), "暗棋全未翻");
        // 明棋
        let mut st2 = crate::state::State::new();
        ui.difficulty_select(2, 0, &mut st2);
        let pieces2: Vec<_> = st2.cells.iter().filter(|c| !c.is_empty()).collect();
        assert!(pieces2.iter().all(|c| c.revealed), "明棋全已翻");
    }
}
/// ★ 游戏内菜单 8 按钮几何（原版 ui_menu.c g_ingame_btns，720×1280 基准，4 列×2 行）
pub const INGAME_BTNS: [(i32, i32, i32, i32); 8] = [
    (49, 466, 201, 624),   // 0 重新开始
    (213, 466, 365, 624),  // 1 悔棋
    (371, 466, 523, 624),  // 2 电脑变着
    (530, 466, 682, 624),  // 3 游戏规则
    (49, 653, 201, 809),   // 4 功能设置
    (213, 653, 365, 809),  // 5 战况统计
    (371, 653, 523, 809),  // 6 软件信息
    (530, 653, 682, 809),  // 7 退出程序
];

/// 命中游戏内菜单按钮（归一化 720×1280 坐标）
pub fn ingame_hit(x: i32, y: i32) -> Option<u8> {
    for (i, &(x0, y0, x1, y1)) in INGAME_BTNS.iter().enumerate() {
        if x >= x0 && x <= x1 && y >= y0 && y <= y1 {
            return Some(i as u8);
        }
    }
    None
}


#[cfg(test)]
mod settings_btn_tests {
    use super::*;
    #[test]
    fn settings_btn_hit_grid() {
        // ★ 原版 datah_012 真值：每个按钮中心命中正确索引（0-3开关/4确定/5取消）
        for (i, &(x0, y0, x1, y1)) in GAMESET_BTNS_ORIG.iter().enumerate() {
            let cx = ((x0 + x1) / 2) as f32;
            let cy = ((y0 + y1) / 2) as f32;
            assert_eq!(settings_btn_hit(cx, cy, true), Some(i as u8), "原版按钮{} 中心命中", i);
        }
        assert_eq!(settings_btn_hit(10.0, 10.0, true), None);
        // ★ 原版 datah_001 声音设置真值：3 开关(0-2) + 确定(4) + 取消(5)，几何与 GameSet 不同
        for (i, &(x0, y0, x1, y1)) in SETTINGS_BTNS_ORIG.iter().enumerate() {
            let cx = ((x0 + x1) / 2) as f32;
            let cy = ((y0 + y1) / 2) as f32;
            let want = if i < 3 { i as u8 } else { (i + 1) as u8 };
            assert_eq!(settings_btn_hit(cx, cy, false), Some(want), "声音设置原版按钮{} 中心命中", i);
        }
        // 防复发：GameSet 确定 y=793 与 Settings 确定 y=780 是不同真值，不能混用
        assert_eq!(settings_btn_hit(255.0, 823.0, true), Some(4), "GameSet 确定 y=823");
        assert_eq!(settings_btn_hit(255.0, 780.0, false), Some(4), "Settings 确定 y=780");
        assert_eq!(settings_btn_hit(255.0, 823.0, false), None, "Settings 确定不在 GameSet y");
    }
}

#[cfg(test)]
mod exit_btn_tests {
    use super::*;
    #[test]
    fn exit_btn_hit_quadrants() {
        // ★ 原版 datah_025 真值中心：确定(255,695)=退出 / 取消(465,695)=继续
        assert_eq!(exit_btn_hit(255.0, 695.0), Some(true), "确定=退出");
        assert_eq!(exit_btn_hit(465.0, 695.0), Some(false), "取消=继续");
        // 按钮外沿内仍命中（覆盖整块 rect）
        assert_eq!(exit_btn_hit(170.0, 670.0), Some(true), "确定左上角");
        assert_eq!(exit_btn_hit(550.0, 720.0), Some(false), "取消右下角");
        assert_eq!(exit_btn_hit(450.0, 200.0), None, "面板外");
        // 与 Menu/GameSet 命中同源：均为 720 基准（jni.rs 统一 ×0.8 转入）
    }
}

#[cfg(test)]
mod newgame_btn_tests {
    use super::*;
    #[test]
    fn newgame_btn_hit_orig_truth() {
        // ★ 原版 datah_062 真值（720 基准）：0=Ok 1=Cancel 2=Lay 3=Show
        for (i, &(x0, y0, x1, y1)) in NEWGAME_BTNS_ORIG.iter().enumerate() {
            let cx = ((x0 + x1) / 2) as f32;
            let cy = ((y0 + y1) / 2) as f32;
            assert_eq!(newgame_btn_hit(cx, cy), Some(i as u8), "原版新局按钮{} 中心命中", i);
        }
        // 原版 XML 防复发：Lay/Show 同列(x 355-555)上下两行(557-617 / 632-692)，非同一行
        assert_eq!(NEWGAME_BTNS_ORIG[2].0, NEWGAME_BTNS_ORIG[3].0, "Lay/Show 同列");
        assert!(NEWGAME_BTNS_ORIG[2].1 < NEWGAME_BTNS_ORIG[3].1, "Lay 在 Show 上方");
        assert_eq!(newgame_btn_hit(10.0, 10.0), None, "面板外");
    }
}
