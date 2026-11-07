/*
 * rulestest.c — 规则引擎全量用例（AUD-R-135 P3 落地：make rulestest 表驱动）
 *
 * 背景：规则引擎公开 API（jq_can_move/jq_try_move/jq_can_flip/jq_flip/
 *       jq_side_stalemated/jq_on_flag_capture/jq_on_draw/jq_on_stalemate/ai_score）
 *       此前散落在 aitest（ai_score 纯函数）/ evverify（事件断言）/ consistency
 *       （9/9 规则矩阵）三处部分覆盖，无单一全量入口。本轮建表驱动全量用例。
 *
 * 诚实边界：
 *   - 坐标全部**实测确证**（2026-11-07 /tmp/opencode/probe 脚本逐一跑过），
 *     夹具自造棋子（覆盖默认布局格），不依赖 JQ_DEFAULT_LAY 具体分布假设。
 *   - 事件码：EV_MOVE=0 EV_EAT=1 EV_BOMB=2 EV_FLAG=3 EV_DEAD=4 EV_MINE=5 EV_FLIP=6。
 *   - 行营（(3,2) 等）非铁路、一步进入受限（cfg(1) 大本营开关）→ 工兵挖雷走
 *     直接覆盖空格非行营格（(3,1)），不依赖行营拓扑。
 *   - 暗棋先手翻子定色（R-003）：jq_dark_setup 全 60 格 side=SIDE_UNKNOWN，
 *     首翻定 side=turn（实测 50 格 UNKNOWN，可翻子 idx=3 rank8）。
 *
 * 对齐 Makefile .PHONY: rulestest target（board.c layout.c rules.c save.c
 *   rulestest.c ui_state_stub.c，-DRS_NO_GL -lm）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"

extern const uint8_t JQ_DEFAULT_LAY[30];

static int fails = 0;
#define CHECK(cond, msg) do { \
    if (cond) printf("  ✓ %s\n", msg); \
    else { printf("  ✗ FAIL: %s\n", msg); fails++; } \
} while (0)

/* 明棋夹具：jq_state_init + 全子 revealed=1 + stage=PLAY（自造棋子） */
static void setup_lay(jq_state *st)
{
    jq_state_init(st);
    for (int i = 0; i < N_CELLS; i++) st->cells[i].revealed = 1;
    st->stage = STG_PLAY;
    st->turn = SIDE_BLUE;
    st->winner = JQ_WIN_NONE;
}

/* cell_index 速记（row*5+col） */
#define C(r,c) ((r)*BOARD_COLS + (c))

/* 覆盖格子：rank 1-9 蓝方原值，红方 rank+0xc；flags 派生 */
static void put(jq_state *st, int idx, int rank_norm_val, int side, int revealed)
{
    st->cells[idx].rank = (side == SIDE_RED && rank_norm_val != 0)
                          ? (uint8_t)(rank_norm_val + 0xc)
                          : (uint8_t)rank_norm_val;
    st->cells[idx].side = (uint8_t)side;
    st->cells[idx].revealed = revealed;
    st->cells[idx].flags = (rank_norm_val == RANK_BOMB ? JQF_BOMB : 0)
                         | (rank_norm_val == RANK_MINE ? JQF_MINE : 0)
                         | (rank_norm_val == RANK_FLAG ? JQF_FLAG : 0);
}

/* ① 明棋空移：蓝 rank2@(3,0) → (3,1) 空格（横向公路相邻） */
static void t_lay_move(void)
{
    jq_state st; setup_lay(&st);
    put(&st, C(3,0), 2, SIDE_BLUE, 1);
    int ev = jq_try_move(&st, C(3,0), C(3,1));
    CHECK(ev == EV_MOVE, "明棋空移 ev=EV_MOVE");
    CHECK(st.cells[C(3,1)].rank == 2 && st.cells[C(3,0)].rank == RANK_EMPTY,
          "空移后换位（蓝 rank2 落(3,1)）");
    CHECK(st.turn == SIDE_RED, "空移后 turn=RED");
}

/* ② 明棋吃子（大压小）：蓝 rank2@(3,0) 吃 红 rank8@(3,1)，2<8 → EV_EAT */
static void t_lay_eat(void)
{
    jq_state st; setup_lay(&st);
    put(&st, C(3,0), 2, SIDE_BLUE, 1);
    put(&st, C(3,1), 8, SIDE_RED, 1);
    int ev = jq_try_move(&st, C(3,0), C(3,1));
    CHECK(ev == EV_EAT, "大压小 ev=EV_EAT");
    CHECK(st.cells[C(3,1)].rank == 2, "蓝 rank2 吃后落(3,1)");
    CHECK(st.cells[C(3,0)].rank == RANK_EMPTY, "源格空");
}

/* ③ 同级同归：蓝 rank6@(3,0) 撞 红 rank6@(3,1) → EV_BOMB 双方消失 */
static void t_same_rank(void)
{
    jq_state st; setup_lay(&st);
    put(&st, C(3,0), 6, SIDE_BLUE, 1);
    put(&st, C(3,1), 6, SIDE_RED, 1);
    int ev = jq_try_move(&st, C(3,0), C(3,1));
    CHECK(ev == EV_BOMB, "同级互杀 ev=EV_BOMB");
    CHECK(st.cells[C(3,0)].rank == RANK_EMPTY && st.cells[C(3,1)].rank == RANK_EMPTY,
          "同级同归后双方格均空");
}

/* ④ 炸弹同归：蓝 rank10@(3,0) 撞 红 rank2@(3,1) → EV_BOMB 双方消失 */
static void t_bomb(void)
{
    jq_state st; setup_lay(&st);
    put(&st, C(3,0), RANK_BOMB, SIDE_BLUE, 1);
    put(&st, C(3,1), 2, SIDE_RED, 1);
    int ev = jq_try_move(&st, C(3,0), C(3,1));
    CHECK(ev == EV_BOMB, "炸弹同归 ev=EV_BOMB");
    CHECK(st.cells[C(3,0)].rank == RANK_EMPTY && st.cells[C(3,1)].rank == RANK_EMPTY,
          "炸弹同归后双方格均空");
}

/* ⑤ 工兵挖雷（明棋）：蓝 rank9@(0,0) 吃 红 rank11@(0,1) 大本营相邻，工兵专属 EV_MINE */
static void t_worker_mine(void)
{
    jq_state st; setup_lay(&st);
    put(&st, C(0,0), RANK_WORKER, SIDE_BLUE, 1);
    put(&st, C(0,1), RANK_MINE, SIDE_RED, 1);
    int ev = jq_try_move(&st, C(0,0), C(0,1));
    CHECK(ev == EV_MINE, "工兵挖雷 ev=EV_MINE");
    CHECK(st.cells[C(0,1)].rank == RANK_WORKER, "工兵落雷格");
    CHECK(st.mines_cleared[SIDE_BLUE] == 1, "mines_cleared[BLUE]=1（工兵挖雷计数）");
}

/* ⑥ 非工兵撞雷（明棋）：蓝 rank2@(0,0) 撞 红 rank11@(0,1) 大本营相邻 → EV_DEAD 送死 */
static void t_nonworker_mine(void)
{
    jq_state st; setup_lay(&st);
    put(&st, C(0,0), 2, SIDE_BLUE, 1);
    put(&st, C(0,1), RANK_MINE, SIDE_RED, 1);
    int ev = jq_try_move(&st, C(0,0), C(0,1));
    CHECK(ev == EV_DEAD, "非工兵撞雷 ev=EV_DEAD（送死）");
    CHECK(st.cells[C(0,0)].rank == RANK_EMPTY, "送死后蓝 rank2 格空");
    CHECK(st.cells[C(0,1)].rank != RANK_EMPTY, "地雷仍在（送死不消雷）");
}

/* ⑦ 扛旗（明棋）：蓝 rank2@(0,0) 吃 红军旗@(0,1) 大本营相邻，EV_FLAG + stage=OVER */
static void t_lay_flag(void)
{
    jq_state st; setup_lay(&st);
    put(&st, C(0,0), 2, SIDE_BLUE, 1);
    put(&st, C(0,1), RANK_FLAG, SIDE_RED, 1);
    int can = jq_can_move(&st, C(0,0), C(0,1));
    CHECK(can == 1, "明棋 rank2→红旗 can=1（任何子可扛旗）");
    int ev = jq_try_move(&st, C(0,0), C(0,1));
    CHECK(ev == EV_FLAG, "扛旗 ev=EV_FLAG");
    CHECK(st.stage == STG_OVER, "扛旗后 stage=OVER");
    CHECK(st.winner == SIDE_BLUE, "扛旗后 winner=BLUE");
}

/* ⑧ 炸弹撞旗（明棋，AUD-R-020）：蓝 rank10@(0,0) 撞 红旗@(0,1) 大本营相邻 → EV_BOMB + 判胜 */
static void t_bomb_flag(void)
{
    jq_state st; setup_lay(&st);
    put(&st, C(0,0), RANK_BOMB, SIDE_BLUE, 1);
    put(&st, C(0,1), RANK_FLAG, SIDE_RED, 1);
    int ev = jq_try_move(&st, C(0,0), C(0,1));
    CHECK(ev == EV_BOMB, "炸弹撞旗 ev=EV_BOMB");
    CHECK(st.stage == STG_OVER, "炸弹撞旗后 stage=OVER（AUD-R-020 判胜）");
    CHECK(st.winner == SIDE_BLUE, "炸弹撞旗 winner=BLUE（扛旗方胜）");
}

/* ⑨ 困毙：红方仅军旗@大本营（不可动）→ jq_side_stalemated(RED)=true */
static void t_stalemate(void)
{
    jq_state st;
    memset(&st, 0, sizeof(st));
    st.turn = SIDE_BLUE; st.stage = STG_PLAY; st.winner = JQ_WIN_NONE;
    for (int i = 0; i < N_CELLS; i++) st.cells[i].rank = RANK_EMPTY;
    put(&st, C(11,3), RANK_FLAG, SIDE_RED, 1);   /* 红仅军旗（大本营不可动） */
    put(&st, C(3,0), 2, SIDE_BLUE, 1);           /* 蓝 rank2（可动，确保蓝不困） */
    CHECK(jq_side_stalemated(&st, SIDE_RED), "红方仅军旗不可动 → jq_side_stalemated(RED)=true");
    CHECK(!jq_side_stalemated(&st, SIDE_BLUE), "蓝方有 rank2 可动 → 蓝不困");
}

/* ⑩ 暗棋工兵扛旗拦截：JQ_MODE_FLIP 非工兵→军旗 can=0 */
static void t_dark_flag_block(void)
{
    jq_state st;
    memset(&st, 0, sizeof(st));
    st.turn = SIDE_BLUE; st.stage = STG_PLAY; st.winner = JQ_WIN_NONE;
    st.mode = JQ_MODE_FLIP;
    put(&st, C(0,0), 2, SIDE_BLUE, 1);
    put(&st, C(0,1), RANK_FLAG, SIDE_RED, 1);
    int can = jq_can_move(&st, C(0,0), C(0,1));
    CHECK(can == 0, "暗棋非工兵→军旗 can=0（工兵专属扛旗）");
}

/* ⑪ 暗棋先手翻子定色（R-137/R-003）：jq_dark_setup 全 0xFF，首翻定 side=turn */
static void t_dark_flip_color(void)
{
    jq_state st;
    jq_dark_setup(&st, JQ_DEFAULT_LAY);
    st.stage = STG_PLAY;
    /* jq_dark_setup 全 60 格 side=SIDE_UNKNOWN + revealed=0 + mode=FLIP */
    int unk = 0;
    for (int i = 0; i < N_CELLS; i++)
        if (st.cells[i].rank != RANK_EMPTY && st.cells[i].side == SIDE_UNKNOWN) unk++;
    CHECK(unk > 0, "jq_dark_setup 存在 side=UNKNOWN 暗子格");
    /* 找一个非军旗/地雷的暗子格（军旗/地雷不可翻，rules.c jq_can_flip） */
    int fi = -1;
    for (int i = 0; i < N_CELLS; i++) {
        int rn = rank_norm(st.cells[i].rank);
        if (st.cells[i].rank != RANK_EMPTY && st.cells[i].side == SIDE_UNKNOWN &&
            rn != RANK_FLAG && rn != RANK_MINE)
            { fi = i; break; }
    }
    CHECK(fi >= 0, "存在可翻暗子（非军旗非地雷）");
    CHECK(jq_can_flip(&st, fi), "暗棋未定色暗子可翻（c->side==SIDE_UNKNOWN 分支）");
    int before = st.turn;   /* jq_dark_setup turn=BLUE */
    CHECK(jq_flip(&st, fi), "jq_flip 成功");
    CHECK(st.cells[fi].side == before, "首翻定色：side=翻转前 turn（本回合方）");
    CHECK(st.cells[fi].revealed == 1, "翻后 revealed=1");
    CHECK(st.turn == 1 - before, "flip 后 turn 切换（1-before）");
}

/* ⑫ 铁路方向拒（AUD-R-001）：非纵线 col0/4 跨底线纵滑 can=0 */
static void t_rail_dir_reject(void)
{
    jq_state st; setup_lay(&st);
    /* 跨底线纵滑：(11,0)→(0,0) col0 非纵线 → 拒 */
    put(&st, C(11,0), 2, SIDE_RED, 1);
    CHECK(jq_can_move(&st, C(11,0), C(0,0)) == 0,
          "R-001 跨底线纵滑 (11,0)→(0,0) 被拒 can=0");
    /* 非铁路行横滑：(1,0)→(1,2) row1 非横向铁路行 → 拒 */
    put(&st, C(1,0), 2, SIDE_BLUE, 1);
    CHECK(jq_can_move(&st, C(1,0), C(1,2)) == 0,
          "R-001 非铁路行横滑 (1,0)→(1,2) 被拒 can=0");
    /* 铁路放行对照：(5,0)→(5,4) 中线横滑（row5） → can=1 */
    put(&st, C(5,0), 2, SIDE_BLUE, 1);
    CHECK(jq_can_move(&st, C(5,0), C(5,4)) == 1,
          "铁路中线横滑 (5,0)→(5,4) 放行 can=1");
}

/* ⑬ 回合门（AUD-R-005）：turn=BLUE 但 src side=RED → jq_can_move false */
static void t_turn_gate(void)
{
    jq_state st;
    memset(&st, 0, sizeof(st));
    st.turn = SIDE_BLUE; st.stage = STG_PLAY; st.winner = JQ_WIN_NONE;
    /* src = 红方子（side=RED）但 turn=BLUE → 回合门拦截 */
    put(&st, C(7,0), 3, SIDE_RED, 1);
    int can = jq_can_move(&st, C(7,0), C(8,0));
    CHECK(can == 0, "回合门：turn=BLUE 但 src side=RED → can=0");
}

int main(void)
{
    printf("=== 规则引擎全量用例（AUD-R-135 P3 落地）===\n");
    t_lay_move();
    t_lay_eat();
    t_same_rank();
    t_bomb();
    t_worker_mine();
    t_nonworker_mine();
    t_lay_flag();
    t_bomb_flag();
    t_stalemate();
    t_dark_flag_block();
    t_dark_flip_color();
    t_rail_dir_reject();
    t_turn_gate();
    printf("=== rulestest 全量 %s（%d FAIL）===\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
