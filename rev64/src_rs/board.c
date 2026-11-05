/*
 * board.c — 棋盘几何 / 状态初始化（逆源码重写）
 * 依据：原版 0x6ef54 坐标比较（[0]=col [1]=row，L51/L79）+
 *       L68 十字铁路（col2 纵线贯穿 + row5/6 横线中线；行营/大本营在铁路外）+
 *       L49 铁路修复（行营非铁路不可穿行、工兵可拐弯、其他子直走）。
 * 棋盘 12×5=60 格；蓝方 row0-5（row0 底线含大本营 col1/3），红方 row6-11。
 */
#include "junqi.h"

/* 行营 = default.lay 5 个空位（layout idx 6,8,12,16,18 → 蓝棋盘 (4,1)(4,3)(3,2)(2,1)(2,3)，
   红镜像 (7,3)(7,1)(8,2)(9,3)(9,1)）。行营是安全岛，非铁路不可穿行。 */
bool is_camp(int row, int col)
{
    if (row == 2 && (col == 1 || col == 3)) return true;
    if (row == 3 && col == 2) return true;
    if (row == 4 && (col == 1 || col == 3)) return true;
    if (row == 7 && (col == 1 || col == 3)) return true;
    if (row == 8 && col == 2) return true;
    if (row == 9 && (col == 1 || col == 3)) return true;
    return false;
}

/* 大本营：底线两侧（蓝 row0 col1/3，红 row11 col1/3）。
   原版 0x2a228 军旗格 56/58 = row11col1/col3（红大本营）。 */
bool is_base(int row, int col)
{
    return ((row == 0 || row == 11) && (col == 1 || col == 3));
}

/* 铁路（L68 十字，对齐原版军棋规则）：
   纵向：col2 纵线贯穿全高；双方底线 row0（蓝方）/row11（红方）横向贯通。
   横向：row5/6（中线）贯通全宽。
   行营/大本营在铁路外（安全格不可穿行）。
   双方底线为铁路是原版军棋标准（否则底角棋子无纵向通道，困毙误判）。 */
bool is_rail(int row, int col)
{
    if (is_camp(row, col) || is_base(row, col)) return false;
    if (col == 2) return true;             /* 纵线 */
    if (row == 5 || row == 6) return true; /* 中线 */
    if (row == 0 || row == 11) return true;/* 双方底线 */
    return false;
}

int cell_index(int row, int col) { return row * BOARD_COLS + col; }
int cell_row(int idx) { return idx / BOARD_COLS; }
int cell_col(int idx) { return idx % BOARD_COLS; }

int coord_cmp(const int8_t *a, const int8_t *b)
{
    /* 0x6ef54：[0]=col [1]=row，0=相同 1=不同（坐标对逐分量） */
    if (a[0] != b[0] || a[1] != b[1]) return 1;
    return 0;
}
