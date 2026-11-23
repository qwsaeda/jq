//! ai_pick_decision_check — AI 选着决策真值测试（C jni_bridge.c:309-380 逐分支语义）
//!
//! ai_score 评分表已有 154 case 桥接测试，但"给定局面 AI 到底选哪步"（候选过滤/
//! 难度分流/tie-break/翻棋回退/空移兜底）无判据——ai_score 对但选着逻辑错照样 PASS。
//! 本文件按 C ai_pick 源码逐分支构造局面，断言 Rust 选着 == C 语义推导的期望。
use junqi_rs::ai::ai_pick;
use junqi_rs::board::cell_index;
use junqi_rs::state::{Cell, Rank, State, SIDE_BLUE, SIDE_RED, SIDE_UNKNOWN, MODE_LAY, MODE_FLIP};

fn clear(st: &mut State) {
    for c in st.cells.iter_mut() { *c = Cell::empty(); }
    st.turn = SIDE_RED; // AI 方
}
fn put(st: &mut State, row: usize, col: usize, rank: u8, side: junqi_rs::state::Side, rev: bool) {
    st.cells[cell_index(row, col)] = Cell { rank: Rank(rank), side, revealed: rev };
}

// 分支①：贪心选最高分——两个可吃目标（弱敌 rank8 vs 强敌 rank1 编码红方 +0xc）
// C: sc>bs 严格大于取先遇到的最高分。rank3 红(15) 吃 rank8 蓝 = 1000，吃 rank1 蓝 = 1000（都1000）
// 用不同分：吃普通子(1000) vs 空移(2) → 必选吃子
#[test]
fn c_greedy_prefers_eat_over_empty() {
    let mut s = State::new(); s.mode = MODE_LAY; s.cfg = [1,1,1,1,0]; clear(&mut s);
    put(&mut s, 6, 0, 3 + 0xc, SIDE_RED, true);  // 红 rank3 在 (6,0)
    put(&mut s, 5, 0, 8, SIDE_BLUE, true);        // 蓝 rank8 在 (5,0) 相邻可吃
    let mv = ai_pick(&s, 3).unwrap();
    assert_eq!(mv.from, cell_index(6,0));
    assert_eq!(mv.to, cell_index(5,0), "贪心应吃蓝子而非空移");
}

// 分支②：level1 被动——sc>=2 取最小（空移 sc=2 < 吃子 sc=1000）
// C: 新手区优先 ws（最小分），有空移格则选空移，不吃子
#[test]
fn c_level1_passive_prefers_empty_move() {
    let mut s = State::new(); s.mode = MODE_LAY; s.cfg = [1,1,1,1,0]; clear(&mut s);
    put(&mut s, 6, 0, 3 + 0xc, SIDE_RED, true);
    put(&mut s, 5, 0, 8, SIDE_BLUE, true);
    // 给一个空移目标 (6,1)（同 row 铁路/普通格，空目标 sc=2）
    let mv = ai_pick(&s, 1).unwrap();
    // C 语义：ws 取 sc>=2 最小=空移(2)，应选空移目标而非吃子
    assert_eq!(mv.from, cell_index(6,0));
    assert!(s.cells[mv.to].rank == Rank(0), "level1 新手区应选空移(目标空), 实际 to={}", mv.to);
}

// 分支③：无走子 → 翻棋（暗棋未翻子定色）
#[test]
fn c_fallback_to_flip_when_no_move() {
    let mut s = State::new(); s.mode = MODE_FLIP; s.cfg = [1,1,1,1,1]; clear(&mut s);
    // 红方仅一枚未翻暗子（side=UNKNOWN），无已翻子可走 → 必翻棋
    put(&mut s, 6, 0, 3, SIDE_UNKNOWN, false);
    let mv = ai_pick(&s, 3).unwrap();
    assert!(mv.is_flip, "无可走子时应翻棋");
    assert_eq!(mv.from, cell_index(6,0));
    assert_eq!(mv.to, cell_index(6,0), "翻棋 from==to");
}

// 分支④：翻棋只排军旗（地雷可翻）——C 2026-11-14 规则实证
#[test]
fn c_flip_excludes_only_flag_not_mine() {
    let mut s = State::new(); s.mode = MODE_FLIP; s.cfg = [1,1,1,1,1]; clear(&mut s);
    put(&mut s, 6, 0, 12, SIDE_UNKNOWN, false); // 未翻军旗（不可翻）
    put(&mut s, 6, 1, 11, SIDE_UNKNOWN, false); // 未翻地雷（可翻）
    let mv = ai_pick(&s, 3).unwrap();
    assert!(mv.is_flip);
    assert_eq!(mv.from, cell_index(6,1), "应翻地雷而非军旗（只排旗）");
}

// 分支⑤：暗棋未翻目标不入候选（不透视）——目标未翻暗子时 AI 不选吃它
#[test]
fn c_dark_target_excluded_from_candidates() {
    let mut s = State::new(); s.mode = MODE_FLIP; s.cfg = [1,1,1,1,1]; clear(&mut s);
    put(&mut s, 6, 0, 3 + 0xc, SIDE_RED, true); // 红已翻司令
    put(&mut s, 5, 0, 8, SIDE_UNKNOWN, false);  // 蓝未翻暗子（不透视）
    let mv = ai_pick(&s, 3).unwrap();
    // 未翻目标被过滤 → 不该选吃它；翻棋候选仅未翻子(5,0) side=UNKNOWN 可翻归 me
    // C 语义：未翻目标被过滤后，红司令仍有空移候选(sc=2≥1) → 贪心走空移，不翻棋。
    // 关键断言：AI【没有】选吃未翻暗子(5,0=idx25)——即不透视。
    assert!(!(mv.from == cell_index(6,0) && mv.to == cell_index(5,0)),
            "AI 不应选吃未翻暗子(不透视), 实际 from={} to={}", mv.from, mv.to);
}

// 分支⑥：无候选返回 None（全空盘）
#[test]
fn c_no_candidate_returns_none() {
    let mut s = State::new(); s.mode = MODE_LAY; s.cfg = [1,1,1,1,0]; clear(&mut s);
    assert!(ai_pick(&s, 3).is_none(), "空盘无子可选应返 None");
}
