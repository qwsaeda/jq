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
        self.screen = if quit { Screen::Menu } else { Screen::Game };
    }
}

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

/// ★ 业务对齐（2026-11-20）：难度弹窗面板 = 原版 datah_041 真机截图实测
///   720×1280 基准 (180,420)-(540,820)，与 C ui_menu.c g_diff_btns 共用真值
///   8 按钮 2 列×4 行：布局/翻棋列 x=304-400/420-516，行 y=450-521/539-610/628-700/718-789
pub const DIFFICULTY_PANEL: Rect = Rect {
    x: 180,
    y: 420,
    w: 360,
    h: 400,
};

/// 面板外 = 取消语义（规格 §10 通病根治：所有设置弹窗点面板外须回滚，与取消按钮对称）
///
/// 返回：true=命中面板外（须回滚快照 + 回主菜单）；false=命中面板内某控件
pub fn is_outside_panel(px: i32, py: i32, panel: &Rect) -> bool {
    !panel.contains(px, py)
}

/// 设置弹窗按钮几何（view 900×1600 基准，与 asset.rs 绘制 + jni.rs 触摸同源）
/// GameSet: 前 4 个 = cfg[0..3] 开关，后 2 = 确定/取消；Settings: 第 0 = 音效开关，后 2 = 确定/取消
/// 面板居中 x20%-80%(180-720) y30%-70%(480-1120)，按钮在面板内 3×2 网格
pub const SETTINGS_PANEL: Rect = Rect { x: 180, y: 480, w: 540, h: 640 };
/// 退出确认对话框按钮（view 900×1600）：左=退出、右=继续，各占中下部一小块
pub const EXIT_BTN_QUIT: Rect = Rect { x: 270, y: 700, w: 160, h: 80 };
pub const EXIT_BTN_STAY: Rect = Rect { x: 470, y: 700, w: 160, h: 80 };
pub fn exit_btn_hit(x: f32, y: f32) -> Option<bool> {
    if EXIT_BTN_QUIT.contains(x as i32, y as i32) { Some(true) }  // 退出
    else if EXIT_BTN_STAY.contains(x as i32, y as i32) { Some(false) } // 继续
    else { None }
}

pub fn settings_btn_hit(x: f32, y: f32) -> Option<u8> {
    let p = &SETTINGS_PANEL;
    if !(p.x as f32..=(p.x + p.w) as f32).contains(&x) || !(p.y as f32..=(p.y + p.h) as f32).contains(&y) {
        return None;
    }
    // 3 列 × 2 行网格：上排 6 槽位简化为 3 按钮（开关1/开关2/开关3）
    // 下排：开关4/确定/取消（GameSet）或 音效开关/确定/取消（Settings）
    let col = ((x - p.x as f32) / (p.w as f32 / 3.0)) as usize;
    let row = ((y - p.y as f32) / (p.h as f32 / 2.0)) as usize;
    let idx = row * 3 + col.min(2);
    if idx < 6 { Some(idx as u8) } else { None }
}

/// 难度弹窗 8 按钮（level 1-4 × mode 布局/翻棋）。按钮索引 0-7。
/// 命中判定 + 解析 (level, mode)。
pub fn difficulty_hit(px: i32, py: i32, panel: &Rect) -> Option<(u8, u8)> {
    if !panel.contains(px, py) {
        return None; // 面板外（调用方 is_outside_panel 判取消）
    }
    // 简化：8 按钮 4×2 网格（4 level 列 × 2 mode 行）
    let ox = px - panel.x;
    let oy = py - panel.y;
    let col = ox / (panel.w / 4);
    let row = oy / (panel.h / 2);
    if !(0..=3).contains(&col) || !(0..=1).contains(&row) {
        return None;
    }
    Some((col as u8 + 1, row as u8)) // level=col+1, mode=row(0布局/1翻棋)
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
        let panel = Rect { x: 135, y: 445, w: 450, h: 390 };
        // 第 2 列 第 1 行（mode=1 翻棋）→ level=2 mode=1
        let cx = panel.x + (panel.w / 4) + 10; // 第 2 列内（col index 1）
        let cy = panel.y + (panel.h / 2) + 10; // 第 2 行内（row index 1）
        let got = difficulty_hit(cx, cy, &panel).unwrap();
        assert_eq!(got, (2, 1), "col1+1=2 level, row1=1 mode");
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
        let p = &SETTINGS_PANEL;
        // 6 按钮中心都应命中正确索引
        for row in 0..2usize {
            for col in 0..3usize {
                let x = p.x as f32 + col as f32 * (p.w as f32 / 3.0) + 30.0;
                let y = p.y as f32 + row as f32 * (p.h as f32 / 2.0) + 60.0;
                assert_eq!(settings_btn_hit(x, y), Some((row * 3 + col) as u8));
            }
        }
        // 面板外不命中
        assert_eq!(settings_btn_hit(10.0, 10.0), None);
    }
}

#[cfg(test)]
mod exit_btn_tests {
    use super::*;
    #[test]
    fn exit_btn_hit_quadrants() {
        assert_eq!(exit_btn_hit(350.0, 740.0), Some(true), "左=退出");
        assert_eq!(exit_btn_hit(550.0, 740.0), Some(false), "右=继续");
        assert_eq!(exit_btn_hit(450.0, 200.0), None, "面板外");
    }
}
