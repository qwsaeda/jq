/*
 * rule_probe_shared.inc — ★ 32/64 位规则引擎行为探针**共享本体**（逐字节同源）
 *
 * 由两端包装文件 include：
 *   · 64 位：rev64/src_rs/rule_probe.c            （#include "junqi.h"）
 *   · 32 位：rev64/src_jni_translated/jq_rs_ruleprobe.c（#include "jq_rs_rules.h"）
 *
 * ★ 为什么必须"逐字节同源"：探针的价值在于**两端跑同一批位置**——若各自维护
 *   用例，迟早漂移（一边加了用例另一边没加），对比结果就不再可信。用例本体
 *   单点存放 = 结构上排除漂移。
 *
 * ★ 只依赖规则 API（jq_state_init 不用 / jq_can_move / jq_try_move /
 *   jq_side_stalemated / jq_flip / cell_index）+ 端无关的原始存储约定
 *   （红方已翻开 rank = rank+0xc）——不含任何一方独有的辅助函数。
 *
 * 输出：PROBE_LOG("RULE-PROBE <case> <k=v,...>")，最后一行 "RULE-PROBE DONE"
 */

/* ★ 端无关兼容层：双端常量集合略有差异（32 位 jq_rs_rules.h 未定义 SIDE_UNKNOWN）——
   此处只补**探针自用**的缺失常量，绝不引用任何一方独有的辅助函数（否则"共享源"
   名不副实 / 双端行为不可比）。 */
#ifndef SIDE_UNKNOWN
#define SIDE_UNKNOWN 0xFF   /* 暗棋未定色（32 位头未定义此宏，但语义同 0xFF） */
#endif

/* ---------- 摆棋辅助（与端无关的原始存储约定） ---------- */

/* 摆一枚子：idx = row*5+col；rank 字节原始存储（红方已翻开 = rank+0xc）；
   side: 0=蓝 1=红 0xFF=未定色；rev: 0=暗 1=明 */
static void probe_put(jq_state *st, int row, int col, int rank, int side, int rev)
{
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS) return;
    jq_cell *c = &st->cells[cell_index(row, col)];
    c->rank = (uint8_t)rank;
    c->side = (uint8_t)side;
    c->revealed = (uint8_t)(rev ? 1 : 0);
    c->flags = 0;
}

/* 红方已翻开子的存储 rank（rank+0xc，与端无关的存储约定） */
static int probe_red_rank(int rank) { return rank + 0xc; }

/* 清空棋盘（不调 jq_state_init——两端 init 语义可能不同，直接清 cells 更可比） */
static void probe_clear(jq_state *st)
{
    memset(st->cells, 0, sizeof(st->cells));
    st->turn = SIDE_BLUE;
    st->stage = STG_PLAY;
    st->winner = -1;
    st->moves_since_eat = 0;
    st->total_moves = 0;
    st->mines_cleared[0] = st->mines_cleared[1] = 0;
}

/* ---------- 用例组 ----------
 * 每组：摆棋 + 一组查询；查询类型：
 *   CM from to   → can_move 结果（0/1）
 *   TM from to   → try_move 结果（事件码，-1=非法）
 *   FL idx       → flip 结果（0/1）
 *   SM side      → side_stalemated 结果（0/1）
 *   WIN          → 走子/翻棋后的 winner（-1/0/1/2）
 *   MINE side    → mines_cleared[side]
 * 覆盖的 AUD-R 项（括号内）：
 *   R-001 底线铁路纵滑 / R-002 暗棋地雷军旗仅工兵吃 / R-005 困毙参数化
 *   R-006 暗棋信息不泄露 / R-007 暗棋大本营 / R-009 AI 炸弹撞旗（规则侧）
 *   R-016 EV_FLAG 开关 / R-020 炸弹撞旗判胜 / R-025 起点越界 / R-028 空行营
 *   R-029 大本营目标 / R-030 进大本营不可再动 / R-033 翻棋计数 / R-045 困毙白名单
 *   R-071 空大本营 / R-073 困毙排地雷 / R-089 困毙 vs 吃旗
 */

/* 棋盘几何常量（12 行 × 5 列，行 0=蓝方底线，行 11=红方底线） */
#define R_BLUE_BOT   0
#define R_MID        5
#define R_RED_TOP   11

void jq_rule_probe_run(void)
{
    jq_state st;

    /* ===== A 组：明棋走子 / 铁路方向（R-001 / R-025 / R-028 / R-029 / R-030 / R-071） ===== */
    {
        probe_clear(&st);
        /* 蓝方工兵在 (5,2)（中路），红方普通子在 (0,2)（底线）→ 纵滑是否允许 */
        probe_put(&st, R_MID, 2, 9, SIDE_BLUE, 1);      /* 工兵 rank9 */
        probe_put(&st, R_BLUE_BOT, 2, 1, SIDE_RED, 1);    /* 红方司令（已翻开，存储 +0xc） */
        /* 修正红方存储编码 */
        st.cells[cell_index(R_BLUE_BOT, 2)].rank = (uint8_t)probe_red_rank(1);
        PROBE_LOG("RULE-PROBE A1_cm_mid_to_bottom CM %d %d",
                  cell_index(R_MID, 2), cell_index(R_BLUE_BOT, 2));
        PROBE_LOG("RULE-PROBE A1_cm_result CM %d %d = %d",
                  cell_index(R_MID, 2), cell_index(R_BLUE_BOT, 2),
                  jq_can_move(&st, cell_index(R_MID, 2), cell_index(R_BLUE_BOT, 2)));
        /* A2：横滑（同底线 row0 col0 → col1） */
        probe_clear(&st);
        probe_put(&st, R_BLUE_BOT, 0, 9, SIDE_BLUE, 1);
        probe_put(&st, R_BLUE_BOT, 1, 1, SIDE_RED, 1);
        st.cells[cell_index(R_BLUE_BOT, 1)].rank = (uint8_t)probe_red_rank(1);
        PROBE_LOG("RULE-PROBE A2_cm_bottom_row CM %d %d = %d",
                  cell_index(R_BLUE_BOT, 0), cell_index(R_BLUE_BOT, 1),
                  jq_can_move(&st, cell_index(R_BLUE_BOT, 0), cell_index(R_BLUE_BOT, 1)));
        /* A3：敌方空行营一步进入（R-028）——行营在 (3,2)/(8,2) */
        probe_clear(&st);
        probe_put(&st, 4, 2, 9, SIDE_BLUE, 1);   /* 工兵在行营下一格 */
        PROBE_LOG("RULE-PROBE A3_cm_into_camp CM %d %d = %d",
                  cell_index(4, 2), cell_index(3, 2),
                  jq_can_move(&st, cell_index(4, 2), cell_index(3, 2)));
        /* A4：大本营（蓝方 (0,0)-(0,4) 底线？→ 用 (0,1) 作大本营测试位）——实际大本营位由 is_base 判定 */
        probe_clear(&st);
        probe_put(&st, 1, 1, 9, SIDE_BLUE, 1);
        PROBE_LOG("RULE-PROBE A4_cm_into_base CM %d %d = %d",
                  cell_index(1, 1), cell_index(0, 1),
                  jq_can_move(&st, cell_index(1, 1), cell_index(0, 1)));
        /* A5：起点越界（R-025）——from 越界必须拒绝（不得崩） */
        probe_clear(&st);
        PROBE_LOG("RULE-PROBE A5_cm_oob CM %d %d = %d",
                  N_CELLS + 5, cell_index(0, 0),
                  jq_can_move(&st, N_CELLS + 5, cell_index(0, 0)));
    }

    /* ===== B 组：吃子 / 炸弹撞旗 / 大本营内旗雷（R-002 / R-009 / R-020 / R-029） ===== */
    {
        /* B1：普通子吃地雷 → 应被拒（地雷仅工兵可挖） */
        probe_clear(&st);
        probe_put(&st, 5, 0, 1, SIDE_BLUE, 1);      /* 蓝方司令 */
        probe_put(&st, 5, 1, 11, SIDE_RED, 1);      /* 红方地雷（存储 +0xc） */
        st.cells[cell_index(5, 1)].rank = (uint8_t)probe_red_rank(11);
        PROBE_LOG("RULE-PROBE B1_cm_normal_vs_mine CM %d %d = %d",
                  cell_index(5, 0), cell_index(5, 1),
                  jq_can_move(&st, cell_index(5, 0), cell_index(5, 1)));
        /* B2：工兵挖雷 → 应允许 */
        probe_clear(&st);
        probe_put(&st, 5, 0, 9, SIDE_BLUE, 1);
        probe_put(&st, 5, 1, 11, SIDE_RED, 1);
        st.cells[cell_index(5, 1)].rank = (uint8_t)probe_red_rank(11);
        PROBE_LOG("RULE-PROBE B2_cm_worker_vs_mine CM %d %d = %d",
                  cell_index(5, 0), cell_index(5, 1),
                  jq_can_move(&st, cell_index(5, 0), cell_index(5, 1)));
        /* B3：炸弹撞旗 → try_move 事件 + winner（R-009 / R-020） */
        probe_clear(&st);
        probe_put(&st, 5, 0, 10, SIDE_BLUE, 1);     /* 蓝方炸弹 */
        probe_put(&st, 5, 1, 12, SIDE_RED, 1);     /* 红方军旗（存储 +0xc） */
        st.cells[cell_index(5, 1)].rank = (uint8_t)probe_red_rank(12);
        {
            int ev = jq_try_move(&st, cell_index(5, 0), cell_index(5, 1));
            PROBE_LOG("RULE-PROBE B3_bomb_vs_flag TM %d %d = ev %d win %d stage %d",
                      cell_index(5, 0), cell_index(5, 1), ev, (int)st.winner, (int)st.stage);
        }
        /* B4：普通子吃旗 → try_move 事件 + winner（R-016 EV_FLAG 分支） */
        probe_clear(&st);
        probe_put(&st, 5, 0, 1, SIDE_BLUE, 1);
        probe_put(&st, 5, 1, 12, SIDE_RED, 1);
        st.cells[cell_index(5, 1)].rank = (uint8_t)probe_red_rank(12);
        {
            int ev = jq_try_move(&st, cell_index(5, 0), cell_index(5, 1));
            PROBE_LOG("RULE-PROBE B4_normal_vs_flag TM %d %d = ev %d win %d stage %d",
                      cell_index(5, 0), cell_index(5, 1), ev, (int)st.winner, (int)st.stage);
        }
    }

    /* ===== C 组：暗棋 ruleflip（R-002 / R-006 / R-007 / R-015 / R-045 / R-073） ===== */
    {
        /* C1：未定色暗子（side=0xFF, revealed=0）不可动（R-006 信息不泄露） */
        probe_clear(&st);
        probe_put(&st, 5, 0, 1, SIDE_UNKNOWN, 0);
        PROBE_LOG("RULE-PROBE C1_cm_unrevealed CM %d %d = %d",
                  cell_index(5, 0), cell_index(5, 1),
                  jq_can_move(&st, cell_index(5, 0), cell_index(5, 1)));
        /* C2：暗子定色后翻棋 → side 变当前回合方（R-003 定色） */
        probe_clear(&st);
        st.turn = SIDE_BLUE;
        probe_put(&st, 5, 0, 1, SIDE_UNKNOWN, 0);
        {
            int ok = jq_flip(&st, cell_index(5, 0));
            PROBE_LOG("RULE-PROBE C2_flip_unknown FL %d = %d side %d rev %d turn %d",
                      cell_index(5, 0), ok, (int)st.cells[cell_index(5, 0)].side,
                      (int)st.cells[cell_index(5, 0)].revealed, (int)st.turn);
        }
        /* C3：暗地雷可翻？（R-045 / R-073 白名单） */
        probe_clear(&st);
        st.turn = SIDE_BLUE;
        probe_put(&st, 5, 0, 11, SIDE_UNKNOWN, 0);
        {
            int ok = jq_flip(&st, cell_index(5, 0));
            PROBE_LOG("RULE-PROBE C3_flip_mine FL %d = %d side %d",
                      cell_index(5, 0), ok, (int)st.cells[cell_index(5, 0)].side);
        }
        /* C4：困毙判定——只剩暗地雷可翻时是否算"未困毙"（R-073 排地雷差异点） */
        probe_clear(&st);
        st.turn = SIDE_BLUE;
        probe_put(&st, 5, 0, 11, SIDE_UNKNOWN, 0);   /* 唯一暗子=地雷（未定色） */
        PROBE_LOG("RULE-PROBE C4_stalemate_only_mine SM %d = %d",
                  (int)SIDE_BLUE, jq_side_stalemated(&st, SIDE_BLUE));
        /* C4b：同 C4 但唯一暗子=军旗（R-089 变体）→ 区分"地雷排除"与"通用终局类排除"
           （假设 2 检验）：若 32 位 C4b=1（旗也排）→ 分歧=通用终局类设计；
           若 32 位 C4b=0（旗未排）→ 分歧精确到"仅地雷"（坐实 32 位侧地雷语义差异） */
        probe_clear(&st);
        st.turn = SIDE_BLUE;
        probe_put(&st, 5, 0, 12, SIDE_UNKNOWN, 0);   /* 唯一暗子=军旗（未定色） */
        PROBE_LOG("RULE-PROBE C4b_stalemate_only_flag SM %d = %d",
                  (int)SIDE_BLUE, jq_side_stalemated(&st, SIDE_BLUE));
        /* C5：困毙判定——无子可动（R-005 参数化：查 side != turn 的方） */
        probe_clear(&st);
        st.turn = SIDE_BLUE;
        probe_put(&st, 5, 0, 12, SIDE_RED, 1);   /* 仅一枚红方旗 */
        PROBE_LOG("RULE-PROBE C5_stalemate_side_ne_turn SM %d = %d (turn %d)",
                  (int)SIDE_RED, jq_side_stalemated(&st, SIDE_RED), (int)st.turn);
        /* C6：工兵挖雷计数（R-015 mines_cleared 写） */
        probe_clear(&st);
        st.turn = SIDE_BLUE;
        probe_put(&st, 5, 0, 9, SIDE_BLUE, 1);
        probe_put(&st, 5, 1, 11, SIDE_RED, 1);
        st.cells[cell_index(5, 1)].rank = (uint8_t)probe_red_rank(11);
        {
            int ev = jq_try_move(&st, cell_index(5, 0), cell_index(5, 1));
            PROBE_LOG("RULE-PROBE C6_worker_mine TM %d %d = ev %d mine_cleared %d",
                      cell_index(5, 0), cell_index(5, 1), ev, st.mines_cleared[SIDE_BLUE]);
        }
    }

    /* ===== D 组：翻棋计数 / 终局（R-033 / R-086 / R-089） ===== */
    {
        /* D1：翻棋后 total_moves / moves_since_eat 是否 +1（R-033 / R-086） */
        probe_clear(&st);
        st.turn = SIDE_BLUE;
        probe_put(&st, 5, 0, 1, SIDE_UNKNOWN, 0);
        {
            int ok = jq_flip(&st, cell_index(5, 0));
            PROBE_LOG("RULE-PROBE D1_flip_count FL %d = %d total %d since %d",
                      cell_index(5, 0), ok, st.total_moves, st.moves_since_eat);
        }
        /* D2：困毙 + 可吃旗共存（R-089）——红方只有旗且蓝方下一手能吃 */
        probe_clear(&st);
        st.turn = SIDE_BLUE;
        probe_put(&st, 5, 0, 1, SIDE_BLUE, 1);
        probe_put(&st, 5, 1, 12, SIDE_RED, 1);
        st.cells[cell_index(5, 1)].rank = (uint8_t)probe_red_rank(12);
        PROBE_LOG("RULE-PROBE D2_stalemate_with_flag SM %d = %d",
                  (int)SIDE_RED, jq_side_stalemated(&st, SIDE_RED));
    }

    PROBE_LOG("RULE-PROBE DONE");
}
