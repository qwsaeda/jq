//! fuzztest — Rust 正向逻辑层渗透测试（对齐 fuzz_probe 4 变异器理念，无蓝牙）
//! 纯逻辑层随机变异 0 崩溃 + 状态域不变量 + 可复现性。
//! ★ 与 C fuzz_probe（src_rs）物理隔离：这是 Rust 版 src_rs_rust 的渗透判据。

#[cfg(test)]
mod tests {
    use crate::state::{Side, State, Stage, Winner, SIDE_BLUE, SIDE_RED, Rank};
    use crate::{ai, rules};

    /// xorshift32（先变换后保存，避免 C 首版自 bug）
    struct Rng(u32);
    impl Rng {
        fn new(seed: u32) -> Self { Self(seed) }
        fn next(&mut self) -> u32 {
            let mut x = self.0;
            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
            self.0 = x; // ★ 变换后保存
            x
        }
        fn below(&mut self, n: usize) -> usize { (self.next() as usize) % n }
    }

    /// F1 随机对弈：随机合法走子直到终局，验证状态域不变量 + 0 崩溃
    #[test]
    fn f1_random_games() {
        for seed in [1u32, 42, 12345] {
            let mut rng = Rng::new(seed);
            let mut st = crate::gametest::make_default_layout();
            st.cfg = [1, 1, 1, 1, 0];
            let mut steps = 0;
            while st.stage == Stage::Play && steps < 200 {
                steps += 1;
                // 不变量：turn 域 / winner 域 / cells rank 域
                assert!(st.turn.is_valid(), "turn 域（Side enum 类型安全）");
                assert!(matches!(st.winner, Winner::None | Winner::Blue | Winner::Red | Winner::Draw));
                assert!(st.cells.iter().all(|c| c.rank <= Rank(24)), "rank 存储域 [0,24]");
                // 随机走子（ai_pick 优先，否则随机合法）
                let mv = if rng.next().is_multiple_of(4) {
                    random_move(&mut rng, &st)
                } else {
                    ai::ai_pick(&st, 3).map(|m| (m.from, m.to, m.is_flip))
                };
                if let Some((f, t, is_flip)) = mv {
                    if is_flip { let _ = rules::flip(&mut st, f); }
                    else { let _ = rules::try_move(&mut st, f, t); }
                } else { break; }
            }
            assert!(steps <= 200, "guard 截断");
        }
    }

    /// 随机合法走子（全量枚举候选 + 随机选）
    fn random_move(rng: &mut Rng, st: &State) -> Option<(usize, usize, bool)> {
        let mut cand = Vec::new();
        for f in 0..60 {
            if st.cells[f].is_empty() { continue; }
            for t in 0..60 {
                if f == t { continue; }
                if rules::can_move(st, f, t) { cand.push((f, t, false)); }
            }
            if rules::can_flip(st, f) { cand.push((f, f, true)); }
        }
        if cand.is_empty() { None } else { Some(cand[rng.below(cand.len())]) }
    }

    /// F2 触摸坐标渗透：边界坐标几何计算不崩（平台无关版：用纯算术模拟映射）
    #[test]
    fn f2_touch_coords() {
        let coords = [(-100, -100), (0, 0), (900, 1600), (901, 1601),
                      (i32::MAX, i32::MIN), (450, 800), (274, 1383)];
        for &(x, y) in &coords {
            // 纯算术映射（与 view_to_cell 同源：x=78.5+col*140.5, y 分段），不依赖 android cfg
            let bx = x as f32 * 0.8;
            let by = y as f32 * 0.8;
            let _col = ((bx - 78.5) / 140.5).round().clamp(0.0, 4.0) as i32;
            let _row = if by < 822.0 { ((by - 222.0) / 71.0).round().clamp(0.0, 5.0) as i32 }
                       else { 6 + ((by - 822.0) / 71.0).round().clamp(0.0, 5.0) as i32 };
        }
    }

    /// F3 事件注入：所有事件码 → 名映射 + 音效映射不崩
    #[test]
    fn f3_event_injection() {
        use crate::evt::{ev_name, sfx_for_code};
        for code in [Ev::Move, Ev::Eat, Ev::Bomb, Ev::Flag, Ev::Dead, Ev::Mine, Ev::Flip] {
            let _ = ev_name(code);
            let _ = sfx_for_code(code);
            assert!(!ev_name(code).is_empty());
        }
        // 73 名表全名查名不崩
        for name in crate::evt::JQ_EVENT_NAMES_73 {
            let _ = crate::evt::name_to_73_index(name);
        }
    }

    use crate::state::Ev;

    /// F4 undo 边界：空栈 undo / 连续 undo / 深栈 undo 不崩 + 状态回退正确
    #[test]
    fn f4_undo_boundary() {
        let mut ui = crate::ui::Ui::new();
        let st = crate::gametest::make_default_layout();
        // 空栈 undo
        assert!(!ui.undo(&mut st.clone()), "空栈 undo 返 false");
        // 深栈 push 64+（容量上限）
        for _ in 0..70 { ui.push_undo(&st); }
        assert!(ui.undo_stack.len() <= 64, "undo 栈容量上限");
        // 连续 pop 到空
        let mut s = st.clone();
        let mut n = 0;
        while ui.undo(&mut s) { n += 1; }
        assert!(n <= 64);
        assert!(ui.undo_stack.is_empty());
    }

    /// F5 存档域校验 fuzz（对齐 C fuzz_probe F5 契约1/2/3）
    /// 契约1 合法域零漂移 / 契约2 脏枚举拒读 / 契约3 越域格归零
    #[test]
    fn f5_save_domain_fuzz() {
        // 契约1：合法域随机局 validate_state 不改数据（零漂移）
        for seed in [1u32, 42, 999] {
            let mut rng = Rng::new(seed);
            let mut st = crate::gametest::make_default_layout();
            st.turn = if rng.next().is_multiple_of(2) { SIDE_BLUE } else { SIDE_RED };
            st.mode = (rng.next() % 2) as u8;
            st.winner = match rng.next() % 4 {
                0 => Winner::None, 1 => Winner::Blue, 2 => Winner::Red, _ => Winner::Draw,
            };
            st.cfg = [1, 1, 1, 1, 1];
            let before = st.clone();
            assert!(crate::save::validate_state(&mut st), "合法域校验通过");
            let same = st.cells.iter().zip(before.cells.iter()).all(|(a,b)| a.rank==b.rank && a.side==b.side && a.revealed==b.revealed)
                && st.turn == before.turn && st.mode == before.mode;
            assert!(same, "契约1：合法域零漂移");
        }
        // 契约2：脏枚举（turn>1/mode>1）拒读
        let mut bad = crate::gametest::make_default_layout();
        bad.mode = 2; // turn 已 Side enum（非法判别值无法表达），改测 mode 越域
        assert!(!crate::save::validate_state(&mut bad), "契约2：mode=2 拒读");
        let mut bad2 = crate::gametest::make_default_layout();
        bad2.mode = 9;
        assert!(!crate::save::validate_state(&mut bad2), "契约2：mode=9 拒读");
        // 契约3：越域格（rank>24 / side 非法）归零
        // ★ revealed 已 bool 类型安全（非法值无法表达），原"revealed=2 归零"契约
        //   前移到反序列化边界（u8→bool try_from），内部字段不再需要该校验。
        let mut dirty = crate::gametest::make_default_layout();
        dirty.cells[0].rank = Rank(99); // 越域
        assert!(Side::from_raw(7).is_none(), "契约3：非法 side=7 被边界拒");
        assert!(crate::save::validate_state(&mut dirty));
        assert_eq!(dirty.cells[0].rank, Rank(0), "契约3：越域 rank 归零");
        
        // 计数负值钳 0
        let mut neg = crate::gametest::make_default_layout();
        neg.pieces_left = [-5, -3];
        neg.mines_cleared = [-1, -1];
        crate::save::validate_state(&mut neg);
        assert_eq!(neg.pieces_left, [0, 0]);
        assert_eq!(neg.mines_cleared, [0, 0]);
    }
}
