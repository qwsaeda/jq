/*
 * game_state.c — 军棋状态管理（R2，手写路线）
 * 标准中国军棋（陆战棋）逻辑网格：12行×5列（60 格）。
 *   行0 蓝方底线（大本营 col1/col3），行11 红方底线（大本营 col1/col3）。
 *   每方 5×5=25 可放子格：19 兵站 + 4 行营 + 2 大本营（底线两侧）。
 * 行营（§86 确证）：蓝 (4,1)(4,3)(3,2)(2,1)(2,3)，红 (7,3)(7,1)(8,2)(9,3)(9,1)。
 * 每方 25 子。大本营内军旗不可动；行营敌方不可入。
 */
#include "junqi_core.h"
#include <string.h>

/* 行营（中心 2×2）：蓝 行2-3 列1-2；红 行8-9 列1-2 */
int jq_cell_index(int row, int col) {
    return row * JQ_BOARD_COLS + col;
}
int jq_cell_row(int idx) { return idx / JQ_BOARD_COLS; }
int jq_cell_col(int idx) { return idx % JQ_BOARD_COLS; }

bool jq_is_camp(int row, int col) {
    /* 行营：default.lay 空位（每方 5 格）。蓝方(行0-5)：layout空位(1,1)(1,3)(2,2)(3,1)(3,3)
       经 行=5-lrow 映射 → 棋盘(4,1)(4,3)(3,2)(2,1)(2,3)。
       红方(行6-11)：镜像 → (7,3)(7,1)(8,2)(9,3)(9,1)。 */
    if (row == 4 && (col == 1 || col == 3)) return true;   /* 蓝 */
    if (row == 3 && col == 2) return true;
    if (row == 2 && (col == 1 || col == 3)) return true;
    if (row == 7 && (col == 1 || col == 3)) return true;   /* 红 */
    if (row == 8 && col == 2) return true;
    if (row == 9 && (col == 1 || col == 3)) return true;
    return false;
}
bool jq_is_base(int row, int col) {
    /* 大本营：军旗所在底线格 + 相邻底线格。蓝(行0) col1/col3；红(行11) col1/col3。 */
    if (row == 0 && (col == 1 || col == 3)) return true;   /* 蓝 */
    if (row == 11 && (col == 1 || col == 3)) return true;  /* 红 */
    return false;
}

/* 铁路：原版军棋"十"字铁路——纵向 col2 贯穿所有行 + 横向 row5/6（中线）贯穿所有列。
 * 行营/大本营是特殊格（非铁路，由 walkable 阻断）。其余格为普通路（只走1步，L68）。 */
bool jq_is_rail(int row, int col) {
    if (jq_is_camp(row, col)) return false;
    if (jq_is_base(row, col)) return false;
    if (col == 2) return true;                    /* 纵向中线贯穿 */
    if (row == 5 || row == 6) return true;        /* 横向中线 row5/6 贯穿 */
    return false;
}

void jq_state_init(jq_state *st, jq_stage stage) {
    memset(st, 0, sizeof(*st));
    st->turn = JQ_SIDE_BLUE;
    st->stage = stage;
    st->winner = 0xFF;
}

void jq_state_reset(jq_state *st) {
    memset(st, 0, sizeof(*st));
    st->turn = JQ_SIDE_BLUE;
    st->winner = 0xFF;
}

void jq_place(jq_state *st, int idx, uint8_t side, uint8_t rank) {
    if (idx < 0 || idx >= JQ_CELLS) return;
    st->board[idx].rank = rank;
    st->board[idx].side = side;
    st->board[idx].revealed = 1;
    st->board[idx].is_flag = (rank == JQ_RANK_FLAG);
    st->board[idx].is_mine = (rank == JQ_RANK_MINE);
    st->board[idx].is_bomb = (rank == JQ_RANK_BOMB);
    if (side == JQ_SIDE_BLUE || side == JQ_SIDE_RED)
        st->pieces_left[side]++;
}
